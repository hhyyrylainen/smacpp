// ------------------------------------ //
#include "parse/ClangASTAction.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
// ------------------------------------ //

static clang::FrontendPluginRegistry::Add<smacpp::ASTAction> X(
    "smacpp", "run smacpp static analysis");
