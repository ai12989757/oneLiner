#include "one_liner_ui.h"

#include "one_list.h"
#include "one_menu.h"
#include "oneqt_brush.h"
#include "oneqt_system.h"

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <QApplication>
#include <QCursor>
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
#include <QScreen>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <QWheelEvent>

namespace {

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
constexpr int kMenuHorizontalPadding = 10;
constexpr int kMenuVerticalPadding = 6;
constexpr int kMenuSpacing = 1;
constexpr int kMenuTextSpacing = 8;

QPointer<OneLinerWindow> g_window;

QWidget* mayaMainWindow()
{
    return reinterpret_cast<QWidget*>(MQtUtil::mainWindow());
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
        "<li><b>/h</b> 作用到选中层级</li>"
        "<li><b>/a</b> 在替换模式下作用到全场景</li>"
        "<li><b>f:</b> 选择包含字符的对象</li>"
        "<li><b>fs:</b> 选择前缀匹配的对象</li>"
        "<li><b>fe:</b> 选择后缀匹配的对象</li>"
        "</ul>");
}

QFont baseUiFont()
{
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(kInputFontSize);
    return font;
}

void syncMenuActionFont(OneQtAction* action, const QFont& font)
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
        style.padding = QMargins(kMenuHorizontalPadding, kMenuVerticalPadding, kMenuHorizontalPadding, kMenuVerticalPadding);
        style.spacing = kMenuTextSpacing;
        style.cornerRadii = {kItemCornerRadius, kItemCornerRadius, kItemCornerRadius, kItemCornerRadius};
        action->setStyle(state, style);
    }
}

} // namespace

OneLinerWindow::OneLinerWindow(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool)
    , _scale(1.0)
    , _maxPreviewCount(19)
    , _previewScrollEnabled(false)
    , _dragging(false)
    , _lineEdit(new QLineEdit(this))
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

    _lineEdit->installEventFilter(this);
    _previewList->installEventFilter(this);
    if (QListView* previewView = _previewList->findChild<QListView*>()) {
        previewView->installEventFilter(this);
    }

    connect(_lineEdit, &QLineEdit::textChanged, this, [this]() { refreshPreview(); });
    connect(_lineEdit, &QLineEdit::returnPressed, this, [this]() { executeRule(); });
    connect(_lineEdit, &QWidget::customContextMenuRequested, this, [this]() { showToolsMenu(); });

    updateScaleFromMaya();
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

void OneLinerWindow::closeWindow()
{
    if (g_window != nullptr) {
        g_window->close();
        g_window->deleteLater();
        g_window = nullptr;
    }
}

bool OneLinerWindow::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate) {
        close();
        return true;
    }
    return QWidget::event(event);
}

bool OneLinerWindow::eventFilter(QObject* watched, QEvent* event)
{
    const QListView* previewView = _previewList != nullptr ? _previewList->findChild<QListView*>() : nullptr;
    if ((watched == _lineEdit || watched == _previewList || watched == previewView) && event != nullptr) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
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
    update();
}

void OneLinerWindow::refreshPreview()
{
    const QString text = _lineEdit->text().trimmed();
    const OneLinerEngine::PreviewResult result = OneLinerEngine::preview(text);
    rebuildPreviewList(result.items, result.selectionOnly);
}

void OneLinerWindow::executeRule()
{
    const QString text = _lineEdit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    OneLinerEngine::execute(text);
    _lineEdit->clear();
    refreshPreview();
}

void OneLinerWindow::showToolsMenu()
{
    OneQtMenu* menu = new OneQtMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose, true);
    menu->setScale(_scale);
    menu->setMinimumWidth(kInputWidth);
    menu->setMaximumWidth(kInputWidth);
    menu->setMargins(QMargins(qRound(3 * _scale), qRound(3 * _scale), qRound(3 * _scale), qRound(3 * _scale)));
    menu->setSpacing(qRound(kMenuSpacing * _scale));
    menu->setItemHeight(kMenuItemHeight);
    menu->setBackground(_inputBackground);

    const QFont menuFont = baseUiFont();

    if (OneQtAction* action = menu->addAction(QStringLiteral("前缀为..."))) {
        syncMenuActionFont(action, menuFont);
        action->setClickedHandler([this]() { _lineEdit->setText(QStringLiteral("fs:")); });
    }
    if (OneQtAction* action = menu->addAction(QStringLiteral("后缀为..."))) {
        syncMenuActionFont(action, menuFont);
        action->setClickedHandler([this]() { _lineEdit->setText(QStringLiteral("fe:")); });
    }
    if (OneQtAction* action = menu->addAction(QStringLiteral("中间包含..."))) {
        syncMenuActionFont(action, menuFont);
        action->setClickedHandler([this]() { _lineEdit->setText(QStringLiteral("f:")); });
    }
    menu->addSeparator();
    if (OneQtAction* action = menu->addAction(QStringLiteral("清除 pasted__ 前缀"))) {
        syncMenuActionFont(action, menuFont);
        action->setClickedHandler([this]() {
            OneLinerEngine::renamePastedPrefix();
            refreshPreview();
        });
    }
    menu->addSeparator();
    if (OneQtAction* action = menu->addAction(QStringLiteral("帮助"))) {
        syncMenuActionFont(action, menuFont);
        action->setClickedHandler([this]() { showHelpDialog(); });
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

    QFont editFont = baseUiFont();
    editFont.setPixelSize(fontSize);
    _lineEdit->setFont(editFont);
    _lineEdit->setGeometry(0, 0, widthValue, lineHeight);
    _lineEdit->setTextMargins(qRound(kContentLeftPadding * _scale), 0, qRound(kContentLeftPadding * _scale), 0);

    QFont listFont = baseUiFont();
    _previewList->setScale(_scale);
    _previewList->setItemFont(listFont);
    _previewList->setItemHeight(kPreviewItemHeight);
    _previewList->setItemSpacing(kPreviewItemSpacing);
    _previewList->setItemPadding(QMargins(kContentLeftPadding, 0, kContentLeftPadding, 0));

    resize(widthValue, lineHeight);
}

void OneLinerWindow::rebuildPreviewList(const QStringList& items, bool selectionOnly)
{
    const int widthValue = qRound(kInputWidth * _scale);
    const int lineHeight = qRound(kInputHeight * _scale);
    const int itemHeight = qMax(24, qRound(kPreviewItemHeight * _scale));
    const int itemSpacing = qRound(kPreviewItemSpacing * _scale);
    const int previewGap = qRound(kPreviewGap * _scale);

    _previewList->clearItems();
    for (const QString& item : items) {
        _previewList->addItem(item);
    }

    if (items.isEmpty()) {
        _previewScrollEnabled = false;
        _previewList->hide();
        resize(widthValue, lineHeight);
        _lineEdit->setPlaceholderText(selectionOnly
            ? QStringLiteral("未找到匹配对象")
            : QStringLiteral("请选择对象，或键入 f: / fs: / fe: 来选择对象"));
    } else {
        const int visibleItemCount = qMin(items.size(), kPreviewMaxVisibleItems);
        int actualItemHeight = itemHeight;
        int actualItemSpacing = itemSpacing;
        int previewTopMargin = qRound(kPreviewListOuterMargin * _scale);
        int previewBottomMargin = qRound(kPreviewListOuterMargin * _scale);
        if (QLayout* previewLayout = _previewList->layout()) {
            const QMargins margins = previewLayout->contentsMargins();
            previewTopMargin = qRound(margins.top() * _scale);
            previewBottomMargin = qRound(margins.bottom() * _scale);
        }

        if (QListView* listView = _previewList->findChild<QListView*>()) {
            actualItemHeight = qMax(actualItemHeight, listView->sizeHintForRow(0));
            actualItemSpacing = qMax(actualItemSpacing, listView->spacing());
        }

        const int previewHeight = visibleItemCount * actualItemHeight
            + qMax(0, visibleItemCount - 1) * actualItemSpacing
            + previewTopMargin
            + previewBottomMargin;

        _previewScrollEnabled = items.size() > visibleItemCount;

        if (QListView* listView = _previewList->findChild<QListView*>()) {
            listView->setVerticalScrollBarPolicy(_previewScrollEnabled ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
            if (QScrollBar* scrollBar = listView->verticalScrollBar()) {
                if (!_previewScrollEnabled) {
                    scrollBar->setValue(scrollBar->minimum());
                }
            }
        }

        _previewList->setGeometry(0, lineHeight + previewGap, widthValue, previewHeight);
        _previewList->show();
        resize(widthValue, lineHeight + previewGap + previewHeight);
        _lineEdit->setPlaceholderText(QStringLiteral("请输入重命名规则，右击查看帮助"));
    }
    update();
}