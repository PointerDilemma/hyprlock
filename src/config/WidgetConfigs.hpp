#include <hyprlang.hpp>
#include <hyprtoolkit/palette/Color.hpp>
#include <unordered_map>

struct SWidgetConfig {
    std::string type;
    std::string monitor;
    //
    int id;
};

struct SBackgroundConfig : public SWidgetConfig {
    Hyprlang::INT           blurPasses = 0;
    Hyprlang::INT           blurSize   = 0;
    Hyprlang::INT           brightness = 0;
    Hyprtoolkit::CHyprColor color;
    Hyprtoolkit::CHyprColor contrast;
    Hyprlang::FLOAT         noise = 0;
    std::string             path;
    std::string             reloadCommand;
    Hyprlang::INT           reloadTime        = -1;
    Hyprlang::FLOAT         vibrancy          = 0.0;
    Hyprlang::FLOAT         vibrancy_darkness = 0.0;
};
