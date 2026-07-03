#include "one_liner_tools_menu.h"

#include "one_menu.h"
#include "one_menu_action.h"
#include "oneqt_brush.h"

#include <QMargins>
#include <QPointer>
#include <QTimer>

namespace {

constexpr int kInputWidth = 320;
constexpr int kInputMinWidth = 220;
constexpr qreal kItemCornerRadius = 4.0;
constexpr int kMenuItemHeight = 28;
constexpr int kMenuVerticalPadding = 5;
constexpr int kMenuHorizontalPadding = 10;
constexpr int kMenuSpacing = 1;

QString menuIconPath(const QString& kind)
{
    if (kind == QStringLiteral("clearPasted")) {
        return QStringLiteral("oneLinerIcons:Recycle.png");
    }
    return QStringLiteral("oneLinerIcons:help.png");
}

void syncMenuActionStyle(OneQtMenuAction* action, const QVariant& iconSource = QVariant())
{
    if (action == nullptr) {
        return;
    }

    // 新架构：菜单项默认已跟随全局主题（components.menu.item.*）。
    // 这里只覆盖 oneLiner 想要的紧凑内边距与圆角，图标直接通过组件内容 API 设置。
    action->setPadding(QMargins(kMenuHorizontalPadding, kMenuVerticalPadding, kMenuHorizontalPadding, kMenuVerticalPadding));
    action->setCornerRadius({kItemCornerRadius, kItemCornerRadius, kItemCornerRadius, kItemCornerRadius});

    if (iconSource.isValid()) {
        action->setIconSource(iconSource);
        action->setIconCanvasSize(QSize(18, 18));
        action->setIconSize(QSize(18, 18));
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

    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("启用预览"), QVariant(), true, false)) {
        syncMenuActionStyle(action);
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
        syncMenuActionStyle(action);
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
        syncMenuActionStyle(action);
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

    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("清除 pasted__ 前缀"))) {
        syncMenuActionStyle(action, menuIconPath(QStringLiteral("clearPasted")));
        action->setToolTip(QStringLiteral("移除所有匹配对象名称中的 pasted__ 前缀。"));
        action->setStatusTip(QStringLiteral("执行 pasted__ 前缀清理。"));
        action->setClickedHandler([callbacks]() {
            if (callbacks.onClearPasted) {
                callbacks.onClearPasted();
            }
        });
    }

    menu->addSeparator();

    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("帮助"))) {
        syncMenuActionStyle(action, menuIconPath(QStringLiteral("help")));
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