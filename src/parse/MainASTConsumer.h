#pragma once

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"

namespace smacpp {

class MainASTConsumer : public clang::ASTConsumer {
public:
    MainASTConsumer() = default;

    virtual void HandleTranslationUnit(clang::ASTContext& Context);

protected:
    void RegisterDiagnostics(clang::DiagnosticsEngine& de);

protected:
    unsigned SMACPPErrorId;
};
} // namespace smacpp
