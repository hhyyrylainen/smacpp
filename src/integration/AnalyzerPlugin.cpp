#include "SMACPPChecker.h"

#include "clang/StaticAnalyzer/Core/CheckerRegistry.h"

extern "C" void clang_registerCheckers(clang::ento::CheckerRegistry& registry)
{
    registry.addChecker<smacpp::SMACPPChecker>(
        "smacpp.All", "Detects memory related errors with SMACPP");
}

extern "C" const char clang_analyzerAPIVersionString[] = CLANG_ANALYZER_API_VERSION_STRING;
