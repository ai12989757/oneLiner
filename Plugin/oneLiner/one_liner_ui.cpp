#include "one_liner_ui.h"

#include "one_list.h"
#include "one_menu.h"
#include "one_menu_action.h"
#include "oneqt_brush.h"
#include "oneqt_system.h"

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <QApplication>
#include <QCursor>
#include <QCloseEvent>
#include <QDialog>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPixmap>
#include <QScreen>
#include <QScrollBar>
#include <QSizePolicy>
#include <QVariantMap>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <QWheelEvent>
#include <QSvgRenderer>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <imm.h>
#endif

namespace {

constexpr const char* kPreviewEnabledOptionVar = "oneLinerPreviewEnabled";
constexpr const char* kAutoCloseEnabledOptionVar = "oneLinerAutoCloseEnabled";
constexpr const char* kWildcardIncludeShapesOptionVar = "oneLinerWildcardIncludeShapes";

constexpr int kInputWidth = 320;
constexpr int kInputHeight = 26;
constexpr int kInputFontSize = 12;
constexpr int kPreviewItemHeight = 22;
constexpr int kPreviewItemSpacing = 2;
constexpr int kPreviewGap = 4;
constexpr int kPreviewListOuterMargin = 10;
constexpr int kContentLeftPadding = 10;
constexpr int kPreviewMaxVisibleItems = 10;
constexpr qreal kPanelCornerRadius = 6.0;
constexpr qreal kItemCornerRadius = 4.0;
constexpr int kMenuItemHeight = 28;
constexpr int kMenuHorizontalPadding = 0;
constexpr int kMenuVerticalPadding = 5;
constexpr int kMenuSpacing = 1;
constexpr int kMenuTextSpacing = 10;
constexpr int kInputHistoryLimit = 50;
constexpr QChar kPreviewHierarchyMarker(0x2022);
constexpr int kImeStatusPollIntervalMs = 120;
constexpr int kImeIconSize = 16;
constexpr int kImeIconSpacing = 4;
constexpr int kImeStatusRightPadding = 6;
#ifdef _WIN32
constexpr WPARAM kImeGetConversionMode = 0x0001;
constexpr WPARAM kImeGetOpenStatus = 0x0005;
#endif

constexpr char kChineseSvg[] = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48"><g fill="none" stroke="#ffffff" stroke-linecap="round" stroke-width="4"><rect width="36" height="36" x="6" y="6" stroke-linejoin="round" rx="3" /><path stroke-linejoin="round" d="M14 18h20v10H14z" /><path d="M24 14v21" /></g></svg>)SVG";
constexpr char kEnglishSvg[] = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48"><g fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round" stroke-width="4"><rect width="36" height="36" x="6" y="6" rx="3" /><path d="M13 31V17h8m-8 7h7.5M13 31h7.5m5.5 0V19m0 12v-6.5a4.5 4.5 0 0 1 4.5-4.5v0a4.5 4.5 0 0 1 4.5 4.5V31" /></g></svg>)SVG";
constexpr char kCapsLockSvg[] = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20"><path fill="#ffffff" d="M11.144 2.53a1.5 1.5 0 0 0-2.288 0l-6.617 7.803a1 1 0 0 0 .763 1.647H6v3.017a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V11.98h2.996a1 1 0 0 0 .763-1.647zM6.5 17a.5.5 0 0 0 0 1h7a.5.5 0 0 0 0-1z" /></svg>)SVG";

QPointer<OneLinerWindow> g_window;
QStringList g_inputHistory;

QPixmap renderWhiteSvgIcon(const QByteArray& svgData, const QSize& size)
{
    if (size.isEmpty()) {
        return QPixmap();
    }

    QSvgRenderer renderer(svgData);
    if (!renderer.isValid()) {
        return QPixmap();
    }

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    renderer.render(&painter);
    return pixmap;
}

#ifdef _WIN32
HWND focusedInputWindow()
{
    const HWND foreground = ::GetForegroundWindow();
    if (foreground == nullptr) {
        return nullptr;
    }

    const DWORD threadId = ::GetWindowThreadProcessId(foreground, nullptr);
    if (threadId == 0) {
        return foreground;
    }

    GUITHREADINFO threadInfo = {};
    threadInfo.cbSize = sizeof(GUITHREADINFO);
    if (::GetGUIThreadInfo(threadId, &threadInfo)) {
        return threadInfo.hwndFocus != nullptr ? threadInfo.hwndFocus : foreground;
    }

    return foreground;
}

int queryImeControl(HWND hwnd, WPARAM command)
{
    if (hwnd == nullptr) {
        return -1;
    }

    const HWND imeWindow = ::ImmGetDefaultIMEWnd(hwnd);
    if (imeWindow == nullptr) {
        return -1;
    }

    DWORD_PTR result = 0;
    const LRESULT ok = ::SendMessageTimeoutW(
        imeWindow,
        WM_IME_CONTROL,
        command,
        0,
        SMTO_NORMAL,
        100,
        &result);
    if (ok == 0) {
        return -1;
    }

    return static_cast<int>(result);
}
#endif

QWidget* mayaMainWindow()
{
    return reinterpret_cast<QWidget*>(MQtUtil::mainWindow());
}

MString toMString(const QString& value)
{
    return MString(value.toUtf8().constData());
}

qreal mayaDpiScale(QWidget* widget)
{
    QWidget* host = widget != nullptr ? widget : mayaMainWindow();
    QScreen* screen = nullptr;
    if (host != nullptr && host->windowHandle() != nullptr) {
        screen = host->windowHandle()->screen();
    }
    if (screen == nullptr && host != nullptr) {
        screen = host->screen();
    }
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }

    const qreal screenScale = OneQtSystemInfo::screenDpiScale(screen);
    const qreal deviceScale = host != nullptr ? host->devicePixelRatioF() : 1.0;
    return qMax<qreal>(1.0, qMax(screenScale, deviceScale));
}

QString helpText()
{
    return QStringLiteral(
        "<h3>oneLiner 规则</h3>"
        "<p>直接输入新名称即可实时预览并回车执行。</p>"
        "<ul>"
        "<li><b>!</b> 复用旧名称，例如 side_!</li>"
        "<li><b>#</b> 数字序号，例如 ctrl_##//3</li>"
        "<li><b>@</b> 字母序号，例如 joint_@</li>"
        "<li><b>old&gt;new</b> 文本替换</li>"
        "<li><b>+3</b> 从前往后删 3 个字符</li>"
        "<li><b>-3</b> 从后往前删 3 个字符</li>"
        "<li><b>--3</b> 保留前 3 个字符</li>"
        "<li><b>* ?</b> 使用 Maya 通配符选择对象，回车直接选中</li>"
        "<li><b>-h</b> 将当前选中子级加入待重命名列表</li>"
        "<li><b>-h -s</b> 将当前选中子级和 shape 加入待重命名列表</li>"
        "<li><b>-type joint blendShape</b> 按类型过滤待重命名列表；无候选时等同 ls -type</li>"
        "</ul>");
}

QString quoteMelString(const QString& value)
{
    QString escaped = value;
    escaped.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    escaped.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

QFont baseUiFont()
{
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(kInputFontSize);
    return font;
}

QString menuIconPath(const QString& kind)
{
    if (kind == QStringLiteral("prefix")) {
        return QStringLiteral("oneLinerIcons:Prefix.png");
    }
    if (kind == QStringLiteral("suffix")) {
        return QStringLiteral("oneLinerIcons:Suffix.png");
    }
    if (kind == QStringLiteral("contains")) {
        return QStringLiteral("oneLinerIcons:Contain.png");
    }
    if (kind == QStringLiteral("clearPasted")) {
        return QStringLiteral("oneLinerIcons:Recycle.png");
    }
    return QStringLiteral("oneLinerIcons:help.png");
}

bool readBoolOptionVar(const char* optionVarName, bool defaultValue)
{
    int exists = 0;
    if (MGlobal::executeCommand(MString("optionVar -exists \"") + optionVarName + "\"", exists, false, false) != MS::kSuccess || exists == 0) {
        return defaultValue;
    }

    int value = defaultValue ? 1 : 0;
    if (MGlobal::executeCommand(MString("optionVar -q \"") + optionVarName + "\"", value, false, false) != MS::kSuccess) {
        return defaultValue;
    }

    return value != 0;
}

void writeBoolOptionVar(const char* optionVarName, bool value)
{
    const QString command = QStringLiteral("optionVar -iv \"%1\" %2")
        .arg(QString::fromUtf8(optionVarName))
        .arg(value ? 1 : 0);
    MGlobal::executeCommand(toMString(command), false, false);
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

} // namespace

OneLinerWindow::OneLinerWindow(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool)
    , _scale(1.0)
    , _maxPreviewCount(19)
    , _previewEnabled(readBoolOptionVar(kPreviewEnabledOptionVar, true))
    , _autoCloseEnabled(readBoolOptionVar(kAutoCloseEnabledOptionVar, true))
    , _wildcardIncludeShapes(readBoolOptionVar(kWildcardIncludeShapesOptionVar, false))
    , _previewScrollEnabled(false)
    , _closeAfterMenuAction(false)
    , _restoreFocusAfterMenu(false)
    , _isClosing(false)
    , _previewTopInset(0)
    , _historyIndex(-1)
    , _imeIsChinese(true)
    , _capsLockOn(false)
    , _dragging(false)
    , _rootLayout(new QVBoxLayout(this))
    , _lineEdit(new QLineEdit(this))
    , _imeStatusHost(new QWidget(_lineEdit))
    , _imeLanguageLabel(new QLabel(_imeStatusHost))
    , _imeCapsLabel(new QLabel(_imeStatusHost))
    , _imeStatusTimer(new QTimer(this))
    , _previewList(new OneQtList(this, false))
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    _inputBackground.setBrush(OneQtBrush::fromColor(QColor(18, 22, 29, 184)));
    _inputBackground.setBorderBrush(OneQtBrush::fromColor(QColor(82, 133, 166, 210)));
    _inputBackground.setBorderWidth(1.0);
    _inputBackground.setCornerRadius({kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius});

    _previewBackground.setBrush(OneQtBrush::fromColor(QColor(18, 22, 29, 176)));
    _previewBackground.setBorderBrush(OneQtBrush::fromColor(QColor(82, 133, 166, 188)));
    _previewBackground.setBorderWidth(1.0);
    _previewBackground.setCornerRadius({kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius});

    _lineEdit->setFrame(false);
    _lineEdit->setAttribute(Qt::WA_TranslucentBackground, true);
    _lineEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    _lineEdit->setPlaceholderText(QStringLiteral("请输入重命名规则，右击查看帮助"));
    _lineEdit->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        " background: transparent;"
        " border: none;"
        " color: #f4f6f8;"
        " selection-background-color: #5285a6;"
        " selection-color: #ffffff;"
        "}"));
    _lineEdit->setToolTip(QStringLiteral("输入重命名规则；支持 Maya 通配符 * ?，以及 -h、-h -s、-type 等 flags。"));
    _lineEdit->setStatusTip(QStringLiteral("输入规则后实时预览，回车执行；上下方向键可切换本次会话的输入历史。"));

    _imeStatusHost->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    _imeStatusHost->setAutoFillBackground(false);
    _imeLanguageLabel->setAlignment(Qt::AlignCenter);
    _imeCapsLabel->setAlignment(Qt::AlignCenter);
    _imeCapsLabel->hide();

    _rootLayout->setContentsMargins(0, 0, 0, 0);
    _rootLayout->setSpacing(0);

    _lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _previewList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    _previewList->setFocusPolicy(Qt::NoFocus);
    _previewList->setSelectionMode(QAbstractItemView::NoSelection);
    _previewList->setPanelBackground(_previewBackground);

    OneQtBackground previewItemBackground;
    previewItemBackground.setBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    previewItemBackground.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    previewItemBackground.setCornerRadius({kItemCornerRadius, kItemCornerRadius, kItemCornerRadius, kItemCornerRadius});
    _previewList->setItemBackground(previewItemBackground);

    OneQtBackground previewHoverBackground;
    previewHoverBackground.setBrush(OneQtBrush::fromColor(QColor(82, 133, 166, 42)));
    previewHoverBackground.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    previewHoverBackground.setCornerRadius({kItemCornerRadius, kItemCornerRadius, kItemCornerRadius, kItemCornerRadius});
    _previewList->setItemHoverBackground(previewHoverBackground);
    _previewList->setItemSelectedBackground(previewHoverBackground);
    _previewList->setTextBrush(OneQtBrush::fromColor(QColor(QStringLiteral("#d7dde3"))));
    _previewList->setSelectedTextBrush(OneQtBrush::fromColor(QColor(QStringLiteral("#f4f6f8"))));
    _previewList->setWheelScrollMode(OneQtList::WheelScrollByItemCount);
    _previewList->setWheelItemsPerStep(3);
    _previewList->setToolTip(QStringLiteral("预览列表。双击任意项可将其文本写回输入框。"));
    _previewList->setStatusTip(QStringLiteral("双击预览项可快速把该文本填入输入框。"));

    _lineEdit->installEventFilter(this);
    _previewList->installEventFilter(this);
    if (QListView* previewView = _previewList->findChild<QListView*>()) {
        previewView->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        previewView->installEventFilter(this);
        connect(previewView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
            if (!index.isValid()) {
                return;
            }

            const QVariant itemData = _previewList->itemData(index.row());
            const QVariantMap previewData = itemData.toMap();
            const QString rawText = previewData.value(QStringLiteral("rawText")).toString();
            _lineEdit->setText(rawText.isEmpty() ? _previewList->itemText(index.row()) : rawText);
            _lineEdit->setFocus();
            _lineEdit->selectAll();
        });
    }

    connect(_lineEdit, &QLineEdit::textChanged, this, [this]() { refreshPreview(); });
    connect(_lineEdit, &QLineEdit::textEdited, this, [this]() {
        _historyIndex = -1;
        _historyDraft.clear();
    });
    connect(_lineEdit, &QLineEdit::returnPressed, this, [this]() { executeRule(); });
    connect(_lineEdit, &QWidget::customContextMenuRequested, this, [this]() { showToolsMenu(); });
    connect(_imeStatusTimer, &QTimer::timeout, this, [this]() { refreshImeStatus(); });

    updateScaleFromMaya();
    refreshImeStatus();
    _imeStatusTimer->start(kImeStatusPollIntervalMs);
    positionNearCursor();
    _previewList->hide();
    _lineEdit->setFocus();
}

OneLinerWindow::~OneLinerWindow() = default;

void OneLinerWindow::showWindow(QWidget* mayaParent)
{
    QWidget* requestedParent = mayaParent;
    QTimer::singleShot(0, [requestedParent]() {
        QWidget* parent = mayaMainWindow();
        if (g_window != nullptr) {
            g_window->close();
            g_window->deleteLater();
            g_window = nullptr;
        }

        g_window = new OneLinerWindow(parent != nullptr ? parent : requestedParent);
        g_window->show();
        g_window->raise();
        g_window->activateWindow();

        QPointer<OneLinerWindow> window = g_window;
        QTimer::singleShot(0, [window]() {
            if (window == nullptr) {
                return;
            }

            window->refreshPreview();
            window->_lineEdit->setFocus();
        });
    });
}

void OneLinerWindow::closeWindow(bool immediate)
{
    if (g_window != nullptr) {
        OneLinerWindow* window = g_window;
        g_window = nullptr;
        window->_isClosing = true;
        if (window->_toolsMenu != nullptr) {
            window->_toolsMenu->close();
            window->_toolsMenu = nullptr;
        }
        window->close();
        if (immediate) {
            delete window;
        } else {
            window->deleteLater();
        }
    }
}

void OneLinerWindow::closeEvent(QCloseEvent* event)
{
    _isClosing = true;
    if (_toolsMenu != nullptr) {
        _toolsMenu->close();
        _toolsMenu = nullptr;
    }
    QWidget::closeEvent(event);
}

bool OneLinerWindow::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate) {
        if (_toolsMenu != nullptr && _toolsMenu->isVisible()) {
            return QWidget::event(event);
        }
        if (!_autoCloseEnabled) {
            _previewList->hide();
            _previewScrollEnabled = false;
            applyWindowLayout(0);
            return QWidget::event(event);
        }
        close();
        return true;
    }
    if (event->type() == QEvent::WindowActivate) {
        if (!_autoCloseEnabled && _previewEnabled && _toolsMenu == nullptr) {
            QPointer<OneLinerWindow> window = this;
            QTimer::singleShot(0, [window]() {
                if (window != nullptr && window->isVisible()) {
                    window->refreshPreview();
                }
            });
        }
    }
    return QWidget::event(event);
}

bool OneLinerWindow::eventFilter(QObject* watched, QEvent* event)
{
    const QListView* previewView = _previewList != nullptr ? _previewList->findChild<QListView*>() : nullptr;
    if ((watched == _lineEdit || watched == _previewList || watched == previewView) && event != nullptr) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (watched == _lineEdit && keyEvent->key() == Qt::Key_Up) {
                return stepHistory(-1);
            }
            if (watched == _lineEdit && keyEvent->key() == Qt::Key_Down) {
                return stepHistory(1);
            }
            if (keyEvent->key() == Qt::Key_Escape) {
                close();
                return true;
            }
        } else if (watched == previewView && event->type() == QEvent::Wheel && !_previewScrollEnabled) {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            wheelEvent->accept();
            return true;
        } else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::MiddleButton) {
                _dragging = true;
                _dragStart = mouseEvent->globalPos();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && _dragging) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint current = mouseEvent->globalPos();
            move(pos() + (current - _dragStart));
            _dragStart = current;
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::MiddleButton) {
                _dragging = false;
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OneLinerWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawPixmap(_lineEdit->geometry().topLeft(), _inputBackground.toPixmap(_lineEdit->size()));
}

void OneLinerWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    const int previewHeight = _previewList->isVisible() ? _previewList->height() : 0;
    applyWindowLayout(previewHeight);
    update();
}

void OneLinerWindow::refreshPreview()
{
    if (!_previewEnabled) {
        rebuildPreviewList(QStringList(), QStringList(), false);
        return;
    }

    const QString text = _lineEdit->text();
    const OneLinerEngine::PreviewResult result = OneLinerEngine::preview(text);
    rebuildPreviewList(result.items, result.rawItems, result.selectionOnly);
}

void OneLinerWindow::executeRule()
{
    const QString text = _lineEdit->text();
    if (text.trimmed().isEmpty()) {
        return;
    }

    rememberHistory(text);
    const QString command = QStringLiteral("oneLiner -execute -rule %1").arg(quoteMelString(text));
    MGlobal::executeCommand(toMString(command), false, true);
    _lineEdit->clear();
    refreshPreview();
}

void OneLinerWindow::rememberHistory(const QString& text)
{
    const QString value = text.trimmed().isEmpty() ? QString() : text;
    if (value.isEmpty()) {
        return;
    }

    g_inputHistory.removeAll(value);
    g_inputHistory.push_back(value);
    while (g_inputHistory.size() > kInputHistoryLimit) {
        g_inputHistory.removeFirst();
    }

    _historyIndex = -1;
    _historyDraft.clear();
}

bool OneLinerWindow::stepHistory(int direction)
{
    if (g_inputHistory.isEmpty()) {
        return false;
    }

    if (_historyIndex < 0 || _historyIndex > g_inputHistory.size()) {
        _historyIndex = g_inputHistory.size();
        _historyDraft = _lineEdit->text();
    }

    if (_historyIndex == g_inputHistory.size()) {
        _historyDraft = _lineEdit->text();
    }

    if (direction < 0) {
        if (_historyIndex > 0) {
            --_historyIndex;
        }
    } else if (direction > 0) {
        if (_historyIndex < g_inputHistory.size() - 1) {
            ++_historyIndex;
        } else {
            _historyIndex = g_inputHistory.size();
        }
    }

    const QString nextText = _historyIndex == g_inputHistory.size()
        ? _historyDraft
        : g_inputHistory.value(_historyIndex);
    _lineEdit->setText(nextText);
    _lineEdit->setCursorPosition(nextText.size());
    return true;
}

void OneLinerWindow::showToolsMenu()
{
    if (_toolsMenu != nullptr) {
        _toolsMenu->close();
        _toolsMenu = nullptr;
    }

    OneQtMenu* menu = new OneQtMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose, true);
    menu->setScale(_scale);
    menu->setMinimumWidth(kInputWidth);
    menu->setMaximumWidth(kInputWidth);
    menu->setMargins(QMargins(qRound(3 * _scale), qRound(3 * _scale), qRound(3 * _scale), qRound(3 * _scale)));
    menu->setSpacing(qRound(kMenuSpacing * _scale));
    menu->setItemHeight(kMenuItemHeight);
    menu->setBackground(_inputBackground);
    _toolsMenu = menu;
    _closeAfterMenuAction = false;
    _restoreFocusAfterMenu = false;

    connect(menu, &QObject::destroyed, this, [this]() {
        _toolsMenu = nullptr;
        if (_isClosing) {
            _closeAfterMenuAction = false;
            _restoreFocusAfterMenu = false;
            return;
        }
        if (_closeAfterMenuAction) {
            _closeAfterMenuAction = false;
            close();
            return;
        }

        if (_restoreFocusAfterMenu) {
            _restoreFocusAfterMenu = false;
            if (isVisible()) {
                raise();
                activateWindow();
                _lineEdit->setFocus();
            }
            return;
        }

        if (_autoCloseEnabled && !isActiveWindow()) {
            close();
            return;
        }

        if (isVisible()) {
            raise();
            activateWindow();
            _lineEdit->setFocus();
        }
    });

    const QFont menuFont = baseUiFont();

    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("启用预览"), QVariant(), true, false)) {
        syncMenuActionStyle(action, menuFont);
        action->setChecked(_previewEnabled);
        action->setToolTip(QStringLiteral("开启后显示预览列表，关闭后隐藏预览列表。"));
        action->setStatusTip(QStringLiteral("切换是否显示预览列表。"));
        action->setToggledHandler([this](bool checked) {
            _closeAfterMenuAction = false;
            _restoreFocusAfterMenu = true;
            _previewEnabled = checked;
            writeBoolOptionVar(kPreviewEnabledOptionVar, _previewEnabled);
            refreshPreview();
            QPointer<OneQtMenu> menuRef = _toolsMenu;
            QTimer::singleShot(0, [menuRef]() {
                if (menuRef != nullptr) {
                    menuRef->close();
                }
            });
        });
    }
    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("自动关闭"), QVariant(), true, false)) {
        syncMenuActionStyle(action, menuFont);
        action->setChecked(_autoCloseEnabled);
        action->setToolTip(QStringLiteral("开启后窗口失去焦点将自动关闭；关闭后仅隐藏预览列表。"));
        action->setStatusTip(QStringLiteral("切换窗口失去焦点时是否自动关闭。"));
        action->setToggledHandler([this](bool checked) {
            _closeAfterMenuAction = false;
            _restoreFocusAfterMenu = true;
            _autoCloseEnabled = checked;
            writeBoolOptionVar(kAutoCloseEnabledOptionVar, _autoCloseEnabled);
            QPointer<OneQtMenu> menuRef = _toolsMenu;
            QTimer::singleShot(0, [menuRef]() {
                if (menuRef != nullptr) {
                    menuRef->close();
                }
            });
        });
    }
    if (OneQtMenuAction* action = menu->addAction(QStringLiteral("检索Shape对象"), QVariant(), true, false)) {
        syncMenuActionStyle(action, menuFont);
        action->setChecked(_wildcardIncludeShapes);
        action->setToolTip(QStringLiteral("开启后，通配符检索会包含 shape 对象；关闭时仅检索 transform/joint 等对象。"));
        action->setStatusTip(QStringLiteral("切换通配符检索时是否包含 shape 对象。"));
        action->setToggledHandler([this](bool checked) {
            _closeAfterMenuAction = false;
            _restoreFocusAfterMenu = true;
            _wildcardIncludeShapes = checked;
            writeBoolOptionVar(kWildcardIncludeShapesOptionVar, _wildcardIncludeShapes);
            refreshPreview();
            QPointer<OneQtMenu> menuRef = _toolsMenu;
            QTimer::singleShot(0, [menuRef]() {
                if (menuRef != nullptr) {
                    menuRef->close();
                }
            });
        });
    }
    menu->addSeparator();
    if (OneQtAction* action = menu->addAction(QStringLiteral("清除 pasted__ 前缀"))) {
        syncMenuActionStyle(action, menuFont, menuIconPath(QStringLiteral("clearPasted")));
        action->setToolTip(QStringLiteral("移除所有匹配对象名称中的 pasted__ 前缀。"));
        action->setStatusTip(QStringLiteral("执行 pasted__ 前缀清理。"));
        action->setClickedHandler([this]() {
            _closeAfterMenuAction = true;
            _restoreFocusAfterMenu = false;
            MGlobal::executeCommand(toMString(QStringLiteral("oneLiner -clearPasted")), false, true);
            refreshPreview();
        });
    }
    menu->addSeparator();
    if (OneQtAction* action = menu->addAction(QStringLiteral("帮助"))) {
        syncMenuActionStyle(action, menuFont, menuIconPath(QStringLiteral("help")));
        action->setToolTip(QStringLiteral("打开 oneLiner 规则帮助。"));
        action->setStatusTip(QStringLiteral("查看 oneLiner 的规则和 flags 说明。"));
        action->setClickedHandler([this]() {
            _closeAfterMenuAction = true;
            _restoreFocusAfterMenu = false;
            showHelpDialog();
        });
    }

    menu->popup(_lineEdit->mapToGlobal(QPoint(0, _lineEdit->height() + qRound(kPreviewGap * _scale))));
}

void OneLinerWindow::showHelpDialog()
{
    if (_helpDialog != nullptr) {
        _helpDialog->raise();
        _helpDialog->activateWindow();
        return;
    }

    QDialog* dialog = new QDialog(this, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setWindowTitle(QStringLiteral("oneLiner 帮助"));
    dialog->resize(qRound(520 * _scale), qRound(420 * _scale));

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(qRound(16 * _scale), qRound(16 * _scale), qRound(16 * _scale), qRound(16 * _scale));

    QTextBrowser* browser = new QTextBrowser(dialog);
    browser->setOpenExternalLinks(true);
    browser->setHtml(helpText());
    layout->addWidget(browser);

    _helpDialog = dialog;
    dialog->show();
}

void OneLinerWindow::positionNearCursor()
{
    const QPoint cursor = QCursor::pos();
    move(cursor.x() - qRound(40 * _scale), cursor.y() - qRound(15 * _scale));
}

void OneLinerWindow::updateScaleFromMaya()
{
    _scale = mayaDpiScale(parentWidget());
    _inputBackground.setScale(_scale);
    _previewBackground.setScale(_scale);
    updateLayoutMetrics();
}

void OneLinerWindow::updateLayoutMetrics()
{
    const int widthValue = qRound(kInputWidth * _scale);
    const int lineHeight = qRound(kInputHeight * _scale);
    const int fontSize = qRound(kInputFontSize * _scale);
    const int imeIconSize = qRound(kImeIconSize * _scale);
    const int imeSpacing = qRound(kImeIconSpacing * _scale);
    const int imeRightPadding = qRound(kImeStatusRightPadding * _scale);
    const int imeHostWidth = imeIconSize * 2 + imeSpacing;

    QFont editFont = baseUiFont();
    editFont.setPixelSize(fontSize);
    _lineEdit->setFont(editFont);
    _lineEdit->setFixedHeight(lineHeight);
    _lineEdit->setFixedWidth(widthValue);
    _lineEdit->setTextMargins(qRound(kContentLeftPadding * _scale), 0, imeHostWidth + imeRightPadding + qRound(4 * _scale), 0);

    _imeStatusHost->setGeometry(widthValue - imeHostWidth - imeRightPadding, 0, imeHostWidth, lineHeight);
    _imeCapsLabel->setGeometry(0, (lineHeight - imeIconSize) / 2, imeIconSize, imeIconSize);
    _imeLanguageLabel->setGeometry(imeIconSize + imeSpacing, (lineHeight - imeIconSize) / 2, imeIconSize, imeIconSize);
    _imeLanguageLabel->setPixmap(renderWhiteSvgIcon(QByteArray(_imeIsChinese ? kChineseSvg : kEnglishSvg), QSize(imeIconSize, imeIconSize)));
    _imeCapsLabel->setPixmap(renderWhiteSvgIcon(QByteArray(kCapsLockSvg), QSize(imeIconSize, imeIconSize)));
    _imeCapsLabel->setVisible(_capsLockOn);

    QFont listFont = baseUiFont();
    int itemLeftPadding = qRound(kContentLeftPadding * _scale);
    int itemRightPadding = qRound(kContentLeftPadding * _scale);
    if (QLayout* previewLayout = _previewList->layout()) {
        const QMargins margins = previewLayout->contentsMargins();
        const int itemPaddingTrim = qMax(1, qRound(2 * _scale));
        itemLeftPadding = qMax(0, itemLeftPadding - margins.left() - itemPaddingTrim + qMax(1, qRound(_scale)));
        itemRightPadding = qMax(0, itemRightPadding - margins.right() - itemPaddingTrim);
    }
    _previewList->setScale(_scale);
    _previewList->setFixedWidth(widthValue);
    _previewList->setItemFont(listFont);
    _previewList->setItemHeight(kPreviewItemHeight);
    _previewList->setItemSpacing(kPreviewItemSpacing);
    _previewList->setItemPadding(QMargins(itemLeftPadding, 0, itemRightPadding, 0));

    applyWindowLayout(_previewList->isVisible() ? _previewList->height() : 0);
}

void OneLinerWindow::refreshImeStatus()
{
#ifdef _WIN32
    const HWND focusedWindow = focusedInputWindow();
    const bool capsLockOn = (::GetKeyState(VK_CAPITAL) & 0x0001) != 0;

    bool isChinese = _imeIsChinese;
    const int imeOpen = queryImeControl(focusedWindow, kImeGetOpenStatus);
    if (imeOpen == 0) {
        isChinese = false;
    } else if (imeOpen > 0) {
        const int conversionMode = queryImeControl(focusedWindow, kImeGetConversionMode);
        if (conversionMode >= 0) {
            isChinese = (conversionMode & 0x0001) != 0;
        }
    }

    if (_imeIsChinese == isChinese && _capsLockOn == capsLockOn) {
        return;
    }

    _imeIsChinese = isChinese;
    _capsLockOn = capsLockOn;
    updateLayoutMetrics();
#else
    if (_imeStatusHost != nullptr) {
        _imeStatusHost->hide();
    }
#endif
}

void OneLinerWindow::applyWindowLayout(int previewHeight)
{
    const int widthValue = qRound(kInputWidth * _scale);
    const int lineHeight = qRound(kInputHeight * _scale);
    const int gap = qRound(kPreviewGap * _scale);
    const int resolvedPreviewHeight = qMax(0, previewHeight);

    const QSize currentSize = size();
    const bool resizing = currentSize.width() != widthValue || currentSize.height() != lineHeight + (resolvedPreviewHeight > 0 ? gap + resolvedPreviewHeight : 0);

    _lineEdit->setGeometry(0, 0, widthValue, lineHeight);

    if (resolvedPreviewHeight > 0 && _previewList->isVisible()) {
        _previewList->setGeometry(0, lineHeight + gap, widthValue, resolvedPreviewHeight);
    } else {
        _previewList->setGeometry(0, lineHeight + gap, widthValue, 0);
    }

    if (resizing) {
        resize(widthValue, lineHeight + (resolvedPreviewHeight > 0 ? gap + resolvedPreviewHeight : 0));
    }
}

void OneLinerWindow::rebuildPreviewList(const QStringList& items, const QStringList& rawItems, bool selectionOnly)
{
    const int itemHeight = qMax(24, qRound(kPreviewItemHeight * _scale));
    const int itemSpacing = qRound(kPreviewItemSpacing * _scale);

    _previewList->clearItems();
    for (int index = 0; index < items.size(); ++index) {
        QVariantMap previewData;
        const QString rawText = rawItems.value(index, items.at(index));
        previewData.insert(QStringLiteral("rawText"), rawText);
        const QString& displayText = items.at(index);
        previewData.insert(QStringLiteral("prefixLength"), qMax(0, displayText.size() - rawText.size()));
        _previewList->addItem(displayText, previewData);
    }

    if (items.isEmpty()) {
        _previewScrollEnabled = false;
        _previewTopInset = 0;
        _previewList->hide();
        _previewList->setMinimumHeight(0);
        _previewList->setMaximumHeight(QWIDGETSIZE_MAX);
        applyWindowLayout(0);
        _lineEdit->setPlaceholderText(selectionOnly
            ? QStringLiteral("未找到匹配对象")
            : QStringLiteral("请选择对象，或输入 Maya 通配符 / -h / -type"));
    } else {
        const int visibleItemCount = qMin(items.size(), kPreviewMaxVisibleItems);
        int actualItemHeight = itemHeight;
        int actualItemSpacing = itemSpacing;
        int previewTopMargin = qRound(kPreviewListOuterMargin * _scale);
        int previewBottomMargin = qRound(kPreviewListOuterMargin * _scale);
        if (QLayout* previewLayout = _previewList->layout()) {
            const QMargins margins = previewLayout->contentsMargins();
            previewTopMargin = margins.top();
            previewBottomMargin = margins.bottom();
        }

        if (QListView* listView = _previewList->findChild<QListView*>()) {
            actualItemHeight = qMax(actualItemHeight, listView->sizeHintForRow(0));
            actualItemSpacing = qMax(actualItemSpacing, listView->spacing());
        }

        int previewHeight = visibleItemCount * actualItemHeight
            + qMax(0, visibleItemCount - 1) * actualItemSpacing
            + previewTopMargin
            + previewBottomMargin;

        _previewScrollEnabled = items.size() > visibleItemCount;

        if (QListView* listView = _previewList->findChild<QListView*>()) {
            listView->setVerticalScrollBarPolicy(_previewScrollEnabled ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
            if (QScrollBar* scrollBar = listView->verticalScrollBar()) {
                scrollBar->setValue(scrollBar->minimum());
            }
        }

        _previewList->setMinimumHeight(previewHeight);
        _previewList->setMaximumHeight(previewHeight);
        _previewList->setFixedHeight(previewHeight);
        _previewList->show();

        if (QListView* listView = _previewList->findChild<QListView*>()) {
            listView->doItemsLayout();
            if (QAbstractItemModel* model = listView->model()) {
                const QModelIndex firstVisibleIndex = model->index(0, 0);
                const QRect firstVisibleRect = listView->visualRect(firstVisibleIndex);
                if (firstVisibleIndex.isValid() && firstVisibleRect.isValid()) {
                    _previewTopInset = qMax(0, firstVisibleRect.top());
                }
                const QModelIndex lastVisibleIndex = model->index(visibleItemCount - 1, 0);
                const QRect lastVisibleRect = listView->visualRect(lastVisibleIndex);
                if (lastVisibleIndex.isValid() && lastVisibleRect.isValid()) {
                    previewHeight = previewTopMargin + lastVisibleRect.bottom() + 1 + previewBottomMargin + qMax(1, qRound(_scale));
                    _previewList->setMinimumHeight(previewHeight);
                    _previewList->setMaximumHeight(previewHeight);
                    _previewList->setFixedHeight(previewHeight);
                }
            }
        }

        applyWindowLayout(previewHeight);
        _lineEdit->setPlaceholderText(QStringLiteral("请输入重命名规则，右击查看帮助"));
    }
    update();
}