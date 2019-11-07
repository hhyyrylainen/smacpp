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
        state.Set(PrimitiveInfo(ApplyCurrentEffects(value->getValue().getSExtValue())));
        FoundValue = state;
        return false;
    }

    bool VisitUnaryOperator(clang::UnaryOperator* op)
    {
        if(op->getOpcode() == clang::UO_Minus) {
            Negate = !Negate;
        }

        return true;
    }

    template<typename T>
    T ApplyCurrentEffects(const T& value)
    {
        if(Negate) {
            Negate = false;
            return -value;
        }
        return value;
    }

    std::optional<VariableState> FoundValue;

protected:
    bool Negate = false;
};
} // namespace smacpp
