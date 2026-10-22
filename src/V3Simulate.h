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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Task.h"
#ଅନ୍ତର୍ଭାଵ "V3Width.h"

#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <sstream>
#ଅନ୍ତର୍ଭାଵ <stack>
#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_ଗଦା_ଗଣ୍ଠି final {
ସର୍ବସାଧାରଣ:
    // MEMBERS
    AstFuncRef* ଅଚଳ m_funcp;
    V3TaskConnects* ଅଚଳ m_tconnects;
    // CONSTRUCTORS
    Sim_ଗଦା_ଗଣ୍ଠି(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ଗଦା_ଗଣ୍ଠି() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

class SimulateVisitor VL_NOT_FINAL : ସର୍ବସାଧାରଣ VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

ବ୍ୟକ୍ତିଗତ:
    // CONSTANTS
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା CALL_ଗଦା_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ଅଚଳ VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    ଗଣନା VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    ସଂରଚନା AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = nullptr;
    };

    AstUser1Allocator<Ast_ଗଣ୍ଠି, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    ସଂରଚନା ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, Ast_ଗଣ୍ଠି* nodep) {
            ଯଦି (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            ଯଦି (m_nextFree == m_constps.size()) {
                m_constps.ଠେଲ_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ଅଚଳ constp = m_constps[m_nextFree++];
            constp->ଅଙ୍କ().nodep(nodep);
            ଫେରନ୍ତୁ constp;
        }

        ~ConstAllocator() {
            ପାଇଁ (AstConst* ଅଚଳ constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_ଗଣ୍ଠି* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_instr_ଗଣନା;  ///< Number of nodes
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_data_ଗଣନା;  ///< Bytes of data
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_recurse_ଗଣନା = 0;  // Now deep in current recursion
    Ast_ଗଣ୍ଠି* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି_DType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<Sim_ଗଦା_ଗଣ୍ଠି*> m_call_ଗଦା;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_ଗଣ୍ଠି*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_ସଂଖ୍ୟା(ଅଚଳ V3_ସଂଖ୍ୟା* nump, ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଯଦି (ଅଚଳ AstRefDType* ଅଚଳ refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ stp = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
            ଯଦି (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                ପାଇଁ (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = itemp->width();
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = itemp->lsb();
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lsb + width - 1;
                    V3_ସଂଖ୍ୟା field_ଅଙ୍କ{nump, width};
                    field_ଅଙ୍କ.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ childTypep = itemp->subDTypep()) {
                        out << pretty_ସଂଖ୍ୟା(&field_ଅଙ୍କ, childTypep);
                    } ଅନ୍ୟଥା {
                        out << field_ଅଙ୍କ;
                    }
                    ଯଦି (itemp->nextp()) out << ", ";
                }
                out << "}";
                ଫେରନ୍ତୁ out.str();
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstPackArrayDType* ଅଚଳ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା arrayElements = arrayp->elementsConst();
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା element = 0; element < arrayElements; ++element) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = childTypep->width();
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = width * element;
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb = lsb + width - 1;
                    V3_ସଂଖ୍ୟା field_ଅଙ୍କ{nump, width};
                    field_ଅଙ୍କ.opSel(*nump, msb, lsb);
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_ସଂଖ୍ୟା(&field_ଅଙ୍କ, childTypep);
                    ଯଦି (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                ଫେରନ୍ତୁ out.str();
            }
        }
        ଫେରନ୍ତୁ nump->ascii();
    }

    // Checking METHODS
ସର୍ବସାଧାରଣ:
    /// Call other-this function on all new *non-constant* var references
    ଆଭାସୀ ଶୂନ୍ୟ varRefCb(AstVarRef* nodep) {}

    ଶୂନ୍ୟ clearOptimizable(Ast_ଗଣ୍ଠି* nodep /*null ok*/, ଅଚଳ string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        ଯଦି (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            ଯଦି (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                ଯଦି (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ଗଦା;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0;
            ପାଇଁ (ଅଚଳ auto& callstack : vlstd::reverse_view(m_call_ଗଦା)) {
                ଅଚଳ AstFuncRef* ଅଚଳ funcp = callstack->m_funcp;
                ଗଦା << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                ପାଇଁ (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    ଅଚଳ AstVar* ଅଚଳ portp = conIt->first;
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ pinp = conIt->second->exprp();
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = pinp->dtypep();
                    ଯଦି (AstConst* ଅଚଳ valp = fetchConstNull(pinp)) {
                        ଗଦା << "\n           " << portp->prettyName() << " = "
                              << pretty_ସଂଖ୍ୟା(&valp->ଅଙ୍କ(), dtypep);
                    } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = fetch_ମୂଲ୍ୟ_Null(pinp)) {
                        ଗଦା << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                ଯଦି (++n > CALL_ଗଦା_MAX) {
                    ଗଦା << "\n           ... stack truncated";
                    ଭାଙ୍ଗ;
                }
            }
            m_whyNotOptimizable += ଗଦା.str();
        }
    }
    bool optimizable() ଅଚଳ { ଫେରନ୍ତୁ m_whyNotNodep == nullptr; }
    string whyNotMessage() ଅଚଳ { ଫେରନ୍ତୁ m_whyNotOptimizable; }
    Ast_ଗଣ୍ଠି* whyNotNodep() ଅଚଳ { ଫେରନ୍ତୁ m_whyNotNodep; }

    bool isAssignDly() ଅଚଳ { ଫେରନ୍ତୁ m_anyAssignDly; }
    bool isImpure() ଅଚଳ { ଫେରନ୍ତୁ m_isImpure; }
    bool isCoverage() ଅଚଳ { ଫେରନ୍ତୁ m_isCoverage; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_instr_ଗଣନା; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା data_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_data_ଗଣନା; }

    // Simulation METHODS
ବ୍ୟକ୍ତିଗତ:
    AstConst* allocConst(Ast_ଗଣ୍ଠି* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        ଫେରନ୍ତୁ m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ new_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->ଅଙ୍କ().opAssign(constp->ଅଙ୍କ());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } ଅନ୍ୟଥା ଯଦି (fetch_ମୂଲ୍ୟ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_ମୂଲ୍ୟ(nodep, newTrackedClone(const_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(valuep)));
        }
    }
    ଶୂନ୍ୟ new_Out_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->ଅଙ୍କ().opAssign(constp->ଅଙ୍କ());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } ଅନ୍ୟଥା ଯଦି (fetch_Out_ମୂଲ୍ୟ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_ମୂଲ୍ୟ(nodep, newTrackedClone(const_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(valuep)));
        }
    }

ବ୍ୟକ୍ତିଗତ:
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newTrackedClone(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = nodep->clone_ବୃକ୍ଷ(false);
        m_reclaimValuesp.ଠେଲ_back(newp);
        ଫେରନ୍ତୁ newp;
    }
    AstConst* newConst(Ast_ଗଣ୍ଠି* nodep) {
        // Set a constant value for this node
        ଯଦି (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ଅଚଳ constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            ଫେରନ୍ତୁ constp;
        }
        ଫେରନ୍ତୁ fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_ଗଣ୍ଠି* nodep) {
        // Set a var-output constant value for this node
        ଯଦି (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* ଅଚଳ constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            ଫେରନ୍ତୁ constp;
        }
        ଫେରନ୍ତୁ fetchOutConst(nodep);
    }

ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fetch_ମୂଲ୍ୟ_Null(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ m_varAux(nodep).valuep; }

ବ୍ୟକ୍ତିଗତ:
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fetch_Out_ମୂଲ୍ୟ_Null(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ VN_CAST(fetch_ମୂଲ୍ୟ_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ VN_CAST(fetch_Out_ମୂଲ୍ୟ_Null(nodep), Const);
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fetch_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି* nodep) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = fetch_ମୂଲ୍ୟ_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        ଫେରନ୍ତୁ valuep;
    }
    AstConst* fetchConst(Ast_ଗଣ୍ଠି* nodep) {
        AstConst* ଅଚଳ constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        ଫେରନ୍ତୁ constp;
    }
    AstConst* fetchOutConst(Ast_ଗଣ୍ଠି* nodep) {
        AstConst* ଅଚଳ constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        ଫେରନ୍ତୁ constp;
    }

ସର୍ବସାଧାରଣ:
    V3_ସଂଖ୍ୟା* fetch_ସଂଖ୍ୟା_Null(Ast_ଗଣ୍ଠି* nodep) {
        AstConst* ଅଚଳ constp = fetchConstNull(nodep);
        ଯଦି (constp) ଫେରନ୍ତୁ &constp->ଅଙ୍କ();
        ଫେରନ୍ତୁ nullptr;
    }
    V3_ସଂଖ୍ୟା* fetch_Out_ସଂଖ୍ୟା_Null(Ast_ଗଣ୍ଠି* nodep) {
        AstConst* ଅଚଳ constp = fetchOutConstNull(nodep);
        ଯଦି (constp) ଫେରନ୍ତୁ &constp->ଅଙ୍କ();
        ଫେରନ୍ତୁ nullptr;
    }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ set_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    ଶୂନ୍ୟ set_Out_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    ଶୂନ୍ୟ check_ଗଣ୍ଠି_Info(Ast_ଗଣ୍ଠି* nodep, bool ignorePredict = false) {
        ଯଦି (m_checkOnly) {
            m_instr_ଗଣନା += nodep->instr_ଗଣନା();
            m_data_ଗଣନା += nodep->width();
        }
        ଯଦି (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        ଯଦି (!nodep->isPure()) m_isImpure = true;
    }

    ଶୂନ୍ୟ known_Bad_ଗଣ୍ଠି_Type(Ast_ଗଣ୍ଠି* nodep) {
        // Call for node types we know we can't handle
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    ଶୂନ୍ୟ bad_ଗଣ୍ଠି_Type(Ast_ଗଣ୍ଠି* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            ସ୍ଥିର std::set<VNType> s_typePrinted;
            ଅଚଳ auto pair = s_typePrinted.emplace(nodep->type());
            ଯଦି (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_ଗଣ୍ଠି* varOrScope(AstVarRef* nodep) ଅଚଳ {
        Ast_ଗଣ୍ଠି* vscp;
        ଯଦି (m_scoped) {
            vscp = nodep->varScopep();
        } ଅନ୍ୟଥା {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        ଫେରନ୍ତୁ vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ଅଚଳ { ଫେରନ୍ତୁ m_jumptargetp; }
    ଶୂନ୍ୟ assign_Out_ମୂଲ୍ୟ(ଅଚଳ Ast_ଗଣ୍ଠି_Assign* nodep, Ast_ଗଣ୍ଠି* vscp, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        ଯଦି (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_ମୂଲ୍ୟ(vscp, valuep);
        } ଅନ୍ୟଥା {
            new_ମୂଲ୍ୟ(vscp, valuep);
            set_Out_ମୂଲ୍ୟ(vscp, fetch_ମୂଲ୍ୟ(vscp));
        }
    }

    string toStringRecurse(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep) {
        // Return string representation of a value, or clearOptimizable
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ avaluep = VN_CAST(valuep, Const)) {
            ଫେରନ୍ତୁ "'h"s + avaluep->ଅଙ୍କ().displayed(valuep, "%0x");
        }
        ଯଦି (ଅଚଳ AstConsArray* ଅଚଳ avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            ଫେରନ୍ତୁ result;
        }
        ଯଦି (ଅଚଳ AstConsStruct* ଅଚଳ avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            ପାଇଁ (ଅଚଳ AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            ଫେରନ୍ତୁ result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        ଫେରନ୍ତୁ "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newInitValuep(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ଯଦି (AstUnpackArrayDType* ଅଚଳ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elementp = newInitValuep(nodep, arrayp->subDTypep());
            ଫେରନ୍ତୁ elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ଅଚଳ structp = VN_CAST(dtypep, StructDType);
        ଯଦି (structp && !structp->packed()) {
            AstConsStruct* ଅଚଳ newp = new AstConsStruct{nodep->fileline(), structp};
            ପାଇଁ (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ memberValuep = newInitValuep(nodep, memberp->subDTypep());
                ଯଦି (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_ବୃକ୍ଷ(), newp);
                    ଫେରନ୍ତୁ nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            ଫେରନ୍ତୁ newp;
        }
        ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ basicp = dtypep->basicp()) {
            AstConst* ଅଚଳ constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, basicp->widthMin(), 0};
            ଯଦି (basicp->isZeroInit()) {
                constp->ଅଙ୍କ() = V3_ସଂଖ୍ୟା{nodep, basicp};
            } ଅନ୍ୟଥା {
                constp->ଅଙ୍କ().setAllBitsX();
            }
            ଫେରନ୍ତୁ constp;
        }
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ initVar(AstVar* nodep) {
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_ମୂଲ୍ୟ(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_ବୃକ୍ଷ(), valuep);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_ଗଣ୍ଠି* ଅଚଳ vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        ଯଦି (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), ଗଣ୍ଠି_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        ଯଦି (nodep->access().isWriteOrRW()) {
            ଯଦି (m_inDlyAssign) {
                ଯଦି (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    ଯଦି (m_checkOnly) varRefCb(nodep);
                }
            } ଅନ୍ୟଥା {  // nondly asn
                ଯଦି (!(m_varAux(vscp).usage & VU_LV)) {
                    ଯଦି (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    ଯଦି (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ଯଦି (nodep->access().isReadOrRW()) {
            ଯଦି (!(m_varAux(vscp).usage & VU_RV)) {
                ଯଦି (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                ଅଚଳ bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep
                    = varIsConst ? fetch_ମୂଲ୍ୟ_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                ଯଦି (varIsConst && valuep) {
                    ଯଦି (!m_checkOnly && optimizable()) new_ମୂଲ୍ୟ(vscp, valuep);
                } ଅନ୍ୟଥା {
                    ଯଦି (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = fetch_ମୂଲ୍ୟ_Null(vscp);
            ଯଦି (!valuep) {
                ଯଦି (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } ଅନ୍ୟଥା {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            set_ମୂଲ୍ୟ(nodep, valuep);
        }
    }
    ଶୂନ୍ୟ visit(AstVarXRef* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (m_scoped) {
            bad_ଗଣ୍ଠି_Type(nodep);
            ଫେରନ୍ତୁ;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!m_params) {
            bad_ଗଣ୍ଠି_Type(nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (nodep->dpiImport()) {
            ଯଦି (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        ଯଦି (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialAutomaticStmt* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStaticStmt* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstInitialStatic* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!m_params) {
            bad_ଗଣ୍ଠି_Type(nodep);
            ଫେରନ୍ତୁ;
        }
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        UINFO(5, "   IF " << nodep);
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ଅନ୍ୟଥା {
            iterateAndNextConstNull(nodep->condp());
            ଯଦି (optimizable()) {
                ଯଦି (fetchConst(nodep->condp())->ଅଙ୍କ().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } ଅନ୍ୟଥା {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        new_ମୂଲ୍ୟ(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    ଶୂନ୍ୟ visit(AstConsArray* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        AstConsArray* ଅଚଳ valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_ମୂଲ୍ୟ(nodep->defaultp())->clone_ବୃକ୍ଷ(false) : nullptr};
        ପାଇଁ (ଅଚଳ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_ମୂଲ୍ୟ(itemp->valuep())->clone_ବୃକ୍ଷ(false));
        }
        m_reclaimValuesp.ଠେଲ_back(valuep);
        set_ମୂଲ୍ୟ(nodep, valuep);
    }
    ଶୂନ୍ୟ visit(AstConsStruct* nodep) override {
        ଯଦି (!m_params) {
            bad_ଗଣ୍ଠି_Type(nodep);
            ଫେରନ୍ତୁ;
        }
        check_ଗଣ୍ଠି_Info(nodep);
        ପାଇଁ (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        AstConsStruct* ଅଚଳ valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        ପାଇଁ (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_ମୂଲ୍ୟ(itemp->valuep())->clone_ବୃକ୍ଷ(false)});
        }
        m_reclaimValuesp.ଠେଲ_back(valuep);
        set_ମୂଲ୍ୟ(nodep, valuep);
    }
    ଶୂନ୍ୟ visit(AstCvtUnpackToUnpack* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_ମୂଲ୍ୟ(nodep->fromp())->clone_ବୃକ୍ଷ(false), nodep->dtypep()};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ଠେଲ_back(valuep);
        set_ମୂଲ୍ୟ(nodep, valuep);
    }
    ଶୂନ୍ୟ visit(AstConsArrayItem* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstEnumItemRef* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି* ଅଚଳ valuep = nodep->itemp()->valuep();
        ଯଦି (valuep) {
            iterateAndNextConstNull(valuep);
            ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
            new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(valuep));
        } ଅନ୍ୟଥା {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Uniop* nodep) override {
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        nodep->ସଂଖ୍ୟା_Operate(newConst(nodep)->ଅଙ୍କ(), fetchConst(nodep->lhsp())->ଅଙ୍କ());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    ଶୂନ୍ୟ aggregateEqRecurse(V3_ସଂଖ୍ୟା& resultr, Ast_ଗଣ୍ଠି* nodep, bool caseEq, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp,
                            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp) {
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ lconstp = VN_CAST(lhsp, Const)) {
            ଅଚଳ V3_ସଂଖ୍ୟା& lnum = lconstp->ଅଙ୍କ();
            ଅଚଳ V3_ସଂଖ୍ୟା& rnum = VN_AS(rhsp, Const)->ଅଙ୍କ();
            V3_ସଂଖ୍ୟା eq{nodep, 1};
            ଯଦି (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } ଅନ୍ୟଥା ଯଦି (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } ଅନ୍ୟଥା ଯଦି (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } ଅନ୍ୟଥା {
                eq.opEq(lnum, rnum);
            }
            ଅଚଳ V3_ସଂଖ୍ୟା prev = resultr;
            resultr.opLogAnd(prev, eq);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstConsArray* ଅଚଳ lconsp = VN_CAST(lhsp, ConsArray)) {
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } ଅନ୍ୟଥା {
            ଅଚଳ AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            ପାଇଁ (ଅଚଳ AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    ଶୂନ୍ୟ handleAggregateCompare(Ast_ଗଣ୍ଠି_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ଅଚଳ bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ଅଚଳ bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_ସଂଖ୍ୟା& resultr = newConst(nodep)->ଅଙ୍କ();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_ମୂଲ୍ୟ(nodep->lhsp()),
                           fetch_ମୂଲ୍ୟ(nodep->rhsp()));
        ଯଦି (neq) {
            ଅଚଳ V3_ସଂଖ୍ୟା eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Biop* nodep) override {
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        ଯଦି (!VN_IS(fetch_ମୂଲ୍ୟ(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            ଫେରନ୍ତୁ;
        }
        AstConst* ଅଚଳ valuep = newConst(nodep);
        nodep->ସଂଖ୍ୟା_Operate(newConst(nodep)->ଅଙ୍କ(), fetchConst(nodep->lhsp())->ଅଙ୍କ(),
                             fetchConst(nodep->rhsp())->ଅଙ୍କ());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        ଯଦି (!m_params && VN_IS(nodep, Sel) && valuep->ଅଙ୍କ().isAnyX()) {
            ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ{valuep, valuep->width(), valuep->ଅଙ୍କ()};
            valuep->ଅଙ୍କ().opBitsOne(ଅଙ୍କ);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Triop* nodep) override {
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        nodep->ସଂଖ୍ୟା_Operate(newConst(nodep)->ଅଙ୍କ(), fetchConst(nodep->lhsp())->ଅଙ୍କ(),
                             fetchConst(nodep->rhsp())->ଅଙ୍କ(), fetchConst(nodep->thsp())->ଅଙ୍କ());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Quadop* nodep) override {
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        nodep->ସଂଖ୍ୟା_Operate(newConst(nodep)->ଅଙ୍କ(), fetchConst(nodep->lhsp())->ଅଙ୍କ(),
                             fetchConst(nodep->rhsp())->ଅଙ୍କ(), fetchConst(nodep->thsp())->ଅଙ୍କ(),
                             fetchConst(nodep->fhsp())->ଅଙ୍କ());
    }
    ଶୂନ୍ୟ visit(AstLogAnd* nodep) override {
        // Need to short circuit
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ଅନ୍ୟଥା {
            iterateConst(nodep->lhsp());
            ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
            ଯଦି (fetchConst(nodep->lhsp())->ଅଙ୍କ().isNeqZero()) {
                iterateConst(nodep->rhsp());
                ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
                new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(nodep->rhsp()));
            } ଅନ୍ୟଥା {
                new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(nodep->lhsp()));  // a zero
            }
        }
    }
    ଶୂନ୍ୟ visit(AstLogOr* nodep) override {
        // Need to short circuit
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ଅନ୍ୟଥା {
            iterateConst(nodep->lhsp());
            ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
            ଯଦି (fetchConst(nodep->lhsp())->ଅଙ୍କ().isNeqZero()) {
                new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(nodep->lhsp()));  // a one
            } ଅନ୍ୟଥା {
                iterateConst(nodep->rhsp());
                ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
                new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(nodep->rhsp()));
            }
        }
    }
    ଶୂନ୍ୟ visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ଅନ୍ୟଥା {
            iterateConst(nodep->lhsp());
            ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
            ଯଦି (fetchConst(nodep->lhsp())->ଅଙ୍କ().isEqZero()) {
                ଅଚଳ AstConst cnst{nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 1, 1};  // a one
                new_ମୂଲ୍ୟ(nodep, &cnst);  // a one
            } ଅନ୍ୟଥା {
                iterateConst(nodep->rhsp());
                ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
                new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(nodep->rhsp()));
            }
        }
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ଅନ୍ୟଥା {
            iterateConst(nodep->condp());
            ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
            ଯଦି (fetchConst(nodep->condp())->ଅଙ୍କ().isNeqZero()) {
                iterateConst(nodep->thenp());
                ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
                new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(nodep->thenp()));
            } ଅନ୍ୟଥା {
                iterateConst(nodep->elsep());
                ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
                new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(nodep->elsep()));
            }
        }
    }

    ଶୂନ୍ୟ handleAssignElement(Ast_ଗଣ୍ଠି_Assign* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ମୂଲ୍ୟ_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> selps;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp = lhsp;
        ଯେତେବେଳେ (true) {
            ଯଦି (AstArraySel* ଅଚଳ selp = VN_CAST(fromp, ArraySel)) {
                check_ଗଣ୍ଠି_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.ଠେଲ_back(selp);
                fromp = selp->fromp();
            } ଅନ୍ୟଥା ଯଦି (AstStructSel* ଅଚଳ selp = VN_CAST(fromp, StructSel)) {
                check_ଗଣ୍ଠି_Info(selp);
                selps.ଠେଲ_back(selp);
                fromp = selp->fromp();
            } ଅନ୍ୟଥା {
                ଭାଙ୍ଗ;
            }
        }
        AstVarRef* ଅଚଳ varrefp = VN_CAST(fromp, VarRef);
        ଯଦି (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି* ଅଚଳ vscp = varOrScope(varrefp);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rootp = fetch_Out_ମୂଲ୍ୟ_Null(vscp);
        ଯଦି (!rootp) rootp = fetch_ମୂଲ୍ୟ_Null(vscp);
        ଯଦି (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            ଫେରନ୍ତୁ;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemp = rootp;
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ selp : vlstd::reverse_view(selps)) {
            ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ଅଚଳ consp = VN_AS(itemp, ConsArray);
                ଅଚଳ uint32_t index = fetchConst(aselp->bitp())->toUInt();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                ଯଦି (index >= static_cast<uint32_t>(elements)) {
                    ଫେରନ୍ତୁ;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                ଯଦି (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_ବୃକ୍ଷ(false);
                    consp->valuep(index, itemp);
                }
            } ଅନ୍ୟଥା {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = newTrackedClone(fetch_ମୂଲ୍ୟ(ମୂଲ୍ୟ_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ଠେଲ_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_ମୂଲ୍ୟ(nodep, vscp, rootp);
    }
    ଶୂନ୍ୟ handleAssignSel(Ast_ଗଣ୍ଠି_Assign* nodep, AstSel* selp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ମୂଲ୍ୟ_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_ସଂଖ୍ୟା lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_ଗଣ୍ଠି* ଅଚଳ vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        ଯଦି (AstConst* ଅଚଳ vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } ଅନ୍ୟଥା ଯଦି (AstConst* ଅଚଳ vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } ଅନ୍ୟଥା {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{},
                                     varrefp->varp()->widthMin(), 0};
            ଯଦି (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->ଅଙ୍କ().setAllBits0();
            } ଅନ୍ୟଥା {
                outconstp->ଅଙ୍କ().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->ଅଙ୍କ().opSelInto(fetchConst(ମୂଲ୍ୟ_Fromp)->ଅଙ୍କ(), lsb, selp->widthConst());
        assign_Out_ମୂଲ୍ୟ(nodep, vscp, outconstp);
    }
    ଶୂନ୍ୟ handleAssignSelRecurse(Ast_ଗଣ୍ଠି_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_ସଂଖ୍ୟା& lsbRef, ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_ଗଣ୍ଠି_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->ଅଙ୍କ();
            ଫେରନ୍ତୁ;  // And presumably still optimizable()
        }
        ଯଦି (AstSel* ଅଚଳ subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_ସଂଖ୍ୟା sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            ଯଦି (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->ଅଙ୍କ());
            }
        } ଅନ୍ୟଥା {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    ଶୂନ୍ୟ handleAssignRecurse(Ast_ଗଣ୍ଠି_Assign* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ମୂଲ୍ୟ_Fromp) {
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
        ଯଦି (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            ଯଦି (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ଫେରନ୍ତୁ;
            }
            handleAssignElement(nodep, lhsp, ମୂଲ୍ୟ_Fromp);
        } ଅନ୍ୟଥା ଯଦି (AstConcat* ଅଚଳ selp = VN_CAST(lhsp, Concat)) {
            check_ଗଣ୍ଠି_Info(selp);
            ଯଦି (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                ଫେରନ୍ତୁ;
            }
            // Split value into left and right concat values
            ଯଦି (!m_checkOnly) {
                {
                    AstConst* ଅଚଳ outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{},
                                       selp->lhsp()->width(), 0};
                    outconstp->ଅଙ୍କ().opSel(fetchConst(ମୂଲ୍ୟ_Fromp)->ଅଙ୍କ(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_ମୂଲ୍ୟ(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ବୃକ୍ଷ(), outconstp);
                }
                {
                    AstConst* ଅଚଳ outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->ଅଙ୍କ().opSel(fetchConst(ମୂଲ୍ୟ_Fromp)->ଅଙ୍କ(),
                                           selp->rhsp()->width() - 1, 0);
                    new_ମୂଲ୍ୟ(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ବୃକ୍ଷ(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } ଅନ୍ୟଥା ଯଦି (AstReplicate* ଅଚଳ selp = VN_CAST(lhsp, Replicate)) {
            check_ଗଣ୍ଠି_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* ଅଚଳ countp = VN_CAST(selp->countp(), Const);
            ଯଦି (!countp || !countp->ଅଙ୍କ().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                ଫେରନ୍ତୁ;
            }
            handleAssignRecurse(nodep, selp->srcp(), ମୂଲ୍ୟ_Fromp);
        } ଅନ୍ୟଥା ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(lhsp, Sel)) {
            ଯଦି (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ଫେରନ୍ତୁ;
            }
            handleAssignSel(nodep, selp, ମୂଲ୍ୟ_Fromp);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(lhsp, VarRef)) {
            ଯଦି (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } ଅନ୍ୟଥା {
                Ast_ଗଣ୍ଠି* ଅଚଳ vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_ମୂଲ୍ୟ(nodep, vscp, fetch_ମୂଲ୍ୟ(ମୂଲ୍ୟ_Fromp));
            }
        } ଅନ୍ୟଥା {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        ଯଦି (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, AssignDly)) {
            ଯଦି (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } ଅନ୍ୟଥା {
            ଯଦି (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        ଯଦି (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            ଫେରନ୍ତୁ;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (ଅଚଳ AstConsArray* ଅଚଳ consp = VN_CAST(fetch_ମୂଲ୍ୟ_Null(nodep->fromp()), ConsArray)) {
            ଅଚଳ AstConst* ଅଚଳ indexp = fetchConst(nodep->bitp());
            ଅଚଳ uint32_t offset = indexp->ଅଙ୍କ().toUInt();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ itemp = consp->valuep(offset);
            ଯଦି (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } ଅନ୍ୟଥା {
                set_ମୂଲ୍ୟ(nodep, itemp);
            }
        } ଅନ୍ୟଥା {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        ଯଦି (!m_params) {
            bad_ଗଣ୍ଠି_Type(nodep);
            ଫେରନ୍ତୁ;
        }
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        AstConsStruct* ଅଚଳ consp = VN_AS(fetch_ମୂଲ୍ୟ(nodep->fromp()), ConsStruct);
        set_ମୂଲ୍ୟ(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    ଶୂନ୍ୟ visit(AstSliceSel* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        // Fetch the base constant array
        ଯଦି (ଅଚଳ AstConsArray* ଅଚଳ consp = VN_CAST(fetch_ମୂଲ୍ୟ_Null(nodep->fromp()), ConsArray)) {
            ଅଚଳ VNumRange& sliceRange = nodep->declRange();
            ଅଚଳ uint32_t sliceElements = sliceRange.elements();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ଅଚଳ newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            ପାଇଁ (uint32_t idx = 0; idx < sliceElements; ++idx) {
                ଅଚଳ uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_ବୃକ୍ଷ(false));
            }
            // Assign the new constant array and track it for later deletion
            set_ମୂଲ୍ୟ(nodep, newConsp);
            m_reclaimValuesp.ଠେଲ_back(newConsp);
        } ଅନ୍ୟଥା {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    ଶୂନ୍ୟ visit(AstBegin* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstCase* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        UINFO(5, "   CASE " << nodep);
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ଅନ୍ୟଥା ଯଦି (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            ପାଇଁ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                ଯଦି (!itemp->isDefault()) {
                    ପାଇଁ (Ast_ଗଣ୍ଠି* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        ଯଦି (hit) ଭାଙ୍ଗ;
                        iterateAndNextConstNull(ep);
                        ଯଦି (optimizable()) {
                            V3_ସଂଖ୍ୟା match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->ଅଙ୍କ(), fetchConst(ep)->ଅଙ୍କ());
                            ଯଦି (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            ପାଇଁ (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                ଯଦି (hit) ଭାଙ୍ଗ;
                ଯଦି (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstComment*) override {}

    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ପଦସମଷ୍ଟି_Stmt* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        check_ଗଣ୍ଠି_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
        iterateAndNextConstNull(nodep->resultp());
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        new_ମୂଲ୍ୟ(nodep, fetch_ମୂଲ୍ୟ(nodep->resultp()));
    }

    ଶୂନ୍ୟ visit(AstJumpBlock* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        iterateChildrenConst(nodep);
        ଯଦି (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ଶୂନ୍ୟ visit(AstJumpGo* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        ଯଦି (!m_params) {
            bad_ଗଣ୍ଠି_Type(nodep);
            ଫେରନ୍ତୁ;
        }
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (m_checkOnly) {
            iterateChildrenConst(nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;

        size_t iter_ଗଣନା = 0;
        ଅଚଳ size_t iterLimit = v3Global.opt.unrollLimit();
        ଯେତେବେଳେ (true) {
            ଯଦି (iter_ଗଣନା > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                ଭାଙ୍ଗ;
            }
            ++iter_ଗଣନା;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            ଯଦି (jumpingOver()) ଭାଙ୍ଗ;
        }

        ଯଦି (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ଶୂନ୍ୟ visit(AstLoopTest* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        check_ଗଣ୍ଠି_Info(nodep);
        iterateConst(nodep->condp());
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        ଯଦି (fetchConst(nodep->condp())->ଅଙ୍କ().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    ଶୂନ୍ୟ visit(AstStop* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_ଗଣ୍ଠି_Info(nodep);
    }
    ଶୂନ୍ୟ visit(AstFuncRef* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_ଗଣ୍ଠି_Info(nodep);
        ଯଦି (!m_params) {
            bad_ଗଣ୍ଠି_Type(nodep);
            ଫେରନ୍ତୁ;
        }
        Ast_ଗଣ୍ଠି_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        ଯଦି (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        ଯଦି (funcp->recursive()) {
            ଯଦି (m_recurse_ଗଣନା >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                ଫେରନ୍ତୁ;
            }
            ++m_recurse_ଗଣନା;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_ଗଣ୍ଠି*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> old_ମୂଲ୍ୟଗୁଡ଼ିକ;

        ଯଦି (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_ମୂଲ୍ୟଗୁଡ଼ିକ](AstVar* varp) {
                ଯଦି (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = fetch_ମୂଲ୍ୟ_Null(varp)) {
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nvaluep = newTrackedClone(valuep);
                        old_ମୂଲ୍ୟଗୁଡ଼ିକ.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_ମୂଲ୍ୟଗୁଡ଼ିକ](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
                ଯଦି (VN_IS(exprp, Const)) ଫେରନ୍ତୁ;  // Speed up as won't change
                ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = fetch_ମୂଲ୍ୟ_Null(exprp)) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nvaluep = newTrackedClone(valuep);
                    old_ମୂଲ୍ୟଗୁଡ଼ିକ.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>> port_ମୂଲ୍ୟଗୁଡ଼ିକ;
        ପାଇଁ (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* ଅଚଳ portp = it->first;
            Ast_ଗଣ୍ଠି* ଅଚଳ pinp = it->second->exprp();
            ଯଦି (pinp) {  // Else too few arguments in function call - ignore it
                ଯଦି (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    ଫେରନ୍ତୁ;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_ମୂଲ୍ୟଗୁଡ଼ିକ.emplace_back(std::make_pair(portp, newTrackedClone(fetch_ମୂଲ୍ୟ(pinp))));
            }
        }
        // Apply value to the function
        ଯଦି (!m_checkOnly && optimizable())
            ପାଇଁ (ଅଚଳ auto& it : port_ମୂଲ୍ୟଗୁଡ଼ିକ) {
                ଯଦି (!m_checkOnly && optimizable()) new_ମୂଲ୍ୟ(it.first, it.second);
            }
        Sim_ଗଦା_ଗଣ୍ଠି ଗଦା_ଗଣ୍ଠି{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ଗଦା.ଠେଲ_back(&ଗଦା_ଗଣ୍ଠି);
        ଯଦି (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                ଯଦି (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_ଗଦା.କାଢ଼_back();
        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* returnp = nullptr;
        ଯଦି (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_ମୂଲ୍ୟ(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        ପାଇଁ (ଅଚଳ auto& it : old_ମୂଲ୍ୟଗୁଡ଼ିକ) {
            ଯଦି (it.second) new_ମୂଲ୍ୟ(it.first, it.second);
        }
        ଯଦି (returnp) new_ମୂଲ୍ୟ(nodep, returnp);
        ଯଦି (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_ଗଣନା > 0, nodep, "recurse underflow");
            --m_recurse_ଗଣନା;
        }
    }

    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!m_params) {
            bad_ଗଣ୍ଠି_Type(nodep);
            ଫେରନ୍ତୁ;
        }
    }

    ଶୂନ୍ୟ visit(AstScopeName* /*nodep*/) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        // Ignore
    }

    ଶୂନ୍ୟ visit(AstSFormatArg* nodep) override {
        check_ଗଣ୍ଠି_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;

        Ast_ଗଣ୍ଠି* nextArgp = nodep->exprsp();
        string result;
        ଅଚଳ string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        ପାଇଁ (; pos != format.cend(); ++pos) {
            ଯଦି (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } ଅନ୍ୟଥା ଯଦି (!inPct) {  // Normal text
                result += *pos;
            } ଅନ୍ୟଥା {  // Format character
                ଯଦି (std::isdigit(pos[0])) {
                    width += pos[0];
                    ଜାରି;
                }

                inPct = false;

                ଯଦି (V3_ସଂଖ୍ୟା::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* ଅଚଳ fargp = VN_CAST(argp, SFormatArg);
                    Ast_ଗଣ୍ଠି* ଅଚଳ subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ଅଚଳ constp = fetchConstNull(subargp);
                    ଅଚଳ VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    ଯଦି (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        ଭାଙ୍ଗ;
                    }
                    ଅଚଳ string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->ଅଙ୍କ().displayedEnum(fargp, pformat)
                                  : constp->ଅଙ୍କ().displayed(nodep, pformat, formatAttr);
                } ଅନ୍ୟଥା {
                    ବାଛ (std::tolower(pos[0])) {
                    ସ୍ଥିତି '%': result += "%"; ଭାଙ୍ଗ;
                    ସ୍ଥିତି 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        ଭାଙ୍ଗ;
                    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: clearOptimizable(nodep, "Unknown $display-like format code."); ଭାଙ୍ଗ;
                    }
                }
            }
        }

        AstConst* ଅଚଳ resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_ମୂଲ୍ୟ(nodep, resultConstp);
        m_reclaimValuesp.ଠେଲ_back(resultConstp);
    }

    ଶୂନ୍ୟ visit(AstDisplay* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_ଗଣ୍ଠି_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        ଯଦି (m_params) {
            ଅଚଳ AstConst* ଅଚଳ textp = fetchConst(nodep->fmtp());
            ବାଛ (nodep->displayType()) {
            ସ୍ଥିତି VDisplayType::DT_DISPLAY:  // FALLTHRU
            ସ୍ଥିତି VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); ଭାଙ୍ଗ;
            ସ୍ଥିତି VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); ଭାଙ୍ଗ;
            ସ୍ଥିତି VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); ଭାଙ୍ଗ;
            ସ୍ଥିତି VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); ଭାଙ୍ଗ;
            ସ୍ଥିତି VDisplayType::DT_WRITE:  // FALLTHRU
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    ଶୂନ୍ୟ visit(AstToStringN* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;  // Accelerate
        check_ଗଣ୍ଠି_Info(nodep);
        iterateChildrenConst(nodep);
        ଯଦି (m_checkOnly || !optimizable()) ଫେରନ୍ତୁ;
        ଅଚଳ std::string result = toStringRecurse(fetch_ମୂଲ୍ୟ(nodep->lhsp()));
        ଯଦି (!optimizable()) ଫେରନ୍ତୁ;
        AstConst* ଅଚଳ resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_ମୂଲ୍ୟ(nodep, resultConstp);
        m_reclaimValuesp.ଠେଲ_back(resultConstp);
    }

    ଶୂନ୍ୟ visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_ଗଣ୍ଠି_Type(nodep);
    }
    ଶୂନ୍ୟ visit(AstGetInitialRandomSeed* nodep) override { known_Bad_ଗଣ୍ଠି_Type(nodep); }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override { known_Bad_ଗଣ୍ଠି_Type(nodep); }
    ଶୂନ୍ୟ visit(AstSampled* nodep) override { known_Bad_ଗଣ୍ଠି_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (jumpingOver()) ଫେରନ୍ତୁ;
        bad_ଗଣ୍ଠି_Type(nodep);
    }

    // MEMBERS - called by constructor
    ଶୂନ୍ୟ setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    ଶୂନ୍ୟ mainGuts(Ast_ଗଣ୍ଠି* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    ଶୂନ୍ୟ clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_ଗଣନା = 0;
        m_data_ଗଣନା = 0;
        m_jumptargetp = nullptr;

        Ast_ଗଣ୍ଠି::user1_Clear_ବୃକ୍ଷ();
        m_varAux.clear();
        ++m_constGeneration;
    }
    ଶୂନ୍ୟ mainTableCheck(Ast_ଗଣ୍ଠି* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ଶୂନ୍ୟ mainTableEmulate(Ast_ଗଣ୍ଠି* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ଶୂନ୍ୟ mainParamEmulate(Ast_ଗଣ୍ଠି* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_ଗଣ୍ଠି*> unusedRootps;
        ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ nodep : m_reclaimValuesp) {
            ଯଦି (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
};

#endif  // Guard
