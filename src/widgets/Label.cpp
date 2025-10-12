#include "Label.hpp"
#include "../helpers/Log.hpp"
#include "../helpers/MiscFunctions.hpp"
#include "../core/hyprlock.hpp"
#include "../config/ConfigManager.hpp"
#include <chrono>
#include <hyprlang.hpp>
#include <filesystem>

#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace Widgets;
using namespace Hyprutils::OS;

void CLabel::configure(const std::unordered_map<std::string, std::any>& props, const SP<Hyprtoolkit::IOutput>& output) {
    try {
        m_config = {
            .pos            = *CLayoutValueData::fromAnyPv(props.at("position")),
            .fontSize       = std::any_cast<Hyprlang::INT>(props.at("font_size")),
            .angle          = std::any_cast<Hyprlang::FLOAT>(props.at("rotate")) * M_PI / 180.0,
            .color          = std::any_cast<Hyprlang::INT>(props.at("color")),
            .textAlign      = parseTextAlignment(std::any_cast<Hyprlang::STRING>(props.at("text_align"))),
            .labelPreFormat = std::any_cast<Hyprlang::STRING>(props.at("text")),
            .onClickCommand = std::any_cast<Hyprlang::STRING>(props.at("onclick")),
            .fontFamily     = std::any_cast<Hyprlang::STRING>(props.at("font_family")),
            .halign         = std::any_cast<Hyprlang::STRING>(props.at("halign")),
            .valign         = std::any_cast<Hyprlang::STRING>(props.at("valign")),
        };
    } catch (const std::bad_any_cast& e) {
        RASSERT(false, "Failed to construct CLabel: {}", e.what()); //
    } catch (const std::out_of_range& e) {
        RASSERT(false, "Missing property for CLabel: {}", e.what()); //
    }
}

void CLabel::layout(const SP<Hyprtoolkit::IElement>& root, const Vector2D& viewport) {
    m_text = Hyprtoolkit::CTextBuilder::begin()
                 //->text("")
                 ->color([this] { return m_config.color; })
                 ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                 ->fontSize({Hyprtoolkit::CFontSize::HT_FONT_ABSOLUTE, m_config.fontSize})
                 ->fontFamily(std::string(m_config.fontFamily))
                 ->callback([this, viewport]() {
                     const auto POS = posFromHVAlign(viewport, m_text->size(), m_config.pos.getAbsolute(viewport), m_config.halign, m_config.valign);
                     m_text->setAbsolutePosition(POS);
                 })
                 ->commence();

    m_text->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);

    root->addChild(m_text);

    updateText();
}

void CLabel::updateText() {
    auto label = formatString(m_config.labelPreFormat);

    if (label.cmd)
        updateTextCmd(label);
    else if (label.formatted != m_prevText) {
        m_prevText = label.formatted;
        m_text->rebuild()->text(std::move(label.formatted))->commence();
    }

    plantTimer(label);
}

void CLabel::updateTextCmd(const SFormatResult& label) {
    m_asyncCommand.out.clear();
    m_asyncCommand.proc = makeUnique<CProcess>("/bin/sh", std::vector<std::string>{"-c", label.formatted + ";echo -n HYPRLOCKEOF"});

    int outPipe[2];
    if (pipe(outPipe))
        return;

    m_asyncCommand.proc->setStdoutFD(outPipe[1]);
    if (!m_asyncCommand.proc->runAsync())
        return;

    close(outPipe[1]);
    int fdFlags = fcntl(outPipe[0], F_GETFL, 0);
    if (fcntl(outPipe[0], F_SETFL, fdFlags | O_NONBLOCK) < 0)
        return;

    g_hyprlock->backend()->addFd(outPipe[0], [this, ref = m_self, outPipe]() {
        if (!ref)
            return;

        if (!m_asyncCommand.proc)
            return;

        int                    ret = 0;
        std::array<char, 1024> buf;
        buf.fill(0);
        while ((ret = read(outPipe[0], buf.data(), 1023)) > 0) {
            m_asyncCommand.out += std::string_view{buf.data(), sc<size_t>(ret)};
        }

        // TODO: Remove this stupid HYPRLOCKEOF workaround
        if (m_asyncCommand.out.ends_with("HYPRLOCKEOF")) {
            m_asyncCommand.proc.reset();
            // Using removeFd here would cause some problems.
            // Instead defere it with a timer.
            g_hyprlock->addTimer(
                std::chrono::milliseconds(0),
                [this, ref, outPipe](auto, auto) {
                    if (!ref)
                        return;

                    close(outPipe[0]);
                    g_hyprlock->backend()->removeFd(outPipe[0]);

                    std::string text = m_asyncCommand.out.substr(0, m_asyncCommand.out.size() - std::string_view("HYPRLOCKEOF").size());
                    Debug::log(TRACE, "Label text: {}", text);

                    m_text->rebuild()->text(std::move(text))->commence();
                },
                nullptr);
        }
    });
}

static void onTimer(WP<CLabel> ref) {
    if (!ref)
        return;

    // update label
    ref->updateText();
}

void CLabel::plantTimer(const SFormatResult& label) {
    if (label.updateEveryMs != 0)
        m_labelTimer = g_hyprlock->addTimer(std::chrono::milliseconds((int)label.updateEveryMs), [REF = m_self](auto, auto) { onTimer(REF); }, this, label.allowForceUpdate);
    else if (label.updateEveryMs == 0 && label.allowForceUpdate)
        m_labelTimer = g_hyprlock->addTimer(std::chrono::hours(1), [REF = m_self](auto, auto) { onTimer(REF); }, this, true);
}
