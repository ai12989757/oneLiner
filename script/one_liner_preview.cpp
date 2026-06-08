#include "one_liner_preview.h"

#include "one_scroll_bar.h"
#include "oneqt_brush.h"

#include <QAbstractItemView>
#include <QFont>
#include <QHash>
#include <QHeaderView>
#include <QLayout>
#include <QScrollBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace {

constexpr int kPreviewItemHeight = 22;
constexpr int kPreviewMaxVisibleItems = 10;

QFont previewFont(qreal scale)
{
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(qRound(12 * scale));
    return font;
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
    hoverScrollThumb.setBrush(OneQtBrush::fromColor(QColor(255, 255, 255, 50)));
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

} // namespace

OneLinerPreviewTree::OneLinerPreviewTree(QWidget* parent)
    : OneQtTree(parent)
    , _previewHeight(0)
    , _itemCount(0)
    , _scrollEnabled(false)
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setToolTip(QStringLiteral("预览列表。双击任意项可将其文本写回输入框。"));
    setStatusTip(QStringLiteral("双击预览项可快速把该文本填入输入框。"));

    QTreeWidget* nativeTree = treeWidget();
    nativeTree->setColumnCount(2);
    nativeTree->setHeaderLabels({ QStringLiteral("name"), QStringLiteral("type") });
    nativeTree->setHeaderHidden(true);
    nativeTree->setSelectionMode(QAbstractItemView::NoSelection);
    nativeTree->setExpandsOnDoubleClick(false);
    nativeTree->setSortingEnabled(false);
    nativeTree->setUniformRowHeights(true);
    nativeTree->setAllColumnsShowFocus(false);
    nativeTree->setRootIsDecorated(true);
    nativeTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    nativeTree->header()->setStretchLastSection(false);
    nativeTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    nativeTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    QObject::connect(nativeTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, int) {
        if (item == nullptr || !_itemActivatedHandler) {
            return;
        }
        _itemActivatedHandler(item->data(0, Qt::UserRole).toString());
    });

    applyPreviewScrollBarStyle(verticalOneScrollBar());
    applyPreviewScrollBarStyle(horizontalOneScrollBar());
}

void OneLinerPreviewTree::applyScale(qreal scale)
{
    OneQtTree::setScale(scale);

    QTreeWidget* nativeTree = treeWidget();
    nativeTree->setFont(previewFont(scale));
    nativeTree->setIndentation(qMax(12, qRound(16 * scale)));

    if (OneQtScrollBar* verticalScrollBar = verticalOneScrollBar()) {
        verticalScrollBar->setThickness(4);
        verticalScrollBar->setMinThumbLength(qMax(18, qRound(20 * scale)));
    }
    if (OneQtScrollBar* horizontalScrollBar = horizontalOneScrollBar()) {
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
    clearItems();

    _itemCount = result.previewItems.size();
    _scrollEnabled = _itemCount > kPreviewMaxVisibleItems;

    QHash<QString, QTreeWidgetItem*> itemsByPath;
    itemsByPath.reserve(result.previewItems.size());

    for (const OneLinerEngine::PreviewItem& previewItem : result.previewItems) {
        QTreeWidgetItem* item = new QTreeWidgetItem({ previewItem.name, previewItem.typeName });
        item->setData(0, Qt::UserRole, previewItem.rawText);
        item->setToolTip(0, previewItem.path);
        item->setToolTip(1, previewItem.typeName);

        QTreeWidgetItem* parentItem = itemsByPath.value(previewItem.parentPath, nullptr);
        if (parentItem != nullptr) {
            parentItem->addChild(item);
        } else {
            nativeTree->addTopLevelItem(item);
        }

        itemsByPath.insert(previewItem.path, item);
    }

    expandAllItems();
    nativeTree->setVerticalScrollBarPolicy(_scrollEnabled ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
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
    const QTreeWidget* nativeTree = treeWidget();
    const int hintedHeight = nativeTree->topLevelItemCount() > 0 ? nativeTree->sizeHintForRow(0) : 0;
    const int fallbackHeight = qRound(kPreviewItemHeight * scale());
    return qMax(fallbackHeight, hintedHeight);
}