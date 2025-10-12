#include "Background.hpp"
#include "../helpers/Log.hpp"
#include "../helpers/MiscFunctions.hpp"
#include "../config/ConfigManager.hpp"
#include <chrono>
#include <hyprlang.hpp>
#include <filesystem>

using namespace Widgets;

void CBackground::configure(const std::unordered_map<std::string, std::any>& props, const SP<Hyprtoolkit::IOutput>& output) {
    try {
        m_config = {
            .blurPasses        = std::any_cast<Hyprlang::INT>(props.at("blur_passes")),
            .blurSize          = std::any_cast<Hyprlang::INT>(props.at("blur_size")),
            .brightness        = std::any_cast<Hyprlang::FLOAT>(props.at("brightness")),
            .color             = std::any_cast<Hyprlang::INT>(props.at("color")),
            .contrast          = std::any_cast<Hyprlang::FLOAT>(props.at("contrast")),
            .noise             = std::any_cast<Hyprlang::FLOAT>(props.at("noise")),
            .path              = std::any_cast<Hyprlang::STRING>(props.at("path")),
            .reloadCommand     = std::any_cast<Hyprlang::STRING>(props.at("reload_cmd")),
            .reloadTime        = std::any_cast<Hyprlang::INT>(props.at("reload_time")),
            .vibrancy          = std::any_cast<Hyprlang::FLOAT>(props.at("vibrancy")),
            .vibrancy_darkness = std::any_cast<Hyprlang::FLOAT>(props.at("vibrancy_darkness")),
        };
    } catch (const std::bad_any_cast& e) {
        RASSERT(false, "Failed to construct CBackground: {}", e.what()); //
    } catch (const std::out_of_range& e) {
        RASSERT(false, "Missing property for CBackground: {}", e.what()); //
    }

    m_isScreenshot = m_config.path == "screenshot";
}

void CBackground::layout(const SP<Hyprtoolkit::IElement>& root, const Vector2D& viewport) {
    m_rect = Hyprtoolkit::CRectangleBuilder::begin()
                 ->color([this] { return m_config.color; })
                 ->rounding(0)
                 ->borderThickness(0)
                 ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                 ->commence();

    m_image = Hyprtoolkit::CImageBuilder::begin() //
                  ->path(std::string(m_config.path))
                  ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                  ->commence();

    root->addChild(m_rect);
    root->addChild(m_image);
}
