#pragma once

#include "Variable.h"

#include "clang/AST/RecursiveASTVisitor.h"

namespace smacpp {

//! Finds a literal value and makes a VariableState out of it
class LiteralStateVisitor : public clang::RecursiveASTVisitor<LiteralStateVisitor> {
public:
    // Parent type of all literals
    // bool VisitExpr(clang::Expr* expr)
    // {
    //     return true;
    // }

    bool TraverseIntegerLiteral(clang::IntegerLiteral* value)
    {
        VariableState state;

        // TODO: this can only handle 64 bit numbers, anything higher will cause an error
        state.Set(PrimitiveInfo(value->getValue().getSExtValue()));
        FoundValue = state;
        return false;
    }

    std::optional<VariableState> FoundValue;
};
} // namespace smacpp
