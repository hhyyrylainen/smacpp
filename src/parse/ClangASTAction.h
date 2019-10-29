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
        return std::make_unique<MainASTConsumer>(EnableDebugPrint
            // Compiler.getASTContext()
        );
    }

    bool ParseArgs(
        const clang::CompilerInstance& CI, const std::vector<std::string>& args) override
    {
        for(size_t i = 0; i < args.size(); ++i) {
            if(args[i] == "-smacpp-debug") {
                EnableDebugPrint = true;
            }
        }
        if(!args.empty() && args[0] == "help")
            PrintHelp(llvm::errs());

        return true;
    }

    void PrintHelp(llvm::raw_ostream& ros)
    {
        ros << "SMACPP Clang plugin:\n"
            << "-smacpp-debug Enables debug printing\n";
    }

    //! This should automatically run the plugin after the main AST action when usinf -fplugin=
    //! clang flag
    PluginASTAction::ActionType getActionType() override
    {
        return AddAfterMainAction;
    }

protected:
    bool EnableDebugPrint = false;
};
} // namespace smacpp
