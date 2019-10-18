#pragma once

#include "CodeBlock.h"

#include "clang/AST/RecursiveASTVisitor.h"

namespace smacpp {
//! Creates CodeBlock from AST and stores them for overall program analysis
class CodeBlockBuildingVisitor : public clang::RecursiveASTVisitor<CodeBlockBuildingVisitor> {

    class ValueVisitBase {
    public:
        ValueVisitBase(clang::ASTContext& context) : Context(context) {}

        bool VisitVarDecl(clang::VarDecl* var)
        {
            clang::FullSourceLoc fullLocation = Context.getFullLoc(var->getBeginLoc());

            if(!fullLocation.isValid())
                return true;

            if(clang::dyn_cast<clang::ParmVarDecl>(var))
                return true;

            // if(fullLocation.isInSystemHeader())
            //     return true;

            const std::string varName = var->getQualifiedNameAsString();
            const std::string varType = var->getType().getAsString();

            const auto* value = var->getAnyInitializer();

            llvm::outs() << "local var: " << varType << " " << varName << " init: ";

            if(value) {

                if(value->getStmtClass() == clang::Stmt::StmtClass::StringLiteralClass) {
                    const auto* literal = static_cast<const clang::StringLiteral*>(value);

                    llvm::outs() << "string literal('" << literal->getBytes() << "')";
                } else {
                    llvm::outs() << "unknown initializer type";
                }
            } else {
                llvm::outs() << "uninitialized";
            }

            llvm::outs() << "\n";

            // var->dump();

            // if(fullLocation.isValid())
            //     llvm::outs() << "Found vardecl at " << fullLocation.getSpellingLineNumber()
            //     <<
            //     ":"
            //                  << fullLocation.getSpellingColumnNumber() << "\n";


            return true;
        }


        bool TraverseIfStmt(clang::IfStmt* stmt)
        {
            Condition condition;
            Condition negated;

            try {
                condition = Condition(stmt->getCond());
                negated = condition.Negate();

            } catch(const std::exception& e) {
                llvm::outs() << "Failed to parse condition, exception: " << e.what() << "\n";
                return true;
            }

            llvm::outs() << "Condition: " << condition.Dump() << "\n";
            llvm::outs() << "Combined with current: "
                         << GetCurrentCondition().And(condition).Dump() << "\n";
            llvm::outs() << "Negated: " << negated.Dump() << "\n";

            if(!negated.IsAlwaysTrue()) {
                ConditionalContentVisitor visitor(
                    GetCurrentCondition().And(condition), Context);
                llvm::outs() << "Sub-visiting then\n";
                visitor.TraverseStmt(stmt->getThen());
            }

            if(!condition.IsAlwaysTrue()) {
                ConditionalContentVisitor visitor(GetCurrentCondition().And(negated), Context);
                llvm::outs() << "Sub-visiting else\n";
                visitor.TraverseStmt(stmt->getElse());
            }

            return true;
        }

        bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* expr)
        {
            const auto* index = expr->getIdx();

            if(!index)
                return true;

            if(index->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass) {
                const auto* literal = static_cast<const clang::IntegerLiteral*>(index);

                llvm::outs() << "used array index: " << literal->getValue() << "\n";
            } else {
                llvm::outs() << "unknown array subscript index\n";
            }

            // TODO: a sub visitor for getting to DeclRefExpr

            return true;
        }

        bool VisitBinaryOperator(clang::BinaryOperator* op)
        {
            if(clang::BO_Assign != op->getOpcode())
                return true;

            llvm::outs() << "Assignment found: ";
            op->dump();

            // TODO: use a sub visitor on LHS and RHS to fish out the DeclRefExpr to see what
            // they refer to

            return true;
        }

        virtual Condition GetCurrentCondition() const
        {
            return Condition();
        }

    protected:
        clang::ASTContext& Context;
    };

#define VALUE_VISITOR_VISIT_TYPES                                 \
    bool VisitVarDecl(clang::VarDecl* var)                        \
    {                                                             \
        return ValueVisitBase::VisitVarDecl(var);                 \
    }                                                             \
    bool TraverseIfStmt(clang::IfStmt* stmt)                      \
    {                                                             \
        return ValueVisitBase::TraverseIfStmt(stmt);              \
    }                                                             \
    bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* expr) \
    {                                                             \
        return ValueVisitBase::VisitArraySubscriptExpr(expr);     \
    }                                                             \
    bool VisitBinaryOperator(clang::BinaryOperator* op)           \
    {                                                             \
        return ValueVisitBase::VisitBinaryOperator(op);           \
    }

    //! \brief Visits stuff under an if statement to take conditions into account
    class ConditionalContentVisitor
        : public clang::RecursiveASTVisitor<ConditionalContentVisitor>,
          public ValueVisitBase {
    public:
        ConditionalContentVisitor(Condition cond, clang::ASTContext& context) :
            ValueVisitBase(context), Cond(cond)
        {}

        VALUE_VISITOR_VISIT_TYPES;

        Condition GetCurrentCondition() const override
        {
            return Cond;
        }

    private:
        Condition Cond;
    };

    //! \brief Builds a code block from a function definition
    class FunctionVisitor : public clang::RecursiveASTVisitor<FunctionVisitor>,
                            public ValueVisitBase {
    public:
        FunctionVisitor(clang::ASTContext& context) : ValueVisitBase(context) {}

        bool VisitParmVarDecl(clang::ParmVarDecl* var)
        {
            llvm::outs() << "function param: " << var->getQualifiedNameAsString() << "\n";
            return true;
        }

        VALUE_VISITOR_VISIT_TYPES;

        CodeBlock Block;
    };

public:
    CodeBlockBuildingVisitor(clang::ASTContext& context) : Context(context) {}

    // TODO: remove, example code
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* Declaration)
    {
        // For debugging, dumping the AST nodes will show which nodes are already
        // being visited.
        Declaration->dump();

        clang::FullSourceLoc fullLocation = Context.getFullLoc(Declaration->getBeginLoc());
        if(fullLocation.isValid())
            llvm::outs() << "Found declaration at " << fullLocation.getSpellingLineNumber()
                         << ":" << fullLocation.getSpellingColumnNumber() << "\n";

        // The return value indicates whether we want the visitation to proceed.
        // Return false to stop the traversal of the AST.
        return true;
    }

    //! \note Using traverse blocks any child nodes from being visited
    bool TraverseFunctionDecl(clang::FunctionDecl* fun)
    {
        // This is split in two to easily detect the function end
        llvm::outs() << "Visiting func: " << fun->getDeclName() << "\n";
        FunctionVisitor Visitor(Context);
        Visitor.TraverseDecl(fun);

        llvm::outs() << "function ended\n";
        // TODO: save the created code block
        // Visitor.Block;
        return true;
    }

private:
    clang::ASTContext& Context;
};

} // namespace smacpp
