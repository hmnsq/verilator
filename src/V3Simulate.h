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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3SIMULATE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3SIMULATE_H_

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
    AstFuncRef* ಸ್ಥಿರಾಂಕ m_funcp;
    V3TaskConnects* ಸ್ಥಿರಾಂಕ m_tconnects;
    // CONSTRUCTORS
    Sim_ರಾಶಿ_ಗಂಟು(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ರಾಶಿ_ಗಂಟು() = ಪೂರ್ವನಿಯೋಜಿತ;
};

class SimulateVisitor VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

ಖಾಸಗಿ:
    // CONSTANTS
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ CALL_ರಾಶಿ_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    ಎಣಿಕೆ VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    ರಚನೆ AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊರಹೋಗು_Valuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nullptr;
    };

    AstUser1Allocator<Ast_ಗಂಟು, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    ರಚನೆ ConstAllocator final {
        ಗಾತ್ರ_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        ಗಾತ್ರ_t m_ಮುಂದಿನ_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(ಗಾತ್ರ_t currentGeneration, Ast_ಗಂಟು* nodep) {
            ಆದರೆ (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_ಮುಂದಿನ_Free = 0;
            }
            UASSERT_OBJ(m_ಮುಂದಿನ_Free <= m_constps.ಗಾತ್ರ_2(), nodep, "Should only allocate at end");
            ಆದರೆ (m_ಮುಂದಿನ_Free == m_constps.ಗಾತ್ರ_2()) {
                m_constps.ತಳ್ಳು_ಹಿಂಭಾಗ(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ಸ್ಥಿರಾಂಕ constp = m_constps[m_ಮುಂದಿನ_Free++];
            constp->ಅಂಕೆ().nodep(nodep);
            ಹಿಂತಿರುಗಿ constp;
        }

        ~ConstAllocator() {
            ಗಾಗಿ (AstConst* ಸ್ಥಿರಾಂಕ constp : m_constps) VL_DO_DANGLING(delete constp, constp);
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
    bool m_any_ನಿಯೋಜಿಸು_Dly;  ///< True if found a delayed assignment
    bool m_any_ನಿಯೋಜಿಸು_Comb;  ///< True if found a non-delayed assignment
    bool m_ಒಳ_Dly_ನಿಯೋಜಿಸು;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    ಪೂರ್ಣಾಂಕ m_instr_ಎಣಿಕೆ;  ///< Number of nodes
    ಪೂರ್ಣಾಂಕ m_ದತ್ತಾಂಶ_ಎಣಿಕೆ;  ///< Bytes of data
    ಪೂರ್ಣಾಂಕ m_recurse_ಎಣಿಕೆ = 0;  // Now deep in current recursion
    Ast_ಗಂಟು* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType*, ConstAllocator> m_constps;
    ಗಾತ್ರ_t m_constGeneration = 0;
    std::ಸದಿಶ<Sim_ರಾಶಿ_ಗಂಟು*> m_call_ರಾಶಿ;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::ಸದಿಶ<Ast_ಗಂಟು*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_ಸಂಖ್ಯೆ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ* nump, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstRefDType* ಸ್ಥಿರಾಂಕ refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ stp = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
            ಆದರೆ (stp->packed()) {
                std::ostringstream ಹೊರಹೋಗು;
                ಹೊರಹೋಗು << "'{";
                ಗಾಗಿ (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = itemp->ಅಗಲ();
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = itemp->lsb();
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = lsb + ಅಗಲ - 1;
                    V3_ಸಂಖ್ಯೆ ಕ್ಷೇತ್ರ_ಅಂಕೆ{nump, ಅಗಲ};
                    ಕ್ಷೇತ್ರ_ಅಂಕೆ.opSel(*nump, msb, lsb);
                    ಹೊರಹೋಗು << itemp->name() << ": ";
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಮಗು_Typep = itemp->subDTypep()) {
                        ಹೊರಹೋಗು << pretty_ಸಂಖ್ಯೆ(&ಕ್ಷೇತ್ರ_ಅಂಕೆ, ಮಗು_Typep);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಹೊರಹೋಗು << ಕ್ಷೇತ್ರ_ಅಂಕೆ;
                    }
                    ಆದರೆ (itemp->nextp()) ಹೊರಹೋಗು << ", ";
                }
                ಹೊರಹೋಗು << "}";
                ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು.str();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಮಗು_Typep = arrayp->subDTypep()) {
                std::ostringstream ಹೊರಹೋಗು;
                ಹೊರಹೋಗು << "[";
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ arrayElements = arrayp->elementsConst();
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ element = 0; element < arrayElements; ++element) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = ಮಗು_Typep->ಅಗಲ();
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = ಅಗಲ * element;
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ msb = lsb + ಅಗಲ - 1;
                    V3_ಸಂಖ್ಯೆ ಕ್ಷೇತ್ರ_ಅಂಕೆ{nump, ಅಗಲ};
                    ಕ್ಷೇತ್ರ_ಅಂಕೆ.opSel(*nump, msb, lsb);
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ arrayElem = arrayp->lo() + element;
                    ಹೊರಹೋಗು << arrayElem << " = " << pretty_ಸಂಖ್ಯೆ(&ಕ್ಷೇತ್ರ_ಅಂಕೆ, ಮಗು_Typep);
                    ಆದರೆ (element < arrayElements - 1) ಹೊರಹೋಗು << ", ";
                }
                ಹೊರಹೋಗು << "]";
                ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು.str();
            }
        }
        ಹಿಂತಿರುಗಿ nump->ascii();
    }

    // Checking METHODS
ಸಾರ್ವಜನಿಕ:
    /// Call other-this function on all new *non-constant* var references
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ varRefCb(AstVarRef* nodep) {}

    ಶೂನ್ಯ clearOptimizable(Ast_ಗಂಟು* nodep /*null ok*/, ಸ್ಥಿರಾಂಕ string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        ಆದರೆ (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            ಆದರೆ (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_ಪೂರ್ವಪ್ರತ್ಯಯ("Clear optimizable: " << why);
                ಆದರೆ (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ರಾಶಿ;
            ಪೂರ್ಣಾಂಕ n = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& callstack : vlstd::ಹಿಮ್ಮುಖ_view(m_call_ರಾಶಿ)) {
                ಸ್ಥಿರಾಂಕ AstFuncRef* ಸ್ಥಿರಾಂಕ funcp = callstack->m_funcp;
                ರಾಶಿ << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                ಗಾಗಿ (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->ಕೊನೆ(); ++conIt) {
                    ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = conIt->ಮೊದಲ;
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinp = conIt->ಎರಡನೇ->exprp();
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = pinp->dtypep();
                    ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ valp = fetchConstNull(pinp)) {
                        ರಾಶಿ << "\n           " << portp->prettyName() << " = "
                              << pretty_ಸಂಖ್ಯೆ(&valp->ಅಂಕೆ(), dtypep);
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = fetch_ಮೌಲ್ಯ_Null(pinp)) {
                        ರಾಶಿ << "\n           " << portp->prettyName() << " = "
                              << ಗಮ್ಯ_String_Recurse(valuep);
                    }
                }
                ಆದರೆ (++n > CALL_ರಾಶಿ_MAX) {
                    ರಾಶಿ << "\n           ... stack truncated";
                    ನಿಲ್ಲಿಸು;
                }
            }
            m_whyNotOptimizable += ರಾಶಿ.str();
        }
    }
    bool optimizable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_whyNotNodep == nullptr; }
    string why_Not_ಸಂದೇಶ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_whyNotOptimizable; }
    Ast_ಗಂಟು* whyNotNodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_whyNotNodep; }

    bool is_ನಿಯೋಜಿಸು_Dly() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_any_ನಿಯೋಜಿಸು_Dly; }
    bool isImpure() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isImpure; }
    bool isCoverage() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isCoverage; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_instr_ಎಣಿಕೆ; }
    ಪೂರ್ಣಾಂಕ ದತ್ತಾಂಶ_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ_ಎಣಿಕೆ; }

    // Simulation METHODS
ಖಾಸಗಿ:
    AstConst* allocConst(Ast_ಗಂಟು* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        ಹಿಂತಿರುಗಿ m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ new_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->ಅಂಕೆ().op_ನಿಯೋಜಿಸು(constp->ಅಂಕೆ());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (fetch_ಮೌಲ್ಯ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            ಗಣ_ಮೌಲ್ಯ(nodep, newTrackedClone(const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(valuep)));
        }
    }
    ಶೂನ್ಯ new_ಹೊರಹೋಗು_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(valuep, Const)) {
            new_ಹೊರಹೋಗು_Const(nodep)->ಅಂಕೆ().op_ನಿಯೋಜಿಸು(constp->ಅಂಕೆ());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (fetch_ಹೊರಹೋಗು_ಮೌಲ್ಯ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            ಗಣ_ಹೊರಹೋಗು_ಮೌಲ್ಯ(nodep, newTrackedClone(const_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(valuep)));
        }
    }

ಖಾಸಗಿ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newTrackedClone(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = nodep->clone_ವೃಕ್ಷ(false);
        m_reclaimValuesp.ತಳ್ಳು_ಹಿಂಭಾಗ(newp);
        ಹಿಂತಿರುಗಿ newp;
    }
    AstConst* newConst(Ast_ಗಂಟು* nodep) {
        // Set a constant value for this node
        ಆದರೆ (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ಸ್ಥಿರಾಂಕ constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            ಹಿಂತಿರುಗಿ constp;
        }
        ಹಿಂತಿರುಗಿ fetchConst(nodep);
    }
    AstConst* new_ಹೊರಹೋಗು_Const(Ast_ಗಂಟು* nodep) {
        // Set a var-output constant value for this node
        ಆದರೆ (!VN_IS(m_varAux(nodep).ಹೊರಹೋಗು_Valuep, Const)) {
            AstConst* ಸ್ಥಿರಾಂಕ constp = allocConst(nodep);
            m_varAux(nodep).ಹೊರಹೋಗು_Valuep = constp;
            ಹಿಂತಿರುಗಿ constp;
        }
        ಹಿಂತಿರುಗಿ fetch_ಹೊರಹೋಗು_Const(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fetch_ಮೌಲ್ಯ_Null(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ m_varAux(nodep).valuep; }

ಖಾಸಗಿ:
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fetch_ಹೊರಹೋಗು_ಮೌಲ್ಯ_Null(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ m_varAux(nodep).ಹೊರಹೋಗು_Valuep; }
    AstConst* fetchConstNull(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ VN_CAST(fetch_ಮೌಲ್ಯ_Null(nodep), Const); }
    AstConst* fetch_ಹೊರಹೋಗು_Const_Null(Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ VN_CAST(fetch_ಹೊರಹೋಗು_ಮೌಲ್ಯ_Null(nodep), Const);
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fetch_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = fetch_ಮೌಲ್ಯ_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        ಹಿಂತಿರುಗಿ valuep;
    }
    AstConst* fetchConst(Ast_ಗಂಟು* nodep) {
        AstConst* ಸ್ಥಿರಾಂಕ constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        ಹಿಂತಿರುಗಿ constp;
    }
    AstConst* fetch_ಹೊರಹೋಗು_Const(Ast_ಗಂಟು* nodep) {
        AstConst* ಸ್ಥಿರಾಂಕ constp = fetch_ಹೊರಹೋಗು_Const_Null(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        ಹಿಂತಿರುಗಿ constp;
    }

ಸಾರ್ವಜನಿಕ:
    V3_ಸಂಖ್ಯೆ* fetch_ಸಂಖ್ಯೆ_Null(Ast_ಗಂಟು* nodep) {
        AstConst* ಸ್ಥಿರಾಂಕ constp = fetchConstNull(nodep);
        ಆದರೆ (constp) ಹಿಂತಿರುಗಿ &constp->ಅಂಕೆ();
        ಹಿಂತಿರುಗಿ nullptr;
    }
    V3_ಸಂಖ್ಯೆ* fetch_ಹೊರಹೋಗು_ಸಂಖ್ಯೆ_Null(Ast_ಗಂಟು* nodep) {
        AstConst* ಸ್ಥಿರಾಂಕ constp = fetch_ಹೊರಹೋಗು_Const_Null(nodep);
        ಆದರೆ (constp) ಹಿಂತಿರುಗಿ &constp->ಅಂಕೆ();
        ಹಿಂತಿರುಗಿ nullptr;
    }

ಖಾಸಗಿ:
    ಶೂನ್ಯ ಗಣ_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    ಶೂನ್ಯ ಗಣ_ಹೊರಹೋಗು_ಮೌಲ್ಯ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).ಹೊರಹೋಗು_Valuep = valuep;
    }

    ಶೂನ್ಯ check_ಗಂಟು_Info(Ast_ಗಂಟು* nodep, bool ignorePredict = false) {
        ಆದರೆ (m_checkOnly) {
            m_instr_ಎಣಿಕೆ += nodep->instr_ಎಣಿಕೆ();
            m_ದತ್ತಾಂಶ_ಎಣಿಕೆ += nodep->ಅಗಲ();
        }
        ಆದರೆ (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        ಆದರೆ (!nodep->is_ಶುದ್ಧ()) m_isImpure = true;
    }

    ಶೂನ್ಯ ತಿಳಿದ_Bad_ಗಂಟು_Type(Ast_ಗಂಟು* nodep) {
        // Call for node types we know we can't handle
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    ಶೂನ್ಯ bad_ಗಂಟು_Type(Ast_ಗಂಟು* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
            ಸ್ಥಿರ std::ಗಣ<VNType> s_typePrinted;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = s_typePrinted.emplace(nodep->type());
            ಆದರೆ (pair.ಎರಡನೇ)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#ಆದರೆ_ಅಂತ್ಯ
        }
    }

    Ast_ಗಂಟು* varOrScope(AstVarRef* nodep) ಸ್ಥಿರಾಂಕ {
        Ast_ಗಂಟು* vscp;
        ಆದರೆ (m_scoped) {
            vscp = nodep->varScopep();
        } ಇಲ್ಲದಿದ್ದರೆ {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        ಹಿಂತಿರುಗಿ vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_jumptargetp; }
    ಶೂನ್ಯ ನಿಯೋಜಿಸು_ಹೊರಹೋಗು_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep, Ast_ಗಂಟು* vscp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಆದರೆ (VN_IS(nodep, ನಿಯೋಜಿಸು_Dly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_ಹೊರಹೋಗು_ಮೌಲ್ಯ(vscp, valuep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            new_ಮೌಲ್ಯ(vscp, valuep);
            ಗಣ_ಹೊರಹೋಗು_ಮೌಲ್ಯ(vscp, fetch_ಮೌಲ್ಯ(vscp));
        }
    }

    string ಗಮ್ಯ_String_Recurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        // Return string representation of a value, or clearOptimizable
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ avaluep = VN_CAST(valuep, Const)) {
            ಹಿಂತಿರುಗಿ "'h"s + avaluep->ಅಂಕೆ().displayed(valuep, "%0x");
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ avaluep = VN_CAST(valuep, ConsArray)) {
            string ಫಲಿತಾಂಶ = "'{";
            string comma;
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ n = 0; n < dtypep->elementsConst(); ++n) {
                ಫಲಿತಾಂಶ += comma + ಗಮ್ಯ_String_Recurse(avaluep->valuep(n));
                comma = ", ";
            }
            ಫಲಿತಾಂಶ += "}";
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsStruct* ಸ್ಥಿರಾಂಕ avaluep = VN_CAST(valuep, ConsStruct)) {
            string ಫಲಿತಾಂಶ = "'{";
            string comma;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Cons_Struct_ಅಂಶ* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_ಅಂಶ)) {
                ಫಲಿತಾಂಶ
                    += comma + itemp->memberp()->name() + ":" + ಗಮ್ಯ_String_Recurse(itemp->valuep());
                comma = ", ";
            }
            ಫಲಿತಾಂಶ += "}";
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        ಹಿಂತಿರುಗಿ "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newInitValuep(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elementp = newInitValuep(nodep, arrayp->subDTypep());
            ಹಿಂತಿರುಗಿ elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_CAST(dtypep, StructDType);
        ಆದರೆ (structp && !structp->packed()) {
            AstConsStruct* ಸ್ಥಿರಾಂಕ newp = new AstConsStruct{nodep->fileline(), structp};
            ಗಾಗಿ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ memberValuep = newInitValuep(nodep, memberp->subDTypep());
                ಆದರೆ (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_ವೃಕ್ಷ(), newp);
                    ಹಿಂತಿರುಗಿ nullptr;
                }
                newp->ಕೂಡಿಸು_Itemsp(new Ast_Cons_Struct_ಅಂಶ{nodep->fileline(), memberp, memberValuep});
            }
            ಹಿಂತಿರುಗಿ newp;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp()) {
            AstConst* ಸ್ಥಿರಾಂಕ constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, basicp->ಅಗಲ_Min(), 0};
            ಆದರೆ (basicp->isZeroInit()) {
                constp->ಅಂಕೆ() = V3_ಸಂಖ್ಯೆ{nodep, basicp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                constp->ಅಂಕೆ().ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
            }
            ಹಿಂತಿರುಗಿ constp;
        }
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಶೂನ್ಯ initVar(AstVar* nodep) {
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_ಮೌಲ್ಯ(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_ವೃಕ್ಷ(), valuep);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        ಆದರೆ (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), ಗಂಟು_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        ಆದರೆ (nodep->access().isWriteOrRW()) {
            ಆದರೆ (m_ಒಳ_Dly_ನಿಯೋಜಿಸು) {
                ಆದರೆ (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    ಆದರೆ (m_checkOnly) varRefCb(nodep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {  // nondly asn
                ಆದರೆ (!(m_varAux(vscp).usage & VU_LV)) {
                    ಆದರೆ (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    ಆದರೆ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ಆದರೆ (nodep->access().isReadOrRW()) {
            ಆದರೆ (!(m_varAux(vscp).usage & VU_RV)) {
                ಆದರೆ (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                ಸ್ಥಿರಾಂಕ bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep
                    = varIsConst ? fetch_ಮೌಲ್ಯ_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                ಆದರೆ (varIsConst && valuep) {
                    ಆದರೆ (!m_checkOnly && optimizable()) new_ಮೌಲ್ಯ(vscp, valuep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = fetch_ಮೌಲ್ಯ_Null(vscp);
            ಆದರೆ (!valuep) {
                ಆದರೆ (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            ಗಣ_ಮೌಲ್ಯ(nodep, valuep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_scoped) {
            bad_ಗಂಟು_Type(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->dpiImport()) {
            ಆದರೆ (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        ಆದರೆ (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialAutomaticStmt* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStaticStmt* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStatic* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        UINFO(5, "   IF " << nodep);
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->condp());
            ಆದರೆ (optimizable()) {
                ಆದರೆ (fetchConst(nodep->condp())->ಅಂಕೆ().isNeqZero()) {
                    iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        new_ಮೌಲ್ಯ(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    ಶೂನ್ಯ ಭೇಟಿ(AstConsArray* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        AstConsArray* ಸ್ಥಿರಾಂಕ valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_ಮೌಲ್ಯ(nodep->defaultp())->clone_ವೃಕ್ಷ(false) : nullptr};
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Cons_Array_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_ಅಂಶ)) {
            valuep->valuep(itemp->ಸೂಚ್ಯಂಕ(), fetch_ಮೌಲ್ಯ(itemp->valuep())->clone_ವೃಕ್ಷ(false));
        }
        m_reclaimValuesp.ತಳ್ಳು_ಹಿಂಭಾಗ(valuep);
        ಗಣ_ಮೌಲ್ಯ(nodep, valuep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConsStruct* nodep) override {
        ಆದರೆ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        check_ಗಂಟು_Info(nodep);
        ಗಾಗಿ (Ast_Cons_Struct_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_ಅಂಶ)) {
            iterateConst(itemp->valuep());
        }
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        AstConsStruct* ಸ್ಥಿರಾಂಕ valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        ಗಾಗಿ (Ast_Cons_Struct_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_ಅಂಶ)) {
            valuep->ಕೂಡಿಸು_Itemsp(
                new Ast_Cons_Struct_ಅಂಶ{itemp->fileline(), itemp->memberp(),
                                      fetch_ಮೌಲ್ಯ(itemp->valuep())->clone_ವೃಕ್ಷ(false)});
        }
        m_reclaimValuesp.ತಳ್ಳು_ಹಿಂಭಾಗ(valuep);
        ಗಣ_ಮೌಲ್ಯ(nodep, valuep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cvtp = new Ast_Cvt_Unpack_ಗಮ್ಯ_Unpack{
            nodep->fileline(), fetch_ಮೌಲ್ಯ(nodep->fromp())->clone_ವೃಕ್ಷ(false), nodep->dtypep()};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ತಳ್ಳು_ಹಿಂಭಾಗ(valuep);
        ಗಣ_ಮೌಲ್ಯ(nodep, valuep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Cons_Array_ಅಂಶ* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ_Ref* nodep) override {
        check_ಗಂಟು_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ valuep = nodep->itemp()->valuep();
        ಆದರೆ (valuep) {
            iterate_And_ಮುಂದಿನ_Const_Null(valuep);
            ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
            new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(valuep));
        } ಇಲ್ಲದಿದ್ದರೆ {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Uniop* nodep) override {
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        nodep->ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(newConst(nodep)->ಅಂಕೆ(), fetchConst(nodep->lhsp())->ಅಂಕೆ());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    ಶೂನ್ಯ aggregateEqRecurse(V3_ಸಂಖ್ಯೆ& resultr, Ast_ಗಂಟು* nodep, bool caseEq, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp,
                            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lconstp = VN_CAST(lhsp, Const)) {
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lnum = lconstp->ಅಂಕೆ();
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rnum = VN_AS(rhsp, Const)->ಅಂಕೆ();
            V3_ಸಂಖ್ಯೆ eq{nodep, 1};
            ಆದರೆ (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } ಇಲ್ಲದಿದ್ದರೆ {
                eq.opEq(lnum, rnum);
            }
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ prev = resultr;
            resultr.opLogAnd(prev, eq);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ lconsp = VN_CAST(lhsp, ConsArray)) {
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ Ast_Cons_Struct_ಅಂಶ* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Cons_Struct_ಅಂಶ* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), Cons_Struct_ಅಂಶ),
                                          ritemp = VN_AS(ritemp->nextp(), Cons_Struct_ಅಂಶ)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    ಶೂನ್ಯ handleAggregateCompare(Ast_ಗಂಟು_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ಸ್ಥಿರಾಂಕ bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ಸ್ಥಿರಾಂಕ bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_ಸಂಖ್ಯೆ& resultr = newConst(nodep)->ಅಂಕೆ();
        resultr.ಗಣ_All_Bits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_ಮೌಲ್ಯ(nodep->lhsp()),
                           fetch_ಮೌಲ್ಯ(nodep->rhsp()));
        ಆದರೆ (neq) {
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Biop* nodep) override {
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        ಆದರೆ (!VN_IS(fetch_ಮೌಲ್ಯ(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        AstConst* ಸ್ಥಿರಾಂಕ valuep = newConst(nodep);
        nodep->ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(newConst(nodep)->ಅಂಕೆ(), fetchConst(nodep->lhsp())->ಅಂಕೆ(),
                             fetchConst(nodep->rhsp())->ಅಂಕೆ());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        ಆದರೆ (!m_params && VN_IS(nodep, Sel) && valuep->ಅಂಕೆ().isAnyX()) {
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ{valuep, valuep->ಅಗಲ(), valuep->ಅಂಕೆ()};
            valuep->ಅಂಕೆ().op_ಬಿಟ್ಟುಗಳು_One(ಅಂಕೆ);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Triop* nodep) override {
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        nodep->ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(newConst(nodep)->ಅಂಕೆ(), fetchConst(nodep->lhsp())->ಅಂಕೆ(),
                             fetchConst(nodep->rhsp())->ಅಂಕೆ(), fetchConst(nodep->thsp())->ಅಂಕೆ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Quadop* nodep) override {
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        nodep->ಸಂಖ್ಯೆ_ನಿರ್ವಹಿಸು(newConst(nodep)->ಅಂಕೆ(), fetchConst(nodep->lhsp())->ಅಂಕೆ(),
                             fetchConst(nodep->rhsp())->ಅಂಕೆ(), fetchConst(nodep->thsp())->ಅಂಕೆ(),
                             fetchConst(nodep->fhsp())->ಅಂಕೆ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogAnd* nodep) override {
        // Need to short circuit
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateConst(nodep->lhsp());
            ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (fetchConst(nodep->lhsp())->ಅಂಕೆ().isNeqZero()) {
                iterateConst(nodep->rhsp());
                ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->rhsp()));
            } ಇಲ್ಲದಿದ್ದರೆ {
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->lhsp()));  // a zero
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogOr* nodep) override {
        // Need to short circuit
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateConst(nodep->lhsp());
            ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (fetchConst(nodep->lhsp())->ಅಂಕೆ().isNeqZero()) {
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->lhsp()));  // a one
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateConst(nodep->rhsp());
                ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->rhsp()));
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateConst(nodep->lhsp());
            ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (fetchConst(nodep->lhsp())->ಅಂಕೆ().isEqZero()) {
                ಸ್ಥಿರಾಂಕ AstConst cnst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 1, 1};  // a one
                new_ಮೌಲ್ಯ(nodep, &cnst);  // a one
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateConst(nodep->rhsp());
                ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->rhsp()));
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateConst(nodep->condp());
            ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (fetchConst(nodep->condp())->ಅಂಕೆ().isNeqZero()) {
                iterateConst(nodep->thenp());
                ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->thenp()));
            } ಇಲ್ಲದಿದ್ದರೆ {
                iterateConst(nodep->elsep());
                ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
                new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->elsep()));
            }
        }
    }

    ಶೂನ್ಯ handle_ನಿಯೋಜಿಸು_Element(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೌಲ್ಯ_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> selps;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp = lhsp;
        ಆಗ (true) {
            ಆದರೆ (AstArraySel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(fromp, ArraySel)) {
                check_ಗಂಟು_Info(selp);
                iterate_And_ಮುಂದಿನ_Const_Null(selp->bitp());  // Bit index
                selps.ತಳ್ಳು_ಹಿಂಭಾಗ(selp);
                fromp = selp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(fromp, StructSel)) {
                check_ಗಂಟು_Info(selp);
                selps.ತಳ್ಳು_ಹಿಂಭಾಗ(selp);
                fromp = selp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ನಿಲ್ಲಿಸು;
            }
        }
        AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(fromp, VarRef);
        ಆದರೆ (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ vscp = varOrScope(varrefp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rootp = fetch_ಹೊರಹೋಗು_ಮೌಲ್ಯ_Null(vscp);
        ಆದರೆ (!rootp) rootp = fetch_ಮೌಲ್ಯ_Null(vscp);
        ಆದರೆ (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            ಹಿಂತಿರುಗಿ;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemp = rootp;
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ selp : vlstd::ಹಿಮ್ಮುಖ_view(selps)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_AS(itemp, ConsArray);
                ಸ್ಥಿರಾಂಕ uint32_t ಸೂಚ್ಯಂಕ = fetchConst(aselp->bitp())->ಗಮ್ಯ_UInt();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                ಆದರೆ (ಸೂಚ್ಯಂಕ >= static_cast<uint32_t>(elements)) {
                    ಹಿಂತಿರುಗಿ;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(ಸೂಚ್ಯಂಕ);
                ಆದರೆ (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_ವೃಕ್ಷ(false);
                    consp->valuep(ಸೂಚ್ಯಂಕ, itemp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = newTrackedClone(fetch_ಮೌಲ್ಯ(ಮೌಲ್ಯ_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ತಳ್ಳು_ಹಿಂಭಾಗ(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        ನಿಯೋಜಿಸು_ಹೊರಹೋಗು_ಮೌಲ್ಯ(nodep, vscp, rootp);
    }
    ಶೂನ್ಯ handle_ನಿಯೋಜಿಸು_Sel(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep, AstSel* selp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೌಲ್ಯ_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_ಸಂಖ್ಯೆ lsb{nodep};
        handle_ನಿಯೋಜಿಸು_Sel_Recurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ vscpnump = fetch_ಹೊರಹೋಗು_Const_Null(vscp)) {
            outconstp = vscpnump;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } ಇಲ್ಲದಿದ್ದರೆ {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                     varrefp->varp()->ಅಗಲ_Min(), 0};
            ಆದರೆ (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->ಅಂಕೆ().ಗಣ_All_Bits0();
            } ಇಲ್ಲದಿದ್ದರೆ {
                outconstp->ಅಂಕೆ().ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
            }
            m_reclaimValuesp.emplace_ಹಿಂಭಾಗ(outconstp);
        }
        outconstp->ಅಂಕೆ().opSelInto(fetchConst(ಮೌಲ್ಯ_Fromp)->ಅಂಕೆ(), lsb, selp->ಅಗಲ_Const());
        ನಿಯೋಜಿಸು_ಹೊರಹೋಗು_ಮೌಲ್ಯ(nodep, vscp, outconstp);
    }
    ಶೂನ್ಯ handle_ನಿಯೋಜಿಸು_Sel_Recurse(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep, AstSel* selp, AstVarRef*& ಹೊರಹೋಗು_Varrefp_Ref,
                                V3_ಸಂಖ್ಯೆ& lsbRef, ಪೂರ್ಣಾಂಕ ಆಳ) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_ಗಂಟು_Info(selp);
        iterate_And_ಮುಂದಿನ_Const_Null(selp->lsbp());  // Bit index
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(selp->fromp(), VarRef)) {
            ಹೊರಹೋಗು_Varrefp_Ref = varrefp;
            lsbRef = fetchConst(selp->lsbp())->ಅಂಕೆ();
            ಹಿಂತಿರುಗಿ;  // And presumably still optimizable()
        }
        ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_ಸಂಖ್ಯೆ sublsb{nodep};
            handle_ನಿಯೋಜಿಸು_Sel_Recurse(nodep, subselp, ಹೊರಹೋಗು_Varrefp_Ref, sublsb /*ref*/, ಆಳ + 1);
            ಆದರೆ (optimizable()) {
                lsbRef = sublsb;
                lsbRef.op_ಕೂಡಿಸು(sublsb, fetchConst(selp->lsbp())->ಅಂಕೆ());
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    ಶೂನ್ಯ handle_ನಿಯೋಜಿಸು_Recurse(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೌಲ್ಯ_Fromp) {
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            ಆದರೆ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ಹಿಂತಿರುಗಿ;
            }
            handle_ನಿಯೋಜಿಸು_Element(nodep, lhsp, ಮೌಲ್ಯ_Fromp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstConcat* ಸ್ಥಿರಾಂಕ selp = VN_CAST(lhsp, Concat)) {
            check_ಗಂಟು_Info(selp);
            ಆದರೆ (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                ಹಿಂತಿರುಗಿ;
            }
            // Split value into left and right concat values
            ಆದರೆ (!m_checkOnly) {
                {
                    AstConst* ಸ್ಥಿರಾಂಕ outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                       selp->lhsp()->ಅಗಲ(), 0};
                    outconstp->ಅಂಕೆ().opSel(fetchConst(ಮೌಲ್ಯ_Fromp)->ಅಂಕೆ(),
                                           selp->lhsp()->ಅಗಲ() + selp->rhsp()->ಅಗಲ() + 1,
                                           selp->rhsp()->ಅಗಲ());
                    new_ಮೌಲ್ಯ(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ವೃಕ್ಷ(), outconstp);
                }
                {
                    AstConst* ಸ್ಥಿರಾಂಕ outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{},
                                       selp->rhsp()->ಅಗಲ_Min(), 0};
                    outconstp->ಅಂಕೆ().opSel(fetchConst(ಮೌಲ್ಯ_Fromp)->ಅಂಕೆ(),
                                           selp->rhsp()->ಅಗಲ() - 1, 0);
                    new_ಮೌಲ್ಯ(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ವೃಕ್ಷ(), outconstp);
                }
            }
            handle_ನಿಯೋಜಿಸು_Recurse(nodep, selp->lhsp(), selp->lhsp());
            handle_ನಿಯೋಜಿಸು_Recurse(nodep, selp->rhsp(), selp->rhsp());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstReplicate* ಸ್ಥಿರಾಂಕ selp = VN_CAST(lhsp, Replicate)) {
            check_ಗಂಟು_Info(selp);
            iterate_And_ಮುಂದಿನ_Const_Null(selp->countp());
            AstConst* ಸ್ಥಿರಾಂಕ countp = VN_CAST(selp->countp(), Const);
            ಆದರೆ (!countp || !countp->ಅಂಕೆ().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                ಹಿಂತಿರುಗಿ;
            }
            handle_ನಿಯೋಜಿಸು_Recurse(nodep, selp->srcp(), ಮೌಲ್ಯ_Fromp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(lhsp, Sel)) {
            ಆದರೆ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ಹಿಂತಿರುಗಿ;
            }
            handle_ನಿಯೋಜಿಸು_Sel(nodep, selp, ಮೌಲ್ಯ_Fromp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(lhsp, VarRef)) {
            ಆದರೆ (m_checkOnly) {
                iterate_And_ಮುಂದಿನ_Const_Null(lhsp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ vscp = varOrScope(VN_CAST(lhsp, VarRef));
                ನಿಯೋಜಿಸು_ಹೊರಹೋಗು_ಮೌಲ್ಯ(nodep, vscp, fetch_ಮೌಲ್ಯ(ಮೌಲ್ಯ_Fromp));
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);

        VL_RESTORER(m_ಒಳ_Dly_ನಿಯೋಜಿಸು);

        ಆದರೆ (VN_IS(nodep, ನಿಯೋಜಿಸು_Force)) {
            clearOptimizable(nodep, "Force");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, ನಿಯೋಜಿಸು_Dly)) {
            ಆದರೆ (m_any_ನಿಯೋಜಿಸು_Comb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_ನಿಯೋಜಿಸು_Dly = true;
            m_ಒಳ_Dly_ನಿಯೋಜಿಸು = true;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_any_ನಿಯೋಜಿಸು_Dly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_ನಿಯೋಜಿಸು_Comb = true;
        }

        ಆದರೆ (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            ಹಿಂತಿರುಗಿ;
        }

        iterate_And_ಮುಂದಿನ_Const_Null(nodep->rhsp());  // Value to assign
        handle_ನಿಯೋಜಿಸು_Recurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_CAST(fetch_ಮೌಲ್ಯ_Null(nodep->fromp()), ConsArray)) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ indexp = fetchConst(nodep->bitp());
            ಸ್ಥಿರಾಂಕ uint32_t ಸ್ಥಾನಾಂತರ = indexp->ಅಂಕೆ().ಗಮ್ಯ_UInt();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ itemp = consp->valuep(ಸ್ಥಾನಾಂತರ);
            ಆದರೆ (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::ಗಮ್ಯ_string(ಸ್ಥಾನಾಂತರ));
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಗಣ_ಮೌಲ್ಯ(nodep, itemp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        ಆದರೆ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        AstConsStruct* ಸ್ಥಿರಾಂಕ consp = VN_AS(fetch_ಮೌಲ್ಯ(nodep->fromp()), ConsStruct);
        ಗಣ_ಮೌಲ್ಯ(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ತುಂಡು_Sel* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        // Fetch the base constant array
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConsArray* ಸ್ಥಿರಾಂಕ consp = VN_CAST(fetch_ಮೌಲ್ಯ_Null(nodep->fromp()), ConsArray)) {
            ಸ್ಥಿರಾಂಕ VNumRange& ತುಂಡು_Range = nodep->declRange();
            ಸ್ಥಿರಾಂಕ uint32_t ತುಂಡು_Elements = ತುಂಡು_Range.elements();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ತುಂಡು_Lo = ತುಂಡು_Range.lo();
            // Use this node's dtype for the slice array
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ಸ್ಥಿರಾಂಕ newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            ಗಾಗಿ (uint32_t idx = 0; idx < ತುಂಡು_Elements; ++idx) {
                ಸ್ಥಿರಾಂಕ uint32_t ತಳ_Idx = ತುಂಡು_Lo + idx;
                newConsp->valuep(idx, consp->valuep(ತಳ_Idx)->clone_ವೃಕ್ಷ(false));
            }
            // Assign the new constant array and track it for later deletion
            ಗಣ_ಮೌಲ್ಯ(nodep, newConsp);
            m_reclaimValuesp.ತಳ್ಳು_ಹಿಂಭಾಗ(newConsp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        UINFO(5, "   CASE " << nodep);
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (optimizable()) {
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
            bool ದೊರಕಿತು = false;
            ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {
                ಆದರೆ (!itemp->isDefault()) {
                    ಗಾಗಿ (Ast_ಗಂಟು* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        ಆದರೆ (ದೊರಕಿತು) ನಿಲ್ಲಿಸು;
                        iterate_And_ಮುಂದಿನ_Const_Null(ep);
                        ಆದರೆ (optimizable()) {
                            V3_ಸಂಖ್ಯೆ ಹೊಂದಿಕೆ{nodep, 1};
                            ಹೊಂದಿಕೆ.opEq(fetchConst(nodep->exprp())->ಅಂಕೆ(), fetchConst(ep)->ಅಂಕೆ());
                            ಆದರೆ (ಹೊಂದಿಕೆ.isNeqZero()) {
                                iterate_And_ಮುಂದಿನ_Const_Null(itemp->stmtsp());
                                ದೊರಕಿತು = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {
                ಆದರೆ (ದೊರಕಿತು) ನಿಲ್ಲಿಸು;
                ಆದರೆ (itemp->isDefault()) {
                    iterate_And_ಮುಂದಿನ_Const_Null(itemp->stmtsp());
                    ದೊರಕಿತು = true;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Case_ಅಂಶ* nodep) override {
        // Real handling is in AstNodeCase
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstComment*) override {}

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        check_ಗಂಟು_Info(nodep);
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->resultp());
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        new_ಮೌಲ್ಯ(nodep, fetch_ಮೌಲ್ಯ(nodep->resultp()));
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        iterateChildrenConst(nodep);
        ಆದರೆ (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstJumpGo* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        ಆದರೆ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (m_checkOnly) {
            iterateChildrenConst(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;

        ಗಾತ್ರ_t iter_ಎಣಿಕೆ = 0;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t iter_ಎಲ್ಲೆ = v3Global.opt.unroll_ಎಲ್ಲೆ();
        ಆಗ (true) {
            ಆದರೆ (iter_ಎಣಿಕೆ > iter_ಎಲ್ಲೆ) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::ಗಮ್ಯ_string(iter_ಎಲ್ಲೆ));
                ನಿಲ್ಲಿಸು;
            }
            ++iter_ಎಣಿಕೆ;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
            ಆದರೆ (jumpingOver()) ನಿಲ್ಲಿಸು;
        }

        ಆದರೆ (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstLoopTest* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        check_ಗಂಟು_Info(nodep);
        iterateConst(nodep->condp());
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (fetchConst(nodep->condp())->ಅಂಕೆ().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstStop* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_ಗಂಟು_Info(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFuncRef* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_ಗಂಟು_Info(nodep);
        ಆದರೆ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        Ast_ಗಂಟು_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        ಆದರೆ (m_params) V3_ಅಗಲ::ಅಗಲ_Params_Edit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        ಆದರೆ (funcp->recursive()) {
            ಆದರೆ (m_recurse_ಎಣಿಕೆ >= v3Global.opt.func_Recursion_ಆಳ()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::ಗಮ್ಯ_string(v3Global.opt.func_Recursion_ಆಳ())
                                            + " times");
                ಹಿಂತಿರುಗಿ;
            }
            ++m_recurse_ಎಣಿಕೆ;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_ಗಂಟು*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> old_ಮೌಲ್ಯಗಳು;

        ಆದರೆ (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_ಮೌಲ್ಯಗಳು](AstVar* varp) {
                ಆದರೆ (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = fetch_ಮೌಲ್ಯ_Null(varp)) {
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nvaluep = newTrackedClone(valuep);
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
                ಆದರೆ (VN_IS(exprp, Const)) ಹಿಂತಿರುಗಿ;  // Speed up as won't change
                ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = fetch_ಮೌಲ್ಯ_Null(exprp)) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nvaluep = newTrackedClone(valuep);
                    old_ಮೌಲ್ಯಗಳು.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::ಸದಿಶ<std::pair<AstVar*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>> ಪೋರ್ಟ್_ಮೌಲ್ಯಗಳು;
        ಗಾಗಿ (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.ಕೊನೆ(); ++it) {
            AstVar* ಸ್ಥಿರಾಂಕ portp = it->ಮೊದಲ;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ pinp = it->ಎರಡನೇ->exprp();
            ಆದರೆ (pinp) {  // Else too few arguments in function call - ignore it
                ಆದರೆ (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    ಹಿಂತಿರುಗಿ;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                ಪೋರ್ಟ್_ಮೌಲ್ಯಗಳು.emplace_ಹಿಂಭಾಗ(std::make_pair(portp, newTrackedClone(fetch_ಮೌಲ್ಯ(pinp))));
            }
        }
        // Apply value to the function
        ಆದರೆ (!m_checkOnly && optimizable())
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : ಪೋರ್ಟ್_ಮೌಲ್ಯಗಳು) {
                ಆದರೆ (!m_checkOnly && optimizable()) new_ಮೌಲ್ಯ(it.ಮೊದಲ, it.ಎರಡನೇ);
            }
        Sim_ರಾಶಿ_ಗಂಟು ರಾಶಿ_ಗಂಟು{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&ರಾಶಿ_ಗಂಟು);
        ಆದರೆ (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                ಆದರೆ (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* returnp = nullptr;
        ಆದರೆ (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_ಮೌಲ್ಯ(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : old_ಮೌಲ್ಯಗಳು) {
            ಆದರೆ (it.ಎರಡನೇ) new_ಮೌಲ್ಯ(it.ಮೊದಲ, it.ಎರಡನೇ);
        }
        ಆದರೆ (returnp) new_ಮೌಲ್ಯ(nodep, returnp);
        ಆದರೆ (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_ಎಣಿಕೆ > 0, nodep, "recurse underflow");
            --m_recurse_ಎಣಿಕೆ;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!m_params) {
            bad_ಗಂಟು_Type(nodep);
            ಹಿಂತಿರುಗಿ;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstScopeName* /*nodep*/) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        // Ignore
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatArg* nodep) override {
        check_ಗಂಟು_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->exprp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;

        Ast_ಗಂಟು* ಮುಂದಿನ_Argp = nodep->exprsp();
        string ಫಲಿತಾಂಶ;
        ಸ್ಥಿರಾಂಕ string format = nodep->ಪಠ್ಯ();
        ಸ್ವಯಂಚಾಲಿತ pos = format.cbegin();
        bool ಒಳ_Pct = false;
        string ಅಗಲ;
        ಗಾಗಿ (; pos != format.cend(); ++pos) {
            ಆದರೆ (!ಒಳ_Pct && pos[0] == '%') {
                ಒಳ_Pct = true;
                ಅಗಲ = "";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಒಳ_Pct) {  // Normal text
                ಫಲಿತಾಂಶ += *pos;
            } ಇಲ್ಲದಿದ್ದರೆ {  // Format character
                ಆದರೆ (std::ಅಂಕಿಯೇ(pos[0])) {
                    ಅಗಲ += pos[0];
                    ಮುಂದುವರಿ;
                }

                ಒಳ_Pct = false;

                ಆದರೆ (V3_ಸಂಖ್ಯೆ::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ argp = ಮುಂದಿನ_Argp;
                    ಮುಂದಿನ_Argp = ಮುಂದಿನ_Argp->nextp();
                    AstSFormatArg* ಸ್ಥಿರಾಂಕ fargp = VN_CAST(argp, SFormatArg);
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ಸ್ಥಿರಾಂಕ constp = fetchConstNull(subargp);
                    ಸ್ಥಿರಾಂಕ VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    ಆದರೆ (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        ನಿಲ್ಲಿಸು;
                    }
                    ಸ್ಥಿರಾಂಕ string pformat = "%"s + ಅಗಲ + pos[0];
                    ಫಲಿತಾಂಶ += formatAttr.isEnum()
                                  ? constp->ಅಂಕೆ().displayedEnum(fargp, pformat)
                                  : constp->ಅಂಕೆ().displayed(nodep, pformat, formatAttr);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆಯ್ಕೆ (std::tolower(pos[0])) {
                    ಸ್ಥಿತಿ '%': ಫಲಿತಾಂಶ += "%"; ನಿಲ್ಲಿಸು;
                    ಸ್ಥಿತಿ 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        ಫಲಿತಾಂಶ += "%m";
                        ನಿಲ್ಲಿಸು;
                    ಪೂರ್ವನಿಯೋಜಿತ: clearOptimizable(nodep, "Unknown $display-like format code."); ನಿಲ್ಲಿಸು;
                    }
                }
            }
        }

        AstConst* ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ_Constp = new AstConst{nodep->fileline(), AstConst::String{}, ಫಲಿತಾಂಶ};
        ಗಣ_ಮೌಲ್ಯ(nodep, ಫಲಿತಾಂಶ_Constp);
        m_reclaimValuesp.ತಳ್ಳು_ಹಿಂಭಾಗ(ಫಲಿತಾಂಶ_Constp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstDisplay* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_ಗಂಟು_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_params) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ textp = fetchConst(nodep->fmtp());
            ಆಯ್ಕೆ (nodep->displayType()) {
            ಸ್ಥಿತಿ VDisplayType::DT_DISPLAY:  // FALLTHRU
            ಸ್ಥಿತಿ VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ VDisplayType::DT_WRITE:  // FALLTHRU
            ಪೂರ್ವನಿಯೋಜಿತ: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಮ್ಯ_String_N* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;  // Accelerate
        check_ಗಂಟು_Info(nodep);
        iterateChildrenConst(nodep);
        ಆದರೆ (m_checkOnly || !optimizable()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ std::string ಫಲಿತಾಂಶ = ಗಮ್ಯ_String_Recurse(fetch_ಮೌಲ್ಯ(nodep->lhsp()));
        ಆದರೆ (!optimizable()) ಹಿಂತಿರುಗಿ;
        AstConst* ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ_Constp = new AstConst{nodep->fileline(), AstConst::String{}, ಫಲಿತಾಂಶ};
        ಗಣ_ಮೌಲ್ಯ(nodep, ಫಲಿತಾಂಶ_Constp);
        m_reclaimValuesp.ತಳ್ಳು_ಹಿಂಭಾಗ(ಫಲಿತಾಂಶ_Constp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        ತಿಳಿದ_Bad_ಗಂಟು_Type(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Get_Initial_Random_ಬೀಜ* nodep) override { ತಿಳಿದ_Bad_ಗಂಟು_Type(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override { ತಿಳಿದ_Bad_ಗಂಟು_Type(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSampled* nodep) override { ತಿಳಿದ_Bad_ಗಂಟು_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (jumpingOver()) ಹಿಂತಿರುಗಿ;
        bad_ಗಂಟು_Type(nodep);
    }

    // MEMBERS - called by constructor
    ಶೂನ್ಯ ಗಣ_ಬಹುಲಕ(bool scoped, bool checkOnly, bool params) {
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
        ಗಣ_ಬಹುಲಕ(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    ಶೂನ್ಯ clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_any_ನಿಯೋಜಿಸು_Comb = false;
        m_any_ನಿಯೋಜಿಸು_Dly = false;
        m_ಒಳ_Dly_ನಿಯೋಜಿಸು = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_ಎಣಿಕೆ = 0;
        m_ದತ್ತಾಂಶ_ಎಣಿಕೆ = 0;
        m_jumptargetp = nullptr;

        Ast_ಗಂಟು::user1_Clear_ವೃಕ್ಷ();
        m_varAux.clear();
        ++m_constGeneration;
    }
    ಶೂನ್ಯ main_ಕೋಷ್ಟಕ_Check(Ast_ಗಂಟು* nodep) {
        ಗಣ_ಬಹುಲಕ(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ಶೂನ್ಯ main_ಕೋಷ್ಟಕ_Emulate(Ast_ಗಂಟು* nodep) {
        ಗಣ_ಬಹುಲಕ(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ಶೂನ್ಯ mainParamEmulate(Ast_ಗಂಟು* nodep) {
        ಗಣ_ಬಹುಲಕ(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::ಗಣ<Ast_ಗಂಟು*> ಬಳಸದ_Rootps;
        ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep : m_reclaimValuesp) {
            ಆದರೆ (!nodep->backp()) ಬಳಸದ_Rootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep : ಬಳಸದ_Rootps) VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
