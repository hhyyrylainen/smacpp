// ------------------------------------ //
#include "SMACPPChecker.h"

using namespace smacpp;
// ------------------------------------ //
void SMACPPChecker::checkPostCall(
    const clang::ento::CallEvent& Call, clang::ento::CheckerContext& C) const
{
    llvm::outs() << "got stuff to check: ";
    Call.dump(llvm::outs());
}

void SMACPPChecker::checkPreCall(
    const clang::ento::CallEvent& Call, clang::ento::CheckerContext& C) const
{}

void SMACPPChecker::checkDeadSymbols(
    clang::ento::SymbolReaper& SymReaper, clang::ento::CheckerContext& C) const
{}

clang::ento::ProgramStateRef SMACPPChecker::checkPointerEscape(
    clang::ento::ProgramStateRef State, const clang::ento::InvalidatedSymbols& Escaped,
    const clang::ento::CallEvent* Call, clang::ento::PointerEscapeKind Kind) const
{
    return State;
}
