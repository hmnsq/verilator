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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Randomize.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3ConstPool.h"
#অন্তর্ভাৱ "V3Container.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3FileLine.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3MemberMap.h"
#অন্তর্ভাৱ "V3Task.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

#অন্তর্ভাৱ <queue>
#অন্তর্ভাৱ <tuple>
#অন্তর্ভাৱ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

গণনা ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

স্থির constexpr ধ্রুৱ ৱর্ণ* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
স্থির constexpr ধ্রুৱ ৱর্ণ* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
স্থির AstVar* getRandModeVarFromClass(Ast_গ্ৰন্থি_Module* classp) {
    যেতিয়ালৈ (classp) {
        যদি (classp->user2p()) ঘূৰাই VN_AS(classp->user2p(), Var);
        AstClass* ধ্রুৱ cp = VN_CAST(classp, Class);
        যদি (!cp || !cp->extendsp()) ঘূৰাই nullptr;
        classp = cp->extendsp()->classp();
    }
    ঘূৰাই nullptr;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
স্থির AstVar* getRandomGenerator(AstClass* ধ্রুৱ classp) {
    যদি (classp->user3p()) ঘূৰাই VN_AS(classp->user3p(), Var);
    যদি (classp->extendsp()) ঘূৰাই getRandomGenerator(classp->extendsp()->classp());
    ঘূৰাই nullptr;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
স্থির AstTask* getCreateConstraintSetupFunc(AstClass* classp, VMemberMap& memberMap) {
    স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ name = "__Vsetup_constraints";
    AstTask* setupAllTaskp = VN_AS(memberMap.findMember(classp, name), Task);
    যদি (setupAllTaskp) ঘূৰাই setupAllTaskp;
    setupAllTaskp = নূতন AstTask{classp->fileline(), "__Vsetup_constraints", nullptr};
    setupAllTaskp->classMethod(সঁচা);
    setupAllTaskp->isVirtual(সঁচা);
    classp->addMembersp(setupAllTaskp);
    memberMap.ঢোকাও(classp, setupAllTaskp);
    ঘূৰাই setupAllTaskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

সংরচনা RandModeTarget final {
    // MEMBERS
    AstVar* ধ্রুৱ receiverp;  // Variable that is the target of the rand_mode() call
    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp;  // Expression from which the rand_mode() call originates
    AstClass* ধ্রুৱ classp;  // Class in which rand_mode should be set

    // METHODS
    স্থির RandModeTarget get(Ast_গ্ৰন্থি_ৰাশিমালা* fromp, Ast_গ্ৰন্থি_Module* modp) {
        যদি (!fromp) ঘূৰাই {nullptr, nullptr, VN_AS(modp, Class)};
        যদি (AstCMethodHard* ধ্রুৱ methodHardp = VN_CAST(fromp, CMethodHard)) {
            ঘূৰাই RandModeTarget::get(methodHardp->fromp(), modp);
        }
        AstVar* receiverp = nullptr;
        AstClass* classp = VN_CAST(modp, Class);
        যদি (AstVarRef* ধ্রুৱ varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            যদি (receiverp->isRand()) {
                fromp = nullptr;
                যদি (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } অন্যথা যদি (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            যদি (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        যদি (!receiverp->isRand()) {
            AstClassRefDType* ধ্রুৱ classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            যদি (classRefDtp) classp = classRefDtp->classp();
        }
        ঘূৰাই {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

সঙ্ঘ RandomizeMode final {
    // MEMBERS
    সংরচনা {
        তার্কিক usesMode : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t index : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
স্থির Ast_গ্ৰন্থি_DType* arrayElementDTypep(Ast_গ্ৰন্থি_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    যেতিয়ালৈ (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    ঘূৰাই dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
স্থির তার্কিক dtypeContainsReal(Ast_গ্ৰন্থি_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ subp = dtypep->subDTypep()) ঘূৰাই dtypeContainsReal(subp);
    যদি (ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ structp = VN_CAST(dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
        যদি (structp->packed()) ঘূৰাই মিছা;
        বাবে (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            যদি (dtypeContainsReal(memberp->subDTypep())) ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }
    ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = VN_CAST(dtypep, BasicDType);
    ঘূৰাই basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
স্থির শূন্য checkRandTypeEligibility(Ast_গ্ৰন্থি* contextp, Ast_গ্ৰন্থি_DType* dtypep, তার্কিক isRandc) {
    dtypep = dtypep->skipRefp();
    যদি (Ast_গ্ৰন্থি_DType* ধ্রুৱ subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        ঘূৰাই;
    }
    যদি (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        যদি (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        ঘূৰাই;
    }
    যদি (ধ্রুৱ AstIfaceRefDType* ধ্রুৱ ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        ঘূৰাই;
    }
    যদি (ধ্রুৱ Ast_গ্ৰন্থি_UOr_Struct_DType* ধ্রুৱ structp = VN_CAST(dtypep, গ্ৰন্থি_UOr_Struct_DType)) {
        যদি (structp->packed()) ঘূৰাই;  // Integral by construction; members checked separately
        যদি (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            ঘূৰাই;
        }
        বাবে (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            যদি (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        ঘূৰাই;
    }
    ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    যদি (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        যদি (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        ঘূৰাই;
    }
    ৱিকল্প (basicp->keyword()) {
    পক্ষ VBasicDTypeKwd::STRING:
    পক্ষ VBasicDTypeKwd::CHANDLE:
    পক্ষ VBasicDTypeKwd::EVENT: {
        ধ্রুৱ ৱর্ণ* ধ্রুৱ articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        ভঙ্গ;
    }
    স্ৱভাৱ: ভঙ্গ;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

শ্ৰেণী RandomizeMarkVisitor final : সার্ৱজনিক VNVisitor {
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
    ধ্রুৱ VNUser1InUse m_inuser1;
    ধ্রুৱ VNUser2InUse m_inuser2;

    using DerivedSet = std::unordered_set<AstClass*>;
    using BaseToDerivedMap = std::unordered_map<ধ্রুৱ AstClass*, DerivedSet>;

    BaseToDerivedMap m_baseToDerivedMap;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = nullptr;  // Current class
    Ast_গ্ৰন্থি* m_constraint_ৰাশিমালা_Genp = nullptr;  // Current constraint or constraint if expression
    Ast_গ্ৰন্থি_Module* m_modp;  // Current module
    Ast_গ্ৰন্থি_Stmt* m_stmtp = nullptr;  // Current statement
    Ast_গ্ৰন্থি_FTask_Ref* m_stdRandCallp = nullptr;  // Current std::randomize() call
    তার্কিক m_inStdWith = মিছা;  // True when inside a 'with {}' clause
    std::set<Ast_গ্ৰন্থি_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = nullptr;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    শূন্য markGlobalConstrainedVars(AstClass* classp) {
        বাবে (ধ্রুৱ AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            বাবে (Ast_গ্ৰন্থি* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ধ্রুৱ varp = VN_CAST(memberp, Var);
                যদি (!varp) অনুৱর্ত;
                যদি (varp->rand().isRandomizable()) varp->globalConstrained(সঁচা);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    তার্কিক isVarInStdRandomizeArgs(ধ্রুৱ AstVar* varp) ধ্রুৱ {
        যদি (!m_inStdWith || !m_stdRandCallp) ঘূৰাই মিছা;

        বাবে (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            যেতিয়ালৈ (exprp) {
                যদি (ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ varrefp = VN_CAST(exprp, গ্ৰন্থি_Var_Ref)) {
                    যদি (varrefp->varp() == varp) ঘূৰাই সঁচা;
                    ভঙ্গ;
                }
                যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ memberselp = VN_CAST(exprp, MemberSel)) {
                    যদি (memberselp->varp() == varp) ঘূৰাই সঁচা;
                    exprp = memberselp->fromp();
                } অন্যথা যদি (ধ্রুৱ AstArraySel* ধ্রুৱ arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } অন্যথা যদি (ধ্রুৱ AstStructSel* ধ্রুৱ strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } অন্যথা যদি (ধ্রুৱ AstCMethodHard* ধ্রুৱ methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } অন্যথা {
                    ভঙ্গ;
                }
            }
        }
        ঘূৰাই মিছা;
    }
    শূন্য markMembers(ধ্রুৱ AstClass* nodep) {
        বাবে (ধ্রুৱ AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            বাবে (Ast_গ্ৰন্থি* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ধ্রুৱ varp = VN_CAST(memberp, Var);
                যদি (!varp) অনুৱর্ত;
                // If member is randomizable and of class type, mark its class
                যদি (varp->rand().isRandomizable()) {
                    ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ varDtypep = varp->dtypep()->skipRefp();
                    ধ্রুৱ AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    যদি (!classRefp) {
                        ধ্রুৱ Ast_গ্ৰন্থি_DType* subDTypep = varDtypep->subDTypep();
                        যদি (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    যদি (classRefp) {
                        AstClass* ধ্রুৱ rclassp = classRefp->classp();
                        যদি (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    যদি (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        RandomizeMode randMode = {};
                        randMode.usesMode = সঁচা;
                        varp->user1(randMode.asUQuad);
                    }
                }
            }
        }
    }
    শূন্য markDerived(ধ্রুৱ AstClass* nodep) {
        ধ্রুৱ auto it = m_baseToDerivedMap.find(nodep);
        যদি (it != m_baseToDerivedMap.end()) {
            বাবে (auto* classp : it->second) {
                যদি (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    শূন্য markAllDerived() {
        বাবে (ধ্রুৱ auto& p : m_baseToDerivedMap) {
            যদি (p.first->user1()) markDerived(p.first);
        }
    }
    শূন্য setPackageRefs() {
        বাবে (Ast_গ্ৰন্থি_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), গ্ৰন্থি_Module));
        }
    }
    শূন্য markNestedGlobalConstrainedRecurse(Ast_গ্ৰন্থি* nodep) {
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(nodep, VarRef)) {
            AstVar* ধ্রুৱ varp = refp->varp();
            যদি (varp->globalConstrained()) ঘূৰাই;
            varp->globalConstrained(সঁচা);
        } অন্যথা যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(nodep, MemberSel)) {
            যদি (memberSelp->varp()) {
                AstVar* ধ্রুৱ varp = memberSelp->varp();
                যদি (varp->globalConstrained()) ঘূৰাই;
                varp->globalConstrained(সঁচা);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_গ্ৰন্থি_ৰাশিমালা* buildMemberSelChain(AstVarRef* মূল_Var_Refp, ধ্রুৱ std::vector<AstVar*>& path) {
        Ast_গ্ৰন্থি_ৰাশিমালা* exprp = মূল_Var_Refp->clone_বৃক্ষ(মিছা);
        বাবে (AstVar* memberVarp : path) {
            AstMemberSel* memberSelp
                = নূতন AstMemberSel{মূল_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        ঘূৰাই exprp;
    }

    // Process a single constraint during nested constraint cloning
    শূন্য processNestedConstraint(AstConstraint* ধ্রুৱ constrp, AstVarRef* মূল_Var_Refp,
                                 ধ্রুৱ std::vector<AstVar*>& newPath, AstClass* targetClassp) {
        std::string pathPrefix = মূল_Var_Refp->name();
        বাবে (AstVar* pathMemberVarp : newPath) {
            pathPrefix += GLOBAL_CONSTRAINT_SEPARATOR + pathMemberVarp->name();
        }

        ধ্রুৱ std::string newName = pathPrefix + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        তার্কিক isDuplicate = মিছা;
        targetClassp->foreachMember([&](AstClass* ধ্রুৱ, AstConstraint* ধ্রুৱ existingConstrp) {
            যদি (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = সঁচা;
            }
        });
        যদি (isDuplicate) ঘূৰাই;

        AstConstraint* ধ্রুৱ cloneConstrp = constrp->clone_বৃক্ষ(মিছা);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::vector<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
            যদি (AstVarRef* ধ্রুৱ varRefp = VN_CAST(exprp, VarRef)) {
                যদি (!varRefp->varp()->isClassMember()) ঘূৰাই;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ chainp = buildMemberSelChain(মূল_Var_Refp, newPath);
                AstMemberSel* ধ্রুৱ finalSelp
                    = নূতন AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(ঠেলো_Deletep(varRefp), varRefp);
            } অন্যথা যদি (AstFuncRef* ধ্রুৱ funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                যদি (funcRefp->taskp()->classMethod() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.ঠেলো_back(funcRefp);
                }
            }
        });
        বাবে (AstFuncRef* funcRefp : funcRefps) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ chainp = buildMemberSelChain(মূল_Var_Refp, newPath);
            AstArg* ধ্রুৱ argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* ধ্রুৱ callp
                = নূতন AstMethodCall{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(ঠেলো_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        targetClassp->addStmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    শূন্য cloneNestedConstraintsRecurse(AstVarRef* মূল_Var_Refp, AstClass* classp,
                                       ধ্রুৱ std::vector<AstVar*>& pathToClass,
                                       AstClass* targetClassp) {
        বাবে (Ast_গ্ৰন্থি* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ধ্রুৱ memberVarp = VN_CAST(memberNodep, Var);
            যদি (!memberVarp) অনুৱর্ত;
            যদি (!memberVarp->rand().isRandomizable()) অনুৱর্ত;
            ধ্রুৱ AstClassRefDType* ধ্রুৱ memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            যদি (!memberClassRefp || !memberClassRefp->classp()) অনুৱর্ত;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::vector<AstVar*> newPath = pathToClass;
            newPath.ঠেলো_back(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ধ্রুৱ containingClassp, AstConstraint* ধ্রুৱ constrp) {
                    processNestedConstraint(constrp, মূল_Var_Refp, newPath, targetClassp);
                });
            cloneNestedConstraintsRecurse(মূল_Var_Refp, nestedClassp, newPath, targetClassp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    শূন্য handleRandomizeArgument(Ast_গ্ৰন্থি_ৰাশিমালা* exprp, AstVar* ধ্রুৱ varp, ধ্রুৱ তার্কিক stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        যেতিয়ালৈ (exprp) {
            AstVar* randVarp = nullptr;
            যদি (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } অন্যথা যদি (AstArraySel* ধ্রুৱ arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                অনুৱর্ত;  // Skip ArraySel, continue traversing
            } অন্যথা যদি (AstStructSel* ধ্রুৱ structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                অনুৱর্ত;  // Skip StructSel, continue traversing
            } অন্যথা যদি (AstCMethodHard* ধ্রুৱ methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                অনুৱর্ত;
            } অন্যথা যদি (AstVarRef* ধ্রুৱ varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(সঁচা);
                varrefp->access(VAccess::READWRITE);
                exprp = nullptr;
            } অন্যথা {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            যদি (randVarp == varp) ঘূৰাই;
            Ast_গ্ৰন্থি* backp = randVarp;
            যেতিয়ালৈ (backp && !VN_IS(backp, গ্ৰন্থি_Module)) backp = backp->backp();
            যদি (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } অন্যথা {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                RandomizeMode randMode = {};
                randMode.usesMode = সঁচা;
                randVarp->user1(randMode.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    শূন্য visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        যদি (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ধ্রুৱ AstClass* ধ্রুৱ basep = nodep->extendsp()->classp();
            m_baseToDerivedMap[basep].ঢোকাও(nodep);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        যদি (!nodep->backp()) VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask_Ref* nodep) override {
        যদি (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        যদি (nodep->name() == "rand_mode") {
            AstMethodCall* ধ্রুৱ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = methodCallp ? methodCallp->fromp() : nullptr;
            তার্কিক valid = সঁচা;
            যদি (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = মিছা;
            } অন্যথা যদি (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = মিছা;
            } অন্যথা যদি (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = মিছা;
            } অন্যথা যদি (AstCMethodHard* ধ্রুৱ methodHardp = VN_CAST(fromp, CMethodHard)) {
                যদি (methodHardp->method() == VCMethod::ARRAY_AT
                    || methodHardp->method() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = মিছা;
                } অন্যথা {
                    methodHardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            যদি (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ৰাশিমালা)
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = মিছা;
            }
            যদি (valid) {
                ধ্রুৱ RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_classp);
                যদি ((!randModeTarget.receiverp || !randModeTarget.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = মিছা;
                } অন্যথা যদি (!randModeTarget.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = মিছা;
                } অন্যথা যদি (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ৰাশিমালা)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = মিছা;
                } অন্যথা যদি (randModeTarget.receiverp && randModeTarget.receiverp->isRand()) {
                    // Called on a rand member variable
                    RandomizeMode randMode = {};
                    randMode.usesMode = সঁচা;
                    randModeTarget.receiverp->user1(randMode.asUQuad);
                } অন্যথা {
                    // Called on 'this' or a non-rand class instance
                    randModeTarget.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        যদি (!varp->isRand()) ঘূৰাই;
                        RandomizeMode randMode = {};
                        randMode.usesMode = সঁচা;
                        varp->user1(randMode.asUQuad);
                    });
                }
            }
            যদি (!valid) {
                যদি (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ৰাশিমালা)) {
                    nodep->replaceWith(নূতন AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                } অন্যথা {
                    m_stmtp->unlinkFrBack();
                }
            }
            ঘূৰাই;
        }

        যদি (nodep->name() == "constraint_mode") {
            তার্কিক valid = সঁচা;
            যদি (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ৰাশিমালা)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = মিছা;
            } অন্যথা যদি (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ৰাশিমালা)
                       && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = মিছা;
            }
            AstConstraint* constrp = nullptr;
            AstClass* classp = m_classp;
            যদি (ধ্রুৱ AstMethodCall* ধ্রুৱ methodCallp = VN_CAST(nodep, MethodCall)) {
                যদি (ধ্রুৱ AstConstraintRef* ধ্রুৱ constrRefp
                    = VN_CAST(methodCallp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    যদি (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } অন্যথা যদি (AstClassRefDType* classRefDtp
                           = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } অন্যথা {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = মিছা;
                }
            }
            যদি (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = মিছা;
            }
            যদি (valid) {
                RandomizeMode constraintMode = {};
                constraintMode.usesMode = সঁচা;
                যদি (constrp) {
                    constrp->user1(constraintMode.asUQuad);
                } অন্যথা {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraintMode.asUQuad);
                    });
                }
            } অন্যথা {
                যদি (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ৰাশিমালা)) {
                    nodep->replaceWith(নূতন AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                } অন্যথা {
                    m_stmtp->unlinkFrBack();
                }
            }
            ঘূৰাই;
        }

        যদি (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            যদি (m_constraint_ৰাশিমালা_Genp || m_inStdWith) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                বাবে (Ast_গ্ৰন্থি* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    যদি (argp->user1()) {
                        nodep->user1(সঁচা);
                        ভঙ্গ;
                    }
                }
            }
            ঘূৰাই;
        }
        AstClass* classp = m_classp;
        যদি (ধ্রুৱ AstMethodCall* ধ্রুৱ methodCallp = VN_CAST(nodep, MethodCall)) {
            যদি (ধ্রুৱ AstClassRefDType* ধ্রুৱ classRefp
                = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        যদি (classp) {
            যদি (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ধ্রুৱ, AstVar* ধ্রুৱ memberVarp) {
                যদি (!memberVarp->rand().isRandomizable()) ঘূৰাই;
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = memberVarp->dtypep()->skipRefp();
                ধ্রুৱ AstClassRefDType* ধ্রুৱ classRefp = VN_CAST(dtypep, ClassRefDType);
                যদি (!classRefp || !classRefp->classp()) ঘূৰাই;
                AstClass* ধ্রুৱ memberClassp = classRefp->classp();
                যদি (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) ঘূৰাই;
                memberVarp->globalConstrained(সঁচা);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* মূল_Var_Refp
                    = নূতন AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::vector<AstVar*> emptyPath;
                memberClassp->foreachMember([&](AstClass* ধ্রুৱ, AstConstraint* ধ্রুৱ constrp) {
                    processNestedConstraint(constrp, মূল_Var_Refp, emptyPath, classp);
                });
                cloneNestedConstraintsRecurse(মূল_Var_Refp, memberClassp, emptyPath, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(মূল_Var_Refp->delete_বৃক্ষ(), মূল_Var_Refp);
            });
        }
        যদি (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = nullptr;
            বাবে (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handleRandomizeArgument(argp->exprp(), nullptr, সঁচা);
            }
            ঘূৰাই;
        }
        বাবে (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->অঙ্ক().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ধ্রুৱ তার্কিক hasUnsupportedMember
                    = classp->existsMember([](ধ্রুৱ AstClass*, ধ্রুৱ AstVar* memberVarp) {
                          যদি (!memberVarp->rand().isRandomizable()) ঘূৰাই মিছা;
                          ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtp = memberVarp->dtypep()->skipRefp();
                          ঘূৰাই VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, QueueDType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                যদি (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                অনুৱর্ত;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = nullptr;  // If nodep is a method call, this is its receiver
            যদি (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                যদি (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(methodCallp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } অন্যথা {
                    AstVarRef* ধ্রুৱ varrefp = VN_AS(methodCallp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handleRandomizeArgument(argp->exprp(), fromVarp, মিছা);
        }
        // Re-mark members after the class became inline-randomized above.
        যদি (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    শূন্য visit(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_ৰাশিমালা_Genp);
        m_stmtp = nodep;
        m_constraint_ৰাশিমালা_Genp = nodep;
        iterateChildren(nodep);
        যদি (!nodep->backp()) VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_Constraint_ৰাশিমালা* nodep) override {
        VL_RESTORER(m_constraint_ৰাশিমালা_Genp);
        m_constraint_ৰাশিমালা_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_ৰাশিমালা_Genp);
            m_constraint_ৰাশিমালা_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterateAndNextConstNull(nodep->thensp());
        iterateAndNextConstNull(nodep->elsesp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        যদি (!m_constraint_ৰাশিমালা_Genp) ঘূৰাই;

        যদি (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        যদি (nodep->varp()->rand().isRandomizable() && !(m_inStdWith && m_stdRandCallp)) {
            nodep->user1(সঁচা);
        } অন্যথা যদি (isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(সঁচা);
        }
    }
    শূন্য visit(AstMemberSel* nodep) override {
        যদি (!m_constraint_ৰাশিমালা_Genp) ঘূৰাই;
        iterateChildrenConst(nodep);
        ধ্রুৱ তার্কিক randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ধ্রুৱ তার্কিক randMember = nodep->varp()->rand().isRandomizable();
        ধ্রুৱ তার্কিক inStdWith = m_inStdWith && m_stdRandCallp;
        যদি (randObject && randMember && !inStdWith) {
            nodep->user1(সঁচা);
        } অন্যথা যদি (inStdWith && isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(সঁচা);
            // Mark parent object for constraint expression visitor
            যদি (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(সঁচা);
        }

        যদি (m_withp) {
            Ast_গ্ৰন্থি* backp = m_withp;
            যেতিয়ালৈ (backp->backp()) {
                যদি (ধ্রুৱ AstMethodCall* ধ্রুৱ callp = VN_CAST(backp, MethodCall)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    ভঙ্গ;
                }
                backp = backp->backp();
            }
        } অন্যথা {
            nodep->user2p(m_modp);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstVar* nodep) override {
        nodep->user2p(m_modp);
        যদি (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        যদি (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_inStdWith = সঁচা;
        iterateChildrenConst(nodep);
        m_inStdWith = মিছা;
    }

    শূন্য visit(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) override {
        iterateChildrenConst(nodep);
        যদি (!m_constraint_ৰাশিমালা_Genp && !m_inStdWith) ঘূৰাই;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    শূন্য visit(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        যদি (!m_constraint_ৰাশিমালা_Genp && !m_inStdWith) ঘূৰাই;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit RandomizeMarkVisitor(Ast_গ্ৰন্থি* nodep) {
        iterateConst(nodep);
        markAllDerived();
        setPackageRefs();
    }
    ~RandomizeMarkVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

শ্ৰেণী Constraint_ৰাশিমালা_Visitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ধ্রুৱ m_classp;
    Ast_গ্ৰন্থি_FTask* ধ্রুৱ m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_গ্ৰন্থি_FTask* ধ্রুৱ m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ধ্রুৱ m_genp;  // VlRandomizer variable of the class
    AstVar* m_randModeVarp;  // Relevant randmode state variable
    তার্কিক m_wantSingle = মিছা;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    তার্কিক m_structSel = মিছা;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    তার্কিক m_deferRandModeHoist = মিছা;  // Wait for the complete struct member path
    std::set<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::set<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertionSet<AstVar*>* m_sizeConstrainedArraysp = nullptr;  // Arrays with size constraints
    Ast_গ্ৰন্থি_ৰাশিমালা* m_conditionp = nullptr;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_গ্ৰন্থি_FTask* m_prepareConstrainedArraysp = nullptr;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = nullptr;  // Current ID of unique call
    V3UniqueNames& m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    std::vector<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_গ্ৰন্থি* m_firstExpressionInsideIndexp = nullptr;
    AstConstraintForeach* m_foreachp = nullptr;  // Innermost constraint-foreach being processed
    std::set<ধ্রুৱ AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    শ্ৰেণী NestedAccessPath final {
        AstMemberSel* m_শীৰ্ষ_Nested_Array_Member_Selp
            = nullptr;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = nullptr;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = nullptr;  // Last node of variable's name format
        Ast_গ্ৰন্থি** m_firstExpressionInsideIndexPointerp
            = nullptr;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    সার্ৱজনিক:
        শূন্য addVarNamePart(AstSFormatF* ধ্রুৱ nodep, std::string&& name) {
            যদি (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } অন্যথা {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.ঢোকাও(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            যদি (name == "") {
                যদি (*m_firstExpressionInsideIndexPointerp) {
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

        AstMemberSel* access_বৃক্ষ() { ঘূৰাই m_শীৰ্ষ_Nested_Array_Member_Selp; }
        ধ্রুৱ std::string& smtName() { ঘূৰাই m_smtName; }

        Ast_Stmt_ৰাশিমালা* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ধ্রুৱ methodp = নূতন AstCMethodHard{
                varp->fileline(),
                নূতন AstVarRef{varp->fileline(), VN_AS(genp->user2p(), গ্ৰন্থি_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtypeSetVoid();

            methodp->addPinsp(m_শীৰ্ষ_Nested_Array_Member_Selp);  // Variable

            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = varp->dtypep();  // Width
            ধ্রুৱ size_t width = m_শীৰ্ষ_Nested_Array_Member_Selp->varp()->dtypep()->width();
            methodp->addPinsp(নূতন AstConst{dtypep->fileline(), AstConst::Unsized64{}, width});

            methodp->addPinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = nullptr;

            methodp->addPinsp(
                নূতন AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_শীৰ্ষ_Nested_Array_Member_Selp = nullptr;
            ঘূৰাই methodp->makeStmt();
        }

        Ast_গ্ৰন্থি_ৰাশিমালা* cloneName() { ঘূৰাই m_nestedNameFormatTopp->clone_বৃক্ষ(মিছা); }

        শূন্য error() {
            VL_DO_DANGLING(m_শীৰ্ষ_Nested_Array_Member_Selp->delete_বৃক্ষ(), m_শীৰ্ষ_Nested_Array_Member_Selp);
        }

        NestedAccessPath(AstMemberSel* শীৰ্ষ_Member_Selp, Ast_গ্ৰন্থি** pointerToFirstExprp)
            : m_শীৰ্ষ_Nested_Array_Member_Selp{শীৰ্ষ_Member_Selp}
            , m_firstExpressionInsideIndexPointerp{pointerToFirstExprp} {}
        ~NestedAccessPath() {
            যদি (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_বৃক্ষ(),
                            m_nestedNameFormatTopp = nullptr);
                m_nestedNameFormatp = nullptr;
            }
        }
    };
    NestedAccessPath* m_nestedAccess = nullptr;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_গ্ৰন্থি_ৰাশিমালা* createEnumConstraint(File_সাৰি* ধ্রুৱ fl, ধ্রুৱ std::string& smtName,
                                      AstEnumDType* ধ্রুৱ enumDtp, AstVar* ধ্রুৱ genVarp) {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ genModp = VN_AS(genVarp->user2p(), গ্ৰন্থি_Module);
        ধ্রুৱ পূর্ণাঙ্ক width = enumDtp->width();
        std::string constraint = "(__Vbv (or";
        বাবে (AstEnumItem* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ধ্রুৱ AstConst* ধ্রুৱ vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvtToStr(vconstp->toUInt()) + " "
                          + cvtToStr(width) + "))";
        }
        constraint += "))";
        AstCMethodHard* ধ্রুৱ callp = নূতন AstCMethodHard{
            fl, নূতন AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            নূতন AstCExpr{fl, AstCExpr::Pure{}, "\"" + constraint + "\""}};
        callp->dtypeSetVoid();
        ঘূৰাই callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* findStaticRandModeVarMember(AstClass* classp) ধ্রুৱ {
        যেতিয়ালৈ (সঁচা) {
            যদি (AstVar* ধ্রুৱ varp
                = VN_CAST(m_memberMap.findMember(classp, "__Vstaticrandmode"), Var)) {
                ঘূৰাই varp;
            }
            AstClassExtends* ধ্রুৱ extendsp = classp->extendsp();
            যদি (!extendsp) ঘূৰাই nullptr;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    শূন্য markConstrainedRandMember(AstStructDType* ধ্রুৱ structp, ধ্রুৱ std::string& name) {
        AstMemberDType* ধ্রুৱ memberp
            = VN_CAST(m_memberMap.findMember(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(সঁচা);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    তার্কিক isRandQualifiedPath(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যেতিয়ালৈ (nodep) {
            যদি (ধ্রুৱ AstStructSel* ধ্রুৱ selp = VN_CAST(nodep, StructSel)) {
                যদি (AstStructDType* ধ্রুৱ structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ধ্রুৱ AstMemberDType* ধ্রুৱ memberp
                        = VN_CAST(m_memberMap.findMember(structp, selp->name()), MemberDType);
                    যদি (memberp && !memberp->rand().isRandomizable()) ঘূৰাই মিছা;
                }
                nodep = selp->fromp();
            } অন্যথা যদি (ধ্রুৱ AstArraySel* ধ্রুৱ selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } অন্যথা যদি (ধ্রুৱ AstCMethodHard* ধ্রুৱ methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } অন্যথা {
                ভঙ্গ;
            }
        }
        ঘূৰাই সঁচা;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    স্থির শূন্য markStructConstrainedRandRecurse(Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep) {
        AstStructDType* ধ্রুৱ structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        যদি (!structp || structp->packed()) ঘূৰাই;
        structp->markConstrainedRand(সঁচা);
        বাবে (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            যদি (!memberp->rand().isRandomizable()) অনুৱর্ত;
            memberp->markConstrainedRand(সঁচা);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string buildMemberPath(ধ্রুৱ AstMemberSel* ধ্রুৱ memberSelp) {
        ধ্রুৱ Ast_গ্ৰন্থি* fromp = memberSelp->fromp();
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            ঘূৰাই refp->name() + "." + memberSelp->name();
        } অন্যথা যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            ঘূৰাই buildMemberPath(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        ঘূৰাই "";
    }

    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* getFromp(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nodep) {
        যদি (ধ্রুৱ AstCMethodHard* ধ্রুৱ cmethp = VN_CAST(nodep, CMethodHard)) {
            ঘূৰাই cmethp->fromp();
        }
        ঘূৰাই getSelFromp(nodep);
    }

    স্থির std::string extractSolveBeforeBaseName(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        যেতিয়ালৈ (ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = getFromp(exprp)) exprp = fromp;
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ vrefp = VN_CAST(exprp, VarRef)) ঘূৰাই vrefp->name();
        ঘূৰাই "";
    }

    স্থির শূন্য buildNamePrefix(AstCExpr* exprp, ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nodep) {
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(nodep, VarRef)) {
            exprp->add("(\""s + refp->name());
            ঘূৰাই;
        }

        ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = getFromp(nodep);
        যদি (fromp) buildNamePrefix(exprp, fromp);

        যদি (ধ্রুৱ AstStructSel* ধ্রুৱ selp = VN_CAST(nodep, StructSel)) {
            exprp->add("." + selp->name());
        } অন্যথা যদি (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            ঘূৰাই;
        } অন্যথা যদি (ধ্রুৱ AstCMethodHard* ধ্রুৱ cmethp = VN_CAST(nodep, CMethodHard)) {
            যদি (cmethp->method() == VCMethod::ARRAY_AT) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = cmethp->pinsp();
                exprp->add(".\" + ");
                AstCFuncHard* ধ্রুৱ solverHexFuncp
                    = নূতন AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(argp->clone_বৃক্ষ_Pure(মিছা));
                solverHexFuncp->dtypeSetString();
                exprp->add(solverHexFuncp);
                exprp->add(" + \"");
            } অন্যথা {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                ঘূৰাই;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    স্থির AstCExpr* build_Path_ৰাশিমালা(File_সাৰি* ধ্রুৱ fl, ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* nodep,
                                   ধ্রুৱ std::string selName) {
        AstCExpr* ধ্রুৱ exprp = নূতন AstCExpr{fl, ""};
        buildNamePrefix(exprp, nodep);
        যদি (selName != "") {
            exprp->add(".");
            exprp->add(selName);
        }
        exprp->add("\")");
        ঘূৰাই exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_ৰাশিমালা(File_সাৰি* fl, ধ্রুৱ std::string& baseName,
                                    ধ্রুৱ Ast_গ্ৰন্থি_Sel* selp) {
        AstCExpr* ধ্রুৱ p = নূতন AstCExpr{fl, ""};
        p->add("(\""s + baseName + ".\"");
        p->add("+");
        AstCFuncHard* ধ্রুৱ solverHexFuncp
            = নূতন AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
        solverHexFuncp->addPinsp(selp->bitp()->clone_বৃক্ষ_Pure(মিছা));
        solverHexFuncp->dtypeSetString();
        p->add(solverHexFuncp);
        p->add(")");
        p->dtypeSetString();
        ঘূৰাই p;
    }

    // Helper: get fromp from MemberSel or StructSel
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* getSelFromp(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ mp = VN_CAST(exprp, MemberSel)) ঘূৰাই mp->fromp();
        যদি (ধ্রুৱ AstStructSel* ধ্রুৱ sp = VN_CAST(exprp, StructSel)) ঘূৰাই sp->fromp();
        ঘূৰাই nullptr;
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* build_Solve_Before_Name_ৰাশিমালা(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ধ্রুৱ p = নূতন AstCExpr{fl, AstCExpr::Pure{}, "\"" + varrefp->name() + "\"s"};
            p->dtypeSetString();
            ঘূৰাই p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ selFromp = getSelFromp(exprp)) {
            ধ্রুৱ std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            যদি (ধ্রুৱ AstArraySel* ধ্রুৱ arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string baseName;
                যদি (ধ্রুৱ AstVarRef* ধ্রুৱ vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    baseName = vp->name();
                } অন্যথা যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    baseName = buildMemberPath(mp);
                }
                যদি (baseName.empty()) ঘূৰাই nullptr;
                AstCExpr* ধ্রুৱ p = নূতন AstCExpr{fl, ""};
                p->add("(\""s + baseName + ".\"");
                p->add("+");
                AstCFuncHard* ধ্রুৱ solverHexFuncp
                    = নূতন AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(arrSelp->bitp()->clone_বৃক্ষ_Pure(মিছা));
                solverHexFuncp->dtypeSetString();
                p->add(solverHexFuncp);
                p->add(")");
                p->dtypeSetString();
                ঘূৰাই p;
            }
            // Static member path (obj.field)
            যদি (ধ্রুৱ AstVarRef* ধ্রুৱ vp = VN_CAST(selFromp, VarRef)) {
                ধ্রুৱ std::string path = vp->name() + "." + selName;
                AstCExpr* ধ্রুৱ p = নূতন AstCExpr{fl, AstCExpr::Pure{}, "\"" + path + "\"s"};
                p->dtypeSetString();
                ঘূৰাই p;
            }
            যদি (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ধ্রুৱ p = build_Path_ৰাশিমালা(fl, selFromp, selName);
                p->dtypeSetString();
                ঘূৰাই p;
            }
            ঘূৰাই nullptr;
        }
        যদি (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ধ্রুৱ p = build_Path_ৰাশিমালা(fl, exprp, "");
            p->dtypeSetString();
            ঘূৰাই p;
        }
        ধ্রুৱ Ast_গ্ৰন্থি_Sel* selp = VN_CAST(exprp, ArraySel);
        যদি (!selp) selp = VN_CAST(exprp, AssocSel);
        যদি (selp) {
            // arr[i] -> dynamic name
            std::string baseName;
            যদি (ধ্রুৱ AstVarRef* ধ্রুৱ vp = VN_CAST(selp->fromp(), VarRef)) {
                baseName = vp->name();
            } অন্যথা যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ mp = VN_CAST(selp->fromp(), MemberSel)) {
                baseName = buildMemberPath(mp);
            }
            যদি (baseName.empty()) ঘূৰাই nullptr;
            ঘূৰাই build_Array_Sel_Name_ৰাশিমালা(fl, baseName, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        যদি (ধ্রুৱ AstSel* ধ্রুৱ bitSelp = VN_CAST(exprp, Sel)) {
            ঘূৰাই build_Solve_Before_Name_ৰাশিমালা(fl, bitSelp->fromp());
        }
        ঘূৰাই nullptr;
    }

    AstSFormatF* getConstFormat(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        ঘূৰাই নূতন AstSFormatF{nodep->fileline(), (nodep->width() & 3) ? "#b%b" : "#x%x", মিছা,
                               nodep};
    }
    তার্কিক editFormat(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (nodep->user1()) ঘূৰাই মিছা;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        handle.relink(getConstFormat(nodep));
        ঘূৰাই সঁচা;
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* wrapWithCond(Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp, Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp,
                              ধ্রুৱ পূর্ণাঙ্ক width) {
        যদি (condp) {
            File_সাৰি* ধ্রুৱ flp = exprp->fileline();
            ঘূৰাই নূতন AstCond{
                flp, condp, exprp,
                getConstFormat(নূতন AstConst{flp, AstConst::Widthed_মান{}, width, 0})};
        }
        ঘূৰাই exprp;
    }
    AstSFormatF* editSMT(Ast_গ্ৰন্থি_ৰাশিমালা* nodep, Ast_গ্ৰন্থি_ৰাশিমালা* lhsp = nullptr,
                         Ast_গ্ৰন্থি_ৰাশিমালা* rhsp = nullptr, Ast_গ্ৰন্থি_ৰাশিমালা* thsp = nullptr) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_ৰাশিমালা = nodep->emitSMT();  // Might need child width (AstExtend)
        যদি (smt_ৰাশিমালা == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            ঘূৰাই nullptr;
        }

        ধ্রুৱ পূর্ণাঙ্ক lhsWidth = lhsp ? lhsp->width() : 0;
        ধ্রুৱ পূর্ণাঙ্ক rhsWidth = rhsp ? rhsp->width() : 0;
        ধ্রুৱ পূর্ণাঙ্ক thsWidth = thsp ? thsp->width() : 0;
        Ast_গ্ৰন্থি_ৰাশিমালা* lhsCondp = nullptr;
        Ast_গ্ৰন্থি_ৰাশিমালা* rhsCondp = nullptr;
        Ast_গ্ৰন্থি_ৰাশিমালা* thsCondp = nullptr;

        যদি (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlinkFrBack() : lhsp),
                         গ্ৰন্থি_ৰাশিমালা);
            lhsCondp = m_conditionp;
        }
        যদি (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlinkFrBack() : rhsp),
                         গ্ৰন্থি_ৰাশিমালা);
            rhsCondp = m_conditionp;
        }
        যদি (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlinkFrBack() : thsp),
                         গ্ৰন্থি_ৰাশিমালা);
            thsCondp = m_conditionp;
        }

        Ast_গ্ৰন্থি_ৰাশিমালা* argsp = nullptr;
        বাবে (string::iterator pos = smt_ৰাশিমালা.begin(); pos != smt_ৰাশিমালা.end(); ++pos) {
            যদি (pos[0] == '%') {
                ++pos;
                ৱিকল্প (pos[0]) {
                পক্ষ '%': ভঙ্গ;
                পক্ষ 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_গ্ৰন্থি::addNext(argsp, wrapWithCond(lhsp, lhsCondp, lhsWidth));
                    lhsp = nullptr;
                    ভঙ্গ;
                পক্ষ 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_গ্ৰন্থি::addNext(argsp, wrapWithCond(rhsp, rhsCondp, rhsWidth));
                    rhsp = nullptr;
                    ভঙ্গ;
                পক্ষ 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_গ্ৰন্থি::addNext(argsp, wrapWithCond(thsp, thsCondp, thsWidth));
                    thsp = nullptr;
                    ভঙ্গ;
                স্ৱভাৱ: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); ভঙ্গ;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ধ্রুৱ newp = নূতন AstSFormatF{nodep->fileline(), smt_ৰাশিমালা, মিছা, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        ঘূৰাই newp;
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* editSingle(File_সাৰি* fl, Ast_গ্ৰন্থি* itemsp) {
        যদি (!itemsp) ঘূৰাই nullptr;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = সঁচা;

        {
            AstBegin* ধ্রুৱ tempp
                = নূতন AstBegin{fl, "[EditWrapper]", itemsp->unlinkFrBackWithNext(), মিছা};
            VL_DO_DANGLING(iterateAndNextNull(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            যদি (itemsp) itemsp->unlinkFrBackWithNext();
            VL_DO_DANGLING(tempp->delete_বৃক্ষ(), tempp);
        }
        যদি (!itemsp) ঘূৰাই nullptr;

        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprsp = VN_CAST(itemsp, গ্ৰন্থি_ৰাশিমালা);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        যদি (!exprsp->nextp()) ঘূৰাই exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        বাবে (Ast_গ্ৰন্থি* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        ঘূৰাই নূতন AstSFormatF{fl, fmt.str(), মিছা, exprsp};
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* newSel(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* arrayp, Ast_গ্ৰন্থি_ৰাশিমালা* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_গ্ৰন্থি_DType* ধ্রুৱ arrDtp = arrayp->unlinkFrBack()->dtypep()->skipRefp();
        Ast_গ্ৰন্থি_ৰাশিমালা* selp = nullptr;
        যদি (VN_IS(arrDtp, QueueDType) || VN_IS(arrDtp, DynArrayDType))
            selp = নূতন AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        অন্যথা যদি (VN_IS(arrDtp, UnpackArrayDType))
            selp = নূতন AstArraySel{fl, arrayp, idxp};
        অন্যথা যদি (VN_IS(arrDtp, AssocArrayDType))
            selp = নূতন AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        ঘূৰাই selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_গ্ৰন্থি_ৰাশিমালা* adjustWidth(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* exprp, পূর্ণাঙ্ক targetWidth,
                             VSigning targetSigning) {
        ধ্রুৱ পূর্ণাঙ্ক ৰাশিমালা_Width = exprp->width();
        যদি (targetWidth > ৰাশিমালা_Width) {
            // Extend to match target width
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ result = নূতন AstExtend{fl, exprp, targetWidth};
            result->dtypeChgSigned(targetSigning);
            ঘূৰাই result;
        } অন্যথা যদি (targetWidth < ৰাশিমালা_Width) {
            // Truncate to match target width
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ result = নূতন AstSel{fl, exprp, 0, targetWidth};
            result->dtypeChgSigned(targetSigning);
            ঘূৰাই result;
        } অন্যথা {
            // Width already matches
            ঘূৰাই exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_গ্ৰন্থি_ৰাশিমালা* extract_Return_ৰাশিমালা(AstFunc* funcp) {
        AstVar* ধ্রুৱ retVarp = VN_CAST(funcp->fvarp(), Var);
        যদি (!retVarp) ঘূৰাই nullptr;
        Ast_গ্ৰন্থি_ৰাশিমালা* retExprp = nullptr;
        পূর্ণাঙ্ক assign_গুনতি = 0;
        funcp->foreach([&](AstAssign* assignp) {
            AstVarRef* ধ্রুৱ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            যদি (!lhsp || lhsp->varp() != retVarp) ঘূৰাই;
            retExprp = assignp->rhsp();
            ++assign_গুনতি;
        });
        // Simple function: initializer + return (2) or just return (1)
        যদি (assign_গুনতি > 2 || !retExprp) ঘূৰাই nullptr;
        ঘূৰাই retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    শূন্য propagateUser1InlineRecurse(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) {
        যদি (VN_IS(nodep, গ্ৰন্থি_Var_Ref)) {
            nodep->user1(VN_AS(nodep, গ্ৰন্থি_Var_Ref)->varp()->rand().isRandomizable());
            ঘূৰাই;
        }
        যদি (VN_IS(nodep, Const)) {
            nodep->user1(মিছা);
            ঘূৰাই;
        }
        পূর্ণাঙ্ক anyChild = মিছা;  // Used as bool
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cp = VN_CAST(nodep->op1p(), গ্ৰন্থি_ৰাশিমালা)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cp = VN_CAST(nodep->op2p(), গ্ৰন্থি_ৰাশিমালা)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cp = VN_CAST(nodep->op3p(), গ্ৰন্থি_ৰাশিমালা)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ cp = VN_CAST(nodep->op4p(), গ্ৰন্থি_ৰাশিমালা)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        nodep->user1(anyChild);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    স্থির Ast_গ্ৰন্থি* prependDistPreamble(AstConstraintForeach* nodep, Ast_গ্ৰন্থি* bodyp) {
        Ast_গ্ৰন্থি* ধ্রুৱ preamblep = nodep->user3p();
        যদি (!preamblep) ঘূৰাই bodyp;
        nodep->user3p(nullptr);
        preamblep->addNext(bodyp);
        ঘূৰাই preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    তার্কিক referencesForeachIdx(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ {
        যদি (m_foreachIdxVars.empty()) ঘূৰাই মিছা;
        ঘূৰাই nodep->exists(
            [&](ধ্রুৱ AstVarRef* refp) { ঘূৰাই m_foreachIdxVars.গুনতি(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    শূন্য addRegistrationStmt(Ast_গ্ৰন্থি* ধ্রুৱ stmtp, Ast_গ্ৰন্থি_FTask* ধ্রুৱ initTaskp) {
        যদি (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->addBodyp(stmtp);
        } অন্যথা {
            initTaskp->addStmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_সাৰি* ধ্রুৱ fl, Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ arrExprp,
                                       Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ idxExprp) {
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argsp = Ast_গ্ৰন্থি::addNext(arrExprp, idxExprp);
        AstSFormatF* ধ্রুৱ formatp = নূতন AstSFormatF{fl, "(select %s %s)", মিছা, argsp};
        ঘূৰাই formatp;
    }

    শূন্য setRandMode(AstVar* ধ্রুৱ varp, AstMemberSel* ধ্রুৱ memberselp,
                     ধ্রুৱ std::string& smtName, ধ্রুৱ RandomizeMode& randMode,
                     Ast_গ্ৰন্থি_FTask* ধ্রুৱ initTaskp) {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ varClassp = VN_AS(varp->user2p(), গ্ৰন্থি_Module);
        AstVar* ধ্রুৱ subRandModeVarp = getRandModeVarFromClass(varClassp);
        যদি (subRandModeVarp) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ parentAccessp = memberselp->fromp()->clone_বৃক্ষ(মিছা);
            AstMemberSel* ধ্রুৱ randModeSelp
                = নূতন AstMemberSel{varp->fileline(), parentAccessp, subRandModeVarp};
            randModeSelp->dtypep(subRandModeVarp->dtypep());
            AstCMethodHard* ধ্রুৱ atp
                = নূতন AstCMethodHard{varp->fileline(), randModeSelp, VCMethod::ARRAY_AT,
                                     নূতন AstConst{varp->fileline(), randMode.index}};
            atp->dtypeSetUInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ধ্রুৱ enablep = নূতন AstCMethodHard{
                varp->fileline(),
                নূতন AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), গ্ৰন্থি_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtypeSetVoid();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : নূতন AstSFormatF{varp->fileline(), smtName, মিছা, nullptr};
            nameExprp->dtypep(varp->dtypep());
            enablep->addPinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ধ্রুৱ disablep = নূতন AstCMethodHard{
                varp->fileline(),
                নূতন AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), গ্ৰন্থি_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_SET_VAR_DISABLED};
            disablep->dtypeSetVoid();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ disnp = nameExprp->clone_বৃক্ষ(মিছা);
            disablep->addPinsp(disnp);
            AstIf* ধ্রুৱ ifp
                = নূতন AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            addRegistrationStmt(ifp, initTaskp);
        }
    }

    শূন্য markRandc(AstVar* ধ্রুৱ varp, ধ্রুৱ std::string& smtName, Ast_গ্ৰন্থি_FTask* ধ্রুৱ initTaskp) {
        AstCMethodHard* ধ্রুৱ markp = নূতন AstCMethodHard{
            varp->fileline(),
            নূতন AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), গ্ৰন্থি_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtypeSetVoid();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : নূতন AstSFormatF{varp->fileline(), smtName, মিছা, nullptr};
        nameExprp->dtypep(varp->dtypep());
        markp->addPinsp(nameExprp);
        addRegistrationStmt(markp->makeStmt(), initTaskp);
    }

    Ast_গ্ৰন্থি_Module* getLeftmostVarModulep(AstMemberSel* ধ্রুৱ memberselp,
                                         AstVar* ধ্রুৱ varp) ধ্রুৱ {
        যদি (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_গ্ৰন্থি* বাম_Most_Nodep = memberselp->fromp();
            যেতিয়ালৈ (AstMemberSel* ধ্রুৱ nestedMemberSelp = VN_CAST(বাম_Most_Nodep, MemberSel)) {
                বাম_Most_Nodep = nestedMemberSelp->fromp();
            }
            যদি (Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ মূল_Var_Refp = VN_CAST(বাম_Most_Nodep, গ্ৰন্থি_Var_Ref)) {
                ঘূৰাই VN_AS(মূল_Var_Refp->varp()->user2p(), গ্ৰন্থি_Module);
            }
            ঘূৰাই VN_AS(memberselp->user2p(), গ্ৰন্থি_Module);
        }
        ঘূৰাই VN_AS(varp->user2p(), গ্ৰন্থি_Module);
    }

    Ast_গ্ৰন্থি_FTask* getInitTaskp(AstVar* ধ্রুৱ varp, ধ্রুৱ তার্কিক memberselOrStructSel,
                               Ast_গ্ৰন্থি_Module* ধ্রুৱ classp) ধ্রুৱ {
        যদি (m_inlineInitTaskp) ঘূৰাই m_inlineInitTaskp;
        varp->user3(সঁচা);
        যদি (memberselOrStructSel) {
            যদি (Ast_গ্ৰন্থি_FTask* ধ্রুৱ initTaskp
                = VN_AS(m_memberMap.findMember(classp, "randomize"), গ্ৰন্থি_FTask)) {
                ঘূৰাই initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            যদি (m_memberselInitTaskp) ঘূৰাই m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } অন্যথা {
            Ast_গ্ৰন্থি_FTask* ধ্রুৱ initTaskp
                = VN_AS(m_memberMap.findMember(classp, "new"), গ্ৰন্থি_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            ঘূৰাই initTaskp;
        }
        ঘূৰাই nullptr;
    }

    শূন্য createWriteVarForClassFields(AstClass* ধ্রুৱ elemClassp, ধ্রুৱ std::string& smtName,
                                      AstVar* ধ্রুৱ iterVarp, ধ্রুৱ তার্কিক isUnpackedClassRefArray,
                                      Ast_গ্ৰন্থি_Module* ধ্রুৱ classOrPackagep, AstVar* ধ্রুৱ varp,
                                      AstIf* ধ্রুৱ ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ধ্রুৱ {
        File_সাৰি* ধ্রুৱ fl = varp->fileline();
        Ast_গ্ৰন্থি_Module* ধ্রুৱ varClassp = VN_AS(varp->user2p(), গ্ৰন্থি_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr পূর্ণাঙ্ক idxWidth = 32;
        ধ্রুৱ পূর্ণাঙ্ক fmtWidth = VL_শব্দগুলি_I(idxWidth) * 8;

        বাবে (ধ্রুৱ AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            বাবে (Ast_গ্ৰন্থি* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ধ্রুৱ memberVarp = VN_CAST(mnodep, Var);
                যদি (!memberVarp || !memberVarp->rand().isRandomizable()) অনুৱর্ত;
                Ast_গ্ৰন্থি_DType* ধ্রুৱ memberDtp = memberVarp->dtypep()->skipRefp();
                যদি (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) অনুৱর্ত;
                ধ্রুৱ পূর্ণাঙ্ক memberWidth = memberDtp->width();

                AstCStmt* ধ্রুৱ fmtp = নূতন AstCStmt{fl};
                যদি (isUnpackedClassRefArray) {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } অন্যথা {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::to_string(fmtWidth) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->add(নূতন AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->add(");\n");
                ifNonNullp->addThensp(fmtp);

                AstVarRef* ধ্রুৱ arrayWrRefp = নূতন AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_গ্ৰন্থি_ৰাশিমালা* atWritep;
                যদি (isUnpackedClassRefArray) {
                    atWritep = নূতন AstArraySel{fl, arrayWrRefp,
                                               নূতন AstVarRef{fl, iterVarp, VAccess::READ}};
                } অন্যথা {
                    atWritep = নূতন AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  নূতন AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* ধ্রুৱ memberSelp = নূতন AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* ধ্রুৱ writeVarp
                    = নূতন AstCMethodHard{fl,
                                         নূতন AstVarRef{fl, VN_AS(m_genp->user2p(), গ্ৰন্থি_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtypeSetVoid();
                writeVarp->addPinsp(memberSelp);
                writeVarp->addPinsp(
                    নূতন AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(memberWidth)});
                AstCExpr* ধ্রুৱ nameRefp = নূতন AstCExpr{fl, AstCExpr::Pure{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->addPinsp(nameRefp);
                writeVarp->addPinsp(নূতন AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->addThensp(writeVarp->makeStmt());
            }
        }
    }

    শূন্য createSolverArrayHandle(AstVar* ধ্রুৱ varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_গ্ৰন্থি_Module* ধ্রুৱ classOrPackagep,
                                 ধ্রুৱ তার্কিক isUnpackedClassRefArray,
                                 ধ্রুৱ std::string& smtName) ধ্রুৱ {
        File_সাৰি* ধ্রুৱ fl = varp->fileline();
        AstClass* ধ্রুৱ elemClassp = elemClassRefDtp->classp();
        Ast_গ্ৰন্থি_Module* ধ্রুৱ varClassp = VN_AS(varp->user2p(), গ্ৰন্থি_Module);

        AstVar* ধ্রুৱ iterVarp = নূতন AstVar{fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__Vi"),
                                            varp->findUInt32DType()};
        iterVarp->funcLocal(সঁচা);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_গ্ৰন্থি* ধ্রুৱ stmtsp
            = নূতন AstAssign{fl, নূতন AstVarRef{fl, iterVarp, VAccess::WRITE}, নূতন AstConst{fl, 0}};

        Ast_গ্ৰন্থি_ৰাশিমালা* sizep;
        যদি (isUnpackedClassRefArray) {
            ধ্রুৱ পূর্ণাঙ্ক arraySize
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = নূতন AstConst{fl, static_cast<uint32_t>(arraySize)};
        } অন্যথা {
            AstVarRef* ধ্রুৱ arraySizeRefp = নূতন AstVarRef{fl, varClassp, varp, VAccess::READ};
            arraySizeRefp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ধ্রুৱ dynSizep
                = নূতন AstCMethodHard{fl, arraySizeRefp, VCMethod::DYN_SIZE, nullptr};
            dynSizep->dtypeSetUInt32();
            sizep = dynSizep;
        }

        AstLoop* ধ্রুৱ loopp = নূতন AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(নূতন AstLoopTest{
            fl, loopp, নূতন AstLt{fl, নূতন AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ধ্রুৱ arrayReadRefp = নূতন AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_গ্ৰন্থি_ৰাশিমালা* atReadp;
        যদি (isUnpackedClassRefArray) {
            atReadp
                = নূতন AstArraySel{fl, arrayReadRefp, নূতন AstVarRef{fl, iterVarp, VAccess::READ}};
        } অন্যথা {
            atReadp = নূতন AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         নূতন AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ধ্রুৱ ifNonNullp
            = নূতন AstIf{fl, নূতন AstNeq{fl, atReadp, নূতন AstConst{fl, AstConst::Null{}}}, nullptr};
        loopp->addStmtsp(ifNonNullp);

        AstCStmt* ধ্রুৱ bufDeclp = নূতন AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->addThensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->addStmtsp(নূতন AstAssign{
            fl, নূতন AstVarRef{fl, iterVarp, VAccess::WRITE},
            নূতন AstAdd{fl, নূতন AstConst{fl, 1}, নূতন AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(সঁচা);
        Ast_গ্ৰন্থি_FTask* initTaskp = m_inlineInitTaskp;
        যদি (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.findMember(varClassp, "new"), গ্ৰন্থি_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->addStmtsp(stmtsp);
    }

    শূন্য createSolverVarHandle(AstVar* ধ্রুৱ varp, ধ্রুৱ তার্কিক structSelOrCMeth,
                               ধ্রুৱ তার্কিক isGlobalConstrained, ধ্রুৱ RandomizeMode randMode,
                               AstMemberSel* ধ্রুৱ memberselp, ধ্রুৱ std::string& smtName,
                               Ast_গ্ৰন্থি_Module* ধ্রুৱ classOrPackagep, Ast_গ্ৰন্থি_Module* ধ্রুৱ classp,
                               Ast_গ্ৰন্থি_FTask* ধ্রুৱ initTaskp) {
        uint32_t unpackedDims = 0;
        যদি (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(মিছা).second;
        }
        যদি (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ধ্রুৱ methodp = নূতন AstCMethodHard{
            varp->fileline(),
            নূতন AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), গ্ৰন্থি_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();
        যদি (memberselp) {
            methodp->addPinsp(memberselp);
        } অন্যথা {
            AstVarRef* ধ্রুৱ varRefp
                = নূতন AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->addPinsp(varRefp);
        }
        ধ্রুৱ size_t width = arrayElementDTypep(varp->dtypep())->width();
        methodp->addPinsp(নূতন AstConst{varp->fileline(), AstConst::Unsized64{}, width});
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ varnamep = নূতন AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                   "\"" + smtName + "\"", varp->width()};
        varnamep->dtypep(varp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(
            নূতন AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        যদি (randMode.usesMode && !(isGlobalConstrained && memberselp)) {
            methodp->addPinsp(
                নূতন AstConst{varp->fileline(), AstConst::Unsized64{}, randMode.index});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->addStmtsp(methodp->makeStmt());
        যদি (varp->lifetime().isStatic() && randMode.usesMode) {
            AstCMethodHard* ধ্রুৱ markp = নূতন AstCMethodHard{
                varp->fileline(),
                নূতন AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), গ্ৰন্থি_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ namep = নূতন AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                    "\"" + smtName + "\"", varp->width()};
            namep->dtypep(varp->dtypep());
            markp->addPinsp(namep);
            markp->dtypeSetVoid();
            initTaskp->addStmtsp(markp->makeStmt());
        }
        যদি (isGlobalConstrained && memberselp && randMode.usesMode) {
            setRandMode(varp, memberselp, smtName, randMode, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        যদি (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* createVarExprp(Ast_গ্ৰন্থি_Var_Ref* nodep, ধ্রুৱ RandomizeMode randMode,
                                ধ্রুৱ std::string& smtName, ধ্রুৱ তার্কিক isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ধ্রুৱ varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_গ্ৰন্থি_ৰাশিমালা* exprp = নূতন AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, মিছা, nullptr};
        যদি (!randMode.usesMode) {
            // Global constraints keep nodep alive for write_var processing
            যদি (!isGlobalConstrained) VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_গ্ৰন্থি_ৰাশিমালা* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_বৃক্ষ(মিছা)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_গ্ৰন্থি_ৰাশিমালা* randModeAccess;
        ধ্রুৱ তার্কিক varIsStatic = varp->lifetime().isStatic();
        AstClass* ধ্রুৱ varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : nullptr;
        AstVar* ধ্রুৱ ownerStaticRandModeVarp
            = varOwningClassp ? findStaticRandModeVarMember(varOwningClassp) : nullptr;
        যদি (ownerStaticRandModeVarp) {
            randModeAccess = নূতন AstVarRef{varp->fileline(),
                                           VN_AS(ownerStaticRandModeVarp->user2p(), গ্ৰন্থি_Module),
                                           ownerStaticRandModeVarp, VAccess::READ};
        } অন্যথা যদি (memberselp) {
            Ast_গ্ৰন্থি_Module* ধ্রুৱ varClassp = VN_AS(varp->user2p(), গ্ৰন্থি_Module);
            AstVar* ধ্রুৱ effectiveRandModeVarp = getRandModeVarFromClass(varClassp);
            যদি (effectiveRandModeVarp) {
                // Member's class has randmode, use it
                Ast_গ্ৰন্থি_ৰাশিমালা* parentAccess = memberselp->fromp()->clone_বৃক্ষ(মিছা);
                AstMemberSel* randModeSel
                    = নূতন AstMemberSel{varp->fileline(), parentAccess, effectiveRandModeVarp};
                randModeSel->dtypep(effectiveRandModeVarp->dtypep());
                randModeAccess = randModeSel;
            } অন্যথা {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
                randModeAccess
                    = নূতন AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), গ্ৰন্থি_Module),
                                    m_randModeVarp, VAccess::READ};
            }
        } অন্যথা {
            UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
            randModeAccess
                = নূতন AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), গ্ৰন্থি_Module),
                                m_randModeVarp, VAccess::READ};
        }

        AstCMethodHard* ধ্রুৱ atp
            = নূতন AstCMethodHard{nodep->fileline(), randModeAccess, VCMethod::ARRAY_AT,
                                 নূতন AstConst{nodep->fileline(), randMode.index}};
        atp->dtypeSetUInt32();
        exprp = নূতন AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(সঁচা);  // Mark as formatted
        ঘূৰাই exprp;
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        যদি (varp->user4p()) {
            তার্কিক isSizeRef = মিছা;
            যদি (AstCMethodHard* ধ্রুৱ methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                যদি (methodp->method() == VCMethod::ASSOC_SIZE
                    || methodp->method() == VCMethod::DYN_SIZE) {
                    isSizeRef = সঁচা;
                }
            }
            যদি (!isSizeRef && m_sizeConstrainedArraysp) { m_sizeConstrainedArraysp->ঢোকাও(varp); }
        }

        // Check if this variable is marked as globally constrained
        ধ্রুৱ তার্কিক isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = nullptr;
        তার্কিক structSelOrCMeth = মিছা;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_গ্ৰন্থি_DType* selectedDtypep = nullptr;
        std::string smtName;
        যদি (m_nestedAccess) {
            m_nestedAccess->addVarNamePart(
                নূতন AstSFormatF{varp->fileline(), nodep->name(), মিছা, nullptr}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_বৃক্ষ();
        } অন্যথা যদি (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_গ্ৰন্থি* শীৰ্ষ_Member_Sel = nodep->backp();
            যেতিয়ালৈ (VN_IS(শীৰ্ষ_Member_Sel->backp(), MemberSel)) {
                শীৰ্ষ_Member_Sel = শীৰ্ষ_Member_Sel->backp();
            }
            memberselp = VN_AS(শীৰ্ষ_Member_Sel, MemberSel)->clone_বৃক্ষ(মিছা);
            smtName = buildMemberPath(memberselp);
        } অন্যথা যদি (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_গ্ৰন্থি* শীৰ্ষ_Nodep = nodep->backp();
            যেতিয়ালৈ (VN_IS(শীৰ্ষ_Nodep->backp(), StructSel) || VN_IS(শীৰ্ষ_Nodep->backp(), CMethodHard)) {
                শীৰ্ষ_Nodep = শীৰ্ষ_Nodep->backp();
            }

            যদি (VN_IS(শীৰ্ষ_Nodep, StructSel) || VN_IS(শীৰ্ষ_Nodep, CMethodHard)) {
                structSelOrCMeth = সঁচা;
                selectedDtypep = VN_AS(শীৰ্ষ_Nodep, গ্ৰন্থি_ৰাশিমালা)->dtypep();
            }

            memberselp = VN_CAST(শীৰ্ষ_Nodep, MemberSel);
            যদি (memberselp) smtName = buildMemberPath(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            যদি (smtName == "") smtName = nodep->name();
        } অন্যথা {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        যদি (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        যদি (dtypeContainsReal(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            ঘূৰাই;
        }
        Ast_গ্ৰন্থি_Module* ধ্রুৱ classOrPackagep = nodep->classOrPackagep();
        ধ্রুৱ RandomizeMode randMode = {.asUQuad = varp->user1()};
        যদি (!randMode.usesMode && editFormat(nodep)) ঘূৰাই;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skipRefToEnump(), EnumDType);
        যদি (enumDtp
            && (std::find(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.end(), varp)
                == m_rangeConstrainedEnums.end())) {
            AstVar* genVarp = m_genp;
            যদি (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_গ্ৰন্থি_ৰাশিমালা* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_গ্ৰন্থি_FTask* targetTaskp = m_inlineInitTaskp;
            যদি (!targetTaskp) targetTaskp = getCreateConstraintSetupFunc(m_classp, m_memberMap);
            UASSERT_OBJ(targetTaskp, nodep, "No function to inline enum range constraint into");
            targetTaskp->addStmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.ঠেলো_back(varp);
        }

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp
            = createVarExprp(nodep, randMode, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ধ্রুৱ তার্কিক alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.গুনতি(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.গুনতি(smtName) > 0
                                    : memberselp          ? m_writtenVars.গুনতি(smtName) > 0
                                                          : varp->user3();
        ধ্রুৱ তার্কিক shouldWriteVar = !alreadyWritten;
        যদি (shouldWriteVar) {
            // Track this variable path as written
            যদি (m_inlineInitTaskp)
                m_inlineWrittenVars.ঢোকাও(smtName);
            অন্যথা যদি (isGlobalConstrained || memberselp)
                m_writtenVars.ঢোকাও(smtName);
            // For global constraints, delete nodep after processing
            যদি (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);

            তার্কিক isClassRefArray = মিছা;
            তার্কিক isUnpackedClassRefArray = মিছা;
            AstClassRefDType* elemClassRefDtp = nullptr;
            {
                Ast_গ্ৰন্থি_DType* varDtp = varp->dtypep()->skipRefp();
                যদি (varDtp->isNonPackedArray()) {
                    Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    যদি (elemClassRefDtp) {
                        isClassRefArray = সঁচা;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_গ্ৰন্থি_Module* ধ্রুৱ classp = getLeftmostVarModulep(memberselp, varp);
            Ast_গ্ৰন্থি_FTask* ধ্রুৱ initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            যদি (m_nestedAccess) {
                Ast_Stmt_ৰাশিমালা* ধ্রুৱ writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                addRegistrationStmt(writeVarStmtp, initTaskp);
                যদি (isGlobalConstrained && memberselp && randMode.usesMode) {
                    setRandMode(varp, memberselp, smtName, randMode, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                যদি (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } অন্যথা যদি (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } অন্যথা {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, randMode,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } অন্যথা {
            // Variable already written, clean up cloned memberselp if any
            যদি (memberselp) VL_DO_DANGLING(memberselp->delete_বৃক্ষ(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            যদি (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
        যদি (m_nestedAccess) { VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_গ্ৰন্থি_ৰাশিমালা* build_গুনতি_Ones_Expansion(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* argp) {
        ধ্রুৱ V3_সংখ্যা অঙ্ক_One{fl, argp->width(), 1};
        Ast_গ্ৰন্থি_ৰাশিমালা* sump = নূতন AstAnd{fl, argp, নূতন AstConst{fl, অঙ্ক_One}};
        sump->user1(সঁচা);
        বাবে (পূর্ণাঙ্ক i = 1; i < argp->width(); i++) {
            V3_সংখ্যা অঙ্ক_Bit_Mask{fl, argp->width(), 0};
            অঙ্ক_Bit_Mask.setBit(i, 1);
            AstAnd* ধ্রুৱ andp
                = নূতন AstAnd{fl, argp->clone_বৃক্ষ_Pure(মিছা), নূতন AstConst{fl, অঙ্ক_Bit_Mask}};
            andp->user1(সঁচা);
            AstShiftR* ধ্রুৱ shiftp
                = নূতন AstShiftR{fl, andp,
                                নূতন AstConst{fl, AstConst::Widthed_মান{}, argp->width(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(সঁচা);
            sump = নূতন AstAdd{fl, sump, shiftp};
            sump->user1(সঁচা);
        }
        ঘূৰাই sump;
    }

    শূন্য visit(Ast_গুনতি_Ones* nodep) override {
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = nodep->lhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* sump = build_গুনতি_Ones_Expansion(fl, argp);
        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(সঁচা);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(sump);
    }
    শূন্য visit(AstRedXor* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = nodep->lhsp()->unlinkFrBack();

        Ast_গ্ৰন্থি_ৰাশিমালা* redxorp = নূতন AstSel{fl, argp, 0, 1};
        redxorp->user1(সঁচা);
        বাবে (পূর্ণাঙ্ক i = 1; i < argp->width(); i++) {
            AstSel* ধ্রুৱ selp = নূতন AstSel{fl, argp->clone_বৃক্ষ_Pure(মিছা), i, 1};
            selp->user1(সঁচা);

            redxorp = নূতন AstXor{fl, redxorp, selp};
            redxorp->user1(সঁচা);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(redxorp);
    }
    শূন্য visit(AstRedAnd* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        // Convert to (~x == 0)
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = nodep->lhsp()->unlinkFrBack();
        ধ্রুৱ V3_সংখ্যা অঙ্ক_Zero{fl, argp->width(), 0};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ negp = নূতন AstNot{fl, argp};
        negp->user1(সঁচা);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ eqp = নূতন AstEq{fl, negp, নূতন AstConst{fl, অঙ্ক_Zero}};
        eqp->user1(সঁচা);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(eqp);
    }
    শূন্য visit(AstRedOr* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        // Convert to (x != 0)
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = nodep->lhsp()->unlinkFrBack();
        ধ্রুৱ V3_সংখ্যা অঙ্ক_Zero{fl, argp->width(), 0};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ neqp = নূতন AstNeq{fl, argp, নূতন AstConst{fl, অঙ্ক_Zero}};
        neqp->user1(সঁচা);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(neqp);
    }
    শূন্য visit(AstOneHot* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = nodep->lhsp()->unlinkFrBack();
        ধ্রুৱ পূর্ণাঙ্ক w = argp->width();

        ধ্রুৱ V3_সংখ্যা অঙ্ক_Zero{fl, w, 0};
        AstNeq* ধ্রুৱ neZerop
            = নূতন AstNeq{fl, argp->clone_বৃক্ষ_Pure(মিছা), নূতন AstConst{fl, অঙ্ক_Zero}};
        neZerop->user1(সঁচা);

        ধ্রুৱ V3_সংখ্যা অঙ্ক_One{fl, w, 1};
        AstSub* ধ্রুৱ subp = নূতন AstSub{fl, argp->clone_বৃক্ষ_Pure(মিছা), নূতন AstConst{fl, অঙ্ক_One}};
        subp->user1(সঁচা);

        AstAnd* ধ্রুৱ andp = নূতন AstAnd{fl, argp, subp};
        andp->user1(সঁচা);

        ধ্রুৱ V3_সংখ্যা অঙ্ক_Zero2{fl, w, 0};
        AstEq* ধ্রুৱ eqZerop = নূতন AstEq{fl, andp, নূতন AstConst{fl, অঙ্ক_Zero2}};
        eqZerop->user1(সঁচা);

        AstLogAnd* ধ্রুৱ resultp = নূতন AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(সঁচা);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(resultp);
    }
    শূন্য visit(AstOneHot0* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        // $onehot0(x) = (x & (x-1)) == 0
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = nodep->lhsp()->unlinkFrBack();
        ধ্রুৱ পূর্ণাঙ্ক w = argp->width();

        ধ্রুৱ V3_সংখ্যা অঙ্ক_One{fl, w, 1};
        AstSub* ধ্রুৱ subp = নূতন AstSub{fl, argp->clone_বৃক্ষ_Pure(মিছা), নূতন AstConst{fl, অঙ্ক_One}};
        subp->dtypeFrom(argp);
        subp->user1(সঁচা);

        AstAnd* ধ্রুৱ andp = নূতন AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(সঁচা);

        ধ্রুৱ V3_সংখ্যা অঙ্ক_Zero{fl, w, 0};
        AstEq* ধ্রুৱ eqp = নূতন AstEq{fl, andp, নূতন AstConst{fl, অঙ্ক_Zero}};
        eqp->user1(সঁচা);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(eqp);
    }
    শূন্য visit(Ast_গুনতি_Bits* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();

        তার্কিক গুনতি_Ones = মিছা;
        তার্কিক গুনতি_Zeros = মিছা;
        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ধ্রুৱ AstConst* ধ্রুৱ cp = VN_CAST(ctrlp, Const);
            যদি (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ধ্রুৱ zerop
                    = নূতন AstConst{fl, AstConst::Widthed_মান{}, nodep->width(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                ঘূৰাই;
            }
            যদি (cp->অঙ্ক().bitIs1(0) && !গুনতি_Ones)
                গুনতি_Ones = সঁচা;
            অন্যথা যদি (cp->অঙ্ক().bitIs0(0) && !গুনতি_Zeros)
                গুনতি_Zeros = সঁচা;
        }

        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = nodep->lhsp()->unlinkFrBack();
        ধ্রুৱ পূর্ণাঙ্ক argWidth = argp->width();
        Ast_গ্ৰন্থি_ৰাশিমালা* sump = nullptr;
        যদি (গুনতি_Ones && গুনতি_Zeros) {
            // ones + zeros = width for 2-state types
            sump = নূতন AstConst{fl, AstConst::Widthed_মান{}, nodep->width(), (uint32_t)argWidth};
            VL_DO_DANGLING(argp->delete_বৃক্ষ(), argp);
        } অন্যথা যদি (গুনতি_Ones) {
            sump = build_গুনতি_Ones_Expansion(fl, argp);
        } অন্যথা যদি (গুনতি_Zeros) {
            // width - countones(x)
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ onesCountp = build_গুনতি_Ones_Expansion(fl, argp);
            ধ্রুৱ V3_সংখ্যা widthVal{nodep, onesCountp->width(), (uint32_t)argWidth};
            sump = নূতন AstSub{fl, নূতন AstConst{fl, widthVal}, onesCountp};
            sump->user1(সঁচা);
        } অন্যথা {
            sump = নূতন AstConst{fl, AstConst::Widthed_মান{}, nodep->width(), 0U};
            VL_DO_DANGLING(argp->delete_বৃক্ষ(), argp);
        }

        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(সঁচা);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(sump);
    }
    শূন্য visit(AstCLog2* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argp = nodep->lhsp()->unlinkFrBack();
        ধ্রুৱ পূর্ণাঙ্ক argWidth = argp->width();
        ধ্রুৱ পূর্ণাঙ্ক resultWidth = nodep->width();

        Ast_গ্ৰন্থি_ৰাশিমালা* resultp
            = নূতন AstConst{fl, AstConst::Widthed_মান{}, resultWidth, (uint32_t)argWidth};

        বাবে (পূর্ণাঙ্ক k = argWidth - 1; k >= 0; k--) {
            V3_সংখ্যা threshold{fl, argWidth, 0};
            যদি (k < 32)
                threshold.setLong(1ULL << k);
            অন্যথা
                threshold.setBit(k, 1);

            AstLte* ধ্রুৱ ltep
                = নূতন AstLte{fl, argp->clone_বৃক্ষ_Pure(মিছা), নূতন AstConst{fl, threshold}};
            ltep->user1(সঁচা);

            AstConst* ধ্রুৱ valuep
                = নূতন AstConst{fl, AstConst::Widthed_মান{}, resultWidth, (uint32_t)k};

            resultp = নূতন AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(সঁচা);
        }

        VL_DO_DANGLING(argp->delete_বৃক্ষ(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(resultp);
    }
    শূন্য visit(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ধ্রুৱ zerop = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    শূন্য handlePow(Ast_গ্ৰন্থি_Biop* nodep) {
        যদি (AstConst* ধ্রুৱ exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ basep = nodep->lhsp();
            ধ্রুৱ V3_সংখ্যা অঙ্ক_One{nodep, basep->width(), 1};
            Ast_গ্ৰন্থি_ৰাশিমালা* powerp = নূতন AstConst{fl, অঙ্ক_One};
            ধ্রুৱ তার্কিক baseSigned = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ধ্রুৱ int32_t exponent = baseSigned ? exponentp->toSInt() : exponentp->toUInt();
            যদি (exponent > 0) {
                বাবে (int32_t i = 0; i < exponent; i++) {
                    যদি (baseSigned) {
                        powerp = নূতন AstMulS{fl, powerp, basep->clone_বৃক্ষ_Pure(মিছা)};
                    } অন্যথা {
                        powerp = নূতন AstMul{fl, powerp, basep->clone_বৃক্ষ_Pure(মিছা)};
                    }
                    powerp->user1(সঁচা);
                }
            } অন্যথা যদি (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                যদি (baseSigned) {
                    powerp = নূতন AstDivS{fl, powerp, basep->clone_বৃক্ষ_Pure(মিছা)};
                    powerp->user1(সঁচা);
                    powerp = নূতন AstDivS{fl, powerp, basep->clone_বৃক্ষ_Pure(মিছা)};
                    powerp->user1(সঁচা);
                } অন্যথা {
                    powerp = নূতন AstDiv{fl, powerp, basep->clone_বৃক্ষ_Pure(মিছা)};
                    powerp->user1(সঁচা);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            iterate(powerp);
        } অন্যথা যদি (ধ্রুৱ AstConst* ধ্রুৱ basep = VN_CAST(nodep->lhsp(), Const)) {
            যদি (basep->অঙ্ক().toUInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_সাৰি* ধ্রুৱ fl = nodep->fileline();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
                ধ্রুৱ পূর্ণাঙ্ক width = nodep->width();
                ধ্রুৱ তার্কিক rhsDependent = rhsp->user1();
                AstConst* ধ্রুৱ onep = নূতন AstConst{fl, AstConst::Widthed_মান{}, width, 1};
                AstShiftL* ধ্রুৱ shiftp = নূতন AstShiftL{fl, onep, rhsp, width};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_গ্ৰন্থি_ৰাশিমালা* resultp = shiftp;
                যদি (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ nClonep = rhsp->clone_বৃক্ষ_Pure(মিছা);
                    nClonep->user1(rhsDependent);
                    AstConst* ধ্রুৱ zeroNp
                        = নূতন AstConst{fl, AstConst::Widthed_মান{}, rhsp->width(), 0};
                    AstGteS* ধ্রুৱ condp = নূতন AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ধ্রুৱ zeroRp = নূতন AstConst{fl, AstConst::Widthed_মান{}, width, 0};
                    resultp = নূতন AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                iterate(resultp);
            } অন্যথা {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } অন্যথা {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    শূন্য visit(AstPow* nodep) override { handlePow(nodep); }
    শূন্য visit(AstPowSS* nodep) override { handlePow(nodep); }
    শূন্য visit(AstPowSU* nodep) override { handlePow(nodep); }
    শূন্য visit(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    শূন্য handleShift(Ast_গ্ৰন্থি_Biop* nodep) {
        যদি (editFormat(nodep)) ঘূৰাই;
        ধ্রুৱ পূর্ণাঙ্ক lhsWidth = nodep->lhsp()->width();
        ধ্রুৱ পূর্ণাঙ্ক rhsWidth = nodep->rhsp()->width();
        যদি (rhsWidth < lhsWidth) {
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = nodep->rhsp()->unlinkFrBack();
            AstExtend* ধ্রুৱ extendp = নূতন AstExtend{fl, rhsp, lhsWidth};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(সঁচা);
            nodep->rhsp(extendp);
        } অন্যথা যদি (rhsWidth > lhsWidth) {
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->dtypep();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ extendp
                = VN_IS(nodep, ShiftRS)
                      ? static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(নূতন AstExtendS{fl, lhsp, rhsWidth})
                      : static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(নূতন AstExtend{fl, lhsp, rhsWidth});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(সঁচা);
            nodep->lhsp(extendp);
            nodep->dtypeSetLogicSized(rhsWidth, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            AstSel* ধ্রুৱ selp = নূতন AstSel{fl, nodep, 0, lhsWidth};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            ঘূৰাই;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    শূন্য visit(AstShiftL* nodep) override { handleShift(nodep); }
    শূন্য visit(AstShiftR* nodep) override { handleShift(nodep); }
    শূন্য visit(AstShiftRS* nodep) override { handleShift(nodep); }
    শূন্য visit(Ast_গ্ৰন্থি_Biop* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Uniop* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        editSMT(nodep, nodep->lhsp());
    }
    শূন্য visit(Ast_গ্ৰন্থি_Triop* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    শূন্য visit(AstCond* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        যদি (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtypeSetString();
            ঘূৰাই;
        }
        // Fall back to "(ite cond then else)"
        visit(static_cast<Ast_গ্ৰন্থি_Triop*>(nodep));
    }
    শূন্য visit(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        যদি (editFormat(nodep)) ঘূৰাই;
        editSMT(nodep, nodep->srcp());
    }
    শূন্য visit(AstSel* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        VNRelinker handle;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ msbp = নূতন AstSFormatF{
            fl, "%1d", মিছা,
            নূতন AstAdd{fl, nodep->lsbp()->clone_বৃক্ষ_Pure(মিছা),
                       নূতন AstConst{fl, static_cast<uint32_t>(nodep->widthConst() - 1)}}};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lsbp
            = নূতন AstSFormatF{fl, "%1d", মিছা, nodep->lsbp()->unlinkFrBack(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    শূন্য visit(AstStructSel* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        ধ্রুৱ তার্কিক outermost = !m_deferRandModeHoist;
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ origp = outermost ? nodep->clone_বৃক্ষ(মিছা) : nullptr;
        VL_RESTORER(m_structSel);
        m_structSel = সঁচা;
        যদি (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = nodep->fromp();
            AstStructDType* ধ্রুৱ structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            যদি (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } অন্যথা {
                structp->markConstrainedRand(সঁচা);
            }
        }
        // Mark Random for structArray
        যদি (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ধ্রুৱ structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            যদি (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } অন্যথা {
                structp->markConstrainedRand(সঁচা);
            }
        }
        {
            VL_RESTORER(m_deferRandModeHoist);
            m_deferRandModeHoist = সঁচা;
            iterateChildren(nodep);
        }
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        AstSFormatF* newp = nullptr;
        যদি (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = নূতন AstSFormatF{fl, "%s.%s." + nodep->name(), মিছা,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_বৃক্ষ_Pure(সঁচা)};
            যদি (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } অন্যথা {
            newp = নূতন AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), মিছা,
                                   nullptr};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        যদি (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(ঠেলো_Deletep(origp), origp);
        }
    }
    শূন্য addStringNamePart(Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ indexp, ধ্রুৱ std::string& fmt = "%32x") {
        যদি (m_nestedAccess) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ bitp = indexp->clone_বৃক্ষ_Pure(মিছা);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ bitFormatp = নূতন AstSFormatF{bitp->fileline(), fmt, মিছা, bitp};
            AstSFormatF* ধ্রুৱ herep
                = নূতন AstSFormatF{indexp->fileline(), "%s.%s", মিছা, bitFormatp};
            m_nestedAccess->addVarNamePart(herep, indexp->name());
        }
    }
    শূন্য visit(AstAssocSel* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ origp = nodep->clone_বৃক্ষ(মিছা);
        AstSFormatF* newp = nullptr;
        যদি (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ idxp = নূতন AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), মিছা,
                                                      nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } অন্যথা যদি (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ধ্রুৱ stringp = VN_AS(nodep->bitp(), CvtPackString);
            ধ্রুৱ size_t stringSize = VN_AS(stringp->lhsp(), Const)->width();
            যদি (stringSize > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << stringSize << "bits, limit is 128 bits");
            }
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ idxp = নূতন AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), মিছা,
                                                      stringp->lhsp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } অন্যথা {
            যদি (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ধ্রুৱ পূর্ণাঙ্ক actual_width = nodep->bitp()->width();
                std::string fmt;
                // Normalize to standard bit width
                যদি (actual_width <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } অন্যথা যদি (actual_width <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } অন্যথা {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::to_string(VL_শব্দগুলি_I(actual_width) * 8) + "x";
                }
                যদি (m_nestedAccess) addStringNamePart(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ idxp
                    = নূতন AstSFormatF{fl, fmt, মিছা, nodep->bitp()->unlinkFrBack(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } অন্যথা {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                যদি (m_nestedAccess) m_nestedAccess->error();
            }
        }
        যদি (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        যদি (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(origp->delete_বৃক্ষ(), origp);
        }
    }
    শূন্য rename_Array_ৰাশিমালা(AstSFormatF* ধ্রুৱ newp) ধ্রুৱ {
        যদি (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    শূন্য visit(AstArraySel* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        তার্কিক indexIsRand = মিছা;
        nodep->bitp()->foreach([&](ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* vrefp) {
            যদি (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                indexIsRand = সঁচা;
            }
        });
        যদি (m_nestedAccess) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ bitp = nodep->bitp()->clone_বৃক্ষ_Pure(মিছা);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ bitFormatp
                = indexIsRand ? নূতন AstSFormatF{bitp->fileline(), bitp->name(), মিছা, nullptr}
                              : নূতন AstSFormatF{bitp->fileline(), "%x", মিছা, bitp};
            AstSFormatF* ধ্রুৱ herep
                = নূতন AstSFormatF{nodep->fileline(), "%s.%s", মিছা, bitFormatp};
            যদি (AstSel* ধ্রুৱ selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->addVarNamePart(herep, selp->fromp()->name());
            } অন্যথা {
                m_nestedAccess->addVarNamePart(herep, bitp->name());
            }
            যদি (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_বৃক্ষ(), bitp);
                ঘূৰাই;
            }
        }
        যদি (indexIsRand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_গ্ৰন্থি_ৰাশিমালা* indexp = nodep->bitp()->unlinkFrBack(&handle);
            যদি (indexp->width() < 32) {
                AstExtend* ধ্রুৱ extendp = নূতন AstExtend{fl, indexp, 32};
                extendp->user1(সঁচা);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ধ্রুৱ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ৰাশিমালা(newp);
        } অন্যথা {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ origp = nodep->clone_বৃক্ষ(মিছা);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ indexp
                = নূতন AstSFormatF{fl, "#x%8x", মিছা, nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(indexp);
            AstSFormatF* ধ্রুৱ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ৰাশিমালা(newp);
            যদি (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
                VL_DO_DANGLING(origp->delete_বৃক্ষ(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    স্থির তার্কিক isRandModeGate(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        ধ্রুৱ AstCMethodHard* ধ্রুৱ atp = VN_CAST(exprp, CMethodHard);
        যদি (!atp || atp->method() != VCMethod::ARRAY_AT) ঘূৰাই মিছা;
        ধ্রুৱ AstVar* varp = nullptr;
        যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } অন্যথা যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        যদি (!varp) ঘূৰাই মিছা;
        ধ্রুৱ AstClass* ধ্রুৱ classp = VN_CAST(varp->user2p(), Class);
        ঘূৰাই classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    তার্কিক hoistRandModeOverSelectAndMember(AstSFormatF* newp, Ast_গ্ৰন্থি_ৰাশিমালা* origp) {
        যদি (m_deferRandModeHoist) ঘূৰাই মিছা;
        // Only for selects yielding a non-array element (full chains)
        যদি (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) ঘূৰাই মিছা;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::vector<AstSFormatF*> frames;
        AstCond* modeCondp = nullptr;
        বাবে (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ firstp = curp->exprsp();
            frames.ঠেলো_back(curp);
            যদি (AstCond* ধ্রুৱ condp = VN_CAST(firstp, Cond)) {
                যদি (isRandModeGate(condp->condp())) modeCondp = condp;
                ভঙ্গ;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        যদি (!modeCondp) ঘূৰাই মিছা;
        File_সাৰি* ধ্রুৱ fl = newp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ modep = modeCondp->condp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* activep = modeCondp->thenp()->unlinkFrBack();
        // Rebuild the select chain text around the SMT name, innermost first
        বাবে (auto it = frames.rbegin(); it != frames.rend(); ++it) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ remainingArgsp = VN_CAST((*it)->exprsp()->nextp(), গ্ৰন্থি_ৰাশিমালা);
            যদি (remainingArgsp) { remainingArgsp->unlinkFrBackWithNext(); }
            activep = নূতন AstSFormatF{fl, (*it)->name(), মিছা,
                                      Ast_গ্ৰন্থি::addNext(activep, remainingArgsp)};
        }
        AstCond* ধ্রুৱ hoistp = নূতন AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(সঁচা);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(ঠেলো_Deletep(newp), newp);
        ঘূৰাই সঁচা;
    }
    শূন্য visit(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        যদি (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            যদি (m_nestedAccess) {
                AstSFormatF* ধ্রুৱ formatp = নূতন AstSFormatF{fl, nodep->name(), মিছা, nullptr};
                m_nestedAccess->addVarNamePart(নূতন AstSFormatF{fl, "%s.%s", মিছা, formatp},
                                               nodep->name());
            }
            Ast_গ্ৰন্থি* মূল_গ্ৰন্থি = nodep->fromp();
            যেতিয়ালৈ (ধ্রুৱ AstMemberSel* ধ্রুৱ selp = VN_CAST(মূল_গ্ৰন্থি, MemberSel)) {
                মূল_গ্ৰন্থি = selp->fromp();
            }

            Ast_গ্ৰন্থি_Sel* arraySelp = VN_CAST(মূল_গ্ৰন্থি, ArraySel);
            যদি (!arraySelp) arraySelp = VN_CAST(মূল_গ্ৰন্থি, AssocSel);

            যদি (arraySelp) {
                Ast_গ্ৰন্থি_DType* ধ্রুৱ arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : nullptr;
                যদি (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ধ্রুৱ তার্কিক isSimple
                        = nodep->fromp() == মূল_গ্ৰন্থি && VN_IS(arraySelp->fromp(), VarRef);
                    যদি (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = নূতন NestedAccessPath(nodep->clone_বৃক্ষ(মিছা),
                                                              &m_firstExpressionInsideIndexp);
                        AstSFormatF* ধ্রুৱ formatp
                            = নূতন AstSFormatF{fl, nodep->name(), মিছা, nullptr};
                        m_nestedAccess->addVarNamePart(
                            নূতন AstSFormatF{fl, "%s.%s", মিছা, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = সঁচা;
                    nodep->user1(সঁচা);
                    arraySelp->user1(সঁচা);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ origp = nodep->clone_বৃক্ষ(মিছা);
                    iterateChildren(nodep);
                    AstSFormatF* ধ্রুৱ newp = নূতন AstSFormatF{fl, "%s." + nodep->name(), মিছা,
                                                              nodep->fromp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    যদি (!hoistRandModeOverSelectAndMember(newp, origp))
                        VL_DO_DANGLING(origp->delete_বৃক্ষ(), origp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess);
                    ঘূৰাই;
                }
            }
            // Check if the root variable participates in global constraints
            যদি (ধ্রুৱ AstVarRef* ধ্রুৱ varRefp = VN_CAST(মূল_গ্ৰন্থি, VarRef)) {
                AstVar* ধ্রুৱ constrainedVar = varRefp->varp();
                যদি (constrainedVar->globalConstrained()) {
                    তার্কিক ধ্রুৱ beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    যদি (beforeIterate) {
                        nodep->replaceWith(নূতন AstSFormatF{fl, "%s." + nodep->name(), মিছা,
                                                           nodep->fromp()->unlinkFrBack()});
                    } অন্যথা {
                        nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                    }
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                    ঘূৰাই;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        যদি (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), গ্ৰন্থি_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ধ্রুৱ varRefp
                = নূতন AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), গ্ৰন্থি_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            visit(varRefp);
        } অন্যথা যদি (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->method() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ধ্রুৱ cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_গ্ৰন্থি_DType* ধ্রুৱ arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDtp = arrayDtp->subDTypep()->skipRefp();
            যদি (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = সঁচা;
                iterateChildren(nodep);
                File_সাৰি* ধ্রুৱ fl = nodep->fileline();
                AstSFormatF* newp = nullptr;
                যদি (AstSFormatF* ধ্রুৱ fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    যদি (fromp->name() == "%s.%s") {
                        newp = নূতন AstSFormatF{fl, "%s.%s." + nodep->name(), মিছা,
                                               fromp->exprsp()->clone_বৃক্ষ_Pure(সঁচা)};
                    } অন্যথা {
                        newp = নূতন AstSFormatF{fl, fromp->name() + "." + nodep->name(), মিছা,
                                               nullptr};
                    }
                } অন্যথা {
                    newp = নূতন AstSFormatF{fl, nodep->name(), মিছা, nullptr};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ঘূৰাই;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        } অন্যথা যদি (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        } অন্যথা {
            editFormat(nodep);
        }
    }
    শূন্য visit(AstSFormatF* nodep) override {}
    শূন্য visit(Ast_Stmt_ৰাশিমালা* nodep) override {}
    শূন্য visit(AstCStmt* nodep) override {}
    শূন্য visit(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    শূন্য visit(AstConstraintIf* nodep) override {
        Ast_গ্ৰন্থি_ৰাশিমালা* newp = nullptr;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ thenp = editSingle(fl, nodep->thensp());
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elsep = editSingle(fl, nodep->elsesp());
        যদি (thenp && elsep) {
            newp = নূতন AstCond{fl, nodep->condp()->unlinkFrBack(), thenp, elsep};
        } অন্যথা যদি (thenp) {
            newp = নূতন AstLogIf{fl, nodep->condp()->unlinkFrBack(), thenp};
        } অন্যথা যদি (elsep) {
            newp = নূতন AstLogIf{fl, নূতন AstNot{fl, nodep->condp()->unlinkFrBack()}, elsep};
        }
        যদি (newp) {
            newp->dtypeSetBit();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(সঁচা);  // Assume result-dependent
            nodep->replaceWith(নূতন Ast_Constraint_ৰাশিমালা{fl, newp});
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstGenBlock* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    শূন্য visit(AstBegin* nodep) override {}
    শূন্য visit(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();

        যদি (!nodep->bodyp()) {
            nodep->unlinkFrBack();
        } অন্যথা যদি (m_wantSingle) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(itemp);
            cstmtp->add(";");
            Ast_গ্ৰন্থি* ধ্রুৱ bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ধ্রুৱ cexprp = নূতন AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(নূতন AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     নূতন AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp},
                                     সঁচা});
            cexprp->add("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(নূতন AstSFormatF{fl, "%s", মিছা, cexprp});
        } অন্যথা {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            বাবে (Ast_গ্ৰন্থি* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(elemp, Var)) m_foreachIdxVars.ঢোকাও(varp);
            }
            iterateAndNextNull(nodep->bodyp());
            Ast_গ্ৰন্থি* ধ্রুৱ bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlinkFrBackWithNext());
            nodep->replaceWith(
                নূতন AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                             নূতন AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp}, সঁচা});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_Module* ধ্রুৱ genModp = VN_AS(m_genp->user2p(), গ্ৰন্থি_Module);

        বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), গ্ৰন্থি_ৰাশিমালা)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsTestp = build_Solve_Before_Name_ৰাশিমালা(fl, lhsp);
            যদি (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                অনুৱর্ত;
            }
            VL_DO_DANGLING(lhsTestp->delete_বৃক্ষ(), lhsTestp);
            বাবে (Ast_গ্ৰন্থি_ৰাশিমালা* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), গ্ৰন্থি_ৰাশিমালা)) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsNamep = build_Solve_Before_Name_ৰাশিমালা(fl, rhsp);
                যদি (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    অনুৱর্ত;
                }
                AstCMethodHard* ধ্রুৱ callp = নূতন AstCMethodHard{
                    fl, নূতন AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtypeSetVoid();
                callp->addPinsp(build_Solve_Before_Name_ৰাশিমালা(fl, lhsp));
                callp->addPinsp(rhsNamep);
                nodep->addHereThisAsNext(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstConstraintUnique* nodep) override {
        Ast_গ্ৰন্থি_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        Ast_গ্ৰন্থি_Module* genModp = nullptr;

        যদি (m_classp) {
            initTaskp = VN_AS(m_memberMap.findMember(m_classp, "new"), গ্ৰন্থি_FTask);
            genVarp = getRandomGenerator(m_classp);
        } অন্যথা {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        যদি (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        যদি (m_classp) {
            genModp = VN_AS(genVarp->user2p(), গ্ৰন্থি_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_গ্ৰন্থি* setupStmtsp = nullptr;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::vector<std::string> smtExprs;
        বাবে (Ast_গ্ৰন্থি* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            যদি (AstVarRef* ধ্রুৱ varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ধ্রুৱ varp = varRefp->varp();
                Ast_গ্ৰন্থি_Module* ধ্রুৱ varModp = VN_AS(varp->user2p(), গ্ৰন্থি_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = varp->dtypep()->skipRefp();

                ধ্রুৱ স্থির auto dynDTypeSupported
                    = [](ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep) -> তার্কিক {
                    ঘূৰাই VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elemWidth;
                uint32_t staticSize = 0;  // Element count, zero when only known at run time
                যদি (ধ্রুৱ AstUnpackArrayDType* ধ্রুৱ up = VN_CAST(dtypep, UnpackArrayDType)) {
                    ধ্রুৱ AstRange* ধ্রুৱ rangep = up->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    staticSize = up->elementsConst();
                    যদি (staticSize > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        অনুৱর্ত;
                    }
                    elemWidth = static_cast<uint64_t>(varp->dtypep()->width());
                } অন্যথা যদি (dynDTypeSupported(dtypep)) {
                    elemWidth = static_cast<uint64_t>(dtypep->subDTypep()->width());
                } অন্যথা {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(মিছা));
                    অনুৱর্ত;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ subp = dtypep->subDTypep()->skipRefp();
                যদি (VN_IS(subp, গ্ৰন্থি_Array_DType) || VN_IS(subp, QueueDType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    অনুৱর্ত;
                }

                যদি (m_wantSingle && !staticSize) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    অনুৱর্ত;
                }

                যদি (!varp->user3()) {
                    varp->user3(সঁচা);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ varnamep = নূতন AstCExpr{
                        fl, AstCExpr::Pure{}, "\"" + varp->name() + "\"", varp->width()};

                    AstCMethodHard* ধ্রুৱ writeVarCallp = নূতন AstCMethodHard{
                        fl, নূতন AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ধ্রুৱ argVarRefp = নূতন AstVarRef{fl, varp, VAccess::READ};
                    যদি (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->addPinsp(argVarRefp);
                    writeVarCallp->addPinsp(নূতন AstConst{fl, AstConst::Unsized64{}, elemWidth});
                    writeVarCallp->addPinsp(varnamep);
                    writeVarCallp->addPinsp(নূতন AstConst{fl, 1});  // Dimension
                    ধ্রুৱ RandomizeMode randMode = {.asUQuad = varp->user1()};
                    যদি (randMode.usesMode) {
                        writeVarCallp->addPinsp(
                            নূতন AstConst{fl, AstConst::Unsized64{}, randMode.index});
                    }
                    writeVarCallp->dtypeSetVoid();
                    initTaskp->addStmtsp(নূতন Ast_Stmt_ৰাশিমালা{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                যদি (varp->user4p() && m_sizeConstrainedArraysp) {
                    m_sizeConstrainedArraysp->ঢোকাও(varp);
                }

                যদি (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    যদি (staticSize < 2) অনুৱর্ত;
                    std::string ৰাশিমালা_Str = "(__Vbv (distinct";
                    বাবে (uint32_t i = 0; i < staticSize; ++i) {
                        ৰাশিমালা_Str += " (select " + varp->name() + " #x";
                        বাবে (পূর্ণাঙ্ক shift = 28; shift >= 0; shift -= 4) {
                            ৰাশিমালা_Str += "0123456789abcdef"[(i >> shift) & 0xf];
                        }
                        ৰাশিমালা_Str += ')';
                    }
                    smtExprs.emplace_back(ৰাশিমালা_Str + "))");
                    অনুৱর্ত;
                }

                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ namep = নূতন AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ধ্রুৱ randUniqueCallp
                    = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->findVoidDType());
                randUniqueCallp->addPinsp(নূতন AstConst{fl, *m_uniqueConstraintId});
                setupStmtsp = Ast_গ্ৰন্থি::addNext(setupStmtsp, নূতন Ast_Stmt_ৰাশিমালা{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        যদি (m_wantSingle && !smtExprs.empty()) {
            std::string ৰাশিমালা_Str = smtExprs.front();
            যদি (smtExprs.size() > 1) {
                ৰাশিমালা_Str = "(bvand";
                বাবে (ধ্রুৱ std::string& oneExprp : smtExprs) ৰাশিমালা_Str += " " + oneExprp;
                ৰাশিমালা_Str += ')';
            }
            AstSFormatF* ধ্রুৱ newp = নূতন AstSFormatF{fl, ৰাশিমালা_Str, মিছা, nullptr};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            ঘূৰাই;
        }
        যদি (setupStmtsp) nodep->addHereThisAsNext(setupStmtsp);
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    শূন্য visit(Ast_Constraint_ৰাশিমালা* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_firstExpressionInsideIndexp);
        যদি (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = nodep->exprp();
            যদি (exprp->width() > 1) {
                File_সাৰি* ধ্রুৱ fl = exprp->fileline();
                ধ্রুৱ V3_সংখ্যা অঙ্ক_Zero{fl, exprp->width(), 0};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ neqp
                    = নূতন AstNeq{fl, exprp->unlinkFrBack(), নূতন AstConst{fl, অঙ্ক_Zero}};
                neqp->user1(সঁচা);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        যদি (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ধ্রুৱ VCMethod method
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ধ্রুৱ callp = নূতন AstCMethodHard{
            nodep->fileline(),
            নূতন AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), গ্ৰন্থি_Module), m_genp,
                          VAccess::READWRITE},
            method, nodep->exprp()->unlinkFrBack()};
        callp->dtypeSetVoid();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->addPinsp(নূতন AstCExpr{nodep->fileline(), AstCExpr::Pure{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ধ্রুৱ uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->addPinsp(নূতন AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string prettyText;
        যদি (!v3Global.opt.protectIds()) {
            prettyText = nodep->fileline()->prettySource();
            size_t pos = 0;
            যেতিয়ালৈ ((pos = prettyText.find('"', pos)) != std::string::npos) {
                prettyText.ঢোকাও(pos, "\\");
                pos += std::strlen("\\\"");
            }
        }
        callp->addPinsp(
            নূতন AstCExpr{nodep->fileline(), AstCExpr::Pure{}, "\"" + prettyText + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();

        যদি (nodep->method() == VCMethod::ARRAY_AT) {
            addStringNamePart(nodep->pinsp(), "%x");
            ধ্রুৱ তার্কিক indexIsRand = nodep->pinsp()->exists([](ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ vrefp) {
                ঘূৰাই vrefp->varp()->rand().isRandomizable();
            });
            যদি (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ basep = nodep->baseFromp(সঁচা);
                    যদি (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        ঘূৰাই;
                    }
                }
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ origp = indexIsRand ? nullptr : nodep->clone_বৃক্ষ(মিছা);
                ধ্রুৱ AstVarRef* ধ্রুৱ baseRefp
                    = m_structSel ? VN_CAST(nodep->fromp()->baseFromp(সঁচা), VarRef) : nullptr;
                যদি (baseRefp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(baseRefp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ indexp = nodep->pinsp()->clone_বৃক্ষ_Pure(মিছা);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ arrayp = nodep->fromp()->clone_বৃক্ষ_Pure(মিছা);
                    AstCMethodHard* ধ্রুৱ currentSizep
                        = নূতন AstCMethodHard{fl, arrayp->clone_বৃক্ষ_Pure(মিছা), VCMethod::DYN_SIZE};
                    currentSizep->dtypeSetInt();
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ inRangeRequiredp = নূতন AstLogAnd{
                        fl,
                        নূতন AstLteS{fl,
                                    নূতন AstConst{fl, AstConst::Widthed_মান{}, indexp->width(), 0},
                                    indexp->clone_বৃক্ষ_Pure(মিছা)},
                        নূতন AstGteS{fl, indexp->clone_বৃক্ষ_Pure(মিছা), currentSizep}};
                    AstCMethodHard* ধ্রুৱ resizep
                        = নূতন AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             নূতন AstAdd{fl, indexp, নূতন AstConst{fl, 1}}};
                    resizep->dtypeSetVoid();
                    m_prepareConstrainedArraysp->addStmtsp(
                        নূতন AstIf{fl, inRangeRequiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ধ্রুৱ sizep
                    = m_structSel ? নূতন AstCMethodHard{fl, nodep->fromp()->clone_বৃক্ষ_Pure(মিছা),
                                                       VCMethod::DYN_SIZE}
                                  : nullptr;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ pinp = nodep->pinsp()->unlinkFrBack();
                AstSFormatF* newp;
                যদি (m_structSel) {
                    যদি (AstSFormatF* ধ্রুৱ sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argsp
                        = Ast_গ্ৰন্থি::addNext(nodep->fromp()->unlinkFrBack(), pinp);
                    sizep->dtypeSetInt();
                    AstLogAnd* ধ্রুৱ condp
                        = নূতন AstLogAnd{fl,
                                        নূতন AstLteS{fl,
                                                    নূতন AstConst{fl, AstConst::Widthed_মান{},
                                                                 originalPinp->width(), 0},
                                                    originalPinp->clone_বৃক্ষ_Pure(মিছা)},
                                        নূতন AstLtS{fl, originalPinp->clone_বৃক্ষ_Pure(মিছা), sizep}};
                    m_conditionp = m_conditionp ? নূতন AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = নূতন AstSFormatF{fl, "%s.%s", মিছা, argsp};
                } অন্যথা {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                যদি (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_বৃক্ষ(), origp);
                }

            } অন্যথা যদি (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } অন্যথা {
                nodep->user1(মিছা);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            ঘূৰাই;
        }

        যদি (nodep->method() == VCMethod::ARRAY_INSIDE) {
            ধ্রুৱ তার্কিক randArr = nodep->fromp()->user1();

            AstVar* ধ্রুৱ newVarp
                = নূতন AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->findIntDType()};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ idxRefp = নূতন AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            AstForeachHeader* ধ্রুৱ headerp
                = নূতন AstForeachHeader{fl, nodep->fromp()->clone_বৃক্ষ_Pure(মিছা), newVarp};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_গ্ৰন্থি* ধ্রুৱ itemp = নূতন AstEq{fl, selp, nodep->pinsp()->unlinkFrBack()};
            itemp->user1(সঁচা);

            AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(iterateSubtreeReturnEdits(itemp));
            cstmtp->add(";");
            AstCExpr* ধ্রুৱ cexprp = নূতন AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(নূতন AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     নূতন AstForeach{fl, headerp, cstmtp}, সঁচা});
            cexprp->add("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(নূতন AstSFormatF{fl, "%s", মিছা, cexprp});
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }

        // Array reduction methods (sum, product, and, or, xor)
        যদি (nodep->method() == VCMethod::ARRAY_R_SUM
            || nodep->method() == VCMethod::ARRAY_R_PRODUCT
            || nodep->method() == VCMethod::ARRAY_R_AND || nodep->method() == VCMethod::ARRAY_R_OR
            || nodep->method() == VCMethod::ARRAY_R_XOR) {

            AstWith* ধ্রুৱ withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            যদি (withp) {
                যদি (AstUnpackArrayDType* ধ্রুৱ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ধ্রুৱ পূর্ণাঙ্ক arraySize = adtypep->elementsConst();
                    যদি (arraySize > v3Global.opt.constraintArrayLimit()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvtToStr(arraySize)
                                          + " exceeds --constraint-array-limit of "
                                          + cvtToStr(v3Global.opt.constraintArrayLimit())
                                          + "), treating as state");
                        nodep->user1(মিছা);
                        যদি (editFormat(nodep)) ঘূৰাই;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        ঘূৰাই;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ধ্রুৱ তার্কিক hasRandomArray = nodep->fromp()->user1();
            যদি (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(মিছা);
                যদি (editFormat(nodep)) ঘূৰাই;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                ঘূৰাই;
            }
            যদি (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                ঘূৰাই;
            }

            // Create loop variable and header
            AstVar* ধ্রুৱ loopVarp
                = নূতন AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->findIntDType()};
            AstForeachHeader* ধ্রুৱ headerp
                = নূতন AstForeachHeader{fl, nodep->fromp()->clone_বৃক্ষ_Pure(মিছা), loopVarp};

            // Determine SMT operation and compute identity elements
            ধ্রুৱ ৱর্ণ* smtOp = nullptr;
            std::string identity;
            যদি (withp) {
                // For 'with' clause: use binary format, compute from result width
                ধ্রুৱ পূর্ণাঙ্ক width = nodep->dtypep()->width();
                যদি (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = "#b" + std::string(width, '0');
                } অন্যথা যদি (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = (width > 0) ? "#b" + std::string(width - 1, '0') + "1" : "#b0";
                } অন্যথা যদি (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = "#b" + std::string(width, '1');
                } অন্যথা যদি (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = "#b" + std::string(width, '0');
                } অন্যথা {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = "#b" + std::string(width, '0');
                }
            } অন্যথা {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ধ্রুৱ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ধ্রুৱ arrVarp = arrRefp->varp();
                Ast_গ্ৰন্থি_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ধ্রুৱ পূর্ণাঙ্ক elemWidth = elemDtp->width();
                ধ্রুৱ পূর্ণাঙ্ক hexDigits = (elemWidth + 3) / 4;

                ধ্রুৱ std::string zeroIdentity = "#x" + std::string(hexDigits, '0');
                ধ্রুৱ std::string oneIdentity = "#x" + std::string(hexDigits - 1, '0') + "1";
                ধ্রুৱ std::string allOnesIdentity = "#x" + std::string(hexDigits, 'f');

                যদি (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = zeroIdentity;
                } অন্যথা যদি (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = oneIdentity;
                } অন্যথা যদি (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = allOnesIdentity;
                } অন্যথা যদি (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = zeroIdentity;
                } অন্যথা {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = zeroIdentity;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ধ্রুৱ cstmtp = নূতন AstCStmt{fl};
            যদি (withp) {
                // With 'with' clause: evaluate expression for each element
                ধ্রুৱ তার্কিক randArr = nodep->fromp()->user1();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ idxRefp = নূতন AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ধ্রুৱ পূর্ণাঙ্ক resultWidth = nodep->dtypep()->width();
                ধ্রুৱ VSigning resultSigning = nodep->dtypep()->numeric();

                Ast_গ্ৰন্থি* perElemExprp = withp->exprp()->clone_বৃক্ষ_Pure(মিছা);
                যদি (AstLambdaArgRef* ধ্রুৱ মূল_Refp = VN_CAST(perElemExprp, LambdaArgRef)) {
                    যদি (মূল_Refp->index()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ধ্রুৱ loopRef = নূতন AstVarRef{fl, loopVarp, VAccess::READ};
                        perElemExprp = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                    } অন্যথা {
                        // item at root -> replace with array element
                        perElemExprp = elemSelp->clone_বৃক্ষ_Pure(মিছা);
                    }
                    VL_DO_DANGLING(মূল_Refp->delete_বৃক্ষ(), মূল_Refp);
                } অন্যথা {
                    perElemExprp->foreach([&](AstLambdaArgRef* refp) {
                        যদি (refp->index()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ধ্রুৱ loopRef = নূতন AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_গ্ৰন্থি* ধ্রুৱ replacement
                                = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                            refp->replaceWith(replacement);
                        } অন্যথা {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_বৃক্ষ_Pure(মিছা));
                        }
                        VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_বৃক্ষ(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                perElemExprp = V3Const::constifyEdit(perElemExprp);

                perElemExprp->foreach([&](Ast_গ্ৰন্থি* nodep) {
                    // Don't mark loop variable references as randomizable
                    যদি (!VN_IS(nodep, Const)) {
                        যদি (Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ vrefp = VN_CAST(nodep, গ্ৰন্থি_Var_Ref)) {
                            যদি (vrefp->varp() != loopVarp) nodep->user1(সঁচা);
                        } অন্যথা {
                            nodep->user1(সঁচা);
                        }
                    }
                });

                cstmtp->add("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(iterateSubtreeReturnEdits(perElemExprp));
                cstmtp->add(";\n");
                cstmtp->add("ret += \")\";\n");
            } অন্যথা {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ধ্রুৱ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ধ্রুৱ arrVarp = arrRefp->varp();
                ধ্রুৱ std::string smtArrayName = arrVarp->name();
                Ast_গ্ৰন্থি_DType* ধ্রুৱ elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ধ্রুৱ পূর্ণাঙ্ক elemWidth = elemDtp->width();
                Ast_গ্ৰন্থি_Module* ধ্রুৱ arrModulep = VN_AS(arrVarp->user2p(), গ্ৰন্থি_Module);
                Ast_গ্ৰন্থি_Module* ধ্রুৱ genModulep = VN_AS(m_genp->user2p(), গ্ৰন্থি_Module);
                arrVarp->user3(সঁচা);

                // Add write_var call to init task
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ arrVarNamep
                    = নূতন AstCExpr{fl, AstCExpr::Pure{}, "\"" + smtArrayName + "\"", elemWidth};
                AstCMethodHard* ধ্রুৱ writeVarCallp
                    = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->addPinsp(নূতন AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->addPinsp(
                    নূতন AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elemWidth)});
                writeVarCallp->addPinsp(arrVarNamep);
                ধ্রুৱ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(মিছা).second;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->addPinsp(নূতন AstConst{fl, 1});  // Dimension

                ধ্রুৱ RandomizeMode randMode = {.asUQuad = arrVarp->user1()};
                যদি (randMode.usesMode) {
                    writeVarCallp->addPinsp(
                        নূতন AstConst{fl, AstConst::Unsized64{}, randMode.index});
                }
                writeVarCallp->dtypeSetVoid();
                Ast_গ্ৰন্থি_FTask* initTaskp = m_inlineInitTaskp;
                যদি (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.findMember(arrModulep, "new"), গ্ৰন্থি_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->addStmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ধ্রুৱ idxFormatp = নূতন AstSFormatF{
                    fl, "#x%08x", মিছা, নূতন AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ধ্রুৱ varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), idxFormatp);

                cstmtp->add("ret += \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(varNamep);
                cstmtp->add(";\n");
            }

            // Build final expression
            যদি (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ধ্রুৱ cexprp = নূতন AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret = \"" + identity + "\";\n");
                cexprp->add(নূতন AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         নূতন AstForeach{fl, headerp, cstmtp}, সঁচা});
                cexprp->add("return ret;\n})()");
                nodep->replaceWith(নূতন AstSFormatF{fl, "%s", মিছা, cexprp});
            } অন্যথা {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ধ্রুৱ cexprp = নূতন AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret;\n");
                cexprp->add(নূতন AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         নূতন AstForeach{fl, headerp, cstmtp}, সঁচা});
                cexprp->add("return ret.empty() ? \"" + identity + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(নূতন AstSFormatF{fl, "%s", মিছা, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }

        // We've handled DYN_SIZE already if they should be randomized
        যদি (nodep->method() == VCMethod::DYN_SIZE || nodep->method() == VCMethod::ASSOC_SIZE) {
            Ast_গ্ৰন্থি_ৰাশিমালা* গ্ৰন্থি_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            যেতিয়ালৈ (VN_IS(গ্ৰন্থি_Exprp, MemberSel)) {
                গ্ৰন্থি_Exprp = VN_AS(গ্ৰন্থি_Exprp, MemberSel)->fromp();
            }
            যদি (VN_IS(গ্ৰন্থি_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_বৃক্ষ_Pure(মিছা)));
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                ঘূৰাই;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(মিছা);

        যদি (editFormat(nodep)) ঘূৰাই;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask_Ref* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;

        AstFunc* ধ্রুৱ funcp = VN_CAST(nodep->taskp(), Func);
        যদি (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            ঘূৰাই;
        }

        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ retExprp = extract_Return_ৰাশিমালা(funcp);
        যদি (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(মিছা);
            যদি (editFormat(nodep)) ঘূৰাই;
            ঘূৰাই;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ধ্রুৱ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), nullptr, মিছা);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_গ্ৰন্থি_ৰাশিমালা* inlinedp = retExprp->clone_বৃক্ষ_Pure(মিছা);
        বাবে (ধ্রুৱ auto& tconnect : tconnects) {
            ধ্রুৱ AstVar* ধ্রুৱ portp = tconnect.first;
            AstArg* ধ্রুৱ argp = tconnect.second;
            যদি (!argp || !argp->exprp()) অনুৱর্ত;
            যদি (AstVarRef* ধ্রুৱ মূল_Refp = VN_CAST(inlinedp, VarRef)) {
                যদি (মূল_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_বৃক্ষ_Pure(মিছা);
                    VL_DO_DANGLING(মূল_Refp->delete_বৃক্ষ(), মূল_Refp);
                    অনুৱর্ত;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                যদি (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_বৃক্ষ_Pure(মিছা));
                    VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    শূন্য visit(Ast_গ্ৰন্থি_Stmt* nodep) override {}
    শূন্য visit(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) override {
        যদি (editFormat(nodep)) ঘূৰাই;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_গ্ৰন্থি* buildDisableSoftCallStmt(Ast_Constraint_ৰাশিমালা* nodep) {
        std::string varName;
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ vrefp = VN_CAST(nodep->exprp(), গ্ৰন্থি_Var_Ref)) {
            varName = vrefp->name();
        } অন্যথা যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } অন্যথা {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ধ্রুৱ callp = নূতন AstCMethodHard{
            nodep->fileline(),
            নূতন AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), গ্ৰন্থি_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            নূতন AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtypeSetVoid();
        ঘূৰাই callp->makeStmt();
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
    Ast_গ্ৰন্থি* extractConditionalDisableSofts(Ast_গ্ৰন্থি* itemsp, Ast_গ্ৰন্থি_ৰাশিমালা* outerCondp) {
        Ast_গ্ৰন্থি* hoistListp = nullptr;
        বাবে (Ast_গ্ৰন্থি *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            যদি (AstConstraintIf* ধ্রুৱ ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_সাৰি* ধ্রুৱ fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ thenCondp
                    = outerCondp ? static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                                       নূতন AstLogAnd{fl, outerCondp->clone_বৃক্ষ_Pure(মিছা),
                                                     ifp->condp()->clone_বৃক্ষ_Pure(মিছা)})
                                 : ifp->condp()->clone_বৃক্ষ_Pure(মিছা);
                যদি (Ast_গ্ৰন্থি* ধ্রুৱ thenHoistp
                    = extractConditionalDisableSofts(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_গ্ৰন্থি::addNext(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_বৃক্ষ(), thenCondp);
                যদি (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ notInnerp
                        = নূতন AstLogNot{fl, ifp->condp()->clone_বৃক্ষ_Pure(মিছা)};
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elseCondp
                        = outerCondp ? static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(নূতন AstLogAnd{
                                           fl, outerCondp->clone_বৃক্ষ_Pure(মিছা), notInnerp})
                                     : notInnerp;
                    যদি (Ast_গ্ৰন্থি* ধ্রুৱ elseHoistp
                        = extractConditionalDisableSofts(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_গ্ৰন্থি::addNext(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_বৃক্ষ(), elseCondp);
                }
            } অন্যথা যদি (Ast_Constraint_ৰাশিমালা* ধ্রুৱ exprp = VN_CAST(stmtp, Constraint_ৰাশিমালা)) {
                যদি (exprp->isDisableSoft() && outerCondp) {
                    File_সাৰি* ধ্রুৱ fl = exprp->fileline();
                    AstIf* ধ্রুৱ hoistIfp = নূতন AstIf{fl, outerCondp->clone_বৃক্ষ_Pure(মিছা),
                                                      buildDisableSoftCallStmt(exprp), nullptr};
                    hoistListp = hoistListp ? Ast_গ্ৰন্থি::addNext(hoistListp,
                                                               static_cast<Ast_গ্ৰন্থি*>(hoistIfp))
                                            : static_cast<Ast_গ্ৰন্থি*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_ৰাশিমালা* ধ্রুৱ truep
                        = নূতন Ast_Constraint_ৰাশিমালা{fl, নূতন AstConst{fl, AstConst::BitTrue{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        ঘূৰাই hoistListp;
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit Constraint_ৰাশিমালা_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_গ্ৰন্থি* nodep,
                                   Ast_গ্ৰন্থি_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* randModeVarp, std::set<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3UniqueNames& uniqueNames,
                                   Ast_গ্ৰন্থি_FTask* memberselInitTaskp = nullptr,
                                   VInsertionSet<AstVar*>* sizeConstrainedArraysp = nullptr,
                                   Ast_গ্ৰন্থি_FTask* prepareConstrainedArraysp = nullptr)
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
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ hoistListp
            = nodep ? extractConditionalDisableSofts(nodep, nullptr) : nullptr) {
            nodep->addNext(hoistListp);
        }
        iterateAndNextNull(nodep);
    }
};

গণনা শ্ৰেণী CaptureMode : uint8_t {
    CAP_NO = 0x0,
    CAP_মান = 0x01,
    CAP_THIS = 0x02,
    CAP_F_SET_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
CaptureMode operator|(CaptureMode a, CaptureMode b) {
    ঘূৰাই static_cast<CaptureMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
CaptureMode operator&(CaptureMode a, CaptureMode b) {
    ঘূৰাই static_cast<CaptureMode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
CaptureMode mode(CaptureMode a) { ঘূৰাই a & static_cast<CaptureMode>(0x3); }
তার্কিক hasFlags(CaptureMode a, CaptureMode flags) {
    ঘূৰাই ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

শ্ৰেণী CaptureVisitor final : সার্ৱজনিক VNVisitor {
    AstArg* m_argsp = nullptr;  // Original references turned into arguments
    Ast_গ্ৰন্থি_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ধ্রুৱ AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::set<Ast_গ্ৰন্থি*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = nullptr;  // Variable for outer context's object, if necessary
    তার্কিক m_staticContext = মিছা;  // True when capturing from a static function context

    // METHODS

    তার্কিক captureVariable(File_সাৰি* ধ্রুৱ fileline, Ast_গ্ৰন্থি_Var_Ref* varrefp, AstVar*& varp) {
        auto it = m_varCloneMap.find(varrefp->varp());
        যদি (it == m_varCloneMap.end()) {
            AstVar* ধ্রুৱ newVarp = varrefp->varp()->clone_বৃক্ষ(মিছা);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(সঁচা);
            newVarp->direction(VDirection::প্ৰবেশ);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            ঘূৰাই সঁচা;
        }
        varp = it->second;
        ঘূৰাই মিছা;
    }

    template <typename T_গ্ৰন্থি>
    শূন্য fixupClassOrPackage(Ast_গ্ৰন্থি* memberp, T_গ্ৰন্থি refp) {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ declClassp = VN_AS(memberp->user2p(), গ্ৰন্থি_Module);
        যদি (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    template <typename T_গ্ৰন্থি>
    তার্কিক isReferenceToInnerMember(T_গ্ৰন্থি nodep) {
        ঘূৰাই VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_সাৰি* fl) {
        যদি (!m_thisp) {
            AstClassRefDType* ধ্রুৱ refDTypep
                = নূতন AstClassRefDType{fl, VN_AS(m_callerp, Class), nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
            m_thisp = নূতন AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(সঁচা);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::প্ৰবেশ);
            m_argsp = Ast_গ্ৰন্থি::addNext(m_argsp, নূতন AstArg{fl, "", নূতন AstThisRef{fl, refDTypep}});
        }
        ঘূৰাই m_thisp;
    }

    AstVar* getVar(AstVar* ধ্রুৱ varp) ধ্রুৱ {
        ধ্রুৱ auto it = m_varCloneMap.find(varp);
        যদি (it == m_varCloneMap.end()) ঘূৰাই nullptr;
        যদি (it->second->isStdRandomizeArg()) ঘূৰাই nullptr;
        ঘূৰাই it->second;
    }

    CaptureMode getVarRefCaptureMode(Ast_গ্ৰন্থি_Var_Ref* varRefp) {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ varModp = VN_AS(varRefp->varp()->user2p(), গ্ৰন্থি_Module);
        AstClass* ধ্রুৱ varClassp = VN_CAST(varModp, Class);
        AstClass* ধ্রুৱ callerClassp = VN_CAST(m_callerp, Class);

        ধ্রুৱ তার্কিক callerIsClass = callerClassp;
        ধ্রুৱ তার্কিক refIsXref = VN_IS(varRefp, VarXRef);
        ধ্রুৱ তার্কিক varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ধ্রুৱ তার্কিক varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ধ্রুৱ তার্কিক varIsFieldOfCaller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ধ্রুৱ তার্কিক varIsParam = varRefp->varp()->isParam();
        ধ্রুৱ তার্কিক varIsConstraintIterator
            = VN_IS(varRefp->varp()->firstAbovep(), ForeachHeader)
              && VN_IS(varRefp->varp()->firstAbovep()->firstAbovep(), ConstraintForeach);
        যদি (refIsXref) ঘূৰাই CaptureMode::CAP_মান | CaptureMode::CAP_F_XREF;
        যদি (varIsConstraintIterator) ঘূৰাই CaptureMode::CAP_NO;
        যদি (varIsFuncLocal && varHasAutomaticLifetime) ঘূৰাই CaptureMode::CAP_মান;
        যদি (varIsParam) ঘূৰাই CaptureMode::CAP_মান;
        // Static var in function (will not be inlined, because it's in class)
        যদি (callerIsClass && varIsFuncLocal) ঘূৰাই CaptureMode::CAP_মান;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        যদি (m_staticContext && callerIsClass && varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            ঘূৰাই CaptureMode::CAP_NO;
        যদি (callerIsClass && varIsFieldOfCaller) ঘূৰাই CaptureMode::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        যদি (callerIsClass && varClassp && !varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            ঘূৰাই CaptureMode::CAP_মান;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        ঘূৰাই CaptureMode::CAP_মান;
    }

    শূন্য capture_Ref_By_মান(Ast_গ্ৰন্থি_Var_Ref* nodep, CaptureMode capModeFlags) {
        AstVar* newVarp;
        ধ্রুৱ তার্কিক newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ newVarRefp = newCapture ? nodep->clone_বৃক্ষ(মিছা) : nullptr;
        যদি (!hasFlags(capModeFlags, CaptureMode::CAP_F_SET_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(nullptr);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        যদি (!newCapture) ঘূৰাই;
        যদি (hasFlags(capModeFlags, CaptureMode::CAP_F_XREF)) {
            AstVarRef* ধ্রুৱ notXVarRefp
                = নূতন AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_গ্ৰন্থি::addNext(m_argsp, নূতন AstArg{nodep->fileline(), "", newVarRefp});
    }

    শূন্য captureRefByThis(Ast_গ্ৰন্থি_Var_Ref* nodep, CaptureMode /*capModeFlags*/) {
        AstVar* ধ্রুৱ thisp = importThisp(nodep->fileline());
        AstVarRef* ধ্রুৱ thisRefp = নূতন AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(সঁচা);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ধ্রুৱ memberSelp
            = নূতন AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        যদি (!m_targetp && nodep->user1()) memberSelp->user1(সঁচা);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        যদি (m_ignore.গুনতি(nodep)) ঘূৰাই;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        যদি (nodep->varp()->isStdRandomizeArg()) ঘূৰাই;
        ধ্রুৱ CaptureMode capMode = getVarRefCaptureMode(nodep);
        যদি (mode(capMode) == CaptureMode::CAP_NO) ঘূৰাই;
        যদি (mode(capMode) == CaptureMode::CAP_মান) capture_Ref_By_মান(nodep, capMode);
        যদি (mode(capMode) == CaptureMode::CAP_THIS) captureRefByThis(nodep, capMode);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask_Ref* nodep) override {
        যদি (m_ignore.গুনতি(nodep)) {
            iterateChildren(nodep);
            ঘূৰাই;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        যদি (VN_IS(nodep, MethodCall) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            ঘূৰাই;
        }
        AstClass* ধ্রুৱ classp = VN_CAST(nodep->taskp()->user2p(), Class);
        যদি ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ধ্রুৱ argsp = nodep->argsp();
            যদি (argsp) argsp->unlinkFrBack();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ধ্রুৱ thisp = importThisp(nodep->fileline());
            AstVarRef* ধ্রুৱ thisRefp = নূতন AstVarRef{
                nodep->fileline(), thisp, nodep->isPure() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            AstMethodCall* ধ্রুৱ methodCallp
                = নূতন AstMethodCall{nodep->fileline(), thisRefp, thisp->name(), argsp};
            methodCallp->taskp(nodep->taskp());
            methodCallp->dtypep(nodep->dtypep());
            nodep->replaceWith(methodCallp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            m_ignore.emplace(methodCallp);
        }
    }
    শূন্য visit(AstMemberSel* nodep) override {
        যদি (!isReferenceToInnerMember(nodep)) {
            iterateChildren(nodep);
            ঘূৰাই;
        }
        AstVarRef* ধ্রুৱ varRefp
            = নূতন AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    শূন্য visit(AstMethodCall* nodep) override {
        যদি (!isReferenceToInnerMember(nodep) || m_ignore.গুনতি(nodep)) {
            iterateChildren(nodep);
            ঘূৰাই;
        }
        AstArg* ধ্রুৱ argsp = nodep->argsp();
        যদি (argsp) argsp->unlinkFrBackWithNext();
        Ast_গ্ৰন্থি_FTask_Ref* taskRefp = nullptr;
        যদি (AstTask* ধ্রুৱ taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = নূতন AstTaskRef{nodep->fileline(), taskp, argsp};
        } অন্যথা যদি (AstFunc* ধ্রুৱ taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = নূতন AstFuncRef{nodep->fileline(), taskp, argsp};
        } অন্যথা {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    explicit CaptureVisitor(Ast_গ্ৰন্থি* ধ্রুৱ nodep, Ast_গ্ৰন্থি_Module* callerp, AstClass* ধ্রুৱ targetp,
                            তার্কিক staticContext = মিছা)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterateAndNextNull(nodep);
    }

    // PUBLIC METHODS

    AstArg* getArgs() ধ্রুৱ { ঘূৰাই m_argsp; }
    AstVar* getThisp() ধ্রুৱ { ঘূৰাই m_thisp; }

    শূন্য addFunctionArguments(Ast_গ্ৰন্থি_FTask* funcp) ধ্রুৱ {
        বাবে (AstArg* argp = getArgs(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            যদি (Ast_গ্ৰন্থি_Var_Ref* varrefp = VN_CAST(argp->exprp(), গ্ৰন্থি_Var_Ref)) {
                যদি ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(nullptr);
                }
                funcp->addStmtsp(getVar(varrefp->varp()));
            } অন্যথা {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->addStmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

শ্ৰেণী RandomizeVisitor final : সার্ৱজনিক VNVisitor {
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
    ধ্রুৱ VNUser3InUse m_inuser3;
    ধ্রুৱ VNUser4InUse m_inuser4;

    // STATE
    V3UniqueNames m_inlineUniqueNames;  // For generating unique function names
    V3UniqueNames m_randArrayUniqueNames{"__Vrandarr"};  // Random array loop indices
    V3UniqueNames m_modeUniqueNames{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3UniqueNames m_inlineUniqueStdName{"__VStdrand"};
    V3UniqueNames m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    std::unordered_map<Ast_গ্ৰন্থি_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_গ্ৰন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্ৰন্থি_Stmt* m_stmtp = nullptr;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = nullptr;  // Dynamic array type (for rand mode)
    পূর্ণাঙ্ক m_rand_Case_অঙ্ক = 0;  // Randcase number within a module for var naming
    পূর্ণাঙ্ক m_dist_অঙ্ক = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = nullptr;  // Current constraint
    std::set<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertionSet<AstVar*>> m_sizeConstrainedArrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_staticConstraintModeVars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_staticRandModeVars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<তার্কিক, তার্কিক>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    স্থির তার্কিক is_Similar_গ্ৰন্থি(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* with_ৰাশিমালা, ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* arg_ৰাশিমালা) {
        // VarRef: compare variable pointers
        যদি (VN_IS(arg_ৰাশিমালা, VarRef) && VN_IS(with_ৰাশিমালা, VarRef)) {
            ঘূৰাই VN_AS(with_ৰাশিমালা, VarRef)->varp() == VN_AS(arg_ৰাশিমালা, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        যদি (VN_IS(arg_ৰাশিমালা, MemberSel) && VN_IS(with_ৰাশিমালা, MemberSel)) {
            ধ্রুৱ AstMemberSel* ধ্রুৱ withMSp = VN_AS(with_ৰাশিমালা, MemberSel);
            ধ্রুৱ AstMemberSel* ধ্রুৱ argMSp = VN_AS(arg_ৰাশিমালা, MemberSel);
            যদি (withMSp->varp() != argMSp->varp()) ঘূৰাই মিছা;
            // Recursively compare the base object expression
            ঘূৰাই is_Similar_গ্ৰন্থি(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        যদি (VN_IS(arg_ৰাশিমালা, ArraySel) && VN_IS(with_ৰাশিমালা, ArraySel)) {
            ধ্রুৱ AstArraySel* ধ্রুৱ withASp = VN_AS(with_ৰাশিমালা, ArraySel);
            ধ্রুৱ AstArraySel* ধ্রুৱ argASp = VN_AS(arg_ৰাশিমালা, ArraySel);
            // Before checking Sel type index, check if both are Const
            যদি (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ধ্রুৱ AstConst* ধ্রুৱ withIdxp = VN_AS(withASp->bitp(), Const);
                ধ্রুৱ AstConst* ধ্রুৱ argIdxp = VN_AS(argASp->bitp(), Const);
                ঘূৰাই is_Similar_গ্ৰন্থি(withASp->fromp(), argASp->fromp())
                       && withIdxp->অঙ্ক().isCaseEq(argIdxp->অঙ্ক());
            }
            // Index must be Sel type, extract VarRef using fromp()
            যদি (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) ঘূৰাই মিছা;
            ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            ঘূৰাই is_Similar_গ্ৰন্থি(withASp->fromp(), argASp->fromp())
                   && is_Similar_গ্ৰন্থি(withIdxp, argIdxp);
        }
        ঘূৰাই মিছা;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    স্থির শূন্য expandUniqueElementList(Ast_গ্ৰন্থি* itemsp) {
        Ast_গ্ৰন্থি* itemp = itemsp;
        যেতিয়ালৈ (itemp) {
            Ast_গ্ৰন্থি* ধ্রুৱ nextp = itemp->nextp();
            যদি (ধ্রুৱ AstConstraintIf* ধ্রুৱ ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                অনুৱর্ত;
            }
            যদি (ধ্রুৱ AstConstraintForeach* ধ্রুৱ foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                অনুৱর্ত;
            }
            AstConstraintUnique* ধ্রুৱ uniquep = VN_CAST(itemp, ConstraintUnique);
            যদি (!uniquep) {
                itemp = nextp;
                অনুৱর্ত;
            }

            std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> ৰাশিমালা_বস্তুসমূহ;
            তার্কিক hasArrayVarRef = মিছা;
            বাবে (Ast_গ্ৰন্থি* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                যদি (ধ্রুৱ AstVarRef* ধ্রুৱ vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = vrp->varp()->dtypep()->skipRefp();
                    যদি (VN_IS(dtypep, গ্ৰন্থি_Array_DType) || VN_IS(dtypep, QueueDType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = সঁচা;
                        অনুৱর্ত;
                    }
                }
                ৰাশিমালা_বস্তুসমূহ.ঠেলো_back(VN_AS(rp, গ্ৰন্থি_ৰাশিমালা));
            }

            যদি (ৰাশিমালা_বস্তুসমূহ.size() >= 2) {
                File_সাৰি* ধ্রুৱ fl = uniquep->fileline();
                বাবে (size_t i = 0; i < ৰাশিমালা_বস্তুসমূহ.size(); i++) {
                    বাবে (size_t j = i + 1; j < ৰাশিমালা_বস্তুসমূহ.size(); j++) {
                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = ৰাশিমালা_বস্তুসমূহ[i]->clone_বৃক্ষ(মিছা);
                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = ৰাশিমালা_বস্তুসমূহ[j]->clone_বৃক্ষ(মিছা);
                        AstNeq* ধ্রুৱ neqp = নূতন AstNeq{fl, lhsp, rhsp};
                        neqp->user1(সঁচা);
                        Ast_Constraint_ৰাশিমালা* ধ্রুৱ cexprp = নূতন Ast_Constraint_ৰাশিমালা{fl, neqp};
                        uniquep->addNextHere(cexprp);
                    }
                }
                যদি (!hasArrayVarRef) {
                    uniquep->unlinkFrBack();
                    VL_DO_DANGLING(uniquep->delete_বৃক্ষ(), uniquep);
                }
            } অন্যথা যদি (ৰাশিমালা_বস্তুসমূহ.size() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlinkFrBack();
                VL_DO_DANGLING(uniquep->delete_বৃক্ষ(), uniquep);
            }
            itemp = nextp;
        }
    }
    শূন্য createRandomGenerator(AstClass* ধ্রুৱ classp) {
        যদি (classp->user3p()) ঘূৰাই;
        যদি (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            ঘূৰাই;
        }
        AstVar* ধ্রুৱ genp = নূতন AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->addMembersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_গ্ৰন্থি_Module* ধ্রুৱ modp) {
        auto it = m_stdMap.find(modp);
        যদি (it == m_stdMap.end()) {
            AstVar* ধ্রুৱ stdgenp
                = নূতন AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->findBasicDType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3ErrorCode::IMPURE, সঁচা);
            modp->addStmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            ঘূৰাই stdgenp;
        }
        ঘূৰাই it->second;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ধ্রুৱ classp) {
        স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ name = "__Vprepare_constrained_arrays";
        AstTask* ধ্রুৱ taskp = নূতন AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(সঁচা);
        classp->addMembersp(taskp);
        m_memberMap.ঢোকাও(classp, taskp);
        ঘূৰাই taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ধ্রুৱ classp) {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.findMember(classp, name), Task);
        যদি (resizeTaskp) ঘূৰাই resizeTaskp;
        resizeTaskp = নূতন AstTask{classp->fileline(), name, nullptr};
        resizeTaskp->classMethod(সঁচা);
        resizeTaskp->isVirtual(সঁচা);
        classp->addMembersp(resizeTaskp);
        m_memberMap.ঢোকাও(classp, resizeTaskp);
        ঘূৰাই resizeTaskp;
    }
    AstVar* getCreateRandModeVar(AstClass* ধ্রুৱ classp) {
        যদি (classp->user2p()) ঘূৰাই VN_AS(classp->user2p(), Var);
        যদি (AstClassExtends* ধ্রুৱ extendsp = classp->extendsp()) {
            ঘূৰাই getCreateRandModeVar(extendsp->classp());
        }
        AstVar* ধ্রুৱ randModeVarp = createModeVar(classp, "__Vrandmode");
        classp->user2p(randModeVarp);
        ঘূৰাই randModeVarp;
    }
    AstVar* getCreateConstraintModeVar(AstClass* ধ্রুৱ classp) {
        যদি (classp->user4p()) ঘূৰাই VN_AS(classp->user4p(), Var);
        যদি (AstClassExtends* ধ্রুৱ extendsp = classp->extendsp()) {
            ঘূৰাই getCreateConstraintModeVar(extendsp->classp());
        }
        AstVar* ধ্রুৱ constraintModeVarp = createModeVar(classp, "__Vconstraintmode");
        classp->user4p(constraintModeVarp);
        ঘূৰাই constraintModeVarp;
    }
    স্থির AstVar* getConstraintModeVar(AstClass* ধ্রুৱ classp) {
        যদি (classp->user4p()) ঘূৰাই VN_AS(classp->user4p(), Var);
        যদি (AstClassExtends* ধ্রুৱ extendsp = classp->extendsp()) {
            ঘূৰাই getConstraintModeVar(extendsp->classp());
        }
        ঘূৰাই nullptr;
    }
    AstVar* getCreateStaticConstraintModeVar(AstClass* ধ্রুৱ classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        যদি (it != m_staticConstraintModeVars.end()) ঘূৰাই it->second;
        যদি (AstClassExtends* ধ্রুৱ extendsp = classp->extendsp()) {
            ঘূৰাই getCreateStaticConstraintModeVar(extendsp->classp());
        }
        AstVar* ধ্রুৱ staticModeVarp = createStaticModeVar(classp, "__Vstaticconstraintmode");
        m_staticConstraintModeVars[classp] = staticModeVarp;
        ঘূৰাই staticModeVarp;
    }
    AstVar* getStaticConstraintModeVar(AstClass* ধ্রুৱ classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        যদি (it != m_staticConstraintModeVars.end()) ঘূৰাই it->second;
        যদি (AstClassExtends* ধ্রুৱ extendsp = classp->extendsp()) {
            ঘূৰাই getStaticConstraintModeVar(extendsp->classp());
        }
        ঘূৰাই nullptr;
    }
    AstVar* getCreateStaticRandModeVar(AstClass* ধ্রুৱ classp) {
        যদি (m_staticRandModeVars.গুনতি(classp)) ঘূৰাই m_staticRandModeVars[classp];
        যদি (AstClassExtends* ধ্রুৱ extendsp = classp->extendsp()) {
            ঘূৰাই getCreateStaticRandModeVar(extendsp->classp());
        }
        AstVar* ধ্রুৱ staticModeVarp = createStaticModeVar(classp, "__Vstaticrandmode");
        m_staticRandModeVars[classp] = staticModeVarp;
        ঘূৰাই staticModeVarp;
    }
    AstVar* getStaticRandModeVar(AstClass* ধ্রুৱ classp) {
        যদি (m_staticRandModeVars.গুনতি(classp)) ঘূৰাই m_staticRandModeVars[classp];
        যদি (AstClassExtends* ধ্রুৱ extendsp = classp->extendsp()) {
            ঘূৰাই getStaticRandModeVar(extendsp->classp());
        }
        ঘূৰাই nullptr;
    }
    AstVar* createModeVar(AstClass* ধ্রুৱ classp, ধ্রুৱ ৱর্ণ* ধ্রুৱ name) {
        File_সাৰি* ধ্রুৱ fl = classp->fileline();
        যদি (!m_dynarrayDtp) {
            m_dynarrayDtp = নূতন AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ধ্রুৱ modeVarp = নূতন AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        ঘূৰাই modeVarp;
    }
    AstVar* createStaticModeVar(AstClass* ধ্রুৱ classp, ধ্রুৱ ৱর্ণ* ধ্রুৱ name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_সাৰি* ধ্রুৱ fl = classp->fileline();
        যদি (!m_dynarrayDtp) {
            m_dynarrayDtp = নূতন AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        AstVar* ধ্রুৱ modeVarp = নূতন AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        modeVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.ঢোকাও(classp, modeVarp);
        ঘূৰাই modeVarp;
    }
    স্থির শূন্য addSetRandMode(Ast_গ্ৰন্থি_FTask* ধ্রুৱ ftaskp, AstVar* ধ্রুৱ genp,
                               AstVar* ধ্রুৱ randModeVarp) {
        File_সাৰি* ধ্রুৱ fl = ftaskp->fileline();
        AstCMethodHard* ধ্রুৱ setRandModep = নূতন AstCMethodHard{
            fl, নূতন AstVarRef{fl, VN_AS(genp->user2p(), গ্ৰন্থি_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_SET_RANDMODE,
            নূতন AstVarRef{fl, VN_AS(randModeVarp->user2p(), গ্ৰন্থি_Module), randModeVarp,
                          VAccess::READ}};
        setRandModep->dtypeSetVoid();
        ftaskp->addStmtsp(setRandModep->makeStmt());
    }
    স্থির শূন্য addSetStaticRandMode(Ast_গ্ৰন্থি_FTask* ধ্রুৱ ftaskp, AstVar* ধ্রুৱ genp,
                                     AstVar* ধ্রুৱ staticRandModeVarp) {
        File_সাৰি* ধ্রুৱ fl = ftaskp->fileline();
        AstCMethodHard* ধ্রুৱ setp = নূতন AstCMethodHard{
            fl, নূতন AstVarRef{fl, VN_AS(genp->user2p(), গ্ৰন্থি_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_SET_STATIC_RANDMODE,
            নূতন AstVarRef{fl, VN_AS(staticRandModeVarp->user2p(), গ্ৰন্থি_Module), staticRandModeVarp,
                          VAccess::READ}};
        setp->dtypeSetVoid();
        ftaskp->addStmtsp(setp->makeStmt());
    }
    শূন্য createRandomizeClassVars(AstNetlist* ধ্রুৱ netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> মূল_Static_Rand_Mode_গুনতি;
        netlistp->foreach([এতত্, &মূল_Static_Rand_Mode_গুনতি](AstClass* ধ্রুৱ classp) {
            তার্কিক hasConstraints = মিছা;
            uint32_t rand_Mode_গুনতি = 0;
            uint32_t static_Rand_Mode_গুনতি = 0;
            uint32_t constraint_Mode_গুনতি = 0;
            uint32_t static_Constraint_Mode_গুনতি = 0;
            classp->foreachMember([&](AstClass*, Ast_গ্ৰন্থি* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                যদি (AstConstraint* ধ্রুৱ constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = সঁচা;
                    RandomizeMode constraintMode = {.asUQuad = memberp->user1()};
                    যদি (!constraintMode.usesMode) ঘূৰাই;
                    যদি (constraintMode.index == 0) {
                        // Use separate index counters for static vs non-static constraints
                        যদি (constrp->isStatic()) {
                            constraintMode.index = static_Constraint_Mode_গুনতি++;
                        } অন্যথা {
                            constraintMode.index = constraint_Mode_গুনতি++;
                        }
                        memberp->user1(constraintMode.asUQuad);
                    } অন্যথা {
                        যদি (constrp->isStatic()) {
                            static_Constraint_Mode_গুনতি = constraintMode.index + 1;
                        } অন্যথা {
                            constraint_Mode_গুনতি = constraintMode.index + 1;
                        }
                    }
                } অন্যথা যদি (AstVar* ধ্রুৱ varp = VN_CAST(memberp, Var)) {
                    RandomizeMode randMode = {.asUQuad = memberp->user1()};
                    যদি (!randMode.usesMode) ঘূৰাই;
                    ধ্রুৱ তার্কিক isStaticVar = varp->lifetime().isStatic();
                    যদি (randMode.index == 0) {
                        যদি (isStaticVar) {
                            randMode.index = static_Rand_Mode_গুনতি++;
                        } অন্যথা {
                            randMode.index = rand_Mode_গুনতি++;
                        }
                        memberp->user1(randMode.asUQuad);
                    } অন্যথা {
                        যদি (isStaticVar) {
                            static_Rand_Mode_গুনতি = randMode.index + 1;
                        } অন্যথা {
                            rand_Mode_গুনতি = randMode.index + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<শূন্য(AstClass*)> findSubObjRandModes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_গ্ৰন্থি* subMemberp) {
                        যদি (AstVar* ধ্রুৱ subVarp = VN_CAST(subMemberp, Var)) {
                            ধ্রুৱ RandomizeMode rm = {.asUQuad = subVarp->user1()};
                            যদি (!rm.usesMode) ঘূৰাই;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            যদি (subVarp->lifetime().isStatic()) ঘূৰাই;
                            ধ্রুৱ uint32_t needed = rm.index + 1;
                            যদি (needed > rand_Mode_গুনতি) rand_Mode_গুনতি = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    যদি (!memberVarp->globalConstrained()) ঘূৰাই;
                    ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = memberVarp->dtypep()->skipRefp();
                    ধ্রুৱ AstClassRefDType* ধ্রুৱ classRefp = VN_CAST(dtypep, ClassRefDType);
                    যদি (!classRefp) ঘূৰাই;
                    findSubObjRandModes(classRefp->classp());
                });
            }
            যদি (hasConstraints) createRandomGenerator(classp);
            যদি (rand_Mode_গুনতি > 0) {
                AstVar* ধ্রুৱ randModeVarp = getCreateRandModeVar(classp);
                makeModeInit(randModeVarp, classp, rand_Mode_গুনতি);
            }
            যদি (constraint_Mode_গুনতি > 0) {
                AstVar* ধ্রুৱ constraintModeVarp = getCreateConstraintModeVar(classp);
                makeModeInit(constraintModeVarp, classp, constraint_Mode_গুনতি);
            }
            যদি (static_Constraint_Mode_গুনতি > 0) {
                AstVar* ধ্রুৱ staticConstraintModeVarp = getCreateStaticConstraintModeVar(classp);
                makeStaticModeInit(staticConstraintModeVarp, classp, static_Constraint_Mode_গুনতি);
            }
            যদি (static_Rand_Mode_গুনতি > 0) {
                getCreateStaticRandModeVar(classp);
                AstClass* rootp = classp;
                যেতিয়ালৈ (AstClassExtends* ধ্রুৱ ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& slot = মূল_Static_Rand_Mode_গুনতি[rootp];
                যদি (static_Rand_Mode_গুনতি > slot) slot = static_Rand_Mode_গুনতি;
            }
        });
        বাবে (ধ্রুৱ auto& kv : মূল_Static_Rand_Mode_গুনতি) emit_মূল_Static_Mode_Init(kv.first, kv.second);
    }
    শূন্য emit_মূল_Static_Mode_Init(AstClass* ধ্রুৱ rootp, ধ্রুৱ uint32_t গুনতি) {
        AstVar* ধ্রুৱ staticRandModeVarp = m_staticRandModeVars[rootp];
        UASSERT_OBJ(staticRandModeVarp, rootp, "Root must have a static rand-mode var");
        makeStaticModeInit(staticRandModeVarp, rootp, গুনতি);
    }
    শূন্য makeModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_গুনতি) {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ modeVarModp = VN_AS(modeVarp->user2p(), গ্ৰন্থি_Module);
        File_সাৰি* ধ্রুৱ fl = modeVarp->fileline();
        AstVar* ধ্রুৱ oldSizeVarp = নূতন AstVar{
            fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__VoldSize"), modeVarp->findUInt32DType()};
        oldSizeVarp->funcLocal(সঁচা);
        oldSizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ধ্রুৱ oldSizep
            = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        oldSizep->dtypeSetUInt32();
        AstCMethodHard* ধ্রুৱ dynarrayNewp
            = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, নূতন AstConst{fl, mode_গুনতি}};
        dynarrayNewp->dtypeSetVoid();
        Ast_গ্ৰন্থি_FTask* ধ্রুৱ ctorNewp = VN_AS(m_memberMap.findMember(classp, "new"), গ্ৰন্থি_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_গ্ৰন্থি* ধ্রুৱ initFirstp = oldSizeVarp;
        initFirstp->addNext(
            নূতন AstAssign{fl, নূতন AstVarRef{fl, oldSizeVarp, VAccess::WRITE}, oldSizep});
        initFirstp->addNext(dynarrayNewp->makeStmt());
        initFirstp->addNext(makeModeSetLoop(
            fl, নূতন AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE}, নূতন AstConst{fl, 1},
            সঁচা, নূতন AstVarRef{fl, oldSizeVarp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        বাবে (Ast_গ্ৰন্থি* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (!VN_IS(stmtp, গ্ৰন্থি_Stmt)) অনুৱর্ত;  // Skip var declarations
            যদি (ধ্রুৱ Ast_Stmt_ৰাশিমালা* ধ্রুৱ sep = VN_CAST(stmtp, Stmt_ৰাশিমালা)) {
                যদি (VN_IS(sep->exprp(), New)) অনুৱর্ত;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->addHereThisAsNext(initFirstp);
            ঘূৰাই;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->addStmtsp(initFirstp);
    }
    শূন্য makeStaticModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_গুনতি) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_গ্ৰন্থি_Module* ধ্রুৱ modeVarModp = VN_AS(modeVarp->user2p(), গ্ৰন্থি_Module);
        File_সাৰি* ধ্রুৱ fl = modeVarp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ধ্রুৱ sizep
            = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstEq* ধ্রুৱ condp = নূতন AstEq{fl, sizep, নূতন AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ধ্রুৱ dynarrayNewp
            = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, নূতন AstConst{fl, mode_গুনতি}};
        dynarrayNewp->dtypeSetVoid();
        Ast_গ্ৰন্থি* ধ্রুৱ thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->addNext(
            makeModeSetLoop(fl, নূতন AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                            নূতন AstConst{fl, 1}, সঁচা));

        // Build the if statement
        AstIf* ধ্রুৱ ifp = নূতন AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_গ্ৰন্থি_FTask* ধ্রুৱ newp = VN_AS(m_memberMap.findMember(classp, "new"), গ্ৰন্থি_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->addStmtsp(ifp);
    }
    Ast_গ্ৰন্থি* makeModeSetLoop(File_সাৰি* ধ্রুৱ fl, Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp, Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp,
                             তার্কিক inTask, Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ startp = nullptr) {
        AstVar* ধ্রুৱ iterVarp = নূতন AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->findUInt32DType()};
        iterVarp->funcLocal(inTask);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ধ্রুৱ sizep = নূতন AstCMethodHard{fl, lhsp, VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstCMethodHard* ধ্রুৱ setp
            = নূতন AstCMethodHard{fl, lhsp->clone_বৃক্ষ(মিছা), VCMethod::ARRAY_AT_WRITE,
                                 নূতন AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtypeSetUInt32();
        Ast_গ্ৰন্থি* ধ্রুৱ stmtsp = iterVarp;
        stmtsp->addNext(নূতন AstAssign{fl, নূতন AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : নূতন AstConst{fl, 0}});

        AstLoop* ধ্রুৱ loopp = নূতন AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(নূতন AstLoopTest{
            fl, loopp, নূতন AstLt{fl, নূতন AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->addStmtsp(নূতন AstAssign{fl, setp, rhsp});
        loopp->addStmtsp(নূতন AstAssign{
            fl, নূতন AstVarRef{fl, iterVarp, VAccess::WRITE},
            নূতন AstAdd{fl, নূতন AstConst{fl, 1}, নূতন AstVarRef{fl, iterVarp, VAccess::READ}}});
        ঘূৰাই নূতন AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtsp, সঁচা};
    }
    শূন্য addTempVarsp(AstVar* varsp) {
        বাবে (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            যদি (m_ftaskp) {
                varp->funcLocal(সঁচা);
            } অন্যথা {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        যদি (!m_ftaskp) {
            m_modp->addStmtsp(varsp);
        } অন্যথা {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_গ্ৰন্থি_Stmt* wrapIfRandMode(AstClass* classp, AstVar* ধ্রুৱ varp, Ast_গ্ৰন্থি_Stmt* stmtp) {
        ধ্রুৱ RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstVar* ধ্রুৱ modeVarp = varp->lifetime().isStatic() ? getStaticRandModeVar(classp)
                                                             : getRandModeVarFromClass(classp);
        ঘূৰাই VN_AS(wrapIfMode(rmode, modeVarp, stmtp), গ্ৰন্থি_Stmt);
    }
    Ast_গ্ৰন্থি* wrapIfConstraintMode(AstClass* classp, AstConstraint* ধ্রুৱ constrp, Ast_গ্ৰন্থি* stmtp) {
        ধ্রুৱ RandomizeMode rmode = {.asUQuad = constrp->user1()};
        AstVar* ধ্রুৱ modeVarp = constrp->isStatic() ? getStaticConstraintModeVar(classp)
                                                     : getConstraintModeVar(classp);
        ঘূৰাই wrapIfMode(rmode, modeVarp, stmtp);
    }
    স্থির Ast_গ্ৰন্থি* wrapIfMode(ধ্রুৱ RandomizeMode mode, AstVar* modeVarp, Ast_গ্ৰন্থি* stmtp) {
        File_সাৰি* ধ্রুৱ fl = stmtp->fileline();
        যদি (mode.usesMode) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = VN_AS(modeVarp->user2p(), গ্ৰন্থি_Module);
            AstCMethodHard* ধ্রুৱ atp
                = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, modp, modeVarp, VAccess::READ},
                                     VCMethod::ARRAY_AT, নূতন AstConst{fl, mode.index}};
            atp->dtypeSetUInt32();
            ঘূৰাই নূতন AstIf{fl, atp, stmtp};
        }
        ঘূৰাই stmtp;
    }
    AstVarRef* enum_মান_Tab_Refp(AstEnumDType* ধ্রুৱ nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        যদি (nodep->user2p()) ঘূৰাই VN_AS(nodep->user2p(), VarRef)->clone_বৃক্ষ(মিছা);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ vardtypep = নূতন AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            নূতন AstRange{nodep->fileline(), static_cast<পূর্ণাঙ্ক>(nodep->item_গুনতি()) - 1, 0}};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        AstConsArray* ধ্রুৱ consp = নূতন AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t index = 0;
        বাবে (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ধ্রুৱ vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(index++, vconstp->clone_বৃক্ষ(মিছা));
        }
        // Share identical tables via the constant pool
        AstVarRef* ধ্রুৱ refp = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_বৃক্ষ(), consp);  // V3ConstPool::find clones it
        ঠেলো_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        ঘূৰাই refp->clone_বৃক্ষ(মিছা);
    }

    AstCDType* findVlRandCDType(File_সাৰি* ধ্রুৱ fl, uint64_t বস্তুসমূহ) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ধ্রুৱ std::string type = AstCDType::typeToHold(V3_সংখ্যা::log2bQuad(বস্তুসমূহ) + 1);
        ধ্রুৱ std::string name = "VlRandC<" + type + ", " + cvtToStr(বস্তুসমূহ) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ধ্রুৱ auto pair = m_randcDtypes.emplace(name, nullptr);
        যদি (pair.second) {
            AstCDType* ধ্রুৱ newp = নূতন AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            pair.first->second = newp;
        }
        ঘূৰাই pair.first->second;
    }

    AstVar* newRandcVarsp(AstVar* ধ্রুৱ varp, AstClass* ধ্রুৱ classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        যদি (!varp->isRandC()) ঘূৰাই nullptr;

        uint64_t বস্তুসমূহ = 0;
        যদি (AstEnumDType* ধ্রুৱ enumDtp = VN_CAST(varp->dtypep()->skipRefToEnump(), EnumDType)) {
            বস্তুসমূহ = static_cast<uint64_t>(enumDtp->item_গুনতি());
        } অন্যথা যদি (AstBasicDType* ধ্রুৱ basicp = varp->dtypep()->skipRefp()->basicp()) {
            যদি (basicp->width() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                যদি (!V3Error::error_গুনতি()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->width() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                ঘূৰাই nullptr;
            }
            বস্তুসমূহ = 1ULL << basicp->width();
        } অন্যথা যদি (AstStructDType* ধ্রুৱ dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            ঘূৰাই nullptr;
        } অন্যথা {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_গুনতি(), varp, "Unexpected randc variable dtype");
            ঘূৰাই nullptr;
        }
        AstCDType* ধ্রুৱ newdtp = findVlRandCDType(varp->fileline(), বস্তুসমূহ);
        AstVar* ধ্রুৱ newp
            = নূতন AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->isInternal(সঁচা);
        classp->addStmtsp(newp);
        UINFO(9, "created " << varp);
        ঘূৰাই newp;
    }
    Ast_গ্ৰন্থি_Stmt* createArrayForeachLoop(File_সাৰি* ধ্রুৱ fl, Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep,
                                        Ast_গ্ৰন্থি_ৰাশিমালা* exprp, AstVar* ধ্রুৱ outputVarp) {
        Ast_গ্ৰন্থি_DType* tempDTypep = dtypep;
        AstVar* randLoopIndxp = nullptr;
        auto createLoopIndex = [&](Ast_গ্ৰন্থি_DType* tempDTypep) {
            যদি (VN_IS(tempDTypep, AssocArrayDType)) {
                ঘূৰাই নূতন AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                                  VN_AS(tempDTypep, AssocArrayDType)->চাবি_DTypep()};
            }
            ঘূৰাই নূতন AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                              dtypep->findBasicDType(VBasicDTypeKwd::UINT32)};
        };
        Ast_গ্ৰন্থি_ৰাশিমালা* tempElementp = nullptr;
        যেতিয়ালৈ (tempDTypep->isNonPackedArray()) {
            AstVar* ধ্রুৱ newRandLoopIndxp = createLoopIndex(tempDTypep);
            randLoopIndxp = Ast_গ্ৰন্থি::addNext(randLoopIndxp, newRandLoopIndxp);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ tempExprp = tempElementp ? tempElementp : exprp;
            AstVarRef* ধ্রুৱ tempRefp = নূতন AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            যদি (VN_IS(tempDTypep, DynArrayDType)) {
                tempElementp
                    = নূতন AstCMethodHard{fl, tempExprp, VCMethod::ARRAY_AT_WRITE, tempRefp};
            } অন্যথা যদি (VN_IS(tempDTypep, UnpackArrayDType)) {
                Ast_গ্ৰন্থি_Array_DType* ধ্রুৱ aryDTypep = VN_CAST(tempDTypep, গ্ৰন্থি_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                tempElementp = নূতন AstArraySel{
                    fl, tempExprp,
                    নূতন AstSel{
                        fl,
                        নূতন AstSub{fl, tempRefp,
                                   নূতন AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        নূতন AstConst{fl, 0}, V3_সংখ্যা::log2b(aryDTypep->hi()) + 1}};
            } অন্যথা যদি (VN_IS(tempDTypep, AssocArrayDType)) {
                tempElementp = নূতন AstAssocSel{fl, tempExprp, tempRefp};
            } অন্যথা যদি (VN_IS(tempDTypep, QueueDType)) {
                tempElementp
                    = নূতন AstCMethodHard{fl, tempExprp, VCMethod::DYN_AT_WRITE_APPEND, tempRefp};
            }
            tempElementp->dtypep(tempDTypep->subDTypep());
            tempDTypep = tempDTypep->virtRefDTypep();
        }

        AstForeachHeader* ধ্রুৱ headerp
            = নূতন AstForeachHeader{fl, exprp->clone_বৃক্ষ(মিছা), randLoopIndxp};
        Ast_গ্ৰন্থি_Stmt* ধ্রুৱ randStmtsp = newRandStmtsp(fl, tempElementp, nullptr, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        যদি (!tempElementp->backp()) VL_DO_DANGLING(ঠেলো_Deletep(tempElementp), tempElementp);
        ঘূৰাই নূতন AstForeach{fl, headerp, randStmtsp};
    }
    Ast_গ্ৰন্থি_Stmt* newRandStmtsp(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* exprp, AstVar* randcVarp,
                               AstVar* ধ্রুৱ outputVarp, পূর্ণাঙ্ক offset = 0,
                               AstMemberDType* memberp = nullptr) {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        যদি (ধ্রুৱ auto* ধ্রুৱ structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_গ্ৰন্থি_Stmt* stmtsp = nullptr;
            যদি (structDtp->packed()) offset += memberp ? memberp->lsb() : 0;
            বাবে (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_গ্ৰন্থি_Stmt* randp = nullptr;
                যদি (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_বৃক্ষ(মিছা) : exprp, nullptr,
                                          outputVarp, offset, smemberp);
                } অন্যথা যদি (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = নূতন AstStructSel{fl, exprp->clone_বৃক্ষ(মিছা), smemberp->name()};
                    structSelp->dtypep(smemberp->childDTypep());
                    যদি (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, nullptr, outputVarp);
                }
                stmtsp = Ast_গ্ৰন্থি::addNextNull(stmtsp, randp);
            }
            ঘূৰাই stmtsp;
        } অন্যথা যদি (ধ্রুৱ auto* ধ্রুৱ unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            যদি (!unionDtp->packed()) ঘূৰাই nullptr;
            AstMemberDType* ধ্রুৱ firstMemberp = unionDtp->membersp();
            ঘূৰাই newRandStmtsp(fl, exprp, nullptr, outputVarp, offset, firstMemberp);
        } অন্যথা যদি (ধ্রুৱ AstClassRefDType* ধ্রুৱ classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ধ্রুৱ memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            AstMethodCall* ধ্রুৱ callp = নূতন AstMethodCall{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            AstAssign* ধ্রুৱ assignp = নূতন AstAssign{
                fl, নূতন AstVarRef{fl, outputVarp, VAccess::WRITE},
                নূতন AstAnd{fl, নূতন AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            ঘূৰাই নূতন AstIf{
                fl, নূতন AstNeq{fl, exprp->clone_বৃক্ষ(মিছা), নূতন AstConst{fl, AstConst::Null{}}},
                assignp};
        } অন্যথা যদি (AstDynArrayDType* ধ্রুৱ dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            ঘূৰাই createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } অন্যথা যদি (AstQueueDType* ধ্রুৱ queueDtp = VN_CAST(memberDtp, QueueDType)) {
            ঘূৰাই createArrayForeachLoop(fl, queueDtp, exprp, outputVarp);
        } অন্যথা যদি (AstUnpackArrayDType* ধ্রুৱ unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            ঘূৰাই createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } অন্যথা যদি (AstAssocArrayDType* ধ্রুৱ assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            ঘূৰাই createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } অন্যথা {
            Ast_গ্ৰন্থি_ৰাশিমালা* valp;
            যদি (AstEnumDType* ধ্রুৱ enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ধ্রুৱ tabRefp = enum_মান_Tab_Refp(enumDtp);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ randp
                    = new_Rand_মান(fl, randcVarp, exprp->findBasicDType(VBasicDTypeKwd::UINT32));
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ moddivp = নূতন AstModDiv{
                    fl, randp, নূতন AstConst{fl, static_cast<uint32_t>(enumDtp->item_গুনতি())}};
                moddivp->dtypep(enumDtp);
                valp = নূতন AstArraySel{fl, tabRefp, moddivp};
            } অন্যথা {
                valp
                    = new_Rand_মান(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            AstAssign* assignp
                = নূতন AstAssign{fl,
                                নূতন AstSel{fl, exprp, offset + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->width() : exprp->width()},
                                valp};
            AstVar* varp = nullptr;
            exprp->exists([&](ধ্রুৱ AstVarRef* varrefp) {
                যদি (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                ঘূৰাই varp != nullptr;
            });
            ঘূৰাই wrapIfRandMode(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* new_Rand_মান(File_সাৰি* ধ্রুৱ fl, AstVar* ধ্রুৱ randcVarp,
                              Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep) {
        যদি (randcVarp) {
            AstCExpr* ধ্রুৱ cexprp = নূতন AstCExpr{fl};
            cexprp->add(নূতন AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->add(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            ঘূৰাই cexprp;
        }

        ঘূৰাই নূতন AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* findPrePostTask(AstClass* classp, ধ্রুৱ string& name) {
        বাবে (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            যদি (AstTask* ধ্রুৱ taskp = VN_CAST(m_memberMap.findMember(cp, name), Task)) {
                ঘূৰাই taskp;
            }
        }
        ঘূৰাই nullptr;
    }
    শূন্য addPrePostCall(AstClass* ধ্রুৱ classp, Ast_গ্ৰন্থি_FTask* ধ্রুৱ funcp, ধ্রুৱ string& name) {
        যদি (AstTask* ধ্রুৱ userFuncp = findPrePostTask(classp, name)) {
            AstTaskRef* ধ্রুৱ callp = নূতন AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->addStmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* ধ্রুৱ classp, ধ্রুৱ string& which) {
        ধ্রুৱ string name = "__V" + which;
        যদি (AstTask* ধ্রুৱ existingp = VN_CAST(m_memberMap.findMember(classp, name), Task)) {
            ঘূৰাই existingp;
        }
        AstTask* ধ্রুৱ taskp = নূতন AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(সঁচা);
        taskp->isVirtual(classp->isExtended());
        classp->addMembersp(taskp);
        m_memberMap.ঢোকাও(classp, taskp);
        addPrePostCall(classp, taskp, which);
        ঘূৰাই taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<তার্কিক, তার্কিক> buildPrePostVirtualWrappers(AstClass* ধ্রুৱ classp) {
        ধ্রুৱ auto cachedIt = m_prePostWrap.find(classp);
        যদি (cachedIt != m_prePostWrap.end()) ঘূৰাই cachedIt->second;
        std::vector<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            যদি (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.ঠেলো_back(subp);
        });
        তার্কিক hasPre = মিছা;
        তার্কিক hasPost = মিছা;
        বাবে (AstClass* ধ্রুৱ cp : hierp) {
            যদি (findPrePostTask(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = সঁচা;
            }
            যদি (findPrePostTask(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = সঁচা;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        যদি (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        যদি (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ধ্রুৱ std::pair<তার্কিক, তার্কিক> result{hasPre, hasPost};
        m_prePostWrap.emplace(classp, result);
        ঘূৰাই result;
    }
    শূন্য addVirtualPrePostCall(AstFunc* ধ্রুৱ randomizeFuncp, AstClass* ধ্রুৱ classp,
                               ধ্রুৱ string& which) {
        File_সাৰি* ধ্রুৱ fl = classp->fileline();
        AstTask* ধ্রুৱ wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ধ্রুৱ refDTypep = নূতন AstClassRefDType{fl, classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
        AstMethodCall* ধ্রুৱ callp
            = নূতন AstMethodCall{fl, নূতন AstThisRef{fl, refDTypep}, wrapperp->name(), nullptr};
        callp->taskp(wrapperp);
        callp->dtypeSetVoid();
        randomizeFuncp->addStmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    তার্কিক classHasRandClassMembers(AstClass* classp) {
        ঘূৰাই classp->existsMember([](ধ্রুৱ AstClass*, ধ্রুৱ AstVar* varp) {
            যদি (!varp->rand().isRandomizable()) ঘূৰাই মিছা;
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = varp->dtypep()->skipRefp();
            ঘূৰাই VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    তার্কিক classOwnsGlobalConstraint(ধ্রুৱ AstClass* classp) ধ্রুৱ {
        ঘূৰাই classp->existsMember([](ধ্রুৱ AstClass*, ধ্রুৱ AstConstraint* constrp) {
            তার্কিক owns = মিছা;
            constrp->foreach([&](ধ্রুৱ AstMemberSel* memberSelp) {
                ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ rootp
                    = const_cast<AstMemberSel*>(memberSelp)->baseFromp(সঁচা);
                যদি (ধ্রুৱ AstVarRef* ধ্রুৱ refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    যেতিয়ালৈ (ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    যদি (VN_IS(dtypep, ClassRefDType)) owns = সঁচা;
                }
            });
            ঘূৰাই owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ধ্রুৱ string& suffix) {
        ধ্রুৱ string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.findMember(classp, name), Task);
        যদি (taskp) ঘূৰাই taskp;
        taskp = নূতন AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(সঁচা);
        classp->addMembersp(taskp);
        m_memberMap.ঢোকাও(classp, taskp);
        ঘূৰাই taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    শূন্য populateNestedCallbackTask(AstTask* ধ্রুৱ callbackTaskp, AstClass* ধ্রুৱ classp,
                                    ধ্রুৱ string& cbName) {
        File_সাৰি* ধ্রুৱ fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            যদি (!memberVarp->rand().isRandomizable()) ঘূৰাই;
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = memberVarp->dtypep()->skipRefp();
            ধ্রুৱ AstClassRefDType* ধ্রুৱ classRefp = VN_CAST(dtypep, ClassRefDType);
            যদি (!classRefp) ঘূৰাই;
            AstClass* ধ্রুৱ memberClassp = classRefp->classp();
            যদি (memberClassp == classp) ঘূৰাই;  // Avoid self-reference

            // Force-visit member class if not yet processed
            যদি (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_গ্ৰন্থি* stmtsp = nullptr;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            যদি (AstTask* ধ্রুৱ userFuncp = findPrePostTask(memberClassp, cbName)) {
                AstMethodCall* ধ্রুৱ callp = নূতন AstMethodCall{
                    fl, নূতন AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtypeSetVoid();
                stmtsp = Ast_গ্ৰন্থি::addNext(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            যদি (classHasRandClassMembers(memberClassp)) {
                ধ্রুৱ string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ধ্রুৱ nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                AstMethodCall* ধ্রুৱ recurseCallp = নূতন AstMethodCall{
                    fl, নূতন AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtypeSetVoid();
                stmtsp = Ast_গ্ৰন্থি::addNext(stmtsp, recurseCallp->makeStmt());
            }

            যদি (!stmtsp) ঘূৰাই;

            // Wrap in null check
            AstIf* ধ্রুৱ nullCheckp = নূতন AstIf{
                fl,
                নূতন AstNeq{fl, নূতন AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           নূতন AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->addStmtsp(wrapIfRandMode(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* newSetupConstraintTask(AstClass* ধ্রুৱ nodep, ধ্রুৱ std::string& name) {
        AstTask* ধ্রুৱ taskp = নূতন AstTask{nodep->fileline(), name + "_setup_constraint", nullptr};
        taskp->classMethod(সঁচা);
        nodep->addMembersp(taskp);
        ঘূৰাই taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ধ্রুৱ nodep, ধ্রুৱ std::string& name) {
        AstTask* ধ্রুৱ taskp
            = নূতন AstTask{nodep->fileline(), name + "_resize_constrained_array", nullptr};
        taskp->classMethod(সঁচা);
        nodep->addMembersp(taskp);
        ঘূৰাই taskp;
    }
    Ast_গ্ৰন্থি_Stmt* implementConstraintsClear(File_সাৰি* ধ্রুৱ fileline, AstVar* ধ্রুৱ genp) {
        AstCMethodHard* ধ্রুৱ clearp = নূতন AstCMethodHard{
            fileline,
            নূতন AstVarRef{fileline, VN_AS(genp->user2p(), গ্ৰন্থি_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtypeSetVoid();
        ঘূৰাই clearp->makeStmt();
    }
    Ast_গ্ৰন্থি_Stmt* implementConstraintsClearAll(File_সাৰি* ধ্রুৱ fileline, AstVar* ধ্রুৱ genp) {
        AstCMethodHard* ধ্রুৱ clearp = নূতন AstCMethodHard{
            fileline,
            নূতন AstVarRef{fileline, VN_AS(genp->user2p(), গ্ৰন্থি_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtypeSetVoid();
        ঘূৰাই clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp) {
        যদি (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(exprp, MemberSel)) {
            ঘূৰাই memberSelp->varp();
        } অন্যথা যদি (AstVarRef* ধ্রুৱ varrefp = VN_CAST(exprp, VarRef)) {
            ঘূৰাই varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        ঘূৰাই nullptr;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_গ্ৰন্থি_ৰাশিমালা* makeModeVarRef(Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp, AstVar* ধ্রুৱ modeVarp,
                                ধ্রুৱ VAccess access) {
        যদি (modeVarp->lifetime().isStatic()) {
            ঘূৰাই নূতন AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), গ্ৰন্থি_Module),
                                 modeVarp, access};
        }
        যদি (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ধ্রুৱ newMemberSelp = নূতন AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_বৃক্ষ(মিছা), modeVarp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            ঘূৰাই newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        ঘূৰাই নূতন AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), Class), modeVarp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* createOrGetSizeVar(AstClass* ধ্রুৱ classp, AstVar* ধ্রুৱ arrVarp, File_সাৰি* ধ্রুৱ fl,
                               Ast_গ্ৰন্থি_DType* ধ্রুৱ signed32DTypep, তার্কিক& wasCreated) {
        AstVar* sizeVarp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = মিছা;
        যদি (!sizeVarp) {
            sizeVarp = নূতন AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->addMembersp(sizeVarp);
            m_memberMap.ঢোকাও(classp, sizeVarp);
            sizeVarp->user2p(classp);
            arrVarp->user4p(sizeVarp);
            wasCreated = সঁচা;
        }
        ঘূৰাই sizeVarp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_ৰাশিমালা* createSizeGteZeroConstraint(File_সাৰি* ধ্রুৱ fl, AstVar* ধ্রুৱ sizeVarp,
                                                   Ast_গ্ৰন্থি_ৰাশিমালা* derefFromp = nullptr) {
        Ast_গ্ৰন্থি_ৰাশিমালা* size_গ্ৰন্থি_Exprp = nullptr;
        যদি (derefFromp) {
            size_গ্ৰন্থি_Exprp = buildMemberSelExprp(derefFromp, sizeVarp);
        } অন্যথা {
            size_গ্ৰন্থি_Exprp = নূতন AstVarRef{fl, sizeVarp, VAccess::READ};
        }
        size_গ্ৰন্থি_Exprp->user1(সঁচা);
        AstGteS* ধ্রুৱ sizeGtep = নূতন AstGteS{fl, size_গ্ৰন্থি_Exprp, নূতন AstConst{fl, 0}};
        sizeGtep->user1(সঁচা);
        ঘূৰাই নূতন Ast_Constraint_ৰাশিমালা{fl, sizeGtep};
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* getFromp(Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp) {
        যদি (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(exprp, MemberSel)) {
            ঘূৰাই memberSelp->fromp();
        } অন্যথা যদি (AstMethodCall* ধ্রুৱ methodCallp = VN_CAST(exprp, MethodCall)) {
            ঘূৰাই methodCallp->fromp();
        }
        ঘূৰাই nullptr;
    }
    AstVar* makeTmpRandModeVar(Ast_গ্ৰন্থি_ৰাশিমালা* siblingExprp, AstVar* randModeVarp,
                               Ast_গ্ৰন্থি*& storeStmtspr, Ast_গ্ৰন্থি_Stmt*& restoreStmtspr) {
        File_সাৰি* ধ্রুৱ fl = randModeVarp->fileline();
        AstVar* ধ্রুৱ randModeTmpVarp = নূতন AstVar{
            fl, VVarType::BLOCKTEMP, m_modeUniqueNames.get(randModeVarp), randModeVarp->dtypep()};
        randModeTmpVarp->funcLocal(m_ftaskp);
        randModeTmpVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_গ্ৰন্থি::addNext(
            storeStmtspr,
            নূতন AstAssign{fl, নূতন AstVarRef{fl, randModeTmpVarp, VAccess::WRITE},
                          makeModeVarRef(siblingExprp, randModeVarp, VAccess::READ)});
        storeStmtspr = Ast_গ্ৰন্থি::addNext(
            storeStmtspr,
            makeModeSetLoop(fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                            নূতন AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_গ্ৰন্থি::addNext(
            restoreStmtspr,
            নূতন AstAssign{fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                          নূতন AstVarRef{fl, randModeTmpVarp, VAccess::READ}});
        ঘূৰাই randModeTmpVarp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_গ্ৰন্থি_ৰাশিমালা* sliceToCommonPrefix(Ast_গ্ৰন্থি_ৰাশিমালা* thisp, Ast_গ্ৰন্থি_ৰাশিমালা* otherp) {
        স্থির std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        যেতিয়ালৈ (thisp) {
            thisHier.ঠেলো_back(thisp);
            thisp = getFromp(thisp);
        }
        যেতিয়ালৈ (otherp) {
            otherHier.ঠেলো_back(otherp);
            otherp = getFromp(otherp);
        }
        Ast_গ্ৰন্থি_ৰাশিমালা* commonp = nullptr;
        বাবে (auto thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            যদি ((*thisIt)->type() != (*otherIt)->type()) ভঙ্গ;
            যদি (AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ধ্রুৱ otherMemberSelp = VN_AS(*otherIt, MemberSel);
                যদি (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    অনুৱর্ত;
                }
            } অন্যথা যদি (AstMethodCall* ধ্রুৱ thisMethodCallp = VN_CAST(*thisIt, MethodCall)) {
                AstMethodCall* ধ্রুৱ otherMethodCallp = VN_AS(*otherIt, MethodCall);
                যদি (thisMethodCallp->taskp() == otherMethodCallp->taskp()) {
                    commonp = thisMethodCallp;
                    অনুৱর্ত;
                }
            } অন্যথা যদি (AstVarRef* ধ্রুৱ firstVarRefp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ধ্রুৱ secondVarRefp = VN_AS(*otherIt, VarRef);
                যদি (firstVarRefp->varp() == secondVarRefp->varp()) {
                    commonp = firstVarRefp;
                    অনুৱর্ত;
                }
            }
            ভঙ্গ;
        }
        ঘূৰাই commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_গ্ৰন্থি_ৰাশিমালা* buildMemberSelExprp(Ast_গ্ৰন্থি_ৰাশিমালা* exprp, AstVar* deepestVarp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_গ্ৰন্থি_ৰাশিমালা* resExprp = nullptr;
        যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), nullptr);
            AstVar* varp = memberSelp->varp();
            যদি (deepestVarp) varp = deepestVarp;
            resExprp = নূতন AstMemberSel{exprp->fileline(), resExprp, varp};
        } অন্যথা যদি (ধ্রুৱ AstVarRef* ধ্রুৱ varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            যদি (deepestVarp) varp = deepestVarp;
            resExprp = নূতন AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), গ্ৰন্থি_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(সঁচা);
        ঘূৰাই resExprp;
    }

    শূন্য addUpdateRandVarsBody(AstClass* ধ্রুৱ nodep, AstFunc* ধ্রুৱ randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ধ্রুৱ updatep
            = নূতন AstFunc{nodep->fileline(), "__VupdateRandVars", nullptr, nullptr};
        updatep->classMethod(সঁচা);
        updatep->isVirtual(সঁচা);
        updatep->keepAlive(সঁচা);
        updatep->needsSyms(মিছা);
        nodep->addMembersp(updatep);
        nodep->hasRandVarsUpdate(সঁচা);

        বাবে (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            Ast_গ্ৰন্থি_FTask* ধ্রুৱ newp = VN_AS(m_memberMap.findMember(classp, "new"), গ্ৰন্থি_FTask);
            বাবে (Ast_গ্ৰন্থি* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_ৰাশিমালা* ধ্রুৱ stmtExprp = VN_CAST(stmtp, Stmt_ৰাশিমালা);
                যদি (!stmtExprp) অনুৱর্ত;
                AstCMethodHard* ধ্রুৱ methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                যদি (!methodp || methodp->method() != VCMethod::RANDOMIZER_WRITE_VAR) অনুৱর্ত;

                Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ varRefp = VN_AS(methodp->pinsp(), গ্ৰন্থি_Var_Ref);
                যদি (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    অনুৱর্ত;
                }
                AstCMethodHard* ধ্রুৱ updateMethodp = নূতন AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_বৃক্ষ_Pure(মিছা),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_বৃক্ষ_Pure(মিছা)};
                updateMethodp->dtypeSetVoid();
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ namep = VN_AS(varRefp->nextp()->nextp(), গ্ৰন্থি_ৰাশিমালা);
                updateMethodp->addPinsp(namep->clone_বৃক্ষ_Pure(মিছা));
                updateMethodp->method(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->addStmtsp(updateMethodp->makeStmt());
            }
        }
    }

    শূন্য addBasicRandomizeBody(AstFunc* ধ্রুৱ basicRandomizep, AstClass* ধ্রুৱ nodep,
                               AstVar* randModeVarp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        AstVar* ধ্রুৱ basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ধ্রুৱ basicFvarRefp = নূতন AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ধ্রুৱ beginBasicValp = নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 1};
        basicRandomizep->addStmtsp(নূতন AstAssign{fl, basicFvarRefp, beginBasicValp});
        Ast_গ্ৰন্থি_FTask* ধ্রুৱ newp = VN_AS(m_memberMap.findMember(nodep, "new"), গ্ৰন্থি_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            যদি (!memberVarp->rand().isRandomizable()) ঘূৰাই;
            ধ্রুৱ RandomizeMode randMode = {.asUQuad = memberVarp->user1()};
            যদি (randMode.usesMode
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ধ্রুৱ setp = নূতন AstCMethodHard{
                    nodep->fileline(),
                    নূতন AstVarRef{fl, VN_AS(randModeVarp->user2p(), গ্ৰন্থি_Module), randModeVarp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, নূতন AstConst{nodep->fileline(), randMode.index}};
                setp->dtypeSetUInt32();
                newp->addStmtsp(নূতন AstAssign{fl, setp, নূতন AstConst{fl, 0}});
            }
            যদি (memberVarp->user3()) ঘূৰাই;  // Handled in constraints
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = memberVarp->dtypep()->skipRefp();
            যদি (ধ্রুৱ AstClassRefDType* ধ্রুৱ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                যদি (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    ঘূৰাই;
                }
                AstFunc* ধ্রুৱ memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                AstMethodCall* ধ্রুৱ callp
                    = memberVarp->globalConstrained()
                          ? নূতন AstMethodCall{fl,
                                              নূতন AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : নূতন AstMethodCall{
                                fl, নূতন AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ধ্রুৱ basicFvarRefReadp = basicFvarRefp->clone_বৃক্ষ(মিছা);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ধ্রুৱ assignIfNotNullp = নূতন AstIf{
                    fl,
                    নূতন AstNeq{fl, নূতন AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               নূতন AstConst{fl, AstConst::Null{}}},
                    নূতন AstAssign{fl, basicFvarRefp->clone_বৃক্ষ(মিছা),
                                  নূতন AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->addStmtsp(wrapIfRandMode(nodep, memberVarp, assignIfNotNullp));
            } অন্যথা {
                AstVar* ধ্রুৱ randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ধ্রুৱ refp
                    = নূতন AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_গ্ৰন্থি_Stmt* ধ্রুৱ stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                যদি (!refp->backp()) VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
                basicRandomizep->addStmtsp(
                    নূতন AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtp, মিছা});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_গ্ৰন্থি_ৰাশিমালা* makeModeAssignLhs(File_সাৰি* ধ্রুৱ fl, AstClass* ধ্রুৱ classp,
                                   Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp, AstVar* ধ্রুৱ modeVarp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        যদি (modeVarp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            যদি (fromp) VL_DO_DANGLING(fromp->unlinkFrBack()->delete_বৃক্ষ(), fromp);
            ঘূৰাই নূতন AstVarRef{fl, VN_AS(modeVarp->user2p(), গ্ৰন্থি_Module), modeVarp,
                                 VAccess::WRITE};
        } অন্যথা যদি (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            ঘূৰাই নূতন AstVarRef{fl, VN_AS(modeVarp->user2p(), গ্ৰন্থি_Module), modeVarp,
                                 VAccess::WRITE};
        } অন্যথা {
            AstMemberSel* ধ্রুৱ memberselp = নূতন AstMemberSel{fl, fromp->unlinkFrBack(), modeVarp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            ঘূৰাই memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    শূন্য replaceWithModeAssign(Ast_গ্ৰন্থি_FTask_Ref* ধ্রুৱ ftaskRefp, Ast_গ্ৰন্থি* ধ্রুৱ receiverp,
                               Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp) {
        replaceWithModeAssignAndAppend(ftaskRefp, receiverp, lhsp, nullptr);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    শূন্য replaceWithModeAssignAndAppend(Ast_গ্ৰন্থি_FTask_Ref* ধ্রুৱ ftaskRefp, Ast_গ্ৰন্থি* ধ্রুৱ receiverp,
                                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp, Ast_গ্ৰন্থি* ধ্রুৱ appendStmtp) {
        File_সাৰি* ধ্রুৱ fl = ftaskRefp->fileline();
        যদি (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_ৰাশিমালা), ftaskRefp, "Should be a statement");
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = ftaskRefp->argsp()->exprp()->unlinkFrBack();
            Ast_গ্ৰন্থি* newStmtp = nullptr;
            যদি (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                // mode
                ধ্রুৱ RandomizeMode rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ধ্রুৱ setp = নূতন AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                নূতন AstConst{fl, rmode.index}};
                setp->dtypeSetUInt32();
                newStmtp = নূতন AstAssign{fl, setp, rhsp};
            } অন্যথা {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = makeModeSetLoop(fl, lhsp, rhsp, m_ftaskp);
            }
            যদি (appendStmtp) newStmtp->addNext(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(ঠেলো_Deletep(m_stmtp), m_stmtp = nullptr);
        } অন্যথা {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ধ্রুৱ RandomizeMode rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ধ্রুৱ setp = নূতন AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            নূতন AstConst{fl, rmode.index}};
            setp->dtypeSetUInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(ঠেলো_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    শূন্য wrapRandomizeCallWithNullGuard(Ast_গ্ৰন্থি_FTask_Ref* nodep) {
        AstMethodCall* ধ্রুৱ callp = VN_CAST(nodep, MethodCall);
        যদি (!callp) ঘূৰাই;
        যদি (callp->user4SetOnce()) ঘূৰাই;
        File_সাৰি* ধ্রুৱ fl = callp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ checkp
            = নূতন AstNeq{fl, callp->fromp()->clone_বৃক্ষ(মিছা), নূতন AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlinkFrBack(&relinker);
        relinker.relink(নূতন AstCond{fl, checkp, callp, নূতন AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    শূন্য handleRandomizeArgs(Ast_গ্ৰন্থি_FTask_Ref* ধ্রুৱ nodep) {
        যদি (!nodep->argsp()) ঘূৰাই;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        তার্কিক hasNullArg = মিছা;
        বাবে (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->অঙ্ক().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = সঁচা;
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        বাবে (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            বাবে (AstArg *otherArgp = nextp, *otherNextp = nullptr; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ prefixp
                    = sliceToCommonPrefix(argp->exprp(), otherArgp->exprp())) {
                    যদি (prefixp == argp->exprp()) {
                        যদি (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlinkFrBack()->delete_বৃক্ষ(), otherArgp);
                        অনুৱর্ত;
                    }
                }
                যদি (Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ prefixp
                    = sliceToCommonPrefix(otherArgp->exprp(), argp->exprp())) {
                    যদি (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
                        ভঙ্গ;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::set<AstVar*> savedRandModeVarps;
        AstVar* tmpVarps = nullptr;
        Ast_গ্ৰন্থি* storeStmtsp = nullptr;
        Ast_গ্ৰন্থি* setStmtsp = nullptr;
        Ast_গ্ৰন্থি_Stmt* restoreStmtsp = nullptr;
        বাবে (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_গ্ৰন্থি_ৰাশিমালা* exprp = argp->exprp();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ commonPrefixp = sliceToCommonPrefix(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_সাৰি* ধ্রুৱ fl = argp->fileline();
            যেতিয়ালৈ (exprp) {
                যদি (commonPrefixp == exprp) ভঙ্গ;
                AstVar* ধ্রুৱ randVarp = getVarFromRef(exprp);
                AstClass* ধ্রুৱ classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ধ্রুৱ randModeVarp = randVarp->lifetime().isStatic()
                                                 ? getStaticRandModeVar(classp)
                                                 : getRandModeVarFromClass(classp);
                UASSERT_OBJ(randModeVarp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                যদি (savedRandModeVarps.find(randModeVarp) == savedRandModeVarps.end()) {
                    AstVar* ধ্রুৱ randModeTmpVarp
                        = makeTmpRandModeVar(exprp, randModeVarp, storeStmtsp, restoreStmtsp);
                    savedRandModeVarps.ঢোকাও(randModeVarp);
                    tmpVarps = Ast_গ্ৰন্থি::addNext(tmpVarps, randModeTmpVarp);
                }
                ধ্রুৱ RandomizeMode randMode = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = নূতন AstCMethodHard{
                    fl, makeModeVarRef(exprp, randModeVarp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, নূতন AstConst{fl, randMode.index}};
                setp->dtypeSetUInt32();
                setStmtsp
                    = Ast_গ্ৰন্থি::addNext(setStmtsp, নূতন AstAssign{fl, setp, নূতন AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlinkFrBack()->delete_বৃক্ষ(), argp);
        }
        যদি (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* targetClassp = nullptr;
            AstMethodCall* ধ্রুৱ methodCallp = VN_CAST(nodep, MethodCall);
            যদি (methodCallp) {
                ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDTypep = methodCallp->fromp()->dtypep();
                ধ্রুৱ AstClassRefDType* ধ্রুৱ crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                targetClassp = crdtp->classp();
            } অন্যথা {
                targetClassp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(targetClassp, nodep, "randomize(null) target class unresolved");
            AstFunc* ধ্রুৱ checkOnlyFuncp = getCreateRandomizeNullFunc(targetClassp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        যদি (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && setStmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlinkFrBack(&relinker);
            Ast_গ্ৰন্থি* ধ্রুৱ stmtsp = storeStmtsp;
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
    AstFunc* getCreateRandomizeNullFunc(AstClass* ধ্রুৱ classp) {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ name = "__Vrandomize_null";
        যদি (AstFunc* ধ্রুৱ existingp = VN_AS(m_memberMap.findMember(classp, name), Func)) {
            ঘূৰাই existingp;
        }
        File_সাৰি* ধ্রুৱ fl = classp->fileline();
        AstFunc* ধ্রুৱ funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ধ্রুৱ fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        addPrePostCall(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ধ্রুৱ classGenp = getRandomGenerator(classp);
        যদি (!classGenp) {
            funcp->addStmtsp(নূতন AstAssign{fl, নূতন AstVarRef{fl, fvarp, VAccess::WRITE},
                                           নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 1}});
        } অন্যথা {
            Ast_গ্ৰন্থি_Module* ধ্রুৱ genModp = VN_AS(classGenp->user2p(), গ্ৰন্থি_Module);
            funcp->addStmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ধ্রুৱ setupAllTaskp = getCreateConstraintSetupFunc(classp, m_memberMap);
            funcp->addStmtsp((নূতন AstTaskRef{fl, setupAllTaskp})->makeStmt());
            AstCExpr* ধ্রুৱ solverCallp = নূতন AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(নূতন AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->add(".next_check_only(__Vm_rng)");
            funcp->addStmtsp(
                নূতন AstAssign{fl, নূতন AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(সঁচা);
        }

        // 3. post_randomize -- only if result is non-zero
        যদি (AstTask* ধ্রুৱ userPostp = findPrePostTask(classp, "post_randomize")) {
            AstTaskRef* ধ্রুৱ callp = নূতন AstTaskRef{userPostp->fileline(), userPostp};
            funcp->addStmtsp(
                নূতন AstIf{fl, নূতন AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        ঘূৰাই funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* liftLogIfChainToConstraintIf(AstLogIf* logIfp) {
        File_সাৰি* ধ্রুৱ fl = logIfp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp = logIfp->lhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = logIfp->rhsp()->unlinkFrBack();
        Ast_গ্ৰন্থি* thenBodyp;
        যদি (AstLogIf* ধ্রুৱ innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = liftLogIfChainToConstraintIf(innerLogIfp);
            VL_DO_DANGLING(ঠেলো_Deletep(innerLogIfp), innerLogIfp);
        } অন্যথা {
            thenBodyp = নূতন Ast_Constraint_ৰাশিমালা{fl, rhsp};
        }
        ঘূৰাই নূতন AstConstraintIf{fl, condp, thenBodyp, nullptr};
    }

    স্থির তার্কিক distBoundRefsRandVar(ধ্রুৱ Ast_গ্ৰন্থি* boundp) {
        ঘূৰাই boundp->exists(
            [](ধ্রুৱ AstVarRef* vrefp) { ঘূৰাই vrefp->varp()->rand().isRandomizable(); });
    }

    স্থির তার্কিক distBoundRefsModeVar(ধ্রুৱ Ast_গ্ৰন্থি* boundp) {
        ঘূৰাই boundp->exists([](ধ্রুৱ AstVarRef* vrefp) {
            যদি (!vrefp->varp()->rand().isRandomizable()) ঘূৰাই মিছা;
            ধ্রুৱ RandomizeMode rmode = {.asUQuad = vrefp->varp()->user1()};
            ঘূৰাই rmode.usesMode;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newDistRangeMembership(AstDist* distp, ধ্রুৱ AstInsideRange* irp) {
        File_সাৰি* ধ্রুৱ fl = distp->fileline();
        ধ্রুৱ তার্কিক isSigned = distp->exprp()->isSigned();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ dist_ৰাশিমালা_Gtep = distp->exprp()->clone_বৃক্ষ_Pure(মিছা);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ dist_ৰাশিমালা_Ltep = distp->exprp()->clone_বৃক্ষ_Pure(মিছা);
        dist_ৰাশিমালা_Gtep->user1(সঁচা);
        dist_ৰাশিমালা_Ltep->user1(সঁচা);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ gep
            = isSigned ? static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                             নূতন AstGteS{fl, dist_ৰাশিমালা_Gtep, irp->lhsp()->clone_বৃক্ষ_Pure(মিছা)})
                       : static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                             নূতন AstGte{fl, dist_ৰাশিমালা_Gtep, irp->lhsp()->clone_বৃক্ষ_Pure(মিছা)});
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lep
            = isSigned ? static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                             নূতন AstLteS{fl, dist_ৰাশিমালা_Ltep, irp->rhsp()->clone_বৃক্ষ_Pure(মিছা)})
                       : static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                             নূতন AstLte{fl, dist_ৰাশিমালা_Ltep, irp->rhsp()->clone_বৃক্ষ_Pure(মিছা)});
        gep->user1(সঁচা);
        lep->user1(সঁচা);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ andp = নূতন AstLogAnd{fl, gep, lep};
        andp->user1(সঁচা);
        ঘূৰাই andp;
    }

    সংরচনা DistBucket final {
        Ast_গ্ৰন্থি_ৰাশিমালা* rangep;  // A single value or an InsideRange
        Ast_গ্ৰন্থি_ৰাশিমালা* weightExprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::vector<DistBucket> collectDistBuckets(AstDist* ধ্রুৱ distp) {
        File_সাৰি* ধ্রুৱ fl = distp->fileline();
        std::vector<DistBucket> buckets;
        বাবে (AstDistItem* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), DistItem)) {
            যদি (ধ্রুৱ AstConst* ধ্রুৱ constp = VN_CAST(ditemp->weightp(), Const)) {
                যদি (constp->toUQuad() == 0) অনুৱর্ত;
            }
            Ast_গ্ৰন্থি_ৰাশিমালা* weightExprp
                = নূতন AstExtend{fl, ditemp->weightp()->clone_বৃক্ষ_Pure(মিছা), 64};
            // := on a range weights every element, so scale by the range size.
            যদি (!ditemp->isWhole()) {
                যদি (ধ্রুৱ AstInsideRange* ধ্রুৱ irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ধ্রুৱ AstConst* ধ্রুৱ lop = VN_CAST(irp->lhsp(), Const);
                    ধ্রুৱ AstConst* ধ্রুৱ hip = VN_CAST(irp->rhsp(), Const);
                    Ast_গ্ৰন্থি_ৰাশিমালা* rangeSizep;
                    যদি (lop && hip) {
                        ধ্রুৱ uint64_t rangeSize = hip->toUQuad() - lop->toUQuad() + 1;
                        rangeSizep = নূতন AstConst{fl, AstConst::Unsized64{}, rangeSize};
                    } অন্যথা {
                        rangeSizep = নূতন AstAdd{
                            fl, নূতন AstConst{fl, AstConst::Unsized64{}, 1},
                            নূতন AstSub{fl,
                                       নূতন AstExtend{fl, irp->rhsp()->clone_বৃক্ষ_Pure(মিছা), 64},
                                       নূতন AstExtend{fl, irp->lhsp()->clone_বৃক্ষ_Pure(মিছা), 64}}};
                    }
                    weightExprp = নূতন AstMul{fl, weightExprp, rangeSizep};
                }
            }
            buckets.ঠেলো_back({ditemp->rangep(), weightExprp});
        }
        ঘূৰাই buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    স্থির Ast_গ্ৰন্থি_ৰাশিমালা* newDistMembershipTerm(AstDist* distp, Ast_গ্ৰন্থি_ৰাশিমালা* rangep) {
        যদি (ধ্রুৱ AstInsideRange* ধ্রুৱ irp = VN_CAST(rangep, InsideRange)) {
            ঘূৰাই newDistRangeMembership(distp, irp);
        }
        File_সাৰি* ধ্রুৱ fl = distp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ eqExprp = distp->exprp()->clone_বৃক্ষ_Pure(মিছা);
        eqExprp->user1(সঁচা);
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ eqp = নূতন AstEq{fl, eqExprp, rangep->clone_বৃক্ষ_Pure(মিছা)};
        eqp->user1(সঁচা);
        ঘূৰাই eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_ৰাশিমালা* buildDistMembership(AstDist* distp,
                                           ধ্রুৱ std::vector<DistBucket>& buckets) {
        File_সাৰি* ধ্রুৱ fl = distp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* unionExprp = nullptr;
        বাবে (ধ্রুৱ auto& bucket : buckets) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ memberp = newDistMembershipTerm(distp, bucket.rangep);
            যদি (!unionExprp) {
                unionExprp = memberp;
            } অন্যথা {
                unionExprp = নূতন AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(সঁচা);
            }
        }
        ঘূৰাই নূতন Ast_Constraint_ৰাশিমালা{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_গ্ৰন্থি_ৰাশিমালা* newGateJoin(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* ap, Ast_গ্ৰন্থি_ৰাশিমালা* bp, তার্কিক isAnd) {
        যদি (ap->same_বৃক্ষ(bp)) {
            VL_DO_DANGLING(ঠেলো_Deletep(bp), bp);
            ঘূৰাই ap;
        }
        যদি (VN_IS(bp, Const)) std::swap(ap, bp);
        যদি (VN_IS(ap, Const)) {
            ধ্রুৱ তার্কিক constWins = VN_AS(ap, Const)->isZero() == isAnd;
            যদি (!constWins) std::swap(ap, bp);
            VL_DO_DANGLING(ঠেলো_Deletep(bp), bp);
            ঘূৰাই ap;
        }
        যদি (isAnd) ঘূৰাই নূতন AstLogAnd{fl, ap, bp};
        ঘূৰাই নূতন AstLogOr{fl, ap, bp};
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* newGateOr(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* ap, Ast_গ্ৰন্থি_ৰাশিমালা* bp) {
        ঘূৰাই newGateJoin(fl, ap, bp, মিছা);
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* newGateAnd(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* ap, Ast_গ্ৰন্থি_ৰাশিমালা* bp) {
        ঘূৰাই newGateJoin(fl, ap, bp, সঁচা);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_গ্ৰন্থি_ৰাশিমালা* newModeBitRead(AstVar* varp, ধ্রুৱ AstMemberSel* mselp, AstVar* randModeVarp,
                                File_সাৰি* fl) {
        Ast_গ্ৰন্থি_ৰাশিমালা* arrayp = nullptr;
        যদি (varp->lifetime().isStatic()) {
            AstClass* ধ্রুৱ ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ধ্রুৱ smodep = getStaticRandModeVar(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = নূতন AstVarRef{fl, VN_AS(smodep->user2p(), গ্ৰন্থি_Module), smodep, VAccess::READ};
        } অন্যথা যদি (mselp) {
            AstVar* ধ্রুৱ memberModep = getRandModeVarFromClass(VN_AS(varp->user2p(), গ্ৰন্থি_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = নূতন AstMemberSel{fl, mselp->fromp()->clone_বৃক্ষ_Pure(মিছা), memberModep};
        } অন্যথা {
            UASSERT_OBJ(randModeVarp, varp, "rand_mode variable without a class mode array");
            arrayp = নূতন AstVarRef{fl, VN_AS(randModeVarp->user2p(), গ্ৰন্থি_Module), randModeVarp,
                                   VAccess::READ};
        }
        ধ্রুৱ RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ধ্রুৱ atp
            = নূতন AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, নূতন AstConst{fl, rmode.index}};
        atp->dtypeSetUInt32();
        ঘূৰাই atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_গ্ৰন্থি_ৰাশিমালা* newVarGate(AstVar* varp, ধ্রুৱ AstMemberSel* mselp, তার্কিক ownerLevel,
                            AstVar* randModeVarp, File_সাৰি* fl) {
        যদি (!varp->rand().isRandomizable()) {
            ঘূৰাই নূতন AstConst{fl, AstConst::BitTrue{}, ownerLevel};
        }
        ধ্রুৱ RandomizeMode rmode = {.asUQuad = varp->user1()};
        যদি (!rmode.usesMode) ঘূৰাই নূতন AstConst{fl, AstConst::BitTrue{}};
        ঘূৰাই newModeBitRead(varp, mselp, randModeVarp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_গ্ৰন্থি_ৰাশিমালা* newAccessGate(ধ্রুৱ Ast_গ্ৰন্থি* nodep, তার্কিক ownerLevel, AstVar* randModeVarp,
                               File_সাৰি* fl) {
        যদি (ধ্রুৱ AstMemberSel* ধ্রুৱ mselp = VN_CAST(nodep, MemberSel)) {
            ঘূৰাই newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, randModeVarp, fl),
                              newAccessGate(mselp->fromp(), সঁচা, randModeVarp, fl));
        }
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ refp = VN_CAST(nodep, গ্ৰন্থি_Var_Ref)) {
            ঘূৰাই newVarGate(refp->varp(), nullptr, ownerLevel, randModeVarp, fl);
        }
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Sel* ধ্রুৱ selp = VN_CAST(nodep, গ্ৰন্থি_Sel)) {
            ঘূৰাই newAccessGate(selp->fromp(), সঁচা, randModeVarp, fl);
        }
        যদি (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        ঘূৰাই নূতন AstConst{fl, AstConst::BitTrue{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_গ্ৰন্থি_ৰাশিমালা* newDistGate(ধ্রুৱ Ast_গ্ৰন্থি* nodep, AstVar* randModeVarp, File_সাৰি* fl) {
        যদি (VN_IS(nodep, গ্ৰন্থি_Var_Ref) || VN_IS(nodep, MemberSel)) {
            ঘূৰাই newAccessGate(nodep, মিছা, randModeVarp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Sel* ধ্রুৱ selp = VN_CAST(nodep, গ্ৰন্থি_Sel)) {
            ঘূৰাই newDistGate(selp->fromp(), randModeVarp, fl);
        }
        যদি (ধ্রুৱ AstSel* ধ্রুৱ selp = VN_CAST(nodep, Sel)) {
            ঘূৰাই newDistGate(selp->fromp(), randModeVarp, fl);
        }
        যদি (ধ্রুৱ AstStructSel* ধ্রুৱ selp = VN_CAST(nodep, StructSel)) {
            ঘূৰাই newDistGate(selp->fromp(), randModeVarp, fl);
        }
        যদি (ধ্রুৱ AstCMethodHard* ধ্রুৱ methodp = VN_CAST(nodep, CMethodHard)) {
            যদি (methodp->method() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                ঘূৰাই newDistGate(methodp->fromp(), randModeVarp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        যদি (ধ্রুৱ AstCond* ধ্রুৱ condp = VN_CAST(nodep, Cond)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ thenGatep = newDistGate(condp->thenp(), randModeVarp, fl);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ elseGatep = newDistGate(condp->elsep(), randModeVarp, fl);
            Ast_গ্ৰন্থি_ৰাশিমালা* armGatep = thenGatep;
            যদি (thenGatep->same_বৃক্ষ(elseGatep)) {
                VL_DO_DANGLING(ঠেলো_Deletep(elseGatep), elseGatep);
            } অন্যথা {
                armGatep
                    = নূতন AstCond{fl, condp->condp()->clone_বৃক্ষ_Pure(মিছা), thenGatep, elseGatep};
            }
            ঘূৰাই newGateOr(fl, newDistGate(condp->condp(), randModeVarp, fl), armGatep);
        }
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Uniop* ধ্রুৱ uopp = VN_CAST(nodep, গ্ৰন্থি_Uniop)) {
            ঘূৰাই newDistGate(uopp->lhsp(), randModeVarp, fl);
        }
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Biop* ধ্রুৱ bopp = VN_CAST(nodep, গ্ৰন্থি_Biop)) {
            ঘূৰাই newGateOr(fl, newDistGate(bopp->lhsp(), randModeVarp, fl),
                             newDistGate(bopp->rhsp(), randModeVarp, fl));
        }
        যদি (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        ঘূৰাই নূতন AstConst{fl, AstConst::BitTrue{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_ৰাশিমালা* buildFrozenMembership(AstDist* distp,
                                             ধ্রুৱ std::vector<DistBucket>& buckets) {
        File_সাৰি* ধ্রুৱ fl = distp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* unionExprp = nullptr;
        বাবে (ধ্রুৱ auto& bucket : buckets) {
            Ast_গ্ৰন্থি_ৰাশিমালা* termp = newDistMembershipTerm(distp, bucket.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ধ্রুৱ তার্কিক runtimeWeight
                = bucket.weightExprp->exists([](ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref*) { ঘূৰাই সঁচা; });
            যদি (runtimeWeight) {
                AstNeq* ধ্রুৱ nzp = নূতন AstNeq{fl, bucket.weightExprp->clone_বৃক্ষ_Pure(মিছা),
                                               নূতন AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(সঁচা);
                termp = নূতন AstLogAnd{fl, termp, nzp};
                termp->user1(সঁচা);
            }
            যদি (unionExprp) {
                unionExprp = নূতন AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(সঁচা);
            } অন্যথা {
                unionExprp = termp;
            }
        }
        ঘূৰাই নূতন Ast_Constraint_ৰাশিমালা{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_গ্ৰন্থি* gateFrozenDist(Ast_গ্ৰন্থি* chainp, AstDist* distp,
                            ধ্রুৱ std::vector<DistBucket>& buckets, AstVar* randModeVarp) {
        File_সাৰি* ধ্রুৱ fl = distp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* gatep = newDistGate(distp->exprp(), randModeVarp, fl);
        যদি (VN_IS(gatep, Const)) {
            ধ্রুৱ তার্কিক drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(ঠেলো_Deletep(gatep), gatep);
            যদি (drawable) ঘূৰাই chainp;
            VL_DO_DANGLING(ঠেলো_Deletep(chainp), chainp);
            ঘূৰাই buildFrozenMembership(distp, buckets);
        }
        ঘূৰাই নূতন AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_গ্ৰন্থি_ৰাশিমালা* newUniformRangePick(AstDist* distp, ধ্রুৱ AstInsideRange* irp) {
        File_সাৰি* ধ্রুৱ fl = distp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ dist_ৰাশিমালা_Copyp = distp->exprp()->clone_বৃক্ষ_Pure(মিছা);
        dist_ৰাশিমালা_Copyp->user1(সঁচা);
        ধ্রুৱ পূর্ণাঙ্ক distWidth = distp->exprp()->width();
        ধ্রুৱ AstConst* ধ্রুৱ lopC = VN_CAST(irp->lhsp(), Const);
        ধ্রুৱ AstConst* ধ্রুৱ hipC = VN_CAST(irp->rhsp(), Const);
        Ast_গ্ৰন্থি_ৰাশিমালা* rangeSzp;
        যদি (lopC && hipC) {
            ধ্রুৱ uint64_t rsz = hipC->toUQuad() - lopC->toUQuad() + 1;
            rangeSzp = নূতন AstConst{fl, AstConst::Unsized64{}, rsz};
        } অন্যথা {
            ধ্রুৱ তার্কিক isSigned = irp->lhsp()->isSigned();
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lo64p
                = isSigned ? static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                                 নূতন AstExtendS{fl, irp->lhsp()->clone_বৃক্ষ_Pure(মিছা), 64})
                           : static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                                 নূতন AstExtend{fl, irp->lhsp()->clone_বৃক্ষ_Pure(মিছা), 64});
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ hi64p
                = isSigned ? static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                                 নূতন AstExtendS{fl, irp->rhsp()->clone_বৃক্ষ_Pure(মিছা), 64})
                           : static_cast<Ast_গ্ৰন্থি_ৰাশিমালা*>(
                                 নূতন AstExtend{fl, irp->rhsp()->clone_বৃক্ষ_Pure(মিছা), 64});
            rangeSzp = নূতন AstAdd{fl, নূতন AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  নূতন AstSub{fl, hi64p, lo64p}};
        }
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rand64p = নূতন AstRand{fl, nullptr, মিছা};
        rand64p->dtypeSetUInt64();
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ offsetp
            = নূতন AstCCast{fl, নূতন AstModDiv{fl, rand64p, rangeSzp}, distWidth};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ valuep = নূতন AstAdd{fl, irp->lhsp()->clone_বৃক্ষ_Pure(মিছা), offsetp};
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ eqp = নূতন AstEq{fl, dist_ৰাশিমালা_Copyp, valuep};
        eqp->user1(সঁচা);
        ঘূৰাই eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_গ্ৰন্থি* buildWeightedBucketChain(AstDist* distp, ধ্রুৱ std::vector<DistBucket>& buckets,
                                      AstVar* bucketVarp,
                                      ধ্রুৱ std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*>& cumSums) {
        File_সাৰি* ধ্রুৱ fl = distp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* chainp = nullptr;
        বাবে (পূর্ণাঙ্ক i = static_cast<পূর্ণাঙ্ক>(buckets.size()) - 1; i >= 0; --i) {
            Ast_গ্ৰন্থি_ৰাশিমালা* constraintExprp;
            ধ্রুৱ AstInsideRange* ধ্রুৱ irp = VN_CAST(buckets[i].rangep, InsideRange);
            যদি (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } অন্যথা যদি (irp) {
                constraintExprp = newUniformRangePick(distp, irp);
            } অন্যথা {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ dist_ৰাশিমালা_Copyp = distp->exprp()->clone_বৃক্ষ_Pure(মিছা);
                dist_ৰাশিমালা_Copyp->user1(সঁচা);
                constraintExprp
                    = নূতন AstEq{fl, dist_ৰাশিমালা_Copyp, buckets[i].rangep->clone_বৃক্ষ_Pure(মিছা)};
                constraintExprp->user1(সঁচা);
            }
            যদি (!chainp) {
                chainp = constraintExprp;
            } অন্যথা {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp
                    = নূতন AstLte{fl, নূতন AstVarRef{fl, bucketVarp, VAccess::READ}, cumSums[i]};
                condp->user1(সঁচা);
                chainp = নূতন AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(সঁচা);
            }
        }
        Ast_Constraint_ৰাশিমালা* ধ্রুৱ chainExprp = নূতন Ast_Constraint_ৰাশিমালা{fl, chainp};
        chainExprp->isSoft(সঁচা);
        ঘূৰাই chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    শূন্য lowerDistConstraints(AstTask* taskp, Ast_গ্ৰন্থি* constrItemsp, AstVar* randModeVarp,
                              AstConstraintForeach* foreachp = nullptr) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_গ্ৰন্থি* foreachTailp = nullptr;
        auto addStmt = [&](Ast_গ্ৰন্থি* nodep) {
            যদি (foreachp) {
                যদি (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } অন্যথা {
                    foreachTailp->addNext(nodep);
                    foreachTailp = nodep;
                }
            } অন্যথা {
                taskp->addStmtsp(nodep);
            }
        };

        বাবে (Ast_গ্ৰন্থি *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            যদি (AstConstraintIf* ধ্রুৱ cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                lowerDistConstraints(taskp, cifp->thensp(), randModeVarp, foreachp);
                যদি (cifp->elsesp()) {
                    lowerDistConstraints(taskp, cifp->elsesp(), randModeVarp, foreachp);
                }
                অনুৱর্ত;
            }
            যদি (AstConstraintForeach* ধ্রুৱ cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                lowerDistConstraints(taskp, cfep->bodyp(), randModeVarp, cfep);
                অনুৱর্ত;
            }

            Ast_Constraint_ৰাশিমালা* ধ্রুৱ constrExprp = VN_CAST(itemp, Constraint_ৰাশিমালা);
            যদি (!constrExprp) অনুৱর্ত;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            যদি (AstLogIf* ধ্রুৱ শীৰ্ষ_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_গ্ৰন্থি* chainEndp = শীৰ্ষ_Log_Ifp->rhsp();
                যেতিয়ালৈ (AstLogIf* ধ্রুৱ innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                যদি (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ধ্রুৱ liftedp = liftLogIfChainToConstraintIf(শীৰ্ষ_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(ঠেলো_Deletep(constrExprp), constrExprp);
                    lowerDistConstraints(taskp, liftedp->thensp(), randModeVarp, foreachp);
                    অনুৱর্ত;
                }
            }

            AstDist* ধ্রুৱ distp = VN_CAST(constrExprp->exprp(), Dist);
            যদি (!distp) অনুৱর্ত;

            File_সাৰি* ধ্রুৱ fl = distp->fileline();

            std::vector<DistBucket> buckets = collectDistBuckets(distp);

            যদি (buckets.empty()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_ৰাশিমালা* ধ্রুৱ truep
                    = নূতন Ast_Constraint_ৰাশিমালা{fl, নূতন AstConst{fl, AstConst::BitTrue{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(ঠেলো_Deletep(constrExprp), constrExprp);
                অনুৱর্ত;
            }

            Ast_Constraint_ৰাশিমালা* ধ্রুৱ membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_গ্ৰন্থি_ৰাশিমালা* totalWeightExprp = nullptr;
            বাবে (auto& bucket : buckets) {
                যদি (!totalWeightExprp) {
                    totalWeightExprp = bucket.weightExprp->clone_বৃক্ষ_Pure(মিছা);
                } অন্যথা {
                    totalWeightExprp = নূতন AstAdd{fl, totalWeightExprp,
                                                  bucket.weightExprp->clone_বৃক্ষ_Pure(মিছা)};
                    totalWeightExprp->dtypeSetUInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ধ্রুৱ পূর্ণাঙ্ক distId = m_dist_অঙ্ক++;
            ধ্রুৱ std::string totalName = "__Vdist_total" + cvtToStr(distId);
            AstVar* ধ্রুৱ totalVarp
                = নূতন AstVar{fl, VVarType::BLOCKTEMP, totalName, taskp->findUInt64DType()};
            totalVarp->noSubst(সঁচা);
            totalVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            totalVarp->funcLocal(সঁচা);
            totalVarp->isInternal(সঁচা);
            addStmt(totalVarp);
            addStmt(
                নূতন AstAssign{fl, নূতন AstVarRef{fl, totalVarp, VAccess::WRITE}, totalWeightExprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ধ্রুৱ std::string bucketName = "__Vdist_bucket" + cvtToStr(distId);
            AstVar* ধ্রুৱ bucketVarp
                = নূতন AstVar{fl, VVarType::BLOCKTEMP, bucketName, taskp->findUInt64DType()};
            bucketVarp->noSubst(সঁচা);
            bucketVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            bucketVarp->funcLocal(সঁচা);
            bucketVarp->isInternal(সঁচা);
            addStmt(bucketVarp);

            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ randp = নূতন AstRand{fl, nullptr, মিছা};
            randp->dtypeSetUInt64();
            addStmt(নূতন AstAssign{
                fl, নূতন AstVarRef{fl, bucketVarp, VAccess::WRITE},
                নূতন AstAdd{
                    fl, নূতন AstConst{fl, AstConst::Unsized64{}, 1},
                    নূতন AstModDiv{fl, randp, নূতন AstVarRef{fl, totalVarp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::vector<Ast_গ্ৰন্থি_ৰাশিমালা*> cumSums;
            Ast_গ্ৰন্থি_ৰাশিমালা* runningSump = nullptr;
            বাবে (size_t i = 0; i < buckets.size(); ++i) {
                যদি (!runningSump) {
                    runningSump = buckets[i].weightExprp->clone_বৃক্ষ_Pure(মিছা);
                } অন্যথা {
                    runningSump = নূতন AstAdd{fl, runningSump,
                                             buckets[i].weightExprp->clone_বৃক্ষ_Pure(মিছা)};
                    runningSump->dtypeSetUInt64();
                }
                cumSums.ঠেলো_back(runningSump->clone_বৃক্ষ_Pure(সঁচা));
            }

            Ast_গ্ৰন্থি* chainp = buildWeightedBucketChain(distp, buckets, bucketVarp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, randModeVarp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(ঠেলো_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->addHereThisAsNext(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            বাবে (auto& bucket : buckets) {
                VL_DO_DANGLING(ঠেলো_Deletep(bucket.weightExprp), bucket.weightExprp);
            }
            VL_DO_DANGLING(ঠেলো_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            ঠেলো_Deletep(cumSums.back());
        }
    }

    স্থির তার্কিক isDynArrOfClassTypeRecurse(ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep) {
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ refp = dtypep->skipRefp();
        যদি (VN_IS(refp, DynArrayDType) || VN_IS(refp, QueueDType)) {
            ঘূৰাই isDynArrOfClassTypeRecurse(refp->subDTypep());
        } অন্যথা যদি (VN_IS(refp, ClassRefDType)) {
            ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    শূন্য addArrayElemRefresh(File_সাৰি* ধ্রুৱ fl, AstVar* ধ্রুৱ arrVarp, AstFunc* ধ্রুৱ randomizep,
                             AstVar* ধ্রুৱ genp) {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ genModp = VN_AS(genp->user2p(), গ্ৰন্থি_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ধ্রুৱ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(মিছা).second;
        যদি (isDynArrOfClassTypeRecurse(arrVarp->dtypep())) {
            যদি (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            ঘূৰাই;
        }
        AstCMethodHard* ধ্রুৱ methodp
            = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();

        Ast_গ্ৰন্থি_Module* ধ্রুৱ classp = VN_AS(arrVarp->user2p(), গ্ৰন্থি_Module);
        AstVarRef* ধ্রুৱ varRefp = নূতন AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->addPinsp(varRefp);

        ধ্রুৱ size_t width = arrayElementDTypep(arrVarp->dtypep())->width();

        methodp->addPinsp(নূতন AstConst{fl, AstConst::Unsized64{}, width});
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ varnamep
            = নূতন AstCExpr{fl, AstCExpr::Pure{}, "\"" + arrVarp->name() + "\"", arrVarp->width()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(নূতন AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ধ্রুৱ RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        যদি (rmode.usesMode) {
            methodp->addPinsp(নূতন AstConst{fl, AstConst::Unsized64{}, rmode.index});
        }

        randomizep->addStmtsp(methodp->makeStmt());
    }

    Ast_গ্ৰন্থি_Stmt* newPinSizeVariable(File_সাৰি* ধ্রুৱ fl, AstVar* ধ্রুৱ arrVarp,
                                    AstVar* ধ্রুৱ genp) {
        Ast_গ্ৰন্থি_Module* ধ্রুৱ genModp = VN_AS(genp->user2p(), গ্ৰন্থি_Module);
        AstVar* ধ্রুৱ sizeVarp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ধ্রুৱ pinp
            = নূতন AstCMethodHard{fl, নূতন AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtypeSetVoid();
        AstCExpr* ধ্রুৱ namep = নূতন AstCExpr{fl, AstCExpr::Pure{}, "\"" + sizeVarp->name() + "\""};
        namep->dtypeSetUInt32();
        pinp->addPinsp(namep);
        pinp->addPinsp(
            নূতন AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(sizeVarp->width())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ধ্রুৱ sizeVarRefp = নূতন AstVarRef{fl, sizeVarp, VAccess::READ};
        sizeVarRefp->classOrPackagep(VN_AS(sizeVarp->user2p(), গ্ৰন্থি_Module));
        pinp->addPinsp(sizeVarRefp);
        ঘূৰাই pinp->makeStmt();
    }

    শূন্য addFrozenSizePin(File_সাৰি* ধ্রুৱ fl, AstVar* ধ্রুৱ arrVarp, AstFunc* ধ্রুৱ randomizep,
                          AstVar* ধ্রুৱ genp, AstVar* ধ্রুৱ randModeVarp) {
        ধ্রুৱ RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        যদি (!rmode.usesMode) ঘূৰাই;
        AstVar* ধ্রুৱ sizeVarp = VN_AS(arrVarp->user4p(), Var);
        Ast_গ্ৰন্থি_Module* ধ্রুৱ arrClassp = VN_AS(arrVarp->user2p(), গ্ৰন্থি_Module);
        Ast_গ্ৰন্থি_Module* ধ্রুৱ sizeClassp = VN_AS(sizeVarp->user2p(), গ্ৰন্থি_Module);
        AstVarRef* ধ্রুৱ sizeWritep = নূতন AstVarRef{fl, sizeClassp, sizeVarp, VAccess::WRITE};
        ধ্রুৱ VCMethod sizeMethod = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_SIZE
                                        : VCMethod::DYN_SIZE;
        AstCMethodHard* ধ্রুৱ currentSizep = নূতন AstCMethodHard{
            fl, নূতন AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, sizeMethod};
        currentSizep->dtypep(sizeVarp->dtypep());
        randomizep->addStmtsp(নূতন AstAssign{fl, sizeWritep, currentSizep});
        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ modep = newModeBitRead(arrVarp, nullptr, randModeVarp, fl);
        randomizep->addStmtsp(
            নূতন AstIf{fl, নূতন AstLogNot{fl, modep}, newPinSizeVariable(fl, arrVarp, genp)});
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_অঙ্ক);
        VL_RESTORER_COPY(m_randArrayUniqueNames);
        m_modp = nodep;
        m_rand_Case_অঙ্ক = 0;
        m_randArrayUniqueNames.reset();
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_অঙ্ক);
        VL_RESTORER(m_dist_অঙ্ক);
        m_modp = nodep;
        m_rand_Case_অঙ্ক = 0;
        m_dist_অঙ্ক = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        যদি (!nodep->user1()) ঘূৰাই;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->baseMostClassp()->needRNG(সঁচা);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ধ্রুৱ তার্কিক basicFirst = classOwnsGlobalConstraint(nodep);

        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        AstFunc* ধ্রুৱ randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ধ্রুৱ fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ধ্রুৱ randModeVarp = getRandModeVarFromClass(nodep);
        addPrePostCall(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        যদি (classHasRandClassMembers(nodep)) {
            AstTask* ধ্রুৱ preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->addStmtsp((নূতন AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_গ্ৰন্থি_ৰাশিমালা* beginValp = nullptr;
        AstVar* genp = getRandomGenerator(nodep);
        যদি (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ধ্রুৱ prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ধ্রুৱ classp, AstConstraint* ধ্রুৱ constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                যদি (!taskp) {
                    taskp = newSetupConstraintTask(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                যদি (AstTask* ধ্রুৱ resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ধ্রুৱ resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ধ্রুৱ resizeTaskRefp
                        = নূতন AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->addStmtsp(resizeTaskRefp->makeStmt());
                }

                যদি (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                যদি (constrp->itemsp()) {
                    lowerDistConstraints(taskp, constrp->itemsp(), randModeVarp);
                }
                VInsertionSet<AstVar*>& sizeArrays = m_sizeConstrainedArrays[classp];
                Constraint_ৰাশিমালা_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), nullptr,
                    genp,          randModeVarp, m_writtenVars,     &m_uniqueConstraintId,
                    m_uniqueNames, randomizep,   &sizeArrays,       prepareArraysTaskp};
                যদি (constrp->itemsp()) {
                    taskp->addStmtsp(wrapIfConstraintMode(
                        nodep, constrp, constrp->itemsp()->unlinkFrBackWithNext()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ধ্রুৱ auto constraintDepth = [](ধ্রুৱ AstConstraint* constrp) {
                    পূর্ণাঙ্ক depth = 0;
                    size_t pos = 0;
                    যেতিয়ালৈ ((pos = constrp->name().find(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++depth;
                        pos += std::strlen(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    ঘূৰাই depth;
                };
                পূর্ণাঙ্ক maxDepth = 0;
                nodep->foreachMember([&](AstClass* ধ্রুৱ, AstConstraint* ধ্রুৱ constrp) {
                    maxDepth = std::max(maxDepth, constraintDepth(constrp));
                });
                AstTask* ধ্রুৱ setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
                বাবে (পূর্ণাঙ্ক d = maxDepth; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ধ্রুৱ classp, AstConstraint* ধ্রুৱ constrp) {
                            যদি (constraintDepth(constrp) != d) ঘূৰাই;
                            AstTask* ধ্রুৱ taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ধ্রুৱ setupTaskRefp
                                = নূতন AstTaskRef{constrp->fileline(), taskp};
                            setupTaskRefp->classOrPackagep(classp);
                            setupAllTaskp->addStmtsp(setupTaskRefp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertionSet<AstVar*> sizeArrayVars;
            যদি (nodep->extendsp()) {
                AstClass* parentClassp = nodep->extendsp()->classp();
                যেতিয়ালৈ (parentClassp) {
                    ধ্রুৱ auto sizeArraysIt = m_sizeConstrainedArrays.find(parentClassp);
                    যদি (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                        বাবে (AstVar* ধ্রুৱ arrVarp : sizeArraysIt->second) {
                            sizeArrayVars.ঢোকাও(arrVarp);
                        }
                    }
                    AstFunc* ধ্রুৱ parentRandomizep
                        = VN_CAST(m_memberMap.findMember(parentClassp, "randomize"), Func);
                    যদি (parentRandomizep && parentRandomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        বাবে (Ast_গ্ৰন্থি* stmtp = parentRandomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            তার্কিক foundClearConstraints = মিছা;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                যদি (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->addStmtsp(stmtp->clone_বৃক্ষ(মিছা));
                                } অন্যথা যদি (methodp->method()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    foundClearConstraints = সঁচা;
                                }
                            });
                            যদি (foundClearConstraints) ভঙ্গ;
                        }
                    }
                    parentClassp
                        = parentClassp->extendsp() ? parentClassp->extendsp()->classp() : nullptr;
                }
            }
            randomizep->addStmtsp(implementConstraintsClear(fl, genp));

            ধ্রুৱ auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
            যদি (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                বাবে (AstVar* ধ্রুৱ arrVarp : sizeArraysIt->second) sizeArrayVars.ঢোকাও(arrVarp);
            }
            AstTask* ধ্রুৱ setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
            যদি (!sizeArrayVars.empty() && prepareArraysTaskp->stmtsp()) {
                Ast_গ্ৰন্থি_Stmt* ধ্রুৱ preparep = (নূতন AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                বাবে (Ast_গ্ৰন্থি* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    তার্কিক writesArray = মিছা;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    যদি (!writesArray) অনুৱর্ত;
                    stmtp->addHereThisAsNext(preparep);
                    ভঙ্গ;
                }
            }
            AstTaskRef* ধ্রুৱ setupTaskRefp = নূতন AstTaskRef{fl, setupAllTaskp};
            randomizep->addStmtsp(setupTaskRefp->makeStmt());

            Ast_গ্ৰন্থি_Module* ধ্রুৱ genModp = VN_AS(genp->user2p(), গ্ৰন্থি_Module);

            AstCExpr* ধ্রুৱ solverCallp = নূতন AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(নূতন AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->add(".next(__Vm_rng)");
            যদি (!sizeArrayVars.empty()) {
                AstVar* ধ্রুৱ sizeOkVarp = নূতন AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                sizeOkVarp->funcLocal(সঁচা);
                randomizep->addStmtsp(sizeOkVarp);

                AstVar* ধ্রুৱ finalOkVarp = নূতন AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                finalOkVarp->funcLocal(সঁচা);
                randomizep->addStmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                বাবে (AstVar* ধ্রুৱ arrVarp : sizeArrayVars)
                    addFrozenSizePin(fl, arrVarp, randomizep, genp, randModeVarp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->addStmtsp(
                    নূতন AstAssign{fl, নূতন AstVarRef{fl, sizeOkVarp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                যদি (AstTask* ধ্রুৱ resizeAllTaskp
                    = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ধ্রুৱ resizeTaskRefp = নূতন AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ধ্রুৱ ifp = নূতন AstIf{fl, নূতন AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), nullptr};
                    randomizep->addStmtsp(ifp);
                }

                // Refresh array element tables after resize
                বাবে (ধ্রুৱ auto& arrVarp : sizeArrayVars) {
                    addArrayElemRefresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->addStmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ধ্রুৱ setupTaskRefp2 = নূতন AstTaskRef{fl, setupAllTaskp};
                randomizep->addStmtsp(setupTaskRefp2->makeStmt());

                বাবে (AstVar* ধ্রুৱ arrVarp : sizeArrayVars) {
                    randomizep->addStmtsp(newPinSizeVariable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ধ্রুৱ solverCallp2 = নূতন AstCExpr{fl};
                solverCallp2->dtypeSetBit();
                solverCallp2->add(নূতন AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->add(".next(__Vm_rng)");
                randomizep->addStmtsp(নূতন AstAssign{
                    fl, নূতন AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = নূতন AstAnd{fl, নূতন AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                       নূতন AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } অন্যথা {
                beginValp = solverCallp;
            }
            যদি (randModeVarp) {
                Ast_গ্ৰন্থি_Module* ধ্রুৱ randModeClassp = VN_AS(randModeVarp->user2p(), Class);
                Ast_গ্ৰন্থি_FTask* ধ্রুৱ newp
                    = VN_AS(m_memberMap.findMember(randModeClassp, "new"), গ্ৰন্থি_FTask);
                UASSERT_OBJ(newp, randModeClassp, "No new() in class");
                addSetRandMode(newp, genp, randModeVarp);
            }
            যদি (AstVar* ধ্রুৱ staticRandModeVarp = getStaticRandModeVar(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_গ্ৰন্থি_FTask* ধ্রুৱ newp = VN_AS(m_memberMap.findMember(nodep, "new"), গ্ৰন্থি_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                addSetStaticRandMode(newp, genp, staticRandModeVarp);
            }
        } অন্যথা {
            beginValp = নূতন AstConst{fl, AstConst::Widthed_মান{}, 32, 1};
        }

        addUpdateRandVarsBody(nodep, randomizep);

        AstFunc* ধ্রুৱ basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        addBasicRandomizeBody(basicRandomizep, nodep, randModeVarp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ধ্রুৱ fvarRefp = নূতন AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->addStmtsp(নূতন AstAssign{
            fl, fvarRefp, basicFirst ? নূতন AstFuncRef{fl, basicRandomizep} : beginValp});

        ধ্রুৱ auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
        ধ্রুৱ তার্কিক needsSizePhase
            = sizeArraysIt != m_sizeConstrainedArrays.end() && !sizeArraysIt->second.empty();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ধ্রুৱ auto emitResizeFallback = [&]() {
            যদি (needsSizePhase) ঘূৰাই;
            যদি (AstTask* ধ্রুৱ resizeAllTaskp
                = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ধ্রুৱ resizeTaskRefp = নূতন AstTaskRef{fl, resizeAllTaskp};
                randomizep->addStmtsp(resizeTaskRefp->makeStmt());
            }
        };
        যদি (!basicFirst) emitResizeFallback();

        AstVarRef* ধ্রুৱ fvarRefReadp = fvarRefp->clone_বৃক্ষ(মিছা);
        fvarRefReadp->access(VAccess::READ);

        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ secondHalfp
            = basicFirst ? beginValp : নূতন AstFuncRef{fl, basicRandomizep};
        randomizep->addStmtsp(নূতন AstAssign{fl, fvarRefp->clone_বৃক্ষ(মিছা),
                                            নূতন AstAnd{fl, fvarRefReadp, secondHalfp}});

        যদি (basicFirst) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        যদি (classHasRandClassMembers(nodep)) {
            AstTask* ধ্রুৱ postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->addStmtsp((নূতন AstTaskRef{fl, postTaskp})->makeStmt());
        }

        addPrePostCall(nodep, randomizep, "post_randomize");
        nodep->user1(মিছা);
    }
    শূন্য visit(AstRandCase* nodep) override {
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
        Ast_গ্ৰন্থি_DType* ধ্রুৱ sumDTypep = nodep->findUInt64DType();

        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        ধ্রুৱ std::string name = "__Vrandcase" + cvtToStr(m_rand_Case_অঙ্ক++);
        AstVar* ধ্রুৱ randVarp = নূতন AstVar{fl, VVarType::BLOCKTEMP, name, sumDTypep};
        randVarp->noSubst(সঁচা);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        addTempVarsp(randVarp);
        Ast_গ্ৰন্থি_ৰাশিমালা* sump = নূতন AstConst{fl, AstConst::Widthed_মান{}, 64, 0};
        Ast_গ্ৰন্থি_If* ধ্রুৱ firstIfsp
            = নূতন AstIf{fl, নূতন AstConst{fl, AstConst::BitFalse{}}, nullptr, nullptr};
        Ast_গ্ৰন্থি_If* ifsp = firstIfsp;

        বাবে (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ condp = itemp->condsp()->unlinkFrBack();
            sump
                = নূতন AstAdd{condp->fileline(), sump, নূতন AstExtend{itemp->fileline(), condp, 64}};
            Ast_গ্ৰন্থি* ধ্রুৱ stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlinkFrBackWithNext() : nullptr;
            AstVarRef* ধ্রুৱ randVarRefp = নূতন AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_গ্ৰন্থি_If* ধ্রুৱ newifp
                = নূতন AstIf{itemp->fileline(),
                            নূতন AstLte{condp->fileline(), randVarRefp, sump->clone_বৃক্ষ_Pure(সঁচা)},
                            stmtsp, nullptr};
            ifsp->addElsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ধ্রুৱ dispp = নূতন AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            nullptr, nullptr};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->addElsesp(dispp);

        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ randp = নূতন AstRand{fl, nullptr, মিছা};
        randp->dtypeSetUInt64();
        AstVarRef* ধ্রুৱ randVarRefp = নূতন AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_গ্ৰন্থি* ধ্রুৱ newp
            = নূতন AstAssign{fl, randVarRefp,
                            নূতন AstAdd{fl, নূতন AstConst{fl, AstConst::Unsized64{}, 1},
                                       নূতন AstModDiv{fl, randp, sump}}};
        newp->addNext(firstIfsp);
        যদি (debug() >= 9) newp->dump_বৃক্ষ_And_Next(cout, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_FTask_Ref* nodep) override {
        যদি (nodep->name() == "rand_mode") {
            AstMethodCall* ধ্রুৱ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = methodCallp ? methodCallp->fromp() : nullptr;
            ধ্রুৱ RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_modp);
            UASSERT_OBJ(randModeTarget.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ধ্রুৱ receiverp = randModeTarget.receiverp;
            ধ্রুৱ তার্কিক isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_গ্ৰন্থি* classLevelStaticLoopp = nullptr;
            যদি (isClassLevel && nodep->argsp()) {
                যদি (AstVar* ধ্রুৱ sVarp = getStaticRandModeVar(randModeTarget.classp)) {
                    File_সাৰি* ধ্রুৱ fl = nodep->fileline();
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ staticLhsp
                        = makeModeAssignLhs(fl, randModeTarget.classp, nullptr, sVarp);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ argClonep = nodep->argsp()->exprp()->clone_বৃক্ষ_Pure(মিছা);
                    classLevelStaticLoopp = makeModeSetLoop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ধ্রুৱ তার্কিক receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ধ্রুৱ randModeVarp = receiverIsStaticRand
                                             ? getStaticRandModeVar(randModeTarget.classp)
                                             : getRandModeVarFromClass(randModeTarget.classp);
            যদি (!randModeVarp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_ৰাশিমালা), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(ঠেলো_Deletep(m_stmtp), m_stmtp = nullptr);
                ঘূৰাই;
            }
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = makeModeAssignLhs(nodep->fileline(), randModeTarget.classp,
                                                        randModeTarget.fromp, randModeVarp);
            replaceWithModeAssignAndAppend(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : nullptr, lhsp,
                classLevelStaticLoopp);
            ঘূৰাই;
        }

        যদি (nodep->name() == "constraint_mode") {
            AstMethodCall* ধ্রুৱ methodCallp = VN_CAST(nodep, MethodCall);
            Ast_গ্ৰন্থি_ৰাশিমালা* fromp = methodCallp ? methodCallp->fromp() : nullptr;
            AstConstraint* constrp = nullptr;
            AstClass* classp = VN_CAST(m_modp, Class);
            যদি (AstConstraintRef* ধ্রুৱ constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                যদি (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } অন্যথা যদি (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ধ্রুৱ constraintModeVarp = (constrp && constrp->isStatic())
                                                   ? getStaticConstraintModeVar(classp)
                                                   : getConstraintModeVar(classp);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp
                = makeModeAssignLhs(nodep->fileline(), classp, fromp, constraintModeVarp);
            replaceWithModeAssign(nodep, constrp, lhsp);
            ঘূৰাই;
        }

        যদি (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            ঘূৰাই;
        }

        যদি (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ধ্রুৱ তার্কিক inStaticContext = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ধ্রুৱ stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ধ্রুৱ randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            যদি (inStaticContext) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(সঁচা);
            }
            randomizeFuncp->addStmtsp(
                নূতন AstAssign{nodep->fileline(),
                              নূতন AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              নূতন AstConst{nodep->fileline(), AstConst::Widthed_মান{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            পূর্ণাঙ্ক argn = 0;
            AstWith* ধ্রুৱ withp = nodep->withp();
            বাবে (ধ্রুৱ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ exprp = argp->exprp();
                AstCMethodHard* ধ্রুৱ basicMethodp = নূতন AstCMethodHard{
                    nodep->fileline(),
                    নূতন AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ধ্রুৱ refvarp
                    = নূতন AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::to_string(++argn), exprp->dtypep()};
                refvarp->setStdRandomizeArg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                যদি (withp) {
                    withp->foreach([&](Ast_গ্ৰন্থি_ৰাশিমালা* exp) {
                        যদি (is_Similar_গ্ৰন্থি(exp, exprp)) {
                            AstVarRef* ধ্রুৱ replaceVar
                                = নূতন AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            exp->replaceWith(replaceVar);
                            replaceVar->user1(exp->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(ঠেলো_Deletep(exp), exp);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(সঁচা);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->addStmtsp(refvarp);

                ধ্রুৱ size_t width = exprp->width();
                basicMethodp->addPinsp(
                    নূতন AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->addPinsp(
                    নূতন AstConst{nodep->fileline(), AstConst::Unsized64{}, width});
                basicMethodp->dtypeSetBit();

                randomizeFuncp->addStmtsp(নূতন AstAssign{
                    nodep->fileline(),
                    নূতন AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    নূতন AstAnd{nodep->fileline(),
                               নূতন AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            যদি (withp) {
                File_সাৰি* ধ্রুৱ fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, nullptr,
                                                                inStaticContext);
                withCapturep->addFunctionArguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                যদি (stdrand) {
                    randomizeFuncp->addStmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_গ্ৰন্থি* ধ্রুৱ capturedTreep = withp->exprp()->unlinkFrBackWithNext();
                randomizeFuncp->addStmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_ৰাশিমালা_Visitor{
                        nullptr, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        nullptr, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
                }
                AstCExpr* ধ্রুৱ solverCallp = নূতন AstCExpr{fl};
                solverCallp->dtypeSetBit();
                solverCallp->add(নূতন AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->add(".next()");
                AstVar* ধ্রুৱ fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ andExprp
                    = নূতন AstAnd{fl, নূতন AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->addStmtsp(
                    নূতন AstAssign{fl, নূতন AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlinkFrBack()->delete_বৃক্ষ(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            যদি (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            যদি (withCapturep) nodep->addArgsp(withCapturep->getArgs());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            ঘূৰাই;
        }
        handleRandomizeArgs(nodep);

        AstWith* ধ্রুৱ withp = nodep->withp();
        যদি (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            ঘূৰাই;
        }
        withp->unlinkFrBack();
        iterateChildren(nodep);

        AstClass* classp = nullptr;
        যদি (AstMethodCall* ধ্রুৱ callp = VN_CAST(nodep, MethodCall)) {
            ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            ধ্রুৱ AstClassRefDType* ধ্রুৱ classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } অন্যথা {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        যদি (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ধ্রুৱ classGenp = getRandomGenerator(classp);
        AstVar* ধ্রুৱ localGenp
            = নূতন AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(সঁচা);

        AstFunc* ধ্রুৱ randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inlineUniqueNames.get(nodep), মিছা);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ধ্রুৱ std::pair<তার্কিক, তার্কিক> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<তার্কিক, তার্কিক>{মিছা, মিছা};
        যদি (prePostWrap.first) {
            addVirtualPrePostCall(randomizeFuncp, classp, "pre_randomize");
        } অন্যথা {
            addPrePostCall(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        যদি (classHasRandClassMembers(classp)) {
            AstTask* ধ্রুৱ preTaskp = getCreateNestedCallbackTask(classp, "pre");
            যদি (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->addStmtsp((নূতন AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ধ্রুৱ CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.addFunctionArguments(randomizeFuncp);

        // Add constraints clearing code
        যদি (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ধ্রুৱ mainRandomizep
                = VN_CAST(m_memberMap.findMember(classp, "randomize"), Func);
            যদি (mainRandomizep && mainRandomizep->stmtsp()) {
                বাবে (Ast_গ্ৰন্থি* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    তার্কিক foundClearConstraints = মিছা;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        যদি (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->addStmtsp(stmtp->clone_বৃক্ষ(মিছা));
                        } অন্যথা যদি (methodp->method() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            foundClearConstraints = সঁচা;
                        }
                    });
                    যদি (foundClearConstraints) ভঙ্গ;
                }
            }
            randomizeFuncp->addStmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->addStmtsp(localGenp);

        AstFunc* ধ্রুৱ basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ধ্রুৱ basicRandomizeFuncCallp
            = নূতন AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        যদি (classGenp) {
            AstTask* ধ্রুৱ constrSetupFuncp = getCreateConstraintSetupFunc(classp, m_memberMap);
            AstTaskRef* ধ্রুৱ callp = নূতন AstTaskRef{nodep->fileline(), constrSetupFuncp};
            randomizeFuncp->addStmtsp(callp->makeStmt());
            randomizeFuncp->addStmtsp(নূতন AstAssign{
                nodep->fileline(), নূতন AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                নূতন AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), গ্ৰন্থি_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ধ্রুৱ randModeVarp = getRandModeVarFromClass(classp);
        যদি (!classGenp && randModeVarp) addSetRandMode(randomizeFuncp, localGenp, randModeVarp);
        যদি (!classGenp) {
            যদি (AstVar* ধ্রুৱ sVarp = getStaticRandModeVar(classp)) {
                addSetStaticRandMode(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_গ্ৰন্থি* ধ্রুৱ capturedTreep = withp->exprp()->unlinkFrBackWithNext();
        randomizeFuncp->addStmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_গ্ৰন্থি* inlineResizeStmtsp = nullptr;
        {
            std::vector<AstCMethodHard*> sizeMethodps;
            capturedTreep->foreachAndNext([&](AstCMethodHard* methodp) {
                যদি (methodp->method() == VCMethod::DYN_SIZE
                    || methodp->method() == VCMethod::ASSOC_SIZE) {
                    sizeMethodps.ঠেলো_back(methodp);
                }
            });
            বাবে (AstCMethodHard* ধ্রুৱ methodp : sizeMethodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = nullptr;
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ মূল_Exprp = methodp->fromp();
                যদি (AstMemberSel* memberSelp = VN_CAST(মূল_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    যেতিয়ালৈ (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ fromp = memberSelp->fromp();
                    যদি (ধ্রুৱ AstVarRef* ধ্রুৱ varRefp = VN_CAST(fromp, VarRef)) {
                        যদি (varRefp->varp() == captured.getThisp()) অনুৱর্ত;
                    } অন্যথা {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } অন্যথা যদি (AstVarRef* ধ্রুৱ varRefp = VN_CAST(মূল_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                যদি (!arrVarp) অনুৱর্ত;
                // Only handle rand-declared dynamic/assoc array variables
                যদি (!arrVarp->rand().isRandomizable()) অনুৱর্ত;
                // Do not randomize associative array size (IEEE 18.4)
                যদি (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) অনুৱর্ত;
                File_সাৰি* ধ্রুৱ fl = methodp->fileline();
                তার্কিক wasCreated = মিছা;
                AstClass* ধ্রুৱ varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ধ্রুৱ sizeVarp = createOrGetSizeVar(varClassp, arrVarp, fl,
                                                            methodp->findIntDType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                যদি (wasCreated) {
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ resizeVarRefp = buildMemberSelExprp(মূল_Exprp, arrVarp);
                    Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ sizeExprp = buildMemberSelExprp(মূল_Exprp, sizeVarp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->addNext(createSizeGteZeroConstraint(fl, sizeVarp, মূল_Exprp));
                    AstCMethodHard* ধ্রুৱ resizep
                        = নূতন AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, sizeExprp};
                    resizep->dtypep(methodp->findVoidDType());
                    inlineResizeStmtsp
                        = Ast_গ্ৰন্থি::addNext(inlineResizeStmtsp, নূতন Ast_Stmt_ৰাশিমালা{fl, resizep});
                }
                Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ size_গ্ৰন্থি_Exprp = buildMemberSelExprp(মূল_Exprp, sizeVarp);
                methodp->replaceWith(size_গ্ৰন্থি_Exprp);
                VL_DO_DANGLING(methodp->delete_বৃক্ষ(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_ৰাশিমালা_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                randModeVarp, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
        }

        // Call the solver and set return value
        AstCExpr* ধ্রুৱ solverCallp = নূতন AstCExpr{nodep->fileline()};
        solverCallp->dtypeSetBit();
        solverCallp->add(নূতন AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->add(".next(__Vm_rng)");
        randomizeFuncp->addStmtsp(নূতন AstAssign{
            nodep->fileline(),
            নূতন AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            নূতন AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        যদি (inlineResizeStmtsp) {
            AstVar* ধ্রুৱ retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ধ্রুৱ ifp = নূতন AstIf{nodep->fileline(),
                                         নূতন AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, nullptr};
            randomizeFuncp->addStmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        যদি (classHasRandClassMembers(classp)) {
            AstTask* ধ্রুৱ postTaskp = getCreateNestedCallbackTask(classp, "post");
            যদি (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->addStmtsp((নূতন AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        যদি (prePostWrap.second) {
            addVirtualPrePostCall(randomizeFuncp, classp, "post_randomize");
        } অন্যথা {
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
        VL_DO_DANGLING(withp->delete_বৃক্ষ(), withp);
    }
    শূন্য visit(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        যদি (m_constraintp && nodep->fromp()->user1()
            && (nodep->method() == VCMethod::ASSOC_SIZE
                || nodep->method() == VCMethod::DYN_SIZE)) {
            AstClass* ধ্রুৱ classp = VN_AS(m_modp, Class);
            AstVarRef* ধ্রুৱ queueVarRefp = VN_CAST(nodep->fromp(), VarRef);
            যদি (!queueVarRefp) {
                // Warning from ConstraintExprVisitor will be thrown
                ঘূৰাই;
            }
            AstVar* ধ্রুৱ queueVarp = queueVarRefp->varp();
            তার্কিক wasCreated = মিছা;
            AstVar* ধ্রুৱ sizeVarp
                = createOrGetSizeVar(classp, queueVarp, fl, nodep->findIntDType(), wasCreated);
            যদি (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                যদি (!VN_IS(queueVarp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    যদি (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ধ্রুৱ sizeVarRefp = নূতন AstVarRef{
                        fl, VN_AS(sizeVarp->user2p(), গ্ৰন্থি_Module), sizeVarp, VAccess::READ};
                    AstCMethodHard* ধ্রুৱ resizep = নূতন AstCMethodHard{
                        fl, nodep->fromp()->unlinkFrBack(), VCMethod::DYN_RESIZE, sizeVarRefp};
                    resizep->dtypep(nodep->findVoidDType());
                    resizerTaskp->addStmtsp(নূতন Ast_Stmt_ৰাশিমালা{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->addItemsp(createSizeGteZeroConstraint(fl, sizeVarp));
            }
            AstVarRef* ধ্রুৱ sizeVarRefp = নূতন AstVarRef{fl, VN_AS(sizeVarp->user2p(), গ্ৰন্থি_Module),
                                                         sizeVarp, VAccess::READ};
            sizeVarRefp->user1(সঁচা);
            nodep->replaceWith(sizeVarRefp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
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
        nodep->foreach([&](AstClass* ধ্রুৱ classp) {
            যদি (!classp->user1()) ঘূৰাই;
            classp->foreachMember([&](AstClass* ধ্রুৱ ownerClassp, AstConstraint* ধ্রুৱ constrp) {
                constrp->foreach([&](AstMemberSel* ধ্রুৱ memberSelp) {
                    AstVar* ধ্রুৱ varp = memberSelp->varp();
                    যদি (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) ঘূৰাই;
                    যদি (VN_AS(varp->user2p(), গ্ৰন্থি_Module) != ownerClassp) ঘূৰাই;
                    যদি (!varp->user3()) varp->user3(সঁচা);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(ঠেলো_Deletep(constrp->unlinkFrBack()), constrp);
        });
    }
    ~RandomizeVisitor() override = স্ৱভাৱ;
};

//######################################################################
// Randomize method class functions

শূন্য V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ধ্রুৱ RandomizeMarkVisitor markVisitor{nodep};
        ধ্রুৱ RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_বৃক্ষ("randomize", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ধ্রুৱ std::string& name, তার্কিক allowVirtual,
                                       তার্কিক childDType) {
    AstFunc* funcp = VN_AS(memberMap.findMember(nodep, name), Func);
    যদি (!funcp) {
        v3Global.useRandomizeMethods(সঁচা);
        Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep
            = childDType
                  ? নূতন AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ধ্রুৱ fvarp = childDType
                                  ? নূতন AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlagChildDType{}, dtypep}
                                  : নূতন AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3ErrorCode::NORETURN, সঁচা);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(সঁচা);
        fvarp->funcReturn(সঁচা);
        fvarp->direction(VDirection::OUTPUT);
        nodep->addMembersp(funcp);
        funcp = নূতন AstFunc{nodep->fileline(), name, nullptr, fvarp};
        যদি (!childDType) funcp->dtypep(dtypep);
        funcp->classMethod(সঁচা);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->addMembersp(funcp);
        memberMap.ঢোকাও(nodep, funcp);
    }
    ঘূৰাই funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_গ্ৰন্থি_Module* nodep, ধ্রুৱ std::string& name) {
    AstFunc* funcp = nullptr;
    v3Global.useRandomizeMethods(সঁচা);
    Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
    AstVar* ধ্রুৱ fvarp = নূতন AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(সঁচা);
    fvarp->funcReturn(সঁচা);
    fvarp->direction(VDirection::OUTPUT);
    funcp = নূতন AstFunc{nodep->fileline(), name, nullptr, fvarp};
    funcp->dtypep(dtypep);
    যদি (VN_IS(nodep, Class)) {
        funcp->classMethod(সঁচা);
    } অন্যথা {
        funcp->classMethod(সঁচা);
        funcp->isStatic(সঁচা);
    }
    nodep->addStmtsp(funcp);
    ঘূৰাই funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ধ্রুৱ basep = nodep->baseMostClassp();
    AstFunc* funcp = VN_AS(memberMap.findMember(basep, "srandom"), Func);
    যদি (!funcp) {
        v3Global.useRandomizeMethods(সঁচা);
        Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep
            = basep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ধ্রুৱ ivarp = নূতন AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(সঁচা);
        ivarp->direction(VDirection::প্ৰবেশ);
        funcp = নূতন AstFunc{basep->fileline(), "srandom", ivarp, nullptr};
        funcp->dtypep(basep->findVoidDType());
        funcp->classMethod(সঁচা);
        funcp->isVirtual(মিছা);
        basep->addMembersp(funcp);
        memberMap.ঢোকাও(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        যদি (basep->name() == "process") {
            funcp->addStmtsp(নূতন AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } অন্যথা {
            funcp->addStmtsp(নূতন AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(সঁচা);
        }
    }
    ঘূৰাই funcp;
}
