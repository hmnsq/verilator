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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Task.h"
#অন্তর্ভাব "V3Width.h"

#অন্তর্ভাব <deque>
#অন্তর্ভাব <set>
#অন্তর্ভাব <sstream>
#অন্তর্ভাব <stack>
#অন্তর্ভাব <string>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_স্তূপ_গ্রন্থি final {
সর্বজনীন:
    // MEMBERS
    AstFuncRef* ধ্রুবক m_funcp;
    V3TaskConnects* ধ্রুবক m_tconnects;
    // CONSTRUCTORS
    Sim_স্তূপ_গ্রন্থি(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_স্তূপ_গ্রন্থি() = পূর্বনির্ধারিত;
};

class SimulateVisitor VL_NOT_FINAL : সর্বজনীন VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

ব্যক্তিগত:
    // CONSTANTS
    স্থির constexpr পূর্ণসংখ্যা CALL_স্তূপ_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ধ্রুবক VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    গণনা VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    কাঠামো AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_গ্রন্থি_রাশিমালা* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;
    };

    AstUser1Allocator<Ast_গ্রন্থি, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    কাঠামো ConstAllocator final {
        আকার_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        আকার_t m_পরবর্তী_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(আকার_t currentGeneration, Ast_গ্রন্থি* nodep) {
            যদি (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_পরবর্তী_Free = 0;
            }
            UASSERT_OBJ(m_পরবর্তী_Free <= m_constps.আকার_2(), nodep, "Should only allocate at end");
            যদি (m_পরবর্তী_Free == m_constps.আকার_2()) {
                m_constps.ঠেলো_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ধ্রুবক constp = m_constps[m_পরবর্তী_Free++];
            constp->অঙ্ক().nodep(nodep);
            ফেরত constp;
        }

        ~ConstAllocator() {
            জন্য (AstConst* ধ্রুবক constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_গ্রন্থি* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    পূর্ণসংখ্যা m_instr_গুনতি;  ///< Number of nodes
    পূর্ণসংখ্যা m_তথ্য_গুনতি;  ///< Bytes of data
    পূর্ণসংখ্যা m_recurse_গুনতি = 0;  // Now deep in current recursion
    Ast_গ্রন্থি* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি_DType*, ConstAllocator> m_constps;
    আকার_t m_constGeneration = 0;
    std::vector<Sim_স্তূপ_গ্রন্থি*> m_call_স্তূপ;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_গ্রন্থি*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_সংখ্যা(ধ্রুবক V3_সংখ্যা* nump, ধ্রুবক Ast_গ্রন্থি_DType* dtypep) {
        যদি (ধ্রুবক AstRefDType* ধ্রুবক refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        যদি (ধ্রুবক Ast_গ্রন্থি_UOr_Struct_DType* ধ্রুবক stp = VN_CAST(dtypep, গ্রন্থি_UOr_Struct_DType)) {
            যদি (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                জন্য (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ধ্রুবক পূর্ণসংখ্যা width = itemp->width();
                    ধ্রুবক পূর্ণসংখ্যা lsb = itemp->lsb();
                    ধ্রুবক পূর্ণসংখ্যা msb = lsb + width - 1;
                    V3_সংখ্যা field_অঙ্ক{nump, width};
                    field_অঙ্ক.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    যদি (ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক childTypep = itemp->subDTypep()) {
                        out << pretty_সংখ্যা(&field_অঙ্ক, childTypep);
                    } অন্যথা {
                        out << field_অঙ্ক;
                    }
                    যদি (itemp->nextp()) out << ", ";
                }
                out << "}";
                ফেরত out.str();
            }
        } অন্যথা যদি (ধ্রুবক AstPackArrayDType* ধ্রুবক arrayp = VN_CAST(dtypep, PackArrayDType)) {
            যদি (ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                ধ্রুবক পূর্ণসংখ্যা arrayElements = arrayp->elementsConst();
                জন্য (পূর্ণসংখ্যা element = 0; element < arrayElements; ++element) {
                    ধ্রুবক পূর্ণসংখ্যা width = childTypep->width();
                    ধ্রুবক পূর্ণসংখ্যা lsb = width * element;
                    ধ্রুবক পূর্ণসংখ্যা msb = lsb + width - 1;
                    V3_সংখ্যা field_অঙ্ক{nump, width};
                    field_অঙ্ক.opSel(*nump, msb, lsb);
                    ধ্রুবক পূর্ণসংখ্যা arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_সংখ্যা(&field_অঙ্ক, childTypep);
                    যদি (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                ফেরত out.str();
            }
        }
        ফেরত nump->ascii();
    }

    // Checking METHODS
সর্বজনীন:
    /// Call other-this function on all new *non-constant* var references
    আভাসী শূন্য varRefCb(AstVarRef* nodep) {}

    শূন্য clearOptimizable(Ast_গ্রন্থি* nodep /*null ok*/, ধ্রুবক string& why) {
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
            পূর্ণসংখ্যা n = 0;
            জন্য (ধ্রুবক auto& callstack : vlstd::reverse_view(m_call_স্তূপ)) {
                ধ্রুবক AstFuncRef* ধ্রুবক funcp = callstack->m_funcp;
                স্তূপ << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                জন্য (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    ধ্রুবক AstVar* ধ্রুবক portp = conIt->first;
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক pinp = conIt->second->exprp();
                    ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = pinp->dtypep();
                    যদি (AstConst* ধ্রুবক valp = fetchConstNull(pinp)) {
                        স্তূপ << "\n           " << portp->prettyName() << " = "
                              << pretty_সংখ্যা(&valp->অঙ্ক(), dtypep);
                    } অন্যথা যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = fetch_মান_Null(pinp)) {
                        স্তূপ << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                যদি (++n > CALL_স্তূপ_MAX) {
                    স্তূপ << "\n           ... stack truncated";
                    বিরতি;
                }
            }
            m_whyNotOptimizable += স্তূপ.str();
        }
    }
    bool optimizable() ধ্রুবক { ফেরত m_whyNotNodep == nullptr; }
    string whyNotMessage() ধ্রুবক { ফেরত m_whyNotOptimizable; }
    Ast_গ্রন্থি* whyNotNodep() ধ্রুবক { ফেরত m_whyNotNodep; }

    bool isAssignDly() ধ্রুবক { ফেরত m_anyAssignDly; }
    bool isImpure() ধ্রুবক { ফেরত m_isImpure; }
    bool isCoverage() ধ্রুবক { ফেরত m_isCoverage; }
    পূর্ণসংখ্যা instr_গুনতি() ধ্রুবক { ফেরত m_instr_গুনতি; }
    পূর্ণসংখ্যা তথ্য_গুনতি() ধ্রুবক { ফেরত m_তথ্য_গুনতি; }

    // Simulation METHODS
ব্যক্তিগত:
    AstConst* allocConst(Ast_গ্রন্থি* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        ফেরত m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

সর্বজনীন:
    শূন্য new_মান(Ast_গ্রন্থি* nodep, ধ্রুবক Ast_গ্রন্থি_রাশিমালা* valuep) {
        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->অঙ্ক().opAssign(constp->অঙ্ক());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } অন্যথা যদি (fetch_মান_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_মান(nodep, newTrackedClone(const_cast<Ast_গ্রন্থি_রাশিমালা*>(valuep)));
        }
    }
    শূন্য new_Out_মান(Ast_গ্রন্থি* nodep, ধ্রুবক Ast_গ্রন্থি_রাশিমালা* valuep) {
        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->অঙ্ক().opAssign(constp->অঙ্ক());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } অন্যথা যদি (fetch_Out_মান_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_মান(nodep, newTrackedClone(const_cast<Ast_গ্রন্থি_রাশিমালা*>(valuep)));
        }
    }

ব্যক্তিগত:
    Ast_গ্রন্থি_রাশিমালা* newTrackedClone(Ast_গ্রন্থি_রাশিমালা* nodep) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = nodep->clone_বৃক্ষ(false);
        m_reclaimValuesp.ঠেলো_back(newp);
        ফেরত newp;
    }
    AstConst* newConst(Ast_গ্রন্থি* nodep) {
        // Set a constant value for this node
        যদি (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ধ্রুবক constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            ফেরত constp;
        }
        ফেরত fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_গ্রন্থি* nodep) {
        // Set a var-output constant value for this node
        যদি (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* ধ্রুবক constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            ফেরত constp;
        }
        ফেরত fetchOutConst(nodep);
    }

সর্বজনীন:
    Ast_গ্রন্থি_রাশিমালা* fetch_মান_Null(Ast_গ্রন্থি* nodep) { ফেরত m_varAux(nodep).valuep; }

ব্যক্তিগত:
    Ast_গ্রন্থি_রাশিমালা* fetch_Out_মান_Null(Ast_গ্রন্থি* nodep) { ফেরত m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_গ্রন্থি* nodep) { ফেরত VN_CAST(fetch_মান_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_গ্রন্থি* nodep) {
        ফেরত VN_CAST(fetch_Out_মান_Null(nodep), Const);
    }
    Ast_গ্রন্থি_রাশিমালা* fetch_মান(Ast_গ্রন্থি* nodep) {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = fetch_মান_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        ফেরত valuep;
    }
    AstConst* fetchConst(Ast_গ্রন্থি* nodep) {
        AstConst* ধ্রুবক constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        ফেরত constp;
    }
    AstConst* fetchOutConst(Ast_গ্রন্থি* nodep) {
        AstConst* ধ্রুবক constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        ফেরত constp;
    }

সর্বজনীন:
    V3_সংখ্যা* fetch_সংখ্যা_Null(Ast_গ্রন্থি* nodep) {
        AstConst* ধ্রুবক constp = fetchConstNull(nodep);
        যদি (constp) ফেরত &constp->অঙ্ক();
        ফেরত nullptr;
    }
    V3_সংখ্যা* fetch_Out_সংখ্যা_Null(Ast_গ্রন্থি* nodep) {
        AstConst* ধ্রুবক constp = fetchOutConstNull(nodep);
        যদি (constp) ফেরত &constp->অঙ্ক();
        ফেরত nullptr;
    }

ব্যক্তিগত:
    শূন্য set_মান(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    শূন্য set_Out_মান(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    শূন্য check_গ্রন্থি_Info(Ast_গ্রন্থি* nodep, bool ignorePredict = false) {
        যদি (m_checkOnly) {
            m_instr_গুনতি += nodep->instr_গুনতি();
            m_তথ্য_গুনতি += nodep->width();
        }
        যদি (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        যদি (!nodep->isPure()) m_isImpure = true;
    }

    শূন্য known_Bad_গ্রন্থি_Type(Ast_গ্রন্থি* nodep) {
        // Call for node types we know we can't handle
        যদি (jumpingOver()) ফেরত;
        check_গ্রন্থি_Info(nodep);
        যদি (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    শূন্য bad_গ্রন্থি_Type(Ast_গ্রন্থি* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_গ্রন্থি_Info(nodep);
        যদি (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            স্থির std::set<VNType> s_typePrinted;
            ধ্রুবক auto pair = s_typePrinted.emplace(nodep->type());
            যদি (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_গ্রন্থি* varOrScope(AstVarRef* nodep) ধ্রুবক {
        Ast_গ্রন্থি* vscp;
        যদি (m_scoped) {
            vscp = nodep->varScopep();
        } অন্যথা {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        ফেরত vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ধ্রুবক { ফেরত m_jumptargetp; }
    শূন্য assign_Out_মান(ধ্রুবক Ast_গ্রন্থি_Assign* nodep, Ast_গ্রন্থি* vscp, ধ্রুবক Ast_গ্রন্থি_রাশিমালা* valuep) {
        যদি (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_মান(vscp, valuep);
        } অন্যথা {
            new_মান(vscp, valuep);
            set_Out_মান(vscp, fetch_মান(vscp));
        }
    }

    string toStringRecurse(Ast_গ্রন্থি_রাশিমালা* valuep) {
        // Return string representation of a value, or clearOptimizable
        যদি (ধ্রুবক AstConst* ধ্রুবক avaluep = VN_CAST(valuep, Const)) {
            ফেরত "'h"s + avaluep->অঙ্ক().displayed(valuep, "%0x");
        }
        যদি (ধ্রুবক AstConsArray* ধ্রুবক avaluep = VN_CAST(valuep, ConsArray)) {
            string ফলাফল = "'{";
            string comma;
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            জন্য (পূর্ণসংখ্যা n = 0; n < dtypep->elementsConst(); ++n) {
                ফলাফল += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            ফলাফল += "}";
            ফেরত ফলাফল;
        }
        যদি (ধ্রুবক AstConsStruct* ধ্রুবক avaluep = VN_CAST(valuep, ConsStruct)) {
            string ফলাফল = "'{";
            string comma;
            জন্য (ধ্রুবক AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                ফলাফল
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            ফলাফল += "}";
            ফেরত ফলাফল;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        ফেরত "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_গ্রন্থি_রাশিমালা* newInitValuep(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        যদি (AstUnpackArrayDType* ধ্রুবক arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক elementp = newInitValuep(nodep, arrayp->subDTypep());
            ফেরত elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ধ্রুবক structp = VN_CAST(dtypep, StructDType);
        যদি (structp && !structp->packed()) {
            AstConsStruct* ধ্রুবক newp = new AstConsStruct{nodep->fileline(), structp};
            জন্য (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক memberValuep = newInitValuep(nodep, memberp->subDTypep());
                যদি (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_বৃক্ষ(), newp);
                    ফেরত nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            ফেরত newp;
        }
        যদি (ধ্রুবক AstBasicDType* ধ্রুবক basicp = dtypep->basicp()) {
            AstConst* ধ্রুবক constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_মান{}, basicp->widthMin(), 0};
            যদি (basicp->isZeroInit()) {
                constp->অঙ্ক() = V3_সংখ্যা{nodep, basicp};
            } অন্যথা {
                constp->অঙ্ক().setAllBitsX();
            }
            ফেরত constp;
        }
        ফেরত nullptr;
    }
    শূন্য initVar(AstVar* nodep) {
        যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_মান(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_বৃক্ষ(), valuep);
        }
    }

    // VISITORS
    শূন্য visit(AstAlways* nodep) override {
        যদি (jumpingOver()) ফেরত;
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    শূন্য visit(AstVarRef* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!optimizable()) ফেরত;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_গ্রন্থি* ধ্রুবক vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        যদি (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), গ্রন্থি_UOr_Struct_DType))
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
                ধ্রুবক bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep
                    = varIsConst ? fetch_মান_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                যদি (varIsConst && valuep) {
                    যদি (!m_checkOnly && optimizable()) new_মান(vscp, valuep);
                } অন্যথা {
                    যদি (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        যদি (m_checkOnly || !optimizable()) ফেরত;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_গ্রন্থি_রাশিমালা* valuep = fetch_মান_Null(vscp);
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
        যদি (jumpingOver()) ফেরত;
        যদি (m_scoped) {
            bad_গ্রন্থি_Type(nodep);
            ফেরত;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!m_params) {
            bad_গ্রন্থি_Type(nodep);
            ফেরত;
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
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstInitialAutomaticStmt* nodep) override {
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstInitialStaticStmt* nodep) override {
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstInitialStatic* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!m_params) {
            bad_গ্রন্থি_Type(nodep);
            ফেরত;
        }
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        যদি (jumpingOver()) ফেরত;
        UINFO(5, "   IF " << nodep);
        check_গ্রন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterate_And_পরবর্তী_Const_Null(nodep->condp());
            যদি (optimizable()) {
                যদি (fetchConst(nodep->condp())->অঙ্ক().isNeqZero()) {
                    iterate_And_পরবর্তী_Const_Null(nodep->thensp());
                } অন্যথা {
                    iterate_And_পরবর্তী_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    শূন্য visit(AstConst* nodep) override {
        check_গ্রন্থি_Info(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        new_মান(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    শূন্য visit(AstConsArray* nodep) override {
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        AstConsArray* ধ্রুবক valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_মান(nodep->defaultp())->clone_বৃক্ষ(false) : nullptr};
        জন্য (ধ্রুবক AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->সূচক(), fetch_মান(itemp->valuep())->clone_বৃক্ষ(false));
        }
        m_reclaimValuesp.ঠেলো_back(valuep);
        set_মান(nodep, valuep);
    }
    শূন্য visit(AstConsStruct* nodep) override {
        যদি (!m_params) {
            bad_গ্রন্থি_Type(nodep);
            ফেরত;
        }
        check_গ্রন্থি_Info(nodep);
        জন্য (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        যদি (m_checkOnly || !optimizable()) ফেরত;
        AstConsStruct* ধ্রুবক valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        জন্য (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_মান(itemp->valuep())->clone_বৃক্ষ(false)});
        }
        m_reclaimValuesp.ঠেলো_back(valuep);
        set_মান(nodep, valuep);
    }
    শূন্য visit(AstCvtUnpackToUnpack* nodep) override {
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_মান(nodep->fromp())->clone_বৃক্ষ(false), nodep->dtypep()};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ঠেলো_back(valuep);
        set_মান(nodep, valuep);
    }
    শূন্য visit(AstConsArrayItem* nodep) override {
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstEnumItemRef* nodep) override {
        check_গ্রন্থি_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        যদি (m_checkOnly || !optimizable()) ফেরত;
        Ast_গ্রন্থি* ধ্রুবক valuep = nodep->itemp()->valuep();
        যদি (valuep) {
            iterate_And_পরবর্তী_Const_Null(valuep);
            যদি (!optimizable()) ফেরত;
            new_মান(nodep, fetch_মান(valuep));
        } অন্যথা {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Uniop* nodep) override {
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        nodep->সংখ্যা_Operate(newConst(nodep)->অঙ্ক(), fetchConst(nodep->lhsp())->অঙ্ক());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    শূন্য aggregateEqRecurse(V3_সংখ্যা& resultr, Ast_গ্রন্থি* nodep, bool caseEq, Ast_গ্রন্থি_রাশিমালা* lhsp,
                            Ast_গ্রন্থি_রাশিমালা* rhsp) {
        যদি (ধ্রুবক AstConst* ধ্রুবক lconstp = VN_CAST(lhsp, Const)) {
            ধ্রুবক V3_সংখ্যা& lnum = lconstp->অঙ্ক();
            ধ্রুবক V3_সংখ্যা& rnum = VN_AS(rhsp, Const)->অঙ্ক();
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
            ধ্রুবক V3_সংখ্যা পূর্ব_2 = resultr;
            resultr.opLogAnd(পূর্ব_2, eq);
        } অন্যথা যদি (ধ্রুবক AstConsArray* ধ্রুবক lconsp = VN_CAST(lhsp, ConsArray)) {
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            জন্য (পূর্ণসংখ্যা n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } অন্যথা {
            ধ্রুবক AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            জন্য (ধ্রুবক AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    শূন্য handleAggregateCompare(Ast_গ্রন্থি_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ধ্রুবক bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ধ্রুবক bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_সংখ্যা& resultr = newConst(nodep)->অঙ্ক();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_মান(nodep->lhsp()),
                           fetch_মান(nodep->rhsp()));
        যদি (neq) {
            ধ্রুবক V3_সংখ্যা eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Biop* nodep) override {
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        যদি (!VN_IS(fetch_মান(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            ফেরত;
        }
        AstConst* ধ্রুবক valuep = newConst(nodep);
        nodep->সংখ্যা_Operate(newConst(nodep)->অঙ্ক(), fetchConst(nodep->lhsp())->অঙ্ক(),
                             fetchConst(nodep->rhsp())->অঙ্ক());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        যদি (!m_params && VN_IS(nodep, Sel) && valuep->অঙ্ক().isAnyX()) {
            ধ্রুবক V3_সংখ্যা অঙ্ক{valuep, valuep->width(), valuep->অঙ্ক()};
            valuep->অঙ্ক().opBitsOne(অঙ্ক);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Triop* nodep) override {
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        nodep->সংখ্যা_Operate(newConst(nodep)->অঙ্ক(), fetchConst(nodep->lhsp())->অঙ্ক(),
                             fetchConst(nodep->rhsp())->অঙ্ক(), fetchConst(nodep->thsp())->অঙ্ক());
    }
    শূন্য visit(Ast_গ্রন্থি_Quadop* nodep) override {
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        nodep->সংখ্যা_Operate(newConst(nodep)->অঙ্ক(), fetchConst(nodep->lhsp())->অঙ্ক(),
                             fetchConst(nodep->rhsp())->অঙ্ক(), fetchConst(nodep->thsp())->অঙ্ক(),
                             fetchConst(nodep->fhsp())->অঙ্ক());
    }
    শূন্য visit(AstLogAnd* nodep) override {
        // Need to short circuit
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateConst(nodep->lhsp());
            যদি (!optimizable()) ফেরত;
            যদি (fetchConst(nodep->lhsp())->অঙ্ক().isNeqZero()) {
                iterateConst(nodep->rhsp());
                যদি (!optimizable()) ফেরত;
                new_মান(nodep, fetch_মান(nodep->rhsp()));
            } অন্যথা {
                new_মান(nodep, fetch_মান(nodep->lhsp()));  // a zero
            }
        }
    }
    শূন্য visit(AstLogOr* nodep) override {
        // Need to short circuit
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateConst(nodep->lhsp());
            যদি (!optimizable()) ফেরত;
            যদি (fetchConst(nodep->lhsp())->অঙ্ক().isNeqZero()) {
                new_মান(nodep, fetch_মান(nodep->lhsp()));  // a one
            } অন্যথা {
                iterateConst(nodep->rhsp());
                যদি (!optimizable()) ফেরত;
                new_মান(nodep, fetch_মান(nodep->rhsp()));
            }
        }
    }
    শূন্য visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateConst(nodep->lhsp());
            যদি (!optimizable()) ফেরত;
            যদি (fetchConst(nodep->lhsp())->অঙ্ক().isEqZero()) {
                ধ্রুবক AstConst cnst{nodep->fileline(), AstConst::Widthed_মান{}, 1, 1};  // a one
                new_মান(nodep, &cnst);  // a one
            } অন্যথা {
                iterateConst(nodep->rhsp());
                যদি (!optimizable()) ফেরত;
                new_মান(nodep, fetch_মান(nodep->rhsp()));
            }
        }
    }
    শূন্য visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা {
            iterateConst(nodep->condp());
            যদি (!optimizable()) ফেরত;
            যদি (fetchConst(nodep->condp())->অঙ্ক().isNeqZero()) {
                iterateConst(nodep->thenp());
                যদি (!optimizable()) ফেরত;
                new_মান(nodep, fetch_মান(nodep->thenp()));
            } অন্যথা {
                iterateConst(nodep->elsep());
                যদি (!optimizable()) ফেরত;
                new_মান(nodep, fetch_মান(nodep->elsep()));
            }
        }
    }

    শূন্য handleAssignElement(Ast_গ্রন্থি_Assign* nodep, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* মান_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_গ্রন্থি_রাশিমালা*> selps;
        Ast_গ্রন্থি_রাশিমালা* fromp = lhsp;
        যখন (true) {
            যদি (AstArraySel* ধ্রুবক selp = VN_CAST(fromp, ArraySel)) {
                check_গ্রন্থি_Info(selp);
                iterate_And_পরবর্তী_Const_Null(selp->bitp());  // Bit index
                selps.ঠেলো_back(selp);
                fromp = selp->fromp();
            } অন্যথা যদি (AstStructSel* ধ্রুবক selp = VN_CAST(fromp, StructSel)) {
                check_গ্রন্থি_Info(selp);
                selps.ঠেলো_back(selp);
                fromp = selp->fromp();
            } অন্যথা {
                বিরতি;
            }
        }
        AstVarRef* ধ্রুবক varrefp = VN_CAST(fromp, VarRef);
        যদি (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            ফেরত;
        }
        যদি (m_checkOnly || !optimizable()) ফেরত;
        Ast_গ্রন্থি* ধ্রুবক vscp = varOrScope(varrefp);
        Ast_গ্রন্থি_রাশিমালা* rootp = fetch_Out_মান_Null(vscp);
        যদি (!rootp) rootp = fetch_মান_Null(vscp);
        যদি (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            ফেরত;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_গ্রন্থি_রাশিমালা* itemp = rootp;
        জন্য (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক selp : vlstd::reverse_view(selps)) {
            যদি (ধ্রুবক AstArraySel* ধ্রুবক aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ধ্রুবক consp = VN_AS(itemp, ConsArray);
                ধ্রুবক uint32_t সূচক = fetchConst(aselp->bitp())->toUInt();
                ধ্রুবক পূর্ণসংখ্যা elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                যদি (সূচক >= static_cast<uint32_t>(elements)) {
                    ফেরত;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(সূচক);
                যদি (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_বৃক্ষ(false);
                    consp->valuep(সূচক, itemp);
                }
            } অন্যথা {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = newTrackedClone(fetch_মান(মান_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ঠেলো_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_মান(nodep, vscp, rootp);
    }
    শূন্য handleAssignSel(Ast_গ্রন্থি_Assign* nodep, AstSel* selp, Ast_গ্রন্থি_রাশিমালা* মান_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_সংখ্যা lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_গ্রন্থি* ধ্রুবক vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        যদি (AstConst* ধ্রুবক vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } অন্যথা যদি (AstConst* ধ্রুবক vscpnump = fetchConstNull(vscp)) {
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
    শূন্য handleAssignSelRecurse(Ast_গ্রন্থি_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_সংখ্যা& lsbRef, পূর্ণসংখ্যা depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_গ্রন্থি_Info(selp);
        iterate_And_পরবর্তী_Const_Null(selp->lsbp());  // Bit index
        যদি (AstVarRef* ধ্রুবক varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->অঙ্ক();
            ফেরত;  // And presumably still optimizable()
        }
        যদি (AstSel* ধ্রুবক subselp = VN_CAST(selp->fromp(), Sel)) {
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
    শূন্য handleAssignRecurse(Ast_গ্রন্থি_Assign* nodep, Ast_গ্রন্থি_রাশিমালা* lhsp, Ast_গ্রন্থি_রাশিমালা* মান_Fromp) {
        যদি (!optimizable()) ফেরত;
        যদি (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            যদি (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ফেরত;
            }
            handleAssignElement(nodep, lhsp, মান_Fromp);
        } অন্যথা যদি (AstConcat* ধ্রুবক selp = VN_CAST(lhsp, Concat)) {
            check_গ্রন্থি_Info(selp);
            যদি (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                ফেরত;
            }
            // Split value into left and right concat values
            যদি (!m_checkOnly) {
                {
                    AstConst* ধ্রুবক outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_মান{},
                                       selp->lhsp()->width(), 0};
                    outconstp->অঙ্ক().opSel(fetchConst(মান_Fromp)->অঙ্ক(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_মান(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_বৃক্ষ(), outconstp);
                }
                {
                    AstConst* ধ্রুবক outconstp
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
        } অন্যথা যদি (AstReplicate* ধ্রুবক selp = VN_CAST(lhsp, Replicate)) {
            check_গ্রন্থি_Info(selp);
            iterate_And_পরবর্তী_Const_Null(selp->countp());
            AstConst* ধ্রুবক countp = VN_CAST(selp->countp(), Const);
            যদি (!countp || !countp->অঙ্ক().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                ফেরত;
            }
            handleAssignRecurse(nodep, selp->srcp(), মান_Fromp);
        } অন্যথা যদি (AstSel* ধ্রুবক selp = VN_CAST(lhsp, Sel)) {
            যদি (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                ফেরত;
            }
            handleAssignSel(nodep, selp, মান_Fromp);
        } অন্যথা যদি (VN_IS(lhsp, VarRef)) {
            যদি (m_checkOnly) {
                iterate_And_পরবর্তী_Const_Null(lhsp);
            } অন্যথা {
                Ast_গ্রন্থি* ধ্রুবক vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_মান(nodep, vscp, fetch_মান(মান_Fromp));
            }
        } অন্যথা {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);

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
            ফেরত;
        }

        iterate_And_পরবর্তী_Const_Null(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    শূন্য visit(AstArraySel* nodep) override {
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (ধ্রুবক AstConsArray* ধ্রুবক consp = VN_CAST(fetch_মান_Null(nodep->fromp()), ConsArray)) {
            ধ্রুবক AstConst* ধ্রুবক indexp = fetchConst(nodep->bitp());
            ধ্রুবক uint32_t offset = indexp->অঙ্ক().toUInt();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক itemp = consp->valuep(offset);
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
            bad_গ্রন্থি_Type(nodep);
            ফেরত;
        }
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        AstConsStruct* ধ্রুবক consp = VN_AS(fetch_মান(nodep->fromp()), ConsStruct);
        set_মান(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    শূন্য visit(AstSliceSel* nodep) override {
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        // Fetch the base constant array
        যদি (ধ্রুবক AstConsArray* ধ্রুবক consp = VN_CAST(fetch_মান_Null(nodep->fromp()), ConsArray)) {
            ধ্রুবক VNumRange& sliceRange = nodep->declRange();
            ধ্রুবক uint32_t sliceElements = sliceRange.elements();
            ধ্রুবক পূর্ণসংখ্যা sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ধ্রুবক newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            জন্য (uint32_t idx = 0; idx < sliceElements; ++idx) {
                ধ্রুবক uint32_t baseIdx = sliceLo + idx;
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
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstCase* nodep) override {
        যদি (jumpingOver()) ফেরত;
        UINFO(5, "   CASE " << nodep);
        check_গ্রন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
        } অন্যথা যদি (optimizable()) {
            iterate_And_পরবর্তী_Const_Null(nodep->exprp());
            bool hit = false;
            জন্য (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                যদি (!itemp->isDefault()) {
                    জন্য (Ast_গ্রন্থি* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        যদি (hit) বিরতি;
                        iterate_And_পরবর্তী_Const_Null(ep);
                        যদি (optimizable()) {
                            V3_সংখ্যা match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->অঙ্ক(), fetchConst(ep)->অঙ্ক());
                            যদি (match.isNeqZero()) {
                                iterate_And_পরবর্তী_Const_Null(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            জন্য (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                যদি (hit) বিরতি;
                যদি (itemp->isDefault()) {
                    iterate_And_পরবর্তী_Const_Null(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    শূন্য visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        যদি (jumpingOver()) ফেরত;
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstComment*) override {}

    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        যদি (jumpingOver()) ফেরত;
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_রাশিমালা_Stmt* nodep) override {
        যদি (jumpingOver()) ফেরত;
        check_গ্রন্থি_Info(nodep);
        iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
        যদি (!optimizable()) ফেরত;
        iterate_And_পরবর্তী_Const_Null(nodep->resultp());
        যদি (m_checkOnly || !optimizable()) ফেরত;
        new_মান(nodep, fetch_মান(nodep->resultp()));
    }

    শূন্য visit(AstJumpBlock* nodep) override {
        যদি (jumpingOver()) ফেরত;
        iterateChildrenConst(nodep);
        যদি (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    শূন্য visit(AstJumpGo* nodep) override {
        যদি (jumpingOver()) ফেরত;
        check_গ্রন্থি_Info(nodep);
        যদি (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    শূন্য visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        যদি (jumpingOver()) ফেরত;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        যদি (!m_params) {
            bad_গ্রন্থি_Type(nodep);
            ফেরত;
        }
        check_গ্রন্থি_Info(nodep);
        যদি (m_checkOnly) {
            iterateChildrenConst(nodep);
            ফেরত;
        }
        যদি (!optimizable()) ফেরত;

        আকার_t iter_গুনতি = 0;
        ধ্রুবক আকার_t iterLimit = v3Global.opt.unrollLimit();
        যখন (true) {
            যদি (iter_গুনতি > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                বিরতি;
            }
            ++iter_গুনতি;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
            যদি (jumpingOver()) বিরতি;
        }

        যদি (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    শূন্য visit(AstLoopTest* nodep) override {
        যদি (jumpingOver()) ফেরত;
        check_গ্রন্থি_Info(nodep);
        iterateConst(nodep->condp());
        যদি (m_checkOnly || !optimizable()) ফেরত;
        যদি (fetchConst(nodep->condp())->অঙ্ক().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    শূন্য visit(AstStop* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_গ্রন্থি_Info(nodep);
    }
    শূন্য visit(AstFuncRef* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!optimizable()) ফেরত;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_গ্রন্থি_Info(nodep);
        যদি (!m_params) {
            bad_গ্রন্থি_Type(nodep);
            ফেরত;
        }
        Ast_গ্রন্থি_FTask* funcp = nodep->taskp();
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
                ফেরত;
            }
            ++m_recurse_গুনতি;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_গ্রন্থি*, Ast_গ্রন্থি_রাশিমালা*> old_মানসমূহ;

        যদি (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_মানসমূহ](AstVar* varp) {
                যদি (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = fetch_মান_Null(varp)) {
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nvaluep = newTrackedClone(valuep);
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
            funcp->foreach([this, &old_মানসমূহ](Ast_গ্রন্থি_রাশিমালা* exprp) {
                যদি (VN_IS(exprp, Const)) ফেরত;  // Speed up as won't change
                যদি (Ast_গ্রন্থি_রাশিমালা* ধ্রুবক valuep = fetch_মান_Null(exprp)) {
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nvaluep = newTrackedClone(valuep);
                    old_মানসমূহ.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_গ্রন্থি_রাশিমালা*>> port_মানসমূহ;
        জন্য (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* ধ্রুবক portp = it->first;
            Ast_গ্রন্থি* ধ্রুবক pinp = it->second->exprp();
            যদি (pinp) {  // Else too few arguments in function call - ignore it
                যদি (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    ফেরত;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_মানসমূহ.emplace_back(std::make_pair(portp, newTrackedClone(fetch_মান(pinp))));
            }
        }
        // Apply value to the function
        যদি (!m_checkOnly && optimizable())
            জন্য (ধ্রুবক auto& it : port_মানসমূহ) {
                যদি (!m_checkOnly && optimizable()) new_মান(it.first, it.second);
            }
        Sim_স্তূপ_গ্রন্থি স্তূপ_গ্রন্থি{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_স্তূপ.ঠেলো_back(&স্তূপ_গ্রন্থি);
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
        m_call_স্তূপ.বের_করো_back();
        ধ্রুবক Ast_গ্রন্থি_রাশিমালা* returnp = nullptr;
        যদি (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_মান(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        জন্য (ধ্রুবক auto& it : old_মানসমূহ) {
            যদি (it.second) new_মান(it.first, it.second);
        }
        যদি (returnp) new_মান(nodep, returnp);
        যদি (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_গুনতি > 0, nodep, "recurse underflow");
            --m_recurse_গুনতি;
        }
    }

    শূন্য visit(AstVar* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!m_params) {
            bad_গ্রন্থি_Type(nodep);
            ফেরত;
        }
    }

    শূন্য visit(AstScopeName* /*nodep*/) override {
        যদি (jumpingOver()) ফেরত;
        // Ignore
    }

    শূন্য visit(AstSFormatArg* nodep) override {
        check_গ্রন্থি_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_পরবর্তী_Const_Null(nodep->exprp());
    }
    শূন্য visit(AstSFormatF* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;

        Ast_গ্রন্থি* পরবর্তী_Argp = nodep->exprsp();
        string ফলাফল;
        ধ্রুবক string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        জন্য (; pos != format.cend(); ++pos) {
            যদি (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } অন্যথা যদি (!inPct) {  // Normal text
                ফলাফল += *pos;
            } অন্যথা {  // Format character
                যদি (std::isdigit(pos[0])) {
                    width += pos[0];
                    চালু;
                }

                inPct = false;

                যদি (V3_সংখ্যা::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_গ্রন্থি* ধ্রুবক argp = পরবর্তী_Argp;
                    পরবর্তী_Argp = পরবর্তী_Argp->nextp();
                    AstSFormatArg* ধ্রুবক fargp = VN_CAST(argp, SFormatArg);
                    Ast_গ্রন্থি* ধ্রুবক subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ধ্রুবক constp = fetchConstNull(subargp);
                    ধ্রুবক VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    যদি (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        বিরতি;
                    }
                    ধ্রুবক string pformat = "%"s + width + pos[0];
                    ফলাফল += formatAttr.isEnum()
                                  ? constp->অঙ্ক().displayedEnum(fargp, pformat)
                                  : constp->অঙ্ক().displayed(nodep, pformat, formatAttr);
                } অন্যথা {
                    নির্বাচন (std::tolower(pos[0])) {
                    ক্ষেত্র '%': ফলাফল += "%"; বিরতি;
                    ক্ষেত্র 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        ফলাফল += "%m";
                        বিরতি;
                    পূর্বনির্ধারিত: clearOptimizable(nodep, "Unknown $display-like format code."); বিরতি;
                    }
                }
            }
        }

        AstConst* ধ্রুবক ফলাফল_Constp = new AstConst{nodep->fileline(), AstConst::String{}, ফলাফল};
        set_মান(nodep, ফলাফল_Constp);
        m_reclaimValuesp.ঠেলো_back(ফলাফল_Constp);
    }

    শূন্য visit(AstDisplay* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!optimizable()) ফেরত;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_গ্রন্থি_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        যদি (m_params) {
            ধ্রুবক AstConst* ধ্রুবক textp = fetchConst(nodep->fmtp());
            নির্বাচন (nodep->displayType()) {
            ক্ষেত্র VDisplayType::DT_DISPLAY:  // FALLTHRU
            ক্ষেত্র VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); বিরতি;
            ক্ষেত্র VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); বিরতি;
            ক্ষেত্র VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); বিরতি;
            ক্ষেত্র VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); বিরতি;
            ক্ষেত্র VDisplayType::DT_WRITE:  // FALLTHRU
            পূর্বনির্ধারিত: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    শূন্য visit(AstToStringN* nodep) override {
        যদি (jumpingOver()) ফেরত;
        যদি (!optimizable()) ফেরত;  // Accelerate
        check_গ্রন্থি_Info(nodep);
        iterateChildrenConst(nodep);
        যদি (m_checkOnly || !optimizable()) ফেরত;
        ধ্রুবক std::string ফলাফল = toStringRecurse(fetch_মান(nodep->lhsp()));
        যদি (!optimizable()) ফেরত;
        AstConst* ধ্রুবক ফলাফল_Constp = new AstConst{nodep->fileline(), AstConst::String{}, ফলাফল};
        set_মান(nodep, ফলাফল_Constp);
        m_reclaimValuesp.ঠেলো_back(ফলাফল_Constp);
    }

    শূন্য visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    শূন্য visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_গ্রন্থি_Type(nodep);
    }
    শূন্য visit(AstGetInitialRandomSeed* nodep) override { known_Bad_গ্রন্থি_Type(nodep); }
    শূন্য visit(AstMemberSel* nodep) override { known_Bad_গ্রন্থি_Type(nodep); }
    শূন্য visit(AstSampled* nodep) override { known_Bad_গ্রন্থি_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (jumpingOver()) ফেরত;
        bad_গ্রন্থি_Type(nodep);
    }

    // MEMBERS - called by constructor
    শূন্য setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    শূন্য mainGuts(Ast_গ্রন্থি* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

সর্বজনীন:
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
        m_তথ্য_গুনতি = 0;
        m_jumptargetp = nullptr;

        Ast_গ্রন্থি::user1_Clear_বৃক্ষ();
        m_varAux.clear();
        ++m_constGeneration;
    }
    শূন্য mainTableCheck(Ast_গ্রন্থি* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    শূন্য mainTableEmulate(Ast_গ্রন্থি* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    শূন্য mainParamEmulate(Ast_গ্রন্থি* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_গ্রন্থি*> unusedRootps;
        জন্য (Ast_গ্রন্থি* ধ্রুবক nodep : m_reclaimValuesp) {
            যদি (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        জন্য (Ast_গ্রন্থি* ধ্রুবক nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
};

#endif  // Guard
