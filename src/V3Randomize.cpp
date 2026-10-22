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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Randomize.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3Container.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3Task.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <queue>
#ଅନ୍ତର୍ଭାଵ <tuple>
#ଅନ୍ତର୍ଭାଵ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

ଗଣନା ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
ସ୍ଥିର AstVar* getRandModeVarFromClass(Ast_ଗଣ୍ଠି_Module* classp) {
    ଯେତେବେଳେ (classp) {
        ଯଦି (classp->user2p()) ଫେରନ୍ତୁ VN_AS(classp->user2p(), Var);
        AstClass* ଅଚଳ cp = VN_CAST(classp, Class);
        ଯଦି (!cp || !cp->extendsp()) ଫେରନ୍ତୁ nullptr;
        classp = cp->extendsp()->classp();
    }
    ଫେରନ୍ତୁ nullptr;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ସ୍ଥିର AstVar* getRandomGenerator(AstClass* ଅଚଳ classp) {
    ଯଦି (classp->user3p()) ଫେରନ୍ତୁ VN_AS(classp->user3p(), Var);
    ଯଦି (classp->extendsp()) ଫେରନ୍ତୁ getRandomGenerator(classp->extendsp()->classp());
    ଫେରନ୍ତୁ nullptr;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ସ୍ଥିର AstTask* getCreateConstraintSetupFunc(AstClass* classp, VMemberMap& memberMap) {
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ name = "__Vsetup_constraints";
    AstTask* setupAllTaskp = VN_AS(memberMap.findMember(classp, name), Task);
    ଯଦି (setupAllTaskp) ଫେରନ୍ତୁ setupAllTaskp;
    setupAllTaskp = ନୂତନ AstTask{classp->fileline(), "__Vsetup_constraints", nullptr};
    setupAllTaskp->classMethod(ସତ୍ୟ);
    setupAllTaskp->isVirtual(ସତ୍ୟ);
    classp->addMembersp(setupAllTaskp);
    memberMap.ଭର୍ତ୍ତି(classp, setupAllTaskp);
    ଫେରନ୍ତୁ setupAllTaskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

ସଂରଚନା RandModeTarget final {
    // MEMBERS
    AstVar* ଅଚଳ receiverp;  // Variable that is the target of the rand_mode() call
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp;  // Expression from which the rand_mode() call originates
    AstClass* ଅଚଳ classp;  // Class in which rand_mode should be set

    // METHODS
    ସ୍ଥିର RandModeTarget get(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_Module* modp) {
        ଯଦି (!fromp) ଫେରନ୍ତୁ {nullptr, nullptr, VN_AS(modp, Class)};
        ଯଦି (AstCMethodHard* ଅଚଳ methodHardp = VN_CAST(fromp, CMethodHard)) {
            ଫେରନ୍ତୁ RandModeTarget::get(methodHardp->fromp(), modp);
        }
        AstVar* receiverp = nullptr;
        AstClass* classp = VN_CAST(modp, Class);
        ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            ଯଦି (receiverp->isRand()) {
                fromp = nullptr;
                ଯଦି (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } ଅନ୍ୟଥା ଯଦି (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            ଯଦି (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        ଯଦି (!receiverp->isRand()) {
            AstClassRefDType* ଅଚଳ classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            ଯଦି (classRefDtp) classp = classRefDtp->classp();
        }
        ଫେରନ୍ତୁ {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

ସଂଘ RandomizeMode final {
    // MEMBERS
    ସଂରଚନା {
        ତାର୍କିକ usesMode : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t index : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* arrayElementDTypep(Ast_ଗଣ୍ଠି_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ଯେତେବେଳେ (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    ଫେରନ୍ତୁ dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ସ୍ଥିର ତାର୍କିକ dtypeContainsReal(Ast_ଗଣ୍ଠି_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ subp = dtypep->subDTypep()) ଫେରନ୍ତୁ dtypeContainsReal(subp);
    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ structp = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
        ଯଦି (structp->packed()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ପାଇଁ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            ଯଦି (dtypeContainsReal(memberp->subDTypep())) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଅଚଳ AstBasicDType* ଅଚଳ basicp = VN_CAST(dtypep, BasicDType);
    ଫେରନ୍ତୁ basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
ସ୍ଥିର ଶୂନ୍ୟ checkRandTypeEligibility(Ast_ଗଣ୍ଠି* contextp, Ast_ଗଣ୍ଠି_DType* dtypep, ତାର୍କିକ isRandc) {
    dtypep = dtypep->skipRefp();
    ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        ଫେରନ୍ତୁ;
    }
    ଯଦି (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        ଯଦି (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        ଫେରନ୍ତୁ;
    }
    ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        ଫେରନ୍ତୁ;
    }
    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_UOr_Struct_DType* ଅଚଳ structp = VN_CAST(dtypep, ଗଣ୍ଠି_UOr_Struct_DType)) {
        ଯଦି (structp->packed()) ଫେରନ୍ତୁ;  // Integral by construction; members checked separately
        ଯଦି (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            ଫେରନ୍ତୁ;
        }
        ପାଇଁ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            ଯଦି (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        ଫେରନ୍ତୁ;
    }
    ଅଚଳ AstBasicDType* ଅଚଳ basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    ଯଦି (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        ଯଦି (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        ଫେରନ୍ତୁ;
    }
    ବାଛ (basicp->keyword()) {
    ସ୍ଥିତି VBasicDTypeKwd::STRING:
    ସ୍ଥିତି VBasicDTypeKwd::CHANDLE:
    ସ୍ଥିତି VBasicDTypeKwd::EVENT: {
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        ଭାଙ୍ଗ;
    }
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଭାଙ୍ଗ;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

ଶ୍ରେଣୀ RandomizeMarkVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
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
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;

    using DerivedSet = std::unordered_set<AstClass*>;
    using BaseToDerivedMap = std::unordered_map<ଅଚଳ AstClass*, DerivedSet>;

    BaseToDerivedMap m_baseToDerivedMap;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = nullptr;  // Current class
    Ast_ଗଣ୍ଠି* m_constraint_ପଦସମଷ୍ଟି_Genp = nullptr;  // Current constraint or constraint if expression
    Ast_ଗଣ୍ଠି_Module* m_modp;  // Current module
    Ast_ଗଣ୍ଠି_Stmt* m_stmtp = nullptr;  // Current statement
    Ast_ଗଣ୍ଠି_FTask_Ref* m_stdRandCallp = nullptr;  // Current std::randomize() call
    ତାର୍କିକ m_inStdWith = ମିଥ୍ୟା;  // True when inside a 'with {}' clause
    std::set<Ast_ଗଣ୍ଠି_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = nullptr;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    ଶୂନ୍ୟ markGlobalConstrainedVars(AstClass* classp) {
        ପାଇଁ (ଅଚଳ AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ଅଚଳ varp = VN_CAST(memberp, Var);
                ଯଦି (!varp) ଜାରି;
                ଯଦି (varp->rand().isRandomizable()) varp->globalConstrained(ସତ୍ୟ);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    ତାର୍କିକ isVarInStdRandomizeArgs(ଅଚଳ AstVar* varp) ଅଚଳ {
        ଯଦି (!m_inStdWith || !m_stdRandCallp) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ପାଇଁ (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            ଯେତେବେଳେ (exprp) {
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(exprp, ଗଣ୍ଠି_Var_Ref)) {
                    ଯଦି (varrefp->varp() == varp) ଫେରନ୍ତୁ ସତ୍ୟ;
                    ଭାଙ୍ଗ;
                }
                ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ memberselp = VN_CAST(exprp, MemberSel)) {
                    ଯଦି (memberselp->varp() == varp) ଫେରନ୍ତୁ ସତ୍ୟ;
                    exprp = memberselp->fromp();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCMethodHard* ଅଚଳ methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } ଅନ୍ୟଥା {
                    ଭାଙ୍ଗ;
                }
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ markMembers(ଅଚଳ AstClass* nodep) {
        ପାଇଁ (ଅଚଳ AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ଅଚଳ varp = VN_CAST(memberp, Var);
                ଯଦି (!varp) ଜାରି;
                // If member is randomizable and of class type, mark its class
                ଯଦି (varp->rand().isRandomizable()) {
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ varDtypep = varp->dtypep()->skipRefp();
                    ଅଚଳ AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    ଯଦି (!classRefp) {
                        ଅଚଳ Ast_ଗଣ୍ଠି_DType* subDTypep = varDtypep->subDTypep();
                        ଯଦି (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    ଯଦି (classRefp) {
                        AstClass* ଅଚଳ rclassp = classRefp->classp();
                        ଯଦି (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    ଯଦି (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        RandomizeMode randMode = {};
                        randMode.usesMode = ସତ୍ୟ;
                        varp->user1(randMode.asUQuad);
                    }
                }
            }
        }
    }
    ଶୂନ୍ୟ markDerived(ଅଚଳ AstClass* nodep) {
        ଅଚଳ auto it = m_baseToDerivedMap.find(nodep);
        ଯଦି (it != m_baseToDerivedMap.end()) {
            ପାଇଁ (auto* classp : it->second) {
                ଯଦି (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    ଶୂନ୍ୟ markAllDerived() {
        ପାଇଁ (ଅଚଳ auto& p : m_baseToDerivedMap) {
            ଯଦି (p.first->user1()) markDerived(p.first);
        }
    }
    ଶୂନ୍ୟ setPackageRefs() {
        ପାଇଁ (Ast_ଗଣ୍ଠି_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), ଗଣ୍ଠି_Module));
        }
    }
    ଶୂନ୍ୟ markNestedGlobalConstrainedRecurse(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
            AstVar* ଅଚଳ varp = refp->varp();
            ଯଦି (varp->globalConstrained()) ଫେରନ୍ତୁ;
            varp->globalConstrained(ସତ୍ୟ);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ memberSelp = VN_CAST(nodep, MemberSel)) {
            ଯଦି (memberSelp->varp()) {
                AstVar* ଅଚଳ varp = memberSelp->varp();
                ଯଦି (varp->globalConstrained()) ଫେରନ୍ତୁ;
                varp->globalConstrained(ସତ୍ୟ);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildMemberSelChain(AstVarRef* ମୂଳ_Var_Refp, ଅଚଳ std::vector<AstVar*>& path) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = ମୂଳ_Var_Refp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        ପାଇଁ (AstVar* memberVarp : path) {
            AstMemberSel* memberSelp
                = ନୂତନ AstMemberSel{ମୂଳ_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        ଫେରନ୍ତୁ exprp;
    }

    // Process a single constraint during nested constraint cloning
    ଶୂନ୍ୟ processNestedConstraint(AstConstraint* ଅଚଳ constrp, AstVarRef* ମୂଳ_Var_Refp,
                                 ଅଚଳ std::vector<AstVar*>& newPath, AstClass* targetClassp) {
        std::string pathPrefix = ମୂଳ_Var_Refp->name();
        ପାଇଁ (AstVar* pathMemberVarp : newPath) {
            pathPrefix += GLOBAL_CONSTRAINT_SEPARATOR + pathMemberVarp->name();
        }

        ଅଚଳ std::string newName = pathPrefix + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        ତାର୍କିକ isDuplicate = ମିଥ୍ୟା;
        targetClassp->foreachMember([&](AstClass* ଅଚଳ, AstConstraint* ଅଚଳ existingConstrp) {
            ଯଦି (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = ସତ୍ୟ;
            }
        });
        ଯଦି (isDuplicate) ଫେରନ୍ତୁ;

        AstConstraint* ଅଚଳ cloneConstrp = constrp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::vector<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
            ଯଦି (AstVarRef* ଅଚଳ varRefp = VN_CAST(exprp, VarRef)) {
                ଯଦି (!varRefp->varp()->isClassMember()) ଫେରନ୍ତୁ;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ chainp = buildMemberSelChain(ମୂଳ_Var_Refp, newPath);
                AstMemberSel* ଅଚଳ finalSelp
                    = ନୂତନ AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(ଠେଲ_Deletep(varRefp), varRefp);
            } ଅନ୍ୟଥା ଯଦି (AstFuncRef* ଅଚଳ funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                ଯଦି (funcRefp->taskp()->classMethod() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.ଠେଲ_back(funcRefp);
                }
            }
        });
        ପାଇଁ (AstFuncRef* funcRefp : funcRefps) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ chainp = buildMemberSelChain(ମୂଳ_Var_Refp, newPath);
            AstArg* ଅଚଳ argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* ଅଚଳ callp
                = ନୂତନ AstMethodCall{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(ଠେଲ_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        targetClassp->addStmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    ଶୂନ୍ୟ cloneNestedConstraintsRecurse(AstVarRef* ମୂଳ_Var_Refp, AstClass* classp,
                                       ଅଚଳ std::vector<AstVar*>& pathToClass,
                                       AstClass* targetClassp) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ଅଚଳ memberVarp = VN_CAST(memberNodep, Var);
            ଯଦି (!memberVarp) ଜାରି;
            ଯଦି (!memberVarp->rand().isRandomizable()) ଜାରି;
            ଅଚଳ AstClassRefDType* ଅଚଳ memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            ଯଦି (!memberClassRefp || !memberClassRefp->classp()) ଜାରି;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::vector<AstVar*> newPath = pathToClass;
            newPath.ଠେଲ_back(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ଅଚଳ containingClassp, AstConstraint* ଅଚଳ constrp) {
                    processNestedConstraint(constrp, ମୂଳ_Var_Refp, newPath, targetClassp);
                });
            cloneNestedConstraintsRecurse(ମୂଳ_Var_Refp, nestedClassp, newPath, targetClassp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    ଶୂନ୍ୟ handleRandomizeArgument(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstVar* ଅଚଳ varp, ଅଚଳ ତାର୍କିକ stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        ଯେତେବେଳେ (exprp) {
            AstVar* randVarp = nullptr;
            ଯଦି (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } ଅନ୍ୟଥା ଯଦି (AstArraySel* ଅଚଳ arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                ଜାରି;  // Skip ArraySel, continue traversing
            } ଅନ୍ୟଥା ଯଦି (AstStructSel* ଅଚଳ structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                ଜାରି;  // Skip StructSel, continue traversing
            } ଅନ୍ୟଥା ଯଦି (AstCMethodHard* ଅଚଳ methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                ଜାରି;
            } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(ସତ୍ୟ);
                varrefp->access(VAccess::READWRITE);
                exprp = nullptr;
            } ଅନ୍ୟଥା {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            ଯଦି (randVarp == varp) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି* backp = randVarp;
            ଯେତେବେଳେ (backp && !VN_IS(backp, ଗଣ୍ଠି_Module)) backp = backp->backp();
            ଯଦି (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } ଅନ୍ୟଥା {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                RandomizeMode randMode = {};
                randMode.usesMode = ସତ୍ୟ;
                randVarp->user1(randMode.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        ଯଦି (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ଅଚଳ AstClass* ଅଚଳ basep = nodep->extendsp()->classp();
            m_baseToDerivedMap[basep].ଭର୍ତ୍ତି(nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        ଯଦି (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        ଯଦି (nodep->name() == "rand_mode") {
            AstMethodCall* ଅଚଳ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = methodCallp ? methodCallp->fromp() : nullptr;
            ତାର୍କିକ valid = ସତ୍ୟ;
            ଯଦି (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = ମିଥ୍ୟା;
            } ଅନ୍ୟଥା ଯଦି (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = ମିଥ୍ୟା;
            } ଅନ୍ୟଥା ଯଦି (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = ମିଥ୍ୟା;
            } ଅନ୍ୟଥା ଯଦି (AstCMethodHard* ଅଚଳ methodHardp = VN_CAST(fromp, CMethodHard)) {
                ଯଦି (methodHardp->method() == VCMethod::ARRAY_AT
                    || methodHardp->method() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = ମିଥ୍ୟା;
                } ଅନ୍ୟଥା {
                    methodHardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            ଯଦି (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = ମିଥ୍ୟା;
            }
            ଯଦି (valid) {
                ଅଚଳ RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_classp);
                ଯଦି ((!randModeTarget.receiverp || !randModeTarget.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = ମିଥ୍ୟା;
                } ଅନ୍ୟଥା ଯଦି (!randModeTarget.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = ମିଥ୍ୟା;
                } ଅନ୍ୟଥା ଯଦି (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = ମିଥ୍ୟା;
                } ଅନ୍ୟଥା ଯଦି (randModeTarget.receiverp && randModeTarget.receiverp->isRand()) {
                    // Called on a rand member variable
                    RandomizeMode randMode = {};
                    randMode.usesMode = ସତ୍ୟ;
                    randModeTarget.receiverp->user1(randMode.asUQuad);
                } ଅନ୍ୟଥା {
                    // Called on 'this' or a non-rand class instance
                    randModeTarget.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        ଯଦି (!varp->isRand()) ଫେରନ୍ତୁ;
                        RandomizeMode randMode = {};
                        randMode.usesMode = ସତ୍ୟ;
                        varp->user1(randMode.asUQuad);
                    });
                }
            }
            ଯଦି (!valid) {
                ଯଦି (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) {
                    nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                } ଅନ୍ୟଥା {
                    m_stmtp->unlinkFrBack();
                }
            }
            ଫେରନ୍ତୁ;
        }

        ଯଦି (nodep->name() == "constraint_mode") {
            ତାର୍କିକ valid = ସତ୍ୟ;
            ଯଦି (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = ମିଥ୍ୟା;
            } ଅନ୍ୟଥା ଯଦି (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)
                       && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = ମିଥ୍ୟା;
            }
            AstConstraint* constrp = nullptr;
            AstClass* classp = m_classp;
            ଯଦି (ଅଚଳ AstMethodCall* ଅଚଳ methodCallp = VN_CAST(nodep, MethodCall)) {
                ଯଦି (ଅଚଳ AstConstraintRef* ଅଚଳ constrRefp
                    = VN_CAST(methodCallp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    ଯଦି (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } ଅନ୍ୟଥା ଯଦି (AstClassRefDType* classRefDtp
                           = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } ଅନ୍ୟଥା {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = ମିଥ୍ୟା;
                }
            }
            ଯଦି (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = ମିଥ୍ୟା;
            }
            ଯଦି (valid) {
                RandomizeMode constraintMode = {};
                constraintMode.usesMode = ସତ୍ୟ;
                ଯଦି (constrp) {
                    constrp->user1(constraintMode.asUQuad);
                } ଅନ୍ୟଥା {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraintMode.asUQuad);
                    });
                }
            } ଅନ୍ୟଥା {
                ଯଦି (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି)) {
                    nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                } ଅନ୍ୟଥା {
                    m_stmtp->unlinkFrBack();
                }
            }
            ଫେରନ୍ତୁ;
        }

        ଯଦି (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            ଯଦି (m_constraint_ପଦସମଷ୍ଟି_Genp || m_inStdWith) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                ପାଇଁ (Ast_ଗଣ୍ଠି* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    ଯଦି (argp->user1()) {
                        nodep->user1(ସତ୍ୟ);
                        ଭାଙ୍ଗ;
                    }
                }
            }
            ଫେରନ୍ତୁ;
        }
        AstClass* classp = m_classp;
        ଯଦି (ଅଚଳ AstMethodCall* ଅଚଳ methodCallp = VN_CAST(nodep, MethodCall)) {
            ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ classRefp
                = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        ଯଦି (classp) {
            ଯଦି (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ଅଚଳ, AstVar* ଅଚଳ memberVarp) {
                ଯଦି (!memberVarp->rand().isRandomizable()) ଫେରନ୍ତୁ;
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = memberVarp->dtypep()->skipRefp();
                ଅଚଳ AstClassRefDType* ଅଚଳ classRefp = VN_CAST(dtypep, ClassRefDType);
                ଯଦି (!classRefp || !classRefp->classp()) ଫେରନ୍ତୁ;
                AstClass* ଅଚଳ memberClassp = classRefp->classp();
                ଯଦି (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) ଫେରନ୍ତୁ;
                memberVarp->globalConstrained(ସତ୍ୟ);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* ମୂଳ_Var_Refp
                    = ନୂତନ AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::vector<AstVar*> emptyPath;
                memberClassp->foreachMember([&](AstClass* ଅଚଳ, AstConstraint* ଅଚଳ constrp) {
                    processNestedConstraint(constrp, ମୂଳ_Var_Refp, emptyPath, classp);
                });
                cloneNestedConstraintsRecurse(ମୂଳ_Var_Refp, memberClassp, emptyPath, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(ମୂଳ_Var_Refp->delete_ବୃକ୍ଷ(), ମୂଳ_Var_Refp);
            });
        }
        ଯଦି (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = nullptr;
            ପାଇଁ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handleRandomizeArgument(argp->exprp(), nullptr, ସତ୍ୟ);
            }
            ଫେରନ୍ତୁ;
        }
        ପାଇଁ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->ଅଙ୍କ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ଅଚଳ ତାର୍କିକ hasUnsupportedMember
                    = classp->existsMember([](ଅଚଳ AstClass*, ଅଚଳ AstVar* memberVarp) {
                          ଯଦି (!memberVarp->rand().isRandomizable()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                          ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = memberVarp->dtypep()->skipRefp();
                          ଫେରନ୍ତୁ VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, QueueDType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                ଯଦି (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                ଜାରି;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = nullptr;  // If nodep is a method call, this is its receiver
            ଯଦି (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                ଯଦି (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(methodCallp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } ଅନ୍ୟଥା {
                    AstVarRef* ଅଚଳ varrefp = VN_AS(methodCallp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handleRandomizeArgument(argp->exprp(), fromVarp, ମିଥ୍ୟା);
        }
        // Re-mark members after the class became inline-randomized above.
        ଯଦି (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    ଶୂନ୍ୟ visit(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_ପଦସମଷ୍ଟି_Genp);
        m_stmtp = nodep;
        m_constraint_ପଦସମଷ୍ଟି_Genp = nodep;
        iterateChildren(nodep);
        ଯଦି (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Constraint_ପଦସମଷ୍ଟି* nodep) override {
        VL_RESTORER(m_constraint_ପଦସମଷ୍ଟି_Genp);
        m_constraint_ପଦସମଷ୍ଟି_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_ପଦସମଷ୍ଟି_Genp);
            m_constraint_ପଦସମଷ୍ଟି_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterateAndNextConstNull(nodep->thensp());
        iterateAndNextConstNull(nodep->elsesp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (!m_constraint_ପଦସମଷ୍ଟି_Genp) ଫେରନ୍ତୁ;

        ଯଦି (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        ଯଦି (nodep->varp()->rand().isRandomizable() && !(m_inStdWith && m_stdRandCallp)) {
            nodep->user1(ସତ୍ୟ);
        } ଅନ୍ୟଥା ଯଦି (isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(ସତ୍ୟ);
        }
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଯଦି (!m_constraint_ପଦସମଷ୍ଟି_Genp) ଫେରନ୍ତୁ;
        iterateChildrenConst(nodep);
        ଅଚଳ ତାର୍କିକ randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ଅଚଳ ତାର୍କିକ randMember = nodep->varp()->rand().isRandomizable();
        ଅଚଳ ତାର୍କିକ inStdWith = m_inStdWith && m_stdRandCallp;
        ଯଦି (randObject && randMember && !inStdWith) {
            nodep->user1(ସତ୍ୟ);
        } ଅନ୍ୟଥା ଯଦି (inStdWith && isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(ସତ୍ୟ);
            // Mark parent object for constraint expression visitor
            ଯଦି (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(ସତ୍ୟ);
        }

        ଯଦି (m_withp) {
            Ast_ଗଣ୍ଠି* backp = m_withp;
            ଯେତେବେଳେ (backp->backp()) {
                ଯଦି (ଅଚଳ AstMethodCall* ଅଚଳ callp = VN_CAST(backp, MethodCall)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    ଭାଙ୍ଗ;
                }
                backp = backp->backp();
            }
        } ଅନ୍ୟଥା {
            nodep->user2p(m_modp);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        nodep->user2p(m_modp);
        ଯଦି (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        ଯଦି (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_inStdWith = ସତ୍ୟ;
        iterateChildrenConst(nodep);
        m_inStdWith = ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        iterateChildrenConst(nodep);
        ଯଦି (!m_constraint_ପଦସମଷ୍ଟି_Genp && !m_inStdWith) ଫେରନ୍ତୁ;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    ଶୂନ୍ୟ visit(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        ଯଦି (!m_constraint_ପଦସମଷ୍ଟି_Genp && !m_inStdWith) ଫେରନ୍ତୁ;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit RandomizeMarkVisitor(Ast_ଗଣ୍ଠି* nodep) {
        iterateConst(nodep);
        markAllDerived();
        setPackageRefs();
    }
    ~RandomizeMarkVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

ଶ୍ରେଣୀ Constraint_ପଦସମଷ୍ଟି_Visitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ଅଚଳ m_classp;
    Ast_ଗଣ୍ଠି_FTask* ଅଚଳ m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_ଗଣ୍ଠି_FTask* ଅଚଳ m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ଅଚଳ m_genp;  // VlRandomizer variable of the class
    AstVar* m_randModeVarp;  // Relevant randmode state variable
    ତାର୍କିକ m_wantSingle = ମିଥ୍ୟା;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    ତାର୍କିକ m_structSel = ମିଥ୍ୟା;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    ତାର୍କିକ m_deferRandModeHoist = ମିଥ୍ୟା;  // Wait for the complete struct member path
    std::set<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::set<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertionSet<AstVar*>* m_sizeConstrainedArraysp = nullptr;  // Arrays with size constraints
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_conditionp = nullptr;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_ଗଣ୍ଠି_FTask* m_prepareConstrainedArraysp = nullptr;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = nullptr;  // Current ID of unique call
    V3UniqueNames& m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    std::vector<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_ଗଣ୍ଠି* m_firstExpressionInsideIndexp = nullptr;
    AstConstraintForeach* m_foreachp = nullptr;  // Innermost constraint-foreach being processed
    std::set<ଅଚଳ AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    ଶ୍ରେଣୀ NestedAccessPath final {
        AstMemberSel* m_ଶୀର୍ଷ_Nested_Array_Member_Selp
            = nullptr;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = nullptr;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = nullptr;  // Last node of variable's name format
        Ast_ଗଣ୍ଠି** m_firstExpressionInsideIndexPointerp
            = nullptr;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    ସର୍ବସାଧାରଣ:
        ଶୂନ୍ୟ addVarNamePart(AstSFormatF* ଅଚଳ nodep, std::string&& name) {
            ଯଦି (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } ଅନ୍ୟଥା {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.ଭର୍ତ୍ତି(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            ଯଦି (name == "") {
                ଯଦି (*m_firstExpressionInsideIndexPointerp) {
                    (*m_firstExpressionInsideIndexPointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_firstExpressionInsideIndexPointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_firstExpressionInsideIndexPointerp = nodep;
            }
        }

        AstMemberSel* access_ବୃକ୍ଷ() { ଫେରନ୍ତୁ m_ଶୀର୍ଷ_Nested_Array_Member_Selp; }
        ଅଚଳ std::string& smtName() { ଫେରନ୍ତୁ m_smtName; }

        Ast_Stmt_ପଦସମଷ୍ଟି* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ଅଚଳ methodp = ନୂତନ AstCMethodHard{
                varp->fileline(),
                ନୂତନ AstVarRef{varp->fileline(), VN_AS(genp->user2p(), ଗଣ୍ଠି_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtypeSetVoid();

            methodp->addPinsp(m_ଶୀର୍ଷ_Nested_Array_Member_Selp);  // Variable

            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep();  // Width
            ଅଚଳ size_t width = m_ଶୀର୍ଷ_Nested_Array_Member_Selp->varp()->dtypep()->width();
            methodp->addPinsp(ନୂତନ AstConst{dtypep->fileline(), AstConst::Unsized64{}, width});

            methodp->addPinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = nullptr;

            methodp->addPinsp(
                ନୂତନ AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_ଶୀର୍ଷ_Nested_Array_Member_Selp = nullptr;
            ଫେରନ୍ତୁ methodp->makeStmt();
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* cloneName() { ଫେରନ୍ତୁ m_nestedNameFormatTopp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା); }

        ଶୂନ୍ୟ error() {
            VL_DO_DANGLING(m_ଶୀର୍ଷ_Nested_Array_Member_Selp->delete_ବୃକ୍ଷ(), m_ଶୀର୍ଷ_Nested_Array_Member_Selp);
        }

        NestedAccessPath(AstMemberSel* ଶୀର୍ଷ_Member_Selp, Ast_ଗଣ୍ଠି** pointerToFirstExprp)
            : m_ଶୀର୍ଷ_Nested_Array_Member_Selp{ଶୀର୍ଷ_Member_Selp}
            , m_firstExpressionInsideIndexPointerp{pointerToFirstExprp} {}
        ~NestedAccessPath() {
            ଯଦି (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_ବୃକ୍ଷ(),
                            m_nestedNameFormatTopp = nullptr);
                m_nestedNameFormatp = nullptr;
            }
        }
    };
    NestedAccessPath* m_nestedAccess = nullptr;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* createEnumConstraint(File_ଧାଡ଼ି* ଅଚଳ fl, ଅଚଳ std::string& smtName,
                                      AstEnumDType* ଅଚଳ enumDtp, AstVar* ଅଚଳ genVarp) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ genModp = VN_AS(genVarp->user2p(), ଗଣ୍ଠି_Module);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = enumDtp->width();
        std::string constraint = "(__Vbv (or";
        ପାଇଁ (AstEnumItem* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ଅଚଳ AstConst* ଅଚଳ vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvtToStr(vconstp->toUInt()) + " "
                          + cvtToStr(width) + "))";
        }
        constraint += "))";
        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
            fl, ନୂତନ AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "\"" + constraint + "\""}};
        callp->dtypeSetVoid();
        ଫେରନ୍ତୁ callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* findStaticRandModeVarMember(AstClass* classp) ଅଚଳ {
        ଯେତେବେଳେ (ସତ୍ୟ) {
            ଯଦି (AstVar* ଅଚଳ varp
                = VN_CAST(m_memberMap.findMember(classp, "__Vstaticrandmode"), Var)) {
                ଫେରନ୍ତୁ varp;
            }
            AstClassExtends* ଅଚଳ extendsp = classp->extendsp();
            ଯଦି (!extendsp) ଫେରନ୍ତୁ nullptr;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    ଶୂନ୍ୟ markConstrainedRandMember(AstStructDType* ଅଚଳ structp, ଅଚଳ std::string& name) {
        AstMemberDType* ଅଚଳ memberp
            = VN_CAST(m_memberMap.findMember(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(ସତ୍ୟ);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    ତାର୍କିକ isRandQualifiedPath(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯେତେବେଳେ (nodep) {
            ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ selp = VN_CAST(nodep, StructSel)) {
                ଯଦି (AstStructDType* ଅଚଳ structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ଅଚଳ AstMemberDType* ଅଚଳ memberp
                        = VN_CAST(m_memberMap.findMember(structp, selp->name()), MemberDType);
                    ଯଦି (memberp && !memberp->rand().isRandomizable()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                nodep = selp->fromp();
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCMethodHard* ଅଚଳ methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } ଅନ୍ୟଥା {
                ଭାଙ୍ଗ;
            }
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    ସ୍ଥିର ଶୂନ୍ୟ markStructConstrainedRandRecurse(Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        AstStructDType* ଅଚଳ structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        ଯଦି (!structp || structp->packed()) ଫେରନ୍ତୁ;
        structp->markConstrainedRand(ସତ୍ୟ);
        ପାଇଁ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            ଯଦି (!memberp->rand().isRandomizable()) ଜାରି;
            memberp->markConstrainedRand(ସତ୍ୟ);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string buildMemberPath(ଅଚଳ AstMemberSel* ଅଚଳ memberSelp) {
        ଅଚଳ Ast_ଗଣ୍ଠି* fromp = memberSelp->fromp();
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            ଫେରନ୍ତୁ refp->name() + "." + memberSelp->name();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            ଫେରନ୍ତୁ buildMemberPath(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        ଫେରନ୍ତୁ "";
    }

    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getFromp(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nodep) {
        ଯଦି (ଅଚଳ AstCMethodHard* ଅଚଳ cmethp = VN_CAST(nodep, CMethodHard)) {
            ଫେରନ୍ତୁ cmethp->fromp();
        }
        ଫେରନ୍ତୁ getSelFromp(nodep);
    }

    ସ୍ଥିର std::string extractSolveBeforeBaseName(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଯେତେବେଳେ (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = getFromp(exprp)) exprp = fromp;
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ vrefp = VN_CAST(exprp, VarRef)) ଫେରନ୍ତୁ vrefp->name();
        ଫେରନ୍ତୁ "";
    }

    ସ୍ଥିର ଶୂନ୍ୟ buildNamePrefix(AstCExpr* exprp, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nodep) {
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
            exprp->add("(\""s + refp->name());
            ଫେରନ୍ତୁ;
        }

        ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = getFromp(nodep);
        ଯଦି (fromp) buildNamePrefix(exprp, fromp);

        ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ selp = VN_CAST(nodep, StructSel)) {
            exprp->add("." + selp->name());
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCMethodHard* ଅଚଳ cmethp = VN_CAST(nodep, CMethodHard)) {
            ଯଦି (cmethp->method() == VCMethod::ARRAY_AT) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = cmethp->pinsp();
                exprp->add(".\" + ");
                AstCFuncHard* ଅଚଳ solverHexFuncp
                    = ନୂତନ AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(argp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                solverHexFuncp->dtypeSetString();
                exprp->add(solverHexFuncp);
                exprp->add(" + \"");
            } ଅନ୍ୟଥା {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                ଫେରନ୍ତୁ;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ସ୍ଥିର AstCExpr* build_Path_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* ଅଚଳ fl, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep,
                                   ଅଚଳ std::string selName) {
        AstCExpr* ଅଚଳ exprp = ନୂତନ AstCExpr{fl, ""};
        buildNamePrefix(exprp, nodep);
        ଯଦି (selName != "") {
            exprp->add(".");
            exprp->add(selName);
        }
        exprp->add("\")");
        ଫେରନ୍ତୁ exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& baseName,
                                    ଅଚଳ Ast_ଗଣ୍ଠି_Sel* selp) {
        AstCExpr* ଅଚଳ p = ନୂତନ AstCExpr{fl, ""};
        p->add("(\""s + baseName + ".\"");
        p->add("+");
        AstCFuncHard* ଅଚଳ solverHexFuncp
            = ନୂତନ AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
        solverHexFuncp->addPinsp(selp->bitp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
        solverHexFuncp->dtypeSetString();
        p->add(solverHexFuncp);
        p->add(")");
        p->dtypeSetString();
        ଫେରନ୍ତୁ p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getSelFromp(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ mp = VN_CAST(exprp, MemberSel)) ଫେରନ୍ତୁ mp->fromp();
        ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ sp = VN_CAST(exprp, StructSel)) ଫେରନ୍ତୁ sp->fromp();
        ଫେରନ୍ତୁ nullptr;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* build_Solve_Before_Name_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ଅଚଳ p = ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "\"" + varrefp->name() + "\"s"};
            p->dtypeSetString();
            ଫେରନ୍ତୁ p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ selFromp = getSelFromp(exprp)) {
            ଅଚଳ std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string baseName;
                ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    baseName = vp->name();
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    baseName = buildMemberPath(mp);
                }
                ଯଦି (baseName.empty()) ଫେରନ୍ତୁ nullptr;
                AstCExpr* ଅଚଳ p = ନୂତନ AstCExpr{fl, ""};
                p->add("(\""s + baseName + ".\"");
                p->add("+");
                AstCFuncHard* ଅଚଳ solverHexFuncp
                    = ନୂତନ AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(arrSelp->bitp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                solverHexFuncp->dtypeSetString();
                p->add(solverHexFuncp);
                p->add(")");
                p->dtypeSetString();
                ଫେରନ୍ତୁ p;
            }
            // Static member path (obj.field)
            ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ vp = VN_CAST(selFromp, VarRef)) {
                ଅଚଳ std::string path = vp->name() + "." + selName;
                AstCExpr* ଅଚଳ p = ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "\"" + path + "\"s"};
                p->dtypeSetString();
                ଫେରନ୍ତୁ p;
            }
            ଯଦି (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ଅଚଳ p = build_Path_ପଦସମଷ୍ଟି(fl, selFromp, selName);
                p->dtypeSetString();
                ଫେରନ୍ତୁ p;
            }
            ଫେରନ୍ତୁ nullptr;
        }
        ଯଦି (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ଅଚଳ p = build_Path_ପଦସମଷ୍ଟି(fl, exprp, "");
            p->dtypeSetString();
            ଫେରନ୍ତୁ p;
        }
        ଅଚଳ Ast_ଗଣ୍ଠି_Sel* selp = VN_CAST(exprp, ArraySel);
        ଯଦି (!selp) selp = VN_CAST(exprp, AssocSel);
        ଯଦି (selp) {
            // arr[i] -> dynamic name
            std::string baseName;
            ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ vp = VN_CAST(selp->fromp(), VarRef)) {
                baseName = vp->name();
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ mp = VN_CAST(selp->fromp(), MemberSel)) {
                baseName = buildMemberPath(mp);
            }
            ଯଦି (baseName.empty()) ଫେରନ୍ତୁ nullptr;
            ଫେରନ୍ତୁ build_Array_Sel_Name_ପଦସମଷ୍ଟି(fl, baseName, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        ଯଦି (ଅଚଳ AstSel* ଅଚଳ bitSelp = VN_CAST(exprp, Sel)) {
            ଫେରନ୍ତୁ build_Solve_Before_Name_ପଦସମଷ୍ଟି(fl, bitSelp->fromp());
        }
        ଫେରନ୍ତୁ nullptr;
    }

    AstSFormatF* getConstFormat(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଫେରନ୍ତୁ ନୂତନ AstSFormatF{nodep->fileline(), (nodep->width() & 3) ? "#b%b" : "#x%x", ମିଥ୍ୟା,
                               nodep};
    }
    ତାର୍କିକ editFormat(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (nodep->user1()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        handle.relink(getConstFormat(nodep));
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* wrapWithCond(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp,
                              ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
        ଯଦି (condp) {
            File_ଧାଡ଼ି* ଅଚଳ flp = exprp->fileline();
            ଫେରନ୍ତୁ ନୂତନ AstCond{
                flp, condp, exprp,
                getConstFormat(ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, width, 0})};
        }
        ଫେରନ୍ତୁ exprp;
    }
    AstSFormatF* editSMT(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nullptr,
                         Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp = nullptr, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsp = nullptr) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_ପଦସମଷ୍ଟି = nodep->emitSMT();  // Might need child width (AstExtend)
        ଯଦି (smt_ପଦସମଷ୍ଟି == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            ଫେରନ୍ତୁ nullptr;
        }

        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhsWidth = lhsp ? lhsp->width() : 0;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsWidth = rhsp ? rhsp->width() : 0;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା thsWidth = thsp ? thsp->width() : 0;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsCondp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsCondp = nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thsCondp = nullptr;

        ଯଦି (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlinkFrBack() : lhsp),
                         ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            lhsCondp = m_conditionp;
        }
        ଯଦି (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlinkFrBack() : rhsp),
                         ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            rhsCondp = m_conditionp;
        }
        ଯଦି (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlinkFrBack() : thsp),
                         ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            thsCondp = m_conditionp;
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argsp = nullptr;
        ପାଇଁ (string::iterator pos = smt_ପଦସମଷ୍ଟି.begin(); pos != smt_ପଦସମଷ୍ଟି.end(); ++pos) {
            ଯଦି (pos[0] == '%') {
                ++pos;
                ବାଛ (pos[0]) {
                ସ୍ଥିତି '%': ଭାଙ୍ଗ;
                ସ୍ଥିତି 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ଗଣ୍ଠି::addNext(argsp, wrapWithCond(lhsp, lhsCondp, lhsWidth));
                    lhsp = nullptr;
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ଗଣ୍ଠି::addNext(argsp, wrapWithCond(rhsp, rhsCondp, rhsWidth));
                    rhsp = nullptr;
                    ଭାଙ୍ଗ;
                ସ୍ଥିତି 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ଗଣ୍ଠି::addNext(argsp, wrapWithCond(thsp, thsCondp, thsWidth));
                    thsp = nullptr;
                    ଭାଙ୍ଗ;
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); ଭାଙ୍ଗ;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ଅଚଳ newp = ନୂତନ AstSFormatF{nodep->fileline(), smt_ପଦସମଷ୍ଟି, ମିଥ୍ୟା, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଫେରନ୍ତୁ newp;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* editSingle(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* itemsp) {
        ଯଦି (!itemsp) ଫେରନ୍ତୁ nullptr;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = ସତ୍ୟ;

        {
            AstBegin* ଅଚଳ tempp
                = ନୂତନ AstBegin{fl, "[EditWrapper]", itemsp->unlinkFrBackWithNext(), ମିଥ୍ୟା};
            VL_DO_DANGLING(iterateAndNextNull(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            ଯଦି (itemsp) itemsp->unlinkFrBackWithNext();
            VL_DO_DANGLING(tempp->delete_ବୃକ୍ଷ(), tempp);
        }
        ଯଦି (!itemsp) ଫେରନ୍ତୁ nullptr;

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprsp = VN_CAST(itemsp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        ଯଦି (!exprsp->nextp()) ଫେରନ୍ତୁ exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        ଫେରନ୍ତୁ ନୂତନ AstSFormatF{fl, fmt.str(), ମିଥ୍ୟା, exprsp};
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newSel(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* arrayp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ arrDtp = arrayp->unlinkFrBack()->dtypep()->skipRefp();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* selp = nullptr;
        ଯଦି (VN_IS(arrDtp, QueueDType) || VN_IS(arrDtp, DynArrayDType))
            selp = ନୂତନ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        ଅନ୍ୟଥା ଯଦି (VN_IS(arrDtp, UnpackArrayDType))
            selp = ନୂତନ AstArraySel{fl, arrayp, idxp};
        ଅନ୍ୟଥା ଯଦି (VN_IS(arrDtp, AssocArrayDType))
            selp = ନୂତନ AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        ଫେରନ୍ତୁ selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* adjustWidth(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା targetWidth,
                             VSigning targetSigning) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ପଦସମଷ୍ଟି_Width = exprp->width();
        ଯଦି (targetWidth > ପଦସମଷ୍ଟି_Width) {
            // Extend to match target width
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ result = ନୂତନ AstExtend{fl, exprp, targetWidth};
            result->dtypeChgSigned(targetSigning);
            ଫେରନ୍ତୁ result;
        } ଅନ୍ୟଥା ଯଦି (targetWidth < ପଦସମଷ୍ଟି_Width) {
            // Truncate to match target width
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ result = ନୂତନ AstSel{fl, exprp, 0, targetWidth};
            result->dtypeChgSigned(targetSigning);
            ଫେରନ୍ତୁ result;
        } ଅନ୍ୟଥା {
            // Width already matches
            ଫେରନ୍ତୁ exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* extract_Return_ପଦସମଷ୍ଟି(AstFunc* funcp) {
        AstVar* ଅଚଳ retVarp = VN_CAST(funcp->fvarp(), Var);
        ଯଦି (!retVarp) ଫେରନ୍ତୁ nullptr;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* retExprp = nullptr;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା assign_ଗଣନା = 0;
        funcp->foreach([&](AstAssign* assignp) {
            AstVarRef* ଅଚଳ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ଯଦି (!lhsp || lhsp->varp() != retVarp) ଫେରନ୍ତୁ;
            retExprp = assignp->rhsp();
            ++assign_ଗଣନା;
        });
        // Simple function: initializer + return (2) or just return (1)
        ଯଦି (assign_ଗଣନା > 2 || !retExprp) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    ଶୂନ୍ୟ propagateUser1InlineRecurse(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        ଯଦି (VN_IS(nodep, ଗଣ୍ଠି_Var_Ref)) {
            nodep->user1(VN_AS(nodep, ଗଣ୍ଠି_Var_Ref)->varp()->rand().isRandomizable());
            ଫେରନ୍ତୁ;
        }
        ଯଦି (VN_IS(nodep, Const)) {
            nodep->user1(ମିଥ୍ୟା);
            ଫେରନ୍ତୁ;
        }
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା anyChild = ମିଥ୍ୟା;  // Used as bool
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp = VN_CAST(nodep->op1p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp = VN_CAST(nodep->op2p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp = VN_CAST(nodep->op3p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ cp = VN_CAST(nodep->op4p(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        nodep->user1(anyChild);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ସ୍ଥିର Ast_ଗଣ୍ଠି* prependDistPreamble(AstConstraintForeach* nodep, Ast_ଗଣ୍ଠି* bodyp) {
        Ast_ଗଣ୍ଠି* ଅଚଳ preamblep = nodep->user3p();
        ଯଦି (!preamblep) ଫେରନ୍ତୁ bodyp;
        nodep->user3p(nullptr);
        preamblep->addNext(bodyp);
        ଫେରନ୍ତୁ preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    ତାର୍କିକ referencesForeachIdx(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
        ଯଦି (m_foreachIdxVars.empty()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ nodep->exists(
            [&](ଅଚଳ AstVarRef* refp) { ଫେରନ୍ତୁ m_foreachIdxVars.ଗଣନା_2(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    ଶୂନ୍ୟ addRegistrationStmt(Ast_ଗଣ୍ଠି* ଅଚଳ stmtp, Ast_ଗଣ୍ଠି_FTask* ଅଚଳ initTaskp) {
        ଯଦି (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->addBodyp(stmtp);
        } ଅନ୍ୟଥା {
            initTaskp->addStmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_ଧାଡ଼ି* ଅଚଳ fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ arrExprp,
                                       Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ idxExprp) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argsp = Ast_ଗଣ୍ଠି::addNext(arrExprp, idxExprp);
        AstSFormatF* ଅଚଳ formatp = ନୂତନ AstSFormatF{fl, "(select %s %s)", ମିଥ୍ୟା, argsp};
        ଫେରନ୍ତୁ formatp;
    }

    ଶୂନ୍ୟ setRandMode(AstVar* ଅଚଳ varp, AstMemberSel* ଅଚଳ memberselp,
                     ଅଚଳ std::string& smtName, ଅଚଳ RandomizeMode& randMode,
                     Ast_ଗଣ୍ଠି_FTask* ଅଚଳ initTaskp) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ varClassp = VN_AS(varp->user2p(), ଗଣ୍ଠି_Module);
        AstVar* ଅଚଳ subRandModeVarp = getRandModeVarFromClass(varClassp);
        ଯଦି (subRandModeVarp) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ parentAccessp = memberselp->fromp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            AstMemberSel* ଅଚଳ randModeSelp
                = ନୂତନ AstMemberSel{varp->fileline(), parentAccessp, subRandModeVarp};
            randModeSelp->dtypep(subRandModeVarp->dtypep());
            AstCMethodHard* ଅଚଳ atp
                = ନୂତନ AstCMethodHard{varp->fileline(), randModeSelp, VCMethod::ARRAY_AT,
                                     ନୂତନ AstConst{varp->fileline(), randMode.index}};
            atp->dtypeSetUInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ଅଚଳ enablep = ନୂତନ AstCMethodHard{
                varp->fileline(),
                ନୂତନ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtypeSetVoid();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : ନୂତନ AstSFormatF{varp->fileline(), smtName, ମିଥ୍ୟା, nullptr};
            nameExprp->dtypep(varp->dtypep());
            enablep->addPinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ଅଚଳ disablep = ନୂତନ AstCMethodHard{
                varp->fileline(),
                ନୂତନ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_SET_VAR_DISABLED};
            disablep->dtypeSetVoid();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ disnp = nameExprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            disablep->addPinsp(disnp);
            AstIf* ଅଚଳ ifp
                = ନୂତନ AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            addRegistrationStmt(ifp, initTaskp);
        }
    }

    ଶୂନ୍ୟ markRandc(AstVar* ଅଚଳ varp, ଅଚଳ std::string& smtName, Ast_ଗଣ୍ଠି_FTask* ଅଚଳ initTaskp) {
        AstCMethodHard* ଅଚଳ markp = ନୂତନ AstCMethodHard{
            varp->fileline(),
            ନୂତନ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtypeSetVoid();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : ନୂତନ AstSFormatF{varp->fileline(), smtName, ମିଥ୍ୟା, nullptr};
        nameExprp->dtypep(varp->dtypep());
        markp->addPinsp(nameExprp);
        addRegistrationStmt(markp->makeStmt(), initTaskp);
    }

    Ast_ଗଣ୍ଠି_Module* getLeftmostVarModulep(AstMemberSel* ଅଚଳ memberselp,
                                         AstVar* ଅଚଳ varp) ଅଚଳ {
        ଯଦି (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_ଗଣ୍ଠି* ବାମ_Most_Nodep = memberselp->fromp();
            ଯେତେବେଳେ (AstMemberSel* ଅଚଳ nestedMemberSelp = VN_CAST(ବାମ_Most_Nodep, MemberSel)) {
                ବାମ_Most_Nodep = nestedMemberSelp->fromp();
            }
            ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ ମୂଳ_Var_Refp = VN_CAST(ବାମ_Most_Nodep, ଗଣ୍ଠି_Var_Ref)) {
                ଫେରନ୍ତୁ VN_AS(ମୂଳ_Var_Refp->varp()->user2p(), ଗଣ୍ଠି_Module);
            }
            ଫେରନ୍ତୁ VN_AS(memberselp->user2p(), ଗଣ୍ଠି_Module);
        }
        ଫେରନ୍ତୁ VN_AS(varp->user2p(), ଗଣ୍ଠି_Module);
    }

    Ast_ଗଣ୍ଠି_FTask* getInitTaskp(AstVar* ଅଚଳ varp, ଅଚଳ ତାର୍କିକ memberselOrStructSel,
                               Ast_ଗଣ୍ଠି_Module* ଅଚଳ classp) ଅଚଳ {
        ଯଦି (m_inlineInitTaskp) ଫେରନ୍ତୁ m_inlineInitTaskp;
        varp->user3(ସତ୍ୟ);
        ଯଦି (memberselOrStructSel) {
            ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ initTaskp
                = VN_AS(m_memberMap.findMember(classp, "randomize"), ଗଣ୍ଠି_FTask)) {
                ଫେରନ୍ତୁ initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            ଯଦି (m_memberselInitTaskp) ଫେରନ୍ତୁ m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_FTask* ଅଚଳ initTaskp
                = VN_AS(m_memberMap.findMember(classp, "new"), ଗଣ୍ଠି_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            ଫେରନ୍ତୁ initTaskp;
        }
        ଫେରନ୍ତୁ nullptr;
    }

    ଶୂନ୍ୟ createWriteVarForClassFields(AstClass* ଅଚଳ elemClassp, ଅଚଳ std::string& smtName,
                                      AstVar* ଅଚଳ iterVarp, ଅଚଳ ତାର୍କିକ isUnpackedClassRefArray,
                                      Ast_ଗଣ୍ଠି_Module* ଅଚଳ classOrPackagep, AstVar* ଅଚଳ varp,
                                      AstIf* ଅଚଳ ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ଅଚଳ {
        File_ଧାଡ଼ି* ଅଚଳ fl = varp->fileline();
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ varClassp = VN_AS(varp->user2p(), ଗଣ୍ଠି_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା idxWidth = 32;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା fmtWidth = VL_ଶବ୍ଦଗୁଡ଼ିକ_I(idxWidth) * 8;

        ପାଇଁ (ଅଚଳ AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            ପାଇଁ (Ast_ଗଣ୍ଠି* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ଅଚଳ memberVarp = VN_CAST(mnodep, Var);
                ଯଦି (!memberVarp || !memberVarp->rand().isRandomizable()) ଜାରି;
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ memberDtp = memberVarp->dtypep()->skipRefp();
                ଯଦି (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) ଜାରି;
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା memberWidth = memberDtp->width();

                AstCStmt* ଅଚଳ fmtp = ନୂତନ AstCStmt{fl};
                ଯଦି (isUnpackedClassRefArray) {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } ଅନ୍ୟଥା {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::to_string(fmtWidth) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->add(ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->add(");\n");
                ifNonNullp->addThensp(fmtp);

                AstVarRef* ଅଚଳ arrayWrRefp = ନୂତନ AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* atWritep;
                ଯଦି (isUnpackedClassRefArray) {
                    atWritep = ନୂତନ AstArraySel{fl, arrayWrRefp,
                                               ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}};
                } ଅନ୍ୟଥା {
                    atWritep = ନୂତନ AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* ଅଚଳ memberSelp = ନୂତନ AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* ଅଚଳ writeVarp
                    = ନୂତନ AstCMethodHard{fl,
                                         ନୂତନ AstVarRef{fl, VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtypeSetVoid();
                writeVarp->addPinsp(memberSelp);
                writeVarp->addPinsp(
                    ନୂତନ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(memberWidth)});
                AstCExpr* ଅଚଳ nameRefp = ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->addPinsp(nameRefp);
                writeVarp->addPinsp(ନୂତନ AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->addThensp(writeVarp->makeStmt());
            }
        }
    }

    ଶୂନ୍ୟ createSolverArrayHandle(AstVar* ଅଚଳ varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_ଗଣ୍ଠି_Module* ଅଚଳ classOrPackagep,
                                 ଅଚଳ ତାର୍କିକ isUnpackedClassRefArray,
                                 ଅଚଳ std::string& smtName) ଅଚଳ {
        File_ଧାଡ଼ି* ଅଚଳ fl = varp->fileline();
        AstClass* ଅଚଳ elemClassp = elemClassRefDtp->classp();
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ varClassp = VN_AS(varp->user2p(), ଗଣ୍ଠି_Module);

        AstVar* ଅଚଳ iterVarp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__Vi"),
                                            varp->findUInt32DType()};
        iterVarp->funcLocal(ସତ୍ୟ);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp
            = ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, iterVarp, VAccess::WRITE}, ନୂତନ AstConst{fl, 0}};

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sizep;
        ଯଦି (isUnpackedClassRefArray) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା arraySize
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = ନୂତନ AstConst{fl, static_cast<uint32_t>(arraySize)};
        } ଅନ୍ୟଥା {
            AstVarRef* ଅଚଳ arraySizeRefp = ନୂତନ AstVarRef{fl, varClassp, varp, VAccess::READ};
            arraySizeRefp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ଅଚଳ dynSizep
                = ନୂତନ AstCMethodHard{fl, arraySizeRefp, VCMethod::DYN_SIZE, nullptr};
            dynSizep->dtypeSetUInt32();
            sizep = dynSizep;
        }

        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(ନୂତନ AstLoopTest{
            fl, loopp, ନୂତନ AstLt{fl, ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ଅଚଳ arrayReadRefp = ନୂତନ AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* atReadp;
        ଯଦି (isUnpackedClassRefArray) {
            atReadp
                = ନୂତନ AstArraySel{fl, arrayReadRefp, ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}};
        } ଅନ୍ୟଥା {
            atReadp = ନୂତନ AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ଅଚଳ ifNonNullp
            = ନୂତନ AstIf{fl, ନୂତନ AstNeq{fl, atReadp, ନୂତନ AstConst{fl, AstConst::Null{}}}, nullptr};
        loopp->addStmtsp(ifNonNullp);

        AstCStmt* ଅଚଳ bufDeclp = ନୂତନ AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->addThensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->addStmtsp(ନୂତନ AstAssign{
            fl, ନୂତନ AstVarRef{fl, iterVarp, VAccess::WRITE},
            ନୂତନ AstAdd{fl, ନୂତନ AstConst{fl, 1}, ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_FTask* initTaskp = m_inlineInitTaskp;
        ଯଦି (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.findMember(varClassp, "new"), ଗଣ୍ଠି_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->addStmtsp(stmtsp);
    }

    ଶୂନ୍ୟ createSolverVarHandle(AstVar* ଅଚଳ varp, ଅଚଳ ତାର୍କିକ structSelOrCMeth,
                               ଅଚଳ ତାର୍କିକ isGlobalConstrained, ଅଚଳ RandomizeMode randMode,
                               AstMemberSel* ଅଚଳ memberselp, ଅଚଳ std::string& smtName,
                               Ast_ଗଣ୍ଠି_Module* ଅଚଳ classOrPackagep, Ast_ଗଣ୍ଠି_Module* ଅଚଳ classp,
                               Ast_ଗଣ୍ଠି_FTask* ଅଚଳ initTaskp) {
        uint32_t unpackedDims = 0;
        ଯଦି (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(ମିଥ୍ୟା).second;
        }
        ଯଦି (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ଅଚଳ methodp = ନୂତନ AstCMethodHard{
            varp->fileline(),
            ନୂତନ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();
        ଯଦି (memberselp) {
            methodp->addPinsp(memberselp);
        } ଅନ୍ୟଥା {
            AstVarRef* ଅଚଳ varRefp
                = ନୂତନ AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->addPinsp(varRefp);
        }
        ଅଚଳ size_t width = arrayElementDTypep(varp->dtypep())->width();
        methodp->addPinsp(ନୂତନ AstConst{varp->fileline(), AstConst::Unsized64{}, width});
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ varnamep = ନୂତନ AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                   "\"" + smtName + "\"", varp->width()};
        varnamep->dtypep(varp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(
            ନୂତନ AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        ଯଦି (randMode.usesMode && !(isGlobalConstrained && memberselp)) {
            methodp->addPinsp(
                ନୂତନ AstConst{varp->fileline(), AstConst::Unsized64{}, randMode.index});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->addStmtsp(methodp->makeStmt());
        ଯଦି (varp->lifetime().isStatic() && randMode.usesMode) {
            AstCMethodHard* ଅଚଳ markp = ନୂତନ AstCMethodHard{
                varp->fileline(),
                ନୂତନ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ namep = ନୂତନ AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                    "\"" + smtName + "\"", varp->width()};
            namep->dtypep(varp->dtypep());
            markp->addPinsp(namep);
            markp->dtypeSetVoid();
            initTaskp->addStmtsp(markp->makeStmt());
        }
        ଯଦି (isGlobalConstrained && memberselp && randMode.usesMode) {
            setRandMode(varp, memberselp, smtName, randMode, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        ଯଦି (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* createVarExprp(Ast_ଗଣ୍ଠି_Var_Ref* nodep, ଅଚଳ RandomizeMode randMode,
                                ଅଚଳ std::string& smtName, ଅଚଳ ତାର୍କିକ isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ଅଚଳ varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = ନୂତନ AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, ମିଥ୍ୟା, nullptr};
        ଯଦି (!randMode.usesMode) {
            // Global constraints keep nodep alive for write_var processing
            ଯଦି (!isGlobalConstrained) VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* randModeAccess;
        ଅଚଳ ତାର୍କିକ varIsStatic = varp->lifetime().isStatic();
        AstClass* ଅଚଳ varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : nullptr;
        AstVar* ଅଚଳ ownerStaticRandModeVarp
            = varOwningClassp ? findStaticRandModeVarMember(varOwningClassp) : nullptr;
        ଯଦି (ownerStaticRandModeVarp) {
            randModeAccess = ନୂତନ AstVarRef{varp->fileline(),
                                           VN_AS(ownerStaticRandModeVarp->user2p(), ଗଣ୍ଠି_Module),
                                           ownerStaticRandModeVarp, VAccess::READ};
        } ଅନ୍ୟଥା ଯଦି (memberselp) {
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ varClassp = VN_AS(varp->user2p(), ଗଣ୍ଠି_Module);
            AstVar* ଅଚଳ effectiveRandModeVarp = getRandModeVarFromClass(varClassp);
            ଯଦି (effectiveRandModeVarp) {
                // Member's class has randmode, use it
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* parentAccess = memberselp->fromp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                AstMemberSel* randModeSel
                    = ନୂତନ AstMemberSel{varp->fileline(), parentAccess, effectiveRandModeVarp};
                randModeSel->dtypep(effectiveRandModeVarp->dtypep());
                randModeAccess = randModeSel;
            } ଅନ୍ୟଥା {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
                randModeAccess
                    = ନୂତନ AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), ଗଣ୍ଠି_Module),
                                    m_randModeVarp, VAccess::READ};
            }
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
            randModeAccess
                = ନୂତନ AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), ଗଣ୍ଠି_Module),
                                m_randModeVarp, VAccess::READ};
        }

        AstCMethodHard* ଅଚଳ atp
            = ନୂତନ AstCMethodHard{nodep->fileline(), randModeAccess, VCMethod::ARRAY_AT,
                                 ନୂତନ AstConst{nodep->fileline(), randMode.index}};
        atp->dtypeSetUInt32();
        exprp = ନୂତନ AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(ସତ୍ୟ);  // Mark as formatted
        ଫେରନ୍ତୁ exprp;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        ଯଦି (varp->user4p()) {
            ତାର୍କିକ isSizeRef = ମିଥ୍ୟା;
            ଯଦି (AstCMethodHard* ଅଚଳ methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                ଯଦି (methodp->method() == VCMethod::ASSOC_SIZE
                    || methodp->method() == VCMethod::DYN_SIZE) {
                    isSizeRef = ସତ୍ୟ;
                }
            }
            ଯଦି (!isSizeRef && m_sizeConstrainedArraysp) { m_sizeConstrainedArraysp->ଭର୍ତ୍ତି(varp); }
        }

        // Check if this variable is marked as globally constrained
        ଅଚଳ ତାର୍କିକ isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = nullptr;
        ତାର୍କିକ structSelOrCMeth = ମିଥ୍ୟା;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_ଗଣ୍ଠି_DType* selectedDtypep = nullptr;
        std::string smtName;
        ଯଦି (m_nestedAccess) {
            m_nestedAccess->addVarNamePart(
                ନୂତନ AstSFormatF{varp->fileline(), nodep->name(), ମିଥ୍ୟା, nullptr}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_ବୃକ୍ଷ();
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_ଗଣ୍ଠି* ଶୀର୍ଷ_Member_Sel = nodep->backp();
            ଯେତେବେଳେ (VN_IS(ଶୀର୍ଷ_Member_Sel->backp(), MemberSel)) {
                ଶୀର୍ଷ_Member_Sel = ଶୀର୍ଷ_Member_Sel->backp();
            }
            memberselp = VN_AS(ଶୀର୍ଷ_Member_Sel, MemberSel)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            smtName = buildMemberPath(memberselp);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_ଗଣ୍ଠି* ଶୀର୍ଷ_Nodep = nodep->backp();
            ଯେତେବେଳେ (VN_IS(ଶୀର୍ଷ_Nodep->backp(), StructSel) || VN_IS(ଶୀର୍ଷ_Nodep->backp(), CMethodHard)) {
                ଶୀର୍ଷ_Nodep = ଶୀର୍ଷ_Nodep->backp();
            }

            ଯଦି (VN_IS(ଶୀର୍ଷ_Nodep, StructSel) || VN_IS(ଶୀର୍ଷ_Nodep, CMethodHard)) {
                structSelOrCMeth = ସତ୍ୟ;
                selectedDtypep = VN_AS(ଶୀର୍ଷ_Nodep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)->dtypep();
            }

            memberselp = VN_CAST(ଶୀର୍ଷ_Nodep, MemberSel);
            ଯଦି (memberselp) smtName = buildMemberPath(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            ଯଦି (smtName == "") smtName = nodep->name();
        } ଅନ୍ୟଥା {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        ଯଦି (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        ଯଦି (dtypeContainsReal(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            ଫେରନ୍ତୁ;
        }
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ classOrPackagep = nodep->classOrPackagep();
        ଅଚଳ RandomizeMode randMode = {.asUQuad = varp->user1()};
        ଯଦି (!randMode.usesMode && editFormat(nodep)) ଫେରନ୍ତୁ;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skipRefToEnump(), EnumDType);
        ଯଦି (enumDtp
            && (std::find(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.end(), varp)
                == m_rangeConstrainedEnums.end())) {
            AstVar* genVarp = m_genp;
            ଯଦି (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_ଗଣ୍ଠି_FTask* targetTaskp = m_inlineInitTaskp;
            ଯଦି (!targetTaskp) targetTaskp = getCreateConstraintSetupFunc(m_classp, m_memberMap);
            UASSERT_OBJ(targetTaskp, nodep, "No function to inline enum range constraint into");
            targetTaskp->addStmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.ଠେଲ_back(varp);
        }

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp
            = createVarExprp(nodep, randMode, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ଅଚଳ ତାର୍କିକ alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.ଗଣନା_2(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.ଗଣନା_2(smtName) > 0
                                    : memberselp          ? m_writtenVars.ଗଣନା_2(smtName) > 0
                                                          : varp->user3();
        ଅଚଳ ତାର୍କିକ shouldWriteVar = !alreadyWritten;
        ଯଦି (shouldWriteVar) {
            // Track this variable path as written
            ଯଦି (m_inlineInitTaskp)
                m_inlineWrittenVars.ଭର୍ତ୍ତି(smtName);
            ଅନ୍ୟଥା ଯଦି (isGlobalConstrained || memberselp)
                m_writtenVars.ଭର୍ତ୍ତି(smtName);
            // For global constraints, delete nodep after processing
            ଯଦି (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);

            ତାର୍କିକ isClassRefArray = ମିଥ୍ୟା;
            ତାର୍କିକ isUnpackedClassRefArray = ମିଥ୍ୟା;
            AstClassRefDType* elemClassRefDtp = nullptr;
            {
                Ast_ଗଣ୍ଠି_DType* varDtp = varp->dtypep()->skipRefp();
                ଯଦି (varDtp->isNonPackedArray()) {
                    Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    ଯଦି (elemClassRefDtp) {
                        isClassRefArray = ସତ୍ୟ;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_ଗଣ୍ଠି_Module* ଅଚଳ classp = getLeftmostVarModulep(memberselp, varp);
            Ast_ଗଣ୍ଠି_FTask* ଅଚଳ initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            ଯଦି (m_nestedAccess) {
                Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                addRegistrationStmt(writeVarStmtp, initTaskp);
                ଯଦି (isGlobalConstrained && memberselp && randMode.usesMode) {
                    setRandMode(varp, memberselp, smtName, randMode, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                ଯଦି (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } ଅନ୍ୟଥା ଯଦି (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } ଅନ୍ୟଥା {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, randMode,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } ଅନ୍ୟଥା {
            // Variable already written, clean up cloned memberselp if any
            ଯଦି (memberselp) VL_DO_DANGLING(memberselp->delete_ବୃକ୍ଷ(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            ଯଦି (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
        ଯଦି (m_nestedAccess) { VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* build_ଗଣନା_Ones_Expansion(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* argp) {
        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_One{fl, argp->width(), 1};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sump = ନୂତନ AstAnd{fl, argp, ନୂତନ AstConst{fl, ଅଙ୍କ_One}};
        sump->user1(ସତ୍ୟ);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1; i < argp->width(); i++) {
            V3_ସଂଖ୍ୟା ଅଙ୍କ_Bit_Mask{fl, argp->width(), 0};
            ଅଙ୍କ_Bit_Mask.setBit(i, 1);
            AstAnd* ଅଚଳ andp
                = ନୂତନ AstAnd{fl, argp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), ନୂତନ AstConst{fl, ଅଙ୍କ_Bit_Mask}};
            andp->user1(ସତ୍ୟ);
            AstShiftR* ଅଚଳ shiftp
                = ନୂତନ AstShiftR{fl, andp,
                                ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, argp->width(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(ସତ୍ୟ);
            sump = ନୂତନ AstAdd{fl, sump, shiftp};
            sump->user1(ସତ୍ୟ);
        }
        ଫେରନ୍ତୁ sump;
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣନା_Ones* nodep) override {
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = nodep->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sump = build_ଗଣନା_Ones_Expansion(fl, argp);
        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(ସତ୍ୟ);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(sump);
    }
    ଶୂନ୍ୟ visit(AstRedXor* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = nodep->lhsp()->unlinkFrBack();

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* redxorp = ନୂତନ AstSel{fl, argp, 0, 1};
        redxorp->user1(ସତ୍ୟ);
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1; i < argp->width(); i++) {
            AstSel* ଅଚଳ selp = ନୂତନ AstSel{fl, argp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), i, 1};
            selp->user1(ସତ୍ୟ);

            redxorp = ନୂତନ AstXor{fl, redxorp, selp};
            redxorp->user1(ସତ୍ୟ);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(redxorp);
    }
    ଶୂନ୍ୟ visit(AstRedAnd* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        // Convert to (~x == 0)
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = nodep->lhsp()->unlinkFrBack();
        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_Zero{fl, argp->width(), 0};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ negp = ନୂତନ AstNot{fl, argp};
        negp->user1(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eqp = ନୂତନ AstEq{fl, negp, ନୂତନ AstConst{fl, ଅଙ୍କ_Zero}};
        eqp->user1(ସତ୍ୟ);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(eqp);
    }
    ଶୂନ୍ୟ visit(AstRedOr* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        // Convert to (x != 0)
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = nodep->lhsp()->unlinkFrBack();
        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_Zero{fl, argp->width(), 0};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ neqp = ନୂତନ AstNeq{fl, argp, ନୂତନ AstConst{fl, ଅଙ୍କ_Zero}};
        neqp->user1(ସତ୍ୟ);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(neqp);
    }
    ଶୂନ୍ୟ visit(AstOneHot* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = nodep->lhsp()->unlinkFrBack();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = argp->width();

        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_Zero{fl, w, 0};
        AstNeq* ଅଚଳ neZerop
            = ନୂତନ AstNeq{fl, argp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), ନୂତନ AstConst{fl, ଅଙ୍କ_Zero}};
        neZerop->user1(ସତ୍ୟ);

        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_One{fl, w, 1};
        AstSub* ଅଚଳ subp = ନୂତନ AstSub{fl, argp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), ନୂତନ AstConst{fl, ଅଙ୍କ_One}};
        subp->user1(ସତ୍ୟ);

        AstAnd* ଅଚଳ andp = ନୂତନ AstAnd{fl, argp, subp};
        andp->user1(ସତ୍ୟ);

        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_Zero2{fl, w, 0};
        AstEq* ଅଚଳ eqZerop = ନୂତନ AstEq{fl, andp, ନୂତନ AstConst{fl, ଅଙ୍କ_Zero2}};
        eqZerop->user1(ସତ୍ୟ);

        AstLogAnd* ଅଚଳ resultp = ନୂତନ AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(ସତ୍ୟ);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(resultp);
    }
    ଶୂନ୍ୟ visit(AstOneHot0* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        // $onehot0(x) = (x & (x-1)) == 0
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = nodep->lhsp()->unlinkFrBack();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା w = argp->width();

        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_One{fl, w, 1};
        AstSub* ଅଚଳ subp = ନୂତନ AstSub{fl, argp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), ନୂତନ AstConst{fl, ଅଙ୍କ_One}};
        subp->dtypeFrom(argp);
        subp->user1(ସତ୍ୟ);

        AstAnd* ଅଚଳ andp = ନୂତନ AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(ସତ୍ୟ);

        ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_Zero{fl, w, 0};
        AstEq* ଅଚଳ eqp = ନୂତନ AstEq{fl, andp, ନୂତନ AstConst{fl, ଅଙ୍କ_Zero}};
        eqp->user1(ସତ୍ୟ);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(eqp);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣନା_Bits* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();

        ତାର୍କିକ ଗଣନା_Ones = ମିଥ୍ୟା;
        ତାର୍କିକ ଗଣନା_Zeros = ମିଥ୍ୟା;
        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ଅଚଳ AstConst* ଅଚଳ cp = VN_CAST(ctrlp, Const);
            ଯଦି (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ଅଚଳ zerop
                    = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, nodep->width(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (cp->ଅଙ୍କ().bitIs1(0) && !ଗଣନା_Ones)
                ଗଣନା_Ones = ସତ୍ୟ;
            ଅନ୍ୟଥା ଯଦି (cp->ଅଙ୍କ().bitIs0(0) && !ଗଣନା_Zeros)
                ଗଣନା_Zeros = ସତ୍ୟ;
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = nodep->lhsp()->unlinkFrBack();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା argWidth = argp->width();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sump = nullptr;
        ଯଦି (ଗଣନା_Ones && ଗଣନା_Zeros) {
            // ones + zeros = width for 2-state types
            sump = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, nodep->width(), (uint32_t)argWidth};
            VL_DO_DANGLING(argp->delete_ବୃକ୍ଷ(), argp);
        } ଅନ୍ୟଥା ଯଦି (ଗଣନା_Ones) {
            sump = build_ଗଣନା_Ones_Expansion(fl, argp);
        } ଅନ୍ୟଥା ଯଦି (ଗଣନା_Zeros) {
            // width - countones(x)
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ onesCountp = build_ଗଣନା_Ones_Expansion(fl, argp);
            ଅଚଳ V3_ସଂଖ୍ୟା widthVal{nodep, onesCountp->width(), (uint32_t)argWidth};
            sump = ନୂତନ AstSub{fl, ନୂତନ AstConst{fl, widthVal}, onesCountp};
            sump->user1(ସତ୍ୟ);
        } ଅନ୍ୟଥା {
            sump = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, nodep->width(), 0U};
            VL_DO_DANGLING(argp->delete_ବୃକ୍ଷ(), argp);
        }

        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(ସତ୍ୟ);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(sump);
    }
    ଶୂନ୍ୟ visit(AstCLog2* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argp = nodep->lhsp()->unlinkFrBack();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା argWidth = argp->width();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା resultWidth = nodep->width();

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp
            = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, resultWidth, (uint32_t)argWidth};

        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା k = argWidth - 1; k >= 0; k--) {
            V3_ସଂଖ୍ୟା threshold{fl, argWidth, 0};
            ଯଦି (k < 32)
                threshold.setLong(1ULL << k);
            ଅନ୍ୟଥା
                threshold.setBit(k, 1);

            AstLte* ଅଚଳ ltep
                = ନୂତନ AstLte{fl, argp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), ନୂତନ AstConst{fl, threshold}};
            ltep->user1(ସତ୍ୟ);

            AstConst* ଅଚଳ valuep
                = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, resultWidth, (uint32_t)k};

            resultp = ନୂତନ AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(ସତ୍ୟ);
        }

        VL_DO_DANGLING(argp->delete_ବୃକ୍ଷ(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(resultp);
    }
    ଶୂନ୍ୟ visit(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ଅଚଳ zerop = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    ଶୂନ୍ୟ handlePow(Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଯଦି (AstConst* ଅଚଳ exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ basep = nodep->lhsp();
            ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_One{nodep, basep->width(), 1};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* powerp = ନୂତନ AstConst{fl, ଅଙ୍କ_One};
            ଅଚଳ ତାର୍କିକ baseSigned = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ଅଚଳ int32_t exponent = baseSigned ? exponentp->toSInt() : exponentp->toUInt();
            ଯଦି (exponent > 0) {
                ପାଇଁ (int32_t i = 0; i < exponent; i++) {
                    ଯଦି (baseSigned) {
                        powerp = ନୂତନ AstMulS{fl, powerp, basep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    } ଅନ୍ୟଥା {
                        powerp = ନୂତନ AstMul{fl, powerp, basep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    }
                    powerp->user1(ସତ୍ୟ);
                }
            } ଅନ୍ୟଥା ଯଦି (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                ଯଦି (baseSigned) {
                    powerp = ନୂତନ AstDivS{fl, powerp, basep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    powerp->user1(ସତ୍ୟ);
                    powerp = ନୂତନ AstDivS{fl, powerp, basep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    powerp->user1(ସତ୍ୟ);
                } ଅନ୍ୟଥା {
                    powerp = ନୂତନ AstDiv{fl, powerp, basep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    powerp->user1(ସତ୍ୟ);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            iterate(powerp);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstConst* ଅଚଳ basep = VN_CAST(nodep->lhsp(), Const)) {
            ଯଦି (basep->ଅଙ୍କ().toUInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->width();
                ଅଚଳ ତାର୍କିକ rhsDependent = rhsp->user1();
                AstConst* ଅଚଳ onep = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, width, 1};
                AstShiftL* ଅଚଳ shiftp = ନୂତନ AstShiftL{fl, onep, rhsp, width};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resultp = shiftp;
                ଯଦି (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ nClonep = rhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    nClonep->user1(rhsDependent);
                    AstConst* ଅଚଳ zeroNp
                        = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, rhsp->width(), 0};
                    AstGteS* ଅଚଳ condp = ନୂତନ AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ଅଚଳ zeroRp = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, width, 0};
                    resultp = ନୂତନ AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                iterate(resultp);
            } ଅନ୍ୟଥା {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } ଅନ୍ୟଥା {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    ଶୂନ୍ୟ visit(AstPow* nodep) override { handlePow(nodep); }
    ଶୂନ୍ୟ visit(AstPowSS* nodep) override { handlePow(nodep); }
    ଶୂନ୍ୟ visit(AstPowSU* nodep) override { handlePow(nodep); }
    ଶୂନ୍ୟ visit(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    ଶୂନ୍ୟ handleShift(Ast_ଗଣ୍ଠି_Biop* nodep) {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhsWidth = nodep->lhsp()->width();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା rhsWidth = nodep->rhsp()->width();
        ଯଦି (rhsWidth < lhsWidth) {
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = nodep->rhsp()->unlinkFrBack();
            AstExtend* ଅଚଳ extendp = ନୂତନ AstExtend{fl, rhsp, lhsWidth};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(ସତ୍ୟ);
            nodep->rhsp(extendp);
        } ଅନ୍ୟଥା ଯଦି (rhsWidth > lhsWidth) {
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->dtypep();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ extendp
                = VN_IS(nodep, ShiftRS)
                      ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstExtendS{fl, lhsp, rhsWidth})
                      : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstExtend{fl, lhsp, rhsWidth});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(ସତ୍ୟ);
            nodep->lhsp(extendp);
            nodep->dtypeSetLogicSized(rhsWidth, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            AstSel* ଅଚଳ selp = ନୂତନ AstSel{fl, nodep, 0, lhsWidth};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            ଫେରନ୍ତୁ;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(AstShiftL* nodep) override { handleShift(nodep); }
    ଶୂନ୍ୟ visit(AstShiftR* nodep) override { handleShift(nodep); }
    ଶୂନ୍ୟ visit(AstShiftRS* nodep) override { handleShift(nodep); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Biop* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Uniop* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        editSMT(nodep, nodep->lhsp());
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Triop* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ଶୂନ୍ୟ visit(AstCond* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        ଯଦି (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtypeSetString();
            ଫେରନ୍ତୁ;
        }
        // Fall back to "(ite cond then else)"
        visit(static_cast<Ast_ଗଣ୍ଠି_Triop*>(nodep));
    }
    ଶୂନ୍ୟ visit(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        editSMT(nodep, nodep->srcp());
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        VNRelinker handle;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ msbp = ନୂତନ AstSFormatF{
            fl, "%1d", ମିଥ୍ୟା,
            ନୂତନ AstAdd{fl, nodep->lsbp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                       ନୂତନ AstConst{fl, static_cast<uint32_t>(nodep->widthConst() - 1)}}};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lsbp
            = ନୂତନ AstSFormatF{fl, "%1d", ମିଥ୍ୟା, nodep->lsbp()->unlinkFrBack(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        ଅଚଳ ତାର୍କିକ outermost = !m_deferRandModeHoist;
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origp = outermost ? nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା) : nullptr;
        VL_RESTORER(m_structSel);
        m_structSel = ସତ୍ୟ;
        ଯଦି (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = nodep->fromp();
            AstStructDType* ଅଚଳ structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ଯଦି (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } ଅନ୍ୟଥା {
                structp->markConstrainedRand(ସତ୍ୟ);
            }
        }
        // Mark Random for structArray
        ଯଦି (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ଅଚଳ structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ଯଦି (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } ଅନ୍ୟଥା {
                structp->markConstrainedRand(ସତ୍ୟ);
            }
        }
        {
            VL_RESTORER(m_deferRandModeHoist);
            m_deferRandModeHoist = ସତ୍ୟ;
            iterateChildren(nodep);
        }
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        AstSFormatF* newp = nullptr;
        ଯଦି (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = ନୂତନ AstSFormatF{fl, "%s.%s." + nodep->name(), ମିଥ୍ୟା,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)};
            ଯଦି (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } ଅନ୍ୟଥା {
            newp = ନୂତନ AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), ମିଥ୍ୟା,
                                   nullptr};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        ଯଦି (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(origp), origp);
        }
    }
    ଶୂନ୍ୟ addStringNamePart(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ indexp, ଅଚଳ std::string& fmt = "%32x") {
        ଯଦି (m_nestedAccess) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bitp = indexp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bitFormatp = ନୂତନ AstSFormatF{bitp->fileline(), fmt, ମିଥ୍ୟା, bitp};
            AstSFormatF* ଅଚଳ herep
                = ନୂତନ AstSFormatF{indexp->fileline(), "%s.%s", ମିଥ୍ୟା, bitFormatp};
            m_nestedAccess->addVarNamePart(herep, indexp->name());
        }
    }
    ଶୂନ୍ୟ visit(AstAssocSel* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origp = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        AstSFormatF* newp = nullptr;
        ଯଦି (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ idxp = ନୂତନ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), ମିଥ୍ୟା,
                                                      nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ଅଚଳ stringp = VN_AS(nodep->bitp(), CvtPackString);
            ଅଚଳ size_t stringSize = VN_AS(stringp->lhsp(), Const)->width();
            ଯଦି (stringSize > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << stringSize << "bits, limit is 128 bits");
            }
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ idxp = ନୂତନ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), ମିଥ୍ୟା,
                                                      stringp->lhsp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } ଅନ୍ୟଥା {
            ଯଦି (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା actual_width = nodep->bitp()->width();
                std::string fmt;
                // Normalize to standard bit width
                ଯଦି (actual_width <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } ଅନ୍ୟଥା ଯଦି (actual_width <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } ଅନ୍ୟଥା {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::to_string(VL_ଶବ୍ଦଗୁଡ଼ିକ_I(actual_width) * 8) + "x";
                }
                ଯଦି (m_nestedAccess) addStringNamePart(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ idxp
                    = ନୂତନ AstSFormatF{fl, fmt, ମିଥ୍ୟା, nodep->bitp()->unlinkFrBack(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } ଅନ୍ୟଥା {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                ଯଦି (m_nestedAccess) m_nestedAccess->error();
            }
        }
        ଯଦି (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        ଯଦି (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(origp->delete_ବୃକ୍ଷ(), origp);
        }
    }
    ଶୂନ୍ୟ rename_Array_ପଦସମଷ୍ଟି(AstSFormatF* ଅଚଳ newp) ଅଚଳ {
        ଯଦି (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        ତାର୍କିକ indexIsRand = ମିଥ୍ୟା;
        nodep->bitp()->foreach([&](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* vrefp) {
            ଯଦି (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                indexIsRand = ସତ୍ୟ;
            }
        });
        ଯଦି (m_nestedAccess) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bitp = nodep->bitp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ bitFormatp
                = indexIsRand ? ନୂତନ AstSFormatF{bitp->fileline(), bitp->name(), ମିଥ୍ୟା, nullptr}
                              : ନୂତନ AstSFormatF{bitp->fileline(), "%x", ମିଥ୍ୟା, bitp};
            AstSFormatF* ଅଚଳ herep
                = ନୂତନ AstSFormatF{nodep->fileline(), "%s.%s", ମିଥ୍ୟା, bitFormatp};
            ଯଦି (AstSel* ଅଚଳ selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->addVarNamePart(herep, selp->fromp()->name());
            } ଅନ୍ୟଥା {
                m_nestedAccess->addVarNamePart(herep, bitp->name());
            }
            ଯଦି (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_ବୃକ୍ଷ(), bitp);
                ଫେରନ୍ତୁ;
            }
        }
        ଯଦି (indexIsRand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* indexp = nodep->bitp()->unlinkFrBack(&handle);
            ଯଦି (indexp->width() < 32) {
                AstExtend* ଅଚଳ extendp = ନୂତନ AstExtend{fl, indexp, 32};
                extendp->user1(ସତ୍ୟ);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ଅଚଳ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ପଦସମଷ୍ଟି(newp);
        } ଅନ୍ୟଥା {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origp = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ indexp
                = ନୂତନ AstSFormatF{fl, "#x%8x", ମିଥ୍ୟା, nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(indexp);
            AstSFormatF* ଅଚଳ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ପଦସମଷ୍ଟି(newp);
            ଯଦି (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
                VL_DO_DANGLING(origp->delete_ବୃକ୍ଷ(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ସ୍ଥିର ତାର୍କିକ isRandModeGate(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଅଚଳ AstCMethodHard* ଅଚଳ atp = VN_CAST(exprp, CMethodHard);
        ଯଦି (!atp || atp->method() != VCMethod::ARRAY_AT) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstVar* varp = nullptr;
        ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        ଯଦି (!varp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(varp->user2p(), Class);
        ଫେରନ୍ତୁ classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    ତାର୍କିକ hoistRandModeOverSelectAndMember(AstSFormatF* newp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* origp) {
        ଯଦି (m_deferRandModeHoist) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Only for selects yielding a non-array element (full chains)
        ଯଦି (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::vector<AstSFormatF*> frames;
        AstCond* modeCondp = nullptr;
        ପାଇଁ (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ firstp = curp->exprsp();
            frames.ଠେଲ_back(curp);
            ଯଦି (AstCond* ଅଚଳ condp = VN_CAST(firstp, Cond)) {
                ଯଦି (isRandModeGate(condp->condp())) modeCondp = condp;
                ଭାଙ୍ଗ;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        ଯଦି (!modeCondp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        File_ଧାଡ଼ି* ଅଚଳ fl = newp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ modep = modeCondp->condp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* activep = modeCondp->thenp()->unlinkFrBack();
        // Rebuild the select chain text around the SMT name, innermost first
        ପାଇଁ (auto it = frames.rbegin(); it != frames.rend(); ++it) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ remainingArgsp = VN_CAST((*it)->exprsp()->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            ଯଦି (remainingArgsp) { remainingArgsp->unlinkFrBackWithNext(); }
            activep = ନୂତନ AstSFormatF{fl, (*it)->name(), ମିଥ୍ୟା,
                                      Ast_ଗଣ୍ଠି::addNext(activep, remainingArgsp)};
        }
        AstCond* ଅଚଳ hoistp = ନୂତନ AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(ସତ୍ୟ);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(ଠେଲ_Deletep(newp), newp);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        ଯଦି (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            ଯଦି (m_nestedAccess) {
                AstSFormatF* ଅଚଳ formatp = ନୂତନ AstSFormatF{fl, nodep->name(), ମିଥ୍ୟା, nullptr};
                m_nestedAccess->addVarNamePart(ନୂତନ AstSFormatF{fl, "%s.%s", ମିଥ୍ୟା, formatp},
                                               nodep->name());
            }
            Ast_ଗଣ୍ଠି* ମୂଳ_ଗଣ୍ଠି = nodep->fromp();
            ଯେତେବେଳେ (ଅଚଳ AstMemberSel* ଅଚଳ selp = VN_CAST(ମୂଳ_ଗଣ୍ଠି, MemberSel)) {
                ମୂଳ_ଗଣ୍ଠି = selp->fromp();
            }

            Ast_ଗଣ୍ଠି_Sel* arraySelp = VN_CAST(ମୂଳ_ଗଣ୍ଠି, ArraySel);
            ଯଦି (!arraySelp) arraySelp = VN_CAST(ମୂଳ_ଗଣ୍ଠି, AssocSel);

            ଯଦି (arraySelp) {
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : nullptr;
                ଯଦି (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ଅଚଳ ତାର୍କିକ isSimple
                        = nodep->fromp() == ମୂଳ_ଗଣ୍ଠି && VN_IS(arraySelp->fromp(), VarRef);
                    ଯଦି (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = ନୂତନ NestedAccessPath(nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                                              &m_firstExpressionInsideIndexp);
                        AstSFormatF* ଅଚଳ formatp
                            = ନୂତନ AstSFormatF{fl, nodep->name(), ମିଥ୍ୟା, nullptr};
                        m_nestedAccess->addVarNamePart(
                            ନୂତନ AstSFormatF{fl, "%s.%s", ମିଥ୍ୟା, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = ସତ୍ୟ;
                    nodep->user1(ସତ୍ୟ);
                    arraySelp->user1(ସତ୍ୟ);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origp = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                    iterateChildren(nodep);
                    AstSFormatF* ଅଚଳ newp = ନୂତନ AstSFormatF{fl, "%s." + nodep->name(), ମିଥ୍ୟା,
                                                              nodep->fromp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    ଯଦି (!hoistRandModeOverSelectAndMember(newp, origp))
                        VL_DO_DANGLING(origp->delete_ବୃକ୍ଷ(), origp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess);
                    ଫେରନ୍ତୁ;
                }
            }
            // Check if the root variable participates in global constraints
            ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ varRefp = VN_CAST(ମୂଳ_ଗଣ୍ଠି, VarRef)) {
                AstVar* ଅଚଳ constrainedVar = varRefp->varp();
                ଯଦି (constrainedVar->globalConstrained()) {
                    ତାର୍କିକ ଅଚଳ beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    ଯଦି (beforeIterate) {
                        nodep->replaceWith(ନୂତନ AstSFormatF{fl, "%s." + nodep->name(), ମିଥ୍ୟା,
                                                           nodep->fromp()->unlinkFrBack()});
                    } ଅନ୍ୟଥା {
                        nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                    }
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        ଯଦି (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), ଗଣ୍ଠି_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ଅଚଳ varRefp
                = ନୂତନ AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), ଗଣ୍ଠି_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            visit(varRefp);
        } ଅନ୍ୟଥା ଯଦି (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->method() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ଅଚଳ cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = arrayDtp->subDTypep()->skipRefp();
            ଯଦି (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = ସତ୍ୟ;
                iterateChildren(nodep);
                File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                AstSFormatF* newp = nullptr;
                ଯଦି (AstSFormatF* ଅଚଳ fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    ଯଦି (fromp->name() == "%s.%s") {
                        newp = ନୂତନ AstSFormatF{fl, "%s.%s." + nodep->name(), ମିଥ୍ୟା,
                                               fromp->exprsp()->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)};
                    } ଅନ୍ୟଥା {
                        newp = ନୂତନ AstSFormatF{fl, fromp->name() + "." + nodep->name(), ମିଥ୍ୟା,
                                               nullptr};
                    }
                } ଅନ୍ୟଥା {
                    newp = ନୂତନ AstSFormatF{fl, nodep->name(), ମିଥ୍ୟା, nullptr};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ଫେରନ୍ତୁ;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା {
            editFormat(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {}
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {}
    ଶୂନ୍ୟ visit(AstCStmt* nodep) override {}
    ଶୂନ୍ୟ visit(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    ଶୂନ୍ୟ visit(AstConstraintIf* nodep) override {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = nullptr;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thenp = editSingle(fl, nodep->thensp());
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elsep = editSingle(fl, nodep->elsesp());
        ଯଦି (thenp && elsep) {
            newp = ନୂତନ AstCond{fl, nodep->condp()->unlinkFrBack(), thenp, elsep};
        } ଅନ୍ୟଥା ଯଦି (thenp) {
            newp = ନୂତନ AstLogIf{fl, nodep->condp()->unlinkFrBack(), thenp};
        } ଅନ୍ୟଥା ଯଦି (elsep) {
            newp = ନୂତନ AstLogIf{fl, ନୂତନ AstNot{fl, nodep->condp()->unlinkFrBack()}, elsep};
        }
        ଯଦି (newp) {
            newp->dtypeSetBit();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(ସତ୍ୟ);  // Assume result-dependent
            nodep->replaceWith(ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, newp});
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    ଶୂନ୍ୟ visit(AstBegin* nodep) override {}
    ଶୂନ୍ୟ visit(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();

        ଯଦି (!nodep->bodyp()) {
            nodep->unlinkFrBack();
        } ଅନ୍ୟଥା ଯଦି (m_wantSingle) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(itemp);
            cstmtp->add(";");
            Ast_ଗଣ୍ଠି* ଅଚଳ bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ଅଚଳ cexprp = ନୂତନ AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(ନୂତନ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     ନୂତନ AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp},
                                     ସତ୍ୟ});
            cexprp->add("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(ନୂତନ AstSFormatF{fl, "%s", ମିଥ୍ୟା, cexprp});
        } ଅନ୍ୟଥା {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            ପାଇଁ (Ast_ଗଣ୍ଠି* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(elemp, Var)) m_foreachIdxVars.ଭର୍ତ୍ତି(varp);
            }
            iterateAndNextNull(nodep->bodyp());
            Ast_ଗଣ୍ଠି* ଅଚଳ bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlinkFrBackWithNext());
            nodep->replaceWith(
                ନୂତନ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                             ନୂତନ AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp}, ସତ୍ୟ});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ genModp = VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module);

        ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsTestp = build_Solve_Before_Name_ପଦସମଷ୍ଟି(fl, lhsp);
            ଯଦି (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                ଜାରି;
            }
            VL_DO_DANGLING(lhsTestp->delete_ବୃକ୍ଷ(), lhsTestp);
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsNamep = build_Solve_Before_Name_ପଦସମଷ୍ଟି(fl, rhsp);
                ଯଦି (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    ଜାରି;
                }
                AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
                    fl, ନୂତନ AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtypeSetVoid();
                callp->addPinsp(build_Solve_Before_Name_ପଦସମଷ୍ଟି(fl, lhsp));
                callp->addPinsp(rhsNamep);
                nodep->addHereThisAsNext(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstConstraintUnique* nodep) override {
        Ast_ଗଣ୍ଠି_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        Ast_ଗଣ୍ଠି_Module* genModp = nullptr;

        ଯଦି (m_classp) {
            initTaskp = VN_AS(m_memberMap.findMember(m_classp, "new"), ଗଣ୍ଠି_FTask);
            genVarp = getRandomGenerator(m_classp);
        } ଅନ୍ୟଥା {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        ଯଦି (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_classp) {
            genModp = VN_AS(genVarp->user2p(), ଗଣ୍ଠି_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_ଗଣ୍ଠି* setupStmtsp = nullptr;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::vector<std::string> smtExprs;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (AstVarRef* ଅଚଳ varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ଅଚଳ varp = varRefp->varp();
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ varModp = VN_AS(varp->user2p(), ଗଣ୍ଠି_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep()->skipRefp();

                ଅଚଳ ସ୍ଥିର auto dynDTypeSupported
                    = [](ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) -> ତାର୍କିକ {
                    ଫେରନ୍ତୁ VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elemWidth;
                uint32_t staticSize = 0;  // Element count, zero when only known at run time
                ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ up = VN_CAST(dtypep, UnpackArrayDType)) {
                    ଅଚଳ AstRange* ଅଚଳ rangep = up->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    staticSize = up->elementsConst();
                    ଯଦି (staticSize > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        ଜାରି;
                    }
                    elemWidth = static_cast<uint64_t>(varp->dtypep()->width());
                } ଅନ୍ୟଥା ଯଦି (dynDTypeSupported(dtypep)) {
                    elemWidth = static_cast<uint64_t>(dtypep->subDTypep()->width());
                } ଅନ୍ୟଥା {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(ମିଥ୍ୟା));
                    ଜାରି;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ subp = dtypep->subDTypep()->skipRefp();
                ଯଦି (VN_IS(subp, ଗଣ୍ଠି_Array_DType) || VN_IS(subp, QueueDType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    ଜାରି;
                }

                ଯଦି (m_wantSingle && !staticSize) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    ଜାରି;
                }

                ଯଦି (!varp->user3()) {
                    varp->user3(ସତ୍ୟ);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ varnamep = ନୂତନ AstCExpr{
                        fl, AstCExpr::Pure{}, "\"" + varp->name() + "\"", varp->width()};

                    AstCMethodHard* ଅଚଳ writeVarCallp = ନୂତନ AstCMethodHard{
                        fl, ନୂତନ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ଅଚଳ argVarRefp = ନୂତନ AstVarRef{fl, varp, VAccess::READ};
                    ଯଦି (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->addPinsp(argVarRefp);
                    writeVarCallp->addPinsp(ନୂତନ AstConst{fl, AstConst::Unsized64{}, elemWidth});
                    writeVarCallp->addPinsp(varnamep);
                    writeVarCallp->addPinsp(ନୂତନ AstConst{fl, 1});  // Dimension
                    ଅଚଳ RandomizeMode randMode = {.asUQuad = varp->user1()};
                    ଯଦି (randMode.usesMode) {
                        writeVarCallp->addPinsp(
                            ନୂତନ AstConst{fl, AstConst::Unsized64{}, randMode.index});
                    }
                    writeVarCallp->dtypeSetVoid();
                    initTaskp->addStmtsp(ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                ଯଦି (varp->user4p() && m_sizeConstrainedArraysp) {
                    m_sizeConstrainedArraysp->ଭର୍ତ୍ତି(varp);
                }

                ଯଦି (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    ଯଦି (staticSize < 2) ଜାରି;
                    std::string ପଦସମଷ୍ଟି_Str = "(__Vbv (distinct";
                    ପାଇଁ (uint32_t i = 0; i < staticSize; ++i) {
                        ପଦସମଷ୍ଟି_Str += " (select " + varp->name() + " #x";
                        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା shift = 28; shift >= 0; shift -= 4) {
                            ପଦସମଷ୍ଟି_Str += "0123456789abcdef"[(i >> shift) & 0xf];
                        }
                        ପଦସମଷ୍ଟି_Str += ')';
                    }
                    smtExprs.emplace_back(ପଦସମଷ୍ଟି_Str + "))");
                    ଜାରି;
                }

                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ namep = ନୂତନ AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ଅଚଳ randUniqueCallp
                    = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->findVoidDType());
                randUniqueCallp->addPinsp(ନୂତନ AstConst{fl, *m_uniqueConstraintId});
                setupStmtsp = Ast_ଗଣ୍ଠି::addNext(setupStmtsp, ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        ଯଦି (m_wantSingle && !smtExprs.empty()) {
            std::string ପଦସମଷ୍ଟି_Str = smtExprs.front();
            ଯଦି (smtExprs.size() > 1) {
                ପଦସମଷ୍ଟି_Str = "(bvand";
                ପାଇଁ (ଅଚଳ std::string& oneExprp : smtExprs) ପଦସମଷ୍ଟି_Str += " " + oneExprp;
                ପଦସମଷ୍ଟି_Str += ')';
            }
            AstSFormatF* ଅଚଳ newp = ନୂତନ AstSFormatF{fl, ପଦସମଷ୍ଟି_Str, ମିଥ୍ୟା, nullptr};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (setupStmtsp) nodep->addHereThisAsNext(setupStmtsp);
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_Constraint_ପଦସମଷ୍ଟି* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_firstExpressionInsideIndexp);
        ଯଦି (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->exprp();
            ଯଦି (exprp->width() > 1) {
                File_ଧାଡ଼ି* ଅଚଳ fl = exprp->fileline();
                ଅଚଳ V3_ସଂଖ୍ୟା ଅଙ୍କ_Zero{fl, exprp->width(), 0};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ neqp
                    = ନୂତନ AstNeq{fl, exprp->unlinkFrBack(), ନୂତନ AstConst{fl, ଅଙ୍କ_Zero}};
                neqp->user1(ସତ୍ୟ);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        ଯଦି (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ଅଚଳ VCMethod method
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
            nodep->fileline(),
            ନୂତନ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module), m_genp,
                          VAccess::READWRITE},
            method, nodep->exprp()->unlinkFrBack()};
        callp->dtypeSetVoid();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->addPinsp(ନୂତନ AstCExpr{nodep->fileline(), AstCExpr::Pure{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ଅଚଳ uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->addPinsp(ନୂତନ AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string prettyText;
        ଯଦି (!v3Global.opt.protectIds()) {
            prettyText = nodep->fileline()->prettySource();
            size_t pos = 0;
            ଯେତେବେଳେ ((pos = prettyText.find('"', pos)) != std::string::npos) {
                prettyText.ଭର୍ତ୍ତି(pos, "\\");
                pos += std::strlen("\\\"");
            }
        }
        callp->addPinsp(
            ନୂତନ AstCExpr{nodep->fileline(), AstCExpr::Pure{}, "\"" + prettyText + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();

        ଯଦି (nodep->method() == VCMethod::ARRAY_AT) {
            addStringNamePart(nodep->pinsp(), "%x");
            ଅଚଳ ତାର୍କିକ indexIsRand = nodep->pinsp()->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ vrefp) {
                ଫେରନ୍ତୁ vrefp->varp()->rand().isRandomizable();
            });
            ଯଦି (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ basep = nodep->baseFromp(ସତ୍ୟ);
                    ଯଦି (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        ଫେରନ୍ତୁ;
                    }
                }
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origp = indexIsRand ? nullptr : nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                ଅଚଳ AstVarRef* ଅଚଳ baseRefp
                    = m_structSel ? VN_CAST(nodep->fromp()->baseFromp(ସତ୍ୟ), VarRef) : nullptr;
                ଯଦି (baseRefp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(baseRefp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ indexp = nodep->pinsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ arrayp = nodep->fromp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    AstCMethodHard* ଅଚଳ currentSizep
                        = ନୂତନ AstCMethodHard{fl, arrayp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), VCMethod::DYN_SIZE};
                    currentSizep->dtypeSetInt();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ inRangeRequiredp = ନୂତନ AstLogAnd{
                        fl,
                        ନୂତନ AstLteS{fl,
                                    ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, indexp->width(), 0},
                                    indexp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)},
                        ନୂତନ AstGteS{fl, indexp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), currentSizep}};
                    AstCMethodHard* ଅଚଳ resizep
                        = ନୂତନ AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             ନୂତନ AstAdd{fl, indexp, ନୂତନ AstConst{fl, 1}}};
                    resizep->dtypeSetVoid();
                    m_prepareConstrainedArraysp->addStmtsp(
                        ନୂତନ AstIf{fl, inRangeRequiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ଅଚଳ sizep
                    = m_structSel ? ନୂତନ AstCMethodHard{fl, nodep->fromp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                       VCMethod::DYN_SIZE}
                                  : nullptr;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ pinp = nodep->pinsp()->unlinkFrBack();
                AstSFormatF* newp;
                ଯଦି (m_structSel) {
                    ଯଦି (AstSFormatF* ଅଚଳ sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argsp
                        = Ast_ଗଣ୍ଠି::addNext(nodep->fromp()->unlinkFrBack(), pinp);
                    sizep->dtypeSetInt();
                    AstLogAnd* ଅଚଳ condp
                        = ନୂତନ AstLogAnd{fl,
                                        ନୂତନ AstLteS{fl,
                                                    ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{},
                                                                 originalPinp->width(), 0},
                                                    originalPinp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)},
                                        ନୂତନ AstLtS{fl, originalPinp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), sizep}};
                    m_conditionp = m_conditionp ? ନୂତନ AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = ନୂତନ AstSFormatF{fl, "%s.%s", ମିଥ୍ୟା, argsp};
                } ଅନ୍ୟଥା {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                ଯଦି (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_ବୃକ୍ଷ(), origp);
                }

            } ଅନ୍ୟଥା ଯଦି (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } ଅନ୍ୟଥା {
                nodep->user1(ମିଥ୍ୟା);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            ଫେରନ୍ତୁ;
        }

        ଯଦି (nodep->method() == VCMethod::ARRAY_INSIDE) {
            ଅଚଳ ତାର୍କିକ randArr = nodep->fromp()->user1();

            AstVar* ଅଚଳ newVarp
                = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->findIntDType()};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ idxRefp = ନୂତନ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            AstForeachHeader* ଅଚଳ headerp
                = ନୂତନ AstForeachHeader{fl, nodep->fromp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), newVarp};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_ଗଣ୍ଠି* ଅଚଳ itemp = ନୂତନ AstEq{fl, selp, nodep->pinsp()->unlinkFrBack()};
            itemp->user1(ସତ୍ୟ);

            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(iterateSubtreeReturnEdits(itemp));
            cstmtp->add(";");
            AstCExpr* ଅଚଳ cexprp = ନୂତନ AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(ନୂତନ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     ନୂତନ AstForeach{fl, headerp, cstmtp}, ସତ୍ୟ});
            cexprp->add("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(ନୂତନ AstSFormatF{fl, "%s", ମିଥ୍ୟା, cexprp});
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }

        // Array reduction methods (sum, product, and, or, xor)
        ଯଦି (nodep->method() == VCMethod::ARRAY_R_SUM
            || nodep->method() == VCMethod::ARRAY_R_PRODUCT
            || nodep->method() == VCMethod::ARRAY_R_AND || nodep->method() == VCMethod::ARRAY_R_OR
            || nodep->method() == VCMethod::ARRAY_R_XOR) {

            AstWith* ଅଚଳ withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            ଯଦି (withp) {
                ଯଦି (AstUnpackArrayDType* ଅଚଳ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା arraySize = adtypep->elementsConst();
                    ଯଦି (arraySize > v3Global.opt.constraintArrayLimit()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvtToStr(arraySize)
                                          + " exceeds --constraint-array-limit of "
                                          + cvtToStr(v3Global.opt.constraintArrayLimit())
                                          + "), treating as state");
                        nodep->user1(ମିଥ୍ୟା);
                        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        ଫେରନ୍ତୁ;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ଅଚଳ ତାର୍କିକ hasRandomArray = nodep->fromp()->user1();
            ଯଦି (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(ମିଥ୍ୟା);
                ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                ଫେରନ୍ତୁ;
            }

            // Create loop variable and header
            AstVar* ଅଚଳ loopVarp
                = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->findIntDType()};
            AstForeachHeader* ଅଚଳ headerp
                = ନୂତନ AstForeachHeader{fl, nodep->fromp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), loopVarp};

            // Determine SMT operation and compute identity elements
            ଅଚଳ ଅକ୍ଷର* smtOp = nullptr;
            std::string identity;
            ଯଦି (withp) {
                // For 'with' clause: use binary format, compute from result width
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->dtypep()->width();
                ଯଦି (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = "#b" + std::string(width, '0');
                } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = (width > 0) ? "#b" + std::string(width - 1, '0') + "1" : "#b0";
                } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = "#b" + std::string(width, '1');
                } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = "#b" + std::string(width, '0');
                } ଅନ୍ୟଥା {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = "#b" + std::string(width, '0');
                }
            } ଅନ୍ୟଥା {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ଅଚଳ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ଅଚଳ arrVarp = arrRefp->varp();
                Ast_ଗଣ୍ଠି_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elemWidth = elemDtp->width();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା hexDigits = (elemWidth + 3) / 4;

                ଅଚଳ std::string zeroIdentity = "#x" + std::string(hexDigits, '0');
                ଅଚଳ std::string oneIdentity = "#x" + std::string(hexDigits - 1, '0') + "1";
                ଅଚଳ std::string allOnesIdentity = "#x" + std::string(hexDigits, 'f');

                ଯଦି (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = zeroIdentity;
                } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = oneIdentity;
                } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = allOnesIdentity;
                } ଅନ୍ୟଥା ଯଦି (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = zeroIdentity;
                } ଅନ୍ୟଥା {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = zeroIdentity;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ଅଚଳ cstmtp = ନୂତନ AstCStmt{fl};
            ଯଦି (withp) {
                // With 'with' clause: evaluate expression for each element
                ଅଚଳ ତାର୍କିକ randArr = nodep->fromp()->user1();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ idxRefp = ନୂତନ AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା resultWidth = nodep->dtypep()->width();
                ଅଚଳ VSigning resultSigning = nodep->dtypep()->numeric();

                Ast_ଗଣ୍ଠି* perElemExprp = withp->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                ଯଦି (AstLambdaArgRef* ଅଚଳ ମୂଳ_Refp = VN_CAST(perElemExprp, LambdaArgRef)) {
                    ଯଦି (ମୂଳ_Refp->index()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ଅଚଳ loopRef = ନୂତନ AstVarRef{fl, loopVarp, VAccess::READ};
                        perElemExprp = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                    } ଅନ୍ୟଥା {
                        // item at root -> replace with array element
                        perElemExprp = elemSelp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    }
                    VL_DO_DANGLING(ମୂଳ_Refp->delete_ବୃକ୍ଷ(), ମୂଳ_Refp);
                } ଅନ୍ୟଥା {
                    perElemExprp->foreach([&](AstLambdaArgRef* refp) {
                        ଯଦି (refp->index()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ଅଚଳ loopRef = ନୂତନ AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_ଗଣ୍ଠି* ଅଚଳ replacement
                                = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                            refp->replaceWith(replacement);
                        } ଅନ୍ୟଥା {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                        }
                        VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_ବୃକ୍ଷ(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                perElemExprp = V3Const::constifyEdit(perElemExprp);

                perElemExprp->foreach([&](Ast_ଗଣ୍ଠି* nodep) {
                    // Don't mark loop variable references as randomizable
                    ଯଦି (!VN_IS(nodep, Const)) {
                        ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ vrefp = VN_CAST(nodep, ଗଣ୍ଠି_Var_Ref)) {
                            ଯଦି (vrefp->varp() != loopVarp) nodep->user1(ସତ୍ୟ);
                        } ଅନ୍ୟଥା {
                            nodep->user1(ସତ୍ୟ);
                        }
                    }
                });

                cstmtp->add("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(iterateSubtreeReturnEdits(perElemExprp));
                cstmtp->add(";\n");
                cstmtp->add("ret += \")\";\n");
            } ଅନ୍ୟଥା {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ଅଚଳ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ଅଚଳ arrVarp = arrRefp->varp();
                ଅଚଳ std::string smtArrayName = arrVarp->name();
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା elemWidth = elemDtp->width();
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ arrModulep = VN_AS(arrVarp->user2p(), ଗଣ୍ଠି_Module);
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ genModulep = VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module);
                arrVarp->user3(ସତ୍ୟ);

                // Add write_var call to init task
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ arrVarNamep
                    = ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "\"" + smtArrayName + "\"", elemWidth};
                AstCMethodHard* ଅଚଳ writeVarCallp
                    = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->addPinsp(ନୂତନ AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->addPinsp(
                    ନୂତନ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elemWidth)});
                writeVarCallp->addPinsp(arrVarNamep);
                ଅଚଳ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(ମିଥ୍ୟା).second;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->addPinsp(ନୂତନ AstConst{fl, 1});  // Dimension

                ଅଚଳ RandomizeMode randMode = {.asUQuad = arrVarp->user1()};
                ଯଦି (randMode.usesMode) {
                    writeVarCallp->addPinsp(
                        ନୂତନ AstConst{fl, AstConst::Unsized64{}, randMode.index});
                }
                writeVarCallp->dtypeSetVoid();
                Ast_ଗଣ୍ଠି_FTask* initTaskp = m_inlineInitTaskp;
                ଯଦି (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.findMember(arrModulep, "new"), ଗଣ୍ଠି_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->addStmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ଅଚଳ idxFormatp = ନୂତନ AstSFormatF{
                    fl, "#x%08x", ମିଥ୍ୟା, ନୂତନ AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ଅଚଳ varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), idxFormatp);

                cstmtp->add("ret += \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(varNamep);
                cstmtp->add(";\n");
            }

            // Build final expression
            ଯଦି (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ଅଚଳ cexprp = ନୂତନ AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret = \"" + identity + "\";\n");
                cexprp->add(ନୂତନ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         ନୂତନ AstForeach{fl, headerp, cstmtp}, ସତ୍ୟ});
                cexprp->add("return ret;\n})()");
                nodep->replaceWith(ନୂତନ AstSFormatF{fl, "%s", ମିଥ୍ୟା, cexprp});
            } ଅନ୍ୟଥା {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ଅଚଳ cexprp = ନୂତନ AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret;\n");
                cexprp->add(ନୂତନ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         ନୂତନ AstForeach{fl, headerp, cstmtp}, ସତ୍ୟ});
                cexprp->add("return ret.empty() ? \"" + identity + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(ନୂତନ AstSFormatF{fl, "%s", ମିଥ୍ୟା, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }

        // We've handled DYN_SIZE already if they should be randomized
        ଯଦି (nodep->method() == VCMethod::DYN_SIZE || nodep->method() == VCMethod::ASSOC_SIZE) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଗଣ୍ଠି_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            ଯେତେବେଳେ (VN_IS(ଗଣ୍ଠି_Exprp, MemberSel)) {
                ଗଣ୍ଠି_Exprp = VN_AS(ଗଣ୍ଠି_Exprp, MemberSel)->fromp();
            }
            ଯଦି (VN_IS(ଗଣ୍ଠି_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)));
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                ଫେରନ୍ତୁ;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(ମିଥ୍ୟା);

        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;

        AstFunc* ଅଚଳ funcp = VN_CAST(nodep->taskp(), Func);
        ଯଦି (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            ଫେରନ୍ତୁ;
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ retExprp = extract_Return_ପଦସମଷ୍ଟି(funcp);
        ଯଦି (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(ମିଥ୍ୟା);
            ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
            ଫେରନ୍ତୁ;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ଅଚଳ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), nullptr, ମିଥ୍ୟା);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* inlinedp = retExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        ପାଇଁ (ଅଚଳ auto& tconnect : tconnects) {
            ଅଚଳ AstVar* ଅଚଳ portp = tconnect.first;
            AstArg* ଅଚଳ argp = tconnect.second;
            ଯଦି (!argp || !argp->exprp()) ଜାରି;
            ଯଦି (AstVarRef* ଅଚଳ ମୂଳ_Refp = VN_CAST(inlinedp, VarRef)) {
                ଯଦି (ମୂଳ_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    VL_DO_DANGLING(ମୂଳ_Refp->delete_ବୃକ୍ଷ(), ମୂଳ_Refp);
                    ଜାରି;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                ଯଦି (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                    VL_DO_DANGLING(refp->delete_ବୃକ୍ଷ(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) override {
        ଯଦି (editFormat(nodep)) ଫେରନ୍ତୁ;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_ଗଣ୍ଠି* buildDisableSoftCallStmt(Ast_Constraint_ପଦସମଷ୍ଟି* nodep) {
        std::string varName;
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ vrefp = VN_CAST(nodep->exprp(), ଗଣ୍ଠି_Var_Ref)) {
            varName = vrefp->name();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ଅଚଳ callp = ନୂତନ AstCMethodHard{
            nodep->fileline(),
            ନୂତନ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ଗଣ୍ଠି_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            ନୂତନ AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtypeSetVoid();
        ଫେରନ୍ତୁ callp->makeStmt();
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
    Ast_ଗଣ୍ଠି* extractConditionalDisableSofts(Ast_ଗଣ୍ଠି* itemsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* outerCondp) {
        Ast_ଗଣ୍ଠି* hoistListp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ଯଦି (AstConstraintIf* ଅଚଳ ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_ଧାଡ଼ି* ଅଚଳ fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thenCondp
                    = outerCondp ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                       ନୂତନ AstLogAnd{fl, outerCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                     ifp->condp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)})
                                 : ifp->condp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ thenHoistp
                    = extractConditionalDisableSofts(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_ଗଣ୍ଠି::addNext(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_ବୃକ୍ଷ(), thenCondp);
                ଯଦି (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ notInnerp
                        = ନୂତନ AstLogNot{fl, ifp->condp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elseCondp
                        = outerCondp ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(ନୂତନ AstLogAnd{
                                           fl, outerCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), notInnerp})
                                     : notInnerp;
                    ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ elseHoistp
                        = extractConditionalDisableSofts(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_ଗଣ୍ଠି::addNext(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_ବୃକ୍ଷ(), elseCondp);
                }
            } ଅନ୍ୟଥା ଯଦି (Ast_Constraint_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_CAST(stmtp, Constraint_ପଦସମଷ୍ଟି)) {
                ଯଦି (exprp->isDisableSoft() && outerCondp) {
                    File_ଧାଡ଼ି* ଅଚଳ fl = exprp->fileline();
                    AstIf* ଅଚଳ hoistIfp = ନୂତନ AstIf{fl, outerCondp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                                      buildDisableSoftCallStmt(exprp), nullptr};
                    hoistListp = hoistListp ? Ast_ଗଣ୍ଠି::addNext(hoistListp,
                                                               static_cast<Ast_ଗଣ୍ଠି*>(hoistIfp))
                                            : static_cast<Ast_ଗଣ୍ଠି*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_ପଦସମଷ୍ଟି* ଅଚଳ truep
                        = ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, ନୂତନ AstConst{fl, AstConst::BitTrue{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_ବୃକ୍ଷ(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        ଫେରନ୍ତୁ hoistListp;
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit Constraint_ପଦସମଷ୍ଟି_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_ଗଣ୍ଠି* nodep,
                                   Ast_ଗଣ୍ଠି_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* randModeVarp, std::set<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3UniqueNames& uniqueNames,
                                   Ast_ଗଣ୍ଠି_FTask* memberselInitTaskp = nullptr,
                                   VInsertionSet<AstVar*>* sizeConstrainedArraysp = nullptr,
                                   Ast_ଗଣ୍ଠି_FTask* prepareConstrainedArraysp = nullptr)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_randModeVarp{randModeVarp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_sizeConstrainedArraysp{sizeConstrainedArraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_uniqueNames{uniqueNames} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ hoistListp
            = nodep ? extractConditionalDisableSofts(nodep, nullptr) : nullptr) {
            nodep->addNext(hoistListp);
        }
        iterateAndNextNull(nodep);
    }
};

ଗଣନା ଶ୍ରେଣୀ CaptureMode : uint8_t {
    CAP_NO = 0x0,
    CAP_ମୂଲ୍ୟ = 0x01,
    CAP_THIS = 0x02,
    CAP_F_SET_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
CaptureMode operator|(CaptureMode a, CaptureMode b) {
    ଫେରନ୍ତୁ static_cast<CaptureMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
CaptureMode operator&(CaptureMode a, CaptureMode b) {
    ଫେରନ୍ତୁ static_cast<CaptureMode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
CaptureMode mode(CaptureMode a) { ଫେରନ୍ତୁ a & static_cast<CaptureMode>(0x3); }
ତାର୍କିକ hasFlags(CaptureMode a, CaptureMode flags) {
    ଫେରନ୍ତୁ ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

ଶ୍ରେଣୀ CaptureVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    AstArg* m_argsp = nullptr;  // Original references turned into arguments
    Ast_ଗଣ୍ଠି_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ଅଚଳ AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::set<Ast_ଗଣ୍ଠି*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = nullptr;  // Variable for outer context's object, if necessary
    ତାର୍କିକ m_staticContext = ମିଥ୍ୟା;  // True when capturing from a static function context

    // METHODS

    ତାର୍କିକ captureVariable(File_ଧାଡ଼ି* ଅଚଳ fileline, Ast_ଗଣ୍ଠି_Var_Ref* varrefp, AstVar*& varp) {
        auto it = m_varCloneMap.find(varrefp->varp());
        ଯଦି (it == m_varCloneMap.end()) {
            AstVar* ଅଚଳ newVarp = varrefp->varp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(ସତ୍ୟ);
            newVarp->direction(VDirection::ନିବେଶ);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        varp = it->second;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    template <typename T_ଗଣ୍ଠି>
    ଶୂନ୍ୟ fixupClassOrPackage(Ast_ଗଣ୍ଠି* memberp, T_ଗଣ୍ଠି refp) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ declClassp = VN_AS(memberp->user2p(), ଗଣ୍ଠି_Module);
        ଯଦି (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    template <typename T_ଗଣ୍ଠି>
    ତାର୍କିକ isReferenceToInnerMember(T_ଗଣ୍ଠି nodep) {
        ଫେରନ୍ତୁ VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_ଧାଡ଼ି* fl) {
        ଯଦି (!m_thisp) {
            AstClassRefDType* ଅଚଳ refDTypep
                = ନୂତନ AstClassRefDType{fl, VN_AS(m_callerp, Class), nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
            m_thisp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(ସତ୍ୟ);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::ନିବେଶ);
            m_argsp = Ast_ଗଣ୍ଠି::addNext(m_argsp, ନୂତନ AstArg{fl, "", ନୂତନ AstThisRef{fl, refDTypep}});
        }
        ଫେରନ୍ତୁ m_thisp;
    }

    AstVar* getVar(AstVar* ଅଚଳ varp) ଅଚଳ {
        ଅଚଳ auto it = m_varCloneMap.find(varp);
        ଯଦି (it == m_varCloneMap.end()) ଫେରନ୍ତୁ nullptr;
        ଯଦି (it->second->isStdRandomizeArg()) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ it->second;
    }

    CaptureMode getVarRefCaptureMode(Ast_ଗଣ୍ଠି_Var_Ref* varRefp) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ varModp = VN_AS(varRefp->varp()->user2p(), ଗଣ୍ଠି_Module);
        AstClass* ଅଚଳ varClassp = VN_CAST(varModp, Class);
        AstClass* ଅଚଳ callerClassp = VN_CAST(m_callerp, Class);

        ଅଚଳ ତାର୍କିକ callerIsClass = callerClassp;
        ଅଚଳ ତାର୍କିକ refIsXref = VN_IS(varRefp, VarXRef);
        ଅଚଳ ତାର୍କିକ varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ଅଚଳ ତାର୍କିକ varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ଅଚଳ ତାର୍କିକ varIsFieldOfCaller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ଅଚଳ ତାର୍କିକ varIsParam = varRefp->varp()->isParam();
        ଅଚଳ ତାର୍କିକ varIsConstraintIterator
            = VN_IS(varRefp->varp()->firstAbovep(), ForeachHeader)
              && VN_IS(varRefp->varp()->firstAbovep()->firstAbovep(), ConstraintForeach);
        ଯଦି (refIsXref) ଫେରନ୍ତୁ CaptureMode::CAP_ମୂଲ୍ୟ | CaptureMode::CAP_F_XREF;
        ଯଦି (varIsConstraintIterator) ଫେରନ୍ତୁ CaptureMode::CAP_NO;
        ଯଦି (varIsFuncLocal && varHasAutomaticLifetime) ଫେରନ୍ତୁ CaptureMode::CAP_ମୂଲ୍ୟ;
        ଯଦି (varIsParam) ଫେରନ୍ତୁ CaptureMode::CAP_ମୂଲ୍ୟ;
        // Static var in function (will not be inlined, because it's in class)
        ଯଦି (callerIsClass && varIsFuncLocal) ଫେରନ୍ତୁ CaptureMode::CAP_ମୂଲ୍ୟ;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        ଯଦି (m_staticContext && callerIsClass && varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            ଫେରନ୍ତୁ CaptureMode::CAP_NO;
        ଯଦି (callerIsClass && varIsFieldOfCaller) ଫେରନ୍ତୁ CaptureMode::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        ଯଦି (callerIsClass && varClassp && !varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            ଫେରନ୍ତୁ CaptureMode::CAP_ମୂଲ୍ୟ;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        ଫେରନ୍ତୁ CaptureMode::CAP_ମୂଲ୍ୟ;
    }

    ଶୂନ୍ୟ capture_Ref_By_ମୂଲ୍ୟ(Ast_ଗଣ୍ଠି_Var_Ref* nodep, CaptureMode capModeFlags) {
        AstVar* newVarp;
        ଅଚଳ ତାର୍କିକ newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ newVarRefp = newCapture ? nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା) : nullptr;
        ଯଦି (!hasFlags(capModeFlags, CaptureMode::CAP_F_SET_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(nullptr);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        ଯଦି (!newCapture) ଫେରନ୍ତୁ;
        ଯଦି (hasFlags(capModeFlags, CaptureMode::CAP_F_XREF)) {
            AstVarRef* ଅଚଳ notXVarRefp
                = ନୂତନ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_ଗଣ୍ଠି::addNext(m_argsp, ନୂତନ AstArg{nodep->fileline(), "", newVarRefp});
    }

    ଶୂନ୍ୟ captureRefByThis(Ast_ଗଣ୍ଠି_Var_Ref* nodep, CaptureMode /*capModeFlags*/) {
        AstVar* ଅଚଳ thisp = importThisp(nodep->fileline());
        AstVarRef* ଅଚଳ thisRefp = ନୂତନ AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(ସତ୍ୟ);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ଅଚଳ memberSelp
            = ନୂତନ AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        ଯଦି (!m_targetp && nodep->user1()) memberSelp->user1(ସତ୍ୟ);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (m_ignore.ଗଣନା_2(nodep)) ଫେରନ୍ତୁ;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        ଯଦି (nodep->varp()->isStdRandomizeArg()) ଫେରନ୍ତୁ;
        ଅଚଳ CaptureMode capMode = getVarRefCaptureMode(nodep);
        ଯଦି (mode(capMode) == CaptureMode::CAP_NO) ଫେରନ୍ତୁ;
        ଯଦି (mode(capMode) == CaptureMode::CAP_ମୂଲ୍ୟ) capture_Ref_By_ମୂଲ୍ୟ(nodep, capMode);
        ଯଦି (mode(capMode) == CaptureMode::CAP_THIS) captureRefByThis(nodep, capMode);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (m_ignore.ଗଣନା_2(nodep)) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        ଯଦି (VN_IS(nodep, MethodCall) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        AstClass* ଅଚଳ classp = VN_CAST(nodep->taskp()->user2p(), Class);
        ଯଦି ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ଅଚଳ argsp = nodep->argsp();
            ଯଦି (argsp) argsp->unlinkFrBack();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ଅଚଳ thisp = importThisp(nodep->fileline());
            AstVarRef* ଅଚଳ thisRefp = ନୂତନ AstVarRef{
                nodep->fileline(), thisp, nodep->isPure() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            AstMethodCall* ଅଚଳ methodCallp
                = ନୂତନ AstMethodCall{nodep->fileline(), thisRefp, thisp->name(), argsp};
            methodCallp->taskp(nodep->taskp());
            methodCallp->dtypep(nodep->dtypep());
            nodep->replaceWith(methodCallp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            m_ignore.emplace(methodCallp);
        }
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        ଯଦି (!isReferenceToInnerMember(nodep)) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        AstVarRef* ଅଚଳ varRefp
            = ନୂତନ AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    ଶୂନ୍ୟ visit(AstMethodCall* nodep) override {
        ଯଦି (!isReferenceToInnerMember(nodep) || m_ignore.ଗଣନା_2(nodep)) {
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }
        AstArg* ଅଚଳ argsp = nodep->argsp();
        ଯଦି (argsp) argsp->unlinkFrBackWithNext();
        Ast_ଗଣ୍ଠି_FTask_Ref* taskRefp = nullptr;
        ଯଦି (AstTask* ଅଚଳ taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = ନୂତନ AstTaskRef{nodep->fileline(), taskp, argsp};
        } ଅନ୍ୟଥା ଯଦି (AstFunc* ଅଚଳ taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = ନୂତନ AstFuncRef{nodep->fileline(), taskp, argsp};
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    explicit CaptureVisitor(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, Ast_ଗଣ୍ଠି_Module* callerp, AstClass* ଅଚଳ targetp,
                            ତାର୍କିକ staticContext = ମିଥ୍ୟା)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterateAndNextNull(nodep);
    }

    // PUBLIC METHODS

    AstArg* getArgs() ଅଚଳ { ଫେରନ୍ତୁ m_argsp; }
    AstVar* getThisp() ଅଚଳ { ଫେରନ୍ତୁ m_thisp; }

    ଶୂନ୍ୟ addFunctionArguments(Ast_ଗଣ୍ଠି_FTask* funcp) ଅଚଳ {
        ପାଇଁ (AstArg* argp = getArgs(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* varrefp = VN_CAST(argp->exprp(), ଗଣ୍ଠି_Var_Ref)) {
                ଯଦି ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(nullptr);
                }
                funcp->addStmtsp(getVar(varrefp->varp()));
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->addStmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

ଶ୍ରେଣୀ RandomizeVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
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
    ଅଚଳ VNUser3InUse m_inuser3;
    ଅଚଳ VNUser4InUse m_inuser4;

    // STATE
    V3UniqueNames m_inlineUniqueNames;  // For generating unique function names
    V3UniqueNames m_randArrayUniqueNames{"__Vrandarr"};  // Random array loop indices
    V3UniqueNames m_modeUniqueNames{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3UniqueNames m_inlineUniqueStdName{"__VStdrand"};
    V3UniqueNames m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    std::unordered_map<Ast_ଗଣ୍ଠି_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ଗଣ୍ଠି_Stmt* m_stmtp = nullptr;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = nullptr;  // Dynamic array type (for rand mode)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_rand_Case_ଅଙ୍କ = 0;  // Randcase number within a module for var naming
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_dist_ଅଙ୍କ = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = nullptr;  // Current constraint
    std::set<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertionSet<AstVar*>> m_sizeConstrainedArrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_staticConstraintModeVars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_staticRandModeVars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<ତାର୍କିକ, ତାର୍କିକ>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ସ୍ଥିର ତାର୍କିକ is_Similar_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* with_ପଦସମଷ୍ଟି, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* arg_ପଦସମଷ୍ଟି) {
        // VarRef: compare variable pointers
        ଯଦି (VN_IS(arg_ପଦସମଷ୍ଟି, VarRef) && VN_IS(with_ପଦସମଷ୍ଟି, VarRef)) {
            ଫେରନ୍ତୁ VN_AS(with_ପଦସମଷ୍ଟି, VarRef)->varp() == VN_AS(arg_ପଦସମଷ୍ଟି, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        ଯଦି (VN_IS(arg_ପଦସମଷ୍ଟି, MemberSel) && VN_IS(with_ପଦସମଷ୍ଟି, MemberSel)) {
            ଅଚଳ AstMemberSel* ଅଚଳ withMSp = VN_AS(with_ପଦସମଷ୍ଟି, MemberSel);
            ଅଚଳ AstMemberSel* ଅଚଳ argMSp = VN_AS(arg_ପଦସମଷ୍ଟି, MemberSel);
            ଯଦି (withMSp->varp() != argMSp->varp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Recursively compare the base object expression
            ଫେରନ୍ତୁ is_Similar_ଗଣ୍ଠି(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        ଯଦି (VN_IS(arg_ପଦସମଷ୍ଟି, ArraySel) && VN_IS(with_ପଦସମଷ୍ଟି, ArraySel)) {
            ଅଚଳ AstArraySel* ଅଚଳ withASp = VN_AS(with_ପଦସମଷ୍ଟି, ArraySel);
            ଅଚଳ AstArraySel* ଅଚଳ argASp = VN_AS(arg_ପଦସମଷ୍ଟି, ArraySel);
            // Before checking Sel type index, check if both are Const
            ଯଦି (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ଅଚଳ AstConst* ଅଚଳ withIdxp = VN_AS(withASp->bitp(), Const);
                ଅଚଳ AstConst* ଅଚଳ argIdxp = VN_AS(argASp->bitp(), Const);
                ଫେରନ୍ତୁ is_Similar_ଗଣ୍ଠି(withASp->fromp(), argASp->fromp())
                       && withIdxp->ଅଙ୍କ().isCaseEq(argIdxp->ଅଙ୍କ());
            }
            // Index must be Sel type, extract VarRef using fromp()
            ଯଦି (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            ଫେରନ୍ତୁ is_Similar_ଗଣ୍ଠି(withASp->fromp(), argASp->fromp())
                   && is_Similar_ଗଣ୍ଠି(withIdxp, argIdxp);
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ସ୍ଥିର ଶୂନ୍ୟ expandUniqueElementList(Ast_ଗଣ୍ଠି* itemsp) {
        Ast_ଗଣ୍ଠି* itemp = itemsp;
        ଯେତେବେଳେ (itemp) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = itemp->nextp();
            ଯଦି (ଅଚଳ AstConstraintIf* ଅଚଳ ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                ଜାରି;
            }
            ଯଦି (ଅଚଳ AstConstraintForeach* ଅଚଳ foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                ଜାରି;
            }
            AstConstraintUnique* ଅଚଳ uniquep = VN_CAST(itemp, ConstraintUnique);
            ଯଦି (!uniquep) {
                itemp = nextp;
                ଜାରି;
            }

            std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> ପଦସମଷ୍ଟି_ବସ୍ତୁଗୁଡ଼ିକ;
            ତାର୍କିକ hasArrayVarRef = ମିଥ୍ୟା;
            ପାଇଁ (Ast_ଗଣ୍ଠି* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = vrp->varp()->dtypep()->skipRefp();
                    ଯଦି (VN_IS(dtypep, ଗଣ୍ଠି_Array_DType) || VN_IS(dtypep, QueueDType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = ସତ୍ୟ;
                        ଜାରି;
                    }
                }
                ପଦସମଷ୍ଟି_ବସ୍ତୁଗୁଡ଼ିକ.ଠେଲ_back(VN_AS(rp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
            }

            ଯଦି (ପଦସମଷ୍ଟି_ବସ୍ତୁଗୁଡ଼ିକ.size() >= 2) {
                File_ଧାଡ଼ି* ଅଚଳ fl = uniquep->fileline();
                ପାଇଁ (size_t i = 0; i < ପଦସମଷ୍ଟି_ବସ୍ତୁଗୁଡ଼ିକ.size(); i++) {
                    ପାଇଁ (size_t j = i + 1; j < ପଦସମଷ୍ଟି_ବସ୍ତୁଗୁଡ଼ିକ.size(); j++) {
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ପଦସମଷ୍ଟି_ବସ୍ତୁଗୁଡ଼ିକ[i]->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ପଦସମଷ୍ଟି_ବସ୍ତୁଗୁଡ଼ିକ[j]->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                        AstNeq* ଅଚଳ neqp = ନୂତନ AstNeq{fl, lhsp, rhsp};
                        neqp->user1(ସତ୍ୟ);
                        Ast_Constraint_ପଦସମଷ୍ଟି* ଅଚଳ cexprp = ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, neqp};
                        uniquep->addNextHere(cexprp);
                    }
                }
                ଯଦି (!hasArrayVarRef) {
                    uniquep->unlinkFrBack();
                    VL_DO_DANGLING(uniquep->delete_ବୃକ୍ଷ(), uniquep);
                }
            } ଅନ୍ୟଥା ଯଦି (ପଦସମଷ୍ଟି_ବସ୍ତୁଗୁଡ଼ିକ.size() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlinkFrBack();
                VL_DO_DANGLING(uniquep->delete_ବୃକ୍ଷ(), uniquep);
            }
            itemp = nextp;
        }
    }
    ଶୂନ୍ୟ createRandomGenerator(AstClass* ଅଚଳ classp) {
        ଯଦି (classp->user3p()) ଫେରନ୍ତୁ;
        ଯଦି (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            ଫେରନ୍ତୁ;
        }
        AstVar* ଅଚଳ genp = ନୂତନ AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->addMembersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp) {
        auto it = m_stdMap.find(modp);
        ଯଦି (it == m_stdMap.end()) {
            AstVar* ଅଚଳ stdgenp
                = ନୂତନ AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->findBasicDType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3ErrorCode::IMPURE, ସତ୍ୟ);
            modp->addStmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            ଫେରନ୍ତୁ stdgenp;
        }
        ଫେରନ୍ତୁ it->second;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ଅଚଳ classp) {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ name = "__Vprepare_constrained_arrays";
        AstTask* ଅଚଳ taskp = ନୂତନ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(ସତ୍ୟ);
        classp->addMembersp(taskp);
        m_memberMap.ଭର୍ତ୍ତି(classp, taskp);
        ଫେରନ୍ତୁ taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ଅଚଳ classp) {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.findMember(classp, name), Task);
        ଯଦି (resizeTaskp) ଫେରନ୍ତୁ resizeTaskp;
        resizeTaskp = ନୂତନ AstTask{classp->fileline(), name, nullptr};
        resizeTaskp->classMethod(ସତ୍ୟ);
        resizeTaskp->isVirtual(ସତ୍ୟ);
        classp->addMembersp(resizeTaskp);
        m_memberMap.ଭର୍ତ୍ତି(classp, resizeTaskp);
        ଫେରନ୍ତୁ resizeTaskp;
    }
    AstVar* getCreateRandModeVar(AstClass* ଅଚଳ classp) {
        ଯଦି (classp->user2p()) ଫେରନ୍ତୁ VN_AS(classp->user2p(), Var);
        ଯଦି (AstClassExtends* ଅଚଳ extendsp = classp->extendsp()) {
            ଫେରନ୍ତୁ getCreateRandModeVar(extendsp->classp());
        }
        AstVar* ଅଚଳ randModeVarp = createModeVar(classp, "__Vrandmode");
        classp->user2p(randModeVarp);
        ଫେରନ୍ତୁ randModeVarp;
    }
    AstVar* getCreateConstraintModeVar(AstClass* ଅଚଳ classp) {
        ଯଦି (classp->user4p()) ଫେରନ୍ତୁ VN_AS(classp->user4p(), Var);
        ଯଦି (AstClassExtends* ଅଚଳ extendsp = classp->extendsp()) {
            ଫେରନ୍ତୁ getCreateConstraintModeVar(extendsp->classp());
        }
        AstVar* ଅଚଳ constraintModeVarp = createModeVar(classp, "__Vconstraintmode");
        classp->user4p(constraintModeVarp);
        ଫେରନ୍ତୁ constraintModeVarp;
    }
    ସ୍ଥିର AstVar* getConstraintModeVar(AstClass* ଅଚଳ classp) {
        ଯଦି (classp->user4p()) ଫେରନ୍ତୁ VN_AS(classp->user4p(), Var);
        ଯଦି (AstClassExtends* ଅଚଳ extendsp = classp->extendsp()) {
            ଫେରନ୍ତୁ getConstraintModeVar(extendsp->classp());
        }
        ଫେରନ୍ତୁ nullptr;
    }
    AstVar* getCreateStaticConstraintModeVar(AstClass* ଅଚଳ classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        ଯଦି (it != m_staticConstraintModeVars.end()) ଫେରନ୍ତୁ it->second;
        ଯଦି (AstClassExtends* ଅଚଳ extendsp = classp->extendsp()) {
            ଫେରନ୍ତୁ getCreateStaticConstraintModeVar(extendsp->classp());
        }
        AstVar* ଅଚଳ staticModeVarp = createStaticModeVar(classp, "__Vstaticconstraintmode");
        m_staticConstraintModeVars[classp] = staticModeVarp;
        ଫେରନ୍ତୁ staticModeVarp;
    }
    AstVar* getStaticConstraintModeVar(AstClass* ଅଚଳ classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        ଯଦି (it != m_staticConstraintModeVars.end()) ଫେରନ୍ତୁ it->second;
        ଯଦି (AstClassExtends* ଅଚଳ extendsp = classp->extendsp()) {
            ଫେରନ୍ତୁ getStaticConstraintModeVar(extendsp->classp());
        }
        ଫେରନ୍ତୁ nullptr;
    }
    AstVar* getCreateStaticRandModeVar(AstClass* ଅଚଳ classp) {
        ଯଦି (m_staticRandModeVars.ଗଣନା_2(classp)) ଫେରନ୍ତୁ m_staticRandModeVars[classp];
        ଯଦି (AstClassExtends* ଅଚଳ extendsp = classp->extendsp()) {
            ଫେରନ୍ତୁ getCreateStaticRandModeVar(extendsp->classp());
        }
        AstVar* ଅଚଳ staticModeVarp = createStaticModeVar(classp, "__Vstaticrandmode");
        m_staticRandModeVars[classp] = staticModeVarp;
        ଫେରନ୍ତୁ staticModeVarp;
    }
    AstVar* getStaticRandModeVar(AstClass* ଅଚଳ classp) {
        ଯଦି (m_staticRandModeVars.ଗଣନା_2(classp)) ଫେରନ୍ତୁ m_staticRandModeVars[classp];
        ଯଦି (AstClassExtends* ଅଚଳ extendsp = classp->extendsp()) {
            ଫେରନ୍ତୁ getStaticRandModeVar(extendsp->classp());
        }
        ଫେରନ୍ତୁ nullptr;
    }
    AstVar* createModeVar(AstClass* ଅଚଳ classp, ଅଚଳ ଅକ୍ଷର* ଅଚଳ name) {
        File_ଧାଡ଼ି* ଅଚଳ fl = classp->fileline();
        ଯଦି (!m_dynarrayDtp) {
            m_dynarrayDtp = ନୂତନ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ଅଚଳ modeVarp = ନୂତନ AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        ଫେରନ୍ତୁ modeVarp;
    }
    AstVar* createStaticModeVar(AstClass* ଅଚଳ classp, ଅଚଳ ଅକ୍ଷର* ଅଚଳ name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_ଧାଡ଼ି* ଅଚଳ fl = classp->fileline();
        ଯଦି (!m_dynarrayDtp) {
            m_dynarrayDtp = ନୂତନ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        AstVar* ଅଚଳ modeVarp = ନୂତନ AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        modeVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.ଭର୍ତ୍ତି(classp, modeVarp);
        ଫେରନ୍ତୁ modeVarp;
    }
    ସ୍ଥିର ଶୂନ୍ୟ addSetRandMode(Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp, AstVar* ଅଚଳ genp,
                               AstVar* ଅଚଳ randModeVarp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = ftaskp->fileline();
        AstCMethodHard* ଅଚଳ setRandModep = ନୂତନ AstCMethodHard{
            fl, ନୂତନ AstVarRef{fl, VN_AS(genp->user2p(), ଗଣ୍ଠି_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_SET_RANDMODE,
            ନୂତନ AstVarRef{fl, VN_AS(randModeVarp->user2p(), ଗଣ୍ଠି_Module), randModeVarp,
                          VAccess::READ}};
        setRandModep->dtypeSetVoid();
        ftaskp->addStmtsp(setRandModep->makeStmt());
    }
    ସ୍ଥିର ଶୂନ୍ୟ addSetStaticRandMode(Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp, AstVar* ଅଚଳ genp,
                                     AstVar* ଅଚଳ staticRandModeVarp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = ftaskp->fileline();
        AstCMethodHard* ଅଚଳ setp = ନୂତନ AstCMethodHard{
            fl, ନୂତନ AstVarRef{fl, VN_AS(genp->user2p(), ଗଣ୍ଠି_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_SET_STATIC_RANDMODE,
            ନୂତନ AstVarRef{fl, VN_AS(staticRandModeVarp->user2p(), ଗଣ୍ଠି_Module), staticRandModeVarp,
                          VAccess::READ}};
        setp->dtypeSetVoid();
        ftaskp->addStmtsp(setp->makeStmt());
    }
    ଶୂନ୍ୟ createRandomizeClassVars(AstNetlist* ଅଚଳ netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> ମୂଳ_Static_Rand_Mode_ଗଣନା;
        netlistp->foreach([ଏହି, &ମୂଳ_Static_Rand_Mode_ଗଣନା](AstClass* ଅଚଳ classp) {
            ତାର୍କିକ hasConstraints = ମିଥ୍ୟା;
            uint32_t rand_Mode_ଗଣନା = 0;
            uint32_t static_Rand_Mode_ଗଣନା = 0;
            uint32_t constraint_Mode_ଗଣନା = 0;
            uint32_t static_Constraint_Mode_ଗଣନା = 0;
            classp->foreachMember([&](AstClass*, Ast_ଗଣ୍ଠି* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                ଯଦି (AstConstraint* ଅଚଳ constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = ସତ୍ୟ;
                    RandomizeMode constraintMode = {.asUQuad = memberp->user1()};
                    ଯଦି (!constraintMode.usesMode) ଫେରନ୍ତୁ;
                    ଯଦି (constraintMode.index == 0) {
                        // Use separate index counters for static vs non-static constraints
                        ଯଦି (constrp->isStatic()) {
                            constraintMode.index = static_Constraint_Mode_ଗଣନା++;
                        } ଅନ୍ୟଥା {
                            constraintMode.index = constraint_Mode_ଗଣନା++;
                        }
                        memberp->user1(constraintMode.asUQuad);
                    } ଅନ୍ୟଥା {
                        ଯଦି (constrp->isStatic()) {
                            static_Constraint_Mode_ଗଣନା = constraintMode.index + 1;
                        } ଅନ୍ୟଥା {
                            constraint_Mode_ଗଣନା = constraintMode.index + 1;
                        }
                    }
                } ଅନ୍ୟଥା ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(memberp, Var)) {
                    RandomizeMode randMode = {.asUQuad = memberp->user1()};
                    ଯଦି (!randMode.usesMode) ଫେରନ୍ତୁ;
                    ଅଚଳ ତାର୍କିକ isStaticVar = varp->lifetime().isStatic();
                    ଯଦି (randMode.index == 0) {
                        ଯଦି (isStaticVar) {
                            randMode.index = static_Rand_Mode_ଗଣନା++;
                        } ଅନ୍ୟଥା {
                            randMode.index = rand_Mode_ଗଣନା++;
                        }
                        memberp->user1(randMode.asUQuad);
                    } ଅନ୍ୟଥା {
                        ଯଦି (isStaticVar) {
                            static_Rand_Mode_ଗଣନା = randMode.index + 1;
                        } ଅନ୍ୟଥା {
                            rand_Mode_ଗଣନା = randMode.index + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<ଶୂନ୍ୟ(AstClass*)> findSubObjRandModes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_ଗଣ୍ଠି* subMemberp) {
                        ଯଦି (AstVar* ଅଚଳ subVarp = VN_CAST(subMemberp, Var)) {
                            ଅଚଳ RandomizeMode rm = {.asUQuad = subVarp->user1()};
                            ଯଦି (!rm.usesMode) ଫେରନ୍ତୁ;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            ଯଦି (subVarp->lifetime().isStatic()) ଫେରନ୍ତୁ;
                            ଅଚଳ uint32_t needed = rm.index + 1;
                            ଯଦି (needed > rand_Mode_ଗଣନା) rand_Mode_ଗଣନା = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    ଯଦି (!memberVarp->globalConstrained()) ଫେରନ୍ତୁ;
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = memberVarp->dtypep()->skipRefp();
                    ଅଚଳ AstClassRefDType* ଅଚଳ classRefp = VN_CAST(dtypep, ClassRefDType);
                    ଯଦି (!classRefp) ଫେରନ୍ତୁ;
                    findSubObjRandModes(classRefp->classp());
                });
            }
            ଯଦି (hasConstraints) createRandomGenerator(classp);
            ଯଦି (rand_Mode_ଗଣନା > 0) {
                AstVar* ଅଚଳ randModeVarp = getCreateRandModeVar(classp);
                makeModeInit(randModeVarp, classp, rand_Mode_ଗଣନା);
            }
            ଯଦି (constraint_Mode_ଗଣନା > 0) {
                AstVar* ଅଚଳ constraintModeVarp = getCreateConstraintModeVar(classp);
                makeModeInit(constraintModeVarp, classp, constraint_Mode_ଗଣନା);
            }
            ଯଦି (static_Constraint_Mode_ଗଣନା > 0) {
                AstVar* ଅଚଳ staticConstraintModeVarp = getCreateStaticConstraintModeVar(classp);
                makeStaticModeInit(staticConstraintModeVarp, classp, static_Constraint_Mode_ଗଣନା);
            }
            ଯଦି (static_Rand_Mode_ଗଣନା > 0) {
                getCreateStaticRandModeVar(classp);
                AstClass* rootp = classp;
                ଯେତେବେଳେ (AstClassExtends* ଅଚଳ ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& slot = ମୂଳ_Static_Rand_Mode_ଗଣନା[rootp];
                ଯଦି (static_Rand_Mode_ଗଣନା > slot) slot = static_Rand_Mode_ଗଣନା;
            }
        });
        ପାଇଁ (ଅଚଳ auto& kv : ମୂଳ_Static_Rand_Mode_ଗଣନା) emit_ମୂଳ_Static_Mode_Init(kv.first, kv.second);
    }
    ଶୂନ୍ୟ emit_ମୂଳ_Static_Mode_Init(AstClass* ଅଚଳ rootp, ଅଚଳ uint32_t ଗଣନା_2) {
        AstVar* ଅଚଳ staticRandModeVarp = m_staticRandModeVars[rootp];
        UASSERT_OBJ(staticRandModeVarp, rootp, "Root must have a static rand-mode var");
        makeStaticModeInit(staticRandModeVarp, rootp, ଗଣନା_2);
    }
    ଶୂନ୍ୟ makeModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_ଗଣନା) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ modeVarModp = VN_AS(modeVarp->user2p(), ଗଣ୍ଠି_Module);
        File_ଧାଡ଼ି* ଅଚଳ fl = modeVarp->fileline();
        AstVar* ଅଚଳ oldSizeVarp = ନୂତନ AstVar{
            fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__VoldSize"), modeVarp->findUInt32DType()};
        oldSizeVarp->funcLocal(ସତ୍ୟ);
        oldSizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ଅଚଳ oldSizep
            = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        oldSizep->dtypeSetUInt32();
        AstCMethodHard* ଅଚଳ dynarrayNewp
            = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, ନୂତନ AstConst{fl, mode_ଗଣନା}};
        dynarrayNewp->dtypeSetVoid();
        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ctorNewp = VN_AS(m_memberMap.findMember(classp, "new"), ଗଣ୍ଠି_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_ଗଣ୍ଠି* ଅଚଳ initFirstp = oldSizeVarp;
        initFirstp->addNext(
            ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, oldSizeVarp, VAccess::WRITE}, oldSizep});
        initFirstp->addNext(dynarrayNewp->makeStmt());
        initFirstp->addNext(makeModeSetLoop(
            fl, ନୂତନ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE}, ନୂତନ AstConst{fl, 1},
            ସତ୍ୟ, ନୂତନ AstVarRef{fl, oldSizeVarp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (!VN_IS(stmtp, ଗଣ୍ଠି_Stmt)) ଜାରି;  // Skip var declarations
            ଯଦି (ଅଚଳ Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ sep = VN_CAST(stmtp, Stmt_ପଦସମଷ୍ଟି)) {
                ଯଦି (VN_IS(sep->exprp(), New)) ଜାରି;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->addHereThisAsNext(initFirstp);
            ଫେରନ୍ତୁ;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->addStmtsp(initFirstp);
    }
    ଶୂନ୍ୟ makeStaticModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_ଗଣନା) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ modeVarModp = VN_AS(modeVarp->user2p(), ଗଣ୍ଠି_Module);
        File_ଧାଡ଼ି* ଅଚଳ fl = modeVarp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ଅଚଳ sizep
            = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstEq* ଅଚଳ condp = ନୂତନ AstEq{fl, sizep, ନୂତନ AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ଅଚଳ dynarrayNewp
            = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, ନୂତନ AstConst{fl, mode_ଗଣନା}};
        dynarrayNewp->dtypeSetVoid();
        Ast_ଗଣ୍ଠି* ଅଚଳ thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->addNext(
            makeModeSetLoop(fl, ନୂତନ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                            ନୂତନ AstConst{fl, 1}, ସତ୍ୟ));

        // Build the if statement
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newp = VN_AS(m_memberMap.findMember(classp, "new"), ଗଣ୍ଠି_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->addStmtsp(ifp);
    }
    Ast_ଗଣ୍ଠି* makeModeSetLoop(File_ଧାଡ଼ି* ଅଚଳ fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp,
                             ତାର୍କିକ inTask, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ startp = nullptr) {
        AstVar* ଅଚଳ iterVarp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->findUInt32DType()};
        iterVarp->funcLocal(inTask);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ଅଚଳ sizep = ନୂତନ AstCMethodHard{fl, lhsp, VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstCMethodHard* ଅଚଳ setp
            = ନୂତନ AstCMethodHard{fl, lhsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), VCMethod::ARRAY_AT_WRITE,
                                 ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtypeSetUInt32();
        Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = iterVarp;
        stmtsp->addNext(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : ନୂତନ AstConst{fl, 0}});

        AstLoop* ଅଚଳ loopp = ନୂତନ AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(ନୂତନ AstLoopTest{
            fl, loopp, ନୂତନ AstLt{fl, ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->addStmtsp(ନୂତନ AstAssign{fl, setp, rhsp});
        loopp->addStmtsp(ନୂତନ AstAssign{
            fl, ନୂତନ AstVarRef{fl, iterVarp, VAccess::WRITE},
            ନୂତନ AstAdd{fl, ନୂତନ AstConst{fl, 1}, ନୂତନ AstVarRef{fl, iterVarp, VAccess::READ}}});
        ଫେରନ୍ତୁ ନୂତନ AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtsp, ସତ୍ୟ};
    }
    ଶୂନ୍ୟ addTempVarsp(AstVar* varsp) {
        ପାଇଁ (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ଯଦି (m_ftaskp) {
                varp->funcLocal(ସତ୍ୟ);
            } ଅନ୍ୟଥା {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        ଯଦି (!m_ftaskp) {
            m_modp->addStmtsp(varsp);
        } ଅନ୍ୟଥା {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_ଗଣ୍ଠି_Stmt* wrapIfRandMode(AstClass* classp, AstVar* ଅଚଳ varp, Ast_ଗଣ୍ଠି_Stmt* stmtp) {
        ଅଚଳ RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstVar* ଅଚଳ modeVarp = varp->lifetime().isStatic() ? getStaticRandModeVar(classp)
                                                             : getRandModeVarFromClass(classp);
        ଫେରନ୍ତୁ VN_AS(wrapIfMode(rmode, modeVarp, stmtp), ଗଣ୍ଠି_Stmt);
    }
    Ast_ଗଣ୍ଠି* wrapIfConstraintMode(AstClass* classp, AstConstraint* ଅଚଳ constrp, Ast_ଗଣ୍ଠି* stmtp) {
        ଅଚଳ RandomizeMode rmode = {.asUQuad = constrp->user1()};
        AstVar* ଅଚଳ modeVarp = constrp->isStatic() ? getStaticConstraintModeVar(classp)
                                                     : getConstraintModeVar(classp);
        ଫେରନ୍ତୁ wrapIfMode(rmode, modeVarp, stmtp);
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି* wrapIfMode(ଅଚଳ RandomizeMode mode, AstVar* modeVarp, Ast_ଗଣ୍ଠି* stmtp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = stmtp->fileline();
        ଯଦି (mode.usesMode) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_AS(modeVarp->user2p(), ଗଣ୍ଠି_Module);
            AstCMethodHard* ଅଚଳ atp
                = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, modp, modeVarp, VAccess::READ},
                                     VCMethod::ARRAY_AT, ନୂତନ AstConst{fl, mode.index}};
            atp->dtypeSetUInt32();
            ଫେରନ୍ତୁ ନୂତନ AstIf{fl, atp, stmtp};
        }
        ଫେରନ୍ତୁ stmtp;
    }
    AstVarRef* enum_ମୂଲ୍ୟ_Tab_Refp(AstEnumDType* ଅଚଳ nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        ଯଦି (nodep->user2p()) ଫେରନ୍ତୁ VN_AS(nodep->user2p(), VarRef)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ vardtypep = ନୂତନ AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            ନୂତନ AstRange{nodep->fileline(), static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(nodep->item_ଗଣନା()) - 1, 0}};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        AstConsArray* ଅଚଳ consp = ନୂତନ AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t index = 0;
        ପାଇଁ (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ଅଚଳ vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(index++, vconstp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        }
        // Share identical tables via the constant pool
        AstVarRef* ଅଚଳ refp = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_ବୃକ୍ଷ(), consp);  // V3ConstPool::find clones it
        ଠେଲ_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        ଫେରନ୍ତୁ refp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
    }

    AstCDType* findVlRandCDType(File_ଧାଡ଼ି* ଅଚଳ fl, uint64_t ବସ୍ତୁଗୁଡ଼ିକ) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ଅଚଳ std::string type = AstCDType::typeToHold(V3_ସଂଖ୍ୟା::log2bQuad(ବସ୍ତୁଗୁଡ଼ିକ) + 1);
        ଅଚଳ std::string name = "VlRandC<" + type + ", " + cvtToStr(ବସ୍ତୁଗୁଡ଼ିକ) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ଅଚଳ auto pair = m_randcDtypes.emplace(name, nullptr);
        ଯଦି (pair.second) {
            AstCDType* ଅଚଳ newp = ନୂତନ AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            pair.first->second = newp;
        }
        ଫେରନ୍ତୁ pair.first->second;
    }

    AstVar* newRandcVarsp(AstVar* ଅଚଳ varp, AstClass* ଅଚଳ classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        ଯଦି (!varp->isRandC()) ଫେରନ୍ତୁ nullptr;

        uint64_t ବସ୍ତୁଗୁଡ଼ିକ = 0;
        ଯଦି (AstEnumDType* ଅଚଳ enumDtp = VN_CAST(varp->dtypep()->skipRefToEnump(), EnumDType)) {
            ବସ୍ତୁଗୁଡ଼ିକ = static_cast<uint64_t>(enumDtp->item_ଗଣନା());
        } ଅନ୍ୟଥା ଯଦି (AstBasicDType* ଅଚଳ basicp = varp->dtypep()->skipRefp()->basicp()) {
            ଯଦି (basicp->width() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                ଯଦି (!V3Error::error_ଗଣନା()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->width() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                ଫେରନ୍ତୁ nullptr;
            }
            ବସ୍ତୁଗୁଡ଼ିକ = 1ULL << basicp->width();
        } ଅନ୍ୟଥା ଯଦି (AstStructDType* ଅଚଳ dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            ଫେରନ୍ତୁ nullptr;
        } ଅନ୍ୟଥା {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_ଗଣନା(), varp, "Unexpected randc variable dtype");
            ଫେରନ୍ତୁ nullptr;
        }
        AstCDType* ଅଚଳ newdtp = findVlRandCDType(varp->fileline(), ବସ୍ତୁଗୁଡ଼ିକ);
        AstVar* ଅଚଳ newp
            = ନୂତନ AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->isInternal(ସତ୍ୟ);
        classp->addStmtsp(newp);
        UINFO(9, "created " << varp);
        ଫେରନ୍ତୁ newp;
    }
    Ast_ଗଣ୍ଠି_Stmt* createArrayForeachLoop(File_ଧାଡ଼ି* ଅଚଳ fl, Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep,
                                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstVar* ଅଚଳ outputVarp) {
        Ast_ଗଣ୍ଠି_DType* tempDTypep = dtypep;
        AstVar* randLoopIndxp = nullptr;
        auto createLoopIndex = [&](Ast_ଗଣ୍ଠି_DType* tempDTypep) {
            ଯଦି (VN_IS(tempDTypep, AssocArrayDType)) {
                ଫେରନ୍ତୁ ନୂତନ AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                                  VN_AS(tempDTypep, AssocArrayDType)->ଚାବି_DTypep()};
            }
            ଫେରନ୍ତୁ ନୂତନ AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                              dtypep->findBasicDType(VBasicDTypeKwd::UINT32)};
        };
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* tempElementp = nullptr;
        ଯେତେବେଳେ (tempDTypep->isNonPackedArray()) {
            AstVar* ଅଚଳ newRandLoopIndxp = createLoopIndex(tempDTypep);
            randLoopIndxp = Ast_ଗଣ୍ଠି::addNext(randLoopIndxp, newRandLoopIndxp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ tempExprp = tempElementp ? tempElementp : exprp;
            AstVarRef* ଅଚଳ tempRefp = ନୂତନ AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            ଯଦି (VN_IS(tempDTypep, DynArrayDType)) {
                tempElementp
                    = ନୂତନ AstCMethodHard{fl, tempExprp, VCMethod::ARRAY_AT_WRITE, tempRefp};
            } ଅନ୍ୟଥା ଯଦି (VN_IS(tempDTypep, UnpackArrayDType)) {
                Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ aryDTypep = VN_CAST(tempDTypep, ଗଣ୍ଠି_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                tempElementp = ନୂତନ AstArraySel{
                    fl, tempExprp,
                    ନୂତନ AstSel{
                        fl,
                        ନୂତନ AstSub{fl, tempRefp,
                                   ନୂତନ AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        ନୂତନ AstConst{fl, 0}, V3_ସଂଖ୍ୟା::log2b(aryDTypep->hi()) + 1}};
            } ଅନ୍ୟଥା ଯଦି (VN_IS(tempDTypep, AssocArrayDType)) {
                tempElementp = ନୂତନ AstAssocSel{fl, tempExprp, tempRefp};
            } ଅନ୍ୟଥା ଯଦି (VN_IS(tempDTypep, QueueDType)) {
                tempElementp
                    = ନୂତନ AstCMethodHard{fl, tempExprp, VCMethod::DYN_AT_WRITE_APPEND, tempRefp};
            }
            tempElementp->dtypep(tempDTypep->subDTypep());
            tempDTypep = tempDTypep->virtRefDTypep();
        }

        AstForeachHeader* ଅଚଳ headerp
            = ନୂତନ AstForeachHeader{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), randLoopIndxp};
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ randStmtsp = newRandStmtsp(fl, tempElementp, nullptr, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        ଯଦି (!tempElementp->backp()) VL_DO_DANGLING(ଠେଲ_Deletep(tempElementp), tempElementp);
        ଫେରନ୍ତୁ ନୂତନ AstForeach{fl, headerp, randStmtsp};
    }
    Ast_ଗଣ୍ଠି_Stmt* newRandStmtsp(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstVar* randcVarp,
                               AstVar* ଅଚଳ outputVarp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା offset = 0,
                               AstMemberDType* memberp = nullptr) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        ଯଦି (ଅଚଳ auto* ଅଚଳ structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_ଗଣ୍ଠି_Stmt* stmtsp = nullptr;
            ଯଦି (structDtp->packed()) offset += memberp ? memberp->lsb() : 0;
            ପାଇଁ (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_ଗଣ୍ଠି_Stmt* randp = nullptr;
                ଯଦି (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା) : exprp, nullptr,
                                          outputVarp, offset, smemberp);
                } ଅନ୍ୟଥା ଯଦି (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = ନୂତନ AstStructSel{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), smemberp->name()};
                    structSelp->dtypep(smemberp->childDTypep());
                    ଯଦି (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, nullptr, outputVarp);
                }
                stmtsp = Ast_ଗଣ୍ଠି::addNextNull(stmtsp, randp);
            }
            ଫେରନ୍ତୁ stmtsp;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ auto* ଅଚଳ unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            ଯଦି (!unionDtp->packed()) ଫେରନ୍ତୁ nullptr;
            AstMemberDType* ଅଚଳ firstMemberp = unionDtp->membersp();
            ଫେରନ୍ତୁ newRandStmtsp(fl, exprp, nullptr, outputVarp, offset, firstMemberp);
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ଅଚଳ memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            AstMethodCall* ଅଚଳ callp = ନୂତନ AstMethodCall{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            AstAssign* ଅଚଳ assignp = ନୂତନ AstAssign{
                fl, ନୂତନ AstVarRef{fl, outputVarp, VAccess::WRITE},
                ନୂତନ AstAnd{fl, ନୂତନ AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            ଫେରନ୍ତୁ ନୂତନ AstIf{
                fl, ନୂତନ AstNeq{fl, exprp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ନୂତନ AstConst{fl, AstConst::Null{}}},
                assignp};
        } ଅନ୍ୟଥା ଯଦି (AstDynArrayDType* ଅଚଳ dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            ଫେରନ୍ତୁ createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } ଅନ୍ୟଥା ଯଦି (AstQueueDType* ଅଚଳ queueDtp = VN_CAST(memberDtp, QueueDType)) {
            ଫେରନ୍ତୁ createArrayForeachLoop(fl, queueDtp, exprp, outputVarp);
        } ଅନ୍ୟଥା ଯଦି (AstUnpackArrayDType* ଅଚଳ unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            ଫେରନ୍ତୁ createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } ଅନ୍ୟଥା ଯଦି (AstAssocArrayDType* ଅଚଳ assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            ଫେରନ୍ତୁ createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valp;
            ଯଦି (AstEnumDType* ଅଚଳ enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ଅଚଳ tabRefp = enum_ମୂଲ୍ୟ_Tab_Refp(enumDtp);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ randp
                    = new_Rand_ମୂଲ୍ୟ(fl, randcVarp, exprp->findBasicDType(VBasicDTypeKwd::UINT32));
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ moddivp = ନୂତନ AstModDiv{
                    fl, randp, ନୂତନ AstConst{fl, static_cast<uint32_t>(enumDtp->item_ଗଣନା())}};
                moddivp->dtypep(enumDtp);
                valp = ନୂତନ AstArraySel{fl, tabRefp, moddivp};
            } ଅନ୍ୟଥା {
                valp
                    = new_Rand_ମୂଲ୍ୟ(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            AstAssign* assignp
                = ନୂତନ AstAssign{fl,
                                ନୂତନ AstSel{fl, exprp, offset + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->width() : exprp->width()},
                                valp};
            AstVar* varp = nullptr;
            exprp->exists([&](ଅଚଳ AstVarRef* varrefp) {
                ଯଦି (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                ଫେରନ୍ତୁ varp != nullptr;
            });
            ଫେରନ୍ତୁ wrapIfRandMode(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* new_Rand_ମୂଲ୍ୟ(File_ଧାଡ଼ି* ଅଚଳ fl, AstVar* ଅଚଳ randcVarp,
                              Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        ଯଦି (randcVarp) {
            AstCExpr* ଅଚଳ cexprp = ନୂତନ AstCExpr{fl};
            cexprp->add(ନୂତନ AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->add(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            ଫେରନ୍ତୁ cexprp;
        }

        ଫେରନ୍ତୁ ନୂତନ AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* findPrePostTask(AstClass* classp, ଅଚଳ string& name) {
        ପାଇଁ (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            ଯଦି (AstTask* ଅଚଳ taskp = VN_CAST(m_memberMap.findMember(cp, name), Task)) {
                ଫେରନ୍ତୁ taskp;
            }
        }
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ addPrePostCall(AstClass* ଅଚଳ classp, Ast_ଗଣ୍ଠି_FTask* ଅଚଳ funcp, ଅଚଳ string& name) {
        ଯଦି (AstTask* ଅଚଳ userFuncp = findPrePostTask(classp, name)) {
            AstTaskRef* ଅଚଳ callp = ନୂତନ AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->addStmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* ଅଚଳ classp, ଅଚଳ string& which) {
        ଅଚଳ string name = "__V" + which;
        ଯଦି (AstTask* ଅଚଳ existingp = VN_CAST(m_memberMap.findMember(classp, name), Task)) {
            ଫେରନ୍ତୁ existingp;
        }
        AstTask* ଅଚଳ taskp = ନୂତନ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(ସତ୍ୟ);
        taskp->isVirtual(classp->isExtended());
        classp->addMembersp(taskp);
        m_memberMap.ଭର୍ତ୍ତି(classp, taskp);
        addPrePostCall(classp, taskp, which);
        ଫେରନ୍ତୁ taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<ତାର୍କିକ, ତାର୍କିକ> buildPrePostVirtualWrappers(AstClass* ଅଚଳ classp) {
        ଅଚଳ auto cachedIt = m_prePostWrap.find(classp);
        ଯଦି (cachedIt != m_prePostWrap.end()) ଫେରନ୍ତୁ cachedIt->second;
        std::vector<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            ଯଦି (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.ଠେଲ_back(subp);
        });
        ତାର୍କିକ hasPre = ମିଥ୍ୟା;
        ତାର୍କିକ hasPost = ମିଥ୍ୟା;
        ପାଇଁ (AstClass* ଅଚଳ cp : hierp) {
            ଯଦି (findPrePostTask(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = ସତ୍ୟ;
            }
            ଯଦି (findPrePostTask(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = ସତ୍ୟ;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        ଯଦି (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        ଯଦି (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ଅଚଳ std::pair<ତାର୍କିକ, ତାର୍କିକ> result{hasPre, hasPost};
        m_prePostWrap.emplace(classp, result);
        ଫେରନ୍ତୁ result;
    }
    ଶୂନ୍ୟ addVirtualPrePostCall(AstFunc* ଅଚଳ randomizeFuncp, AstClass* ଅଚଳ classp,
                               ଅଚଳ string& which) {
        File_ଧାଡ଼ି* ଅଚଳ fl = classp->fileline();
        AstTask* ଅଚଳ wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ଅଚଳ refDTypep = ନୂତନ AstClassRefDType{fl, classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
        AstMethodCall* ଅଚଳ callp
            = ନୂତନ AstMethodCall{fl, ନୂତନ AstThisRef{fl, refDTypep}, wrapperp->name(), nullptr};
        callp->taskp(wrapperp);
        callp->dtypeSetVoid();
        randomizeFuncp->addStmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    ତାର୍କିକ classHasRandClassMembers(AstClass* classp) {
        ଫେରନ୍ତୁ classp->existsMember([](ଅଚଳ AstClass*, ଅଚଳ AstVar* varp) {
            ଯଦି (!varp->rand().isRandomizable()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = varp->dtypep()->skipRefp();
            ଫେରନ୍ତୁ VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    ତାର୍କିକ classOwnsGlobalConstraint(ଅଚଳ AstClass* classp) ଅଚଳ {
        ଫେରନ୍ତୁ classp->existsMember([](ଅଚଳ AstClass*, ଅଚଳ AstConstraint* constrp) {
            ତାର୍କିକ owns = ମିଥ୍ୟା;
            constrp->foreach([&](ଅଚଳ AstMemberSel* memberSelp) {
                ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ rootp
                    = const_cast<AstMemberSel*>(memberSelp)->baseFromp(ସତ୍ୟ);
                ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    ଯେତେବେଳେ (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    ଯଦି (VN_IS(dtypep, ClassRefDType)) owns = ସତ୍ୟ;
                }
            });
            ଫେରନ୍ତୁ owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ଅଚଳ string& suffix) {
        ଅଚଳ string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.findMember(classp, name), Task);
        ଯଦି (taskp) ଫେରନ୍ତୁ taskp;
        taskp = ନୂତନ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(ସତ୍ୟ);
        classp->addMembersp(taskp);
        m_memberMap.ଭର୍ତ୍ତି(classp, taskp);
        ଫେରନ୍ତୁ taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    ଶୂନ୍ୟ populateNestedCallbackTask(AstTask* ଅଚଳ callbackTaskp, AstClass* ଅଚଳ classp,
                                    ଅଚଳ string& cbName) {
        File_ଧାଡ଼ି* ଅଚଳ fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            ଯଦି (!memberVarp->rand().isRandomizable()) ଫେରନ୍ତୁ;
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = memberVarp->dtypep()->skipRefp();
            ଅଚଳ AstClassRefDType* ଅଚଳ classRefp = VN_CAST(dtypep, ClassRefDType);
            ଯଦି (!classRefp) ଫେରନ୍ତୁ;
            AstClass* ଅଚଳ memberClassp = classRefp->classp();
            ଯଦି (memberClassp == classp) ଫେରନ୍ତୁ;  // Avoid self-reference

            // Force-visit member class if not yet processed
            ଯଦି (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_ଗଣ୍ଠି* stmtsp = nullptr;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            ଯଦି (AstTask* ଅଚଳ userFuncp = findPrePostTask(memberClassp, cbName)) {
                AstMethodCall* ଅଚଳ callp = ନୂତନ AstMethodCall{
                    fl, ନୂତନ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtypeSetVoid();
                stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            ଯଦି (classHasRandClassMembers(memberClassp)) {
                ଅଚଳ string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ଅଚଳ nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                AstMethodCall* ଅଚଳ recurseCallp = ନୂତନ AstMethodCall{
                    fl, ନୂତନ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtypeSetVoid();
                stmtsp = Ast_ଗଣ୍ଠି::addNext(stmtsp, recurseCallp->makeStmt());
            }

            ଯଦି (!stmtsp) ଫେରନ୍ତୁ;

            // Wrap in null check
            AstIf* ଅଚଳ nullCheckp = ନୂତନ AstIf{
                fl,
                ନୂତନ AstNeq{fl, ନୂତନ AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           ନୂତନ AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->addStmtsp(wrapIfRandMode(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* newSetupConstraintTask(AstClass* ଅଚଳ nodep, ଅଚଳ std::string& name) {
        AstTask* ଅଚଳ taskp = ନୂତନ AstTask{nodep->fileline(), name + "_setup_constraint", nullptr};
        taskp->classMethod(ସତ୍ୟ);
        nodep->addMembersp(taskp);
        ଫେରନ୍ତୁ taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ଅଚଳ nodep, ଅଚଳ std::string& name) {
        AstTask* ଅଚଳ taskp
            = ନୂତନ AstTask{nodep->fileline(), name + "_resize_constrained_array", nullptr};
        taskp->classMethod(ସତ୍ୟ);
        nodep->addMembersp(taskp);
        ଫେରନ୍ତୁ taskp;
    }
    Ast_ଗଣ୍ଠି_Stmt* implementConstraintsClear(File_ଧାଡ଼ି* ଅଚଳ fileline, AstVar* ଅଚଳ genp) {
        AstCMethodHard* ଅଚଳ clearp = ନୂତନ AstCMethodHard{
            fileline,
            ନୂତନ AstVarRef{fileline, VN_AS(genp->user2p(), ଗଣ୍ଠି_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtypeSetVoid();
        ଫେରନ୍ତୁ clearp->makeStmt();
    }
    Ast_ଗଣ୍ଠି_Stmt* implementConstraintsClearAll(File_ଧାଡ଼ି* ଅଚଳ fileline, AstVar* ଅଚଳ genp) {
        AstCMethodHard* ଅଚଳ clearp = ନୂତନ AstCMethodHard{
            fileline,
            ନୂତନ AstVarRef{fileline, VN_AS(genp->user2p(), ଗଣ୍ଠି_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtypeSetVoid();
        ଫେରନ୍ତୁ clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp) {
        ଯଦି (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(exprp, MemberSel)) {
            ଫେରନ୍ତୁ memberSelp->varp();
        } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ varrefp = VN_CAST(exprp, VarRef)) {
            ଫେରନ୍ତୁ varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        ଫେରନ୍ତୁ nullptr;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* makeModeVarRef(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp, AstVar* ଅଚଳ modeVarp,
                                ଅଚଳ VAccess access) {
        ଯଦି (modeVarp->lifetime().isStatic()) {
            ଫେରନ୍ତୁ ନୂତନ AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), ଗଣ୍ଠି_Module),
                                 modeVarp, access};
        }
        ଯଦି (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ଅଚଳ newMemberSelp = ନୂତନ AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), modeVarp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            ଫେରନ୍ତୁ newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        ଫେରନ୍ତୁ ନୂତନ AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), Class), modeVarp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* createOrGetSizeVar(AstClass* ଅଚଳ classp, AstVar* ଅଚଳ arrVarp, File_ଧାଡ଼ି* ଅଚଳ fl,
                               Ast_ଗଣ୍ଠି_DType* ଅଚଳ signed32DTypep, ତାର୍କିକ& wasCreated) {
        AstVar* sizeVarp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = ମିଥ୍ୟା;
        ଯଦି (!sizeVarp) {
            sizeVarp = ନୂତନ AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->addMembersp(sizeVarp);
            m_memberMap.ଭର୍ତ୍ତି(classp, sizeVarp);
            sizeVarp->user2p(classp);
            arrVarp->user4p(sizeVarp);
            wasCreated = ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ sizeVarp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_ପଦସମଷ୍ଟି* createSizeGteZeroConstraint(File_ଧାଡ଼ି* ଅଚଳ fl, AstVar* ଅଚଳ sizeVarp,
                                                   Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* derefFromp = nullptr) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* size_ଗଣ୍ଠି_Exprp = nullptr;
        ଯଦି (derefFromp) {
            size_ଗଣ୍ଠି_Exprp = buildMemberSelExprp(derefFromp, sizeVarp);
        } ଅନ୍ୟଥା {
            size_ଗଣ୍ଠି_Exprp = ନୂତନ AstVarRef{fl, sizeVarp, VAccess::READ};
        }
        size_ଗଣ୍ଠି_Exprp->user1(ସତ୍ୟ);
        AstGteS* ଅଚଳ sizeGtep = ନୂତନ AstGteS{fl, size_ଗଣ୍ଠି_Exprp, ନୂତନ AstConst{fl, 0}};
        sizeGtep->user1(ସତ୍ୟ);
        ଫେରନ୍ତୁ ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, sizeGtep};
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* getFromp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp) {
        ଯଦି (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(exprp, MemberSel)) {
            ଫେରନ୍ତୁ memberSelp->fromp();
        } ଅନ୍ୟଥା ଯଦି (AstMethodCall* ଅଚଳ methodCallp = VN_CAST(exprp, MethodCall)) {
            ଫେରନ୍ତୁ methodCallp->fromp();
        }
        ଫେରନ୍ତୁ nullptr;
    }
    AstVar* makeTmpRandModeVar(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* siblingExprp, AstVar* randModeVarp,
                               Ast_ଗଣ୍ଠି*& storeStmtspr, Ast_ଗଣ୍ଠି_Stmt*& restoreStmtspr) {
        File_ଧାଡ଼ି* ଅଚଳ fl = randModeVarp->fileline();
        AstVar* ଅଚଳ randModeTmpVarp = ନୂତନ AstVar{
            fl, VVarType::BLOCKTEMP, m_modeUniqueNames.get(randModeVarp), randModeVarp->dtypep()};
        randModeTmpVarp->funcLocal(m_ftaskp);
        randModeTmpVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_ଗଣ୍ଠି::addNext(
            storeStmtspr,
            ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, randModeTmpVarp, VAccess::WRITE},
                          makeModeVarRef(siblingExprp, randModeVarp, VAccess::READ)});
        storeStmtspr = Ast_ଗଣ୍ଠି::addNext(
            storeStmtspr,
            makeModeSetLoop(fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                            ନୂତନ AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_ଗଣ୍ଠି::addNext(
            restoreStmtspr,
            ନୂତନ AstAssign{fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                          ନୂତନ AstVarRef{fl, randModeTmpVarp, VAccess::READ}});
        ଫେରନ୍ତୁ randModeTmpVarp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sliceToCommonPrefix(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* thisp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* otherp) {
        ସ୍ଥିର std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        ଯେତେବେଳେ (thisp) {
            thisHier.ଠେଲ_back(thisp);
            thisp = getFromp(thisp);
        }
        ଯେତେବେଳେ (otherp) {
            otherHier.ଠେଲ_back(otherp);
            otherp = getFromp(otherp);
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* commonp = nullptr;
        ପାଇଁ (auto thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            ଯଦି ((*thisIt)->type() != (*otherIt)->type()) ଭାଙ୍ଗ;
            ଯଦି (AstMemberSel* ଅଚଳ memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ଅଚଳ otherMemberSelp = VN_AS(*otherIt, MemberSel);
                ଯଦି (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    ଜାରି;
                }
            } ଅନ୍ୟଥା ଯଦି (AstMethodCall* ଅଚଳ thisMethodCallp = VN_CAST(*thisIt, MethodCall)) {
                AstMethodCall* ଅଚଳ otherMethodCallp = VN_AS(*otherIt, MethodCall);
                ଯଦି (thisMethodCallp->taskp() == otherMethodCallp->taskp()) {
                    commonp = thisMethodCallp;
                    ଜାରି;
                }
            } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ firstVarRefp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ଅଚଳ secondVarRefp = VN_AS(*otherIt, VarRef);
                ଯଦି (firstVarRefp->varp() == secondVarRefp->varp()) {
                    commonp = firstVarRefp;
                    ଜାରି;
                }
            }
            ଭାଙ୍ଗ;
        }
        ଫେରନ୍ତୁ commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* buildMemberSelExprp(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstVar* deepestVarp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* resExprp = nullptr;
        ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), nullptr);
            AstVar* varp = memberSelp->varp();
            ଯଦି (deepestVarp) varp = deepestVarp;
            resExprp = ନୂତନ AstMemberSel{exprp->fileline(), resExprp, varp};
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            ଯଦି (deepestVarp) varp = deepestVarp;
            resExprp = ନୂତନ AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), ଗଣ୍ଠି_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(ସତ୍ୟ);
        ଫେରନ୍ତୁ resExprp;
    }

    ଶୂନ୍ୟ addUpdateRandVarsBody(AstClass* ଅଚଳ nodep, AstFunc* ଅଚଳ randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ଅଚଳ updatep
            = ନୂତନ AstFunc{nodep->fileline(), "__VupdateRandVars", nullptr, nullptr};
        updatep->classMethod(ସତ୍ୟ);
        updatep->isVirtual(ସତ୍ୟ);
        updatep->keepAlive(ସତ୍ୟ);
        updatep->needsSyms(ମିଥ୍ୟା);
        nodep->addMembersp(updatep);
        nodep->hasRandVarsUpdate(ସତ୍ୟ);

        ପାଇଁ (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newp = VN_AS(m_memberMap.findMember(classp, "new"), ଗଣ୍ଠି_FTask);
            ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_ପଦସମଷ୍ଟି* ଅଚଳ stmtExprp = VN_CAST(stmtp, Stmt_ପଦସମଷ୍ଟି);
                ଯଦି (!stmtExprp) ଜାରି;
                AstCMethodHard* ଅଚଳ methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                ଯଦି (!methodp || methodp->method() != VCMethod::RANDOMIZER_WRITE_VAR) ଜାରି;

                Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varRefp = VN_AS(methodp->pinsp(), ଗଣ୍ଠି_Var_Ref);
                ଯଦି (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    ଜାରି;
                }
                AstCMethodHard* ଅଚଳ updateMethodp = ନୂତନ AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                updateMethodp->dtypeSetVoid();
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ namep = VN_AS(varRefp->nextp()->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                updateMethodp->addPinsp(namep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା));
                updateMethodp->method(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->addStmtsp(updateMethodp->makeStmt());
            }
        }
    }

    ଶୂନ୍ୟ addBasicRandomizeBody(AstFunc* ଅଚଳ basicRandomizep, AstClass* ଅଚଳ nodep,
                               AstVar* randModeVarp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        AstVar* ଅଚଳ basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ଅଚଳ basicFvarRefp = ନୂତନ AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ଅଚଳ beginBasicValp = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1};
        basicRandomizep->addStmtsp(ନୂତନ AstAssign{fl, basicFvarRefp, beginBasicValp});
        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newp = VN_AS(m_memberMap.findMember(nodep, "new"), ଗଣ୍ଠି_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            ଯଦି (!memberVarp->rand().isRandomizable()) ଫେରନ୍ତୁ;
            ଅଚଳ RandomizeMode randMode = {.asUQuad = memberVarp->user1()};
            ଯଦି (randMode.usesMode
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ଅଚଳ setp = ନୂତନ AstCMethodHard{
                    nodep->fileline(),
                    ନୂତନ AstVarRef{fl, VN_AS(randModeVarp->user2p(), ଗଣ୍ଠି_Module), randModeVarp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, ନୂତନ AstConst{nodep->fileline(), randMode.index}};
                setp->dtypeSetUInt32();
                newp->addStmtsp(ନୂତନ AstAssign{fl, setp, ନୂତନ AstConst{fl, 0}});
            }
            ଯଦି (memberVarp->user3()) ଫେରନ୍ତୁ;  // Handled in constraints
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = memberVarp->dtypep()->skipRefp();
            ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                ଯଦି (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    ଫେରନ୍ତୁ;
                }
                AstFunc* ଅଚଳ memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                AstMethodCall* ଅଚଳ callp
                    = memberVarp->globalConstrained()
                          ? ନୂତନ AstMethodCall{fl,
                                              ନୂତନ AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : ନୂତନ AstMethodCall{
                                fl, ନୂତନ AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ଅଚଳ basicFvarRefReadp = basicFvarRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ଅଚଳ assignIfNotNullp = ନୂତନ AstIf{
                    fl,
                    ନୂତନ AstNeq{fl, ନୂତନ AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               ନୂତନ AstConst{fl, AstConst::Null{}}},
                    ନୂତନ AstAssign{fl, basicFvarRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                  ନୂତନ AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->addStmtsp(wrapIfRandMode(nodep, memberVarp, assignIfNotNullp));
            } ଅନ୍ୟଥା {
                AstVar* ଅଚଳ randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ଅଚଳ refp
                    = ନୂତନ AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                ଯଦି (!refp->backp()) VL_DO_DANGLING(refp->delete_ବୃକ୍ଷ(), refp);
                basicRandomizep->addStmtsp(
                    ନୂତନ AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtp, ମିଥ୍ୟା});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* makeModeAssignLhs(File_ଧାଡ଼ି* ଅଚଳ fl, AstClass* ଅଚଳ classp,
                                   Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp, AstVar* ଅଚଳ modeVarp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        ଯଦି (modeVarp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            ଯଦି (fromp) VL_DO_DANGLING(fromp->unlinkFrBack()->delete_ବୃକ୍ଷ(), fromp);
            ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, VN_AS(modeVarp->user2p(), ଗଣ୍ଠି_Module), modeVarp,
                                 VAccess::WRITE};
        } ଅନ୍ୟଥା ଯଦି (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            ଫେରନ୍ତୁ ନୂତନ AstVarRef{fl, VN_AS(modeVarp->user2p(), ଗଣ୍ଠି_Module), modeVarp,
                                 VAccess::WRITE};
        } ଅନ୍ୟଥା {
            AstMemberSel* ଅଚଳ memberselp = ନୂତନ AstMemberSel{fl, fromp->unlinkFrBack(), modeVarp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            ଫେରନ୍ତୁ memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    ଶୂନ୍ୟ replaceWithModeAssign(Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ ftaskRefp, Ast_ଗଣ୍ଠି* ଅଚଳ receiverp,
                               Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp) {
        replaceWithModeAssignAndAppend(ftaskRefp, receiverp, lhsp, nullptr);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    ଶୂନ୍ୟ replaceWithModeAssignAndAppend(Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ ftaskRefp, Ast_ଗଣ୍ଠି* ଅଚଳ receiverp,
                                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp, Ast_ଗଣ୍ଠି* ଅଚଳ appendStmtp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = ftaskRefp->fileline();
        ଯଦି (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_ପଦସମଷ୍ଟି), ftaskRefp, "Should be a statement");
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ftaskRefp->argsp()->exprp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି* newStmtp = nullptr;
            ଯଦି (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                // mode
                ଅଚଳ RandomizeMode rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ଅଚଳ setp = ନୂତନ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                ନୂତନ AstConst{fl, rmode.index}};
                setp->dtypeSetUInt32();
                newStmtp = ନୂତନ AstAssign{fl, setp, rhsp};
            } ଅନ୍ୟଥା {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = makeModeSetLoop(fl, lhsp, rhsp, m_ftaskp);
            }
            ଯଦି (appendStmtp) newStmtp->addNext(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(ଠେଲ_Deletep(m_stmtp), m_stmtp = nullptr);
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ଅଚଳ RandomizeMode rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ଅଚଳ setp = ନୂତନ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            ନୂତନ AstConst{fl, rmode.index}};
            setp->dtypeSetUInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(ଠେଲ_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    ଶୂନ୍ୟ wrapRandomizeCallWithNullGuard(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) {
        AstMethodCall* ଅଚଳ callp = VN_CAST(nodep, MethodCall);
        ଯଦି (!callp) ଫେରନ୍ତୁ;
        ଯଦି (callp->user4SetOnce()) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ fl = callp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ checkp
            = ନୂତନ AstNeq{fl, callp->fromp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), ନୂତନ AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlinkFrBack(&relinker);
        relinker.relink(ନୂତନ AstCond{fl, checkp, callp, ନୂତନ AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    ଶୂନ୍ୟ handleRandomizeArgs(Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ nodep) {
        ଯଦି (!nodep->argsp()) ଫେରନ୍ତୁ;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        ତାର୍କିକ hasNullArg = ମିଥ୍ୟା;
        ପାଇଁ (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->ଅଙ୍କ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = ସତ୍ୟ;
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        ପାଇଁ (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ପାଇଁ (AstArg *otherArgp = nextp, *otherNextp = nullptr; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prefixp
                    = sliceToCommonPrefix(argp->exprp(), otherArgp->exprp())) {
                    ଯଦି (prefixp == argp->exprp()) {
                        ଯଦି (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlinkFrBack()->delete_ବୃକ୍ଷ(), otherArgp);
                        ଜାରି;
                    }
                }
                ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ prefixp
                    = sliceToCommonPrefix(otherArgp->exprp(), argp->exprp())) {
                    ଯଦି (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);
                        ଭାଙ୍ଗ;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::set<AstVar*> savedRandModeVarps;
        AstVar* tmpVarps = nullptr;
        Ast_ଗଣ୍ଠି* storeStmtsp = nullptr;
        Ast_ଗଣ୍ଠି* setStmtsp = nullptr;
        Ast_ଗଣ୍ଠି_Stmt* restoreStmtsp = nullptr;
        ପାଇଁ (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = argp->exprp();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ commonPrefixp = sliceToCommonPrefix(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_ଧାଡ଼ି* ଅଚଳ fl = argp->fileline();
            ଯେତେବେଳେ (exprp) {
                ଯଦି (commonPrefixp == exprp) ଭାଙ୍ଗ;
                AstVar* ଅଚଳ randVarp = getVarFromRef(exprp);
                AstClass* ଅଚଳ classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ଅଚଳ randModeVarp = randVarp->lifetime().isStatic()
                                                 ? getStaticRandModeVar(classp)
                                                 : getRandModeVarFromClass(classp);
                UASSERT_OBJ(randModeVarp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                ଯଦି (savedRandModeVarps.find(randModeVarp) == savedRandModeVarps.end()) {
                    AstVar* ଅଚଳ randModeTmpVarp
                        = makeTmpRandModeVar(exprp, randModeVarp, storeStmtsp, restoreStmtsp);
                    savedRandModeVarps.ଭର୍ତ୍ତି(randModeVarp);
                    tmpVarps = Ast_ଗଣ୍ଠି::addNext(tmpVarps, randModeTmpVarp);
                }
                ଅଚଳ RandomizeMode randMode = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = ନୂତନ AstCMethodHard{
                    fl, makeModeVarRef(exprp, randModeVarp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, ନୂତନ AstConst{fl, randMode.index}};
                setp->dtypeSetUInt32();
                setStmtsp
                    = Ast_ଗଣ୍ଠି::addNext(setStmtsp, ନୂତନ AstAssign{fl, setp, ନୂତନ AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlinkFrBack()->delete_ବୃକ୍ଷ(), argp);
        }
        ଯଦି (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* targetClassp = nullptr;
            AstMethodCall* ଅଚଳ methodCallp = VN_CAST(nodep, MethodCall);
            ଯଦି (methodCallp) {
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDTypep = methodCallp->fromp()->dtypep();
                ଅଚଳ AstClassRefDType* ଅଚଳ crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                targetClassp = crdtp->classp();
            } ଅନ୍ୟଥା {
                targetClassp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(targetClassp, nodep, "randomize(null) target class unresolved");
            AstFunc* ଅଚଳ checkOnlyFuncp = getCreateRandomizeNullFunc(targetClassp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        ଯଦି (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && setStmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlinkFrBack(&relinker);
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = storeStmtsp;
            stmtsp->addNext(setStmtsp);
            stmtsp->addNext(m_stmtp);
            stmtsp->addNext(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            addTempVarsp(tmpVarps);
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
    AstFunc* getCreateRandomizeNullFunc(AstClass* ଅଚଳ classp) {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ name = "__Vrandomize_null";
        ଯଦି (AstFunc* ଅଚଳ existingp = VN_AS(m_memberMap.findMember(classp, name), Func)) {
            ଫେରନ୍ତୁ existingp;
        }
        File_ଧାଡ଼ି* ଅଚଳ fl = classp->fileline();
        AstFunc* ଅଚଳ funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ଅଚଳ fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        addPrePostCall(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ଅଚଳ classGenp = getRandomGenerator(classp);
        ଯଦି (!classGenp) {
            funcp->addStmtsp(ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, fvarp, VAccess::WRITE},
                                           ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1}});
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ genModp = VN_AS(classGenp->user2p(), ଗଣ୍ଠି_Module);
            funcp->addStmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ଅଚଳ setupAllTaskp = getCreateConstraintSetupFunc(classp, m_memberMap);
            funcp->addStmtsp((ନୂତନ AstTaskRef{fl, setupAllTaskp})->makeStmt());
            AstCExpr* ଅଚଳ solverCallp = ନୂତନ AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(ନୂତନ AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->add(".next_check_only(__Vm_rng)");
            funcp->addStmtsp(
                ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(ସତ୍ୟ);
        }

        // 3. post_randomize -- only if result is non-zero
        ଯଦି (AstTask* ଅଚଳ userPostp = findPrePostTask(classp, "post_randomize")) {
            AstTaskRef* ଅଚଳ callp = ନୂତନ AstTaskRef{userPostp->fileline(), userPostp};
            funcp->addStmtsp(
                ନୂତନ AstIf{fl, ନୂତନ AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        ଫେରନ୍ତୁ funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* liftLogIfChainToConstraintIf(AstLogIf* logIfp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = logIfp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = logIfp->lhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = logIfp->rhsp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି* thenBodyp;
        ଯଦି (AstLogIf* ଅଚଳ innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = liftLogIfChainToConstraintIf(innerLogIfp);
            VL_DO_DANGLING(ଠେଲ_Deletep(innerLogIfp), innerLogIfp);
        } ଅନ୍ୟଥା {
            thenBodyp = ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, rhsp};
        }
        ଫେରନ୍ତୁ ନୂତନ AstConstraintIf{fl, condp, thenBodyp, nullptr};
    }

    ସ୍ଥିର ତାର୍କିକ distBoundRefsRandVar(ଅଚଳ Ast_ଗଣ୍ଠି* boundp) {
        ଫେରନ୍ତୁ boundp->exists(
            [](ଅଚଳ AstVarRef* vrefp) { ଫେରନ୍ତୁ vrefp->varp()->rand().isRandomizable(); });
    }

    ସ୍ଥିର ତାର୍କିକ distBoundRefsModeVar(ଅଚଳ Ast_ଗଣ୍ଠି* boundp) {
        ଫେରନ୍ତୁ boundp->exists([](ଅଚଳ AstVarRef* vrefp) {
            ଯଦି (!vrefp->varp()->rand().isRandomizable()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଅଚଳ RandomizeMode rmode = {.asUQuad = vrefp->varp()->user1()};
            ଫେରନ୍ତୁ rmode.usesMode;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newDistRangeMembership(AstDist* distp, ଅଚଳ AstInsideRange* irp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();
        ଅଚଳ ତାର୍କିକ isSigned = distp->exprp()->isSigned();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dist_ପଦସମଷ୍ଟି_Gtep = distp->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dist_ପଦସମଷ୍ଟି_Ltep = distp->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        dist_ପଦସମଷ୍ଟି_Gtep->user1(ସତ୍ୟ);
        dist_ପଦସମଷ୍ଟି_Ltep->user1(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ gep
            = isSigned ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                             ନୂତନ AstGteS{fl, dist_ପଦସମଷ୍ଟି_Gtep, irp->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)})
                       : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                             ନୂତନ AstGte{fl, dist_ପଦସମଷ୍ଟି_Gtep, irp->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)});
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lep
            = isSigned ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                             ନୂତନ AstLteS{fl, dist_ପଦସମଷ୍ଟି_Ltep, irp->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)})
                       : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                             ନୂତନ AstLte{fl, dist_ପଦସମଷ୍ଟି_Ltep, irp->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)});
        gep->user1(ସତ୍ୟ);
        lep->user1(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ andp = ନୂତନ AstLogAnd{fl, gep, lep};
        andp->user1(ସତ୍ୟ);
        ଫେରନ୍ତୁ andp;
    }

    ସଂରଚନା DistBucket final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rangep;  // A single value or an InsideRange
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* weightExprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::vector<DistBucket> collectDistBuckets(AstDist* ଅଚଳ distp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();
        std::vector<DistBucket> buckets;
        ପାଇଁ (AstDistItem* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), DistItem)) {
            ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(ditemp->weightp(), Const)) {
                ଯଦି (constp->toUQuad() == 0) ଜାରି;
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* weightExprp
                = ନୂତନ AstExtend{fl, ditemp->weightp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 64};
            // := on a range weights every element, so scale by the range size.
            ଯଦି (!ditemp->isWhole()) {
                ଯଦି (ଅଚଳ AstInsideRange* ଅଚଳ irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ଅଚଳ AstConst* ଅଚଳ lop = VN_CAST(irp->lhsp(), Const);
                    ଅଚଳ AstConst* ଅଚଳ hip = VN_CAST(irp->rhsp(), Const);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rangeSizep;
                    ଯଦି (lop && hip) {
                        ଅଚଳ uint64_t rangeSize = hip->toUQuad() - lop->toUQuad() + 1;
                        rangeSizep = ନୂତନ AstConst{fl, AstConst::Unsized64{}, rangeSize};
                    } ଅନ୍ୟଥା {
                        rangeSizep = ନୂତନ AstAdd{
                            fl, ନୂତନ AstConst{fl, AstConst::Unsized64{}, 1},
                            ନୂତନ AstSub{fl,
                                       ନୂତନ AstExtend{fl, irp->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 64},
                                       ନୂତନ AstExtend{fl, irp->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 64}}};
                    }
                    weightExprp = ନୂତନ AstMul{fl, weightExprp, rangeSizep};
                }
            }
            buckets.ଠେଲ_back({ditemp->rangep(), weightExprp});
        }
        ଫେରନ୍ତୁ buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newDistMembershipTerm(AstDist* distp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rangep) {
        ଯଦି (ଅଚଳ AstInsideRange* ଅଚଳ irp = VN_CAST(rangep, InsideRange)) {
            ଫେରନ୍ତୁ newDistRangeMembership(distp, irp);
        }
        File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eqExprp = distp->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        eqExprp->user1(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eqp = ନୂତନ AstEq{fl, eqExprp, rangep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
        eqp->user1(ସତ୍ୟ);
        ଫେରନ୍ତୁ eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_ପଦସମଷ୍ଟି* buildDistMembership(AstDist* distp,
                                           ଅଚଳ std::vector<DistBucket>& buckets) {
        File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* unionExprp = nullptr;
        ପାଇଁ (ଅଚଳ auto& bucket : buckets) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ memberp = newDistMembershipTerm(distp, bucket.rangep);
            ଯଦି (!unionExprp) {
                unionExprp = memberp;
            } ଅନ୍ୟଥା {
                unionExprp = ନୂତନ AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(ସତ୍ୟ);
            }
        }
        ଫେରନ୍ତୁ ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newGateJoin(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ap, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bp, ତାର୍କିକ isAnd) {
        ଯଦି (ap->same_ବୃକ୍ଷ(bp)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(bp), bp);
            ଫେରନ୍ତୁ ap;
        }
        ଯଦି (VN_IS(bp, Const)) std::swap(ap, bp);
        ଯଦି (VN_IS(ap, Const)) {
            ଅଚଳ ତାର୍କିକ constWins = VN_AS(ap, Const)->isZero() == isAnd;
            ଯଦି (!constWins) std::swap(ap, bp);
            VL_DO_DANGLING(ଠେଲ_Deletep(bp), bp);
            ଫେରନ୍ତୁ ap;
        }
        ଯଦି (isAnd) ଫେରନ୍ତୁ ନୂତନ AstLogAnd{fl, ap, bp};
        ଫେରନ୍ତୁ ନୂତନ AstLogOr{fl, ap, bp};
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newGateOr(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ap, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bp) {
        ଫେରନ୍ତୁ newGateJoin(fl, ap, bp, ମିଥ୍ୟା);
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newGateAnd(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ap, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* bp) {
        ଫେରନ୍ତୁ newGateJoin(fl, ap, bp, ସତ୍ୟ);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newModeBitRead(AstVar* varp, ଅଚଳ AstMemberSel* mselp, AstVar* randModeVarp,
                                File_ଧାଡ଼ି* fl) {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* arrayp = nullptr;
        ଯଦି (varp->lifetime().isStatic()) {
            AstClass* ଅଚଳ ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ଅଚଳ smodep = getStaticRandModeVar(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = ନୂତନ AstVarRef{fl, VN_AS(smodep->user2p(), ଗଣ୍ଠି_Module), smodep, VAccess::READ};
        } ଅନ୍ୟଥା ଯଦି (mselp) {
            AstVar* ଅଚଳ memberModep = getRandModeVarFromClass(VN_AS(varp->user2p(), ଗଣ୍ଠି_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = ନୂତନ AstMemberSel{fl, mselp->fromp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), memberModep};
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(randModeVarp, varp, "rand_mode variable without a class mode array");
            arrayp = ନୂତନ AstVarRef{fl, VN_AS(randModeVarp->user2p(), ଗଣ୍ଠି_Module), randModeVarp,
                                   VAccess::READ};
        }
        ଅଚଳ RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ଅଚଳ atp
            = ନୂତନ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, ନୂତନ AstConst{fl, rmode.index}};
        atp->dtypeSetUInt32();
        ଫେରନ୍ତୁ atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newVarGate(AstVar* varp, ଅଚଳ AstMemberSel* mselp, ତାର୍କିକ ownerLevel,
                            AstVar* randModeVarp, File_ଧାଡ଼ି* fl) {
        ଯଦି (!varp->rand().isRandomizable()) {
            ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitTrue{}, ownerLevel};
        }
        ଅଚଳ RandomizeMode rmode = {.asUQuad = varp->user1()};
        ଯଦି (!rmode.usesMode) ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitTrue{}};
        ଫେରନ୍ତୁ newModeBitRead(varp, mselp, randModeVarp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newAccessGate(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ ownerLevel, AstVar* randModeVarp,
                               File_ଧାଡ଼ି* fl) {
        ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ mselp = VN_CAST(nodep, MemberSel)) {
            ଫେରନ୍ତୁ newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, randModeVarp, fl),
                              newAccessGate(mselp->fromp(), ସତ୍ୟ, randModeVarp, fl));
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ refp = VN_CAST(nodep, ଗଣ୍ଠି_Var_Ref)) {
            ଫେରନ୍ତୁ newVarGate(refp->varp(), nullptr, ownerLevel, randModeVarp, fl);
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Sel* ଅଚଳ selp = VN_CAST(nodep, ଗଣ୍ଠି_Sel)) {
            ଫେରନ୍ତୁ newAccessGate(selp->fromp(), ସତ୍ୟ, randModeVarp, fl);
        }
        ଯଦି (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitTrue{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newDistGate(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, AstVar* randModeVarp, File_ଧାଡ଼ି* fl) {
        ଯଦି (VN_IS(nodep, ଗଣ୍ଠି_Var_Ref) || VN_IS(nodep, MemberSel)) {
            ଫେରନ୍ତୁ newAccessGate(nodep, ମିଥ୍ୟା, randModeVarp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Sel* ଅଚଳ selp = VN_CAST(nodep, ଗଣ୍ଠି_Sel)) {
            ଫେରନ୍ତୁ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(nodep, Sel)) {
            ଫେରନ୍ତୁ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        ଯଦି (ଅଚଳ AstStructSel* ଅଚଳ selp = VN_CAST(nodep, StructSel)) {
            ଫେରନ୍ତୁ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        ଯଦି (ଅଚଳ AstCMethodHard* ଅଚଳ methodp = VN_CAST(nodep, CMethodHard)) {
            ଯଦି (methodp->method() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                ଫେରନ୍ତୁ newDistGate(methodp->fromp(), randModeVarp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        ଯଦି (ଅଚଳ AstCond* ଅଚଳ condp = VN_CAST(nodep, Cond)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thenGatep = newDistGate(condp->thenp(), randModeVarp, fl);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elseGatep = newDistGate(condp->elsep(), randModeVarp, fl);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* armGatep = thenGatep;
            ଯଦି (thenGatep->same_ବୃକ୍ଷ(elseGatep)) {
                VL_DO_DANGLING(ଠେଲ_Deletep(elseGatep), elseGatep);
            } ଅନ୍ୟଥା {
                armGatep
                    = ନୂତନ AstCond{fl, condp->condp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), thenGatep, elseGatep};
            }
            ଫେରନ୍ତୁ newGateOr(fl, newDistGate(condp->condp(), randModeVarp, fl), armGatep);
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Uniop* ଅଚଳ uopp = VN_CAST(nodep, ଗଣ୍ଠି_Uniop)) {
            ଫେରନ୍ତୁ newDistGate(uopp->lhsp(), randModeVarp, fl);
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ bopp = VN_CAST(nodep, ଗଣ୍ଠି_Biop)) {
            ଫେରନ୍ତୁ newGateOr(fl, newDistGate(bopp->lhsp(), randModeVarp, fl),
                             newDistGate(bopp->rhsp(), randModeVarp, fl));
        }
        ଯଦି (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        ଫେରନ୍ତୁ ନୂତନ AstConst{fl, AstConst::BitTrue{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_ପଦସମଷ୍ଟି* buildFrozenMembership(AstDist* distp,
                                             ଅଚଳ std::vector<DistBucket>& buckets) {
        File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* unionExprp = nullptr;
        ପାଇଁ (ଅଚଳ auto& bucket : buckets) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* termp = newDistMembershipTerm(distp, bucket.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ଅଚଳ ତାର୍କିକ runtimeWeight
                = bucket.weightExprp->exists([](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref*) { ଫେରନ୍ତୁ ସତ୍ୟ; });
            ଯଦି (runtimeWeight) {
                AstNeq* ଅଚଳ nzp = ନୂତନ AstNeq{fl, bucket.weightExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା),
                                               ନୂତନ AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(ସତ୍ୟ);
                termp = ନୂତନ AstLogAnd{fl, termp, nzp};
                termp->user1(ସତ୍ୟ);
            }
            ଯଦି (unionExprp) {
                unionExprp = ନୂତନ AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(ସତ୍ୟ);
            } ଅନ୍ୟଥା {
                unionExprp = termp;
            }
        }
        ଫେରନ୍ତୁ ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_ଗଣ୍ଠି* gateFrozenDist(Ast_ଗଣ୍ଠି* chainp, AstDist* distp,
                            ଅଚଳ std::vector<DistBucket>& buckets, AstVar* randModeVarp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* gatep = newDistGate(distp->exprp(), randModeVarp, fl);
        ଯଦି (VN_IS(gatep, Const)) {
            ଅଚଳ ତାର୍କିକ drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(ଠେଲ_Deletep(gatep), gatep);
            ଯଦି (drawable) ଫେରନ୍ତୁ chainp;
            VL_DO_DANGLING(ଠେଲ_Deletep(chainp), chainp);
            ଫେରନ୍ତୁ buildFrozenMembership(distp, buckets);
        }
        ଫେରନ୍ତୁ ନୂତନ AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newUniformRangePick(AstDist* distp, ଅଚଳ AstInsideRange* irp) {
        File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dist_ପଦସମଷ୍ଟି_Copyp = distp->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        dist_ପଦସମଷ୍ଟି_Copyp->user1(ସତ୍ୟ);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା distWidth = distp->exprp()->width();
        ଅଚଳ AstConst* ଅଚଳ lopC = VN_CAST(irp->lhsp(), Const);
        ଅଚଳ AstConst* ଅଚଳ hipC = VN_CAST(irp->rhsp(), Const);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rangeSzp;
        ଯଦି (lopC && hipC) {
            ଅଚଳ uint64_t rsz = hipC->toUQuad() - lopC->toUQuad() + 1;
            rangeSzp = ନୂତନ AstConst{fl, AstConst::Unsized64{}, rsz};
        } ଅନ୍ୟଥା {
            ଅଚଳ ତାର୍କିକ isSigned = irp->lhsp()->isSigned();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lo64p
                = isSigned ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                 ନୂତନ AstExtendS{fl, irp->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 64})
                           : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                 ନୂତନ AstExtend{fl, irp->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 64});
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ hi64p
                = isSigned ? static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                 ନୂତନ AstExtendS{fl, irp->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 64})
                           : static_cast<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>(
                                 ନୂତନ AstExtend{fl, irp->rhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 64});
            rangeSzp = ନୂତନ AstAdd{fl, ନୂତନ AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  ନୂତନ AstSub{fl, hi64p, lo64p}};
        }
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rand64p = ନୂତନ AstRand{fl, nullptr, ମିଥ୍ୟା};
        rand64p->dtypeSetUInt64();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ offsetp
            = ନୂତନ AstCCast{fl, ନୂତନ AstModDiv{fl, rand64p, rangeSzp}, distWidth};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ valuep = ନୂତନ AstAdd{fl, irp->lhsp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), offsetp};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ eqp = ନୂତନ AstEq{fl, dist_ପଦସମଷ୍ଟି_Copyp, valuep};
        eqp->user1(ସତ୍ୟ);
        ଫେରନ୍ତୁ eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_ଗଣ୍ଠି* buildWeightedBucketChain(AstDist* distp, ଅଚଳ std::vector<DistBucket>& buckets,
                                      AstVar* bucketVarp,
                                      ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& cumSums) {
        File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* chainp = nullptr;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(buckets.size()) - 1; i >= 0; --i) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* constraintExprp;
            ଅଚଳ AstInsideRange* ଅଚଳ irp = VN_CAST(buckets[i].rangep, InsideRange);
            ଯଦି (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } ଅନ୍ୟଥା ଯଦି (irp) {
                constraintExprp = newUniformRangePick(distp, irp);
            } ଅନ୍ୟଥା {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ dist_ପଦସମଷ୍ଟି_Copyp = distp->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                dist_ପଦସମଷ୍ଟି_Copyp->user1(ସତ୍ୟ);
                constraintExprp
                    = ନୂତନ AstEq{fl, dist_ପଦସମଷ୍ଟି_Copyp, buckets[i].rangep->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                constraintExprp->user1(ସତ୍ୟ);
            }
            ଯଦି (!chainp) {
                chainp = constraintExprp;
            } ଅନ୍ୟଥା {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp
                    = ନୂତନ AstLte{fl, ନୂତନ AstVarRef{fl, bucketVarp, VAccess::READ}, cumSums[i]};
                condp->user1(ସତ୍ୟ);
                chainp = ନୂତନ AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(ସତ୍ୟ);
            }
        }
        Ast_Constraint_ପଦସମଷ୍ଟି* ଅଚଳ chainExprp = ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, chainp};
        chainExprp->isSoft(ସତ୍ୟ);
        ଫେରନ୍ତୁ chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    ଶୂନ୍ୟ lowerDistConstraints(AstTask* taskp, Ast_ଗଣ୍ଠି* constrItemsp, AstVar* randModeVarp,
                              AstConstraintForeach* foreachp = nullptr) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_ଗଣ୍ଠି* foreachTailp = nullptr;
        auto addStmt = [&](Ast_ଗଣ୍ଠି* nodep) {
            ଯଦି (foreachp) {
                ଯଦି (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } ଅନ୍ୟଥା {
                    foreachTailp->addNext(nodep);
                    foreachTailp = nodep;
                }
            } ଅନ୍ୟଥା {
                taskp->addStmtsp(nodep);
            }
        };

        ପାଇଁ (Ast_ଗଣ୍ଠି *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            ଯଦି (AstConstraintIf* ଅଚଳ cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                lowerDistConstraints(taskp, cifp->thensp(), randModeVarp, foreachp);
                ଯଦି (cifp->elsesp()) {
                    lowerDistConstraints(taskp, cifp->elsesp(), randModeVarp, foreachp);
                }
                ଜାରି;
            }
            ଯଦି (AstConstraintForeach* ଅଚଳ cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                lowerDistConstraints(taskp, cfep->bodyp(), randModeVarp, cfep);
                ଜାରି;
            }

            Ast_Constraint_ପଦସମଷ୍ଟି* ଅଚଳ constrExprp = VN_CAST(itemp, Constraint_ପଦସମଷ୍ଟି);
            ଯଦି (!constrExprp) ଜାରି;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            ଯଦି (AstLogIf* ଅଚଳ ଶୀର୍ଷ_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_ଗଣ୍ଠି* chainEndp = ଶୀର୍ଷ_Log_Ifp->rhsp();
                ଯେତେବେଳେ (AstLogIf* ଅଚଳ innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                ଯଦି (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ଅଚଳ liftedp = liftLogIfChainToConstraintIf(ଶୀର୍ଷ_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(constrExprp), constrExprp);
                    lowerDistConstraints(taskp, liftedp->thensp(), randModeVarp, foreachp);
                    ଜାରି;
                }
            }

            AstDist* ଅଚଳ distp = VN_CAST(constrExprp->exprp(), Dist);
            ଯଦି (!distp) ଜାରି;

            File_ଧାଡ଼ି* ଅଚଳ fl = distp->fileline();

            std::vector<DistBucket> buckets = collectDistBuckets(distp);

            ଯଦି (buckets.empty()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_ପଦସମଷ୍ଟି* ଅଚଳ truep
                    = ନୂତନ Ast_Constraint_ପଦସମଷ୍ଟି{fl, ନୂତନ AstConst{fl, AstConst::BitTrue{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(ଠେଲ_Deletep(constrExprp), constrExprp);
                ଜାରି;
            }

            Ast_Constraint_ପଦସମଷ୍ଟି* ଅଚଳ membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* totalWeightExprp = nullptr;
            ପାଇଁ (auto& bucket : buckets) {
                ଯଦି (!totalWeightExprp) {
                    totalWeightExprp = bucket.weightExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                } ଅନ୍ୟଥା {
                    totalWeightExprp = ନୂତନ AstAdd{fl, totalWeightExprp,
                                                  bucket.weightExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    totalWeightExprp->dtypeSetUInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା distId = m_dist_ଅଙ୍କ++;
            ଅଚଳ std::string totalName = "__Vdist_total" + cvtToStr(distId);
            AstVar* ଅଚଳ totalVarp
                = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, totalName, taskp->findUInt64DType()};
            totalVarp->noSubst(ସତ୍ୟ);
            totalVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            totalVarp->funcLocal(ସତ୍ୟ);
            totalVarp->isInternal(ସତ୍ୟ);
            addStmt(totalVarp);
            addStmt(
                ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, totalVarp, VAccess::WRITE}, totalWeightExprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ଅଚଳ std::string bucketName = "__Vdist_bucket" + cvtToStr(distId);
            AstVar* ଅଚଳ bucketVarp
                = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, bucketName, taskp->findUInt64DType()};
            bucketVarp->noSubst(ସତ୍ୟ);
            bucketVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            bucketVarp->funcLocal(ସତ୍ୟ);
            bucketVarp->isInternal(ସତ୍ୟ);
            addStmt(bucketVarp);

            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ randp = ନୂତନ AstRand{fl, nullptr, ମିଥ୍ୟା};
            randp->dtypeSetUInt64();
            addStmt(ନୂତନ AstAssign{
                fl, ନୂତନ AstVarRef{fl, bucketVarp, VAccess::WRITE},
                ନୂତନ AstAdd{
                    fl, ନୂତନ AstConst{fl, AstConst::Unsized64{}, 1},
                    ନୂତନ AstModDiv{fl, randp, ନୂତନ AstVarRef{fl, totalVarp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*> cumSums;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* runningSump = nullptr;
            ପାଇଁ (size_t i = 0; i < buckets.size(); ++i) {
                ଯଦି (!runningSump) {
                    runningSump = buckets[i].weightExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                } ଅନ୍ୟଥା {
                    runningSump = ନୂତନ AstAdd{fl, runningSump,
                                             buckets[i].weightExprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା)};
                    runningSump->dtypeSetUInt64();
                }
                cumSums.ଠେଲ_back(runningSump->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ));
            }

            Ast_ଗଣ୍ଠି* chainp = buildWeightedBucketChain(distp, buckets, bucketVarp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, randModeVarp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(ଠେଲ_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->addHereThisAsNext(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            ପାଇଁ (auto& bucket : buckets) {
                VL_DO_DANGLING(ଠେଲ_Deletep(bucket.weightExprp), bucket.weightExprp);
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            ଠେଲ_Deletep(cumSums.back());
        }
    }

    ସ୍ଥିର ତାର୍କିକ isDynArrOfClassTypeRecurse(ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep) {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ refp = dtypep->skipRefp();
        ଯଦି (VN_IS(refp, DynArrayDType) || VN_IS(refp, QueueDType)) {
            ଫେରନ୍ତୁ isDynArrOfClassTypeRecurse(refp->subDTypep());
        } ଅନ୍ୟଥା ଯଦି (VN_IS(refp, ClassRefDType)) {
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    ଶୂନ୍ୟ addArrayElemRefresh(File_ଧାଡ଼ି* ଅଚଳ fl, AstVar* ଅଚଳ arrVarp, AstFunc* ଅଚଳ randomizep,
                             AstVar* ଅଚଳ genp) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ genModp = VN_AS(genp->user2p(), ଗଣ୍ଠି_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ଅଚଳ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(ମିଥ୍ୟା).second;
        ଯଦି (isDynArrOfClassTypeRecurse(arrVarp->dtypep())) {
            ଯଦି (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            ଫେରନ୍ତୁ;
        }
        AstCMethodHard* ଅଚଳ methodp
            = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();

        Ast_ଗଣ୍ଠି_Module* ଅଚଳ classp = VN_AS(arrVarp->user2p(), ଗଣ୍ଠି_Module);
        AstVarRef* ଅଚଳ varRefp = ନୂତନ AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->addPinsp(varRefp);

        ଅଚଳ size_t width = arrayElementDTypep(arrVarp->dtypep())->width();

        methodp->addPinsp(ନୂତନ AstConst{fl, AstConst::Unsized64{}, width});
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ varnamep
            = ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "\"" + arrVarp->name() + "\"", arrVarp->width()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(ନୂତନ AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ଅଚଳ RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        ଯଦି (rmode.usesMode) {
            methodp->addPinsp(ନୂତନ AstConst{fl, AstConst::Unsized64{}, rmode.index});
        }

        randomizep->addStmtsp(methodp->makeStmt());
    }

    Ast_ଗଣ୍ଠି_Stmt* newPinSizeVariable(File_ଧାଡ଼ି* ଅଚଳ fl, AstVar* ଅଚଳ arrVarp,
                                    AstVar* ଅଚଳ genp) {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ genModp = VN_AS(genp->user2p(), ଗଣ୍ଠି_Module);
        AstVar* ଅଚଳ sizeVarp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ଅଚଳ pinp
            = ନୂତନ AstCMethodHard{fl, ନୂତନ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtypeSetVoid();
        AstCExpr* ଅଚଳ namep = ନୂତନ AstCExpr{fl, AstCExpr::Pure{}, "\"" + sizeVarp->name() + "\""};
        namep->dtypeSetUInt32();
        pinp->addPinsp(namep);
        pinp->addPinsp(
            ନୂତନ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(sizeVarp->width())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ଅଚଳ sizeVarRefp = ନୂତନ AstVarRef{fl, sizeVarp, VAccess::READ};
        sizeVarRefp->classOrPackagep(VN_AS(sizeVarp->user2p(), ଗଣ୍ଠି_Module));
        pinp->addPinsp(sizeVarRefp);
        ଫେରନ୍ତୁ pinp->makeStmt();
    }

    ଶୂନ୍ୟ addFrozenSizePin(File_ଧାଡ଼ି* ଅଚଳ fl, AstVar* ଅଚଳ arrVarp, AstFunc* ଅଚଳ randomizep,
                          AstVar* ଅଚଳ genp, AstVar* ଅଚଳ randModeVarp) {
        ଅଚଳ RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        ଯଦି (!rmode.usesMode) ଫେରନ୍ତୁ;
        AstVar* ଅଚଳ sizeVarp = VN_AS(arrVarp->user4p(), Var);
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ arrClassp = VN_AS(arrVarp->user2p(), ଗଣ୍ଠି_Module);
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ sizeClassp = VN_AS(sizeVarp->user2p(), ଗଣ୍ଠି_Module);
        AstVarRef* ଅଚଳ sizeWritep = ନୂତନ AstVarRef{fl, sizeClassp, sizeVarp, VAccess::WRITE};
        ଅଚଳ VCMethod sizeMethod = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_SIZE
                                        : VCMethod::DYN_SIZE;
        AstCMethodHard* ଅଚଳ currentSizep = ନୂତନ AstCMethodHard{
            fl, ନୂତନ AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, sizeMethod};
        currentSizep->dtypep(sizeVarp->dtypep());
        randomizep->addStmtsp(ନୂତନ AstAssign{fl, sizeWritep, currentSizep});
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ modep = newModeBitRead(arrVarp, nullptr, randModeVarp, fl);
        randomizep->addStmtsp(
            ନୂତନ AstIf{fl, ନୂତନ AstLogNot{fl, modep}, newPinSizeVariable(fl, arrVarp, genp)});
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_ଅଙ୍କ);
        VL_RESTORER_COPY(m_randArrayUniqueNames);
        m_modp = nodep;
        m_rand_Case_ଅଙ୍କ = 0;
        m_randArrayUniqueNames.reset();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_ଅଙ୍କ);
        VL_RESTORER(m_dist_ଅଙ୍କ);
        m_modp = nodep;
        m_rand_Case_ଅଙ୍କ = 0;
        m_dist_ଅଙ୍କ = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        ଯଦି (!nodep->user1()) ଫେରନ୍ତୁ;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->baseMostClassp()->needRNG(ସତ୍ୟ);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ଅଚଳ ତାର୍କିକ basicFirst = classOwnsGlobalConstraint(nodep);

        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        AstFunc* ଅଚଳ randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ଅଚଳ fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ଅଚଳ randModeVarp = getRandModeVarFromClass(nodep);
        addPrePostCall(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ଯଦି (classHasRandClassMembers(nodep)) {
            AstTask* ଅଚଳ preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->addStmtsp((ନୂତନ AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* beginValp = nullptr;
        AstVar* genp = getRandomGenerator(nodep);
        ଯଦି (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ଅଚଳ prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ଅଚଳ classp, AstConstraint* ଅଚଳ constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                ଯଦି (!taskp) {
                    taskp = newSetupConstraintTask(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                ଯଦି (AstTask* ଅଚଳ resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ଅଚଳ resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ଅଚଳ resizeTaskRefp
                        = ନୂତନ AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->addStmtsp(resizeTaskRefp->makeStmt());
                }

                ଯଦି (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                ଯଦି (constrp->itemsp()) {
                    lowerDistConstraints(taskp, constrp->itemsp(), randModeVarp);
                }
                VInsertionSet<AstVar*>& sizeArrays = m_sizeConstrainedArrays[classp];
                Constraint_ପଦସମଷ୍ଟି_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), nullptr,
                    genp,          randModeVarp, m_writtenVars,     &m_uniqueConstraintId,
                    m_uniqueNames, randomizep,   &sizeArrays,       prepareArraysTaskp};
                ଯଦି (constrp->itemsp()) {
                    taskp->addStmtsp(wrapIfConstraintMode(
                        nodep, constrp, constrp->itemsp()->unlinkFrBackWithNext()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ଅଚଳ auto constraintDepth = [](ଅଚଳ AstConstraint* constrp) {
                    ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth = 0;
                    size_t pos = 0;
                    ଯେତେବେଳେ ((pos = constrp->name().find(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++depth;
                        pos += std::strlen(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    ଫେରନ୍ତୁ depth;
                };
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxDepth = 0;
                nodep->foreachMember([&](AstClass* ଅଚଳ, AstConstraint* ଅଚଳ constrp) {
                    maxDepth = std::max(maxDepth, constraintDepth(constrp));
                });
                AstTask* ଅଚଳ setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା d = maxDepth; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ଅଚଳ classp, AstConstraint* ଅଚଳ constrp) {
                            ଯଦି (constraintDepth(constrp) != d) ଫେରନ୍ତୁ;
                            AstTask* ଅଚଳ taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ଅଚଳ setupTaskRefp
                                = ନୂତନ AstTaskRef{constrp->fileline(), taskp};
                            setupTaskRefp->classOrPackagep(classp);
                            setupAllTaskp->addStmtsp(setupTaskRefp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertionSet<AstVar*> sizeArrayVars;
            ଯଦି (nodep->extendsp()) {
                AstClass* parentClassp = nodep->extendsp()->classp();
                ଯେତେବେଳେ (parentClassp) {
                    ଅଚଳ auto sizeArraysIt = m_sizeConstrainedArrays.find(parentClassp);
                    ଯଦି (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                        ପାଇଁ (AstVar* ଅଚଳ arrVarp : sizeArraysIt->second) {
                            sizeArrayVars.ଭର୍ତ୍ତି(arrVarp);
                        }
                    }
                    AstFunc* ଅଚଳ parentRandomizep
                        = VN_CAST(m_memberMap.findMember(parentClassp, "randomize"), Func);
                    ଯଦି (parentRandomizep && parentRandomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = parentRandomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ତାର୍କିକ foundClearConstraints = ମିଥ୍ୟା;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                ଯଦି (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->addStmtsp(stmtp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                                } ଅନ୍ୟଥା ଯଦି (methodp->method()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    foundClearConstraints = ସତ୍ୟ;
                                }
                            });
                            ଯଦି (foundClearConstraints) ଭାଙ୍ଗ;
                        }
                    }
                    parentClassp
                        = parentClassp->extendsp() ? parentClassp->extendsp()->classp() : nullptr;
                }
            }
            randomizep->addStmtsp(implementConstraintsClear(fl, genp));

            ଅଚଳ auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
            ଯଦି (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                ପାଇଁ (AstVar* ଅଚଳ arrVarp : sizeArraysIt->second) sizeArrayVars.ଭର୍ତ୍ତି(arrVarp);
            }
            AstTask* ଅଚଳ setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
            ଯଦି (!sizeArrayVars.empty() && prepareArraysTaskp->stmtsp()) {
                Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ preparep = (ନୂତନ AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    ତାର୍କିକ writesArray = ମିଥ୍ୟା;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    ଯଦି (!writesArray) ଜାରି;
                    stmtp->addHereThisAsNext(preparep);
                    ଭାଙ୍ଗ;
                }
            }
            AstTaskRef* ଅଚଳ setupTaskRefp = ନୂତନ AstTaskRef{fl, setupAllTaskp};
            randomizep->addStmtsp(setupTaskRefp->makeStmt());

            Ast_ଗଣ୍ଠି_Module* ଅଚଳ genModp = VN_AS(genp->user2p(), ଗଣ୍ଠି_Module);

            AstCExpr* ଅଚଳ solverCallp = ନୂତନ AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(ନୂତନ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->add(".next(__Vm_rng)");
            ଯଦି (!sizeArrayVars.empty()) {
                AstVar* ଅଚଳ sizeOkVarp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                sizeOkVarp->funcLocal(ସତ୍ୟ);
                randomizep->addStmtsp(sizeOkVarp);

                AstVar* ଅଚଳ finalOkVarp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                finalOkVarp->funcLocal(ସତ୍ୟ);
                randomizep->addStmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                ପାଇଁ (AstVar* ଅଚଳ arrVarp : sizeArrayVars)
                    addFrozenSizePin(fl, arrVarp, randomizep, genp, randModeVarp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->addStmtsp(
                    ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, sizeOkVarp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                ଯଦି (AstTask* ଅଚଳ resizeAllTaskp
                    = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ଅଚଳ resizeTaskRefp = ନୂତନ AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ଅଚଳ ifp = ନୂତନ AstIf{fl, ନୂତନ AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), nullptr};
                    randomizep->addStmtsp(ifp);
                }

                // Refresh array element tables after resize
                ପାଇଁ (ଅଚଳ auto& arrVarp : sizeArrayVars) {
                    addArrayElemRefresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->addStmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ଅଚଳ setupTaskRefp2 = ନୂତନ AstTaskRef{fl, setupAllTaskp};
                randomizep->addStmtsp(setupTaskRefp2->makeStmt());

                ପାଇଁ (AstVar* ଅଚଳ arrVarp : sizeArrayVars) {
                    randomizep->addStmtsp(newPinSizeVariable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ଅଚଳ solverCallp2 = ନୂତନ AstCExpr{fl};
                solverCallp2->dtypeSetBit();
                solverCallp2->add(ନୂତନ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->add(".next(__Vm_rng)");
                randomizep->addStmtsp(ନୂତନ AstAssign{
                    fl, ନୂତନ AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = ନୂତନ AstAnd{fl, ନୂତନ AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                       ନୂତନ AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } ଅନ୍ୟଥା {
                beginValp = solverCallp;
            }
            ଯଦି (randModeVarp) {
                Ast_ଗଣ୍ଠି_Module* ଅଚଳ randModeClassp = VN_AS(randModeVarp->user2p(), Class);
                Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newp
                    = VN_AS(m_memberMap.findMember(randModeClassp, "new"), ଗଣ୍ଠି_FTask);
                UASSERT_OBJ(newp, randModeClassp, "No new() in class");
                addSetRandMode(newp, genp, randModeVarp);
            }
            ଯଦି (AstVar* ଅଚଳ staticRandModeVarp = getStaticRandModeVar(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_ଗଣ୍ଠି_FTask* ଅଚଳ newp = VN_AS(m_memberMap.findMember(nodep, "new"), ଗଣ୍ଠି_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                addSetStaticRandMode(newp, genp, staticRandModeVarp);
            }
        } ଅନ୍ୟଥା {
            beginValp = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1};
        }

        addUpdateRandVarsBody(nodep, randomizep);

        AstFunc* ଅଚଳ basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        addBasicRandomizeBody(basicRandomizep, nodep, randModeVarp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ଅଚଳ fvarRefp = ନୂତନ AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->addStmtsp(ନୂତନ AstAssign{
            fl, fvarRefp, basicFirst ? ନୂତନ AstFuncRef{fl, basicRandomizep} : beginValp});

        ଅଚଳ auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
        ଅଚଳ ତାର୍କିକ needsSizePhase
            = sizeArraysIt != m_sizeConstrainedArrays.end() && !sizeArraysIt->second.empty();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ଅଚଳ auto emitResizeFallback = [&]() {
            ଯଦି (needsSizePhase) ଫେରନ୍ତୁ;
            ଯଦି (AstTask* ଅଚଳ resizeAllTaskp
                = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ଅଚଳ resizeTaskRefp = ନୂତନ AstTaskRef{fl, resizeAllTaskp};
                randomizep->addStmtsp(resizeTaskRefp->makeStmt());
            }
        };
        ଯଦି (!basicFirst) emitResizeFallback();

        AstVarRef* ଅଚଳ fvarRefReadp = fvarRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        fvarRefReadp->access(VAccess::READ);

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ secondHalfp
            = basicFirst ? beginValp : ନୂତନ AstFuncRef{fl, basicRandomizep};
        randomizep->addStmtsp(ନୂତନ AstAssign{fl, fvarRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                                            ନୂତନ AstAnd{fl, fvarRefReadp, secondHalfp}});

        ଯଦି (basicFirst) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ଯଦି (classHasRandClassMembers(nodep)) {
            AstTask* ଅଚଳ postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->addStmtsp((ନୂତନ AstTaskRef{fl, postTaskp})->makeStmt());
        }

        addPrePostCall(nodep, randomizep, "post_randomize");
        nodep->user1(ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ visit(AstRandCase* nodep) override {
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
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ sumDTypep = nodep->findUInt64DType();

        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଅଚଳ std::string name = "__Vrandcase" + cvtToStr(m_rand_Case_ଅଙ୍କ++);
        AstVar* ଅଚଳ randVarp = ନୂତନ AstVar{fl, VVarType::BLOCKTEMP, name, sumDTypep};
        randVarp->noSubst(ସତ୍ୟ);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        addTempVarsp(randVarp);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sump = ନୂତନ AstConst{fl, AstConst::Widthed_ମୂଲ୍ୟ{}, 64, 0};
        Ast_ଗଣ୍ଠି_If* ଅଚଳ firstIfsp
            = ନୂତନ AstIf{fl, ନୂତନ AstConst{fl, AstConst::BitFalse{}}, nullptr, nullptr};
        Ast_ଗଣ୍ଠି_If* ifsp = firstIfsp;

        ପାଇଁ (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = itemp->condsp()->unlinkFrBack();
            sump
                = ନୂତନ AstAdd{condp->fileline(), sump, ନୂତନ AstExtend{itemp->fileline(), condp, 64}};
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlinkFrBackWithNext() : nullptr;
            AstVarRef* ଅଚଳ randVarRefp = ନୂତନ AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_ଗଣ୍ଠି_If* ଅଚଳ newifp
                = ନୂତନ AstIf{itemp->fileline(),
                            ନୂତନ AstLte{condp->fileline(), randVarRefp, sump->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)},
                            stmtsp, nullptr};
            ifsp->addElsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ଅଚଳ dispp = ନୂତନ AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            nullptr, nullptr};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->addElsesp(dispp);

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ randp = ନୂତନ AstRand{fl, nullptr, ମିଥ୍ୟା};
        randp->dtypeSetUInt64();
        AstVarRef* ଅଚଳ randVarRefp = ନୂତନ AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_ଗଣ୍ଠି* ଅଚଳ newp
            = ନୂତନ AstAssign{fl, randVarRefp,
                            ନୂତନ AstAdd{fl, ନୂତନ AstConst{fl, AstConst::Unsized64{}, 1},
                                       ନୂତନ AstModDiv{fl, randp, sump}}};
        newp->addNext(firstIfsp);
        ଯଦି (debug() >= 9) newp->dump_ବୃକ୍ଷ_And_Next(cout, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (nodep->name() == "rand_mode") {
            AstMethodCall* ଅଚଳ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = methodCallp ? methodCallp->fromp() : nullptr;
            ଅଚଳ RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_modp);
            UASSERT_OBJ(randModeTarget.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ଅଚଳ receiverp = randModeTarget.receiverp;
            ଅଚଳ ତାର୍କିକ isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_ଗଣ୍ଠି* classLevelStaticLoopp = nullptr;
            ଯଦି (isClassLevel && nodep->argsp()) {
                ଯଦି (AstVar* ଅଚଳ sVarp = getStaticRandModeVar(randModeTarget.classp)) {
                    File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ staticLhsp
                        = makeModeAssignLhs(fl, randModeTarget.classp, nullptr, sVarp);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ argClonep = nodep->argsp()->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    classLevelStaticLoopp = makeModeSetLoop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ଅଚଳ ତାର୍କିକ receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ଅଚଳ randModeVarp = receiverIsStaticRand
                                             ? getStaticRandModeVar(randModeTarget.classp)
                                             : getRandModeVarFromClass(randModeTarget.classp);
            ଯଦି (!randModeVarp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_ପଦସମଷ୍ଟି), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(ଠେଲ_Deletep(m_stmtp), m_stmtp = nullptr);
                ଫେରନ୍ତୁ;
            }
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = makeModeAssignLhs(nodep->fileline(), randModeTarget.classp,
                                                        randModeTarget.fromp, randModeVarp);
            replaceWithModeAssignAndAppend(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : nullptr, lhsp,
                classLevelStaticLoopp);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (nodep->name() == "constraint_mode") {
            AstMethodCall* ଅଚଳ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp = methodCallp ? methodCallp->fromp() : nullptr;
            AstConstraint* constrp = nullptr;
            AstClass* classp = VN_CAST(m_modp, Class);
            ଯଦି (AstConstraintRef* ଅଚଳ constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                ଯଦି (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } ଅନ୍ୟଥା ଯଦି (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ଅଚଳ constraintModeVarp = (constrp && constrp->isStatic())
                                                   ? getStaticConstraintModeVar(classp)
                                                   : getConstraintModeVar(classp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp
                = makeModeAssignLhs(nodep->fileline(), classp, fromp, constraintModeVarp);
            replaceWithModeAssign(nodep, constrp, lhsp);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            ଫେରନ୍ତୁ;
        }

        ଯଦି (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ଅଚଳ ତାର୍କିକ inStaticContext = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ଅଚଳ stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ଅଚଳ randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            ଯଦି (inStaticContext) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(ସତ୍ୟ);
            }
            randomizeFuncp->addStmtsp(
                ନୂତନ AstAssign{nodep->fileline(),
                              ନୂତନ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              ନୂତନ AstConst{nodep->fileline(), AstConst::Widthed_ମୂଲ୍ୟ{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା argn = 0;
            AstWith* ଅଚଳ withp = nodep->withp();
            ପାଇଁ (ଅଚଳ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = argp->exprp();
                AstCMethodHard* ଅଚଳ basicMethodp = ନୂତନ AstCMethodHard{
                    nodep->fileline(),
                    ନୂତନ AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ଅଚଳ refvarp
                    = ନୂତନ AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::to_string(++argn), exprp->dtypep()};
                refvarp->setStdRandomizeArg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                ଯଦି (withp) {
                    withp->foreach([&](Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exp) {
                        ଯଦି (is_Similar_ଗଣ୍ଠି(exp, exprp)) {
                            AstVarRef* ଅଚଳ replaceVar
                                = ନୂତନ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            exp->replaceWith(replaceVar);
                            replaceVar->user1(exp->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(ଠେଲ_Deletep(exp), exp);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(ସତ୍ୟ);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->addStmtsp(refvarp);

                ଅଚଳ size_t width = exprp->width();
                basicMethodp->addPinsp(
                    ନୂତନ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->addPinsp(
                    ନୂତନ AstConst{nodep->fileline(), AstConst::Unsized64{}, width});
                basicMethodp->dtypeSetBit();

                randomizeFuncp->addStmtsp(ନୂତନ AstAssign{
                    nodep->fileline(),
                    ନୂତନ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    ନୂତନ AstAnd{nodep->fileline(),
                               ନୂତନ AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            ଯଦି (withp) {
                File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, nullptr,
                                                                inStaticContext);
                withCapturep->addFunctionArguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                ଯଦି (stdrand) {
                    randomizeFuncp->addStmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_ଗଣ୍ଠି* ଅଚଳ capturedTreep = withp->exprp()->unlinkFrBackWithNext();
                randomizeFuncp->addStmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_ପଦସମଷ୍ଟି_Visitor{
                        nullptr, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        nullptr, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
                }
                AstCExpr* ଅଚଳ solverCallp = ନୂତନ AstCExpr{fl};
                solverCallp->dtypeSetBit();
                solverCallp->add(ନୂତନ AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->add(".next()");
                AstVar* ଅଚଳ fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ andExprp
                    = ନୂତନ AstAnd{fl, ନୂତନ AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->addStmtsp(
                    ନୂତନ AstAssign{fl, ନୂତନ AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlinkFrBack()->delete_ବୃକ୍ଷ(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            ଯଦି (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            ଯଦି (withCapturep) nodep->addArgsp(withCapturep->getArgs());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            ଫେରନ୍ତୁ;
        }
        handleRandomizeArgs(nodep);

        AstWith* ଅଚଳ withp = nodep->withp();
        ଯଦି (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            ଫେରନ୍ତୁ;
        }
        withp->unlinkFrBack();
        iterateChildren(nodep);

        AstClass* classp = nullptr;
        ଯଦି (AstMethodCall* ଅଚଳ callp = VN_CAST(nodep, MethodCall)) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            ଅଚଳ AstClassRefDType* ଅଚଳ classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } ଅନ୍ୟଥା {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        ଯଦି (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ଅଚଳ classGenp = getRandomGenerator(classp);
        AstVar* ଅଚଳ localGenp
            = ନୂତନ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(ସତ୍ୟ);

        AstFunc* ଅଚଳ randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inlineUniqueNames.get(nodep), ମିଥ୍ୟା);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ଅଚଳ std::pair<ତାର୍କିକ, ତାର୍କିକ> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<ତାର୍କିକ, ତାର୍କିକ>{ମିଥ୍ୟା, ମିଥ୍ୟା};
        ଯଦି (prePostWrap.first) {
            addVirtualPrePostCall(randomizeFuncp, classp, "pre_randomize");
        } ଅନ୍ୟଥା {
            addPrePostCall(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ଯଦି (classHasRandClassMembers(classp)) {
            AstTask* ଅଚଳ preTaskp = getCreateNestedCallbackTask(classp, "pre");
            ଯଦି (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->addStmtsp((ନୂତନ AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ଅଚଳ CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.addFunctionArguments(randomizeFuncp);

        // Add constraints clearing code
        ଯଦି (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ଅଚଳ mainRandomizep
                = VN_CAST(m_memberMap.findMember(classp, "randomize"), Func);
            ଯଦି (mainRandomizep && mainRandomizep->stmtsp()) {
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ତାର୍କିକ foundClearConstraints = ମିଥ୍ୟା;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        ଯଦି (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->addStmtsp(stmtp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                        } ଅନ୍ୟଥା ଯଦି (methodp->method() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            foundClearConstraints = ସତ୍ୟ;
                        }
                    });
                    ଯଦି (foundClearConstraints) ଭାଙ୍ଗ;
                }
            }
            randomizeFuncp->addStmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->addStmtsp(localGenp);

        AstFunc* ଅଚଳ basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ଅଚଳ basicRandomizeFuncCallp
            = ନୂତନ AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        ଯଦି (classGenp) {
            AstTask* ଅଚଳ constrSetupFuncp = getCreateConstraintSetupFunc(classp, m_memberMap);
            AstTaskRef* ଅଚଳ callp = ନୂତନ AstTaskRef{nodep->fileline(), constrSetupFuncp};
            randomizeFuncp->addStmtsp(callp->makeStmt());
            randomizeFuncp->addStmtsp(ନୂତନ AstAssign{
                nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                ନୂତନ AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), ଗଣ୍ଠି_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ଅଚଳ randModeVarp = getRandModeVarFromClass(classp);
        ଯଦି (!classGenp && randModeVarp) addSetRandMode(randomizeFuncp, localGenp, randModeVarp);
        ଯଦି (!classGenp) {
            ଯଦି (AstVar* ଅଚଳ sVarp = getStaticRandModeVar(classp)) {
                addSetStaticRandMode(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_ଗଣ୍ଠି* ଅଚଳ capturedTreep = withp->exprp()->unlinkFrBackWithNext();
        randomizeFuncp->addStmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_ଗଣ୍ଠି* inlineResizeStmtsp = nullptr;
        {
            std::vector<AstCMethodHard*> sizeMethodps;
            capturedTreep->foreachAndNext([&](AstCMethodHard* methodp) {
                ଯଦି (methodp->method() == VCMethod::DYN_SIZE
                    || methodp->method() == VCMethod::ASSOC_SIZE) {
                    sizeMethodps.ଠେଲ_back(methodp);
                }
            });
            ପାଇଁ (AstCMethodHard* ଅଚଳ methodp : sizeMethodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = nullptr;
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ମୂଳ_Exprp = methodp->fromp();
                ଯଦି (AstMemberSel* memberSelp = VN_CAST(ମୂଳ_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    ଯେତେବେଳେ (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ fromp = memberSelp->fromp();
                    ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ varRefp = VN_CAST(fromp, VarRef)) {
                        ଯଦି (varRefp->varp() == captured.getThisp()) ଜାରି;
                    } ଅନ୍ୟଥା {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ varRefp = VN_CAST(ମୂଳ_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                ଯଦି (!arrVarp) ଜାରି;
                // Only handle rand-declared dynamic/assoc array variables
                ଯଦି (!arrVarp->rand().isRandomizable()) ଜାରି;
                // Do not randomize associative array size (IEEE 18.4)
                ଯଦି (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) ଜାରି;
                File_ଧାଡ଼ି* ଅଚଳ fl = methodp->fileline();
                ତାର୍କିକ wasCreated = ମିଥ୍ୟା;
                AstClass* ଅଚଳ varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ଅଚଳ sizeVarp = createOrGetSizeVar(varClassp, arrVarp, fl,
                                                            methodp->findIntDType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                ଯଦି (wasCreated) {
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resizeVarRefp = buildMemberSelExprp(ମୂଳ_Exprp, arrVarp);
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ sizeExprp = buildMemberSelExprp(ମୂଳ_Exprp, sizeVarp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->addNext(createSizeGteZeroConstraint(fl, sizeVarp, ମୂଳ_Exprp));
                    AstCMethodHard* ଅଚଳ resizep
                        = ନୂତନ AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, sizeExprp};
                    resizep->dtypep(methodp->findVoidDType());
                    inlineResizeStmtsp
                        = Ast_ଗଣ୍ଠି::addNext(inlineResizeStmtsp, ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{fl, resizep});
                }
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ size_ଗଣ୍ଠି_Exprp = buildMemberSelExprp(ମୂଳ_Exprp, sizeVarp);
                methodp->replaceWith(size_ଗଣ୍ଠି_Exprp);
                VL_DO_DANGLING(methodp->delete_ବୃକ୍ଷ(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_ପଦସମଷ୍ଟି_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                randModeVarp, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
        }

        // Call the solver and set return value
        AstCExpr* ଅଚଳ solverCallp = ନୂତନ AstCExpr{nodep->fileline()};
        solverCallp->dtypeSetBit();
        solverCallp->add(ନୂତନ AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->add(".next(__Vm_rng)");
        randomizeFuncp->addStmtsp(ନୂତନ AstAssign{
            nodep->fileline(),
            ନୂତନ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            ନୂତନ AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        ଯଦି (inlineResizeStmtsp) {
            AstVar* ଅଚଳ retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ଅଚଳ ifp = ନୂତନ AstIf{nodep->fileline(),
                                         ନୂତନ AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, nullptr};
            randomizeFuncp->addStmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ଯଦି (classHasRandClassMembers(classp)) {
            AstTask* ଅଚଳ postTaskp = getCreateNestedCallbackTask(classp, "post");
            ଯଦି (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->addStmtsp((ନୂତନ AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        ଯଦି (prePostWrap.second) {
            addVirtualPrePostCall(randomizeFuncp, classp, "post_randomize");
        } ଅନ୍ୟଥା {
            addPrePostCall(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->addArgsp(captured.getArgs());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_ବୃକ୍ଷ(), withp);
    }
    ଶୂନ୍ୟ visit(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        ଯଦି (m_constraintp && nodep->fromp()->user1()
            && (nodep->method() == VCMethod::ASSOC_SIZE
                || nodep->method() == VCMethod::DYN_SIZE)) {
            AstClass* ଅଚଳ classp = VN_AS(m_modp, Class);
            AstVarRef* ଅଚଳ queueVarRefp = VN_CAST(nodep->fromp(), VarRef);
            ଯଦି (!queueVarRefp) {
                // Warning from ConstraintExprVisitor will be thrown
                ଫେରନ୍ତୁ;
            }
            AstVar* ଅଚଳ queueVarp = queueVarRefp->varp();
            ତାର୍କିକ wasCreated = ମିଥ୍ୟା;
            AstVar* ଅଚଳ sizeVarp
                = createOrGetSizeVar(classp, queueVarp, fl, nodep->findIntDType(), wasCreated);
            ଯଦି (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                ଯଦି (!VN_IS(queueVarp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    ଯଦି (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ଅଚଳ sizeVarRefp = ନୂତନ AstVarRef{
                        fl, VN_AS(sizeVarp->user2p(), ଗଣ୍ଠି_Module), sizeVarp, VAccess::READ};
                    AstCMethodHard* ଅଚଳ resizep = ନୂତନ AstCMethodHard{
                        fl, nodep->fromp()->unlinkFrBack(), VCMethod::DYN_RESIZE, sizeVarRefp};
                    resizep->dtypep(nodep->findVoidDType());
                    resizerTaskp->addStmtsp(ନୂତନ Ast_Stmt_ପଦସମଷ୍ଟି{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->addItemsp(createSizeGteZeroConstraint(fl, sizeVarp));
            }
            AstVarRef* ଅଚଳ sizeVarRefp = ନୂତନ AstVarRef{fl, VN_AS(sizeVarp->user2p(), ଗଣ୍ଠି_Module),
                                                         sizeVarp, VAccess::READ};
            sizeVarRefp->user1(ସତ୍ୟ);
            nodep->replaceWith(sizeVarRefp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit RandomizeVisitor(AstNetlist* nodep)
        : m_inlineUniqueNames{"__Vrandwith"} {
        createRandomizeClassVars(nodep);
        // Flag local constraint leaves as solver-owned so __VBasicRand skips them.
        // Runs before any class is lowered. Only a randomized class counts, and
        // only a leaf owned by the constraint's own class: a leaf reached through
        // a global constraint stays basic-randomized so a standalone randomize()
        // of its type still randomizes it (issue #7833); a class-typed sub-object
        // is skipped so its own members are still basic-randomized.
        nodep->foreach([&](AstClass* ଅଚଳ classp) {
            ଯଦି (!classp->user1()) ଫେରନ୍ତୁ;
            classp->foreachMember([&](AstClass* ଅଚଳ ownerClassp, AstConstraint* ଅଚଳ constrp) {
                constrp->foreach([&](AstMemberSel* ଅଚଳ memberSelp) {
                    AstVar* ଅଚଳ varp = memberSelp->varp();
                    ଯଦି (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) ଫେରନ୍ତୁ;
                    ଯଦି (VN_AS(varp->user2p(), ଗଣ୍ଠି_Module) != ownerClassp) ଫେରନ୍ତୁ;
                    ଯଦି (!varp->user3()) varp->user3(ସତ୍ୟ);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(ଠେଲ_Deletep(constrp->unlinkFrBack()), constrp);
        });
    }
    ~RandomizeVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Randomize method class functions

ଶୂନ୍ୟ V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ଅଚଳ RandomizeMarkVisitor markVisitor{nodep};
        ଅଚଳ RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_ବୃକ୍ଷ("randomize", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ଅଚଳ std::string& name, ତାର୍କିକ allowVirtual,
                                       ତାର୍କିକ childDType) {
    AstFunc* funcp = VN_AS(memberMap.findMember(nodep, name), Func);
    ଯଦି (!funcp) {
        v3Global.useRandomizeMethods(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep
            = childDType
                  ? ନୂତନ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ଅଚଳ fvarp = childDType
                                  ? ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlagChildDType{}, dtypep}
                                  : ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3ErrorCode::NORETURN, ସତ୍ୟ);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(ସତ୍ୟ);
        fvarp->funcReturn(ସତ୍ୟ);
        fvarp->direction(VDirection::OUTPUT);
        nodep->addMembersp(funcp);
        funcp = ନୂତନ AstFunc{nodep->fileline(), name, nullptr, fvarp};
        ଯଦି (!childDType) funcp->dtypep(dtypep);
        funcp->classMethod(ସତ୍ୟ);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->addMembersp(funcp);
        memberMap.ଭର୍ତ୍ତି(nodep, funcp);
    }
    ଫେରନ୍ତୁ funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_ଗଣ୍ଠି_Module* nodep, ଅଚଳ std::string& name) {
    AstFunc* funcp = nullptr;
    v3Global.useRandomizeMethods(ସତ୍ୟ);
    Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
    AstVar* ଅଚଳ fvarp = ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(ସତ୍ୟ);
    fvarp->funcReturn(ସତ୍ୟ);
    fvarp->direction(VDirection::OUTPUT);
    funcp = ନୂତନ AstFunc{nodep->fileline(), name, nullptr, fvarp};
    funcp->dtypep(dtypep);
    ଯଦି (VN_IS(nodep, Class)) {
        funcp->classMethod(ସତ୍ୟ);
    } ଅନ୍ୟଥା {
        funcp->classMethod(ସତ୍ୟ);
        funcp->isStatic(ସତ୍ୟ);
    }
    nodep->addStmtsp(funcp);
    ଫେରନ୍ତୁ funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ଅଚଳ basep = nodep->baseMostClassp();
    AstFunc* funcp = VN_AS(memberMap.findMember(basep, "srandom"), Func);
    ଯଦି (!funcp) {
        v3Global.useRandomizeMethods(ସତ୍ୟ);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep
            = basep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ଅଚଳ ivarp = ନୂତନ AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(ସତ୍ୟ);
        ivarp->direction(VDirection::ନିବେଶ);
        funcp = ନୂତନ AstFunc{basep->fileline(), "srandom", ivarp, nullptr};
        funcp->dtypep(basep->findVoidDType());
        funcp->classMethod(ସତ୍ୟ);
        funcp->isVirtual(ମିଥ୍ୟା);
        basep->addMembersp(funcp);
        memberMap.ଭର୍ତ୍ତି(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        ଯଦି (basep->name() == "process") {
            funcp->addStmtsp(ନୂତନ AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } ଅନ୍ୟଥା {
            funcp->addStmtsp(ନୂତନ AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(ସତ୍ୟ);
        }
    }
    ଫେରନ୍ତୁ funcp;
}
