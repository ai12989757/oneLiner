#pragma once

#include <maya/MObject.h>

#include <QString>
#include <QStringList>
#include <QVector>

class QWidget;

class OneLinerEngine
{
public:
    enum class ScopeMode {
        Selected,
        Hierarchy,
        All,
    };

    struct PreviewResult {
        QStringList items;
        bool selectionOnly = false;
    };

    struct RenameTarget {
        MObject object;
        QString path;
        QString currentName;
        bool isDag = false;
    };

    static PreviewResult preview(const QString& rule);
    static PreviewResult preview(const QString& rule, ScopeMode forcedMode, bool useForcedMode);
    static bool execute(const QString& rule);
    static bool execute(const QString& rule, ScopeMode forcedMode, bool useForcedMode);
    static bool renamePastedPrefix();
    static bool isTransformLike(const MObject& object);

private:
    enum class SelectorMode {
        None,
        Contains,
        Prefix,
        Suffix,
    };

    struct ParsedRule {
        QString rawRule;
        QString cleanRule;
        ScopeMode scopeMode = ScopeMode::Selected;
        SelectorMode selectorMode = SelectorMode::None;
        QString selectorText;
        bool selectionOnly = false;
    };

    static ParsedRule parseRule(const QString& rule, ScopeMode forcedMode, bool useForcedMode);
    static QVector<RenameTarget> collectTargets(ScopeMode mode);
    static QVector<RenameTarget> collectSelectionTargets();
    static QVector<RenameTarget> collectHierarchyTargets();
    static QVector<RenameTarget> collectAllTargets();
    static QVector<RenameTarget> collectPatternTargets(SelectorMode mode, const QString& pattern);
    static QStringList buildPreviewItems(const ParsedRule& parsed, QVector<RenameTarget>* outTargets = nullptr);
    static QStringList buildRenamedItems(const ParsedRule& parsed, QVector<RenameTarget>* outTargets);
    static QString resolvePattern(SelectorMode mode, const QString& pattern);
    static QString applyNumberPattern(QString text, int index);
    static QString uniqueNameWithDigits(QString name);
    static QString shortName(const QString& name);
    static bool pathIsAncestorOf(const QString& parentPath, const QString& childPath);
    static bool renameObject(const RenameTarget& target, const QString& newName);
    static bool selectTargets(const QVector<RenameTarget>& targets);
    static int nameMatchCount(const QString& name);
    static bool nameExists(const QString& name);
};