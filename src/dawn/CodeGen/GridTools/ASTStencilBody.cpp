//===--------------------------------------------------------------------------------*- C++ -*-===//
//                          _
//                         | |
//                       __| | __ ___      ___ ___
//                      / _` |/ _` \ \ /\ / / '_  |
//                     | (_| | (_| |\ V  V /| | | |
//                      \__,_|\__,_| \_/\_/ |_| |_| - Compiler Toolchain
//
//
//  This file is distributed under the MIT License (MIT).
//  See LICENSE.txt for details.
//
//===------------------------------------------------------------------------------------------===//

#include "dawn/CodeGen/GridTools/ASTStencilBody.h"
#include "dawn/CodeGen/CXXUtil.h"
#include "dawn/IIR/StencilFunctionInstantiation.h"
#include "dawn/IIR/StencilInstantiation.h"
#include "dawn/Optimizer/OptimizerContext.h"
#include "dawn/SIR/AST.h"
#include "dawn/Support/Unreachable.h"

namespace dawn {
namespace codegen {
namespace gt {

ASTStencilBody::ASTStencilBody(
    const iir::StencilMetaInformation& metadata,
    const std::unordered_set<iir::IntervalProperties>& intervalProperties)
    : ASTCodeGenCXX(), metadata_(metadata), intervalProperties_(intervalProperties),
      offsetPrinter_(",", "(", ")"), currentFunction_(nullptr), nestingOfStencilFunArgLists_(0) {}

ASTStencilBody::~ASTStencilBody() {}

std::string ASTStencilBody::getName(const std::shared_ptr<Stmt>& stmt) const {
  if(currentFunction_)
    return currentFunction_->getFieldNameFromAccessID(currentFunction_->getAccessIDFromStmt(stmt));
  else
    return metadata_.getFieldNameFromAccessID(metadata_.getAccessIDFromStmt(stmt));
}

std::string ASTStencilBody::getName(const std::shared_ptr<Expr>& expr) const {
  if(currentFunction_)
    return currentFunction_->getFieldNameFromAccessID(currentFunction_->getAccessIDFromExpr(expr));
  else
    return metadata_.getFieldNameFromAccessID(metadata_.getAccessIDFromExpr(expr));
}

int ASTStencilBody::getAccessID(const std::shared_ptr<Expr>& expr) const {
  if(currentFunction_)
    return currentFunction_->getAccessIDFromExpr(expr);
  else
    return metadata_.getAccessIDFromExpr(expr);
}

//===------------------------------------------------------------------------------------------===//
//     Stmt
//===------------------------------------------------------------------------------------------===//

void ASTStencilBody::visit(const std::shared_ptr<BlockStmt>& stmt) { Base::visit(stmt); }

void ASTStencilBody::visit(const std::shared_ptr<ExprStmt>& stmt) {
  if(isa<StencilFunCallExpr>(*(stmt->getExpr())))
    triggerCallProc_ = true;
  Base::visit(stmt);
}

void ASTStencilBody::visit(const std::shared_ptr<ReturnStmt>& stmt) {
  if(scopeDepth_ == 0)
    ss_ << std::string(indent_, ' ');

  // Inside stencil functions there are no return statements, instead we assign the return value to
  // the special field `__out`
  if(currentFunction_)
    ss_ << "eval(__out(0, 0, 0)) =";
  else
    ss_ << "return ";

  stmt->getExpr()->accept(*this);
  ss_ << ";\n";
}

void ASTStencilBody::visit(const std::shared_ptr<VarDeclStmt>& stmt) { Base::visit(stmt); }

void ASTStencilBody::visit(const std::shared_ptr<VerticalRegionDeclStmt>& stmt) {
  dawn_unreachable("VerticalRegionDeclStmt not allowed in this context");
}

void ASTStencilBody::visit(const std::shared_ptr<StencilCallDeclStmt>& stmt) {
  dawn_unreachable("StencilCallDeclStmt not allowed in this context");
}

void ASTStencilBody::visit(const std::shared_ptr<BoundaryConditionDeclStmt>& stmt) {
  DAWN_ASSERT_MSG(0, "BoundaryConditionDeclStmt not allowed in this context");
}

void ASTStencilBody::visit(const std::shared_ptr<IfStmt>& stmt) { Base::visit(stmt); }

//===------------------------------------------------------------------------------------------===//
//     Expr
//===------------------------------------------------------------------------------------------===//

void ASTStencilBody::visit(const std::shared_ptr<UnaryOperator>& expr) { Base::visit(expr); }

void ASTStencilBody::visit(const std::shared_ptr<BinaryOperator>& expr) { Base::visit(expr); }

void ASTStencilBody::visit(const std::shared_ptr<AssignmentExpr>& expr) { Base::visit(expr); }

void ASTStencilBody::visit(const std::shared_ptr<TernaryOperator>& expr) { Base::visit(expr); }

void ASTStencilBody::visit(const std::shared_ptr<FunCallExpr>& expr) { Base::visit(expr); }

void ASTStencilBody::visit(const std::shared_ptr<StencilFunCallExpr>& expr) {
  if(nestingOfStencilFunArgLists_++)
    ss_ << ", ";

  const std::shared_ptr<iir::StencilFunctionInstantiation> stencilFun =
      currentFunction_ ? currentFunction_->getStencilFunctionInstantiation(expr)
                       : metadata_.getStencilFunctionInstantiation(expr);

  ss_ << (triggerCallProc_ ? "gridtools::call_proc<" : "gridtools::call<")
      << iir::StencilFunctionInstantiation::makeCodeGenName(*stencilFun) << ", "
      << intervalProperties_.find(stencilFun->getInterval())->name_ << ">::with(eval";

  triggerCallProc_ = false;

  for(auto& arg : expr->getArguments()) {
    arg->accept(*this);
  }

  if(stencilFun->hasGlobalVariables()) {
    ss_ << ","
        << "globals()";
  }

  nestingOfStencilFunArgLists_--;
  ss_ << ")";
}

void ASTStencilBody::visit(const std::shared_ptr<StencilFunArgExpr>& expr) {}

void ASTStencilBody::visit(const std::shared_ptr<VarAccessExpr>& expr) {
  std::string name = getName(expr);
  int AccessID = getAccessID(expr);

  if(metadata_.isAccessType(iir::FieldAccessType::FAT_GlobalVariable, AccessID)) {
    if(!nestingOfStencilFunArgLists_)
      ss_ << "eval(";
    else
      ss_ << ", ";

    ss_ << "globals()";

    if(!nestingOfStencilFunArgLists_) {
      ss_ << ")." << name;
    }
  } else {
    ss_ << name;

    if(expr->isArrayAccess()) {
      ss_ << "[";
      expr->getIndex()->accept(*this);
      ss_ << "]";
    }
  }
}

void ASTStencilBody::visit(const std::shared_ptr<LiteralAccessExpr>& expr) { Base::visit(expr); }

void ASTStencilBody::visit(const std::shared_ptr<FieldAccessExpr>& expr) {
  if(!nestingOfStencilFunArgLists_)
    ss_ << "eval(";
  else
    ss_ << ", ";

  if(currentFunction_) {
    ss_ << currentFunction_->getOriginalNameFromCallerAccessID(
               currentFunction_->getAccessIDFromExpr(expr))
        << offsetPrinter_(currentFunction_->evalOffsetOfFieldAccessExpr(expr, false));
  } else
    ss_ << getName(expr) << offsetPrinter_(expr->getOffset());

  if(!nestingOfStencilFunArgLists_)
    ss_ << ")";
}

void ASTStencilBody::setCurrentStencilFunction(
    const std::shared_ptr<iir::StencilFunctionInstantiation>& currentFunction) {
  currentFunction_ = currentFunction;
}

} // namespace gt
} // namespace codegen
} // namespace dawn
