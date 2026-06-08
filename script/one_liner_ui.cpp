#include "one_liner_ui.h"

#include "one_liner_help.h"
#include "one_liner_preview.h"
#include "one_liner_tools_menu.h"

#include "one_menu.h"
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
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPixmap>
#include <QScreen>
#include <QSizePolicy>
#include <QTimer>
#include <QTreeWidget>
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
constexpr const char* kInputWidthOptionVar = "oneLinerInputWidth";

constexpr int kInputWidth = 320;
constexpr int kInputMinWidth = 220;
constexpr int kInputHeight = 26;
constexpr int kInputFontSize = 12;
constexpr int kPreviewGap = 4;
constexpr int kContentLeftPadding = 10;
constexpr qreal kPanelCornerRadius = 6.0;
constexpr qreal kItemCornerRadius = 4.0;
constexpr int kInputHistoryLimit = 50;
constexpr int kImeStatusPollIntervalMs = 120;
constexpr int kImeIconSize = 18;
constexpr int kImeStatusGlyphSize = 10;
constexpr int kImeClearGlyphSize = 12;
constexpr int kImeIconSpacing = 0;
constexpr int kImeStatusRightPadding = 4;
constexpr int kResizeHandleWidth = 8;
#ifdef _WIN32
constexpr WPARAM kImeGetConversionMode = 0x0001;
constexpr WPARAM kImeGetOpenStatus = 0x0005;
constexpr WPARAM kImeSetConversionMode = 0x0002;
constexpr WPARAM kImeSetOpenStatus = 0x0006;
#endif

constexpr char kClearTextIconPath[] = "oneLinerIcons:fluent--dismiss-20-filled.svg";
constexpr char kLanguageChineseIconPath[] = "oneLinerIcons:Chinese.svg";
constexpr char kLanguageEnglishIconPath[] = "oneLinerIcons:English.svg";
constexpr char kCapsLockIconPath[] = "oneLinerIcons:fluent--keyboard-shift-uppercase-20-filled.svg";

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

bool setImeControl(HWND hwnd, WPARAM command, LPARAM value)
{
    if (hwnd == nullptr) {
        return false;
    }

    const HWND imeWindow = ::ImmGetDefaultIMEWnd(hwnd);
    if (imeWindow == nullptr) {
        return false;
    }

    DWORD_PTR result = 0;
    const LRESULT ok = ::SendMessageTimeoutW(
        imeWindow,
        WM_IME_CONTROL,
        command,
        value,
        SMTO_NORMAL,
        100,
        &result);
    return ok != 0;
}

bool setImeChineseState(HWND hwnd, bool chinese)
{
    if (hwnd == nullptr) {
        return false;
    }

    if (!chinese) {
        return setImeControl(hwnd, kImeSetOpenStatus, FALSE);
    }

    const int currentMode = queryImeControl(hwnd, kImeGetConversionMode);
    const LPARAM nextMode = currentMode >= 0
        ? static_cast<LPARAM>(currentMode | IME_CMODE_NATIVE)
        : static_cast<LPARAM>(IME_CMODE_NATIVE);

    const bool opened = setImeControl(hwnd, kImeSetOpenStatus, TRUE);
    const bool converted = setImeControl(hwnd, kImeSetConversionMode, nextMode);
    return opened || converted;
}

void sendVirtualKey(WORD virtualKey)
{
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = virtualKey;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = virtualKey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    ::SendInput(2, inputs, sizeof(INPUT));
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

int readIntOptionVar(const char* optionVarName, int defaultValue)
{
    int exists = 0;
    if (MGlobal::executeCommand(MString("optionVar -exists \"") + optionVarName + "\"", exists, false, false) != MS::kSuccess || exists == 0) {
        return defaultValue;
    }

    int value = defaultValue;
    if (MGlobal::executeCommand(MString("optionVar -q \"") + optionVarName + "\"", value, false, false) != MS::kSuccess) {
        return defaultValue;
    }

    return value;
}

void writeBoolOptionVar(const char* optionVarName, bool value)
{
    const QString command = QStringLiteral("optionVar -iv \"%1\" %2")
        .arg(QString::fromUtf8(optionVarName))
        .arg(value ? 1 : 0);
    MGlobal::executeCommand(toMString(command), false, false);
}

void writeIntOptionVar(const char* optionVarName, int value)
{
    const QString command = QStringLiteral("optionVar -iv \"%1\" %2")
        .arg(QString::fromUtf8(optionVarName))
        .arg(value);
    MGlobal::executeCommand(toMString(command), false, false);
}

void configureInputIconActionStyle(OneQtAction* action, qreal scale, const QVariant& iconSource, int iconGlyphSize = kImeIconSize)
{
    if (action == nullptr) {
        return;
    }

    const OneQtAction::StateName states[] = {
        OneQtAction::Normal,
        OneQtAction::Hover,
        OneQtAction::Pressed,
        OneQtAction::Disabled,
    };

    for (OneQtAction::StateName state : states) {
        OneQtActionStyle style = action->style(state);
        style.text.clear();
        style.iconSource = iconSource;
        style.iconCanvasSize = QSize(kImeIconSize, kImeIconSize);
        style.iconSize = QSize(iconGlyphSize, iconGlyphSize);
        style.padding = QMargins(0, 0, 0, 0);
        style.spacing = 0;
        style.borderWidth = 0.0;
        style.cornerRadii = {kItemCornerRadius, kItemCornerRadius, kItemCornerRadius, kItemCornerRadius};
        style.iconBrush = OneQtBrush::fromColor(QColor(QStringLiteral("#f4f6f8")));
        style.borderBrush = OneQtBrush::fromColor(QColor(0, 0, 0, 0));
        style.backgroundBrush = OneQtBrush::fromColor(QColor(0, 0, 0, 0));

        if (state == OneQtAction::Hover) {
            style.backgroundBrush = OneQtBrush::fromColor(QColor(255, 255, 255, 28));
        } else if (state == OneQtAction::Pressed) {
            style.backgroundBrush = OneQtBrush::fromColor(QColor(255, 255, 255, 42));
        } else if (state == OneQtAction::Disabled) {
            style.iconBrush = OneQtBrush::fromColor(QColor(244, 246, 248, 90));
        }

        action->setStyle(state, style);
    }

    action->setScale(scale);
    action->setPreferredSize(QSize(kImeIconSize, kImeIconSize));
}

} // namespace

OneLinerWindow::OneLinerWindow(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool)
    , _scale(1.0)
    , _contentWidth(readIntOptionVar(kInputWidthOptionVar, kInputWidth))
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
    , _resizingWidth(false)
    , _resizeStartGlobalX(0)
    , _resizeStartWidth(0)
    , _rootLayout(new QVBoxLayout(this))
    , _lineEdit(new QLineEdit(this))
    , _imeStatusHost(new QWidget(_lineEdit))
    , _clearTextAction(new OneQtAction(_imeStatusHost))
    , _imeLanguageAction(new OneQtAction(_imeStatusHost))
    , _imeCapsAction(new OneQtAction(_imeStatusHost))
    , _imeStatusTimer(new QTimer(this))
    , _previewList(new OneLinerPreviewTree(this))
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    _inputBackground.setBrush(OneQtBrush::fromColor(QColor(18, 22, 29, 184)));
    _inputBackground.setBorderBrush(OneQtBrush::fromColor(QColor(82, 133, 166, 210)));
    _inputBackground.setBorderWidth(1.0);
    _inputBackground.setCornerRadius({kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius});

    _previewBackground.setBrush(OneQtBrush::fromColor(QColor(18, 22, 29, 176)));
    _previewBackground.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    _previewBackground.setBorderWidth(0.0);
    _previewBackground.setCornerRadius({kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius});

    _lineEdit->setFrame(false);
    _lineEdit->setAttribute(Qt::WA_TranslucentBackground, true);
    _lineEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    _lineEdit->setPlaceholderText(QStringLiteral("请输入重命名规则，右击查看更多；按住中间拖动以移动窗口"));
    _lineEdit->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        " background: transparent;"
        " border: none;"
        " color: #f4f6f8;"
        " selection-background-color: #5285a6;"
        " selection-color: #ffffff;"
        "}"));
    _lineEdit->setToolTip(QStringLiteral("输入重命名规则；支持 Maya 通配符 * ?，以及 -h、-h -s、-type 等 flags。"));
    _lineEdit->setStatusTip(QStringLiteral("输入规则后实时预览，回车执行；上下方向键可切换本次会话的输入历史；按住中间拖动以移动窗口"));

    _imeStatusHost->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    _imeStatusHost->setAutoFillBackground(false);
    _clearTextAction->hide();
    _clearTextAction->setToolTip(QStringLiteral("清除当前输入文本。"));
    _clearTextAction->setStatusTip(QStringLiteral("点击清空输入框内容。"));
    _clearTextAction->setClickedHandler([this]() {
        _lineEdit->clear();
        _lineEdit->setFocus();
    });
    configureInputIconActionStyle(_clearTextAction, _scale, QString::fromUtf8(kClearTextIconPath), kImeClearGlyphSize);
    _imeLanguageAction->setToolTip(QStringLiteral("点击切换中英文状态。"));
    _imeLanguageAction->setStatusTip(QStringLiteral("点击切换当前输入法的中英文状态。"));
    _imeLanguageAction->setClickedHandler([this]() { toggleImeLanguage(); });
    configureInputIconActionStyle(_imeLanguageAction, _scale, QString::fromUtf8(kLanguageChineseIconPath), kImeStatusGlyphSize);
    _imeCapsAction->setToolTip(QStringLiteral("点击切换大写锁定。"));
    _imeCapsAction->setStatusTip(QStringLiteral("点击切换 Caps Lock。"));
    _imeCapsAction->setClickedHandler([this]() { toggleCapsLock(); });
    configureInputIconActionStyle(_imeCapsAction, _scale, QString::fromUtf8(kCapsLockIconPath), kImeStatusGlyphSize);
    _imeCapsAction->hide();

    _rootLayout->setContentsMargins(0, 0, 0, 0);
    _rootLayout->setSpacing(0);

    _lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _previewList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _previewList->setPanelBackground(_previewBackground);

    _lineEdit->installEventFilter(this);
    _previewList->installEventFilter(this);

    if (QTreeWidget* previewView = _previewList->treeWidget()) {
        previewView->installEventFilter(this);
    }
    _previewList->setItemActivatedHandler([this](const QString& rawText) {
        _lineEdit->setText(rawText);
        _lineEdit->setFocus();
        _lineEdit->selectAll();
    });

    connect(_lineEdit, &QLineEdit::textChanged, this, [this]() {
        refreshPreview();
        updateLayoutMetrics();
    });
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
    const QTreeWidget* previewView = _previewList != nullptr ? _previewList->treeWidget() : nullptr;
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
        } else if (watched == previewView && event->type() == QEvent::Wheel && !_previewList->scrollEnabled()) {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            wheelEvent->accept();
            return true;
        } else if (watched == _lineEdit && event->type() == QEvent::MouseMove && !_dragging && !_resizingWidth) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            updateResizeCursor(mouseEvent->pos());
        } else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (watched == _lineEdit && mouseEvent->button() == Qt::LeftButton && isInResizeHandle(mouseEvent->pos())) {
                _resizingWidth = true;
                _resizeStartGlobalX = mouseEvent->globalX();
                _resizeStartWidth = _lineEdit->width();
                _lineEdit->grabMouse();
                _lineEdit->setCursor(Qt::SizeHorCursor);
                return true;
            }
            if (mouseEvent->button() == Qt::MiddleButton) {
                _dragging = true;
                _dragStart = mouseEvent->globalPos();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && _resizingWidth) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            setContentWidth(_resizeStartWidth + (mouseEvent->globalX() - _resizeStartGlobalX));
            return true;
        } else if (event->type() == QEvent::MouseMove && _dragging) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint current = mouseEvent->globalPos();
            move(pos() + (current - _dragStart));
            _dragStart = current;
            return true;
        } else if (watched == _lineEdit && event->type() == QEvent::Leave && !_resizingWidth) {
            _lineEdit->setCursor(Qt::IBeamCursor);
        } else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (_resizingWidth && mouseEvent->button() == Qt::LeftButton) {
                _resizingWidth = false;
                _lineEdit->releaseMouse();
                updateResizeCursor(_lineEdit->mapFromGlobal(QCursor::pos()));
                return true;
            }
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
        rebuildPreviewTree(OneLinerEngine::PreviewResult());
        return;
    }

    const QString text = _lineEdit->text();
    const OneLinerEngine::PreviewResult result = OneLinerEngine::preview(text);
    rebuildPreviewTree(result);
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

    OneLinerToolsMenu::State state;
    state.scale = _scale;
    state.contentWidth = qMax(kInputMinWidth, _contentWidth > 0 ? _contentWidth : kInputWidth);
    state.background = &_inputBackground;
    state.previewEnabled = _previewEnabled;
    state.autoCloseEnabled = _autoCloseEnabled;
    state.wildcardIncludeShapes = _wildcardIncludeShapes;

    OneLinerToolsMenu::Callbacks callbacks;
    callbacks.onPreviewToggled = [this](bool checked) {
        _closeAfterMenuAction = false;
        _restoreFocusAfterMenu = true;
        _previewEnabled = checked;
        writeBoolOptionVar(kPreviewEnabledOptionVar, _previewEnabled);
        refreshPreview();
    };
    callbacks.onAutoCloseToggled = [this](bool checked) {
        _closeAfterMenuAction = false;
        _restoreFocusAfterMenu = true;
        _autoCloseEnabled = checked;
        writeBoolOptionVar(kAutoCloseEnabledOptionVar, _autoCloseEnabled);
    };
    callbacks.onWildcardIncludeShapesToggled = [this](bool checked) {
        _closeAfterMenuAction = false;
        _restoreFocusAfterMenu = true;
        _wildcardIncludeShapes = checked;
        writeBoolOptionVar(kWildcardIncludeShapesOptionVar, _wildcardIncludeShapes);
        refreshPreview();
    };
    callbacks.onClearPasted = [this]() {
        _closeAfterMenuAction = true;
        _restoreFocusAfterMenu = false;
        MGlobal::executeCommand(toMString(QStringLiteral("oneLiner -clearPasted")), false, true);
        refreshPreview();
    };
    callbacks.onShowHelp = [this]() {
        _closeAfterMenuAction = true;
        _restoreFocusAfterMenu = false;
        showHelpDialog();
    };

    OneQtMenu* menu = OneLinerToolsMenu::createMenu(this, state, callbacks);
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

    menu->popup(_lineEdit->mapToGlobal(QPoint(0, _lineEdit->height() + qRound(kPreviewGap * _scale))));
}

void OneLinerWindow::showHelpDialog()
{
    if (_helpDialog != nullptr) {
        _helpDialog->raise();
        _helpDialog->activateWindow();
        return;
    }

    _helpDialog = OneLinerHelp::createHelpDialog(this, _scale);
    _helpDialog->show();
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
    const int defaultWidthValue = kInputWidth;
    const int minWidthValue = kInputMinWidth;
    if (_contentWidth <= 0) {
        _contentWidth = defaultWidthValue;
    }
    const int logicalWidthValue = qMax(minWidthValue, _contentWidth);
    const int widthValue = qRound(logicalWidthValue * _scale);
    const int lineHeight = qRound(kInputHeight * _scale);
    const int fontSize = qRound(kInputFontSize * _scale);
    const int imeIconSize = qRound(kImeIconSize * _scale);
    const int imeSpacing = qRound(kImeIconSpacing * _scale);
    const int imeRightPadding = qRound(kImeStatusRightPadding * _scale);
    const bool showClearTextAction = !_lineEdit->text().isEmpty();
    const int visibleIconCount = 1 + (_capsLockOn ? 1 : 0) + (showClearTextAction ? 1 : 0);
    const int imeHostWidth = visibleIconCount > 0
        ? imeIconSize * visibleIconCount + imeSpacing * qMax(0, visibleIconCount - 1)
        : 0;

    QFont editFont = baseUiFont();
    editFont.setPixelSize(fontSize);
    _lineEdit->setFont(editFont);
    _lineEdit->setFixedHeight(lineHeight);
    _lineEdit->setFixedWidth(widthValue);
    _lineEdit->setTextMargins(qRound(kContentLeftPadding * _scale), 0, imeHostWidth + imeRightPadding + qRound(4 * _scale), 0);

    configureInputIconActionStyle(_clearTextAction, _scale, QString::fromUtf8(kClearTextIconPath), kImeClearGlyphSize);
    configureInputIconActionStyle(
        _imeLanguageAction,
        _scale,
        QString::fromUtf8(_imeIsChinese ? kLanguageChineseIconPath : kLanguageEnglishIconPath),
        kImeStatusGlyphSize);
    configureInputIconActionStyle(_imeCapsAction, _scale, QString::fromUtf8(kCapsLockIconPath), kImeStatusGlyphSize);
    _imeStatusHost->setGeometry(widthValue - imeHostWidth - imeRightPadding, 0, imeHostWidth, lineHeight);
    int currentX = 0;
    if (showClearTextAction) {
        _clearTextAction->setGeometry(currentX, (lineHeight - imeIconSize) / 2, imeIconSize, imeIconSize);
        _clearTextAction->show();
        currentX += imeIconSize + imeSpacing;
    } else {
        _clearTextAction->hide();
    }
    if (_capsLockOn) {
        _imeCapsAction->setGeometry(currentX, (lineHeight - imeIconSize) / 2, imeIconSize, imeIconSize);
        _imeCapsAction->show();
        currentX += imeIconSize + imeSpacing;
    } else {
        _imeCapsAction->hide();
    }
    _imeLanguageAction->setGeometry(currentX, (lineHeight - imeIconSize) / 2, imeIconSize, imeIconSize);
    _imeLanguageAction->show();

    _previewList->applyScale(_scale);
    _previewList->setFixedWidth(widthValue);

    applyWindowLayout(_previewList->isVisible() ? _previewList->previewHeight() : 0);
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

void OneLinerWindow::toggleImeLanguage()
{
#ifdef _WIN32
    const HWND focusedWindow = focusedInputWindow();
    if (setImeChineseState(focusedWindow, !_imeIsChinese)) {
        QTimer::singleShot(60, this, [this]() {
            refreshImeStatus();
            if (_lineEdit != nullptr) {
                _lineEdit->setFocus();
            }
        });
        return;
    }

    sendVirtualKey(_imeIsChinese ? VK_NONCONVERT : VK_CONVERT);
    QTimer::singleShot(60, this, [this]() {
        refreshImeStatus();
        if (_lineEdit != nullptr) {
            _lineEdit->setFocus();
        }
    });
#else
    if (_lineEdit != nullptr) {
        _lineEdit->setFocus();
    }
#endif
}

void OneLinerWindow::toggleCapsLock()
{
#ifdef _WIN32
    sendVirtualKey(VK_CAPITAL);
    refreshImeStatus();
#endif
    if (_lineEdit != nullptr) {
        _lineEdit->setFocus();
    }
}

void OneLinerWindow::applyWindowLayout(int previewHeight)
{
    const int logicalWidthValue = qMax(kInputMinWidth, _contentWidth > 0 ? _contentWidth : kInputWidth);
    const int widthValue = qRound(logicalWidthValue * _scale);
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

    syncToolsMenuGeometry();
}

void OneLinerWindow::setContentWidth(int width)
{
    const int minWidthValue = kInputMinWidth;
    const int nextWidth = qMax(minWidthValue, qRound(width / qMax<qreal>(0.1, _scale)));
    if (_contentWidth == nextWidth) {
        return;
    }

    _contentWidth = nextWidth;
    writeIntOptionVar(kInputWidthOptionVar, _contentWidth);
    updateLayoutMetrics();
}

void OneLinerWindow::syncToolsMenuGeometry()
{
    if (_toolsMenu == nullptr || _lineEdit == nullptr) {
        return;
    }

    const int menuWidth = qMax(kInputMinWidth, _contentWidth > 0 ? _contentWidth : kInputWidth);
    _toolsMenu->setMinimumWidth(menuWidth);
    _toolsMenu->setMaximumWidth(menuWidth);
    if (_toolsMenu->isVisible()) {
        _toolsMenu->resize(qRound(menuWidth * _scale), _toolsMenu->height());
        _toolsMenu->move(_lineEdit->mapToGlobal(QPoint(0, _lineEdit->height() + qRound(kPreviewGap * _scale))));
    }
}

bool OneLinerWindow::isInResizeHandle(const QPoint& localPos) const
{
    if (_lineEdit == nullptr) {
        return false;
    }

    const int handleWidth = qMax(6, qRound(kResizeHandleWidth * _scale));
    return localPos.x() >= _lineEdit->width() - handleWidth;
}

void OneLinerWindow::updateResizeCursor(const QPoint& localPos)
{
    if (_lineEdit == nullptr) {
        return;
    }

    _lineEdit->setCursor(isInResizeHandle(localPos) ? Qt::SizeHorCursor : Qt::IBeamCursor);
}

void OneLinerWindow::rebuildPreviewTree(const OneLinerEngine::PreviewResult& result)
{
    _previewList->rebuildPreview(result);
    _previewScrollEnabled = _previewList->scrollEnabled();

    if (!_previewList->hasItems()) {
        _previewScrollEnabled = false;
        _previewTopInset = 0;
        _previewList->hide();
        _previewList->setMinimumHeight(0);
        _previewList->setMaximumHeight(0);
        _previewList->setFixedHeight(0);
        applyWindowLayout(0);
        _lineEdit->setPlaceholderText(result.selectionOnly
            ? QStringLiteral("未找到匹配对象；按住中间拖动以移动窗口")
            : QStringLiteral("请选择对象，右击查看更多"));
    } else {
        _previewList->show();

        applyWindowLayout(_previewList->previewHeight());
        _lineEdit->setPlaceholderText(QStringLiteral("请输入重命名规则，右击查看更多"));
    }
    update();
}