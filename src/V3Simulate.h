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

#यदि_अपरिभाषितम् VERILATOR_V3SIMULATE_H_
#परिभाषय VERILATOR_V3SIMULATE_H_

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

class Sim_राशिः_ग्रन्थिः final {
सार्वजनिक:
    // MEMBERS
    AstFuncRef* ध्रुव m_funcp;
    V3TaskConnects* ध्रुव m_tconnects;
    // CONSTRUCTORS
    Sim_राशिः_ग्रन्थिः(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_राशिः_ग्रन्थिः() = स्वभाव;
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
    स्थिर constexpr पूर्णाङ्क CALL_राशिः_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    गणना VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    संरचना AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_ग्रन्थिः_व्यञ्जकम्* बहिर्गमनम्_Valuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = nullptr;
    };

    AstUser1Allocator<Ast_ग्रन्थिः, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    संरचना ConstAllocator final {
        आकार_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        आकार_t m_अग्र_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(आकार_t currentGeneration, Ast_ग्रन्थिः* nodep) {
            यदि (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_अग्र_Free = 0;
            }
            UASSERT_OBJ(m_अग्र_Free <= m_constps.आकार(), nodep, "Should only allocate at end");
            यदि (m_अग्र_Free == m_constps.आकार()) {
                m_constps.प्रक्षिप_पृष्ठभागः(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ध्रुव constp = m_constps[m_अग्र_Free++];
            constp->अङ्कः().nodep(nodep);
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
    Ast_ग्रन्थिः* m_whyNotNodep;  ///< First node not optimizable
    bool m_any_नियोजय_Dly;  ///< True if found a delayed assignment
    bool m_any_नियोजय_Comb;  ///< True if found a non-delayed assignment
    bool m_अन्तर्_Dly_नियोजय;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    पूर्णाङ्क m_instr_गणनम्;  ///< Number of nodes
    पूर्णाङ्क m_दत्त_गणनम्;  ///< Bytes of data
    पूर्णाङ्क m_recurse_गणनम् = 0;  // Now deep in current recursion
    Ast_ग्रन्थिः* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ध्रुव Ast_ग्रन्थिः_DType*, ConstAllocator> m_constps;
    आकार_t m_constGeneration = 0;
    std::सदिशम्<Sim_राशिः_ग्रन्थिः*> m_call_राशिः;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::सदिशम्<Ast_ग्रन्थिः*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_सङ्ख्या(ध्रुव V3_सङ्ख्या* nump, ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        यदि (ध्रुव AstRefDType* ध्रुव refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव stp = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
            यदि (stp->packed()) {
                std::ostringstream बहिर्गमनम्;
                बहिर्गमनम् << "'{";
                प्रति (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ध्रुव पूर्णाङ्क विस्तारः = itemp->विस्तारः();
                    ध्रुव पूर्णाङ्क lsb = itemp->lsb();
                    ध्रुव पूर्णाङ्क msb = lsb + विस्तारः - 1;
                    V3_सङ्ख्या क्षेत्रम्_अङ्कः{nump, विस्तारः};
                    क्षेत्रम्_अङ्कः.opSel(*nump, msb, lsb);
                    बहिर्गमनम् << itemp->name() << ": ";
                    यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव childTypep = itemp->subDTypep()) {
                        बहिर्गमनम् << pretty_सङ्ख्या(&क्षेत्रम्_अङ्कः, childTypep);
                    } अन्यथा {
                        बहिर्गमनम् << क्षेत्रम्_अङ्कः;
                    }
                    यदि (itemp->nextp()) बहिर्गमनम् << ", ";
                }
                बहिर्गमनम् << "}";
                प्रत्यागम बहिर्गमनम्.str();
            }
        } अन्यथा यदि (ध्रुव AstPackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, PackArrayDType)) {
            यदि (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव childTypep = arrayp->subDTypep()) {
                std::ostringstream बहिर्गमनम्;
                बहिर्गमनम् << "[";
                ध्रुव पूर्णाङ्क arrayElements = arrayp->elementsConst();
                प्रति (पूर्णाङ्क element = 0; element < arrayElements; ++element) {
                    ध्रुव पूर्णाङ्क विस्तारः = childTypep->विस्तारः();
                    ध्रुव पूर्णाङ्क lsb = विस्तारः * element;
                    ध्रुव पूर्णाङ्क msb = lsb + विस्तारः - 1;
                    V3_सङ्ख्या क्षेत्रम्_अङ्कः{nump, विस्तारः};
                    क्षेत्रम्_अङ्कः.opSel(*nump, msb, lsb);
                    ध्रुव पूर्णाङ्क arrayElem = arrayp->lo() + element;
                    बहिर्गमनम् << arrayElem << " = " << pretty_सङ्ख्या(&क्षेत्रम्_अङ्कः, childTypep);
                    यदि (element < arrayElements - 1) बहिर्गमनम् << ", ";
                }
                बहिर्गमनम् << "]";
                प्रत्यागम बहिर्गमनम्.str();
            }
        }
        प्रत्यागम nump->ascii();
    }

    // Checking METHODS
सार्वजनिक:
    /// Call other-this function on all new *non-constant* var references
    आभास शून्य varRefCb(AstVarRef* nodep) {}

    शून्य clearOptimizable(Ast_ग्रन्थिः* nodep /*null ok*/, ध्रुव string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        यदि (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            यदि (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_उपसर्गः("Clear optimizable: " << why);
                यदि (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream राशिः;
            पूर्णाङ्क n = 0;
            प्रति (ध्रुव स्वचालित& callstack : vlstd::reverse_view(m_call_राशिः)) {
                ध्रुव AstFuncRef* ध्रुव funcp = callstack->m_funcp;
                राशिः << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                प्रति (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->अन्तः_2(); ++conIt) {
                    ध्रुव AstVar* ध्रुव portp = conIt->first;
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = conIt->second->exprp();
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = pinp->dtypep();
                    यदि (AstConst* ध्रुव valp = fetchConstNull(pinp)) {
                        राशिः << "\n           " << portp->prettyName() << " = "
                              << pretty_सङ्ख्या(&valp->अङ्कः(), dtypep);
                    } अन्यथा यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = fetch_मानम्_Null(pinp)) {
                        राशिः << "\n           " << portp->prettyName() << " = "
                              << गन्तव्यम्_String_Recurse(valuep);
                    }
                }
                यदि (++n > CALL_राशिः_MAX) {
                    राशिः << "\n           ... stack truncated";
                    भङ्ग;
                }
            }
            m_whyNotOptimizable += राशिः.str();
        }
    }
    bool optimizable() ध्रुव { प्रत्यागम m_whyNotNodep == nullptr; }
    string why_Not_सन्देशः() ध्रुव { प्रत्यागम m_whyNotOptimizable; }
    Ast_ग्रन्थिः* whyNotNodep() ध्रुव { प्रत्यागम m_whyNotNodep; }

    bool is_नियोजय_Dly() ध्रुव { प्रत्यागम m_any_नियोजय_Dly; }
    bool isImpure() ध्रुव { प्रत्यागम m_isImpure; }
    bool isCoverage() ध्रुव { प्रत्यागम m_isCoverage; }
    पूर्णाङ्क instr_गणनम्() ध्रुव { प्रत्यागम m_instr_गणनम्; }
    पूर्णाङ्क दत्त_गणनम्() ध्रुव { प्रत्यागम m_दत्त_गणनम्; }

    // Simulation METHODS
गुप्त:
    AstConst* allocConst(Ast_ग्रन्थिः* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        प्रत्यागम m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

सार्वजनिक:
    शून्य new_मानम्(Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->अङ्कः().op_नियोजय(constp->अङ्कः());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } अन्यथा यदि (fetch_मानम्_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            समुच्चयः_मानम्(nodep, newTrackedClone(const_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(valuep)));
        }
    }
    शून्य new_बहिर्गमनम्_मानम्(Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
            new_बहिर्गमनम्_Const(nodep)->अङ्कः().op_नियोजय(constp->अङ्कः());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } अन्यथा यदि (fetch_बहिर्गमनम्_मानम्_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            समुच्चयः_बहिर्गमनम्_मानम्(nodep, newTrackedClone(const_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(valuep)));
        }
    }

गुप्त:
    Ast_ग्रन्थिः_व्यञ्जकम्* newTrackedClone(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = nodep->clone_वृक्षः(false);
        m_reclaimValuesp.प्रक्षिप_पृष्ठभागः(newp);
        प्रत्यागम newp;
    }
    AstConst* newConst(Ast_ग्रन्थिः* nodep) {
        // Set a constant value for this node
        यदि (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ध्रुव constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            प्रत्यागम constp;
        }
        प्रत्यागम fetchConst(nodep);
    }
    AstConst* new_बहिर्गमनम्_Const(Ast_ग्रन्थिः* nodep) {
        // Set a var-output constant value for this node
        यदि (!VN_IS(m_varAux(nodep).बहिर्गमनम्_Valuep, Const)) {
            AstConst* ध्रुव constp = allocConst(nodep);
            m_varAux(nodep).बहिर्गमनम्_Valuep = constp;
            प्रत्यागम constp;
        }
        प्रत्यागम fetch_बहिर्गमनम्_Const(nodep);
    }

सार्वजनिक:
    Ast_ग्रन्थिः_व्यञ्जकम्* fetch_मानम्_Null(Ast_ग्रन्थिः* nodep) { प्रत्यागम m_varAux(nodep).valuep; }

गुप्त:
    Ast_ग्रन्थिः_व्यञ्जकम्* fetch_बहिर्गमनम्_मानम्_Null(Ast_ग्रन्थिः* nodep) { प्रत्यागम m_varAux(nodep).बहिर्गमनम्_Valuep; }
    AstConst* fetchConstNull(Ast_ग्रन्थिः* nodep) { प्रत्यागम VN_CAST(fetch_मानम्_Null(nodep), Const); }
    AstConst* fetch_बहिर्गमनम्_Const_Null(Ast_ग्रन्थिः* nodep) {
        प्रत्यागम VN_CAST(fetch_बहिर्गमनम्_मानम्_Null(nodep), Const);
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* fetch_मानम्(Ast_ग्रन्थिः* nodep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = fetch_मानम्_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        प्रत्यागम valuep;
    }
    AstConst* fetchConst(Ast_ग्रन्थिः* nodep) {
        AstConst* ध्रुव constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        प्रत्यागम constp;
    }
    AstConst* fetch_बहिर्गमनम्_Const(Ast_ग्रन्थिः* nodep) {
        AstConst* ध्रुव constp = fetch_बहिर्गमनम्_Const_Null(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        प्रत्यागम constp;
    }

सार्वजनिक:
    V3_सङ्ख्या* fetch_सङ्ख्या_Null(Ast_ग्रन्थिः* nodep) {
        AstConst* ध्रुव constp = fetchConstNull(nodep);
        यदि (constp) प्रत्यागम &constp->अङ्कः();
        प्रत्यागम nullptr;
    }
    V3_सङ्ख्या* fetch_बहिर्गमनम्_सङ्ख्या_Null(Ast_ग्रन्थिः* nodep) {
        AstConst* ध्रुव constp = fetch_बहिर्गमनम्_Const_Null(nodep);
        यदि (constp) प्रत्यागम &constp->अङ्कः();
        प्रत्यागम nullptr;
    }

गुप्त:
    शून्य समुच्चयः_मानम्(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    शून्य समुच्चयः_बहिर्गमनम्_मानम्(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).बहिर्गमनम्_Valuep = valuep;
    }

    शून्य check_ग्रन्थिः_Info(Ast_ग्रन्थिः* nodep, bool ignorePredict = false) {
        यदि (m_checkOnly) {
            m_instr_गणनम् += nodep->instr_गणनम्();
            m_दत्त_गणनम् += nodep->विस्तारः();
        }
        यदि (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        यदि (!nodep->is_विशुद्धम्()) m_isImpure = true;
    }

    शून्य ज्ञातम्_Bad_ग्रन्थिः_Type(Ast_ग्रन्थिः* nodep) {
        // Call for node types we know we can't handle
        यदि (jumpingOver()) प्रत्यागम;
        check_ग्रन्थिः_Info(nodep);
        यदि (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    शून्य bad_ग्रन्थिः_Type(Ast_ग्रन्थिः* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_ग्रन्थिः_Info(nodep);
        यदि (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#यदि_परिभाषितम् VL_DEBUG
            स्थिर std::समुच्चयः<VNType> s_typePrinted;
            ध्रुव स्वचालित pair = s_typePrinted.emplace(nodep->type());
            यदि (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#यदि_अन्तः
        }
    }

    Ast_ग्रन्थिः* varOrScope(AstVarRef* nodep) ध्रुव {
        Ast_ग्रन्थिः* vscp;
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
    शून्य नियोजय_बहिर्गमनम्_मानम्(ध्रुव Ast_ग्रन्थिः_नियोजय* nodep, Ast_ग्रन्थिः* vscp, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        यदि (VN_IS(nodep, नियोजय_Dly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_बहिर्गमनम्_मानम्(vscp, valuep);
        } अन्यथा {
            new_मानम्(vscp, valuep);
            समुच्चयः_बहिर्गमनम्_मानम्(vscp, fetch_मानम्(vscp));
        }
    }

    string गन्तव्यम्_String_Recurse(Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        // Return string representation of a value, or clearOptimizable
        यदि (ध्रुव AstConst* ध्रुव avaluep = VN_CAST(valuep, Const)) {
            प्रत्यागम "'h"s + avaluep->अङ्कः().displayed(valuep, "%0x");
        }
        यदि (ध्रुव AstConsArray* ध्रुव avaluep = VN_CAST(valuep, ConsArray)) {
            string परिणाम = "'{";
            string comma;
            ध्रुव AstUnpackArrayDType* ध्रुव dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            प्रति (पूर्णाङ्क n = 0; n < dtypep->elementsConst(); ++n) {
                परिणाम += comma + गन्तव्यम्_String_Recurse(avaluep->valuep(n));
                comma = ", ";
            }
            परिणाम += "}";
            प्रत्यागम परिणाम;
        }
        यदि (ध्रुव AstConsStruct* ध्रुव avaluep = VN_CAST(valuep, ConsStruct)) {
            string परिणाम = "'{";
            string comma;
            प्रति (ध्रुव AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                परिणाम
                    += comma + itemp->memberp()->name() + ":" + गन्तव्यम्_String_Recurse(itemp->valuep());
                comma = ", ";
            }
            परिणाम += "}";
            प्रत्यागम परिणाम;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        प्रत्यागम "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_ग्रन्थिः_व्यञ्जकम्* newInitValuep(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elementp = newInitValuep(nodep, arrayp->subDTypep());
            प्रत्यागम elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ध्रुव structp = VN_CAST(dtypep, StructDType);
        यदि (structp && !structp->packed()) {
            AstConsStruct* ध्रुव newp = new AstConsStruct{nodep->fileline(), structp};
            प्रति (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव memberValuep = newInitValuep(nodep, memberp->subDTypep());
                यदि (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_वृक्षः(), newp);
                    प्रत्यागम nullptr;
                }
                newp->सङ्कलय_Itemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            प्रत्यागम newp;
        }
        यदि (ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp()) {
            AstConst* ध्रुव constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_मानम्{}, basicp->विस्तारः_Min(), 0};
            यदि (basicp->isZeroInit()) {
                constp->अङ्कः() = V3_सङ्ख्या{nodep, basicp};
            } अन्यथा {
                constp->अङ्कः().समुच्चयः_All_द्वयङ्काः_X();
            }
            प्रत्यागम constp;
        }
        प्रत्यागम nullptr;
    }
    शून्य initVar(AstVar* nodep) {
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_मानम्(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_वृक्षः(), valuep);
        }
    }

    // VISITORS
    शून्य दर्शय(AstAlways* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_Sen_वृक्षः* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    शून्य दर्शय(AstVarRef* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_ग्रन्थिः* ध्रुव vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        यदि (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), ग्रन्थिः_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        यदि (nodep->access().isWriteOrRW()) {
            यदि (m_अन्तर्_Dly_नियोजय) {
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
                ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep
                    = varIsConst ? fetch_मानम्_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                यदि (varIsConst && valuep) {
                    यदि (!m_checkOnly && optimizable()) new_मानम्(vscp, valuep);
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
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = fetch_मानम्_Null(vscp);
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
            समुच्चयः_मानम्(nodep, valuep);
        }
    }
    शून्य दर्शय(AstVarXRef* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (m_scoped) {
            bad_ग्रन्थिः_Type(nodep);
            प्रत्यागम;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!m_params) {
            bad_ग्रन्थिः_Type(nodep);
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
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstInitialAutomaticStmt* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstInitialStaticStmt* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstInitialStatic* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!m_params) {
            bad_ग्रन्थिः_Type(nodep);
            प्रत्यागम;
        }
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        UINFO(5, "   IF " << nodep);
        check_ग्रन्थिः_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterate_And_अग्र_Const_Null(nodep->condp());
            यदि (optimizable()) {
                यदि (fetchConst(nodep->condp())->अङ्कः().isNeqZero()) {
                    iterate_And_अग्र_Const_Null(nodep->thensp());
                } अन्यथा {
                    iterate_And_अग्र_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    शून्य दर्शय(AstConst* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        new_मानम्(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    शून्य दर्शय(AstConsArray* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        AstConsArray* ध्रुव valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_मानम्(nodep->defaultp())->clone_वृक्षः(false) : nullptr};
        प्रति (ध्रुव AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->सूचकांक(), fetch_मानम्(itemp->valuep())->clone_वृक्षः(false));
        }
        m_reclaimValuesp.प्रक्षिप_पृष्ठभागः(valuep);
        समुच्चयः_मानम्(nodep, valuep);
    }
    शून्य दर्शय(AstConsStruct* nodep) override {
        यदि (!m_params) {
            bad_ग्रन्थिः_Type(nodep);
            प्रत्यागम;
        }
        check_ग्रन्थिः_Info(nodep);
        प्रति (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        AstConsStruct* ध्रुव valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        प्रति (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->सङ्कलय_Itemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_मानम्(itemp->valuep())->clone_वृक्षः(false)});
        }
        m_reclaimValuesp.प्रक्षिप_पृष्ठभागः(valuep);
        समुच्चयः_मानम्(nodep, valuep);
    }
    शून्य दर्शय(Ast_Cvt_Unpack_गन्तव्यम्_Unpack* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cvtp = new Ast_Cvt_Unpack_गन्तव्यम्_Unpack{
            nodep->fileline(), fetch_मानम्(nodep->fromp())->clone_वृक्षः(false), nodep->dtypep()};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.प्रक्षिप_पृष्ठभागः(valuep);
        समुच्चयः_मानम्(nodep, valuep);
    }
    शून्य दर्शय(AstConsArrayItem* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstEnumItemRef* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        Ast_ग्रन्थिः* ध्रुव valuep = nodep->itemp()->valuep();
        यदि (valuep) {
            iterate_And_अग्र_Const_Null(valuep);
            यदि (!optimizable()) प्रत्यागम;
            new_मानम्(nodep, fetch_मानम्(valuep));
        } अन्यथा {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Uniop* nodep) override {
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        nodep->सङ्ख्या_सञ्चालय(newConst(nodep)->अङ्कः(), fetchConst(nodep->lhsp())->अङ्कः());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    शून्य aggregateEqRecurse(V3_सङ्ख्या& resultr, Ast_ग्रन्थिः* nodep, bool caseEq, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp,
                            Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        यदि (ध्रुव AstConst* ध्रुव lconstp = VN_CAST(lhsp, Const)) {
            ध्रुव V3_सङ्ख्या& lnum = lconstp->अङ्कः();
            ध्रुव V3_सङ्ख्या& rnum = VN_AS(rhsp, Const)->अङ्कः();
            V3_सङ्ख्या eq{nodep, 1};
            यदि (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } अन्यथा यदि (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } अन्यथा यदि (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } अन्यथा {
                eq.opEq(lnum, rnum);
            }
            ध्रुव V3_सङ्ख्या पूर्व_2 = resultr;
            resultr.opLogAnd(पूर्व_2, eq);
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
    शून्य handleAggregateCompare(Ast_ग्रन्थिः_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ध्रुव bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ध्रुव bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_सङ्ख्या& resultr = newConst(nodep)->अङ्कः();
        resultr.समुच्चयः_All_Bits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_मानम्(nodep->lhsp()),
                           fetch_मानम्(nodep->rhsp()));
        यदि (neq) {
            ध्रुव V3_सङ्ख्या eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Biop* nodep) override {
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        यदि (!VN_IS(fetch_मानम्(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            प्रत्यागम;
        }
        AstConst* ध्रुव valuep = newConst(nodep);
        nodep->सङ्ख्या_सञ्चालय(newConst(nodep)->अङ्कः(), fetchConst(nodep->lhsp())->अङ्कः(),
                             fetchConst(nodep->rhsp())->अङ्कः());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        यदि (!m_params && VN_IS(nodep, Sel) && valuep->अङ्कः().isAnyX()) {
            ध्रुव V3_सङ्ख्या अङ्कः{valuep, valuep->विस्तारः(), valuep->अङ्कः()};
            valuep->अङ्कः().op_द्वयङ्काः_One(अङ्कः);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Triop* nodep) override {
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        nodep->सङ्ख्या_सञ्चालय(newConst(nodep)->अङ्कः(), fetchConst(nodep->lhsp())->अङ्कः(),
                             fetchConst(nodep->rhsp())->अङ्कः(), fetchConst(nodep->thsp())->अङ्कः());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Quadop* nodep) override {
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        nodep->सङ्ख्या_सञ्चालय(newConst(nodep)->अङ्कः(), fetchConst(nodep->lhsp())->अङ्कः(),
                             fetchConst(nodep->rhsp())->अङ्कः(), fetchConst(nodep->thsp())->अङ्कः(),
                             fetchConst(nodep->fhsp())->अङ्कः());
    }
    शून्य दर्शय(AstLogAnd* nodep) override {
        // Need to short circuit
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) प्रत्यागम;
            यदि (fetchConst(nodep->lhsp())->अङ्कः().isNeqZero()) {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) प्रत्यागम;
                new_मानम्(nodep, fetch_मानम्(nodep->rhsp()));
            } अन्यथा {
                new_मानम्(nodep, fetch_मानम्(nodep->lhsp()));  // a zero
            }
        }
    }
    शून्य दर्शय(AstLogOr* nodep) override {
        // Need to short circuit
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) प्रत्यागम;
            यदि (fetchConst(nodep->lhsp())->अङ्कः().isNeqZero()) {
                new_मानम्(nodep, fetch_मानम्(nodep->lhsp()));  // a one
            } अन्यथा {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) प्रत्यागम;
                new_मानम्(nodep, fetch_मानम्(nodep->rhsp()));
            }
        }
    }
    शून्य दर्शय(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) प्रत्यागम;
            यदि (fetchConst(nodep->lhsp())->अङ्कः().isEqZero()) {
                ध्रुव AstConst cnst{nodep->fileline(), AstConst::Widthed_मानम्{}, 1, 1};  // a one
                new_मानम्(nodep, &cnst);  // a one
            } अन्यथा {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) प्रत्यागम;
                new_मानम्(nodep, fetch_मानम्(nodep->rhsp()));
            }
        }
    }
    शून्य दर्शय(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->condp());
            यदि (!optimizable()) प्रत्यागम;
            यदि (fetchConst(nodep->condp())->अङ्कः().isNeqZero()) {
                iterateConst(nodep->thenp());
                यदि (!optimizable()) प्रत्यागम;
                new_मानम्(nodep, fetch_मानम्(nodep->thenp()));
            } अन्यथा {
                iterateConst(nodep->elsep());
                यदि (!optimizable()) प्रत्यागम;
                new_मानम्(nodep, fetch_मानम्(nodep->elsep()));
            }
        }
    }

    शून्य handle_नियोजय_Element(Ast_ग्रन्थिः_नियोजय* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* मानम्_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> selps;
        Ast_ग्रन्थिः_व्यञ्जकम्* fromp = lhsp;
        यावत् (true) {
            यदि (AstArraySel* ध्रुव selp = VN_CAST(fromp, ArraySel)) {
                check_ग्रन्थिः_Info(selp);
                iterate_And_अग्र_Const_Null(selp->bitp());  // Bit index
                selps.प्रक्षिप_पृष्ठभागः(selp);
                fromp = selp->fromp();
            } अन्यथा यदि (AstStructSel* ध्रुव selp = VN_CAST(fromp, StructSel)) {
                check_ग्रन्थिः_Info(selp);
                selps.प्रक्षिप_पृष्ठभागः(selp);
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
        Ast_ग्रन्थिः* ध्रुव vscp = varOrScope(varrefp);
        Ast_ग्रन्थिः_व्यञ्जकम्* rootp = fetch_बहिर्गमनम्_मानम्_Null(vscp);
        यदि (!rootp) rootp = fetch_मानम्_Null(vscp);
        यदि (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            प्रत्यागम;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_ग्रन्थिः_व्यञ्जकम्* itemp = rootp;
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selp : vlstd::reverse_view(selps)) {
            यदि (ध्रुव AstArraySel* ध्रुव aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ध्रुव consp = VN_AS(itemp, ConsArray);
                ध्रुव uint32_t सूचकांक = fetchConst(aselp->bitp())->गन्तव्यम्_UInt();
                ध्रुव पूर्णाङ्क elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                यदि (सूचकांक >= static_cast<uint32_t>(elements)) {
                    प्रत्यागम;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(सूचकांक);
                यदि (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_वृक्षः(false);
                    consp->valuep(सूचकांक, itemp);
                }
            } अन्यथा {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = newTrackedClone(fetch_मानम्(मानम्_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.प्रक्षिप_पृष्ठभागः(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        नियोजय_बहिर्गमनम्_मानम्(nodep, vscp, rootp);
    }
    शून्य handle_नियोजय_Sel(Ast_ग्रन्थिः_नियोजय* nodep, AstSel* selp, Ast_ग्रन्थिः_व्यञ्जकम्* मानम्_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_सङ्ख्या lsb{nodep};
        handle_नियोजय_Sel_Recurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_ग्रन्थिः* ध्रुव vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        यदि (AstConst* ध्रुव vscpnump = fetch_बहिर्गमनम्_Const_Null(vscp)) {
            outconstp = vscpnump;
        } अन्यथा यदि (AstConst* ध्रुव vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } अन्यथा {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_मानम्{},
                                     varrefp->varp()->विस्तारः_Min(), 0};
            यदि (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->अङ्कः().समुच्चयः_All_Bits0();
            } अन्यथा {
                outconstp->अङ्कः().समुच्चयः_All_द्वयङ्काः_X();
            }
            m_reclaimValuesp.emplace_पृष्ठभागः(outconstp);
        }
        outconstp->अङ्कः().opSelInto(fetchConst(मानम्_Fromp)->अङ्कः(), lsb, selp->विस्तारः_Const());
        नियोजय_बहिर्गमनम्_मानम्(nodep, vscp, outconstp);
    }
    शून्य handle_नियोजय_Sel_Recurse(Ast_ग्रन्थिः_नियोजय* nodep, AstSel* selp, AstVarRef*& बहिर्गमनम्_Varrefp_Ref,
                                V3_सङ्ख्या& lsbRef, पूर्णाङ्क गभीरता) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_ग्रन्थिः_Info(selp);
        iterate_And_अग्र_Const_Null(selp->lsbp());  // Bit index
        यदि (AstVarRef* ध्रुव varrefp = VN_CAST(selp->fromp(), VarRef)) {
            बहिर्गमनम्_Varrefp_Ref = varrefp;
            lsbRef = fetchConst(selp->lsbp())->अङ्कः();
            प्रत्यागम;  // And presumably still optimizable()
        }
        यदि (AstSel* ध्रुव subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_सङ्ख्या sublsb{nodep};
            handle_नियोजय_Sel_Recurse(nodep, subselp, बहिर्गमनम्_Varrefp_Ref, sublsb /*ref*/, गभीरता + 1);
            यदि (optimizable()) {
                lsbRef = sublsb;
                lsbRef.op_सङ्कलय(sublsb, fetchConst(selp->lsbp())->अङ्कः());
            }
        } अन्यथा {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    शून्य handle_नियोजय_Recurse(Ast_ग्रन्थिः_नियोजय* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* मानम्_Fromp) {
        यदि (!optimizable()) प्रत्यागम;
        यदि (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            यदि (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                प्रत्यागम;
            }
            handle_नियोजय_Element(nodep, lhsp, मानम्_Fromp);
        } अन्यथा यदि (AstConcat* ध्रुव selp = VN_CAST(lhsp, Concat)) {
            check_ग्रन्थिः_Info(selp);
            यदि (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                प्रत्यागम;
            }
            // Split value into left and right concat values
            यदि (!m_checkOnly) {
                {
                    AstConst* ध्रुव outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_मानम्{},
                                       selp->lhsp()->विस्तारः(), 0};
                    outconstp->अङ्कः().opSel(fetchConst(मानम्_Fromp)->अङ्कः(),
                                           selp->lhsp()->विस्तारः() + selp->rhsp()->विस्तारः() + 1,
                                           selp->rhsp()->विस्तारः());
                    new_मानम्(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_वृक्षः(), outconstp);
                }
                {
                    AstConst* ध्रुव outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_मानम्{},
                                       selp->rhsp()->विस्तारः_Min(), 0};
                    outconstp->अङ्कः().opSel(fetchConst(मानम्_Fromp)->अङ्कः(),
                                           selp->rhsp()->विस्तारः() - 1, 0);
                    new_मानम्(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_वृक्षः(), outconstp);
                }
            }
            handle_नियोजय_Recurse(nodep, selp->lhsp(), selp->lhsp());
            handle_नियोजय_Recurse(nodep, selp->rhsp(), selp->rhsp());
        } अन्यथा यदि (AstReplicate* ध्रुव selp = VN_CAST(lhsp, Replicate)) {
            check_ग्रन्थिः_Info(selp);
            iterate_And_अग्र_Const_Null(selp->countp());
            AstConst* ध्रुव countp = VN_CAST(selp->countp(), Const);
            यदि (!countp || !countp->अङ्कः().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                प्रत्यागम;
            }
            handle_नियोजय_Recurse(nodep, selp->srcp(), मानम्_Fromp);
        } अन्यथा यदि (AstSel* ध्रुव selp = VN_CAST(lhsp, Sel)) {
            यदि (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                प्रत्यागम;
            }
            handle_नियोजय_Sel(nodep, selp, मानम्_Fromp);
        } अन्यथा यदि (VN_IS(lhsp, VarRef)) {
            यदि (m_checkOnly) {
                iterate_And_अग्र_Const_Null(lhsp);
            } अन्यथा {
                Ast_ग्रन्थिः* ध्रुव vscp = varOrScope(VN_CAST(lhsp, VarRef));
                नियोजय_बहिर्गमनम्_मानम्(nodep, vscp, fetch_मानम्(मानम्_Fromp));
            }
        } अन्यथा {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);

        VL_RESTORER(m_अन्तर्_Dly_नियोजय);

        यदि (VN_IS(nodep, नियोजय_Force)) {
            clearOptimizable(nodep, "Force");
        } अन्यथा यदि (VN_IS(nodep, नियोजय_Dly)) {
            यदि (m_any_नियोजय_Comb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_नियोजय_Dly = true;
            m_अन्तर्_Dly_नियोजय = true;
        } अन्यथा {
            यदि (m_any_नियोजय_Dly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_नियोजय_Comb = true;
        }

        यदि (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            प्रत्यागम;
        }

        iterate_And_अग्र_Const_Null(nodep->rhsp());  // Value to assign
        handle_नियोजय_Recurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    शून्य दर्शय(AstArraySel* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (ध्रुव AstConsArray* ध्रुव consp = VN_CAST(fetch_मानम्_Null(nodep->fromp()), ConsArray)) {
            ध्रुव AstConst* ध्रुव indexp = fetchConst(nodep->bitp());
            ध्रुव uint32_t विस्थापनम् = indexp->अङ्कः().गन्तव्यम्_UInt();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव itemp = consp->valuep(विस्थापनम्);
            यदि (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::गन्तव्यम्_string(विस्थापनम्));
            } अन्यथा {
                समुच्चयः_मानम्(nodep, itemp);
            }
        } अन्यथा {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    शून्य दर्शय(AstStructSel* nodep) override {
        यदि (!m_params) {
            bad_ग्रन्थिः_Type(nodep);
            प्रत्यागम;
        }
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        AstConsStruct* ध्रुव consp = VN_AS(fetch_मानम्(nodep->fromp()), ConsStruct);
        समुच्चयः_मानम्(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    शून्य दर्शय(Ast_खण्डिका_Sel* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        // Fetch the base constant array
        यदि (ध्रुव AstConsArray* ध्रुव consp = VN_CAST(fetch_मानम्_Null(nodep->fromp()), ConsArray)) {
            ध्रुव VNumRange& खण्डिका_Range = nodep->declRange();
            ध्रुव uint32_t खण्डिका_Elements = खण्डिका_Range.elements();
            ध्रुव पूर्णाङ्क खण्डिका_Lo = खण्डिका_Range.lo();
            // Use this node's dtype for the slice array
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ध्रुव newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            प्रति (uint32_t idx = 0; idx < खण्डिका_Elements; ++idx) {
                ध्रुव uint32_t आधारः_Idx = खण्डिका_Lo + idx;
                newConsp->valuep(idx, consp->valuep(आधारः_Idx)->clone_वृक्षः(false));
            }
            // Assign the new constant array and track it for later deletion
            समुच्चयः_मानम्(nodep, newConsp);
            m_reclaimValuesp.प्रक्षिप_पृष्ठभागः(newConsp);
        } अन्यथा {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    शून्य दर्शय(AstBegin* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstCase* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        UINFO(5, "   CASE " << nodep);
        check_ग्रन्थिः_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा यदि (optimizable()) {
            iterate_And_अग्र_Const_Null(nodep->exprp());
            bool लब्धम् = false;
            प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                यदि (!itemp->isDefault()) {
                    प्रति (Ast_ग्रन्थिः* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        यदि (लब्धम्) भङ्ग;
                        iterate_And_अग्र_Const_Null(ep);
                        यदि (optimizable()) {
                            V3_सङ्ख्या मेलनम्{nodep, 1};
                            मेलनम्.opEq(fetchConst(nodep->exprp())->अङ्कः(), fetchConst(ep)->अङ्कः());
                            यदि (मेलनम्.isNeqZero()) {
                                iterate_And_अग्र_Const_Null(itemp->stmtsp());
                                लब्धम् = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                यदि (लब्धम्) भङ्ग;
                यदि (itemp->isDefault()) {
                    iterate_And_अग्र_Const_Null(itemp->stmtsp());
                    लब्धम् = true;
                }
            }
        }
    }

    शून्य दर्शय(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        यदि (jumpingOver()) प्रत्यागम;
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }

    शून्य दर्शय(AstComment*) override {}

    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_ग्रन्थिः_Info(nodep);
        iterate_And_अग्र_Const_Null(nodep->stmtsp());
        यदि (!optimizable()) प्रत्यागम;
        iterate_And_अग्र_Const_Null(nodep->resultp());
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        new_मानम्(nodep, fetch_मानम्(nodep->resultp()));
    }

    शून्य दर्शय(Ast_Jump_खण्डम्* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        iterateChildrenConst(nodep);
        यदि (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    शून्य दर्शय(AstJumpGo* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_ग्रन्थिः_Info(nodep);
        यदि (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    शून्य दर्शय(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        यदि (jumpingOver()) प्रत्यागम;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        यदि (!m_params) {
            bad_ग्रन्थिः_Type(nodep);
            प्रत्यागम;
        }
        check_ग्रन्थिः_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
            प्रत्यागम;
        }
        यदि (!optimizable()) प्रत्यागम;

        आकार_t iter_गणनम् = 0;
        ध्रुव आकार_t iter_सीमा = v3Global.opt.unroll_सीमा();
        यावत् (true) {
            यदि (iter_गणनम् > iter_सीमा) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::गन्तव्यम्_string(iter_सीमा));
                भङ्ग;
            }
            ++iter_गणनम्;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
            यदि (jumpingOver()) भङ्ग;
        }

        यदि (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    शून्य दर्शय(AstLoopTest* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        check_ग्रन्थिः_Info(nodep);
        iterateConst(nodep->condp());
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        यदि (fetchConst(nodep->condp())->अङ्कः().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    शून्य दर्शय(AstStop* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_ग्रन्थिः_Info(nodep);
    }
    शून्य दर्शय(AstFuncRef* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_ग्रन्थिः_Info(nodep);
        यदि (!m_params) {
            bad_ग्रन्थिः_Type(nodep);
            प्रत्यागम;
        }
        Ast_ग्रन्थिः_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        यदि (m_params) V3_विस्तारः::विस्तारः_Params_Edit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        यदि (funcp->recursive()) {
            यदि (m_recurse_गणनम् >= v3Global.opt.func_Recursion_गभीरता()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::गन्तव्यम्_string(v3Global.opt.func_Recursion_गभीरता())
                                            + " times");
                प्रत्यागम;
            }
            ++m_recurse_गणनम्;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_ग्रन्थिः*, Ast_ग्रन्थिः_व्यञ्जकम्*> old_मानानि;

        यदि (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_मानानि](AstVar* varp) {
                यदि (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = fetch_मानम्_Null(varp)) {
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nvaluep = newTrackedClone(valuep);
                        old_मानानि.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_मानानि](Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
                यदि (VN_IS(exprp, Const)) प्रत्यागम;  // Speed up as won't change
                यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = fetch_मानम्_Null(exprp)) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nvaluep = newTrackedClone(valuep);
                    old_मानानि.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::सदिशम्<std::pair<AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*>> पत्तनम्_मानानि;
        प्रति (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.अन्तः_2(); ++it) {
            AstVar* ध्रुव portp = it->first;
            Ast_ग्रन्थिः* ध्रुव pinp = it->second->exprp();
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
                पत्तनम्_मानानि.emplace_पृष्ठभागः(std::make_pair(portp, newTrackedClone(fetch_मानम्(pinp))));
            }
        }
        // Apply value to the function
        यदि (!m_checkOnly && optimizable())
            प्रति (ध्रुव स्वचालित& it : पत्तनम्_मानानि) {
                यदि (!m_checkOnly && optimizable()) new_मानम्(it.first, it.second);
            }
        Sim_राशिः_ग्रन्थिः राशिः_ग्रन्थिः{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_राशिः.प्रक्षिप_पृष्ठभागः(&राशिः_ग्रन्थिः);
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
        m_call_राशिः.निष्कासय_पृष्ठभागः();
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* returnp = nullptr;
        यदि (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_मानम्(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        प्रति (ध्रुव स्वचालित& it : old_मानानि) {
            यदि (it.second) new_मानम्(it.first, it.second);
        }
        यदि (returnp) new_मानम्(nodep, returnp);
        यदि (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_गणनम् > 0, nodep, "recurse underflow");
            --m_recurse_गणनम्;
        }
    }

    शून्य दर्शय(AstVar* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!m_params) {
            bad_ग्रन्थिः_Type(nodep);
            प्रत्यागम;
        }
    }

    शून्य दर्शय(AstScopeName* /*nodep*/) override {
        यदि (jumpingOver()) प्रत्यागम;
        // Ignore
    }

    शून्य दर्शय(AstSFormatArg* nodep) override {
        check_ग्रन्थिः_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_अग्र_Const_Null(nodep->exprp());
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;

        Ast_ग्रन्थिः* अग्र_Argp = nodep->exprsp();
        string परिणाम;
        ध्रुव string format = nodep->text();
        स्वचालित pos = format.cbegin();
        bool अन्तर्_Pct = false;
        string विस्तारः;
        प्रति (; pos != format.cend(); ++pos) {
            यदि (!अन्तर्_Pct && pos[0] == '%') {
                अन्तर्_Pct = true;
                विस्तारः = "";
            } अन्यथा यदि (!अन्तर्_Pct) {  // Normal text
                परिणाम += *pos;
            } अन्यथा {  // Format character
                यदि (std::अङ्कः_वा(pos[0])) {
                    विस्तारः += pos[0];
                    अनुवर्त;
                }

                अन्तर्_Pct = false;

                यदि (V3_सङ्ख्या::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_ग्रन्थिः* ध्रुव argp = अग्र_Argp;
                    अग्र_Argp = अग्र_Argp->nextp();
                    AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);
                    Ast_ग्रन्थिः* ध्रुव subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ध्रुव constp = fetchConstNull(subargp);
                    ध्रुव VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    यदि (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        भङ्ग;
                    }
                    ध्रुव string pformat = "%"s + विस्तारः + pos[0];
                    परिणाम += formatAttr.isEnum()
                                  ? constp->अङ्कः().displayedEnum(fargp, pformat)
                                  : constp->अङ्कः().displayed(nodep, pformat, formatAttr);
                } अन्यथा {
                    विकल्प (std::tolower(pos[0])) {
                    पक्ष '%': परिणाम += "%"; भङ्ग;
                    पक्ष 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        परिणाम += "%m";
                        भङ्ग;
                    स्वभाव: clearOptimizable(nodep, "Unknown $display-like format code."); भङ्ग;
                    }
                }
            }
        }

        AstConst* ध्रुव परिणाम_Constp = new AstConst{nodep->fileline(), AstConst::String{}, परिणाम};
        समुच्चयः_मानम्(nodep, परिणाम_Constp);
        m_reclaimValuesp.प्रक्षिप_पृष्ठभागः(परिणाम_Constp);
    }

    शून्य दर्शय(AstDisplay* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_ग्रन्थिः_Info(nodep, /*display:*/ true);
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
    शून्य दर्शय(Ast_गन्तव्यम्_String_N* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        यदि (!optimizable()) प्रत्यागम;  // Accelerate
        check_ग्रन्थिः_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) प्रत्यागम;
        ध्रुव std::string परिणाम = गन्तव्यम्_String_Recurse(fetch_मानम्(nodep->lhsp()));
        यदि (!optimizable()) प्रत्यागम;
        AstConst* ध्रुव परिणाम_Constp = new AstConst{nodep->fileline(), AstConst::String{}, परिणाम};
        समुच्चयः_मानम्(nodep, परिणाम_Constp);
        m_reclaimValuesp.प्रक्षिप_पृष्ठभागः(परिणाम_Constp);
    }

    शून्य दर्शय(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    शून्य दर्शय(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        ज्ञातम्_Bad_ग्रन्थिः_Type(nodep);
    }
    शून्य दर्शय(Ast_Get_Initial_Random_बीजम्* nodep) override { ज्ञातम्_Bad_ग्रन्थिः_Type(nodep); }
    शून्य दर्शय(AstMemberSel* nodep) override { ज्ञातम्_Bad_ग्रन्थिः_Type(nodep); }
    शून्य दर्शय(AstSampled* nodep) override { ज्ञातम्_Bad_ग्रन्थिः_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        यदि (jumpingOver()) प्रत्यागम;
        bad_ग्रन्थिः_Type(nodep);
    }

    // MEMBERS - called by constructor
    शून्य समुच्चयः_बहुलकम्(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    शून्य mainGuts(Ast_ग्रन्थिः* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

सार्वजनिक:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        समुच्चयः_बहुलकम्(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    शून्य clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_any_नियोजय_Comb = false;
        m_any_नियोजय_Dly = false;
        m_अन्तर्_Dly_नियोजय = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_गणनम् = 0;
        m_दत्त_गणनम् = 0;
        m_jumptargetp = nullptr;

        Ast_ग्रन्थिः::user1_Clear_वृक्षः();
        m_varAux.clear();
        ++m_constGeneration;
    }
    शून्य main_सारणी_Check(Ast_ग्रन्थिः* nodep) {
        समुच्चयः_बहुलकम्(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    शून्य main_सारणी_Emulate(Ast_ग्रन्थिः* nodep) {
        समुच्चयः_बहुलकम्(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    शून्य mainParamEmulate(Ast_ग्रन्थिः* nodep) {
        समुच्चयः_बहुलकम्(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::समुच्चयः<Ast_ग्रन्थिः*> अप्रयुक्तम्_Rootps;
        प्रति (Ast_ग्रन्थिः* ध्रुव nodep : m_reclaimValuesp) {
            यदि (!nodep->backp()) अप्रयुक्तम्_Rootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        प्रति (Ast_ग्रन्थिः* ध्रुव nodep : अप्रयुक्तम्_Rootps) VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
};

#यदि_अन्तः  // Guard
