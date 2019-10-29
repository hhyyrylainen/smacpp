// ------------------------------------ //
#include "MainASTConsumer.h"

#include "CodeBlockBuildingVisitor.h"
#include "analysis/BlockRegistry.h"

using namespace smacpp;
// ------------------------------------ //
void MainASTConsumer::HandleTranslationUnit(clang::ASTContext& Context)
{
    clang::DiagnosticsEngine& de = Context.getDiagnostics();

    RegisterDiagnostics(de);

    BlockRegistry registry;
    CodeBlockBuildingVisitor visitor(Context, registry, DebugPrint);

    // Traversing the translation unit decl via a RecursiveASTVisitor
    // will visit all nodes in the AST.
    visitor.TraverseDecl(Context.getTranslationUnitDecl());

    // The traversal creates all the CodeBlocks in this TU
    // This analysis here can only find problems within this TU as it only has the current TU's
    // CodeBlocks loaded
    const auto errors = registry.PerformAnalysis();

    for(const auto& error : errors) {
        if(error.Severity == FoundProblem::SEVERITY::Error) {
            de.Report(error.Location, SMACPPErrorId).AddString(error.Message);
        } else {
            // TODO: use the proper clang error output mechanism
            llvm::errs() << "smacpp: " << error.FormatAsString() << "\n";
        }
    }
}
// ------------------------------------ //
void MainASTConsumer::RegisterDiagnostics(clang::DiagnosticsEngine& de)
{
    SMACPPErrorId = de.getCustomDiagID(clang::DiagnosticsEngine::Error, "%0");
}
