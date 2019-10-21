// ------------------------------------ //
#include "CodeBlockBuildingVisitor.h"

#include "ProcessedAction.h"

using namespace smacpp;
// ------------------------------------ //
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

// ------------------------------------ //
// CodeBlockBuildingVisitor::ConditionalContentVisitor
class CodeBlockBuildingVisitor::ConditionalContentVisitor
    : public clang::RecursiveASTVisitor<ConditionalContentVisitor>,
      public ValueVisitBase {
public:
    ConditionalContentVisitor(Condition cond, clang::ASTContext& context, CodeBlock& target) :
        ValueVisitBase(context, target), Cond(cond)
    {}

    VALUE_VISITOR_VISIT_TYPES;

    Condition GetCurrentCondition() const override
    {
        return Cond;
    }

private:
    Condition Cond;
};
// ------------------------------------ //
// CodeBlockBuildingVisitor::FunctionVisitor
class CodeBlockBuildingVisitor::FunctionVisitor
    : public clang::RecursiveASTVisitor<FunctionVisitor>,
      public ValueVisitBase {
public:
    FunctionVisitor(clang::ASTContext& context, CodeBlock& target) :
        ValueVisitBase(context, target)
    {}

    bool VisitParmVarDecl(clang::ParmVarDecl* var)
    {
        llvm::outs() << "function param: " << var->getQualifiedNameAsString() << "\n";

        Target.AddFunctionParameter(VariableIdentifier(var));

        return true;
    }

    VALUE_VISITOR_VISIT_TYPES;
};
// ------------------------------------ //
// CodeBlockBuildingVisitor::ValueVisitBase
CodeBlockBuildingVisitor::ValueVisitBase::ValueVisitBase(
    clang::ASTContext& context, CodeBlock& target) :
    Context(context),
    Target(target)
{}

bool CodeBlockBuildingVisitor::ValueVisitBase::VisitVarDecl(clang::VarDecl* var)
{
    clang::FullSourceLoc fullLocation = Context.getFullLoc(var->getBeginLoc());

    if(!fullLocation.isValid())
        return true;

    if(clang::dyn_cast<clang::ParmVarDecl>(var))
        return true;

    // if(fullLocation.isInSystemHeader())
    //     return true;

    VariableState state;

    const std::string varName = var->getQualifiedNameAsString();
    const std::string varType = var->getType().getAsString();

    const auto* value = var->getAnyInitializer();

    llvm::outs() << "local var: " << varType << " " << varName << " init: ";

    if(value) {

        if(value->getStmtClass() == clang::Stmt::StmtClass::StringLiteralClass) {
            const auto* literal = static_cast<const clang::StringLiteral*>(value);

            llvm::outs() << "string literal('" << literal->getBytes() << "')";
            state.Set(BufferInfo(literal->getByteLength()));
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
    Target.AddProcessedAction(std::make_unique<action::VarDeclared>(
        GetCurrentCondition(), VariableIdentifier(var), state));

    return true;
}


bool CodeBlockBuildingVisitor::ValueVisitBase::TraverseIfStmt(clang::IfStmt* stmt)
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
    llvm::outs() << "Combined with current: " << GetCurrentCondition().And(condition).Dump()
                 << "\n";
    llvm::outs() << "Negated: " << negated.Dump() << "\n";

    if(!negated.IsAlwaysTrue()) {
        ConditionalContentVisitor visitor(
            GetCurrentCondition().And(condition), Context, Target);
        llvm::outs() << "Sub-visiting then\n";
        visitor.TraverseStmt(stmt->getThen());
    }

    if(!condition.IsAlwaysTrue()) {
        ConditionalContentVisitor visitor(GetCurrentCondition().And(negated), Context, Target);
        llvm::outs() << "Sub-visiting else\n";
        visitor.TraverseStmt(stmt->getElse());
    }

    return true;
}

bool CodeBlockBuildingVisitor::ValueVisitBase::VisitArraySubscriptExpr(
    clang::ArraySubscriptExpr* expr)
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

bool CodeBlockBuildingVisitor::ValueVisitBase::VisitBinaryOperator(clang::BinaryOperator* op)
{
    if(clang::BO_Assign != op->getOpcode())
        return true;

    llvm::outs() << "Assignment found: ";
    op->dump();

    // TODO: use a sub visitor on LHS and RHS to fish out the DeclRefExpr to see what
    // they refer to

    return true;
}
// ------------------------------------ //
// CodeBlockBuildingVisitor
CodeBlockBuildingVisitor::CodeBlockBuildingVisitor(clang::ASTContext& context) :
    Context(context)
{}
// ------------------------------------ //
bool CodeBlockBuildingVisitor::TraverseFunctionDecl(clang::FunctionDecl* fun)
{
    CodeBlock block(fun->getQualifiedNameAsString());
    // This is split in two to easily detect the function end

    FunctionVisitor Visitor(Context, block);
    Visitor.TraverseDecl(fun);

    llvm::outs() << "completed block: " << block.Dump() << "\n";

    // TODO: save the created code block
    return true;
}
