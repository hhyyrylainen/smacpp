#pragma once

#include "clang/Frontend/FrontendAction.h"


namespace smacpp {
class FrontendAction : public clang::ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& Compiler, llvm::StringRef InFile);
};
} // namespace smacpp
