// ------------------------------------ //
#include "CodeBlockBuildingVisitor.h"

#include "ComplexExpressionParser.h"
#include "LiteralStateVisitor.h"
#include "ProcessedAction.h"
#include "analysis/BlockRegistry.h"

#include <optional>

using namespace smacpp;
// ------------------------------------ //
// CodeBlockBuildingVisitor::VariableRefOrArrayVisitor
class CodeBlockBuildingVisitor::VariableRefOrArrayVisitor
    : public clang::RecursiveASTVisitor<VariableRefOrArrayVisitor> {
public:
    VariableRefOrArrayVisitor(bool debug) : Debug(debug) {}

    bool VisitDeclRefExpr(clang::DeclRefExpr* expr)
    {
        if(clang::VarDecl* var = clang::dyn_cast<clang::VarDecl>(expr->getDecl()); var) {

            VariableIdentifier ident(var);

            if(Debug)
                llvm::outs() << "found var reference: " << ident.Dump() << "\n";

            if(LikelyFullVariableAssign) {
                FoundVar = ident;
            }

        } else {
            if(Debug)
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
    bool Debug;
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
    bool TraverseSwitchStmt(clang::SwitchStmt* stmt)              \
    {                                                             \
        return ValueVisitBase::TraverseSwitchStmt(stmt);          \
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
    ConditionalContentVisitor(
        Condition cond, clang::ASTContext& context, CodeBlock& target, bool debug) :
        ValueVisitBase(context, target, debug),
        Cond(cond)
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
    FunctionVisitor(clang::ASTContext& context, CodeBlock& target, bool debug) :
        ValueVisitBase(context, target, debug)
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
// CodeBlockBuildingVisitor::CaseConditionalVisitor
class CodeBlockBuildingVisitor::CaseConditionalVisitor
    : public clang::RecursiveASTVisitor<CaseConditionalVisitor>,
      public ValueVisitBase {
public:
    CaseConditionalVisitor(Condition baseCondition, VariableIdentifier switchVar,
        clang::ASTContext& context, CodeBlock& target, bool debug) :
        ValueVisitBase(context, target, debug),
        BaseCondition(baseCondition), SwitchVar(switchVar)
    {}

    bool VisitCaseStmt(clang::CaseStmt* stmt)
    {
        if(!stmt->getLHS()) {
            llvm::outs() << "Case statement without LHS";
            return true;
        }

        LiteralStateVisitor literal;
        literal.TraverseStmt(stmt->getLHS());

        if(!literal.FoundValue) {
            if(Debug) {
                llvm::outs() << "Could not parse switch case value: ";
                stmt->getLHS()->dump();
            }
            return true;
        }

        Condition newCondition(Condition::Part(VariableValueCondition(
            SwitchVar, ValueRange(COMPARISON::EQUAL, *literal.FoundValue))));

        if(newCondition.IsAlwaysTrue()) {
            llvm::outs() << "Logic error in case stmt parsing, condition is always true\n";
            return true;
        }

        SwitchCases.push_back(newCondition);
        if(CurrentSwitchCondition.IsAlwaysTrue()) {
            CurrentSwitchCondition = newCondition;
        } else {
            CurrentSwitchCondition = CurrentSwitchCondition.Or(newCondition);
        }

        if(Debug) {
            llvm::outs() << "Current switch condition is: " << CurrentSwitchCondition.Dump()
                         << "\n";
        }

        return true;
    }

    bool VisitBreakStmt(clang::BreakStmt* stmt)
    {
        // TODO: this needs to detect if there is a loop inside this case statement or not
        CurrentSwitchCondition = Condition();

        if(Debug) {
            llvm::outs() << "Hit case break\n";
        }
        return true;
    }

    bool VisitDefaultStmt(clang::DefaultStmt* stmt)
    {
        // TODO: this only works correctly if the default statement is last

        Condition defaultCondition;

        for(const auto& cond : SwitchCases) {
            if(defaultCondition.IsAlwaysTrue()) {
                defaultCondition = cond.Negate();
            } else {
                defaultCondition = defaultCondition.And(cond.Negate());
            }
        }

        if(defaultCondition.IsAlwaysTrue()) {
            llvm::outs()
                << "Logic error in case stmt parsing, default condition is always true\n";
            return true;
        }

        if(CurrentSwitchCondition.IsAlwaysTrue()) {
            CurrentSwitchCondition = defaultCondition;
        } else {
            CurrentSwitchCondition = CurrentSwitchCondition.Or(defaultCondition);
        }

        if(Debug) {
            llvm::outs() << "Default switch case condition is: "
                         << CurrentSwitchCondition.Dump() << "\n";
        }

        return true;
    }

    VALUE_VISITOR_VISIT_TYPES;

    Condition GetCurrentCondition() const override
    {
        return BaseCondition.And(CurrentSwitchCondition);
    }

protected:
    Condition BaseCondition;
    Condition CurrentSwitchCondition;
    std::vector<Condition> SwitchCases;
    VariableIdentifier SwitchVar;
};
// ------------------------------------ //
// CodeBlockBuildingVisitor::ValueVisitBase
CodeBlockBuildingVisitor::ValueVisitBase::ValueVisitBase(
    clang::ASTContext& context, CodeBlock& target, bool debug) :
    Context(context),
    Target(target), Debug(debug)
{}

bool CodeBlockBuildingVisitor::ValueVisitBase::VisitVarDecl(clang::VarDecl* var)
{
    clang::FullSourceLoc fullLocation = Context.getFullLoc(var->getBeginLoc());

    // if(!fullLocation.isValid())
    //     return true;

    if(clang::dyn_cast<clang::ParmVarDecl>(var))
        return true;

    // if(fullLocation.isInSystemHeader())
    //     return true;

    VariableState state;

    const std::string varName = var->getQualifiedNameAsString();
    const std::string varType = var->getType().getAsString();

    const auto* value = var->getAnyInitializer();

    if(Debug)
        llvm::outs() << "local var: " << varType << " " << varName << " init: ";

    if(value) {

        if(value->getStmtClass() == clang::Stmt::StmtClass::StringLiteralClass) {
            const auto* literal = static_cast<const clang::StringLiteral*>(value);

            if(Debug)
                llvm::outs() << "string literal('" << literal->getBytes() << "')";
            state.Set(BufferInfo(literal->getByteLength()));
        } else {
            if(Debug)
                llvm::outs() << "unknown initializer type";
        }
    } else {
        if(Debug)
            llvm::outs() << "uninitialized";
    }

    if(Debug)
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

    if(Debug)
        llvm::outs() << "Condition: " << condition.Dump() << "\n"
                     << "Combined with current: "
                     << GetCurrentCondition().And(condition).Dump() << "\n"
                     << "Negated: " << negated.Dump() << "\n";

    if(!negated.IsAlwaysTrue()) {
        ConditionalContentVisitor visitor(
            GetCurrentCondition().And(condition), Context, Target, Debug);
        visitor.TraverseStmt(stmt->getThen());
    }

    if(!condition.IsAlwaysTrue()) {
        ConditionalContentVisitor visitor(
            GetCurrentCondition().And(negated), Context, Target, Debug);
        visitor.TraverseStmt(stmt->getElse());
    }

    return true;
}

bool CodeBlockBuildingVisitor::ValueVisitBase::TraverseSwitchStmt(clang::SwitchStmt* stmt)
{
    std::optional<VariableIdentifier> var;

    if(stmt->getConditionVariable()) {

        // TODO: this needs to parse the initial value for the variable and add a state set
        var = VariableIdentifier(stmt->getConditionVariable());

        return true;
    } else if(stmt->getCond()) {

        VariableRefOrArrayVisitor visitor(Debug);
        visitor.TraverseStmt(stmt->getCond());

        if(!visitor.FoundVar) {
            llvm::outs() << "Could not find switch variable\n";
            return true;
        }

        var = visitor.FoundVar;

    } else {
        if(Debug)
            llvm::outs() << "Switch has no variable we can understand\n";
    }

    if(Debug)
        llvm::outs() << "Traversing switch on variable: " << var->Dump() << "\n";

    CaseConditionalVisitor visitor(GetCurrentCondition(), *var, Context, Target, Debug);
    visitor.TraverseStmt(stmt->getBody());

    return true;
}

bool CodeBlockBuildingVisitor::ValueVisitBase::VisitArraySubscriptExpr(
    clang::ArraySubscriptExpr* expr)
{
    const auto* index = expr->getIdx();

    if(!index)
        return true;

    VariableRefOrArrayVisitor lhsVisitor(Debug);
    lhsVisitor.TraverseStmt(expr->getLHS());

    if(!lhsVisitor.FoundVar)
        return true;

    if(Debug)
        llvm::outs() << "found array access for variable: " << lhsVisitor.FoundVar->Dump()
                     << "\n";

    VariableState indexValue;

    if(index->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass) {
        const auto* literal = static_cast<const clang::IntegerLiteral*>(index);

        if(Debug)
            llvm::outs() << "used array index: " << literal->getValue() << "\n";

        // TODO: this can only handle 64 bit numbers, anything higher will cause an error
        indexValue.Set(PrimitiveInfo(literal->getValue().getSExtValue()));

    } else {
        if(Debug)
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

    // if(Debug) {
    //     llvm::outs() << "Assignment statement found: ";
    //     op->dump();
    // }

    VariableRefOrArrayVisitor lhsVisitor(Debug);
    lhsVisitor.TraverseStmt(op->getLHS());

    if(!lhsVisitor.FoundVar)
        return true;

    ComplexExpressionParser rhsVisitor(Debug);
    rhsVisitor.TraverseStmt(op->getRHS());

    if(!rhsVisitor.ParsedState) {
        // Assign from literal
        LiteralStateVisitor literalVisitor;
        literalVisitor.TraverseStmt(op->getRHS());

        if(literalVisitor.FoundValue)
            rhsVisitor.ParsedState = literalVisitor.FoundValue;
    }

    // Simple variable to variable assign
    if(rhsVisitor.ParsedState) {
        if(Debug)
            llvm::outs() << "Assignment found: " << lhsVisitor.FoundVar->Dump() << " = "
                         << rhsVisitor.ParsedState->Dump() << "\n";

        // TODO: the location here is not fully accurate, the sub visitor needs to store the
        // accurate location
        Target.AddProcessedAction(std::make_unique<action::VarAssigned>(GetCurrentCondition(),
                                      *lhsVisitor.FoundVar, *rhsVisitor.ParsedState),
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
        LiteralStateVisitor visitor;
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
    clang::ASTContext& context, BlockRegistry& registry, bool debug) :
    Context(context),
    Registry(registry), Debug(debug)
{}
// ------------------------------------ //
bool CodeBlockBuildingVisitor::TraverseFunctionDecl(clang::FunctionDecl* fun)
{
    CodeBlock block(fun->getQualifiedNameAsString(), Context.getFullLoc(fun->getBeginLoc()));
    // This is split in two to easily detect the function end

    FunctionVisitor Visitor(Context, block, Debug);
    Visitor.TraverseDecl(fun);

    if(Debug)
        llvm::outs() << "completed block: " << block.Dump() << "\n";

    Registry.AddBlock(std::move(block));
    return true;
}
