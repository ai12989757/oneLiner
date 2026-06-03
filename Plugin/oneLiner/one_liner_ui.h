#pragma once

#include "one_liner_engine.h"

#include "one_background.h"

#include <QString>
#include <QStringList>
#include <QPointer>
#include <QWidget>

class QDialog;
class QLineEdit;
class OneQtList;
class OneQtMenu;
class QVBoxLayout;

class OneLinerWindow : public QWidget
{
public:
    explicit OneLinerWindow(QWidget* parent = nullptr);
    ~OneLinerWindow() override;

    static void showWindow(QWidget* mayaParent = nullptr);
    static void closeWindow(bool immediate = false);

protected:
    void closeEvent(QCloseEvent* event) override;
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
    void applyWindowLayout(int previewHeight);
    void rebuildPreviewList(const QStringList& items, const QStringList& rawItems, bool selectionOnly);
    void rememberHistory(const QString& text);
    bool stepHistory(int direction);

    qreal _scale;
    int _maxPreviewCount;
    bool _previewEnabled;
    bool _autoCloseEnabled;
    bool _wildcardIncludeShapes;
    bool _previewScrollEnabled;
    bool _closeAfterMenuAction;
    bool _restoreFocusAfterMenu;
    bool _isClosing;
    int _previewTopInset;
    int _historyIndex;
    bool _dragging;
    QPoint _dragStart;
    QString _historyDraft;
    OneQtBackground _inputBackground;
    OneQtBackground _previewBackground;
    QVBoxLayout* _rootLayout;
    QLineEdit* _lineEdit;
    OneQtList* _previewList;
    QPointer<OneQtMenu> _toolsMenu;
    QPointer<QDialog> _helpDialog;
};