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

#నిర్వచితం_కాకపోతే VERILATOR_V3SIMULATE_H_
#నిర్వచించు VERILATOR_V3SIMULATE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Task.h"
#అన్తర్భావ "V3Width.h"

#అన్తర్భావ <deque>
#అన్తర్భావ <set>
#అన్తర్భావ <sstream>
#అన్తర్భావ <stack>
#అన్తర్భావ <string>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

//============================================================================

//######################################################################
// Simulate class functions

class Sim_దొంతర_కణుపు final {
బహిరంగ:
    // MEMBERS
    AstFuncRef* స్థిరరాశి m_funcp;
    V3TaskConnects* స్థిరరాశి m_tconnects;
    // CONSTRUCTORS
    Sim_దొంతర_కణుపు(AstFuncRef* funcp, V3TaskConnects* tconnects)
        : m_funcp{funcp}
        , m_tconnects{tconnects} {}
    ~Sim_దొంతర_కణుపు() = అప్రమేయ;
};

class SimulateVisitor VL_NOT_FINAL : బహిరంగ VNVisitorConst {
    // Simulate a node tree, returning value of variables
    // Two major operating modes:
    //   Test the tree to see if it is conformant
    //   Given a set of input values, find the output values
    // Both are done in this same visitor to reduce risk; if a visitor
    // is missing, we will not apply the optimization, rather then bomb.

అంతరంగ:
    // CONSTANTS
    స్థిర constexpr పూర్ణాంకం CALL_దొంతర_MAX = 100;

    // NODE STATE
    // Cleared on each always/assignw
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

    // AstNode::user1p()     -> See AuxAstVar via m_varAux

    గణన VarUsage : uint8_t { VU_NONE = 0, VU_LV = 1, VU_RV = 2, VU_LVDLY = 4 };

    నిర్మాణం AuxVariable final {
        // Checking:
        //  Set true to indicate tracking as lvalue/rvalue
        uint8_t usage = VU_NONE;
        // Simulating:
        //  Output value of variable (delayed assignments)
        Ast_కణుపు_సమాసం* బహిర్గతం_Valuep = nullptr;
        //  Input value of variable or node (and output for non-delayed assignments)
        Ast_కణుపు_సమాసం* valuep = nullptr;
    };

    AstUser1Allocator<Ast_కణుపు, AuxVariable> m_varAux;

    // We want to re-use allocated constants across calls to clear(), but we want to be able
    // to 'clear()' fast, so we use a generation number based allocator.
    నిర్మాణం ConstAllocator final {
        కొలత_t m_generation = 0;  // Generation as of last clear(); invalidates m_nextFree cheaply
        కొలత_t m_తదుపరి_Free = 0;  // Index of the next unused constant in m_constps
        std::deque<AstConst*> m_constps;  // Pool of allocated constants, reused across generations
        AstConst* allocate(కొలత_t currentGeneration, Ast_కణుపు* nodep) {
            ఒకవేళ (m_generation != currentGeneration) {
                m_generation = currentGeneration;
                m_తదుపరి_Free = 0;
            }
            UASSERT_OBJ(m_తదుపరి_Free <= m_constps.కొలత(), nodep, "Should only allocate at end");
            ఒకవేళ (m_తదుపరి_Free == m_constps.కొలత()) {
                m_constps.నెట్టు_వెనుక(
                    new AstConst{nodep->fileline(), AstConst::DTyped{}, nodep->dtypep()});
            }
            AstConst* స్థిరరాశి constp = m_constps[m_తదుపరి_Free++];
            constp->అంకె().nodep(nodep);
            తిరిగి constp;
        }

        ~ConstAllocator() {
            కొరకు (AstConst* స్థిరరాశి constp : m_constps) VL_DO_DANGLING(delete constp, constp);
        }
    };

    // STATE
    // Major mode
    bool m_checkOnly;  ///< Checking only (no simulation) mode
    bool m_scoped;  ///< Running with AstVarScopes instead of AstVars
    bool m_params;  ///< Doing parameter propagation
    // Checking:
    string m_whyNotOptimizable;  ///< String explaining why not optimizable or nullptr to optimize
    Ast_కణుపు* m_whyNotNodep;  ///< First node not optimizable
    bool m_any_కేటాయించు_Dly;  ///< True if found a delayed assignment
    bool m_any_కేటాయించు_Comb;  ///< True if found a non-delayed assignment
    bool m_లోపల_Dly_కేటాయించు;  ///< Under delayed assignment
    bool m_isImpure;  // Not pure
    bool m_isCoverage;  // Has coverage
    పూర్ణాంకం m_instr_లెక్క;  ///< Number of nodes
    పూర్ణాంకం m_దత్తాంశం_లెక్క;  ///< Bytes of data
    పూర్ణాంకం m_recurse_లెక్క = 0;  // Now deep in current recursion
    Ast_కణుపు* m_jumptargetp = nullptr;  // AstJumpBlock/AstLoop we are branching over
    // Simulating:
    // Allocators for constants of various data types
    std::unordered_map<స్థిరరాశి Ast_కణుపు_DType*, ConstAllocator> m_constps;
    కొలత_t m_constGeneration = 0;
    std::సదిశ<Sim_దొంతర_కణుపు*> m_call_దొంతర;  // Call stack for verbose error messages

    // Cleanup
    // V3Numbers that represents strings are a bit special and the API for
    // V3Number does not allow changing them.
    std::సదిశ<Ast_కణుపు*> m_reclaimValuesp;  // List of allocated string numbers

    // Note level 8&9 include debugging each simulation value
    VL_DEFINE_DEBUG_FUNCTIONS;

    // Potentially very slow, intended for debugging
    string pretty_సంఖ్య(స్థిరరాశి V3_సంఖ్య* nump, స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        ఒకవేళ (స్థిరరాశి AstRefDType* స్థిరరాశి refdtypep = VN_CAST(dtypep, RefDType)) {  //
            dtypep = refdtypep->skipRefp();
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి stp = VN_CAST(dtypep, కణుపు_UOr_Struct_DType)) {
            ఒకవేళ (stp->packed()) {
                std::ostringstream బహిర్గతం;
                బహిర్గతం << "'{";
                కొరకు (AstMemberDType* itemp = stp->membersp(); itemp;
                     itemp = VN_AS(itemp->nextp(), MemberDType)) {
                    స్థిరరాశి పూర్ణాంకం వెడల్పు = itemp->వెడల్పు();
                    స్థిరరాశి పూర్ణాంకం lsb = itemp->lsb();
                    స్థిరరాశి పూర్ణాంకం msb = lsb + వెడల్పు - 1;
                    V3_సంఖ్య క్షేత్రం_అంకె{nump, వెడల్పు};
                    క్షేత్రం_అంకె.opSel(*nump, msb, lsb);
                    బహిర్గతం << itemp->name() << ": ";
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి బిడ్డ_Typep = itemp->subDTypep()) {
                        బహిర్గతం << pretty_సంఖ్య(&క్షేత్రం_అంకె, బిడ్డ_Typep);
                    } లేకపోతే {
                        బహిర్గతం << క్షేత్రం_అంకె;
                    }
                    ఒకవేళ (itemp->nextp()) బహిర్గతం << ", ";
                }
                బహిర్గతం << "}";
                తిరిగి బహిర్గతం.str();
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstPackArrayDType* స్థిరరాశి arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి బిడ్డ_Typep = arrayp->subDTypep()) {
                std::ostringstream బహిర్గతం;
                బహిర్గతం << "[";
                స్థిరరాశి పూర్ణాంకం arrayElements = arrayp->elementsConst();
                కొరకు (పూర్ణాంకం element = 0; element < arrayElements; ++element) {
                    స్థిరరాశి పూర్ణాంకం వెడల్పు = బిడ్డ_Typep->వెడల్పు();
                    స్థిరరాశి పూర్ణాంకం lsb = వెడల్పు * element;
                    స్థిరరాశి పూర్ణాంకం msb = lsb + వెడల్పు - 1;
                    V3_సంఖ్య క్షేత్రం_అంకె{nump, వెడల్పు};
                    క్షేత్రం_అంకె.opSel(*nump, msb, lsb);
                    స్థిరరాశి పూర్ణాంకం arrayElem = arrayp->lo() + element;
                    బహిర్గతం << arrayElem << " = " << pretty_సంఖ్య(&క్షేత్రం_అంకె, బిడ్డ_Typep);
                    ఒకవేళ (element < arrayElements - 1) బహిర్గతం << ", ";
                }
                బహిర్గతం << "]";
                తిరిగి బహిర్గతం.str();
            }
        }
        తిరిగి nump->ascii();
    }

    // Checking METHODS
బహిరంగ:
    /// Call other-this function on all new *non-constant* var references
    మాయా శూన్య varRefCb(AstVarRef* nodep) {}

    శూన్య clearOptimizable(Ast_కణుపు* nodep /*null ok*/, స్థిరరాశి string& why) {
        //  Something bad found.  optimizable() will return false,
        //  and fetchConst should not be called or it may assert.
        ఒకవేళ (!m_whyNotNodep) {
            m_whyNotNodep = nodep;
            ఒకవేళ (debug() >= 5) {  // LCOV_EXCL_START
                UINFO_ఉపసర్గ("Clear optimizable: " << why);
                ఒకవేళ (nodep) std::cout << ": " << nodep;
                std::cout << '\n';
            }  // LCOV_EXCL_STOP
            m_whyNotOptimizable = why;
            std::ostringstream దొంతర;
            పూర్ణాంకం n = 0;
            కొరకు (స్థిరరాశి స్వయంచాలక& callstack : vlstd::తిరగేయి_view(m_call_దొంతర)) {
                స్థిరరాశి AstFuncRef* స్థిరరాశి funcp = callstack->m_funcp;
                దొంతర << "\n        " << funcp->fileline() << "... Called from '"
                      << funcp->prettyName() << "()' with parameters:";
                V3TaskConnects* tconnects = callstack->m_tconnects;
                కొరకు (V3TaskConnects::iterator conIt = tconnects->begin();
                     conIt != tconnects->చివర(); ++conIt) {
                    స్థిరరాశి AstVar* స్థిరరాశి portp = conIt->మొదటి;
                    Ast_కణుపు_సమాసం* స్థిరరాశి pinp = conIt->రెండవ->exprp();
                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = pinp->dtypep();
                    ఒకవేళ (AstConst* స్థిరరాశి valp = fetchConstNull(pinp)) {
                        దొంతర << "\n           " << portp->prettyName() << " = "
                              << pretty_సంఖ్య(&valp->అంకె(), dtypep);
                    } లేకపోతే ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి valuep = fetch_విలువ_Null(pinp)) {
                        దొంతర << "\n           " << portp->prettyName() << " = "
                              << గమ్యం_String_Recurse(valuep);
                    }
                }
                ఒకవేళ (++n > CALL_దొంతర_MAX) {
                    దొంతర << "\n           ... stack truncated";
                    ఆపు;
                }
            }
            m_whyNotOptimizable += దొంతర.str();
        }
    }
    bool optimizable() స్థిరరాశి { తిరిగి m_whyNotNodep == nullptr; }
    string why_Not_సందేశం() స్థిరరాశి { తిరిగి m_whyNotOptimizable; }
    Ast_కణుపు* whyNotNodep() స్థిరరాశి { తిరిగి m_whyNotNodep; }

    bool is_కేటాయించు_Dly() స్థిరరాశి { తిరిగి m_any_కేటాయించు_Dly; }
    bool isImpure() స్థిరరాశి { తిరిగి m_isImpure; }
    bool isCoverage() స్థిరరాశి { తిరిగి m_isCoverage; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి { తిరిగి m_instr_లెక్క; }
    పూర్ణాంకం దత్తాంశం_లెక్క() స్థిరరాశి { తిరిగి m_దత్తాంశం_లెక్క; }

    // Simulation METHODS
అంతరంగ:
    AstConst* allocConst(Ast_కణుపు* nodep) {
        // Allocate a constant with this dtype. Reuse them across a 'clear()' call for efficiency.
        తిరిగి m_constps[nodep->dtypep()].allocate(m_constGeneration, nodep);
    }

బహిరంగ:
    శూన్య new_విలువ(Ast_కణుపు* nodep, స్థిరరాశి Ast_కణుపు_సమాసం* valuep) {
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const)) {
            newConst(nodep)->అంకె().op_కేటాయించు(constp->అంకె());
            UINFO(9, "     new val " << valuep->name() << " on " << nodep);
        } లేకపోతే ఒకవేళ (fetch_విలువ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            సమితి_విలువ(nodep, newTrackedClone(const_cast<Ast_కణుపు_సమాసం*>(valuep)));
        }
    }
    శూన్య new_బహిర్గతం_విలువ(Ast_కణుపు* nodep, స్థిరరాశి Ast_కణుపు_సమాసం* valuep) {
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const)) {
            new_బహిర్గతం_Const(nodep)->అంకె().op_కేటాయించు(constp->అంకె());
            UINFO(9, "     new oval " << valuep->name() << " on " << nodep);
        } లేకపోతే ఒకవేళ (fetch_బహిర్గతం_విలువ_Null(nodep) != valuep) {
            // const_cast, as clonep() is set on valuep, but nothing should care
            సమితి_బహిర్గతం_విలువ(nodep, newTrackedClone(const_cast<Ast_కణుపు_సమాసం*>(valuep)));
        }
    }

అంతరంగ:
    Ast_కణుపు_సమాసం* newTrackedClone(Ast_కణుపు_సమాసం* nodep) {
        Ast_కణుపు_సమాసం* స్థిరరాశి newp = nodep->clone_వృక్షం(false);
        m_reclaimValuesp.నెట్టు_వెనుక(newp);
        తిరిగి newp;
    }
    AstConst* newConst(Ast_కణుపు* nodep) {
        // Set a constant value for this node
        ఒకవేళ (!VN_IS(m_varAux(nodep).valuep, Const)) {
            AstConst* స్థిరరాశి constp = allocConst(nodep);
            m_varAux(nodep).valuep = constp;
            తిరిగి constp;
        }
        తిరిగి fetchConst(nodep);
    }
    AstConst* new_బహిర్గతం_Const(Ast_కణుపు* nodep) {
        // Set a var-output constant value for this node
        ఒకవేళ (!VN_IS(m_varAux(nodep).బహిర్గతం_Valuep, Const)) {
            AstConst* స్థిరరాశి constp = allocConst(nodep);
            m_varAux(nodep).బహిర్గతం_Valuep = constp;
            తిరిగి constp;
        }
        తిరిగి fetch_బహిర్గతం_Const(nodep);
    }

బహిరంగ:
    Ast_కణుపు_సమాసం* fetch_విలువ_Null(Ast_కణుపు* nodep) { తిరిగి m_varAux(nodep).valuep; }

అంతరంగ:
    Ast_కణుపు_సమాసం* fetch_బహిర్గతం_విలువ_Null(Ast_కణుపు* nodep) { తిరిగి m_varAux(nodep).బహిర్గతం_Valuep; }
    AstConst* fetchConstNull(Ast_కణుపు* nodep) { తిరిగి VN_CAST(fetch_విలువ_Null(nodep), Const); }
    AstConst* fetch_బహిర్గతం_Const_Null(Ast_కణుపు* nodep) {
        తిరిగి VN_CAST(fetch_బహిర్గతం_విలువ_Null(nodep), Const);
    }
    Ast_కణుపు_సమాసం* fetch_విలువ(Ast_కణుపు* nodep) {
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = fetch_విలువ_Null(nodep);
        UASSERT_OBJ(valuep, nodep, "No value found for node.");
        // UINFO(9, "     fetch val " << *valuep << " on " << nodep);
        తిరిగి valuep;
    }
    AstConst* fetchConst(Ast_కణుపు* nodep) {
        AstConst* స్థిరరాశి constp = fetchConstNull(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        // UINFO(9, "     fetch num " << *constp << " on " << nodep);
        తిరిగి constp;
    }
    AstConst* fetch_బహిర్గతం_Const(Ast_కణుపు* nodep) {
        AstConst* స్థిరరాశి constp = fetch_బహిర్గతం_Const_Null(nodep);
        UASSERT_OBJ(constp, nodep, "No value found for node.");
        తిరిగి constp;
    }

బహిరంగ:
    V3_సంఖ్య* fetch_సంఖ్య_Null(Ast_కణుపు* nodep) {
        AstConst* స్థిరరాశి constp = fetchConstNull(nodep);
        ఒకవేళ (constp) తిరిగి &constp->అంకె();
        తిరిగి nullptr;
    }
    V3_సంఖ్య* fetch_బహిర్గతం_సంఖ్య_Null(Ast_కణుపు* nodep) {
        AstConst* స్థిరరాశి constp = fetch_బహిర్గతం_Const_Null(nodep);
        ఒకవేళ (constp) తిరిగి &constp->అంకె();
        తిరిగి nullptr;
    }

అంతరంగ:
    శూన్య సమితి_విలువ(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set val " << valuep->name() << " on " << nodep);
        m_varAux(nodep).valuep = valuep;
    }
    శూన్య సమితి_బహిర్గతం_విలువ(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* valuep) {
        UASSERT_OBJ(valuep, nodep, "Simulate setting null value");
        UINFO(9, "     set oval " << valuep->name() << " on " << nodep);
        m_varAux(nodep).బహిర్గతం_Valuep = valuep;
    }

    శూన్య check_కణుపు_Info(Ast_కణుపు* nodep, bool ignorePredict = false) {
        ఒకవేళ (m_checkOnly) {
            m_instr_లెక్క += nodep->instr_లెక్క();
            m_దత్తాంశం_లెక్క += nodep->వెడల్పు();
        }
        ఒకవేళ (!ignorePredict && !nodep->isPredictOptimizable()) {
            // UINFO(9, "     !predictopt " << nodep);
            clearOptimizable(nodep, "Isn't predictable");
        }
        ఒకవేళ (!nodep->is_శుద్ధ()) m_isImpure = true;
    }

    శూన్య తెలిసిన_Bad_కణుపు_Type(Ast_కణుపు* nodep) {
        // Call for node types we know we can't handle
        ఒకవేళ (jumpingOver()) తిరిగి;
        check_కణుపు_Info(nodep);
        ఒకవేళ (optimizable()) {
            clearOptimizable(nodep, "Known unhandled node type "s + nodep->typeName());
        }
    }
    శూన్య bad_కణుపు_Type(Ast_కణుపు* nodep) {
        // Call for default node types, or other node types we don't know how to handle
        check_కణుపు_Info(nodep);
        ఒకవేళ (optimizable()) {
            // Hmm, what is this then?
            // In production code, we'll just not optimize.  It should be fixed though.
            clearOptimizable(nodep,
                             "Unknown node type, perhaps missing visitor in SimulateVisitor");
#నిర్వచితమైతే VL_DEBUG
            స్థిర std::సమితి<VNType> s_typePrinted;
            స్థిరరాశి స్వయంచాలక pair = s_typePrinted.emplace(nodep->type());
            ఒకవేళ (pair.రెండవ)
                UINFO(0, "Unknown node type in SimulateVisitor: " << nodep->prettyTypeName());
#అయితే_ముగింపు
        }
    }

    Ast_కణుపు* varOrScope(AstVarRef* nodep) స్థిరరాశి {
        Ast_కణుపు* vscp;
        ఒకవేళ (m_scoped) {
            vscp = nodep->varScopep();
        } లేకపోతే {
            vscp = nodep->varp();
        }
        UASSERT_OBJ(vscp, nodep, "Not linked");
        తిరిగి vscp;
    }

    // True if current node might be jumped over - all visitors must call this up front
    bool jumpingOver() స్థిరరాశి { తిరిగి m_jumptargetp; }
    శూన్య కేటాయించు_బహిర్గతం_విలువ(స్థిరరాశి Ast_కణుపు_కేటాయించు* nodep, Ast_కణుపు* vscp, స్థిరరాశి Ast_కణుపు_సమాసం* valuep) {
        ఒకవేళ (VN_IS(nodep, కేటాయించు_Dly)) {
            // Don't do setValue, as value isn't yet visible to following statements
            new_బహిర్గతం_విలువ(vscp, valuep);
        } లేకపోతే {
            new_విలువ(vscp, valuep);
            సమితి_బహిర్గతం_విలువ(vscp, fetch_విలువ(vscp));
        }
    }

    string గమ్యం_String_Recurse(Ast_కణుపు_సమాసం* valuep) {
        // Return string representation of a value, or clearOptimizable
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి avaluep = VN_CAST(valuep, Const)) {
            తిరిగి "'h"s + avaluep->అంకె().displayed(valuep, "%0x");
        }
        ఒకవేళ (స్థిరరాశి AstConsArray* స్థిరరాశి avaluep = VN_CAST(valuep, ConsArray)) {
            string ఫలితం = "'{";
            string comma;
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి dtypep
                = VN_AS(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            కొరకు (పూర్ణాంకం n = 0; n < dtypep->elementsConst(); ++n) {
                ఫలితం += comma + గమ్యం_String_Recurse(avaluep->valuep(n));
                comma = ", ";
            }
            ఫలితం += "}";
            తిరిగి ఫలితం;
        }
        ఒకవేళ (స్థిరరాశి AstConsStruct* స్థిరరాశి avaluep = VN_CAST(valuep, ConsStruct)) {
            string ఫలితం = "'{";
            string comma;
            కొరకు (స్థిరరాశి Ast_Cons_Struct_అంశం* itemp = avaluep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_అంశం)) {
                ఫలితం
                    += comma + itemp->memberp()->name() + ":" + గమ్యం_String_Recurse(itemp->valuep());
                comma = ", ";
            }
            ఫలితం += "}";
            తిరిగి ఫలితం;
        }
        clearOptimizable(valuep, "Cannot convert to string");  // LCOV_EXCL_LINE
        తిరిగి "";
    }

    // Return a new value of the initial value of a variable of the given type, or nullptr if
    // the type isn't supported
    Ast_కణుపు_సమాసం* newInitValuep(Ast_కణుపు* nodep, Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి arrayp = VN_CAST(dtypep, UnpackArrayDType)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి elementp = newInitValuep(nodep, arrayp->subDTypep());
            తిరిగి elementp ? new AstConsArray{nodep->fileline(), arrayp, elementp} : nullptr;
        }
        AstStructDType* స్థిరరాశి structp = VN_CAST(dtypep, StructDType);
        ఒకవేళ (structp && !structp->packed()) {
            AstConsStruct* స్థిరరాశి newp = new AstConsStruct{nodep->fileline(), structp};
            కొరకు (AstMemberDType* memberp = structp->membersp(); memberp;
                 memberp = VN_AS(memberp->nextp(), MemberDType)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి memberValuep = newInitValuep(nodep, memberp->subDTypep());
                ఒకవేళ (!memberValuep) {
                    VL_DO_DANGLING(newp->delete_వృక్షం(), newp);
                    తిరిగి nullptr;
                }
                newp->కూడు_Itemsp(new Ast_Cons_Struct_అంశం{nodep->fileline(), memberp, memberValuep});
            }
            తిరిగి newp;
        }
        ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = dtypep->basicp()) {
            AstConst* స్థిరరాశి constp
                = new AstConst{nodep->fileline(), AstConst::Widthed_విలువ{}, basicp->వెడల్పు_Min(), 0};
            ఒకవేళ (basicp->isZeroInit()) {
                constp->అంకె() = V3_సంఖ్య{nodep, basicp};
            } లేకపోతే {
                constp->అంకె().సమితి_All_బిట్లు_X();
            }
            తిరిగి constp;
        }
        తిరిగి nullptr;
    }
    శూన్య initVar(AstVar* nodep) {
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి valuep = newInitValuep(nodep, nodep->dtypep())) {
            new_విలువ(nodep, valuep);
            VL_DO_DANGLING(valuep->delete_వృక్షం(), valuep);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override {
        // Sensitivities aren't inputs per se; we'll keep our tree under the same sens.
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        UASSERT_OBJ(nodep->varp(), nodep, "Unlinked");
        iterateChildrenConst(nodep->varp());
        Ast_కణుపు* స్థిరరాశి vscp = varOrScope(nodep);

        // We can't have non-delayed assignments with same value on LHS and RHS
        // as we don't figure out variable ordering.
        // Delayed is OK though, as we'll decode the next state separately.
        ఒకవేళ (!VN_IS(nodep->varp()->dtypeSkipRefp(), BasicDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), PackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)
            && !VN_IS(nodep->varp()->dtypeSkipRefp(), కణుపు_UOr_Struct_DType))
            clearOptimizable(nodep, "Array references/not basic");
        ఒకవేళ (nodep->access().isWriteOrRW()) {
            ఒకవేళ (m_లోపల_Dly_కేటాయించు) {
                ఒకవేళ (!(m_varAux(vscp).usage & VU_LVDLY)) {
                    m_varAux(vscp).usage |= VU_LVDLY;
                    ఒకవేళ (m_checkOnly) varRefCb(nodep);
                }
            } లేకపోతే {  // nondly asn
                ఒకవేళ (!(m_varAux(vscp).usage & VU_LV)) {
                    ఒకవేళ (!m_params && (m_varAux(vscp).usage & VU_RV)) {
                        clearOptimizable(nodep, "Var read & write");
                    }
                    m_varAux(vscp).usage |= VarUsage::VU_LV;
                    ఒకవేళ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ఒకవేళ (nodep->access().isReadOrRW()) {
            ఒకవేళ (!(m_varAux(vscp).usage & VU_RV)) {
                ఒకవేళ (!m_params && (m_varAux(vscp).usage & VU_LV)) {
                    clearOptimizable(nodep, "Var write & read");
                }
                m_varAux(vscp).usage |= VU_RV;
                స్థిరరాశి bool varIsConst = (nodep->varp()->isConst() || nodep->varp()->isParam())
                                        && nodep->varp()->valuep();
                స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి valuep
                    = varIsConst ? fetch_విలువ_Null(nodep->varp()->valuep()) : nullptr;
                // Propagate PARAM constants for constant function analysis
                ఒకవేళ (varIsConst && valuep) {
                    ఒకవేళ (!m_checkOnly && optimizable()) new_విలువ(vscp, valuep);
                } లేకపోతే {
                    ఒకవేళ (m_checkOnly) varRefCb(nodep);
                }
            }
        }
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;  // Not simulating
        UASSERT_OBJ(nodep->access().isReadOnly(), nodep,
                    "LHS varref should be handled in AstAssign visitor.");
        {
            // Return simulation value - copy by reference instead of value for speed
            Ast_కణుపు_సమాసం* valuep = fetch_విలువ_Null(vscp);
            ఒకవేళ (!valuep) {
                ఒకవేళ (m_params) {
                    clearOptimizable(
                        nodep, "Language violation: reference to non-function-local variable");
                } లేకపోతే {
                    nodep->v3fatalSrc(
                        "Variable value should have been set before any visitor called.");
                }
                valuep = allocConst(nodep);  // Any value; just so recover from error
            }
            సమితి_విలువ(nodep, valuep);
        }
    }
    శూన్య సందర్శించు(AstVarXRef* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (m_scoped) {
            bad_కణుపు_Type(nodep);
            తిరిగి;
        }
        clearOptimizable(nodep, "Language violation: Dotted hierarchical references not "
                                "allowed in constant functions");
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!m_params) {
            bad_కణుపు_Type(nodep);
            తిరిగి;
        }
        ఒకవేళ (nodep->dpiImport()) {
            ఒకవేళ (m_params) {
                nodep->v3error("Constant function may not be DPI import (IEEE 1800-2023 13.4.3)");
            }
            clearOptimizable(nodep, "DPI import functions aren't simulatable");
        }
        ఒకవేళ (nodep->underGenerate()) {
            nodep->v3error(
                "Constant function may not be declared under generate (IEEE 1800-2023 13.4.3)");
            clearOptimizable(nodep, "Constant function called under generate");
        }
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstInitialAutomaticStmt* nodep) override {
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstInitialStaticStmt* nodep) override {
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstInitialStatic* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!m_params) {
            bad_కణుపు_Type(nodep);
            తిరిగి;
        }
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        UINFO(5, "   IF " << nodep);
        check_కణుపు_Info(nodep);
        ఒకవేళ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } లేకపోతే {
            iterate_And_తదుపరి_Const_Null(nodep->condp());
            ఒకవేళ (optimizable()) {
                ఒకవేళ (fetchConst(nodep->condp())->అంకె().isNeqZero()) {
                    iterate_And_తదుపరి_Const_Null(nodep->thensp());
                } లేకపోతే {
                    iterate_And_తదుపరి_Const_Null(nodep->elsesp());
                }
            }
        }
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        check_కణుపు_Info(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        new_విలువ(nodep, nodep);
    }
    // An aggregate value is a tree of only AstConst, AstConsArray and AstConsStruct
    // nodes, so the value of an element or member is its subtree
    శూన్య సందర్శించు(AstConsArray* nodep) override {
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        AstConsArray* స్థిరరాశి valuep = new AstConsArray{
            nodep->fileline(), nodep->dtypep(),
            nodep->defaultp() ? fetch_విలువ(nodep->defaultp())->clone_వృక్షం(false) : nullptr};
        కొరకు (స్థిరరాశి Ast_Cons_Array_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Array_అంశం)) {
            valuep->valuep(itemp->సూచీ(), fetch_విలువ(itemp->valuep())->clone_వృక్షం(false));
        }
        m_reclaimValuesp.నెట్టు_వెనుక(valuep);
        సమితి_విలువ(nodep, valuep);
    }
    శూన్య సందర్శించు(AstConsStruct* nodep) override {
        ఒకవేళ (!m_params) {
            bad_కణుపు_Type(nodep);
            తిరిగి;
        }
        check_కణుపు_Info(nodep);
        కొరకు (Ast_Cons_Struct_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_అంశం)) {
            iterateConst(itemp->valuep());
        }
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        AstConsStruct* స్థిరరాశి valuep = new AstConsStruct{
            nodep->fileline(), VN_AS(nodep->dtypep()->skipRefp(), StructDType)};
        కొరకు (Ast_Cons_Struct_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Cons_Struct_అంశం)) {
            valuep->కూడు_Itemsp(
                new Ast_Cons_Struct_అంశం{itemp->fileline(), itemp->memberp(),
                                      fetch_విలువ(itemp->valuep())->clone_వృక్షం(false)});
        }
        m_reclaimValuesp.నెట్టు_వెనుక(valuep);
        సమితి_విలువ(nodep, valuep);
    }
    శూన్య సందర్శించు(Ast_Cvt_Unpack_గమ్యం_Unpack* nodep) override {
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        // An array value is an AstConsArray, so V3Const folds the conversion
        Ast_కణుపు_సమాసం* స్థిరరాశి cvtp = new Ast_Cvt_Unpack_గమ్యం_Unpack{
            nodep->fileline(), fetch_విలువ(nodep->fromp())->clone_వృక్షం(false), nodep->dtypep()};
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = V3Const::constifyEdit(cvtp);
        m_reclaimValuesp.నెట్టు_వెనుక(valuep);
        సమితి_విలువ(nodep, valuep);
    }
    శూన్య సందర్శించు(Ast_Cons_Array_అంశం* nodep) override {
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం_Ref* nodep) override {
        check_కణుపు_Info(nodep);
        UASSERT_OBJ(nodep->itemp(), nodep, "Not linked");
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        Ast_కణుపు* స్థిరరాశి valuep = nodep->itemp()->valuep();
        ఒకవేళ (valuep) {
            iterate_And_తదుపరి_Const_Null(valuep);
            ఒకవేళ (!optimizable()) తిరిగి;
            new_విలువ(nodep, fetch_విలువ(valuep));
        } లేకపోతే {
            clearOptimizable(nodep, "No value found for enum item");  // LCOV_EXCL_LINE
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Uniop* nodep) override {
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        nodep->సంఖ్య_నిర్వహించు(newConst(nodep)->అంకె(), fetchConst(nodep->lhsp())->అంకె());
    }
    // Set resultr to whether the aggregate values are equal, like a logical and of comparing
    // each element or member. V3Width allows comparing only values of matching types, so the
    // values have the same shape, and an array value is of an unpacked array, as associative
    // and dynamic arrays and queues are not values here.
    శూన్య aggregateEqRecurse(V3_సంఖ్య& resultr, Ast_కణుపు* nodep, bool caseEq, Ast_కణుపు_సమాసం* lhsp,
                            Ast_కణుపు_సమాసం* rhsp) {
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి lconstp = VN_CAST(lhsp, Const)) {
            స్థిరరాశి V3_సంఖ్య& lnum = lconstp->అంకె();
            స్థిరరాశి V3_సంఖ్య& rnum = VN_AS(rhsp, Const)->అంకె();
            V3_సంఖ్య eq{nodep, 1};
            ఒకవేళ (lnum.isDouble()) {
                eq.opEqD(lnum, rnum);
            } లేకపోతే ఒకవేళ (lnum.isString()) {
                eq.opEqN(lnum, rnum);
            } లేకపోతే ఒకవేళ (caseEq) {
                eq.opCaseEq(lnum, rnum);
            } లేకపోతే {
                eq.opEq(lnum, rnum);
            }
            స్థిరరాశి V3_సంఖ్య prev = resultr;
            resultr.opLogAnd(prev, eq);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstConsArray* స్థిరరాశి lconsp = VN_CAST(lhsp, ConsArray)) {
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి dtypep
                = VN_AS(lhsp->dtypep()->skipRefp(), UnpackArrayDType);
            కొరకు (పూర్ణాంకం n = 0; n < dtypep->elementsConst(); ++n) {
                aggregateEqRecurse(resultr, nodep, caseEq, lconsp->valuep(n),
                                   VN_AS(rhsp, ConsArray)->valuep(n));
            }
        } లేకపోతే {
            స్థిరరాశి Ast_Cons_Struct_అంశం* ritemp = VN_AS(rhsp, ConsStruct)->itemsp();
            కొరకు (స్థిరరాశి Ast_Cons_Struct_అంశం* litemp = VN_AS(lhsp, ConsStruct)->itemsp(); litemp;
                 litemp = VN_AS(litemp->nextp(), Cons_Struct_అంశం),
                                          ritemp = VN_AS(ritemp->nextp(), Cons_Struct_అంశం)) {
                aggregateEqRecurse(resultr, nodep, caseEq, litemp->valuep(), ritemp->valuep());
            }
        }
    }
    శూన్య handleAggregateCompare(Ast_కణుపు_Biop* nodep) {
        // V3Width allows only equality operators on aggregates (IEEE 1800-2023 11.3)
        స్థిరరాశి bool caseEq = VN_IS(nodep, EqCase) || VN_IS(nodep, NeqCase);
        స్థిరరాశి bool neq = VN_IS(nodep, Neq) || VN_IS(nodep, NeqCase);
        UASSERT_OBJ(caseEq || neq || VN_IS(nodep, Eq), nodep, "Unexpected operator on aggregate");
        V3_సంఖ్య& resultr = newConst(nodep)->అంకె();
        resultr.సమితి_All_Bits1();
        aggregateEqRecurse(resultr, nodep, caseEq, fetch_విలువ(nodep->lhsp()),
                           fetch_విలువ(nodep->rhsp()));
        ఒకవేళ (neq) {
            స్థిరరాశి V3_సంఖ్య eq = resultr;
            resultr.opLogNot(eq);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Biop* nodep) override {
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        // A value that is not a constant is an aggregate, and V3Width allows only
        // equality operators on aggregates (IEEE 1800-2023 11.3)
        ఒకవేళ (!VN_IS(fetch_విలువ(nodep->lhsp()), Const)) {
            handleAggregateCompare(nodep);
            తిరిగి;
        }
        AstConst* స్థిరరాశి valuep = newConst(nodep);
        nodep->సంఖ్య_నిర్వహించు(newConst(nodep)->అంకె(), fetchConst(nodep->lhsp())->అంకె(),
                             fetchConst(nodep->rhsp())->అంకె());
        // See #5490. 'numberOperate' on partially out of range select yields 'x' bits,
        // but in reality it would yield '0's without V3Table, so force 'x' bits to '0',
        // to ensure the result is the same with and without V3Table.
        ఒకవేళ (!m_params && VN_IS(nodep, Sel) && valuep->అంకె().isAnyX()) {
            స్థిరరాశి V3_సంఖ్య అంకె{valuep, valuep->వెడల్పు(), valuep->అంకె()};
            valuep->అంకె().op_బిట్లు_One(అంకె);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Triop* nodep) override {
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        nodep->సంఖ్య_నిర్వహించు(newConst(nodep)->అంకె(), fetchConst(nodep->lhsp())->అంకె(),
                             fetchConst(nodep->rhsp())->అంకె(), fetchConst(nodep->thsp())->అంకె());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Quadop* nodep) override {
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        nodep->సంఖ్య_నిర్వహించు(newConst(nodep)->అంకె(), fetchConst(nodep->lhsp())->అంకె(),
                             fetchConst(nodep->rhsp())->అంకె(), fetchConst(nodep->thsp())->అంకె(),
                             fetchConst(nodep->fhsp())->అంకె());
    }
    శూన్య సందర్శించు(AstLogAnd* nodep) override {
        // Need to short circuit
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        ఒకవేళ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } లేకపోతే {
            iterateConst(nodep->lhsp());
            ఒకవేళ (!optimizable()) తిరిగి;
            ఒకవేళ (fetchConst(nodep->lhsp())->అంకె().isNeqZero()) {
                iterateConst(nodep->rhsp());
                ఒకవేళ (!optimizable()) తిరిగి;
                new_విలువ(nodep, fetch_విలువ(nodep->rhsp()));
            } లేకపోతే {
                new_విలువ(nodep, fetch_విలువ(nodep->lhsp()));  // a zero
            }
        }
    }
    శూన్య సందర్శించు(AstLogOr* nodep) override {
        // Need to short circuit
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        ఒకవేళ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } లేకపోతే {
            iterateConst(nodep->lhsp());
            ఒకవేళ (!optimizable()) తిరిగి;
            ఒకవేళ (fetchConst(nodep->lhsp())->అంకె().isNeqZero()) {
                new_విలువ(nodep, fetch_విలువ(nodep->lhsp()));  // a one
            } లేకపోతే {
                iterateConst(nodep->rhsp());
                ఒకవేళ (!optimizable()) తిరిగి;
                new_విలువ(nodep, fetch_విలువ(nodep->rhsp()));
            }
        }
    }
    శూన్య సందర్శించు(AstLogIf* nodep) override {
        // Need to short circuit, same as (!A || B)
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        ఒకవేళ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } లేకపోతే {
            iterateConst(nodep->lhsp());
            ఒకవేళ (!optimizable()) తిరిగి;
            ఒకవేళ (fetchConst(nodep->lhsp())->అంకె().isEqZero()) {
                స్థిరరాశి AstConst cnst{nodep->fileline(), AstConst::Widthed_విలువ{}, 1, 1};  // a one
                new_విలువ(nodep, &cnst);  // a one
            } లేకపోతే {
                iterateConst(nodep->rhsp());
                ఒకవేళ (!optimizable()) తిరిగి;
                new_విలువ(nodep, fetch_విలువ(nodep->rhsp()));
            }
        }
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        // We could use above visit(AstNodeTriop), but need to do short circuiting.
        // It's also slower even O(n^2) to evaluate both sides when we
        // really only need to evaluate one side.
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        ఒకవేళ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } లేకపోతే {
            iterateConst(nodep->condp());
            ఒకవేళ (!optimizable()) తిరిగి;
            ఒకవేళ (fetchConst(nodep->condp())->అంకె().isNeqZero()) {
                iterateConst(nodep->thenp());
                ఒకవేళ (!optimizable()) తిరిగి;
                new_విలువ(nodep, fetch_విలువ(nodep->thenp()));
            } లేకపోతే {
                iterateConst(nodep->elsep());
                ఒకవేళ (!optimizable()) తిరిగి;
                new_విలువ(nodep, fetch_విలువ(nodep->elsep()));
            }
        }
    }

    శూన్య handle_కేటాయించు_Element(Ast_కణుపు_కేటాయించు* nodep, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* విలువ_Fromp) {
        // Assign an element or member, maybe nested, of a variable, by editing the variable's
        // value. Selects, from the element inwards:
        std::సదిశ<Ast_కణుపు_సమాసం*> selps;
        Ast_కణుపు_సమాసం* fromp = lhsp;
        అయితే (true) {
            ఒకవేళ (AstArraySel* స్థిరరాశి selp = VN_CAST(fromp, ArraySel)) {
                check_కణుపు_Info(selp);
                iterate_And_తదుపరి_Const_Null(selp->bitp());  // Bit index
                selps.నెట్టు_వెనుక(selp);
                fromp = selp->fromp();
            } లేకపోతే ఒకవేళ (AstStructSel* స్థిరరాశి selp = VN_CAST(fromp, StructSel)) {
                check_కణుపు_Info(selp);
                selps.నెట్టు_వెనుక(selp);
                fromp = selp->fromp();
            } లేకపోతే {
                ఆపు;
            }
        }
        AstVarRef* స్థిరరాశి varrefp = VN_CAST(fromp, VarRef);
        ఒకవేళ (!varrefp) {
            clearOptimizable(fromp, "Array select LHS isn't simple variable");
            తిరిగి;
        }
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        Ast_కణుపు* స్థిరరాశి vscp = varOrScope(varrefp);
        Ast_కణుపు_సమాసం* rootp = fetch_బహిర్గతం_విలువ_Null(vscp);
        ఒకవేళ (!rootp) rootp = fetch_విలువ_Null(vscp);
        ఒకవేళ (!rootp) {  // As initVar couldn't make an initial value of its type
            clearOptimizable(varrefp, "Assigned variable has unsupported data type");
            తిరిగి;
        }
        // Find the value of the selected element, making a defaulted array element explicit
        Ast_కణుపు_సమాసం* itemp = rootp;
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి selp : vlstd::తిరగేయి_view(selps)) {
            ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి aselp = VN_CAST(selp, ArraySel)) {
                AstConsArray* స్థిరరాశి consp = VN_AS(itemp, ConsArray);
                స్థిరరాశి uint32_t సూచీ = fetchConst(aselp->bitp())->గమ్యం_UInt();
                స్థిరరాశి పూర్ణాంకం elements
                    = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
                ఒకవేళ (సూచీ >= static_cast<uint32_t>(elements)) {
                    తిరిగి;  // Ignore a write out of bounds (IEEE 1800-2023 7.4.6)
                }
                itemp = consp->valuep(సూచీ);
                ఒకవేళ (itemp == consp->defaultp()) {  // Make a defaulted element explicit
                    itemp = itemp->clone_వృక్షం(false);
                    consp->valuep(సూచీ, itemp);
                }
            } లేకపోతే {
                itemp = VN_AS(itemp, ConsStruct)->memberValuep(selp->name());
            }
        }
        // Clone first, as the value assigned may be the old value of the element
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = newTrackedClone(fetch_విలువ(విలువ_Fromp));
        UINFO(9, "     set " << lhsp << " = " << valuep);
        itemp->replaceWith(valuep);
        m_reclaimValuesp.నెట్టు_వెనుక(itemp);
        UINFOTREE(9, rootp, "", "aggregate");
        కేటాయించు_బహిర్గతం_విలువ(nodep, vscp, rootp);
    }
    శూన్య handle_కేటాయించు_Sel(Ast_కణుపు_కేటాయించు* nodep, AstSel* selp, Ast_కణుపు_సమాసం* విలువ_Fromp) {
        AstVarRef* varrefp = nullptr;
        V3_సంఖ్య lsb{nodep};
        handle_కేటాయించు_Sel_Recurse(nodep, selp, varrefp /*ref*/, lsb /*ref*/, 0);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        UASSERT_OBJ(varrefp, nodep,
                    "Indicated optimizable, but no variable found on RHS of select");
        Ast_కణుపు* స్థిరరాశి vscp = varOrScope(varrefp);
        AstConst* outconstp = nullptr;
        ఒకవేళ (AstConst* స్థిరరాశి vscpnump = fetch_బహిర్గతం_Const_Null(vscp)) {
            outconstp = vscpnump;
        } లేకపోతే ఒకవేళ (AstConst* స్థిరరాశి vscpnump = fetchConstNull(vscp)) {
            outconstp = vscpnump;
        } లేకపోతే {  // Assignment to unassigned variable, all bits are X or 0
            outconstp = new AstConst{nodep->fileline(), AstConst::Widthed_విలువ{},
                                     varrefp->varp()->వెడల్పు_Min(), 0};
            ఒకవేళ (varrefp->varp()->basicp() && varrefp->varp()->basicp()->isZeroInit()) {
                outconstp->అంకె().సమితి_All_Bits0();
            } లేకపోతే {
                outconstp->అంకె().సమితి_All_బిట్లు_X();
            }
            m_reclaimValuesp.emplace_వెనుక(outconstp);
        }
        outconstp->అంకె().opSelInto(fetchConst(విలువ_Fromp)->అంకె(), lsb, selp->వెడల్పు_Const());
        కేటాయించు_బహిర్గతం_విలువ(nodep, vscp, outconstp);
    }
    శూన్య handle_కేటాయించు_Sel_Recurse(Ast_కణుపు_కేటాయించు* nodep, AstSel* selp, AstVarRef*& బహిర్గతం_Varrefp_Ref,
                                V3_సంఖ్య& lsbRef, పూర్ణాంకం లోతు) {
        // Recurse down to find final variable being set (outVarrefp), with
        // lsb to be eventually set on lsbRef
        check_కణుపు_Info(selp);
        iterate_And_తదుపరి_Const_Null(selp->lsbp());  // Bit index
        ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(selp->fromp(), VarRef)) {
            బహిర్గతం_Varrefp_Ref = varrefp;
            lsbRef = fetchConst(selp->lsbp())->అంకె();
            తిరిగి;  // And presumably still optimizable()
        }
        ఒకవేళ (AstSel* స్థిరరాశి subselp = VN_CAST(selp->fromp(), Sel)) {
            V3_సంఖ్య sublsb{nodep};
            handle_కేటాయించు_Sel_Recurse(nodep, subselp, బహిర్గతం_Varrefp_Ref, sublsb /*ref*/, లోతు + 1);
            ఒకవేళ (optimizable()) {
                lsbRef = sublsb;
                lsbRef.op_కూడు(sublsb, fetchConst(selp->lsbp())->అంకె());
            }
        } లేకపోతే {
            clearOptimizable(selp->fromp(), "Select LHS isn't simple variable");
        }
    }
    శూన్య handle_కేటాయించు_Recurse(Ast_కణుపు_కేటాయించు* nodep, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* విలువ_Fromp) {
        ఒకవేళ (!optimizable()) తిరిగి;
        ఒకవేళ (VN_IS(lhsp, ArraySel) || VN_IS(lhsp, StructSel)) {
            ఒకవేళ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                తిరిగి;
            }
            handle_కేటాయించు_Element(nodep, lhsp, విలువ_Fromp);
        } లేకపోతే ఒకవేళ (AstConcat* స్థిరరాశి selp = VN_CAST(lhsp, Concat)) {
            check_కణుపు_Info(selp);
            ఒకవేళ (!VN_IS(selp->rhsp()->dtypep()->skipRefp(), BasicDType)) {
                clearOptimizable(lhsp, "Assign LHS concat of non-basic type");
                తిరిగి;
            }
            // Split value into left and right concat values
            ఒకవేళ (!m_checkOnly) {
                {
                    AstConst* స్థిరరాశి outconstp
                        = new AstConst{selp->lhsp()->fileline(), AstConst::Widthed_విలువ{},
                                       selp->lhsp()->వెడల్పు(), 0};
                    outconstp->అంకె().opSel(fetchConst(విలువ_Fromp)->అంకె(),
                                           selp->lhsp()->వెడల్పు() + selp->rhsp()->వెడల్పు() + 1,
                                           selp->rhsp()->వెడల్పు());
                    new_విలువ(selp->lhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_వృక్షం(), outconstp);
                }
                {
                    AstConst* స్థిరరాశి outconstp
                        = new AstConst{selp->rhsp()->fileline(), AstConst::Widthed_విలువ{},
                                       selp->rhsp()->వెడల్పు_Min(), 0};
                    outconstp->అంకె().opSel(fetchConst(విలువ_Fromp)->అంకె(),
                                           selp->rhsp()->వెడల్పు() - 1, 0);
                    new_విలువ(selp->rhsp(), outconstp);
                    VL_DO_DANGLING(outconstp->delete_వృక్షం(), outconstp);
                }
            }
            handle_కేటాయించు_Recurse(nodep, selp->lhsp(), selp->lhsp());
            handle_కేటాయించు_Recurse(nodep, selp->rhsp(), selp->rhsp());
        } లేకపోతే ఒకవేళ (AstReplicate* స్థిరరాశి selp = VN_CAST(lhsp, Replicate)) {
            check_కణుపు_Info(selp);
            iterate_And_తదుపరి_Const_Null(selp->countp());
            AstConst* స్థిరరాశి countp = VN_CAST(selp->countp(), Const);
            ఒకవేళ (!countp || !countp->అంకె().isEqOne()) {
                clearOptimizable(selp, "Replicate LHS count isn't one");
                తిరిగి;
            }
            handle_కేటాయించు_Recurse(nodep, selp->srcp(), విలువ_Fromp);
        } లేకపోతే ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(lhsp, Sel)) {
            ఒకవేళ (!m_params) {
                clearOptimizable(lhsp, "Assign LHS has select");
                తిరిగి;
            }
            handle_కేటాయించు_Sel(nodep, selp, విలువ_Fromp);
        } లేకపోతే ఒకవేళ (VN_IS(lhsp, VarRef)) {
            ఒకవేళ (m_checkOnly) {
                iterate_And_తదుపరి_Const_Null(lhsp);
            } లేకపోతే {
                Ast_కణుపు* స్థిరరాశి vscp = varOrScope(VN_CAST(lhsp, VarRef));
                కేటాయించు_బహిర్గతం_విలువ(nodep, vscp, fetch_విలువ(విలువ_Fromp));
            }
        } లేకపోతే {
            clearOptimizable(lhsp, "Assign LHS isn't simple variable");
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);

        VL_RESTORER(m_లోపల_Dly_కేటాయించు);

        ఒకవేళ (VN_IS(nodep, కేటాయించు_Force)) {
            clearOptimizable(nodep, "Force");
        } లేకపోతే ఒకవేళ (VN_IS(nodep, కేటాయించు_Dly)) {
            ఒకవేళ (m_any_కేటాయించు_Comb) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_కేటాయించు_Dly = true;
            m_లోపల_Dly_కేటాయించు = true;
        } లేకపోతే {
            ఒకవేళ (m_any_కేటాయించు_Dly) clearOptimizable(nodep, "Mix of dly/non-dly assigns");
            m_any_కేటాయించు_Comb = true;
        }

        ఒకవేళ (VN_IS(nodep->rhsp(), CReset)) {
            initVar(VN_AS(nodep->lhsp(), VarRef)->varp());
            తిరిగి;
        }

        iterate_And_తదుపరి_Const_Null(nodep->rhsp());  // Value to assign
        handle_కేటాయించు_Recurse(nodep, nodep->lhsp(), nodep->rhsp());
        // UINFO(9, "set " << fetchConst(nodep->rhsp())->num().ascii() << " for assign "
        //  << nodep->lhsp()->name());
    }
    శూన్య సందర్శించు(AstArraySel* nodep) override {
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (స్థిరరాశి AstConsArray* స్థిరరాశి consp = VN_CAST(fetch_విలువ_Null(nodep->fromp()), ConsArray)) {
            స్థిరరాశి AstConst* స్థిరరాశి indexp = fetchConst(nodep->bitp());
            స్థిరరాశి uint32_t స్థానభ్రంశం = indexp->అంకె().గమ్యం_UInt();
            Ast_కణుపు_సమాసం* స్థిరరాశి itemp = consp->valuep(స్థానభ్రంశం);
            ఒకవేళ (!itemp) {
                clearOptimizable(nodep, "Array select out of range: " + std::గమ్యం_string(స్థానభ్రంశం));
            } లేకపోతే {
                సమితి_విలువ(nodep, itemp);
            }
        } లేకపోతే {
            clearOptimizable(nodep, "Array select of non-array");
        }
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        ఒకవేళ (!m_params) {
            bad_కణుపు_Type(nodep);
            తిరిగి;
        }
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        AstConsStruct* స్థిరరాశి consp = VN_AS(fetch_విలువ(nodep->fromp()), ConsStruct);
        సమితి_విలువ(nodep, consp->memberValuep(nodep->name()));
    }

    // Evaluate a slice of an unpacked array.  If the base value is a constant
    // AstConsArray, build a new AstConsArray representing the slice and assign
    // it as this node's value.  New index 0 corresponds to the lowest index of
    // the slice.  Otherwise, mark this node as unoptimizable.
    శూన్య సందర్శించు(Ast_ముక్క_Sel* nodep) override {
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        // Fetch the base constant array
        ఒకవేళ (స్థిరరాశి AstConsArray* స్థిరరాశి consp = VN_CAST(fetch_విలువ_Null(nodep->fromp()), ConsArray)) {
            స్థిరరాశి VNumRange& ముక్క_Range = nodep->declRange();
            స్థిరరాశి uint32_t ముక్క_Elements = ముక్క_Range.elements();
            స్థిరరాశి పూర్ణాంకం ముక్క_Lo = ముక్క_Range.lo();
            // Use this node's dtype for the slice array
            Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->dtypep()->skipRefp();
            AstConsArray* స్థిరరాశి newConsp = new AstConsArray{nodep->fileline(), dtypep};
            // Copy slice elements in ascending order
            కొరకు (uint32_t idx = 0; idx < ముక్క_Elements; ++idx) {
                స్థిరరాశి uint32_t పునాది_Idx = ముక్క_Lo + idx;
                newConsp->valuep(idx, consp->valuep(పునాది_Idx)->clone_వృక్షం(false));
            }
            // Assign the new constant array and track it for later deletion
            సమితి_విలువ(nodep, newConsp);
            m_reclaimValuesp.నెట్టు_వెనుక(newConsp);
        } లేకపోతే {
            clearOptimizable(nodep, "Slice select of non-array");
        }
    }

    శూన్య సందర్శించు(AstBegin* nodep) override {
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstCase* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        UINFO(5, "   CASE " << nodep);
        check_కణుపు_Info(nodep);
        ఒకవేళ (m_checkOnly) {
            iterateChildrenConst(nodep);
        } లేకపోతే ఒకవేళ (optimizable()) {
            iterate_And_తదుపరి_Const_Null(nodep->exprp());
            bool తగిలింది = false;
            కొరకు (Ast_Case_అంశం* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_అంశం)) {
                ఒకవేళ (!itemp->isDefault()) {
                    కొరకు (Ast_కణుపు* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                        ఒకవేళ (తగిలింది) ఆపు;
                        iterate_And_తదుపరి_Const_Null(ep);
                        ఒకవేళ (optimizable()) {
                            V3_సంఖ్య సరిపోలిక{nodep, 1};
                            సరిపోలిక.opEq(fetchConst(nodep->exprp())->అంకె(), fetchConst(ep)->అంకె());
                            ఒకవేళ (సరిపోలిక.isNeqZero()) {
                                iterate_And_తదుపరి_Const_Null(itemp->stmtsp());
                                తగిలింది = true;
                            }
                        }
                    }
                }
            }
            // Else default match
            కొరకు (Ast_Case_అంశం* itemp = nodep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Case_అంశం)) {
                ఒకవేళ (తగిలింది) ఆపు;
                ఒకవేళ (itemp->isDefault()) {
                    iterate_And_తదుపరి_Const_Null(itemp->stmtsp());
                    తగిలింది = true;
                }
            }
        }
    }

    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        // Real handling is in AstNodeCase
        ఒకవేళ (jumpingOver()) తిరిగి;
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstComment*) override {}

    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        check_కణుపు_Info(nodep);
        iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
        ఒకవేళ (!optimizable()) తిరిగి;
        iterate_And_తదుపరి_Const_Null(nodep->resultp());
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        new_విలువ(nodep, fetch_విలువ(nodep->resultp()));
    }

    శూన్య సందర్శించు(Ast_Jump_ఖండం* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        iterateChildrenConst(nodep);
        ఒకవేళ (m_jumptargetp == nodep) {
            UINFO(5, "   JUMP DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    శూన్య సందర్శించు(AstJumpGo* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        check_కణుపు_Info(nodep);
        ఒకవేళ (!m_checkOnly) {
            UINFO(5, "   JUMP GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->blockp();
        }
    }
    శూన్య సందర్శించు(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        ఒకవేళ (jumpingOver()) తిరిగి;
        UINFO(5, "   LOOP " << nodep);
        // Doing lots of loops is slow, so only for parameters
        ఒకవేళ (!m_params) {
            bad_కణుపు_Type(nodep);
            తిరిగి;
        }
        check_కణుపు_Info(nodep);
        ఒకవేళ (m_checkOnly) {
            iterateChildrenConst(nodep);
            తిరిగి;
        }
        ఒకవేళ (!optimizable()) తిరిగి;

        కొలత_t iter_లెక్క = 0;
        స్థిరరాశి కొలత_t iter_హద్దు = v3Global.opt.unroll_హద్దు();
        అయితే (true) {
            ఒకవేళ (iter_లెక్క > iter_హద్దు) {
                clearOptimizable(nodep, "Loop simulation took too long; probably this is an "
                                        "infinite loop, otherwise set '--unroll-limit' above "
                                            + std::గమ్యం_string(iter_హద్దు));
                ఆపు;
            }
            ++iter_లెక్క;

            UINFO(5, "    LOOP-ITER " << nodep);
            iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
            ఒకవేళ (jumpingOver()) ఆపు;
        }

        ఒకవేళ (m_jumptargetp == nodep) {
            UINFO(5, "   LOOP TEST DONE " << nodep);
            m_jumptargetp = nullptr;
        }
    }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        check_కణుపు_Info(nodep);
        iterateConst(nodep->condp());
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        ఒకవేళ (fetchConst(nodep->condp())->అంకె().isEqZero()) {
            UINFO(5, "   LOOP TEST GO " << nodep);
            UASSERT_OBJ(!m_jumptargetp, nodep, "Jump inside jump");
            m_jumptargetp = nodep->loopp();
        }
    }

    శూన్య సందర్శించు(AstStop* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (m_params) {  // This message seems better than an obscure $stop
            // The spec says $stop is just ignored, it seems evil to ignore assertions
            clearOptimizable(
                nodep,
                "$stop executed during function constification; maybe indicates assertion firing");
        }
        check_కణుపు_Info(nodep);
    }
    శూన్య సందర్శించు(AstFuncRef* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        UINFO(5, "   FUNCREF " << nodep);
        check_కణుపు_Info(nodep);
        ఒకవేళ (!m_params) {
            bad_కణుపు_Type(nodep);
            తిరిగి;
        }
        Ast_కణుపు_FTask* funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");
        ఒకవేళ (m_params) V3_వెడల్పు::వెడల్పు_Params_Edit(funcp);
        VL_DANGLING(funcp);  // Make sure we've sized the function
        funcp = nodep->taskp();
        UASSERT_OBJ(funcp, nodep, "Not linked");

        ఒకవేళ (funcp->recursive()) {
            ఒకవేళ (m_recurse_లెక్క >= v3Global.opt.func_Recursion_లోతు()) {
                clearOptimizable(funcp, "Constant function recursed more than "s
                                            + std::గమ్యం_string(v3Global.opt.func_Recursion_లోతు())
                                            + " times");
                తిరిగి;
            }
            ++m_recurse_లెక్క;
        }

        // Values from previous call, so can save to stack
        // The "stack" is this visit function's local stack, as this visit is itself recursing
        std::map<Ast_కణుపు*, Ast_కణుపు_సమాసం*> old_విలువలు;

        ఒకవేళ (funcp->recursive() && !m_checkOnly) {
            // Save local automatics
            funcp->foreach([this, &old_విలువలు](AstVar* varp) {
                ఒకవేళ (varp->lifetime().isAutomatic()) {  // This also does function's I/O
                    ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి valuep = fetch_విలువ_Null(varp)) {
                        Ast_కణుపు_సమాసం* స్థిరరాశి nvaluep = newTrackedClone(valuep);
                        old_విలువలు.emplace(varp, nvaluep);
                    }
                }
            });
            // Save every expression value, as might be in middle of expression
            // that calls recursively back to this same function.
            // This is much heavier-weight then likely is needed, in theory
            // we could look at the visit stack to determine what nodes
            // need save-restore, but that is difficult to get right, and
            // recursion is rare.
            funcp->foreach([this, &old_విలువలు](Ast_కణుపు_సమాసం* exprp) {
                ఒకవేళ (VN_IS(exprp, Const)) తిరిగి;  // Speed up as won't change
                ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి valuep = fetch_విలువ_Null(exprp)) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి nvaluep = newTrackedClone(valuep);
                    old_విలువలు.emplace(exprp, nvaluep);
                }
            });
        }
        // Apply function call values to function
        V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        // Must do this in two steps, eval all params, then apply them
        // Otherwise chained functions may have the wrong results
        std::సదిశ<std::pair<AstVar*, Ast_కణుపు_సమాసం*>> పోర్టు_విలువలు;
        కొరకు (V3TaskConnects::iterator it = tconnects.begin(); it != tconnects.చివర(); ++it) {
            AstVar* స్థిరరాశి portp = it->మొదటి;
            Ast_కణుపు* స్థిరరాశి pinp = it->రెండవ->exprp();
            ఒకవేళ (pinp) {  // Else too few arguments in function call - ignore it
                ఒకవేళ (portp->isWritable()) {
                    clearOptimizable(
                        portp,
                        "Language violation: Outputs/refs not allowed in constant functions");
                    తిరిగి;
                }
                // Evaluate pin value
                iterateConst(pinp);
                // Clone in case are recursing
                పోర్టు_విలువలు.emplace_వెనుక(std::make_pair(portp, newTrackedClone(fetch_విలువ(pinp))));
            }
        }
        // Apply value to the function
        ఒకవేళ (!m_checkOnly && optimizable())
            కొరకు (స్థిరరాశి స్వయంచాలక& it : పోర్టు_విలువలు) {
                ఒకవేళ (!m_checkOnly && optimizable()) new_విలువ(it.మొదటి, it.రెండవ);
            }
        Sim_దొంతర_కణుపు దొంతర_కణుపు{nodep, &tconnects};
        // cppcheck-suppress danglingLifetime
        m_call_దొంతర.నెట్టు_వెనుక(&దొంతర_కణుపు);
        ఒకవేళ (!m_checkOnly) {
            // Clear output variable
            initVar(VN_CAST(funcp->fvarp(), Var));
            // Clear other automatic variables
            funcp->foreach([this](AstVar* varp) {
                ఒకవేళ (varp->lifetime().isAutomatic() && (!varp->isIO() || varp->isFuncReturn()))
                    initVar(varp);
            });
        }

        // Evaluate the function
        iterateConst(funcp);
        m_call_దొంతర.తీయి_వెనుక();
        స్థిరరాశి Ast_కణుపు_సమాసం* returnp = nullptr;
        ఒకవేళ (!m_checkOnly && optimizable()) {
            // Grab return value from output variable
            UASSERT_OBJ(funcp->fvarp(), nodep, "Function reference points at non-function");
            returnp = newTrackedClone(fetch_విలువ(funcp->fvarp()));
            UINFO(5, "func " << nodep->name() << " return = " << returnp);
        }
        // Restore local automatics (none unless recursed)
        కొరకు (స్థిరరాశి స్వయంచాలక& it : old_విలువలు) {
            ఒకవేళ (it.రెండవ) new_విలువ(it.మొదటి, it.రెండవ);
        }
        ఒకవేళ (returnp) new_విలువ(nodep, returnp);
        ఒకవేళ (funcp->recursive()) {
            UASSERT_OBJ(m_recurse_లెక్క > 0, nodep, "recurse underflow");
            --m_recurse_లెక్క;
        }
    }

    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!m_params) {
            bad_కణుపు_Type(nodep);
            తిరిగి;
        }
    }

    శూన్య సందర్శించు(AstScopeName* /*nodep*/) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        // Ignore
    }

    శూన్య సందర్శించు(AstSFormatArg* nodep) override {
        check_కణుపు_Info(nodep);
        // Constant enum names come from the dtype, not the runtime lookup.
        iterate_And_తదుపరి_Const_Null(nodep->exprp());
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;

        Ast_కణుపు* తదుపరి_Argp = nodep->exprsp();
        string ఫలితం;
        స్థిరరాశి string format = nodep->పాఠ్యం();
        స్వయంచాలక pos = format.cbegin();
        bool లోపల_Pct = false;
        string వెడల్పు;
        కొరకు (; pos != format.cend(); ++pos) {
            ఒకవేళ (!లోపల_Pct && pos[0] == '%') {
                లోపల_Pct = true;
                వెడల్పు = "";
            } లేకపోతే ఒకవేళ (!లోపల_Pct) {  // Normal text
                ఫలితం += *pos;
            } లేకపోతే {  // Format character
                ఒకవేళ (std::అంకెనా(pos[0])) {
                    వెడల్పు += pos[0];
                    కొనసాగు;
                }

                లోపల_Pct = false;

                ఒకవేళ (V3_సంఖ్య::displayedFmtHasArg(std::tolower(pos[0]), false)) {
                    Ast_కణుపు* స్థిరరాశి argp = తదుపరి_Argp;
                    తదుపరి_Argp = తదుపరి_Argp->nextp();
                    AstSFormatArg* స్థిరరాశి fargp = VN_CAST(argp, SFormatArg);
                    Ast_కణుపు* స్థిరరాశి subargp = fargp ? fargp->exprp() : argp;
                    AstConst* స్థిరరాశి constp = fetchConstNull(subargp);
                    స్థిరరాశి VFormatAttr formatAttr
                        = argp ? AstSFormatArg::formatAttrDefauled(fargp, subargp->dtypep())
                               : VFormatAttr{};
                    ఒకవేళ (!constp) {
                        clearOptimizable(nodep,
                                         "Argument for $display-like statement is not constant");
                        ఆపు;
                    }
                    స్థిరరాశి string pformat = "%"s + వెడల్పు + pos[0];
                    ఫలితం += formatAttr.isEnum()
                                  ? constp->అంకె().displayedEnum(fargp, pformat)
                                  : constp->అంకె().displayed(nodep, pformat, formatAttr);
                } లేకపోతే {
                    ఎంపిక (std::tolower(pos[0])) {
                    స్థితి '%': ఫలితం += "%"; ఆపు;
                    స్థితి 'm':
                        // This happens prior to AstScope so we don't
                        // know the scope name. Leave the %m in place.
                        ఫలితం += "%m";
                        ఆపు;
                    అప్రమేయ: clearOptimizable(nodep, "Unknown $display-like format code."); ఆపు;
                    }
                }
            }
        }

        AstConst* స్థిరరాశి ఫలితం_Constp = new AstConst{nodep->fileline(), AstConst::String{}, ఫలితం};
        సమితి_విలువ(nodep, ఫలితం_Constp);
        m_reclaimValuesp.నెట్టు_వెనుక(ఫలితం_Constp);
    }

    శూన్య సందర్శించు(AstDisplay* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        // We ignore isPredictOptimizable as $display is often in constant
        // functions and we want them to work if used with parameters
        check_కణుపు_Info(nodep, /*display:*/ true);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_params) {
            స్థిరరాశి AstConst* స్థిరరాశి textp = fetchConst(nodep->fmtp());
            ఎంపిక (nodep->displayType()) {
            స్థితి VDisplayType::DT_DISPLAY:  // FALLTHRU
            స్థితి VDisplayType::DT_INFO: v3warn(USERINFO, textp->name()); ఆపు;
            స్థితి VDisplayType::DT_ERROR: v3warn(USERERROR, textp->name()); ఆపు;
            స్థితి VDisplayType::DT_WARNING: v3warn(USERWARN, textp->name()); ఆపు;
            స్థితి VDisplayType::DT_FATAL: v3warn(USERFATAL, textp->name()); ఆపు;
            స్థితి VDisplayType::DT_WRITE:  // FALLTHRU
            అప్రమేయ: clearOptimizable(nodep, "Unexpected display type");
            }
        }
    }
    శూన్య సందర్శించు(Ast_గమ్యం_String_N* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        ఒకవేళ (!optimizable()) తిరిగి;  // Accelerate
        check_కణుపు_Info(nodep);
        iterateChildrenConst(nodep);
        ఒకవేళ (m_checkOnly || !optimizable()) తిరిగి;
        స్థిరరాశి std::string ఫలితం = గమ్యం_String_Recurse(fetch_విలువ(nodep->lhsp()));
        ఒకవేళ (!optimizable()) తిరిగి;
        AstConst* స్థిరరాశి ఫలితం_Constp = new AstConst{nodep->fileline(), AstConst::String{}, ఫలితం};
        సమితి_విలువ(nodep, ఫలితం_Constp);
        m_reclaimValuesp.నెట్టు_వెనుక(ఫలితం_Constp);
    }

    శూన్య సందర్శించు(AstCoverInc* /*nodep*/) override { m_isCoverage = true; }

    // ====
    // Known Bad
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        // Some CMethods such as size() on queues could be supported, but
        // instead we should change those methods to new Ast types so we can
        // properly dispatch them
        తెలిసిన_Bad_కణుపు_Type(nodep);
    }
    శూన్య సందర్శించు(Ast_Get_Initial_Random_విత్తనం* nodep) override { తెలిసిన_Bad_కణుపు_Type(nodep); }
    శూన్య సందర్శించు(AstMemberSel* nodep) override { తెలిసిన_Bad_కణుపు_Type(nodep); }
    శూన్య సందర్శించు(AstSampled* nodep) override { తెలిసిన_Bad_కణుపు_Type(nodep); }
    // ====
    // default
    // These types are definitely not reducible
    //   AstCoverInc, AstFinish,
    //   AstRand, AstTime, AstCCall, AstCStmt*, AstCExpr*
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (jumpingOver()) తిరిగి;
        bad_కణుపు_Type(nodep);
    }

    // MEMBERS - called by constructor
    శూన్య సమితి_బహుళకం(bool scoped, bool checkOnly, bool params) {
        m_checkOnly = checkOnly;
        m_scoped = scoped;
        m_params = params;
    }
    శూన్య mainGuts(Ast_కణుపు* nodep) {
        iterateConst(nodep);
        UASSERT_OBJ(!m_jumptargetp, m_jumptargetp, "Jump target was not found");
    }

బహిరంగ:
    // CONSTRUCTORS
    SimulateVisitor() {
        // Note AstUser#InUse ensures only one invocation exists at once
        సమితి_బహుళకం(false, false, false);
        clear();  // We reuse this structure in the main loop, so put initializers inside clear()
    }
    శూన్య clear() {
        m_whyNotOptimizable = "";
        m_whyNotNodep = nullptr;
        m_any_కేటాయించు_Comb = false;
        m_any_కేటాయించు_Dly = false;
        m_లోపల_Dly_కేటాయించు = false;
        m_isImpure = false;
        m_isCoverage = false;
        m_instr_లెక్క = 0;
        m_దత్తాంశం_లెక్క = 0;
        m_jumptargetp = nullptr;

        Ast_కణుపు::user1_Clear_వృక్షం();
        m_varAux.clear();
        ++m_constGeneration;
    }
    శూన్య main_పట్టిక_Check(Ast_కణుపు* nodep) {
        సమితి_బహుళకం(true /*scoped*/, true /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    శూన్య main_పట్టిక_Emulate(Ast_కణుపు* nodep) {
        సమితి_బహుళకం(true /*scoped*/, false /*checking*/, false /*params*/);
        mainGuts(nodep);
    }
    శూన్య mainParamEmulate(Ast_కణుపు* nodep) {
        సమితి_బహుళకం(false /*scoped*/, false /*checking*/, true /*params*/);
        mainGuts(nodep);
    }
    ~SimulateVisitor() override {
        m_constps.clear();
        // A value replaced in an aggregate may be listed twice
        std::సమితి<Ast_కణుపు*> వాడని_Rootps;
        కొరకు (Ast_కణుపు* స్థిరరాశి nodep : m_reclaimValuesp) {
            ఒకవేళ (!nodep->backp()) వాడని_Rootps.emplace(nodep);
        }
        m_reclaimValuesp.clear();
        కొరకు (Ast_కణుపు* స్థిరరాశి nodep : వాడని_Rootps) VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
};

#అయితే_ముగింపు  // Guard
