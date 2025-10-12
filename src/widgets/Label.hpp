#pragma once

#include "IWidget.hpp"
#include "../defines.hpp"
#include "../config/ConfigDataValues.hpp"
#include <hyprgraphics/resource/resources/TextResource.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Color.hpp>
#include <hyprutils/math/Misc.hpp>
#include <hyprutils/os/Process.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <string>
#include <unordered_map>
#include <any>

class COutput;

namespace Widgets {
    struct SLabelConfig {
        CLayoutValueData                                pos;

        int                                             fontSize = 0;
        double                                          angle    = 0;

        CHyprColor                                      color;
        Hyprgraphics::CTextResource::eTextAlignmentMode textAlign = Hyprgraphics::CTextResource::TEXT_ALIGN_CENTER;

        std::string                                     labelPreFormat = "";
        std::string                                     onClickCommand = "";
        std::string                                     fontFamily     = "";
        std::string                                     halign         = "";
        std::string                                     valign         = "";
    };

    class CLabel : public IWidget {
      public:
        CLabel()  = default;
        ~CLabel() = default;

        virtual void configure(const std::unordered_map<std::string, std::any>& props, const SP<Hyprtoolkit::IOutput>& pOutput);
        virtual void layout(const SP<Hyprtoolkit::IElement>& root, const Vector2D& viewport);

        WP<CLabel>   m_self;

        void         updateText();

      private:
        SLabelConfig             m_config;

        void                     updateTextCmd(const SFormatResult& label);
        void                     plantTimer(const SFormatResult& label);

        ASP<Hyprtoolkit::CTimer> m_labelTimer;

        struct {
            UP<Hyprutils::OS::CProcess> proc;
            std::string                 out;
        } m_asyncCommand;

        std::string                   m_prevText = "";
        SP<Hyprtoolkit::CTextElement> m_text;
    };
}
