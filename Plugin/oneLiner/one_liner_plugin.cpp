#include "one_liner_engine.h"
#include "one_liner_ui.h"

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>

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
constexpr const char* kHelpFlag = "-hp";
constexpr const char* kHelpLongFlag = "-help";

QString commandHelp()
{
    return QStringLiteral(
        "oneLiner flags:\n"
        "  -showWindow / -sw             打开 C++ UI\n"
        "  -rule / -r <text>             传入 oneLiner 规则\n"
        "  -preview / -p                 返回预览列表\n"
        "  -execute / -e                 直接执行重命名\n"
        "  -mode / -m <s|h|a>            强制作用范围\n"
        "  -clearPasted / -cp            清理 pasted__ 前缀\n"
        "  -help / -hp                   输出帮助文本");
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
            return OneLinerEngine::renamePastedPrefix() ? MS::kSuccess : MS::kFailure;
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
            return OneLinerEngine::execute(rule, forcedMode, useForcedMode) ? MS::kSuccess : MS::kFailure;
        }

        if (!hasRule || database.isFlagSet(kShowWindowFlag) || args.length() == 0) {
            OneLinerWindow::showWindow();
            return MS::kSuccess;
        }

        return OneLinerEngine::execute(rule, forcedMode, useForcedMode) ? MS::kSuccess : MS::kFailure;
    }

    bool isUndoable() const override
    {
        return false;
    }
};

} // namespace

MAYA_PLUGIN_EXPORT MStatus initializePlugin(MObject object)
{
    MFnPlugin plugin(object, "yibai", "1.0.0", "Any");
    const MStatus status = plugin.registerCommand(kCommandName, OneLinerCommand::creator, OneLinerCommand::newSyntax);
    if (status != MS::kSuccess) {
        MGlobal::displayError(QStringLiteral("Failed to register Maya command: %1").arg(QString::fromUtf8(kCommandName)).toUtf8().constData());
    }
    return status;
}

MAYA_PLUGIN_EXPORT MStatus uninitializePlugin(MObject object)
{
    OneLinerWindow::closeWindow();
    MFnPlugin plugin(object);
    return plugin.deregisterCommand(kCommandName);
}