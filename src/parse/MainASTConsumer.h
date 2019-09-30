#pragma once

#include "CodeBlockBuildingVisitor.h"

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"

namespace smacpp {

class MainASTConsumer : public clang::ASTConsumer {
public:
    MainASTConsumer(clang::ASTContext& context) : Visitor(context) {}

    virtual void HandleTranslationUnit(clang::ASTContext& Context)
    {
        // Traversing the translation unit decl via a RecursiveASTVisitor
        // will visit all nodes in the AST.
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    CodeBlockBuildingVisitor Visitor;
};
} // namespace smacpp
