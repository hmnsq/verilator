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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3SIMULATE_H_
#وصف_ڪريو VERILATOR_V3SIMULATE_H_

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

class Sim_ڍير_ڳنڍ final {
عام:
    // MEMBERS
    AstFuncRef* ثابت m_funcp;
    V3TaskConnects* ثابت m_tconnects;
    // CONSTRUCTORS
    Sim_ڍير_ڳنڍ(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ڍير_ڳنڍ() = افتراضي;
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
    ساكن constexpr صحيح CALL_ڍير_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ثابت VNUser1_اندريون_Use m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    تعداد VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    بنية AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_ڳنڍ_اظهار* ٻاهريون_Valuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_ڳنڍ_اظهار* valuep = nullptr;
    };

    AstUser1Allocator<Ast_ڳنڍ, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    بنية ConstAllocator final {
        سائيز_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        سائيز_t m_ايندڙ_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(سائيز_t currentGeneration, Ast_ڳنڍ* nodep) {
            إذا (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_ايندڙ_Free = 0;
            }
            UASSERT_OBJ(m_ايندڙ_Free <= m_constps.سائيز(), nodep, "Should only allocate at end");
            إذا (m_ايندڙ_Free == m_constps.سائيز()) {
                m_constps.ڌڪو_پٺ(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ثابت constp = m_constps[m_ايندڙ_Free++];
            constp->انگ().nodep(nodep);
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
    Ast_ڳنڍ* m_whyNotNodep;  ///< First node not optimizable
    bool m_any_سونپيو_Dly;  ///< True if found a delayed assignment
    bool m_any_سونپيو_Comb;  ///< True if found a non-delayed assignment
    bool m_اندريون_Dly_سونپيو;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    صحيح m_instr_ڳڻپ;  ///< Number of nodes
    صحيح m_معلومات_ڳڻپ;  ///< Bytes of data
    صحيح m_recurse_ڳڻپ = 0;  // Now deep in current recursion
    Ast_ڳنڍ* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ثابت Ast_ڳنڍ_DType*, ConstAllocator> m_constps;
    سائيز_t m_constGeneration = 0;
    std::ويڪٽر<Sim_ڍير_ڳنڍ*> m_call_ڍير;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::ويڪٽر<Ast_ڳنڍ*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_عدد(ثابت V3_عدد* nump, ثابت Ast_ڳنڍ_DType* dtypep) {
        إذا (ثابت AstRefDType* ثابت refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت stp = VN_CAST(dtypep, ڳنڍ_UOr_Struct_DType)) {
            إذا (stp->packed()) {
                std::ostringstream ٻاهريون;
                ٻاهريون << "'{";
                لكل (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت صحيح ويڪر = itemp->ويڪر();
                    ثابت صحيح lsb = itemp->lsb();
                    ثابت صحيح msb = lsb + ويڪر - 1;
                    V3_عدد ميدان_انگ{nump, ويڪر};
                    ميدان_انگ.opSel(*nump, msb, lsb);
                    ٻاهريون << itemp->name() << ": ";
                    إذا (ثابت Ast_ڳنڍ_DType* ثابت ٻار_Typep = itemp->subDTypep()) {
                        ٻاهريون << pretty_عدد(&ميدان_انگ, ٻار_Typep);
                    } وإلا {
                        ٻاهريون << ميدان_انگ;
                    }
                    إذا (itemp->nextp()) ٻاهريون << ", ";
                }
                ٻاهريون << "}";
                أرجع ٻاهريون.str();
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            إذا (ثابت Ast_ڳنڍ_DType* ثابت ٻار_Typep = arrayp->subDTypep()) {
                std::ostringstream ٻاهريون;
                ٻاهريون << "[";
                ثابت صحيح arrayElements = arrayp->elementsConst();
                لكل (صحيح element = 0; element < arrayElements; ++element) {
                    ثابت صحيح ويڪر = ٻار_Typep->ويڪر();
                    ثابت صحيح lsb = ويڪر * element;
                    ثابت صحيح msb = lsb + ويڪر - 1;
                    V3_عدد ميدان_انگ{nump, ويڪر};
                    ميدان_انگ.opSel(*nump, msb, lsb);
                    ثابت صحيح arrayElem = arrayp->lo() + element;
                    ٻاهريون << arrayElem << " = " << pretty_عدد(&ميدان_انگ, ٻار_Typep);
                    إذا (element < arrayElements - 1) ٻاهريون << ", ";
                }
                ٻاهريون << "]";
                أرجع ٻاهريون.str();
            }
        }
        أرجع nump->ascii();
    }

    // Checking METHODS
عام:
    /// Call other-this function on all new *non-constant* var references
    مجازي فراغ varRefCb(AstVarRef* nodep) {}

    فراغ clearOptimizable(Ast_ڳنڍ* nodep /*null ok*/, ثابت string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        إذا (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            إذا (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_اڳياڙي("Clear optimizable: " << why);
                إذا (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ڍير;
            صحيح n = 0;
            لكل (ثابت خودڪار& callstack : vlstd::ابتو_view(m_call_ڍير)) {
                ثابت AstFuncRef* ثابت funcp = callstack->m_funcp;
                ڍير << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                لكل (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->پڇاڙي(); ++conIt) {
                    ثابت AstVar* ثابت portp = conIt->پهريون;
                    Ast_ڳنڍ_اظهار* ثابت pinp = conIt->ٻيو->exprp();
                    ثابت Ast_ڳنڍ_DType* ثابت dtypep = pinp->dtypep();
                    إذا (AstConst* ثابت valp = fetchConstNull(pinp)) {
                        ڍير << "\n           " << portp->prettyName() << " = "
                              << pretty_عدد(&valp->انگ(), dtypep);
                    } وإلا إذا (Ast_ڳنڍ_اظهار* ثابت valuep = fetch_قدر_Null(pinp)) {
                        ڍير << "\n           " << portp->prettyName() << " = "
                              << منزل_String_Recurse(valuep);
                    }
                }
                إذا (++n > CALL_ڍير_MAX) {
                    ڍير << "\n           ... stack truncated";
                    اكسر;
                }
            }
            m_whyNotOptimizable += ڍير.str();
        }
    }
    bool optimizable() ثابت { أرجع m_whyNotNodep == nullptr; }
    string why_Not_پيغام() ثابت { أرجع m_whyNotOptimizable; }
    Ast_ڳنڍ* whyNotNodep() ثابت { أرجع m_whyNotNodep; }

    bool is_سونپيو_Dly() ثابت { أرجع m_any_سونپيو_Dly; }
    bool isImpure() ثابت { أرجع m_isImpure; }
    bool isCoverage() ثابت { أرجع m_isCoverage; }
    صحيح instr_ڳڻپ() ثابت { أرجع m_instr_ڳڻپ; }
    صحيح معلومات_ڳڻپ() ثابت { أرجع m_معلومات_ڳڻپ; }

    // Simulation METHODS
خاص:
    AstConst* allocConst(Ast_ڳنڍ* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        أرجع m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

عام:
    فراغ new_قدر(Ast_ڳنڍ* nodep, ثابت Ast_ڳنڍ_اظهار* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->انگ().op_سونپيو(constp->انگ());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } وإلا إذا (fetch_قدر_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            سيٽ_قدر(nodep, newTrackedClone(const_cast<Ast_ڳنڍ_اظهار*>(valuep)));
        }
    }
    فراغ new_ٻاهريون_قدر(Ast_ڳنڍ* nodep, ثابت Ast_ڳنڍ_اظهار* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            new_ٻاهريون_Const(nodep)->انگ().op_سونپيو(constp->انگ());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } وإلا إذا (fetch_ٻاهريون_قدر_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            سيٽ_ٻاهريون_قدر(nodep, newTrackedClone(const_cast<Ast_ڳنڍ_اظهار*>(valuep)));
        }
    }

خاص:
    Ast_ڳنڍ_اظهار* newTrackedClone(Ast_ڳنڍ_اظهار* nodep) {
        Ast_ڳنڍ_اظهار* ثابت newp = nodep->clone_وڻ(false);
        m_reclaimValuesp.ڌڪو_پٺ(newp);
        أرجع newp;
    }
    AstConst* newConst(Ast_ڳنڍ* nodep) {
        // Set a constant value for this node
        إذا (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            أرجع constp;
        }
        أرجع fetchConst(nodep);
    }
    AstConst* new_ٻاهريون_Const(Ast_ڳنڍ* nodep) {
        // Set a var-output constant value for this node
        إذا (!VN_IS(m_varAux(nodep).ٻاهريون_Valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).ٻاهريون_Valuep = constp;
            أرجع constp;
        }
        أرجع fetch_ٻاهريون_Const(nodep);
    }

عام:
    Ast_ڳنڍ_اظهار* fetch_قدر_Null(Ast_ڳنڍ* nodep) { أرجع m_varAux(nodep).valuep; }

خاص:
    Ast_ڳنڍ_اظهار* fetch_ٻاهريون_قدر_Null(Ast_ڳنڍ* nodep) { أرجع m_varAux(nodep).ٻاهريون_Valuep; }
    AstConst* fetchConstNull(Ast_ڳنڍ* nodep) { أرجع VN_CAST(fetch_قدر_Null(nodep), Const); }
    AstConst* fetch_ٻاهريون_Const_Null(Ast_ڳنڍ* nodep) {
        أرجع VN_CAST(fetch_ٻاهريون_قدر_Null(nodep), Const);
    }
    Ast_ڳنڍ_اظهار* fetch_قدر(Ast_ڳنڍ* nodep) {
        Ast_ڳنڍ_اظهار* ثابت valuep = fetch_قدر_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        أرجع valuep;
    }
    AstConst* fetchConst(Ast_ڳنڍ* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        أرجع constp;
    }
    AstConst* fetch_ٻاهريون_Const(Ast_ڳنڍ* nodep) {
        AstConst* ثابت constp = fetch_ٻاهريون_Const_Null(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        أرجع constp;
    }

عام:
    V3_عدد* fetch_عدد_Null(Ast_ڳنڍ* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        إذا (constp) أرجع &constp->انگ();
        أرجع nullptr;
    }
    V3_عدد* fetch_ٻاهريون_عدد_Null(Ast_ڳنڍ* nodep) {
        AstConst* ثابت constp = fetch_ٻاهريون_Const_Null(nodep);
        إذا (constp) أرجع &constp->انگ();
        أرجع nullptr;
    }

خاص:
    فراغ سيٽ_قدر(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    فراغ سيٽ_ٻاهريون_قدر(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).ٻاهريون_Valuep = valuep;
    }

    فراغ check_ڳنڍ_Info(Ast_ڳنڍ* nodep, bool ignorePredict = false) {
        إذا (m_checkOnly) {
            m_instr_ڳڻپ += nodep->instr_ڳڻپ();
            m_معلومات_ڳڻپ += nodep->ويڪر();
        }
        إذا (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        إذا (!nodep->is_خالص()) m_isImpure = true;
    }

    فراغ معلوم_Bad_ڳنڍ_Type(Ast_ڳنڍ* nodep) {
        // Call for node types we know we can't handle
        إذا (jumpingOver()) أرجع;
        check_ڳنڍ_Info(nodep);
        إذا (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    فراغ bad_ڳنڍ_Type(Ast_ڳنڍ* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_ڳنڍ_Info(nodep);
        إذا (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#جيڪڏهن_وصف_ٿيل VL_DEBUG
            ساكن std::سيٽ<VNType> s_typePrinted;
            ثابت خودڪار pair = s_typePrinted.emplace(nodep->type());
            إذا (pair.ٻيو)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#جيڪڏهن_پورو
        }
    }

    Ast_ڳنڍ* varOrScope(AstVarRef* nodep) ثابت {
        Ast_ڳنڍ* vscp;
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
    فراغ سونپيو_ٻاهريون_قدر(ثابت Ast_ڳنڍ_سونپيو* nodep, Ast_ڳنڍ* vscp, ثابت Ast_ڳنڍ_اظهار* valuep) {
        إذا (VN_IS(nodep, سونپيو_Dly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_ٻاهريون_قدر(vscp, valuep);
        } وإلا {
            new_قدر(vscp, valuep);
            سيٽ_ٻاهريون_قدر(vscp, fetch_قدر(vscp));
        }
    }

    string منزل_String_Recurse(Ast_ڳنڍ_اظهار* valuep) {
        // Return string representation of a value, or clearOptimizable
        إذا (ثابت AstConst* ثابت avaluep = VN_CAST(valuep, Const)) {
            أرجع "'h"s + avaluep->انگ().displayed(valuep, "%0x");
        }
        إذا (ثابت AstConsArray* ثابت avaluep = VN_CAST(valuep, ConsArray)) {
            string نتيجو = "'{";
            string comma;
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            لكل (صحيح n = 0; n < dtypep->elementsConst(); ++n) {
                نتيجو += comma + منزل_String_Recurse(avaluep->valuep(n));
                comma = ", ";
            }
            نتيجو += "}";
            أرجع نتيجو;
        }
        إذا (ثابت AstConsStruct* ثابت avaluep = VN_CAST(valuep, ConsStruct)) {
            string نتيجو = "'{";
            string comma;
            لكل (ثابت Ast_Cons_Struct_شي* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_شي)) {
                نتيجو
                    += comma + itemp->memberp()->name() + ":" + منزل_String_Recurse(itemp->valuep());
                comma = ", ";
            }
            نتيجو += "}";
            أرجع نتيجو;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        أرجع "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_ڳنڍ_اظهار* newInitValuep(Ast_ڳنڍ* nodep, Ast_ڳنڍ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_ڳنڍ_اظهار* ثابت elementp = newInitValuep(nodep, arrayp->subDTypep());
            أرجع elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        إذا (structp && !structp->packed()) {
            AstConsStruct* ثابت newp = new AstConsStruct{nodep->fileline(), structp};
            لكل (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_ڳنڍ_اظهار* ثابت memberValuep = newInitValuep(nodep, memberp->subDTypep());
                إذا (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_وڻ(), newp);
                    أرجع nullptr;
                }
                newp->جوڙيو_Itemsp(new Ast_Cons_Struct_شي{nodep->fileline(), memberp, memberValuep});
            }
            أرجع newp;
        }
        إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
            AstConst* ثابت constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, basicp->ويڪر_Min(), 0};
            إذا (basicp->isZeroInit()) {
                constp->انگ() = V3_عدد{nodep, basicp};
            } وإلا {
                constp->انگ().سيٽ_All_بٽ_X();
            }
            أرجع constp;
        }
        أرجع nullptr;
    }
    فراغ initVar(AstVar* nodep) {
        إذا (Ast_ڳنڍ_اظهار* ثابت valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_قدر(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_وڻ(), valuep);
        }
    }

    // VISITORS
    فراغ گهمو(AstAlways* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_Sen_وڻ* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    فراغ گهمو(AstVarRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_ڳنڍ* ثابت vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        إذا (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), ڳنڍ_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        إذا (nodep->access().isWriteOrRW()) {
            إذا (m_اندريون_Dly_سونپيو) {
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
                ثابت Ast_ڳنڍ_اظهار* ثابت valuep
                    = varIsConst ? fetch_قدر_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                إذا (varIsConst && valuep) {
                    إذا (!m_checkOnly && optimizable()) new_قدر(vscp, valuep);
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
            Ast_ڳنڍ_اظهار* valuep = fetch_قدر_Null(vscp);
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
            سيٽ_قدر(nodep, valuep);
        }
    }
    فراغ گهمو(AstVarXRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (m_scoped) {
            bad_ڳنڍ_Type(nodep);
            أرجع;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_ڳنڍ_Type(nodep);
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
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstInitialAutomaticStmt* nodep) override {
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstInitialStaticStmt* nodep) override {
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstInitialStatic* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_ڳنڍ_Type(nodep);
            أرجع;
        }
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   IF " << nodep);
        check_ڳنڍ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterate_And_ايندڙ_Const_Null(nodep->condp());
            إذا (optimizable()) {
                إذا (fetchConst(nodep->condp())->انگ().isNeqZero()) {
                    iterate_And_ايندڙ_Const_Null(nodep->thensp());
                } وإلا {
                    iterate_And_ايندڙ_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    فراغ گهمو(AstConst* nodep) override {
        check_ڳنڍ_Info(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        new_قدر(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    فراغ گهمو(AstConsArray* nodep) override {
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsArray* ثابت valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_قدر(nodep->defaultp())->clone_وڻ(false) : nullptr};
        لكل (ثابت Ast_Cons_Array_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_شي)) {
            valuep->valuep(itemp->اشاريو(), fetch_قدر(itemp->valuep())->clone_وڻ(false));
        }
        m_reclaimValuesp.ڌڪو_پٺ(valuep);
        سيٽ_قدر(nodep, valuep);
    }
    فراغ گهمو(AstConsStruct* nodep) override {
        إذا (!m_params) {
            bad_ڳنڍ_Type(nodep);
            أرجع;
        }
        check_ڳنڍ_Info(nodep);
        لكل (Ast_Cons_Struct_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_شي)) {
            iterateConst(itemp->valuep());
        }
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsStruct* ثابت valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        لكل (Ast_Cons_Struct_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_شي)) {
            valuep->جوڙيو_Itemsp(
                new Ast_Cons_Struct_شي{itemp->fileline(), itemp->memberp(),
                                      fetch_قدر(itemp->valuep())->clone_وڻ(false)});
        }
        m_reclaimValuesp.ڌڪو_پٺ(valuep);
        سيٽ_قدر(nodep, valuep);
    }
    فراغ گهمو(Ast_Cvt_Unpack_منزل_Unpack* nodep) override {
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_ڳنڍ_اظهار* ثابت cvtp = new Ast_Cvt_Unpack_منزل_Unpack{
            nodep->fileline(), fetch_قدر(nodep->fromp())->clone_وڻ(false), nodep->dtypep()};
        Ast_ڳنڍ_اظهار* ثابت valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ڌڪو_پٺ(valuep);
        سيٽ_قدر(nodep, valuep);
    }
    فراغ گهمو(Ast_Cons_Array_شي* nodep) override {
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_Enum_شي_Ref* nodep) override {
        check_ڳنڍ_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        إذا (m_checkOnly || !optimizable()) أرجع;
        Ast_ڳنڍ* ثابت valuep = nodep->itemp()->valuep();
        إذا (valuep) {
            iterate_And_ايندڙ_Const_Null(valuep);
            إذا (!optimizable()) أرجع;
            new_قدر(nodep, fetch_قدر(valuep));
        } وإلا {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Uniop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->عدد_هلايو(newConst(nodep)->انگ(), fetchConst(nodep->lhsp())->انگ());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    فراغ aggregateEqRecurse(V3_عدد& resultr, Ast_ڳنڍ* nodep, bool caseEq, Ast_ڳنڍ_اظهار* lhsp,
                            Ast_ڳنڍ_اظهار* rhsp) {
        إذا (ثابت AstConst* ثابت lconstp = VN_CAST(lhsp, Const)) {
            ثابت V3_عدد& lnum = lconstp->انگ();
            ثابت V3_عدد& rnum = VN_AS(rhsp, Const)->انگ();
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
            ثابت Ast_Cons_Struct_شي* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            لكل (ثابت Ast_Cons_Struct_شي* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), Cons_Struct_شي),
                                          ritemp = VN_AS(ritemp->nextp(), Cons_Struct_شي)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    فراغ handleAggregateCompare(Ast_ڳنڍ_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ثابت bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ثابت bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_عدد& resultr = newConst(nodep)->انگ();
        resultr.سيٽ_All_Bits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_قدر(nodep->lhsp()),
                           fetch_قدر(nodep->rhsp()));
        إذا (neq) {
            ثابت V3_عدد eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Biop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        إذا (!VN_IS(fetch_قدر(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            أرجع;
        }
        AstConst* ثابت valuep = newConst(nodep);
        nodep->عدد_هلايو(newConst(nodep)->انگ(), fetchConst(nodep->lhsp())->انگ(),
                             fetchConst(nodep->rhsp())->انگ());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        إذا (!m_params && VN_IS(nodep, Sel) && valuep->انگ().isAnyX()) {
            ثابت V3_عدد انگ{valuep, valuep->ويڪر(), valuep->انگ()};
            valuep->انگ().op_بٽ_One(انگ);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Triop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->عدد_هلايو(newConst(nodep)->انگ(), fetchConst(nodep->lhsp())->انگ(),
                             fetchConst(nodep->rhsp())->انگ(), fetchConst(nodep->thsp())->انگ());
    }
    فراغ گهمو(Ast_ڳنڍ_Quadop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->عدد_هلايو(newConst(nodep)->انگ(), fetchConst(nodep->lhsp())->انگ(),
                             fetchConst(nodep->rhsp())->انگ(), fetchConst(nodep->thsp())->انگ(),
                             fetchConst(nodep->fhsp())->انگ());
    }
    فراغ گهمو(AstLogAnd* nodep) override {
        // Need to short circuit
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->انگ().isNeqZero()) {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            } وإلا {
                new_قدر(nodep, fetch_قدر(nodep->lhsp()));  // a zero
            }
        }
    }
    فراغ گهمو(AstLogOr* nodep) override {
        // Need to short circuit
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->انگ().isNeqZero()) {
                new_قدر(nodep, fetch_قدر(nodep->lhsp()));  // a one
            } وإلا {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            }
        }
    }
    فراغ گهمو(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->انگ().isEqZero()) {
                ثابت AstConst cnst{nodep->fileline(), AstConst::Widthed_قدر{}, 1, 1};  // a one
                new_قدر(nodep, &cnst);  // a one
            } وإلا {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            }
        }
    }
    فراغ گهمو(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->condp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->condp())->انگ().isNeqZero()) {
                iterateConst(nodep->thenp());
                إذا (!optimizable()) أرجع;
                new_قدر(nodep, fetch_قدر(nodep->thenp()));
            } وإلا {
                iterateConst(nodep->elsep());
                إذا (!optimizable()) أرجع;
                new_قدر(nodep, fetch_قدر(nodep->elsep()));
            }
        }
    }

    فراغ handle_سونپيو_Element(Ast_ڳنڍ_سونپيو* nodep, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* قدر_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> selps;
        Ast_ڳنڍ_اظهار* fromp = lhsp;
        طالما (true) {
            إذا (AstArraySel* ثابت selp = VN_CAST(fromp, ArraySel)) {
                check_ڳنڍ_Info(selp);
                iterate_And_ايندڙ_Const_Null(selp->bitp());  // Bit index
                selps.ڌڪو_پٺ(selp);
                fromp = selp->fromp();
            } وإلا إذا (AstStructSel* ثابت selp = VN_CAST(fromp, StructSel)) {
                check_ڳنڍ_Info(selp);
                selps.ڌڪو_پٺ(selp);
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
        Ast_ڳنڍ* ثابت vscp = varOrScope(varrefp);
        Ast_ڳنڍ_اظهار* rootp = fetch_ٻاهريون_قدر_Null(vscp);
        إذا (!rootp) rootp = fetch_قدر_Null(vscp);
        إذا (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            أرجع;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_ڳنڍ_اظهار* itemp = rootp;
        لكل (Ast_ڳنڍ_اظهار* ثابت selp : vlstd::ابتو_view(selps)) {
            إذا (ثابت AstArraySel* ثابت aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ثابت consp = VN_AS(itemp, ConsArray);
                ثابت uint32_t اشاريو = fetchConst(aselp->bitp())->منزل_UInt();
                ثابت صحيح elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                إذا (اشاريو >= static_cast<uint32_t>(elements)) {
                    أرجع;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(اشاريو);
                إذا (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_وڻ(false);
                    consp->valuep(اشاريو, itemp);
                }
            } وإلا {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_ڳنڍ_اظهار* ثابت valuep = newTrackedClone(fetch_قدر(قدر_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ڌڪو_پٺ(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        سونپيو_ٻاهريون_قدر(nodep, vscp, rootp);
    }
    فراغ handle_سونپيو_Sel(Ast_ڳنڍ_سونپيو* nodep, AstSel* selp, Ast_ڳنڍ_اظهار* قدر_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_عدد lsb{nodep};
        handle_سونپيو_Sel_Recurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        إذا (m_checkOnly || !optimizable()) أرجع;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_ڳنڍ* ثابت vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        إذا (AstConst* ثابت vscpnump = fetch_ٻاهريون_Const_Null(vscp)) {
            outconstp = vscpnump;
        } وإلا إذا (AstConst* ثابت vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } وإلا {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_قدر{},
                                     varrefp->varp()->ويڪر_Min(), 0};
            إذا (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->انگ().سيٽ_All_Bits0();
            } وإلا {
                outconstp->انگ().سيٽ_All_بٽ_X();
            }
            m_reclaimValuesp.emplace_پٺ(outconstp);
        }
        outconstp->انگ().opSelInto(fetchConst(قدر_Fromp)->انگ(), lsb, selp->ويڪر_Const());
        سونپيو_ٻاهريون_قدر(nodep, vscp, outconstp);
    }
    فراغ handle_سونپيو_Sel_Recurse(Ast_ڳنڍ_سونپيو* nodep, AstSel* selp, AstVarRef*& ٻاهريون_Varrefp_Ref,
                                V3_عدد& lsbRef, صحيح اونهائي) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_ڳنڍ_Info(selp);
        iterate_And_ايندڙ_Const_Null(selp->lsbp());  // Bit index
        إذا (AstVarRef* ثابت varrefp = VN_CAST(selp->fromp(), VarRef)) {
            ٻاهريون_Varrefp_Ref = varrefp;
            lsbRef = fetchConst(selp->lsbp())->انگ();
            أرجع;  // And presumably still optimizable()
        }
        إذا (AstSel* ثابت subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_عدد sublsb{nodep};
            handle_سونپيو_Sel_Recurse(nodep, subselp, ٻاهريون_Varrefp_Ref, sublsb /*ref*/, اونهائي + 1);
            إذا (optimizable()) {
                lsbRef = sublsb;
                lsbRef.op_جوڙيو(sublsb, fetchConst(selp->lsbp())->انگ());
            }
        } وإلا {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    فراغ handle_سونپيو_Recurse(Ast_ڳنڍ_سونپيو* nodep, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* قدر_Fromp) {
        إذا (!optimizable()) أرجع;
        إذا (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            إذا (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                أرجع;
            }
            handle_سونپيو_Element(nodep, lhsp, قدر_Fromp);
        } وإلا إذا (AstConcat* ثابت selp = VN_CAST(lhsp, Concat)) {
            check_ڳنڍ_Info(selp);
            إذا (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                أرجع;
            }
            // Split value into left and right concat values
            إذا (!m_checkOnly) {
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_قدر{},
                                       selp->lhsp()->ويڪر(), 0};
                    outconstp->انگ().opSel(fetchConst(قدر_Fromp)->انگ(),
                                           selp->lhsp()->ويڪر() + selp->rhsp()->ويڪر() + 1,
                                           selp->rhsp()->ويڪر());
                    new_قدر(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_وڻ(), outconstp);
                }
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_قدر{},
                                       selp->rhsp()->ويڪر_Min(), 0};
                    outconstp->انگ().opSel(fetchConst(قدر_Fromp)->انگ(),
                                           selp->rhsp()->ويڪر() - 1, 0);
                    new_قدر(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_وڻ(), outconstp);
                }
            }
            handle_سونپيو_Recurse(nodep, selp->lhsp(), selp->lhsp());
            handle_سونپيو_Recurse(nodep, selp->rhsp(), selp->rhsp());
        } وإلا إذا (AstReplicate* ثابت selp = VN_CAST(lhsp, Replicate)) {
            check_ڳنڍ_Info(selp);
            iterate_And_ايندڙ_Const_Null(selp->countp());
            AstConst* ثابت countp = VN_CAST(selp->countp(), Const);
            إذا (!countp || !countp->انگ().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                أرجع;
            }
            handle_سونپيو_Recurse(nodep, selp->srcp(), قدر_Fromp);
        } وإلا إذا (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            إذا (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                أرجع;
            }
            handle_سونپيو_Sel(nodep, selp, قدر_Fromp);
        } وإلا إذا (VN_IS(lhsp, VarRef)) {
            إذا (m_checkOnly) {
                iterate_And_ايندڙ_Const_Null(lhsp);
            } وإلا {
                Ast_ڳنڍ* ثابت vscp = varOrScope(VN_CAST(lhsp, VarRef));
                سونپيو_ٻاهريون_قدر(nodep, vscp, fetch_قدر(قدر_Fromp));
            }
        } وإلا {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);

        VL_RESTORER(m_اندريون_Dly_سونپيو);

        إذا (VN_IS(nodep, سونپيو_Force)) {
            clearOptimizable(nodep, "Force");
        } وإلا إذا (VN_IS(nodep, سونپيو_Dly)) {
            إذا (m_any_سونپيو_Comb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_سونپيو_Dly = true;
            m_اندريون_Dly_سونپيو = true;
        } وإلا {
            إذا (m_any_سونپيو_Dly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_سونپيو_Comb = true;
        }

        إذا (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            أرجع;
        }

        iterate_And_ايندڙ_Const_Null(nodep->rhsp());  // Value to assign
        handle_سونپيو_Recurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    فراغ گهمو(AstArraySel* nodep) override {
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_قدر_Null(nodep->fromp()), ConsArray)) {
            ثابت AstConst* ثابت indexp = fetchConst(nodep->bitp());
            ثابت uint32_t وٿي = indexp->انگ().منزل_UInt();
            Ast_ڳنڍ_اظهار* ثابت itemp = consp->valuep(وٿي);
            إذا (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::منزل_string(وٿي));
            } وإلا {
                سيٽ_قدر(nodep, itemp);
            }
        } وإلا {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    فراغ گهمو(AstStructSel* nodep) override {
        إذا (!m_params) {
            bad_ڳنڍ_Type(nodep);
            أرجع;
        }
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsStruct* ثابت consp = VN_AS(fetch_قدر(nodep->fromp()), ConsStruct);
        سيٽ_قدر(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    فراغ گهمو(Ast_ٽڪرو_Sel* nodep) override {
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // Fetch the base constant array
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_قدر_Null(nodep->fromp()), ConsArray)) {
            ثابت VNumRange& ٽڪرو_Range = nodep->declRange();
            ثابت uint32_t ٽڪرو_Elements = ٽڪرو_Range.elements();
            ثابت صحيح ٽڪرو_Lo = ٽڪرو_Range.lo();
            // Use this node's dtype for the slice array
            Ast_ڳنڍ_DType* ثابت dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ثابت newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            لكل (uint32_t idx = 0; idx < ٽڪرو_Elements; ++idx) {
                ثابت uint32_t بنياد_Idx = ٽڪرو_Lo + idx;
                newConsp->valuep(idx, consp->valuep(بنياد_Idx)->clone_وڻ(false));
            }
            // Assign the new constant array and track it for later deletion
            سيٽ_قدر(nodep, newConsp);
            m_reclaimValuesp.ڌڪو_پٺ(newConsp);
        } وإلا {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    فراغ گهمو(AstBegin* nodep) override {
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstCase* nodep) override {
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   CASE " << nodep);
        check_ڳنڍ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا إذا (optimizable()) {
            iterate_And_ايندڙ_Const_Null(nodep->exprp());
            bool ڪاميابي = false;
            لكل (Ast_Case_شي* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_شي)) {
                إذا (!itemp->isDefault()) {
                    لكل (Ast_ڳنڍ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        إذا (ڪاميابي) اكسر;
                        iterate_And_ايندڙ_Const_Null(ep);
                        إذا (optimizable()) {
                            V3_عدد ميلاپ{nodep, 1};
                            ميلاپ.opEq(fetchConst(nodep->exprp())->انگ(), fetchConst(ep)->انگ());
                            إذا (ميلاپ.isNeqZero()) {
                                iterate_And_ايندڙ_Const_Null(itemp->stmtsp());
                                ڪاميابي = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            لكل (Ast_Case_شي* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_شي)) {
                إذا (ڪاميابي) اكسر;
                إذا (itemp->isDefault()) {
                    iterate_And_ايندڙ_Const_Null(itemp->stmtsp());
                    ڪاميابي = true;
                }
            }
        }
    }

    فراغ گهمو(Ast_Case_شي* nodep) override {
        // Real handling is in AstNodeCase
        إذا (jumpingOver()) أرجع;
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }

    فراغ گهمو(AstComment*) override {}

    فراغ گهمو(Ast_Stmt_اظهار* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_اظهار_Stmt* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_ڳنڍ_Info(nodep);
        iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
        إذا (!optimizable()) أرجع;
        iterate_And_ايندڙ_Const_Null(nodep->resultp());
        إذا (m_checkOnly || !optimizable()) أرجع;
        new_قدر(nodep, fetch_قدر(nodep->resultp()));
    }

    فراغ گهمو(Ast_Jump_بلاڪ* nodep) override {
        إذا (jumpingOver()) أرجع;
        iterateChildrenConst(nodep);
        إذا (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    فراغ گهمو(AstJumpGo* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_ڳنڍ_Info(nodep);
        إذا (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    فراغ گهمو(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        إذا (!m_params) {
            bad_ڳنڍ_Type(nodep);
            أرجع;
        }
        check_ڳنڍ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
            أرجع;
        }
        إذا (!optimizable()) أرجع;

        سائيز_t iter_ڳڻپ = 0;
        ثابت سائيز_t iter_حد_بندي = v3Global.opt.unroll_حد_بندي();
        طالما (true) {
            إذا (iter_ڳڻپ > iter_حد_بندي) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::منزل_string(iter_حد_بندي));
                اكسر;
            }
            ++iter_ڳڻپ;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_ايندڙ_Const_Null(nodep->stmtsp());
            إذا (jumpingOver()) اكسر;
        }

        إذا (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    فراغ گهمو(AstLoopTest* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_ڳنڍ_Info(nodep);
        iterateConst(nodep->condp());
        إذا (m_checkOnly || !optimizable()) أرجع;
        إذا (fetchConst(nodep->condp())->انگ().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    فراغ گهمو(AstStop* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_ڳنڍ_Info(nodep);
    }
    فراغ گهمو(AstFuncRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_ڳنڍ_Info(nodep);
        إذا (!m_params) {
            bad_ڳنڍ_Type(nodep);
            أرجع;
        }
        Ast_ڳنڍ_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        إذا (m_params) V3_ويڪر::ويڪر_Params_Edit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        إذا (funcp->recursive()) {
            إذا (m_recurse_ڳڻپ >= v3Global.opt.func_Recursion_اونهائي()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::منزل_string(v3Global.opt.func_Recursion_اونهائي())
                                            + " times");
                أرجع;
            }
            ++m_recurse_ڳڻپ;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_ڳنڍ*, Ast_ڳنڍ_اظهار*> old_قدرون;

        إذا (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_قدرون](AstVar* varp) {
                إذا (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    إذا (Ast_ڳنڍ_اظهار* ثابت valuep = fetch_قدر_Null(varp)) {
                        Ast_ڳنڍ_اظهار* ثابت nvaluep = newTrackedClone(valuep);
                        old_قدرون.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_قدرون](Ast_ڳنڍ_اظهار* exprp) {
                إذا (VN_IS(exprp, Const)) أرجع;  // Speed up as won't change
                إذا (Ast_ڳنڍ_اظهار* ثابت valuep = fetch_قدر_Null(exprp)) {
                    Ast_ڳنڍ_اظهار* ثابت nvaluep = newTrackedClone(valuep);
                    old_قدرون.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::ويڪٽر<std::pair<AstVar*, Ast_ڳنڍ_اظهار*>> پورٽ_قدرون;
        لكل (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.پڇاڙي(); ++it) {
            AstVar* ثابت portp = it->پهريون;
            Ast_ڳنڍ* ثابت pinp = it->ٻيو->exprp();
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
                پورٽ_قدرون.emplace_پٺ(std::make_pair(portp, newTrackedClone(fetch_قدر(pinp))));
            }
        }
        // Apply value to the function
        إذا (!m_checkOnly && optimizable())
            لكل (ثابت خودڪار& it : پورٽ_قدرون) {
                إذا (!m_checkOnly && optimizable()) new_قدر(it.پهريون, it.ٻيو);
            }
        Sim_ڍير_ڳنڍ ڍير_ڳنڍ{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ڍير.ڌڪو_پٺ(&ڍير_ڳنڍ);
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
        m_call_ڍير.ڪڍو_پٺ();
        ثابت Ast_ڳنڍ_اظهار* returnp = nullptr;
        إذا (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_قدر(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        لكل (ثابت خودڪار& it : old_قدرون) {
            إذا (it.ٻيو) new_قدر(it.پهريون, it.ٻيو);
        }
        إذا (returnp) new_قدر(nodep, returnp);
        إذا (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_ڳڻپ > 0, nodep, "recurse underflow");
            --m_recurse_ڳڻپ;
        }
    }

    فراغ گهمو(AstVar* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_ڳنڍ_Type(nodep);
            أرجع;
        }
    }

    فراغ گهمو(AstScopeName* /*nodep*/) override {
        إذا (jumpingOver()) أرجع;
        // Ignore
    }

    فراغ گهمو(AstSFormatArg* nodep) override {
        check_ڳنڍ_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_ايندڙ_Const_Null(nodep->exprp());
    }
    فراغ گهمو(AstSFormatF* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;

        Ast_ڳنڍ* ايندڙ_Argp = nodep->exprsp();
        string نتيجو;
        ثابت string format = nodep->متن();
        خودڪار pos = format.cbegin();
        bool اندريون_Pct = false;
        string ويڪر;
        لكل (; pos != format.cend(); ++pos) {
            إذا (!اندريون_Pct && pos[0] == '%') {
                اندريون_Pct = true;
                ويڪر = "";
            } وإلا إذا (!اندريون_Pct) {  // Normal text
                نتيجو += *pos;
            } وإلا {  // Format character
                إذا (std::انگ_آهي(pos[0])) {
                    ويڪر += pos[0];
                    استمر;
                }

                اندريون_Pct = false;

                إذا (V3_عدد::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_ڳنڍ* ثابت argp = ايندڙ_Argp;
                    ايندڙ_Argp = ايندڙ_Argp->nextp();
                    AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                    Ast_ڳنڍ* ثابت subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ثابت constp = fetchConstNull(subargp);
                    ثابت VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    إذا (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        اكسر;
                    }
                    ثابت string pformat = "%"s + ويڪر + pos[0];
                    نتيجو += formatAttr.isEnum()
                                  ? constp->انگ().displayedEnum(fargp, pformat)
                                  : constp->انگ().displayed(nodep, pformat, formatAttr);
                } وإلا {
                    بدل (std::tolower(pos[0])) {
                    حالة '%': نتيجو += "%"; اكسر;
                    حالة 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        نتيجو += "%m";
                        اكسر;
                    افتراضي: clearOptimizable(nodep, "Unknown $display-like format code."); اكسر;
                    }
                }
            }
        }

        AstConst* ثابت نتيجو_Constp = new AstConst{nodep->fileline(), AstConst::String{}, نتيجو};
        سيٽ_قدر(nodep, نتيجو_Constp);
        m_reclaimValuesp.ڌڪو_پٺ(نتيجو_Constp);
    }

    فراغ گهمو(AstDisplay* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_ڳنڍ_Info(nodep, /*display:*/ true);
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
    فراغ گهمو(Ast_منزل_String_N* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_ڳنڍ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        ثابت std::string نتيجو = منزل_String_Recurse(fetch_قدر(nodep->lhsp()));
        إذا (!optimizable()) أرجع;
        AstConst* ثابت نتيجو_Constp = new AstConst{nodep->fileline(), AstConst::String{}, نتيجو};
        سيٽ_قدر(nodep, نتيجو_Constp);
        m_reclaimValuesp.ڌڪو_پٺ(نتيجو_Constp);
    }

    فراغ گهمو(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    فراغ گهمو(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        معلوم_Bad_ڳنڍ_Type(nodep);
    }
    فراغ گهمو(Ast_Get_Initial_Random_ٻج* nodep) override { معلوم_Bad_ڳنڍ_Type(nodep); }
    فراغ گهمو(AstMemberSel* nodep) override { معلوم_Bad_ڳنڍ_Type(nodep); }
    فراغ گهمو(AstSampled* nodep) override { معلوم_Bad_ڳنڍ_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        إذا (jumpingOver()) أرجع;
        bad_ڳنڍ_Type(nodep);
    }

    // MEMBERS - called by constructor
    فراغ سيٽ_گهڻائي(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    فراغ mainGuts(Ast_ڳنڍ* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

عام:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        سيٽ_گهڻائي(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    فراغ clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_any_سونپيو_Comb = false;
        m_any_سونپيو_Dly = false;
        m_اندريون_Dly_سونپيو = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_ڳڻپ = 0;
        m_معلومات_ڳڻپ = 0;
        m_jumptargetp = nullptr;

        Ast_ڳنڍ::user1_Clear_وڻ();
        m_varAux.clear();
        ++m_constGeneration;
    }
    فراغ main_جدول_Check(Ast_ڳنڍ* nodep) {
        سيٽ_گهڻائي(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    فراغ main_جدول_Emulate(Ast_ڳنڍ* nodep) {
        سيٽ_گهڻائي(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    فراغ mainParamEmulate(Ast_ڳنڍ* nodep) {
        سيٽ_گهڻائي(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::سيٽ<Ast_ڳنڍ*> اڻ_استعمال_ٿيل_Rootps;
        لكل (Ast_ڳنڍ* ثابت nodep : m_reclaimValuesp) {
            إذا (!nodep->backp()) اڻ_استعمال_ٿيل_Rootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        لكل (Ast_ڳنڍ* ثابت nodep : اڻ_استعمال_ٿيل_Rootps) VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
};

#جيڪڏهن_پورو  // Guard
