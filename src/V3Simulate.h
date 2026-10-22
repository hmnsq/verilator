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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Task.h"
#અન્તર્ભાવ "V3Width.h"

#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <sstream>
#અન્તર્ભાવ <stack>
#અન્તર્ભાવ <string>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_ઢગલો_ગાંઠ final {
જાહેર:
    // MEMBERS
    AstFuncRef* અચલ m_funcp;
    V3TaskConnects* અચલ m_tconnects;
    // CONSTRUCTORS
    Sim_ઢગલો_ગાંઠ(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ઢગલો_ગાંઠ() = મૂળભૂત;
};

class SimulateVisitor VL_NOT_FINAL : જાહેર VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

ખાનગી:
    // CONSTANTS
    સ્થિર constexpr પૂર્ણાંક CALL_ઢગલો_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    અચલ VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    ગણના VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    સંરચના AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_ગાંઠ_પદાવલિ* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_ગાંઠ_પદાવલિ* valuep = nullptr;
    };

    AstUser1Allocator<Ast_ગાંઠ, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    સંરચના ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, Ast_ગાંઠ* nodep) {
            જો (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            જો (m_nextFree == m_constps.size()) {
                m_constps.ધકેલો_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* અચલ constp = m_constps[m_nextFree++];
            constp->અંક().nodep(nodep);
            પાછા constp;
        }

        ~ConstAllocator() {
            માટે (AstConst* અચલ constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_ગાંઠ* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    પૂર્ણાંક m_instr_ગણતરી;  ///< Number of nodes
    પૂર્ણાંક m_data_ગણતરી;  ///< Bytes of data
    પૂર્ણાંક m_recurse_ગણતરી = 0;  // Now deep in current recursion
    Ast_ગાંઠ* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<અચલ Ast_ગાંઠ_DType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<Sim_ઢગલો_ગાંઠ*> m_call_ઢગલો;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_ગાંઠ*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_સંખ્યા(અચલ V3_સંખ્યા* nump, અચલ Ast_ગાંઠ_DType* dtypep) {
        જો (અચલ AstRefDType* અચલ refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        જો (અચલ Ast_ગાંઠ_UOr_Struct_DType* અચલ stp = VN_CAST(dtypep, ગાંઠ_UOr_Struct_DType)) {
            જો (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                માટે (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    અચલ પૂર્ણાંક width = itemp->width();
                    અચલ પૂર્ણાંક lsb = itemp->lsb();
                    અચલ પૂર્ણાંક msb = lsb + width - 1;
                    V3_સંખ્યા field_અંક{nump, width};
                    field_અંક.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    જો (અચલ Ast_ગાંઠ_DType* અચલ childTypep = itemp->subDTypep()) {
                        out << pretty_સંખ્યા(&field_અંક, childTypep);
                    } નહીંતર {
                        out << field_અંક;
                    }
                    જો (itemp->nextp()) out << ", ";
                }
                out << "}";
                પાછા out.str();
            }
        } નહીંતર જો (અચલ AstPackArrayDType* અચલ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            જો (અચલ Ast_ગાંઠ_DType* અચલ childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                અચલ પૂર્ણાંક arrayElements = arrayp->elementsConst();
                માટે (પૂર્ણાંક element = 0; element < arrayElements; ++element) {
                    અચલ પૂર્ણાંક width = childTypep->width();
                    અચલ પૂર્ણાંક lsb = width * element;
                    અચલ પૂર્ણાંક msb = lsb + width - 1;
                    V3_સંખ્યા field_અંક{nump, width};
                    field_અંક.opSel(*nump, msb, lsb);
                    અચલ પૂર્ણાંક arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_સંખ્યા(&field_અંક, childTypep);
                    જો (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                પાછા out.str();
            }
        }
        પાછા nump->ascii();
    }

    // Checking METHODS
જાહેર:
    /// Call other-this function on all new *non-constant* var references
    આભાસી શૂન્ય varRefCb(AstVarRef* nodep) {}

    શૂન્ય clearOptimizable(Ast_ગાંઠ* nodep /*null ok*/, અચલ string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        જો (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            જો (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                જો (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ઢગલો;
            પૂર્ણાંક n = 0;
            માટે (અચલ auto& callstack : vlstd::reverse_view(m_call_ઢગલો)) {
                અચલ AstFuncRef* અચલ funcp = callstack->m_funcp;
                ઢગલો << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                માટે (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    અચલ AstVar* અચલ portp = conIt->first;
                    Ast_ગાંઠ_પદાવલિ* અચલ pinp = conIt->second->exprp();
                    અચલ Ast_ગાંઠ_DType* અચલ dtypep = pinp->dtypep();
                    જો (AstConst* અચલ valp = fetchConstNull(pinp)) {
                        ઢગલો << "\n           " << portp->prettyName() << " = "
                              << pretty_સંખ્યા(&valp->અંક(), dtypep);
                    } નહીંતર જો (Ast_ગાંઠ_પદાવલિ* અચલ valuep = fetch_કિંમત_Null(pinp)) {
                        ઢગલો << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                જો (++n > CALL_ઢગલો_MAX) {
                    ઢગલો << "\n           ... stack truncated";
                    તોડો;
                }
            }
            m_whyNotOptimizable += ઢગલો.str();
        }
    }
    bool optimizable() અચલ { પાછા m_whyNotNodep == nullptr; }
    string whyNotMessage() અચલ { પાછા m_whyNotOptimizable; }
    Ast_ગાંઠ* whyNotNodep() અચલ { પાછા m_whyNotNodep; }

    bool isAssignDly() અચલ { પાછા m_anyAssignDly; }
    bool isImpure() અચલ { પાછા m_isImpure; }
    bool isCoverage() અચલ { પાછા m_isCoverage; }
    પૂર્ણાંક instr_ગણતરી() અચલ { પાછા m_instr_ગણતરી; }
    પૂર્ણાંક data_ગણતરી() અચલ { પાછા m_data_ગણતરી; }

    // Simulation METHODS
ખાનગી:
    AstConst* allocConst(Ast_ગાંઠ* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        પાછા m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

જાહેર:
    શૂન્ય new_કિંમત(Ast_ગાંઠ* nodep, અચલ Ast_ગાંઠ_પદાવલિ* valuep) {
        જો (અચલ AstConst* અચલ constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->અંક().opAssign(constp->અંક());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } નહીંતર જો (fetch_કિંમત_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_કિંમત(nodep, newTrackedClone(const_cast<Ast_ગાંઠ_પદાવલિ*>(valuep)));
        }
    }
    શૂન્ય new_Out_કિંમત(Ast_ગાંઠ* nodep, અચલ Ast_ગાંઠ_પદાવલિ* valuep) {
        જો (અચલ AstConst* અચલ constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->અંક().opAssign(constp->અંક());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } નહીંતર જો (fetch_Out_કિંમત_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_કિંમત(nodep, newTrackedClone(const_cast<Ast_ગાંઠ_પદાવલિ*>(valuep)));
        }
    }

ખાનગી:
    Ast_ગાંઠ_પદાવલિ* newTrackedClone(Ast_ગાંઠ_પદાવલિ* nodep) {
        Ast_ગાંઠ_પદાવલિ* અચલ newp = nodep->clone_વૃક્ષ(false);
        m_reclaimValuesp.ધકેલો_back(newp);
        પાછા newp;
    }
    AstConst* newConst(Ast_ગાંઠ* nodep) {
        // Set a constant value for this node
        જો (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* અચલ constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            પાછા constp;
        }
        પાછા fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_ગાંઠ* nodep) {
        // Set a var-output constant value for this node
        જો (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* અચલ constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            પાછા constp;
        }
        પાછા fetchOutConst(nodep);
    }

જાહેર:
    Ast_ગાંઠ_પદાવલિ* fetch_કિંમત_Null(Ast_ગાંઠ* nodep) { પાછા m_varAux(nodep).valuep; }

ખાનગી:
    Ast_ગાંઠ_પદાવલિ* fetch_Out_કિંમત_Null(Ast_ગાંઠ* nodep) { પાછા m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_ગાંઠ* nodep) { પાછા VN_CAST(fetch_કિંમત_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_ગાંઠ* nodep) {
        પાછા VN_CAST(fetch_Out_કિંમત_Null(nodep), Const);
    }
    Ast_ગાંઠ_પદાવલિ* fetch_કિંમત(Ast_ગાંઠ* nodep) {
        Ast_ગાંઠ_પદાવલિ* અચલ valuep = fetch_કિંમત_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        પાછા valuep;
    }
    AstConst* fetchConst(Ast_ગાંઠ* nodep) {
        AstConst* અચલ constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        પાછા constp;
    }
    AstConst* fetchOutConst(Ast_ગાંઠ* nodep) {
        AstConst* અચલ constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        પાછા constp;
    }

જાહેર:
    V3_સંખ્યા* fetch_સંખ્યા_Null(Ast_ગાંઠ* nodep) {
        AstConst* અચલ constp = fetchConstNull(nodep);
        જો (constp) પાછા &constp->અંક();
        પાછા nullptr;
    }
    V3_સંખ્યા* fetch_Out_સંખ્યા_Null(Ast_ગાંઠ* nodep) {
        AstConst* અચલ constp = fetchOutConstNull(nodep);
        જો (constp) પાછા &constp->અંક();
        પાછા nullptr;
    }

ખાનગી:
    શૂન્ય set_કિંમત(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    શૂન્ય set_Out_કિંમત(Ast_ગાંઠ* nodep, Ast_ગાંઠ_પદાવલિ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    શૂન્ય check_ગાંઠ_Info(Ast_ગાંઠ* nodep, bool ignorePredict = false) {
        જો (m_checkOnly) {
            m_instr_ગણતરી += nodep->instr_ગણતરી();
            m_data_ગણતરી += nodep->width();
        }
        જો (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        જો (!nodep->isPure()) m_isImpure = true;
    }

    શૂન્ય known_Bad_ગાંઠ_Type(Ast_ગાંઠ* nodep) {
        // Call for node types we know we can't handle
        જો (jumpingOver()) પાછા;
        check_ગાંઠ_Info(nodep);
        જો (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    શૂન્ય bad_ગાંઠ_Type(Ast_ગાંઠ* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_ગાંઠ_Info(nodep);
        જો (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            સ્થિર std::set<VNType> s_typePrinted;
            અચલ auto pair = s_typePrinted.emplace(nodep->type());
            જો (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_ગાંઠ* varOrScope(AstVarRef* nodep) અચલ {
        Ast_ગાંઠ* vscp;
        જો (m_scoped) {
            vscp = nodep->varScopep();
        } નહીંતર {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        પાછા vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() અચલ { પાછા m_jumptargetp; }
    શૂન્ય assign_Out_કિંમત(અચલ Ast_ગાંઠ_Assign* nodep, Ast_ગાંઠ* vscp, અચલ Ast_ગાંઠ_પદાવલિ* valuep) {
        જો (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_કિંમત(vscp, valuep);
        } નહીંતર {
            new_કિંમત(vscp, valuep);
            set_Out_કિંમત(vscp, fetch_કિંમત(vscp));
        }
    }

    string toStringRecurse(Ast_ગાંઠ_પદાવલિ* valuep) {
        // Return string representation of a value, or clearOptimizable
        જો (અચલ AstConst* અચલ avaluep = VN_CAST(valuep, Const)) {
            પાછા "'h"s + avaluep->અંક().displayed(valuep, "%0x");
        }
        જો (અચલ AstConsArray* અચલ avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            અચલ AstUnpackArrayDType* અચલ dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            માટે (પૂર્ણાંક n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            પાછા result;
        }
        જો (અચલ AstConsStruct* અચલ avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            માટે (અચલ AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            પાછા result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        પાછા "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_ગાંઠ_પદાવલિ* newInitValuep(Ast_ગાંઠ* nodep, Ast_ગાંઠ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        જો (AstUnpackArrayDType* અચલ arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_ગાંઠ_પદાવલિ* અચલ elementp = newInitValuep(nodep, arrayp->subDTypep());
            પાછા elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* અચલ structp = VN_CAST(dtypep, StructDType);
        જો (structp && !structp->packed()) {
            AstConsStruct* અચલ newp = new AstConsStruct{nodep->fileline(), structp};
            માટે (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ગાંઠ_પદાવલિ* અચલ memberValuep = newInitValuep(nodep, memberp->subDTypep());
                જો (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_વૃક્ષ(), newp);
                    પાછા nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            પાછા newp;
        }
        જો (અચલ AstBasicDType* અચલ basicp = dtypep->basicp()) {
            AstConst* અચલ constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_કિંમત{}, basicp->widthMin(), 0};
            જો (basicp->isZeroInit()) {
                constp->અંક() = V3_સંખ્યા{nodep, basicp};
            } નહીંતર {
                constp->અંક().setAllBitsX();
            }
            પાછા constp;
        }
        પાછા nullptr;
    }
    શૂન્ય initVar(AstVar* nodep) {
        જો (Ast_ગાંઠ_પદાવલિ* અચલ valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_કિંમત(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_વૃક્ષ(), valuep);
        }
    }

    // VISITORS
    શૂન્ય visit(AstAlways* nodep) override {
        જો (jumpingOver()) પાછા;
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!optimizable()) પાછા;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_ગાંઠ* અચલ vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        જો (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), ગાંઠ_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        જો (nodep->access().isWriteOrRW()) {
            જો (m_inDlyAssign) {
                જો (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    જો (m_checkOnly) varRefCb(nodep);
                }
            } નહીંતર {  // nondly asn
                જો (!(m_varAux(vscp).usage & VU_LV)) {
                    જો (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    જો (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        જો (nodep->access().isReadOrRW()) {
            જો (!(m_varAux(vscp).usage & VU_RV)) {
                જો (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                અચલ bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                અચલ Ast_ગાંઠ_પદાવલિ* અચલ valuep
                    = varIsConst ? fetch_કિંમત_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                જો (varIsConst && valuep) {
                    જો (!m_checkOnly && optimizable()) new_કિંમત(vscp, valuep);
                } નહીંતર {
                    જો (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        જો (m_checkOnly || !optimizable()) પાછા;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_ગાંઠ_પદાવલિ* valuep = fetch_કિંમત_Null(vscp);
            જો (!valuep) {
                જો (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } નહીંતર {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            set_કિંમત(nodep, valuep);
        }
    }
    શૂન્ય visit(AstVarXRef* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (m_scoped) {
            bad_ગાંઠ_Type(nodep);
            પાછા;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!m_params) {
            bad_ગાંઠ_Type(nodep);
            પાછા;
        }
        જો (nodep->dpiImport()) {
            જો (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        જો (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstInitialAutomaticStmt* nodep) override {
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstInitialStaticStmt* nodep) override {
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstInitialStatic* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!m_params) {
            bad_ગાંઠ_Type(nodep);
            પાછા;
        }
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        જો (jumpingOver()) પાછા;
        UINFO(5, "   IF " << nodep);
        check_ગાંઠ_Info(nodep);
        જો (m_checkOnly) {
            iterateChildrenConst(nodep);
        } નહીંતર {
            iterateAndNextConstNull(nodep->condp());
            જો (optimizable()) {
                જો (fetchConst(nodep->condp())->અંક().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } નહીંતર {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    શૂન્ય visit(AstConst* nodep) override {
        check_ગાંઠ_Info(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        new_કિંમત(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    શૂન્ય visit(AstConsArray* nodep) override {
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        AstConsArray* અચલ valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_કિંમત(nodep->defaultp())->clone_વૃક્ષ(false) : nullptr};
        માટે (અચલ AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_કિંમત(itemp->valuep())->clone_વૃક્ષ(false));
        }
        m_reclaimValuesp.ધકેલો_back(valuep);
        set_કિંમત(nodep, valuep);
    }
    શૂન્ય visit(AstConsStruct* nodep) override {
        જો (!m_params) {
            bad_ગાંઠ_Type(nodep);
            પાછા;
        }
        check_ગાંઠ_Info(nodep);
        માટે (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        જો (m_checkOnly || !optimizable()) પાછા;
        AstConsStruct* અચલ valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        માટે (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_કિંમત(itemp->valuep())->clone_વૃક્ષ(false)});
        }
        m_reclaimValuesp.ધકેલો_back(valuep);
        set_કિંમત(nodep, valuep);
    }
    શૂન્ય visit(AstCvtUnpackToUnpack* nodep) override {
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_ગાંઠ_પદાવલિ* અચલ cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_કિંમત(nodep->fromp())->clone_વૃક્ષ(false), nodep->dtypep()};
        Ast_ગાંઠ_પદાવલિ* અચલ valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ધકેલો_back(valuep);
        set_કિંમત(nodep, valuep);
    }
    શૂન્ય visit(AstConsArrayItem* nodep) override {
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstEnumItemRef* nodep) override {
        check_ગાંઠ_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        જો (m_checkOnly || !optimizable()) પાછા;
        Ast_ગાંઠ* અચલ valuep = nodep->itemp()->valuep();
        જો (valuep) {
            iterateAndNextConstNull(valuep);
            જો (!optimizable()) પાછા;
            new_કિંમત(nodep, fetch_કિંમત(valuep));
        } નહીંતર {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Uniop* nodep) override {
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        nodep->સંખ્યા_Operate(newConst(nodep)->અંક(), fetchConst(nodep->lhsp())->અંક());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    શૂન્ય aggregateEqRecurse(V3_સંખ્યા& resultr, Ast_ગાંઠ* nodep, bool caseEq, Ast_ગાંઠ_પદાવલિ* lhsp,
                            Ast_ગાંઠ_પદાવલિ* rhsp) {
        જો (અચલ AstConst* અચલ lconstp = VN_CAST(lhsp, Const)) {
            અચલ V3_સંખ્યા& lnum = lconstp->અંક();
            અચલ V3_સંખ્યા& rnum = VN_AS(rhsp, Const)->અંક();
            V3_સંખ્યા eq{nodep, 1};
            જો (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } નહીંતર જો (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } નહીંતર જો (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } નહીંતર {
                eq.opEq(lnum, rnum);
            }
            અચલ V3_સંખ્યા prev = resultr;
            resultr.opLogAnd(prev, eq);
        } નહીંતર જો (અચલ AstConsArray* અચલ lconsp = VN_CAST(lhsp, ConsArray)) {
            અચલ AstUnpackArrayDType* અચલ dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            માટે (પૂર્ણાંક n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } નહીંતર {
            અચલ AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            માટે (અચલ AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    શૂન્ય handleAggregateCompare(Ast_ગાંઠ_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        અચલ bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        અચલ bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_સંખ્યા& resultr = newConst(nodep)->અંક();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_કિંમત(nodep->lhsp()),
                           fetch_કિંમત(nodep->rhsp()));
        જો (neq) {
            અચલ V3_સંખ્યા eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Biop* nodep) override {
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        જો (!VN_IS(fetch_કિંમત(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            પાછા;
        }
        AstConst* અચલ valuep = newConst(nodep);
        nodep->સંખ્યા_Operate(newConst(nodep)->અંક(), fetchConst(nodep->lhsp())->અંક(),
                             fetchConst(nodep->rhsp())->અંક());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        જો (!m_params && VN_IS(nodep, Sel) && valuep->અંક().isAnyX()) {
            અચલ V3_સંખ્યા અંક{valuep, valuep->width(), valuep->અંક()};
            valuep->અંક().opBitsOne(અંક);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Triop* nodep) override {
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        nodep->સંખ્યા_Operate(newConst(nodep)->અંક(), fetchConst(nodep->lhsp())->અંક(),
                             fetchConst(nodep->rhsp())->અંક(), fetchConst(nodep->thsp())->અંક());
    }
    શૂન્ય visit(Ast_ગાંઠ_Quadop* nodep) override {
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        nodep->સંખ્યા_Operate(newConst(nodep)->અંક(), fetchConst(nodep->lhsp())->અંક(),
                             fetchConst(nodep->rhsp())->અંક(), fetchConst(nodep->thsp())->અંક(),
                             fetchConst(nodep->fhsp())->અંક());
    }
    શૂન્ય visit(AstLogAnd* nodep) override {
        // Need to short circuit
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        જો (m_checkOnly) {
            iterateChildrenConst(nodep);
        } નહીંતર {
            iterateConst(nodep->lhsp());
            જો (!optimizable()) પાછા;
            જો (fetchConst(nodep->lhsp())->અંક().isNeqZero()) {
                iterateConst(nodep->rhsp());
                જો (!optimizable()) પાછા;
                new_કિંમત(nodep, fetch_કિંમત(nodep->rhsp()));
            } નહીંતર {
                new_કિંમત(nodep, fetch_કિંમત(nodep->lhsp()));  // a zero
            }
        }
    }
    શૂન્ય visit(AstLogOr* nodep) override {
        // Need to short circuit
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        જો (m_checkOnly) {
            iterateChildrenConst(nodep);
        } નહીંતર {
            iterateConst(nodep->lhsp());
            જો (!optimizable()) પાછા;
            જો (fetchConst(nodep->lhsp())->અંક().isNeqZero()) {
                new_કિંમત(nodep, fetch_કિંમત(nodep->lhsp()));  // a one
            } નહીંતર {
                iterateConst(nodep->rhsp());
                જો (!optimizable()) પાછા;
                new_કિંમત(nodep, fetch_કિંમત(nodep->rhsp()));
            }
        }
    }
    શૂન્ય visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        જો (m_checkOnly) {
            iterateChildrenConst(nodep);
        } નહીંતર {
            iterateConst(nodep->lhsp());
            જો (!optimizable()) પાછા;
            જો (fetchConst(nodep->lhsp())->અંક().isEqZero()) {
                અચલ AstConst cnst{nodep->fileline(), AstConst::Widthed_કિંમત{}, 1, 1};  // a one
                new_કિંમત(nodep, &cnst);  // a one
            } નહીંતર {
                iterateConst(nodep->rhsp());
                જો (!optimizable()) પાછા;
                new_કિંમત(nodep, fetch_કિંમત(nodep->rhsp()));
            }
        }
    }
    શૂન્ય visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        જો (m_checkOnly) {
            iterateChildrenConst(nodep);
        } નહીંતર {
            iterateConst(nodep->condp());
            જો (!optimizable()) પાછા;
            જો (fetchConst(nodep->condp())->અંક().isNeqZero()) {
                iterateConst(nodep->thenp());
                જો (!optimizable()) પાછા;
                new_કિંમત(nodep, fetch_કિંમત(nodep->thenp()));
            } નહીંતર {
                iterateConst(nodep->elsep());
                જો (!optimizable()) પાછા;
                new_કિંમત(nodep, fetch_કિંમત(nodep->elsep()));
            }
        }
    }

    શૂન્ય handleAssignElement(Ast_ગાંઠ_Assign* nodep, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* કિંમત_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_ગાંઠ_પદાવલિ*> selps;
        Ast_ગાંઠ_પદાવલિ* fromp = lhsp;
        જ્યારે (true) {
            જો (AstArraySel* અચલ selp = VN_CAST(fromp, ArraySel)) {
                check_ગાંઠ_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.ધકેલો_back(selp);
                fromp = selp->fromp();
            } નહીંતર જો (AstStructSel* અચલ selp = VN_CAST(fromp, StructSel)) {
                check_ગાંઠ_Info(selp);
                selps.ધકેલો_back(selp);
                fromp = selp->fromp();
            } નહીંતર {
                તોડો;
            }
        }
        AstVarRef* અચલ varrefp = VN_CAST(fromp, VarRef);
        જો (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            પાછા;
        }
        જો (m_checkOnly || !optimizable()) પાછા;
        Ast_ગાંઠ* અચલ vscp = varOrScope(varrefp);
        Ast_ગાંઠ_પદાવલિ* rootp = fetch_Out_કિંમત_Null(vscp);
        જો (!rootp) rootp = fetch_કિંમત_Null(vscp);
        જો (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            પાછા;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_ગાંઠ_પદાવલિ* itemp = rootp;
        માટે (Ast_ગાંઠ_પદાવલિ* અચલ selp : vlstd::reverse_view(selps)) {
            જો (અચલ AstArraySel* અચલ aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* અચલ consp = VN_AS(itemp, ConsArray);
                અચલ uint32_t index = fetchConst(aselp->bitp())->toUInt();
                અચલ પૂર્ણાંક elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                જો (index >= static_cast<uint32_t>(elements)) {
                    પાછા;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                જો (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_વૃક્ષ(false);
                    consp->valuep(index, itemp);
                }
            } નહીંતર {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_ગાંઠ_પદાવલિ* અચલ valuep = newTrackedClone(fetch_કિંમત(કિંમત_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ધકેલો_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_કિંમત(nodep, vscp, rootp);
    }
    શૂન્ય handleAssignSel(Ast_ગાંઠ_Assign* nodep, AstSel* selp, Ast_ગાંઠ_પદાવલિ* કિંમત_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_સંખ્યા lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        જો (m_checkOnly || !optimizable()) પાછા;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_ગાંઠ* અચલ vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        જો (AstConst* અચલ vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } નહીંતર જો (AstConst* અચલ vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } નહીંતર {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_કિંમત{},
                                     varrefp->varp()->widthMin(), 0};
            જો (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->અંક().setAllBits0();
            } નહીંતર {
                outconstp->અંક().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->અંક().opSelInto(fetchConst(કિંમત_Fromp)->અંક(), lsb, selp->widthConst());
        assign_Out_કિંમત(nodep, vscp, outconstp);
    }
    શૂન્ય handleAssignSelRecurse(Ast_ગાંઠ_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_સંખ્યા& lsbRef, પૂર્ણાંક depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_ગાંઠ_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        જો (AstVarRef* અચલ varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->અંક();
            પાછા;  // And presumably still optimizable()
        }
        જો (AstSel* અચલ subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_સંખ્યા sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            જો (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->અંક());
            }
        } નહીંતર {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    શૂન્ય handleAssignRecurse(Ast_ગાંઠ_Assign* nodep, Ast_ગાંઠ_પદાવલિ* lhsp, Ast_ગાંઠ_પદાવલિ* કિંમત_Fromp) {
        જો (!optimizable()) પાછા;
        જો (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            જો (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                પાછા;
            }
            handleAssignElement(nodep, lhsp, કિંમત_Fromp);
        } નહીંતર જો (AstConcat* અચલ selp = VN_CAST(lhsp, Concat)) {
            check_ગાંઠ_Info(selp);
            જો (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                પાછા;
            }
            // Split value into left and right concat values
            જો (!m_checkOnly) {
                {
                    AstConst* અચલ outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_કિંમત{},
                                       selp->lhsp()->width(), 0};
                    outconstp->અંક().opSel(fetchConst(કિંમત_Fromp)->અંક(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_કિંમત(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_વૃક્ષ(), outconstp);
                }
                {
                    AstConst* અચલ outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_કિંમત{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->અંક().opSel(fetchConst(કિંમત_Fromp)->અંક(),
                                           selp->rhsp()->width() - 1, 0);
                    new_કિંમત(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_વૃક્ષ(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } નહીંતર જો (AstReplicate* અચલ selp = VN_CAST(lhsp, Replicate)) {
            check_ગાંઠ_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* અચલ countp = VN_CAST(selp->countp(), Const);
            જો (!countp || !countp->અંક().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                પાછા;
            }
            handleAssignRecurse(nodep, selp->srcp(), કિંમત_Fromp);
        } નહીંતર જો (AstSel* અચલ selp = VN_CAST(lhsp, Sel)) {
            જો (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                પાછા;
            }
            handleAssignSel(nodep, selp, કિંમત_Fromp);
        } નહીંતર જો (VN_IS(lhsp, VarRef)) {
            જો (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } નહીંતર {
                Ast_ગાંઠ* અચલ vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_કિંમત(nodep, vscp, fetch_કિંમત(કિંમત_Fromp));
            }
        } નહીંતર {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        જો (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } નહીંતર જો (VN_IS(nodep, AssignDly)) {
            જો (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } નહીંતર {
            જો (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        જો (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            પાછા;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    શૂન્ય visit(AstArraySel* nodep) override {
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (અચલ AstConsArray* અચલ consp = VN_CAST(fetch_કિંમત_Null(nodep->fromp()), ConsArray)) {
            અચલ AstConst* અચલ indexp = fetchConst(nodep->bitp());
            અચલ uint32_t offset = indexp->અંક().toUInt();
            Ast_ગાંઠ_પદાવલિ* અચલ itemp = consp->valuep(offset);
            જો (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } નહીંતર {
                set_કિંમત(nodep, itemp);
            }
        } નહીંતર {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    શૂન્ય visit(AstStructSel* nodep) override {
        જો (!m_params) {
            bad_ગાંઠ_Type(nodep);
            પાછા;
        }
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        AstConsStruct* અચલ consp = VN_AS(fetch_કિંમત(nodep->fromp()), ConsStruct);
        set_કિંમત(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    શૂન્ય visit(AstSliceSel* nodep) override {
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        // Fetch the base constant array
        જો (અચલ AstConsArray* અચલ consp = VN_CAST(fetch_કિંમત_Null(nodep->fromp()), ConsArray)) {
            અચલ VNumRange& sliceRange = nodep->declRange();
            અચલ uint32_t sliceElements = sliceRange.elements();
            અચલ પૂર્ણાંક sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_ગાંઠ_DType* અચલ dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* અચલ newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            માટે (uint32_t idx = 0; idx < sliceElements; ++idx) {
                અચલ uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_વૃક્ષ(false));
            }
            // Assign the new constant array and track it for later deletion
            set_કિંમત(nodep, newConsp);
            m_reclaimValuesp.ધકેલો_back(newConsp);
        } નહીંતર {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    શૂન્ય visit(AstBegin* nodep) override {
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstCase* nodep) override {
        જો (jumpingOver()) પાછા;
        UINFO(5, "   CASE " << nodep);
        check_ગાંઠ_Info(nodep);
        જો (m_checkOnly) {
            iterateChildrenConst(nodep);
        } નહીંતર જો (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            માટે (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                જો (!itemp->isDefault()) {
                    માટે (Ast_ગાંઠ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        જો (hit) તોડો;
                        iterateAndNextConstNull(ep);
                        જો (optimizable()) {
                            V3_સંખ્યા match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->અંક(), fetchConst(ep)->અંક());
                            જો (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            માટે (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                જો (hit) તોડો;
                જો (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    શૂન્ય visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        જો (jumpingOver()) પાછા;
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstComment*) override {}

    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        જો (jumpingOver()) પાછા;
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_પદાવલિ_Stmt* nodep) override {
        જો (jumpingOver()) પાછા;
        check_ગાંઠ_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        જો (!optimizable()) પાછા;
        iterateAndNextConstNull(nodep->resultp());
        જો (m_checkOnly || !optimizable()) પાછા;
        new_કિંમત(nodep, fetch_કિંમત(nodep->resultp()));
    }

    શૂન્ય visit(AstJumpBlock* nodep) override {
        જો (jumpingOver()) પાછા;
        iterateChildrenConst(nodep);
        જો (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    શૂન્ય visit(AstJumpGo* nodep) override {
        જો (jumpingOver()) પાછા;
        check_ગાંઠ_Info(nodep);
        જો (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    શૂન્ય visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        જો (jumpingOver()) પાછા;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        જો (!m_params) {
            bad_ગાંઠ_Type(nodep);
            પાછા;
        }
        check_ગાંઠ_Info(nodep);
        જો (m_checkOnly) {
            iterateChildrenConst(nodep);
            પાછા;
        }
        જો (!optimizable()) પાછા;

        size_t iter_ગણતરી = 0;
        અચલ size_t iterLimit = v3Global.opt.unrollLimit();
        જ્યારે (true) {
            જો (iter_ગણતરી > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                તોડો;
            }
            ++iter_ગણતરી;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            જો (jumpingOver()) તોડો;
        }

        જો (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    શૂન્ય visit(AstLoopTest* nodep) override {
        જો (jumpingOver()) પાછા;
        check_ગાંઠ_Info(nodep);
        iterateConst(nodep->condp());
        જો (m_checkOnly || !optimizable()) પાછા;
        જો (fetchConst(nodep->condp())->અંક().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    શૂન્ય visit(AstStop* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_ગાંઠ_Info(nodep);
    }
    શૂન્ય visit(AstFuncRef* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!optimizable()) પાછા;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_ગાંઠ_Info(nodep);
        જો (!m_params) {
            bad_ગાંઠ_Type(nodep);
            પાછા;
        }
        Ast_ગાંઠ_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        જો (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        જો (funcp->recursive()) {
            જો (m_recurse_ગણતરી >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                પાછા;
            }
            ++m_recurse_ગણતરી;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_ગાંઠ*, Ast_ગાંઠ_પદાવલિ*> old_કિંમતો;

        જો (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_કિંમતો](AstVar* varp) {
                જો (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    જો (Ast_ગાંઠ_પદાવલિ* અચલ valuep = fetch_કિંમત_Null(varp)) {
                        Ast_ગાંઠ_પદાવલિ* અચલ nvaluep = newTrackedClone(valuep);
                        old_કિંમતો.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_કિંમતો](Ast_ગાંઠ_પદાવલિ* exprp) {
                જો (VN_IS(exprp, Const)) પાછા;  // Speed up as won't change
                જો (Ast_ગાંઠ_પદાવલિ* અચલ valuep = fetch_કિંમત_Null(exprp)) {
                    Ast_ગાંઠ_પદાવલિ* અચલ nvaluep = newTrackedClone(valuep);
                    old_કિંમતો.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_ગાંઠ_પદાવલિ*>> port_કિંમતો;
        માટે (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* અચલ portp = it->first;
            Ast_ગાંઠ* અચલ pinp = it->second->exprp();
            જો (pinp) {  // Else too few arguments in function call - ignore it
                જો (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    પાછા;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_કિંમતો.emplace_back(std::make_pair(portp, newTrackedClone(fetch_કિંમત(pinp))));
            }
        }
        // Apply value to the function
        જો (!m_checkOnly && optimizable())
            માટે (અચલ auto& it : port_કિંમતો) {
                જો (!m_checkOnly && optimizable()) new_કિંમત(it.first, it.second);
            }
        Sim_ઢગલો_ગાંઠ ઢગલો_ગાંઠ{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ઢગલો.ધકેલો_back(&ઢગલો_ગાંઠ);
        જો (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                જો (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_ઢગલો.કાઢો_back();
        અચલ Ast_ગાંઠ_પદાવલિ* returnp = nullptr;
        જો (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_કિંમત(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        માટે (અચલ auto& it : old_કિંમતો) {
            જો (it.second) new_કિંમત(it.first, it.second);
        }
        જો (returnp) new_કિંમત(nodep, returnp);
        જો (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_ગણતરી > 0, nodep, "recurse underflow");
            --m_recurse_ગણતરી;
        }
    }

    શૂન્ય visit(AstVar* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!m_params) {
            bad_ગાંઠ_Type(nodep);
            પાછા;
        }
    }

    શૂન્ય visit(AstScopeName* /*nodep*/) override {
        જો (jumpingOver()) પાછા;
        // Ignore
    }

    શૂન્ય visit(AstSFormatArg* nodep) override {
        check_ગાંઠ_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;

        Ast_ગાંઠ* nextArgp = nodep->exprsp();
        string result;
        અચલ string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        માટે (; pos != format.cend(); ++pos) {
            જો (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } નહીંતર જો (!inPct) {  // Normal text
                result += *pos;
            } નહીંતર {  // Format character
                જો (std::isdigit(pos[0])) {
                    width += pos[0];
                    ચાલુ;
                }

                inPct = false;

                જો (V3_સંખ્યા::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_ગાંઠ* અચલ argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* અચલ fargp = VN_CAST(argp, SFormatArg);
                    Ast_ગાંઠ* અચલ subargp = fargp ? fargp->exprp() : argp;
                    AstConst* અચલ constp = fetchConstNull(subargp);
                    અચલ VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    જો (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        તોડો;
                    }
                    અચલ string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->અંક().displayedEnum(fargp, pformat)
                                  : constp->અંક().displayed(nodep, pformat, formatAttr);
                } નહીંતર {
                    પસંદગી (std::tolower(pos[0])) {
                    સ્થિતિ '%': result += "%"; તોડો;
                    સ્થિતિ 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        તોડો;
                    મૂળભૂત: clearOptimizable(nodep, "Unknown $display-like format code."); તોડો;
                    }
                }
            }
        }

        AstConst* અચલ resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_કિંમત(nodep, resultConstp);
        m_reclaimValuesp.ધકેલો_back(resultConstp);
    }

    શૂન્ય visit(AstDisplay* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!optimizable()) પાછા;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_ગાંઠ_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        જો (m_params) {
            અચલ AstConst* અચલ textp = fetchConst(nodep->fmtp());
            પસંદગી (nodep->displayType()) {
            સ્થિતિ VDisplayType::DT_DISPLAY:  // FALLTHRU
            સ્થિતિ VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); તોડો;
            સ્થિતિ VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); તોડો;
            સ્થિતિ VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); તોડો;
            સ્થિતિ VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); તોડો;
            સ્થિતિ VDisplayType::DT_WRITE:  // FALLTHRU
            મૂળભૂત: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    શૂન્ય visit(AstToStringN* nodep) override {
        જો (jumpingOver()) પાછા;
        જો (!optimizable()) પાછા;  // Accelerate
        check_ગાંઠ_Info(nodep);
        iterateChildrenConst(nodep);
        જો (m_checkOnly || !optimizable()) પાછા;
        અચલ std::string result = toStringRecurse(fetch_કિંમત(nodep->lhsp()));
        જો (!optimizable()) પાછા;
        AstConst* અચલ resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_કિંમત(nodep, resultConstp);
        m_reclaimValuesp.ધકેલો_back(resultConstp);
    }

    શૂન્ય visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    શૂન્ય visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_ગાંઠ_Type(nodep);
    }
    શૂન્ય visit(AstGetInitialRandomSeed* nodep) override { known_Bad_ગાંઠ_Type(nodep); }
    શૂન્ય visit(AstMemberSel* nodep) override { known_Bad_ગાંઠ_Type(nodep); }
    શૂન્ય visit(AstSampled* nodep) override { known_Bad_ગાંઠ_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (jumpingOver()) પાછા;
        bad_ગાંઠ_Type(nodep);
    }

    // MEMBERS - called by constructor
    શૂન્ય setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    શૂન્ય mainGuts(Ast_ગાંઠ* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

જાહેર:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    શૂન્ય clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_ગણતરી = 0;
        m_data_ગણતરી = 0;
        m_jumptargetp = nullptr;

        Ast_ગાંઠ::user1_Clear_વૃક્ષ();
        m_varAux.clear();
        ++m_constGeneration;
    }
    શૂન્ય mainTableCheck(Ast_ગાંઠ* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    શૂન્ય mainTableEmulate(Ast_ગાંઠ* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    શૂન્ય mainParamEmulate(Ast_ગાંઠ* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_ગાંઠ*> unusedRootps;
        માટે (Ast_ગાંઠ* અચલ nodep : m_reclaimValuesp) {
            જો (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        માટે (Ast_ગાંઠ* અચલ nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
};

#endif  // Guard
