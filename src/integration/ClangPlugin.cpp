// ------------------------------------ //
#include "parse/ClangASTAction.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
// ------------------------------------ //

static clang::FrontendPluginRegistry::Add<smacpp::ASTAction> X(
    "smacpp", "run smacpp static analysis");

extern "C" const char clang_analyzerAPIVersionString[] = CLANG_ANALYZER_API_VERSION_STRING;
