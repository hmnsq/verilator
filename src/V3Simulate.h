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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Task.h"
#അന്തര്ഭാവ "V3Width.h"

#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <sstream>
#അന്തര്ഭാവ <stack>
#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_അട്ടി_കണ്ണി final {
പൊതു:
    // MEMBERS
    AstFuncRef* സ്ഥിരം m_funcp;
    V3TaskConnects* സ്ഥിരം m_tconnects;
    // CONSTRUCTORS
    Sim_അട്ടി_കണ്ണി(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_അട്ടി_കണ്ണി() = സ്ഥിരസ്ഥിതി;
};

class SimulateVisitor VL_NOT_FINAL : പൊതു VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

സ്വകാര്യ:
    // CONSTANTS
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ CALL_അട്ടി_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    സ്ഥിരം VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    എണ്ണൽ VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    ഘടന AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_കണ്ണി_വ്യഞ്ജകം* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = nullptr;
    };

    AstUser1Allocator<Ast_കണ്ണി, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    ഘടന ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, Ast_കണ്ണി* nodep) {
            എങ്കിൽ (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            എങ്കിൽ (m_nextFree == m_constps.size()) {
                m_constps.തള്ളുക_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* സ്ഥിരം constp = m_constps[m_nextFree++];
            constp->അക്കം().nodep(nodep);
            തിരികെ constp;
        }

        ~ConstAllocator() {
            വേണ്ടി (AstConst* സ്ഥിരം constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_കണ്ണി* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    പൂർണ്ണസംഖ്യ m_instr_എണ്ണം;  ///< Number of nodes
    പൂർണ്ണസംഖ്യ m_data_എണ്ണം;  ///< Bytes of data
    പൂർണ്ണസംഖ്യ m_recurse_എണ്ണം = 0;  // Now deep in current recursion
    Ast_കണ്ണി* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി_DType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<Sim_അട്ടി_കണ്ണി*> m_call_അട്ടി;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_കണ്ണി*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_സംഖ്യ(സ്ഥിരം V3_സംഖ്യ* nump, സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        എങ്കിൽ (സ്ഥിരം AstRefDType* സ്ഥിരം refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം stp = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType)) {
            എങ്കിൽ (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                വേണ്ടി (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ width = itemp->width();
                    സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = itemp->lsb();
                    സ്ഥിരം പൂർണ്ണസംഖ്യ msb = lsb + width - 1;
                    V3_സംഖ്യ field_അക്കം{nump, width};
                    field_അക്കം.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം childTypep = itemp->subDTypep()) {
                        out << pretty_സംഖ്യ(&field_അക്കം, childTypep);
                    } അല്ലെങ്കിൽ {
                        out << field_അക്കം;
                    }
                    എങ്കിൽ (itemp->nextp()) out << ", ";
                }
                out << "}";
                തിരികെ out.str();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം arrayp = VN_CAST(dtypep, PackArrayDType)) {
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                സ്ഥിരം പൂർണ്ണസംഖ്യ arrayElements = arrayp->elementsConst();
                വേണ്ടി (പൂർണ്ണസംഖ്യ element = 0; element < arrayElements; ++element) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ width = childTypep->width();
                    സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = width * element;
                    സ്ഥിരം പൂർണ്ണസംഖ്യ msb = lsb + width - 1;
                    V3_സംഖ്യ field_അക്കം{nump, width};
                    field_അക്കം.opSel(*nump, msb, lsb);
                    സ്ഥിരം പൂർണ്ണസംഖ്യ arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_സംഖ്യ(&field_അക്കം, childTypep);
                    എങ്കിൽ (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                തിരികെ out.str();
            }
        }
        തിരികെ nump->ascii();
    }

    // Checking METHODS
പൊതു:
    /// Call other-this function on all new *non-constant* var references
    മായാ ശൂന്യം varRefCb(AstVarRef* nodep) {}

    ശൂന്യം clearOptimizable(Ast_കണ്ണി* nodep /*null ok*/, സ്ഥിരം string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        എങ്കിൽ (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            എങ്കിൽ (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                എങ്കിൽ (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream അട്ടി;
            പൂർണ്ണസംഖ്യ n = 0;
            വേണ്ടി (സ്ഥിരം auto& callstack : vlstd::reverse_view(m_call_അട്ടി)) {
                സ്ഥിരം AstFuncRef* സ്ഥിരം funcp = callstack->m_funcp;
                അട്ടി << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                വേണ്ടി (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    സ്ഥിരം AstVar* സ്ഥിരം portp = conIt->first;
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinp = conIt->second->exprp();
                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = pinp->dtypep();
                    എങ്കിൽ (AstConst* സ്ഥിരം valp = fetchConstNull(pinp)) {
                        അട്ടി << "\n           " << portp->prettyName() << " = "
                              << pretty_സംഖ്യ(&valp->അക്കം(), dtypep);
                    } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = fetch_മൂല്യം_Null(pinp)) {
                        അട്ടി << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                എങ്കിൽ (++n > CALL_അട്ടി_MAX) {
                    അട്ടി << "\n           ... stack truncated";
                    നിർത്തുക;
                }
            }
            m_whyNotOptimizable += അട്ടി.str();
        }
    }
    bool optimizable() സ്ഥിരം { തിരികെ m_whyNotNodep == nullptr; }
    string whyNotMessage() സ്ഥിരം { തിരികെ m_whyNotOptimizable; }
    Ast_കണ്ണി* whyNotNodep() സ്ഥിരം { തിരികെ m_whyNotNodep; }

    bool isAssignDly() സ്ഥിരം { തിരികെ m_anyAssignDly; }
    bool isImpure() സ്ഥിരം { തിരികെ m_isImpure; }
    bool isCoverage() സ്ഥിരം { തിരികെ m_isCoverage; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം { തിരികെ m_instr_എണ്ണം; }
    പൂർണ്ണസംഖ്യ data_എണ്ണം() സ്ഥിരം { തിരികെ m_data_എണ്ണം; }

    // Simulation METHODS
സ്വകാര്യ:
    AstConst* allocConst(Ast_കണ്ണി* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        തിരികെ m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

പൊതു:
    ശൂന്യം new_മൂല്യം(Ast_കണ്ണി* nodep, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->അക്കം().opAssign(constp->അക്കം());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (fetch_മൂല്യം_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_മൂല്യം(nodep, newTrackedClone(const_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(valuep)));
        }
    }
    ശൂന്യം new_Out_മൂല്യം(Ast_കണ്ണി* nodep, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->അക്കം().opAssign(constp->അക്കം());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (fetch_Out_മൂല്യം_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_മൂല്യം(nodep, newTrackedClone(const_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(valuep)));
        }
    }

സ്വകാര്യ:
    Ast_കണ്ണി_വ്യഞ്ജകം* newTrackedClone(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = nodep->clone_വൃക്ഷം(false);
        m_reclaimValuesp.തള്ളുക_back(newp);
        തിരികെ newp;
    }
    AstConst* newConst(Ast_കണ്ണി* nodep) {
        // Set a constant value for this node
        എങ്കിൽ (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* സ്ഥിരം constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            തിരികെ constp;
        }
        തിരികെ fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_കണ്ണി* nodep) {
        // Set a var-output constant value for this node
        എങ്കിൽ (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* സ്ഥിരം constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            തിരികെ constp;
        }
        തിരികെ fetchOutConst(nodep);
    }

പൊതു:
    Ast_കണ്ണി_വ്യഞ്ജകം* fetch_മൂല്യം_Null(Ast_കണ്ണി* nodep) { തിരികെ m_varAux(nodep).valuep; }

സ്വകാര്യ:
    Ast_കണ്ണി_വ്യഞ്ജകം* fetch_Out_മൂല്യം_Null(Ast_കണ്ണി* nodep) { തിരികെ m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_കണ്ണി* nodep) { തിരികെ VN_CAST(fetch_മൂല്യം_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_കണ്ണി* nodep) {
        തിരികെ VN_CAST(fetch_Out_മൂല്യം_Null(nodep), Const);
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* fetch_മൂല്യം(Ast_കണ്ണി* nodep) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = fetch_മൂല്യം_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        തിരികെ valuep;
    }
    AstConst* fetchConst(Ast_കണ്ണി* nodep) {
        AstConst* സ്ഥിരം constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        തിരികെ constp;
    }
    AstConst* fetchOutConst(Ast_കണ്ണി* nodep) {
        AstConst* സ്ഥിരം constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        തിരികെ constp;
    }

പൊതു:
    V3_സംഖ്യ* fetch_സംഖ്യ_Null(Ast_കണ്ണി* nodep) {
        AstConst* സ്ഥിരം constp = fetchConstNull(nodep);
        എങ്കിൽ (constp) തിരികെ &constp->അക്കം();
        തിരികെ nullptr;
    }
    V3_സംഖ്യ* fetch_Out_സംഖ്യ_Null(Ast_കണ്ണി* nodep) {
        AstConst* സ്ഥിരം constp = fetchOutConstNull(nodep);
        എങ്കിൽ (constp) തിരികെ &constp->അക്കം();
        തിരികെ nullptr;
    }

സ്വകാര്യ:
    ശൂന്യം set_മൂല്യം(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    ശൂന്യം set_Out_മൂല്യം(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    ശൂന്യം check_കണ്ണി_Info(Ast_കണ്ണി* nodep, bool ignorePredict = false) {
        എങ്കിൽ (m_checkOnly) {
            m_instr_എണ്ണം += nodep->instr_എണ്ണം();
            m_data_എണ്ണം += nodep->width();
        }
        എങ്കിൽ (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        എങ്കിൽ (!nodep->isPure()) m_isImpure = true;
    }

    ശൂന്യം known_Bad_കണ്ണി_Type(Ast_കണ്ണി* nodep) {
        // Call for node types we know we can't handle
        എങ്കിൽ (jumpingOver()) തിരികെ;
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    ശൂന്യം bad_കണ്ണി_Type(Ast_കണ്ണി* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            സ്ഥിതം std::set<VNType> s_typePrinted;
            സ്ഥിരം auto pair = s_typePrinted.emplace(nodep->type());
            എങ്കിൽ (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_കണ്ണി* varOrScope(AstVarRef* nodep) സ്ഥിരം {
        Ast_കണ്ണി* vscp;
        എങ്കിൽ (m_scoped) {
            vscp = nodep->varScopep();
        } അല്ലെങ്കിൽ {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        തിരികെ vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() സ്ഥിരം { തിരികെ m_jumptargetp; }
    ശൂന്യം assign_Out_മൂല്യം(സ്ഥിരം Ast_കണ്ണി_Assign* nodep, Ast_കണ്ണി* vscp, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        എങ്കിൽ (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_മൂല്യം(vscp, valuep);
        } അല്ലെങ്കിൽ {
            new_മൂല്യം(vscp, valuep);
            set_Out_മൂല്യം(vscp, fetch_മൂല്യം(vscp));
        }
    }

    string toStringRecurse(Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        // Return string representation of a value, or clearOptimizable
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം avaluep = VN_CAST(valuep, Const)) {
            തിരികെ "'h"s + avaluep->അക്കം().displayed(valuep, "%0x");
        }
        എങ്കിൽ (സ്ഥിരം AstConsArray* സ്ഥിരം avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            വേണ്ടി (പൂർണ്ണസംഖ്യ n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            തിരികെ result;
        }
        എങ്കിൽ (സ്ഥിരം AstConsStruct* സ്ഥിരം avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            വേണ്ടി (സ്ഥിരം AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            തിരികെ result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        തിരികെ "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_കണ്ണി_വ്യഞ്ജകം* newInitValuep(Ast_കണ്ണി* nodep, Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elementp = newInitValuep(nodep, arrayp->subDTypep());
            തിരികെ elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* സ്ഥിരം structp = VN_CAST(dtypep, StructDType);
        എങ്കിൽ (structp && !structp->packed()) {
            AstConsStruct* സ്ഥിരം newp = new AstConsStruct{nodep->fileline(), structp};
            വേണ്ടി (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം memberValuep = newInitValuep(nodep, memberp->subDTypep());
                എങ്കിൽ (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_വൃക്ഷം(), newp);
                    തിരികെ nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            തിരികെ newp;
        }
        എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = dtypep->basicp()) {
            AstConst* സ്ഥിരം constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_മൂല്യം{}, basicp->widthMin(), 0};
            എങ്കിൽ (basicp->isZeroInit()) {
                constp->അക്കം() = V3_സംഖ്യ{nodep, basicp};
            } അല്ലെങ്കിൽ {
                constp->അക്കം().setAllBitsX();
            }
            തിരികെ constp;
        }
        തിരികെ nullptr;
    }
    ശൂന്യം initVar(AstVar* nodep) {
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_മൂല്യം(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_വൃക്ഷം(), valuep);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_കണ്ണി* സ്ഥിരം vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        എങ്കിൽ (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), കണ്ണി_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        എങ്കിൽ (nodep->access().isWriteOrRW()) {
            എങ്കിൽ (m_inDlyAssign) {
                എങ്കിൽ (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    എങ്കിൽ (m_checkOnly) varRefCb(nodep);
                }
            } അല്ലെങ്കിൽ {  // nondly asn
                എങ്കിൽ (!(m_varAux(vscp).usage & VU_LV)) {
                    എങ്കിൽ (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    എങ്കിൽ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        എങ്കിൽ (nodep->access().isReadOrRW()) {
            എങ്കിൽ (!(m_varAux(vscp).usage & VU_RV)) {
                എങ്കിൽ (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                സ്ഥിരം bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep
                    = varIsConst ? fetch_മൂല്യം_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                എങ്കിൽ (varIsConst && valuep) {
                    എങ്കിൽ (!m_checkOnly && optimizable()) new_മൂല്യം(vscp, valuep);
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_കണ്ണി_വ്യഞ്ജകം* valuep = fetch_മൂല്യം_Null(vscp);
            എങ്കിൽ (!valuep) {
                എങ്കിൽ (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } അല്ലെങ്കിൽ {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            set_മൂല്യം(nodep, valuep);
        }
    }
    ശൂന്യം visit(AstVarXRef* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (m_scoped) {
            bad_കണ്ണി_Type(nodep);
            തിരികെ;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!m_params) {
            bad_കണ്ണി_Type(nodep);
            തിരികെ;
        }
        എങ്കിൽ (nodep->dpiImport()) {
            എങ്കിൽ (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        എങ്കിൽ (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstInitialAutomaticStmt* nodep) override {
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstInitialStaticStmt* nodep) override {
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstInitialStatic* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!m_params) {
            bad_കണ്ണി_Type(nodep);
            തിരികെ;
        }
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        UINFO(5, "   IF " << nodep);
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } അല്ലെങ്കിൽ {
            iterateAndNextConstNull(nodep->condp());
            എങ്കിൽ (optimizable()) {
                എങ്കിൽ (fetchConst(nodep->condp())->അക്കം().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } അല്ലെങ്കിൽ {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    ശൂന്യം visit(AstConst* nodep) override {
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        new_മൂല്യം(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    ശൂന്യം visit(AstConsArray* nodep) override {
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        AstConsArray* സ്ഥിരം valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_മൂല്യം(nodep->defaultp())->clone_വൃക്ഷം(false) : nullptr};
        വേണ്ടി (സ്ഥിരം AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_മൂല്യം(itemp->valuep())->clone_വൃക്ഷം(false));
        }
        m_reclaimValuesp.തള്ളുക_back(valuep);
        set_മൂല്യം(nodep, valuep);
    }
    ശൂന്യം visit(AstConsStruct* nodep) override {
        എങ്കിൽ (!m_params) {
            bad_കണ്ണി_Type(nodep);
            തിരികെ;
        }
        check_കണ്ണി_Info(nodep);
        വേണ്ടി (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        AstConsStruct* സ്ഥിരം valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        വേണ്ടി (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_മൂല്യം(itemp->valuep())->clone_വൃക്ഷം(false)});
        }
        m_reclaimValuesp.തള്ളുക_back(valuep);
        set_മൂല്യം(nodep, valuep);
    }
    ശൂന്യം visit(AstCvtUnpackToUnpack* nodep) override {
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_മൂല്യം(nodep->fromp())->clone_വൃക്ഷം(false), nodep->dtypep()};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.തള്ളുക_back(valuep);
        set_മൂല്യം(nodep, valuep);
    }
    ശൂന്യം visit(AstConsArrayItem* nodep) override {
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstEnumItemRef* nodep) override {
        check_കണ്ണി_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        Ast_കണ്ണി* സ്ഥിരം valuep = nodep->itemp()->valuep();
        എങ്കിൽ (valuep) {
            iterateAndNextConstNull(valuep);
            എങ്കിൽ (!optimizable()) തിരികെ;
            new_മൂല്യം(nodep, fetch_മൂല്യം(valuep));
        } അല്ലെങ്കിൽ {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Uniop* nodep) override {
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        nodep->സംഖ്യ_Operate(newConst(nodep)->അക്കം(), fetchConst(nodep->lhsp())->അക്കം());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    ശൂന്യം aggregateEqRecurse(V3_സംഖ്യ& resultr, Ast_കണ്ണി* nodep, bool caseEq, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp,
                            Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം lconstp = VN_CAST(lhsp, Const)) {
            സ്ഥിരം V3_സംഖ്യ& lnum = lconstp->അക്കം();
            സ്ഥിരം V3_സംഖ്യ& rnum = VN_AS(rhsp, Const)->അക്കം();
            V3_സംഖ്യ eq{nodep, 1};
            എങ്കിൽ (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } അല്ലെങ്കിൽ എങ്കിൽ (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } അല്ലെങ്കിൽ എങ്കിൽ (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } അല്ലെങ്കിൽ {
                eq.opEq(lnum, rnum);
            }
            സ്ഥിരം V3_സംഖ്യ prev = resultr;
            resultr.opLogAnd(prev, eq);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstConsArray* സ്ഥിരം lconsp = VN_CAST(lhsp, ConsArray)) {
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            വേണ്ടി (പൂർണ്ണസംഖ്യ n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } അല്ലെങ്കിൽ {
            സ്ഥിരം AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            വേണ്ടി (സ്ഥിരം AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    ശൂന്യം handleAggregateCompare(Ast_കണ്ണി_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        സ്ഥിരം bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        സ്ഥിരം bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_സംഖ്യ& resultr = newConst(nodep)->അക്കം();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_മൂല്യം(nodep->lhsp()),
                           fetch_മൂല്യം(nodep->rhsp()));
        എങ്കിൽ (neq) {
            സ്ഥിരം V3_സംഖ്യ eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Biop* nodep) override {
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        എങ്കിൽ (!VN_IS(fetch_മൂല്യം(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            തിരികെ;
        }
        AstConst* സ്ഥിരം valuep = newConst(nodep);
        nodep->സംഖ്യ_Operate(newConst(nodep)->അക്കം(), fetchConst(nodep->lhsp())->അക്കം(),
                             fetchConst(nodep->rhsp())->അക്കം());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        എങ്കിൽ (!m_params && VN_IS(nodep, Sel) && valuep->അക്കം().isAnyX()) {
            സ്ഥിരം V3_സംഖ്യ അക്കം{valuep, valuep->width(), valuep->അക്കം()};
            valuep->അക്കം().opBitsOne(അക്കം);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Triop* nodep) override {
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        nodep->സംഖ്യ_Operate(newConst(nodep)->അക്കം(), fetchConst(nodep->lhsp())->അക്കം(),
                             fetchConst(nodep->rhsp())->അക്കം(), fetchConst(nodep->thsp())->അക്കം());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Quadop* nodep) override {
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        nodep->സംഖ്യ_Operate(newConst(nodep)->അക്കം(), fetchConst(nodep->lhsp())->അക്കം(),
                             fetchConst(nodep->rhsp())->അക്കം(), fetchConst(nodep->thsp())->അക്കം(),
                             fetchConst(nodep->fhsp())->അക്കം());
    }
    ശൂന്യം visit(AstLogAnd* nodep) override {
        // Need to short circuit
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } അല്ലെങ്കിൽ {
            iterateConst(nodep->lhsp());
            എങ്കിൽ (!optimizable()) തിരികെ;
            എങ്കിൽ (fetchConst(nodep->lhsp())->അക്കം().isNeqZero()) {
                iterateConst(nodep->rhsp());
                എങ്കിൽ (!optimizable()) തിരികെ;
                new_മൂല്യം(nodep, fetch_മൂല്യം(nodep->rhsp()));
            } അല്ലെങ്കിൽ {
                new_മൂല്യം(nodep, fetch_മൂല്യം(nodep->lhsp()));  // a zero
            }
        }
    }
    ശൂന്യം visit(AstLogOr* nodep) override {
        // Need to short circuit
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } അല്ലെങ്കിൽ {
            iterateConst(nodep->lhsp());
            എങ്കിൽ (!optimizable()) തിരികെ;
            എങ്കിൽ (fetchConst(nodep->lhsp())->അക്കം().isNeqZero()) {
                new_മൂല്യം(nodep, fetch_മൂല്യം(nodep->lhsp()));  // a one
            } അല്ലെങ്കിൽ {
                iterateConst(nodep->rhsp());
                എങ്കിൽ (!optimizable()) തിരികെ;
                new_മൂല്യം(nodep, fetch_മൂല്യം(nodep->rhsp()));
            }
        }
    }
    ശൂന്യം visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } അല്ലെങ്കിൽ {
            iterateConst(nodep->lhsp());
            എങ്കിൽ (!optimizable()) തിരികെ;
            എങ്കിൽ (fetchConst(nodep->lhsp())->അക്കം().isEqZero()) {
                സ്ഥിരം AstConst cnst{nodep->fileline(), AstConst::Widthed_മൂല്യം{}, 1, 1};  // a one
                new_മൂല്യം(nodep, &cnst);  // a one
            } അല്ലെങ്കിൽ {
                iterateConst(nodep->rhsp());
                എങ്കിൽ (!optimizable()) തിരികെ;
                new_മൂല്യം(nodep, fetch_മൂല്യം(nodep->rhsp()));
            }
        }
    }
    ശൂന്യം visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } അല്ലെങ്കിൽ {
            iterateConst(nodep->condp());
            എങ്കിൽ (!optimizable()) തിരികെ;
            എങ്കിൽ (fetchConst(nodep->condp())->അക്കം().isNeqZero()) {
                iterateConst(nodep->thenp());
                എങ്കിൽ (!optimizable()) തിരികെ;
                new_മൂല്യം(nodep, fetch_മൂല്യം(nodep->thenp()));
            } അല്ലെങ്കിൽ {
                iterateConst(nodep->elsep());
                എങ്കിൽ (!optimizable()) തിരികെ;
                new_മൂല്യം(nodep, fetch_മൂല്യം(nodep->elsep()));
            }
        }
    }

    ശൂന്യം handleAssignElement(Ast_കണ്ണി_Assign* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* മൂല്യം_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> selps;
        Ast_കണ്ണി_വ്യഞ്ജകം* fromp = lhsp;
        ആകുമ്പോൾ (true) {
            എങ്കിൽ (AstArraySel* സ്ഥിരം selp = VN_CAST(fromp, ArraySel)) {
                check_കണ്ണി_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.തള്ളുക_back(selp);
                fromp = selp->fromp();
            } അല്ലെങ്കിൽ എങ്കിൽ (AstStructSel* സ്ഥിരം selp = VN_CAST(fromp, StructSel)) {
                check_കണ്ണി_Info(selp);
                selps.തള്ളുക_back(selp);
                fromp = selp->fromp();
            } അല്ലെങ്കിൽ {
                നിർത്തുക;
            }
        }
        AstVarRef* സ്ഥിരം varrefp = VN_CAST(fromp, VarRef);
        എങ്കിൽ (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            തിരികെ;
        }
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        Ast_കണ്ണി* സ്ഥിരം vscp = varOrScope(varrefp);
        Ast_കണ്ണി_വ്യഞ്ജകം* rootp = fetch_Out_മൂല്യം_Null(vscp);
        എങ്കിൽ (!rootp) rootp = fetch_മൂല്യം_Null(vscp);
        എങ്കിൽ (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            തിരികെ;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_കണ്ണി_വ്യഞ്ജകം* itemp = rootp;
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം selp : vlstd::reverse_view(selps)) {
            എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* സ്ഥിരം consp = VN_AS(itemp, ConsArray);
                സ്ഥിരം uint32_t index = fetchConst(aselp->bitp())->toUInt();
                സ്ഥിരം പൂർണ്ണസംഖ്യ elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                എങ്കിൽ (index >= static_cast<uint32_t>(elements)) {
                    തിരികെ;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                എങ്കിൽ (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_വൃക്ഷം(false);
                    consp->valuep(index, itemp);
                }
            } അല്ലെങ്കിൽ {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = newTrackedClone(fetch_മൂല്യം(മൂല്യം_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.തള്ളുക_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_മൂല്യം(nodep, vscp, rootp);
    }
    ശൂന്യം handleAssignSel(Ast_കണ്ണി_Assign* nodep, AstSel* selp, Ast_കണ്ണി_വ്യഞ്ജകം* മൂല്യം_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_സംഖ്യ lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_കണ്ണി* സ്ഥിരം vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        എങ്കിൽ (AstConst* സ്ഥിരം vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } അല്ലെങ്കിൽ എങ്കിൽ (AstConst* സ്ഥിരം vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } അല്ലെങ്കിൽ {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_മൂല്യം{},
                                     varrefp->varp()->widthMin(), 0};
            എങ്കിൽ (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->അക്കം().setAllBits0();
            } അല്ലെങ്കിൽ {
                outconstp->അക്കം().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->അക്കം().opSelInto(fetchConst(മൂല്യം_Fromp)->അക്കം(), lsb, selp->widthConst());
        assign_Out_മൂല്യം(nodep, vscp, outconstp);
    }
    ശൂന്യം handleAssignSelRecurse(Ast_കണ്ണി_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_സംഖ്യ& lsbRef, പൂർണ്ണസംഖ്യ depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_കണ്ണി_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->അക്കം();
            തിരികെ;  // And presumably still optimizable()
        }
        എങ്കിൽ (AstSel* സ്ഥിരം subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_സംഖ്യ sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            എങ്കിൽ (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->അക്കം());
            }
        } അല്ലെങ്കിൽ {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    ശൂന്യം handleAssignRecurse(Ast_കണ്ണി_Assign* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* മൂല്യം_Fromp) {
        എങ്കിൽ (!optimizable()) തിരികെ;
        എങ്കിൽ (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            എങ്കിൽ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                തിരികെ;
            }
            handleAssignElement(nodep, lhsp, മൂല്യം_Fromp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstConcat* സ്ഥിരം selp = VN_CAST(lhsp, Concat)) {
            check_കണ്ണി_Info(selp);
            എങ്കിൽ (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                തിരികെ;
            }
            // Split value into left and right concat values
            എങ്കിൽ (!m_checkOnly) {
                {
                    AstConst* സ്ഥിരം outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_മൂല്യം{},
                                       selp->lhsp()->width(), 0};
                    outconstp->അക്കം().opSel(fetchConst(മൂല്യം_Fromp)->അക്കം(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_മൂല്യം(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_വൃക്ഷം(), outconstp);
                }
                {
                    AstConst* സ്ഥിരം outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_മൂല്യം{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->അക്കം().opSel(fetchConst(മൂല്യം_Fromp)->അക്കം(),
                                           selp->rhsp()->width() - 1, 0);
                    new_മൂല്യം(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_വൃക്ഷം(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstReplicate* സ്ഥിരം selp = VN_CAST(lhsp, Replicate)) {
            check_കണ്ണി_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* സ്ഥിരം countp = VN_CAST(selp->countp(), Const);
            എങ്കിൽ (!countp || !countp->അക്കം().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                തിരികെ;
            }
            handleAssignRecurse(nodep, selp->srcp(), മൂല്യം_Fromp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(lhsp, Sel)) {
            എങ്കിൽ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                തിരികെ;
            }
            handleAssignSel(nodep, selp, മൂല്യം_Fromp);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(lhsp, VarRef)) {
            എങ്കിൽ (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } അല്ലെങ്കിൽ {
                Ast_കണ്ണി* സ്ഥിരം vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_മൂല്യം(nodep, vscp, fetch_മൂല്യം(മൂല്യം_Fromp));
            }
        } അല്ലെങ്കിൽ {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        എങ്കിൽ (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, AssignDly)) {
            എങ്കിൽ (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        എങ്കിൽ (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            തിരികെ;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    ശൂന്യം visit(AstArraySel* nodep) override {
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (സ്ഥിരം AstConsArray* സ്ഥിരം consp = VN_CAST(fetch_മൂല്യം_Null(nodep->fromp()), ConsArray)) {
            സ്ഥിരം AstConst* സ്ഥിരം indexp = fetchConst(nodep->bitp());
            സ്ഥിരം uint32_t offset = indexp->അക്കം().toUInt();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം itemp = consp->valuep(offset);
            എങ്കിൽ (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } അല്ലെങ്കിൽ {
                set_മൂല്യം(nodep, itemp);
            }
        } അല്ലെങ്കിൽ {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    ശൂന്യം visit(AstStructSel* nodep) override {
        എങ്കിൽ (!m_params) {
            bad_കണ്ണി_Type(nodep);
            തിരികെ;
        }
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        AstConsStruct* സ്ഥിരം consp = VN_AS(fetch_മൂല്യം(nodep->fromp()), ConsStruct);
        set_മൂല്യം(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    ശൂന്യം visit(AstSliceSel* nodep) override {
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        // Fetch the base constant array
        എങ്കിൽ (സ്ഥിരം AstConsArray* സ്ഥിരം consp = VN_CAST(fetch_മൂല്യം_Null(nodep->fromp()), ConsArray)) {
            സ്ഥിരം VNumRange& sliceRange = nodep->declRange();
            സ്ഥിരം uint32_t sliceElements = sliceRange.elements();
            സ്ഥിരം പൂർണ്ണസംഖ്യ sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* സ്ഥിരം newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            വേണ്ടി (uint32_t idx = 0; idx < sliceElements; ++idx) {
                സ്ഥിരം uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_വൃക്ഷം(false));
            }
            // Assign the new constant array and track it for later deletion
            set_മൂല്യം(nodep, newConsp);
            m_reclaimValuesp.തള്ളുക_back(newConsp);
        } അല്ലെങ്കിൽ {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    ശൂന്യം visit(AstBegin* nodep) override {
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstCase* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        UINFO(5, "   CASE " << nodep);
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            വേണ്ടി (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                എങ്കിൽ (!itemp->isDefault()) {
                    വേണ്ടി (Ast_കണ്ണി* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        എങ്കിൽ (hit) നിർത്തുക;
                        iterateAndNextConstNull(ep);
                        എങ്കിൽ (optimizable()) {
                            V3_സംഖ്യ match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->അക്കം(), fetchConst(ep)->അക്കം());
                            എങ്കിൽ (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            വേണ്ടി (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                എങ്കിൽ (hit) നിർത്തുക;
                എങ്കിൽ (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    ശൂന്യം visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        എങ്കിൽ (jumpingOver()) തിരികെ;
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstComment*) override {}

    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        check_കണ്ണി_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        എങ്കിൽ (!optimizable()) തിരികെ;
        iterateAndNextConstNull(nodep->resultp());
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        new_മൂല്യം(nodep, fetch_മൂല്യം(nodep->resultp()));
    }

    ശൂന്യം visit(AstJumpBlock* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ശൂന്യം visit(AstJumpGo* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        എങ്കിൽ (jumpingOver()) തിരികെ;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        എങ്കിൽ (!m_params) {
            bad_കണ്ണി_Type(nodep);
            തിരികെ;
        }
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (m_checkOnly) {
            iterateChildrenConst(nodep);
            തിരികെ;
        }
        എങ്കിൽ (!optimizable()) തിരികെ;

        size_t iter_എണ്ണം = 0;
        സ്ഥിരം size_t iterLimit = v3Global.opt.unrollLimit();
        ആകുമ്പോൾ (true) {
            എങ്കിൽ (iter_എണ്ണം > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                നിർത്തുക;
            }
            ++iter_എണ്ണം;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            എങ്കിൽ (jumpingOver()) നിർത്തുക;
        }

        എങ്കിൽ (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        check_കണ്ണി_Info(nodep);
        iterateConst(nodep->condp());
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        എങ്കിൽ (fetchConst(nodep->condp())->അക്കം().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    ശൂന്യം visit(AstStop* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_കണ്ണി_Info(nodep);
    }
    ശൂന്യം visit(AstFuncRef* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_കണ്ണി_Info(nodep);
        എങ്കിൽ (!m_params) {
            bad_കണ്ണി_Type(nodep);
            തിരികെ;
        }
        Ast_കണ്ണി_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        എങ്കിൽ (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        എങ്കിൽ (funcp->recursive()) {
            എങ്കിൽ (m_recurse_എണ്ണം >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                തിരികെ;
            }
            ++m_recurse_എണ്ണം;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_കണ്ണി*, Ast_കണ്ണി_വ്യഞ്ജകം*> old_മൂല്യങ്ങൾ;

        എങ്കിൽ (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_മൂല്യങ്ങൾ](AstVar* varp) {
                എങ്കിൽ (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = fetch_മൂല്യം_Null(varp)) {
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nvaluep = newTrackedClone(valuep);
                        old_മൂല്യങ്ങൾ.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_മൂല്യങ്ങൾ](Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
                എങ്കിൽ (VN_IS(exprp, Const)) തിരികെ;  // Speed up as won't change
                എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = fetch_മൂല്യം_Null(exprp)) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nvaluep = newTrackedClone(valuep);
                    old_മൂല്യങ്ങൾ.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*>> port_മൂല്യങ്ങൾ;
        വേണ്ടി (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* സ്ഥിരം portp = it->first;
            Ast_കണ്ണി* സ്ഥിരം pinp = it->second->exprp();
            എങ്കിൽ (pinp) {  // Else too few arguments in function call - ignore it
                എങ്കിൽ (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    തിരികെ;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_മൂല്യങ്ങൾ.emplace_back(std::make_pair(portp, newTrackedClone(fetch_മൂല്യം(pinp))));
            }
        }
        // Apply value to the function
        എങ്കിൽ (!m_checkOnly && optimizable())
            വേണ്ടി (സ്ഥിരം auto& it : port_മൂല്യങ്ങൾ) {
                എങ്കിൽ (!m_checkOnly && optimizable()) new_മൂല്യം(it.first, it.second);
            }
        Sim_അട്ടി_കണ്ണി അട്ടി_കണ്ണി{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_അട്ടി.തള്ളുക_back(&അട്ടി_കണ്ണി);
        എങ്കിൽ (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                എങ്കിൽ (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_അട്ടി.എടുക്കുക_back();
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* returnp = nullptr;
        എങ്കിൽ (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_മൂല്യം(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        വേണ്ടി (സ്ഥിരം auto& it : old_മൂല്യങ്ങൾ) {
            എങ്കിൽ (it.second) new_മൂല്യം(it.first, it.second);
        }
        എങ്കിൽ (returnp) new_മൂല്യം(nodep, returnp);
        എങ്കിൽ (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_എണ്ണം > 0, nodep, "recurse underflow");
            --m_recurse_എണ്ണം;
        }
    }

    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!m_params) {
            bad_കണ്ണി_Type(nodep);
            തിരികെ;
        }
    }

    ശൂന്യം visit(AstScopeName* /*nodep*/) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        // Ignore
    }

    ശൂന്യം visit(AstSFormatArg* nodep) override {
        check_കണ്ണി_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;

        Ast_കണ്ണി* nextArgp = nodep->exprsp();
        string result;
        സ്ഥിരം string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        വേണ്ടി (; pos != format.cend(); ++pos) {
            എങ്കിൽ (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } അല്ലെങ്കിൽ എങ്കിൽ (!inPct) {  // Normal text
                result += *pos;
            } അല്ലെങ്കിൽ {  // Format character
                എങ്കിൽ (std::isdigit(pos[0])) {
                    width += pos[0];
                    തുടരുക;
                }

                inPct = false;

                എങ്കിൽ (V3_സംഖ്യ::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_കണ്ണി* സ്ഥിരം argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* സ്ഥിരം fargp = VN_CAST(argp, SFormatArg);
                    Ast_കണ്ണി* സ്ഥിരം subargp = fargp ? fargp->exprp() : argp;
                    AstConst* സ്ഥിരം constp = fetchConstNull(subargp);
                    സ്ഥിരം VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    എങ്കിൽ (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        നിർത്തുക;
                    }
                    സ്ഥിരം string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->അക്കം().displayedEnum(fargp, pformat)
                                  : constp->അക്കം().displayed(nodep, pformat, formatAttr);
                } അല്ലെങ്കിൽ {
                    തിരഞ്ഞെടുപ്പ് (std::tolower(pos[0])) {
                    അവസ്ഥ '%': result += "%"; നിർത്തുക;
                    അവസ്ഥ 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        നിർത്തുക;
                    സ്ഥിരസ്ഥിതി: clearOptimizable(nodep, "Unknown $display-like format code."); നിർത്തുക;
                    }
                }
            }
        }

        AstConst* സ്ഥിരം resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_മൂല്യം(nodep, resultConstp);
        m_reclaimValuesp.തള്ളുക_back(resultConstp);
    }

    ശൂന്യം visit(AstDisplay* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_കണ്ണി_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_params) {
            സ്ഥിരം AstConst* സ്ഥിരം textp = fetchConst(nodep->fmtp());
            തിരഞ്ഞെടുപ്പ് (nodep->displayType()) {
            അവസ്ഥ VDisplayType::DT_DISPLAY:  // FALLTHRU
            അവസ്ഥ VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); നിർത്തുക;
            അവസ്ഥ VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); നിർത്തുക;
            അവസ്ഥ VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); നിർത്തുക;
            അവസ്ഥ VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); നിർത്തുക;
            അവസ്ഥ VDisplayType::DT_WRITE:  // FALLTHRU
            സ്ഥിരസ്ഥിതി: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    ശൂന്യം visit(AstToStringN* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        എങ്കിൽ (!optimizable()) തിരികെ;  // Accelerate
        check_കണ്ണി_Info(nodep);
        iterateChildrenConst(nodep);
        എങ്കിൽ (m_checkOnly || !optimizable()) തിരികെ;
        സ്ഥിരം std::string result = toStringRecurse(fetch_മൂല്യം(nodep->lhsp()));
        എങ്കിൽ (!optimizable()) തിരികെ;
        AstConst* സ്ഥിരം resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_മൂല്യം(nodep, resultConstp);
        m_reclaimValuesp.തള്ളുക_back(resultConstp);
    }

    ശൂന്യം visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_കണ്ണി_Type(nodep);
    }
    ശൂന്യം visit(AstGetInitialRandomSeed* nodep) override { known_Bad_കണ്ണി_Type(nodep); }
    ശൂന്യം visit(AstMemberSel* nodep) override { known_Bad_കണ്ണി_Type(nodep); }
    ശൂന്യം visit(AstSampled* nodep) override { known_Bad_കണ്ണി_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (jumpingOver()) തിരികെ;
        bad_കണ്ണി_Type(nodep);
    }

    // MEMBERS - called by constructor
    ശൂന്യം setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    ശൂന്യം mainGuts(Ast_കണ്ണി* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

പൊതു:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    ശൂന്യം clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_എണ്ണം = 0;
        m_data_എണ്ണം = 0;
        m_jumptargetp = nullptr;

        Ast_കണ്ണി::user1_Clear_വൃക്ഷം();
        m_varAux.clear();
        ++m_constGeneration;
    }
    ശൂന്യം mainTableCheck(Ast_കണ്ണി* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ശൂന്യം mainTableEmulate(Ast_കണ്ണി* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    ശൂന്യം mainParamEmulate(Ast_കണ്ണി* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_കണ്ണി*> unusedRootps;
        വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം nodep : m_reclaimValuesp) {
            എങ്കിൽ (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
};

#endif  // Guard
