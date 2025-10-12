#include "PasswordInputField.hpp"
#include "IWidget.hpp"
#include "../helpers/Log.hpp"
#include "../helpers/MiscFunctions.hpp"
#include "../config/ConfigManager.hpp"
#include "../core/hyprlock.hpp"
#include <chrono>
#include <hyprlang.hpp>
#include <filesystem>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/types/SizeType.hpp>

using namespace Widgets;

void CPasswordInputField::configure(const std::unordered_map<std::string, std::any>& props, const SP<Hyprtoolkit::IOutput>& output) {
    try {
        m_config = {
            .colors =
                {
                    .both       = *CGradientValueData::fromAnyPv(props.at("bothlock_color")),
                    .caps       = *CGradientValueData::fromAnyPv(props.at("capslock_color")),
                    .check      = *CGradientValueData::fromAnyPv(props.at("check_color")),
                    .fail       = *CGradientValueData::fromAnyPv(props.at("fail_color")),
                    .outer      = *CGradientValueData::fromAnyPv(props.at("outer_color")),
                    .num        = *CGradientValueData::fromAnyPv(props.at("numlock_color")),
                    .font       = std::any_cast<Hyprlang::INT>(props.at("font_color")),
                    .hiddenBase = std::any_cast<Hyprlang::INT>(props.at("hide_input_base_color")),
                    .inner      = std::any_cast<Hyprlang::INT>(props.at("inner_color")),
                    .invertNum  = std::any_cast<Hyprlang::INT>(props.at("invert_numlock")),
                },

            .dots =
                {
                    .center     = std::any_cast<Hyprlang::INT>(props.at("dots_center")),
                    .size       = std::any_cast<Hyprlang::FLOAT>(props.at("dots_size")),
                    .spacing    = std::any_cast<Hyprlang::FLOAT>(props.at("dots_spacing")),
                    .rounding   = std::any_cast<Hyprlang::INT>(props.at("dots_rounding")),
                    .textFormat = std::any_cast<Hyprlang::STRING>(props.at("dots_text_format")),
                },

            .fadeOnEmpty     = std::any_cast<Hyprlang::INT>(props.at("fade_on_empty")),
            .hiddenInput     = std::any_cast<Hyprlang::INT>(props.at("hide_input")),
            .swapFont        = std::any_cast<Hyprlang::INT>(props.at("swap_font_color")),
            .outThick        = std::any_cast<Hyprlang::INT>(props.at("outline_thickness")),
            .rounding        = std::any_cast<Hyprlang::INT>(props.at("rounding")),
            .fadeTimeoutMs   = std::any_cast<Hyprlang::INT>(props.at("fade_timeout")),
            .size            = *CLayoutValueData::fromAnyPv(props.at("size")),
            .pos             = *CLayoutValueData::fromAnyPv(props.at("position")),
            .placeholderText = std::any_cast<Hyprlang::STRING>(props.at("placeholder_text")),
            .failText        = std::any_cast<Hyprlang::STRING>(props.at("fail_text")),
            .fontFamily      = std::any_cast<Hyprlang::STRING>(props.at("font_family")),
            .halign          = std::any_cast<Hyprlang::STRING>(props.at("halign")),
            .valign          = std::any_cast<Hyprlang::STRING>(props.at("valign")),
        };
    } catch (const std::bad_any_cast& e) {
        RASSERT(false, "Failed to construct CPasswordInputField: {}", e.what()); //
    } catch (const std::out_of_range& e) {
        RASSERT(false, "Missing property for CPasswordInputField: {}", e.what()); //
    }
}

void CPasswordInputField::layout(const SP<Hyprtoolkit::IElement>& root, const Vector2D& viewport) {
    m_rect = Hyprtoolkit::CRectangleBuilder::begin()
                 ->color([this] { return m_inner; })
                 ->borderColor([this] { return m_outer; })
                 ->borderThickness(m_config.outThick)
                 ->rounding(m_config.rounding)
                 ->size(m_config.size.toDynamicSize())
                 ->commence();

    const auto SIZE = m_config.size.getAbsolute(viewport);
    const auto POS  = posFromHVAlign(viewport, SIZE, m_config.pos.getAbsolute(viewport), m_config.halign, m_config.valign);
    Debug::log(LOG, "vieport {} config pos {} afer hv position {}", viewport, m_config.pos.toString(), POS);
    // m_rect->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    // m_rect->setAbsolutePosition(POS);

    m_textbox = Hyprtoolkit::CTextboxBuilder::begin()
                    ->placeholder(std::string(m_config.placeholderText))
                    ->password(true)
                    ->multiline(false)
                    ->size(m_config.size.toDynamicSize())
                    ->commence();

    m_textbox->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    m_textbox->setAbsolutePosition(POS);

    root->addChild(m_textbox);
    root->addChild(m_rect);

    m_textbox->focus(true);
}

std::string_view CPasswordInputField::getCurrentInput() {
    return m_textbox->currentText();
}
