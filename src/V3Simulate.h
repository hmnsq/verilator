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

class Sim_थाक_गाँठो final {
सार्वजनिक:
    // MEMBERS
    AstFuncRef* अचर m_funcp;
    V3TaskConnects* अचर m_tconnects;
    // CONSTRUCTORS
    Sim_थाक_गाँठो(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_थाक_गाँठो() = स्वभाव;
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
    स्थिर constexpr पूर्णाङ्क CALL_थाक_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    अचर VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    गणना VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    संरचना AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_गाँठो_अभिव्यक्ति* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_गाँठो_अभिव्यक्ति* valuep = nullptr;
    };

    AstUser1Allocator<Ast_गाँठो, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    संरचना ConstAllocator final {
        माप_प्रकार m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        माप_प्रकार m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(माप_प्रकार currentGeneration, Ast_गाँठो* nodep) {
            यदि (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            यदि (m_nextFree == m_constps.size()) {
                m_constps.थप_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* अचर constp = m_constps[m_nextFree++];
            constp->अङ्क().nodep(nodep);
            फर्काऊ constp;
        }

        ~ConstAllocator() {
            लागि (AstConst* अचर constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_गाँठो* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    पूर्णाङ्क m_instr_गन्ती;  ///< Number of nodes
    पूर्णाङ्क m_data_गन्ती;  ///< Bytes of data
    पूर्णाङ्क m_recurse_गन्ती = 0;  // Now deep in current recursion
    Ast_गाँठो* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<अचर Ast_गाँठो_DType*, ConstAllocator> m_constps;
    माप_प्रकार m_constGeneration = 0;
    std::vector<Sim_थाक_गाँठो*> m_call_थाक;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_गाँठो*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_सङ्ख्या(अचर V3_सङ्ख्या* nump, अचर Ast_गाँठो_DType* dtypep) {
        यदि (अचर AstRefDType* अचर refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        यदि (अचर Ast_गाँठो_UOr_Struct_DType* अचर stp = VN_CAST(dtypep, गाँठो_UOr_Struct_DType)) {
            यदि (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                लागि (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    अचर पूर्णाङ्क width = itemp->width();
                    अचर पूर्णाङ्क lsb = itemp->lsb();
                    अचर पूर्णाङ्क msb = lsb + width - 1;
                    V3_सङ्ख्या field_अङ्क{nump, width};
                    field_अङ्क.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    यदि (अचर Ast_गाँठो_DType* अचर childTypep = itemp->subDTypep()) {
                        out << pretty_सङ्ख्या(&field_अङ्क, childTypep);
                    } अन्यथा {
                        out << field_अङ्क;
                    }
                    यदि (itemp->nextp()) out << ", ";
                }
                out << "}";
                फर्काऊ out.str();
            }
        } अन्यथा यदि (अचर AstPackArrayDType* अचर arrayp = VN_CAST(dtypep, PackArrayDType)) {
            यदि (अचर Ast_गाँठो_DType* अचर childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                अचर पूर्णाङ्क arrayElements = arrayp->elementsConst();
                लागि (पूर्णाङ्क element = 0; element < arrayElements; ++element) {
                    अचर पूर्णाङ्क width = childTypep->width();
                    अचर पूर्णाङ्क lsb = width * element;
                    अचर पूर्णाङ्क msb = lsb + width - 1;
                    V3_सङ्ख्या field_अङ्क{nump, width};
                    field_अङ्क.opSel(*nump, msb, lsb);
                    अचर पूर्णाङ्क arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_सङ्ख्या(&field_अङ्क, childTypep);
                    यदि (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                फर्काऊ out.str();
            }
        }
        फर्काऊ nump->ascii();
    }

    // Checking METHODS
सार्वजनिक:
    /// Call other-this function on all new *non-constant* var references
    आभास शून्य varRefCb(AstVarRef* nodep) {}

    शून्य clearOptimizable(Ast_गाँठो* nodep /*null ok*/, अचर string& why) {
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
            std::ostringstream थाक;
            पूर्णाङ्क n = 0;
            लागि (अचर स्वतः& callstack : vlstd::reverse_view(m_call_थाक)) {
                अचर AstFuncRef* अचर funcp = callstack->m_funcp;
                थाक << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                लागि (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    अचर AstVar* अचर portp = conIt->first;
                    Ast_गाँठो_अभिव्यक्ति* अचर pinp = conIt->second->exprp();
                    अचर Ast_गाँठो_DType* अचर dtypep = pinp->dtypep();
                    यदि (AstConst* अचर valp = fetchConstNull(pinp)) {
                        थाक << "\n           " << portp->prettyName() << " = "
                              << pretty_सङ्ख्या(&valp->अङ्क(), dtypep);
                    } अन्यथा यदि (Ast_गाँठो_अभिव्यक्ति* अचर valuep = fetch_मान_Null(pinp)) {
                        थाक << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                यदि (++n > CALL_थाक_MAX) {
                    थाक << "\n           ... stack truncated";
                    रोक;
                }
            }
            m_whyNotOptimizable += थाक.str();
        }
    }
    bool optimizable() अचर { फर्काऊ m_whyNotNodep == nullptr; }
    string whyNotMessage() अचर { फर्काऊ m_whyNotOptimizable; }
    Ast_गाँठो* whyNotNodep() अचर { फर्काऊ m_whyNotNodep; }

    bool isAssignDly() अचर { फर्काऊ m_anyAssignDly; }
    bool isImpure() अचर { फर्काऊ m_isImpure; }
    bool isCoverage() अचर { फर्काऊ m_isCoverage; }
    पूर्णाङ्क instr_गन्ती() अचर { फर्काऊ m_instr_गन्ती; }
    पूर्णाङ्क data_गन्ती() अचर { फर्काऊ m_data_गन्ती; }

    // Simulation METHODS
गुप्त:
    AstConst* allocConst(Ast_गाँठो* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        फर्काऊ m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

सार्वजनिक:
    शून्य new_मान(Ast_गाँठो* nodep, अचर Ast_गाँठो_अभिव्यक्ति* valuep) {
        यदि (अचर AstConst* अचर constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->अङ्क().opAssign(constp->अङ्क());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } अन्यथा यदि (fetch_मान_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_मान(nodep, newTrackedClone(const_cast<Ast_गाँठो_अभिव्यक्ति*>(valuep)));
        }
    }
    शून्य new_Out_मान(Ast_गाँठो* nodep, अचर Ast_गाँठो_अभिव्यक्ति* valuep) {
        यदि (अचर AstConst* अचर constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->अङ्क().opAssign(constp->अङ्क());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } अन्यथा यदि (fetch_Out_मान_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_मान(nodep, newTrackedClone(const_cast<Ast_गाँठो_अभिव्यक्ति*>(valuep)));
        }
    }

गुप्त:
    Ast_गाँठो_अभिव्यक्ति* newTrackedClone(Ast_गाँठो_अभिव्यक्ति* nodep) {
        Ast_गाँठो_अभिव्यक्ति* अचर newp = nodep->clone_रूख(false);
        m_reclaimValuesp.थप_back(newp);
        फर्काऊ newp;
    }
    AstConst* newConst(Ast_गाँठो* nodep) {
        // Set a constant value for this node
        यदि (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* अचर constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            फर्काऊ constp;
        }
        फर्काऊ fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_गाँठो* nodep) {
        // Set a var-output constant value for this node
        यदि (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* अचर constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            फर्काऊ constp;
        }
        फर्काऊ fetchOutConst(nodep);
    }

सार्वजनिक:
    Ast_गाँठो_अभिव्यक्ति* fetch_मान_Null(Ast_गाँठो* nodep) { फर्काऊ m_varAux(nodep).valuep; }

गुप्त:
    Ast_गाँठो_अभिव्यक्ति* fetch_Out_मान_Null(Ast_गाँठो* nodep) { फर्काऊ m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_गाँठो* nodep) { फर्काऊ VN_CAST(fetch_मान_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_गाँठो* nodep) {
        फर्काऊ VN_CAST(fetch_Out_मान_Null(nodep), Const);
    }
    Ast_गाँठो_अभिव्यक्ति* fetch_मान(Ast_गाँठो* nodep) {
        Ast_गाँठो_अभिव्यक्ति* अचर valuep = fetch_मान_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        फर्काऊ valuep;
    }
    AstConst* fetchConst(Ast_गाँठो* nodep) {
        AstConst* अचर constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        फर्काऊ constp;
    }
    AstConst* fetchOutConst(Ast_गाँठो* nodep) {
        AstConst* अचर constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        फर्काऊ constp;
    }

सार्वजनिक:
    V3_सङ्ख्या* fetch_सङ्ख्या_Null(Ast_गाँठो* nodep) {
        AstConst* अचर constp = fetchConstNull(nodep);
        यदि (constp) फर्काऊ &constp->अङ्क();
        फर्काऊ nullptr;
    }
    V3_सङ्ख्या* fetch_Out_सङ्ख्या_Null(Ast_गाँठो* nodep) {
        AstConst* अचर constp = fetchOutConstNull(nodep);
        यदि (constp) फर्काऊ &constp->अङ्क();
        फर्काऊ nullptr;
    }

गुप्त:
    शून्य set_मान(Ast_गाँठो* nodep, Ast_गाँठो_अभिव्यक्ति* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    शून्य set_Out_मान(Ast_गाँठो* nodep, Ast_गाँठो_अभिव्यक्ति* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    शून्य check_गाँठो_Info(Ast_गाँठो* nodep, bool ignorePredict = false) {
        यदि (m_checkOnly) {
            m_instr_गन्ती += nodep->instr_गन्ती();
            m_data_गन्ती += nodep->width();
        }
        यदि (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        यदि (!nodep->isPure()) m_isImpure = true;
    }

    शून्य known_Bad_गाँठो_Type(Ast_गाँठो* nodep) {
        // Call for node types we know we can't handle
        यदि (jumpingOver()) फर्काऊ;
        check_गाँठो_Info(nodep);
        यदि (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    शून्य bad_गाँठो_Type(Ast_गाँठो* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_गाँठो_Info(nodep);
        यदि (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            स्थिर std::set<VNType> s_typePrinted;
            अचर स्वतः pair = s_typePrinted.emplace(nodep->type());
            यदि (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_गाँठो* varOrScope(AstVarRef* nodep) अचर {
        Ast_गाँठो* vscp;
        यदि (m_scoped) {
            vscp = nodep->varScopep();
        } अन्यथा {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        फर्काऊ vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() अचर { फर्काऊ m_jumptargetp; }
    शून्य assign_Out_मान(अचर Ast_गाँठो_Assign* nodep, Ast_गाँठो* vscp, अचर Ast_गाँठो_अभिव्यक्ति* valuep) {
        यदि (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_मान(vscp, valuep);
        } अन्यथा {
            new_मान(vscp, valuep);
            set_Out_मान(vscp, fetch_मान(vscp));
        }
    }

    string toStringRecurse(Ast_गाँठो_अभिव्यक्ति* valuep) {
        // Return string representation of a value, or clearOptimizable
        यदि (अचर AstConst* अचर avaluep = VN_CAST(valuep, Const)) {
            फर्काऊ "'h"s + avaluep->अङ्क().displayed(valuep, "%0x");
        }
        यदि (अचर AstConsArray* अचर avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            अचर AstUnpackArrayDType* अचर dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            लागि (पूर्णाङ्क n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            फर्काऊ result;
        }
        यदि (अचर AstConsStruct* अचर avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            लागि (अचर AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            फर्काऊ result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        फर्काऊ "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_गाँठो_अभिव्यक्ति* newInitValuep(Ast_गाँठो* nodep, Ast_गाँठो_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (AstUnpackArrayDType* अचर arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_गाँठो_अभिव्यक्ति* अचर elementp = newInitValuep(nodep, arrayp->subDTypep());
            फर्काऊ elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* अचर structp = VN_CAST(dtypep, StructDType);
        यदि (structp && !structp->packed()) {
            AstConsStruct* अचर newp = new AstConsStruct{nodep->fileline(), structp};
            लागि (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_गाँठो_अभिव्यक्ति* अचर memberValuep = newInitValuep(nodep, memberp->subDTypep());
                यदि (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_रूख(), newp);
                    फर्काऊ nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            फर्काऊ newp;
        }
        यदि (अचर AstBasicDType* अचर basicp = dtypep->basicp()) {
            AstConst* अचर constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_मान{}, basicp->widthMin(), 0};
            यदि (basicp->isZeroInit()) {
                constp->अङ्क() = V3_सङ्ख्या{nodep, basicp};
            } अन्यथा {
                constp->अङ्क().setAllBitsX();
            }
            फर्काऊ constp;
        }
        फर्काऊ nullptr;
    }
    शून्य initVar(AstVar* nodep) {
        यदि (Ast_गाँठो_अभिव्यक्ति* अचर valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_मान(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_रूख(), valuep);
        }
    }

    // VISITORS
    शून्य visit(AstAlways* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_Sen_रूख* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    शून्य visit(AstVarRef* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_गाँठो* अचर vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        यदि (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), गाँठो_UOr_Struct_DType))
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
                अचर bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                अचर Ast_गाँठो_अभिव्यक्ति* अचर valuep
                    = varIsConst ? fetch_मान_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                यदि (varIsConst && valuep) {
                    यदि (!m_checkOnly && optimizable()) new_मान(vscp, valuep);
                } अन्यथा {
                    यदि (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        यदि (m_checkOnly || !optimizable()) फर्काऊ;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_गाँठो_अभिव्यक्ति* valuep = fetch_मान_Null(vscp);
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
            set_मान(nodep, valuep);
        }
    }
    शून्य visit(AstVarXRef* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (m_scoped) {
            bad_गाँठो_Type(nodep);
            फर्काऊ;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!m_params) {
            bad_गाँठो_Type(nodep);
            फर्काऊ;
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
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialAutomaticStmt* nodep) override {
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStaticStmt* nodep) override {
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstInitialStatic* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!m_params) {
            bad_गाँठो_Type(nodep);
            फर्काऊ;
        }
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाँठो_If* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        UINFO(5, "   IF " << nodep);
        check_गाँठो_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateAndNextConstNull(nodep->condp());
            यदि (optimizable()) {
                यदि (fetchConst(nodep->condp())->अङ्क().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } अन्यथा {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    शून्य visit(AstConst* nodep) override {
        check_गाँठो_Info(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        new_मान(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    शून्य visit(AstConsArray* nodep) override {
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        AstConsArray* अचर valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_मान(nodep->defaultp())->clone_रूख(false) : nullptr};
        लागि (अचर AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_मान(itemp->valuep())->clone_रूख(false));
        }
        m_reclaimValuesp.थप_back(valuep);
        set_मान(nodep, valuep);
    }
    शून्य visit(AstConsStruct* nodep) override {
        यदि (!m_params) {
            bad_गाँठो_Type(nodep);
            फर्काऊ;
        }
        check_गाँठो_Info(nodep);
        लागि (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        AstConsStruct* अचर valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        लागि (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_मान(itemp->valuep())->clone_रूख(false)});
        }
        m_reclaimValuesp.थप_back(valuep);
        set_मान(nodep, valuep);
    }
    शून्य visit(AstCvtUnpackToUnpack* nodep) override {
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_गाँठो_अभिव्यक्ति* अचर cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_मान(nodep->fromp())->clone_रूख(false), nodep->dtypep()};
        Ast_गाँठो_अभिव्यक्ति* अचर valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.थप_back(valuep);
        set_मान(nodep, valuep);
    }
    शून्य visit(AstConsArrayItem* nodep) override {
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        check_गाँठो_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        Ast_गाँठो* अचर valuep = nodep->itemp()->valuep();
        यदि (valuep) {
            iterateAndNextConstNull(valuep);
            यदि (!optimizable()) फर्काऊ;
            new_मान(nodep, fetch_मान(valuep));
        } अन्यथा {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    शून्य visit(Ast_गाँठो_Uniop* nodep) override {
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        nodep->सङ्ख्या_Operate(newConst(nodep)->अङ्क(), fetchConst(nodep->lhsp())->अङ्क());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    शून्य aggregateEqRecurse(V3_सङ्ख्या& resultr, Ast_गाँठो* nodep, bool caseEq, Ast_गाँठो_अभिव्यक्ति* lhsp,
                            Ast_गाँठो_अभिव्यक्ति* rhsp) {
        यदि (अचर AstConst* अचर lconstp = VN_CAST(lhsp, Const)) {
            अचर V3_सङ्ख्या& lnum = lconstp->अङ्क();
            अचर V3_सङ्ख्या& rnum = VN_AS(rhsp, Const)->अङ्क();
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
            अचर V3_सङ्ख्या prev = resultr;
            resultr.opLogAnd(prev, eq);
        } अन्यथा यदि (अचर AstConsArray* अचर lconsp = VN_CAST(lhsp, ConsArray)) {
            अचर AstUnpackArrayDType* अचर dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            लागि (पूर्णाङ्क n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } अन्यथा {
            अचर AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            लागि (अचर AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    शून्य handleAggregateCompare(Ast_गाँठो_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        अचर bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        अचर bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_सङ्ख्या& resultr = newConst(nodep)->अङ्क();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_मान(nodep->lhsp()),
                           fetch_मान(nodep->rhsp()));
        यदि (neq) {
            अचर V3_सङ्ख्या eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    शून्य visit(Ast_गाँठो_Biop* nodep) override {
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        यदि (!VN_IS(fetch_मान(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            फर्काऊ;
        }
        AstConst* अचर valuep = newConst(nodep);
        nodep->सङ्ख्या_Operate(newConst(nodep)->अङ्क(), fetchConst(nodep->lhsp())->अङ्क(),
                             fetchConst(nodep->rhsp())->अङ्क());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        यदि (!m_params && VN_IS(nodep, Sel) && valuep->अङ्क().isAnyX()) {
            अचर V3_सङ्ख्या अङ्क{valuep, valuep->width(), valuep->अङ्क()};
            valuep->अङ्क().opBitsOne(अङ्क);
        }
    }
    शून्य visit(Ast_गाँठो_Triop* nodep) override {
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        nodep->सङ्ख्या_Operate(newConst(nodep)->अङ्क(), fetchConst(nodep->lhsp())->अङ्क(),
                             fetchConst(nodep->rhsp())->अङ्क(), fetchConst(nodep->thsp())->अङ्क());
    }
    शून्य visit(Ast_गाँठो_Quadop* nodep) override {
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        nodep->सङ्ख्या_Operate(newConst(nodep)->अङ्क(), fetchConst(nodep->lhsp())->अङ्क(),
                             fetchConst(nodep->rhsp())->अङ्क(), fetchConst(nodep->thsp())->अङ्क(),
                             fetchConst(nodep->fhsp())->अङ्क());
    }
    शून्य visit(AstLogAnd* nodep) override {
        // Need to short circuit
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) फर्काऊ;
            यदि (fetchConst(nodep->lhsp())->अङ्क().isNeqZero()) {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) फर्काऊ;
                new_मान(nodep, fetch_मान(nodep->rhsp()));
            } अन्यथा {
                new_मान(nodep, fetch_मान(nodep->lhsp()));  // a zero
            }
        }
    }
    शून्य visit(AstLogOr* nodep) override {
        // Need to short circuit
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) फर्काऊ;
            यदि (fetchConst(nodep->lhsp())->अङ्क().isNeqZero()) {
                new_मान(nodep, fetch_मान(nodep->lhsp()));  // a one
            } अन्यथा {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) फर्काऊ;
                new_मान(nodep, fetch_मान(nodep->rhsp()));
            }
        }
    }
    शून्य visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->lhsp());
            यदि (!optimizable()) फर्काऊ;
            यदि (fetchConst(nodep->lhsp())->अङ्क().isEqZero()) {
                अचर AstConst cnst{nodep->fileline(), AstConst::Widthed_मान{}, 1, 1};  // a one
                new_मान(nodep, &cnst);  // a one
            } अन्यथा {
                iterateConst(nodep->rhsp());
                यदि (!optimizable()) फर्काऊ;
                new_मान(nodep, fetch_मान(nodep->rhsp()));
            }
        }
    }
    शून्य visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा {
            iterateConst(nodep->condp());
            यदि (!optimizable()) फर्काऊ;
            यदि (fetchConst(nodep->condp())->अङ्क().isNeqZero()) {
                iterateConst(nodep->thenp());
                यदि (!optimizable()) फर्काऊ;
                new_मान(nodep, fetch_मान(nodep->thenp()));
            } अन्यथा {
                iterateConst(nodep->elsep());
                यदि (!optimizable()) फर्काऊ;
                new_मान(nodep, fetch_मान(nodep->elsep()));
            }
        }
    }

    शून्य handleAssignElement(Ast_गाँठो_Assign* nodep, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* मान_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_गाँठो_अभिव्यक्ति*> selps;
        Ast_गाँठो_अभिव्यक्ति* fromp = lhsp;
        जबसम्म (true) {
            यदि (AstArraySel* अचर selp = VN_CAST(fromp, ArraySel)) {
                check_गाँठो_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.थप_back(selp);
                fromp = selp->fromp();
            } अन्यथा यदि (AstStructSel* अचर selp = VN_CAST(fromp, StructSel)) {
                check_गाँठो_Info(selp);
                selps.थप_back(selp);
                fromp = selp->fromp();
            } अन्यथा {
                रोक;
            }
        }
        AstVarRef* अचर varrefp = VN_CAST(fromp, VarRef);
        यदि (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            फर्काऊ;
        }
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        Ast_गाँठो* अचर vscp = varOrScope(varrefp);
        Ast_गाँठो_अभिव्यक्ति* rootp = fetch_Out_मान_Null(vscp);
        यदि (!rootp) rootp = fetch_मान_Null(vscp);
        यदि (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            फर्काऊ;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_गाँठो_अभिव्यक्ति* itemp = rootp;
        लागि (Ast_गाँठो_अभिव्यक्ति* अचर selp : vlstd::reverse_view(selps)) {
            यदि (अचर AstArraySel* अचर aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* अचर consp = VN_AS(itemp, ConsArray);
                अचर uint32_t index = fetchConst(aselp->bitp())->toUInt();
                अचर पूर्णाङ्क elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                यदि (index >= static_cast<uint32_t>(elements)) {
                    फर्काऊ;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                यदि (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_रूख(false);
                    consp->valuep(index, itemp);
                }
            } अन्यथा {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_गाँठो_अभिव्यक्ति* अचर valuep = newTrackedClone(fetch_मान(मान_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.थप_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_मान(nodep, vscp, rootp);
    }
    शून्य handleAssignSel(Ast_गाँठो_Assign* nodep, AstSel* selp, Ast_गाँठो_अभिव्यक्ति* मान_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_सङ्ख्या lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_गाँठो* अचर vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        यदि (AstConst* अचर vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } अन्यथा यदि (AstConst* अचर vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } अन्यथा {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_मान{},
                                     varrefp->varp()->widthMin(), 0};
            यदि (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->अङ्क().setAllBits0();
            } अन्यथा {
                outconstp->अङ्क().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->अङ्क().opSelInto(fetchConst(मान_Fromp)->अङ्क(), lsb, selp->widthConst());
        assign_Out_मान(nodep, vscp, outconstp);
    }
    शून्य handleAssignSelRecurse(Ast_गाँठो_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_सङ्ख्या& lsbRef, पूर्णाङ्क depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_गाँठो_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        यदि (AstVarRef* अचर varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->अङ्क();
            फर्काऊ;  // And presumably still optimizable()
        }
        यदि (AstSel* अचर subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_सङ्ख्या sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            यदि (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->अङ्क());
            }
        } अन्यथा {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    शून्य handleAssignRecurse(Ast_गाँठो_Assign* nodep, Ast_गाँठो_अभिव्यक्ति* lhsp, Ast_गाँठो_अभिव्यक्ति* मान_Fromp) {
        यदि (!optimizable()) फर्काऊ;
        यदि (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            यदि (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                फर्काऊ;
            }
            handleAssignElement(nodep, lhsp, मान_Fromp);
        } अन्यथा यदि (AstConcat* अचर selp = VN_CAST(lhsp, Concat)) {
            check_गाँठो_Info(selp);
            यदि (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                फर्काऊ;
            }
            // Split value into left and right concat values
            यदि (!m_checkOnly) {
                {
                    AstConst* अचर outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_मान{},
                                       selp->lhsp()->width(), 0};
                    outconstp->अङ्क().opSel(fetchConst(मान_Fromp)->अङ्क(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_मान(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_रूख(), outconstp);
                }
                {
                    AstConst* अचर outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_मान{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->अङ्क().opSel(fetchConst(मान_Fromp)->अङ्क(),
                                           selp->rhsp()->width() - 1, 0);
                    new_मान(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_रूख(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } अन्यथा यदि (AstReplicate* अचर selp = VN_CAST(lhsp, Replicate)) {
            check_गाँठो_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* अचर countp = VN_CAST(selp->countp(), Const);
            यदि (!countp || !countp->अङ्क().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                फर्काऊ;
            }
            handleAssignRecurse(nodep, selp->srcp(), मान_Fromp);
        } अन्यथा यदि (AstSel* अचर selp = VN_CAST(lhsp, Sel)) {
            यदि (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                फर्काऊ;
            }
            handleAssignSel(nodep, selp, मान_Fromp);
        } अन्यथा यदि (VN_IS(lhsp, VarRef)) {
            यदि (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } अन्यथा {
                Ast_गाँठो* अचर vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_मान(nodep, vscp, fetch_मान(मान_Fromp));
            }
        } अन्यथा {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);

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
            फर्काऊ;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    शून्य visit(AstArraySel* nodep) override {
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (अचर AstConsArray* अचर consp = VN_CAST(fetch_मान_Null(nodep->fromp()), ConsArray)) {
            अचर AstConst* अचर indexp = fetchConst(nodep->bitp());
            अचर uint32_t offset = indexp->अङ्क().toUInt();
            Ast_गाँठो_अभिव्यक्ति* अचर itemp = consp->valuep(offset);
            यदि (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } अन्यथा {
                set_मान(nodep, itemp);
            }
        } अन्यथा {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    शून्य visit(AstStructSel* nodep) override {
        यदि (!m_params) {
            bad_गाँठो_Type(nodep);
            फर्काऊ;
        }
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        AstConsStruct* अचर consp = VN_AS(fetch_मान(nodep->fromp()), ConsStruct);
        set_मान(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    शून्य visit(AstSliceSel* nodep) override {
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        // Fetch the base constant array
        यदि (अचर AstConsArray* अचर consp = VN_CAST(fetch_मान_Null(nodep->fromp()), ConsArray)) {
            अचर VNumRange& sliceRange = nodep->declRange();
            अचर uint32_t sliceElements = sliceRange.elements();
            अचर पूर्णाङ्क sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_गाँठो_DType* अचर dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* अचर newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            लागि (uint32_t idx = 0; idx < sliceElements; ++idx) {
                अचर uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_रूख(false));
            }
            // Assign the new constant array and track it for later deletion
            set_मान(nodep, newConsp);
            m_reclaimValuesp.थप_back(newConsp);
        } अन्यथा {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    शून्य visit(AstBegin* nodep) override {
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstCase* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        UINFO(5, "   CASE " << nodep);
        check_गाँठो_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
        } अन्यथा यदि (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            लागि (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                यदि (!itemp->isDefault()) {
                    लागि (Ast_गाँठो* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        यदि (hit) रोक;
                        iterateAndNextConstNull(ep);
                        यदि (optimizable()) {
                            V3_सङ्ख्या match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->अङ्क(), fetchConst(ep)->अङ्क());
                            यदि (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            लागि (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                यदि (hit) रोक;
                यदि (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    शून्य visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        यदि (jumpingOver()) फर्काऊ;
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstComment*) override {}

    शून्य visit(Ast_Stmt_अभिव्यक्ति* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_अभिव्यक्ति_Stmt* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        check_गाँठो_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        यदि (!optimizable()) फर्काऊ;
        iterateAndNextConstNull(nodep->resultp());
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        new_मान(nodep, fetch_मान(nodep->resultp()));
    }

    शून्य visit(AstJumpBlock* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        iterateChildrenConst(nodep);
        यदि (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    शून्य visit(AstJumpGo* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        check_गाँठो_Info(nodep);
        यदि (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        यदि (jumpingOver()) फर्काऊ;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        यदि (!m_params) {
            bad_गाँठो_Type(nodep);
            फर्काऊ;
        }
        check_गाँठो_Info(nodep);
        यदि (m_checkOnly) {
            iterateChildrenConst(nodep);
            फर्काऊ;
        }
        यदि (!optimizable()) फर्काऊ;

        माप_प्रकार iter_गन्ती = 0;
        अचर माप_प्रकार iterLimit = v3Global.opt.unrollLimit();
        जबसम्म (true) {
            यदि (iter_गन्ती > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                रोक;
            }
            ++iter_गन्ती;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            यदि (jumpingOver()) रोक;
        }

        यदि (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    शून्य visit(AstLoopTest* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        check_गाँठो_Info(nodep);
        iterateConst(nodep->condp());
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        यदि (fetchConst(nodep->condp())->अङ्क().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    शून्य visit(AstStop* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_गाँठो_Info(nodep);
    }
    शून्य visit(AstFuncRef* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_गाँठो_Info(nodep);
        यदि (!m_params) {
            bad_गाँठो_Type(nodep);
            फर्काऊ;
        }
        Ast_गाँठो_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        यदि (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        यदि (funcp->recursive()) {
            यदि (m_recurse_गन्ती >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                फर्काऊ;
            }
            ++m_recurse_गन्ती;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_गाँठो*, Ast_गाँठो_अभिव्यक्ति*> old_मानहरू;

        यदि (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_मानहरू](AstVar* varp) {
                यदि (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    यदि (Ast_गाँठो_अभिव्यक्ति* अचर valuep = fetch_मान_Null(varp)) {
                        Ast_गाँठो_अभिव्यक्ति* अचर nvaluep = newTrackedClone(valuep);
                        old_मानहरू.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_मानहरू](Ast_गाँठो_अभिव्यक्ति* exprp) {
                यदि (VN_IS(exprp, Const)) फर्काऊ;  // Speed up as won't change
                यदि (Ast_गाँठो_अभिव्यक्ति* अचर valuep = fetch_मान_Null(exprp)) {
                    Ast_गाँठो_अभिव्यक्ति* अचर nvaluep = newTrackedClone(valuep);
                    old_मानहरू.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_गाँठो_अभिव्यक्ति*>> port_मानहरू;
        लागि (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* अचर portp = it->first;
            Ast_गाँठो* अचर pinp = it->second->exprp();
            यदि (pinp) {  // Else too few arguments in function call - ignore it
                यदि (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    फर्काऊ;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_मानहरू.emplace_back(std::make_pair(portp, newTrackedClone(fetch_मान(pinp))));
            }
        }
        // Apply value to the function
        यदि (!m_checkOnly && optimizable())
            लागि (अचर स्वतः& it : port_मानहरू) {
                यदि (!m_checkOnly && optimizable()) new_मान(it.first, it.second);
            }
        Sim_थाक_गाँठो थाक_गाँठो{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_थाक.थप_back(&थाक_गाँठो);
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
        m_call_थाक.झिक_back();
        अचर Ast_गाँठो_अभिव्यक्ति* returnp = nullptr;
        यदि (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_मान(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        लागि (अचर स्वतः& it : old_मानहरू) {
            यदि (it.second) new_मान(it.first, it.second);
        }
        यदि (returnp) new_मान(nodep, returnp);
        यदि (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_गन्ती > 0, nodep, "recurse underflow");
            --m_recurse_गन्ती;
        }
    }

    शून्य visit(AstVar* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!m_params) {
            bad_गाँठो_Type(nodep);
            फर्काऊ;
        }
    }

    शून्य visit(AstScopeName* /*nodep*/) override {
        यदि (jumpingOver()) फर्काऊ;
        // Ignore
    }

    शून्य visit(AstSFormatArg* nodep) override {
        check_गाँठो_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    शून्य visit(AstSFormatF* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;

        Ast_गाँठो* nextArgp = nodep->exprsp();
        string result;
        अचर string format = nodep->text();
        स्वतः pos = format.cbegin();
        bool inPct = false;
        string width;
        लागि (; pos != format.cend(); ++pos) {
            यदि (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } अन्यथा यदि (!inPct) {  // Normal text
                result += *pos;
            } अन्यथा {  // Format character
                यदि (std::है_अंक(pos[0])) {
                    width += pos[0];
                    जारी;
                }

                inPct = false;

                यदि (V3_सङ्ख्या::displayedFmtHasArg(std::छोटे(pos[0]), false)) {
                    Ast_गाँठो* अचर argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* अचर fargp = VN_CAST(argp, SFormatArg);
                    Ast_गाँठो* अचर subargp = fargp ? fargp->exprp() : argp;
                    AstConst* अचर constp = fetchConstNull(subargp);
                    अचर VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    यदि (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        रोक;
                    }
                    अचर string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->अङ्क().displayedEnum(fargp, pformat)
                                  : constp->अङ्क().displayed(nodep, pformat, formatAttr);
                } अन्यथा {
                    विकल्प (std::छोटे(pos[0])) {
                    पक्ष '%': result += "%"; रोक;
                    पक्ष 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        रोक;
                    स्वभाव: clearOptimizable(nodep, "Unknown $display-like format code."); रोक;
                    }
                }
            }
        }

        AstConst* अचर resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_मान(nodep, resultConstp);
        m_reclaimValuesp.थप_back(resultConstp);
    }

    शून्य visit(AstDisplay* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_गाँठो_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        यदि (m_params) {
            अचर AstConst* अचर textp = fetchConst(nodep->fmtp());
            विकल्प (nodep->displayType()) {
            पक्ष VDisplayType::DT_DISPLAY:  // FALLTHRU
            पक्ष VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); रोक;
            पक्ष VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); रोक;
            पक्ष VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); रोक;
            पक्ष VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); रोक;
            पक्ष VDisplayType::DT_WRITE:  // FALLTHRU
            स्वभाव: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    शून्य visit(AstToStringN* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        यदि (!optimizable()) फर्काऊ;  // Accelerate
        check_गाँठो_Info(nodep);
        iterateChildrenConst(nodep);
        यदि (m_checkOnly || !optimizable()) फर्काऊ;
        अचर std::string result = toStringRecurse(fetch_मान(nodep->lhsp()));
        यदि (!optimizable()) फर्काऊ;
        AstConst* अचर resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_मान(nodep, resultConstp);
        m_reclaimValuesp.थप_back(resultConstp);
    }

    शून्य visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    शून्य visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_गाँठो_Type(nodep);
    }
    शून्य visit(AstGetInitialRandomSeed* nodep) override { known_Bad_गाँठो_Type(nodep); }
    शून्य visit(AstMemberSel* nodep) override { known_Bad_गाँठो_Type(nodep); }
    शून्य visit(AstSampled* nodep) override { known_Bad_गाँठो_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (jumpingOver()) फर्काऊ;
        bad_गाँठो_Type(nodep);
    }

    // MEMBERS - called by constructor
    शून्य setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    शून्य mainGuts(Ast_गाँठो* nodep) {
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
        m_instr_गन्ती = 0;
        m_data_गन्ती = 0;
        m_jumptargetp = nullptr;

        Ast_गाँठो::user1_Clear_रूख();
        m_varAux.clear();
        ++m_constGeneration;
    }
    शून्य mainTableCheck(Ast_गाँठो* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    शून्य mainTableEmulate(Ast_गाँठो* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    शून्य mainParamEmulate(Ast_गाँठो* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_गाँठो*> unusedRootps;
        लागि (Ast_गाँठो* अचर nodep : m_reclaimValuesp) {
            यदि (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        लागि (Ast_गाँठो* अचर nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    }
};

#endif  // Guard
