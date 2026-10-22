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

#اگر_غیر_تعریف_شدہ VERILATOR_V3SIMULATE_H_
#تعریف_کرو VERILATOR_V3SIMULATE_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3AstUserAllocator.h"
#شامل "V3Const.h"
#شامل "V3Error.h"
#شامل "V3Task.h"
#شامل "V3Width.h"

#شامل <deque>
#شامل <set>
#شامل <sstream>
#شامل <stack>
#شامل <string>
#شامل <unordered_map>
#شامل <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_ڈھیر_گرہ final {
عوامی:
    // MEMBERS
    AstFuncRef* ثابت m_funcp;
    V3TaskConnects* ثابت m_tconnects;
    // CONSTRUCTORS
    Sim_ڈھیر_گرہ(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ڈھیر_گرہ() = طےشدہ;
};

class SimulateVisitor VL_NOT_FINAL : عوامی VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

نجی:
    // CONSTANTS
    ساکن constexpr صحیح CALL_ڈھیر_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ثابت VNUser1_اندر_Use m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    شمار VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    ساخت AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_گرہ_اظہاریہ* برآمد_Valuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_گرہ_اظہاریہ* valuep = nullptr;
    };

    AstUser1Allocator<Ast_گرہ, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    ساخت ConstAllocator final {
        سائز_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        سائز_t m_اگلا_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(سائز_t currentGeneration, Ast_گرہ* nodep) {
            اگر (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_اگلا_Free = 0;
            }
            UASSERT_OBJ(m_اگلا_Free <= m_constps.سائز(), nodep, "Should only allocate at end");
            اگر (m_اگلا_Free == m_constps.سائز()) {
                m_constps.ڈالو_عقب(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ثابت constp = m_constps[m_اگلا_Free++];
            constp->عدد().nodep(nodep);
            واپس constp;
        }

        ~ConstAllocator() {
            برائے (AstConst* ثابت constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_گرہ* m_whyNotNodep;  ///< First node not optimizable
    bool m_any_تفویض_Dly;  ///< True if found a delayed assignment
    bool m_any_تفویض_Comb;  ///< True if found a non-delayed assignment
    bool m_اندر_Dly_تفویض;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    صحیح m_instr_گنتی;  ///< Number of nodes
    صحیح m_ڈیٹا_گنتی;  ///< Bytes of data
    صحیح m_recurse_گنتی = 0;  // Now deep in current recursion
    Ast_گرہ* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ثابت Ast_گرہ_DType*, ConstAllocator> m_constps;
    سائز_t m_constGeneration = 0;
    std::سمتیہ<Sim_ڈھیر_گرہ*> m_call_ڈھیر;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::سمتیہ<Ast_گرہ*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_نمبر(ثابت V3_نمبر* nump, ثابت Ast_گرہ_DType* dtypep) {
        اگر (ثابت AstRefDType* ثابت refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت stp = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
            اگر (stp->packed()) {
                std::ostringstream برآمد;
                برآمد << "'{";
                برائے (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت صحیح چوڑائی = itemp->چوڑائی();
                    ثابت صحیح lsb = itemp->lsb();
                    ثابت صحیح msb = lsb + چوڑائی - 1;
                    V3_نمبر میدان_عدد{nump, چوڑائی};
                    میدان_عدد.opSel(*nump, msb, lsb);
                    برآمد << itemp->name() << ": ";
                    اگر (ثابت Ast_گرہ_DType* ثابت بچہ_Typep = itemp->subDTypep()) {
                        برآمد << pretty_نمبر(&میدان_عدد, بچہ_Typep);
                    } ورنہ {
                        برآمد << میدان_عدد;
                    }
                    اگر (itemp->nextp()) برآمد << ", ";
                }
                برآمد << "}";
                واپس برآمد.str();
            }
        } ورنہ اگر (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            اگر (ثابت Ast_گرہ_DType* ثابت بچہ_Typep = arrayp->subDTypep()) {
                std::ostringstream برآمد;
                برآمد << "[";
                ثابت صحیح arrayElements = arrayp->elementsConst();
                برائے (صحیح element = 0; element < arrayElements; ++element) {
                    ثابت صحیح چوڑائی = بچہ_Typep->چوڑائی();
                    ثابت صحیح lsb = چوڑائی * element;
                    ثابت صحیح msb = lsb + چوڑائی - 1;
                    V3_نمبر میدان_عدد{nump, چوڑائی};
                    میدان_عدد.opSel(*nump, msb, lsb);
                    ثابت صحیح arrayElem = arrayp->lo() + element;
                    برآمد << arrayElem << " = " << pretty_نمبر(&میدان_عدد, بچہ_Typep);
                    اگر (element < arrayElements - 1) برآمد << ", ";
                }
                برآمد << "]";
                واپس برآمد.str();
            }
        }
        واپس nump->ascii();
    }

    // Checking METHODS
عوامی:
    /// Call other-this function on all new *non-constant* var references
    مجازی خالی varRefCb(AstVarRef* nodep) {}

    خالی clearOptimizable(Ast_گرہ* nodep /*null ok*/, ثابت string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        اگر (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            اگر (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_سابقہ("Clear optimizable: " << why);
                اگر (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ڈھیر;
            صحیح n = 0;
            برائے (ثابت خودکار& callstack : vlstd::الٹ_view(m_call_ڈھیر)) {
                ثابت AstFuncRef* ثابت funcp = callstack->m_funcp;
                ڈھیر << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                برائے (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->انتہا(); ++conIt) {
                    ثابت AstVar* ثابت portp = conIt->پہلا;
                    Ast_گرہ_اظہاریہ* ثابت pinp = conIt->دوسرا->exprp();
                    ثابت Ast_گرہ_DType* ثابت dtypep = pinp->dtypep();
                    اگر (AstConst* ثابت valp = fetchConstNull(pinp)) {
                        ڈھیر << "\n           " << portp->prettyName() << " = "
                              << pretty_نمبر(&valp->عدد(), dtypep);
                    } ورنہ اگر (Ast_گرہ_اظہاریہ* ثابت valuep = fetch_قدر_Null(pinp)) {
                        ڈھیر << "\n           " << portp->prettyName() << " = "
                              << مقصد_String_Recurse(valuep);
                    }
                }
                اگر (++n > CALL_ڈھیر_MAX) {
                    ڈھیر << "\n           ... stack truncated";
                    توڑو;
                }
            }
            m_whyNotOptimizable += ڈھیر.str();
        }
    }
    bool optimizable() ثابت { واپس m_whyNotNodep == nullptr; }
    string why_Not_پیغام() ثابت { واپس m_whyNotOptimizable; }
    Ast_گرہ* whyNotNodep() ثابت { واپس m_whyNotNodep; }

    bool is_تفویض_Dly() ثابت { واپس m_any_تفویض_Dly; }
    bool isImpure() ثابت { واپس m_isImpure; }
    bool isCoverage() ثابت { واپس m_isCoverage; }
    صحیح instr_گنتی() ثابت { واپس m_instr_گنتی; }
    صحیح ڈیٹا_گنتی() ثابت { واپس m_ڈیٹا_گنتی; }

    // Simulation METHODS
نجی:
    AstConst* allocConst(Ast_گرہ* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        واپس m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

عوامی:
    خالی new_قدر(Ast_گرہ* nodep, ثابت Ast_گرہ_اظہاریہ* valuep) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->عدد().op_تفویض(constp->عدد());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } ورنہ اگر (fetch_قدر_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            سیٹ_قدر(nodep, newTrackedClone(const_cast<Ast_گرہ_اظہاریہ*>(valuep)));
        }
    }
    خالی new_برآمد_قدر(Ast_گرہ* nodep, ثابت Ast_گرہ_اظہاریہ* valuep) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            new_برآمد_Const(nodep)->عدد().op_تفویض(constp->عدد());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } ورنہ اگر (fetch_برآمد_قدر_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            سیٹ_برآمد_قدر(nodep, newTrackedClone(const_cast<Ast_گرہ_اظہاریہ*>(valuep)));
        }
    }

نجی:
    Ast_گرہ_اظہاریہ* newTrackedClone(Ast_گرہ_اظہاریہ* nodep) {
        Ast_گرہ_اظہاریہ* ثابت newp = nodep->clone_درخت(false);
        m_reclaimValuesp.ڈالو_عقب(newp);
        واپس newp;
    }
    AstConst* newConst(Ast_گرہ* nodep) {
        // Set a constant value for this node
        اگر (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            واپس constp;
        }
        واپس fetchConst(nodep);
    }
    AstConst* new_برآمد_Const(Ast_گرہ* nodep) {
        // Set a var-output constant value for this node
        اگر (!VN_IS(m_varAux(nodep).برآمد_Valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).برآمد_Valuep = constp;
            واپس constp;
        }
        واپس fetch_برآمد_Const(nodep);
    }

عوامی:
    Ast_گرہ_اظہاریہ* fetch_قدر_Null(Ast_گرہ* nodep) { واپس m_varAux(nodep).valuep; }

نجی:
    Ast_گرہ_اظہاریہ* fetch_برآمد_قدر_Null(Ast_گرہ* nodep) { واپس m_varAux(nodep).برآمد_Valuep; }
    AstConst* fetchConstNull(Ast_گرہ* nodep) { واپس VN_CAST(fetch_قدر_Null(nodep), Const); }
    AstConst* fetch_برآمد_Const_Null(Ast_گرہ* nodep) {
        واپس VN_CAST(fetch_برآمد_قدر_Null(nodep), Const);
    }
    Ast_گرہ_اظہاریہ* fetch_قدر(Ast_گرہ* nodep) {
        Ast_گرہ_اظہاریہ* ثابت valuep = fetch_قدر_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        واپس valuep;
    }
    AstConst* fetchConst(Ast_گرہ* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        واپس constp;
    }
    AstConst* fetch_برآمد_Const(Ast_گرہ* nodep) {
        AstConst* ثابت constp = fetch_برآمد_Const_Null(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        واپس constp;
    }

عوامی:
    V3_نمبر* fetch_نمبر_Null(Ast_گرہ* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        اگر (constp) واپس &constp->عدد();
        واپس nullptr;
    }
    V3_نمبر* fetch_برآمد_نمبر_Null(Ast_گرہ* nodep) {
        AstConst* ثابت constp = fetch_برآمد_Const_Null(nodep);
        اگر (constp) واپس &constp->عدد();
        واپس nullptr;
    }

نجی:
    خالی سیٹ_قدر(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    خالی سیٹ_برآمد_قدر(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).برآمد_Valuep = valuep;
    }

    خالی check_گرہ_Info(Ast_گرہ* nodep, bool ignorePredict = false) {
        اگر (m_checkOnly) {
            m_instr_گنتی += nodep->instr_گنتی();
            m_ڈیٹا_گنتی += nodep->چوڑائی();
        }
        اگر (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        اگر (!nodep->is_خالص()) m_isImpure = true;
    }

    خالی معلوم_Bad_گرہ_Type(Ast_گرہ* nodep) {
        // Call for node types we know we can't handle
        اگر (jumpingOver()) واپس;
        check_گرہ_Info(nodep);
        اگر (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    خالی bad_گرہ_Type(Ast_گرہ* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_گرہ_Info(nodep);
        اگر (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#اگر_تعریف_شدہ VL_DEBUG
            ساکن std::سیٹ<VNType> s_typePrinted;
            ثابت خودکار pair = s_typePrinted.emplace(nodep->type());
            اگر (pair.دوسرا)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#اگر_ختم
        }
    }

    Ast_گرہ* varOrScope(AstVarRef* nodep) ثابت {
        Ast_گرہ* vscp;
        اگر (m_scoped) {
            vscp = nodep->varScopep();
        } ورنہ {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        واپس vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() ثابت { واپس m_jumptargetp; }
    خالی تفویض_برآمد_قدر(ثابت Ast_گرہ_تفویض* nodep, Ast_گرہ* vscp, ثابت Ast_گرہ_اظہاریہ* valuep) {
        اگر (VN_IS(nodep, تفویض_Dly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_برآمد_قدر(vscp, valuep);
        } ورنہ {
            new_قدر(vscp, valuep);
            سیٹ_برآمد_قدر(vscp, fetch_قدر(vscp));
        }
    }

    string مقصد_String_Recurse(Ast_گرہ_اظہاریہ* valuep) {
        // Return string representation of a value, or clearOptimizable
        اگر (ثابت AstConst* ثابت avaluep = VN_CAST(valuep, Const)) {
            واپس "'h"s + avaluep->عدد().displayed(valuep, "%0x");
        }
        اگر (ثابت AstConsArray* ثابت avaluep = VN_CAST(valuep, ConsArray)) {
            string نتیجہ = "'{";
            string comma;
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            برائے (صحیح n = 0; n < dtypep->elementsConst(); ++n) {
                نتیجہ += comma + مقصد_String_Recurse(avaluep->valuep(n));
                comma = ", ";
            }
            نتیجہ += "}";
            واپس نتیجہ;
        }
        اگر (ثابت AstConsStruct* ثابت avaluep = VN_CAST(valuep, ConsStruct)) {
            string نتیجہ = "'{";
            string comma;
            برائے (ثابت Ast_Cons_Struct_شے* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_شے)) {
                نتیجہ
                    += comma + itemp->memberp()->name() + ":" + مقصد_String_Recurse(itemp->valuep());
                comma = ", ";
            }
            نتیجہ += "}";
            واپس نتیجہ;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        واپس "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_گرہ_اظہاریہ* newInitValuep(Ast_گرہ* nodep, Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_گرہ_اظہاریہ* ثابت elementp = newInitValuep(nodep, arrayp->subDTypep());
            واپس elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        اگر (structp && !structp->packed()) {
            AstConsStruct* ثابت newp = new AstConsStruct{nodep->fileline(), structp};
            برائے (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_گرہ_اظہاریہ* ثابت memberValuep = newInitValuep(nodep, memberp->subDTypep());
                اگر (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_درخت(), newp);
                    واپس nullptr;
                }
                newp->جوڑو_Itemsp(new Ast_Cons_Struct_شے{nodep->fileline(), memberp, memberValuep});
            }
            واپس newp;
        }
        اگر (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
            AstConst* ثابت constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, basicp->چوڑائی_Min(), 0};
            اگر (basicp->isZeroInit()) {
                constp->عدد() = V3_نمبر{nodep, basicp};
            } ورنہ {
                constp->عدد().سیٹ_All_بٹ_X();
            }
            واپس constp;
        }
        واپس nullptr;
    }
    خالی initVar(AstVar* nodep) {
        اگر (Ast_گرہ_اظہاریہ* ثابت valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_قدر(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_درخت(), valuep);
        }
    }

    // VISITORS
    خالی دورہ(AstAlways* nodep) override {
        اگر (jumpingOver()) واپس;
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_Sen_درخت* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    خالی دورہ(AstVarRef* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!optimizable()) واپس;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_گرہ* ثابت vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        اگر (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), گرہ_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        اگر (nodep->access().isWriteOrRW()) {
            اگر (m_اندر_Dly_تفویض) {
                اگر (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    اگر (m_checkOnly) varRefCb(nodep);
                }
            } ورنہ {  // nondly asn
                اگر (!(m_varAux(vscp).usage & VU_LV)) {
                    اگر (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    اگر (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        اگر (nodep->access().isReadOrRW()) {
            اگر (!(m_varAux(vscp).usage & VU_RV)) {
                اگر (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                ثابت bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                ثابت Ast_گرہ_اظہاریہ* ثابت valuep
                    = varIsConst ? fetch_قدر_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                اگر (varIsConst && valuep) {
                    اگر (!m_checkOnly && optimizable()) new_قدر(vscp, valuep);
                } ورنہ {
                    اگر (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        اگر (m_checkOnly || !optimizable()) واپس;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_گرہ_اظہاریہ* valuep = fetch_قدر_Null(vscp);
            اگر (!valuep) {
                اگر (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } ورنہ {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            سیٹ_قدر(nodep, valuep);
        }
    }
    خالی دورہ(AstVarXRef* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (m_scoped) {
            bad_گرہ_Type(nodep);
            واپس;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!m_params) {
            bad_گرہ_Type(nodep);
            واپس;
        }
        اگر (nodep->dpiImport()) {
            اگر (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        اگر (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstInitialAutomaticStmt* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstInitialStaticStmt* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstInitialStatic* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!m_params) {
            bad_گرہ_Type(nodep);
            واپس;
        }
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_گرہ_If* nodep) override {
        اگر (jumpingOver()) واپس;
        UINFO(5, "   IF " << nodep);
        check_گرہ_Info(nodep);
        اگر (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ورنہ {
            iterate_And_اگلا_Const_Null(nodep->condp());
            اگر (optimizable()) {
                اگر (fetchConst(nodep->condp())->عدد().isNeqZero()) {
                    iterate_And_اگلا_Const_Null(nodep->thensp());
                } ورنہ {
                    iterate_And_اگلا_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    خالی دورہ(AstConst* nodep) override {
        check_گرہ_Info(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        new_قدر(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    خالی دورہ(AstConsArray* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        AstConsArray* ثابت valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_قدر(nodep->defaultp())->clone_درخت(false) : nullptr};
        برائے (ثابت Ast_Cons_Array_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_شے)) {
            valuep->valuep(itemp->اشاریہ(), fetch_قدر(itemp->valuep())->clone_درخت(false));
        }
        m_reclaimValuesp.ڈالو_عقب(valuep);
        سیٹ_قدر(nodep, valuep);
    }
    خالی دورہ(AstConsStruct* nodep) override {
        اگر (!m_params) {
            bad_گرہ_Type(nodep);
            واپس;
        }
        check_گرہ_Info(nodep);
        برائے (Ast_Cons_Struct_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_شے)) {
            iterateConst(itemp->valuep());
        }
        اگر (m_checkOnly || !optimizable()) واپس;
        AstConsStruct* ثابت valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        برائے (Ast_Cons_Struct_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_شے)) {
            valuep->جوڑو_Itemsp(
                new Ast_Cons_Struct_شے{itemp->fileline(), itemp->memberp(),
                                      fetch_قدر(itemp->valuep())->clone_درخت(false)});
        }
        m_reclaimValuesp.ڈالو_عقب(valuep);
        سیٹ_قدر(nodep, valuep);
    }
    خالی دورہ(Ast_Cvt_Unpack_مقصد_Unpack* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_گرہ_اظہاریہ* ثابت cvtp = new Ast_Cvt_Unpack_مقصد_Unpack{
            nodep->fileline(), fetch_قدر(nodep->fromp())->clone_درخت(false), nodep->dtypep()};
        Ast_گرہ_اظہاریہ* ثابت valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ڈالو_عقب(valuep);
        سیٹ_قدر(nodep, valuep);
    }
    خالی دورہ(Ast_Cons_Array_شے* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_Enum_شے_Ref* nodep) override {
        check_گرہ_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        اگر (m_checkOnly || !optimizable()) واپس;
        Ast_گرہ* ثابت valuep = nodep->itemp()->valuep();
        اگر (valuep) {
            iterate_And_اگلا_Const_Null(valuep);
            اگر (!optimizable()) واپس;
            new_قدر(nodep, fetch_قدر(valuep));
        } ورنہ {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    خالی دورہ(Ast_گرہ_Uniop* nodep) override {
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        nodep->نمبر_عمل_کرو(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    خالی aggregateEqRecurse(V3_نمبر& resultr, Ast_گرہ* nodep, bool caseEq, Ast_گرہ_اظہاریہ* lhsp,
                            Ast_گرہ_اظہاریہ* rhsp) {
        اگر (ثابت AstConst* ثابت lconstp = VN_CAST(lhsp, Const)) {
            ثابت V3_نمبر& lnum = lconstp->عدد();
            ثابت V3_نمبر& rnum = VN_AS(rhsp, Const)->عدد();
            V3_نمبر eq{nodep, 1};
            اگر (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } ورنہ اگر (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } ورنہ اگر (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } ورنہ {
                eq.opEq(lnum, rnum);
            }
            ثابت V3_نمبر پچھلا_2 = resultr;
            resultr.opLogAnd(پچھلا_2, eq);
        } ورنہ اگر (ثابت AstConsArray* ثابت lconsp = VN_CAST(lhsp, ConsArray)) {
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            برائے (صحیح n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } ورنہ {
            ثابت Ast_Cons_Struct_شے* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            برائے (ثابت Ast_Cons_Struct_شے* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), Cons_Struct_شے),
                                          ritemp = VN_AS(ritemp->nextp(), Cons_Struct_شے)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    خالی handleAggregateCompare(Ast_گرہ_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ثابت bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ثابت bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_نمبر& resultr = newConst(nodep)->عدد();
        resultr.سیٹ_All_Bits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_قدر(nodep->lhsp()),
                           fetch_قدر(nodep->rhsp()));
        اگر (neq) {
            ثابت V3_نمبر eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    خالی دورہ(Ast_گرہ_Biop* nodep) override {
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        اگر (!VN_IS(fetch_قدر(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            واپس;
        }
        AstConst* ثابت valuep = newConst(nodep);
        nodep->نمبر_عمل_کرو(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        اگر (!m_params && VN_IS(nodep, Sel) && valuep->عدد().isAnyX()) {
            ثابت V3_نمبر عدد{valuep, valuep->چوڑائی(), valuep->عدد()};
            valuep->عدد().op_بٹ_One(عدد);
        }
    }
    خالی دورہ(Ast_گرہ_Triop* nodep) override {
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        nodep->نمبر_عمل_کرو(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد(), fetchConst(nodep->thsp())->عدد());
    }
    خالی دورہ(Ast_گرہ_Quadop* nodep) override {
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        nodep->نمبر_عمل_کرو(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد(), fetchConst(nodep->thsp())->عدد(),
                             fetchConst(nodep->fhsp())->عدد());
    }
    خالی دورہ(AstLogAnd* nodep) override {
        // Need to short circuit
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        اگر (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ورنہ {
            iterateConst(nodep->lhsp());
            اگر (!optimizable()) واپس;
            اگر (fetchConst(nodep->lhsp())->عدد().isNeqZero()) {
                iterateConst(nodep->rhsp());
                اگر (!optimizable()) واپس;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            } ورنہ {
                new_قدر(nodep, fetch_قدر(nodep->lhsp()));  // a zero
            }
        }
    }
    خالی دورہ(AstLogOr* nodep) override {
        // Need to short circuit
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        اگر (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ورنہ {
            iterateConst(nodep->lhsp());
            اگر (!optimizable()) واپس;
            اگر (fetchConst(nodep->lhsp())->عدد().isNeqZero()) {
                new_قدر(nodep, fetch_قدر(nodep->lhsp()));  // a one
            } ورنہ {
                iterateConst(nodep->rhsp());
                اگر (!optimizable()) واپس;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            }
        }
    }
    خالی دورہ(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        اگر (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ورنہ {
            iterateConst(nodep->lhsp());
            اگر (!optimizable()) واپس;
            اگر (fetchConst(nodep->lhsp())->عدد().isEqZero()) {
                ثابت AstConst cnst{nodep->fileline(), AstConst::Widthed_قدر{}, 1, 1};  // a one
                new_قدر(nodep, &cnst);  // a one
            } ورنہ {
                iterateConst(nodep->rhsp());
                اگر (!optimizable()) واپس;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            }
        }
    }
    خالی دورہ(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        اگر (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ورنہ {
            iterateConst(nodep->condp());
            اگر (!optimizable()) واپس;
            اگر (fetchConst(nodep->condp())->عدد().isNeqZero()) {
                iterateConst(nodep->thenp());
                اگر (!optimizable()) واپس;
                new_قدر(nodep, fetch_قدر(nodep->thenp()));
            } ورنہ {
                iterateConst(nodep->elsep());
                اگر (!optimizable()) واپس;
                new_قدر(nodep, fetch_قدر(nodep->elsep()));
            }
        }
    }

    خالی handle_تفویض_Element(Ast_گرہ_تفویض* nodep, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* قدر_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::سمتیہ<Ast_گرہ_اظہاریہ*> selps;
        Ast_گرہ_اظہاریہ* fromp = lhsp;
        جب (true) {
            اگر (AstArraySel* ثابت selp = VN_CAST(fromp, ArraySel)) {
                check_گرہ_Info(selp);
                iterate_And_اگلا_Const_Null(selp->bitp());  // Bit index
                selps.ڈالو_عقب(selp);
                fromp = selp->fromp();
            } ورنہ اگر (AstStructSel* ثابت selp = VN_CAST(fromp, StructSel)) {
                check_گرہ_Info(selp);
                selps.ڈالو_عقب(selp);
                fromp = selp->fromp();
            } ورنہ {
                توڑو;
            }
        }
        AstVarRef* ثابت varrefp = VN_CAST(fromp, VarRef);
        اگر (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            واپس;
        }
        اگر (m_checkOnly || !optimizable()) واپس;
        Ast_گرہ* ثابت vscp = varOrScope(varrefp);
        Ast_گرہ_اظہاریہ* rootp = fetch_برآمد_قدر_Null(vscp);
        اگر (!rootp) rootp = fetch_قدر_Null(vscp);
        اگر (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            واپس;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_گرہ_اظہاریہ* itemp = rootp;
        برائے (Ast_گرہ_اظہاریہ* ثابت selp : vlstd::الٹ_view(selps)) {
            اگر (ثابت AstArraySel* ثابت aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ثابت consp = VN_AS(itemp, ConsArray);
                ثابت uint32_t اشاریہ = fetchConst(aselp->bitp())->مقصد_UInt();
                ثابت صحیح elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                اگر (اشاریہ >= static_cast<uint32_t>(elements)) {
                    واپس;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(اشاریہ);
                اگر (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_درخت(false);
                    consp->valuep(اشاریہ, itemp);
                }
            } ورنہ {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_گرہ_اظہاریہ* ثابت valuep = newTrackedClone(fetch_قدر(قدر_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ڈالو_عقب(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        تفویض_برآمد_قدر(nodep, vscp, rootp);
    }
    خالی handle_تفویض_Sel(Ast_گرہ_تفویض* nodep, AstSel* selp, Ast_گرہ_اظہاریہ* قدر_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_نمبر lsb{nodep};
        handle_تفویض_Sel_Recurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        اگر (m_checkOnly || !optimizable()) واپس;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_گرہ* ثابت vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        اگر (AstConst* ثابت vscpnump = fetch_برآمد_Const_Null(vscp)) {
            outconstp = vscpnump;
        } ورنہ اگر (AstConst* ثابت vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } ورنہ {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_قدر{},
                                     varrefp->varp()->چوڑائی_Min(), 0};
            اگر (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->عدد().سیٹ_All_Bits0();
            } ورنہ {
                outconstp->عدد().سیٹ_All_بٹ_X();
            }
            m_reclaimValuesp.emplace_عقب(outconstp);
        }
        outconstp->عدد().opSelInto(fetchConst(قدر_Fromp)->عدد(), lsb, selp->چوڑائی_Const());
        تفویض_برآمد_قدر(nodep, vscp, outconstp);
    }
    خالی handle_تفویض_Sel_Recurse(Ast_گرہ_تفویض* nodep, AstSel* selp, AstVarRef*& برآمد_Varrefp_Ref,
                                V3_نمبر& lsbRef, صحیح گہرائی) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_گرہ_Info(selp);
        iterate_And_اگلا_Const_Null(selp->lsbp());  // Bit index
        اگر (AstVarRef* ثابت varrefp = VN_CAST(selp->fromp(), VarRef)) {
            برآمد_Varrefp_Ref = varrefp;
            lsbRef = fetchConst(selp->lsbp())->عدد();
            واپس;  // And presumably still optimizable()
        }
        اگر (AstSel* ثابت subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_نمبر sublsb{nodep};
            handle_تفویض_Sel_Recurse(nodep, subselp, برآمد_Varrefp_Ref, sublsb /*ref*/, گہرائی + 1);
            اگر (optimizable()) {
                lsbRef = sublsb;
                lsbRef.op_جوڑو(sublsb, fetchConst(selp->lsbp())->عدد());
            }
        } ورنہ {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    خالی handle_تفویض_Recurse(Ast_گرہ_تفویض* nodep, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* قدر_Fromp) {
        اگر (!optimizable()) واپس;
        اگر (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            اگر (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                واپس;
            }
            handle_تفویض_Element(nodep, lhsp, قدر_Fromp);
        } ورنہ اگر (AstConcat* ثابت selp = VN_CAST(lhsp, Concat)) {
            check_گرہ_Info(selp);
            اگر (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                واپس;
            }
            // Split value into left and right concat values
            اگر (!m_checkOnly) {
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_قدر{},
                                       selp->lhsp()->چوڑائی(), 0};
                    outconstp->عدد().opSel(fetchConst(قدر_Fromp)->عدد(),
                                           selp->lhsp()->چوڑائی() + selp->rhsp()->چوڑائی() + 1,
                                           selp->rhsp()->چوڑائی());
                    new_قدر(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_درخت(), outconstp);
                }
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_قدر{},
                                       selp->rhsp()->چوڑائی_Min(), 0};
                    outconstp->عدد().opSel(fetchConst(قدر_Fromp)->عدد(),
                                           selp->rhsp()->چوڑائی() - 1, 0);
                    new_قدر(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_درخت(), outconstp);
                }
            }
            handle_تفویض_Recurse(nodep, selp->lhsp(), selp->lhsp());
            handle_تفویض_Recurse(nodep, selp->rhsp(), selp->rhsp());
        } ورنہ اگر (AstReplicate* ثابت selp = VN_CAST(lhsp, Replicate)) {
            check_گرہ_Info(selp);
            iterate_And_اگلا_Const_Null(selp->countp());
            AstConst* ثابت countp = VN_CAST(selp->countp(), Const);
            اگر (!countp || !countp->عدد().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                واپس;
            }
            handle_تفویض_Recurse(nodep, selp->srcp(), قدر_Fromp);
        } ورنہ اگر (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            اگر (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                واپس;
            }
            handle_تفویض_Sel(nodep, selp, قدر_Fromp);
        } ورنہ اگر (VN_IS(lhsp, VarRef)) {
            اگر (m_checkOnly) {
                iterate_And_اگلا_Const_Null(lhsp);
            } ورنہ {
                Ast_گرہ* ثابت vscp = varOrScope(VN_CAST(lhsp, VarRef));
                تفویض_برآمد_قدر(nodep, vscp, fetch_قدر(قدر_Fromp));
            }
        } ورنہ {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);

        VL_RESTORER(m_اندر_Dly_تفویض);

        اگر (VN_IS(nodep, تفویض_Force)) {
            clearOptimizable(nodep, "Force");
        } ورنہ اگر (VN_IS(nodep, تفویض_Dly)) {
            اگر (m_any_تفویض_Comb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_تفویض_Dly = true;
            m_اندر_Dly_تفویض = true;
        } ورنہ {
            اگر (m_any_تفویض_Dly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_تفویض_Comb = true;
        }

        اگر (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            واپس;
        }

        iterate_And_اگلا_Const_Null(nodep->rhsp());  // Value to assign
        handle_تفویض_Recurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    خالی دورہ(AstArraySel* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_قدر_Null(nodep->fromp()), ConsArray)) {
            ثابت AstConst* ثابت indexp = fetchConst(nodep->bitp());
            ثابت uint32_t انحراف = indexp->عدد().مقصد_UInt();
            Ast_گرہ_اظہاریہ* ثابت itemp = consp->valuep(انحراف);
            اگر (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::مقصد_string(انحراف));
            } ورنہ {
                سیٹ_قدر(nodep, itemp);
            }
        } ورنہ {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    خالی دورہ(AstStructSel* nodep) override {
        اگر (!m_params) {
            bad_گرہ_Type(nodep);
            واپس;
        }
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        AstConsStruct* ثابت consp = VN_AS(fetch_قدر(nodep->fromp()), ConsStruct);
        سیٹ_قدر(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    خالی دورہ(Ast_قاش_Sel* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        // Fetch the base constant array
        اگر (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_قدر_Null(nodep->fromp()), ConsArray)) {
            ثابت VNumRange& قاش_Range = nodep->declRange();
            ثابت uint32_t قاش_Elements = قاش_Range.elements();
            ثابت صحیح قاش_Lo = قاش_Range.lo();
            // Use this node's dtype for the slice array
            Ast_گرہ_DType* ثابت dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ثابت newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            برائے (uint32_t idx = 0; idx < قاش_Elements; ++idx) {
                ثابت uint32_t بنیاد_Idx = قاش_Lo + idx;
                newConsp->valuep(idx, consp->valuep(بنیاد_Idx)->clone_درخت(false));
            }
            // Assign the new constant array and track it for later deletion
            سیٹ_قدر(nodep, newConsp);
            m_reclaimValuesp.ڈالو_عقب(newConsp);
        } ورنہ {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    خالی دورہ(AstBegin* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstCase* nodep) override {
        اگر (jumpingOver()) واپس;
        UINFO(5, "   CASE " << nodep);
        check_گرہ_Info(nodep);
        اگر (m_checkOnly) {
            iterateChildrenConst(nodep);
        } ورنہ اگر (optimizable()) {
            iterate_And_اگلا_Const_Null(nodep->exprp());
            bool کامیابی = false;
            برائے (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_شے)) {
                اگر (!itemp->isDefault()) {
                    برائے (Ast_گرہ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        اگر (کامیابی) توڑو;
                        iterate_And_اگلا_Const_Null(ep);
                        اگر (optimizable()) {
                            V3_نمبر مطابقت{nodep, 1};
                            مطابقت.opEq(fetchConst(nodep->exprp())->عدد(), fetchConst(ep)->عدد());
                            اگر (مطابقت.isNeqZero()) {
                                iterate_And_اگلا_Const_Null(itemp->stmtsp());
                                کامیابی = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            برائے (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_شے)) {
                اگر (کامیابی) توڑو;
                اگر (itemp->isDefault()) {
                    iterate_And_اگلا_Const_Null(itemp->stmtsp());
                    کامیابی = true;
                }
            }
        }
    }

    خالی دورہ(Ast_Case_شے* nodep) override {
        // Real handling is in AstNodeCase
        اگر (jumpingOver()) واپس;
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstComment*) override {}

    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {
        اگر (jumpingOver()) واپس;
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_اظہاریہ_Stmt* nodep) override {
        اگر (jumpingOver()) واپس;
        check_گرہ_Info(nodep);
        iterate_And_اگلا_Const_Null(nodep->stmtsp());
        اگر (!optimizable()) واپس;
        iterate_And_اگلا_Const_Null(nodep->resultp());
        اگر (m_checkOnly || !optimizable()) واپس;
        new_قدر(nodep, fetch_قدر(nodep->resultp()));
    }

    خالی دورہ(Ast_Jump_بلاک* nodep) override {
        اگر (jumpingOver()) واپس;
        iterateChildrenConst(nodep);
        اگر (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    خالی دورہ(AstJumpGo* nodep) override {
        اگر (jumpingOver()) واپس;
        check_گرہ_Info(nodep);
        اگر (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    خالی دورہ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        اگر (jumpingOver()) واپس;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        اگر (!m_params) {
            bad_گرہ_Type(nodep);
            واپس;
        }
        check_گرہ_Info(nodep);
        اگر (m_checkOnly) {
            iterateChildrenConst(nodep);
            واپس;
        }
        اگر (!optimizable()) واپس;

        سائز_t iter_گنتی = 0;
        ثابت سائز_t iter_حد_بندی = v3Global.opt.unroll_حد_بندی();
        جب (true) {
            اگر (iter_گنتی > iter_حد_بندی) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::مقصد_string(iter_حد_بندی));
                توڑو;
            }
            ++iter_گنتی;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
            اگر (jumpingOver()) توڑو;
        }

        اگر (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    خالی دورہ(AstLoopTest* nodep) override {
        اگر (jumpingOver()) واپس;
        check_گرہ_Info(nodep);
        iterateConst(nodep->condp());
        اگر (m_checkOnly || !optimizable()) واپس;
        اگر (fetchConst(nodep->condp())->عدد().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    خالی دورہ(AstStop* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_گرہ_Info(nodep);
    }
    خالی دورہ(AstFuncRef* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!optimizable()) واپس;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_گرہ_Info(nodep);
        اگر (!m_params) {
            bad_گرہ_Type(nodep);
            واپس;
        }
        Ast_گرہ_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        اگر (m_params) V3_چوڑائی::چوڑائی_Params_Edit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        اگر (funcp->recursive()) {
            اگر (m_recurse_گنتی >= v3Global.opt.func_Recursion_گہرائی()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::مقصد_string(v3Global.opt.func_Recursion_گہرائی())
                                            + " times");
                واپس;
            }
            ++m_recurse_گنتی;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_گرہ*, Ast_گرہ_اظہاریہ*> old_اقدار;

        اگر (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_اقدار](AstVar* varp) {
                اگر (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    اگر (Ast_گرہ_اظہاریہ* ثابت valuep = fetch_قدر_Null(varp)) {
                        Ast_گرہ_اظہاریہ* ثابت nvaluep = newTrackedClone(valuep);
                        old_اقدار.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_اقدار](Ast_گرہ_اظہاریہ* exprp) {
                اگر (VN_IS(exprp, Const)) واپس;  // Speed up as won't change
                اگر (Ast_گرہ_اظہاریہ* ثابت valuep = fetch_قدر_Null(exprp)) {
                    Ast_گرہ_اظہاریہ* ثابت nvaluep = newTrackedClone(valuep);
                    old_اقدار.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::سمتیہ<std::pair<AstVar*, Ast_گرہ_اظہاریہ*>> پورٹ_اقدار;
        برائے (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.انتہا(); ++it) {
            AstVar* ثابت portp = it->پہلا;
            Ast_گرہ* ثابت pinp = it->دوسرا->exprp();
            اگر (pinp) {  // Else too few arguments in function call - ignore it
                اگر (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    واپس;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                پورٹ_اقدار.emplace_عقب(std::make_pair(portp, newTrackedClone(fetch_قدر(pinp))));
            }
        }
        // Apply value to the function
        اگر (!m_checkOnly && optimizable())
            برائے (ثابت خودکار& it : پورٹ_اقدار) {
                اگر (!m_checkOnly && optimizable()) new_قدر(it.پہلا, it.دوسرا);
            }
        Sim_ڈھیر_گرہ ڈھیر_گرہ{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ڈھیر.ڈالو_عقب(&ڈھیر_گرہ);
        اگر (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                اگر (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_ڈھیر.نکالو_عقب();
        ثابت Ast_گرہ_اظہاریہ* returnp = nullptr;
        اگر (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_قدر(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        برائے (ثابت خودکار& it : old_اقدار) {
            اگر (it.دوسرا) new_قدر(it.پہلا, it.دوسرا);
        }
        اگر (returnp) new_قدر(nodep, returnp);
        اگر (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_گنتی > 0, nodep, "recurse underflow");
            --m_recurse_گنتی;
        }
    }

    خالی دورہ(AstVar* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!m_params) {
            bad_گرہ_Type(nodep);
            واپس;
        }
    }

    خالی دورہ(AstScopeName* /*nodep*/) override {
        اگر (jumpingOver()) واپس;
        // Ignore
    }

    خالی دورہ(AstSFormatArg* nodep) override {
        check_گرہ_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_اگلا_Const_Null(nodep->exprp());
    }
    خالی دورہ(AstSFormatF* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;

        Ast_گرہ* اگلا_Argp = nodep->exprsp();
        string نتیجہ;
        ثابت string format = nodep->متن();
        خودکار pos = format.cbegin();
        bool اندر_Pct = false;
        string چوڑائی;
        برائے (; pos != format.cend(); ++pos) {
            اگر (!اندر_Pct && pos[0] == '%') {
                اندر_Pct = true;
                چوڑائی = "";
            } ورنہ اگر (!اندر_Pct) {  // Normal text
                نتیجہ += *pos;
            } ورنہ {  // Format character
                اگر (std::ہندسہ_ہے(pos[0])) {
                    چوڑائی += pos[0];
                    جاری;
                }

                اندر_Pct = false;

                اگر (V3_نمبر::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_گرہ* ثابت argp = اگلا_Argp;
                    اگلا_Argp = اگلا_Argp->nextp();
                    AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                    Ast_گرہ* ثابت subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ثابت constp = fetchConstNull(subargp);
                    ثابت VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    اگر (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        توڑو;
                    }
                    ثابت string pformat = "%"s + چوڑائی + pos[0];
                    نتیجہ += formatAttr.isEnum()
                                  ? constp->عدد().displayedEnum(fargp, pformat)
                                  : constp->عدد().displayed(nodep, pformat, formatAttr);
                } ورنہ {
                    انتخاب (std::tolower(pos[0])) {
                    حالت '%': نتیجہ += "%"; توڑو;
                    حالت 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        نتیجہ += "%m";
                        توڑو;
                    طےشدہ: clearOptimizable(nodep, "Unknown $display-like format code."); توڑو;
                    }
                }
            }
        }

        AstConst* ثابت نتیجہ_Constp = new AstConst{nodep->fileline(), AstConst::String{}, نتیجہ};
        سیٹ_قدر(nodep, نتیجہ_Constp);
        m_reclaimValuesp.ڈالو_عقب(نتیجہ_Constp);
    }

    خالی دورہ(AstDisplay* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!optimizable()) واپس;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_گرہ_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        اگر (m_params) {
            ثابت AstConst* ثابت textp = fetchConst(nodep->fmtp());
            انتخاب (nodep->displayType()) {
            حالت VDisplayType::DT_DISPLAY:  // FALLTHRU
            حالت VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); توڑو;
            حالت VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); توڑو;
            حالت VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); توڑو;
            حالت VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); توڑو;
            حالت VDisplayType::DT_WRITE:  // FALLTHRU
            طےشدہ: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    خالی دورہ(Ast_مقصد_String_N* nodep) override {
        اگر (jumpingOver()) واپس;
        اگر (!optimizable()) واپس;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        اگر (m_checkOnly || !optimizable()) واپس;
        ثابت std::string نتیجہ = مقصد_String_Recurse(fetch_قدر(nodep->lhsp()));
        اگر (!optimizable()) واپس;
        AstConst* ثابت نتیجہ_Constp = new AstConst{nodep->fileline(), AstConst::String{}, نتیجہ};
        سیٹ_قدر(nodep, نتیجہ_Constp);
        m_reclaimValuesp.ڈالو_عقب(نتیجہ_Constp);
    }

    خالی دورہ(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    خالی دورہ(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        معلوم_Bad_گرہ_Type(nodep);
    }
    خالی دورہ(Ast_Get_Initial_Random_بیج* nodep) override { معلوم_Bad_گرہ_Type(nodep); }
    خالی دورہ(AstMemberSel* nodep) override { معلوم_Bad_گرہ_Type(nodep); }
    خالی دورہ(AstSampled* nodep) override { معلوم_Bad_گرہ_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (jumpingOver()) واپس;
        bad_گرہ_Type(nodep);
    }

    // MEMBERS - called by constructor
    خالی سیٹ_منوال(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    خالی mainGuts(Ast_گرہ* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

عوامی:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        سیٹ_منوال(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    خالی clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_any_تفویض_Comb = false;
        m_any_تفویض_Dly = false;
        m_اندر_Dly_تفویض = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_گنتی = 0;
        m_ڈیٹا_گنتی = 0;
        m_jumptargetp = nullptr;

        Ast_گرہ::user1_Clear_درخت();
        m_varAux.clear();
        ++m_constGeneration;
    }
    خالی main_جدول_Check(Ast_گرہ* nodep) {
        سیٹ_منوال(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    خالی main_جدول_Emulate(Ast_گرہ* nodep) {
        سیٹ_منوال(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    خالی mainParamEmulate(Ast_گرہ* nodep) {
        سیٹ_منوال(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::سیٹ<Ast_گرہ*> غیر_مستعمل_Rootps;
        برائے (Ast_گرہ* ثابت nodep : m_reclaimValuesp) {
            اگر (!nodep->backp()) غیر_مستعمل_Rootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        برائے (Ast_گرہ* ثابت nodep : غیر_مستعمل_Rootps) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
};

#اگر_ختم  // Guard
