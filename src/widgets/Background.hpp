#pragma once

#include "IWidget.hpp"
#include "../defines.hpp"
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Color.hpp>
#include <hyprutils/math/Misc.hpp>
#include <string>
#include <unordered_map>
#include <any>
#include <filesystem>

namespace Widgets {
    struct SBackgroundConfig {
        int         blurPasses = 10;
        int         blurSize   = 3;
        float       brightness = 0.8172;
        CHyprColor  color;
        float       contrast = 0.8916;
        float       noise    = 0.0117;
        std::string path;
        std::string reloadCommand;
        int         reloadTime        = -1;
        float       vibrancy          = 0.1696;
        float       vibrancy_darkness = 0.0;
    };

    class CBackground : public IWidget {
      public:
        CBackground()  = default;
        ~CBackground() = default;

        virtual void configure(const std::unordered_map<std::string, std::any>& props, const SP<Hyprtoolkit::IOutput>& pOutput);
        virtual void layout(const SP<Hyprtoolkit::IElement>& root, const Vector2D& viewport);

      private:
        SBackgroundConfig                  m_config;

        SP<Hyprtoolkit::CRectangleElement> m_rect;
        SP<Hyprtoolkit::CImageElement>     m_image;

        bool                               m_isScreenshot = false;

        // when reload is used
        std::filesystem::file_time_type m_modificationTime;
        SP<Hyprtoolkit::CTimer>         m_reloadTimer;
    };
}
