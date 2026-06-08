#pragma once

#include "one_liner_engine.h"

#include "one_tree.h"

#include <functional>

class OneLinerPreviewTree : public OneQtTree
{
public:
    explicit OneLinerPreviewTree(QWidget* parent = nullptr);

    void applyScale(qreal scale);
    void rebuildPreview(const OneLinerEngine::PreviewResult& result);

    int previewHeight() const;
    bool hasItems() const;
    bool scrollEnabled() const;

    void setItemActivatedHandler(const std::function<void(const QString&)>& handler);

private:
    void expandAllItems();
    int resolvedRowHeight() const;

    int _previewHeight;
    int _itemCount;
    bool _scrollEnabled;
    std::function<void(const QString&)> _itemActivatedHandler;
};