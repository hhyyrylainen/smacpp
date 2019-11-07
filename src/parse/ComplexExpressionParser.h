#pragma once

#include "LiteralStateVisitor.h"
#include "Variable.h"

#include "clang/AST/RecursiveASTVisitor.h"

namespace smacpp {

//! Finds a literal value and makes a VariableState out of it
class ComplexExpressionParser : public clang::RecursiveASTVisitor<ComplexExpressionParser> {
public:
    ComplexExpressionParser(bool debug) : Debug(debug) {}

    // No clue why traverse doesn't work here
    bool VisitBinaryOperator(clang::BinaryOperator* op)
    {
        if(Debug)
            llvm::outs() << "Traversing binary operator with complex parser\n";

        clang::Expr* lhs = op->getLHS();
        clang::Expr* rhs = op->getRHS();

        if(!lhs || !rhs)
            return true;

        if(op->getReferencedDeclOfCallee()) {
            if(Debug) {
                llvm::outs() << "binary operator has decl of callee, this is not handled: ";
                llvm::outs() << "decl of callee: ";
                op->getReferencedDeclOfCallee()->dump();
            }
            return true;
        }

        llvm::outs() << "lhs visiting: ";
        lhs->dump();

        ComplexExpressionParser lhsVisitor(Debug);
        lhsVisitor.TraverseStmt(lhs);

        llvm::outs() << "rhs visiting: ";
        rhs->dump();

        ComplexExpressionParser rhsVisitor(Debug);
        rhsVisitor.TraverseStmt(rhs);

        const auto lhsConstant = LiteralFromExpr(lhs);
        const auto rhsConstant = LiteralFromExpr(rhs);

        if(!lhsVisitor.ParsedState && lhsConstant)
            lhsVisitor.ParsedState = lhsConstant;

        if(!rhsVisitor.ParsedState && rhsConstant)
            rhsVisitor.ParsedState = rhsConstant;

        if(lhsVisitor.ParsedState && rhsVisitor.ParsedState) {

            std::optional<OPERATOR> basicOperator;

            switch(op->getOpcode()) {
            case clang::BO_Add: basicOperator = OPERATOR::Add; break;
            case clang::BO_Mul: basicOperator = OPERATOR::Multiply; break;
            case clang::BO_Sub: basicOperator = OPERATOR::Subtract; break;
            default:
                llvm::outs() << "unknown binary operator opcode in expression parser: "
                             << op->getOpcode() << "\n";
                return false;
            }

            if(basicOperator) {

                llvm::outs() << "lhs state is: " << lhsVisitor.ParsedState->Dump() << "\n";
                llvm::outs() << "rhs state is: " << rhsVisitor.ParsedState->Dump() << "\n";

                ParsedState = lhsVisitor.ParsedState->CreateOperatorApplyingState(
                    *basicOperator, *rhsVisitor.ParsedState);
                llvm::outs() << "parsed state is: " << ParsedState->Dump() << "\n";
            }

            return false;
        }

        llvm::outs()
            << "binary operator has lhs and rhs that couldn't be combined to a State\n";

        return false;
    }

    bool VisitDeclRefExpr(clang::DeclRefExpr* expr)
    {
        if(clang::VarDecl* var = clang::dyn_cast<clang::VarDecl>(expr->getDecl()); var) {

            VariableIdentifier ident(var);

            if(Debug)
                llvm::outs() << "found var reference: " << ident.Dump() << "\n";

            VariableState state;
            state.Set(VarCopyInfo(ident));
            ParsedState = state;

        } else {
            if(Debug)
                llvm::outs() << "found unknown reference type\n";
        }

        return true;
    }

    bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* expr)
    {
        return true;
    }

    std::optional<VariableState> LiteralFromExpr(clang::Expr* expr)
    {
        LiteralStateVisitor visitor;
        visitor.TraverseStmt(expr);

        return visitor.FoundValue;
    }


    std::optional<VariableState> ParsedState;

protected:
    bool Debug = false;
};
} // namespace smacpp
