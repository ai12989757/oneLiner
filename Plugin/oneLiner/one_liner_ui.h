#pragma once

#include "one_liner_engine.h"

#include "one_background.h"

#include <QPointer>
#include <QWidget>

class QDialog;
class QLineEdit;
class OneQtList;

class OneLinerWindow : public QWidget
{
public:
    explicit OneLinerWindow(QWidget* parent = nullptr);
    ~OneLinerWindow() override;

    static void showWindow(QWidget* mayaParent = nullptr);
    static void closeWindow();

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void refreshPreview();
    void executeRule();
    void showToolsMenu();
    void showHelpDialog();
    void positionNearCursor();
    void updateScaleFromMaya();
    void updateLayoutMetrics();
    void rebuildPreviewList(const QStringList& items, bool selectionOnly);

    qreal _scale;
    int _maxPreviewCount;
    bool _previewScrollEnabled;
    bool _dragging;
    QPoint _dragStart;
    OneQtBackground _inputBackground;
    OneQtBackground _previewBackground;
    QLineEdit* _lineEdit;
    OneQtList* _previewList;
    QPointer<QDialog> _helpDialog;
};