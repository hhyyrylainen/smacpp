// ------------------------------------ //
#include "SMACPPChecker.h"

using namespace smacpp;
using namespace clang;
using namespace clang::ento;
// ------------------------------------ //


class MemoryAreaStats {
public:
    MemoryAreaStats(size_t size) : Size(size) {}

    void Profile(llvm::FoldingSetNodeID& ID) const
    {
        ID.AddInteger(Size);
    }

    const auto GetSize() const
    {
        return Size;
    }

private:
    size_t Size;
};


REGISTER_MAP_WITH_PROGRAMSTATE(VariableToPointedMemoryMap, SymbolRef, MemoryAreaStats)

void SMACPPChecker::checkPostCall(const CallEvent& Call, CheckerContext& C) const
{
    // llvm::outs() << "got stuff to check: ";
    // Call.dump(llvm::outs());
}

void SMACPPChecker::checkPreCall(const CallEvent& Call, CheckerContext& C) const {}

void SMACPPChecker::checkDeadSymbols(SymbolReaper& SymReaper, CheckerContext& C) const {}

ProgramStateRef SMACPPChecker::checkPointerEscape(ProgramStateRef State,
    const InvalidatedSymbols& Escaped, const CallEvent* Call, PointerEscapeKind Kind) const
{
    return State;
}

void SMACPPChecker::checkBind(
    const SVal& location, const SVal& val, const clang::Stmt* S, CheckerContext& C) const
{
    SymbolRef var = location.getAsSymbol();
    SymbolRef source = val.getAsSymbol();

    llvm::outs() << "assigning: ";
    if(var)
        var->dumpToStream(llvm::outs());
    else
        llvm::outs() << "unknown var: ";

    llvm::outs() << " value: ";
    if(source)
        source->dumpToStream(llvm::outs());
    else {
        llvm::outs() << " statement: ";
        S->dump();
    }


    // ProgramStateRef State = C.getState();
    // State = State->set<VariableToPointedMemoryMap>(var, MemoryAreaStats(0));
    // C.addTransition(State);
}

void SMACPPChecker::checkLocation(
    const SVal& location, bool isLoad, const clang::Stmt* S, CheckerContext& C) const
{
    // llvm::outs() << "got location to check: ";
    // S->dump(llvm::outs());
}
