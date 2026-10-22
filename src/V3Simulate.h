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

class Sim_ڈھیر_گرہ final {
عام:
    // MEMBERS
    AstFuncRef* ثابت m_funcp;
    V3TaskConnects* ثابت m_tconnects;
    // CONSTRUCTORS
    Sim_ڈھیر_گرہ(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_ڈھیر_گرہ() = افتراضي;
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
    ساكن constexpr صحيح CALL_ڈھیر_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    ثابت VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    تعداد VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    بنية AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_گرہ_اظہاریہ* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_گرہ_اظہاریہ* valuep = nullptr;
    };

    AstUser1Allocator<Ast_گرہ, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    بنية ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, Ast_گرہ* nodep) {
            إذا (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            إذا (m_nextFree == m_constps.size()) {
                m_constps.ڈالو_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* ثابت constp = m_constps[m_nextFree++];
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
    Ast_گرہ* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    صحيح m_instr_گنتی;  ///< Number of nodes
    صحيح m_data_گنتی;  ///< Bytes of data
    صحيح m_recurse_گنتی = 0;  // Now deep in current recursion
    Ast_گرہ* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<ثابت Ast_گرہ_DType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<Sim_ڈھیر_گرہ*> m_call_ڈھیر;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_گرہ*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_نمبر(ثابت V3_نمبر* nump, ثابت Ast_گرہ_DType* dtypep) {
        إذا (ثابت AstRefDType* ثابت refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        إذا (ثابت Ast_گرہ_UOr_Struct_DType* ثابت stp = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
            إذا (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                لكل (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    ثابت صحيح width = itemp->width();
                    ثابت صحيح lsb = itemp->lsb();
                    ثابت صحيح msb = lsb + width - 1;
                    V3_نمبر field_عدد{nump, width};
                    field_عدد.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    إذا (ثابت Ast_گرہ_DType* ثابت childTypep = itemp->subDTypep()) {
                        out << pretty_نمبر(&field_عدد, childTypep);
                    } وإلا {
                        out << field_عدد;
                    }
                    إذا (itemp->nextp()) out << ", ";
                }
                out << "}";
                أرجع out.str();
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            إذا (ثابت Ast_گرہ_DType* ثابت childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                ثابت صحيح arrayElements = arrayp->elementsConst();
                لكل (صحيح element = 0; element < arrayElements; ++element) {
                    ثابت صحيح width = childTypep->width();
                    ثابت صحيح lsb = width * element;
                    ثابت صحيح msb = lsb + width - 1;
                    V3_نمبر field_عدد{nump, width};
                    field_عدد.opSel(*nump, msb, lsb);
                    ثابت صحيح arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_نمبر(&field_عدد, childTypep);
                    إذا (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                أرجع out.str();
            }
        }
        أرجع nump->ascii();
    }

    // Checking METHODS
عام:
    /// Call other-this function on all new *non-constant* var references
    virtual فراغ varRefCb(AstVarRef* nodep) {}

    فراغ clearOptimizable(Ast_گرہ* nodep /*null ok*/, ثابت string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        إذا (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            إذا (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                إذا (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream ڈھیر;
            صحيح n = 0;
            لكل (ثابت auto& callstack : vlstd::reverse_view(m_call_ڈھیر)) {
                ثابت AstFuncRef* ثابت funcp = callstack->m_funcp;
                ڈھیر << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                لكل (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    ثابت AstVar* ثابت portp = conIt->first;
                    Ast_گرہ_اظہاریہ* ثابت pinp = conIt->second->exprp();
                    ثابت Ast_گرہ_DType* ثابت dtypep = pinp->dtypep();
                    إذا (AstConst* ثابت valp = fetchConstNull(pinp)) {
                        ڈھیر << "\n           " << portp->prettyName() << " = "
                              << pretty_نمبر(&valp->عدد(), dtypep);
                    } وإلا إذا (Ast_گرہ_اظہاریہ* ثابت valuep = fetch_قدر_Null(pinp)) {
                        ڈھیر << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                إذا (++n > CALL_ڈھیر_MAX) {
                    ڈھیر << "\n           ... stack truncated";
                    اكسر;
                }
            }
            m_whyNotOptimizable += ڈھیر.str();
        }
    }
    bool optimizable() ثابت { أرجع m_whyNotNodep == nullptr; }
    string whyNotMessage() ثابت { أرجع m_whyNotOptimizable; }
    Ast_گرہ* whyNotNodep() ثابت { أرجع m_whyNotNodep; }

    bool isAssignDly() ثابت { أرجع m_anyAssignDly; }
    bool isImpure() ثابت { أرجع m_isImpure; }
    bool isCoverage() ثابت { أرجع m_isCoverage; }
    صحيح instr_گنتی() ثابت { أرجع m_instr_گنتی; }
    صحيح data_گنتی() ثابت { أرجع m_data_گنتی; }

    // Simulation METHODS
خاص:
    AstConst* allocConst(Ast_گرہ* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        أرجع m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

عام:
    فراغ new_قدر(Ast_گرہ* nodep, ثابت Ast_گرہ_اظہاریہ* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->عدد().opAssign(constp->عدد());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } وإلا إذا (fetch_قدر_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_قدر(nodep, newTrackedClone(const_cast<Ast_گرہ_اظہاریہ*>(valuep)));
        }
    }
    فراغ new_Out_قدر(Ast_گرہ* nodep, ثابت Ast_گرہ_اظہاریہ* valuep) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->عدد().opAssign(constp->عدد());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } وإلا إذا (fetch_Out_قدر_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_قدر(nodep, newTrackedClone(const_cast<Ast_گرہ_اظہاریہ*>(valuep)));
        }
    }

خاص:
    Ast_گرہ_اظہاریہ* newTrackedClone(Ast_گرہ_اظہاریہ* nodep) {
        Ast_گرہ_اظہاریہ* ثابت newp = nodep->clone_درخت(false);
        m_reclaimValuesp.ڈالو_back(newp);
        أرجع newp;
    }
    AstConst* newConst(Ast_گرہ* nodep) {
        // Set a constant value for this node
        إذا (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            أرجع constp;
        }
        أرجع fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_گرہ* nodep) {
        // Set a var-output constant value for this node
        إذا (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* ثابت constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            أرجع constp;
        }
        أرجع fetchOutConst(nodep);
    }

عام:
    Ast_گرہ_اظہاریہ* fetch_قدر_Null(Ast_گرہ* nodep) { أرجع m_varAux(nodep).valuep; }

خاص:
    Ast_گرہ_اظہاریہ* fetch_Out_قدر_Null(Ast_گرہ* nodep) { أرجع m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_گرہ* nodep) { أرجع VN_CAST(fetch_قدر_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_گرہ* nodep) {
        أرجع VN_CAST(fetch_Out_قدر_Null(nodep), Const);
    }
    Ast_گرہ_اظہاریہ* fetch_قدر(Ast_گرہ* nodep) {
        Ast_گرہ_اظہاریہ* ثابت valuep = fetch_قدر_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        أرجع valuep;
    }
    AstConst* fetchConst(Ast_گرہ* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        أرجع constp;
    }
    AstConst* fetchOutConst(Ast_گرہ* nodep) {
        AstConst* ثابت constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        أرجع constp;
    }

عام:
    V3_نمبر* fetch_نمبر_Null(Ast_گرہ* nodep) {
        AstConst* ثابت constp = fetchConstNull(nodep);
        إذا (constp) أرجع &constp->عدد();
        أرجع nullptr;
    }
    V3_نمبر* fetch_Out_نمبر_Null(Ast_گرہ* nodep) {
        AstConst* ثابت constp = fetchOutConstNull(nodep);
        إذا (constp) أرجع &constp->عدد();
        أرجع nullptr;
    }

خاص:
    فراغ set_قدر(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    فراغ set_Out_قدر(Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    فراغ check_گرہ_Info(Ast_گرہ* nodep, bool ignorePredict = false) {
        إذا (m_checkOnly) {
            m_instr_گنتی += nodep->instr_گنتی();
            m_data_گنتی += nodep->width();
        }
        إذا (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        إذا (!nodep->isPure()) m_isImpure = true;
    }

    فراغ known_Bad_گرہ_Type(Ast_گرہ* nodep) {
        // Call for node types we know we can't handle
        إذا (jumpingOver()) أرجع;
        check_گرہ_Info(nodep);
        إذا (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    فراغ bad_گرہ_Type(Ast_گرہ* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_گرہ_Info(nodep);
        إذا (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            ساكن std::set<VNType> s_typePrinted;
            ثابت auto pair = s_typePrinted.emplace(nodep->type());
            إذا (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_گرہ* varOrScope(AstVarRef* nodep) ثابت {
        Ast_گرہ* vscp;
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
    فراغ assign_Out_قدر(ثابت Ast_گرہ_Assign* nodep, Ast_گرہ* vscp, ثابت Ast_گرہ_اظہاریہ* valuep) {
        إذا (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_قدر(vscp, valuep);
        } وإلا {
            new_قدر(vscp, valuep);
            set_Out_قدر(vscp, fetch_قدر(vscp));
        }
    }

    string toStringRecurse(Ast_گرہ_اظہاریہ* valuep) {
        // Return string representation of a value, or clearOptimizable
        إذا (ثابت AstConst* ثابت avaluep = VN_CAST(valuep, Const)) {
            أرجع "'h"s + avaluep->عدد().displayed(valuep, "%0x");
        }
        إذا (ثابت AstConsArray* ثابت avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            لكل (صحيح n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            أرجع result;
        }
        إذا (ثابت AstConsStruct* ثابت avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            لكل (ثابت AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            أرجع result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        أرجع "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_گرہ_اظہاریہ* newInitValuep(Ast_گرہ* nodep, Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_گرہ_اظہاریہ* ثابت elementp = newInitValuep(nodep, arrayp->subDTypep());
            أرجع elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        إذا (structp && !structp->packed()) {
            AstConsStruct* ثابت newp = new AstConsStruct{nodep->fileline(), structp};
            لكل (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_گرہ_اظہاریہ* ثابت memberValuep = newInitValuep(nodep, memberp->subDTypep());
                إذا (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_درخت(), newp);
                    أرجع nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            أرجع newp;
        }
        إذا (ثابت AstBasicDType* ثابت basicp = dtypep->basicp()) {
            AstConst* ثابت constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, basicp->widthMin(), 0};
            إذا (basicp->isZeroInit()) {
                constp->عدد() = V3_نمبر{nodep, basicp};
            } وإلا {
                constp->عدد().setAllBitsX();
            }
            أرجع constp;
        }
        أرجع nullptr;
    }
    فراغ initVar(AstVar* nodep) {
        إذا (Ast_گرہ_اظہاریہ* ثابت valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_قدر(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_درخت(), valuep);
        }
    }

    // VISITORS
    فراغ visit(AstAlways* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ visit(Ast_Sen_درخت* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    فراغ visit(AstVarRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_گرہ* ثابت vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        إذا (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), گرہ_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        إذا (nodep->access().isWriteOrRW()) {
            إذا (m_inDlyAssign) {
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
                ثابت Ast_گرہ_اظہاریہ* ثابت valuep
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
            Ast_گرہ_اظہاریہ* valuep = fetch_قدر_Null(vscp);
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
            set_قدر(nodep, valuep);
        }
    }
    فراغ visit(AstVarXRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (m_scoped) {
            bad_گرہ_Type(nodep);
            أرجع;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    فراغ visit(Ast_گرہ_FTask* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_گرہ_Type(nodep);
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
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstInitialAutomaticStmt* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstInitialStaticStmt* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstInitialStatic* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_گرہ_Type(nodep);
            أرجع;
        }
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ visit(Ast_گرہ_If* nodep) override {
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   IF " << nodep);
        check_گرہ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateAndNextConstNull(nodep->condp());
            إذا (optimizable()) {
                إذا (fetchConst(nodep->condp())->عدد().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } وإلا {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    فراغ visit(AstConst* nodep) override {
        check_گرہ_Info(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        new_قدر(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    فراغ visit(AstConsArray* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsArray* ثابت valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_قدر(nodep->defaultp())->clone_درخت(false) : nullptr};
        لكل (ثابت AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_قدر(itemp->valuep())->clone_درخت(false));
        }
        m_reclaimValuesp.ڈالو_back(valuep);
        set_قدر(nodep, valuep);
    }
    فراغ visit(AstConsStruct* nodep) override {
        إذا (!m_params) {
            bad_گرہ_Type(nodep);
            أرجع;
        }
        check_گرہ_Info(nodep);
        لكل (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsStruct* ثابت valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        لكل (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_قدر(itemp->valuep())->clone_درخت(false)});
        }
        m_reclaimValuesp.ڈالو_back(valuep);
        set_قدر(nodep, valuep);
    }
    فراغ visit(AstCvtUnpackToUnpack* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_گرہ_اظہاریہ* ثابت cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_قدر(nodep->fromp())->clone_درخت(false), nodep->dtypep()};
        Ast_گرہ_اظہاریہ* ثابت valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.ڈالو_back(valuep);
        set_قدر(nodep, valuep);
    }
    فراغ visit(AstConsArrayItem* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        check_گرہ_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        إذا (m_checkOnly || !optimizable()) أرجع;
        Ast_گرہ* ثابت valuep = nodep->itemp()->valuep();
        إذا (valuep) {
            iterateAndNextConstNull(valuep);
            إذا (!optimizable()) أرجع;
            new_قدر(nodep, fetch_قدر(valuep));
        } وإلا {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    فراغ visit(Ast_گرہ_Uniop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->نمبر_Operate(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    فراغ aggregateEqRecurse(V3_نمبر& resultr, Ast_گرہ* nodep, bool caseEq, Ast_گرہ_اظہاریہ* lhsp,
                            Ast_گرہ_اظہاریہ* rhsp) {
        إذا (ثابت AstConst* ثابت lconstp = VN_CAST(lhsp, Const)) {
            ثابت V3_نمبر& lnum = lconstp->عدد();
            ثابت V3_نمبر& rnum = VN_AS(rhsp, Const)->عدد();
            V3_نمبر eq{nodep, 1};
            إذا (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } وإلا إذا (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } وإلا إذا (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } وإلا {
                eq.opEq(lnum, rnum);
            }
            ثابت V3_نمبر prev = resultr;
            resultr.opLogAnd(prev, eq);
        } وإلا إذا (ثابت AstConsArray* ثابت lconsp = VN_CAST(lhsp, ConsArray)) {
            ثابت AstUnpackArrayDType* ثابت dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            لكل (صحيح n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } وإلا {
            ثابت AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            لكل (ثابت AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    فراغ handleAggregateCompare(Ast_گرہ_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        ثابت bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        ثابت bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_نمبر& resultr = newConst(nodep)->عدد();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_قدر(nodep->lhsp()),
                           fetch_قدر(nodep->rhsp()));
        إذا (neq) {
            ثابت V3_نمبر eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    فراغ visit(Ast_گرہ_Biop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        إذا (!VN_IS(fetch_قدر(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            أرجع;
        }
        AstConst* ثابت valuep = newConst(nodep);
        nodep->نمبر_Operate(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        إذا (!m_params && VN_IS(nodep, Sel) && valuep->عدد().isAnyX()) {
            ثابت V3_نمبر عدد{valuep, valuep->width(), valuep->عدد()};
            valuep->عدد().opBitsOne(عدد);
        }
    }
    فراغ visit(Ast_گرہ_Triop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->نمبر_Operate(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد(), fetchConst(nodep->thsp())->عدد());
    }
    فراغ visit(Ast_گرہ_Quadop* nodep) override {
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        nodep->نمبر_Operate(newConst(nodep)->عدد(), fetchConst(nodep->lhsp())->عدد(),
                             fetchConst(nodep->rhsp())->عدد(), fetchConst(nodep->thsp())->عدد(),
                             fetchConst(nodep->fhsp())->عدد());
    }
    فراغ visit(AstLogAnd* nodep) override {
        // Need to short circuit
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->عدد().isNeqZero()) {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            } وإلا {
                new_قدر(nodep, fetch_قدر(nodep->lhsp()));  // a zero
            }
        }
    }
    فراغ visit(AstLogOr* nodep) override {
        // Need to short circuit
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->عدد().isNeqZero()) {
                new_قدر(nodep, fetch_قدر(nodep->lhsp()));  // a one
            } وإلا {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            }
        }
    }
    فراغ visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->lhsp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->lhsp())->عدد().isEqZero()) {
                ثابت AstConst cnst{nodep->fileline(), AstConst::Widthed_قدر{}, 1, 1};  // a one
                new_قدر(nodep, &cnst);  // a one
            } وإلا {
                iterateConst(nodep->rhsp());
                إذا (!optimizable()) أرجع;
                new_قدر(nodep, fetch_قدر(nodep->rhsp()));
            }
        }
    }
    فراغ visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا {
            iterateConst(nodep->condp());
            إذا (!optimizable()) أرجع;
            إذا (fetchConst(nodep->condp())->عدد().isNeqZero()) {
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

    فراغ handleAssignElement(Ast_گرہ_Assign* nodep, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* قدر_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_گرہ_اظہاریہ*> selps;
        Ast_گرہ_اظہاریہ* fromp = lhsp;
        طالما (true) {
            إذا (AstArraySel* ثابت selp = VN_CAST(fromp, ArraySel)) {
                check_گرہ_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.ڈالو_back(selp);
                fromp = selp->fromp();
            } وإلا إذا (AstStructSel* ثابت selp = VN_CAST(fromp, StructSel)) {
                check_گرہ_Info(selp);
                selps.ڈالو_back(selp);
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
        Ast_گرہ* ثابت vscp = varOrScope(varrefp);
        Ast_گرہ_اظہاریہ* rootp = fetch_Out_قدر_Null(vscp);
        إذا (!rootp) rootp = fetch_قدر_Null(vscp);
        إذا (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            أرجع;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_گرہ_اظہاریہ* itemp = rootp;
        لكل (Ast_گرہ_اظہاریہ* ثابت selp : vlstd::reverse_view(selps)) {
            إذا (ثابت AstArraySel* ثابت aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* ثابت consp = VN_AS(itemp, ConsArray);
                ثابت uint32_t index = fetchConst(aselp->bitp())->toUInt();
                ثابت صحيح elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                إذا (index >= static_cast<uint32_t>(elements)) {
                    أرجع;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                إذا (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_درخت(false);
                    consp->valuep(index, itemp);
                }
            } وإلا {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_گرہ_اظہاریہ* ثابت valuep = newTrackedClone(fetch_قدر(قدر_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.ڈالو_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_قدر(nodep, vscp, rootp);
    }
    فراغ handleAssignSel(Ast_گرہ_Assign* nodep, AstSel* selp, Ast_گرہ_اظہاریہ* قدر_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_نمبر lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        إذا (m_checkOnly || !optimizable()) أرجع;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_گرہ* ثابت vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        إذا (AstConst* ثابت vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } وإلا إذا (AstConst* ثابت vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } وإلا {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_قدر{},
                                     varrefp->varp()->widthMin(), 0};
            إذا (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->عدد().setAllBits0();
            } وإلا {
                outconstp->عدد().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->عدد().opSelInto(fetchConst(قدر_Fromp)->عدد(), lsb, selp->widthConst());
        assign_Out_قدر(nodep, vscp, outconstp);
    }
    فراغ handleAssignSelRecurse(Ast_گرہ_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_نمبر& lsbRef, صحيح depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_گرہ_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        إذا (AstVarRef* ثابت varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->عدد();
            أرجع;  // And presumably still optimizable()
        }
        إذا (AstSel* ثابت subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_نمبر sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            إذا (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->عدد());
            }
        } وإلا {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    فراغ handleAssignRecurse(Ast_گرہ_Assign* nodep, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* قدر_Fromp) {
        إذا (!optimizable()) أرجع;
        إذا (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            إذا (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                أرجع;
            }
            handleAssignElement(nodep, lhsp, قدر_Fromp);
        } وإلا إذا (AstConcat* ثابت selp = VN_CAST(lhsp, Concat)) {
            check_گرہ_Info(selp);
            إذا (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                أرجع;
            }
            // Split value into left and right concat values
            إذا (!m_checkOnly) {
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_قدر{},
                                       selp->lhsp()->width(), 0};
                    outconstp->عدد().opSel(fetchConst(قدر_Fromp)->عدد(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_قدر(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_درخت(), outconstp);
                }
                {
                    AstConst* ثابت outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_قدر{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->عدد().opSel(fetchConst(قدر_Fromp)->عدد(),
                                           selp->rhsp()->width() - 1, 0);
                    new_قدر(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_درخت(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } وإلا إذا (AstReplicate* ثابت selp = VN_CAST(lhsp, Replicate)) {
            check_گرہ_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* ثابت countp = VN_CAST(selp->countp(), Const);
            إذا (!countp || !countp->عدد().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                أرجع;
            }
            handleAssignRecurse(nodep, selp->srcp(), قدر_Fromp);
        } وإلا إذا (AstSel* ثابت selp = VN_CAST(lhsp, Sel)) {
            إذا (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                أرجع;
            }
            handleAssignSel(nodep, selp, قدر_Fromp);
        } وإلا إذا (VN_IS(lhsp, VarRef)) {
            إذا (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } وإلا {
                Ast_گرہ* ثابت vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_قدر(nodep, vscp, fetch_قدر(قدر_Fromp));
            }
        } وإلا {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    فراغ visit(Ast_گرہ_Assign* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        إذا (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } وإلا إذا (VN_IS(nodep, AssignDly)) {
            إذا (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } وإلا {
            إذا (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        إذا (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            أرجع;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    فراغ visit(AstArraySel* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_قدر_Null(nodep->fromp()), ConsArray)) {
            ثابت AstConst* ثابت indexp = fetchConst(nodep->bitp());
            ثابت uint32_t offset = indexp->عدد().toUInt();
            Ast_گرہ_اظہاریہ* ثابت itemp = consp->valuep(offset);
            إذا (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } وإلا {
                set_قدر(nodep, itemp);
            }
        } وإلا {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    فراغ visit(AstStructSel* nodep) override {
        إذا (!m_params) {
            bad_گرہ_Type(nodep);
            أرجع;
        }
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        AstConsStruct* ثابت consp = VN_AS(fetch_قدر(nodep->fromp()), ConsStruct);
        set_قدر(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    فراغ visit(AstSliceSel* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        // Fetch the base constant array
        إذا (ثابت AstConsArray* ثابت consp = VN_CAST(fetch_قدر_Null(nodep->fromp()), ConsArray)) {
            ثابت VNumRange& sliceRange = nodep->declRange();
            ثابت uint32_t sliceElements = sliceRange.elements();
            ثابت صحيح sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_گرہ_DType* ثابت dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* ثابت newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            لكل (uint32_t idx = 0; idx < sliceElements; ++idx) {
                ثابت uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_درخت(false));
            }
            // Assign the new constant array and track it for later deletion
            set_قدر(nodep, newConsp);
            m_reclaimValuesp.ڈالو_back(newConsp);
        } وإلا {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    فراغ visit(AstBegin* nodep) override {
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstCase* nodep) override {
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   CASE " << nodep);
        check_گرہ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
        } وإلا إذا (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                إذا (!itemp->isDefault()) {
                    لكل (Ast_گرہ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        إذا (hit) اكسر;
                        iterateAndNextConstNull(ep);
                        إذا (optimizable()) {
                            V3_نمبر match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->عدد(), fetchConst(ep)->عدد());
                            إذا (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                إذا (hit) اكسر;
                إذا (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    فراغ visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        إذا (jumpingOver()) أرجع;
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstComment*) override {}

    فراغ visit(Ast_Stmt_اظہاریہ* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
    }
    فراغ visit(Ast_اظہاریہ_Stmt* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گرہ_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        إذا (!optimizable()) أرجع;
        iterateAndNextConstNull(nodep->resultp());
        إذا (m_checkOnly || !optimizable()) أرجع;
        new_قدر(nodep, fetch_قدر(nodep->resultp()));
    }

    فراغ visit(AstJumpBlock* nodep) override {
        إذا (jumpingOver()) أرجع;
        iterateChildrenConst(nodep);
        إذا (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    فراغ visit(AstJumpGo* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گرہ_Info(nodep);
        إذا (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    فراغ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        إذا (jumpingOver()) أرجع;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        إذا (!m_params) {
            bad_گرہ_Type(nodep);
            أرجع;
        }
        check_گرہ_Info(nodep);
        إذا (m_checkOnly) {
            iterateChildrenConst(nodep);
            أرجع;
        }
        إذا (!optimizable()) أرجع;

        size_t iter_گنتی = 0;
        ثابت size_t iterLimit = v3Global.opt.unrollLimit();
        طالما (true) {
            إذا (iter_گنتی > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                اكسر;
            }
            ++iter_گنتی;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            إذا (jumpingOver()) اكسر;
        }

        إذا (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    فراغ visit(AstLoopTest* nodep) override {
        إذا (jumpingOver()) أرجع;
        check_گرہ_Info(nodep);
        iterateConst(nodep->condp());
        إذا (m_checkOnly || !optimizable()) أرجع;
        إذا (fetchConst(nodep->condp())->عدد().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    فراغ visit(AstStop* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_گرہ_Info(nodep);
    }
    فراغ visit(AstFuncRef* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_گرہ_Info(nodep);
        إذا (!m_params) {
            bad_گرہ_Type(nodep);
            أرجع;
        }
        Ast_گرہ_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        إذا (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        إذا (funcp->recursive()) {
            إذا (m_recurse_گنتی >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                أرجع;
            }
            ++m_recurse_گنتی;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_گرہ*, Ast_گرہ_اظہاریہ*> old_اقدار;

        إذا (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_اقدار](AstVar* varp) {
                إذا (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    إذا (Ast_گرہ_اظہاریہ* ثابت valuep = fetch_قدر_Null(varp)) {
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
                إذا (VN_IS(exprp, Const)) أرجع;  // Speed up as won't change
                إذا (Ast_گرہ_اظہاریہ* ثابت valuep = fetch_قدر_Null(exprp)) {
                    Ast_گرہ_اظہاریہ* ثابت nvaluep = newTrackedClone(valuep);
                    old_اقدار.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_گرہ_اظہاریہ*>> port_اقدار;
        لكل (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* ثابت portp = it->first;
            Ast_گرہ* ثابت pinp = it->second->exprp();
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
                port_اقدار.emplace_back(std::make_pair(portp, newTrackedClone(fetch_قدر(pinp))));
            }
        }
        // Apply value to the function
        إذا (!m_checkOnly && optimizable())
            لكل (ثابت auto& it : port_اقدار) {
                إذا (!m_checkOnly && optimizable()) new_قدر(it.first, it.second);
            }
        Sim_ڈھیر_گرہ ڈھیر_گرہ{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_ڈھیر.ڈالو_back(&ڈھیر_گرہ);
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
        m_call_ڈھیر.نکالو_back();
        ثابت Ast_گرہ_اظہاریہ* returnp = nullptr;
        إذا (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_قدر(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        لكل (ثابت auto& it : old_اقدار) {
            إذا (it.second) new_قدر(it.first, it.second);
        }
        إذا (returnp) new_قدر(nodep, returnp);
        إذا (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_گنتی > 0, nodep, "recurse underflow");
            --m_recurse_گنتی;
        }
    }

    فراغ visit(AstVar* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!m_params) {
            bad_گرہ_Type(nodep);
            أرجع;
        }
    }

    فراغ visit(AstScopeName* /*nodep*/) override {
        إذا (jumpingOver()) أرجع;
        // Ignore
    }

    فراغ visit(AstSFormatArg* nodep) override {
        check_گرہ_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    فراغ visit(AstSFormatF* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;

        Ast_گرہ* nextArgp = nodep->exprsp();
        string result;
        ثابت string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        لكل (; pos != format.cend(); ++pos) {
            إذا (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } وإلا إذا (!inPct) {  // Normal text
                result += *pos;
            } وإلا {  // Format character
                إذا (std::isdigit(pos[0])) {
                    width += pos[0];
                    استمر;
                }

                inPct = false;

                إذا (V3_نمبر::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_گرہ* ثابت argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);
                    Ast_گرہ* ثابت subargp = fargp ? fargp->exprp() : argp;
                    AstConst* ثابت constp = fetchConstNull(subargp);
                    ثابت VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    إذا (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        اكسر;
                    }
                    ثابت string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->عدد().displayedEnum(fargp, pformat)
                                  : constp->عدد().displayed(nodep, pformat, formatAttr);
                } وإلا {
                    بدل (std::tolower(pos[0])) {
                    حالة '%': result += "%"; اكسر;
                    حالة 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        اكسر;
                    افتراضي: clearOptimizable(nodep, "Unknown $display-like format code."); اكسر;
                    }
                }
            }
        }

        AstConst* ثابت resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_قدر(nodep, resultConstp);
        m_reclaimValuesp.ڈالو_back(resultConstp);
    }

    فراغ visit(AstDisplay* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_گرہ_Info(nodep, /*display:*/ true);
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
    فراغ visit(AstToStringN* nodep) override {
        إذا (jumpingOver()) أرجع;
        إذا (!optimizable()) أرجع;  // Accelerate
        check_گرہ_Info(nodep);
        iterateChildrenConst(nodep);
        إذا (m_checkOnly || !optimizable()) أرجع;
        ثابت std::string result = toStringRecurse(fetch_قدر(nodep->lhsp()));
        إذا (!optimizable()) أرجع;
        AstConst* ثابت resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_قدر(nodep, resultConstp);
        m_reclaimValuesp.ڈالو_back(resultConstp);
    }

    فراغ visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    فراغ visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_گرہ_Type(nodep);
    }
    فراغ visit(AstGetInitialRandomSeed* nodep) override { known_Bad_گرہ_Type(nodep); }
    فراغ visit(AstMemberSel* nodep) override { known_Bad_گرہ_Type(nodep); }
    فراغ visit(AstSampled* nodep) override { known_Bad_گرہ_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    فراغ visit(Ast_گرہ* nodep) override {
        إذا (jumpingOver()) أرجع;
        bad_گرہ_Type(nodep);
    }

    // MEMBERS - called by constructor
    فراغ setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    فراغ mainGuts(Ast_گرہ* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

عام:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    فراغ clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_گنتی = 0;
        m_data_گنتی = 0;
        m_jumptargetp = nullptr;

        Ast_گرہ::user1_Clear_درخت();
        m_varAux.clear();
        ++m_constGeneration;
    }
    فراغ mainTableCheck(Ast_گرہ* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    فراغ mainTableEmulate(Ast_گرہ* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    فراغ mainParamEmulate(Ast_گرہ* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_گرہ*> unusedRootps;
        لكل (Ast_گرہ* ثابت nodep : m_reclaimValuesp) {
            إذا (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        لكل (Ast_گرہ* ثابت nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
};

#endif  // Guard
