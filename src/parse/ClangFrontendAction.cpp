// ------------------------------------ //
#include "ClangFrontendAction.h"

#include "MainASTConsumer.h"

#include <clang/Frontend/CompilerInstance.h>

using namespace smacpp;
// ------------------------------------ //
std::unique_ptr<clang::ASTConsumer> FrontendAction::CreateASTConsumer(
    clang::CompilerInstance& Compiler, llvm::StringRef InFile)
{
    return std::make_unique<MainASTConsumer>( // Compiler.getASTContext()
    );
}
