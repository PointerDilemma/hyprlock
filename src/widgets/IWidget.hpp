#pragma once

#include "../defines.hpp"
#include "../helpers/Math.hpp"
#include <hyprgraphics/resource/resources/TextResource.hpp>
#include <hyprlang.hpp>
#include <hyprtoolkit/core/Output.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/palette/Color.hpp>

#include <any>
#include <string>
#include <unordered_map>

namespace Widgets {
    Vector2D posFromHVAlign(const Vector2D& viewport, const Vector2D& size, const Vector2D& offset, const std::string& halign, const std::string& valign, const double& ang = 0);
    int      roundingForBox(const CBox& box, int roundingConfig);
    int      roundingForBorderBox(const CBox& borderBox, int roundingConfig, int thickness);
    Hyprgraphics::CTextResource::eTextAlignmentMode parseTextAlignment(const std::string& alignment);

    struct SFormatResult {
        std::string formatted;
        float       updateEveryMs    = 0; // 0 means don't (static)
        bool        alwaysUpdate     = false;
        bool        cmd              = false;
        bool        allowForceUpdate = false;
    };

    SFormatResult formatString(std::string in);

    class IWidget {
      public:
        virtual ~IWidget()                                                                                                    = default;
        virtual void configure(const std::unordered_map<std::string, std::any>& prop, const SP<Hyprtoolkit::IOutput>& output) = 0;
        virtual void layout(const SP<Hyprtoolkit::IElement>& root, const Vector2D& viewport)                                  = 0;
    };
};
