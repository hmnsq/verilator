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

class Sim_चळत_गाठ final {
सार्वजनिक:
    // MEMBERS
    AstFuncRef* स्थिरांक m_funcp;
    V3TaskConnects* स्थिरांक m_tconnects;
    // CONSTRUCTORS
    Sim_चळत_गाठ(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_चळत_गाठ() = पूर्वनिर्धारित;
};

class SimulateVisitor VL_NOT_FINAL : सार्वजनिक VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

खाजगी:
    // CONSTANTS
    स्थिर constexpr पूर्णांक CALL_चळत_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    स्थिरांक VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    गणना VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    संरचना AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_गाठ_अभिव्यक्ती* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_गाठ_अभिव्यक्ती* valuep = nullptr;
    };

    AstUser1Allocator<Ast_गाठ, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    संरचना ConstAllocator final {
        माप_प्रकार m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        माप_प्रकार m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(माप_प्रकार currentGeneration, Ast_गाठ* nodep) {
            जर (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            जर (m_nextFree == m_constps.size()) {
                m_constps.ढकला_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* स्थिरांक constp = m_constps[m_nextFree++];
            constp->अंक().nodep(nodep);
            परत constp;
        }

        ~ConstAllocator() {
            साठी (AstConst* स्थिरांक constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_गाठ* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    पूर्णांक m_instr_मोजणी;  ///< Number of nodes
    पूर्णांक m_data_मोजणी;  ///< Bytes of data
    पूर्णांक m_recurse_मोजणी = 0;  // Now deep in current recursion
    Ast_गाठ* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<स्थिरांक Ast_गाठ_DType*, ConstAllocator> m_constps;
    माप_प्रकार m_constGeneration = 0;
    std::vector<Sim_चळत_गाठ*> m_call_चळत;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_गाठ*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_संख्या(स्थिरांक V3_संख्या* nump, स्थिरांक Ast_गाठ_DType* dtypep) {
        जर (स्थिरांक AstRefDType* स्थिरांक refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक stp = VN_CAST(dtypep, गाठ_UOr_Struct_DType)) {
            जर (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                साठी (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    स्थिरांक पूर्णांक width = itemp->width();
                    स्थिरांक पूर्णांक lsb = itemp->lsb();
                    स्थिरांक पूर्णांक msb = lsb + width - 1;
                    V3_संख्या field_अंक{nump, width};
                    field_अंक.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    जर (स्थिरांक Ast_गाठ_DType* स्थिरांक childTypep = itemp->subDTypep()) {
                        out << pretty_संख्या(&field_अंक, childTypep);
                    } अन्यथा {
                        out << field_अंक;
                    }
                    जर (itemp->nextp()) out << ", ";
                }
                out << "}";
                परत out.str();
            }
        } अन्यथा जर (स्थिरांक AstPackArrayDType* स्थिरांक arrayp = VN_CAST(dtypep, PackArrayDType)) {
            जर (स्थिरांक Ast_गाठ_DType* स्थिरांक childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                स्थिरांक पूर्णांक arrayElements = arrayp->elementsConst();
                साठी (पूर्णांक element = 0; element < arrayElements; ++element) {
                    स्थिरांक पूर्णांक width = childTypep->width();
                    स्थिरांक पूर्णांक lsb = width * element;
                    स्थिरांक पूर्णांक msb = lsb + width - 1;
                    V3_संख्या field_अंक{nump, width};
                    field_अंक.opSel(*nump, msb, lsb);
                    स्थिरांक पूर्णांक arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_संख्या(&field_अंक, childTypep);
                    जर (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                परत out.str();
            }
        }
        परत nump->ascii();
    }

    // Checking METHODS
सार्वजनिक:
    /// Call other-this function on all new *non-constant* var references
    आभासी शून्य varRefCb(AstVarRef* nodep) {}

    शून्य clearOptimizable(Ast_गाठ* nodep /*null ok*/, स्थिरांक string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        जर (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            जर (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                जर (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream चळत;
            पूर्णांक n = 0;
            साठी (स्थिरांक स्वतः& callstack : vlstd::reverse_view(m_call_चळत)) {
                स्थिरांक AstFuncRef* स्थिरांक funcp = callstack->m_funcp;
                चळत << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                साठी (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    स्थिरांक AstVar* स्थिरांक portp = conIt->first;
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक pinp = conIt->second->exprp();
                    स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = pinp->dtypep();
                    जर (AstConst* स्थिरांक valp = fetchConstNull(pinp)) {
                        चळत << "\n           " << portp->prettyName() << " = "
                              << pretty_संख्या(&valp->अंक(), dtypep);
                    } अन्यथा जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = fetch_मूल्य_Null(pinp)) {
                        चळत << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                जर (++n > CALL_चळत_MAX) {
                    चळत << "\n           ... stack truncated";
                    थांबा;
                }
            }
            m_whyNotOptimizable += चळत.str();
        }
    }
    bool optimizable() स्थिरांक { परत m_whyNotNodep == nullptr; }
    string whyNotMessage() स्थिरांक { परत m_whyNotOptimizable; }
    Ast_गाठ* whyNotNodep() स्थिरांक { परत m_whyNotNodep; }

    bool isAssignDly() स्थिरांक { परत m_anyAssignDly; }
    bool isImpure() स्थिरांक { परत m_isImpure; }
    bool isCoverage() स्थिरांक { परत m_isCoverage; }
    पूर्णांक instr_मोजणी() स्थिरांक { परत m_instr_मोजणी; }
    पूर्णांक data_मोजणी() स्थिरांक { परत m_data_मोजणी; }

    // Simulation METHODS
खाजगी:
    AstConst* allocConst(Ast_गाठ* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        परत m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

सार्वजनिक:
    शून्य new_मूल्य(Ast_गाठ* nodep, स्थिरांक Ast_गाठ_अभिव्यक्ती* valuep) {
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->अंक().opAssign(constp->अंक());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } अन्यथा जर (fetch_मूल्य_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_मूल्य(nodep, newTrackedClone(const_cast<Ast_गाठ_अभिव्यक्ती*>(valuep)));
        }
    }
    शून्य new_Out_मूल्य(Ast_गाठ* nodep, स्थिरांक Ast_गाठ_अभिव्यक्ती* valuep) {
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->अंक().opAssign(constp->अंक());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } अन्यथा जर (fetch_Out_मूल्य_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_मूल्य(nodep, newTrackedClone(const_cast<Ast_गाठ_अभिव्यक्ती*>(valuep)));
        }
    }

खाजगी:
    Ast_गाठ_अभिव्यक्ती* newTrackedClone(Ast_गाठ_अभिव्यक्ती* nodep) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = nodep->clone_वृक्ष(false);
        m_reclaimValuesp.ढकला_back(newp);
        परत newp;
    }
    AstConst* newConst(Ast_गाठ* nodep) {
        // Set a constant value for this node
        जर (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* स्थिरांक constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            परत constp;
        }
        परत fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_गाठ* nodep) {
        // Set a var-output constant value for this node
        जर (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* स्थिरांक constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            परत constp;
        }
        परत fetchOutConst(nodep);
    }

सार्वजनिक:
    Ast_गाठ_अभिव्यक्ती* fetch_मूल्य_Null(Ast_गाठ* nodep) { परत m_varAux(nodep).valuep; }

खाजगी:
    Ast_गाठ_अभिव्यक्ती* fetch_Out_मूल्य_Null(Ast_गाठ* nodep) { परत m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_गाठ* nodep) { परत VN_CAST(fetch_मूल्य_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_गाठ* nodep) {
        परत VN_CAST(fetch_Out_मूल्य_Null(nodep), Const);
    }
    Ast_गाठ_अभिव्यक्ती* fetch_मूल्य(Ast_गाठ* nodep) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = fetch_मूल्य_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        परत valuep;
    }
    AstConst* fetchConst(Ast_गाठ* nodep) {
        AstConst* स्थिरांक constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        परत constp;
    }
    AstConst* fetchOutConst(Ast_गाठ* nodep) {
        AstConst* स्थिरांक constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        परत constp;
    }

सार्वजनिक:
    V3_संख्या* fetch_संख्या_Null(Ast_गाठ* nodep) {
        AstConst* स्थिरांक constp = fetchConstNull(nodep);
        जर (constp) परत &constp->अंक();
        परत nullptr;
    }
    V3_संख्या* fetch_Out_संख्या_Null(Ast_गाठ* nodep) {
        AstConst* स्थिरांक constp = fetchOutConstNull(nodep);
        जर (constp) परत &constp->अंक();
        परत nullptr;
    }

खाजगी:
    शून्य set_मूल्य(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    शून्य set_Out_मूल्य(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    शून्य check_गाठ_Info(Ast_गाठ* nodep, bool ignorePredict = false) {
        जर (m_checkOnly) {
            m_instr_मोजणी += nodep->instr_मोजणी();
            m_data_मोजणी += nodep->width();
        }
        जर (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        जर (!nodep->isPure()) m_isImpure = true;
    }

    शून्य known_Bad_गाठ_Type(Ast_गाठ* nodep) {
        // Call for node types we know we can't handle
        जर (jumpingOver()) परत;
        check_गाठ_Info(nodep);
        जर (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    शून्य bad_गाठ_Type(Ast_गाठ* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_गाठ_Info(nodep);
        जर (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            स्थिर std::set<VNType> s_typePrinted;
            स्थिरांक स्वतः pair = s_typePrinted.emplace(nodep->type());
            जर (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_गाठ* varOrScope(AstVarRef* nodep) स्थिरांक {
        Ast_गाठ* vscp;
        जर (m_scoped) {
            vscp = nodep->varScopep();
        } अन्यथा {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        परत vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() स्थिरांक { परत m_jumptargetp; }
    शून्य assign_Out_मूल्य(स्थिरांक Ast_गाठ_Assign* nodep, Ast_गाठ* vscp, स्थिरांक Ast_गाठ_अभिव्यक्ती* valuep) {
        जर (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_मूल्य(vscp, valuep);
        } अन्यथा {
            new_मूल्य(vscp, valuep);
            set_Out_मूल्य(vscp, fetch_मूल्य(vscp));
        }
    }

    string toStringRecurse(Ast_गाठ_अभिव्यक्ती* valuep) {
        // Return string representation of a value, or clearOptimizable
        जर (स्थिरांक AstConst* स्थिरांक avaluep = VN_CAST(valuep, Const)) {
            परत "'h"s + avaluep->अंक().displayed(valuep, "%0x");
        }
        जर (स्थिरांक AstConsArray* स्थिरांक avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            स्थिरांक AstUnpackArrayDType* स्थिरांक dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            साठी (पूर्णांक n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            परत result;
        }
        जर (स्थिरांक AstConsStruct* स्थिरांक avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            साठी (स्थिरांक AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            परत result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        परत "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_गाठ_अभिव्यक्ती* newInitValuep(Ast_गाठ* nodep, Ast_गाठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        जर (AstUnpackArrayDType* स्थिरांक arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक elementp = newInitValuep(nodep, arrayp->subDTypep());
            परत elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* स्थिरांक structp = VN_CAST(dtypep, StructDType);
        जर (structp && !structp->packed()) {
            AstConsStruct* स्थिरांक newp = new AstConsStruct{nodep->fileline(), structp};
            साठी (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक memberValuep = newInitValuep(nodep, memberp->subDTypep());
                जर (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_वृक्ष(), newp);
                    परत nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            परत newp;
        }
        जर (स्थिरांक AstBasicDType* स्थिरांक basicp = dtypep->basicp()) {
            AstConst* स्थिरांक constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, basicp->widthMin(), 0};
            जर (basicp->isZeroInit()) {
                constp->अंक() = V3_संख्या{nodep, basicp};
            } अन्यथा {
                constp->अंक().setAllBitsX();
            }
            परत constp;
        }
        परत nullptr;
    }
    शून्य initVar(AstVar* nodep) {
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_मूल्य(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_वृक्ष(), valuep);
        }
    }

    // VISITORS
    शून्य visit(AstAlways* nodep) override {
        जर (jumpingOver()) परत;
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_Sen_वृक्ष* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    शून्य visit(AstVarRef* nodep) override {
        जर (jumpingOver()) परत;
        जर (!optimizable()) परत;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_गाठ* स्थिरांक vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        जर (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), गाठ_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        जर (nodep->access().isWriteOrRW()) {
            जर (m_inDlyAssign) {
                जर (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    जर (m_checkOnly) varRefCb(nodep);
                }
            } अन्यथा {  // nondly asn
                जर (!(m_varAux(vscp).usage & VU_LV)) {
                    जर (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    जर (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        जर (nodep->access().isReadOrRW()) {
            जर (!(m_varAux(vscp).usage & VU_RV)) {
                जर (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                स्थिरांक bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep
                    = varIsConst ? fetch_मूल्य_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                जर (varIsConst && valuep) {
                    जर (!m_checkOnly && optimizable()) new_मूल्य(vscp, valuep);
                } अन्यथा {
                    जर (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        जर (m_checkOnly || !optimizable()) परत;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_गाठ_अभिव्यक्ती* valuep = fetch_मूल्य_Null(vscp);
            जर (!valuep) {
                जर (m_params) {
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
        जर (jumpingOver()) परत;
        जर (m_scoped) {
            bad_गाठ_Type(nodep);
            परत;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        जर (jumpingOver()) परत;
        जर (!m_params) {
            bad_गाठ_Type(nodep);
            परत;
        }
        जर (nodep->dpiImport()) {
            जर (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        जर (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStatic* nodep) override {
        जर (jumpingOver()) परत;
        जर (!m_params) {
            bad_गाठ_Type(nodep);
            परत;
        }
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाठ_If* nodep) override {
        जर (jumpingOver()) परत;
        UINFO(5, "   IF " << nodep);
        check_गाठ_Info(nodep);
        जर (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateAndNextConstNull(nodep->condp());
            जर (optimizable()) {
                जर (fetchConst(nodep->condp())->अंक().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } अन्यथा {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    शून्य visit(AstConst* nodep) override {
        check_गाठ_Info(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        new_मूल्य(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    शून्य visit(AstConsArray* nodep) override {
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        AstConsArray* स्थिरांक valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_मूल्य(nodep->defaultp())->clone_वृक्ष(false) : nullptr};
        साठी (स्थिरांक AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_मूल्य(itemp->valuep())->clone_वृक्ष(false));
        }
        m_reclaimValuesp.ढकला_back(valuep);
        set_मूल्य(nodep, valuep);
    }
    शून्य visit(AstConsStruct* nodep) override {
        जर (!m_params) {
            bad_गाठ_Type(nodep);
            परत;
        }
        check_गाठ_Info(nodep);
        साठी (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        जर (m_checkOnly || !optimizable()) परत;
        AstConsStruct* स्थिरांक valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        साठी (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_मूल्य(itemp->valuep())->clone_वृक्ष(false)});
        }
        m_reclaimValuesp.ढकला_back(valuep);
        set_मूल्य(nodep, valuep);
    }
    शून्य visit(AstCvtUnpackToUnpack* nodep) override {
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_गाठ_अभिव्यक्ती* स्थिरांक cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_मूल्य(nodep->fromp())->clone_वृक्ष(false), nodep->dtypep()};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ढकला_back(valuep);
        set_मूल्य(nodep, valuep);
    }
    शून्य visit(AstConsArrayItem* nodep) override {
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        check_गाठ_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        जर (m_checkOnly || !optimizable()) परत;
        Ast_गाठ* स्थिरांक valuep = nodep->itemp()->valuep();
        जर (valuep) {
            iterateAndNextConstNull(valuep);
            जर (!optimizable()) परत;
            new_मूल्य(nodep, fetch_मूल्य(valuep));
        } अन्यथा {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    शून्य visit(Ast_गाठ_Uniop* nodep) override {
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        nodep->संख्या_Operate(newConst(nodep)->अंक(), fetchConst(nodep->lhsp())->अंक());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    शून्य aggregateEqRecurse(V3_संख्या& resultr, Ast_गाठ* nodep, bool caseEq, Ast_गाठ_अभिव्यक्ती* lhsp,
                            Ast_गाठ_अभिव्यक्ती* rhsp) {
        जर (स्थिरांक AstConst* स्थिरांक lconstp = VN_CAST(lhsp, Const)) {
            स्थिरांक V3_संख्या& lnum = lconstp->अंक();
            स्थिरांक V3_संख्या& rnum = VN_AS(rhsp, Const)->अंक();
            V3_संख्या eq{nodep, 1};
            जर (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } अन्यथा जर (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } अन्यथा जर (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } अन्यथा {
                eq.opEq(lnum, rnum);
            }
            स्थिरांक V3_संख्या prev = resultr;
            resultr.opLogAnd(prev, eq);
        } अन्यथा जर (स्थिरांक AstConsArray* स्थिरांक lconsp = VN_CAST(lhsp, ConsArray)) {
            स्थिरांक AstUnpackArrayDType* स्थिरांक dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            साठी (पूर्णांक n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } अन्यथा {
            स्थिरांक AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            साठी (स्थिरांक AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    शून्य handleAggregateCompare(Ast_गाठ_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        स्थिरांक bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        स्थिरांक bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_संख्या& resultr = newConst(nodep)->अंक();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_मूल्य(nodep->lhsp()),
                           fetch_मूल्य(nodep->rhsp()));
        जर (neq) {
            स्थिरांक V3_संख्या eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    शून्य visit(Ast_गाठ_Biop* nodep) override {
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        जर (!VN_IS(fetch_मूल्य(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            परत;
        }
        AstConst* स्थिरांक valuep = newConst(nodep);
        nodep->संख्या_Operate(newConst(nodep)->अंक(), fetchConst(nodep->lhsp())->अंक(),
                             fetchConst(nodep->rhsp())->अंक());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        जर (!m_params && VN_IS(nodep, Sel) && valuep->अंक().isAnyX()) {
            स्थिरांक V3_संख्या अंक{valuep, valuep->width(), valuep->अंक()};
            valuep->अंक().opBitsOne(अंक);
        }
    }
    शून्य visit(Ast_गाठ_Triop* nodep) override {
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        nodep->संख्या_Operate(newConst(nodep)->अंक(), fetchConst(nodep->lhsp())->अंक(),
                             fetchConst(nodep->rhsp())->अंक(), fetchConst(nodep->thsp())->अंक());
    }
    शून्य visit(Ast_गाठ_Quadop* nodep) override {
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        nodep->संख्या_Operate(newConst(nodep)->अंक(), fetchConst(nodep->lhsp())->अंक(),
                             fetchConst(nodep->rhsp())->अंक(), fetchConst(nodep->thsp())->अंक(),
                             fetchConst(nodep->fhsp())->अंक());
    }
    शून्य visit(AstLogAnd* nodep) override {
        // Need to short circuit
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        जर (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            जर (!optimizable()) परत;
            जर (fetchConst(nodep->lhsp())->अंक().isNeqZero()) {
                iterateConst(nodep->rhsp());
                जर (!optimizable()) परत;
                new_मूल्य(nodep, fetch_मूल्य(nodep->rhsp()));
            } अन्यथा {
                new_मूल्य(nodep, fetch_मूल्य(nodep->lhsp()));  // a zero
            }
        }
    }
    शून्य visit(AstLogOr* nodep) override {
        // Need to short circuit
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        जर (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            जर (!optimizable()) परत;
            जर (fetchConst(nodep->lhsp())->अंक().isNeqZero()) {
                new_मूल्य(nodep, fetch_मूल्य(nodep->lhsp()));  // a one
            } अन्यथा {
                iterateConst(nodep->rhsp());
                जर (!optimizable()) परत;
                new_मूल्य(nodep, fetch_मूल्य(nodep->rhsp()));
            }
        }
    }
    शून्य visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        जर (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            जर (!optimizable()) परत;
            जर (fetchConst(nodep->lhsp())->अंक().isEqZero()) {
                स्थिरांक AstConst cnst{nodep->fileline(), AstConst::Widthed_मूल्य{}, 1, 1};  // a one
                new_मूल्य(nodep, &cnst);  // a one
            } अन्यथा {
                iterateConst(nodep->rhsp());
                जर (!optimizable()) परत;
                new_मूल्य(nodep, fetch_मूल्य(nodep->rhsp()));
            }
        }
    }
    शून्य visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        जर (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->condp());
            जर (!optimizable()) परत;
            जर (fetchConst(nodep->condp())->अंक().isNeqZero()) {
                iterateConst(nodep->thenp());
                जर (!optimizable()) परत;
                new_मूल्य(nodep, fetch_मूल्य(nodep->thenp()));
            } अन्यथा {
                iterateConst(nodep->elsep());
                जर (!optimizable()) परत;
                new_मूल्य(nodep, fetch_मूल्य(nodep->elsep()));
            }
        }
    }

    शून्य handleAssignElement(Ast_गाठ_Assign* nodep, Ast_गाठ_अभिव्यक्ती* lhsp, Ast_गाठ_अभिव्यक्ती* मूल्य_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_गाठ_अभिव्यक्ती*> selps;
        Ast_गाठ_अभिव्यक्ती* fromp = lhsp;
        पर्यंत (true) {
            जर (AstArraySel* स्थिरांक selp = VN_CAST(fromp, ArraySel)) {
                check_गाठ_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.ढकला_back(selp);
                fromp = selp->fromp();
            } अन्यथा जर (AstStructSel* स्थिरांक selp = VN_CAST(fromp, StructSel)) {
                check_गाठ_Info(selp);
                selps.ढकला_back(selp);
                fromp = selp->fromp();
            } अन्यथा {
                थांबा;
            }
        }
        AstVarRef* स्थिरांक varrefp = VN_CAST(fromp, VarRef);
        जर (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            परत;
        }
        जर (m_checkOnly || !optimizable()) परत;
        Ast_गाठ* स्थिरांक vscp = varOrScope(varrefp);
        Ast_गाठ_अभिव्यक्ती* rootp = fetch_Out_मूल्य_Null(vscp);
        जर (!rootp) rootp = fetch_मूल्य_Null(vscp);
        जर (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            परत;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_गाठ_अभिव्यक्ती* itemp = rootp;
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक selp : vlstd::reverse_view(selps)) {
            जर (स्थिरांक AstArraySel* स्थिरांक aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* स्थिरांक consp = VN_AS(itemp, ConsArray);
                स्थिरांक uint32_t index = fetchConst(aselp->bitp())->toUInt();
                स्थिरांक पूर्णांक elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                जर (index >= static_cast<uint32_t>(elements)) {
                    परत;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                जर (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_वृक्ष(false);
                    consp->valuep(index, itemp);
                }
            } अन्यथा {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = newTrackedClone(fetch_मूल्य(मूल्य_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ढकला_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_मूल्य(nodep, vscp, rootp);
    }
    शून्य handleAssignSel(Ast_गाठ_Assign* nodep, AstSel* selp, Ast_गाठ_अभिव्यक्ती* मूल्य_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_संख्या lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        जर (m_checkOnly || !optimizable()) परत;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_गाठ* स्थिरांक vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        जर (AstConst* स्थिरांक vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } अन्यथा जर (AstConst* स्थिरांक vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } अन्यथा {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{},
                                     varrefp->varp()->widthMin(), 0};
            जर (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->अंक().setAllBits0();
            } अन्यथा {
                outconstp->अंक().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->अंक().opSelInto(fetchConst(मूल्य_Fromp)->अंक(), lsb, selp->widthConst());
        assign_Out_मूल्य(nodep, vscp, outconstp);
    }
    शून्य handleAssignSelRecurse(Ast_गाठ_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_संख्या& lsbRef, पूर्णांक depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_गाठ_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        जर (AstVarRef* स्थिरांक varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->अंक();
            परत;  // And presumably still optimizable()
        }
        जर (AstSel* स्थिरांक subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_संख्या sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            जर (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->अंक());
            }
        } अन्यथा {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    शून्य handleAssignRecurse(Ast_गाठ_Assign* nodep, Ast_गाठ_अभिव्यक्ती* lhsp, Ast_गाठ_अभिव्यक्ती* मूल्य_Fromp) {
        जर (!optimizable()) परत;
        जर (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            जर (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                परत;
            }
            handleAssignElement(nodep, lhsp, मूल्य_Fromp);
        } अन्यथा जर (AstConcat* स्थिरांक selp = VN_CAST(lhsp, Concat)) {
            check_गाठ_Info(selp);
            जर (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                परत;
            }
            // Split value into left and right concat values
            जर (!m_checkOnly) {
                {
                    AstConst* स्थिरांक outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_मूल्य{},
                                       selp->lhsp()->width(), 0};
                    outconstp->अंक().opSel(fetchConst(मूल्य_Fromp)->अंक(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_मूल्य(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_वृक्ष(), outconstp);
                }
                {
                    AstConst* स्थिरांक outconstp
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
        } अन्यथा जर (AstReplicate* स्थिरांक selp = VN_CAST(lhsp, Replicate)) {
            check_गाठ_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* स्थिरांक countp = VN_CAST(selp->countp(), Const);
            जर (!countp || !countp->अंक().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                परत;
            }
            handleAssignRecurse(nodep, selp->srcp(), मूल्य_Fromp);
        } अन्यथा जर (AstSel* स्थिरांक selp = VN_CAST(lhsp, Sel)) {
            जर (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                परत;
            }
            handleAssignSel(nodep, selp, मूल्य_Fromp);
        } अन्यथा जर (VN_IS(lhsp, VarRef)) {
            जर (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } अन्यथा {
                Ast_गाठ* स्थिरांक vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_मूल्य(nodep, vscp, fetch_मूल्य(मूल्य_Fromp));
            }
        } अन्यथा {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (jumpingOver()) परत;
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        जर (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } अन्यथा जर (VN_IS(nodep, AssignDly)) {
            जर (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } अन्यथा {
            जर (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        जर (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            परत;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    शून्य visit(AstArraySel* nodep) override {
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (स्थिरांक AstConsArray* स्थिरांक consp = VN_CAST(fetch_मूल्य_Null(nodep->fromp()), ConsArray)) {
            स्थिरांक AstConst* स्थिरांक indexp = fetchConst(nodep->bitp());
            स्थिरांक uint32_t offset = indexp->अंक().toUInt();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक itemp = consp->valuep(offset);
            जर (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } अन्यथा {
                set_मूल्य(nodep, itemp);
            }
        } अन्यथा {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    शून्य visit(AstStructSel* nodep) override {
        जर (!m_params) {
            bad_गाठ_Type(nodep);
            परत;
        }
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        AstConsStruct* स्थिरांक consp = VN_AS(fetch_मूल्य(nodep->fromp()), ConsStruct);
        set_मूल्य(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    शून्य visit(AstSliceSel* nodep) override {
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        // Fetch the base constant array
        जर (स्थिरांक AstConsArray* स्थिरांक consp = VN_CAST(fetch_मूल्य_Null(nodep->fromp()), ConsArray)) {
            स्थिरांक VNumRange& sliceRange = nodep->declRange();
            स्थिरांक uint32_t sliceElements = sliceRange.elements();
            स्थिरांक पूर्णांक sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_गाठ_DType* स्थिरांक dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* स्थिरांक newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            साठी (uint32_t idx = 0; idx < sliceElements; ++idx) {
                स्थिरांक uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_वृक्ष(false));
            }
            // Assign the new constant array and track it for later deletion
            set_मूल्य(nodep, newConsp);
            m_reclaimValuesp.ढकला_back(newConsp);
        } अन्यथा {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    शून्य visit(AstBegin* nodep) override {
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCase* nodep) override {
        जर (jumpingOver()) परत;
        UINFO(5, "   CASE " << nodep);
        check_गाठ_Info(nodep);
        जर (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा जर (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            साठी (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                जर (!itemp->isDefault()) {
                    साठी (Ast_गाठ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        जर (hit) थांबा;
                        iterateAndNextConstNull(ep);
                        जर (optimizable()) {
                            V3_संख्या match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->अंक(), fetchConst(ep)->अंक());
                            जर (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            साठी (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                जर (hit) थांबा;
                जर (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    शून्य visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        जर (jumpingOver()) परत;
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstComment*) override {}

    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override {
        जर (jumpingOver()) परत;
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        जर (jumpingOver()) परत;
        check_गाठ_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        जर (!optimizable()) परत;
        iterateAndNextConstNull(nodep->resultp());
        जर (m_checkOnly || !optimizable()) परत;
        new_मूल्य(nodep, fetch_मूल्य(nodep->resultp()));
    }

    शून्य visit(AstJumpBlock* nodep) override {
        जर (jumpingOver()) परत;
        iterateChildrenConst(nodep);
        जर (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    शून्य visit(AstJumpGo* nodep) override {
        जर (jumpingOver()) परत;
        check_गाठ_Info(nodep);
        जर (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        जर (jumpingOver()) परत;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        जर (!m_params) {
            bad_गाठ_Type(nodep);
            परत;
        }
        check_गाठ_Info(nodep);
        जर (m_checkOnly) {
            iterateChildrenConst(nodep);
            परत;
        }
        जर (!optimizable()) परत;

        माप_प्रकार iter_मोजणी = 0;
        स्थिरांक माप_प्रकार iterLimit = v3Global.opt.unrollLimit();
        पर्यंत (true) {
            जर (iter_मोजणी > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                थांबा;
            }
            ++iter_मोजणी;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            जर (jumpingOver()) थांबा;
        }

        जर (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    शून्य visit(AstLoopTest* nodep) override {
        जर (jumpingOver()) परत;
        check_गाठ_Info(nodep);
        iterateConst(nodep->condp());
        जर (m_checkOnly || !optimizable()) परत;
        जर (fetchConst(nodep->condp())->अंक().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    शून्य visit(AstStop* nodep) override {
        जर (jumpingOver()) परत;
        जर (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_गाठ_Info(nodep);
    }
    शून्य visit(AstFuncRef* nodep) override {
        जर (jumpingOver()) परत;
        जर (!optimizable()) परत;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_गाठ_Info(nodep);
        जर (!m_params) {
            bad_गाठ_Type(nodep);
            परत;
        }
        Ast_गाठ_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        जर (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        जर (funcp->recursive()) {
            जर (m_recurse_मोजणी >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                परत;
            }
            ++m_recurse_मोजणी;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_गाठ*, Ast_गाठ_अभिव्यक्ती*> old_मूल्ये;

        जर (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_मूल्ये](AstVar* varp) {
                जर (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = fetch_मूल्य_Null(varp)) {
                        Ast_गाठ_अभिव्यक्ती* स्थिरांक nvaluep = newTrackedClone(valuep);
                        old_मूल्ये.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_मूल्ये](Ast_गाठ_अभिव्यक्ती* exprp) {
                जर (VN_IS(exprp, Const)) परत;  // Speed up as won't change
                जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = fetch_मूल्य_Null(exprp)) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक nvaluep = newTrackedClone(valuep);
                    old_मूल्ये.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_गाठ_अभिव्यक्ती*>> port_मूल्ये;
        साठी (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* स्थिरांक portp = it->first;
            Ast_गाठ* स्थिरांक pinp = it->second->exprp();
            जर (pinp) {  // Else too few arguments in function call - ignore it
                जर (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    परत;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_मूल्ये.emplace_back(std::make_pair(portp, newTrackedClone(fetch_मूल्य(pinp))));
            }
        }
        // Apply value to the function
        जर (!m_checkOnly && optimizable())
            साठी (स्थिरांक स्वतः& it : port_मूल्ये) {
                जर (!m_checkOnly && optimizable()) new_मूल्य(it.first, it.second);
            }
        Sim_चळत_गाठ चळत_गाठ{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_चळत.ढकला_back(&चळत_गाठ);
        जर (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                जर (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_चळत.काढा_back();
        स्थिरांक Ast_गाठ_अभिव्यक्ती* returnp = nullptr;
        जर (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_मूल्य(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        साठी (स्थिरांक स्वतः& it : old_मूल्ये) {
            जर (it.second) new_मूल्य(it.first, it.second);
        }
        जर (returnp) new_मूल्य(nodep, returnp);
        जर (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_मोजणी > 0, nodep, "recurse underflow");
            --m_recurse_मोजणी;
        }
    }

    शून्य visit(AstVar* nodep) override {
        जर (jumpingOver()) परत;
        जर (!m_params) {
            bad_गाठ_Type(nodep);
            परत;
        }
    }

    शून्य visit(AstScopeName* /*nodep*/) override {
        जर (jumpingOver()) परत;
        // Ignore
    }

    शून्य visit(AstSFormatArg* nodep) override {
        check_गाठ_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    शून्य visit(AstSFormatF* nodep) override {
        जर (jumpingOver()) परत;
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;

        Ast_गाठ* nextArgp = nodep->exprsp();
        string result;
        स्थिरांक string format = nodep->text();
        स्वतः pos = format.cbegin();
        bool inPct = false;
        string width;
        साठी (; pos != format.cend(); ++pos) {
            जर (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } अन्यथा जर (!inPct) {  // Normal text
                result += *pos;
            } अन्यथा {  // Format character
                जर (std::है_अंक(pos[0])) {
                    width += pos[0];
                    सुरूठेवा;
                }

                inPct = false;

                जर (V3_संख्या::displayedFmtHasArg(std::छोटे(pos[0]), false)) {
                    Ast_गाठ* स्थिरांक argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* स्थिरांक fargp = VN_CAST(argp, SFormatArg);
                    Ast_गाठ* स्थिरांक subargp = fargp ? fargp->exprp() : argp;
                    AstConst* स्थिरांक constp = fetchConstNull(subargp);
                    स्थिरांक VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    जर (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        थांबा;
                    }
                    स्थिरांक string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->अंक().displayedEnum(fargp, pformat)
                                  : constp->अंक().displayed(nodep, pformat, formatAttr);
                } अन्यथा {
                    निवड (std::छोटे(pos[0])) {
                    स्थिती '%': result += "%"; थांबा;
                    स्थिती 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        थांबा;
                    पूर्वनिर्धारित: clearOptimizable(nodep, "Unknown $display-like format code."); थांबा;
                    }
                }
            }
        }

        AstConst* स्थिरांक resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_मूल्य(nodep, resultConstp);
        m_reclaimValuesp.ढकला_back(resultConstp);
    }

    शून्य visit(AstDisplay* nodep) override {
        जर (jumpingOver()) परत;
        जर (!optimizable()) परत;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_गाठ_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        जर (m_params) {
            स्थिरांक AstConst* स्थिरांक textp = fetchConst(nodep->fmtp());
            निवड (nodep->displayType()) {
            स्थिती VDisplayType::DT_DISPLAY:  // FALLTHRU
            स्थिती VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); थांबा;
            स्थिती VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); थांबा;
            स्थिती VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); थांबा;
            स्थिती VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); थांबा;
            स्थिती VDisplayType::DT_WRITE:  // FALLTHRU
            पूर्वनिर्धारित: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    शून्य visit(AstToStringN* nodep) override {
        जर (jumpingOver()) परत;
        जर (!optimizable()) परत;  // Accelerate
        check_गाठ_Info(nodep);
        iterateChildrenConst(nodep);
        जर (m_checkOnly || !optimizable()) परत;
        स्थिरांक std::string result = toStringRecurse(fetch_मूल्य(nodep->lhsp()));
        जर (!optimizable()) परत;
        AstConst* स्थिरांक resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_मूल्य(nodep, resultConstp);
        m_reclaimValuesp.ढकला_back(resultConstp);
    }

    शून्य visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    शून्य visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_गाठ_Type(nodep);
    }
    शून्य visit(AstGetInitialRandomSeed* nodep) override { known_Bad_गाठ_Type(nodep); }
    शून्य visit(AstMemberSel* nodep) override { known_Bad_गाठ_Type(nodep); }
    शून्य visit(AstSampled* nodep) override { known_Bad_गाठ_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    शून्य visit(Ast_गाठ* nodep) override {
        जर (jumpingOver()) परत;
        bad_गाठ_Type(nodep);
    }

    // MEMBERS - called by constructor
    शून्य setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    शून्य mainGuts(Ast_गाठ* nodep) {
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
        m_instr_मोजणी = 0;
        m_data_मोजणी = 0;
        m_jumptargetp = nullptr;

        Ast_गाठ::user1_Clear_वृक्ष();
        m_varAux.clear();
        ++m_constGeneration;
    }
    शून्य mainTableCheck(Ast_गाठ* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    शून्य mainTableEmulate(Ast_गाठ* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    शून्य mainParamEmulate(Ast_गाठ* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_गाठ*> unusedRootps;
        साठी (Ast_गाठ* स्थिरांक nodep : m_reclaimValuesp) {
            जर (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        साठी (Ast_गाठ* स्थिरांक nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
};

#endif  // Guard
