#include "one_liner_engine.h"
#include "one_liner_ui.h"

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#ifdef _WIN32
#define MAYA_PLUGIN_EXPORT __declspec(dllexport)
#else
#define MAYA_PLUGIN_EXPORT
#endif

namespace {

constexpr const char* kCommandName = "oneLiner";
constexpr const char* kShowWindowFlag = "-sw";
constexpr const char* kShowWindowLongFlag = "-showWindow";
constexpr const char* kRuleFlag = "-r";
constexpr const char* kRuleLongFlag = "-rule";
constexpr const char* kPreviewFlag = "-p";
constexpr const char* kPreviewLongFlag = "-preview";
constexpr const char* kExecuteFlag = "-e";
constexpr const char* kExecuteLongFlag = "-execute";
constexpr const char* kModeFlag = "-m";
constexpr const char* kModeLongFlag = "-mode";
constexpr const char* kClearPastedFlag = "-cp";
constexpr const char* kClearPastedLongFlag = "-clearPasted";
constexpr const char* kHelpFlag = "-h";
constexpr const char* kHelpLongFlag = "-help";

QString commandHelp()
{
    return QStringLiteral(
        "oneLiner flags:\n"
        "  -showWindow / -sw             Open the C++ UI\n"
        "  -rule / -r <text>             Provide a oneLiner rule string\n"
        "  -preview / -p                 Return a preview list instead of renaming\n"
        "  -execute / -e                 Execute renaming immediately\n"
        "  -mode / -m <s|h|a>            Force scope (selected/hierarchy/all)\n"
        "  -clearPasted / -cp            Clear pasted__ prefixes\n"
        "  -help / -h                    Print this help text\n");
}

QString resolveProjectRoot(const QDir& pluginBinDir)
{
    QDir candidate = pluginBinDir;
    for (int depth = 0; depth < 4; ++depth) {
        if (candidate.exists(QStringLiteral("images"))) {
            return candidate.absolutePath();
        }
        if (!candidate.cdUp()) {
            break;
        }
    }

    return pluginBinDir.absoluteFilePath(QStringLiteral("../.."));
}

void registerMediaSearchPaths(const MFnPlugin& plugin)
{
    const QFileInfo pluginFile(QString::fromUtf8(plugin.loadPath().asChar()));
    const QDir pluginBinDir = pluginFile.absoluteDir();
    const QString projectRoot = resolveProjectRoot(pluginBinDir);
    const QString imagesRoot = QDir(projectRoot).absoluteFilePath(QStringLiteral("images"));
    const QString iconsRoot = QDir(imagesRoot).absoluteFilePath(QStringLiteral("icon"));

    QDir::setSearchPaths(QStringLiteral("oneLinerImages"), QStringList{imagesRoot});
    QDir::setSearchPaths(QStringLiteral("oneLinerIcons"), QStringList{iconsRoot});
}

OneLinerEngine::ScopeMode parseMode(const QString& modeText)
{
    if (modeText.compare(QStringLiteral("h"), Qt::CaseInsensitive) == 0) {
        return OneLinerEngine::ScopeMode::Hierarchy;
    }
    if (modeText.compare(QStringLiteral("a"), Qt::CaseInsensitive) == 0) {
        return OneLinerEngine::ScopeMode::All;
    }
    return OneLinerEngine::ScopeMode::Selected;
}

class OneLinerCommand : public MPxCommand
{
public:
    static void* creator()
    {
        return new OneLinerCommand();
    }

    static MSyntax newSyntax()
    {
        MSyntax syntax;
        syntax.addFlag(kShowWindowFlag, kShowWindowLongFlag);
        syntax.addFlag(kRuleFlag, kRuleLongFlag, MSyntax::kString);
        syntax.addFlag(kPreviewFlag, kPreviewLongFlag);
        syntax.addFlag(kExecuteFlag, kExecuteLongFlag);
        syntax.addFlag(kModeFlag, kModeLongFlag, MSyntax::kString);
        syntax.addFlag(kClearPastedFlag, kClearPastedLongFlag);
        syntax.addFlag(kHelpFlag, kHelpLongFlag);
        return syntax;
    }

    MStatus doIt(const MArgList& args) override
    {
        _renameOperations.clear();
        _canUndo = false;

        MStatus status;
        MArgDatabase database(OneLinerCommand::newSyntax(), args, &status);
        if (status != MS::kSuccess) {
            return status;
        }

        if (database.isFlagSet(kHelpFlag)) {
            setResult(commandHelp().toUtf8().constData());
            return MS::kSuccess;
        }

        if (database.isFlagSet(kClearPastedFlag)) {
            _renameOperations = OneLinerEngine::buildClearPastedOperations();
            _canUndo = !_renameOperations.isEmpty();
            return _canUndo ? redoIt() : MS::kSuccess;
        }

        const bool hasRule = database.isFlagSet(kRuleFlag);
        QString rule;
        if (hasRule) {
            MString value;
            database.getFlagArgument(kRuleFlag, 0, value);
            rule = QString::fromUtf8(value.asChar());
        }

        bool useForcedMode = false;
        OneLinerEngine::ScopeMode forcedMode = OneLinerEngine::ScopeMode::Selected;
        if (database.isFlagSet(kModeFlag)) {
            MString modeText;
            database.getFlagArgument(kModeFlag, 0, modeText);
            forcedMode = parseMode(QString::fromUtf8(modeText.asChar()));
            useForcedMode = true;
        }

        if (database.isFlagSet(kPreviewFlag)) {
            const OneLinerEngine::PreviewResult preview = OneLinerEngine::preview(rule, forcedMode, useForcedMode);
            MStringArray result;
            for (const QString& item : preview.items) {
                result.append(item.toUtf8().constData());
            }
            setResult(result);
            return MS::kSuccess;
        }

        if (database.isFlagSet(kExecuteFlag)) {
            const OneLinerEngine::ExecutePlan plan = OneLinerEngine::buildExecutePlan(rule, forcedMode, useForcedMode);
            if (plan.noop) {
                return MS::kSuccess;
            }
            if (plan.selectionOnly) {
                return OneLinerEngine::selectTargets(plan.selectionTargets) ? MS::kSuccess : MS::kFailure;
            }

            _renameOperations = plan.renameOperations;
            _canUndo = !_renameOperations.isEmpty();
            return _canUndo ? redoIt() : MS::kSuccess;
        }

        if (!hasRule || database.isFlagSet(kShowWindowFlag) || args.length() == 0) {
            OneLinerWindow::showWindow();
            return MS::kSuccess;
        }

        const OneLinerEngine::ExecutePlan plan = OneLinerEngine::buildExecutePlan(rule, forcedMode, useForcedMode);
        if (plan.noop) {
            return MS::kSuccess;
        }
        if (plan.selectionOnly) {
            return OneLinerEngine::selectTargets(plan.selectionTargets) ? MS::kSuccess : MS::kFailure;
        }

        _renameOperations = plan.renameOperations;
        _canUndo = !_renameOperations.isEmpty();
        return _canUndo ? redoIt() : MS::kSuccess;
    }

    MStatus redoIt() override
    {
        return OneLinerEngine::applyRenameOperations(_renameOperations, true) ? MS::kSuccess : MS::kFailure;
    }

    MStatus undoIt() override
    {
        return OneLinerEngine::applyRenameOperations(_renameOperations, false) ? MS::kSuccess : MS::kFailure;
    }

    bool isUndoable() const override
    {
        return _canUndo;
    }

private:
    QVector<OneLinerEngine::RenameOperation> _renameOperations;
    bool _canUndo = false;
};

} // namespace

MAYA_PLUGIN_EXPORT MStatus initializePlugin(MObject object)
{
    MFnPlugin plugin(object, "yibai", "1.0.0", "Any");
    registerMediaSearchPaths(plugin);
    const MStatus status = plugin.registerCommand(kCommandName, OneLinerCommand::creator, OneLinerCommand::newSyntax);
    if (status != MS::kSuccess) {
        MGlobal::displayError(QStringLiteral("Failed to register Maya command: %1").arg(QString::fromUtf8(kCommandName)).toUtf8().constData());
    }
    return status;
}

MAYA_PLUGIN_EXPORT MStatus uninitializePlugin(MObject object)
{
    OneLinerWindow::closeWindow(true);
    MFnPlugin plugin(object);
    return plugin.deregisterCommand(kCommandName);
}