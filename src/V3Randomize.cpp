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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Randomize.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Container.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3Task.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

#ಅನ್ತರ್ಭಾವ <queue>
#ಅನ್ತರ್ಭಾವ <tuple>
#ಅನ್ತರ್ಭಾವ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

ಗಣನಾ ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
ಸ್ಥಿರ AstVar* getRandModeVarFromClass(Ast_ಗಂಟು_Module* classp) {
    ಯಾವತ್ (classp) {
        ಯದಿ (classp->user2p()) ಪ್ರತ್ಯಾಗಮ VN_AS(classp->user2p(), Var);
        AstClass* ಧ್ರುವ cp = VN_CAST(classp, Class);
        ಯದಿ (!cp || !cp->extendsp()) ಪ್ರತ್ಯಾಗಮ nullptr;
        classp = cp->extendsp()->classp();
    }
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ಸ್ಥಿರ AstVar* getRandomGenerator(AstClass* ಧ್ರುವ classp) {
    ಯದಿ (classp->user3p()) ಪ್ರತ್ಯಾಗಮ VN_AS(classp->user3p(), Var);
    ಯದಿ (classp->extendsp()) ಪ್ರತ್ಯಾಗಮ getRandomGenerator(classp->extendsp()->classp());
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ಸ್ಥಿರ AstTask* getCreateConstraintSetupFunc(AstClass* classp, VMemberMap& memberMap) {
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ name = "__Vsetup_constraints";
    AstTask* setupAllTaskp = VN_AS(memberMap.findMember(classp, name), Task);
    ಯದಿ (setupAllTaskp) ಪ್ರತ್ಯಾಗಮ setupAllTaskp;
    setupAllTaskp = ನೂತನ AstTask{classp->fileline(), "__Vsetup_constraints", nullptr};
    setupAllTaskp->classMethod(ಸತ್ಯ);
    setupAllTaskp->isVirtual(ಸತ್ಯ);
    classp->addMembersp(setupAllTaskp);
    memberMap.ಸೇರಿಸು(classp, setupAllTaskp);
    ಪ್ರತ್ಯಾಗಮ setupAllTaskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

ಸಂರಚನಾ RandModeTarget final {
    // MEMBERS
    AstVar* ಧ್ರುವ receiverp;  // Variable that is the target of the rand_mode() call
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp;  // Expression from which the rand_mode() call originates
    AstClass* ಧ್ರುವ classp;  // Class in which rand_mode should be set

    // METHODS
    ಸ್ಥಿರ RandModeTarget get(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_Module* modp) {
        ಯದಿ (!fromp) ಪ್ರತ್ಯಾಗಮ {nullptr, nullptr, VN_AS(modp, Class)};
        ಯದಿ (AstCMethodHard* ಧ್ರುವ methodHardp = VN_CAST(fromp, CMethodHard)) {
            ಪ್ರತ್ಯಾಗಮ RandModeTarget::get(methodHardp->fromp(), modp);
        }
        AstVar* receiverp = nullptr;
        AstClass* classp = VN_CAST(modp, Class);
        ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            ಯದಿ (receiverp->isRand()) {
                fromp = nullptr;
                ಯದಿ (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            ಯದಿ (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        ಯದಿ (!receiverp->isRand()) {
            AstClassRefDType* ಧ್ರುವ classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            ಯದಿ (classRefDtp) classp = classRefDtp->classp();
        }
        ಪ್ರತ್ಯಾಗಮ {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

ಸಙ್ಘ RandomizeMode final {
    // MEMBERS
    ಸಂರಚನಾ {
        ತಾರ್ಕಿಕ usesMode : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t index : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ಸ್ಥಿರ Ast_ಗಂಟು_DType* arrayElementDTypep(Ast_ಗಂಟು_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ಯಾವತ್ (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    ಪ್ರತ್ಯಾಗಮ dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ಸ್ಥಿರ ತಾರ್ಕಿಕ dtypeContainsReal(Ast_ಗಂಟು_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ subp = dtypep->subDTypep()) ಪ್ರತ್ಯಾಗಮ dtypeContainsReal(subp);
    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ structp = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
        ಯದಿ (structp->packed()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತಿ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            ಯದಿ (dtypeContainsReal(memberp->subDTypep())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = VN_CAST(dtypep, BasicDType);
    ಪ್ರತ್ಯಾಗಮ basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
ಸ್ಥಿರ ಶೂನ್ಯ checkRandTypeEligibility(Ast_ಗಂಟು* contextp, Ast_ಗಂಟು_DType* dtypep, ತಾರ್ಕಿಕ isRandc) {
    dtypep = dtypep->skipRefp();
    ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಯದಿ (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        ಯದಿ (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ structp = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
        ಯದಿ (structp->packed()) ಪ್ರತ್ಯಾಗಮ;  // Integral by construction; members checked separately
        ಯದಿ (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಪ್ರತಿ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            ಯದಿ (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    ಯದಿ (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        ಯದಿ (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        ಪ್ರತ್ಯಾಗಮ;
    }
    ವಿಕಲ್ಪ (basicp->keyword()) {
    ಪಕ್ಷ VBasicDTypeKwd::STRING:
    ಪಕ್ಷ VBasicDTypeKwd::CHANDLE:
    ಪಕ್ಷ VBasicDTypeKwd::EVENT: {
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        ಭಙ್ಗ;
    }
    ಸ್ವಭಾವ: ಭಙ್ಗ;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

ವರ್ಗ RandomizeMarkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
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
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;

    using DerivedSet = std::unordered_set<AstClass*>;
    using BaseToDerivedMap = std::unordered_map<ಧ್ರುವ AstClass*, DerivedSet>;

    BaseToDerivedMap m_baseToDerivedMap;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = nullptr;  // Current class
    Ast_ಗಂಟು* m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp = nullptr;  // Current constraint or constraint if expression
    Ast_ಗಂಟು_Module* m_modp;  // Current module
    Ast_ಗಂಟು_Stmt* m_stmtp = nullptr;  // Current statement
    Ast_ಗಂಟು_FTask_Ref* m_stdRandCallp = nullptr;  // Current std::randomize() call
    ತಾರ್ಕಿಕ m_inStdWith = ಅಸತ್ಯ;  // True when inside a 'with {}' clause
    std::set<Ast_ಗಂಟು_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = nullptr;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    ಶೂನ್ಯ markGlobalConstrainedVars(AstClass* classp) {
        ಪ್ರತಿ (ಧ್ರುವ AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            ಪ್ರತಿ (Ast_ಗಂಟು* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ಧ್ರುವ varp = VN_CAST(memberp, Var);
                ಯದಿ (!varp) ಅನುವರ್ತ;
                ಯದಿ (varp->rand().isRandomizable()) varp->globalConstrained(ಸತ್ಯ);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    ತಾರ್ಕಿಕ isVarInStdRandomizeArgs(ಧ್ರುವ AstVar* varp) ಧ್ರುವ {
        ಯದಿ (!m_inStdWith || !m_stdRandCallp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಪ್ರತಿ (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            ಯಾವತ್ (exprp) {
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(exprp, ಗಂಟು_Var_Ref)) {
                    ಯದಿ (varrefp->varp() == varp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    ಭಙ್ಗ;
                }
                ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ memberselp = VN_CAST(exprp, MemberSel)) {
                    ಯದಿ (memberselp->varp() == varp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    exprp = memberselp->fromp();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCMethodHard* ಧ್ರುವ methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } ಅನ್ಯಥಾ {
                    ಭಙ್ಗ;
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ markMembers(ಧ್ರುವ AstClass* nodep) {
        ಪ್ರತಿ (ಧ್ರುವ AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            ಪ್ರತಿ (Ast_ಗಂಟು* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ಧ್ರುವ varp = VN_CAST(memberp, Var);
                ಯದಿ (!varp) ಅನುವರ್ತ;
                // If member is randomizable and of class type, mark its class
                ಯದಿ (varp->rand().isRandomizable()) {
                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ varDtypep = varp->dtypep()->skipRefp();
                    ಧ್ರುವ AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    ಯದಿ (!classRefp) {
                        ಧ್ರುವ Ast_ಗಂಟು_DType* subDTypep = varDtypep->subDTypep();
                        ಯದಿ (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    ಯದಿ (classRefp) {
                        AstClass* ಧ್ರುವ rclassp = classRefp->classp();
                        ಯದಿ (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    ಯದಿ (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        RandomizeMode randMode = {};
                        randMode.usesMode = ಸತ್ಯ;
                        varp->user1(randMode.asUQuad);
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ markDerived(ಧ್ರುವ AstClass* nodep) {
        ಧ್ರುವ auto it = m_baseToDerivedMap.find(nodep);
        ಯದಿ (it != m_baseToDerivedMap.end()) {
            ಪ್ರತಿ (auto* classp : it->second) {
                ಯದಿ (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    ಶೂನ್ಯ markAllDerived() {
        ಪ್ರತಿ (ಧ್ರುವ auto& p : m_baseToDerivedMap) {
            ಯದಿ (p.first->user1()) markDerived(p.first);
        }
    }
    ಶೂನ್ಯ setPackageRefs() {
        ಪ್ರತಿ (Ast_ಗಂಟು_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), ಗಂಟು_Module));
        }
    }
    ಶೂನ್ಯ markNestedGlobalConstrainedRecurse(Ast_ಗಂಟು* nodep) {
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) {
            AstVar* ಧ್ರುವ varp = refp->varp();
            ಯದಿ (varp->globalConstrained()) ಪ್ರತ್ಯಾಗಮ;
            varp->globalConstrained(ಸತ್ಯ);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(nodep, MemberSel)) {
            ಯದಿ (memberSelp->varp()) {
                AstVar* ಧ್ರುವ varp = memberSelp->varp();
                ಯದಿ (varp->globalConstrained()) ಪ್ರತ್ಯಾಗಮ;
                varp->globalConstrained(ಸತ್ಯ);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildMemberSelChain(AstVarRef* ಬೇರು_Var_Refp, ಧ್ರುವ std::vector<AstVar*>& path) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = ಬೇರು_Var_Refp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        ಪ್ರತಿ (AstVar* memberVarp : path) {
            AstMemberSel* memberSelp
                = ನೂತನ AstMemberSel{ಬೇರು_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        ಪ್ರತ್ಯಾಗಮ exprp;
    }

    // Process a single constraint during nested constraint cloning
    ಶೂನ್ಯ processNestedConstraint(AstConstraint* ಧ್ರುವ constrp, AstVarRef* ಬೇರು_Var_Refp,
                                 ಧ್ರುವ std::vector<AstVar*>& newPath, AstClass* targetClassp) {
        std::string pathPrefix = ಬೇರು_Var_Refp->name();
        ಪ್ರತಿ (AstVar* pathMemberVarp : newPath) {
            pathPrefix += GLOBAL_CONSTRAINT_SEPARATOR + pathMemberVarp->name();
        }

        ಧ್ರುವ std::string newName = pathPrefix + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        ತಾರ್ಕಿಕ isDuplicate = ಅಸತ್ಯ;
        targetClassp->foreachMember([&](AstClass* ಧ್ರುವ, AstConstraint* ಧ್ರುವ existingConstrp) {
            ಯದಿ (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = ಸತ್ಯ;
            }
        });
        ಯದಿ (isDuplicate) ಪ್ರತ್ಯಾಗಮ;

        AstConstraint* ಧ್ರುವ cloneConstrp = constrp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::vector<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
            ಯದಿ (AstVarRef* ಧ್ರುವ varRefp = VN_CAST(exprp, VarRef)) {
                ಯದಿ (!varRefp->varp()->isClassMember()) ಪ್ರತ್ಯಾಗಮ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ chainp = buildMemberSelChain(ಬೇರು_Var_Refp, newPath);
                AstMemberSel* ಧ್ರುವ finalSelp
                    = ನೂತನ AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(varRefp), varRefp);
            } ಅನ್ಯಥಾ ಯದಿ (AstFuncRef* ಧ್ರುವ funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                ಯದಿ (funcRefp->taskp()->classMethod() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.ತಳ್ಳು_back(funcRefp);
                }
            }
        });
        ಪ್ರತಿ (AstFuncRef* funcRefp : funcRefps) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ chainp = buildMemberSelChain(ಬೇರು_Var_Refp, newPath);
            AstArg* ಧ್ರುವ argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* ಧ್ರುವ callp
                = ನೂತನ AstMethodCall{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        targetClassp->addStmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    ಶೂನ್ಯ cloneNestedConstraintsRecurse(AstVarRef* ಬೇರು_Var_Refp, AstClass* classp,
                                       ಧ್ರುವ std::vector<AstVar*>& pathToClass,
                                       AstClass* targetClassp) {
        ಪ್ರತಿ (Ast_ಗಂಟು* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ಧ್ರುವ memberVarp = VN_CAST(memberNodep, Var);
            ಯದಿ (!memberVarp) ಅನುವರ್ತ;
            ಯದಿ (!memberVarp->rand().isRandomizable()) ಅನುವರ್ತ;
            ಧ್ರುವ AstClassRefDType* ಧ್ರುವ memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            ಯದಿ (!memberClassRefp || !memberClassRefp->classp()) ಅನುವರ್ತ;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::vector<AstVar*> newPath = pathToClass;
            newPath.ತಳ್ಳು_back(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ಧ್ರುವ containingClassp, AstConstraint* ಧ್ರುವ constrp) {
                    processNestedConstraint(constrp, ಬೇರು_Var_Refp, newPath, targetClassp);
                });
            cloneNestedConstraintsRecurse(ಬೇರು_Var_Refp, nestedClassp, newPath, targetClassp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    ಶೂನ್ಯ handleRandomizeArgument(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar* ಧ್ರುವ varp, ಧ್ರುವ ತಾರ್ಕಿಕ stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        ಯಾವತ್ (exprp) {
            AstVar* randVarp = nullptr;
            ಯದಿ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } ಅನ್ಯಥಾ ಯದಿ (AstArraySel* ಧ್ರುವ arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                ಅನುವರ್ತ;  // Skip ArraySel, continue traversing
            } ಅನ್ಯಥಾ ಯದಿ (AstStructSel* ಧ್ರುವ structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                ಅನುವರ್ತ;  // Skip StructSel, continue traversing
            } ಅನ್ಯಥಾ ಯದಿ (AstCMethodHard* ಧ್ರುವ methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                ಅನುವರ್ತ;
            } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(ಸತ್ಯ);
                varrefp->access(VAccess::READWRITE);
                exprp = nullptr;
            } ಅನ್ಯಥಾ {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            ಯದಿ (randVarp == varp) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು* backp = randVarp;
            ಯಾವತ್ (backp && !VN_IS(backp, ಗಂಟು_Module)) backp = backp->backp();
            ಯದಿ (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } ಅನ್ಯಥಾ {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                RandomizeMode randMode = {};
                randMode.usesMode = ಸತ್ಯ;
                randVarp->user1(randMode.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        ಯದಿ (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ಧ್ರುವ AstClass* ಧ್ರುವ basep = nodep->extendsp()->classp();
            m_baseToDerivedMap[basep].ಸೇರಿಸು(nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        ಯದಿ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        ಯದಿ (nodep->name() == "rand_mode") {
            AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = methodCallp ? methodCallp->fromp() : nullptr;
            ತಾರ್ಕಿಕ valid = ಸತ್ಯ;
            ಯದಿ (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = ಅಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = ಅಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = ಅಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (AstCMethodHard* ಧ್ರುವ methodHardp = VN_CAST(fromp, CMethodHard)) {
                ಯದಿ (methodHardp->method() == VCMethod::ARRAY_AT
                    || methodHardp->method() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = ಅಸತ್ಯ;
                } ಅನ್ಯಥಾ {
                    methodHardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            ಯದಿ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = ಅಸತ್ಯ;
            }
            ಯದಿ (valid) {
                ಧ್ರುವ RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_classp);
                ಯದಿ ((!randModeTarget.receiverp || !randModeTarget.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = ಅಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (!randModeTarget.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = ಅಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = ಅಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (randModeTarget.receiverp && randModeTarget.receiverp->isRand()) {
                    // Called on a rand member variable
                    RandomizeMode randMode = {};
                    randMode.usesMode = ಸತ್ಯ;
                    randModeTarget.receiverp->user1(randMode.asUQuad);
                } ಅನ್ಯಥಾ {
                    // Called on 'this' or a non-rand class instance
                    randModeTarget.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        ಯದಿ (!varp->isRand()) ಪ್ರತ್ಯಾಗಮ;
                        RandomizeMode randMode = {};
                        randMode.usesMode = ಸತ್ಯ;
                        varp->user1(randMode.asUQuad);
                    });
                }
            }
            ಯದಿ (!valid) {
                ಯದಿ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                    nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                } ಅನ್ಯಥಾ {
                    m_stmtp->unlinkFrBack();
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (nodep->name() == "constraint_mode") {
            ತಾರ್ಕಿಕ valid = ಸತ್ಯ;
            ಯದಿ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = ಅಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)
                       && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = ಅಸತ್ಯ;
            }
            AstConstraint* constrp = nullptr;
            AstClass* classp = m_classp;
            ಯದಿ (ಧ್ರುವ AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(nodep, MethodCall)) {
                ಯದಿ (ಧ್ರುವ AstConstraintRef* ಧ್ರುವ constrRefp
                    = VN_CAST(methodCallp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    ಯದಿ (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } ಅನ್ಯಥಾ ಯದಿ (AstClassRefDType* classRefDtp
                           = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } ಅನ್ಯಥಾ {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = ಅಸತ್ಯ;
                }
            }
            ಯದಿ (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = ಅಸತ್ಯ;
            }
            ಯದಿ (valid) {
                RandomizeMode constraintMode = {};
                constraintMode.usesMode = ಸತ್ಯ;
                ಯದಿ (constrp) {
                    constrp->user1(constraintMode.asUQuad);
                } ಅನ್ಯಥಾ {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraintMode.asUQuad);
                    });
                }
            } ಅನ್ಯಥಾ {
                ಯದಿ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                    nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                } ಅನ್ಯಥಾ {
                    m_stmtp->unlinkFrBack();
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            ಯದಿ (m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp || m_inStdWith) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                ಪ್ರತಿ (Ast_ಗಂಟು* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    ಯದಿ (argp->user1()) {
                        nodep->user1(ಸತ್ಯ);
                        ಭಙ್ಗ;
                    }
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstClass* classp = m_classp;
        ಯದಿ (ಧ್ರುವ AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(nodep, MethodCall)) {
            ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp
                = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        ಯದಿ (classp) {
            ಯದಿ (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ಧ್ರುವ, AstVar* ಧ್ರುವ memberVarp) {
                ಯದಿ (!memberVarp->rand().isRandomizable()) ಪ್ರತ್ಯಾಗಮ;
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = memberVarp->dtypep()->skipRefp();
                ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(dtypep, ClassRefDType);
                ಯದಿ (!classRefp || !classRefp->classp()) ಪ್ರತ್ಯಾಗಮ;
                AstClass* ಧ್ರುವ memberClassp = classRefp->classp();
                ಯದಿ (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) ಪ್ರತ್ಯಾಗಮ;
                memberVarp->globalConstrained(ಸತ್ಯ);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* ಬೇರು_Var_Refp
                    = ನೂತನ AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::vector<AstVar*> emptyPath;
                memberClassp->foreachMember([&](AstClass* ಧ್ರುವ, AstConstraint* ಧ್ರುವ constrp) {
                    processNestedConstraint(constrp, ಬೇರು_Var_Refp, emptyPath, classp);
                });
                cloneNestedConstraintsRecurse(ಬೇರು_Var_Refp, memberClassp, emptyPath, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(ಬೇರು_Var_Refp->delete_ವೃಕ್ಷ(), ಬೇರು_Var_Refp);
            });
        }
        ಯದಿ (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = nullptr;
            ಪ್ರತಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handleRandomizeArgument(argp->exprp(), nullptr, ಸತ್ಯ);
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಪ್ರತಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->ಅಂಕೆ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ಧ್ರುವ ತಾರ್ಕಿಕ hasUnsupportedMember
                    = classp->existsMember([](ಧ್ರುವ AstClass*, ಧ್ರುವ AstVar* memberVarp) {
                          ಯದಿ (!memberVarp->rand().isRandomizable()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                          ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = memberVarp->dtypep()->skipRefp();
                          ಪ್ರತ್ಯಾಗಮ VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, QueueDType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                ಯದಿ (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                ಅನುವರ್ತ;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = nullptr;  // If nodep is a method call, this is its receiver
            ಯದಿ (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                ಯದಿ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(methodCallp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } ಅನ್ಯಥಾ {
                    AstVarRef* ಧ್ರುವ varrefp = VN_AS(methodCallp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handleRandomizeArgument(argp->exprp(), fromVarp, ಅಸತ್ಯ);
        }
        // Re-mark members after the class became inline-randomized above.
        ಯದಿ (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    ಶೂನ್ಯ visit(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp);
        m_stmtp = nodep;
        m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp = nodep;
        iterateChildren(nodep);
        ಯದಿ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        VL_RESTORER(m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp);
        m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp);
            m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterateAndNextConstNull(nodep->thensp());
        iterateAndNextConstNull(nodep->elsesp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (!m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        ಯದಿ (nodep->varp()->rand().isRandomizable() && !(m_inStdWith && m_stdRandCallp)) {
            nodep->user1(ಸತ್ಯ);
        } ಅನ್ಯಥಾ ಯದಿ (isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(ಸತ್ಯ);
        }
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        ಯದಿ (!m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp) ಪ್ರತ್ಯಾಗಮ;
        iterateChildrenConst(nodep);
        ಧ್ರುವ ತಾರ್ಕಿಕ randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ಧ್ರುವ ತಾರ್ಕಿಕ randMember = nodep->varp()->rand().isRandomizable();
        ಧ್ರುವ ತಾರ್ಕಿಕ inStdWith = m_inStdWith && m_stdRandCallp;
        ಯದಿ (randObject && randMember && !inStdWith) {
            nodep->user1(ಸತ್ಯ);
        } ಅನ್ಯಥಾ ಯದಿ (inStdWith && isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(ಸತ್ಯ);
            // Mark parent object for constraint expression visitor
            ಯದಿ (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(ಸತ್ಯ);
        }

        ಯದಿ (m_withp) {
            Ast_ಗಂಟು* backp = m_withp;
            ಯಾವತ್ (backp->backp()) {
                ಯದಿ (ಧ್ರುವ AstMethodCall* ಧ್ರುವ callp = VN_CAST(backp, MethodCall)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    ಭಙ್ಗ;
                }
                backp = backp->backp();
            }
        } ಅನ್ಯಥಾ {
            nodep->user2p(m_modp);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        nodep->user2p(m_modp);
        ಯದಿ (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        ಯದಿ (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_inStdWith = ಸತ್ಯ;
        iterateChildrenConst(nodep);
        m_inStdWith = ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        iterateChildrenConst(nodep);
        ಯದಿ (!m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp && !m_inStdWith) ಪ್ರತ್ಯಾಗಮ;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    ಶೂನ್ಯ visit(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        ಯದಿ (!m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp && !m_inStdWith) ಪ್ರತ್ಯಾಗಮ;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit RandomizeMarkVisitor(Ast_ಗಂಟು* nodep) {
        iterateConst(nodep);
        markAllDerived();
        setPackageRefs();
    }
    ~RandomizeMarkVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

ವರ್ಗ Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ಧ್ರುವ m_classp;
    Ast_ಗಂಟು_FTask* ಧ್ರುವ m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_ಗಂಟು_FTask* ಧ್ರುವ m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ಧ್ರುವ m_genp;  // VlRandomizer variable of the class
    AstVar* m_randModeVarp;  // Relevant randmode state variable
    ತಾರ್ಕಿಕ m_wantSingle = ಅಸತ್ಯ;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    ತಾರ್ಕಿಕ m_structSel = ಅಸತ್ಯ;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    ತಾರ್ಕಿಕ m_deferRandModeHoist = ಅಸತ್ಯ;  // Wait for the complete struct member path
    std::set<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::set<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertionSet<AstVar*>* m_sizeConstrainedArraysp = nullptr;  // Arrays with size constraints
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_conditionp = nullptr;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_ಗಂಟು_FTask* m_prepareConstrainedArraysp = nullptr;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = nullptr;  // Current ID of unique call
    V3UniqueNames& m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    std::vector<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_ಗಂಟು* m_firstExpressionInsideIndexp = nullptr;
    AstConstraintForeach* m_foreachp = nullptr;  // Innermost constraint-foreach being processed
    std::set<ಧ್ರುವ AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    ವರ್ಗ NestedAccessPath final {
        AstMemberSel* m_ತುದಿ_Nested_Array_Member_Selp
            = nullptr;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = nullptr;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = nullptr;  // Last node of variable's name format
        Ast_ಗಂಟು** m_firstExpressionInsideIndexPointerp
            = nullptr;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    ಸಾರ್ವಜನಿಕ:
        ಶೂನ್ಯ addVarNamePart(AstSFormatF* ಧ್ರುವ nodep, std::string&& name) {
            ಯದಿ (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } ಅನ್ಯಥಾ {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.ಸೇರಿಸು(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            ಯದಿ (name == "") {
                ಯದಿ (*m_firstExpressionInsideIndexPointerp) {
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

        AstMemberSel* access_ವೃಕ್ಷ() { ಪ್ರತ್ಯಾಗಮ m_ತುದಿ_Nested_Array_Member_Selp; }
        ಧ್ರುವ std::string& smtName() { ಪ್ರತ್ಯಾಗಮ m_smtName; }

        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ಧ್ರುವ methodp = ನೂತನ AstCMethodHard{
                varp->fileline(),
                ನೂತನ AstVarRef{varp->fileline(), VN_AS(genp->user2p(), ಗಂಟು_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtypeSetVoid();

            methodp->addPinsp(m_ತುದಿ_Nested_Array_Member_Selp);  // Variable

            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep();  // Width
            ಧ್ರುವ size_t width = m_ತುದಿ_Nested_Array_Member_Selp->varp()->dtypep()->width();
            methodp->addPinsp(ನೂತನ AstConst{dtypep->fileline(), AstConst::Unsized64{}, width});

            methodp->addPinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = nullptr;

            methodp->addPinsp(
                ನೂತನ AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_ತುದಿ_Nested_Array_Member_Selp = nullptr;
            ಪ್ರತ್ಯಾಗಮ methodp->makeStmt();
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cloneName() { ಪ್ರತ್ಯಾಗಮ m_nestedNameFormatTopp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ); }

        ಶೂನ್ಯ error() {
            VL_DO_DANGLING(m_ತುದಿ_Nested_Array_Member_Selp->delete_ವೃಕ್ಷ(), m_ತುದಿ_Nested_Array_Member_Selp);
        }

        NestedAccessPath(AstMemberSel* ತುದಿ_Member_Selp, Ast_ಗಂಟು** pointerToFirstExprp)
            : m_ತುದಿ_Nested_Array_Member_Selp{ತುದಿ_Member_Selp}
            , m_firstExpressionInsideIndexPointerp{pointerToFirstExprp} {}
        ~NestedAccessPath() {
            ಯದಿ (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_ವೃಕ್ಷ(),
                            m_nestedNameFormatTopp = nullptr);
                m_nestedNameFormatp = nullptr;
            }
        }
    };
    NestedAccessPath* m_nestedAccess = nullptr;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createEnumConstraint(File_ಸಾಲು* ಧ್ರುವ fl, ಧ್ರುವ std::string& smtName,
                                      AstEnumDType* ಧ್ರುವ enumDtp, AstVar* ಧ್ರುವ genVarp) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ genModp = VN_AS(genVarp->user2p(), ಗಂಟು_Module);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = enumDtp->width();
        std::string constraint = "(__Vbv (or";
        ಪ್ರತಿ (AstEnumItem* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ಧ್ರುವ AstConst* ಧ್ರುವ vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvtToStr(vconstp->toUInt()) + " "
                          + cvtToStr(width) + "))";
        }
        constraint += "))";
        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
            fl, ನೂತನ AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "\"" + constraint + "\""}};
        callp->dtypeSetVoid();
        ಪ್ರತ್ಯಾಗಮ callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* findStaticRandModeVarMember(AstClass* classp) ಧ್ರುವ {
        ಯಾವತ್ (ಸತ್ಯ) {
            ಯದಿ (AstVar* ಧ್ರುವ varp
                = VN_CAST(m_memberMap.findMember(classp, "__Vstaticrandmode"), Var)) {
                ಪ್ರತ್ಯಾಗಮ varp;
            }
            AstClassExtends* ಧ್ರುವ extendsp = classp->extendsp();
            ಯದಿ (!extendsp) ಪ್ರತ್ಯಾಗಮ nullptr;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    ಶೂನ್ಯ markConstrainedRandMember(AstStructDType* ಧ್ರುವ structp, ಧ್ರುವ std::string& name) {
        AstMemberDType* ಧ್ರುವ memberp
            = VN_CAST(m_memberMap.findMember(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(ಸತ್ಯ);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    ತಾರ್ಕಿಕ isRandQualifiedPath(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯಾವತ್ (nodep) {
            ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ selp = VN_CAST(nodep, StructSel)) {
                ಯದಿ (AstStructDType* ಧ್ರುವ structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ಧ್ರುವ AstMemberDType* ಧ್ರುವ memberp
                        = VN_CAST(m_memberMap.findMember(structp, selp->name()), MemberDType);
                    ಯದಿ (memberp && !memberp->rand().isRandomizable()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                nodep = selp->fromp();
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCMethodHard* ಧ್ರುವ methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } ಅನ್ಯಥಾ {
                ಭಙ್ಗ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    ಸ್ಥಿರ ಶೂನ್ಯ markStructConstrainedRandRecurse(Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        AstStructDType* ಧ್ರುವ structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        ಯದಿ (!structp || structp->packed()) ಪ್ರತ್ಯಾಗಮ;
        structp->markConstrainedRand(ಸತ್ಯ);
        ಪ್ರತಿ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            ಯದಿ (!memberp->rand().isRandomizable()) ಅನುವರ್ತ;
            memberp->markConstrainedRand(ಸತ್ಯ);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string buildMemberPath(ಧ್ರುವ AstMemberSel* ಧ್ರುವ memberSelp) {
        ಧ್ರುವ Ast_ಗಂಟು* fromp = memberSelp->fromp();
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            ಪ್ರತ್ಯಾಗಮ refp->name() + "." + memberSelp->name();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            ಪ್ರತ್ಯಾಗಮ buildMemberPath(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        ಪ್ರತ್ಯಾಗಮ "";
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getFromp(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep) {
        ಯದಿ (ಧ್ರುವ AstCMethodHard* ಧ್ರುವ cmethp = VN_CAST(nodep, CMethodHard)) {
            ಪ್ರತ್ಯಾಗಮ cmethp->fromp();
        }
        ಪ್ರತ್ಯಾಗಮ getSelFromp(nodep);
    }

    ಸ್ಥಿರ std::string extractSolveBeforeBaseName(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಯಾವತ್ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = getFromp(exprp)) exprp = fromp;
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ vrefp = VN_CAST(exprp, VarRef)) ಪ್ರತ್ಯಾಗಮ vrefp->name();
        ಪ್ರತ್ಯಾಗಮ "";
    }

    ಸ್ಥಿರ ಶೂನ್ಯ buildNamePrefix(AstCExpr* exprp, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep) {
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) {
            exprp->add("(\""s + refp->name());
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = getFromp(nodep);
        ಯದಿ (fromp) buildNamePrefix(exprp, fromp);

        ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ selp = VN_CAST(nodep, StructSel)) {
            exprp->add("." + selp->name());
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstCMethodHard* ಧ್ರುವ cmethp = VN_CAST(nodep, CMethodHard)) {
            ಯದಿ (cmethp->method() == VCMethod::ARRAY_AT) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = cmethp->pinsp();
                exprp->add(".\" + ");
                AstCFuncHard* ಧ್ರುವ solverHexFuncp
                    = ನೂತನ AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(argp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                solverHexFuncp->dtypeSetString();
                exprp->add(solverHexFuncp);
                exprp->add(" + \"");
            } ಅನ್ಯಥಾ {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ಸ್ಥಿರ AstCExpr* build_Path_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* ಧ್ರುವ fl, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep,
                                   ಧ್ರುವ std::string selName) {
        AstCExpr* ಧ್ರುವ exprp = ನೂತನ AstCExpr{fl, ""};
        buildNamePrefix(exprp, nodep);
        ಯದಿ (selName != "") {
            exprp->add(".");
            exprp->add(selName);
        }
        exprp->add("\")");
        ಪ್ರತ್ಯಾಗಮ exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, ಧ್ರುವ std::string& baseName,
                                    ಧ್ರುವ Ast_ಗಂಟು_Sel* selp) {
        AstCExpr* ಧ್ರುವ p = ನೂತನ AstCExpr{fl, ""};
        p->add("(\""s + baseName + ".\"");
        p->add("+");
        AstCFuncHard* ಧ್ರುವ solverHexFuncp
            = ನೂತನ AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
        solverHexFuncp->addPinsp(selp->bitp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
        solverHexFuncp->dtypeSetString();
        p->add(solverHexFuncp);
        p->add(")");
        p->dtypeSetString();
        ಪ್ರತ್ಯಾಗಮ p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getSelFromp(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ mp = VN_CAST(exprp, MemberSel)) ಪ್ರತ್ಯಾಗಮ mp->fromp();
        ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ sp = VN_CAST(exprp, StructSel)) ಪ್ರತ್ಯಾಗಮ sp->fromp();
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ಧ್ರುವ p = ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "\"" + varrefp->name() + "\"s"};
            p->dtypeSetString();
            ಪ್ರತ್ಯಾಗಮ p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ selFromp = getSelFromp(exprp)) {
            ಧ್ರುವ std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string baseName;
                ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    baseName = vp->name();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    baseName = buildMemberPath(mp);
                }
                ಯದಿ (baseName.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;
                AstCExpr* ಧ್ರುವ p = ನೂತನ AstCExpr{fl, ""};
                p->add("(\""s + baseName + ".\"");
                p->add("+");
                AstCFuncHard* ಧ್ರುವ solverHexFuncp
                    = ನೂತನ AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(arrSelp->bitp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                solverHexFuncp->dtypeSetString();
                p->add(solverHexFuncp);
                p->add(")");
                p->dtypeSetString();
                ಪ್ರತ್ಯಾಗಮ p;
            }
            // Static member path (obj.field)
            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ vp = VN_CAST(selFromp, VarRef)) {
                ಧ್ರುವ std::string path = vp->name() + "." + selName;
                AstCExpr* ಧ್ರುವ p = ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "\"" + path + "\"s"};
                p->dtypeSetString();
                ಪ್ರತ್ಯಾಗಮ p;
            }
            ಯದಿ (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ಧ್ರುವ p = build_Path_ಅಭಿವ್ಯಕ್ತಿ(fl, selFromp, selName);
                p->dtypeSetString();
                ಪ್ರತ್ಯಾಗಮ p;
            }
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಯದಿ (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ಧ್ರುವ p = build_Path_ಅಭಿವ್ಯಕ್ತಿ(fl, exprp, "");
            p->dtypeSetString();
            ಪ್ರತ್ಯಾಗಮ p;
        }
        ಧ್ರುವ Ast_ಗಂಟು_Sel* selp = VN_CAST(exprp, ArraySel);
        ಯದಿ (!selp) selp = VN_CAST(exprp, AssocSel);
        ಯದಿ (selp) {
            // arr[i] -> dynamic name
            std::string baseName;
            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ vp = VN_CAST(selp->fromp(), VarRef)) {
                baseName = vp->name();
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ mp = VN_CAST(selp->fromp(), MemberSel)) {
                baseName = buildMemberPath(mp);
            }
            ಯದಿ (baseName.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;
            ಪ್ರತ್ಯಾಗಮ build_Array_Sel_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, baseName, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ bitSelp = VN_CAST(exprp, Sel)) {
            ಪ್ರತ್ಯಾಗಮ build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, bitSelp->fromp());
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    AstSFormatF* getConstFormat(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSFormatF{nodep->fileline(), (nodep->width() & 3) ? "#b%b" : "#x%x", ಅಸತ್ಯ,
                               nodep};
    }
    ತಾರ್ಕಿಕ editFormat(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (nodep->user1()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        handle.relink(getConstFormat(nodep));
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wrapWithCond(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp,
                              ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width) {
        ಯದಿ (condp) {
            File_ಸಾಲು* ಧ್ರುವ flp = exprp->fileline();
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCond{
                flp, condp, exprp,
                getConstFormat(ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, width, 0})};
        }
        ಪ್ರತ್ಯಾಗಮ exprp;
    }
    AstSFormatF* editSMT(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nullptr,
                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nullptr, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp = nullptr) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_ಅಭಿವ್ಯಕ್ತಿ = nodep->emitSMT();  // Might need child width (AstExtend)
        ಯದಿ (smt_ಅಭಿವ್ಯಕ್ತಿ == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }

        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhsWidth = lhsp ? lhsp->width() : 0;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rhsWidth = rhsp ? rhsp->width() : 0;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ thsWidth = thsp ? thsp->width() : 0;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsCondp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsCondp = nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsCondp = nullptr;

        ಯದಿ (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlinkFrBack() : lhsp),
                         ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            lhsCondp = m_conditionp;
        }
        ಯದಿ (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlinkFrBack() : rhsp),
                         ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            rhsCondp = m_conditionp;
        }
        ಯದಿ (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlinkFrBack() : thsp),
                         ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            thsCondp = m_conditionp;
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr;
        ಪ್ರತಿ (string::iterator pos = smt_ಅಭಿವ್ಯಕ್ತಿ.begin(); pos != smt_ಅಭಿವ್ಯಕ್ತಿ.end(); ++pos) {
            ಯದಿ (pos[0] == '%') {
                ++pos;
                ವಿಕಲ್ಪ (pos[0]) {
                ಪಕ್ಷ '%': ಭಙ್ಗ;
                ಪಕ್ಷ 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ಗಂಟು::addNext(argsp, wrapWithCond(lhsp, lhsCondp, lhsWidth));
                    lhsp = nullptr;
                    ಭಙ್ಗ;
                ಪಕ್ಷ 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ಗಂಟು::addNext(argsp, wrapWithCond(rhsp, rhsCondp, rhsWidth));
                    rhsp = nullptr;
                    ಭಙ್ಗ;
                ಪಕ್ಷ 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ಗಂಟು::addNext(argsp, wrapWithCond(thsp, thsCondp, thsWidth));
                    thsp = nullptr;
                    ಭಙ್ಗ;
                ಸ್ವಭಾವ: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); ಭಙ್ಗ;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ಧ್ರುವ newp = ನೂತನ AstSFormatF{nodep->fileline(), smt_ಅಭಿವ್ಯಕ್ತಿ, ಅಸತ್ಯ, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* editSingle(File_ಸಾಲು* fl, Ast_ಗಂಟು* itemsp) {
        ಯದಿ (!itemsp) ಪ್ರತ್ಯಾಗಮ nullptr;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = ಸತ್ಯ;

        {
            AstBegin* ಧ್ರುವ tempp
                = ನೂತನ AstBegin{fl, "[EditWrapper]", itemsp->unlinkFrBackWithNext(), ಅಸತ್ಯ};
            VL_DO_DANGLING(iterateAndNextNull(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            ಯದಿ (itemsp) itemsp->unlinkFrBackWithNext();
            VL_DO_DANGLING(tempp->delete_ವೃಕ್ಷ(), tempp);
        }
        ಯದಿ (!itemsp) ಪ್ರತ್ಯಾಗಮ nullptr;

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprsp = VN_CAST(itemsp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        ಯದಿ (!exprsp->nextp()) ಪ್ರತ್ಯಾಗಮ exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSFormatF{fl, fmt.str(), ಅಸತ್ಯ, exprsp};
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arrayp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_ಗಂಟು_DType* ಧ್ರುವ arrDtp = arrayp->unlinkFrBack()->dtypep()->skipRefp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selp = nullptr;
        ಯದಿ (VN_IS(arrDtp, QueueDType) || VN_IS(arrDtp, DynArrayDType))
            selp = ನೂತನ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        ಅನ್ಯಥಾ ಯದಿ (VN_IS(arrDtp, UnpackArrayDType))
            selp = ನೂತನ AstArraySel{fl, arrayp, idxp};
        ಅನ್ಯಥಾ ಯದಿ (VN_IS(arrDtp, AssocArrayDType))
            selp = ನೂತನ AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        ಪ್ರತ್ಯಾಗಮ selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* adjustWidth(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಪೂರ್ಣಾಙ್ಕ targetWidth,
                             VSigning targetSigning) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಭಿವ್ಯಕ್ತಿ_Width = exprp->width();
        ಯದಿ (targetWidth > ಅಭಿವ್ಯಕ್ತಿ_Width) {
            // Extend to match target width
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ result = ನೂತನ AstExtend{fl, exprp, targetWidth};
            result->dtypeChgSigned(targetSigning);
            ಪ್ರತ್ಯಾಗಮ result;
        } ಅನ್ಯಥಾ ಯದಿ (targetWidth < ಅಭಿವ್ಯಕ್ತಿ_Width) {
            // Truncate to match target width
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ result = ನೂತನ AstSel{fl, exprp, 0, targetWidth};
            result->dtypeChgSigned(targetSigning);
            ಪ್ರತ್ಯಾಗಮ result;
        } ಅನ್ಯಥಾ {
            // Width already matches
            ಪ್ರತ್ಯಾಗಮ exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* extract_Return_ಅಭಿವ್ಯಕ್ತಿ(AstFunc* funcp) {
        AstVar* ಧ್ರುವ retVarp = VN_CAST(funcp->fvarp(), Var);
        ಯದಿ (!retVarp) ಪ್ರತ್ಯಾಗಮ nullptr;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* retExprp = nullptr;
        ಪೂರ್ಣಾಙ್ಕ assign_ಎಣಿಕೆ = 0;
        funcp->foreach([&](AstAssign* assignp) {
            AstVarRef* ಧ್ರುವ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ಯದಿ (!lhsp || lhsp->varp() != retVarp) ಪ್ರತ್ಯಾಗಮ;
            retExprp = assignp->rhsp();
            ++assign_ಎಣಿಕೆ;
        });
        // Simple function: initializer + return (2) or just return (1)
        ಯದಿ (assign_ಎಣಿಕೆ > 2 || !retExprp) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    ಶೂನ್ಯ propagateUser1InlineRecurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (VN_IS(nodep, ಗಂಟು_Var_Ref)) {
            nodep->user1(VN_AS(nodep, ಗಂಟು_Var_Ref)->varp()->rand().isRandomizable());
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (VN_IS(nodep, Const)) {
            nodep->user1(ಅಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಪೂರ್ಣಾಙ್ಕ anyChild = ಅಸತ್ಯ;  // Used as bool
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp = VN_CAST(nodep->op1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp = VN_CAST(nodep->op2p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp = VN_CAST(nodep->op3p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cp = VN_CAST(nodep->op4p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        nodep->user1(anyChild);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ಸ್ಥಿರ Ast_ಗಂಟು* prependDistPreamble(AstConstraintForeach* nodep, Ast_ಗಂಟು* bodyp) {
        Ast_ಗಂಟು* ಧ್ರುವ preamblep = nodep->user3p();
        ಯದಿ (!preamblep) ಪ್ರತ್ಯಾಗಮ bodyp;
        nodep->user3p(nullptr);
        preamblep->addNext(bodyp);
        ಪ್ರತ್ಯಾಗಮ preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    ತಾರ್ಕಿಕ referencesForeachIdx(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ {
        ಯದಿ (m_foreachIdxVars.empty()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ nodep->exists(
            [&](ಧ್ರುವ AstVarRef* refp) { ಪ್ರತ್ಯಾಗಮ m_foreachIdxVars.ಎಣಿಕೆ(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    ಶೂನ್ಯ addRegistrationStmt(Ast_ಗಂಟು* ಧ್ರುವ stmtp, Ast_ಗಂಟು_FTask* ಧ್ರುವ initTaskp) {
        ಯದಿ (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->addBodyp(stmtp);
        } ಅನ್ಯಥಾ {
            initTaskp->addStmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_ಸಾಲು* ಧ್ರುವ fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ arrExprp,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ idxExprp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argsp = Ast_ಗಂಟು::addNext(arrExprp, idxExprp);
        AstSFormatF* ಧ್ರುವ formatp = ನೂತನ AstSFormatF{fl, "(select %s %s)", ಅಸತ್ಯ, argsp};
        ಪ್ರತ್ಯಾಗಮ formatp;
    }

    ಶೂನ್ಯ setRandMode(AstVar* ಧ್ರುವ varp, AstMemberSel* ಧ್ರುವ memberselp,
                     ಧ್ರುವ std::string& smtName, ಧ್ರುವ RandomizeMode& randMode,
                     Ast_ಗಂಟು_FTask* ಧ್ರುವ initTaskp) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ varClassp = VN_AS(varp->user2p(), ಗಂಟು_Module);
        AstVar* ಧ್ರುವ subRandModeVarp = getRandModeVarFromClass(varClassp);
        ಯದಿ (subRandModeVarp) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ parentAccessp = memberselp->fromp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            AstMemberSel* ಧ್ರುವ randModeSelp
                = ನೂತನ AstMemberSel{varp->fileline(), parentAccessp, subRandModeVarp};
            randModeSelp->dtypep(subRandModeVarp->dtypep());
            AstCMethodHard* ಧ್ರುವ atp
                = ನೂತನ AstCMethodHard{varp->fileline(), randModeSelp, VCMethod::ARRAY_AT,
                                     ನೂತನ AstConst{varp->fileline(), randMode.index}};
            atp->dtypeSetUInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ಧ್ರುವ enablep = ನೂತನ AstCMethodHard{
                varp->fileline(),
                ನೂತನ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtypeSetVoid();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : ನೂತನ AstSFormatF{varp->fileline(), smtName, ಅಸತ್ಯ, nullptr};
            nameExprp->dtypep(varp->dtypep());
            enablep->addPinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ಧ್ರುವ disablep = ನೂತನ AstCMethodHard{
                varp->fileline(),
                ನೂತನ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_SET_VAR_DISABLED};
            disablep->dtypeSetVoid();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ disnp = nameExprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            disablep->addPinsp(disnp);
            AstIf* ಧ್ರುವ ifp
                = ನೂತನ AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            addRegistrationStmt(ifp, initTaskp);
        }
    }

    ಶೂನ್ಯ markRandc(AstVar* ಧ್ರುವ varp, ಧ್ರುವ std::string& smtName, Ast_ಗಂಟು_FTask* ಧ್ರುವ initTaskp) {
        AstCMethodHard* ಧ್ರುವ markp = ನೂತನ AstCMethodHard{
            varp->fileline(),
            ನೂತನ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtypeSetVoid();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : ನೂತನ AstSFormatF{varp->fileline(), smtName, ಅಸತ್ಯ, nullptr};
        nameExprp->dtypep(varp->dtypep());
        markp->addPinsp(nameExprp);
        addRegistrationStmt(markp->makeStmt(), initTaskp);
    }

    Ast_ಗಂಟು_Module* getLeftmostVarModulep(AstMemberSel* ಧ್ರುವ memberselp,
                                         AstVar* ಧ್ರುವ varp) ಧ್ರುವ {
        ಯದಿ (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_ಗಂಟು* ಎಡ_Most_Nodep = memberselp->fromp();
            ಯಾವತ್ (AstMemberSel* ಧ್ರುವ nestedMemberSelp = VN_CAST(ಎಡ_Most_Nodep, MemberSel)) {
                ಎಡ_Most_Nodep = nestedMemberSelp->fromp();
            }
            ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ ಬೇರು_Var_Refp = VN_CAST(ಎಡ_Most_Nodep, ಗಂಟು_Var_Ref)) {
                ಪ್ರತ್ಯಾಗಮ VN_AS(ಬೇರು_Var_Refp->varp()->user2p(), ಗಂಟು_Module);
            }
            ಪ್ರತ್ಯಾಗಮ VN_AS(memberselp->user2p(), ಗಂಟು_Module);
        }
        ಪ್ರತ್ಯಾಗಮ VN_AS(varp->user2p(), ಗಂಟು_Module);
    }

    Ast_ಗಂಟು_FTask* getInitTaskp(AstVar* ಧ್ರುವ varp, ಧ್ರುವ ತಾರ್ಕಿಕ memberselOrStructSel,
                               Ast_ಗಂಟು_Module* ಧ್ರುವ classp) ಧ್ರುವ {
        ಯದಿ (m_inlineInitTaskp) ಪ್ರತ್ಯಾಗಮ m_inlineInitTaskp;
        varp->user3(ಸತ್ಯ);
        ಯದಿ (memberselOrStructSel) {
            ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ initTaskp
                = VN_AS(m_memberMap.findMember(classp, "randomize"), ಗಂಟು_FTask)) {
                ಪ್ರತ್ಯಾಗಮ initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            ಯದಿ (m_memberselInitTaskp) ಪ್ರತ್ಯಾಗಮ m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_FTask* ಧ್ರುವ initTaskp
                = VN_AS(m_memberMap.findMember(classp, "new"), ಗಂಟು_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            ಪ್ರತ್ಯಾಗಮ initTaskp;
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    ಶೂನ್ಯ createWriteVarForClassFields(AstClass* ಧ್ರುವ elemClassp, ಧ್ರುವ std::string& smtName,
                                      AstVar* ಧ್ರುವ iterVarp, ಧ್ರುವ ತಾರ್ಕಿಕ isUnpackedClassRefArray,
                                      Ast_ಗಂಟು_Module* ಧ್ರುವ classOrPackagep, AstVar* ಧ್ರುವ varp,
                                      AstIf* ಧ್ರುವ ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ಧ್ರುವ {
        File_ಸಾಲು* ಧ್ರುವ fl = varp->fileline();
        Ast_ಗಂಟು_Module* ಧ್ರುವ varClassp = VN_AS(varp->user2p(), ಗಂಟು_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr ಪೂರ್ಣಾಙ್ಕ idxWidth = 32;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ fmtWidth = VL_ಪದಗಳು_I(idxWidth) * 8;

        ಪ್ರತಿ (ಧ್ರುವ AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            ಪ್ರತಿ (Ast_ಗಂಟು* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ಧ್ರುವ memberVarp = VN_CAST(mnodep, Var);
                ಯದಿ (!memberVarp || !memberVarp->rand().isRandomizable()) ಅನುವರ್ತ;
                Ast_ಗಂಟು_DType* ಧ್ರುವ memberDtp = memberVarp->dtypep()->skipRefp();
                ಯದಿ (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) ಅನುವರ್ತ;
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ memberWidth = memberDtp->width();

                AstCStmt* ಧ್ರುವ fmtp = ನೂತನ AstCStmt{fl};
                ಯದಿ (isUnpackedClassRefArray) {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } ಅನ್ಯಥಾ {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::to_string(fmtWidth) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->add(ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->add(");\n");
                ifNonNullp->addThensp(fmtp);

                AstVarRef* ಧ್ರುವ arrayWrRefp = ನೂತನ AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* atWritep;
                ಯದಿ (isUnpackedClassRefArray) {
                    atWritep = ನೂತನ AstArraySel{fl, arrayWrRefp,
                                               ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}};
                } ಅನ್ಯಥಾ {
                    atWritep = ನೂತನ AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* ಧ್ರುವ memberSelp = ನೂತನ AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* ಧ್ರುವ writeVarp
                    = ನೂತನ AstCMethodHard{fl,
                                         ನೂತನ AstVarRef{fl, VN_AS(m_genp->user2p(), ಗಂಟು_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtypeSetVoid();
                writeVarp->addPinsp(memberSelp);
                writeVarp->addPinsp(
                    ನೂತನ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(memberWidth)});
                AstCExpr* ಧ್ರುವ nameRefp = ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->addPinsp(nameRefp);
                writeVarp->addPinsp(ನೂತನ AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->addThensp(writeVarp->makeStmt());
            }
        }
    }

    ಶೂನ್ಯ createSolverArrayHandle(AstVar* ಧ್ರುವ varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_ಗಂಟು_Module* ಧ್ರುವ classOrPackagep,
                                 ಧ್ರುವ ತಾರ್ಕಿಕ isUnpackedClassRefArray,
                                 ಧ್ರುವ std::string& smtName) ಧ್ರುವ {
        File_ಸಾಲು* ಧ್ರುವ fl = varp->fileline();
        AstClass* ಧ್ರುವ elemClassp = elemClassRefDtp->classp();
        Ast_ಗಂಟು_Module* ಧ್ರುವ varClassp = VN_AS(varp->user2p(), ಗಂಟು_Module);

        AstVar* ಧ್ರುವ iterVarp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__Vi"),
                                            varp->findUInt32DType()};
        iterVarp->funcLocal(ಸತ್ಯ);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ಗಂಟು* ಧ್ರುವ stmtsp
            = ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, iterVarp, VAccess::WRITE}, ನೂತನ AstConst{fl, 0}};

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizep;
        ಯದಿ (isUnpackedClassRefArray) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ arraySize
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = ನೂತನ AstConst{fl, static_cast<uint32_t>(arraySize)};
        } ಅನ್ಯಥಾ {
            AstVarRef* ಧ್ರುವ arraySizeRefp = ನೂತನ AstVarRef{fl, varClassp, varp, VAccess::READ};
            arraySizeRefp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ಧ್ರುವ dynSizep
                = ನೂತನ AstCMethodHard{fl, arraySizeRefp, VCMethod::DYN_SIZE, nullptr};
            dynSizep->dtypeSetUInt32();
            sizep = dynSizep;
        }

        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(ನೂತನ AstLoopTest{
            fl, loopp, ನೂತನ AstLt{fl, ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ಧ್ರುವ arrayReadRefp = ನೂತನ AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* atReadp;
        ಯದಿ (isUnpackedClassRefArray) {
            atReadp
                = ನೂತನ AstArraySel{fl, arrayReadRefp, ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}};
        } ಅನ್ಯಥಾ {
            atReadp = ನೂತನ AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ಧ್ರುವ ifNonNullp
            = ನೂತನ AstIf{fl, ನೂತನ AstNeq{fl, atReadp, ನೂತನ AstConst{fl, AstConst::Null{}}}, nullptr};
        loopp->addStmtsp(ifNonNullp);

        AstCStmt* ಧ್ರುವ bufDeclp = ನೂತನ AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->addThensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->addStmtsp(ನೂತನ AstAssign{
            fl, ನೂತನ AstVarRef{fl, iterVarp, VAccess::WRITE},
            ನೂತನ AstAdd{fl, ನೂತನ AstConst{fl, 1}, ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(ಸತ್ಯ);
        Ast_ಗಂಟು_FTask* initTaskp = m_inlineInitTaskp;
        ಯದಿ (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.findMember(varClassp, "new"), ಗಂಟು_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->addStmtsp(stmtsp);
    }

    ಶೂನ್ಯ createSolverVarHandle(AstVar* ಧ್ರುವ varp, ಧ್ರುವ ತಾರ್ಕಿಕ structSelOrCMeth,
                               ಧ್ರುವ ತಾರ್ಕಿಕ isGlobalConstrained, ಧ್ರುವ RandomizeMode randMode,
                               AstMemberSel* ಧ್ರುವ memberselp, ಧ್ರುವ std::string& smtName,
                               Ast_ಗಂಟು_Module* ಧ್ರುವ classOrPackagep, Ast_ಗಂಟು_Module* ಧ್ರುವ classp,
                               Ast_ಗಂಟು_FTask* ಧ್ರುವ initTaskp) {
        uint32_t unpackedDims = 0;
        ಯದಿ (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(ಅಸತ್ಯ).second;
        }
        ಯದಿ (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ಧ್ರುವ methodp = ನೂತನ AstCMethodHard{
            varp->fileline(),
            ನೂತನ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();
        ಯದಿ (memberselp) {
            methodp->addPinsp(memberselp);
        } ಅನ್ಯಥಾ {
            AstVarRef* ಧ್ರುವ varRefp
                = ನೂತನ AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->addPinsp(varRefp);
        }
        ಧ್ರುವ size_t width = arrayElementDTypep(varp->dtypep())->width();
        methodp->addPinsp(ನೂತನ AstConst{varp->fileline(), AstConst::Unsized64{}, width});
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ varnamep = ನೂತನ AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                   "\"" + smtName + "\"", varp->width()};
        varnamep->dtypep(varp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(
            ನೂತನ AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        ಯದಿ (randMode.usesMode && !(isGlobalConstrained && memberselp)) {
            methodp->addPinsp(
                ನೂತನ AstConst{varp->fileline(), AstConst::Unsized64{}, randMode.index});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->addStmtsp(methodp->makeStmt());
        ಯದಿ (varp->lifetime().isStatic() && randMode.usesMode) {
            AstCMethodHard* ಧ್ರುವ markp = ನೂತನ AstCMethodHard{
                varp->fileline(),
                ನೂತನ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ namep = ನೂತನ AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                    "\"" + smtName + "\"", varp->width()};
            namep->dtypep(varp->dtypep());
            markp->addPinsp(namep);
            markp->dtypeSetVoid();
            initTaskp->addStmtsp(markp->makeStmt());
        }
        ಯದಿ (isGlobalConstrained && memberselp && randMode.usesMode) {
            setRandMode(varp, memberselp, smtName, randMode, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        ಯದಿ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createVarExprp(Ast_ಗಂಟು_Var_Ref* nodep, ಧ್ರುವ RandomizeMode randMode,
                                ಧ್ರುವ std::string& smtName, ಧ್ರುವ ತಾರ್ಕಿಕ isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ಧ್ರುವ varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = ನೂತನ AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, ಅಸತ್ಯ, nullptr};
        ಯದಿ (!randMode.usesMode) {
            // Global constraints keep nodep alive for write_var processing
            ಯದಿ (!isGlobalConstrained) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* randModeAccess;
        ಧ್ರುವ ತಾರ್ಕಿಕ varIsStatic = varp->lifetime().isStatic();
        AstClass* ಧ್ರುವ varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : nullptr;
        AstVar* ಧ್ರುವ ownerStaticRandModeVarp
            = varOwningClassp ? findStaticRandModeVarMember(varOwningClassp) : nullptr;
        ಯದಿ (ownerStaticRandModeVarp) {
            randModeAccess = ನೂತನ AstVarRef{varp->fileline(),
                                           VN_AS(ownerStaticRandModeVarp->user2p(), ಗಂಟು_Module),
                                           ownerStaticRandModeVarp, VAccess::READ};
        } ಅನ್ಯಥಾ ಯದಿ (memberselp) {
            Ast_ಗಂಟು_Module* ಧ್ರುವ varClassp = VN_AS(varp->user2p(), ಗಂಟು_Module);
            AstVar* ಧ್ರುವ effectiveRandModeVarp = getRandModeVarFromClass(varClassp);
            ಯದಿ (effectiveRandModeVarp) {
                // Member's class has randmode, use it
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* parentAccess = memberselp->fromp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                AstMemberSel* randModeSel
                    = ನೂತನ AstMemberSel{varp->fileline(), parentAccess, effectiveRandModeVarp};
                randModeSel->dtypep(effectiveRandModeVarp->dtypep());
                randModeAccess = randModeSel;
            } ಅನ್ಯಥಾ {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
                randModeAccess
                    = ನೂತನ AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), ಗಂಟು_Module),
                                    m_randModeVarp, VAccess::READ};
            }
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
            randModeAccess
                = ನೂತನ AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), ಗಂಟು_Module),
                                m_randModeVarp, VAccess::READ};
        }

        AstCMethodHard* ಧ್ರುವ atp
            = ನೂತನ AstCMethodHard{nodep->fileline(), randModeAccess, VCMethod::ARRAY_AT,
                                 ನೂತನ AstConst{nodep->fileline(), randMode.index}};
        atp->dtypeSetUInt32();
        exprp = ನೂತನ AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(ಸತ್ಯ);  // Mark as formatted
        ಪ್ರತ್ಯಾಗಮ exprp;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        ಯದಿ (varp->user4p()) {
            ತಾರ್ಕಿಕ isSizeRef = ಅಸತ್ಯ;
            ಯದಿ (AstCMethodHard* ಧ್ರುವ methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                ಯದಿ (methodp->method() == VCMethod::ASSOC_SIZE
                    || methodp->method() == VCMethod::DYN_SIZE) {
                    isSizeRef = ಸತ್ಯ;
                }
            }
            ಯದಿ (!isSizeRef && m_sizeConstrainedArraysp) { m_sizeConstrainedArraysp->ಸೇರಿಸು(varp); }
        }

        // Check if this variable is marked as globally constrained
        ಧ್ರುವ ತಾರ್ಕಿಕ isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = nullptr;
        ತಾರ್ಕಿಕ structSelOrCMeth = ಅಸತ್ಯ;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_ಗಂಟು_DType* selectedDtypep = nullptr;
        std::string smtName;
        ಯದಿ (m_nestedAccess) {
            m_nestedAccess->addVarNamePart(
                ನೂತನ AstSFormatF{varp->fileline(), nodep->name(), ಅಸತ್ಯ, nullptr}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_ವೃಕ್ಷ();
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_ಗಂಟು* ತುದಿ_Member_Sel = nodep->backp();
            ಯಾವತ್ (VN_IS(ತುದಿ_Member_Sel->backp(), MemberSel)) {
                ತುದಿ_Member_Sel = ತುದಿ_Member_Sel->backp();
            }
            memberselp = VN_AS(ತುದಿ_Member_Sel, MemberSel)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            smtName = buildMemberPath(memberselp);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_ಗಂಟು* ತುದಿ_Nodep = nodep->backp();
            ಯಾವತ್ (VN_IS(ತುದಿ_Nodep->backp(), StructSel) || VN_IS(ತುದಿ_Nodep->backp(), CMethodHard)) {
                ತುದಿ_Nodep = ತುದಿ_Nodep->backp();
            }

            ಯದಿ (VN_IS(ತುದಿ_Nodep, StructSel) || VN_IS(ತುದಿ_Nodep, CMethodHard)) {
                structSelOrCMeth = ಸತ್ಯ;
                selectedDtypep = VN_AS(ತುದಿ_Nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->dtypep();
            }

            memberselp = VN_CAST(ತುದಿ_Nodep, MemberSel);
            ಯದಿ (memberselp) smtName = buildMemberPath(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            ಯದಿ (smtName == "") smtName = nodep->name();
        } ಅನ್ಯಥಾ {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        ಯದಿ (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        ಯದಿ (dtypeContainsReal(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            ಪ್ರತ್ಯಾಗಮ;
        }
        Ast_ಗಂಟು_Module* ಧ್ರುವ classOrPackagep = nodep->classOrPackagep();
        ಧ್ರುವ RandomizeMode randMode = {.asUQuad = varp->user1()};
        ಯದಿ (!randMode.usesMode && editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skipRefToEnump(), EnumDType);
        ಯದಿ (enumDtp
            && (std::find(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.end(), varp)
                == m_rangeConstrainedEnums.end())) {
            AstVar* genVarp = m_genp;
            ಯದಿ (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_ಗಂಟು_FTask* targetTaskp = m_inlineInitTaskp;
            ಯದಿ (!targetTaskp) targetTaskp = getCreateConstraintSetupFunc(m_classp, m_memberMap);
            UASSERT_OBJ(targetTaskp, nodep, "No function to inline enum range constraint into");
            targetTaskp->addStmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.ತಳ್ಳು_back(varp);
        }

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp
            = createVarExprp(nodep, randMode, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ಧ್ರುವ ತಾರ್ಕಿಕ alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.ಎಣಿಕೆ(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.ಎಣಿಕೆ(smtName) > 0
                                    : memberselp          ? m_writtenVars.ಎಣಿಕೆ(smtName) > 0
                                                          : varp->user3();
        ಧ್ರುವ ತಾರ್ಕಿಕ shouldWriteVar = !alreadyWritten;
        ಯದಿ (shouldWriteVar) {
            // Track this variable path as written
            ಯದಿ (m_inlineInitTaskp)
                m_inlineWrittenVars.ಸೇರಿಸು(smtName);
            ಅನ್ಯಥಾ ಯದಿ (isGlobalConstrained || memberselp)
                m_writtenVars.ಸೇರಿಸು(smtName);
            // For global constraints, delete nodep after processing
            ಯದಿ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);

            ತಾರ್ಕಿಕ isClassRefArray = ಅಸತ್ಯ;
            ತಾರ್ಕಿಕ isUnpackedClassRefArray = ಅಸತ್ಯ;
            AstClassRefDType* elemClassRefDtp = nullptr;
            {
                Ast_ಗಂಟು_DType* varDtp = varp->dtypep()->skipRefp();
                ಯದಿ (varDtp->isNonPackedArray()) {
                    Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    ಯದಿ (elemClassRefDtp) {
                        isClassRefArray = ಸತ್ಯ;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_ಗಂಟು_Module* ಧ್ರುವ classp = getLeftmostVarModulep(memberselp, varp);
            Ast_ಗಂಟು_FTask* ಧ್ರುವ initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            ಯದಿ (m_nestedAccess) {
                Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                addRegistrationStmt(writeVarStmtp, initTaskp);
                ಯದಿ (isGlobalConstrained && memberselp && randMode.usesMode) {
                    setRandMode(varp, memberselp, smtName, randMode, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                ಯದಿ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } ಅನ್ಯಥಾ ಯದಿ (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } ಅನ್ಯಥಾ {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, randMode,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } ಅನ್ಯಥಾ {
            // Variable already written, clean up cloned memberselp if any
            ಯದಿ (memberselp) VL_DO_DANGLING(memberselp->delete_ವೃಕ್ಷ(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            ಯದಿ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        ಯದಿ (m_nestedAccess) { VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_ಎಣಿಕೆ_Ones_Expansion(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argp) {
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, argp->width(), 1};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sump = ನೂತನ AstAnd{fl, argp, ನೂತನ AstConst{fl, ಅಂಕೆ_One}};
        sump->user1(ಸತ್ಯ);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1; i < argp->width(); i++) {
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Bit_Mask{fl, argp->width(), 0};
            ಅಂಕೆ_Bit_Mask.setBit(i, 1);
            AstAnd* ಧ್ರುವ andp
                = ನೂತನ AstAnd{fl, argp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), ನೂತನ AstConst{fl, ಅಂಕೆ_Bit_Mask}};
            andp->user1(ಸತ್ಯ);
            AstShiftR* ಧ್ರುವ shiftp
                = ನೂತನ AstShiftR{fl, andp,
                                ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, argp->width(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(ಸತ್ಯ);
            sump = ನೂತನ AstAdd{fl, sump, shiftp};
            sump->user1(ಸತ್ಯ);
        }
        ಪ್ರತ್ಯಾಗಮ sump;
    }

    ಶೂನ್ಯ visit(Ast_ಎಣಿಕೆ_Ones* nodep) override {
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sump = build_ಎಣಿಕೆ_Ones_Expansion(fl, argp);
        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(ಸತ್ಯ);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(sump);
    }
    ಶೂನ್ಯ visit(AstRedXor* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = nodep->lhsp()->unlinkFrBack();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* redxorp = ನೂತನ AstSel{fl, argp, 0, 1};
        redxorp->user1(ಸತ್ಯ);
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1; i < argp->width(); i++) {
            AstSel* ಧ್ರುವ selp = ನೂತನ AstSel{fl, argp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), i, 1};
            selp->user1(ಸತ್ಯ);

            redxorp = ನೂತನ AstXor{fl, redxorp, selp};
            redxorp->user1(ಸತ್ಯ);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(redxorp);
    }
    ಶೂನ್ಯ visit(AstRedAnd* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        // Convert to (~x == 0)
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = nodep->lhsp()->unlinkFrBack();
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, argp->width(), 0};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ negp = ನೂತನ AstNot{fl, argp};
        negp->user1(ಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eqp = ನೂತನ AstEq{fl, negp, ನೂತನ AstConst{fl, ಅಂಕೆ_Zero}};
        eqp->user1(ಸತ್ಯ);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(eqp);
    }
    ಶೂನ್ಯ visit(AstRedOr* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        // Convert to (x != 0)
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = nodep->lhsp()->unlinkFrBack();
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, argp->width(), 0};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ neqp = ನೂತನ AstNeq{fl, argp, ನೂತನ AstConst{fl, ಅಂಕೆ_Zero}};
        neqp->user1(ಸತ್ಯ);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(neqp);
    }
    ಶೂನ್ಯ visit(AstOneHot* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = nodep->lhsp()->unlinkFrBack();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ w = argp->width();

        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, w, 0};
        AstNeq* ಧ್ರುವ neZerop
            = ನೂತನ AstNeq{fl, argp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), ನೂತನ AstConst{fl, ಅಂಕೆ_Zero}};
        neZerop->user1(ಸತ್ಯ);

        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, w, 1};
        AstSub* ಧ್ರುವ subp = ನೂತನ AstSub{fl, argp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), ನೂತನ AstConst{fl, ಅಂಕೆ_One}};
        subp->user1(ಸತ್ಯ);

        AstAnd* ಧ್ರುವ andp = ನೂತನ AstAnd{fl, argp, subp};
        andp->user1(ಸತ್ಯ);

        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero2{fl, w, 0};
        AstEq* ಧ್ರುವ eqZerop = ನೂತನ AstEq{fl, andp, ನೂತನ AstConst{fl, ಅಂಕೆ_Zero2}};
        eqZerop->user1(ಸತ್ಯ);

        AstLogAnd* ಧ್ರುವ resultp = ನೂತನ AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(ಸತ್ಯ);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(resultp);
    }
    ಶೂನ್ಯ visit(AstOneHot0* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        // $onehot0(x) = (x & (x-1)) == 0
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = nodep->lhsp()->unlinkFrBack();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ w = argp->width();

        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, w, 1};
        AstSub* ಧ್ರುವ subp = ನೂತನ AstSub{fl, argp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), ನೂತನ AstConst{fl, ಅಂಕೆ_One}};
        subp->dtypeFrom(argp);
        subp->user1(ಸತ್ಯ);

        AstAnd* ಧ್ರುವ andp = ನೂತನ AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(ಸತ್ಯ);

        ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, w, 0};
        AstEq* ಧ್ರುವ eqp = ನೂತನ AstEq{fl, andp, ನೂತನ AstConst{fl, ಅಂಕೆ_Zero}};
        eqp->user1(ಸತ್ಯ);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(eqp);
    }
    ಶೂನ್ಯ visit(Ast_ಎಣಿಕೆ_Bits* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();

        ತಾರ್ಕಿಕ ಎಣಿಕೆ_Ones = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ ಎಣಿಕೆ_Zeros = ಅಸತ್ಯ;
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ಧ್ರುವ AstConst* ಧ್ರುವ cp = VN_CAST(ctrlp, Const);
            ಯದಿ (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ಧ್ರುವ zerop
                    = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->width(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (cp->ಅಂಕೆ().bitIs1(0) && !ಎಣಿಕೆ_Ones)
                ಎಣಿಕೆ_Ones = ಸತ್ಯ;
            ಅನ್ಯಥಾ ಯದಿ (cp->ಅಂಕೆ().bitIs0(0) && !ಎಣಿಕೆ_Zeros)
                ಎಣಿಕೆ_Zeros = ಸತ್ಯ;
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = nodep->lhsp()->unlinkFrBack();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ argWidth = argp->width();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sump = nullptr;
        ಯದಿ (ಎಣಿಕೆ_Ones && ಎಣಿಕೆ_Zeros) {
            // ones + zeros = width for 2-state types
            sump = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->width(), (uint32_t)argWidth};
            VL_DO_DANGLING(argp->delete_ವೃಕ್ಷ(), argp);
        } ಅನ್ಯಥಾ ಯದಿ (ಎಣಿಕೆ_Ones) {
            sump = build_ಎಣಿಕೆ_Ones_Expansion(fl, argp);
        } ಅನ್ಯಥಾ ಯದಿ (ಎಣಿಕೆ_Zeros) {
            // width - countones(x)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ onesCountp = build_ಎಣಿಕೆ_Ones_Expansion(fl, argp);
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ widthVal{nodep, onesCountp->width(), (uint32_t)argWidth};
            sump = ನೂತನ AstSub{fl, ನೂತನ AstConst{fl, widthVal}, onesCountp};
            sump->user1(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            sump = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->width(), 0U};
            VL_DO_DANGLING(argp->delete_ವೃಕ್ಷ(), argp);
        }

        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(ಸತ್ಯ);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(sump);
    }
    ಶೂನ್ಯ visit(AstCLog2* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argp = nodep->lhsp()->unlinkFrBack();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ argWidth = argp->width();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ resultWidth = nodep->width();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp
            = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, resultWidth, (uint32_t)argWidth};

        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ k = argWidth - 1; k >= 0; k--) {
            V3_ಸಂಖ್ಯೆ threshold{fl, argWidth, 0};
            ಯದಿ (k < 32)
                threshold.setLong(1ULL << k);
            ಅನ್ಯಥಾ
                threshold.setBit(k, 1);

            AstLte* ಧ್ರುವ ltep
                = ನೂತನ AstLte{fl, argp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), ನೂತನ AstConst{fl, threshold}};
            ltep->user1(ಸತ್ಯ);

            AstConst* ಧ್ರುವ valuep
                = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, resultWidth, (uint32_t)k};

            resultp = ನೂತನ AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(ಸತ್ಯ);
        }

        VL_DO_DANGLING(argp->delete_ವೃಕ್ಷ(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(resultp);
    }
    ಶೂನ್ಯ visit(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ಧ್ರುವ zerop = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    ಶೂನ್ಯ handlePow(Ast_ಗಂಟು_Biop* nodep) {
        ಯದಿ (AstConst* ಧ್ರುವ exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ basep = nodep->lhsp();
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{nodep, basep->width(), 1};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* powerp = ನೂತನ AstConst{fl, ಅಂಕೆ_One};
            ಧ್ರುವ ತಾರ್ಕಿಕ baseSigned = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ಧ್ರುವ int32_t exponent = baseSigned ? exponentp->toSInt() : exponentp->toUInt();
            ಯದಿ (exponent > 0) {
                ಪ್ರತಿ (int32_t i = 0; i < exponent; i++) {
                    ಯದಿ (baseSigned) {
                        powerp = ನೂತನ AstMulS{fl, powerp, basep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    } ಅನ್ಯಥಾ {
                        powerp = ನೂತನ AstMul{fl, powerp, basep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    }
                    powerp->user1(ಸತ್ಯ);
                }
            } ಅನ್ಯಥಾ ಯದಿ (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                ಯದಿ (baseSigned) {
                    powerp = ನೂತನ AstDivS{fl, powerp, basep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    powerp->user1(ಸತ್ಯ);
                    powerp = ನೂತನ AstDivS{fl, powerp, basep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    powerp->user1(ಸತ್ಯ);
                } ಅನ್ಯಥಾ {
                    powerp = ನೂತನ AstDiv{fl, powerp, basep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    powerp->user1(ಸತ್ಯ);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            iterate(powerp);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ basep = VN_CAST(nodep->lhsp(), Const)) {
            ಯದಿ (basep->ಅಂಕೆ().toUInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->width();
                ಧ್ರುವ ತಾರ್ಕಿಕ rhsDependent = rhsp->user1();
                AstConst* ಧ್ರುವ onep = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, width, 1};
                AstShiftL* ಧ್ರುವ shiftp = ನೂತನ AstShiftL{fl, onep, rhsp, width};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = shiftp;
                ಯದಿ (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nClonep = rhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    nClonep->user1(rhsDependent);
                    AstConst* ಧ್ರುವ zeroNp
                        = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, rhsp->width(), 0};
                    AstGteS* ಧ್ರುವ condp = ನೂತನ AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ಧ್ರುವ zeroRp = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, width, 0};
                    resultp = ನೂತನ AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                iterate(resultp);
            } ಅನ್ಯಥಾ {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } ಅನ್ಯಥಾ {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    ಶೂನ್ಯ visit(AstPow* nodep) override { handlePow(nodep); }
    ಶೂನ್ಯ visit(AstPowSS* nodep) override { handlePow(nodep); }
    ಶೂನ್ಯ visit(AstPowSU* nodep) override { handlePow(nodep); }
    ಶೂನ್ಯ visit(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    ಶೂನ್ಯ handleShift(Ast_ಗಂಟು_Biop* nodep) {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhsWidth = nodep->lhsp()->width();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rhsWidth = nodep->rhsp()->width();
        ಯದಿ (rhsWidth < lhsWidth) {
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
            AstExtend* ಧ್ರುವ extendp = ನೂತನ AstExtend{fl, rhsp, lhsWidth};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(ಸತ್ಯ);
            nodep->rhsp(extendp);
        } ಅನ್ಯಥಾ ಯದಿ (rhsWidth > lhsWidth) {
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->dtypep();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ extendp
                = VN_IS(nodep, ShiftRS)
                      ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtendS{fl, lhsp, rhsWidth})
                      : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtend{fl, lhsp, rhsWidth});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(ಸತ್ಯ);
            nodep->lhsp(extendp);
            nodep->dtypeSetLogicSized(rhsWidth, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            AstSel* ಧ್ರುವ selp = ನೂತನ AstSel{fl, nodep, 0, lhsWidth};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ಶೂನ್ಯ visit(AstShiftL* nodep) override { handleShift(nodep); }
    ಶೂನ್ಯ visit(AstShiftR* nodep) override { handleShift(nodep); }
    ಶೂನ್ಯ visit(AstShiftRS* nodep) override { handleShift(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Biop* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Uniop* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        editSMT(nodep, nodep->lhsp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Triop* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtypeSetString();
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Fall back to "(ite cond then else)"
        visit(static_cast<Ast_ಗಂಟು_Triop*>(nodep));
    }
    ಶೂನ್ಯ visit(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        editSMT(nodep, nodep->srcp());
    }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        VNRelinker handle;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ msbp = ನೂತನ AstSFormatF{
            fl, "%1d", ಅಸತ್ಯ,
            ನೂತನ AstAdd{fl, nodep->lsbp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                       ನೂತನ AstConst{fl, static_cast<uint32_t>(nodep->widthConst() - 1)}}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp
            = ನೂತನ AstSFormatF{fl, "%1d", ಅಸತ್ಯ, nodep->lsbp()->unlinkFrBack(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ ತಾರ್ಕಿಕ outermost = !m_deferRandModeHoist;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origp = outermost ? nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ) : nullptr;
        VL_RESTORER(m_structSel);
        m_structSel = ಸತ್ಯ;
        ಯದಿ (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = nodep->fromp();
            AstStructDType* ಧ್ರುವ structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ಯದಿ (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } ಅನ್ಯಥಾ {
                structp->markConstrainedRand(ಸತ್ಯ);
            }
        }
        // Mark Random for structArray
        ಯದಿ (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ಧ್ರುವ structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ಯದಿ (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } ಅನ್ಯಥಾ {
                structp->markConstrainedRand(ಸತ್ಯ);
            }
        }
        {
            VL_RESTORER(m_deferRandModeHoist);
            m_deferRandModeHoist = ಸತ್ಯ;
            iterateChildren(nodep);
        }
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        AstSFormatF* newp = nullptr;
        ಯದಿ (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = ನೂತನ AstSFormatF{fl, "%s.%s." + nodep->name(), ಅಸತ್ಯ,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)};
            ಯದಿ (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } ಅನ್ಯಥಾ {
            newp = ನೂತನ AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), ಅಸತ್ಯ,
                                   nullptr};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಯದಿ (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(origp), origp);
        }
    }
    ಶೂನ್ಯ addStringNamePart(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ indexp, ಧ್ರುವ std::string& fmt = "%32x") {
        ಯದಿ (m_nestedAccess) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bitp = indexp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bitFormatp = ನೂತನ AstSFormatF{bitp->fileline(), fmt, ಅಸತ್ಯ, bitp};
            AstSFormatF* ಧ್ರುವ herep
                = ನೂತನ AstSFormatF{indexp->fileline(), "%s.%s", ಅಸತ್ಯ, bitFormatp};
            m_nestedAccess->addVarNamePart(herep, indexp->name());
        }
    }
    ಶೂನ್ಯ visit(AstAssocSel* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origp = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        AstSFormatF* newp = nullptr;
        ಯದಿ (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ idxp = ನೂತನ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), ಅಸತ್ಯ,
                                                      nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ಧ್ರುವ stringp = VN_AS(nodep->bitp(), CvtPackString);
            ಧ್ರುವ size_t stringSize = VN_AS(stringp->lhsp(), Const)->width();
            ಯದಿ (stringSize > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << stringSize << "bits, limit is 128 bits");
            }
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ idxp = ನೂತನ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), ಅಸತ್ಯ,
                                                      stringp->lhsp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } ಅನ್ಯಥಾ {
            ಯದಿ (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ actual_width = nodep->bitp()->width();
                std::string fmt;
                // Normalize to standard bit width
                ಯದಿ (actual_width <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } ಅನ್ಯಥಾ ಯದಿ (actual_width <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } ಅನ್ಯಥಾ {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::to_string(VL_ಪದಗಳು_I(actual_width) * 8) + "x";
                }
                ಯದಿ (m_nestedAccess) addStringNamePart(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ idxp
                    = ನೂತನ AstSFormatF{fl, fmt, ಅಸತ್ಯ, nodep->bitp()->unlinkFrBack(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } ಅನ್ಯಥಾ {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                ಯದಿ (m_nestedAccess) m_nestedAccess->error();
            }
        }
        ಯದಿ (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        ಯದಿ (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(origp->delete_ವೃಕ್ಷ(), origp);
        }
    }
    ಶೂನ್ಯ rename_Array_ಅಭಿವ್ಯಕ್ತಿ(AstSFormatF* ಧ್ರುವ newp) ಧ್ರುವ {
        ಯದಿ (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        ತಾರ್ಕಿಕ indexIsRand = ಅಸತ್ಯ;
        nodep->bitp()->foreach([&](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* vrefp) {
            ಯದಿ (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                indexIsRand = ಸತ್ಯ;
            }
        });
        ಯದಿ (m_nestedAccess) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bitp = nodep->bitp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ bitFormatp
                = indexIsRand ? ನೂತನ AstSFormatF{bitp->fileline(), bitp->name(), ಅಸತ್ಯ, nullptr}
                              : ನೂತನ AstSFormatF{bitp->fileline(), "%x", ಅಸತ್ಯ, bitp};
            AstSFormatF* ಧ್ರುವ herep
                = ನೂತನ AstSFormatF{nodep->fileline(), "%s.%s", ಅಸತ್ಯ, bitFormatp};
            ಯದಿ (AstSel* ಧ್ರುವ selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->addVarNamePart(herep, selp->fromp()->name());
            } ಅನ್ಯಥಾ {
                m_nestedAccess->addVarNamePart(herep, bitp->name());
            }
            ಯದಿ (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_ವೃಕ್ಷ(), bitp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        ಯದಿ (indexIsRand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp = nodep->bitp()->unlinkFrBack(&handle);
            ಯದಿ (indexp->width() < 32) {
                AstExtend* ಧ್ರುವ extendp = ನೂತನ AstExtend{fl, indexp, 32};
                extendp->user1(ಸತ್ಯ);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ಧ್ರುವ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ಅಭಿವ್ಯಕ್ತಿ(newp);
        } ಅನ್ಯಥಾ {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origp = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ indexp
                = ನೂತನ AstSFormatF{fl, "#x%8x", ಅಸತ್ಯ, nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(indexp);
            AstSFormatF* ಧ್ರುವ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ಅಭಿವ್ಯಕ್ತಿ(newp);
            ಯದಿ (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
                VL_DO_DANGLING(origp->delete_ವೃಕ್ಷ(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isRandModeGate(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಧ್ರುವ AstCMethodHard* ಧ್ರುವ atp = VN_CAST(exprp, CMethodHard);
        ಯದಿ (!atp || atp->method() != VCMethod::ARRAY_AT) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstVar* varp = nullptr;
        ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        ಯದಿ (!varp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(varp->user2p(), Class);
        ಪ್ರತ್ಯಾಗಮ classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    ತಾರ್ಕಿಕ hoistRandModeOverSelectAndMember(AstSFormatF* newp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* origp) {
        ಯದಿ (m_deferRandModeHoist) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Only for selects yielding a non-array element (full chains)
        ಯದಿ (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::vector<AstSFormatF*> frames;
        AstCond* modeCondp = nullptr;
        ಪ್ರತಿ (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ firstp = curp->exprsp();
            frames.ತಳ್ಳು_back(curp);
            ಯದಿ (AstCond* ಧ್ರುವ condp = VN_CAST(firstp, Cond)) {
                ಯದಿ (isRandModeGate(condp->condp())) modeCondp = condp;
                ಭಙ್ಗ;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        ಯದಿ (!modeCondp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        File_ಸಾಲು* ಧ್ರುವ fl = newp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ modep = modeCondp->condp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* activep = modeCondp->thenp()->unlinkFrBack();
        // Rebuild the select chain text around the SMT name, innermost first
        ಪ್ರತಿ (auto it = frames.rbegin(); it != frames.rend(); ++it) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ remainingArgsp = VN_CAST((*it)->exprsp()->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಯದಿ (remainingArgsp) { remainingArgsp->unlinkFrBackWithNext(); }
            activep = ನೂತನ AstSFormatF{fl, (*it)->name(), ಅಸತ್ಯ,
                                      Ast_ಗಂಟು::addNext(activep, remainingArgsp)};
        }
        AstCond* ಧ್ರುವ hoistp = ನೂತನ AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(ಸತ್ಯ);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(newp), newp);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        ಯದಿ (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
            ಯದಿ (m_nestedAccess) {
                AstSFormatF* ಧ್ರುವ formatp = ನೂತನ AstSFormatF{fl, nodep->name(), ಅಸತ್ಯ, nullptr};
                m_nestedAccess->addVarNamePart(ನೂತನ AstSFormatF{fl, "%s.%s", ಅಸತ್ಯ, formatp},
                                               nodep->name());
            }
            Ast_ಗಂಟು* ಬೇರು_ಗಂಟು = nodep->fromp();
            ಯಾವತ್ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ selp = VN_CAST(ಬೇರು_ಗಂಟು, MemberSel)) {
                ಬೇರು_ಗಂಟು = selp->fromp();
            }

            Ast_ಗಂಟು_Sel* arraySelp = VN_CAST(ಬೇರು_ಗಂಟು, ArraySel);
            ಯದಿ (!arraySelp) arraySelp = VN_CAST(ಬೇರು_ಗಂಟು, AssocSel);

            ಯದಿ (arraySelp) {
                Ast_ಗಂಟು_DType* ಧ್ರುವ arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : nullptr;
                ಯದಿ (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ಧ್ರುವ ತಾರ್ಕಿಕ isSimple
                        = nodep->fromp() == ಬೇರು_ಗಂಟು && VN_IS(arraySelp->fromp(), VarRef);
                    ಯದಿ (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = ನೂತನ NestedAccessPath(nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                                              &m_firstExpressionInsideIndexp);
                        AstSFormatF* ಧ್ರುವ formatp
                            = ನೂತನ AstSFormatF{fl, nodep->name(), ಅಸತ್ಯ, nullptr};
                        m_nestedAccess->addVarNamePart(
                            ನೂತನ AstSFormatF{fl, "%s.%s", ಅಸತ್ಯ, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = ಸತ್ಯ;
                    nodep->user1(ಸತ್ಯ);
                    arraySelp->user1(ಸತ್ಯ);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origp = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    iterateChildren(nodep);
                    AstSFormatF* ಧ್ರುವ newp = ನೂತನ AstSFormatF{fl, "%s." + nodep->name(), ಅಸತ್ಯ,
                                                              nodep->fromp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    ಯದಿ (!hoistRandModeOverSelectAndMember(newp, origp))
                        VL_DO_DANGLING(origp->delete_ವೃಕ್ಷ(), origp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            // Check if the root variable participates in global constraints
            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ varRefp = VN_CAST(ಬೇರು_ಗಂಟು, VarRef)) {
                AstVar* ಧ್ರುವ constrainedVar = varRefp->varp();
                ಯದಿ (constrainedVar->globalConstrained()) {
                    ತಾರ್ಕಿಕ ಧ್ರುವ beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    ಯದಿ (beforeIterate) {
                        nodep->replaceWith(ನೂತನ AstSFormatF{fl, "%s." + nodep->name(), ಅಸತ್ಯ,
                                                           nodep->fromp()->unlinkFrBack()});
                    } ಅನ್ಯಥಾ {
                        nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                    }
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        ಯದಿ (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), ಗಂಟು_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ಧ್ರುವ varRefp
                = ನೂತನ AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), ಗಂಟು_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            visit(varRefp);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->method() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ಧ್ರುವ cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_ಗಂಟು_DType* ಧ್ರುವ arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = arrayDtp->subDTypep()->skipRefp();
            ಯದಿ (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = ಸತ್ಯ;
                iterateChildren(nodep);
                File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                AstSFormatF* newp = nullptr;
                ಯದಿ (AstSFormatF* ಧ್ರುವ fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    ಯದಿ (fromp->name() == "%s.%s") {
                        newp = ನೂತನ AstSFormatF{fl, "%s.%s." + nodep->name(), ಅಸತ್ಯ,
                                               fromp->exprsp()->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)};
                    } ಅನ್ಯಥಾ {
                        newp = ನೂತನ AstSFormatF{fl, fromp->name() + "." + nodep->name(), ಅಸತ್ಯ,
                                               nullptr};
                    }
                } ಅನ್ಯಥಾ {
                    newp = ನೂತನ AstSFormatF{fl, nodep->name(), ಅಸತ್ಯ, nullptr};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ {
            editFormat(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {}
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {}
    ಶೂನ್ಯ visit(AstCStmt* nodep) override {}
    ಶೂನ್ಯ visit(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    ಶೂನ್ಯ visit(AstConstraintIf* nodep) override {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thenp = editSingle(fl, nodep->thensp());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elsep = editSingle(fl, nodep->elsesp());
        ಯದಿ (thenp && elsep) {
            newp = ನೂತನ AstCond{fl, nodep->condp()->unlinkFrBack(), thenp, elsep};
        } ಅನ್ಯಥಾ ಯದಿ (thenp) {
            newp = ನೂತನ AstLogIf{fl, nodep->condp()->unlinkFrBack(), thenp};
        } ಅನ್ಯಥಾ ಯದಿ (elsep) {
            newp = ನೂತನ AstLogIf{fl, ನೂತನ AstNot{fl, nodep->condp()->unlinkFrBack()}, elsep};
        }
        ಯದಿ (newp) {
            newp->dtypeSetBit();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(ಸತ್ಯ);  // Assume result-dependent
            nodep->replaceWith(ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, newp});
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    ಶೂನ್ಯ visit(AstBegin* nodep) override {}
    ಶೂನ್ಯ visit(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();

        ಯದಿ (!nodep->bodyp()) {
            nodep->unlinkFrBack();
        } ಅನ್ಯಥಾ ಯದಿ (m_wantSingle) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(itemp);
            cstmtp->add(";");
            Ast_ಗಂಟು* ಧ್ರುವ bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ಧ್ರುವ cexprp = ನೂತನ AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(ನೂತನ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     ನೂತನ AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp},
                                     ಸತ್ಯ});
            cexprp->add("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(ನೂತನ AstSFormatF{fl, "%s", ಅಸತ್ಯ, cexprp});
        } ಅನ್ಯಥಾ {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            ಪ್ರತಿ (Ast_ಗಂಟು* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(elemp, Var)) m_foreachIdxVars.ಸೇರಿಸು(varp);
            }
            iterateAndNextNull(nodep->bodyp());
            Ast_ಗಂಟು* ಧ್ರುವ bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlinkFrBackWithNext());
            nodep->replaceWith(
                ನೂತನ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                             ನೂತನ AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp}, ಸತ್ಯ});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_Module* ಧ್ರುವ genModp = VN_AS(m_genp->user2p(), ಗಂಟು_Module);

        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsTestp = build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, lhsp);
            ಯದಿ (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                ಅನುವರ್ತ;
            }
            VL_DO_DANGLING(lhsTestp->delete_ವೃಕ್ಷ(), lhsTestp);
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsNamep = build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, rhsp);
                ಯದಿ (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    ಅನುವರ್ತ;
                }
                AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
                    fl, ನೂತನ AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtypeSetVoid();
                callp->addPinsp(build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, lhsp));
                callp->addPinsp(rhsNamep);
                nodep->addHereThisAsNext(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstConstraintUnique* nodep) override {
        Ast_ಗಂಟು_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_Module* genModp = nullptr;

        ಯದಿ (m_classp) {
            initTaskp = VN_AS(m_memberMap.findMember(m_classp, "new"), ಗಂಟು_FTask);
            genVarp = getRandomGenerator(m_classp);
        } ಅನ್ಯಥಾ {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        ಯದಿ (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_classp) {
            genModp = VN_AS(genVarp->user2p(), ಗಂಟು_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_ಗಂಟು* setupStmtsp = nullptr;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::vector<std::string> smtExprs;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (AstVarRef* ಧ್ರುವ varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ಧ್ರುವ varp = varRefp->varp();
                Ast_ಗಂಟು_Module* ಧ್ರುವ varModp = VN_AS(varp->user2p(), ಗಂಟು_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep()->skipRefp();

                ಧ್ರುವ ಸ್ಥಿರ auto dynDTypeSupported
                    = [](ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) -> ತಾರ್ಕಿಕ {
                    ಪ್ರತ್ಯಾಗಮ VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elemWidth;
                uint32_t staticSize = 0;  // Element count, zero when only known at run time
                ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ up = VN_CAST(dtypep, UnpackArrayDType)) {
                    ಧ್ರುವ AstRange* ಧ್ರುವ rangep = up->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    staticSize = up->elementsConst();
                    ಯದಿ (staticSize > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        ಅನುವರ್ತ;
                    }
                    elemWidth = static_cast<uint64_t>(varp->dtypep()->width());
                } ಅನ್ಯಥಾ ಯದಿ (dynDTypeSupported(dtypep)) {
                    elemWidth = static_cast<uint64_t>(dtypep->subDTypep()->width());
                } ಅನ್ಯಥಾ {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(ಅಸತ್ಯ));
                    ಅನುವರ್ತ;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ subp = dtypep->subDTypep()->skipRefp();
                ಯದಿ (VN_IS(subp, ಗಂಟು_Array_DType) || VN_IS(subp, QueueDType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    ಅನುವರ್ತ;
                }

                ಯದಿ (m_wantSingle && !staticSize) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    ಅನುವರ್ತ;
                }

                ಯದಿ (!varp->user3()) {
                    varp->user3(ಸತ್ಯ);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ varnamep = ನೂತನ AstCExpr{
                        fl, AstCExpr::Pure{}, "\"" + varp->name() + "\"", varp->width()};

                    AstCMethodHard* ಧ್ರುವ writeVarCallp = ನೂತನ AstCMethodHard{
                        fl, ನೂತನ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ಧ್ರುವ argVarRefp = ನೂತನ AstVarRef{fl, varp, VAccess::READ};
                    ಯದಿ (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->addPinsp(argVarRefp);
                    writeVarCallp->addPinsp(ನೂತನ AstConst{fl, AstConst::Unsized64{}, elemWidth});
                    writeVarCallp->addPinsp(varnamep);
                    writeVarCallp->addPinsp(ನೂತನ AstConst{fl, 1});  // Dimension
                    ಧ್ರುವ RandomizeMode randMode = {.asUQuad = varp->user1()};
                    ಯದಿ (randMode.usesMode) {
                        writeVarCallp->addPinsp(
                            ನೂತನ AstConst{fl, AstConst::Unsized64{}, randMode.index});
                    }
                    writeVarCallp->dtypeSetVoid();
                    initTaskp->addStmtsp(ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                ಯದಿ (varp->user4p() && m_sizeConstrainedArraysp) {
                    m_sizeConstrainedArraysp->ಸೇರಿಸು(varp);
                }

                ಯದಿ (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    ಯದಿ (staticSize < 2) ಅನುವರ್ತ;
                    std::string ಅಭಿವ್ಯಕ್ತಿ_Str = "(__Vbv (distinct";
                    ಪ್ರತಿ (uint32_t i = 0; i < staticSize; ++i) {
                        ಅಭಿವ್ಯಕ್ತಿ_Str += " (select " + varp->name() + " #x";
                        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ shift = 28; shift >= 0; shift -= 4) {
                            ಅಭಿವ್ಯಕ್ತಿ_Str += "0123456789abcdef"[(i >> shift) & 0xf];
                        }
                        ಅಭಿವ್ಯಕ್ತಿ_Str += ')';
                    }
                    smtExprs.emplace_back(ಅಭಿವ್ಯಕ್ತಿ_Str + "))");
                    ಅನುವರ್ತ;
                }

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ namep = ನೂತನ AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ಧ್ರುವ randUniqueCallp
                    = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->findVoidDType());
                randUniqueCallp->addPinsp(ನೂತನ AstConst{fl, *m_uniqueConstraintId});
                setupStmtsp = Ast_ಗಂಟು::addNext(setupStmtsp, ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        ಯದಿ (m_wantSingle && !smtExprs.empty()) {
            std::string ಅಭಿವ್ಯಕ್ತಿ_Str = smtExprs.front();
            ಯದಿ (smtExprs.size() > 1) {
                ಅಭಿವ್ಯಕ್ತಿ_Str = "(bvand";
                ಪ್ರತಿ (ಧ್ರುವ std::string& oneExprp : smtExprs) ಅಭಿವ್ಯಕ್ತಿ_Str += " " + oneExprp;
                ಅಭಿವ್ಯಕ್ತಿ_Str += ')';
            }
            AstSFormatF* ಧ್ರುವ newp = ನೂತನ AstSFormatF{fl, ಅಭಿವ್ಯಕ್ತಿ_Str, ಅಸತ್ಯ, nullptr};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (setupStmtsp) nodep->addHereThisAsNext(setupStmtsp);
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_firstExpressionInsideIndexp);
        ಯದಿ (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->exprp();
            ಯದಿ (exprp->width() > 1) {
                File_ಸಾಲು* ಧ್ರುವ fl = exprp->fileline();
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, exprp->width(), 0};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ neqp
                    = ನೂತನ AstNeq{fl, exprp->unlinkFrBack(), ನೂತನ AstConst{fl, ಅಂಕೆ_Zero}};
                neqp->user1(ಸತ್ಯ);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        ಯದಿ (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ಧ್ರುವ VCMethod method
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
            nodep->fileline(),
            ನೂತನ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                          VAccess::READWRITE},
            method, nodep->exprp()->unlinkFrBack()};
        callp->dtypeSetVoid();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->addPinsp(ನೂತನ AstCExpr{nodep->fileline(), AstCExpr::Pure{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ಧ್ರುವ uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->addPinsp(ನೂತನ AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string prettyText;
        ಯದಿ (!v3Global.opt.protectIds()) {
            prettyText = nodep->fileline()->prettySource();
            size_t pos = 0;
            ಯಾವತ್ ((pos = prettyText.find('"', pos)) != std::string::npos) {
                prettyText.ಸೇರಿಸು(pos, "\\");
                pos += std::strlen("\\\"");
            }
        }
        callp->addPinsp(
            ನೂತನ AstCExpr{nodep->fileline(), AstCExpr::Pure{}, "\"" + prettyText + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();

        ಯದಿ (nodep->method() == VCMethod::ARRAY_AT) {
            addStringNamePart(nodep->pinsp(), "%x");
            ಧ್ರುವ ತಾರ್ಕಿಕ indexIsRand = nodep->pinsp()->exists([](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ vrefp) {
                ಪ್ರತ್ಯಾಗಮ vrefp->varp()->rand().isRandomizable();
            });
            ಯದಿ (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ basep = nodep->baseFromp(ಸತ್ಯ);
                    ಯದಿ (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origp = indexIsRand ? nullptr : nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                ಧ್ರುವ AstVarRef* ಧ್ರುವ baseRefp
                    = m_structSel ? VN_CAST(nodep->fromp()->baseFromp(ಸತ್ಯ), VarRef) : nullptr;
                ಯದಿ (baseRefp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(baseRefp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ indexp = nodep->pinsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ arrayp = nodep->fromp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    AstCMethodHard* ಧ್ರುವ currentSizep
                        = ನೂತನ AstCMethodHard{fl, arrayp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), VCMethod::DYN_SIZE};
                    currentSizep->dtypeSetInt();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ inRangeRequiredp = ನೂತನ AstLogAnd{
                        fl,
                        ನೂತನ AstLteS{fl,
                                    ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, indexp->width(), 0},
                                    indexp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)},
                        ನೂತನ AstGteS{fl, indexp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), currentSizep}};
                    AstCMethodHard* ಧ್ರುವ resizep
                        = ನೂತನ AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             ನೂತನ AstAdd{fl, indexp, ನೂತನ AstConst{fl, 1}}};
                    resizep->dtypeSetVoid();
                    m_prepareConstrainedArraysp->addStmtsp(
                        ನೂತನ AstIf{fl, inRangeRequiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ಧ್ರುವ sizep
                    = m_structSel ? ನೂತನ AstCMethodHard{fl, nodep->fromp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                       VCMethod::DYN_SIZE}
                                  : nullptr;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ pinp = nodep->pinsp()->unlinkFrBack();
                AstSFormatF* newp;
                ಯದಿ (m_structSel) {
                    ಯದಿ (AstSFormatF* ಧ್ರುವ sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argsp
                        = Ast_ಗಂಟು::addNext(nodep->fromp()->unlinkFrBack(), pinp);
                    sizep->dtypeSetInt();
                    AstLogAnd* ಧ್ರುವ condp
                        = ನೂತನ AstLogAnd{fl,
                                        ನೂತನ AstLteS{fl,
                                                    ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{},
                                                                 originalPinp->width(), 0},
                                                    originalPinp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)},
                                        ನೂತನ AstLtS{fl, originalPinp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), sizep}};
                    m_conditionp = m_conditionp ? ನೂತನ AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = ನೂತನ AstSFormatF{fl, "%s.%s", ಅಸತ್ಯ, argsp};
                } ಅನ್ಯಥಾ {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಯದಿ (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_ವೃಕ್ಷ(), origp);
                }

            } ಅನ್ಯಥಾ ಯದಿ (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } ಅನ್ಯಥಾ {
                nodep->user1(ಅಸತ್ಯ);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (nodep->method() == VCMethod::ARRAY_INSIDE) {
            ಧ್ರುವ ತಾರ್ಕಿಕ randArr = nodep->fromp()->user1();

            AstVar* ಧ್ರುವ newVarp
                = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->findIntDType()};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ idxRefp = ನೂತನ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            AstForeachHeader* ಧ್ರುವ headerp
                = ನೂತನ AstForeachHeader{fl, nodep->fromp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), newVarp};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_ಗಂಟು* ಧ್ರುವ itemp = ನೂತನ AstEq{fl, selp, nodep->pinsp()->unlinkFrBack()};
            itemp->user1(ಸತ್ಯ);

            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(iterateSubtreeReturnEdits(itemp));
            cstmtp->add(";");
            AstCExpr* ಧ್ರುವ cexprp = ನೂತನ AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(ನೂತನ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     ನೂತನ AstForeach{fl, headerp, cstmtp}, ಸತ್ಯ});
            cexprp->add("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(ನೂತನ AstSFormatF{fl, "%s", ಅಸತ್ಯ, cexprp});
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Array reduction methods (sum, product, and, or, xor)
        ಯದಿ (nodep->method() == VCMethod::ARRAY_R_SUM
            || nodep->method() == VCMethod::ARRAY_R_PRODUCT
            || nodep->method() == VCMethod::ARRAY_R_AND || nodep->method() == VCMethod::ARRAY_R_OR
            || nodep->method() == VCMethod::ARRAY_R_XOR) {

            AstWith* ಧ್ರುವ withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            ಯದಿ (withp) {
                ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ arraySize = adtypep->elementsConst();
                    ಯದಿ (arraySize > v3Global.opt.constraintArrayLimit()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvtToStr(arraySize)
                                          + " exceeds --constraint-array-limit of "
                                          + cvtToStr(v3Global.opt.constraintArrayLimit())
                                          + "), treating as state");
                        nodep->user1(ಅಸತ್ಯ);
                        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ಧ್ರುವ ತಾರ್ಕಿಕ hasRandomArray = nodep->fromp()->user1();
            ಯದಿ (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(ಅಸತ್ಯ);
                ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                ಪ್ರತ್ಯಾಗಮ;
            }

            // Create loop variable and header
            AstVar* ಧ್ರುವ loopVarp
                = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->findIntDType()};
            AstForeachHeader* ಧ್ರುವ headerp
                = ನೂತನ AstForeachHeader{fl, nodep->fromp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), loopVarp};

            // Determine SMT operation and compute identity elements
            ಧ್ರುವ ವರ್ಣ* smtOp = nullptr;
            std::string identity;
            ಯದಿ (withp) {
                // For 'with' clause: use binary format, compute from result width
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->dtypep()->width();
                ಯದಿ (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = "#b" + std::string(width, '0');
                } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = (width > 0) ? "#b" + std::string(width - 1, '0') + "1" : "#b0";
                } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = "#b" + std::string(width, '1');
                } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = "#b" + std::string(width, '0');
                } ಅನ್ಯಥಾ {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = "#b" + std::string(width, '0');
                }
            } ಅನ್ಯಥಾ {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ಧ್ರುವ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ಧ್ರುವ arrVarp = arrRefp->varp();
                Ast_ಗಂಟು_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elemWidth = elemDtp->width();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ hexDigits = (elemWidth + 3) / 4;

                ಧ್ರುವ std::string zeroIdentity = "#x" + std::string(hexDigits, '0');
                ಧ್ರುವ std::string oneIdentity = "#x" + std::string(hexDigits - 1, '0') + "1";
                ಧ್ರುವ std::string allOnesIdentity = "#x" + std::string(hexDigits, 'f');

                ಯದಿ (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = zeroIdentity;
                } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = oneIdentity;
                } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = allOnesIdentity;
                } ಅನ್ಯಥಾ ಯದಿ (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = zeroIdentity;
                } ಅನ್ಯಥಾ {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = zeroIdentity;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ಧ್ರುವ cstmtp = ನೂತನ AstCStmt{fl};
            ಯದಿ (withp) {
                // With 'with' clause: evaluate expression for each element
                ಧ್ರುವ ತಾರ್ಕಿಕ randArr = nodep->fromp()->user1();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ idxRefp = ನೂತನ AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ resultWidth = nodep->dtypep()->width();
                ಧ್ರುವ VSigning resultSigning = nodep->dtypep()->numeric();

                Ast_ಗಂಟು* perElemExprp = withp->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                ಯದಿ (AstLambdaArgRef* ಧ್ರುವ ಬೇರು_Refp = VN_CAST(perElemExprp, LambdaArgRef)) {
                    ಯದಿ (ಬೇರು_Refp->index()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ಧ್ರುವ loopRef = ನೂತನ AstVarRef{fl, loopVarp, VAccess::READ};
                        perElemExprp = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                    } ಅನ್ಯಥಾ {
                        // item at root -> replace with array element
                        perElemExprp = elemSelp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    }
                    VL_DO_DANGLING(ಬೇರು_Refp->delete_ವೃಕ್ಷ(), ಬೇರು_Refp);
                } ಅನ್ಯಥಾ {
                    perElemExprp->foreach([&](AstLambdaArgRef* refp) {
                        ಯದಿ (refp->index()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ಧ್ರುವ loopRef = ನೂತನ AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_ಗಂಟು* ಧ್ರುವ replacement
                                = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                            refp->replaceWith(replacement);
                        } ಅನ್ಯಥಾ {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                        }
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_ವೃಕ್ಷ(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                perElemExprp = V3Const::constifyEdit(perElemExprp);

                perElemExprp->foreach([&](Ast_ಗಂಟು* nodep) {
                    // Don't mark loop variable references as randomizable
                    ಯದಿ (!VN_IS(nodep, Const)) {
                        ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ vrefp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
                            ಯದಿ (vrefp->varp() != loopVarp) nodep->user1(ಸತ್ಯ);
                        } ಅನ್ಯಥಾ {
                            nodep->user1(ಸತ್ಯ);
                        }
                    }
                });

                cstmtp->add("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(iterateSubtreeReturnEdits(perElemExprp));
                cstmtp->add(";\n");
                cstmtp->add("ret += \")\";\n");
            } ಅನ್ಯಥಾ {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ಧ್ರುವ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ಧ್ರುವ arrVarp = arrRefp->varp();
                ಧ್ರುವ std::string smtArrayName = arrVarp->name();
                Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elemWidth = elemDtp->width();
                Ast_ಗಂಟು_Module* ಧ್ರುವ arrModulep = VN_AS(arrVarp->user2p(), ಗಂಟು_Module);
                Ast_ಗಂಟು_Module* ಧ್ರುವ genModulep = VN_AS(m_genp->user2p(), ಗಂಟು_Module);
                arrVarp->user3(ಸತ್ಯ);

                // Add write_var call to init task
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ arrVarNamep
                    = ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "\"" + smtArrayName + "\"", elemWidth};
                AstCMethodHard* ಧ್ರುವ writeVarCallp
                    = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->addPinsp(ನೂತನ AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->addPinsp(
                    ನೂತನ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elemWidth)});
                writeVarCallp->addPinsp(arrVarNamep);
                ಧ್ರುವ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(ಅಸತ್ಯ).second;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->addPinsp(ನೂತನ AstConst{fl, 1});  // Dimension

                ಧ್ರುವ RandomizeMode randMode = {.asUQuad = arrVarp->user1()};
                ಯದಿ (randMode.usesMode) {
                    writeVarCallp->addPinsp(
                        ನೂತನ AstConst{fl, AstConst::Unsized64{}, randMode.index});
                }
                writeVarCallp->dtypeSetVoid();
                Ast_ಗಂಟು_FTask* initTaskp = m_inlineInitTaskp;
                ಯದಿ (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.findMember(arrModulep, "new"), ಗಂಟು_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->addStmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ಧ್ರುವ idxFormatp = ನೂತನ AstSFormatF{
                    fl, "#x%08x", ಅಸತ್ಯ, ನೂತನ AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ಧ್ರುವ varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), idxFormatp);

                cstmtp->add("ret += \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(varNamep);
                cstmtp->add(";\n");
            }

            // Build final expression
            ಯದಿ (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ಧ್ರುವ cexprp = ನೂತನ AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret = \"" + identity + "\";\n");
                cexprp->add(ನೂತನ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         ನೂತನ AstForeach{fl, headerp, cstmtp}, ಸತ್ಯ});
                cexprp->add("return ret;\n})()");
                nodep->replaceWith(ನೂತನ AstSFormatF{fl, "%s", ಅಸತ್ಯ, cexprp});
            } ಅನ್ಯಥಾ {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ಧ್ರುವ cexprp = ನೂತನ AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret;\n");
                cexprp->add(ನೂತನ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         ನೂತನ AstForeach{fl, headerp, cstmtp}, ಸತ್ಯ});
                cexprp->add("return ret.empty() ? \"" + identity + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(ನೂತನ AstSFormatF{fl, "%s", ಅಸತ್ಯ, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // We've handled DYN_SIZE already if they should be randomized
        ಯದಿ (nodep->method() == VCMethod::DYN_SIZE || nodep->method() == VCMethod::ASSOC_SIZE) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಗಂಟು_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            ಯಾವತ್ (VN_IS(ಗಂಟು_Exprp, MemberSel)) {
                ಗಂಟು_Exprp = VN_AS(ಗಂಟು_Exprp, MemberSel)->fromp();
            }
            ಯದಿ (VN_IS(ಗಂಟು_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)));
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(ಅಸತ್ಯ);

        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;

        AstFunc* ಧ್ರುವ funcp = VN_CAST(nodep->taskp(), Func);
        ಯದಿ (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            ಪ್ರತ್ಯಾಗಮ;
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ retExprp = extract_Return_ಅಭಿವ್ಯಕ್ತಿ(funcp);
        ಯದಿ (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(ಅಸತ್ಯ);
            ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ಧ್ರುವ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), nullptr, ಅಸತ್ಯ);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inlinedp = retExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            ಧ್ರುವ AstVar* ಧ್ರುವ portp = tconnect.first;
            AstArg* ಧ್ರುವ argp = tconnect.second;
            ಯದಿ (!argp || !argp->exprp()) ಅನುವರ್ತ;
            ಯದಿ (AstVarRef* ಧ್ರುವ ಬೇರು_Refp = VN_CAST(inlinedp, VarRef)) {
                ಯದಿ (ಬೇರು_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    VL_DO_DANGLING(ಬೇರು_Refp->delete_ವೃಕ್ಷ(), ಬೇರು_Refp);
                    ಅನುವರ್ತ;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                ಯದಿ (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                    VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಯದಿ (editFormat(nodep)) ಪ್ರತ್ಯಾಗಮ;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_ಗಂಟು* buildDisableSoftCallStmt(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        std::string varName;
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ vrefp = VN_CAST(nodep->exprp(), ಗಂಟು_Var_Ref)) {
            varName = vrefp->name();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
            nodep->fileline(),
            ನೂತನ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            ನೂತನ AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtypeSetVoid();
        ಪ್ರತ್ಯಾಗಮ callp->makeStmt();
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
    Ast_ಗಂಟು* extractConditionalDisableSofts(Ast_ಗಂಟು* itemsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outerCondp) {
        Ast_ಗಂಟು* hoistListp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ಯದಿ (AstConstraintIf* ಧ್ರುವ ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_ಸಾಲು* ಧ್ರುವ fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thenCondp
                    = outerCondp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                       ನೂತನ AstLogAnd{fl, outerCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                     ifp->condp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)})
                                 : ifp->condp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ thenHoistp
                    = extractConditionalDisableSofts(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_ಗಂಟು::addNext(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_ವೃಕ್ಷ(), thenCondp);
                ಯದಿ (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ notInnerp
                        = ನೂತನ AstLogNot{fl, ifp->condp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elseCondp
                        = outerCondp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstLogAnd{
                                           fl, outerCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), notInnerp})
                                     : notInnerp;
                    ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ elseHoistp
                        = extractConditionalDisableSofts(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_ಗಂಟು::addNext(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_ವೃಕ್ಷ(), elseCondp);
                }
            } ಅನ್ಯಥಾ ಯದಿ (Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_CAST(stmtp, Constraint_ಅಭಿವ್ಯಕ್ತಿ)) {
                ಯದಿ (exprp->isDisableSoft() && outerCondp) {
                    File_ಸಾಲು* ಧ್ರುವ fl = exprp->fileline();
                    AstIf* ಧ್ರುವ hoistIfp = ನೂತನ AstIf{fl, outerCondp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                                      buildDisableSoftCallStmt(exprp), nullptr};
                    hoistListp = hoistListp ? Ast_ಗಂಟು::addNext(hoistListp,
                                                               static_cast<Ast_ಗಂಟು*>(hoistIfp))
                                            : static_cast<Ast_ಗಂಟು*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ truep
                        = ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, ನೂತನ AstConst{fl, AstConst::BitTrue{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        ಪ್ರತ್ಯಾಗಮ hoistListp;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_ಗಂಟು* nodep,
                                   Ast_ಗಂಟು_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* randModeVarp, std::set<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3UniqueNames& uniqueNames,
                                   Ast_ಗಂಟು_FTask* memberselInitTaskp = nullptr,
                                   VInsertionSet<AstVar*>* sizeConstrainedArraysp = nullptr,
                                   Ast_ಗಂಟು_FTask* prepareConstrainedArraysp = nullptr)
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
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ hoistListp
            = nodep ? extractConditionalDisableSofts(nodep, nullptr) : nullptr) {
            nodep->addNext(hoistListp);
        }
        iterateAndNextNull(nodep);
    }
};

ಗಣನಾ ವರ್ಗ CaptureMode : uint8_t {
    CAP_NO = 0x0,
    CAP_ಮೌಲ್ಯ = 0x01,
    CAP_THIS = 0x02,
    CAP_F_SET_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
CaptureMode operator|(CaptureMode a, CaptureMode b) {
    ಪ್ರತ್ಯಾಗಮ static_cast<CaptureMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
CaptureMode operator&(CaptureMode a, CaptureMode b) {
    ಪ್ರತ್ಯಾಗಮ static_cast<CaptureMode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
CaptureMode mode(CaptureMode a) { ಪ್ರತ್ಯಾಗಮ a & static_cast<CaptureMode>(0x3); }
ತಾರ್ಕಿಕ hasFlags(CaptureMode a, CaptureMode flags) {
    ಪ್ರತ್ಯಾಗಮ ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

ವರ್ಗ CaptureVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    AstArg* m_argsp = nullptr;  // Original references turned into arguments
    Ast_ಗಂಟು_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ಧ್ರುವ AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::set<Ast_ಗಂಟು*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = nullptr;  // Variable for outer context's object, if necessary
    ತಾರ್ಕಿಕ m_staticContext = ಅಸತ್ಯ;  // True when capturing from a static function context

    // METHODS

    ತಾರ್ಕಿಕ captureVariable(File_ಸಾಲು* ಧ್ರುವ fileline, Ast_ಗಂಟು_Var_Ref* varrefp, AstVar*& varp) {
        auto it = m_varCloneMap.find(varrefp->varp());
        ಯದಿ (it == m_varCloneMap.end()) {
            AstVar* ಧ್ರುವ newVarp = varrefp->varp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(ಸತ್ಯ);
            newVarp->direction(VDirection::ಒಳಹರಿವು);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        varp = it->second;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    template <typename T_ಗಂಟು>
    ಶೂನ್ಯ fixupClassOrPackage(Ast_ಗಂಟು* memberp, T_ಗಂಟು refp) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ declClassp = VN_AS(memberp->user2p(), ಗಂಟು_Module);
        ಯದಿ (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    template <typename T_ಗಂಟು>
    ತಾರ್ಕಿಕ isReferenceToInnerMember(T_ಗಂಟು nodep) {
        ಪ್ರತ್ಯಾಗಮ VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_ಸಾಲು* fl) {
        ಯದಿ (!m_thisp) {
            AstClassRefDType* ಧ್ರುವ refDTypep
                = ನೂತನ AstClassRefDType{fl, VN_AS(m_callerp, Class), nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
            m_thisp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(ಸತ್ಯ);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::ಒಳಹರಿವು);
            m_argsp = Ast_ಗಂಟು::addNext(m_argsp, ನೂತನ AstArg{fl, "", ನೂತನ AstThisRef{fl, refDTypep}});
        }
        ಪ್ರತ್ಯಾಗಮ m_thisp;
    }

    AstVar* getVar(AstVar* ಧ್ರುವ varp) ಧ್ರುವ {
        ಧ್ರುವ auto it = m_varCloneMap.find(varp);
        ಯದಿ (it == m_varCloneMap.end()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (it->second->isStdRandomizeArg()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ it->second;
    }

    CaptureMode getVarRefCaptureMode(Ast_ಗಂಟು_Var_Ref* varRefp) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ varModp = VN_AS(varRefp->varp()->user2p(), ಗಂಟು_Module);
        AstClass* ಧ್ರುವ varClassp = VN_CAST(varModp, Class);
        AstClass* ಧ್ರುವ callerClassp = VN_CAST(m_callerp, Class);

        ಧ್ರುವ ತಾರ್ಕಿಕ callerIsClass = callerClassp;
        ಧ್ರುವ ತಾರ್ಕಿಕ refIsXref = VN_IS(varRefp, VarXRef);
        ಧ್ರುವ ತಾರ್ಕಿಕ varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ಧ್ರುವ ತಾರ್ಕಿಕ varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ಧ್ರುವ ತಾರ್ಕಿಕ varIsFieldOfCaller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ಧ್ರುವ ತಾರ್ಕಿಕ varIsParam = varRefp->varp()->isParam();
        ಧ್ರುವ ತಾರ್ಕಿಕ varIsConstraintIterator
            = VN_IS(varRefp->varp()->firstAbovep(), ForeachHeader)
              && VN_IS(varRefp->varp()->firstAbovep()->firstAbovep(), ConstraintForeach);
        ಯದಿ (refIsXref) ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_ಮೌಲ್ಯ | CaptureMode::CAP_F_XREF;
        ಯದಿ (varIsConstraintIterator) ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_NO;
        ಯದಿ (varIsFuncLocal && varHasAutomaticLifetime) ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_ಮೌಲ್ಯ;
        ಯದಿ (varIsParam) ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_ಮೌಲ್ಯ;
        // Static var in function (will not be inlined, because it's in class)
        ಯದಿ (callerIsClass && varIsFuncLocal) ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_ಮೌಲ್ಯ;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        ಯದಿ (m_staticContext && callerIsClass && varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_NO;
        ಯದಿ (callerIsClass && varIsFieldOfCaller) ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        ಯದಿ (callerIsClass && varClassp && !varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_ಮೌಲ್ಯ;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        ಪ್ರತ್ಯಾಗಮ CaptureMode::CAP_ಮೌಲ್ಯ;
    }

    ಶೂನ್ಯ capture_Ref_By_ಮೌಲ್ಯ(Ast_ಗಂಟು_Var_Ref* nodep, CaptureMode capModeFlags) {
        AstVar* newVarp;
        ಧ್ರುವ ತಾರ್ಕಿಕ newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ newVarRefp = newCapture ? nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ) : nullptr;
        ಯದಿ (!hasFlags(capModeFlags, CaptureMode::CAP_F_SET_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(nullptr);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        ಯದಿ (!newCapture) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (hasFlags(capModeFlags, CaptureMode::CAP_F_XREF)) {
            AstVarRef* ಧ್ರುವ notXVarRefp
                = ನೂತನ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_ಗಂಟು::addNext(m_argsp, ನೂತನ AstArg{nodep->fileline(), "", newVarRefp});
    }

    ಶೂನ್ಯ captureRefByThis(Ast_ಗಂಟು_Var_Ref* nodep, CaptureMode /*capModeFlags*/) {
        AstVar* ಧ್ರುವ thisp = importThisp(nodep->fileline());
        AstVarRef* ಧ್ರುವ thisRefp = ನೂತನ AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(ಸತ್ಯ);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ಧ್ರುವ memberSelp
            = ನೂತನ AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        ಯದಿ (!m_targetp && nodep->user1()) memberSelp->user1(ಸತ್ಯ);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (m_ignore.ಎಣಿಕೆ(nodep)) ಪ್ರತ್ಯಾಗಮ;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        ಯದಿ (nodep->varp()->isStdRandomizeArg()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ CaptureMode capMode = getVarRefCaptureMode(nodep);
        ಯದಿ (mode(capMode) == CaptureMode::CAP_NO) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (mode(capMode) == CaptureMode::CAP_ಮೌಲ್ಯ) capture_Ref_By_ಮೌಲ್ಯ(nodep, capMode);
        ಯದಿ (mode(capMode) == CaptureMode::CAP_THIS) captureRefByThis(nodep, capMode);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (m_ignore.ಎಣಿಕೆ(nodep)) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        ಯದಿ (VN_IS(nodep, MethodCall) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstClass* ಧ್ರುವ classp = VN_CAST(nodep->taskp()->user2p(), Class);
        ಯದಿ ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ಧ್ರುವ argsp = nodep->argsp();
            ಯದಿ (argsp) argsp->unlinkFrBack();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ಧ್ರುವ thisp = importThisp(nodep->fileline());
            AstVarRef* ಧ್ರುವ thisRefp = ನೂತನ AstVarRef{
                nodep->fileline(), thisp, nodep->isPure() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            AstMethodCall* ಧ್ರುವ methodCallp
                = ನೂತನ AstMethodCall{nodep->fileline(), thisRefp, thisp->name(), argsp};
            methodCallp->taskp(nodep->taskp());
            methodCallp->dtypep(nodep->dtypep());
            nodep->replaceWith(methodCallp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            m_ignore.emplace(methodCallp);
        }
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        ಯದಿ (!isReferenceToInnerMember(nodep)) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstVarRef* ಧ್ರುವ varRefp
            = ನೂತನ AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    ಶೂನ್ಯ visit(AstMethodCall* nodep) override {
        ಯದಿ (!isReferenceToInnerMember(nodep) || m_ignore.ಎಣಿಕೆ(nodep)) {
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstArg* ಧ್ರುವ argsp = nodep->argsp();
        ಯದಿ (argsp) argsp->unlinkFrBackWithNext();
        Ast_ಗಂಟು_FTask_Ref* taskRefp = nullptr;
        ಯದಿ (AstTask* ಧ್ರುವ taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = ನೂತನ AstTaskRef{nodep->fileline(), taskp, argsp};
        } ಅನ್ಯಥಾ ಯದಿ (AstFunc* ಧ್ರುವ taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = ನೂತನ AstFuncRef{nodep->fileline(), taskp, argsp};
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    explicit CaptureVisitor(Ast_ಗಂಟು* ಧ್ರುವ nodep, Ast_ಗಂಟು_Module* callerp, AstClass* ಧ್ರುವ targetp,
                            ತಾರ್ಕಿಕ staticContext = ಅಸತ್ಯ)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterateAndNextNull(nodep);
    }

    // PUBLIC METHODS

    AstArg* getArgs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_argsp; }
    AstVar* getThisp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_thisp; }

    ಶೂನ್ಯ addFunctionArguments(Ast_ಗಂಟು_FTask* funcp) ಧ್ರುವ {
        ಪ್ರತಿ (AstArg* argp = getArgs(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ಯದಿ (Ast_ಗಂಟು_Var_Ref* varrefp = VN_CAST(argp->exprp(), ಗಂಟು_Var_Ref)) {
                ಯದಿ ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(nullptr);
                }
                funcp->addStmtsp(getVar(varrefp->varp()));
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->addStmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

ವರ್ಗ RandomizeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
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
    ಧ್ರುವ VNUser3InUse m_inuser3;
    ಧ್ರುವ VNUser4InUse m_inuser4;

    // STATE
    V3UniqueNames m_inlineUniqueNames;  // For generating unique function names
    V3UniqueNames m_randArrayUniqueNames{"__Vrandarr"};  // Random array loop indices
    V3UniqueNames m_modeUniqueNames{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3UniqueNames m_inlineUniqueStdName{"__VStdrand"};
    V3UniqueNames m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    std::unordered_map<Ast_ಗಂಟು_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ಗಂಟು_Stmt* m_stmtp = nullptr;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = nullptr;  // Dynamic array type (for rand mode)
    ಪೂರ್ಣಾಙ್ಕ m_rand_Case_ಅಂಕೆ = 0;  // Randcase number within a module for var naming
    ಪೂರ್ಣಾಙ್ಕ m_dist_ಅಂಕೆ = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = nullptr;  // Current constraint
    std::set<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertionSet<AstVar*>> m_sizeConstrainedArrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_staticConstraintModeVars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_staticRandModeVars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Similar_ಗಂಟು(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* with_ಅಭಿವ್ಯಕ್ತಿ, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arg_ಅಭಿವ್ಯಕ್ತಿ) {
        // VarRef: compare variable pointers
        ಯದಿ (VN_IS(arg_ಅಭಿವ್ಯಕ್ತಿ, VarRef) && VN_IS(with_ಅಭಿವ್ಯಕ್ತಿ, VarRef)) {
            ಪ್ರತ್ಯಾಗಮ VN_AS(with_ಅಭಿವ್ಯಕ್ತಿ, VarRef)->varp() == VN_AS(arg_ಅಭಿವ್ಯಕ್ತಿ, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        ಯದಿ (VN_IS(arg_ಅಭಿವ್ಯಕ್ತಿ, MemberSel) && VN_IS(with_ಅಭಿವ್ಯಕ್ತಿ, MemberSel)) {
            ಧ್ರುವ AstMemberSel* ಧ್ರುವ withMSp = VN_AS(with_ಅಭಿವ್ಯಕ್ತಿ, MemberSel);
            ಧ್ರುವ AstMemberSel* ಧ್ರುವ argMSp = VN_AS(arg_ಅಭಿವ್ಯಕ್ತಿ, MemberSel);
            ಯದಿ (withMSp->varp() != argMSp->varp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Recursively compare the base object expression
            ಪ್ರತ್ಯಾಗಮ is_Similar_ಗಂಟು(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        ಯದಿ (VN_IS(arg_ಅಭಿವ್ಯಕ್ತಿ, ArraySel) && VN_IS(with_ಅಭಿವ್ಯಕ್ತಿ, ArraySel)) {
            ಧ್ರುವ AstArraySel* ಧ್ರುವ withASp = VN_AS(with_ಅಭಿವ್ಯಕ್ತಿ, ArraySel);
            ಧ್ರುವ AstArraySel* ಧ್ರುವ argASp = VN_AS(arg_ಅಭಿವ್ಯಕ್ತಿ, ArraySel);
            // Before checking Sel type index, check if both are Const
            ಯದಿ (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ಧ್ರುವ AstConst* ಧ್ರುವ withIdxp = VN_AS(withASp->bitp(), Const);
                ಧ್ರುವ AstConst* ಧ್ರುವ argIdxp = VN_AS(argASp->bitp(), Const);
                ಪ್ರತ್ಯಾಗಮ is_Similar_ಗಂಟು(withASp->fromp(), argASp->fromp())
                       && withIdxp->ಅಂಕೆ().isCaseEq(argIdxp->ಅಂಕೆ());
            }
            // Index must be Sel type, extract VarRef using fromp()
            ಯದಿ (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            ಪ್ರತ್ಯಾಗಮ is_Similar_ಗಂಟು(withASp->fromp(), argASp->fromp())
                   && is_Similar_ಗಂಟು(withIdxp, argIdxp);
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ಸ್ಥಿರ ಶೂನ್ಯ expandUniqueElementList(Ast_ಗಂಟು* itemsp) {
        Ast_ಗಂಟು* itemp = itemsp;
        ಯಾವತ್ (itemp) {
            Ast_ಗಂಟು* ಧ್ರುವ nextp = itemp->nextp();
            ಯದಿ (ಧ್ರುವ AstConstraintIf* ಧ್ರುವ ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                ಅನುವರ್ತ;
            }
            ಯದಿ (ಧ್ರುವ AstConstraintForeach* ಧ್ರುವ foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                ಅನುವರ್ತ;
            }
            AstConstraintUnique* ಧ್ರುವ uniquep = VN_CAST(itemp, ConstraintUnique);
            ಯದಿ (!uniquep) {
                itemp = nextp;
                ಅನುವರ್ತ;
            }

            std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು;
            ತಾರ್ಕಿಕ hasArrayVarRef = ಅಸತ್ಯ;
            ಪ್ರತಿ (Ast_ಗಂಟು* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = vrp->varp()->dtypep()->skipRefp();
                    ಯದಿ (VN_IS(dtypep, ಗಂಟು_Array_DType) || VN_IS(dtypep, QueueDType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = ಸತ್ಯ;
                        ಅನುವರ್ತ;
                    }
                }
                ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.ತಳ್ಳು_back(VN_AS(rp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            }

            ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.size() >= 2) {
                File_ಸಾಲು* ಧ್ರುವ fl = uniquep->fileline();
                ಪ್ರತಿ (size_t i = 0; i < ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.size(); i++) {
                    ಪ್ರತಿ (size_t j = i + 1; j < ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.size(); j++) {
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು[i]->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು[j]->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                        AstNeq* ಧ್ರುವ neqp = ನೂತನ AstNeq{fl, lhsp, rhsp};
                        neqp->user1(ಸತ್ಯ);
                        Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cexprp = ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, neqp};
                        uniquep->addNextHere(cexprp);
                    }
                }
                ಯದಿ (!hasArrayVarRef) {
                    uniquep->unlinkFrBack();
                    VL_DO_DANGLING(uniquep->delete_ವೃಕ್ಷ(), uniquep);
                }
            } ಅನ್ಯಥಾ ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.size() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlinkFrBack();
                VL_DO_DANGLING(uniquep->delete_ವೃಕ್ಷ(), uniquep);
            }
            itemp = nextp;
        }
    }
    ಶೂನ್ಯ createRandomGenerator(AstClass* ಧ್ರುವ classp) {
        ಯದಿ (classp->user3p()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstVar* ಧ್ರುವ genp = ನೂತನ AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->addMembersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_ಗಂಟು_Module* ಧ್ರುವ modp) {
        auto it = m_stdMap.find(modp);
        ಯದಿ (it == m_stdMap.end()) {
            AstVar* ಧ್ರುವ stdgenp
                = ನೂತನ AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->findBasicDType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3ErrorCode::IMPURE, ಸತ್ಯ);
            modp->addStmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            ಪ್ರತ್ಯಾಗಮ stdgenp;
        }
        ಪ್ರತ್ಯಾಗಮ it->second;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ಧ್ರುವ classp) {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ name = "__Vprepare_constrained_arrays";
        AstTask* ಧ್ರುವ taskp = ನೂತನ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(ಸತ್ಯ);
        classp->addMembersp(taskp);
        m_memberMap.ಸೇರಿಸು(classp, taskp);
        ಪ್ರತ್ಯಾಗಮ taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ಧ್ರುವ classp) {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.findMember(classp, name), Task);
        ಯದಿ (resizeTaskp) ಪ್ರತ್ಯಾಗಮ resizeTaskp;
        resizeTaskp = ನೂತನ AstTask{classp->fileline(), name, nullptr};
        resizeTaskp->classMethod(ಸತ್ಯ);
        resizeTaskp->isVirtual(ಸತ್ಯ);
        classp->addMembersp(resizeTaskp);
        m_memberMap.ಸೇರಿಸು(classp, resizeTaskp);
        ಪ್ರತ್ಯಾಗಮ resizeTaskp;
    }
    AstVar* getCreateRandModeVar(AstClass* ಧ್ರುವ classp) {
        ಯದಿ (classp->user2p()) ಪ್ರತ್ಯಾಗಮ VN_AS(classp->user2p(), Var);
        ಯದಿ (AstClassExtends* ಧ್ರುವ extendsp = classp->extendsp()) {
            ಪ್ರತ್ಯಾಗಮ getCreateRandModeVar(extendsp->classp());
        }
        AstVar* ಧ್ರುವ randModeVarp = createModeVar(classp, "__Vrandmode");
        classp->user2p(randModeVarp);
        ಪ್ರತ್ಯಾಗಮ randModeVarp;
    }
    AstVar* getCreateConstraintModeVar(AstClass* ಧ್ರುವ classp) {
        ಯದಿ (classp->user4p()) ಪ್ರತ್ಯಾಗಮ VN_AS(classp->user4p(), Var);
        ಯದಿ (AstClassExtends* ಧ್ರುವ extendsp = classp->extendsp()) {
            ಪ್ರತ್ಯಾಗಮ getCreateConstraintModeVar(extendsp->classp());
        }
        AstVar* ಧ್ರುವ constraintModeVarp = createModeVar(classp, "__Vconstraintmode");
        classp->user4p(constraintModeVarp);
        ಪ್ರತ್ಯಾಗಮ constraintModeVarp;
    }
    ಸ್ಥಿರ AstVar* getConstraintModeVar(AstClass* ಧ್ರುವ classp) {
        ಯದಿ (classp->user4p()) ಪ್ರತ್ಯಾಗಮ VN_AS(classp->user4p(), Var);
        ಯದಿ (AstClassExtends* ಧ್ರುವ extendsp = classp->extendsp()) {
            ಪ್ರತ್ಯಾಗಮ getConstraintModeVar(extendsp->classp());
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    AstVar* getCreateStaticConstraintModeVar(AstClass* ಧ್ರುವ classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        ಯದಿ (it != m_staticConstraintModeVars.end()) ಪ್ರತ್ಯಾಗಮ it->second;
        ಯದಿ (AstClassExtends* ಧ್ರುವ extendsp = classp->extendsp()) {
            ಪ್ರತ್ಯಾಗಮ getCreateStaticConstraintModeVar(extendsp->classp());
        }
        AstVar* ಧ್ರುವ staticModeVarp = createStaticModeVar(classp, "__Vstaticconstraintmode");
        m_staticConstraintModeVars[classp] = staticModeVarp;
        ಪ್ರತ್ಯಾಗಮ staticModeVarp;
    }
    AstVar* getStaticConstraintModeVar(AstClass* ಧ್ರುವ classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        ಯದಿ (it != m_staticConstraintModeVars.end()) ಪ್ರತ್ಯಾಗಮ it->second;
        ಯದಿ (AstClassExtends* ಧ್ರುವ extendsp = classp->extendsp()) {
            ಪ್ರತ್ಯಾಗಮ getStaticConstraintModeVar(extendsp->classp());
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    AstVar* getCreateStaticRandModeVar(AstClass* ಧ್ರುವ classp) {
        ಯದಿ (m_staticRandModeVars.ಎಣಿಕೆ(classp)) ಪ್ರತ್ಯಾಗಮ m_staticRandModeVars[classp];
        ಯದಿ (AstClassExtends* ಧ್ರುವ extendsp = classp->extendsp()) {
            ಪ್ರತ್ಯಾಗಮ getCreateStaticRandModeVar(extendsp->classp());
        }
        AstVar* ಧ್ರುವ staticModeVarp = createStaticModeVar(classp, "__Vstaticrandmode");
        m_staticRandModeVars[classp] = staticModeVarp;
        ಪ್ರತ್ಯಾಗಮ staticModeVarp;
    }
    AstVar* getStaticRandModeVar(AstClass* ಧ್ರುವ classp) {
        ಯದಿ (m_staticRandModeVars.ಎಣಿಕೆ(classp)) ಪ್ರತ್ಯಾಗಮ m_staticRandModeVars[classp];
        ಯದಿ (AstClassExtends* ಧ್ರುವ extendsp = classp->extendsp()) {
            ಪ್ರತ್ಯಾಗಮ getStaticRandModeVar(extendsp->classp());
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    AstVar* createModeVar(AstClass* ಧ್ರುವ classp, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ name) {
        File_ಸಾಲು* ಧ್ರುವ fl = classp->fileline();
        ಯದಿ (!m_dynarrayDtp) {
            m_dynarrayDtp = ನೂತನ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ಧ್ರುವ modeVarp = ನೂತನ AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        ಪ್ರತ್ಯಾಗಮ modeVarp;
    }
    AstVar* createStaticModeVar(AstClass* ಧ್ರುವ classp, ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_ಸಾಲು* ಧ್ರುವ fl = classp->fileline();
        ಯದಿ (!m_dynarrayDtp) {
            m_dynarrayDtp = ನೂತನ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        AstVar* ಧ್ರುವ modeVarp = ನೂತನ AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        modeVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.ಸೇರಿಸು(classp, modeVarp);
        ಪ್ರತ್ಯಾಗಮ modeVarp;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ addSetRandMode(Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp, AstVar* ಧ್ರುವ genp,
                               AstVar* ಧ್ರುವ randModeVarp) {
        File_ಸಾಲು* ಧ್ರುವ fl = ftaskp->fileline();
        AstCMethodHard* ಧ್ರುವ setRandModep = ನೂತನ AstCMethodHard{
            fl, ನೂತನ AstVarRef{fl, VN_AS(genp->user2p(), ಗಂಟು_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_SET_RANDMODE,
            ನೂತನ AstVarRef{fl, VN_AS(randModeVarp->user2p(), ಗಂಟು_Module), randModeVarp,
                          VAccess::READ}};
        setRandModep->dtypeSetVoid();
        ftaskp->addStmtsp(setRandModep->makeStmt());
    }
    ಸ್ಥಿರ ಶೂನ್ಯ addSetStaticRandMode(Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp, AstVar* ಧ್ರುವ genp,
                                     AstVar* ಧ್ರುವ staticRandModeVarp) {
        File_ಸಾಲು* ಧ್ರುವ fl = ftaskp->fileline();
        AstCMethodHard* ಧ್ರುವ setp = ನೂತನ AstCMethodHard{
            fl, ನೂತನ AstVarRef{fl, VN_AS(genp->user2p(), ಗಂಟು_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_SET_STATIC_RANDMODE,
            ನೂತನ AstVarRef{fl, VN_AS(staticRandModeVarp->user2p(), ಗಂಟು_Module), staticRandModeVarp,
                          VAccess::READ}};
        setp->dtypeSetVoid();
        ftaskp->addStmtsp(setp->makeStmt());
    }
    ಶೂನ್ಯ createRandomizeClassVars(AstNetlist* ಧ್ರುವ netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> ಬೇರು_Static_Rand_Mode_ಎಣಿಕೆ;
        netlistp->foreach([ಏತತ್, &ಬೇರು_Static_Rand_Mode_ಎಣಿಕೆ](AstClass* ಧ್ರುವ classp) {
            ತಾರ್ಕಿಕ hasConstraints = ಅಸತ್ಯ;
            uint32_t rand_Mode_ಎಣಿಕೆ = 0;
            uint32_t static_Rand_Mode_ಎಣಿಕೆ = 0;
            uint32_t constraint_Mode_ಎಣಿಕೆ = 0;
            uint32_t static_Constraint_Mode_ಎಣಿಕೆ = 0;
            classp->foreachMember([&](AstClass*, Ast_ಗಂಟು* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                ಯದಿ (AstConstraint* ಧ್ರುವ constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = ಸತ್ಯ;
                    RandomizeMode constraintMode = {.asUQuad = memberp->user1()};
                    ಯದಿ (!constraintMode.usesMode) ಪ್ರತ್ಯಾಗಮ;
                    ಯದಿ (constraintMode.index == 0) {
                        // Use separate index counters for static vs non-static constraints
                        ಯದಿ (constrp->isStatic()) {
                            constraintMode.index = static_Constraint_Mode_ಎಣಿಕೆ++;
                        } ಅನ್ಯಥಾ {
                            constraintMode.index = constraint_Mode_ಎಣಿಕೆ++;
                        }
                        memberp->user1(constraintMode.asUQuad);
                    } ಅನ್ಯಥಾ {
                        ಯದಿ (constrp->isStatic()) {
                            static_Constraint_Mode_ಎಣಿಕೆ = constraintMode.index + 1;
                        } ಅನ್ಯಥಾ {
                            constraint_Mode_ಎಣಿಕೆ = constraintMode.index + 1;
                        }
                    }
                } ಅನ್ಯಥಾ ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(memberp, Var)) {
                    RandomizeMode randMode = {.asUQuad = memberp->user1()};
                    ಯದಿ (!randMode.usesMode) ಪ್ರತ್ಯಾಗಮ;
                    ಧ್ರುವ ತಾರ್ಕಿಕ isStaticVar = varp->lifetime().isStatic();
                    ಯದಿ (randMode.index == 0) {
                        ಯದಿ (isStaticVar) {
                            randMode.index = static_Rand_Mode_ಎಣಿಕೆ++;
                        } ಅನ್ಯಥಾ {
                            randMode.index = rand_Mode_ಎಣಿಕೆ++;
                        }
                        memberp->user1(randMode.asUQuad);
                    } ಅನ್ಯಥಾ {
                        ಯದಿ (isStaticVar) {
                            static_Rand_Mode_ಎಣಿಕೆ = randMode.index + 1;
                        } ಅನ್ಯಥಾ {
                            rand_Mode_ಎಣಿಕೆ = randMode.index + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<ಶೂನ್ಯ(AstClass*)> findSubObjRandModes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_ಗಂಟು* subMemberp) {
                        ಯದಿ (AstVar* ಧ್ರುವ subVarp = VN_CAST(subMemberp, Var)) {
                            ಧ್ರುವ RandomizeMode rm = {.asUQuad = subVarp->user1()};
                            ಯದಿ (!rm.usesMode) ಪ್ರತ್ಯಾಗಮ;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            ಯದಿ (subVarp->lifetime().isStatic()) ಪ್ರತ್ಯಾಗಮ;
                            ಧ್ರುವ uint32_t needed = rm.index + 1;
                            ಯದಿ (needed > rand_Mode_ಎಣಿಕೆ) rand_Mode_ಎಣಿಕೆ = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    ಯದಿ (!memberVarp->globalConstrained()) ಪ್ರತ್ಯಾಗಮ;
                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = memberVarp->dtypep()->skipRefp();
                    ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(dtypep, ClassRefDType);
                    ಯದಿ (!classRefp) ಪ್ರತ್ಯಾಗಮ;
                    findSubObjRandModes(classRefp->classp());
                });
            }
            ಯದಿ (hasConstraints) createRandomGenerator(classp);
            ಯದಿ (rand_Mode_ಎಣಿಕೆ > 0) {
                AstVar* ಧ್ರುವ randModeVarp = getCreateRandModeVar(classp);
                makeModeInit(randModeVarp, classp, rand_Mode_ಎಣಿಕೆ);
            }
            ಯದಿ (constraint_Mode_ಎಣಿಕೆ > 0) {
                AstVar* ಧ್ರುವ constraintModeVarp = getCreateConstraintModeVar(classp);
                makeModeInit(constraintModeVarp, classp, constraint_Mode_ಎಣಿಕೆ);
            }
            ಯದಿ (static_Constraint_Mode_ಎಣಿಕೆ > 0) {
                AstVar* ಧ್ರುವ staticConstraintModeVarp = getCreateStaticConstraintModeVar(classp);
                makeStaticModeInit(staticConstraintModeVarp, classp, static_Constraint_Mode_ಎಣಿಕೆ);
            }
            ಯದಿ (static_Rand_Mode_ಎಣಿಕೆ > 0) {
                getCreateStaticRandModeVar(classp);
                AstClass* rootp = classp;
                ಯಾವತ್ (AstClassExtends* ಧ್ರುವ ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& slot = ಬೇರು_Static_Rand_Mode_ಎಣಿಕೆ[rootp];
                ಯದಿ (static_Rand_Mode_ಎಣಿಕೆ > slot) slot = static_Rand_Mode_ಎಣಿಕೆ;
            }
        });
        ಪ್ರತಿ (ಧ್ರುವ auto& kv : ಬೇರು_Static_Rand_Mode_ಎಣಿಕೆ) emit_ಬೇರು_Static_Mode_Init(kv.first, kv.second);
    }
    ಶೂನ್ಯ emit_ಬೇರು_Static_Mode_Init(AstClass* ಧ್ರುವ rootp, ಧ್ರುವ uint32_t ಎಣಿಕೆ) {
        AstVar* ಧ್ರುವ staticRandModeVarp = m_staticRandModeVars[rootp];
        UASSERT_OBJ(staticRandModeVarp, rootp, "Root must have a static rand-mode var");
        makeStaticModeInit(staticRandModeVarp, rootp, ಎಣಿಕೆ);
    }
    ಶೂನ್ಯ makeModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_ಎಣಿಕೆ) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ modeVarModp = VN_AS(modeVarp->user2p(), ಗಂಟು_Module);
        File_ಸಾಲು* ಧ್ರುವ fl = modeVarp->fileline();
        AstVar* ಧ್ರುವ oldSizeVarp = ನೂತನ AstVar{
            fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__VoldSize"), modeVarp->findUInt32DType()};
        oldSizeVarp->funcLocal(ಸತ್ಯ);
        oldSizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ಧ್ರುವ oldSizep
            = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        oldSizep->dtypeSetUInt32();
        AstCMethodHard* ಧ್ರುವ dynarrayNewp
            = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, ನೂತನ AstConst{fl, mode_ಎಣಿಕೆ}};
        dynarrayNewp->dtypeSetVoid();
        Ast_ಗಂಟು_FTask* ಧ್ರುವ ctorNewp = VN_AS(m_memberMap.findMember(classp, "new"), ಗಂಟು_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_ಗಂಟು* ಧ್ರುವ initFirstp = oldSizeVarp;
        initFirstp->addNext(
            ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, oldSizeVarp, VAccess::WRITE}, oldSizep});
        initFirstp->addNext(dynarrayNewp->makeStmt());
        initFirstp->addNext(makeModeSetLoop(
            fl, ನೂತನ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE}, ನೂತನ AstConst{fl, 1},
            ಸತ್ಯ, ನೂತನ AstVarRef{fl, oldSizeVarp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (!VN_IS(stmtp, ಗಂಟು_Stmt)) ಅನುವರ್ತ;  // Skip var declarations
            ಯದಿ (ಧ್ರುವ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sep = VN_CAST(stmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                ಯದಿ (VN_IS(sep->exprp(), New)) ಅನುವರ್ತ;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->addHereThisAsNext(initFirstp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->addStmtsp(initFirstp);
    }
    ಶೂನ್ಯ makeStaticModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_ಎಣಿಕೆ) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_ಗಂಟು_Module* ಧ್ರುವ modeVarModp = VN_AS(modeVarp->user2p(), ಗಂಟು_Module);
        File_ಸಾಲು* ಧ್ರುವ fl = modeVarp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ಧ್ರುವ sizep
            = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstEq* ಧ್ರುವ condp = ನೂತನ AstEq{fl, sizep, ನೂತನ AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ಧ್ರುವ dynarrayNewp
            = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, ನೂತನ AstConst{fl, mode_ಎಣಿಕೆ}};
        dynarrayNewp->dtypeSetVoid();
        Ast_ಗಂಟು* ಧ್ರುವ thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->addNext(
            makeModeSetLoop(fl, ನೂತನ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                            ನೂತನ AstConst{fl, 1}, ಸತ್ಯ));

        // Build the if statement
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_ಗಂಟು_FTask* ಧ್ರುವ newp = VN_AS(m_memberMap.findMember(classp, "new"), ಗಂಟು_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->addStmtsp(ifp);
    }
    Ast_ಗಂಟು* makeModeSetLoop(File_ಸಾಲು* ಧ್ರುವ fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp,
                             ತಾರ್ಕಿಕ inTask, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ startp = nullptr) {
        AstVar* ಧ್ರುವ iterVarp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->findUInt32DType()};
        iterVarp->funcLocal(inTask);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ಧ್ರುವ sizep = ನೂತನ AstCMethodHard{fl, lhsp, VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstCMethodHard* ಧ್ರುವ setp
            = ನೂತನ AstCMethodHard{fl, lhsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), VCMethod::ARRAY_AT_WRITE,
                                 ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtypeSetUInt32();
        Ast_ಗಂಟು* ಧ್ರುವ stmtsp = iterVarp;
        stmtsp->addNext(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : ನೂತನ AstConst{fl, 0}});

        AstLoop* ಧ್ರುವ loopp = ನೂತನ AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(ನೂತನ AstLoopTest{
            fl, loopp, ನೂತನ AstLt{fl, ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->addStmtsp(ನೂತನ AstAssign{fl, setp, rhsp});
        loopp->addStmtsp(ನೂತನ AstAssign{
            fl, ನೂತನ AstVarRef{fl, iterVarp, VAccess::WRITE},
            ನೂತನ AstAdd{fl, ನೂತನ AstConst{fl, 1}, ನೂತನ AstVarRef{fl, iterVarp, VAccess::READ}}});
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtsp, ಸತ್ಯ};
    }
    ಶೂನ್ಯ addTempVarsp(AstVar* varsp) {
        ಪ್ರತಿ (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ಯದಿ (m_ftaskp) {
                varp->funcLocal(ಸತ್ಯ);
            } ಅನ್ಯಥಾ {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        ಯದಿ (!m_ftaskp) {
            m_modp->addStmtsp(varsp);
        } ಅನ್ಯಥಾ {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_ಗಂಟು_Stmt* wrapIfRandMode(AstClass* classp, AstVar* ಧ್ರುವ varp, Ast_ಗಂಟು_Stmt* stmtp) {
        ಧ್ರುವ RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstVar* ಧ್ರುವ modeVarp = varp->lifetime().isStatic() ? getStaticRandModeVar(classp)
                                                             : getRandModeVarFromClass(classp);
        ಪ್ರತ್ಯಾಗಮ VN_AS(wrapIfMode(rmode, modeVarp, stmtp), ಗಂಟು_Stmt);
    }
    Ast_ಗಂಟು* wrapIfConstraintMode(AstClass* classp, AstConstraint* ಧ್ರುವ constrp, Ast_ಗಂಟು* stmtp) {
        ಧ್ರುವ RandomizeMode rmode = {.asUQuad = constrp->user1()};
        AstVar* ಧ್ರುವ modeVarp = constrp->isStatic() ? getStaticConstraintModeVar(classp)
                                                     : getConstraintModeVar(classp);
        ಪ್ರತ್ಯಾಗಮ wrapIfMode(rmode, modeVarp, stmtp);
    }
    ಸ್ಥಿರ Ast_ಗಂಟು* wrapIfMode(ಧ್ರುವ RandomizeMode mode, AstVar* modeVarp, Ast_ಗಂಟು* stmtp) {
        File_ಸಾಲು* ಧ್ರುವ fl = stmtp->fileline();
        ಯದಿ (mode.usesMode) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_ಗಂಟು_Module* ಧ್ರುವ modp = VN_AS(modeVarp->user2p(), ಗಂಟು_Module);
            AstCMethodHard* ಧ್ರುವ atp
                = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, modp, modeVarp, VAccess::READ},
                                     VCMethod::ARRAY_AT, ನೂತನ AstConst{fl, mode.index}};
            atp->dtypeSetUInt32();
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstIf{fl, atp, stmtp};
        }
        ಪ್ರತ್ಯಾಗಮ stmtp;
    }
    AstVarRef* enum_ಮೌಲ್ಯ_Tab_Refp(AstEnumDType* ಧ್ರುವ nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        ಯದಿ (nodep->user2p()) ಪ್ರತ್ಯಾಗಮ VN_AS(nodep->user2p(), VarRef)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_ಗಂಟು_Array_DType* ಧ್ರುವ vardtypep = ನೂತನ AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            ನೂತನ AstRange{nodep->fileline(), static_cast<ಪೂರ್ಣಾಙ್ಕ>(nodep->item_ಎಣಿಕೆ()) - 1, 0}};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        AstConsArray* ಧ್ರುವ consp = ನೂತನ AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t index = 0;
        ಪ್ರತಿ (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ಧ್ರುವ vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(index++, vconstp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        }
        // Share identical tables via the constant pool
        AstVarRef* ಧ್ರುವ refp = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_ವೃಕ್ಷ(), consp);  // V3ConstPool::find clones it
        ತಳ್ಳು_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        ಪ್ರತ್ಯಾಗಮ refp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
    }

    AstCDType* findVlRandCDType(File_ಸಾಲು* ಧ್ರುವ fl, uint64_t ವಸ್ತುಗಳು) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ಧ್ರುವ std::string type = AstCDType::typeToHold(V3_ಸಂಖ್ಯೆ::log2bQuad(ವಸ್ತುಗಳು) + 1);
        ಧ್ರುವ std::string name = "VlRandC<" + type + ", " + cvtToStr(ವಸ್ತುಗಳು) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ಧ್ರುವ auto pair = m_randcDtypes.emplace(name, nullptr);
        ಯದಿ (pair.second) {
            AstCDType* ಧ್ರುವ newp = ನೂತನ AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            pair.first->second = newp;
        }
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    AstVar* newRandcVarsp(AstVar* ಧ್ರುವ varp, AstClass* ಧ್ರುವ classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        ಯದಿ (!varp->isRandC()) ಪ್ರತ್ಯಾಗಮ nullptr;

        uint64_t ವಸ್ತುಗಳು = 0;
        ಯದಿ (AstEnumDType* ಧ್ರುವ enumDtp = VN_CAST(varp->dtypep()->skipRefToEnump(), EnumDType)) {
            ವಸ್ತುಗಳು = static_cast<uint64_t>(enumDtp->item_ಎಣಿಕೆ());
        } ಅನ್ಯಥಾ ಯದಿ (AstBasicDType* ಧ್ರುವ basicp = varp->dtypep()->skipRefp()->basicp()) {
            ಯದಿ (basicp->width() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                ಯದಿ (!V3Error::error_ಎಣಿಕೆ()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->width() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
            ವಸ್ತುಗಳು = 1ULL << basicp->width();
        } ಅನ್ಯಥಾ ಯದಿ (AstStructDType* ಧ್ರುವ dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        } ಅನ್ಯಥಾ {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_ಎಣಿಕೆ(), varp, "Unexpected randc variable dtype");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        AstCDType* ಧ್ರುವ newdtp = findVlRandCDType(varp->fileline(), ವಸ್ತುಗಳು);
        AstVar* ಧ್ರುವ newp
            = ನೂತನ AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->isInternal(ಸತ್ಯ);
        classp->addStmtsp(newp);
        UINFO(9, "created " << varp);
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    Ast_ಗಂಟು_Stmt* createArrayForeachLoop(File_ಸಾಲು* ಧ್ರುವ fl, Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep,
                                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar* ಧ್ರುವ outputVarp) {
        Ast_ಗಂಟು_DType* tempDTypep = dtypep;
        AstVar* randLoopIndxp = nullptr;
        auto createLoopIndex = [&](Ast_ಗಂಟು_DType* tempDTypep) {
            ಯದಿ (VN_IS(tempDTypep, AssocArrayDType)) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                                  VN_AS(tempDTypep, AssocArrayDType)->ಕೀಲಿ_DTypep()};
            }
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                              dtypep->findBasicDType(VBasicDTypeKwd::UINT32)};
        };
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* tempElementp = nullptr;
        ಯಾವತ್ (tempDTypep->isNonPackedArray()) {
            AstVar* ಧ್ರುವ newRandLoopIndxp = createLoopIndex(tempDTypep);
            randLoopIndxp = Ast_ಗಂಟು::addNext(randLoopIndxp, newRandLoopIndxp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ tempExprp = tempElementp ? tempElementp : exprp;
            AstVarRef* ಧ್ರುವ tempRefp = ನೂತನ AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            ಯದಿ (VN_IS(tempDTypep, DynArrayDType)) {
                tempElementp
                    = ನೂತನ AstCMethodHard{fl, tempExprp, VCMethod::ARRAY_AT_WRITE, tempRefp};
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(tempDTypep, UnpackArrayDType)) {
                Ast_ಗಂಟು_Array_DType* ಧ್ರುವ aryDTypep = VN_CAST(tempDTypep, ಗಂಟು_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                tempElementp = ನೂತನ AstArraySel{
                    fl, tempExprp,
                    ನೂತನ AstSel{
                        fl,
                        ನೂತನ AstSub{fl, tempRefp,
                                   ನೂತನ AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        ನೂತನ AstConst{fl, 0}, V3_ಸಂಖ್ಯೆ::log2b(aryDTypep->hi()) + 1}};
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(tempDTypep, AssocArrayDType)) {
                tempElementp = ನೂತನ AstAssocSel{fl, tempExprp, tempRefp};
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(tempDTypep, QueueDType)) {
                tempElementp
                    = ನೂತನ AstCMethodHard{fl, tempExprp, VCMethod::DYN_AT_WRITE_APPEND, tempRefp};
            }
            tempElementp->dtypep(tempDTypep->subDTypep());
            tempDTypep = tempDTypep->virtRefDTypep();
        }

        AstForeachHeader* ಧ್ರುವ headerp
            = ನೂತನ AstForeachHeader{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), randLoopIndxp};
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ randStmtsp = newRandStmtsp(fl, tempElementp, nullptr, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        ಯದಿ (!tempElementp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(tempElementp), tempElementp);
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstForeach{fl, headerp, randStmtsp};
    }
    Ast_ಗಂಟು_Stmt* newRandStmtsp(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar* randcVarp,
                               AstVar* ಧ್ರುವ outputVarp, ಪೂರ್ಣಾಙ್ಕ offset = 0,
                               AstMemberDType* memberp = nullptr) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_ಗಂಟು_Stmt* stmtsp = nullptr;
            ಯದಿ (structDtp->packed()) offset += memberp ? memberp->lsb() : 0;
            ಪ್ರತಿ (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_ಗಂಟು_Stmt* randp = nullptr;
                ಯದಿ (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ) : exprp, nullptr,
                                          outputVarp, offset, smemberp);
                } ಅನ್ಯಥಾ ಯದಿ (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = ನೂತನ AstStructSel{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), smemberp->name()};
                    structSelp->dtypep(smemberp->childDTypep());
                    ಯದಿ (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, nullptr, outputVarp);
                }
                stmtsp = Ast_ಗಂಟು::addNextNull(stmtsp, randp);
            }
            ಪ್ರತ್ಯಾಗಮ stmtsp;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            ಯದಿ (!unionDtp->packed()) ಪ್ರತ್ಯಾಗಮ nullptr;
            AstMemberDType* ಧ್ರುವ firstMemberp = unionDtp->membersp();
            ಪ್ರತ್ಯಾಗಮ newRandStmtsp(fl, exprp, nullptr, outputVarp, offset, firstMemberp);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ಧ್ರುವ memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            AstMethodCall* ಧ್ರುವ callp = ನೂತನ AstMethodCall{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            AstAssign* ಧ್ರುವ assignp = ನೂತನ AstAssign{
                fl, ನೂತನ AstVarRef{fl, outputVarp, VAccess::WRITE},
                ನೂತನ AstAnd{fl, ನೂತನ AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstIf{
                fl, ನೂತನ AstNeq{fl, exprp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ನೂತನ AstConst{fl, AstConst::Null{}}},
                assignp};
        } ಅನ್ಯಥಾ ಯದಿ (AstDynArrayDType* ಧ್ರುವ dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            ಪ್ರತ್ಯಾಗಮ createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } ಅನ್ಯಥಾ ಯದಿ (AstQueueDType* ಧ್ರುವ queueDtp = VN_CAST(memberDtp, QueueDType)) {
            ಪ್ರತ್ಯಾಗಮ createArrayForeachLoop(fl, queueDtp, exprp, outputVarp);
        } ಅನ್ಯಥಾ ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            ಪ್ರತ್ಯಾಗಮ createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } ಅನ್ಯಥಾ ಯದಿ (AstAssocArrayDType* ಧ್ರುವ assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            ಪ್ರತ್ಯಾಗಮ createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valp;
            ಯದಿ (AstEnumDType* ಧ್ರುವ enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ಧ್ರುವ tabRefp = enum_ಮೌಲ್ಯ_Tab_Refp(enumDtp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ randp
                    = new_Rand_ಮೌಲ್ಯ(fl, randcVarp, exprp->findBasicDType(VBasicDTypeKwd::UINT32));
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ moddivp = ನೂತನ AstModDiv{
                    fl, randp, ನೂತನ AstConst{fl, static_cast<uint32_t>(enumDtp->item_ಎಣಿಕೆ())}};
                moddivp->dtypep(enumDtp);
                valp = ನೂತನ AstArraySel{fl, tabRefp, moddivp};
            } ಅನ್ಯಥಾ {
                valp
                    = new_Rand_ಮೌಲ್ಯ(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            AstAssign* assignp
                = ನೂತನ AstAssign{fl,
                                ನೂತನ AstSel{fl, exprp, offset + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->width() : exprp->width()},
                                valp};
            AstVar* varp = nullptr;
            exprp->exists([&](ಧ್ರುವ AstVarRef* varrefp) {
                ಯದಿ (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                ಪ್ರತ್ಯಾಗಮ varp != nullptr;
            });
            ಪ್ರತ್ಯಾಗಮ wrapIfRandMode(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Rand_ಮೌಲ್ಯ(File_ಸಾಲು* ಧ್ರುವ fl, AstVar* ಧ್ರುವ randcVarp,
                              Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        ಯದಿ (randcVarp) {
            AstCExpr* ಧ್ರುವ cexprp = ನೂತನ AstCExpr{fl};
            cexprp->add(ನೂತನ AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->add(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            ಪ್ರತ್ಯಾಗಮ cexprp;
        }

        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* findPrePostTask(AstClass* classp, ಧ್ರುವ string& name) {
        ಪ್ರತಿ (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            ಯದಿ (AstTask* ಧ್ರುವ taskp = VN_CAST(m_memberMap.findMember(cp, name), Task)) {
                ಪ್ರತ್ಯಾಗಮ taskp;
            }
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ addPrePostCall(AstClass* ಧ್ರುವ classp, Ast_ಗಂಟು_FTask* ಧ್ರುವ funcp, ಧ್ರುವ string& name) {
        ಯದಿ (AstTask* ಧ್ರುವ userFuncp = findPrePostTask(classp, name)) {
            AstTaskRef* ಧ್ರುವ callp = ನೂತನ AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->addStmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* ಧ್ರುವ classp, ಧ್ರುವ string& which) {
        ಧ್ರುವ string name = "__V" + which;
        ಯದಿ (AstTask* ಧ್ರುವ existingp = VN_CAST(m_memberMap.findMember(classp, name), Task)) {
            ಪ್ರತ್ಯಾಗಮ existingp;
        }
        AstTask* ಧ್ರುವ taskp = ನೂತನ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(ಸತ್ಯ);
        taskp->isVirtual(classp->isExtended());
        classp->addMembersp(taskp);
        m_memberMap.ಸೇರಿಸು(classp, taskp);
        addPrePostCall(classp, taskp, which);
        ಪ್ರತ್ಯಾಗಮ taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ> buildPrePostVirtualWrappers(AstClass* ಧ್ರುವ classp) {
        ಧ್ರುವ auto cachedIt = m_prePostWrap.find(classp);
        ಯದಿ (cachedIt != m_prePostWrap.end()) ಪ್ರತ್ಯಾಗಮ cachedIt->second;
        std::vector<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            ಯದಿ (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.ತಳ್ಳು_back(subp);
        });
        ತಾರ್ಕಿಕ hasPre = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ hasPost = ಅಸತ್ಯ;
        ಪ್ರತಿ (AstClass* ಧ್ರುವ cp : hierp) {
            ಯದಿ (findPrePostTask(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = ಸತ್ಯ;
            }
            ಯದಿ (findPrePostTask(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = ಸತ್ಯ;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        ಯದಿ (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        ಯದಿ (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ಧ್ರುವ std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ> result{hasPre, hasPost};
        m_prePostWrap.emplace(classp, result);
        ಪ್ರತ್ಯಾಗಮ result;
    }
    ಶೂನ್ಯ addVirtualPrePostCall(AstFunc* ಧ್ರುವ randomizeFuncp, AstClass* ಧ್ರುವ classp,
                               ಧ್ರುವ string& which) {
        File_ಸಾಲು* ಧ್ರುವ fl = classp->fileline();
        AstTask* ಧ್ರುವ wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ಧ್ರುವ refDTypep = ನೂತನ AstClassRefDType{fl, classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
        AstMethodCall* ಧ್ರುವ callp
            = ನೂತನ AstMethodCall{fl, ನೂತನ AstThisRef{fl, refDTypep}, wrapperp->name(), nullptr};
        callp->taskp(wrapperp);
        callp->dtypeSetVoid();
        randomizeFuncp->addStmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    ತಾರ್ಕಿಕ classHasRandClassMembers(AstClass* classp) {
        ಪ್ರತ್ಯಾಗಮ classp->existsMember([](ಧ್ರುವ AstClass*, ಧ್ರುವ AstVar* varp) {
            ಯದಿ (!varp->rand().isRandomizable()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->dtypep()->skipRefp();
            ಪ್ರತ್ಯಾಗಮ VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    ತಾರ್ಕಿಕ classOwnsGlobalConstraint(ಧ್ರುವ AstClass* classp) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ classp->existsMember([](ಧ್ರುವ AstClass*, ಧ್ರುವ AstConstraint* constrp) {
            ತಾರ್ಕಿಕ owns = ಅಸತ್ಯ;
            constrp->foreach([&](ಧ್ರುವ AstMemberSel* memberSelp) {
                ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ rootp
                    = const_cast<AstMemberSel*>(memberSelp)->baseFromp(ಸತ್ಯ);
                ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    ಯಾವತ್ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    ಯದಿ (VN_IS(dtypep, ClassRefDType)) owns = ಸತ್ಯ;
                }
            });
            ಪ್ರತ್ಯಾಗಮ owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ಧ್ರುವ string& suffix) {
        ಧ್ರುವ string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.findMember(classp, name), Task);
        ಯದಿ (taskp) ಪ್ರತ್ಯಾಗಮ taskp;
        taskp = ನೂತನ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(ಸತ್ಯ);
        classp->addMembersp(taskp);
        m_memberMap.ಸೇರಿಸು(classp, taskp);
        ಪ್ರತ್ಯಾಗಮ taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    ಶೂನ್ಯ populateNestedCallbackTask(AstTask* ಧ್ರುವ callbackTaskp, AstClass* ಧ್ರುವ classp,
                                    ಧ್ರುವ string& cbName) {
        File_ಸಾಲು* ಧ್ರುವ fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            ಯದಿ (!memberVarp->rand().isRandomizable()) ಪ್ರತ್ಯಾಗಮ;
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = memberVarp->dtypep()->skipRefp();
            ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(dtypep, ClassRefDType);
            ಯದಿ (!classRefp) ಪ್ರತ್ಯಾಗಮ;
            AstClass* ಧ್ರುವ memberClassp = classRefp->classp();
            ಯದಿ (memberClassp == classp) ಪ್ರತ್ಯಾಗಮ;  // Avoid self-reference

            // Force-visit member class if not yet processed
            ಯದಿ (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_ಗಂಟು* stmtsp = nullptr;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            ಯದಿ (AstTask* ಧ್ರುವ userFuncp = findPrePostTask(memberClassp, cbName)) {
                AstMethodCall* ಧ್ರುವ callp = ನೂತನ AstMethodCall{
                    fl, ನೂತನ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtypeSetVoid();
                stmtsp = Ast_ಗಂಟು::addNext(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            ಯದಿ (classHasRandClassMembers(memberClassp)) {
                ಧ್ರುವ string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ಧ್ರುವ nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                AstMethodCall* ಧ್ರುವ recurseCallp = ನೂತನ AstMethodCall{
                    fl, ನೂತನ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtypeSetVoid();
                stmtsp = Ast_ಗಂಟು::addNext(stmtsp, recurseCallp->makeStmt());
            }

            ಯದಿ (!stmtsp) ಪ್ರತ್ಯಾಗಮ;

            // Wrap in null check
            AstIf* ಧ್ರುವ nullCheckp = ನೂತನ AstIf{
                fl,
                ನೂತನ AstNeq{fl, ನೂತನ AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           ನೂತನ AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->addStmtsp(wrapIfRandMode(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* newSetupConstraintTask(AstClass* ಧ್ರುವ nodep, ಧ್ರುವ std::string& name) {
        AstTask* ಧ್ರುವ taskp = ನೂತನ AstTask{nodep->fileline(), name + "_setup_constraint", nullptr};
        taskp->classMethod(ಸತ್ಯ);
        nodep->addMembersp(taskp);
        ಪ್ರತ್ಯಾಗಮ taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ಧ್ರುವ nodep, ಧ್ರುವ std::string& name) {
        AstTask* ಧ್ರುವ taskp
            = ನೂತನ AstTask{nodep->fileline(), name + "_resize_constrained_array", nullptr};
        taskp->classMethod(ಸತ್ಯ);
        nodep->addMembersp(taskp);
        ಪ್ರತ್ಯಾಗಮ taskp;
    }
    Ast_ಗಂಟು_Stmt* implementConstraintsClear(File_ಸಾಲು* ಧ್ರುವ fileline, AstVar* ಧ್ರುವ genp) {
        AstCMethodHard* ಧ್ರುವ clearp = ನೂತನ AstCMethodHard{
            fileline,
            ನೂತನ AstVarRef{fileline, VN_AS(genp->user2p(), ಗಂಟು_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtypeSetVoid();
        ಪ್ರತ್ಯಾಗಮ clearp->makeStmt();
    }
    Ast_ಗಂಟು_Stmt* implementConstraintsClearAll(File_ಸಾಲು* ಧ್ರುವ fileline, AstVar* ಧ್ರುವ genp) {
        AstCMethodHard* ಧ್ರುವ clearp = ನೂತನ AstCMethodHard{
            fileline,
            ನೂತನ AstVarRef{fileline, VN_AS(genp->user2p(), ಗಂಟು_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtypeSetVoid();
        ಪ್ರತ್ಯಾಗಮ clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp) {
        ಯದಿ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(exprp, MemberSel)) {
            ಪ್ರತ್ಯಾಗಮ memberSelp->varp();
        } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(exprp, VarRef)) {
            ಪ್ರತ್ಯಾಗಮ varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        ಪ್ರತ್ಯಾಗಮ nullptr;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* makeModeVarRef(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp, AstVar* ಧ್ರುವ modeVarp,
                                ಧ್ರುವ VAccess access) {
        ಯದಿ (modeVarp->lifetime().isStatic()) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), ಗಂಟು_Module),
                                 modeVarp, access};
        }
        ಯದಿ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ಧ್ರುವ newMemberSelp = ನೂತನ AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), modeVarp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            ಪ್ರತ್ಯಾಗಮ newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), Class), modeVarp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* createOrGetSizeVar(AstClass* ಧ್ರುವ classp, AstVar* ಧ್ರುವ arrVarp, File_ಸಾಲು* ಧ್ರುವ fl,
                               Ast_ಗಂಟು_DType* ಧ್ರುವ signed32DTypep, ತಾರ್ಕಿಕ& wasCreated) {
        AstVar* sizeVarp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = ಅಸತ್ಯ;
        ಯದಿ (!sizeVarp) {
            sizeVarp = ನೂತನ AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->addMembersp(sizeVarp);
            m_memberMap.ಸೇರಿಸು(classp, sizeVarp);
            sizeVarp->user2p(classp);
            arrVarp->user4p(sizeVarp);
            wasCreated = ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ sizeVarp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* createSizeGteZeroConstraint(File_ಸಾಲು* ಧ್ರುವ fl, AstVar* ಧ್ರುವ sizeVarp,
                                                   Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* derefFromp = nullptr) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* size_ಗಂಟು_Exprp = nullptr;
        ಯದಿ (derefFromp) {
            size_ಗಂಟು_Exprp = buildMemberSelExprp(derefFromp, sizeVarp);
        } ಅನ್ಯಥಾ {
            size_ಗಂಟು_Exprp = ನೂತನ AstVarRef{fl, sizeVarp, VAccess::READ};
        }
        size_ಗಂಟು_Exprp->user1(ಸತ್ಯ);
        AstGteS* ಧ್ರುವ sizeGtep = ನೂತನ AstGteS{fl, size_ಗಂಟು_Exprp, ನೂತನ AstConst{fl, 0}};
        sizeGtep->user1(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, sizeGtep};
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getFromp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp) {
        ಯದಿ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(exprp, MemberSel)) {
            ಪ್ರತ್ಯಾಗಮ memberSelp->fromp();
        } ಅನ್ಯಥಾ ಯದಿ (AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(exprp, MethodCall)) {
            ಪ್ರತ್ಯಾಗಮ methodCallp->fromp();
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    AstVar* makeTmpRandModeVar(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* siblingExprp, AstVar* randModeVarp,
                               Ast_ಗಂಟು*& storeStmtspr, Ast_ಗಂಟು_Stmt*& restoreStmtspr) {
        File_ಸಾಲು* ಧ್ರುವ fl = randModeVarp->fileline();
        AstVar* ಧ್ರುವ randModeTmpVarp = ನೂತನ AstVar{
            fl, VVarType::BLOCKTEMP, m_modeUniqueNames.get(randModeVarp), randModeVarp->dtypep()};
        randModeTmpVarp->funcLocal(m_ftaskp);
        randModeTmpVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_ಗಂಟು::addNext(
            storeStmtspr,
            ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, randModeTmpVarp, VAccess::WRITE},
                          makeModeVarRef(siblingExprp, randModeVarp, VAccess::READ)});
        storeStmtspr = Ast_ಗಂಟು::addNext(
            storeStmtspr,
            makeModeSetLoop(fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                            ನೂತನ AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_ಗಂಟು::addNext(
            restoreStmtspr,
            ನೂತನ AstAssign{fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                          ನೂತನ AstVarRef{fl, randModeTmpVarp, VAccess::READ}});
        ಪ್ರತ್ಯಾಗಮ randModeTmpVarp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sliceToCommonPrefix(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thisp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* otherp) {
        ಸ್ಥಿರ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        ಯಾವತ್ (thisp) {
            thisHier.ತಳ್ಳು_back(thisp);
            thisp = getFromp(thisp);
        }
        ಯಾವತ್ (otherp) {
            otherHier.ತಳ್ಳು_back(otherp);
            otherp = getFromp(otherp);
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* commonp = nullptr;
        ಪ್ರತಿ (auto thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            ಯದಿ ((*thisIt)->type() != (*otherIt)->type()) ಭಙ್ಗ;
            ಯದಿ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ಧ್ರುವ otherMemberSelp = VN_AS(*otherIt, MemberSel);
                ಯದಿ (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    ಅನುವರ್ತ;
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstMethodCall* ಧ್ರುವ thisMethodCallp = VN_CAST(*thisIt, MethodCall)) {
                AstMethodCall* ಧ್ರುವ otherMethodCallp = VN_AS(*otherIt, MethodCall);
                ಯದಿ (thisMethodCallp->taskp() == otherMethodCallp->taskp()) {
                    commonp = thisMethodCallp;
                    ಅನುವರ್ತ;
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ firstVarRefp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ಧ್ರುವ secondVarRefp = VN_AS(*otherIt, VarRef);
                ಯದಿ (firstVarRefp->varp() == secondVarRefp->varp()) {
                    commonp = firstVarRefp;
                    ಅನುವರ್ತ;
                }
            }
            ಭಙ್ಗ;
        }
        ಪ್ರತ್ಯಾಗಮ commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildMemberSelExprp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar* deepestVarp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resExprp = nullptr;
        ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), nullptr);
            AstVar* varp = memberSelp->varp();
            ಯದಿ (deepestVarp) varp = deepestVarp;
            resExprp = ನೂತನ AstMemberSel{exprp->fileline(), resExprp, varp};
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            ಯದಿ (deepestVarp) varp = deepestVarp;
            resExprp = ನೂತನ AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), ಗಂಟು_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ resExprp;
    }

    ಶೂನ್ಯ addUpdateRandVarsBody(AstClass* ಧ್ರುವ nodep, AstFunc* ಧ್ರುವ randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ಧ್ರುವ updatep
            = ನೂತನ AstFunc{nodep->fileline(), "__VupdateRandVars", nullptr, nullptr};
        updatep->classMethod(ಸತ್ಯ);
        updatep->isVirtual(ಸತ್ಯ);
        updatep->keepAlive(ಸತ್ಯ);
        updatep->needsSyms(ಅಸತ್ಯ);
        nodep->addMembersp(updatep);
        nodep->hasRandVarsUpdate(ಸತ್ಯ);

        ಪ್ರತಿ (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            Ast_ಗಂಟು_FTask* ಧ್ರುವ newp = VN_AS(m_memberMap.findMember(classp, "new"), ಗಂಟು_FTask);
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ stmtExprp = VN_CAST(stmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ);
                ಯದಿ (!stmtExprp) ಅನುವರ್ತ;
                AstCMethodHard* ಧ್ರುವ methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                ಯದಿ (!methodp || methodp->method() != VCMethod::RANDOMIZER_WRITE_VAR) ಅನುವರ್ತ;

                Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varRefp = VN_AS(methodp->pinsp(), ಗಂಟು_Var_Ref);
                ಯದಿ (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    ಅನುವರ್ತ;
                }
                AstCMethodHard* ಧ್ರುವ updateMethodp = ನೂತನ AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                updateMethodp->dtypeSetVoid();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ namep = VN_AS(varRefp->nextp()->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                updateMethodp->addPinsp(namep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ));
                updateMethodp->method(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->addStmtsp(updateMethodp->makeStmt());
            }
        }
    }

    ಶೂನ್ಯ addBasicRandomizeBody(AstFunc* ಧ್ರುವ basicRandomizep, AstClass* ಧ್ರುವ nodep,
                               AstVar* randModeVarp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        AstVar* ಧ್ರುವ basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ಧ್ರುವ basicFvarRefp = ನೂತನ AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ಧ್ರುವ beginBasicValp = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1};
        basicRandomizep->addStmtsp(ನೂತನ AstAssign{fl, basicFvarRefp, beginBasicValp});
        Ast_ಗಂಟು_FTask* ಧ್ರುವ newp = VN_AS(m_memberMap.findMember(nodep, "new"), ಗಂಟು_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            ಯದಿ (!memberVarp->rand().isRandomizable()) ಪ್ರತ್ಯಾಗಮ;
            ಧ್ರುವ RandomizeMode randMode = {.asUQuad = memberVarp->user1()};
            ಯದಿ (randMode.usesMode
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ಧ್ರುವ setp = ನೂತನ AstCMethodHard{
                    nodep->fileline(),
                    ನೂತನ AstVarRef{fl, VN_AS(randModeVarp->user2p(), ಗಂಟು_Module), randModeVarp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, ನೂತನ AstConst{nodep->fileline(), randMode.index}};
                setp->dtypeSetUInt32();
                newp->addStmtsp(ನೂತನ AstAssign{fl, setp, ನೂತನ AstConst{fl, 0}});
            }
            ಯದಿ (memberVarp->user3()) ಪ್ರತ್ಯಾಗಮ;  // Handled in constraints
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = memberVarp->dtypep()->skipRefp();
            ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                ಯದಿ (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    ಪ್ರತ್ಯಾಗಮ;
                }
                AstFunc* ಧ್ರುವ memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                AstMethodCall* ಧ್ರುವ callp
                    = memberVarp->globalConstrained()
                          ? ನೂತನ AstMethodCall{fl,
                                              ನೂತನ AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : ನೂತನ AstMethodCall{
                                fl, ನೂತನ AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ಧ್ರುವ basicFvarRefReadp = basicFvarRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ಧ್ರುವ assignIfNotNullp = ನೂತನ AstIf{
                    fl,
                    ನೂತನ AstNeq{fl, ನೂತನ AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               ನೂತನ AstConst{fl, AstConst::Null{}}},
                    ನೂತನ AstAssign{fl, basicFvarRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                  ನೂತನ AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->addStmtsp(wrapIfRandMode(nodep, memberVarp, assignIfNotNullp));
            } ಅನ್ಯಥಾ {
                AstVar* ಧ್ರುವ randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ಧ್ರುವ refp
                    = ನೂತನ AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_ಗಂಟು_Stmt* ಧ್ರುವ stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                ಯದಿ (!refp->backp()) VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
                basicRandomizep->addStmtsp(
                    ನೂತನ AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtp, ಅಸತ್ಯ});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* makeModeAssignLhs(File_ಸಾಲು* ಧ್ರುವ fl, AstClass* ಧ್ರುವ classp,
                                   Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp, AstVar* ಧ್ರುವ modeVarp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        ಯದಿ (modeVarp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            ಯದಿ (fromp) VL_DO_DANGLING(fromp->unlinkFrBack()->delete_ವೃಕ್ಷ(), fromp);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, VN_AS(modeVarp->user2p(), ಗಂಟು_Module), modeVarp,
                                 VAccess::WRITE};
        } ಅನ್ಯಥಾ ಯದಿ (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstVarRef{fl, VN_AS(modeVarp->user2p(), ಗಂಟು_Module), modeVarp,
                                 VAccess::WRITE};
        } ಅನ್ಯಥಾ {
            AstMemberSel* ಧ್ರುವ memberselp = ನೂತನ AstMemberSel{fl, fromp->unlinkFrBack(), modeVarp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            ಪ್ರತ್ಯಾಗಮ memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    ಶೂನ್ಯ replaceWithModeAssign(Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ ftaskRefp, Ast_ಗಂಟು* ಧ್ರುವ receiverp,
                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp) {
        replaceWithModeAssignAndAppend(ftaskRefp, receiverp, lhsp, nullptr);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    ಶೂನ್ಯ replaceWithModeAssignAndAppend(Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ ftaskRefp, Ast_ಗಂಟು* ಧ್ರುವ receiverp,
                                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp, Ast_ಗಂಟು* ಧ್ರುವ appendStmtp) {
        File_ಸಾಲು* ಧ್ರುವ fl = ftaskRefp->fileline();
        ಯದಿ (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ), ftaskRefp, "Should be a statement");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ftaskRefp->argsp()->exprp()->unlinkFrBack();
            Ast_ಗಂಟು* newStmtp = nullptr;
            ಯದಿ (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                // mode
                ಧ್ರುವ RandomizeMode rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ಧ್ರುವ setp = ನೂತನ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                ನೂತನ AstConst{fl, rmode.index}};
                setp->dtypeSetUInt32();
                newStmtp = ನೂತನ AstAssign{fl, setp, rhsp};
            } ಅನ್ಯಥಾ {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = makeModeSetLoop(fl, lhsp, rhsp, m_ftaskp);
            }
            ಯದಿ (appendStmtp) newStmtp->addNext(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(ತಳ್ಳು_Deletep(m_stmtp), m_stmtp = nullptr);
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ಧ್ರುವ RandomizeMode rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ಧ್ರುವ setp = ನೂತನ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            ನೂತನ AstConst{fl, rmode.index}};
            setp->dtypeSetUInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    ಶೂನ್ಯ wrapRandomizeCallWithNullGuard(Ast_ಗಂಟು_FTask_Ref* nodep) {
        AstMethodCall* ಧ್ರುವ callp = VN_CAST(nodep, MethodCall);
        ಯದಿ (!callp) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (callp->user4SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        File_ಸಾಲು* ಧ್ರುವ fl = callp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ checkp
            = ನೂತನ AstNeq{fl, callp->fromp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ನೂತನ AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlinkFrBack(&relinker);
        relinker.relink(ನೂತನ AstCond{fl, checkp, callp, ನೂತನ AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    ಶೂನ್ಯ handleRandomizeArgs(Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ nodep) {
        ಯದಿ (!nodep->argsp()) ಪ್ರತ್ಯಾಗಮ;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        ತಾರ್ಕಿಕ hasNullArg = ಅಸತ್ಯ;
        ಪ್ರತಿ (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->ಅಂಕೆ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = ಸತ್ಯ;
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        ಪ್ರತಿ (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ಪ್ರತಿ (AstArg *otherArgp = nextp, *otherNextp = nullptr; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prefixp
                    = sliceToCommonPrefix(argp->exprp(), otherArgp->exprp())) {
                    ಯದಿ (prefixp == argp->exprp()) {
                        ಯದಿ (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlinkFrBack()->delete_ವೃಕ್ಷ(), otherArgp);
                        ಅನುವರ್ತ;
                    }
                }
                ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ prefixp
                    = sliceToCommonPrefix(otherArgp->exprp(), argp->exprp())) {
                    ಯದಿ (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);
                        ಭಙ್ಗ;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::set<AstVar*> savedRandModeVarps;
        AstVar* tmpVarps = nullptr;
        Ast_ಗಂಟು* storeStmtsp = nullptr;
        Ast_ಗಂಟು* setStmtsp = nullptr;
        Ast_ಗಂಟು_Stmt* restoreStmtsp = nullptr;
        ಪ್ರತಿ (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = argp->exprp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ commonPrefixp = sliceToCommonPrefix(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_ಸಾಲು* ಧ್ರುವ fl = argp->fileline();
            ಯಾವತ್ (exprp) {
                ಯದಿ (commonPrefixp == exprp) ಭಙ್ಗ;
                AstVar* ಧ್ರುವ randVarp = getVarFromRef(exprp);
                AstClass* ಧ್ರುವ classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ಧ್ರುವ randModeVarp = randVarp->lifetime().isStatic()
                                                 ? getStaticRandModeVar(classp)
                                                 : getRandModeVarFromClass(classp);
                UASSERT_OBJ(randModeVarp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                ಯದಿ (savedRandModeVarps.find(randModeVarp) == savedRandModeVarps.end()) {
                    AstVar* ಧ್ರುವ randModeTmpVarp
                        = makeTmpRandModeVar(exprp, randModeVarp, storeStmtsp, restoreStmtsp);
                    savedRandModeVarps.ಸೇರಿಸು(randModeVarp);
                    tmpVarps = Ast_ಗಂಟು::addNext(tmpVarps, randModeTmpVarp);
                }
                ಧ್ರುವ RandomizeMode randMode = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = ನೂತನ AstCMethodHard{
                    fl, makeModeVarRef(exprp, randModeVarp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, ನೂತನ AstConst{fl, randMode.index}};
                setp->dtypeSetUInt32();
                setStmtsp
                    = Ast_ಗಂಟು::addNext(setStmtsp, ನೂತನ AstAssign{fl, setp, ನೂತನ AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);
        }
        ಯದಿ (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* targetClassp = nullptr;
            AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(nodep, MethodCall);
            ಯದಿ (methodCallp) {
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDTypep = methodCallp->fromp()->dtypep();
                ಧ್ರುವ AstClassRefDType* ಧ್ರುವ crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                targetClassp = crdtp->classp();
            } ಅನ್ಯಥಾ {
                targetClassp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(targetClassp, nodep, "randomize(null) target class unresolved");
            AstFunc* ಧ್ರುವ checkOnlyFuncp = getCreateRandomizeNullFunc(targetClassp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        ಯದಿ (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && setStmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlinkFrBack(&relinker);
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = storeStmtsp;
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
    AstFunc* getCreateRandomizeNullFunc(AstClass* ಧ್ರುವ classp) {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ name = "__Vrandomize_null";
        ಯದಿ (AstFunc* ಧ್ರುವ existingp = VN_AS(m_memberMap.findMember(classp, name), Func)) {
            ಪ್ರತ್ಯಾಗಮ existingp;
        }
        File_ಸಾಲು* ಧ್ರುವ fl = classp->fileline();
        AstFunc* ಧ್ರುವ funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ಧ್ರುವ fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        addPrePostCall(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ಧ್ರುವ classGenp = getRandomGenerator(classp);
        ಯದಿ (!classGenp) {
            funcp->addStmtsp(ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, fvarp, VAccess::WRITE},
                                           ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}});
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_Module* ಧ್ರುವ genModp = VN_AS(classGenp->user2p(), ಗಂಟು_Module);
            funcp->addStmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ಧ್ರುವ setupAllTaskp = getCreateConstraintSetupFunc(classp, m_memberMap);
            funcp->addStmtsp((ನೂತನ AstTaskRef{fl, setupAllTaskp})->makeStmt());
            AstCExpr* ಧ್ರುವ solverCallp = ನೂತನ AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(ನೂತನ AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->add(".next_check_only(__Vm_rng)");
            funcp->addStmtsp(
                ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(ಸತ್ಯ);
        }

        // 3. post_randomize -- only if result is non-zero
        ಯದಿ (AstTask* ಧ್ರುವ userPostp = findPrePostTask(classp, "post_randomize")) {
            AstTaskRef* ಧ್ರುವ callp = ನೂತನ AstTaskRef{userPostp->fileline(), userPostp};
            funcp->addStmtsp(
                ನೂತನ AstIf{fl, ನೂತನ AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        ಪ್ರತ್ಯಾಗಮ funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* liftLogIfChainToConstraintIf(AstLogIf* logIfp) {
        File_ಸಾಲು* ಧ್ರುವ fl = logIfp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = logIfp->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = logIfp->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು* thenBodyp;
        ಯದಿ (AstLogIf* ಧ್ರುವ innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = liftLogIfChainToConstraintIf(innerLogIfp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(innerLogIfp), innerLogIfp);
        } ಅನ್ಯಥಾ {
            thenBodyp = ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, rhsp};
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConstraintIf{fl, condp, thenBodyp, nullptr};
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ distBoundRefsRandVar(ಧ್ರುವ Ast_ಗಂಟು* boundp) {
        ಪ್ರತ್ಯಾಗಮ boundp->exists(
            [](ಧ್ರುವ AstVarRef* vrefp) { ಪ್ರತ್ಯಾಗಮ vrefp->varp()->rand().isRandomizable(); });
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ distBoundRefsModeVar(ಧ್ರುವ Ast_ಗಂಟು* boundp) {
        ಪ್ರತ್ಯಾಗಮ boundp->exists([](ಧ್ರುವ AstVarRef* vrefp) {
            ಯದಿ (!vrefp->varp()->rand().isRandomizable()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಧ್ರುವ RandomizeMode rmode = {.asUQuad = vrefp->varp()->user1()};
            ಪ್ರತ್ಯಾಗಮ rmode.usesMode;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newDistRangeMembership(AstDist* distp, ಧ್ರುವ AstInsideRange* irp) {
        File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();
        ಧ್ರುವ ತಾರ್ಕಿಕ isSigned = distp->exprp()->isSigned();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dist_ಅಭಿವ್ಯಕ್ತಿ_Gtep = distp->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dist_ಅಭಿವ್ಯಕ್ತಿ_Ltep = distp->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        dist_ಅಭಿವ್ಯಕ್ತಿ_Gtep->user1(ಸತ್ಯ);
        dist_ಅಭಿವ್ಯಕ್ತಿ_Ltep->user1(ಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ gep
            = isSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                             ನೂತನ AstGteS{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Gtep, irp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)})
                       : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                             ನೂತನ AstGte{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Gtep, irp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)});
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lep
            = isSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                             ನೂತನ AstLteS{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Ltep, irp->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)})
                       : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                             ನೂತನ AstLte{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Ltep, irp->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)});
        gep->user1(ಸತ್ಯ);
        lep->user1(ಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ andp = ನೂತನ AstLogAnd{fl, gep, lep};
        andp->user1(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ andp;
    }

    ಸಂರಚನಾ DistBucket final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangep;  // A single value or an InsideRange
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightExprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::vector<DistBucket> collectDistBuckets(AstDist* ಧ್ರುವ distp) {
        File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();
        std::vector<DistBucket> buckets;
        ಪ್ರತಿ (AstDistItem* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), DistItem)) {
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(ditemp->weightp(), Const)) {
                ಯದಿ (constp->toUQuad() == 0) ಅನುವರ್ತ;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* weightExprp
                = ನೂತನ AstExtend{fl, ditemp->weightp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 64};
            // := on a range weights every element, so scale by the range size.
            ಯದಿ (!ditemp->isWhole()) {
                ಯದಿ (ಧ್ರುವ AstInsideRange* ಧ್ರುವ irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ಧ್ರುವ AstConst* ಧ್ರುವ lop = VN_CAST(irp->lhsp(), Const);
                    ಧ್ರುವ AstConst* ಧ್ರುವ hip = VN_CAST(irp->rhsp(), Const);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangeSizep;
                    ಯದಿ (lop && hip) {
                        ಧ್ರುವ uint64_t rangeSize = hip->toUQuad() - lop->toUQuad() + 1;
                        rangeSizep = ನೂತನ AstConst{fl, AstConst::Unsized64{}, rangeSize};
                    } ಅನ್ಯಥಾ {
                        rangeSizep = ನೂತನ AstAdd{
                            fl, ನೂತನ AstConst{fl, AstConst::Unsized64{}, 1},
                            ನೂತನ AstSub{fl,
                                       ನೂತನ AstExtend{fl, irp->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 64},
                                       ನೂತನ AstExtend{fl, irp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 64}}};
                    }
                    weightExprp = ನೂತನ AstMul{fl, weightExprp, rangeSizep};
                }
            }
            buckets.ತಳ್ಳು_back({ditemp->rangep(), weightExprp});
        }
        ಪ್ರತ್ಯಾಗಮ buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newDistMembershipTerm(AstDist* distp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangep) {
        ಯದಿ (ಧ್ರುವ AstInsideRange* ಧ್ರುವ irp = VN_CAST(rangep, InsideRange)) {
            ಪ್ರತ್ಯಾಗಮ newDistRangeMembership(distp, irp);
        }
        File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eqExprp = distp->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        eqExprp->user1(ಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eqp = ನೂತನ AstEq{fl, eqExprp, rangep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
        eqp->user1(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* buildDistMembership(AstDist* distp,
                                           ಧ್ರುವ std::vector<DistBucket>& buckets) {
        File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* unionExprp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ auto& bucket : buckets) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ memberp = newDistMembershipTerm(distp, bucket.rangep);
            ಯದಿ (!unionExprp) {
                unionExprp = memberp;
            } ಅನ್ಯಥಾ {
                unionExprp = ನೂತನ AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(ಸತ್ಯ);
            }
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newGateJoin(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp, ತಾರ್ಕಿಕ isAnd) {
        ಯದಿ (ap->same_ವೃಕ್ಷ(bp)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(bp), bp);
            ಪ್ರತ್ಯಾಗಮ ap;
        }
        ಯದಿ (VN_IS(bp, Const)) std::swap(ap, bp);
        ಯದಿ (VN_IS(ap, Const)) {
            ಧ್ರುವ ತಾರ್ಕಿಕ constWins = VN_AS(ap, Const)->isZero() == isAnd;
            ಯದಿ (!constWins) std::swap(ap, bp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(bp), bp);
            ಪ್ರತ್ಯಾಗಮ ap;
        }
        ಯದಿ (isAnd) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogAnd{fl, ap, bp};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstLogOr{fl, ap, bp};
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newGateOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) {
        ಪ್ರತ್ಯಾಗಮ newGateJoin(fl, ap, bp, ಅಸತ್ಯ);
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newGateAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) {
        ಪ್ರತ್ಯಾಗಮ newGateJoin(fl, ap, bp, ಸತ್ಯ);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newModeBitRead(AstVar* varp, ಧ್ರುವ AstMemberSel* mselp, AstVar* randModeVarp,
                                File_ಸಾಲು* fl) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arrayp = nullptr;
        ಯದಿ (varp->lifetime().isStatic()) {
            AstClass* ಧ್ರುವ ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ಧ್ರುವ smodep = getStaticRandModeVar(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = ನೂತನ AstVarRef{fl, VN_AS(smodep->user2p(), ಗಂಟು_Module), smodep, VAccess::READ};
        } ಅನ್ಯಥಾ ಯದಿ (mselp) {
            AstVar* ಧ್ರುವ memberModep = getRandModeVarFromClass(VN_AS(varp->user2p(), ಗಂಟು_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = ನೂತನ AstMemberSel{fl, mselp->fromp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), memberModep};
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(randModeVarp, varp, "rand_mode variable without a class mode array");
            arrayp = ನೂತನ AstVarRef{fl, VN_AS(randModeVarp->user2p(), ಗಂಟು_Module), randModeVarp,
                                   VAccess::READ};
        }
        ಧ್ರುವ RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ಧ್ರುವ atp
            = ನೂತನ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, ನೂತನ AstConst{fl, rmode.index}};
        atp->dtypeSetUInt32();
        ಪ್ರತ್ಯಾಗಮ atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newVarGate(AstVar* varp, ಧ್ರುವ AstMemberSel* mselp, ತಾರ್ಕಿಕ ownerLevel,
                            AstVar* randModeVarp, File_ಸಾಲು* fl) {
        ಯದಿ (!varp->rand().isRandomizable()) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitTrue{}, ownerLevel};
        }
        ಧ್ರುವ RandomizeMode rmode = {.asUQuad = varp->user1()};
        ಯದಿ (!rmode.usesMode) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitTrue{}};
        ಪ್ರತ್ಯಾಗಮ newModeBitRead(varp, mselp, randModeVarp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newAccessGate(ಧ್ರುವ Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ ownerLevel, AstVar* randModeVarp,
                               File_ಸಾಲು* fl) {
        ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ mselp = VN_CAST(nodep, MemberSel)) {
            ಪ್ರತ್ಯಾಗಮ newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, randModeVarp, fl),
                              newAccessGate(mselp->fromp(), ಸತ್ಯ, randModeVarp, fl));
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ refp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
            ಪ್ರತ್ಯಾಗಮ newVarGate(refp->varp(), nullptr, ownerLevel, randModeVarp, fl);
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Sel* ಧ್ರುವ selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
            ಪ್ರತ್ಯಾಗಮ newAccessGate(selp->fromp(), ಸತ್ಯ, randModeVarp, fl);
        }
        ಯದಿ (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitTrue{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newDistGate(ಧ್ರುವ Ast_ಗಂಟು* nodep, AstVar* randModeVarp, File_ಸಾಲು* fl) {
        ಯದಿ (VN_IS(nodep, ಗಂಟು_Var_Ref) || VN_IS(nodep, MemberSel)) {
            ಪ್ರತ್ಯಾಗಮ newAccessGate(nodep, ಅಸತ್ಯ, randModeVarp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Sel* ಧ್ರುವ selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
            ಪ್ರತ್ಯಾಗಮ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(nodep, Sel)) {
            ಪ್ರತ್ಯಾಗಮ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ selp = VN_CAST(nodep, StructSel)) {
            ಪ್ರತ್ಯಾಗಮ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        ಯದಿ (ಧ್ರುವ AstCMethodHard* ಧ್ರುವ methodp = VN_CAST(nodep, CMethodHard)) {
            ಯದಿ (methodp->method() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                ಪ್ರತ್ಯಾಗಮ newDistGate(methodp->fromp(), randModeVarp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        ಯದಿ (ಧ್ರುವ AstCond* ಧ್ರುವ condp = VN_CAST(nodep, Cond)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thenGatep = newDistGate(condp->thenp(), randModeVarp, fl);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elseGatep = newDistGate(condp->elsep(), randModeVarp, fl);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* armGatep = thenGatep;
            ಯದಿ (thenGatep->same_ವೃಕ್ಷ(elseGatep)) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(elseGatep), elseGatep);
            } ಅನ್ಯಥಾ {
                armGatep
                    = ನೂತನ AstCond{fl, condp->condp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), thenGatep, elseGatep};
            }
            ಪ್ರತ್ಯಾಗಮ newGateOr(fl, newDistGate(condp->condp(), randModeVarp, fl), armGatep);
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Uniop* ಧ್ರುವ uopp = VN_CAST(nodep, ಗಂಟು_Uniop)) {
            ಪ್ರತ್ಯಾಗಮ newDistGate(uopp->lhsp(), randModeVarp, fl);
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ bopp = VN_CAST(nodep, ಗಂಟು_Biop)) {
            ಪ್ರತ್ಯಾಗಮ newGateOr(fl, newDistGate(bopp->lhsp(), randModeVarp, fl),
                             newDistGate(bopp->rhsp(), randModeVarp, fl));
        }
        ಯದಿ (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{fl, AstConst::BitTrue{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* buildFrozenMembership(AstDist* distp,
                                             ಧ್ರುವ std::vector<DistBucket>& buckets) {
        File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* unionExprp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ auto& bucket : buckets) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* termp = newDistMembershipTerm(distp, bucket.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ಧ್ರುವ ತಾರ್ಕಿಕ runtimeWeight
                = bucket.weightExprp->exists([](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; });
            ಯದಿ (runtimeWeight) {
                AstNeq* ಧ್ರುವ nzp = ನೂತನ AstNeq{fl, bucket.weightExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                                               ನೂತನ AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(ಸತ್ಯ);
                termp = ನೂತನ AstLogAnd{fl, termp, nzp};
                termp->user1(ಸತ್ಯ);
            }
            ಯದಿ (unionExprp) {
                unionExprp = ನೂತನ AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(ಸತ್ಯ);
            } ಅನ್ಯಥಾ {
                unionExprp = termp;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_ಗಂಟು* gateFrozenDist(Ast_ಗಂಟು* chainp, AstDist* distp,
                            ಧ್ರುವ std::vector<DistBucket>& buckets, AstVar* randModeVarp) {
        File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gatep = newDistGate(distp->exprp(), randModeVarp, fl);
        ಯದಿ (VN_IS(gatep, Const)) {
            ಧ್ರುವ ತಾರ್ಕಿಕ drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(gatep), gatep);
            ಯದಿ (drawable) ಪ್ರತ್ಯಾಗಮ chainp;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(chainp), chainp);
            ಪ್ರತ್ಯಾಗಮ buildFrozenMembership(distp, buckets);
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newUniformRangePick(AstDist* distp, ಧ್ರುವ AstInsideRange* irp) {
        File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp = distp->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp->user1(ಸತ್ಯ);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ distWidth = distp->exprp()->width();
        ಧ್ರುವ AstConst* ಧ್ರುವ lopC = VN_CAST(irp->lhsp(), Const);
        ಧ್ರುವ AstConst* ಧ್ರುವ hipC = VN_CAST(irp->rhsp(), Const);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangeSzp;
        ಯದಿ (lopC && hipC) {
            ಧ್ರುವ uint64_t rsz = hipC->toUQuad() - lopC->toUQuad() + 1;
            rangeSzp = ನೂತನ AstConst{fl, AstConst::Unsized64{}, rsz};
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ ತಾರ್ಕಿಕ isSigned = irp->lhsp()->isSigned();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lo64p
                = isSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                 ನೂತನ AstExtendS{fl, irp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 64})
                           : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                 ನೂತನ AstExtend{fl, irp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 64});
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ hi64p
                = isSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                 ನೂತನ AstExtendS{fl, irp->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 64})
                           : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                 ನೂತನ AstExtend{fl, irp->rhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 64});
            rangeSzp = ನೂತನ AstAdd{fl, ನೂತನ AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  ನೂತನ AstSub{fl, hi64p, lo64p}};
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rand64p = ನೂತನ AstRand{fl, nullptr, ಅಸತ್ಯ};
        rand64p->dtypeSetUInt64();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ offsetp
            = ನೂತನ AstCCast{fl, ನೂತನ AstModDiv{fl, rand64p, rangeSzp}, distWidth};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = ನೂತನ AstAdd{fl, irp->lhsp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), offsetp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ eqp = ನೂತನ AstEq{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp, valuep};
        eqp->user1(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_ಗಂಟು* buildWeightedBucketChain(AstDist* distp, ಧ್ರುವ std::vector<DistBucket>& buckets,
                                      AstVar* bucketVarp,
                                      ಧ್ರುವ std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& cumSums) {
        File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* chainp = nullptr;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = static_cast<ಪೂರ್ಣಾಙ್ಕ>(buckets.size()) - 1; i >= 0; --i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* constraintExprp;
            ಧ್ರುವ AstInsideRange* ಧ್ರುವ irp = VN_CAST(buckets[i].rangep, InsideRange);
            ಯದಿ (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } ಅನ್ಯಥಾ ಯದಿ (irp) {
                constraintExprp = newUniformRangePick(distp, irp);
            } ಅನ್ಯಥಾ {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp = distp->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp->user1(ಸತ್ಯ);
                constraintExprp
                    = ನೂತನ AstEq{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp, buckets[i].rangep->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                constraintExprp->user1(ಸತ್ಯ);
            }
            ಯದಿ (!chainp) {
                chainp = constraintExprp;
            } ಅನ್ಯಥಾ {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp
                    = ನೂತನ AstLte{fl, ನೂತನ AstVarRef{fl, bucketVarp, VAccess::READ}, cumSums[i]};
                condp->user1(ಸತ್ಯ);
                chainp = ನೂತನ AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(ಸತ್ಯ);
            }
        }
        Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ chainExprp = ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, chainp};
        chainExprp->isSoft(ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    ಶೂನ್ಯ lowerDistConstraints(AstTask* taskp, Ast_ಗಂಟು* constrItemsp, AstVar* randModeVarp,
                              AstConstraintForeach* foreachp = nullptr) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_ಗಂಟು* foreachTailp = nullptr;
        auto addStmt = [&](Ast_ಗಂಟು* nodep) {
            ಯದಿ (foreachp) {
                ಯದಿ (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } ಅನ್ಯಥಾ {
                    foreachTailp->addNext(nodep);
                    foreachTailp = nodep;
                }
            } ಅನ್ಯಥಾ {
                taskp->addStmtsp(nodep);
            }
        };

        ಪ್ರತಿ (Ast_ಗಂಟು *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            ಯದಿ (AstConstraintIf* ಧ್ರುವ cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                lowerDistConstraints(taskp, cifp->thensp(), randModeVarp, foreachp);
                ಯದಿ (cifp->elsesp()) {
                    lowerDistConstraints(taskp, cifp->elsesp(), randModeVarp, foreachp);
                }
                ಅನುವರ್ತ;
            }
            ಯದಿ (AstConstraintForeach* ಧ್ರುವ cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                lowerDistConstraints(taskp, cfep->bodyp(), randModeVarp, cfep);
                ಅನುವರ್ತ;
            }

            Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ constrExprp = VN_CAST(itemp, Constraint_ಅಭಿವ್ಯಕ್ತಿ);
            ಯದಿ (!constrExprp) ಅನುವರ್ತ;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            ಯದಿ (AstLogIf* ಧ್ರುವ ತುದಿ_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_ಗಂಟು* chainEndp = ತುದಿ_Log_Ifp->rhsp();
                ಯಾವತ್ (AstLogIf* ಧ್ರುವ innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                ಯದಿ (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ಧ್ರುವ liftedp = liftLogIfChainToConstraintIf(ತುದಿ_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(constrExprp), constrExprp);
                    lowerDistConstraints(taskp, liftedp->thensp(), randModeVarp, foreachp);
                    ಅನುವರ್ತ;
                }
            }

            AstDist* ಧ್ರುವ distp = VN_CAST(constrExprp->exprp(), Dist);
            ಯದಿ (!distp) ಅನುವರ್ತ;

            File_ಸಾಲು* ಧ್ರುವ fl = distp->fileline();

            std::vector<DistBucket> buckets = collectDistBuckets(distp);

            ಯದಿ (buckets.empty()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ truep
                    = ನೂತನ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, ನೂತನ AstConst{fl, AstConst::BitTrue{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(constrExprp), constrExprp);
                ಅನುವರ್ತ;
            }

            Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* totalWeightExprp = nullptr;
            ಪ್ರತಿ (auto& bucket : buckets) {
                ಯದಿ (!totalWeightExprp) {
                    totalWeightExprp = bucket.weightExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                } ಅನ್ಯಥಾ {
                    totalWeightExprp = ನೂತನ AstAdd{fl, totalWeightExprp,
                                                  bucket.weightExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    totalWeightExprp->dtypeSetUInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ distId = m_dist_ಅಂಕೆ++;
            ಧ್ರುವ std::string totalName = "__Vdist_total" + cvtToStr(distId);
            AstVar* ಧ್ರುವ totalVarp
                = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, totalName, taskp->findUInt64DType()};
            totalVarp->noSubst(ಸತ್ಯ);
            totalVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            totalVarp->funcLocal(ಸತ್ಯ);
            totalVarp->isInternal(ಸತ್ಯ);
            addStmt(totalVarp);
            addStmt(
                ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, totalVarp, VAccess::WRITE}, totalWeightExprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ಧ್ರುವ std::string bucketName = "__Vdist_bucket" + cvtToStr(distId);
            AstVar* ಧ್ರುವ bucketVarp
                = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, bucketName, taskp->findUInt64DType()};
            bucketVarp->noSubst(ಸತ್ಯ);
            bucketVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            bucketVarp->funcLocal(ಸತ್ಯ);
            bucketVarp->isInternal(ಸತ್ಯ);
            addStmt(bucketVarp);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ randp = ನೂತನ AstRand{fl, nullptr, ಅಸತ್ಯ};
            randp->dtypeSetUInt64();
            addStmt(ನೂತನ AstAssign{
                fl, ನೂತನ AstVarRef{fl, bucketVarp, VAccess::WRITE},
                ನೂತನ AstAdd{
                    fl, ನೂತನ AstConst{fl, AstConst::Unsized64{}, 1},
                    ನೂತನ AstModDiv{fl, randp, ನೂತನ AstVarRef{fl, totalVarp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> cumSums;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* runningSump = nullptr;
            ಪ್ರತಿ (size_t i = 0; i < buckets.size(); ++i) {
                ಯದಿ (!runningSump) {
                    runningSump = buckets[i].weightExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                } ಅನ್ಯಥಾ {
                    runningSump = ನೂತನ AstAdd{fl, runningSump,
                                             buckets[i].weightExprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
                    runningSump->dtypeSetUInt64();
                }
                cumSums.ತಳ್ಳು_back(runningSump->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ));
            }

            Ast_ಗಂಟು* chainp = buildWeightedBucketChain(distp, buckets, bucketVarp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, randModeVarp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->addHereThisAsNext(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            ಪ್ರತಿ (auto& bucket : buckets) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(bucket.weightExprp), bucket.weightExprp);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            ತಳ್ಳು_Deletep(cumSums.back());
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isDynArrOfClassTypeRecurse(ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ refp = dtypep->skipRefp();
        ಯದಿ (VN_IS(refp, DynArrayDType) || VN_IS(refp, QueueDType)) {
            ಪ್ರತ್ಯಾಗಮ isDynArrOfClassTypeRecurse(refp->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(refp, ClassRefDType)) {
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    ಶೂನ್ಯ addArrayElemRefresh(File_ಸಾಲು* ಧ್ರುವ fl, AstVar* ಧ್ರುವ arrVarp, AstFunc* ಧ್ರುವ randomizep,
                             AstVar* ಧ್ರುವ genp) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ genModp = VN_AS(genp->user2p(), ಗಂಟು_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ಧ್ರುವ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(ಅಸತ್ಯ).second;
        ಯದಿ (isDynArrOfClassTypeRecurse(arrVarp->dtypep())) {
            ಯದಿ (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstCMethodHard* ಧ್ರುವ methodp
            = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();

        Ast_ಗಂಟು_Module* ಧ್ರುವ classp = VN_AS(arrVarp->user2p(), ಗಂಟು_Module);
        AstVarRef* ಧ್ರುವ varRefp = ನೂತನ AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->addPinsp(varRefp);

        ಧ್ರುವ size_t width = arrayElementDTypep(arrVarp->dtypep())->width();

        methodp->addPinsp(ನೂತನ AstConst{fl, AstConst::Unsized64{}, width});
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ varnamep
            = ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "\"" + arrVarp->name() + "\"", arrVarp->width()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(ನೂತನ AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ಧ್ರುವ RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        ಯದಿ (rmode.usesMode) {
            methodp->addPinsp(ನೂತನ AstConst{fl, AstConst::Unsized64{}, rmode.index});
        }

        randomizep->addStmtsp(methodp->makeStmt());
    }

    Ast_ಗಂಟು_Stmt* newPinSizeVariable(File_ಸಾಲು* ಧ್ರುವ fl, AstVar* ಧ್ರುವ arrVarp,
                                    AstVar* ಧ್ರುವ genp) {
        Ast_ಗಂಟು_Module* ಧ್ರುವ genModp = VN_AS(genp->user2p(), ಗಂಟು_Module);
        AstVar* ಧ್ರುವ sizeVarp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ಧ್ರುವ pinp
            = ನೂತನ AstCMethodHard{fl, ನೂತನ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtypeSetVoid();
        AstCExpr* ಧ್ರುವ namep = ನೂತನ AstCExpr{fl, AstCExpr::Pure{}, "\"" + sizeVarp->name() + "\""};
        namep->dtypeSetUInt32();
        pinp->addPinsp(namep);
        pinp->addPinsp(
            ನೂತನ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(sizeVarp->width())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ಧ್ರುವ sizeVarRefp = ನೂತನ AstVarRef{fl, sizeVarp, VAccess::READ};
        sizeVarRefp->classOrPackagep(VN_AS(sizeVarp->user2p(), ಗಂಟು_Module));
        pinp->addPinsp(sizeVarRefp);
        ಪ್ರತ್ಯಾಗಮ pinp->makeStmt();
    }

    ಶೂನ್ಯ addFrozenSizePin(File_ಸಾಲು* ಧ್ರುವ fl, AstVar* ಧ್ರುವ arrVarp, AstFunc* ಧ್ರುವ randomizep,
                          AstVar* ಧ್ರುವ genp, AstVar* ಧ್ರುವ randModeVarp) {
        ಧ್ರುವ RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        ಯದಿ (!rmode.usesMode) ಪ್ರತ್ಯಾಗಮ;
        AstVar* ಧ್ರುವ sizeVarp = VN_AS(arrVarp->user4p(), Var);
        Ast_ಗಂಟು_Module* ಧ್ರುವ arrClassp = VN_AS(arrVarp->user2p(), ಗಂಟು_Module);
        Ast_ಗಂಟು_Module* ಧ್ರುವ sizeClassp = VN_AS(sizeVarp->user2p(), ಗಂಟು_Module);
        AstVarRef* ಧ್ರುವ sizeWritep = ನೂತನ AstVarRef{fl, sizeClassp, sizeVarp, VAccess::WRITE};
        ಧ್ರುವ VCMethod sizeMethod = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_SIZE
                                        : VCMethod::DYN_SIZE;
        AstCMethodHard* ಧ್ರುವ currentSizep = ನೂತನ AstCMethodHard{
            fl, ನೂತನ AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, sizeMethod};
        currentSizep->dtypep(sizeVarp->dtypep());
        randomizep->addStmtsp(ನೂತನ AstAssign{fl, sizeWritep, currentSizep});
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ modep = newModeBitRead(arrVarp, nullptr, randModeVarp, fl);
        randomizep->addStmtsp(
            ನೂತನ AstIf{fl, ನೂತನ AstLogNot{fl, modep}, newPinSizeVariable(fl, arrVarp, genp)});
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_ಅಂಕೆ);
        VL_RESTORER_COPY(m_randArrayUniqueNames);
        m_modp = nodep;
        m_rand_Case_ಅಂಕೆ = 0;
        m_randArrayUniqueNames.reset();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_ಅಂಕೆ);
        VL_RESTORER(m_dist_ಅಂಕೆ);
        m_modp = nodep;
        m_rand_Case_ಅಂಕೆ = 0;
        m_dist_ಅಂಕೆ = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        ಯದಿ (!nodep->user1()) ಪ್ರತ್ಯಾಗಮ;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->baseMostClassp()->needRNG(ಸತ್ಯ);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ಧ್ರುವ ತಾರ್ಕಿಕ basicFirst = classOwnsGlobalConstraint(nodep);

        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        AstFunc* ಧ್ರುವ randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ಧ್ರುವ fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ಧ್ರುವ randModeVarp = getRandModeVarFromClass(nodep);
        addPrePostCall(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ಯದಿ (classHasRandClassMembers(nodep)) {
            AstTask* ಧ್ರುವ preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->addStmtsp((ನೂತನ AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* beginValp = nullptr;
        AstVar* genp = getRandomGenerator(nodep);
        ಯದಿ (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ಧ್ರುವ prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ಧ್ರುವ classp, AstConstraint* ಧ್ರುವ constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                ಯದಿ (!taskp) {
                    taskp = newSetupConstraintTask(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                ಯದಿ (AstTask* ಧ್ರುವ resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ಧ್ರುವ resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ಧ್ರುವ resizeTaskRefp
                        = ನೂತನ AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->addStmtsp(resizeTaskRefp->makeStmt());
                }

                ಯದಿ (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                ಯದಿ (constrp->itemsp()) {
                    lowerDistConstraints(taskp, constrp->itemsp(), randModeVarp);
                }
                VInsertionSet<AstVar*>& sizeArrays = m_sizeConstrainedArrays[classp];
                Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), nullptr,
                    genp,          randModeVarp, m_writtenVars,     &m_uniqueConstraintId,
                    m_uniqueNames, randomizep,   &sizeArrays,       prepareArraysTaskp};
                ಯದಿ (constrp->itemsp()) {
                    taskp->addStmtsp(wrapIfConstraintMode(
                        nodep, constrp, constrp->itemsp()->unlinkFrBackWithNext()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ಧ್ರುವ auto constraintDepth = [](ಧ್ರುವ AstConstraint* constrp) {
                    ಪೂರ್ಣಾಙ್ಕ depth = 0;
                    size_t pos = 0;
                    ಯಾವತ್ ((pos = constrp->name().find(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++depth;
                        pos += std::strlen(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    ಪ್ರತ್ಯಾಗಮ depth;
                };
                ಪೂರ್ಣಾಙ್ಕ maxDepth = 0;
                nodep->foreachMember([&](AstClass* ಧ್ರುವ, AstConstraint* ಧ್ರುವ constrp) {
                    maxDepth = std::max(maxDepth, constraintDepth(constrp));
                });
                AstTask* ಧ್ರುವ setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ d = maxDepth; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ಧ್ರುವ classp, AstConstraint* ಧ್ರುವ constrp) {
                            ಯದಿ (constraintDepth(constrp) != d) ಪ್ರತ್ಯಾಗಮ;
                            AstTask* ಧ್ರುವ taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ಧ್ರುವ setupTaskRefp
                                = ನೂತನ AstTaskRef{constrp->fileline(), taskp};
                            setupTaskRefp->classOrPackagep(classp);
                            setupAllTaskp->addStmtsp(setupTaskRefp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertionSet<AstVar*> sizeArrayVars;
            ಯದಿ (nodep->extendsp()) {
                AstClass* parentClassp = nodep->extendsp()->classp();
                ಯಾವತ್ (parentClassp) {
                    ಧ್ರುವ auto sizeArraysIt = m_sizeConstrainedArrays.find(parentClassp);
                    ಯದಿ (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                        ಪ್ರತಿ (AstVar* ಧ್ರುವ arrVarp : sizeArraysIt->second) {
                            sizeArrayVars.ಸೇರಿಸು(arrVarp);
                        }
                    }
                    AstFunc* ಧ್ರುವ parentRandomizep
                        = VN_CAST(m_memberMap.findMember(parentClassp, "randomize"), Func);
                    ಯದಿ (parentRandomizep && parentRandomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = parentRandomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ತಾರ್ಕಿಕ foundClearConstraints = ಅಸತ್ಯ;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                ಯದಿ (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->addStmtsp(stmtp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                                } ಅನ್ಯಥಾ ಯದಿ (methodp->method()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    foundClearConstraints = ಸತ್ಯ;
                                }
                            });
                            ಯದಿ (foundClearConstraints) ಭಙ್ಗ;
                        }
                    }
                    parentClassp
                        = parentClassp->extendsp() ? parentClassp->extendsp()->classp() : nullptr;
                }
            }
            randomizep->addStmtsp(implementConstraintsClear(fl, genp));

            ಧ್ರುವ auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
            ಯದಿ (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                ಪ್ರತಿ (AstVar* ಧ್ರುವ arrVarp : sizeArraysIt->second) sizeArrayVars.ಸೇರಿಸು(arrVarp);
            }
            AstTask* ಧ್ರುವ setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
            ಯದಿ (!sizeArrayVars.empty() && prepareArraysTaskp->stmtsp()) {
                Ast_ಗಂಟು_Stmt* ಧ್ರುವ preparep = (ನೂತನ AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    ತಾರ್ಕಿಕ writesArray = ಅಸತ್ಯ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    ಯದಿ (!writesArray) ಅನುವರ್ತ;
                    stmtp->addHereThisAsNext(preparep);
                    ಭಙ್ಗ;
                }
            }
            AstTaskRef* ಧ್ರುವ setupTaskRefp = ನೂತನ AstTaskRef{fl, setupAllTaskp};
            randomizep->addStmtsp(setupTaskRefp->makeStmt());

            Ast_ಗಂಟು_Module* ಧ್ರುವ genModp = VN_AS(genp->user2p(), ಗಂಟು_Module);

            AstCExpr* ಧ್ರುವ solverCallp = ನೂತನ AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(ನೂತನ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->add(".next(__Vm_rng)");
            ಯದಿ (!sizeArrayVars.empty()) {
                AstVar* ಧ್ರುವ sizeOkVarp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                sizeOkVarp->funcLocal(ಸತ್ಯ);
                randomizep->addStmtsp(sizeOkVarp);

                AstVar* ಧ್ರುವ finalOkVarp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                finalOkVarp->funcLocal(ಸತ್ಯ);
                randomizep->addStmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                ಪ್ರತಿ (AstVar* ಧ್ರುವ arrVarp : sizeArrayVars)
                    addFrozenSizePin(fl, arrVarp, randomizep, genp, randModeVarp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->addStmtsp(
                    ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, sizeOkVarp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                ಯದಿ (AstTask* ಧ್ರುವ resizeAllTaskp
                    = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ಧ್ರುವ resizeTaskRefp = ನೂತನ AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{fl, ನೂತನ AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), nullptr};
                    randomizep->addStmtsp(ifp);
                }

                // Refresh array element tables after resize
                ಪ್ರತಿ (ಧ್ರುವ auto& arrVarp : sizeArrayVars) {
                    addArrayElemRefresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->addStmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ಧ್ರುವ setupTaskRefp2 = ನೂತನ AstTaskRef{fl, setupAllTaskp};
                randomizep->addStmtsp(setupTaskRefp2->makeStmt());

                ಪ್ರತಿ (AstVar* ಧ್ರುವ arrVarp : sizeArrayVars) {
                    randomizep->addStmtsp(newPinSizeVariable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ಧ್ರುವ solverCallp2 = ನೂತನ AstCExpr{fl};
                solverCallp2->dtypeSetBit();
                solverCallp2->add(ನೂತನ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->add(".next(__Vm_rng)");
                randomizep->addStmtsp(ನೂತನ AstAssign{
                    fl, ನೂತನ AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = ನೂತನ AstAnd{fl, ನೂತನ AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                       ನೂತನ AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } ಅನ್ಯಥಾ {
                beginValp = solverCallp;
            }
            ಯದಿ (randModeVarp) {
                Ast_ಗಂಟು_Module* ಧ್ರುವ randModeClassp = VN_AS(randModeVarp->user2p(), Class);
                Ast_ಗಂಟು_FTask* ಧ್ರುವ newp
                    = VN_AS(m_memberMap.findMember(randModeClassp, "new"), ಗಂಟು_FTask);
                UASSERT_OBJ(newp, randModeClassp, "No new() in class");
                addSetRandMode(newp, genp, randModeVarp);
            }
            ಯದಿ (AstVar* ಧ್ರುವ staticRandModeVarp = getStaticRandModeVar(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_ಗಂಟು_FTask* ಧ್ರುವ newp = VN_AS(m_memberMap.findMember(nodep, "new"), ಗಂಟು_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                addSetStaticRandMode(newp, genp, staticRandModeVarp);
            }
        } ಅನ್ಯಥಾ {
            beginValp = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1};
        }

        addUpdateRandVarsBody(nodep, randomizep);

        AstFunc* ಧ್ರುವ basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        addBasicRandomizeBody(basicRandomizep, nodep, randModeVarp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ಧ್ರುವ fvarRefp = ನೂತನ AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->addStmtsp(ನೂತನ AstAssign{
            fl, fvarRefp, basicFirst ? ನೂತನ AstFuncRef{fl, basicRandomizep} : beginValp});

        ಧ್ರುವ auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
        ಧ್ರುವ ತಾರ್ಕಿಕ needsSizePhase
            = sizeArraysIt != m_sizeConstrainedArrays.end() && !sizeArraysIt->second.empty();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ಧ್ರುವ auto emitResizeFallback = [&]() {
            ಯದಿ (needsSizePhase) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (AstTask* ಧ್ರುವ resizeAllTaskp
                = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ಧ್ರುವ resizeTaskRefp = ನೂತನ AstTaskRef{fl, resizeAllTaskp};
                randomizep->addStmtsp(resizeTaskRefp->makeStmt());
            }
        };
        ಯದಿ (!basicFirst) emitResizeFallback();

        AstVarRef* ಧ್ರುವ fvarRefReadp = fvarRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        fvarRefReadp->access(VAccess::READ);

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ secondHalfp
            = basicFirst ? beginValp : ನೂತನ AstFuncRef{fl, basicRandomizep};
        randomizep->addStmtsp(ನೂತನ AstAssign{fl, fvarRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                            ನೂತನ AstAnd{fl, fvarRefReadp, secondHalfp}});

        ಯದಿ (basicFirst) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ಯದಿ (classHasRandClassMembers(nodep)) {
            AstTask* ಧ್ರುವ postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->addStmtsp((ನೂತನ AstTaskRef{fl, postTaskp})->makeStmt());
        }

        addPrePostCall(nodep, randomizep, "post_randomize");
        nodep->user1(ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstRandCase* nodep) override {
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
        Ast_ಗಂಟು_DType* ಧ್ರುವ sumDTypep = nodep->findUInt64DType();

        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಧ್ರುವ std::string name = "__Vrandcase" + cvtToStr(m_rand_Case_ಅಂಕೆ++);
        AstVar* ಧ್ರುವ randVarp = ನೂತನ AstVar{fl, VVarType::BLOCKTEMP, name, sumDTypep};
        randVarp->noSubst(ಸತ್ಯ);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        addTempVarsp(randVarp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sump = ನೂತನ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 64, 0};
        Ast_ಗಂಟು_If* ಧ್ರುವ firstIfsp
            = ನೂತನ AstIf{fl, ನೂತನ AstConst{fl, AstConst::BitFalse{}}, nullptr, nullptr};
        Ast_ಗಂಟು_If* ifsp = firstIfsp;

        ಪ್ರತಿ (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = itemp->condsp()->unlinkFrBack();
            sump
                = ನೂತನ AstAdd{condp->fileline(), sump, ನೂತನ AstExtend{itemp->fileline(), condp, 64}};
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlinkFrBackWithNext() : nullptr;
            AstVarRef* ಧ್ರುವ randVarRefp = ನೂತನ AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_ಗಂಟು_If* ಧ್ರುವ newifp
                = ನೂತನ AstIf{itemp->fileline(),
                            ನೂತನ AstLte{condp->fileline(), randVarRefp, sump->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)},
                            stmtsp, nullptr};
            ifsp->addElsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ಧ್ರುವ dispp = ನೂತನ AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            nullptr, nullptr};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->addElsesp(dispp);

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ randp = ನೂತನ AstRand{fl, nullptr, ಅಸತ್ಯ};
        randp->dtypeSetUInt64();
        AstVarRef* ಧ್ರುವ randVarRefp = ನೂತನ AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_ಗಂಟು* ಧ್ರುವ newp
            = ನೂತನ AstAssign{fl, randVarRefp,
                            ನೂತನ AstAdd{fl, ನೂತನ AstConst{fl, AstConst::Unsized64{}, 1},
                                       ನೂತನ AstModDiv{fl, randp, sump}}};
        newp->addNext(firstIfsp);
        ಯದಿ (debug() >= 9) newp->dump_ವೃಕ್ಷ_And_Next(cout, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (nodep->name() == "rand_mode") {
            AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = methodCallp ? methodCallp->fromp() : nullptr;
            ಧ್ರುವ RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_modp);
            UASSERT_OBJ(randModeTarget.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ಧ್ರುವ receiverp = randModeTarget.receiverp;
            ಧ್ರುವ ತಾರ್ಕಿಕ isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_ಗಂಟು* classLevelStaticLoopp = nullptr;
            ಯದಿ (isClassLevel && nodep->argsp()) {
                ಯದಿ (AstVar* ಧ್ರುವ sVarp = getStaticRandModeVar(randModeTarget.classp)) {
                    File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ staticLhsp
                        = makeModeAssignLhs(fl, randModeTarget.classp, nullptr, sVarp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ argClonep = nodep->argsp()->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    classLevelStaticLoopp = makeModeSetLoop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ಧ್ರುವ ತಾರ್ಕಿಕ receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ಧ್ರುವ randModeVarp = receiverIsStaticRand
                                             ? getStaticRandModeVar(randModeTarget.classp)
                                             : getRandModeVarFromClass(randModeTarget.classp);
            ಯದಿ (!randModeVarp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(ತಳ್ಳು_Deletep(m_stmtp), m_stmtp = nullptr);
                ಪ್ರತ್ಯಾಗಮ;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = makeModeAssignLhs(nodep->fileline(), randModeTarget.classp,
                                                        randModeTarget.fromp, randModeVarp);
            replaceWithModeAssignAndAppend(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : nullptr, lhsp,
                classLevelStaticLoopp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (nodep->name() == "constraint_mode") {
            AstMethodCall* ಧ್ರುವ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp = methodCallp ? methodCallp->fromp() : nullptr;
            AstConstraint* constrp = nullptr;
            AstClass* classp = VN_CAST(m_modp, Class);
            ಯದಿ (AstConstraintRef* ಧ್ರುವ constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                ಯದಿ (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } ಅನ್ಯಥಾ ಯದಿ (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ಧ್ರುವ constraintModeVarp = (constrp && constrp->isStatic())
                                                   ? getStaticConstraintModeVar(classp)
                                                   : getConstraintModeVar(classp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp
                = makeModeAssignLhs(nodep->fileline(), classp, fromp, constraintModeVarp);
            replaceWithModeAssign(nodep, constrp, lhsp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ಧ್ರುವ ತಾರ್ಕಿಕ inStaticContext = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ಧ್ರುವ stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ಧ್ರುವ randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            ಯದಿ (inStaticContext) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(ಸತ್ಯ);
            }
            randomizeFuncp->addStmtsp(
                ನೂತನ AstAssign{nodep->fileline(),
                              ನೂತನ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              ನೂತನ AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            ಪೂರ್ಣಾಙ್ಕ argn = 0;
            AstWith* ಧ್ರುವ withp = nodep->withp();
            ಪ್ರತಿ (ಧ್ರುವ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = argp->exprp();
                AstCMethodHard* ಧ್ರುವ basicMethodp = ನೂತನ AstCMethodHard{
                    nodep->fileline(),
                    ನೂತನ AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ಧ್ರುವ refvarp
                    = ನೂತನ AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::to_string(++argn), exprp->dtypep()};
                refvarp->setStdRandomizeArg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                ಯದಿ (withp) {
                    withp->foreach([&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exp) {
                        ಯದಿ (is_Similar_ಗಂಟು(exp, exprp)) {
                            AstVarRef* ಧ್ರುವ replaceVar
                                = ನೂತನ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            exp->replaceWith(replaceVar);
                            replaceVar->user1(exp->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(exp), exp);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(ಸತ್ಯ);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->addStmtsp(refvarp);

                ಧ್ರುವ size_t width = exprp->width();
                basicMethodp->addPinsp(
                    ನೂತನ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->addPinsp(
                    ನೂತನ AstConst{nodep->fileline(), AstConst::Unsized64{}, width});
                basicMethodp->dtypeSetBit();

                randomizeFuncp->addStmtsp(ನೂತನ AstAssign{
                    nodep->fileline(),
                    ನೂತನ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    ನೂತನ AstAnd{nodep->fileline(),
                               ನೂತನ AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            ಯದಿ (withp) {
                File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, nullptr,
                                                                inStaticContext);
                withCapturep->addFunctionArguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                ಯದಿ (stdrand) {
                    randomizeFuncp->addStmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_ಗಂಟು* ಧ್ರುವ capturedTreep = withp->exprp()->unlinkFrBackWithNext();
                randomizeFuncp->addStmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor{
                        nullptr, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        nullptr, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
                }
                AstCExpr* ಧ್ರುವ solverCallp = ನೂತನ AstCExpr{fl};
                solverCallp->dtypeSetBit();
                solverCallp->add(ನೂತನ AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->add(".next()");
                AstVar* ಧ್ರುವ fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ andExprp
                    = ನೂತನ AstAnd{fl, ನೂತನ AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->addStmtsp(
                    ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlinkFrBack()->delete_ವೃಕ್ಷ(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            ಯದಿ (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            ಯದಿ (withCapturep) nodep->addArgsp(withCapturep->getArgs());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            ಪ್ರತ್ಯಾಗಮ;
        }
        handleRandomizeArgs(nodep);

        AstWith* ಧ್ರುವ withp = nodep->withp();
        ಯದಿ (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        withp->unlinkFrBack();
        iterateChildren(nodep);

        AstClass* classp = nullptr;
        ಯದಿ (AstMethodCall* ಧ್ರುವ callp = VN_CAST(nodep, MethodCall)) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } ಅನ್ಯಥಾ {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        ಯದಿ (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ಧ್ರುವ classGenp = getRandomGenerator(classp);
        AstVar* ಧ್ರುವ localGenp
            = ನೂತನ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(ಸತ್ಯ);

        AstFunc* ಧ್ರುವ randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inlineUniqueNames.get(nodep), ಅಸತ್ಯ);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ಧ್ರುವ std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ>{ಅಸತ್ಯ, ಅಸತ್ಯ};
        ಯದಿ (prePostWrap.first) {
            addVirtualPrePostCall(randomizeFuncp, classp, "pre_randomize");
        } ಅನ್ಯಥಾ {
            addPrePostCall(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ಯದಿ (classHasRandClassMembers(classp)) {
            AstTask* ಧ್ರುವ preTaskp = getCreateNestedCallbackTask(classp, "pre");
            ಯದಿ (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->addStmtsp((ನೂತನ AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ಧ್ರುವ CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.addFunctionArguments(randomizeFuncp);

        // Add constraints clearing code
        ಯದಿ (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ಧ್ರುವ mainRandomizep
                = VN_CAST(m_memberMap.findMember(classp, "randomize"), Func);
            ಯದಿ (mainRandomizep && mainRandomizep->stmtsp()) {
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ತಾರ್ಕಿಕ foundClearConstraints = ಅಸತ್ಯ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        ಯದಿ (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->addStmtsp(stmtp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                        } ಅನ್ಯಥಾ ಯದಿ (methodp->method() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            foundClearConstraints = ಸತ್ಯ;
                        }
                    });
                    ಯದಿ (foundClearConstraints) ಭಙ್ಗ;
                }
            }
            randomizeFuncp->addStmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->addStmtsp(localGenp);

        AstFunc* ಧ್ರುವ basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ಧ್ರುವ basicRandomizeFuncCallp
            = ನೂತನ AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        ಯದಿ (classGenp) {
            AstTask* ಧ್ರುವ constrSetupFuncp = getCreateConstraintSetupFunc(classp, m_memberMap);
            AstTaskRef* ಧ್ರುವ callp = ನೂತನ AstTaskRef{nodep->fileline(), constrSetupFuncp};
            randomizeFuncp->addStmtsp(callp->makeStmt());
            randomizeFuncp->addStmtsp(ನೂತನ AstAssign{
                nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                ನೂತನ AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), ಗಂಟು_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ಧ್ರುವ randModeVarp = getRandModeVarFromClass(classp);
        ಯದಿ (!classGenp && randModeVarp) addSetRandMode(randomizeFuncp, localGenp, randModeVarp);
        ಯದಿ (!classGenp) {
            ಯದಿ (AstVar* ಧ್ರುವ sVarp = getStaticRandModeVar(classp)) {
                addSetStaticRandMode(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_ಗಂಟು* ಧ್ರುವ capturedTreep = withp->exprp()->unlinkFrBackWithNext();
        randomizeFuncp->addStmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_ಗಂಟು* inlineResizeStmtsp = nullptr;
        {
            std::vector<AstCMethodHard*> sizeMethodps;
            capturedTreep->foreachAndNext([&](AstCMethodHard* methodp) {
                ಯದಿ (methodp->method() == VCMethod::DYN_SIZE
                    || methodp->method() == VCMethod::ASSOC_SIZE) {
                    sizeMethodps.ತಳ್ಳು_back(methodp);
                }
            });
            ಪ್ರತಿ (AstCMethodHard* ಧ್ರುವ methodp : sizeMethodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = nullptr;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ಬೇರು_Exprp = methodp->fromp();
                ಯದಿ (AstMemberSel* memberSelp = VN_CAST(ಬೇರು_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    ಯಾವತ್ (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = memberSelp->fromp();
                    ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ varRefp = VN_CAST(fromp, VarRef)) {
                        ಯದಿ (varRefp->varp() == captured.getThisp()) ಅನುವರ್ತ;
                    } ಅನ್ಯಥಾ {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ varRefp = VN_CAST(ಬೇರು_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                ಯದಿ (!arrVarp) ಅನುವರ್ತ;
                // Only handle rand-declared dynamic/assoc array variables
                ಯದಿ (!arrVarp->rand().isRandomizable()) ಅನುವರ್ತ;
                // Do not randomize associative array size (IEEE 18.4)
                ಯದಿ (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) ಅನುವರ್ತ;
                File_ಸಾಲು* ಧ್ರುವ fl = methodp->fileline();
                ತಾರ್ಕಿಕ wasCreated = ಅಸತ್ಯ;
                AstClass* ಧ್ರುವ varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ಧ್ರುವ sizeVarp = createOrGetSizeVar(varClassp, arrVarp, fl,
                                                            methodp->findIntDType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                ಯದಿ (wasCreated) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resizeVarRefp = buildMemberSelExprp(ಬೇರು_Exprp, arrVarp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sizeExprp = buildMemberSelExprp(ಬೇರು_Exprp, sizeVarp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->addNext(createSizeGteZeroConstraint(fl, sizeVarp, ಬೇರು_Exprp));
                    AstCMethodHard* ಧ್ರುವ resizep
                        = ನೂತನ AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, sizeExprp};
                    resizep->dtypep(methodp->findVoidDType());
                    inlineResizeStmtsp
                        = Ast_ಗಂಟು::addNext(inlineResizeStmtsp, ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, resizep});
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ size_ಗಂಟು_Exprp = buildMemberSelExprp(ಬೇರು_Exprp, sizeVarp);
                methodp->replaceWith(size_ಗಂಟು_Exprp);
                VL_DO_DANGLING(methodp->delete_ವೃಕ್ಷ(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                randModeVarp, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
        }

        // Call the solver and set return value
        AstCExpr* ಧ್ರುವ solverCallp = ನೂತನ AstCExpr{nodep->fileline()};
        solverCallp->dtypeSetBit();
        solverCallp->add(ನೂತನ AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->add(".next(__Vm_rng)");
        randomizeFuncp->addStmtsp(ನೂತನ AstAssign{
            nodep->fileline(),
            ನೂತನ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            ನೂತನ AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        ಯದಿ (inlineResizeStmtsp) {
            AstVar* ಧ್ರುವ retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{nodep->fileline(),
                                         ನೂತನ AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, nullptr};
            randomizeFuncp->addStmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ಯದಿ (classHasRandClassMembers(classp)) {
            AstTask* ಧ್ರುವ postTaskp = getCreateNestedCallbackTask(classp, "post");
            ಯದಿ (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->addStmtsp((ನೂತನ AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        ಯದಿ (prePostWrap.second) {
            addVirtualPrePostCall(randomizeFuncp, classp, "post_randomize");
        } ಅನ್ಯಥಾ {
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
        VL_DO_DANGLING(withp->delete_ವೃಕ್ಷ(), withp);
    }
    ಶೂನ್ಯ visit(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಯದಿ (m_constraintp && nodep->fromp()->user1()
            && (nodep->method() == VCMethod::ASSOC_SIZE
                || nodep->method() == VCMethod::DYN_SIZE)) {
            AstClass* ಧ್ರುವ classp = VN_AS(m_modp, Class);
            AstVarRef* ಧ್ರುವ queueVarRefp = VN_CAST(nodep->fromp(), VarRef);
            ಯದಿ (!queueVarRefp) {
                // Warning from ConstraintExprVisitor will be thrown
                ಪ್ರತ್ಯಾಗಮ;
            }
            AstVar* ಧ್ರುವ queueVarp = queueVarRefp->varp();
            ತಾರ್ಕಿಕ wasCreated = ಅಸತ್ಯ;
            AstVar* ಧ್ರುವ sizeVarp
                = createOrGetSizeVar(classp, queueVarp, fl, nodep->findIntDType(), wasCreated);
            ಯದಿ (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                ಯದಿ (!VN_IS(queueVarp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    ಯದಿ (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ಧ್ರುವ sizeVarRefp = ನೂತನ AstVarRef{
                        fl, VN_AS(sizeVarp->user2p(), ಗಂಟು_Module), sizeVarp, VAccess::READ};
                    AstCMethodHard* ಧ್ರುವ resizep = ನೂತನ AstCMethodHard{
                        fl, nodep->fromp()->unlinkFrBack(), VCMethod::DYN_RESIZE, sizeVarRefp};
                    resizep->dtypep(nodep->findVoidDType());
                    resizerTaskp->addStmtsp(ನೂತನ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->addItemsp(createSizeGteZeroConstraint(fl, sizeVarp));
            }
            AstVarRef* ಧ್ರುವ sizeVarRefp = ನೂತನ AstVarRef{fl, VN_AS(sizeVarp->user2p(), ಗಂಟು_Module),
                                                         sizeVarp, VAccess::READ};
            sizeVarRefp->user1(ಸತ್ಯ);
            nodep->replaceWith(sizeVarRefp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
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
        nodep->foreach([&](AstClass* ಧ್ರುವ classp) {
            ಯದಿ (!classp->user1()) ಪ್ರತ್ಯಾಗಮ;
            classp->foreachMember([&](AstClass* ಧ್ರುವ ownerClassp, AstConstraint* ಧ್ರುವ constrp) {
                constrp->foreach([&](AstMemberSel* ಧ್ರುವ memberSelp) {
                    AstVar* ಧ್ರುವ varp = memberSelp->varp();
                    ಯದಿ (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) ಪ್ರತ್ಯಾಗಮ;
                    ಯದಿ (VN_AS(varp->user2p(), ಗಂಟು_Module) != ownerClassp) ಪ್ರತ್ಯಾಗಮ;
                    ಯದಿ (!varp->user3()) varp->user3(ಸತ್ಯ);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(constrp->unlinkFrBack()), constrp);
        });
    }
    ~RandomizeVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Randomize method class functions

ಶೂನ್ಯ V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ಧ್ರುವ RandomizeMarkVisitor markVisitor{nodep};
        ಧ್ರುವ RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("randomize", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ಧ್ರುವ std::string& name, ತಾರ್ಕಿಕ allowVirtual,
                                       ತಾರ್ಕಿಕ childDType) {
    AstFunc* funcp = VN_AS(memberMap.findMember(nodep, name), Func);
    ಯದಿ (!funcp) {
        v3Global.useRandomizeMethods(ಸತ್ಯ);
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep
            = childDType
                  ? ನೂತನ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ಧ್ರುವ fvarp = childDType
                                  ? ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlagChildDType{}, dtypep}
                                  : ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3ErrorCode::NORETURN, ಸತ್ಯ);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(ಸತ್ಯ);
        fvarp->funcReturn(ಸತ್ಯ);
        fvarp->direction(VDirection::OUTPUT);
        nodep->addMembersp(funcp);
        funcp = ನೂತನ AstFunc{nodep->fileline(), name, nullptr, fvarp};
        ಯದಿ (!childDType) funcp->dtypep(dtypep);
        funcp->classMethod(ಸತ್ಯ);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->addMembersp(funcp);
        memberMap.ಸೇರಿಸು(nodep, funcp);
    }
    ಪ್ರತ್ಯಾಗಮ funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_ಗಂಟು_Module* nodep, ಧ್ರುವ std::string& name) {
    AstFunc* funcp = nullptr;
    v3Global.useRandomizeMethods(ಸತ್ಯ);
    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
    AstVar* ಧ್ರುವ fvarp = ನೂತನ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(ಸತ್ಯ);
    fvarp->funcReturn(ಸತ್ಯ);
    fvarp->direction(VDirection::OUTPUT);
    funcp = ನೂತನ AstFunc{nodep->fileline(), name, nullptr, fvarp};
    funcp->dtypep(dtypep);
    ಯದಿ (VN_IS(nodep, Class)) {
        funcp->classMethod(ಸತ್ಯ);
    } ಅನ್ಯಥಾ {
        funcp->classMethod(ಸತ್ಯ);
        funcp->isStatic(ಸತ್ಯ);
    }
    nodep->addStmtsp(funcp);
    ಪ್ರತ್ಯಾಗಮ funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ಧ್ರುವ basep = nodep->baseMostClassp();
    AstFunc* funcp = VN_AS(memberMap.findMember(basep, "srandom"), Func);
    ಯದಿ (!funcp) {
        v3Global.useRandomizeMethods(ಸತ್ಯ);
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep
            = basep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ಧ್ರುವ ivarp = ನೂತನ AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(ಸತ್ಯ);
        ivarp->direction(VDirection::ಒಳಹರಿವು);
        funcp = ನೂತನ AstFunc{basep->fileline(), "srandom", ivarp, nullptr};
        funcp->dtypep(basep->findVoidDType());
        funcp->classMethod(ಸತ್ಯ);
        funcp->isVirtual(ಅಸತ್ಯ);
        basep->addMembersp(funcp);
        memberMap.ಸೇರಿಸು(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        ಯದಿ (basep->name() == "process") {
            funcp->addStmtsp(ನೂತನ AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } ಅನ್ಯಥಾ {
            funcp->addStmtsp(ನೂತನ AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(ಸತ್ಯ);
        }
    }
    ಪ್ರತ್ಯಾಗಮ funcp;
}
