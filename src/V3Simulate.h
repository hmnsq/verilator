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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Task.h"
#ಅನ್ತರ್ಭಾವ "V3Width.h"

#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <sstream>
#ಅನ್ತರ್ಭಾವ <stack>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_ರಾಶಿ_ಗಂಟು final {
ಸಾರ್ವಜನಿಕ:
    // MEMBERS
    AstFuncRef* ಧ್ರುವ m_funcp;
    V3TaskConnects* ಧ್ರುವ m_tconnects;
    // CONSTRUCTORS
    Sim_ರಾಶಿ_ಗಂಟು(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ರಾಶಿ_ಗಂಟು() = ಸ್ವಭಾವ;
};

class SimulateVisitor VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

ಗುಪ್ತ:
    // CONSTANTS
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ CALL_ರಾಶಿ_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ಧ್ರುವ VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    ಗಣನಾ VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    ಸಂರಚನಾ AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nullptr;
    };

    AstUser1Allocator<Ast_ಗಂಟು, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    ಸಂರಚನಾ ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, Ast_ಗಂಟು* nodep) {
            ಯದಿ (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            ಯದಿ (m_nextFree == m_constps.size()) {
                m_constps.ತಳ್ಳು_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ಧ್ರುವ constp = m_constps[m_nextFree++];
            constp->ಅಂಕೆ().nodep(nodep);
            ಪ್ರತ್ಯಾಗಮ constp;
        }

        ~ConstAllocator() {
            ಪ್ರತಿ (AstConst* ಧ್ರುವ constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_ಗಂಟು* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    ಪೂರ್ಣಾಙ್ಕ m_instr_ಎಣಿಕೆ;  ///< Number of nodes
    ಪೂರ್ಣಾಙ್ಕ m_data_ಎಣಿಕೆ;  ///< Bytes of data
    ಪೂರ್ಣಾಙ್ಕ m_recurse_ಎಣಿಕೆ = 0;  // Now deep in current recursion
    Ast_ಗಂಟು* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು_DType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<Sim_ರಾಶಿ_ಗಂಟು*> m_call_ರಾಶಿ;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_ಗಂಟು*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_ಸಂಖ್ಯೆ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ* nump, ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        ಯದಿ (ಧ್ರುವ AstRefDType* ಧ್ರುವ refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ stp = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
            ಯದಿ (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                ಪ್ರತಿ (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = itemp->width();
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = itemp->lsb();
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = lsb + width - 1;
                    V3_ಸಂಖ್ಯೆ field_ಅಂಕೆ{nump, width};
                    field_ಅಂಕೆ.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ childTypep = itemp->subDTypep()) {
                        out << pretty_ಸಂಖ್ಯೆ(&field_ಅಂಕೆ, childTypep);
                    } ಅನ್ಯಥಾ {
                        out << field_ಅಂಕೆ;
                    }
                    ಯದಿ (itemp->nextp()) out << ", ";
                }
                out << "}";
                ಪ್ರತ್ಯಾಗಮ out.str();
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ arrayElements = arrayp->elementsConst();
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ element = 0; element < arrayElements; ++element) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = childTypep->width();
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = width * element;
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ msb = lsb + width - 1;
                    V3_ಸಂಖ್ಯೆ field_ಅಂಕೆ{nump, width};
                    field_ಅಂಕೆ.opSel(*nump, msb, lsb);
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_ಸಂಖ್ಯೆ(&field_ಅಂಕೆ, childTypep);
                    ಯದಿ (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                ಪ್ರತ್ಯಾಗಮ out.str();
            }
        }
        ಪ್ರತ್ಯಾಗಮ nump->ascii();
    }

    // Checking METHODS
ಸಾರ್ವಜನಿಕ:
    /// Call other-this function on all new *non-constant* var references
    ಆಭಾಸ ಶೂನ್ಯ varRefCb(AstVarRef* nodep) {}

    ಶೂನ್ಯ clearOptimizable(Ast_ಗಂಟು* nodep /*null ok*/, ಧ್ರುವ string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        ಯದಿ (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            ಯದಿ (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                ಯದಿ (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ರಾಶಿ;
            ಪೂರ್ಣಾಙ್ಕ n = 0;
            ಪ್ರತಿ (ಧ್ರುವ auto& callstack : vlstd::reverse_view(m_call_ರಾಶಿ)) {
                ಧ್ರುವ AstFuncRef* ಧ್ರುವ funcp = callstack->m_funcp;
                ರಾಶಿ << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                ಪ್ರತಿ (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    ಧ್ರುವ AstVar* ಧ್ರುವ portp = conIt->first;
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ pinp = conIt->second->exprp();
                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = pinp->dtypep();
                    ಯದಿ (AstConst* ಧ್ರುವ valp = fetchConstNull(pinp)) {
                        ರಾಶಿ << "\n           " << portp->prettyName() << " = "
                              << pretty_ಸಂಖ್ಯೆ(&valp->ಅಂಕೆ(), dtypep);
                    } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = fetch_ಮೌಲ್ಯ_Null(pinp)) {
                        ರಾಶಿ << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                ಯದಿ (++n > CALL_ರಾಶಿ_MAX) {
                    ರಾಶಿ << "\n           ... stack truncated";
                    ಭಙ್ಗ;
                }
            }
            m_whyNotOptimizable += ರಾಶಿ.str();
        }
    }
    bool optimizable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_whyNotNodep == nullptr; }
    string whyNotMessage() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_whyNotOptimizable; }
    Ast_ಗಂಟು* whyNotNodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_whyNotNodep; }

    bool isAssignDly() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_anyAssignDly; }
    bool isImpure() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isImpure; }
    bool isCoverage() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isCoverage; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_instr_ಎಣಿಕೆ; }
    ಪೂರ್ಣಾಙ್ಕ data_ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_data_ಎಣಿಕೆ; }

    // Simulation METHODS
ಗುಪ್ತ:
    AstConst* allocConst(Ast_ಗಂಟು* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        ಪ್ರತ್ಯಾಗಮ m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ new_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->ಅಂಕೆ().opAssign(constp->ಅಂಕೆ());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } ಅನ್ಯಥಾ ಯದಿ (fetch_ಮೌಲ್ಯ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_ಮೌಲ್ಯ(nodep, newTrackedClone(const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(valuep)));
        }
    }
    ಶೂನ್ಯ new_Out_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->ಅಂಕೆ().opAssign(constp->ಅಂಕೆ());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } ಅನ್ಯಥಾ ಯದಿ (fetch_Out_ಮೌಲ್ಯ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_ಮೌಲ್ಯ(nodep, newTrackedClone(const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(valuep)));
        }
    }

ಗುಪ್ತ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newTrackedClone(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = nodep->clone_ವೃಕ್ಷ(false);
        m_reclaimValuesp.ತಳ್ಳು_back(newp);
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    AstConst* newConst(Ast_ಗಂಟು* nodep) {
        // Set a constant value for this node
        ಯದಿ (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ಧ್ರುವ constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            ಪ್ರತ್ಯಾಗಮ constp;
        }
        ಪ್ರತ್ಯಾಗಮ fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_ಗಂಟು* nodep) {
        // Set a var-output constant value for this node
        ಯದಿ (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* ಧ್ರುವ constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            ಪ್ರತ್ಯಾಗಮ constp;
        }
        ಪ್ರತ್ಯಾಗಮ fetchOutConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fetch_ಮೌಲ್ಯ_Null(Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ m_varAux(nodep).valuep; }

ಗುಪ್ತ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fetch_Out_ಮೌಲ್ಯ_Null(Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ VN_CAST(fetch_ಮೌಲ್ಯ_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_ಗಂಟು* nodep) {
        ಪ್ರತ್ಯಾಗಮ VN_CAST(fetch_Out_ಮೌಲ್ಯ_Null(nodep), Const);
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fetch_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = fetch_ಮೌಲ್ಯ_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        ಪ್ರತ್ಯಾಗಮ valuep;
    }
    AstConst* fetchConst(Ast_ಗಂಟು* nodep) {
        AstConst* ಧ್ರುವ constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        ಪ್ರತ್ಯಾಗಮ constp;
    }
    AstConst* fetchOutConst(Ast_ಗಂಟು* nodep) {
        AstConst* ಧ್ರುವ constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        ಪ್ರತ್ಯಾಗಮ constp;
    }

ಸಾರ್ವಜನಿಕ:
    V3_ಸಂಖ್ಯೆ* fetch_ಸಂಖ್ಯೆ_Null(Ast_ಗಂಟು* nodep) {
        AstConst* ಧ್ರುವ constp = fetchConstNull(nodep);
        ಯದಿ (constp) ಪ್ರತ್ಯಾಗಮ &constp->ಅಂಕೆ();
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    V3_ಸಂಖ್ಯೆ* fetch_Out_ಸಂಖ್ಯೆ_Null(Ast_ಗಂಟು* nodep) {
        AstConst* ಧ್ರುವ constp = fetchOutConstNull(nodep);
        ಯದಿ (constp) ಪ್ರತ್ಯಾಗಮ &constp->ಅಂಕೆ();
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

ಗುಪ್ತ:
    ಶೂನ್ಯ set_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    ಶೂನ್ಯ set_Out_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    ಶೂನ್ಯ check_ಗಂಟು_Info(Ast_ಗಂಟು* nodep, bool ignorePredict = false) {
        ಯದಿ (m_checkOnly) {
            m_instr_ಎಣಿಕೆ += nodep->instr_ಎಣಿಕೆ();
            m_data_ಎಣಿಕೆ += nodep->width();
        }
        ಯದಿ (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        ಯದಿ (!nodep->isPure()) m_isImpure = true;
    }

    ಶೂನ್ಯ known_Bad_ಗಂಟು_Type(Ast_ಗಂಟು* nodep) {
        // Call for node types we know we can't handle
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    ಶೂನ್ಯ bad_ಗಂಟು_Type(Ast_ಗಂಟು* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            ಸ್ಥಿರ std::set<VNType> s_typePrinted;
            ಧ್ರುವ auto pair = s_typePrinted.emplace(nodep->type());
            ಯದಿ (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_ಗಂಟು* varOrScope(AstVarRef* nodep) ಧ್ರುವ {
        Ast_ಗಂಟು* vscp;
        ಯದಿ (m_scoped) {
            vscp = nodep->varScopep();
        } ಅನ್ಯಥಾ {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        ಪ್ರತ್ಯಾಗಮ vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_jumptargetp; }
    ಶೂನ್ಯ assign_Out_ಮೌಲ್ಯ(ಧ್ರುವ Ast_ಗಂಟು_Assign* nodep, Ast_ಗಂಟು* vscp, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಯದಿ (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_ಮೌಲ್ಯ(vscp, valuep);
        } ಅನ್ಯಥಾ {
            new_ಮೌಲ್ಯ(vscp, valuep);
            set_Out_ಮೌಲ್ಯ(vscp, fetch_ಮೌಲ್ಯ(vscp));
        }
    }

    string toStringRecurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        // Return string representation of a value, or clearOptimizable
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ avaluep = VN_CAST(valuep, Const)) {
            ಪ್ರತ್ಯಾಗಮ "'h"s + avaluep->ಅಂಕೆ().displayed(valuep, "%0x");
        }
        ಯದಿ (ಧ್ರುವ AstConsArray* ಧ್ರುವ avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            ಪ್ರತ್ಯಾಗಮ result;
        }
        ಯದಿ (ಧ್ರುವ AstConsStruct* ಧ್ರುವ avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            ಪ್ರತಿ (ಧ್ರುವ AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            ಪ್ರತ್ಯಾಗಮ result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        ಪ್ರತ್ಯಾಗಮ "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newInitValuep(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elementp = newInitValuep(nodep, arrayp->subDTypep());
            ಪ್ರತ್ಯಾಗಮ elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ಧ್ರುವ structp = VN_CAST(dtypep, StructDType);
        ಯದಿ (structp && !structp->packed()) {
            AstConsStruct* ಧ್ರುವ newp = new AstConsStruct{nodep->fileline(), structp};
            ಪ್ರತಿ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ memberValuep = newInitValuep(nodep, memberp->subDTypep());
                ಯದಿ (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_ವೃಕ್ಷ(), newp);
                    ಪ್ರತ್ಯಾಗಮ nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            ಪ್ರತ್ಯಾಗಮ newp;
        }
        ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp()) {
            AstConst* ಧ್ರುವ constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, basicp->widthMin(), 0};
            ಯದಿ (basicp->isZeroInit()) {
                constp->ಅಂಕೆ() = V3_ಸಂಖ್ಯೆ{nodep, basicp};
            } ಅನ್ಯಥಾ {
                constp->ಅಂಕೆ().setAllBitsX();
            }
            ಪ್ರತ್ಯಾಗಮ constp;
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ initVar(AstVar* nodep) {
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_ಮೌಲ್ಯ(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_ವೃಕ್ಷ(), valuep);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_ಗಂಟು* ಧ್ರುವ vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        ಯದಿ (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), ಗಂಟು_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        ಯದಿ (nodep->access().isWriteOrRW()) {
            ಯದಿ (m_inDlyAssign) {
                ಯದಿ (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    ಯದಿ (m_checkOnly) varRefCb(nodep);
                }
            } ಅನ್ಯಥಾ {  // nondly asn
                ಯದಿ (!(m_varAux(vscp).usage & VU_LV)) {
                    ಯದಿ (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    ಯದಿ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ಯದಿ (nodep->access().isReadOrRW()) {
            ಯದಿ (!(m_varAux(vscp).usage & VU_RV)) {
                ಯದಿ (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                ಧ್ರುವ bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep
                    = varIsConst ? fetch_ಮೌಲ್ಯ_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                ಯದಿ (varIsConst && valuep) {
                    ಯದಿ (!m_checkOnly && optimizable()) new_ಮೌಲ್ಯ(vscp, valuep);
                } ಅನ್ಯಥಾ {
                    ಯದಿ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = fetch_ಮೌಲ್ಯ_Null(vscp);
            ಯದಿ (!valuep) {
                ಯದಿ (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } ಅನ್ಯಥಾ {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            set_ಮೌಲ್ಯ(nodep, valuep);
        }
    }
    ಶೂನ್ಯ visit(AstVarXRef* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_scoped) {
            bad_ಗಂಟು_Type(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->dpiImport()) {
            ಯದಿ (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        ಯದಿ (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstInitialAutomaticStmt* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstInitialStaticStmt* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstInitialStatic* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(5, "   IF " << nodep);
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಅನ್ಯಥಾ {
            iterateAndNextConstNull(nodep->condp());
            ಯದಿ (optimizable()) {
                ಯದಿ (fetchConst(nodep->condp())->ಅಂಕೆ().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } ಅನ್ಯಥಾ {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        new_ಮೌಲ್ಯ(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    ಶೂನ್ಯ visit(AstConsArray* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        AstConsArray* ಧ್ರುವ valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_ಮೌಲ್ಯ(nodep->defaultp())->clone_ವೃಕ್ಷ(false) : nullptr};
        ಪ್ರತಿ (ಧ್ರುವ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_ಮೌಲ್ಯ(itemp->valuep())->clone_ವೃಕ್ಷ(false));
        }
        m_reclaimValuesp.ತಳ್ಳು_back(valuep);
        set_ಮೌಲ್ಯ(nodep, valuep);
    }
    ಶೂನ್ಯ visit(AstConsStruct* nodep) override {
        ಯದಿ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        check_ಗಂಟು_Info(nodep);
        ಪ್ರತಿ (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        AstConsStruct* ಧ್ರುವ valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        ಪ್ರತಿ (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_ಮೌಲ್ಯ(itemp->valuep())->clone_ವೃಕ್ಷ(false)});
        }
        m_reclaimValuesp.ತಳ್ಳು_back(valuep);
        set_ಮೌಲ್ಯ(nodep, valuep);
    }
    ಶೂನ್ಯ visit(AstCvtUnpackToUnpack* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_ಮೌಲ್ಯ(nodep->fromp())->clone_ವೃಕ್ಷ(false), nodep->dtypep()};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ತಳ್ಳು_back(valuep);
        set_ಮೌಲ್ಯ(nodep, valuep);
    }
    ಶೂನ್ಯ visit(AstConsArrayItem* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstEnumItemRef* nodep) override {
        check_ಗಂಟು_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು* ಧ್ರುವ valuep = nodep->itemp()->valuep();
        ಯದಿ (valuep) {
            iterateAndNextConstNull(valuep);
            ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
            new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(valuep));
        } ಅನ್ಯಥಾ {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Uniop* nodep) override {
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        nodep->ಸಂಖ್ಯೆ_Operate(newConst(nodep)->ಅಂಕೆ(), fetchConst(nodep->lhsp())->ಅಂಕೆ());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    ಶೂನ್ಯ aggregateEqRecurse(V3_ಸಂಖ್ಯೆ& resultr, Ast_ಗಂಟು* nodep, bool caseEq, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ lconstp = VN_CAST(lhsp, Const)) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lnum = lconstp->ಅಂಕೆ();
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rnum = VN_AS(rhsp, Const)->ಅಂಕೆ();
            V3_ಸಂಖ್ಯೆ eq{nodep, 1};
            ಯದಿ (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } ಅನ್ಯಥಾ ಯದಿ (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } ಅನ್ಯಥಾ ಯದಿ (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } ಅನ್ಯಥಾ {
                eq.opEq(lnum, rnum);
            }
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ prev = resultr;
            resultr.opLogAnd(prev, eq);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstConsArray* ಧ್ರುವ lconsp = VN_CAST(lhsp, ConsArray)) {
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            ಪ್ರತಿ (ಧ್ರುವ AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    ಶೂನ್ಯ handleAggregateCompare(Ast_ಗಂಟು_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ಧ್ರುವ bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ಧ್ರುವ bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_ಸಂಖ್ಯೆ& resultr = newConst(nodep)->ಅಂಕೆ();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_ಮೌಲ್ಯ(nodep->lhsp()),
                           fetch_ಮೌಲ್ಯ(nodep->rhsp()));
        ಯದಿ (neq) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Biop* nodep) override {
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        ಯದಿ (!VN_IS(fetch_ಮೌಲ್ಯ(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstConst* ಧ್ರುವ valuep = newConst(nodep);
        nodep->ಸಂಖ್ಯೆ_Operate(newConst(nodep)->ಅಂಕೆ(), fetchConst(nodep->lhsp())->ಅಂಕೆ(),
                             fetchConst(nodep->rhsp())->ಅಂಕೆ());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        ಯದಿ (!m_params && VN_IS(nodep, Sel) && valuep->ಅಂಕೆ().isAnyX()) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ{valuep, valuep->width(), valuep->ಅಂಕೆ()};
            valuep->ಅಂಕೆ().opBitsOne(ಅಂಕೆ);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Triop* nodep) override {
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        nodep->ಸಂಖ್ಯೆ_Operate(newConst(nodep)->ಅಂಕೆ(), fetchConst(nodep->lhsp())->ಅಂಕೆ(),
                             fetchConst(nodep->rhsp())->ಅಂಕೆ(), fetchConst(nodep->thsp())->ಅಂಕೆ());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Quadop* nodep) override {
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        nodep->ಸಂಖ್ಯೆ_Operate(newConst(nodep)->ಅಂಕೆ(), fetchConst(nodep->lhsp())->ಅಂಕೆ(),
                             fetchConst(nodep->rhsp())->ಅಂಕೆ(), fetchConst(nodep->thsp())->ಅಂಕೆ(),
                             fetchConst(nodep->fhsp())->ಅಂಕೆ());
    }
    ಶೂನ್ಯ visit(AstLogAnd* nodep) override {
        // Need to short circuit
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಅನ್ಯಥಾ {
            iterateConst(nodep->lhsp());
            ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (fetchConst(nodep->lhsp())->ಅಂಕೆ().isNeqZero()) {
                iterateConst(nodep->rhsp());
                ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->rhsp()));
            } ಅನ್ಯಥಾ {
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->lhsp()));  // a zero
            }
        }
    }
    ಶೂನ್ಯ visit(AstLogOr* nodep) override {
        // Need to short circuit
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಅನ್ಯಥಾ {
            iterateConst(nodep->lhsp());
            ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (fetchConst(nodep->lhsp())->ಅಂಕೆ().isNeqZero()) {
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->lhsp()));  // a one
            } ಅನ್ಯಥಾ {
                iterateConst(nodep->rhsp());
                ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->rhsp()));
            }
        }
    }
    ಶೂನ್ಯ visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಅನ್ಯಥಾ {
            iterateConst(nodep->lhsp());
            ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (fetchConst(nodep->lhsp())->ಅಂಕೆ().isEqZero()) {
                ಧ್ರುವ AstConst cnst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 1, 1};  // a one
                new_ಮೌಲ್ಯ(nodep, &cnst);  // a one
            } ಅನ್ಯಥಾ {
                iterateConst(nodep->rhsp());
                ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->rhsp()));
            }
        }
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಅನ್ಯಥಾ {
            iterateConst(nodep->condp());
            ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (fetchConst(nodep->condp())->ಅಂಕೆ().isNeqZero()) {
                iterateConst(nodep->thenp());
                ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->thenp()));
            } ಅನ್ಯಥಾ {
                iterateConst(nodep->elsep());
                ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->elsep()));
            }
        }
    }

    ಶೂನ್ಯ handleAssignElement(Ast_ಗಂಟು_Assign* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೌಲ್ಯ_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> selps;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp = lhsp;
        ಯಾವತ್ (true) {
            ಯದಿ (AstArraySel* ಧ್ರುವ selp = VN_CAST(fromp, ArraySel)) {
                check_ಗಂಟು_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.ತಳ್ಳು_back(selp);
                fromp = selp->fromp();
            } ಅನ್ಯಥಾ ಯದಿ (AstStructSel* ಧ್ರುವ selp = VN_CAST(fromp, StructSel)) {
                check_ಗಂಟು_Info(selp);
                selps.ತಳ್ಳು_back(selp);
                fromp = selp->fromp();
            } ಅನ್ಯಥಾ {
                ಭಙ್ಗ;
            }
        }
        AstVarRef* ಧ್ರುವ varrefp = VN_CAST(fromp, VarRef);
        ಯದಿ (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು* ಧ್ರುವ vscp = varOrScope(varrefp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rootp = fetch_Out_ಮೌಲ್ಯ_Null(vscp);
        ಯದಿ (!rootp) rootp = fetch_ಮೌಲ್ಯ_Null(vscp);
        ಯದಿ (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemp = rootp;
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ selp : vlstd::reverse_view(selps)) {
            ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ಧ್ರುವ consp = VN_AS(itemp, ConsArray);
                ಧ್ರುವ uint32_t index = fetchConst(aselp->bitp())->toUInt();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                ಯದಿ (index >= static_cast<uint32_t>(elements)) {
                    ಪ್ರತ್ಯಾಗಮ;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                ಯದಿ (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_ವೃಕ್ಷ(false);
                    consp->valuep(index, itemp);
                }
            } ಅನ್ಯಥಾ {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = newTrackedClone(fetch_ಮೌಲ್ಯ(ಮೌಲ್ಯ_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ತಳ್ಳು_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_ಮೌಲ್ಯ(nodep, vscp, rootp);
    }
    ಶೂನ್ಯ handleAssignSel(Ast_ಗಂಟು_Assign* nodep, AstSel* selp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೌಲ್ಯ_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_ಸಂಖ್ಯೆ lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_ಗಂಟು* ಧ್ರುವ vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        ಯದಿ (AstConst* ಧ್ರುವ vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } ಅನ್ಯಥಾ ಯದಿ (AstConst* ಧ್ರುವ vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } ಅನ್ಯಥಾ {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                     varrefp->varp()->widthMin(), 0};
            ಯದಿ (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->ಅಂಕೆ().setAllBits0();
            } ಅನ್ಯಥಾ {
                outconstp->ಅಂಕೆ().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->ಅಂಕೆ().opSelInto(fetchConst(ಮೌಲ್ಯ_Fromp)->ಅಂಕೆ(), lsb, selp->widthConst());
        assign_Out_ಮೌಲ್ಯ(nodep, vscp, outconstp);
    }
    ಶೂನ್ಯ handleAssignSelRecurse(Ast_ಗಂಟು_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_ಸಂಖ್ಯೆ& lsbRef, ಪೂರ್ಣಾಙ್ಕ depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_ಗಂಟು_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->ಅಂಕೆ();
            ಪ್ರತ್ಯಾಗಮ;  // And presumably still optimizable()
        }
        ಯದಿ (AstSel* ಧ್ರುವ subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_ಸಂಖ್ಯೆ sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            ಯದಿ (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->ಅಂಕೆ());
            }
        } ಅನ್ಯಥಾ {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    ಶೂನ್ಯ handleAssignRecurse(Ast_ಗಂಟು_Assign* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೌಲ್ಯ_Fromp) {
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            ಯದಿ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ಪ್ರತ್ಯಾಗಮ;
            }
            handleAssignElement(nodep, lhsp, ಮೌಲ್ಯ_Fromp);
        } ಅನ್ಯಥಾ ಯದಿ (AstConcat* ಧ್ರುವ selp = VN_CAST(lhsp, Concat)) {
            check_ಗಂಟು_Info(selp);
            ಯದಿ (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Split value into left and right concat values
            ಯದಿ (!m_checkOnly) {
                {
                    AstConst* ಧ್ರುವ outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                       selp->lhsp()->width(), 0};
                    outconstp->ಅಂಕೆ().opSel(fetchConst(ಮೌಲ್ಯ_Fromp)->ಅಂಕೆ(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_ಮೌಲ್ಯ(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ವೃಕ್ಷ(), outconstp);
                }
                {
                    AstConst* ಧ್ರುವ outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->ಅಂಕೆ().opSel(fetchConst(ಮೌಲ್ಯ_Fromp)->ಅಂಕೆ(),
                                           selp->rhsp()->width() - 1, 0);
                    new_ಮೌಲ್ಯ(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ವೃಕ್ಷ(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } ಅನ್ಯಥಾ ಯದಿ (AstReplicate* ಧ್ರುವ selp = VN_CAST(lhsp, Replicate)) {
            check_ಗಂಟು_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* ಧ್ರುವ countp = VN_CAST(selp->countp(), Const);
            ಯದಿ (!countp || !countp->ಅಂಕೆ().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                ಪ್ರತ್ಯಾಗಮ;
            }
            handleAssignRecurse(nodep, selp->srcp(), ಮೌಲ್ಯ_Fromp);
        } ಅನ್ಯಥಾ ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(lhsp, Sel)) {
            ಯದಿ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ಪ್ರತ್ಯಾಗಮ;
            }
            handleAssignSel(nodep, selp, ಮೌಲ್ಯ_Fromp);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(lhsp, VarRef)) {
            ಯದಿ (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } ಅನ್ಯಥಾ {
                Ast_ಗಂಟು* ಧ್ರುವ vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_ಮೌಲ್ಯ(nodep, vscp, fetch_ಮೌಲ್ಯ(ಮೌಲ್ಯ_Fromp));
            }
        } ಅನ್ಯಥಾ {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        ಯದಿ (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, AssignDly)) {
            ಯದಿ (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        ಯದಿ (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            ಪ್ರತ್ಯಾಗಮ;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (ಧ್ರುವ AstConsArray* ಧ್ರುವ consp = VN_CAST(fetch_ಮೌಲ್ಯ_Null(nodep->fromp()), ConsArray)) {
            ಧ್ರುವ AstConst* ಧ್ರುವ indexp = fetchConst(nodep->bitp());
            ಧ್ರುವ uint32_t offset = indexp->ಅಂಕೆ().toUInt();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ itemp = consp->valuep(offset);
            ಯದಿ (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } ಅನ್ಯಥಾ {
                set_ಮೌಲ್ಯ(nodep, itemp);
            }
        } ಅನ್ಯಥಾ {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        ಯದಿ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        AstConsStruct* ಧ್ರುವ consp = VN_AS(fetch_ಮೌಲ್ಯ(nodep->fromp()), ConsStruct);
        set_ಮೌಲ್ಯ(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    ಶೂನ್ಯ visit(AstSliceSel* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        // Fetch the base constant array
        ಯದಿ (ಧ್ರುವ AstConsArray* ಧ್ರುವ consp = VN_CAST(fetch_ಮೌಲ್ಯ_Null(nodep->fromp()), ConsArray)) {
            ಧ್ರುವ VNumRange& sliceRange = nodep->declRange();
            ಧ್ರುವ uint32_t sliceElements = sliceRange.elements();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ಧ್ರುವ newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            ಪ್ರತಿ (uint32_t idx = 0; idx < sliceElements; ++idx) {
                ಧ್ರುವ uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_ವೃಕ್ಷ(false));
            }
            // Assign the new constant array and track it for later deletion
            set_ಮೌಲ್ಯ(nodep, newConsp);
            m_reclaimValuesp.ತಳ್ಳು_back(newConsp);
        } ಅನ್ಯಥಾ {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    ಶೂನ್ಯ visit(AstBegin* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstCase* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(5, "   CASE " << nodep);
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಅನ್ಯಥಾ ಯದಿ (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            ಪ್ರತಿ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                ಯದಿ (!itemp->isDefault()) {
                    ಪ್ರತಿ (Ast_ಗಂಟು* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        ಯದಿ (hit) ಭಙ್ಗ;
                        iterateAndNextConstNull(ep);
                        ಯದಿ (optimizable()) {
                            V3_ಸಂಖ್ಯೆ match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->ಅಂಕೆ(), fetchConst(ep)->ಅಂಕೆ());
                            ಯದಿ (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            ಪ್ರತಿ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                ಯದಿ (hit) ಭಙ್ಗ;
                ಯದಿ (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstComment*) override {}

    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        check_ಗಂಟು_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
        iterateAndNextConstNull(nodep->resultp());
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->resultp()));
    }

    ಶೂನ್ಯ visit(AstJumpBlock* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildrenConst(nodep);
        ಯದಿ (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ಶೂನ್ಯ visit(AstJumpGo* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        ಯದಿ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (m_checkOnly) {
            iterateChildrenConst(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;

        size_t iter_ಎಣಿಕೆ = 0;
        ಧ್ರುವ size_t iterLimit = v3Global.opt.unrollLimit();
        ಯಾವತ್ (true) {
            ಯದಿ (iter_ಎಣಿಕೆ > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                ಭಙ್ಗ;
            }
            ++iter_ಎಣಿಕೆ;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            ಯದಿ (jumpingOver()) ಭಙ್ಗ;
        }

        ಯದಿ (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        check_ಗಂಟು_Info(nodep);
        iterateConst(nodep->condp());
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (fetchConst(nodep->condp())->ಅಂಕೆ().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    ಶೂನ್ಯ visit(AstStop* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_ಗಂಟು_Info(nodep);
    }
    ಶೂನ್ಯ visit(AstFuncRef* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_ಗಂಟು_Info(nodep);
        ಯದಿ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        Ast_ಗಂಟು_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        ಯದಿ (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        ಯದಿ (funcp->recursive()) {
            ಯದಿ (m_recurse_ಎಣಿಕೆ >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                ಪ್ರತ್ಯಾಗಮ;
            }
            ++m_recurse_ಎಣಿಕೆ;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_ಗಂಟು*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> old_ಮೌಲ್ಯಗಳು;

        ಯದಿ (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_ಮೌಲ್ಯಗಳು](AstVar* varp) {
                ಯದಿ (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = fetch_ಮೌಲ್ಯ_Null(varp)) {
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nvaluep = newTrackedClone(valuep);
                        old_ಮೌಲ್ಯಗಳು.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_ಮೌಲ್ಯಗಳು](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
                ಯದಿ (VN_IS(exprp, Const)) ಪ್ರತ್ಯಾಗಮ;  // Speed up as won't change
                ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = fetch_ಮೌಲ್ಯ_Null(exprp)) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nvaluep = newTrackedClone(valuep);
                    old_ಮೌಲ್ಯಗಳು.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> port_ಮೌಲ್ಯಗಳು;
        ಪ್ರತಿ (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* ಧ್ರುವ portp = it->first;
            Ast_ಗಂಟು* ಧ್ರುವ pinp = it->second->exprp();
            ಯದಿ (pinp) {  // Else too few arguments in function call - ignore it
                ಯದಿ (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    ಪ್ರತ್ಯಾಗಮ;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_ಮೌಲ್ಯಗಳು.emplace_back(std::make_pair(portp, newTrackedClone(fetch_ಮೌಲ್ಯ(pinp))));
            }
        }
        // Apply value to the function
        ಯದಿ (!m_checkOnly && optimizable())
            ಪ್ರತಿ (ಧ್ರುವ auto& it : port_ಮೌಲ್ಯಗಳು) {
                ಯದಿ (!m_checkOnly && optimizable()) new_ಮೌಲ್ಯ(it.first, it.second);
            }
        Sim_ರಾಶಿ_ಗಂಟು ರಾಶಿ_ಗಂಟು{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ರಾಶಿ.ತಳ್ಳು_back(&ರಾಶಿ_ಗಂಟು);
        ಯದಿ (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                ಯದಿ (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_ರಾಶಿ.ತೆಗೆ_back();
        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* returnp = nullptr;
        ಯದಿ (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_ಮೌಲ್ಯ(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        ಪ್ರತಿ (ಧ್ರುವ auto& it : old_ಮೌಲ್ಯಗಳು) {
            ಯದಿ (it.second) new_ಮೌಲ್ಯ(it.first, it.second);
        }
        ಯದಿ (returnp) new_ಮೌಲ್ಯ(nodep, returnp);
        ಯದಿ (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_ಎಣಿಕೆ > 0, nodep, "recurse underflow");
            --m_recurse_ಎಣಿಕೆ;
        }
    }

    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
    }

    ಶೂನ್ಯ visit(AstScopeName* /*nodep*/) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        // Ignore
    }

    ಶೂನ್ಯ visit(AstSFormatArg* nodep) override {
        check_ಗಂಟು_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;

        Ast_ಗಂಟು* nextArgp = nodep->exprsp();
        string result;
        ಧ್ರುವ string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        ಪ್ರತಿ (; pos != format.cend(); ++pos) {
            ಯದಿ (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } ಅನ್ಯಥಾ ಯದಿ (!inPct) {  // Normal text
                result += *pos;
            } ಅನ್ಯಥಾ {  // Format character
                ಯದಿ (std::isdigit(pos[0])) {
                    width += pos[0];
                    ಅನುವರ್ತ;
                }

                inPct = false;

                ಯದಿ (V3_ಸಂಖ್ಯೆ::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_ಗಂಟು* ಧ್ರುವ argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* ಧ್ರುವ fargp = VN_CAST(argp, SFormatArg);
                    Ast_ಗಂಟು* ಧ್ರುವ subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ಧ್ರುವ constp = fetchConstNull(subargp);
                    ಧ್ರುವ VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    ಯದಿ (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        ಭಙ್ಗ;
                    }
                    ಧ್ರುವ string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->ಅಂಕೆ().displayedEnum(fargp, pformat)
                                  : constp->ಅಂಕೆ().displayed(nodep, pformat, formatAttr);
                } ಅನ್ಯಥಾ {
                    ವಿಕಲ್ಪ (std::tolower(pos[0])) {
                    ಪಕ್ಷ '%': result += "%"; ಭಙ್ಗ;
                    ಪಕ್ಷ 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        ಭಙ್ಗ;
                    ಸ್ವಭಾವ: clearOptimizable(nodep, "Unknown $display-like format code."); ಭಙ್ಗ;
                    }
                }
            }
        }

        AstConst* ಧ್ರುವ resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_ಮೌಲ್ಯ(nodep, resultConstp);
        m_reclaimValuesp.ತಳ್ಳು_back(resultConstp);
    }

    ಶೂನ್ಯ visit(AstDisplay* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_ಗಂಟು_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        ಯದಿ (m_params) {
            ಧ್ರುವ AstConst* ಧ್ರುವ textp = fetchConst(nodep->fmtp());
            ವಿಕಲ್ಪ (nodep->displayType()) {
            ಪಕ್ಷ VDisplayType::DT_DISPLAY:  // FALLTHRU
            ಪಕ್ಷ VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); ಭಙ್ಗ;
            ಪಕ್ಷ VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); ಭಙ್ಗ;
            ಪಕ್ಷ VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); ಭಙ್ಗ;
            ಪಕ್ಷ VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); ಭಙ್ಗ;
            ಪಕ್ಷ VDisplayType::DT_WRITE:  // FALLTHRU
            ಸ್ವಭಾವ: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    ಶೂನ್ಯ visit(AstToStringN* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಯದಿ (m_checkOnly || !optimizable()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ std::string result = toStringRecurse(fetch_ಮೌಲ್ಯ(nodep->lhsp()));
        ಯದಿ (!optimizable()) ಪ್ರತ್ಯಾಗಮ;
        AstConst* ಧ್ರುವ resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_ಮೌಲ್ಯ(nodep, resultConstp);
        m_reclaimValuesp.ತಳ್ಳು_back(resultConstp);
    }

    ಶೂನ್ಯ visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_ಗಂಟು_Type(nodep);
    }
    ಶೂನ್ಯ visit(AstGetInitialRandomSeed* nodep) override { known_Bad_ಗಂಟು_Type(nodep); }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override { known_Bad_ಗಂಟು_Type(nodep); }
    ಶೂನ್ಯ visit(AstSampled* nodep) override { known_Bad_ಗಂಟು_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (jumpingOver()) ಪ್ರತ್ಯಾಗಮ;
        bad_ಗಂಟು_Type(nodep);
    }

    // MEMBERS - called by constructor
    ಶೂನ್ಯ setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    ಶೂನ್ಯ mainGuts(Ast_ಗಂಟು* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    ಶೂನ್ಯ clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_ಎಣಿಕೆ = 0;
        m_data_ಎಣಿಕೆ = 0;
        m_jumptargetp = nullptr;

        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
        m_varAux.clear();
        ++m_constGeneration;
    }
    ಶೂನ್ಯ mainTableCheck(Ast_ಗಂಟು* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ಶೂನ್ಯ mainTableEmulate(Ast_ಗಂಟು* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ಶೂನ್ಯ mainParamEmulate(Ast_ಗಂಟು* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_ಗಂಟು*> unusedRootps;
        ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ nodep : m_reclaimValuesp) {
            ಯದಿ (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
};

#endif  // Guard
