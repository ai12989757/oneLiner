#include "one_liner_preview.h"

#include "one_scroll_bar.h"
#include "one_background.h"
#include "oneqt_brush.h"

#include <QAbstractItemView>
#include <QFont>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHash>
#include <QHeaderView>
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

namespace {

constexpr int kPreviewItemHeight = 22;
constexpr int kPreviewMaxVisibleItems = 10;
constexpr int kPreviewIconSourceSize = 64;
constexpr qreal kPreviewIconHeightRatio = 0.8;
constexpr int kPreviewOuterMargin = 5;

QFont previewFont(qreal scale)
{
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(qRound(12 * scale));
    return font;
}

class PreviewNameItemWidget : public QWidget
{
public:
    PreviewNameItemWidget(const QPixmap& iconPixmap, const QString& text, qreal scale, QWidget* parent = nullptr)
        : QWidget(parent)
        , _iconPixmap(iconPixmap)
        , _text(text)
        , _scale(scale)
    {
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAutoFillBackground(false);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    void setScale(qreal scale)
    {
        _scale = scale;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const int contentGap = qMax(1, qRound(1 * _scale));
        const int resolvedIconSize = qMax(11, qRound(height() * kPreviewIconHeightRatio));

        int textLeft = 0;
        if (!_iconPixmap.isNull()) {
            const QPixmap scaledPixmap = _iconPixmap.scaled(
                resolvedIconSize,
                resolvedIconSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
            const int iconX = contentGap;
            const int iconY = qMax(0, (height() - scaledPixmap.height()) / 2);
            painter.drawPixmap(iconX, iconY, scaledPixmap);
            textLeft = iconX + scaledPixmap.width() + contentGap;
        }

        painter.setPen(QColor(226, 232, 240, 240));
        painter.setFont(font());
        const QFontMetrics metrics(font());
        const QString elidedText = metrics.elidedText(_text, Qt::ElideRight, qMax(0, width() - textLeft));
        painter.drawText(QRect(textLeft, 0, qMax(0, width() - textLeft), height()), Qt::AlignVCenter | Qt::AlignLeft, elidedText);
    }

private:
    QPixmap _iconPixmap;
    QString _text;
    qreal _scale;
};

void updatePreviewNameWidgets(QTreeWidget* treeWidget, QTreeWidgetItem* item, qreal scale)
{
    if (treeWidget == nullptr || item == nullptr) {
        return;
    }

    if (PreviewNameItemWidget* nameWidget = dynamic_cast<PreviewNameItemWidget*>(treeWidget->itemWidget(item, 0))) {
        nameWidget->setFont(previewFont(scale));
        nameWidget->setScale(scale);
    }

    for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
        updatePreviewNameWidgets(treeWidget, item->child(childIndex), scale);
    }
}

QPixmap mayaNodePixmap(const QString& typeName)
{
    static QHash<QString, QPixmap> iconCache;

    const QString normalizedTypeName = typeName.trimmed();
    if (normalizedTypeName.isEmpty()) {
        return QPixmap();
    }

    const auto cachedIt = iconCache.constFind(normalizedTypeName);
    if (cachedIt != iconCache.constEnd()) {
        return cachedIt.value();
    }

    const QStringList candidatePaths = {
        QStringLiteral(":/out_%1.png").arg(normalizedTypeName),
        QStringLiteral(":/out_%1.svg").arg(normalizedTypeName),
        QStringLiteral(":/%1.png").arg(normalizedTypeName),
        QStringLiteral(":/%1.svg").arg(normalizedTypeName)
    };

    for (const QString& candidatePath : candidatePaths) {
        const QIcon icon(candidatePath);
        if (!icon.isNull()) {
            const QPixmap pixmap = icon.pixmap(kPreviewIconSourceSize, kPreviewIconSourceSize);
            if (!pixmap.isNull()) {
                iconCache.insert(normalizedTypeName, pixmap);
                return pixmap;
            }
        }
    }

    iconCache.insert(normalizedTypeName, QPixmap());
    return QPixmap();
}

void applyPreviewScrollBarStyle(OneQtScrollBar* scrollBar)
{
    if (scrollBar == nullptr) {
        return;
    }

    OneQtBackground transparentScrollTrack;
    transparentScrollTrack.setBrush(OneQtBrush::fromColor(QColor(255, 255, 255, 0)));
    transparentScrollTrack.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    transparentScrollTrack.setBorderWidth(0.0);
    transparentScrollTrack.setCornerRadius(0.0);

    OneQtBackground normalScrollThumb;
    normalScrollThumb.setBrush(OneQtBrush::fromColor(QColor(255, 255, 255, 30)));
    normalScrollThumb.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    normalScrollThumb.setBorderWidth(0.0);
    normalScrollThumb.setCornerRadius(0.0);

    OneQtBackground hoverScrollThumb;
    hoverScrollThumb.setBrush(OneQtBrush::fromColor(QColor(255, 255, 255, 0)));
    hoverScrollThumb.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    hoverScrollThumb.setBorderWidth(0.0);
    hoverScrollThumb.setCornerRadius(0.0);

    OneQtBackground pressedScrollThumb;
    pressedScrollThumb.setBrush(OneQtBrush::fromColor(QColor(255, 255, 255, 80)));
    pressedScrollThumb.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    pressedScrollThumb.setBorderWidth(0.0);
    pressedScrollThumb.setCornerRadius(0.0);

    scrollBar->setTrackBackground(transparentScrollTrack);
    scrollBar->setThumbBackground(normalScrollThumb);
    scrollBar->setThumbHoverBackground(hoverScrollThumb);
    scrollBar->setThumbPressedBackground(pressedScrollThumb);
    scrollBar->setTrackMargins(QMargins(0, 0, 0, 0));
}

void applyPreviewTreeItemStyle(QTreeWidget* treeWidget)
{
    if (treeWidget == nullptr) {
        return;
    }

    treeWidget->setProperty("oneLinerPreviewItemStyle", true);
    treeWidget->setStyleSheet(
        QStringLiteral(
            "QTreeView {"
            " border: none;"
            " outline: 0;"
            "}"
            "QTreeView::viewport {"
            " border: none;"
            "}"
            "QTreeView::item {"
            " color: rgba(226,232,240,240);"
            " background: transparent;"
            " padding: 0px;"
            " margin: 0px;"
            "}"
            "QTreeView::item:hover {"
            " color: rgba(226,232,240,240);"
            " background: transparent;"
            " padding: 0px;"
            " margin: 0px;"
            "}"
            "QTreeView::item:selected {"
            " color: rgba(226,232,240,240);"
            " background: transparent;"
            " border: none;"
            " outline: 0;"
            " padding: 0px;"
            " margin: 0px;"
            "}"));
}

} // namespace

OneLinerPreviewTree::OneLinerPreviewTree(QWidget* parent)
    : QWidget(parent)
    , _tree(new OneQtTree(this))
    , _background(this)
    , _scale(1.0)
    , _previewHeight(0)
    , _itemCount(0)
    , _scrollEnabled(false)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setToolTip(QStringLiteral("预览列表。双击任意项可将其文本写回输入框。"));
    setStatusTip(QStringLiteral("双击预览项可快速把该文本填入输入框。"));

    _background.setBrush(OneQtBrush::fromColor(QColor(18, 22, 29, 176)));
    _background.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    _background.setBorderWidth(0.0);
    _background.setCornerRadius(6.0);

    QGridLayout* rootLayout = new QGridLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(_tree, 0, 0);

    OneQtBackground transparentTreeBackground;
    transparentTreeBackground.setBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    transparentTreeBackground.setBorderBrush(OneQtBrush::fromColor(QColor(0, 0, 0, 0)));
    transparentTreeBackground.setBorderWidth(0.0);
    transparentTreeBackground.setCornerRadius(0.0);
    _tree->setPanelBackground(transparentTreeBackground);

    QTreeWidget* nativeTree = treeWidget();
    nativeTree->setColumnCount(1);
    nativeTree->setHeaderLabels({ QStringLiteral("name") });
    nativeTree->setHeaderHidden(true);
    nativeTree->setSelectionMode(QAbstractItemView::NoSelection);
    nativeTree->setExpandsOnDoubleClick(false);
    nativeTree->setSortingEnabled(false);
    nativeTree->setUniformRowHeights(true);
    nativeTree->setAllColumnsShowFocus(false);
    nativeTree->setRootIsDecorated(true);
    nativeTree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    nativeTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    nativeTree->header()->setStretchLastSection(true);
    nativeTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    QObject::connect(nativeTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        if (item == nullptr || !_itemActivatedHandler) {
            return;
        }
        _itemActivatedHandler(item->data(0, Qt::UserRole).toString());
    });

    applyPreviewScrollBarStyle(_tree->verticalOneScrollBar());
    applyPreviewScrollBarStyle(_tree->horizontalOneScrollBar());
}

void OneLinerPreviewTree::applyScale(qreal scale)
{
    _scale = qMax<qreal>(0.1, scale);
    QTreeWidget* nativeTree = treeWidget();
    applyPreviewTreeItemStyle(nativeTree);
    const QFont treeFont = previewFont(scale);
    nativeTree->setFont(treeFont);
    nativeTree->setProperty("oneqtTreeStructureLeftMargin", qRound(4 * scale));
    nativeTree->setProperty("oneqtTreeIndicatorExtent", qRound(10 * scale));
    nativeTree->setProperty("oneqtTreeLeafDotDiameter", qRound(5 * scale));
    nativeTree->setProperty("oneqtTreeMarkerItemGap", qRound(3 * scale));
    nativeTree->setProperty("oneqtTreeRowCornerRadius", qMax(2, qRound(4 * scale)));
    nativeTree->setProperty("oneqtTreeDisableSelectionState", true);

    if (QLayout* rootLayout = layout()) {
        const int outerMargin = qRound(kPreviewOuterMargin * scale);
        rootLayout->setContentsMargins(outerMargin, outerMargin, outerMargin, outerMargin);
    }

    _background.setScale(scale);
    _tree->setScale(scale);
    applyPreviewTreeItemStyle(nativeTree);
    nativeTree->clearSelection();
    nativeTree->setCurrentItem(nullptr);

    for (int index = 0; index < nativeTree->topLevelItemCount(); ++index) {
        updatePreviewNameWidgets(nativeTree, nativeTree->topLevelItem(index), scale);
    }
    nativeTree->viewport()->update();

    if (OneQtScrollBar* verticalScrollBar = _tree->verticalOneScrollBar()) {
        verticalScrollBar->setThickness(4);
        verticalScrollBar->setMinThumbLength(qMax(18, qRound(20 * scale)));
    }
    if (OneQtScrollBar* horizontalScrollBar = _tree->horizontalOneScrollBar()) {
        horizontalScrollBar->setThickness(4);
        horizontalScrollBar->setMinThumbLength(qMax(18, qRound(20 * scale)));
    }

    const int visibleItemCount = qMin(_itemCount, kPreviewMaxVisibleItems);
    const QMargins margins = layout() != nullptr ? layout()->contentsMargins() : QMargins();
    _previewHeight = visibleItemCount > 0
        ? margins.top() + margins.bottom() + visibleItemCount * resolvedRowHeight()
        : 0;
    setMinimumHeight(_previewHeight);
    setMaximumHeight(_previewHeight);
    setFixedHeight(_previewHeight);
}

void OneLinerPreviewTree::rebuildPreview(const OneLinerEngine::PreviewResult& result)
{
    QTreeWidget* nativeTree = treeWidget();
    _tree->clearItems();

    _itemCount = result.previewItems.size();
    _scrollEnabled = _itemCount > kPreviewMaxVisibleItems;

    QHash<QString, QTreeWidgetItem*> itemsByPath;
    itemsByPath.reserve(result.previewItems.size());

    for (const OneLinerEngine::PreviewItem& previewItem : result.previewItems) {
        QTreeWidgetItem* item = new QTreeWidgetItem({ QString() });
        item->setData(0, Qt::UserRole, previewItem.rawText);
        item->setToolTip(0, previewItem.path);

        QTreeWidgetItem* parentItem = itemsByPath.value(previewItem.parentPath, nullptr);
        if (parentItem != nullptr) {
            parentItem->addChild(item);
        } else {
            nativeTree->addTopLevelItem(item);
        }

        itemsByPath.insert(previewItem.path, item);
    }

    for (const OneLinerEngine::PreviewItem& previewItem : result.previewItems) {
        QTreeWidgetItem* item = itemsByPath.value(previewItem.path, nullptr);
        if (item == nullptr) {
            continue;
        }

        const int rowHeight = resolvedRowHeight();
        item->setSizeHint(0, QSize(0, rowHeight));

        PreviewNameItemWidget* nameWidget = new PreviewNameItemWidget(
            mayaNodePixmap(previewItem.iconTypeName),
            previewItem.name,
            _scale,
            nativeTree);
        nameWidget->setFont(previewFont(_scale));
        nameWidget->setMinimumHeight(rowHeight);
        nativeTree->setItemWidget(item, 0, nameWidget);
    }

    expandAllItems();
    nativeTree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    nativeTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    if (QScrollBar* scrollBar = nativeTree->verticalScrollBar()) {
        scrollBar->setValue(scrollBar->minimum());
    }

    const int visibleItemCount = qMin(_itemCount, kPreviewMaxVisibleItems);
    const QMargins margins = layout() != nullptr ? layout()->contentsMargins() : QMargins();
    _previewHeight = visibleItemCount > 0
        ? margins.top() + margins.bottom() + visibleItemCount * resolvedRowHeight()
        : 0;

    setMinimumHeight(_previewHeight);
    setMaximumHeight(_previewHeight);
    setFixedHeight(_previewHeight);
}

int OneLinerPreviewTree::previewHeight() const
{
    return _previewHeight;
}

void OneLinerPreviewTree::setPanelBackground(const OneQtBackground& background)
{
    _background.setBrush(background.brush());
    _background.setPadding(background.padding());
    _background.setCornerRadius(background.cornerRadius());
    _background.setBorderBrush(background.borderBrush());
    _background.setBorderWidth(background.borderWidth());
    _background.setScale(_scale);
    update();
}

QTreeWidget* OneLinerPreviewTree::treeWidget() const
{
    return _tree != nullptr ? _tree->treeWidget() : nullptr;
}

bool OneLinerPreviewTree::hasItems() const
{
    return _itemCount > 0;
}

bool OneLinerPreviewTree::scrollEnabled() const
{
    return _scrollEnabled;
}

void OneLinerPreviewTree::setItemActivatedHandler(const std::function<void(const QString&)>& handler)
{
    _itemActivatedHandler = handler;
}

void OneLinerPreviewTree::expandAllItems()
{
    QTreeWidget* nativeTree = treeWidget();
    std::function<void(QTreeWidgetItem*)> expandRecursively = [&](QTreeWidgetItem* item) {
        if (item == nullptr) {
            return;
        }
        item->setExpanded(true);
        for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
            expandRecursively(item->child(childIndex));
        }
    };

    for (int index = 0; index < nativeTree->topLevelItemCount(); ++index) {
        expandRecursively(nativeTree->topLevelItem(index));
    }
}

int OneLinerPreviewTree::resolvedRowHeight() const
{
    // 行高由 rebuildPreview 中 item->setSizeHint 主动设定为固定值，
    // 这里必须返回同一个固定值，而不能依赖 sizeHintForRow(0)：
    // 后者需要 tree 完成一次布局后才准确，首次显示时会偏大，
    // 导致预览面板底部出现多余空白（直到下一次更新才修正）。
    return qMax(22, qRound(kPreviewItemHeight * _scale));
}

void OneLinerPreviewTree::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    if (!rect().isValid()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawPixmap(rect(), _background.toPixmap(size()));
}