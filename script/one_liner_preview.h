#pragma once

#include "one_liner_engine.h"

#include "one_background.h"
#include "one_tree.h"

#include <functional>

class QTreeWidget;

class OneLinerPreviewTree : public QWidget
{
public:
    explicit OneLinerPreviewTree(QWidget* parent = nullptr);

    void applyScale(qreal scale);
    void rebuildPreview(const OneLinerEngine::PreviewResult& result);

    void setPanelBackground(const OneQtBackground& background);
    QTreeWidget* treeWidget() const;

    int previewHeight() const;
    bool hasItems() const;
    bool scrollEnabled() const;

    void setItemActivatedHandler(const std::function<void(const QString&)>& handler);

private:
    void expandAllItems();
    int resolvedRowHeight() const;
    void paintEvent(QPaintEvent* event) override;

    OneQtTree* _tree;
    OneQtBackground _background;
    qreal _scale;
    int _previewHeight;
    int _itemCount;
    bool _scrollEnabled;
    std::function<void(const QString&)> _itemActivatedHandler;
};