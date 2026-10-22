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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "verilatedos.h"

#ضمن "V3Randomize.h"

#ضمن "V3Ast.h"
#ضمن "V3Const.h"
#ضمن "V3ConstPool.h"
#ضمن "V3Container.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Global.h"
#ضمن "V3MemberMap.h"
#ضمن "V3Task.h"
#ضمن "V3UniqueNames.h"

#ضمن <queue>
#ضمن <tuple>
#ضمن <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

تعداد ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

ساكن constexpr ثابت حرف* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
ساكن constexpr ثابت حرف* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
ساكن AstVar* get_Rand_نما_Var_From_Class(Ast_گره_Module* classp) {
    طالما (classp) {
        إذا (classp->user2p()) أرجع VN_AS(classp->user2p(), Var);
        AstClass* ثابت cp = VN_CAST(classp, Class);
        إذا (!cp || !cp->extendsp()) أرجع اشاره_تهی;
        classp = cp->extendsp()->classp();
    }
    أرجع اشاره_تهی;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ساكن AstVar* getRandomGenerator(AstClass* ثابت classp) {
    إذا (classp->user3p()) أرجع VN_AS(classp->user3p(), Var);
    إذا (classp->extendsp()) أرجع getRandomGenerator(classp->extendsp()->classp());
    أرجع اشاره_تهی;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ساكن AstTask* get_Create_Constraint_راهاندازی_Func(AstClass* classp, VMemberMap& memberMap) {
    ساكن ثابت حرف* ثابت name = "__Vsetup_constraints";
    AstTask* راهاندازی_All_Taskp = VN_AS(memberMap.پیدا_کن_Member(classp, name), Task);
    إذا (راهاندازی_All_Taskp) أرجع راهاندازی_All_Taskp;
    راهاندازی_All_Taskp = جديد AstTask{classp->fileline(), "__Vsetup_constraints", اشاره_تهی};
    راهاندازی_All_Taskp->class_روش(درست);
    راهاندازی_All_Taskp->isVirtual(درست);
    classp->جمع_کن_Membersp(راهاندازی_All_Taskp);
    memberMap.درج(classp, راهاندازی_All_Taskp);
    أرجع راهاندازی_All_Taskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

بنية Rand_نما_هدف final {
    // MEMBERS
    AstVar* ثابت receiverp;  // Variable that is the target of the rand_mode() call
    Ast_گره_عبارت* ثابت fromp;  // Expression from which the rand_mode() call originates
    AstClass* ثابت classp;  // Class in which rand_mode should be set

    // METHODS
    ساكن Rand_نما_هدف get(Ast_گره_عبارت* fromp, Ast_گره_Module* modp) {
        إذا (!fromp) أرجع {اشاره_تهی, اشاره_تهی, VN_AS(modp, Class)};
        إذا (AstCMethodHard* ثابت روش_Hardp = VN_CAST(fromp, CMethodHard)) {
            أرجع Rand_نما_هدف::get(روش_Hardp->fromp(), modp);
        }
        AstVar* receiverp = اشاره_تهی;
        AstClass* classp = VN_CAST(modp, Class);
        إذا (AstVarRef* ثابت varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            إذا (receiverp->isRand()) {
                fromp = اشاره_تهی;
                إذا (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } وإلا إذا (AstMemberSel* ثابت memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            إذا (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        إذا (!receiverp->isRand()) {
            AstClassRefDType* ثابت classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            إذا (classRefDtp) classp = classRefDtp->classp();
        }
        أرجع {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

اتحاد Randomize_نما final {
    // MEMBERS
    بنية {
        منطقي استفاده_میکند_نما : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t شاخص : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ساكن Ast_گره_DType* arrayElementDTypep(Ast_گره_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    طالما (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    أرجع dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ساكن منطقي dtype_شامل_است_Real(Ast_گره_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    إذا (Ast_گره_DType* ثابت subp = dtypep->subDTypep()) أرجع dtype_شامل_است_Real(subp);
    إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
        إذا (structp->packed()) أرجع خطأ;
        لكل (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            إذا (dtype_شامل_است_Real(memberp->subDTypep())) أرجع درست;
        }
        أرجع خطأ;
    }
    ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
    أرجع basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
ساكن فراغ checkRandTypeEligibility(Ast_گره* contextp, Ast_گره_DType* dtypep, منطقي isRandc) {
    dtypep = dtypep->skipRefp();
    إذا (Ast_گره_DType* ثابت subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        أرجع;
    }
    إذا (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        إذا (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        أرجع;
    }
    إذا (ثابت AstIfaceRefDType* ثابت ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        أرجع;
    }
    إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
        إذا (structp->packed()) أرجع;  // Integral by construction; members checked separately
        إذا (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            أرجع;
        }
        لكل (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            إذا (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        أرجع;
    }
    ثابت AstBasicDType* ثابت basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    إذا (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        إذا (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        أرجع;
    }
    بدل (basicp->keyword()) {
    حالة VBasicDTypeKwd::STRING:
    حالة VBasicDTypeKwd::CHANDLE:
    حالة VBasicDTypeKwd::EVENT: {
        ثابت حرف* ثابت articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        اكسر;
    }
    افتراضي: اكسر;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

صنف RandomizeMarkVisitor final : عام VNVisitor {
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
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;

    استفاده Derived_مجموعه = std::unordered_مجموعه<AstClass*>;
    استفاده پایه_مقصد_Derived_Map = std::unordered_map<ثابت AstClass*, Derived_مجموعه>;

    پایه_مقصد_Derived_Map m_پایه_مقصد_Derived_Map;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = اشاره_تهی;  // Current class
    Ast_گره* m_constraint_عبارت_Genp = اشاره_تهی;  // Current constraint or constraint if expression
    Ast_گره_Module* m_modp;  // Current module
    Ast_گره_Stmt* m_stmtp = اشاره_تهی;  // Current statement
    Ast_گره_FTask_Ref* m_stdRandCallp = اشاره_تهی;  // Current std::randomize() call
    منطقي m_درون_Std_With = خطأ;  // True when inside a 'with {}' clause
    std::مجموعه<Ast_گره_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = اشاره_تهی;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    فراغ markGlobalConstrainedVars(AstClass* classp) {
        لكل (ثابت AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : اشاره_تهی) {
            لكل (Ast_گره* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                إذا (!varp) استمر;
                إذا (varp->rand().isRandomizable()) varp->globalConstrained(درست);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    منطقي is_Var_درون_Std_Randomize_آرگومانها(ثابت AstVar* varp) ثابت {
        إذا (!m_درون_Std_With || !m_stdRandCallp) أرجع خطأ;

        لكل (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ثابت Ast_گره_عبارت* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            طالما (exprp) {
                إذا (ثابت Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(exprp, گره_Var_Ref)) {
                    إذا (varrefp->varp() == varp) أرجع درست;
                    اكسر;
                }
                إذا (ثابت AstMemberSel* ثابت memberselp = VN_CAST(exprp, MemberSel)) {
                    إذا (memberselp->varp() == varp) أرجع درست;
                    exprp = memberselp->fromp();
                } وإلا إذا (ثابت AstArraySel* ثابت arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } وإلا إذا (ثابت AstStructSel* ثابت strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } وإلا إذا (ثابت AstCMethodHard* ثابت methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } وإلا {
                    اكسر;
                }
            }
        }
        أرجع خطأ;
    }
    فراغ markMembers(ثابت AstClass* nodep) {
        لكل (ثابت AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : اشاره_تهی) {
            لكل (Ast_گره* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                إذا (!varp) استمر;
                // If member is randomizable and of class type, mark its class
                إذا (varp->rand().isRandomizable()) {
                    ثابت Ast_گره_DType* ثابت varDtypep = varp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    إذا (!classRefp) {
                        ثابت Ast_گره_DType* subDTypep = varDtypep->subDTypep();
                        إذا (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    إذا (classRefp) {
                        AstClass* ثابت rclassp = classRefp->classp();
                        إذا (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    إذا (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        Randomize_نما rand_نما = {};
                        rand_نما.استفاده_میکند_نما = درست;
                        varp->user1(rand_نما.asUQuad);
                    }
                }
            }
        }
    }
    فراغ markDerived(ثابت AstClass* nodep) {
        ثابت خودکار it = m_پایه_مقصد_Derived_Map.پیدا_کن(nodep);
        إذا (it != m_پایه_مقصد_Derived_Map.انتها()) {
            لكل (خودکار* classp : it->دومی) {
                إذا (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    فراغ markAllDerived() {
        لكل (ثابت خودکار& p : m_پایه_مقصد_Derived_Map) {
            إذا (p.اولی->user1()) markDerived(p.اولی);
        }
    }
    فراغ مجموعه_Package_Refs() {
        لكل (Ast_گره_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), گره_Module));
        }
    }
    فراغ markNestedGlobalConstrainedRecurse(Ast_گره* nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            AstVar* ثابت varp = refp->varp();
            إذا (varp->globalConstrained()) أرجع;
            varp->globalConstrained(درست);
        } وإلا إذا (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(nodep, MemberSel)) {
            إذا (memberSelp->varp()) {
                AstVar* ثابت varp = memberSelp->varp();
                إذا (varp->globalConstrained()) أرجع;
                varp->globalConstrained(درست);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_گره_عبارت* buildMemberSelChain(AstVarRef* ریشه_Var_Refp, ثابت std::وکتور<AstVar*>& مسیر) {
        Ast_گره_عبارت* exprp = ریشه_Var_Refp->clone_درخت(خطأ);
        لكل (AstVar* memberVarp : مسیر) {
            AstMemberSel* memberSelp
                = جديد AstMemberSel{ریشه_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        أرجع exprp;
    }

    // Process a single constraint during nested constraint cloning
    فراغ processNestedConstraint(AstConstraint* ثابت constrp, AstVarRef* ریشه_Var_Refp,
                                 ثابت std::وکتور<AstVar*>& new_مسیر, AstClass* هدف_Classp) {
        std::string مسیر_پیشوند = ریشه_Var_Refp->name();
        لكل (AstVar* مسیر_Member_Varp : new_مسیر) {
            مسیر_پیشوند += GLOBAL_CONSTRAINT_SEPARATOR + مسیر_Member_Varp->name();
        }

        ثابت std::string newName = مسیر_پیشوند + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        منطقي isDuplicate = خطأ;
        هدف_Classp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت existingConstrp) {
            إذا (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = درست;
            }
        });
        إذا (isDuplicate) أرجع;

        AstConstraint* ثابت cloneConstrp = constrp->clone_درخت(خطأ);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::وکتور<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_گره_عبارت* exprp) {
            إذا (AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
                إذا (!varRefp->varp()->isClassMember()) أرجع;
                Ast_گره_عبارت* ثابت chainp = buildMemberSelChain(ریشه_Var_Refp, new_مسیر);
                AstMemberSel* ثابت finalSelp
                    = جديد AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(افزودن_Deletep(varRefp), varRefp);
            } وإلا إذا (AstFuncRef* ثابت funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                إذا (funcRefp->taskp()->class_روش() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.افزودن_پشت(funcRefp);
                }
            }
        });
        لكل (AstFuncRef* funcRefp : funcRefps) {
            Ast_گره_عبارت* ثابت chainp = buildMemberSelChain(ریشه_Var_Refp, new_مسیر);
            AstArg* ثابت argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlink_Fr_پشت_With_بعدی() : اشاره_تهی;
            Ast_روش_Call* ثابت callp
                = جديد Ast_روش_Call{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(افزودن_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        هدف_Classp->جمع_کن_Stmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    فراغ cloneNestedConstraintsRecurse(AstVarRef* ریشه_Var_Refp, AstClass* classp,
                                       ثابت std::وکتور<AstVar*>& مسیر_مقصد_Class,
                                       AstClass* هدف_Classp) {
        لكل (Ast_گره* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ثابت memberVarp = VN_CAST(memberNodep, Var);
            إذا (!memberVarp) استمر;
            إذا (!memberVarp->rand().isRandomizable()) استمر;
            ثابت AstClassRefDType* ثابت memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            إذا (!memberClassRefp || !memberClassRefp->classp()) استمر;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::وکتور<AstVar*> new_مسیر = مسیر_مقصد_Class;
            new_مسیر.افزودن_پشت(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ثابت containingClassp, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, ریشه_Var_Refp, new_مسیر, هدف_Classp);
                });
            cloneNestedConstraintsRecurse(ریشه_Var_Refp, nestedClassp, new_مسیر, هدف_Classp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    فراغ handle_Randomize_آرگومان(Ast_گره_عبارت* exprp, AstVar* ثابت varp, ثابت منطقي stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        طالما (exprp) {
            AstVar* randVarp = اشاره_تهی;
            إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } وإلا إذا (AstArraySel* ثابت arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                استمر;  // Skip ArraySel, continue traversing
            } وإلا إذا (AstStructSel* ثابت structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                استمر;  // Skip StructSel, continue traversing
            } وإلا إذا (AstCMethodHard* ثابت methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                استمر;
            } وإلا إذا (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(درست);
                varrefp->access(VAccess::READWRITE);
                exprp = اشاره_تهی;
            } وإلا {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            إذا (randVarp == varp) أرجع;
            Ast_گره* backp = randVarp;
            طالما (backp && !VN_IS(backp, گره_Module)) backp = backp->backp();
            إذا (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } وإلا {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                Randomize_نما rand_نما = {};
                rand_نما.استفاده_میکند_نما = درست;
                randVarp->user1(rand_نما.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    فراغ بازدید(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        إذا (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ثابت AstClass* ثابت basep = nodep->extendsp()->classp();
            m_پایه_مقصد_Derived_Map[basep].درج(nodep);
        }
    }
    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->backp()) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        إذا (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        إذا (nodep->name() == "rand_mode") {
            Ast_روش_Call* ثابت روش_Callp = VN_CAST(nodep, روش_Call);
            Ast_گره_عبارت* ثابت fromp = روش_Callp ? روش_Callp->fromp() : اشاره_تهی;
            منطقي valid = درست;
            إذا (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = خطأ;
            } وإلا إذا (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = خطأ;
            } وإلا إذا (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = خطأ;
            } وإلا إذا (AstCMethodHard* ثابت روش_Hardp = VN_CAST(fromp, CMethodHard)) {
                إذا (روش_Hardp->روش() == VCMethod::ARRAY_AT
                    || روش_Hardp->روش() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = خطأ;
                } وإلا {
                    روش_Hardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            إذا (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_عبارت)
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_کد::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = خطأ;
            }
            إذا (valid) {
                ثابت Rand_نما_هدف rand_نما_هدف = Rand_نما_هدف::get(fromp, m_classp);
                إذا ((!rand_نما_هدف.receiverp || !rand_نما_هدف.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = خطأ;
                } وإلا إذا (!rand_نما_هدف.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = خطأ;
                } وإلا إذا (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = خطأ;
                } وإلا إذا (rand_نما_هدف.receiverp && rand_نما_هدف.receiverp->isRand()) {
                    // Called on a rand member variable
                    Randomize_نما rand_نما = {};
                    rand_نما.استفاده_میکند_نما = درست;
                    rand_نما_هدف.receiverp->user1(rand_نما.asUQuad);
                } وإلا {
                    // Called on 'this' or a non-rand class instance
                    rand_نما_هدف.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        إذا (!varp->isRand()) أرجع;
                        Randomize_نما rand_نما = {};
                        rand_نما.استفاده_میکند_نما = درست;
                        varp->user1(rand_نما.asUQuad);
                    });
                }
            }
            إذا (!valid) {
                إذا (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                    nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } وإلا {
                    m_stmtp->unlink_Fr_پشت();
                }
            }
            أرجع;
        }

        إذا (nodep->name() == "constraint_mode") {
            منطقي valid = درست;
            إذا (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = خطأ;
            } وإلا إذا (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_عبارت)
                       && !nodep->backp()->fileline()->warnIsOff(V3_Error_کد::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = خطأ;
            }
            AstConstraint* constrp = اشاره_تهی;
            AstClass* classp = m_classp;
            إذا (ثابت Ast_روش_Call* ثابت روش_Callp = VN_CAST(nodep, روش_Call)) {
                إذا (ثابت AstConstraintRef* ثابت constrRefp
                    = VN_CAST(روش_Callp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    إذا (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } وإلا إذا (AstClassRefDType* classRefDtp
                           = VN_CAST(روش_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } وإلا {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = خطأ;
                }
            }
            إذا (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = خطأ;
            }
            إذا (valid) {
                Randomize_نما constraint_نما = {};
                constraint_نما.استفاده_میکند_نما = درست;
                إذا (constrp) {
                    constrp->user1(constraint_نما.asUQuad);
                } وإلا {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraint_نما.asUQuad);
                    });
                }
            } وإلا {
                إذا (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                    nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } وإلا {
                    m_stmtp->unlink_Fr_پشت();
                }
            }
            أرجع;
        }

        إذا (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            إذا (m_constraint_عبارت_Genp || m_درون_Std_With) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                لكل (Ast_گره* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    إذا (argp->user1()) {
                        nodep->user1(درست);
                        اكسر;
                    }
                }
            }
            أرجع;
        }
        AstClass* classp = m_classp;
        إذا (ثابت Ast_روش_Call* ثابت روش_Callp = VN_CAST(nodep, روش_Call)) {
            إذا (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(روش_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        إذا (classp) {
            إذا (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت memberVarp) {
                إذا (!memberVarp->rand().isRandomizable()) أرجع;
                ثابت Ast_گره_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                إذا (!classRefp || !classRefp->classp()) أرجع;
                AstClass* ثابت memberClassp = classRefp->classp();
                إذا (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) أرجع;
                memberVarp->globalConstrained(درست);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* ریشه_Var_Refp
                    = جديد AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::وکتور<AstVar*> خالی_مسیر;
                memberClassp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, ریشه_Var_Refp, خالی_مسیر, classp);
                });
                cloneNestedConstraintsRecurse(ریشه_Var_Refp, memberClassp, خالی_مسیر, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(ریشه_Var_Refp->delete_درخت(), ریشه_Var_Refp);
            });
        }
        إذا (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = اشاره_تهی;
            لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handle_Randomize_آرگومان(argp->exprp(), اشاره_تهی, درست);
            }
            أرجع;
        }
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            إذا (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->رقم().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ثابت منطقي hasUnsupportedMember
                    = classp->existsMember([](ثابت AstClass*, ثابت AstVar* memberVarp) {
                          إذا (!memberVarp->rand().isRandomizable()) أرجع خطأ;
                          ثابت Ast_گره_DType* ثابت dtp = memberVarp->dtypep()->skipRefp();
                          أرجع VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, صف_DType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                إذا (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                استمر;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = اشاره_تهی;  // If nodep is a method call, this is its receiver
            إذا (Ast_روش_Call* روش_Callp = VN_CAST(nodep, روش_Call)) {
                إذا (AstMemberSel* ثابت memberSelp = VN_CAST(روش_Callp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } وإلا {
                    AstVarRef* ثابت varrefp = VN_AS(روش_Callp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handle_Randomize_آرگومان(argp->exprp(), fromVarp, خطأ);
        }
        // Re-mark members after the class became inline-randomized above.
        إذا (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    فراغ بازدید(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_عبارت_Genp);
        m_stmtp = nodep;
        m_constraint_عبارت_Genp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->backp()) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(Ast_Constraint_عبارت* nodep) override {
        VL_RESTORER(m_constraint_عبارت_Genp);
        m_constraint_عبارت_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_عبارت_Genp);
            m_constraint_عبارت_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterate_And_بعدی_Const_Null(nodep->thensp());
        iterate_And_بعدی_Const_Null(nodep->elsesp());
    }
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        إذا (!m_constraint_عبارت_Genp) أرجع;

        إذا (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        إذا (nodep->varp()->rand().isRandomizable() && !(m_درون_Std_With && m_stdRandCallp)) {
            nodep->user1(درست);
        } وإلا إذا (is_Var_درون_Std_Randomize_آرگومانها(nodep->varp())) {
            nodep->user1(درست);
        }
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        إذا (!m_constraint_عبارت_Genp) أرجع;
        iterateChildrenConst(nodep);
        ثابت منطقي randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ثابت منطقي randMember = nodep->varp()->rand().isRandomizable();
        ثابت منطقي درون_Std_With = m_درون_Std_With && m_stdRandCallp;
        إذا (randObject && randMember && !درون_Std_With) {
            nodep->user1(درست);
        } وإلا إذا (درون_Std_With && is_Var_درون_Std_Randomize_آرگومانها(nodep->varp())) {
            nodep->user1(درست);
            // Mark parent object for constraint expression visitor
            إذا (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(درست);
        }

        إذا (m_withp) {
            Ast_گره* backp = m_withp;
            طالما (backp->backp()) {
                إذا (ثابت Ast_روش_Call* ثابت callp = VN_CAST(backp, روش_Call)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    اكسر;
                }
                backp = backp->backp();
            }
        } وإلا {
            nodep->user2p(m_modp);
        }
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstVar* nodep) override {
        nodep->user2p(m_modp);
        إذا (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        إذا (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_درون_Std_With = درست;
        iterateChildrenConst(nodep);
        m_درون_Std_With = خطأ;
    }

    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        iterateChildrenConst(nodep);
        إذا (!m_constraint_عبارت_Genp && !m_درون_Std_With) أرجع;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    فراغ بازدید(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        إذا (!m_constraint_عبارت_Genp && !m_درون_Std_With) أرجع;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    صریح RandomizeMarkVisitor(Ast_گره* nodep) {
        iterateConst(nodep);
        markAllDerived();
        مجموعه_Package_Refs();
    }
    ~RandomizeMarkVisitor() override = افتراضي;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

صنف Constraint_عبارت_Visitor final : عام VNVisitor {
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
    AstVar* m_rand_نما_Varp;  // Relevant randmode state variable
    منطقي m_wantSingle = خطأ;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    منطقي m_structSel = خطأ;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    منطقي m_defer_Rand_نما_Hoist = خطأ;  // Wait for the complete struct member path
    std::مجموعه<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::مجموعه<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertion_مجموعه<AstVar*>* m_اندازه_Constrained_Arraysp = اشاره_تهی;  // Arrays with size constraints
    Ast_گره_عبارت* m_conditionp = اشاره_تهی;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_گره_FTask* m_prepareConstrainedArraysp = اشاره_تهی;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = اشاره_تهی;  // Current ID of unique call
    V3_Unique_نامها& m_unique_نامها;  // Unique names of temporaries, and of blocks holding them
    std::وکتور<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_گره* m_اولی_عبارت_Inside_Indexp = اشاره_تهی;
    AstConstraintForeach* m_foreachp = اشاره_تهی;  // Innermost constraint-foreach being processed
    std::مجموعه<ثابت AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    صنف Nested_Access_مسیر final {
        AstMemberSel* m_بالاترین_Nested_Array_Member_Selp
            = اشاره_تهی;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = اشاره_تهی;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = اشاره_تهی;  // Last node of variable's name format
        Ast_گره** m_اولی_عبارت_Inside_شاخص_Pointerp
            = اشاره_تهی;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    عام:
        فراغ جمع_کن_Var_Name_Part(AstSFormatF* ثابت nodep, std::string&& name) {
            إذا (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } وإلا {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.درج(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            إذا (name == "") {
                إذا (*m_اولی_عبارت_Inside_شاخص_Pointerp) {
                    (*m_اولی_عبارت_Inside_شاخص_Pointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_اولی_عبارت_Inside_شاخص_Pointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_اولی_عبارت_Inside_شاخص_Pointerp = nodep;
            }
        }

        AstMemberSel* access_درخت() { أرجع m_بالاترین_Nested_Array_Member_Selp; }
        ثابت std::string& smtName() { أرجع m_smtName; }

        Ast_Stmt_عبارت* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ثابت methodp = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(genp->user2p(), گره_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtype_مجموعه_Void();

            methodp->جمع_کن_Pinsp(m_بالاترین_Nested_Array_Member_Selp);  // Variable

            ثابت Ast_گره_DType* ثابت dtypep = varp->dtypep();  //عرض
            ثابت اندازه_t عرض = m_بالاترین_Nested_Array_Member_Selp->varp()->dtypep()->عرض();
            methodp->جمع_کن_Pinsp(جديد AstConst{dtypep->fileline(), AstConst::Unsized64{}, عرض});

            methodp->جمع_کن_Pinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = اشاره_تهی;

            methodp->جمع_کن_Pinsp(
                جديد AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_بالاترین_Nested_Array_Member_Selp = اشاره_تهی;
            أرجع methodp->makeStmt();
        }

        Ast_گره_عبارت* cloneName() { أرجع m_nestedNameFormatTopp->clone_درخت(خطأ); }

        فراغ error() {
            VL_DO_DANGLING(m_بالاترین_Nested_Array_Member_Selp->delete_درخت(), m_بالاترین_Nested_Array_Member_Selp);
        }

        Nested_Access_مسیر(AstMemberSel* بالاترین_Member_Selp, Ast_گره** اشاره_گر_مقصد_اولی_Exprp)
            : m_بالاترین_Nested_Array_Member_Selp{بالاترین_Member_Selp}
            , m_اولی_عبارت_Inside_شاخص_Pointerp{اشاره_گر_مقصد_اولی_Exprp} {}
        ~Nested_Access_مسیر() {
            إذا (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_درخت(),
                            m_nestedNameFormatTopp = اشاره_تهی);
                m_nestedNameFormatp = اشاره_تهی;
            }
        }
    };
    Nested_Access_مسیر* m_nestedAccess = اشاره_تهی;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_گره_عبارت* createEnumConstraint(File_سطر* ثابت fl, ثابت std::string& smtName,
                                      AstEnumDType* ثابت enumDtp, AstVar* ثابت genVarp) {
        Ast_گره_Module* ثابت genModp = VN_AS(genVarp->user2p(), گره_Module);
        ثابت صحيح عرض = enumDtp->عرض();
        std::string constraint = "(__Vbv (or";
        لكل (Ast_Enum_قلم* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
            ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvt_مقصد_Str(vconstp->مقصد_UInt()) + " "
                          + cvt_مقصد_Str(عرض) + "))";
        }
        constraint += "))";
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + constraint + "\""}};
        callp->dtype_مجموعه_Void();
        أرجع callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* پیدا_کن_Static_Rand_نما_Var_Member(AstClass* classp) ثابت {
        طالما (درست) {
            إذا (AstVar* ثابت varp
                = VN_CAST(m_memberMap.پیدا_کن_Member(classp, "__Vstaticrandmode"), Var)) {
                أرجع varp;
            }
            AstClassExtends* ثابت extendsp = classp->extendsp();
            إذا (!extendsp) أرجع اشاره_تهی;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    فراغ markConstrainedRandMember(AstStructDType* ثابت structp, ثابت std::string& name) {
        AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.پیدا_کن_Member(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(درست);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    منطقي is_Rand_Qualified_مسیر(ثابت Ast_گره_عبارت* nodep) {
        طالما (nodep) {
            إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                إذا (AstStructDType* ثابت structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ثابت AstMemberDType* ثابت memberp
                        = VN_CAST(m_memberMap.پیدا_کن_Member(structp, selp->name()), MemberDType);
                    إذا (memberp && !memberp->rand().isRandomizable()) أرجع خطأ;
                }
                nodep = selp->fromp();
            } وإلا إذا (ثابت AstArraySel* ثابت selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } وإلا إذا (ثابت AstCMethodHard* ثابت methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } وإلا {
                اكسر;
            }
        }
        أرجع درست;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    ساكن فراغ markStructConstrainedRandRecurse(Ast_گره_DType* ثابت dtypep) {
        AstStructDType* ثابت structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        إذا (!structp || structp->packed()) أرجع;
        structp->markConstrainedRand(درست);
        لكل (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            إذا (!memberp->rand().isRandomizable()) استمر;
            memberp->markConstrainedRand(درست);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string build_Member_مسیر(ثابت AstMemberSel* ثابت memberSelp) {
        ثابت Ast_گره* fromp = memberSelp->fromp();
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            أرجع refp->name() + "." + memberSelp->name();
        } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            أرجع build_Member_مسیر(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        أرجع "";
    }

    ساكن Ast_گره_عبارت* getFromp(ثابت Ast_گره_عبارت* ثابت nodep) {
        إذا (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            أرجع cmethp->fromp();
        }
        أرجع getSelFromp(nodep);
    }

    ساكن std::string استخراج_Solve_Before_پایه_Name(ثابت Ast_گره_عبارت* exprp) {
        طالما (ثابت Ast_گره_عبارت* ثابت fromp = getFromp(exprp)) exprp = fromp;
        إذا (ثابت AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) أرجع vrefp->name();
        أرجع "";
    }

    ساكن فراغ build_Name_پیشوند(AstCExpr* exprp, ثابت Ast_گره_عبارت* ثابت nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            exprp->جمع_کن("(\""s + refp->name());
            أرجع;
        }

        ثابت Ast_گره_عبارت* ثابت fromp = getFromp(nodep);
        إذا (fromp) build_Name_پیشوند(exprp, fromp);

        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            exprp->جمع_کن("." + selp->name());
        } وإلا إذا (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            أرجع;
        } وإلا إذا (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            إذا (cmethp->روش() == VCMethod::ARRAY_AT) {
                Ast_گره_عبارت* ثابت argp = cmethp->pinsp();
                exprp->جمع_کن(".\" + ");
                AstCFuncHard* ثابت solverHexFuncp
                    = جديد AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_مقصد_SOLVER_HEX};
                solverHexFuncp->جمع_کن_Pinsp(argp->clone_درخت_خالص(خطأ));
                solverHexFuncp->dtype_مجموعه_String();
                exprp->جمع_کن(solverHexFuncp);
                exprp->جمع_کن(" + \"");
            } وإلا {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                أرجع;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ساكن AstCExpr* build_مسیر_عبارت(File_سطر* ثابت fl, ثابت Ast_گره_عبارت* nodep,
                                   ثابت std::string selName) {
        AstCExpr* ثابت exprp = جديد AstCExpr{fl, ""};
        build_Name_پیشوند(exprp, nodep);
        إذا (selName != "") {
            exprp->جمع_کن(".");
            exprp->جمع_کن(selName);
        }
        exprp->جمع_کن("\")");
        أرجع exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_عبارت(File_سطر* fl, ثابت std::string& پایه_Name,
                                    ثابت Ast_گره_Sel* selp) {
        AstCExpr* ثابت p = جديد AstCExpr{fl, ""};
        p->جمع_کن("(\""s + پایه_Name + ".\"");
        p->جمع_کن("+");
        AstCFuncHard* ثابت solverHexFuncp
            = جديد AstCFuncHard{fl, VCFunction::RANDOMIZER_مقصد_SOLVER_HEX};
        solverHexFuncp->جمع_کن_Pinsp(selp->bitp()->clone_درخت_خالص(خطأ));
        solverHexFuncp->dtype_مجموعه_String();
        p->جمع_کن(solverHexFuncp);
        p->جمع_کن(")");
        p->dtype_مجموعه_String();
        أرجع p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ساكن Ast_گره_عبارت* getSelFromp(ثابت Ast_گره_عبارت* exprp) {
        إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(exprp, MemberSel)) أرجع mp->fromp();
        إذا (ثابت AstStructSel* ثابت sp = VN_CAST(exprp, StructSel)) أرجع sp->fromp();
        أرجع اشاره_تهی;
    }

    Ast_گره_عبارت* build_Solve_Before_Name_عبارت(File_سطر* fl, Ast_گره_عبارت* exprp) {
        إذا (ثابت AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ثابت p = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + varrefp->name() + "\"s"};
            p->dtype_مجموعه_String();
            أرجع p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        إذا (Ast_گره_عبارت* ثابت selFromp = getSelFromp(exprp)) {
            ثابت std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            إذا (ثابت AstArraySel* ثابت arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string پایه_Name;
                إذا (ثابت AstVarRef* ثابت vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    پایه_Name = vp->name();
                } وإلا إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    پایه_Name = build_Member_مسیر(mp);
                }
                إذا (پایه_Name.خالی()) أرجع اشاره_تهی;
                AstCExpr* ثابت p = جديد AstCExpr{fl, ""};
                p->جمع_کن("(\""s + پایه_Name + ".\"");
                p->جمع_کن("+");
                AstCFuncHard* ثابت solverHexFuncp
                    = جديد AstCFuncHard{fl, VCFunction::RANDOMIZER_مقصد_SOLVER_HEX};
                solverHexFuncp->جمع_کن_Pinsp(arrSelp->bitp()->clone_درخت_خالص(خطأ));
                solverHexFuncp->dtype_مجموعه_String();
                p->جمع_کن(solverHexFuncp);
                p->جمع_کن(")");
                p->dtype_مجموعه_String();
                أرجع p;
            }
            // Static member path (obj.field)
            إذا (ثابت AstVarRef* ثابت vp = VN_CAST(selFromp, VarRef)) {
                ثابت std::string مسیر = vp->name() + "." + selName;
                AstCExpr* ثابت p = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + مسیر + "\"s"};
                p->dtype_مجموعه_String();
                أرجع p;
            }
            إذا (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ثابت p = build_مسیر_عبارت(fl, selFromp, selName);
                p->dtype_مجموعه_String();
                أرجع p;
            }
            أرجع اشاره_تهی;
        }
        إذا (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ثابت p = build_مسیر_عبارت(fl, exprp, "");
            p->dtype_مجموعه_String();
            أرجع p;
        }
        ثابت Ast_گره_Sel* selp = VN_CAST(exprp, ArraySel);
        إذا (!selp) selp = VN_CAST(exprp, AssocSel);
        إذا (selp) {
            // arr[i] -> dynamic name
            std::string پایه_Name;
            إذا (ثابت AstVarRef* ثابت vp = VN_CAST(selp->fromp(), VarRef)) {
                پایه_Name = vp->name();
            } وإلا إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(selp->fromp(), MemberSel)) {
                پایه_Name = build_Member_مسیر(mp);
            }
            إذا (پایه_Name.خالی()) أرجع اشاره_تهی;
            أرجع build_Array_Sel_Name_عبارت(fl, پایه_Name, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        إذا (ثابت AstSel* ثابت بیت_Selp = VN_CAST(exprp, Sel)) {
            أرجع build_Solve_Before_Name_عبارت(fl, بیت_Selp->fromp());
        }
        أرجع اشاره_تهی;
    }

    AstSFormatF* getConstFormat(Ast_گره_عبارت* nodep) {
        أرجع جديد AstSFormatF{nodep->fileline(), (nodep->عرض() & 3) ? "#b%b" : "#x%x", خطأ,
                               nodep};
    }
    منطقي editFormat(Ast_گره_عبارت* nodep) {
        إذا (nodep->user1()) أرجع خطأ;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlink_Fr_پشت(&handle);
        handle.relink(getConstFormat(nodep));
        أرجع درست;
    }
    Ast_گره_عبارت* wrapWithCond(Ast_گره_عبارت* ثابت exprp, Ast_گره_عبارت* ثابت condp,
                              ثابت صحيح عرض) {
        إذا (condp) {
            File_سطر* ثابت flp = exprp->fileline();
            أرجع جديد AstCond{
                flp, condp, exprp,
                getConstFormat(جديد AstConst{flp, AstConst::Widthed_مقدار{}, عرض, 0})};
        }
        أرجع exprp;
    }
    AstSFormatF* editSMT(Ast_گره_عبارت* nodep, Ast_گره_عبارت* lhsp = اشاره_تهی,
                         Ast_گره_عبارت* rhsp = اشاره_تهی, Ast_گره_عبارت* thsp = اشاره_تهی) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_عبارت = nodep->emitSMT();  // Might need child width (AstExtend)
        إذا (smt_عبارت == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            أرجع اشاره_تهی;
        }

        ثابت صحيح lhs_عرض = lhsp ? lhsp->عرض() : 0;
        ثابت صحيح rhs_عرض = rhsp ? rhsp->عرض() : 0;
        ثابت صحيح ths_عرض = thsp ? thsp->عرض() : 0;
        Ast_گره_عبارت* lhsCondp = اشاره_تهی;
        Ast_گره_عبارت* rhsCondp = اشاره_تهی;
        Ast_گره_عبارت* thsCondp = اشاره_تهی;

        إذا (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = اشاره_تهی;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlink_Fr_پشت() : lhsp),
                         گره_عبارت);
            lhsCondp = m_conditionp;
        }
        إذا (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = اشاره_تهی;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlink_Fr_پشت() : rhsp),
                         گره_عبارت);
            rhsCondp = m_conditionp;
        }
        إذا (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = اشاره_تهی;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlink_Fr_پشت() : thsp),
                         گره_عبارت);
            thsCondp = m_conditionp;
        }

        Ast_گره_عبارت* argsp = اشاره_تهی;
        لكل (string::iterator pos = smt_عبارت.begin(); pos != smt_عبارت.انتها(); ++pos) {
            إذا (pos[0] == '%') {
                ++pos;
                بدل (pos[0]) {
                حالة '%': اكسر;
                حالة 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گره::جمع_کن_بعدی(argsp, wrapWithCond(lhsp, lhsCondp, lhs_عرض));
                    lhsp = اشاره_تهی;
                    اكسر;
                حالة 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گره::جمع_کن_بعدی(argsp, wrapWithCond(rhsp, rhsCondp, rhs_عرض));
                    rhsp = اشاره_تهی;
                    اكسر;
                حالة 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گره::جمع_کن_بعدی(argsp, wrapWithCond(thsp, thsCondp, ths_عرض));
                    thsp = اشاره_تهی;
                    اكسر;
                افتراضي: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); اكسر;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ثابت newp = جديد AstSFormatF{nodep->fileline(), smt_عبارت, خطأ, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        أرجع newp;
    }

    Ast_گره_عبارت* editSingle(File_سطر* fl, Ast_گره* itemsp) {
        إذا (!itemsp) أرجع اشاره_تهی;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = درست;

        {
            AstBegin* ثابت tempp
                = جديد AstBegin{fl, "[EditWrapper]", itemsp->unlink_Fr_پشت_With_بعدی(), خطأ};
            VL_DO_DANGLING(iterate_And_بعدی_Null(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            إذا (itemsp) itemsp->unlink_Fr_پشت_With_بعدی();
            VL_DO_DANGLING(tempp->delete_درخت(), tempp);
        }
        إذا (!itemsp) أرجع اشاره_تهی;

        Ast_گره_عبارت* ثابت exprsp = VN_CAST(itemsp, گره_عبارت);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        إذا (!exprsp->nextp()) أرجع exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        لكل (Ast_گره* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        أرجع جديد AstSFormatF{fl, fmt.str(), خطأ, exprsp};
    }

    Ast_گره_عبارت* newSel(File_سطر* fl, Ast_گره_عبارت* arrayp, Ast_گره_عبارت* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_گره_DType* ثابت arrDtp = arrayp->unlink_Fr_پشت()->dtypep()->skipRefp();
        Ast_گره_عبارت* selp = اشاره_تهی;
        إذا (VN_IS(arrDtp, صف_DType) || VN_IS(arrDtp, DynArrayDType))
            selp = جديد AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        وإلا إذا (VN_IS(arrDtp, UnpackArrayDType))
            selp = جديد AstArraySel{fl, arrayp, idxp};
        وإلا إذا (VN_IS(arrDtp, AssocArrayDType))
            selp = جديد AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        أرجع selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_گره_عبارت* adjust_عرض(File_سطر* fl, Ast_گره_عبارت* exprp, صحيح هدف_عرض,
                             VSigning هدف_Signing) {
        ثابت صحيح عبارت_عرض = exprp->عرض();
        إذا (هدف_عرض > عبارت_عرض) {
            // Extend to match target width
            Ast_گره_عبارت* ثابت نتیجه = جديد AstExtend{fl, exprp, هدف_عرض};
            نتیجه->dtypeChgSigned(هدف_Signing);
            أرجع نتیجه;
        } وإلا إذا (هدف_عرض < عبارت_عرض) {
            // Truncate to match target width
            Ast_گره_عبارت* ثابت نتیجه = جديد AstSel{fl, exprp, 0, هدف_عرض};
            نتیجه->dtypeChgSigned(هدف_Signing);
            أرجع نتیجه;
        } وإلا {
            // Width already matches
            أرجع exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_گره_عبارت* استخراج_Return_عبارت(AstFunc* funcp) {
        AstVar* ثابت retVarp = VN_CAST(funcp->fvarp(), Var);
        إذا (!retVarp) أرجع اشاره_تهی;
        Ast_گره_عبارت* retExprp = اشاره_تهی;
        صحيح تخصیص_بده_شمار = 0;
        funcp->foreach([&](Ast_تخصیص_بده* assignp) {
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            إذا (!lhsp || lhsp->varp() != retVarp) أرجع;
            retExprp = assignp->rhsp();
            ++تخصیص_بده_شمار;
        });
        // Simple function: initializer + return (2) or just return (1)
        إذا (تخصیص_بده_شمار > 2 || !retExprp) أرجع اشاره_تهی;
        أرجع retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    فراغ propagateUser1InlineRecurse(Ast_گره_عبارت* nodep) {
        إذا (VN_IS(nodep, گره_Var_Ref)) {
            nodep->user1(VN_AS(nodep, گره_Var_Ref)->varp()->rand().isRandomizable());
            أرجع;
        }
        إذا (VN_IS(nodep, Const)) {
            nodep->user1(خطأ);
            أرجع;
        }
        صحيح any_فرزند = خطأ;  // Used as bool
        إذا (Ast_گره_عبارت* ثابت cp = VN_CAST(nodep->op1p(), گره_عبارت)) {
            propagateUser1InlineRecurse(cp);
            any_فرزند |= cp->user1();
        }
        إذا (Ast_گره_عبارت* ثابت cp = VN_CAST(nodep->op2p(), گره_عبارت)) {
            propagateUser1InlineRecurse(cp);
            any_فرزند |= cp->user1();
        }
        إذا (Ast_گره_عبارت* ثابت cp = VN_CAST(nodep->op3p(), گره_عبارت)) {
            propagateUser1InlineRecurse(cp);
            any_فرزند |= cp->user1();
        }
        إذا (Ast_گره_عبارت* ثابت cp = VN_CAST(nodep->op4p(), گره_عبارت)) {
            propagateUser1InlineRecurse(cp);
            any_فرزند |= cp->user1();
        }
        nodep->user1(any_فرزند);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ساكن Ast_گره* prependDistPreamble(AstConstraintForeach* nodep, Ast_گره* bodyp) {
        Ast_گره* ثابت preamblep = nodep->user3p();
        إذا (!preamblep) أرجع bodyp;
        nodep->user3p(اشاره_تهی);
        preamblep->جمع_کن_بعدی(bodyp);
        أرجع preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    منطقي referencesForeachIdx(ثابت Ast_گره* nodep) ثابت {
        إذا (m_foreachIdxVars.خالی()) أرجع خطأ;
        أرجع nodep->exists(
            [&](ثابت AstVarRef* refp) { أرجع m_foreachIdxVars.شمار(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    فراغ جمع_کن_Registration_Stmt(Ast_گره* ثابت stmtp, Ast_گره_FTask* ثابت initTaskp) {
        إذا (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->جمع_کن_Bodyp(stmtp);
        } وإلا {
            initTaskp->جمع_کن_Stmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_سطر* ثابت fl, Ast_گره_عبارت* ثابت arrExprp,
                                       Ast_گره_عبارت* ثابت idxExprp) {
        Ast_گره_عبارت* ثابت argsp = Ast_گره::جمع_کن_بعدی(arrExprp, idxExprp);
        AstSFormatF* ثابت formatp = جديد AstSFormatF{fl, "(select %s %s)", خطأ, argsp};
        أرجع formatp;
    }

    فراغ مجموعه_Rand_نما(AstVar* ثابت varp, AstMemberSel* ثابت memberselp,
                     ثابت std::string& smtName, ثابت Randomize_نما& rand_نما,
                     Ast_گره_FTask* ثابت initTaskp) {
        Ast_گره_Module* ثابت varClassp = VN_AS(varp->user2p(), گره_Module);
        AstVar* ثابت sub_Rand_نما_Varp = get_Rand_نما_Var_From_Class(varClassp);
        إذا (sub_Rand_نما_Varp) {
            Ast_گره_عبارت* ثابت والد_Accessp = memberselp->fromp()->clone_درخت(خطأ);
            AstMemberSel* ثابت rand_نما_Selp
                = جديد AstMemberSel{varp->fileline(), والد_Accessp, sub_Rand_نما_Varp};
            rand_نما_Selp->dtypep(sub_Rand_نما_Varp->dtypep());
            AstCMethodHard* ثابت atp
                = جديد AstCMethodHard{varp->fileline(), rand_نما_Selp, VCMethod::ARRAY_AT,
                                     جديد AstConst{varp->fileline(), rand_نما.شاخص}};
            atp->dtype_مجموعه_UInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ثابت enablep = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtype_مجموعه_Void();
            Ast_گره_عبارت* ثابت nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : جديد AstSFormatF{varp->fileline(), smtName, خطأ, اشاره_تهی};
            nameExprp->dtypep(varp->dtypep());
            enablep->جمع_کن_Pinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ثابت disablep = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_مجموعه_VAR_DISABLED};
            disablep->dtype_مجموعه_Void();
            Ast_گره_عبارت* ثابت disnp = nameExprp->clone_درخت(خطأ);
            disablep->جمع_کن_Pinsp(disnp);
            AstIf* ثابت ifp
                = جديد AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            جمع_کن_Registration_Stmt(ifp, initTaskp);
        }
    }

    فراغ markRandc(AstVar* ثابت varp, ثابت std::string& smtName, Ast_گره_FTask* ثابت initTaskp) {
        AstCMethodHard* ثابت markp = جديد AstCMethodHard{
            varp->fileline(),
            جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtype_مجموعه_Void();
        Ast_گره_عبارت* ثابت nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : جديد AstSFormatF{varp->fileline(), smtName, خطأ, اشاره_تهی};
        nameExprp->dtypep(varp->dtypep());
        markp->جمع_کن_Pinsp(nameExprp);
        جمع_کن_Registration_Stmt(markp->makeStmt(), initTaskp);
    }

    Ast_گره_Module* getLeftmostVarModulep(AstMemberSel* ثابت memberselp,
                                         AstVar* ثابت varp) ثابت {
        إذا (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_گره* چپ_Most_Nodep = memberselp->fromp();
            طالما (AstMemberSel* ثابت nestedMemberSelp = VN_CAST(چپ_Most_Nodep, MemberSel)) {
                چپ_Most_Nodep = nestedMemberSelp->fromp();
            }
            إذا (Ast_گره_Var_Ref* ثابت ریشه_Var_Refp = VN_CAST(چپ_Most_Nodep, گره_Var_Ref)) {
                أرجع VN_AS(ریشه_Var_Refp->varp()->user2p(), گره_Module);
            }
            أرجع VN_AS(memberselp->user2p(), گره_Module);
        }
        أرجع VN_AS(varp->user2p(), گره_Module);
    }

    Ast_گره_FTask* getInitTaskp(AstVar* ثابت varp, ثابت منطقي memberselOrStructSel,
                               Ast_گره_Module* ثابت classp) ثابت {
        إذا (m_inlineInitTaskp) أرجع m_inlineInitTaskp;
        varp->user3(درست);
        إذا (memberselOrStructSel) {
            إذا (Ast_گره_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.پیدا_کن_Member(classp, "randomize"), گره_FTask)) {
                أرجع initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            إذا (m_memberselInitTaskp) أرجع m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } وإلا {
            Ast_گره_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.پیدا_کن_Member(classp, "new"), گره_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            أرجع initTaskp;
        }
        أرجع اشاره_تهی;
    }

    فراغ createWriteVarForClassFields(AstClass* ثابت elemClassp, ثابت std::string& smtName,
                                      AstVar* ثابت iterVarp, ثابت منطقي isUnpackedClassRefArray,
                                      Ast_گره_Module* ثابت classOrPackagep, AstVar* ثابت varp,
                                      AstIf* ثابت ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ثابت {
        File_سطر* ثابت fl = varp->fileline();
        Ast_گره_Module* ثابت varClassp = VN_AS(varp->user2p(), گره_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr صحيح idx_عرض = 32;
        ثابت صحيح fmt_عرض = VL_واژگان_I(idx_عرض) * 8;

        لكل (ثابت AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : اشاره_تهی) {
            لكل (Ast_گره* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ثابت memberVarp = VN_CAST(mnodep, Var);
                إذا (!memberVarp || !memberVarp->rand().isRandomizable()) استمر;
                Ast_گره_DType* ثابت memberDtp = memberVarp->dtypep()->skipRefp();
                إذا (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) استمر;
                ثابت صحيح member_عرض = memberDtp->عرض();

                AstCStmt* ثابت fmtp = جديد AstCStmt{fl};
                إذا (isUnpackedClassRefArray) {
                    fmtp->جمع_کن("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } وإلا {
                    fmtp->جمع_کن("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::مقصد_string(fmt_عرض) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->جمع_کن(جديد AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->جمع_کن(");\n");
                ifNonNullp->جمع_کن_Thensp(fmtp);

                AstVarRef* ثابت arrayWrRefp = جديد AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_گره_عبارت* atWritep;
                إذا (isUnpackedClassRefArray) {
                    atWritep = جديد AstArraySel{fl, arrayWrRefp,
                                               جديد AstVarRef{fl, iterVarp, VAccess::READ}};
                } وإلا {
                    atWritep = جديد AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  جديد AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* ثابت memberSelp = جديد AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* ثابت writeVarp
                    = جديد AstCMethodHard{fl,
                                         جديد AstVarRef{fl, VN_AS(m_genp->user2p(), گره_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtype_مجموعه_Void();
                writeVarp->جمع_کن_Pinsp(memberSelp);
                writeVarp->جمع_کن_Pinsp(
                    جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(member_عرض)});
                AstCExpr* ثابت nameRefp = جديد AstCExpr{fl, AstCExpr::خالص{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->جمع_کن_Pinsp(nameRefp);
                writeVarp->جمع_کن_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->جمع_کن_Thensp(writeVarp->makeStmt());
            }
        }
    }

    فراغ createSolverArrayHandle(AstVar* ثابت varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_گره_Module* ثابت classOrPackagep,
                                 ثابت منطقي isUnpackedClassRefArray,
                                 ثابت std::string& smtName) ثابت {
        File_سطر* ثابت fl = varp->fileline();
        AstClass* ثابت elemClassp = elemClassRefDtp->classp();
        Ast_گره_Module* ثابت varClassp = VN_AS(varp->user2p(), گره_Module);

        AstVar* ثابت iterVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, m_unique_نامها.get("__Vi"),
                                            varp->پیدا_کن_UInt32_DType()};
        iterVarp->funcLocal(درست);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_گره* ثابت stmtsp
            = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE}, جديد AstConst{fl, 0}};

        Ast_گره_عبارت* sizep;
        إذا (isUnpackedClassRefArray) {
            ثابت صحيح array_اندازه_2
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = جديد AstConst{fl, static_cast<uint32_t>(array_اندازه_2)};
        } وإلا {
            AstVarRef* ثابت array_اندازه_Refp = جديد AstVarRef{fl, varClassp, varp, VAccess::READ};
            array_اندازه_Refp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ثابت dynSizep
                = جديد AstCMethodHard{fl, array_اندازه_Refp, VCMethod::DYN_اندازه, اشاره_تهی};
            dynSizep->dtype_مجموعه_UInt32();
            sizep = dynSizep;
        }

        AstLoop* ثابت loopp = جديد AstLoop{fl};
        stmtsp->جمع_کن_بعدی(loopp);
        loopp->جمع_کن_Stmtsp(جديد AstLoopTest{
            fl, loopp, جديد AstLt{fl, جديد AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ثابت arrayReadRefp = جديد AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_گره_عبارت* atReadp;
        إذا (isUnpackedClassRefArray) {
            atReadp
                = جديد AstArraySel{fl, arrayReadRefp, جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        } وإلا {
            atReadp = جديد AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ثابت ifNonNullp
            = جديد AstIf{fl, جديد AstNeq{fl, atReadp, جديد AstConst{fl, AstConst::Null{}}}, اشاره_تهی};
        loopp->جمع_کن_Stmtsp(ifNonNullp);

        AstCStmt* ثابت bufDeclp = جديد AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->جمع_کن_Thensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
            fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
            جديد Ast_جمع_کن{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(درست);
        Ast_گره_FTask* initTaskp = m_inlineInitTaskp;
        إذا (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.پیدا_کن_Member(varClassp, "new"), گره_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->جمع_کن_Stmtsp(stmtsp);
    }

    فراغ createSolverVarHandle(AstVar* ثابت varp, ثابت منطقي structSelOrCMeth,
                               ثابت منطقي isGlobalConstrained, ثابت Randomize_نما rand_نما,
                               AstMemberSel* ثابت memberselp, ثابت std::string& smtName,
                               Ast_گره_Module* ثابت classOrPackagep, Ast_گره_Module* ثابت classp,
                               Ast_گره_FTask* ثابت initTaskp) {
        uint32_t unpackedDims = 0;
        إذا (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(خطأ).دومی;
        }
        إذا (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ثابت methodp = جديد AstCMethodHard{
            varp->fileline(),
            جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_مجموعه_Void();
        إذا (memberselp) {
            methodp->جمع_کن_Pinsp(memberselp);
        } وإلا {
            AstVarRef* ثابت varRefp
                = جديد AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->جمع_کن_Pinsp(varRefp);
        }
        ثابت اندازه_t عرض = arrayElementDTypep(varp->dtypep())->عرض();
        methodp->جمع_کن_Pinsp(جديد AstConst{varp->fileline(), AstConst::Unsized64{}, عرض});
        Ast_گره_عبارت* ثابت varnamep = جديد AstCExpr{varp->fileline(), AstCExpr::خالص{},
                                                   "\"" + smtName + "\"", varp->عرض()};
        varnamep->dtypep(varp->dtypep());
        methodp->جمع_کن_Pinsp(varnamep);
        methodp->جمع_کن_Pinsp(
            جديد AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        إذا (rand_نما.استفاده_میکند_نما && !(isGlobalConstrained && memberselp)) {
            methodp->جمع_کن_Pinsp(
                جديد AstConst{varp->fileline(), AstConst::Unsized64{}, rand_نما.شاخص});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->جمع_کن_Stmtsp(methodp->makeStmt());
        إذا (varp->lifetime().isStatic() && rand_نما.استفاده_میکند_نما) {
            AstCMethodHard* ثابت markp = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_گره_عبارت* ثابت namep = جديد AstCExpr{varp->fileline(), AstCExpr::خالص{},
                                                    "\"" + smtName + "\"", varp->عرض()};
            namep->dtypep(varp->dtypep());
            markp->جمع_کن_Pinsp(namep);
            markp->dtype_مجموعه_Void();
            initTaskp->جمع_کن_Stmtsp(markp->makeStmt());
        }
        إذا (isGlobalConstrained && memberselp && rand_نما.استفاده_میکند_نما) {
            مجموعه_Rand_نما(varp, memberselp, smtName, rand_نما, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        إذا (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_گره_عبارت* createVarExprp(Ast_گره_Var_Ref* nodep, ثابت Randomize_نما rand_نما,
                                ثابت std::string& smtName, ثابت منطقي isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ثابت varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_گره_عبارت* exprp = جديد AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, خطأ, اشاره_تهی};
        إذا (!rand_نما.استفاده_میکند_نما) {
            // Global constraints keep nodep alive for write_var processing
            إذا (!isGlobalConstrained) VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_گره_عبارت* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_درخت(خطأ)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_گره_عبارت* rand_نما_Access;
        ثابت منطقي varIsStatic = varp->lifetime().isStatic();
        AstClass* ثابت varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : اشاره_تهی;
        AstVar* ثابت owner_Static_Rand_نما_Varp
            = varOwningClassp ? پیدا_کن_Static_Rand_نما_Var_Member(varOwningClassp) : اشاره_تهی;
        إذا (owner_Static_Rand_نما_Varp) {
            rand_نما_Access = جديد AstVarRef{varp->fileline(),
                                           VN_AS(owner_Static_Rand_نما_Varp->user2p(), گره_Module),
                                           owner_Static_Rand_نما_Varp, VAccess::READ};
        } وإلا إذا (memberselp) {
            Ast_گره_Module* ثابت varClassp = VN_AS(varp->user2p(), گره_Module);
            AstVar* ثابت effective_Rand_نما_Varp = get_Rand_نما_Var_From_Class(varClassp);
            إذا (effective_Rand_نما_Varp) {
                // Member's class has randmode, use it
                Ast_گره_عبارت* والد_Access = memberselp->fromp()->clone_درخت(خطأ);
                AstMemberSel* rand_نما_Sel
                    = جديد AstMemberSel{varp->fileline(), والد_Access, effective_Rand_نما_Varp};
                rand_نما_Sel->dtypep(effective_Rand_نما_Varp->dtypep());
                rand_نما_Access = rand_نما_Sel;
            } وإلا {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_rand_نما_Varp, nodep, "No m_randModeVarp");
                rand_نما_Access
                    = جديد AstVarRef{varp->fileline(), VN_AS(m_rand_نما_Varp->user2p(), گره_Module),
                                    m_rand_نما_Varp, VAccess::READ};
            }
        } وإلا {
            UASSERT_OBJ(m_rand_نما_Varp, nodep, "No m_randModeVarp");
            rand_نما_Access
                = جديد AstVarRef{varp->fileline(), VN_AS(m_rand_نما_Varp->user2p(), گره_Module),
                                m_rand_نما_Varp, VAccess::READ};
        }

        AstCMethodHard* ثابت atp
            = جديد AstCMethodHard{nodep->fileline(), rand_نما_Access, VCMethod::ARRAY_AT,
                                 جديد AstConst{nodep->fileline(), rand_نما.شاخص}};
        atp->dtype_مجموعه_UInt32();
        exprp = جديد AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(درست);  // Mark as formatted
        أرجع exprp;
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        إذا (varp->user4p()) {
            منطقي is_اندازه_Ref = خطأ;
            إذا (AstCMethodHard* ثابت methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                إذا (methodp->روش() == VCMethod::ASSOC_اندازه
                    || methodp->روش() == VCMethod::DYN_اندازه) {
                    is_اندازه_Ref = درست;
                }
            }
            إذا (!is_اندازه_Ref && m_اندازه_Constrained_Arraysp) { m_اندازه_Constrained_Arraysp->درج(varp); }
        }

        // Check if this variable is marked as globally constrained
        ثابت منطقي isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = اشاره_تهی;
        منطقي structSelOrCMeth = خطأ;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_گره_DType* selectedDtypep = اشاره_تهی;
        std::string smtName;
        إذا (m_nestedAccess) {
            m_nestedAccess->جمع_کن_Var_Name_Part(
                جديد AstSFormatF{varp->fileline(), nodep->name(), خطأ, اشاره_تهی}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_درخت();
        } وإلا إذا (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_گره* بالاترین_Member_Sel = nodep->backp();
            طالما (VN_IS(بالاترین_Member_Sel->backp(), MemberSel)) {
                بالاترین_Member_Sel = بالاترین_Member_Sel->backp();
            }
            memberselp = VN_AS(بالاترین_Member_Sel, MemberSel)->clone_درخت(خطأ);
            smtName = build_Member_مسیر(memberselp);
        } وإلا إذا (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_گره* بالاترین_Nodep = nodep->backp();
            طالما (VN_IS(بالاترین_Nodep->backp(), StructSel) || VN_IS(بالاترین_Nodep->backp(), CMethodHard)) {
                بالاترین_Nodep = بالاترین_Nodep->backp();
            }

            إذا (VN_IS(بالاترین_Nodep, StructSel) || VN_IS(بالاترین_Nodep, CMethodHard)) {
                structSelOrCMeth = درست;
                selectedDtypep = VN_AS(بالاترین_Nodep, گره_عبارت)->dtypep();
            }

            memberselp = VN_CAST(بالاترین_Nodep, MemberSel);
            إذا (memberselp) smtName = build_Member_مسیر(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            إذا (smtName == "") smtName = nodep->name();
        } وإلا {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        إذا (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        إذا (dtype_شامل_است_Real(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            أرجع;
        }
        Ast_گره_Module* ثابت classOrPackagep = nodep->classOrPackagep();
        ثابت Randomize_نما rand_نما = {.asUQuad = varp->user1()};
        إذا (!rand_نما.استفاده_میکند_نما && editFormat(nodep)) أرجع;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
        إذا (enumDtp
            && (std::پیدا_کن(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.انتها(), varp)
                == m_rangeConstrainedEnums.انتها())) {
            AstVar* genVarp = m_genp;
            إذا (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_گره_عبارت* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_گره_FTask* هدف_Taskp = m_inlineInitTaskp;
            إذا (!هدف_Taskp) هدف_Taskp = get_Create_Constraint_راهاندازی_Func(m_classp, m_memberMap);
            UASSERT_OBJ(هدف_Taskp, nodep, "No function to inline enum range constraint into");
            هدف_Taskp->جمع_کن_Stmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.افزودن_پشت(varp);
        }

        VNRelinker relinker;
        nodep->unlink_Fr_پشت(&relinker);
        Ast_گره_عبارت* ثابت exprp
            = createVarExprp(nodep, rand_نما, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ثابت منطقي alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.شمار(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.شمار(smtName) > 0
                                    : memberselp          ? m_writtenVars.شمار(smtName) > 0
                                                          : varp->user3();
        ثابت منطقي shouldWriteVar = !alreadyWritten;
        إذا (shouldWriteVar) {
            // Track this variable path as written
            إذا (m_inlineInitTaskp)
                m_inlineWrittenVars.درج(smtName);
            وإلا إذا (isGlobalConstrained || memberselp)
                m_writtenVars.درج(smtName);
            // For global constraints, delete nodep after processing
            إذا (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);

            منطقي isClassRefArray = خطأ;
            منطقي isUnpackedClassRefArray = خطأ;
            AstClassRefDType* elemClassRefDtp = اشاره_تهی;
            {
                Ast_گره_DType* varDtp = varp->dtypep()->skipRefp();
                إذا (varDtp->isNonPackedArray()) {
                    Ast_گره_DType* ثابت elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    إذا (elemClassRefDtp) {
                        isClassRefArray = درست;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_گره_Module* ثابت classp = getLeftmostVarModulep(memberselp, varp);
            Ast_گره_FTask* ثابت initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            إذا (m_nestedAccess) {
                Ast_Stmt_عبارت* ثابت writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                جمع_کن_Registration_Stmt(writeVarStmtp, initTaskp);
                إذا (isGlobalConstrained && memberselp && rand_نما.استفاده_میکند_نما) {
                    مجموعه_Rand_نما(varp, memberselp, smtName, rand_نما, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                إذا (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } وإلا إذا (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } وإلا {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, rand_نما,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } وإلا {
            // Variable already written, clean up cloned memberselp if any
            إذا (memberselp) VL_DO_DANGLING(memberselp->delete_درخت(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            إذا (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        إذا (m_nestedAccess) { VL_DO_DANGLING(حذف m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_گره_عبارت* build_شمار_Ones_Expansion(File_سطر* fl, Ast_گره_عبارت* argp) {
        ثابت V3_عدد رقم_One{fl, argp->عرض(), 1};
        Ast_گره_عبارت* sump = جديد AstAnd{fl, argp, جديد AstConst{fl, رقم_One}};
        sump->user1(درست);
        لكل (صحيح i = 1; i < argp->عرض(); i++) {
            V3_عدد رقم_بیت_ماسک{fl, argp->عرض(), 0};
            رقم_بیت_ماسک.مجموعه_بیت(i, 1);
            AstAnd* ثابت andp
                = جديد AstAnd{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, رقم_بیت_ماسک}};
            andp->user1(درست);
            Ast_لغزش_R* ثابت shiftp
                = جديد Ast_لغزش_R{fl, andp,
                                جديد AstConst{fl, AstConst::Widthed_مقدار{}, argp->عرض(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(درست);
            sump = جديد Ast_جمع_کن{fl, sump, shiftp};
            sump->user1(درست);
        }
        أرجع sump;
    }

    فراغ بازدید(Ast_شمار_Ones* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlink_Fr_پشت();
        Ast_گره_عبارت* sump = build_شمار_Ones_Expansion(fl, argp);
        sump = adjust_عرض(fl, sump, nodep->عرض(), nodep->dtypep()->numeric());
        sump->user1(درست);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(sump);
    }
    فراغ بازدید(AstRedXor* nodep) override {
        إذا (editFormat(nodep)) أرجع;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlink_Fr_پشت();

        Ast_گره_عبارت* redxorp = جديد AstSel{fl, argp, 0, 1};
        redxorp->user1(درست);
        لكل (صحيح i = 1; i < argp->عرض(); i++) {
            AstSel* ثابت selp = جديد AstSel{fl, argp->clone_درخت_خالص(خطأ), i, 1};
            selp->user1(درست);

            redxorp = جديد AstXor{fl, redxorp, selp};
            redxorp->user1(درست);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(redxorp);
    }
    فراغ بازدید(AstRedAnd* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // Convert to (~x == 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlink_Fr_پشت();
        ثابت V3_عدد رقم_Zero{fl, argp->عرض(), 0};
        Ast_گره_عبارت* ثابت negp = جديد AstNot{fl, argp};
        negp->user1(درست);
        Ast_گره_عبارت* ثابت eqp = جديد AstEq{fl, negp, جديد AstConst{fl, رقم_Zero}};
        eqp->user1(درست);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(eqp);
    }
    فراغ بازدید(AstRedOr* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // Convert to (x != 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlink_Fr_پشت();
        ثابت V3_عدد رقم_Zero{fl, argp->عرض(), 0};
        Ast_گره_عبارت* ثابت neqp = جديد AstNeq{fl, argp, جديد AstConst{fl, رقم_Zero}};
        neqp->user1(درست);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(neqp);
    }
    فراغ بازدید(AstOneHot* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlink_Fr_پشت();
        ثابت صحيح w = argp->عرض();

        ثابت V3_عدد رقم_Zero{fl, w, 0};
        AstNeq* ثابت neZerop
            = جديد AstNeq{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, رقم_Zero}};
        neZerop->user1(درست);

        ثابت V3_عدد رقم_One{fl, w, 1};
        AstSub* ثابت subp = جديد AstSub{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, رقم_One}};
        subp->user1(درست);

        AstAnd* ثابت andp = جديد AstAnd{fl, argp, subp};
        andp->user1(درست);

        ثابت V3_عدد رقم_Zero2{fl, w, 0};
        AstEq* ثابت eqZerop = جديد AstEq{fl, andp, جديد AstConst{fl, رقم_Zero2}};
        eqZerop->user1(درست);

        AstLogAnd* ثابت resultp = جديد AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(درست);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(resultp);
    }
    فراغ بازدید(AstOneHot0* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $onehot0(x) = (x & (x-1)) == 0
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlink_Fr_پشت();
        ثابت صحيح w = argp->عرض();

        ثابت V3_عدد رقم_One{fl, w, 1};
        AstSub* ثابت subp = جديد AstSub{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, رقم_One}};
        subp->dtypeFrom(argp);
        subp->user1(درست);

        AstAnd* ثابت andp = جديد AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(درست);

        ثابت V3_عدد رقم_Zero{fl, w, 0};
        AstEq* ثابت eqp = جديد AstEq{fl, andp, جديد AstConst{fl, رقم_Zero}};
        eqp->user1(درست);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(eqp);
    }
    فراغ بازدید(Ast_شمار_بیتها* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سطر* ثابت fl = nodep->fileline();

        منطقي شمار_Ones = خطأ;
        منطقي شمار_Zeros = خطأ;
        لكل (Ast_گره_عبارت* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ثابت AstConst* ثابت cp = VN_CAST(ctrlp, Const);
            إذا (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ثابت zerop
                    = جديد AstConst{fl, AstConst::Widthed_مقدار{}, nodep->عرض(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                أرجع;
            }
            إذا (cp->رقم().بیت_Is1(0) && !شمار_Ones)
                شمار_Ones = درست;
            وإلا إذا (cp->رقم().بیت_Is0(0) && !شمار_Zeros)
                شمار_Zeros = درست;
        }

        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlink_Fr_پشت();
        ثابت صحيح arg_عرض = argp->عرض();
        Ast_گره_عبارت* sump = اشاره_تهی;
        إذا (شمار_Ones && شمار_Zeros) {
            // ones + zeros = width for 2-state types
            sump = جديد AstConst{fl, AstConst::Widthed_مقدار{}, nodep->عرض(), (uint32_t)arg_عرض};
            VL_DO_DANGLING(argp->delete_درخت(), argp);
        } وإلا إذا (شمار_Ones) {
            sump = build_شمار_Ones_Expansion(fl, argp);
        } وإلا إذا (شمار_Zeros) {
            // width - countones(x)
            Ast_گره_عبارت* ثابت onesCountp = build_شمار_Ones_Expansion(fl, argp);
            ثابت V3_عدد عرض_Val{nodep, onesCountp->عرض(), (uint32_t)arg_عرض};
            sump = جديد AstSub{fl, جديد AstConst{fl, عرض_Val}, onesCountp};
            sump->user1(درست);
        } وإلا {
            sump = جديد AstConst{fl, AstConst::Widthed_مقدار{}, nodep->عرض(), 0U};
            VL_DO_DANGLING(argp->delete_درخت(), argp);
        }

        sump = adjust_عرض(fl, sump, nodep->عرض(), nodep->dtypep()->numeric());
        sump->user1(درست);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(sump);
    }
    فراغ بازدید(AstCLog2* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت argp = nodep->lhsp()->unlink_Fr_پشت();
        ثابت صحيح arg_عرض = argp->عرض();
        ثابت صحيح نتیجه_عرض = nodep->عرض();

        Ast_گره_عبارت* resultp
            = جديد AstConst{fl, AstConst::Widthed_مقدار{}, نتیجه_عرض, (uint32_t)arg_عرض};

        لكل (صحيح k = arg_عرض - 1; k >= 0; k--) {
            V3_عدد آستانه{fl, arg_عرض, 0};
            إذا (k < 32)
                آستانه.مجموعه_Long(1ULL << k);
            وإلا
                آستانه.مجموعه_بیت(k, 1);

            AstLte* ثابت ltep
                = جديد AstLte{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, آستانه}};
            ltep->user1(درست);

            AstConst* ثابت valuep
                = جديد AstConst{fl, AstConst::Widthed_مقدار{}, نتیجه_عرض, (uint32_t)k};

            resultp = جديد AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(درست);
        }

        VL_DO_DANGLING(argp->delete_درخت(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(resultp);
    }
    فراغ بازدید(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ثابت zerop = جديد AstConst{nodep->fileline(), AstConst::بیت_False{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    فراغ handlePow(Ast_گره_Biop* nodep) {
        إذا (AstConst* ثابت exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* ثابت basep = nodep->lhsp();
            ثابت V3_عدد رقم_One{nodep, basep->عرض(), 1};
            Ast_گره_عبارت* powerp = جديد AstConst{fl, رقم_One};
            ثابت منطقي پایه_Signed = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ثابت int32_t exponent = پایه_Signed ? exponentp->مقصد_SInt() : exponentp->مقصد_UInt();
            إذا (exponent > 0) {
                لكل (int32_t i = 0; i < exponent; i++) {
                    إذا (پایه_Signed) {
                        powerp = جديد AstMulS{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    } وإلا {
                        powerp = جديد AstMul{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    }
                    powerp->user1(درست);
                }
            } وإلا إذا (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                إذا (پایه_Signed) {
                    powerp = جديد AstDivS{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    powerp->user1(درست);
                    powerp = جديد AstDivS{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    powerp->user1(درست);
                } وإلا {
                    powerp = جديد AstDiv{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    powerp->user1(درست);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            iterate(powerp);
        } وإلا إذا (ثابت AstConst* ثابت basep = VN_CAST(nodep->lhsp(), Const)) {
            إذا (basep->رقم().مقصد_UInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_سطر* ثابت fl = nodep->fileline();
                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
                ثابت صحيح عرض = nodep->عرض();
                ثابت منطقي rhsDependent = rhsp->user1();
                AstConst* ثابت onep = جديد AstConst{fl, AstConst::Widthed_مقدار{}, عرض, 1};
                Ast_لغزش_L* ثابت shiftp = جديد Ast_لغزش_L{fl, onep, rhsp, عرض};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_گره_عبارت* resultp = shiftp;
                إذا (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_گره_عبارت* ثابت nClonep = rhsp->clone_درخت_خالص(خطأ);
                    nClonep->user1(rhsDependent);
                    AstConst* ثابت zeroNp
                        = جديد AstConst{fl, AstConst::Widthed_مقدار{}, rhsp->عرض(), 0};
                    AstGteS* ثابت condp = جديد AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ثابت zeroRp = جديد AstConst{fl, AstConst::Widthed_مقدار{}, عرض, 0};
                    resultp = جديد AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                iterate(resultp);
            } وإلا {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } وإلا {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    فراغ بازدید(AstPow* nodep) override { handlePow(nodep); }
    فراغ بازدید(AstPowSS* nodep) override { handlePow(nodep); }
    فراغ بازدید(AstPowSU* nodep) override { handlePow(nodep); }
    فراغ بازدید(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    فراغ handle_لغزش(Ast_گره_Biop* nodep) {
        إذا (editFormat(nodep)) أرجع;
        ثابت صحيح lhs_عرض = nodep->lhsp()->عرض();
        ثابت صحيح rhs_عرض = nodep->rhsp()->عرض();
        إذا (rhs_عرض < lhs_عرض) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
            AstExtend* ثابت extendp = جديد AstExtend{fl, rhsp, lhs_عرض};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(درست);
            nodep->rhsp(extendp);
        } وإلا إذا (rhs_عرض > lhs_عرض) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گره_DType* ثابت dtypep = nodep->dtypep();
            Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت extendp
                = VN_IS(nodep, لغزش_RS)
                      ? static_cast<Ast_گره_عبارت*>(جديد AstExtendS{fl, lhsp, rhs_عرض})
                      : static_cast<Ast_گره_عبارت*>(جديد AstExtend{fl, lhsp, rhs_عرض});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(درست);
            nodep->lhsp(extendp);
            nodep->dtype_مجموعه_Logic_Sized(rhs_عرض, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlink_Fr_پشت(&relinker);
            AstSel* ثابت selp = جديد AstSel{fl, nodep, 0, lhs_عرض};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            أرجع;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ بازدید(Ast_لغزش_L* nodep) override { handle_لغزش(nodep); }
    فراغ بازدید(Ast_لغزش_R* nodep) override { handle_لغزش(nodep); }
    فراغ بازدید(Ast_لغزش_RS* nodep) override { handle_لغزش(nodep); }
    فراغ بازدید(Ast_گره_Biop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ بازدید(Ast_گره_Uniop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp());
    }
    فراغ بازدید(Ast_گره_Triop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    فراغ بازدید(AstCond* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        إذا (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtype_مجموعه_String();
            أرجع;
        }
        // Fall back to "(ite cond then else)"
        بازدید(static_cast<Ast_گره_Triop*>(nodep));
    }
    فراغ بازدید(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->srcp());
    }
    فراغ بازدید(AstSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        VNRelinker handle;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت msbp = جديد AstSFormatF{
            fl, "%1d", خطأ,
            جديد Ast_جمع_کن{fl, nodep->lsbp()->clone_درخت_خالص(خطأ),
                       جديد AstConst{fl, static_cast<uint32_t>(nodep->عرض_Const() - 1)}}};
        Ast_گره_عبارت* ثابت lsbp
            = جديد AstSFormatF{fl, "%1d", خطأ, nodep->lsbp()->unlink_Fr_پشت(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    فراغ بازدید(AstStructSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        ثابت منطقي outermost = !m_defer_Rand_نما_Hoist;
        Ast_گره_عبارت* ثابت origp = outermost ? nodep->clone_درخت(خطأ) : اشاره_تهی;
        VL_RESTORER(m_structSel);
        m_structSel = درست;
        إذا (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_گره_عبارت* ثابت fromp = nodep->fromp();
            AstStructDType* ثابت structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            إذا (is_Rand_Qualified_مسیر(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } وإلا {
                structp->markConstrainedRand(درست);
            }
        }
        // Mark Random for structArray
        إذا (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_گره_عبارت* ثابت fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ثابت structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            إذا (is_Rand_Qualified_مسیر(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } وإلا {
                structp->markConstrainedRand(درست);
            }
        }
        {
            VL_RESTORER(m_defer_Rand_نما_Hoist);
            m_defer_Rand_نما_Hoist = درست;
            iterateChildren(nodep);
        }
        File_سطر* ثابت fl = nodep->fileline();
        AstSFormatF* newp = اشاره_تهی;
        إذا (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = جديد AstSFormatF{fl, "%s.%s." + nodep->name(), خطأ,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_درخت_خالص(درست)};
            إذا (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } وإلا {
            newp = جديد AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), خطأ,
                                   اشاره_تهی};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        إذا (origp && !hoist_Rand_نما_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(افزودن_Deletep(origp), origp);
        }
    }
    فراغ جمع_کن_String_Name_Part(Ast_گره_عبارت* ثابت indexp, ثابت std::string& fmt = "%32x") {
        إذا (m_nestedAccess) {
            Ast_گره_عبارت* ثابت bitp = indexp->clone_درخت_خالص(خطأ);
            Ast_گره_عبارت* ثابت بیت_Formatp = جديد AstSFormatF{bitp->fileline(), fmt, خطأ, bitp};
            AstSFormatF* ثابت herep
                = جديد AstSFormatF{indexp->fileline(), "%s.%s", خطأ, بیت_Formatp};
            m_nestedAccess->جمع_کن_Var_Name_Part(herep, indexp->name());
        }
    }
    فراغ بازدید(AstAssocSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سطر* ثابت fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_گره_عبارت* ثابت origp = nodep->clone_درخت(خطأ);
        AstSFormatF* newp = اشاره_تهی;
        إذا (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            جمع_کن_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_گره_عبارت* ثابت idxp = جديد AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), خطأ,
                                                      nodep->bitp()->unlink_Fr_پشت(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وإلا إذا (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ثابت stringp = VN_AS(nodep->bitp(), CvtPackString);
            ثابت اندازه_t string_اندازه = VN_AS(stringp->lhsp(), Const)->عرض();
            إذا (string_اندازه > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << string_اندازه << "bits, limit is 128 bits");
            }
            جمع_کن_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_گره_عبارت* ثابت idxp = جديد AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), خطأ,
                                                      stringp->lhsp()->unlink_Fr_پشت(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وإلا {
            إذا (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ثابت صحيح actual_عرض = nodep->bitp()->عرض();
                std::string fmt;
                // Normalize to standard bit width
                إذا (actual_عرض <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } وإلا إذا (actual_عرض <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } وإلا {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::مقصد_string(VL_واژگان_I(actual_عرض) * 8) + "x";
                }
                إذا (m_nestedAccess) جمع_کن_String_Name_Part(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_گره_عبارت* ثابت idxp
                    = جديد AstSFormatF{fl, fmt, خطأ, nodep->bitp()->unlink_Fr_پشت(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } وإلا {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                إذا (m_nestedAccess) m_nestedAccess->error();
            }
        }
        إذا (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        إذا (!newp || !hoist_Rand_نما_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(origp->delete_درخت(), origp);
        }
    }
    فراغ rename_Array_عبارت(AstSFormatF* ثابت newp) ثابت {
        إذا (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    فراغ بازدید(AstArraySel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سطر* ثابت fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        منطقي شاخص_Is_Rand = خطأ;
        nodep->bitp()->foreach([&](ثابت Ast_گره_Var_Ref* vrefp) {
            إذا (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                شاخص_Is_Rand = درست;
            }
        });
        إذا (m_nestedAccess) {
            Ast_گره_عبارت* ثابت bitp = nodep->bitp()->clone_درخت_خالص(خطأ);
            Ast_گره_عبارت* ثابت بیت_Formatp
                = شاخص_Is_Rand ? جديد AstSFormatF{bitp->fileline(), bitp->name(), خطأ, اشاره_تهی}
                              : جديد AstSFormatF{bitp->fileline(), "%x", خطأ, bitp};
            AstSFormatF* ثابت herep
                = جديد AstSFormatF{nodep->fileline(), "%s.%s", خطأ, بیت_Formatp};
            إذا (AstSel* ثابت selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->جمع_کن_Var_Name_Part(herep, selp->fromp()->name());
            } وإلا {
                m_nestedAccess->جمع_کن_Var_Name_Part(herep, bitp->name());
            }
            إذا (شاخص_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_درخت(), bitp);
                أرجع;
            }
        }
        إذا (شاخص_Is_Rand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_گره_عبارت* indexp = nodep->bitp()->unlink_Fr_پشت(&handle);
            إذا (indexp->عرض() < 32) {
                AstExtend* ثابت extendp = جديد AstExtend{fl, indexp, 32};
                extendp->user1(درست);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_عبارت(newp);
        } وإلا {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_گره_عبارت* ثابت origp = nodep->clone_درخت(خطأ);
            Ast_گره_عبارت* ثابت indexp
                = جديد AstSFormatF{fl, "#x%8x", خطأ, nodep->bitp()->unlink_Fr_پشت(&handle)};
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_عبارت(newp);
            إذا (!newp || !hoist_Rand_نما_Over_Select_And_Member(newp, origp)) {
                VL_DO_DANGLING(origp->delete_درخت(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ساكن منطقي is_Rand_نما_Gate(ثابت Ast_گره_عبارت* exprp) {
        ثابت AstCMethodHard* ثابت atp = VN_CAST(exprp, CMethodHard);
        إذا (!atp || atp->روش() != VCMethod::ARRAY_AT) أرجع خطأ;
        ثابت AstVar* varp = اشاره_تهی;
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } وإلا إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        إذا (!varp) أرجع خطأ;
        ثابت AstClass* ثابت classp = VN_CAST(varp->user2p(), Class);
        أرجع classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    منطقي hoist_Rand_نما_Over_Select_And_Member(AstSFormatF* newp, Ast_گره_عبارت* origp) {
        إذا (m_defer_Rand_نما_Hoist) أرجع خطأ;
        // Only for selects yielding a non-array element (full chains)
        إذا (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::وکتور<AstSFormatF*> قابها;
        AstCond* نما_Condp = اشاره_تهی;
        لكل (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_گره_عبارت* ثابت firstp = curp->exprsp();
            قابها.افزودن_پشت(curp);
            إذا (AstCond* ثابت condp = VN_CAST(firstp, Cond)) {
                إذا (is_Rand_نما_Gate(condp->condp())) نما_Condp = condp;
                اكسر;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        إذا (!نما_Condp) أرجع خطأ;
        File_سطر* ثابت fl = newp->fileline();
        Ast_گره_عبارت* ثابت modep = نما_Condp->condp()->unlink_Fr_پشت();
        Ast_گره_عبارت* activep = نما_Condp->thenp()->unlink_Fr_پشت();
        // Rebuild the select chain text around the SMT name, innermost first
        لكل (خودکار it = قابها.rbegin(); it != قابها.rend(); ++it) {
            Ast_گره_عبارت* ثابت مانده_Argsp = VN_CAST((*it)->exprsp()->nextp(), گره_عبارت);
            إذا (مانده_Argsp) { مانده_Argsp->unlink_Fr_پشت_With_بعدی(); }
            activep = جديد AstSFormatF{fl, (*it)->name(), خطأ,
                                      Ast_گره::جمع_کن_بعدی(activep, مانده_Argsp)};
        }
        AstCond* ثابت hoistp = جديد AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(درست);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(افزودن_Deletep(newp), newp);
        أرجع درست;
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        إذا (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_سطر* ثابت fl = nodep->fileline();
            إذا (m_nestedAccess) {
                AstSFormatF* ثابت formatp = جديد AstSFormatF{fl, nodep->name(), خطأ, اشاره_تهی};
                m_nestedAccess->جمع_کن_Var_Name_Part(جديد AstSFormatF{fl, "%s.%s", خطأ, formatp},
                                               nodep->name());
            }
            Ast_گره* ریشه_گره = nodep->fromp();
            طالما (ثابت AstMemberSel* ثابت selp = VN_CAST(ریشه_گره, MemberSel)) {
                ریشه_گره = selp->fromp();
            }

            Ast_گره_Sel* arraySelp = VN_CAST(ریشه_گره, ArraySel);
            إذا (!arraySelp) arraySelp = VN_CAST(ریشه_گره, AssocSel);

            إذا (arraySelp) {
                Ast_گره_DType* ثابت arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_گره_DType* ثابت elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : اشاره_تهی;
                إذا (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ثابت منطقي isSimple
                        = nodep->fromp() == ریشه_گره && VN_IS(arraySelp->fromp(), VarRef);
                    إذا (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = جديد Nested_Access_مسیر(nodep->clone_درخت(خطأ),
                                                              &m_اولی_عبارت_Inside_Indexp);
                        AstSFormatF* ثابت formatp
                            = جديد AstSFormatF{fl, nodep->name(), خطأ, اشاره_تهی};
                        m_nestedAccess->جمع_کن_Var_Name_Part(
                            جديد AstSFormatF{fl, "%s.%s", خطأ, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = درست;
                    nodep->user1(درست);
                    arraySelp->user1(درست);
                    Ast_گره_عبارت* ثابت origp = nodep->clone_درخت(خطأ);
                    iterateChildren(nodep);
                    AstSFormatF* ثابت newp = جديد AstSFormatF{fl, "%s." + nodep->name(), خطأ,
                                                              nodep->fromp()->unlink_Fr_پشت()};
                    nodep->replaceWith(newp);
                    إذا (!hoist_Rand_نما_Over_Select_And_Member(newp, origp))
                        VL_DO_DANGLING(origp->delete_درخت(), origp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    VL_DO_DANGLING(حذف m_nestedAccess, m_nestedAccess);
                    أرجع;
                }
            }
            // Check if the root variable participates in global constraints
            إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(ریشه_گره, VarRef)) {
                AstVar* ثابت constrainedVar = varRefp->varp();
                إذا (constrainedVar->globalConstrained()) {
                    منطقي ثابت beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    إذا (beforeIterate) {
                        nodep->replaceWith(جديد AstSFormatF{fl, "%s." + nodep->name(), خطأ,
                                                           nodep->fromp()->unlink_Fr_پشت()});
                    } وإلا {
                        nodep->replaceWith(nodep->fromp()->unlink_Fr_پشت());
                    }
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    أرجع;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        إذا (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), گره_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ثابت varRefp
                = جديد AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), گره_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازدید(varRefp);
        } وإلا إذا (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->روش() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ثابت cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_گره_DType* ثابت arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_گره_DType* ثابت elemDtp = arrayDtp->subDTypep()->skipRefp();
            إذا (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = درست;
                iterateChildren(nodep);
                File_سطر* ثابت fl = nodep->fileline();
                AstSFormatF* newp = اشاره_تهی;
                إذا (AstSFormatF* ثابت fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    إذا (fromp->name() == "%s.%s") {
                        newp = جديد AstSFormatF{fl, "%s.%s." + nodep->name(), خطأ,
                                               fromp->exprsp()->clone_درخت_خالص(درست)};
                    } وإلا {
                        newp = جديد AstSFormatF{fl, fromp->name() + "." + nodep->name(), خطأ,
                                               اشاره_تهی};
                    }
                } وإلا {
                    newp = جديد AstSFormatF{fl, nodep->name(), خطأ, اشاره_تهی};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_پشت());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        } وإلا إذا (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_پشت());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        } وإلا {
            editFormat(nodep);
        }
    }
    فراغ بازدید(AstSFormatF* nodep) override {}
    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {}
    فراغ بازدید(AstCStmt* nodep) override {}
    فراغ بازدید(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    فراغ بازدید(AstConstraintIf* nodep) override {
        Ast_گره_عبارت* newp = اشاره_تهی;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت thenp = editSingle(fl, nodep->thensp());
        Ast_گره_عبارت* ثابت elsep = editSingle(fl, nodep->elsesp());
        إذا (thenp && elsep) {
            newp = جديد AstCond{fl, nodep->condp()->unlink_Fr_پشت(), thenp, elsep};
        } وإلا إذا (thenp) {
            newp = جديد AstLogIf{fl, nodep->condp()->unlink_Fr_پشت(), thenp};
        } وإلا إذا (elsep) {
            newp = جديد AstLogIf{fl, جديد AstNot{fl, nodep->condp()->unlink_Fr_پشت()}, elsep};
        }
        إذا (newp) {
            newp->dtype_مجموعه_بیت();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(درست);  // Assume result-dependent
            nodep->replaceWith(جديد Ast_Constraint_عبارت{fl, newp});
        } وإلا {
            nodep->unlink_Fr_پشت();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(Ast_Gen_بلوک* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    فراغ بازدید(AstBegin* nodep) override {}
    فراغ بازدید(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_سطر* ثابت fl = nodep->fileline();

        إذا (!nodep->bodyp()) {
            nodep->unlink_Fr_پشت();
        } وإلا إذا (m_wantSingle) {
            Ast_گره_عبارت* ثابت itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            cstmtp->جمع_کن("ret += \" \";\n");
            cstmtp->جمع_کن("ret += ");
            cstmtp->جمع_کن(itemp);
            cstmtp->جمع_کن(";");
            Ast_گره* ثابت bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->dtype_مجموعه_String();
            cexprp->جمع_کن("([&]{\nstd::string ret;\n");
            cexprp->جمع_کن(جديد AstBegin{fl, m_unique_نامها.get("__VrandBlock"),
                                     جديد AstForeach{fl, nodep->headerp()->unlink_Fr_پشت(), bodyp},
                                     درست});
            cexprp->جمع_کن("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
        } وإلا {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            لكل (Ast_گره* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                إذا (ثابت AstVar* ثابت varp = VN_CAST(elemp, Var)) m_foreachIdxVars.درج(varp);
            }
            iterate_And_بعدی_Null(nodep->bodyp());
            Ast_گره* ثابت bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlink_Fr_پشت_With_بعدی());
            nodep->replaceWith(
                جديد AstBegin{fl, m_unique_نامها.get("__VrandBlock"),
                             جديد AstForeach{fl, nodep->headerp()->unlink_Fr_پشت(), bodyp}, درست});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_Module* ثابت genModp = VN_AS(m_genp->user2p(), گره_Module);

        لكل (Ast_گره_عبارت* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), گره_عبارت)) {
            Ast_گره_عبارت* ثابت lhsTestp = build_Solve_Before_Name_عبارت(fl, lhsp);
            إذا (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                استمر;
            }
            VL_DO_DANGLING(lhsTestp->delete_درخت(), lhsTestp);
            لكل (Ast_گره_عبارت* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), گره_عبارت)) {
                Ast_گره_عبارت* ثابت rhsNamep = build_Solve_Before_Name_عبارت(fl, rhsp);
                إذا (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    استمر;
                }
                AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                    fl, جديد AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtype_مجموعه_Void();
                callp->جمع_کن_Pinsp(build_Solve_Before_Name_عبارت(fl, lhsp));
                callp->جمع_کن_Pinsp(rhsNamep);
                nodep->جمع_کن_اینجا_This_As_بعدی(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
    }
    فراغ بازدید(AstConstraintUnique* nodep) override {
        Ast_گره_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_Module* genModp = اشاره_تهی;

        إذا (m_classp) {
            initTaskp = VN_AS(m_memberMap.پیدا_کن_Member(m_classp, "new"), گره_FTask);
            genVarp = getRandomGenerator(m_classp);
        } وإلا {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        إذا (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            أرجع;
        }
        إذا (m_classp) {
            genModp = VN_AS(genVarp->user2p(), گره_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_گره* راهاندازی_Stmtsp = اشاره_تهی;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::وکتور<std::string> smtExprs;
        لكل (Ast_گره* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            إذا (AstVarRef* ثابت varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ثابت varp = varRefp->varp();
                Ast_گره_Module* ثابت varModp = VN_AS(varp->user2p(), گره_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_گره_DType* ثابت dtypep = varp->dtypep()->skipRefp();

                ثابت ساكن خودکار dynDTypeSupported
                    = [](ثابت Ast_گره_DType* ثابت dtypep) -> منطقي {
                    أرجع VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, صف_DType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elem_عرض;
                uint32_t static_اندازه = 0;  // Element count, zero when only known at run time
                إذا (ثابت AstUnpackArrayDType* ثابت به_بالا = VN_CAST(dtypep, UnpackArrayDType)) {
                    ثابت AstRange* ثابت rangep = به_بالا->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    static_اندازه = به_بالا->elementsConst();
                    إذا (static_اندازه > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        استمر;
                    }
                    elem_عرض = static_cast<uint64_t>(varp->dtypep()->عرض());
                } وإلا إذا (dynDTypeSupported(dtypep)) {
                    elem_عرض = static_cast<uint64_t>(dtypep->subDTypep()->عرض());
                } وإلا {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(خطأ));
                    استمر;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ثابت Ast_گره_DType* ثابت subp = dtypep->subDTypep()->skipRefp();
                إذا (VN_IS(subp, گره_Array_DType) || VN_IS(subp, صف_DType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    استمر;
                }

                إذا (m_wantSingle && !static_اندازه) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    استمر;
                }

                إذا (!varp->user3()) {
                    varp->user3(درست);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_گره_عبارت* ثابت varnamep = جديد AstCExpr{
                        fl, AstCExpr::خالص{}, "\"" + varp->name() + "\"", varp->عرض()};

                    AstCMethodHard* ثابت writeVarCallp = جديد AstCMethodHard{
                        fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ثابت argVarRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                    إذا (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->جمع_کن_Pinsp(argVarRefp);
                    writeVarCallp->جمع_کن_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, elem_عرض});
                    writeVarCallp->جمع_کن_Pinsp(varnamep);
                    writeVarCallp->جمع_کن_Pinsp(جديد AstConst{fl, 1});  // Dimension
                    ثابت Randomize_نما rand_نما = {.asUQuad = varp->user1()};
                    إذا (rand_نما.استفاده_میکند_نما) {
                        writeVarCallp->جمع_کن_Pinsp(
                            جديد AstConst{fl, AstConst::Unsized64{}, rand_نما.شاخص});
                    }
                    writeVarCallp->dtype_مجموعه_Void();
                    initTaskp->جمع_کن_Stmtsp(جديد Ast_Stmt_عبارت{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                إذا (varp->user4p() && m_اندازه_Constrained_Arraysp) {
                    m_اندازه_Constrained_Arraysp->درج(varp);
                }

                إذا (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    إذا (static_اندازه < 2) استمر;
                    std::string عبارت_Str = "(__Vbv (distinct";
                    لكل (uint32_t i = 0; i < static_اندازه; ++i) {
                        عبارت_Str += " (select " + varp->name() + " #x";
                        لكل (صحيح لغزش_2 = 28; لغزش_2 >= 0; لغزش_2 -= 4) {
                            عبارت_Str += "0123456789abcdef"[(i >> لغزش_2) & 0xf];
                        }
                        عبارت_Str += ')';
                    }
                    smtExprs.emplace_پشت(عبارت_Str + "))");
                    استمر;
                }

                Ast_گره_عبارت* ثابت namep = جديد AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ثابت randUniqueCallp
                    = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->پیدا_کن_Void_DType());
                randUniqueCallp->جمع_کن_Pinsp(جديد AstConst{fl, *m_uniqueConstraintId});
                راهاندازی_Stmtsp = Ast_گره::جمع_کن_بعدی(راهاندازی_Stmtsp, جديد Ast_Stmt_عبارت{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        إذا (m_wantSingle && !smtExprs.خالی()) {
            std::string عبارت_Str = smtExprs.front();
            إذا (smtExprs.اندازه() > 1) {
                عبارت_Str = "(bvand";
                لكل (ثابت std::string& oneExprp : smtExprs) عبارت_Str += " " + oneExprp;
                عبارت_Str += ')';
            }
            AstSFormatF* ثابت newp = جديد AstSFormatF{fl, عبارت_Str, خطأ, اشاره_تهی};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (راهاندازی_Stmtsp) nodep->جمع_کن_اینجا_This_As_بعدی(راهاندازی_Stmtsp);
        nodep->unlink_Fr_پشت();
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    فراغ بازدید(Ast_Constraint_عبارت* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_اولی_عبارت_Inside_Indexp);
        إذا (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_گره_عبارت* ثابت exprp = nodep->exprp();
            إذا (exprp->عرض() > 1) {
                File_سطر* ثابت fl = exprp->fileline();
                ثابت V3_عدد رقم_Zero{fl, exprp->عرض(), 0};
                Ast_گره_عبارت* ثابت neqp
                    = جديد AstNeq{fl, exprp->unlink_Fr_پشت(), جديد AstConst{fl, رقم_Zero}};
                neqp->user1(درست);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        إذا (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_پشت());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ثابت VCMethod روش
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                          VAccess::READWRITE},
            روش, nodep->exprp()->unlink_Fr_پشت()};
        callp->dtype_مجموعه_Void();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->جمع_کن_Pinsp(جديد AstCExpr{nodep->fileline(), AstCExpr::خالص{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ثابت uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->جمع_کن_Pinsp(جديد AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string pretty_متن;
        إذا (!v3Global.opt.protectIds()) {
            pretty_متن = nodep->fileline()->pretty_منبع();
            اندازه_t pos = 0;
            طالما ((pos = pretty_متن.پیدا_کن('"', pos)) != std::string::npos) {
                pretty_متن.درج(pos, "\\");
                pos += std::طول_رشته("\\\"");
            }
        }
        callp->جمع_کن_Pinsp(
            جديد AstCExpr{nodep->fileline(), AstCExpr::خالص{}, "\"" + pretty_متن + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstCMethodHard* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سطر* ثابت fl = nodep->fileline();

        إذا (nodep->روش() == VCMethod::ARRAY_AT) {
            جمع_کن_String_Name_Part(nodep->pinsp(), "%x");
            ثابت منطقي شاخص_Is_Rand = nodep->pinsp()->exists([](ثابت Ast_گره_Var_Ref* ثابت vrefp) {
                أرجع vrefp->varp()->rand().isRandomizable();
            });
            إذا (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ثابت Ast_گره* ثابت basep = nodep->پایه_Fromp(درست);
                    إذا (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        أرجع;
                    }
                }
                Ast_گره_عبارت* ثابت origp = شاخص_Is_Rand ? اشاره_تهی : nodep->clone_درخت(خطأ);
                ثابت AstVarRef* ثابت پایه_Refp
                    = m_structSel ? VN_CAST(nodep->fromp()->پایه_Fromp(درست), VarRef) : اشاره_تهی;
                إذا (پایه_Refp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(پایه_Refp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_گره_عبارت* ثابت indexp = nodep->pinsp()->clone_درخت_خالص(خطأ);
                    Ast_گره_عبارت* ثابت arrayp = nodep->fromp()->clone_درخت_خالص(خطأ);
                    AstCMethodHard* ثابت currentSizep
                        = جديد AstCMethodHard{fl, arrayp->clone_درخت_خالص(خطأ), VCMethod::DYN_اندازه};
                    currentSizep->dtype_مجموعه_Int();
                    Ast_گره_عبارت* ثابت درون_Range_Requiredp = جديد AstLogAnd{
                        fl,
                        جديد AstLteS{fl,
                                    جديد AstConst{fl, AstConst::Widthed_مقدار{}, indexp->عرض(), 0},
                                    indexp->clone_درخت_خالص(خطأ)},
                        جديد AstGteS{fl, indexp->clone_درخت_خالص(خطأ), currentSizep}};
                    AstCMethodHard* ثابت resizep
                        = جديد AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             جديد Ast_جمع_کن{fl, indexp, جديد AstConst{fl, 1}}};
                    resizep->dtype_مجموعه_Void();
                    m_prepareConstrainedArraysp->جمع_کن_Stmtsp(
                        جديد AstIf{fl, درون_Range_Requiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ثابت sizep
                    = m_structSel ? جديد AstCMethodHard{fl, nodep->fromp()->clone_درخت_خالص(خطأ),
                                                       VCMethod::DYN_اندازه}
                                  : اشاره_تهی;
                Ast_گره_عبارت* ثابت originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_گره_عبارت* ثابت pinp = nodep->pinsp()->unlink_Fr_پشت();
                AstSFormatF* newp;
                إذا (m_structSel) {
                    إذا (AstSFormatF* ثابت sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_گره_عبارت* ثابت argsp
                        = Ast_گره::جمع_کن_بعدی(nodep->fromp()->unlink_Fr_پشت(), pinp);
                    sizep->dtype_مجموعه_Int();
                    AstLogAnd* ثابت condp
                        = جديد AstLogAnd{fl,
                                        جديد AstLteS{fl,
                                                    جديد AstConst{fl, AstConst::Widthed_مقدار{},
                                                                 originalPinp->عرض(), 0},
                                                    originalPinp->clone_درخت_خالص(خطأ)},
                                        جديد AstLtS{fl, originalPinp->clone_درخت_خالص(خطأ), sizep}};
                    m_conditionp = m_conditionp ? جديد AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = جديد AstSFormatF{fl, "%s.%s", خطأ, argsp};
                } وإلا {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_پشت(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                إذا (origp && !hoist_Rand_نما_Over_Select_And_Member(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_درخت(), origp);
                }

            } وإلا إذا (شاخص_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } وإلا {
                nodep->user1(خطأ);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            أرجع;
        }

        إذا (nodep->روش() == VCMethod::ARRAY_INSIDE) {
            ثابت منطقي randArr = nodep->fromp()->user1();

            AstVar* ثابت newVarp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->پیدا_کن_Int_DType()};
            Ast_گره_عبارت* ثابت idxRefp = جديد AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            Ast_Foreach_سرآیند* ثابت headerp
                = جديد Ast_Foreach_سرآیند{fl, nodep->fromp()->clone_درخت_خالص(خطأ), newVarp};
            Ast_گره_عبارت* ثابت selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_گره* ثابت itemp = جديد AstEq{fl, selp, nodep->pinsp()->unlink_Fr_پشت()};
            itemp->user1(درست);

            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            cstmtp->جمع_کن("ret += \" \";\n");
            cstmtp->جمع_کن("ret += ");
            cstmtp->جمع_کن(iterateSubtreeReturnEdits(itemp));
            cstmtp->جمع_کن(";");
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->dtype_مجموعه_String();
            cexprp->جمع_کن("([&]{\nstd::string ret;\n");
            cexprp->جمع_کن(جديد AstBegin{fl, m_unique_نامها.get("__VrandBlock"),
                                     جديد AstForeach{fl, headerp, cstmtp}, درست});
            cexprp->جمع_کن("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }

        // Array reduction methods (sum, product, and, or, xor)
        إذا (nodep->روش() == VCMethod::ARRAY_R_جمع
            || nodep->روش() == VCMethod::ARRAY_R_حاصلضرب
            || nodep->روش() == VCMethod::ARRAY_R_AND || nodep->روش() == VCMethod::ARRAY_R_OR
            || nodep->روش() == VCMethod::ARRAY_R_XOR) {

            AstWith* ثابت withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            إذا (withp) {
                إذا (AstUnpackArrayDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ثابت صحيح array_اندازه_2 = adtypep->elementsConst();
                    إذا (array_اندازه_2 > v3Global.opt.constraint_Array_حد()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvt_مقصد_Str(array_اندازه_2)
                                          + " exceeds --constraint-array-limit of "
                                          + cvt_مقصد_Str(v3Global.opt.constraint_Array_حد())
                                          + "), treating as state");
                        nodep->user1(خطأ);
                        إذا (editFormat(nodep)) أرجع;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        أرجع;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ثابت منطقي hasRandomArray = nodep->fromp()->user1();
            إذا (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(خطأ);
                إذا (editFormat(nodep)) أرجع;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                أرجع;
            }
            إذا (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                أرجع;
            }

            // Create loop variable and header
            AstVar* ثابت loopVarp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->پیدا_کن_Int_DType()};
            Ast_Foreach_سرآیند* ثابت headerp
                = جديد Ast_Foreach_سرآیند{fl, nodep->fromp()->clone_درخت_خالص(خطأ), loopVarp};

            // Determine SMT operation and compute identity elements
            ثابت حرف* smtOp = اشاره_تهی;
            std::string عنصر_خنثی;
            إذا (withp) {
                // For 'with' clause: use binary format, compute from result width
                ثابت صحيح عرض = nodep->dtypep()->عرض();
                إذا (nodep->روش() == VCMethod::ARRAY_R_جمع) {
                    smtOp = "bvadd";
                    عنصر_خنثی = "#b" + std::string(عرض, '0');
                } وإلا إذا (nodep->روش() == VCMethod::ARRAY_R_حاصلضرب) {
                    smtOp = "bvmul";
                    عنصر_خنثی = (عرض > 0) ? "#b" + std::string(عرض - 1, '0') + "1" : "#b0";
                } وإلا إذا (nodep->روش() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    عنصر_خنثی = "#b" + std::string(عرض, '1');
                } وإلا إذا (nodep->روش() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    عنصر_خنثی = "#b" + std::string(عرض, '0');
                } وإلا {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    عنصر_خنثی = "#b" + std::string(عرض, '0');
                }
            } وإلا {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ثابت arrVarp = arrRefp->varp();
                Ast_گره_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحيح elem_عرض = elemDtp->عرض();
                ثابت صحيح hexDigits = (elem_عرض + 3) / 4;

                ثابت std::string zero_عنصر_خنثی = "#x" + std::string(hexDigits, '0');
                ثابت std::string one_عنصر_خنثی = "#x" + std::string(hexDigits - 1, '0') + "1";
                ثابت std::string all_Ones_عنصر_خنثی = "#x" + std::string(hexDigits, 'f');

                إذا (nodep->روش() == VCMethod::ARRAY_R_جمع) {
                    smtOp = "bvadd";
                    عنصر_خنثی = zero_عنصر_خنثی;
                } وإلا إذا (nodep->روش() == VCMethod::ARRAY_R_حاصلضرب) {
                    smtOp = "bvmul";
                    عنصر_خنثی = one_عنصر_خنثی;
                } وإلا إذا (nodep->روش() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    عنصر_خنثی = all_Ones_عنصر_خنثی;
                } وإلا إذا (nodep->روش() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    عنصر_خنثی = zero_عنصر_خنثی;
                } وإلا {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    عنصر_خنثی = zero_عنصر_خنثی;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            إذا (withp) {
                // With 'with' clause: evaluate expression for each element
                ثابت منطقي randArr = nodep->fromp()->user1();
                Ast_گره_عبارت* ثابت idxRefp = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_گره_عبارت* ثابت elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ثابت صحيح نتیجه_عرض = nodep->dtypep()->عرض();
                ثابت VSigning نتیجه_Signing = nodep->dtypep()->numeric();

                Ast_گره* در_هر_Elem_Exprp = withp->exprp()->clone_درخت_خالص(خطأ);
                إذا (AstLambdaArgRef* ثابت ریشه_Refp = VN_CAST(در_هر_Elem_Exprp, LambdaArgRef)) {
                    إذا (ریشه_Refp->شاخص()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ثابت loopRef = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                        در_هر_Elem_Exprp = adjust_عرض(fl, loopRef, نتیجه_عرض, نتیجه_Signing);
                    } وإلا {
                        // item at root -> replace with array element
                        در_هر_Elem_Exprp = elemSelp->clone_درخت_خالص(خطأ);
                    }
                    VL_DO_DANGLING(ریشه_Refp->delete_درخت(), ریشه_Refp);
                } وإلا {
                    در_هر_Elem_Exprp->foreach([&](AstLambdaArgRef* refp) {
                        إذا (refp->شاخص()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ثابت loopRef = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_گره* ثابت replacement
                                = adjust_عرض(fl, loopRef, نتیجه_عرض, نتیجه_Signing);
                            refp->replaceWith(replacement);
                        } وإلا {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_درخت_خالص(خطأ));
                        }
                        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_درخت(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                در_هر_Elem_Exprp = V3Const::constifyEdit(در_هر_Elem_Exprp);

                در_هر_Elem_Exprp->foreach([&](Ast_گره* nodep) {
                    // Don't mark loop variable references as randomizable
                    إذا (!VN_IS(nodep, Const)) {
                        إذا (Ast_گره_Var_Ref* ثابت vrefp = VN_CAST(nodep, گره_Var_Ref)) {
                            إذا (vrefp->varp() != loopVarp) nodep->user1(درست);
                        } وإلا {
                            nodep->user1(درست);
                        }
                    }
                });

                cstmtp->جمع_کن("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->جمع_کن("ret += ");
                cstmtp->جمع_کن(iterateSubtreeReturnEdits(در_هر_Elem_Exprp));
                cstmtp->جمع_کن(";\n");
                cstmtp->جمع_کن("ret += \")\";\n");
            } وإلا {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ثابت arrVarp = arrRefp->varp();
                ثابت std::string smtArrayName = arrVarp->name();
                Ast_گره_DType* ثابت elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحيح elem_عرض = elemDtp->عرض();
                Ast_گره_Module* ثابت arrModulep = VN_AS(arrVarp->user2p(), گره_Module);
                Ast_گره_Module* ثابت genModulep = VN_AS(m_genp->user2p(), گره_Module);
                arrVarp->user3(درست);

                // Add write_var call to init task
                Ast_گره_عبارت* ثابت arrVarNamep
                    = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + smtArrayName + "\"", elem_عرض};
                AstCMethodHard* ثابت writeVarCallp
                    = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->جمع_کن_Pinsp(جديد AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->جمع_کن_Pinsp(
                    جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elem_عرض)});
                writeVarCallp->جمع_کن_Pinsp(arrVarNamep);
                ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(خطأ).دومی;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->جمع_کن_Pinsp(جديد AstConst{fl, 1});  // Dimension

                ثابت Randomize_نما rand_نما = {.asUQuad = arrVarp->user1()};
                إذا (rand_نما.استفاده_میکند_نما) {
                    writeVarCallp->جمع_کن_Pinsp(
                        جديد AstConst{fl, AstConst::Unsized64{}, rand_نما.شاخص});
                }
                writeVarCallp->dtype_مجموعه_Void();
                Ast_گره_FTask* initTaskp = m_inlineInitTaskp;
                إذا (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.پیدا_کن_Member(arrModulep, "new"), گره_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->جمع_کن_Stmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ثابت idxFormatp = جديد AstSFormatF{
                    fl, "#x%08x", خطأ, جديد AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ثابت varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_پشت(), idxFormatp);

                cstmtp->جمع_کن("ret += \" \";\n");
                cstmtp->جمع_کن("ret += ");
                cstmtp->جمع_کن(varNamep);
                cstmtp->جمع_کن(";\n");
            }

            // Build final expression
            إذا (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
                cexprp->dtype_مجموعه_String();
                cexprp->جمع_کن("([&]{\nstd::string ret = \"" + عنصر_خنثی + "\";\n");
                cexprp->جمع_کن(جديد AstBegin{fl, m_unique_نامها.get("__VrandBlock"),
                                         جديد AstForeach{fl, headerp, cstmtp}, درست});
                cexprp->جمع_کن("return ret;\n})()");
                nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            } وإلا {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
                cexprp->dtype_مجموعه_String();
                cexprp->جمع_کن("([&]{\nstd::string ret;\n");
                cexprp->جمع_کن(جديد AstBegin{fl, m_unique_نامها.get("__VrandBlock"),
                                         جديد AstForeach{fl, headerp, cstmtp}, درست});
                cexprp->جمع_کن("return ret.empty() ? \"" + عنصر_خنثی + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }

        // We've handled DYN_SIZE already if they should be randomized
        إذا (nodep->روش() == VCMethod::DYN_اندازه || nodep->روش() == VCMethod::ASSOC_اندازه) {
            Ast_گره_عبارت* گره_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            طالما (VN_IS(گره_Exprp, MemberSel)) {
                گره_Exprp = VN_AS(گره_Exprp, MemberSel)->fromp();
            }
            إذا (VN_IS(گره_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_درخت_خالص(خطأ)));
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                أرجع;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(خطأ);

        إذا (editFormat(nodep)) أرجع;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        إذا (editFormat(nodep)) أرجع;

        AstFunc* ثابت funcp = VN_CAST(nodep->taskp(), Func);
        إذا (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            أرجع;
        }

        Ast_گره_عبارت* ثابت retExprp = استخراج_Return_عبارت(funcp);
        إذا (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(خطأ);
            إذا (editFormat(nodep)) أرجع;
            أرجع;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), اشاره_تهی, خطأ);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_گره_عبارت* inlinedp = retExprp->clone_درخت_خالص(خطأ);
        لكل (ثابت خودکار& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.اولی;
            AstArg* ثابت argp = tconnect.دومی;
            إذا (!argp || !argp->exprp()) استمر;
            إذا (AstVarRef* ثابت ریشه_Refp = VN_CAST(inlinedp, VarRef)) {
                إذا (ریشه_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_درخت_خالص(خطأ);
                    VL_DO_DANGLING(ریشه_Refp->delete_درخت(), ریشه_Refp);
                    استمر;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                إذا (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_درخت_خالص(خطأ));
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
    فراغ بازدید(Ast_گره_Stmt* nodep) override {}
    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_گره* buildDisableSoftCallStmt(Ast_Constraint_عبارت* nodep) {
        std::string varName;
        إذا (ثابت Ast_گره_Var_Ref* ثابت vrefp = VN_CAST(nodep->exprp(), گره_Var_Ref)) {
            varName = vrefp->name();
        } وإلا إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } وإلا {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), گره_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            جديد AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtype_مجموعه_Void();
        أرجع callp->makeStmt();
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
    Ast_گره* استخراج_Conditional_Disable_Softs(Ast_گره* itemsp, Ast_گره_عبارت* outerCondp) {
        Ast_گره* hoistListp = اشاره_تهی;
        لكل (Ast_گره *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (AstConstraintIf* ثابت ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_سطر* ثابت fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_گره_عبارت* ثابت thenCondp
                    = outerCondp ? static_cast<Ast_گره_عبارت*>(
                                       جديد AstLogAnd{fl, outerCondp->clone_درخت_خالص(خطأ),
                                                     ifp->condp()->clone_درخت_خالص(خطأ)})
                                 : ifp->condp()->clone_درخت_خالص(خطأ);
                إذا (Ast_گره* ثابت thenHoistp
                    = استخراج_Conditional_Disable_Softs(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_گره::جمع_کن_بعدی(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_درخت(), thenCondp);
                إذا (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_گره_عبارت* ثابت notInnerp
                        = جديد AstLogNot{fl, ifp->condp()->clone_درخت_خالص(خطأ)};
                    Ast_گره_عبارت* ثابت elseCondp
                        = outerCondp ? static_cast<Ast_گره_عبارت*>(جديد AstLogAnd{
                                           fl, outerCondp->clone_درخت_خالص(خطأ), notInnerp})
                                     : notInnerp;
                    إذا (Ast_گره* ثابت elseHoistp
                        = استخراج_Conditional_Disable_Softs(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_گره::جمع_کن_بعدی(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_درخت(), elseCondp);
                }
            } وإلا إذا (Ast_Constraint_عبارت* ثابت exprp = VN_CAST(stmtp, Constraint_عبارت)) {
                إذا (exprp->isDisableSoft() && outerCondp) {
                    File_سطر* ثابت fl = exprp->fileline();
                    AstIf* ثابت hoistIfp = جديد AstIf{fl, outerCondp->clone_درخت_خالص(خطأ),
                                                      buildDisableSoftCallStmt(exprp), اشاره_تهی};
                    hoistListp = hoistListp ? Ast_گره::جمع_کن_بعدی(hoistListp,
                                                               static_cast<Ast_گره*>(hoistIfp))
                                            : static_cast<Ast_گره*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_عبارت* ثابت truep
                        = جديد Ast_Constraint_عبارت{fl, جديد AstConst{fl, AstConst::بیت_True{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        أرجع hoistListp;
    }

عام:
    // CONSTRUCTORS
    صریح Constraint_عبارت_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_گره* nodep,
                                   Ast_گره_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* rand_نما_Varp, std::مجموعه<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3_Unique_نامها& unique_نامها,
                                   Ast_گره_FTask* memberselInitTaskp = اشاره_تهی,
                                   VInsertion_مجموعه<AstVar*>* اندازه_Constrained_Arraysp = اشاره_تهی,
                                   Ast_گره_FTask* prepareConstrainedArraysp = اشاره_تهی)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_rand_نما_Varp{rand_نما_Varp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_اندازه_Constrained_Arraysp{اندازه_Constrained_Arraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_unique_نامها{unique_نامها} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        إذا (Ast_گره* ثابت hoistListp
            = nodep ? استخراج_Conditional_Disable_Softs(nodep, اشاره_تهی) : اشاره_تهی) {
            nodep->جمع_کن_بعدی(hoistListp);
        }
        iterate_And_بعدی_Null(nodep);
    }
};

تعداد صنف Capture_نما : uint8_t {
    CAP_NO = 0x0,
    CAP_مقدار = 0x01,
    CAP_THIS = 0x02,
    CAP_F_مجموعه_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
Capture_نما عملگر|(Capture_نما a, Capture_نما b) {
    أرجع static_cast<Capture_نما>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
Capture_نما عملگر&(Capture_نما a, Capture_نما b) {
    أرجع static_cast<Capture_نما>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
Capture_نما نما(Capture_نما a) { أرجع a & static_cast<Capture_نما>(0x3); }
منطقي hasFlags(Capture_نما a, Capture_نما flags) {
    أرجع ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

صنف CaptureVisitor final : عام VNVisitor {
    AstArg* m_argsp = اشاره_تهی;  // Original references turned into arguments
    Ast_گره_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ثابت AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::مجموعه<Ast_گره*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = اشاره_تهی;  // Variable for outer context's object, if necessary
    منطقي m_staticContext = خطأ;  // True when capturing from a static function context

    // METHODS

    منطقي captureVariable(File_سطر* ثابت fileline, Ast_گره_Var_Ref* varrefp, AstVar*& varp) {
        خودکار it = m_varCloneMap.پیدا_کن(varrefp->varp());
        إذا (it == m_varCloneMap.انتها()) {
            AstVar* ثابت newVarp = varrefp->varp()->clone_درخت(خطأ);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(درست);
            newVarp->direction(VDirection::درون_داد);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            أرجع درست;
        }
        varp = it->دومی;
        أرجع خطأ;
    }

    قالب <نام_نوع T_گره>
    فراغ fixupClassOrPackage(Ast_گره* memberp, T_گره refp) {
        Ast_گره_Module* ثابت declClassp = VN_AS(memberp->user2p(), گره_Module);
        إذا (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    قالب <نام_نوع T_گره>
    منطقي is_Reference_مقصد_Inner_Member(T_گره nodep) {
        أرجع VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_سطر* fl) {
        إذا (!m_thisp) {
            AstClassRefDType* ثابت refDTypep
                = جديد AstClassRefDType{fl, VN_AS(m_callerp, Class), اشاره_تهی};
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(refDTypep);
            m_thisp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(درست);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::درون_داد);
            m_argsp = Ast_گره::جمع_کن_بعدی(m_argsp, جديد AstArg{fl, "", جديد AstThisRef{fl, refDTypep}});
        }
        أرجع m_thisp;
    }

    AstVar* getVar(AstVar* ثابت varp) ثابت {
        ثابت خودکار it = m_varCloneMap.پیدا_کن(varp);
        إذا (it == m_varCloneMap.انتها()) أرجع اشاره_تهی;
        إذا (it->دومی->isStdRandomizeArg()) أرجع اشاره_تهی;
        أرجع it->دومی;
    }

    Capture_نما get_Var_Ref_Capture_نما(Ast_گره_Var_Ref* varRefp) {
        Ast_گره_Module* ثابت varModp = VN_AS(varRefp->varp()->user2p(), گره_Module);
        AstClass* ثابت varClassp = VN_CAST(varModp, Class);
        AstClass* ثابت callerClassp = VN_CAST(m_callerp, Class);

        ثابت منطقي callerIsClass = callerClassp;
        ثابت منطقي refIsXref = VN_IS(varRefp, VarXRef);
        ثابت منطقي varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ثابت منطقي varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ثابت منطقي var_Is_فیلد_از_Caller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ثابت منطقي varIsParam = varRefp->varp()->isParam();
        ثابت منطقي varIsConstraintIterator
            = VN_IS(varRefp->varp()->اولی_Abovep(), Foreach_سرآیند)
              && VN_IS(varRefp->varp()->اولی_Abovep()->اولی_Abovep(), ConstraintForeach);
        إذا (refIsXref) أرجع Capture_نما::CAP_مقدار | Capture_نما::CAP_F_XREF;
        إذا (varIsConstraintIterator) أرجع Capture_نما::CAP_NO;
        إذا (varIsFuncLocal && varHasAutomaticLifetime) أرجع Capture_نما::CAP_مقدار;
        إذا (varIsParam) أرجع Capture_نما::CAP_مقدار;
        // Static var in function (will not be inlined, because it's in class)
        إذا (callerIsClass && varIsFuncLocal) أرجع Capture_نما::CAP_مقدار;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        إذا (m_staticContext && callerIsClass && var_Is_فیلد_از_Caller
            && varRefp->varp()->lifetime().isStatic())
            أرجع Capture_نما::CAP_NO;
        إذا (callerIsClass && var_Is_فیلد_از_Caller) أرجع Capture_نما::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        إذا (callerIsClass && varClassp && !var_Is_فیلد_از_Caller
            && varRefp->varp()->lifetime().isStatic())
            أرجع Capture_نما::CAP_مقدار;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        أرجع Capture_نما::CAP_مقدار;
    }

    فراغ capture_Ref_By_مقدار(Ast_گره_Var_Ref* nodep, Capture_نما cap_نما_Flags) {
        AstVar* newVarp;
        ثابت منطقي newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_گره_Var_Ref* ثابت newVarRefp = newCapture ? nodep->clone_درخت(خطأ) : اشاره_تهی;
        إذا (!hasFlags(cap_نما_Flags, Capture_نما::CAP_F_مجموعه_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(اشاره_تهی);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        إذا (!newCapture) أرجع;
        إذا (hasFlags(cap_نما_Flags, Capture_نما::CAP_F_XREF)) {
            AstVarRef* ثابت notXVarRefp
                = جديد AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_گره::جمع_کن_بعدی(m_argsp, جديد AstArg{nodep->fileline(), "", newVarRefp});
    }

    فراغ captureRefByThis(Ast_گره_Var_Ref* nodep, Capture_نما /*capModeFlags*/) {
        AstVar* ثابت thisp = importThisp(nodep->fileline());
        AstVarRef* ثابت thisRefp = جديد AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(درست);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ثابت memberSelp
            = جديد AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        إذا (!m_targetp && nodep->user1()) memberSelp->user1(درست);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        إذا (m_ignore.شمار(nodep)) أرجع;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        إذا (nodep->varp()->isStdRandomizeArg()) أرجع;
        ثابت Capture_نما cap_نما = get_Var_Ref_Capture_نما(nodep);
        إذا (نما(cap_نما) == Capture_نما::CAP_NO) أرجع;
        إذا (نما(cap_نما) == Capture_نما::CAP_مقدار) capture_Ref_By_مقدار(nodep, cap_نما);
        إذا (نما(cap_نما) == Capture_نما::CAP_THIS) captureRefByThis(nodep, cap_نما);
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        إذا (m_ignore.شمار(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        إذا (VN_IS(nodep, روش_Call) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            أرجع;
        }
        AstClass* ثابت classp = VN_CAST(nodep->taskp()->user2p(), Class);
        إذا ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ثابت argsp = nodep->argsp();
            إذا (argsp) argsp->unlink_Fr_پشت();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ثابت thisp = importThisp(nodep->fileline());
            AstVarRef* ثابت thisRefp = جديد AstVarRef{
                nodep->fileline(), thisp, nodep->is_خالص() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            Ast_روش_Call* ثابت روش_Callp
                = جديد Ast_روش_Call{nodep->fileline(), thisRefp, thisp->name(), argsp};
            روش_Callp->taskp(nodep->taskp());
            روش_Callp->dtypep(nodep->dtypep());
            nodep->replaceWith(روش_Callp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            m_ignore.emplace(روش_Callp);
        }
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        إذا (!is_Reference_مقصد_Inner_Member(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        AstVarRef* ثابت varRefp
            = جديد AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    فراغ بازدید(Ast_روش_Call* nodep) override {
        إذا (!is_Reference_مقصد_Inner_Member(nodep) || m_ignore.شمار(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        AstArg* ثابت argsp = nodep->argsp();
        إذا (argsp) argsp->unlink_Fr_پشت_With_بعدی();
        Ast_گره_FTask_Ref* taskRefp = اشاره_تهی;
        إذا (AstTask* ثابت taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = جديد AstTaskRef{nodep->fileline(), taskp, argsp};
        } وإلا إذا (AstFunc* ثابت taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = جديد AstFuncRef{nodep->fileline(), taskp, argsp};
        } وإلا {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    صریح CaptureVisitor(Ast_گره* ثابت nodep, Ast_گره_Module* callerp, AstClass* ثابت targetp,
                            منطقي staticContext = خطأ)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterate_And_بعدی_Null(nodep);
    }

    // PUBLIC METHODS

    AstArg* get_آرگومانها() ثابت { أرجع m_argsp; }
    AstVar* getThisp() ثابت { أرجع m_thisp; }

    فراغ جمع_کن_Function_Arguments(Ast_گره_FTask* funcp) ثابت {
        لكل (AstArg* argp = get_آرگومانها(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            إذا (Ast_گره_Var_Ref* varrefp = VN_CAST(argp->exprp(), گره_Var_Ref)) {
                إذا ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(اشاره_تهی);
                }
                funcp->جمع_کن_Stmtsp(getVar(varrefp->varp()));
            } وإلا {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->جمع_کن_Stmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

صنف RandomizeVisitor final : عام VNVisitor {
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
    ثابت VNUser3_درون_Use m_inuser3;
    ثابت VNUser4_درون_Use m_inuser4;

    // STATE
    V3_Unique_نامها m_inline_Unique_نامها;  // For generating unique function names
    V3_Unique_نامها m_rand_Array_Unique_نامها{"__Vrandarr"};  // Random array loop indices
    V3_Unique_نامها m_نما_Unique_نامها{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3_Unique_نامها m_inlineUniqueStdName{"__VStdrand"};
    V3_Unique_نامها m_unique_نامها;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    std::unordered_map<Ast_گره_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Current function/task
    Ast_گره_Stmt* m_stmtp = اشاره_تهی;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = اشاره_تهی;  // Dynamic array type (for rand mode)
    صحيح m_rand_Case_رقم = 0;  // Randcase number within a module for var naming
    صحيح m_dist_رقم = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = اشاره_تهی;  // Current constraint
    std::مجموعه<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertion_مجموعه<AstVar*>> m_اندازه_Constrained_Arrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_static_Constraint_نما_Vars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_static_Rand_نما_Vars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<منطقي, منطقي>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ساكن منطقي is_Similar_گره(ثابت Ast_گره_عبارت* with_عبارت, ثابت Ast_گره_عبارت* arg_عبارت) {
        // VarRef: compare variable pointers
        إذا (VN_IS(arg_عبارت, VarRef) && VN_IS(with_عبارت, VarRef)) {
            أرجع VN_AS(with_عبارت, VarRef)->varp() == VN_AS(arg_عبارت, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        إذا (VN_IS(arg_عبارت, MemberSel) && VN_IS(with_عبارت, MemberSel)) {
            ثابت AstMemberSel* ثابت withMSp = VN_AS(with_عبارت, MemberSel);
            ثابت AstMemberSel* ثابت argMSp = VN_AS(arg_عبارت, MemberSel);
            إذا (withMSp->varp() != argMSp->varp()) أرجع خطأ;
            // Recursively compare the base object expression
            أرجع is_Similar_گره(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        إذا (VN_IS(arg_عبارت, ArraySel) && VN_IS(with_عبارت, ArraySel)) {
            ثابت AstArraySel* ثابت withASp = VN_AS(with_عبارت, ArraySel);
            ثابت AstArraySel* ثابت argASp = VN_AS(arg_عبارت, ArraySel);
            // Before checking Sel type index, check if both are Const
            إذا (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ثابت AstConst* ثابت withIdxp = VN_AS(withASp->bitp(), Const);
                ثابت AstConst* ثابت argIdxp = VN_AS(argASp->bitp(), Const);
                أرجع is_Similar_گره(withASp->fromp(), argASp->fromp())
                       && withIdxp->رقم().isCaseEq(argIdxp->رقم());
            }
            // Index must be Sel type, extract VarRef using fromp()
            إذا (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) أرجع خطأ;
            ثابت Ast_گره_عبارت* ثابت withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ثابت Ast_گره_عبارت* ثابت argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            أرجع is_Similar_گره(withASp->fromp(), argASp->fromp())
                   && is_Similar_گره(withIdxp, argIdxp);
        }
        أرجع خطأ;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ساكن فراغ expandUniqueElementList(Ast_گره* itemsp) {
        Ast_گره* itemp = itemsp;
        طالما (itemp) {
            Ast_گره* ثابت nextp = itemp->nextp();
            إذا (ثابت AstConstraintIf* ثابت ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                استمر;
            }
            إذا (ثابت AstConstraintForeach* ثابت foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                استمر;
            }
            AstConstraintUnique* ثابت uniquep = VN_CAST(itemp, ConstraintUnique);
            إذا (!uniquep) {
                itemp = nextp;
                استمر;
            }

            std::وکتور<Ast_گره_عبارت*> عبارت_اقلام;
            منطقي hasArrayVarRef = خطأ;
            لكل (Ast_گره* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                إذا (ثابت AstVarRef* ثابت vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ثابت Ast_گره_DType* ثابت dtypep = vrp->varp()->dtypep()->skipRefp();
                    إذا (VN_IS(dtypep, گره_Array_DType) || VN_IS(dtypep, صف_DType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = درست;
                        استمر;
                    }
                }
                عبارت_اقلام.افزودن_پشت(VN_AS(rp, گره_عبارت));
            }

            إذا (عبارت_اقلام.اندازه() >= 2) {
                File_سطر* ثابت fl = uniquep->fileline();
                لكل (اندازه_t i = 0; i < عبارت_اقلام.اندازه(); i++) {
                    لكل (اندازه_t j = i + 1; j < عبارت_اقلام.اندازه(); j++) {
                        Ast_گره_عبارت* ثابت lhsp = عبارت_اقلام[i]->clone_درخت(خطأ);
                        Ast_گره_عبارت* ثابت rhsp = عبارت_اقلام[j]->clone_درخت(خطأ);
                        AstNeq* ثابت neqp = جديد AstNeq{fl, lhsp, rhsp};
                        neqp->user1(درست);
                        Ast_Constraint_عبارت* ثابت cexprp = جديد Ast_Constraint_عبارت{fl, neqp};
                        uniquep->جمع_کن_بعدی_اینجا(cexprp);
                    }
                }
                إذا (!hasArrayVarRef) {
                    uniquep->unlink_Fr_پشت();
                    VL_DO_DANGLING(uniquep->delete_درخت(), uniquep);
                }
            } وإلا إذا (عبارت_اقلام.اندازه() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlink_Fr_پشت();
                VL_DO_DANGLING(uniquep->delete_درخت(), uniquep);
            }
            itemp = nextp;
        }
    }
    فراغ createRandomGenerator(AstClass* ثابت classp) {
        إذا (classp->user3p()) أرجع;
        إذا (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            أرجع;
        }
        AstVar* ثابت genp = جديد AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->پیدا_کن_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->جمع_کن_Membersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_گره_Module* ثابت modp) {
        خودکار it = m_stdMap.پیدا_کن(modp);
        إذا (it == m_stdMap.انتها()) {
            AstVar* ثابت stdgenp
                = جديد AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->پیدا_کن_Basic_DType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3_Error_کد::IMPURE, درست);
            modp->جمع_کن_Stmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            أرجع stdgenp;
        }
        أرجع it->دومی;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ثابت classp) {
        ساكن ثابت حرف* ثابت name = "__Vprepare_constrained_arrays";
        AstTask* ثابت taskp = جديد AstTask{classp->fileline(), name, اشاره_تهی};
        taskp->class_روش(درست);
        classp->جمع_کن_Membersp(taskp);
        m_memberMap.درج(classp, taskp);
        أرجع taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ثابت classp) {
        ساكن constexpr ثابت حرف* ثابت name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.پیدا_کن_Member(classp, name), Task);
        إذا (resizeTaskp) أرجع resizeTaskp;
        resizeTaskp = جديد AstTask{classp->fileline(), name, اشاره_تهی};
        resizeTaskp->class_روش(درست);
        resizeTaskp->isVirtual(درست);
        classp->جمع_کن_Membersp(resizeTaskp);
        m_memberMap.درج(classp, resizeTaskp);
        أرجع resizeTaskp;
    }
    AstVar* get_Create_Rand_نما_Var(AstClass* ثابت classp) {
        إذا (classp->user2p()) أرجع VN_AS(classp->user2p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Rand_نما_Var(extendsp->classp());
        }
        AstVar* ثابت rand_نما_Varp = create_نما_Var(classp, "__Vrandmode");
        classp->user2p(rand_نما_Varp);
        أرجع rand_نما_Varp;
    }
    AstVar* get_Create_Constraint_نما_Var(AstClass* ثابت classp) {
        إذا (classp->user4p()) أرجع VN_AS(classp->user4p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Constraint_نما_Var(extendsp->classp());
        }
        AstVar* ثابت constraint_نما_Varp = create_نما_Var(classp, "__Vconstraintmode");
        classp->user4p(constraint_نما_Varp);
        أرجع constraint_نما_Varp;
    }
    ساكن AstVar* get_Constraint_نما_Var(AstClass* ثابت classp) {
        إذا (classp->user4p()) أرجع VN_AS(classp->user4p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Constraint_نما_Var(extendsp->classp());
        }
        أرجع اشاره_تهی;
    }
    AstVar* get_Create_Static_Constraint_نما_Var(AstClass* ثابت classp) {
        خودکار it = m_static_Constraint_نما_Vars.پیدا_کن(classp);
        إذا (it != m_static_Constraint_نما_Vars.انتها()) أرجع it->دومی;
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Static_Constraint_نما_Var(extendsp->classp());
        }
        AstVar* ثابت static_نما_Varp = create_Static_نما_Var(classp, "__Vstaticconstraintmode");
        m_static_Constraint_نما_Vars[classp] = static_نما_Varp;
        أرجع static_نما_Varp;
    }
    AstVar* get_Static_Constraint_نما_Var(AstClass* ثابت classp) {
        خودکار it = m_static_Constraint_نما_Vars.پیدا_کن(classp);
        إذا (it != m_static_Constraint_نما_Vars.انتها()) أرجع it->دومی;
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Static_Constraint_نما_Var(extendsp->classp());
        }
        أرجع اشاره_تهی;
    }
    AstVar* get_Create_Static_Rand_نما_Var(AstClass* ثابت classp) {
        إذا (m_static_Rand_نما_Vars.شمار(classp)) أرجع m_static_Rand_نما_Vars[classp];
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Static_Rand_نما_Var(extendsp->classp());
        }
        AstVar* ثابت static_نما_Varp = create_Static_نما_Var(classp, "__Vstaticrandmode");
        m_static_Rand_نما_Vars[classp] = static_نما_Varp;
        أرجع static_نما_Varp;
    }
    AstVar* get_Static_Rand_نما_Var(AstClass* ثابت classp) {
        إذا (m_static_Rand_نما_Vars.شمار(classp)) أرجع m_static_Rand_نما_Vars[classp];
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Static_Rand_نما_Var(extendsp->classp());
        }
        أرجع اشاره_تهی;
    }
    AstVar* create_نما_Var(AstClass* ثابت classp, ثابت حرف* ثابت name) {
        File_سطر* ثابت fl = classp->fileline();
        إذا (!m_dynarrayDtp) {
            m_dynarrayDtp = جديد AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->پیدا_کن_بیت_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ثابت نما_Varp = جديد AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        نما_Varp->user2p(classp);
        classp->جمع_کن_Stmtsp(نما_Varp);
        أرجع نما_Varp;
    }
    AstVar* create_Static_نما_Var(AstClass* ثابت classp, ثابت حرف* ثابت name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_سطر* ثابت fl = classp->fileline();
        إذا (!m_dynarrayDtp) {
            m_dynarrayDtp = جديد AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->پیدا_کن_بیت_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(m_dynarrayDtp);
        }
        AstVar* ثابت نما_Varp = جديد AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        نما_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        نما_Varp->user2p(classp);
        classp->جمع_کن_Stmtsp(نما_Varp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.درج(classp, نما_Varp);
        أرجع نما_Varp;
    }
    ساكن فراغ جمع_کن_مجموعه_Rand_نما(Ast_گره_FTask* ثابت ftaskp, AstVar* ثابت genp,
                               AstVar* ثابت rand_نما_Varp) {
        File_سطر* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت مجموعه_Rand_Modep = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, VN_AS(genp->user2p(), گره_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_مجموعه_RANDMODE,
            جديد AstVarRef{fl, VN_AS(rand_نما_Varp->user2p(), گره_Module), rand_نما_Varp,
                          VAccess::READ}};
        مجموعه_Rand_Modep->dtype_مجموعه_Void();
        ftaskp->جمع_کن_Stmtsp(مجموعه_Rand_Modep->makeStmt());
    }
    ساكن فراغ جمع_کن_مجموعه_Static_Rand_نما(Ast_گره_FTask* ثابت ftaskp, AstVar* ثابت genp,
                                     AstVar* ثابت static_Rand_نما_Varp) {
        File_سطر* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت setp = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, VN_AS(genp->user2p(), گره_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_مجموعه_STATIC_RANDMODE,
            جديد AstVarRef{fl, VN_AS(static_Rand_نما_Varp->user2p(), گره_Module), static_Rand_نما_Varp,
                          VAccess::READ}};
        setp->dtype_مجموعه_Void();
        ftaskp->جمع_کن_Stmtsp(setp->makeStmt());
    }
    فراغ createRandomizeClassVars(AstNetlist* ثابت netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> ریشه_Static_Rand_نما_شمار;
        netlistp->foreach([هذا, &ریشه_Static_Rand_نما_شمار](AstClass* ثابت classp) {
            منطقي hasConstraints = خطأ;
            uint32_t rand_نما_شمار = 0;
            uint32_t static_Rand_نما_شمار = 0;
            uint32_t constraint_نما_شمار = 0;
            uint32_t static_Constraint_نما_شمار = 0;
            classp->foreachMember([&](AstClass*, Ast_گره* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                إذا (AstConstraint* ثابت constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = درست;
                    Randomize_نما constraint_نما = {.asUQuad = memberp->user1()};
                    إذا (!constraint_نما.استفاده_میکند_نما) أرجع;
                    إذا (constraint_نما.شاخص == 0) {
                        // Use separate index counters for static vs non-static constraints
                        إذا (constrp->isStatic()) {
                            constraint_نما.شاخص = static_Constraint_نما_شمار++;
                        } وإلا {
                            constraint_نما.شاخص = constraint_نما_شمار++;
                        }
                        memberp->user1(constraint_نما.asUQuad);
                    } وإلا {
                        إذا (constrp->isStatic()) {
                            static_Constraint_نما_شمار = constraint_نما.شاخص + 1;
                        } وإلا {
                            constraint_نما_شمار = constraint_نما.شاخص + 1;
                        }
                    }
                } وإلا إذا (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
                    Randomize_نما rand_نما = {.asUQuad = memberp->user1()};
                    إذا (!rand_نما.استفاده_میکند_نما) أرجع;
                    ثابت منطقي isStaticVar = varp->lifetime().isStatic();
                    إذا (rand_نما.شاخص == 0) {
                        إذا (isStaticVar) {
                            rand_نما.شاخص = static_Rand_نما_شمار++;
                        } وإلا {
                            rand_نما.شاخص = rand_نما_شمار++;
                        }
                        memberp->user1(rand_نما.asUQuad);
                    } وإلا {
                        إذا (isStaticVar) {
                            static_Rand_نما_شمار = rand_نما.شاخص + 1;
                        } وإلا {
                            rand_نما_شمار = rand_نما.شاخص + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<فراغ(AstClass*)> پیدا_کن_Sub_Obj_Rand_Modes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_گره* subMemberp) {
                        إذا (AstVar* ثابت subVarp = VN_CAST(subMemberp, Var)) {
                            ثابت Randomize_نما rm = {.asUQuad = subVarp->user1()};
                            إذا (!rm.استفاده_میکند_نما) أرجع;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            إذا (subVarp->lifetime().isStatic()) أرجع;
                            ثابت uint32_t needed = rm.شاخص + 1;
                            إذا (needed > rand_نما_شمار) rand_نما_شمار = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    إذا (!memberVarp->globalConstrained()) أرجع;
                    ثابت Ast_گره_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                    إذا (!classRefp) أرجع;
                    پیدا_کن_Sub_Obj_Rand_Modes(classRefp->classp());
                });
            }
            إذا (hasConstraints) createRandomGenerator(classp);
            إذا (rand_نما_شمار > 0) {
                AstVar* ثابت rand_نما_Varp = get_Create_Rand_نما_Var(classp);
                make_نما_Init(rand_نما_Varp, classp, rand_نما_شمار);
            }
            إذا (constraint_نما_شمار > 0) {
                AstVar* ثابت constraint_نما_Varp = get_Create_Constraint_نما_Var(classp);
                make_نما_Init(constraint_نما_Varp, classp, constraint_نما_شمار);
            }
            إذا (static_Constraint_نما_شمار > 0) {
                AstVar* ثابت static_Constraint_نما_Varp = get_Create_Static_Constraint_نما_Var(classp);
                make_Static_نما_Init(static_Constraint_نما_Varp, classp, static_Constraint_نما_شمار);
            }
            إذا (static_Rand_نما_شمار > 0) {
                get_Create_Static_Rand_نما_Var(classp);
                AstClass* rootp = classp;
                طالما (AstClassExtends* ثابت ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& جایگاه_2 = ریشه_Static_Rand_نما_شمار[rootp];
                إذا (static_Rand_نما_شمار > جایگاه_2) جایگاه_2 = static_Rand_نما_شمار;
            }
        });
        لكل (ثابت خودکار& kv : ریشه_Static_Rand_نما_شمار) emit_ریشه_Static_نما_Init(kv.اولی, kv.دومی);
    }
    فراغ emit_ریشه_Static_نما_Init(AstClass* ثابت rootp, ثابت uint32_t شمار) {
        AstVar* ثابت static_Rand_نما_Varp = m_static_Rand_نما_Vars[rootp];
        UASSERT_OBJ(static_Rand_نما_Varp, rootp, "Root must have a static rand-mode var");
        make_Static_نما_Init(static_Rand_نما_Varp, rootp, شمار);
    }
    فراغ make_نما_Init(AstVar* نما_Varp, AstClass* classp, uint32_t نما_شمار) {
        Ast_گره_Module* ثابت نما_Var_Modp = VN_AS(نما_Varp->user2p(), گره_Module);
        File_سطر* ثابت fl = نما_Varp->fileline();
        AstVar* ثابت old_اندازه_Varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, m_unique_نامها.get("__VoldSize"), نما_Varp->پیدا_کن_UInt32_DType()};
        old_اندازه_Varp->funcLocal(درست);
        old_اندازه_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت oldSizep
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, نما_Var_Modp, نما_Varp, VAccess::READ},
                                 VCMethod::DYN_اندازه, اشاره_تهی};
        oldSizep->dtype_مجموعه_UInt32();
        AstCMethodHard* ثابت dynarrayNewp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, نما_Var_Modp, نما_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جديد AstConst{fl, نما_شمار}};
        dynarrayNewp->dtype_مجموعه_Void();
        Ast_گره_FTask* ثابت ctorNewp = VN_AS(m_memberMap.پیدا_کن_Member(classp, "new"), گره_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_گره* ثابت initFirstp = old_اندازه_Varp;
        initFirstp->جمع_کن_بعدی(
            جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, old_اندازه_Varp, VAccess::WRITE}, oldSizep});
        initFirstp->جمع_کن_بعدی(dynarrayNewp->makeStmt());
        initFirstp->جمع_کن_بعدی(make_نما_مجموعه_Loop(
            fl, جديد AstVarRef{fl, نما_Var_Modp, نما_Varp, VAccess::WRITE}, جديد AstConst{fl, 1},
            درست, جديد AstVarRef{fl, old_اندازه_Varp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        لكل (Ast_گره* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (!VN_IS(stmtp, گره_Stmt)) استمر;  // Skip var declarations
            إذا (ثابت Ast_Stmt_عبارت* ثابت sep = VN_CAST(stmtp, Stmt_عبارت)) {
                إذا (VN_IS(sep->exprp(), New)) استمر;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->جمع_کن_اینجا_This_As_بعدی(initFirstp);
            أرجع;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->جمع_کن_Stmtsp(initFirstp);
    }
    فراغ make_Static_نما_Init(AstVar* نما_Varp, AstClass* classp, uint32_t نما_شمار) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_گره_Module* ثابت نما_Var_Modp = VN_AS(نما_Varp->user2p(), گره_Module);
        File_سطر* ثابت fl = نما_Varp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ثابت sizep
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, نما_Var_Modp, نما_Varp, VAccess::READ},
                                 VCMethod::DYN_اندازه, اشاره_تهی};
        sizep->dtype_مجموعه_UInt32();
        AstEq* ثابت condp = جديد AstEq{fl, sizep, جديد AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ثابت dynarrayNewp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, نما_Var_Modp, نما_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جديد AstConst{fl, نما_شمار}};
        dynarrayNewp->dtype_مجموعه_Void();
        Ast_گره* ثابت thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->جمع_کن_بعدی(
            make_نما_مجموعه_Loop(fl, جديد AstVarRef{fl, نما_Var_Modp, نما_Varp, VAccess::WRITE},
                            جديد AstConst{fl, 1}, درست));

        // Build the if statement
        AstIf* ثابت ifp = جديد AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_گره_FTask* ثابت newp = VN_AS(m_memberMap.پیدا_کن_Member(classp, "new"), گره_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->جمع_کن_Stmtsp(ifp);
    }
    Ast_گره* make_نما_مجموعه_Loop(File_سطر* ثابت fl, Ast_گره_عبارت* ثابت lhsp, Ast_گره_عبارت* ثابت rhsp,
                             منطقي درون_Task, Ast_گره_عبارت* ثابت startp = اشاره_تهی) {
        AstVar* ثابت iterVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->پیدا_کن_UInt32_DType()};
        iterVarp->funcLocal(درون_Task);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت sizep = جديد AstCMethodHard{fl, lhsp, VCMethod::DYN_اندازه, اشاره_تهی};
        sizep->dtype_مجموعه_UInt32();
        AstCMethodHard* ثابت setp
            = جديد AstCMethodHard{fl, lhsp->clone_درخت(خطأ), VCMethod::ARRAY_AT_WRITE,
                                 جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtype_مجموعه_UInt32();
        Ast_گره* ثابت stmtsp = iterVarp;
        stmtsp->جمع_کن_بعدی(جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : جديد AstConst{fl, 0}});

        AstLoop* ثابت loopp = جديد AstLoop{fl};
        stmtsp->جمع_کن_بعدی(loopp);
        loopp->جمع_کن_Stmtsp(جديد AstLoopTest{
            fl, loopp, جديد AstLt{fl, جديد AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, setp, rhsp});
        loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
            fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
            جديد Ast_جمع_کن{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, iterVarp, VAccess::READ}}});
        أرجع جديد AstBegin{fl, m_unique_نامها.get("__VrandBlock"), stmtsp, درست};
    }
    فراغ جمع_کن_موقت_Varsp(AstVar* varsp) {
        لكل (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (m_ftaskp) {
                varp->funcLocal(درست);
            } وإلا {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        إذا (!m_ftaskp) {
            m_modp->جمع_کن_Stmtsp(varsp);
        } وإلا {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_گره_Stmt* wrap_If_Rand_نما(AstClass* classp, AstVar* ثابت varp, Ast_گره_Stmt* stmtp) {
        ثابت Randomize_نما rmode = {.asUQuad = varp->user1()};
        AstVar* ثابت نما_Varp = varp->lifetime().isStatic() ? get_Static_Rand_نما_Var(classp)
                                                             : get_Rand_نما_Var_From_Class(classp);
        أرجع VN_AS(wrap_If_نما(rmode, نما_Varp, stmtp), گره_Stmt);
    }
    Ast_گره* wrap_If_Constraint_نما(AstClass* classp, AstConstraint* ثابت constrp, Ast_گره* stmtp) {
        ثابت Randomize_نما rmode = {.asUQuad = constrp->user1()};
        AstVar* ثابت نما_Varp = constrp->isStatic() ? get_Static_Constraint_نما_Var(classp)
                                                     : get_Constraint_نما_Var(classp);
        أرجع wrap_If_نما(rmode, نما_Varp, stmtp);
    }
    ساكن Ast_گره* wrap_If_نما(ثابت Randomize_نما نما, AstVar* نما_Varp, Ast_گره* stmtp) {
        File_سطر* ثابت fl = stmtp->fileline();
        إذا (نما.استفاده_میکند_نما) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_گره_Module* ثابت modp = VN_AS(نما_Varp->user2p(), گره_Module);
            AstCMethodHard* ثابت atp
                = جديد AstCMethodHard{fl, جديد AstVarRef{fl, modp, نما_Varp, VAccess::READ},
                                     VCMethod::ARRAY_AT, جديد AstConst{fl, نما.شاخص}};
            atp->dtype_مجموعه_UInt32();
            أرجع جديد AstIf{fl, atp, stmtp};
        }
        أرجع stmtp;
    }
    AstVarRef* enum_مقدار_Tab_Refp(AstEnumDType* ثابت nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), VarRef)->clone_درخت(خطأ);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_گره_Array_DType* ثابت vardtypep = جديد AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            جديد AstRange{nodep->fileline(), static_cast<صحيح>(nodep->قلم_شمار()) - 1, 0}};
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(vardtypep);
        AstConsArray* ثابت consp = جديد AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t شاخص = 0;
        لكل (Ast_Enum_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
            AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(شاخص++, vconstp->clone_درخت(خطأ));
        }
        // Share identical tables via the constant pool
        AstVarRef* ثابت refp = V3ConstPool::پیدا_کن(consp);
        VL_DO_DANGLING(consp->delete_درخت(), consp);  // V3ConstPool::find clones it
        افزودن_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        أرجع refp->clone_درخت(خطأ);
    }

    AstCDType* پیدا_کن_Vl_Rand_CDType(File_سطر* ثابت fl, uint64_t اقلام) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ثابت std::string type = AstCDType::type_مقصد_Hold(V3_عدد::log2bQuad(اقلام) + 1);
        ثابت std::string name = "VlRandC<" + type + ", " + cvt_مقصد_Str(اقلام) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ثابت خودکار pair = m_randcDtypes.emplace(name, اشاره_تهی);
        إذا (pair.دومی) {
            AstCDType* ثابت newp = جديد AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newp);
            pair.اولی->دومی = newp;
        }
        أرجع pair.اولی->دومی;
    }

    AstVar* newRandcVarsp(AstVar* ثابت varp, AstClass* ثابت classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        إذا (!varp->isRandC()) أرجع اشاره_تهی;

        uint64_t اقلام = 0;
        إذا (AstEnumDType* ثابت enumDtp = VN_CAST(varp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType)) {
            اقلام = static_cast<uint64_t>(enumDtp->قلم_شمار());
        } وإلا إذا (AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp()) {
            إذا (basicp->عرض() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                إذا (!V3Error::error_شمار()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->عرض() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                أرجع اشاره_تهی;
            }
            اقلام = 1ULL << basicp->عرض();
        } وإلا إذا (AstStructDType* ثابت dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            أرجع اشاره_تهی;
        } وإلا {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_شمار(), varp, "Unexpected randc variable dtype");
            أرجع اشاره_تهی;
        }
        AstCDType* ثابت newdtp = پیدا_کن_Vl_Rand_CDType(varp->fileline(), اقلام);
        AstVar* ثابت newp
            = جديد AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->is_داخلی(درست);
        classp->جمع_کن_Stmtsp(newp);
        UINFO(9, "created " << varp);
        أرجع newp;
    }
    Ast_گره_Stmt* createArrayForeachLoop(File_سطر* ثابت fl, Ast_گره_DType* ثابت dtypep,
                                        Ast_گره_عبارت* exprp, AstVar* ثابت outputVarp) {
        Ast_گره_DType* موقت_DTypep = dtypep;
        AstVar* randLoopIndxp = اشاره_تهی;
        خودکار create_Loop_شاخص = [&](Ast_گره_DType* موقت_DTypep) {
            إذا (VN_IS(موقت_DTypep, AssocArrayDType)) {
                أرجع جديد AstVar{fl, VVarType::VAR, m_rand_Array_Unique_نامها.get(""),
                                  VN_AS(موقت_DTypep, AssocArrayDType)->کلید_DTypep()};
            }
            أرجع جديد AstVar{fl, VVarType::VAR, m_rand_Array_Unique_نامها.get(""),
                              dtypep->پیدا_کن_Basic_DType(VBasicDTypeKwd::UINT32)};
        };
        Ast_گره_عبارت* موقت_Elementp = اشاره_تهی;
        طالما (موقت_DTypep->isNonPackedArray()) {
            AstVar* ثابت newRandLoopIndxp = create_Loop_شاخص(موقت_DTypep);
            randLoopIndxp = Ast_گره::جمع_کن_بعدی(randLoopIndxp, newRandLoopIndxp);
            Ast_گره_عبارت* ثابت موقت_Exprp = موقت_Elementp ? موقت_Elementp : exprp;
            AstVarRef* ثابت موقت_Refp = جديد AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            إذا (VN_IS(موقت_DTypep, DynArrayDType)) {
                موقت_Elementp
                    = جديد AstCMethodHard{fl, موقت_Exprp, VCMethod::ARRAY_AT_WRITE, موقت_Refp};
            } وإلا إذا (VN_IS(موقت_DTypep, UnpackArrayDType)) {
                Ast_گره_Array_DType* ثابت aryDTypep = VN_CAST(موقت_DTypep, گره_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                موقت_Elementp = جديد AstArraySel{
                    fl, موقت_Exprp,
                    جديد AstSel{
                        fl,
                        جديد AstSub{fl, موقت_Refp,
                                   جديد AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        جديد AstConst{fl, 0}, V3_عدد::log2b(aryDTypep->hi()) + 1}};
            } وإلا إذا (VN_IS(موقت_DTypep, AssocArrayDType)) {
                موقت_Elementp = جديد AstAssocSel{fl, موقت_Exprp, موقت_Refp};
            } وإلا إذا (VN_IS(موقت_DTypep, صف_DType)) {
                موقت_Elementp
                    = جديد AstCMethodHard{fl, موقت_Exprp, VCMethod::DYN_AT_WRITE_APPEND, موقت_Refp};
            }
            موقت_Elementp->dtypep(موقت_DTypep->subDTypep());
            موقت_DTypep = موقت_DTypep->virtRefDTypep();
        }

        Ast_Foreach_سرآیند* ثابت headerp
            = جديد Ast_Foreach_سرآیند{fl, exprp->clone_درخت(خطأ), randLoopIndxp};
        Ast_گره_Stmt* ثابت randStmtsp = newRandStmtsp(fl, موقت_Elementp, اشاره_تهی, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        إذا (!موقت_Elementp->backp()) VL_DO_DANGLING(افزودن_Deletep(موقت_Elementp), موقت_Elementp);
        أرجع جديد AstForeach{fl, headerp, randStmtsp};
    }
    Ast_گره_Stmt* newRandStmtsp(File_سطر* fl, Ast_گره_عبارت* exprp, AstVar* randcVarp,
                               AstVar* ثابت outputVarp, صحيح جابه_جایی = 0,
                               AstMemberDType* memberp = اشاره_تهی) {
        Ast_گره_DType* ثابت memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        إذا (ثابت خودکار* ثابت structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_گره_Stmt* stmtsp = اشاره_تهی;
            إذا (structDtp->packed()) جابه_جایی += memberp ? memberp->lsb() : 0;
            لكل (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_گره_Stmt* randp = اشاره_تهی;
                إذا (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_درخت(خطأ) : exprp, اشاره_تهی,
                                          outputVarp, جابه_جایی, smemberp);
                } وإلا إذا (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = جديد AstStructSel{fl, exprp->clone_درخت(خطأ), smemberp->name()};
                    structSelp->dtypep(smemberp->فرزند_DTypep());
                    إذا (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, اشاره_تهی, outputVarp);
                }
                stmtsp = Ast_گره::جمع_کن_بعدی_Null(stmtsp, randp);
            }
            أرجع stmtsp;
        } وإلا إذا (ثابت خودکار* ثابت unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            إذا (!unionDtp->packed()) أرجع اشاره_تهی;
            AstMemberDType* ثابت اولی_Memberp = unionDtp->membersp();
            أرجع newRandStmtsp(fl, exprp, اشاره_تهی, outputVarp, جابه_جایی, اولی_Memberp);
        } وإلا إذا (ثابت AstClassRefDType* ثابت classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ثابت memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            Ast_روش_Call* ثابت callp = جديد Ast_روش_Call{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            Ast_تخصیص_بده* ثابت assignp = جديد Ast_تخصیص_بده{
                fl, جديد AstVarRef{fl, outputVarp, VAccess::WRITE},
                جديد AstAnd{fl, جديد AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            أرجع جديد AstIf{
                fl, جديد AstNeq{fl, exprp->clone_درخت(خطأ), جديد AstConst{fl, AstConst::Null{}}},
                assignp};
        } وإلا إذا (AstDynArrayDType* ثابت dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            أرجع createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } وإلا إذا (Ast_صف_DType* ثابت صف_Dtp = VN_CAST(memberDtp, صف_DType)) {
            أرجع createArrayForeachLoop(fl, صف_Dtp, exprp, outputVarp);
        } وإلا إذا (AstUnpackArrayDType* ثابت unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            أرجع createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } وإلا إذا (AstAssocArrayDType* ثابت assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            أرجع createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } وإلا {
            Ast_گره_عبارت* valp;
            إذا (AstEnumDType* ثابت enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ثابت tabRefp = enum_مقدار_Tab_Refp(enumDtp);
                Ast_گره_عبارت* ثابت randp
                    = new_Rand_مقدار(fl, randcVarp, exprp->پیدا_کن_Basic_DType(VBasicDTypeKwd::UINT32));
                Ast_گره_عبارت* ثابت moddivp = جديد AstModDiv{
                    fl, randp, جديد AstConst{fl, static_cast<uint32_t>(enumDtp->قلم_شمار())}};
                moddivp->dtypep(enumDtp);
                valp = جديد AstArraySel{fl, tabRefp, moddivp};
            } وإلا {
                valp
                    = new_Rand_مقدار(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            Ast_تخصیص_بده* assignp
                = جديد Ast_تخصیص_بده{fl,
                                جديد AstSel{fl, exprp, جابه_جایی + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->عرض() : exprp->عرض()},
                                valp};
            AstVar* varp = اشاره_تهی;
            exprp->exists([&](ثابت AstVarRef* varrefp) {
                إذا (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                أرجع varp != اشاره_تهی;
            });
            أرجع wrap_If_Rand_نما(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_گره_عبارت* new_Rand_مقدار(File_سطر* ثابت fl, AstVar* ثابت randcVarp,
                              Ast_گره_DType* ثابت dtypep) {
        إذا (randcVarp) {
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->جمع_کن(جديد AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->جمع_کن(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            أرجع cexprp;
        }

        أرجع جديد AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* پیدا_کن_Pre_Post_Task(AstClass* classp, ثابت string& name) {
        لكل (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : اشاره_تهی) {
            إذا (AstTask* ثابت taskp = VN_CAST(m_memberMap.پیدا_کن_Member(cp, name), Task)) {
                أرجع taskp;
            }
        }
        أرجع اشاره_تهی;
    }
    فراغ جمع_کن_Pre_Post_Call(AstClass* ثابت classp, Ast_گره_FTask* ثابت funcp, ثابت string& name) {
        إذا (AstTask* ثابت userFuncp = پیدا_کن_Pre_Post_Task(classp, name)) {
            AstTaskRef* ثابت callp = جديد AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->جمع_کن_Stmtsp(callp->makeStmt());
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
        إذا (AstTask* ثابت existingp = VN_CAST(m_memberMap.پیدا_کن_Member(classp, name), Task)) {
            أرجع existingp;
        }
        AstTask* ثابت taskp = جديد AstTask{classp->fileline(), name, اشاره_تهی};
        taskp->class_روش(درست);
        taskp->isVirtual(classp->isExtended());
        classp->جمع_کن_Membersp(taskp);
        m_memberMap.درج(classp, taskp);
        جمع_کن_Pre_Post_Call(classp, taskp, which);
        أرجع taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<منطقي, منطقي> buildPrePostVirtualWrappers(AstClass* ثابت classp) {
        ثابت خودکار cachedIt = m_prePostWrap.پیدا_کن(classp);
        إذا (cachedIt != m_prePostWrap.انتها()) أرجع cachedIt->دومی;
        std::وکتور<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            إذا (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.افزودن_پشت(subp);
        });
        منطقي hasPre = خطأ;
        منطقي hasPost = خطأ;
        لكل (AstClass* ثابت cp : hierp) {
            إذا (پیدا_کن_Pre_Post_Task(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = درست;
            }
            إذا (پیدا_کن_Pre_Post_Task(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = درست;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        إذا (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        إذا (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ثابت std::pair<منطقي, منطقي> نتیجه{hasPre, hasPost};
        m_prePostWrap.emplace(classp, نتیجه);
        أرجع نتیجه;
    }
    فراغ جمع_کن_Virtual_Pre_Post_Call(AstFunc* ثابت randomizeFuncp, AstClass* ثابت classp,
                               ثابت string& which) {
        File_سطر* ثابت fl = classp->fileline();
        AstTask* ثابت wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ثابت refDTypep = جديد AstClassRefDType{fl, classp, اشاره_تهی};
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(refDTypep);
        Ast_روش_Call* ثابت callp
            = جديد Ast_روش_Call{fl, جديد AstThisRef{fl, refDTypep}, wrapperp->name(), اشاره_تهی};
        callp->taskp(wrapperp);
        callp->dtype_مجموعه_Void();
        randomizeFuncp->جمع_کن_Stmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    منطقي classHasRandClassMembers(AstClass* classp) {
        أرجع classp->existsMember([](ثابت AstClass*, ثابت AstVar* varp) {
            إذا (!varp->rand().isRandomizable()) أرجع خطأ;
            ثابت Ast_گره_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            أرجع VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    منطقي classOwnsGlobalConstraint(ثابت AstClass* classp) ثابت {
        أرجع classp->existsMember([](ثابت AstClass*, ثابت AstConstraint* constrp) {
            منطقي owns = خطأ;
            constrp->foreach([&](ثابت AstMemberSel* memberSelp) {
                ثابت Ast_گره* ثابت rootp
                    = const_cast<AstMemberSel*>(memberSelp)->پایه_Fromp(درست);
                إذا (ثابت AstVarRef* ثابت refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ثابت Ast_گره_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    طالما (ثابت Ast_گره_DType* ثابت subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    إذا (VN_IS(dtypep, ClassRefDType)) owns = درست;
                }
            });
            أرجع owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ثابت string& suffix) {
        ثابت string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.پیدا_کن_Member(classp, name), Task);
        إذا (taskp) أرجع taskp;
        taskp = جديد AstTask{classp->fileline(), name, اشاره_تهی};
        taskp->class_روش(درست);
        classp->جمع_کن_Membersp(taskp);
        m_memberMap.درج(classp, taskp);
        أرجع taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    فراغ populateNestedCallbackTask(AstTask* ثابت callbackTaskp, AstClass* ثابت classp,
                                    ثابت string& cbName) {
        File_سطر* ثابت fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            إذا (!memberVarp->rand().isRandomizable()) أرجع;
            ثابت Ast_گره_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
            ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
            إذا (!classRefp) أرجع;
            AstClass* ثابت memberClassp = classRefp->classp();
            إذا (memberClassp == classp) أرجع;  // Avoid self-reference

            // Force-visit member class if not yet processed
            إذا (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_گره* stmtsp = اشاره_تهی;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            إذا (AstTask* ثابت userFuncp = پیدا_کن_Pre_Post_Task(memberClassp, cbName)) {
                Ast_روش_Call* ثابت callp = جديد Ast_روش_Call{
                    fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtype_مجموعه_Void();
                stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            إذا (classHasRandClassMembers(memberClassp)) {
                ثابت string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ثابت nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                Ast_روش_Call* ثابت recurseCallp = جديد Ast_روش_Call{
                    fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtype_مجموعه_Void();
                stmtsp = Ast_گره::جمع_کن_بعدی(stmtsp, recurseCallp->makeStmt());
            }

            إذا (!stmtsp) أرجع;

            // Wrap in null check
            AstIf* ثابت nullCheckp = جديد AstIf{
                fl,
                جديد AstNeq{fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           جديد AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->جمع_کن_Stmtsp(wrap_If_Rand_نما(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* new_راهاندازی_Constraint_Task(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp = جديد AstTask{nodep->fileline(), name + "_setup_constraint", اشاره_تهی};
        taskp->class_روش(درست);
        nodep->جمع_کن_Membersp(taskp);
        أرجع taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp
            = جديد AstTask{nodep->fileline(), name + "_resize_constrained_array", اشاره_تهی};
        taskp->class_روش(درست);
        nodep->جمع_کن_Membersp(taskp);
        أرجع taskp;
    }
    Ast_گره_Stmt* implementConstraintsClear(File_سطر* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جديد AstCMethodHard{
            fileline,
            جديد AstVarRef{fileline, VN_AS(genp->user2p(), گره_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtype_مجموعه_Void();
        أرجع clearp->makeStmt();
    }
    Ast_گره_Stmt* implementConstraintsClearAll(File_سطر* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جديد AstCMethodHard{
            fileline,
            جديد AstVarRef{fileline, VN_AS(genp->user2p(), گره_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtype_مجموعه_Void();
        أرجع clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_گره_عبارت* ثابت exprp) {
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            أرجع memberSelp->varp();
        } وإلا إذا (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            أرجع varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        أرجع اشاره_تهی;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_گره_عبارت* make_نما_Var_Ref(Ast_گره_عبارت* ثابت exprp, AstVar* ثابت نما_Varp,
                                ثابت VAccess access) {
        إذا (نما_Varp->lifetime().isStatic()) {
            أرجع جديد AstVarRef{exprp->fileline(), VN_AS(نما_Varp->user2p(), گره_Module),
                                 نما_Varp, access};
        }
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ثابت newMemberSelp = جديد AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_درخت(خطأ), نما_Varp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            أرجع newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        أرجع جديد AstVarRef{exprp->fileline(), VN_AS(نما_Varp->user2p(), Class), نما_Varp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* create_Or_Get_اندازه_Var(AstClass* ثابت classp, AstVar* ثابت arrVarp, File_سطر* ثابت fl,
                               Ast_گره_DType* ثابت signed32DTypep, منطقي& wasCreated) {
        AstVar* اندازه_Varp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = خطأ;
        إذا (!اندازه_Varp) {
            اندازه_Varp = جديد AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->جمع_کن_Membersp(اندازه_Varp);
            m_memberMap.درج(classp, اندازه_Varp);
            اندازه_Varp->user2p(classp);
            arrVarp->user4p(اندازه_Varp);
            wasCreated = درست;
        }
        أرجع اندازه_Varp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_عبارت* create_اندازه_Gte_Zero_Constraint(File_سطر* ثابت fl, AstVar* ثابت اندازه_Varp,
                                                   Ast_گره_عبارت* derefFromp = اشاره_تهی) {
        Ast_گره_عبارت* اندازه_گره_Exprp = اشاره_تهی;
        إذا (derefFromp) {
            اندازه_گره_Exprp = buildMemberSelExprp(derefFromp, اندازه_Varp);
        } وإلا {
            اندازه_گره_Exprp = جديد AstVarRef{fl, اندازه_Varp, VAccess::READ};
        }
        اندازه_گره_Exprp->user1(درست);
        AstGteS* ثابت اندازه_Gtep = جديد AstGteS{fl, اندازه_گره_Exprp, جديد AstConst{fl, 0}};
        اندازه_Gtep->user1(درست);
        أرجع جديد Ast_Constraint_عبارت{fl, اندازه_Gtep};
    }
    Ast_گره_عبارت* getFromp(Ast_گره_عبارت* ثابت exprp) {
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            أرجع memberSelp->fromp();
        } وإلا إذا (Ast_روش_Call* ثابت روش_Callp = VN_CAST(exprp, روش_Call)) {
            أرجع روش_Callp->fromp();
        }
        أرجع اشاره_تهی;
    }
    AstVar* make_Tmp_Rand_نما_Var(Ast_گره_عبارت* siblingExprp, AstVar* rand_نما_Varp,
                               Ast_گره*& storeStmtspr, Ast_گره_Stmt*& restoreStmtspr) {
        File_سطر* ثابت fl = rand_نما_Varp->fileline();
        AstVar* ثابت rand_نما_Tmp_Varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, m_نما_Unique_نامها.get(rand_نما_Varp), rand_نما_Varp->dtypep()};
        rand_نما_Tmp_Varp->funcLocal(m_ftaskp);
        rand_نما_Tmp_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_گره::جمع_کن_بعدی(
            storeStmtspr,
            جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, rand_نما_Tmp_Varp, VAccess::WRITE},
                          make_نما_Var_Ref(siblingExprp, rand_نما_Varp, VAccess::READ)});
        storeStmtspr = Ast_گره::جمع_کن_بعدی(
            storeStmtspr,
            make_نما_مجموعه_Loop(fl, make_نما_Var_Ref(siblingExprp, rand_نما_Varp, VAccess::WRITE),
                            جديد AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_گره::جمع_کن_بعدی(
            restoreStmtspr,
            جديد Ast_تخصیص_بده{fl, make_نما_Var_Ref(siblingExprp, rand_نما_Varp, VAccess::WRITE),
                          جديد AstVarRef{fl, rand_نما_Tmp_Varp, VAccess::READ}});
        أرجع rand_نما_Tmp_Varp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_گره_عبارت* برش_مقصد_Common_پیشوند(Ast_گره_عبارت* thisp, Ast_گره_عبارت* otherp) {
        ساكن std::وکتور<Ast_گره_عبارت*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        طالما (thisp) {
            thisHier.افزودن_پشت(thisp);
            thisp = getFromp(thisp);
        }
        طالما (otherp) {
            otherHier.افزودن_پشت(otherp);
            otherp = getFromp(otherp);
        }
        Ast_گره_عبارت* commonp = اشاره_تهی;
        لكل (خودکار thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            إذا ((*thisIt)->type() != (*otherIt)->type()) اكسر;
            إذا (AstMemberSel* ثابت memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ثابت otherMemberSelp = VN_AS(*otherIt, MemberSel);
                إذا (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    استمر;
                }
            } وإلا إذا (Ast_روش_Call* ثابت this_روش_Callp = VN_CAST(*thisIt, روش_Call)) {
                Ast_روش_Call* ثابت other_روش_Callp = VN_AS(*otherIt, روش_Call);
                إذا (this_روش_Callp->taskp() == other_روش_Callp->taskp()) {
                    commonp = this_روش_Callp;
                    استمر;
                }
            } وإلا إذا (AstVarRef* ثابت اولی_Var_Refp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ثابت دومی_Var_Refp = VN_AS(*otherIt, VarRef);
                إذا (اولی_Var_Refp->varp() == دومی_Var_Refp->varp()) {
                    commonp = اولی_Var_Refp;
                    استمر;
                }
            }
            اكسر;
        }
        أرجع commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_گره_عبارت* buildMemberSelExprp(Ast_گره_عبارت* exprp, AstVar* عمیقترین_Varp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_گره_عبارت* resExprp = اشاره_تهی;
        إذا (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), اشاره_تهی);
            AstVar* varp = memberSelp->varp();
            إذا (عمیقترین_Varp) varp = عمیقترین_Varp;
            resExprp = جديد AstMemberSel{exprp->fileline(), resExprp, varp};
        } وإلا إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            إذا (عمیقترین_Varp) varp = عمیقترین_Varp;
            resExprp = جديد AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), گره_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(درست);
        أرجع resExprp;
    }

    فراغ جمع_کن_Update_Rand_Vars_Body(AstClass* ثابت nodep, AstFunc* ثابت randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ثابت updatep
            = جديد AstFunc{nodep->fileline(), "__VupdateRandVars", اشاره_تهی, اشاره_تهی};
        updatep->class_روش(درست);
        updatep->isVirtual(درست);
        updatep->keepAlive(درست);
        updatep->needsSyms(خطأ);
        nodep->جمع_کن_Membersp(updatep);
        nodep->hasRandVarsUpdate(درست);

        لكل (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : اشاره_تهی) {
            Ast_گره_FTask* ثابت newp = VN_AS(m_memberMap.پیدا_کن_Member(classp, "new"), گره_FTask);
            لكل (Ast_گره* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_عبارت* ثابت stmtExprp = VN_CAST(stmtp, Stmt_عبارت);
                إذا (!stmtExprp) استمر;
                AstCMethodHard* ثابت methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                إذا (!methodp || methodp->روش() != VCMethod::RANDOMIZER_WRITE_VAR) استمر;

                Ast_گره_Var_Ref* ثابت varRefp = VN_AS(methodp->pinsp(), گره_Var_Ref);
                إذا (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    استمر;
                }
                AstCMethodHard* ثابت updateMethodp = جديد AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_درخت_خالص(خطأ),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_درخت_خالص(خطأ)};
                updateMethodp->dtype_مجموعه_Void();
                Ast_گره_عبارت* ثابت namep = VN_AS(varRefp->nextp()->nextp(), گره_عبارت);
                updateMethodp->جمع_کن_Pinsp(namep->clone_درخت_خالص(خطأ));
                updateMethodp->روش(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->جمع_کن_Stmtsp(updateMethodp->makeStmt());
            }
        }
    }

    فراغ جمع_کن_Basic_Randomize_Body(AstFunc* ثابت basicRandomizep, AstClass* ثابت nodep,
                               AstVar* rand_نما_Varp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_سطر* ثابت fl = nodep->fileline();
        AstVar* ثابت basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ثابت basicFvarRefp = جديد AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ثابت beginBasicValp = جديد AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1};
        basicRandomizep->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, basicFvarRefp, beginBasicValp});
        Ast_گره_FTask* ثابت newp = VN_AS(m_memberMap.پیدا_کن_Member(nodep, "new"), گره_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            إذا (!memberVarp->rand().isRandomizable()) أرجع;
            ثابت Randomize_نما rand_نما = {.asUQuad = memberVarp->user1()};
            إذا (rand_نما.استفاده_میکند_نما
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ثابت setp = جديد AstCMethodHard{
                    nodep->fileline(),
                    جديد AstVarRef{fl, VN_AS(rand_نما_Varp->user2p(), گره_Module), rand_نما_Varp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, جديد AstConst{nodep->fileline(), rand_نما.شاخص}};
                setp->dtype_مجموعه_UInt32();
                newp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, setp, جديد AstConst{fl, 0}});
            }
            إذا (memberVarp->user3()) أرجع;  // Handled in constraints
            ثابت Ast_گره_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
            إذا (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                إذا (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    أرجع;
                }
                AstFunc* ثابت memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                Ast_روش_Call* ثابت callp
                    = memberVarp->globalConstrained()
                          ? جديد Ast_روش_Call{fl,
                                              جديد AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : جديد Ast_روش_Call{
                                fl, جديد AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ثابت basicFvarRefReadp = basicFvarRefp->clone_درخت(خطأ);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ثابت تخصیص_بده_If_Not_Nullp = جديد AstIf{
                    fl,
                    جديد AstNeq{fl, جديد AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               جديد AstConst{fl, AstConst::Null{}}},
                    جديد Ast_تخصیص_بده{fl, basicFvarRefp->clone_درخت(خطأ),
                                  جديد AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->جمع_کن_Stmtsp(wrap_If_Rand_نما(nodep, memberVarp, تخصیص_بده_If_Not_Nullp));
            } وإلا {
                AstVar* ثابت randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ثابت refp
                    = جديد AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_گره_Stmt* ثابت stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                إذا (!refp->backp()) VL_DO_DANGLING(refp->delete_درخت(), refp);
                basicRandomizep->جمع_کن_Stmtsp(
                    جديد AstBegin{fl, m_unique_نامها.get("__VrandBlock"), stmtp, خطأ});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_گره_عبارت* make_نما_تخصیص_بده_Lhs(File_سطر* ثابت fl, AstClass* ثابت classp,
                                   Ast_گره_عبارت* ثابت fromp, AstVar* ثابت نما_Varp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        إذا (نما_Varp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            إذا (fromp) VL_DO_DANGLING(fromp->unlink_Fr_پشت()->delete_درخت(), fromp);
            أرجع جديد AstVarRef{fl, VN_AS(نما_Varp->user2p(), گره_Module), نما_Varp,
                                 VAccess::WRITE};
        } وإلا إذا (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            أرجع جديد AstVarRef{fl, VN_AS(نما_Varp->user2p(), گره_Module), نما_Varp,
                                 VAccess::WRITE};
        } وإلا {
            AstMemberSel* ثابت memberselp = جديد AstMemberSel{fl, fromp->unlink_Fr_پشت(), نما_Varp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            أرجع memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    فراغ replace_With_نما_تخصیص_بده(Ast_گره_FTask_Ref* ثابت ftaskRefp, Ast_گره* ثابت receiverp,
                               Ast_گره_عبارت* ثابت lhsp) {
        replace_With_نما_تخصیص_بده_And_Append(ftaskRefp, receiverp, lhsp, اشاره_تهی);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    فراغ replace_With_نما_تخصیص_بده_And_Append(Ast_گره_FTask_Ref* ثابت ftaskRefp, Ast_گره* ثابت receiverp,
                                        Ast_گره_عبارت* ثابت lhsp, Ast_گره* ثابت appendStmtp) {
        File_سطر* ثابت fl = ftaskRefp->fileline();
        إذا (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_عبارت), ftaskRefp, "Should be a statement");
            Ast_گره_عبارت* ثابت rhsp = ftaskRefp->argsp()->exprp()->unlink_Fr_پشت();
            Ast_گره* newStmtp = اشاره_تهی;
            إذا (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                //نما
                ثابت Randomize_نما rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.استفاده_میکند_نما, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ثابت setp = جديد AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                جديد AstConst{fl, rmode.شاخص}};
                setp->dtype_مجموعه_UInt32();
                newStmtp = جديد Ast_تخصیص_بده{fl, setp, rhsp};
            } وإلا {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = make_نما_مجموعه_Loop(fl, lhsp, rhsp, m_ftaskp);
            }
            إذا (appendStmtp) newStmtp->جمع_کن_بعدی(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(افزودن_Deletep(m_stmtp), m_stmtp = اشاره_تهی);
        } وإلا {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ثابت Randomize_نما rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.استفاده_میکند_نما, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ثابت setp = جديد AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            جديد AstConst{fl, rmode.شاخص}};
            setp->dtype_مجموعه_UInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(افزودن_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    فراغ wrapRandomizeCallWithNullGuard(Ast_گره_FTask_Ref* nodep) {
        Ast_روش_Call* ثابت callp = VN_CAST(nodep, روش_Call);
        إذا (!callp) أرجع;
        إذا (callp->user4_مجموعه_Once()) أرجع;
        File_سطر* ثابت fl = callp->fileline();
        Ast_گره_عبارت* ثابت checkp
            = جديد AstNeq{fl, callp->fromp()->clone_درخت(خطأ), جديد AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlink_Fr_پشت(&relinker);
        relinker.relink(جديد AstCond{fl, checkp, callp, جديد AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    فراغ handle_Randomize_آرگومانها(Ast_گره_FTask_Ref* ثابت nodep) {
        إذا (!nodep->argsp()) أرجع;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        منطقي hasNullArg = خطأ;
        لكل (AstArg *argp = nodep->argsp(), *nextp = اشاره_تهی; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->رقم().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = درست;
                VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        لكل (AstArg *argp = nodep->argsp(), *nextp = اشاره_تهی; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            لكل (AstArg *otherArgp = nextp, *otherNextp = اشاره_تهی; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                إذا (Ast_گره_عبارت* ثابت prefixp
                    = برش_مقصد_Common_پیشوند(argp->exprp(), otherArgp->exprp())) {
                    إذا (prefixp == argp->exprp()) {
                        إذا (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlink_Fr_پشت()->delete_درخت(), otherArgp);
                        استمر;
                    }
                }
                إذا (Ast_گره_عبارت* ثابت prefixp
                    = برش_مقصد_Common_پیشوند(otherArgp->exprp(), argp->exprp())) {
                    إذا (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);
                        اكسر;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::مجموعه<AstVar*> saved_Rand_نما_Varps;
        AstVar* tmpVarps = اشاره_تهی;
        Ast_گره* storeStmtsp = اشاره_تهی;
        Ast_گره* مجموعه_Stmtsp = اشاره_تهی;
        Ast_گره_Stmt* restoreStmtsp = اشاره_تهی;
        لكل (AstArg *argp = nodep->argsp(), *nextp = اشاره_تهی; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_گره_عبارت* exprp = argp->exprp();
            Ast_گره_عبارت* ثابت commonPrefixp = برش_مقصد_Common_پیشوند(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_سطر* ثابت fl = argp->fileline();
            طالما (exprp) {
                إذا (commonPrefixp == exprp) اكسر;
                AstVar* ثابت randVarp = getVarFromRef(exprp);
                AstClass* ثابت classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ثابت rand_نما_Varp = randVarp->lifetime().isStatic()
                                                 ? get_Static_Rand_نما_Var(classp)
                                                 : get_Rand_نما_Var_From_Class(classp);
                UASSERT_OBJ(rand_نما_Varp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                إذا (saved_Rand_نما_Varps.پیدا_کن(rand_نما_Varp) == saved_Rand_نما_Varps.انتها()) {
                    AstVar* ثابت rand_نما_Tmp_Varp
                        = make_Tmp_Rand_نما_Var(exprp, rand_نما_Varp, storeStmtsp, restoreStmtsp);
                    saved_Rand_نما_Varps.درج(rand_نما_Varp);
                    tmpVarps = Ast_گره::جمع_کن_بعدی(tmpVarps, rand_نما_Tmp_Varp);
                }
                ثابت Randomize_نما rand_نما = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = جديد AstCMethodHard{
                    fl, make_نما_Var_Ref(exprp, rand_نما_Varp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, جديد AstConst{fl, rand_نما.شاخص}};
                setp->dtype_مجموعه_UInt32();
                مجموعه_Stmtsp
                    = Ast_گره::جمع_کن_بعدی(مجموعه_Stmtsp, جديد Ast_تخصیص_بده{fl, setp, جديد AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);
        }
        إذا (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* هدف_Classp = اشاره_تهی;
            Ast_روش_Call* ثابت روش_Callp = VN_CAST(nodep, روش_Call);
            إذا (روش_Callp) {
                ثابت Ast_گره_DType* ثابت fromDTypep = روش_Callp->fromp()->dtypep();
                ثابت AstClassRefDType* ثابت crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                هدف_Classp = crdtp->classp();
            } وإلا {
                هدف_Classp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(هدف_Classp, nodep, "randomize(null) target class unresolved");
            AstFunc* ثابت checkOnlyFuncp = getCreateRandomizeNullFunc(هدف_Classp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        إذا (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && مجموعه_Stmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlink_Fr_پشت(&relinker);
            Ast_گره* ثابت stmtsp = storeStmtsp;
            stmtsp->جمع_کن_بعدی(مجموعه_Stmtsp);
            stmtsp->جمع_کن_بعدی(m_stmtp);
            stmtsp->جمع_کن_بعدی(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            جمع_کن_موقت_Varsp(tmpVarps);
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
        ساكن constexpr ثابت حرف* ثابت name = "__Vrandomize_null";
        إذا (AstFunc* ثابت existingp = VN_AS(m_memberMap.پیدا_کن_Member(classp, name), Func)) {
            أرجع existingp;
        }
        File_سطر* ثابت fl = classp->fileline();
        AstFunc* ثابت funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ثابت fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        جمع_کن_Pre_Post_Call(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ثابت classGenp = getRandomGenerator(classp);
        إذا (!classGenp) {
            funcp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE},
                                           جديد AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1}});
        } وإلا {
            Ast_گره_Module* ثابت genModp = VN_AS(classGenp->user2p(), گره_Module);
            funcp->جمع_کن_Stmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ثابت راهاندازی_All_Taskp = get_Create_Constraint_راهاندازی_Func(classp, m_memberMap);
            funcp->جمع_کن_Stmtsp((جديد AstTaskRef{fl, راهاندازی_All_Taskp})->makeStmt());
            AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
            solverCallp->dtype_مجموعه_بیت();
            solverCallp->جمع_کن(جديد AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->جمع_کن(".next_check_only(__Vm_rng)");
            funcp->جمع_کن_Stmtsp(
                جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(درست);
        }

        // 3. post_randomize -- only if result is non-zero
        إذا (AstTask* ثابت userPostp = پیدا_کن_Pre_Post_Task(classp, "post_randomize")) {
            AstTaskRef* ثابت callp = جديد AstTaskRef{userPostp->fileline(), userPostp};
            funcp->جمع_کن_Stmtsp(
                جديد AstIf{fl, جديد AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        أرجع funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* lift_Log_If_Chain_مقصد_Constraint_If(AstLogIf* logIfp) {
        File_سطر* ثابت fl = logIfp->fileline();
        Ast_گره_عبارت* ثابت condp = logIfp->lhsp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت rhsp = logIfp->rhsp()->unlink_Fr_پشت();
        Ast_گره* thenBodyp;
        إذا (AstLogIf* ثابت innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = lift_Log_If_Chain_مقصد_Constraint_If(innerLogIfp);
            VL_DO_DANGLING(افزودن_Deletep(innerLogIfp), innerLogIfp);
        } وإلا {
            thenBodyp = جديد Ast_Constraint_عبارت{fl, rhsp};
        }
        أرجع جديد AstConstraintIf{fl, condp, thenBodyp, اشاره_تهی};
    }

    ساكن منطقي distBoundRefsRandVar(ثابت Ast_گره* boundp) {
        أرجع boundp->exists(
            [](ثابت AstVarRef* vrefp) { أرجع vrefp->varp()->rand().isRandomizable(); });
    }

    ساكن منطقي dist_Bound_Refs_نما_Var(ثابت Ast_گره* boundp) {
        أرجع boundp->exists([](ثابت AstVarRef* vrefp) {
            إذا (!vrefp->varp()->rand().isRandomizable()) أرجع خطأ;
            ثابت Randomize_نما rmode = {.asUQuad = vrefp->varp()->user1()};
            أرجع rmode.استفاده_میکند_نما;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ساكن Ast_گره_عبارت* newDistRangeMembership(AstDist* distp, ثابت AstInsideRange* irp) {
        File_سطر* ثابت fl = distp->fileline();
        ثابت منطقي isSigned = distp->exprp()->isSigned();
        Ast_گره_عبارت* ثابت dist_عبارت_Gtep = distp->exprp()->clone_درخت_خالص(خطأ);
        Ast_گره_عبارت* ثابت dist_عبارت_Ltep = distp->exprp()->clone_درخت_خالص(خطأ);
        dist_عبارت_Gtep->user1(درست);
        dist_عبارت_Ltep->user1(درست);
        Ast_گره_عبارت* ثابت gep
            = isSigned ? static_cast<Ast_گره_عبارت*>(
                             جديد AstGteS{fl, dist_عبارت_Gtep, irp->lhsp()->clone_درخت_خالص(خطأ)})
                       : static_cast<Ast_گره_عبارت*>(
                             جديد AstGte{fl, dist_عبارت_Gtep, irp->lhsp()->clone_درخت_خالص(خطأ)});
        Ast_گره_عبارت* ثابت lep
            = isSigned ? static_cast<Ast_گره_عبارت*>(
                             جديد AstLteS{fl, dist_عبارت_Ltep, irp->rhsp()->clone_درخت_خالص(خطأ)})
                       : static_cast<Ast_گره_عبارت*>(
                             جديد AstLte{fl, dist_عبارت_Ltep, irp->rhsp()->clone_درخت_خالص(خطأ)});
        gep->user1(درست);
        lep->user1(درست);
        Ast_گره_عبارت* ثابت andp = جديد AstLogAnd{fl, gep, lep};
        andp->user1(درست);
        أرجع andp;
    }

    بنية Dist_سطل final {
        Ast_گره_عبارت* rangep;  // A single value or an InsideRange
        Ast_گره_عبارت* وزن_Exprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::وکتور<Dist_سطل> collectDistBuckets(AstDist* ثابت distp) {
        File_سطر* ثابت fl = distp->fileline();
        std::وکتور<Dist_سطل> buckets;
        لكل (Ast_Dist_قلم* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), Dist_قلم)) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(ditemp->weightp(), Const)) {
                إذا (constp->مقصد_UQuad() == 0) استمر;
            }
            Ast_گره_عبارت* وزن_Exprp
                = جديد AstExtend{fl, ditemp->weightp()->clone_درخت_خالص(خطأ), 64};
            // := on a range weights every element, so scale by the range size.
            إذا (!ditemp->is_کامل()) {
                إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ثابت AstConst* ثابت lop = VN_CAST(irp->lhsp(), Const);
                    ثابت AstConst* ثابت hip = VN_CAST(irp->rhsp(), Const);
                    Ast_گره_عبارت* rangeSizep;
                    إذا (lop && hip) {
                        ثابت uint64_t range_اندازه = hip->مقصد_UQuad() - lop->مقصد_UQuad() + 1;
                        rangeSizep = جديد AstConst{fl, AstConst::Unsized64{}, range_اندازه};
                    } وإلا {
                        rangeSizep = جديد Ast_جمع_کن{
                            fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                            جديد AstSub{fl,
                                       جديد AstExtend{fl, irp->rhsp()->clone_درخت_خالص(خطأ), 64},
                                       جديد AstExtend{fl, irp->lhsp()->clone_درخت_خالص(خطأ), 64}}};
                    }
                    وزن_Exprp = جديد AstMul{fl, وزن_Exprp, rangeSizep};
                }
            }
            buckets.افزودن_پشت({ditemp->rangep(), وزن_Exprp});
        }
        أرجع buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ساكن Ast_گره_عبارت* new_Dist_Membership_جمله(AstDist* distp, Ast_گره_عبارت* rangep) {
        إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            أرجع newDistRangeMembership(distp, irp);
        }
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* ثابت eqExprp = distp->exprp()->clone_درخت_خالص(خطأ);
        eqExprp->user1(درست);
        Ast_گره_عبارت* ثابت eqp = جديد AstEq{fl, eqExprp, rangep->clone_درخت_خالص(خطأ)};
        eqp->user1(درست);
        أرجع eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_عبارت* buildDistMembership(AstDist* distp,
                                           ثابت std::وکتور<Dist_سطل>& buckets) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* unionExprp = اشاره_تهی;
        لكل (ثابت خودکار& سطل : buckets) {
            Ast_گره_عبارت* ثابت memberp = new_Dist_Membership_جمله(distp, سطل.rangep);
            إذا (!unionExprp) {
                unionExprp = memberp;
            } وإلا {
                unionExprp = جديد AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(درست);
            }
        }
        أرجع جديد Ast_Constraint_عبارت{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_گره_عبارت* newGateJoin(File_سطر* fl, Ast_گره_عبارت* ap, Ast_گره_عبارت* bp, منطقي isAnd) {
        إذا (ap->same_درخت(bp)) {
            VL_DO_DANGLING(افزودن_Deletep(bp), bp);
            أرجع ap;
        }
        إذا (VN_IS(bp, Const)) std::جابجا(ap, bp);
        إذا (VN_IS(ap, Const)) {
            ثابت منطقي constWins = VN_AS(ap, Const)->isZero() == isAnd;
            إذا (!constWins) std::جابجا(ap, bp);
            VL_DO_DANGLING(افزودن_Deletep(bp), bp);
            أرجع ap;
        }
        إذا (isAnd) أرجع جديد AstLogAnd{fl, ap, bp};
        أرجع جديد AstLogOr{fl, ap, bp};
    }
    Ast_گره_عبارت* newGateOr(File_سطر* fl, Ast_گره_عبارت* ap, Ast_گره_عبارت* bp) {
        أرجع newGateJoin(fl, ap, bp, خطأ);
    }
    Ast_گره_عبارت* newGateAnd(File_سطر* fl, Ast_گره_عبارت* ap, Ast_گره_عبارت* bp) {
        أرجع newGateJoin(fl, ap, bp, درست);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_گره_عبارت* new_نما_بیت_Read(AstVar* varp, ثابت AstMemberSel* mselp, AstVar* rand_نما_Varp,
                                File_سطر* fl) {
        Ast_گره_عبارت* arrayp = اشاره_تهی;
        إذا (varp->lifetime().isStatic()) {
            AstClass* ثابت ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ثابت smodep = get_Static_Rand_نما_Var(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = جديد AstVarRef{fl, VN_AS(smodep->user2p(), گره_Module), smodep, VAccess::READ};
        } وإلا إذا (mselp) {
            AstVar* ثابت memberModep = get_Rand_نما_Var_From_Class(VN_AS(varp->user2p(), گره_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = جديد AstMemberSel{fl, mselp->fromp()->clone_درخت_خالص(خطأ), memberModep};
        } وإلا {
            UASSERT_OBJ(rand_نما_Varp, varp, "rand_mode variable without a class mode array");
            arrayp = جديد AstVarRef{fl, VN_AS(rand_نما_Varp->user2p(), گره_Module), rand_نما_Varp,
                                   VAccess::READ};
        }
        ثابت Randomize_نما rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ثابت atp
            = جديد AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, جديد AstConst{fl, rmode.شاخص}};
        atp->dtype_مجموعه_UInt32();
        أرجع atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_گره_عبارت* newVarGate(AstVar* varp, ثابت AstMemberSel* mselp, منطقي ownerLevel,
                            AstVar* rand_نما_Varp, File_سطر* fl) {
        إذا (!varp->rand().isRandomizable()) {
            أرجع جديد AstConst{fl, AstConst::بیت_True{}, ownerLevel};
        }
        ثابت Randomize_نما rmode = {.asUQuad = varp->user1()};
        إذا (!rmode.استفاده_میکند_نما) أرجع جديد AstConst{fl, AstConst::بیت_True{}};
        أرجع new_نما_بیت_Read(varp, mselp, rand_نما_Varp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_گره_عبارت* newAccessGate(ثابت Ast_گره* nodep, منطقي ownerLevel, AstVar* rand_نما_Varp,
                               File_سطر* fl) {
        إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel)) {
            أرجع newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, rand_نما_Varp, fl),
                              newAccessGate(mselp->fromp(), درست, rand_نما_Varp, fl));
        }
        إذا (ثابت Ast_گره_Var_Ref* ثابت refp = VN_CAST(nodep, گره_Var_Ref)) {
            أرجع newVarGate(refp->varp(), اشاره_تهی, ownerLevel, rand_نما_Varp, fl);
        }
        إذا (ثابت Ast_گره_Sel* ثابت selp = VN_CAST(nodep, گره_Sel)) {
            أرجع newAccessGate(selp->fromp(), درست, rand_نما_Varp, fl);
        }
        إذا (dist_Bound_Refs_نما_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        أرجع جديد AstConst{fl, AstConst::بیت_True{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_گره_عبارت* newDistGate(ثابت Ast_گره* nodep, AstVar* rand_نما_Varp, File_سطر* fl) {
        إذا (VN_IS(nodep, گره_Var_Ref) || VN_IS(nodep, MemberSel)) {
            أرجع newAccessGate(nodep, خطأ, rand_نما_Varp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        إذا (ثابت Ast_گره_Sel* ثابت selp = VN_CAST(nodep, گره_Sel)) {
            أرجع newDistGate(selp->fromp(), rand_نما_Varp, fl);
        }
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            أرجع newDistGate(selp->fromp(), rand_نما_Varp, fl);
        }
        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            أرجع newDistGate(selp->fromp(), rand_نما_Varp, fl);
        }
        إذا (ثابت AstCMethodHard* ثابت methodp = VN_CAST(nodep, CMethodHard)) {
            إذا (methodp->روش() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                أرجع newDistGate(methodp->fromp(), rand_نما_Varp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        إذا (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            Ast_گره_عبارت* ثابت thenGatep = newDistGate(condp->thenp(), rand_نما_Varp, fl);
            Ast_گره_عبارت* ثابت elseGatep = newDistGate(condp->elsep(), rand_نما_Varp, fl);
            Ast_گره_عبارت* armGatep = thenGatep;
            إذا (thenGatep->same_درخت(elseGatep)) {
                VL_DO_DANGLING(افزودن_Deletep(elseGatep), elseGatep);
            } وإلا {
                armGatep
                    = جديد AstCond{fl, condp->condp()->clone_درخت_خالص(خطأ), thenGatep, elseGatep};
            }
            أرجع newGateOr(fl, newDistGate(condp->condp(), rand_نما_Varp, fl), armGatep);
        }
        إذا (ثابت Ast_گره_Uniop* ثابت uopp = VN_CAST(nodep, گره_Uniop)) {
            أرجع newDistGate(uopp->lhsp(), rand_نما_Varp, fl);
        }
        إذا (ثابت Ast_گره_Biop* ثابت bopp = VN_CAST(nodep, گره_Biop)) {
            أرجع newGateOr(fl, newDistGate(bopp->lhsp(), rand_نما_Varp, fl),
                             newDistGate(bopp->rhsp(), rand_نما_Varp, fl));
        }
        إذا (dist_Bound_Refs_نما_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        أرجع جديد AstConst{fl, AstConst::بیت_True{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_عبارت* buildFrozenMembership(AstDist* distp,
                                             ثابت std::وکتور<Dist_سطل>& buckets) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* unionExprp = اشاره_تهی;
        لكل (ثابت خودکار& سطل : buckets) {
            Ast_گره_عبارت* termp = new_Dist_Membership_جمله(distp, سطل.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ثابت منطقي runtime_وزن
                = سطل.وزن_Exprp->exists([](ثابت Ast_گره_Var_Ref*) { أرجع درست; });
            إذا (runtime_وزن) {
                AstNeq* ثابت nzp = جديد AstNeq{fl, سطل.وزن_Exprp->clone_درخت_خالص(خطأ),
                                               جديد AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(درست);
                termp = جديد AstLogAnd{fl, termp, nzp};
                termp->user1(درست);
            }
            إذا (unionExprp) {
                unionExprp = جديد AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(درست);
            } وإلا {
                unionExprp = termp;
            }
        }
        أرجع جديد Ast_Constraint_عبارت{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_گره* gateFrozenDist(Ast_گره* chainp, AstDist* distp,
                            ثابت std::وکتور<Dist_سطل>& buckets, AstVar* rand_نما_Varp) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* gatep = newDistGate(distp->exprp(), rand_نما_Varp, fl);
        إذا (VN_IS(gatep, Const)) {
            ثابت منطقي drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(افزودن_Deletep(gatep), gatep);
            إذا (drawable) أرجع chainp;
            VL_DO_DANGLING(افزودن_Deletep(chainp), chainp);
            أرجع buildFrozenMembership(distp, buckets);
        }
        أرجع جديد AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_گره_عبارت* new_یکنواخت_Range_Pick(AstDist* distp, ثابت AstInsideRange* irp) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* ثابت dist_عبارت_Copyp = distp->exprp()->clone_درخت_خالص(خطأ);
        dist_عبارت_Copyp->user1(درست);
        ثابت صحيح dist_عرض = distp->exprp()->عرض();
        ثابت AstConst* ثابت lopC = VN_CAST(irp->lhsp(), Const);
        ثابت AstConst* ثابت hipC = VN_CAST(irp->rhsp(), Const);
        Ast_گره_عبارت* rangeSzp;
        إذا (lopC && hipC) {
            ثابت uint64_t rsz = hipC->مقصد_UQuad() - lopC->مقصد_UQuad() + 1;
            rangeSzp = جديد AstConst{fl, AstConst::Unsized64{}, rsz};
        } وإلا {
            ثابت منطقي isSigned = irp->lhsp()->isSigned();
            Ast_گره_عبارت* ثابت lo64p
                = isSigned ? static_cast<Ast_گره_عبارت*>(
                                 جديد AstExtendS{fl, irp->lhsp()->clone_درخت_خالص(خطأ), 64})
                           : static_cast<Ast_گره_عبارت*>(
                                 جديد AstExtend{fl, irp->lhsp()->clone_درخت_خالص(خطأ), 64});
            Ast_گره_عبارت* ثابت hi64p
                = isSigned ? static_cast<Ast_گره_عبارت*>(
                                 جديد AstExtendS{fl, irp->rhsp()->clone_درخت_خالص(خطأ), 64})
                           : static_cast<Ast_گره_عبارت*>(
                                 جديد AstExtend{fl, irp->rhsp()->clone_درخت_خالص(خطأ), 64});
            rangeSzp = جديد Ast_جمع_کن{fl, جديد AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  جديد AstSub{fl, hi64p, lo64p}};
        }
        Ast_گره_عبارت* ثابت rand64p = جديد AstRand{fl, اشاره_تهی, خطأ};
        rand64p->dtype_مجموعه_UInt64();
        Ast_گره_عبارت* ثابت offsetp
            = جديد AstCCast{fl, جديد AstModDiv{fl, rand64p, rangeSzp}, dist_عرض};
        Ast_گره_عبارت* ثابت valuep = جديد Ast_جمع_کن{fl, irp->lhsp()->clone_درخت_خالص(خطأ), offsetp};
        Ast_گره_عبارت* ثابت eqp = جديد AstEq{fl, dist_عبارت_Copyp, valuep};
        eqp->user1(درست);
        أرجع eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_گره* build_Weighted_سطل_Chain(AstDist* distp, ثابت std::وکتور<Dist_سطل>& buckets,
                                      AstVar* سطل_Varp,
                                      ثابت std::وکتور<Ast_گره_عبارت*>& cumSums) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گره_عبارت* chainp = اشاره_تهی;
        لكل (صحيح i = static_cast<صحيح>(buckets.اندازه()) - 1; i >= 0; --i) {
            Ast_گره_عبارت* constraintExprp;
            ثابت AstInsideRange* ثابت irp = VN_CAST(buckets[i].rangep, InsideRange);
            إذا (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } وإلا إذا (irp) {
                constraintExprp = new_یکنواخت_Range_Pick(distp, irp);
            } وإلا {
                Ast_گره_عبارت* ثابت dist_عبارت_Copyp = distp->exprp()->clone_درخت_خالص(خطأ);
                dist_عبارت_Copyp->user1(درست);
                constraintExprp
                    = جديد AstEq{fl, dist_عبارت_Copyp, buckets[i].rangep->clone_درخت_خالص(خطأ)};
                constraintExprp->user1(درست);
            }
            إذا (!chainp) {
                chainp = constraintExprp;
            } وإلا {
                Ast_گره_عبارت* ثابت condp
                    = جديد AstLte{fl, جديد AstVarRef{fl, سطل_Varp, VAccess::READ}, cumSums[i]};
                condp->user1(درست);
                chainp = جديد AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(درست);
            }
        }
        Ast_Constraint_عبارت* ثابت chainExprp = جديد Ast_Constraint_عبارت{fl, chainp};
        chainExprp->isSoft(درست);
        أرجع chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    فراغ پایینی_Dist_Constraints(AstTask* taskp, Ast_گره* constrItemsp, AstVar* rand_نما_Varp,
                              AstConstraintForeach* foreachp = اشاره_تهی) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_گره* foreachTailp = اشاره_تهی;
        خودکار جمع_کن_Stmt = [&](Ast_گره* nodep) {
            إذا (foreachp) {
                إذا (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } وإلا {
                    foreachTailp->جمع_کن_بعدی(nodep);
                    foreachTailp = nodep;
                }
            } وإلا {
                taskp->جمع_کن_Stmtsp(nodep);
            }
        };

        لكل (Ast_گره *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            إذا (AstConstraintIf* ثابت cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                پایینی_Dist_Constraints(taskp, cifp->thensp(), rand_نما_Varp, foreachp);
                إذا (cifp->elsesp()) {
                    پایینی_Dist_Constraints(taskp, cifp->elsesp(), rand_نما_Varp, foreachp);
                }
                استمر;
            }
            إذا (AstConstraintForeach* ثابت cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                پایینی_Dist_Constraints(taskp, cfep->bodyp(), rand_نما_Varp, cfep);
                استمر;
            }

            Ast_Constraint_عبارت* ثابت constrExprp = VN_CAST(itemp, Constraint_عبارت);
            إذا (!constrExprp) استمر;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            إذا (AstLogIf* ثابت بالاترین_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_گره* chainEndp = بالاترین_Log_Ifp->rhsp();
                طالما (AstLogIf* ثابت innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                إذا (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ثابت liftedp = lift_Log_If_Chain_مقصد_Constraint_If(بالاترین_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(افزودن_Deletep(constrExprp), constrExprp);
                    پایینی_Dist_Constraints(taskp, liftedp->thensp(), rand_نما_Varp, foreachp);
                    استمر;
                }
            }

            AstDist* ثابت distp = VN_CAST(constrExprp->exprp(), Dist);
            إذا (!distp) استمر;

            File_سطر* ثابت fl = distp->fileline();

            std::وکتور<Dist_سطل> buckets = collectDistBuckets(distp);

            إذا (buckets.خالی()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_عبارت* ثابت truep
                    = جديد Ast_Constraint_عبارت{fl, جديد AstConst{fl, AstConst::بیت_True{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(افزودن_Deletep(constrExprp), constrExprp);
                استمر;
            }

            Ast_Constraint_عبارت* ثابت membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_گره_عبارت* مجموع_وزن_Exprp = اشاره_تهی;
            لكل (خودکار& سطل : buckets) {
                إذا (!مجموع_وزن_Exprp) {
                    مجموع_وزن_Exprp = سطل.وزن_Exprp->clone_درخت_خالص(خطأ);
                } وإلا {
                    مجموع_وزن_Exprp = جديد Ast_جمع_کن{fl, مجموع_وزن_Exprp,
                                                  سطل.وزن_Exprp->clone_درخت_خالص(خطأ)};
                    مجموع_وزن_Exprp->dtype_مجموعه_UInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ثابت صحيح distId = m_dist_رقم++;
            ثابت std::string مجموع_Name = "__Vdist_total" + cvt_مقصد_Str(distId);
            AstVar* ثابت مجموع_Varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, مجموع_Name, taskp->پیدا_کن_UInt64_DType()};
            مجموع_Varp->noSubst(درست);
            مجموع_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            مجموع_Varp->funcLocal(درست);
            مجموع_Varp->is_داخلی(درست);
            جمع_کن_Stmt(مجموع_Varp);
            جمع_کن_Stmt(
                جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, مجموع_Varp, VAccess::WRITE}, مجموع_وزن_Exprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ثابت std::string سطل_Name = "__Vdist_bucket" + cvt_مقصد_Str(distId);
            AstVar* ثابت سطل_Varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, سطل_Name, taskp->پیدا_کن_UInt64_DType()};
            سطل_Varp->noSubst(درست);
            سطل_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            سطل_Varp->funcLocal(درست);
            سطل_Varp->is_داخلی(درست);
            جمع_کن_Stmt(سطل_Varp);

            Ast_گره_عبارت* ثابت randp = جديد AstRand{fl, اشاره_تهی, خطأ};
            randp->dtype_مجموعه_UInt64();
            جمع_کن_Stmt(جديد Ast_تخصیص_بده{
                fl, جديد AstVarRef{fl, سطل_Varp, VAccess::WRITE},
                جديد Ast_جمع_کن{
                    fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                    جديد AstModDiv{fl, randp, جديد AstVarRef{fl, مجموع_Varp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::وکتور<Ast_گره_عبارت*> cumSums;
            Ast_گره_عبارت* runningSump = اشاره_تهی;
            لكل (اندازه_t i = 0; i < buckets.اندازه(); ++i) {
                إذا (!runningSump) {
                    runningSump = buckets[i].وزن_Exprp->clone_درخت_خالص(خطأ);
                } وإلا {
                    runningSump = جديد Ast_جمع_کن{fl, runningSump,
                                             buckets[i].وزن_Exprp->clone_درخت_خالص(خطأ)};
                    runningSump->dtype_مجموعه_UInt64();
                }
                cumSums.افزودن_پشت(runningSump->clone_درخت_خالص(درست));
            }

            Ast_گره* chainp = build_Weighted_سطل_Chain(distp, buckets, سطل_Varp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, rand_نما_Varp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(افزودن_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->جمع_کن_اینجا_This_As_بعدی(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            لكل (خودکار& سطل : buckets) {
                VL_DO_DANGLING(افزودن_Deletep(سطل.وزن_Exprp), سطل.وزن_Exprp);
            }
            VL_DO_DANGLING(افزودن_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            افزودن_Deletep(cumSums.پشت());
        }
    }

    ساكن منطقي is_Dyn_Arr_از_Class_Type_Recurse(ثابت Ast_گره_DType* ثابت dtypep) {
        ثابت Ast_گره_DType* ثابت refp = dtypep->skipRefp();
        إذا (VN_IS(refp, DynArrayDType) || VN_IS(refp, صف_DType)) {
            أرجع is_Dyn_Arr_از_Class_Type_Recurse(refp->subDTypep());
        } وإلا إذا (VN_IS(refp, ClassRefDType)) {
            أرجع درست;
        }
        أرجع خطأ;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    فراغ جمع_کن_Array_Elem_Refresh(File_سطر* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                             AstVar* ثابت genp) {
        Ast_گره_Module* ثابت genModp = VN_AS(genp->user2p(), گره_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(خطأ).دومی;
        إذا (is_Dyn_Arr_از_Class_Type_Recurse(arrVarp->dtypep())) {
            إذا (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            أرجع;
        }
        AstCMethodHard* ثابت methodp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_مجموعه_Void();

        Ast_گره_Module* ثابت classp = VN_AS(arrVarp->user2p(), گره_Module);
        AstVarRef* ثابت varRefp = جديد AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->جمع_کن_Pinsp(varRefp);

        ثابت اندازه_t عرض = arrayElementDTypep(arrVarp->dtypep())->عرض();

        methodp->جمع_کن_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, عرض});
        Ast_گره_عبارت* ثابت varnamep
            = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + arrVarp->name() + "\"", arrVarp->عرض()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->جمع_کن_Pinsp(varnamep);
        methodp->جمع_کن_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ثابت Randomize_نما rmode = {.asUQuad = arrVarp->user1()};
        إذا (rmode.استفاده_میکند_نما) {
            methodp->جمع_کن_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, rmode.شاخص});
        }

        randomizep->جمع_کن_Stmtsp(methodp->makeStmt());
    }

    Ast_گره_Stmt* new_Pin_اندازه_Variable(File_سطر* ثابت fl, AstVar* ثابت arrVarp,
                                    AstVar* ثابت genp) {
        Ast_گره_Module* ثابت genModp = VN_AS(genp->user2p(), گره_Module);
        AstVar* ثابت اندازه_Varp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ثابت pinp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtype_مجموعه_Void();
        AstCExpr* ثابت namep = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + اندازه_Varp->name() + "\""};
        namep->dtype_مجموعه_UInt32();
        pinp->جمع_کن_Pinsp(namep);
        pinp->جمع_کن_Pinsp(
            جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(اندازه_Varp->عرض())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ثابت اندازه_Var_Refp = جديد AstVarRef{fl, اندازه_Varp, VAccess::READ};
        اندازه_Var_Refp->classOrPackagep(VN_AS(اندازه_Varp->user2p(), گره_Module));
        pinp->جمع_کن_Pinsp(اندازه_Var_Refp);
        أرجع pinp->makeStmt();
    }

    فراغ جمع_کن_Frozen_اندازه_Pin(File_سطر* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                          AstVar* ثابت genp, AstVar* ثابت rand_نما_Varp) {
        ثابت Randomize_نما rmode = {.asUQuad = arrVarp->user1()};
        إذا (!rmode.استفاده_میکند_نما) أرجع;
        AstVar* ثابت اندازه_Varp = VN_AS(arrVarp->user4p(), Var);
        Ast_گره_Module* ثابت arrClassp = VN_AS(arrVarp->user2p(), گره_Module);
        Ast_گره_Module* ثابت اندازه_Classp = VN_AS(اندازه_Varp->user2p(), گره_Module);
        AstVarRef* ثابت اندازه_Writep = جديد AstVarRef{fl, اندازه_Classp, اندازه_Varp, VAccess::WRITE};
        ثابت VCMethod اندازه_روش = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_اندازه
                                        : VCMethod::DYN_اندازه;
        AstCMethodHard* ثابت currentSizep = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, اندازه_روش};
        currentSizep->dtypep(اندازه_Varp->dtypep());
        randomizep->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, اندازه_Writep, currentSizep});
        Ast_گره_عبارت* ثابت modep = new_نما_بیت_Read(arrVarp, اشاره_تهی, rand_نما_Varp, fl);
        randomizep->جمع_کن_Stmtsp(
            جديد AstIf{fl, جديد AstLogNot{fl, modep}, new_Pin_اندازه_Variable(fl, arrVarp, genp)});
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_رقم);
        VL_RESTORER_COPY(m_rand_Array_Unique_نامها);
        m_modp = nodep;
        m_rand_Case_رقم = 0;
        m_rand_Array_Unique_نامها.reset();
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_رقم);
        VL_RESTORER(m_dist_رقم);
        m_modp = nodep;
        m_rand_Case_رقم = 0;
        m_dist_رقم = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        إذا (!nodep->user1()) أرجع;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->پایه_Most_Classp()->needRNG(درست);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ثابت منطقي basic_اولی = classOwnsGlobalConstraint(nodep);

        File_سطر* ثابت fl = nodep->fileline();
        AstFunc* ثابت randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ثابت fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ثابت rand_نما_Varp = get_Rand_نما_Var_From_Class(nodep);
        جمع_کن_Pre_Post_Call(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(nodep)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->جمع_کن_Stmtsp((جديد AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_گره_عبارت* beginValp = اشاره_تهی;
        AstVar* genp = getRandomGenerator(nodep);
        إذا (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ثابت prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                إذا (!taskp) {
                    taskp = new_راهاندازی_Constraint_Task(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                إذا (AstTask* ثابت resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ثابت resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ثابت resizeTaskRefp
                        = جديد AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->جمع_کن_Stmtsp(resizeTaskRefp->makeStmt());
                }

                إذا (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                إذا (constrp->itemsp()) {
                    پایینی_Dist_Constraints(taskp, constrp->itemsp(), rand_نما_Varp);
                }
                VInsertion_مجموعه<AstVar*>& اندازه_Arrays = m_اندازه_Constrained_Arrays[classp];
                Constraint_عبارت_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), اشاره_تهی,
                    genp,          rand_نما_Varp, m_writtenVars,     &m_uniqueConstraintId,
                    m_unique_نامها, randomizep,   &اندازه_Arrays,       prepareArraysTaskp};
                إذا (constrp->itemsp()) {
                    taskp->جمع_کن_Stmtsp(wrap_If_Constraint_نما(
                        nodep, constrp, constrp->itemsp()->unlink_Fr_پشت_With_بعدی()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ثابت خودکار constraint_عمق = [](ثابت AstConstraint* constrp) {
                    صحيح عمق = 0;
                    اندازه_t pos = 0;
                    طالما ((pos = constrp->name().پیدا_کن(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++عمق;
                        pos += std::طول_رشته(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    أرجع عمق;
                };
                صحيح max_عمق = 0;
                nodep->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    max_عمق = std::max(max_عمق, constraint_عمق(constrp));
                });
                AstTask* ثابت راهاندازی_All_Taskp = get_Create_Constraint_راهاندازی_Func(nodep, m_memberMap);
                لكل (صحيح d = max_عمق; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                            إذا (constraint_عمق(constrp) != d) أرجع;
                            AstTask* ثابت taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ثابت راهاندازی_Task_Refp
                                = جديد AstTaskRef{constrp->fileline(), taskp};
                            راهاندازی_Task_Refp->classOrPackagep(classp);
                            راهاندازی_All_Taskp->جمع_کن_Stmtsp(راهاندازی_Task_Refp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertion_مجموعه<AstVar*> اندازه_Array_Vars;
            إذا (nodep->extendsp()) {
                AstClass* والد_Classp = nodep->extendsp()->classp();
                طالما (والد_Classp) {
                    ثابت خودکار اندازه_Arrays_It = m_اندازه_Constrained_Arrays.پیدا_کن(والد_Classp);
                    إذا (اندازه_Arrays_It != m_اندازه_Constrained_Arrays.انتها()) {
                        لكل (AstVar* ثابت arrVarp : اندازه_Arrays_It->دومی) {
                            اندازه_Array_Vars.درج(arrVarp);
                        }
                    }
                    AstFunc* ثابت والد_Randomizep
                        = VN_CAST(m_memberMap.پیدا_کن_Member(والد_Classp, "randomize"), Func);
                    إذا (والد_Randomizep && والد_Randomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        لكل (Ast_گره* stmtp = والد_Randomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            منطقي یافت_Clear_Constraints = خطأ;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                إذا (methodp->روش() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->روش() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->جمع_کن_Stmtsp(stmtp->clone_درخت(خطأ));
                                } وإلا إذا (methodp->روش()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    یافت_Clear_Constraints = درست;
                                }
                            });
                            إذا (یافت_Clear_Constraints) اكسر;
                        }
                    }
                    والد_Classp
                        = والد_Classp->extendsp() ? والد_Classp->extendsp()->classp() : اشاره_تهی;
                }
            }
            randomizep->جمع_کن_Stmtsp(implementConstraintsClear(fl, genp));

            ثابت خودکار اندازه_Arrays_It = m_اندازه_Constrained_Arrays.پیدا_کن(nodep);
            إذا (اندازه_Arrays_It != m_اندازه_Constrained_Arrays.انتها()) {
                لكل (AstVar* ثابت arrVarp : اندازه_Arrays_It->دومی) اندازه_Array_Vars.درج(arrVarp);
            }
            AstTask* ثابت راهاندازی_All_Taskp = get_Create_Constraint_راهاندازی_Func(nodep, m_memberMap);
            إذا (!اندازه_Array_Vars.خالی() && prepareArraysTaskp->stmtsp()) {
                Ast_گره_Stmt* ثابت preparep = (جديد AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                لكل (Ast_گره* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    منطقي writesArray = خطأ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->روش() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    إذا (!writesArray) استمر;
                    stmtp->جمع_کن_اینجا_This_As_بعدی(preparep);
                    اكسر;
                }
            }
            AstTaskRef* ثابت راهاندازی_Task_Refp = جديد AstTaskRef{fl, راهاندازی_All_Taskp};
            randomizep->جمع_کن_Stmtsp(راهاندازی_Task_Refp->makeStmt());

            Ast_گره_Module* ثابت genModp = VN_AS(genp->user2p(), گره_Module);

            AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
            solverCallp->dtype_مجموعه_بیت();
            solverCallp->جمع_کن(جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->جمع_کن(".next(__Vm_rng)");
            إذا (!اندازه_Array_Vars.خالی()) {
                AstVar* ثابت اندازه_Ok_Varp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->پیدا_کن_Basic_DType(VBasicDTypeKwd::بیت)};
                اندازه_Ok_Varp->funcLocal(درست);
                randomizep->جمع_کن_Stmtsp(اندازه_Ok_Varp);

                AstVar* ثابت finalOkVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->پیدا_کن_Basic_DType(VBasicDTypeKwd::بیت)};
                finalOkVarp->funcLocal(درست);
                randomizep->جمع_کن_Stmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                لكل (AstVar* ثابت arrVarp : اندازه_Array_Vars)
                    جمع_کن_Frozen_اندازه_Pin(fl, arrVarp, randomizep, genp, rand_نما_Varp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->جمع_کن_Stmtsp(
                    جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, اندازه_Ok_Varp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                إذا (AstTask* ثابت resizeAllTaskp
                    = VN_AS(m_memberMap.پیدا_کن_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ثابت resizeTaskRefp = جديد AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ثابت ifp = جديد AstIf{fl, جديد AstVarRef{fl, اندازه_Ok_Varp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), اشاره_تهی};
                    randomizep->جمع_کن_Stmtsp(ifp);
                }

                // Refresh array element tables after resize
                لكل (ثابت خودکار& arrVarp : اندازه_Array_Vars) {
                    جمع_کن_Array_Elem_Refresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->جمع_کن_Stmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ثابت راهاندازی_Task_Refp2 = جديد AstTaskRef{fl, راهاندازی_All_Taskp};
                randomizep->جمع_کن_Stmtsp(راهاندازی_Task_Refp2->makeStmt());

                لكل (AstVar* ثابت arrVarp : اندازه_Array_Vars) {
                    randomizep->جمع_کن_Stmtsp(new_Pin_اندازه_Variable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ثابت solverCallp2 = جديد AstCExpr{fl};
                solverCallp2->dtype_مجموعه_بیت();
                solverCallp2->جمع_کن(جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->جمع_کن(".next(__Vm_rng)");
                randomizep->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
                    fl, جديد AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = جديد AstAnd{fl, جديد AstVarRef{fl, اندازه_Ok_Varp, VAccess::READ},
                                       جديد AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } وإلا {
                beginValp = solverCallp;
            }
            إذا (rand_نما_Varp) {
                Ast_گره_Module* ثابت rand_نما_Classp = VN_AS(rand_نما_Varp->user2p(), Class);
                Ast_گره_FTask* ثابت newp
                    = VN_AS(m_memberMap.پیدا_کن_Member(rand_نما_Classp, "new"), گره_FTask);
                UASSERT_OBJ(newp, rand_نما_Classp, "No new() in class");
                جمع_کن_مجموعه_Rand_نما(newp, genp, rand_نما_Varp);
            }
            إذا (AstVar* ثابت static_Rand_نما_Varp = get_Static_Rand_نما_Var(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_گره_FTask* ثابت newp = VN_AS(m_memberMap.پیدا_کن_Member(nodep, "new"), گره_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                جمع_کن_مجموعه_Static_Rand_نما(newp, genp, static_Rand_نما_Varp);
            }
        } وإلا {
            beginValp = جديد AstConst{fl, AstConst::Widthed_مقدار{}, 32, 1};
        }

        جمع_کن_Update_Rand_Vars_Body(nodep, randomizep);

        AstFunc* ثابت basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        جمع_کن_Basic_Randomize_Body(basicRandomizep, nodep, rand_نما_Varp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ثابت fvarRefp = جديد AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
            fl, fvarRefp, basic_اولی ? جديد AstFuncRef{fl, basicRandomizep} : beginValp});

        ثابت خودکار اندازه_Arrays_It = m_اندازه_Constrained_Arrays.پیدا_کن(nodep);
        ثابت منطقي needs_اندازه_Phase
            = اندازه_Arrays_It != m_اندازه_Constrained_Arrays.انتها() && !اندازه_Arrays_It->دومی.خالی();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ثابت خودکار emitResizeFallback = [&]() {
            إذا (needs_اندازه_Phase) أرجع;
            إذا (AstTask* ثابت resizeAllTaskp
                = VN_AS(m_memberMap.پیدا_کن_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ثابت resizeTaskRefp = جديد AstTaskRef{fl, resizeAllTaskp};
                randomizep->جمع_کن_Stmtsp(resizeTaskRefp->makeStmt());
            }
        };
        إذا (!basic_اولی) emitResizeFallback();

        AstVarRef* ثابت fvarRefReadp = fvarRefp->clone_درخت(خطأ);
        fvarRefReadp->access(VAccess::READ);

        Ast_گره_عبارت* ثابت دومی_Halfp
            = basic_اولی ? beginValp : جديد AstFuncRef{fl, basicRandomizep};
        randomizep->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, fvarRefp->clone_درخت(خطأ),
                                            جديد AstAnd{fl, fvarRefReadp, دومی_Halfp}});

        إذا (basic_اولی) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(nodep)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->جمع_کن_Stmtsp((جديد AstTaskRef{fl, postTaskp})->makeStmt());
        }

        جمع_کن_Pre_Post_Call(nodep, randomizep, "post_randomize");
        nodep->user1(خطأ);
    }
    فراغ بازدید(AstRandCase* nodep) override {
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
        Ast_گره_DType* ثابت جمع_DTypep = nodep->پیدا_کن_UInt64_DType();

        File_سطر* ثابت fl = nodep->fileline();
        ثابت std::string name = "__Vrandcase" + cvt_مقصد_Str(m_rand_Case_رقم++);
        AstVar* ثابت randVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, name, جمع_DTypep};
        randVarp->noSubst(درست);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        جمع_کن_موقت_Varsp(randVarp);
        Ast_گره_عبارت* sump = جديد AstConst{fl, AstConst::Widthed_مقدار{}, 64, 0};
        Ast_گره_If* ثابت اولی_Ifsp
            = جديد AstIf{fl, جديد AstConst{fl, AstConst::بیت_False{}}, اشاره_تهی, اشاره_تهی};
        Ast_گره_If* ifsp = اولی_Ifsp;

        لكل (Ast_Case_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_قلم)) {
            Ast_گره_عبارت* ثابت condp = itemp->condsp()->unlink_Fr_پشت();
            sump
                = جديد Ast_جمع_کن{condp->fileline(), sump, جديد AstExtend{itemp->fileline(), condp, 64}};
            Ast_گره* ثابت stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlink_Fr_پشت_With_بعدی() : اشاره_تهی;
            AstVarRef* ثابت randVarRefp = جديد AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_گره_If* ثابت newifp
                = جديد AstIf{itemp->fileline(),
                            جديد AstLte{condp->fileline(), randVarRefp, sump->clone_درخت_خالص(درست)},
                            stmtsp, اشاره_تهی};
            ifsp->جمع_کن_Elsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ثابت dispp = جديد AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            اشاره_تهی, اشاره_تهی};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->جمع_کن_Elsesp(dispp);

        Ast_گره_عبارت* ثابت randp = جديد AstRand{fl, اشاره_تهی, خطأ};
        randp->dtype_مجموعه_UInt64();
        AstVarRef* ثابت randVarRefp = جديد AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_گره* ثابت newp
            = جديد Ast_تخصیص_بده{fl, randVarRefp,
                            جديد Ast_جمع_کن{fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                                       جديد AstModDiv{fl, randp, sump}}};
        newp->جمع_کن_بعدی(اولی_Ifsp);
        إذا (debug() >= 9) newp->dump_درخت_And_بعدی(خروجی, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        إذا (nodep->name() == "rand_mode") {
            Ast_روش_Call* ثابت روش_Callp = VN_CAST(nodep, روش_Call);
            Ast_گره_عبارت* ثابت fromp = روش_Callp ? روش_Callp->fromp() : اشاره_تهی;
            ثابت Rand_نما_هدف rand_نما_هدف = Rand_نما_هدف::get(fromp, m_modp);
            UASSERT_OBJ(rand_نما_هدف.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ثابت receiverp = rand_نما_هدف.receiverp;
            ثابت منطقي isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_گره* classLevelStaticLoopp = اشاره_تهی;
            إذا (isClassLevel && nodep->argsp()) {
                إذا (AstVar* ثابت sVarp = get_Static_Rand_نما_Var(rand_نما_هدف.classp)) {
                    File_سطر* ثابت fl = nodep->fileline();
                    Ast_گره_عبارت* ثابت staticLhsp
                        = make_نما_تخصیص_بده_Lhs(fl, rand_نما_هدف.classp, اشاره_تهی, sVarp);
                    Ast_گره_عبارت* ثابت argClonep = nodep->argsp()->exprp()->clone_درخت_خالص(خطأ);
                    classLevelStaticLoopp = make_نما_مجموعه_Loop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ثابت منطقي receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ثابت rand_نما_Varp = receiverIsStaticRand
                                             ? get_Static_Rand_نما_Var(rand_نما_هدف.classp)
                                             : get_Rand_نما_Var_From_Class(rand_نما_هدف.classp);
            إذا (!rand_نما_Varp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_عبارت), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(افزودن_Deletep(m_stmtp), m_stmtp = اشاره_تهی);
                أرجع;
            }
            Ast_گره_عبارت* ثابت lhsp = make_نما_تخصیص_بده_Lhs(nodep->fileline(), rand_نما_هدف.classp,
                                                        rand_نما_هدف.fromp, rand_نما_Varp);
            replace_With_نما_تخصیص_بده_And_Append(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : اشاره_تهی, lhsp,
                classLevelStaticLoopp);
            أرجع;
        }

        إذا (nodep->name() == "constraint_mode") {
            Ast_روش_Call* ثابت روش_Callp = VN_CAST(nodep, روش_Call);
            Ast_گره_عبارت* fromp = روش_Callp ? روش_Callp->fromp() : اشاره_تهی;
            AstConstraint* constrp = اشاره_تهی;
            AstClass* classp = VN_CAST(m_modp, Class);
            إذا (AstConstraintRef* ثابت constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                إذا (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } وإلا إذا (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ثابت constraint_نما_Varp = (constrp && constrp->isStatic())
                                                   ? get_Static_Constraint_نما_Var(classp)
                                                   : get_Constraint_نما_Var(classp);
            Ast_گره_عبارت* ثابت lhsp
                = make_نما_تخصیص_بده_Lhs(nodep->fileline(), classp, fromp, constraint_نما_Varp);
            replace_With_نما_تخصیص_بده(nodep, constrp, lhsp);
            أرجع;
        }

        إذا (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            أرجع;
        }

        إذا (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ثابت منطقي درون_Static_Context = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ثابت stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ثابت randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            إذا (درون_Static_Context) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(درست);
            }
            randomizeFuncp->جمع_کن_Stmtsp(
                جديد Ast_تخصیص_بده{nodep->fileline(),
                              جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              جديد AstConst{nodep->fileline(), AstConst::Widthed_مقدار{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            صحيح argn = 0;
            AstWith* ثابت withp = nodep->withp();
            لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_گره_عبارت* ثابت exprp = argp->exprp();
                AstCMethodHard* ثابت basicMethodp = جديد AstCMethodHard{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ثابت refvarp
                    = جديد AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::مقصد_string(++argn), exprp->dtypep()};
                refvarp->مجموعه_Std_Randomize_Arg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                إذا (withp) {
                    withp->foreach([&](Ast_گره_عبارت* نمایی_تابع) {
                        إذا (is_Similar_گره(نمایی_تابع, exprp)) {
                            AstVarRef* ثابت replaceVar
                                = جديد AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            نمایی_تابع->replaceWith(replaceVar);
                            replaceVar->user1(نمایی_تابع->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(افزودن_Deletep(نمایی_تابع), نمایی_تابع);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(درست);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->جمع_کن_Stmtsp(refvarp);

                ثابت اندازه_t عرض = exprp->عرض();
                basicMethodp->جمع_کن_Pinsp(
                    جديد AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->جمع_کن_Pinsp(
                    جديد AstConst{nodep->fileline(), AstConst::Unsized64{}, عرض});
                basicMethodp->dtype_مجموعه_بیت();

                randomizeFuncp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    جديد AstAnd{nodep->fileline(),
                               جديد AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            إذا (withp) {
                File_سطر* ثابت fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, اشاره_تهی,
                                                                درون_Static_Context);
                withCapturep->جمع_کن_Function_Arguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                إذا (stdrand) {
                    randomizeFuncp->جمع_کن_Stmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_گره* ثابت capturedTreep = withp->exprp()->unlink_Fr_پشت_With_بعدی();
                randomizeFuncp->جمع_کن_Stmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_عبارت_Visitor{
                        اشاره_تهی, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        اشاره_تهی, m_writtenVars, &m_uniqueConstraintId, m_unique_نامها,  اشاره_تهی};
                }
                AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
                solverCallp->dtype_مجموعه_بیت();
                solverCallp->جمع_کن(جديد AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->جمع_کن(".next()");
                AstVar* ثابت fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_گره_عبارت* ثابت andExprp
                    = جديد AstAnd{fl, جديد AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->جمع_کن_Stmtsp(
                    جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlink_Fr_پشت()->delete_درخت(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            إذا (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            إذا (withCapturep) nodep->جمع_کن_Argsp(withCapturep->get_آرگومانها());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            أرجع;
        }
        handle_Randomize_آرگومانها(nodep);

        AstWith* ثابت withp = nodep->withp();
        إذا (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            أرجع;
        }
        withp->unlink_Fr_پشت();
        iterateChildren(nodep);

        AstClass* classp = اشاره_تهی;
        إذا (Ast_روش_Call* ثابت callp = VN_CAST(nodep, روش_Call)) {
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
        } وإلا {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        إذا (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ثابت classGenp = getRandomGenerator(classp);
        AstVar* ثابت localGenp
            = جديد AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->پیدا_کن_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(درست);

        AstFunc* ثابت randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inline_Unique_نامها.get(nodep), خطأ);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ثابت std::pair<منطقي, منطقي> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<منطقي, منطقي>{خطأ, خطأ};
        إذا (prePostWrap.اولی) {
            جمع_کن_Virtual_Pre_Post_Call(randomizeFuncp, classp, "pre_randomize");
        } وإلا {
            جمع_کن_Pre_Post_Call(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(classp)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(classp, "pre");
            إذا (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->جمع_کن_Stmtsp((جديد AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ثابت CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.جمع_کن_Function_Arguments(randomizeFuncp);

        // Add constraints clearing code
        إذا (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ثابت mainRandomizep
                = VN_CAST(m_memberMap.پیدا_کن_Member(classp, "randomize"), Func);
            إذا (mainRandomizep && mainRandomizep->stmtsp()) {
                لكل (Ast_گره* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    منطقي یافت_Clear_Constraints = خطأ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        إذا (methodp->روش() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->روش() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->جمع_کن_Stmtsp(stmtp->clone_درخت(خطأ));
                        } وإلا إذا (methodp->روش() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            یافت_Clear_Constraints = درست;
                        }
                    });
                    إذا (یافت_Clear_Constraints) اكسر;
                }
            }
            randomizeFuncp->جمع_کن_Stmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->جمع_کن_Stmtsp(localGenp);

        AstFunc* ثابت basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ثابت basicRandomizeFuncCallp
            = جديد AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        إذا (classGenp) {
            AstTask* ثابت constr_راهاندازی_Funcp = get_Create_Constraint_راهاندازی_Func(classp, m_memberMap);
            AstTaskRef* ثابت callp = جديد AstTaskRef{nodep->fileline(), constr_راهاندازی_Funcp};
            randomizeFuncp->جمع_کن_Stmtsp(callp->makeStmt());
            randomizeFuncp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                جديد AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), گره_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ثابت rand_نما_Varp = get_Rand_نما_Var_From_Class(classp);
        إذا (!classGenp && rand_نما_Varp) جمع_کن_مجموعه_Rand_نما(randomizeFuncp, localGenp, rand_نما_Varp);
        إذا (!classGenp) {
            إذا (AstVar* ثابت sVarp = get_Static_Rand_نما_Var(classp)) {
                جمع_کن_مجموعه_Static_Rand_نما(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_گره* ثابت capturedTreep = withp->exprp()->unlink_Fr_پشت_With_بعدی();
        randomizeFuncp->جمع_کن_Stmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_گره* inlineResizeStmtsp = اشاره_تهی;
        {
            std::وکتور<AstCMethodHard*> اندازه_Methodps;
            capturedTreep->foreach_And_بعدی([&](AstCMethodHard* methodp) {
                إذا (methodp->روش() == VCMethod::DYN_اندازه
                    || methodp->روش() == VCMethod::ASSOC_اندازه) {
                    اندازه_Methodps.افزودن_پشت(methodp);
                }
            });
            لكل (AstCMethodHard* ثابت methodp : اندازه_Methodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = اشاره_تهی;
                Ast_گره_عبارت* ثابت ریشه_Exprp = methodp->fromp();
                إذا (AstMemberSel* memberSelp = VN_CAST(ریشه_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    طالما (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ثابت Ast_گره_عبارت* ثابت fromp = memberSelp->fromp();
                    إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(fromp, VarRef)) {
                        إذا (varRefp->varp() == captured.getThisp()) استمر;
                    } وإلا {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } وإلا إذا (AstVarRef* ثابت varRefp = VN_CAST(ریشه_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                إذا (!arrVarp) استمر;
                // Only handle rand-declared dynamic/assoc array variables
                إذا (!arrVarp->rand().isRandomizable()) استمر;
                // Do not randomize associative array size (IEEE 18.4)
                إذا (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) استمر;
                File_سطر* ثابت fl = methodp->fileline();
                منطقي wasCreated = خطأ;
                AstClass* ثابت varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ثابت اندازه_Varp = create_Or_Get_اندازه_Var(varClassp, arrVarp, fl,
                                                            methodp->پیدا_کن_Int_DType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                إذا (wasCreated) {
                    Ast_گره_عبارت* ثابت resizeVarRefp = buildMemberSelExprp(ریشه_Exprp, arrVarp);
                    Ast_گره_عبارت* ثابت اندازه_Exprp = buildMemberSelExprp(ریشه_Exprp, اندازه_Varp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->جمع_کن_بعدی(create_اندازه_Gte_Zero_Constraint(fl, اندازه_Varp, ریشه_Exprp));
                    AstCMethodHard* ثابت resizep
                        = جديد AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, اندازه_Exprp};
                    resizep->dtypep(methodp->پیدا_کن_Void_DType());
                    inlineResizeStmtsp
                        = Ast_گره::جمع_کن_بعدی(inlineResizeStmtsp, جديد Ast_Stmt_عبارت{fl, resizep});
                }
                Ast_گره_عبارت* ثابت اندازه_گره_Exprp = buildMemberSelExprp(ریشه_Exprp, اندازه_Varp);
                methodp->replaceWith(اندازه_گره_Exprp);
                VL_DO_DANGLING(methodp->delete_درخت(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_عبارت_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                rand_نما_Varp, m_writtenVars, &m_uniqueConstraintId, m_unique_نامها,  اشاره_تهی};
        }

        // Call the solver and set return value
        AstCExpr* ثابت solverCallp = جديد AstCExpr{nodep->fileline()};
        solverCallp->dtype_مجموعه_بیت();
        solverCallp->جمع_کن(جديد AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->جمع_کن(".next(__Vm_rng)");
        randomizeFuncp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            جديد AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        إذا (inlineResizeStmtsp) {
            AstVar* ثابت retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ثابت ifp = جديد AstIf{nodep->fileline(),
                                         جديد AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, اشاره_تهی};
            randomizeFuncp->جمع_کن_Stmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(classp)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(classp, "post");
            إذا (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->جمع_کن_Stmtsp((جديد AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        إذا (prePostWrap.دومی) {
            جمع_کن_Virtual_Pre_Post_Call(randomizeFuncp, classp, "post_randomize");
        } وإلا {
            جمع_کن_Pre_Post_Call(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->جمع_کن_Argsp(captured.get_آرگومانها());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_درخت(), withp);
    }
    فراغ بازدید(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        إذا (m_constraintp && nodep->fromp()->user1()
            && (nodep->روش() == VCMethod::ASSOC_اندازه
                || nodep->روش() == VCMethod::DYN_اندازه)) {
            AstClass* ثابت classp = VN_AS(m_modp, Class);
            AstVarRef* ثابت صف_Var_Refp = VN_CAST(nodep->fromp(), VarRef);
            إذا (!صف_Var_Refp) {
                // Warning from ConstraintExprVisitor will be thrown
                أرجع;
            }
            AstVar* ثابت صف_Varp = صف_Var_Refp->varp();
            منطقي wasCreated = خطأ;
            AstVar* ثابت اندازه_Varp
                = create_Or_Get_اندازه_Var(classp, صف_Varp, fl, nodep->پیدا_کن_Int_DType(), wasCreated);
            إذا (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                إذا (!VN_IS(صف_Varp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    إذا (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ثابت اندازه_Var_Refp = جديد AstVarRef{
                        fl, VN_AS(اندازه_Varp->user2p(), گره_Module), اندازه_Varp, VAccess::READ};
                    AstCMethodHard* ثابت resizep = جديد AstCMethodHard{
                        fl, nodep->fromp()->unlink_Fr_پشت(), VCMethod::DYN_RESIZE, اندازه_Var_Refp};
                    resizep->dtypep(nodep->پیدا_کن_Void_DType());
                    resizerTaskp->جمع_کن_Stmtsp(جديد Ast_Stmt_عبارت{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->جمع_کن_Itemsp(create_اندازه_Gte_Zero_Constraint(fl, اندازه_Varp));
            }
            AstVarRef* ثابت اندازه_Var_Refp = جديد AstVarRef{fl, VN_AS(اندازه_Varp->user2p(), گره_Module),
                                                         اندازه_Varp, VAccess::READ};
            اندازه_Var_Refp->user1(درست);
            nodep->replaceWith(اندازه_Var_Refp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح RandomizeVisitor(AstNetlist* nodep)
        : m_inline_Unique_نامها{"__Vrandwith"} {
        createRandomizeClassVars(nodep);
        // Flag local constraint leaves as solver-owned so __VBasicRand skips them.
        // Runs before any class is lowered. Only a randomized class counts, and
        // only a leaf owned by the constraint's own class: a leaf reached through
        // a global constraint stays basic-randomized so a standalone randomize()
        // of its type still randomizes it (issue #7833); a class-typed sub-object
        // is skipped so its own members are still basic-randomized.
        nodep->foreach([&](AstClass* ثابت classp) {
            إذا (!classp->user1()) أرجع;
            classp->foreachMember([&](AstClass* ثابت ownerClassp, AstConstraint* ثابت constrp) {
                constrp->foreach([&](AstMemberSel* ثابت memberSelp) {
                    AstVar* ثابت varp = memberSelp->varp();
                    إذا (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) أرجع;
                    إذا (VN_AS(varp->user2p(), گره_Module) != ownerClassp) أرجع;
                    إذا (!varp->user3()) varp->user3(درست);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(افزودن_Deletep(constrp->unlink_Fr_پشت()), constrp);
        });
    }
    ~RandomizeVisitor() override = افتراضي;
};

//######################################################################
// Randomize method class functions

فراغ V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ثابت RandomizeMarkVisitor markVisitor{nodep};
        ثابت RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_درخت("randomize", 0, dump_درخت_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ثابت std::string& name, منطقي allowVirtual,
                                       منطقي فرزند_DType) {
    AstFunc* funcp = VN_AS(memberMap.پیدا_کن_Member(nodep, name), Func);
    إذا (!funcp) {
        v3Global.useRandomizeMethods(درست);
        Ast_گره_DType* ثابت dtypep
            = فرزند_DType
                  ? جديد AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->پیدا_کن_بیت_DType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ثابت fvarp = فرزند_DType
                                  ? جديد AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlag_فرزند_DType{}, dtypep}
                                  : جديد AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3_Error_کد::NORETURN, درست);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(درست);
        fvarp->funcReturn(درست);
        fvarp->direction(VDirection::OUTPUT);
        nodep->جمع_کن_Membersp(funcp);
        funcp = جديد AstFunc{nodep->fileline(), name, اشاره_تهی, fvarp};
        إذا (!فرزند_DType) funcp->dtypep(dtypep);
        funcp->class_روش(درست);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->جمع_کن_Membersp(funcp);
        memberMap.درج(nodep, funcp);
    }
    أرجع funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_گره_Module* nodep, ثابت std::string& name) {
    AstFunc* funcp = اشاره_تهی;
    v3Global.useRandomizeMethods(درست);
    Ast_گره_DType* ثابت dtypep = nodep->پیدا_کن_بیت_DType(32, 32, VSigning::SIGNED);
    AstVar* ثابت fvarp = جديد AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(درست);
    fvarp->funcReturn(درست);
    fvarp->direction(VDirection::OUTPUT);
    funcp = جديد AstFunc{nodep->fileline(), name, اشاره_تهی, fvarp};
    funcp->dtypep(dtypep);
    إذا (VN_IS(nodep, Class)) {
        funcp->class_روش(درست);
    } وإلا {
        funcp->class_روش(درست);
        funcp->isStatic(درست);
    }
    nodep->جمع_کن_Stmtsp(funcp);
    أرجع funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ثابت basep = nodep->پایه_Most_Classp();
    AstFunc* funcp = VN_AS(memberMap.پیدا_کن_Member(basep, "srandom"), Func);
    إذا (!funcp) {
        v3Global.useRandomizeMethods(درست);
        Ast_گره_DType* ثابت dtypep
            = basep->پیدا_کن_بیت_DType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ثابت ivarp = جديد AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(درست);
        ivarp->direction(VDirection::درون_داد);
        funcp = جديد AstFunc{basep->fileline(), "srandom", ivarp, اشاره_تهی};
        funcp->dtypep(basep->پیدا_کن_Void_DType());
        funcp->class_روش(درست);
        funcp->isVirtual(خطأ);
        basep->جمع_کن_Membersp(funcp);
        memberMap.درج(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        إذا (basep->name() == "process") {
            funcp->جمع_کن_Stmtsp(جديد AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } وإلا {
            funcp->جمع_کن_Stmtsp(جديد AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(درست);
        }
    }
    أرجع funcp;
}
