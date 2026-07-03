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

    struct PreviewItem {
        QString displayText;
        QString rawText;
        QString name;
        QString typeName;
        QString iconTypeName;
        QString path;
        QString parentPath;
        bool isDag = false;
        bool isHidden = false;
        bool isLocked = false;
    };

    struct PreviewResult {
        QStringList items;
        QStringList rawItems;
        QVector<PreviewItem> previewItems;
        bool selectionOnly = false;
    };

    struct RenameTarget {
        MObject object;
        QString path;
        QString currentName;
        bool isDag = false;
        bool isLocked = false;
    };

    struct RenameOperation {
        MObject object;
        QString oldName;
        QString newName;
        bool isDag = false;
        bool isLocked = false;
    };

    struct ExecutePlan {
        QVector<RenameTarget> selectionTargets;
        QVector<RenameOperation> renameOperations;
        bool selectionOnly = false;
        bool noop = false;
    };

    static PreviewResult preview(const QString& rule);
    static PreviewResult preview(const QString& rule, ScopeMode forcedMode, bool useForcedMode);
    static bool execute(const QString& rule);
    static bool execute(const QString& rule, ScopeMode forcedMode, bool useForcedMode);
    static bool renamePastedPrefix();
    static ExecutePlan buildExecutePlan(const QString& rule, ScopeMode forcedMode, bool useForcedMode);
    static QVector<RenameOperation> buildClearPastedOperations();
    static bool applyRenameOperations(const QVector<RenameOperation>& operations, bool useNewNames);
    static bool selectTargets(const QVector<RenameTarget>& targets);
    static bool isTransformLike(const MObject& object);

private:
    struct ParsedRule {
        QString originalRule;
        QString rawRule;
        QString cleanRule;
        ScopeMode scopeMode = ScopeMode::Selected;
        QString wildcardPattern;
        QStringList typeFilters;
        bool wildcardSelection = false;
        bool flagsMode = false;
        bool includeHierarchy = false;
        bool includeShapes = false;
        bool selectionOnly = false;
    };

    static ParsedRule parseRule(const QString& rule, ScopeMode forcedMode, bool useForcedMode);
    static QVector<RenameTarget> collectTargets(const ParsedRule& parsed);
    static QVector<RenameTarget> collectTargets(ScopeMode mode);
    static QVector<RenameTarget> collectSelectionTargets();
    static QVector<RenameTarget> collectHierarchyTargets(bool includeShapes = false);
    static QVector<RenameTarget> collectAllTargets();
    static QVector<RenameTarget> collectWildcardTargets(const QString& pattern);
    static QVector<RenameTarget> filterTargetsByType(const QVector<RenameTarget>& targets, const QStringList& typeFilters);
    static QStringList buildPreviewItems(const ParsedRule& parsed, QVector<RenameTarget>* outTargets = nullptr, QStringList* outRawItems = nullptr);
    static QStringList buildRenamedItems(const ParsedRule& parsed, QVector<RenameTarget>* outTargets, bool reverseForRename = true);
    static QStringList formatPreviewItems(const QVector<RenameTarget>& targets, const QStringList& displayNames);
    static QString applyNumberPattern(QString text, int index);
    static QString uniqueNameWithDigits(QString name);
    static QString shortName(const QString& name);
    static bool pathIsAncestorOf(const QString& parentPath, const QString& childPath);
    static bool renameObject(const RenameTarget& target, const QString& newName);
    static int nameMatchCount(const QString& name);
    static bool nameExists(const QString& name);
};