#pragma once

#include "MainASTConsumer.h"


#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"


namespace smacpp {
class ASTAction : public clang::PluginASTAction {
protected:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& Compiler, llvm::StringRef InFile) override
    {
        return std::make_unique<MainASTConsumer>(Compiler.getASTContext());
    }

    bool ParseArgs(
        const clang::CompilerInstance& CI, const std::vector<std::string>& args) override
    {
        // Sample code from:
        // https://github.com/llvm/llvm-project/blob/master/clang/examples/PrintFunctionNames/PrintFunctionNames.cpp
        // for (unsigned i = 0, e = args.size(); i != e; ++i) {
        //     llvm::errs() << "PrintFunctionNames arg = " << args[i] << "\n";

        //     // Example error handling.
        //     DiagnosticsEngine &D = CI.getDiagnostics();
        //     if (args[i] == "-an-error") {
        //         unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,
        //             "invalid argument '%0'");
        //         D.Report(DiagID) << args[i];
        //         return false;
        //     } else if (args[i] == "-parse-template") {
        //         if (i + 1 >= e) {
        //             D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
        //                     "missing -parse-template argument"));
        //             return false;
        //         }
        //         ++i;
        //         ParsedTemplates.insert(args[i]);
        //     }
        // }
        if(!args.empty() && args[0] == "help")
            PrintHelp(llvm::errs());

        return true;
    }

    void PrintHelp(llvm::raw_ostream& ros)
    {
        ros << "Help for smacpp plugin goes here\n";
    }

    //! This should automatically run the plugin after the main AST action when usinf -fplugin=
    //! clang flag
    PluginASTAction::ActionType getActionType() override
    {
        return AddAfterMainAction;
    }
};
} // namespace smacpp
