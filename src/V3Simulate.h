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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Task.h"
#अन्तर्भाव "V3Width.h"

#अन्तर्भाव <deque>
#अन्तर्भाव <set>
#अन्तर्भाव <sstream>
#अन्तर्भाव <stack>
#अन्तर्भाव <string>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_ढेर_गाँठ final {
सार्वजनिक:
    // MEMBERS
    AstFuncRef* ध्रुव m_funcp;
    V3TaskConnects* ध्रुव m_tconnects;
    // CONSTRUCTORS
    Sim_ढेर_गाँठ(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ढेर_गाँठ() = स्वभाव;
};

class SimulateVisitor VL_NOT_FINAL : सार्वजनिक VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

गुप्त:
    // CONSTANTS
    स्थिर constexpr पूर्णाङ्क CALL_ढेर_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ध्रुव VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    गणना VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    संरचना AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_गाँठ_व्यंजक* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_गाँठ_व्यंजक* valuep = nullptr;
    };

    AstUser1Allocator<Ast_गाँठ, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    संरचना ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, Ast_गाँठ* nodep) {
            यदि (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            यदि (m_nextFree == m_constps.size()) {
                m_constps.धकेलो_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ध्रुव constp = m_constps[m_nextFree++];
            constp->अंक().nodep(nodep);
            प्रत्यागम constp;
        }

        ~ConstAllocator() {
            प्रति (AstConst* ध्रुव constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_गाँठ* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    पूर्णाङ्क m_instr_गिनती;  ///< Number of nodes
    पूर्णाङ्क m_data_गिनती;  ///< Bytes of data
    पूर्णाङ्क m_recurse_गिनती = 0;  // Now deep in current recursion
    Ast_गाँठ* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ध्रुव Ast_गाँठ_DType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<Sim_ढेर_गाँठ*> m_call_ढेर;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_गाँठ*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_संख्या(ध्रुव V3_संख्या* nump, ध्रुव Ast_गाँठ_DType* dtypep) {
        यदि (ध्रुव AstRefDType* ध्रुव refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        यदि (ध्रुव Ast_गाँठ_UOr_Struct_DType* ध्रुव stp = VN_CAST(dtypep, गाँठ_UOr_Struct_DType)) {
            यदि (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                प्रति (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ध्रुव पूर्णाङ्क width = itemp->width();
                    ध्रुव पूर्णाङ्क lsb = itemp->lsb();
                    ध्रुव पूर्णाङ्क msb = lsb + width - 1;
                    V3_संख्या field_अंक{nump, width};
                    field_अंक.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    यदि (ध्रुव Ast_गाँठ_DType* ध्रुव childTypep = itemp->subDTypep()) {
                        out << pretty_संख्या(&field_अंक, childTypep);
                    } अन्यथा {
                        out << field_अंक;
                    }
                    यदि (itemp->nextp()) out << ", ";
                }
                out << "}";
                प्रत्यागम out.str();
            }
        } अन्यथा यदि (ध्रुव AstPackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, PackArrayDType)) {
            यदि (ध्रुव Ast_गाँठ_DType* ध्रुव childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                ध्रुव पूर्णाङ्क arrayElements = arrayp->elementsConst();
                प्रति (पूर्णाङ्क element = 0; element < arrayElements; ++element) {
                    ध्रुव पूर्णाङ्क width = childTypep->width();
                    ध्रुव पूर्णाङ्क lsb = width * element;
                    ध्रुव पूर्णाङ्क msb = lsb + width - 1;
                    V3_संख्या field_अंक{nump, width};
                    field_अंक.opSel(*nump, msb, lsb);
                    ध्रुव पूर्णाङ्क arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_संख्या(&field_अंक, childTypep);
                    यदि (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                प्रत्यागम out.str();
            }
        }
        प्रत्यागम nump->ascii();
    }

    // Checking METHODS
सार्वजनिक:
    /// Call other-this function on all new *non-constant* var references
    आभास शून्य varRefCb(AstVarRef* nodep) {}

    शून्य clearOptimizable(Ast_गाँठ* nodep /*null ok*/, ध्रुव string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        यदि (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            यदि (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                यदि (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ढेर;
            पूर्णाङ्क n = 0;
            प्रति (ध्रुव auto& callstack : vlstd::reverse_view(m_call_ढेर)) {
                ध्रुव AstFuncRef* ध्रुव funcp = callstack->m_funcp;
                ढेर << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                प्रति (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    ध्रुव AstVar* ध्रुव portp = conIt->first;
                    Ast_गाँठ_व्यंजक* ध्रुव pinp = conIt->second->exprp();
                    ध्रुव Ast_गाँठ_DType* ध्रुव dtypep = pinp->dtypep();
                    यदि (AstConst* ध्रुव valp = fetchConstNull(pinp)) {
                        ढेर << "\n           " << portp->prettyName() << " = "
                              << pretty_संख्या(&valp->अंक(), dtypep);
                    } अन्यथा यदि (Ast_गाँठ_व्यंजक* ध्रुव valuep = fetch_मूल्य_Null(pinp)) {
                        ढेर << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                यदि (++n > CALL_ढेर_MAX) {
                    ढेर << "\n           ... stack truncated";
                    भङ्ग;
                }
            }
            m_whyNotOptimizable += ढेर.str();
        }
    }
    bool optimizable() ध्रुव { प्रत्यागम m_whyNotNodep == nullptr; }
    string whyNotMessage() ध्रुव { प्रत्यागम m_whyNotOptimizable; }
    Ast_गाँठ* whyNotNodep() ध्रुव { प्रत्यागम m_whyNotNodep; }

    bool isAssignDly() ध्रुव { प्रत्यागम m_anyAssignDly; }
    bool isImpure() ध्रुव { प्रत्यागम m_isImpure; }
    bool isCoverage() ध्रुव { प्रत्यागम m_isCoverage; }
    पूर्णाङ्क instr_गिनती() ध्रुव { प्रत्यागम m_instr_गिनती; }
    पूर्णाङ्क data_गिनती() ध्रुव { प्रत्यागम m_data_गिनती; }

    // Simulation METHODS
गुप्त:
    AstConst* allocConst(Ast_गाँठ* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        प्रत्यागम m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

सार्वजनिक:
    शून्य new_मूल्य(Ast_गाँठ* nodep, ध्रुव Ast_गाँठ_व्यंजक* valuep) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->अंक().opAssign(constp->अंक());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } अन्यथा यदि (fetch_मूल्य_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_मूल्य(nodep, newTrackedClone(const_cast<Ast_गाँठ_व्यंजक*>(valuep)));
        }
    }
    शून्य new_Out_मूल्य(Ast_गाँठ* nodep, ध्रुव Ast_गाँठ_व्यंजक* valuep) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->अंक().opAssign(constp->अंक());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } अन्यथा यदि (fetch_Out_मूल्य_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_मूल्य(nodep, newTrackedClone(const_cast<Ast_गाँठ_व्यंजक*>(valuep)));
        }
    }

गुप्त:
    Ast_गाँठ_व्यंजक* newTrackedClone(Ast_गाँठ_व्यंजक* nodep) {
        Ast_गाँठ_व्यंजक* ध्रुव newp = nodep->clone_वृक्ष(false);
        m_reclaimValuesp.धकेलो_back(newp);
        प्रत्यागम newp;
    }
    AstConst* newConst(Ast_गाँठ* nodep) {
        // Set a constant value for this node
        यदि (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ध्रुव constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            प्रत्यागम constp;
        }
        प्रत्यागम fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_गाँठ* nodep) {
        // Set a var-output constant value for this node
        यदि (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* ध्रुव constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            प्रत्यागम constp;
        }
        प्रत्यागम fetchOutConst(nodep);
    }

सार्वजनिक:
    Ast_गाँठ_व्यंजक* fetch_मूल्य_Null(Ast_गाँठ* nodep) { प्रत्यागम m_varAux(nodep).valuep; }

गुप्त:
    Ast_गाँठ_व्यंजक* fetch_Out_मूल्य_Null(Ast_गाँठ* nodep) { प्रत्यागम m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_गाँठ* nodep) { प्रत्यागम VN_CAST(fetch_मूल्य_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_गाँठ* nodep) {
        प्रत्यागम VN_CAST(fetch_Out_मूल्य_Null(nodep), Const);
    }
    Ast_गाँठ_व्यंजक* fetch_मूल्य(Ast_गाँठ* nodep) {
        Ast_गाँठ_व्यंजक* ध्रुव valuep = fetch_मूल्य_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        प्रत्यागम valuep;
    }
    AstConst* fetchConst(Ast_गाँठ* nodep) {
        AstConst* ध्रुव constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        प्रत्यागम constp;
    }
    AstConst* fetchOutConst(Ast_गाँठ* nodep) {
        AstConst* ध्रुव constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        प्रत्यागम constp;
    }

सार्वजनिक:
    V3_संख्या* fetch_संख्या_Null(Ast_गाँठ* nodep) {
        AstConst* ध्रुव constp = fetchConstNull(nodep);
        यदि (constp) प्रत्यागम &constp->अंक();
        प्रत्यागम nullptr;
    }
    V3_संख्या* fetch_Out_संख्या_Null(Ast_गाँठ* nodep) {
        AstConst* ध्रुव constp = fetchOutConstNull(nodep);
        यदि (constp) प्रत्यागम &constp->अंक();
        प्रत्यागम nullptr;
    }

गुप्त:
    शून्य set_मूल्य(Ast_गाँठ* nodep, Ast_गाँठ_व्यंजक* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    शून्य set_Out_मूल्य(Ast_गाँठ* nodep, Ast_गाँठ_व्यंजक* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    शून्य check_गाँठ_Info(Ast_गाँठ* nodep, bool ignorePredict = false) {
        यदि (m_checkOnly) {
            m_instr_गिनती += nodep->instr_गिनती();
            m_data_गिनती += nodep->width();
        }
        यदि (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        यदि (!nodep->isPure()) m_isImpure = true;
    }

    शून्य known_Bad_गाँठ_Type(Ast_गाँठ* nodep) {
        // Call for node types we know we can't handle
        यदि (jumpingOver()) प्रत्यागम;
        check_गाँठ_Info(nodep);
        यदि (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    शून्य bad_गाँठ_Type(Ast_गाँठ* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_गाँठ_Info(nodep);
        यदि (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            स्थिर std::set<VNType> s_typePrinted;
            ध्रुव auto pair = s_typePrinted.emplace(nodep->type());
            यदि (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_गाँठ* varOrScope(AstVarRef* nodep) ध्रुव {
        Ast_गाँठ* vscp;
        यदि (m_scoped) {
            vscp = nodep->varScopep();
        } अन्यथा {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        प्रत्यागम vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ध्रुव { प्रत्यागम m_jumptargetp; }
    शून्य assign_Out_मूल्य(ध्रुव Ast_गाँठ_Assign* nodep, Ast_गाँठ* vscp, ध्रुव Ast_गाँठ_व्यंजक* valuep) {
        यदि (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_मूल्य(vscp, valuep);
        } अन्यथा {
            new_मूल्य(vscp, valuep);
            set_Out_मूल्य(vscp, fetch_मूल्य(vscp));
        }
    }

    string toStringRecurse(Ast_गाँठ_व्यंजक* valuep) {
        // Return string representation of a value, or clearOptimizable
        यदि (ध्रुव AstConst* ध्रुव avaluep = VN_CAST(valuep, Const)) {
            प्रत्यागम "'h"s + avaluep->अंक().displayed(valuep, "%0x");
        }
        यदि (ध्रुव AstConsArray* ध्रुव avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            ध्रुव AstUnpackArrayDType* ध्रुव dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            प्रति (पूर्णाङ्क n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            प्रत्यागम result;
        }
        यदि (ध्रुव AstConsStruct* ध्रुव avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            प्रति (ध्रुव AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            प्रत्यागम result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        प्रत्यागम "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_गाँठ_व्यंजक* newInitValuep(Ast_गाँठ* nodep, Ast_गाँठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_गाँठ_व्यंजक* ध्रुव elementp = newInitValuep(nodep, arrayp->subDTypep());
            प्रत्यागम elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ध्रुव structp = VN_CAST(dtypep, StructDType);
        यदि (structp && !structp->packed()) {
            AstConsStruct* ध्रुव newp = new AstConsStruct{nodep->fileline(), structp};
            प्रति (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_गाँठ_व्यंजक* ध्रुव memberValuep = newInitValuep(nodep, memberp->subDTypep());
                यदि (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_वृक्ष(), newp);
                    प्रत्यागम nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            प्रत्यागम newp;
        }
        यदि (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
            AstConst* ध्रुव constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, basicp->widthMin(), 0};
            यदि (basicp->isZeroInit()) {
                constp->अंक() = V3_संख्या{nodep, basicp};
            } अन्यथा {
                constp->अंक().setAllBitsX();
            }
            प्रत्यागम constp;
        }
        प्रत्यागम nullptr;
    }
    शून्य initVar(AstVar* nodep) {
        यदि (Ast_गाँठ_व्यंजक* ध्रुव valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_मूल्य(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_वृक्ष(), valuep);
        }
    }

    // VISITORS
    शून्य visit(AstAlways* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_Sen_वृक्ष* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    शून्य visit(AstVarRef* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_गाँठ* ध्रुव vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        यदि (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), गाँठ_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        यदि (nodep->access().isWriteOrRW()) {
            यदि (m_inDlyAssign) {
                यदि (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    यदि (m_checkOnly) varRefCb(nodep);
                }
            } अन्यथा {  // nondly asn
                यदि (!(m_varAux(vscp).usage & VU_LV)) {
                    यदि (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    यदि (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        यदि (nodep->access().isReadOrRW()) {
            यदि (!(m_varAux(vscp).usage & VU_RV)) {
                यदि (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                ध्रुव bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ध्रुव Ast_गाँठ_व्यंजक* ध्रुव valuep
                    = varIsConst ? fetch_मूल्य_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                यदि (varIsConst && valuep) {
                    यदि (!m_checkOnly && optimizable()) new_मूल्य(vscp, valuep);
                } अन्यथा {
                    यदि (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_गाँठ_व्यंजक* valuep = fetch_मूल्य_Null(vscp);
            यदि (!valuep) {
                यदि (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } अन्यथा {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            set_मूल्य(nodep, valuep);
        }
    }
    शून्य visit(AstVarXRef* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (m_scoped) {
            bad_गाँठ_Type(nodep);
            प्रत्यागम;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    शून्य visit(Ast_गाँठ_FTask* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!m_params) {
            bad_गाँठ_Type(nodep);
            प्रत्यागम;
        }
        यदि (nodep->dpiImport()) {
            यदि (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        यदि (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStatic* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!m_params) {
            bad_गाँठ_Type(nodep);
            प्रत्यागम;
        }
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाँठ_If* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        UINFO(5, "   IF " << nodep);
        check_गाँठ_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateAndNextConstNull(nodep->condp());
            यदि (optimizable()) {
                यदि (fetchConst(nodep->condp())->अंक().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } अन्यथा {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    शून्य visit(AstConst* nodep) override {
        check_गाँठ_Info(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        new_मूल्य(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    शून्य visit(AstConsArray* nodep) override {
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        AstConsArray* ध्रुव valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_मूल्य(nodep->defaultp())->clone_वृक्ष(false) : nullptr};
        प्रति (ध्रुव AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_मूल्य(itemp->valuep())->clone_वृक्ष(false));
        }
        m_reclaimValuesp.धकेलो_back(valuep);
        set_मूल्य(nodep, valuep);
    }
    शून्य visit(AstConsStruct* nodep) override {
        यदि (!m_params) {
            bad_गाँठ_Type(nodep);
            प्रत्यागम;
        }
        check_गाँठ_Info(nodep);
        प्रति (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        AstConsStruct* ध्रुव valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        प्रति (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_मूल्य(itemp->valuep())->clone_वृक्ष(false)});
        }
        m_reclaimValuesp.धकेलो_back(valuep);
        set_मूल्य(nodep, valuep);
    }
    शून्य visit(AstCvtUnpackToUnpack* nodep) override {
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_गाँठ_व्यंजक* ध्रुव cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_मूल्य(nodep->fromp())->clone_वृक्ष(false), nodep->dtypep()};
        Ast_गाँठ_व्यंजक* ध्रुव valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.धकेलो_back(valuep);
        set_मूल्य(nodep, valuep);
    }
    शून्य visit(AstConsArrayItem* nodep) override {
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        check_गाँठ_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        Ast_गाँठ* ध्रुव valuep = nodep->itemp()->valuep();
        यदि (valuep) {
            iterateAndNextConstNull(valuep);
            यदि (!optimizable()) प्रत्यागम;
            new_मूल्य(nodep, fetch_मूल्य(valuep));
        } अन्यथा {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    शून्य visit(Ast_गाँठ_Uniop* nodep) override {
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        nodep->संख्या_Operate(newConst(nodep)->अंक(), fetchConst(nodep->lhsp())->अंक());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    शून्य aggregateEqRecurse(V3_संख्या& resultr, Ast_गाँठ* nodep, bool caseEq, Ast_गाँठ_व्यंजक* lhsp,
                            Ast_गाँठ_व्यंजक* rhsp) {
        यदि (ध्रुव AstConst* ध्रुव lconstp = VN_CAST(lhsp, Const)) {
            ध्रुव V3_संख्या& lnum = lconstp->अंक();
            ध्रुव V3_संख्या& rnum = VN_AS(rhsp, Const)->अंक();
            V3_संख्या eq{nodep, 1};
            यदि (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } अन्यथा यदि (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } अन्यथा यदि (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } अन्यथा {
                eq.opEq(lnum, rnum);
            }
            ध्रुव V3_संख्या prev = resultr;
            resultr.opLogAnd(prev, eq);
        } अन्यथा यदि (ध्रुव AstConsArray* ध्रुव lconsp = VN_CAST(lhsp, ConsArray)) {
            ध्रुव AstUnpackArrayDType* ध्रुव dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            प्रति (पूर्णाङ्क n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } अन्यथा {
            ध्रुव AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            प्रति (ध्रुव AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    शून्य handleAggregateCompare(Ast_गाँठ_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ध्रुव bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ध्रुव bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_संख्या& resultr = newConst(nodep)->अंक();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_मूल्य(nodep->lhsp()),
                           fetch_मूल्य(nodep->rhsp()));
        यदि (neq) {
            ध्रुव V3_संख्या eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    शून्य visit(Ast_गाँठ_Biop* nodep) override {
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        यदि (!VN_IS(fetch_मूल्य(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            प्रत्यागम;
        }
        AstConst* ध्रुव valuep = newConst(nodep);
        nodep->संख्या_Operate(newConst(nodep)->अंक(), fetchConst(nodep->lhsp())->अंक(),
                             fetchConst(nodep->rhsp())->अंक());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        यदि (!m_params && VN_IS(nodep, Sel) && valuep->अंक().isAnyX()) {
            ध्रुव V3_संख्या अंक{valuep, valuep->width(), valuep->अंक()};
            valuep->अंक().opBitsOne(अंक);
        }
    }
    शून्य visit(Ast_गाँठ_Triop* nodep) override {
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        nodep->संख्या_Operate(newConst(nodep)->अंक(), fetchConst(nodep->lhsp())->अंक(),
                             fetchConst(nodep->rhsp())->अंक(), fetchConst(nodep->thsp())->अंक());
    }
    शून्य visit(Ast_गाँठ_Quadop* nodep) override {
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        nodep->संख्या_Operate(newConst(nodep)->अंक(), fetchConst(nodep->lhsp())->अंक(),
                             fetchConst(nodep->rhsp())->अंक(), fetchConst(nodep->thsp())->अंक(),
                             fetchConst(nodep->fhsp())->अंक());
    }
    शून्य visit(AstLogAnd* nodep) override {
        // Need to short circuit
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) प्रत्यागम;
            यदि (fetchConst(nodep->lhsp())->अंक().isNeqZero()) {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) प्रत्यागम;
                new_मूल्य(nodep, fetch_मूल्य(nodep->rhsp()));
            } अन्यथा {
                new_मूल्य(nodep, fetch_मूल्य(nodep->lhsp()));  // a zero
            }
        }
    }
    शून्य visit(AstLogOr* nodep) override {
        // Need to short circuit
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) प्रत्यागम;
            यदि (fetchConst(nodep->lhsp())->अंक().isNeqZero()) {
                new_मूल्य(nodep, fetch_मूल्य(nodep->lhsp()));  // a one
            } अन्यथा {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) प्रत्यागम;
                new_मूल्य(nodep, fetch_मूल्य(nodep->rhsp()));
            }
        }
    }
    शून्य visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) प्रत्यागम;
            यदि (fetchConst(nodep->lhsp())->अंक().isEqZero()) {
                ध्रुव AstConst cnst{nodep->fileline(), AstConst::Widthed_मूल्य{}, 1, 1};  // a one
                new_मूल्य(nodep, &cnst);  // a one
            } अन्यथा {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) प्रत्यागम;
                new_मूल्य(nodep, fetch_मूल्य(nodep->rhsp()));
            }
        }
    }
    शून्य visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->condp());
            यदि (!optimizable()) प्रत्यागम;
            यदि (fetchConst(nodep->condp())->अंक().isNeqZero()) {
                iterateConst(nodep->thenp());
                यदि (!optimizable()) प्रत्यागम;
                new_मूल्य(nodep, fetch_मूल्य(nodep->thenp()));
            } अन्यथा {
                iterateConst(nodep->elsep());
                यदि (!optimizable()) प्रत्यागम;
                new_मूल्य(nodep, fetch_मूल्य(nodep->elsep()));
            }
        }
    }

    शून्य handleAssignElement(Ast_गाँठ_Assign* nodep, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* मूल्य_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_गाँठ_व्यंजक*> selps;
        Ast_गाँठ_व्यंजक* fromp = lhsp;
        यावत् (true) {
            यदि (AstArraySel* ध्रुव selp = VN_CAST(fromp, ArraySel)) {
                check_गाँठ_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.धकेलो_back(selp);
                fromp = selp->fromp();
            } अन्यथा यदि (AstStructSel* ध्रुव selp = VN_CAST(fromp, StructSel)) {
                check_गाँठ_Info(selp);
                selps.धकेलो_back(selp);
                fromp = selp->fromp();
            } अन्यथा {
                भङ्ग;
            }
        }
        AstVarRef* ध्रुव varrefp = VN_CAST(fromp, VarRef);
        यदि (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            प्रत्यागम;
        }
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        Ast_गाँठ* ध्रुव vscp = varOrScope(varrefp);
        Ast_गाँठ_व्यंजक* rootp = fetch_Out_मूल्य_Null(vscp);
        यदि (!rootp) rootp = fetch_मूल्य_Null(vscp);
        यदि (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            प्रत्यागम;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_गाँठ_व्यंजक* itemp = rootp;
        प्रति (Ast_गाँठ_व्यंजक* ध्रुव selp : vlstd::reverse_view(selps)) {
            यदि (ध्रुव AstArraySel* ध्रुव aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ध्रुव consp = VN_AS(itemp, ConsArray);
                ध्रुव uint32_t index = fetchConst(aselp->bitp())->toUInt();
                ध्रुव पूर्णाङ्क elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                यदि (index >= static_cast<uint32_t>(elements)) {
                    प्रत्यागम;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                यदि (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_वृक्ष(false);
                    consp->valuep(index, itemp);
                }
            } अन्यथा {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_गाँठ_व्यंजक* ध्रुव valuep = newTrackedClone(fetch_मूल्य(मूल्य_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.धकेलो_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_मूल्य(nodep, vscp, rootp);
    }
    शून्य handleAssignSel(Ast_गाँठ_Assign* nodep, AstSel* selp, Ast_गाँठ_व्यंजक* मूल्य_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_संख्या lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_गाँठ* ध्रुव vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        यदि (AstConst* ध्रुव vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } अन्यथा यदि (AstConst* ध्रुव vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } अन्यथा {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{},
                                     varrefp->varp()->widthMin(), 0};
            यदि (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->अंक().setAllBits0();
            } अन्यथा {
                outconstp->अंक().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->अंक().opSelInto(fetchConst(मूल्य_Fromp)->अंक(), lsb, selp->widthConst());
        assign_Out_मूल्य(nodep, vscp, outconstp);
    }
    शून्य handleAssignSelRecurse(Ast_गाँठ_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_संख्या& lsbRef, पूर्णाङ्क depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_गाँठ_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        यदि (AstVarRef* ध्रुव varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->अंक();
            प्रत्यागम;  // And presumably still optimizable()
        }
        यदि (AstSel* ध्रुव subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_संख्या sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            यदि (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->अंक());
            }
        } अन्यथा {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    शून्य handleAssignRecurse(Ast_गाँठ_Assign* nodep, Ast_गाँठ_व्यंजक* lhsp, Ast_गाँठ_व्यंजक* मूल्य_Fromp) {
        यदि (!optimizable()) प्रत्यागम;
        यदि (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            यदि (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                प्रत्यागम;
            }
            handleAssignElement(nodep, lhsp, मूल्य_Fromp);
        } अन्यथा यदि (AstConcat* ध्रुव selp = VN_CAST(lhsp, Concat)) {
            check_गाँठ_Info(selp);
            यदि (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                प्रत्यागम;
            }
            // Split value into left and right concat values
            यदि (!m_checkOnly) {
                {
                    AstConst* ध्रुव outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_मूल्य{},
                                       selp->lhsp()->width(), 0};
                    outconstp->अंक().opSel(fetchConst(मूल्य_Fromp)->अंक(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_मूल्य(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_वृक्ष(), outconstp);
                }
                {
                    AstConst* ध्रुव outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_मूल्य{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->अंक().opSel(fetchConst(मूल्य_Fromp)->अंक(),
                                           selp->rhsp()->width() - 1, 0);
                    new_मूल्य(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_वृक्ष(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } अन्यथा यदि (AstReplicate* ध्रुव selp = VN_CAST(lhsp, Replicate)) {
            check_गाँठ_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* ध्रुव countp = VN_CAST(selp->countp(), Const);
            यदि (!countp || !countp->अंक().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                प्रत्यागम;
            }
            handleAssignRecurse(nodep, selp->srcp(), मूल्य_Fromp);
        } अन्यथा यदि (AstSel* ध्रुव selp = VN_CAST(lhsp, Sel)) {
            यदि (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                प्रत्यागम;
            }
            handleAssignSel(nodep, selp, मूल्य_Fromp);
        } अन्यथा यदि (VN_IS(lhsp, VarRef)) {
            यदि (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } अन्यथा {
                Ast_गाँठ* ध्रुव vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_मूल्य(nodep, vscp, fetch_मूल्य(मूल्य_Fromp));
            }
        } अन्यथा {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    शून्य visit(Ast_गाँठ_Assign* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        यदि (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } अन्यथा यदि (VN_IS(nodep, AssignDly)) {
            यदि (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } अन्यथा {
            यदि (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        यदि (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            प्रत्यागम;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    शून्य visit(AstArraySel* nodep) override {
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (ध्रुव AstConsArray* ध्रुव consp = VN_CAST(fetch_मूल्य_Null(nodep->fromp()), ConsArray)) {
            ध्रुव AstConst* ध्रुव indexp = fetchConst(nodep->bitp());
            ध्रुव uint32_t offset = indexp->अंक().toUInt();
            Ast_गाँठ_व्यंजक* ध्रुव itemp = consp->valuep(offset);
            यदि (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } अन्यथा {
                set_मूल्य(nodep, itemp);
            }
        } अन्यथा {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    शून्य visit(AstStructSel* nodep) override {
        यदि (!m_params) {
            bad_गाँठ_Type(nodep);
            प्रत्यागम;
        }
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        AstConsStruct* ध्रुव consp = VN_AS(fetch_मूल्य(nodep->fromp()), ConsStruct);
        set_मूल्य(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    शून्य visit(AstSliceSel* nodep) override {
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        // Fetch the base constant array
        यदि (ध्रुव AstConsArray* ध्रुव consp = VN_CAST(fetch_मूल्य_Null(nodep->fromp()), ConsArray)) {
            ध्रुव VNumRange& sliceRange = nodep->declRange();
            ध्रुव uint32_t sliceElements = sliceRange.elements();
            ध्रुव पूर्णाङ्क sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_गाँठ_DType* ध्रुव dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ध्रुव newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            प्रति (uint32_t idx = 0; idx < sliceElements; ++idx) {
                ध्रुव uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_वृक्ष(false));
            }
            // Assign the new constant array and track it for later deletion
            set_मूल्य(nodep, newConsp);
            m_reclaimValuesp.धकेलो_back(newConsp);
        } अन्यथा {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    शून्य visit(AstBegin* nodep) override {
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCase* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        UINFO(5, "   CASE " << nodep);
        check_गाँठ_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा यदि (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                यदि (!itemp->isDefault()) {
                    प्रति (Ast_गाँठ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        यदि (hit) भङ्ग;
                        iterateAndNextConstNull(ep);
                        यदि (optimizable()) {
                            V3_संख्या match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->अंक(), fetchConst(ep)->अंक());
                            यदि (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                यदि (hit) भङ्ग;
                यदि (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    शून्य visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        यदि (jumpingOver()) प्रत्यागम;
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstComment*) override {}

    शून्य visit(Ast_Stmt_व्यंजक* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_व्यंजक_Stmt* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_गाँठ_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        यदि (!optimizable()) प्रत्यागम;
        iterateAndNextConstNull(nodep->resultp());
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        new_मूल्य(nodep, fetch_मूल्य(nodep->resultp()));
    }

    शून्य visit(AstJumpBlock* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        iterateChildrenConst(nodep);
        यदि (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    शून्य visit(AstJumpGo* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_गाँठ_Info(nodep);
        यदि (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        यदि (jumpingOver()) प्रत्यागम;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        यदि (!m_params) {
            bad_गाँठ_Type(nodep);
            प्रत्यागम;
        }
        check_गाँठ_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
            प्रत्यागम;
        }
        यदि (!optimizable()) प्रत्यागम;

        size_t iter_गिनती = 0;
        ध्रुव size_t iterLimit = v3Global.opt.unrollLimit();
        यावत् (true) {
            यदि (iter_गिनती > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                भङ्ग;
            }
            ++iter_गिनती;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            यदि (jumpingOver()) भङ्ग;
        }

        यदि (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    शून्य visit(AstLoopTest* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_गाँठ_Info(nodep);
        iterateConst(nodep->condp());
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        यदि (fetchConst(nodep->condp())->अंक().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    शून्य visit(AstStop* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_गाँठ_Info(nodep);
    }
    शून्य visit(AstFuncRef* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_गाँठ_Info(nodep);
        यदि (!m_params) {
            bad_गाँठ_Type(nodep);
            प्रत्यागम;
        }
        Ast_गाँठ_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        यदि (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        यदि (funcp->recursive()) {
            यदि (m_recurse_गिनती >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                प्रत्यागम;
            }
            ++m_recurse_गिनती;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_गाँठ*, Ast_गाँठ_व्यंजक*> old_मूल्यावली;

        यदि (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_मूल्यावली](AstVar* varp) {
                यदि (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    यदि (Ast_गाँठ_व्यंजक* ध्रुव valuep = fetch_मूल्य_Null(varp)) {
                        Ast_गाँठ_व्यंजक* ध्रुव nvaluep = newTrackedClone(valuep);
                        old_मूल्यावली.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_मूल्यावली](Ast_गाँठ_व्यंजक* exprp) {
                यदि (VN_IS(exprp, Const)) प्रत्यागम;  // Speed up as won't change
                यदि (Ast_गाँठ_व्यंजक* ध्रुव valuep = fetch_मूल्य_Null(exprp)) {
                    Ast_गाँठ_व्यंजक* ध्रुव nvaluep = newTrackedClone(valuep);
                    old_मूल्यावली.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_गाँठ_व्यंजक*>> port_मूल्यावली;
        प्रति (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* ध्रुव portp = it->first;
            Ast_गाँठ* ध्रुव pinp = it->second->exprp();
            यदि (pinp) {  // Else too few arguments in function call - ignore it
                यदि (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    प्रत्यागम;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_मूल्यावली.emplace_back(std::make_pair(portp, newTrackedClone(fetch_मूल्य(pinp))));
            }
        }
        // Apply value to the function
        यदि (!m_checkOnly && optimizable())
            प्रति (ध्रुव auto& it : port_मूल्यावली) {
                यदि (!m_checkOnly && optimizable()) new_मूल्य(it.first, it.second);
            }
        Sim_ढेर_गाँठ ढेर_गाँठ{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ढेर.धकेलो_back(&ढेर_गाँठ);
        यदि (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                यदि (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_ढेर.निकालो_back();
        ध्रुव Ast_गाँठ_व्यंजक* returnp = nullptr;
        यदि (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_मूल्य(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        प्रति (ध्रुव auto& it : old_मूल्यावली) {
            यदि (it.second) new_मूल्य(it.first, it.second);
        }
        यदि (returnp) new_मूल्य(nodep, returnp);
        यदि (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_गिनती > 0, nodep, "recurse underflow");
            --m_recurse_गिनती;
        }
    }

    शून्य visit(AstVar* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!m_params) {
            bad_गाँठ_Type(nodep);
            प्रत्यागम;
        }
    }

    शून्य visit(AstScopeName* /*nodep*/) override {
        यदि (jumpingOver()) प्रत्यागम;
        // Ignore
    }

    शून्य visit(AstSFormatArg* nodep) override {
        check_गाँठ_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    शून्य visit(AstSFormatF* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;

        Ast_गाँठ* nextArgp = nodep->exprsp();
        string result;
        ध्रुव string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        प्रति (; pos != format.cend(); ++pos) {
            यदि (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } अन्यथा यदि (!inPct) {  // Normal text
                result += *pos;
            } अन्यथा {  // Format character
                यदि (std::isdigit(pos[0])) {
                    width += pos[0];
                    अनुवर्त;
                }

                inPct = false;

                यदि (V3_संख्या::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_गाँठ* ध्रुव argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
                    Ast_गाँठ* ध्रुव subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ध्रुव constp = fetchConstNull(subargp);
                    ध्रुव VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    यदि (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        भङ्ग;
                    }
                    ध्रुव string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->अंक().displayedEnum(fargp, pformat)
                                  : constp->अंक().displayed(nodep, pformat, formatAttr);
                } अन्यथा {
                    विकल्प (std::tolower(pos[0])) {
                    पक्ष '%': result += "%"; भङ्ग;
                    पक्ष 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        भङ्ग;
                    स्वभाव: clearOptimizable(nodep, "Unknown $display-like format code."); भङ्ग;
                    }
                }
            }
        }

        AstConst* ध्रुव resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_मूल्य(nodep, resultConstp);
        m_reclaimValuesp.धकेलो_back(resultConstp);
    }

    शून्य visit(AstDisplay* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_गाँठ_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        यदि (m_params) {
            ध्रुव AstConst* ध्रुव textp = fetchConst(nodep->fmtp());
            विकल्प (nodep->displayType()) {
            पक्ष VDisplayType::DT_DISPLAY:  // FALLTHRU
            पक्ष VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); भङ्ग;
            पक्ष VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); भङ्ग;
            पक्ष VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); भङ्ग;
            पक्ष VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); भङ्ग;
            पक्ष VDisplayType::DT_WRITE:  // FALLTHRU
            स्वभाव: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    शून्य visit(AstToStringN* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_गाँठ_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        ध्रुव std::string result = toStringRecurse(fetch_मूल्य(nodep->lhsp()));
        यदि (!optimizable()) प्रत्यागम;
        AstConst* ध्रुव resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_मूल्य(nodep, resultConstp);
        m_reclaimValuesp.धकेलो_back(resultConstp);
    }

    शून्य visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    शून्य visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_गाँठ_Type(nodep);
    }
    शून्य visit(AstGetInitialRandomSeed* nodep) override { known_Bad_गाँठ_Type(nodep); }
    शून्य visit(AstMemberSel* nodep) override { known_Bad_गाँठ_Type(nodep); }
    शून्य visit(AstSampled* nodep) override { known_Bad_गाँठ_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        bad_गाँठ_Type(nodep);
    }

    // MEMBERS - called by constructor
    शून्य setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    शून्य mainGuts(Ast_गाँठ* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

सार्वजनिक:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    शून्य clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_गिनती = 0;
        m_data_गिनती = 0;
        m_jumptargetp = nullptr;

        Ast_गाँठ::user1_Clear_वृक्ष();
        m_varAux.clear();
        ++m_constGeneration;
    }
    शून्य mainTableCheck(Ast_गाँठ* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    शून्य mainTableEmulate(Ast_गाँठ* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    शून्य mainParamEmulate(Ast_गाँठ* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_गाँठ*> unusedRootps;
        प्रति (Ast_गाँठ* ध्रुव nodep : m_reclaimValuesp) {
            यदि (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        प्रति (Ast_गाँठ* ध्रुव nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
};

#endif  // Guard
