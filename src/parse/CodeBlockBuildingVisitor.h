#pragma once

#include "clang/AST/RecursiveASTVisitor.h"

namespace smacpp {
class CodeBlockBuildingVisitor : public clang::RecursiveASTVisitor<CodeBlockBuildingVisitor> {
public:
    CodeBlockBuildingVisitor(clang::ASTContext& context) : Context(context) {}

    bool VisitCXXRecordDecl(clang::CXXRecordDecl* Declaration)
    {
        // For debugging, dumping the AST nodes will show which nodes are already
        // being visited.
        Declaration->dump();

        clang::FullSourceLoc FullLocation = Context.getFullLoc(Declaration->getBeginLoc());
        if(FullLocation.isValid())
            llvm::outs() << "Found declaration at " << FullLocation.getSpellingLineNumber()
                         << ":" << FullLocation.getSpellingColumnNumber() << "\n";

        // The return value indicates whether we want the visitation to proceed.
        // Return false to stop the traversal of the AST.
        return true;
    }

private:
    clang::ASTContext& Context;
};

} // namespace smacpp
