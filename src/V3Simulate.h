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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3SIMULATE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3SIMULATE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Width.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <stack>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_ਢੇਰ_ਗੰਢ final {
ਜਨਤਕ:
    // MEMBERS
    AstFuncRef* ਅਚਲ m_funcp;
    V3TaskConnects* ਅਚਲ m_tconnects;
    // CONSTRUCTORS
    Sim_ਢੇਰ_ਗੰਢ(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ਢੇਰ_ਗੰਢ() = ਮੂਲ;
};

class SimulateVisitor VL_NOT_FINAL : ਜਨਤਕ VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

ਨਿਜੀ:
    // CONSTANTS
    ਸਥਿਰ constexpr ਪੂਰਨ CALL_ਢੇਰ_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    ਗਿਣਤੀ VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    ਬਣਤਰ AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਬਾਹਰਲਾ_Valuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = nullptr;
    };

    AstUser1Allocator<Ast_ਗੰਢ, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    ਬਣਤਰ ConstAllocator final {
        ਮਾਪ_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        ਮਾਪ_t m_ਅਗਲਾ_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(ਮਾਪ_t currentGeneration, Ast_ਗੰਢ* nodep) {
            ਜੇ (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_ਅਗਲਾ_Free = 0;
            }
            UASSERT_OBJ(m_ਅਗਲਾ_Free <= m_constps.ਮਾਪ(), nodep, "Should only allocate at end");
            ਜੇ (m_ਅਗਲਾ_Free == m_constps.ਮਾਪ()) {
                m_constps.ਧੱਕੋ_ਪਿੱਠ(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ਅਚਲ constp = m_constps[m_ਅਗਲਾ_Free++];
            constp->ਅੰਕ().nodep(nodep);
            ਵਾਪਸ constp;
        }

        ~ConstAllocator() {
            ਲਈ (AstConst* ਅਚਲ constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_ਗੰਢ* m_whyNotNodep;  ///< First node not optimizable
    bool m_any_ਸੌਂਪੋ_Dly;  ///< True if found a delayed assignment
    bool m_any_ਸੌਂਪੋ_Comb;  ///< True if found a non-delayed assignment
    bool m_ਵਿੱਚ_Dly_ਸੌਂਪੋ;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    ਪੂਰਨ m_instr_ਗਿਣਤੀ;  ///< Number of nodes
    ਪੂਰਨ m_ਅੰਕੜੇ_ਗਿਣਤੀ;  ///< Bytes of data
    ਪੂਰਨ m_recurse_ਗਿਣਤੀ = 0;  // Now deep in current recursion
    Ast_ਗੰਢ* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ_DType*, ConstAllocator> m_constps;
    ਮਾਪ_t m_constGeneration = 0;
    std::ਸਦਿਸ਼<Sim_ਢੇਰ_ਗੰਢ*> m_call_ਢੇਰ;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::ਸਦਿਸ਼<Ast_ਗੰਢ*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_ਸੰਖਿਆ(ਅਚਲ V3_ਸੰਖਿਆ* nump, ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        ਜੇ (ਅਚਲ AstRefDType* ਅਚਲ refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ stp = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType)) {
            ਜੇ (stp->packed()) {
                std::ostringstream ਬਾਹਰਲਾ;
                ਬਾਹਰਲਾ << "'{";
                ਲਈ (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = itemp->ਚੌੜਾਈ();
                    ਅਚਲ ਪੂਰਨ lsb = itemp->lsb();
                    ਅਚਲ ਪੂਰਨ msb = lsb + ਚੌੜਾਈ - 1;
                    V3_ਸੰਖਿਆ ਖੇਤਰ_ਅੰਕ{nump, ਚੌੜਾਈ};
                    ਖੇਤਰ_ਅੰਕ.opSel(*nump, msb, lsb);
                    ਬਾਹਰਲਾ << itemp->name() << ": ";
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਬੱਚਾ_Typep = itemp->subDTypep()) {
                        ਬਾਹਰਲਾ << pretty_ਸੰਖਿਆ(&ਖੇਤਰ_ਅੰਕ, ਬੱਚਾ_Typep);
                    } ਨਹੀਂਤਾਂ {
                        ਬਾਹਰਲਾ << ਖੇਤਰ_ਅੰਕ;
                    }
                    ਜੇ (itemp->nextp()) ਬਾਹਰਲਾ << ", ";
                }
                ਬਾਹਰਲਾ << "}";
                ਵਾਪਸ ਬਾਹਰਲਾ.str();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstPackArrayDType* ਅਚਲ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਬੱਚਾ_Typep = arrayp->subDTypep()) {
                std::ostringstream ਬਾਹਰਲਾ;
                ਬਾਹਰਲਾ << "[";
                ਅਚਲ ਪੂਰਨ arrayElements = arrayp->elementsConst();
                ਲਈ (ਪੂਰਨ element = 0; element < arrayElements; ++element) {
                    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = ਬੱਚਾ_Typep->ਚੌੜਾਈ();
                    ਅਚਲ ਪੂਰਨ lsb = ਚੌੜਾਈ * element;
                    ਅਚਲ ਪੂਰਨ msb = lsb + ਚੌੜਾਈ - 1;
                    V3_ਸੰਖਿਆ ਖੇਤਰ_ਅੰਕ{nump, ਚੌੜਾਈ};
                    ਖੇਤਰ_ਅੰਕ.opSel(*nump, msb, lsb);
                    ਅਚਲ ਪੂਰਨ arrayElem = arrayp->lo() + element;
                    ਬਾਹਰਲਾ << arrayElem << " = " << pretty_ਸੰਖਿਆ(&ਖੇਤਰ_ਅੰਕ, ਬੱਚਾ_Typep);
                    ਜੇ (element < arrayElements - 1) ਬਾਹਰਲਾ << ", ";
                }
                ਬਾਹਰਲਾ << "]";
                ਵਾਪਸ ਬਾਹਰਲਾ.str();
            }
        }
        ਵਾਪਸ nump->ascii();
    }

    // Checking METHODS
ਜਨਤਕ:
    /// Call other-this function on all new *non-constant* var references
    ਆਭਾਸੀ ਖਾਲੀ varRefCb(AstVarRef* nodep) {}

    ਖਾਲੀ clearOptimizable(Ast_ਗੰਢ* nodep /*null ok*/, ਅਚਲ string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        ਜੇ (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            ਜੇ (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_ਅਗੇਤਰ("Clear optimizable: " << why);
                ਜੇ (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ਢੇਰ;
            ਪੂਰਨ n = 0;
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& callstack : vlstd::ਉਲਟ_view(m_call_ਢੇਰ)) {
                ਅਚਲ AstFuncRef* ਅਚਲ funcp = callstack->m_funcp;
                ਢੇਰ << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                ਲਈ (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->ਸਿਰਾ(); ++conIt) {
                    ਅਚਲ AstVar* ਅਚਲ portp = conIt->ਪਹਿਲਾ;
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinp = conIt->ਦੂਜਾ->exprp();
                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = pinp->dtypep();
                    ਜੇ (AstConst* ਅਚਲ valp = fetchConstNull(pinp)) {
                        ਢੇਰ << "\n           " << portp->prettyName() << " = "
                              << pretty_ਸੰਖਿਆ(&valp->ਅੰਕ(), dtypep);
                    } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = fetch_ਮੁੱਲ_Null(pinp)) {
                        ਢੇਰ << "\n           " << portp->prettyName() << " = "
                              << ਮੰਜ਼ਿਲ_String_Recurse(valuep);
                    }
                }
                ਜੇ (++n > CALL_ਢੇਰ_MAX) {
                    ਢੇਰ << "\n           ... stack truncated";
                    ਰੋਕੋ;
                }
            }
            m_whyNotOptimizable += ਢੇਰ.str();
        }
    }
    bool optimizable() ਅਚਲ { ਵਾਪਸ m_whyNotNodep == nullptr; }
    string why_Not_ਸੁਨੇਹਾ() ਅਚਲ { ਵਾਪਸ m_whyNotOptimizable; }
    Ast_ਗੰਢ* whyNotNodep() ਅਚਲ { ਵਾਪਸ m_whyNotNodep; }

    bool is_ਸੌਂਪੋ_Dly() ਅਚਲ { ਵਾਪਸ m_any_ਸੌਂਪੋ_Dly; }
    bool isImpure() ਅਚਲ { ਵਾਪਸ m_isImpure; }
    bool isCoverage() ਅਚਲ { ਵਾਪਸ m_isCoverage; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_instr_ਗਿਣਤੀ; }
    ਪੂਰਨ ਅੰਕੜੇ_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_ਅੰਕੜੇ_ਗਿਣਤੀ; }

    // Simulation METHODS
ਨਿਜੀ:
    AstConst* allocConst(Ast_ਗੰਢ* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        ਵਾਪਸ m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

ਜਨਤਕ:
    ਖਾਲੀ new_ਮੁੱਲ(Ast_ਗੰਢ* nodep, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->ਅੰਕ().op_ਸੌਂਪੋ(constp->ਅੰਕ());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (fetch_ਮੁੱਲ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            ਸੈੱਟ_ਮੁੱਲ(nodep, newTrackedClone(const_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(valuep)));
        }
    }
    ਖਾਲੀ new_ਬਾਹਰਲਾ_ਮੁੱਲ(Ast_ਗੰਢ* nodep, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const)) {
            new_ਬਾਹਰਲਾ_Const(nodep)->ਅੰਕ().op_ਸੌਂਪੋ(constp->ਅੰਕ());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (fetch_ਬਾਹਰਲਾ_ਮੁੱਲ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            ਸੈੱਟ_ਬਾਹਰਲਾ_ਮੁੱਲ(nodep, newTrackedClone(const_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(valuep)));
        }
    }

ਨਿਜੀ:
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newTrackedClone(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = nodep->clone_ਰੁੱਖ(false);
        m_reclaimValuesp.ਧੱਕੋ_ਪਿੱਠ(newp);
        ਵਾਪਸ newp;
    }
    AstConst* newConst(Ast_ਗੰਢ* nodep) {
        // Set a constant value for this node
        ਜੇ (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ਅਚਲ constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            ਵਾਪਸ constp;
        }
        ਵਾਪਸ fetchConst(nodep);
    }
    AstConst* new_ਬਾਹਰਲਾ_Const(Ast_ਗੰਢ* nodep) {
        // Set a var-output constant value for this node
        ਜੇ (!VN_IS(m_varAux(nodep).ਬਾਹਰਲਾ_Valuep, Const)) {
            AstConst* ਅਚਲ constp = allocConst(nodep);
            m_varAux(nodep).ਬਾਹਰਲਾ_Valuep = constp;
            ਵਾਪਸ constp;
        }
        ਵਾਪਸ fetch_ਬਾਹਰਲਾ_Const(nodep);
    }

ਜਨਤਕ:
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* fetch_ਮੁੱਲ_Null(Ast_ਗੰਢ* nodep) { ਵਾਪਸ m_varAux(nodep).valuep; }

ਨਿਜੀ:
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* fetch_ਬਾਹਰਲਾ_ਮੁੱਲ_Null(Ast_ਗੰਢ* nodep) { ਵਾਪਸ m_varAux(nodep).ਬਾਹਰਲਾ_Valuep; }
    AstConst* fetchConstNull(Ast_ਗੰਢ* nodep) { ਵਾਪਸ VN_CAST(fetch_ਮੁੱਲ_Null(nodep), Const); }
    AstConst* fetch_ਬਾਹਰਲਾ_Const_Null(Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ VN_CAST(fetch_ਬਾਹਰਲਾ_ਮੁੱਲ_Null(nodep), Const);
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* fetch_ਮੁੱਲ(Ast_ਗੰਢ* nodep) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = fetch_ਮੁੱਲ_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        ਵਾਪਸ valuep;
    }
    AstConst* fetchConst(Ast_ਗੰਢ* nodep) {
        AstConst* ਅਚਲ constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        ਵਾਪਸ constp;
    }
    AstConst* fetch_ਬਾਹਰਲਾ_Const(Ast_ਗੰਢ* nodep) {
        AstConst* ਅਚਲ constp = fetch_ਬਾਹਰਲਾ_Const_Null(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        ਵਾਪਸ constp;
    }

ਜਨਤਕ:
    V3_ਸੰਖਿਆ* fetch_ਸੰਖਿਆ_Null(Ast_ਗੰਢ* nodep) {
        AstConst* ਅਚਲ constp = fetchConstNull(nodep);
        ਜੇ (constp) ਵਾਪਸ &constp->ਅੰਕ();
        ਵਾਪਸ nullptr;
    }
    V3_ਸੰਖਿਆ* fetch_ਬਾਹਰਲਾ_ਸੰਖਿਆ_Null(Ast_ਗੰਢ* nodep) {
        AstConst* ਅਚਲ constp = fetch_ਬਾਹਰਲਾ_Const_Null(nodep);
        ਜੇ (constp) ਵਾਪਸ &constp->ਅੰਕ();
        ਵਾਪਸ nullptr;
    }

ਨਿਜੀ:
    ਖਾਲੀ ਸੈੱਟ_ਮੁੱਲ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    ਖਾਲੀ ਸੈੱਟ_ਬਾਹਰਲਾ_ਮੁੱਲ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).ਬਾਹਰਲਾ_Valuep = valuep;
    }

    ਖਾਲੀ check_ਗੰਢ_Info(Ast_ਗੰਢ* nodep, bool ignorePredict = false) {
        ਜੇ (m_checkOnly) {
            m_instr_ਗਿਣਤੀ += nodep->instr_ਗਿਣਤੀ();
            m_ਅੰਕੜੇ_ਗਿਣਤੀ += nodep->ਚੌੜਾਈ();
        }
        ਜੇ (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) m_isImpure = true;
    }

    ਖਾਲੀ ਗਿਆਤ_Bad_ਗੰਢ_Type(Ast_ਗੰਢ* nodep) {
        // Call for node types we know we can't handle
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        check_ਗੰਢ_Info(nodep);
        ਜੇ (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    ਖਾਲੀ bad_ਗੰਢ_Type(Ast_ਗੰਢ* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_ਗੰਢ_Info(nodep);
        ਜੇ (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
            ਸਥਿਰ std::ਸੈੱਟ<VNType> s_typePrinted;
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = s_typePrinted.emplace(nodep->type());
            ਜੇ (pair.ਦੂਜਾ)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#ਜੇ_ਅੰਤ
        }
    }

    Ast_ਗੰਢ* varOrScope(AstVarRef* nodep) ਅਚਲ {
        Ast_ਗੰਢ* vscp;
        ਜੇ (m_scoped) {
            vscp = nodep->varScopep();
        } ਨਹੀਂਤਾਂ {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        ਵਾਪਸ vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ਅਚਲ { ਵਾਪਸ m_jumptargetp; }
    ਖਾਲੀ ਸੌਂਪੋ_ਬਾਹਰਲਾ_ਮੁੱਲ(ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, Ast_ਗੰਢ* vscp, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਜੇ (VN_IS(nodep, ਸੌਂਪੋ_Dly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_ਬਾਹਰਲਾ_ਮੁੱਲ(vscp, valuep);
        } ਨਹੀਂਤਾਂ {
            new_ਮੁੱਲ(vscp, valuep);
            ਸੈੱਟ_ਬਾਹਰਲਾ_ਮੁੱਲ(vscp, fetch_ਮੁੱਲ(vscp));
        }
    }

    string ਮੰਜ਼ਿਲ_String_Recurse(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        // Return string representation of a value, or clearOptimizable
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ avaluep = VN_CAST(valuep, Const)) {
            ਵਾਪਸ "'h"s + avaluep->ਅੰਕ().displayed(valuep, "%0x");
        }
        ਜੇ (ਅਚਲ AstConsArray* ਅਚਲ avaluep = VN_CAST(valuep, ConsArray)) {
            string ਨਤੀਜਾ = "'{";
            string comma;
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ਲਈ (ਪੂਰਨ n = 0; n < dtypep->elementsConst(); ++n) {
                ਨਤੀਜਾ += comma + ਮੰਜ਼ਿਲ_String_Recurse(avaluep->valuep(n));
                comma = ", ";
            }
            ਨਤੀਜਾ += "}";
            ਵਾਪਸ ਨਤੀਜਾ;
        }
        ਜੇ (ਅਚਲ AstConsStruct* ਅਚਲ avaluep = VN_CAST(valuep, ConsStruct)) {
            string ਨਤੀਜਾ = "'{";
            string comma;
            ਲਈ (ਅਚਲ Ast_Cons_Struct_ਵਸਤੂ* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_ਵਸਤੂ)) {
                ਨਤੀਜਾ
                    += comma + itemp->memberp()->name() + ":" + ਮੰਜ਼ਿਲ_String_Recurse(itemp->valuep());
                comma = ", ";
            }
            ਨਤੀਜਾ += "}";
            ਵਾਪਸ ਨਤੀਜਾ;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        ਵਾਪਸ "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newInitValuep(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ਜੇ (AstUnpackArrayDType* ਅਚਲ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elementp = newInitValuep(nodep, arrayp->subDTypep());
            ਵਾਪਸ elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ਅਚਲ structp = VN_CAST(dtypep, StructDType);
        ਜੇ (structp && !structp->packed()) {
            AstConsStruct* ਅਚਲ newp = new AstConsStruct{nodep->fileline(), structp};
            ਲਈ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ memberValuep = newInitValuep(nodep, memberp->subDTypep());
                ਜੇ (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_ਰੁੱਖ(), newp);
                    ਵਾਪਸ nullptr;
                }
                newp->ਜੋੜੋ_Itemsp(new Ast_Cons_Struct_ਵਸਤੂ{nodep->fileline(), memberp, memberValuep});
            }
            ਵਾਪਸ newp;
        }
        ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ basicp = dtypep->basicp()) {
            AstConst* ਅਚਲ constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, basicp->ਚੌੜਾਈ_Min(), 0};
            ਜੇ (basicp->isZeroInit()) {
                constp->ਅੰਕ() = V3_ਸੰਖਿਆ{nodep, basicp};
            } ਨਹੀਂਤਾਂ {
                constp->ਅੰਕ().ਸੈੱਟ_All_ਬਿੱਟ_X();
            }
            ਵਾਪਸ constp;
        }
        ਵਾਪਸ nullptr;
    }
    ਖਾਲੀ initVar(AstVar* nodep) {
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_ਮੁੱਲ(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_ਰੁੱਖ(), valuep);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_ਗੰਢ* ਅਚਲ vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        ਜੇ (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), ਗੰਢ_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        ਜੇ (nodep->access().isWriteOrRW()) {
            ਜੇ (m_ਵਿੱਚ_Dly_ਸੌਂਪੋ) {
                ਜੇ (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    ਜੇ (m_checkOnly) varRefCb(nodep);
                }
            } ਨਹੀਂਤਾਂ {  // nondly asn
                ਜੇ (!(m_varAux(vscp).usage & VU_LV)) {
                    ਜੇ (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    ਜੇ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ਜੇ (nodep->access().isReadOrRW()) {
            ਜੇ (!(m_varAux(vscp).usage & VU_RV)) {
                ਜੇ (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                ਅਚਲ bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep
                    = varIsConst ? fetch_ਮੁੱਲ_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                ਜੇ (varIsConst && valuep) {
                    ਜੇ (!m_checkOnly && optimizable()) new_ਮੁੱਲ(vscp, valuep);
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = fetch_ਮੁੱਲ_Null(vscp);
            ਜੇ (!valuep) {
                ਜੇ (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } ਨਹੀਂਤਾਂ {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            ਸੈੱਟ_ਮੁੱਲ(nodep, valuep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (m_scoped) {
            bad_ਗੰਢ_Type(nodep);
            ਵਾਪਸ;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!m_params) {
            bad_ਗੰਢ_Type(nodep);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->dpiImport()) {
            ਜੇ (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        ਜੇ (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialAutomaticStmt* nodep) override {
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStaticStmt* nodep) override {
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstInitialStatic* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!m_params) {
            bad_ਗੰਢ_Type(nodep);
            ਵਾਪਸ;
        }
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        UINFO(5, "   IF " << nodep);
        check_ਗੰਢ_Info(nodep);
        ਜੇ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ਨਹੀਂਤਾਂ {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->condp());
            ਜੇ (optimizable()) {
                ਜੇ (fetchConst(nodep->condp())->ਅੰਕ().isNeqZero()) {
                    iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
                } ਨਹੀਂਤਾਂ {
                    iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        check_ਗੰਢ_Info(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        new_ਮੁੱਲ(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    ਖਾਲੀ ਦੇਖੋ(AstConsArray* nodep) override {
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        AstConsArray* ਅਚਲ valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_ਮੁੱਲ(nodep->defaultp())->clone_ਰੁੱਖ(false) : nullptr};
        ਲਈ (ਅਚਲ Ast_Cons_Array_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_ਵਸਤੂ)) {
            valuep->valuep(itemp->ਸੂਚਕਾਂਕ(), fetch_ਮੁੱਲ(itemp->valuep())->clone_ਰੁੱਖ(false));
        }
        m_reclaimValuesp.ਧੱਕੋ_ਪਿੱਠ(valuep);
        ਸੈੱਟ_ਮੁੱਲ(nodep, valuep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsStruct* nodep) override {
        ਜੇ (!m_params) {
            bad_ਗੰਢ_Type(nodep);
            ਵਾਪਸ;
        }
        check_ਗੰਢ_Info(nodep);
        ਲਈ (Ast_Cons_Struct_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_ਵਸਤੂ)) {
            iterateConst(itemp->valuep());
        }
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        AstConsStruct* ਅਚਲ valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        ਲਈ (Ast_Cons_Struct_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_ਵਸਤੂ)) {
            valuep->ਜੋੜੋ_Itemsp(
                new Ast_Cons_Struct_ਵਸਤੂ{itemp->fileline(), itemp->memberp(),
                                      fetch_ਮੁੱਲ(itemp->valuep())->clone_ਰੁੱਖ(false)});
        }
        m_reclaimValuesp.ਧੱਕੋ_ਪਿੱਠ(valuep);
        ਸੈੱਟ_ਮੁੱਲ(nodep, valuep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack* nodep) override {
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cvtp = new Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack{
            nodep->fileline(), fetch_ਮੁੱਲ(nodep->fromp())->clone_ਰੁੱਖ(false), nodep->dtypep()};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ਧੱਕੋ_ਪਿੱਠ(valuep);
        ਸੈੱਟ_ਮੁੱਲ(nodep, valuep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cons_Array_ਵਸਤੂ* nodep) override {
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ_Ref* nodep) override {
        check_ਗੰਢ_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        Ast_ਗੰਢ* ਅਚਲ valuep = nodep->itemp()->valuep();
        ਜੇ (valuep) {
            iterate_And_ਅਗਲਾ_Const_Null(valuep);
            ਜੇ (!optimizable()) ਵਾਪਸ;
            new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(valuep));
        } ਨਹੀਂਤਾਂ {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Uniop* nodep) override {
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        nodep->ਸੰਖਿਆ_ਚਲਾਓ(newConst(nodep)->ਅੰਕ(), fetchConst(nodep->lhsp())->ਅੰਕ());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    ਖਾਲੀ aggregateEqRecurse(V3_ਸੰਖਿਆ& resultr, Ast_ਗੰਢ* nodep, bool caseEq, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp,
                            Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ lconstp = VN_CAST(lhsp, Const)) {
            ਅਚਲ V3_ਸੰਖਿਆ& lnum = lconstp->ਅੰਕ();
            ਅਚਲ V3_ਸੰਖਿਆ& rnum = VN_AS(rhsp, Const)->ਅੰਕ();
            V3_ਸੰਖਿਆ eq{nodep, 1};
            ਜੇ (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } ਨਹੀਂਤਾਂ ਜੇ (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } ਨਹੀਂਤਾਂ ਜੇ (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } ਨਹੀਂਤਾਂ {
                eq.opEq(lnum, rnum);
            }
            ਅਚਲ V3_ਸੰਖਿਆ prev = resultr;
            resultr.opLogAnd(prev, eq);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstConsArray* ਅਚਲ lconsp = VN_CAST(lhsp, ConsArray)) {
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            ਲਈ (ਪੂਰਨ n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ Ast_Cons_Struct_ਵਸਤੂ* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            ਲਈ (ਅਚਲ Ast_Cons_Struct_ਵਸਤੂ* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), Cons_Struct_ਵਸਤੂ),
                                          ritemp = VN_AS(ritemp->nextp(), Cons_Struct_ਵਸਤੂ)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    ਖਾਲੀ handleAggregateCompare(Ast_ਗੰਢ_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ਅਚਲ bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ਅਚਲ bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_ਸੰਖਿਆ& resultr = newConst(nodep)->ਅੰਕ();
        resultr.ਸੈੱਟ_All_Bits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_ਮੁੱਲ(nodep->lhsp()),
                           fetch_ਮੁੱਲ(nodep->rhsp()));
        ਜੇ (neq) {
            ਅਚਲ V3_ਸੰਖਿਆ eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Biop* nodep) override {
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        ਜੇ (!VN_IS(fetch_ਮੁੱਲ(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            ਵਾਪਸ;
        }
        AstConst* ਅਚਲ valuep = newConst(nodep);
        nodep->ਸੰਖਿਆ_ਚਲਾਓ(newConst(nodep)->ਅੰਕ(), fetchConst(nodep->lhsp())->ਅੰਕ(),
                             fetchConst(nodep->rhsp())->ਅੰਕ());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        ਜੇ (!m_params && VN_IS(nodep, Sel) && valuep->ਅੰਕ().isAnyX()) {
            ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ{valuep, valuep->ਚੌੜਾਈ(), valuep->ਅੰਕ()};
            valuep->ਅੰਕ().op_ਬਿੱਟ_One(ਅੰਕ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Triop* nodep) override {
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        nodep->ਸੰਖਿਆ_ਚਲਾਓ(newConst(nodep)->ਅੰਕ(), fetchConst(nodep->lhsp())->ਅੰਕ(),
                             fetchConst(nodep->rhsp())->ਅੰਕ(), fetchConst(nodep->thsp())->ਅੰਕ());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Quadop* nodep) override {
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        nodep->ਸੰਖਿਆ_ਚਲਾਓ(newConst(nodep)->ਅੰਕ(), fetchConst(nodep->lhsp())->ਅੰਕ(),
                             fetchConst(nodep->rhsp())->ਅੰਕ(), fetchConst(nodep->thsp())->ਅੰਕ(),
                             fetchConst(nodep->fhsp())->ਅੰਕ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogAnd* nodep) override {
        // Need to short circuit
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        ਜੇ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ਨਹੀਂਤਾਂ {
            iterateConst(nodep->lhsp());
            ਜੇ (!optimizable()) ਵਾਪਸ;
            ਜੇ (fetchConst(nodep->lhsp())->ਅੰਕ().isNeqZero()) {
                iterateConst(nodep->rhsp());
                ਜੇ (!optimizable()) ਵਾਪਸ;
                new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(nodep->rhsp()));
            } ਨਹੀਂਤਾਂ {
                new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(nodep->lhsp()));  // a zero
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogOr* nodep) override {
        // Need to short circuit
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        ਜੇ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ਨਹੀਂਤਾਂ {
            iterateConst(nodep->lhsp());
            ਜੇ (!optimizable()) ਵਾਪਸ;
            ਜੇ (fetchConst(nodep->lhsp())->ਅੰਕ().isNeqZero()) {
                new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(nodep->lhsp()));  // a one
            } ਨਹੀਂਤਾਂ {
                iterateConst(nodep->rhsp());
                ਜੇ (!optimizable()) ਵਾਪਸ;
                new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(nodep->rhsp()));
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        ਜੇ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ਨਹੀਂਤਾਂ {
            iterateConst(nodep->lhsp());
            ਜੇ (!optimizable()) ਵਾਪਸ;
            ਜੇ (fetchConst(nodep->lhsp())->ਅੰਕ().isEqZero()) {
                ਅਚਲ AstConst cnst{nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 1, 1};  // a one
                new_ਮੁੱਲ(nodep, &cnst);  // a one
            } ਨਹੀਂਤਾਂ {
                iterateConst(nodep->rhsp());
                ਜੇ (!optimizable()) ਵਾਪਸ;
                new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(nodep->rhsp()));
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        ਜੇ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ਨਹੀਂਤਾਂ {
            iterateConst(nodep->condp());
            ਜੇ (!optimizable()) ਵਾਪਸ;
            ਜੇ (fetchConst(nodep->condp())->ਅੰਕ().isNeqZero()) {
                iterateConst(nodep->thenp());
                ਜੇ (!optimizable()) ਵਾਪਸ;
                new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(nodep->thenp()));
            } ਨਹੀਂਤਾਂ {
                iterateConst(nodep->elsep());
                ਜੇ (!optimizable()) ਵਾਪਸ;
                new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(nodep->elsep()));
            }
        }
    }

    ਖਾਲੀ handle_ਸੌਂਪੋ_Element(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੁੱਲ_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> selps;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp = lhsp;
        ਜਦੋਂ (true) {
            ਜੇ (AstArraySel* ਅਚਲ selp = VN_CAST(fromp, ArraySel)) {
                check_ਗੰਢ_Info(selp);
                iterate_And_ਅਗਲਾ_Const_Null(selp->bitp());  // Bit index
                selps.ਧੱਕੋ_ਪਿੱਠ(selp);
                fromp = selp->fromp();
            } ਨਹੀਂਤਾਂ ਜੇ (AstStructSel* ਅਚਲ selp = VN_CAST(fromp, StructSel)) {
                check_ਗੰਢ_Info(selp);
                selps.ਧੱਕੋ_ਪਿੱਠ(selp);
                fromp = selp->fromp();
            } ਨਹੀਂਤਾਂ {
                ਰੋਕੋ;
            }
        }
        AstVarRef* ਅਚਲ varrefp = VN_CAST(fromp, VarRef);
        ਜੇ (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            ਵਾਪਸ;
        }
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        Ast_ਗੰਢ* ਅਚਲ vscp = varOrScope(varrefp);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rootp = fetch_ਬਾਹਰਲਾ_ਮੁੱਲ_Null(vscp);
        ਜੇ (!rootp) rootp = fetch_ਮੁੱਲ_Null(vscp);
        ਜੇ (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            ਵਾਪਸ;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* itemp = rootp;
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ selp : vlstd::ਉਲਟ_view(selps)) {
            ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ਅਚਲ consp = VN_AS(itemp, ConsArray);
                ਅਚਲ uint32_t ਸੂਚਕਾਂਕ = fetchConst(aselp->bitp())->ਮੰਜ਼ਿਲ_UInt();
                ਅਚਲ ਪੂਰਨ elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                ਜੇ (ਸੂਚਕਾਂਕ >= static_cast<uint32_t>(elements)) {
                    ਵਾਪਸ;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(ਸੂਚਕਾਂਕ);
                ਜੇ (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_ਰੁੱਖ(false);
                    consp->valuep(ਸੂਚਕਾਂਕ, itemp);
                }
            } ਨਹੀਂਤਾਂ {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = newTrackedClone(fetch_ਮੁੱਲ(ਮੁੱਲ_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ਧੱਕੋ_ਪਿੱਠ(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        ਸੌਂਪੋ_ਬਾਹਰਲਾ_ਮੁੱਲ(nodep, vscp, rootp);
    }
    ਖਾਲੀ handle_ਸੌਂਪੋ_Sel(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstSel* selp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੁੱਲ_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_ਸੰਖਿਆ lsb{nodep};
        handle_ਸੌਂਪੋ_Sel_Recurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_ਗੰਢ* ਅਚਲ vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        ਜੇ (AstConst* ਅਚਲ vscpnump = fetch_ਬਾਹਰਲਾ_Const_Null(vscp)) {
            outconstp = vscpnump;
        } ਨਹੀਂਤਾਂ ਜੇ (AstConst* ਅਚਲ vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } ਨਹੀਂਤਾਂ {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{},
                                     varrefp->varp()->ਚੌੜਾਈ_Min(), 0};
            ਜੇ (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->ਅੰਕ().ਸੈੱਟ_All_Bits0();
            } ਨਹੀਂਤਾਂ {
                outconstp->ਅੰਕ().ਸੈੱਟ_All_ਬਿੱਟ_X();
            }
            m_reclaimValuesp.emplace_ਪਿੱਠ(outconstp);
        }
        outconstp->ਅੰਕ().opSelInto(fetchConst(ਮੁੱਲ_Fromp)->ਅੰਕ(), lsb, selp->ਚੌੜਾਈ_Const());
        ਸੌਂਪੋ_ਬਾਹਰਲਾ_ਮੁੱਲ(nodep, vscp, outconstp);
    }
    ਖਾਲੀ handle_ਸੌਂਪੋ_Sel_Recurse(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, AstSel* selp, AstVarRef*& ਬਾਹਰਲਾ_Varrefp_Ref,
                                V3_ਸੰਖਿਆ& lsbRef, ਪੂਰਨ ਡੂੰਘਾਈ) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_ਗੰਢ_Info(selp);
        iterate_And_ਅਗਲਾ_Const_Null(selp->lsbp());  // Bit index
        ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(selp->fromp(), VarRef)) {
            ਬਾਹਰਲਾ_Varrefp_Ref = varrefp;
            lsbRef = fetchConst(selp->lsbp())->ਅੰਕ();
            ਵਾਪਸ;  // And presumably still optimizable()
        }
        ਜੇ (AstSel* ਅਚਲ subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_ਸੰਖਿਆ sublsb{nodep};
            handle_ਸੌਂਪੋ_Sel_Recurse(nodep, subselp, ਬਾਹਰਲਾ_Varrefp_Ref, sublsb /*ref*/, ਡੂੰਘਾਈ + 1);
            ਜੇ (optimizable()) {
                lsbRef = sublsb;
                lsbRef.op_ਜੋੜੋ(sublsb, fetchConst(selp->lsbp())->ਅੰਕ());
            }
        } ਨਹੀਂਤਾਂ {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    ਖਾਲੀ handle_ਸੌਂਪੋ_Recurse(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੁੱਲ_Fromp) {
        ਜੇ (!optimizable()) ਵਾਪਸ;
        ਜੇ (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            ਜੇ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ਵਾਪਸ;
            }
            handle_ਸੌਂਪੋ_Element(nodep, lhsp, ਮੁੱਲ_Fromp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstConcat* ਅਚਲ selp = VN_CAST(lhsp, Concat)) {
            check_ਗੰਢ_Info(selp);
            ਜੇ (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                ਵਾਪਸ;
            }
            // Split value into left and right concat values
            ਜੇ (!m_checkOnly) {
                {
                    AstConst* ਅਚਲ outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_ਮੁੱਲ{},
                                       selp->lhsp()->ਚੌੜਾਈ(), 0};
                    outconstp->ਅੰਕ().opSel(fetchConst(ਮੁੱਲ_Fromp)->ਅੰਕ(),
                                           selp->lhsp()->ਚੌੜਾਈ() + selp->rhsp()->ਚੌੜਾਈ() + 1,
                                           selp->rhsp()->ਚੌੜਾਈ());
                    new_ਮੁੱਲ(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ਰੁੱਖ(), outconstp);
                }
                {
                    AstConst* ਅਚਲ outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_ਮੁੱਲ{},
                                       selp->rhsp()->ਚੌੜਾਈ_Min(), 0};
                    outconstp->ਅੰਕ().opSel(fetchConst(ਮੁੱਲ_Fromp)->ਅੰਕ(),
                                           selp->rhsp()->ਚੌੜਾਈ() - 1, 0);
                    new_ਮੁੱਲ(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ਰੁੱਖ(), outconstp);
                }
            }
            handle_ਸੌਂਪੋ_Recurse(nodep, selp->lhsp(), selp->lhsp());
            handle_ਸੌਂਪੋ_Recurse(nodep, selp->rhsp(), selp->rhsp());
        } ਨਹੀਂਤਾਂ ਜੇ (AstReplicate* ਅਚਲ selp = VN_CAST(lhsp, Replicate)) {
            check_ਗੰਢ_Info(selp);
            iterate_And_ਅਗਲਾ_Const_Null(selp->countp());
            AstConst* ਅਚਲ countp = VN_CAST(selp->countp(), Const);
            ਜੇ (!countp || !countp->ਅੰਕ().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                ਵਾਪਸ;
            }
            handle_ਸੌਂਪੋ_Recurse(nodep, selp->srcp(), ਮੁੱਲ_Fromp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(lhsp, Sel)) {
            ਜੇ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ਵਾਪਸ;
            }
            handle_ਸੌਂਪੋ_Sel(nodep, selp, ਮੁੱਲ_Fromp);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(lhsp, VarRef)) {
            ਜੇ (m_checkOnly) {
                iterate_And_ਅਗਲਾ_Const_Null(lhsp);
            } ਨਹੀਂਤਾਂ {
                Ast_ਗੰਢ* ਅਚਲ vscp = varOrScope(VN_CAST(lhsp, VarRef));
                ਸੌਂਪੋ_ਬਾਹਰਲਾ_ਮੁੱਲ(nodep, vscp, fetch_ਮੁੱਲ(ਮੁੱਲ_Fromp));
            }
        } ਨਹੀਂਤਾਂ {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);

        VL_RESTORER(m_ਵਿੱਚ_Dly_ਸੌਂਪੋ);

        ਜੇ (VN_IS(nodep, ਸੌਂਪੋ_Force)) {
            clearOptimizable(nodep, "Force");
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, ਸੌਂਪੋ_Dly)) {
            ਜੇ (m_any_ਸੌਂਪੋ_Comb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_ਸੌਂਪੋ_Dly = true;
            m_ਵਿੱਚ_Dly_ਸੌਂਪੋ = true;
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_any_ਸੌਂਪੋ_Dly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_ਸੌਂਪੋ_Comb = true;
        }

        ਜੇ (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            ਵਾਪਸ;
        }

        iterate_And_ਅਗਲਾ_Const_Null(nodep->rhsp());  // Value to assign
        handle_ਸੌਂਪੋ_Recurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (ਅਚਲ AstConsArray* ਅਚਲ consp = VN_CAST(fetch_ਮੁੱਲ_Null(nodep->fromp()), ConsArray)) {
            ਅਚਲ AstConst* ਅਚਲ indexp = fetchConst(nodep->bitp());
            ਅਚਲ uint32_t ਵਿੱਥ = indexp->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ itemp = consp->valuep(ਵਿੱਥ);
            ਜੇ (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::ਮੰਜ਼ਿਲ_string(ਵਿੱਥ));
            } ਨਹੀਂਤਾਂ {
                ਸੈੱਟ_ਮੁੱਲ(nodep, itemp);
            }
        } ਨਹੀਂਤਾਂ {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        ਜੇ (!m_params) {
            bad_ਗੰਢ_Type(nodep);
            ਵਾਪਸ;
        }
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        AstConsStruct* ਅਚਲ consp = VN_AS(fetch_ਮੁੱਲ(nodep->fromp()), ConsStruct);
        ਸੈੱਟ_ਮੁੱਲ(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    ਖਾਲੀ ਦੇਖੋ(Ast_ਟੁਕੜਾ_Sel* nodep) override {
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        // Fetch the base constant array
        ਜੇ (ਅਚਲ AstConsArray* ਅਚਲ consp = VN_CAST(fetch_ਮੁੱਲ_Null(nodep->fromp()), ConsArray)) {
            ਅਚਲ VNumRange& ਟੁਕੜਾ_Range = nodep->declRange();
            ਅਚਲ uint32_t ਟੁਕੜਾ_Elements = ਟੁਕੜਾ_Range.elements();
            ਅਚਲ ਪੂਰਨ ਟੁਕੜਾ_Lo = ਟੁਕੜਾ_Range.lo();
            // Use this node's dtype for the slice array
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ਅਚਲ newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            ਲਈ (uint32_t idx = 0; idx < ਟੁਕੜਾ_Elements; ++idx) {
                ਅਚਲ uint32_t ਅਧਾਰ_Idx = ਟੁਕੜਾ_Lo + idx;
                newConsp->valuep(idx, consp->valuep(ਅਧਾਰ_Idx)->clone_ਰੁੱਖ(false));
            }
            // Assign the new constant array and track it for later deletion
            ਸੈੱਟ_ਮੁੱਲ(nodep, newConsp);
            m_reclaimValuesp.ਧੱਕੋ_ਪਿੱਠ(newConsp);
        } ਨਹੀਂਤਾਂ {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        UINFO(5, "   CASE " << nodep);
        check_ਗੰਢ_Info(nodep);
        ਜੇ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (optimizable()) {
            iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
            bool ਪ੍ਰਾਪਤੀ = false;
            ਲਈ (Ast_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {
                ਜੇ (!itemp->isDefault()) {
                    ਲਈ (Ast_ਗੰਢ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        ਜੇ (ਪ੍ਰਾਪਤੀ) ਰੋਕੋ;
                        iterate_And_ਅਗਲਾ_Const_Null(ep);
                        ਜੇ (optimizable()) {
                            V3_ਸੰਖਿਆ ਮੇਲ{nodep, 1};
                            ਮੇਲ.opEq(fetchConst(nodep->exprp())->ਅੰਕ(), fetchConst(ep)->ਅੰਕ());
                            ਜੇ (ਮੇਲ.isNeqZero()) {
                                iterate_And_ਅਗਲਾ_Const_Null(itemp->stmtsp());
                                ਪ੍ਰਾਪਤੀ = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            ਲਈ (Ast_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {
                ਜੇ (ਪ੍ਰਾਪਤੀ) ਰੋਕੋ;
                ਜੇ (itemp->isDefault()) {
                    iterate_And_ਅਗਲਾ_Const_Null(itemp->stmtsp());
                    ਪ੍ਰਾਪਤੀ = true;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        // Real handling is in AstNodeCase
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstComment*) override {}

    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        check_ਗੰਢ_Info(nodep);
        iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
        ਜੇ (!optimizable()) ਵਾਪਸ;
        iterate_And_ਅਗਲਾ_Const_Null(nodep->resultp());
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        new_ਮੁੱਲ(nodep, fetch_ਮੁੱਲ(nodep->resultp()));
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        iterateChildrenConst(nodep);
        ਜੇ (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstJumpGo* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        check_ਗੰਢ_Info(nodep);
        ਜੇ (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        ਜੇ (!m_params) {
            bad_ਗੰਢ_Type(nodep);
            ਵਾਪਸ;
        }
        check_ਗੰਢ_Info(nodep);
        ਜੇ (m_checkOnly) {
            iterateChildrenConst(nodep);
            ਵਾਪਸ;
        }
        ਜੇ (!optimizable()) ਵਾਪਸ;

        ਮਾਪ_t iter_ਗਿਣਤੀ = 0;
        ਅਚਲ ਮਾਪ_t iter_ਹੱਦਬੰਦੀ = v3Global.opt.unroll_ਹੱਦਬੰਦੀ();
        ਜਦੋਂ (true) {
            ਜੇ (iter_ਗਿਣਤੀ > iter_ਹੱਦਬੰਦੀ) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::ਮੰਜ਼ਿਲ_string(iter_ਹੱਦਬੰਦੀ));
                ਰੋਕੋ;
            }
            ++iter_ਗਿਣਤੀ;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
            ਜੇ (jumpingOver()) ਰੋਕੋ;
        }

        ਜੇ (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        check_ਗੰਢ_Info(nodep);
        iterateConst(nodep->condp());
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        ਜੇ (fetchConst(nodep->condp())->ਅੰਕ().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstStop* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_ਗੰਢ_Info(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFuncRef* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_ਗੰਢ_Info(nodep);
        ਜੇ (!m_params) {
            bad_ਗੰਢ_Type(nodep);
            ਵਾਪਸ;
        }
        Ast_ਗੰਢ_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        ਜੇ (m_params) V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        ਜੇ (funcp->recursive()) {
            ਜੇ (m_recurse_ਗਿਣਤੀ >= v3Global.opt.func_Recursion_ਡੂੰਘਾਈ()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::ਮੰਜ਼ਿਲ_string(v3Global.opt.func_Recursion_ਡੂੰਘਾਈ())
                                            + " times");
                ਵਾਪਸ;
            }
            ++m_recurse_ਗਿਣਤੀ;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_ਗੰਢ*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> old_ਮੁੱਲਾਂ;

        ਜੇ (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_ਮੁੱਲਾਂ](AstVar* varp) {
                ਜੇ (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = fetch_ਮੁੱਲ_Null(varp)) {
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nvaluep = newTrackedClone(valuep);
                        old_ਮੁੱਲਾਂ.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_ਮੁੱਲਾਂ](Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
                ਜੇ (VN_IS(exprp, Const)) ਵਾਪਸ;  // Speed up as won't change
                ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = fetch_ਮੁੱਲ_Null(exprp)) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nvaluep = newTrackedClone(valuep);
                    old_ਮੁੱਲਾਂ.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::ਸਦਿਸ਼<std::pair<AstVar*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>> ਪੋਰਟ_ਮੁੱਲਾਂ;
        ਲਈ (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.ਸਿਰਾ(); ++it) {
            AstVar* ਅਚਲ portp = it->ਪਹਿਲਾ;
            Ast_ਗੰਢ* ਅਚਲ pinp = it->ਦੂਜਾ->exprp();
            ਜੇ (pinp) {  // Else too few arguments in function call - ignore it
                ਜੇ (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    ਵਾਪਸ;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                ਪੋਰਟ_ਮੁੱਲਾਂ.emplace_ਪਿੱਠ(std::make_pair(portp, newTrackedClone(fetch_ਮੁੱਲ(pinp))));
            }
        }
        // Apply value to the function
        ਜੇ (!m_checkOnly && optimizable())
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : ਪੋਰਟ_ਮੁੱਲਾਂ) {
                ਜੇ (!m_checkOnly && optimizable()) new_ਮੁੱਲ(it.ਪਹਿਲਾ, it.ਦੂਜਾ);
            }
        Sim_ਢੇਰ_ਗੰਢ ਢੇਰ_ਗੰਢ{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&ਢੇਰ_ਗੰਢ);
        ਜੇ (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                ਜੇ (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* returnp = nullptr;
        ਜੇ (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_ਮੁੱਲ(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : old_ਮੁੱਲਾਂ) {
            ਜੇ (it.ਦੂਜਾ) new_ਮੁੱਲ(it.ਪਹਿਲਾ, it.ਦੂਜਾ);
        }
        ਜੇ (returnp) new_ਮੁੱਲ(nodep, returnp);
        ਜੇ (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_ਗਿਣਤੀ > 0, nodep, "recurse underflow");
            --m_recurse_ਗਿਣਤੀ;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!m_params) {
            bad_ਗੰਢ_Type(nodep);
            ਵਾਪਸ;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstScopeName* /*nodep*/) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        // Ignore
    }

    ਖਾਲੀ ਦੇਖੋ(AstSFormatArg* nodep) override {
        check_ਗੰਢ_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_ਅਗਲਾ_Const_Null(nodep->exprp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;

        Ast_ਗੰਢ* ਅਗਲਾ_Argp = nodep->exprsp();
        string ਨਤੀਜਾ;
        ਅਚਲ string format = nodep->ਪਾਠ();
        ਸਵੈਚਾਲਿਤ pos = format.cbegin();
        bool ਵਿੱਚ_Pct = false;
        string ਚੌੜਾਈ;
        ਲਈ (; pos != format.cend(); ++pos) {
            ਜੇ (!ਵਿੱਚ_Pct && pos[0] == '%') {
                ਵਿੱਚ_Pct = true;
                ਚੌੜਾਈ = "";
            } ਨਹੀਂਤਾਂ ਜੇ (!ਵਿੱਚ_Pct) {  // Normal text
                ਨਤੀਜਾ += *pos;
            } ਨਹੀਂਤਾਂ {  // Format character
                ਜੇ (std::ਅੰਕ_ਹੈ(pos[0])) {
                    ਚੌੜਾਈ += pos[0];
                    ਜਾਰੀ;
                }

                ਵਿੱਚ_Pct = false;

                ਜੇ (V3_ਸੰਖਿਆ::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_ਗੰਢ* ਅਚਲ argp = ਅਗਲਾ_Argp;
                    ਅਗਲਾ_Argp = ਅਗਲਾ_Argp->nextp();
                    AstSFormatArg* ਅਚਲ fargp = VN_CAST(argp, SFormatArg);
                    Ast_ਗੰਢ* ਅਚਲ subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ਅਚਲ constp = fetchConstNull(subargp);
                    ਅਚਲ VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    ਜੇ (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        ਰੋਕੋ;
                    }
                    ਅਚਲ string pformat = "%"s + ਚੌੜਾਈ + pos[0];
                    ਨਤੀਜਾ += formatAttr.isEnum()
                                  ? constp->ਅੰਕ().displayedEnum(fargp, pformat)
                                  : constp->ਅੰਕ().displayed(nodep, pformat, formatAttr);
                } ਨਹੀਂਤਾਂ {
                    ਚੋਣ (std::tolower(pos[0])) {
                    ਹਾਲਤ '%': ਨਤੀਜਾ += "%"; ਰੋਕੋ;
                    ਹਾਲਤ 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        ਨਤੀਜਾ += "%m";
                        ਰੋਕੋ;
                    ਮੂਲ: clearOptimizable(nodep, "Unknown $display-like format code."); ਰੋਕੋ;
                    }
                }
            }
        }

        AstConst* ਅਚਲ ਨਤੀਜਾ_Constp = new AstConst{nodep->fileline(), AstConst::String{}, ਨਤੀਜਾ};
        ਸੈੱਟ_ਮੁੱਲ(nodep, ਨਤੀਜਾ_Constp);
        m_reclaimValuesp.ਧੱਕੋ_ਪਿੱਠ(ਨਤੀਜਾ_Constp);
    }

    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_ਗੰਢ_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        ਜੇ (m_params) {
            ਅਚਲ AstConst* ਅਚਲ textp = fetchConst(nodep->fmtp());
            ਚੋਣ (nodep->displayType()) {
            ਹਾਲਤ VDisplayType::DT_DISPLAY:  // FALLTHRU
            ਹਾਲਤ VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); ਰੋਕੋ;
            ਹਾਲਤ VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); ਰੋਕੋ;
            ਹਾਲਤ VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); ਰੋਕੋ;
            ਹਾਲਤ VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); ਰੋਕੋ;
            ਹਾਲਤ VDisplayType::DT_WRITE:  // FALLTHRU
            ਮੂਲ: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੰਜ਼ਿਲ_String_N* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        ਜੇ (!optimizable()) ਵਾਪਸ;  // Accelerate
        check_ਗੰਢ_Info(nodep);
        iterateChildrenConst(nodep);
        ਜੇ (m_checkOnly || !optimizable()) ਵਾਪਸ;
        ਅਚਲ std::string ਨਤੀਜਾ = ਮੰਜ਼ਿਲ_String_Recurse(fetch_ਮੁੱਲ(nodep->lhsp()));
        ਜੇ (!optimizable()) ਵਾਪਸ;
        AstConst* ਅਚਲ ਨਤੀਜਾ_Constp = new AstConst{nodep->fileline(), AstConst::String{}, ਨਤੀਜਾ};
        ਸੈੱਟ_ਮੁੱਲ(nodep, ਨਤੀਜਾ_Constp);
        m_reclaimValuesp.ਧੱਕੋ_ਪਿੱਠ(ਨਤੀਜਾ_Constp);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        ਗਿਆਤ_Bad_ਗੰਢ_Type(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Get_Initial_Random_ਬੀਜ* nodep) override { ਗਿਆਤ_Bad_ਗੰਢ_Type(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override { ਗਿਆਤ_Bad_ਗੰਢ_Type(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSampled* nodep) override { ਗਿਆਤ_Bad_ਗੰਢ_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (jumpingOver()) ਵਾਪਸ;
        bad_ਗੰਢ_Type(nodep);
    }

    // MEMBERS - called by constructor
    ਖਾਲੀ ਸੈੱਟ_ਬਹੁਲਕ(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    ਖਾਲੀ mainGuts(Ast_ਗੰਢ* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        ਸੈੱਟ_ਬਹੁਲਕ(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    ਖਾਲੀ clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_any_ਸੌਂਪੋ_Comb = false;
        m_any_ਸੌਂਪੋ_Dly = false;
        m_ਵਿੱਚ_Dly_ਸੌਂਪੋ = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_ਗਿਣਤੀ = 0;
        m_ਅੰਕੜੇ_ਗਿਣਤੀ = 0;
        m_jumptargetp = nullptr;

        Ast_ਗੰਢ::user1_Clear_ਰੁੱਖ();
        m_varAux.clear();
        ++m_constGeneration;
    }
    ਖਾਲੀ main_ਸਾਰਣੀ_Check(Ast_ਗੰਢ* nodep) {
        ਸੈੱਟ_ਬਹੁਲਕ(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ਖਾਲੀ main_ਸਾਰਣੀ_Emulate(Ast_ਗੰਢ* nodep) {
        ਸੈੱਟ_ਬਹੁਲਕ(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ਖਾਲੀ mainParamEmulate(Ast_ਗੰਢ* nodep) {
        ਸੈੱਟ_ਬਹੁਲਕ(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::ਸੈੱਟ<Ast_ਗੰਢ*> ਅਣਵਰਤਿਆ_Rootps;
        ਲਈ (Ast_ਗੰਢ* ਅਚਲ nodep : m_reclaimValuesp) {
            ਜੇ (!nodep->backp()) ਅਣਵਰਤਿਆ_Rootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        ਲਈ (Ast_ਗੰਢ* ਅਚਲ nodep : ਅਣਵਰਤਿਆ_Rootps) VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
};

#ਜੇ_ਅੰਤ  // Guard
