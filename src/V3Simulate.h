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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Task.h"
#அந்தர்பாவ "V3Width.h"

#அந்தர்பாவ <deque>
#அந்தர்பாவ <set>
#அந்தர்பாவ <sstream>
#அந்தர்பாவ <stack>
#அந்தர்பாவ <string>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_அடுக்கு_முனை final {
பொது:
    // MEMBERS
    AstFuncRef* மாறிலி m_funcp;
    V3TaskConnects* மாறிலி m_tconnects;
    // CONSTRUCTORS
    Sim_அடுக்கு_முனை(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_அடுக்கு_முனை() = இயல்பு;
};

class SimulateVisitor VL_NOT_FINAL : பொது VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

தனிப்பட்ட:
    // CONSTANTS
    நிலையான constexpr முழு CALL_அடுக்கு_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    மாறிலி VNUser1InUse m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    எண்ணிக்கை VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    கட்டமைப்பு AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_முனை_கோவை* outValuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_முனை_கோவை* valuep = nullptr;
    };

    AstUser1Allocator<Ast_முனை, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    கட்டமைப்பு ConstAllocator final {
        size_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        size_t m_nextFree = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(size_t currentGeneration, Ast_முனை* nodep) {
            எனில் (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_nextFree = 0;
            }
            UASSERT_OBJ(m_nextFree <= m_constps.size(), nodep, "Should only allocate at end");
            எனில் (m_nextFree == m_constps.size()) {
                m_constps.தள்ளு_back(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* மாறிலி constp = m_constps[m_nextFree++];
            constp->இலக்கம்().nodep(nodep);
            திரும்பு constp;
        }

        ~ConstAllocator() {
            க்கு (AstConst* மாறிலி constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_முனை* m_whyNotNodep;  ///< First node not optimizable
    bool m_anyAssignDly;  ///< True if found a delayed assignment
    bool m_anyAssignComb;  ///< True if found a non-delayed assignment
    bool m_inDlyAssign;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    முழு m_instr_மொத்தம்;  ///< Number of nodes
    முழு m_data_மொத்தம்;  ///< Bytes of data
    முழு m_recurse_மொத்தம் = 0;  // Now deep in current recursion
    Ast_முனை* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<மாறிலி Ast_முனை_DType*, ConstAllocator> m_constps;
    size_t m_constGeneration = 0;
    std::vector<Sim_அடுக்கு_முனை*> m_call_அடுக்கு;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::vector<Ast_முனை*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_எண்(மாறிலி V3_எண்* nump, மாறிலி Ast_முனை_DType* dtypep) {
        எனில் (மாறிலி AstRefDType* மாறிலி refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        எனில் (மாறிலி Ast_முனை_UOr_Struct_DType* மாறிலி stp = VN_CAST(dtypep, முனை_UOr_Struct_DType)) {
            எனில் (stp->packed()) {
                std::ostringstream out;
                out << "'{";
                க்கு (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    மாறிலி முழு width = itemp->width();
                    மாறிலி முழு lsb = itemp->lsb();
                    மாறிலி முழு msb = lsb + width - 1;
                    V3_எண் field_இலக்கம்{nump, width};
                    field_இலக்கம்.opSel(*nump, msb, lsb);
                    out << itemp->name() << ": ";
                    எனில் (மாறிலி Ast_முனை_DType* மாறிலி childTypep = itemp->subDTypep()) {
                        out << pretty_எண்(&field_இலக்கம், childTypep);
                    } இல்லையெனில் {
                        out << field_இலக்கம்;
                    }
                    எனில் (itemp->nextp()) out << ", ";
                }
                out << "}";
                திரும்பு out.str();
            }
        } இல்லையெனில் எனில் (மாறிலி AstPackArrayDType* மாறிலி arrayp = VN_CAST(dtypep, PackArrayDType)) {
            எனில் (மாறிலி Ast_முனை_DType* மாறிலி childTypep = arrayp->subDTypep()) {
                std::ostringstream out;
                out << "[";
                மாறிலி முழு arrayElements = arrayp->elementsConst();
                க்கு (முழு element = 0; element < arrayElements; ++element) {
                    மாறிலி முழு width = childTypep->width();
                    மாறிலி முழு lsb = width * element;
                    மாறிலி முழு msb = lsb + width - 1;
                    V3_எண் field_இலக்கம்{nump, width};
                    field_இலக்கம்.opSel(*nump, msb, lsb);
                    மாறிலி முழு arrayElem = arrayp->lo() + element;
                    out << arrayElem << " = " << pretty_எண்(&field_இலக்கம், childTypep);
                    எனில் (element < arrayElements - 1) out << ", ";
                }
                out << "]";
                திரும்பு out.str();
            }
        }
        திரும்பு nump->ascii();
    }

    // Checking METHODS
பொது:
    /// Call other-this function on all new *non-constant* var references
    மெய்நிகர் வெறுமை varRefCb(AstVarRef* nodep) {}

    வெறுமை clearOptimizable(Ast_முனை* nodep /*null ok*/, மாறிலி string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        எனில் (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            எனில் (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_PREFIX("Clear optimizable: " << why);
                எனில் (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream அடுக்கு;
            முழு n = 0;
            க்கு (மாறிலி auto& callstack : vlstd::reverse_view(m_call_அடுக்கு)) {
                மாறிலி AstFuncRef* மாறிலி funcp = callstack->m_funcp;
                அடுக்கு << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                க்கு (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->end(); ++conIt) {
                    மாறிலி AstVar* மாறிலி portp = conIt->first;
                    Ast_முனை_கோவை* மாறிலி pinp = conIt->second->exprp();
                    மாறிலி Ast_முனை_DType* மாறிலி dtypep = pinp->dtypep();
                    எனில் (AstConst* மாறிலி valp = fetchConstNull(pinp)) {
                        அடுக்கு << "\n           " << portp->prettyName() << " = "
                              << pretty_எண்(&valp->இலக்கம்(), dtypep);
                    } இல்லையெனில் எனில் (Ast_முனை_கோவை* மாறிலி valuep = fetch_மதிப்பு_Null(pinp)) {
                        அடுக்கு << "\n           " << portp->prettyName() << " = "
                              << toStringRecurse(valuep);
                    }
                }
                எனில் (++n > CALL_அடுக்கு_MAX) {
                    அடுக்கு << "\n           ... stack truncated";
                    நிறுத்து;
                }
            }
            m_whyNotOptimizable += அடுக்கு.str();
        }
    }
    bool optimizable() மாறிலி { திரும்பு m_whyNotNodep == nullptr; }
    string whyNotMessage() மாறிலி { திரும்பு m_whyNotOptimizable; }
    Ast_முனை* whyNotNodep() மாறிலி { திரும்பு m_whyNotNodep; }

    bool isAssignDly() மாறிலி { திரும்பு m_anyAssignDly; }
    bool isImpure() மாறிலி { திரும்பு m_isImpure; }
    bool isCoverage() மாறிலி { திரும்பு m_isCoverage; }
    முழு instr_மொத்தம்() மாறிலி { திரும்பு m_instr_மொத்தம்; }
    முழு data_மொத்தம்() மாறிலி { திரும்பு m_data_மொத்தம்; }

    // Simulation METHODS
தனிப்பட்ட:
    AstConst* allocConst(Ast_முனை* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        திரும்பு m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

பொது:
    வெறுமை new_மதிப்பு(Ast_முனை* nodep, மாறிலி Ast_முனை_கோவை* valuep) {
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->இலக்கம்().opAssign(constp->இலக்கம்());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } இல்லையெனில் எனில் (fetch_மதிப்பு_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_மதிப்பு(nodep, newTrackedClone(const_cast<Ast_முனை_கோவை*>(valuep)));
        }
    }
    வெறுமை new_Out_மதிப்பு(Ast_முனை* nodep, மாறிலி Ast_முனை_கோவை* valuep) {
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(valuep, Const)) {
            newOutConst(nodep)->இலக்கம்().opAssign(constp->இலக்கம்());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } இல்லையெனில் எனில் (fetch_Out_மதிப்பு_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            set_Out_மதிப்பு(nodep, newTrackedClone(const_cast<Ast_முனை_கோவை*>(valuep)));
        }
    }

தனிப்பட்ட:
    Ast_முனை_கோவை* newTrackedClone(Ast_முனை_கோவை* nodep) {
        Ast_முனை_கோவை* மாறிலி newp = nodep->clone_மரம்(false);
        m_reclaimValuesp.தள்ளு_back(newp);
        திரும்பு newp;
    }
    AstConst* newConst(Ast_முனை* nodep) {
        // Set a constant value for this node
        எனில் (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* மாறிலி constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            திரும்பு constp;
        }
        திரும்பு fetchConst(nodep);
    }
    AstConst* newOutConst(Ast_முனை* nodep) {
        // Set a var-output constant value for this node
        எனில் (!VN_IS(m_varAux(nodep).outValuep, Const)) {
            AstConst* மாறிலி constp = allocConst(nodep);
            m_varAux(nodep).outValuep = constp;
            திரும்பு constp;
        }
        திரும்பு fetchOutConst(nodep);
    }

பொது:
    Ast_முனை_கோவை* fetch_மதிப்பு_Null(Ast_முனை* nodep) { திரும்பு m_varAux(nodep).valuep; }

தனிப்பட்ட:
    Ast_முனை_கோவை* fetch_Out_மதிப்பு_Null(Ast_முனை* nodep) { திரும்பு m_varAux(nodep).outValuep; }
    AstConst* fetchConstNull(Ast_முனை* nodep) { திரும்பு VN_CAST(fetch_மதிப்பு_Null(nodep), Const); }
    AstConst* fetchOutConstNull(Ast_முனை* nodep) {
        திரும்பு VN_CAST(fetch_Out_மதிப்பு_Null(nodep), Const);
    }
    Ast_முனை_கோவை* fetch_மதிப்பு(Ast_முனை* nodep) {
        Ast_முனை_கோவை* மாறிலி valuep = fetch_மதிப்பு_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        திரும்பு valuep;
    }
    AstConst* fetchConst(Ast_முனை* nodep) {
        AstConst* மாறிலி constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        திரும்பு constp;
    }
    AstConst* fetchOutConst(Ast_முனை* nodep) {
        AstConst* மாறிலி constp = fetchOutConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        திரும்பு constp;
    }

பொது:
    V3_எண்* fetch_எண்_Null(Ast_முனை* nodep) {
        AstConst* மாறிலி constp = fetchConstNull(nodep);
        எனில் (constp) திரும்பு &constp->இலக்கம்();
        திரும்பு nullptr;
    }
    V3_எண்* fetch_Out_எண்_Null(Ast_முனை* nodep) {
        AstConst* மாறிலி constp = fetchOutConstNull(nodep);
        எனில் (constp) திரும்பு &constp->இலக்கம்();
        திரும்பு nullptr;
    }

தனிப்பட்ட:
    வெறுமை set_மதிப்பு(Ast_முனை* nodep, Ast_முனை_கோவை* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    வெறுமை set_Out_மதிப்பு(Ast_முனை* nodep, Ast_முனை_கோவை* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).outValuep = valuep;
    }

    வெறுமை check_முனை_Info(Ast_முனை* nodep, bool ignorePredict = false) {
        எனில் (m_checkOnly) {
            m_instr_மொத்தம் += nodep->instr_மொத்தம்();
            m_data_மொத்தம் += nodep->width();
        }
        எனில் (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        எனில் (!nodep->isPure()) m_isImpure = true;
    }

    வெறுமை known_Bad_முனை_Type(Ast_முனை* nodep) {
        // Call for node types we know we can't handle
        எனில் (jumpingOver()) திரும்பு;
        check_முனை_Info(nodep);
        எனில் (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    வெறுமை bad_முனை_Type(Ast_முனை* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_முனை_Info(nodep);
        எனில் (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#ifdef VL_DEBUG
            நிலையான std::set<VNType> s_typePrinted;
            மாறிலி auto pair = s_typePrinted.emplace(nodep->type());
            எனில் (pair.second)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#endif
        }
    }

    Ast_முனை* varOrScope(AstVarRef* nodep) மாறிலி {
        Ast_முனை* vscp;
        எனில் (m_scoped) {
            vscp = nodep->varScopep();
        } இல்லையெனில் {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        திரும்பு vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() மாறிலி { திரும்பு m_jumptargetp; }
    வெறுமை assign_Out_மதிப்பு(மாறிலி Ast_முனை_Assign* nodep, Ast_முனை* vscp, மாறிலி Ast_முனை_கோவை* valuep) {
        எனில் (VN_IS(nodep, AssignDly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_Out_மதிப்பு(vscp, valuep);
        } இல்லையெனில் {
            new_மதிப்பு(vscp, valuep);
            set_Out_மதிப்பு(vscp, fetch_மதிப்பு(vscp));
        }
    }

    string toStringRecurse(Ast_முனை_கோவை* valuep) {
        // Return string representation of a value, or clearOptimizable
        எனில் (மாறிலி AstConst* மாறிலி avaluep = VN_CAST(valuep, Const)) {
            திரும்பு "'h"s + avaluep->இலக்கம்().displayed(valuep, "%0x");
        }
        எனில் (மாறிலி AstConsArray* மாறிலி avaluep = VN_CAST(valuep, ConsArray)) {
            string result = "'{";
            string comma;
            மாறிலி AstUnpackArrayDType* மாறிலி dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            க்கு (முழு n = 0; n < dtypep->elementsConst(); ++n) {
                result += comma + toStringRecurse(avaluep->valuep(n));
                comma = ", ";
            }
            result += "}";
            திரும்பு result;
        }
        எனில் (மாறிலி AstConsStruct* மாறிலி avaluep = VN_CAST(valuep, ConsStruct)) {
            string result = "'{";
            string comma;
            க்கு (மாறிலி AstConsStructItem* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                result
                    += comma + itemp->memberp()->name() + ":" + toStringRecurse(itemp->valuep());
                comma = ", ";
            }
            result += "}";
            திரும்பு result;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        திரும்பு "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_முனை_கோவை* newInitValuep(Ast_முனை* nodep, Ast_முனை_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        எனில் (AstUnpackArrayDType* மாறிலி arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_முனை_கோவை* மாறிலி elementp = newInitValuep(nodep, arrayp->subDTypep());
            திரும்பு elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* மாறிலி structp = VN_CAST(dtypep, StructDType);
        எனில் (structp && !structp->packed()) {
            AstConsStruct* மாறிலி newp = new AstConsStruct{nodep->fileline(), structp};
            க்கு (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_முனை_கோவை* மாறிலி memberValuep = newInitValuep(nodep, memberp->subDTypep());
                எனில் (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_மரம்(), newp);
                    திரும்பு nullptr;
                }
                newp->addItemsp(new AstConsStructItem{nodep->fileline(), memberp, memberValuep});
            }
            திரும்பு newp;
        }
        எனில் (மாறிலி AstBasicDType* மாறிலி basicp = dtypep->basicp()) {
            AstConst* மாறிலி constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_மதிப்பு{}, basicp->widthMin(), 0};
            எனில் (basicp->isZeroInit()) {
                constp->இலக்கம்() = V3_எண்{nodep, basicp};
            } இல்லையெனில் {
                constp->இலக்கம்().setAllBitsX();
            }
            திரும்பு constp;
        }
        திரும்பு nullptr;
    }
    வெறுமை initVar(AstVar* nodep) {
        எனில் (Ast_முனை_கோவை* மாறிலி valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_மதிப்பு(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_மரம்(), valuep);
        }
    }

    // VISITORS
    வெறுமை visit(AstAlways* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_Sen_மரம்* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    வெறுமை visit(AstVarRef* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_முனை* மாறிலி vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        எனில் (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), முனை_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        எனில் (nodep->access().isWriteOrRW()) {
            எனில் (m_inDlyAssign) {
                எனில் (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    எனில் (m_checkOnly) varRefCb(nodep);
                }
            } இல்லையெனில் {  // nondly asn
                எனில் (!(m_varAux(vscp).usage & VU_LV)) {
                    எனில் (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    எனில் (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        எனில் (nodep->access().isReadOrRW()) {
            எனில் (!(m_varAux(vscp).usage & VU_RV)) {
                எனில் (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                மாறிலி bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                மாறிலி Ast_முனை_கோவை* மாறிலி valuep
                    = varIsConst ? fetch_மதிப்பு_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                எனில் (varIsConst && valuep) {
                    எனில் (!m_checkOnly && optimizable()) new_மதிப்பு(vscp, valuep);
                } இல்லையெனில் {
                    எனில் (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        எனில் (m_checkOnly || !optimizable()) திரும்பு;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_முனை_கோவை* valuep = fetch_மதிப்பு_Null(vscp);
            எனில் (!valuep) {
                எனில் (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } இல்லையெனில் {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            set_மதிப்பு(nodep, valuep);
        }
    }
    வெறுமை visit(AstVarXRef* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (m_scoped) {
            bad_முனை_Type(nodep);
            திரும்பு;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!m_params) {
            bad_முனை_Type(nodep);
            திரும்பு;
        }
        எனில் (nodep->dpiImport()) {
            எனில் (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        எனில் (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstInitialAutomaticStmt* nodep) override {
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstInitialStaticStmt* nodep) override {
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstInitialStatic* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!m_params) {
            bad_முனை_Type(nodep);
            திரும்பு;
        }
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_முனை_If* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        UINFO(5, "   IF " << nodep);
        check_முனை_Info(nodep);
        எனில் (m_checkOnly) {
            iterateChildrenConst(nodep);
        } இல்லையெனில் {
            iterateAndNextConstNull(nodep->condp());
            எனில் (optimizable()) {
                எனில் (fetchConst(nodep->condp())->இலக்கம்().isNeqZero()) {
                    iterateAndNextConstNull(nodep->thensp());
                } இல்லையெனில் {
                    iterateAndNextConstNull(nodep->elsesp());
                }
            }
        }
    }
    வெறுமை visit(AstConst* nodep) override {
        check_முனை_Info(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        new_மதிப்பு(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    வெறுமை visit(AstConsArray* nodep) override {
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        AstConsArray* மாறிலி valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_மதிப்பு(nodep->defaultp())->clone_மரம்(false) : nullptr};
        க்கு (மாறிலி AstConsArrayItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
            valuep->valuep(itemp->index(), fetch_மதிப்பு(itemp->valuep())->clone_மரம்(false));
        }
        m_reclaimValuesp.தள்ளு_back(valuep);
        set_மதிப்பு(nodep, valuep);
    }
    வெறுமை visit(AstConsStruct* nodep) override {
        எனில் (!m_params) {
            bad_முனை_Type(nodep);
            திரும்பு;
        }
        check_முனை_Info(nodep);
        க்கு (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            iterateConst(itemp->valuep());
        }
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        AstConsStruct* மாறிலி valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        க்கு (AstConsStructItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
            valuep->addItemsp(
                new AstConsStructItem{itemp->fileline(), itemp->memberp(),
                                      fetch_மதிப்பு(itemp->valuep())->clone_மரம்(false)});
        }
        m_reclaimValuesp.தள்ளு_back(valuep);
        set_மதிப்பு(nodep, valuep);
    }
    வெறுமை visit(AstCvtUnpackToUnpack* nodep) override {
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_முனை_கோவை* மாறிலி cvtp = new AstCvtUnpackToUnpack{
            nodep->fileline(), fetch_மதிப்பு(nodep->fromp())->clone_மரம்(false), nodep->dtypep()};
        Ast_முனை_கோவை* மாறிலி valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.தள்ளு_back(valuep);
        set_மதிப்பு(nodep, valuep);
    }
    வெறுமை visit(AstConsArrayItem* nodep) override {
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstEnumItemRef* nodep) override {
        check_முனை_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        Ast_முனை* மாறிலி valuep = nodep->itemp()->valuep();
        எனில் (valuep) {
            iterateAndNextConstNull(valuep);
            எனில் (!optimizable()) திரும்பு;
            new_மதிப்பு(nodep, fetch_மதிப்பு(valuep));
        } இல்லையெனில் {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    வெறுமை visit(Ast_முனை_Uniop* nodep) override {
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        nodep->எண்_Operate(newConst(nodep)->இலக்கம்(), fetchConst(nodep->lhsp())->இலக்கம்());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    வெறுமை aggregateEqRecurse(V3_எண்& resultr, Ast_முனை* nodep, bool caseEq, Ast_முனை_கோவை* lhsp,
                            Ast_முனை_கோவை* rhsp) {
        எனில் (மாறிலி AstConst* மாறிலி lconstp = VN_CAST(lhsp, Const)) {
            மாறிலி V3_எண்& lnum = lconstp->இலக்கம்();
            மாறிலி V3_எண்& rnum = VN_AS(rhsp, Const)->இலக்கம்();
            V3_எண் eq{nodep, 1};
            எனில் (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } இல்லையெனில் எனில் (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } இல்லையெனில் எனில் (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } இல்லையெனில் {
                eq.opEq(lnum, rnum);
            }
            மாறிலி V3_எண் prev = resultr;
            resultr.opLogAnd(prev, eq);
        } இல்லையெனில் எனில் (மாறிலி AstConsArray* மாறிலி lconsp = VN_CAST(lhsp, ConsArray)) {
            மாறிலி AstUnpackArrayDType* மாறிலி dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            க்கு (முழு n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } இல்லையெனில் {
            மாறிலி AstConsStructItem* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            க்கு (மாறிலி AstConsStructItem* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), ConsStructItem),
                                          ritemp = VN_AS(ritemp->nextp(), ConsStructItem)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    வெறுமை handleAggregateCompare(Ast_முனை_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        மாறிலி bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        மாறிலி bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_எண்& resultr = newConst(nodep)->இலக்கம்();
        resultr.setAllBits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_மதிப்பு(nodep->lhsp()),
                           fetch_மதிப்பு(nodep->rhsp()));
        எனில் (neq) {
            மாறிலி V3_எண் eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    வெறுமை visit(Ast_முனை_Biop* nodep) override {
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        எனில் (!VN_IS(fetch_மதிப்பு(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            திரும்பு;
        }
        AstConst* மாறிலி valuep = newConst(nodep);
        nodep->எண்_Operate(newConst(nodep)->இலக்கம்(), fetchConst(nodep->lhsp())->இலக்கம்(),
                             fetchConst(nodep->rhsp())->இலக்கம்());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        எனில் (!m_params && VN_IS(nodep, Sel) && valuep->இலக்கம்().isAnyX()) {
            மாறிலி V3_எண் இலக்கம்{valuep, valuep->width(), valuep->இலக்கம்()};
            valuep->இலக்கம்().opBitsOne(இலக்கம்);
        }
    }
    வெறுமை visit(Ast_முனை_Triop* nodep) override {
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        nodep->எண்_Operate(newConst(nodep)->இலக்கம்(), fetchConst(nodep->lhsp())->இலக்கம்(),
                             fetchConst(nodep->rhsp())->இலக்கம்(), fetchConst(nodep->thsp())->இலக்கம்());
    }
    வெறுமை visit(Ast_முனை_Quadop* nodep) override {
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        nodep->எண்_Operate(newConst(nodep)->இலக்கம்(), fetchConst(nodep->lhsp())->இலக்கம்(),
                             fetchConst(nodep->rhsp())->இலக்கம்(), fetchConst(nodep->thsp())->இலக்கம்(),
                             fetchConst(nodep->fhsp())->இலக்கம்());
    }
    வெறுமை visit(AstLogAnd* nodep) override {
        // Need to short circuit
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        எனில் (m_checkOnly) {
            iterateChildrenConst(nodep);
        } இல்லையெனில் {
            iterateConst(nodep->lhsp());
            எனில் (!optimizable()) திரும்பு;
            எனில் (fetchConst(nodep->lhsp())->இலக்கம்().isNeqZero()) {
                iterateConst(nodep->rhsp());
                எனில் (!optimizable()) திரும்பு;
                new_மதிப்பு(nodep, fetch_மதிப்பு(nodep->rhsp()));
            } இல்லையெனில் {
                new_மதிப்பு(nodep, fetch_மதிப்பு(nodep->lhsp()));  // a zero
            }
        }
    }
    வெறுமை visit(AstLogOr* nodep) override {
        // Need to short circuit
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        எனில் (m_checkOnly) {
            iterateChildrenConst(nodep);
        } இல்லையெனில் {
            iterateConst(nodep->lhsp());
            எனில் (!optimizable()) திரும்பு;
            எனில் (fetchConst(nodep->lhsp())->இலக்கம்().isNeqZero()) {
                new_மதிப்பு(nodep, fetch_மதிப்பு(nodep->lhsp()));  // a one
            } இல்லையெனில் {
                iterateConst(nodep->rhsp());
                எனில் (!optimizable()) திரும்பு;
                new_மதிப்பு(nodep, fetch_மதிப்பு(nodep->rhsp()));
            }
        }
    }
    வெறுமை visit(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        எனில் (m_checkOnly) {
            iterateChildrenConst(nodep);
        } இல்லையெனில் {
            iterateConst(nodep->lhsp());
            எனில் (!optimizable()) திரும்பு;
            எனில் (fetchConst(nodep->lhsp())->இலக்கம்().isEqZero()) {
                மாறிலி AstConst cnst{nodep->fileline(), AstConst::Widthed_மதிப்பு{}, 1, 1};  // a one
                new_மதிப்பு(nodep, &cnst);  // a one
            } இல்லையெனில் {
                iterateConst(nodep->rhsp());
                எனில் (!optimizable()) திரும்பு;
                new_மதிப்பு(nodep, fetch_மதிப்பு(nodep->rhsp()));
            }
        }
    }
    வெறுமை visit(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        எனில் (m_checkOnly) {
            iterateChildrenConst(nodep);
        } இல்லையெனில் {
            iterateConst(nodep->condp());
            எனில் (!optimizable()) திரும்பு;
            எனில் (fetchConst(nodep->condp())->இலக்கம்().isNeqZero()) {
                iterateConst(nodep->thenp());
                எனில் (!optimizable()) திரும்பு;
                new_மதிப்பு(nodep, fetch_மதிப்பு(nodep->thenp()));
            } இல்லையெனில் {
                iterateConst(nodep->elsep());
                எனில் (!optimizable()) திரும்பு;
                new_மதிப்பு(nodep, fetch_மதிப்பு(nodep->elsep()));
            }
        }
    }

    வெறுமை handleAssignElement(Ast_முனை_Assign* nodep, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* மதிப்பு_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::vector<Ast_முனை_கோவை*> selps;
        Ast_முனை_கோவை* fromp = lhsp;
        போது (true) {
            எனில் (AstArraySel* மாறிலி selp = VN_CAST(fromp, ArraySel)) {
                check_முனை_Info(selp);
                iterateAndNextConstNull(selp->bitp());  // Bit index
                selps.தள்ளு_back(selp);
                fromp = selp->fromp();
            } இல்லையெனில் எனில் (AstStructSel* மாறிலி selp = VN_CAST(fromp, StructSel)) {
                check_முனை_Info(selp);
                selps.தள்ளு_back(selp);
                fromp = selp->fromp();
            } இல்லையெனில் {
                நிறுத்து;
            }
        }
        AstVarRef* மாறிலி varrefp = VN_CAST(fromp, VarRef);
        எனில் (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            திரும்பு;
        }
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        Ast_முனை* மாறிலி vscp = varOrScope(varrefp);
        Ast_முனை_கோவை* rootp = fetch_Out_மதிப்பு_Null(vscp);
        எனில் (!rootp) rootp = fetch_மதிப்பு_Null(vscp);
        எனில் (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            திரும்பு;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_முனை_கோவை* itemp = rootp;
        க்கு (Ast_முனை_கோவை* மாறிலி selp : vlstd::reverse_view(selps)) {
            எனில் (மாறிலி AstArraySel* மாறிலி aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* மாறிலி consp = VN_AS(itemp, ConsArray);
                மாறிலி uint32_t index = fetchConst(aselp->bitp())->toUInt();
                மாறிலி முழு elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                எனில் (index >= static_cast<uint32_t>(elements)) {
                    திரும்பு;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(index);
                எனில் (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_மரம்(false);
                    consp->valuep(index, itemp);
                }
            } இல்லையெனில் {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_முனை_கோவை* மாறிலி valuep = newTrackedClone(fetch_மதிப்பு(மதிப்பு_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.தள்ளு_back(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        assign_Out_மதிப்பு(nodep, vscp, rootp);
    }
    வெறுமை handleAssignSel(Ast_முனை_Assign* nodep, AstSel* selp, Ast_முனை_கோவை* மதிப்பு_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_எண் lsb{nodep};
        handleAssignSelRecurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_முனை* மாறிலி vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        எனில் (AstConst* மாறிலி vscpnump = fetchOutConstNull(vscp)) {
            outconstp = vscpnump;
        } இல்லையெனில் எனில் (AstConst* மாறிலி vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } இல்லையெனில் {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_மதிப்பு{},
                                     varrefp->varp()->widthMin(), 0};
            எனில் (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->இலக்கம்().setAllBits0();
            } இல்லையெனில் {
                outconstp->இலக்கம்().setAllBitsX();
            }
            m_reclaimValuesp.emplace_back(outconstp);
        }
        outconstp->இலக்கம்().opSelInto(fetchConst(மதிப்பு_Fromp)->இலக்கம்(), lsb, selp->widthConst());
        assign_Out_மதிப்பு(nodep, vscp, outconstp);
    }
    வெறுமை handleAssignSelRecurse(Ast_முனை_Assign* nodep, AstSel* selp, AstVarRef*& outVarrefpRef,
                                V3_எண்& lsbRef, முழு depth) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_முனை_Info(selp);
        iterateAndNextConstNull(selp->lsbp());  // Bit index
        எனில் (AstVarRef* மாறிலி varrefp = VN_CAST(selp->fromp(), VarRef)) {
            outVarrefpRef = varrefp;
            lsbRef = fetchConst(selp->lsbp())->இலக்கம்();
            திரும்பு;  // And presumably still optimizable()
        }
        எனில் (AstSel* மாறிலி subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_எண் sublsb{nodep};
            handleAssignSelRecurse(nodep, subselp, outVarrefpRef, sublsb /*ref*/, depth + 1);
            எனில் (optimizable()) {
                lsbRef = sublsb;
                lsbRef.opAdd(sublsb, fetchConst(selp->lsbp())->இலக்கம்());
            }
        } இல்லையெனில் {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    வெறுமை handleAssignRecurse(Ast_முனை_Assign* nodep, Ast_முனை_கோவை* lhsp, Ast_முனை_கோவை* மதிப்பு_Fromp) {
        எனில் (!optimizable()) திரும்பு;
        எனில் (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            எனில் (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                திரும்பு;
            }
            handleAssignElement(nodep, lhsp, மதிப்பு_Fromp);
        } இல்லையெனில் எனில் (AstConcat* மாறிலி selp = VN_CAST(lhsp, Concat)) {
            check_முனை_Info(selp);
            எனில் (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                திரும்பு;
            }
            // Split value into left and right concat values
            எனில் (!m_checkOnly) {
                {
                    AstConst* மாறிலி outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_மதிப்பு{},
                                       selp->lhsp()->width(), 0};
                    outconstp->இலக்கம்().opSel(fetchConst(மதிப்பு_Fromp)->இலக்கம்(),
                                           selp->lhsp()->width() + selp->rhsp()->width() + 1,
                                           selp->rhsp()->width());
                    new_மதிப்பு(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_மரம்(), outconstp);
                }
                {
                    AstConst* மாறிலி outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_மதிப்பு{},
                                       selp->rhsp()->widthMin(), 0};
                    outconstp->இலக்கம்().opSel(fetchConst(மதிப்பு_Fromp)->இலக்கம்(),
                                           selp->rhsp()->width() - 1, 0);
                    new_மதிப்பு(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_மரம்(), outconstp);
                }
            }
            handleAssignRecurse(nodep, selp->lhsp(), selp->lhsp());
            handleAssignRecurse(nodep, selp->rhsp(), selp->rhsp());
        } இல்லையெனில் எனில் (AstReplicate* மாறிலி selp = VN_CAST(lhsp, Replicate)) {
            check_முனை_Info(selp);
            iterateAndNextConstNull(selp->countp());
            AstConst* மாறிலி countp = VN_CAST(selp->countp(), Const);
            எனில் (!countp || !countp->இலக்கம்().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                திரும்பு;
            }
            handleAssignRecurse(nodep, selp->srcp(), மதிப்பு_Fromp);
        } இல்லையெனில் எனில் (AstSel* மாறிலி selp = VN_CAST(lhsp, Sel)) {
            எனில் (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                திரும்பு;
            }
            handleAssignSel(nodep, selp, மதிப்பு_Fromp);
        } இல்லையெனில் எனில் (VN_IS(lhsp, VarRef)) {
            எனில் (m_checkOnly) {
                iterateAndNextConstNull(lhsp);
            } இல்லையெனில் {
                Ast_முனை* மாறிலி vscp = varOrScope(VN_CAST(lhsp, VarRef));
                assign_Out_மதிப்பு(nodep, vscp, fetch_மதிப்பு(மதிப்பு_Fromp));
            }
        } இல்லையெனில் {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);

        VL_RESTORER(m_inDlyAssign);

        எனில் (VN_IS(nodep, AssignForce)) {
            clearOptimizable(nodep, "Force");
        } இல்லையெனில் எனில் (VN_IS(nodep, AssignDly)) {
            எனில் (m_anyAssignComb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignDly = true;
            m_inDlyAssign = true;
        } இல்லையெனில் {
            எனில் (m_anyAssignDly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_anyAssignComb = true;
        }

        எனில் (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            திரும்பு;
        }

        iterateAndNextConstNull(nodep->rhsp());  // Value to assign
        handleAssignRecurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    வெறுமை visit(AstArraySel* nodep) override {
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (மாறிலி AstConsArray* மாறிலி consp = VN_CAST(fetch_மதிப்பு_Null(nodep->fromp()), ConsArray)) {
            மாறிலி AstConst* மாறிலி indexp = fetchConst(nodep->bitp());
            மாறிலி uint32_t offset = indexp->இலக்கம்().toUInt();
            Ast_முனை_கோவை* மாறிலி itemp = consp->valuep(offset);
            எனில் (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::to_string(offset));
            } இல்லையெனில் {
                set_மதிப்பு(nodep, itemp);
            }
        } இல்லையெனில் {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    வெறுமை visit(AstStructSel* nodep) override {
        எனில் (!m_params) {
            bad_முனை_Type(nodep);
            திரும்பு;
        }
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        AstConsStruct* மாறிலி consp = VN_AS(fetch_மதிப்பு(nodep->fromp()), ConsStruct);
        set_மதிப்பு(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    வெறுமை visit(AstSliceSel* nodep) override {
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        // Fetch the base constant array
        எனில் (மாறிலி AstConsArray* மாறிலி consp = VN_CAST(fetch_மதிப்பு_Null(nodep->fromp()), ConsArray)) {
            மாறிலி VNumRange& sliceRange = nodep->declRange();
            மாறிலி uint32_t sliceElements = sliceRange.elements();
            மாறிலி முழு sliceLo = sliceRange.lo();
            // Use this node's dtype for the slice array
            Ast_முனை_DType* மாறிலி dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* மாறிலி newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            க்கு (uint32_t idx = 0; idx < sliceElements; ++idx) {
                மாறிலி uint32_t baseIdx = sliceLo + idx;
                newConsp->valuep(idx, consp->valuep(baseIdx)->clone_மரம்(false));
            }
            // Assign the new constant array and track it for later deletion
            set_மதிப்பு(nodep, newConsp);
            m_reclaimValuesp.தள்ளு_back(newConsp);
        } இல்லையெனில் {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    வெறுமை visit(AstBegin* nodep) override {
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstCase* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        UINFO(5, "   CASE " << nodep);
        check_முனை_Info(nodep);
        எனில் (m_checkOnly) {
            iterateChildrenConst(nodep);
        } இல்லையெனில் எனில் (optimizable()) {
            iterateAndNextConstNull(nodep->exprp());
            bool hit = false;
            க்கு (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                எனில் (!itemp->isDefault()) {
                    க்கு (Ast_முனை* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        எனில் (hit) நிறுத்து;
                        iterateAndNextConstNull(ep);
                        எனில் (optimizable()) {
                            V3_எண் match{nodep, 1};
                            match.opEq(fetchConst(nodep->exprp())->இலக்கம்(), fetchConst(ep)->இலக்கம்());
                            எனில் (match.isNeqZero()) {
                                iterateAndNextConstNull(itemp->stmtsp());
                                hit = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            க்கு (AstCaseItem* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), CaseItem)) {
                எனில் (hit) நிறுத்து;
                எனில் (itemp->isDefault()) {
                    iterateAndNextConstNull(itemp->stmtsp());
                    hit = true;
                }
            }
        }
    }

    வெறுமை visit(AstCaseItem* nodep) override {
        // Real handling is in AstNodeCase
        எனில் (jumpingOver()) திரும்பு;
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstComment*) override {}

    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_கோவை_Stmt* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        check_முனை_Info(nodep);
        iterateAndNextConstNull(nodep->stmtsp());
        எனில் (!optimizable()) திரும்பு;
        iterateAndNextConstNull(nodep->resultp());
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        new_மதிப்பு(nodep, fetch_மதிப்பு(nodep->resultp()));
    }

    வெறுமை visit(AstJumpBlock* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        iterateChildrenConst(nodep);
        எனில் (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    வெறுமை visit(AstJumpGo* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        check_முனை_Info(nodep);
        எனில் (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    வெறுமை visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        எனில் (jumpingOver()) திரும்பு;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        எனில் (!m_params) {
            bad_முனை_Type(nodep);
            திரும்பு;
        }
        check_முனை_Info(nodep);
        எனில் (m_checkOnly) {
            iterateChildrenConst(nodep);
            திரும்பு;
        }
        எனில் (!optimizable()) திரும்பு;

        size_t iter_மொத்தம் = 0;
        மாறிலி size_t iterLimit = v3Global.opt.unrollLimit();
        போது (true) {
            எனில் (iter_மொத்தம் > iterLimit) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::to_string(iterLimit));
                நிறுத்து;
            }
            ++iter_மொத்தம்;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterateAndNextConstNull(nodep->stmtsp());
            எனில் (jumpingOver()) நிறுத்து;
        }

        எனில் (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    வெறுமை visit(AstLoopTest* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        check_முனை_Info(nodep);
        iterateConst(nodep->condp());
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        எனில் (fetchConst(nodep->condp())->இலக்கம்().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    வெறுமை visit(AstStop* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_முனை_Info(nodep);
    }
    வெறுமை visit(AstFuncRef* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_முனை_Info(nodep);
        எனில் (!m_params) {
            bad_முனை_Type(nodep);
            திரும்பு;
        }
        Ast_முனை_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        எனில் (m_params) V3Width::widthParamsEdit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        எனில் (funcp->recursive()) {
            எனில் (m_recurse_மொத்தம் >= v3Global.opt.funcRecursionDepth()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::to_string(v3Global.opt.funcRecursionDepth())
                                            + " times");
                திரும்பு;
            }
            ++m_recurse_மொத்தம்;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_முனை*, Ast_முனை_கோவை*> old_மதிப்புகள்;

        எனில் (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_மதிப்புகள்](AstVar* varp) {
                எனில் (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    எனில் (Ast_முனை_கோவை* மாறிலி valuep = fetch_மதிப்பு_Null(varp)) {
                        Ast_முனை_கோவை* மாறிலி nvaluep = newTrackedClone(valuep);
                        old_மதிப்புகள்.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_மதிப்புகள்](Ast_முனை_கோவை* exprp) {
                எனில் (VN_IS(exprp, Const)) திரும்பு;  // Speed up as won't change
                எனில் (Ast_முனை_கோவை* மாறிலி valuep = fetch_மதிப்பு_Null(exprp)) {
                    Ast_முனை_கோவை* மாறிலி nvaluep = newTrackedClone(valuep);
                    old_மதிப்புகள்.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::vector<std::pair<AstVar*, Ast_முனை_கோவை*>> port_மதிப்புகள்;
        க்கு (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.end(); ++it) {
            AstVar* மாறிலி portp = it->first;
            Ast_முனை* மாறிலி pinp = it->second->exprp();
            எனில் (pinp) {  // Else too few arguments in function call - ignore it
                எனில் (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    திரும்பு;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                port_மதிப்புகள்.emplace_back(std::make_pair(portp, newTrackedClone(fetch_மதிப்பு(pinp))));
            }
        }
        // Apply value to the function
        எனில் (!m_checkOnly && optimizable())
            க்கு (மாறிலி auto& it : port_மதிப்புகள்) {
                எனில் (!m_checkOnly && optimizable()) new_மதிப்பு(it.first, it.second);
            }
        Sim_அடுக்கு_முனை அடுக்கு_முனை{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_அடுக்கு.தள்ளு_back(&அடுக்கு_முனை);
        எனில் (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                எனில் (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_அடுக்கு.எடு_back();
        மாறிலி Ast_முனை_கோவை* returnp = nullptr;
        எனில் (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_மதிப்பு(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        க்கு (மாறிலி auto& it : old_மதிப்புகள்) {
            எனில் (it.second) new_மதிப்பு(it.first, it.second);
        }
        எனில் (returnp) new_மதிப்பு(nodep, returnp);
        எனில் (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_மொத்தம் > 0, nodep, "recurse underflow");
            --m_recurse_மொத்தம்;
        }
    }

    வெறுமை visit(AstVar* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!m_params) {
            bad_முனை_Type(nodep);
            திரும்பு;
        }
    }

    வெறுமை visit(AstScopeName* /*nodep*/) override {
        எனில் (jumpingOver()) திரும்பு;
        // Ignore
    }

    வெறுமை visit(AstSFormatArg* nodep) override {
        check_முனை_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterateAndNextConstNull(nodep->exprp());
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;

        Ast_முனை* nextArgp = nodep->exprsp();
        string result;
        மாறிலி string format = nodep->text();
        auto pos = format.cbegin();
        bool inPct = false;
        string width;
        க்கு (; pos != format.cend(); ++pos) {
            எனில் (!inPct && pos[0] == '%') {
                inPct = true;
                width = "";
            } இல்லையெனில் எனில் (!inPct) {  // Normal text
                result += *pos;
            } இல்லையெனில் {  // Format character
                எனில் (std::isdigit(pos[0])) {
                    width += pos[0];
                    தொடர்;
                }

                inPct = false;

                எனில் (V3_எண்::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_முனை* மாறிலி argp = nextArgp;
                    nextArgp = nextArgp->nextp();
                    AstSFormatArg* மாறிலி fargp = VN_CAST(argp, SFormatArg);
                    Ast_முனை* மாறிலி subargp = fargp ? fargp->exprp() : argp;
                    AstConst* மாறிலி constp = fetchConstNull(subargp);
                    மாறிலி VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    எனில் (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        நிறுத்து;
                    }
                    மாறிலி string pformat = "%"s + width + pos[0];
                    result += formatAttr.isEnum()
                                  ? constp->இலக்கம்().displayedEnum(fargp, pformat)
                                  : constp->இலக்கம்().displayed(nodep, pformat, formatAttr);
                } இல்லையெனில் {
                    தேர்வு (std::tolower(pos[0])) {
                    நிலை '%': result += "%"; நிறுத்து;
                    நிலை 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        result += "%m";
                        நிறுத்து;
                    இயல்பு: clearOptimizable(nodep, "Unknown $display-like format code."); நிறுத்து;
                    }
                }
            }
        }

        AstConst* மாறிலி resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_மதிப்பு(nodep, resultConstp);
        m_reclaimValuesp.தள்ளு_back(resultConstp);
    }

    வெறுமை visit(AstDisplay* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_முனை_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        எனில் (m_params) {
            மாறிலி AstConst* மாறிலி textp = fetchConst(nodep->fmtp());
            தேர்வு (nodep->displayType()) {
            நிலை VDisplayType::DT_DISPLAY:  // FALLTHRU
            நிலை VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); நிறுத்து;
            நிலை VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); நிறுத்து;
            நிலை VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); நிறுத்து;
            நிலை VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); நிறுத்து;
            நிலை VDisplayType::DT_WRITE:  // FALLTHRU
            இயல்பு: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    வெறுமை visit(AstToStringN* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        எனில் (!optimizable()) திரும்பு;  // Accelerate
        check_முனை_Info(nodep);
        iterateChildrenConst(nodep);
        எனில் (m_checkOnly || !optimizable()) திரும்பு;
        மாறிலி std::string result = toStringRecurse(fetch_மதிப்பு(nodep->lhsp()));
        எனில் (!optimizable()) திரும்பு;
        AstConst* மாறிலி resultConstp = new AstConst{nodep->fileline(), AstConst::String{}, result};
        set_மதிப்பு(nodep, resultConstp);
        m_reclaimValuesp.தள்ளு_back(resultConstp);
    }

    வெறுமை visit(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    வெறுமை visit(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        known_Bad_முனை_Type(nodep);
    }
    வெறுமை visit(AstGetInitialRandomSeed* nodep) override { known_Bad_முனை_Type(nodep); }
    வெறுமை visit(AstMemberSel* nodep) override { known_Bad_முனை_Type(nodep); }
    வெறுமை visit(AstSampled* nodep) override { known_Bad_முனை_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (jumpingOver()) திரும்பு;
        bad_முனை_Type(nodep);
    }

    // MEMBERS - called by constructor
    வெறுமை setMode(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    வெறுமை mainGuts(Ast_முனை* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

பொது:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        setMode(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    வெறுமை clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_anyAssignComb = false;
        m_anyAssignDly = false;
        m_inDlyAssign = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_மொத்தம் = 0;
        m_data_மொத்தம் = 0;
        m_jumptargetp = nullptr;

        Ast_முனை::user1_Clear_மரம்();
        m_varAux.clear();
        ++m_constGeneration;
    }
    வெறுமை mainTableCheck(Ast_முனை* nodep) {
        setMode(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    வெறுமை mainTableEmulate(Ast_முனை* nodep) {
        setMode(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    வெறுமை mainParamEmulate(Ast_முனை* nodep) {
        setMode(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::set<Ast_முனை*> unusedRootps;
        க்கு (Ast_முனை* மாறிலி nodep : m_reclaimValuesp) {
            எனில் (!nodep->backp()) unusedRootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        க்கு (Ast_முனை* மாறிலி nodep : unusedRootps) VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
};

#endif  // Guard
