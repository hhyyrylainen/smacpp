#pragma once

#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"


#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"

namespace smacpp {

class SMACPPChecker
    : public clang::ento::Checker<clang::ento::check::PostCall, clang::ento::check::PreCall,
          clang::ento::check::DeadSymbols, clang::ento::check::PointerEscape> {
public:
    void checkPostCall(
        const clang::ento::CallEvent& Call, clang::ento::CheckerContext& C) const;

    void checkPreCall(
        const clang::ento::CallEvent& Call, clang::ento::CheckerContext& C) const;

    void checkDeadSymbols(
        clang::ento::SymbolReaper& SymReaper, clang::ento::CheckerContext& C) const;

    clang::ento::ProgramStateRef checkPointerEscape(clang::ento::ProgramStateRef State,
        const clang::ento::InvalidatedSymbols& Escaped, const clang::ento::CallEvent* Call,
        clang::ento::PointerEscapeKind Kind) const;
};

} // namespace smacpp
