// ------------------------------------ //
#include "CodeBlockBuildingVisitor.h"

#include "ProcessedAction.h"
#include "analysis/BlockRegistry.h"

#include <optional>

using namespace smacpp;
// ------------------------------------ //
// CodeBlockBuildingVisitor::VariableRefOrArrayVisitor
class CodeBlockBuildingVisitor::VariableRefOrArrayVisitor
    : public clang::RecursiveASTVisitor<VariableRefOrArrayVisitor> {
public:
    bool VisitDeclRefExpr(clang::DeclRefExpr* expr)
    {
        if(clang::VarDecl* var = clang::dyn_cast<clang::VarDecl>(expr->getDecl()); var) {

            VariableIdentifier ident(var);

            llvm::outs() << "found var reference: " << ident.Dump() << "\n";

            if(LikelyFullVariableAssign) {
                FoundVar = ident;
            }

        } else {
            llvm::outs() << "found unknown reference\n";
        }


        return true;
    }

    bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr* expr)
    {
        LikelyFullVariableAssign = false;
        return true;
    }

    std::optional<VariableIdentifier> FoundVar;

private:
    bool LikelyFullVariableAssign = true;
};


// ------------------------------------ //
// CodeBlockBuildingVisitor::VariableRefOrArrayVisitor
class CodeBlockBuildingVisitor::VariableStateFindVisitor
    : public clang::RecursiveASTVisitor<VariableStateFindVisitor> {
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
        state.Set(PrimitiveInfo(value->getValue().getSExtValue()));
        FoundValue = state;
        return false;
    }

    std::optional<VariableState> FoundValue;
};


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
    }                                                             \
    bool TraverseCallExpr(clang::CallExpr* call)                  \
    {                                                             \
        return ValueVisitBase::TraverseCallExpr(call);            \
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
        // llvm::outs() << "function param: " << var->getQualifiedNameAsString() << "\n";

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
                                  GetCurrentCondition(), VariableIdentifier(var), state),
        fullLocation);

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

    VariableRefOrArrayVisitor lhsVisitor;
    lhsVisitor.TraverseStmt(expr->getLHS());

    if(!lhsVisitor.FoundVar)
        return true;

    llvm::outs() << "found array access for variable: " << lhsVisitor.FoundVar->Dump() << "\n";

    VariableState indexValue;

    if(index->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass) {
        const auto* literal = static_cast<const clang::IntegerLiteral*>(index);

        llvm::outs() << "used array index: " << literal->getValue() << "\n";

        // TODO: this can only handle 64 bit numbers, anything higher will cause an error
        indexValue.Set(PrimitiveInfo(literal->getValue().getSExtValue()));

    } else {
        llvm::outs() << "unknown array subscript index\n";
    }

    if(indexValue.State != VariableState::STATE::Unknown) {
        Target.AddProcessedAction(std::make_unique<action::ArrayIndexAccess>(
                                      GetCurrentCondition(), *lhsVisitor.FoundVar, indexValue),
            Context.getFullLoc(expr->getBeginLoc()));
    }

    return true;
}

bool CodeBlockBuildingVisitor::ValueVisitBase::VisitBinaryOperator(clang::BinaryOperator* op)
{
    if(clang::BO_Assign != op->getOpcode())
        return true;

    // llvm::outs() << "Assignment found: ";
    // op->dump();

    VariableRefOrArrayVisitor lhsVisitor;
    lhsVisitor.TraverseStmt(op->getLHS());

    VariableRefOrArrayVisitor rhsVisitor;
    rhsVisitor.TraverseStmt(op->getRHS());

    if(lhsVisitor.FoundVar && rhsVisitor.FoundVar) {
        llvm::outs() << "Assignment found: " << lhsVisitor.FoundVar->Dump() << " = "
                     << rhsVisitor.FoundVar->Dump() << "\n";

        VariableState state;
        state.Set(*rhsVisitor.FoundVar);

        // TODO: the location here is not fully accurate, the sub visitor needs to store the
        // accurate location
        Target.AddProcessedAction(std::make_unique<action::VarAssigned>(
                                      GetCurrentCondition(), *lhsVisitor.FoundVar, state),
            Context.getFullLoc(op->getBeginLoc()));
    }

    return true;
}

bool CodeBlockBuildingVisitor::ValueVisitBase::TraverseCallExpr(clang::CallExpr* call)
{
    if(!call->getDirectCallee())
        return true;

    const auto functionName = call->getDirectCallee()->getQualifiedNameAsString();

    // llvm::outs() << "func call: " << functionName << "\n";

    // llvm::outs() << "arg count: " <<  << "\n";

    std::vector<VariableState> callParams;

    for(size_t i = 0; i < call->getNumArgs(); ++i) {
        clang::Expr* arg = call->getArg(i);

        // llvm::outs() << "visiting arg(" << i << "): ";
        // arg->dump();
        VariableStateFindVisitor visitor;
        visitor.TraverseStmt(arg);

        if(visitor.FoundValue) {
            callParams.push_back(*visitor.FoundValue);
        } else {
            callParams.push_back(VariableState());
        }
    }

    Target.AddProcessedAction(std::make_unique<action::FunctionCall>(
                                  GetCurrentCondition(), functionName, callParams),
        Context.getFullLoc(call->getBeginLoc()));

    return true;
}
// ------------------------------------ //
// CodeBlockBuildingVisitor
CodeBlockBuildingVisitor::CodeBlockBuildingVisitor(
    clang::ASTContext& context, BlockRegistry& registry) :
    Context(context),
    Registry(registry)
{}
// ------------------------------------ //
bool CodeBlockBuildingVisitor::TraverseFunctionDecl(clang::FunctionDecl* fun)
{
    CodeBlock block(fun->getQualifiedNameAsString(), Context.getFullLoc(fun->getBeginLoc()));
    // This is split in two to easily detect the function end

    FunctionVisitor Visitor(Context, block);
    Visitor.TraverseDecl(fun);

    llvm::outs() << "completed block: " << block.Dump() << "\n";

    Registry.AddBlock(std::move(block));
    return true;
}
