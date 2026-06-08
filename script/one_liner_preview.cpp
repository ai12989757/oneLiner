#include "one_liner_preview.h"

#include "one_scroll_bar.h"
#include "oneqt_brush.h"

#include <QAbstractItemView>
#include <QFont>
#include <QFontMetrics>
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
constexpr qreal kPreviewIconHeightRatio = 0.6;

QFont previewFont(qreal scale)
{
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(qRound(12 * scale));
    return font;
}

int previewTreeIndentation(const QFontMetrics& metrics)
{
    return qMax(1, qRound(metrics.height() * 0.8));
}

class PreviewNameItemWidget : public QWidget
{
public:
    PreviewNameItemWidget(const QPixmap& iconPixmap, const QString& text, int leadingInset, qreal scale, QWidget* parent = nullptr)
        : QWidget(parent)
        , _iconPixmap(iconPixmap)
        , _text(text)
        , _leadingInset(leadingInset)
        , _scale(scale)
    {
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAutoFillBackground(false);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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

        int textLeft = _leadingInset;
        if (!_iconPixmap.isNull()) {
            const QPixmap scaledPixmap = _iconPixmap.scaled(
                resolvedIconSize,
                resolvedIconSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
            const int iconX = _leadingInset + contentGap;
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
    int _leadingInset;
    qreal _scale;
};

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

void applyPreviewTreeItemStyle(QTreeWidget* treeWidget)
{
    if (treeWidget == nullptr) {
        return;
    }

    treeWidget->setStyleSheet(
        treeWidget->styleSheet()
        + QStringLiteral(
            "QTreeView::item {"
            " padding: 0px;"
            " margin: 0px;"
            "}"
            "QTreeView::item:hover {"
            " padding: 0px;"
            " margin: 0px;"
            "}"
            "QTreeView::item:selected {"
            " padding: 0px;"
            " margin: 0px;"
            "}"));
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

    applyPreviewScrollBarStyle(verticalOneScrollBar());
    applyPreviewScrollBarStyle(horizontalOneScrollBar());
}

void OneLinerPreviewTree::applyScale(qreal scale)
{
    OneQtTree::setScale(scale);

    QTreeWidget* nativeTree = treeWidget();
    applyPreviewTreeItemStyle(nativeTree);
    const QFont treeFont = previewFont(scale);
    nativeTree->setFont(treeFont);
    // Item widgets rely on the native tree indentation for hierarchy placement.
    // Keep it aligned with OneTree's own branch offset computation.
    nativeTree->setIndentation(previewTreeIndentation(QFontMetrics(treeFont)));

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

        const int rowHeight = qMax(22, qRound(kPreviewItemHeight * scale()));
        item->setSizeHint(0, QSize(0, rowHeight));

        PreviewNameItemWidget* nameWidget = new PreviewNameItemWidget(
            mayaNodePixmap(previewItem.iconTypeName),
            previewItem.name,
            0,
            scale(),
            nativeTree);
        nameWidget->setFont(nativeTree->font());
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