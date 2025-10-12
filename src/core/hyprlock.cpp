#include "hyprlock.hpp"

#include "../helpers/Log.hpp"
#include "../config/ConfigManager.hpp"
#include "../auth/Auth.hpp"
#include "../auth/Fingerprint.hpp"
#include "../widgets/IWidget.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/palette/Color.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/os/Process.hpp>
#include <malloc.h>
#include <sdbus-c++/sdbus-c++.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xf86drm.h>

using namespace Hyprutils::OS;

static void setMallocThreshold() {
#ifdef M_TRIM_THRESHOLD
    // The default is 128 pages,
    // which is very large and can lead to a lot of memory used for no reason
    // because trimming hasn't happened
    static const int PAGESIZE = sysconf(_SC_PAGESIZE);
    mallopt(M_TRIM_THRESHOLD, 6 * PAGESIZE);
#endif
}

CHyprlock::CHyprlock(const bool immediateRender, const int graceSeconds) {
    setMallocThreshold();

    static const auto IMMEDIATERENDER = g_configManager->getValue<Hyprlang::INT>("general:immediate_render");
    m_immediateRender                 = immediateRender || *IMMEDIATERENDER;

    const auto CURRENTDESKTOP = getenv("XDG_CURRENT_DESKTOP");
    const auto SZCURRENTD     = std::string{CURRENTDESKTOP ? CURRENTDESKTOP : ""};
    m_sCurrentDesktop         = SZCURRENTD;

    m_backend = Hyprtoolkit::IBackend::create();

    g_auth = makeUnique<CAuth>();

    if (graceSeconds > 0)
        m_tGraceEnds = std::chrono::system_clock::now() + std::chrono::seconds(graceSeconds);
    else
        m_tGraceEnds = std::chrono::system_clock::from_time_t(0);
}

static void registerSignalAction(int sig, void (*handler)(int), int sa_flags = 0) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = sa_flags;
    sigaction(sig, &sa, nullptr);
}

static void handleUnlockSignal(int sig) {
    if (sig == SIGUSR1) {
        Debug::log(LOG, "Unlocking with a SIGUSR1");
        g_auth->enqueueUnlock();
    }
}

//static void handleForceUpdateSignal(int sig) {
//    if (sig == SIGUSR2) {
//        for (auto& t : g_hyprlock->getTimers()) {
//            if (t->canForceUpdate()) {
//                t->call(t);
//                t->cancel();
//            }
//        }
//    }
//}

void CHyprlock::createLockSurface(SP<Hyprtoolkit::IOutput> output) {
    auto window = Hyprtoolkit::CWindowBuilder::begin()->type(Hyprtoolkit::HT_WINDOW_LOCK_SURFACE)->prefferedOutput(output)->commence();
    if (!window) {
        Debug::log(ERR, "Failed new session lock surface!");
        return;
    }
    Debug::log(LOG, "New session lock surface on monitor"); // TODO: offer monitor name/desc via IOutput

    WP<Hyprtoolkit::IWindow> weakWindow = window;
    window->m_events.closeRequest.listenStatic([this, weakWindow]() {
        Debug::log(LOG, "Remove session lock surface!");
        auto windowsIt = std::ranges::find_if(m_ui, [weakWindow](const auto& pair) { return pair.first == weakWindow; });
        if (windowsIt == m_ui.end())
            return;

        (*windowsIt).second.backgrounds.clear();
        (*windowsIt).second.passwordInputFields.clear();
        (*windowsIt).first->close();
        m_ui.erase(windowsIt);
        // TODO: we need to add awareness for the finished event here.
        if (m_unlocked && m_ui.empty()) {
            Debug::log(LOG, "We unlocked and all windows have been destroyed. Seems like we are done here");
            m_backend->destroy();
        }
    });

    window->m_events.keyboardKey.listenStatic([this](Hyprtoolkit::Input::SKeyboardKeyEvent e) { onKey(e); });

    setupLockSurfaceWindow(window, output);
}

void CHyprlock::setupLockSurfaceWindow(const SP<Hyprtoolkit::IWindow>& window, const SP<Hyprtoolkit::IOutput> output) {
    auto WIDGETS = g_configManager->getWidgetConfigs();

    std::ranges::sort(WIDGETS, [](CConfigManager::SWidgetConfig& a, CConfigManager::SWidgetConfig& b) {
        return std::any_cast<Hyprlang::INT>(a.values.at("zindex")) < std::any_cast<Hyprlang::INT>(b.values.at("zindex"));
    });

    m_ui.emplace_back(window, SWidgetsByType{});
    auto& widgetsByType = m_ui.back().second;

    for (auto& c : WIDGETS) {
        // by type
        if (c.type == "background") {
            widgetsByType.backgrounds.emplace_back(makeUnique<Widgets::CBackground>());
            widgetsByType.backgrounds.back()->configure(c.values, output);
        } else if (c.type == "input-field") {
            widgetsByType.passwordInputFields.emplace_back(makeUnique<Widgets::CPasswordInputField>());
            widgetsByType.passwordInputFields.back()->configure(c.values, output);
        } else if (c.type == "label") {
            widgetsByType.labels.emplace_back(makeUnique<Widgets::CLabel>());
            widgetsByType.labels.back()->configure(c.values, output);
            widgetsByType.labels.back()->m_self = widgetsByType.labels.back();
        }
        // else if (c.type == "shape")
        // else if (c.type == "image")
        else {
            Debug::log(ERR, "Unknown widget type: {}", c.type);
            continue;
        }
    }

    window->open();

    WP<Hyprtoolkit::IWindow> weakWindow = window;
    window->m_rootElement->setRepositioned([this, weakWindow] {
        if (!weakWindow)
            return;

        Debug::log(LOG, "window reposition: {}", weakWindow->pixelSize());
        const auto VIEWPORT = weakWindow->pixelSize() * weakWindow->scale();
        for (const auto& [w, ww] : m_ui) {
            if (w == weakWindow) {
                for (const auto& b : ww.backgrounds) {
                    b->layout(w->m_rootElement, VIEWPORT);
                }
                for (const auto& pwf : ww.passwordInputFields) {
                    pwf->layout(w->m_rootElement, VIEWPORT);
                }
                for (const auto& l : ww.labels) {
                    l->layout(w->m_rootElement, VIEWPORT);
                }
            }
        }
        // monitors can't really be resized in a wayland typical manor.
        // we expect the output size to be constant.
        // thus we can remove this listener
        weakWindow->m_rootElement->setRepositioned([] {});
    });
}

void CHyprlock::run() {
    Debug::log(LOG, "Running on {}", m_sCurrentDesktop);

    g_auth->start();

    Debug::log(LOG, "Stated auth");
    // All of this needs to be done with ht somehow
    //g_asyncResourceManager->enqueueStaticAssets();
    //g_asyncResourceManager->enqueueScreencopyFrames();

    //if (!g_hyprlock->m_bImmediateRender)
    // Gather background resources and screencopy frames before locking the screen.
    // We need to do this because as soon as we lock the screen, workspaces frames can no longer be captured. It either won't work at all, or we will capture hyprlock itself.
    // Bypass with --immediate-render (can cause the background first rendering a solid color and missing or inaccurate screencopy frames)
    //g_asyncResourceManager->gatherInitialResources(m_sWaylandState.display);
    if (!m_backend) {
        Debug::log(CRIT, "Failed to create the HT backend!");
        return;
    }

    m_backend->m_events.outputAdded.listenStatic([this](auto output) { createLockSurface(output); });

    auto sessionLockState = m_backend->aquireSessionLock();
    if (sessionLockState.has_value())
        m_sessionLockState = sessionLockState.value();

    if (!m_sessionLockState) {
        Debug::log(CRIT, "Couldn't lock the session. Is another lockscreen running?");
        return;
    }

    for (const auto& o : m_backend->getOutputs()) {
        createLockSurface(o);
    }

    registerSignalAction(SIGUSR1, handleUnlockSignal, SA_RESTART);
    //registerSignalAction(SIGUSR2, handleForceUpdateSignal);

    // const auto fingerprintAuth = g_auth->getImpl(AUTH_IMPL_FINGERPRINT);
    // const auto dbusConn        = (fingerprintAuth) ? ((CFingerprint*)fingerprintAuth.get())->getConnection() : nullptr;
    // m_backend->addFd(dbusConn->getEventLoopPollData().fd, [dbusConn]() {
    //     while (dbusConn && dbusConn->processPendingEvent()) {
    //         ;
    //     }
    // });

    Debug::log(LOG, "Entering the loop");

    m_backend->enterLoop();

    g_auth->terminate();

    Debug::log(LOG, "Reached the end, exiting");
}

void CHyprlock::unlock() {
    // TODO: fade out
    releaseSessionLock();
}

void CHyprlock::onKey(Hyprtoolkit::Input::SKeyboardKeyEvent e) {
    if (m_unlocked)
        return;

    if (e.down && std::chrono::system_clock::now() < m_tGraceEnds) {
        unlock();
        return;
    }

    if (g_auth->m_displayFailText) {
        g_auth->resetDisplayFail();
        displayPlaceholder();
    }

    if (e.down)
        handleKeySym(e);
}

void CHyprlock::handleKeySym(Hyprtoolkit::Input::SKeyboardKeyEvent e) {
    const auto SYM = e.xkbKeysym;

    if (SYM == XKB_KEY_Return || SYM == XKB_KEY_KP_Enter) {
        Debug::log(LOG, "Authenticating");

        // static const auto IGNOREEMPTY = g_configManager->getValue<Hyprlang::INT>("general:ignore_empty_input");

        if (g_auth->checkWaiting())
            return;

        displayCheckWaiting();

        // TODO: we need to track focus somehow and select that password field
        for (const auto& [w, ww] : m_ui) {
            for (const auto& pwf : ww.passwordInputFields) {
                const auto CURRENTINPUT = pwf->getCurrentInput();
                if (CURRENTINPUT.empty())
                    continue;

                g_auth->submitInput(CURRENTINPUT);
                break;
            }
        }
    } else if (SYM == XKB_KEY_Caps_Lock) {
        m_capsLock = !m_capsLock;
    } else if (SYM == XKB_KEY_Num_Lock) {
        m_numLock = !m_numLock;
    }
}

void CHyprlock::releaseSessionLock() {
    Debug::log(LOG, "Unlocking session");
    m_unlocked = true;
    m_sessionLockState->unlock();
}

// TODO: remove??
ASP<Hyprtoolkit::CTimer> CHyprlock::addTimer(const std::chrono::system_clock::duration& timeout, std::function<void(ASP<Hyprtoolkit::CTimer> self, void* data)> cb_, void* data,
                                             bool force) {
    return m_backend->addTimer(timeout, cb_, data, force);
}

// TODO: force update????
//std::vector<ASP<Hyprtoolkit::CTimer>> CHyprlock::getTimers() {
//    return m_vTimers;
//}
//
void CHyprlock::enqueueForceUpdateTimers() {
    //    addTimer(
    //        std::chrono::milliseconds(1),
    //        [](ASP<Hyprtoolkit::CTimer> self, void* data) {
    //            for (auto& t : g_hyprlock->getTimers()) {
    //                if (t->canForceUpdate()) {
    //                    t->call(t);
    //                    t->cancel();
    //                }
    //            }
    //        },
    //        nullptr, false);
}

void CHyprlock::displayPlaceholder() {
    for (const auto& [w, ww] : m_ui) {
        for (const auto& pwf : ww.passwordInputFields) {
            // pwf->setPlaceholder();
        }
    }
}

void CHyprlock::displayCheckWaiting() {
    for (const auto& [w, ww] : m_ui) {
        for (const auto& pwf : ww.passwordInputFields) {
            // pwf->setWaiting();
        }
    }
}

void CHyprlock::displayFail() {
    for (const auto& [w, ww] : m_ui) {
        for (const auto& pwf : ww.passwordInputFields) {
            // pwf->setPasswordLength(0);
        }

        for (const auto& pwf : ww.passwordInputFields) {
            // pwf->setFail();
        }
    }
}

const SP<Hyprtoolkit::IBackend>& CHyprlock::backend() {
    return m_backend;
}

const std::vector<std::pair<SP<Hyprtoolkit::IWindow>, CHyprlock::SWidgetsByType>>& CHyprlock::ui() {
    return m_ui;
}
