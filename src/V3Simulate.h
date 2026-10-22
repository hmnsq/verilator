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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3AstUserAllocator.h"
#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3Task.h"
#অন্তর্ভাৱ "V3Width.h"

#অন্তর্ভাৱ <deque>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <sstream>
#অন্তর্ভাৱ <stack>
#অন্তর্ভাৱ <string>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_স্তূপ_গ্ৰন্থি final {
সার্ৱজনিক:
    // MEMBERS
    AstFuncRef* ধ্রুৱ m_funcp;
    V3TaskConnects* ধ্রুৱ m_tconnects;
    // CONSTRUCTORS
    Sim_স্তূপ_গ্ৰন্থি(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_স্তূপ_গ্ৰন্থি() = স্ৱভাৱ;
};

class SimulateVisitor VL_NOT_FINAL : সার্ৱজনিক VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

গুপ্ত:
    // CONSTANTS
    স্থির constexpr পূর্ণাঙ্ক CALL_স্তূপ_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ধ্রুৱ VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    গণনা VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    সংরচনা AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_গ্ৰন্থি_ৰাশিমালা* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_গ্ৰন্থি_ৰাশিমালা* valuep = nullptr;
    };

    AstUser1Allocator<Ast_গ্ৰন্থি, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    সংরচনা ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, Ast_গ্ৰন্থি* nodep) {
            যদি (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            যদি (m_nextFree == m_constps.size()) {
                m_constps.ঠেলো_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ধ্রুৱ constp = m_constps[m_nextFree++];
            constp->অঙ্ক().nodep(nodep);
            ঘূৰাই constp;
        }

        ~ConstAllocator() {
            বাবে (AstConst* ধ্রুৱ constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_গ্ৰন্থি* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    পূর্ণাঙ্ক m_instr_গুনতি;  ///< Number of nodes
    পূর্ণাঙ্ক m_data_গুনতি;  ///< Bytes of data
    পূর্ণাঙ্ক m_recurse_গুনতি = 0;  // Now deep in current recursion
    Ast_গ্ৰন্থি* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ধ্রুৱ Ast_গ্ৰন্থি_DType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<Sim_স্তূপ_গ্ৰন্থি*> m_call_স্তূপ;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_গ্ৰন্থি*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_সংখ্যা(ধ্রুৱ V3_সংখ্যা* nump, ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep) {
        যদি (ধ্রুৱ AstRefDType* ধ্রুৱ refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ stp = VN_CAST(dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
            যদি (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                বাবে (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ধ্রুৱ পূর্ণাঙ্ক width = itemp->width();
                    ধ্রুৱ পূর্ণাঙ্ক lsb = itemp->lsb();
                    ধ্রুৱ পূর্ণাঙ্ক msb = lsb + width - 1;
                    V3_সংখ্যা field_অঙ্ক{nump, width};
                    field_অঙ্ক.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    যদি (ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ childTypep = itemp->subDTypep()) {
                        out << pretty_সংখ্যা(&field_অঙ্ক, childTypep);
                    } অন্যথা {
                        out << field_অঙ্ক;
                    }
                    যদি (itemp->nextp()) out << ", ";
                }
                out << "}";
                ঘূৰাই out.str();
            }
        } অন্যথা যদি (ধ্রুৱ AstPackArrayDType* ধ্রুৱ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            যদি (ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                ধ্রুৱ পূর্ণাঙ্ক arrayElements = arrayp->elementsConst();
                বাবে (পূর্ণাঙ্ক element = 0; element < arrayElements; ++element) {
                    ধ্রুৱ পূর্ণাঙ্ক width = childTypep->width();
                    ধ্রুৱ পূর্ণাঙ্ক lsb = width * element;
                    ধ্রুৱ পূর্ণাঙ্ক msb = lsb + width - 1;
                    V3_সংখ্যা field_অঙ্ক{nump, width};
                    field_অঙ্ক.opSel(*nump, msb, lsb);
                    ধ্রুৱ পূর্ণাঙ্ক arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_সংখ্যা(&field_অঙ্ক, childTypep);
                    যদি (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                ঘূৰাই out.str();
            }
        }
        ঘূৰাই nump->ascii();
    }

    // Checking METHODS
সার্ৱজনিক:
    /// Call other-this function on all new *non-constant* var references
    আভাস শূন্য varRefCb(AstVarRef* nodep) {}

    শূন্য clearOptimizable(Ast_গ্ৰন্থি* nodep /*null ok*/, ধ্রুৱ string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        যদি (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            যদি (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                যদি (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream স্তূপ;
            পূর্ণাঙ্ক n = 0;
            বাবে (ধ্রুৱ auto& callstack : vlstd::reverse_view(m_call_স্তূপ)) {
                ধ্রুৱ AstFuncRef* ধ্রুৱ funcp = callstack->m_funcp;
                স্তূপ << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                বাবে (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    ধ্রুৱ AstVar* ধ্রুৱ portp = conIt->first;
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ pinp = conIt->second->exprp();
                    ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = pinp->dtypep();
                    যদি (AstConst* ধ্রুৱ valp = fetchConstNull(pinp)) {
                        স্তূপ << "\n           " << portp->prettyName() << " = "
                              << pretty_সংখ্যা(&valp->অঙ্ক(), dtypep);
                    } অন্যথা যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = fetch_মান_Null(pinp)) {
                        স্তূপ << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                যদি (++n > CALL_স্তূপ_MAX) {
                    স্তূপ << "\n           ... stack truncated";
                    ভঙ্গ;
                }
            }
            m_whyNotOptimizable += স্তূপ.str();
        }
    }
    bool optimizable() ধ্রুৱ { ঘূৰাই m_whyNotNodep == nullptr; }
    string whyNotMessage() ধ্রুৱ { ঘূৰাই m_whyNotOptimizable; }
    Ast_গ্ৰন্থি* whyNotNodep() ধ্রুৱ { ঘূৰাই m_whyNotNodep; }

    bool isAssignDly() ধ্রুৱ { ঘূৰাই m_anyAssignDly; }
    bool isImpure() ধ্রুৱ { ঘূৰাই m_isImpure; }
    bool isCoverage() ধ্রুৱ { ঘূৰাই m_isCoverage; }
    পূর্ণাঙ্ক instr_গুনতি() ধ্রুৱ { ঘূৰাই m_instr_গুনতি; }
    পূর্ণাঙ্ক data_গুনতি() ধ্রুৱ { ঘূৰাই m_data_গুনতি; }

    // Simulation METHODS
গুপ্ত:
    AstConst* allocConst(Ast_গ্ৰন্থি* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        ঘূৰাই m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

সার্ৱজনিক:
    শূন্য new_মান(Ast_গ্ৰন্থি* nodep, ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->অঙ্ক().opAssign(constp->অঙ্ক());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } অন্যথা যদি (fetch_মান_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_মান(nodep, newTrackedClone(const_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(valuep)));
        }
    }
    শূন্য new_Out_মান(Ast_গ্ৰন্থি* nodep, ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->অঙ্ক().opAssign(constp->অঙ্ক());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } অন্যথা যদি (fetch_Out_মান_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_মান(nodep, newTrackedClone(const_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(valuep)));
        }
    }

গুপ্ত:
    Ast_গ্ৰন্থি_ৰাশিমালা* newTrackedClone(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = nodep->clone_বৃক্ষ(false);
        m_reclaimValuesp.ঠেলো_back(newp);
        ঘূৰাই newp;
    }
    AstConst* newConst(Ast_গ্ৰন্থি* nodep) {
        // Set a constant value for this node
        যদি (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ধ্রুৱ constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            ঘূৰাই constp;
        }
        ঘূৰাই fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_গ্ৰন্থি* nodep) {
        // Set a var-output constant value for this node
        যদি (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* ধ্রুৱ constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            ঘূৰাই constp;
        }
        ঘূৰাই fetchOutConst(nodep);
    }

সার্ৱজনিক:
    Ast_গ্ৰন্থি_ৰাশিমালা* fetch_মান_Null(Ast_গ্ৰন্থি* nodep) { ঘূৰাই m_varAux(nodep).valuep; }

গুপ্ত:
    Ast_গ্ৰন্থি_ৰাশিমালা* fetch_Out_মান_Null(Ast_গ্ৰন্থি* nodep) { ঘূৰাই m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_গ্ৰন্থি* nodep) { ঘূৰাই VN_CAST(fetch_মান_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_গ্ৰন্থি* nodep) {
        ঘূৰাই VN_CAST(fetch_Out_মান_Null(nodep), Const);
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* fetch_মান(Ast_গ্ৰন্থি* nodep) {
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = fetch_মান_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        ঘূৰাই valuep;
    }
    AstConst* fetchConst(Ast_গ্ৰন্থি* nodep) {
        AstConst* ধ্রুৱ constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        ঘূৰাই constp;
    }
    AstConst* fetchOutConst(Ast_গ্ৰন্থি* nodep) {
        AstConst* ধ্রুৱ constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        ঘূৰাই constp;
    }

সার্ৱজনিক:
    V3_সংখ্যা* fetch_সংখ্যা_Null(Ast_গ্ৰন্থি* nodep) {
        AstConst* ধ্রুৱ constp = fetchConstNull(nodep);
        যদি (constp) ঘূৰাই &constp->অঙ্ক();
        ঘূৰাই nullptr;
    }
    V3_সংখ্যা* fetch_Out_সংখ্যা_Null(Ast_গ্ৰন্থি* nodep) {
        AstConst* ধ্রুৱ constp = fetchOutConstNull(nodep);
        যদি (constp) ঘূৰাই &constp->অঙ্ক();
        ঘূৰাই nullptr;
    }

গুপ্ত:
    শূন্য set_মান(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    শূন্য set_Out_মান(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    শূন্য check_গ্ৰন্থি_Info(Ast_গ্ৰন্থি* nodep, bool ignorePredict = false) {
        যদি (m_checkOnly) {
            m_instr_গুনতি += nodep->instr_গুনতি();
            m_data_গুনতি += nodep->width();
        }
        যদি (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        যদি (!nodep->isPure()) m_isImpure = true;
    }

    শূন্য known_Bad_গ্ৰন্থি_Type(Ast_গ্ৰন্থি* nodep) {
        // Call for node types we know we can't handle
        যদি (jumpingOver()) ঘূৰাই;
        check_গ্ৰন্থি_Info(nodep);
        যদি (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    শূন্য bad_গ্ৰন্থি_Type(Ast_গ্ৰন্থি* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_গ্ৰন্থি_Info(nodep);
        যদি (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            স্থির std::set<VNType> s_typePrinted;
            ধ্রুৱ auto pair = s_typePrinted.emplace(nodep->type());
            যদি (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_গ্ৰন্থি* varOrScope(AstVarRef* nodep) ধ্রুৱ {
        Ast_গ্ৰন্থি* vscp;
        যদি (m_scoped) {
            vscp = nodep->varScopep();
        } অন্যথা {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        ঘূৰাই vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ধ্রুৱ { ঘূৰাই m_jumptargetp; }
    শূন্য assign_Out_মান(ধ্রুৱ Ast_গ্ৰন্থি_Assign* nodep, Ast_গ্ৰন্থি* vscp, ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        যদি (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_মান(vscp, valuep);
        } অন্যথা {
            new_মান(vscp, valuep);
            set_Out_মান(vscp, fetch_মান(vscp));
        }
    }

    string toStringRecurse(Ast_গ্ৰন্থি_ৰাশিমালা* valuep) {
        // Return string representation of a value, or clearOptimizable
        যদি (ধ্রুৱ AstConst* ধ্রুৱ avaluep = VN_CAST(valuep, Const)) {
            ঘূৰাই "'h"s + avaluep->অঙ্ক().displayed(valuep, "%0x");
        }
        যদি (ধ্রুৱ AstConsArray* ধ্রুৱ avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            বাবে (পূর্ণাঙ্ক n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            ঘূৰাই result;
        }
        যদি (ধ্রুৱ AstConsStruct* ধ্রুৱ avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            বাবে (ধ্রুৱ AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            ঘূৰাই result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        ঘূৰাই "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_গ্ৰন্থি_ৰাশিমালা* newInitValuep(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (AstUnpackArrayDType* ধ্রুৱ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elementp = newInitValuep(nodep, arrayp->subDTypep());
            ঘূৰাই elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ধ্রুৱ structp = VN_CAST(dtypep, StructDType);
        যদি (structp && !structp->packed()) {
            AstConsStruct* ধ্রুৱ newp = new AstConsStruct{nodep->fileline(), structp};
            বাবে (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ memberValuep = newInitValuep(nodep, memberp->subDTypep());
                যদি (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_বৃক্ষ(), newp);
                    ঘূৰাই nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            ঘূৰাই newp;
        }
        যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = dtypep->basicp()) {
            AstConst* ধ্রুৱ constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_মান{}, basicp->widthMin(), 0};
            যদি (basicp->isZeroInit()) {
                constp->অঙ্ক() = V3_সংখ্যা{nodep, basicp};
            } অন্যথা {
                constp->অঙ্ক().setAllBitsX();
            }
            ঘূৰাই constp;
        }
        ঘূৰাই nullptr;
    }
    শূন্য initVar(AstVar* nodep) {
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_মান(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_বৃক্ষ(), valuep);
        }
    }

    // VISITORS
    শূন্য visit(AstAlways* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    শূন্য visit(AstVarRef* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_গ্ৰন্থি* ধ্রুৱ vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        যদি (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), গ্ৰন্থি_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        যদি (nodep->access().isWriteOrRW()) {
            যদি (m_inDlyAssign) {
                যদি (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    যদি (m_checkOnly) varRefCb(nodep);
                }
            } অন্যথা {  // nondly asn
                যদি (!(m_varAux(vscp).usage & VU_LV)) {
                    যদি (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    যদি (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        যদি (nodep->access().isReadOrRW()) {
            যদি (!(m_varAux(vscp).usage & VU_RV)) {
                যদি (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                ধ্রুৱ bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep
                    = varIsConst ? fetch_মান_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                যদি (varIsConst && valuep) {
                    যদি (!m_checkOnly && optimizable()) new_মান(vscp, valuep);
                } অন্যথা {
                    যদি (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_গ্ৰন্থি_ৰাশিমালা* valuep = fetch_মান_Null(vscp);
            যদি (!valuep) {
                যদি (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } অন্যথা {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            set_মান(nodep, valuep);
        }
    }
    শূন্য visit(AstVarXRef* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (m_scoped) {
            bad_গ্ৰন্থি_Type(nodep);
            ঘূৰাই;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!m_params) {
            bad_গ্ৰন্থি_Type(nodep);
            ঘূৰাই;
        }
        যদি (nodep->dpiImport()) {
            যদি (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        যদি (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstInitialAutomaticStmt* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstInitialStaticStmt* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstInitialStatic* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!m_params) {
            bad_গ্ৰন্থি_Type(nodep);
            ঘূৰাই;
        }
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        UINFO(5, "   IF " << nodep);
        check_গ্ৰন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateAndNextConstNull(nodep->condp());
            যদি (optimizable()) {
                যদি (fetchConst(nodep->condp())->অঙ্ক().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } অন্যথা {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    শূন্য visit(AstConst* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        new_মান(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    শূন্য visit(AstConsArray* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        AstConsArray* ধ্রুৱ valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_মান(nodep->defaultp())->clone_বৃক্ষ(false) : nullptr};
        বাবে (ধ্রুৱ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_মান(itemp->valuep())->clone_বৃক্ষ(false));
        }
        m_reclaimValuesp.ঠেলো_back(valuep);
        set_মান(nodep, valuep);
    }
    শূন্য visit(AstConsStruct* nodep) override {
        যদি (!m_params) {
            bad_গ্ৰন্থি_Type(nodep);
            ঘূৰাই;
        }
        check_গ্ৰন্থি_Info(nodep);
        বাবে (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        AstConsStruct* ধ্রুৱ valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        বাবে (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_মান(itemp->valuep())->clone_বৃক্ষ(false)});
        }
        m_reclaimValuesp.ঠেলো_back(valuep);
        set_মান(nodep, valuep);
    }
    শূন্য visit(AstCvtUnpackToUnpack* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_মান(nodep->fromp())->clone_বৃক্ষ(false), nodep->dtypep()};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ঠেলো_back(valuep);
        set_মান(nodep, valuep);
    }
    শূন্য visit(AstConsArrayItem* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstEnumItemRef* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        Ast_গ্ৰন্থি* ধ্রুৱ valuep = nodep->itemp()->valuep();
        যদি (valuep) {
            iterateAndNextConstNull(valuep);
            যদি (!optimizable()) ঘূৰাই;
            new_মান(nodep, fetch_মান(valuep));
        } অন্যথা {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Uniop* nodep) override {
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        nodep->সংখ্যা_Operate(newConst(nodep)->অঙ্ক(), fetchConst(nodep->lhsp())->অঙ্ক());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    শূন্য aggregateEqRecurse(V3_সংখ্যা& resultr, Ast_গ্ৰন্থি* nodep, bool caseEq, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp,
                            Ast_গ্ৰন্থি_ৰাশিমালা* rhsp) {
        যদি (ধ্রুৱ AstConst* ধ্রুৱ lconstp = VN_CAST(lhsp, Const)) {
            ধ্রুৱ V3_সংখ্যা& lnum = lconstp->অঙ্ক();
            ধ্রুৱ V3_সংখ্যা& rnum = VN_AS(rhsp, Const)->অঙ্ক();
            V3_সংখ্যা eq{nodep, 1};
            যদি (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } অন্যথা যদি (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } অন্যথা যদি (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } অন্যথা {
                eq.opEq(lnum, rnum);
            }
            ধ্রুৱ V3_সংখ্যা prev = resultr;
            resultr.opLogAnd(prev, eq);
        } অন্যথা যদি (ধ্রুৱ AstConsArray* ধ্রুৱ lconsp = VN_CAST(lhsp, ConsArray)) {
            ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            বাবে (পূর্ণাঙ্ক n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } অন্যথা {
            ধ্রুৱ AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            বাবে (ধ্রুৱ AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    শূন্য handleAggregateCompare(Ast_গ্ৰন্থি_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ধ্রুৱ bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ধ্রুৱ bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_সংখ্যা& resultr = newConst(nodep)->অঙ্ক();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_মান(nodep->lhsp()),
                           fetch_মান(nodep->rhsp()));
        যদি (neq) {
            ধ্রুৱ V3_সংখ্যা eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Biop* nodep) override {
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        যদি (!VN_IS(fetch_মান(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            ঘূৰাই;
        }
        AstConst* ধ্রুৱ valuep = newConst(nodep);
        nodep->সংখ্যা_Operate(newConst(nodep)->অঙ্ক(), fetchConst(nodep->lhsp())->অঙ্ক(),
                             fetchConst(nodep->rhsp())->অঙ্ক());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        যদি (!m_params && VN_IS(nodep, Sel) && valuep->অঙ্ক().isAnyX()) {
            ধ্রুৱ V3_সংখ্যা অঙ্ক{valuep, valuep->width(), valuep->অঙ্ক()};
            valuep->অঙ্ক().opBitsOne(অঙ্ক);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Triop* nodep) override {
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        nodep->সংখ্যা_Operate(newConst(nodep)->অঙ্ক(), fetchConst(nodep->lhsp())->অঙ্ক(),
                             fetchConst(nodep->rhsp())->অঙ্ক(), fetchConst(nodep->thsp())->অঙ্ক());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Quadop* nodep) override {
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        nodep->সংখ্যা_Operate(newConst(nodep)->অঙ্ক(), fetchConst(nodep->lhsp())->অঙ্ক(),
                             fetchConst(nodep->rhsp())->অঙ্ক(), fetchConst(nodep->thsp())->অঙ্ক(),
                             fetchConst(nodep->fhsp())->অঙ্ক());
    }
    শূন্য visit(AstLogAnd* nodep) override {
        // Need to short circuit
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateConst(nodep->lhsp());
            যদি (!optimizable()) ঘূৰাই;
            যদি (fetchConst(nodep->lhsp())->অঙ্ক().isNeqZero()) {
                iterateConst(nodep->rhsp());
                যদি (!optimizable()) ঘূৰাই;
                new_মান(nodep, fetch_মান(nodep->rhsp()));
            } অন্যথা {
                new_মান(nodep, fetch_মান(nodep->lhsp()));  // a zero
            }
        }
    }
    শূন্য visit(AstLogOr* nodep) override {
        // Need to short circuit
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateConst(nodep->lhsp());
            যদি (!optimizable()) ঘূৰাই;
            যদি (fetchConst(nodep->lhsp())->অঙ্ক().isNeqZero()) {
                new_মান(nodep, fetch_মান(nodep->lhsp()));  // a one
            } অন্যথা {
                iterateConst(nodep->rhsp());
                যদি (!optimizable()) ঘূৰাই;
                new_মান(nodep, fetch_মান(nodep->rhsp()));
            }
        }
    }
    শূন্য visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateConst(nodep->lhsp());
            যদি (!optimizable()) ঘূৰাই;
            যদি (fetchConst(nodep->lhsp())->অঙ্ক().isEqZero()) {
                ধ্রুৱ AstConst cnst{nodep->fileline(), AstConst::Widthed_মান{}, 1, 1};  // a one
                new_মান(nodep, &cnst);  // a one
            } অন্যথা {
                iterateConst(nodep->rhsp());
                যদি (!optimizable()) ঘূৰাই;
                new_মান(nodep, fetch_মান(nodep->rhsp()));
            }
        }
    }
    শূন্য visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateConst(nodep->condp());
            যদি (!optimizable()) ঘূৰাই;
            যদি (fetchConst(nodep->condp())->অঙ্ক().isNeqZero()) {
                iterateConst(nodep->thenp());
                যদি (!optimizable()) ঘূৰাই;
                new_মান(nodep, fetch_মান(nodep->thenp()));
            } অন্যথা {
                iterateConst(nodep->elsep());
                যদি (!optimizable()) ঘূৰাই;
                new_মান(nodep, fetch_মান(nodep->elsep()));
            }
        }
    }

    শূন্য handleAssignElement(Ast_গ্ৰন্থি_Assign* nodep, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, Ast_গ্ৰন্থি_ৰাশিমালা* মান_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> selps;
        Ast_গ্ৰন্থি_ৰাশিমালা* fromp = lhsp;
        যেতিয়ালৈ (true) {
            যদি (AstArraySel* ধ্রুৱ selp = VN_CAST(fromp, ArraySel)) {
                check_গ্ৰন্থি_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.ঠেলো_back(selp);
                fromp = selp->fromp();
            } অন্যথা যদি (AstStructSel* ধ্রুৱ selp = VN_CAST(fromp, StructSel)) {
                check_গ্ৰন্থি_Info(selp);
                selps.ঠেলো_back(selp);
                fromp = selp->fromp();
            } অন্যথা {
                ভঙ্গ;
            }
        }
        AstVarRef* ধ্রুৱ varrefp = VN_CAST(fromp, VarRef);
        যদি (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            ঘূৰাই;
        }
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        Ast_গ্ৰন্থি* ধ্রুৱ vscp = varOrScope(varrefp);
        Ast_গ্ৰন্থি_ৰাশিমালা* rootp = fetch_Out_মান_Null(vscp);
        যদি (!rootp) rootp = fetch_মান_Null(vscp);
        যদি (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            ঘূৰাই;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_গ্ৰন্থি_ৰাশিমালা* itemp = rootp;
        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ selp : vlstd::reverse_view(selps)) {
            যদি (ধ্রুৱ AstArraySel* ধ্রুৱ aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ধ্রুৱ consp = VN_AS(itemp, ConsArray);
                ধ্রুৱ uint32_t index = fetchConst(aselp->bitp())->toUInt();
                ধ্রুৱ পূর্ণাঙ্ক elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                যদি (index >= static_cast<uint32_t>(elements)) {
                    ঘূৰাই;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                যদি (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_বৃক্ষ(false);
                    consp->valuep(index, itemp);
                }
            } অন্যথা {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = newTrackedClone(fetch_মান(মান_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ঠেলো_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_মান(nodep, vscp, rootp);
    }
    শূন্য handleAssignSel(Ast_গ্ৰন্থি_Assign* nodep, AstSel* selp, Ast_গ্ৰন্থি_ৰাশিমালা* মান_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_সংখ্যা lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_গ্ৰন্থি* ধ্রুৱ vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        যদি (AstConst* ধ্রুৱ vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } অন্যথা যদি (AstConst* ধ্রুৱ vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } অন্যথা {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_মান{},
                                     varrefp->varp()->widthMin(), 0};
            যদি (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->অঙ্ক().setAllBits0();
            } অন্যথা {
                outconstp->অঙ্ক().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->অঙ্ক().opSelInto(fetchConst(মান_Fromp)->অঙ্ক(), lsb, selp->widthConst());
        assign_Out_মান(nodep, vscp, outconstp);
    }
    শূন্য handleAssignSelRecurse(Ast_গ্ৰন্থি_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_সংখ্যা& lsbRef, পূর্ণাঙ্ক depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_গ্ৰন্থি_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        যদি (AstVarRef* ধ্রুৱ varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->অঙ্ক();
            ঘূৰাই;  // And presumably still optimizable()
        }
        যদি (AstSel* ধ্রুৱ subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_সংখ্যা sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            যদি (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->অঙ্ক());
            }
        } অন্যথা {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    শূন্য handleAssignRecurse(Ast_গ্ৰন্থি_Assign* nodep, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp, Ast_গ্ৰন্থি_ৰাশিমালা* মান_Fromp) {
        যদি (!optimizable()) ঘূৰাই;
        যদি (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            যদি (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ঘূৰাই;
            }
            handleAssignElement(nodep, lhsp, মান_Fromp);
        } অন্যথা যদি (AstConcat* ধ্রুৱ selp = VN_CAST(lhsp, Concat)) {
            check_গ্ৰন্থি_Info(selp);
            যদি (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                ঘূৰাই;
            }
            // Split value into left and right concat values
            যদি (!m_checkOnly) {
                {
                    AstConst* ধ্রুৱ outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_মান{},
                                       selp->lhsp()->width(), 0};
                    outconstp->অঙ্ক().opSel(fetchConst(মান_Fromp)->অঙ্ক(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_মান(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_বৃক্ষ(), outconstp);
                }
                {
                    AstConst* ধ্রুৱ outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_মান{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->অঙ্ক().opSel(fetchConst(মান_Fromp)->অঙ্ক(),
                                           selp->rhsp()->width() - 1, 0);
                    new_মান(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_বৃক্ষ(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } অন্যথা যদি (AstReplicate* ধ্রুৱ selp = VN_CAST(lhsp, Replicate)) {
            check_গ্ৰন্থি_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* ধ্রুৱ countp = VN_CAST(selp->countp(), Const);
            যদি (!countp || !countp->অঙ্ক().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                ঘূৰাই;
            }
            handleAssignRecurse(nodep, selp->srcp(), মান_Fromp);
        } অন্যথা যদি (AstSel* ধ্রুৱ selp = VN_CAST(lhsp, Sel)) {
            যদি (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ঘূৰাই;
            }
            handleAssignSel(nodep, selp, মান_Fromp);
        } অন্যথা যদি (VN_IS(lhsp, VarRef)) {
            যদি (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } অন্যথা {
                Ast_গ্ৰন্থি* ধ্রুৱ vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_মান(nodep, vscp, fetch_মান(মান_Fromp));
            }
        } অন্যথা {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        যদি (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } অন্যথা যদি (VN_IS(nodep, AssignDly)) {
            যদি (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } অন্যথা {
            যদি (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        যদি (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            ঘূৰাই;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    শূন্য visit(AstArraySel* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (ধ্রুৱ AstConsArray* ধ্রুৱ consp = VN_CAST(fetch_মান_Null(nodep->fromp()), ConsArray)) {
            ধ্রুৱ AstConst* ধ্রুৱ indexp = fetchConst(nodep->bitp());
            ধ্রুৱ uint32_t offset = indexp->অঙ্ক().toUInt();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ itemp = consp->valuep(offset);
            যদি (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } অন্যথা {
                set_মান(nodep, itemp);
            }
        } অন্যথা {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    শূন্য visit(AstStructSel* nodep) override {
        যদি (!m_params) {
            bad_গ্ৰন্থি_Type(nodep);
            ঘূৰাই;
        }
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        AstConsStruct* ধ্রুৱ consp = VN_AS(fetch_মান(nodep->fromp()), ConsStruct);
        set_মান(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    শূন্য visit(AstSliceSel* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        // Fetch the base constant array
        যদি (ধ্রুৱ AstConsArray* ধ্রুৱ consp = VN_CAST(fetch_মান_Null(nodep->fromp()), ConsArray)) {
            ধ্রুৱ VNumRange& sliceRange = nodep->declRange();
            ধ্রুৱ uint32_t sliceElements = sliceRange.elements();
            ধ্রুৱ পূর্ণাঙ্ক sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ধ্রুৱ newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            বাবে (uint32_t idx = 0; idx < sliceElements; ++idx) {
                ধ্রুৱ uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_বৃক্ষ(false));
            }
            // Assign the new constant array and track it for later deletion
            set_মান(nodep, newConsp);
            m_reclaimValuesp.ঠেলো_back(newConsp);
        } অন্যথা {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    শূন্য visit(AstBegin* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCase* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        UINFO(5, "   CASE " << nodep);
        check_গ্ৰন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা যদি (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            বাবে (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                যদি (!itemp->isDefault()) {
                    বাবে (Ast_গ্ৰন্থি* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        যদি (hit) ভঙ্গ;
                        iterateAndNextConstNull(ep);
                        যদি (optimizable()) {
                            V3_সংখ্যা match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->অঙ্ক(), fetchConst(ep)->অঙ্ক());
                            যদি (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            বাবে (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                যদি (hit) ভঙ্গ;
                যদি (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    শূন্য visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        যদি (jumpingOver()) ঘূৰাই;
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstComment*) override {}

    শূন্য visit(Ast_Stmt_ৰাশিমালা* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_ৰাশিমালা_Stmt* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        check_গ্ৰন্থি_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        যদি (!optimizable()) ঘূৰাই;
        iterateAndNextConstNull(nodep->resultp());
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        new_মান(nodep, fetch_মান(nodep->resultp()));
    }

    শূন্য visit(AstJumpBlock* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        iterateChildrenConst(nodep);
        যদি (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    শূন্য visit(AstJumpGo* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        check_গ্ৰন্থি_Info(nodep);
        যদি (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        যদি (jumpingOver()) ঘূৰাই;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        যদি (!m_params) {
            bad_গ্ৰন্থি_Type(nodep);
            ঘূৰাই;
        }
        check_গ্ৰন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
            ঘূৰাই;
        }
        যদি (!optimizable()) ঘূৰাই;

        size_t iter_গুনতি = 0;
        ধ্রুৱ size_t iterLimit = v3Global.opt.unrollLimit();
        যেতিয়ালৈ (true) {
            যদি (iter_গুনতি > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                ভঙ্গ;
            }
            ++iter_গুনতি;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            যদি (jumpingOver()) ভঙ্গ;
        }

        যদি (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    শূন্য visit(AstLoopTest* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        check_গ্ৰন্থি_Info(nodep);
        iterateConst(nodep->condp());
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        যদি (fetchConst(nodep->condp())->অঙ্ক().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    শূন্য visit(AstStop* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_গ্ৰন্থি_Info(nodep);
    }
    শূন্য visit(AstFuncRef* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_গ্ৰন্থি_Info(nodep);
        যদি (!m_params) {
            bad_গ্ৰন্থি_Type(nodep);
            ঘূৰাই;
        }
        Ast_গ্ৰন্থি_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        যদি (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        যদি (funcp->recursive()) {
            যদি (m_recurse_গুনতি >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                ঘূৰাই;
            }
            ++m_recurse_গুনতি;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_গ্ৰন্থি*, Ast_গ্ৰন্থি_ৰাশিমালা*> old_মানসমূহ;

        যদি (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_মানসমূহ](AstVar* varp) {
                যদি (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = fetch_মান_Null(varp)) {
                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nvaluep = newTrackedClone(valuep);
                        old_মানসমূহ.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_মানসমূহ](Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
                যদি (VN_IS(exprp, Const)) ঘূৰাই;  // Speed up as won't change
                যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = fetch_মান_Null(exprp)) {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nvaluep = newTrackedClone(valuep);
                    old_মানসমূহ.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_গ্ৰন্থি_ৰাশিমালা*>> port_মানসমূহ;
        বাবে (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* ধ্রুৱ portp = it->first;
            Ast_গ্ৰন্থি* ধ্রুৱ pinp = it->second->exprp();
            যদি (pinp) {  // Else too few arguments in function call - ignore it
                যদি (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    ঘূৰাই;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_মানসমূহ.emplace_back(std::make_pair(portp, newTrackedClone(fetch_মান(pinp))));
            }
        }
        // Apply value to the function
        যদি (!m_checkOnly && optimizable())
            বাবে (ধ্রুৱ auto& it : port_মানসমূহ) {
                যদি (!m_checkOnly && optimizable()) new_মান(it.first, it.second);
            }
        Sim_স্তূপ_গ্ৰন্থি স্তূপ_গ্ৰন্থি{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_স্তূপ.ঠেলো_back(&স্তূপ_গ্ৰন্থি);
        যদি (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                যদি (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_স্তূপ.বেৰ_কৰো_back();
        ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* returnp = nullptr;
        যদি (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_মান(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        বাবে (ধ্রুৱ auto& it : old_মানসমূহ) {
            যদি (it.second) new_মান(it.first, it.second);
        }
        যদি (returnp) new_মান(nodep, returnp);
        যদি (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_গুনতি > 0, nodep, "recurse underflow");
            --m_recurse_গুনতি;
        }
    }

    শূন্য visit(AstVar* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!m_params) {
            bad_গ্ৰন্থি_Type(nodep);
            ঘূৰাই;
        }
    }

    শূন্য visit(AstScopeName* /*nodep*/) override {
        যদি (jumpingOver()) ঘূৰাই;
        // Ignore
    }

    শূন্য visit(AstSFormatArg* nodep) override {
        check_গ্ৰন্থি_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    শূন্য visit(AstSFormatF* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;

        Ast_গ্ৰন্থি* nextArgp = nodep->exprsp();
        string result;
        ধ্রুৱ string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        বাবে (; pos != format.cend(); ++pos) {
            যদি (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } অন্যথা যদি (!inPct) {  // Normal text
                result += *pos;
            } অন্যথা {  // Format character
                যদি (std::isdigit(pos[0])) {
                    width += pos[0];
                    অনুৱর্ত;
                }

                inPct = false;

                যদি (V3_সংখ্যা::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_গ্ৰন্থি* ধ্রুৱ argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* ধ্রুৱ fargp = VN_CAST(argp, SFormatArg);
                    Ast_গ্ৰন্থি* ধ্রুৱ subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ধ্রুৱ constp = fetchConstNull(subargp);
                    ধ্রুৱ VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    যদি (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        ভঙ্গ;
                    }
                    ধ্রুৱ string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->অঙ্ক().displayedEnum(fargp, pformat)
                                  : constp->অঙ্ক().displayed(nodep, pformat, formatAttr);
                } অন্যথা {
                    ৱিকল্প (std::tolower(pos[0])) {
                    পক্ষ '%': result += "%"; ভঙ্গ;
                    পক্ষ 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        ভঙ্গ;
                    স্ৱভাৱ: clearOptimizable(nodep, "Unknown $display-like format code."); ভঙ্গ;
                    }
                }
            }
        }

        AstConst* ধ্রুৱ resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_মান(nodep, resultConstp);
        m_reclaimValuesp.ঠেলো_back(resultConstp);
    }

    শূন্য visit(AstDisplay* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_গ্ৰন্থি_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        যদি (m_params) {
            ধ্রুৱ AstConst* ধ্রুৱ textp = fetchConst(nodep->fmtp());
            ৱিকল্প (nodep->displayType()) {
            পক্ষ VDisplayType::DT_DISPLAY:  // FALLTHRU
            পক্ষ VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); ভঙ্গ;
            পক্ষ VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); ভঙ্গ;
            পক্ষ VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); ভঙ্গ;
            পক্ষ VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); ভঙ্গ;
            পক্ষ VDisplayType::DT_WRITE:  // FALLTHRU
            স্ৱভাৱ: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    শূন্য visit(AstToStringN* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        যদি (!optimizable()) ঘূৰাই;  // Accelerate
        check_গ্ৰন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ঘূৰাই;
        ধ্রুৱ std::string result = toStringRecurse(fetch_মান(nodep->lhsp()));
        যদি (!optimizable()) ঘূৰাই;
        AstConst* ধ্রুৱ resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_মান(nodep, resultConstp);
        m_reclaimValuesp.ঠেলো_back(resultConstp);
    }

    শূন্য visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    শূন্য visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_গ্ৰন্থি_Type(nodep);
    }
    শূন্য visit(AstGetInitialRandomSeed* nodep) override { known_Bad_গ্ৰন্থি_Type(nodep); }
    শূন্য visit(AstMemberSel* nodep) override { known_Bad_গ্ৰন্থি_Type(nodep); }
    শূন্য visit(AstSampled* nodep) override { known_Bad_গ্ৰন্থি_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        যদি (jumpingOver()) ঘূৰাই;
        bad_গ্ৰন্থি_Type(nodep);
    }

    // MEMBERS - called by constructor
    শূন্য setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    শূন্য mainGuts(Ast_গ্ৰন্থি* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    শূন্য clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_গুনতি = 0;
        m_data_গুনতি = 0;
        m_jumptargetp = nullptr;

        Ast_গ্ৰন্থি::user1_Clear_বৃক্ষ();
        m_varAux.clear();
        ++m_constGeneration;
    }
    শূন্য mainTableCheck(Ast_গ্ৰন্থি* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    শূন্য mainTableEmulate(Ast_গ্ৰন্থি* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    শূন্য mainParamEmulate(Ast_গ্ৰন্থি* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_গ্ৰন্থি*> unusedRootps;
        বাবে (Ast_গ্ৰন্থি* ধ্রুৱ nodep : m_reclaimValuesp) {
            যদি (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        বাবে (Ast_গ্ৰন্থি* ধ্রুৱ nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
};

#endif  // Guard
