#pragma once

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"

namespace smacpp {

class MainASTConsumer : public clang::ASTConsumer {
public:
    MainASTConsumer(bool debugPrint) : DebugPrint(debugPrint) {}

    virtual void HandleTranslationUnit(clang::ASTContext& Context);

protected:
    void RegisterDiagnostics(clang::DiagnosticsEngine& de);

protected:
    unsigned SMACPPErrorId;
    bool DebugPrint;
};
} // namespace smacpp
