#include "Auth.hpp"
#include "Pam.hpp"
#include "Fingerprint.hpp"
#include "../config/ConfigManager.hpp"
#include "../core/hyprlock.hpp"
#include "src/helpers/Log.hpp"

#include <hyprlang.hpp>

CAuth::CAuth() {
    static const auto ENABLEPAM = g_configManager->getValue<Hyprlang::INT>("auth:pam:enabled");
    if (*ENABLEPAM)
        m_vImpls.emplace_back(makeShared<CPam>());
    static const auto ENABLEFINGERPRINT = g_configManager->getValue<Hyprlang::INT>("auth:fingerprint:enabled");
    if (*ENABLEFINGERPRINT)
        m_vImpls.emplace_back(makeShared<CFingerprint>());

    RASSERT(!m_vImpls.empty(), "At least one authentication method must be enabled!");
}

void CAuth::start() {
    for (const auto& i : m_vImpls) {
        i->init();
    }
}

void CAuth::submitInput(std::string_view input) {
    for (const auto& i : m_vImpls) {
        i->handleInput(input);
    }
}

bool CAuth::checkWaiting() {
    return std::ranges::any_of(m_vImpls, [](const auto& i) { return i->checkWaiting(); });
}

const std::string& CAuth::getCurrentFailText() {
    return m_currentFail.failText;
}

std::optional<std::string> CAuth::getFailText(eAuthImplementations implType) {
    for (const auto& i : m_vImpls) {
        if (i->getImplType() == implType)
            return i->getLastFailText();
    }
    return std::nullopt;
}

std::optional<std::string> CAuth::getPrompt(eAuthImplementations implType) {
    for (const auto& i : m_vImpls) {
        if (i->getImplType() == implType)
            return i->getLastPrompt();
    }
    return std::nullopt;
}

size_t CAuth::getFailedAttempts() {
    return m_failedAttempts;
}

SP<IAuthImplementation> CAuth::getImpl(eAuthImplementations implType) {
    for (const auto& i : m_vImpls) {
        if (i->getImplType() == implType)
            return i;
    }

    return nullptr;
}

void CAuth::terminate() {
    for (const auto& i : m_vImpls) {
        i->terminate();
    }
}

static void unlockCallback(ASP<Hyprtoolkit::CTimer> self, void* data) {
    g_hyprlock->unlock();
}

void CAuth::enqueueUnlock() {
    g_hyprlock->addTimer(std::chrono::milliseconds(0), unlockCallback, nullptr);
}

void CAuth::enqueueFail(const std::string& failText, eAuthImplementations implType) {
    static const auto FAILTIMEOUT = g_configManager->getValue<Hyprlang::INT>("general:fail_timeout");

    m_currentFail.failText   = failText;
    m_currentFail.failSource = implType;
    m_failedAttempts++;

    Debug::log(LOG, "Failed attempts: {}", m_failedAttempts);

    if (m_resetDisplayFailTimer) {
        m_resetDisplayFailTimer->cancel();
        m_resetDisplayFailTimer.reset();
    }

    g_hyprlock->addTimer(
        std::chrono::milliseconds(0),
        [](auto, auto) {
            g_hyprlock->displayFail();
            g_auth->m_displayFailText = true;
        },
        nullptr);
    m_resetDisplayFailTimer = g_hyprlock->addTimer(std::chrono::milliseconds(*FAILTIMEOUT), [](auto, auto) { g_hyprlock->displayPlaceholder(); }, nullptr);
}

void CAuth::resetDisplayFail() {
    m_resetDisplayFailTimer->cancel();
    m_resetDisplayFailTimer.reset();
}
