#pragma once

#include "../defines.hpp"
#include "../widgets/Background.hpp"
#include "../widgets/PasswordInputField.hpp"
#include "../widgets/Label.hpp"
#include <vector>
#include <optional>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/core/Input.hpp>

class CHyprlock {
  public:
    CHyprlock(const bool immediateRender, const int gracePeriod);
    ~CHyprlock() = default;

    void                     createLockSurface(SP<Hyprtoolkit::IOutput> output);
    void                     setupLockSurfaceWindow(const SP<Hyprtoolkit::IWindow>& window, const SP<Hyprtoolkit::IOutput> output);
    void                     run();

    void                     unlock();

    ASP<Hyprtoolkit::CTimer> addTimer(const std::chrono::system_clock::duration& timeout, std::function<void(ASP<Hyprtoolkit::CTimer> self, void* data)> cb_, void* data,
                                      bool force = false);
    //void                             processTimers();

    void                       enqueueForceUpdateTimers();

    void                       onLockLocked();
    void                       onLockFinished();

    bool                       acquireSessionLock();
    void                       releaseSessionLock();

    void                       onKey(Hyprtoolkit::Input::SKeyboardKeyEvent e);
    void                       handleKeySym(Hyprtoolkit::Input::SKeyboardKeyEvent e);
    void                       onPasswordCheckTimer();
    bool                       passwordCheckWaiting();
    std::optional<std::string> passwordLastFailReason();

    void                       displayPlaceholder();
    void                       displayCheckWaiting();
    void                       displayFail();

    bool                       m_capsLock = false;
    bool                       m_numLock  = false;
    bool                       m_ctrl     = false;

    bool                       m_immediateRender = false;

    bool                       m_unlocked = false;

    std::string                m_sCurrentDesktop = "";

    //
    std::chrono::system_clock::time_point m_tGraceEnds;

    const SP<Hyprtoolkit::IBackend>&      backend();

    struct SWidgetsByType {
        std::vector<UP<Widgets::CBackground>>         backgrounds;
        std::vector<UP<Widgets::CPasswordInputField>> passwordInputFields;
        std::vector<UP<Widgets::CLabel>>              labels;
    };

    const std::vector<std::pair<SP<Hyprtoolkit::IWindow>, SWidgetsByType>>& ui();

  private:
    SP<Hyprtoolkit::IBackend>                                        m_backend;
    SP<Hyprtoolkit::ISessionLockState>                               m_sessionLockState;

    std::vector<std::pair<SP<Hyprtoolkit::IWindow>, SWidgetsByType>> m_ui;
};

inline UP<CHyprlock> g_hyprlock;
