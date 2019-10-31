#pragma once

#include "CodeBlock.h"

#include "clang/AST/RecursiveASTVisitor.h"

namespace smacpp {

class BlockRegistry;

//! Creates CodeBlock from AST and stores them for overall program analysis
class CodeBlockBuildingVisitor : public clang::RecursiveASTVisitor<CodeBlockBuildingVisitor> {

    //! \brief Looks for a variable reference or an array subscript to a variable
    class VariableRefOrArrayVisitor;

    //! \brief Makes a VariableState from an Expr
    class VariableStateFindVisitor;

    class ValueVisitBase {
    public:
        ValueVisitBase(clang::ASTContext& context, CodeBlock& target, bool debug);

        bool VisitVarDecl(clang::VarDecl* var);

        bool TraverseIfStmt(clang::IfStmt* stmt);

        bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* expr);

        bool VisitBinaryOperator(clang::BinaryOperator* op);

        bool TraverseCallExpr(clang::CallExpr* call);

        virtual Condition GetCurrentCondition() const
        {
            return Condition();
        }

    protected:
        clang::ASTContext& Context;

        CodeBlock& Target;

        bool Debug;
    };

    //! \brief Visits stuff under an if statement to take conditions into account
    class ConditionalContentVisitor;

    //! \brief Builds a code block from a function definition
    class FunctionVisitor;

public:
    CodeBlockBuildingVisitor(clang::ASTContext& context, BlockRegistry& registry, bool debug);

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
    bool TraverseFunctionDecl(clang::FunctionDecl* fun);

private:
    clang::ASTContext& Context;
    BlockRegistry& Registry;
    bool Debug;
};

} // namespace smacpp
