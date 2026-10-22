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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "verilatedos.h"

#شامل "V3Randomize.h"

#شامل "V3Ast.h"
#شامل "V3Const.h"
#شامل "V3ConstPool.h"
#شامل "V3Container.h"
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Global.h"
#شامل "V3MemberMap.h"
#شامل "V3Task.h"
#شامل "V3UniqueNames.h"

#شامل <queue>
#شامل <tuple>
#شامل <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

شمارش ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

ایستا constexpr ثابت نویسه* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
ایستا constexpr ثابت نویسه* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
ایستا AstVar* getRandModeVarFromClass(Ast_گره_Module* classp) {
    وقتی (classp) {
        اگر (classp->user2p()) بازگشت VN_AS(classp->user2p(), Var);
        AstClass* ثابت cp = VN_CAST(classp, Class);
        اگر (!cp || !cp->extendsp()) بازگشت nullptr;
        classp = cp->extendsp()->classp();
    }
    بازگشت nullptr;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ایستا AstVar* getRandomGenerator(AstClass* ثابت classp) {
    اگر (classp->user3p()) بازگشت VN_AS(classp->user3p(), Var);
    اگر (classp->extendsp()) بازگشت getRandomGenerator(classp->extendsp()->classp());
    بازگشت nullptr;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ایستا AstTask* getCreateConstraintSetupFunc(AstClass* classp, VMemberMap& memberMap) {
    ایستا ثابت نویسه* ثابت name = "__Vsetup_constraints";
    AstTask* setupAllTaskp = VN_AS(memberMap.findMember(classp, name), Task);
    اگر (setupAllTaskp) بازگشت setupAllTaskp;
    setupAllTaskp = جدید AstTask{classp->fileline(), "__Vsetup_constraints", nullptr};
    setupAllTaskp->classMethod(درست);
    setupAllTaskp->isVirtual(درست);
    classp->addMembersp(setupAllTaskp);
    memberMap.درج(classp, setupAllTaskp);
    بازگشت setupAllTaskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

ساختار RandModeTarget final {
    // MEMBERS
    AstVar* ثابت receiverp;  // Variable that is the target of the rand_mode() call
    Ast_گره_عبارت* ثابت fromp;  // Expression from which the rand_mode() call originates
    AstClass* ثابت classp;  // Class in which rand_mode should be set

    // METHODS
    ایستا RandModeTarget get(Ast_گره_عبارت* fromp, Ast_گره_Module* modp) {
        اگر (!fromp) بازگشت {nullptr, nullptr, VN_AS(modp, Class)};
        اگر (AstCMethodHard* ثابت methodHardp = VN_CAST(fromp, CMethodHard)) {
            بازگشت RandModeTarget::get(methodHardp->fromp(), modp);
        }
        AstVar* receiverp = nullptr;
        AstClass* classp = VN_CAST(modp, Class);
        اگر (AstVarRef* ثابت varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            اگر (receiverp->isRand()) {
                fromp = nullptr;
                اگر (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } وگرنه اگر (AstMemberSel* ثابت memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            اگر (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        اگر (!receiverp->isRand()) {
            AstClassRefDType* ثابت classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            اگر (classRefDtp) classp = classRefDtp->classp();
        }
        بازگشت {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

اجتماع RandomizeMode final {
    // MEMBERS
    ساختار {
        منطقی usesMode : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t index : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ایستا Ast_گره_DType* arrayElementDTypep(Ast_گره_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    وقتی (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    بازگشت dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ایستا منطقی dtypeContainsReal(Ast_گره_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    اگر (Ast_گره_DType* ثابت subp = dtypep->subDTypep()) بازگشت dtypeContainsReal(subp);
    اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
        اگر (structp->packed()) بازگشت نادرست;
        برای (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            اگر (dtypeContainsReal(memberp->subDTypep())) بازگشت درست;
        }
        بازگشت نادرست;
    }
    ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
    بازگشت basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
ایستا تهی checkRandTypeEligibility(Ast_گره* contextp, Ast_گره_DType* dtypep, منطقی isRandc) {
    dtypep = dtypep->skipRefp();
    اگر (Ast_گره_DType* ثابت subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        بازگشت;
    }
    اگر (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        اگر (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        بازگشت;
    }
    اگر (ثابت AstIfaceRefDType* ثابت ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        بازگشت;
    }
    اگر (ثابت Ast_گره_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
        اگر (structp->packed()) بازگشت;  // Integral by construction; members checked separately
        اگر (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            بازگشت;
        }
        برای (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            اگر (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        بازگشت;
    }
    ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    اگر (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        اگر (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        بازگشت;
    }
    انتخاب (basicp->keyword()) {
    حالت VBasicDTypeKwd::STRING:
    حالت VBasicDTypeKwd::CHANDLE:
    حالت VBasicDTypeKwd::EVENT: {
        ثابت نویسه* ثابت articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        بشکن;
    }
    پیشفرض: بشکن;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

رده RandomizeMarkVisitor final : عمومی VNVisitor {
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
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    using DerivedSet = std::unordered_set<AstClass*>;
    using BaseToDerivedMap = std::unordered_map<ثابت AstClass*, DerivedSet>;

    BaseToDerivedMap m_baseToDerivedMap;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = nullptr;  // Current class
    Ast_گره* m_constraint_عبارت_Genp = nullptr;  // Current constraint or constraint if expression
    Ast_گره_Module* m_modp;  // Current module
    Ast_گره_Stmt* m_stmtp = nullptr;  // Current statement
    Ast_گره_FTask_Ref* m_stdRandCallp = nullptr;  // Current std::randomize() call
    منطقی m_inStdWith = نادرست;  // True when inside a 'with {}' clause
    std::set<Ast_گره_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = nullptr;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    تهی markGlobalConstrainedVars(AstClass* classp) {
        برای (ثابت AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            برای (Ast_گره* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                اگر (!varp) ادامه;
                اگر (varp->rand().isRandomizable()) varp->globalConstrained(درست);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    منطقی isVarInStdRandomizeArgs(ثابت AstVar* varp) ثابت {
        اگر (!m_inStdWith || !m_stdRandCallp) بازگشت نادرست;

        برای (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ثابت Ast_گره_عبارت* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            وقتی (exprp) {
                اگر (ثابت Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(exprp, گره_Var_Ref)) {
                    اگر (varrefp->varp() == varp) بازگشت درست;
                    بشکن;
                }
                اگر (ثابت AstMemberSel* ثابت memberselp = VN_CAST(exprp, MemberSel)) {
                    اگر (memberselp->varp() == varp) بازگشت درست;
                    exprp = memberselp->fromp();
                } وگرنه اگر (ثابت AstArraySel* ثابت arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } وگرنه اگر (ثابت AstStructSel* ثابت strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } وگرنه اگر (ثابت AstCMethodHard* ثابت methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } وگرنه {
                    بشکن;
                }
            }
        }
        بازگشت نادرست;
    }
    تهی markMembers(ثابت AstClass* nodep) {
        برای (ثابت AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            برای (Ast_گره* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                اگر (!varp) ادامه;
                // If member is randomizable and of class type, mark its class
                اگر (varp->rand().isRandomizable()) {
                    ثابت Ast_گره_DType* ثابت varDtypep = varp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    اگر (!classRefp) {
                        ثابت Ast_گره_DType* subDTypep = varDtypep->subDTypep();
                        اگر (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    اگر (classRefp) {
                        AstClass* ثابت rclassp = classRefp->classp();
                        اگر (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    اگر (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        RandomizeMode randMode = {};
                        randMode.usesMode = درست;
                        varp->user1(randMode.asUQuad);
                    }
                }
            }
        }
    }
    تهی markDerived(ثابت AstClass* nodep) {
        ثابت auto it = m_baseToDerivedMap.find(nodep);
        اگر (it != m_baseToDerivedMap.end()) {
            برای (auto* classp : it->second) {
                اگر (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    تهی markAllDerived() {
        برای (ثابت auto& p : m_baseToDerivedMap) {
            اگر (p.first->user1()) markDerived(p.first);
        }
    }
    تهی setPackageRefs() {
        برای (Ast_گره_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), گره_Module));
        }
    }
    تهی markNestedGlobalConstrainedRecurse(Ast_گره* nodep) {
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            AstVar* ثابت varp = refp->varp();
            اگر (varp->globalConstrained()) بازگشت;
            varp->globalConstrained(درست);
        } وگرنه اگر (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(nodep, MemberSel)) {
            اگر (memberSelp->varp()) {
                AstVar* ثابت varp = memberSelp->varp();
                اگر (varp->globalConstrained()) بازگشت;
                varp->globalConstrained(درست);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_گره_عبارت* buildMemberSelChain(AstVarRef* ریشه_Var_Refp, ثابت std::vector<AstVar*>& path) {
        Ast_گره_عبارت* exprp = ریشه_Var_Refp->clone_درخت(نادرست);
        برای (AstVar* memberVarp : path) {
            AstMemberSel* memberSelp
                = جدید AstMemberSel{ریشه_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        بازگشت exprp;
    }

    // Process a single constraint during nested constraint cloning
    تهی processNestedConstraint(AstConstraint* ثابت constrp, AstVarRef* ریشه_Var_Refp,
                                 ثابت std::vector<AstVar*>& newPath, AstClass* targetClassp) {
        std::string pathPrefix = ریشه_Var_Refp->name();
        برای (AstVar* pathMemberVarp : newPath) {
            pathPrefix += GLOBAL_CONSTRAINT_SEPARATOR + pathMemberVarp->name();
        }

        ثابت std::string newName = pathPrefix + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        منطقی isDuplicate = نادرست;
        targetClassp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت existingConstrp) {
            اگر (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = درست;
            }
        });
        اگر (isDuplicate) بازگشت;

        AstConstraint* ثابت cloneConstrp = constrp->clone_درخت(نادرست);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::vector<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_گره_عبارت* exprp) {
            اگر (AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
                اگر (!varRefp->varp()->isClassMember()) بازگشت;
                Ast_گره_عبارت* ثابت chainp = buildMemberSelChain(ریشه_Var_Refp, newPath);
                AstMemberSel* ثابت finalSelp
                    = جدید AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(افزودن_Deletep(varRefp), varRefp);
            } وگرنه اگر (AstFuncRef* ثابت funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                اگر (funcRefp->taskp()->classMethod() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.افزودن_back(funcRefp);
                }
            }
        });
        برای (AstFuncRef* funcRefp : funcRefps) {
            Ast_گره_عبارت* ثابت chainp = buildMemberSelChain(ریشه_Var_Refp, newPath);
            AstArg* ثابت argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* ثابت callp
                = جدید AstMethodCall{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(افزودن_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        targetClassp->addStmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    تهی cloneNestedConstraintsRecurse(AstVarRef* ریشه_Var_Refp, AstClass* classp,
                                       ثابت std::vector<AstVar*>& pathToClass,
                                       AstClass* targetClassp) {
        برای (Ast_گره* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ثابت memberVarp = VN_CAST(memberNodep, Var);
            اگر (!memberVarp) ادامه;
            اگر (!memberVarp->rand().isRandomizable()) ادامه;
            ثابت AstClassRefDType* ثابت memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            اگر (!memberClassRefp || !memberClassRefp->classp()) ادامه;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::vector<AstVar*> newPath = pathToClass;
            newPath.افزودن_back(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ثابت containingClassp, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, ریشه_Var_Refp, newPath, targetClassp);
                });
            cloneNestedConstraintsRecurse(ریشه_Var_Refp, nestedClassp, newPath, targetClassp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    تهی handleRandomizeArgument(Ast_گره_عبارت* exprp, AstVar* ثابت varp, ثابت منطقی stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        وقتی (exprp) {
            AstVar* randVarp = nullptr;
            اگر (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } وگرنه اگر (AstArraySel* ثابت arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                ادامه;  // Skip ArraySel, continue traversing
            } وگرنه اگر (AstStructSel* ثابت structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                ادامه;  // Skip StructSel, continue traversing
            } وگرنه اگر (AstCMethodHard* ثابت methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                ادامه;
            } وگرنه اگر (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(درست);
                varrefp->access(VAccess::READWRITE);
                exprp = nullptr;
            } وگرنه {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            اگر (randVarp == varp) بازگشت;
            Ast_گره* backp = randVarp;
            وقتی (backp && !VN_IS(backp, گره_Module)) backp = backp->backp();
            اگر (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } وگرنه {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                RandomizeMode randMode = {};
                randMode.usesMode = درست;
                randVarp->user1(randMode.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    تهی visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        اگر (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ثابت AstClass* ثابت basep = nodep->extendsp()->classp();
            m_baseToDerivedMap[basep].درج(nodep);
        }
    }
    تهی visit(Ast_گره_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        اگر (!nodep->backp()) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        اگر (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        اگر (nodep->name() == "rand_mode") {
            AstMethodCall* ثابت methodCallp = VN_CAST(nodep, MethodCall);
            Ast_گره_عبارت* ثابت fromp = methodCallp ? methodCallp->fromp() : nullptr;
            منطقی valid = درست;
            اگر (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = نادرست;
            } وگرنه اگر (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = نادرست;
            } وگرنه اگر (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = نادرست;
            } وگرنه اگر (AstCMethodHard* ثابت methodHardp = VN_CAST(fromp, CMethodHard)) {
                اگر (methodHardp->method() == VCMethod::ARRAY_AT
                    || methodHardp->method() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = نادرست;
                } وگرنه {
                    methodHardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            اگر (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_عبارت)
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = نادرست;
            }
            اگر (valid) {
                ثابت RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_classp);
                اگر ((!randModeTarget.receiverp || !randModeTarget.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = نادرست;
                } وگرنه اگر (!randModeTarget.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = نادرست;
                } وگرنه اگر (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = نادرست;
                } وگرنه اگر (randModeTarget.receiverp && randModeTarget.receiverp->isRand()) {
                    // Called on a rand member variable
                    RandomizeMode randMode = {};
                    randMode.usesMode = درست;
                    randModeTarget.receiverp->user1(randMode.asUQuad);
                } وگرنه {
                    // Called on 'this' or a non-rand class instance
                    randModeTarget.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        اگر (!varp->isRand()) بازگشت;
                        RandomizeMode randMode = {};
                        randMode.usesMode = درست;
                        varp->user1(randMode.asUQuad);
                    });
                }
            }
            اگر (!valid) {
                اگر (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                    nodep->replaceWith(جدید AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } وگرنه {
                    m_stmtp->unlinkFrBack();
                }
            }
            بازگشت;
        }

        اگر (nodep->name() == "constraint_mode") {
            منطقی valid = درست;
            اگر (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = نادرست;
            } وگرنه اگر (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_عبارت)
                       && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = نادرست;
            }
            AstConstraint* constrp = nullptr;
            AstClass* classp = m_classp;
            اگر (ثابت AstMethodCall* ثابت methodCallp = VN_CAST(nodep, MethodCall)) {
                اگر (ثابت AstConstraintRef* ثابت constrRefp
                    = VN_CAST(methodCallp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    اگر (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } وگرنه اگر (AstClassRefDType* classRefDtp
                           = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } وگرنه {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = نادرست;
                }
            }
            اگر (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = نادرست;
            }
            اگر (valid) {
                RandomizeMode constraintMode = {};
                constraintMode.usesMode = درست;
                اگر (constrp) {
                    constrp->user1(constraintMode.asUQuad);
                } وگرنه {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraintMode.asUQuad);
                    });
                }
            } وگرنه {
                اگر (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                    nodep->replaceWith(جدید AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } وگرنه {
                    m_stmtp->unlinkFrBack();
                }
            }
            بازگشت;
        }

        اگر (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            اگر (m_constraint_عبارت_Genp || m_inStdWith) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                برای (Ast_گره* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    اگر (argp->user1()) {
                        nodep->user1(درست);
                        بشکن;
                    }
                }
            }
            بازگشت;
        }
        AstClass* classp = m_classp;
        اگر (ثابت AstMethodCall* ثابت methodCallp = VN_CAST(nodep, MethodCall)) {
            اگر (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        اگر (classp) {
            اگر (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت memberVarp) {
                اگر (!memberVarp->rand().isRandomizable()) بازگشت;
                ثابت Ast_گره_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                اگر (!classRefp || !classRefp->classp()) بازگشت;
                AstClass* ثابت memberClassp = classRefp->classp();
                اگر (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) بازگشت;
                memberVarp->globalConstrained(درست);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* ریشه_Var_Refp
                    = جدید AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::vector<AstVar*> emptyPath;
                memberClassp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, ریشه_Var_Refp, emptyPath, classp);
                });
                cloneNestedConstraintsRecurse(ریشه_Var_Refp, memberClassp, emptyPath, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(ریشه_Var_Refp->delete_درخت(), ریشه_Var_Refp);
            });
        }
        اگر (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = nullptr;
            برای (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handleRandomizeArgument(argp->exprp(), nullptr, درست);
            }
            بازگشت;
        }
        برای (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            اگر (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->رقم().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ثابت منطقی hasUnsupportedMember
                    = classp->existsMember([](ثابت AstClass*, ثابت AstVar* memberVarp) {
                          اگر (!memberVarp->rand().isRandomizable()) بازگشت نادرست;
                          ثابت Ast_گره_DType* ثابت dtp = memberVarp->dtypep()->skipRefp();
                          بازگشت VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, QueueDType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                اگر (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                ادامه;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = nullptr;  // If nodep is a method call, this is its receiver
            اگر (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                اگر (AstMemberSel* ثابت memberSelp = VN_CAST(methodCallp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } وگرنه {
                    AstVarRef* ثابت varrefp = VN_AS(methodCallp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handleRandomizeArgument(argp->exprp(), fromVarp, نادرست);
        }
        // Re-mark members after the class became inline-randomized above.
        اگر (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    تهی visit(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_عبارت_Genp);
        m_stmtp = nodep;
        m_constraint_عبارت_Genp = nodep;
        iterateChildren(nodep);
        اگر (!nodep->backp()) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(Ast_Constraint_عبارت* nodep) override {
        VL_RESTORER(m_constraint_عبارت_Genp);
        m_constraint_عبارت_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_عبارت_Genp);
            m_constraint_عبارت_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterateAndNextConstNull(nodep->thensp());
        iterateAndNextConstNull(nodep->elsesp());
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (!m_constraint_عبارت_Genp) بازگشت;

        اگر (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        اگر (nodep->varp()->rand().isRandomizable() && !(m_inStdWith && m_stdRandCallp)) {
            nodep->user1(درست);
        } وگرنه اگر (isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(درست);
        }
    }
    تهی visit(AstMemberSel* nodep) override {
        اگر (!m_constraint_عبارت_Genp) بازگشت;
        iterateChildrenConst(nodep);
        ثابت منطقی randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ثابت منطقی randMember = nodep->varp()->rand().isRandomizable();
        ثابت منطقی inStdWith = m_inStdWith && m_stdRandCallp;
        اگر (randObject && randMember && !inStdWith) {
            nodep->user1(درست);
        } وگرنه اگر (inStdWith && isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(درست);
            // Mark parent object for constraint expression visitor
            اگر (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(درست);
        }

        اگر (m_withp) {
            Ast_گره* backp = m_withp;
            وقتی (backp->backp()) {
                اگر (ثابت AstMethodCall* ثابت callp = VN_CAST(backp, MethodCall)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    بشکن;
                }
                backp = backp->backp();
            }
        } وگرنه {
            nodep->user2p(m_modp);
        }
    }
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    تهی visit(AstVar* nodep) override {
        nodep->user2p(m_modp);
        اگر (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    تهی visit(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        اگر (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_inStdWith = درست;
        iterateChildrenConst(nodep);
        m_inStdWith = نادرست;
    }

    تهی visit(Ast_گره_عبارت* nodep) override {
        iterateChildrenConst(nodep);
        اگر (!m_constraint_عبارت_Genp && !m_inStdWith) بازگشت;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    تهی visit(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        اگر (!m_constraint_عبارت_Genp && !m_inStdWith) بازگشت;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit RandomizeMarkVisitor(Ast_گره* nodep) {
        iterateConst(nodep);
        markAllDerived();
        setPackageRefs();
    }
    ~RandomizeMarkVisitor() override = پیشفرض;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

رده Constraint_عبارت_Visitor final : عمومی VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ثابت m_classp;
    Ast_گره_FTask* ثابت m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_گره_FTask* ثابت m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ثابت m_genp;  // VlRandomizer variable of the class
    AstVar* m_randModeVarp;  // Relevant randmode state variable
    منطقی m_wantSingle = نادرست;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    منطقی m_structSel = نادرست;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    منطقی m_deferRandModeHoist = نادرست;  // Wait for the complete struct member path
    std::set<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::set<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertionSet<AstVar*>* m_sizeConstrainedArraysp = nullptr;  // Arrays with size constraints
    Ast_گره_عبارت* m_conditionp = nullptr;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_گره_FTask* m_prepareConstrainedArraysp = nullptr;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = nullptr;  // Current ID of unique call
    V3UniqueNames& m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    std::vector<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_گره* m_firstExpressionInsideIndexp = nullptr;
    AstConstraintForeach* m_foreachp = nullptr;  // Innermost constraint-foreach being processed
    std::set<ثابت AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    رده NestedAccessPath final {
        AstMemberSel* m_بالاترین_Nested_Array_Member_Selp
            = nullptr;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = nullptr;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = nullptr;  // Last node of variable's name format
        Ast_گره** m_firstExpressionInsideIndexPointerp
            = nullptr;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    عمومی:
        تهی addVarNamePart(AstSFormatF* ثابت nodep, std::string&& name) {
            اگر (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } وگرنه {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.درج(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            اگر (name == "") {
                اگر (*m_firstExpressionInsideIndexPointerp) {
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

        AstMemberSel* access_درخت() { بازگشت m_بالاترین_Nested_Array_Member_Selp; }
        ثابت std::string& smtName() { بازگشت m_smtName; }

        Ast_Stmt_عبارت* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ثابت methodp = جدید AstCMethodHard{
                varp->fileline(),
                جدید AstVarRef{varp->fileline(), VN_AS(genp->user2p(), گره_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtypeSetVoid();

            methodp->addPinsp(m_بالاترین_Nested_Array_Member_Selp);  // Variable

            ثابت Ast_گره_DType* ثابت dtypep = varp->dtypep();  // Width
            ثابت size_t width = m_بالاترین_Nested_Array_Member_Selp->varp()->dtypep()->width();
            methodp->addPinsp(جدید AstConst{dtypep->fileline(), AstConst::Unsized64{}, width});

            methodp->addPinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = nullptr;

            methodp->addPinsp(
                جدید AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_بالاترین_Nested_Array_Member_Selp = nullptr;
            بازگشت methodp->makeStmt();
        }

        Ast_گره_عبارت* cloneName() { بازگشت m_nestedNameFormatTopp->clone_درخت(نادرست); }

        تهی error() {
            VL_DO_DANGLING(m_بالاترین_Nested_Array_Member_Selp->delete_درخت(), m_بالاترین_Nested_Array_Member_Selp);
        }

        NestedAccessPath(AstMemberSel* بالاترین_Member_Selp, Ast_گره** pointerToFirstExprp)
            : m_بالاترین_Nested_Array_Member_Selp{بالاترین_Member_Selp}
            , m_firstExpressionInsideIndexPointerp{pointerToFirstExprp} {}
        ~NestedAccessPath() {
            اگر (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_درخت(),
                            m_nestedNameFormatTopp = nullptr);
                m_nestedNameFormatp = nullptr;
            }
        }
    };
    NestedAccessPath* m_nestedAccess = nullptr;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_گره_عبارت* createEnumConstraint(File_سطر* ثابت fl, ثابت std::string& smtName,
                                      AstEnumDType* ثابت enumDtp, AstVar* ثابت genVarp) {
        Ast_گره_Module* ثابت genModp = VN_AS(genVarp->user2p(), گره_Module);
        ثابت صحیح width = enumDtp->width();
        std::string constraint = "(__Vbv (or";
        برای (AstEnumItem* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvtToStr(vconstp->toUInt()) + " "
                          + cvtToStr(width) + "))";
        }
        constraint += "))";
        AstCMethodHard* ثابت callp = جدید AstCMethodHard{
            fl, جدید AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            جدید AstCExpr{fl, AstCExpr::Pure{}, "\"" + constraint + "\""}};
        callp->dtypeSetVoid();
        بازگشت callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* findStaticRandModeVarMember(AstClass* classp) ثابت {
        وقتی (درست) {
            اگر (AstVar* ثابت varp
                = VN_CAST(m_memberMap.findMember(classp, "__Vstaticrandmode"), Var)) {
                بازگشت varp;
            }
            AstClassExtends* ثابت extendsp = classp->extendsp();
            اگر (!extendsp) بازگشت nullptr;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    تهی markConstrainedRandMember(AstStructDType* ثابت structp, ثابت std::string& name) {
        AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.findMember(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(درست);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    منطقی isRandQualifiedPath(ثابت Ast_گره_عبارت* nodep) {
        وقتی (nodep) {
            اگر (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                اگر (AstStructDType* ثابت structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ثابت AstMemberDType* ثابت memberp
                        = VN_CAST(m_memberMap.findMember(structp, selp->name()), MemberDType);
                    اگر (memberp && !memberp->rand().isRandomizable()) بازگشت نادرست;
                }
                nodep = selp->fromp();
            } وگرنه اگر (ثابت AstArraySel* ثابت selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } وگرنه اگر (ثابت AstCMethodHard* ثابت methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } وگرنه {
                بشکن;
            }
        }
        بازگشت درست;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    ایستا تهی markStructConstrainedRandRecurse(Ast_گره_DType* ثابت dtypep) {
        AstStructDType* ثابت structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        اگر (!structp || structp->packed()) بازگشت;
        structp->markConstrainedRand(درست);
        برای (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            اگر (!memberp->rand().isRandomizable()) ادامه;
            memberp->markConstrainedRand(درست);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string buildMemberPath(ثابت AstMemberSel* ثابت memberSelp) {
        ثابت Ast_گره* fromp = memberSelp->fromp();
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            بازگشت refp->name() + "." + memberSelp->name();
        } وگرنه اگر (ثابت AstMemberSel* ثابت selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            بازگشت buildMemberPath(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        بازگشت "";
    }

    ایستا Ast_گره_عبارت* getFromp(ثابت Ast_گره_عبارت* ثابت nodep) {
        اگر (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            بازگشت cmethp->fromp();
        }
        بازگشت getSelFromp(nodep);
    }

    ایستا std::string extractSolveBeforeBaseName(ثابت Ast_گره_عبارت* exprp) {
        وقتی (ثابت Ast_گره_عبارت* ثابت fromp = getFromp(exprp)) exprp = fromp;
        اگر (ثابت AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) بازگشت vrefp->name();
        بازگشت "";
    }

    ایستا تهی buildNamePrefix(AstCExpr* exprp, ثابت Ast_گره_عبارت* ثابت nodep) {
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            exprp->add("(\""s + refp->name());
            بازگشت;
        }

        ثابت Ast_گره_عبارت* ثابت fromp = getFromp(nodep);
        اگر (fromp) buildNamePrefix(exprp, fromp);

        اگر (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            exprp->add("." + selp->name());
        } وگرنه اگر (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            بازگشت;
        } وگرنه اگر (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            اگر (cmethp->method() == VCMethod::ARRAY_AT) {
                Ast_گره_عبارت* ثابت argp = cmethp->pinsp();
                exprp->add(".\" + ");
                AstCFuncHard* ثابت solverHexFuncp
                    = جدید AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(argp->clone_درخت_Pure(نادرست));
                solverHexFuncp->dtypeSetString();
                exprp->add(solverHexFuncp);
                exprp->add(" + \"");
            } وگرنه {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                بازگشت;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ایستا AstCExpr* build_Path_عبارت(File_سطر* ثابت fl, ثابت Ast_گره_عبارت* nodep,
                                   ثابت std::string selName) {
        AstCExpr* ثابت exprp = جدید AstCExpr{fl, ""};
        buildNamePrefix(exprp, nodep);
        اگر (selName != "") {
            exprp->add(".");
            exprp->add(selName);
        }
        exprp->add("\")");
        بازگشت exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_عبارت(File_سطر* fl, ثابت std::string& baseName,
                                    ثابت Ast_گره_Sel* selp) {
        AstCExpr* ثابت p = جدید AstCExpr{fl, ""};
        p->add("(\""s + baseName + ".\"");
        p->add("+");
        AstCFuncHard* ثابت solverHexFuncp
            = جدید AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
        solverHexFuncp->addPinsp(selp->bitp()->clone_درخت_Pure(نادرست));
        solverHexFuncp->dtypeSetString();
        p->add(solverHexFuncp);
        p->add(")");
        p->dtypeSetString();
        بازگشت p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ایستا Ast_گره_عبارت* getSelFromp(ثابت Ast_گره_عبارت* exprp) {
        اگر (ثابت AstMemberSel* ثابت mp = VN_CAST(exprp, MemberSel)) بازگشت mp->fromp();
        اگر (ثابت AstStructSel* ثابت sp = VN_CAST(exprp, StructSel)) بازگشت sp->fromp();
        بازگشت nullptr;
    }

    Ast_گره_عبارت* build_Solve_Before_Name_عبارت(File_سطر* fl, Ast_گره_عبارت* exprp) {
        اگر (ثابت AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ثابت p = جدید AstCExpr{fl, AstCExpr::Pure{}, "\"" + varrefp->name() + "\"s"};
            p->dtypeSetString();
            بازگشت p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        اگر (Ast_گره_عبارت* ثابت selFromp = getSelFromp(exprp)) {
            ثابت std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            اگر (ثابت AstArraySel* ثابت arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string baseName;
                اگر (ثابت AstVarRef* ثابت vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    baseName = vp->name();
                } وگرنه اگر (ثابت AstMemberSel* ثابت mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    baseName = buildMemberPath(mp);
                }
                اگر (baseName.empty()) بازگشت nullptr;
                AstCExpr* ثابت p = جدید AstCExpr{fl, ""};
                p->add("(\""s + baseName + ".\"");
                p->add("+");
                AstCFuncHard* ثابت solverHexFuncp
                    = جدید AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(arrSelp->bitp()->clone_درخت_Pure(نادرست));
                solverHexFuncp->dtypeSetString();
                p->add(solverHexFuncp);
                p->add(")");
                p->dtypeSetString();
                بازگشت p;
            }
            // Static member path (obj.field)
            اگر (ثابت AstVarRef* ثابت vp = VN_CAST(selFromp, VarRef)) {
                ثابت std::string path = vp->name() + "." + selName;
                AstCExpr* ثابت p = جدید AstCExpr{fl, AstCExpr::Pure{}, "\"" + path + "\"s"};
                p->dtypeSetString();
                بازگشت p;
            }
            اگر (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ثابت p = build_Path_عبارت(fl, selFromp, selName);
                p->dtypeSetString();
                بازگشت p;
            }
            بازگشت nullptr;
        }
        اگر (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ثابت p = build_Path_عبارت(fl, exprp, "");
            p->dtypeSetString();
            بازگشت p;
        }
        ثابت Ast_گره_Sel* selp = VN_CAST(exprp, ArraySel);
        اگر (!selp) selp = VN_CAST(exprp, AssocSel);
        اگر (selp) {
            // arr[i] -> dynamic name
            std::string baseName;
            اگر (ثابت AstVarRef* ثابت vp = VN_CAST(selp->fromp(), VarRef)) {
                baseName = vp->name();
            } وگرنه اگر (ثابت AstMemberSel* ثابت mp = VN_CAST(selp->fromp(), MemberSel)) {
                baseName = buildMemberPath(mp);
            }
            اگر (baseName.empty()) بازگشت nullptr;
            بازگشت build_Array_Sel_Name_عبارت(fl, baseName, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        اگر (ثابت AstSel* ثابت bitSelp = VN_CAST(exprp, Sel)) {
            بازگشت build_Solve_Before_Name_عبارت(fl, bitSelp->fromp());
        }
        بازگشت nullptr;
    }

    AstSFormatF* getConstFormat(Ast_گره_عبارت* nodep) {
        بازگشت جدید AstSFormatF{nodep->fileline(), (nodep->width() & 3) ? "#b%b" : "#x%x", نادرست,
                               nodep};
    }
    منطقی editFormat(Ast_گره_عبارت* nodep) {
        اگر (nodep->user1()) بازگشت نادرست;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        handle.relink(getConstFormat(nodep));
        بازگشت درست;
    }
    Ast_گره_عبارت* wrapWithCond(Ast_گره_عبارت* ثابت exprp, Ast_گره_عبارت* ثابت condp,
                              ثابت صحیح width) {
        اگر (condp) {
            File_سطر* ثابت flp = exprp->fileline();
            بازگشت جدید AstCond{
                flp, condp, exprp,
                getConstFormat(جدید AstConst{flp, AstConst::Widthed_مقدار{}, width, 0})};
        }
        بازگشت exprp;
    }
    AstSFormatF* editSMT(Ast_گره_عبارت* nodep, Ast_گره_عبارت* lhsp = nullptr,
                         Ast_گره_عبارت* rhsp = nullptr, Ast_گره_عبارت* thsp = nullptr) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_عبارت = nodep->emitSMT();  // Might need child width (AstExtend)
        اگر (smt_عبارت == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            بازگشت nullptr;
        }

        ثابت صحیح lhsWidth = lhsp ? lhsp->width() : 0;
        ثابت صحیح rhsWidth = rhsp ? rhsp->width() : 0;
        ثابت صحیح thsWidth = thsp ? thsp->width() : 0;
        Ast_گره_عبارت* lhsCondp = nullptr;
        Ast_گره_عبارت* rhsCondp = nullptr;
        Ast_گره_عبارت* thsCondp = nullptr;

        اگر (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlinkFrBack() : lhsp),
                         گره_عبارت);
            lhsCondp = m_conditionp;
        }
        اگر (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlinkFrBack() : rhsp),
                         گره_عبارت);
            rhsCondp = m_conditionp;
        }
        اگر (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlinkFrBack() : thsp),
                         گره_عبارت);
            thsCondp = m_conditionp;
        }

        Ast_گره_عبارت* argsp = nullptr;
        برای (string::iterator pos = smt_عبارت.begin(); pos != smt_عبارت.end(); ++pos) {
            اگر (pos[0] == '%') {
                ++pos;
                انتخاب (pos[0]) {
                حالت '%': بشکن;
                حالت 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گره::addNext(argsp, wrapWithCond(lhsp, lhsCondp, lhsWidth));
                    lhsp = nullptr;
                    بشکن;
                حالت 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گره::addNext(argsp, wrapWithCond(rhsp, rhsCondp, rhsWidth));
                    rhsp = nullptr;
                    بشکن;
                حالت 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گره::addNext(argsp, wrapWithCond(thsp, thsCondp, thsWidth));
                    thsp = nullptr;
                    بشکن;
                پیشفرض: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); بشکن;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ثابت newp = جدید AstSFormatF{nodep->fileline(), smt_عبارت, نادرست, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        بازگشت newp;
    }

    Ast_گره_عبارت* editSingle(File_سطر* fl, Ast_گره* itemsp) {
        اگر (!itemsp) بازگشت nullptr;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = درست;

        {
            AstBegin* ثابت tempp
                = جدید AstBegin{fl, "[EditWrapper]", itemsp->unlinkFrBackWithNext(), نادرست};
            VL_DO_DANGLING(iterateAndNextNull(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            اگر (itemsp) itemsp->unlinkFrBackWithNext();
            VL_DO_DANGLING(tempp->delete_درخت(), tempp);
        }
        اگر (!itemsp) بازگشت nullptr;

        Ast_گره_عبارت* ثابت exprsp = VN_CAST(itemsp, گره_عبارت);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        اگر (!exprsp->nextp()) بازگشت exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        برای (Ast_گره* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        بازگشت جدید AstSFormatF{fl, fmt.str(), نادرست, exprsp};
    }

    Ast_گره_عبارت* newSel(File_سطر* fl, Ast_گره_عبارت* arrayp, Ast_گره_عبارت* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_گره_DType* ثابت arrDtp = arrayp->unlinkFrBack()->dtypep()->skipRefp();
        Ast_گره_عبارت* selp = nullptr;
        اگر (VN_IS(arrDtp, QueueDType) || VN_IS(arrDtp, DynArrayDType))
            selp = جدید AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        وگرنه اگر (VN_IS(arrDtp, UnpackArrayDType))
            selp = جدید AstArraySel{fl, arrayp, idxp};
        وگرنه اگر (VN_IS(arrDtp, AssocArrayDType))
            selp = جدید AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        بازگشت selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_گره_عبارت* adjustWidth(File_سطر* fl, Ast_گره_عبارت* exprp, صحیح targetWidth,
                             VSigning targetSigning) {
        ثابت صحیح عبارت_Width = exprp->width();
        اگر (targetWidth > عبارت_Width) {
            // Extend to match target width
            Ast_گره_عبارت* ثابت result = جدید AstExtend{fl, exprp, targetWidth};
            result->dtypeChgSigned(targetSigning);
            بازگشت result;
        } وگرنه اگر (targetWidth < عبارت_Width) {
            // Truncate to match target width
            Ast_گره_عبارت* ثابت result = جدید AstSel{fl, exprp, 0, targetWidth};
            result->dtypeChgSigned(targetSigning);
            بازگشت result;
        } وگرنه {
            // Width already matches
            بازگشت exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_گره_عبارت* extract_Return_عبارت(AstFunc* funcp) {
        AstVar* ثابت retVarp = VN_CAST(funcp->fvarp(), Var);
        اگر (!retVarp) بازگشت nullptr;
        Ast_گره_عبارت* retExprp = nullptr;
        صحیح assign_شمار = 0;
        funcp->foreach([&](AstAssign* assignp) {
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            اگر (!lhsp || lhsp->varp() != retVarp) بازگشت;
            retExprp = assignp->rhsp();
            ++assign_شمار;
        });
        // Simple function: initializer + return (2) or just return (1)
        اگر (assign_شمار > 2 || !retExprp) بازگشت nullptr;
        بازگشت retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    تهی propagateUser1InlineRecurse(Ast_گره_عبارت* nodep) {
        اگر (VN_IS(nodep, گره_Var_Ref)) {
            nodep->user1(VN_AS(nodep, گره_Var_Ref)->varp()->rand().isRandomizable());
            بازگشت;
        }
        اگر (VN_IS(nodep, Const)) {
            nodep->user1(نادرست);
            بازگشت;
        }
        صحیح anyChild = نادرست;  // Used as bool
        اگر (Ast_گره_عبارت* ثابت cp = VN_CAST(nodep->op1p(), گره_عبارت)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        اگر (Ast_گره_عبارت* ثابت cp = VN_CAST(nodep->op2p(), گره_عبارت)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        اگر (Ast_گره_عبارت* ثابت cp = VN_CAST(nodep->op3p(), گره_عبارت)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        اگر (Ast_گره_عبارت* ثابت cp = VN_CAST(nodep->op4p(), گره_عبارت)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        nodep->user1(anyChild);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ایستا Ast_گره* prependDistPreamble(AstConstraintForeach* nodep, Ast_گره* bodyp) {
        Ast_گره* ثابت preamblep = nodep->user3p();
        اگر (!preamblep) بازگشت bodyp;
        nodep->user3p(nullptr);
        preamblep->addNext(bodyp);
        بازگشت preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    منطقی referencesForeachIdx(ثابت Ast_گره* nodep) ثابت {
        اگر (m_foreachIdxVars.empty()) بازگشت نادرست;
        بازگشت nodep->exists(
            [&](ثابت AstVarRef* refp) { بازگشت m_foreachIdxVars.شمار(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    تهی addRegistrationStmt(Ast_گره* ثابت stmtp, Ast_گره_FTask* ثابت initTaskp) {
        اگر (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->addBodyp(stmtp);
        } وگرنه {
            initTaskp->addStmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_سطر* ثابت fl, Ast_گره_عبارت* ثابت arrExprp,
                                       Ast_گره_عبارت* ثابت idxExprp) {
        Ast_گره_عبارت* ثابت argsp = Ast_گره::addNext(arrExprp, idxExprp);
        AstSFormatF* ثابت formatp = جدید AstSFormatF{fl, "(select %s %s)", نادرست, argsp};
        بازگشت formatp;
    }

    تهی setRandMode(AstVar* ثابت varp, AstMemberSel* ثابت memberselp,
                     ثابت std::string& smtName, ثابت RandomizeMode& randMode,
                     Ast_گره_FTask* ثابت initTaskp) {
        Ast_گره_Module* ثابت varClassp = VN_AS(varp->user2p(), گره_Module);
        AstVar* ثابت subRandModeVarp = getRandModeVarFromClass(varClassp);
        اگر (subRandModeVarp) {
            Ast_گره_عبارت* ثابت parentAccessp = memberselp->fromp()->clone_درخت(نادرست);
            AstMemberSel* ثابت randModeSelp
                = جدید AstMemberSel{varp->fileline(), parentAccessp, subRandModeVarp};
            randModeSelp->dtypep(subRandModeVarp->dtypep());
            AstCMethodHard* ثابت atp
                = جدید AstCMethodHard{varp->fileline(), randModeSelp, VCMethod::ARRAY_AT,
                                     جدید AstConst{varp->fileline(), randMode.index}};
            atp->dtypeSetUInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ثابت enablep = جدید AstCMethodHard{
                varp->fileline(),
                جدید AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtypeSetVoid();
            Ast_گره_عبارت* ثابت nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : جدید AstSFormatF{varp->fileline(), smtName, نادرست, nullptr};
            nameExprp->dtypep(varp->dtypep());
            enablep->addPinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ثابت disablep = جدید AstCMethodHard{
                varp->fileline(),
                جدید AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_SET_VAR_DISABLED};
            disablep->dtypeSetVoid();
            Ast_گره_عبارت* ثابت disnp = nameExprp->clone_درخت(نادرست);
            disablep->addPinsp(disnp);
            AstIf* ثابت ifp
                = جدید AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            addRegistrationStmt(ifp, initTaskp);
        }
    }

    تهی markRandc(AstVar* ثابت varp, ثابت std::string& smtName, Ast_گره_FTask* ثابت initTaskp) {
        AstCMethodHard* ثابت markp = جدید AstCMethodHard{
            varp->fileline(),
            جدید AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtypeSetVoid();
        Ast_گره_عبارت* ثابت nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : جدید AstSFormatF{varp->fileline(), smtName, نادرست, nullptr};
        nameExprp->dtypep(varp->dtypep());
        markp->addPinsp(nameExprp);
        addRegistrationStmt(markp->makeStmt(), initTaskp);
    }

    Ast_گره_Module* getLeftmostVarModulep(AstMemberSel* ثابت memberselp,
                                         AstVar* ثابت varp) ثابت {
        اگر (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_گره* چپ_Most_Nodep = memberselp->fromp();
            وقتی (AstMemberSel* ثابت nestedMemberSelp = VN_CAST(چپ_Most_Nodep, MemberSel)) {
                چپ_Most_Nodep = nestedMemberSelp->fromp();
            }
            اگر (Ast_گره_Var_Ref* ثابت ریشه_Var_Refp = VN_CAST(چپ_Most_Nodep, گره_Var_Ref)) {
                بازگشت VN_AS(ریشه_Var_Refp->varp()->user2p(), گره_Module);
            }
            بازگشت VN_AS(memberselp->user2p(), گره_Module);
        }
        بازگشت VN_AS(varp->user2p(), گره_Module);
    }

    Ast_گره_FTask* getInitTaskp(AstVar* ثابت varp, ثابت منطقی memberselOrStructSel,
                               Ast_گره_Module* ثابت classp) ثابت {
        اگر (m_inlineInitTaskp) بازگشت m_inlineInitTaskp;
        varp->user3(درست);
        اگر (memberselOrStructSel) {
            اگر (Ast_گره_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.findMember(classp, "randomize"), گره_FTask)) {
                بازگشت initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            اگر (m_memberselInitTaskp) بازگشت m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } وگرنه {
            Ast_گره_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.findMember(classp, "new"), گره_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            بازگشت initTaskp;
        }
        بازگشت nullptr;
    }

    تهی createWriteVarForClassFields(AstClass* ثابت elemClassp, ثابت std::string& smtName,
                                      AstVar* ثابت iterVarp, ثابت منطقی isUnpackedClassRefArray,
                                      Ast_گره_Module* ثابت classOrPackagep, AstVar* ثابت varp,
                                      AstIf* ثابت ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ثابت {
        File_سطر* ثابت fl = varp->fileline();
        Ast_گره_Module* ثابت varClassp = VN_AS(varp->user2p(), گره_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr صحیح idxWidth = 32;
        ثابت صحیح fmtWidth = VL_واژگان_I(idxWidth) * 8;

        برای (ثابت AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            برای (Ast_گره* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ثابت memberVarp = VN_CAST(mnodep, Var);
                اگر (!memberVarp || !memberVarp->rand().isRandomizable()) ادامه;
                Ast_گره_DType* ثابت memberDtp = memberVarp->dtypep()->skipRefp();
                اگر (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) ادامه;
                ثابت صحیح memberWidth = memberDtp->width();

                AstCStmt* ثابت fmtp = جدید AstCStmt{fl};
                اگر (isUnpackedClassRefArray) {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } وگرنه {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::to_string(fmtWidth) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->add(جدید AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->add(");\n");
                ifNonNullp->addThensp(fmtp);

                AstVarRef* ثابت arrayWrRefp = جدید AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_گره_عبارت* atWritep;
                اگر (isUnpackedClassRefArray) {
                    atWritep = جدید AstArraySel{fl, arrayWrRefp,
                                               جدید AstVarRef{fl, iterVarp, VAccess::READ}};
                } وگرنه {
                    atWritep = جدید AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  جدید AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* ثابت memberSelp = جدید AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* ثابت writeVarp
                    = جدید AstCMethodHard{fl,
                                         جدید AstVarRef{fl, VN_AS(m_genp->user2p(), گره_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtypeSetVoid();
                writeVarp->addPinsp(memberSelp);
                writeVarp->addPinsp(
                    جدید AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(memberWidth)});
                AstCExpr* ثابت nameRefp = جدید AstCExpr{fl, AstCExpr::Pure{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->addPinsp(nameRefp);
                writeVarp->addPinsp(جدید AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->addThensp(writeVarp->makeStmt());
            }
        }
    }

    تهی createSolverArrayHandle(AstVar* ثابت varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_گره_Module* ثابت classOrPackagep,
                                 ثابت منطقی isUnpackedClassRefArray,
                                 ثابت std::string& smtName) ثابت {
        File_سطر* ثابت fl = varp->fileline();
        AstClass* ثابت elemClassp = elemClassRefDtp->classp();
        Ast_گره_Module* ثابت varClassp = VN_AS(varp->user2p(), گره_Module);

        AstVar* ثابت iterVarp = جدید AstVar{fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__Vi"),
                                            varp->findUInt32DType()};
        iterVarp->funcLocal(درست);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_گره* ثابت stmtsp
            = جدید AstAssign{fl, جدید AstVarRef{fl, iterVarp, VAccess::WRITE}, جدید AstConst{fl, 0}};

        Ast_گره_عبارت* sizep;
        اگر (isUnpackedClassRefArray) {
            ثابت صحیح arraySize
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = جدید AstConst{fl, static_cast<uint32_t>(arraySize)};
        } وگرنه {
            AstVarRef* ثابت arraySizeRefp = جدید AstVarRef{fl, varClassp, varp, VAccess::READ};
            arraySizeRefp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ثابت dynSizep
                = جدید AstCMethodHard{fl, arraySizeRefp, VCMethod::DYN_SIZE, nullptr};
            dynSizep->dtypeSetUInt32();
            sizep = dynSizep;
        }

        AstLoop* ثابت loopp = جدید AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(جدید AstLoopTest{
            fl, loopp, جدید AstLt{fl, جدید AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ثابت arrayReadRefp = جدید AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_گره_عبارت* atReadp;
        اگر (isUnpackedClassRefArray) {
            atReadp
                = جدید AstArraySel{fl, arrayReadRefp, جدید AstVarRef{fl, iterVarp, VAccess::READ}};
        } وگرنه {
            atReadp = جدید AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         جدید AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ثابت ifNonNullp
            = جدید AstIf{fl, جدید AstNeq{fl, atReadp, جدید AstConst{fl, AstConst::Null{}}}, nullptr};
        loopp->addStmtsp(ifNonNullp);

        AstCStmt* ثابت bufDeclp = جدید AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->addThensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->addStmtsp(جدید AstAssign{
            fl, جدید AstVarRef{fl, iterVarp, VAccess::WRITE},
            جدید AstAdd{fl, جدید AstConst{fl, 1}, جدید AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(درست);
        Ast_گره_FTask* initTaskp = m_inlineInitTaskp;
        اگر (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.findMember(varClassp, "new"), گره_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->addStmtsp(stmtsp);
    }

    تهی createSolverVarHandle(AstVar* ثابت varp, ثابت منطقی structSelOrCMeth,
                               ثابت منطقی isGlobalConstrained, ثابت RandomizeMode randMode,
                               AstMemberSel* ثابت memberselp, ثابت std::string& smtName,
                               Ast_گره_Module* ثابت classOrPackagep, Ast_گره_Module* ثابت classp,
                               Ast_گره_FTask* ثابت initTaskp) {
        uint32_t unpackedDims = 0;
        اگر (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(نادرست).second;
        }
        اگر (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ثابت methodp = جدید AstCMethodHard{
            varp->fileline(),
            جدید AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();
        اگر (memberselp) {
            methodp->addPinsp(memberselp);
        } وگرنه {
            AstVarRef* ثابت varRefp
                = جدید AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->addPinsp(varRefp);
        }
        ثابت size_t width = arrayElementDTypep(varp->dtypep())->width();
        methodp->addPinsp(جدید AstConst{varp->fileline(), AstConst::Unsized64{}, width});
        Ast_گره_عبارت* ثابت varnamep = جدید AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                   "\"" + smtName + "\"", varp->width()};
        varnamep->dtypep(varp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(
            جدید AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        اگر (randMode.usesMode && !(isGlobalConstrained && memberselp)) {
            methodp->addPinsp(
                جدید AstConst{varp->fileline(), AstConst::Unsized64{}, randMode.index});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->addStmtsp(methodp->makeStmt());
        اگر (varp->lifetime().isStatic() && randMode.usesMode) {
            AstCMethodHard* ثابت markp = جدید AstCMethodHard{
                varp->fileline(),
                جدید AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_گره_عبارت* ثابت namep = جدید AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                    "\"" + smtName + "\"", varp->width()};
            namep->dtypep(varp->dtypep());
            markp->addPinsp(namep);
            markp->dtypeSetVoid();
            initTaskp->addStmtsp(markp->makeStmt());
        }
        اگر (isGlobalConstrained && memberselp && randMode.usesMode) {
            setRandMode(varp, memberselp, smtName, randMode, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        اگر (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_گره_عبارت* createVarExprp(Ast_گره_Var_Ref* nodep, ثابت RandomizeMode randMode,
                                ثابت std::string& smtName, ثابت منطقی isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ثابت varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_گره_عبارت* exprp = جدید AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, نادرست, nullptr};
        اگر (!randMode.usesMode) {
            // Global constraints keep nodep alive for write_var processing
            اگر (!isGlobalConstrained) VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_گره_عبارت* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_درخت(نادرست)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_گره_عبارت* randModeAccess;
        ثابت منطقی varIsStatic = varp->lifetime().isStatic();
        AstClass* ثابت varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : nullptr;
        AstVar* ثابت ownerStaticRandModeVarp
            = varOwningClassp ? findStaticRandModeVarMember(varOwningClassp) : nullptr;
        اگر (ownerStaticRandModeVarp) {
            randModeAccess = جدید AstVarRef{varp->fileline(),
                                           VN_AS(ownerStaticRandModeVarp->user2p(), گره_Module),
                                           ownerStaticRandModeVarp, VAccess::READ};
        } وگرنه اگر (memberselp) {
            Ast_گره_Module* ثابت varClassp = VN_AS(varp->user2p(), گره_Module);
            AstVar* ثابت effectiveRandModeVarp = getRandModeVarFromClass(varClassp);
            اگر (effectiveRandModeVarp) {
                // Member's class has randmode, use it
                Ast_گره_عبارت* parentAccess = memberselp->fromp()->clone_درخت(نادرست);
                AstMemberSel* randModeSel
                    = جدید AstMemberSel{varp->fileline(), parentAccess, effectiveRandModeVarp};
                randModeSel->dtypep(effectiveRandModeVarp->dtypep());
                randModeAccess = randModeSel;
            } وگرنه {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
                randModeAccess
                    = جدید AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), گره_Module),
                                    m_randModeVarp, VAccess::READ};
            }
        } وگرنه {
            UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
            randModeAccess
                = جدید AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), گره_Module),
                                m_randModeVarp, VAccess::READ};
        }

        AstCMethodHard* ثابت atp
            = جدید AstCMethodHard{nodep->fileline(), randModeAccess, VCMethod::ARRAY_AT,
                                 جدید AstConst{nodep->fileline(), randMode.index}};
        atp->dtypeSetUInt32();
        exprp = جدید AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(درست);  // Mark as formatted
        بازگشت exprp;
    }

    // VISITORS
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        اگر (varp->user4p()) {
            منطقی isSizeRef = نادرست;
            اگر (AstCMethodHard* ثابت methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                اگر (methodp->method() == VCMethod::ASSOC_SIZE
                    || methodp->method() == VCMethod::DYN_SIZE) {
                    isSizeRef = درست;
                }
            }
            اگر (!isSizeRef && m_sizeConstrainedArraysp) { m_sizeConstrainedArraysp->درج(varp); }
        }

        // Check if this variable is marked as globally constrained
        ثابت منطقی isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = nullptr;
        منطقی structSelOrCMeth = نادرست;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_گره_DType* selectedDtypep = nullptr;
        std::string smtName;
        اگر (m_nestedAccess) {
            m_nestedAccess->addVarNamePart(
                جدید AstSFormatF{varp->fileline(), nodep->name(), نادرست, nullptr}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_درخت();
        } وگرنه اگر (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_گره* بالاترین_Member_Sel = nodep->backp();
            وقتی (VN_IS(بالاترین_Member_Sel->backp(), MemberSel)) {
                بالاترین_Member_Sel = بالاترین_Member_Sel->backp();
            }
            memberselp = VN_AS(بالاترین_Member_Sel, MemberSel)->clone_درخت(نادرست);
            smtName = buildMemberPath(memberselp);
        } وگرنه اگر (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_گره* بالاترین_Nodep = nodep->backp();
            وقتی (VN_IS(بالاترین_Nodep->backp(), StructSel) || VN_IS(بالاترین_Nodep->backp(), CMethodHard)) {
                بالاترین_Nodep = بالاترین_Nodep->backp();
            }

            اگر (VN_IS(بالاترین_Nodep, StructSel) || VN_IS(بالاترین_Nodep, CMethodHard)) {
                structSelOrCMeth = درست;
                selectedDtypep = VN_AS(بالاترین_Nodep, گره_عبارت)->dtypep();
            }

            memberselp = VN_CAST(بالاترین_Nodep, MemberSel);
            اگر (memberselp) smtName = buildMemberPath(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            اگر (smtName == "") smtName = nodep->name();
        } وگرنه {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        اگر (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        اگر (dtypeContainsReal(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            بازگشت;
        }
        Ast_گره_Module* ثابت classOrPackagep = nodep->classOrPackagep();
        ثابت RandomizeMode randMode = {.asUQuad = varp->user1()};
        اگر (!randMode.usesMode && editFormat(nodep)) بازگشت;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skipRefToEnump(), EnumDType);
        اگر (enumDtp
            && (std::find(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.end(), varp)
                == m_rangeConstrainedEnums.end())) {
            AstVar* genVarp = m_genp;
            اگر (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_گره_عبارت* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_گره_FTask* targetTaskp = m_inlineInitTaskp;
            اگر (!targetTaskp) targetTaskp = getCreateConstraintSetupFunc(m_classp, m_memberMap);
            UASSERT_OBJ(targetTaskp, nodep, "No function to inline enum range constraint into");
            targetTaskp->addStmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.افزودن_back(varp);
        }

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        Ast_گره_عبارت* ثابت exprp
            = createVarExprp(nodep, randMode, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ثابت منطقی alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.شمار(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.شمار(smtName) > 0
                                    : memberselp          ? m_writtenVars.شمار(smtName) > 0
                                                          : varp->user3();
        ثابت منطقی shouldWriteVar = !alreadyWritten;
        اگر (shouldWriteVar) {
            // Track this variable path as written
            اگر (m_inlineInitTaskp)
                m_inlineWrittenVars.درج(smtName);
            وگرنه اگر (isGlobalConstrained || memberselp)
                m_writtenVars.درج(smtName);
            // For global constraints, delete nodep after processing
            اگر (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);

            منطقی isClassRefArray = نادرست;
            منطقی isUnpackedClassRefArray = نادرست;
            AstClassRefDType* elemClassRefDtp = nullptr;
            {
                Ast_گره_DType* varDtp = varp->dtypep()->skipRefp();
                اگر (varDtp->isNonPackedArray()) {
                    Ast_گره_DType* ثابت elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    اگر (elemClassRefDtp) {
                        isClassRefArray = درست;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_گره_Module* ثابت classp = getLeftmostVarModulep(memberselp, varp);
            Ast_گره_FTask* ثابت initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            اگر (m_nestedAccess) {
                Ast_Stmt_عبارت* ثابت writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                addRegistrationStmt(writeVarStmtp, initTaskp);
                اگر (isGlobalConstrained && memberselp && randMode.usesMode) {
                    setRandMode(varp, memberselp, smtName, randMode, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                اگر (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } وگرنه اگر (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } وگرنه {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, randMode,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } وگرنه {
            // Variable already written, clean up cloned memberselp if any
            اگر (memberselp) VL_DO_DANGLING(memberselp->delete_درخت(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            اگر (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        اگر (m_nestedAccess) { VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_گره_عبارت* build_شمار_Ones_Expansion(File_سطر* fl, Ast_گره_عبارت* argp) {
        ثابت V3_عدد رقم_One{fl, argp->width(), 1};
        Ast_گره_عبارت* sump = جدید AstAnd{fl, argp, جدید AstConst{fl, رقم_One}};
        sump->user1(درست);
        برای (صحیح i = 1; i < argp->width(); i++) {
            V3_عدد رقم_Bit_Mask{fl, argp->width(), 0};
            رقم_Bit_Mask.setBit(i, 1);
            AstAnd* ثابت andp
                = جدید AstAnd{fl, argp->clone_درخت_Pure(نادرست), جدید AstConst{fl, رقم_Bit_Mask}};
            andp->user1(درست);
            AstShiftR* ثابت shiftp
                = جدید AstShiftR{fl, andp,
                                جدید AstConst{fl, AstConst::Widthed_مقدار{}, argp->width(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(درست);
            sump = جدید AstAdd{fl, sump, shiftp};
            sump->user1(درست);
        }
        بازگشت sump;
    }

    تهی visit(Ast_شمار_Ones* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* sump = build_شمار_Ones_Expansion(fl, argp);
        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(درست);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(sump);
    }
    تهی visit(AstRedXor* nodep) override {
        اگر (editFormat(nodep)) بازگشت;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlinkFrBack();

        Ast_گره_عبارت* redxorp = جدید AstSel{fl, argp, 0, 1};
        redxorp->user1(درست);
        برای (صحیح i = 1; i < argp->width(); i++) {
            AstSel* ثابت selp = جدید AstSel{fl, argp->clone_درخت_Pure(نادرست), i, 1};
            selp->user1(درست);

            redxorp = جدید AstXor{fl, redxorp, selp};
            redxorp->user1(درست);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(redxorp);
    }
    تهی visit(AstRedAnd* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        // Convert to (~x == 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlinkFrBack();
        ثابت V3_عدد رقم_Zero{fl, argp->width(), 0};
        Ast_گره_عبارت* ثابت negp = جدید AstNot{fl, argp};
        negp->user1(درست);
        Ast_گره_عبارت* ثابت eqp = جدید AstEq{fl, negp, جدید AstConst{fl, رقم_Zero}};
        eqp->user1(درست);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(eqp);
    }
    تهی visit(AstRedOr* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        // Convert to (x != 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlinkFrBack();
        ثابت V3_عدد رقم_Zero{fl, argp->width(), 0};
        Ast_گره_عبارت* ثابت neqp = جدید AstNeq{fl, argp, جدید AstConst{fl, رقم_Zero}};
        neqp->user1(درست);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(neqp);
    }
    تهی visit(AstOneHot* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlinkFrBack();
        ثابت صحیح w = argp->width();

        ثابت V3_عدد رقم_Zero{fl, w, 0};
        AstNeq* ثابت neZerop
            = جدید AstNeq{fl, argp->clone_درخت_Pure(نادرست), جدید AstConst{fl, رقم_Zero}};
        neZerop->user1(درست);

        ثابت V3_عدد رقم_One{fl, w, 1};
        AstSub* ثابت subp = جدید AstSub{fl, argp->clone_درخت_Pure(نادرست), جدید AstConst{fl, رقم_One}};
        subp->user1(درست);

        AstAnd* ثابت andp = جدید AstAnd{fl, argp, subp};
        andp->user1(درست);

        ثابت V3_عدد رقم_Zero2{fl, w, 0};
        AstEq* ثابت eqZerop = جدید AstEq{fl, andp, جدید AstConst{fl, رقم_Zero2}};
        eqZerop->user1(درست);

        AstLogAnd* ثابت resultp = جدید AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(درست);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(resultp);
    }
    تهی visit(AstOneHot0* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        // $onehot0(x) = (x & (x-1)) == 0
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlinkFrBack();
        ثابت صحیح w = argp->width();

        ثابت V3_عدد رقم_One{fl, w, 1};
        AstSub* ثابت subp = جدید AstSub{fl, argp->clone_درخت_Pure(نادرست), جدید AstConst{fl, رقم_One}};
        subp->dtypeFrom(argp);
        subp->user1(درست);

        AstAnd* ثابت andp = جدید AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(درست);

        ثابت V3_عدد رقم_Zero{fl, w, 0};
        AstEq* ثابت eqp = جدید AstEq{fl, andp, جدید AstConst{fl, رقم_Zero}};
        eqp->user1(درست);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(eqp);
    }
    تهی visit(Ast_شمار_Bits* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        File_سطر* ثابت fl = nodep->fileline();

        منطقی شمار_Ones = نادرست;
        منطقی شمار_Zeros = نادرست;
        برای (Ast_گره_عبارت* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ثابت AstConst* ثابت cp = VN_CAST(ctrlp, Const);
            اگر (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ثابت zerop
                    = جدید AstConst{fl, AstConst::Widthed_مقدار{}, nodep->width(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                بازگشت;
            }
            اگر (cp->رقم().bitIs1(0) && !شمار_Ones)
                شمار_Ones = درست;
            وگرنه اگر (cp->رقم().bitIs0(0) && !شمار_Zeros)
                شمار_Zeros = درست;
        }

        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlinkFrBack();
        ثابت صحیح argWidth = argp->width();
        Ast_گره_عبارت* sump = nullptr;
        اگر (شمار_Ones && شمار_Zeros) {
            // ones + zeros = width for 2-state types
            sump = جدید AstConst{fl, AstConst::Widthed_مقدار{}, nodep->width(), (uint32_t)argWidth};
            VL_DO_DANGLING(argp->delete_درخت(), argp);
        } وگرنه اگر (شمار_Ones) {
            sump = build_شمار_Ones_Expansion(fl, argp);
        } وگرنه اگر (شمار_Zeros) {
            // width - countones(x)
            Ast_گره_عبارت* ثابت onesCountp = build_شمار_Ones_Expansion(fl, argp);
            ثابت V3_عدد widthVal{nodep, onesCountp->width(), (uint32_t)argWidth};
            sump = جدید AstSub{fl, جدید AstConst{fl, widthVal}, onesCountp};
            sump->user1(درست);
        } وگرنه {
            sump = جدید AstConst{fl, AstConst::Widthed_مقدار{}, nodep->width(), 0U};
            VL_DO_DANGLING(argp->delete_درخت(), argp);
        }

        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(درست);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(sump);
    }
    تهی visit(AstCLog2* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlinkFrBack();
        ثابت صحیح argWidth = argp->width();
        ثابت صحیح resultWidth = nodep->width();

        Ast_گره_عبارت* resultp
            = جدید AstConst{fl, AstConst::Widthed_مقدار{}, resultWidth, (uint32_t)argWidth};

        برای (صحیح k = argWidth - 1; k >= 0; k--) {
            V3_عدد threshold{fl, argWidth, 0};
            اگر (k < 32)
                threshold.setLong(1ULL << k);
            وگرنه
                threshold.setBit(k, 1);

            AstLte* ثابت ltep
                = جدید AstLte{fl, argp->clone_درخت_Pure(نادرست), جدید AstConst{fl, threshold}};
            ltep->user1(درست);

            AstConst* ثابت valuep
                = جدید AstConst{fl, AstConst::Widthed_مقدار{}, resultWidth, (uint32_t)k};

            resultp = جدید AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(درست);
        }

        VL_DO_DANGLING(argp->delete_درخت(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(resultp);
    }
    تهی visit(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ثابت zerop = جدید AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    تهی handlePow(Ast_گره_Biop* nodep) {
        اگر (AstConst* ثابت exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* ثابت basep = nodep->lhsp();
            ثابت V3_عدد رقم_One{nodep, basep->width(), 1};
            Ast_گره_عبارت* powerp = جدید AstConst{fl, رقم_One};
            ثابت منطقی baseSigned = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ثابت int32_t exponent = baseSigned ? exponentp->toSInt() : exponentp->toUInt();
            اگر (exponent > 0) {
                برای (int32_t i = 0; i < exponent; i++) {
                    اگر (baseSigned) {
                        powerp = جدید AstMulS{fl, powerp, basep->clone_درخت_Pure(نادرست)};
                    } وگرنه {
                        powerp = جدید AstMul{fl, powerp, basep->clone_درخت_Pure(نادرست)};
                    }
                    powerp->user1(درست);
                }
            } وگرنه اگر (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                اگر (baseSigned) {
                    powerp = جدید AstDivS{fl, powerp, basep->clone_درخت_Pure(نادرست)};
                    powerp->user1(درست);
                    powerp = جدید AstDivS{fl, powerp, basep->clone_درخت_Pure(نادرست)};
                    powerp->user1(درست);
                } وگرنه {
                    powerp = جدید AstDiv{fl, powerp, basep->clone_درخت_Pure(نادرست)};
                    powerp->user1(درست);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            iterate(powerp);
        } وگرنه اگر (ثابت AstConst* ثابت basep = VN_CAST(nodep->lhsp(), Const)) {
            اگر (basep->رقم().toUInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_سطر* ثابت fl = nodep->fileline();
                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
                ثابت صحیح width = nodep->width();
                ثابت منطقی rhsDependent = rhsp->user1();
                AstConst* ثابت onep = جدید AstConst{fl, AstConst::Widthed_مقدار{}, width, 1};
                AstShiftL* ثابت shiftp = جدید AstShiftL{fl, onep, rhsp, width};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_گره_عبارت* resultp = shiftp;
                اگر (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_گره_عبارت* ثابت nClonep = rhsp->clone_درخت_Pure(نادرست);
                    nClonep->user1(rhsDependent);
                    AstConst* ثابت zeroNp
                        = جدید AstConst{fl, AstConst::Widthed_مقدار{}, rhsp->width(), 0};
                    AstGteS* ثابت condp = جدید AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ثابت zeroRp = جدید AstConst{fl, AstConst::Widthed_مقدار{}, width, 0};
                    resultp = جدید AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                iterate(resultp);
            } وگرنه {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } وگرنه {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    تهی visit(AstPow* nodep) override { handlePow(nodep); }
    تهی visit(AstPowSS* nodep) override { handlePow(nodep); }
    تهی visit(AstPowSU* nodep) override { handlePow(nodep); }
    تهی visit(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    تهی handleShift(Ast_گره_Biop* nodep) {
        اگر (editFormat(nodep)) بازگشت;
        ثابت صحیح lhsWidth = nodep->lhsp()->width();
        ثابت صحیح rhsWidth = nodep->rhsp()->width();
        اگر (rhsWidth < lhsWidth) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlinkFrBack();
            AstExtend* ثابت extendp = جدید AstExtend{fl, rhsp, lhsWidth};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(درست);
            nodep->rhsp(extendp);
        } وگرنه اگر (rhsWidth > lhsWidth) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_DType* ثابت dtypep = nodep->dtypep();
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت extendp
                = VN_IS(nodep, ShiftRS)
                      ? static_cast<Ast_گره_عبارت*>(جدید AstExtendS{fl, lhsp, rhsWidth})
                      : static_cast<Ast_گره_عبارت*>(جدید AstExtend{fl, lhsp, rhsWidth});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(درست);
            nodep->lhsp(extendp);
            nodep->dtypeSetLogicSized(rhsWidth, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            AstSel* ثابت selp = جدید AstSel{fl, nodep, 0, lhsWidth};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            بازگشت;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    تهی visit(AstShiftL* nodep) override { handleShift(nodep); }
    تهی visit(AstShiftR* nodep) override { handleShift(nodep); }
    تهی visit(AstShiftRS* nodep) override { handleShift(nodep); }
    تهی visit(Ast_گره_Biop* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    تهی visit(Ast_گره_Uniop* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        editSMT(nodep, nodep->lhsp());
    }
    تهی visit(Ast_گره_Triop* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    تهی visit(AstCond* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        اگر (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtypeSetString();
            بازگشت;
        }
        // Fall back to "(ite cond then else)"
        visit(static_cast<Ast_گره_Triop*>(nodep));
    }
    تهی visit(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        اگر (editFormat(nodep)) بازگشت;
        editSMT(nodep, nodep->srcp());
    }
    تهی visit(AstSel* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        VNRelinker handle;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت msbp = جدید AstSFormatF{
            fl, "%1d", نادرست,
            جدید AstAdd{fl, nodep->lsbp()->clone_درخت_Pure(نادرست),
                       جدید AstConst{fl, static_cast<uint32_t>(nodep->widthConst() - 1)}}};
        Ast_گره_عبارت* ثابت lsbp
            = جدید AstSFormatF{fl, "%1d", نادرست, nodep->lsbp()->unlinkFrBack(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    تهی visit(AstStructSel* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        ثابت منطقی outermost = !m_deferRandModeHoist;
        Ast_گره_عبارت* ثابت origp = outermost ? nodep->clone_درخت(نادرست) : nullptr;
        VL_RESTORER(m_structSel);
        m_structSel = درست;
        اگر (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_گره_عبارت* ثابت fromp = nodep->fromp();
            AstStructDType* ثابت structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            اگر (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } وگرنه {
                structp->markConstrainedRand(درست);
            }
        }
        // Mark Random for structArray
        اگر (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_گره_عبارت* ثابت fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ثابت structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            اگر (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } وگرنه {
                structp->markConstrainedRand(درست);
            }
        }
        {
            VL_RESTORER(m_deferRandModeHoist);
            m_deferRandModeHoist = درست;
            iterateChildren(nodep);
        }
        File_سطر* ثابت fl = nodep->fileline();
        AstSFormatF* newp = nullptr;
        اگر (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = جدید AstSFormatF{fl, "%s.%s." + nodep->name(), نادرست,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_درخت_Pure(درست)};
            اگر (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } وگرنه {
            newp = جدید AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), نادرست,
                                   nullptr};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        اگر (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(افزودن_Deletep(origp), origp);
        }
    }
    تهی addStringNamePart(Ast_گره_عبارت* ثابت indexp, ثابت std::string& fmt = "%32x") {
        اگر (m_nestedAccess) {
            Ast_گره_عبارت* ثابت bitp = indexp->clone_درخت_Pure(نادرست);
            Ast_گره_عبارت* ثابت bitFormatp = جدید AstSFormatF{bitp->fileline(), fmt, نادرست, bitp};
            AstSFormatF* ثابت herep
                = جدید AstSFormatF{indexp->fileline(), "%s.%s", نادرست, bitFormatp};
            m_nestedAccess->addVarNamePart(herep, indexp->name());
        }
    }
    تهی visit(AstAssocSel* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        File_سطر* ثابت fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_گره_عبارت* ثابت origp = nodep->clone_درخت(نادرست);
        AstSFormatF* newp = nullptr;
        اگر (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_گره_عبارت* ثابت idxp = جدید AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), نادرست,
                                                      nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وگرنه اگر (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ثابت stringp = VN_AS(nodep->bitp(), CvtPackString);
            ثابت size_t stringSize = VN_AS(stringp->lhsp(), Const)->width();
            اگر (stringSize > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << stringSize << "bits, limit is 128 bits");
            }
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_گره_عبارت* ثابت idxp = جدید AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), نادرست,
                                                      stringp->lhsp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وگرنه {
            اگر (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ثابت صحیح actual_width = nodep->bitp()->width();
                std::string fmt;
                // Normalize to standard bit width
                اگر (actual_width <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } وگرنه اگر (actual_width <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } وگرنه {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::to_string(VL_واژگان_I(actual_width) * 8) + "x";
                }
                اگر (m_nestedAccess) addStringNamePart(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_گره_عبارت* ثابت idxp
                    = جدید AstSFormatF{fl, fmt, نادرست, nodep->bitp()->unlinkFrBack(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } وگرنه {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                اگر (m_nestedAccess) m_nestedAccess->error();
            }
        }
        اگر (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        اگر (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(origp->delete_درخت(), origp);
        }
    }
    تهی rename_Array_عبارت(AstSFormatF* ثابت newp) ثابت {
        اگر (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    تهی visit(AstArraySel* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        File_سطر* ثابت fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        منطقی indexIsRand = نادرست;
        nodep->bitp()->foreach([&](ثابت Ast_گره_Var_Ref* vrefp) {
            اگر (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                indexIsRand = درست;
            }
        });
        اگر (m_nestedAccess) {
            Ast_گره_عبارت* ثابت bitp = nodep->bitp()->clone_درخت_Pure(نادرست);
            Ast_گره_عبارت* ثابت bitFormatp
                = indexIsRand ? جدید AstSFormatF{bitp->fileline(), bitp->name(), نادرست, nullptr}
                              : جدید AstSFormatF{bitp->fileline(), "%x", نادرست, bitp};
            AstSFormatF* ثابت herep
                = جدید AstSFormatF{nodep->fileline(), "%s.%s", نادرست, bitFormatp};
            اگر (AstSel* ثابت selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->addVarNamePart(herep, selp->fromp()->name());
            } وگرنه {
                m_nestedAccess->addVarNamePart(herep, bitp->name());
            }
            اگر (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_درخت(), bitp);
                بازگشت;
            }
        }
        اگر (indexIsRand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_گره_عبارت* indexp = nodep->bitp()->unlinkFrBack(&handle);
            اگر (indexp->width() < 32) {
                AstExtend* ثابت extendp = جدید AstExtend{fl, indexp, 32};
                extendp->user1(درست);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_عبارت(newp);
        } وگرنه {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_گره_عبارت* ثابت origp = nodep->clone_درخت(نادرست);
            Ast_گره_عبارت* ثابت indexp
                = جدید AstSFormatF{fl, "#x%8x", نادرست, nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_عبارت(newp);
            اگر (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
                VL_DO_DANGLING(origp->delete_درخت(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ایستا منطقی isRandModeGate(ثابت Ast_گره_عبارت* exprp) {
        ثابت AstCMethodHard* ثابت atp = VN_CAST(exprp, CMethodHard);
        اگر (!atp || atp->method() != VCMethod::ARRAY_AT) بازگشت نادرست;
        ثابت AstVar* varp = nullptr;
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } وگرنه اگر (ثابت AstMemberSel* ثابت mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        اگر (!varp) بازگشت نادرست;
        ثابت AstClass* ثابت classp = VN_CAST(varp->user2p(), Class);
        بازگشت classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    منطقی hoistRandModeOverSelectAndMember(AstSFormatF* newp, Ast_گره_عبارت* origp) {
        اگر (m_deferRandModeHoist) بازگشت نادرست;
        // Only for selects yielding a non-array element (full chains)
        اگر (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) بازگشت نادرست;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::vector<AstSFormatF*> frames;
        AstCond* modeCondp = nullptr;
        برای (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_گره_عبارت* ثابت firstp = curp->exprsp();
            frames.افزودن_back(curp);
            اگر (AstCond* ثابت condp = VN_CAST(firstp, Cond)) {
                اگر (isRandModeGate(condp->condp())) modeCondp = condp;
                بشکن;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        اگر (!modeCondp) بازگشت نادرست;
        File_سطر* ثابت fl = newp->fileline();
        Ast_گره_عبارت* ثابت modep = modeCondp->condp()->unlinkFrBack();
        Ast_گره_عبارت* activep = modeCondp->thenp()->unlinkFrBack();
        // Rebuild the select chain text around the SMT name, innermost first
        برای (auto it = frames.rbegin(); it != frames.rend(); ++it) {
            Ast_گره_عبارت* ثابت remainingArgsp = VN_CAST((*it)->exprsp()->nextp(), گره_عبارت);
            اگر (remainingArgsp) { remainingArgsp->unlinkFrBackWithNext(); }
            activep = جدید AstSFormatF{fl, (*it)->name(), نادرست,
                                      Ast_گره::addNext(activep, remainingArgsp)};
        }
        AstCond* ثابت hoistp = جدید AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(درست);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(افزودن_Deletep(newp), newp);
        بازگشت درست;
    }
    تهی visit(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        اگر (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_سطر* ثابت fl = nodep->fileline();
            اگر (m_nestedAccess) {
                AstSFormatF* ثابت formatp = جدید AstSFormatF{fl, nodep->name(), نادرست, nullptr};
                m_nestedAccess->addVarNamePart(جدید AstSFormatF{fl, "%s.%s", نادرست, formatp},
                                               nodep->name());
            }
            Ast_گره* ریشه_گره = nodep->fromp();
            وقتی (ثابت AstMemberSel* ثابت selp = VN_CAST(ریشه_گره, MemberSel)) {
                ریشه_گره = selp->fromp();
            }

            Ast_گره_Sel* arraySelp = VN_CAST(ریشه_گره, ArraySel);
            اگر (!arraySelp) arraySelp = VN_CAST(ریشه_گره, AssocSel);

            اگر (arraySelp) {
                Ast_گره_DType* ثابت arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_گره_DType* ثابت elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : nullptr;
                اگر (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ثابت منطقی isSimple
                        = nodep->fromp() == ریشه_گره && VN_IS(arraySelp->fromp(), VarRef);
                    اگر (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = جدید NestedAccessPath(nodep->clone_درخت(نادرست),
                                                              &m_firstExpressionInsideIndexp);
                        AstSFormatF* ثابت formatp
                            = جدید AstSFormatF{fl, nodep->name(), نادرست, nullptr};
                        m_nestedAccess->addVarNamePart(
                            جدید AstSFormatF{fl, "%s.%s", نادرست, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = درست;
                    nodep->user1(درست);
                    arraySelp->user1(درست);
                    Ast_گره_عبارت* ثابت origp = nodep->clone_درخت(نادرست);
                    iterateChildren(nodep);
                    AstSFormatF* ثابت newp = جدید AstSFormatF{fl, "%s." + nodep->name(), نادرست,
                                                              nodep->fromp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    اگر (!hoistRandModeOverSelectAndMember(newp, origp))
                        VL_DO_DANGLING(origp->delete_درخت(), origp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess);
                    بازگشت;
                }
            }
            // Check if the root variable participates in global constraints
            اگر (ثابت AstVarRef* ثابت varRefp = VN_CAST(ریشه_گره, VarRef)) {
                AstVar* ثابت constrainedVar = varRefp->varp();
                اگر (constrainedVar->globalConstrained()) {
                    منطقی ثابت beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    اگر (beforeIterate) {
                        nodep->replaceWith(جدید AstSFormatF{fl, "%s." + nodep->name(), نادرست,
                                                           nodep->fromp()->unlinkFrBack()});
                    } وگرنه {
                        nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                    }
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    بازگشت;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        اگر (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), گره_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ثابت varRefp
                = جدید AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), گره_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            visit(varRefp);
        } وگرنه اگر (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->method() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ثابت cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_گره_DType* ثابت arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_گره_DType* ثابت elemDtp = arrayDtp->subDTypep()->skipRefp();
            اگر (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = درست;
                iterateChildren(nodep);
                File_سطر* ثابت fl = nodep->fileline();
                AstSFormatF* newp = nullptr;
                اگر (AstSFormatF* ثابت fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    اگر (fromp->name() == "%s.%s") {
                        newp = جدید AstSFormatF{fl, "%s.%s." + nodep->name(), نادرست,
                                               fromp->exprsp()->clone_درخت_Pure(درست)};
                    } وگرنه {
                        newp = جدید AstSFormatF{fl, fromp->name() + "." + nodep->name(), نادرست,
                                               nullptr};
                    }
                } وگرنه {
                    newp = جدید AstSFormatF{fl, nodep->name(), نادرست, nullptr};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                بازگشت;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        } وگرنه اگر (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        } وگرنه {
            editFormat(nodep);
        }
    }
    تهی visit(AstSFormatF* nodep) override {}
    تهی visit(Ast_Stmt_عبارت* nodep) override {}
    تهی visit(AstCStmt* nodep) override {}
    تهی visit(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    تهی visit(AstConstraintIf* nodep) override {
        Ast_گره_عبارت* newp = nullptr;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت thenp = editSingle(fl, nodep->thensp());
        Ast_گره_عبارت* ثابت elsep = editSingle(fl, nodep->elsesp());
        اگر (thenp && elsep) {
            newp = جدید AstCond{fl, nodep->condp()->unlinkFrBack(), thenp, elsep};
        } وگرنه اگر (thenp) {
            newp = جدید AstLogIf{fl, nodep->condp()->unlinkFrBack(), thenp};
        } وگرنه اگر (elsep) {
            newp = جدید AstLogIf{fl, جدید AstNot{fl, nodep->condp()->unlinkFrBack()}, elsep};
        }
        اگر (newp) {
            newp->dtypeSetBit();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(درست);  // Assume result-dependent
            nodep->replaceWith(جدید Ast_Constraint_عبارت{fl, newp});
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstGenBlock* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    تهی visit(AstBegin* nodep) override {}
    تهی visit(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_سطر* ثابت fl = nodep->fileline();

        اگر (!nodep->bodyp()) {
            nodep->unlinkFrBack();
        } وگرنه اگر (m_wantSingle) {
            Ast_گره_عبارت* ثابت itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ثابت cstmtp = جدید AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(itemp);
            cstmtp->add(";");
            Ast_گره* ثابت bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ثابت cexprp = جدید AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(جدید AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     جدید AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp},
                                     درست});
            cexprp->add("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(جدید AstSFormatF{fl, "%s", نادرست, cexprp});
        } وگرنه {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            برای (Ast_گره* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                اگر (ثابت AstVar* ثابت varp = VN_CAST(elemp, Var)) m_foreachIdxVars.درج(varp);
            }
            iterateAndNextNull(nodep->bodyp());
            Ast_گره* ثابت bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlinkFrBackWithNext());
            nodep->replaceWith(
                جدید AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                             جدید AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp}, درست});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_Module* ثابت genModp = VN_AS(m_genp->user2p(), گره_Module);

        برای (Ast_گره_عبارت* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), گره_عبارت)) {
            Ast_گره_عبارت* ثابت lhsTestp = build_Solve_Before_Name_عبارت(fl, lhsp);
            اگر (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                ادامه;
            }
            VL_DO_DANGLING(lhsTestp->delete_درخت(), lhsTestp);
            برای (Ast_گره_عبارت* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), گره_عبارت)) {
                Ast_گره_عبارت* ثابت rhsNamep = build_Solve_Before_Name_عبارت(fl, rhsp);
                اگر (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    ادامه;
                }
                AstCMethodHard* ثابت callp = جدید AstCMethodHard{
                    fl, جدید AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtypeSetVoid();
                callp->addPinsp(build_Solve_Before_Name_عبارت(fl, lhsp));
                callp->addPinsp(rhsNamep);
                nodep->addHereThisAsNext(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
    }
    تهی visit(AstConstraintUnique* nodep) override {
        Ast_گره_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_Module* genModp = nullptr;

        اگر (m_classp) {
            initTaskp = VN_AS(m_memberMap.findMember(m_classp, "new"), گره_FTask);
            genVarp = getRandomGenerator(m_classp);
        } وگرنه {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        اگر (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        اگر (m_classp) {
            genModp = VN_AS(genVarp->user2p(), گره_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_گره* setupStmtsp = nullptr;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::vector<std::string> smtExprs;
        برای (Ast_گره* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            اگر (AstVarRef* ثابت varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ثابت varp = varRefp->varp();
                Ast_گره_Module* ثابت varModp = VN_AS(varp->user2p(), گره_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_گره_DType* ثابت dtypep = varp->dtypep()->skipRefp();

                ثابت ایستا auto dynDTypeSupported
                    = [](ثابت Ast_گره_DType* ثابت dtypep) -> منطقی {
                    بازگشت VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elemWidth;
                uint32_t staticSize = 0;  // Element count, zero when only known at run time
                اگر (ثابت AstUnpackArrayDType* ثابت up = VN_CAST(dtypep, UnpackArrayDType)) {
                    ثابت AstRange* ثابت rangep = up->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    staticSize = up->elementsConst();
                    اگر (staticSize > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        ادامه;
                    }
                    elemWidth = static_cast<uint64_t>(varp->dtypep()->width());
                } وگرنه اگر (dynDTypeSupported(dtypep)) {
                    elemWidth = static_cast<uint64_t>(dtypep->subDTypep()->width());
                } وگرنه {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(نادرست));
                    ادامه;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ثابت Ast_گره_DType* ثابت subp = dtypep->subDTypep()->skipRefp();
                اگر (VN_IS(subp, گره_Array_DType) || VN_IS(subp, QueueDType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    ادامه;
                }

                اگر (m_wantSingle && !staticSize) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    ادامه;
                }

                اگر (!varp->user3()) {
                    varp->user3(درست);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_گره_عبارت* ثابت varnamep = جدید AstCExpr{
                        fl, AstCExpr::Pure{}, "\"" + varp->name() + "\"", varp->width()};

                    AstCMethodHard* ثابت writeVarCallp = جدید AstCMethodHard{
                        fl, جدید AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ثابت argVarRefp = جدید AstVarRef{fl, varp, VAccess::READ};
                    اگر (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->addPinsp(argVarRefp);
                    writeVarCallp->addPinsp(جدید AstConst{fl, AstConst::Unsized64{}, elemWidth});
                    writeVarCallp->addPinsp(varnamep);
                    writeVarCallp->addPinsp(جدید AstConst{fl, 1});  // Dimension
                    ثابت RandomizeMode randMode = {.asUQuad = varp->user1()};
                    اگر (randMode.usesMode) {
                        writeVarCallp->addPinsp(
                            جدید AstConst{fl, AstConst::Unsized64{}, randMode.index});
                    }
                    writeVarCallp->dtypeSetVoid();
                    initTaskp->addStmtsp(جدید Ast_Stmt_عبارت{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                اگر (varp->user4p() && m_sizeConstrainedArraysp) {
                    m_sizeConstrainedArraysp->درج(varp);
                }

                اگر (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    اگر (staticSize < 2) ادامه;
                    std::string عبارت_Str = "(__Vbv (distinct";
                    برای (uint32_t i = 0; i < staticSize; ++i) {
                        عبارت_Str += " (select " + varp->name() + " #x";
                        برای (صحیح shift = 28; shift >= 0; shift -= 4) {
                            عبارت_Str += "0123456789abcdef"[(i >> shift) & 0xf];
                        }
                        عبارت_Str += ')';
                    }
                    smtExprs.emplace_back(عبارت_Str + "))");
                    ادامه;
                }

                Ast_گره_عبارت* ثابت namep = جدید AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ثابت randUniqueCallp
                    = جدید AstCMethodHard{fl, جدید AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->findVoidDType());
                randUniqueCallp->addPinsp(جدید AstConst{fl, *m_uniqueConstraintId});
                setupStmtsp = Ast_گره::addNext(setupStmtsp, جدید Ast_Stmt_عبارت{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        اگر (m_wantSingle && !smtExprs.empty()) {
            std::string عبارت_Str = smtExprs.front();
            اگر (smtExprs.size() > 1) {
                عبارت_Str = "(bvand";
                برای (ثابت std::string& oneExprp : smtExprs) عبارت_Str += " " + oneExprp;
                عبارت_Str += ')';
            }
            AstSFormatF* ثابت newp = جدید AstSFormatF{fl, عبارت_Str, نادرست, nullptr};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازگشت;
        }
        اگر (setupStmtsp) nodep->addHereThisAsNext(setupStmtsp);
        nodep->unlinkFrBack();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    تهی visit(Ast_Constraint_عبارت* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_firstExpressionInsideIndexp);
        اگر (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_گره_عبارت* ثابت exprp = nodep->exprp();
            اگر (exprp->width() > 1) {
                File_سطر* ثابت fl = exprp->fileline();
                ثابت V3_عدد رقم_Zero{fl, exprp->width(), 0};
                Ast_گره_عبارت* ثابت neqp
                    = جدید AstNeq{fl, exprp->unlinkFrBack(), جدید AstConst{fl, رقم_Zero}};
                neqp->user1(درست);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        اگر (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ثابت VCMethod method
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ثابت callp = جدید AstCMethodHard{
            nodep->fileline(),
            جدید AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                          VAccess::READWRITE},
            method, nodep->exprp()->unlinkFrBack()};
        callp->dtypeSetVoid();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->addPinsp(جدید AstCExpr{nodep->fileline(), AstCExpr::Pure{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ثابت uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->addPinsp(جدید AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string prettyText;
        اگر (!v3Global.opt.protectIds()) {
            prettyText = nodep->fileline()->prettySource();
            size_t pos = 0;
            وقتی ((pos = prettyText.find('"', pos)) != std::string::npos) {
                prettyText.درج(pos, "\\");
                pos += std::strlen("\\\"");
            }
        }
        callp->addPinsp(
            جدید AstCExpr{nodep->fileline(), AstCExpr::Pure{}, "\"" + prettyText + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(AstCMethodHard* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        File_سطر* ثابت fl = nodep->fileline();

        اگر (nodep->method() == VCMethod::ARRAY_AT) {
            addStringNamePart(nodep->pinsp(), "%x");
            ثابت منطقی indexIsRand = nodep->pinsp()->exists([](ثابت Ast_گره_Var_Ref* ثابت vrefp) {
                بازگشت vrefp->varp()->rand().isRandomizable();
            });
            اگر (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ثابت Ast_گره* ثابت basep = nodep->baseFromp(درست);
                    اگر (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        بازگشت;
                    }
                }
                Ast_گره_عبارت* ثابت origp = indexIsRand ? nullptr : nodep->clone_درخت(نادرست);
                ثابت AstVarRef* ثابت baseRefp
                    = m_structSel ? VN_CAST(nodep->fromp()->baseFromp(درست), VarRef) : nullptr;
                اگر (baseRefp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(baseRefp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_گره_عبارت* ثابت indexp = nodep->pinsp()->clone_درخت_Pure(نادرست);
                    Ast_گره_عبارت* ثابت arrayp = nodep->fromp()->clone_درخت_Pure(نادرست);
                    AstCMethodHard* ثابت currentSizep
                        = جدید AstCMethodHard{fl, arrayp->clone_درخت_Pure(نادرست), VCMethod::DYN_SIZE};
                    currentSizep->dtypeSetInt();
                    Ast_گره_عبارت* ثابت inRangeRequiredp = جدید AstLogAnd{
                        fl,
                        جدید AstLteS{fl,
                                    جدید AstConst{fl, AstConst::Widthed_مقدار{}, indexp->width(), 0},
                                    indexp->clone_درخت_Pure(نادرست)},
                        جدید AstGteS{fl, indexp->clone_درخت_Pure(نادرست), currentSizep}};
                    AstCMethodHard* ثابت resizep
                        = جدید AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             جدید AstAdd{fl, indexp, جدید AstConst{fl, 1}}};
                    resizep->dtypeSetVoid();
                    m_prepareConstrainedArraysp->addStmtsp(
                        جدید AstIf{fl, inRangeRequiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ثابت sizep
                    = m_structSel ? جدید AstCMethodHard{fl, nodep->fromp()->clone_درخت_Pure(نادرست),
                                                       VCMethod::DYN_SIZE}
                                  : nullptr;
                Ast_گره_عبارت* ثابت originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_گره_عبارت* ثابت pinp = nodep->pinsp()->unlinkFrBack();
                AstSFormatF* newp;
                اگر (m_structSel) {
                    اگر (AstSFormatF* ثابت sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_گره_عبارت* ثابت argsp
                        = Ast_گره::addNext(nodep->fromp()->unlinkFrBack(), pinp);
                    sizep->dtypeSetInt();
                    AstLogAnd* ثابت condp
                        = جدید AstLogAnd{fl,
                                        جدید AstLteS{fl,
                                                    جدید AstConst{fl, AstConst::Widthed_مقدار{},
                                                                 originalPinp->width(), 0},
                                                    originalPinp->clone_درخت_Pure(نادرست)},
                                        جدید AstLtS{fl, originalPinp->clone_درخت_Pure(نادرست), sizep}};
                    m_conditionp = m_conditionp ? جدید AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = جدید AstSFormatF{fl, "%s.%s", نادرست, argsp};
                } وگرنه {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                اگر (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_درخت(), origp);
                }

            } وگرنه اگر (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } وگرنه {
                nodep->user1(نادرست);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            بازگشت;
        }

        اگر (nodep->method() == VCMethod::ARRAY_INSIDE) {
            ثابت منطقی randArr = nodep->fromp()->user1();

            AstVar* ثابت newVarp
                = جدید AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->findIntDType()};
            Ast_گره_عبارت* ثابت idxRefp = جدید AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            AstForeachHeader* ثابت headerp
                = جدید AstForeachHeader{fl, nodep->fromp()->clone_درخت_Pure(نادرست), newVarp};
            Ast_گره_عبارت* ثابت selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_گره* ثابت itemp = جدید AstEq{fl, selp, nodep->pinsp()->unlinkFrBack()};
            itemp->user1(درست);

            AstCStmt* ثابت cstmtp = جدید AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(iterateSubtreeReturnEdits(itemp));
            cstmtp->add(";");
            AstCExpr* ثابت cexprp = جدید AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(جدید AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     جدید AstForeach{fl, headerp, cstmtp}, درست});
            cexprp->add("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(جدید AstSFormatF{fl, "%s", نادرست, cexprp});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        }

        // Array reduction methods (sum, product, and, or, xor)
        اگر (nodep->method() == VCMethod::ARRAY_R_SUM
            || nodep->method() == VCMethod::ARRAY_R_PRODUCT
            || nodep->method() == VCMethod::ARRAY_R_AND || nodep->method() == VCMethod::ARRAY_R_OR
            || nodep->method() == VCMethod::ARRAY_R_XOR) {

            AstWith* ثابت withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            اگر (withp) {
                اگر (AstUnpackArrayDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ثابت صحیح arraySize = adtypep->elementsConst();
                    اگر (arraySize > v3Global.opt.constraintArrayLimit()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvtToStr(arraySize)
                                          + " exceeds --constraint-array-limit of "
                                          + cvtToStr(v3Global.opt.constraintArrayLimit())
                                          + "), treating as state");
                        nodep->user1(نادرست);
                        اگر (editFormat(nodep)) بازگشت;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        بازگشت;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ثابت منطقی hasRandomArray = nodep->fromp()->user1();
            اگر (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(نادرست);
                اگر (editFormat(nodep)) بازگشت;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                بازگشت;
            }
            اگر (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                بازگشت;
            }

            // Create loop variable and header
            AstVar* ثابت loopVarp
                = جدید AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->findIntDType()};
            AstForeachHeader* ثابت headerp
                = جدید AstForeachHeader{fl, nodep->fromp()->clone_درخت_Pure(نادرست), loopVarp};

            // Determine SMT operation and compute identity elements
            ثابت نویسه* smtOp = nullptr;
            std::string identity;
            اگر (withp) {
                // For 'with' clause: use binary format, compute from result width
                ثابت صحیح width = nodep->dtypep()->width();
                اگر (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = "#b" + std::string(width, '0');
                } وگرنه اگر (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = (width > 0) ? "#b" + std::string(width - 1, '0') + "1" : "#b0";
                } وگرنه اگر (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = "#b" + std::string(width, '1');
                } وگرنه اگر (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = "#b" + std::string(width, '0');
                } وگرنه {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = "#b" + std::string(width, '0');
                }
            } وگرنه {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ثابت arrVarp = arrRefp->varp();
                Ast_گره_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحیح elemWidth = elemDtp->width();
                ثابت صحیح hexDigits = (elemWidth + 3) / 4;

                ثابت std::string zeroIdentity = "#x" + std::string(hexDigits, '0');
                ثابت std::string oneIdentity = "#x" + std::string(hexDigits - 1, '0') + "1";
                ثابت std::string allOnesIdentity = "#x" + std::string(hexDigits, 'f');

                اگر (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = zeroIdentity;
                } وگرنه اگر (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = oneIdentity;
                } وگرنه اگر (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = allOnesIdentity;
                } وگرنه اگر (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = zeroIdentity;
                } وگرنه {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = zeroIdentity;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ثابت cstmtp = جدید AstCStmt{fl};
            اگر (withp) {
                // With 'with' clause: evaluate expression for each element
                ثابت منطقی randArr = nodep->fromp()->user1();
                Ast_گره_عبارت* ثابت idxRefp = جدید AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_گره_عبارت* ثابت elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ثابت صحیح resultWidth = nodep->dtypep()->width();
                ثابت VSigning resultSigning = nodep->dtypep()->numeric();

                Ast_گره* perElemExprp = withp->exprp()->clone_درخت_Pure(نادرست);
                اگر (AstLambdaArgRef* ثابت ریشه_Refp = VN_CAST(perElemExprp, LambdaArgRef)) {
                    اگر (ریشه_Refp->index()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ثابت loopRef = جدید AstVarRef{fl, loopVarp, VAccess::READ};
                        perElemExprp = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                    } وگرنه {
                        // item at root -> replace with array element
                        perElemExprp = elemSelp->clone_درخت_Pure(نادرست);
                    }
                    VL_DO_DANGLING(ریشه_Refp->delete_درخت(), ریشه_Refp);
                } وگرنه {
                    perElemExprp->foreach([&](AstLambdaArgRef* refp) {
                        اگر (refp->index()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ثابت loopRef = جدید AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_گره* ثابت replacement
                                = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                            refp->replaceWith(replacement);
                        } وگرنه {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_درخت_Pure(نادرست));
                        }
                        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_درخت(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                perElemExprp = V3Const::constifyEdit(perElemExprp);

                perElemExprp->foreach([&](Ast_گره* nodep) {
                    // Don't mark loop variable references as randomizable
                    اگر (!VN_IS(nodep, Const)) {
                        اگر (Ast_گره_Var_Ref* ثابت vrefp = VN_CAST(nodep, گره_Var_Ref)) {
                            اگر (vrefp->varp() != loopVarp) nodep->user1(درست);
                        } وگرنه {
                            nodep->user1(درست);
                        }
                    }
                });

                cstmtp->add("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(iterateSubtreeReturnEdits(perElemExprp));
                cstmtp->add(";\n");
                cstmtp->add("ret += \")\";\n");
            } وگرنه {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ثابت arrVarp = arrRefp->varp();
                ثابت std::string smtArrayName = arrVarp->name();
                Ast_گره_DType* ثابت elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحیح elemWidth = elemDtp->width();
                Ast_گره_Module* ثابت arrModulep = VN_AS(arrVarp->user2p(), گره_Module);
                Ast_گره_Module* ثابت genModulep = VN_AS(m_genp->user2p(), گره_Module);
                arrVarp->user3(درست);

                // Add write_var call to init task
                Ast_گره_عبارت* ثابت arrVarNamep
                    = جدید AstCExpr{fl, AstCExpr::Pure{}, "\"" + smtArrayName + "\"", elemWidth};
                AstCMethodHard* ثابت writeVarCallp
                    = جدید AstCMethodHard{fl, جدید AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->addPinsp(جدید AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->addPinsp(
                    جدید AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elemWidth)});
                writeVarCallp->addPinsp(arrVarNamep);
                ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(نادرست).second;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->addPinsp(جدید AstConst{fl, 1});  // Dimension

                ثابت RandomizeMode randMode = {.asUQuad = arrVarp->user1()};
                اگر (randMode.usesMode) {
                    writeVarCallp->addPinsp(
                        جدید AstConst{fl, AstConst::Unsized64{}, randMode.index});
                }
                writeVarCallp->dtypeSetVoid();
                Ast_گره_FTask* initTaskp = m_inlineInitTaskp;
                اگر (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.findMember(arrModulep, "new"), گره_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->addStmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ثابت idxFormatp = جدید AstSFormatF{
                    fl, "#x%08x", نادرست, جدید AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ثابت varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), idxFormatp);

                cstmtp->add("ret += \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(varNamep);
                cstmtp->add(";\n");
            }

            // Build final expression
            اگر (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ثابت cexprp = جدید AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret = \"" + identity + "\";\n");
                cexprp->add(جدید AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         جدید AstForeach{fl, headerp, cstmtp}, درست});
                cexprp->add("return ret;\n})()");
                nodep->replaceWith(جدید AstSFormatF{fl, "%s", نادرست, cexprp});
            } وگرنه {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ثابت cexprp = جدید AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret;\n");
                cexprp->add(جدید AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         جدید AstForeach{fl, headerp, cstmtp}, درست});
                cexprp->add("return ret.empty() ? \"" + identity + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(جدید AstSFormatF{fl, "%s", نادرست, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        }

        // We've handled DYN_SIZE already if they should be randomized
        اگر (nodep->method() == VCMethod::DYN_SIZE || nodep->method() == VCMethod::ASSOC_SIZE) {
            Ast_گره_عبارت* گره_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            وقتی (VN_IS(گره_Exprp, MemberSel)) {
                گره_Exprp = VN_AS(گره_Exprp, MemberSel)->fromp();
            }
            اگر (VN_IS(گره_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_درخت_Pure(نادرست)));
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                بازگشت;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(نادرست);

        اگر (editFormat(nodep)) بازگشت;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        اگر (editFormat(nodep)) بازگشت;

        AstFunc* ثابت funcp = VN_CAST(nodep->taskp(), Func);
        اگر (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            بازگشت;
        }

        Ast_گره_عبارت* ثابت retExprp = extract_Return_عبارت(funcp);
        اگر (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(نادرست);
            اگر (editFormat(nodep)) بازگشت;
            بازگشت;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), nullptr, نادرست);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_گره_عبارت* inlinedp = retExprp->clone_درخت_Pure(نادرست);
        برای (ثابت auto& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.first;
            AstArg* ثابت argp = tconnect.second;
            اگر (!argp || !argp->exprp()) ادامه;
            اگر (AstVarRef* ثابت ریشه_Refp = VN_CAST(inlinedp, VarRef)) {
                اگر (ریشه_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_درخت_Pure(نادرست);
                    VL_DO_DANGLING(ریشه_Refp->delete_درخت(), ریشه_Refp);
                    ادامه;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                اگر (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_درخت_Pure(نادرست));
                    VL_DO_DANGLING(refp->delete_درخت(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    تهی visit(Ast_گره_Stmt* nodep) override {}
    تهی visit(Ast_گره_عبارت* nodep) override {
        اگر (editFormat(nodep)) بازگشت;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    تهی visit(Ast_گره* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_گره* buildDisableSoftCallStmt(Ast_Constraint_عبارت* nodep) {
        std::string varName;
        اگر (ثابت Ast_گره_Var_Ref* ثابت vrefp = VN_CAST(nodep->exprp(), گره_Var_Ref)) {
            varName = vrefp->name();
        } وگرنه اگر (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } وگرنه {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ثابت callp = جدید AstCMethodHard{
            nodep->fileline(),
            جدید AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            جدید AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtypeSetVoid();
        بازگشت callp->makeStmt();
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
    Ast_گره* extractConditionalDisableSofts(Ast_گره* itemsp, Ast_گره_عبارت* outerCondp) {
        Ast_گره* hoistListp = nullptr;
        برای (Ast_گره *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            اگر (AstConstraintIf* ثابت ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_سطر* ثابت fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_گره_عبارت* ثابت thenCondp
                    = outerCondp ? static_cast<Ast_گره_عبارت*>(
                                       جدید AstLogAnd{fl, outerCondp->clone_درخت_Pure(نادرست),
                                                     ifp->condp()->clone_درخت_Pure(نادرست)})
                                 : ifp->condp()->clone_درخت_Pure(نادرست);
                اگر (Ast_گره* ثابت thenHoistp
                    = extractConditionalDisableSofts(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_گره::addNext(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_درخت(), thenCondp);
                اگر (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_گره_عبارت* ثابت notInnerp
                        = جدید AstLogNot{fl, ifp->condp()->clone_درخت_Pure(نادرست)};
                    Ast_گره_عبارت* ثابت elseCondp
                        = outerCondp ? static_cast<Ast_گره_عبارت*>(جدید AstLogAnd{
                                           fl, outerCondp->clone_درخت_Pure(نادرست), notInnerp})
                                     : notInnerp;
                    اگر (Ast_گره* ثابت elseHoistp
                        = extractConditionalDisableSofts(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_گره::addNext(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_درخت(), elseCondp);
                }
            } وگرنه اگر (Ast_Constraint_عبارت* ثابت exprp = VN_CAST(stmtp, Constraint_عبارت)) {
                اگر (exprp->isDisableSoft() && outerCondp) {
                    File_سطر* ثابت fl = exprp->fileline();
                    AstIf* ثابت hoistIfp = جدید AstIf{fl, outerCondp->clone_درخت_Pure(نادرست),
                                                      buildDisableSoftCallStmt(exprp), nullptr};
                    hoistListp = hoistListp ? Ast_گره::addNext(hoistListp,
                                                               static_cast<Ast_گره*>(hoistIfp))
                                            : static_cast<Ast_گره*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_عبارت* ثابت truep
                        = جدید Ast_Constraint_عبارت{fl, جدید AstConst{fl, AstConst::BitTrue{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        بازگشت hoistListp;
    }

عمومی:
    // CONSTRUCTORS
    explicit Constraint_عبارت_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_گره* nodep,
                                   Ast_گره_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* randModeVarp, std::set<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3UniqueNames& uniqueNames,
                                   Ast_گره_FTask* memberselInitTaskp = nullptr,
                                   VInsertionSet<AstVar*>* sizeConstrainedArraysp = nullptr,
                                   Ast_گره_FTask* prepareConstrainedArraysp = nullptr)
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
        اگر (Ast_گره* ثابت hoistListp
            = nodep ? extractConditionalDisableSofts(nodep, nullptr) : nullptr) {
            nodep->addNext(hoistListp);
        }
        iterateAndNextNull(nodep);
    }
};

شمارش رده CaptureMode : uint8_t {
    CAP_NO = 0x0,
    CAP_مقدار = 0x01,
    CAP_THIS = 0x02,
    CAP_F_SET_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
CaptureMode operator|(CaptureMode a, CaptureMode b) {
    بازگشت static_cast<CaptureMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
CaptureMode operator&(CaptureMode a, CaptureMode b) {
    بازگشت static_cast<CaptureMode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
CaptureMode mode(CaptureMode a) { بازگشت a & static_cast<CaptureMode>(0x3); }
منطقی hasFlags(CaptureMode a, CaptureMode flags) {
    بازگشت ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

رده CaptureVisitor final : عمومی VNVisitor {
    AstArg* m_argsp = nullptr;  // Original references turned into arguments
    Ast_گره_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ثابت AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::set<Ast_گره*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = nullptr;  // Variable for outer context's object, if necessary
    منطقی m_staticContext = نادرست;  // True when capturing from a static function context

    // METHODS

    منطقی captureVariable(File_سطر* ثابت fileline, Ast_گره_Var_Ref* varrefp, AstVar*& varp) {
        auto it = m_varCloneMap.find(varrefp->varp());
        اگر (it == m_varCloneMap.end()) {
            AstVar* ثابت newVarp = varrefp->varp()->clone_درخت(نادرست);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(درست);
            newVarp->direction(VDirection::درون_داد);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            بازگشت درست;
        }
        varp = it->second;
        بازگشت نادرست;
    }

    template <typename T_گره>
    تهی fixupClassOrPackage(Ast_گره* memberp, T_گره refp) {
        Ast_گره_Module* ثابت declClassp = VN_AS(memberp->user2p(), گره_Module);
        اگر (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    template <typename T_گره>
    منطقی isReferenceToInnerMember(T_گره nodep) {
        بازگشت VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_سطر* fl) {
        اگر (!m_thisp) {
            AstClassRefDType* ثابت refDTypep
                = جدید AstClassRefDType{fl, VN_AS(m_callerp, Class), nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
            m_thisp = جدید AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(درست);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::درون_داد);
            m_argsp = Ast_گره::addNext(m_argsp, جدید AstArg{fl, "", جدید AstThisRef{fl, refDTypep}});
        }
        بازگشت m_thisp;
    }

    AstVar* getVar(AstVar* ثابت varp) ثابت {
        ثابت auto it = m_varCloneMap.find(varp);
        اگر (it == m_varCloneMap.end()) بازگشت nullptr;
        اگر (it->second->isStdRandomizeArg()) بازگشت nullptr;
        بازگشت it->second;
    }

    CaptureMode getVarRefCaptureMode(Ast_گره_Var_Ref* varRefp) {
        Ast_گره_Module* ثابت varModp = VN_AS(varRefp->varp()->user2p(), گره_Module);
        AstClass* ثابت varClassp = VN_CAST(varModp, Class);
        AstClass* ثابت callerClassp = VN_CAST(m_callerp, Class);

        ثابت منطقی callerIsClass = callerClassp;
        ثابت منطقی refIsXref = VN_IS(varRefp, VarXRef);
        ثابت منطقی varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ثابت منطقی varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ثابت منطقی varIsFieldOfCaller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ثابت منطقی varIsParam = varRefp->varp()->isParam();
        ثابت منطقی varIsConstraintIterator
            = VN_IS(varRefp->varp()->firstAbovep(), ForeachHeader)
              && VN_IS(varRefp->varp()->firstAbovep()->firstAbovep(), ConstraintForeach);
        اگر (refIsXref) بازگشت CaptureMode::CAP_مقدار | CaptureMode::CAP_F_XREF;
        اگر (varIsConstraintIterator) بازگشت CaptureMode::CAP_NO;
        اگر (varIsFuncLocal && varHasAutomaticLifetime) بازگشت CaptureMode::CAP_مقدار;
        اگر (varIsParam) بازگشت CaptureMode::CAP_مقدار;
        // Static var in function (will not be inlined, because it's in class)
        اگر (callerIsClass && varIsFuncLocal) بازگشت CaptureMode::CAP_مقدار;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        اگر (m_staticContext && callerIsClass && varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            بازگشت CaptureMode::CAP_NO;
        اگر (callerIsClass && varIsFieldOfCaller) بازگشت CaptureMode::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        اگر (callerIsClass && varClassp && !varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            بازگشت CaptureMode::CAP_مقدار;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        بازگشت CaptureMode::CAP_مقدار;
    }

    تهی capture_Ref_By_مقدار(Ast_گره_Var_Ref* nodep, CaptureMode capModeFlags) {
        AstVar* newVarp;
        ثابت منطقی newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_گره_Var_Ref* ثابت newVarRefp = newCapture ? nodep->clone_درخت(نادرست) : nullptr;
        اگر (!hasFlags(capModeFlags, CaptureMode::CAP_F_SET_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(nullptr);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        اگر (!newCapture) بازگشت;
        اگر (hasFlags(capModeFlags, CaptureMode::CAP_F_XREF)) {
            AstVarRef* ثابت notXVarRefp
                = جدید AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_گره::addNext(m_argsp, جدید AstArg{nodep->fileline(), "", newVarRefp});
    }

    تهی captureRefByThis(Ast_گره_Var_Ref* nodep, CaptureMode /*capModeFlags*/) {
        AstVar* ثابت thisp = importThisp(nodep->fileline());
        AstVarRef* ثابت thisRefp = جدید AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(درست);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ثابت memberSelp
            = جدید AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        اگر (!m_targetp && nodep->user1()) memberSelp->user1(درست);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (m_ignore.شمار(nodep)) بازگشت;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        اگر (nodep->varp()->isStdRandomizeArg()) بازگشت;
        ثابت CaptureMode capMode = getVarRefCaptureMode(nodep);
        اگر (mode(capMode) == CaptureMode::CAP_NO) بازگشت;
        اگر (mode(capMode) == CaptureMode::CAP_مقدار) capture_Ref_By_مقدار(nodep, capMode);
        اگر (mode(capMode) == CaptureMode::CAP_THIS) captureRefByThis(nodep, capMode);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        اگر (m_ignore.شمار(nodep)) {
            iterateChildren(nodep);
            بازگشت;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        اگر (VN_IS(nodep, MethodCall) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            بازگشت;
        }
        AstClass* ثابت classp = VN_CAST(nodep->taskp()->user2p(), Class);
        اگر ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ثابت argsp = nodep->argsp();
            اگر (argsp) argsp->unlinkFrBack();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ثابت thisp = importThisp(nodep->fileline());
            AstVarRef* ثابت thisRefp = جدید AstVarRef{
                nodep->fileline(), thisp, nodep->isPure() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            AstMethodCall* ثابت methodCallp
                = جدید AstMethodCall{nodep->fileline(), thisRefp, thisp->name(), argsp};
            methodCallp->taskp(nodep->taskp());
            methodCallp->dtypep(nodep->dtypep());
            nodep->replaceWith(methodCallp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            m_ignore.emplace(methodCallp);
        }
    }
    تهی visit(AstMemberSel* nodep) override {
        اگر (!isReferenceToInnerMember(nodep)) {
            iterateChildren(nodep);
            بازگشت;
        }
        AstVarRef* ثابت varRefp
            = جدید AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    تهی visit(AstMethodCall* nodep) override {
        اگر (!isReferenceToInnerMember(nodep) || m_ignore.شمار(nodep)) {
            iterateChildren(nodep);
            بازگشت;
        }
        AstArg* ثابت argsp = nodep->argsp();
        اگر (argsp) argsp->unlinkFrBackWithNext();
        Ast_گره_FTask_Ref* taskRefp = nullptr;
        اگر (AstTask* ثابت taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = جدید AstTaskRef{nodep->fileline(), taskp, argsp};
        } وگرنه اگر (AstFunc* ثابت taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = جدید AstFuncRef{nodep->fileline(), taskp, argsp};
        } وگرنه {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    explicit CaptureVisitor(Ast_گره* ثابت nodep, Ast_گره_Module* callerp, AstClass* ثابت targetp,
                            منطقی staticContext = نادرست)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterateAndNextNull(nodep);
    }

    // PUBLIC METHODS

    AstArg* getArgs() ثابت { بازگشت m_argsp; }
    AstVar* getThisp() ثابت { بازگشت m_thisp; }

    تهی addFunctionArguments(Ast_گره_FTask* funcp) ثابت {
        برای (AstArg* argp = getArgs(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            اگر (Ast_گره_Var_Ref* varrefp = VN_CAST(argp->exprp(), گره_Var_Ref)) {
                اگر ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(nullptr);
                }
                funcp->addStmtsp(getVar(varrefp->varp()));
            } وگرنه {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->addStmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

رده RandomizeVisitor final : عمومی VNVisitor {
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
    ثابت VNUser3InUse m_inuser3;
    ثابت VNUser4InUse m_inuser4;

    // STATE
    V3UniqueNames m_inlineUniqueNames;  // For generating unique function names
    V3UniqueNames m_randArrayUniqueNames{"__Vrandarr"};  // Random array loop indices
    V3UniqueNames m_modeUniqueNames{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3UniqueNames m_inlineUniqueStdName{"__VStdrand"};
    V3UniqueNames m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_گره_Module* m_modp = nullptr;  // Current module
    std::unordered_map<Ast_گره_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گره_Stmt* m_stmtp = nullptr;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = nullptr;  // Dynamic array type (for rand mode)
    صحیح m_rand_Case_رقم = 0;  // Randcase number within a module for var naming
    صحیح m_dist_رقم = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = nullptr;  // Current constraint
    std::set<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertionSet<AstVar*>> m_sizeConstrainedArrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_staticConstraintModeVars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_staticRandModeVars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<منطقی, منطقی>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ایستا منطقی is_Similar_گره(ثابت Ast_گره_عبارت* with_عبارت, ثابت Ast_گره_عبارت* arg_عبارت) {
        // VarRef: compare variable pointers
        اگر (VN_IS(arg_عبارت, VarRef) && VN_IS(with_عبارت, VarRef)) {
            بازگشت VN_AS(with_عبارت, VarRef)->varp() == VN_AS(arg_عبارت, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        اگر (VN_IS(arg_عبارت, MemberSel) && VN_IS(with_عبارت, MemberSel)) {
            ثابت AstMemberSel* ثابت withMSp = VN_AS(with_عبارت, MemberSel);
            ثابت AstMemberSel* ثابت argMSp = VN_AS(arg_عبارت, MemberSel);
            اگر (withMSp->varp() != argMSp->varp()) بازگشت نادرست;
            // Recursively compare the base object expression
            بازگشت is_Similar_گره(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        اگر (VN_IS(arg_عبارت, ArraySel) && VN_IS(with_عبارت, ArraySel)) {
            ثابت AstArraySel* ثابت withASp = VN_AS(with_عبارت, ArraySel);
            ثابت AstArraySel* ثابت argASp = VN_AS(arg_عبارت, ArraySel);
            // Before checking Sel type index, check if both are Const
            اگر (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ثابت AstConst* ثابت withIdxp = VN_AS(withASp->bitp(), Const);
                ثابت AstConst* ثابت argIdxp = VN_AS(argASp->bitp(), Const);
                بازگشت is_Similar_گره(withASp->fromp(), argASp->fromp())
                       && withIdxp->رقم().isCaseEq(argIdxp->رقم());
            }
            // Index must be Sel type, extract VarRef using fromp()
            اگر (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) بازگشت نادرست;
            ثابت Ast_گره_عبارت* ثابت withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ثابت Ast_گره_عبارت* ثابت argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            بازگشت is_Similar_گره(withASp->fromp(), argASp->fromp())
                   && is_Similar_گره(withIdxp, argIdxp);
        }
        بازگشت نادرست;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ایستا تهی expandUniqueElementList(Ast_گره* itemsp) {
        Ast_گره* itemp = itemsp;
        وقتی (itemp) {
            Ast_گره* ثابت nextp = itemp->nextp();
            اگر (ثابت AstConstraintIf* ثابت ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                ادامه;
            }
            اگر (ثابت AstConstraintForeach* ثابت foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                ادامه;
            }
            AstConstraintUnique* ثابت uniquep = VN_CAST(itemp, ConstraintUnique);
            اگر (!uniquep) {
                itemp = nextp;
                ادامه;
            }

            std::vector<Ast_گره_عبارت*> عبارت_اقلام;
            منطقی hasArrayVarRef = نادرست;
            برای (Ast_گره* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                اگر (ثابت AstVarRef* ثابت vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ثابت Ast_گره_DType* ثابت dtypep = vrp->varp()->dtypep()->skipRefp();
                    اگر (VN_IS(dtypep, گره_Array_DType) || VN_IS(dtypep, QueueDType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = درست;
                        ادامه;
                    }
                }
                عبارت_اقلام.افزودن_back(VN_AS(rp, گره_عبارت));
            }

            اگر (عبارت_اقلام.size() >= 2) {
                File_سطر* ثابت fl = uniquep->fileline();
                برای (size_t i = 0; i < عبارت_اقلام.size(); i++) {
                    برای (size_t j = i + 1; j < عبارت_اقلام.size(); j++) {
                        Ast_گره_عبارت* ثابت lhsp = عبارت_اقلام[i]->clone_درخت(نادرست);
                        Ast_گره_عبارت* ثابت rhsp = عبارت_اقلام[j]->clone_درخت(نادرست);
                        AstNeq* ثابت neqp = جدید AstNeq{fl, lhsp, rhsp};
                        neqp->user1(درست);
                        Ast_Constraint_عبارت* ثابت cexprp = جدید Ast_Constraint_عبارت{fl, neqp};
                        uniquep->addNextHere(cexprp);
                    }
                }
                اگر (!hasArrayVarRef) {
                    uniquep->unlinkFrBack();
                    VL_DO_DANGLING(uniquep->delete_درخت(), uniquep);
                }
            } وگرنه اگر (عبارت_اقلام.size() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlinkFrBack();
                VL_DO_DANGLING(uniquep->delete_درخت(), uniquep);
            }
            itemp = nextp;
        }
    }
    تهی createRandomGenerator(AstClass* ثابت classp) {
        اگر (classp->user3p()) بازگشت;
        اگر (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            بازگشت;
        }
        AstVar* ثابت genp = جدید AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->addMembersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_گره_Module* ثابت modp) {
        auto it = m_stdMap.find(modp);
        اگر (it == m_stdMap.end()) {
            AstVar* ثابت stdgenp
                = جدید AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->findBasicDType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3ErrorCode::IMPURE, درست);
            modp->addStmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            بازگشت stdgenp;
        }
        بازگشت it->second;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ثابت classp) {
        ایستا ثابت نویسه* ثابت name = "__Vprepare_constrained_arrays";
        AstTask* ثابت taskp = جدید AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(درست);
        classp->addMembersp(taskp);
        m_memberMap.درج(classp, taskp);
        بازگشت taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ثابت classp) {
        ایستا constexpr ثابت نویسه* ثابت name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.findMember(classp, name), Task);
        اگر (resizeTaskp) بازگشت resizeTaskp;
        resizeTaskp = جدید AstTask{classp->fileline(), name, nullptr};
        resizeTaskp->classMethod(درست);
        resizeTaskp->isVirtual(درست);
        classp->addMembersp(resizeTaskp);
        m_memberMap.درج(classp, resizeTaskp);
        بازگشت resizeTaskp;
    }
    AstVar* getCreateRandModeVar(AstClass* ثابت classp) {
        اگر (classp->user2p()) بازگشت VN_AS(classp->user2p(), Var);
        اگر (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            بازگشت getCreateRandModeVar(extendsp->classp());
        }
        AstVar* ثابت randModeVarp = createModeVar(classp, "__Vrandmode");
        classp->user2p(randModeVarp);
        بازگشت randModeVarp;
    }
    AstVar* getCreateConstraintModeVar(AstClass* ثابت classp) {
        اگر (classp->user4p()) بازگشت VN_AS(classp->user4p(), Var);
        اگر (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            بازگشت getCreateConstraintModeVar(extendsp->classp());
        }
        AstVar* ثابت constraintModeVarp = createModeVar(classp, "__Vconstraintmode");
        classp->user4p(constraintModeVarp);
        بازگشت constraintModeVarp;
    }
    ایستا AstVar* getConstraintModeVar(AstClass* ثابت classp) {
        اگر (classp->user4p()) بازگشت VN_AS(classp->user4p(), Var);
        اگر (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            بازگشت getConstraintModeVar(extendsp->classp());
        }
        بازگشت nullptr;
    }
    AstVar* getCreateStaticConstraintModeVar(AstClass* ثابت classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        اگر (it != m_staticConstraintModeVars.end()) بازگشت it->second;
        اگر (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            بازگشت getCreateStaticConstraintModeVar(extendsp->classp());
        }
        AstVar* ثابت staticModeVarp = createStaticModeVar(classp, "__Vstaticconstraintmode");
        m_staticConstraintModeVars[classp] = staticModeVarp;
        بازگشت staticModeVarp;
    }
    AstVar* getStaticConstraintModeVar(AstClass* ثابت classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        اگر (it != m_staticConstraintModeVars.end()) بازگشت it->second;
        اگر (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            بازگشت getStaticConstraintModeVar(extendsp->classp());
        }
        بازگشت nullptr;
    }
    AstVar* getCreateStaticRandModeVar(AstClass* ثابت classp) {
        اگر (m_staticRandModeVars.شمار(classp)) بازگشت m_staticRandModeVars[classp];
        اگر (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            بازگشت getCreateStaticRandModeVar(extendsp->classp());
        }
        AstVar* ثابت staticModeVarp = createStaticModeVar(classp, "__Vstaticrandmode");
        m_staticRandModeVars[classp] = staticModeVarp;
        بازگشت staticModeVarp;
    }
    AstVar* getStaticRandModeVar(AstClass* ثابت classp) {
        اگر (m_staticRandModeVars.شمار(classp)) بازگشت m_staticRandModeVars[classp];
        اگر (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            بازگشت getStaticRandModeVar(extendsp->classp());
        }
        بازگشت nullptr;
    }
    AstVar* createModeVar(AstClass* ثابت classp, ثابت نویسه* ثابت name) {
        File_سطر* ثابت fl = classp->fileline();
        اگر (!m_dynarrayDtp) {
            m_dynarrayDtp = جدید AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ثابت modeVarp = جدید AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        بازگشت modeVarp;
    }
    AstVar* createStaticModeVar(AstClass* ثابت classp, ثابت نویسه* ثابت name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_سطر* ثابت fl = classp->fileline();
        اگر (!m_dynarrayDtp) {
            m_dynarrayDtp = جدید AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        AstVar* ثابت modeVarp = جدید AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        modeVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.درج(classp, modeVarp);
        بازگشت modeVarp;
    }
    ایستا تهی addSetRandMode(Ast_گره_FTask* ثابت ftaskp, AstVar* ثابت genp,
                               AstVar* ثابت randModeVarp) {
        File_سطر* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت setRandModep = جدید AstCMethodHard{
            fl, جدید AstVarRef{fl, VN_AS(genp->user2p(), گره_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_SET_RANDMODE,
            جدید AstVarRef{fl, VN_AS(randModeVarp->user2p(), گره_Module), randModeVarp,
                          VAccess::READ}};
        setRandModep->dtypeSetVoid();
        ftaskp->addStmtsp(setRandModep->makeStmt());
    }
    ایستا تهی addSetStaticRandMode(Ast_گره_FTask* ثابت ftaskp, AstVar* ثابت genp,
                                     AstVar* ثابت staticRandModeVarp) {
        File_سطر* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت setp = جدید AstCMethodHard{
            fl, جدید AstVarRef{fl, VN_AS(genp->user2p(), گره_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_SET_STATIC_RANDMODE,
            جدید AstVarRef{fl, VN_AS(staticRandModeVarp->user2p(), گره_Module), staticRandModeVarp,
                          VAccess::READ}};
        setp->dtypeSetVoid();
        ftaskp->addStmtsp(setp->makeStmt());
    }
    تهی createRandomizeClassVars(AstNetlist* ثابت netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> ریشه_Static_Rand_Mode_شمار;
        netlistp->foreach([این, &ریشه_Static_Rand_Mode_شمار](AstClass* ثابت classp) {
            منطقی hasConstraints = نادرست;
            uint32_t rand_Mode_شمار = 0;
            uint32_t static_Rand_Mode_شمار = 0;
            uint32_t constraint_Mode_شمار = 0;
            uint32_t static_Constraint_Mode_شمار = 0;
            classp->foreachMember([&](AstClass*, Ast_گره* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                اگر (AstConstraint* ثابت constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = درست;
                    RandomizeMode constraintMode = {.asUQuad = memberp->user1()};
                    اگر (!constraintMode.usesMode) بازگشت;
                    اگر (constraintMode.index == 0) {
                        // Use separate index counters for static vs non-static constraints
                        اگر (constrp->isStatic()) {
                            constraintMode.index = static_Constraint_Mode_شمار++;
                        } وگرنه {
                            constraintMode.index = constraint_Mode_شمار++;
                        }
                        memberp->user1(constraintMode.asUQuad);
                    } وگرنه {
                        اگر (constrp->isStatic()) {
                            static_Constraint_Mode_شمار = constraintMode.index + 1;
                        } وگرنه {
                            constraint_Mode_شمار = constraintMode.index + 1;
                        }
                    }
                } وگرنه اگر (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
                    RandomizeMode randMode = {.asUQuad = memberp->user1()};
                    اگر (!randMode.usesMode) بازگشت;
                    ثابت منطقی isStaticVar = varp->lifetime().isStatic();
                    اگر (randMode.index == 0) {
                        اگر (isStaticVar) {
                            randMode.index = static_Rand_Mode_شمار++;
                        } وگرنه {
                            randMode.index = rand_Mode_شمار++;
                        }
                        memberp->user1(randMode.asUQuad);
                    } وگرنه {
                        اگر (isStaticVar) {
                            static_Rand_Mode_شمار = randMode.index + 1;
                        } وگرنه {
                            rand_Mode_شمار = randMode.index + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<تهی(AstClass*)> findSubObjRandModes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_گره* subMemberp) {
                        اگر (AstVar* ثابت subVarp = VN_CAST(subMemberp, Var)) {
                            ثابت RandomizeMode rm = {.asUQuad = subVarp->user1()};
                            اگر (!rm.usesMode) بازگشت;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            اگر (subVarp->lifetime().isStatic()) بازگشت;
                            ثابت uint32_t needed = rm.index + 1;
                            اگر (needed > rand_Mode_شمار) rand_Mode_شمار = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    اگر (!memberVarp->globalConstrained()) بازگشت;
                    ثابت Ast_گره_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                    اگر (!classRefp) بازگشت;
                    findSubObjRandModes(classRefp->classp());
                });
            }
            اگر (hasConstraints) createRandomGenerator(classp);
            اگر (rand_Mode_شمار > 0) {
                AstVar* ثابت randModeVarp = getCreateRandModeVar(classp);
                makeModeInit(randModeVarp, classp, rand_Mode_شمار);
            }
            اگر (constraint_Mode_شمار > 0) {
                AstVar* ثابت constraintModeVarp = getCreateConstraintModeVar(classp);
                makeModeInit(constraintModeVarp, classp, constraint_Mode_شمار);
            }
            اگر (static_Constraint_Mode_شمار > 0) {
                AstVar* ثابت staticConstraintModeVarp = getCreateStaticConstraintModeVar(classp);
                makeStaticModeInit(staticConstraintModeVarp, classp, static_Constraint_Mode_شمار);
            }
            اگر (static_Rand_Mode_شمار > 0) {
                getCreateStaticRandModeVar(classp);
                AstClass* rootp = classp;
                وقتی (AstClassExtends* ثابت ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& slot = ریشه_Static_Rand_Mode_شمار[rootp];
                اگر (static_Rand_Mode_شمار > slot) slot = static_Rand_Mode_شمار;
            }
        });
        برای (ثابت auto& kv : ریشه_Static_Rand_Mode_شمار) emit_ریشه_Static_Mode_Init(kv.first, kv.second);
    }
    تهی emit_ریشه_Static_Mode_Init(AstClass* ثابت rootp, ثابت uint32_t شمار) {
        AstVar* ثابت staticRandModeVarp = m_staticRandModeVars[rootp];
        UASSERT_OBJ(staticRandModeVarp, rootp, "Root must have a static rand-mode var");
        makeStaticModeInit(staticRandModeVarp, rootp, شمار);
    }
    تهی makeModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_شمار) {
        Ast_گره_Module* ثابت modeVarModp = VN_AS(modeVarp->user2p(), گره_Module);
        File_سطر* ثابت fl = modeVarp->fileline();
        AstVar* ثابت oldSizeVarp = جدید AstVar{
            fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__VoldSize"), modeVarp->findUInt32DType()};
        oldSizeVarp->funcLocal(درست);
        oldSizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت oldSizep
            = جدید AstCMethodHard{fl, جدید AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        oldSizep->dtypeSetUInt32();
        AstCMethodHard* ثابت dynarrayNewp
            = جدید AstCMethodHard{fl, جدید AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جدید AstConst{fl, mode_شمار}};
        dynarrayNewp->dtypeSetVoid();
        Ast_گره_FTask* ثابت ctorNewp = VN_AS(m_memberMap.findMember(classp, "new"), گره_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_گره* ثابت initFirstp = oldSizeVarp;
        initFirstp->addNext(
            جدید AstAssign{fl, جدید AstVarRef{fl, oldSizeVarp, VAccess::WRITE}, oldSizep});
        initFirstp->addNext(dynarrayNewp->makeStmt());
        initFirstp->addNext(makeModeSetLoop(
            fl, جدید AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE}, جدید AstConst{fl, 1},
            درست, جدید AstVarRef{fl, oldSizeVarp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        برای (Ast_گره* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (!VN_IS(stmtp, گره_Stmt)) ادامه;  // Skip var declarations
            اگر (ثابت Ast_Stmt_عبارت* ثابت sep = VN_CAST(stmtp, Stmt_عبارت)) {
                اگر (VN_IS(sep->exprp(), New)) ادامه;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->addHereThisAsNext(initFirstp);
            بازگشت;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->addStmtsp(initFirstp);
    }
    تهی makeStaticModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_شمار) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_گره_Module* ثابت modeVarModp = VN_AS(modeVarp->user2p(), گره_Module);
        File_سطر* ثابت fl = modeVarp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ثابت sizep
            = جدید AstCMethodHard{fl, جدید AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstEq* ثابت condp = جدید AstEq{fl, sizep, جدید AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ثابت dynarrayNewp
            = جدید AstCMethodHard{fl, جدید AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جدید AstConst{fl, mode_شمار}};
        dynarrayNewp->dtypeSetVoid();
        Ast_گره* ثابت thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->addNext(
            makeModeSetLoop(fl, جدید AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                            جدید AstConst{fl, 1}, درست));

        // Build the if statement
        AstIf* ثابت ifp = جدید AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_گره_FTask* ثابت newp = VN_AS(m_memberMap.findMember(classp, "new"), گره_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->addStmtsp(ifp);
    }
    Ast_گره* makeModeSetLoop(File_سطر* ثابت fl, Ast_گره_عبارت* ثابت lhsp, Ast_گره_عبارت* ثابت rhsp,
                             منطقی inTask, Ast_گره_عبارت* ثابت startp = nullptr) {
        AstVar* ثابت iterVarp = جدید AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->findUInt32DType()};
        iterVarp->funcLocal(inTask);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت sizep = جدید AstCMethodHard{fl, lhsp, VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstCMethodHard* ثابت setp
            = جدید AstCMethodHard{fl, lhsp->clone_درخت(نادرست), VCMethod::ARRAY_AT_WRITE,
                                 جدید AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtypeSetUInt32();
        Ast_گره* ثابت stmtsp = iterVarp;
        stmtsp->addNext(جدید AstAssign{fl, جدید AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : جدید AstConst{fl, 0}});

        AstLoop* ثابت loopp = جدید AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(جدید AstLoopTest{
            fl, loopp, جدید AstLt{fl, جدید AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->addStmtsp(جدید AstAssign{fl, setp, rhsp});
        loopp->addStmtsp(جدید AstAssign{
            fl, جدید AstVarRef{fl, iterVarp, VAccess::WRITE},
            جدید AstAdd{fl, جدید AstConst{fl, 1}, جدید AstVarRef{fl, iterVarp, VAccess::READ}}});
        بازگشت جدید AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtsp, درست};
    }
    تهی addTempVarsp(AstVar* varsp) {
        برای (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            اگر (m_ftaskp) {
                varp->funcLocal(درست);
            } وگرنه {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        اگر (!m_ftaskp) {
            m_modp->addStmtsp(varsp);
        } وگرنه {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_گره_Stmt* wrapIfRandMode(AstClass* classp, AstVar* ثابت varp, Ast_گره_Stmt* stmtp) {
        ثابت RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstVar* ثابت modeVarp = varp->lifetime().isStatic() ? getStaticRandModeVar(classp)
                                                             : getRandModeVarFromClass(classp);
        بازگشت VN_AS(wrapIfMode(rmode, modeVarp, stmtp), گره_Stmt);
    }
    Ast_گره* wrapIfConstraintMode(AstClass* classp, AstConstraint* ثابت constrp, Ast_گره* stmtp) {
        ثابت RandomizeMode rmode = {.asUQuad = constrp->user1()};
        AstVar* ثابت modeVarp = constrp->isStatic() ? getStaticConstraintModeVar(classp)
                                                     : getConstraintModeVar(classp);
        بازگشت wrapIfMode(rmode, modeVarp, stmtp);
    }
    ایستا Ast_گره* wrapIfMode(ثابت RandomizeMode mode, AstVar* modeVarp, Ast_گره* stmtp) {
        File_سطر* ثابت fl = stmtp->fileline();
        اگر (mode.usesMode) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_گره_Module* ثابت modp = VN_AS(modeVarp->user2p(), گره_Module);
            AstCMethodHard* ثابت atp
                = جدید AstCMethodHard{fl, جدید AstVarRef{fl, modp, modeVarp, VAccess::READ},
                                     VCMethod::ARRAY_AT, جدید AstConst{fl, mode.index}};
            atp->dtypeSetUInt32();
            بازگشت جدید AstIf{fl, atp, stmtp};
        }
        بازگشت stmtp;
    }
    AstVarRef* enum_مقدار_Tab_Refp(AstEnumDType* ثابت nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        اگر (nodep->user2p()) بازگشت VN_AS(nodep->user2p(), VarRef)->clone_درخت(نادرست);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_گره_Array_DType* ثابت vardtypep = جدید AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            جدید AstRange{nodep->fileline(), static_cast<صحیح>(nodep->item_شمار()) - 1, 0}};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        AstConsArray* ثابت consp = جدید AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t index = 0;
        برای (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(index++, vconstp->clone_درخت(نادرست));
        }
        // Share identical tables via the constant pool
        AstVarRef* ثابت refp = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_درخت(), consp);  // V3ConstPool::find clones it
        افزودن_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        بازگشت refp->clone_درخت(نادرست);
    }

    AstCDType* findVlRandCDType(File_سطر* ثابت fl, uint64_t اقلام) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ثابت std::string type = AstCDType::typeToHold(V3_عدد::log2bQuad(اقلام) + 1);
        ثابت std::string name = "VlRandC<" + type + ", " + cvtToStr(اقلام) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ثابت auto pair = m_randcDtypes.emplace(name, nullptr);
        اگر (pair.second) {
            AstCDType* ثابت newp = جدید AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            pair.first->second = newp;
        }
        بازگشت pair.first->second;
    }

    AstVar* newRandcVarsp(AstVar* ثابت varp, AstClass* ثابت classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        اگر (!varp->isRandC()) بازگشت nullptr;

        uint64_t اقلام = 0;
        اگر (AstEnumDType* ثابت enumDtp = VN_CAST(varp->dtypep()->skipRefToEnump(), EnumDType)) {
            اقلام = static_cast<uint64_t>(enumDtp->item_شمار());
        } وگرنه اگر (AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp()) {
            اگر (basicp->width() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                اگر (!V3Error::error_شمار()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->width() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                بازگشت nullptr;
            }
            اقلام = 1ULL << basicp->width();
        } وگرنه اگر (AstStructDType* ثابت dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            بازگشت nullptr;
        } وگرنه {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_شمار(), varp, "Unexpected randc variable dtype");
            بازگشت nullptr;
        }
        AstCDType* ثابت newdtp = findVlRandCDType(varp->fileline(), اقلام);
        AstVar* ثابت newp
            = جدید AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->isInternal(درست);
        classp->addStmtsp(newp);
        UINFO(9, "created " << varp);
        بازگشت newp;
    }
    Ast_گره_Stmt* createArrayForeachLoop(File_سطر* ثابت fl, Ast_گره_DType* ثابت dtypep,
                                        Ast_گره_عبارت* exprp, AstVar* ثابت outputVarp) {
        Ast_گره_DType* tempDTypep = dtypep;
        AstVar* randLoopIndxp = nullptr;
        auto createLoopIndex = [&](Ast_گره_DType* tempDTypep) {
            اگر (VN_IS(tempDTypep, AssocArrayDType)) {
                بازگشت جدید AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                                  VN_AS(tempDTypep, AssocArrayDType)->کلید_DTypep()};
            }
            بازگشت جدید AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                              dtypep->findBasicDType(VBasicDTypeKwd::UINT32)};
        };
        Ast_گره_عبارت* tempElementp = nullptr;
        وقتی (tempDTypep->isNonPackedArray()) {
            AstVar* ثابت newRandLoopIndxp = createLoopIndex(tempDTypep);
            randLoopIndxp = Ast_گره::addNext(randLoopIndxp, newRandLoopIndxp);
            Ast_گره_عبارت* ثابت tempExprp = tempElementp ? tempElementp : exprp;
            AstVarRef* ثابت tempRefp = جدید AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            اگر (VN_IS(tempDTypep, DynArrayDType)) {
                tempElementp
                    = جدید AstCMethodHard{fl, tempExprp, VCMethod::ARRAY_AT_WRITE, tempRefp};
            } وگرنه اگر (VN_IS(tempDTypep, UnpackArrayDType)) {
                Ast_گره_Array_DType* ثابت aryDTypep = VN_CAST(tempDTypep, گره_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                tempElementp = جدید AstArraySel{
                    fl, tempExprp,
                    جدید AstSel{
                        fl,
                        جدید AstSub{fl, tempRefp,
                                   جدید AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        جدید AstConst{fl, 0}, V3_عدد::log2b(aryDTypep->hi()) + 1}};
            } وگرنه اگر (VN_IS(tempDTypep, AssocArrayDType)) {
                tempElementp = جدید AstAssocSel{fl, tempExprp, tempRefp};
            } وگرنه اگر (VN_IS(tempDTypep, QueueDType)) {
                tempElementp
                    = جدید AstCMethodHard{fl, tempExprp, VCMethod::DYN_AT_WRITE_APPEND, tempRefp};
            }
            tempElementp->dtypep(tempDTypep->subDTypep());
            tempDTypep = tempDTypep->virtRefDTypep();
        }

        AstForeachHeader* ثابت headerp
            = جدید AstForeachHeader{fl, exprp->clone_درخت(نادرست), randLoopIndxp};
        Ast_گره_Stmt* ثابت randStmtsp = newRandStmtsp(fl, tempElementp, nullptr, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        اگر (!tempElementp->backp()) VL_DO_DANGLING(افزودن_Deletep(tempElementp), tempElementp);
        بازگشت جدید AstForeach{fl, headerp, randStmtsp};
    }
    Ast_گره_Stmt* newRandStmtsp(File_سطر* fl, Ast_گره_عبارت* exprp, AstVar* randcVarp,
                               AstVar* ثابت outputVarp, صحیح offset = 0,
                               AstMemberDType* memberp = nullptr) {
        Ast_گره_DType* ثابت memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        اگر (ثابت auto* ثابت structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_گره_Stmt* stmtsp = nullptr;
            اگر (structDtp->packed()) offset += memberp ? memberp->lsb() : 0;
            برای (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_گره_Stmt* randp = nullptr;
                اگر (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_درخت(نادرست) : exprp, nullptr,
                                          outputVarp, offset, smemberp);
                } وگرنه اگر (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = جدید AstStructSel{fl, exprp->clone_درخت(نادرست), smemberp->name()};
                    structSelp->dtypep(smemberp->childDTypep());
                    اگر (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, nullptr, outputVarp);
                }
                stmtsp = Ast_گره::addNextNull(stmtsp, randp);
            }
            بازگشت stmtsp;
        } وگرنه اگر (ثابت auto* ثابت unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            اگر (!unionDtp->packed()) بازگشت nullptr;
            AstMemberDType* ثابت firstMemberp = unionDtp->membersp();
            بازگشت newRandStmtsp(fl, exprp, nullptr, outputVarp, offset, firstMemberp);
        } وگرنه اگر (ثابت AstClassRefDType* ثابت classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ثابت memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            AstMethodCall* ثابت callp = جدید AstMethodCall{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            AstAssign* ثابت assignp = جدید AstAssign{
                fl, جدید AstVarRef{fl, outputVarp, VAccess::WRITE},
                جدید AstAnd{fl, جدید AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            بازگشت جدید AstIf{
                fl, جدید AstNeq{fl, exprp->clone_درخت(نادرست), جدید AstConst{fl, AstConst::Null{}}},
                assignp};
        } وگرنه اگر (AstDynArrayDType* ثابت dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            بازگشت createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } وگرنه اگر (AstQueueDType* ثابت queueDtp = VN_CAST(memberDtp, QueueDType)) {
            بازگشت createArrayForeachLoop(fl, queueDtp, exprp, outputVarp);
        } وگرنه اگر (AstUnpackArrayDType* ثابت unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            بازگشت createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } وگرنه اگر (AstAssocArrayDType* ثابت assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            بازگشت createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } وگرنه {
            Ast_گره_عبارت* valp;
            اگر (AstEnumDType* ثابت enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ثابت tabRefp = enum_مقدار_Tab_Refp(enumDtp);
                Ast_گره_عبارت* ثابت randp
                    = new_Rand_مقدار(fl, randcVarp, exprp->findBasicDType(VBasicDTypeKwd::UINT32));
                Ast_گره_عبارت* ثابت moddivp = جدید AstModDiv{
                    fl, randp, جدید AstConst{fl, static_cast<uint32_t>(enumDtp->item_شمار())}};
                moddivp->dtypep(enumDtp);
                valp = جدید AstArraySel{fl, tabRefp, moddivp};
            } وگرنه {
                valp
                    = new_Rand_مقدار(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            AstAssign* assignp
                = جدید AstAssign{fl,
                                جدید AstSel{fl, exprp, offset + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->width() : exprp->width()},
                                valp};
            AstVar* varp = nullptr;
            exprp->exists([&](ثابت AstVarRef* varrefp) {
                اگر (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                بازگشت varp != nullptr;
            });
            بازگشت wrapIfRandMode(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_گره_عبارت* new_Rand_مقدار(File_سطر* ثابت fl, AstVar* ثابت randcVarp,
                              Ast_گره_DType* ثابت dtypep) {
        اگر (randcVarp) {
            AstCExpr* ثابت cexprp = جدید AstCExpr{fl};
            cexprp->add(جدید AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->add(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            بازگشت cexprp;
        }

        بازگشت جدید AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* findPrePostTask(AstClass* classp, ثابت string& name) {
        برای (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            اگر (AstTask* ثابت taskp = VN_CAST(m_memberMap.findMember(cp, name), Task)) {
                بازگشت taskp;
            }
        }
        بازگشت nullptr;
    }
    تهی addPrePostCall(AstClass* ثابت classp, Ast_گره_FTask* ثابت funcp, ثابت string& name) {
        اگر (AstTask* ثابت userFuncp = findPrePostTask(classp, name)) {
            AstTaskRef* ثابت callp = جدید AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->addStmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* ثابت classp, ثابت string& which) {
        ثابت string name = "__V" + which;
        اگر (AstTask* ثابت existingp = VN_CAST(m_memberMap.findMember(classp, name), Task)) {
            بازگشت existingp;
        }
        AstTask* ثابت taskp = جدید AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(درست);
        taskp->isVirtual(classp->isExtended());
        classp->addMembersp(taskp);
        m_memberMap.درج(classp, taskp);
        addPrePostCall(classp, taskp, which);
        بازگشت taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<منطقی, منطقی> buildPrePostVirtualWrappers(AstClass* ثابت classp) {
        ثابت auto cachedIt = m_prePostWrap.find(classp);
        اگر (cachedIt != m_prePostWrap.end()) بازگشت cachedIt->second;
        std::vector<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            اگر (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.افزودن_back(subp);
        });
        منطقی hasPre = نادرست;
        منطقی hasPost = نادرست;
        برای (AstClass* ثابت cp : hierp) {
            اگر (findPrePostTask(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = درست;
            }
            اگر (findPrePostTask(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = درست;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        اگر (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        اگر (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ثابت std::pair<منطقی, منطقی> result{hasPre, hasPost};
        m_prePostWrap.emplace(classp, result);
        بازگشت result;
    }
    تهی addVirtualPrePostCall(AstFunc* ثابت randomizeFuncp, AstClass* ثابت classp,
                               ثابت string& which) {
        File_سطر* ثابت fl = classp->fileline();
        AstTask* ثابت wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ثابت refDTypep = جدید AstClassRefDType{fl, classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
        AstMethodCall* ثابت callp
            = جدید AstMethodCall{fl, جدید AstThisRef{fl, refDTypep}, wrapperp->name(), nullptr};
        callp->taskp(wrapperp);
        callp->dtypeSetVoid();
        randomizeFuncp->addStmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    منطقی classHasRandClassMembers(AstClass* classp) {
        بازگشت classp->existsMember([](ثابت AstClass*, ثابت AstVar* varp) {
            اگر (!varp->rand().isRandomizable()) بازگشت نادرست;
            ثابت Ast_گره_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            بازگشت VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    منطقی classOwnsGlobalConstraint(ثابت AstClass* classp) ثابت {
        بازگشت classp->existsMember([](ثابت AstClass*, ثابت AstConstraint* constrp) {
            منطقی owns = نادرست;
            constrp->foreach([&](ثابت AstMemberSel* memberSelp) {
                ثابت Ast_گره* ثابت rootp
                    = const_cast<AstMemberSel*>(memberSelp)->baseFromp(درست);
                اگر (ثابت AstVarRef* ثابت refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ثابت Ast_گره_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    وقتی (ثابت Ast_گره_DType* ثابت subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    اگر (VN_IS(dtypep, ClassRefDType)) owns = درست;
                }
            });
            بازگشت owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ثابت string& suffix) {
        ثابت string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.findMember(classp, name), Task);
        اگر (taskp) بازگشت taskp;
        taskp = جدید AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(درست);
        classp->addMembersp(taskp);
        m_memberMap.درج(classp, taskp);
        بازگشت taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    تهی populateNestedCallbackTask(AstTask* ثابت callbackTaskp, AstClass* ثابت classp,
                                    ثابت string& cbName) {
        File_سطر* ثابت fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            اگر (!memberVarp->rand().isRandomizable()) بازگشت;
            ثابت Ast_گره_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
            ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
            اگر (!classRefp) بازگشت;
            AstClass* ثابت memberClassp = classRefp->classp();
            اگر (memberClassp == classp) بازگشت;  // Avoid self-reference

            // Force-visit member class if not yet processed
            اگر (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_گره* stmtsp = nullptr;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            اگر (AstTask* ثابت userFuncp = findPrePostTask(memberClassp, cbName)) {
                AstMethodCall* ثابت callp = جدید AstMethodCall{
                    fl, جدید AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtypeSetVoid();
                stmtsp = Ast_گره::addNext(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            اگر (classHasRandClassMembers(memberClassp)) {
                ثابت string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ثابت nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                AstMethodCall* ثابت recurseCallp = جدید AstMethodCall{
                    fl, جدید AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtypeSetVoid();
                stmtsp = Ast_گره::addNext(stmtsp, recurseCallp->makeStmt());
            }

            اگر (!stmtsp) بازگشت;

            // Wrap in null check
            AstIf* ثابت nullCheckp = جدید AstIf{
                fl,
                جدید AstNeq{fl, جدید AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           جدید AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->addStmtsp(wrapIfRandMode(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* newSetupConstraintTask(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp = جدید AstTask{nodep->fileline(), name + "_setup_constraint", nullptr};
        taskp->classMethod(درست);
        nodep->addMembersp(taskp);
        بازگشت taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp
            = جدید AstTask{nodep->fileline(), name + "_resize_constrained_array", nullptr};
        taskp->classMethod(درست);
        nodep->addMembersp(taskp);
        بازگشت taskp;
    }
    Ast_گره_Stmt* implementConstraintsClear(File_سطر* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جدید AstCMethodHard{
            fileline,
            جدید AstVarRef{fileline, VN_AS(genp->user2p(), گره_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtypeSetVoid();
        بازگشت clearp->makeStmt();
    }
    Ast_گره_Stmt* implementConstraintsClearAll(File_سطر* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جدید AstCMethodHard{
            fileline,
            جدید AstVarRef{fileline, VN_AS(genp->user2p(), گره_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtypeSetVoid();
        بازگشت clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_گره_عبارت* ثابت exprp) {
        اگر (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            بازگشت memberSelp->varp();
        } وگرنه اگر (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            بازگشت varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        بازگشت nullptr;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_گره_عبارت* makeModeVarRef(Ast_گره_عبارت* ثابت exprp, AstVar* ثابت modeVarp,
                                ثابت VAccess access) {
        اگر (modeVarp->lifetime().isStatic()) {
            بازگشت جدید AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), گره_Module),
                                 modeVarp, access};
        }
        اگر (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ثابت newMemberSelp = جدید AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_درخت(نادرست), modeVarp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            بازگشت newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        بازگشت جدید AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), Class), modeVarp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* createOrGetSizeVar(AstClass* ثابت classp, AstVar* ثابت arrVarp, File_سطر* ثابت fl,
                               Ast_گره_DType* ثابت signed32DTypep, منطقی& wasCreated) {
        AstVar* sizeVarp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = نادرست;
        اگر (!sizeVarp) {
            sizeVarp = جدید AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->addMembersp(sizeVarp);
            m_memberMap.درج(classp, sizeVarp);
            sizeVarp->user2p(classp);
            arrVarp->user4p(sizeVarp);
            wasCreated = درست;
        }
        بازگشت sizeVarp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_عبارت* createSizeGteZeroConstraint(File_سطر* ثابت fl, AstVar* ثابت sizeVarp,
                                                   Ast_گره_عبارت* derefFromp = nullptr) {
        Ast_گره_عبارت* size_گره_Exprp = nullptr;
        اگر (derefFromp) {
            size_گره_Exprp = buildMemberSelExprp(derefFromp, sizeVarp);
        } وگرنه {
            size_گره_Exprp = جدید AstVarRef{fl, sizeVarp, VAccess::READ};
        }
        size_گره_Exprp->user1(درست);
        AstGteS* ثابت sizeGtep = جدید AstGteS{fl, size_گره_Exprp, جدید AstConst{fl, 0}};
        sizeGtep->user1(درست);
        بازگشت جدید Ast_Constraint_عبارت{fl, sizeGtep};
    }
    Ast_گره_عبارت* getFromp(Ast_گره_عبارت* ثابت exprp) {
        اگر (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            بازگشت memberSelp->fromp();
        } وگرنه اگر (AstMethodCall* ثابت methodCallp = VN_CAST(exprp, MethodCall)) {
            بازگشت methodCallp->fromp();
        }
        بازگشت nullptr;
    }
    AstVar* makeTmpRandModeVar(Ast_گره_عبارت* siblingExprp, AstVar* randModeVarp,
                               Ast_گره*& storeStmtspr, Ast_گره_Stmt*& restoreStmtspr) {
        File_سطر* ثابت fl = randModeVarp->fileline();
        AstVar* ثابت randModeTmpVarp = جدید AstVar{
            fl, VVarType::BLOCKTEMP, m_modeUniqueNames.get(randModeVarp), randModeVarp->dtypep()};
        randModeTmpVarp->funcLocal(m_ftaskp);
        randModeTmpVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_گره::addNext(
            storeStmtspr,
            جدید AstAssign{fl, جدید AstVarRef{fl, randModeTmpVarp, VAccess::WRITE},
                          makeModeVarRef(siblingExprp, randModeVarp, VAccess::READ)});
        storeStmtspr = Ast_گره::addNext(
            storeStmtspr,
            makeModeSetLoop(fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                            جدید AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_گره::addNext(
            restoreStmtspr,
            جدید AstAssign{fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                          جدید AstVarRef{fl, randModeTmpVarp, VAccess::READ}});
        بازگشت randModeTmpVarp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_گره_عبارت* sliceToCommonPrefix(Ast_گره_عبارت* thisp, Ast_گره_عبارت* otherp) {
        ایستا std::vector<Ast_گره_عبارت*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        وقتی (thisp) {
            thisHier.افزودن_back(thisp);
            thisp = getFromp(thisp);
        }
        وقتی (otherp) {
            otherHier.افزودن_back(otherp);
            otherp = getFromp(otherp);
        }
        Ast_گره_عبارت* commonp = nullptr;
        برای (auto thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            اگر ((*thisIt)->type() != (*otherIt)->type()) بشکن;
            اگر (AstMemberSel* ثابت memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ثابت otherMemberSelp = VN_AS(*otherIt, MemberSel);
                اگر (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    ادامه;
                }
            } وگرنه اگر (AstMethodCall* ثابت thisMethodCallp = VN_CAST(*thisIt, MethodCall)) {
                AstMethodCall* ثابت otherMethodCallp = VN_AS(*otherIt, MethodCall);
                اگر (thisMethodCallp->taskp() == otherMethodCallp->taskp()) {
                    commonp = thisMethodCallp;
                    ادامه;
                }
            } وگرنه اگر (AstVarRef* ثابت firstVarRefp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ثابت secondVarRefp = VN_AS(*otherIt, VarRef);
                اگر (firstVarRefp->varp() == secondVarRefp->varp()) {
                    commonp = firstVarRefp;
                    ادامه;
                }
            }
            بشکن;
        }
        بازگشت commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_گره_عبارت* buildMemberSelExprp(Ast_گره_عبارت* exprp, AstVar* deepestVarp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_گره_عبارت* resExprp = nullptr;
        اگر (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), nullptr);
            AstVar* varp = memberSelp->varp();
            اگر (deepestVarp) varp = deepestVarp;
            resExprp = جدید AstMemberSel{exprp->fileline(), resExprp, varp};
        } وگرنه اگر (ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            اگر (deepestVarp) varp = deepestVarp;
            resExprp = جدید AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), گره_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(درست);
        بازگشت resExprp;
    }

    تهی addUpdateRandVarsBody(AstClass* ثابت nodep, AstFunc* ثابت randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ثابت updatep
            = جدید AstFunc{nodep->fileline(), "__VupdateRandVars", nullptr, nullptr};
        updatep->classMethod(درست);
        updatep->isVirtual(درست);
        updatep->keepAlive(درست);
        updatep->needsSyms(نادرست);
        nodep->addMembersp(updatep);
        nodep->hasRandVarsUpdate(درست);

        برای (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            Ast_گره_FTask* ثابت newp = VN_AS(m_memberMap.findMember(classp, "new"), گره_FTask);
            برای (Ast_گره* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_عبارت* ثابت stmtExprp = VN_CAST(stmtp, Stmt_عبارت);
                اگر (!stmtExprp) ادامه;
                AstCMethodHard* ثابت methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                اگر (!methodp || methodp->method() != VCMethod::RANDOMIZER_WRITE_VAR) ادامه;

                Ast_گره_Var_Ref* ثابت varRefp = VN_AS(methodp->pinsp(), گره_Var_Ref);
                اگر (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    ادامه;
                }
                AstCMethodHard* ثابت updateMethodp = جدید AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_درخت_Pure(نادرست),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_درخت_Pure(نادرست)};
                updateMethodp->dtypeSetVoid();
                Ast_گره_عبارت* ثابت namep = VN_AS(varRefp->nextp()->nextp(), گره_عبارت);
                updateMethodp->addPinsp(namep->clone_درخت_Pure(نادرست));
                updateMethodp->method(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->addStmtsp(updateMethodp->makeStmt());
            }
        }
    }

    تهی addBasicRandomizeBody(AstFunc* ثابت basicRandomizep, AstClass* ثابت nodep,
                               AstVar* randModeVarp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_سطر* ثابت fl = nodep->fileline();
        AstVar* ثابت basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ثابت basicFvarRefp = جدید AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ثابت beginBasicValp = جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1};
        basicRandomizep->addStmtsp(جدید AstAssign{fl, basicFvarRefp, beginBasicValp});
        Ast_گره_FTask* ثابت newp = VN_AS(m_memberMap.findMember(nodep, "new"), گره_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            اگر (!memberVarp->rand().isRandomizable()) بازگشت;
            ثابت RandomizeMode randMode = {.asUQuad = memberVarp->user1()};
            اگر (randMode.usesMode
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ثابت setp = جدید AstCMethodHard{
                    nodep->fileline(),
                    جدید AstVarRef{fl, VN_AS(randModeVarp->user2p(), گره_Module), randModeVarp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, جدید AstConst{nodep->fileline(), randMode.index}};
                setp->dtypeSetUInt32();
                newp->addStmtsp(جدید AstAssign{fl, setp, جدید AstConst{fl, 0}});
            }
            اگر (memberVarp->user3()) بازگشت;  // Handled in constraints
            ثابت Ast_گره_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
            اگر (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                اگر (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    بازگشت;
                }
                AstFunc* ثابت memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                AstMethodCall* ثابت callp
                    = memberVarp->globalConstrained()
                          ? جدید AstMethodCall{fl,
                                              جدید AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : جدید AstMethodCall{
                                fl, جدید AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ثابت basicFvarRefReadp = basicFvarRefp->clone_درخت(نادرست);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ثابت assignIfNotNullp = جدید AstIf{
                    fl,
                    جدید AstNeq{fl, جدید AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               جدید AstConst{fl, AstConst::Null{}}},
                    جدید AstAssign{fl, basicFvarRefp->clone_درخت(نادرست),
                                  جدید AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->addStmtsp(wrapIfRandMode(nodep, memberVarp, assignIfNotNullp));
            } وگرنه {
                AstVar* ثابت randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ثابت refp
                    = جدید AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_گره_Stmt* ثابت stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                اگر (!refp->backp()) VL_DO_DANGLING(refp->delete_درخت(), refp);
                basicRandomizep->addStmtsp(
                    جدید AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtp, نادرست});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_گره_عبارت* makeModeAssignLhs(File_سطر* ثابت fl, AstClass* ثابت classp,
                                   Ast_گره_عبارت* ثابت fromp, AstVar* ثابت modeVarp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        اگر (modeVarp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            اگر (fromp) VL_DO_DANGLING(fromp->unlinkFrBack()->delete_درخت(), fromp);
            بازگشت جدید AstVarRef{fl, VN_AS(modeVarp->user2p(), گره_Module), modeVarp,
                                 VAccess::WRITE};
        } وگرنه اگر (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            بازگشت جدید AstVarRef{fl, VN_AS(modeVarp->user2p(), گره_Module), modeVarp,
                                 VAccess::WRITE};
        } وگرنه {
            AstMemberSel* ثابت memberselp = جدید AstMemberSel{fl, fromp->unlinkFrBack(), modeVarp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            بازگشت memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    تهی replaceWithModeAssign(Ast_گره_FTask_Ref* ثابت ftaskRefp, Ast_گره* ثابت receiverp,
                               Ast_گره_عبارت* ثابت lhsp) {
        replaceWithModeAssignAndAppend(ftaskRefp, receiverp, lhsp, nullptr);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    تهی replaceWithModeAssignAndAppend(Ast_گره_FTask_Ref* ثابت ftaskRefp, Ast_گره* ثابت receiverp,
                                        Ast_گره_عبارت* ثابت lhsp, Ast_گره* ثابت appendStmtp) {
        File_سطر* ثابت fl = ftaskRefp->fileline();
        اگر (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_عبارت), ftaskRefp, "Should be a statement");
            Ast_گره_عبارت* ثابت rhsp = ftaskRefp->argsp()->exprp()->unlinkFrBack();
            Ast_گره* newStmtp = nullptr;
            اگر (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                // mode
                ثابت RandomizeMode rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ثابت setp = جدید AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                جدید AstConst{fl, rmode.index}};
                setp->dtypeSetUInt32();
                newStmtp = جدید AstAssign{fl, setp, rhsp};
            } وگرنه {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = makeModeSetLoop(fl, lhsp, rhsp, m_ftaskp);
            }
            اگر (appendStmtp) newStmtp->addNext(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(افزودن_Deletep(m_stmtp), m_stmtp = nullptr);
        } وگرنه {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ثابت RandomizeMode rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ثابت setp = جدید AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            جدید AstConst{fl, rmode.index}};
            setp->dtypeSetUInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(افزودن_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    تهی wrapRandomizeCallWithNullGuard(Ast_گره_FTask_Ref* nodep) {
        AstMethodCall* ثابت callp = VN_CAST(nodep, MethodCall);
        اگر (!callp) بازگشت;
        اگر (callp->user4SetOnce()) بازگشت;
        File_سطر* ثابت fl = callp->fileline();
        Ast_گره_عبارت* ثابت checkp
            = جدید AstNeq{fl, callp->fromp()->clone_درخت(نادرست), جدید AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlinkFrBack(&relinker);
        relinker.relink(جدید AstCond{fl, checkp, callp, جدید AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    تهی handleRandomizeArgs(Ast_گره_FTask_Ref* ثابت nodep) {
        اگر (!nodep->argsp()) بازگشت;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        منطقی hasNullArg = نادرست;
        برای (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            اگر (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->رقم().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = درست;
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        برای (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            برای (AstArg *otherArgp = nextp, *otherNextp = nullptr; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                اگر (Ast_گره_عبارت* ثابت prefixp
                    = sliceToCommonPrefix(argp->exprp(), otherArgp->exprp())) {
                    اگر (prefixp == argp->exprp()) {
                        اگر (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlinkFrBack()->delete_درخت(), otherArgp);
                        ادامه;
                    }
                }
                اگر (Ast_گره_عبارت* ثابت prefixp
                    = sliceToCommonPrefix(otherArgp->exprp(), argp->exprp())) {
                    اگر (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);
                        بشکن;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::set<AstVar*> savedRandModeVarps;
        AstVar* tmpVarps = nullptr;
        Ast_گره* storeStmtsp = nullptr;
        Ast_گره* setStmtsp = nullptr;
        Ast_گره_Stmt* restoreStmtsp = nullptr;
        برای (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_گره_عبارت* exprp = argp->exprp();
            Ast_گره_عبارت* ثابت commonPrefixp = sliceToCommonPrefix(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_سطر* ثابت fl = argp->fileline();
            وقتی (exprp) {
                اگر (commonPrefixp == exprp) بشکن;
                AstVar* ثابت randVarp = getVarFromRef(exprp);
                AstClass* ثابت classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ثابت randModeVarp = randVarp->lifetime().isStatic()
                                                 ? getStaticRandModeVar(classp)
                                                 : getRandModeVarFromClass(classp);
                UASSERT_OBJ(randModeVarp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                اگر (savedRandModeVarps.find(randModeVarp) == savedRandModeVarps.end()) {
                    AstVar* ثابت randModeTmpVarp
                        = makeTmpRandModeVar(exprp, randModeVarp, storeStmtsp, restoreStmtsp);
                    savedRandModeVarps.درج(randModeVarp);
                    tmpVarps = Ast_گره::addNext(tmpVarps, randModeTmpVarp);
                }
                ثابت RandomizeMode randMode = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = جدید AstCMethodHard{
                    fl, makeModeVarRef(exprp, randModeVarp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, جدید AstConst{fl, randMode.index}};
                setp->dtypeSetUInt32();
                setStmtsp
                    = Ast_گره::addNext(setStmtsp, جدید AstAssign{fl, setp, جدید AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlinkFrBack()->delete_درخت(), argp);
        }
        اگر (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* targetClassp = nullptr;
            AstMethodCall* ثابت methodCallp = VN_CAST(nodep, MethodCall);
            اگر (methodCallp) {
                ثابت Ast_گره_DType* ثابت fromDTypep = methodCallp->fromp()->dtypep();
                ثابت AstClassRefDType* ثابت crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                targetClassp = crdtp->classp();
            } وگرنه {
                targetClassp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(targetClassp, nodep, "randomize(null) target class unresolved");
            AstFunc* ثابت checkOnlyFuncp = getCreateRandomizeNullFunc(targetClassp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        اگر (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && setStmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlinkFrBack(&relinker);
            Ast_گره* ثابت stmtsp = storeStmtsp;
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
    AstFunc* getCreateRandomizeNullFunc(AstClass* ثابت classp) {
        ایستا constexpr ثابت نویسه* ثابت name = "__Vrandomize_null";
        اگر (AstFunc* ثابت existingp = VN_AS(m_memberMap.findMember(classp, name), Func)) {
            بازگشت existingp;
        }
        File_سطر* ثابت fl = classp->fileline();
        AstFunc* ثابت funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ثابت fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        addPrePostCall(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ثابت classGenp = getRandomGenerator(classp);
        اگر (!classGenp) {
            funcp->addStmtsp(جدید AstAssign{fl, جدید AstVarRef{fl, fvarp, VAccess::WRITE},
                                           جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1}});
        } وگرنه {
            Ast_گره_Module* ثابت genModp = VN_AS(classGenp->user2p(), گره_Module);
            funcp->addStmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ثابت setupAllTaskp = getCreateConstraintSetupFunc(classp, m_memberMap);
            funcp->addStmtsp((جدید AstTaskRef{fl, setupAllTaskp})->makeStmt());
            AstCExpr* ثابت solverCallp = جدید AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(جدید AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->add(".next_check_only(__Vm_rng)");
            funcp->addStmtsp(
                جدید AstAssign{fl, جدید AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(درست);
        }

        // 3. post_randomize -- only if result is non-zero
        اگر (AstTask* ثابت userPostp = findPrePostTask(classp, "post_randomize")) {
            AstTaskRef* ثابت callp = جدید AstTaskRef{userPostp->fileline(), userPostp};
            funcp->addStmtsp(
                جدید AstIf{fl, جدید AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        بازگشت funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* liftLogIfChainToConstraintIf(AstLogIf* logIfp) {
        File_سطر* ثابت fl = logIfp->fileline();
        Ast_گره_عبارت* ثابت condp = logIfp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = logIfp->rhsp()->unlinkFrBack();
        Ast_گره* thenBodyp;
        اگر (AstLogIf* ثابت innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = liftLogIfChainToConstraintIf(innerLogIfp);
            VL_DO_DANGLING(افزودن_Deletep(innerLogIfp), innerLogIfp);
        } وگرنه {
            thenBodyp = جدید Ast_Constraint_عبارت{fl, rhsp};
        }
        بازگشت جدید AstConstraintIf{fl, condp, thenBodyp, nullptr};
    }

    ایستا منطقی distBoundRefsRandVar(ثابت Ast_گره* boundp) {
        بازگشت boundp->exists(
            [](ثابت AstVarRef* vrefp) { بازگشت vrefp->varp()->rand().isRandomizable(); });
    }

    ایستا منطقی distBoundRefsModeVar(ثابت Ast_گره* boundp) {
        بازگشت boundp->exists([](ثابت AstVarRef* vrefp) {
            اگر (!vrefp->varp()->rand().isRandomizable()) بازگشت نادرست;
            ثابت RandomizeMode rmode = {.asUQuad = vrefp->varp()->user1()};
            بازگشت rmode.usesMode;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ایستا Ast_گره_عبارت* newDistRangeMembership(AstDist* distp, ثابت AstInsideRange* irp) {
        File_سطر* ثابت fl = distp->fileline();
        ثابت منطقی isSigned = distp->exprp()->isSigned();
        Ast_گره_عبارت* ثابت dist_عبارت_Gtep = distp->exprp()->clone_درخت_Pure(نادرست);
        Ast_گره_عبارت* ثابت dist_عبارت_Ltep = distp->exprp()->clone_درخت_Pure(نادرست);
        dist_عبارت_Gtep->user1(درست);
        dist_عبارت_Ltep->user1(درست);
        Ast_گره_عبارت* ثابت gep
            = isSigned ? static_cast<Ast_گره_عبارت*>(
                             جدید AstGteS{fl, dist_عبارت_Gtep, irp->lhsp()->clone_درخت_Pure(نادرست)})
                       : static_cast<Ast_گره_عبارت*>(
                             جدید AstGte{fl, dist_عبارت_Gtep, irp->lhsp()->clone_درخت_Pure(نادرست)});
        Ast_گره_عبارت* ثابت lep
            = isSigned ? static_cast<Ast_گره_عبارت*>(
                             جدید AstLteS{fl, dist_عبارت_Ltep, irp->rhsp()->clone_درخت_Pure(نادرست)})
                       : static_cast<Ast_گره_عبارت*>(
                             جدید AstLte{fl, dist_عبارت_Ltep, irp->rhsp()->clone_درخت_Pure(نادرست)});
        gep->user1(درست);
        lep->user1(درست);
        Ast_گره_عبارت* ثابت andp = جدید AstLogAnd{fl, gep, lep};
        andp->user1(درست);
        بازگشت andp;
    }

    ساختار DistBucket final {
        Ast_گره_عبارت* rangep;  // A single value or an InsideRange
        Ast_گره_عبارت* weightExprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::vector<DistBucket> collectDistBuckets(AstDist* ثابت distp) {
        File_سطر* ثابت fl = distp->fileline();
        std::vector<DistBucket> buckets;
        برای (AstDistItem* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), DistItem)) {
            اگر (ثابت AstConst* ثابت constp = VN_CAST(ditemp->weightp(), Const)) {
                اگر (constp->toUQuad() == 0) ادامه;
            }
            Ast_گره_عبارت* weightExprp
                = جدید AstExtend{fl, ditemp->weightp()->clone_درخت_Pure(نادرست), 64};
            // := on a range weights every element, so scale by the range size.
            اگر (!ditemp->isWhole()) {
                اگر (ثابت AstInsideRange* ثابت irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ثابت AstConst* ثابت lop = VN_CAST(irp->lhsp(), Const);
                    ثابت AstConst* ثابت hip = VN_CAST(irp->rhsp(), Const);
                    Ast_گره_عبارت* rangeSizep;
                    اگر (lop && hip) {
                        ثابت uint64_t rangeSize = hip->toUQuad() - lop->toUQuad() + 1;
                        rangeSizep = جدید AstConst{fl, AstConst::Unsized64{}, rangeSize};
                    } وگرنه {
                        rangeSizep = جدید AstAdd{
                            fl, جدید AstConst{fl, AstConst::Unsized64{}, 1},
                            جدید AstSub{fl,
                                       جدید AstExtend{fl, irp->rhsp()->clone_درخت_Pure(نادرست), 64},
                                       جدید AstExtend{fl, irp->lhsp()->clone_درخت_Pure(نادرست), 64}}};
                    }
                    weightExprp = جدید AstMul{fl, weightExprp, rangeSizep};
                }
            }
            buckets.افزودن_back({ditemp->rangep(), weightExprp});
        }
        بازگشت buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ایستا Ast_گره_عبارت* newDistMembershipTerm(AstDist* distp, Ast_گره_عبارت* rangep) {
        اگر (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            بازگشت newDistRangeMembership(distp, irp);
        }
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* ثابت eqExprp = distp->exprp()->clone_درخت_Pure(نادرست);
        eqExprp->user1(درست);
        Ast_گره_عبارت* ثابت eqp = جدید AstEq{fl, eqExprp, rangep->clone_درخت_Pure(نادرست)};
        eqp->user1(درست);
        بازگشت eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_عبارت* buildDistMembership(AstDist* distp,
                                           ثابت std::vector<DistBucket>& buckets) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* unionExprp = nullptr;
        برای (ثابت auto& bucket : buckets) {
            Ast_گره_عبارت* ثابت memberp = newDistMembershipTerm(distp, bucket.rangep);
            اگر (!unionExprp) {
                unionExprp = memberp;
            } وگرنه {
                unionExprp = جدید AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(درست);
            }
        }
        بازگشت جدید Ast_Constraint_عبارت{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_گره_عبارت* newGateJoin(File_سطر* fl, Ast_گره_عبارت* ap, Ast_گره_عبارت* bp, منطقی isAnd) {
        اگر (ap->same_درخت(bp)) {
            VL_DO_DANGLING(افزودن_Deletep(bp), bp);
            بازگشت ap;
        }
        اگر (VN_IS(bp, Const)) std::swap(ap, bp);
        اگر (VN_IS(ap, Const)) {
            ثابت منطقی constWins = VN_AS(ap, Const)->isZero() == isAnd;
            اگر (!constWins) std::swap(ap, bp);
            VL_DO_DANGLING(افزودن_Deletep(bp), bp);
            بازگشت ap;
        }
        اگر (isAnd) بازگشت جدید AstLogAnd{fl, ap, bp};
        بازگشت جدید AstLogOr{fl, ap, bp};
    }
    Ast_گره_عبارت* newGateOr(File_سطر* fl, Ast_گره_عبارت* ap, Ast_گره_عبارت* bp) {
        بازگشت newGateJoin(fl, ap, bp, نادرست);
    }
    Ast_گره_عبارت* newGateAnd(File_سطر* fl, Ast_گره_عبارت* ap, Ast_گره_عبارت* bp) {
        بازگشت newGateJoin(fl, ap, bp, درست);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_گره_عبارت* newModeBitRead(AstVar* varp, ثابت AstMemberSel* mselp, AstVar* randModeVarp,
                                File_سطر* fl) {
        Ast_گره_عبارت* arrayp = nullptr;
        اگر (varp->lifetime().isStatic()) {
            AstClass* ثابت ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ثابت smodep = getStaticRandModeVar(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = جدید AstVarRef{fl, VN_AS(smodep->user2p(), گره_Module), smodep, VAccess::READ};
        } وگرنه اگر (mselp) {
            AstVar* ثابت memberModep = getRandModeVarFromClass(VN_AS(varp->user2p(), گره_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = جدید AstMemberSel{fl, mselp->fromp()->clone_درخت_Pure(نادرست), memberModep};
        } وگرنه {
            UASSERT_OBJ(randModeVarp, varp, "rand_mode variable without a class mode array");
            arrayp = جدید AstVarRef{fl, VN_AS(randModeVarp->user2p(), گره_Module), randModeVarp,
                                   VAccess::READ};
        }
        ثابت RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ثابت atp
            = جدید AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, جدید AstConst{fl, rmode.index}};
        atp->dtypeSetUInt32();
        بازگشت atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_گره_عبارت* newVarGate(AstVar* varp, ثابت AstMemberSel* mselp, منطقی ownerLevel,
                            AstVar* randModeVarp, File_سطر* fl) {
        اگر (!varp->rand().isRandomizable()) {
            بازگشت جدید AstConst{fl, AstConst::BitTrue{}, ownerLevel};
        }
        ثابت RandomizeMode rmode = {.asUQuad = varp->user1()};
        اگر (!rmode.usesMode) بازگشت جدید AstConst{fl, AstConst::BitTrue{}};
        بازگشت newModeBitRead(varp, mselp, randModeVarp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_گره_عبارت* newAccessGate(ثابت Ast_گره* nodep, منطقی ownerLevel, AstVar* randModeVarp,
                               File_سطر* fl) {
        اگر (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel)) {
            بازگشت newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, randModeVarp, fl),
                              newAccessGate(mselp->fromp(), درست, randModeVarp, fl));
        }
        اگر (ثابت Ast_گره_Var_Ref* ثابت refp = VN_CAST(nodep, گره_Var_Ref)) {
            بازگشت newVarGate(refp->varp(), nullptr, ownerLevel, randModeVarp, fl);
        }
        اگر (ثابت Ast_گره_Sel* ثابت selp = VN_CAST(nodep, گره_Sel)) {
            بازگشت newAccessGate(selp->fromp(), درست, randModeVarp, fl);
        }
        اگر (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        بازگشت جدید AstConst{fl, AstConst::BitTrue{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_گره_عبارت* newDistGate(ثابت Ast_گره* nodep, AstVar* randModeVarp, File_سطر* fl) {
        اگر (VN_IS(nodep, گره_Var_Ref) || VN_IS(nodep, MemberSel)) {
            بازگشت newAccessGate(nodep, نادرست, randModeVarp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        اگر (ثابت Ast_گره_Sel* ثابت selp = VN_CAST(nodep, گره_Sel)) {
            بازگشت newDistGate(selp->fromp(), randModeVarp, fl);
        }
        اگر (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            بازگشت newDistGate(selp->fromp(), randModeVarp, fl);
        }
        اگر (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            بازگشت newDistGate(selp->fromp(), randModeVarp, fl);
        }
        اگر (ثابت AstCMethodHard* ثابت methodp = VN_CAST(nodep, CMethodHard)) {
            اگر (methodp->method() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                بازگشت newDistGate(methodp->fromp(), randModeVarp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        اگر (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            Ast_گره_عبارت* ثابت thenGatep = newDistGate(condp->thenp(), randModeVarp, fl);
            Ast_گره_عبارت* ثابت elseGatep = newDistGate(condp->elsep(), randModeVarp, fl);
            Ast_گره_عبارت* armGatep = thenGatep;
            اگر (thenGatep->same_درخت(elseGatep)) {
                VL_DO_DANGLING(افزودن_Deletep(elseGatep), elseGatep);
            } وگرنه {
                armGatep
                    = جدید AstCond{fl, condp->condp()->clone_درخت_Pure(نادرست), thenGatep, elseGatep};
            }
            بازگشت newGateOr(fl, newDistGate(condp->condp(), randModeVarp, fl), armGatep);
        }
        اگر (ثابت Ast_گره_Uniop* ثابت uopp = VN_CAST(nodep, گره_Uniop)) {
            بازگشت newDistGate(uopp->lhsp(), randModeVarp, fl);
        }
        اگر (ثابت Ast_گره_Biop* ثابت bopp = VN_CAST(nodep, گره_Biop)) {
            بازگشت newGateOr(fl, newDistGate(bopp->lhsp(), randModeVarp, fl),
                             newDistGate(bopp->rhsp(), randModeVarp, fl));
        }
        اگر (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        بازگشت جدید AstConst{fl, AstConst::BitTrue{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_عبارت* buildFrozenMembership(AstDist* distp,
                                             ثابت std::vector<DistBucket>& buckets) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* unionExprp = nullptr;
        برای (ثابت auto& bucket : buckets) {
            Ast_گره_عبارت* termp = newDistMembershipTerm(distp, bucket.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ثابت منطقی runtimeWeight
                = bucket.weightExprp->exists([](ثابت Ast_گره_Var_Ref*) { بازگشت درست; });
            اگر (runtimeWeight) {
                AstNeq* ثابت nzp = جدید AstNeq{fl, bucket.weightExprp->clone_درخت_Pure(نادرست),
                                               جدید AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(درست);
                termp = جدید AstLogAnd{fl, termp, nzp};
                termp->user1(درست);
            }
            اگر (unionExprp) {
                unionExprp = جدید AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(درست);
            } وگرنه {
                unionExprp = termp;
            }
        }
        بازگشت جدید Ast_Constraint_عبارت{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_گره* gateFrozenDist(Ast_گره* chainp, AstDist* distp,
                            ثابت std::vector<DistBucket>& buckets, AstVar* randModeVarp) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* gatep = newDistGate(distp->exprp(), randModeVarp, fl);
        اگر (VN_IS(gatep, Const)) {
            ثابت منطقی drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(افزودن_Deletep(gatep), gatep);
            اگر (drawable) بازگشت chainp;
            VL_DO_DANGLING(افزودن_Deletep(chainp), chainp);
            بازگشت buildFrozenMembership(distp, buckets);
        }
        بازگشت جدید AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_گره_عبارت* newUniformRangePick(AstDist* distp, ثابت AstInsideRange* irp) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* ثابت dist_عبارت_Copyp = distp->exprp()->clone_درخت_Pure(نادرست);
        dist_عبارت_Copyp->user1(درست);
        ثابت صحیح distWidth = distp->exprp()->width();
        ثابت AstConst* ثابت lopC = VN_CAST(irp->lhsp(), Const);
        ثابت AstConst* ثابت hipC = VN_CAST(irp->rhsp(), Const);
        Ast_گره_عبارت* rangeSzp;
        اگر (lopC && hipC) {
            ثابت uint64_t rsz = hipC->toUQuad() - lopC->toUQuad() + 1;
            rangeSzp = جدید AstConst{fl, AstConst::Unsized64{}, rsz};
        } وگرنه {
            ثابت منطقی isSigned = irp->lhsp()->isSigned();
            Ast_گره_عبارت* ثابت lo64p
                = isSigned ? static_cast<Ast_گره_عبارت*>(
                                 جدید AstExtendS{fl, irp->lhsp()->clone_درخت_Pure(نادرست), 64})
                           : static_cast<Ast_گره_عبارت*>(
                                 جدید AstExtend{fl, irp->lhsp()->clone_درخت_Pure(نادرست), 64});
            Ast_گره_عبارت* ثابت hi64p
                = isSigned ? static_cast<Ast_گره_عبارت*>(
                                 جدید AstExtendS{fl, irp->rhsp()->clone_درخت_Pure(نادرست), 64})
                           : static_cast<Ast_گره_عبارت*>(
                                 جدید AstExtend{fl, irp->rhsp()->clone_درخت_Pure(نادرست), 64});
            rangeSzp = جدید AstAdd{fl, جدید AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  جدید AstSub{fl, hi64p, lo64p}};
        }
        Ast_گره_عبارت* ثابت rand64p = جدید AstRand{fl, nullptr, نادرست};
        rand64p->dtypeSetUInt64();
        Ast_گره_عبارت* ثابت offsetp
            = جدید AstCCast{fl, جدید AstModDiv{fl, rand64p, rangeSzp}, distWidth};
        Ast_گره_عبارت* ثابت valuep = جدید AstAdd{fl, irp->lhsp()->clone_درخت_Pure(نادرست), offsetp};
        Ast_گره_عبارت* ثابت eqp = جدید AstEq{fl, dist_عبارت_Copyp, valuep};
        eqp->user1(درست);
        بازگشت eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_گره* buildWeightedBucketChain(AstDist* distp, ثابت std::vector<DistBucket>& buckets,
                                      AstVar* bucketVarp,
                                      ثابت std::vector<Ast_گره_عبارت*>& cumSums) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* chainp = nullptr;
        برای (صحیح i = static_cast<صحیح>(buckets.size()) - 1; i >= 0; --i) {
            Ast_گره_عبارت* constraintExprp;
            ثابت AstInsideRange* ثابت irp = VN_CAST(buckets[i].rangep, InsideRange);
            اگر (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } وگرنه اگر (irp) {
                constraintExprp = newUniformRangePick(distp, irp);
            } وگرنه {
                Ast_گره_عبارت* ثابت dist_عبارت_Copyp = distp->exprp()->clone_درخت_Pure(نادرست);
                dist_عبارت_Copyp->user1(درست);
                constraintExprp
                    = جدید AstEq{fl, dist_عبارت_Copyp, buckets[i].rangep->clone_درخت_Pure(نادرست)};
                constraintExprp->user1(درست);
            }
            اگر (!chainp) {
                chainp = constraintExprp;
            } وگرنه {
                Ast_گره_عبارت* ثابت condp
                    = جدید AstLte{fl, جدید AstVarRef{fl, bucketVarp, VAccess::READ}, cumSums[i]};
                condp->user1(درست);
                chainp = جدید AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(درست);
            }
        }
        Ast_Constraint_عبارت* ثابت chainExprp = جدید Ast_Constraint_عبارت{fl, chainp};
        chainExprp->isSoft(درست);
        بازگشت chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    تهی lowerDistConstraints(AstTask* taskp, Ast_گره* constrItemsp, AstVar* randModeVarp,
                              AstConstraintForeach* foreachp = nullptr) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_گره* foreachTailp = nullptr;
        auto addStmt = [&](Ast_گره* nodep) {
            اگر (foreachp) {
                اگر (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } وگرنه {
                    foreachTailp->addNext(nodep);
                    foreachTailp = nodep;
                }
            } وگرنه {
                taskp->addStmtsp(nodep);
            }
        };

        برای (Ast_گره *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            اگر (AstConstraintIf* ثابت cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                lowerDistConstraints(taskp, cifp->thensp(), randModeVarp, foreachp);
                اگر (cifp->elsesp()) {
                    lowerDistConstraints(taskp, cifp->elsesp(), randModeVarp, foreachp);
                }
                ادامه;
            }
            اگر (AstConstraintForeach* ثابت cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                lowerDistConstraints(taskp, cfep->bodyp(), randModeVarp, cfep);
                ادامه;
            }

            Ast_Constraint_عبارت* ثابت constrExprp = VN_CAST(itemp, Constraint_عبارت);
            اگر (!constrExprp) ادامه;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            اگر (AstLogIf* ثابت بالاترین_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_گره* chainEndp = بالاترین_Log_Ifp->rhsp();
                وقتی (AstLogIf* ثابت innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                اگر (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ثابت liftedp = liftLogIfChainToConstraintIf(بالاترین_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(افزودن_Deletep(constrExprp), constrExprp);
                    lowerDistConstraints(taskp, liftedp->thensp(), randModeVarp, foreachp);
                    ادامه;
                }
            }

            AstDist* ثابت distp = VN_CAST(constrExprp->exprp(), Dist);
            اگر (!distp) ادامه;

            File_سطر* ثابت fl = distp->fileline();

            std::vector<DistBucket> buckets = collectDistBuckets(distp);

            اگر (buckets.empty()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_عبارت* ثابت truep
                    = جدید Ast_Constraint_عبارت{fl, جدید AstConst{fl, AstConst::BitTrue{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(افزودن_Deletep(constrExprp), constrExprp);
                ادامه;
            }

            Ast_Constraint_عبارت* ثابت membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_گره_عبارت* totalWeightExprp = nullptr;
            برای (auto& bucket : buckets) {
                اگر (!totalWeightExprp) {
                    totalWeightExprp = bucket.weightExprp->clone_درخت_Pure(نادرست);
                } وگرنه {
                    totalWeightExprp = جدید AstAdd{fl, totalWeightExprp,
                                                  bucket.weightExprp->clone_درخت_Pure(نادرست)};
                    totalWeightExprp->dtypeSetUInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ثابت صحیح distId = m_dist_رقم++;
            ثابت std::string totalName = "__Vdist_total" + cvtToStr(distId);
            AstVar* ثابت totalVarp
                = جدید AstVar{fl, VVarType::BLOCKTEMP, totalName, taskp->findUInt64DType()};
            totalVarp->noSubst(درست);
            totalVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            totalVarp->funcLocal(درست);
            totalVarp->isInternal(درست);
            addStmt(totalVarp);
            addStmt(
                جدید AstAssign{fl, جدید AstVarRef{fl, totalVarp, VAccess::WRITE}, totalWeightExprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ثابت std::string bucketName = "__Vdist_bucket" + cvtToStr(distId);
            AstVar* ثابت bucketVarp
                = جدید AstVar{fl, VVarType::BLOCKTEMP, bucketName, taskp->findUInt64DType()};
            bucketVarp->noSubst(درست);
            bucketVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            bucketVarp->funcLocal(درست);
            bucketVarp->isInternal(درست);
            addStmt(bucketVarp);

            Ast_گره_عبارت* ثابت randp = جدید AstRand{fl, nullptr, نادرست};
            randp->dtypeSetUInt64();
            addStmt(جدید AstAssign{
                fl, جدید AstVarRef{fl, bucketVarp, VAccess::WRITE},
                جدید AstAdd{
                    fl, جدید AstConst{fl, AstConst::Unsized64{}, 1},
                    جدید AstModDiv{fl, randp, جدید AstVarRef{fl, totalVarp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::vector<Ast_گره_عبارت*> cumSums;
            Ast_گره_عبارت* runningSump = nullptr;
            برای (size_t i = 0; i < buckets.size(); ++i) {
                اگر (!runningSump) {
                    runningSump = buckets[i].weightExprp->clone_درخت_Pure(نادرست);
                } وگرنه {
                    runningSump = جدید AstAdd{fl, runningSump,
                                             buckets[i].weightExprp->clone_درخت_Pure(نادرست)};
                    runningSump->dtypeSetUInt64();
                }
                cumSums.افزودن_back(runningSump->clone_درخت_Pure(درست));
            }

            Ast_گره* chainp = buildWeightedBucketChain(distp, buckets, bucketVarp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, randModeVarp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(افزودن_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->addHereThisAsNext(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            برای (auto& bucket : buckets) {
                VL_DO_DANGLING(افزودن_Deletep(bucket.weightExprp), bucket.weightExprp);
            }
            VL_DO_DANGLING(افزودن_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            افزودن_Deletep(cumSums.back());
        }
    }

    ایستا منطقی isDynArrOfClassTypeRecurse(ثابت Ast_گره_DType* ثابت dtypep) {
        ثابت Ast_گره_DType* ثابت refp = dtypep->skipRefp();
        اگر (VN_IS(refp, DynArrayDType) || VN_IS(refp, QueueDType)) {
            بازگشت isDynArrOfClassTypeRecurse(refp->subDTypep());
        } وگرنه اگر (VN_IS(refp, ClassRefDType)) {
            بازگشت درست;
        }
        بازگشت نادرست;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    تهی addArrayElemRefresh(File_سطر* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                             AstVar* ثابت genp) {
        Ast_گره_Module* ثابت genModp = VN_AS(genp->user2p(), گره_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(نادرست).second;
        اگر (isDynArrOfClassTypeRecurse(arrVarp->dtypep())) {
            اگر (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            بازگشت;
        }
        AstCMethodHard* ثابت methodp
            = جدید AstCMethodHard{fl, جدید AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();

        Ast_گره_Module* ثابت classp = VN_AS(arrVarp->user2p(), گره_Module);
        AstVarRef* ثابت varRefp = جدید AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->addPinsp(varRefp);

        ثابت size_t width = arrayElementDTypep(arrVarp->dtypep())->width();

        methodp->addPinsp(جدید AstConst{fl, AstConst::Unsized64{}, width});
        Ast_گره_عبارت* ثابت varnamep
            = جدید AstCExpr{fl, AstCExpr::Pure{}, "\"" + arrVarp->name() + "\"", arrVarp->width()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(جدید AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ثابت RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        اگر (rmode.usesMode) {
            methodp->addPinsp(جدید AstConst{fl, AstConst::Unsized64{}, rmode.index});
        }

        randomizep->addStmtsp(methodp->makeStmt());
    }

    Ast_گره_Stmt* newPinSizeVariable(File_سطر* ثابت fl, AstVar* ثابت arrVarp,
                                    AstVar* ثابت genp) {
        Ast_گره_Module* ثابت genModp = VN_AS(genp->user2p(), گره_Module);
        AstVar* ثابت sizeVarp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ثابت pinp
            = جدید AstCMethodHard{fl, جدید AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtypeSetVoid();
        AstCExpr* ثابت namep = جدید AstCExpr{fl, AstCExpr::Pure{}, "\"" + sizeVarp->name() + "\""};
        namep->dtypeSetUInt32();
        pinp->addPinsp(namep);
        pinp->addPinsp(
            جدید AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(sizeVarp->width())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ثابت sizeVarRefp = جدید AstVarRef{fl, sizeVarp, VAccess::READ};
        sizeVarRefp->classOrPackagep(VN_AS(sizeVarp->user2p(), گره_Module));
        pinp->addPinsp(sizeVarRefp);
        بازگشت pinp->makeStmt();
    }

    تهی addFrozenSizePin(File_سطر* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                          AstVar* ثابت genp, AstVar* ثابت randModeVarp) {
        ثابت RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        اگر (!rmode.usesMode) بازگشت;
        AstVar* ثابت sizeVarp = VN_AS(arrVarp->user4p(), Var);
        Ast_گره_Module* ثابت arrClassp = VN_AS(arrVarp->user2p(), گره_Module);
        Ast_گره_Module* ثابت sizeClassp = VN_AS(sizeVarp->user2p(), گره_Module);
        AstVarRef* ثابت sizeWritep = جدید AstVarRef{fl, sizeClassp, sizeVarp, VAccess::WRITE};
        ثابت VCMethod sizeMethod = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_SIZE
                                        : VCMethod::DYN_SIZE;
        AstCMethodHard* ثابت currentSizep = جدید AstCMethodHard{
            fl, جدید AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, sizeMethod};
        currentSizep->dtypep(sizeVarp->dtypep());
        randomizep->addStmtsp(جدید AstAssign{fl, sizeWritep, currentSizep});
        Ast_گره_عبارت* ثابت modep = newModeBitRead(arrVarp, nullptr, randModeVarp, fl);
        randomizep->addStmtsp(
            جدید AstIf{fl, جدید AstLogNot{fl, modep}, newPinSizeVariable(fl, arrVarp, genp)});
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_رقم);
        VL_RESTORER_COPY(m_randArrayUniqueNames);
        m_modp = nodep;
        m_rand_Case_رقم = 0;
        m_randArrayUniqueNames.reset();
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_رقم);
        VL_RESTORER(m_dist_رقم);
        m_modp = nodep;
        m_rand_Case_رقم = 0;
        m_dist_رقم = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        اگر (!nodep->user1()) بازگشت;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->baseMostClassp()->needRNG(درست);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ثابت منطقی basicFirst = classOwnsGlobalConstraint(nodep);

        File_سطر* ثابت fl = nodep->fileline();
        AstFunc* ثابت randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ثابت fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ثابت randModeVarp = getRandModeVarFromClass(nodep);
        addPrePostCall(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        اگر (classHasRandClassMembers(nodep)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->addStmtsp((جدید AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_گره_عبارت* beginValp = nullptr;
        AstVar* genp = getRandomGenerator(nodep);
        اگر (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ثابت prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                اگر (!taskp) {
                    taskp = newSetupConstraintTask(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                اگر (AstTask* ثابت resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ثابت resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ثابت resizeTaskRefp
                        = جدید AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->addStmtsp(resizeTaskRefp->makeStmt());
                }

                اگر (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                اگر (constrp->itemsp()) {
                    lowerDistConstraints(taskp, constrp->itemsp(), randModeVarp);
                }
                VInsertionSet<AstVar*>& sizeArrays = m_sizeConstrainedArrays[classp];
                Constraint_عبارت_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), nullptr,
                    genp,          randModeVarp, m_writtenVars,     &m_uniqueConstraintId,
                    m_uniqueNames, randomizep,   &sizeArrays,       prepareArraysTaskp};
                اگر (constrp->itemsp()) {
                    taskp->addStmtsp(wrapIfConstraintMode(
                        nodep, constrp, constrp->itemsp()->unlinkFrBackWithNext()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ثابت auto constraintDepth = [](ثابت AstConstraint* constrp) {
                    صحیح depth = 0;
                    size_t pos = 0;
                    وقتی ((pos = constrp->name().find(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++depth;
                        pos += std::strlen(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    بازگشت depth;
                };
                صحیح maxDepth = 0;
                nodep->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    maxDepth = std::max(maxDepth, constraintDepth(constrp));
                });
                AstTask* ثابت setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
                برای (صحیح d = maxDepth; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                            اگر (constraintDepth(constrp) != d) بازگشت;
                            AstTask* ثابت taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ثابت setupTaskRefp
                                = جدید AstTaskRef{constrp->fileline(), taskp};
                            setupTaskRefp->classOrPackagep(classp);
                            setupAllTaskp->addStmtsp(setupTaskRefp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertionSet<AstVar*> sizeArrayVars;
            اگر (nodep->extendsp()) {
                AstClass* parentClassp = nodep->extendsp()->classp();
                وقتی (parentClassp) {
                    ثابت auto sizeArraysIt = m_sizeConstrainedArrays.find(parentClassp);
                    اگر (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                        برای (AstVar* ثابت arrVarp : sizeArraysIt->second) {
                            sizeArrayVars.درج(arrVarp);
                        }
                    }
                    AstFunc* ثابت parentRandomizep
                        = VN_CAST(m_memberMap.findMember(parentClassp, "randomize"), Func);
                    اگر (parentRandomizep && parentRandomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        برای (Ast_گره* stmtp = parentRandomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            منطقی foundClearConstraints = نادرست;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                اگر (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->addStmtsp(stmtp->clone_درخت(نادرست));
                                } وگرنه اگر (methodp->method()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    foundClearConstraints = درست;
                                }
                            });
                            اگر (foundClearConstraints) بشکن;
                        }
                    }
                    parentClassp
                        = parentClassp->extendsp() ? parentClassp->extendsp()->classp() : nullptr;
                }
            }
            randomizep->addStmtsp(implementConstraintsClear(fl, genp));

            ثابت auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
            اگر (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                برای (AstVar* ثابت arrVarp : sizeArraysIt->second) sizeArrayVars.درج(arrVarp);
            }
            AstTask* ثابت setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
            اگر (!sizeArrayVars.empty() && prepareArraysTaskp->stmtsp()) {
                Ast_گره_Stmt* ثابت preparep = (جدید AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                برای (Ast_گره* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    منطقی writesArray = نادرست;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    اگر (!writesArray) ادامه;
                    stmtp->addHereThisAsNext(preparep);
                    بشکن;
                }
            }
            AstTaskRef* ثابت setupTaskRefp = جدید AstTaskRef{fl, setupAllTaskp};
            randomizep->addStmtsp(setupTaskRefp->makeStmt());

            Ast_گره_Module* ثابت genModp = VN_AS(genp->user2p(), گره_Module);

            AstCExpr* ثابت solverCallp = جدید AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(جدید AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->add(".next(__Vm_rng)");
            اگر (!sizeArrayVars.empty()) {
                AstVar* ثابت sizeOkVarp = جدید AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                sizeOkVarp->funcLocal(درست);
                randomizep->addStmtsp(sizeOkVarp);

                AstVar* ثابت finalOkVarp = جدید AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                finalOkVarp->funcLocal(درست);
                randomizep->addStmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                برای (AstVar* ثابت arrVarp : sizeArrayVars)
                    addFrozenSizePin(fl, arrVarp, randomizep, genp, randModeVarp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->addStmtsp(
                    جدید AstAssign{fl, جدید AstVarRef{fl, sizeOkVarp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                اگر (AstTask* ثابت resizeAllTaskp
                    = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ثابت resizeTaskRefp = جدید AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ثابت ifp = جدید AstIf{fl, جدید AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), nullptr};
                    randomizep->addStmtsp(ifp);
                }

                // Refresh array element tables after resize
                برای (ثابت auto& arrVarp : sizeArrayVars) {
                    addArrayElemRefresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->addStmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ثابت setupTaskRefp2 = جدید AstTaskRef{fl, setupAllTaskp};
                randomizep->addStmtsp(setupTaskRefp2->makeStmt());

                برای (AstVar* ثابت arrVarp : sizeArrayVars) {
                    randomizep->addStmtsp(newPinSizeVariable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ثابت solverCallp2 = جدید AstCExpr{fl};
                solverCallp2->dtypeSetBit();
                solverCallp2->add(جدید AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->add(".next(__Vm_rng)");
                randomizep->addStmtsp(جدید AstAssign{
                    fl, جدید AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = جدید AstAnd{fl, جدید AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                       جدید AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } وگرنه {
                beginValp = solverCallp;
            }
            اگر (randModeVarp) {
                Ast_گره_Module* ثابت randModeClassp = VN_AS(randModeVarp->user2p(), Class);
                Ast_گره_FTask* ثابت newp
                    = VN_AS(m_memberMap.findMember(randModeClassp, "new"), گره_FTask);
                UASSERT_OBJ(newp, randModeClassp, "No new() in class");
                addSetRandMode(newp, genp, randModeVarp);
            }
            اگر (AstVar* ثابت staticRandModeVarp = getStaticRandModeVar(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_گره_FTask* ثابت newp = VN_AS(m_memberMap.findMember(nodep, "new"), گره_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                addSetStaticRandMode(newp, genp, staticRandModeVarp);
            }
        } وگرنه {
            beginValp = جدید AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1};
        }

        addUpdateRandVarsBody(nodep, randomizep);

        AstFunc* ثابت basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        addBasicRandomizeBody(basicRandomizep, nodep, randModeVarp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ثابت fvarRefp = جدید AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->addStmtsp(جدید AstAssign{
            fl, fvarRefp, basicFirst ? جدید AstFuncRef{fl, basicRandomizep} : beginValp});

        ثابت auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
        ثابت منطقی needsSizePhase
            = sizeArraysIt != m_sizeConstrainedArrays.end() && !sizeArraysIt->second.empty();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ثابت auto emitResizeFallback = [&]() {
            اگر (needsSizePhase) بازگشت;
            اگر (AstTask* ثابت resizeAllTaskp
                = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ثابت resizeTaskRefp = جدید AstTaskRef{fl, resizeAllTaskp};
                randomizep->addStmtsp(resizeTaskRefp->makeStmt());
            }
        };
        اگر (!basicFirst) emitResizeFallback();

        AstVarRef* ثابت fvarRefReadp = fvarRefp->clone_درخت(نادرست);
        fvarRefReadp->access(VAccess::READ);

        Ast_گره_عبارت* ثابت secondHalfp
            = basicFirst ? beginValp : جدید AstFuncRef{fl, basicRandomizep};
        randomizep->addStmtsp(جدید AstAssign{fl, fvarRefp->clone_درخت(نادرست),
                                            جدید AstAnd{fl, fvarRefReadp, secondHalfp}});

        اگر (basicFirst) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        اگر (classHasRandClassMembers(nodep)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->addStmtsp((جدید AstTaskRef{fl, postTaskp})->makeStmt());
        }

        addPrePostCall(nodep, randomizep, "post_randomize");
        nodep->user1(نادرست);
    }
    تهی visit(AstRandCase* nodep) override {
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
        Ast_گره_DType* ثابت sumDTypep = nodep->findUInt64DType();

        File_سطر* ثابت fl = nodep->fileline();
        ثابت std::string name = "__Vrandcase" + cvtToStr(m_rand_Case_رقم++);
        AstVar* ثابت randVarp = جدید AstVar{fl, VVarType::BLOCKTEMP, name, sumDTypep};
        randVarp->noSubst(درست);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        addTempVarsp(randVarp);
        Ast_گره_عبارت* sump = جدید AstConst{fl, AstConst::Widthed_مقدار{}, 64, 0};
        Ast_گره_If* ثابت firstIfsp
            = جدید AstIf{fl, جدید AstConst{fl, AstConst::BitFalse{}}, nullptr, nullptr};
        Ast_گره_If* ifsp = firstIfsp;

        برای (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            Ast_گره_عبارت* ثابت condp = itemp->condsp()->unlinkFrBack();
            sump
                = جدید AstAdd{condp->fileline(), sump, جدید AstExtend{itemp->fileline(), condp, 64}};
            Ast_گره* ثابت stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlinkFrBackWithNext() : nullptr;
            AstVarRef* ثابت randVarRefp = جدید AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_گره_If* ثابت newifp
                = جدید AstIf{itemp->fileline(),
                            جدید AstLte{condp->fileline(), randVarRefp, sump->clone_درخت_Pure(درست)},
                            stmtsp, nullptr};
            ifsp->addElsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ثابت dispp = جدید AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            nullptr, nullptr};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->addElsesp(dispp);

        Ast_گره_عبارت* ثابت randp = جدید AstRand{fl, nullptr, نادرست};
        randp->dtypeSetUInt64();
        AstVarRef* ثابت randVarRefp = جدید AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_گره* ثابت newp
            = جدید AstAssign{fl, randVarRefp,
                            جدید AstAdd{fl, جدید AstConst{fl, AstConst::Unsized64{}, 1},
                                       جدید AstModDiv{fl, randp, sump}}};
        newp->addNext(firstIfsp);
        اگر (debug() >= 9) newp->dump_درخت_And_Next(cout, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        اگر (nodep->name() == "rand_mode") {
            AstMethodCall* ثابت methodCallp = VN_CAST(nodep, MethodCall);
            Ast_گره_عبارت* ثابت fromp = methodCallp ? methodCallp->fromp() : nullptr;
            ثابت RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_modp);
            UASSERT_OBJ(randModeTarget.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ثابت receiverp = randModeTarget.receiverp;
            ثابت منطقی isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_گره* classLevelStaticLoopp = nullptr;
            اگر (isClassLevel && nodep->argsp()) {
                اگر (AstVar* ثابت sVarp = getStaticRandModeVar(randModeTarget.classp)) {
                    File_سطر* ثابت fl = nodep->fileline();
                    Ast_گره_عبارت* ثابت staticLhsp
                        = makeModeAssignLhs(fl, randModeTarget.classp, nullptr, sVarp);
                    Ast_گره_عبارت* ثابت argClonep = nodep->argsp()->exprp()->clone_درخت_Pure(نادرست);
                    classLevelStaticLoopp = makeModeSetLoop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ثابت منطقی receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ثابت randModeVarp = receiverIsStaticRand
                                             ? getStaticRandModeVar(randModeTarget.classp)
                                             : getRandModeVarFromClass(randModeTarget.classp);
            اگر (!randModeVarp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_عبارت), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(افزودن_Deletep(m_stmtp), m_stmtp = nullptr);
                بازگشت;
            }
            Ast_گره_عبارت* ثابت lhsp = makeModeAssignLhs(nodep->fileline(), randModeTarget.classp,
                                                        randModeTarget.fromp, randModeVarp);
            replaceWithModeAssignAndAppend(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : nullptr, lhsp,
                classLevelStaticLoopp);
            بازگشت;
        }

        اگر (nodep->name() == "constraint_mode") {
            AstMethodCall* ثابت methodCallp = VN_CAST(nodep, MethodCall);
            Ast_گره_عبارت* fromp = methodCallp ? methodCallp->fromp() : nullptr;
            AstConstraint* constrp = nullptr;
            AstClass* classp = VN_CAST(m_modp, Class);
            اگر (AstConstraintRef* ثابت constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                اگر (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } وگرنه اگر (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ثابت constraintModeVarp = (constrp && constrp->isStatic())
                                                   ? getStaticConstraintModeVar(classp)
                                                   : getConstraintModeVar(classp);
            Ast_گره_عبارت* ثابت lhsp
                = makeModeAssignLhs(nodep->fileline(), classp, fromp, constraintModeVarp);
            replaceWithModeAssign(nodep, constrp, lhsp);
            بازگشت;
        }

        اگر (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            بازگشت;
        }

        اگر (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ثابت منطقی inStaticContext = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ثابت stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ثابت randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            اگر (inStaticContext) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(درست);
            }
            randomizeFuncp->addStmtsp(
                جدید AstAssign{nodep->fileline(),
                              جدید AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              جدید AstConst{nodep->fileline(), AstConst::Widthed_مقدار{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            صحیح argn = 0;
            AstWith* ثابت withp = nodep->withp();
            برای (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_گره_عبارت* ثابت exprp = argp->exprp();
                AstCMethodHard* ثابت basicMethodp = جدید AstCMethodHard{
                    nodep->fileline(),
                    جدید AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ثابت refvarp
                    = جدید AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::to_string(++argn), exprp->dtypep()};
                refvarp->setStdRandomizeArg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                اگر (withp) {
                    withp->foreach([&](Ast_گره_عبارت* exp) {
                        اگر (is_Similar_گره(exp, exprp)) {
                            AstVarRef* ثابت replaceVar
                                = جدید AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            exp->replaceWith(replaceVar);
                            replaceVar->user1(exp->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(افزودن_Deletep(exp), exp);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(درست);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->addStmtsp(refvarp);

                ثابت size_t width = exprp->width();
                basicMethodp->addPinsp(
                    جدید AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->addPinsp(
                    جدید AstConst{nodep->fileline(), AstConst::Unsized64{}, width});
                basicMethodp->dtypeSetBit();

                randomizeFuncp->addStmtsp(جدید AstAssign{
                    nodep->fileline(),
                    جدید AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    جدید AstAnd{nodep->fileline(),
                               جدید AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            اگر (withp) {
                File_سطر* ثابت fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, nullptr,
                                                                inStaticContext);
                withCapturep->addFunctionArguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                اگر (stdrand) {
                    randomizeFuncp->addStmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_گره* ثابت capturedTreep = withp->exprp()->unlinkFrBackWithNext();
                randomizeFuncp->addStmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_عبارت_Visitor{
                        nullptr, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        nullptr, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
                }
                AstCExpr* ثابت solverCallp = جدید AstCExpr{fl};
                solverCallp->dtypeSetBit();
                solverCallp->add(جدید AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->add(".next()");
                AstVar* ثابت fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_گره_عبارت* ثابت andExprp
                    = جدید AstAnd{fl, جدید AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->addStmtsp(
                    جدید AstAssign{fl, جدید AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlinkFrBack()->delete_درخت(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            اگر (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            اگر (withCapturep) nodep->addArgsp(withCapturep->getArgs());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            بازگشت;
        }
        handleRandomizeArgs(nodep);

        AstWith* ثابت withp = nodep->withp();
        اگر (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            بازگشت;
        }
        withp->unlinkFrBack();
        iterateChildren(nodep);

        AstClass* classp = nullptr;
        اگر (AstMethodCall* ثابت callp = VN_CAST(nodep, MethodCall)) {
            ثابت Ast_گره_DType* ثابت fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            ثابت AstClassRefDType* ثابت classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } وگرنه {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        اگر (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ثابت classGenp = getRandomGenerator(classp);
        AstVar* ثابت localGenp
            = جدید AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(درست);

        AstFunc* ثابت randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inlineUniqueNames.get(nodep), نادرست);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ثابت std::pair<منطقی, منطقی> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<منطقی, منطقی>{نادرست, نادرست};
        اگر (prePostWrap.first) {
            addVirtualPrePostCall(randomizeFuncp, classp, "pre_randomize");
        } وگرنه {
            addPrePostCall(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        اگر (classHasRandClassMembers(classp)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(classp, "pre");
            اگر (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->addStmtsp((جدید AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ثابت CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.addFunctionArguments(randomizeFuncp);

        // Add constraints clearing code
        اگر (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ثابت mainRandomizep
                = VN_CAST(m_memberMap.findMember(classp, "randomize"), Func);
            اگر (mainRandomizep && mainRandomizep->stmtsp()) {
                برای (Ast_گره* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    منطقی foundClearConstraints = نادرست;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        اگر (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->addStmtsp(stmtp->clone_درخت(نادرست));
                        } وگرنه اگر (methodp->method() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            foundClearConstraints = درست;
                        }
                    });
                    اگر (foundClearConstraints) بشکن;
                }
            }
            randomizeFuncp->addStmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->addStmtsp(localGenp);

        AstFunc* ثابت basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ثابت basicRandomizeFuncCallp
            = جدید AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        اگر (classGenp) {
            AstTask* ثابت constrSetupFuncp = getCreateConstraintSetupFunc(classp, m_memberMap);
            AstTaskRef* ثابت callp = جدید AstTaskRef{nodep->fileline(), constrSetupFuncp};
            randomizeFuncp->addStmtsp(callp->makeStmt());
            randomizeFuncp->addStmtsp(جدید AstAssign{
                nodep->fileline(), جدید AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                جدید AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), گره_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ثابت randModeVarp = getRandModeVarFromClass(classp);
        اگر (!classGenp && randModeVarp) addSetRandMode(randomizeFuncp, localGenp, randModeVarp);
        اگر (!classGenp) {
            اگر (AstVar* ثابت sVarp = getStaticRandModeVar(classp)) {
                addSetStaticRandMode(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_گره* ثابت capturedTreep = withp->exprp()->unlinkFrBackWithNext();
        randomizeFuncp->addStmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_گره* inlineResizeStmtsp = nullptr;
        {
            std::vector<AstCMethodHard*> sizeMethodps;
            capturedTreep->foreachAndNext([&](AstCMethodHard* methodp) {
                اگر (methodp->method() == VCMethod::DYN_SIZE
                    || methodp->method() == VCMethod::ASSOC_SIZE) {
                    sizeMethodps.افزودن_back(methodp);
                }
            });
            برای (AstCMethodHard* ثابت methodp : sizeMethodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = nullptr;
                Ast_گره_عبارت* ثابت ریشه_Exprp = methodp->fromp();
                اگر (AstMemberSel* memberSelp = VN_CAST(ریشه_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    وقتی (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ثابت Ast_گره_عبارت* ثابت fromp = memberSelp->fromp();
                    اگر (ثابت AstVarRef* ثابت varRefp = VN_CAST(fromp, VarRef)) {
                        اگر (varRefp->varp() == captured.getThisp()) ادامه;
                    } وگرنه {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } وگرنه اگر (AstVarRef* ثابت varRefp = VN_CAST(ریشه_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                اگر (!arrVarp) ادامه;
                // Only handle rand-declared dynamic/assoc array variables
                اگر (!arrVarp->rand().isRandomizable()) ادامه;
                // Do not randomize associative array size (IEEE 18.4)
                اگر (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) ادامه;
                File_سطر* ثابت fl = methodp->fileline();
                منطقی wasCreated = نادرست;
                AstClass* ثابت varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ثابت sizeVarp = createOrGetSizeVar(varClassp, arrVarp, fl,
                                                            methodp->findIntDType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                اگر (wasCreated) {
                    Ast_گره_عبارت* ثابت resizeVarRefp = buildMemberSelExprp(ریشه_Exprp, arrVarp);
                    Ast_گره_عبارت* ثابت sizeExprp = buildMemberSelExprp(ریشه_Exprp, sizeVarp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->addNext(createSizeGteZeroConstraint(fl, sizeVarp, ریشه_Exprp));
                    AstCMethodHard* ثابت resizep
                        = جدید AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, sizeExprp};
                    resizep->dtypep(methodp->findVoidDType());
                    inlineResizeStmtsp
                        = Ast_گره::addNext(inlineResizeStmtsp, جدید Ast_Stmt_عبارت{fl, resizep});
                }
                Ast_گره_عبارت* ثابت size_گره_Exprp = buildMemberSelExprp(ریشه_Exprp, sizeVarp);
                methodp->replaceWith(size_گره_Exprp);
                VL_DO_DANGLING(methodp->delete_درخت(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_عبارت_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                randModeVarp, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
        }

        // Call the solver and set return value
        AstCExpr* ثابت solverCallp = جدید AstCExpr{nodep->fileline()};
        solverCallp->dtypeSetBit();
        solverCallp->add(جدید AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->add(".next(__Vm_rng)");
        randomizeFuncp->addStmtsp(جدید AstAssign{
            nodep->fileline(),
            جدید AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            جدید AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        اگر (inlineResizeStmtsp) {
            AstVar* ثابت retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ثابت ifp = جدید AstIf{nodep->fileline(),
                                         جدید AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, nullptr};
            randomizeFuncp->addStmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        اگر (classHasRandClassMembers(classp)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(classp, "post");
            اگر (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->addStmtsp((جدید AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        اگر (prePostWrap.second) {
            addVirtualPrePostCall(randomizeFuncp, classp, "post_randomize");
        } وگرنه {
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
        VL_DO_DANGLING(withp->delete_درخت(), withp);
    }
    تهی visit(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        اگر (m_constraintp && nodep->fromp()->user1()
            && (nodep->method() == VCMethod::ASSOC_SIZE
                || nodep->method() == VCMethod::DYN_SIZE)) {
            AstClass* ثابت classp = VN_AS(m_modp, Class);
            AstVarRef* ثابت queueVarRefp = VN_CAST(nodep->fromp(), VarRef);
            اگر (!queueVarRefp) {
                // Warning from ConstraintExprVisitor will be thrown
                بازگشت;
            }
            AstVar* ثابت queueVarp = queueVarRefp->varp();
            منطقی wasCreated = نادرست;
            AstVar* ثابت sizeVarp
                = createOrGetSizeVar(classp, queueVarp, fl, nodep->findIntDType(), wasCreated);
            اگر (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                اگر (!VN_IS(queueVarp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    اگر (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ثابت sizeVarRefp = جدید AstVarRef{
                        fl, VN_AS(sizeVarp->user2p(), گره_Module), sizeVarp, VAccess::READ};
                    AstCMethodHard* ثابت resizep = جدید AstCMethodHard{
                        fl, nodep->fromp()->unlinkFrBack(), VCMethod::DYN_RESIZE, sizeVarRefp};
                    resizep->dtypep(nodep->findVoidDType());
                    resizerTaskp->addStmtsp(جدید Ast_Stmt_عبارت{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->addItemsp(createSizeGteZeroConstraint(fl, sizeVarp));
            }
            AstVarRef* ثابت sizeVarRefp = جدید AstVarRef{fl, VN_AS(sizeVarp->user2p(), گره_Module),
                                                         sizeVarp, VAccess::READ};
            sizeVarRefp->user1(درست);
            nodep->replaceWith(sizeVarRefp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    تهی visit(Ast_گره_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
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
        nodep->foreach([&](AstClass* ثابت classp) {
            اگر (!classp->user1()) بازگشت;
            classp->foreachMember([&](AstClass* ثابت ownerClassp, AstConstraint* ثابت constrp) {
                constrp->foreach([&](AstMemberSel* ثابت memberSelp) {
                    AstVar* ثابت varp = memberSelp->varp();
                    اگر (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) بازگشت;
                    اگر (VN_AS(varp->user2p(), گره_Module) != ownerClassp) بازگشت;
                    اگر (!varp->user3()) varp->user3(درست);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(افزودن_Deletep(constrp->unlinkFrBack()), constrp);
        });
    }
    ~RandomizeVisitor() override = پیشفرض;
};

//######################################################################
// Randomize method class functions

تهی V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت RandomizeMarkVisitor markVisitor{nodep};
        ثابت RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_درخت("randomize", 0, dump_درخت_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ثابت std::string& name, منطقی allowVirtual,
                                       منطقی childDType) {
    AstFunc* funcp = VN_AS(memberMap.findMember(nodep, name), Func);
    اگر (!funcp) {
        v3Global.useRandomizeMethods(درست);
        Ast_گره_DType* ثابت dtypep
            = childDType
                  ? جدید AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ثابت fvarp = childDType
                                  ? جدید AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlagChildDType{}, dtypep}
                                  : جدید AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3ErrorCode::NORETURN, درست);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(درست);
        fvarp->funcReturn(درست);
        fvarp->direction(VDirection::OUTPUT);
        nodep->addMembersp(funcp);
        funcp = جدید AstFunc{nodep->fileline(), name, nullptr, fvarp};
        اگر (!childDType) funcp->dtypep(dtypep);
        funcp->classMethod(درست);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->addMembersp(funcp);
        memberMap.درج(nodep, funcp);
    }
    بازگشت funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_گره_Module* nodep, ثابت std::string& name) {
    AstFunc* funcp = nullptr;
    v3Global.useRandomizeMethods(درست);
    Ast_گره_DType* ثابت dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
    AstVar* ثابت fvarp = جدید AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(درست);
    fvarp->funcReturn(درست);
    fvarp->direction(VDirection::OUTPUT);
    funcp = جدید AstFunc{nodep->fileline(), name, nullptr, fvarp};
    funcp->dtypep(dtypep);
    اگر (VN_IS(nodep, Class)) {
        funcp->classMethod(درست);
    } وگرنه {
        funcp->classMethod(درست);
        funcp->isStatic(درست);
    }
    nodep->addStmtsp(funcp);
    بازگشت funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ثابت basep = nodep->baseMostClassp();
    AstFunc* funcp = VN_AS(memberMap.findMember(basep, "srandom"), Func);
    اگر (!funcp) {
        v3Global.useRandomizeMethods(درست);
        Ast_گره_DType* ثابت dtypep
            = basep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ثابت ivarp = جدید AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(درست);
        ivarp->direction(VDirection::درون_داد);
        funcp = جدید AstFunc{basep->fileline(), "srandom", ivarp, nullptr};
        funcp->dtypep(basep->findVoidDType());
        funcp->classMethod(درست);
        funcp->isVirtual(نادرست);
        basep->addMembersp(funcp);
        memberMap.درج(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        اگر (basep->name() == "process") {
            funcp->addStmtsp(جدید AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } وگرنه {
            funcp->addStmtsp(جدید AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(درست);
        }
    }
    بازگشت funcp;
}
