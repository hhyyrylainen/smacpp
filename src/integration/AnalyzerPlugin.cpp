#include "SMACPPChecker.h"

#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"

extern "C" void clang_registerCheckers(clang::ento::CheckerRegistry& registry)
{
    // TODO: the third parameter is docs url
    registry.addChecker<smacpp::SMACPPChecker>(
        "smacpp.All", "Detects memory related errors with SMACPP", "");
}

extern "C" const char clang_analyzerAPIVersionString[] = CLANG_ANALYZER_API_VERSION_STRING;
