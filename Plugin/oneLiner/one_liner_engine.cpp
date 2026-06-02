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

#include <QSet>

namespace {

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

void appendDescendants(const MDagPath& parentPath, QVector<OneLinerEngine::RenameTarget>& targets)
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

        if (OneLinerEngine::isTransformLike(childObject)) {
            OneLinerEngine::RenameTarget target;
            target.object = childObject;
            target.path = toQString(childPath.fullPathName());
            target.currentName = toQString(childPath.partialPathName());
            target.isDag = true;
            appendUniqueTarget(targets, target);
        }

        appendDescendants(childPath, targets);
    }
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
    result.selectionOnly = parsed.selectionOnly;
    result.items = buildPreviewItems(parsed, nullptr);
    return result;
}

bool OneLinerEngine::execute(const QString& rule)
{
    return execute(rule, ScopeMode::Selected, false);
}

bool OneLinerEngine::execute(const QString& rule, ScopeMode forcedMode, bool useForcedMode)
{
    const ParsedRule parsed = parseRule(rule, forcedMode, useForcedMode);
    if (parsed.rawRule.isEmpty() || parsed.rawRule == QStringLiteral("/")) {
        return true;
    }

    QVector<RenameTarget> targets;
    const QStringList previewItems = buildPreviewItems(parsed, &targets);
    Q_UNUSED(previewItems)

    if (parsed.selectionOnly || parsed.cleanRule.isEmpty()) {
        return selectTargets(targets);
    }

    const QStringList renamedItems = buildRenamedItems(parsed, &targets);
    if (targets.size() != renamedItems.size()) {
        return false;
    }

    bool success = true;
    MGlobal::executeCommand(toMString(QStringLiteral("undoInfo -openChunk \"oneLiner\"")), false, true);
    for (int index = 0; index < targets.size(); ++index) {
        if (!renameObject(targets[index], renamedItems[index])) {
            success = false;
        }
    }
    MGlobal::executeCommand(toMString(QStringLiteral("undoInfo -closeChunk")), false, true);
    return success;
}

bool OneLinerEngine::renamePastedPrefix()
{
    const QVector<RenameTarget> targets = collectPatternTargets(SelectorMode::Prefix, QStringLiteral("pasted__"));
    if (targets.isEmpty()) {
        return true;
    }

    bool success = true;
    MGlobal::executeCommand(toMString(QStringLiteral("undoInfo -openChunk \"oneLinerClearPasted\"")), false, true);
    for (const RenameTarget& target : targets) {
        QString newName = target.currentName;
        newName.replace(QStringLiteral("pasted__"), QString(), Qt::CaseSensitive);
        newName = uniqueNameWithDigits(newName);
        if (!renameObject(target, newName)) {
            success = false;
        }
    }
    MGlobal::executeCommand(toMString(QStringLiteral("undoInfo -closeChunk")), false, true);
    return success;
}

OneLinerEngine::ParsedRule OneLinerEngine::parseRule(const QString& rule, ScopeMode forcedMode, bool useForcedMode)
{
    ParsedRule parsed;
    parsed.rawRule = rule.trimmed();
    parsed.cleanRule = parsed.rawRule;

    if (parsed.rawRule.startsWith(QStringLiteral("fs:"))) {
        parsed.selectorMode = SelectorMode::Prefix;
        parsed.selectorText = parsed.rawRule.mid(3);
        parsed.selectionOnly = true;
        parsed.cleanRule.clear();
    } else if (parsed.rawRule.startsWith(QStringLiteral("fe:"))) {
        parsed.selectorMode = SelectorMode::Suffix;
        parsed.selectorText = parsed.rawRule.mid(3);
        parsed.selectionOnly = true;
        parsed.cleanRule.clear();
    } else if (parsed.rawRule.startsWith(QStringLiteral("f:"))) {
        parsed.selectorMode = SelectorMode::Contains;
        parsed.selectorText = parsed.rawRule.mid(2);
        parsed.selectionOnly = true;
        parsed.cleanRule.clear();
    } else {
        if (parsed.cleanRule.contains(QStringLiteral("/s"))) {
            parsed.scopeMode = ScopeMode::Selected;
            parsed.cleanRule.replace(QStringLiteral("/s"), QString());
        }
        if (parsed.cleanRule.contains(QStringLiteral("/h"))) {
            parsed.scopeMode = ScopeMode::Hierarchy;
            parsed.cleanRule.replace(QStringLiteral("/h"), QString());
        }
        if (parsed.cleanRule.contains(QStringLiteral("/a")) && parsed.cleanRule.contains(QChar(u'>'))) {
            parsed.scopeMode = ScopeMode::All;
            parsed.cleanRule.replace(QStringLiteral("/a"), QString());
        }
        parsed.cleanRule = parsed.cleanRule.trimmed();
    }

    if (useForcedMode) {
        parsed.scopeMode = forcedMode;
    }

    return parsed;
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

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::collectHierarchyTargets()
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
        appendDescendants(rootPath, targets);
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

QVector<OneLinerEngine::RenameTarget> OneLinerEngine::collectPatternTargets(SelectorMode mode, const QString& pattern)
{
    QVector<RenameTarget> targets;
    MSelectionList selection;
    const QString resolvedPattern = resolvePattern(mode, pattern);
    if (MGlobal::getSelectionListByName(toMString(resolvedPattern), selection) != MS::kSuccess) {
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

QStringList OneLinerEngine::buildPreviewItems(const ParsedRule& parsed, QVector<RenameTarget>* outTargets)
{
    QVector<RenameTarget> targets;
    if (parsed.selectionOnly) {
        targets = collectPatternTargets(parsed.selectorMode, parsed.selectorText);
        if (outTargets != nullptr) {
            *outTargets = targets;
        }

        QStringList names;
        for (const RenameTarget& target : targets) {
            names.push_back(shortName(target.currentName));
        }
        return names;
    }

    return buildRenamedItems(parsed, outTargets);
}

QStringList OneLinerEngine::buildRenamedItems(const ParsedRule& parsed, QVector<RenameTarget>* outTargets)
{
    QVector<RenameTarget> targets = collectTargets(parsed.scopeMode);
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
            const QStringList words = parsed.cleanRule.split(QChar(u'>'));
            const QString oldWord = words.value(0);
            const QString newWord = applyNumberPattern(words.value(1), index);
            nextName = currentName;
            nextName.replace(oldWord, newWord, Qt::CaseSensitive);
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

    if (needsReverse) {
        std::reverse(renamedItems.begin(), renamedItems.end());
        if (outTargets != nullptr) {
            std::reverse(outTargets->begin(), outTargets->end());
        }
    }
    return renamedItems;
}

QString OneLinerEngine::resolvePattern(SelectorMode mode, const QString& pattern)
{
    switch (mode) {
    case SelectorMode::Prefix:
        return pattern + QStringLiteral("*");
    case SelectorMode::Suffix:
        return QStringLiteral("*") + pattern;
    case SelectorMode::Contains:
        return QStringLiteral("*") + pattern + QStringLiteral("*");
    case SelectorMode::None:
    default:
        return pattern;
    }
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

    const QString command = QStringLiteral("rename %1 %2")
        .arg(quoteMel(target.path), quoteMel(shortName(newName)));
    return MGlobal::executeCommand(toMString(command), false, true) == MS::kSuccess;
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