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

#include "dawn/CodeGen/CXXNaive/ASTStencilDesc.h"
#include "dawn/CodeGen/CXXUtil.h"
#include "dawn/SIR/AST.h"
#include "dawn/Support/Unreachable.h"

namespace dawn {
namespace codegen {
namespace cxxnaive {

ASTStencilDesc::ASTStencilDesc(const iir::StencilMetaInformation& metadata,
                               CodeGenProperties const& codeGenProperties)
    : ASTCodeGenCXX(), metadata_(metadata), codeGenProperties_(codeGenProperties) {}

ASTStencilDesc::~ASTStencilDesc() {}

std::string ASTStencilDesc::getName(const std::shared_ptr<Stmt>& stmt) const {
  return metadata_.getFieldNameFromAccessID(metadata_.getAccessIDFromStmt(stmt));
}

std::string ASTStencilDesc::getName(const std::shared_ptr<Expr>& expr) const {
  return metadata_.getFieldNameFromAccessID(metadata_.getAccessIDFromExpr(expr));
}

//===------------------------------------------------------------------------------------------===//
//     Stmt
//===------------------------------------------------------------------------------------------===//

void ASTStencilDesc::visit(const std::shared_ptr<ReturnStmt>& stmt) {
  DAWN_ASSERT_MSG(0, "ReturnStmt not allowed in StencilDesc AST");
}

void ASTStencilDesc::visit(const std::shared_ptr<VerticalRegionDeclStmt>& stmt) {
  DAWN_ASSERT_MSG(0, "VerticalRegionDeclStmt not allowed in StencilDesc AST");
}

void ASTStencilDesc::visit(const std::shared_ptr<StencilCallDeclStmt>& stmt) {
  int stencilID = metadata_.getStencilIDFromStencilCallStmt(stmt);

  std::string stencilName =
      codeGenProperties_.getStencilName(StencilContext::SC_Stencil, stencilID);
  ss_ << "m_" << stencilName + "->run();\n";
}

void ASTStencilDesc::visit(const std::shared_ptr<BoundaryConditionDeclStmt>& stmt) {
  //  DAWN_ASSERT_MSG(0, "BoundaryConditionDeclStmt not yet implemented");
}

//===------------------------------------------------------------------------------------------===//
//     Expr
//===------------------------------------------------------------------------------------------===//

void ASTStencilDesc::visit(const std::shared_ptr<StencilFunCallExpr>& expr) {
  DAWN_ASSERT_MSG(0, "StencilFunCallExpr not allowed in StencilDesc AST");
}

void ASTStencilDesc::visit(const std::shared_ptr<StencilFunArgExpr>& expr) {
  DAWN_ASSERT_MSG(0, "StencilFunArgExpr not allowed in StencilDesc AST");
}

void ASTStencilDesc::visit(const std::shared_ptr<VarAccessExpr>& expr) {
  if(metadata_.isAccessType(iir::FieldAccessType::FAT_GlobalVariable,
                            metadata_.getAccessIDFromExpr(expr)))
    ss_ << "m_globals.";

  ss_ << getName(expr);

  if(expr->isArrayAccess()) {
    ss_ << "[";
    expr->getIndex()->accept(*this);
    ss_ << "]";
  }
}

void ASTStencilDesc::visit(const std::shared_ptr<FieldAccessExpr>& expr) {
  DAWN_ASSERT_MSG(0, "FieldAccessExpr not allowed in StencilDesc AST");
}

} // namespace cxxnaive
} // namespace codegen
} // namespace dawn
