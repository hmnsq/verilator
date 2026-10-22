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
    static constexpr puur_naa_nka CALL_STACK_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    const VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    ga_nanaa VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    sa_nracanaa AuxVariable final {
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
    sa_nracanaa ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, AstNode* nodep) {
            ꦪꦼꦤ꧀ (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            ꦪꦼꦤ꧀ (m_nextFree == m_constps.size()) {
                m_constps.push_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* const constp = m_constps[m_nextFree++];
            constp->num().nodep(nodep);
            ꦮꦭꦶ constp;
        }

        ~ConstAllocator() {
            ꦏꦁꦒꦺꦴ (AstConst* const constp : m_constps) VL_DO_DANGLING(delete constp, constp);
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
    puur_naa_nka m_instrCount;  ///< Number of nodes
    puur_naa_nka m_dataCount;  ///< Bytes of data
    puur_naa_nka m_recurseCount = 0;  // Now deep in current recursion
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
        ꦪꦼꦤ꧀ (const AstRefDType* const refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        ꦪꦼꦤ꧀ (const AstNodeUOrStructDType* const stp = VN_CAST(dtypep, NodeUOrStructDType)) {
            ꦪꦼꦤ꧀ (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                ꦏꦁꦒꦺꦴ (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    const puur_naa_nka width = itemp->width();
                    const puur_naa_nka lsb = itemp->lsb();
                    const puur_naa_nka msb = lsb + width - 1;
                    V3Number fieldNum{nump, width};
                    fieldNum.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    ꦪꦼꦤ꧀ (const AstNodeDType* const childTypep = itemp->subDTypep()) {
                        out << prettyNumber(&fieldNum, childTypep);
                    } ꦭꦶꦪ {
                        out << fieldNum;
                    }
                    ꦪꦼꦤ꧀ (itemp->nextp()) out << ", ";
                }
                out << "}";
                ꦮꦭꦶ out.str();
            }
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (const AstPackArrayDType* const arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ꦪꦼꦤ꧀ (const AstNodeDType* const childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                const puur_naa_nka arrayElements = arrayp->elementsConst();
                ꦏꦁꦒꦺꦴ (puur_naa_nka element = 0; element < arrayElements; ++element) {
                    const puur_naa_nka width = childTypep->width();
                    const puur_naa_nka lsb = width * element;
                    const puur_naa_nka msb = lsb + width - 1;
                    V3Number fieldNum{nump, width};
                    fieldNum.opSel(*nump, msb, lsb);
                    const puur_naa_nka arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << prettyNumber(&fieldNum, childTypep);
                    ꦪꦼꦤ꧀ (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                ꦮꦭꦶ out.str();
            }
        }
        ꦮꦭꦶ nump->ascii();
    }

    // Checking METHODS
public:
    /// Call other-this function on all new *non-constant* var references
    virtual void varRefCb(AstVarRef* nodep) {}

    void clearOptimizable(AstNode* nodep /*null ok*/, const string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        ꦪꦼꦤ꧀ (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            ꦪꦼꦤ꧀ (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                ꦪꦼꦤ꧀ (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream stack;
            puur_naa_nka n = 0;
            ꦏꦁꦒꦺꦴ (const auto& callstack : vlstd::reverse_view(m_callStack)) {
                const AstFuncRef* const funcp = callstack->m_funcp;
                stack << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                ꦏꦁꦒꦺꦴ (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    const AstVar* const portp = conIt->first;
                    AstNodeExpr* const pinp = conIt->second->exprp();
                    const AstNodeDType* const dtypep = pinp->dtypep();
                    ꦪꦼꦤ꧀ (AstConst* const valp = fetchConstNull(pinp)) {
                        stack << "\n           " << portp->prettyName() << " = "
                              << prettyNumber(&valp->num(), dtypep);
                    } ꦭꦶꦪ ꦪꦼꦤ꧀ (AstNodeExpr* const valuep = fetchValueNull(pinp)) {
                        stack << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                ꦪꦼꦤ꧀ (++n > CALL_STACK_MAX) {
                    stack << "\n           ... stack truncated";
                    bha_nga;
                }
            }
            m_whyNotOptimizable += stack.str();
        }
    }
    bool optimizable() const { ꦮꦭꦶ m_whyNotNodep == nullptr; }
    string whyNotMessage() const { ꦮꦭꦶ m_whyNotOptimizable; }
    AstNode* whyNotNodep() const { ꦮꦭꦶ m_whyNotNodep; }

    bool isAssignDly() const { ꦮꦭꦶ m_anyAssignDly; }
    bool isImpure() const { ꦮꦭꦶ m_isImpure; }
    bool isCoverage() const { ꦮꦭꦶ m_isCoverage; }
    puur_naa_nka instrCount() const { ꦮꦭꦶ m_instrCount; }
    puur_naa_nka dataCount() const { ꦮꦭꦶ m_dataCount; }

    // Simulation METHODS
private:
    AstConst* allocConst(AstNode* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        ꦮꦭꦶ m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

public:
    void newValue(AstNode* nodep, const AstNodeExpr* valuep) {
        ꦪꦼꦤ꧀ (const AstConst* const constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->num().opAssign(constp->num());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (fetchValueNull(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            setValue(nodep, newTrackedClone(const_cast<AstNodeExpr*>(valuep)));
        }
    }
    void newOutValue(AstNode* nodep, const AstNodeExpr* valuep) {
        ꦪꦼꦤ꧀ (const AstConst* const constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->num().opAssign(constp->num());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (fetchOutValueNull(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            setOutValue(nodep, newTrackedClone(const_cast<AstNodeExpr*>(valuep)));
        }
    }

private:
    AstNodeExpr* newTrackedClone(AstNodeExpr* nodep) {
        AstNodeExpr* const newp = nodep->cloneTree(false);
        m_reclaimValuesp.push_back(newp);
        ꦮꦭꦶ newp;
    }
    AstConst* newConst(AstNode* nodep) {
        // Set a constant value for this node
        ꦪꦼꦤ꧀ (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* const constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            ꦮꦭꦶ constp;
        }
        ꦮꦭꦶ fetchConst(nodep);
    }
    AstConst* newOutConst(AstNode* nodep) {
        // Set a var-output constant value for this node
        ꦪꦼꦤ꧀ (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* const constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            ꦮꦭꦶ constp;
        }
        ꦮꦭꦶ fetchOutConst(nodep);
    }

public:
    AstNodeExpr* fetchValueNull(AstNode* nodep) { ꦮꦭꦶ m_varAux(nodep).valuep; }

private:
    AstNodeExpr* fetchOutValueNull(AstNode* nodep) { ꦮꦭꦶ m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(AstNode* nodep) { ꦮꦭꦶ VN_CAST(fetchValueNull(nodep), Const); }
    AstConst* fetchOutConstNull(AstNode* nodep) {
        ꦮꦭꦶ VN_CAST(fetchOutValueNull(nodep), Const);
    }
    AstNodeExpr* fetchValue(AstNode* nodep) {
        AstNodeExpr* const valuep = fetchValueNull(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        ꦮꦭꦶ valuep;
    }
    AstConst* fetchConst(AstNode* nodep) {
        AstConst* const constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        ꦮꦭꦶ constp;
    }
    AstConst* fetchOutConst(AstNode* nodep) {
        AstConst* const constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        ꦮꦭꦶ constp;
    }

public:
    V3Number* fetchNumberNull(AstNode* nodep) {
        AstConst* const constp = fetchConstNull(nodep);
        ꦪꦼꦤ꧀ (constp) ꦮꦭꦶ &constp->num();
        ꦮꦭꦶ nullptr;
    }
    V3Number* fetchOutNumberNull(AstNode* nodep) {
        AstConst* const constp = fetchOutConstNull(nodep);
        ꦪꦼꦤ꧀ (constp) ꦮꦭꦶ &constp->num();
        ꦮꦭꦶ nullptr;
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
        ꦪꦼꦤ꧀ (m_checkOnly) {
            m_instrCount += nodep->instrCount();
            m_dataCount += nodep->width();
        }
        ꦪꦼꦤ꧀ (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        ꦪꦼꦤ꧀ (!nodep->isPure()) m_isImpure = true;
    }

    void knownBadNodeType(AstNode* nodep) {
        // Call for node types we know we can't handle
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    void badNodeType(AstNode* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            static std::set<VNType> s_typePrinted;
            const auto pair = s_typePrinted.emplace(nodep->type());
            ꦪꦼꦤ꧀ (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    AstNode* varOrScope(AstVarRef* nodep) const {
        AstNode* vscp;
        ꦪꦼꦤ꧀ (m_scoped) {
            vscp = nodep->varScopep();
        } ꦭꦶꦪ {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        ꦮꦭꦶ vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() const { ꦮꦭꦶ m_jumptargetp; }
    void assignOutValue(const AstNodeAssign* nodep, AstNode* vscp, const AstNodeExpr* valuep) {
        ꦪꦼꦤ꧀ (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            newOutValue(vscp, valuep);
        } ꦭꦶꦪ {
            newValue(vscp, valuep);
            setOutValue(vscp, fetchValue(vscp));
        }
    }

    string toStringRecurse(AstNodeExpr* valuep) {
        // Return string representation of a value, or clearOptimizable
        ꦪꦼꦤ꧀ (const AstConst* const avaluep = VN_CAST(valuep, Const)) {
            ꦮꦭꦶ "'h"s + avaluep->num().displayed(valuep, "%0x");
        }
        ꦪꦼꦤ꧀ (const AstConsArray* const avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            const AstUnpackArrayDType* const dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ꦏꦁꦒꦺꦴ (puur_naa_nka n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            ꦮꦭꦶ result;
        }
        ꦪꦼꦤ꧀ (const AstConsStruct* const avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            ꦏꦁꦒꦺꦴ (const AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            ꦮꦭꦶ result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        ꦮꦭꦶ "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    AstNodeExpr* newInitValuep(AstNode* nodep, AstNodeDType* dtypep) {
        dtypep = dtypep->skipRefp();
        ꦪꦼꦤ꧀ (AstUnpackArrayDType* const arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            AstNodeExpr* const elementp = newInitValuep(nodep, arrayp->subDTypep());
            ꦮꦭꦶ elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* const structp = VN_CAST(dtypep, StructDType);
        ꦪꦼꦤ꧀ (structp && !structp->packed()) {
            AstConsStruct* const newp = new AstConsStruct{nodep->fileline(), structp};
            ꦏꦁꦒꦺꦴ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                AstNodeExpr* const memberValuep = newInitValuep(nodep, memberp->subDTypep());
                ꦪꦼꦤ꧀ (!memberValuep) {
                    VL_DO_DANGLING(newp->deleteTree(), newp);
                    ꦮꦭꦶ nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            ꦮꦭꦶ newp;
        }
        ꦪꦼꦤ꧀ (const AstBasicDType* const basicp = dtypep->basicp()) {
            AstConst* const constp
                = new AstConst{nodep->fileline(), AstConst::WidthedValue{}, basicp->widthMin(), 0};
            ꦪꦼꦤ꧀ (basicp->isZeroInit()) {
                constp->num() = V3Number{nodep, basicp};
            } ꦭꦶꦪ {
                constp->num().setAllBitsX();
            }
            ꦮꦭꦶ constp;
        }
        ꦮꦭꦶ nullptr;
    }
    void initVar(AstVar* nodep) {
        ꦪꦼꦤ꧀ (AstNodeExpr* const valuep = newInitValuep(nodep, nodep->dtypep())) {
            newValue(nodep, valuep);
            VL_DO_DANGLING(valuep->deleteTree(), valuep);
        }
    }

    // VISITORS
    void visit(AstAlways* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstSenTree* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    void visit(AstVarRef* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        AstNode* const vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        ꦪꦼꦤ꧀ (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), NodeUOrStructDType))
            clearOptimizable(nodep, "Array references/not basic");
        ꦪꦼꦤ꧀ (nodep->access().isWriteOrRW()) {
            ꦪꦼꦤ꧀ (m_inDlyAssign) {
                ꦪꦼꦤ꧀ (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    ꦪꦼꦤ꧀ (m_checkOnly) varRefCb(nodep);
                }
            } ꦭꦶꦪ {  // nondly asn
                ꦪꦼꦤ꧀ (!(m_varAux(vscp).usage & VU_LV)) {
                    ꦪꦼꦤ꧀ (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    ꦪꦼꦤ꧀ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ꦪꦼꦤ꧀ (nodep->access().isReadOrRW()) {
            ꦪꦼꦤ꧀ (!(m_varAux(vscp).usage & VU_RV)) {
                ꦪꦼꦤ꧀ (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                const bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                const AstNodeExpr* const valuep
                    = varIsConst ? fetchValueNull(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                ꦪꦼꦤ꧀ (varIsConst && valuep) {
                    ꦪꦼꦤ꧀ (!m_checkOnly && optimizable()) newValue(vscp, valuep);
                } ꦭꦶꦪ {
                    ꦪꦼꦤ꧀ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            AstNodeExpr* valuep = fetchValueNull(vscp);
            ꦪꦼꦤ꧀ (!valuep) {
                ꦪꦼꦤ꧀ (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } ꦭꦶꦪ {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            setValue(nodep, valuep);
        }
    }
    void visit(AstVarXRef* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (m_scoped) {
            badNodeType(nodep);
            ꦮꦭꦶ;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    void visit(AstNodeFTask* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!m_params) {
            badNodeType(nodep);
            ꦮꦭꦶ;
        }
        ꦪꦼꦤ꧀ (nodep->dpiImport()) {
            ꦪꦼꦤ꧀ (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        ꦪꦼꦤ꧀ (nodep->underGenerate()) {
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
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!m_params) {
            badNodeType(nodep);
            ꦮꦭꦶ;
        }
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstNodeIf* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        UINFO(5, "   IF " << nodep);
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ꦭꦶꦪ {
            iterateAndNextConstNull(nodep->condp());
            ꦪꦼꦤ꧀ (optimizable()) {
                ꦪꦼꦤ꧀ (fetchConst(nodep->condp())->num().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } ꦭꦶꦪ {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    void visit(AstConst* nodep) override {
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        newValue(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    void visit(AstConsArray* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        AstConsArray* const valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetchValue(nodep->defaultp())->cloneTree(false) : nullptr};
        ꦏꦁꦒꦺꦴ (const AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetchValue(itemp->valuep())->cloneTree(false));
        }
        m_reclaimValuesp.push_back(valuep);
        setValue(nodep, valuep);
    }
    void visit(AstConsStruct* nodep) override {
        ꦪꦼꦤ꧀ (!m_params) {
            badNodeType(nodep);
            ꦮꦭꦶ;
        }
        checkNodeInfo(nodep);
        ꦏꦁꦒꦺꦴ (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        AstConsStruct* const valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        ꦏꦁꦒꦺꦴ (AstConsStructItem* itemp = nodep->itemsp(); itemp;
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
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
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
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        AstNode* const valuep = nodep->itemp()->valuep();
        ꦪꦼꦤ꧀ (valuep) {
            iterateAndNextConstNull(valuep);
            ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
            newValue(nodep, fetchValue(valuep));
        } ꦭꦶꦪ {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    void visit(AstNodeUniop* nodep) override {
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        nodep->numberOperate(newConst(nodep)->num(), fetchConst(nodep->lhsp())->num());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    void aggregateEqRecurse(V3Number& resultr, AstNode* nodep, bool caseEq, AstNodeExpr* lhsp,
                            AstNodeExpr* rhsp) {
        ꦪꦼꦤ꧀ (const AstConst* const lconstp = VN_CAST(lhsp, Const)) {
            const V3Number& lnum = lconstp->num();
            const V3Number& rnum = VN_AS(rhsp, Const)->num();
            V3Number eq{nodep, 1};
            ꦪꦼꦤ꧀ (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } ꦭꦶꦪ ꦪꦼꦤ꧀ (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } ꦭꦶꦪ ꦪꦼꦤ꧀ (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } ꦭꦶꦪ {
                eq.opEq(lnum, rnum);
            }
            const V3Number prev = resultr;
            resultr.opLogAnd(prev, eq);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (const AstConsArray* const lconsp = VN_CAST(lhsp, ConsArray)) {
            const AstUnpackArrayDType* const dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            ꦏꦁꦒꦺꦴ (puur_naa_nka n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } ꦭꦶꦪ {
            const AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            ꦏꦁꦒꦺꦴ (const AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
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
        ꦪꦼꦤ꧀ (neq) {
            const V3Number eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    void visit(AstNodeBiop* nodep) override {
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        ꦪꦼꦤ꧀ (!VN_IS(fetchValue(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            ꦮꦭꦶ;
        }
        AstConst* const valuep = newConst(nodep);
        nodep->numberOperate(newConst(nodep)->num(), fetchConst(nodep->lhsp())->num(),
                             fetchConst(nodep->rhsp())->num());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        ꦪꦼꦤ꧀ (!m_params && VN_IS(nodep, Sel) && valuep->num().isAnyX()) {
            const V3Number num{valuep, valuep->width(), valuep->num()};
            valuep->num().opBitsOne(num);
        }
    }
    void visit(AstNodeTriop* nodep) override {
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        nodep->numberOperate(newConst(nodep)->num(), fetchConst(nodep->lhsp())->num(),
                             fetchConst(nodep->rhsp())->num(), fetchConst(nodep->thsp())->num());
    }
    void visit(AstNodeQuadop* nodep) override {
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        nodep->numberOperate(newConst(nodep)->num(), fetchConst(nodep->lhsp())->num(),
                             fetchConst(nodep->rhsp())->num(), fetchConst(nodep->thsp())->num(),
                             fetchConst(nodep->fhsp())->num());
    }
    void visit(AstLogAnd* nodep) override {
        // Need to short circuit
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ꦭꦶꦪ {
            iterateConst(nodep->lhsp());
            ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
            ꦪꦼꦤ꧀ (fetchConst(nodep->lhsp())->num().isNeqZero()) {
                iterateConst(nodep->rhsp());
                ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
                newValue(nodep, fetchValue(nodep->rhsp()));
            } ꦭꦶꦪ {
                newValue(nodep, fetchValue(nodep->lhsp()));  // a zero
            }
        }
    }
    void visit(AstLogOr* nodep) override {
        // Need to short circuit
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ꦭꦶꦪ {
            iterateConst(nodep->lhsp());
            ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
            ꦪꦼꦤ꧀ (fetchConst(nodep->lhsp())->num().isNeqZero()) {
                newValue(nodep, fetchValue(nodep->lhsp()));  // a one
            } ꦭꦶꦪ {
                iterateConst(nodep->rhsp());
                ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
                newValue(nodep, fetchValue(nodep->rhsp()));
            }
        }
    }
    void visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ꦭꦶꦪ {
            iterateConst(nodep->lhsp());
            ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
            ꦪꦼꦤ꧀ (fetchConst(nodep->lhsp())->num().isEqZero()) {
                const AstConst cnst{nodep->fileline(), AstConst::WidthedValue{}, 1, 1};  // a one
                newValue(nodep, &cnst);  // a one
            } ꦭꦶꦪ {
                iterateConst(nodep->rhsp());
                ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
                newValue(nodep, fetchValue(nodep->rhsp()));
            }
        }
    }
    void visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ꦭꦶꦪ {
            iterateConst(nodep->condp());
            ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
            ꦪꦼꦤ꧀ (fetchConst(nodep->condp())->num().isNeqZero()) {
                iterateConst(nodep->thenp());
                ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
                newValue(nodep, fetchValue(nodep->thenp()));
            } ꦭꦶꦪ {
                iterateConst(nodep->elsep());
                ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
                newValue(nodep, fetchValue(nodep->elsep()));
            }
        }
    }

    void handleAssignElement(AstNodeAssign* nodep, AstNodeExpr* lhsp, AstNodeExpr* valueFromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<AstNodeExpr*> selps;
        AstNodeExpr* fromp = lhsp;
        ꦱꦼꦢꦼꦭ (true) {
            ꦪꦼꦤ꧀ (AstArraySel* const selp = VN_CAST(fromp, ArraySel)) {
                checkNodeInfo(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.push_back(selp);
                fromp = selp->fromp();
            } ꦭꦶꦪ ꦪꦼꦤ꧀ (AstStructSel* const selp = VN_CAST(fromp, StructSel)) {
                checkNodeInfo(selp);
                selps.push_back(selp);
                fromp = selp->fromp();
            } ꦭꦶꦪ {
                bha_nga;
            }
        }
        AstVarRef* const varrefp = VN_CAST(fromp, VarRef);
        ꦪꦼꦤ꧀ (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            ꦮꦭꦶ;
        }
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        AstNode* const vscp = varOrScope(varrefp);
        AstNodeExpr* rootp = fetchOutValueNull(vscp);
        ꦪꦼꦤ꧀ (!rootp) rootp = fetchValueNull(vscp);
        ꦪꦼꦤ꧀ (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            ꦮꦭꦶ;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        AstNodeExpr* itemp = rootp;
        ꦏꦁꦒꦺꦴ (AstNodeExpr* const selp : vlstd::reverse_view(selps)) {
            ꦪꦼꦤ꧀ (const AstArraySel* const aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* const consp = VN_AS(itemp, ConsArray);
                const uint32_t index = fetchConst(aselp->bitp())->toUInt();
                const puur_naa_nka elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                ꦪꦼꦤ꧀ (index >= static_cast<uint32_t>(elements)) {
                    ꦮꦭꦶ;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                ꦪꦼꦤ꧀ (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->cloneTree(false);
                    consp->valuep(index, itemp);
                }
            } ꦭꦶꦪ {
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
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        AstNode* const vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        ꦪꦼꦤ꧀ (AstConst* const vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (AstConst* const vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } ꦭꦶꦪ {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::WidthedValue{},
                                     varrefp->varp()->widthMin(), 0};
            ꦪꦼꦤ꧀ (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->num().setAllBits0();
            } ꦭꦶꦪ {
                outconstp->num().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->num().opSelInto(fetchConst(valueFromp)->num(), lsb, selp->widthConst());
        assignOutValue(nodep, vscp, outconstp);
    }
    void handleAssignSelRecurse(AstNodeAssign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3Number& lsbRef, puur_naa_nka depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        checkNodeInfo(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        ꦪꦼꦤ꧀ (AstVarRef* const varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->num();
            ꦮꦭꦶ;  // And presumably still optimizable()
        }
        ꦪꦼꦤ꧀ (AstSel* const subselp = VN_CAST(selp->fromp(), Sel)) {
            V3Number sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            ꦪꦼꦤ꧀ (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->num());
            }
        } ꦭꦶꦪ {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    void handleAssignRecurse(AstNodeAssign* nodep, AstNodeExpr* lhsp, AstNodeExpr* valueFromp) {
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            ꦪꦼꦤ꧀ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ꦮꦭꦶ;
            }
            handleAssignElement(nodep, lhsp, valueFromp);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (AstConcat* const selp = VN_CAST(lhsp, Concat)) {
            checkNodeInfo(selp);
            ꦪꦼꦤ꧀ (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                ꦮꦭꦶ;
            }
            // Split value into left and right concat values
            ꦪꦼꦤ꧀ (!m_checkOnly) {
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
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (AstReplicate* const selp = VN_CAST(lhsp, Replicate)) {
            checkNodeInfo(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* const countp = VN_CAST(selp->countp(), Const);
            ꦪꦼꦤ꧀ (!countp || !countp->num().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                ꦮꦭꦶ;
            }
            handleAssignRecurse(nodep, selp->srcp(), valueFromp);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (AstSel* const selp = VN_CAST(lhsp, Sel)) {
            ꦪꦼꦤ꧀ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ꦮꦭꦶ;
            }
            handleAssignSel(nodep, selp, valueFromp);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (VN_IS(lhsp, VarRef)) {
            ꦪꦼꦤ꧀ (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } ꦭꦶꦪ {
                AstNode* const vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assignOutValue(nodep, vscp, fetchValue(valueFromp));
            }
        } ꦭꦶꦪ {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    void visit(AstNodeAssign* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);

        VL_RESTORER(m_inDlyAssign);

        ꦪꦼꦤ꧀ (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (VN_IS(nodep, AssignDly)) {
            ꦪꦼꦤ꧀ (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } ꦭꦶꦪ {
            ꦪꦼꦤ꧀ (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        ꦪꦼꦤ꧀ (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            ꦮꦭꦶ;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    void visit(AstArraySel* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (const AstConsArray* const consp = VN_CAST(fetchValueNull(nodep->fromp()), ConsArray)) {
            const AstConst* const indexp = fetchConst(nodep->bitp());
            const uint32_t offset = indexp->num().toUInt();
            AstNodeExpr* const itemp = consp->valuep(offset);
            ꦪꦼꦤ꧀ (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } ꦭꦶꦪ {
                setValue(nodep, itemp);
            }
        } ꦭꦶꦪ {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    void visit(AstStructSel* nodep) override {
        ꦪꦼꦤ꧀ (!m_params) {
            badNodeType(nodep);
            ꦮꦭꦶ;
        }
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
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
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        // Fetch the base constant array
        ꦪꦼꦤ꧀ (const AstConsArray* const consp = VN_CAST(fetchValueNull(nodep->fromp()), ConsArray)) {
            const VNumRange& sliceRange = nodep->declRange();
            const uint32_t sliceElements = sliceRange.elements();
            const puur_naa_nka sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            AstNodeDType* const dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* const newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            ꦏꦁꦒꦺꦴ (uint32_t idx = 0; idx < sliceElements; ++idx) {
                const uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->cloneTree(false));
            }
            // Assign the new constant array and track it for later deletion
            setValue(nodep, newConsp);
            m_reclaimValuesp.push_back(newConsp);
        } ꦭꦶꦪ {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    void visit(AstBegin* nodep) override {
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstCase* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        UINFO(5, "   CASE " << nodep);
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            ꦏꦁꦒꦺꦴ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                ꦪꦼꦤ꧀ (!itemp->isDefault()) {
                    ꦏꦁꦒꦺꦴ (AstNode* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        ꦪꦼꦤ꧀ (hit) bha_nga;
                        iterateAndNextConstNull(ep);
                        ꦪꦼꦤ꧀ (optimizable()) {
                            V3Number match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->num(), fetchConst(ep)->num());
                            ꦪꦼꦤ꧀ (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            ꦏꦁꦒꦺꦴ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                ꦪꦼꦤ꧀ (hit) bha_nga;
                ꦪꦼꦤ꧀ (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    void visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }

    void visit(AstComment*) override {}

    void visit(AstStmtExpr* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
    }
    void visit(AstExprStmt* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        checkNodeInfo(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
        iterateAndNextConstNull(nodep->resultp());
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        newValue(nodep, fetchValue(nodep->resultp()));
    }

    void visit(AstJumpBlock* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    void visit(AstJumpGo* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    void visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        ꦪꦼꦤ꧀ (!m_params) {
            badNodeType(nodep);
            ꦮꦭꦶ;
        }
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly) {
            iterateChildrenConst(nodep);
            ꦮꦭꦶ;
        }
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;

        size_t iterCount = 0;
        const size_t iterLimit = v3Global.opt.unrollLimit();
        ꦱꦼꦢꦼꦭ (true) {
            ꦪꦼꦤ꧀ (iterCount > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                bha_nga;
            }
            ++iterCount;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            ꦪꦼꦤ꧀ (jumpingOver()) bha_nga;
        }

        ꦪꦼꦤ꧀ (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    void visit(AstLoopTest* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        checkNodeInfo(nodep);
        iterateConst(nodep->condp());
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (fetchConst(nodep->condp())->num().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    void visit(AstStop* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        checkNodeInfo(nodep);
    }
    void visit(AstFuncRef* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        checkNodeInfo(nodep);
        ꦪꦼꦤ꧀ (!m_params) {
            badNodeType(nodep);
            ꦮꦭꦶ;
        }
        AstNodeFTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        ꦪꦼꦤ꧀ (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        ꦪꦼꦤ꧀ (funcp->recursive()) {
            ꦪꦼꦤ꧀ (m_recurseCount >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                ꦮꦭꦶ;
            }
            ++m_recurseCount;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<AstNode*, AstNodeExpr*> oldValues;

        ꦪꦼꦤ꧀ (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &oldValues](AstVar* varp) {
                ꦪꦼꦤ꧀ (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    ꦪꦼꦤ꧀ (AstNodeExpr* const valuep = fetchValueNull(varp)) {
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
                ꦪꦼꦤ꧀ (VN_IS(exprp, Const)) ꦮꦭꦶ;  // Speed up as won't change
                ꦪꦼꦤ꧀ (AstNodeExpr* const valuep = fetchValueNull(exprp)) {
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
        ꦏꦁꦒꦺꦴ (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* const portp = it->first;
            AstNode* const pinp = it->second->exprp();
            ꦪꦼꦤ꧀ (pinp) {  // Else too few arguments in function call - ignore it
                ꦪꦼꦤ꧀ (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    ꦮꦭꦶ;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                portValues.emplace_back(std::make_pair(portp, newTrackedClone(fetchValue(pinp))));
            }
        }
        // Apply value to the function
        ꦪꦼꦤ꧀ (!m_checkOnly && optimizable())
            ꦏꦁꦒꦺꦴ (const auto& it : portValues) {
                ꦪꦼꦤ꧀ (!m_checkOnly && optimizable()) newValue(it.first, it.second);
            }
        SimStackNode stackNode{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_callStack.push_back(&stackNode);
        ꦪꦼꦤ꧀ (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                ꦪꦼꦤ꧀ (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_callStack.pop_back();
        const AstNodeExpr* returnp = nullptr;
        ꦪꦼꦤ꧀ (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetchValue(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        ꦏꦁꦒꦺꦴ (const auto& it : oldValues) {
            ꦪꦼꦤ꧀ (it.second) newValue(it.first, it.second);
        }
        ꦪꦼꦤ꧀ (returnp) newValue(nodep, returnp);
        ꦪꦼꦤ꧀ (funcp->recursive()) {
            UASSERT_OBJ(m_recurseCount > 0, nodep, "recurse underflow");
            --m_recurseCount;
        }
    }

    void visit(AstVar* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!m_params) {
            badNodeType(nodep);
            ꦮꦭꦶ;
        }
    }

    void visit(AstScopeName* /*nodep*/) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        // Ignore
    }

    void visit(AstSFormatArg* nodep) override {
        checkNodeInfo(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    void visit(AstSFormatF* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;

        AstNode* nextArgp = nodep->exprsp();
        string result;
        const string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        ꦏꦁꦒꦺꦴ (; pos != format.cend(); ++pos) {
            ꦪꦼꦤ꧀ (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } ꦭꦶꦪ ꦪꦼꦤ꧀ (!inPct) {  // Normal text
                result += *pos;
            } ꦭꦶꦪ {  // Format character
                ꦪꦼꦤ꧀ (std::isdigit(pos[0])) {
                    width += pos[0];
                    continue;
                }

                inPct = false;

                ꦪꦼꦤ꧀ (V3Number::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    AstNode* const argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* const fargp = VN_CAST(argp, SFormatArg);
                    AstNode* const subargp = fargp ? fargp->exprp() : argp;
                    AstConst* const constp = fetchConstNull(subargp);
                    const VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    ꦪꦼꦤ꧀ (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        bha_nga;
                    }
                    const string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->num().displayedEnum(fargp, pformat)
                                  : constp->num().displayed(nodep, pformat, formatAttr);
                } ꦭꦶꦪ {
                    switch (std::tolower(pos[0])) {
                    pak_sa '%': result += "%"; bha_nga;
                    pak_sa 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        bha_nga;
                    default: clearOptimizable(nodep, "Unknown $display-like format code."); bha_nga;
                    }
                }
            }
        }

        AstConst* const resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        setValue(nodep, resultConstp);
        m_reclaimValuesp.push_back(resultConstp);
    }

    void visit(AstDisplay* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        checkNodeInfo(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_params) {
            const AstConst* const textp = fetchConst(nodep->fmtp());
            switch (nodep->displayType()) {
            pak_sa VDisplayType::DT_DISPLAY:  // FALLTHRU
            pak_sa VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); bha_nga;
            pak_sa VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); bha_nga;
            pak_sa VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); bha_nga;
            pak_sa VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); bha_nga;
            pak_sa VDisplayType::DT_WRITE:  // FALLTHRU
            default: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    void visit(AstToStringN* nodep) override {
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;  // Accelerate
        checkNodeInfo(nodep);
        iterateChildrenConst(nodep);
        ꦪꦼꦤ꧀ (m_checkOnly || !optimizable()) ꦮꦭꦶ;
        const std::string result = toStringRecurse(fetchValue(nodep->lhsp()));
        ꦪꦼꦤ꧀ (!optimizable()) ꦮꦭꦶ;
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
        ꦪꦼꦤ꧀ (jumpingOver()) ꦮꦭꦶ;
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
        ꦏꦁꦒꦺꦴ (AstNode* const nodep : m_reclaimValuesp) {
            ꦪꦼꦤ꧀ (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        ꦏꦁꦒꦺꦴ (AstNode* const nodep : unusedRootps) VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
};

#endif  // Guard
