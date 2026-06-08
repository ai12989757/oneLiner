#include "one_liner_engine.h"

#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItSelectionList.h>
#include <maya/MObjectHandle.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <QRegularExpression>
#include <QSet>

namespace {

constexpr const char* kWildcardIncludeShapesOptionVar = "oneLinerWildcardIncludeShapes";

QString indexToLetters(int index);

MString toMString(const QString& value)
{
    return MString(value.toUtf8().constData());
}

QString toQString(const MString& value)
{
    return QString::fromUtf8(value.asChar());
}

QString quoteMel(const QString& value)
{
    QString escaped = value;
    escaped.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    escaped.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

bool readBoolOptionVar(const char* optionVarName, bool defaultValue)
{
    int exists = 0;
    if (MGlobal::executeCommand(MString("optionVar -exists \"") + optionVarName + "\"", exists, false, false) != MS::kSuccess || exists == 0) {
        return defaultValue;
    }

    int value = defaultValue ? 1 : 0;
    if (MGlobal::executeCommand(MString("optionVar -q \"") + optionVarName + "\"", value, false, false) != MS::kSuccess) {
        return defaultValue;
    }

    return value != 0;
}

bool wildcardHasSearchText(const QString& pattern)
{
    QString candidate = pattern;
    candidate.remove(QChar(u'*'));
    candidate.remove(QChar(u'?'));
    return !candidate.trimmed().isEmpty();
}

QString normalizeRuleInput(QString rule)
{
    rule.replace(QChar(u'！'), QChar(u'!'));
    rule.replace(QChar(u'？'), QChar(u'?'));
    rule.replace(QChar(u'＃'), QChar(u'#'));
    rule.replace(QChar(u'＠'), QChar(u'@'));
    rule.replace(QChar(u'＊'), QChar(u'*'));
    rule.replace(QChar(u'＞'), QChar(u'>'));
    rule.replace(QChar(u'，'), QChar(u','));
    rule.replace(QChar(u'　'), QChar(u' '));
    return rule;
}

QStringList splitReplacementTerms(const QString& text)
{
    return text.split(QRegularExpression(QStringLiteral("[\\s,，]+")), Qt::SkipEmptyParts);
}

QString applyReplacementPattern(QString text, int index)
{
    int start = 1;
    const int startMarker = text.indexOf(QStringLiteral("//"));
    if (startMarker >= 0) {
        bool ok = false;
        const int parsedStart = text.mid(startMarker + 2).toInt(&ok);
        if (ok) {
            start = parsedStart;
        }
        text = text.left(startMarker);
    }

    const int number = index + start;
    if (text.contains(QChar(u'#'))) {
        const int padding = text.count(QChar(u'#'));
        const QString token(padding, QChar(u'#'));
        text.replace(token, QStringLiteral("%1").arg(number, padding, 10, QChar(u'0')));
    }

    if (text.contains(QChar(u'@'))) {
        const int padding = text.count(QChar(u'@'));
        const QString token(padding, QChar(u'@'));
        text.replace(token, indexToLetters(index));
    }
    return text;
}

QVector<QPair<QString, QString>> replacementPairsForRule(const QString& rule, int index)
{
    QVector<QPair<QString, QString>> pairs;
    const QString normalizedRule = normalizeRuleInput(rule).trimmed();
    if (!normalizedRule.contains(QChar(u'>'))) {
        return pairs;
    }

    const QStringList tokens = splitReplacementTerms(normalizedRule);
    bool allTokensArePairs = !tokens.isEmpty();
    for (const QString& token : tokens) {
        if (!token.contains(QChar(u'>'))) {
            allTokensArePairs = false;
            break;
        }
    }

    if (allTokensArePairs) {
        pairs.reserve(tokens.size());
        for (const QString& token : tokens) {
            const int splitIndex = token.indexOf(QChar(u'>'));
            const QString oldWord = token.left(splitIndex).trimmed();
            const QString newWord = applyReplacementPattern(token.mid(splitIndex + 1).trimmed(), index);
            if (!oldWord.isEmpty()) {
                pairs.push_back(qMakePair(oldWord, newWord));
            }
        }
        return pairs;
    }

    const int splitIndex = normalizedRule.indexOf(QChar(u'>'));
    const QStringList oldWords = splitReplacementTerms(normalizedRule.left(splitIndex));
    const QStringList newWords = splitReplacementTerms(normalizedRule.mid(splitIndex + 1));
    const int pairCount = qMin(oldWords.size(), newWords.size());
    pairs.reserve(pairCount);
    for (int pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
        const QString oldWord = oldWords[pairIndex].trimmed();
        const QString newWord = applyReplacementPattern(newWords[pairIndex].trimmed(), index);
        if (!oldWord.isEmpty()) {
            pairs.push_back(qMakePair(oldWord, newWord));
        }
    }
    return pairs;
}

bool isNumericTrimRule(const QString& rule)
{
    static const QRegularExpression pattern(QStringLiteral("^--?\\d+$"));
    return pattern.match(rule).hasMatch();
}

bool isFlagsRule(const QString& rule)
{
    if (!rule.startsWith(QChar(u'-'))) {
        return false;
    }

    const QStringList tokens = rule.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        return false;
    }

    const QString& firstToken = tokens.front();
    return firstToken == QStringLiteral("-h")
        || firstToken == QStringLiteral("-s")
        || firstToken == QStringLiteral("-type");
}

bool pathIsAncestorPath(const QString& parentPath, const QString& childPath)
{
    if (parentPath.isEmpty() || childPath.isEmpty() || parentPath == childPath) {
        return false;
    }
    return childPath.startsWith(parentPath + QStringLiteral("|"), Qt::CaseSensitive);
}

int dagDepth(const QString& path)
{
    const int pipeCount = path.count(QChar(u'|'));
    return path.startsWith(QChar(u'|')) ? qMax(0, pipeCount - 1) : pipeCount;
}

QString previewLabelForTarget(const OneLinerEngine::RenameTarget& target, const QString& displayName, int baseDagDepth)
{
    if (!target.isDag) {
        return displayName;
    }

    const int relativeDepth = qMax(0, dagDepth(target.path) - baseDagDepth);
    if (relativeDepth <= 0) {
        return displayName;
    }

    QString prefix;
    prefix.reserve(relativeDepth * 2);
    for (int depthIndex = 0; depthIndex < relativeDepth; ++depthIndex) {
        prefix += QChar(0x2022);
        prefix += QChar(u' ');
    }
    return prefix + displayName;
}

void sortPreviewEntries(QVector<OneLinerEngine::RenameTarget>& targets, QStringList& displayNames, QStringList* rawNames)
{
    if (targets.size() != displayNames.size()) {
        return;
    }
    if (rawNames != nullptr && rawNames->size() != displayNames.size()) {
        return;
    }

    struct PreviewEntry {
        OneLinerEngine::RenameTarget target;
        QString displayName;
        QString rawName;
    };

    QVector<PreviewEntry> entries;
    entries.reserve(targets.size());
    for (int index = 0; index < targets.size(); ++index) {
        PreviewEntry entry;
        entry.target = targets[index];
        entry.displayName = displayNames[index];
        entry.rawName = rawNames != nullptr ? rawNames->value(index) : displayNames[index];
        entries.push_back(entry);
    }

    QVector<int> parentIndex(entries.size(), -1);
    QVector<QVector<int>> childIndices(entries.size());
    QVector<int> dagRoots;
    QVector<int> nonDagEntries;

    for (int index = 0; index < entries.size(); ++index) {
        if (!entries[index].target.isDag) {
            nonDagEntries.push_back(index);
            continue;
        }

        int nearestParentIndex = -1;
        int nearestParentDepth = -1;
        for (int candidateIndex = 0; candidateIndex < entries.size(); ++candidateIndex) {
            if (candidateIndex == index || !entries[candidateIndex].target.isDag) {
                continue;
            }

            if (!pathIsAncestorPath(entries[candidateIndex].target.path, entries[index].target.path)) {
                continue;
            }

            const int candidateDepth = dagDepth(entries[candidateIndex].target.path);
            if (candidateDepth > nearestParentDepth) {
                nearestParentDepth = candidateDepth;
                nearestParentIndex = candidateIndex;
            }
        }

        parentIndex[index] = nearestParentIndex;
        if (nearestParentIndex >= 0) {
            childIndices[nearestParentIndex].push_back(index);
        } else {
            dagRoots.push_back(index);
        }
    }

    QVector<PreviewEntry> orderedEntries;
    orderedEntries.reserve(entries.size());

    std::function<void(int)> appendSubtree = [&](int entryIndex) {
        orderedEntries.push_back(entries[entryIndex]);
        for (int childIndex : childIndices[entryIndex]) {
            appendSubtree(childIndex);
        }
    };

    for (int rootIndex : dagRoots) {
        appendSubtree(rootIndex);
    }
    for (int entryIndex : nonDagEntries) {
        orderedEntries.push_back(entries[entryIndex]);
    }

    if (orderedEntries.size() != entries.size()) {
        return;
    }

    for (int index = 0; index < orderedEntries.size(); ++index) {
        targets[index] = orderedEntries[index].target;
        displayNames[index] = orderedEntries[index].displayName;
        if (rawNames != nullptr) {
            (*rawNames)[index] = orderedEntries[index].rawName;
        }
    }
}

QString indexToLetters(int index)
{
    QString result;
    int value = qMax(0, index);
    do {
        const int remainder = value % 26;
        result.prepend(QChar(u'A' + remainder));
        value = value / 26 - 1;
    } while (value >= 0);
    return result;
}

bool appendUniqueTarget(QVector<OneLinerEngine::RenameTarget>& targets, const OneLinerEngine::RenameTarget& target)
{
    for (const OneLinerEngine::RenameTarget& existing : targets) {
        if (existing.path == target.path) {
            return false;
        }
    }
    targets.push_back(target);
    return true;
}

bool shouldIncludeDagChild(const MObject& childObject, bool includeShapes)
{
    if (childObject.hasFn(MFn::kTransform) || childObject.hasFn(MFn::kJoint)) {
        return true;
    }

    if (includeShapes && childObject.hasFn(MFn::kShape)) {
        return true;
    }

    return false;
}

void appendDescendants(const MDagPath& parentPath, QVector<OneLinerEngine::RenameTarget>& targets, bool includeShapes)
{
    MStatus status;
    MFnDagNode dagNode(parentPath, &status);
    if (status != MS::kSuccess) {
        return;
    }

    for (unsigned int index = 0; index < dagNode.childCount(); ++index) {
        MObject childObject = dagNode.child(index, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        MDagPath childPath(parentPath);
        status = childPath.push(childObject);
        if (status != MS::kSuccess) {
            continue;
        }

        if (shouldIncludeDagChild(childObject, includeShapes)) {
            OneLinerEngine::RenameTarget target;
            target.object = childObject;
            target.path = toQString(childPath.fullPathName());
            target.currentName = toQString(childPath.partialPathName());
            target.isDag = true;
            appendUniqueTarget(targets, target);
        }

        appendDescendants(childPath, targets, includeShapes);
    }
}

QString targetTypeName(const OneLinerEngine::RenameTarget& target)
{
    MStatus status;
    MFnDependencyNode dependencyNode(target.object, &status);
    if (status != MS::kSuccess) {
        return QString();
    }
    return toQString(dependencyNode.typeName());
}

QString targetIconTypeName(const OneLinerEngine::RenameTarget& target)
{
    const QString targetType = targetTypeName(target);
    if (!target.isDag || !OneLinerEngine::isTransformLike(target.object)) {
        return targetType;
    }

    MStatus status;
    MFnDagNode dagNode(target.object, &status);
    if (status != MS::kSuccess) {
        return targetType;
    }

    QString fallbackShapeType;
    for (unsigned int childIndex = 0; childIndex < dagNode.childCount(); ++childIndex) {
        MObject childObject = dagNode.child(childIndex, &status);
        if (status != MS::kSuccess || !childObject.hasFn(MFn::kShape)) {
            continue;
        }

        MFnDagNode childDagNode(childObject, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        const QString shapeType = toQString(childDagNode.typeName());
        if (!childDagNode.isIntermediateObject()) {
            return shapeType;
        }
        if (fallbackShapeType.isEmpty()) {
            fallbackShapeType = shapeType;
        }
    }

    return fallbackShapeType.isEmpty() ? targetType : fallbackShapeType;
}

QString targetParentPath(const OneLinerEngine::RenameTarget& target)
{
    if (!target.isDag) {
        return QString();
    }

    const int splitIndex = target.path.lastIndexOf(QChar(u'|'));
    if (splitIndex <= 0) {
        return QString();
    }
    return target.path.left(splitIndex);
}

} // namespace

OneLinerEngine::PreviewResult OneLinerEngine::preview(const QString& rule)
{
    return preview(rule, ScopeMode::Selected, false);
}

OneLinerEngine::PreviewResult OneLinerEngine::preview(const QString& rule, ScopeMode forcedMode, bool useForcedMode)
{
    const ParsedRule parsed = parseRule(rule, forcedMode, useForcedMode);

    PreviewResult result;
    QVector<RenameTarget> previewTargets;
    result.selectionOnly = parsed.selectionOnly;
    result.items = buildPreviewItems(parsed, &previewTargets, &result.rawItems);
    result.previewItems.reserve(result.items.size());
    for (int index = 0; index < result.items.size(); ++index) {
        const RenameTarget& target = previewTargets.value(index);
        PreviewItem previewItem;
        previewItem.displayText = result.items.at(index);
        previewItem.rawText = result.rawItems.value(index, result.items.at(index));
        previewItem.name = previewItem.rawText;
        previewItem.typeName = targetTypeName(target);
        previewItem.iconTypeName = targetIconTypeName(target);
        previewItem.path = target.path;
        previewItem.parentPath = targetParentPath(target);
        previewItem.isDag = target.isDag;
        result.previewItems.push_back(previewItem);
    }
    return result;
}

bool OneLinerEngine::execute(const QString& rule)
{
    return execute(rule, ScopeMode::Selected, false);
}

bool OneLinerEngine::execute(const QString& rule, ScopeMode forcedMode, bool useForcedMode)
{
    const ExecutePlan plan = buildExecutePlan(rule, forcedMode, useForcedMode);
    if (plan.noop) {
        return true;
    }

    if (plan.selectionOnly) {
        return selectTargets(plan.selectionTargets);
    }

    return applyRenameOperations(plan.renameOperations, true);
}

bool OneLinerEngine::renamePastedPrefix()
{
    const QVector<RenameOperation> operations = buildClearPastedOperations();
    if (operations.isEmpty()) {
        return true;
    }

    return applyRenameOperations(operations, true);
}

OneLinerEngine::ExecutePlan OneLinerEngine::buildExecutePlan(const QString& rule, ScopeMode forcedMode, bool useForcedMode)
{
    const ParsedRule parsed = parseRule(rule, forcedMode, useForcedMode);

    ExecutePlan plan;
    if (parsed.rawRule.isEmpty() || parsed.rawRule == QStringLiteral("/")) {
        plan.noop = true;
        return plan;
    }

    QVector<RenameTarget> targets;
    const QStringList previewItems = buildPreviewItems(parsed, &targets);
    Q_UNUSED(previewItems)

    if (parsed.selectionOnly) {
        plan.selectionOnly = true;
        plan.selectionTargets = targets;
        return plan;
    }

    if (parsed.flagsMode && parsed.cleanRule.isEmpty()) {
        plan.selectionOnly = true;
        plan.selectionTargets = targets;
        return plan;
    }

    const QStringList renamedItems = buildRenamedItems(parsed, &targets);
    if (targets.size() != renamedItems.size()) {
        return plan;
    }

    plan.renameOperations.reserve(targets.size());
    for (int index = 0; index < targets.size(); ++index) {
        RenameOperation operation;
        operation.object = targets[index].object;
        operation.oldName = shortName(targets[index].currentName);
        operation.newName = shortName(renamedItems[index]);
        operation.isDag = targets[index].isDag;
        if (!operation.newName.isEmpty() && operation.oldName != operation.newName) {
            plan.renameOperations.push_back(operation);
        }
    }
    return plan;
}

QVector<OneLinerEngine::RenameOperation> OneLinerEngine::buildClearPastedOperations()
{
    const QVector<RenameTarget> targets = collectWildcardTargets(QStringLiteral("pasted__*"));
    QVector<RenameOperation> operations;
    operations.reserve(targets.size());

    for (const RenameTarget& target : targets) {
        QString newName = shortName(target.currentName);
        newName.replace(QStringLiteral("pasted__"), QString(), Qt::CaseSensitive);
        newName = uniqueNameWithDigits(newName);

        RenameOperation operation;
        operation.object = target.object;
        operation.oldName = shortName(target.currentName);
        operation.newName = shortName(newName);
        operation.isDag = target.isDag;
        if (!operation.newName.isEmpty() && operation.oldName != operation.newName) {
            operations.push_back(operation);
        }
    }

    return operations;
}

bool OneLinerEngine::applyRenameOperations(const QVector<RenameOperation>& operations, bool useNewNames)
{
    bool success = true;
    if (useNewNames) {
        for (const RenameOperation& operation : operations) {
            RenameTarget target;
            target.object = operation.object;
            target.currentName = operation.oldName;
            target.isDag = operation.isDag;
            if (!renameObject(target, operation.newName)) {
                success = false;
            }
        }
        return success;
    }

    for (auto iterator = operations.crbegin(); iterator != operations.crend(); ++iterator) {
        RenameTarget target;
        target.object = iterator->object;
        target.currentName = iterator->newName;
        target.isDag = iterator->isDag;
        if (!renameObject(target, iterator->oldName)) {
            success = false;
        }
    }
    return success;
}

OneLinerEngine::ParsedRule OneLinerEngine::parseRule(const QString& rule, ScopeMode forcedMode, bool useForcedMode)
{
    ParsedRule parsed;
    parsed.originalRule = rule;
    parsed.rawRule = normalizeRuleInput(rule).trimmed();
    parsed.cleanRule = parsed.rawRule;

    if (isFlagsRule(parsed.rawRule) && !isNumericTrimRule(parsed.rawRule)) {
        parsed.flagsMode = true;
        parsed.cleanRule.clear();

        const QStringList tokens = parsed.rawRule.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        for (int index = 0; index < tokens.size(); ++index) {
            const QString& token = tokens[index];
            if (token == QStringLiteral("-h")) {
                parsed.includeHierarchy = true;
                continue;
            }
            if (token == QStringLiteral("-s")) {
                parsed.includeShapes = true;
                continue;
            }
            if (token == QStringLiteral("-type")) {
                while (index + 1 < tokens.size() && !tokens[index + 1].startsWith(QChar(u'-'))) {
                    parsed.typeFilters.push_back(tokens[++index]);
                }
            }
        }
    } else if (parsed.rawRule.contains(QChar(u'*')) || parsed.rawRule.contains(QChar(u'?'))) {
        parsed.wildcardSelection = true;
        parsed.selectionOnly = true;
        parsed.wildcardPattern = wildcardHasSearchText(parsed.rawRule) ? parsed.rawRule : QString();
        parsed.cleanRule.clear();
    } else {
        parsed.cleanRule = parsed.cleanRule.trimmed();
    }

    if (useForcedMode) {
        parsed.scopeMode = forcedMode;
    }

    return parsed;
}

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::collectTargets(const ParsedRule& parsed)
{
    QVector<RenameTarget> targets = parsed.includeHierarchy
        ? collectHierarchyTargets(parsed.includeShapes)
        : collectTargets(parsed.scopeMode);

    if (targets.isEmpty() && !parsed.includeHierarchy && !parsed.typeFilters.isEmpty()) {
        targets = collectAllTargets();
    }

    if (!parsed.typeFilters.isEmpty()) {
        targets = filterTargetsByType(targets, parsed.typeFilters);
    }
    return targets;
}

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::collectTargets(ScopeMode mode)
{
    switch (mode) {
    case ScopeMode::Hierarchy:
        return collectHierarchyTargets();
    case ScopeMode::All:
        return collectAllTargets();
    case ScopeMode::Selected:
    default:
        return collectSelectionTargets();
    }
}

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::collectSelectionTargets()
{
    QVector<RenameTarget> targets;
    MSelectionList selection;
    if (MGlobal::getActiveSelectionList(selection) != MS::kSuccess) {
        return targets;
    }

    MItSelectionList iterator(selection);
    for (; !iterator.isDone(); iterator.next()) {
        MDagPath dagPath;
        MObject component;
        MStatus status = iterator.getDagPath(dagPath, component);
        if (status == MS::kSuccess) {
            RenameTarget target;
            target.object = dagPath.node();
            target.path = toQString(dagPath.fullPathName());
            target.currentName = toQString(dagPath.partialPathName());
            target.isDag = true;
            appendUniqueTarget(targets, target);
            continue;
        }

        MObject object;
        status = iterator.getDependNode(object);
        if (status != MS::kSuccess) {
            continue;
        }

        MFnDependencyNode dependencyNode(object, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        RenameTarget target;
        target.object = object;
        target.path = toQString(dependencyNode.name());
        target.currentName = target.path;
        target.isDag = false;
        appendUniqueTarget(targets, target);
    }
    return targets;
}

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::collectHierarchyTargets(bool includeShapes)
{
    const QVector<RenameTarget> selectionTargets = collectSelectionTargets();
    QVector<RenameTarget> roots;
    roots.reserve(selectionTargets.size());

    for (const RenameTarget& candidate : selectionTargets) {
        bool removeCandidate = false;
        for (const RenameTarget& other : selectionTargets) {
            if (candidate.path == other.path) {
                continue;
            }
            if (candidate.isDag && other.isDag && pathIsAncestorOf(other.path, candidate.path)) {
                removeCandidate = true;
                break;
            }
        }
        if (!removeCandidate) {
            roots.push_back(candidate);
        }
    }

    QVector<RenameTarget> targets;
    for (const RenameTarget& root : roots) {
        appendUniqueTarget(targets, root);
        if (!root.isDag) {
            continue;
        }

        MDagPath rootPath;
        MStatus status = MDagPath::getAPathTo(root.object, rootPath);
        if (status != MS::kSuccess) {
            continue;
        }
        appendDescendants(rootPath, targets, includeShapes);
    }
    return targets;
}

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::collectAllTargets()
{
    QVector<RenameTarget> targets;
    MItDependencyNodes iterator;
    for (; !iterator.isDone(); iterator.next()) {
        MObject object = iterator.thisNode();
        if (object.isNull()) {
            continue;
        }

        MStatus status;
        if (object.hasFn(MFn::kDagNode)) {
            MDagPath path;
            status = MDagPath::getAPathTo(object, path);
            if (status != MS::kSuccess) {
                continue;
            }

            RenameTarget target;
            target.object = object;
            target.path = toQString(path.fullPathName());
            target.currentName = toQString(path.partialPathName());
            target.isDag = true;
            appendUniqueTarget(targets, target);
            continue;
        }

        MFnDependencyNode dependencyNode(object, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        RenameTarget target;
        target.object = object;
        target.path = toQString(dependencyNode.name());
        target.currentName = target.path;
        appendUniqueTarget(targets, target);
    }
    return targets;
}

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::collectWildcardTargets(const QString& pattern)
{
    QVector<RenameTarget> targets;
    if (pattern.trimmed().isEmpty()) {
        return targets;
    }

    MSelectionList selection;
    if (MGlobal::getSelectionListByName(toMString(pattern), selection) != MS::kSuccess) {
        return targets;
    }

    const bool includeShapes = readBoolOptionVar(kWildcardIncludeShapesOptionVar, false);

    MItSelectionList iterator(selection);
    for (; !iterator.isDone(); iterator.next()) {
        MDagPath dagPath;
        MObject component;
        MStatus status = iterator.getDagPath(dagPath, component);
        if (status == MS::kSuccess) {
            RenameTarget target;
            target.object = dagPath.node();
            target.path = toQString(dagPath.fullPathName());
            target.currentName = toQString(dagPath.partialPathName());
            target.isDag = true;
            appendUniqueTarget(targets, target);
            continue;
        }

        MObject object;
        status = iterator.getDependNode(object);
        if (status != MS::kSuccess) {
            continue;
        }

        MFnDependencyNode dependencyNode(object, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        RenameTarget target;
        target.object = object;
        target.path = toQString(dependencyNode.name());
        target.currentName = target.path;
        appendUniqueTarget(targets, target);
    }

    QVector<RenameTarget> filtered;
    filtered.reserve(targets.size());
    QSet<QString> selectedPaths;
    for (const RenameTarget& target : targets) {
        selectedPaths.insert(target.path);
    }

    for (const RenameTarget& target : targets) {
        if (isTransformLike(target.object)) {
            filtered.push_back(target);
            continue;
        }

        if (includeShapes && target.object.hasFn(MFn::kShape)) {
            filtered.push_back(target);
            continue;
        }

        if (target.isDag) {
            const int splitIndex = target.path.lastIndexOf(QChar(u'|'));
            if (splitIndex > 0) {
                const QString parentPath = target.path.left(splitIndex);
                if (selectedPaths.contains(parentPath)) {
                    continue;
                }
            }
        }
        filtered.push_back(target);
    }
    return filtered;
}

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::filterTargetsByType(const QVector<RenameTarget>& targets, const QStringList& typeFilters)
{
    if (typeFilters.isEmpty()) {
        return targets;
    }

    QSet<QString> allowedTypes;
    for (const QString& typeName : typeFilters) {
        allowedTypes.insert(typeName);
    }

    QVector<RenameTarget> filtered;
    filtered.reserve(targets.size());
    for (const RenameTarget& target : targets) {
        MStatus status;
        MFnDependencyNode dependencyNode(target.object, &status);
        if (status != MS::kSuccess) {
            continue;
        }
        if (allowedTypes.contains(toQString(dependencyNode.typeName()))) {
            filtered.push_back(target);
        }
    }
    return filtered;
}

QStringList OneLinerEngine::buildPreviewItems(const ParsedRule& parsed, QVector<RenameTarget>* outTargets, QStringList* outRawItems)
{
    QVector<RenameTarget> targets;
    if (parsed.selectionOnly) {
        targets = collectWildcardTargets(parsed.wildcardPattern);

        QStringList names;
        for (const RenameTarget& target : targets) {
            names.push_back(shortName(target.currentName));
        }
        QStringList rawItems = names;
        sortPreviewEntries(targets, names, &rawItems);
        if (outTargets != nullptr) {
            *outTargets = targets;
        }
        if (outRawItems != nullptr) {
            *outRawItems = rawItems;
        }
        return formatPreviewItems(targets, names);
    }

    QStringList renamedItems = buildRenamedItems(parsed, &targets, false);
    QStringList rawItems = renamedItems;
    sortPreviewEntries(targets, renamedItems, &rawItems);
    if (outTargets != nullptr) {
        *outTargets = targets;
    }
    if (outRawItems != nullptr) {
        *outRawItems = rawItems;
    }
    return formatPreviewItems(targets, renamedItems);
}

QStringList OneLinerEngine::buildRenamedItems(const ParsedRule& parsed, QVector<RenameTarget>* outTargets, bool reverseForRename)
{
    QVector<RenameTarget> targets = collectTargets(parsed);
    if (outTargets != nullptr) {
        *outTargets = targets;
    }

    QStringList renamedItems;
    renamedItems.reserve(targets.size());
    if (parsed.cleanRule.isEmpty()) {
        for (const RenameTarget& target : targets) {
            renamedItems.push_back(shortName(target.currentName));
        }
        return renamedItems;
    }

    for (int index = 0; index < targets.size(); ++index) {
        const RenameTarget& target = targets[index];
        const QString currentName = shortName(target.currentName);
        QString nextName;

        if (parsed.cleanRule.contains(QChar(u'>'))) {
            nextName = currentName;
            const QVector<QPair<QString, QString>> replacementPairs = replacementPairsForRule(parsed.cleanRule, index);
            if (replacementPairs.isEmpty()) {
                renamedItems.push_back(currentName);
                continue;
            }
            for (const auto& replacementPair : replacementPairs) {
                nextName.replace(replacementPair.first, replacementPair.second, Qt::CaseSensitive);
            }
        } else if (parsed.cleanRule.startsWith(QChar(u'-'))) {
            if (parsed.cleanRule == QStringLiteral("-") || parsed.cleanRule == QStringLiteral("--")) {
                nextName = currentName;
            } else {
                bool ok = false;
                const int charCount = parsed.cleanRule.mid(1).toInt(&ok);
                if (!ok) {
                    nextName = currentName;
                } else if (charCount >= 0) {
                    nextName = currentName.left(qMax(0, currentName.size() - charCount));
                } else {
                    nextName = currentName.left(qMin(currentName.size(), -charCount));
                }
            }
        } else if (parsed.cleanRule.startsWith(QChar(u'+'))) {
            if (parsed.cleanRule.size() == 1) {
                nextName = currentName;
            } else {
                bool ok = false;
                const int charCount = parsed.cleanRule.mid(1).toInt(&ok);
                nextName = ok ? currentName.mid(qMax(0, charCount)) : currentName;
            }
        } else {
            nextName = applyNumberPattern(parsed.cleanRule, index);
            nextName.replace(QStringLiteral("!"), currentName, Qt::CaseSensitive);
        }

        if (nextName == QStringLiteral("/")) {
            renamedItems.push_back(currentName);
            continue;
        }

        nextName = shortName(nextName);
        if (nameMatchCount(nextName) > 1) {
            nextName = uniqueNameWithDigits(nextName);
        }
        renamedItems.push_back(nextName);
    }

    bool needsReverse = false;
    for (const RenameTarget& target : targets) {
        if (target.path.contains(QChar(u'|'))) {
            needsReverse = true;
            break;
        }
    }
    if (!needsReverse) {
        for (const QString& name : renamedItems) {
            if (name.contains(QChar(u'|'))) {
                needsReverse = true;
                break;
            }
        }
    }

    if (needsReverse && reverseForRename) {
        std::reverse(renamedItems.begin(), renamedItems.end());
        if (outTargets != nullptr) {
            std::reverse(outTargets->begin(), outTargets->end());
        }
    }
    return renamedItems;
}

QStringList OneLinerEngine::formatPreviewItems(const QVector<RenameTarget>& targets, const QStringList& displayNames)
{
    if (targets.size() != displayNames.size()) {
        return displayNames;
    }

    int baseDagDepth = std::numeric_limits<int>::max();
    for (const RenameTarget& target : targets) {
        if (!target.isDag) {
            continue;
        }
        baseDagDepth = qMin(baseDagDepth, dagDepth(target.path));
    }

    if (baseDagDepth == std::numeric_limits<int>::max()) {
        baseDagDepth = 0;
    }

    QStringList previewItems;
    previewItems.reserve(displayNames.size());
    for (int index = 0; index < displayNames.size(); ++index) {
        previewItems.push_back(previewLabelForTarget(targets[index], displayNames[index], baseDagDepth));
    }
    return previewItems;
}

QString OneLinerEngine::applyNumberPattern(QString text, int index)
{
    int start = 1;
    const int startMarker = text.indexOf(QStringLiteral("//"));
    if (startMarker >= 0) {
        bool ok = false;
        const int parsedStart = text.mid(startMarker + 2).toInt(&ok);
        if (ok) {
            start = parsedStart;
        }
        text = text.left(startMarker);
    }

    const int number = index + start;
    if (text.contains(QChar(u'#'))) {
        const int padding = text.count(QChar(u'#'));
        const QString token(padding, QChar(u'#'));
        text.replace(token, QStringLiteral("%1").arg(number, padding, 10, QChar(u'0')));
    }

    if (text.contains(QChar(u'@'))) {
        const int padding = text.count(QChar(u'@'));
        const QString token(padding, QChar(u'@'));
        text.replace(token, indexToLetters(index));
    }
    return text;
}

QString OneLinerEngine::uniqueNameWithDigits(QString name)
{
    name = shortName(name);
    int digit = 0;
    while (!name.isEmpty() && name.back().isDigit()) {
        digit += name.back().digitValue();
        name.chop(1);
    }

    if (nameExists(name)) {
        while (nameExists(name + QString::number(digit))) {
            ++digit;
        }
        return name + QString::number(digit + 1);
    }
    return name + QString::number(digit);
}

QString OneLinerEngine::shortName(const QString& name)
{
    const int splitIndex = name.lastIndexOf(QChar(u'|'));
    return splitIndex >= 0 ? name.mid(splitIndex + 1) : name;
}

bool OneLinerEngine::isTransformLike(const MObject& object)
{
    MStatus status;
    MFnDependencyNode dependencyNode(object, &status);
    if (status != MS::kSuccess) {
        return false;
    }
    const QString typeName = toQString(dependencyNode.typeName());
    return typeName == QStringLiteral("transform") || typeName == QStringLiteral("joint");
}

bool OneLinerEngine::pathIsAncestorOf(const QString& parentPath, const QString& childPath)
{
    if (parentPath.isEmpty() || childPath.isEmpty() || parentPath == childPath) {
        return false;
    }
    return childPath.startsWith(parentPath + QStringLiteral("|"), Qt::CaseSensitive);
}

bool OneLinerEngine::renameObject(const RenameTarget& target, const QString& newName)
{
    if (newName.isEmpty()) {
        return false;
    }

    MStatus status;
    MFnDependencyNode dependencyNode(target.object, &status);
    if (status != MS::kSuccess) {
        return false;
    }

    dependencyNode.setName(toMString(shortName(newName)), false, &status);
    return status == MS::kSuccess;
}

bool OneLinerEngine::selectTargets(const QVector<RenameTarget>& targets)
{
    MSelectionList selection;
    for (const RenameTarget& target : targets) {
        selection.add(toMString(target.path));
    }
    return MGlobal::setActiveSelectionList(selection, MGlobal::kReplaceList) == MS::kSuccess;
}

int OneLinerEngine::nameMatchCount(const QString& name)
{
    MSelectionList selection;
    if (MGlobal::getSelectionListByName(toMString(name), selection) != MS::kSuccess) {
        return 0;
    }
    return static_cast<int>(selection.length());
}

bool OneLinerEngine::nameExists(const QString& name)
{
    return nameMatchCount(name) > 0;
}