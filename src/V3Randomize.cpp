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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Randomize.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3Container.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3Task.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <queue>
#अन्तर्भाव <tuple>
#अन्तर्भाव <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

गणना ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

स्थिर constexpr ध्रुव वर्ण* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
स्थिर constexpr ध्रुव वर्ण* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
स्थिर AstVar* getRandModeVarFromClass(Ast_ग्रन्थिः_Module* classp) {
    याव (classp) {
        सचे (classp->user2p()) पटिनिवत्त VN_AS(classp->user2p(), Var);
        AstClass* ध्रुव cp = VN_CAST(classp, Class);
        सचे (!cp || !cp->extendsp()) पटिनिवत्त nullptr;
        classp = cp->extendsp()->classp();
    }
    पटिनिवत्त nullptr;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
स्थिर AstVar* getRandomGenerator(AstClass* ध्रुव classp) {
    सचे (classp->user3p()) पटिनिवत्त VN_AS(classp->user3p(), Var);
    सचे (classp->extendsp()) पटिनिवत्त getRandomGenerator(classp->extendsp()->classp());
    पटिनिवत्त nullptr;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
स्थिर AstTask* getCreateConstraintSetupFunc(AstClass* classp, VMemberMap& memberMap) {
    स्थिर ध्रुव वर्ण* ध्रुव name = "__Vsetup_constraints";
    AstTask* setupAllTaskp = VN_AS(memberMap.findMember(classp, name), Task);
    सचे (setupAllTaskp) पटिनिवत्त setupAllTaskp;
    setupAllTaskp = नूतन AstTask{classp->fileline(), "__Vsetup_constraints", nullptr};
    setupAllTaskp->classMethod(सच्च);
    setupAllTaskp->isVirtual(सच्च);
    classp->addMembersp(setupAllTaskp);
    memberMap.निवेशय(classp, setupAllTaskp);
    पटिनिवत्त setupAllTaskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

संरचना RandModeTarget final {
    // MEMBERS
    AstVar* ध्रुव receiverp;  // Variable that is the target of the rand_mode() call
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp;  // Expression from which the rand_mode() call originates
    AstClass* ध्रुव classp;  // Class in which rand_mode should be set

    // METHODS
    स्थिर RandModeTarget get(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_Module* modp) {
        सचे (!fromp) पटिनिवत्त {nullptr, nullptr, VN_AS(modp, Class)};
        सचे (AstCMethodHard* ध्रुव methodHardp = VN_CAST(fromp, CMethodHard)) {
            पटिनिवत्त RandModeTarget::get(methodHardp->fromp(), modp);
        }
        AstVar* receiverp = nullptr;
        AstClass* classp = VN_CAST(modp, Class);
        सचे (AstVarRef* ध्रुव varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            सचे (receiverp->isRand()) {
                fromp = nullptr;
                सचे (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } अञ्ञथा सचे (AstMemberSel* ध्रुव memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            सचे (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        सचे (!receiverp->isRand()) {
            AstClassRefDType* ध्रुव classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            सचे (classRefDtp) classp = classRefDtp->classp();
        }
        पटिनिवत्त {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

सङ्घ RandomizeMode final {
    // MEMBERS
    संरचना {
        तार्किक usesMode : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t index : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
स्थिर Ast_ग्रन्थिः_DType* arrayElementDTypep(Ast_ग्रन्थिः_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    याव (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    पटिनिवत्त dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
स्थिर तार्किक dtypeContainsReal(Ast_ग्रन्थिः_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    सचे (Ast_ग्रन्थिः_DType* ध्रुव subp = dtypep->subDTypep()) पटिनिवत्त dtypeContainsReal(subp);
    सचे (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव structp = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
        सचे (structp->packed()) पटिनिवत्त मिच्छा;
        अत्थाय (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            सचे (dtypeContainsReal(memberp->subDTypep())) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }
    ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(dtypep, BasicDType);
    पटिनिवत्त basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
स्थिर शून्य checkRandTypeEligibility(Ast_ग्रन्थिः* contextp, Ast_ग्रन्थिः_DType* dtypep, तार्किक isRandc) {
    dtypep = dtypep->skipRefp();
    सचे (Ast_ग्रन्थिः_DType* ध्रुव subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        पटिनिवत्त;
    }
    सचे (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        सचे (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        पटिनिवत्त;
    }
    सचे (ध्रुव AstIfaceRefDType* ध्रुव ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        पटिनिवत्त;
    }
    सचे (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव structp = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
        सचे (structp->packed()) पटिनिवत्त;  // Integral by construction; members checked separately
        सचे (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            पटिनिवत्त;
        }
        अत्थाय (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            सचे (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        पटिनिवत्त;
    }
    ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    सचे (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        सचे (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        पटिनिवत्त;
    }
    विकल्प (basicp->keyword()) {
    पक्ष VBasicDTypeKwd::STRING:
    पक्ष VBasicDTypeKwd::CHANDLE:
    पक्ष VBasicDTypeKwd::EVENT: {
        ध्रुव वर्ण* ध्रुव articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        भङ्ग;
    }
    स्वभाव: भङ्ग;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

वग्ग RandomizeMarkVisitor final : सार्वजनिक VNVisitor {
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
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;

    using DerivedSet = std::unordered_set<AstClass*>;
    using BaseToDerivedMap = std::unordered_map<ध्रुव AstClass*, DerivedSet>;

    BaseToDerivedMap m_baseToDerivedMap;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = nullptr;  // Current class
    Ast_ग्रन्थिः* m_constraint_व्यञ्जकम्_Genp = nullptr;  // Current constraint or constraint if expression
    Ast_ग्रन्थिः_Module* m_modp;  // Current module
    Ast_ग्रन्थिः_Stmt* m_stmtp = nullptr;  // Current statement
    Ast_ग्रन्थिः_FTask_Ref* m_stdRandCallp = nullptr;  // Current std::randomize() call
    तार्किक m_inStdWith = मिच्छा;  // True when inside a 'with {}' clause
    std::set<Ast_ग्रन्थिः_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = nullptr;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    शून्य markGlobalConstrainedVars(AstClass* classp) {
        अत्थाय (ध्रुव AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            अत्थाय (Ast_ग्रन्थिः* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ध्रुव varp = VN_CAST(memberp, Var);
                सचे (!varp) अनुवर्त;
                सचे (varp->rand().isRandomizable()) varp->globalConstrained(सच्च);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    तार्किक isVarInStdRandomizeArgs(ध्रुव AstVar* varp) ध्रुव {
        सचे (!m_inStdWith || !m_stdRandCallp) पटिनिवत्त मिच्छा;

        अत्थाय (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            याव (exprp) {
                सचे (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(exprp, ग्रन्थिः_Var_Ref)) {
                    सचे (varrefp->varp() == varp) पटिनिवत्त सच्च;
                    भङ्ग;
                }
                सचे (ध्रुव AstMemberSel* ध्रुव memberselp = VN_CAST(exprp, MemberSel)) {
                    सचे (memberselp->varp() == varp) पटिनिवत्त सच्च;
                    exprp = memberselp->fromp();
                } अञ्ञथा सचे (ध्रुव AstArraySel* ध्रुव arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } अञ्ञथा सचे (ध्रुव AstStructSel* ध्रुव strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } अञ्ञथा सचे (ध्रुव AstCMethodHard* ध्रुव methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } अञ्ञथा {
                    भङ्ग;
                }
            }
        }
        पटिनिवत्त मिच्छा;
    }
    शून्य markMembers(ध्रुव AstClass* nodep) {
        अत्थाय (ध्रुव AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            अत्थाय (Ast_ग्रन्थिः* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ध्रुव varp = VN_CAST(memberp, Var);
                सचे (!varp) अनुवर्त;
                // If member is randomizable and of class type, mark its class
                सचे (varp->rand().isRandomizable()) {
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव varDtypep = varp->dtypep()->skipRefp();
                    ध्रुव AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    सचे (!classRefp) {
                        ध्रुव Ast_ग्रन्थिः_DType* subDTypep = varDtypep->subDTypep();
                        सचे (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    सचे (classRefp) {
                        AstClass* ध्रुव rclassp = classRefp->classp();
                        सचे (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    सचे (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        RandomizeMode randMode = {};
                        randMode.usesMode = सच्च;
                        varp->user1(randMode.asUQuad);
                    }
                }
            }
        }
    }
    शून्य markDerived(ध्रुव AstClass* nodep) {
        ध्रुव auto it = m_baseToDerivedMap.find(nodep);
        सचे (it != m_baseToDerivedMap.end()) {
            अत्थाय (auto* classp : it->second) {
                सचे (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    शून्य markAllDerived() {
        अत्थाय (ध्रुव auto& p : m_baseToDerivedMap) {
            सचे (p.first->user1()) markDerived(p.first);
        }
    }
    शून्य setPackageRefs() {
        अत्थाय (Ast_ग्रन्थिः_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), ग्रन्थिः_Module));
        }
    }
    शून्य markNestedGlobalConstrainedRecurse(Ast_ग्रन्थिः* nodep) {
        सचे (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            AstVar* ध्रुव varp = refp->varp();
            सचे (varp->globalConstrained()) पटिनिवत्त;
            varp->globalConstrained(सच्च);
        } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव memberSelp = VN_CAST(nodep, MemberSel)) {
            सचे (memberSelp->varp()) {
                AstVar* ध्रुव varp = memberSelp->varp();
                सचे (varp->globalConstrained()) पटिनिवत्त;
                varp->globalConstrained(सच्च);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_ग्रन्थिः_व्यञ्जकम्* buildMemberSelChain(AstVarRef* मूलम्_Var_Refp, ध्रुव std::vector<AstVar*>& path) {
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = मूलम्_Var_Refp->clone_वृक्षः(मिच्छा);
        अत्थाय (AstVar* memberVarp : path) {
            AstMemberSel* memberSelp
                = नूतन AstMemberSel{मूलम्_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        पटिनिवत्त exprp;
    }

    // Process a single constraint during nested constraint cloning
    शून्य processNestedConstraint(AstConstraint* ध्रुव constrp, AstVarRef* मूलम्_Var_Refp,
                                 ध्रुव std::vector<AstVar*>& newPath, AstClass* targetClassp) {
        std::string pathPrefix = मूलम्_Var_Refp->name();
        अत्थाय (AstVar* pathMemberVarp : newPath) {
            pathPrefix += GLOBAL_CONSTRAINT_SEPARATOR + pathMemberVarp->name();
        }

        ध्रुव std::string newName = pathPrefix + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        तार्किक isDuplicate = मिच्छा;
        targetClassp->foreachMember([&](AstClass* ध्रुव, AstConstraint* ध्रुव existingConstrp) {
            सचे (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = सच्च;
            }
        });
        सचे (isDuplicate) पटिनिवत्त;

        AstConstraint* ध्रुव cloneConstrp = constrp->clone_वृक्षः(मिच्छा);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::vector<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
            सचे (AstVarRef* ध्रुव varRefp = VN_CAST(exprp, VarRef)) {
                सचे (!varRefp->varp()->isClassMember()) पटिनिवत्त;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव chainp = buildMemberSelChain(मूलम्_Var_Refp, newPath);
                AstMemberSel* ध्रुव finalSelp
                    = नूतन AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(varRefp), varRefp);
            } अञ्ञथा सचे (AstFuncRef* ध्रुव funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                सचे (funcRefp->taskp()->classMethod() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.प्रक्षिप_back(funcRefp);
                }
            }
        });
        अत्थाय (AstFuncRef* funcRefp : funcRefps) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव chainp = buildMemberSelChain(मूलम्_Var_Refp, newPath);
            AstArg* ध्रुव argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* ध्रुव callp
                = नूतन AstMethodCall{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        targetClassp->addStmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    शून्य cloneNestedConstraintsRecurse(AstVarRef* मूलम्_Var_Refp, AstClass* classp,
                                       ध्रुव std::vector<AstVar*>& pathToClass,
                                       AstClass* targetClassp) {
        अत्थाय (Ast_ग्रन्थिः* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ध्रुव memberVarp = VN_CAST(memberNodep, Var);
            सचे (!memberVarp) अनुवर्त;
            सचे (!memberVarp->rand().isRandomizable()) अनुवर्त;
            ध्रुव AstClassRefDType* ध्रुव memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            सचे (!memberClassRefp || !memberClassRefp->classp()) अनुवर्त;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::vector<AstVar*> newPath = pathToClass;
            newPath.प्रक्षिप_back(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ध्रुव containingClassp, AstConstraint* ध्रुव constrp) {
                    processNestedConstraint(constrp, मूलम्_Var_Refp, newPath, targetClassp);
                });
            cloneNestedConstraintsRecurse(मूलम्_Var_Refp, nestedClassp, newPath, targetClassp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    शून्य handleRandomizeArgument(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar* ध्रुव varp, ध्रुव तार्किक stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        याव (exprp) {
            AstVar* randVarp = nullptr;
            सचे (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } अञ्ञथा सचे (AstArraySel* ध्रुव arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                अनुवर्त;  // Skip ArraySel, continue traversing
            } अञ्ञथा सचे (AstStructSel* ध्रुव structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                अनुवर्त;  // Skip StructSel, continue traversing
            } अञ्ञथा सचे (AstCMethodHard* ध्रुव methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                अनुवर्त;
            } अञ्ञथा सचे (AstVarRef* ध्रुव varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(सच्च);
                varrefp->access(VAccess::READWRITE);
                exprp = nullptr;
            } अञ्ञथा {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            सचे (randVarp == varp) पटिनिवत्त;
            Ast_ग्रन्थिः* backp = randVarp;
            याव (backp && !VN_IS(backp, ग्रन्थिः_Module)) backp = backp->backp();
            सचे (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } अञ्ञथा {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                RandomizeMode randMode = {};
                randMode.usesMode = सच्च;
                randVarp->user1(randMode.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        सचे (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ध्रुव AstClass* ध्रुव basep = nodep->extendsp()->classp();
            m_baseToDerivedMap[basep].निवेशय(nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        सचे (!nodep->backp()) VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        सचे (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        सचे (nodep->name() == "rand_mode") {
            AstMethodCall* ध्रुव methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = methodCallp ? methodCallp->fromp() : nullptr;
            तार्किक valid = सच्च;
            सचे (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = मिच्छा;
            } अञ्ञथा सचे (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = मिच्छा;
            } अञ्ञथा सचे (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = मिच्छा;
            } अञ्ञथा सचे (AstCMethodHard* ध्रुव methodHardp = VN_CAST(fromp, CMethodHard)) {
                सचे (methodHardp->method() == VCMethod::ARRAY_AT
                    || methodHardp->method() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = मिच्छा;
                } अञ्ञथा {
                    methodHardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            सचे (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = मिच्छा;
            }
            सचे (valid) {
                ध्रुव RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_classp);
                सचे ((!randModeTarget.receiverp || !randModeTarget.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = मिच्छा;
                } अञ्ञथा सचे (!randModeTarget.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = मिच्छा;
                } अञ्ञथा सचे (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = मिच्छा;
                } अञ्ञथा सचे (randModeTarget.receiverp && randModeTarget.receiverp->isRand()) {
                    // Called on a rand member variable
                    RandomizeMode randMode = {};
                    randMode.usesMode = सच्च;
                    randModeTarget.receiverp->user1(randMode.asUQuad);
                } अञ्ञथा {
                    // Called on 'this' or a non-rand class instance
                    randModeTarget.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        सचे (!varp->isRand()) पटिनिवत्त;
                        RandomizeMode randMode = {};
                        randMode.usesMode = सच्च;
                        varp->user1(randMode.asUQuad);
                    });
                }
            }
            सचे (!valid) {
                सचे (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                    nodep->replaceWith(नूतन AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                } अञ्ञथा {
                    m_stmtp->unlinkFrBack();
                }
            }
            पटिनिवत्त;
        }

        सचे (nodep->name() == "constraint_mode") {
            तार्किक valid = सच्च;
            सचे (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = मिच्छा;
            } अञ्ञथा सचे (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)
                       && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = मिच्छा;
            }
            AstConstraint* constrp = nullptr;
            AstClass* classp = m_classp;
            सचे (ध्रुव AstMethodCall* ध्रुव methodCallp = VN_CAST(nodep, MethodCall)) {
                सचे (ध्रुव AstConstraintRef* ध्रुव constrRefp
                    = VN_CAST(methodCallp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    सचे (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } अञ्ञथा सचे (AstClassRefDType* classRefDtp
                           = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } अञ्ञथा {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = मिच्छा;
                }
            }
            सचे (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = मिच्छा;
            }
            सचे (valid) {
                RandomizeMode constraintMode = {};
                constraintMode.usesMode = सच्च;
                सचे (constrp) {
                    constrp->user1(constraintMode.asUQuad);
                } अञ्ञथा {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraintMode.asUQuad);
                    });
                }
            } अञ्ञथा {
                सचे (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                    nodep->replaceWith(नूतन AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                } अञ्ञथा {
                    m_stmtp->unlinkFrBack();
                }
            }
            पटिनिवत्त;
        }

        सचे (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            सचे (m_constraint_व्यञ्जकम्_Genp || m_inStdWith) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                अत्थाय (Ast_ग्रन्थिः* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    सचे (argp->user1()) {
                        nodep->user1(सच्च);
                        भङ्ग;
                    }
                }
            }
            पटिनिवत्त;
        }
        AstClass* classp = m_classp;
        सचे (ध्रुव AstMethodCall* ध्रुव methodCallp = VN_CAST(nodep, MethodCall)) {
            सचे (ध्रुव AstClassRefDType* ध्रुव classRefp
                = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        सचे (classp) {
            सचे (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ध्रुव, AstVar* ध्रुव memberVarp) {
                सचे (!memberVarp->rand().isRandomizable()) पटिनिवत्त;
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = memberVarp->dtypep()->skipRefp();
                ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
                सचे (!classRefp || !classRefp->classp()) पटिनिवत्त;
                AstClass* ध्रुव memberClassp = classRefp->classp();
                सचे (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) पटिनिवत्त;
                memberVarp->globalConstrained(सच्च);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* मूलम्_Var_Refp
                    = नूतन AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::vector<AstVar*> emptyPath;
                memberClassp->foreachMember([&](AstClass* ध्रुव, AstConstraint* ध्रुव constrp) {
                    processNestedConstraint(constrp, मूलम्_Var_Refp, emptyPath, classp);
                });
                cloneNestedConstraintsRecurse(मूलम्_Var_Refp, memberClassp, emptyPath, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(मूलम्_Var_Refp->delete_वृक्षः(), मूलम्_Var_Refp);
            });
        }
        सचे (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = nullptr;
            अत्थाय (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handleRandomizeArgument(argp->exprp(), nullptr, सच्च);
            }
            पटिनिवत्त;
        }
        अत्थाय (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->अङ्कः().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ध्रुव तार्किक hasUnsupportedMember
                    = classp->existsMember([](ध्रुव AstClass*, ध्रुव AstVar* memberVarp) {
                          सचे (!memberVarp->rand().isRandomizable()) पटिनिवत्त मिच्छा;
                          ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtp = memberVarp->dtypep()->skipRefp();
                          पटिनिवत्त VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, QueueDType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                सचे (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                अनुवर्त;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = nullptr;  // If nodep is a method call, this is its receiver
            सचे (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                सचे (AstMemberSel* ध्रुव memberSelp = VN_CAST(methodCallp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } अञ्ञथा {
                    AstVarRef* ध्रुव varrefp = VN_AS(methodCallp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handleRandomizeArgument(argp->exprp(), fromVarp, मिच्छा);
        }
        // Re-mark members after the class became inline-randomized above.
        सचे (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    शून्य visit(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_व्यञ्जकम्_Genp);
        m_stmtp = nodep;
        m_constraint_व्यञ्जकम्_Genp = nodep;
        iterateChildren(nodep);
        सचे (!nodep->backp()) VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(Ast_Constraint_व्यञ्जकम्* nodep) override {
        VL_RESTORER(m_constraint_व्यञ्जकम्_Genp);
        m_constraint_व्यञ्जकम्_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_व्यञ्जकम्_Genp);
            m_constraint_व्यञ्जकम्_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterateAndNextConstNull(nodep->thensp());
        iterateAndNextConstNull(nodep->elsesp());
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        सचे (!m_constraint_व्यञ्जकम्_Genp) पटिनिवत्त;

        सचे (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        सचे (nodep->varp()->rand().isRandomizable() && !(m_inStdWith && m_stdRandCallp)) {
            nodep->user1(सच्च);
        } अञ्ञथा सचे (isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(सच्च);
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        सचे (!m_constraint_व्यञ्जकम्_Genp) पटिनिवत्त;
        iterateChildrenConst(nodep);
        ध्रुव तार्किक randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ध्रुव तार्किक randMember = nodep->varp()->rand().isRandomizable();
        ध्रुव तार्किक inStdWith = m_inStdWith && m_stdRandCallp;
        सचे (randObject && randMember && !inStdWith) {
            nodep->user1(सच्च);
        } अञ्ञथा सचे (inStdWith && isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(सच्च);
            // Mark parent object for constraint expression visitor
            सचे (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(सच्च);
        }

        सचे (m_withp) {
            Ast_ग्रन्थिः* backp = m_withp;
            याव (backp->backp()) {
                सचे (ध्रुव AstMethodCall* ध्रुव callp = VN_CAST(backp, MethodCall)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    भङ्ग;
                }
                backp = backp->backp();
            }
        } अञ्ञथा {
            nodep->user2p(m_modp);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstVar* nodep) override {
        nodep->user2p(m_modp);
        सचे (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        सचे (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_inStdWith = सच्च;
        iterateChildrenConst(nodep);
        m_inStdWith = मिच्छा;
    }

    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        iterateChildrenConst(nodep);
        सचे (!m_constraint_व्यञ्जकम्_Genp && !m_inStdWith) पटिनिवत्त;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    शून्य visit(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        सचे (!m_constraint_व्यञ्जकम्_Genp && !m_inStdWith) पटिनिवत्त;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit RandomizeMarkVisitor(Ast_ग्रन्थिः* nodep) {
        iterateConst(nodep);
        markAllDerived();
        setPackageRefs();
    }
    ~RandomizeMarkVisitor() override = स्वभाव;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

वग्ग Constraint_व्यञ्जकम्_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ध्रुव m_classp;
    Ast_ग्रन्थिः_FTask* ध्रुव m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_ग्रन्थिः_FTask* ध्रुव m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ध्रुव m_genp;  // VlRandomizer variable of the class
    AstVar* m_randModeVarp;  // Relevant randmode state variable
    तार्किक m_wantSingle = मिच्छा;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    तार्किक m_structSel = मिच्छा;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    तार्किक m_deferRandModeHoist = मिच्छा;  // Wait for the complete struct member path
    std::set<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::set<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertionSet<AstVar*>* m_sizeConstrainedArraysp = nullptr;  // Arrays with size constraints
    Ast_ग्रन्थिः_व्यञ्जकम्* m_conditionp = nullptr;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_ग्रन्थिः_FTask* m_prepareConstrainedArraysp = nullptr;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = nullptr;  // Current ID of unique call
    V3UniqueNames& m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    std::vector<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_ग्रन्थिः* m_firstExpressionInsideIndexp = nullptr;
    AstConstraintForeach* m_foreachp = nullptr;  // Innermost constraint-foreach being processed
    std::set<ध्रुव AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    वग्ग NestedAccessPath final {
        AstMemberSel* m_शिखरम्_Nested_Array_Member_Selp
            = nullptr;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = nullptr;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = nullptr;  // Last node of variable's name format
        Ast_ग्रन्थिः** m_firstExpressionInsideIndexPointerp
            = nullptr;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    सार्वजनिक:
        शून्य addVarNamePart(AstSFormatF* ध्रुव nodep, std::string&& name) {
            सचे (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } अञ्ञथा {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.निवेशय(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            सचे (name == "") {
                सचे (*m_firstExpressionInsideIndexPointerp) {
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

        AstMemberSel* access_वृक्षः() { पटिनिवत्त m_शिखरम्_Nested_Array_Member_Selp; }
        ध्रुव std::string& smtName() { पटिनिवत्त m_smtName; }

        Ast_Stmt_व्यञ्जकम्* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ध्रुव methodp = नूतन AstCMethodHard{
                varp->fileline(),
                नूतन AstVarRef{varp->fileline(), VN_AS(genp->user2p(), ग्रन्थिः_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtypeSetVoid();

            methodp->addPinsp(m_शिखरम्_Nested_Array_Member_Selp);  // Variable

            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep();  // Width
            ध्रुव size_t width = m_शिखरम्_Nested_Array_Member_Selp->varp()->dtypep()->width();
            methodp->addPinsp(नूतन AstConst{dtypep->fileline(), AstConst::Unsized64{}, width});

            methodp->addPinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = nullptr;

            methodp->addPinsp(
                नूतन AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_शिखरम्_Nested_Array_Member_Selp = nullptr;
            पटिनिवत्त methodp->makeStmt();
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* cloneName() { पटिनिवत्त m_nestedNameFormatTopp->clone_वृक्षः(मिच्छा); }

        शून्य error() {
            VL_DO_DANGLING(m_शिखरम्_Nested_Array_Member_Selp->delete_वृक्षः(), m_शिखरम्_Nested_Array_Member_Selp);
        }

        NestedAccessPath(AstMemberSel* शिखरम्_Member_Selp, Ast_ग्रन्थिः** pointerToFirstExprp)
            : m_शिखरम्_Nested_Array_Member_Selp{शिखरम्_Member_Selp}
            , m_firstExpressionInsideIndexPointerp{pointerToFirstExprp} {}
        ~NestedAccessPath() {
            सचे (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_वृक्षः(),
                            m_nestedNameFormatTopp = nullptr);
                m_nestedNameFormatp = nullptr;
            }
        }
    };
    NestedAccessPath* m_nestedAccess = nullptr;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_ग्रन्थिः_व्यञ्जकम्* createEnumConstraint(File_पङ्क्तिः* ध्रुव fl, ध्रुव std::string& smtName,
                                      AstEnumDType* ध्रुव enumDtp, AstVar* ध्रुव genVarp) {
        Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(genVarp->user2p(), ग्रन्थिः_Module);
        ध्रुव पूर्णाङ्क width = enumDtp->width();
        std::string constraint = "(__Vbv (or";
        अत्थाय (AstEnumItem* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ध्रुव AstConst* ध्रुव vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvtToStr(vconstp->toUInt()) + " "
                          + cvtToStr(width) + "))";
        }
        constraint += "))";
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            fl, नूतन AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            नूतन AstCExpr{fl, AstCExpr::Pure{}, "\"" + constraint + "\""}};
        callp->dtypeSetVoid();
        पटिनिवत्त callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* findStaticRandModeVarMember(AstClass* classp) ध्रुव {
        याव (सच्च) {
            सचे (AstVar* ध्रुव varp
                = VN_CAST(m_memberMap.findMember(classp, "__Vstaticrandmode"), Var)) {
                पटिनिवत्त varp;
            }
            AstClassExtends* ध्रुव extendsp = classp->extendsp();
            सचे (!extendsp) पटिनिवत्त nullptr;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    शून्य markConstrainedRandMember(AstStructDType* ध्रुव structp, ध्रुव std::string& name) {
        AstMemberDType* ध्रुव memberp
            = VN_CAST(m_memberMap.findMember(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(सच्च);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    तार्किक isRandQualifiedPath(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        याव (nodep) {
            सचे (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
                सचे (AstStructDType* ध्रुव structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ध्रुव AstMemberDType* ध्रुव memberp
                        = VN_CAST(m_memberMap.findMember(structp, selp->name()), MemberDType);
                    सचे (memberp && !memberp->rand().isRandomizable()) पटिनिवत्त मिच्छा;
                }
                nodep = selp->fromp();
            } अञ्ञथा सचे (ध्रुव AstArraySel* ध्रुव selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } अञ्ञथा सचे (ध्रुव AstCMethodHard* ध्रुव methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } अञ्ञथा {
                भङ्ग;
            }
        }
        पटिनिवत्त सच्च;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    स्थिर शून्य markStructConstrainedRandRecurse(Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        AstStructDType* ध्रुव structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        सचे (!structp || structp->packed()) पटिनिवत्त;
        structp->markConstrainedRand(सच्च);
        अत्थाय (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            सचे (!memberp->rand().isRandomizable()) अनुवर्त;
            memberp->markConstrainedRand(सच्च);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string buildMemberPath(ध्रुव AstMemberSel* ध्रुव memberSelp) {
        ध्रुव Ast_ग्रन्थिः* fromp = memberSelp->fromp();
        सचे (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            पटिनिवत्त refp->name() + "." + memberSelp->name();
        } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            पटिनिवत्त buildMemberPath(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        पटिनिवत्त "";
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* getFromp(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
        सचे (ध्रुव AstCMethodHard* ध्रुव cmethp = VN_CAST(nodep, CMethodHard)) {
            पटिनिवत्त cmethp->fromp();
        }
        पटिनिवत्त getSelFromp(nodep);
    }

    स्थिर std::string extractSolveBeforeBaseName(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        याव (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = getFromp(exprp)) exprp = fromp;
        सचे (ध्रुव AstVarRef* ध्रुव vrefp = VN_CAST(exprp, VarRef)) पटिनिवत्त vrefp->name();
        पटिनिवत्त "";
    }

    स्थिर शून्य buildNamePrefix(AstCExpr* exprp, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
        सचे (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            exprp->add("(\""s + refp->name());
            पटिनिवत्त;
        }

        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = getFromp(nodep);
        सचे (fromp) buildNamePrefix(exprp, fromp);

        सचे (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
            exprp->add("." + selp->name());
        } अञ्ञथा सचे (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            पटिनिवत्त;
        } अञ्ञथा सचे (ध्रुव AstCMethodHard* ध्रुव cmethp = VN_CAST(nodep, CMethodHard)) {
            सचे (cmethp->method() == VCMethod::ARRAY_AT) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = cmethp->pinsp();
                exprp->add(".\" + ");
                AstCFuncHard* ध्रुव solverHexFuncp
                    = नूतन AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(argp->clone_वृक्षः_Pure(मिच्छा));
                solverHexFuncp->dtypeSetString();
                exprp->add(solverHexFuncp);
                exprp->add(" + \"");
            } अञ्ञथा {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                पटिनिवत्त;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    स्थिर AstCExpr* build_Path_व्यञ्जकम्(File_पङ्क्तिः* ध्रुव fl, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep,
                                   ध्रुव std::string selName) {
        AstCExpr* ध्रुव exprp = नूतन AstCExpr{fl, ""};
        buildNamePrefix(exprp, nodep);
        सचे (selName != "") {
            exprp->add(".");
            exprp->add(selName);
        }
        exprp->add("\")");
        पटिनिवत्त exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_व्यञ्जकम्(File_पङ्क्तिः* fl, ध्रुव std::string& baseName,
                                    ध्रुव Ast_ग्रन्थिः_Sel* selp) {
        AstCExpr* ध्रुव p = नूतन AstCExpr{fl, ""};
        p->add("(\""s + baseName + ".\"");
        p->add("+");
        AstCFuncHard* ध्रुव solverHexFuncp
            = नूतन AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
        solverHexFuncp->addPinsp(selp->bitp()->clone_वृक्षः_Pure(मिच्छा));
        solverHexFuncp->dtypeSetString();
        p->add(solverHexFuncp);
        p->add(")");
        p->dtypeSetString();
        पटिनिवत्त p;
    }

    // Helper: get fromp from MemberSel or StructSel
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* getSelFromp(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        सचे (ध्रुव AstMemberSel* ध्रुव mp = VN_CAST(exprp, MemberSel)) पटिनिवत्त mp->fromp();
        सचे (ध्रुव AstStructSel* ध्रुव sp = VN_CAST(exprp, StructSel)) पटिनिवत्त sp->fromp();
        पटिनिवत्त nullptr;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* build_Solve_Before_Name_व्यञ्जकम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        सचे (ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ध्रुव p = नूतन AstCExpr{fl, AstCExpr::Pure{}, "\"" + varrefp->name() + "\"s"};
            p->dtypeSetString();
            पटिनिवत्त p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selFromp = getSelFromp(exprp)) {
            ध्रुव std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            सचे (ध्रुव AstArraySel* ध्रुव arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string baseName;
                सचे (ध्रुव AstVarRef* ध्रुव vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    baseName = vp->name();
                } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    baseName = buildMemberPath(mp);
                }
                सचे (baseName.empty()) पटिनिवत्त nullptr;
                AstCExpr* ध्रुव p = नूतन AstCExpr{fl, ""};
                p->add("(\""s + baseName + ".\"");
                p->add("+");
                AstCFuncHard* ध्रुव solverHexFuncp
                    = नूतन AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(arrSelp->bitp()->clone_वृक्षः_Pure(मिच्छा));
                solverHexFuncp->dtypeSetString();
                p->add(solverHexFuncp);
                p->add(")");
                p->dtypeSetString();
                पटिनिवत्त p;
            }
            // Static member path (obj.field)
            सचे (ध्रुव AstVarRef* ध्रुव vp = VN_CAST(selFromp, VarRef)) {
                ध्रुव std::string path = vp->name() + "." + selName;
                AstCExpr* ध्रुव p = नूतन AstCExpr{fl, AstCExpr::Pure{}, "\"" + path + "\"s"};
                p->dtypeSetString();
                पटिनिवत्त p;
            }
            सचे (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ध्रुव p = build_Path_व्यञ्जकम्(fl, selFromp, selName);
                p->dtypeSetString();
                पटिनिवत्त p;
            }
            पटिनिवत्त nullptr;
        }
        सचे (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ध्रुव p = build_Path_व्यञ्जकम्(fl, exprp, "");
            p->dtypeSetString();
            पटिनिवत्त p;
        }
        ध्रुव Ast_ग्रन्थिः_Sel* selp = VN_CAST(exprp, ArraySel);
        सचे (!selp) selp = VN_CAST(exprp, AssocSel);
        सचे (selp) {
            // arr[i] -> dynamic name
            std::string baseName;
            सचे (ध्रुव AstVarRef* ध्रुव vp = VN_CAST(selp->fromp(), VarRef)) {
                baseName = vp->name();
            } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव mp = VN_CAST(selp->fromp(), MemberSel)) {
                baseName = buildMemberPath(mp);
            }
            सचे (baseName.empty()) पटिनिवत्त nullptr;
            पटिनिवत्त build_Array_Sel_Name_व्यञ्जकम्(fl, baseName, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        सचे (ध्रुव AstSel* ध्रुव bitSelp = VN_CAST(exprp, Sel)) {
            पटिनिवत्त build_Solve_Before_Name_व्यञ्जकम्(fl, bitSelp->fromp());
        }
        पटिनिवत्त nullptr;
    }

    AstSFormatF* getConstFormat(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        पटिनिवत्त नूतन AstSFormatF{nodep->fileline(), (nodep->width() & 3) ? "#b%b" : "#x%x", मिच्छा,
                               nodep};
    }
    तार्किक editFormat(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (nodep->user1()) पटिनिवत्त मिच्छा;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        handle.relink(getConstFormat(nodep));
        पटिनिवत्त सच्च;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* wrapWithCond(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp,
                              ध्रुव पूर्णाङ्क width) {
        सचे (condp) {
            File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
            पटिनिवत्त नूतन AstCond{
                flp, condp, exprp,
                getConstFormat(नूतन AstConst{flp, AstConst::Widthed_मानम्{}, width, 0})};
        }
        पटिनिवत्त exprp;
    }
    AstSFormatF* editSMT(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nullptr,
                         Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nullptr, Ast_ग्रन्थिः_व्यञ्जकम्* thsp = nullptr) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_व्यञ्जकम् = nodep->emitSMT();  // Might need child width (AstExtend)
        सचे (smt_व्यञ्जकम् == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            पटिनिवत्त nullptr;
        }

        ध्रुव पूर्णाङ्क lhsWidth = lhsp ? lhsp->width() : 0;
        ध्रुव पूर्णाङ्क rhsWidth = rhsp ? rhsp->width() : 0;
        ध्रुव पूर्णाङ्क thsWidth = thsp ? thsp->width() : 0;
        Ast_ग्रन्थिः_व्यञ्जकम्* lhsCondp = nullptr;
        Ast_ग्रन्थिः_व्यञ्जकम्* rhsCondp = nullptr;
        Ast_ग्रन्थिः_व्यञ्जकम्* thsCondp = nullptr;

        सचे (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlinkFrBack() : lhsp),
                         ग्रन्थिः_व्यञ्जकम्);
            lhsCondp = m_conditionp;
        }
        सचे (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlinkFrBack() : rhsp),
                         ग्रन्थिः_व्यञ्जकम्);
            rhsCondp = m_conditionp;
        }
        सचे (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlinkFrBack() : thsp),
                         ग्रन्थिः_व्यञ्जकम्);
            thsCondp = m_conditionp;
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* argsp = nullptr;
        अत्थाय (string::iterator pos = smt_व्यञ्जकम्.begin(); pos != smt_व्यञ्जकम्.end(); ++pos) {
            सचे (pos[0] == '%') {
                ++pos;
                विकल्प (pos[0]) {
                पक्ष '%': भङ्ग;
                पक्ष 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ग्रन्थिः::addNext(argsp, wrapWithCond(lhsp, lhsCondp, lhsWidth));
                    lhsp = nullptr;
                    भङ्ग;
                पक्ष 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ग्रन्थिः::addNext(argsp, wrapWithCond(rhsp, rhsCondp, rhsWidth));
                    rhsp = nullptr;
                    भङ्ग;
                पक्ष 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ग्रन्थिः::addNext(argsp, wrapWithCond(thsp, thsCondp, thsWidth));
                    thsp = nullptr;
                    भङ्ग;
                स्वभाव: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); भङ्ग;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ध्रुव newp = नूतन AstSFormatF{nodep->fileline(), smt_व्यञ्जकम्, मिच्छा, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        पटिनिवत्त newp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* editSingle(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* itemsp) {
        सचे (!itemsp) पटिनिवत्त nullptr;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = सच्च;

        {
            AstBegin* ध्रुव tempp
                = नूतन AstBegin{fl, "[EditWrapper]", itemsp->unlinkFrBackWithNext(), मिच्छा};
            VL_DO_DANGLING(iterateAndNextNull(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            सचे (itemsp) itemsp->unlinkFrBackWithNext();
            VL_DO_DANGLING(tempp->delete_वृक्षः(), tempp);
        }
        सचे (!itemsp) पटिनिवत्त nullptr;

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprsp = VN_CAST(itemsp, ग्रन्थिः_व्यञ्जकम्);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        सचे (!exprsp->nextp()) पटिनिवत्त exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        अत्थाय (Ast_ग्रन्थिः* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        पटिनिवत्त नूतन AstSFormatF{fl, fmt.str(), मिच्छा, exprsp};
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* arrayp, Ast_ग्रन्थिः_व्यञ्जकम्* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_ग्रन्थिः_DType* ध्रुव arrDtp = arrayp->unlinkFrBack()->dtypep()->skipRefp();
        Ast_ग्रन्थिः_व्यञ्जकम्* selp = nullptr;
        सचे (VN_IS(arrDtp, QueueDType) || VN_IS(arrDtp, DynArrayDType))
            selp = नूतन AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        अञ्ञथा सचे (VN_IS(arrDtp, UnpackArrayDType))
            selp = नूतन AstArraySel{fl, arrayp, idxp};
        अञ्ञथा सचे (VN_IS(arrDtp, AssocArrayDType))
            selp = नूतन AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        पटिनिवत्त selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_ग्रन्थिः_व्यञ्जकम्* adjustWidth(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, पूर्णाङ्क targetWidth,
                             VSigning targetSigning) {
        ध्रुव पूर्णाङ्क व्यञ्जकम्_Width = exprp->width();
        सचे (targetWidth > व्यञ्जकम्_Width) {
            // Extend to match target width
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव result = नूतन AstExtend{fl, exprp, targetWidth};
            result->dtypeChgSigned(targetSigning);
            पटिनिवत्त result;
        } अञ्ञथा सचे (targetWidth < व्यञ्जकम्_Width) {
            // Truncate to match target width
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव result = नूतन AstSel{fl, exprp, 0, targetWidth};
            result->dtypeChgSigned(targetSigning);
            पटिनिवत्त result;
        } अञ्ञथा {
            // Width already matches
            पटिनिवत्त exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_ग्रन्थिः_व्यञ्जकम्* extract_Return_व्यञ्जकम्(AstFunc* funcp) {
        AstVar* ध्रुव retVarp = VN_CAST(funcp->fvarp(), Var);
        सचे (!retVarp) पटिनिवत्त nullptr;
        Ast_ग्रन्थिः_व्यञ्जकम्* retExprp = nullptr;
        पूर्णाङ्क assign_गणनम् = 0;
        funcp->foreach([&](AstAssign* assignp) {
            AstVarRef* ध्रुव lhsp = VN_CAST(assignp->lhsp(), VarRef);
            सचे (!lhsp || lhsp->varp() != retVarp) पटिनिवत्त;
            retExprp = assignp->rhsp();
            ++assign_गणनम्;
        });
        // Simple function: initializer + return (2) or just return (1)
        सचे (assign_गणनम् > 2 || !retExprp) पटिनिवत्त nullptr;
        पटिनिवत्त retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    शून्य propagateUser1InlineRecurse(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (VN_IS(nodep, ग्रन्थिः_Var_Ref)) {
            nodep->user1(VN_AS(nodep, ग्रन्थिः_Var_Ref)->varp()->rand().isRandomizable());
            पटिनिवत्त;
        }
        सचे (VN_IS(nodep, Const)) {
            nodep->user1(मिच्छा);
            पटिनिवत्त;
        }
        पूर्णाङ्क anyChild = मिच्छा;  // Used as bool
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = VN_CAST(nodep->op1p(), ग्रन्थिः_व्यञ्जकम्)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = VN_CAST(nodep->op2p(), ग्रन्थिः_व्यञ्जकम्)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = VN_CAST(nodep->op3p(), ग्रन्थिः_व्यञ्जकम्)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = VN_CAST(nodep->op4p(), ग्रन्थिः_व्यञ्जकम्)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        nodep->user1(anyChild);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    स्थिर Ast_ग्रन्थिः* prependDistPreamble(AstConstraintForeach* nodep, Ast_ग्रन्थिः* bodyp) {
        Ast_ग्रन्थिः* ध्रुव preamblep = nodep->user3p();
        सचे (!preamblep) पटिनिवत्त bodyp;
        nodep->user3p(nullptr);
        preamblep->addNext(bodyp);
        पटिनिवत्त preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    तार्किक referencesForeachIdx(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {
        सचे (m_foreachIdxVars.empty()) पटिनिवत्त मिच्छा;
        पटिनिवत्त nodep->exists(
            [&](ध्रुव AstVarRef* refp) { पटिनिवत्त m_foreachIdxVars.गणनम्(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    शून्य addRegistrationStmt(Ast_ग्रन्थिः* ध्रुव stmtp, Ast_ग्रन्थिः_FTask* ध्रुव initTaskp) {
        सचे (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->addBodyp(stmtp);
        } अञ्ञथा {
            initTaskp->addStmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_पङ्क्तिः* ध्रुव fl, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrExprp,
                                       Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxExprp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argsp = Ast_ग्रन्थिः::addNext(arrExprp, idxExprp);
        AstSFormatF* ध्रुव formatp = नूतन AstSFormatF{fl, "(select %s %s)", मिच्छा, argsp};
        पटिनिवत्त formatp;
    }

    शून्य setRandMode(AstVar* ध्रुव varp, AstMemberSel* ध्रुव memberselp,
                     ध्रुव std::string& smtName, ध्रुव RandomizeMode& randMode,
                     Ast_ग्रन्थिः_FTask* ध्रुव initTaskp) {
        Ast_ग्रन्थिः_Module* ध्रुव varClassp = VN_AS(varp->user2p(), ग्रन्थिः_Module);
        AstVar* ध्रुव subRandModeVarp = getRandModeVarFromClass(varClassp);
        सचे (subRandModeVarp) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव parentAccessp = memberselp->fromp()->clone_वृक्षः(मिच्छा);
            AstMemberSel* ध्रुव randModeSelp
                = नूतन AstMemberSel{varp->fileline(), parentAccessp, subRandModeVarp};
            randModeSelp->dtypep(subRandModeVarp->dtypep());
            AstCMethodHard* ध्रुव atp
                = नूतन AstCMethodHard{varp->fileline(), randModeSelp, VCMethod::ARRAY_AT,
                                     नूतन AstConst{varp->fileline(), randMode.index}};
            atp->dtypeSetUInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ध्रुव enablep = नूतन AstCMethodHard{
                varp->fileline(),
                नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtypeSetVoid();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : नूतन AstSFormatF{varp->fileline(), smtName, मिच्छा, nullptr};
            nameExprp->dtypep(varp->dtypep());
            enablep->addPinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ध्रुव disablep = नूतन AstCMethodHard{
                varp->fileline(),
                नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_SET_VAR_DISABLED};
            disablep->dtypeSetVoid();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disnp = nameExprp->clone_वृक्षः(मिच्छा);
            disablep->addPinsp(disnp);
            AstIf* ध्रुव ifp
                = नूतन AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            addRegistrationStmt(ifp, initTaskp);
        }
    }

    शून्य markRandc(AstVar* ध्रुव varp, ध्रुव std::string& smtName, Ast_ग्रन्थिः_FTask* ध्रुव initTaskp) {
        AstCMethodHard* ध्रुव markp = नूतन AstCMethodHard{
            varp->fileline(),
            नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtypeSetVoid();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : नूतन AstSFormatF{varp->fileline(), smtName, मिच्छा, nullptr};
        nameExprp->dtypep(varp->dtypep());
        markp->addPinsp(nameExprp);
        addRegistrationStmt(markp->makeStmt(), initTaskp);
    }

    Ast_ग्रन्थिः_Module* getLeftmostVarModulep(AstMemberSel* ध्रुव memberselp,
                                         AstVar* ध्रुव varp) ध्रुव {
        सचे (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_ग्रन्थिः* वामः_Most_Nodep = memberselp->fromp();
            याव (AstMemberSel* ध्रुव nestedMemberSelp = VN_CAST(वामः_Most_Nodep, MemberSel)) {
                वामः_Most_Nodep = nestedMemberSelp->fromp();
            }
            सचे (Ast_ग्रन्थिः_Var_Ref* ध्रुव मूलम्_Var_Refp = VN_CAST(वामः_Most_Nodep, ग्रन्थिः_Var_Ref)) {
                पटिनिवत्त VN_AS(मूलम्_Var_Refp->varp()->user2p(), ग्रन्थिः_Module);
            }
            पटिनिवत्त VN_AS(memberselp->user2p(), ग्रन्थिः_Module);
        }
        पटिनिवत्त VN_AS(varp->user2p(), ग्रन्थिः_Module);
    }

    Ast_ग्रन्थिः_FTask* getInitTaskp(AstVar* ध्रुव varp, ध्रुव तार्किक memberselOrStructSel,
                               Ast_ग्रन्थिः_Module* ध्रुव classp) ध्रुव {
        सचे (m_inlineInitTaskp) पटिनिवत्त m_inlineInitTaskp;
        varp->user3(सच्च);
        सचे (memberselOrStructSel) {
            सचे (Ast_ग्रन्थिः_FTask* ध्रुव initTaskp
                = VN_AS(m_memberMap.findMember(classp, "randomize"), ग्रन्थिः_FTask)) {
                पटिनिवत्त initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            सचे (m_memberselInitTaskp) पटिनिवत्त m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } अञ्ञथा {
            Ast_ग्रन्थिः_FTask* ध्रुव initTaskp
                = VN_AS(m_memberMap.findMember(classp, "new"), ग्रन्थिः_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            पटिनिवत्त initTaskp;
        }
        पटिनिवत्त nullptr;
    }

    शून्य createWriteVarForClassFields(AstClass* ध्रुव elemClassp, ध्रुव std::string& smtName,
                                      AstVar* ध्रुव iterVarp, ध्रुव तार्किक isUnpackedClassRefArray,
                                      Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep, AstVar* ध्रुव varp,
                                      AstIf* ध्रुव ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ध्रुव {
        File_पङ्क्तिः* ध्रुव fl = varp->fileline();
        Ast_ग्रन्थिः_Module* ध्रुव varClassp = VN_AS(varp->user2p(), ग्रन्थिः_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr पूर्णाङ्क idxWidth = 32;
        ध्रुव पूर्णाङ्क fmtWidth = VL_शब्दाः_I(idxWidth) * 8;

        अत्थाय (ध्रुव AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            अत्थाय (Ast_ग्रन्थिः* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ध्रुव memberVarp = VN_CAST(mnodep, Var);
                सचे (!memberVarp || !memberVarp->rand().isRandomizable()) अनुवर्त;
                Ast_ग्रन्थिः_DType* ध्रुव memberDtp = memberVarp->dtypep()->skipRefp();
                सचे (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) अनुवर्त;
                ध्रुव पूर्णाङ्क memberWidth = memberDtp->width();

                AstCStmt* ध्रुव fmtp = नूतन AstCStmt{fl};
                सचे (isUnpackedClassRefArray) {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } अञ्ञथा {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::to_string(fmtWidth) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->add(नूतन AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->add(");\n");
                ifNonNullp->addThensp(fmtp);

                AstVarRef* ध्रुव arrayWrRefp = नूतन AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_ग्रन्थिः_व्यञ्जकम्* atWritep;
                सचे (isUnpackedClassRefArray) {
                    atWritep = नूतन AstArraySel{fl, arrayWrRefp,
                                               नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
                } अञ्ञथा {
                    atWritep = नूतन AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* ध्रुव memberSelp = नूतन AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* ध्रुव writeVarp
                    = नूतन AstCMethodHard{fl,
                                         नूतन AstVarRef{fl, VN_AS(m_genp->user2p(), ग्रन्थिः_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtypeSetVoid();
                writeVarp->addPinsp(memberSelp);
                writeVarp->addPinsp(
                    नूतन AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(memberWidth)});
                AstCExpr* ध्रुव nameRefp = नूतन AstCExpr{fl, AstCExpr::Pure{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->addPinsp(nameRefp);
                writeVarp->addPinsp(नूतन AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->addThensp(writeVarp->makeStmt());
            }
        }
    }

    शून्य createSolverArrayHandle(AstVar* ध्रुव varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep,
                                 ध्रुव तार्किक isUnpackedClassRefArray,
                                 ध्रुव std::string& smtName) ध्रुव {
        File_पङ्क्तिः* ध्रुव fl = varp->fileline();
        AstClass* ध्रुव elemClassp = elemClassRefDtp->classp();
        Ast_ग्रन्थिः_Module* ध्रुव varClassp = VN_AS(varp->user2p(), ग्रन्थिः_Module);

        AstVar* ध्रुव iterVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__Vi"),
                                            varp->findUInt32DType()};
        iterVarp->funcLocal(सच्च);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ग्रन्थिः* ध्रुव stmtsp
            = नूतन AstAssign{fl, नूतन AstVarRef{fl, iterVarp, VAccess::WRITE}, नूतन AstConst{fl, 0}};

        Ast_ग्रन्थिः_व्यञ्जकम्* sizep;
        सचे (isUnpackedClassRefArray) {
            ध्रुव पूर्णाङ्क arraySize
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = नूतन AstConst{fl, static_cast<uint32_t>(arraySize)};
        } अञ्ञथा {
            AstVarRef* ध्रुव arraySizeRefp = नूतन AstVarRef{fl, varClassp, varp, VAccess::READ};
            arraySizeRefp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ध्रुव dynSizep
                = नूतन AstCMethodHard{fl, arraySizeRefp, VCMethod::DYN_SIZE, nullptr};
            dynSizep->dtypeSetUInt32();
            sizep = dynSizep;
        }

        AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(नूतन AstLoopTest{
            fl, loopp, नूतन AstLt{fl, नूतन AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ध्रुव arrayReadRefp = नूतन AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_ग्रन्थिः_व्यञ्जकम्* atReadp;
        सचे (isUnpackedClassRefArray) {
            atReadp
                = नूतन AstArraySel{fl, arrayReadRefp, नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
        } अञ्ञथा {
            atReadp = नूतन AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ध्रुव ifNonNullp
            = नूतन AstIf{fl, नूतन AstNeq{fl, atReadp, नूतन AstConst{fl, AstConst::Null{}}}, nullptr};
        loopp->addStmtsp(ifNonNullp);

        AstCStmt* ध्रुव bufDeclp = नूतन AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->addThensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->addStmtsp(नूतन AstAssign{
            fl, नूतन AstVarRef{fl, iterVarp, VAccess::WRITE},
            नूतन AstAdd{fl, नूतन AstConst{fl, 1}, नूतन AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(सच्च);
        Ast_ग्रन्थिः_FTask* initTaskp = m_inlineInitTaskp;
        सचे (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.findMember(varClassp, "new"), ग्रन्थिः_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->addStmtsp(stmtsp);
    }

    शून्य createSolverVarHandle(AstVar* ध्रुव varp, ध्रुव तार्किक structSelOrCMeth,
                               ध्रुव तार्किक isGlobalConstrained, ध्रुव RandomizeMode randMode,
                               AstMemberSel* ध्रुव memberselp, ध्रुव std::string& smtName,
                               Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep, Ast_ग्रन्थिः_Module* ध्रुव classp,
                               Ast_ग्रन्थिः_FTask* ध्रुव initTaskp) {
        uint32_t unpackedDims = 0;
        सचे (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(मिच्छा).second;
        }
        सचे (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ध्रुव methodp = नूतन AstCMethodHard{
            varp->fileline(),
            नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();
        सचे (memberselp) {
            methodp->addPinsp(memberselp);
        } अञ्ञथा {
            AstVarRef* ध्रुव varRefp
                = नूतन AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->addPinsp(varRefp);
        }
        ध्रुव size_t width = arrayElementDTypep(varp->dtypep())->width();
        methodp->addPinsp(नूतन AstConst{varp->fileline(), AstConst::Unsized64{}, width});
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varnamep = नूतन AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                   "\"" + smtName + "\"", varp->width()};
        varnamep->dtypep(varp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(
            नूतन AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        सचे (randMode.usesMode && !(isGlobalConstrained && memberselp)) {
            methodp->addPinsp(
                नूतन AstConst{varp->fileline(), AstConst::Unsized64{}, randMode.index});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->addStmtsp(methodp->makeStmt());
        सचे (varp->lifetime().isStatic() && randMode.usesMode) {
            AstCMethodHard* ध्रुव markp = नूतन AstCMethodHard{
                varp->fileline(),
                नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव namep = नूतन AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                    "\"" + smtName + "\"", varp->width()};
            namep->dtypep(varp->dtypep());
            markp->addPinsp(namep);
            markp->dtypeSetVoid();
            initTaskp->addStmtsp(markp->makeStmt());
        }
        सचे (isGlobalConstrained && memberselp && randMode.usesMode) {
            setRandMode(varp, memberselp, smtName, randMode, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        सचे (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createVarExprp(Ast_ग्रन्थिः_Var_Ref* nodep, ध्रुव RandomizeMode randMode,
                                ध्रुव std::string& smtName, ध्रुव तार्किक isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ध्रुव varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = नूतन AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, मिच्छा, nullptr};
        सचे (!randMode.usesMode) {
            // Global constraints keep nodep alive for write_var processing
            सचे (!isGlobalConstrained) VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_ग्रन्थिः_व्यञ्जकम्* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_वृक्षः(मिच्छा)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_ग्रन्थिः_व्यञ्जकम्* randModeAccess;
        ध्रुव तार्किक varIsStatic = varp->lifetime().isStatic();
        AstClass* ध्रुव varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : nullptr;
        AstVar* ध्रुव ownerStaticRandModeVarp
            = varOwningClassp ? findStaticRandModeVarMember(varOwningClassp) : nullptr;
        सचे (ownerStaticRandModeVarp) {
            randModeAccess = नूतन AstVarRef{varp->fileline(),
                                           VN_AS(ownerStaticRandModeVarp->user2p(), ग्रन्थिः_Module),
                                           ownerStaticRandModeVarp, VAccess::READ};
        } अञ्ञथा सचे (memberselp) {
            Ast_ग्रन्थिः_Module* ध्रुव varClassp = VN_AS(varp->user2p(), ग्रन्थिः_Module);
            AstVar* ध्रुव effectiveRandModeVarp = getRandModeVarFromClass(varClassp);
            सचे (effectiveRandModeVarp) {
                // Member's class has randmode, use it
                Ast_ग्रन्थिः_व्यञ्जकम्* parentAccess = memberselp->fromp()->clone_वृक्षः(मिच्छा);
                AstMemberSel* randModeSel
                    = नूतन AstMemberSel{varp->fileline(), parentAccess, effectiveRandModeVarp};
                randModeSel->dtypep(effectiveRandModeVarp->dtypep());
                randModeAccess = randModeSel;
            } अञ्ञथा {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
                randModeAccess
                    = नूतन AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), ग्रन्थिः_Module),
                                    m_randModeVarp, VAccess::READ};
            }
        } अञ्ञथा {
            UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
            randModeAccess
                = नूतन AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), ग्रन्थिः_Module),
                                m_randModeVarp, VAccess::READ};
        }

        AstCMethodHard* ध्रुव atp
            = नूतन AstCMethodHard{nodep->fileline(), randModeAccess, VCMethod::ARRAY_AT,
                                 नूतन AstConst{nodep->fileline(), randMode.index}};
        atp->dtypeSetUInt32();
        exprp = नूतन AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(सच्च);  // Mark as formatted
        पटिनिवत्त exprp;
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        सचे (varp->user4p()) {
            तार्किक isSizeRef = मिच्छा;
            सचे (AstCMethodHard* ध्रुव methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                सचे (methodp->method() == VCMethod::ASSOC_SIZE
                    || methodp->method() == VCMethod::DYN_SIZE) {
                    isSizeRef = सच्च;
                }
            }
            सचे (!isSizeRef && m_sizeConstrainedArraysp) { m_sizeConstrainedArraysp->निवेशय(varp); }
        }

        // Check if this variable is marked as globally constrained
        ध्रुव तार्किक isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = nullptr;
        तार्किक structSelOrCMeth = मिच्छा;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_ग्रन्थिः_DType* selectedDtypep = nullptr;
        std::string smtName;
        सचे (m_nestedAccess) {
            m_nestedAccess->addVarNamePart(
                नूतन AstSFormatF{varp->fileline(), nodep->name(), मिच्छा, nullptr}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_वृक्षः();
        } अञ्ञथा सचे (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_ग्रन्थिः* शिखरम्_Member_Sel = nodep->backp();
            याव (VN_IS(शिखरम्_Member_Sel->backp(), MemberSel)) {
                शिखरम्_Member_Sel = शिखरम्_Member_Sel->backp();
            }
            memberselp = VN_AS(शिखरम्_Member_Sel, MemberSel)->clone_वृक्षः(मिच्छा);
            smtName = buildMemberPath(memberselp);
        } अञ्ञथा सचे (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_ग्रन्थिः* शिखरम्_Nodep = nodep->backp();
            याव (VN_IS(शिखरम्_Nodep->backp(), StructSel) || VN_IS(शिखरम्_Nodep->backp(), CMethodHard)) {
                शिखरम्_Nodep = शिखरम्_Nodep->backp();
            }

            सचे (VN_IS(शिखरम्_Nodep, StructSel) || VN_IS(शिखरम्_Nodep, CMethodHard)) {
                structSelOrCMeth = सच्च;
                selectedDtypep = VN_AS(शिखरम्_Nodep, ग्रन्थिः_व्यञ्जकम्)->dtypep();
            }

            memberselp = VN_CAST(शिखरम्_Nodep, MemberSel);
            सचे (memberselp) smtName = buildMemberPath(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            सचे (smtName == "") smtName = nodep->name();
        } अञ्ञथा {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        सचे (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        सचे (dtypeContainsReal(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            पटिनिवत्त;
        }
        Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep = nodep->classOrPackagep();
        ध्रुव RandomizeMode randMode = {.asUQuad = varp->user1()};
        सचे (!randMode.usesMode && editFormat(nodep)) पटिनिवत्त;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skipRefToEnump(), EnumDType);
        सचे (enumDtp
            && (std::find(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.end(), varp)
                == m_rangeConstrainedEnums.end())) {
            AstVar* genVarp = m_genp;
            सचे (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_ग्रन्थिः_व्यञ्जकम्* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_ग्रन्थिः_FTask* targetTaskp = m_inlineInitTaskp;
            सचे (!targetTaskp) targetTaskp = getCreateConstraintSetupFunc(m_classp, m_memberMap);
            UASSERT_OBJ(targetTaskp, nodep, "No function to inline enum range constraint into");
            targetTaskp->addStmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.प्रक्षिप_back(varp);
        }

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp
            = createVarExprp(nodep, randMode, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ध्रुव तार्किक alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.गणनम्(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.गणनम्(smtName) > 0
                                    : memberselp          ? m_writtenVars.गणनम्(smtName) > 0
                                                          : varp->user3();
        ध्रुव तार्किक shouldWriteVar = !alreadyWritten;
        सचे (shouldWriteVar) {
            // Track this variable path as written
            सचे (m_inlineInitTaskp)
                m_inlineWrittenVars.निवेशय(smtName);
            अञ्ञथा सचे (isGlobalConstrained || memberselp)
                m_writtenVars.निवेशय(smtName);
            // For global constraints, delete nodep after processing
            सचे (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);

            तार्किक isClassRefArray = मिच्छा;
            तार्किक isUnpackedClassRefArray = मिच्छा;
            AstClassRefDType* elemClassRefDtp = nullptr;
            {
                Ast_ग्रन्थिः_DType* varDtp = varp->dtypep()->skipRefp();
                सचे (varDtp->isNonPackedArray()) {
                    Ast_ग्रन्थिः_DType* ध्रुव elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    सचे (elemClassRefDtp) {
                        isClassRefArray = सच्च;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_ग्रन्थिः_Module* ध्रुव classp = getLeftmostVarModulep(memberselp, varp);
            Ast_ग्रन्थिः_FTask* ध्रुव initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            सचे (m_nestedAccess) {
                Ast_Stmt_व्यञ्जकम्* ध्रुव writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                addRegistrationStmt(writeVarStmtp, initTaskp);
                सचे (isGlobalConstrained && memberselp && randMode.usesMode) {
                    setRandMode(varp, memberselp, smtName, randMode, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                सचे (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } अञ्ञथा सचे (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } अञ्ञथा {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, randMode,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } अञ्ञथा {
            // Variable already written, clean up cloned memberselp if any
            सचे (memberselp) VL_DO_DANGLING(memberselp->delete_वृक्षः(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            सचे (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
        सचे (m_nestedAccess) { VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_ग्रन्थिः_व्यञ्जकम्* build_गणनम्_Ones_Expansion(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* argp) {
        ध्रुव V3_सङ्ख्या अङ्कः_One{fl, argp->width(), 1};
        Ast_ग्रन्थिः_व्यञ्जकम्* sump = नूतन AstAnd{fl, argp, नूतन AstConst{fl, अङ्कः_One}};
        sump->user1(सच्च);
        अत्थाय (पूर्णाङ्क i = 1; i < argp->width(); i++) {
            V3_सङ्ख्या अङ्कः_Bit_Mask{fl, argp->width(), 0};
            अङ्कः_Bit_Mask.setBit(i, 1);
            AstAnd* ध्रुव andp
                = नूतन AstAnd{fl, argp->clone_वृक्षः_Pure(मिच्छा), नूतन AstConst{fl, अङ्कः_Bit_Mask}};
            andp->user1(सच्च);
            AstShiftR* ध्रुव shiftp
                = नूतन AstShiftR{fl, andp,
                                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, argp->width(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(सच्च);
            sump = नूतन AstAdd{fl, sump, shiftp};
            sump->user1(सच्च);
        }
        पटिनिवत्त sump;
    }

    शून्य visit(Ast_गणनम्_Ones* nodep) override {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* sump = build_गणनम्_Ones_Expansion(fl, argp);
        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(सच्च);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(sump);
    }
    शून्य visit(AstRedXor* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlinkFrBack();

        Ast_ग्रन्थिः_व्यञ्जकम्* redxorp = नूतन AstSel{fl, argp, 0, 1};
        redxorp->user1(सच्च);
        अत्थाय (पूर्णाङ्क i = 1; i < argp->width(); i++) {
            AstSel* ध्रुव selp = नूतन AstSel{fl, argp->clone_वृक्षः_Pure(मिच्छा), i, 1};
            selp->user1(सच्च);

            redxorp = नूतन AstXor{fl, redxorp, selp};
            redxorp->user1(सच्च);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(redxorp);
    }
    शून्य visit(AstRedAnd* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        // Convert to (~x == 0)
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlinkFrBack();
        ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, argp->width(), 0};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव negp = नूतन AstNot{fl, argp};
        negp->user1(सच्च);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = नूतन AstEq{fl, negp, नूतन AstConst{fl, अङ्कः_Zero}};
        eqp->user1(सच्च);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(eqp);
    }
    शून्य visit(AstRedOr* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        // Convert to (x != 0)
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlinkFrBack();
        ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, argp->width(), 0};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव neqp = नूतन AstNeq{fl, argp, नूतन AstConst{fl, अङ्कः_Zero}};
        neqp->user1(सच्च);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(neqp);
    }
    शून्य visit(AstOneHot* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlinkFrBack();
        ध्रुव पूर्णाङ्क w = argp->width();

        ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, w, 0};
        AstNeq* ध्रुव neZerop
            = नूतन AstNeq{fl, argp->clone_वृक्षः_Pure(मिच्छा), नूतन AstConst{fl, अङ्कः_Zero}};
        neZerop->user1(सच्च);

        ध्रुव V3_सङ्ख्या अङ्कः_One{fl, w, 1};
        AstSub* ध्रुव subp = नूतन AstSub{fl, argp->clone_वृक्षः_Pure(मिच्छा), नूतन AstConst{fl, अङ्कः_One}};
        subp->user1(सच्च);

        AstAnd* ध्रुव andp = नूतन AstAnd{fl, argp, subp};
        andp->user1(सच्च);

        ध्रुव V3_सङ्ख्या अङ्कः_Zero2{fl, w, 0};
        AstEq* ध्रुव eqZerop = नूतन AstEq{fl, andp, नूतन AstConst{fl, अङ्कः_Zero2}};
        eqZerop->user1(सच्च);

        AstLogAnd* ध्रुव resultp = नूतन AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(सच्च);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(resultp);
    }
    शून्य visit(AstOneHot0* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        // $onehot0(x) = (x & (x-1)) == 0
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlinkFrBack();
        ध्रुव पूर्णाङ्क w = argp->width();

        ध्रुव V3_सङ्ख्या अङ्कः_One{fl, w, 1};
        AstSub* ध्रुव subp = नूतन AstSub{fl, argp->clone_वृक्षः_Pure(मिच्छा), नूतन AstConst{fl, अङ्कः_One}};
        subp->dtypeFrom(argp);
        subp->user1(सच्च);

        AstAnd* ध्रुव andp = नूतन AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(सच्च);

        ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, w, 0};
        AstEq* ध्रुव eqp = नूतन AstEq{fl, andp, नूतन AstConst{fl, अङ्कः_Zero}};
        eqp->user1(सच्च);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(eqp);
    }
    शून्य visit(Ast_गणनम्_Bits* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

        तार्किक गणनम्_Ones = मिच्छा;
        तार्किक गणनम्_Zeros = मिच्छा;
        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ध्रुव AstConst* ध्रुव cp = VN_CAST(ctrlp, Const);
            सचे (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ध्रुव zerop
                    = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, nodep->width(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                पटिनिवत्त;
            }
            सचे (cp->अङ्कः().bitIs1(0) && !गणनम्_Ones)
                गणनम्_Ones = सच्च;
            अञ्ञथा सचे (cp->अङ्कः().bitIs0(0) && !गणनम्_Zeros)
                गणनम्_Zeros = सच्च;
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlinkFrBack();
        ध्रुव पूर्णाङ्क argWidth = argp->width();
        Ast_ग्रन्थिः_व्यञ्जकम्* sump = nullptr;
        सचे (गणनम्_Ones && गणनम्_Zeros) {
            // ones + zeros = width for 2-state types
            sump = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, nodep->width(), (uint32_t)argWidth};
            VL_DO_DANGLING(argp->delete_वृक्षः(), argp);
        } अञ्ञथा सचे (गणनम्_Ones) {
            sump = build_गणनम्_Ones_Expansion(fl, argp);
        } अञ्ञथा सचे (गणनम्_Zeros) {
            // width - countones(x)
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव onesCountp = build_गणनम्_Ones_Expansion(fl, argp);
            ध्रुव V3_सङ्ख्या widthVal{nodep, onesCountp->width(), (uint32_t)argWidth};
            sump = नूतन AstSub{fl, नूतन AstConst{fl, widthVal}, onesCountp};
            sump->user1(सच्च);
        } अञ्ञथा {
            sump = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, nodep->width(), 0U};
            VL_DO_DANGLING(argp->delete_वृक्षः(), argp);
        }

        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(सच्च);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(sump);
    }
    शून्य visit(AstCLog2* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlinkFrBack();
        ध्रुव पूर्णाङ्क argWidth = argp->width();
        ध्रुव पूर्णाङ्क resultWidth = nodep->width();

        Ast_ग्रन्थिः_व्यञ्जकम्* resultp
            = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, resultWidth, (uint32_t)argWidth};

        अत्थाय (पूर्णाङ्क k = argWidth - 1; k >= 0; k--) {
            V3_सङ्ख्या threshold{fl, argWidth, 0};
            सचे (k < 32)
                threshold.setLong(1ULL << k);
            अञ्ञथा
                threshold.setBit(k, 1);

            AstLte* ध्रुव ltep
                = नूतन AstLte{fl, argp->clone_वृक्षः_Pure(मिच्छा), नूतन AstConst{fl, threshold}};
            ltep->user1(सच्च);

            AstConst* ध्रुव valuep
                = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, resultWidth, (uint32_t)k};

            resultp = नूतन AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(सच्च);
        }

        VL_DO_DANGLING(argp->delete_वृक्षः(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(resultp);
    }
    शून्य visit(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ध्रुव zerop = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    शून्य handlePow(Ast_ग्रन्थिः_Biop* nodep) {
        सचे (AstConst* ध्रुव exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव basep = nodep->lhsp();
            ध्रुव V3_सङ्ख्या अङ्कः_One{nodep, basep->width(), 1};
            Ast_ग्रन्थिः_व्यञ्जकम्* powerp = नूतन AstConst{fl, अङ्कः_One};
            ध्रुव तार्किक baseSigned = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ध्रुव int32_t exponent = baseSigned ? exponentp->toSInt() : exponentp->toUInt();
            सचे (exponent > 0) {
                अत्थाय (int32_t i = 0; i < exponent; i++) {
                    सचे (baseSigned) {
                        powerp = नूतन AstMulS{fl, powerp, basep->clone_वृक्षः_Pure(मिच्छा)};
                    } अञ्ञथा {
                        powerp = नूतन AstMul{fl, powerp, basep->clone_वृक्षः_Pure(मिच्छा)};
                    }
                    powerp->user1(सच्च);
                }
            } अञ्ञथा सचे (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                सचे (baseSigned) {
                    powerp = नूतन AstDivS{fl, powerp, basep->clone_वृक्षः_Pure(मिच्छा)};
                    powerp->user1(सच्च);
                    powerp = नूतन AstDivS{fl, powerp, basep->clone_वृक्षः_Pure(मिच्छा)};
                    powerp->user1(सच्च);
                } अञ्ञथा {
                    powerp = नूतन AstDiv{fl, powerp, basep->clone_वृक्षः_Pure(मिच्छा)};
                    powerp->user1(सच्च);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            iterate(powerp);
        } अञ्ञथा सचे (ध्रुव AstConst* ध्रुव basep = VN_CAST(nodep->lhsp(), Const)) {
            सचे (basep->अङ्कः().toUInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
                ध्रुव पूर्णाङ्क width = nodep->width();
                ध्रुव तार्किक rhsDependent = rhsp->user1();
                AstConst* ध्रुव onep = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, width, 1};
                AstShiftL* ध्रुव shiftp = नूतन AstShiftL{fl, onep, rhsp, width};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_ग्रन्थिः_व्यञ्जकम्* resultp = shiftp;
                सचे (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nClonep = rhsp->clone_वृक्षः_Pure(मिच्छा);
                    nClonep->user1(rhsDependent);
                    AstConst* ध्रुव zeroNp
                        = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, rhsp->width(), 0};
                    AstGteS* ध्रुव condp = नूतन AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ध्रुव zeroRp = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, width, 0};
                    resultp = नूतन AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                iterate(resultp);
            } अञ्ञथा {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } अञ्ञथा {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    शून्य visit(AstPow* nodep) override { handlePow(nodep); }
    शून्य visit(AstPowSS* nodep) override { handlePow(nodep); }
    शून्य visit(AstPowSU* nodep) override { handlePow(nodep); }
    शून्य visit(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    शून्य handleShift(Ast_ग्रन्थिः_Biop* nodep) {
        सचे (editFormat(nodep)) पटिनिवत्त;
        ध्रुव पूर्णाङ्क lhsWidth = nodep->lhsp()->width();
        ध्रुव पूर्णाङ्क rhsWidth = nodep->rhsp()->width();
        सचे (rhsWidth < lhsWidth) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlinkFrBack();
            AstExtend* ध्रुव extendp = नूतन AstExtend{fl, rhsp, lhsWidth};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(सच्च);
            nodep->rhsp(extendp);
        } अञ्ञथा सचे (rhsWidth > lhsWidth) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->dtypep();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव extendp
                = VN_IS(nodep, ShiftRS)
                      ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{fl, lhsp, rhsWidth})
                      : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtend{fl, lhsp, rhsWidth});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(सच्च);
            nodep->lhsp(extendp);
            nodep->dtypeSetLogicSized(rhsWidth, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            AstSel* ध्रुव selp = नूतन AstSel{fl, nodep, 0, lhsWidth};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            पटिनिवत्त;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    शून्य visit(AstShiftL* nodep) override { handleShift(nodep); }
    शून्य visit(AstShiftR* nodep) override { handleShift(nodep); }
    शून्य visit(AstShiftRS* nodep) override { handleShift(nodep); }
    शून्य visit(Ast_ग्रन्थिः_Biop* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    शून्य visit(Ast_ग्रन्थिः_Uniop* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        editSMT(nodep, nodep->lhsp());
    }
    शून्य visit(Ast_ग्रन्थिः_Triop* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    शून्य visit(AstCond* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        सचे (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtypeSetString();
            पटिनिवत्त;
        }
        // Fall back to "(ite cond then else)"
        visit(static_cast<Ast_ग्रन्थिः_Triop*>(nodep));
    }
    शून्य visit(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        सचे (editFormat(nodep)) पटिनिवत्त;
        editSMT(nodep, nodep->srcp());
    }
    शून्य visit(AstSel* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        VNRelinker handle;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव msbp = नूतन AstSFormatF{
            fl, "%1d", मिच्छा,
            नूतन AstAdd{fl, nodep->lsbp()->clone_वृक्षः_Pure(मिच्छा),
                       नूतन AstConst{fl, static_cast<uint32_t>(nodep->widthConst() - 1)}}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp
            = नूतन AstSFormatF{fl, "%1d", मिच्छा, nodep->lsbp()->unlinkFrBack(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    शून्य visit(AstStructSel* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        ध्रुव तार्किक outermost = !m_deferRandModeHoist;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = outermost ? nodep->clone_वृक्षः(मिच्छा) : nullptr;
        VL_RESTORER(m_structSel);
        m_structSel = सच्च;
        सचे (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = nodep->fromp();
            AstStructDType* ध्रुव structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            सचे (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } अञ्ञथा {
                structp->markConstrainedRand(सच्च);
            }
        }
        // Mark Random for structArray
        सचे (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ध्रुव structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            सचे (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } अञ्ञथा {
                structp->markConstrainedRand(सच्च);
            }
        }
        {
            VL_RESTORER(m_deferRandModeHoist);
            m_deferRandModeHoist = सच्च;
            iterateChildren(nodep);
        }
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstSFormatF* newp = nullptr;
        सचे (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = नूतन AstSFormatF{fl, "%s.%s." + nodep->name(), मिच्छा,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_वृक्षः_Pure(सच्च)};
            सचे (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } अञ्ञथा {
            newp = नूतन AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), मिच्छा,
                                   nullptr};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        सचे (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(origp), origp);
        }
    }
    शून्य addStringNamePart(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव indexp, ध्रुव std::string& fmt = "%32x") {
        सचे (m_nestedAccess) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitp = indexp->clone_वृक्षः_Pure(मिच्छा);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitFormatp = नूतन AstSFormatF{bitp->fileline(), fmt, मिच्छा, bitp};
            AstSFormatF* ध्रुव herep
                = नूतन AstSFormatF{indexp->fileline(), "%s.%s", मिच्छा, bitFormatp};
            m_nestedAccess->addVarNamePart(herep, indexp->name());
        }
    }
    शून्य visit(AstAssocSel* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->clone_वृक्षः(मिच्छा);
        AstSFormatF* newp = nullptr;
        सचे (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxp = नूतन AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), मिच्छा,
                                                      nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } अञ्ञथा सचे (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ध्रुव stringp = VN_AS(nodep->bitp(), CvtPackString);
            ध्रुव size_t stringSize = VN_AS(stringp->lhsp(), Const)->width();
            सचे (stringSize > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << stringSize << "bits, limit is 128 bits");
            }
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxp = नूतन AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), मिच्छा,
                                                      stringp->lhsp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } अञ्ञथा {
            सचे (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ध्रुव पूर्णाङ्क actual_width = nodep->bitp()->width();
                std::string fmt;
                // Normalize to standard bit width
                सचे (actual_width <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } अञ्ञथा सचे (actual_width <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } अञ्ञथा {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::to_string(VL_शब्दाः_I(actual_width) * 8) + "x";
                }
                सचे (m_nestedAccess) addStringNamePart(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxp
                    = नूतन AstSFormatF{fl, fmt, मिच्छा, nodep->bitp()->unlinkFrBack(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } अञ्ञथा {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                सचे (m_nestedAccess) m_nestedAccess->error();
            }
        }
        सचे (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        सचे (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(origp->delete_वृक्षः(), origp);
        }
    }
    शून्य rename_Array_व्यञ्जकम्(AstSFormatF* ध्रुव newp) ध्रुव {
        सचे (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    शून्य visit(AstArraySel* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        तार्किक indexIsRand = मिच्छा;
        nodep->bitp()->foreach([&](ध्रुव Ast_ग्रन्थिः_Var_Ref* vrefp) {
            सचे (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                indexIsRand = सच्च;
            }
        });
        सचे (m_nestedAccess) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitp = nodep->bitp()->clone_वृक्षः_Pure(मिच्छा);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitFormatp
                = indexIsRand ? नूतन AstSFormatF{bitp->fileline(), bitp->name(), मिच्छा, nullptr}
                              : नूतन AstSFormatF{bitp->fileline(), "%x", मिच्छा, bitp};
            AstSFormatF* ध्रुव herep
                = नूतन AstSFormatF{nodep->fileline(), "%s.%s", मिच्छा, bitFormatp};
            सचे (AstSel* ध्रुव selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->addVarNamePart(herep, selp->fromp()->name());
            } अञ्ञथा {
                m_nestedAccess->addVarNamePart(herep, bitp->name());
            }
            सचे (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_वृक्षः(), bitp);
                पटिनिवत्त;
            }
        }
        सचे (indexIsRand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_ग्रन्थिः_व्यञ्जकम्* indexp = nodep->bitp()->unlinkFrBack(&handle);
            सचे (indexp->width() < 32) {
                AstExtend* ध्रुव extendp = नूतन AstExtend{fl, indexp, 32};
                extendp->user1(सच्च);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ध्रुव newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_व्यञ्जकम्(newp);
        } अञ्ञथा {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->clone_वृक्षः(मिच्छा);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव indexp
                = नूतन AstSFormatF{fl, "#x%8x", मिच्छा, nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(indexp);
            AstSFormatF* ध्रुव newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_व्यञ्जकम्(newp);
            सचे (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
                VL_DO_DANGLING(origp->delete_वृक्षः(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    स्थिर तार्किक isRandModeGate(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ध्रुव AstCMethodHard* ध्रुव atp = VN_CAST(exprp, CMethodHard);
        सचे (!atp || atp->method() != VCMethod::ARRAY_AT) पटिनिवत्त मिच्छा;
        ध्रुव AstVar* varp = nullptr;
        सचे (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        सचे (!varp) पटिनिवत्त मिच्छा;
        ध्रुव AstClass* ध्रुव classp = VN_CAST(varp->user2p(), Class);
        पटिनिवत्त classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    तार्किक hoistRandModeOverSelectAndMember(AstSFormatF* newp, Ast_ग्रन्थिः_व्यञ्जकम्* origp) {
        सचे (m_deferRandModeHoist) पटिनिवत्त मिच्छा;
        // Only for selects yielding a non-array element (full chains)
        सचे (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) पटिनिवत्त मिच्छा;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::vector<AstSFormatF*> frames;
        AstCond* modeCondp = nullptr;
        अत्थाय (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firstp = curp->exprsp();
            frames.प्रक्षिप_back(curp);
            सचे (AstCond* ध्रुव condp = VN_CAST(firstp, Cond)) {
                सचे (isRandModeGate(condp->condp())) modeCondp = condp;
                भङ्ग;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        सचे (!modeCondp) पटिनिवत्त मिच्छा;
        File_पङ्क्तिः* ध्रुव fl = newp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव modep = modeCondp->condp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* activep = modeCondp->thenp()->unlinkFrBack();
        // Rebuild the select chain text around the SMT name, innermost first
        अत्थाय (auto it = frames.rbegin(); it != frames.rend(); ++it) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव remainingArgsp = VN_CAST((*it)->exprsp()->nextp(), ग्रन्थिः_व्यञ्जकम्);
            सचे (remainingArgsp) { remainingArgsp->unlinkFrBackWithNext(); }
            activep = नूतन AstSFormatF{fl, (*it)->name(), मिच्छा,
                                      Ast_ग्रन्थिः::addNext(activep, remainingArgsp)};
        }
        AstCond* ध्रुव hoistp = नूतन AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(सच्च);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(newp), newp);
        पटिनिवत्त सच्च;
    }
    शून्य visit(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        सचे (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            सचे (m_nestedAccess) {
                AstSFormatF* ध्रुव formatp = नूतन AstSFormatF{fl, nodep->name(), मिच्छा, nullptr};
                m_nestedAccess->addVarNamePart(नूतन AstSFormatF{fl, "%s.%s", मिच्छा, formatp},
                                               nodep->name());
            }
            Ast_ग्रन्थिः* मूलम्_ग्रन्थिः = nodep->fromp();
            याव (ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(मूलम्_ग्रन्थिः, MemberSel)) {
                मूलम्_ग्रन्थिः = selp->fromp();
            }

            Ast_ग्रन्थिः_Sel* arraySelp = VN_CAST(मूलम्_ग्रन्थिः, ArraySel);
            सचे (!arraySelp) arraySelp = VN_CAST(मूलम्_ग्रन्थिः, AssocSel);

            सचे (arraySelp) {
                Ast_ग्रन्थिः_DType* ध्रुव arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_ग्रन्थिः_DType* ध्रुव elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : nullptr;
                सचे (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ध्रुव तार्किक isSimple
                        = nodep->fromp() == मूलम्_ग्रन्थिः && VN_IS(arraySelp->fromp(), VarRef);
                    सचे (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = नूतन NestedAccessPath(nodep->clone_वृक्षः(मिच्छा),
                                                              &m_firstExpressionInsideIndexp);
                        AstSFormatF* ध्रुव formatp
                            = नूतन AstSFormatF{fl, nodep->name(), मिच्छा, nullptr};
                        m_nestedAccess->addVarNamePart(
                            नूतन AstSFormatF{fl, "%s.%s", मिच्छा, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = सच्च;
                    nodep->user1(सच्च);
                    arraySelp->user1(सच्च);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->clone_वृक्षः(मिच्छा);
                    iterateChildren(nodep);
                    AstSFormatF* ध्रुव newp = नूतन AstSFormatF{fl, "%s." + nodep->name(), मिच्छा,
                                                              nodep->fromp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    सचे (!hoistRandModeOverSelectAndMember(newp, origp))
                        VL_DO_DANGLING(origp->delete_वृक्षः(), origp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess);
                    पटिनिवत्त;
                }
            }
            // Check if the root variable participates in global constraints
            सचे (ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(मूलम्_ग्रन्थिः, VarRef)) {
                AstVar* ध्रुव constrainedVar = varRefp->varp();
                सचे (constrainedVar->globalConstrained()) {
                    तार्किक ध्रुव beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    सचे (beforeIterate) {
                        nodep->replaceWith(नूतन AstSFormatF{fl, "%s." + nodep->name(), मिच्छा,
                                                           nodep->fromp()->unlinkFrBack()});
                    } अञ्ञथा {
                        nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                    }
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    पटिनिवत्त;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        सचे (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), ग्रन्थिः_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ध्रुव varRefp
                = नूतन AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), ग्रन्थिः_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            visit(varRefp);
        } अञ्ञथा सचे (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->method() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ध्रुव cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_ग्रन्थिः_DType* ध्रुव arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_ग्रन्थिः_DType* ध्रुव elemDtp = arrayDtp->subDTypep()->skipRefp();
            सचे (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = सच्च;
                iterateChildren(nodep);
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                AstSFormatF* newp = nullptr;
                सचे (AstSFormatF* ध्रुव fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    सचे (fromp->name() == "%s.%s") {
                        newp = नूतन AstSFormatF{fl, "%s.%s." + nodep->name(), मिच्छा,
                                               fromp->exprsp()->clone_वृक्षः_Pure(सच्च)};
                    } अञ्ञथा {
                        newp = नूतन AstSFormatF{fl, fromp->name() + "." + nodep->name(), मिच्छा,
                                               nullptr};
                    }
                } अञ्ञथा {
                    newp = नूतन AstSFormatF{fl, nodep->name(), मिच्छा, nullptr};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                पटिनिवत्त;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        } अञ्ञथा सचे (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        } अञ्ञथा {
            editFormat(nodep);
        }
    }
    शून्य visit(AstSFormatF* nodep) override {}
    शून्य visit(Ast_Stmt_व्यञ्जकम्* nodep) override {}
    शून्य visit(AstCStmt* nodep) override {}
    शून्य visit(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    शून्य visit(AstConstraintIf* nodep) override {
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = nullptr;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenp = editSingle(fl, nodep->thensp());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elsep = editSingle(fl, nodep->elsesp());
        सचे (thenp && elsep) {
            newp = नूतन AstCond{fl, nodep->condp()->unlinkFrBack(), thenp, elsep};
        } अञ्ञथा सचे (thenp) {
            newp = नूतन AstLogIf{fl, nodep->condp()->unlinkFrBack(), thenp};
        } अञ्ञथा सचे (elsep) {
            newp = नूतन AstLogIf{fl, नूतन AstNot{fl, nodep->condp()->unlinkFrBack()}, elsep};
        }
        सचे (newp) {
            newp->dtypeSetBit();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(सच्च);  // Assume result-dependent
            nodep->replaceWith(नूतन Ast_Constraint_व्यञ्जकम्{fl, newp});
        } अञ्ञथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    शून्य visit(AstBegin* nodep) override {}
    शून्य visit(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

        सचे (!nodep->bodyp()) {
            nodep->unlinkFrBack();
        } अञ्ञथा सचे (m_wantSingle) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(itemp);
            cstmtp->add(";");
            Ast_ग्रन्थिः* ध्रुव bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(नूतन AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     नूतन AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp},
                                     सच्च});
            cexprp->add("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(नूतन AstSFormatF{fl, "%s", मिच्छा, cexprp});
        } अञ्ञथा {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            अत्थाय (Ast_ग्रन्थिः* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(elemp, Var)) m_foreachIdxVars.निवेशय(varp);
            }
            iterateAndNextNull(nodep->bodyp());
            Ast_ग्रन्थिः* ध्रुव bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlinkFrBackWithNext());
            nodep->replaceWith(
                नूतन AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                             नूतन AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp}, सच्च});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(m_genp->user2p(), ग्रन्थिः_Module);

        अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsTestp = build_Solve_Before_Name_व्यञ्जकम्(fl, lhsp);
            सचे (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                अनुवर्त;
            }
            VL_DO_DANGLING(lhsTestp->delete_वृक्षः(), lhsTestp);
            अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsNamep = build_Solve_Before_Name_व्यञ्जकम्(fl, rhsp);
                सचे (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    अनुवर्त;
                }
                AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                    fl, नूतन AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtypeSetVoid();
                callp->addPinsp(build_Solve_Before_Name_व्यञ्जकम्(fl, lhsp));
                callp->addPinsp(rhsNamep);
                nodep->addHereThisAsNext(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
    }
    शून्य visit(AstConstraintUnique* nodep) override {
        Ast_ग्रन्थिः_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_Module* genModp = nullptr;

        सचे (m_classp) {
            initTaskp = VN_AS(m_memberMap.findMember(m_classp, "new"), ग्रन्थिः_FTask);
            genVarp = getRandomGenerator(m_classp);
        } अञ्ञथा {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        सचे (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            पटिनिवत्त;
        }
        सचे (m_classp) {
            genModp = VN_AS(genVarp->user2p(), ग्रन्थिः_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_ग्रन्थिः* setupStmtsp = nullptr;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::vector<std::string> smtExprs;
        अत्थाय (Ast_ग्रन्थिः* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            सचे (AstVarRef* ध्रुव varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ध्रुव varp = varRefp->varp();
                Ast_ग्रन्थिः_Module* ध्रुव varModp = VN_AS(varp->user2p(), ग्रन्थिः_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();

                ध्रुव स्थिर auto dynDTypeSupported
                    = [](ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) -> तार्किक {
                    पटिनिवत्त VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elemWidth;
                uint32_t staticSize = 0;  // Element count, zero when only known at run time
                सचे (ध्रुव AstUnpackArrayDType* ध्रुव up = VN_CAST(dtypep, UnpackArrayDType)) {
                    ध्रुव AstRange* ध्रुव rangep = up->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    staticSize = up->elementsConst();
                    सचे (staticSize > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        अनुवर्त;
                    }
                    elemWidth = static_cast<uint64_t>(varp->dtypep()->width());
                } अञ्ञथा सचे (dynDTypeSupported(dtypep)) {
                    elemWidth = static_cast<uint64_t>(dtypep->subDTypep()->width());
                } अञ्ञथा {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(मिच्छा));
                    अनुवर्त;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subp = dtypep->subDTypep()->skipRefp();
                सचे (VN_IS(subp, ग्रन्थिः_Array_DType) || VN_IS(subp, QueueDType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    अनुवर्त;
                }

                सचे (m_wantSingle && !staticSize) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    अनुवर्त;
                }

                सचे (!varp->user3()) {
                    varp->user3(सच्च);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varnamep = नूतन AstCExpr{
                        fl, AstCExpr::Pure{}, "\"" + varp->name() + "\"", varp->width()};

                    AstCMethodHard* ध्रुव writeVarCallp = नूतन AstCMethodHard{
                        fl, नूतन AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ध्रुव argVarRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
                    सचे (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->addPinsp(argVarRefp);
                    writeVarCallp->addPinsp(नूतन AstConst{fl, AstConst::Unsized64{}, elemWidth});
                    writeVarCallp->addPinsp(varnamep);
                    writeVarCallp->addPinsp(नूतन AstConst{fl, 1});  // Dimension
                    ध्रुव RandomizeMode randMode = {.asUQuad = varp->user1()};
                    सचे (randMode.usesMode) {
                        writeVarCallp->addPinsp(
                            नूतन AstConst{fl, AstConst::Unsized64{}, randMode.index});
                    }
                    writeVarCallp->dtypeSetVoid();
                    initTaskp->addStmtsp(नूतन Ast_Stmt_व्यञ्जकम्{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                सचे (varp->user4p() && m_sizeConstrainedArraysp) {
                    m_sizeConstrainedArraysp->निवेशय(varp);
                }

                सचे (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    सचे (staticSize < 2) अनुवर्त;
                    std::string व्यञ्जकम्_Str = "(__Vbv (distinct";
                    अत्थाय (uint32_t i = 0; i < staticSize; ++i) {
                        व्यञ्जकम्_Str += " (select " + varp->name() + " #x";
                        अत्थाय (पूर्णाङ्क shift = 28; shift >= 0; shift -= 4) {
                            व्यञ्जकम्_Str += "0123456789abcdef"[(i >> shift) & 0xf];
                        }
                        व्यञ्जकम्_Str += ')';
                    }
                    smtExprs.emplace_back(व्यञ्जकम्_Str + "))");
                    अनुवर्त;
                }

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव namep = नूतन AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ध्रुव randUniqueCallp
                    = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->findVoidDType());
                randUniqueCallp->addPinsp(नूतन AstConst{fl, *m_uniqueConstraintId});
                setupStmtsp = Ast_ग्रन्थिः::addNext(setupStmtsp, नूतन Ast_Stmt_व्यञ्जकम्{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        सचे (m_wantSingle && !smtExprs.empty()) {
            std::string व्यञ्जकम्_Str = smtExprs.front();
            सचे (smtExprs.size() > 1) {
                व्यञ्जकम्_Str = "(bvand";
                अत्थाय (ध्रुव std::string& oneExprp : smtExprs) व्यञ्जकम्_Str += " " + oneExprp;
                व्यञ्जकम्_Str += ')';
            }
            AstSFormatF* ध्रुव newp = नूतन AstSFormatF{fl, व्यञ्जकम्_Str, मिच्छा, nullptr};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            पटिनिवत्त;
        }
        सचे (setupStmtsp) nodep->addHereThisAsNext(setupStmtsp);
        nodep->unlinkFrBack();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य visit(Ast_Constraint_व्यञ्जकम्* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_firstExpressionInsideIndexp);
        सचे (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp();
            सचे (exprp->width() > 1) {
                File_पङ्क्तिः* ध्रुव fl = exprp->fileline();
                ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, exprp->width(), 0};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव neqp
                    = नूतन AstNeq{fl, exprp->unlinkFrBack(), नूतन AstConst{fl, अङ्कः_Zero}};
                neqp->user1(सच्च);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        सचे (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ध्रुव VCMethod method
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            nodep->fileline(),
            नूतन AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                          VAccess::READWRITE},
            method, nodep->exprp()->unlinkFrBack()};
        callp->dtypeSetVoid();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->addPinsp(नूतन AstCExpr{nodep->fileline(), AstCExpr::Pure{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ध्रुव uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->addPinsp(नूतन AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string prettyText;
        सचे (!v3Global.opt.protectIds()) {
            prettyText = nodep->fileline()->prettySource();
            size_t pos = 0;
            याव ((pos = prettyText.find('"', pos)) != std::string::npos) {
                prettyText.निवेशय(pos, "\\");
                pos += std::strlen("\\\"");
            }
        }
        callp->addPinsp(
            नूतन AstCExpr{nodep->fileline(), AstCExpr::Pure{}, "\"" + prettyText + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

        सचे (nodep->method() == VCMethod::ARRAY_AT) {
            addStringNamePart(nodep->pinsp(), "%x");
            ध्रुव तार्किक indexIsRand = nodep->pinsp()->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp) {
                पटिनिवत्त vrefp->varp()->rand().isRandomizable();
            });
            सचे (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ध्रुव Ast_ग्रन्थिः* ध्रुव basep = nodep->baseFromp(सच्च);
                    सचे (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        पटिनिवत्त;
                    }
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = indexIsRand ? nullptr : nodep->clone_वृक्षः(मिच्छा);
                ध्रुव AstVarRef* ध्रुव baseRefp
                    = m_structSel ? VN_CAST(nodep->fromp()->baseFromp(सच्च), VarRef) : nullptr;
                सचे (baseRefp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(baseRefp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव indexp = nodep->pinsp()->clone_वृक्षः_Pure(मिच्छा);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrayp = nodep->fromp()->clone_वृक्षः_Pure(मिच्छा);
                    AstCMethodHard* ध्रुव currentSizep
                        = नूतन AstCMethodHard{fl, arrayp->clone_वृक्षः_Pure(मिच्छा), VCMethod::DYN_SIZE};
                    currentSizep->dtypeSetInt();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव inRangeRequiredp = नूतन AstLogAnd{
                        fl,
                        नूतन AstLteS{fl,
                                    नूतन AstConst{fl, AstConst::Widthed_मानम्{}, indexp->width(), 0},
                                    indexp->clone_वृक्षः_Pure(मिच्छा)},
                        नूतन AstGteS{fl, indexp->clone_वृक्षः_Pure(मिच्छा), currentSizep}};
                    AstCMethodHard* ध्रुव resizep
                        = नूतन AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             नूतन AstAdd{fl, indexp, नूतन AstConst{fl, 1}}};
                    resizep->dtypeSetVoid();
                    m_prepareConstrainedArraysp->addStmtsp(
                        नूतन AstIf{fl, inRangeRequiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ध्रुव sizep
                    = m_structSel ? नूतन AstCMethodHard{fl, nodep->fromp()->clone_वृक्षः_Pure(मिच्छा),
                                                       VCMethod::DYN_SIZE}
                                  : nullptr;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = nodep->pinsp()->unlinkFrBack();
                AstSFormatF* newp;
                सचे (m_structSel) {
                    सचे (AstSFormatF* ध्रुव sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argsp
                        = Ast_ग्रन्थिः::addNext(nodep->fromp()->unlinkFrBack(), pinp);
                    sizep->dtypeSetInt();
                    AstLogAnd* ध्रुव condp
                        = नूतन AstLogAnd{fl,
                                        नूतन AstLteS{fl,
                                                    नूतन AstConst{fl, AstConst::Widthed_मानम्{},
                                                                 originalPinp->width(), 0},
                                                    originalPinp->clone_वृक्षः_Pure(मिच्छा)},
                                        नूतन AstLtS{fl, originalPinp->clone_वृक्षः_Pure(मिच्छा), sizep}};
                    m_conditionp = m_conditionp ? नूतन AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = नूतन AstSFormatF{fl, "%s.%s", मिच्छा, argsp};
                } अञ्ञथा {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                सचे (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_वृक्षः(), origp);
                }

            } अञ्ञथा सचे (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } अञ्ञथा {
                nodep->user1(मिच्छा);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            पटिनिवत्त;
        }

        सचे (nodep->method() == VCMethod::ARRAY_INSIDE) {
            ध्रुव तार्किक randArr = nodep->fromp()->user1();

            AstVar* ध्रुव newVarp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->findIntDType()};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxRefp = नूतन AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            AstForeachHeader* ध्रुव headerp
                = नूतन AstForeachHeader{fl, nodep->fromp()->clone_वृक्षः_Pure(मिच्छा), newVarp};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_ग्रन्थिः* ध्रुव itemp = नूतन AstEq{fl, selp, nodep->pinsp()->unlinkFrBack()};
            itemp->user1(सच्च);

            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(iterateSubtreeReturnEdits(itemp));
            cstmtp->add(";");
            AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(नूतन AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     नूतन AstForeach{fl, headerp, cstmtp}, सच्च});
            cexprp->add("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(नूतन AstSFormatF{fl, "%s", मिच्छा, cexprp});
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }

        // Array reduction methods (sum, product, and, or, xor)
        सचे (nodep->method() == VCMethod::ARRAY_R_SUM
            || nodep->method() == VCMethod::ARRAY_R_PRODUCT
            || nodep->method() == VCMethod::ARRAY_R_AND || nodep->method() == VCMethod::ARRAY_R_OR
            || nodep->method() == VCMethod::ARRAY_R_XOR) {

            AstWith* ध्रुव withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            सचे (withp) {
                सचे (AstUnpackArrayDType* ध्रुव adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ध्रुव पूर्णाङ्क arraySize = adtypep->elementsConst();
                    सचे (arraySize > v3Global.opt.constraintArrayLimit()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvtToStr(arraySize)
                                          + " exceeds --constraint-array-limit of "
                                          + cvtToStr(v3Global.opt.constraintArrayLimit())
                                          + "), treating as state");
                        nodep->user1(मिच्छा);
                        सचे (editFormat(nodep)) पटिनिवत्त;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        पटिनिवत्त;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ध्रुव तार्किक hasRandomArray = nodep->fromp()->user1();
            सचे (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(मिच्छा);
                सचे (editFormat(nodep)) पटिनिवत्त;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                पटिनिवत्त;
            }
            सचे (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                पटिनिवत्त;
            }

            // Create loop variable and header
            AstVar* ध्रुव loopVarp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->findIntDType()};
            AstForeachHeader* ध्रुव headerp
                = नूतन AstForeachHeader{fl, nodep->fromp()->clone_वृक्षः_Pure(मिच्छा), loopVarp};

            // Determine SMT operation and compute identity elements
            ध्रुव वर्ण* smtOp = nullptr;
            std::string identity;
            सचे (withp) {
                // For 'with' clause: use binary format, compute from result width
                ध्रुव पूर्णाङ्क width = nodep->dtypep()->width();
                सचे (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = "#b" + std::string(width, '0');
                } अञ्ञथा सचे (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = (width > 0) ? "#b" + std::string(width - 1, '0') + "1" : "#b0";
                } अञ्ञथा सचे (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = "#b" + std::string(width, '1');
                } अञ्ञथा सचे (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = "#b" + std::string(width, '0');
                } अञ्ञथा {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = "#b" + std::string(width, '0');
                }
            } अञ्ञथा {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ध्रुव arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ध्रुव arrVarp = arrRefp->varp();
                Ast_ग्रन्थिः_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ध्रुव पूर्णाङ्क elemWidth = elemDtp->width();
                ध्रुव पूर्णाङ्क hexDigits = (elemWidth + 3) / 4;

                ध्रुव std::string zeroIdentity = "#x" + std::string(hexDigits, '0');
                ध्रुव std::string oneIdentity = "#x" + std::string(hexDigits - 1, '0') + "1";
                ध्रुव std::string allOnesIdentity = "#x" + std::string(hexDigits, 'f');

                सचे (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = zeroIdentity;
                } अञ्ञथा सचे (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = oneIdentity;
                } अञ्ञथा सचे (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = allOnesIdentity;
                } अञ्ञथा सचे (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = zeroIdentity;
                } अञ्ञथा {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = zeroIdentity;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            सचे (withp) {
                // With 'with' clause: evaluate expression for each element
                ध्रुव तार्किक randArr = nodep->fromp()->user1();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxRefp = नूतन AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ध्रुव पूर्णाङ्क resultWidth = nodep->dtypep()->width();
                ध्रुव VSigning resultSigning = nodep->dtypep()->numeric();

                Ast_ग्रन्थिः* perElemExprp = withp->exprp()->clone_वृक्षः_Pure(मिच्छा);
                सचे (AstLambdaArgRef* ध्रुव मूलम्_Refp = VN_CAST(perElemExprp, LambdaArgRef)) {
                    सचे (मूलम्_Refp->index()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ध्रुव loopRef = नूतन AstVarRef{fl, loopVarp, VAccess::READ};
                        perElemExprp = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                    } अञ्ञथा {
                        // item at root -> replace with array element
                        perElemExprp = elemSelp->clone_वृक्षः_Pure(मिच्छा);
                    }
                    VL_DO_DANGLING(मूलम्_Refp->delete_वृक्षः(), मूलम्_Refp);
                } अञ्ञथा {
                    perElemExprp->foreach([&](AstLambdaArgRef* refp) {
                        सचे (refp->index()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ध्रुव loopRef = नूतन AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_ग्रन्थिः* ध्रुव replacement
                                = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                            refp->replaceWith(replacement);
                        } अञ्ञथा {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_वृक्षः_Pure(मिच्छा));
                        }
                        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_वृक्षः(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                perElemExprp = V3Const::constifyEdit(perElemExprp);

                perElemExprp->foreach([&](Ast_ग्रन्थिः* nodep) {
                    // Don't mark loop variable references as randomizable
                    सचे (!VN_IS(nodep, Const)) {
                        सचे (Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
                            सचे (vrefp->varp() != loopVarp) nodep->user1(सच्च);
                        } अञ्ञथा {
                            nodep->user1(सच्च);
                        }
                    }
                });

                cstmtp->add("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(iterateSubtreeReturnEdits(perElemExprp));
                cstmtp->add(";\n");
                cstmtp->add("ret += \")\";\n");
            } अञ्ञथा {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ध्रुव arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ध्रुव arrVarp = arrRefp->varp();
                ध्रुव std::string smtArrayName = arrVarp->name();
                Ast_ग्रन्थिः_DType* ध्रुव elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ध्रुव पूर्णाङ्क elemWidth = elemDtp->width();
                Ast_ग्रन्थिः_Module* ध्रुव arrModulep = VN_AS(arrVarp->user2p(), ग्रन्थिः_Module);
                Ast_ग्रन्थिः_Module* ध्रुव genModulep = VN_AS(m_genp->user2p(), ग्रन्थिः_Module);
                arrVarp->user3(सच्च);

                // Add write_var call to init task
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrVarNamep
                    = नूतन AstCExpr{fl, AstCExpr::Pure{}, "\"" + smtArrayName + "\"", elemWidth};
                AstCMethodHard* ध्रुव writeVarCallp
                    = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->addPinsp(नूतन AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->addPinsp(
                    नूतन AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elemWidth)});
                writeVarCallp->addPinsp(arrVarNamep);
                ध्रुव uint32_t unpackedDims = arrVarp->dtypep()->dimensions(मिच्छा).second;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->addPinsp(नूतन AstConst{fl, 1});  // Dimension

                ध्रुव RandomizeMode randMode = {.asUQuad = arrVarp->user1()};
                सचे (randMode.usesMode) {
                    writeVarCallp->addPinsp(
                        नूतन AstConst{fl, AstConst::Unsized64{}, randMode.index});
                }
                writeVarCallp->dtypeSetVoid();
                Ast_ग्रन्थिः_FTask* initTaskp = m_inlineInitTaskp;
                सचे (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.findMember(arrModulep, "new"), ग्रन्थिः_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->addStmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ध्रुव idxFormatp = नूतन AstSFormatF{
                    fl, "#x%08x", मिच्छा, नूतन AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ध्रुव varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), idxFormatp);

                cstmtp->add("ret += \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(varNamep);
                cstmtp->add(";\n");
            }

            // Build final expression
            सचे (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret = \"" + identity + "\";\n");
                cexprp->add(नूतन AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         नूतन AstForeach{fl, headerp, cstmtp}, सच्च});
                cexprp->add("return ret;\n})()");
                nodep->replaceWith(नूतन AstSFormatF{fl, "%s", मिच्छा, cexprp});
            } अञ्ञथा {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret;\n");
                cexprp->add(नूतन AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         नूतन AstForeach{fl, headerp, cstmtp}, सच्च});
                cexprp->add("return ret.empty() ? \"" + identity + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(नूतन AstSFormatF{fl, "%s", मिच्छा, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            पटिनिवत्त;
        }

        // We've handled DYN_SIZE already if they should be randomized
        सचे (nodep->method() == VCMethod::DYN_SIZE || nodep->method() == VCMethod::ASSOC_SIZE) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ग्रन्थिः_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            याव (VN_IS(ग्रन्थिः_Exprp, MemberSel)) {
                ग्रन्थिः_Exprp = VN_AS(ग्रन्थिः_Exprp, MemberSel)->fromp();
            }
            सचे (VN_IS(ग्रन्थिः_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_वृक्षः_Pure(मिच्छा)));
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                पटिनिवत्त;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(मिच्छा);

        सचे (editFormat(nodep)) पटिनिवत्त;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;

        AstFunc* ध्रुव funcp = VN_CAST(nodep->taskp(), Func);
        सचे (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            पटिनिवत्त;
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव retExprp = extract_Return_व्यञ्जकम्(funcp);
        सचे (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(मिच्छा);
            सचे (editFormat(nodep)) पटिनिवत्त;
            पटिनिवत्त;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ध्रुव V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), nullptr, मिच्छा);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_ग्रन्थिः_व्यञ्जकम्* inlinedp = retExprp->clone_वृक्षः_Pure(मिच्छा);
        अत्थाय (ध्रुव auto& tconnect : tconnects) {
            ध्रुव AstVar* ध्रुव portp = tconnect.first;
            AstArg* ध्रुव argp = tconnect.second;
            सचे (!argp || !argp->exprp()) अनुवर्त;
            सचे (AstVarRef* ध्रुव मूलम्_Refp = VN_CAST(inlinedp, VarRef)) {
                सचे (मूलम्_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_वृक्षः_Pure(मिच्छा);
                    VL_DO_DANGLING(मूलम्_Refp->delete_वृक्षः(), मूलम्_Refp);
                    अनुवर्त;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                सचे (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_वृक्षः_Pure(मिच्छा));
                    VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    शून्य visit(Ast_ग्रन्थिः_Stmt* nodep) override {}
    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        सचे (editFormat(nodep)) पटिनिवत्त;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_ग्रन्थिः* buildDisableSoftCallStmt(Ast_Constraint_व्यञ्जकम्* nodep) {
        std::string varName;
        सचे (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp = VN_CAST(nodep->exprp(), ग्रन्थिः_Var_Ref)) {
            varName = vrefp->name();
        } अञ्ञथा सचे (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } अञ्ञथा {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            nodep->fileline(),
            नूतन AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            नूतन AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtypeSetVoid();
        पटिनिवत्त callp->makeStmt();
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
    Ast_ग्रन्थिः* extractConditionalDisableSofts(Ast_ग्रन्थिः* itemsp, Ast_ग्रन्थिः_व्यञ्जकम्* outerCondp) {
        Ast_ग्रन्थिः* hoistListp = nullptr;
        अत्थाय (Ast_ग्रन्थिः *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            सचे (AstConstraintIf* ध्रुव ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_पङ्क्तिः* ध्रुव fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenCondp
                    = outerCondp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                       नूतन AstLogAnd{fl, outerCondp->clone_वृक्षः_Pure(मिच्छा),
                                                     ifp->condp()->clone_वृक्षः_Pure(मिच्छा)})
                                 : ifp->condp()->clone_वृक्षः_Pure(मिच्छा);
                सचे (Ast_ग्रन्थिः* ध्रुव thenHoistp
                    = extractConditionalDisableSofts(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_ग्रन्थिः::addNext(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_वृक्षः(), thenCondp);
                सचे (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notInnerp
                        = नूतन AstLogNot{fl, ifp->condp()->clone_वृक्षः_Pure(मिच्छा)};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elseCondp
                        = outerCondp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{
                                           fl, outerCondp->clone_वृक्षः_Pure(मिच्छा), notInnerp})
                                     : notInnerp;
                    सचे (Ast_ग्रन्थिः* ध्रुव elseHoistp
                        = extractConditionalDisableSofts(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_ग्रन्थिः::addNext(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_वृक्षः(), elseCondp);
                }
            } अञ्ञथा सचे (Ast_Constraint_व्यञ्जकम्* ध्रुव exprp = VN_CAST(stmtp, Constraint_व्यञ्जकम्)) {
                सचे (exprp->isDisableSoft() && outerCondp) {
                    File_पङ्क्तिः* ध्रुव fl = exprp->fileline();
                    AstIf* ध्रुव hoistIfp = नूतन AstIf{fl, outerCondp->clone_वृक्षः_Pure(मिच्छा),
                                                      buildDisableSoftCallStmt(exprp), nullptr};
                    hoistListp = hoistListp ? Ast_ग्रन्थिः::addNext(hoistListp,
                                                               static_cast<Ast_ग्रन्थिः*>(hoistIfp))
                                            : static_cast<Ast_ग्रन्थिः*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_व्यञ्जकम्* ध्रुव truep
                        = नूतन Ast_Constraint_व्यञ्जकम्{fl, नूतन AstConst{fl, AstConst::BitTrue{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_वृक्षः(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        पटिनिवत्त hoistListp;
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit Constraint_व्यञ्जकम्_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_ग्रन्थिः* nodep,
                                   Ast_ग्रन्थिः_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* randModeVarp, std::set<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3UniqueNames& uniqueNames,
                                   Ast_ग्रन्थिः_FTask* memberselInitTaskp = nullptr,
                                   VInsertionSet<AstVar*>* sizeConstrainedArraysp = nullptr,
                                   Ast_ग्रन्थिः_FTask* prepareConstrainedArraysp = nullptr)
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
        सचे (Ast_ग्रन्थिः* ध्रुव hoistListp
            = nodep ? extractConditionalDisableSofts(nodep, nullptr) : nullptr) {
            nodep->addNext(hoistListp);
        }
        iterateAndNextNull(nodep);
    }
};

गणना वग्ग CaptureMode : uint8_t {
    CAP_NO = 0x0,
    CAP_मानम् = 0x01,
    CAP_THIS = 0x02,
    CAP_F_SET_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
CaptureMode operator|(CaptureMode a, CaptureMode b) {
    पटिनिवत्त static_cast<CaptureMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
CaptureMode operator&(CaptureMode a, CaptureMode b) {
    पटिनिवत्त static_cast<CaptureMode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
CaptureMode mode(CaptureMode a) { पटिनिवत्त a & static_cast<CaptureMode>(0x3); }
तार्किक hasFlags(CaptureMode a, CaptureMode flags) {
    पटिनिवत्त ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

वग्ग CaptureVisitor final : सार्वजनिक VNVisitor {
    AstArg* m_argsp = nullptr;  // Original references turned into arguments
    Ast_ग्रन्थिः_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ध्रुव AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::set<Ast_ग्रन्थिः*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = nullptr;  // Variable for outer context's object, if necessary
    तार्किक m_staticContext = मिच्छा;  // True when capturing from a static function context

    // METHODS

    तार्किक captureVariable(File_पङ्क्तिः* ध्रुव fileline, Ast_ग्रन्थिः_Var_Ref* varrefp, AstVar*& varp) {
        auto it = m_varCloneMap.find(varrefp->varp());
        सचे (it == m_varCloneMap.end()) {
            AstVar* ध्रुव newVarp = varrefp->varp()->clone_वृक्षः(मिच्छा);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(सच्च);
            newVarp->direction(VDirection::निवेशः);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            पटिनिवत्त सच्च;
        }
        varp = it->second;
        पटिनिवत्त मिच्छा;
    }

    template <typename T_ग्रन्थिः>
    शून्य fixupClassOrPackage(Ast_ग्रन्थिः* memberp, T_ग्रन्थिः refp) {
        Ast_ग्रन्थिः_Module* ध्रुव declClassp = VN_AS(memberp->user2p(), ग्रन्थिः_Module);
        सचे (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    template <typename T_ग्रन्थिः>
    तार्किक isReferenceToInnerMember(T_ग्रन्थिः nodep) {
        पटिनिवत्त VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_पङ्क्तिः* fl) {
        सचे (!m_thisp) {
            AstClassRefDType* ध्रुव refDTypep
                = नूतन AstClassRefDType{fl, VN_AS(m_callerp, Class), nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
            m_thisp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(सच्च);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::निवेशः);
            m_argsp = Ast_ग्रन्थिः::addNext(m_argsp, नूतन AstArg{fl, "", नूतन AstThisRef{fl, refDTypep}});
        }
        पटिनिवत्त m_thisp;
    }

    AstVar* getVar(AstVar* ध्रुव varp) ध्रुव {
        ध्रुव auto it = m_varCloneMap.find(varp);
        सचे (it == m_varCloneMap.end()) पटिनिवत्त nullptr;
        सचे (it->second->isStdRandomizeArg()) पटिनिवत्त nullptr;
        पटिनिवत्त it->second;
    }

    CaptureMode getVarRefCaptureMode(Ast_ग्रन्थिः_Var_Ref* varRefp) {
        Ast_ग्रन्थिः_Module* ध्रुव varModp = VN_AS(varRefp->varp()->user2p(), ग्रन्थिः_Module);
        AstClass* ध्रुव varClassp = VN_CAST(varModp, Class);
        AstClass* ध्रुव callerClassp = VN_CAST(m_callerp, Class);

        ध्रुव तार्किक callerIsClass = callerClassp;
        ध्रुव तार्किक refIsXref = VN_IS(varRefp, VarXRef);
        ध्रुव तार्किक varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ध्रुव तार्किक varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ध्रुव तार्किक varIsFieldOfCaller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ध्रुव तार्किक varIsParam = varRefp->varp()->isParam();
        ध्रुव तार्किक varIsConstraintIterator
            = VN_IS(varRefp->varp()->firstAbovep(), ForeachHeader)
              && VN_IS(varRefp->varp()->firstAbovep()->firstAbovep(), ConstraintForeach);
        सचे (refIsXref) पटिनिवत्त CaptureMode::CAP_मानम् | CaptureMode::CAP_F_XREF;
        सचे (varIsConstraintIterator) पटिनिवत्त CaptureMode::CAP_NO;
        सचे (varIsFuncLocal && varHasAutomaticLifetime) पटिनिवत्त CaptureMode::CAP_मानम्;
        सचे (varIsParam) पटिनिवत्त CaptureMode::CAP_मानम्;
        // Static var in function (will not be inlined, because it's in class)
        सचे (callerIsClass && varIsFuncLocal) पटिनिवत्त CaptureMode::CAP_मानम्;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        सचे (m_staticContext && callerIsClass && varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            पटिनिवत्त CaptureMode::CAP_NO;
        सचे (callerIsClass && varIsFieldOfCaller) पटिनिवत्त CaptureMode::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        सचे (callerIsClass && varClassp && !varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            पटिनिवत्त CaptureMode::CAP_मानम्;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        पटिनिवत्त CaptureMode::CAP_मानम्;
    }

    शून्य capture_Ref_By_मानम्(Ast_ग्रन्थिः_Var_Ref* nodep, CaptureMode capModeFlags) {
        AstVar* newVarp;
        ध्रुव तार्किक newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_ग्रन्थिः_Var_Ref* ध्रुव newVarRefp = newCapture ? nodep->clone_वृक्षः(मिच्छा) : nullptr;
        सचे (!hasFlags(capModeFlags, CaptureMode::CAP_F_SET_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(nullptr);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        सचे (!newCapture) पटिनिवत्त;
        सचे (hasFlags(capModeFlags, CaptureMode::CAP_F_XREF)) {
            AstVarRef* ध्रुव notXVarRefp
                = नूतन AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_ग्रन्थिः::addNext(m_argsp, नूतन AstArg{nodep->fileline(), "", newVarRefp});
    }

    शून्य captureRefByThis(Ast_ग्रन्थिः_Var_Ref* nodep, CaptureMode /*capModeFlags*/) {
        AstVar* ध्रुव thisp = importThisp(nodep->fileline());
        AstVarRef* ध्रुव thisRefp = नूतन AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(सच्च);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ध्रुव memberSelp
            = नूतन AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        सचे (!m_targetp && nodep->user1()) memberSelp->user1(सच्च);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        सचे (m_ignore.गणनम्(nodep)) पटिनिवत्त;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        सचे (nodep->varp()->isStdRandomizeArg()) पटिनिवत्त;
        ध्रुव CaptureMode capMode = getVarRefCaptureMode(nodep);
        सचे (mode(capMode) == CaptureMode::CAP_NO) पटिनिवत्त;
        सचे (mode(capMode) == CaptureMode::CAP_मानम्) capture_Ref_By_मानम्(nodep, capMode);
        सचे (mode(capMode) == CaptureMode::CAP_THIS) captureRefByThis(nodep, capMode);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        सचे (m_ignore.गणनम्(nodep)) {
            iterateChildren(nodep);
            पटिनिवत्त;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        सचे (VN_IS(nodep, MethodCall) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            पटिनिवत्त;
        }
        AstClass* ध्रुव classp = VN_CAST(nodep->taskp()->user2p(), Class);
        सचे ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ध्रुव argsp = nodep->argsp();
            सचे (argsp) argsp->unlinkFrBack();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ध्रुव thisp = importThisp(nodep->fileline());
            AstVarRef* ध्रुव thisRefp = नूतन AstVarRef{
                nodep->fileline(), thisp, nodep->isPure() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            AstMethodCall* ध्रुव methodCallp
                = नूतन AstMethodCall{nodep->fileline(), thisRefp, thisp->name(), argsp};
            methodCallp->taskp(nodep->taskp());
            methodCallp->dtypep(nodep->dtypep());
            nodep->replaceWith(methodCallp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            m_ignore.emplace(methodCallp);
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        सचे (!isReferenceToInnerMember(nodep)) {
            iterateChildren(nodep);
            पटिनिवत्त;
        }
        AstVarRef* ध्रुव varRefp
            = नूतन AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    शून्य visit(AstMethodCall* nodep) override {
        सचे (!isReferenceToInnerMember(nodep) || m_ignore.गणनम्(nodep)) {
            iterateChildren(nodep);
            पटिनिवत्त;
        }
        AstArg* ध्रुव argsp = nodep->argsp();
        सचे (argsp) argsp->unlinkFrBackWithNext();
        Ast_ग्रन्थिः_FTask_Ref* taskRefp = nullptr;
        सचे (AstTask* ध्रुव taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = नूतन AstTaskRef{nodep->fileline(), taskp, argsp};
        } अञ्ञथा सचे (AstFunc* ध्रुव taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = नूतन AstFuncRef{nodep->fileline(), taskp, argsp};
        } अञ्ञथा {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    explicit CaptureVisitor(Ast_ग्रन्थिः* ध्रुव nodep, Ast_ग्रन्थिः_Module* callerp, AstClass* ध्रुव targetp,
                            तार्किक staticContext = मिच्छा)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterateAndNextNull(nodep);
    }

    // PUBLIC METHODS

    AstArg* getArgs() ध्रुव { पटिनिवत्त m_argsp; }
    AstVar* getThisp() ध्रुव { पटिनिवत्त m_thisp; }

    शून्य addFunctionArguments(Ast_ग्रन्थिः_FTask* funcp) ध्रुव {
        अत्थाय (AstArg* argp = getArgs(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            सचे (Ast_ग्रन्थिः_Var_Ref* varrefp = VN_CAST(argp->exprp(), ग्रन्थिः_Var_Ref)) {
                सचे ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(nullptr);
                }
                funcp->addStmtsp(getVar(varrefp->varp()));
            } अञ्ञथा {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->addStmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

वग्ग RandomizeVisitor final : सार्वजनिक VNVisitor {
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
    ध्रुव VNUser3InUse m_inuser3;
    ध्रुव VNUser4InUse m_inuser4;

    // STATE
    V3UniqueNames m_inlineUniqueNames;  // For generating unique function names
    V3UniqueNames m_randArrayUniqueNames{"__Vrandarr"};  // Random array loop indices
    V3UniqueNames m_modeUniqueNames{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3UniqueNames m_inlineUniqueStdName{"__VStdrand"};
    V3UniqueNames m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    std::unordered_map<Ast_ग्रन्थिः_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_ग्रन्थिः_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ग्रन्थिः_Stmt* m_stmtp = nullptr;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = nullptr;  // Dynamic array type (for rand mode)
    पूर्णाङ्क m_rand_Case_अङ्कः = 0;  // Randcase number within a module for var naming
    पूर्णाङ्क m_dist_अङ्कः = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = nullptr;  // Current constraint
    std::set<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertionSet<AstVar*>> m_sizeConstrainedArrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_staticConstraintModeVars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_staticRandModeVars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<तार्किक, तार्किक>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    स्थिर तार्किक is_Similar_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* with_व्यञ्जकम्, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* arg_व्यञ्जकम्) {
        // VarRef: compare variable pointers
        सचे (VN_IS(arg_व्यञ्जकम्, VarRef) && VN_IS(with_व्यञ्जकम्, VarRef)) {
            पटिनिवत्त VN_AS(with_व्यञ्जकम्, VarRef)->varp() == VN_AS(arg_व्यञ्जकम्, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        सचे (VN_IS(arg_व्यञ्जकम्, MemberSel) && VN_IS(with_व्यञ्जकम्, MemberSel)) {
            ध्रुव AstMemberSel* ध्रुव withMSp = VN_AS(with_व्यञ्जकम्, MemberSel);
            ध्रुव AstMemberSel* ध्रुव argMSp = VN_AS(arg_व्यञ्जकम्, MemberSel);
            सचे (withMSp->varp() != argMSp->varp()) पटिनिवत्त मिच्छा;
            // Recursively compare the base object expression
            पटिनिवत्त is_Similar_ग्रन्थिः(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        सचे (VN_IS(arg_व्यञ्जकम्, ArraySel) && VN_IS(with_व्यञ्जकम्, ArraySel)) {
            ध्रुव AstArraySel* ध्रुव withASp = VN_AS(with_व्यञ्जकम्, ArraySel);
            ध्रुव AstArraySel* ध्रुव argASp = VN_AS(arg_व्यञ्जकम्, ArraySel);
            // Before checking Sel type index, check if both are Const
            सचे (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ध्रुव AstConst* ध्रुव withIdxp = VN_AS(withASp->bitp(), Const);
                ध्रुव AstConst* ध्रुव argIdxp = VN_AS(argASp->bitp(), Const);
                पटिनिवत्त is_Similar_ग्रन्थिः(withASp->fromp(), argASp->fromp())
                       && withIdxp->अङ्कः().isCaseEq(argIdxp->अङ्कः());
            }
            // Index must be Sel type, extract VarRef using fromp()
            सचे (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) पटिनिवत्त मिच्छा;
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            पटिनिवत्त is_Similar_ग्रन्थिः(withASp->fromp(), argASp->fromp())
                   && is_Similar_ग्रन्थिः(withIdxp, argIdxp);
        }
        पटिनिवत्त मिच्छा;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    स्थिर शून्य expandUniqueElementList(Ast_ग्रन्थिः* itemsp) {
        Ast_ग्रन्थिः* itemp = itemsp;
        याव (itemp) {
            Ast_ग्रन्थिः* ध्रुव nextp = itemp->nextp();
            सचे (ध्रुव AstConstraintIf* ध्रुव ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                अनुवर्त;
            }
            सचे (ध्रुव AstConstraintForeach* ध्रुव foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                अनुवर्त;
            }
            AstConstraintUnique* ध्रुव uniquep = VN_CAST(itemp, ConstraintUnique);
            सचे (!uniquep) {
                itemp = nextp;
                अनुवर्त;
            }

            std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> व्यञ्जकम्_वस्तूनि;
            तार्किक hasArrayVarRef = मिच्छा;
            अत्थाय (Ast_ग्रन्थिः* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                सचे (ध्रुव AstVarRef* ध्रुव vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = vrp->varp()->dtypep()->skipRefp();
                    सचे (VN_IS(dtypep, ग्रन्थिः_Array_DType) || VN_IS(dtypep, QueueDType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = सच्च;
                        अनुवर्त;
                    }
                }
                व्यञ्जकम्_वस्तूनि.प्रक्षिप_back(VN_AS(rp, ग्रन्थिः_व्यञ्जकम्));
            }

            सचे (व्यञ्जकम्_वस्तूनि.size() >= 2) {
                File_पङ्क्तिः* ध्रुव fl = uniquep->fileline();
                अत्थाय (size_t i = 0; i < व्यञ्जकम्_वस्तूनि.size(); i++) {
                    अत्थाय (size_t j = i + 1; j < व्यञ्जकम्_वस्तूनि.size(); j++) {
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = व्यञ्जकम्_वस्तूनि[i]->clone_वृक्षः(मिच्छा);
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = व्यञ्जकम्_वस्तूनि[j]->clone_वृक्षः(मिच्छा);
                        AstNeq* ध्रुव neqp = नूतन AstNeq{fl, lhsp, rhsp};
                        neqp->user1(सच्च);
                        Ast_Constraint_व्यञ्जकम्* ध्रुव cexprp = नूतन Ast_Constraint_व्यञ्जकम्{fl, neqp};
                        uniquep->addNextHere(cexprp);
                    }
                }
                सचे (!hasArrayVarRef) {
                    uniquep->unlinkFrBack();
                    VL_DO_DANGLING(uniquep->delete_वृक्षः(), uniquep);
                }
            } अञ्ञथा सचे (व्यञ्जकम्_वस्तूनि.size() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlinkFrBack();
                VL_DO_DANGLING(uniquep->delete_वृक्षः(), uniquep);
            }
            itemp = nextp;
        }
    }
    शून्य createRandomGenerator(AstClass* ध्रुव classp) {
        सचे (classp->user3p()) पटिनिवत्त;
        सचे (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            पटिनिवत्त;
        }
        AstVar* ध्रुव genp = नूतन AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->addMembersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_ग्रन्थिः_Module* ध्रुव modp) {
        auto it = m_stdMap.find(modp);
        सचे (it == m_stdMap.end()) {
            AstVar* ध्रुव stdgenp
                = नूतन AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->findBasicDType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3ErrorCode::IMPURE, सच्च);
            modp->addStmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            पटिनिवत्त stdgenp;
        }
        पटिनिवत्त it->second;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ध्रुव classp) {
        स्थिर ध्रुव वर्ण* ध्रुव name = "__Vprepare_constrained_arrays";
        AstTask* ध्रुव taskp = नूतन AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(सच्च);
        classp->addMembersp(taskp);
        m_memberMap.निवेशय(classp, taskp);
        पटिनिवत्त taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ध्रुव classp) {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.findMember(classp, name), Task);
        सचे (resizeTaskp) पटिनिवत्त resizeTaskp;
        resizeTaskp = नूतन AstTask{classp->fileline(), name, nullptr};
        resizeTaskp->classMethod(सच्च);
        resizeTaskp->isVirtual(सच्च);
        classp->addMembersp(resizeTaskp);
        m_memberMap.निवेशय(classp, resizeTaskp);
        पटिनिवत्त resizeTaskp;
    }
    AstVar* getCreateRandModeVar(AstClass* ध्रुव classp) {
        सचे (classp->user2p()) पटिनिवत्त VN_AS(classp->user2p(), Var);
        सचे (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            पटिनिवत्त getCreateRandModeVar(extendsp->classp());
        }
        AstVar* ध्रुव randModeVarp = createModeVar(classp, "__Vrandmode");
        classp->user2p(randModeVarp);
        पटिनिवत्त randModeVarp;
    }
    AstVar* getCreateConstraintModeVar(AstClass* ध्रुव classp) {
        सचे (classp->user4p()) पटिनिवत्त VN_AS(classp->user4p(), Var);
        सचे (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            पटिनिवत्त getCreateConstraintModeVar(extendsp->classp());
        }
        AstVar* ध्रुव constraintModeVarp = createModeVar(classp, "__Vconstraintmode");
        classp->user4p(constraintModeVarp);
        पटिनिवत्त constraintModeVarp;
    }
    स्थिर AstVar* getConstraintModeVar(AstClass* ध्रुव classp) {
        सचे (classp->user4p()) पटिनिवत्त VN_AS(classp->user4p(), Var);
        सचे (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            पटिनिवत्त getConstraintModeVar(extendsp->classp());
        }
        पटिनिवत्त nullptr;
    }
    AstVar* getCreateStaticConstraintModeVar(AstClass* ध्रुव classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        सचे (it != m_staticConstraintModeVars.end()) पटिनिवत्त it->second;
        सचे (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            पटिनिवत्त getCreateStaticConstraintModeVar(extendsp->classp());
        }
        AstVar* ध्रुव staticModeVarp = createStaticModeVar(classp, "__Vstaticconstraintmode");
        m_staticConstraintModeVars[classp] = staticModeVarp;
        पटिनिवत्त staticModeVarp;
    }
    AstVar* getStaticConstraintModeVar(AstClass* ध्रुव classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        सचे (it != m_staticConstraintModeVars.end()) पटिनिवत्त it->second;
        सचे (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            पटिनिवत्त getStaticConstraintModeVar(extendsp->classp());
        }
        पटिनिवत्त nullptr;
    }
    AstVar* getCreateStaticRandModeVar(AstClass* ध्रुव classp) {
        सचे (m_staticRandModeVars.गणनम्(classp)) पटिनिवत्त m_staticRandModeVars[classp];
        सचे (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            पटिनिवत्त getCreateStaticRandModeVar(extendsp->classp());
        }
        AstVar* ध्रुव staticModeVarp = createStaticModeVar(classp, "__Vstaticrandmode");
        m_staticRandModeVars[classp] = staticModeVarp;
        पटिनिवत्त staticModeVarp;
    }
    AstVar* getStaticRandModeVar(AstClass* ध्रुव classp) {
        सचे (m_staticRandModeVars.गणनम्(classp)) पटिनिवत्त m_staticRandModeVars[classp];
        सचे (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            पटिनिवत्त getStaticRandModeVar(extendsp->classp());
        }
        पटिनिवत्त nullptr;
    }
    AstVar* createModeVar(AstClass* ध्रुव classp, ध्रुव वर्ण* ध्रुव name) {
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        सचे (!m_dynarrayDtp) {
            m_dynarrayDtp = नूतन AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ध्रुव modeVarp = नूतन AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        पटिनिवत्त modeVarp;
    }
    AstVar* createStaticModeVar(AstClass* ध्रुव classp, ध्रुव वर्ण* ध्रुव name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        सचे (!m_dynarrayDtp) {
            m_dynarrayDtp = नूतन AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        AstVar* ध्रुव modeVarp = नूतन AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        modeVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.निवेशय(classp, modeVarp);
        पटिनिवत्त modeVarp;
    }
    स्थिर शून्य addSetRandMode(Ast_ग्रन्थिः_FTask* ध्रुव ftaskp, AstVar* ध्रुव genp,
                               AstVar* ध्रुव randModeVarp) {
        File_पङ्क्तिः* ध्रुव fl = ftaskp->fileline();
        AstCMethodHard* ध्रुव setRandModep = नूतन AstCMethodHard{
            fl, नूतन AstVarRef{fl, VN_AS(genp->user2p(), ग्रन्थिः_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_SET_RANDMODE,
            नूतन AstVarRef{fl, VN_AS(randModeVarp->user2p(), ग्रन्थिः_Module), randModeVarp,
                          VAccess::READ}};
        setRandModep->dtypeSetVoid();
        ftaskp->addStmtsp(setRandModep->makeStmt());
    }
    स्थिर शून्य addSetStaticRandMode(Ast_ग्रन्थिः_FTask* ध्रुव ftaskp, AstVar* ध्रुव genp,
                                     AstVar* ध्रुव staticRandModeVarp) {
        File_पङ्क्तिः* ध्रुव fl = ftaskp->fileline();
        AstCMethodHard* ध्रुव setp = नूतन AstCMethodHard{
            fl, नूतन AstVarRef{fl, VN_AS(genp->user2p(), ग्रन्थिः_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_SET_STATIC_RANDMODE,
            नूतन AstVarRef{fl, VN_AS(staticRandModeVarp->user2p(), ग्रन्थिः_Module), staticRandModeVarp,
                          VAccess::READ}};
        setp->dtypeSetVoid();
        ftaskp->addStmtsp(setp->makeStmt());
    }
    शून्य createRandomizeClassVars(AstNetlist* ध्रुव netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> मूलम्_Static_Rand_Mode_गणनम्;
        netlistp->foreach([एतत्, &मूलम्_Static_Rand_Mode_गणनम्](AstClass* ध्रुव classp) {
            तार्किक hasConstraints = मिच्छा;
            uint32_t rand_Mode_गणनम् = 0;
            uint32_t static_Rand_Mode_गणनम् = 0;
            uint32_t constraint_Mode_गणनम् = 0;
            uint32_t static_Constraint_Mode_गणनम् = 0;
            classp->foreachMember([&](AstClass*, Ast_ग्रन्थिः* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                सचे (AstConstraint* ध्रुव constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = सच्च;
                    RandomizeMode constraintMode = {.asUQuad = memberp->user1()};
                    सचे (!constraintMode.usesMode) पटिनिवत्त;
                    सचे (constraintMode.index == 0) {
                        // Use separate index counters for static vs non-static constraints
                        सचे (constrp->isStatic()) {
                            constraintMode.index = static_Constraint_Mode_गणनम्++;
                        } अञ्ञथा {
                            constraintMode.index = constraint_Mode_गणनम्++;
                        }
                        memberp->user1(constraintMode.asUQuad);
                    } अञ्ञथा {
                        सचे (constrp->isStatic()) {
                            static_Constraint_Mode_गणनम् = constraintMode.index + 1;
                        } अञ्ञथा {
                            constraint_Mode_गणनम् = constraintMode.index + 1;
                        }
                    }
                } अञ्ञथा सचे (AstVar* ध्रुव varp = VN_CAST(memberp, Var)) {
                    RandomizeMode randMode = {.asUQuad = memberp->user1()};
                    सचे (!randMode.usesMode) पटिनिवत्त;
                    ध्रुव तार्किक isStaticVar = varp->lifetime().isStatic();
                    सचे (randMode.index == 0) {
                        सचे (isStaticVar) {
                            randMode.index = static_Rand_Mode_गणनम्++;
                        } अञ्ञथा {
                            randMode.index = rand_Mode_गणनम्++;
                        }
                        memberp->user1(randMode.asUQuad);
                    } अञ्ञथा {
                        सचे (isStaticVar) {
                            static_Rand_Mode_गणनम् = randMode.index + 1;
                        } अञ्ञथा {
                            rand_Mode_गणनम् = randMode.index + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<शून्य(AstClass*)> findSubObjRandModes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_ग्रन्थिः* subMemberp) {
                        सचे (AstVar* ध्रुव subVarp = VN_CAST(subMemberp, Var)) {
                            ध्रुव RandomizeMode rm = {.asUQuad = subVarp->user1()};
                            सचे (!rm.usesMode) पटिनिवत्त;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            सचे (subVarp->lifetime().isStatic()) पटिनिवत्त;
                            ध्रुव uint32_t needed = rm.index + 1;
                            सचे (needed > rand_Mode_गणनम्) rand_Mode_गणनम् = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    सचे (!memberVarp->globalConstrained()) पटिनिवत्त;
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = memberVarp->dtypep()->skipRefp();
                    ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
                    सचे (!classRefp) पटिनिवत्त;
                    findSubObjRandModes(classRefp->classp());
                });
            }
            सचे (hasConstraints) createRandomGenerator(classp);
            सचे (rand_Mode_गणनम् > 0) {
                AstVar* ध्रुव randModeVarp = getCreateRandModeVar(classp);
                makeModeInit(randModeVarp, classp, rand_Mode_गणनम्);
            }
            सचे (constraint_Mode_गणनम् > 0) {
                AstVar* ध्रुव constraintModeVarp = getCreateConstraintModeVar(classp);
                makeModeInit(constraintModeVarp, classp, constraint_Mode_गणनम्);
            }
            सचे (static_Constraint_Mode_गणनम् > 0) {
                AstVar* ध्रुव staticConstraintModeVarp = getCreateStaticConstraintModeVar(classp);
                makeStaticModeInit(staticConstraintModeVarp, classp, static_Constraint_Mode_गणनम्);
            }
            सचे (static_Rand_Mode_गणनम् > 0) {
                getCreateStaticRandModeVar(classp);
                AstClass* rootp = classp;
                याव (AstClassExtends* ध्रुव ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& slot = मूलम्_Static_Rand_Mode_गणनम्[rootp];
                सचे (static_Rand_Mode_गणनम् > slot) slot = static_Rand_Mode_गणनम्;
            }
        });
        अत्थाय (ध्रुव auto& kv : मूलम्_Static_Rand_Mode_गणनम्) emit_मूलम्_Static_Mode_Init(kv.first, kv.second);
    }
    शून्य emit_मूलम्_Static_Mode_Init(AstClass* ध्रुव rootp, ध्रुव uint32_t गणनम्) {
        AstVar* ध्रुव staticRandModeVarp = m_staticRandModeVars[rootp];
        UASSERT_OBJ(staticRandModeVarp, rootp, "Root must have a static rand-mode var");
        makeStaticModeInit(staticRandModeVarp, rootp, गणनम्);
    }
    शून्य makeModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_गणनम्) {
        Ast_ग्रन्थिः_Module* ध्रुव modeVarModp = VN_AS(modeVarp->user2p(), ग्रन्थिः_Module);
        File_पङ्क्तिः* ध्रुव fl = modeVarp->fileline();
        AstVar* ध्रुव oldSizeVarp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__VoldSize"), modeVarp->findUInt32DType()};
        oldSizeVarp->funcLocal(सच्च);
        oldSizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ध्रुव oldSizep
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        oldSizep->dtypeSetUInt32();
        AstCMethodHard* ध्रुव dynarrayNewp
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, नूतन AstConst{fl, mode_गणनम्}};
        dynarrayNewp->dtypeSetVoid();
        Ast_ग्रन्थिः_FTask* ध्रुव ctorNewp = VN_AS(m_memberMap.findMember(classp, "new"), ग्रन्थिः_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_ग्रन्थिः* ध्रुव initFirstp = oldSizeVarp;
        initFirstp->addNext(
            नूतन AstAssign{fl, नूतन AstVarRef{fl, oldSizeVarp, VAccess::WRITE}, oldSizep});
        initFirstp->addNext(dynarrayNewp->makeStmt());
        initFirstp->addNext(makeModeSetLoop(
            fl, नूतन AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE}, नूतन AstConst{fl, 1},
            सच्च, नूतन AstVarRef{fl, oldSizeVarp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        अत्थाय (Ast_ग्रन्थिः* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (!VN_IS(stmtp, ग्रन्थिः_Stmt)) अनुवर्त;  // Skip var declarations
            सचे (ध्रुव Ast_Stmt_व्यञ्जकम्* ध्रुव sep = VN_CAST(stmtp, Stmt_व्यञ्जकम्)) {
                सचे (VN_IS(sep->exprp(), New)) अनुवर्त;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->addHereThisAsNext(initFirstp);
            पटिनिवत्त;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->addStmtsp(initFirstp);
    }
    शून्य makeStaticModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_गणनम्) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_ग्रन्थिः_Module* ध्रुव modeVarModp = VN_AS(modeVarp->user2p(), ग्रन्थिः_Module);
        File_पङ्क्तिः* ध्रुव fl = modeVarp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ध्रुव sizep
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstEq* ध्रुव condp = नूतन AstEq{fl, sizep, नूतन AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ध्रुव dynarrayNewp
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, नूतन AstConst{fl, mode_गणनम्}};
        dynarrayNewp->dtypeSetVoid();
        Ast_ग्रन्थिः* ध्रुव thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->addNext(
            makeModeSetLoop(fl, नूतन AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                            नूतन AstConst{fl, 1}, सच्च));

        // Build the if statement
        AstIf* ध्रुव ifp = नूतन AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(m_memberMap.findMember(classp, "new"), ग्रन्थिः_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->addStmtsp(ifp);
    }
    Ast_ग्रन्थिः* makeModeSetLoop(File_पङ्क्तिः* ध्रुव fl, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp,
                             तार्किक inTask, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव startp = nullptr) {
        AstVar* ध्रुव iterVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->findUInt32DType()};
        iterVarp->funcLocal(inTask);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ध्रुव sizep = नूतन AstCMethodHard{fl, lhsp, VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstCMethodHard* ध्रुव setp
            = नूतन AstCMethodHard{fl, lhsp->clone_वृक्षः(मिच्छा), VCMethod::ARRAY_AT_WRITE,
                                 नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtypeSetUInt32();
        Ast_ग्रन्थिः* ध्रुव stmtsp = iterVarp;
        stmtsp->addNext(नूतन AstAssign{fl, नूतन AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : नूतन AstConst{fl, 0}});

        AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(नूतन AstLoopTest{
            fl, loopp, नूतन AstLt{fl, नूतन AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->addStmtsp(नूतन AstAssign{fl, setp, rhsp});
        loopp->addStmtsp(नूतन AstAssign{
            fl, नूतन AstVarRef{fl, iterVarp, VAccess::WRITE},
            नूतन AstAdd{fl, नूतन AstConst{fl, 1}, नूतन AstVarRef{fl, iterVarp, VAccess::READ}}});
        पटिनिवत्त नूतन AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtsp, सच्च};
    }
    शून्य addTempVarsp(AstVar* varsp) {
        अत्थाय (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            सचे (m_ftaskp) {
                varp->funcLocal(सच्च);
            } अञ्ञथा {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        सचे (!m_ftaskp) {
            m_modp->addStmtsp(varsp);
        } अञ्ञथा {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_ग्रन्थिः_Stmt* wrapIfRandMode(AstClass* classp, AstVar* ध्रुव varp, Ast_ग्रन्थिः_Stmt* stmtp) {
        ध्रुव RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstVar* ध्रुव modeVarp = varp->lifetime().isStatic() ? getStaticRandModeVar(classp)
                                                             : getRandModeVarFromClass(classp);
        पटिनिवत्त VN_AS(wrapIfMode(rmode, modeVarp, stmtp), ग्रन्थिः_Stmt);
    }
    Ast_ग्रन्थिः* wrapIfConstraintMode(AstClass* classp, AstConstraint* ध्रुव constrp, Ast_ग्रन्थिः* stmtp) {
        ध्रुव RandomizeMode rmode = {.asUQuad = constrp->user1()};
        AstVar* ध्रुव modeVarp = constrp->isStatic() ? getStaticConstraintModeVar(classp)
                                                     : getConstraintModeVar(classp);
        पटिनिवत्त wrapIfMode(rmode, modeVarp, stmtp);
    }
    स्थिर Ast_ग्रन्थिः* wrapIfMode(ध्रुव RandomizeMode mode, AstVar* modeVarp, Ast_ग्रन्थिः* stmtp) {
        File_पङ्क्तिः* ध्रुव fl = stmtp->fileline();
        सचे (mode.usesMode) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_ग्रन्थिः_Module* ध्रुव modp = VN_AS(modeVarp->user2p(), ग्रन्थिः_Module);
            AstCMethodHard* ध्रुव atp
                = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, modp, modeVarp, VAccess::READ},
                                     VCMethod::ARRAY_AT, नूतन AstConst{fl, mode.index}};
            atp->dtypeSetUInt32();
            पटिनिवत्त नूतन AstIf{fl, atp, stmtp};
        }
        पटिनिवत्त stmtp;
    }
    AstVarRef* enum_मानम्_Tab_Refp(AstEnumDType* ध्रुव nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        सचे (nodep->user2p()) पटिनिवत्त VN_AS(nodep->user2p(), VarRef)->clone_वृक्षः(मिच्छा);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_ग्रन्थिः_Array_DType* ध्रुव vardtypep = नूतन AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            नूतन AstRange{nodep->fileline(), static_cast<पूर्णाङ्क>(nodep->item_गणनम्()) - 1, 0}};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        AstConsArray* ध्रुव consp = नूतन AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t index = 0;
        अत्थाय (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ध्रुव vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(index++, vconstp->clone_वृक्षः(मिच्छा));
        }
        // Share identical tables via the constant pool
        AstVarRef* ध्रुव refp = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_वृक्षः(), consp);  // V3ConstPool::find clones it
        प्रक्षिप_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        पटिनिवत्त refp->clone_वृक्षः(मिच्छा);
    }

    AstCDType* findVlRandCDType(File_पङ्क्तिः* ध्रुव fl, uint64_t वस्तूनि) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ध्रुव std::string type = AstCDType::typeToHold(V3_सङ्ख्या::log2bQuad(वस्तूनि) + 1);
        ध्रुव std::string name = "VlRandC<" + type + ", " + cvtToStr(वस्तूनि) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ध्रुव auto pair = m_randcDtypes.emplace(name, nullptr);
        सचे (pair.second) {
            AstCDType* ध्रुव newp = नूतन AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            pair.first->second = newp;
        }
        पटिनिवत्त pair.first->second;
    }

    AstVar* newRandcVarsp(AstVar* ध्रुव varp, AstClass* ध्रुव classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        सचे (!varp->isRandC()) पटिनिवत्त nullptr;

        uint64_t वस्तूनि = 0;
        सचे (AstEnumDType* ध्रुव enumDtp = VN_CAST(varp->dtypep()->skipRefToEnump(), EnumDType)) {
            वस्तूनि = static_cast<uint64_t>(enumDtp->item_गणनम्());
        } अञ्ञथा सचे (AstBasicDType* ध्रुव basicp = varp->dtypep()->skipRefp()->basicp()) {
            सचे (basicp->width() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                सचे (!V3Error::error_गणनम्()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->width() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                पटिनिवत्त nullptr;
            }
            वस्तूनि = 1ULL << basicp->width();
        } अञ्ञथा सचे (AstStructDType* ध्रुव dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            पटिनिवत्त nullptr;
        } अञ्ञथा {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_गणनम्(), varp, "Unexpected randc variable dtype");
            पटिनिवत्त nullptr;
        }
        AstCDType* ध्रुव newdtp = findVlRandCDType(varp->fileline(), वस्तूनि);
        AstVar* ध्रुव newp
            = नूतन AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->isInternal(सच्च);
        classp->addStmtsp(newp);
        UINFO(9, "created " << varp);
        पटिनिवत्त newp;
    }
    Ast_ग्रन्थिः_Stmt* createArrayForeachLoop(File_पङ्क्तिः* ध्रुव fl, Ast_ग्रन्थिः_DType* ध्रुव dtypep,
                                        Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar* ध्रुव outputVarp) {
        Ast_ग्रन्थिः_DType* tempDTypep = dtypep;
        AstVar* randLoopIndxp = nullptr;
        auto createLoopIndex = [&](Ast_ग्रन्थिः_DType* tempDTypep) {
            सचे (VN_IS(tempDTypep, AssocArrayDType)) {
                पटिनिवत्त नूतन AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                                  VN_AS(tempDTypep, AssocArrayDType)->कुञ्चिका_DTypep()};
            }
            पटिनिवत्त नूतन AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                              dtypep->findBasicDType(VBasicDTypeKwd::UINT32)};
        };
        Ast_ग्रन्थिः_व्यञ्जकम्* tempElementp = nullptr;
        याव (tempDTypep->isNonPackedArray()) {
            AstVar* ध्रुव newRandLoopIndxp = createLoopIndex(tempDTypep);
            randLoopIndxp = Ast_ग्रन्थिः::addNext(randLoopIndxp, newRandLoopIndxp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tempExprp = tempElementp ? tempElementp : exprp;
            AstVarRef* ध्रुव tempRefp = नूतन AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            सचे (VN_IS(tempDTypep, DynArrayDType)) {
                tempElementp
                    = नूतन AstCMethodHard{fl, tempExprp, VCMethod::ARRAY_AT_WRITE, tempRefp};
            } अञ्ञथा सचे (VN_IS(tempDTypep, UnpackArrayDType)) {
                Ast_ग्रन्थिः_Array_DType* ध्रुव aryDTypep = VN_CAST(tempDTypep, ग्रन्थिः_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                tempElementp = नूतन AstArraySel{
                    fl, tempExprp,
                    नूतन AstSel{
                        fl,
                        नूतन AstSub{fl, tempRefp,
                                   नूतन AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        नूतन AstConst{fl, 0}, V3_सङ्ख्या::log2b(aryDTypep->hi()) + 1}};
            } अञ्ञथा सचे (VN_IS(tempDTypep, AssocArrayDType)) {
                tempElementp = नूतन AstAssocSel{fl, tempExprp, tempRefp};
            } अञ्ञथा सचे (VN_IS(tempDTypep, QueueDType)) {
                tempElementp
                    = नूतन AstCMethodHard{fl, tempExprp, VCMethod::DYN_AT_WRITE_APPEND, tempRefp};
            }
            tempElementp->dtypep(tempDTypep->subDTypep());
            tempDTypep = tempDTypep->virtRefDTypep();
        }

        AstForeachHeader* ध्रुव headerp
            = नूतन AstForeachHeader{fl, exprp->clone_वृक्षः(मिच्छा), randLoopIndxp};
        Ast_ग्रन्थिः_Stmt* ध्रुव randStmtsp = newRandStmtsp(fl, tempElementp, nullptr, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        सचे (!tempElementp->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(tempElementp), tempElementp);
        पटिनिवत्त नूतन AstForeach{fl, headerp, randStmtsp};
    }
    Ast_ग्रन्थिः_Stmt* newRandStmtsp(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar* randcVarp,
                               AstVar* ध्रुव outputVarp, पूर्णाङ्क offset = 0,
                               AstMemberDType* memberp = nullptr) {
        Ast_ग्रन्थिः_DType* ध्रुव memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        सचे (ध्रुव auto* ध्रुव structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_ग्रन्थिः_Stmt* stmtsp = nullptr;
            सचे (structDtp->packed()) offset += memberp ? memberp->lsb() : 0;
            अत्थाय (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_ग्रन्थिः_Stmt* randp = nullptr;
                सचे (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_वृक्षः(मिच्छा) : exprp, nullptr,
                                          outputVarp, offset, smemberp);
                } अञ्ञथा सचे (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = नूतन AstStructSel{fl, exprp->clone_वृक्षः(मिच्छा), smemberp->name()};
                    structSelp->dtypep(smemberp->childDTypep());
                    सचे (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, nullptr, outputVarp);
                }
                stmtsp = Ast_ग्रन्थिः::addNextNull(stmtsp, randp);
            }
            पटिनिवत्त stmtsp;
        } अञ्ञथा सचे (ध्रुव auto* ध्रुव unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            सचे (!unionDtp->packed()) पटिनिवत्त nullptr;
            AstMemberDType* ध्रुव firstMemberp = unionDtp->membersp();
            पटिनिवत्त newRandStmtsp(fl, exprp, nullptr, outputVarp, offset, firstMemberp);
        } अञ्ञथा सचे (ध्रुव AstClassRefDType* ध्रुव classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ध्रुव memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            AstMethodCall* ध्रुव callp = नूतन AstMethodCall{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            AstAssign* ध्रुव assignp = नूतन AstAssign{
                fl, नूतन AstVarRef{fl, outputVarp, VAccess::WRITE},
                नूतन AstAnd{fl, नूतन AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            पटिनिवत्त नूतन AstIf{
                fl, नूतन AstNeq{fl, exprp->clone_वृक्षः(मिच्छा), नूतन AstConst{fl, AstConst::Null{}}},
                assignp};
        } अञ्ञथा सचे (AstDynArrayDType* ध्रुव dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            पटिनिवत्त createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } अञ्ञथा सचे (AstQueueDType* ध्रुव queueDtp = VN_CAST(memberDtp, QueueDType)) {
            पटिनिवत्त createArrayForeachLoop(fl, queueDtp, exprp, outputVarp);
        } अञ्ञथा सचे (AstUnpackArrayDType* ध्रुव unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            पटिनिवत्त createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } अञ्ञथा सचे (AstAssocArrayDType* ध्रुव assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            पटिनिवत्त createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } अञ्ञथा {
            Ast_ग्रन्थिः_व्यञ्जकम्* valp;
            सचे (AstEnumDType* ध्रुव enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ध्रुव tabRefp = enum_मानम्_Tab_Refp(enumDtp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव randp
                    = new_Rand_मानम्(fl, randcVarp, exprp->findBasicDType(VBasicDTypeKwd::UINT32));
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव moddivp = नूतन AstModDiv{
                    fl, randp, नूतन AstConst{fl, static_cast<uint32_t>(enumDtp->item_गणनम्())}};
                moddivp->dtypep(enumDtp);
                valp = नूतन AstArraySel{fl, tabRefp, moddivp};
            } अञ्ञथा {
                valp
                    = new_Rand_मानम्(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            AstAssign* assignp
                = नूतन AstAssign{fl,
                                नूतन AstSel{fl, exprp, offset + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->width() : exprp->width()},
                                valp};
            AstVar* varp = nullptr;
            exprp->exists([&](ध्रुव AstVarRef* varrefp) {
                सचे (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                पटिनिवत्त varp != nullptr;
            });
            पटिनिवत्त wrapIfRandMode(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* new_Rand_मानम्(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव randcVarp,
                              Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        सचे (randcVarp) {
            AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
            cexprp->add(नूतन AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->add(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            पटिनिवत्त cexprp;
        }

        पटिनिवत्त नूतन AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* findPrePostTask(AstClass* classp, ध्रुव string& name) {
        अत्थाय (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            सचे (AstTask* ध्रुव taskp = VN_CAST(m_memberMap.findMember(cp, name), Task)) {
                पटिनिवत्त taskp;
            }
        }
        पटिनिवत्त nullptr;
    }
    शून्य addPrePostCall(AstClass* ध्रुव classp, Ast_ग्रन्थिः_FTask* ध्रुव funcp, ध्रुव string& name) {
        सचे (AstTask* ध्रुव userFuncp = findPrePostTask(classp, name)) {
            AstTaskRef* ध्रुव callp = नूतन AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->addStmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* ध्रुव classp, ध्रुव string& which) {
        ध्रुव string name = "__V" + which;
        सचे (AstTask* ध्रुव existingp = VN_CAST(m_memberMap.findMember(classp, name), Task)) {
            पटिनिवत्त existingp;
        }
        AstTask* ध्रुव taskp = नूतन AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(सच्च);
        taskp->isVirtual(classp->isExtended());
        classp->addMembersp(taskp);
        m_memberMap.निवेशय(classp, taskp);
        addPrePostCall(classp, taskp, which);
        पटिनिवत्त taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<तार्किक, तार्किक> buildPrePostVirtualWrappers(AstClass* ध्रुव classp) {
        ध्रुव auto cachedIt = m_prePostWrap.find(classp);
        सचे (cachedIt != m_prePostWrap.end()) पटिनिवत्त cachedIt->second;
        std::vector<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            सचे (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.प्रक्षिप_back(subp);
        });
        तार्किक hasPre = मिच्छा;
        तार्किक hasPost = मिच्छा;
        अत्थाय (AstClass* ध्रुव cp : hierp) {
            सचे (findPrePostTask(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = सच्च;
            }
            सचे (findPrePostTask(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = सच्च;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        सचे (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        सचे (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ध्रुव std::pair<तार्किक, तार्किक> result{hasPre, hasPost};
        m_prePostWrap.emplace(classp, result);
        पटिनिवत्त result;
    }
    शून्य addVirtualPrePostCall(AstFunc* ध्रुव randomizeFuncp, AstClass* ध्रुव classp,
                               ध्रुव string& which) {
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        AstTask* ध्रुव wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ध्रुव refDTypep = नूतन AstClassRefDType{fl, classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
        AstMethodCall* ध्रुव callp
            = नूतन AstMethodCall{fl, नूतन AstThisRef{fl, refDTypep}, wrapperp->name(), nullptr};
        callp->taskp(wrapperp);
        callp->dtypeSetVoid();
        randomizeFuncp->addStmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    तार्किक classHasRandClassMembers(AstClass* classp) {
        पटिनिवत्त classp->existsMember([](ध्रुव AstClass*, ध्रुव AstVar* varp) {
            सचे (!varp->rand().isRandomizable()) पटिनिवत्त मिच्छा;
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
            पटिनिवत्त VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    तार्किक classOwnsGlobalConstraint(ध्रुव AstClass* classp) ध्रुव {
        पटिनिवत्त classp->existsMember([](ध्रुव AstClass*, ध्रुव AstConstraint* constrp) {
            तार्किक owns = मिच्छा;
            constrp->foreach([&](ध्रुव AstMemberSel* memberSelp) {
                ध्रुव Ast_ग्रन्थिः* ध्रुव rootp
                    = const_cast<AstMemberSel*>(memberSelp)->baseFromp(सच्च);
                सचे (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ध्रुव Ast_ग्रन्थिः_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    याव (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    सचे (VN_IS(dtypep, ClassRefDType)) owns = सच्च;
                }
            });
            पटिनिवत्त owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ध्रुव string& suffix) {
        ध्रुव string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.findMember(classp, name), Task);
        सचे (taskp) पटिनिवत्त taskp;
        taskp = नूतन AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(सच्च);
        classp->addMembersp(taskp);
        m_memberMap.निवेशय(classp, taskp);
        पटिनिवत्त taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    शून्य populateNestedCallbackTask(AstTask* ध्रुव callbackTaskp, AstClass* ध्रुव classp,
                                    ध्रुव string& cbName) {
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            सचे (!memberVarp->rand().isRandomizable()) पटिनिवत्त;
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = memberVarp->dtypep()->skipRefp();
            ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
            सचे (!classRefp) पटिनिवत्त;
            AstClass* ध्रुव memberClassp = classRefp->classp();
            सचे (memberClassp == classp) पटिनिवत्त;  // Avoid self-reference

            // Force-visit member class if not yet processed
            सचे (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_ग्रन्थिः* stmtsp = nullptr;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            सचे (AstTask* ध्रुव userFuncp = findPrePostTask(memberClassp, cbName)) {
                AstMethodCall* ध्रुव callp = नूतन AstMethodCall{
                    fl, नूतन AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtypeSetVoid();
                stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            सचे (classHasRandClassMembers(memberClassp)) {
                ध्रुव string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ध्रुव nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                AstMethodCall* ध्रुव recurseCallp = नूतन AstMethodCall{
                    fl, नूतन AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtypeSetVoid();
                stmtsp = Ast_ग्रन्थिः::addNext(stmtsp, recurseCallp->makeStmt());
            }

            सचे (!stmtsp) पटिनिवत्त;

            // Wrap in null check
            AstIf* ध्रुव nullCheckp = नूतन AstIf{
                fl,
                नूतन AstNeq{fl, नूतन AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           नूतन AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->addStmtsp(wrapIfRandMode(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* newSetupConstraintTask(AstClass* ध्रुव nodep, ध्रुव std::string& name) {
        AstTask* ध्रुव taskp = नूतन AstTask{nodep->fileline(), name + "_setup_constraint", nullptr};
        taskp->classMethod(सच्च);
        nodep->addMembersp(taskp);
        पटिनिवत्त taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ध्रुव nodep, ध्रुव std::string& name) {
        AstTask* ध्रुव taskp
            = नूतन AstTask{nodep->fileline(), name + "_resize_constrained_array", nullptr};
        taskp->classMethod(सच्च);
        nodep->addMembersp(taskp);
        पटिनिवत्त taskp;
    }
    Ast_ग्रन्थिः_Stmt* implementConstraintsClear(File_पङ्क्तिः* ध्रुव fileline, AstVar* ध्रुव genp) {
        AstCMethodHard* ध्रुव clearp = नूतन AstCMethodHard{
            fileline,
            नूतन AstVarRef{fileline, VN_AS(genp->user2p(), ग्रन्थिः_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtypeSetVoid();
        पटिनिवत्त clearp->makeStmt();
    }
    Ast_ग्रन्थिः_Stmt* implementConstraintsClearAll(File_पङ्क्तिः* ध्रुव fileline, AstVar* ध्रुव genp) {
        AstCMethodHard* ध्रुव clearp = नूतन AstCMethodHard{
            fileline,
            नूतन AstVarRef{fileline, VN_AS(genp->user2p(), ग्रन्थिः_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtypeSetVoid();
        पटिनिवत्त clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
        सचे (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
            पटिनिवत्त memberSelp->varp();
        } अञ्ञथा सचे (AstVarRef* ध्रुव varrefp = VN_CAST(exprp, VarRef)) {
            पटिनिवत्त varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        पटिनिवत्त nullptr;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_ग्रन्थिः_व्यञ्जकम्* makeModeVarRef(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp, AstVar* ध्रुव modeVarp,
                                ध्रुव VAccess access) {
        सचे (modeVarp->lifetime().isStatic()) {
            पटिनिवत्त नूतन AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), ग्रन्थिः_Module),
                                 modeVarp, access};
        }
        सचे (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ध्रुव newMemberSelp = नूतन AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_वृक्षः(मिच्छा), modeVarp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            पटिनिवत्त newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        पटिनिवत्त नूतन AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), Class), modeVarp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* createOrGetSizeVar(AstClass* ध्रुव classp, AstVar* ध्रुव arrVarp, File_पङ्क्तिः* ध्रुव fl,
                               Ast_ग्रन्थिः_DType* ध्रुव signed32DTypep, तार्किक& wasCreated) {
        AstVar* sizeVarp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = मिच्छा;
        सचे (!sizeVarp) {
            sizeVarp = नूतन AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->addMembersp(sizeVarp);
            m_memberMap.निवेशय(classp, sizeVarp);
            sizeVarp->user2p(classp);
            arrVarp->user4p(sizeVarp);
            wasCreated = सच्च;
        }
        पटिनिवत्त sizeVarp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_व्यञ्जकम्* createSizeGteZeroConstraint(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव sizeVarp,
                                                   Ast_ग्रन्थिः_व्यञ्जकम्* derefFromp = nullptr) {
        Ast_ग्रन्थिः_व्यञ्जकम्* size_ग्रन्थिः_Exprp = nullptr;
        सचे (derefFromp) {
            size_ग्रन्थिः_Exprp = buildMemberSelExprp(derefFromp, sizeVarp);
        } अञ्ञथा {
            size_ग्रन्थिः_Exprp = नूतन AstVarRef{fl, sizeVarp, VAccess::READ};
        }
        size_ग्रन्थिः_Exprp->user1(सच्च);
        AstGteS* ध्रुव sizeGtep = नूतन AstGteS{fl, size_ग्रन्थिः_Exprp, नूतन AstConst{fl, 0}};
        sizeGtep->user1(सच्च);
        पटिनिवत्त नूतन Ast_Constraint_व्यञ्जकम्{fl, sizeGtep};
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* getFromp(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
        सचे (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
            पटिनिवत्त memberSelp->fromp();
        } अञ्ञथा सचे (AstMethodCall* ध्रुव methodCallp = VN_CAST(exprp, MethodCall)) {
            पटिनिवत्त methodCallp->fromp();
        }
        पटिनिवत्त nullptr;
    }
    AstVar* makeTmpRandModeVar(Ast_ग्रन्थिः_व्यञ्जकम्* siblingExprp, AstVar* randModeVarp,
                               Ast_ग्रन्थिः*& storeStmtspr, Ast_ग्रन्थिः_Stmt*& restoreStmtspr) {
        File_पङ्क्तिः* ध्रुव fl = randModeVarp->fileline();
        AstVar* ध्रुव randModeTmpVarp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, m_modeUniqueNames.get(randModeVarp), randModeVarp->dtypep()};
        randModeTmpVarp->funcLocal(m_ftaskp);
        randModeTmpVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_ग्रन्थिः::addNext(
            storeStmtspr,
            नूतन AstAssign{fl, नूतन AstVarRef{fl, randModeTmpVarp, VAccess::WRITE},
                          makeModeVarRef(siblingExprp, randModeVarp, VAccess::READ)});
        storeStmtspr = Ast_ग्रन्थिः::addNext(
            storeStmtspr,
            makeModeSetLoop(fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                            नूतन AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_ग्रन्थिः::addNext(
            restoreStmtspr,
            नूतन AstAssign{fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                          नूतन AstVarRef{fl, randModeTmpVarp, VAccess::READ}});
        पटिनिवत्त randModeTmpVarp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_ग्रन्थिः_व्यञ्जकम्* sliceToCommonPrefix(Ast_ग्रन्थिः_व्यञ्जकम्* thisp, Ast_ग्रन्थिः_व्यञ्जकम्* otherp) {
        स्थिर std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        याव (thisp) {
            thisHier.प्रक्षिप_back(thisp);
            thisp = getFromp(thisp);
        }
        याव (otherp) {
            otherHier.प्रक्षिप_back(otherp);
            otherp = getFromp(otherp);
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* commonp = nullptr;
        अत्थाय (auto thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            सचे ((*thisIt)->type() != (*otherIt)->type()) भङ्ग;
            सचे (AstMemberSel* ध्रुव memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ध्रुव otherMemberSelp = VN_AS(*otherIt, MemberSel);
                सचे (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    अनुवर्त;
                }
            } अञ्ञथा सचे (AstMethodCall* ध्रुव thisMethodCallp = VN_CAST(*thisIt, MethodCall)) {
                AstMethodCall* ध्रुव otherMethodCallp = VN_AS(*otherIt, MethodCall);
                सचे (thisMethodCallp->taskp() == otherMethodCallp->taskp()) {
                    commonp = thisMethodCallp;
                    अनुवर्त;
                }
            } अञ्ञथा सचे (AstVarRef* ध्रुव firstVarRefp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ध्रुव secondVarRefp = VN_AS(*otherIt, VarRef);
                सचे (firstVarRefp->varp() == secondVarRefp->varp()) {
                    commonp = firstVarRefp;
                    अनुवर्त;
                }
            }
            भङ्ग;
        }
        पटिनिवत्त commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_ग्रन्थिः_व्यञ्जकम्* buildMemberSelExprp(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar* deepestVarp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_ग्रन्थिः_व्यञ्जकम्* resExprp = nullptr;
        सचे (ध्रुव AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), nullptr);
            AstVar* varp = memberSelp->varp();
            सचे (deepestVarp) varp = deepestVarp;
            resExprp = नूतन AstMemberSel{exprp->fileline(), resExprp, varp};
        } अञ्ञथा सचे (ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            सचे (deepestVarp) varp = deepestVarp;
            resExprp = नूतन AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), ग्रन्थिः_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(सच्च);
        पटिनिवत्त resExprp;
    }

    शून्य addUpdateRandVarsBody(AstClass* ध्रुव nodep, AstFunc* ध्रुव randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ध्रुव updatep
            = नूतन AstFunc{nodep->fileline(), "__VupdateRandVars", nullptr, nullptr};
        updatep->classMethod(सच्च);
        updatep->isVirtual(सच्च);
        updatep->keepAlive(सच्च);
        updatep->needsSyms(मिच्छा);
        nodep->addMembersp(updatep);
        nodep->hasRandVarsUpdate(सच्च);

        अत्थाय (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(m_memberMap.findMember(classp, "new"), ग्रन्थिः_FTask);
            अत्थाय (Ast_ग्रन्थिः* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_व्यञ्जकम्* ध्रुव stmtExprp = VN_CAST(stmtp, Stmt_व्यञ्जकम्);
                सचे (!stmtExprp) अनुवर्त;
                AstCMethodHard* ध्रुव methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                सचे (!methodp || methodp->method() != VCMethod::RANDOMIZER_WRITE_VAR) अनुवर्त;

                Ast_ग्रन्थिः_Var_Ref* ध्रुव varRefp = VN_AS(methodp->pinsp(), ग्रन्थिः_Var_Ref);
                सचे (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    अनुवर्त;
                }
                AstCMethodHard* ध्रुव updateMethodp = नूतन AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_वृक्षः_Pure(मिच्छा),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_वृक्षः_Pure(मिच्छा)};
                updateMethodp->dtypeSetVoid();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव namep = VN_AS(varRefp->nextp()->nextp(), ग्रन्थिः_व्यञ्जकम्);
                updateMethodp->addPinsp(namep->clone_वृक्षः_Pure(मिच्छा));
                updateMethodp->method(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->addStmtsp(updateMethodp->makeStmt());
            }
        }
    }

    शून्य addBasicRandomizeBody(AstFunc* ध्रुव basicRandomizep, AstClass* ध्रुव nodep,
                               AstVar* randModeVarp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstVar* ध्रुव basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ध्रुव basicFvarRefp = नूतन AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ध्रुव beginBasicValp = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1};
        basicRandomizep->addStmtsp(नूतन AstAssign{fl, basicFvarRefp, beginBasicValp});
        Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(m_memberMap.findMember(nodep, "new"), ग्रन्थिः_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            सचे (!memberVarp->rand().isRandomizable()) पटिनिवत्त;
            ध्रुव RandomizeMode randMode = {.asUQuad = memberVarp->user1()};
            सचे (randMode.usesMode
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ध्रुव setp = नूतन AstCMethodHard{
                    nodep->fileline(),
                    नूतन AstVarRef{fl, VN_AS(randModeVarp->user2p(), ग्रन्थिः_Module), randModeVarp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, नूतन AstConst{nodep->fileline(), randMode.index}};
                setp->dtypeSetUInt32();
                newp->addStmtsp(नूतन AstAssign{fl, setp, नूतन AstConst{fl, 0}});
            }
            सचे (memberVarp->user3()) पटिनिवत्त;  // Handled in constraints
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = memberVarp->dtypep()->skipRefp();
            सचे (ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType)) {
                सचे (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    पटिनिवत्त;
                }
                AstFunc* ध्रुव memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                AstMethodCall* ध्रुव callp
                    = memberVarp->globalConstrained()
                          ? नूतन AstMethodCall{fl,
                                              नूतन AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : नूतन AstMethodCall{
                                fl, नूतन AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ध्रुव basicFvarRefReadp = basicFvarRefp->clone_वृक्षः(मिच्छा);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ध्रुव assignIfNotNullp = नूतन AstIf{
                    fl,
                    नूतन AstNeq{fl, नूतन AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               नूतन AstConst{fl, AstConst::Null{}}},
                    नूतन AstAssign{fl, basicFvarRefp->clone_वृक्षः(मिच्छा),
                                  नूतन AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->addStmtsp(wrapIfRandMode(nodep, memberVarp, assignIfNotNullp));
            } अञ्ञथा {
                AstVar* ध्रुव randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ध्रुव refp
                    = नूतन AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                सचे (!refp->backp()) VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
                basicRandomizep->addStmtsp(
                    नूतन AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtp, मिच्छा});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_ग्रन्थिः_व्यञ्जकम्* makeModeAssignLhs(File_पङ्क्तिः* ध्रुव fl, AstClass* ध्रुव classp,
                                   Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp, AstVar* ध्रुव modeVarp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        सचे (modeVarp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            सचे (fromp) VL_DO_DANGLING(fromp->unlinkFrBack()->delete_वृक्षः(), fromp);
            पटिनिवत्त नूतन AstVarRef{fl, VN_AS(modeVarp->user2p(), ग्रन्थिः_Module), modeVarp,
                                 VAccess::WRITE};
        } अञ्ञथा सचे (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            पटिनिवत्त नूतन AstVarRef{fl, VN_AS(modeVarp->user2p(), ग्रन्थिः_Module), modeVarp,
                                 VAccess::WRITE};
        } अञ्ञथा {
            AstMemberSel* ध्रुव memberselp = नूतन AstMemberSel{fl, fromp->unlinkFrBack(), modeVarp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            पटिनिवत्त memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    शून्य replaceWithModeAssign(Ast_ग्रन्थिः_FTask_Ref* ध्रुव ftaskRefp, Ast_ग्रन्थिः* ध्रुव receiverp,
                               Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp) {
        replaceWithModeAssignAndAppend(ftaskRefp, receiverp, lhsp, nullptr);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    शून्य replaceWithModeAssignAndAppend(Ast_ग्रन्थिः_FTask_Ref* ध्रुव ftaskRefp, Ast_ग्रन्थिः* ध्रुव receiverp,
                                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp, Ast_ग्रन्थिः* ध्रुव appendStmtp) {
        File_पङ्क्तिः* ध्रुव fl = ftaskRefp->fileline();
        सचे (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_व्यञ्जकम्), ftaskRefp, "Should be a statement");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = ftaskRefp->argsp()->exprp()->unlinkFrBack();
            Ast_ग्रन्थिः* newStmtp = nullptr;
            सचे (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                // mode
                ध्रुव RandomizeMode rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ध्रुव setp = नूतन AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                नूतन AstConst{fl, rmode.index}};
                setp->dtypeSetUInt32();
                newStmtp = नूतन AstAssign{fl, setp, rhsp};
            } अञ्ञथा {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = makeModeSetLoop(fl, lhsp, rhsp, m_ftaskp);
            }
            सचे (appendStmtp) newStmtp->addNext(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(प्रक्षिप_Deletep(m_stmtp), m_stmtp = nullptr);
        } अञ्ञथा {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ध्रुव RandomizeMode rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ध्रुव setp = नूतन AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            नूतन AstConst{fl, rmode.index}};
            setp->dtypeSetUInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    शून्य wrapRandomizeCallWithNullGuard(Ast_ग्रन्थिः_FTask_Ref* nodep) {
        AstMethodCall* ध्रुव callp = VN_CAST(nodep, MethodCall);
        सचे (!callp) पटिनिवत्त;
        सचे (callp->user4SetOnce()) पटिनिवत्त;
        File_पङ्क्तिः* ध्रुव fl = callp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव checkp
            = नूतन AstNeq{fl, callp->fromp()->clone_वृक्षः(मिच्छा), नूतन AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlinkFrBack(&relinker);
        relinker.relink(नूतन AstCond{fl, checkp, callp, नूतन AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    शून्य handleRandomizeArgs(Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep) {
        सचे (!nodep->argsp()) पटिनिवत्त;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        तार्किक hasNullArg = मिच्छा;
        अत्थाय (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->अङ्कः().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = सच्च;
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्षः(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        अत्थाय (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            अत्थाय (AstArg *otherArgp = nextp, *otherNextp = nullptr; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prefixp
                    = sliceToCommonPrefix(argp->exprp(), otherArgp->exprp())) {
                    सचे (prefixp == argp->exprp()) {
                        सचे (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlinkFrBack()->delete_वृक्षः(), otherArgp);
                        अनुवर्त;
                    }
                }
                सचे (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prefixp
                    = sliceToCommonPrefix(otherArgp->exprp(), argp->exprp())) {
                    सचे (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्षः(), argp);
                        भङ्ग;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::set<AstVar*> savedRandModeVarps;
        AstVar* tmpVarps = nullptr;
        Ast_ग्रन्थिः* storeStmtsp = nullptr;
        Ast_ग्रन्थिः* setStmtsp = nullptr;
        Ast_ग्रन्थिः_Stmt* restoreStmtsp = nullptr;
        अत्थाय (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ग्रन्थिः_व्यञ्जकम्* exprp = argp->exprp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव commonPrefixp = sliceToCommonPrefix(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_पङ्क्तिः* ध्रुव fl = argp->fileline();
            याव (exprp) {
                सचे (commonPrefixp == exprp) भङ्ग;
                AstVar* ध्रुव randVarp = getVarFromRef(exprp);
                AstClass* ध्रुव classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ध्रुव randModeVarp = randVarp->lifetime().isStatic()
                                                 ? getStaticRandModeVar(classp)
                                                 : getRandModeVarFromClass(classp);
                UASSERT_OBJ(randModeVarp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                सचे (savedRandModeVarps.find(randModeVarp) == savedRandModeVarps.end()) {
                    AstVar* ध्रुव randModeTmpVarp
                        = makeTmpRandModeVar(exprp, randModeVarp, storeStmtsp, restoreStmtsp);
                    savedRandModeVarps.निवेशय(randModeVarp);
                    tmpVarps = Ast_ग्रन्थिः::addNext(tmpVarps, randModeTmpVarp);
                }
                ध्रुव RandomizeMode randMode = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = नूतन AstCMethodHard{
                    fl, makeModeVarRef(exprp, randModeVarp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, नूतन AstConst{fl, randMode.index}};
                setp->dtypeSetUInt32();
                setStmtsp
                    = Ast_ग्रन्थिः::addNext(setStmtsp, नूतन AstAssign{fl, setp, नूतन AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्षः(), argp);
        }
        सचे (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* targetClassp = nullptr;
            AstMethodCall* ध्रुव methodCallp = VN_CAST(nodep, MethodCall);
            सचे (methodCallp) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDTypep = methodCallp->fromp()->dtypep();
                ध्रुव AstClassRefDType* ध्रुव crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                targetClassp = crdtp->classp();
            } अञ्ञथा {
                targetClassp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(targetClassp, nodep, "randomize(null) target class unresolved");
            AstFunc* ध्रुव checkOnlyFuncp = getCreateRandomizeNullFunc(targetClassp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        सचे (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && setStmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlinkFrBack(&relinker);
            Ast_ग्रन्थिः* ध्रुव stmtsp = storeStmtsp;
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
    AstFunc* getCreateRandomizeNullFunc(AstClass* ध्रुव classp) {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव name = "__Vrandomize_null";
        सचे (AstFunc* ध्रुव existingp = VN_AS(m_memberMap.findMember(classp, name), Func)) {
            पटिनिवत्त existingp;
        }
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        AstFunc* ध्रुव funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ध्रुव fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        addPrePostCall(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ध्रुव classGenp = getRandomGenerator(classp);
        सचे (!classGenp) {
            funcp->addStmtsp(नूतन AstAssign{fl, नूतन AstVarRef{fl, fvarp, VAccess::WRITE},
                                           नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1}});
        } अञ्ञथा {
            Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(classGenp->user2p(), ग्रन्थिः_Module);
            funcp->addStmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ध्रुव setupAllTaskp = getCreateConstraintSetupFunc(classp, m_memberMap);
            funcp->addStmtsp((नूतन AstTaskRef{fl, setupAllTaskp})->makeStmt());
            AstCExpr* ध्रुव solverCallp = नूतन AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(नूतन AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->add(".next_check_only(__Vm_rng)");
            funcp->addStmtsp(
                नूतन AstAssign{fl, नूतन AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(सच्च);
        }

        // 3. post_randomize -- only if result is non-zero
        सचे (AstTask* ध्रुव userPostp = findPrePostTask(classp, "post_randomize")) {
            AstTaskRef* ध्रुव callp = नूतन AstTaskRef{userPostp->fileline(), userPostp};
            funcp->addStmtsp(
                नूतन AstIf{fl, नूतन AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        पटिनिवत्त funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* liftLogIfChainToConstraintIf(AstLogIf* logIfp) {
        File_पङ्क्तिः* ध्रुव fl = logIfp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = logIfp->lhsp()->unlinkFrBack();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = logIfp->rhsp()->unlinkFrBack();
        Ast_ग्रन्थिः* thenBodyp;
        सचे (AstLogIf* ध्रुव innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = liftLogIfChainToConstraintIf(innerLogIfp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(innerLogIfp), innerLogIfp);
        } अञ्ञथा {
            thenBodyp = नूतन Ast_Constraint_व्यञ्जकम्{fl, rhsp};
        }
        पटिनिवत्त नूतन AstConstraintIf{fl, condp, thenBodyp, nullptr};
    }

    स्थिर तार्किक distBoundRefsRandVar(ध्रुव Ast_ग्रन्थिः* boundp) {
        पटिनिवत्त boundp->exists(
            [](ध्रुव AstVarRef* vrefp) { पटिनिवत्त vrefp->varp()->rand().isRandomizable(); });
    }

    स्थिर तार्किक distBoundRefsModeVar(ध्रुव Ast_ग्रन्थिः* boundp) {
        पटिनिवत्त boundp->exists([](ध्रुव AstVarRef* vrefp) {
            सचे (!vrefp->varp()->rand().isRandomizable()) पटिनिवत्त मिच्छा;
            ध्रुव RandomizeMode rmode = {.asUQuad = vrefp->varp()->user1()};
            पटिनिवत्त rmode.usesMode;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newDistRangeMembership(AstDist* distp, ध्रुव AstInsideRange* irp) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        ध्रुव तार्किक isSigned = distp->exprp()->isSigned();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dist_व्यञ्जकम्_Gtep = distp->exprp()->clone_वृक्षः_Pure(मिच्छा);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dist_व्यञ्जकम्_Ltep = distp->exprp()->clone_वृक्षः_Pure(मिच्छा);
        dist_व्यञ्जकम्_Gtep->user1(सच्च);
        dist_व्यञ्जकम्_Ltep->user1(सच्च);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव gep
            = isSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                             नूतन AstGteS{fl, dist_व्यञ्जकम्_Gtep, irp->lhsp()->clone_वृक्षः_Pure(मिच्छा)})
                       : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                             नूतन AstGte{fl, dist_व्यञ्जकम्_Gtep, irp->lhsp()->clone_वृक्षः_Pure(मिच्छा)});
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lep
            = isSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                             नूतन AstLteS{fl, dist_व्यञ्जकम्_Ltep, irp->rhsp()->clone_वृक्षः_Pure(मिच्छा)})
                       : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                             नूतन AstLte{fl, dist_व्यञ्जकम्_Ltep, irp->rhsp()->clone_वृक्षः_Pure(मिच्छा)});
        gep->user1(सच्च);
        lep->user1(सच्च);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव andp = नूतन AstLogAnd{fl, gep, lep};
        andp->user1(सच्च);
        पटिनिवत्त andp;
    }

    संरचना DistBucket final {
        Ast_ग्रन्थिः_व्यञ्जकम्* rangep;  // A single value or an InsideRange
        Ast_ग्रन्थिः_व्यञ्जकम्* weightExprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::vector<DistBucket> collectDistBuckets(AstDist* ध्रुव distp) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        std::vector<DistBucket> buckets;
        अत्थाय (AstDistItem* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), DistItem)) {
            सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(ditemp->weightp(), Const)) {
                सचे (constp->toUQuad() == 0) अनुवर्त;
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* weightExprp
                = नूतन AstExtend{fl, ditemp->weightp()->clone_वृक्षः_Pure(मिच्छा), 64};
            // := on a range weights every element, so scale by the range size.
            सचे (!ditemp->isWhole()) {
                सचे (ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ध्रुव AstConst* ध्रुव lop = VN_CAST(irp->lhsp(), Const);
                    ध्रुव AstConst* ध्रुव hip = VN_CAST(irp->rhsp(), Const);
                    Ast_ग्रन्थिः_व्यञ्जकम्* rangeSizep;
                    सचे (lop && hip) {
                        ध्रुव uint64_t rangeSize = hip->toUQuad() - lop->toUQuad() + 1;
                        rangeSizep = नूतन AstConst{fl, AstConst::Unsized64{}, rangeSize};
                    } अञ्ञथा {
                        rangeSizep = नूतन AstAdd{
                            fl, नूतन AstConst{fl, AstConst::Unsized64{}, 1},
                            नूतन AstSub{fl,
                                       नूतन AstExtend{fl, irp->rhsp()->clone_वृक्षः_Pure(मिच्छा), 64},
                                       नूतन AstExtend{fl, irp->lhsp()->clone_वृक्षः_Pure(मिच्छा), 64}}};
                    }
                    weightExprp = नूतन AstMul{fl, weightExprp, rangeSizep};
                }
            }
            buckets.प्रक्षिप_back({ditemp->rangep(), weightExprp});
        }
        पटिनिवत्त buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newDistMembershipTerm(AstDist* distp, Ast_ग्रन्थिः_व्यञ्जकम्* rangep) {
        सचे (ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange)) {
            पटिनिवत्त newDistRangeMembership(distp, irp);
        }
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqExprp = distp->exprp()->clone_वृक्षः_Pure(मिच्छा);
        eqExprp->user1(सच्च);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = नूतन AstEq{fl, eqExprp, rangep->clone_वृक्षः_Pure(मिच्छा)};
        eqp->user1(सच्च);
        पटिनिवत्त eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_व्यञ्जकम्* buildDistMembership(AstDist* distp,
                                           ध्रुव std::vector<DistBucket>& buckets) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* unionExprp = nullptr;
        अत्थाय (ध्रुव auto& bucket : buckets) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव memberp = newDistMembershipTerm(distp, bucket.rangep);
            सचे (!unionExprp) {
                unionExprp = memberp;
            } अञ्ञथा {
                unionExprp = नूतन AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(सच्च);
            }
        }
        पटिनिवत्त नूतन Ast_Constraint_व्यञ्जकम्{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_ग्रन्थिः_व्यञ्जकम्* newGateJoin(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp, तार्किक isAnd) {
        सचे (ap->same_वृक्षः(bp)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(bp), bp);
            पटिनिवत्त ap;
        }
        सचे (VN_IS(bp, Const)) std::swap(ap, bp);
        सचे (VN_IS(ap, Const)) {
            ध्रुव तार्किक constWins = VN_AS(ap, Const)->isZero() == isAnd;
            सचे (!constWins) std::swap(ap, bp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(bp), bp);
            पटिनिवत्त ap;
        }
        सचे (isAnd) पटिनिवत्त नूतन AstLogAnd{fl, ap, bp};
        पटिनिवत्त नूतन AstLogOr{fl, ap, bp};
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* newGateOr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp) {
        पटिनिवत्त newGateJoin(fl, ap, bp, मिच्छा);
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* newGateAnd(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp) {
        पटिनिवत्त newGateJoin(fl, ap, bp, सच्च);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_ग्रन्थिः_व्यञ्जकम्* newModeBitRead(AstVar* varp, ध्रुव AstMemberSel* mselp, AstVar* randModeVarp,
                                File_पङ्क्तिः* fl) {
        Ast_ग्रन्थिः_व्यञ्जकम्* arrayp = nullptr;
        सचे (varp->lifetime().isStatic()) {
            AstClass* ध्रुव ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ध्रुव smodep = getStaticRandModeVar(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = नूतन AstVarRef{fl, VN_AS(smodep->user2p(), ग्रन्थिः_Module), smodep, VAccess::READ};
        } अञ्ञथा सचे (mselp) {
            AstVar* ध्रुव memberModep = getRandModeVarFromClass(VN_AS(varp->user2p(), ग्रन्थिः_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = नूतन AstMemberSel{fl, mselp->fromp()->clone_वृक्षः_Pure(मिच्छा), memberModep};
        } अञ्ञथा {
            UASSERT_OBJ(randModeVarp, varp, "rand_mode variable without a class mode array");
            arrayp = नूतन AstVarRef{fl, VN_AS(randModeVarp->user2p(), ग्रन्थिः_Module), randModeVarp,
                                   VAccess::READ};
        }
        ध्रुव RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ध्रुव atp
            = नूतन AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, नूतन AstConst{fl, rmode.index}};
        atp->dtypeSetUInt32();
        पटिनिवत्त atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_ग्रन्थिः_व्यञ्जकम्* newVarGate(AstVar* varp, ध्रुव AstMemberSel* mselp, तार्किक ownerLevel,
                            AstVar* randModeVarp, File_पङ्क्तिः* fl) {
        सचे (!varp->rand().isRandomizable()) {
            पटिनिवत्त नूतन AstConst{fl, AstConst::BitTrue{}, ownerLevel};
        }
        ध्रुव RandomizeMode rmode = {.asUQuad = varp->user1()};
        सचे (!rmode.usesMode) पटिनिवत्त नूतन AstConst{fl, AstConst::BitTrue{}};
        पटिनिवत्त newModeBitRead(varp, mselp, randModeVarp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_ग्रन्थिः_व्यञ्जकम्* newAccessGate(ध्रुव Ast_ग्रन्थिः* nodep, तार्किक ownerLevel, AstVar* randModeVarp,
                               File_पङ्क्तिः* fl) {
        सचे (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(nodep, MemberSel)) {
            पटिनिवत्त newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, randModeVarp, fl),
                              newAccessGate(mselp->fromp(), सच्च, randModeVarp, fl));
        }
        सचे (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
            पटिनिवत्त newVarGate(refp->varp(), nullptr, ownerLevel, randModeVarp, fl);
        }
        सचे (ध्रुव Ast_ग्रन्थिः_Sel* ध्रुव selp = VN_CAST(nodep, ग्रन्थिः_Sel)) {
            पटिनिवत्त newAccessGate(selp->fromp(), सच्च, randModeVarp, fl);
        }
        सचे (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        पटिनिवत्त नूतन AstConst{fl, AstConst::BitTrue{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_ग्रन्थिः_व्यञ्जकम्* newDistGate(ध्रुव Ast_ग्रन्थिः* nodep, AstVar* randModeVarp, File_पङ्क्तिः* fl) {
        सचे (VN_IS(nodep, ग्रन्थिः_Var_Ref) || VN_IS(nodep, MemberSel)) {
            पटिनिवत्त newAccessGate(nodep, मिच्छा, randModeVarp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        सचे (ध्रुव Ast_ग्रन्थिः_Sel* ध्रुव selp = VN_CAST(nodep, ग्रन्थिः_Sel)) {
            पटिनिवत्त newDistGate(selp->fromp(), randModeVarp, fl);
        }
        सचे (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            पटिनिवत्त newDistGate(selp->fromp(), randModeVarp, fl);
        }
        सचे (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
            पटिनिवत्त newDistGate(selp->fromp(), randModeVarp, fl);
        }
        सचे (ध्रुव AstCMethodHard* ध्रुव methodp = VN_CAST(nodep, CMethodHard)) {
            सचे (methodp->method() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                पटिनिवत्त newDistGate(methodp->fromp(), randModeVarp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        सचे (ध्रुव AstCond* ध्रुव condp = VN_CAST(nodep, Cond)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenGatep = newDistGate(condp->thenp(), randModeVarp, fl);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elseGatep = newDistGate(condp->elsep(), randModeVarp, fl);
            Ast_ग्रन्थिः_व्यञ्जकम्* armGatep = thenGatep;
            सचे (thenGatep->same_वृक्षः(elseGatep)) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(elseGatep), elseGatep);
            } अञ्ञथा {
                armGatep
                    = नूतन AstCond{fl, condp->condp()->clone_वृक्षः_Pure(मिच्छा), thenGatep, elseGatep};
            }
            पटिनिवत्त newGateOr(fl, newDistGate(condp->condp(), randModeVarp, fl), armGatep);
        }
        सचे (ध्रुव Ast_ग्रन्थिः_Uniop* ध्रुव uopp = VN_CAST(nodep, ग्रन्थिः_Uniop)) {
            पटिनिवत्त newDistGate(uopp->lhsp(), randModeVarp, fl);
        }
        सचे (ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव bopp = VN_CAST(nodep, ग्रन्थिः_Biop)) {
            पटिनिवत्त newGateOr(fl, newDistGate(bopp->lhsp(), randModeVarp, fl),
                             newDistGate(bopp->rhsp(), randModeVarp, fl));
        }
        सचे (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        पटिनिवत्त नूतन AstConst{fl, AstConst::BitTrue{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_व्यञ्जकम्* buildFrozenMembership(AstDist* distp,
                                             ध्रुव std::vector<DistBucket>& buckets) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* unionExprp = nullptr;
        अत्थाय (ध्रुव auto& bucket : buckets) {
            Ast_ग्रन्थिः_व्यञ्जकम्* termp = newDistMembershipTerm(distp, bucket.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ध्रुव तार्किक runtimeWeight
                = bucket.weightExprp->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref*) { पटिनिवत्त सच्च; });
            सचे (runtimeWeight) {
                AstNeq* ध्रुव nzp = नूतन AstNeq{fl, bucket.weightExprp->clone_वृक्षः_Pure(मिच्छा),
                                               नूतन AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(सच्च);
                termp = नूतन AstLogAnd{fl, termp, nzp};
                termp->user1(सच्च);
            }
            सचे (unionExprp) {
                unionExprp = नूतन AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(सच्च);
            } अञ्ञथा {
                unionExprp = termp;
            }
        }
        पटिनिवत्त नूतन Ast_Constraint_व्यञ्जकम्{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_ग्रन्थिः* gateFrozenDist(Ast_ग्रन्थिः* chainp, AstDist* distp,
                            ध्रुव std::vector<DistBucket>& buckets, AstVar* randModeVarp) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* gatep = newDistGate(distp->exprp(), randModeVarp, fl);
        सचे (VN_IS(gatep, Const)) {
            ध्रुव तार्किक drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(प्रक्षिप_Deletep(gatep), gatep);
            सचे (drawable) पटिनिवत्त chainp;
            VL_DO_DANGLING(प्रक्षिप_Deletep(chainp), chainp);
            पटिनिवत्त buildFrozenMembership(distp, buckets);
        }
        पटिनिवत्त नूतन AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newUniformRangePick(AstDist* distp, ध्रुव AstInsideRange* irp) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dist_व्यञ्जकम्_Copyp = distp->exprp()->clone_वृक्षः_Pure(मिच्छा);
        dist_व्यञ्जकम्_Copyp->user1(सच्च);
        ध्रुव पूर्णाङ्क distWidth = distp->exprp()->width();
        ध्रुव AstConst* ध्रुव lopC = VN_CAST(irp->lhsp(), Const);
        ध्रुव AstConst* ध्रुव hipC = VN_CAST(irp->rhsp(), Const);
        Ast_ग्रन्थिः_व्यञ्जकम्* rangeSzp;
        सचे (lopC && hipC) {
            ध्रुव uint64_t rsz = hipC->toUQuad() - lopC->toUQuad() + 1;
            rangeSzp = नूतन AstConst{fl, AstConst::Unsized64{}, rsz};
        } अञ्ञथा {
            ध्रुव तार्किक isSigned = irp->lhsp()->isSigned();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lo64p
                = isSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                 नूतन AstExtendS{fl, irp->lhsp()->clone_वृक्षः_Pure(मिच्छा), 64})
                           : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                 नूतन AstExtend{fl, irp->lhsp()->clone_वृक्षः_Pure(मिच्छा), 64});
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव hi64p
                = isSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                 नूतन AstExtendS{fl, irp->rhsp()->clone_वृक्षः_Pure(मिच्छा), 64})
                           : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                 नूतन AstExtend{fl, irp->rhsp()->clone_वृक्षः_Pure(मिच्छा), 64});
            rangeSzp = नूतन AstAdd{fl, नूतन AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  नूतन AstSub{fl, hi64p, lo64p}};
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rand64p = नूतन AstRand{fl, nullptr, मिच्छा};
        rand64p->dtypeSetUInt64();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव offsetp
            = नूतन AstCCast{fl, नूतन AstModDiv{fl, rand64p, rangeSzp}, distWidth};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = नूतन AstAdd{fl, irp->lhsp()->clone_वृक्षः_Pure(मिच्छा), offsetp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = नूतन AstEq{fl, dist_व्यञ्जकम्_Copyp, valuep};
        eqp->user1(सच्च);
        पटिनिवत्त eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_ग्रन्थिः* buildWeightedBucketChain(AstDist* distp, ध्रुव std::vector<DistBucket>& buckets,
                                      AstVar* bucketVarp,
                                      ध्रुव std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*>& cumSums) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* chainp = nullptr;
        अत्थाय (पूर्णाङ्क i = static_cast<पूर्णाङ्क>(buckets.size()) - 1; i >= 0; --i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* constraintExprp;
            ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(buckets[i].rangep, InsideRange);
            सचे (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } अञ्ञथा सचे (irp) {
                constraintExprp = newUniformRangePick(distp, irp);
            } अञ्ञथा {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dist_व्यञ्जकम्_Copyp = distp->exprp()->clone_वृक्षः_Pure(मिच्छा);
                dist_व्यञ्जकम्_Copyp->user1(सच्च);
                constraintExprp
                    = नूतन AstEq{fl, dist_व्यञ्जकम्_Copyp, buckets[i].rangep->clone_वृक्षः_Pure(मिच्छा)};
                constraintExprp->user1(सच्च);
            }
            सचे (!chainp) {
                chainp = constraintExprp;
            } अञ्ञथा {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp
                    = नूतन AstLte{fl, नूतन AstVarRef{fl, bucketVarp, VAccess::READ}, cumSums[i]};
                condp->user1(सच्च);
                chainp = नूतन AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(सच्च);
            }
        }
        Ast_Constraint_व्यञ्जकम्* ध्रुव chainExprp = नूतन Ast_Constraint_व्यञ्जकम्{fl, chainp};
        chainExprp->isSoft(सच्च);
        पटिनिवत्त chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    शून्य lowerDistConstraints(AstTask* taskp, Ast_ग्रन्थिः* constrItemsp, AstVar* randModeVarp,
                              AstConstraintForeach* foreachp = nullptr) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_ग्रन्थिः* foreachTailp = nullptr;
        auto addStmt = [&](Ast_ग्रन्थिः* nodep) {
            सचे (foreachp) {
                सचे (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } अञ्ञथा {
                    foreachTailp->addNext(nodep);
                    foreachTailp = nodep;
                }
            } अञ्ञथा {
                taskp->addStmtsp(nodep);
            }
        };

        अत्थाय (Ast_ग्रन्थिः *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            सचे (AstConstraintIf* ध्रुव cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                lowerDistConstraints(taskp, cifp->thensp(), randModeVarp, foreachp);
                सचे (cifp->elsesp()) {
                    lowerDistConstraints(taskp, cifp->elsesp(), randModeVarp, foreachp);
                }
                अनुवर्त;
            }
            सचे (AstConstraintForeach* ध्रुव cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                lowerDistConstraints(taskp, cfep->bodyp(), randModeVarp, cfep);
                अनुवर्त;
            }

            Ast_Constraint_व्यञ्जकम्* ध्रुव constrExprp = VN_CAST(itemp, Constraint_व्यञ्जकम्);
            सचे (!constrExprp) अनुवर्त;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            सचे (AstLogIf* ध्रुव शिखरम्_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_ग्रन्थिः* chainEndp = शिखरम्_Log_Ifp->rhsp();
                याव (AstLogIf* ध्रुव innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                सचे (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ध्रुव liftedp = liftLogIfChainToConstraintIf(शिखरम्_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(constrExprp), constrExprp);
                    lowerDistConstraints(taskp, liftedp->thensp(), randModeVarp, foreachp);
                    अनुवर्त;
                }
            }

            AstDist* ध्रुव distp = VN_CAST(constrExprp->exprp(), Dist);
            सचे (!distp) अनुवर्त;

            File_पङ्क्तिः* ध्रुव fl = distp->fileline();

            std::vector<DistBucket> buckets = collectDistBuckets(distp);

            सचे (buckets.empty()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_व्यञ्जकम्* ध्रुव truep
                    = नूतन Ast_Constraint_व्यञ्जकम्{fl, नूतन AstConst{fl, AstConst::BitTrue{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(constrExprp), constrExprp);
                अनुवर्त;
            }

            Ast_Constraint_व्यञ्जकम्* ध्रुव membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_ग्रन्थिः_व्यञ्जकम्* totalWeightExprp = nullptr;
            अत्थाय (auto& bucket : buckets) {
                सचे (!totalWeightExprp) {
                    totalWeightExprp = bucket.weightExprp->clone_वृक्षः_Pure(मिच्छा);
                } अञ्ञथा {
                    totalWeightExprp = नूतन AstAdd{fl, totalWeightExprp,
                                                  bucket.weightExprp->clone_वृक्षः_Pure(मिच्छा)};
                    totalWeightExprp->dtypeSetUInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ध्रुव पूर्णाङ्क distId = m_dist_अङ्कः++;
            ध्रुव std::string totalName = "__Vdist_total" + cvtToStr(distId);
            AstVar* ध्रुव totalVarp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP, totalName, taskp->findUInt64DType()};
            totalVarp->noSubst(सच्च);
            totalVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            totalVarp->funcLocal(सच्च);
            totalVarp->isInternal(सच्च);
            addStmt(totalVarp);
            addStmt(
                नूतन AstAssign{fl, नूतन AstVarRef{fl, totalVarp, VAccess::WRITE}, totalWeightExprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ध्रुव std::string bucketName = "__Vdist_bucket" + cvtToStr(distId);
            AstVar* ध्रुव bucketVarp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP, bucketName, taskp->findUInt64DType()};
            bucketVarp->noSubst(सच्च);
            bucketVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            bucketVarp->funcLocal(सच्च);
            bucketVarp->isInternal(सच्च);
            addStmt(bucketVarp);

            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव randp = नूतन AstRand{fl, nullptr, मिच्छा};
            randp->dtypeSetUInt64();
            addStmt(नूतन AstAssign{
                fl, नूतन AstVarRef{fl, bucketVarp, VAccess::WRITE},
                नूतन AstAdd{
                    fl, नूतन AstConst{fl, AstConst::Unsized64{}, 1},
                    नूतन AstModDiv{fl, randp, नूतन AstVarRef{fl, totalVarp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> cumSums;
            Ast_ग्रन्थिः_व्यञ्जकम्* runningSump = nullptr;
            अत्थाय (size_t i = 0; i < buckets.size(); ++i) {
                सचे (!runningSump) {
                    runningSump = buckets[i].weightExprp->clone_वृक्षः_Pure(मिच्छा);
                } अञ्ञथा {
                    runningSump = नूतन AstAdd{fl, runningSump,
                                             buckets[i].weightExprp->clone_वृक्षः_Pure(मिच्छा)};
                    runningSump->dtypeSetUInt64();
                }
                cumSums.प्रक्षिप_back(runningSump->clone_वृक्षः_Pure(सच्च));
            }

            Ast_ग्रन्थिः* chainp = buildWeightedBucketChain(distp, buckets, bucketVarp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, randModeVarp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->addHereThisAsNext(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            अत्थाय (auto& bucket : buckets) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(bucket.weightExprp), bucket.weightExprp);
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            प्रक्षिप_Deletep(cumSums.back());
        }
    }

    स्थिर तार्किक isDynArrOfClassTypeRecurse(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव refp = dtypep->skipRefp();
        सचे (VN_IS(refp, DynArrayDType) || VN_IS(refp, QueueDType)) {
            पटिनिवत्त isDynArrOfClassTypeRecurse(refp->subDTypep());
        } अञ्ञथा सचे (VN_IS(refp, ClassRefDType)) {
            पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    शून्य addArrayElemRefresh(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव arrVarp, AstFunc* ध्रुव randomizep,
                             AstVar* ध्रुव genp) {
        Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(genp->user2p(), ग्रन्थिः_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ध्रुव uint32_t unpackedDims = arrVarp->dtypep()->dimensions(मिच्छा).second;
        सचे (isDynArrOfClassTypeRecurse(arrVarp->dtypep())) {
            सचे (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            पटिनिवत्त;
        }
        AstCMethodHard* ध्रुव methodp
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();

        Ast_ग्रन्थिः_Module* ध्रुव classp = VN_AS(arrVarp->user2p(), ग्रन्थिः_Module);
        AstVarRef* ध्रुव varRefp = नूतन AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->addPinsp(varRefp);

        ध्रुव size_t width = arrayElementDTypep(arrVarp->dtypep())->width();

        methodp->addPinsp(नूतन AstConst{fl, AstConst::Unsized64{}, width});
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varnamep
            = नूतन AstCExpr{fl, AstCExpr::Pure{}, "\"" + arrVarp->name() + "\"", arrVarp->width()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(नूतन AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ध्रुव RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        सचे (rmode.usesMode) {
            methodp->addPinsp(नूतन AstConst{fl, AstConst::Unsized64{}, rmode.index});
        }

        randomizep->addStmtsp(methodp->makeStmt());
    }

    Ast_ग्रन्थिः_Stmt* newPinSizeVariable(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव arrVarp,
                                    AstVar* ध्रुव genp) {
        Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(genp->user2p(), ग्रन्थिः_Module);
        AstVar* ध्रुव sizeVarp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ध्रुव pinp
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtypeSetVoid();
        AstCExpr* ध्रुव namep = नूतन AstCExpr{fl, AstCExpr::Pure{}, "\"" + sizeVarp->name() + "\""};
        namep->dtypeSetUInt32();
        pinp->addPinsp(namep);
        pinp->addPinsp(
            नूतन AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(sizeVarp->width())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ध्रुव sizeVarRefp = नूतन AstVarRef{fl, sizeVarp, VAccess::READ};
        sizeVarRefp->classOrPackagep(VN_AS(sizeVarp->user2p(), ग्रन्थिः_Module));
        pinp->addPinsp(sizeVarRefp);
        पटिनिवत्त pinp->makeStmt();
    }

    शून्य addFrozenSizePin(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव arrVarp, AstFunc* ध्रुव randomizep,
                          AstVar* ध्रुव genp, AstVar* ध्रुव randModeVarp) {
        ध्रुव RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        सचे (!rmode.usesMode) पटिनिवत्त;
        AstVar* ध्रुव sizeVarp = VN_AS(arrVarp->user4p(), Var);
        Ast_ग्रन्थिः_Module* ध्रुव arrClassp = VN_AS(arrVarp->user2p(), ग्रन्थिः_Module);
        Ast_ग्रन्थिः_Module* ध्रुव sizeClassp = VN_AS(sizeVarp->user2p(), ग्रन्थिः_Module);
        AstVarRef* ध्रुव sizeWritep = नूतन AstVarRef{fl, sizeClassp, sizeVarp, VAccess::WRITE};
        ध्रुव VCMethod sizeMethod = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_SIZE
                                        : VCMethod::DYN_SIZE;
        AstCMethodHard* ध्रुव currentSizep = नूतन AstCMethodHard{
            fl, नूतन AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, sizeMethod};
        currentSizep->dtypep(sizeVarp->dtypep());
        randomizep->addStmtsp(नूतन AstAssign{fl, sizeWritep, currentSizep});
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव modep = newModeBitRead(arrVarp, nullptr, randModeVarp, fl);
        randomizep->addStmtsp(
            नूतन AstIf{fl, नूतन AstLogNot{fl, modep}, newPinSizeVariable(fl, arrVarp, genp)});
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_अङ्कः);
        VL_RESTORER_COPY(m_randArrayUniqueNames);
        m_modp = nodep;
        m_rand_Case_अङ्कः = 0;
        m_randArrayUniqueNames.reset();
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_अङ्कः);
        VL_RESTORER(m_dist_अङ्कः);
        m_modp = nodep;
        m_rand_Case_अङ्कः = 0;
        m_dist_अङ्कः = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        सचे (!nodep->user1()) पटिनिवत्त;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->baseMostClassp()->needRNG(सच्च);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ध्रुव तार्किक basicFirst = classOwnsGlobalConstraint(nodep);

        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstFunc* ध्रुव randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ध्रुव fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ध्रुव randModeVarp = getRandModeVarFromClass(nodep);
        addPrePostCall(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        सचे (classHasRandClassMembers(nodep)) {
            AstTask* ध्रुव preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->addStmtsp((नूतन AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_ग्रन्थिः_व्यञ्जकम्* beginValp = nullptr;
        AstVar* genp = getRandomGenerator(nodep);
        सचे (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ध्रुव prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ध्रुव classp, AstConstraint* ध्रुव constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                सचे (!taskp) {
                    taskp = newSetupConstraintTask(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                सचे (AstTask* ध्रुव resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ध्रुव resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ध्रुव resizeTaskRefp
                        = नूतन AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->addStmtsp(resizeTaskRefp->makeStmt());
                }

                सचे (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                सचे (constrp->itemsp()) {
                    lowerDistConstraints(taskp, constrp->itemsp(), randModeVarp);
                }
                VInsertionSet<AstVar*>& sizeArrays = m_sizeConstrainedArrays[classp];
                Constraint_व्यञ्जकम्_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), nullptr,
                    genp,          randModeVarp, m_writtenVars,     &m_uniqueConstraintId,
                    m_uniqueNames, randomizep,   &sizeArrays,       prepareArraysTaskp};
                सचे (constrp->itemsp()) {
                    taskp->addStmtsp(wrapIfConstraintMode(
                        nodep, constrp, constrp->itemsp()->unlinkFrBackWithNext()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ध्रुव auto constraintDepth = [](ध्रुव AstConstraint* constrp) {
                    पूर्णाङ्क depth = 0;
                    size_t pos = 0;
                    याव ((pos = constrp->name().find(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++depth;
                        pos += std::strlen(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    पटिनिवत्त depth;
                };
                पूर्णाङ्क maxDepth = 0;
                nodep->foreachMember([&](AstClass* ध्रुव, AstConstraint* ध्रुव constrp) {
                    maxDepth = std::max(maxDepth, constraintDepth(constrp));
                });
                AstTask* ध्रुव setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
                अत्थाय (पूर्णाङ्क d = maxDepth; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ध्रुव classp, AstConstraint* ध्रुव constrp) {
                            सचे (constraintDepth(constrp) != d) पटिनिवत्त;
                            AstTask* ध्रुव taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ध्रुव setupTaskRefp
                                = नूतन AstTaskRef{constrp->fileline(), taskp};
                            setupTaskRefp->classOrPackagep(classp);
                            setupAllTaskp->addStmtsp(setupTaskRefp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertionSet<AstVar*> sizeArrayVars;
            सचे (nodep->extendsp()) {
                AstClass* parentClassp = nodep->extendsp()->classp();
                याव (parentClassp) {
                    ध्रुव auto sizeArraysIt = m_sizeConstrainedArrays.find(parentClassp);
                    सचे (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                        अत्थाय (AstVar* ध्रुव arrVarp : sizeArraysIt->second) {
                            sizeArrayVars.निवेशय(arrVarp);
                        }
                    }
                    AstFunc* ध्रुव parentRandomizep
                        = VN_CAST(m_memberMap.findMember(parentClassp, "randomize"), Func);
                    सचे (parentRandomizep && parentRandomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        अत्थाय (Ast_ग्रन्थिः* stmtp = parentRandomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            तार्किक foundClearConstraints = मिच्छा;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                सचे (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->addStmtsp(stmtp->clone_वृक्षः(मिच्छा));
                                } अञ्ञथा सचे (methodp->method()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    foundClearConstraints = सच्च;
                                }
                            });
                            सचे (foundClearConstraints) भङ्ग;
                        }
                    }
                    parentClassp
                        = parentClassp->extendsp() ? parentClassp->extendsp()->classp() : nullptr;
                }
            }
            randomizep->addStmtsp(implementConstraintsClear(fl, genp));

            ध्रुव auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
            सचे (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                अत्थाय (AstVar* ध्रुव arrVarp : sizeArraysIt->second) sizeArrayVars.निवेशय(arrVarp);
            }
            AstTask* ध्रुव setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
            सचे (!sizeArrayVars.empty() && prepareArraysTaskp->stmtsp()) {
                Ast_ग्रन्थिः_Stmt* ध्रुव preparep = (नूतन AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                अत्थाय (Ast_ग्रन्थिः* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    तार्किक writesArray = मिच्छा;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    सचे (!writesArray) अनुवर्त;
                    stmtp->addHereThisAsNext(preparep);
                    भङ्ग;
                }
            }
            AstTaskRef* ध्रुव setupTaskRefp = नूतन AstTaskRef{fl, setupAllTaskp};
            randomizep->addStmtsp(setupTaskRefp->makeStmt());

            Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(genp->user2p(), ग्रन्थिः_Module);

            AstCExpr* ध्रुव solverCallp = नूतन AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(नूतन AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->add(".next(__Vm_rng)");
            सचे (!sizeArrayVars.empty()) {
                AstVar* ध्रुव sizeOkVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                sizeOkVarp->funcLocal(सच्च);
                randomizep->addStmtsp(sizeOkVarp);

                AstVar* ध्रुव finalOkVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                finalOkVarp->funcLocal(सच्च);
                randomizep->addStmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                अत्थाय (AstVar* ध्रुव arrVarp : sizeArrayVars)
                    addFrozenSizePin(fl, arrVarp, randomizep, genp, randModeVarp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->addStmtsp(
                    नूतन AstAssign{fl, नूतन AstVarRef{fl, sizeOkVarp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                सचे (AstTask* ध्रुव resizeAllTaskp
                    = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ध्रुव resizeTaskRefp = नूतन AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ध्रुव ifp = नूतन AstIf{fl, नूतन AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), nullptr};
                    randomizep->addStmtsp(ifp);
                }

                // Refresh array element tables after resize
                अत्थाय (ध्रुव auto& arrVarp : sizeArrayVars) {
                    addArrayElemRefresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->addStmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ध्रुव setupTaskRefp2 = नूतन AstTaskRef{fl, setupAllTaskp};
                randomizep->addStmtsp(setupTaskRefp2->makeStmt());

                अत्थाय (AstVar* ध्रुव arrVarp : sizeArrayVars) {
                    randomizep->addStmtsp(newPinSizeVariable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ध्रुव solverCallp2 = नूतन AstCExpr{fl};
                solverCallp2->dtypeSetBit();
                solverCallp2->add(नूतन AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->add(".next(__Vm_rng)");
                randomizep->addStmtsp(नूतन AstAssign{
                    fl, नूतन AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = नूतन AstAnd{fl, नूतन AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                       नूतन AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } अञ्ञथा {
                beginValp = solverCallp;
            }
            सचे (randModeVarp) {
                Ast_ग्रन्थिः_Module* ध्रुव randModeClassp = VN_AS(randModeVarp->user2p(), Class);
                Ast_ग्रन्थिः_FTask* ध्रुव newp
                    = VN_AS(m_memberMap.findMember(randModeClassp, "new"), ग्रन्थिः_FTask);
                UASSERT_OBJ(newp, randModeClassp, "No new() in class");
                addSetRandMode(newp, genp, randModeVarp);
            }
            सचे (AstVar* ध्रुव staticRandModeVarp = getStaticRandModeVar(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(m_memberMap.findMember(nodep, "new"), ग्रन्थिः_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                addSetStaticRandMode(newp, genp, staticRandModeVarp);
            }
        } अञ्ञथा {
            beginValp = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1};
        }

        addUpdateRandVarsBody(nodep, randomizep);

        AstFunc* ध्रुव basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        addBasicRandomizeBody(basicRandomizep, nodep, randModeVarp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ध्रुव fvarRefp = नूतन AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->addStmtsp(नूतन AstAssign{
            fl, fvarRefp, basicFirst ? नूतन AstFuncRef{fl, basicRandomizep} : beginValp});

        ध्रुव auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
        ध्रुव तार्किक needsSizePhase
            = sizeArraysIt != m_sizeConstrainedArrays.end() && !sizeArraysIt->second.empty();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ध्रुव auto emitResizeFallback = [&]() {
            सचे (needsSizePhase) पटिनिवत्त;
            सचे (AstTask* ध्रुव resizeAllTaskp
                = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ध्रुव resizeTaskRefp = नूतन AstTaskRef{fl, resizeAllTaskp};
                randomizep->addStmtsp(resizeTaskRefp->makeStmt());
            }
        };
        सचे (!basicFirst) emitResizeFallback();

        AstVarRef* ध्रुव fvarRefReadp = fvarRefp->clone_वृक्षः(मिच्छा);
        fvarRefReadp->access(VAccess::READ);

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव secondHalfp
            = basicFirst ? beginValp : नूतन AstFuncRef{fl, basicRandomizep};
        randomizep->addStmtsp(नूतन AstAssign{fl, fvarRefp->clone_वृक्षः(मिच्छा),
                                            नूतन AstAnd{fl, fvarRefReadp, secondHalfp}});

        सचे (basicFirst) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        सचे (classHasRandClassMembers(nodep)) {
            AstTask* ध्रुव postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->addStmtsp((नूतन AstTaskRef{fl, postTaskp})->makeStmt());
        }

        addPrePostCall(nodep, randomizep, "post_randomize");
        nodep->user1(मिच्छा);
    }
    शून्य visit(AstRandCase* nodep) override {
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
        Ast_ग्रन्थिः_DType* ध्रुव sumDTypep = nodep->findUInt64DType();

        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        ध्रुव std::string name = "__Vrandcase" + cvtToStr(m_rand_Case_अङ्कः++);
        AstVar* ध्रुव randVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, name, sumDTypep};
        randVarp->noSubst(सच्च);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        addTempVarsp(randVarp);
        Ast_ग्रन्थिः_व्यञ्जकम्* sump = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 64, 0};
        Ast_ग्रन्थिः_If* ध्रुव firstIfsp
            = नूतन AstIf{fl, नूतन AstConst{fl, AstConst::BitFalse{}}, nullptr, nullptr};
        Ast_ग्रन्थिः_If* ifsp = firstIfsp;

        अत्थाय (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = itemp->condsp()->unlinkFrBack();
            sump
                = नूतन AstAdd{condp->fileline(), sump, नूतन AstExtend{itemp->fileline(), condp, 64}};
            Ast_ग्रन्थिः* ध्रुव stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlinkFrBackWithNext() : nullptr;
            AstVarRef* ध्रुव randVarRefp = नूतन AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_ग्रन्थिः_If* ध्रुव newifp
                = नूतन AstIf{itemp->fileline(),
                            नूतन AstLte{condp->fileline(), randVarRefp, sump->clone_वृक्षः_Pure(सच्च)},
                            stmtsp, nullptr};
            ifsp->addElsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ध्रुव dispp = नूतन AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            nullptr, nullptr};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->addElsesp(dispp);

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव randp = नूतन AstRand{fl, nullptr, मिच्छा};
        randp->dtypeSetUInt64();
        AstVarRef* ध्रुव randVarRefp = नूतन AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_ग्रन्थिः* ध्रुव newp
            = नूतन AstAssign{fl, randVarRefp,
                            नूतन AstAdd{fl, नूतन AstConst{fl, AstConst::Unsized64{}, 1},
                                       नूतन AstModDiv{fl, randp, sump}}};
        newp->addNext(firstIfsp);
        सचे (debug() >= 9) newp->dump_वृक्षः_And_Next(cout, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        सचे (nodep->name() == "rand_mode") {
            AstMethodCall* ध्रुव methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = methodCallp ? methodCallp->fromp() : nullptr;
            ध्रुव RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_modp);
            UASSERT_OBJ(randModeTarget.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ध्रुव receiverp = randModeTarget.receiverp;
            ध्रुव तार्किक isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_ग्रन्थिः* classLevelStaticLoopp = nullptr;
            सचे (isClassLevel && nodep->argsp()) {
                सचे (AstVar* ध्रुव sVarp = getStaticRandModeVar(randModeTarget.classp)) {
                    File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव staticLhsp
                        = makeModeAssignLhs(fl, randModeTarget.classp, nullptr, sVarp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argClonep = nodep->argsp()->exprp()->clone_वृक्षः_Pure(मिच्छा);
                    classLevelStaticLoopp = makeModeSetLoop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ध्रुव तार्किक receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ध्रुव randModeVarp = receiverIsStaticRand
                                             ? getStaticRandModeVar(randModeTarget.classp)
                                             : getRandModeVarFromClass(randModeTarget.classp);
            सचे (!randModeVarp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_व्यञ्जकम्), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(प्रक्षिप_Deletep(m_stmtp), m_stmtp = nullptr);
                पटिनिवत्त;
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = makeModeAssignLhs(nodep->fileline(), randModeTarget.classp,
                                                        randModeTarget.fromp, randModeVarp);
            replaceWithModeAssignAndAppend(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : nullptr, lhsp,
                classLevelStaticLoopp);
            पटिनिवत्त;
        }

        सचे (nodep->name() == "constraint_mode") {
            AstMethodCall* ध्रुव methodCallp = VN_CAST(nodep, MethodCall);
            Ast_ग्रन्थिः_व्यञ्जकम्* fromp = methodCallp ? methodCallp->fromp() : nullptr;
            AstConstraint* constrp = nullptr;
            AstClass* classp = VN_CAST(m_modp, Class);
            सचे (AstConstraintRef* ध्रुव constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                सचे (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } अञ्ञथा सचे (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ध्रुव constraintModeVarp = (constrp && constrp->isStatic())
                                                   ? getStaticConstraintModeVar(classp)
                                                   : getConstraintModeVar(classp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp
                = makeModeAssignLhs(nodep->fileline(), classp, fromp, constraintModeVarp);
            replaceWithModeAssign(nodep, constrp, lhsp);
            पटिनिवत्त;
        }

        सचे (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            पटिनिवत्त;
        }

        सचे (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ध्रुव तार्किक inStaticContext = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ध्रुव stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ध्रुव randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            सचे (inStaticContext) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(सच्च);
            }
            randomizeFuncp->addStmtsp(
                नूतन AstAssign{nodep->fileline(),
                              नूतन AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              नूतन AstConst{nodep->fileline(), AstConst::Widthed_मानम्{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            पूर्णाङ्क argn = 0;
            AstWith* ध्रुव withp = nodep->withp();
            अत्थाय (ध्रुव AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = argp->exprp();
                AstCMethodHard* ध्रुव basicMethodp = नूतन AstCMethodHard{
                    nodep->fileline(),
                    नूतन AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ध्रुव refvarp
                    = नूतन AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::to_string(++argn), exprp->dtypep()};
                refvarp->setStdRandomizeArg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                सचे (withp) {
                    withp->foreach([&](Ast_ग्रन्थिः_व्यञ्जकम्* exp) {
                        सचे (is_Similar_ग्रन्थिः(exp, exprp)) {
                            AstVarRef* ध्रुव replaceVar
                                = नूतन AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            exp->replaceWith(replaceVar);
                            replaceVar->user1(exp->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(प्रक्षिप_Deletep(exp), exp);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(सच्च);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->addStmtsp(refvarp);

                ध्रुव size_t width = exprp->width();
                basicMethodp->addPinsp(
                    नूतन AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->addPinsp(
                    नूतन AstConst{nodep->fileline(), AstConst::Unsized64{}, width});
                basicMethodp->dtypeSetBit();

                randomizeFuncp->addStmtsp(नूतन AstAssign{
                    nodep->fileline(),
                    नूतन AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    नूतन AstAnd{nodep->fileline(),
                               नूतन AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            सचे (withp) {
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, nullptr,
                                                                inStaticContext);
                withCapturep->addFunctionArguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                सचे (stdrand) {
                    randomizeFuncp->addStmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_ग्रन्थिः* ध्रुव capturedTreep = withp->exprp()->unlinkFrBackWithNext();
                randomizeFuncp->addStmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_व्यञ्जकम्_Visitor{
                        nullptr, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        nullptr, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
                }
                AstCExpr* ध्रुव solverCallp = नूतन AstCExpr{fl};
                solverCallp->dtypeSetBit();
                solverCallp->add(नूतन AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->add(".next()");
                AstVar* ध्रुव fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव andExprp
                    = नूतन AstAnd{fl, नूतन AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->addStmtsp(
                    नूतन AstAssign{fl, नूतन AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlinkFrBack()->delete_वृक्षः(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            सचे (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            सचे (withCapturep) nodep->addArgsp(withCapturep->getArgs());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            पटिनिवत्त;
        }
        handleRandomizeArgs(nodep);

        AstWith* ध्रुव withp = nodep->withp();
        सचे (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            पटिनिवत्त;
        }
        withp->unlinkFrBack();
        iterateChildren(nodep);

        AstClass* classp = nullptr;
        सचे (AstMethodCall* ध्रुव callp = VN_CAST(nodep, MethodCall)) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            ध्रुव AstClassRefDType* ध्रुव classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } अञ्ञथा {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        सचे (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ध्रुव classGenp = getRandomGenerator(classp);
        AstVar* ध्रुव localGenp
            = नूतन AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(सच्च);

        AstFunc* ध्रुव randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inlineUniqueNames.get(nodep), मिच्छा);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ध्रुव std::pair<तार्किक, तार्किक> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<तार्किक, तार्किक>{मिच्छा, मिच्छा};
        सचे (prePostWrap.first) {
            addVirtualPrePostCall(randomizeFuncp, classp, "pre_randomize");
        } अञ्ञथा {
            addPrePostCall(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        सचे (classHasRandClassMembers(classp)) {
            AstTask* ध्रुव preTaskp = getCreateNestedCallbackTask(classp, "pre");
            सचे (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->addStmtsp((नूतन AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ध्रुव CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.addFunctionArguments(randomizeFuncp);

        // Add constraints clearing code
        सचे (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ध्रुव mainRandomizep
                = VN_CAST(m_memberMap.findMember(classp, "randomize"), Func);
            सचे (mainRandomizep && mainRandomizep->stmtsp()) {
                अत्थाय (Ast_ग्रन्थिः* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    तार्किक foundClearConstraints = मिच्छा;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        सचे (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->addStmtsp(stmtp->clone_वृक्षः(मिच्छा));
                        } अञ्ञथा सचे (methodp->method() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            foundClearConstraints = सच्च;
                        }
                    });
                    सचे (foundClearConstraints) भङ्ग;
                }
            }
            randomizeFuncp->addStmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->addStmtsp(localGenp);

        AstFunc* ध्रुव basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ध्रुव basicRandomizeFuncCallp
            = नूतन AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        सचे (classGenp) {
            AstTask* ध्रुव constrSetupFuncp = getCreateConstraintSetupFunc(classp, m_memberMap);
            AstTaskRef* ध्रुव callp = नूतन AstTaskRef{nodep->fileline(), constrSetupFuncp};
            randomizeFuncp->addStmtsp(callp->makeStmt());
            randomizeFuncp->addStmtsp(नूतन AstAssign{
                nodep->fileline(), नूतन AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                नूतन AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), ग्रन्थिः_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ध्रुव randModeVarp = getRandModeVarFromClass(classp);
        सचे (!classGenp && randModeVarp) addSetRandMode(randomizeFuncp, localGenp, randModeVarp);
        सचे (!classGenp) {
            सचे (AstVar* ध्रुव sVarp = getStaticRandModeVar(classp)) {
                addSetStaticRandMode(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_ग्रन्थिः* ध्रुव capturedTreep = withp->exprp()->unlinkFrBackWithNext();
        randomizeFuncp->addStmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_ग्रन्थिः* inlineResizeStmtsp = nullptr;
        {
            std::vector<AstCMethodHard*> sizeMethodps;
            capturedTreep->foreachAndNext([&](AstCMethodHard* methodp) {
                सचे (methodp->method() == VCMethod::DYN_SIZE
                    || methodp->method() == VCMethod::ASSOC_SIZE) {
                    sizeMethodps.प्रक्षिप_back(methodp);
                }
            });
            अत्थाय (AstCMethodHard* ध्रुव methodp : sizeMethodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = nullptr;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मूलम्_Exprp = methodp->fromp();
                सचे (AstMemberSel* memberSelp = VN_CAST(मूलम्_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    याव (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = memberSelp->fromp();
                    सचे (ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(fromp, VarRef)) {
                        सचे (varRefp->varp() == captured.getThisp()) अनुवर्त;
                    } अञ्ञथा {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } अञ्ञथा सचे (AstVarRef* ध्रुव varRefp = VN_CAST(मूलम्_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                सचे (!arrVarp) अनुवर्त;
                // Only handle rand-declared dynamic/assoc array variables
                सचे (!arrVarp->rand().isRandomizable()) अनुवर्त;
                // Do not randomize associative array size (IEEE 18.4)
                सचे (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) अनुवर्त;
                File_पङ्क्तिः* ध्रुव fl = methodp->fileline();
                तार्किक wasCreated = मिच्छा;
                AstClass* ध्रुव varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ध्रुव sizeVarp = createOrGetSizeVar(varClassp, arrVarp, fl,
                                                            methodp->findIntDType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                सचे (wasCreated) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resizeVarRefp = buildMemberSelExprp(मूलम्_Exprp, arrVarp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sizeExprp = buildMemberSelExprp(मूलम्_Exprp, sizeVarp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->addNext(createSizeGteZeroConstraint(fl, sizeVarp, मूलम्_Exprp));
                    AstCMethodHard* ध्रुव resizep
                        = नूतन AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, sizeExprp};
                    resizep->dtypep(methodp->findVoidDType());
                    inlineResizeStmtsp
                        = Ast_ग्रन्थिः::addNext(inlineResizeStmtsp, नूतन Ast_Stmt_व्यञ्जकम्{fl, resizep});
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव size_ग्रन्थिः_Exprp = buildMemberSelExprp(मूलम्_Exprp, sizeVarp);
                methodp->replaceWith(size_ग्रन्थिः_Exprp);
                VL_DO_DANGLING(methodp->delete_वृक्षः(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_व्यञ्जकम्_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                randModeVarp, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
        }

        // Call the solver and set return value
        AstCExpr* ध्रुव solverCallp = नूतन AstCExpr{nodep->fileline()};
        solverCallp->dtypeSetBit();
        solverCallp->add(नूतन AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->add(".next(__Vm_rng)");
        randomizeFuncp->addStmtsp(नूतन AstAssign{
            nodep->fileline(),
            नूतन AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            नूतन AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        सचे (inlineResizeStmtsp) {
            AstVar* ध्रुव retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ध्रुव ifp = नूतन AstIf{nodep->fileline(),
                                         नूतन AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, nullptr};
            randomizeFuncp->addStmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        सचे (classHasRandClassMembers(classp)) {
            AstTask* ध्रुव postTaskp = getCreateNestedCallbackTask(classp, "post");
            सचे (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->addStmtsp((नूतन AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        सचे (prePostWrap.second) {
            addVirtualPrePostCall(randomizeFuncp, classp, "post_randomize");
        } अञ्ञथा {
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
        VL_DO_DANGLING(withp->delete_वृक्षः(), withp);
    }
    शून्य visit(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        सचे (m_constraintp && nodep->fromp()->user1()
            && (nodep->method() == VCMethod::ASSOC_SIZE
                || nodep->method() == VCMethod::DYN_SIZE)) {
            AstClass* ध्रुव classp = VN_AS(m_modp, Class);
            AstVarRef* ध्रुव queueVarRefp = VN_CAST(nodep->fromp(), VarRef);
            सचे (!queueVarRefp) {
                // Warning from ConstraintExprVisitor will be thrown
                पटिनिवत्त;
            }
            AstVar* ध्रुव queueVarp = queueVarRefp->varp();
            तार्किक wasCreated = मिच्छा;
            AstVar* ध्रुव sizeVarp
                = createOrGetSizeVar(classp, queueVarp, fl, nodep->findIntDType(), wasCreated);
            सचे (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                सचे (!VN_IS(queueVarp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    सचे (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ध्रुव sizeVarRefp = नूतन AstVarRef{
                        fl, VN_AS(sizeVarp->user2p(), ग्रन्थिः_Module), sizeVarp, VAccess::READ};
                    AstCMethodHard* ध्रुव resizep = नूतन AstCMethodHard{
                        fl, nodep->fromp()->unlinkFrBack(), VCMethod::DYN_RESIZE, sizeVarRefp};
                    resizep->dtypep(nodep->findVoidDType());
                    resizerTaskp->addStmtsp(नूतन Ast_Stmt_व्यञ्जकम्{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->addItemsp(createSizeGteZeroConstraint(fl, sizeVarp));
            }
            AstVarRef* ध्रुव sizeVarRefp = नूतन AstVarRef{fl, VN_AS(sizeVarp->user2p(), ग्रन्थिः_Module),
                                                         sizeVarp, VAccess::READ};
            sizeVarRefp->user1(सच्च);
            nodep->replaceWith(sizeVarRefp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
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
        nodep->foreach([&](AstClass* ध्रुव classp) {
            सचे (!classp->user1()) पटिनिवत्त;
            classp->foreachMember([&](AstClass* ध्रुव ownerClassp, AstConstraint* ध्रुव constrp) {
                constrp->foreach([&](AstMemberSel* ध्रुव memberSelp) {
                    AstVar* ध्रुव varp = memberSelp->varp();
                    सचे (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) पटिनिवत्त;
                    सचे (VN_AS(varp->user2p(), ग्रन्थिः_Module) != ownerClassp) पटिनिवत्त;
                    सचे (!varp->user3()) varp->user3(सच्च);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(constrp->unlinkFrBack()), constrp);
        });
    }
    ~RandomizeVisitor() override = स्वभाव;
};

//######################################################################
// Randomize method class functions

शून्य V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ध्रुव RandomizeMarkVisitor markVisitor{nodep};
        ध्रुव RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_वृक्षः("randomize", 0, dump_वृक्षः_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ध्रुव std::string& name, तार्किक allowVirtual,
                                       तार्किक childDType) {
    AstFunc* funcp = VN_AS(memberMap.findMember(nodep, name), Func);
    सचे (!funcp) {
        v3Global.useRandomizeMethods(सच्च);
        Ast_ग्रन्थिः_DType* ध्रुव dtypep
            = childDType
                  ? नूतन AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ध्रुव fvarp = childDType
                                  ? नूतन AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlagChildDType{}, dtypep}
                                  : नूतन AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3ErrorCode::NORETURN, सच्च);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(सच्च);
        fvarp->funcReturn(सच्च);
        fvarp->direction(VDirection::OUTPUT);
        nodep->addMembersp(funcp);
        funcp = नूतन AstFunc{nodep->fileline(), name, nullptr, fvarp};
        सचे (!childDType) funcp->dtypep(dtypep);
        funcp->classMethod(सच्च);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->addMembersp(funcp);
        memberMap.निवेशय(nodep, funcp);
    }
    पटिनिवत्त funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_ग्रन्थिः_Module* nodep, ध्रुव std::string& name) {
    AstFunc* funcp = nullptr;
    v3Global.useRandomizeMethods(सच्च);
    Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
    AstVar* ध्रुव fvarp = नूतन AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(सच्च);
    fvarp->funcReturn(सच्च);
    fvarp->direction(VDirection::OUTPUT);
    funcp = नूतन AstFunc{nodep->fileline(), name, nullptr, fvarp};
    funcp->dtypep(dtypep);
    सचे (VN_IS(nodep, Class)) {
        funcp->classMethod(सच्च);
    } अञ्ञथा {
        funcp->classMethod(सच्च);
        funcp->isStatic(सच्च);
    }
    nodep->addStmtsp(funcp);
    पटिनिवत्त funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ध्रुव basep = nodep->baseMostClassp();
    AstFunc* funcp = VN_AS(memberMap.findMember(basep, "srandom"), Func);
    सचे (!funcp) {
        v3Global.useRandomizeMethods(सच्च);
        Ast_ग्रन्थिः_DType* ध्रुव dtypep
            = basep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ध्रुव ivarp = नूतन AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(सच्च);
        ivarp->direction(VDirection::निवेशः);
        funcp = नूतन AstFunc{basep->fileline(), "srandom", ivarp, nullptr};
        funcp->dtypep(basep->findVoidDType());
        funcp->classMethod(सच्च);
        funcp->isVirtual(मिच्छा);
        basep->addMembersp(funcp);
        memberMap.निवेशय(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        सचे (basep->name() == "process") {
            funcp->addStmtsp(नूतन AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } अञ्ञथा {
            funcp->addStmtsp(नूतन AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(सच्च);
        }
    }
    पटिनिवत्त funcp;
}
