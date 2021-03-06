#pragma once

#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"


#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"

namespace smacpp {

class SMACPPChecker
    : public clang::ento::Checker<clang::ento::check::PostCall, clang::ento::check::PreCall,
          clang::ento::check::DeadSymbols, clang::ento::check::PointerEscape,
          clang::ento::check::Location, clang::ento::check::Bind> {
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

    void checkBind(const clang::ento::SVal& location, const clang::ento::SVal& val,
        const clang::Stmt* S, clang::ento::CheckerContext& C) const;

    void checkLocation(const clang::ento::SVal& location, bool isLoad, const clang::Stmt* S,
        clang::ento::CheckerContext& C) const;
};

} // namespace smacpp
