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

#اگر_تعریف_نشده VERILATOR_V3SIMULATE_H_
#تعریف_کن VERILATOR_V3SIMULATE_H_

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

class Sim_پشته_گره final {
عام:
    // MEMBERS
    AstFuncRef* ثابت m_funcp;
    V3TaskConnects* ثابت m_tconnects;
    // CONSTRUCTORS
    Sim_پشته_گره(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_پشته_گره() = افتراضي;
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
    ساكن constexpr صحيح CALL_پشته_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ثابت VNUser1_درون_Use m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    تعداد VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    بنية AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_گره_عبارت* بیرون_Valuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_گره_عبارت* valuep = nullptr;
    };

    AstUser1Allocator<Ast_گره, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    بنية ConstAllocator final {
        اندازه_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        اندازه_t m_بعدی_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(اندازه_t currentGeneration, Ast_گره* nodep) {
            إذا (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_بعدی_Free = 0;
            }
            UASSERT_OBJ(m_بعدی_Free <= m_constps.اندازه(), nodep, "Should only allocate at end");
            إذا (m_بعدی_Free == m_constps.اندازه()) {
                m_constps.افزودن_پشت(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ثابت constp = m_constps[m_بعدی_Free++];
            constp->رقم().nodep(nodep);
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
    Ast_گره* m_whyNotNodep;  ///< First node not optimizable
    bool m_any_تخصیص_بده_Dly;  ///< True if found a delayed assignment
    bool m_any_تخصیص_بده_Comb;  ///< True if found a non-delayed assignment
    bool m_درون_Dly_تخصیص_بده;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    صحيح m_instr_شمار;  ///< Number of nodes
    صحيح m_داده_شمار;  ///< Bytes of data
    صحيح m_recurse_شمار = 0;  // Now deep in current recursion
    Ast_گره* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ثابت Ast_گره_DType*, ConstAllocator> m_constps;
    اندازه_t m_constGeneration = 0;
    std::وکتور<Sim_پشته_گره*> m_call_پشته;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::وکتور<Ast_گره*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_عدد(ثابت V3_عدد* nump, ثابت Ast_گره_DType* dtypep) {
        إذا (ثابت AstRefDType* ثابت refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت stp = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
            إذا (stp->packed()) {
                std::ostringstream بیرون;
                بیرون << "'{";
                لكل (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت صحيح عرض = itemp->عرض();
                    ثابت صحيح lsb = itemp->lsb();
                    ثابت صحيح msb = lsb + عرض - 1;
                    V3_عدد فیلد_رقم{nump, عرض};
                    فیلد_رقم.opSel(*nump, msb, lsb);
                    بیرون << itemp->name() << ": ";
                    إذا (ثابت Ast_گره_DType* ثابت فرزند_Typep = itemp->subDTypep()) {
                        بیرون << pretty_عدد(&فیلد_رقم, فرزند_Typep);
                    } وإلا {
                        بیرون << فیلد_رقم;
                    }
                    إذا (itemp->nextp()) بیرون << ", ";
                }
                بیرون << "}";
                أرجع بیرون.str();
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            إذا (ثابت Ast_گره_DType* ثابت فرزند_Typep = arrayp->subDTypep()) {
                std::ostringstream بیرون;
                بیرون << "[";
                ثابت صحيح arrayElements = arrayp->elementsConst();
                لكل (صحيح element = 0; element < arrayElements; ++element) {
                    ثابت صحيح عرض = فرزند_Typep->عرض();
                    ثابت صحيح lsb = عرض * element;
                    ثابت صحيح msb = lsb + عرض - 1;
                    V3_عدد فیلد_رقم{nump, عرض};
                    فیلد_رقم.opSel(*nump, msb, lsb);
                    ثابت صحيح arrayElem = arrayp->lo() + element;
                    بیرون << arrayElem << " = " << pretty_عدد(&فیلد_رقم, فرزند_Typep);
                    إذا (element < arrayElements - 1) بیرون << ", ";
                }
                بیرون << "]";
                أرجع بیرون.str();
            }
        }
        أرجع nump->ascii();
    }

    // Checking METHODS
عام:
    /// Call other-this function on all new *non-constant* var references
    مجازی فراغ varRefCb(AstVarRef* nodep) {}

    فراغ clearOptimizable(Ast_گره* nodep /*null ok*/, ثابت string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        إذا (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            إذا (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_پیشوند("Clear optimizable: " << why);
                إذا (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream پشته;
            صحيح n = 0;
            لكل (ثابت خودکار& callstack : vlstd::معکوس_view(m_call_پشته)) {
                ثابت AstFuncRef* ثابت funcp = callstack->m_funcp;
                پشته << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                لكل (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->انتها(); ++conIt) {
                    ثابت AstVar* ثابت portp = conIt->اولی;
                    Ast_گره_عبارت* ثابت pinp = conIt->دومی->exprp();
                    ثابت Ast_گره_DType* ثابت dtypep = pinp->dtypep();
                    إذا (AstConst* ثابت valp = fetchConstNull(pinp)) {
                        پشته << "\n           " << portp->prettyName() << " = "
                              << pretty_عدد(&valp->رقم(), dtypep);
                    } وإلا إذا (Ast_گره_عبارت* ثابت valuep = fetch_مقدار_Null(pinp)) {
                        پشته << "\n           " << portp->prettyName() << " = "
                              << مقصد_String_Recurse(valuep);
                    }
                }
                إذا (++n > CALL_پشته_MAX) {
                    پشته << "\n           ... stack truncated";
                    اكسر;
                }
            }
            m_whyNotOptimizable += پشته.str();
        }
    }
    bool optimizable() ثابت { أرجع m_whyNotNodep == nullptr; }
    string why_Not_پیام() ثابت { أرجع m_whyNotOptimizable; }
    Ast_گره* whyNotNodep() ثابت { أرجع m_whyNotNodep; }

    bool is_تخصیص_بده_Dly() ثابت { أرجع m_any_تخصیص_بده_Dly; }
    bool isImpure() ثابت { أرجع m_isImpure; }
    bool isCoverage() ثابت { أرجع m_isCoverage; }
    صحيح instr_شمار() ثابت { أرجع m_instr_شمار; }
    صحيح داده_شمار() ثابت { أرجع m_داده_شمار; }

    // Simulation METHODS
خاص:
    AstConst* allocConst(Ast_گره* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        أرجع m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

عام:
    فراغ new_مقدار(Ast_گره* nodep, ثابت Ast_گره_عبارت* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->رقم().op_تخصیص_بده(constp->رقم());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } وإلا إذا (fetch_مقدار_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            مجموعه_مقدار(nodep, newTrackedClone(const_cast<Ast_گره_عبارت*>(valuep)));
        }
    }
    فراغ new_بیرون_مقدار(Ast_گره* nodep, ثابت Ast_گره_عبارت* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            new_بیرون_Const(nodep)->رقم().op_تخصیص_بده(constp->رقم());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } وإلا إذا (fetch_بیرون_مقدار_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            مجموعه_بیرون_مقدار(nodep, newTrackedClone(const_cast<Ast_گره_عبارت*>(valuep)));
        }
    }

خاص:
    Ast_گره_عبارت* newTrackedClone(Ast_گره_عبارت* nodep) {
        Ast_گره_عبارت* ثابت newp = nodep->clone_درخت(false);
        m_reclaimValuesp.افزودن_پشت(newp);
        أرجع newp;
    }
    AstConst* newConst(Ast_گره* nodep) {
        // Set a constant value for this node
        إذا (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            أرجع constp;
        }
        أرجع fetchConst(nodep);
    }
    AstConst* new_بیرون_Const(Ast_گره* nodep) {
        // Set a var-output constant value for this node
        إذا (!VN_IS(m_varAux(nodep).بیرون_Valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).بیرون_Valuep = constp;
            أرجع constp;
        }
        أرجع fetch_بیرون_Const(nodep);
    }

عام:
    Ast_گره_عبارت* fetch_مقدار_Null(Ast_گره* nodep) { أرجع m_varAux(nodep).valuep; }

خاص:
    Ast_گره_عبارت* fetch_بیرون_مقدار_Null(Ast_گره* nodep) { أرجع m_varAux(nodep).بیرون_Valuep; }
    AstConst* fetchConstNull(Ast_گره* nodep) { أرجع VN_CAST(fetch_مقدار_Null(nodep), Const); }
    AstConst* fetch_بیرون_Const_Null(Ast_گره* nodep) {
        أرجع VN_CAST(fetch_بیرون_مقدار_Null(nodep), Const);
    }
    Ast_گره_عبارت* fetch_مقدار(Ast_گره* nodep) {
        Ast_گره_عبارت* ثابت valuep = fetch_مقدار_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        أرجع valuep;
    }
    AstConst* fetchConst(Ast_گره* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        أرجع constp;
    }
    AstConst* fetch_بیرون_Const(Ast_گره* nodep) {
        AstConst* ثابت constp = fetch_بیرون_Const_Null(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        أرجع constp;
    }

عام:
    V3_عدد* fetch_عدد_Null(Ast_گره* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        إذا (constp) أرجع &constp->رقم();
        أرجع nullptr;
    }
    V3_عدد* fetch_بیرون_عدد_Null(Ast_گره* nodep) {
        AstConst* ثابت constp = fetch_بیرون_Const_Null(nodep);
        إذا (constp) أرجع &constp->رقم();
        أرجع nullptr;
    }

خاص:
    فراغ مجموعه_مقدار(Ast_گره* nodep, Ast_گره_عبارت* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    فراغ مجموعه_بیرون_مقدار(Ast_گره* nodep, Ast_گره_عبارت* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).بیرون_Valuep = valuep;
    }

    فراغ check_گره_Info(Ast_گره* nodep, bool ignorePredict = false) {
        إذا (m_checkOnly) {
            m_instr_شمار += nodep->instr_شمار();
            m_داده_شمار += nodep->عرض();
        }
        إذا (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        إذا (!nodep->is_خالص()) m_isImpure = true;
    }

    فراغ معلوم_Bad_گره_Type(Ast_گره* nodep) {
        // Call for node types we know we can't handle
        إذا (jumpingOver()) أرجع;
        check_گره_Info(nodep);
        إذا (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    فراغ bad_گره_Type(Ast_گره* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_گره_Info(nodep);
        إذا (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#اگر_تعریف_شده VL_DEBUG
            ساكن std::مجموعه<VNType> s_typePrinted;
            ثابت خودکار pair = s_typePrinted.emplace(nodep->type());
            إذا (pair.دومی)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#پایان_اگر
        }
    }

    Ast_گره* varOrScope(AstVarRef* nodep) ثابت {
        Ast_گره* vscp;
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
    فراغ تخصیص_بده_بیرون_مقدار(ثابت Ast_گره_تخصیص_بده* nodep, Ast_گره* vscp, ثابت Ast_گره_عبارت* valuep) {
        إذا (VN_IS(nodep, تخصیص_بده_Dly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_بیرون_مقدار(vscp, valuep);
        } وإلا {
            new_مقدار(vscp, valuep);
            مجموعه_بیرون_مقدار(vscp, fetch_مقدار(vscp));
        }
    }

    string مقصد_String_Recurse(Ast_گره_عبارت* valuep) {
        // Return string representation of a value, or clearOptimizable
        إذا (ثابت AstConst* ثابت avaluep = VN_CAST(valuep, Const)) {
            أرجع "'h"s + avaluep->رقم().displayed(valuep, "%0x");
        }
        إذا (ثابت AstConsArray* ثابت avaluep = VN_CAST(valuep, ConsArray)) {
            string نتیجه = "'{";
            string comma;
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            لكل (صحيح n = 0; n < dtypep->elementsConst(); ++n) {
                نتیجه += comma + مقصد_String_Recurse(avaluep->valuep(n));
                comma = ", ";
            }
            نتیجه += "}";
            أرجع نتیجه;
        }
        إذا (ثابت AstConsStruct* ثابت avaluep = VN_CAST(valuep, ConsStruct)) {
            string نتیجه = "'{";
            string comma;
            لكل (ثابت Ast_Cons_Struct_قلم* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_قلم)) {
                نتیجه
                    += comma + itemp->memberp()->name() + ":" + مقصد_String_Recurse(itemp->valuep());
                comma = ", ";
            }
            نتیجه += "}";
            أرجع نتیجه;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        أرجع "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_گره_عبارت* newInitValuep(Ast_گره* nodep, Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_گره_عبارت* ثابت elementp = newInitValuep(nodep, arrayp->subDTypep());
            أرجع elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        إذا (structp && !structp->packed()) {
            AstConsStruct* ثابت newp = new AstConsStruct{nodep->fileline(), structp};
            لكل (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_گره_عبارت* ثابت memberValuep = newInitValuep(nodep, memberp->subDTypep());
                إذا (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_درخت(), newp);
                    أرجع nullptr;
                }
                newp->جمع_کن_Itemsp(new Ast_Cons_Struct_قلم{nodep->fileline(), memberp, memberValuep});
            }
            أرجع newp;
        }
        إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
            AstConst* ثابت constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_مقدار{}, basicp->عرض_Min(), 0};
            إذا (basicp->isZeroInit()) {
                constp->رقم() = V3_عدد{nodep, basicp};
            } وإلا {
                constp->رقم().مجموعه_All_بیتها_X();
            }
            أرجع constp;
        }
        أرجع nullptr;
    }
    فراغ initVar(AstVar* nodep) {
        إذا (Ast_گره_عبارت* ثابت valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_مقدار(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_درخت(), valuep);
        }
    }

    // VISITORS
    فراغ بازدید(AstAlways* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_Sen_درخت* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    فراغ بازدید(AstVarRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_گره* ثابت vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        إذا (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), گره_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        إذا (nodep->access().isWriteOrRW()) {
            إذا (m_درون_Dly_تخصیص_بده) {
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
                ثابت Ast_گره_عبارت* ثابت valuep
                    = varIsConst ? fetch_مقدار_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                إذا (varIsConst && valuep) {
                    إذا (!m_checkOnly && optimizable()) new_مقدار(vscp, valuep);
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
            Ast_گره_عبارت* valuep = fetch_مقدار_Null(vscp);
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
            مجموعه_مقدار(nodep, valuep);
        }
    }
    فراغ بازدید(AstVarXRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (m_scoped) {
            bad_گره_Type(nodep);
            أرجع;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_گره_Type(nodep);
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
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstInitialAutomaticStmt* nodep) override {
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstInitialStaticStmt* nodep) override {
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstInitialStatic* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_گره_Type(nodep);
            أرجع;
        }
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره_If* nodep) override {
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   IF " << nodep);
        check_گره_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterate_And_بعدی_Const_Null(nodep->condp());
            إذا (optimizable()) {
                إذا (fetchConst(nodep->condp())->رقم().isNeqZero()) {
                    iterate_And_بعدی_Const_Null(nodep->thensp());
                } وإلا {
                    iterate_And_بعدی_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    فراغ بازدید(AstConst* nodep) override {
        check_گره_Info(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        new_مقدار(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    فراغ بازدید(AstConsArray* nodep) override {
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsArray* ثابت valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_مقدار(nodep->defaultp())->clone_درخت(false) : nullptr};
        لكل (ثابت Ast_Cons_Array_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_قلم)) {
            valuep->valuep(itemp->شاخص(), fetch_مقدار(itemp->valuep())->clone_درخت(false));
        }
        m_reclaimValuesp.افزودن_پشت(valuep);
        مجموعه_مقدار(nodep, valuep);
    }
    فراغ بازدید(AstConsStruct* nodep) override {
        إذا (!m_params) {
            bad_گره_Type(nodep);
            أرجع;
        }
        check_گره_Info(nodep);
        لكل (Ast_Cons_Struct_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_قلم)) {
            iterateConst(itemp->valuep());
        }
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsStruct* ثابت valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        لكل (Ast_Cons_Struct_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_قلم)) {
            valuep->جمع_کن_Itemsp(
                new Ast_Cons_Struct_قلم{itemp->fileline(), itemp->memberp(),
                                      fetch_مقدار(itemp->valuep())->clone_درخت(false)});
        }
        m_reclaimValuesp.افزودن_پشت(valuep);
        مجموعه_مقدار(nodep, valuep);
    }
    فراغ بازدید(Ast_Cvt_Unpack_مقصد_Unpack* nodep) override {
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_گره_عبارت* ثابت cvtp = new Ast_Cvt_Unpack_مقصد_Unpack{
            nodep->fileline(), fetch_مقدار(nodep->fromp())->clone_درخت(false), nodep->dtypep()};
        Ast_گره_عبارت* ثابت valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.افزودن_پشت(valuep);
        مجموعه_مقدار(nodep, valuep);
    }
    فراغ بازدید(Ast_Cons_Array_قلم* nodep) override {
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_Enum_قلم_Ref* nodep) override {
        check_گره_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        إذا (m_checkOnly || !optimizable()) أرجع;
        Ast_گره* ثابت valuep = nodep->itemp()->valuep();
        إذا (valuep) {
            iterate_And_بعدی_Const_Null(valuep);
            إذا (!optimizable()) أرجع;
            new_مقدار(nodep, fetch_مقدار(valuep));
        } وإلا {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    فراغ بازدید(Ast_گره_Uniop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->عدد_عمل_کن(newConst(nodep)->رقم(), fetchConst(nodep->lhsp())->رقم());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    فراغ aggregateEqRecurse(V3_عدد& resultr, Ast_گره* nodep, bool caseEq, Ast_گره_عبارت* lhsp,
                            Ast_گره_عبارت* rhsp) {
        إذا (ثابت AstConst* ثابت lconstp = VN_CAST(lhsp, Const)) {
            ثابت V3_عدد& lnum = lconstp->رقم();
            ثابت V3_عدد& rnum = VN_AS(rhsp, Const)->رقم();
            V3_عدد eq{nodep, 1};
            إذا (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } وإلا إذا (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } وإلا إذا (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } وإلا {
                eq.opEq(lnum, rnum);
            }
            ثابت V3_عدد prev = resultr;
            resultr.opLogAnd(prev, eq);
        } وإلا إذا (ثابت AstConsArray* ثابت lconsp = VN_CAST(lhsp, ConsArray)) {
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            لكل (صحيح n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } وإلا {
            ثابت Ast_Cons_Struct_قلم* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            لكل (ثابت Ast_Cons_Struct_قلم* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), Cons_Struct_قلم),
                                          ritemp = VN_AS(ritemp->nextp(), Cons_Struct_قلم)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    فراغ handleAggregateCompare(Ast_گره_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ثابت bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ثابت bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_عدد& resultr = newConst(nodep)->رقم();
        resultr.مجموعه_All_Bits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_مقدار(nodep->lhsp()),
                           fetch_مقدار(nodep->rhsp()));
        إذا (neq) {
            ثابت V3_عدد eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    فراغ بازدید(Ast_گره_Biop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        إذا (!VN_IS(fetch_مقدار(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            أرجع;
        }
        AstConst* ثابت valuep = newConst(nodep);
        nodep->عدد_عمل_کن(newConst(nodep)->رقم(), fetchConst(nodep->lhsp())->رقم(),
                             fetchConst(nodep->rhsp())->رقم());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        إذا (!m_params && VN_IS(nodep, Sel) && valuep->رقم().isAnyX()) {
            ثابت V3_عدد رقم{valuep, valuep->عرض(), valuep->رقم()};
            valuep->رقم().op_بیتها_One(رقم);
        }
    }
    فراغ بازدید(Ast_گره_Triop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->عدد_عمل_کن(newConst(nodep)->رقم(), fetchConst(nodep->lhsp())->رقم(),
                             fetchConst(nodep->rhsp())->رقم(), fetchConst(nodep->thsp())->رقم());
    }
    فراغ بازدید(Ast_گره_Quadop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->عدد_عمل_کن(newConst(nodep)->رقم(), fetchConst(nodep->lhsp())->رقم(),
                             fetchConst(nodep->rhsp())->رقم(), fetchConst(nodep->thsp())->رقم(),
                             fetchConst(nodep->fhsp())->رقم());
    }
    فراغ بازدید(AstLogAnd* nodep) override {
        // Need to short circuit
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->رقم().isNeqZero()) {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_مقدار(nodep, fetch_مقدار(nodep->rhsp()));
            } وإلا {
                new_مقدار(nodep, fetch_مقدار(nodep->lhsp()));  // a zero
            }
        }
    }
    فراغ بازدید(AstLogOr* nodep) override {
        // Need to short circuit
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->رقم().isNeqZero()) {
                new_مقدار(nodep, fetch_مقدار(nodep->lhsp()));  // a one
            } وإلا {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_مقدار(nodep, fetch_مقدار(nodep->rhsp()));
            }
        }
    }
    فراغ بازدید(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->رقم().isEqZero()) {
                ثابت AstConst cnst{nodep->fileline(), AstConst::Widthed_مقدار{}, 1, 1};  // a one
                new_مقدار(nodep, &cnst);  // a one
            } وإلا {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_مقدار(nodep, fetch_مقدار(nodep->rhsp()));
            }
        }
    }
    فراغ بازدید(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->condp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->condp())->رقم().isNeqZero()) {
                iterateConst(nodep->thenp());
                إذا (!optimizable()) أرجع;
                new_مقدار(nodep, fetch_مقدار(nodep->thenp()));
            } وإلا {
                iterateConst(nodep->elsep());
                إذا (!optimizable()) أرجع;
                new_مقدار(nodep, fetch_مقدار(nodep->elsep()));
            }
        }
    }

    فراغ handle_تخصیص_بده_Element(Ast_گره_تخصیص_بده* nodep, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* مقدار_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::وکتور<Ast_گره_عبارت*> selps;
        Ast_گره_عبارت* fromp = lhsp;
        طالما (true) {
            إذا (AstArraySel* ثابت selp = VN_CAST(fromp, ArraySel)) {
                check_گره_Info(selp);
                iterate_And_بعدی_Const_Null(selp->bitp());  // Bit index
                selps.افزودن_پشت(selp);
                fromp = selp->fromp();
            } وإلا إذا (AstStructSel* ثابت selp = VN_CAST(fromp, StructSel)) {
                check_گره_Info(selp);
                selps.افزودن_پشت(selp);
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
        Ast_گره* ثابت vscp = varOrScope(varrefp);
        Ast_گره_عبارت* rootp = fetch_بیرون_مقدار_Null(vscp);
        إذا (!rootp) rootp = fetch_مقدار_Null(vscp);
        إذا (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            أرجع;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_گره_عبارت* itemp = rootp;
        لكل (Ast_گره_عبارت* ثابت selp : vlstd::معکوس_view(selps)) {
            إذا (ثابت AstArraySel* ثابت aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ثابت consp = VN_AS(itemp, ConsArray);
                ثابت uint32_t شاخص = fetchConst(aselp->bitp())->مقصد_UInt();
                ثابت صحيح elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                إذا (شاخص >= static_cast<uint32_t>(elements)) {
                    أرجع;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(شاخص);
                إذا (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_درخت(false);
                    consp->valuep(شاخص, itemp);
                }
            } وإلا {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_گره_عبارت* ثابت valuep = newTrackedClone(fetch_مقدار(مقدار_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.افزودن_پشت(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        تخصیص_بده_بیرون_مقدار(nodep, vscp, rootp);
    }
    فراغ handle_تخصیص_بده_Sel(Ast_گره_تخصیص_بده* nodep, AstSel* selp, Ast_گره_عبارت* مقدار_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_عدد lsb{nodep};
        handle_تخصیص_بده_Sel_Recurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        إذا (m_checkOnly || !optimizable()) أرجع;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_گره* ثابت vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        إذا (AstConst* ثابت vscpnump = fetch_بیرون_Const_Null(vscp)) {
            outconstp = vscpnump;
        } وإلا إذا (AstConst* ثابت vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } وإلا {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_مقدار{},
                                     varrefp->varp()->عرض_Min(), 0};
            إذا (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->رقم().مجموعه_All_Bits0();
            } وإلا {
                outconstp->رقم().مجموعه_All_بیتها_X();
            }
            m_reclaimValuesp.emplace_پشت(outconstp);
        }
        outconstp->رقم().opSelInto(fetchConst(مقدار_Fromp)->رقم(), lsb, selp->عرض_Const());
        تخصیص_بده_بیرون_مقدار(nodep, vscp, outconstp);
    }
    فراغ handle_تخصیص_بده_Sel_Recurse(Ast_گره_تخصیص_بده* nodep, AstSel* selp, AstVarRef*& بیرون_Varrefp_Ref,
                                V3_عدد& lsbRef, صحيح عمق) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_گره_Info(selp);
        iterate_And_بعدی_Const_Null(selp->lsbp());  // Bit index
        إذا (AstVarRef* ثابت varrefp = VN_CAST(selp->fromp(), VarRef)) {
            بیرون_Varrefp_Ref = varrefp;
            lsbRef = fetchConst(selp->lsbp())->رقم();
            أرجع;  // And presumably still optimizable()
        }
        إذا (AstSel* ثابت subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_عدد sublsb{nodep};
            handle_تخصیص_بده_Sel_Recurse(nodep, subselp, بیرون_Varrefp_Ref, sublsb /*ref*/, عمق + 1);
            إذا (optimizable()) {
                lsbRef = sublsb;
                lsbRef.op_جمع_کن(sublsb, fetchConst(selp->lsbp())->رقم());
            }
        } وإلا {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    فراغ handle_تخصیص_بده_Recurse(Ast_گره_تخصیص_بده* nodep, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* مقدار_Fromp) {
        إذا (!optimizable()) أرجع;
        إذا (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            إذا (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                أرجع;
            }
            handle_تخصیص_بده_Element(nodep, lhsp, مقدار_Fromp);
        } وإلا إذا (AstConcat* ثابت selp = VN_CAST(lhsp, Concat)) {
            check_گره_Info(selp);
            إذا (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                أرجع;
            }
            // Split value into left and right concat values
            إذا (!m_checkOnly) {
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_مقدار{},
                                       selp->lhsp()->عرض(), 0};
                    outconstp->رقم().opSel(fetchConst(مقدار_Fromp)->رقم(),
                                           selp->lhsp()->عرض() + selp->rhsp()->عرض() + 1,
                                           selp->rhsp()->عرض());
                    new_مقدار(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_درخت(), outconstp);
                }
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_مقدار{},
                                       selp->rhsp()->عرض_Min(), 0};
                    outconstp->رقم().opSel(fetchConst(مقدار_Fromp)->رقم(),
                                           selp->rhsp()->عرض() - 1, 0);
                    new_مقدار(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_درخت(), outconstp);
                }
            }
            handle_تخصیص_بده_Recurse(nodep, selp->lhsp(), selp->lhsp());
            handle_تخصیص_بده_Recurse(nodep, selp->rhsp(), selp->rhsp());
        } وإلا إذا (AstReplicate* ثابت selp = VN_CAST(lhsp, Replicate)) {
            check_گره_Info(selp);
            iterate_And_بعدی_Const_Null(selp->countp());
            AstConst* ثابت countp = VN_CAST(selp->countp(), Const);
            إذا (!countp || !countp->رقم().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                أرجع;
            }
            handle_تخصیص_بده_Recurse(nodep, selp->srcp(), مقدار_Fromp);
        } وإلا إذا (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            إذا (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                أرجع;
            }
            handle_تخصیص_بده_Sel(nodep, selp, مقدار_Fromp);
        } وإلا إذا (VN_IS(lhsp, VarRef)) {
            إذا (m_checkOnly) {
                iterate_And_بعدی_Const_Null(lhsp);
            } وإلا {
                Ast_گره* ثابت vscp = varOrScope(VN_CAST(lhsp, VarRef));
                تخصیص_بده_بیرون_مقدار(nodep, vscp, fetch_مقدار(مقدار_Fromp));
            }
        } وإلا {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);

        VL_RESTORER(m_درون_Dly_تخصیص_بده);

        إذا (VN_IS(nodep, تخصیص_بده_Force)) {
            clearOptimizable(nodep, "Force");
        } وإلا إذا (VN_IS(nodep, تخصیص_بده_Dly)) {
            إذا (m_any_تخصیص_بده_Comb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_تخصیص_بده_Dly = true;
            m_درون_Dly_تخصیص_بده = true;
        } وإلا {
            إذا (m_any_تخصیص_بده_Dly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_تخصیص_بده_Comb = true;
        }

        إذا (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            أرجع;
        }

        iterate_And_بعدی_Const_Null(nodep->rhsp());  // Value to assign
        handle_تخصیص_بده_Recurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    فراغ بازدید(AstArraySel* nodep) override {
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_مقدار_Null(nodep->fromp()), ConsArray)) {
            ثابت AstConst* ثابت indexp = fetchConst(nodep->bitp());
            ثابت uint32_t جابه_جایی = indexp->رقم().مقصد_UInt();
            Ast_گره_عبارت* ثابت itemp = consp->valuep(جابه_جایی);
            إذا (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::مقصد_string(جابه_جایی));
            } وإلا {
                مجموعه_مقدار(nodep, itemp);
            }
        } وإلا {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    فراغ بازدید(AstStructSel* nodep) override {
        إذا (!m_params) {
            bad_گره_Type(nodep);
            أرجع;
        }
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsStruct* ثابت consp = VN_AS(fetch_مقدار(nodep->fromp()), ConsStruct);
        مجموعه_مقدار(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    فراغ بازدید(Ast_برش_Sel* nodep) override {
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // Fetch the base constant array
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_مقدار_Null(nodep->fromp()), ConsArray)) {
            ثابت VNumRange& برش_Range = nodep->declRange();
            ثابت uint32_t برش_Elements = برش_Range.elements();
            ثابت صحيح برش_Lo = برش_Range.lo();
            // Use this node's dtype for the slice array
            Ast_گره_DType* ثابت dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ثابت newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            لكل (uint32_t idx = 0; idx < برش_Elements; ++idx) {
                ثابت uint32_t پایه_Idx = برش_Lo + idx;
                newConsp->valuep(idx, consp->valuep(پایه_Idx)->clone_درخت(false));
            }
            // Assign the new constant array and track it for later deletion
            مجموعه_مقدار(nodep, newConsp);
            m_reclaimValuesp.افزودن_پشت(newConsp);
        } وإلا {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    فراغ بازدید(AstBegin* nodep) override {
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstCase* nodep) override {
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   CASE " << nodep);
        check_گره_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا إذا (optimizable()) {
            iterate_And_بعدی_Const_Null(nodep->exprp());
            bool اصابت = false;
            لكل (Ast_Case_قلم* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_قلم)) {
                إذا (!itemp->isDefault()) {
                    لكل (Ast_گره* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        إذا (اصابت) اكسر;
                        iterate_And_بعدی_Const_Null(ep);
                        إذا (optimizable()) {
                            V3_عدد تطابق{nodep, 1};
                            تطابق.opEq(fetchConst(nodep->exprp())->رقم(), fetchConst(ep)->رقم());
                            إذا (تطابق.isNeqZero()) {
                                iterate_And_بعدی_Const_Null(itemp->stmtsp());
                                اصابت = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            لكل (Ast_Case_قلم* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_قلم)) {
                إذا (اصابت) اكسر;
                إذا (itemp->isDefault()) {
                    iterate_And_بعدی_Const_Null(itemp->stmtsp());
                    اصابت = true;
                }
            }
        }
    }

    فراغ بازدید(Ast_Case_قلم* nodep) override {
        // Real handling is in AstNodeCase
        إذا (jumpingOver()) أرجع;
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }

    فراغ بازدید(AstComment*) override {}

    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_عبارت_Stmt* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گره_Info(nodep);
        iterate_And_بعدی_Const_Null(nodep->stmtsp());
        إذا (!optimizable()) أرجع;
        iterate_And_بعدی_Const_Null(nodep->resultp());
        إذا (m_checkOnly || !optimizable()) أرجع;
        new_مقدار(nodep, fetch_مقدار(nodep->resultp()));
    }

    فراغ بازدید(Ast_Jump_بلوک* nodep) override {
        إذا (jumpingOver()) أرجع;
        iterateChildrenConst(nodep);
        إذا (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    فراغ بازدید(AstJumpGo* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گره_Info(nodep);
        إذا (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    فراغ بازدید(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        إذا (!m_params) {
            bad_گره_Type(nodep);
            أرجع;
        }
        check_گره_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
            أرجع;
        }
        إذا (!optimizable()) أرجع;

        اندازه_t iter_شمار = 0;
        ثابت اندازه_t iter_حد = v3Global.opt.unroll_حد();
        طالما (true) {
            إذا (iter_شمار > iter_حد) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::مقصد_string(iter_حد));
                اكسر;
            }
            ++iter_شمار;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_بعدی_Const_Null(nodep->stmtsp());
            إذا (jumpingOver()) اكسر;
        }

        إذا (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    فراغ بازدید(AstLoopTest* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گره_Info(nodep);
        iterateConst(nodep->condp());
        إذا (m_checkOnly || !optimizable()) أرجع;
        إذا (fetchConst(nodep->condp())->رقم().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    فراغ بازدید(AstStop* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_گره_Info(nodep);
    }
    فراغ بازدید(AstFuncRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_گره_Info(nodep);
        إذا (!m_params) {
            bad_گره_Type(nodep);
            أرجع;
        }
        Ast_گره_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        إذا (m_params) V3_عرض::عرض_Params_Edit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        إذا (funcp->recursive()) {
            إذا (m_recurse_شمار >= v3Global.opt.func_Recursion_عمق()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::مقصد_string(v3Global.opt.func_Recursion_عمق())
                                            + " times");
                أرجع;
            }
            ++m_recurse_شمار;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_گره*, Ast_گره_عبارت*> old_مقادیر;

        إذا (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_مقادیر](AstVar* varp) {
                إذا (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    إذا (Ast_گره_عبارت* ثابت valuep = fetch_مقدار_Null(varp)) {
                        Ast_گره_عبارت* ثابت nvaluep = newTrackedClone(valuep);
                        old_مقادیر.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_مقادیر](Ast_گره_عبارت* exprp) {
                إذا (VN_IS(exprp, Const)) أرجع;  // Speed up as won't change
                إذا (Ast_گره_عبارت* ثابت valuep = fetch_مقدار_Null(exprp)) {
                    Ast_گره_عبارت* ثابت nvaluep = newTrackedClone(valuep);
                    old_مقادیر.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::وکتور<std::pair<AstVar*, Ast_گره_عبارت*>> پورت_مقادیر;
        لكل (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.انتها(); ++it) {
            AstVar* ثابت portp = it->اولی;
            Ast_گره* ثابت pinp = it->دومی->exprp();
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
                پورت_مقادیر.emplace_پشت(std::make_pair(portp, newTrackedClone(fetch_مقدار(pinp))));
            }
        }
        // Apply value to the function
        إذا (!m_checkOnly && optimizable())
            لكل (ثابت خودکار& it : پورت_مقادیر) {
                إذا (!m_checkOnly && optimizable()) new_مقدار(it.اولی, it.دومی);
            }
        Sim_پشته_گره پشته_گره{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_پشته.افزودن_پشت(&پشته_گره);
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
        m_call_پشته.برداشت_پشت();
        ثابت Ast_گره_عبارت* returnp = nullptr;
        إذا (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_مقدار(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        لكل (ثابت خودکار& it : old_مقادیر) {
            إذا (it.دومی) new_مقدار(it.اولی, it.دومی);
        }
        إذا (returnp) new_مقدار(nodep, returnp);
        إذا (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_شمار > 0, nodep, "recurse underflow");
            --m_recurse_شمار;
        }
    }

    فراغ بازدید(AstVar* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_گره_Type(nodep);
            أرجع;
        }
    }

    فراغ بازدید(AstScopeName* /*nodep*/) override {
        إذا (jumpingOver()) أرجع;
        // Ignore
    }

    فراغ بازدید(AstSFormatArg* nodep) override {
        check_گره_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_بعدی_Const_Null(nodep->exprp());
    }
    فراغ بازدید(AstSFormatF* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;

        Ast_گره* بعدی_Argp = nodep->exprsp();
        string نتیجه;
        ثابت string format = nodep->متن();
        خودکار pos = format.cbegin();
        bool درون_Pct = false;
        string عرض;
        لكل (; pos != format.cend(); ++pos) {
            إذا (!درون_Pct && pos[0] == '%') {
                درون_Pct = true;
                عرض = "";
            } وإلا إذا (!درون_Pct) {  // Normal text
                نتیجه += *pos;
            } وإلا {  // Format character
                إذا (std::آیا_رقم(pos[0])) {
                    عرض += pos[0];
                    استمر;
                }

                درون_Pct = false;

                إذا (V3_عدد::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_گره* ثابت argp = بعدی_Argp;
                    بعدی_Argp = بعدی_Argp->nextp();
                    AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                    Ast_گره* ثابت subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ثابت constp = fetchConstNull(subargp);
                    ثابت VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    إذا (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        اكسر;
                    }
                    ثابت string pformat = "%"s + عرض + pos[0];
                    نتیجه += formatAttr.isEnum()
                                  ? constp->رقم().displayedEnum(fargp, pformat)
                                  : constp->رقم().displayed(nodep, pformat, formatAttr);
                } وإلا {
                    بدل (std::tolower(pos[0])) {
                    حالة '%': نتیجه += "%"; اكسر;
                    حالة 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        نتیجه += "%m";
                        اكسر;
                    افتراضي: clearOptimizable(nodep, "Unknown $display-like format code."); اكسر;
                    }
                }
            }
        }

        AstConst* ثابت نتیجه_Constp = new AstConst{nodep->fileline(), AstConst::String{}, نتیجه};
        مجموعه_مقدار(nodep, نتیجه_Constp);
        m_reclaimValuesp.افزودن_پشت(نتیجه_Constp);
    }

    فراغ بازدید(AstDisplay* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_گره_Info(nodep, /*display:*/ true);
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
    فراغ بازدید(Ast_مقصد_String_N* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گره_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        ثابت std::string نتیجه = مقصد_String_Recurse(fetch_مقدار(nodep->lhsp()));
        إذا (!optimizable()) أرجع;
        AstConst* ثابت نتیجه_Constp = new AstConst{nodep->fileline(), AstConst::String{}, نتیجه};
        مجموعه_مقدار(nodep, نتیجه_Constp);
        m_reclaimValuesp.افزودن_پشت(نتیجه_Constp);
    }

    فراغ بازدید(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    فراغ بازدید(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        معلوم_Bad_گره_Type(nodep);
    }
    فراغ بازدید(Ast_Get_Initial_Random_دانه* nodep) override { معلوم_Bad_گره_Type(nodep); }
    فراغ بازدید(AstMemberSel* nodep) override { معلوم_Bad_گره_Type(nodep); }
    فراغ بازدید(AstSampled* nodep) override { معلوم_Bad_گره_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    فراغ بازدید(Ast_گره* nodep) override {
        إذا (jumpingOver()) أرجع;
        bad_گره_Type(nodep);
    }

    // MEMBERS - called by constructor
    فراغ مجموعه_نما(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    فراغ mainGuts(Ast_گره* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

عام:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        مجموعه_نما(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    فراغ clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_any_تخصیص_بده_Comb = false;
        m_any_تخصیص_بده_Dly = false;
        m_درون_Dly_تخصیص_بده = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_شمار = 0;
        m_داده_شمار = 0;
        m_jumptargetp = nullptr;

        Ast_گره::user1_Clear_درخت();
        m_varAux.clear();
        ++m_constGeneration;
    }
    فراغ main_جدول_Check(Ast_گره* nodep) {
        مجموعه_نما(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    فراغ main_جدول_Emulate(Ast_گره* nodep) {
        مجموعه_نما(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    فراغ mainParamEmulate(Ast_گره* nodep) {
        مجموعه_نما(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::مجموعه<Ast_گره*> استفادهنشده_Rootps;
        لكل (Ast_گره* ثابت nodep : m_reclaimValuesp) {
            إذا (!nodep->backp()) استفادهنشده_Rootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        لكل (Ast_گره* ثابت nodep : استفادهنشده_Rootps) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
};

#پایان_اگر  // Guard
