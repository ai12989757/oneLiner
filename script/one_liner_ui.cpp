#include "one_liner_ui.h"

#include "one_liner_help.h"
#include "one_liner_preview.h"
#include "one_liner_tools_menu.h"

#include "one_line_edit.h"
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
#include <QKeyEvent>
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

namespace {

constexpr const char* kPreviewEnabledOptionVar = "oneLinerPreviewEnabled";
constexpr const char* kAutoCloseEnabledOptionVar = "oneLinerAutoCloseEnabled";
constexpr const char* kWildcardIncludeShapesOptionVar = "oneLinerWildcardIncludeShapes";
constexpr const char* kInputWidthOptionVar = "oneLinerInputWidth";

constexpr int kInputWidth = 320;
constexpr int kInputMinWidth = 220;
constexpr int kInputHeight = 28;
constexpr int kInputFontSize = 12;
constexpr int kPreviewGap = 4;
constexpr qreal kPanelCornerRadius = 6.0;
constexpr int kInputHistoryLimit = 50;
constexpr int kResizeHandleWidth = 8;

constexpr char kClearTextIconPath[] = "oneLinerIcons:fluent--dismiss-20-filled.svg";
constexpr char kLanguageChineseIconPath[] = "oneLinerIcons:Chinese.svg";
constexpr char kLanguageEnglishIconPath[] = "oneLinerIcons:English.svg";
constexpr char kCapsLockOnIconPath[] = "oneLinerIcons:fluent--keyboard-shift-uppercase-20-filled.svg";
constexpr char kCapsLockOffIconPath[] = "oneLinerIcons:fluent--keyboard-shift-uppercase-20-regular.svg";

QPointer<OneLinerWindow> g_window;
QStringList g_inputHistory;

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
    , _dragging(false)
    , _resizingWidth(false)
    , _resizeStartGlobalX(0)
    , _resizeStartWidth(0)
    , _rootLayout(new QVBoxLayout(this))
    , _lineEdit(new OneQtLineEdit(this))
    , _previewList(new OneLinerPreviewTree(this))
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    // 输入框面板背景（同时用于工具菜单外壳背景，保持视觉一致）。
    _inputBackground.setBrush(OneQtBrush::fromColor(QColor(18, 22, 29, 184)));
    _inputBackground.setBorderBrush(OneQtBrush::fromColor(QColor(82, 133, 166, 210)));
    _inputBackground.setBorderWidth(1.0);
    _inputBackground.setCornerRadius({kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius});

    _previewBackground.setBrush(OneQtBrush::fromColor(QColor(18, 22, 29, 176)));
    _previewBackground.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    _previewBackground.setBorderWidth(0.0);
    _previewBackground.setCornerRadius({kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius, kPanelCornerRadius});

    // 让 OneLineEdit 使用与旧版一致的面板背景（焦点时才显示边框）。
    _lineEdit->setBackground(_inputBackground);
    _lineEdit->setFocusBorderColor(QColor(82, 133, 166, 210));
    _lineEdit->setSelectionColor(QColor(82, 133, 166));
    _lineEdit->setPlaceholderText(QStringLiteral("请输入重命名规则，右击查看更多；按住中间拖动以移动窗口"));
    _lineEdit->setToolTip(QStringLiteral("输入重命名规则；支持 Maya 通配符 * ?，以及 -h、-h -s、-type 等 flags。"));
    _lineEdit->setStatusTip(QStringLiteral("输入规则后实时预览，回车执行；上下方向键可切换本次会话的输入历史；按住中间拖动以移动窗口"));

    // 启用内置工具按钮：清除、大小写(Caps)、中英文(IME)。这些由 OneLineEdit
    // 自动跟随系统状态并处理点击切换（Windows）。
    _lineEdit->setClearButtonEnabled(true);
    _lineEdit->setClearButtonIcon(QString::fromUtf8(kClearTextIconPath));
    _lineEdit->setCapsButtonEnabled(true, /*autoHide=*/true);
    _lineEdit->setCapsOffIcon(QString::fromUtf8(kCapsLockOffIconPath));
    _lineEdit->setCapsOnIcon(QString::fromUtf8(kCapsLockOnIconPath));
    _lineEdit->setLanguageButtonEnabled(true);
    _lineEdit->setLanguageChineseIcon(QString::fromUtf8(kLanguageChineseIconPath));
    _lineEdit->setLanguageEnglishIcon(QString::fromUtf8(kLanguageEnglishIconPath));

    _rootLayout->setContentsMargins(0, 0, 0, 0);
    _rootLayout->setSpacing(0);

    _lineEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _previewList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _previewList->setPanelBackground(_previewBackground);

    // 在整体控件与内部 QLineEdit 上安装事件过滤器，用于历史/拖动/调宽/右键菜单。
    _lineEdit->installEventFilter(this);
    if (QLineEdit* innerEdit = _lineEdit->lineEdit()) {
        innerEdit->installEventFilter(this);
        innerEdit->setStatusTip(_lineEdit->statusTip());
    }
    _previewList->installEventFilter(this);

    if (QTreeWidget* previewView = _previewList->treeWidget()) {
        previewView->installEventFilter(this);
    }
    _previewList->setItemActivatedHandler([this](const QString& rawText) {
        _lineEdit->setText(rawText);
        if (QLineEdit* innerEdit = _lineEdit->lineEdit()) {
            innerEdit->setFocus();
            innerEdit->selectAll();
        }
    });

    _lineEdit->setTextChangedHandler([this](const QString&) {
        _historyIndex = -1;
        _historyDraft.clear();
        refreshPreview();
        updateLayoutMetrics();
    });
    _lineEdit->setReturnPressedHandler([this]() { executeRule(); });

    updateScaleFromMaya();
    positionNearCursor();
    _previewList->hide();
    if (QLineEdit* innerEdit = _lineEdit->lineEdit()) {
        innerEdit->setFocus();
    }
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
            if (QLineEdit* innerEdit = window->_lineEdit->lineEdit()) {
                innerEdit->setFocus();
            }
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
    QLineEdit* innerEdit = _lineEdit != nullptr ? _lineEdit->lineEdit() : nullptr;
    // 输入相关事件来自内部 QLineEdit；整体 OneLineEdit 只用于识别（历史键在内部触发）。
    const bool fromInput = (watched == _lineEdit || watched == innerEdit);

    if ((fromInput || watched == _previewList || watched == previewView) && event != nullptr) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (fromInput && keyEvent->key() == Qt::Key_Up) {
                return stepHistory(-1);
            }
            if (fromInput && keyEvent->key() == Qt::Key_Down) {
                return stepHistory(1);
            }
            if (keyEvent->key() == Qt::Key_Escape) {
                close();
                return true;
            }
        } else if (fromInput && event->type() == QEvent::ContextMenu) {
            // 用 oneLiner 自己的工具菜单替换内置右键菜单。
            showToolsMenu();
            return true;
        } else if (watched == previewView && event->type() == QEvent::Wheel && !_previewList->scrollEnabled()) {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            wheelEvent->accept();
            return true;
        } else if (fromInput && event->type() == QEvent::MouseMove && !_dragging && !_resizingWidth) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            updateResizeCursor(_lineEdit->mapFromGlobal(mouseEvent->globalPos()));
        } else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint localInWidget = _lineEdit->mapFromGlobal(mouseEvent->globalPos());
            if (fromInput && mouseEvent->button() == Qt::LeftButton && isInResizeHandle(localInWidget)) {
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
        } else if (fromInput && event->type() == QEvent::Leave && !_resizingWidth) {
            if (innerEdit != nullptr) {
                innerEdit->setCursor(Qt::IBeamCursor);
            }
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
    // OneLineEdit 自身负责绘制输入框面板背景，主窗口不再手工绘制。
    Q_UNUSED(event)
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
    const OneLinerEngine::ExecutePlan plan = OneLinerEngine::buildExecutePlan(
        text,
        OneLinerEngine::ScopeMode::Selected,
        false);
    bool success = true;
    if (!plan.noop) {
        if (plan.selectionOnly) {
            success = OneLinerEngine::selectTargets(plan.selectionTargets);
        } else if (!plan.renameOperations.isEmpty()) {
            success = OneLinerEngine::applyRenameOperations(plan.renameOperations, true);
        }
    }
    if (!success) {
        MGlobal::displayWarning("oneLiner: failed to execute rule.");
    }
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
    if (QLineEdit* innerEdit = _lineEdit->lineEdit()) {
        innerEdit->setCursorPosition(nextText.size());
    }
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
                if (QLineEdit* innerEdit = _lineEdit->lineEdit()) {
                    innerEdit->setFocus();
                }
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
            if (QLineEdit* innerEdit = _lineEdit->lineEdit()) {
                innerEdit->setFocus();
            }
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

    // 让 OneLineEdit 跟随 DPI 缩放（内部按钮/背景随之更新），并固定尺寸。
    _lineEdit->setScale(_scale);
    if (QLineEdit* innerEdit = _lineEdit->lineEdit()) {
        QFont editFont = baseUiFont();
        editFont.setPixelSize(fontSize);
        innerEdit->setFont(editFont);
    }
    _lineEdit->setFixedHeight(lineHeight);
    _lineEdit->setFixedWidth(widthValue);

    _previewList->applyScale(_scale);
    _previewList->setFixedWidth(widthValue);

    applyWindowLayout(_previewList->isVisible() ? _previewList->previewHeight() : 0);
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

    const Qt::CursorShape shape = isInResizeHandle(localPos) ? Qt::SizeHorCursor : Qt::IBeamCursor;
    _lineEdit->setCursor(shape);
    if (QLineEdit* innerEdit = _lineEdit->lineEdit()) {
        innerEdit->setCursor(shape);
    }
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