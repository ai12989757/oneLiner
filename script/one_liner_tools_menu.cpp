#include "one_liner_tools_menu.h"

#include "one_menu.h"
#include "one_menu_action.h"
#include "oneqt_brush.h"

#include <QFont>
#include <QMargins>
#include <QPointer>
#include <QTimer>

namespace {

constexpr int kInputWidth = 320;
constexpr int kInputMinWidth = 220;
constexpr qreal kItemCornerRadius = 4.0;
constexpr int kMenuItemHeight = 28;
constexpr int kMenuVerticalPadding = 5;
constexpr int kMenuSpacing = 1;
constexpr int kMenuTextSpacing = 10;

QFont menuFont()
{
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(12);
    return font;
}

QString menuIconPath(const QString& kind)
{
    if (kind == QStringLiteral("clearPasted")) {
        return QStringLiteral("oneLinerIcons:Recycle.png");
    }
    return QStringLiteral("oneLinerIcons:help.png");
}

void syncMenuActionStyle(OneQtAction* action, const QFont& font, const QVariant& iconSource = QVariant())
{
    if (action == nullptr) {
        return;
    }

    const OneQtAction::StateName states[] = {
        OneQtAction::Normal,
        OneQtAction::Hover,
        OneQtAction::Pressed,
        OneQtAction::Disabled,
        OneQtAction::Checked,
        OneQtAction::CheckedHover,
        OneQtAction::CheckedPressed,
    };

    for (OneQtAction::StateName state : states) {
        OneQtActionStyle style = action->style(state);
        style.font = font;
        style.padding = QMargins(0, kMenuVerticalPadding, 0, kMenuVerticalPadding);
        style.spacing = kMenuTextSpacing;
        style.iconBrush = OneQtBrush();
        style.cornerRadii = {kItemCornerRadius, kItemCornerRadius, kItemCornerRadius, kItemCornerRadius};
        action->setStyle(state, style);
    }

    if (OneQtMenuAction* menuAction = dynamic_cast<OneQtMenuAction*>(action)) {
        menuAction->setIconSource(iconSource);
        menuAction->setIconCanvasSize(QSize(18, 18));
        menuAction->setIconSize(QSize(18, 18));
        menuAction->syncMenuVisuals();
    }
}

void closeMenuLater(OneQtMenu* menu)
{
    QPointer<OneQtMenu> menuRef = menu;
    QTimer::singleShot(0, [menuRef]() {
        if (menuRef != nullptr) {
            menuRef->close();
        }
    });
}

} // namespace

namespace OneLinerToolsMenu {

OneQtMenu* createMenu(QWidget* parent, const State& state, const Callbacks& callbacks)
{
    OneQtMenu* menu = new OneQtMenu(parent);
    menu->setAttribute(Qt::WA_DeleteOnClose, true);
    menu->setScale(state.scale);
    menu->setMinimumWidth(qMax(kInputMinWidth, state.contentWidth > 0 ? state.contentWidth : kInputWidth));
    menu->setMaximumWidth(qMax(kInputMinWidth, state.contentWidth > 0 ? state.contentWidth : kInputWidth));
    menu->setMargins(QMargins(qRound(3 * state.scale), qRound(3 * state.scale), qRound(3 * state.scale), qRound(3 * state.scale)));
    menu->setSpacing(qRound(kMenuSpacing * state.scale));
    menu->setItemHeight(kMenuItemHeight);
    if (state.background != nullptr) {
        menu->setBackground(*state.background);
    }

    const QFont font = menuFont();

    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("启用预览"), QVariant(), true, false)) {
        syncMenuActionStyle(action, font);
        action->setChecked(state.previewEnabled);
        action->setToolTip(QStringLiteral("开启后显示预览列表，关闭后隐藏预览列表。"));
        action->setStatusTip(QStringLiteral("切换是否显示预览列表。"));
        action->setToggledHandler([menu, callbacks](bool checked) {
            if (callbacks.onPreviewToggled) {
                callbacks.onPreviewToggled(checked);
            }
            closeMenuLater(menu);
        });
    }
    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("自动关闭"), QVariant(), true, false)) {
        syncMenuActionStyle(action, font);
        action->setChecked(state.autoCloseEnabled);
        action->setToolTip(QStringLiteral("开启后窗口失去焦点将自动关闭；关闭后仅隐藏预览列表。"));
        action->setStatusTip(QStringLiteral("切换窗口失去焦点时是否自动关闭。"));
        action->setToggledHandler([menu, callbacks](bool checked) {
            if (callbacks.onAutoCloseToggled) {
                callbacks.onAutoCloseToggled(checked);
            }
            closeMenuLater(menu);
        });
    }
    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("检索Shape对象"), QVariant(), true, false)) {
        syncMenuActionStyle(action, font);
        action->setChecked(state.wildcardIncludeShapes);
        action->setToolTip(QStringLiteral("开启后，通配符检索会包含 shape 对象；关闭时仅检索 transform/joint 等对象。"));
        action->setStatusTip(QStringLiteral("切换通配符检索时是否包含 shape 对象。"));
        action->setToggledHandler([menu, callbacks](bool checked) {
            if (callbacks.onWildcardIncludeShapesToggled) {
                callbacks.onWildcardIncludeShapesToggled(checked);
            }
            closeMenuLater(menu);
        });
    }

    menu->addSeparator();

    if (OneQtAction* action = menu->addAction(QStringLiteral("清除 pasted__ 前缀"))) {
        syncMenuActionStyle(action, font, menuIconPath(QStringLiteral("clearPasted")));
        action->setToolTip(QStringLiteral("移除所有匹配对象名称中的 pasted__ 前缀。"));
        action->setStatusTip(QStringLiteral("执行 pasted__ 前缀清理。"));
        action->setClickedHandler([callbacks]() {
            if (callbacks.onClearPasted) {
                callbacks.onClearPasted();
            }
        });
    }

    menu->addSeparator();

    if (OneQtAction* action = menu->addAction(QStringLiteral("帮助"))) {
        syncMenuActionStyle(action, font, menuIconPath(QStringLiteral("help")));
        action->setToolTip(QStringLiteral("打开 oneLiner 规则帮助。"));
        action->setStatusTip(QStringLiteral("查看 oneLiner 的规则和 flags 说明。"));
        action->setClickedHandler([callbacks]() {
            if (callbacks.onShowHelp) {
                callbacks.onShowHelp();
            }
        });
    }

    return menu;
}

} // namespace OneLinerToolsMenu