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

#که_نه_تعریف_شوی VERILATOR_V3SIMULATE_H_
#تعریف_کړه VERILATOR_V3SIMULATE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3AstUserAllocator.h"
#ضمن "V3Const.h"
#ضمن "V3Error.h"
#ضمن "V3Task.h"
#ضمن "V3Width.h"

#ضمن <deque>
#ضمن <set>
#ضمن <sstream>
#ضمن <stack>
#ضمن <string>
#ضمن <unordered_map>
#ضمن <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_ډېری_غوټه final {
عام:
    // MEMBERS
    AstFuncRef* ثابت m_funcp;
    V3TaskConnects* ثابت m_tconnects;
    // CONSTRUCTORS
    Sim_ډېری_غوټه(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ډېری_غوټه() = افتراضي;
};

class SimulateVisitor VL_NOT_FINAL : عام VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

خاص:
    // CONSTANTS
    ساكن constexpr صحيح CALL_ډېری_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ثابت VNUser1_دننه_Use m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    تعداد VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    بنية AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_غوټه_بیان* بهر_Valuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_غوټه_بیان* valuep = nullptr;
    };

    AstUser1Allocator<Ast_غوټه, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    بنية ConstAllocator final {
        اندازه_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        اندازه_t m_بل_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(اندازه_t currentGeneration, Ast_غوټه* nodep) {
            إذا (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_بل_Free = 0;
            }
            UASSERT_OBJ(m_بل_Free <= m_constps.اندازه(), nodep, "Should only allocate at end");
            إذا (m_بل_Free == m_constps.اندازه()) {
                m_constps.ټېل_شا(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ثابت constp = m_constps[m_بل_Free++];
            constp->عدد().nodep(nodep);
            أرجع constp;
        }

        ~ConstAllocator() {
            لكل (AstConst* ثابت constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_غوټه* m_whyNotNodep;  ///< First node not optimizable
    bool m_any_ټاکل_Dly;  ///< True if found a delayed assignment
    bool m_any_ټاکل_Comb;  ///< True if found a non-delayed assignment
    bool m_دننه_Dly_ټاکل;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    صحيح m_instr_شمېر;  ///< Number of nodes
    صحيح m_معلومات_شمېر;  ///< Bytes of data
    صحيح m_recurse_شمېر = 0;  // Now deep in current recursion
    Ast_غوټه* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ثابت Ast_غوټه_DType*, ConstAllocator> m_constps;
    اندازه_t m_constGeneration = 0;
    std::ویکټور<Sim_ډېری_غوټه*> m_call_ډېری;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::ویکټور<Ast_غوټه*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_شمېره(ثابت V3_شمېره* nump, ثابت Ast_غوټه_DType* dtypep) {
        إذا (ثابت AstRefDType* ثابت refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت stp = VN_CAST(dtypep, غوټه_UOr_Struct_DType)) {
            إذا (stp->packed()) {
                std::ostringstream بهر;
                بهر << "'{";
                لكل (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت صحيح پلنوالی = itemp->پلنوالی();
                    ثابت صحيح lsb = itemp->lsb();
                    ثابت صحيح msb = lsb + پلنوالی - 1;
                    V3_شمېره ډګر_عدد{nump, پلنوالی};
                    ډګر_عدد.opSel(*nump, msb, lsb);
                    بهر << itemp->name() << ": ";
                    إذا (ثابت Ast_غوټه_DType* ثابت ماشوم_Typep = itemp->subDTypep()) {
                        بهر << pretty_شمېره(&ډګر_عدد, ماشوم_Typep);
                    } وإلا {
                        بهر << ډګر_عدد;
                    }
                    إذا (itemp->nextp()) بهر << ", ";
                }
                بهر << "}";
                أرجع بهر.str();
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            إذا (ثابت Ast_غوټه_DType* ثابت ماشوم_Typep = arrayp->subDTypep()) {
                std::ostringstream بهر;
                بهر << "[";
                ثابت صحيح arrayElements = arrayp->elementsConst();
                لكل (صحيح element = 0; element < arrayElements; ++element) {
                    ثابت صحيح پلنوالی = ماشوم_Typep->پلنوالی();
                    ثابت صحيح lsb = پلنوالی * element;
                    ثابت صحيح msb = lsb + پلنوالی - 1;
                    V3_شمېره ډګر_عدد{nump, پلنوالی};
                    ډګر_عدد.opSel(*nump, msb, lsb);
                    ثابت صحيح arrayElem = arrayp->lo() + element;
                    بهر << arrayElem << " = " << pretty_شمېره(&ډګر_عدد, ماشوم_Typep);
                    إذا (element < arrayElements - 1) بهر << ", ";
                }
                بهر << "]";
                أرجع بهر.str();
            }
        }
        أرجع nump->ascii();
    }

    // Checking METHODS
عام:
    /// Call other-this function on all new *non-constant* var references
    مجازي فراغ varRefCb(AstVarRef* nodep) {}

    فراغ clearOptimizable(Ast_غوټه* nodep /*null ok*/, ثابت string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        إذا (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            إذا (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_مختاړی("Clear optimizable: " << why);
                إذا (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ډېری;
            صحيح n = 0;
            لكل (ثابت خپلکاری& callstack : vlstd::سرچپه_view(m_call_ډېری)) {
                ثابت AstFuncRef* ثابت funcp = callstack->m_funcp;
                ډېری << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                لكل (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->پای_ټکی(); ++conIt) {
                    ثابت AstVar* ثابت portp = conIt->لومړی;
                    Ast_غوټه_بیان* ثابت pinp = conIt->دوهم->exprp();
                    ثابت Ast_غوټه_DType* ثابت dtypep = pinp->dtypep();
                    إذا (AstConst* ثابت valp = fetchConstNull(pinp)) {
                        ډېری << "\n           " << portp->prettyName() << " = "
                              << pretty_شمېره(&valp->عدد(), dtypep);
                    } وإلا إذا (Ast_غوټه_بیان* ثابت valuep = fetch_ارزښت_Null(pinp)) {
                        ډېری << "\n           " << portp->prettyName() << " = "
                              << منزل_String_Recurse(valuep);
                    }
                }
                إذا (++n > CALL_ډېری_MAX) {
                    ډېری << "\n           ... stack truncated";
                    اكسر;
                }
            }
            m_whyNotOptimizable += ډېری.str();
        }
    }
    bool optimizable() ثابت { أرجع m_whyNotNodep == nullptr; }
    string why_Not_پیغام() ثابت { أرجع m_whyNotOptimizable; }
    Ast_غوټه* whyNotNodep() ثابت { أرجع m_whyNotNodep; }

    bool is_ټاکل_Dly() ثابت { أرجع m_any_ټاکل_Dly; }
    bool isImpure() ثابت { أرجع m_isImpure; }
    bool isCoverage() ثابت { أرجع m_isCoverage; }
    صحيح instr_شمېر() ثابت { أرجع m_instr_شمېر; }
    صحيح معلومات_شمېر() ثابت { أرجع m_معلومات_شمېر; }

    // Simulation METHODS
خاص:
    AstConst* allocConst(Ast_غوټه* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        أرجع m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

عام:
    فراغ new_ارزښت(Ast_غوټه* nodep, ثابت Ast_غوټه_بیان* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->عدد().op_ټاکل(constp->عدد());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } وإلا إذا (fetch_ارزښت_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            سېټ_ارزښت(nodep, newTrackedClone(const_cast<Ast_غوټه_بیان*>(valuep)));
        }
    }
    فراغ new_بهر_ارزښت(Ast_غوټه* nodep, ثابت Ast_غوټه_بیان* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            new_بهر_Const(nodep)->عدد().op_ټاکل(constp->عدد());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } وإلا إذا (fetch_بهر_ارزښت_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            سېټ_بهر_ارزښت(nodep, newTrackedClone(const_cast<Ast_غوټه_بیان*>(valuep)));
        }
    }

خاص:
    Ast_غوټه_بیان* newTrackedClone(Ast_غوټه_بیان* nodep) {
        Ast_غوټه_بیان* ثابت newp = nodep->clone_ونه(false);
        m_reclaimValuesp.ټېل_شا(newp);
        أرجع newp;
    }
    AstConst* newConst(Ast_غوټه* nodep) {
        // Set a constant value for this node
        إذا (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            أرجع constp;
        }
        أرجع fetchConst(nodep);
    }
    AstConst* new_بهر_Const(Ast_غوټه* nodep) {
        // Set a var-output constant value for this node
        إذا (!VN_IS(m_varAux(nodep).بهر_Valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).بهر_Valuep = constp;
            أرجع constp;
        }
        أرجع fetch_بهر_Const(nodep);
    }

عام:
    Ast_غوټه_بیان* fetch_ارزښت_Null(Ast_غوټه* nodep) { أرجع m_varAux(nodep).valuep; }

خاص:
    Ast_غوټه_بیان* fetch_بهر_ارزښت_Null(Ast_غوټه* nodep) { أرجع m_varAux(nodep).بهر_Valuep; }
    AstConst* fetchConstNull(Ast_غوټه* nodep) { أرجع VN_CAST(fetch_ارزښت_Null(nodep), Const); }
    AstConst* fetch_بهر_Const_Null(Ast_غوټه* nodep) {
        أرجع VN_CAST(fetch_بهر_ارزښت_Null(nodep), Const);
    }
    Ast_غوټه_بیان* fetch_ارزښت(Ast_غوټه* nodep) {
        Ast_غوټه_بیان* ثابت valuep = fetch_ارزښت_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        أرجع valuep;
    }
    AstConst* fetchConst(Ast_غوټه* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        أرجع constp;
    }
    AstConst* fetch_بهر_Const(Ast_غوټه* nodep) {
        AstConst* ثابت constp = fetch_بهر_Const_Null(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        أرجع constp;
    }

عام:
    V3_شمېره* fetch_شمېره_Null(Ast_غوټه* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        إذا (constp) أرجع &constp->عدد();
        أرجع nullptr;
    }
    V3_شمېره* fetch_بهر_شمېره_Null(Ast_غوټه* nodep) {
        AstConst* ثابت constp = fetch_بهر_Const_Null(nodep);
        إذا (constp) أرجع &constp->عدد();
        أرجع nullptr;
    }

خاص:
    فراغ سېټ_ارزښت(Ast_غوټه* nodep, Ast_غوټه_بیان* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    فراغ سېټ_بهر_ارزښت(Ast_غوټه* nodep, Ast_غوټه_بیان* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).بهر_Valuep = valuep;
    }

    فراغ check_غوټه_Info(Ast_غوټه* nodep, bool ignorePredict = false) {
        إذا (m_checkOnly) {
            m_instr_شمېر += nodep->instr_شمېر();
            m_معلومات_شمېر += nodep->پلنوالی();
        }
        إذا (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        إذا (!nodep->is_خالص()) m_isImpure = true;
    }

    فراغ معلوم_Bad_غوټه_Type(Ast_غوټه* nodep) {
        // Call for node types we know we can't handle
        إذا (jumpingOver()) أرجع;
        check_غوټه_Info(nodep);
        إذا (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    فراغ bad_غوټه_Type(Ast_غوټه* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_غوټه_Info(nodep);
        إذا (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#که_تعریف_شوی VL_DEBUG
            ساكن std::سېټ<VNType> s_typePrinted;
            ثابت خپلکاری pair = s_typePrinted.emplace(nodep->type());
            إذا (pair.دوهم)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#د_که_پای
        }
    }

    Ast_غوټه* varOrScope(AstVarRef* nodep) ثابت {
        Ast_غوټه* vscp;
        إذا (m_scoped) {
            vscp = nodep->varScopep();
        } وإلا {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        أرجع vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ثابت { أرجع m_jumptargetp; }
    فراغ ټاکل_بهر_ارزښت(ثابت Ast_غوټه_ټاکل* nodep, Ast_غوټه* vscp, ثابت Ast_غوټه_بیان* valuep) {
        إذا (VN_IS(nodep, ټاکل_Dly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_بهر_ارزښت(vscp, valuep);
        } وإلا {
            new_ارزښت(vscp, valuep);
            سېټ_بهر_ارزښت(vscp, fetch_ارزښت(vscp));
        }
    }

    string منزل_String_Recurse(Ast_غوټه_بیان* valuep) {
        // Return string representation of a value, or clearOptimizable
        إذا (ثابت AstConst* ثابت avaluep = VN_CAST(valuep, Const)) {
            أرجع "'h"s + avaluep->عدد().displayed(valuep, "%0x");
        }
        إذا (ثابت AstConsArray* ثابت avaluep = VN_CAST(valuep, ConsArray)) {
            string پایله = "'{";
            string comma;
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            لكل (صحيح n = 0; n < dtypep->elementsConst(); ++n) {
                پایله += comma + منزل_String_Recurse(avaluep->valuep(n));
                comma = ", ";
            }
            پایله += "}";
            أرجع پایله;
        }
        إذا (ثابت AstConsStruct* ثابت avaluep = VN_CAST(valuep, ConsStruct)) {
            string پایله = "'{";
            string comma;
            لكل (ثابت Ast_Cons_Struct_توکی* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_توکی)) {
                پایله
                    += comma + itemp->memberp()->name() + ":" + منزل_String_Recurse(itemp->valuep());
                comma = ", ";
            }
            پایله += "}";
            أرجع پایله;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        أرجع "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_غوټه_بیان* newInitValuep(Ast_غوټه* nodep, Ast_غوټه_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_غوټه_بیان* ثابت elementp = newInitValuep(nodep, arrayp->subDTypep());
            أرجع elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        إذا (structp && !structp->packed()) {
            AstConsStruct* ثابت newp = new AstConsStruct{nodep->fileline(), structp};
            لكل (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_غوټه_بیان* ثابت memberValuep = newInitValuep(nodep, memberp->subDTypep());
                إذا (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_ونه(), newp);
                    أرجع nullptr;
                }
                newp->جمع_کړه_Itemsp(new Ast_Cons_Struct_توکی{nodep->fileline(), memberp, memberValuep});
            }
            أرجع newp;
        }
        إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
            AstConst* ثابت constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_ارزښت{}, basicp->پلنوالی_Min(), 0};
            إذا (basicp->isZeroInit()) {
                constp->عدد() = V3_شمېره{nodep, basicp};
            } وإلا {
                constp->عدد().سېټ_All_بټونه_X();
            }
            أرجع constp;
        }
        أرجع nullptr;
    }
    فراغ initVar(AstVar* nodep) {
        إذا (Ast_غوټه_بیان* ثابت valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_ارزښت(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_ونه(), valuep);
        }
    }

    // VISITORS
    فراغ لیدنه(AstAlways* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_Sen_ونه* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    فراغ لیدنه(AstVarRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_غوټه* ثابت vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        إذا (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), غوټه_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        إذا (nodep->access().isWriteOrRW()) {
            إذا (m_دننه_Dly_ټاکل) {
                إذا (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    إذا (m_checkOnly) varRefCb(nodep);
                }
            } وإلا {  // nondly asn
                إذا (!(m_varAux(vscp).usage & VU_LV)) {
                    إذا (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    إذا (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        إذا (nodep->access().isReadOrRW()) {
            إذا (!(m_varAux(vscp).usage & VU_RV)) {
                إذا (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                ثابت bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ثابت Ast_غوټه_بیان* ثابت valuep
                    = varIsConst ? fetch_ارزښت_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                إذا (varIsConst && valuep) {
                    إذا (!m_checkOnly && optimizable()) new_ارزښت(vscp, valuep);
                } وإلا {
                    إذا (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        إذا (m_checkOnly || !optimizable()) أرجع;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_غوټه_بیان* valuep = fetch_ارزښت_Null(vscp);
            إذا (!valuep) {
                إذا (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } وإلا {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            سېټ_ارزښت(nodep, valuep);
        }
    }
    فراغ لیدنه(AstVarXRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (m_scoped) {
            bad_غوټه_Type(nodep);
            أرجع;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_غوټه_Type(nodep);
            أرجع;
        }
        إذا (nodep->dpiImport()) {
            إذا (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        إذا (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstInitialAutomaticStmt* nodep) override {
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstInitialStaticStmt* nodep) override {
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstInitialStatic* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_غوټه_Type(nodep);
            أرجع;
        }
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   IF " << nodep);
        check_غوټه_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterate_And_بل_Const_Null(nodep->condp());
            إذا (optimizable()) {
                إذا (fetchConst(nodep->condp())->عدد().isNeqZero()) {
                    iterate_And_بل_Const_Null(nodep->thensp());
                } وإلا {
                    iterate_And_بل_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    فراغ لیدنه(AstConst* nodep) override {
        check_غوټه_Info(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        new_ارزښت(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    فراغ لیدنه(AstConsArray* nodep) override {
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsArray* ثابت valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_ارزښت(nodep->defaultp())->clone_ونه(false) : nullptr};
        لكل (ثابت Ast_Cons_Array_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_توکی)) {
            valuep->valuep(itemp->شاخص(), fetch_ارزښت(itemp->valuep())->clone_ونه(false));
        }
        m_reclaimValuesp.ټېل_شا(valuep);
        سېټ_ارزښت(nodep, valuep);
    }
    فراغ لیدنه(AstConsStruct* nodep) override {
        إذا (!m_params) {
            bad_غوټه_Type(nodep);
            أرجع;
        }
        check_غوټه_Info(nodep);
        لكل (Ast_Cons_Struct_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_توکی)) {
            iterateConst(itemp->valuep());
        }
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsStruct* ثابت valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        لكل (Ast_Cons_Struct_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_توکی)) {
            valuep->جمع_کړه_Itemsp(
                new Ast_Cons_Struct_توکی{itemp->fileline(), itemp->memberp(),
                                      fetch_ارزښت(itemp->valuep())->clone_ونه(false)});
        }
        m_reclaimValuesp.ټېل_شا(valuep);
        سېټ_ارزښت(nodep, valuep);
    }
    فراغ لیدنه(Ast_Cvt_Unpack_منزل_Unpack* nodep) override {
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_غوټه_بیان* ثابت cvtp = new Ast_Cvt_Unpack_منزل_Unpack{
            nodep->fileline(), fetch_ارزښت(nodep->fromp())->clone_ونه(false), nodep->dtypep()};
        Ast_غوټه_بیان* ثابت valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ټېل_شا(valuep);
        سېټ_ارزښت(nodep, valuep);
    }
    فراغ لیدنه(Ast_Cons_Array_توکی* nodep) override {
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_Enum_توکی_Ref* nodep) override {
        check_غوټه_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        إذا (m_checkOnly || !optimizable()) أرجع;
        Ast_غوټه* ثابت valuep = nodep->itemp()->valuep();
        إذا (valuep) {
            iterate_And_بل_Const_Null(valuep);
            إذا (!optimizable()) أرجع;
            new_ارزښت(nodep, fetch_ارزښت(valuep));
        } وإلا {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    فراغ لیدنه(Ast_غوټه_Uniop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->شمېره_عمل_کړه(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    فراغ aggregateEqRecurse(V3_شمېره& resultr, Ast_غوټه* nodep, bool caseEq, Ast_غوټه_بیان* lhsp,
                            Ast_غوټه_بیان* rhsp) {
        إذا (ثابت AstConst* ثابت lconstp = VN_CAST(lhsp, Const)) {
            ثابت V3_شمېره& lnum = lconstp->عدد();
            ثابت V3_شمېره& rnum = VN_AS(rhsp, Const)->عدد();
            V3_شمېره eq{nodep, 1};
            إذا (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } وإلا إذا (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } وإلا إذا (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } وإلا {
                eq.opEq(lnum, rnum);
            }
            ثابت V3_شمېره prev = resultr;
            resultr.opLogAnd(prev, eq);
        } وإلا إذا (ثابت AstConsArray* ثابت lconsp = VN_CAST(lhsp, ConsArray)) {
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            لكل (صحيح n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } وإلا {
            ثابت Ast_Cons_Struct_توکی* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            لكل (ثابت Ast_Cons_Struct_توکی* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), Cons_Struct_توکی),
                                          ritemp = VN_AS(ritemp->nextp(), Cons_Struct_توکی)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    فراغ handleAggregateCompare(Ast_غوټه_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ثابت bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ثابت bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_شمېره& resultr = newConst(nodep)->عدد();
        resultr.سېټ_All_Bits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_ارزښت(nodep->lhsp()),
                           fetch_ارزښت(nodep->rhsp()));
        إذا (neq) {
            ثابت V3_شمېره eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    فراغ لیدنه(Ast_غوټه_Biop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        إذا (!VN_IS(fetch_ارزښت(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            أرجع;
        }
        AstConst* ثابت valuep = newConst(nodep);
        nodep->شمېره_عمل_کړه(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        إذا (!m_params && VN_IS(nodep, Sel) && valuep->عدد().isAnyX()) {
            ثابت V3_شمېره عدد{valuep, valuep->پلنوالی(), valuep->عدد()};
            valuep->عدد().op_بټونه_One(عدد);
        }
    }
    فراغ لیدنه(Ast_غوټه_Triop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->شمېره_عمل_کړه(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد(), fetchConst(nodep->thsp())->عدد());
    }
    فراغ لیدنه(Ast_غوټه_Quadop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->شمېره_عمل_کړه(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد(), fetchConst(nodep->thsp())->عدد(),
                             fetchConst(nodep->fhsp())->عدد());
    }
    فراغ لیدنه(AstLogAnd* nodep) override {
        // Need to short circuit
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->عدد().isNeqZero()) {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_ارزښت(nodep, fetch_ارزښت(nodep->rhsp()));
            } وإلا {
                new_ارزښت(nodep, fetch_ارزښت(nodep->lhsp()));  // a zero
            }
        }
    }
    فراغ لیدنه(AstLogOr* nodep) override {
        // Need to short circuit
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->عدد().isNeqZero()) {
                new_ارزښت(nodep, fetch_ارزښت(nodep->lhsp()));  // a one
            } وإلا {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_ارزښت(nodep, fetch_ارزښت(nodep->rhsp()));
            }
        }
    }
    فراغ لیدنه(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->عدد().isEqZero()) {
                ثابت AstConst cnst{nodep->fileline(), AstConst::Widthed_ارزښت{}, 1, 1};  // a one
                new_ارزښت(nodep, &cnst);  // a one
            } وإلا {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_ارزښت(nodep, fetch_ارزښت(nodep->rhsp()));
            }
        }
    }
    فراغ لیدنه(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->condp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->condp())->عدد().isNeqZero()) {
                iterateConst(nodep->thenp());
                إذا (!optimizable()) أرجع;
                new_ارزښت(nodep, fetch_ارزښت(nodep->thenp()));
            } وإلا {
                iterateConst(nodep->elsep());
                إذا (!optimizable()) أرجع;
                new_ارزښت(nodep, fetch_ارزښت(nodep->elsep()));
            }
        }
    }

    فراغ handle_ټاکل_Element(Ast_غوټه_ټاکل* nodep, Ast_غوټه_بیان* lhsp, Ast_غوټه_بیان* ارزښت_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::ویکټور<Ast_غوټه_بیان*> selps;
        Ast_غوټه_بیان* fromp = lhsp;
        طالما (true) {
            إذا (AstArraySel* ثابت selp = VN_CAST(fromp, ArraySel)) {
                check_غوټه_Info(selp);
                iterate_And_بل_Const_Null(selp->bitp());  // Bit index
                selps.ټېل_شا(selp);
                fromp = selp->fromp();
            } وإلا إذا (AstStructSel* ثابت selp = VN_CAST(fromp, StructSel)) {
                check_غوټه_Info(selp);
                selps.ټېل_شا(selp);
                fromp = selp->fromp();
            } وإلا {
                اكسر;
            }
        }
        AstVarRef* ثابت varrefp = VN_CAST(fromp, VarRef);
        إذا (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            أرجع;
        }
        إذا (m_checkOnly || !optimizable()) أرجع;
        Ast_غوټه* ثابت vscp = varOrScope(varrefp);
        Ast_غوټه_بیان* rootp = fetch_بهر_ارزښت_Null(vscp);
        إذا (!rootp) rootp = fetch_ارزښت_Null(vscp);
        إذا (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            أرجع;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_غوټه_بیان* itemp = rootp;
        لكل (Ast_غوټه_بیان* ثابت selp : vlstd::سرچپه_view(selps)) {
            إذا (ثابت AstArraySel* ثابت aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ثابت consp = VN_AS(itemp, ConsArray);
                ثابت uint32_t شاخص = fetchConst(aselp->bitp())->منزل_UInt();
                ثابت صحيح elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                إذا (شاخص >= static_cast<uint32_t>(elements)) {
                    أرجع;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(شاخص);
                إذا (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_ونه(false);
                    consp->valuep(شاخص, itemp);
                }
            } وإلا {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_غوټه_بیان* ثابت valuep = newTrackedClone(fetch_ارزښت(ارزښت_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ټېل_شا(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        ټاکل_بهر_ارزښت(nodep, vscp, rootp);
    }
    فراغ handle_ټاکل_Sel(Ast_غوټه_ټاکل* nodep, AstSel* selp, Ast_غوټه_بیان* ارزښت_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_شمېره lsb{nodep};
        handle_ټاکل_Sel_Recurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        إذا (m_checkOnly || !optimizable()) أرجع;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_غوټه* ثابت vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        إذا (AstConst* ثابت vscpnump = fetch_بهر_Const_Null(vscp)) {
            outconstp = vscpnump;
        } وإلا إذا (AstConst* ثابت vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } وإلا {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_ارزښت{},
                                     varrefp->varp()->پلنوالی_Min(), 0};
            إذا (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->عدد().سېټ_All_Bits0();
            } وإلا {
                outconstp->عدد().سېټ_All_بټونه_X();
            }
            m_reclaimValuesp.emplace_شا(outconstp);
        }
        outconstp->عدد().opSelInto(fetchConst(ارزښت_Fromp)->عدد(), lsb, selp->پلنوالی_Const());
        ټاکل_بهر_ارزښت(nodep, vscp, outconstp);
    }
    فراغ handle_ټاکل_Sel_Recurse(Ast_غوټه_ټاکل* nodep, AstSel* selp, AstVarRef*& بهر_Varrefp_Ref,
                                V3_شمېره& lsbRef, صحيح ژوروالی) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_غوټه_Info(selp);
        iterate_And_بل_Const_Null(selp->lsbp());  // Bit index
        إذا (AstVarRef* ثابت varrefp = VN_CAST(selp->fromp(), VarRef)) {
            بهر_Varrefp_Ref = varrefp;
            lsbRef = fetchConst(selp->lsbp())->عدد();
            أرجع;  // And presumably still optimizable()
        }
        إذا (AstSel* ثابت subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_شمېره sublsb{nodep};
            handle_ټاکل_Sel_Recurse(nodep, subselp, بهر_Varrefp_Ref, sublsb /*ref*/, ژوروالی + 1);
            إذا (optimizable()) {
                lsbRef = sublsb;
                lsbRef.op_جمع_کړه(sublsb, fetchConst(selp->lsbp())->عدد());
            }
        } وإلا {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    فراغ handle_ټاکل_Recurse(Ast_غوټه_ټاکل* nodep, Ast_غوټه_بیان* lhsp, Ast_غوټه_بیان* ارزښت_Fromp) {
        إذا (!optimizable()) أرجع;
        إذا (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            إذا (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                أرجع;
            }
            handle_ټاکل_Element(nodep, lhsp, ارزښت_Fromp);
        } وإلا إذا (AstConcat* ثابت selp = VN_CAST(lhsp, Concat)) {
            check_غوټه_Info(selp);
            إذا (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                أرجع;
            }
            // Split value into left and right concat values
            إذا (!m_checkOnly) {
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_ارزښت{},
                                       selp->lhsp()->پلنوالی(), 0};
                    outconstp->عدد().opSel(fetchConst(ارزښت_Fromp)->عدد(),
                                           selp->lhsp()->پلنوالی() + selp->rhsp()->پلنوالی() + 1,
                                           selp->rhsp()->پلنوالی());
                    new_ارزښت(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ونه(), outconstp);
                }
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_ارزښت{},
                                       selp->rhsp()->پلنوالی_Min(), 0};
                    outconstp->عدد().opSel(fetchConst(ارزښت_Fromp)->عدد(),
                                           selp->rhsp()->پلنوالی() - 1, 0);
                    new_ارزښت(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_ونه(), outconstp);
                }
            }
            handle_ټاکل_Recurse(nodep, selp->lhsp(), selp->lhsp());
            handle_ټاکل_Recurse(nodep, selp->rhsp(), selp->rhsp());
        } وإلا إذا (AstReplicate* ثابت selp = VN_CAST(lhsp, Replicate)) {
            check_غوټه_Info(selp);
            iterate_And_بل_Const_Null(selp->countp());
            AstConst* ثابت countp = VN_CAST(selp->countp(), Const);
            إذا (!countp || !countp->عدد().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                أرجع;
            }
            handle_ټاکل_Recurse(nodep, selp->srcp(), ارزښت_Fromp);
        } وإلا إذا (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            إذا (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                أرجع;
            }
            handle_ټاکل_Sel(nodep, selp, ارزښت_Fromp);
        } وإلا إذا (VN_IS(lhsp, VarRef)) {
            إذا (m_checkOnly) {
                iterate_And_بل_Const_Null(lhsp);
            } وإلا {
                Ast_غوټه* ثابت vscp = varOrScope(VN_CAST(lhsp, VarRef));
                ټاکل_بهر_ارزښت(nodep, vscp, fetch_ارزښت(ارزښت_Fromp));
            }
        } وإلا {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);

        VL_RESTORER(m_دننه_Dly_ټاکل);

        إذا (VN_IS(nodep, ټاکل_Force)) {
            clearOptimizable(nodep, "Force");
        } وإلا إذا (VN_IS(nodep, ټاکل_Dly)) {
            إذا (m_any_ټاکل_Comb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_ټاکل_Dly = true;
            m_دننه_Dly_ټاکل = true;
        } وإلا {
            إذا (m_any_ټاکل_Dly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_ټاکل_Comb = true;
        }

        إذا (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            أرجع;
        }

        iterate_And_بل_Const_Null(nodep->rhsp());  // Value to assign
        handle_ټاکل_Recurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    فراغ لیدنه(AstArraySel* nodep) override {
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_ارزښت_Null(nodep->fromp()), ConsArray)) {
            ثابت AstConst* ثابت indexp = fetchConst(nodep->bitp());
            ثابت uint32_t دننه_واټن = indexp->عدد().منزل_UInt();
            Ast_غوټه_بیان* ثابت itemp = consp->valuep(دننه_واټن);
            إذا (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::منزل_string(دننه_واټن));
            } وإلا {
                سېټ_ارزښت(nodep, itemp);
            }
        } وإلا {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    فراغ لیدنه(AstStructSel* nodep) override {
        إذا (!m_params) {
            bad_غوټه_Type(nodep);
            أرجع;
        }
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsStruct* ثابت consp = VN_AS(fetch_ارزښت(nodep->fromp()), ConsStruct);
        سېټ_ارزښت(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    فراغ لیدنه(Ast_ټوټه_Sel* nodep) override {
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // Fetch the base constant array
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_ارزښت_Null(nodep->fromp()), ConsArray)) {
            ثابت VNumRange& ټوټه_Range = nodep->declRange();
            ثابت uint32_t ټوټه_Elements = ټوټه_Range.elements();
            ثابت صحيح ټوټه_Lo = ټوټه_Range.lo();
            // Use this node's dtype for the slice array
            Ast_غوټه_DType* ثابت dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ثابت newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            لكل (uint32_t idx = 0; idx < ټوټه_Elements; ++idx) {
                ثابت uint32_t بنسټ_Idx = ټوټه_Lo + idx;
                newConsp->valuep(idx, consp->valuep(بنسټ_Idx)->clone_ونه(false));
            }
            // Assign the new constant array and track it for later deletion
            سېټ_ارزښت(nodep, newConsp);
            m_reclaimValuesp.ټېل_شا(newConsp);
        } وإلا {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    فراغ لیدنه(AstBegin* nodep) override {
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstCase* nodep) override {
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   CASE " << nodep);
        check_غوټه_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا إذا (optimizable()) {
            iterate_And_بل_Const_Null(nodep->exprp());
            bool موندنه = false;
            لكل (Ast_Case_توکی* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_توکی)) {
                إذا (!itemp->isDefault()) {
                    لكل (Ast_غوټه* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        إذا (موندنه) اكسر;
                        iterate_And_بل_Const_Null(ep);
                        إذا (optimizable()) {
                            V3_شمېره سمون{nodep, 1};
                            سمون.opEq(fetchConst(nodep->exprp())->عدد(), fetchConst(ep)->عدد());
                            إذا (سمون.isNeqZero()) {
                                iterate_And_بل_Const_Null(itemp->stmtsp());
                                موندنه = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            لكل (Ast_Case_توکی* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_توکی)) {
                إذا (موندنه) اكسر;
                إذا (itemp->isDefault()) {
                    iterate_And_بل_Const_Null(itemp->stmtsp());
                    موندنه = true;
                }
            }
        }
    }

    فراغ لیدنه(Ast_Case_توکی* nodep) override {
        // Real handling is in AstNodeCase
        إذا (jumpingOver()) أرجع;
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }

    فراغ لیدنه(AstComment*) override {}

    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_بیان_Stmt* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_غوټه_Info(nodep);
        iterate_And_بل_Const_Null(nodep->stmtsp());
        إذا (!optimizable()) أرجع;
        iterate_And_بل_Const_Null(nodep->resultp());
        إذا (m_checkOnly || !optimizable()) أرجع;
        new_ارزښت(nodep, fetch_ارزښت(nodep->resultp()));
    }

    فراغ لیدنه(Ast_Jump_بلاک* nodep) override {
        إذا (jumpingOver()) أرجع;
        iterateChildrenConst(nodep);
        إذا (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    فراغ لیدنه(AstJumpGo* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_غوټه_Info(nodep);
        إذا (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    فراغ لیدنه(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        إذا (!m_params) {
            bad_غوټه_Type(nodep);
            أرجع;
        }
        check_غوټه_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
            أرجع;
        }
        إذا (!optimizable()) أرجع;

        اندازه_t iter_شمېر = 0;
        ثابت اندازه_t iter_حد = v3Global.opt.unroll_حد();
        طالما (true) {
            إذا (iter_شمېر > iter_حد) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::منزل_string(iter_حد));
                اكسر;
            }
            ++iter_شمېر;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_بل_Const_Null(nodep->stmtsp());
            إذا (jumpingOver()) اكسر;
        }

        إذا (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    فراغ لیدنه(AstLoopTest* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_غوټه_Info(nodep);
        iterateConst(nodep->condp());
        إذا (m_checkOnly || !optimizable()) أرجع;
        إذا (fetchConst(nodep->condp())->عدد().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    فراغ لیدنه(AstStop* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_غوټه_Info(nodep);
    }
    فراغ لیدنه(AstFuncRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_غوټه_Info(nodep);
        إذا (!m_params) {
            bad_غوټه_Type(nodep);
            أرجع;
        }
        Ast_غوټه_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        إذا (m_params) V3_پلنوالی::پلنوالی_Params_Edit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        إذا (funcp->recursive()) {
            إذا (m_recurse_شمېر >= v3Global.opt.func_Recursion_ژوروالی()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::منزل_string(v3Global.opt.func_Recursion_ژوروالی())
                                            + " times");
                أرجع;
            }
            ++m_recurse_شمېر;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_غوټه*, Ast_غوټه_بیان*> old_ارزښتونه;

        إذا (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_ارزښتونه](AstVar* varp) {
                إذا (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    إذا (Ast_غوټه_بیان* ثابت valuep = fetch_ارزښت_Null(varp)) {
                        Ast_غوټه_بیان* ثابت nvaluep = newTrackedClone(valuep);
                        old_ارزښتونه.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_ارزښتونه](Ast_غوټه_بیان* exprp) {
                إذا (VN_IS(exprp, Const)) أرجع;  // Speed up as won't change
                إذا (Ast_غوټه_بیان* ثابت valuep = fetch_ارزښت_Null(exprp)) {
                    Ast_غوټه_بیان* ثابت nvaluep = newTrackedClone(valuep);
                    old_ارزښتونه.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::ویکټور<std::pair<AstVar*, Ast_غوټه_بیان*>> پورټ_ارزښتونه;
        لكل (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.پای_ټکی(); ++it) {
            AstVar* ثابت portp = it->لومړی;
            Ast_غوټه* ثابت pinp = it->دوهم->exprp();
            إذا (pinp) {  // Else too few arguments in function call - ignore it
                إذا (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    أرجع;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                پورټ_ارزښتونه.emplace_شا(std::make_pair(portp, newTrackedClone(fetch_ارزښت(pinp))));
            }
        }
        // Apply value to the function
        إذا (!m_checkOnly && optimizable())
            لكل (ثابت خپلکاری& it : پورټ_ارزښتونه) {
                إذا (!m_checkOnly && optimizable()) new_ارزښت(it.لومړی, it.دوهم);
            }
        Sim_ډېری_غوټه ډېری_غوټه{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ډېری.ټېل_شا(&ډېری_غوټه);
        إذا (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                إذا (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_ډېری.راوباسه_شا();
        ثابت Ast_غوټه_بیان* returnp = nullptr;
        إذا (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_ارزښت(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        لكل (ثابت خپلکاری& it : old_ارزښتونه) {
            إذا (it.دوهم) new_ارزښت(it.لومړی, it.دوهم);
        }
        إذا (returnp) new_ارزښت(nodep, returnp);
        إذا (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_شمېر > 0, nodep, "recurse underflow");
            --m_recurse_شمېر;
        }
    }

    فراغ لیدنه(AstVar* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_غوټه_Type(nodep);
            أرجع;
        }
    }

    فراغ لیدنه(AstScopeName* /*nodep*/) override {
        إذا (jumpingOver()) أرجع;
        // Ignore
    }

    فراغ لیدنه(AstSFormatArg* nodep) override {
        check_غوټه_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_بل_Const_Null(nodep->exprp());
    }
    فراغ لیدنه(AstSFormatF* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;

        Ast_غوټه* بل_Argp = nodep->exprsp();
        string پایله;
        ثابت string format = nodep->متن();
        خپلکاری pos = format.cbegin();
        bool دننه_Pct = false;
        string پلنوالی;
        لكل (; pos != format.cend(); ++pos) {
            إذا (!دننه_Pct && pos[0] == '%') {
                دننه_Pct = true;
                پلنوالی = "";
            } وإلا إذا (!دننه_Pct) {  // Normal text
                پایله += *pos;
            } وإلا {  // Format character
                إذا (std::که_عدد(pos[0])) {
                    پلنوالی += pos[0];
                    استمر;
                }

                دننه_Pct = false;

                إذا (V3_شمېره::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_غوټه* ثابت argp = بل_Argp;
                    بل_Argp = بل_Argp->nextp();
                    AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                    Ast_غوټه* ثابت subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ثابت constp = fetchConstNull(subargp);
                    ثابت VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    إذا (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        اكسر;
                    }
                    ثابت string pformat = "%"s + پلنوالی + pos[0];
                    پایله += formatAttr.isEnum()
                                  ? constp->عدد().displayedEnum(fargp, pformat)
                                  : constp->عدد().displayed(nodep, pformat, formatAttr);
                } وإلا {
                    بدل (std::tolower(pos[0])) {
                    حالة '%': پایله += "%"; اكسر;
                    حالة 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        پایله += "%m";
                        اكسر;
                    افتراضي: clearOptimizable(nodep, "Unknown $display-like format code."); اكسر;
                    }
                }
            }
        }

        AstConst* ثابت پایله_Constp = new AstConst{nodep->fileline(), AstConst::String{}, پایله};
        سېټ_ارزښت(nodep, پایله_Constp);
        m_reclaimValuesp.ټېل_شا(پایله_Constp);
    }

    فراغ لیدنه(AstDisplay* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_غوټه_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        إذا (m_params) {
            ثابت AstConst* ثابت textp = fetchConst(nodep->fmtp());
            بدل (nodep->displayType()) {
            حالة VDisplayType::DT_DISPLAY:  // FALLTHRU
            حالة VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); اكسر;
            حالة VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); اكسر;
            حالة VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); اكسر;
            حالة VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); اكسر;
            حالة VDisplayType::DT_WRITE:  // FALLTHRU
            افتراضي: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    فراغ لیدنه(Ast_منزل_String_N* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_غوټه_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        ثابت std::string پایله = منزل_String_Recurse(fetch_ارزښت(nodep->lhsp()));
        إذا (!optimizable()) أرجع;
        AstConst* ثابت پایله_Constp = new AstConst{nodep->fileline(), AstConst::String{}, پایله};
        سېټ_ارزښت(nodep, پایله_Constp);
        m_reclaimValuesp.ټېل_شا(پایله_Constp);
    }

    فراغ لیدنه(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    فراغ لیدنه(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        معلوم_Bad_غوټه_Type(nodep);
    }
    فراغ لیدنه(Ast_Get_Initial_Random_تخم* nodep) override { معلوم_Bad_غوټه_Type(nodep); }
    فراغ لیدنه(AstMemberSel* nodep) override { معلوم_Bad_غوټه_Type(nodep); }
    فراغ لیدنه(AstSampled* nodep) override { معلوم_Bad_غوټه_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    فراغ لیدنه(Ast_غوټه* nodep) override {
        إذا (jumpingOver()) أرجع;
        bad_غوټه_Type(nodep);
    }

    // MEMBERS - called by constructor
    فراغ سېټ_ډېرپېښ(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    فراغ mainGuts(Ast_غوټه* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

عام:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        سېټ_ډېرپېښ(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    فراغ clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_any_ټاکل_Comb = false;
        m_any_ټاکل_Dly = false;
        m_دننه_Dly_ټاکل = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_شمېر = 0;
        m_معلومات_شمېر = 0;
        m_jumptargetp = nullptr;

        Ast_غوټه::user1_Clear_ونه();
        m_varAux.clear();
        ++m_constGeneration;
    }
    فراغ main_جدول_Check(Ast_غوټه* nodep) {
        سېټ_ډېرپېښ(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    فراغ main_جدول_Emulate(Ast_غوټه* nodep) {
        سېټ_ډېرپېښ(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    فراغ mainParamEmulate(Ast_غوټه* nodep) {
        سېټ_ډېرپېښ(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::سېټ<Ast_غوټه*> ناکارول_شوی_Rootps;
        لكل (Ast_غوټه* ثابت nodep : m_reclaimValuesp) {
            إذا (!nodep->backp()) ناکارول_شوی_Rootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        لكل (Ast_غوټه* ثابت nodep : ناکارول_شوی_Rootps) VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
};

#د_که_پای  // Guard
