#pragma once

#include "one_background.h"

#include <functional>

class OneQtMenu;
class QWidget;

namespace OneLinerToolsMenu {

struct State {
    qreal scale = 1.0;
    int contentWidth = 320;
    const OneQtBackground* background = nullptr;
    bool previewEnabled = true;
    bool autoCloseEnabled = true;
    bool wildcardIncludeShapes = false;
};

struct Callbacks {
    std::function<void(bool)> onPreviewToggled;
    std::function<void(bool)> onAutoCloseToggled;
    std::function<void(bool)> onWildcardIncludeShapesToggled;
    std::function<void()> onClearPasted;
    std::function<void()> onShowHelp;
};

OneQtMenu* createMenu(QWidget* parent, const State& state, const Callbacks& callbacks);

} // namespace OneLinerToolsMenu