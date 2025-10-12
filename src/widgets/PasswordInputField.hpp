#pragma once

#include "IWidget.hpp"
#include "../defines.hpp"
#include "../config/ConfigDataValues.hpp"
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/palette/Color.hpp>
#include <hyprutils/math/Misc.hpp>
#include <string>
#include <unordered_map>
#include <any>

struct SPreloadedAsset;
class COutput;

namespace Widgets {
    struct SPasswordInputFieldConfig {
        struct {
            CGradientValueData both;
            CGradientValueData caps;
            CGradientValueData check;
            CGradientValueData fail;
            CGradientValueData outer;
            CGradientValueData num;
            CHyprColor         font;
            CHyprColor         hiddenBase;
            CHyprColor         inner;
            CHyprColor         invertNum;
        } colors;

        struct {
            bool        center     = false;
            float       size       = 0.0;
            float       spacing    = 0;
            int64_t     rounding   = 0;
            std::string textFormat = "";
        } dots;

        bool             fadeOnEmpty = false;
        bool             hiddenInput = false;
        bool             swapFont;
        int64_t          outThick      = 0;
        int64_t          rounding      = 0;
        int64_t          fadeTimeoutMs = 2000;
        CLayoutValueData size;
        CLayoutValueData pos;
        std::string      placeholderText = "";
        std::string      failText        = "";
        std::string      fontFamily      = "";
        std::string      halign          = "";
        std::string      valign          = "";
    };

    class CPasswordInputField : public IWidget {
      public:
        CPasswordInputField()  = default;
        ~CPasswordInputField() = default;

        virtual void configure(const std::unordered_map<std::string, std::any>& props, const SP<Hyprtoolkit::IOutput>& pOutput);
        virtual void layout(const SP<Hyprtoolkit::IElement>& root, const Vector2D& viewport);

        // void         setPasswordLength(size_t len);
        // void         setPlaceholder();
        // void         setWaiting();
        // void         setFail();
        //
        // void         startFadeOutTimer();
        // void         triggerFadeIn();

        void                    updateAccent(const CHyprColor& color);
        std::string_view        getCurrentInput();

        WP<CPasswordInputField> m_self;

      private:
        void                               updateDots();

        SPasswordInputFieldConfig          m_config;

        SP<Hyprtoolkit::CRectangleElement> m_rect;
        SP<Hyprtoolkit::CTextboxElement>   m_textbox;
        // This contains either placeholder or the password indicator (dots)
        // SP<Hyprtoolkit::CRowLayoutElement> m_inputLayout;

        enum : uint8_t {
            PLACEHOLDER,
            TYPING,
            WAITING,
            FAIL,
        } m_inputState          = PLACEHOLDER;
        size_t m_passwordLength = 0;

        // colors
        CHyprColor m_outer;
        CHyprColor m_inner;
        CHyprColor m_font;
    };
}
