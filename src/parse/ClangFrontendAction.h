#pragma once

#include "MainASTConsumer.h"


#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"


namespace smacpp {
class FrontendAction : public clang::ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& Compiler, llvm::StringRef InFile)
    {
        return std::make_unique<MainASTConsumer>(Compiler.getASTContext());
    }
};
} // namespace smacpp
