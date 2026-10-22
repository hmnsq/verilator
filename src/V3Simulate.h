// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Simulate code to determine output values/variables
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
// void example_usage() {
//      SimulateVisitor simvis{false, false};
//      simvis.clear();
//      // Set all inputs to the constant
//      for (deque<AstVarScope*>::iterator it = m_inVarps.begin(); it!=m_inVarps.end(); ++it) {
//          simvis.newValue(invscp, #);
//      }
//      // Simulate
//      simvis.main(nodep);
//      // Read outputs
//      for (deque<AstVarScope*>::iterator it = m_outVarps.begin(); it!=m_outVarps.end(); ++it) {
//          AstConst* outconstp = simvis.fetchOutNumberNull(outvscp);
//
//*************************************************************************

#ifndef VERILATOR_V3SIMULATE_H_
#define VERILATOR_V3SIMULATE_H_

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3AstUserAllocator.h"
#include "V3Const.h"
#include "V3Error.h"
#include "V3Task.h"
#include "V3Width.h"

#include <deque>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

//============================================================================

//######################################################################
// Simulate class functions

class SimStackNode final {
public:
    // MEMBERS
    AstFuncRef* const m_funcp;
    V3TaskConnects* const m_tconnects;
    // CONSTRUCTORS
    SimStackNode(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~SimStackNode() = default;
};

class SimulateVisitor VL_NOT_FINAL : public VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

private:
    // CONSTANTS
    static constexpr целое CALL_STACK_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    const VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    enum VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    struct AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        AstNodeExpr* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        AstNodeExpr* valuep = nullptr;
    };

    AstUser1Allocator<AstNode, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    struct ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, AstNode* nodep) {
            если (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            если (m_nextFree == m_constps.size()) {
                m_constps.push_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* const constp = m_constps[m_nextFree++];
            constp->num().nodep(nodep);
            вернуть constp;
        }

        ~ConstAllocator() {
            для (AstConst* const constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    AstNode* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    целое m_instrCount;  ///< Number of nodes
    целое m_dataCount;  ///< Bytes of data
    целое m_recurseCount = 0;  // Now deep in current recursion
    AstNode* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<const AstNodeDType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<SimStackNode*> m_callStack;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<AstNode*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string prettyNumber(const V3Number* nump, const AstNodeDType* dtypep) {
        если (const AstRefDType* const refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        если (const AstNodeUOrStructDType* const stp = VN_CAST(dtypep, NodeUOrStructDType)) {
            если (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                для (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    const целое width = itemp->width();
                    const целое lsb = itemp->lsb();
                    const целое msb = lsb + width - 1;
                    V3Number fieldNum{nump, width};
                    fieldNum.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    если (const AstNodeDType* const childTypep = itemp->subDTypep()) {
                        out << prettyNumber(&fieldNum, childTypep);
                    } иначе {
                        out << fieldNum;
                    }
                    если (itemp->nextp()) out << ", ";
                }
                out << "}";
                вернуть out.str();
            }
        } иначе если (const AstPackArrayDType* const arrayp = VN_CAST(dtypep, PackArrayDType)) {
            если (const AstNodeDType* const childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                const целое arrayElements = arrayp->elementsConst();
                для (целое element = 0; element < arrayElements; ++element) {
                    const целое width = childTypep->width();
                    const целое lsb = width * element;
                    const целое msb = lsb + width - 1;
                    V3Number fieldNum{nump, width};
                    fieldNum.opSel(*nump, msb, lsb);
                    const целое arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << prettyNumber(&fieldNum, childTypep);
                    если (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                вернуть out.str();
            }
        }
        вернуть nump->ascii();
    }

    // Checking METHODS
public:
    /// Call other-this function on all new *non-constant* var references
    virtual void varRefCb(AstVarRef* nodep) {}

    void clearOptimizable(AstNode* nodep /*null ok*/, const string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        если (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            если (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                если (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream stack;
            целое n = 0;
            для (const auto& callstack : vlstd::reverse_view(m_callStack)) {
                const AstFuncRef* const funcp = callstack->m_funcp;
                stack << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                для (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    const AstVar* const portp = conIt->first;
                    AstNodeExpr* const pinp = conIt->second->exprp();
                    const AstNodeDType* const dtypep = pinp->dtypep();
                    если (AstConst* const valp = fetchConstNull(pinp)) {
                        stack << "\n           " << portp->prettyName() << " = "
                              << prettyNumber(&valp->num(), dtypep);
                    } иначе если (AstNodeExpr* const valuep = fetchValueNull(pinp)) {
                        stack << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                если (++n > CALL_STACK_MAX) {
                    stack << "\n           ... stack truncated";
                    break;
                }
            }
            m_whyNotOptimizable += stack.str();
        }
    }
    bool optimizable() const { вернуть m_whyNotNodep == nullptr; }
    string whyNotMessage() const { вернуть m_whyNotOptimizable; }
    AstNode* whyNotNodep() const { вернуть m_whyNotNodep; }

    bool isAssignDly() const { вернуть m_anyAssignDly; }
    bool isImpure() const { вернуть m_isImpure; }
    bool isCoverage() const { вернуть m_isCoverage; }
    целое instrCount() const { вернуть m_instrCount; }
    целое dataCount() const { вернуть m_dataCount; }

    // Simulation METHODS
private:
    AstConst* allocConst(AstNode* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        вернуть m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

public:
    void newValue(AstNode* nodep, const AstNodeExpr* valuep) {
        если (const AstConst* const constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->num().opAssign(constp->num());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } иначе если (fetchValueNull(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            setValue(nodep, newTrackedClone(const_cast<AstNodeExpr*>(valuep)));
        }
    }
    void newOutValue(AstNode* nodep, const AstNodeExpr* valuep) {
        если (const AstConst* const constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->num().opAssign(constp->num());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } иначе если (fetchOutValueNull(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            setOutValue(nodep, newTrackedClone(const_cast<AstNodeExpr*>(valuep)));
        }
    }

private:
    AstNodeExpr* newTrackedClone(AstNodeExpr* nodep) {
        AstNodeExpr* const newp = nodep->cloneTree(false);
        m_reclaimValuesp.push_back(newp);
        вернуть newp;
    }
    AstConst* newConst(AstNode* nodep) {
        // Set a constant value for this node
        если (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* const constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            вернуть constp;
        }
        вернуть fetchConst(nodep);
    }
    AstConst* newOutConst(AstNode* nodep) {
        // Set a var-output constant value for this node
        если (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* const constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            вернуть constp;
        }
        вернуть fetchOutConst(nodep);
    }

public:
    AstNodeExpr* fetchValueNull(AstNode* nodep) { вернуть m_varAux(nodep).valuep; }

private:
    AstNodeExpr* fetchOutValueNull(AstNode* nodep) { вернуть m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(AstNode* nodep) { вернуть VN_CAST(fetchValueNull(nodep), Const); }
    AstConst* fetchOutConstNull(AstNode* nodep) {
        вернуть VN_CAST(fetchOutValueNull(nodep), Const);
    }
    AstNodeExpr* fetchValue(AstNode* nodep) {
        AstNodeExpr* const valuep = fetchValueNull(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        вернуть valuep;
    }
    AstConst* fetchConst(AstNode* nodep) {
        AstConst* const constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        вернуть constp;
    }
    AstConst* fetchOutConst(AstNode* nodep) {
        AstConst* const constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        вернуть constp;
    }

public:
    V3Number* fetchNumberNull(AstNode* nodep) {
        AstConst* const constp = fetchConstNull(nodep);
        если (constp) вернуть &constp->num();
        вернуть nullptr;
    }
    V3Number* fetchOutNumberNull(AstNode* nodep) {
        AstConst* const constp = fetchOutConstNull(nodep);
        если (constp) вернуть &constp->num();
        вернуть nullptr;
    }

private:
    void setValue(AstNode* nodep, AstNodeExpr* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    void setOutValue(AstNode* nodep, AstNodeExpr* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    void checkNodeInfo(AstNode* nodep, bool ignorePredict = false) {
        если (m_checkOnly) {
            m_instrCount += nodep->instrCount();
            m_dataCount += nodep->width();
        }
        если (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        если (!nodep->isPure()) m_isImpure = true;
    }

    void knownBadNodeType(AstNode* nodep) {
        // Call for node types we know we can't handle
        если (jumpingOver()) вернуть;
        checkNodeInfo(nodep);
        если (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    void badNodeType(AstNode* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        checkNodeInfo(nodep);
        если (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            static std::set<VNType> s_typePrinted;
            const auto pair = s_typePrinted.emplace(nodep->type());
            если (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    AstNode* varOrScope(AstVarRef* nodep) const {
        AstNode* vscp;
        если (m_scoped) {
            vscp = nodep->varScopep();
        } иначе {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        вернуть vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() const { вернуть m_jumptargetp; }
    void assignOutValue(const AstNodeAssign* nodep, AstNode* vscp, const AstNodeExpr* valuep) {
        если (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            newOutValue(vscp, valuep);
        } иначе {
            newValue(vscp, valuep);
            setOutValue(vscp, fetchValue(vscp));
        }
    }

    string toStringRecurse(AstNodeExpr* valuep) {
        // Return string representation of a value, or clearOptimizable
        если (const AstConst* const avaluep = VN_CAST(valuep, Const)) {
            вернуть "'h"s + avaluep->num().displayed(valuep, "%0x");
        }
        если (const AstConsArray* const avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            const AstUnpackArrayDType* const dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            для (целое n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            вернуть result;
        }
        если (const AstConsStruct* const avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            для (const AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            вернуть result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        вернуть "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    AstNodeExpr* newInitValuep(AstNode* nodep, AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();
        если (AstUnpackArrayDType* const arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            AstNodeExpr* const elementp = newInitValuep(nodep, arrayp->subDTypep());
            вернуть elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* const structp = VN_CAST(dtypep, StructDType);
        если (structp && !structp->packed()) {
            AstConsStruct* const newp = new AstConsStruct{nodep->fileline(), structp};
            для (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                AstNodeExpr* const memberValuep = newInitValuep(nodep, memberp->subDTypep());
                если (!memberValuep) {
                    VL_DO_DANGLING(newp->deleteTree(), newp);
                    вернуть nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            вернуть newp;
        }
        если (const AstBasicDType* const basicp = dtypep->basicp()) {
            AstConst* const constp
                = new AstConst{nodep->fileline(), AstConst::WidthedValue{}, basicp->widthMin(), 0};
            если (basicp->isZeroInit()) {
                constp->num() = V3Number{nodep, basicp};
            } иначе {
                constp->num().setAllBitsX();
            }
            вернуть constp;
        }
        вернуть nullptr;
    }
    void initVar(AstVar* nodep) {
        если (AstNodeExpr* const valuep = newInitValuep(nodep, nodep->dtypep())) {
            newValue(nodep, valuep);
            VL_DO_DANGLING(valuep->deleteTree(), valuep);
        }
    }

    // VISITORS
    void visit(AstAlways* nodep) override {
        если (jumpingOver()) вернуть;
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstSenTree* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    void visit(AstVarRef* nodep) override {
        если (jumpingOver()) вернуть;
        если (!optimizable()) вернуть;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        AstNode* const vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        если (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), NodeUOrStructDType))
            clearOptimizable(nodep, "Array references/not basic");
        если (nodep->access().isWriteOrRW()) {
            если (m_inDlyAssign) {
                если (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    если (m_checkOnly) varRefCb(nodep);
                }
            } иначе {  // nondly asn
                если (!(m_varAux(vscp).usage & VU_LV)) {
                    если (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    если (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        если (nodep->access().isReadOrRW()) {
            если (!(m_varAux(vscp).usage & VU_RV)) {
                если (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                const bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                const AstNodeExpr* const valuep
                    = varIsConst ? fetchValueNull(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                если (varIsConst && valuep) {
                    если (!m_checkOnly && optimizable()) newValue(vscp, valuep);
                } иначе {
                    если (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        если (m_checkOnly || !optimizable()) вернуть;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            AstNodeExpr* valuep = fetchValueNull(vscp);
            если (!valuep) {
                если (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } иначе {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            setValue(nodep, valuep);
        }
    }
    void visit(AstVarXRef* nodep) override {
        если (jumpingOver()) вернуть;
        если (m_scoped) {
            badNodeType(nodep);
            вернуть;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    void visit(AstNodeFTask* nodep) override {
        если (jumpingOver()) вернуть;
        если (!m_params) {
            badNodeType(nodep);
            вернуть;
        }
        если (nodep->dpiImport()) {
            если (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        если (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstInitialAutomaticStmt* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstInitialStaticStmt* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstInitialStatic* nodep) override {
        если (jumpingOver()) вернуть;
        если (!m_params) {
            badNodeType(nodep);
            вернуть;
        }
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstNodeIf* nodep) override {
        если (jumpingOver()) вернуть;
        UINFO(5, "   IF " << nodep);
        checkNodeInfo(nodep);
        если (m_checkOnly) {
            iterateChildrenConst(nodep);
        } иначе {
            iterateAndNextConstNull(nodep->condp());
            если (optimizable()) {
                если (fetchConst(nodep->condp())->num().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } иначе {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    void visit(AstConst* nodep) override {
        checkNodeInfo(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        newValue(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    void visit(AstConsArray* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        AstConsArray* const valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetchValue(nodep->defaultp())->cloneTree(false) : nullptr};
        для (const AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetchValue(itemp->valuep())->cloneTree(false));
        }
        m_reclaimValuesp.push_back(valuep);
        setValue(nodep, valuep);
    }
    void visit(AstConsStruct* nodep) override {
        если (!m_params) {
            badNodeType(nodep);
            вернуть;
        }
        checkNodeInfo(nodep);
        для (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        если (m_checkOnly || !optimizable()) вернуть;
        AstConsStruct* const valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        для (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetchValue(itemp->valuep())->cloneTree(false)});
        }
        m_reclaimValuesp.push_back(valuep);
        setValue(nodep, valuep);
    }
    void visit(AstCvtUnpackToUnpack* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        // An array value is an AstConsArray, so V3Const folds the conversion
        AstNodeExpr* const cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetchValue(nodep->fromp())->cloneTree(false), nodep->dtypep()};
        AstNodeExpr* const valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.push_back(valuep);
        setValue(nodep, valuep);
    }
    void visit(AstConsArrayItem* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstEnumItemRef* nodep) override {
        checkNodeInfo(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        если (m_checkOnly || !optimizable()) вернуть;
        AstNode* const valuep = nodep->itemp()->valuep();
        если (valuep) {
            iterateAndNextConstNull(valuep);
            если (!optimizable()) вернуть;
            newValue(nodep, fetchValue(valuep));
        } иначе {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    void visit(AstNodeUniop* nodep) override {
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        nodep->numberOperate(newConst(nodep)->num(), fetchConst(nodep->lhsp())->num());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    void aggregateEqRecurse(V3Number& resultr, AstNode* nodep, bool caseEq, AstNodeExpr* lhsp,
                            AstNodeExpr* rhsp) {
        если (const AstConst* const lconstp = VN_CAST(lhsp, Const)) {
            const V3Number& lnum = lconstp->num();
            const V3Number& rnum = VN_AS(rhsp, Const)->num();
            V3Number eq{nodep, 1};
            если (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } иначе если (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } иначе если (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } иначе {
                eq.opEq(lnum, rnum);
            }
            const V3Number prev = resultr;
            resultr.opLogAnd(prev, eq);
        } иначе если (const AstConsArray* const lconsp = VN_CAST(lhsp, ConsArray)) {
            const AstUnpackArrayDType* const dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            для (целое n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } иначе {
            const AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            для (const AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    void handleAggregateCompare(AstNodeBiop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        const bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        const bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3Number& resultr = newConst(nodep)->num();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetchValue(nodep->lhsp()),
                           fetchValue(nodep->rhsp()));
        если (neq) {
            const V3Number eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    void visit(AstNodeBiop* nodep) override {
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        если (!VN_IS(fetchValue(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            вернуть;
        }
        AstConst* const valuep = newConst(nodep);
        nodep->numberOperate(newConst(nodep)->num(), fetchConst(nodep->lhsp())->num(),
                             fetchConst(nodep->rhsp())->num());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        если (!m_params && VN_IS(nodep, Sel) && valuep->num().isAnyX()) {
            const V3Number num{valuep, valuep->width(), valuep->num()};
            valuep->num().opBitsOne(num);
        }
    }
    void visit(AstNodeTriop* nodep) override {
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        nodep->numberOperate(newConst(nodep)->num(), fetchConst(nodep->lhsp())->num(),
                             fetchConst(nodep->rhsp())->num(), fetchConst(nodep->thsp())->num());
    }
    void visit(AstNodeQuadop* nodep) override {
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        nodep->numberOperate(newConst(nodep)->num(), fetchConst(nodep->lhsp())->num(),
                             fetchConst(nodep->rhsp())->num(), fetchConst(nodep->thsp())->num(),
                             fetchConst(nodep->fhsp())->num());
    }
    void visit(AstLogAnd* nodep) override {
        // Need to short circuit
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        если (m_checkOnly) {
            iterateChildrenConst(nodep);
        } иначе {
            iterateConst(nodep->lhsp());
            если (!optimizable()) вернуть;
            если (fetchConst(nodep->lhsp())->num().isNeqZero()) {
                iterateConst(nodep->rhsp());
                если (!optimizable()) вернуть;
                newValue(nodep, fetchValue(nodep->rhsp()));
            } иначе {
                newValue(nodep, fetchValue(nodep->lhsp()));  // a zero
            }
        }
    }
    void visit(AstLogOr* nodep) override {
        // Need to short circuit
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        если (m_checkOnly) {
            iterateChildrenConst(nodep);
        } иначе {
            iterateConst(nodep->lhsp());
            если (!optimizable()) вернуть;
            если (fetchConst(nodep->lhsp())->num().isNeqZero()) {
                newValue(nodep, fetchValue(nodep->lhsp()));  // a one
            } иначе {
                iterateConst(nodep->rhsp());
                если (!optimizable()) вернуть;
                newValue(nodep, fetchValue(nodep->rhsp()));
            }
        }
    }
    void visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        если (m_checkOnly) {
            iterateChildrenConst(nodep);
        } иначе {
            iterateConst(nodep->lhsp());
            если (!optimizable()) вернуть;
            если (fetchConst(nodep->lhsp())->num().isEqZero()) {
                const AstConst cnst{nodep->fileline(), AstConst::WidthedValue{}, 1, 1};  // a one
                newValue(nodep, &cnst);  // a one
            } иначе {
                iterateConst(nodep->rhsp());
                если (!optimizable()) вернуть;
                newValue(nodep, fetchValue(nodep->rhsp()));
            }
        }
    }
    void visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        если (m_checkOnly) {
            iterateChildrenConst(nodep);
        } иначе {
            iterateConst(nodep->condp());
            если (!optimizable()) вернуть;
            если (fetchConst(nodep->condp())->num().isNeqZero()) {
                iterateConst(nodep->thenp());
                если (!optimizable()) вернуть;
                newValue(nodep, fetchValue(nodep->thenp()));
            } иначе {
                iterateConst(nodep->elsep());
                если (!optimizable()) вернуть;
                newValue(nodep, fetchValue(nodep->elsep()));
            }
        }
    }

    void handleAssignElement(AstNodeAssign* nodep, AstNodeExpr* lhsp, AstNodeExpr* valueFromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<AstNodeExpr*> selps;
        AstNodeExpr* fromp = lhsp;
        пока (true) {
            если (AstArraySel* const selp = VN_CAST(fromp, ArraySel)) {
                checkNodeInfo(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.push_back(selp);
                fromp = selp->fromp();
            } иначе если (AstStructSel* const selp = VN_CAST(fromp, StructSel)) {
                checkNodeInfo(selp);
                selps.push_back(selp);
                fromp = selp->fromp();
            } иначе {
                break;
            }
        }
        AstVarRef* const varrefp = VN_CAST(fromp, VarRef);
        если (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            вернуть;
        }
        если (m_checkOnly || !optimizable()) вернуть;
        AstNode* const vscp = varOrScope(varrefp);
        AstNodeExpr* rootp = fetchOutValueNull(vscp);
        если (!rootp) rootp = fetchValueNull(vscp);
        если (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            вернуть;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        AstNodeExpr* itemp = rootp;
        для (AstNodeExpr* const selp : vlstd::reverse_view(selps)) {
            если (const AstArraySel* const aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* const consp = VN_AS(itemp, ConsArray);
                const uint32_t index = fetchConst(aselp->bitp())->toUInt();
                const целое elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                если (index >= static_cast<uint32_t>(elements)) {
                    вернуть;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                если (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->cloneTree(false);
                    consp->valuep(index, itemp);
                }
            } иначе {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        AstNodeExpr* const valuep = newTrackedClone(fetchValue(valueFromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.push_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assignOutValue(nodep, vscp, rootp);
    }
    void handleAssignSel(AstNodeAssign* nodep, AstSel* selp, AstNodeExpr* valueFromp) {
        AstVarRef* varrefp = nullptr;
        V3Number lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        если (m_checkOnly || !optimizable()) вернуть;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        AstNode* const vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        если (AstConst* const vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } иначе если (AstConst* const vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } иначе {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::WidthedValue{},
                                     varrefp->varp()->widthMin(), 0};
            если (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->num().setAllBits0();
            } иначе {
                outconstp->num().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->num().opSelInto(fetchConst(valueFromp)->num(), lsb, selp->widthConst());
        assignOutValue(nodep, vscp, outconstp);
    }
    void handleAssignSelRecurse(AstNodeAssign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3Number& lsbRef, целое depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        checkNodeInfo(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        если (AstVarRef* const varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->num();
            вернуть;  // And presumably still optimizable()
        }
        если (AstSel* const subselp = VN_CAST(selp->fromp(), Sel)) {
            V3Number sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            если (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->num());
            }
        } иначе {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    void handleAssignRecurse(AstNodeAssign* nodep, AstNodeExpr* lhsp, AstNodeExpr* valueFromp) {
        если (!optimizable()) вернуть;
        если (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            если (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                вернуть;
            }
            handleAssignElement(nodep, lhsp, valueFromp);
        } иначе если (AstConcat* const selp = VN_CAST(lhsp, Concat)) {
            checkNodeInfo(selp);
            если (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                вернуть;
            }
            // Split value into left and right concat values
            если (!m_checkOnly) {
                {
                    AstConst* const outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::WidthedValue{},
                                       selp->lhsp()->width(), 0};
                    outconstp->num().opSel(fetchConst(valueFromp)->num(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    newValue(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->deleteTree(), outconstp);
                }
                {
                    AstConst* const outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::WidthedValue{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->num().opSel(fetchConst(valueFromp)->num(),
                                           selp->rhsp()->width() - 1, 0);
                    newValue(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->deleteTree(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } иначе если (AstReplicate* const selp = VN_CAST(lhsp, Replicate)) {
            checkNodeInfo(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* const countp = VN_CAST(selp->countp(), Const);
            если (!countp || !countp->num().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                вернуть;
            }
            handleAssignRecurse(nodep, selp->srcp(), valueFromp);
        } иначе если (AstSel* const selp = VN_CAST(lhsp, Sel)) {
            если (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                вернуть;
            }
            handleAssignSel(nodep, selp, valueFromp);
        } иначе если (VN_IS(lhsp, VarRef)) {
            если (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } иначе {
                AstNode* const vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assignOutValue(nodep, vscp, fetchValue(valueFromp));
            }
        } иначе {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    void visit(AstNodeAssign* nodep) override {
        если (jumpingOver()) вернуть;
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);

        VL_RESTORER(m_inDlyAssign);

        если (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } иначе если (VN_IS(nodep, AssignDly)) {
            если (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } иначе {
            если (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        если (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            вернуть;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    void visit(AstArraySel* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (const AstConsArray* const consp = VN_CAST(fetchValueNull(nodep->fromp()), ConsArray)) {
            const AstConst* const indexp = fetchConst(nodep->bitp());
            const uint32_t offset = indexp->num().toUInt();
            AstNodeExpr* const itemp = consp->valuep(offset);
            если (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } иначе {
                setValue(nodep, itemp);
            }
        } иначе {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    void visit(AstStructSel* nodep) override {
        если (!m_params) {
            badNodeType(nodep);
            вернуть;
        }
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        AstConsStruct* const consp = VN_AS(fetchValue(nodep->fromp()), ConsStruct);
        setValue(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    void visit(AstSliceSel* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        // Fetch the base constant array
        если (const AstConsArray* const consp = VN_CAST(fetchValueNull(nodep->fromp()), ConsArray)) {
            const VNumRange& sliceRange = nodep->declRange();
            const uint32_t sliceElements = sliceRange.elements();
            const целое sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            AstNodeDType* const dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* const newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            для (uint32_t idx = 0; idx < sliceElements; ++idx) {
                const uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->cloneTree(false));
            }
            // Assign the new constant array and track it for later deletion
            setValue(nodep, newConsp);
            m_reclaimValuesp.push_back(newConsp);
        } иначе {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    void visit(AstBegin* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstCase* nodep) override {
        если (jumpingOver()) вернуть;
        UINFO(5, "   CASE " << nodep);
        checkNodeInfo(nodep);
        если (m_checkOnly) {
            iterateChildrenConst(nodep);
        } иначе если (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            для (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                если (!itemp->isDefault()) {
                    для (AstNode* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        если (hit) break;
                        iterateAndNextConstNull(ep);
                        если (optimizable()) {
                            V3Number match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->num(), fetchConst(ep)->num());
                            если (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            для (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                если (hit) break;
                если (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    void visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        если (jumpingOver()) вернуть;
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }

    void visit(AstComment*) override {}

    void visit(AstStmtExpr* nodep) override {
        если (jumpingOver()) вернуть;
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstExprStmt* nodep) override {
        если (jumpingOver()) вернуть;
        checkNodeInfo(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        если (!optimizable()) вернуть;
        iterateAndNextConstNull(nodep->resultp());
        если (m_checkOnly || !optimizable()) вернуть;
        newValue(nodep, fetchValue(nodep->resultp()));
    }

    void visit(AstJumpBlock* nodep) override {
        если (jumpingOver()) вернуть;
        iterateChildrenConst(nodep);
        если (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    void visit(AstJumpGo* nodep) override {
        если (jumpingOver()) вернуть;
        checkNodeInfo(nodep);
        если (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    void visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        если (jumpingOver()) вернуть;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        если (!m_params) {
            badNodeType(nodep);
            вернуть;
        }
        checkNodeInfo(nodep);
        если (m_checkOnly) {
            iterateChildrenConst(nodep);
            вернуть;
        }
        если (!optimizable()) вернуть;

        size_t iterCount = 0;
        const size_t iterLimit = v3Global.opt.unrollLimit();
        пока (true) {
            если (iterCount > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                break;
            }
            ++iterCount;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            если (jumpingOver()) break;
        }

        если (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    void visit(AstLoopTest* nodep) override {
        если (jumpingOver()) вернуть;
        checkNodeInfo(nodep);
        iterateConst(nodep->condp());
        если (m_checkOnly || !optimizable()) вернуть;
        если (fetchConst(nodep->condp())->num().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    void visit(AstStop* nodep) override {
        если (jumpingOver()) вернуть;
        если (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        checkNodeInfo(nodep);
    }
    void visit(AstFuncRef* nodep) override {
        если (jumpingOver()) вернуть;
        если (!optimizable()) вернуть;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        checkNodeInfo(nodep);
        если (!m_params) {
            badNodeType(nodep);
            вернуть;
        }
        AstNodeFTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        если (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        если (funcp->recursive()) {
            если (m_recurseCount >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                вернуть;
            }
            ++m_recurseCount;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<AstNode*, AstNodeExpr*> oldValues;

        если (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &oldValues](AstVar* varp) {
                если (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    если (AstNodeExpr* const valuep = fetchValueNull(varp)) {
                        AstNodeExpr* const nvaluep = newTrackedClone(valuep);
                        oldValues.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &oldValues](AstNodeExpr* exprp) {
                если (VN_IS(exprp, Const)) вернуть;  // Speed up as won't change
                если (AstNodeExpr* const valuep = fetchValueNull(exprp)) {
                    AstNodeExpr* const nvaluep = newTrackedClone(valuep);
                    oldValues.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, AstNodeExpr*>> portValues;
        для (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* const portp = it->first;
            AstNode* const pinp = it->second->exprp();
            если (pinp) {  // Else too few arguments in function call - ignore it
                если (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    вернуть;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                portValues.emplace_back(std::make_pair(portp, newTrackedClone(fetchValue(pinp))));
            }
        }
        // Apply value to the function
        если (!m_checkOnly && optimizable())
            для (const auto& it : portValues) {
                если (!m_checkOnly && optimizable()) newValue(it.first, it.second);
            }
        SimStackNode stackNode{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_callStack.push_back(&stackNode);
        если (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                если (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_callStack.pop_back();
        const AstNodeExpr* returnp = nullptr;
        если (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetchValue(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        для (const auto& it : oldValues) {
            если (it.second) newValue(it.first, it.second);
        }
        если (returnp) newValue(nodep, returnp);
        если (funcp->recursive()) {
            UASSERT_OBJ(m_recurseCount > 0, nodep, "recurse underflow");
            --m_recurseCount;
        }
    }

    void visit(AstVar* nodep) override {
        если (jumpingOver()) вернуть;
        если (!m_params) {
            badNodeType(nodep);
            вернуть;
        }
    }

    void visit(AstScopeName* /*nodep*/) override {
        если (jumpingOver()) вернуть;
        // Ignore
    }

    void visit(AstSFormatArg* nodep) override {
        checkNodeInfo(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    void visit(AstSFormatF* nodep) override {
        если (jumpingOver()) вернуть;
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;

        AstNode* nextArgp = nodep->exprsp();
        string result;
        const string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        для (; pos != format.cend(); ++pos) {
            если (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } иначе если (!inPct) {  // Normal text
                result += *pos;
            } иначе {  // Format character
                если (std::isdigit(pos[0])) {
                    width += pos[0];
                    continue;
                }

                inPct = false;

                если (V3Number::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    AstNode* const argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);
                    AstNode* const subargp = fargp ? fargp->exprp() : argp;
                    AstConst* const constp = fetchConstNull(subargp);
                    const VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    если (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        break;
                    }
                    const string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->num().displayedEnum(fargp, pformat)
                                  : constp->num().displayed(nodep, pformat, formatAttr);
                } иначе {
                    switch (std::tolower(pos[0])) {
                    case '%': result += "%"; break;
                    case 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        break;
                    default: clearOptimizable(nodep, "Unknown $display-like format code."); break;
                    }
                }
            }
        }

        AstConst* const resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        setValue(nodep, resultConstp);
        m_reclaimValuesp.push_back(resultConstp);
    }

    void visit(AstDisplay* nodep) override {
        если (jumpingOver()) вернуть;
        если (!optimizable()) вернуть;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        checkNodeInfo(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        если (m_params) {
            const AstConst* const textp = fetchConst(nodep->fmtp());
            switch (nodep->displayType()) {
            case VDisplayType::DT_DISPLAY:  // FALLTHRU
            case VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); break;
            case VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); break;
            case VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); break;
            case VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); break;
            case VDisplayType::DT_WRITE:  // FALLTHRU
            default: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    void visit(AstToStringN* nodep) override {
        если (jumpingOver()) вернуть;
        если (!optimizable()) вернуть;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        если (m_checkOnly || !optimizable()) вернуть;
        const std::string result = toStringRecurse(fetchValue(nodep->lhsp()));
        если (!optimizable()) вернуть;
        AstConst* const resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        setValue(nodep, resultConstp);
        m_reclaimValuesp.push_back(resultConstp);
    }

    void visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    void visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        knownBadNodeType(nodep);
    }
    void visit(AstGetInitialRandomSeed* nodep) override { knownBadNodeType(nodep); }
    void visit(AstMemberSel* nodep) override { knownBadNodeType(nodep); }
    void visit(AstSampled* nodep) override { knownBadNodeType(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    void visit(AstNode* nodep) override {
        если (jumpingOver()) вернуть;
        badNodeType(nodep);
    }

    // MEMBERS - called by constructor
    void setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    void mainGuts(AstNode* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

public:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    void clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instrCount = 0;
        m_dataCount = 0;
        m_jumptargetp = nullptr;

        AstNode::user1ClearTree();
        m_varAux.clear();
        ++m_constGeneration;
    }
    void mainTableCheck(AstNode* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    void mainTableEmulate(AstNode* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    void mainParamEmulate(AstNode* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<AstNode*> unusedRootps;
        для (AstNode* const nodep : m_reclaimValuesp) {
            если (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        для (AstNode* const nodep : unusedRootps) VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
};

#endif  // Guard
