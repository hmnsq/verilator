// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Generate randomization procedures
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
// V3Randomize's Transformations:
//
// Each randomize() method call:
//      Mark class of object on which randomize() is called
// Mark all classes that inherit from previously marked classed
// Mark all classes whose instances are randomized member variables of marked classes
// Each marked class:
//      * define a virtual randomize() method that randomizes its random variables
// Each call to randomize():
//      * define __Vrandwith### functions for randomize() calls with inline constraints and
//        put then into randomized classes
//      * replace calls to randomize() that use inline constraints with calls to __Vrandwith###
//        functions
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Randomize.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3Container.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3Task.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <queue>
#అన్తర్భావ <tuple>
#అన్తర్భావ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

గణన ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

స్థిర constexpr స్థిరరాశి అక్షరం* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
స్థిర constexpr స్థిరరాశి అక్షరం* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
స్థిర AstVar* get_Rand_బహుళకం_Var_From_Class(Ast_కణుపు_Module* classp) {
    అయితే (classp) {
        ఒకవేళ (classp->user2p()) తిరిగి VN_AS(classp->user2p(), Var);
        AstClass* స్థిరరాశి cp = VN_CAST(classp, Class);
        ఒకవేళ (!cp || !cp->extendsp()) తిరిగి శూన్యసూచిక;
        classp = cp->extendsp()->classp();
    }
    తిరిగి శూన్యసూచిక;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
స్థిర AstVar* getRandomGenerator(AstClass* స్థిరరాశి classp) {
    ఒకవేళ (classp->user3p()) తిరిగి VN_AS(classp->user3p(), Var);
    ఒకవేళ (classp->extendsp()) తిరిగి getRandomGenerator(classp->extendsp()->classp());
    తిరిగి శూన్యసూచిక;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
స్థిర AstTask* get_Create_Constraint_అమరిక_Func(AstClass* classp, VMemberMap& memberMap) {
    స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి name = "__Vsetup_constraints";
    AstTask* అమరిక_All_Taskp = VN_AS(memberMap.కనుగొను_Member(classp, name), Task);
    ఒకవేళ (అమరిక_All_Taskp) తిరిగి అమరిక_All_Taskp;
    అమరిక_All_Taskp = కొత్త AstTask{classp->fileline(), "__Vsetup_constraints", శూన్యసూచిక};
    అమరిక_All_Taskp->class_పద్ధతి(నిజం);
    అమరిక_All_Taskp->isVirtual(నిజం);
    classp->కూడు_Membersp(అమరిక_All_Taskp);
    memberMap.చొప్పించు(classp, అమరిక_All_Taskp);
    తిరిగి అమరిక_All_Taskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

నిర్మాణం Rand_బహుళకం_లక్ష్యం final {
    // MEMBERS
    AstVar* స్థిరరాశి receiverp;  // Variable that is the target of the rand_mode() call
    Ast_కణుపు_సమాసం* స్థిరరాశి fromp;  // Expression from which the rand_mode() call originates
    AstClass* స్థిరరాశి classp;  // Class in which rand_mode should be set

    // METHODS
    స్థిర Rand_బహుళకం_లక్ష్యం get(Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_Module* modp) {
        ఒకవేళ (!fromp) తిరిగి {శూన్యసూచిక, శూన్యసూచిక, VN_AS(modp, Class)};
        ఒకవేళ (AstCMethodHard* స్థిరరాశి పద్ధతి_Hardp = VN_CAST(fromp, CMethodHard)) {
            తిరిగి Rand_బహుళకం_లక్ష్యం::get(పద్ధతి_Hardp->fromp(), modp);
        }
        AstVar* receiverp = శూన్యసూచిక;
        AstClass* classp = VN_CAST(modp, Class);
        ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            ఒకవేళ (receiverp->isRand()) {
                fromp = శూన్యసూచిక;
                ఒకవేళ (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } లేకపోతే ఒకవేళ (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            ఒకవేళ (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        ఒకవేళ (!receiverp->isRand()) {
            AstClassRefDType* స్థిరరాశి classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            ఒకవేళ (classRefDtp) classp = classRefDtp->classp();
        }
        తిరిగి {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

సంఘం Randomize_బహుళకం final {
    // MEMBERS
    నిర్మాణం {
        తార్కిక ఉపయోగిస్తుంది_బహుళకం : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t సూచీ : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
స్థిర Ast_కణుపు_DType* arrayElementDTypep(Ast_కణుపు_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    అయితే (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    తిరిగి dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
స్థిర తార్కిక dtype_కలిగియుంది_Real(Ast_కణుపు_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి subp = dtypep->subDTypep()) తిరిగి dtype_కలిగియుంది_Real(subp);
    ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి structp = VN_CAST(dtypep, కణుపు_UOr_Struct_DType)) {
        ఒకవేళ (structp->packed()) తిరిగి అబద్ధం;
        కొరకు (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            ఒకవేళ (dtype_కలిగియుంది_Real(memberp->subDTypep())) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = VN_CAST(dtypep, BasicDType);
    తిరిగి basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
స్థిర శూన్య checkRandTypeEligibility(Ast_కణుపు* contextp, Ast_కణుపు_DType* dtypep, తార్కిక isRandc) {
    dtypep = dtypep->skipRefp();
    ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        తిరిగి;
    }
    ఒకవేళ (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        ఒకవేళ (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        తిరిగి;
    }
    ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        తిరిగి;
    }
    ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి structp = VN_CAST(dtypep, కణుపు_UOr_Struct_DType)) {
        ఒకవేళ (structp->packed()) తిరిగి;  // Integral by construction; members checked separately
        ఒకవేళ (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            తిరిగి;
        }
        కొరకు (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            ఒకవేళ (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        తిరిగి;
    }
    స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    ఒకవేళ (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        ఒకవేళ (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        తిరిగి;
    }
    ఎంపిక (basicp->keyword()) {
    స్థితి VBasicDTypeKwd::STRING:
    స్థితి VBasicDTypeKwd::CHANDLE:
    స్థితి VBasicDTypeKwd::EVENT: {
        స్థిరరాశి అక్షరం* స్థిరరాశి articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        ఆపు;
    }
    అప్రమేయ: ఆపు;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

తరగతి RandomizeMarkVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstClass::user1()       -> bool.  Set true to indicate needs randomize processing
    //  AstNodeModule::user1()  -> bool.  Set true to indicate needs std::randomize processing
    //  AstNodeExpr::user1()    -> bool.  Set true to indicate constraint expression depending on a
    //                                    randomized variable
    //  AstVar::user1()         -> bool.  Set true to indicate needs rand_mode
    //  AstVar::user2p()        -> AstNodeModule*. Pointer to containing module
    //  AstNodeFTask::user2p()  -> AstNodeModule*. Pointer to containing module
    //  AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    ఉపయోగించి Derived_సమితి = std::unordered_సమితి<AstClass*>;
    ఉపయోగించి పునాది_గమ్యం_Derived_Map = std::unordered_map<స్థిరరాశి AstClass*, Derived_సమితి>;

    పునాది_గమ్యం_Derived_Map m_పునాది_గమ్యం_Derived_Map;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = శూన్యసూచిక;  // Current class
    Ast_కణుపు* m_constraint_సమాసం_Genp = శూన్యసూచిక;  // Current constraint or constraint if expression
    Ast_కణుపు_Module* m_modp;  // Current module
    Ast_కణుపు_Stmt* m_stmtp = శూన్యసూచిక;  // Current statement
    Ast_కణుపు_FTask_Ref* m_stdRandCallp = శూన్యసూచిక;  // Current std::randomize() call
    తార్కిక m_లోపల_Std_With = అబద్ధం;  // True when inside a 'with {}' clause
    std::సమితి<Ast_కణుపు_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = శూన్యసూచిక;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    శూన్య markGlobalConstrainedVars(AstClass* classp) {
        కొరకు (స్థిరరాశి AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : శూన్యసూచిక) {
            కొరకు (Ast_కణుపు* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* స్థిరరాశి varp = VN_CAST(memberp, Var);
                ఒకవేళ (!varp) కొనసాగు;
                ఒకవేళ (varp->rand().isRandomizable()) varp->globalConstrained(నిజం);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    తార్కిక is_Var_లోపల_Std_Randomize_వాదనలు(స్థిరరాశి AstVar* varp) స్థిరరాశి {
        ఒకవేళ (!m_లోపల_Std_With || !m_stdRandCallp) తిరిగి అబద్ధం;

        కొరకు (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            స్థిరరాశి Ast_కణుపు_సమాసం* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            అయితే (exprp) {
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(exprp, కణుపు_Var_Ref)) {
                    ఒకవేళ (varrefp->varp() == varp) తిరిగి నిజం;
                    ఆపు;
                }
                ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి memberselp = VN_CAST(exprp, MemberSel)) {
                    ఒకవేళ (memberselp->varp() == varp) తిరిగి నిజం;
                    exprp = memberselp->fromp();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstCMethodHard* స్థిరరాశి methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } లేకపోతే {
                    ఆపు;
                }
            }
        }
        తిరిగి అబద్ధం;
    }
    శూన్య markMembers(స్థిరరాశి AstClass* nodep) {
        కొరకు (స్థిరరాశి AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : శూన్యసూచిక) {
            కొరకు (Ast_కణుపు* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* స్థిరరాశి varp = VN_CAST(memberp, Var);
                ఒకవేళ (!varp) కొనసాగు;
                // If member is randomizable and of class type, mark its class
                ఒకవేళ (varp->rand().isRandomizable()) {
                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి varDtypep = varp->dtypep()->skipRefp();
                    స్థిరరాశి AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    ఒకవేళ (!classRefp) {
                        స్థిరరాశి Ast_కణుపు_DType* subDTypep = varDtypep->subDTypep();
                        ఒకవేళ (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    ఒకవేళ (classRefp) {
                        AstClass* స్థిరరాశి rclassp = classRefp->classp();
                        ఒకవేళ (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    ఒకవేళ (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        Randomize_బహుళకం rand_బహుళకం = {};
                        rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం = నిజం;
                        varp->user1(rand_బహుళకం.asUQuad);
                    }
                }
            }
        }
    }
    శూన్య markDerived(స్థిరరాశి AstClass* nodep) {
        స్థిరరాశి స్వయంచాలక it = m_పునాది_గమ్యం_Derived_Map.కనుగొను(nodep);
        ఒకవేళ (it != m_పునాది_గమ్యం_Derived_Map.చివర()) {
            కొరకు (స్వయంచాలక* classp : it->రెండవ) {
                ఒకవేళ (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    శూన్య markAllDerived() {
        కొరకు (స్థిరరాశి స్వయంచాలక& p : m_పునాది_గమ్యం_Derived_Map) {
            ఒకవేళ (p.మొదటి->user1()) markDerived(p.మొదటి);
        }
    }
    శూన్య సమితి_Package_Refs() {
        కొరకు (Ast_కణుపు_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), కణుపు_Module));
        }
    }
    శూన్య markNestedGlobalConstrainedRecurse(Ast_కణుపు* nodep) {
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
            AstVar* స్థిరరాశి varp = refp->varp();
            ఒకవేళ (varp->globalConstrained()) తిరిగి;
            varp->globalConstrained(నిజం);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(nodep, MemberSel)) {
            ఒకవేళ (memberSelp->varp()) {
                AstVar* స్థిరరాశి varp = memberSelp->varp();
                ఒకవేళ (varp->globalConstrained()) తిరిగి;
                varp->globalConstrained(నిజం);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_కణుపు_సమాసం* buildMemberSelChain(AstVarRef* మూలం_Var_Refp, స్థిరరాశి std::సదిశ<AstVar*>& మార్గం) {
        Ast_కణుపు_సమాసం* exprp = మూలం_Var_Refp->clone_వృక్షం(అబద్ధం);
        కొరకు (AstVar* memberVarp : మార్గం) {
            AstMemberSel* memberSelp
                = కొత్త AstMemberSel{మూలం_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        తిరిగి exprp;
    }

    // Process a single constraint during nested constraint cloning
    శూన్య processNestedConstraint(AstConstraint* స్థిరరాశి constrp, AstVarRef* మూలం_Var_Refp,
                                 స్థిరరాశి std::సదిశ<AstVar*>& new_మార్గం, AstClass* లక్ష్యం_Classp) {
        std::string మార్గం_ఉపసర్గ = మూలం_Var_Refp->name();
        కొరకు (AstVar* మార్గం_Member_Varp : new_మార్గం) {
            మార్గం_ఉపసర్గ += GLOBAL_CONSTRAINT_SEPARATOR + మార్గం_Member_Varp->name();
        }

        స్థిరరాశి std::string newName = మార్గం_ఉపసర్గ + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        తార్కిక isDuplicate = అబద్ధం;
        లక్ష్యం_Classp->foreachMember([&](AstClass* స్థిరరాశి, AstConstraint* స్థిరరాశి existingConstrp) {
            ఒకవేళ (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = నిజం;
            }
        });
        ఒకవేళ (isDuplicate) తిరిగి;

        AstConstraint* స్థిరరాశి cloneConstrp = constrp->clone_వృక్షం(అబద్ధం);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::సదిశ<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_కణుపు_సమాసం* exprp) {
            ఒకవేళ (AstVarRef* స్థిరరాశి varRefp = VN_CAST(exprp, VarRef)) {
                ఒకవేళ (!varRefp->varp()->isClassMember()) తిరిగి;
                Ast_కణుపు_సమాసం* స్థిరరాశి chainp = buildMemberSelChain(మూలం_Var_Refp, new_మార్గం);
                AstMemberSel* స్థిరరాశి finalSelp
                    = కొత్త AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(నెట్టు_Deletep(varRefp), varRefp);
            } లేకపోతే ఒకవేళ (AstFuncRef* స్థిరరాశి funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                ఒకవేళ (funcRefp->taskp()->class_పద్ధతి() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.నెట్టు_వెనుక(funcRefp);
                }
            }
        });
        కొరకు (AstFuncRef* funcRefp : funcRefps) {
            Ast_కణుపు_సమాసం* స్థిరరాశి chainp = buildMemberSelChain(మూలం_Var_Refp, new_మార్గం);
            AstArg* స్థిరరాశి argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlink_Fr_వెనుక_With_తదుపరి() : శూన్యసూచిక;
            Ast_పద్ధతి_Call* స్థిరరాశి callp
                = కొత్త Ast_పద్ధతి_Call{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(నెట్టు_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        లక్ష్యం_Classp->కూడు_Stmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    శూన్య cloneNestedConstraintsRecurse(AstVarRef* మూలం_Var_Refp, AstClass* classp,
                                       స్థిరరాశి std::సదిశ<AstVar*>& మార్గం_గమ్యం_Class,
                                       AstClass* లక్ష్యం_Classp) {
        కొరకు (Ast_కణుపు* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* స్థిరరాశి memberVarp = VN_CAST(memberNodep, Var);
            ఒకవేళ (!memberVarp) కొనసాగు;
            ఒకవేళ (!memberVarp->rand().isRandomizable()) కొనసాగు;
            స్థిరరాశి AstClassRefDType* స్థిరరాశి memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            ఒకవేళ (!memberClassRefp || !memberClassRefp->classp()) కొనసాగు;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::సదిశ<AstVar*> new_మార్గం = మార్గం_గమ్యం_Class;
            new_మార్గం.నెట్టు_వెనుక(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* స్థిరరాశి containingClassp, AstConstraint* స్థిరరాశి constrp) {
                    processNestedConstraint(constrp, మూలం_Var_Refp, new_మార్గం, లక్ష్యం_Classp);
                });
            cloneNestedConstraintsRecurse(మూలం_Var_Refp, nestedClassp, new_మార్గం, లక్ష్యం_Classp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    శూన్య handle_Randomize_వాదం(Ast_కణుపు_సమాసం* exprp, AstVar* స్థిరరాశి varp, స్థిరరాశి తార్కిక stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        అయితే (exprp) {
            AstVar* randVarp = శూన్యసూచిక;
            ఒకవేళ (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } లేకపోతే ఒకవేళ (AstArraySel* స్థిరరాశి arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                కొనసాగు;  // Skip ArraySel, continue traversing
            } లేకపోతే ఒకవేళ (AstStructSel* స్థిరరాశి structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                కొనసాగు;  // Skip StructSel, continue traversing
            } లేకపోతే ఒకవేళ (AstCMethodHard* స్థిరరాశి methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                కొనసాగు;
            } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(నిజం);
                varrefp->access(VAccess::READWRITE);
                exprp = శూన్యసూచిక;
            } లేకపోతే {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            ఒకవేళ (randVarp == varp) తిరిగి;
            Ast_కణుపు* backp = randVarp;
            అయితే (backp && !VN_IS(backp, కణుపు_Module)) backp = backp->backp();
            ఒకవేళ (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } లేకపోతే {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                Randomize_బహుళకం rand_బహుళకం = {};
                rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం = నిజం;
                randVarp->user1(rand_బహుళకం.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        ఒకవేళ (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            స్థిరరాశి AstClass* స్థిరరాశి basep = nodep->extendsp()->classp();
            m_పునాది_గమ్యం_Derived_Map[basep].చొప్పించు(nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        ఒకవేళ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        ఒకవేళ (nodep->name() == "rand_mode") {
            Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(nodep, పద్ధతి_Call);
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = పద్ధతి_Callp ? పద్ధతి_Callp->fromp() : శూన్యసూచిక;
            తార్కిక valid = నిజం;
            ఒకవేళ (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = అబద్ధం;
            } లేకపోతే ఒకవేళ (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = అబద్ధం;
            } లేకపోతే ఒకవేళ (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = అబద్ధం;
            } లేకపోతే ఒకవేళ (AstCMethodHard* స్థిరరాశి పద్ధతి_Hardp = VN_CAST(fromp, CMethodHard)) {
                ఒకవేళ (పద్ధతి_Hardp->పద్ధతి() == VCMethod::ARRAY_AT
                    || పద్ధతి_Hardp->పద్ధతి() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = అబద్ధం;
                } లేకపోతే {
                    పద్ధతి_Hardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            ఒకవేళ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_సమాసం)
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_సంకేతం::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = అబద్ధం;
            }
            ఒకవేళ (valid) {
                స్థిరరాశి Rand_బహుళకం_లక్ష్యం rand_బహుళకం_లక్ష్యం = Rand_బహుళకం_లక్ష్యం::get(fromp, m_classp);
                ఒకవేళ ((!rand_బహుళకం_లక్ష్యం.receiverp || !rand_బహుళకం_లక్ష్యం.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = అబద్ధం;
                } లేకపోతే ఒకవేళ (!rand_బహుళకం_లక్ష్యం.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = అబద్ధం;
                } లేకపోతే ఒకవేళ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_సమాసం)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = అబద్ధం;
                } లేకపోతే ఒకవేళ (rand_బహుళకం_లక్ష్యం.receiverp && rand_బహుళకం_లక్ష్యం.receiverp->isRand()) {
                    // Called on a rand member variable
                    Randomize_బహుళకం rand_బహుళకం = {};
                    rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం = నిజం;
                    rand_బహుళకం_లక్ష్యం.receiverp->user1(rand_బహుళకం.asUQuad);
                } లేకపోతే {
                    // Called on 'this' or a non-rand class instance
                    rand_బహుళకం_లక్ష్యం.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        ఒకవేళ (!varp->isRand()) తిరిగి;
                        Randomize_బహుళకం rand_బహుళకం = {};
                        rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం = నిజం;
                        varp->user1(rand_బహుళకం.asUQuad);
                    });
                }
            }
            ఒకవేళ (!valid) {
                ఒకవేళ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_సమాసం)) {
                    nodep->replaceWith(కొత్త AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                } లేకపోతే {
                    m_stmtp->unlink_Fr_వెనుక();
                }
            }
            తిరిగి;
        }

        ఒకవేళ (nodep->name() == "constraint_mode") {
            తార్కిక valid = నిజం;
            ఒకవేళ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_సమాసం)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = అబద్ధం;
            } లేకపోతే ఒకవేళ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_సమాసం)
                       && !nodep->backp()->fileline()->warnIsOff(V3_Error_సంకేతం::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = అబద్ధం;
            }
            AstConstraint* constrp = శూన్యసూచిక;
            AstClass* classp = m_classp;
            ఒకవేళ (స్థిరరాశి Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(nodep, పద్ధతి_Call)) {
                ఒకవేళ (స్థిరరాశి AstConstraintRef* స్థిరరాశి constrRefp
                    = VN_CAST(పద్ధతి_Callp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    ఒకవేళ (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } లేకపోతే ఒకవేళ (AstClassRefDType* classRefDtp
                           = VN_CAST(పద్ధతి_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } లేకపోతే {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = అబద్ధం;
                }
            }
            ఒకవేళ (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = అబద్ధం;
            }
            ఒకవేళ (valid) {
                Randomize_బహుళకం constraint_బహుళకం = {};
                constraint_బహుళకం.ఉపయోగిస్తుంది_బహుళకం = నిజం;
                ఒకవేళ (constrp) {
                    constrp->user1(constraint_బహుళకం.asUQuad);
                } లేకపోతే {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraint_బహుళకం.asUQuad);
                    });
                }
            } లేకపోతే {
                ఒకవేళ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_సమాసం)) {
                    nodep->replaceWith(కొత్త AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                } లేకపోతే {
                    m_stmtp->unlink_Fr_వెనుక();
                }
            }
            తిరిగి;
        }

        ఒకవేళ (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            ఒకవేళ (m_constraint_సమాసం_Genp || m_లోపల_Std_With) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                కొరకు (Ast_కణుపు* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    ఒకవేళ (argp->user1()) {
                        nodep->user1(నిజం);
                        ఆపు;
                    }
                }
            }
            తిరిగి;
        }
        AstClass* classp = m_classp;
        ఒకవేళ (స్థిరరాశి Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(nodep, పద్ధతి_Call)) {
            ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp
                = VN_CAST(పద్ధతి_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        ఒకవేళ (classp) {
            ఒకవేళ (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* స్థిరరాశి, AstVar* స్థిరరాశి memberVarp) {
                ఒకవేళ (!memberVarp->rand().isRandomizable()) తిరిగి;
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = memberVarp->dtypep()->skipRefp();
                స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(dtypep, ClassRefDType);
                ఒకవేళ (!classRefp || !classRefp->classp()) తిరిగి;
                AstClass* స్థిరరాశి memberClassp = classRefp->classp();
                ఒకవేళ (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) తిరిగి;
                memberVarp->globalConstrained(నిజం);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* మూలం_Var_Refp
                    = కొత్త AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::సదిశ<AstVar*> ఖాళీలు_మార్గం;
                memberClassp->foreachMember([&](AstClass* స్థిరరాశి, AstConstraint* స్థిరరాశి constrp) {
                    processNestedConstraint(constrp, మూలం_Var_Refp, ఖాళీలు_మార్గం, classp);
                });
                cloneNestedConstraintsRecurse(మూలం_Var_Refp, memberClassp, ఖాళీలు_మార్గం, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(మూలం_Var_Refp->delete_వృక్షం(), మూలం_Var_Refp);
            });
        }
        ఒకవేళ (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = శూన్యసూచిక;
            కొరకు (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handle_Randomize_వాదం(argp->exprp(), శూన్యసూచిక, నిజం);
            }
            తిరిగి;
        }
        కొరకు (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->అంకె().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                స్థిరరాశి తార్కిక hasUnsupportedMember
                    = classp->existsMember([](స్థిరరాశి AstClass*, స్థిరరాశి AstVar* memberVarp) {
                          ఒకవేళ (!memberVarp->rand().isRandomizable()) తిరిగి అబద్ధం;
                          స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtp = memberVarp->dtypep()->skipRefp();
                          తిరిగి VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, వరుస_DType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                ఒకవేళ (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                కొనసాగు;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = శూన్యసూచిక;  // If nodep is a method call, this is its receiver
            ఒకవేళ (Ast_పద్ధతి_Call* పద్ధతి_Callp = VN_CAST(nodep, పద్ధతి_Call)) {
                ఒకవేళ (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(పద్ధతి_Callp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } లేకపోతే {
                    AstVarRef* స్థిరరాశి varrefp = VN_AS(పద్ధతి_Callp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handle_Randomize_వాదం(argp->exprp(), fromVarp, అబద్ధం);
        }
        // Re-mark members after the class became inline-randomized above.
        ఒకవేళ (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    శూన్య సందర్శించు(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_సమాసం_Genp);
        m_stmtp = nodep;
        m_constraint_సమాసం_Genp = nodep;
        iterateChildren(nodep);
        ఒకవేళ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_Constraint_సమాసం* nodep) override {
        VL_RESTORER(m_constraint_సమాసం_Genp);
        m_constraint_సమాసం_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_సమాసం_Genp);
            m_constraint_సమాసం_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterate_And_తదుపరి_Const_Null(nodep->thensp());
        iterate_And_తదుపరి_Const_Null(nodep->elsesp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (!m_constraint_సమాసం_Genp) తిరిగి;

        ఒకవేళ (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        ఒకవేళ (nodep->varp()->rand().isRandomizable() && !(m_లోపల_Std_With && m_stdRandCallp)) {
            nodep->user1(నిజం);
        } లేకపోతే ఒకవేళ (is_Var_లోపల_Std_Randomize_వాదనలు(nodep->varp())) {
            nodep->user1(నిజం);
        }
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        ఒకవేళ (!m_constraint_సమాసం_Genp) తిరిగి;
        iterateChildrenConst(nodep);
        స్థిరరాశి తార్కిక randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        స్థిరరాశి తార్కిక randMember = nodep->varp()->rand().isRandomizable();
        స్థిరరాశి తార్కిక లోపల_Std_With = m_లోపల_Std_With && m_stdRandCallp;
        ఒకవేళ (randObject && randMember && !లోపల_Std_With) {
            nodep->user1(నిజం);
        } లేకపోతే ఒకవేళ (లోపల_Std_With && is_Var_లోపల_Std_Randomize_వాదనలు(nodep->varp())) {
            nodep->user1(నిజం);
            // Mark parent object for constraint expression visitor
            ఒకవేళ (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(నిజం);
        }

        ఒకవేళ (m_withp) {
            Ast_కణుపు* backp = m_withp;
            అయితే (backp->backp()) {
                ఒకవేళ (స్థిరరాశి Ast_పద్ధతి_Call* స్థిరరాశి callp = VN_CAST(backp, పద్ధతి_Call)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    ఆపు;
                }
                backp = backp->backp();
            }
        } లేకపోతే {
            nodep->user2p(m_modp);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        nodep->user2p(m_modp);
        ఒకవేళ (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        ఒకవేళ (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_లోపల_Std_With = నిజం;
        iterateChildrenConst(nodep);
        m_లోపల_Std_With = అబద్ధం;
    }

    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        iterateChildrenConst(nodep);
        ఒకవేళ (!m_constraint_సమాసం_Genp && !m_లోపల_Std_With) తిరిగి;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    శూన్య సందర్శించు(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        ఒకవేళ (!m_constraint_సమాసం_Genp && !m_లోపల_Std_With) తిరిగి;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన RandomizeMarkVisitor(Ast_కణుపు* nodep) {
        iterateConst(nodep);
        markAllDerived();
        సమితి_Package_Refs();
    }
    ~RandomizeMarkVisitor() override = అప్రమేయ;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

తరగతి Constraint_సమాసం_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* స్థిరరాశి m_classp;
    Ast_కణుపు_FTask* స్థిరరాశి m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_కణుపు_FTask* స్థిరరాశి m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* స్థిరరాశి m_genp;  // VlRandomizer variable of the class
    AstVar* m_rand_బహుళకం_Varp;  // Relevant randmode state variable
    తార్కిక m_wantSingle = అబద్ధం;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    తార్కిక m_structSel = అబద్ధం;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    తార్కిక m_defer_Rand_బహుళకం_Hoist = అబద్ధం;  // Wait for the complete struct member path
    std::సమితి<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::సమితి<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertion_సమితి<AstVar*>* m_కొలత_Constrained_Arraysp = శూన్యసూచిక;  // Arrays with size constraints
    Ast_కణుపు_సమాసం* m_conditionp = శూన్యసూచిక;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_కణుపు_FTask* m_prepareConstrainedArraysp = శూన్యసూచిక;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = శూన్యసూచిక;  // Current ID of unique call
    V3_Unique_పేర్లు& m_unique_పేర్లు;  // Unique names of temporaries, and of blocks holding them
    std::సదిశ<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_కణుపు* m_మొదటి_వ్యక్తీకరణ_Inside_Indexp = శూన్యసూచిక;
    AstConstraintForeach* m_foreachp = శూన్యసూచిక;  // Innermost constraint-foreach being processed
    std::సమితి<స్థిరరాశి AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    తరగతి Nested_Access_మార్గం final {
        AstMemberSel* m_శిఖరం_Nested_Array_Member_Selp
            = శూన్యసూచిక;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = శూన్యసూచిక;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = శూన్యసూచిక;  // Last node of variable's name format
        Ast_కణుపు** m_మొదటి_వ్యక్తీకరణ_Inside_సూచీ_Pointerp
            = శూన్యసూచిక;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    బహిరంగ:
        శూన్య కూడు_Var_Name_Part(AstSFormatF* స్థిరరాశి nodep, std::string&& name) {
            ఒకవేళ (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } లేకపోతే {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.చొప్పించు(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            ఒకవేళ (name == "") {
                ఒకవేళ (*m_మొదటి_వ్యక్తీకరణ_Inside_సూచీ_Pointerp) {
                    (*m_మొదటి_వ్యక్తీకరణ_Inside_సూచీ_Pointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_మొదటి_వ్యక్తీకరణ_Inside_సూచీ_Pointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_మొదటి_వ్యక్తీకరణ_Inside_సూచీ_Pointerp = nodep;
            }
        }

        AstMemberSel* access_వృక్షం() { తిరిగి m_శిఖరం_Nested_Array_Member_Selp; }
        స్థిరరాశి std::string& smtName() { తిరిగి m_smtName; }

        Ast_Stmt_సమాసం* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* స్థిరరాశి methodp = కొత్త AstCMethodHard{
                varp->fileline(),
                కొత్త AstVarRef{varp->fileline(), VN_AS(genp->user2p(), కణుపు_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtype_సమితి_Void();

            methodp->కూడు_Pinsp(m_శిఖరం_Nested_Array_Member_Selp);  // Variable

            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep();  //వెడల్పు
            స్థిరరాశి కొలత_t వెడల్పు = m_శిఖరం_Nested_Array_Member_Selp->varp()->dtypep()->వెడల్పు();
            methodp->కూడు_Pinsp(కొత్త AstConst{dtypep->fileline(), AstConst::Unsized64{}, వెడల్పు});

            methodp->కూడు_Pinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = శూన్యసూచిక;

            methodp->కూడు_Pinsp(
                కొత్త AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_శిఖరం_Nested_Array_Member_Selp = శూన్యసూచిక;
            తిరిగి methodp->makeStmt();
        }

        Ast_కణుపు_సమాసం* cloneName() { తిరిగి m_nestedNameFormatTopp->clone_వృక్షం(అబద్ధం); }

        శూన్య error() {
            VL_DO_DANGLING(m_శిఖరం_Nested_Array_Member_Selp->delete_వృక్షం(), m_శిఖరం_Nested_Array_Member_Selp);
        }

        Nested_Access_మార్గం(AstMemberSel* శిఖరం_Member_Selp, Ast_కణుపు** సూచిక_గమ్యం_మొదటి_Exprp)
            : m_శిఖరం_Nested_Array_Member_Selp{శిఖరం_Member_Selp}
            , m_మొదటి_వ్యక్తీకరణ_Inside_సూచీ_Pointerp{సూచిక_గమ్యం_మొదటి_Exprp} {}
        ~Nested_Access_మార్గం() {
            ఒకవేళ (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_వృక్షం(),
                            m_nestedNameFormatTopp = శూన్యసూచిక);
                m_nestedNameFormatp = శూన్యసూచిక;
            }
        }
    };
    Nested_Access_మార్గం* m_nestedAccess = శూన్యసూచిక;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_కణుపు_సమాసం* createEnumConstraint(File_పంక్తి* స్థిరరాశి fl, స్థిరరాశి std::string& smtName,
                                      AstEnumDType* స్థిరరాశి enumDtp, AstVar* స్థిరరాశి genVarp) {
        Ast_కణుపు_Module* స్థిరరాశి genModp = VN_AS(genVarp->user2p(), కణుపు_Module);
        స్థిరరాశి పూర్ణాంకం వెడల్పు = enumDtp->వెడల్పు();
        std::string constraint = "(__Vbv (or";
        కొరకు (Ast_Enum_అంశం* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
            స్థిరరాశి AstConst* స్థిరరాశి vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvt_గమ్యం_Str(vconstp->గమ్యం_UInt()) + " "
                          + cvt_గమ్యం_Str(వెడల్పు) + "))";
        }
        constraint += "))";
        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
            fl, కొత్త AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            కొత్త AstCExpr{fl, AstCExpr::శుద్ధ{}, "\"" + constraint + "\""}};
        callp->dtype_సమితి_Void();
        తిరిగి callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* కనుగొను_Static_Rand_బహుళకం_Var_Member(AstClass* classp) స్థిరరాశి {
        అయితే (నిజం) {
            ఒకవేళ (AstVar* స్థిరరాశి varp
                = VN_CAST(m_memberMap.కనుగొను_Member(classp, "__Vstaticrandmode"), Var)) {
                తిరిగి varp;
            }
            AstClassExtends* స్థిరరాశి extendsp = classp->extendsp();
            ఒకవేళ (!extendsp) తిరిగి శూన్యసూచిక;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    శూన్య markConstrainedRandMember(AstStructDType* స్థిరరాశి structp, స్థిరరాశి std::string& name) {
        AstMemberDType* స్థిరరాశి memberp
            = VN_CAST(m_memberMap.కనుగొను_Member(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(నిజం);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    తార్కిక is_Rand_Qualified_మార్గం(స్థిరరాశి Ast_కణుపు_సమాసం* nodep) {
        అయితే (nodep) {
            ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి selp = VN_CAST(nodep, StructSel)) {
                ఒకవేళ (AstStructDType* స్థిరరాశి structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    స్థిరరాశి AstMemberDType* స్థిరరాశి memberp
                        = VN_CAST(m_memberMap.కనుగొను_Member(structp, selp->name()), MemberDType);
                    ఒకవేళ (memberp && !memberp->rand().isRandomizable()) తిరిగి అబద్ధం;
                }
                nodep = selp->fromp();
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstCMethodHard* స్థిరరాశి methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } లేకపోతే {
                ఆపు;
            }
        }
        తిరిగి నిజం;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    స్థిర శూన్య markStructConstrainedRandRecurse(Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        AstStructDType* స్థిరరాశి structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        ఒకవేళ (!structp || structp->packed()) తిరిగి;
        structp->markConstrainedRand(నిజం);
        కొరకు (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            ఒకవేళ (!memberp->rand().isRandomizable()) కొనసాగు;
            memberp->markConstrainedRand(నిజం);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string build_Member_మార్గం(స్థిరరాశి AstMemberSel* స్థిరరాశి memberSelp) {
        స్థిరరాశి Ast_కణుపు* fromp = memberSelp->fromp();
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            తిరిగి refp->name() + "." + memberSelp->name();
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            తిరిగి build_Member_మార్గం(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        తిరిగి "";
    }

    స్థిర Ast_కణుపు_సమాసం* getFromp(స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి nodep) {
        ఒకవేళ (స్థిరరాశి AstCMethodHard* స్థిరరాశి cmethp = VN_CAST(nodep, CMethodHard)) {
            తిరిగి cmethp->fromp();
        }
        తిరిగి getSelFromp(nodep);
    }

    స్థిర std::string వెలికితీయి_Solve_Before_పునాది_Name(స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        అయితే (స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి fromp = getFromp(exprp)) exprp = fromp;
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి vrefp = VN_CAST(exprp, VarRef)) తిరిగి vrefp->name();
        తిరిగి "";
    }

    స్థిర శూన్య build_Name_ఉపసర్గ(AstCExpr* exprp, స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి nodep) {
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
            exprp->కూడు("(\""s + refp->name());
            తిరిగి;
        }

        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి fromp = getFromp(nodep);
        ఒకవేళ (fromp) build_Name_ఉపసర్గ(exprp, fromp);

        ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి selp = VN_CAST(nodep, StructSel)) {
            exprp->కూడు("." + selp->name());
        } లేకపోతే ఒకవేళ (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            తిరిగి;
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstCMethodHard* స్థిరరాశి cmethp = VN_CAST(nodep, CMethodHard)) {
            ఒకవేళ (cmethp->పద్ధతి() == VCMethod::ARRAY_AT) {
                Ast_కణుపు_సమాసం* స్థిరరాశి argp = cmethp->pinsp();
                exprp->కూడు(".\" + ");
                AstCFuncHard* స్థిరరాశి solverHexFuncp
                    = కొత్త AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_గమ్యం_SOLVER_HEX};
                solverHexFuncp->కూడు_Pinsp(argp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                solverHexFuncp->dtype_సమితి_String();
                exprp->కూడు(solverHexFuncp);
                exprp->కూడు(" + \"");
            } లేకపోతే {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                తిరిగి;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    స్థిర AstCExpr* build_మార్గం_సమాసం(File_పంక్తి* స్థిరరాశి fl, స్థిరరాశి Ast_కణుపు_సమాసం* nodep,
                                   స్థిరరాశి std::string selName) {
        AstCExpr* స్థిరరాశి exprp = కొత్త AstCExpr{fl, ""};
        build_Name_ఉపసర్గ(exprp, nodep);
        ఒకవేళ (selName != "") {
            exprp->కూడు(".");
            exprp->కూడు(selName);
        }
        exprp->కూడు("\")");
        తిరిగి exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_సమాసం(File_పంక్తి* fl, స్థిరరాశి std::string& పునాది_Name,
                                    స్థిరరాశి Ast_కణుపు_Sel* selp) {
        AstCExpr* స్థిరరాశి p = కొత్త AstCExpr{fl, ""};
        p->కూడు("(\""s + పునాది_Name + ".\"");
        p->కూడు("+");
        AstCFuncHard* స్థిరరాశి solverHexFuncp
            = కొత్త AstCFuncHard{fl, VCFunction::RANDOMIZER_గమ్యం_SOLVER_HEX};
        solverHexFuncp->కూడు_Pinsp(selp->bitp()->clone_వృక్షం_శుద్ధ(అబద్ధం));
        solverHexFuncp->dtype_సమితి_String();
        p->కూడు(solverHexFuncp);
        p->కూడు(")");
        p->dtype_సమితి_String();
        తిరిగి p;
    }

    // Helper: get fromp from MemberSel or StructSel
    స్థిర Ast_కణుపు_సమాసం* getSelFromp(స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి mp = VN_CAST(exprp, MemberSel)) తిరిగి mp->fromp();
        ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి sp = VN_CAST(exprp, StructSel)) తిరిగి sp->fromp();
        తిరిగి శూన్యసూచిక;
    }

    Ast_కణుపు_సమాసం* build_Solve_Before_Name_సమాసం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp) {
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* స్థిరరాశి p = కొత్త AstCExpr{fl, AstCExpr::శుద్ధ{}, "\"" + varrefp->name() + "\"s"};
            p->dtype_సమితి_String();
            తిరిగి p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి selFromp = getSelFromp(exprp)) {
            స్థిరరాశి std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string పునాది_Name;
                ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    పునాది_Name = vp->name();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    పునాది_Name = build_Member_మార్గం(mp);
                }
                ఒకవేళ (పునాది_Name.ఖాళీలు()) తిరిగి శూన్యసూచిక;
                AstCExpr* స్థిరరాశి p = కొత్త AstCExpr{fl, ""};
                p->కూడు("(\""s + పునాది_Name + ".\"");
                p->కూడు("+");
                AstCFuncHard* స్థిరరాశి solverHexFuncp
                    = కొత్త AstCFuncHard{fl, VCFunction::RANDOMIZER_గమ్యం_SOLVER_HEX};
                solverHexFuncp->కూడు_Pinsp(arrSelp->bitp()->clone_వృక్షం_శుద్ధ(అబద్ధం));
                solverHexFuncp->dtype_సమితి_String();
                p->కూడు(solverHexFuncp);
                p->కూడు(")");
                p->dtype_సమితి_String();
                తిరిగి p;
            }
            // Static member path (obj.field)
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి vp = VN_CAST(selFromp, VarRef)) {
                స్థిరరాశి std::string మార్గం = vp->name() + "." + selName;
                AstCExpr* స్థిరరాశి p = కొత్త AstCExpr{fl, AstCExpr::శుద్ధ{}, "\"" + మార్గం + "\"s"};
                p->dtype_సమితి_String();
                తిరిగి p;
            }
            ఒకవేళ (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* స్థిరరాశి p = build_మార్గం_సమాసం(fl, selFromp, selName);
                p->dtype_సమితి_String();
                తిరిగి p;
            }
            తిరిగి శూన్యసూచిక;
        }
        ఒకవేళ (VN_IS(exprp, CMethodHard)) {
            AstCExpr* స్థిరరాశి p = build_మార్గం_సమాసం(fl, exprp, "");
            p->dtype_సమితి_String();
            తిరిగి p;
        }
        స్థిరరాశి Ast_కణుపు_Sel* selp = VN_CAST(exprp, ArraySel);
        ఒకవేళ (!selp) selp = VN_CAST(exprp, AssocSel);
        ఒకవేళ (selp) {
            // arr[i] -> dynamic name
            std::string పునాది_Name;
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి vp = VN_CAST(selp->fromp(), VarRef)) {
                పునాది_Name = vp->name();
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి mp = VN_CAST(selp->fromp(), MemberSel)) {
                పునాది_Name = build_Member_మార్గం(mp);
            }
            ఒకవేళ (పునాది_Name.ఖాళీలు()) తిరిగి శూన్యసూచిక;
            తిరిగి build_Array_Sel_Name_సమాసం(fl, పునాది_Name, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి బిట్టు_Selp = VN_CAST(exprp, Sel)) {
            తిరిగి build_Solve_Before_Name_సమాసం(fl, బిట్టు_Selp->fromp());
        }
        తిరిగి శూన్యసూచిక;
    }

    AstSFormatF* getConstFormat(Ast_కణుపు_సమాసం* nodep) {
        తిరిగి కొత్త AstSFormatF{nodep->fileline(), (nodep->వెడల్పు() & 3) ? "#b%b" : "#x%x", అబద్ధం,
                               nodep};
    }
    తార్కిక editFormat(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (nodep->user1()) తిరిగి అబద్ధం;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlink_Fr_వెనుక(&handle);
        handle.relink(getConstFormat(nodep));
        తిరిగి నిజం;
    }
    Ast_కణుపు_సమాసం* wrapWithCond(Ast_కణుపు_సమాసం* స్థిరరాశి exprp, Ast_కణుపు_సమాసం* స్థిరరాశి condp,
                              స్థిరరాశి పూర్ణాంకం వెడల్పు) {
        ఒకవేళ (condp) {
            File_పంక్తి* స్థిరరాశి flp = exprp->fileline();
            తిరిగి కొత్త AstCond{
                flp, condp, exprp,
                getConstFormat(కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, వెడల్పు, 0})};
        }
        తిరిగి exprp;
    }
    AstSFormatF* editSMT(Ast_కణుపు_సమాసం* nodep, Ast_కణుపు_సమాసం* lhsp = శూన్యసూచిక,
                         Ast_కణుపు_సమాసం* rhsp = శూన్యసూచిక, Ast_కణుపు_సమాసం* thsp = శూన్యసూచిక) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_సమాసం = nodep->emitSMT();  // Might need child width (AstExtend)
        ఒకవేళ (smt_సమాసం == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            తిరిగి శూన్యసూచిక;
        }

        స్థిరరాశి పూర్ణాంకం lhs_వెడల్పు = lhsp ? lhsp->వెడల్పు() : 0;
        స్థిరరాశి పూర్ణాంకం rhs_వెడల్పు = rhsp ? rhsp->వెడల్పు() : 0;
        స్థిరరాశి పూర్ణాంకం ths_వెడల్పు = thsp ? thsp->వెడల్పు() : 0;
        Ast_కణుపు_సమాసం* lhsCondp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* rhsCondp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* thsCondp = శూన్యసూచిక;

        ఒకవేళ (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = శూన్యసూచిక;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlink_Fr_వెనుక() : lhsp),
                         కణుపు_సమాసం);
            lhsCondp = m_conditionp;
        }
        ఒకవేళ (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = శూన్యసూచిక;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlink_Fr_వెనుక() : rhsp),
                         కణుపు_సమాసం);
            rhsCondp = m_conditionp;
        }
        ఒకవేళ (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = శూన్యసూచిక;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlink_Fr_వెనుక() : thsp),
                         కణుపు_సమాసం);
            thsCondp = m_conditionp;
        }

        Ast_కణుపు_సమాసం* argsp = శూన్యసూచిక;
        కొరకు (string::iterator pos = smt_సమాసం.begin(); pos != smt_సమాసం.చివర(); ++pos) {
            ఒకవేళ (pos[0] == '%') {
                ++pos;
                ఎంపిక (pos[0]) {
                స్థితి '%': ఆపు;
                స్థితి 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_కణుపు::కూడు_తదుపరి(argsp, wrapWithCond(lhsp, lhsCondp, lhs_వెడల్పు));
                    lhsp = శూన్యసూచిక;
                    ఆపు;
                స్థితి 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_కణుపు::కూడు_తదుపరి(argsp, wrapWithCond(rhsp, rhsCondp, rhs_వెడల్పు));
                    rhsp = శూన్యసూచిక;
                    ఆపు;
                స్థితి 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_కణుపు::కూడు_తదుపరి(argsp, wrapWithCond(thsp, thsCondp, ths_వెడల్పు));
                    thsp = శూన్యసూచిక;
                    ఆపు;
                అప్రమేయ: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); ఆపు;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* స్థిరరాశి newp = కొత్త AstSFormatF{nodep->fileline(), smt_సమాసం, అబద్ధం, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి newp;
    }

    Ast_కణుపు_సమాసం* editSingle(File_పంక్తి* fl, Ast_కణుపు* itemsp) {
        ఒకవేళ (!itemsp) తిరిగి శూన్యసూచిక;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = నిజం;

        {
            AstBegin* స్థిరరాశి tempp
                = కొత్త AstBegin{fl, "[EditWrapper]", itemsp->unlink_Fr_వెనుక_With_తదుపరి(), అబద్ధం};
            VL_DO_DANGLING(iterate_And_తదుపరి_Null(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            ఒకవేళ (itemsp) itemsp->unlink_Fr_వెనుక_With_తదుపరి();
            VL_DO_DANGLING(tempp->delete_వృక్షం(), tempp);
        }
        ఒకవేళ (!itemsp) తిరిగి శూన్యసూచిక;

        Ast_కణుపు_సమాసం* స్థిరరాశి exprsp = VN_CAST(itemsp, కణుపు_సమాసం);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        ఒకవేళ (!exprsp->nextp()) తిరిగి exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        కొరకు (Ast_కణుపు* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        తిరిగి కొత్త AstSFormatF{fl, fmt.str(), అబద్ధం, exprsp};
    }

    Ast_కణుపు_సమాసం* newSel(File_పంక్తి* fl, Ast_కణుపు_సమాసం* arrayp, Ast_కణుపు_సమాసం* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_కణుపు_DType* స్థిరరాశి arrDtp = arrayp->unlink_Fr_వెనుక()->dtypep()->skipRefp();
        Ast_కణుపు_సమాసం* selp = శూన్యసూచిక;
        ఒకవేళ (VN_IS(arrDtp, వరుస_DType) || VN_IS(arrDtp, DynArrayDType))
            selp = కొత్త AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        లేకపోతే ఒకవేళ (VN_IS(arrDtp, UnpackArrayDType))
            selp = కొత్త AstArraySel{fl, arrayp, idxp};
        లేకపోతే ఒకవేళ (VN_IS(arrDtp, AssocArrayDType))
            selp = కొత్త AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        తిరిగి selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_కణుపు_సమాసం* adjust_వెడల్పు(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, పూర్ణాంకం లక్ష్యం_వెడల్పు,
                             VSigning లక్ష్యం_Signing) {
        స్థిరరాశి పూర్ణాంకం సమాసం_వెడల్పు = exprp->వెడల్పు();
        ఒకవేళ (లక్ష్యం_వెడల్పు > సమాసం_వెడల్పు) {
            // Extend to match target width
            Ast_కణుపు_సమాసం* స్థిరరాశి ఫలితం = కొత్త AstExtend{fl, exprp, లక్ష్యం_వెడల్పు};
            ఫలితం->dtypeChgSigned(లక్ష్యం_Signing);
            తిరిగి ఫలితం;
        } లేకపోతే ఒకవేళ (లక్ష్యం_వెడల్పు < సమాసం_వెడల్పు) {
            // Truncate to match target width
            Ast_కణుపు_సమాసం* స్థిరరాశి ఫలితం = కొత్త AstSel{fl, exprp, 0, లక్ష్యం_వెడల్పు};
            ఫలితం->dtypeChgSigned(లక్ష్యం_Signing);
            తిరిగి ఫలితం;
        } లేకపోతే {
            // Width already matches
            తిరిగి exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_కణుపు_సమాసం* వెలికితీయి_Return_సమాసం(AstFunc* funcp) {
        AstVar* స్థిరరాశి retVarp = VN_CAST(funcp->fvarp(), Var);
        ఒకవేళ (!retVarp) తిరిగి శూన్యసూచిక;
        Ast_కణుపు_సమాసం* retExprp = శూన్యసూచిక;
        పూర్ణాంకం కేటాయించు_లెక్క = 0;
        funcp->foreach([&](Ast_కేటాయించు* assignp) {
            AstVarRef* స్థిరరాశి lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ఒకవేళ (!lhsp || lhsp->varp() != retVarp) తిరిగి;
            retExprp = assignp->rhsp();
            ++కేటాయించు_లెక్క;
        });
        // Simple function: initializer + return (2) or just return (1)
        ఒకవేళ (కేటాయించు_లెక్క > 2 || !retExprp) తిరిగి శూన్యసూచిక;
        తిరిగి retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    శూన్య propagateUser1InlineRecurse(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (VN_IS(nodep, కణుపు_Var_Ref)) {
            nodep->user1(VN_AS(nodep, కణుపు_Var_Ref)->varp()->rand().isRandomizable());
            తిరిగి;
        }
        ఒకవేళ (VN_IS(nodep, Const)) {
            nodep->user1(అబద్ధం);
            తిరిగి;
        }
        పూర్ణాంకం any_బిడ్డ = అబద్ధం;  // Used as bool
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి cp = VN_CAST(nodep->op1p(), కణుపు_సమాసం)) {
            propagateUser1InlineRecurse(cp);
            any_బిడ్డ |= cp->user1();
        }
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి cp = VN_CAST(nodep->op2p(), కణుపు_సమాసం)) {
            propagateUser1InlineRecurse(cp);
            any_బిడ్డ |= cp->user1();
        }
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి cp = VN_CAST(nodep->op3p(), కణుపు_సమాసం)) {
            propagateUser1InlineRecurse(cp);
            any_బిడ్డ |= cp->user1();
        }
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి cp = VN_CAST(nodep->op4p(), కణుపు_సమాసం)) {
            propagateUser1InlineRecurse(cp);
            any_బిడ్డ |= cp->user1();
        }
        nodep->user1(any_బిడ్డ);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    స్థిర Ast_కణుపు* prependDistPreamble(AstConstraintForeach* nodep, Ast_కణుపు* bodyp) {
        Ast_కణుపు* స్థిరరాశి preamblep = nodep->user3p();
        ఒకవేళ (!preamblep) తిరిగి bodyp;
        nodep->user3p(శూన్యసూచిక);
        preamblep->కూడు_తదుపరి(bodyp);
        తిరిగి preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    తార్కిక referencesForeachIdx(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి {
        ఒకవేళ (m_foreachIdxVars.ఖాళీలు()) తిరిగి అబద్ధం;
        తిరిగి nodep->exists(
            [&](స్థిరరాశి AstVarRef* refp) { తిరిగి m_foreachIdxVars.లెక్క(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    శూన్య కూడు_Registration_Stmt(Ast_కణుపు* స్థిరరాశి stmtp, Ast_కణుపు_FTask* స్థిరరాశి initTaskp) {
        ఒకవేళ (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->కూడు_Bodyp(stmtp);
        } లేకపోతే {
            initTaskp->కూడు_Stmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_పంక్తి* స్థిరరాశి fl, Ast_కణుపు_సమాసం* స్థిరరాశి arrExprp,
                                       Ast_కణుపు_సమాసం* స్థిరరాశి idxExprp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి argsp = Ast_కణుపు::కూడు_తదుపరి(arrExprp, idxExprp);
        AstSFormatF* స్థిరరాశి formatp = కొత్త AstSFormatF{fl, "(select %s %s)", అబద్ధం, argsp};
        తిరిగి formatp;
    }

    శూన్య సమితి_Rand_బహుళకం(AstVar* స్థిరరాశి varp, AstMemberSel* స్థిరరాశి memberselp,
                     స్థిరరాశి std::string& smtName, స్థిరరాశి Randomize_బహుళకం& rand_బహుళకం,
                     Ast_కణుపు_FTask* స్థిరరాశి initTaskp) {
        Ast_కణుపు_Module* స్థిరరాశి varClassp = VN_AS(varp->user2p(), కణుపు_Module);
        AstVar* స్థిరరాశి sub_Rand_బహుళకం_Varp = get_Rand_బహుళకం_Var_From_Class(varClassp);
        ఒకవేళ (sub_Rand_బహుళకం_Varp) {
            Ast_కణుపు_సమాసం* స్థిరరాశి మాతృ_Accessp = memberselp->fromp()->clone_వృక్షం(అబద్ధం);
            AstMemberSel* స్థిరరాశి rand_బహుళకం_Selp
                = కొత్త AstMemberSel{varp->fileline(), మాతృ_Accessp, sub_Rand_బహుళకం_Varp};
            rand_బహుళకం_Selp->dtypep(sub_Rand_బహుళకం_Varp->dtypep());
            AstCMethodHard* స్థిరరాశి atp
                = కొత్త AstCMethodHard{varp->fileline(), rand_బహుళకం_Selp, VCMethod::ARRAY_AT,
                                     కొత్త AstConst{varp->fileline(), rand_బహుళకం.సూచీ}};
            atp->dtype_సమితి_UInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* స్థిరరాశి enablep = కొత్త AstCMethodHard{
                varp->fileline(),
                కొత్త AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), కణుపు_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtype_సమితి_Void();
            Ast_కణుపు_సమాసం* స్థిరరాశి nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : కొత్త AstSFormatF{varp->fileline(), smtName, అబద్ధం, శూన్యసూచిక};
            nameExprp->dtypep(varp->dtypep());
            enablep->కూడు_Pinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* స్థిరరాశి disablep = కొత్త AstCMethodHard{
                varp->fileline(),
                కొత్త AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), కణుపు_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_సమితి_VAR_DISABLED};
            disablep->dtype_సమితి_Void();
            Ast_కణుపు_సమాసం* స్థిరరాశి disnp = nameExprp->clone_వృక్షం(అబద్ధం);
            disablep->కూడు_Pinsp(disnp);
            AstIf* స్థిరరాశి ifp
                = కొత్త AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            కూడు_Registration_Stmt(ifp, initTaskp);
        }
    }

    శూన్య markRandc(AstVar* స్థిరరాశి varp, స్థిరరాశి std::string& smtName, Ast_కణుపు_FTask* స్థిరరాశి initTaskp) {
        AstCMethodHard* స్థిరరాశి markp = కొత్త AstCMethodHard{
            varp->fileline(),
            కొత్త AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), కణుపు_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtype_సమితి_Void();
        Ast_కణుపు_సమాసం* స్థిరరాశి nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : కొత్త AstSFormatF{varp->fileline(), smtName, అబద్ధం, శూన్యసూచిక};
        nameExprp->dtypep(varp->dtypep());
        markp->కూడు_Pinsp(nameExprp);
        కూడు_Registration_Stmt(markp->makeStmt(), initTaskp);
    }

    Ast_కణుపు_Module* getLeftmostVarModulep(AstMemberSel* స్థిరరాశి memberselp,
                                         AstVar* స్థిరరాశి varp) స్థిరరాశి {
        ఒకవేళ (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_కణుపు* ఎడమ_Most_Nodep = memberselp->fromp();
            అయితే (AstMemberSel* స్థిరరాశి nestedMemberSelp = VN_CAST(ఎడమ_Most_Nodep, MemberSel)) {
                ఎడమ_Most_Nodep = nestedMemberSelp->fromp();
            }
            ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి మూలం_Var_Refp = VN_CAST(ఎడమ_Most_Nodep, కణుపు_Var_Ref)) {
                తిరిగి VN_AS(మూలం_Var_Refp->varp()->user2p(), కణుపు_Module);
            }
            తిరిగి VN_AS(memberselp->user2p(), కణుపు_Module);
        }
        తిరిగి VN_AS(varp->user2p(), కణుపు_Module);
    }

    Ast_కణుపు_FTask* getInitTaskp(AstVar* స్థిరరాశి varp, స్థిరరాశి తార్కిక memberselOrStructSel,
                               Ast_కణుపు_Module* స్థిరరాశి classp) స్థిరరాశి {
        ఒకవేళ (m_inlineInitTaskp) తిరిగి m_inlineInitTaskp;
        varp->user3(నిజం);
        ఒకవేళ (memberselOrStructSel) {
            ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి initTaskp
                = VN_AS(m_memberMap.కనుగొను_Member(classp, "randomize"), కణుపు_FTask)) {
                తిరిగి initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            ఒకవేళ (m_memberselInitTaskp) తిరిగి m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } లేకపోతే {
            Ast_కణుపు_FTask* స్థిరరాశి initTaskp
                = VN_AS(m_memberMap.కనుగొను_Member(classp, "new"), కణుపు_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            తిరిగి initTaskp;
        }
        తిరిగి శూన్యసూచిక;
    }

    శూన్య createWriteVarForClassFields(AstClass* స్థిరరాశి elemClassp, స్థిరరాశి std::string& smtName,
                                      AstVar* స్థిరరాశి iterVarp, స్థిరరాశి తార్కిక isUnpackedClassRefArray,
                                      Ast_కణుపు_Module* స్థిరరాశి classOrPackagep, AstVar* స్థిరరాశి varp,
                                      AstIf* స్థిరరాశి ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) స్థిరరాశి {
        File_పంక్తి* స్థిరరాశి fl = varp->fileline();
        Ast_కణుపు_Module* స్థిరరాశి varClassp = VN_AS(varp->user2p(), కణుపు_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr పూర్ణాంకం idx_వెడల్పు = 32;
        స్థిరరాశి పూర్ణాంకం fmt_వెడల్పు = VL_పదాలు_I(idx_వెడల్పు) * 8;

        కొరకు (స్థిరరాశి AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : శూన్యసూచిక) {
            కొరకు (Ast_కణుపు* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* స్థిరరాశి memberVarp = VN_CAST(mnodep, Var);
                ఒకవేళ (!memberVarp || !memberVarp->rand().isRandomizable()) కొనసాగు;
                Ast_కణుపు_DType* స్థిరరాశి memberDtp = memberVarp->dtypep()->skipRefp();
                ఒకవేళ (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) కొనసాగు;
                స్థిరరాశి పూర్ణాంకం member_వెడల్పు = memberDtp->వెడల్పు();

                AstCStmt* స్థిరరాశి fmtp = కొత్త AstCStmt{fl};
                ఒకవేళ (isUnpackedClassRefArray) {
                    fmtp->కూడు("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } లేకపోతే {
                    fmtp->కూడు("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::గమ్యం_string(fmt_వెడల్పు) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->కూడు(కొత్త AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->కూడు(");\n");
                ifNonNullp->కూడు_Thensp(fmtp);

                AstVarRef* స్థిరరాశి arrayWrRefp = కొత్త AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_కణుపు_సమాసం* atWritep;
                ఒకవేళ (isUnpackedClassRefArray) {
                    atWritep = కొత్త AstArraySel{fl, arrayWrRefp,
                                               కొత్త AstVarRef{fl, iterVarp, VAccess::READ}};
                } లేకపోతే {
                    atWritep = కొత్త AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  కొత్త AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* స్థిరరాశి memberSelp = కొత్త AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* స్థిరరాశి writeVarp
                    = కొత్త AstCMethodHard{fl,
                                         కొత్త AstVarRef{fl, VN_AS(m_genp->user2p(), కణుపు_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtype_సమితి_Void();
                writeVarp->కూడు_Pinsp(memberSelp);
                writeVarp->కూడు_Pinsp(
                    కొత్త AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(member_వెడల్పు)});
                AstCExpr* స్థిరరాశి nameRefp = కొత్త AstCExpr{fl, AstCExpr::శుద్ధ{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->కూడు_Pinsp(nameRefp);
                writeVarp->కూడు_Pinsp(కొత్త AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->కూడు_Thensp(writeVarp->makeStmt());
            }
        }
    }

    శూన్య createSolverArrayHandle(AstVar* స్థిరరాశి varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_కణుపు_Module* స్థిరరాశి classOrPackagep,
                                 స్థిరరాశి తార్కిక isUnpackedClassRefArray,
                                 స్థిరరాశి std::string& smtName) స్థిరరాశి {
        File_పంక్తి* స్థిరరాశి fl = varp->fileline();
        AstClass* స్థిరరాశి elemClassp = elemClassRefDtp->classp();
        Ast_కణుపు_Module* స్థిరరాశి varClassp = VN_AS(varp->user2p(), కణుపు_Module);

        AstVar* స్థిరరాశి iterVarp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, m_unique_పేర్లు.get("__Vi"),
                                            varp->కనుగొను_UInt32_DType()};
        iterVarp->funcLocal(నిజం);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_కణుపు* స్థిరరాశి stmtsp
            = కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, iterVarp, VAccess::WRITE}, కొత్త AstConst{fl, 0}};

        Ast_కణుపు_సమాసం* sizep;
        ఒకవేళ (isUnpackedClassRefArray) {
            స్థిరరాశి పూర్ణాంకం array_కొలత_2
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = కొత్త AstConst{fl, static_cast<uint32_t>(array_కొలత_2)};
        } లేకపోతే {
            AstVarRef* స్థిరరాశి array_కొలత_Refp = కొత్త AstVarRef{fl, varClassp, varp, VAccess::READ};
            array_కొలత_Refp->classOrPackagep(classOrPackagep);
            AstCMethodHard* స్థిరరాశి dynSizep
                = కొత్త AstCMethodHard{fl, array_కొలత_Refp, VCMethod::DYN_కొలత, శూన్యసూచిక};
            dynSizep->dtype_సమితి_UInt32();
            sizep = dynSizep;
        }

        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{fl};
        stmtsp->కూడు_తదుపరి(loopp);
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{
            fl, loopp, కొత్త AstLt{fl, కొత్త AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* స్థిరరాశి arrayReadRefp = కొత్త AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_కణుపు_సమాసం* atReadp;
        ఒకవేళ (isUnpackedClassRefArray) {
            atReadp
                = కొత్త AstArraySel{fl, arrayReadRefp, కొత్త AstVarRef{fl, iterVarp, VAccess::READ}};
        } లేకపోతే {
            atReadp = కొత్త AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         కొత్త AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* స్థిరరాశి ifNonNullp
            = కొత్త AstIf{fl, కొత్త AstNeq{fl, atReadp, కొత్త AstConst{fl, AstConst::Null{}}}, శూన్యసూచిక};
        loopp->కూడు_Stmtsp(ifNonNullp);

        AstCStmt* స్థిరరాశి bufDeclp = కొత్త AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->కూడు_Thensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
            fl, కొత్త AstVarRef{fl, iterVarp, VAccess::WRITE},
            కొత్త Ast_కూడు{fl, కొత్త AstConst{fl, 1}, కొత్త AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(నిజం);
        Ast_కణుపు_FTask* initTaskp = m_inlineInitTaskp;
        ఒకవేళ (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.కనుగొను_Member(varClassp, "new"), కణుపు_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->కూడు_Stmtsp(stmtsp);
    }

    శూన్య createSolverVarHandle(AstVar* స్థిరరాశి varp, స్థిరరాశి తార్కిక structSelOrCMeth,
                               స్థిరరాశి తార్కిక isGlobalConstrained, స్థిరరాశి Randomize_బహుళకం rand_బహుళకం,
                               AstMemberSel* స్థిరరాశి memberselp, స్థిరరాశి std::string& smtName,
                               Ast_కణుపు_Module* స్థిరరాశి classOrPackagep, Ast_కణుపు_Module* స్థిరరాశి classp,
                               Ast_కణుపు_FTask* స్థిరరాశి initTaskp) {
        uint32_t unpackedDims = 0;
        ఒకవేళ (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(అబద్ధం).రెండవ;
        }
        ఒకవేళ (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* స్థిరరాశి methodp = కొత్త AstCMethodHard{
            varp->fileline(),
            కొత్త AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), కణుపు_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_సమితి_Void();
        ఒకవేళ (memberselp) {
            methodp->కూడు_Pinsp(memberselp);
        } లేకపోతే {
            AstVarRef* స్థిరరాశి varRefp
                = కొత్త AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->కూడు_Pinsp(varRefp);
        }
        స్థిరరాశి కొలత_t వెడల్పు = arrayElementDTypep(varp->dtypep())->వెడల్పు();
        methodp->కూడు_Pinsp(కొత్త AstConst{varp->fileline(), AstConst::Unsized64{}, వెడల్పు});
        Ast_కణుపు_సమాసం* స్థిరరాశి varnamep = కొత్త AstCExpr{varp->fileline(), AstCExpr::శుద్ధ{},
                                                   "\"" + smtName + "\"", varp->వెడల్పు()};
        varnamep->dtypep(varp->dtypep());
        methodp->కూడు_Pinsp(varnamep);
        methodp->కూడు_Pinsp(
            కొత్త AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        ఒకవేళ (rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం && !(isGlobalConstrained && memberselp)) {
            methodp->కూడు_Pinsp(
                కొత్త AstConst{varp->fileline(), AstConst::Unsized64{}, rand_బహుళకం.సూచీ});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->కూడు_Stmtsp(methodp->makeStmt());
        ఒకవేళ (varp->lifetime().isStatic() && rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం) {
            AstCMethodHard* స్థిరరాశి markp = కొత్త AstCMethodHard{
                varp->fileline(),
                కొత్త AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), కణుపు_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_కణుపు_సమాసం* స్థిరరాశి namep = కొత్త AstCExpr{varp->fileline(), AstCExpr::శుద్ధ{},
                                                    "\"" + smtName + "\"", varp->వెడల్పు()};
            namep->dtypep(varp->dtypep());
            markp->కూడు_Pinsp(namep);
            markp->dtype_సమితి_Void();
            initTaskp->కూడు_Stmtsp(markp->makeStmt());
        }
        ఒకవేళ (isGlobalConstrained && memberselp && rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం) {
            సమితి_Rand_బహుళకం(varp, memberselp, smtName, rand_బహుళకం, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        ఒకవేళ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_కణుపు_సమాసం* createVarExprp(Ast_కణుపు_Var_Ref* nodep, స్థిరరాశి Randomize_బహుళకం rand_బహుళకం,
                                స్థిరరాశి std::string& smtName, స్థిరరాశి తార్కిక isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* స్థిరరాశి varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_కణుపు_సమాసం* exprp = కొత్త AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, అబద్ధం, శూన్యసూచిక};
        ఒకవేళ (!rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం) {
            // Global constraints keep nodep alive for write_var processing
            ఒకవేళ (!isGlobalConstrained) VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_కణుపు_సమాసం* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_వృక్షం(అబద్ధం)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_కణుపు_సమాసం* rand_బహుళకం_Access;
        స్థిరరాశి తార్కిక varIsStatic = varp->lifetime().isStatic();
        AstClass* స్థిరరాశి varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : శూన్యసూచిక;
        AstVar* స్థిరరాశి owner_Static_Rand_బహుళకం_Varp
            = varOwningClassp ? కనుగొను_Static_Rand_బహుళకం_Var_Member(varOwningClassp) : శూన్యసూచిక;
        ఒకవేళ (owner_Static_Rand_బహుళకం_Varp) {
            rand_బహుళకం_Access = కొత్త AstVarRef{varp->fileline(),
                                           VN_AS(owner_Static_Rand_బహుళకం_Varp->user2p(), కణుపు_Module),
                                           owner_Static_Rand_బహుళకం_Varp, VAccess::READ};
        } లేకపోతే ఒకవేళ (memberselp) {
            Ast_కణుపు_Module* స్థిరరాశి varClassp = VN_AS(varp->user2p(), కణుపు_Module);
            AstVar* స్థిరరాశి effective_Rand_బహుళకం_Varp = get_Rand_బహుళకం_Var_From_Class(varClassp);
            ఒకవేళ (effective_Rand_బహుళకం_Varp) {
                // Member's class has randmode, use it
                Ast_కణుపు_సమాసం* మాతృ_Access = memberselp->fromp()->clone_వృక్షం(అబద్ధం);
                AstMemberSel* rand_బహుళకం_Sel
                    = కొత్త AstMemberSel{varp->fileline(), మాతృ_Access, effective_Rand_బహుళకం_Varp};
                rand_బహుళకం_Sel->dtypep(effective_Rand_బహుళకం_Varp->dtypep());
                rand_బహుళకం_Access = rand_బహుళకం_Sel;
            } లేకపోతే {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_rand_బహుళకం_Varp, nodep, "No m_randModeVarp");
                rand_బహుళకం_Access
                    = కొత్త AstVarRef{varp->fileline(), VN_AS(m_rand_బహుళకం_Varp->user2p(), కణుపు_Module),
                                    m_rand_బహుళకం_Varp, VAccess::READ};
            }
        } లేకపోతే {
            UASSERT_OBJ(m_rand_బహుళకం_Varp, nodep, "No m_randModeVarp");
            rand_బహుళకం_Access
                = కొత్త AstVarRef{varp->fileline(), VN_AS(m_rand_బహుళకం_Varp->user2p(), కణుపు_Module),
                                m_rand_బహుళకం_Varp, VAccess::READ};
        }

        AstCMethodHard* స్థిరరాశి atp
            = కొత్త AstCMethodHard{nodep->fileline(), rand_బహుళకం_Access, VCMethod::ARRAY_AT,
                                 కొత్త AstConst{nodep->fileline(), rand_బహుళకం.సూచీ}};
        atp->dtype_సమితి_UInt32();
        exprp = కొత్త AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(నిజం);  // Mark as formatted
        తిరిగి exprp;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        ఒకవేళ (varp->user4p()) {
            తార్కిక is_కొలత_Ref = అబద్ధం;
            ఒకవేళ (AstCMethodHard* స్థిరరాశి methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                ఒకవేళ (methodp->పద్ధతి() == VCMethod::ASSOC_కొలత
                    || methodp->పద్ధతి() == VCMethod::DYN_కొలత) {
                    is_కొలత_Ref = నిజం;
                }
            }
            ఒకవేళ (!is_కొలత_Ref && m_కొలత_Constrained_Arraysp) { m_కొలత_Constrained_Arraysp->చొప్పించు(varp); }
        }

        // Check if this variable is marked as globally constrained
        స్థిరరాశి తార్కిక isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = శూన్యసూచిక;
        తార్కిక structSelOrCMeth = అబద్ధం;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_కణుపు_DType* selectedDtypep = శూన్యసూచిక;
        std::string smtName;
        ఒకవేళ (m_nestedAccess) {
            m_nestedAccess->కూడు_Var_Name_Part(
                కొత్త AstSFormatF{varp->fileline(), nodep->name(), అబద్ధం, శూన్యసూచిక}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_వృక్షం();
        } లేకపోతే ఒకవేళ (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_కణుపు* శిఖరం_Member_Sel = nodep->backp();
            అయితే (VN_IS(శిఖరం_Member_Sel->backp(), MemberSel)) {
                శిఖరం_Member_Sel = శిఖరం_Member_Sel->backp();
            }
            memberselp = VN_AS(శిఖరం_Member_Sel, MemberSel)->clone_వృక్షం(అబద్ధం);
            smtName = build_Member_మార్గం(memberselp);
        } లేకపోతే ఒకవేళ (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_కణుపు* శిఖరం_Nodep = nodep->backp();
            అయితే (VN_IS(శిఖరం_Nodep->backp(), StructSel) || VN_IS(శిఖరం_Nodep->backp(), CMethodHard)) {
                శిఖరం_Nodep = శిఖరం_Nodep->backp();
            }

            ఒకవేళ (VN_IS(శిఖరం_Nodep, StructSel) || VN_IS(శిఖరం_Nodep, CMethodHard)) {
                structSelOrCMeth = నిజం;
                selectedDtypep = VN_AS(శిఖరం_Nodep, కణుపు_సమాసం)->dtypep();
            }

            memberselp = VN_CAST(శిఖరం_Nodep, MemberSel);
            ఒకవేళ (memberselp) smtName = build_Member_మార్గం(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            ఒకవేళ (smtName == "") smtName = nodep->name();
        } లేకపోతే {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        ఒకవేళ (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        ఒకవేళ (dtype_కలిగియుంది_Real(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            తిరిగి;
        }
        Ast_కణుపు_Module* స్థిరరాశి classOrPackagep = nodep->classOrPackagep();
        స్థిరరాశి Randomize_బహుళకం rand_బహుళకం = {.asUQuad = varp->user1()};
        ఒకవేళ (!rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం && editFormat(nodep)) తిరిగి;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType);
        ఒకవేళ (enumDtp
            && (std::కనుగొను(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.చివర(), varp)
                == m_rangeConstrainedEnums.చివర())) {
            AstVar* genVarp = m_genp;
            ఒకవేళ (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_కణుపు_సమాసం* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_కణుపు_FTask* లక్ష్యం_Taskp = m_inlineInitTaskp;
            ఒకవేళ (!లక్ష్యం_Taskp) లక్ష్యం_Taskp = get_Create_Constraint_అమరిక_Func(m_classp, m_memberMap);
            UASSERT_OBJ(లక్ష్యం_Taskp, nodep, "No function to inline enum range constraint into");
            లక్ష్యం_Taskp->కూడు_Stmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.నెట్టు_వెనుక(varp);
        }

        VNRelinker relinker;
        nodep->unlink_Fr_వెనుక(&relinker);
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp
            = createVarExprp(nodep, rand_బహుళకం, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        స్థిరరాశి తార్కిక alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.లెక్క(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.లెక్క(smtName) > 0
                                    : memberselp          ? m_writtenVars.లెక్క(smtName) > 0
                                                          : varp->user3();
        స్థిరరాశి తార్కిక shouldWriteVar = !alreadyWritten;
        ఒకవేళ (shouldWriteVar) {
            // Track this variable path as written
            ఒకవేళ (m_inlineInitTaskp)
                m_inlineWrittenVars.చొప్పించు(smtName);
            లేకపోతే ఒకవేళ (isGlobalConstrained || memberselp)
                m_writtenVars.చొప్పించు(smtName);
            // For global constraints, delete nodep after processing
            ఒకవేళ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);

            తార్కిక isClassRefArray = అబద్ధం;
            తార్కిక isUnpackedClassRefArray = అబద్ధం;
            AstClassRefDType* elemClassRefDtp = శూన్యసూచిక;
            {
                Ast_కణుపు_DType* varDtp = varp->dtypep()->skipRefp();
                ఒకవేళ (varDtp->isNonPackedArray()) {
                    Ast_కణుపు_DType* స్థిరరాశి elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    ఒకవేళ (elemClassRefDtp) {
                        isClassRefArray = నిజం;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_కణుపు_Module* స్థిరరాశి classp = getLeftmostVarModulep(memberselp, varp);
            Ast_కణుపు_FTask* స్థిరరాశి initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            ఒకవేళ (m_nestedAccess) {
                Ast_Stmt_సమాసం* స్థిరరాశి writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                కూడు_Registration_Stmt(writeVarStmtp, initTaskp);
                ఒకవేళ (isGlobalConstrained && memberselp && rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం) {
                    సమితి_Rand_బహుళకం(varp, memberselp, smtName, rand_బహుళకం, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                ఒకవేళ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } లేకపోతే ఒకవేళ (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } లేకపోతే {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, rand_బహుళకం,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } లేకపోతే {
            // Variable already written, clean up cloned memberselp if any
            ఒకవేళ (memberselp) VL_DO_DANGLING(memberselp->delete_వృక్షం(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            ఒకవేళ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
        ఒకవేళ (m_nestedAccess) { VL_DO_DANGLING(తొలగించు m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_కణుపు_సమాసం* build_లెక్క_Ones_Expansion(File_పంక్తి* fl, Ast_కణుపు_సమాసం* argp) {
        స్థిరరాశి V3_సంఖ్య అంకె_One{fl, argp->వెడల్పు(), 1};
        Ast_కణుపు_సమాసం* sump = కొత్త AstAnd{fl, argp, కొత్త AstConst{fl, అంకె_One}};
        sump->user1(నిజం);
        కొరకు (పూర్ణాంకం i = 1; i < argp->వెడల్పు(); i++) {
            V3_సంఖ్య అంకె_బిట్టు_ముసుగు{fl, argp->వెడల్పు(), 0};
            అంకె_బిట్టు_ముసుగు.సమితి_బిట్టు(i, 1);
            AstAnd* స్థిరరాశి andp
                = కొత్త AstAnd{fl, argp->clone_వృక్షం_శుద్ధ(అబద్ధం), కొత్త AstConst{fl, అంకె_బిట్టు_ముసుగు}};
            andp->user1(నిజం);
            Ast_జరుపు_R* స్థిరరాశి shiftp
                = కొత్త Ast_జరుపు_R{fl, andp,
                                కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, argp->వెడల్పు(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(నిజం);
            sump = కొత్త Ast_కూడు{fl, sump, shiftp};
            sump->user1(నిజం);
        }
        తిరిగి sump;
    }

    శూన్య సందర్శించు(Ast_లెక్క_Ones* nodep) override {
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి argp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* sump = build_లెక్క_Ones_Expansion(fl, argp);
        sump = adjust_వెడల్పు(fl, sump, nodep->వెడల్పు(), nodep->dtypep()->numeric());
        sump->user1(నిజం);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(sump);
    }
    శూన్య సందర్శించు(AstRedXor* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి argp = nodep->lhsp()->unlink_Fr_వెనుక();

        Ast_కణుపు_సమాసం* redxorp = కొత్త AstSel{fl, argp, 0, 1};
        redxorp->user1(నిజం);
        కొరకు (పూర్ణాంకం i = 1; i < argp->వెడల్పు(); i++) {
            AstSel* స్థిరరాశి selp = కొత్త AstSel{fl, argp->clone_వృక్షం_శుద్ధ(అబద్ధం), i, 1};
            selp->user1(నిజం);

            redxorp = కొత్త AstXor{fl, redxorp, selp};
            redxorp->user1(నిజం);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(redxorp);
    }
    శూన్య సందర్శించు(AstRedAnd* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        // Convert to (~x == 0)
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి argp = nodep->lhsp()->unlink_Fr_వెనుక();
        స్థిరరాశి V3_సంఖ్య అంకె_Zero{fl, argp->వెడల్పు(), 0};
        Ast_కణుపు_సమాసం* స్థిరరాశి negp = కొత్త AstNot{fl, argp};
        negp->user1(నిజం);
        Ast_కణుపు_సమాసం* స్థిరరాశి eqp = కొత్త AstEq{fl, negp, కొత్త AstConst{fl, అంకె_Zero}};
        eqp->user1(నిజం);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(eqp);
    }
    శూన్య సందర్శించు(AstRedOr* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        // Convert to (x != 0)
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి argp = nodep->lhsp()->unlink_Fr_వెనుక();
        స్థిరరాశి V3_సంఖ్య అంకె_Zero{fl, argp->వెడల్పు(), 0};
        Ast_కణుపు_సమాసం* స్థిరరాశి neqp = కొత్త AstNeq{fl, argp, కొత్త AstConst{fl, అంకె_Zero}};
        neqp->user1(నిజం);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(neqp);
    }
    శూన్య సందర్శించు(AstOneHot* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి argp = nodep->lhsp()->unlink_Fr_వెనుక();
        స్థిరరాశి పూర్ణాంకం w = argp->వెడల్పు();

        స్థిరరాశి V3_సంఖ్య అంకె_Zero{fl, w, 0};
        AstNeq* స్థిరరాశి neZerop
            = కొత్త AstNeq{fl, argp->clone_వృక్షం_శుద్ధ(అబద్ధం), కొత్త AstConst{fl, అంకె_Zero}};
        neZerop->user1(నిజం);

        స్థిరరాశి V3_సంఖ్య అంకె_One{fl, w, 1};
        AstSub* స్థిరరాశి subp = కొత్త AstSub{fl, argp->clone_వృక్షం_శుద్ధ(అబద్ధం), కొత్త AstConst{fl, అంకె_One}};
        subp->user1(నిజం);

        AstAnd* స్థిరరాశి andp = కొత్త AstAnd{fl, argp, subp};
        andp->user1(నిజం);

        స్థిరరాశి V3_సంఖ్య అంకె_Zero2{fl, w, 0};
        AstEq* స్థిరరాశి eqZerop = కొత్త AstEq{fl, andp, కొత్త AstConst{fl, అంకె_Zero2}};
        eqZerop->user1(నిజం);

        AstLogAnd* స్థిరరాశి resultp = కొత్త AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(నిజం);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(resultp);
    }
    శూన్య సందర్శించు(AstOneHot0* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        // $onehot0(x) = (x & (x-1)) == 0
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి argp = nodep->lhsp()->unlink_Fr_వెనుక();
        స్థిరరాశి పూర్ణాంకం w = argp->వెడల్పు();

        స్థిరరాశి V3_సంఖ్య అంకె_One{fl, w, 1};
        AstSub* స్థిరరాశి subp = కొత్త AstSub{fl, argp->clone_వృక్షం_శుద్ధ(అబద్ధం), కొత్త AstConst{fl, అంకె_One}};
        subp->dtypeFrom(argp);
        subp->user1(నిజం);

        AstAnd* స్థిరరాశి andp = కొత్త AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(నిజం);

        స్థిరరాశి V3_సంఖ్య అంకె_Zero{fl, w, 0};
        AstEq* స్థిరరాశి eqp = కొత్త AstEq{fl, andp, కొత్త AstConst{fl, అంకె_Zero}};
        eqp->user1(నిజం);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(eqp);
    }
    శూన్య సందర్శించు(Ast_లెక్క_బిట్లు* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();

        తార్కిక లెక్క_Ones = అబద్ధం;
        తార్కిక లెక్క_Zeros = అబద్ధం;
        కొరకు (Ast_కణుపు_సమాసం* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            స్థిరరాశి AstConst* స్థిరరాశి cp = VN_CAST(ctrlp, Const);
            ఒకవేళ (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* స్థిరరాశి zerop
                    = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, nodep->వెడల్పు(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                తిరిగి;
            }
            ఒకవేళ (cp->అంకె().బిట్టు_Is1(0) && !లెక్క_Ones)
                లెక్క_Ones = నిజం;
            లేకపోతే ఒకవేళ (cp->అంకె().బిట్టు_Is0(0) && !లెక్క_Zeros)
                లెక్క_Zeros = నిజం;
        }

        Ast_కణుపు_సమాసం* స్థిరరాశి argp = nodep->lhsp()->unlink_Fr_వెనుక();
        స్థిరరాశి పూర్ణాంకం arg_వెడల్పు = argp->వెడల్పు();
        Ast_కణుపు_సమాసం* sump = శూన్యసూచిక;
        ఒకవేళ (లెక్క_Ones && లెక్క_Zeros) {
            // ones + zeros = width for 2-state types
            sump = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, nodep->వెడల్పు(), (uint32_t)arg_వెడల్పు};
            VL_DO_DANGLING(argp->delete_వృక్షం(), argp);
        } లేకపోతే ఒకవేళ (లెక్క_Ones) {
            sump = build_లెక్క_Ones_Expansion(fl, argp);
        } లేకపోతే ఒకవేళ (లెక్క_Zeros) {
            // width - countones(x)
            Ast_కణుపు_సమాసం* స్థిరరాశి onesCountp = build_లెక్క_Ones_Expansion(fl, argp);
            స్థిరరాశి V3_సంఖ్య వెడల్పు_Val{nodep, onesCountp->వెడల్పు(), (uint32_t)arg_వెడల్పు};
            sump = కొత్త AstSub{fl, కొత్త AstConst{fl, వెడల్పు_Val}, onesCountp};
            sump->user1(నిజం);
        } లేకపోతే {
            sump = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, nodep->వెడల్పు(), 0U};
            VL_DO_DANGLING(argp->delete_వృక్షం(), argp);
        }

        sump = adjust_వెడల్పు(fl, sump, nodep->వెడల్పు(), nodep->dtypep()->numeric());
        sump->user1(నిజం);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(sump);
    }
    శూన్య సందర్శించు(AstCLog2* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి argp = nodep->lhsp()->unlink_Fr_వెనుక();
        స్థిరరాశి పూర్ణాంకం arg_వెడల్పు = argp->వెడల్పు();
        స్థిరరాశి పూర్ణాంకం ఫలితం_వెడల్పు = nodep->వెడల్పు();

        Ast_కణుపు_సమాసం* resultp
            = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, ఫలితం_వెడల్పు, (uint32_t)arg_వెడల్పు};

        కొరకు (పూర్ణాంకం k = arg_వెడల్పు - 1; k >= 0; k--) {
            V3_సంఖ్య పరిమితి{fl, arg_వెడల్పు, 0};
            ఒకవేళ (k < 32)
                పరిమితి.సమితి_Long(1ULL << k);
            లేకపోతే
                పరిమితి.సమితి_బిట్టు(k, 1);

            AstLte* స్థిరరాశి ltep
                = కొత్త AstLte{fl, argp->clone_వృక్షం_శుద్ధ(అబద్ధం), కొత్త AstConst{fl, పరిమితి}};
            ltep->user1(నిజం);

            AstConst* స్థిరరాశి valuep
                = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, ఫలితం_వెడల్పు, (uint32_t)k};

            resultp = కొత్త AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(నిజం);
        }

        VL_DO_DANGLING(argp->delete_వృక్షం(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        iterate(resultp);
    }
    శూన్య సందర్శించు(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* స్థిరరాశి zerop = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    శూన్య handlePow(Ast_కణుపు_Biop* nodep) {
        ఒకవేళ (AstConst* స్థిరరాశి exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_సమాసం* స్థిరరాశి basep = nodep->lhsp();
            స్థిరరాశి V3_సంఖ్య అంకె_One{nodep, basep->వెడల్పు(), 1};
            Ast_కణుపు_సమాసం* powerp = కొత్త AstConst{fl, అంకె_One};
            స్థిరరాశి తార్కిక పునాది_Signed = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            స్థిరరాశి int32_t exponent = పునాది_Signed ? exponentp->గమ్యం_SInt() : exponentp->గమ్యం_UInt();
            ఒకవేళ (exponent > 0) {
                కొరకు (int32_t i = 0; i < exponent; i++) {
                    ఒకవేళ (పునాది_Signed) {
                        powerp = కొత్త AstMulS{fl, powerp, basep->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    } లేకపోతే {
                        powerp = కొత్త AstMul{fl, powerp, basep->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    }
                    powerp->user1(నిజం);
                }
            } లేకపోతే ఒకవేళ (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                ఒకవేళ (పునాది_Signed) {
                    powerp = కొత్త AstDivS{fl, powerp, basep->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    powerp->user1(నిజం);
                    powerp = కొత్త AstDivS{fl, powerp, basep->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    powerp->user1(నిజం);
                } లేకపోతే {
                    powerp = కొత్త AstDiv{fl, powerp, basep->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    powerp->user1(నిజం);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            iterate(powerp);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి basep = VN_CAST(nodep->lhsp(), Const)) {
            ఒకవేళ (basep->అంకె().గమ్యం_UInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
                స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->వెడల్పు();
                స్థిరరాశి తార్కిక rhsDependent = rhsp->user1();
                AstConst* స్థిరరాశి onep = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, వెడల్పు, 1};
                Ast_జరుపు_L* స్థిరరాశి shiftp = కొత్త Ast_జరుపు_L{fl, onep, rhsp, వెడల్పు};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_కణుపు_సమాసం* resultp = shiftp;
                ఒకవేళ (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_కణుపు_సమాసం* స్థిరరాశి nClonep = rhsp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    nClonep->user1(rhsDependent);
                    AstConst* స్థిరరాశి zeroNp
                        = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, rhsp->వెడల్పు(), 0};
                    AstGteS* స్థిరరాశి condp = కొత్త AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* స్థిరరాశి zeroRp = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, వెడల్పు, 0};
                    resultp = కొత్త AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                iterate(resultp);
            } లేకపోతే {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } లేకపోతే {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    శూన్య సందర్శించు(AstPow* nodep) override { handlePow(nodep); }
    శూన్య సందర్శించు(AstPowSS* nodep) override { handlePow(nodep); }
    శూన్య సందర్శించు(AstPowSU* nodep) override { handlePow(nodep); }
    శూన్య సందర్శించు(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    శూన్య handle_జరుపు(Ast_కణుపు_Biop* nodep) {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        స్థిరరాశి పూర్ణాంకం lhs_వెడల్పు = nodep->lhsp()->వెడల్పు();
        స్థిరరాశి పూర్ణాంకం rhs_వెడల్పు = nodep->rhsp()->వెడల్పు();
        ఒకవేళ (rhs_వెడల్పు < lhs_వెడల్పు) {
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
            AstExtend* స్థిరరాశి extendp = కొత్త AstExtend{fl, rhsp, lhs_వెడల్పు};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(నిజం);
            nodep->rhsp(extendp);
        } లేకపోతే ఒకవేళ (rhs_వెడల్పు > lhs_వెడల్పు) {
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->dtypep();
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి extendp
                = VN_IS(nodep, జరుపు_RS)
                      ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtendS{fl, lhsp, rhs_వెడల్పు})
                      : static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtend{fl, lhsp, rhs_వెడల్పు});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(నిజం);
            nodep->lhsp(extendp);
            nodep->dtype_సమితి_Logic_Sized(rhs_వెడల్పు, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlink_Fr_వెనుక(&relinker);
            AstSel* స్థిరరాశి selp = కొత్త AstSel{fl, nodep, 0, lhs_వెడల్పు};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            తిరిగి;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    శూన్య సందర్శించు(Ast_జరుపు_L* nodep) override { handle_జరుపు(nodep); }
    శూన్య సందర్శించు(Ast_జరుపు_R* nodep) override { handle_జరుపు(nodep); }
    శూన్య సందర్శించు(Ast_జరుపు_RS* nodep) override { handle_జరుపు(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_Biop* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Uniop* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        editSMT(nodep, nodep->lhsp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Triop* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        ఒకవేళ (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtype_సమితి_String();
            తిరిగి;
        }
        // Fall back to "(ite cond then else)"
        సందర్శించు(static_cast<Ast_కణుపు_Triop*>(nodep));
    }
    శూన్య సందర్శించు(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        editSMT(nodep, nodep->srcp());
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        VNRelinker handle;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి msbp = కొత్త AstSFormatF{
            fl, "%1d", అబద్ధం,
            కొత్త Ast_కూడు{fl, nodep->lsbp()->clone_వృక్షం_శుద్ధ(అబద్ధం),
                       కొత్త AstConst{fl, static_cast<uint32_t>(nodep->వెడల్పు_Const() - 1)}}};
        Ast_కణుపు_సమాసం* స్థిరరాశి lsbp
            = కొత్త AstSFormatF{fl, "%1d", అబద్ధం, nodep->lsbp()->unlink_Fr_వెనుక(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        స్థిరరాశి తార్కిక outermost = !m_defer_Rand_బహుళకం_Hoist;
        Ast_కణుపు_సమాసం* స్థిరరాశి origp = outermost ? nodep->clone_వృక్షం(అబద్ధం) : శూన్యసూచిక;
        VL_RESTORER(m_structSel);
        m_structSel = నిజం;
        ఒకవేళ (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = nodep->fromp();
            AstStructDType* స్థిరరాశి structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ఒకవేళ (is_Rand_Qualified_మార్గం(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } లేకపోతే {
                structp->markConstrainedRand(నిజం);
            }
        }
        // Mark Random for structArray
        ఒకవేళ (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* స్థిరరాశి structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ఒకవేళ (is_Rand_Qualified_మార్గం(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } లేకపోతే {
                structp->markConstrainedRand(నిజం);
            }
        }
        {
            VL_RESTORER(m_defer_Rand_బహుళకం_Hoist);
            m_defer_Rand_బహుళకం_Hoist = నిజం;
            iterateChildren(nodep);
        }
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        AstSFormatF* newp = శూన్యసూచిక;
        ఒకవేళ (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = కొత్త AstSFormatF{fl, "%s.%s." + nodep->name(), అబద్ధం,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_వృక్షం_శుద్ధ(నిజం)};
            ఒకవేళ (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } లేకపోతే {
            newp = కొత్త AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), అబద్ధం,
                                   శూన్యసూచిక};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        ఒకవేళ (origp && !hoist_Rand_బహుళకం_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(నెట్టు_Deletep(origp), origp);
        }
    }
    శూన్య కూడు_String_Name_Part(Ast_కణుపు_సమాసం* స్థిరరాశి indexp, స్థిరరాశి std::string& fmt = "%32x") {
        ఒకవేళ (m_nestedAccess) {
            Ast_కణుపు_సమాసం* స్థిరరాశి bitp = indexp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            Ast_కణుపు_సమాసం* స్థిరరాశి బిట్టు_Formatp = కొత్త AstSFormatF{bitp->fileline(), fmt, అబద్ధం, bitp};
            AstSFormatF* స్థిరరాశి herep
                = కొత్త AstSFormatF{indexp->fileline(), "%s.%s", అబద్ధం, బిట్టు_Formatp};
            m_nestedAccess->కూడు_Var_Name_Part(herep, indexp->name());
        }
    }
    శూన్య సందర్శించు(AstAssocSel* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_కణుపు_సమాసం* స్థిరరాశి origp = nodep->clone_వృక్షం(అబద్ధం);
        AstSFormatF* newp = శూన్యసూచిక;
        ఒకవేళ (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            కూడు_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_కణుపు_సమాసం* స్థిరరాశి idxp = కొత్త AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), అబద్ధం,
                                                      nodep->bitp()->unlink_Fr_వెనుక(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } లేకపోతే ఒకవేళ (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* స్థిరరాశి stringp = VN_AS(nodep->bitp(), CvtPackString);
            స్థిరరాశి కొలత_t string_కొలత = VN_AS(stringp->lhsp(), Const)->వెడల్పు();
            ఒకవేళ (string_కొలత > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << string_కొలత << "bits, limit is 128 bits");
            }
            కూడు_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_కణుపు_సమాసం* స్థిరరాశి idxp = కొత్త AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), అబద్ధం,
                                                      stringp->lhsp()->unlink_Fr_వెనుక(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } లేకపోతే {
            ఒకవేళ (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                స్థిరరాశి పూర్ణాంకం actual_వెడల్పు = nodep->bitp()->వెడల్పు();
                std::string fmt;
                // Normalize to standard bit width
                ఒకవేళ (actual_వెడల్పు <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } లేకపోతే ఒకవేళ (actual_వెడల్పు <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } లేకపోతే {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::గమ్యం_string(VL_పదాలు_I(actual_వెడల్పు) * 8) + "x";
                }
                ఒకవేళ (m_nestedAccess) కూడు_String_Name_Part(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_కణుపు_సమాసం* స్థిరరాశి idxp
                    = కొత్త AstSFormatF{fl, fmt, అబద్ధం, nodep->bitp()->unlink_Fr_వెనుక(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } లేకపోతే {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                ఒకవేళ (m_nestedAccess) m_nestedAccess->error();
            }
        }
        ఒకవేళ (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        ఒకవేళ (!newp || !hoist_Rand_బహుళకం_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(origp->delete_వృక్షం(), origp);
        }
    }
    శూన్య rename_Array_సమాసం(AstSFormatF* స్థిరరాశి newp) స్థిరరాశి {
        ఒకవేళ (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    శూన్య సందర్శించు(AstArraySel* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        తార్కిక సూచీ_Is_Rand = అబద్ధం;
        nodep->bitp()->foreach([&](స్థిరరాశి Ast_కణుపు_Var_Ref* vrefp) {
            ఒకవేళ (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                సూచీ_Is_Rand = నిజం;
            }
        });
        ఒకవేళ (m_nestedAccess) {
            Ast_కణుపు_సమాసం* స్థిరరాశి bitp = nodep->bitp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
            Ast_కణుపు_సమాసం* స్థిరరాశి బిట్టు_Formatp
                = సూచీ_Is_Rand ? కొత్త AstSFormatF{bitp->fileline(), bitp->name(), అబద్ధం, శూన్యసూచిక}
                              : కొత్త AstSFormatF{bitp->fileline(), "%x", అబద్ధం, bitp};
            AstSFormatF* స్థిరరాశి herep
                = కొత్త AstSFormatF{nodep->fileline(), "%s.%s", అబద్ధం, బిట్టు_Formatp};
            ఒకవేళ (AstSel* స్థిరరాశి selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->కూడు_Var_Name_Part(herep, selp->fromp()->name());
            } లేకపోతే {
                m_nestedAccess->కూడు_Var_Name_Part(herep, bitp->name());
            }
            ఒకవేళ (సూచీ_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_వృక్షం(), bitp);
                తిరిగి;
            }
        }
        ఒకవేళ (సూచీ_Is_Rand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_కణుపు_సమాసం* indexp = nodep->bitp()->unlink_Fr_వెనుక(&handle);
            ఒకవేళ (indexp->వెడల్పు() < 32) {
                AstExtend* స్థిరరాశి extendp = కొత్త AstExtend{fl, indexp, 32};
                extendp->user1(నిజం);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* స్థిరరాశి newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_సమాసం(newp);
        } లేకపోతే {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_కణుపు_సమాసం* స్థిరరాశి origp = nodep->clone_వృక్షం(అబద్ధం);
            Ast_కణుపు_సమాసం* స్థిరరాశి indexp
                = కొత్త AstSFormatF{fl, "#x%8x", అబద్ధం, nodep->bitp()->unlink_Fr_వెనుక(&handle)};
            handle.relink(indexp);
            AstSFormatF* స్థిరరాశి newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_సమాసం(newp);
            ఒకవేళ (!newp || !hoist_Rand_బహుళకం_Over_Select_And_Member(newp, origp)) {
                VL_DO_DANGLING(origp->delete_వృక్షం(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    స్థిర తార్కిక is_Rand_బహుళకం_Gate(స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        స్థిరరాశి AstCMethodHard* స్థిరరాశి atp = VN_CAST(exprp, CMethodHard);
        ఒకవేళ (!atp || atp->పద్ధతి() != VCMethod::ARRAY_AT) తిరిగి అబద్ధం;
        స్థిరరాశి AstVar* varp = శూన్యసూచిక;
        ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        ఒకవేళ (!varp) తిరిగి అబద్ధం;
        స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(varp->user2p(), Class);
        తిరిగి classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    తార్కిక hoist_Rand_బహుళకం_Over_Select_And_Member(AstSFormatF* newp, Ast_కణుపు_సమాసం* origp) {
        ఒకవేళ (m_defer_Rand_బహుళకం_Hoist) తిరిగి అబద్ధం;
        // Only for selects yielding a non-array element (full chains)
        ఒకవేళ (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) తిరిగి అబద్ధం;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::సదిశ<AstSFormatF*> చట్రాలు;
        AstCond* బహుళకం_Condp = శూన్యసూచిక;
        కొరకు (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_కణుపు_సమాసం* స్థిరరాశి firstp = curp->exprsp();
            చట్రాలు.నెట్టు_వెనుక(curp);
            ఒకవేళ (AstCond* స్థిరరాశి condp = VN_CAST(firstp, Cond)) {
                ఒకవేళ (is_Rand_బహుళకం_Gate(condp->condp())) బహుళకం_Condp = condp;
                ఆపు;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        ఒకవేళ (!బహుళకం_Condp) తిరిగి అబద్ధం;
        File_పంక్తి* స్థిరరాశి fl = newp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి modep = బహుళకం_Condp->condp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* activep = బహుళకం_Condp->thenp()->unlink_Fr_వెనుక();
        // Rebuild the select chain text around the SMT name, innermost first
        కొరకు (స్వయంచాలక it = చట్రాలు.rbegin(); it != చట్రాలు.rend(); ++it) {
            Ast_కణుపు_సమాసం* స్థిరరాశి మిగిలిన_Argsp = VN_CAST((*it)->exprsp()->nextp(), కణుపు_సమాసం);
            ఒకవేళ (మిగిలిన_Argsp) { మిగిలిన_Argsp->unlink_Fr_వెనుక_With_తదుపరి(); }
            activep = కొత్త AstSFormatF{fl, (*it)->name(), అబద్ధం,
                                      Ast_కణుపు::కూడు_తదుపరి(activep, మిగిలిన_Argsp)};
        }
        AstCond* స్థిరరాశి hoistp = కొత్త AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(నిజం);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(నెట్టు_Deletep(newp), newp);
        తిరిగి నిజం;
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        ఒకవేళ (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
            ఒకవేళ (m_nestedAccess) {
                AstSFormatF* స్థిరరాశి formatp = కొత్త AstSFormatF{fl, nodep->name(), అబద్ధం, శూన్యసూచిక};
                m_nestedAccess->కూడు_Var_Name_Part(కొత్త AstSFormatF{fl, "%s.%s", అబద్ధం, formatp},
                                               nodep->name());
            }
            Ast_కణుపు* మూలం_కణుపు = nodep->fromp();
            అయితే (స్థిరరాశి AstMemberSel* స్థిరరాశి selp = VN_CAST(మూలం_కణుపు, MemberSel)) {
                మూలం_కణుపు = selp->fromp();
            }

            Ast_కణుపు_Sel* arraySelp = VN_CAST(మూలం_కణుపు, ArraySel);
            ఒకవేళ (!arraySelp) arraySelp = VN_CAST(మూలం_కణుపు, AssocSel);

            ఒకవేళ (arraySelp) {
                Ast_కణుపు_DType* స్థిరరాశి arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_కణుపు_DType* స్థిరరాశి elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : శూన్యసూచిక;
                ఒకవేళ (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    స్థిరరాశి తార్కిక isSimple
                        = nodep->fromp() == మూలం_కణుపు && VN_IS(arraySelp->fromp(), VarRef);
                    ఒకవేళ (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = కొత్త Nested_Access_మార్గం(nodep->clone_వృక్షం(అబద్ధం),
                                                              &m_మొదటి_వ్యక్తీకరణ_Inside_Indexp);
                        AstSFormatF* స్థిరరాశి formatp
                            = కొత్త AstSFormatF{fl, nodep->name(), అబద్ధం, శూన్యసూచిక};
                        m_nestedAccess->కూడు_Var_Name_Part(
                            కొత్త AstSFormatF{fl, "%s.%s", అబద్ధం, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = నిజం;
                    nodep->user1(నిజం);
                    arraySelp->user1(నిజం);
                    Ast_కణుపు_సమాసం* స్థిరరాశి origp = nodep->clone_వృక్షం(అబద్ధం);
                    iterateChildren(nodep);
                    AstSFormatF* స్థిరరాశి newp = కొత్త AstSFormatF{fl, "%s." + nodep->name(), అబద్ధం,
                                                              nodep->fromp()->unlink_Fr_వెనుక()};
                    nodep->replaceWith(newp);
                    ఒకవేళ (!hoist_Rand_బహుళకం_Over_Select_And_Member(newp, origp))
                        VL_DO_DANGLING(origp->delete_వృక్షం(), origp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    VL_DO_DANGLING(తొలగించు m_nestedAccess, m_nestedAccess);
                    తిరిగి;
                }
            }
            // Check if the root variable participates in global constraints
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి varRefp = VN_CAST(మూలం_కణుపు, VarRef)) {
                AstVar* స్థిరరాశి constrainedVar = varRefp->varp();
                ఒకవేళ (constrainedVar->globalConstrained()) {
                    తార్కిక స్థిరరాశి beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    ఒకవేళ (beforeIterate) {
                        nodep->replaceWith(కొత్త AstSFormatF{fl, "%s." + nodep->name(), అబద్ధం,
                                                           nodep->fromp()->unlink_Fr_వెనుక()});
                    } లేకపోతే {
                        nodep->replaceWith(nodep->fromp()->unlink_Fr_వెనుక());
                    }
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                    తిరిగి;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        ఒకవేళ (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), కణుపు_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* స్థిరరాశి varRefp
                = కొత్త AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), కణుపు_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            సందర్శించు(varRefp);
        } లేకపోతే ఒకవేళ (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->పద్ధతి() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* స్థిరరాశి cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_కణుపు_DType* స్థిరరాశి arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_కణుపు_DType* స్థిరరాశి elemDtp = arrayDtp->subDTypep()->skipRefp();
            ఒకవేళ (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = నిజం;
                iterateChildren(nodep);
                File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                AstSFormatF* newp = శూన్యసూచిక;
                ఒకవేళ (AstSFormatF* స్థిరరాశి fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    ఒకవేళ (fromp->name() == "%s.%s") {
                        newp = కొత్త AstSFormatF{fl, "%s.%s." + nodep->name(), అబద్ధం,
                                               fromp->exprsp()->clone_వృక్షం_శుద్ధ(నిజం)};
                    } లేకపోతే {
                        newp = కొత్త AstSFormatF{fl, fromp->name() + "." + nodep->name(), అబద్ధం,
                                               శూన్యసూచిక};
                    }
                } లేకపోతే {
                    newp = కొత్త AstSFormatF{fl, nodep->name(), అబద్ధం, శూన్యసూచిక};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        } లేకపోతే ఒకవేళ (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        } లేకపోతే {
            editFormat(nodep);
        }
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {}
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {}
    శూన్య సందర్శించు(AstCStmt* nodep) override {}
    శూన్య సందర్శించు(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    శూన్య సందర్శించు(AstConstraintIf* nodep) override {
        Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి thenp = editSingle(fl, nodep->thensp());
        Ast_కణుపు_సమాసం* స్థిరరాశి elsep = editSingle(fl, nodep->elsesp());
        ఒకవేళ (thenp && elsep) {
            newp = కొత్త AstCond{fl, nodep->condp()->unlink_Fr_వెనుక(), thenp, elsep};
        } లేకపోతే ఒకవేళ (thenp) {
            newp = కొత్త AstLogIf{fl, nodep->condp()->unlink_Fr_వెనుక(), thenp};
        } లేకపోతే ఒకవేళ (elsep) {
            newp = కొత్త AstLogIf{fl, కొత్త AstNot{fl, nodep->condp()->unlink_Fr_వెనుక()}, elsep};
        }
        ఒకవేళ (newp) {
            newp->dtype_సమితి_బిట్టు();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(నిజం);  // Assume result-dependent
            nodep->replaceWith(కొత్త Ast_Constraint_సమాసం{fl, newp});
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {}
    శూన్య సందర్శించు(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();

        ఒకవేళ (!nodep->bodyp()) {
            nodep->unlink_Fr_వెనుక();
        } లేకపోతే ఒకవేళ (m_wantSingle) {
            Ast_కణుపు_సమాసం* స్థిరరాశి itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{fl};
            cstmtp->కూడు("ret += \" \";\n");
            cstmtp->కూడు("ret += ");
            cstmtp->కూడు(itemp);
            cstmtp->కూడు(";");
            Ast_కణుపు* స్థిరరాశి bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* స్థిరరాశి cexprp = కొత్త AstCExpr{fl};
            cexprp->dtype_సమితి_String();
            cexprp->కూడు("([&]{\nstd::string ret;\n");
            cexprp->కూడు(కొత్త AstBegin{fl, m_unique_పేర్లు.get("__VrandBlock"),
                                     కొత్త AstForeach{fl, nodep->headerp()->unlink_Fr_వెనుక(), bodyp},
                                     నిజం});
            cexprp->కూడు("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(కొత్త AstSFormatF{fl, "%s", అబద్ధం, cexprp});
        } లేకపోతే {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            కొరకు (Ast_కణుపు* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(elemp, Var)) m_foreachIdxVars.చొప్పించు(varp);
            }
            iterate_And_తదుపరి_Null(nodep->bodyp());
            Ast_కణుపు* స్థిరరాశి bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlink_Fr_వెనుక_With_తదుపరి());
            nodep->replaceWith(
                కొత్త AstBegin{fl, m_unique_పేర్లు.get("__VrandBlock"),
                             కొత్త AstForeach{fl, nodep->headerp()->unlink_Fr_వెనుక(), bodyp}, నిజం});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_Module* స్థిరరాశి genModp = VN_AS(m_genp->user2p(), కణుపు_Module);

        కొరకు (Ast_కణుపు_సమాసం* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), కణుపు_సమాసం)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsTestp = build_Solve_Before_Name_సమాసం(fl, lhsp);
            ఒకవేళ (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                కొనసాగు;
            }
            VL_DO_DANGLING(lhsTestp->delete_వృక్షం(), lhsTestp);
            కొరకు (Ast_కణుపు_సమాసం* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), కణుపు_సమాసం)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి rhsNamep = build_Solve_Before_Name_సమాసం(fl, rhsp);
                ఒకవేళ (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    కొనసాగు;
                }
                AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
                    fl, కొత్త AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtype_సమితి_Void();
                callp->కూడు_Pinsp(build_Solve_Before_Name_సమాసం(fl, lhsp));
                callp->కూడు_Pinsp(rhsNamep);
                nodep->కూడు_ఇక్కడ_This_As_తదుపరి(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstConstraintUnique* nodep) override {
        Ast_కణుపు_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_Module* genModp = శూన్యసూచిక;

        ఒకవేళ (m_classp) {
            initTaskp = VN_AS(m_memberMap.కనుగొను_Member(m_classp, "new"), కణుపు_FTask);
            genVarp = getRandomGenerator(m_classp);
        } లేకపోతే {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        ఒకవేళ (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        ఒకవేళ (m_classp) {
            genModp = VN_AS(genVarp->user2p(), కణుపు_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_కణుపు* అమరిక_Stmtsp = శూన్యసూచిక;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::సదిశ<std::string> smtExprs;
        కొరకు (Ast_కణుపు* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (AstVarRef* స్థిరరాశి varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* స్థిరరాశి varp = varRefp->varp();
                Ast_కణుపు_Module* స్థిరరాశి varModp = VN_AS(varp->user2p(), కణుపు_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep()->skipRefp();

                స్థిరరాశి స్థిర స్వయంచాలక dynDTypeSupported
                    = [](స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep) -> తార్కిక {
                    తిరిగి VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, వరుస_DType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elem_వెడల్పు;
                uint32_t static_కొలత = 0;  // Element count, zero when only known at run time
                ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి పైకి = VN_CAST(dtypep, UnpackArrayDType)) {
                    స్థిరరాశి AstRange* స్థిరరాశి rangep = పైకి->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    static_కొలత = పైకి->elementsConst();
                    ఒకవేళ (static_కొలత > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        కొనసాగు;
                    }
                    elem_వెడల్పు = static_cast<uint64_t>(varp->dtypep()->వెడల్పు());
                } లేకపోతే ఒకవేళ (dynDTypeSupported(dtypep)) {
                    elem_వెడల్పు = static_cast<uint64_t>(dtypep->subDTypep()->వెడల్పు());
                } లేకపోతే {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(అబద్ధం));
                    కొనసాగు;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి subp = dtypep->subDTypep()->skipRefp();
                ఒకవేళ (VN_IS(subp, కణుపు_Array_DType) || VN_IS(subp, వరుస_DType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    కొనసాగు;
                }

                ఒకవేళ (m_wantSingle && !static_కొలత) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    కొనసాగు;
                }

                ఒకవేళ (!varp->user3()) {
                    varp->user3(నిజం);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_కణుపు_సమాసం* స్థిరరాశి varnamep = కొత్త AstCExpr{
                        fl, AstCExpr::శుద్ధ{}, "\"" + varp->name() + "\"", varp->వెడల్పు()};

                    AstCMethodHard* స్థిరరాశి writeVarCallp = కొత్త AstCMethodHard{
                        fl, కొత్త AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* స్థిరరాశి argVarRefp = కొత్త AstVarRef{fl, varp, VAccess::READ};
                    ఒకవేళ (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->కూడు_Pinsp(argVarRefp);
                    writeVarCallp->కూడు_Pinsp(కొత్త AstConst{fl, AstConst::Unsized64{}, elem_వెడల్పు});
                    writeVarCallp->కూడు_Pinsp(varnamep);
                    writeVarCallp->కూడు_Pinsp(కొత్త AstConst{fl, 1});  // Dimension
                    స్థిరరాశి Randomize_బహుళకం rand_బహుళకం = {.asUQuad = varp->user1()};
                    ఒకవేళ (rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం) {
                        writeVarCallp->కూడు_Pinsp(
                            కొత్త AstConst{fl, AstConst::Unsized64{}, rand_బహుళకం.సూచీ});
                    }
                    writeVarCallp->dtype_సమితి_Void();
                    initTaskp->కూడు_Stmtsp(కొత్త Ast_Stmt_సమాసం{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                ఒకవేళ (varp->user4p() && m_కొలత_Constrained_Arraysp) {
                    m_కొలత_Constrained_Arraysp->చొప్పించు(varp);
                }

                ఒకవేళ (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    ఒకవేళ (static_కొలత < 2) కొనసాగు;
                    std::string సమాసం_Str = "(__Vbv (distinct";
                    కొరకు (uint32_t i = 0; i < static_కొలత; ++i) {
                        సమాసం_Str += " (select " + varp->name() + " #x";
                        కొరకు (పూర్ణాంకం జరుపు_2 = 28; జరుపు_2 >= 0; జరుపు_2 -= 4) {
                            సమాసం_Str += "0123456789abcdef"[(i >> జరుపు_2) & 0xf];
                        }
                        సమాసం_Str += ')';
                    }
                    smtExprs.emplace_వెనుక(సమాసం_Str + "))");
                    కొనసాగు;
                }

                Ast_కణుపు_సమాసం* స్థిరరాశి namep = కొత్త AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* స్థిరరాశి randUniqueCallp
                    = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->కనుగొను_Void_DType());
                randUniqueCallp->కూడు_Pinsp(కొత్త AstConst{fl, *m_uniqueConstraintId});
                అమరిక_Stmtsp = Ast_కణుపు::కూడు_తదుపరి(అమరిక_Stmtsp, కొత్త Ast_Stmt_సమాసం{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        ఒకవేళ (m_wantSingle && !smtExprs.ఖాళీలు()) {
            std::string సమాసం_Str = smtExprs.front();
            ఒకవేళ (smtExprs.కొలత() > 1) {
                సమాసం_Str = "(bvand";
                కొరకు (స్థిరరాశి std::string& oneExprp : smtExprs) సమాసం_Str += " " + oneExprp;
                సమాసం_Str += ')';
            }
            AstSFormatF* స్థిరరాశి newp = కొత్త AstSFormatF{fl, సమాసం_Str, అబద్ధం, శూన్యసూచిక};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        ఒకవేళ (అమరిక_Stmtsp) nodep->కూడు_ఇక్కడ_This_As_తదుపరి(అమరిక_Stmtsp);
        nodep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(Ast_Constraint_సమాసం* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_మొదటి_వ్యక్తీకరణ_Inside_Indexp);
        ఒకవేళ (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->exprp();
            ఒకవేళ (exprp->వెడల్పు() > 1) {
                File_పంక్తి* స్థిరరాశి fl = exprp->fileline();
                స్థిరరాశి V3_సంఖ్య అంకె_Zero{fl, exprp->వెడల్పు(), 0};
                Ast_కణుపు_సమాసం* స్థిరరాశి neqp
                    = కొత్త AstNeq{fl, exprp->unlink_Fr_వెనుక(), కొత్త AstConst{fl, అంకె_Zero}};
                neqp->user1(నిజం);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        ఒకవేళ (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        స్థిరరాశి VCMethod పద్ధతి
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
            nodep->fileline(),
            కొత్త AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), కణుపు_Module), m_genp,
                          VAccess::READWRITE},
            పద్ధతి, nodep->exprp()->unlink_Fr_వెనుక()};
        callp->dtype_సమితి_Void();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->కూడు_Pinsp(కొత్త AstCExpr{nodep->fileline(), AstCExpr::శుద్ధ{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        స్థిరరాశి uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->కూడు_Pinsp(కొత్త AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string pretty_పాఠ్యం;
        ఒకవేళ (!v3Global.opt.protectIds()) {
            pretty_పాఠ్యం = nodep->fileline()->pretty_ఆధారం();
            కొలత_t pos = 0;
            అయితే ((pos = pretty_పాఠ్యం.కనుగొను('"', pos)) != std::string::npos) {
                pretty_పాఠ్యం.చొప్పించు(pos, "\\");
                pos += std::అక్షరశ్రేణి_పొడవు("\\\"");
            }
        }
        callp->కూడు_Pinsp(
            కొత్త AstCExpr{nodep->fileline(), AstCExpr::శుద్ధ{}, "\"" + pretty_పాఠ్యం + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();

        ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_AT) {
            కూడు_String_Name_Part(nodep->pinsp(), "%x");
            స్థిరరాశి తార్కిక సూచీ_Is_Rand = nodep->pinsp()->exists([](స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి vrefp) {
                తిరిగి vrefp->varp()->rand().isRandomizable();
            });
            ఒకవేళ (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    స్థిరరాశి Ast_కణుపు* స్థిరరాశి basep = nodep->పునాది_Fromp(నిజం);
                    ఒకవేళ (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        తిరిగి;
                    }
                }
                Ast_కణుపు_సమాసం* స్థిరరాశి origp = సూచీ_Is_Rand ? శూన్యసూచిక : nodep->clone_వృక్షం(అబద్ధం);
                స్థిరరాశి AstVarRef* స్థిరరాశి పునాది_Refp
                    = m_structSel ? VN_CAST(nodep->fromp()->పునాది_Fromp(నిజం), VarRef) : శూన్యసూచిక;
                ఒకవేళ (పునాది_Refp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(పునాది_Refp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి indexp = nodep->pinsp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    Ast_కణుపు_సమాసం* స్థిరరాశి arrayp = nodep->fromp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    AstCMethodHard* స్థిరరాశి currentSizep
                        = కొత్త AstCMethodHard{fl, arrayp->clone_వృక్షం_శుద్ధ(అబద్ధం), VCMethod::DYN_కొలత};
                    currentSizep->dtype_సమితి_Int();
                    Ast_కణుపు_సమాసం* స్థిరరాశి లోపల_Range_Requiredp = కొత్త AstLogAnd{
                        fl,
                        కొత్త AstLteS{fl,
                                    కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, indexp->వెడల్పు(), 0},
                                    indexp->clone_వృక్షం_శుద్ధ(అబద్ధం)},
                        కొత్త AstGteS{fl, indexp->clone_వృక్షం_శుద్ధ(అబద్ధం), currentSizep}};
                    AstCMethodHard* స్థిరరాశి resizep
                        = కొత్త AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             కొత్త Ast_కూడు{fl, indexp, కొత్త AstConst{fl, 1}}};
                    resizep->dtype_సమితి_Void();
                    m_prepareConstrainedArraysp->కూడు_Stmtsp(
                        కొత్త AstIf{fl, లోపల_Range_Requiredp, resizep->makeStmt()});
                }
                AstCMethodHard* స్థిరరాశి sizep
                    = m_structSel ? కొత్త AstCMethodHard{fl, nodep->fromp()->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                       VCMethod::DYN_కొలత}
                                  : శూన్యసూచిక;
                Ast_కణుపు_సమాసం* స్థిరరాశి originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_కణుపు_సమాసం* స్థిరరాశి pinp = nodep->pinsp()->unlink_Fr_వెనుక();
                AstSFormatF* newp;
                ఒకవేళ (m_structSel) {
                    ఒకవేళ (AstSFormatF* స్థిరరాశి sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_కణుపు_సమాసం* స్థిరరాశి argsp
                        = Ast_కణుపు::కూడు_తదుపరి(nodep->fromp()->unlink_Fr_వెనుక(), pinp);
                    sizep->dtype_సమితి_Int();
                    AstLogAnd* స్థిరరాశి condp
                        = కొత్త AstLogAnd{fl,
                                        కొత్త AstLteS{fl,
                                                    కొత్త AstConst{fl, AstConst::Widthed_విలువ{},
                                                                 originalPinp->వెడల్పు(), 0},
                                                    originalPinp->clone_వృక్షం_శుద్ధ(అబద్ధం)},
                                        కొత్త AstLtS{fl, originalPinp->clone_వృక్షం_శుద్ధ(అబద్ధం), sizep}};
                    m_conditionp = m_conditionp ? కొత్త AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = కొత్త AstSFormatF{fl, "%s.%s", అబద్ధం, argsp};
                } లేకపోతే {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_వెనుక(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                ఒకవేళ (origp && !hoist_Rand_బహుళకం_Over_Select_And_Member(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_వృక్షం(), origp);
                }

            } లేకపోతే ఒకవేళ (సూచీ_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } లేకపోతే {
                nodep->user1(అబద్ధం);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            తిరిగి;
        }

        ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_INSIDE) {
            స్థిరరాశి తార్కిక randArr = nodep->fromp()->user1();

            AstVar* స్థిరరాశి newVarp
                = కొత్త AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->కనుగొను_Int_DType()};
            Ast_కణుపు_సమాసం* స్థిరరాశి idxRefp = కొత్త AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            Ast_Foreach_శీర్షిక* స్థిరరాశి headerp
                = కొత్త Ast_Foreach_శీర్షిక{fl, nodep->fromp()->clone_వృక్షం_శుద్ధ(అబద్ధం), newVarp};
            Ast_కణుపు_సమాసం* స్థిరరాశి selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_కణుపు* స్థిరరాశి itemp = కొత్త AstEq{fl, selp, nodep->pinsp()->unlink_Fr_వెనుక()};
            itemp->user1(నిజం);

            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{fl};
            cstmtp->కూడు("ret += \" \";\n");
            cstmtp->కూడు("ret += ");
            cstmtp->కూడు(iterateSubtreeReturnEdits(itemp));
            cstmtp->కూడు(";");
            AstCExpr* స్థిరరాశి cexprp = కొత్త AstCExpr{fl};
            cexprp->dtype_సమితి_String();
            cexprp->కూడు("([&]{\nstd::string ret;\n");
            cexprp->కూడు(కొత్త AstBegin{fl, m_unique_పేర్లు.get("__VrandBlock"),
                                     కొత్త AstForeach{fl, headerp, cstmtp}, నిజం});
            cexprp->కూడు("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(కొత్త AstSFormatF{fl, "%s", అబద్ధం, cexprp});
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        }

        // Array reduction methods (sum, product, and, or, xor)
        ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_కూడిక
            || nodep->పద్ధతి() == VCMethod::ARRAY_R_లబ్ధం
            || nodep->పద్ధతి() == VCMethod::ARRAY_R_AND || nodep->పద్ధతి() == VCMethod::ARRAY_R_OR
            || nodep->పద్ధతి() == VCMethod::ARRAY_R_XOR) {

            AstWith* స్థిరరాశి withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            ఒకవేళ (withp) {
                ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    స్థిరరాశి పూర్ణాంకం array_కొలత_2 = adtypep->elementsConst();
                    ఒకవేళ (array_కొలత_2 > v3Global.opt.constraint_Array_హద్దు()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvt_గమ్యం_Str(array_కొలత_2)
                                          + " exceeds --constraint-array-limit of "
                                          + cvt_గమ్యం_Str(v3Global.opt.constraint_Array_హద్దు())
                                          + "), treating as state");
                        nodep->user1(అబద్ధం);
                        ఒకవేళ (editFormat(nodep)) తిరిగి;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        తిరిగి;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            స్థిరరాశి తార్కిక hasRandomArray = nodep->fromp()->user1();
            ఒకవేళ (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(అబద్ధం);
                ఒకవేళ (editFormat(nodep)) తిరిగి;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                తిరిగి;
            }
            ఒకవేళ (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                తిరిగి;
            }

            // Create loop variable and header
            AstVar* స్థిరరాశి loopVarp
                = కొత్త AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->కనుగొను_Int_DType()};
            Ast_Foreach_శీర్షిక* స్థిరరాశి headerp
                = కొత్త Ast_Foreach_శీర్షిక{fl, nodep->fromp()->clone_వృక్షం_శుద్ధ(అబద్ధం), loopVarp};

            // Determine SMT operation and compute identity elements
            స్థిరరాశి అక్షరం* smtOp = శూన్యసూచిక;
            std::string తటస్థం;
            ఒకవేళ (withp) {
                // For 'with' clause: use binary format, compute from result width
                స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->dtypep()->వెడల్పు();
                ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_కూడిక) {
                    smtOp = "bvadd";
                    తటస్థం = "#b" + std::string(వెడల్పు, '0');
                } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_లబ్ధం) {
                    smtOp = "bvmul";
                    తటస్థం = (వెడల్పు > 0) ? "#b" + std::string(వెడల్పు - 1, '0') + "1" : "#b0";
                } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    తటస్థం = "#b" + std::string(వెడల్పు, '1');
                } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    తటస్థం = "#b" + std::string(వెడల్పు, '0');
                } లేకపోతే {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    తటస్థం = "#b" + std::string(వెడల్పు, '0');
                }
            } లేకపోతే {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* స్థిరరాశి arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* స్థిరరాశి arrVarp = arrRefp->varp();
                Ast_కణుపు_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                స్థిరరాశి పూర్ణాంకం elem_వెడల్పు = elemDtp->వెడల్పు();
                స్థిరరాశి పూర్ణాంకం hexDigits = (elem_వెడల్పు + 3) / 4;

                స్థిరరాశి std::string zero_తటస్థం = "#x" + std::string(hexDigits, '0');
                స్థిరరాశి std::string one_తటస్థం = "#x" + std::string(hexDigits - 1, '0') + "1";
                స్థిరరాశి std::string all_Ones_తటస్థం = "#x" + std::string(hexDigits, 'f');

                ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_కూడిక) {
                    smtOp = "bvadd";
                    తటస్థం = zero_తటస్థం;
                } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_లబ్ధం) {
                    smtOp = "bvmul";
                    తటస్థం = one_తటస్థం;
                } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    తటస్థం = all_Ones_తటస్థం;
                } లేకపోతే ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    తటస్థం = zero_తటస్థం;
                } లేకపోతే {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    తటస్థం = zero_తటస్థం;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* స్థిరరాశి cstmtp = కొత్త AstCStmt{fl};
            ఒకవేళ (withp) {
                // With 'with' clause: evaluate expression for each element
                స్థిరరాశి తార్కిక randArr = nodep->fromp()->user1();
                Ast_కణుపు_సమాసం* స్థిరరాశి idxRefp = కొత్త AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_కణుపు_సమాసం* స్థిరరాశి elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                స్థిరరాశి పూర్ణాంకం ఫలితం_వెడల్పు = nodep->dtypep()->వెడల్పు();
                స్థిరరాశి VSigning ఫలితం_Signing = nodep->dtypep()->numeric();

                Ast_కణుపు* చొప్పున_Elem_Exprp = withp->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
                ఒకవేళ (AstLambdaArgRef* స్థిరరాశి మూలం_Refp = VN_CAST(చొప్పున_Elem_Exprp, LambdaArgRef)) {
                    ఒకవేళ (మూలం_Refp->సూచీ()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* స్థిరరాశి loopRef = కొత్త AstVarRef{fl, loopVarp, VAccess::READ};
                        చొప్పున_Elem_Exprp = adjust_వెడల్పు(fl, loopRef, ఫలితం_వెడల్పు, ఫలితం_Signing);
                    } లేకపోతే {
                        // item at root -> replace with array element
                        చొప్పున_Elem_Exprp = elemSelp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    }
                    VL_DO_DANGLING(మూలం_Refp->delete_వృక్షం(), మూలం_Refp);
                } లేకపోతే {
                    చొప్పున_Elem_Exprp->foreach([&](AstLambdaArgRef* refp) {
                        ఒకవేళ (refp->సూచీ()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* స్థిరరాశి loopRef = కొత్త AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_కణుపు* స్థిరరాశి replacement
                                = adjust_వెడల్పు(fl, loopRef, ఫలితం_వెడల్పు, ఫలితం_Signing);
                            refp->replaceWith(replacement);
                        } లేకపోతే {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                        }
                        VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_వృక్షం(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                చొప్పున_Elem_Exprp = V3Const::constifyEdit(చొప్పున_Elem_Exprp);

                చొప్పున_Elem_Exprp->foreach([&](Ast_కణుపు* nodep) {
                    // Don't mark loop variable references as randomizable
                    ఒకవేళ (!VN_IS(nodep, Const)) {
                        ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి vrefp = VN_CAST(nodep, కణుపు_Var_Ref)) {
                            ఒకవేళ (vrefp->varp() != loopVarp) nodep->user1(నిజం);
                        } లేకపోతే {
                            nodep->user1(నిజం);
                        }
                    }
                });

                cstmtp->కూడు("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->కూడు("ret += ");
                cstmtp->కూడు(iterateSubtreeReturnEdits(చొప్పున_Elem_Exprp));
                cstmtp->కూడు(";\n");
                cstmtp->కూడు("ret += \")\";\n");
            } లేకపోతే {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* స్థిరరాశి arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* స్థిరరాశి arrVarp = arrRefp->varp();
                స్థిరరాశి std::string smtArrayName = arrVarp->name();
                Ast_కణుపు_DType* స్థిరరాశి elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                స్థిరరాశి పూర్ణాంకం elem_వెడల్పు = elemDtp->వెడల్పు();
                Ast_కణుపు_Module* స్థిరరాశి arrModulep = VN_AS(arrVarp->user2p(), కణుపు_Module);
                Ast_కణుపు_Module* స్థిరరాశి genModulep = VN_AS(m_genp->user2p(), కణుపు_Module);
                arrVarp->user3(నిజం);

                // Add write_var call to init task
                Ast_కణుపు_సమాసం* స్థిరరాశి arrVarNamep
                    = కొత్త AstCExpr{fl, AstCExpr::శుద్ధ{}, "\"" + smtArrayName + "\"", elem_వెడల్పు};
                AstCMethodHard* స్థిరరాశి writeVarCallp
                    = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->కూడు_Pinsp(కొత్త AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->కూడు_Pinsp(
                    కొత్త AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elem_వెడల్పు)});
                writeVarCallp->కూడు_Pinsp(arrVarNamep);
                స్థిరరాశి uint32_t unpackedDims = arrVarp->dtypep()->dimensions(అబద్ధం).రెండవ;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->కూడు_Pinsp(కొత్త AstConst{fl, 1});  // Dimension

                స్థిరరాశి Randomize_బహుళకం rand_బహుళకం = {.asUQuad = arrVarp->user1()};
                ఒకవేళ (rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం) {
                    writeVarCallp->కూడు_Pinsp(
                        కొత్త AstConst{fl, AstConst::Unsized64{}, rand_బహుళకం.సూచీ});
                }
                writeVarCallp->dtype_సమితి_Void();
                Ast_కణుపు_FTask* initTaskp = m_inlineInitTaskp;
                ఒకవేళ (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.కనుగొను_Member(arrModulep, "new"), కణుపు_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->కూడు_Stmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* స్థిరరాశి idxFormatp = కొత్త AstSFormatF{
                    fl, "#x%08x", అబద్ధం, కొత్త AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* స్థిరరాశి varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_వెనుక(), idxFormatp);

                cstmtp->కూడు("ret += \" \";\n");
                cstmtp->కూడు("ret += ");
                cstmtp->కూడు(varNamep);
                cstmtp->కూడు(";\n");
            }

            // Build final expression
            ఒకవేళ (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* స్థిరరాశి cexprp = కొత్త AstCExpr{fl};
                cexprp->dtype_సమితి_String();
                cexprp->కూడు("([&]{\nstd::string ret = \"" + తటస్థం + "\";\n");
                cexprp->కూడు(కొత్త AstBegin{fl, m_unique_పేర్లు.get("__VrandBlock"),
                                         కొత్త AstForeach{fl, headerp, cstmtp}, నిజం});
                cexprp->కూడు("return ret;\n})()");
                nodep->replaceWith(కొత్త AstSFormatF{fl, "%s", అబద్ధం, cexprp});
            } లేకపోతే {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* స్థిరరాశి cexprp = కొత్త AstCExpr{fl};
                cexprp->dtype_సమితి_String();
                cexprp->కూడు("([&]{\nstd::string ret;\n");
                cexprp->కూడు(కొత్త AstBegin{fl, m_unique_పేర్లు.get("__VrandBlock"),
                                         కొత్త AstForeach{fl, headerp, cstmtp}, నిజం});
                cexprp->కూడు("return ret.empty() ? \"" + తటస్థం + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(కొత్త AstSFormatF{fl, "%s", అబద్ధం, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        }

        // We've handled DYN_SIZE already if they should be randomized
        ఒకవేళ (nodep->పద్ధతి() == VCMethod::DYN_కొలత || nodep->పద్ధతి() == VCMethod::ASSOC_కొలత) {
            Ast_కణుపు_సమాసం* కణుపు_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            అయితే (VN_IS(కణుపు_Exprp, MemberSel)) {
                కణుపు_Exprp = VN_AS(కణుపు_Exprp, MemberSel)->fromp();
            }
            ఒకవేళ (VN_IS(కణుపు_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_వృక్షం_శుద్ధ(అబద్ధం)));
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                తిరిగి;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(అబద్ధం);

        ఒకవేళ (editFormat(nodep)) తిరిగి;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;

        AstFunc* స్థిరరాశి funcp = VN_CAST(nodep->taskp(), Func);
        ఒకవేళ (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            తిరిగి;
        }

        Ast_కణుపు_సమాసం* స్థిరరాశి retExprp = వెలికితీయి_Return_సమాసం(funcp);
        ఒకవేళ (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(అబద్ధం);
            ఒకవేళ (editFormat(nodep)) తిరిగి;
            తిరిగి;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        స్థిరరాశి V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), శూన్యసూచిక, అబద్ధం);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_కణుపు_సమాసం* inlinedp = retExprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            స్థిరరాశి AstVar* స్థిరరాశి portp = tconnect.మొదటి;
            AstArg* స్థిరరాశి argp = tconnect.రెండవ;
            ఒకవేళ (!argp || !argp->exprp()) కొనసాగు;
            ఒకవేళ (AstVarRef* స్థిరరాశి మూలం_Refp = VN_CAST(inlinedp, VarRef)) {
                ఒకవేళ (మూలం_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    VL_DO_DANGLING(మూలం_Refp->delete_వృక్షం(), మూలం_Refp);
                    కొనసాగు;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                ఒకవేళ (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం));
                    VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {}
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        ఒకవేళ (editFormat(nodep)) తిరిగి;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_కణుపు* buildDisableSoftCallStmt(Ast_Constraint_సమాసం* nodep) {
        std::string varName;
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి vrefp = VN_CAST(nodep->exprp(), కణుపు_Var_Ref)) {
            varName = vrefp->name();
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } లేకపోతే {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
            nodep->fileline(),
            కొత్త AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), కణుపు_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            కొత్త AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtype_సమితి_Void();
        తిరిగి callp->makeStmt();
    }

    // Pre-pass: for every AstConstraintExpr(isDisableSoft) nested inside an
    // AstConstraintIf subtree (any depth), return a list of runtime AstIf
    // statements `if (accumulated_cond) randomizer.disable_soft(var);` where
    // accumulated_cond AND-folds the enclosing then/else branch conditions.
    // The original ConstraintExpr is replaced with `1'b1` so editSingle()
    // folds the surrounding boolean cleanly.  Caller appends the returned
    // list to the constraint-items chain; that chain becomes the setup task
    // body, which runs before the solver on every randomize() call -- giving
    // disable-soft real conditional semantics per IEEE 1800-2023 18.5.13.
    Ast_కణుపు* వెలికితీయి_Conditional_Disable_Softs(Ast_కణుపు* itemsp, Ast_కణుపు_సమాసం* outerCondp) {
        Ast_కణుపు* hoistListp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ఒకవేళ (AstConstraintIf* స్థిరరాశి ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_పంక్తి* స్థిరరాశి fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_కణుపు_సమాసం* స్థిరరాశి thenCondp
                    = outerCondp ? static_cast<Ast_కణుపు_సమాసం*>(
                                       కొత్త AstLogAnd{fl, outerCondp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                     ifp->condp()->clone_వృక్షం_శుద్ధ(అబద్ధం)})
                                 : ifp->condp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
                ఒకవేళ (Ast_కణుపు* స్థిరరాశి thenHoistp
                    = వెలికితీయి_Conditional_Disable_Softs(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_కణుపు::కూడు_తదుపరి(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_వృక్షం(), thenCondp);
                ఒకవేళ (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_కణుపు_సమాసం* స్థిరరాశి notInnerp
                        = కొత్త AstLogNot{fl, ifp->condp()->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    Ast_కణుపు_సమాసం* స్థిరరాశి elseCondp
                        = outerCondp ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstLogAnd{
                                           fl, outerCondp->clone_వృక్షం_శుద్ధ(అబద్ధం), notInnerp})
                                     : notInnerp;
                    ఒకవేళ (Ast_కణుపు* స్థిరరాశి elseHoistp
                        = వెలికితీయి_Conditional_Disable_Softs(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_కణుపు::కూడు_తదుపరి(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_వృక్షం(), elseCondp);
                }
            } లేకపోతే ఒకవేళ (Ast_Constraint_సమాసం* స్థిరరాశి exprp = VN_CAST(stmtp, Constraint_సమాసం)) {
                ఒకవేళ (exprp->isDisableSoft() && outerCondp) {
                    File_పంక్తి* స్థిరరాశి fl = exprp->fileline();
                    AstIf* స్థిరరాశి hoistIfp = కొత్త AstIf{fl, outerCondp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                      buildDisableSoftCallStmt(exprp), శూన్యసూచిక};
                    hoistListp = hoistListp ? Ast_కణుపు::కూడు_తదుపరి(hoistListp,
                                                               static_cast<Ast_కణుపు*>(hoistIfp))
                                            : static_cast<Ast_కణుపు*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_సమాసం* స్థిరరాశి truep
                        = కొత్త Ast_Constraint_సమాసం{fl, కొత్త AstConst{fl, AstConst::బిట్టు_True{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_వృక్షం(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        తిరిగి hoistListp;
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన Constraint_సమాసం_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_కణుపు* nodep,
                                   Ast_కణుపు_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* rand_బహుళకం_Varp, std::సమితి<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3_Unique_పేర్లు& unique_పేర్లు,
                                   Ast_కణుపు_FTask* memberselInitTaskp = శూన్యసూచిక,
                                   VInsertion_సమితి<AstVar*>* కొలత_Constrained_Arraysp = శూన్యసూచిక,
                                   Ast_కణుపు_FTask* prepareConstrainedArraysp = శూన్యసూచిక)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_rand_బహుళకం_Varp{rand_బహుళకం_Varp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_కొలత_Constrained_Arraysp{కొలత_Constrained_Arraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_unique_పేర్లు{unique_పేర్లు} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి hoistListp
            = nodep ? వెలికితీయి_Conditional_Disable_Softs(nodep, శూన్యసూచిక) : శూన్యసూచిక) {
            nodep->కూడు_తదుపరి(hoistListp);
        }
        iterate_And_తదుపరి_Null(nodep);
    }
};

గణన తరగతి Capture_బహుళకం : uint8_t {
    CAP_NO = 0x0,
    CAP_విలువ = 0x01,
    CAP_THIS = 0x02,
    CAP_F_సమితి_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
Capture_బహుళకం క్రియాకారకం|(Capture_బహుళకం a, Capture_బహుళకం b) {
    తిరిగి static_cast<Capture_బహుళకం>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
Capture_బహుళకం క్రియాకారకం&(Capture_బహుళకం a, Capture_బహుళకం b) {
    తిరిగి static_cast<Capture_బహుళకం>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
Capture_బహుళకం బహుళకం(Capture_బహుళకం a) { తిరిగి a & static_cast<Capture_బహుళకం>(0x3); }
తార్కిక hasFlags(Capture_బహుళకం a, Capture_బహుళకం flags) {
    తిరిగి ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

తరగతి CaptureVisitor final : బహిరంగ VNVisitor {
    AstArg* m_argsp = శూన్యసూచిక;  // Original references turned into arguments
    Ast_కణుపు_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<స్థిరరాశి AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::సమితి<Ast_కణుపు*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = శూన్యసూచిక;  // Variable for outer context's object, if necessary
    తార్కిక m_staticContext = అబద్ధం;  // True when capturing from a static function context

    // METHODS

    తార్కిక captureVariable(File_పంక్తి* స్థిరరాశి fileline, Ast_కణుపు_Var_Ref* varrefp, AstVar*& varp) {
        స్వయంచాలక it = m_varCloneMap.కనుగొను(varrefp->varp());
        ఒకవేళ (it == m_varCloneMap.చివర()) {
            AstVar* స్థిరరాశి newVarp = varrefp->varp()->clone_వృక్షం(అబద్ధం);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(నిజం);
            newVarp->direction(VDirection::ఆదానం);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            తిరిగి నిజం;
        }
        varp = it->రెండవ;
        తిరిగి అబద్ధం;
    }

    మూస <రకం_పేరు T_కణుపు>
    శూన్య fixupClassOrPackage(Ast_కణుపు* memberp, T_కణుపు refp) {
        Ast_కణుపు_Module* స్థిరరాశి declClassp = VN_AS(memberp->user2p(), కణుపు_Module);
        ఒకవేళ (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    మూస <రకం_పేరు T_కణుపు>
    తార్కిక is_Reference_గమ్యం_Inner_Member(T_కణుపు nodep) {
        తిరిగి VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_పంక్తి* fl) {
        ఒకవేళ (!m_thisp) {
            AstClassRefDType* స్థిరరాశి refDTypep
                = కొత్త AstClassRefDType{fl, VN_AS(m_callerp, Class), శూన్యసూచిక};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(refDTypep);
            m_thisp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(నిజం);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::ఆదానం);
            m_argsp = Ast_కణుపు::కూడు_తదుపరి(m_argsp, కొత్త AstArg{fl, "", కొత్త AstThisRef{fl, refDTypep}});
        }
        తిరిగి m_thisp;
    }

    AstVar* getVar(AstVar* స్థిరరాశి varp) స్థిరరాశి {
        స్థిరరాశి స్వయంచాలక it = m_varCloneMap.కనుగొను(varp);
        ఒకవేళ (it == m_varCloneMap.చివర()) తిరిగి శూన్యసూచిక;
        ఒకవేళ (it->రెండవ->isStdRandomizeArg()) తిరిగి శూన్యసూచిక;
        తిరిగి it->రెండవ;
    }

    Capture_బహుళకం get_Var_Ref_Capture_బహుళకం(Ast_కణుపు_Var_Ref* varRefp) {
        Ast_కణుపు_Module* స్థిరరాశి varModp = VN_AS(varRefp->varp()->user2p(), కణుపు_Module);
        AstClass* స్థిరరాశి varClassp = VN_CAST(varModp, Class);
        AstClass* స్థిరరాశి callerClassp = VN_CAST(m_callerp, Class);

        స్థిరరాశి తార్కిక callerIsClass = callerClassp;
        స్థిరరాశి తార్కిక refIsXref = VN_IS(varRefp, VarXRef);
        స్థిరరాశి తార్కిక varIsFuncLocal = varRefp->varp()->isFuncLocal();
        స్థిరరాశి తార్కిక varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        స్థిరరాశి తార్కిక var_Is_క్షేత్రం_యొక్క_Caller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        స్థిరరాశి తార్కిక varIsParam = varRefp->varp()->isParam();
        స్థిరరాశి తార్కిక varIsConstraintIterator
            = VN_IS(varRefp->varp()->మొదటి_Abovep(), Foreach_శీర్షిక)
              && VN_IS(varRefp->varp()->మొదటి_Abovep()->మొదటి_Abovep(), ConstraintForeach);
        ఒకవేళ (refIsXref) తిరిగి Capture_బహుళకం::CAP_విలువ | Capture_బహుళకం::CAP_F_XREF;
        ఒకవేళ (varIsConstraintIterator) తిరిగి Capture_బహుళకం::CAP_NO;
        ఒకవేళ (varIsFuncLocal && varHasAutomaticLifetime) తిరిగి Capture_బహుళకం::CAP_విలువ;
        ఒకవేళ (varIsParam) తిరిగి Capture_బహుళకం::CAP_విలువ;
        // Static var in function (will not be inlined, because it's in class)
        ఒకవేళ (callerIsClass && varIsFuncLocal) తిరిగి Capture_బహుళకం::CAP_విలువ;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        ఒకవేళ (m_staticContext && callerIsClass && var_Is_క్షేత్రం_యొక్క_Caller
            && varRefp->varp()->lifetime().isStatic())
            తిరిగి Capture_బహుళకం::CAP_NO;
        ఒకవేళ (callerIsClass && var_Is_క్షేత్రం_యొక్క_Caller) తిరిగి Capture_బహుళకం::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        ఒకవేళ (callerIsClass && varClassp && !var_Is_క్షేత్రం_యొక్క_Caller
            && varRefp->varp()->lifetime().isStatic())
            తిరిగి Capture_బహుళకం::CAP_విలువ;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        తిరిగి Capture_బహుళకం::CAP_విలువ;
    }

    శూన్య capture_Ref_By_విలువ(Ast_కణుపు_Var_Ref* nodep, Capture_బహుళకం cap_బహుళకం_Flags) {
        AstVar* newVarp;
        స్థిరరాశి తార్కిక newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_కణుపు_Var_Ref* స్థిరరాశి newVarRefp = newCapture ? nodep->clone_వృక్షం(అబద్ధం) : శూన్యసూచిక;
        ఒకవేళ (!hasFlags(cap_బహుళకం_Flags, Capture_బహుళకం::CAP_F_సమితి_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(శూన్యసూచిక);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        ఒకవేళ (!newCapture) తిరిగి;
        ఒకవేళ (hasFlags(cap_బహుళకం_Flags, Capture_బహుళకం::CAP_F_XREF)) {
            AstVarRef* స్థిరరాశి notXVarRefp
                = కొత్త AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_కణుపు::కూడు_తదుపరి(m_argsp, కొత్త AstArg{nodep->fileline(), "", newVarRefp});
    }

    శూన్య captureRefByThis(Ast_కణుపు_Var_Ref* nodep, Capture_బహుళకం /*capModeFlags*/) {
        AstVar* స్థిరరాశి thisp = importThisp(nodep->fileline());
        AstVarRef* స్థిరరాశి thisRefp = కొత్త AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(నిజం);
        m_ignore.emplace(thisRefp);
        AstMemberSel* స్థిరరాశి memberSelp
            = కొత్త AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        ఒకవేళ (!m_targetp && nodep->user1()) memberSelp->user1(నిజం);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (m_ignore.లెక్క(nodep)) తిరిగి;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        ఒకవేళ (nodep->varp()->isStdRandomizeArg()) తిరిగి;
        స్థిరరాశి Capture_బహుళకం cap_బహుళకం = get_Var_Ref_Capture_బహుళకం(nodep);
        ఒకవేళ (బహుళకం(cap_బహుళకం) == Capture_బహుళకం::CAP_NO) తిరిగి;
        ఒకవేళ (బహుళకం(cap_బహుళకం) == Capture_బహుళకం::CAP_విలువ) capture_Ref_By_విలువ(nodep, cap_బహుళకం);
        ఒకవేళ (బహుళకం(cap_బహుళకం) == Capture_బహుళకం::CAP_THIS) captureRefByThis(nodep, cap_బహుళకం);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (m_ignore.లెక్క(nodep)) {
            iterateChildren(nodep);
            తిరిగి;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        ఒకవేళ (VN_IS(nodep, పద్ధతి_Call) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            తిరిగి;
        }
        AstClass* స్థిరరాశి classp = VN_CAST(nodep->taskp()->user2p(), Class);
        ఒకవేళ ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* స్థిరరాశి argsp = nodep->argsp();
            ఒకవేళ (argsp) argsp->unlink_Fr_వెనుక();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* స్థిరరాశి thisp = importThisp(nodep->fileline());
            AstVarRef* స్థిరరాశి thisRefp = కొత్త AstVarRef{
                nodep->fileline(), thisp, nodep->is_శుద్ధ() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp
                = కొత్త Ast_పద్ధతి_Call{nodep->fileline(), thisRefp, thisp->name(), argsp};
            పద్ధతి_Callp->taskp(nodep->taskp());
            పద్ధతి_Callp->dtypep(nodep->dtypep());
            nodep->replaceWith(పద్ధతి_Callp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            m_ignore.emplace(పద్ధతి_Callp);
        }
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        ఒకవేళ (!is_Reference_గమ్యం_Inner_Member(nodep)) {
            iterateChildren(nodep);
            తిరిగి;
        }
        AstVarRef* స్థిరరాశి varRefp
            = కొత్త AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    శూన్య సందర్శించు(Ast_పద్ధతి_Call* nodep) override {
        ఒకవేళ (!is_Reference_గమ్యం_Inner_Member(nodep) || m_ignore.లెక్క(nodep)) {
            iterateChildren(nodep);
            తిరిగి;
        }
        AstArg* స్థిరరాశి argsp = nodep->argsp();
        ఒకవేళ (argsp) argsp->unlink_Fr_వెనుక_With_తదుపరి();
        Ast_కణుపు_FTask_Ref* taskRefp = శూన్యసూచిక;
        ఒకవేళ (AstTask* స్థిరరాశి taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = కొత్త AstTaskRef{nodep->fileline(), taskp, argsp};
        } లేకపోతే ఒకవేళ (AstFunc* స్థిరరాశి taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = కొత్త AstFuncRef{nodep->fileline(), taskp, argsp};
        } లేకపోతే {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    స్పష్టమైన CaptureVisitor(Ast_కణుపు* స్థిరరాశి nodep, Ast_కణుపు_Module* callerp, AstClass* స్థిరరాశి targetp,
                            తార్కిక staticContext = అబద్ధం)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterate_And_తదుపరి_Null(nodep);
    }

    // PUBLIC METHODS

    AstArg* get_వాదనలు() స్థిరరాశి { తిరిగి m_argsp; }
    AstVar* getThisp() స్థిరరాశి { తిరిగి m_thisp; }

    శూన్య కూడు_Function_Arguments(Ast_కణుపు_FTask* funcp) స్థిరరాశి {
        కొరకు (AstArg* argp = get_వాదనలు(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ఒకవేళ (Ast_కణుపు_Var_Ref* varrefp = VN_CAST(argp->exprp(), కణుపు_Var_Ref)) {
                ఒకవేళ ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(శూన్యసూచిక);
                }
                funcp->కూడు_Stmtsp(getVar(varrefp->varp()));
            } లేకపోతే {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->కూడు_Stmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

తరగతి RandomizeVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared on Netlist
    //  AstClass::user1()       -> bool.  Set true to indicate needs randomize processing
    //  AstVar::user2p()        -> AstNodeModule*. Pointer to containing module
    //  AstNodeFTask::user2p()  -> AstNodeModule*. Pointer to containing module
    //  AstEnumDType::user2()   -> AstVarRef*. Reference to table with enum values, to clone
    //  AstConstraint::user2p() -> AstTask*. Pointer to constraint setup procedure
    //  AstClass::user2p()      -> AstVar*.  Rand mode state variable
    //  AstVar::user3()         -> bool. Handled in constraints
    //  AstClass::user3p()      -> AstVar*.  Constrained randomizer variable
    //  AstConstraint::user3p() -> AstTask*. Pointer to resize procedure
    //  AstConstraintForeach::user3p() -> AstNode*. Dist bucket preamble stmts (foreach case)
    //  AstClass::user4p()      -> AstVar*.  Constraint mode state variable
    //  AstVar::user4p()        -> AstVar*.  Size variable for constrained queues
    //  AstNodeFTaskRef::user4() -> bool.  Processed
    //  AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNUser1InUse    m_inuser1;      (Allocated for use in RandomizeMarkVisitor)
    // VNUser2InUse    m_inuser2;      (Allocated for use in RandomizeMarkVisitor)
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;
    స్థిరరాశి VNUser4_లోపల_Use m_inuser4;

    // STATE
    V3_Unique_పేర్లు m_inline_Unique_పేర్లు;  // For generating unique function names
    V3_Unique_పేర్లు m_rand_Array_Unique_పేర్లు{"__Vrandarr"};  // Random array loop indices
    V3_Unique_పేర్లు m_బహుళకం_Unique_పేర్లు{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3_Unique_పేర్లు m_inlineUniqueStdName{"__VStdrand"};
    V3_Unique_పేర్లు m_unique_పేర్లు;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    std::unordered_map<Ast_కణుపు_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    Ast_కణుపు_Stmt* m_stmtp = శూన్యసూచిక;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = శూన్యసూచిక;  // Dynamic array type (for rand mode)
    పూర్ణాంకం m_rand_Case_అంకె = 0;  // Randcase number within a module for var naming
    పూర్ణాంకం m_dist_అంకె = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = శూన్యసూచిక;  // Current constraint
    std::సమితి<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertion_సమితి<AstVar*>> m_కొలత_Constrained_Arrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_static_Constraint_బహుళకం_Vars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_static_Rand_బహుళకం_Vars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<తార్కిక, తార్కిక>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    స్థిర తార్కిక is_Similar_కణుపు(స్థిరరాశి Ast_కణుపు_సమాసం* with_సమాసం, స్థిరరాశి Ast_కణుపు_సమాసం* arg_సమాసం) {
        // VarRef: compare variable pointers
        ఒకవేళ (VN_IS(arg_సమాసం, VarRef) && VN_IS(with_సమాసం, VarRef)) {
            తిరిగి VN_AS(with_సమాసం, VarRef)->varp() == VN_AS(arg_సమాసం, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        ఒకవేళ (VN_IS(arg_సమాసం, MemberSel) && VN_IS(with_సమాసం, MemberSel)) {
            స్థిరరాశి AstMemberSel* స్థిరరాశి withMSp = VN_AS(with_సమాసం, MemberSel);
            స్థిరరాశి AstMemberSel* స్థిరరాశి argMSp = VN_AS(arg_సమాసం, MemberSel);
            ఒకవేళ (withMSp->varp() != argMSp->varp()) తిరిగి అబద్ధం;
            // Recursively compare the base object expression
            తిరిగి is_Similar_కణుపు(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        ఒకవేళ (VN_IS(arg_సమాసం, ArraySel) && VN_IS(with_సమాసం, ArraySel)) {
            స్థిరరాశి AstArraySel* స్థిరరాశి withASp = VN_AS(with_సమాసం, ArraySel);
            స్థిరరాశి AstArraySel* స్థిరరాశి argASp = VN_AS(arg_సమాసం, ArraySel);
            // Before checking Sel type index, check if both are Const
            ఒకవేళ (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                స్థిరరాశి AstConst* స్థిరరాశి withIdxp = VN_AS(withASp->bitp(), Const);
                స్థిరరాశి AstConst* స్థిరరాశి argIdxp = VN_AS(argASp->bitp(), Const);
                తిరిగి is_Similar_కణుపు(withASp->fromp(), argASp->fromp())
                       && withIdxp->అంకె().isCaseEq(argIdxp->అంకె());
            }
            // Index must be Sel type, extract VarRef using fromp()
            ఒకవేళ (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) తిరిగి అబద్ధం;
            స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            తిరిగి is_Similar_కణుపు(withASp->fromp(), argASp->fromp())
                   && is_Similar_కణుపు(withIdxp, argIdxp);
        }
        తిరిగి అబద్ధం;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    స్థిర శూన్య expandUniqueElementList(Ast_కణుపు* itemsp) {
        Ast_కణుపు* itemp = itemsp;
        అయితే (itemp) {
            Ast_కణుపు* స్థిరరాశి nextp = itemp->nextp();
            ఒకవేళ (స్థిరరాశి AstConstraintIf* స్థిరరాశి ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                కొనసాగు;
            }
            ఒకవేళ (స్థిరరాశి AstConstraintForeach* స్థిరరాశి foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                కొనసాగు;
            }
            AstConstraintUnique* స్థిరరాశి uniquep = VN_CAST(itemp, ConstraintUnique);
            ఒకవేళ (!uniquep) {
                itemp = nextp;
                కొనసాగు;
            }

            std::సదిశ<Ast_కణుపు_సమాసం*> సమాసం_వస్తువులు;
            తార్కిక hasArrayVarRef = అబద్ధం;
            కొరకు (Ast_కణుపు* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = vrp->varp()->dtypep()->skipRefp();
                    ఒకవేళ (VN_IS(dtypep, కణుపు_Array_DType) || VN_IS(dtypep, వరుస_DType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = నిజం;
                        కొనసాగు;
                    }
                }
                సమాసం_వస్తువులు.నెట్టు_వెనుక(VN_AS(rp, కణుపు_సమాసం));
            }

            ఒకవేళ (సమాసం_వస్తువులు.కొలత() >= 2) {
                File_పంక్తి* స్థిరరాశి fl = uniquep->fileline();
                కొరకు (కొలత_t i = 0; i < సమాసం_వస్తువులు.కొలత(); i++) {
                    కొరకు (కొలత_t j = i + 1; j < సమాసం_వస్తువులు.కొలత(); j++) {
                        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = సమాసం_వస్తువులు[i]->clone_వృక్షం(అబద్ధం);
                        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = సమాసం_వస్తువులు[j]->clone_వృక్షం(అబద్ధం);
                        AstNeq* స్థిరరాశి neqp = కొత్త AstNeq{fl, lhsp, rhsp};
                        neqp->user1(నిజం);
                        Ast_Constraint_సమాసం* స్థిరరాశి cexprp = కొత్త Ast_Constraint_సమాసం{fl, neqp};
                        uniquep->కూడు_తదుపరి_ఇక్కడ(cexprp);
                    }
                }
                ఒకవేళ (!hasArrayVarRef) {
                    uniquep->unlink_Fr_వెనుక();
                    VL_DO_DANGLING(uniquep->delete_వృక్షం(), uniquep);
                }
            } లేకపోతే ఒకవేళ (సమాసం_వస్తువులు.కొలత() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlink_Fr_వెనుక();
                VL_DO_DANGLING(uniquep->delete_వృక్షం(), uniquep);
            }
            itemp = nextp;
        }
    }
    శూన్య createRandomGenerator(AstClass* స్థిరరాశి classp) {
        ఒకవేళ (classp->user3p()) తిరిగి;
        ఒకవేళ (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            తిరిగి;
        }
        AstVar* స్థిరరాశి genp = కొత్త AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->కనుగొను_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->కూడు_Membersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_కణుపు_Module* స్థిరరాశి modp) {
        స్వయంచాలక it = m_stdMap.కనుగొను(modp);
        ఒకవేళ (it == m_stdMap.చివర()) {
            AstVar* స్థిరరాశి stdgenp
                = కొత్త AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->కనుగొను_Basic_DType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3_Error_సంకేతం::IMPURE, నిజం);
            modp->కూడు_Stmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            తిరిగి stdgenp;
        }
        తిరిగి it->రెండవ;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* స్థిరరాశి classp) {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి name = "__Vprepare_constrained_arrays";
        AstTask* స్థిరరాశి taskp = కొత్త AstTask{classp->fileline(), name, శూన్యసూచిక};
        taskp->class_పద్ధతి(నిజం);
        classp->కూడు_Membersp(taskp);
        m_memberMap.చొప్పించు(classp, taskp);
        తిరిగి taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* స్థిరరాశి classp) {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.కనుగొను_Member(classp, name), Task);
        ఒకవేళ (resizeTaskp) తిరిగి resizeTaskp;
        resizeTaskp = కొత్త AstTask{classp->fileline(), name, శూన్యసూచిక};
        resizeTaskp->class_పద్ధతి(నిజం);
        resizeTaskp->isVirtual(నిజం);
        classp->కూడు_Membersp(resizeTaskp);
        m_memberMap.చొప్పించు(classp, resizeTaskp);
        తిరిగి resizeTaskp;
    }
    AstVar* get_Create_Rand_బహుళకం_Var(AstClass* స్థిరరాశి classp) {
        ఒకవేళ (classp->user2p()) తిరిగి VN_AS(classp->user2p(), Var);
        ఒకవేళ (AstClassExtends* స్థిరరాశి extendsp = classp->extendsp()) {
            తిరిగి get_Create_Rand_బహుళకం_Var(extendsp->classp());
        }
        AstVar* స్థిరరాశి rand_బహుళకం_Varp = create_బహుళకం_Var(classp, "__Vrandmode");
        classp->user2p(rand_బహుళకం_Varp);
        తిరిగి rand_బహుళకం_Varp;
    }
    AstVar* get_Create_Constraint_బహుళకం_Var(AstClass* స్థిరరాశి classp) {
        ఒకవేళ (classp->user4p()) తిరిగి VN_AS(classp->user4p(), Var);
        ఒకవేళ (AstClassExtends* స్థిరరాశి extendsp = classp->extendsp()) {
            తిరిగి get_Create_Constraint_బహుళకం_Var(extendsp->classp());
        }
        AstVar* స్థిరరాశి constraint_బహుళకం_Varp = create_బహుళకం_Var(classp, "__Vconstraintmode");
        classp->user4p(constraint_బహుళకం_Varp);
        తిరిగి constraint_బహుళకం_Varp;
    }
    స్థిర AstVar* get_Constraint_బహుళకం_Var(AstClass* స్థిరరాశి classp) {
        ఒకవేళ (classp->user4p()) తిరిగి VN_AS(classp->user4p(), Var);
        ఒకవేళ (AstClassExtends* స్థిరరాశి extendsp = classp->extendsp()) {
            తిరిగి get_Constraint_బహుళకం_Var(extendsp->classp());
        }
        తిరిగి శూన్యసూచిక;
    }
    AstVar* get_Create_Static_Constraint_బహుళకం_Var(AstClass* స్థిరరాశి classp) {
        స్వయంచాలక it = m_static_Constraint_బహుళకం_Vars.కనుగొను(classp);
        ఒకవేళ (it != m_static_Constraint_బహుళకం_Vars.చివర()) తిరిగి it->రెండవ;
        ఒకవేళ (AstClassExtends* స్థిరరాశి extendsp = classp->extendsp()) {
            తిరిగి get_Create_Static_Constraint_బహుళకం_Var(extendsp->classp());
        }
        AstVar* స్థిరరాశి static_బహుళకం_Varp = create_Static_బహుళకం_Var(classp, "__Vstaticconstraintmode");
        m_static_Constraint_బహుళకం_Vars[classp] = static_బహుళకం_Varp;
        తిరిగి static_బహుళకం_Varp;
    }
    AstVar* get_Static_Constraint_బహుళకం_Var(AstClass* స్థిరరాశి classp) {
        స్వయంచాలక it = m_static_Constraint_బహుళకం_Vars.కనుగొను(classp);
        ఒకవేళ (it != m_static_Constraint_బహుళకం_Vars.చివర()) తిరిగి it->రెండవ;
        ఒకవేళ (AstClassExtends* స్థిరరాశి extendsp = classp->extendsp()) {
            తిరిగి get_Static_Constraint_బహుళకం_Var(extendsp->classp());
        }
        తిరిగి శూన్యసూచిక;
    }
    AstVar* get_Create_Static_Rand_బహుళకం_Var(AstClass* స్థిరరాశి classp) {
        ఒకవేళ (m_static_Rand_బహుళకం_Vars.లెక్క(classp)) తిరిగి m_static_Rand_బహుళకం_Vars[classp];
        ఒకవేళ (AstClassExtends* స్థిరరాశి extendsp = classp->extendsp()) {
            తిరిగి get_Create_Static_Rand_బహుళకం_Var(extendsp->classp());
        }
        AstVar* స్థిరరాశి static_బహుళకం_Varp = create_Static_బహుళకం_Var(classp, "__Vstaticrandmode");
        m_static_Rand_బహుళకం_Vars[classp] = static_బహుళకం_Varp;
        తిరిగి static_బహుళకం_Varp;
    }
    AstVar* get_Static_Rand_బహుళకం_Var(AstClass* స్థిరరాశి classp) {
        ఒకవేళ (m_static_Rand_బహుళకం_Vars.లెక్క(classp)) తిరిగి m_static_Rand_బహుళకం_Vars[classp];
        ఒకవేళ (AstClassExtends* స్థిరరాశి extendsp = classp->extendsp()) {
            తిరిగి get_Static_Rand_బహుళకం_Var(extendsp->classp());
        }
        తిరిగి శూన్యసూచిక;
    }
    AstVar* create_బహుళకం_Var(AstClass* స్థిరరాశి classp, స్థిరరాశి అక్షరం* స్థిరరాశి name) {
        File_పంక్తి* స్థిరరాశి fl = classp->fileline();
        ఒకవేళ (!m_dynarrayDtp) {
            m_dynarrayDtp = కొత్త AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->కనుగొను_బిట్టు_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->కూడు_Typesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* స్థిరరాశి బహుళకం_Varp = కొత్త AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        బహుళకం_Varp->user2p(classp);
        classp->కూడు_Stmtsp(బహుళకం_Varp);
        తిరిగి బహుళకం_Varp;
    }
    AstVar* create_Static_బహుళకం_Var(AstClass* స్థిరరాశి classp, స్థిరరాశి అక్షరం* స్థిరరాశి name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_పంక్తి* స్థిరరాశి fl = classp->fileline();
        ఒకవేళ (!m_dynarrayDtp) {
            m_dynarrayDtp = కొత్త AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->కనుగొను_బిట్టు_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->కూడు_Typesp(m_dynarrayDtp);
        }
        AstVar* స్థిరరాశి బహుళకం_Varp = కొత్త AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        బహుళకం_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        బహుళకం_Varp->user2p(classp);
        classp->కూడు_Stmtsp(బహుళకం_Varp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.చొప్పించు(classp, బహుళకం_Varp);
        తిరిగి బహుళకం_Varp;
    }
    స్థిర శూన్య కూడు_సమితి_Rand_బహుళకం(Ast_కణుపు_FTask* స్థిరరాశి ftaskp, AstVar* స్థిరరాశి genp,
                               AstVar* స్థిరరాశి rand_బహుళకం_Varp) {
        File_పంక్తి* స్థిరరాశి fl = ftaskp->fileline();
        AstCMethodHard* స్థిరరాశి సమితి_Rand_Modep = కొత్త AstCMethodHard{
            fl, కొత్త AstVarRef{fl, VN_AS(genp->user2p(), కణుపు_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_సమితి_RANDMODE,
            కొత్త AstVarRef{fl, VN_AS(rand_బహుళకం_Varp->user2p(), కణుపు_Module), rand_బహుళకం_Varp,
                          VAccess::READ}};
        సమితి_Rand_Modep->dtype_సమితి_Void();
        ftaskp->కూడు_Stmtsp(సమితి_Rand_Modep->makeStmt());
    }
    స్థిర శూన్య కూడు_సమితి_Static_Rand_బహుళకం(Ast_కణుపు_FTask* స్థిరరాశి ftaskp, AstVar* స్థిరరాశి genp,
                                     AstVar* స్థిరరాశి static_Rand_బహుళకం_Varp) {
        File_పంక్తి* స్థిరరాశి fl = ftaskp->fileline();
        AstCMethodHard* స్థిరరాశి setp = కొత్త AstCMethodHard{
            fl, కొత్త AstVarRef{fl, VN_AS(genp->user2p(), కణుపు_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_సమితి_STATIC_RANDMODE,
            కొత్త AstVarRef{fl, VN_AS(static_Rand_బహుళకం_Varp->user2p(), కణుపు_Module), static_Rand_బహుళకం_Varp,
                          VAccess::READ}};
        setp->dtype_సమితి_Void();
        ftaskp->కూడు_Stmtsp(setp->makeStmt());
    }
    శూన్య createRandomizeClassVars(AstNetlist* స్థిరరాశి netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> మూలం_Static_Rand_బహుళకం_లెక్క;
        netlistp->foreach([ఇది, &మూలం_Static_Rand_బహుళకం_లెక్క](AstClass* స్థిరరాశి classp) {
            తార్కిక hasConstraints = అబద్ధం;
            uint32_t rand_బహుళకం_లెక్క = 0;
            uint32_t static_Rand_బహుళకం_లెక్క = 0;
            uint32_t constraint_బహుళకం_లెక్క = 0;
            uint32_t static_Constraint_బహుళకం_లెక్క = 0;
            classp->foreachMember([&](AstClass*, Ast_కణుపు* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                ఒకవేళ (AstConstraint* స్థిరరాశి constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = నిజం;
                    Randomize_బహుళకం constraint_బహుళకం = {.asUQuad = memberp->user1()};
                    ఒకవేళ (!constraint_బహుళకం.ఉపయోగిస్తుంది_బహుళకం) తిరిగి;
                    ఒకవేళ (constraint_బహుళకం.సూచీ == 0) {
                        // Use separate index counters for static vs non-static constraints
                        ఒకవేళ (constrp->isStatic()) {
                            constraint_బహుళకం.సూచీ = static_Constraint_బహుళకం_లెక్క++;
                        } లేకపోతే {
                            constraint_బహుళకం.సూచీ = constraint_బహుళకం_లెక్క++;
                        }
                        memberp->user1(constraint_బహుళకం.asUQuad);
                    } లేకపోతే {
                        ఒకవేళ (constrp->isStatic()) {
                            static_Constraint_బహుళకం_లెక్క = constraint_బహుళకం.సూచీ + 1;
                        } లేకపోతే {
                            constraint_బహుళకం_లెక్క = constraint_బహుళకం.సూచీ + 1;
                        }
                    }
                } లేకపోతే ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(memberp, Var)) {
                    Randomize_బహుళకం rand_బహుళకం = {.asUQuad = memberp->user1()};
                    ఒకవేళ (!rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం) తిరిగి;
                    స్థిరరాశి తార్కిక isStaticVar = varp->lifetime().isStatic();
                    ఒకవేళ (rand_బహుళకం.సూచీ == 0) {
                        ఒకవేళ (isStaticVar) {
                            rand_బహుళకం.సూచీ = static_Rand_బహుళకం_లెక్క++;
                        } లేకపోతే {
                            rand_బహుళకం.సూచీ = rand_బహుళకం_లెక్క++;
                        }
                        memberp->user1(rand_బహుళకం.asUQuad);
                    } లేకపోతే {
                        ఒకవేళ (isStaticVar) {
                            static_Rand_బహుళకం_లెక్క = rand_బహుళకం.సూచీ + 1;
                        } లేకపోతే {
                            rand_బహుళకం_లెక్క = rand_బహుళకం.సూచీ + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<శూన్య(AstClass*)> కనుగొను_Sub_Obj_Rand_Modes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_కణుపు* subMemberp) {
                        ఒకవేళ (AstVar* స్థిరరాశి subVarp = VN_CAST(subMemberp, Var)) {
                            స్థిరరాశి Randomize_బహుళకం rm = {.asUQuad = subVarp->user1()};
                            ఒకవేళ (!rm.ఉపయోగిస్తుంది_బహుళకం) తిరిగి;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            ఒకవేళ (subVarp->lifetime().isStatic()) తిరిగి;
                            స్థిరరాశి uint32_t needed = rm.సూచీ + 1;
                            ఒకవేళ (needed > rand_బహుళకం_లెక్క) rand_బహుళకం_లెక్క = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    ఒకవేళ (!memberVarp->globalConstrained()) తిరిగి;
                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = memberVarp->dtypep()->skipRefp();
                    స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(dtypep, ClassRefDType);
                    ఒకవేళ (!classRefp) తిరిగి;
                    కనుగొను_Sub_Obj_Rand_Modes(classRefp->classp());
                });
            }
            ఒకవేళ (hasConstraints) createRandomGenerator(classp);
            ఒకవేళ (rand_బహుళకం_లెక్క > 0) {
                AstVar* స్థిరరాశి rand_బహుళకం_Varp = get_Create_Rand_బహుళకం_Var(classp);
                make_బహుళకం_Init(rand_బహుళకం_Varp, classp, rand_బహుళకం_లెక్క);
            }
            ఒకవేళ (constraint_బహుళకం_లెక్క > 0) {
                AstVar* స్థిరరాశి constraint_బహుళకం_Varp = get_Create_Constraint_బహుళకం_Var(classp);
                make_బహుళకం_Init(constraint_బహుళకం_Varp, classp, constraint_బహుళకం_లెక్క);
            }
            ఒకవేళ (static_Constraint_బహుళకం_లెక్క > 0) {
                AstVar* స్థిరరాశి static_Constraint_బహుళకం_Varp = get_Create_Static_Constraint_బహుళకం_Var(classp);
                make_Static_బహుళకం_Init(static_Constraint_బహుళకం_Varp, classp, static_Constraint_బహుళకం_లెక్క);
            }
            ఒకవేళ (static_Rand_బహుళకం_లెక్క > 0) {
                get_Create_Static_Rand_బహుళకం_Var(classp);
                AstClass* rootp = classp;
                అయితే (AstClassExtends* స్థిరరాశి ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& స్థానం_2 = మూలం_Static_Rand_బహుళకం_లెక్క[rootp];
                ఒకవేళ (static_Rand_బహుళకం_లెక్క > స్థానం_2) స్థానం_2 = static_Rand_బహుళకం_లెక్క;
            }
        });
        కొరకు (స్థిరరాశి స్వయంచాలక& kv : మూలం_Static_Rand_బహుళకం_లెక్క) emit_మూలం_Static_బహుళకం_Init(kv.మొదటి, kv.రెండవ);
    }
    శూన్య emit_మూలం_Static_బహుళకం_Init(AstClass* స్థిరరాశి rootp, స్థిరరాశి uint32_t లెక్క) {
        AstVar* స్థిరరాశి static_Rand_బహుళకం_Varp = m_static_Rand_బహుళకం_Vars[rootp];
        UASSERT_OBJ(static_Rand_బహుళకం_Varp, rootp, "Root must have a static rand-mode var");
        make_Static_బహుళకం_Init(static_Rand_బహుళకం_Varp, rootp, లెక్క);
    }
    శూన్య make_బహుళకం_Init(AstVar* బహుళకం_Varp, AstClass* classp, uint32_t బహుళకం_లెక్క) {
        Ast_కణుపు_Module* స్థిరరాశి బహుళకం_Var_Modp = VN_AS(బహుళకం_Varp->user2p(), కణుపు_Module);
        File_పంక్తి* స్థిరరాశి fl = బహుళకం_Varp->fileline();
        AstVar* స్థిరరాశి old_కొలత_Varp = కొత్త AstVar{
            fl, VVarType::BLOCKTEMP, m_unique_పేర్లు.get("__VoldSize"), బహుళకం_Varp->కనుగొను_UInt32_DType()};
        old_కొలత_Varp->funcLocal(నిజం);
        old_కొలత_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* స్థిరరాశి oldSizep
            = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, బహుళకం_Var_Modp, బహుళకం_Varp, VAccess::READ},
                                 VCMethod::DYN_కొలత, శూన్యసూచిక};
        oldSizep->dtype_సమితి_UInt32();
        AstCMethodHard* స్థిరరాశి dynarrayNewp
            = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, బహుళకం_Var_Modp, బహుళకం_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, కొత్త AstConst{fl, బహుళకం_లెక్క}};
        dynarrayNewp->dtype_సమితి_Void();
        Ast_కణుపు_FTask* స్థిరరాశి ctorNewp = VN_AS(m_memberMap.కనుగొను_Member(classp, "new"), కణుపు_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_కణుపు* స్థిరరాశి initFirstp = old_కొలత_Varp;
        initFirstp->కూడు_తదుపరి(
            కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, old_కొలత_Varp, VAccess::WRITE}, oldSizep});
        initFirstp->కూడు_తదుపరి(dynarrayNewp->makeStmt());
        initFirstp->కూడు_తదుపరి(make_బహుళకం_సమితి_Loop(
            fl, కొత్త AstVarRef{fl, బహుళకం_Var_Modp, బహుళకం_Varp, VAccess::WRITE}, కొత్త AstConst{fl, 1},
            నిజం, కొత్త AstVarRef{fl, old_కొలత_Varp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        కొరకు (Ast_కణుపు* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (!VN_IS(stmtp, కణుపు_Stmt)) కొనసాగు;  // Skip var declarations
            ఒకవేళ (స్థిరరాశి Ast_Stmt_సమాసం* స్థిరరాశి sep = VN_CAST(stmtp, Stmt_సమాసం)) {
                ఒకవేళ (VN_IS(sep->exprp(), New)) కొనసాగు;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->కూడు_ఇక్కడ_This_As_తదుపరి(initFirstp);
            తిరిగి;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->కూడు_Stmtsp(initFirstp);
    }
    శూన్య make_Static_బహుళకం_Init(AstVar* బహుళకం_Varp, AstClass* classp, uint32_t బహుళకం_లెక్క) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_కణుపు_Module* స్థిరరాశి బహుళకం_Var_Modp = VN_AS(బహుళకం_Varp->user2p(), కణుపు_Module);
        File_పంక్తి* స్థిరరాశి fl = బహుళకం_Varp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* స్థిరరాశి sizep
            = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, బహుళకం_Var_Modp, బహుళకం_Varp, VAccess::READ},
                                 VCMethod::DYN_కొలత, శూన్యసూచిక};
        sizep->dtype_సమితి_UInt32();
        AstEq* స్థిరరాశి condp = కొత్త AstEq{fl, sizep, కొత్త AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* స్థిరరాశి dynarrayNewp
            = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, బహుళకం_Var_Modp, బహుళకం_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, కొత్త AstConst{fl, బహుళకం_లెక్క}};
        dynarrayNewp->dtype_సమితి_Void();
        Ast_కణుపు* స్థిరరాశి thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->కూడు_తదుపరి(
            make_బహుళకం_సమితి_Loop(fl, కొత్త AstVarRef{fl, బహుళకం_Var_Modp, బహుళకం_Varp, VAccess::WRITE},
                            కొత్త AstConst{fl, 1}, నిజం));

        // Build the if statement
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_కణుపు_FTask* స్థిరరాశి newp = VN_AS(m_memberMap.కనుగొను_Member(classp, "new"), కణుపు_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->కూడు_Stmtsp(ifp);
    }
    Ast_కణుపు* make_బహుళకం_సమితి_Loop(File_పంక్తి* స్థిరరాశి fl, Ast_కణుపు_సమాసం* స్థిరరాశి lhsp, Ast_కణుపు_సమాసం* స్థిరరాశి rhsp,
                             తార్కిక లోపల_Task, Ast_కణుపు_సమాసం* స్థిరరాశి startp = శూన్యసూచిక) {
        AstVar* స్థిరరాశి iterVarp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->కనుగొను_UInt32_DType()};
        iterVarp->funcLocal(లోపల_Task);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* స్థిరరాశి sizep = కొత్త AstCMethodHard{fl, lhsp, VCMethod::DYN_కొలత, శూన్యసూచిక};
        sizep->dtype_సమితి_UInt32();
        AstCMethodHard* స్థిరరాశి setp
            = కొత్త AstCMethodHard{fl, lhsp->clone_వృక్షం(అబద్ధం), VCMethod::ARRAY_AT_WRITE,
                                 కొత్త AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtype_సమితి_UInt32();
        Ast_కణుపు* స్థిరరాశి stmtsp = iterVarp;
        stmtsp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : కొత్త AstConst{fl, 0}});

        AstLoop* స్థిరరాశి loopp = కొత్త AstLoop{fl};
        stmtsp->కూడు_తదుపరి(loopp);
        loopp->కూడు_Stmtsp(కొత్త AstLoopTest{
            fl, loopp, కొత్త AstLt{fl, కొత్త AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, setp, rhsp});
        loopp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
            fl, కొత్త AstVarRef{fl, iterVarp, VAccess::WRITE},
            కొత్త Ast_కూడు{fl, కొత్త AstConst{fl, 1}, కొత్త AstVarRef{fl, iterVarp, VAccess::READ}}});
        తిరిగి కొత్త AstBegin{fl, m_unique_పేర్లు.get("__VrandBlock"), stmtsp, నిజం};
    }
    శూన్య కూడు_తాత్కాలిక_Varsp(AstVar* varsp) {
        కొరకు (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ఒకవేళ (m_ftaskp) {
                varp->funcLocal(నిజం);
            } లేకపోతే {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        ఒకవేళ (!m_ftaskp) {
            m_modp->కూడు_Stmtsp(varsp);
        } లేకపోతే {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_కణుపు_Stmt* wrap_If_Rand_బహుళకం(AstClass* classp, AstVar* స్థిరరాశి varp, Ast_కణుపు_Stmt* stmtp) {
        స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = varp->user1()};
        AstVar* స్థిరరాశి బహుళకం_Varp = varp->lifetime().isStatic() ? get_Static_Rand_బహుళకం_Var(classp)
                                                             : get_Rand_బహుళకం_Var_From_Class(classp);
        తిరిగి VN_AS(wrap_If_బహుళకం(rmode, బహుళకం_Varp, stmtp), కణుపు_Stmt);
    }
    Ast_కణుపు* wrap_If_Constraint_బహుళకం(AstClass* classp, AstConstraint* స్థిరరాశి constrp, Ast_కణుపు* stmtp) {
        స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = constrp->user1()};
        AstVar* స్థిరరాశి బహుళకం_Varp = constrp->isStatic() ? get_Static_Constraint_బహుళకం_Var(classp)
                                                     : get_Constraint_బహుళకం_Var(classp);
        తిరిగి wrap_If_బహుళకం(rmode, బహుళకం_Varp, stmtp);
    }
    స్థిర Ast_కణుపు* wrap_If_బహుళకం(స్థిరరాశి Randomize_బహుళకం బహుళకం, AstVar* బహుళకం_Varp, Ast_కణుపు* stmtp) {
        File_పంక్తి* స్థిరరాశి fl = stmtp->fileline();
        ఒకవేళ (బహుళకం.ఉపయోగిస్తుంది_బహుళకం) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_కణుపు_Module* స్థిరరాశి modp = VN_AS(బహుళకం_Varp->user2p(), కణుపు_Module);
            AstCMethodHard* స్థిరరాశి atp
                = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, modp, బహుళకం_Varp, VAccess::READ},
                                     VCMethod::ARRAY_AT, కొత్త AstConst{fl, బహుళకం.సూచీ}};
            atp->dtype_సమితి_UInt32();
            తిరిగి కొత్త AstIf{fl, atp, stmtp};
        }
        తిరిగి stmtp;
    }
    AstVarRef* enum_విలువ_Tab_Refp(AstEnumDType* స్థిరరాశి nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        ఒకవేళ (nodep->user2p()) తిరిగి VN_AS(nodep->user2p(), VarRef)->clone_వృక్షం(అబద్ధం);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_కణుపు_Array_DType* స్థిరరాశి vardtypep = కొత్త AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            కొత్త AstRange{nodep->fileline(), static_cast<పూర్ణాంకం>(nodep->అంశం_లెక్క()) - 1, 0}};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(vardtypep);
        AstConsArray* స్థిరరాశి consp = కొత్త AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t సూచీ = 0;
        కొరకు (Ast_Enum_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
            AstConst* స్థిరరాశి vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(సూచీ++, vconstp->clone_వృక్షం(అబద్ధం));
        }
        // Share identical tables via the constant pool
        AstVarRef* స్థిరరాశి refp = V3ConstPool::కనుగొను(consp);
        VL_DO_DANGLING(consp->delete_వృక్షం(), consp);  // V3ConstPool::find clones it
        నెట్టు_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        తిరిగి refp->clone_వృక్షం(అబద్ధం);
    }

    AstCDType* కనుగొను_Vl_Rand_CDType(File_పంక్తి* స్థిరరాశి fl, uint64_t వస్తువులు) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        స్థిరరాశి std::string type = AstCDType::type_గమ్యం_Hold(V3_సంఖ్య::log2bQuad(వస్తువులు) + 1);
        స్థిరరాశి std::string name = "VlRandC<" + type + ", " + cvt_గమ్యం_Str(వస్తువులు) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        స్థిరరాశి స్వయంచాలక pair = m_randcDtypes.emplace(name, శూన్యసూచిక);
        ఒకవేళ (pair.రెండవ) {
            AstCDType* స్థిరరాశి newp = కొత్త AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(newp);
            pair.మొదటి->రెండవ = newp;
        }
        తిరిగి pair.మొదటి->రెండవ;
    }

    AstVar* newRandcVarsp(AstVar* స్థిరరాశి varp, AstClass* స్థిరరాశి classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        ఒకవేళ (!varp->isRandC()) తిరిగి శూన్యసూచిక;

        uint64_t వస్తువులు = 0;
        ఒకవేళ (AstEnumDType* స్థిరరాశి enumDtp = VN_CAST(varp->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType)) {
            వస్తువులు = static_cast<uint64_t>(enumDtp->అంశం_లెక్క());
        } లేకపోతే ఒకవేళ (AstBasicDType* స్థిరరాశి basicp = varp->dtypep()->skipRefp()->basicp()) {
            ఒకవేళ (basicp->వెడల్పు() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                ఒకవేళ (!V3Error::error_లెక్క()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->వెడల్పు() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                తిరిగి శూన్యసూచిక;
            }
            వస్తువులు = 1ULL << basicp->వెడల్పు();
        } లేకపోతే ఒకవేళ (AstStructDType* స్థిరరాశి dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            తిరిగి శూన్యసూచిక;
        } లేకపోతే {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_లెక్క(), varp, "Unexpected randc variable dtype");
            తిరిగి శూన్యసూచిక;
        }
        AstCDType* స్థిరరాశి newdtp = కనుగొను_Vl_Rand_CDType(varp->fileline(), వస్తువులు);
        AstVar* స్థిరరాశి newp
            = కొత్త AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->is_అంతర్గత(నిజం);
        classp->కూడు_Stmtsp(newp);
        UINFO(9, "created " << varp);
        తిరిగి newp;
    }
    Ast_కణుపు_Stmt* createArrayForeachLoop(File_పంక్తి* స్థిరరాశి fl, Ast_కణుపు_DType* స్థిరరాశి dtypep,
                                        Ast_కణుపు_సమాసం* exprp, AstVar* స్థిరరాశి outputVarp) {
        Ast_కణుపు_DType* తాత్కాలిక_DTypep = dtypep;
        AstVar* randLoopIndxp = శూన్యసూచిక;
        స్వయంచాలక create_Loop_సూచీ = [&](Ast_కణుపు_DType* తాత్కాలిక_DTypep) {
            ఒకవేళ (VN_IS(తాత్కాలిక_DTypep, AssocArrayDType)) {
                తిరిగి కొత్త AstVar{fl, VVarType::VAR, m_rand_Array_Unique_పేర్లు.get(""),
                                  VN_AS(తాత్కాలిక_DTypep, AssocArrayDType)->కీలకం_DTypep()};
            }
            తిరిగి కొత్త AstVar{fl, VVarType::VAR, m_rand_Array_Unique_పేర్లు.get(""),
                              dtypep->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
        };
        Ast_కణుపు_సమాసం* తాత్కాలిక_Elementp = శూన్యసూచిక;
        అయితే (తాత్కాలిక_DTypep->isNonPackedArray()) {
            AstVar* స్థిరరాశి newRandLoopIndxp = create_Loop_సూచీ(తాత్కాలిక_DTypep);
            randLoopIndxp = Ast_కణుపు::కూడు_తదుపరి(randLoopIndxp, newRandLoopIndxp);
            Ast_కణుపు_సమాసం* స్థిరరాశి తాత్కాలిక_Exprp = తాత్కాలిక_Elementp ? తాత్కాలిక_Elementp : exprp;
            AstVarRef* స్థిరరాశి తాత్కాలిక_Refp = కొత్త AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            ఒకవేళ (VN_IS(తాత్కాలిక_DTypep, DynArrayDType)) {
                తాత్కాలిక_Elementp
                    = కొత్త AstCMethodHard{fl, తాత్కాలిక_Exprp, VCMethod::ARRAY_AT_WRITE, తాత్కాలిక_Refp};
            } లేకపోతే ఒకవేళ (VN_IS(తాత్కాలిక_DTypep, UnpackArrayDType)) {
                Ast_కణుపు_Array_DType* స్థిరరాశి aryDTypep = VN_CAST(తాత్కాలిక_DTypep, కణుపు_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                తాత్కాలిక_Elementp = కొత్త AstArraySel{
                    fl, తాత్కాలిక_Exprp,
                    కొత్త AstSel{
                        fl,
                        కొత్త AstSub{fl, తాత్కాలిక_Refp,
                                   కొత్త AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        కొత్త AstConst{fl, 0}, V3_సంఖ్య::log2b(aryDTypep->hi()) + 1}};
            } లేకపోతే ఒకవేళ (VN_IS(తాత్కాలిక_DTypep, AssocArrayDType)) {
                తాత్కాలిక_Elementp = కొత్త AstAssocSel{fl, తాత్కాలిక_Exprp, తాత్కాలిక_Refp};
            } లేకపోతే ఒకవేళ (VN_IS(తాత్కాలిక_DTypep, వరుస_DType)) {
                తాత్కాలిక_Elementp
                    = కొత్త AstCMethodHard{fl, తాత్కాలిక_Exprp, VCMethod::DYN_AT_WRITE_APPEND, తాత్కాలిక_Refp};
            }
            తాత్కాలిక_Elementp->dtypep(తాత్కాలిక_DTypep->subDTypep());
            తాత్కాలిక_DTypep = తాత్కాలిక_DTypep->virtRefDTypep();
        }

        Ast_Foreach_శీర్షిక* స్థిరరాశి headerp
            = కొత్త Ast_Foreach_శీర్షిక{fl, exprp->clone_వృక్షం(అబద్ధం), randLoopIndxp};
        Ast_కణుపు_Stmt* స్థిరరాశి randStmtsp = newRandStmtsp(fl, తాత్కాలిక_Elementp, శూన్యసూచిక, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        ఒకవేళ (!తాత్కాలిక_Elementp->backp()) VL_DO_DANGLING(నెట్టు_Deletep(తాత్కాలిక_Elementp), తాత్కాలిక_Elementp);
        తిరిగి కొత్త AstForeach{fl, headerp, randStmtsp};
    }
    Ast_కణుపు_Stmt* newRandStmtsp(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, AstVar* randcVarp,
                               AstVar* స్థిరరాశి outputVarp, పూర్ణాంకం స్థానభ్రంశం = 0,
                               AstMemberDType* memberp = శూన్యసూచిక) {
        Ast_కణుపు_DType* స్థిరరాశి memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_కణుపు_Stmt* stmtsp = శూన్యసూచిక;
            ఒకవేళ (structDtp->packed()) స్థానభ్రంశం += memberp ? memberp->lsb() : 0;
            కొరకు (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_కణుపు_Stmt* randp = శూన్యసూచిక;
                ఒకవేళ (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_వృక్షం(అబద్ధం) : exprp, శూన్యసూచిక,
                                          outputVarp, స్థానభ్రంశం, smemberp);
                } లేకపోతే ఒకవేళ (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = కొత్త AstStructSel{fl, exprp->clone_వృక్షం(అబద్ధం), smemberp->name()};
                    structSelp->dtypep(smemberp->బిడ్డ_DTypep());
                    ఒకవేళ (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, శూన్యసూచిక, outputVarp);
                }
                stmtsp = Ast_కణుపు::కూడు_తదుపరి_Null(stmtsp, randp);
            }
            తిరిగి stmtsp;
        } లేకపోతే ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            ఒకవేళ (!unionDtp->packed()) తిరిగి శూన్యసూచిక;
            AstMemberDType* స్థిరరాశి మొదటి_Memberp = unionDtp->membersp();
            తిరిగి newRandStmtsp(fl, exprp, శూన్యసూచిక, outputVarp, స్థానభ్రంశం, మొదటి_Memberp);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* స్థిరరాశి memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            Ast_పద్ధతి_Call* స్థిరరాశి callp = కొత్త Ast_పద్ధతి_Call{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{
                fl, కొత్త AstVarRef{fl, outputVarp, VAccess::WRITE},
                కొత్త AstAnd{fl, కొత్త AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            తిరిగి కొత్త AstIf{
                fl, కొత్త AstNeq{fl, exprp->clone_వృక్షం(అబద్ధం), కొత్త AstConst{fl, AstConst::Null{}}},
                assignp};
        } లేకపోతే ఒకవేళ (AstDynArrayDType* స్థిరరాశి dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            తిరిగి createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } లేకపోతే ఒకవేళ (Ast_వరుస_DType* స్థిరరాశి వరుస_Dtp = VN_CAST(memberDtp, వరుస_DType)) {
            తిరిగి createArrayForeachLoop(fl, వరుస_Dtp, exprp, outputVarp);
        } లేకపోతే ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            తిరిగి createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } లేకపోతే ఒకవేళ (AstAssocArrayDType* స్థిరరాశి assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            తిరిగి createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } లేకపోతే {
            Ast_కణుపు_సమాసం* valp;
            ఒకవేళ (AstEnumDType* స్థిరరాశి enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* స్థిరరాశి tabRefp = enum_విలువ_Tab_Refp(enumDtp);
                Ast_కణుపు_సమాసం* స్థిరరాశి randp
                    = new_Rand_విలువ(fl, randcVarp, exprp->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32));
                Ast_కణుపు_సమాసం* స్థిరరాశి moddivp = కొత్త AstModDiv{
                    fl, randp, కొత్త AstConst{fl, static_cast<uint32_t>(enumDtp->అంశం_లెక్క())}};
                moddivp->dtypep(enumDtp);
                valp = కొత్త AstArraySel{fl, tabRefp, moddivp};
            } లేకపోతే {
                valp
                    = new_Rand_విలువ(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            Ast_కేటాయించు* assignp
                = కొత్త Ast_కేటాయించు{fl,
                                కొత్త AstSel{fl, exprp, స్థానభ్రంశం + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->వెడల్పు() : exprp->వెడల్పు()},
                                valp};
            AstVar* varp = శూన్యసూచిక;
            exprp->exists([&](స్థిరరాశి AstVarRef* varrefp) {
                ఒకవేళ (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                తిరిగి varp != శూన్యసూచిక;
            });
            తిరిగి wrap_If_Rand_బహుళకం(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_కణుపు_సమాసం* new_Rand_విలువ(File_పంక్తి* స్థిరరాశి fl, AstVar* స్థిరరాశి randcVarp,
                              Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        ఒకవేళ (randcVarp) {
            AstCExpr* స్థిరరాశి cexprp = కొత్త AstCExpr{fl};
            cexprp->కూడు(కొత్త AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->కూడు(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            తిరిగి cexprp;
        }

        తిరిగి కొత్త AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* కనుగొను_Pre_Post_Task(AstClass* classp, స్థిరరాశి string& name) {
        కొరకు (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : శూన్యసూచిక) {
            ఒకవేళ (AstTask* స్థిరరాశి taskp = VN_CAST(m_memberMap.కనుగొను_Member(cp, name), Task)) {
                తిరిగి taskp;
            }
        }
        తిరిగి శూన్యసూచిక;
    }
    శూన్య కూడు_Pre_Post_Call(AstClass* స్థిరరాశి classp, Ast_కణుపు_FTask* స్థిరరాశి funcp, స్థిరరాశి string& name) {
        ఒకవేళ (AstTask* స్థిరరాశి userFuncp = కనుగొను_Pre_Post_Task(classp, name)) {
            AstTaskRef* స్థిరరాశి callp = కొత్త AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->కూడు_Stmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* స్థిరరాశి classp, స్థిరరాశి string& which) {
        స్థిరరాశి string name = "__V" + which;
        ఒకవేళ (AstTask* స్థిరరాశి existingp = VN_CAST(m_memberMap.కనుగొను_Member(classp, name), Task)) {
            తిరిగి existingp;
        }
        AstTask* స్థిరరాశి taskp = కొత్త AstTask{classp->fileline(), name, శూన్యసూచిక};
        taskp->class_పద్ధతి(నిజం);
        taskp->isVirtual(classp->isExtended());
        classp->కూడు_Membersp(taskp);
        m_memberMap.చొప్పించు(classp, taskp);
        కూడు_Pre_Post_Call(classp, taskp, which);
        తిరిగి taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<తార్కిక, తార్కిక> buildPrePostVirtualWrappers(AstClass* స్థిరరాశి classp) {
        స్థిరరాశి స్వయంచాలక cachedIt = m_prePostWrap.కనుగొను(classp);
        ఒకవేళ (cachedIt != m_prePostWrap.చివర()) తిరిగి cachedIt->రెండవ;
        std::సదిశ<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            ఒకవేళ (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.నెట్టు_వెనుక(subp);
        });
        తార్కిక hasPre = అబద్ధం;
        తార్కిక hasPost = అబద్ధం;
        కొరకు (AstClass* స్థిరరాశి cp : hierp) {
            ఒకవేళ (కనుగొను_Pre_Post_Task(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = నిజం;
            }
            ఒకవేళ (కనుగొను_Pre_Post_Task(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = నిజం;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        ఒకవేళ (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        ఒకవేళ (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        స్థిరరాశి std::pair<తార్కిక, తార్కిక> ఫలితం{hasPre, hasPost};
        m_prePostWrap.emplace(classp, ఫలితం);
        తిరిగి ఫలితం;
    }
    శూన్య కూడు_Virtual_Pre_Post_Call(AstFunc* స్థిరరాశి randomizeFuncp, AstClass* స్థిరరాశి classp,
                               స్థిరరాశి string& which) {
        File_పంక్తి* స్థిరరాశి fl = classp->fileline();
        AstTask* స్థిరరాశి wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* స్థిరరాశి refDTypep = కొత్త AstClassRefDType{fl, classp, శూన్యసూచిక};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(refDTypep);
        Ast_పద్ధతి_Call* స్థిరరాశి callp
            = కొత్త Ast_పద్ధతి_Call{fl, కొత్త AstThisRef{fl, refDTypep}, wrapperp->name(), శూన్యసూచిక};
        callp->taskp(wrapperp);
        callp->dtype_సమితి_Void();
        randomizeFuncp->కూడు_Stmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    తార్కిక classHasRandClassMembers(AstClass* classp) {
        తిరిగి classp->existsMember([](స్థిరరాశి AstClass*, స్థిరరాశి AstVar* varp) {
            ఒకవేళ (!varp->rand().isRandomizable()) తిరిగి అబద్ధం;
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->dtypep()->skipRefp();
            తిరిగి VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    తార్కిక classOwnsGlobalConstraint(స్థిరరాశి AstClass* classp) స్థిరరాశి {
        తిరిగి classp->existsMember([](స్థిరరాశి AstClass*, స్థిరరాశి AstConstraint* constrp) {
            తార్కిక owns = అబద్ధం;
            constrp->foreach([&](స్థిరరాశి AstMemberSel* memberSelp) {
                స్థిరరాశి Ast_కణుపు* స్థిరరాశి rootp
                    = const_cast<AstMemberSel*>(memberSelp)->పునాది_Fromp(నిజం);
                ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    స్థిరరాశి Ast_కణుపు_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    అయితే (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    ఒకవేళ (VN_IS(dtypep, ClassRefDType)) owns = నిజం;
                }
            });
            తిరిగి owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, స్థిరరాశి string& suffix) {
        స్థిరరాశి string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.కనుగొను_Member(classp, name), Task);
        ఒకవేళ (taskp) తిరిగి taskp;
        taskp = కొత్త AstTask{classp->fileline(), name, శూన్యసూచిక};
        taskp->class_పద్ధతి(నిజం);
        classp->కూడు_Membersp(taskp);
        m_memberMap.చొప్పించు(classp, taskp);
        తిరిగి taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    శూన్య populateNestedCallbackTask(AstTask* స్థిరరాశి callbackTaskp, AstClass* స్థిరరాశి classp,
                                    స్థిరరాశి string& cbName) {
        File_పంక్తి* స్థిరరాశి fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            ఒకవేళ (!memberVarp->rand().isRandomizable()) తిరిగి;
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = memberVarp->dtypep()->skipRefp();
            స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(dtypep, ClassRefDType);
            ఒకవేళ (!classRefp) తిరిగి;
            AstClass* స్థిరరాశి memberClassp = classRefp->classp();
            ఒకవేళ (memberClassp == classp) తిరిగి;  // Avoid self-reference

            // Force-visit member class if not yet processed
            ఒకవేళ (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_కణుపు* stmtsp = శూన్యసూచిక;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            ఒకవేళ (AstTask* స్థిరరాశి userFuncp = కనుగొను_Pre_Post_Task(memberClassp, cbName)) {
                Ast_పద్ధతి_Call* స్థిరరాశి callp = కొత్త Ast_పద్ధతి_Call{
                    fl, కొత్త AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtype_సమితి_Void();
                stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            ఒకవేళ (classHasRandClassMembers(memberClassp)) {
                స్థిరరాశి string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* స్థిరరాశి nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                Ast_పద్ధతి_Call* స్థిరరాశి recurseCallp = కొత్త Ast_పద్ధతి_Call{
                    fl, కొత్త AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtype_సమితి_Void();
                stmtsp = Ast_కణుపు::కూడు_తదుపరి(stmtsp, recurseCallp->makeStmt());
            }

            ఒకవేళ (!stmtsp) తిరిగి;

            // Wrap in null check
            AstIf* స్థిరరాశి nullCheckp = కొత్త AstIf{
                fl,
                కొత్త AstNeq{fl, కొత్త AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           కొత్త AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->కూడు_Stmtsp(wrap_If_Rand_బహుళకం(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* new_అమరిక_Constraint_Task(AstClass* స్థిరరాశి nodep, స్థిరరాశి std::string& name) {
        AstTask* స్థిరరాశి taskp = కొత్త AstTask{nodep->fileline(), name + "_setup_constraint", శూన్యసూచిక};
        taskp->class_పద్ధతి(నిజం);
        nodep->కూడు_Membersp(taskp);
        తిరిగి taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* స్థిరరాశి nodep, స్థిరరాశి std::string& name) {
        AstTask* స్థిరరాశి taskp
            = కొత్త AstTask{nodep->fileline(), name + "_resize_constrained_array", శూన్యసూచిక};
        taskp->class_పద్ధతి(నిజం);
        nodep->కూడు_Membersp(taskp);
        తిరిగి taskp;
    }
    Ast_కణుపు_Stmt* implementConstraintsClear(File_పంక్తి* స్థిరరాశి fileline, AstVar* స్థిరరాశి genp) {
        AstCMethodHard* స్థిరరాశి clearp = కొత్త AstCMethodHard{
            fileline,
            కొత్త AstVarRef{fileline, VN_AS(genp->user2p(), కణుపు_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtype_సమితి_Void();
        తిరిగి clearp->makeStmt();
    }
    Ast_కణుపు_Stmt* implementConstraintsClearAll(File_పంక్తి* స్థిరరాశి fileline, AstVar* స్థిరరాశి genp) {
        AstCMethodHard* స్థిరరాశి clearp = కొత్త AstCMethodHard{
            fileline,
            కొత్త AstVarRef{fileline, VN_AS(genp->user2p(), కణుపు_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtype_సమితి_Void();
        తిరిగి clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_కణుపు_సమాసం* స్థిరరాశి exprp) {
        ఒకవేళ (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(exprp, MemberSel)) {
            తిరిగి memberSelp->varp();
        } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(exprp, VarRef)) {
            తిరిగి varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        తిరిగి శూన్యసూచిక;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_కణుపు_సమాసం* make_బహుళకం_Var_Ref(Ast_కణుపు_సమాసం* స్థిరరాశి exprp, AstVar* స్థిరరాశి బహుళకం_Varp,
                                స్థిరరాశి VAccess access) {
        ఒకవేళ (బహుళకం_Varp->lifetime().isStatic()) {
            తిరిగి కొత్త AstVarRef{exprp->fileline(), VN_AS(బహుళకం_Varp->user2p(), కణుపు_Module),
                                 బహుళకం_Varp, access};
        }
        ఒకవేళ (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* స్థిరరాశి newMemberSelp = కొత్త AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_వృక్షం(అబద్ధం), బహుళకం_Varp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            తిరిగి newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        తిరిగి కొత్త AstVarRef{exprp->fileline(), VN_AS(బహుళకం_Varp->user2p(), Class), బహుళకం_Varp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* create_Or_Get_కొలత_Var(AstClass* స్థిరరాశి classp, AstVar* స్థిరరాశి arrVarp, File_పంక్తి* స్థిరరాశి fl,
                               Ast_కణుపు_DType* స్థిరరాశి signed32DTypep, తార్కిక& wasCreated) {
        AstVar* కొలత_Varp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = అబద్ధం;
        ఒకవేళ (!కొలత_Varp) {
            కొలత_Varp = కొత్త AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->కూడు_Membersp(కొలత_Varp);
            m_memberMap.చొప్పించు(classp, కొలత_Varp);
            కొలత_Varp->user2p(classp);
            arrVarp->user4p(కొలత_Varp);
            wasCreated = నిజం;
        }
        తిరిగి కొలత_Varp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_సమాసం* create_కొలత_Gte_Zero_Constraint(File_పంక్తి* స్థిరరాశి fl, AstVar* స్థిరరాశి కొలత_Varp,
                                                   Ast_కణుపు_సమాసం* derefFromp = శూన్యసూచిక) {
        Ast_కణుపు_సమాసం* కొలత_కణుపు_Exprp = శూన్యసూచిక;
        ఒకవేళ (derefFromp) {
            కొలత_కణుపు_Exprp = buildMemberSelExprp(derefFromp, కొలత_Varp);
        } లేకపోతే {
            కొలత_కణుపు_Exprp = కొత్త AstVarRef{fl, కొలత_Varp, VAccess::READ};
        }
        కొలత_కణుపు_Exprp->user1(నిజం);
        AstGteS* స్థిరరాశి కొలత_Gtep = కొత్త AstGteS{fl, కొలత_కణుపు_Exprp, కొత్త AstConst{fl, 0}};
        కొలత_Gtep->user1(నిజం);
        తిరిగి కొత్త Ast_Constraint_సమాసం{fl, కొలత_Gtep};
    }
    Ast_కణుపు_సమాసం* getFromp(Ast_కణుపు_సమాసం* స్థిరరాశి exprp) {
        ఒకవేళ (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(exprp, MemberSel)) {
            తిరిగి memberSelp->fromp();
        } లేకపోతే ఒకవేళ (Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(exprp, పద్ధతి_Call)) {
            తిరిగి పద్ధతి_Callp->fromp();
        }
        తిరిగి శూన్యసూచిక;
    }
    AstVar* make_Tmp_Rand_బహుళకం_Var(Ast_కణుపు_సమాసం* siblingExprp, AstVar* rand_బహుళకం_Varp,
                               Ast_కణుపు*& storeStmtspr, Ast_కణుపు_Stmt*& restoreStmtspr) {
        File_పంక్తి* స్థిరరాశి fl = rand_బహుళకం_Varp->fileline();
        AstVar* స్థిరరాశి rand_బహుళకం_Tmp_Varp = కొత్త AstVar{
            fl, VVarType::BLOCKTEMP, m_బహుళకం_Unique_పేర్లు.get(rand_బహుళకం_Varp), rand_బహుళకం_Varp->dtypep()};
        rand_బహుళకం_Tmp_Varp->funcLocal(m_ftaskp);
        rand_బహుళకం_Tmp_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_కణుపు::కూడు_తదుపరి(
            storeStmtspr,
            కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, rand_బహుళకం_Tmp_Varp, VAccess::WRITE},
                          make_బహుళకం_Var_Ref(siblingExprp, rand_బహుళకం_Varp, VAccess::READ)});
        storeStmtspr = Ast_కణుపు::కూడు_తదుపరి(
            storeStmtspr,
            make_బహుళకం_సమితి_Loop(fl, make_బహుళకం_Var_Ref(siblingExprp, rand_బహుళకం_Varp, VAccess::WRITE),
                            కొత్త AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_కణుపు::కూడు_తదుపరి(
            restoreStmtspr,
            కొత్త Ast_కేటాయించు{fl, make_బహుళకం_Var_Ref(siblingExprp, rand_బహుళకం_Varp, VAccess::WRITE),
                          కొత్త AstVarRef{fl, rand_బహుళకం_Tmp_Varp, VAccess::READ}});
        తిరిగి rand_బహుళకం_Tmp_Varp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_కణుపు_సమాసం* ముక్క_గమ్యం_Common_ఉపసర్గ(Ast_కణుపు_సమాసం* thisp, Ast_కణుపు_సమాసం* otherp) {
        స్థిర std::సదిశ<Ast_కణుపు_సమాసం*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        అయితే (thisp) {
            thisHier.నెట్టు_వెనుక(thisp);
            thisp = getFromp(thisp);
        }
        అయితే (otherp) {
            otherHier.నెట్టు_వెనుక(otherp);
            otherp = getFromp(otherp);
        }
        Ast_కణుపు_సమాసం* commonp = శూన్యసూచిక;
        కొరకు (స్వయంచాలక thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            ఒకవేళ ((*thisIt)->type() != (*otherIt)->type()) ఆపు;
            ఒకవేళ (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* స్థిరరాశి otherMemberSelp = VN_AS(*otherIt, MemberSel);
                ఒకవేళ (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    కొనసాగు;
                }
            } లేకపోతే ఒకవేళ (Ast_పద్ధతి_Call* స్థిరరాశి this_పద్ధతి_Callp = VN_CAST(*thisIt, పద్ధతి_Call)) {
                Ast_పద్ధతి_Call* స్థిరరాశి other_పద్ధతి_Callp = VN_AS(*otherIt, పద్ధతి_Call);
                ఒకవేళ (this_పద్ధతి_Callp->taskp() == other_పద్ధతి_Callp->taskp()) {
                    commonp = this_పద్ధతి_Callp;
                    కొనసాగు;
                }
            } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి మొదటి_Var_Refp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* స్థిరరాశి రెండవ_Var_Refp = VN_AS(*otherIt, VarRef);
                ఒకవేళ (మొదటి_Var_Refp->varp() == రెండవ_Var_Refp->varp()) {
                    commonp = మొదటి_Var_Refp;
                    కొనసాగు;
                }
            }
            ఆపు;
        }
        తిరిగి commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_కణుపు_సమాసం* buildMemberSelExprp(Ast_కణుపు_సమాసం* exprp, AstVar* అత్యంతలోతు_Varp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_కణుపు_సమాసం* resExprp = శూన్యసూచిక;
        ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), శూన్యసూచిక);
            AstVar* varp = memberSelp->varp();
            ఒకవేళ (అత్యంతలోతు_Varp) varp = అత్యంతలోతు_Varp;
            resExprp = కొత్త AstMemberSel{exprp->fileline(), resExprp, varp};
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            ఒకవేళ (అత్యంతలోతు_Varp) varp = అత్యంతలోతు_Varp;
            resExprp = కొత్త AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), కణుపు_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(నిజం);
        తిరిగి resExprp;
    }

    శూన్య కూడు_Update_Rand_Vars_Body(AstClass* స్థిరరాశి nodep, AstFunc* స్థిరరాశి randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* స్థిరరాశి updatep
            = కొత్త AstFunc{nodep->fileline(), "__VupdateRandVars", శూన్యసూచిక, శూన్యసూచిక};
        updatep->class_పద్ధతి(నిజం);
        updatep->isVirtual(నిజం);
        updatep->keepAlive(నిజం);
        updatep->needsSyms(అబద్ధం);
        nodep->కూడు_Membersp(updatep);
        nodep->hasRandVarsUpdate(నిజం);

        కొరకు (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : శూన్యసూచిక) {
            Ast_కణుపు_FTask* స్థిరరాశి newp = VN_AS(m_memberMap.కనుగొను_Member(classp, "new"), కణుపు_FTask);
            కొరకు (Ast_కణుపు* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_సమాసం* స్థిరరాశి stmtExprp = VN_CAST(stmtp, Stmt_సమాసం);
                ఒకవేళ (!stmtExprp) కొనసాగు;
                AstCMethodHard* స్థిరరాశి methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                ఒకవేళ (!methodp || methodp->పద్ధతి() != VCMethod::RANDOMIZER_WRITE_VAR) కొనసాగు;

                Ast_కణుపు_Var_Ref* స్థిరరాశి varRefp = VN_AS(methodp->pinsp(), కణుపు_Var_Ref);
                ఒకవేళ (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    కొనసాగు;
                }
                AstCMethodHard* స్థిరరాశి updateMethodp = కొత్త AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_వృక్షం_శుద్ధ(అబద్ధం),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                updateMethodp->dtype_సమితి_Void();
                Ast_కణుపు_సమాసం* స్థిరరాశి namep = VN_AS(varRefp->nextp()->nextp(), కణుపు_సమాసం);
                updateMethodp->కూడు_Pinsp(namep->clone_వృక్షం_శుద్ధ(అబద్ధం));
                updateMethodp->పద్ధతి(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->కూడు_Stmtsp(updateMethodp->makeStmt());
            }
        }
    }

    శూన్య కూడు_Basic_Randomize_Body(AstFunc* స్థిరరాశి basicRandomizep, AstClass* స్థిరరాశి nodep,
                               AstVar* rand_బహుళకం_Varp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        AstVar* స్థిరరాశి basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* స్థిరరాశి basicFvarRefp = కొత్త AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* స్థిరరాశి beginBasicValp = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 1};
        basicRandomizep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, basicFvarRefp, beginBasicValp});
        Ast_కణుపు_FTask* స్థిరరాశి newp = VN_AS(m_memberMap.కనుగొను_Member(nodep, "new"), కణుపు_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            ఒకవేళ (!memberVarp->rand().isRandomizable()) తిరిగి;
            స్థిరరాశి Randomize_బహుళకం rand_బహుళకం = {.asUQuad = memberVarp->user1()};
            ఒకవేళ (rand_బహుళకం.ఉపయోగిస్తుంది_బహుళకం
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* స్థిరరాశి setp = కొత్త AstCMethodHard{
                    nodep->fileline(),
                    కొత్త AstVarRef{fl, VN_AS(rand_బహుళకం_Varp->user2p(), కణుపు_Module), rand_బహుళకం_Varp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, కొత్త AstConst{nodep->fileline(), rand_బహుళకం.సూచీ}};
                setp->dtype_సమితి_UInt32();
                newp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, setp, కొత్త AstConst{fl, 0}});
            }
            ఒకవేళ (memberVarp->user3()) తిరిగి;  // Handled in constraints
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = memberVarp->dtypep()->skipRefp();
            ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(dtypep, ClassRefDType)) {
                ఒకవేళ (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    తిరిగి;
                }
                AstFunc* స్థిరరాశి memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                Ast_పద్ధతి_Call* స్థిరరాశి callp
                    = memberVarp->globalConstrained()
                          ? కొత్త Ast_పద్ధతి_Call{fl,
                                              కొత్త AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : కొత్త Ast_పద్ధతి_Call{
                                fl, కొత్త AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* స్థిరరాశి basicFvarRefReadp = basicFvarRefp->clone_వృక్షం(అబద్ధం);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* స్థిరరాశి కేటాయించు_If_Not_Nullp = కొత్త AstIf{
                    fl,
                    కొత్త AstNeq{fl, కొత్త AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               కొత్త AstConst{fl, AstConst::Null{}}},
                    కొత్త Ast_కేటాయించు{fl, basicFvarRefp->clone_వృక్షం(అబద్ధం),
                                  కొత్త AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->కూడు_Stmtsp(wrap_If_Rand_బహుళకం(nodep, memberVarp, కేటాయించు_If_Not_Nullp));
            } లేకపోతే {
                AstVar* స్థిరరాశి randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* స్థిరరాశి refp
                    = కొత్త AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_కణుపు_Stmt* స్థిరరాశి stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                ఒకవేళ (!refp->backp()) VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
                basicRandomizep->కూడు_Stmtsp(
                    కొత్త AstBegin{fl, m_unique_పేర్లు.get("__VrandBlock"), stmtp, అబద్ధం});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_కణుపు_సమాసం* make_బహుళకం_కేటాయించు_Lhs(File_పంక్తి* స్థిరరాశి fl, AstClass* స్థిరరాశి classp,
                                   Ast_కణుపు_సమాసం* స్థిరరాశి fromp, AstVar* స్థిరరాశి బహుళకం_Varp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        ఒకవేళ (బహుళకం_Varp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            ఒకవేళ (fromp) VL_DO_DANGLING(fromp->unlink_Fr_వెనుక()->delete_వృక్షం(), fromp);
            తిరిగి కొత్త AstVarRef{fl, VN_AS(బహుళకం_Varp->user2p(), కణుపు_Module), బహుళకం_Varp,
                                 VAccess::WRITE};
        } లేకపోతే ఒకవేళ (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            తిరిగి కొత్త AstVarRef{fl, VN_AS(బహుళకం_Varp->user2p(), కణుపు_Module), బహుళకం_Varp,
                                 VAccess::WRITE};
        } లేకపోతే {
            AstMemberSel* స్థిరరాశి memberselp = కొత్త AstMemberSel{fl, fromp->unlink_Fr_వెనుక(), బహుళకం_Varp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            తిరిగి memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    శూన్య replace_With_బహుళకం_కేటాయించు(Ast_కణుపు_FTask_Ref* స్థిరరాశి ftaskRefp, Ast_కణుపు* స్థిరరాశి receiverp,
                               Ast_కణుపు_సమాసం* స్థిరరాశి lhsp) {
        replace_With_బహుళకం_కేటాయించు_And_Append(ftaskRefp, receiverp, lhsp, శూన్యసూచిక);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    శూన్య replace_With_బహుళకం_కేటాయించు_And_Append(Ast_కణుపు_FTask_Ref* స్థిరరాశి ftaskRefp, Ast_కణుపు* స్థిరరాశి receiverp,
                                        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp, Ast_కణుపు* స్థిరరాశి appendStmtp) {
        File_పంక్తి* స్థిరరాశి fl = ftaskRefp->fileline();
        ఒకవేళ (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_సమాసం), ftaskRefp, "Should be a statement");
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = ftaskRefp->argsp()->exprp()->unlink_Fr_వెనుక();
            Ast_కణుపు* newStmtp = శూన్యసూచిక;
            ఒకవేళ (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                //బహుళకం
                స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.ఉపయోగిస్తుంది_బహుళకం, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* స్థిరరాశి setp = కొత్త AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                కొత్త AstConst{fl, rmode.సూచీ}};
                setp->dtype_సమితి_UInt32();
                newStmtp = కొత్త Ast_కేటాయించు{fl, setp, rhsp};
            } లేకపోతే {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = make_బహుళకం_సమితి_Loop(fl, lhsp, rhsp, m_ftaskp);
            }
            ఒకవేళ (appendStmtp) newStmtp->కూడు_తదుపరి(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(నెట్టు_Deletep(m_stmtp), m_stmtp = శూన్యసూచిక);
        } లేకపోతే {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.ఉపయోగిస్తుంది_బహుళకం, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* స్థిరరాశి setp = కొత్త AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            కొత్త AstConst{fl, rmode.సూచీ}};
            setp->dtype_సమితి_UInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(నెట్టు_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    శూన్య wrapRandomizeCallWithNullGuard(Ast_కణుపు_FTask_Ref* nodep) {
        Ast_పద్ధతి_Call* స్థిరరాశి callp = VN_CAST(nodep, పద్ధతి_Call);
        ఒకవేళ (!callp) తిరిగి;
        ఒకవేళ (callp->user4_సమితి_Once()) తిరిగి;
        File_పంక్తి* స్థిరరాశి fl = callp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి checkp
            = కొత్త AstNeq{fl, callp->fromp()->clone_వృక్షం(అబద్ధం), కొత్త AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlink_Fr_వెనుక(&relinker);
        relinker.relink(కొత్త AstCond{fl, checkp, callp, కొత్త AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    శూన్య handle_Randomize_వాదనలు(Ast_కణుపు_FTask_Ref* స్థిరరాశి nodep) {
        ఒకవేళ (!nodep->argsp()) తిరిగి;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        తార్కిక hasNullArg = అబద్ధం;
        కొరకు (AstArg *argp = nodep->argsp(), *nextp = శూన్యసూచిక; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->అంకె().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = నిజం;
                VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        కొరకు (AstArg *argp = nodep->argsp(), *nextp = శూన్యసూచిక; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            కొరకు (AstArg *otherArgp = nextp, *otherNextp = శూన్యసూచిక; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి prefixp
                    = ముక్క_గమ్యం_Common_ఉపసర్గ(argp->exprp(), otherArgp->exprp())) {
                    ఒకవేళ (prefixp == argp->exprp()) {
                        ఒకవేళ (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlink_Fr_వెనుక()->delete_వృక్షం(), otherArgp);
                        కొనసాగు;
                    }
                }
                ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి prefixp
                    = ముక్క_గమ్యం_Common_ఉపసర్గ(otherArgp->exprp(), argp->exprp())) {
                    ఒకవేళ (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);
                        ఆపు;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::సమితి<AstVar*> saved_Rand_బహుళకం_Varps;
        AstVar* tmpVarps = శూన్యసూచిక;
        Ast_కణుపు* storeStmtsp = శూన్యసూచిక;
        Ast_కణుపు* సమితి_Stmtsp = శూన్యసూచిక;
        Ast_కణుపు_Stmt* restoreStmtsp = శూన్యసూచిక;
        కొరకు (AstArg *argp = nodep->argsp(), *nextp = శూన్యసూచిక; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_కణుపు_సమాసం* exprp = argp->exprp();
            Ast_కణుపు_సమాసం* స్థిరరాశి commonPrefixp = ముక్క_గమ్యం_Common_ఉపసర్గ(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_పంక్తి* స్థిరరాశి fl = argp->fileline();
            అయితే (exprp) {
                ఒకవేళ (commonPrefixp == exprp) ఆపు;
                AstVar* స్థిరరాశి randVarp = getVarFromRef(exprp);
                AstClass* స్థిరరాశి classp = VN_AS(randVarp->user2p(), Class);
                AstVar* స్థిరరాశి rand_బహుళకం_Varp = randVarp->lifetime().isStatic()
                                                 ? get_Static_Rand_బహుళకం_Var(classp)
                                                 : get_Rand_బహుళకం_Var_From_Class(classp);
                UASSERT_OBJ(rand_బహుళకం_Varp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                ఒకవేళ (saved_Rand_బహుళకం_Varps.కనుగొను(rand_బహుళకం_Varp) == saved_Rand_బహుళకం_Varps.చివర()) {
                    AstVar* స్థిరరాశి rand_బహుళకం_Tmp_Varp
                        = make_Tmp_Rand_బహుళకం_Var(exprp, rand_బహుళకం_Varp, storeStmtsp, restoreStmtsp);
                    saved_Rand_బహుళకం_Varps.చొప్పించు(rand_బహుళకం_Varp);
                    tmpVarps = Ast_కణుపు::కూడు_తదుపరి(tmpVarps, rand_బహుళకం_Tmp_Varp);
                }
                స్థిరరాశి Randomize_బహుళకం rand_బహుళకం = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = కొత్త AstCMethodHard{
                    fl, make_బహుళకం_Var_Ref(exprp, rand_బహుళకం_Varp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, కొత్త AstConst{fl, rand_బహుళకం.సూచీ}};
                setp->dtype_సమితి_UInt32();
                సమితి_Stmtsp
                    = Ast_కణుపు::కూడు_తదుపరి(సమితి_Stmtsp, కొత్త Ast_కేటాయించు{fl, setp, కొత్త AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);
        }
        ఒకవేళ (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* లక్ష్యం_Classp = శూన్యసూచిక;
            Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(nodep, పద్ధతి_Call);
            ఒకవేళ (పద్ధతి_Callp) {
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDTypep = పద్ధతి_Callp->fromp()->dtypep();
                స్థిరరాశి AstClassRefDType* స్థిరరాశి crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                లక్ష్యం_Classp = crdtp->classp();
            } లేకపోతే {
                లక్ష్యం_Classp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(లక్ష్యం_Classp, nodep, "randomize(null) target class unresolved");
            AstFunc* స్థిరరాశి checkOnlyFuncp = getCreateRandomizeNullFunc(లక్ష్యం_Classp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        ఒకవేళ (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && సమితి_Stmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlink_Fr_వెనుక(&relinker);
            Ast_కణుపు* స్థిరరాశి stmtsp = storeStmtsp;
            stmtsp->కూడు_తదుపరి(సమితి_Stmtsp);
            stmtsp->కూడు_తదుపరి(m_stmtp);
            stmtsp->కూడు_తదుపరి(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            కూడు_తాత్కాలిక_Varsp(tmpVarps);
        }
    }

    // Create a class method `__Vrandomize_null` that implements the IEEE
    // 1800-2023 18.11 semantic: validate all declared constraints against the
    // current runtime values without assigning new ones. Body is:
    //   1. pre_randomize() -- always (IEEE 1800-2023 18.6.2; 18.11 has no
    //      carve-out for the null case).
    //   2. Compute result:
    //      - no constraints: `fvar = 1` (trivially satisfied; IEEE 18.11.1).
    //      - has constraints: clear solver constraints, re-run
    //        `__Vsetup_constraints`, then `fvar = gen.next_check_only(rng)`.
    //   3. if (fvar) post_randomize() -- IEEE 1800-2023 18.6.3 says
    //      post_randomize is not called when randomize() fails.
    AstFunc* getCreateRandomizeNullFunc(AstClass* స్థిరరాశి classp) {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి name = "__Vrandomize_null";
        ఒకవేళ (AstFunc* స్థిరరాశి existingp = VN_AS(m_memberMap.కనుగొను_Member(classp, name), Func)) {
            తిరిగి existingp;
        }
        File_పంక్తి* స్థిరరాశి fl = classp->fileline();
        AstFunc* స్థిరరాశి funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* స్థిరరాశి fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        కూడు_Pre_Post_Call(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* స్థిరరాశి classGenp = getRandomGenerator(classp);
        ఒకవేళ (!classGenp) {
            funcp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, fvarp, VAccess::WRITE},
                                           కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 1}});
        } లేకపోతే {
            Ast_కణుపు_Module* స్థిరరాశి genModp = VN_AS(classGenp->user2p(), కణుపు_Module);
            funcp->కూడు_Stmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* స్థిరరాశి అమరిక_All_Taskp = get_Create_Constraint_అమరిక_Func(classp, m_memberMap);
            funcp->కూడు_Stmtsp((కొత్త AstTaskRef{fl, అమరిక_All_Taskp})->makeStmt());
            AstCExpr* స్థిరరాశి solverCallp = కొత్త AstCExpr{fl};
            solverCallp->dtype_సమితి_బిట్టు();
            solverCallp->కూడు(కొత్త AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->కూడు(".next_check_only(__Vm_rng)");
            funcp->కూడు_Stmtsp(
                కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(నిజం);
        }

        // 3. post_randomize -- only if result is non-zero
        ఒకవేళ (AstTask* స్థిరరాశి userPostp = కనుగొను_Pre_Post_Task(classp, "post_randomize")) {
            AstTaskRef* స్థిరరాశి callp = కొత్త AstTaskRef{userPostp->fileline(), userPostp};
            funcp->కూడు_Stmtsp(
                కొత్త AstIf{fl, కొత్త AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        తిరిగి funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* lift_Log_If_Chain_గమ్యం_Constraint_If(AstLogIf* logIfp) {
        File_పంక్తి* స్థిరరాశి fl = logIfp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి condp = logIfp->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = logIfp->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు* thenBodyp;
        ఒకవేళ (AstLogIf* స్థిరరాశి innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = lift_Log_If_Chain_గమ్యం_Constraint_If(innerLogIfp);
            VL_DO_DANGLING(నెట్టు_Deletep(innerLogIfp), innerLogIfp);
        } లేకపోతే {
            thenBodyp = కొత్త Ast_Constraint_సమాసం{fl, rhsp};
        }
        తిరిగి కొత్త AstConstraintIf{fl, condp, thenBodyp, శూన్యసూచిక};
    }

    స్థిర తార్కిక distBoundRefsRandVar(స్థిరరాశి Ast_కణుపు* boundp) {
        తిరిగి boundp->exists(
            [](స్థిరరాశి AstVarRef* vrefp) { తిరిగి vrefp->varp()->rand().isRandomizable(); });
    }

    స్థిర తార్కిక dist_Bound_Refs_బహుళకం_Var(స్థిరరాశి Ast_కణుపు* boundp) {
        తిరిగి boundp->exists([](స్థిరరాశి AstVarRef* vrefp) {
            ఒకవేళ (!vrefp->varp()->rand().isRandomizable()) తిరిగి అబద్ధం;
            స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = vrefp->varp()->user1()};
            తిరిగి rmode.ఉపయోగిస్తుంది_బహుళకం;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    స్థిర Ast_కణుపు_సమాసం* newDistRangeMembership(AstDist* distp, స్థిరరాశి AstInsideRange* irp) {
        File_పంక్తి* స్థిరరాశి fl = distp->fileline();
        స్థిరరాశి తార్కిక isSigned = distp->exprp()->isSigned();
        Ast_కణుపు_సమాసం* స్థిరరాశి dist_సమాసం_Gtep = distp->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_కణుపు_సమాసం* స్థిరరాశి dist_సమాసం_Ltep = distp->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        dist_సమాసం_Gtep->user1(నిజం);
        dist_సమాసం_Ltep->user1(నిజం);
        Ast_కణుపు_సమాసం* స్థిరరాశి gep
            = isSigned ? static_cast<Ast_కణుపు_సమాసం*>(
                             కొత్త AstGteS{fl, dist_సమాసం_Gtep, irp->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం)})
                       : static_cast<Ast_కణుపు_సమాసం*>(
                             కొత్త AstGte{fl, dist_సమాసం_Gtep, irp->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం)});
        Ast_కణుపు_సమాసం* స్థిరరాశి lep
            = isSigned ? static_cast<Ast_కణుపు_సమాసం*>(
                             కొత్త AstLteS{fl, dist_సమాసం_Ltep, irp->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం)})
                       : static_cast<Ast_కణుపు_సమాసం*>(
                             కొత్త AstLte{fl, dist_సమాసం_Ltep, irp->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం)});
        gep->user1(నిజం);
        lep->user1(నిజం);
        Ast_కణుపు_సమాసం* స్థిరరాశి andp = కొత్త AstLogAnd{fl, gep, lep};
        andp->user1(నిజం);
        తిరిగి andp;
    }

    నిర్మాణం Dist_పాత్ర final {
        Ast_కణుపు_సమాసం* rangep;  // A single value or an InsideRange
        Ast_కణుపు_సమాసం* బరువు_Exprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::సదిశ<Dist_పాత్ర> collectDistBuckets(AstDist* స్థిరరాశి distp) {
        File_పంక్తి* స్థిరరాశి fl = distp->fileline();
        std::సదిశ<Dist_పాత్ర> buckets;
        కొరకు (Ast_Dist_అంశం* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), Dist_అంశం)) {
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(ditemp->weightp(), Const)) {
                ఒకవేళ (constp->గమ్యం_UQuad() == 0) కొనసాగు;
            }
            Ast_కణుపు_సమాసం* బరువు_Exprp
                = కొత్త AstExtend{fl, ditemp->weightp()->clone_వృక్షం_శుద్ధ(అబద్ధం), 64};
            // := on a range weights every element, so scale by the range size.
            ఒకవేళ (!ditemp->is_సంపూర్ణ()) {
                ఒకవేళ (స్థిరరాశి AstInsideRange* స్థిరరాశి irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    స్థిరరాశి AstConst* స్థిరరాశి lop = VN_CAST(irp->lhsp(), Const);
                    స్థిరరాశి AstConst* స్థిరరాశి hip = VN_CAST(irp->rhsp(), Const);
                    Ast_కణుపు_సమాసం* rangeSizep;
                    ఒకవేళ (lop && hip) {
                        స్థిరరాశి uint64_t range_కొలత = hip->గమ్యం_UQuad() - lop->గమ్యం_UQuad() + 1;
                        rangeSizep = కొత్త AstConst{fl, AstConst::Unsized64{}, range_కొలత};
                    } లేకపోతే {
                        rangeSizep = కొత్త Ast_కూడు{
                            fl, కొత్త AstConst{fl, AstConst::Unsized64{}, 1},
                            కొత్త AstSub{fl,
                                       కొత్త AstExtend{fl, irp->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), 64},
                                       కొత్త AstExtend{fl, irp->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), 64}}};
                    }
                    బరువు_Exprp = కొత్త AstMul{fl, బరువు_Exprp, rangeSizep};
                }
            }
            buckets.నెట్టు_వెనుక({ditemp->rangep(), బరువు_Exprp});
        }
        తిరిగి buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    స్థిర Ast_కణుపు_సమాసం* new_Dist_Membership_పదం(AstDist* distp, Ast_కణుపు_సమాసం* rangep) {
        ఒకవేళ (స్థిరరాశి AstInsideRange* స్థిరరాశి irp = VN_CAST(rangep, InsideRange)) {
            తిరిగి newDistRangeMembership(distp, irp);
        }
        File_పంక్తి* స్థిరరాశి fl = distp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి eqExprp = distp->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        eqExprp->user1(నిజం);
        Ast_కణుపు_సమాసం* స్థిరరాశి eqp = కొత్త AstEq{fl, eqExprp, rangep->clone_వృక్షం_శుద్ధ(అబద్ధం)};
        eqp->user1(నిజం);
        తిరిగి eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_సమాసం* buildDistMembership(AstDist* distp,
                                           స్థిరరాశి std::సదిశ<Dist_పాత్ర>& buckets) {
        File_పంక్తి* స్థిరరాశి fl = distp->fileline();
        Ast_కణుపు_సమాసం* unionExprp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి స్వయంచాలక& పాత్ర : buckets) {
            Ast_కణుపు_సమాసం* స్థిరరాశి memberp = new_Dist_Membership_పదం(distp, పాత్ర.rangep);
            ఒకవేళ (!unionExprp) {
                unionExprp = memberp;
            } లేకపోతే {
                unionExprp = కొత్త AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(నిజం);
            }
        }
        తిరిగి కొత్త Ast_Constraint_సమాసం{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_కణుపు_సమాసం* newGateJoin(File_పంక్తి* fl, Ast_కణుపు_సమాసం* ap, Ast_కణుపు_సమాసం* bp, తార్కిక isAnd) {
        ఒకవేళ (ap->same_వృక్షం(bp)) {
            VL_DO_DANGLING(నెట్టు_Deletep(bp), bp);
            తిరిగి ap;
        }
        ఒకవేళ (VN_IS(bp, Const)) std::మార్పిడి(ap, bp);
        ఒకవేళ (VN_IS(ap, Const)) {
            స్థిరరాశి తార్కిక constWins = VN_AS(ap, Const)->isZero() == isAnd;
            ఒకవేళ (!constWins) std::మార్పిడి(ap, bp);
            VL_DO_DANGLING(నెట్టు_Deletep(bp), bp);
            తిరిగి ap;
        }
        ఒకవేళ (isAnd) తిరిగి కొత్త AstLogAnd{fl, ap, bp};
        తిరిగి కొత్త AstLogOr{fl, ap, bp};
    }
    Ast_కణుపు_సమాసం* newGateOr(File_పంక్తి* fl, Ast_కణుపు_సమాసం* ap, Ast_కణుపు_సమాసం* bp) {
        తిరిగి newGateJoin(fl, ap, bp, అబద్ధం);
    }
    Ast_కణుపు_సమాసం* newGateAnd(File_పంక్తి* fl, Ast_కణుపు_సమాసం* ap, Ast_కణుపు_సమాసం* bp) {
        తిరిగి newGateJoin(fl, ap, bp, నిజం);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_కణుపు_సమాసం* new_బహుళకం_బిట్టు_Read(AstVar* varp, స్థిరరాశి AstMemberSel* mselp, AstVar* rand_బహుళకం_Varp,
                                File_పంక్తి* fl) {
        Ast_కణుపు_సమాసం* arrayp = శూన్యసూచిక;
        ఒకవేళ (varp->lifetime().isStatic()) {
            AstClass* స్థిరరాశి ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* స్థిరరాశి smodep = get_Static_Rand_బహుళకం_Var(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = కొత్త AstVarRef{fl, VN_AS(smodep->user2p(), కణుపు_Module), smodep, VAccess::READ};
        } లేకపోతే ఒకవేళ (mselp) {
            AstVar* స్థిరరాశి memberModep = get_Rand_బహుళకం_Var_From_Class(VN_AS(varp->user2p(), కణుపు_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = కొత్త AstMemberSel{fl, mselp->fromp()->clone_వృక్షం_శుద్ధ(అబద్ధం), memberModep};
        } లేకపోతే {
            UASSERT_OBJ(rand_బహుళకం_Varp, varp, "rand_mode variable without a class mode array");
            arrayp = కొత్త AstVarRef{fl, VN_AS(rand_బహుళకం_Varp->user2p(), కణుపు_Module), rand_బహుళకం_Varp,
                                   VAccess::READ};
        }
        స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* స్థిరరాశి atp
            = కొత్త AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, కొత్త AstConst{fl, rmode.సూచీ}};
        atp->dtype_సమితి_UInt32();
        తిరిగి atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_కణుపు_సమాసం* newVarGate(AstVar* varp, స్థిరరాశి AstMemberSel* mselp, తార్కిక ownerLevel,
                            AstVar* rand_బహుళకం_Varp, File_పంక్తి* fl) {
        ఒకవేళ (!varp->rand().isRandomizable()) {
            తిరిగి కొత్త AstConst{fl, AstConst::బిట్టు_True{}, ownerLevel};
        }
        స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = varp->user1()};
        ఒకవేళ (!rmode.ఉపయోగిస్తుంది_బహుళకం) తిరిగి కొత్త AstConst{fl, AstConst::బిట్టు_True{}};
        తిరిగి new_బహుళకం_బిట్టు_Read(varp, mselp, rand_బహుళకం_Varp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_కణుపు_సమాసం* newAccessGate(స్థిరరాశి Ast_కణుపు* nodep, తార్కిక ownerLevel, AstVar* rand_బహుళకం_Varp,
                               File_పంక్తి* fl) {
        ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి mselp = VN_CAST(nodep, MemberSel)) {
            తిరిగి newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, rand_బహుళకం_Varp, fl),
                              newAccessGate(mselp->fromp(), నిజం, rand_బహుళకం_Varp, fl));
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి refp = VN_CAST(nodep, కణుపు_Var_Ref)) {
            తిరిగి newVarGate(refp->varp(), శూన్యసూచిక, ownerLevel, rand_బహుళకం_Varp, fl);
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Sel* స్థిరరాశి selp = VN_CAST(nodep, కణుపు_Sel)) {
            తిరిగి newAccessGate(selp->fromp(), నిజం, rand_బహుళకం_Varp, fl);
        }
        ఒకవేళ (dist_Bound_Refs_బహుళకం_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        తిరిగి కొత్త AstConst{fl, AstConst::బిట్టు_True{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_కణుపు_సమాసం* newDistGate(స్థిరరాశి Ast_కణుపు* nodep, AstVar* rand_బహుళకం_Varp, File_పంక్తి* fl) {
        ఒకవేళ (VN_IS(nodep, కణుపు_Var_Ref) || VN_IS(nodep, MemberSel)) {
            తిరిగి newAccessGate(nodep, అబద్ధం, rand_బహుళకం_Varp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Sel* స్థిరరాశి selp = VN_CAST(nodep, కణుపు_Sel)) {
            తిరిగి newDistGate(selp->fromp(), rand_బహుళకం_Varp, fl);
        }
        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(nodep, Sel)) {
            తిరిగి newDistGate(selp->fromp(), rand_బహుళకం_Varp, fl);
        }
        ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి selp = VN_CAST(nodep, StructSel)) {
            తిరిగి newDistGate(selp->fromp(), rand_బహుళకం_Varp, fl);
        }
        ఒకవేళ (స్థిరరాశి AstCMethodHard* స్థిరరాశి methodp = VN_CAST(nodep, CMethodHard)) {
            ఒకవేళ (methodp->పద్ధతి() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                తిరిగి newDistGate(methodp->fromp(), rand_బహుళకం_Varp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        ఒకవేళ (స్థిరరాశి AstCond* స్థిరరాశి condp = VN_CAST(nodep, Cond)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి thenGatep = newDistGate(condp->thenp(), rand_బహుళకం_Varp, fl);
            Ast_కణుపు_సమాసం* స్థిరరాశి elseGatep = newDistGate(condp->elsep(), rand_బహుళకం_Varp, fl);
            Ast_కణుపు_సమాసం* armGatep = thenGatep;
            ఒకవేళ (thenGatep->same_వృక్షం(elseGatep)) {
                VL_DO_DANGLING(నెట్టు_Deletep(elseGatep), elseGatep);
            } లేకపోతే {
                armGatep
                    = కొత్త AstCond{fl, condp->condp()->clone_వృక్షం_శుద్ధ(అబద్ధం), thenGatep, elseGatep};
            }
            తిరిగి newGateOr(fl, newDistGate(condp->condp(), rand_బహుళకం_Varp, fl), armGatep);
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Uniop* స్థిరరాశి uopp = VN_CAST(nodep, కణుపు_Uniop)) {
            తిరిగి newDistGate(uopp->lhsp(), rand_బహుళకం_Varp, fl);
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి bopp = VN_CAST(nodep, కణుపు_Biop)) {
            తిరిగి newGateOr(fl, newDistGate(bopp->lhsp(), rand_బహుళకం_Varp, fl),
                             newDistGate(bopp->rhsp(), rand_బహుళకం_Varp, fl));
        }
        ఒకవేళ (dist_Bound_Refs_బహుళకం_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        తిరిగి కొత్త AstConst{fl, AstConst::బిట్టు_True{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_సమాసం* buildFrozenMembership(AstDist* distp,
                                             స్థిరరాశి std::సదిశ<Dist_పాత్ర>& buckets) {
        File_పంక్తి* స్థిరరాశి fl = distp->fileline();
        Ast_కణుపు_సమాసం* unionExprp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి స్వయంచాలక& పాత్ర : buckets) {
            Ast_కణుపు_సమాసం* termp = new_Dist_Membership_పదం(distp, పాత్ర.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            స్థిరరాశి తార్కిక runtime_బరువు
                = పాత్ర.బరువు_Exprp->exists([](స్థిరరాశి Ast_కణుపు_Var_Ref*) { తిరిగి నిజం; });
            ఒకవేళ (runtime_బరువు) {
                AstNeq* స్థిరరాశి nzp = కొత్త AstNeq{fl, పాత్ర.బరువు_Exprp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                               కొత్త AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(నిజం);
                termp = కొత్త AstLogAnd{fl, termp, nzp};
                termp->user1(నిజం);
            }
            ఒకవేళ (unionExprp) {
                unionExprp = కొత్త AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(నిజం);
            } లేకపోతే {
                unionExprp = termp;
            }
        }
        తిరిగి కొత్త Ast_Constraint_సమాసం{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_కణుపు* gateFrozenDist(Ast_కణుపు* chainp, AstDist* distp,
                            స్థిరరాశి std::సదిశ<Dist_పాత్ర>& buckets, AstVar* rand_బహుళకం_Varp) {
        File_పంక్తి* స్థిరరాశి fl = distp->fileline();
        Ast_కణుపు_సమాసం* gatep = newDistGate(distp->exprp(), rand_బహుళకం_Varp, fl);
        ఒకవేళ (VN_IS(gatep, Const)) {
            స్థిరరాశి తార్కిక drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(నెట్టు_Deletep(gatep), gatep);
            ఒకవేళ (drawable) తిరిగి chainp;
            VL_DO_DANGLING(నెట్టు_Deletep(chainp), chainp);
            తిరిగి buildFrozenMembership(distp, buckets);
        }
        తిరిగి కొత్త AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_కణుపు_సమాసం* new_ఏకరీతి_Range_Pick(AstDist* distp, స్థిరరాశి AstInsideRange* irp) {
        File_పంక్తి* స్థిరరాశి fl = distp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి dist_సమాసం_Copyp = distp->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        dist_సమాసం_Copyp->user1(నిజం);
        స్థిరరాశి పూర్ణాంకం dist_వెడల్పు = distp->exprp()->వెడల్పు();
        స్థిరరాశి AstConst* స్థిరరాశి lopC = VN_CAST(irp->lhsp(), Const);
        స్థిరరాశి AstConst* స్థిరరాశి hipC = VN_CAST(irp->rhsp(), Const);
        Ast_కణుపు_సమాసం* rangeSzp;
        ఒకవేళ (lopC && hipC) {
            స్థిరరాశి uint64_t rsz = hipC->గమ్యం_UQuad() - lopC->గమ్యం_UQuad() + 1;
            rangeSzp = కొత్త AstConst{fl, AstConst::Unsized64{}, rsz};
        } లేకపోతే {
            స్థిరరాశి తార్కిక isSigned = irp->lhsp()->isSigned();
            Ast_కణుపు_సమాసం* స్థిరరాశి lo64p
                = isSigned ? static_cast<Ast_కణుపు_సమాసం*>(
                                 కొత్త AstExtendS{fl, irp->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), 64})
                           : static_cast<Ast_కణుపు_సమాసం*>(
                                 కొత్త AstExtend{fl, irp->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), 64});
            Ast_కణుపు_సమాసం* స్థిరరాశి hi64p
                = isSigned ? static_cast<Ast_కణుపు_సమాసం*>(
                                 కొత్త AstExtendS{fl, irp->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), 64})
                           : static_cast<Ast_కణుపు_సమాసం*>(
                                 కొత్త AstExtend{fl, irp->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), 64});
            rangeSzp = కొత్త Ast_కూడు{fl, కొత్త AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  కొత్త AstSub{fl, hi64p, lo64p}};
        }
        Ast_కణుపు_సమాసం* స్థిరరాశి rand64p = కొత్త AstRand{fl, శూన్యసూచిక, అబద్ధం};
        rand64p->dtype_సమితి_UInt64();
        Ast_కణుపు_సమాసం* స్థిరరాశి offsetp
            = కొత్త AstCCast{fl, కొత్త AstModDiv{fl, rand64p, rangeSzp}, dist_వెడల్పు};
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = కొత్త Ast_కూడు{fl, irp->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం), offsetp};
        Ast_కణుపు_సమాసం* స్థిరరాశి eqp = కొత్త AstEq{fl, dist_సమాసం_Copyp, valuep};
        eqp->user1(నిజం);
        తిరిగి eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_కణుపు* build_Weighted_పాత్ర_Chain(AstDist* distp, స్థిరరాశి std::సదిశ<Dist_పాత్ర>& buckets,
                                      AstVar* పాత్ర_Varp,
                                      స్థిరరాశి std::సదిశ<Ast_కణుపు_సమాసం*>& cumSums) {
        File_పంక్తి* స్థిరరాశి fl = distp->fileline();
        Ast_కణుపు_సమాసం* chainp = శూన్యసూచిక;
        కొరకు (పూర్ణాంకం i = static_cast<పూర్ణాంకం>(buckets.కొలత()) - 1; i >= 0; --i) {
            Ast_కణుపు_సమాసం* constraintExprp;
            స్థిరరాశి AstInsideRange* స్థిరరాశి irp = VN_CAST(buckets[i].rangep, InsideRange);
            ఒకవేళ (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } లేకపోతే ఒకవేళ (irp) {
                constraintExprp = new_ఏకరీతి_Range_Pick(distp, irp);
            } లేకపోతే {
                Ast_కణుపు_సమాసం* స్థిరరాశి dist_సమాసం_Copyp = distp->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
                dist_సమాసం_Copyp->user1(నిజం);
                constraintExprp
                    = కొత్త AstEq{fl, dist_సమాసం_Copyp, buckets[i].rangep->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                constraintExprp->user1(నిజం);
            }
            ఒకవేళ (!chainp) {
                chainp = constraintExprp;
            } లేకపోతే {
                Ast_కణుపు_సమాసం* స్థిరరాశి condp
                    = కొత్త AstLte{fl, కొత్త AstVarRef{fl, పాత్ర_Varp, VAccess::READ}, cumSums[i]};
                condp->user1(నిజం);
                chainp = కొత్త AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(నిజం);
            }
        }
        Ast_Constraint_సమాసం* స్థిరరాశి chainExprp = కొత్త Ast_Constraint_సమాసం{fl, chainp};
        chainExprp->isSoft(నిజం);
        తిరిగి chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    శూన్య క్రింది_Dist_Constraints(AstTask* taskp, Ast_కణుపు* constrItemsp, AstVar* rand_బహుళకం_Varp,
                              AstConstraintForeach* foreachp = శూన్యసూచిక) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_కణుపు* foreachTailp = శూన్యసూచిక;
        స్వయంచాలక కూడు_Stmt = [&](Ast_కణుపు* nodep) {
            ఒకవేళ (foreachp) {
                ఒకవేళ (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } లేకపోతే {
                    foreachTailp->కూడు_తదుపరి(nodep);
                    foreachTailp = nodep;
                }
            } లేకపోతే {
                taskp->కూడు_Stmtsp(nodep);
            }
        };

        కొరకు (Ast_కణుపు *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            ఒకవేళ (AstConstraintIf* స్థిరరాశి cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                క్రింది_Dist_Constraints(taskp, cifp->thensp(), rand_బహుళకం_Varp, foreachp);
                ఒకవేళ (cifp->elsesp()) {
                    క్రింది_Dist_Constraints(taskp, cifp->elsesp(), rand_బహుళకం_Varp, foreachp);
                }
                కొనసాగు;
            }
            ఒకవేళ (AstConstraintForeach* స్థిరరాశి cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                క్రింది_Dist_Constraints(taskp, cfep->bodyp(), rand_బహుళకం_Varp, cfep);
                కొనసాగు;
            }

            Ast_Constraint_సమాసం* స్థిరరాశి constrExprp = VN_CAST(itemp, Constraint_సమాసం);
            ఒకవేళ (!constrExprp) కొనసాగు;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            ఒకవేళ (AstLogIf* స్థిరరాశి శిఖరం_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_కణుపు* chainEndp = శిఖరం_Log_Ifp->rhsp();
                అయితే (AstLogIf* స్థిరరాశి innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                ఒకవేళ (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* స్థిరరాశి liftedp = lift_Log_If_Chain_గమ్యం_Constraint_If(శిఖరం_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(నెట్టు_Deletep(constrExprp), constrExprp);
                    క్రింది_Dist_Constraints(taskp, liftedp->thensp(), rand_బహుళకం_Varp, foreachp);
                    కొనసాగు;
                }
            }

            AstDist* స్థిరరాశి distp = VN_CAST(constrExprp->exprp(), Dist);
            ఒకవేళ (!distp) కొనసాగు;

            File_పంక్తి* స్థిరరాశి fl = distp->fileline();

            std::సదిశ<Dist_పాత్ర> buckets = collectDistBuckets(distp);

            ఒకవేళ (buckets.ఖాళీలు()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_సమాసం* స్థిరరాశి truep
                    = కొత్త Ast_Constraint_సమాసం{fl, కొత్త AstConst{fl, AstConst::బిట్టు_True{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(నెట్టు_Deletep(constrExprp), constrExprp);
                కొనసాగు;
            }

            Ast_Constraint_సమాసం* స్థిరరాశి membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_కణుపు_సమాసం* మొత్తం_బరువు_Exprp = శూన్యసూచిక;
            కొరకు (స్వయంచాలక& పాత్ర : buckets) {
                ఒకవేళ (!మొత్తం_బరువు_Exprp) {
                    మొత్తం_బరువు_Exprp = పాత్ర.బరువు_Exprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                } లేకపోతే {
                    మొత్తం_బరువు_Exprp = కొత్త Ast_కూడు{fl, మొత్తం_బరువు_Exprp,
                                                  పాత్ర.బరువు_Exprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    మొత్తం_బరువు_Exprp->dtype_సమితి_UInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            స్థిరరాశి పూర్ణాంకం distId = m_dist_అంకె++;
            స్థిరరాశి std::string మొత్తం_Name = "__Vdist_total" + cvt_గమ్యం_Str(distId);
            AstVar* స్థిరరాశి మొత్తం_Varp
                = కొత్త AstVar{fl, VVarType::BLOCKTEMP, మొత్తం_Name, taskp->కనుగొను_UInt64_DType()};
            మొత్తం_Varp->noSubst(నిజం);
            మొత్తం_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            మొత్తం_Varp->funcLocal(నిజం);
            మొత్తం_Varp->is_అంతర్గత(నిజం);
            కూడు_Stmt(మొత్తం_Varp);
            కూడు_Stmt(
                కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, మొత్తం_Varp, VAccess::WRITE}, మొత్తం_బరువు_Exprp});

            // bucketVar = (rand64() % totalWeight) + 1
            స్థిరరాశి std::string పాత్ర_Name = "__Vdist_bucket" + cvt_గమ్యం_Str(distId);
            AstVar* స్థిరరాశి పాత్ర_Varp
                = కొత్త AstVar{fl, VVarType::BLOCKTEMP, పాత్ర_Name, taskp->కనుగొను_UInt64_DType()};
            పాత్ర_Varp->noSubst(నిజం);
            పాత్ర_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            పాత్ర_Varp->funcLocal(నిజం);
            పాత్ర_Varp->is_అంతర్గత(నిజం);
            కూడు_Stmt(పాత్ర_Varp);

            Ast_కణుపు_సమాసం* స్థిరరాశి randp = కొత్త AstRand{fl, శూన్యసూచిక, అబద్ధం};
            randp->dtype_సమితి_UInt64();
            కూడు_Stmt(కొత్త Ast_కేటాయించు{
                fl, కొత్త AstVarRef{fl, పాత్ర_Varp, VAccess::WRITE},
                కొత్త Ast_కూడు{
                    fl, కొత్త AstConst{fl, AstConst::Unsized64{}, 1},
                    కొత్త AstModDiv{fl, randp, కొత్త AstVarRef{fl, మొత్తం_Varp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::సదిశ<Ast_కణుపు_సమాసం*> cumSums;
            Ast_కణుపు_సమాసం* runningSump = శూన్యసూచిక;
            కొరకు (కొలత_t i = 0; i < buckets.కొలత(); ++i) {
                ఒకవేళ (!runningSump) {
                    runningSump = buckets[i].బరువు_Exprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                } లేకపోతే {
                    runningSump = కొత్త Ast_కూడు{fl, runningSump,
                                             buckets[i].బరువు_Exprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    runningSump->dtype_సమితి_UInt64();
                }
                cumSums.నెట్టు_వెనుక(runningSump->clone_వృక్షం_శుద్ధ(నిజం));
            }

            Ast_కణుపు* chainp = build_Weighted_పాత్ర_Chain(distp, buckets, పాత్ర_Varp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, rand_బహుళకం_Varp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(నెట్టు_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->కూడు_ఇక్కడ_This_As_తదుపరి(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            కొరకు (స్వయంచాలక& పాత్ర : buckets) {
                VL_DO_DANGLING(నెట్టు_Deletep(పాత్ర.బరువు_Exprp), పాత్ర.బరువు_Exprp);
            }
            VL_DO_DANGLING(నెట్టు_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            నెట్టు_Deletep(cumSums.వెనుక());
        }
    }

    స్థిర తార్కిక is_Dyn_Arr_యొక్క_Class_Type_Recurse(స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి refp = dtypep->skipRefp();
        ఒకవేళ (VN_IS(refp, DynArrayDType) || VN_IS(refp, వరుస_DType)) {
            తిరిగి is_Dyn_Arr_యొక్క_Class_Type_Recurse(refp->subDTypep());
        } లేకపోతే ఒకవేళ (VN_IS(refp, ClassRefDType)) {
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    శూన్య కూడు_Array_Elem_Refresh(File_పంక్తి* స్థిరరాశి fl, AstVar* స్థిరరాశి arrVarp, AstFunc* స్థిరరాశి randomizep,
                             AstVar* స్థిరరాశి genp) {
        Ast_కణుపు_Module* స్థిరరాశి genModp = VN_AS(genp->user2p(), కణుపు_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        స్థిరరాశి uint32_t unpackedDims = arrVarp->dtypep()->dimensions(అబద్ధం).రెండవ;
        ఒకవేళ (is_Dyn_Arr_యొక్క_Class_Type_Recurse(arrVarp->dtypep())) {
            ఒకవేళ (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            తిరిగి;
        }
        AstCMethodHard* స్థిరరాశి methodp
            = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_సమితి_Void();

        Ast_కణుపు_Module* స్థిరరాశి classp = VN_AS(arrVarp->user2p(), కణుపు_Module);
        AstVarRef* స్థిరరాశి varRefp = కొత్త AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->కూడు_Pinsp(varRefp);

        స్థిరరాశి కొలత_t వెడల్పు = arrayElementDTypep(arrVarp->dtypep())->వెడల్పు();

        methodp->కూడు_Pinsp(కొత్త AstConst{fl, AstConst::Unsized64{}, వెడల్పు});
        Ast_కణుపు_సమాసం* స్థిరరాశి varnamep
            = కొత్త AstCExpr{fl, AstCExpr::శుద్ధ{}, "\"" + arrVarp->name() + "\"", arrVarp->వెడల్పు()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->కూడు_Pinsp(varnamep);
        methodp->కూడు_Pinsp(కొత్త AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = arrVarp->user1()};
        ఒకవేళ (rmode.ఉపయోగిస్తుంది_బహుళకం) {
            methodp->కూడు_Pinsp(కొత్త AstConst{fl, AstConst::Unsized64{}, rmode.సూచీ});
        }

        randomizep->కూడు_Stmtsp(methodp->makeStmt());
    }

    Ast_కణుపు_Stmt* new_Pin_కొలత_Variable(File_పంక్తి* స్థిరరాశి fl, AstVar* స్థిరరాశి arrVarp,
                                    AstVar* స్థిరరాశి genp) {
        Ast_కణుపు_Module* స్థిరరాశి genModp = VN_AS(genp->user2p(), కణుపు_Module);
        AstVar* స్థిరరాశి కొలత_Varp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* స్థిరరాశి pinp
            = కొత్త AstCMethodHard{fl, కొత్త AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtype_సమితి_Void();
        AstCExpr* స్థిరరాశి namep = కొత్త AstCExpr{fl, AstCExpr::శుద్ధ{}, "\"" + కొలత_Varp->name() + "\""};
        namep->dtype_సమితి_UInt32();
        pinp->కూడు_Pinsp(namep);
        pinp->కూడు_Pinsp(
            కొత్త AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(కొలత_Varp->వెడల్పు())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* స్థిరరాశి కొలత_Var_Refp = కొత్త AstVarRef{fl, కొలత_Varp, VAccess::READ};
        కొలత_Var_Refp->classOrPackagep(VN_AS(కొలత_Varp->user2p(), కణుపు_Module));
        pinp->కూడు_Pinsp(కొలత_Var_Refp);
        తిరిగి pinp->makeStmt();
    }

    శూన్య కూడు_Frozen_కొలత_Pin(File_పంక్తి* స్థిరరాశి fl, AstVar* స్థిరరాశి arrVarp, AstFunc* స్థిరరాశి randomizep,
                          AstVar* స్థిరరాశి genp, AstVar* స్థిరరాశి rand_బహుళకం_Varp) {
        స్థిరరాశి Randomize_బహుళకం rmode = {.asUQuad = arrVarp->user1()};
        ఒకవేళ (!rmode.ఉపయోగిస్తుంది_బహుళకం) తిరిగి;
        AstVar* స్థిరరాశి కొలత_Varp = VN_AS(arrVarp->user4p(), Var);
        Ast_కణుపు_Module* స్థిరరాశి arrClassp = VN_AS(arrVarp->user2p(), కణుపు_Module);
        Ast_కణుపు_Module* స్థిరరాశి కొలత_Classp = VN_AS(కొలత_Varp->user2p(), కణుపు_Module);
        AstVarRef* స్థిరరాశి కొలత_Writep = కొత్త AstVarRef{fl, కొలత_Classp, కొలత_Varp, VAccess::WRITE};
        స్థిరరాశి VCMethod కొలత_పద్ధతి = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_కొలత
                                        : VCMethod::DYN_కొలత;
        AstCMethodHard* స్థిరరాశి currentSizep = కొత్త AstCMethodHard{
            fl, కొత్త AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, కొలత_పద్ధతి};
        currentSizep->dtypep(కొలత_Varp->dtypep());
        randomizep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, కొలత_Writep, currentSizep});
        Ast_కణుపు_సమాసం* స్థిరరాశి modep = new_బహుళకం_బిట్టు_Read(arrVarp, శూన్యసూచిక, rand_బహుళకం_Varp, fl);
        randomizep->కూడు_Stmtsp(
            కొత్త AstIf{fl, కొత్త AstLogNot{fl, modep}, new_Pin_కొలత_Variable(fl, arrVarp, genp)});
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_అంకె);
        VL_RESTORER_COPY(m_rand_Array_Unique_పేర్లు);
        m_modp = nodep;
        m_rand_Case_అంకె = 0;
        m_rand_Array_Unique_పేర్లు.reset();
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_అంకె);
        VL_RESTORER(m_dist_అంకె);
        m_modp = nodep;
        m_rand_Case_అంకె = 0;
        m_dist_అంకె = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        ఒకవేళ (!nodep->user1()) తిరిగి;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->పునాది_Most_Classp()->needRNG(నిజం);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        స్థిరరాశి తార్కిక basic_మొదటి = classOwnsGlobalConstraint(nodep);

        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        AstFunc* స్థిరరాశి randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* స్థిరరాశి fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* స్థిరరాశి rand_బహుళకం_Varp = get_Rand_బహుళకం_Var_From_Class(nodep);
        కూడు_Pre_Post_Call(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ఒకవేళ (classHasRandClassMembers(nodep)) {
            AstTask* స్థిరరాశి preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->కూడు_Stmtsp((కొత్త AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_కణుపు_సమాసం* beginValp = శూన్యసూచిక;
        AstVar* genp = getRandomGenerator(nodep);
        ఒకవేళ (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* స్థిరరాశి prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* స్థిరరాశి classp, AstConstraint* స్థిరరాశి constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                ఒకవేళ (!taskp) {
                    taskp = new_అమరిక_Constraint_Task(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                ఒకవేళ (AstTask* స్థిరరాశి resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* స్థిరరాశి resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* స్థిరరాశి resizeTaskRefp
                        = కొత్త AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->కూడు_Stmtsp(resizeTaskRefp->makeStmt());
                }

                ఒకవేళ (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                ఒకవేళ (constrp->itemsp()) {
                    క్రింది_Dist_Constraints(taskp, constrp->itemsp(), rand_బహుళకం_Varp);
                }
                VInsertion_సమితి<AstVar*>& కొలత_Arrays = m_కొలత_Constrained_Arrays[classp];
                Constraint_సమాసం_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), శూన్యసూచిక,
                    genp,          rand_బహుళకం_Varp, m_writtenVars,     &m_uniqueConstraintId,
                    m_unique_పేర్లు, randomizep,   &కొలత_Arrays,       prepareArraysTaskp};
                ఒకవేళ (constrp->itemsp()) {
                    taskp->కూడు_Stmtsp(wrap_If_Constraint_బహుళకం(
                        nodep, constrp, constrp->itemsp()->unlink_Fr_వెనుక_With_తదుపరి()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                స్థిరరాశి స్వయంచాలక constraint_లోతు = [](స్థిరరాశి AstConstraint* constrp) {
                    పూర్ణాంకం లోతు = 0;
                    కొలత_t pos = 0;
                    అయితే ((pos = constrp->name().కనుగొను(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++లోతు;
                        pos += std::అక్షరశ్రేణి_పొడవు(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    తిరిగి లోతు;
                };
                పూర్ణాంకం max_లోతు = 0;
                nodep->foreachMember([&](AstClass* స్థిరరాశి, AstConstraint* స్థిరరాశి constrp) {
                    max_లోతు = std::max(max_లోతు, constraint_లోతు(constrp));
                });
                AstTask* స్థిరరాశి అమరిక_All_Taskp = get_Create_Constraint_అమరిక_Func(nodep, m_memberMap);
                కొరకు (పూర్ణాంకం d = max_లోతు; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* స్థిరరాశి classp, AstConstraint* స్థిరరాశి constrp) {
                            ఒకవేళ (constraint_లోతు(constrp) != d) తిరిగి;
                            AstTask* స్థిరరాశి taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* స్థిరరాశి అమరిక_Task_Refp
                                = కొత్త AstTaskRef{constrp->fileline(), taskp};
                            అమరిక_Task_Refp->classOrPackagep(classp);
                            అమరిక_All_Taskp->కూడు_Stmtsp(అమరిక_Task_Refp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertion_సమితి<AstVar*> కొలత_Array_Vars;
            ఒకవేళ (nodep->extendsp()) {
                AstClass* మాతృ_Classp = nodep->extendsp()->classp();
                అయితే (మాతృ_Classp) {
                    స్థిరరాశి స్వయంచాలక కొలత_Arrays_It = m_కొలత_Constrained_Arrays.కనుగొను(మాతృ_Classp);
                    ఒకవేళ (కొలత_Arrays_It != m_కొలత_Constrained_Arrays.చివర()) {
                        కొరకు (AstVar* స్థిరరాశి arrVarp : కొలత_Arrays_It->రెండవ) {
                            కొలత_Array_Vars.చొప్పించు(arrVarp);
                        }
                    }
                    AstFunc* స్థిరరాశి మాతృ_Randomizep
                        = VN_CAST(m_memberMap.కనుగొను_Member(మాతృ_Classp, "randomize"), Func);
                    ఒకవేళ (మాతృ_Randomizep && మాతృ_Randomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        కొరకు (Ast_కణుపు* stmtp = మాతృ_Randomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            తార్కిక దొరికింది_Clear_Constraints = అబద్ధం;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                ఒకవేళ (methodp->పద్ధతి() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->పద్ధతి() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->కూడు_Stmtsp(stmtp->clone_వృక్షం(అబద్ధం));
                                } లేకపోతే ఒకవేళ (methodp->పద్ధతి()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    దొరికింది_Clear_Constraints = నిజం;
                                }
                            });
                            ఒకవేళ (దొరికింది_Clear_Constraints) ఆపు;
                        }
                    }
                    మాతృ_Classp
                        = మాతృ_Classp->extendsp() ? మాతృ_Classp->extendsp()->classp() : శూన్యసూచిక;
                }
            }
            randomizep->కూడు_Stmtsp(implementConstraintsClear(fl, genp));

            స్థిరరాశి స్వయంచాలక కొలత_Arrays_It = m_కొలత_Constrained_Arrays.కనుగొను(nodep);
            ఒకవేళ (కొలత_Arrays_It != m_కొలత_Constrained_Arrays.చివర()) {
                కొరకు (AstVar* స్థిరరాశి arrVarp : కొలత_Arrays_It->రెండవ) కొలత_Array_Vars.చొప్పించు(arrVarp);
            }
            AstTask* స్థిరరాశి అమరిక_All_Taskp = get_Create_Constraint_అమరిక_Func(nodep, m_memberMap);
            ఒకవేళ (!కొలత_Array_Vars.ఖాళీలు() && prepareArraysTaskp->stmtsp()) {
                Ast_కణుపు_Stmt* స్థిరరాశి preparep = (కొత్త AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                కొరకు (Ast_కణుపు* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    తార్కిక writesArray = అబద్ధం;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->పద్ధతి() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    ఒకవేళ (!writesArray) కొనసాగు;
                    stmtp->కూడు_ఇక్కడ_This_As_తదుపరి(preparep);
                    ఆపు;
                }
            }
            AstTaskRef* స్థిరరాశి అమరిక_Task_Refp = కొత్త AstTaskRef{fl, అమరిక_All_Taskp};
            randomizep->కూడు_Stmtsp(అమరిక_Task_Refp->makeStmt());

            Ast_కణుపు_Module* స్థిరరాశి genModp = VN_AS(genp->user2p(), కణుపు_Module);

            AstCExpr* స్థిరరాశి solverCallp = కొత్త AstCExpr{fl};
            solverCallp->dtype_సమితి_బిట్టు();
            solverCallp->కూడు(కొత్త AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->కూడు(".next(__Vm_rng)");
            ఒకవేళ (!కొలత_Array_Vars.ఖాళీలు()) {
                AstVar* స్థిరరాశి కొలత_Ok_Varp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::బిట్టు)};
                కొలత_Ok_Varp->funcLocal(నిజం);
                randomizep->కూడు_Stmtsp(కొలత_Ok_Varp);

                AstVar* స్థిరరాశి finalOkVarp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::బిట్టు)};
                finalOkVarp->funcLocal(నిజం);
                randomizep->కూడు_Stmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                కొరకు (AstVar* స్థిరరాశి arrVarp : కొలత_Array_Vars)
                    కూడు_Frozen_కొలత_Pin(fl, arrVarp, randomizep, genp, rand_బహుళకం_Varp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->కూడు_Stmtsp(
                    కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, కొలత_Ok_Varp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                ఒకవేళ (AstTask* స్థిరరాశి resizeAllTaskp
                    = VN_AS(m_memberMap.కనుగొను_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* స్థిరరాశి resizeTaskRefp = కొత్త AstTaskRef{fl, resizeAllTaskp};
                    AstIf* స్థిరరాశి ifp = కొత్త AstIf{fl, కొత్త AstVarRef{fl, కొలత_Ok_Varp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), శూన్యసూచిక};
                    randomizep->కూడు_Stmtsp(ifp);
                }

                // Refresh array element tables after resize
                కొరకు (స్థిరరాశి స్వయంచాలక& arrVarp : కొలత_Array_Vars) {
                    కూడు_Array_Elem_Refresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->కూడు_Stmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* స్థిరరాశి అమరిక_Task_Refp2 = కొత్త AstTaskRef{fl, అమరిక_All_Taskp};
                randomizep->కూడు_Stmtsp(అమరిక_Task_Refp2->makeStmt());

                కొరకు (AstVar* స్థిరరాశి arrVarp : కొలత_Array_Vars) {
                    randomizep->కూడు_Stmtsp(new_Pin_కొలత_Variable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* స్థిరరాశి solverCallp2 = కొత్త AstCExpr{fl};
                solverCallp2->dtype_సమితి_బిట్టు();
                solverCallp2->కూడు(కొత్త AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->కూడు(".next(__Vm_rng)");
                randomizep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
                    fl, కొత్త AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = కొత్త AstAnd{fl, కొత్త AstVarRef{fl, కొలత_Ok_Varp, VAccess::READ},
                                       కొత్త AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } లేకపోతే {
                beginValp = solverCallp;
            }
            ఒకవేళ (rand_బహుళకం_Varp) {
                Ast_కణుపు_Module* స్థిరరాశి rand_బహుళకం_Classp = VN_AS(rand_బహుళకం_Varp->user2p(), Class);
                Ast_కణుపు_FTask* స్థిరరాశి newp
                    = VN_AS(m_memberMap.కనుగొను_Member(rand_బహుళకం_Classp, "new"), కణుపు_FTask);
                UASSERT_OBJ(newp, rand_బహుళకం_Classp, "No new() in class");
                కూడు_సమితి_Rand_బహుళకం(newp, genp, rand_బహుళకం_Varp);
            }
            ఒకవేళ (AstVar* స్థిరరాశి static_Rand_బహుళకం_Varp = get_Static_Rand_బహుళకం_Var(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_కణుపు_FTask* స్థిరరాశి newp = VN_AS(m_memberMap.కనుగొను_Member(nodep, "new"), కణుపు_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                కూడు_సమితి_Static_Rand_బహుళకం(newp, genp, static_Rand_బహుళకం_Varp);
            }
        } లేకపోతే {
            beginValp = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 32, 1};
        }

        కూడు_Update_Rand_Vars_Body(nodep, randomizep);

        AstFunc* స్థిరరాశి basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        కూడు_Basic_Randomize_Body(basicRandomizep, nodep, rand_బహుళకం_Varp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* స్థిరరాశి fvarRefp = కొత్త AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
            fl, fvarRefp, basic_మొదటి ? కొత్త AstFuncRef{fl, basicRandomizep} : beginValp});

        స్థిరరాశి స్వయంచాలక కొలత_Arrays_It = m_కొలత_Constrained_Arrays.కనుగొను(nodep);
        స్థిరరాశి తార్కిక needs_కొలత_Phase
            = కొలత_Arrays_It != m_కొలత_Constrained_Arrays.చివర() && !కొలత_Arrays_It->రెండవ.ఖాళీలు();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        స్థిరరాశి స్వయంచాలక emitResizeFallback = [&]() {
            ఒకవేళ (needs_కొలత_Phase) తిరిగి;
            ఒకవేళ (AstTask* స్థిరరాశి resizeAllTaskp
                = VN_AS(m_memberMap.కనుగొను_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* స్థిరరాశి resizeTaskRefp = కొత్త AstTaskRef{fl, resizeAllTaskp};
                randomizep->కూడు_Stmtsp(resizeTaskRefp->makeStmt());
            }
        };
        ఒకవేళ (!basic_మొదటి) emitResizeFallback();

        AstVarRef* స్థిరరాశి fvarRefReadp = fvarRefp->clone_వృక్షం(అబద్ధం);
        fvarRefReadp->access(VAccess::READ);

        Ast_కణుపు_సమాసం* స్థిరరాశి రెండవ_Halfp
            = basic_మొదటి ? beginValp : కొత్త AstFuncRef{fl, basicRandomizep};
        randomizep->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{fl, fvarRefp->clone_వృక్షం(అబద్ధం),
                                            కొత్త AstAnd{fl, fvarRefReadp, రెండవ_Halfp}});

        ఒకవేళ (basic_మొదటి) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ఒకవేళ (classHasRandClassMembers(nodep)) {
            AstTask* స్థిరరాశి postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->కూడు_Stmtsp((కొత్త AstTaskRef{fl, postTaskp})->makeStmt());
        }

        కూడు_Pre_Post_Call(nodep, randomizep, "post_randomize");
        nodep->user1(అబద్ధం);
    }
    శూన్య సందర్శించు(AstRandCase* nodep) override {
        // RANDCASE
        //   CASEITEM expr1 : stmt1
        //   CASEITEM expr2 : stmt2
        // ->
        //   tmp = URandomRange{0, num} + 1   // + 1 so weight 0 means never
        //   if (tmp < expr1) stmt1;
        //   else if (tmp < (expr2 + expr1)) stmt1;
        //   else warning
        // Note this code assumes that the expressions after V3Const are fast to compute
        // Optimize: we would be better with a binary search tree to reduce ifs that execute
        UINFOTREE(9, nodep, "", "rcin:");
        Ast_కణుపు_DType* స్థిరరాశి కూడిక_DTypep = nodep->కనుగొను_UInt64_DType();

        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        స్థిరరాశి std::string name = "__Vrandcase" + cvt_గమ్యం_Str(m_rand_Case_అంకె++);
        AstVar* స్థిరరాశి randVarp = కొత్త AstVar{fl, VVarType::BLOCKTEMP, name, కూడిక_DTypep};
        randVarp->noSubst(నిజం);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        కూడు_తాత్కాలిక_Varsp(randVarp);
        Ast_కణుపు_సమాసం* sump = కొత్త AstConst{fl, AstConst::Widthed_విలువ{}, 64, 0};
        Ast_కణుపు_If* స్థిరరాశి మొదటి_Ifsp
            = కొత్త AstIf{fl, కొత్త AstConst{fl, AstConst::బిట్టు_False{}}, శూన్యసూచిక, శూన్యసూచిక};
        Ast_కణుపు_If* ifsp = మొదటి_Ifsp;

        కొరకు (Ast_Case_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_అంశం)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి condp = itemp->condsp()->unlink_Fr_వెనుక();
            sump
                = కొత్త Ast_కూడు{condp->fileline(), sump, కొత్త AstExtend{itemp->fileline(), condp, 64}};
            Ast_కణుపు* స్థిరరాశి stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి() : శూన్యసూచిక;
            AstVarRef* స్థిరరాశి randVarRefp = కొత్త AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_కణుపు_If* స్థిరరాశి newifp
                = కొత్త AstIf{itemp->fileline(),
                            కొత్త AstLte{condp->fileline(), randVarRefp, sump->clone_వృక్షం_శుద్ధ(నిజం)},
                            stmtsp, శూన్యసూచిక};
            ifsp->కూడు_Elsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* స్థిరరాశి dispp = కొత్త AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            శూన్యసూచిక, శూన్యసూచిక};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->కూడు_Elsesp(dispp);

        Ast_కణుపు_సమాసం* స్థిరరాశి randp = కొత్త AstRand{fl, శూన్యసూచిక, అబద్ధం};
        randp->dtype_సమితి_UInt64();
        AstVarRef* స్థిరరాశి randVarRefp = కొత్త AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_కణుపు* స్థిరరాశి newp
            = కొత్త Ast_కేటాయించు{fl, randVarRefp,
                            కొత్త Ast_కూడు{fl, కొత్త AstConst{fl, AstConst::Unsized64{}, 1},
                                       కొత్త AstModDiv{fl, randp, sump}}};
        newp->కూడు_తదుపరి(మొదటి_Ifsp);
        ఒకవేళ (debug() >= 9) newp->dump_వృక్షం_And_తదుపరి(బయటకు, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (nodep->name() == "rand_mode") {
            Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(nodep, పద్ధతి_Call);
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = పద్ధతి_Callp ? పద్ధతి_Callp->fromp() : శూన్యసూచిక;
            స్థిరరాశి Rand_బహుళకం_లక్ష్యం rand_బహుళకం_లక్ష్యం = Rand_బహుళకం_లక్ష్యం::get(fromp, m_modp);
            UASSERT_OBJ(rand_బహుళకం_లక్ష్యం.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* స్థిరరాశి receiverp = rand_బహుళకం_లక్ష్యం.receiverp;
            స్థిరరాశి తార్కిక isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_కణుపు* classLevelStaticLoopp = శూన్యసూచిక;
            ఒకవేళ (isClassLevel && nodep->argsp()) {
                ఒకవేళ (AstVar* స్థిరరాశి sVarp = get_Static_Rand_బహుళకం_Var(rand_బహుళకం_లక్ష్యం.classp)) {
                    File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                    Ast_కణుపు_సమాసం* స్థిరరాశి staticLhsp
                        = make_బహుళకం_కేటాయించు_Lhs(fl, rand_బహుళకం_లక్ష్యం.classp, శూన్యసూచిక, sVarp);
                    Ast_కణుపు_సమాసం* స్థిరరాశి argClonep = nodep->argsp()->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    classLevelStaticLoopp = make_బహుళకం_సమితి_Loop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            స్థిరరాశి తార్కిక receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* స్థిరరాశి rand_బహుళకం_Varp = receiverIsStaticRand
                                             ? get_Static_Rand_బహుళకం_Var(rand_బహుళకం_లక్ష్యం.classp)
                                             : get_Rand_బహుళకం_Var_From_Class(rand_బహుళకం_లక్ష్యం.classp);
            ఒకవేళ (!rand_బహుళకం_Varp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_సమాసం), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(నెట్టు_Deletep(m_stmtp), m_stmtp = శూన్యసూచిక);
                తిరిగి;
            }
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = make_బహుళకం_కేటాయించు_Lhs(nodep->fileline(), rand_బహుళకం_లక్ష్యం.classp,
                                                        rand_బహుళకం_లక్ష్యం.fromp, rand_బహుళకం_Varp);
            replace_With_బహుళకం_కేటాయించు_And_Append(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : శూన్యసూచిక, lhsp,
                classLevelStaticLoopp);
            తిరిగి;
        }

        ఒకవేళ (nodep->name() == "constraint_mode") {
            Ast_పద్ధతి_Call* స్థిరరాశి పద్ధతి_Callp = VN_CAST(nodep, పద్ధతి_Call);
            Ast_కణుపు_సమాసం* fromp = పద్ధతి_Callp ? పద్ధతి_Callp->fromp() : శూన్యసూచిక;
            AstConstraint* constrp = శూన్యసూచిక;
            AstClass* classp = VN_CAST(m_modp, Class);
            ఒకవేళ (AstConstraintRef* స్థిరరాశి constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                ఒకవేళ (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } లేకపోతే ఒకవేళ (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* స్థిరరాశి constraint_బహుళకం_Varp = (constrp && constrp->isStatic())
                                                   ? get_Static_Constraint_బహుళకం_Var(classp)
                                                   : get_Constraint_బహుళకం_Var(classp);
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp
                = make_బహుళకం_కేటాయించు_Lhs(nodep->fileline(), classp, fromp, constraint_బహుళకం_Varp);
            replace_With_బహుళకం_కేటాయించు(nodep, constrp, lhsp);
            తిరిగి;
        }

        ఒకవేళ (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            తిరిగి;
        }

        ఒకవేళ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            స్థిరరాశి తార్కిక లోపల_Static_Context = m_ftaskp && m_ftaskp->isStatic();
            AstVar* స్థిరరాశి stdrand = createStdRandomGenerator(m_modp);
            AstFunc* స్థిరరాశి randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            ఒకవేళ (లోపల_Static_Context) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(నిజం);
            }
            randomizeFuncp->కూడు_Stmtsp(
                కొత్త Ast_కేటాయించు{nodep->fileline(),
                              కొత్త AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              కొత్త AstConst{nodep->fileline(), AstConst::Widthed_విలువ{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            పూర్ణాంకం argn = 0;
            AstWith* స్థిరరాశి withp = nodep->withp();
            కొరకు (స్థిరరాశి AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి exprp = argp->exprp();
                AstCMethodHard* స్థిరరాశి basicMethodp = కొత్త AstCMethodHard{
                    nodep->fileline(),
                    కొత్త AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* స్థిరరాశి refvarp
                    = కొత్త AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::గమ్యం_string(++argn), exprp->dtypep()};
                refvarp->సమితి_Std_Randomize_Arg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                ఒకవేళ (withp) {
                    withp->foreach([&](Ast_కణుపు_సమాసం* ఘాతాంకం) {
                        ఒకవేళ (is_Similar_కణుపు(ఘాతాంకం, exprp)) {
                            AstVarRef* స్థిరరాశి replaceVar
                                = కొత్త AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            ఘాతాంకం->replaceWith(replaceVar);
                            replaceVar->user1(ఘాతాంకం->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(నెట్టు_Deletep(ఘాతాంకం), ఘాతాంకం);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(నిజం);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->కూడు_Stmtsp(refvarp);

                స్థిరరాశి కొలత_t వెడల్పు = exprp->వెడల్పు();
                basicMethodp->కూడు_Pinsp(
                    కొత్త AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->కూడు_Pinsp(
                    కొత్త AstConst{nodep->fileline(), AstConst::Unsized64{}, వెడల్పు});
                basicMethodp->dtype_సమితి_బిట్టు();

                randomizeFuncp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
                    nodep->fileline(),
                    కొత్త AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    కొత్త AstAnd{nodep->fileline(),
                               కొత్త AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            ఒకవేళ (withp) {
                File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, శూన్యసూచిక,
                                                                లోపల_Static_Context);
                withCapturep->కూడు_Function_Arguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                ఒకవేళ (stdrand) {
                    randomizeFuncp->కూడు_Stmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_కణుపు* స్థిరరాశి capturedTreep = withp->exprp()->unlink_Fr_వెనుక_With_తదుపరి();
                randomizeFuncp->కూడు_Stmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_సమాసం_Visitor{
                        శూన్యసూచిక, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        శూన్యసూచిక, m_writtenVars, &m_uniqueConstraintId, m_unique_పేర్లు,  శూన్యసూచిక};
                }
                AstCExpr* స్థిరరాశి solverCallp = కొత్త AstCExpr{fl};
                solverCallp->dtype_సమితి_బిట్టు();
                solverCallp->కూడు(కొత్త AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->కూడు(".next()");
                AstVar* స్థిరరాశి fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_కణుపు_సమాసం* స్థిరరాశి andExprp
                    = కొత్త AstAnd{fl, కొత్త AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->కూడు_Stmtsp(
                    కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlink_Fr_వెనుక()->delete_వృక్షం(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            ఒకవేళ (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            ఒకవేళ (withCapturep) nodep->కూడు_Argsp(withCapturep->get_వాదనలు());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            తిరిగి;
        }
        handle_Randomize_వాదనలు(nodep);

        AstWith* స్థిరరాశి withp = nodep->withp();
        ఒకవేళ (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            తిరిగి;
        }
        withp->unlink_Fr_వెనుక();
        iterateChildren(nodep);

        AstClass* classp = శూన్యసూచిక;
        ఒకవేళ (Ast_పద్ధతి_Call* స్థిరరాశి callp = VN_CAST(nodep, పద్ధతి_Call)) {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            స్థిరరాశి AstClassRefDType* స్థిరరాశి classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } లేకపోతే {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        ఒకవేళ (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* స్థిరరాశి classGenp = getRandomGenerator(classp);
        AstVar* స్థిరరాశి localGenp
            = కొత్త AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->కనుగొను_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(నిజం);

        AstFunc* స్థిరరాశి randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inline_Unique_పేర్లు.get(nodep), అబద్ధం);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        స్థిరరాశి std::pair<తార్కిక, తార్కిక> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<తార్కిక, తార్కిక>{అబద్ధం, అబద్ధం};
        ఒకవేళ (prePostWrap.మొదటి) {
            కూడు_Virtual_Pre_Post_Call(randomizeFuncp, classp, "pre_randomize");
        } లేకపోతే {
            కూడు_Pre_Post_Call(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ఒకవేళ (classHasRandClassMembers(classp)) {
            AstTask* స్థిరరాశి preTaskp = getCreateNestedCallbackTask(classp, "pre");
            ఒకవేళ (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->కూడు_Stmtsp((కొత్త AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        స్థిరరాశి CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.కూడు_Function_Arguments(randomizeFuncp);

        // Add constraints clearing code
        ఒకవేళ (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* స్థిరరాశి mainRandomizep
                = VN_CAST(m_memberMap.కనుగొను_Member(classp, "randomize"), Func);
            ఒకవేళ (mainRandomizep && mainRandomizep->stmtsp()) {
                కొరకు (Ast_కణుపు* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    తార్కిక దొరికింది_Clear_Constraints = అబద్ధం;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        ఒకవేళ (methodp->పద్ధతి() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->పద్ధతి() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->కూడు_Stmtsp(stmtp->clone_వృక్షం(అబద్ధం));
                        } లేకపోతే ఒకవేళ (methodp->పద్ధతి() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            దొరికింది_Clear_Constraints = నిజం;
                        }
                    });
                    ఒకవేళ (దొరికింది_Clear_Constraints) ఆపు;
                }
            }
            randomizeFuncp->కూడు_Stmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->కూడు_Stmtsp(localGenp);

        AstFunc* స్థిరరాశి basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* స్థిరరాశి basicRandomizeFuncCallp
            = కొత్త AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        ఒకవేళ (classGenp) {
            AstTask* స్థిరరాశి constr_అమరిక_Funcp = get_Create_Constraint_అమరిక_Func(classp, m_memberMap);
            AstTaskRef* స్థిరరాశి callp = కొత్త AstTaskRef{nodep->fileline(), constr_అమరిక_Funcp};
            randomizeFuncp->కూడు_Stmtsp(callp->makeStmt());
            randomizeFuncp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
                nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                కొత్త AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), కణుపు_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* స్థిరరాశి rand_బహుళకం_Varp = get_Rand_బహుళకం_Var_From_Class(classp);
        ఒకవేళ (!classGenp && rand_బహుళకం_Varp) కూడు_సమితి_Rand_బహుళకం(randomizeFuncp, localGenp, rand_బహుళకం_Varp);
        ఒకవేళ (!classGenp) {
            ఒకవేళ (AstVar* స్థిరరాశి sVarp = get_Static_Rand_బహుళకం_Var(classp)) {
                కూడు_సమితి_Static_Rand_బహుళకం(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_కణుపు* స్థిరరాశి capturedTreep = withp->exprp()->unlink_Fr_వెనుక_With_తదుపరి();
        randomizeFuncp->కూడు_Stmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_కణుపు* inlineResizeStmtsp = శూన్యసూచిక;
        {
            std::సదిశ<AstCMethodHard*> కొలత_Methodps;
            capturedTreep->foreach_And_తదుపరి([&](AstCMethodHard* methodp) {
                ఒకవేళ (methodp->పద్ధతి() == VCMethod::DYN_కొలత
                    || methodp->పద్ధతి() == VCMethod::ASSOC_కొలత) {
                    కొలత_Methodps.నెట్టు_వెనుక(methodp);
                }
            });
            కొరకు (AstCMethodHard* స్థిరరాశి methodp : కొలత_Methodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = శూన్యసూచిక;
                Ast_కణుపు_సమాసం* స్థిరరాశి మూలం_Exprp = methodp->fromp();
                ఒకవేళ (AstMemberSel* memberSelp = VN_CAST(మూలం_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    అయితే (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి fromp = memberSelp->fromp();
                    ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి varRefp = VN_CAST(fromp, VarRef)) {
                        ఒకవేళ (varRefp->varp() == captured.getThisp()) కొనసాగు;
                    } లేకపోతే {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి varRefp = VN_CAST(మూలం_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                ఒకవేళ (!arrVarp) కొనసాగు;
                // Only handle rand-declared dynamic/assoc array variables
                ఒకవేళ (!arrVarp->rand().isRandomizable()) కొనసాగు;
                // Do not randomize associative array size (IEEE 18.4)
                ఒకవేళ (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) కొనసాగు;
                File_పంక్తి* స్థిరరాశి fl = methodp->fileline();
                తార్కిక wasCreated = అబద్ధం;
                AstClass* స్థిరరాశి varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* స్థిరరాశి కొలత_Varp = create_Or_Get_కొలత_Var(varClassp, arrVarp, fl,
                                                            methodp->కనుగొను_Int_DType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                ఒకవేళ (wasCreated) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి resizeVarRefp = buildMemberSelExprp(మూలం_Exprp, arrVarp);
                    Ast_కణుపు_సమాసం* స్థిరరాశి కొలత_Exprp = buildMemberSelExprp(మూలం_Exprp, కొలత_Varp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->కూడు_తదుపరి(create_కొలత_Gte_Zero_Constraint(fl, కొలత_Varp, మూలం_Exprp));
                    AstCMethodHard* స్థిరరాశి resizep
                        = కొత్త AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, కొలత_Exprp};
                    resizep->dtypep(methodp->కనుగొను_Void_DType());
                    inlineResizeStmtsp
                        = Ast_కణుపు::కూడు_తదుపరి(inlineResizeStmtsp, కొత్త Ast_Stmt_సమాసం{fl, resizep});
                }
                Ast_కణుపు_సమాసం* స్థిరరాశి కొలత_కణుపు_Exprp = buildMemberSelExprp(మూలం_Exprp, కొలత_Varp);
                methodp->replaceWith(కొలత_కణుపు_Exprp);
                VL_DO_DANGLING(methodp->delete_వృక్షం(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_సమాసం_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                rand_బహుళకం_Varp, m_writtenVars, &m_uniqueConstraintId, m_unique_పేర్లు,  శూన్యసూచిక};
        }

        // Call the solver and set return value
        AstCExpr* స్థిరరాశి solverCallp = కొత్త AstCExpr{nodep->fileline()};
        solverCallp->dtype_సమితి_బిట్టు();
        solverCallp->కూడు(కొత్త AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->కూడు(".next(__Vm_rng)");
        randomizeFuncp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{
            nodep->fileline(),
            కొత్త AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            కొత్త AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        ఒకవేళ (inlineResizeStmtsp) {
            AstVar* స్థిరరాశి retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{nodep->fileline(),
                                         కొత్త AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, శూన్యసూచిక};
            randomizeFuncp->కూడు_Stmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ఒకవేళ (classHasRandClassMembers(classp)) {
            AstTask* స్థిరరాశి postTaskp = getCreateNestedCallbackTask(classp, "post");
            ఒకవేళ (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->కూడు_Stmtsp((కొత్త AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        ఒకవేళ (prePostWrap.రెండవ) {
            కూడు_Virtual_Pre_Post_Call(randomizeFuncp, classp, "post_randomize");
        } లేకపోతే {
            కూడు_Pre_Post_Call(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->కూడు_Argsp(captured.get_వాదనలు());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_వృక్షం(), withp);
    }
    శూన్య సందర్శించు(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        ఒకవేళ (m_constraintp && nodep->fromp()->user1()
            && (nodep->పద్ధతి() == VCMethod::ASSOC_కొలత
                || nodep->పద్ధతి() == VCMethod::DYN_కొలత)) {
            AstClass* స్థిరరాశి classp = VN_AS(m_modp, Class);
            AstVarRef* స్థిరరాశి వరుస_Var_Refp = VN_CAST(nodep->fromp(), VarRef);
            ఒకవేళ (!వరుస_Var_Refp) {
                // Warning from ConstraintExprVisitor will be thrown
                తిరిగి;
            }
            AstVar* స్థిరరాశి వరుస_Varp = వరుస_Var_Refp->varp();
            తార్కిక wasCreated = అబద్ధం;
            AstVar* స్థిరరాశి కొలత_Varp
                = create_Or_Get_కొలత_Var(classp, వరుస_Varp, fl, nodep->కనుగొను_Int_DType(), wasCreated);
            ఒకవేళ (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                ఒకవేళ (!VN_IS(వరుస_Varp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    ఒకవేళ (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* స్థిరరాశి కొలత_Var_Refp = కొత్త AstVarRef{
                        fl, VN_AS(కొలత_Varp->user2p(), కణుపు_Module), కొలత_Varp, VAccess::READ};
                    AstCMethodHard* స్థిరరాశి resizep = కొత్త AstCMethodHard{
                        fl, nodep->fromp()->unlink_Fr_వెనుక(), VCMethod::DYN_RESIZE, కొలత_Var_Refp};
                    resizep->dtypep(nodep->కనుగొను_Void_DType());
                    resizerTaskp->కూడు_Stmtsp(కొత్త Ast_Stmt_సమాసం{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->కూడు_Itemsp(create_కొలత_Gte_Zero_Constraint(fl, కొలత_Varp));
            }
            AstVarRef* స్థిరరాశి కొలత_Var_Refp = కొత్త AstVarRef{fl, VN_AS(కొలత_Varp->user2p(), కణుపు_Module),
                                                         కొలత_Varp, VAccess::READ};
            కొలత_Var_Refp->user1(నిజం);
            nodep->replaceWith(కొలత_Var_Refp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన RandomizeVisitor(AstNetlist* nodep)
        : m_inline_Unique_పేర్లు{"__Vrandwith"} {
        createRandomizeClassVars(nodep);
        // Flag local constraint leaves as solver-owned so __VBasicRand skips them.
        // Runs before any class is lowered. Only a randomized class counts, and
        // only a leaf owned by the constraint's own class: a leaf reached through
        // a global constraint stays basic-randomized so a standalone randomize()
        // of its type still randomizes it (issue #7833); a class-typed sub-object
        // is skipped so its own members are still basic-randomized.
        nodep->foreach([&](AstClass* స్థిరరాశి classp) {
            ఒకవేళ (!classp->user1()) తిరిగి;
            classp->foreachMember([&](AstClass* స్థిరరాశి ownerClassp, AstConstraint* స్థిరరాశి constrp) {
                constrp->foreach([&](AstMemberSel* స్థిరరాశి memberSelp) {
                    AstVar* స్థిరరాశి varp = memberSelp->varp();
                    ఒకవేళ (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) తిరిగి;
                    ఒకవేళ (VN_AS(varp->user2p(), కణుపు_Module) != ownerClassp) తిరిగి;
                    ఒకవేళ (!varp->user3()) varp->user3(నిజం);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(నెట్టు_Deletep(constrp->unlink_Fr_వెనుక()), constrp);
        });
    }
    ~RandomizeVisitor() override = అప్రమేయ;
};

//######################################################################
// Randomize method class functions

శూన్య V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        స్థిరరాశి RandomizeMarkVisitor markVisitor{nodep};
        స్థిరరాశి RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_వృక్షం("randomize", 0, dump_వృక్షం_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       స్థిరరాశి std::string& name, తార్కిక allowVirtual,
                                       తార్కిక బిడ్డ_DType) {
    AstFunc* funcp = VN_AS(memberMap.కనుగొను_Member(nodep, name), Func);
    ఒకవేళ (!funcp) {
        v3Global.useRandomizeMethods(నిజం);
        Ast_కణుపు_DType* స్థిరరాశి dtypep
            = బిడ్డ_DType
                  ? కొత్త AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->కనుగొను_బిట్టు_DType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* స్థిరరాశి fvarp = బిడ్డ_DType
                                  ? కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlag_బిడ్డ_DType{}, dtypep}
                                  : కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3_Error_సంకేతం::NORETURN, నిజం);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(నిజం);
        fvarp->funcReturn(నిజం);
        fvarp->direction(VDirection::OUTPUT);
        nodep->కూడు_Membersp(funcp);
        funcp = కొత్త AstFunc{nodep->fileline(), name, శూన్యసూచిక, fvarp};
        ఒకవేళ (!బిడ్డ_DType) funcp->dtypep(dtypep);
        funcp->class_పద్ధతి(నిజం);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->కూడు_Membersp(funcp);
        memberMap.చొప్పించు(nodep, funcp);
    }
    తిరిగి funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_కణుపు_Module* nodep, స్థిరరాశి std::string& name) {
    AstFunc* funcp = శూన్యసూచిక;
    v3Global.useRandomizeMethods(నిజం);
    Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->కనుగొను_బిట్టు_DType(32, 32, VSigning::SIGNED);
    AstVar* స్థిరరాశి fvarp = కొత్త AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(నిజం);
    fvarp->funcReturn(నిజం);
    fvarp->direction(VDirection::OUTPUT);
    funcp = కొత్త AstFunc{nodep->fileline(), name, శూన్యసూచిక, fvarp};
    funcp->dtypep(dtypep);
    ఒకవేళ (VN_IS(nodep, Class)) {
        funcp->class_పద్ధతి(నిజం);
    } లేకపోతే {
        funcp->class_పద్ధతి(నిజం);
        funcp->isStatic(నిజం);
    }
    nodep->కూడు_Stmtsp(funcp);
    తిరిగి funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* స్థిరరాశి basep = nodep->పునాది_Most_Classp();
    AstFunc* funcp = VN_AS(memberMap.కనుగొను_Member(basep, "srandom"), Func);
    ఒకవేళ (!funcp) {
        v3Global.useRandomizeMethods(నిజం);
        Ast_కణుపు_DType* స్థిరరాశి dtypep
            = basep->కనుగొను_బిట్టు_DType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* స్థిరరాశి ivarp = కొత్త AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(నిజం);
        ivarp->direction(VDirection::ఆదానం);
        funcp = కొత్త AstFunc{basep->fileline(), "srandom", ivarp, శూన్యసూచిక};
        funcp->dtypep(basep->కనుగొను_Void_DType());
        funcp->class_పద్ధతి(నిజం);
        funcp->isVirtual(అబద్ధం);
        basep->కూడు_Membersp(funcp);
        memberMap.చొప్పించు(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        ఒకవేళ (basep->name() == "process") {
            funcp->కూడు_Stmtsp(కొత్త AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } లేకపోతే {
            funcp->కూడు_Stmtsp(కొత్త AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(నిజం);
        }
    }
    తిరిగి funcp;
}
