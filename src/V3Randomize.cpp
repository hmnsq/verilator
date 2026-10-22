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
ساكن AstVar* get_Rand_ډېرپېښ_Var_From_Class(Ast_غوټه_Module* classp) {
    طالما (classp) {
        إذا (classp->user2p()) أرجع VN_AS(classp->user2p(), Var);
        AstClass* ثابت cp = VN_CAST(classp, Class);
        إذا (!cp || !cp->extendsp()) أرجع تش_اشاره;
        classp = cp->extendsp()->classp();
    }
    أرجع تش_اشاره;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ساكن AstVar* getRandomGenerator(AstClass* ثابت classp) {
    إذا (classp->user3p()) أرجع VN_AS(classp->user3p(), Var);
    إذا (classp->extendsp()) أرجع getRandomGenerator(classp->extendsp()->classp());
    أرجع تش_اشاره;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ساكن AstTask* get_Create_Constraint_تیاری_Func(AstClass* classp, VMemberMap& memberMap) {
    ساكن ثابت حرف* ثابت name = "__Vsetup_constraints";
    AstTask* تیاری_All_Taskp = VN_AS(memberMap.پیدا_کړه_Member(classp, name), Task);
    إذا (تیاری_All_Taskp) أرجع تیاری_All_Taskp;
    تیاری_All_Taskp = جديد AstTask{classp->fileline(), "__Vsetup_constraints", تش_اشاره};
    تیاری_All_Taskp->class_لاره(ریښتیا);
    تیاری_All_Taskp->isVirtual(ریښتیا);
    classp->جمع_کړه_Membersp(تیاری_All_Taskp);
    memberMap.ورننباسه(classp, تیاری_All_Taskp);
    أرجع تیاری_All_Taskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

بنية Rand_ډېرپېښ_موخه final {
    // MEMBERS
    AstVar* ثابت receiverp;  // Variable that is the target of the rand_mode() call
    Ast_غوټه_بیان* ثابت fromp;  // Expression from which the rand_mode() call originates
    AstClass* ثابت classp;  // Class in which rand_mode should be set

    // METHODS
    ساكن Rand_ډېرپېښ_موخه get(Ast_غوټه_بیان* fromp, Ast_غوټه_Module* modp) {
        إذا (!fromp) أرجع {تش_اشاره, تش_اشاره, VN_AS(modp, Class)};
        إذا (AstCMethodHard* ثابت لاره_Hardp = VN_CAST(fromp, CMethodHard)) {
            أرجع Rand_ډېرپېښ_موخه::get(لاره_Hardp->fromp(), modp);
        }
        AstVar* receiverp = تش_اشاره;
        AstClass* classp = VN_CAST(modp, Class);
        إذا (AstVarRef* ثابت varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            إذا (receiverp->isRand()) {
                fromp = تش_اشاره;
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

اتحاد Randomize_ډېرپېښ final {
    // MEMBERS
    بنية {
        منطقي کاروي_ډېرپېښ : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t شاخص : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ساكن Ast_غوټه_DType* arrayElementDTypep(Ast_غوټه_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    طالما (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    أرجع dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ساكن منطقي dtype_لري_Real(Ast_غوټه_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    إذا (Ast_غوټه_DType* ثابت subp = dtypep->subDTypep()) أرجع dtype_لري_Real(subp);
    إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, غوټه_UOr_Struct_DType)) {
        إذا (structp->packed()) أرجع خطأ;
        لكل (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            إذا (dtype_لري_Real(memberp->subDTypep())) أرجع ریښتیا;
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
ساكن فراغ checkRandTypeEligibility(Ast_غوټه* contextp, Ast_غوټه_DType* dtypep, منطقي isRandc) {
    dtypep = dtypep->skipRefp();
    إذا (Ast_غوټه_DType* ثابت subp = dtypep->subDTypep()) {
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
    إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, غوټه_UOr_Struct_DType)) {
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
    ثابت VNUser1_دننه_Use m_inuser1;
    ثابت VNUser2_دننه_Use m_inuser2;

    کارول Derived_سېټ = std::unordered_سېټ<AstClass*>;
    کارول بنسټ_منزل_Derived_Map = std::unordered_map<ثابت AstClass*, Derived_سېټ>;

    بنسټ_منزل_Derived_Map m_بنسټ_منزل_Derived_Map;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = تش_اشاره;  // Current class
    Ast_غوټه* m_constraint_بیان_Genp = تش_اشاره;  // Current constraint or constraint if expression
    Ast_غوټه_Module* m_modp;  // Current module
    Ast_غوټه_Stmt* m_stmtp = تش_اشاره;  // Current statement
    Ast_غوټه_FTask_Ref* m_stdRandCallp = تش_اشاره;  // Current std::randomize() call
    منطقي m_دننه_Std_With = خطأ;  // True when inside a 'with {}' clause
    std::سېټ<Ast_غوټه_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = تش_اشاره;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    فراغ markGlobalConstrainedVars(AstClass* classp) {
        لكل (ثابت AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : تش_اشاره) {
            لكل (Ast_غوټه* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                إذا (!varp) استمر;
                إذا (varp->rand().isRandomizable()) varp->globalConstrained(ریښتیا);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    منطقي is_Var_دننه_Std_Randomize_دلیلونه(ثابت AstVar* varp) ثابت {
        إذا (!m_دننه_Std_With || !m_stdRandCallp) أرجع خطأ;

        لكل (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ثابت Ast_غوټه_بیان* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            طالما (exprp) {
                إذا (ثابت Ast_غوټه_Var_Ref* ثابت varrefp = VN_CAST(exprp, غوټه_Var_Ref)) {
                    إذا (varrefp->varp() == varp) أرجع ریښتیا;
                    اكسر;
                }
                إذا (ثابت AstMemberSel* ثابت memberselp = VN_CAST(exprp, MemberSel)) {
                    إذا (memberselp->varp() == varp) أرجع ریښتیا;
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
             classp = classp->extendsp() ? classp->extendsp()->classp() : تش_اشاره) {
            لكل (Ast_غوټه* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                إذا (!varp) استمر;
                // If member is randomizable and of class type, mark its class
                إذا (varp->rand().isRandomizable()) {
                    ثابت Ast_غوټه_DType* ثابت varDtypep = varp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    إذا (!classRefp) {
                        ثابت Ast_غوټه_DType* subDTypep = varDtypep->subDTypep();
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
                        Randomize_ډېرپېښ rand_ډېرپېښ = {};
                        rand_ډېرپېښ.کاروي_ډېرپېښ = ریښتیا;
                        varp->user1(rand_ډېرپېښ.asUQuad);
                    }
                }
            }
        }
    }
    فراغ markDerived(ثابت AstClass* nodep) {
        ثابت خپلکاری it = m_بنسټ_منزل_Derived_Map.پیدا_کړه(nodep);
        إذا (it != m_بنسټ_منزل_Derived_Map.پای_ټکی()) {
            لكل (خپلکاری* classp : it->دوهم) {
                إذا (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    فراغ markAllDerived() {
        لكل (ثابت خپلکاری& p : m_بنسټ_منزل_Derived_Map) {
            إذا (p.لومړی->user1()) markDerived(p.لومړی);
        }
    }
    فراغ سېټ_Package_Refs() {
        لكل (Ast_غوټه_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), غوټه_Module));
        }
    }
    فراغ markNestedGlobalConstrainedRecurse(Ast_غوټه* nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            AstVar* ثابت varp = refp->varp();
            إذا (varp->globalConstrained()) أرجع;
            varp->globalConstrained(ریښتیا);
        } وإلا إذا (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(nodep, MemberSel)) {
            إذا (memberSelp->varp()) {
                AstVar* ثابت varp = memberSelp->varp();
                إذا (varp->globalConstrained()) أرجع;
                varp->globalConstrained(ریښتیا);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_غوټه_بیان* buildMemberSelChain(AstVarRef* ریښه_Var_Refp, ثابت std::ویکټور<AstVar*>& لار) {
        Ast_غوټه_بیان* exprp = ریښه_Var_Refp->clone_ونه(خطأ);
        لكل (AstVar* memberVarp : لار) {
            AstMemberSel* memberSelp
                = جديد AstMemberSel{ریښه_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        أرجع exprp;
    }

    // Process a single constraint during nested constraint cloning
    فراغ processNestedConstraint(AstConstraint* ثابت constrp, AstVarRef* ریښه_Var_Refp,
                                 ثابت std::ویکټور<AstVar*>& new_لار, AstClass* موخه_Classp) {
        std::string لار_مختاړی = ریښه_Var_Refp->name();
        لكل (AstVar* لار_Member_Varp : new_لار) {
            لار_مختاړی += GLOBAL_CONSTRAINT_SEPARATOR + لار_Member_Varp->name();
        }

        ثابت std::string newName = لار_مختاړی + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        منطقي isDuplicate = خطأ;
        موخه_Classp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت existingConstrp) {
            إذا (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = ریښتیا;
            }
        });
        إذا (isDuplicate) أرجع;

        AstConstraint* ثابت cloneConstrp = constrp->clone_ونه(خطأ);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::ویکټور<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_غوټه_بیان* exprp) {
            إذا (AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
                إذا (!varRefp->varp()->isClassMember()) أرجع;
                Ast_غوټه_بیان* ثابت chainp = buildMemberSelChain(ریښه_Var_Refp, new_لار);
                AstMemberSel* ثابت finalSelp
                    = جديد AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(ټېل_Deletep(varRefp), varRefp);
            } وإلا إذا (AstFuncRef* ثابت funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                إذا (funcRefp->taskp()->class_لاره() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.ټېل_شا(funcRefp);
                }
            }
        });
        لكل (AstFuncRef* funcRefp : funcRefps) {
            Ast_غوټه_بیان* ثابت chainp = buildMemberSelChain(ریښه_Var_Refp, new_لار);
            AstArg* ثابت argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlink_Fr_شا_With_بل() : تش_اشاره;
            Ast_لاره_Call* ثابت callp
                = جديد Ast_لاره_Call{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(ټېل_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        موخه_Classp->جمع_کړه_Stmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    فراغ cloneNestedConstraintsRecurse(AstVarRef* ریښه_Var_Refp, AstClass* classp,
                                       ثابت std::ویکټور<AstVar*>& لار_منزل_Class,
                                       AstClass* موخه_Classp) {
        لكل (Ast_غوټه* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ثابت memberVarp = VN_CAST(memberNodep, Var);
            إذا (!memberVarp) استمر;
            إذا (!memberVarp->rand().isRandomizable()) استمر;
            ثابت AstClassRefDType* ثابت memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            إذا (!memberClassRefp || !memberClassRefp->classp()) استمر;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::ویکټور<AstVar*> new_لار = لار_منزل_Class;
            new_لار.ټېل_شا(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ثابت containingClassp, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, ریښه_Var_Refp, new_لار, موخه_Classp);
                });
            cloneNestedConstraintsRecurse(ریښه_Var_Refp, nestedClassp, new_لار, موخه_Classp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    فراغ handle_Randomize_دلیل(Ast_غوټه_بیان* exprp, AstVar* ثابت varp, ثابت منطقي stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        طالما (exprp) {
            AstVar* randVarp = تش_اشاره;
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
                varrefp->user1(ریښتیا);
                varrefp->access(VAccess::READWRITE);
                exprp = تش_اشاره;
            } وإلا {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            إذا (randVarp == varp) أرجع;
            Ast_غوټه* backp = randVarp;
            طالما (backp && !VN_IS(backp, غوټه_Module)) backp = backp->backp();
            إذا (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } وإلا {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                Randomize_ډېرپېښ rand_ډېرپېښ = {};
                rand_ډېرپېښ.کاروي_ډېرپېښ = ریښتیا;
                randVarp->user1(rand_ډېرپېښ.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    فراغ لیدنه(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        إذا (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ثابت AstClass* ثابت basep = nodep->extendsp()->classp();
            m_بنسټ_منزل_Derived_Map[basep].ورننباسه(nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        إذا (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        إذا (nodep->name() == "rand_mode") {
            Ast_لاره_Call* ثابت لاره_Callp = VN_CAST(nodep, لاره_Call);
            Ast_غوټه_بیان* ثابت fromp = لاره_Callp ? لاره_Callp->fromp() : تش_اشاره;
            منطقي valid = ریښتیا;
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
            } وإلا إذا (AstCMethodHard* ثابت لاره_Hardp = VN_CAST(fromp, CMethodHard)) {
                إذا (لاره_Hardp->لاره() == VCMethod::ARRAY_AT
                    || لاره_Hardp->لاره() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = خطأ;
                } وإلا {
                    لاره_Hardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            إذا (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_بیان)
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_کوډ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = خطأ;
            }
            إذا (valid) {
                ثابت Rand_ډېرپېښ_موخه rand_ډېرپېښ_موخه = Rand_ډېرپېښ_موخه::get(fromp, m_classp);
                إذا ((!rand_ډېرپېښ_موخه.receiverp || !rand_ډېرپېښ_موخه.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = خطأ;
                } وإلا إذا (!rand_ډېرپېښ_موخه.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = خطأ;
                } وإلا إذا (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_بیان)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = خطأ;
                } وإلا إذا (rand_ډېرپېښ_موخه.receiverp && rand_ډېرپېښ_موخه.receiverp->isRand()) {
                    // Called on a rand member variable
                    Randomize_ډېرپېښ rand_ډېرپېښ = {};
                    rand_ډېرپېښ.کاروي_ډېرپېښ = ریښتیا;
                    rand_ډېرپېښ_موخه.receiverp->user1(rand_ډېرپېښ.asUQuad);
                } وإلا {
                    // Called on 'this' or a non-rand class instance
                    rand_ډېرپېښ_موخه.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        إذا (!varp->isRand()) أرجع;
                        Randomize_ډېرپېښ rand_ډېرپېښ = {};
                        rand_ډېرپېښ.کاروي_ډېرپېښ = ریښتیا;
                        varp->user1(rand_ډېرپېښ.asUQuad);
                    });
                }
            }
            إذا (!valid) {
                إذا (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_بیان)) {
                    nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                } وإلا {
                    m_stmtp->unlink_Fr_شا();
                }
            }
            أرجع;
        }

        إذا (nodep->name() == "constraint_mode") {
            منطقي valid = ریښتیا;
            إذا (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_بیان)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = خطأ;
            } وإلا إذا (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_بیان)
                       && !nodep->backp()->fileline()->warnIsOff(V3_Error_کوډ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = خطأ;
            }
            AstConstraint* constrp = تش_اشاره;
            AstClass* classp = m_classp;
            إذا (ثابت Ast_لاره_Call* ثابت لاره_Callp = VN_CAST(nodep, لاره_Call)) {
                إذا (ثابت AstConstraintRef* ثابت constrRefp
                    = VN_CAST(لاره_Callp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    إذا (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } وإلا إذا (AstClassRefDType* classRefDtp
                           = VN_CAST(لاره_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
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
                Randomize_ډېرپېښ constraint_ډېرپېښ = {};
                constraint_ډېرپېښ.کاروي_ډېرپېښ = ریښتیا;
                إذا (constrp) {
                    constrp->user1(constraint_ډېرپېښ.asUQuad);
                } وإلا {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraint_ډېرپېښ.asUQuad);
                    });
                }
            } وإلا {
                إذا (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_بیان)) {
                    nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                } وإلا {
                    m_stmtp->unlink_Fr_شا();
                }
            }
            أرجع;
        }

        إذا (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            إذا (m_constraint_بیان_Genp || m_دننه_Std_With) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                لكل (Ast_غوټه* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    إذا (argp->user1()) {
                        nodep->user1(ریښتیا);
                        اكسر;
                    }
                }
            }
            أرجع;
        }
        AstClass* classp = m_classp;
        إذا (ثابت Ast_لاره_Call* ثابت لاره_Callp = VN_CAST(nodep, لاره_Call)) {
            إذا (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(لاره_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        إذا (classp) {
            إذا (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت memberVarp) {
                إذا (!memberVarp->rand().isRandomizable()) أرجع;
                ثابت Ast_غوټه_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                إذا (!classRefp || !classRefp->classp()) أرجع;
                AstClass* ثابت memberClassp = classRefp->classp();
                إذا (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) أرجع;
                memberVarp->globalConstrained(ریښتیا);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* ریښه_Var_Refp
                    = جديد AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::ویکټور<AstVar*> تشه_لار;
                memberClassp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, ریښه_Var_Refp, تشه_لار, classp);
                });
                cloneNestedConstraintsRecurse(ریښه_Var_Refp, memberClassp, تشه_لار, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(ریښه_Var_Refp->delete_ونه(), ریښه_Var_Refp);
            });
        }
        إذا (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = تش_اشاره;
            لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handle_Randomize_دلیل(argp->exprp(), تش_اشاره, ریښتیا);
            }
            أرجع;
        }
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            إذا (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->عدد().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ثابت منطقي hasUnsupportedMember
                    = classp->existsMember([](ثابت AstClass*, ثابت AstVar* memberVarp) {
                          إذا (!memberVarp->rand().isRandomizable()) أرجع خطأ;
                          ثابت Ast_غوټه_DType* ثابت dtp = memberVarp->dtypep()->skipRefp();
                          أرجع VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, کتار_DType) || VN_IS(dtp, AssocArrayDType)
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
            AstVar* fromVarp = تش_اشاره;  // If nodep is a method call, this is its receiver
            إذا (Ast_لاره_Call* لاره_Callp = VN_CAST(nodep, لاره_Call)) {
                إذا (AstMemberSel* ثابت memberSelp = VN_CAST(لاره_Callp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } وإلا {
                    AstVarRef* ثابت varrefp = VN_AS(لاره_Callp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handle_Randomize_دلیل(argp->exprp(), fromVarp, خطأ);
        }
        // Re-mark members after the class became inline-randomized above.
        إذا (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    فراغ لیدنه(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_بیان_Genp);
        m_stmtp = nodep;
        m_constraint_بیان_Genp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_Constraint_بیان* nodep) override {
        VL_RESTORER(m_constraint_بیان_Genp);
        m_constraint_بیان_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_بیان_Genp);
            m_constraint_بیان_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterate_And_بل_Const_Null(nodep->thensp());
        iterate_And_بل_Const_Null(nodep->elsesp());
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        إذا (!m_constraint_بیان_Genp) أرجع;

        إذا (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        إذا (nodep->varp()->rand().isRandomizable() && !(m_دننه_Std_With && m_stdRandCallp)) {
            nodep->user1(ریښتیا);
        } وإلا إذا (is_Var_دننه_Std_Randomize_دلیلونه(nodep->varp())) {
            nodep->user1(ریښتیا);
        }
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        إذا (!m_constraint_بیان_Genp) أرجع;
        iterateChildrenConst(nodep);
        ثابت منطقي randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ثابت منطقي randMember = nodep->varp()->rand().isRandomizable();
        ثابت منطقي دننه_Std_With = m_دننه_Std_With && m_stdRandCallp;
        إذا (randObject && randMember && !دننه_Std_With) {
            nodep->user1(ریښتیا);
        } وإلا إذا (دننه_Std_With && is_Var_دننه_Std_Randomize_دلیلونه(nodep->varp())) {
            nodep->user1(ریښتیا);
            // Mark parent object for constraint expression visitor
            إذا (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(ریښتیا);
        }

        إذا (m_withp) {
            Ast_غوټه* backp = m_withp;
            طالما (backp->backp()) {
                إذا (ثابت Ast_لاره_Call* ثابت callp = VN_CAST(backp, لاره_Call)) {
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
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstVar* nodep) override {
        nodep->user2p(m_modp);
        إذا (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        إذا (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_دننه_Std_With = ریښتیا;
        iterateChildrenConst(nodep);
        m_دننه_Std_With = خطأ;
    }

    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {
        iterateChildrenConst(nodep);
        إذا (!m_constraint_بیان_Genp && !m_دننه_Std_With) أرجع;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    فراغ لیدنه(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        إذا (!m_constraint_بیان_Genp && !m_دننه_Std_With) أرجع;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    څرګند RandomizeMarkVisitor(Ast_غوټه* nodep) {
        iterateConst(nodep);
        markAllDerived();
        سېټ_Package_Refs();
    }
    ~RandomizeMarkVisitor() override = افتراضي;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

صنف Constraint_بیان_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ثابت m_classp;
    Ast_غوټه_FTask* ثابت m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_غوټه_FTask* ثابت m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ثابت m_genp;  // VlRandomizer variable of the class
    AstVar* m_rand_ډېرپېښ_Varp;  // Relevant randmode state variable
    منطقي m_wantSingle = خطأ;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    منطقي m_structSel = خطأ;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    منطقي m_defer_Rand_ډېرپېښ_Hoist = خطأ;  // Wait for the complete struct member path
    std::سېټ<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::سېټ<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertion_سېټ<AstVar*>* m_اندازه_Constrained_Arraysp = تش_اشاره;  // Arrays with size constraints
    Ast_غوټه_بیان* m_conditionp = تش_اشاره;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_غوټه_FTask* m_prepareConstrainedArraysp = تش_اشاره;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = تش_اشاره;  // Current ID of unique call
    V3_Unique_نومونه& m_unique_نومونه;  // Unique names of temporaries, and of blocks holding them
    std::ویکټور<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_غوټه* m_لومړی_افاده_Inside_Indexp = تش_اشاره;
    AstConstraintForeach* m_foreachp = تش_اشاره;  // Innermost constraint-foreach being processed
    std::سېټ<ثابت AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    صنف Nested_Access_لار final {
        AstMemberSel* m_سر_Nested_Array_Member_Selp
            = تش_اشاره;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = تش_اشاره;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = تش_اشاره;  // Last node of variable's name format
        Ast_غوټه** m_لومړی_افاده_Inside_شاخص_Pointerp
            = تش_اشاره;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    عام:
        فراغ جمع_کړه_Var_Name_Part(AstSFormatF* ثابت nodep, std::string&& name) {
            إذا (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } وإلا {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.ورننباسه(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            إذا (name == "") {
                إذا (*m_لومړی_افاده_Inside_شاخص_Pointerp) {
                    (*m_لومړی_افاده_Inside_شاخص_Pointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_لومړی_افاده_Inside_شاخص_Pointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_لومړی_افاده_Inside_شاخص_Pointerp = nodep;
            }
        }

        AstMemberSel* access_ونه() { أرجع m_سر_Nested_Array_Member_Selp; }
        ثابت std::string& smtName() { أرجع m_smtName; }

        Ast_Stmt_بیان* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ثابت methodp = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(genp->user2p(), غوټه_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtype_سېټ_Void();

            methodp->جمع_کړه_Pinsp(m_سر_Nested_Array_Member_Selp);  // Variable

            ثابت Ast_غوټه_DType* ثابت dtypep = varp->dtypep();  //پلنوالی
            ثابت اندازه_t پلنوالی = m_سر_Nested_Array_Member_Selp->varp()->dtypep()->پلنوالی();
            methodp->جمع_کړه_Pinsp(جديد AstConst{dtypep->fileline(), AstConst::Unsized64{}, پلنوالی});

            methodp->جمع_کړه_Pinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = تش_اشاره;

            methodp->جمع_کړه_Pinsp(
                جديد AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_سر_Nested_Array_Member_Selp = تش_اشاره;
            أرجع methodp->makeStmt();
        }

        Ast_غوټه_بیان* cloneName() { أرجع m_nestedNameFormatTopp->clone_ونه(خطأ); }

        فراغ error() {
            VL_DO_DANGLING(m_سر_Nested_Array_Member_Selp->delete_ونه(), m_سر_Nested_Array_Member_Selp);
        }

        Nested_Access_لار(AstMemberSel* سر_Member_Selp, Ast_غوټه** اشاره_ګر_منزل_لومړی_Exprp)
            : m_سر_Nested_Array_Member_Selp{سر_Member_Selp}
            , m_لومړی_افاده_Inside_شاخص_Pointerp{اشاره_ګر_منزل_لومړی_Exprp} {}
        ~Nested_Access_لار() {
            إذا (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_ونه(),
                            m_nestedNameFormatTopp = تش_اشاره);
                m_nestedNameFormatp = تش_اشاره;
            }
        }
    };
    Nested_Access_لار* m_nestedAccess = تش_اشاره;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_غوټه_بیان* createEnumConstraint(File_کرښه* ثابت fl, ثابت std::string& smtName,
                                      AstEnumDType* ثابت enumDtp, AstVar* ثابت genVarp) {
        Ast_غوټه_Module* ثابت genModp = VN_AS(genVarp->user2p(), غوټه_Module);
        ثابت صحيح پلنوالی = enumDtp->پلنوالی();
        std::string constraint = "(__Vbv (or";
        لكل (Ast_Enum_توکی* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_توکی)) {
            ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvt_منزل_Str(vconstp->منزل_UInt()) + " "
                          + cvt_منزل_Str(پلنوالی) + "))";
        }
        constraint += "))";
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + constraint + "\""}};
        callp->dtype_سېټ_Void();
        أرجع callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* پیدا_کړه_Static_Rand_ډېرپېښ_Var_Member(AstClass* classp) ثابت {
        طالما (ریښتیا) {
            إذا (AstVar* ثابت varp
                = VN_CAST(m_memberMap.پیدا_کړه_Member(classp, "__Vstaticrandmode"), Var)) {
                أرجع varp;
            }
            AstClassExtends* ثابت extendsp = classp->extendsp();
            إذا (!extendsp) أرجع تش_اشاره;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    فراغ markConstrainedRandMember(AstStructDType* ثابت structp, ثابت std::string& name) {
        AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.پیدا_کړه_Member(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(ریښتیا);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    منطقي is_Rand_Qualified_لار(ثابت Ast_غوټه_بیان* nodep) {
        طالما (nodep) {
            إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                إذا (AstStructDType* ثابت structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ثابت AstMemberDType* ثابت memberp
                        = VN_CAST(m_memberMap.پیدا_کړه_Member(structp, selp->name()), MemberDType);
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
        أرجع ریښتیا;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    ساكن فراغ markStructConstrainedRandRecurse(Ast_غوټه_DType* ثابت dtypep) {
        AstStructDType* ثابت structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        إذا (!structp || structp->packed()) أرجع;
        structp->markConstrainedRand(ریښتیا);
        لكل (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            إذا (!memberp->rand().isRandomizable()) استمر;
            memberp->markConstrainedRand(ریښتیا);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string build_Member_لار(ثابت AstMemberSel* ثابت memberSelp) {
        ثابت Ast_غوټه* fromp = memberSelp->fromp();
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            أرجع refp->name() + "." + memberSelp->name();
        } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            أرجع build_Member_لار(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        أرجع "";
    }

    ساكن Ast_غوټه_بیان* getFromp(ثابت Ast_غوټه_بیان* ثابت nodep) {
        إذا (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            أرجع cmethp->fromp();
        }
        أرجع getSelFromp(nodep);
    }

    ساكن std::string ایستل_Solve_Before_بنسټ_Name(ثابت Ast_غوټه_بیان* exprp) {
        طالما (ثابت Ast_غوټه_بیان* ثابت fromp = getFromp(exprp)) exprp = fromp;
        إذا (ثابت AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) أرجع vrefp->name();
        أرجع "";
    }

    ساكن فراغ build_Name_مختاړی(AstCExpr* exprp, ثابت Ast_غوټه_بیان* ثابت nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            exprp->جمع_کړه("(\""s + refp->name());
            أرجع;
        }

        ثابت Ast_غوټه_بیان* ثابت fromp = getFromp(nodep);
        إذا (fromp) build_Name_مختاړی(exprp, fromp);

        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            exprp->جمع_کړه("." + selp->name());
        } وإلا إذا (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            أرجع;
        } وإلا إذا (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            إذا (cmethp->لاره() == VCMethod::ARRAY_AT) {
                Ast_غوټه_بیان* ثابت argp = cmethp->pinsp();
                exprp->جمع_کړه(".\" + ");
                AstCFuncHard* ثابت solverHexFuncp
                    = جديد AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
                solverHexFuncp->جمع_کړه_Pinsp(argp->clone_ونه_خالص(خطأ));
                solverHexFuncp->dtype_سېټ_String();
                exprp->جمع_کړه(solverHexFuncp);
                exprp->جمع_کړه(" + \"");
            } وإلا {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                أرجع;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ساكن AstCExpr* build_لار_بیان(File_کرښه* ثابت fl, ثابت Ast_غوټه_بیان* nodep,
                                   ثابت std::string selName) {
        AstCExpr* ثابت exprp = جديد AstCExpr{fl, ""};
        build_Name_مختاړی(exprp, nodep);
        إذا (selName != "") {
            exprp->جمع_کړه(".");
            exprp->جمع_کړه(selName);
        }
        exprp->جمع_کړه("\")");
        أرجع exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_بیان(File_کرښه* fl, ثابت std::string& بنسټ_Name,
                                    ثابت Ast_غوټه_Sel* selp) {
        AstCExpr* ثابت p = جديد AstCExpr{fl, ""};
        p->جمع_کړه("(\""s + بنسټ_Name + ".\"");
        p->جمع_کړه("+");
        AstCFuncHard* ثابت solverHexFuncp
            = جديد AstCFuncHard{fl, VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
        solverHexFuncp->جمع_کړه_Pinsp(selp->bitp()->clone_ونه_خالص(خطأ));
        solverHexFuncp->dtype_سېټ_String();
        p->جمع_کړه(solverHexFuncp);
        p->جمع_کړه(")");
        p->dtype_سېټ_String();
        أرجع p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ساكن Ast_غوټه_بیان* getSelFromp(ثابت Ast_غوټه_بیان* exprp) {
        إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(exprp, MemberSel)) أرجع mp->fromp();
        إذا (ثابت AstStructSel* ثابت sp = VN_CAST(exprp, StructSel)) أرجع sp->fromp();
        أرجع تش_اشاره;
    }

    Ast_غوټه_بیان* build_Solve_Before_Name_بیان(File_کرښه* fl, Ast_غوټه_بیان* exprp) {
        إذا (ثابت AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ثابت p = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + varrefp->name() + "\"s"};
            p->dtype_سېټ_String();
            أرجع p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        إذا (Ast_غوټه_بیان* ثابت selFromp = getSelFromp(exprp)) {
            ثابت std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            إذا (ثابت AstArraySel* ثابت arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string بنسټ_Name;
                إذا (ثابت AstVarRef* ثابت vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    بنسټ_Name = vp->name();
                } وإلا إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    بنسټ_Name = build_Member_لار(mp);
                }
                إذا (بنسټ_Name.تشه()) أرجع تش_اشاره;
                AstCExpr* ثابت p = جديد AstCExpr{fl, ""};
                p->جمع_کړه("(\""s + بنسټ_Name + ".\"");
                p->جمع_کړه("+");
                AstCFuncHard* ثابت solverHexFuncp
                    = جديد AstCFuncHard{fl, VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
                solverHexFuncp->جمع_کړه_Pinsp(arrSelp->bitp()->clone_ونه_خالص(خطأ));
                solverHexFuncp->dtype_سېټ_String();
                p->جمع_کړه(solverHexFuncp);
                p->جمع_کړه(")");
                p->dtype_سېټ_String();
                أرجع p;
            }
            // Static member path (obj.field)
            إذا (ثابت AstVarRef* ثابت vp = VN_CAST(selFromp, VarRef)) {
                ثابت std::string لار = vp->name() + "." + selName;
                AstCExpr* ثابت p = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + لار + "\"s"};
                p->dtype_سېټ_String();
                أرجع p;
            }
            إذا (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ثابت p = build_لار_بیان(fl, selFromp, selName);
                p->dtype_سېټ_String();
                أرجع p;
            }
            أرجع تش_اشاره;
        }
        إذا (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ثابت p = build_لار_بیان(fl, exprp, "");
            p->dtype_سېټ_String();
            أرجع p;
        }
        ثابت Ast_غوټه_Sel* selp = VN_CAST(exprp, ArraySel);
        إذا (!selp) selp = VN_CAST(exprp, AssocSel);
        إذا (selp) {
            // arr[i] -> dynamic name
            std::string بنسټ_Name;
            إذا (ثابت AstVarRef* ثابت vp = VN_CAST(selp->fromp(), VarRef)) {
                بنسټ_Name = vp->name();
            } وإلا إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(selp->fromp(), MemberSel)) {
                بنسټ_Name = build_Member_لار(mp);
            }
            إذا (بنسټ_Name.تشه()) أرجع تش_اشاره;
            أرجع build_Array_Sel_Name_بیان(fl, بنسټ_Name, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        إذا (ثابت AstSel* ثابت بټ_Selp = VN_CAST(exprp, Sel)) {
            أرجع build_Solve_Before_Name_بیان(fl, بټ_Selp->fromp());
        }
        أرجع تش_اشاره;
    }

    AstSFormatF* getConstFormat(Ast_غوټه_بیان* nodep) {
        أرجع جديد AstSFormatF{nodep->fileline(), (nodep->پلنوالی() & 3) ? "#b%b" : "#x%x", خطأ,
                               nodep};
    }
    منطقي editFormat(Ast_غوټه_بیان* nodep) {
        إذا (nodep->user1()) أرجع خطأ;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlink_Fr_شا(&handle);
        handle.relink(getConstFormat(nodep));
        أرجع ریښتیا;
    }
    Ast_غوټه_بیان* wrapWithCond(Ast_غوټه_بیان* ثابت exprp, Ast_غوټه_بیان* ثابت condp,
                              ثابت صحيح پلنوالی) {
        إذا (condp) {
            File_کرښه* ثابت flp = exprp->fileline();
            أرجع جديد AstCond{
                flp, condp, exprp,
                getConstFormat(جديد AstConst{flp, AstConst::Widthed_ارزښت{}, پلنوالی, 0})};
        }
        أرجع exprp;
    }
    AstSFormatF* editSMT(Ast_غوټه_بیان* nodep, Ast_غوټه_بیان* lhsp = تش_اشاره,
                         Ast_غوټه_بیان* rhsp = تش_اشاره, Ast_غوټه_بیان* thsp = تش_اشاره) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_بیان = nodep->emitSMT();  // Might need child width (AstExtend)
        إذا (smt_بیان == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            أرجع تش_اشاره;
        }

        ثابت صحيح lhs_پلنوالی = lhsp ? lhsp->پلنوالی() : 0;
        ثابت صحيح rhs_پلنوالی = rhsp ? rhsp->پلنوالی() : 0;
        ثابت صحيح ths_پلنوالی = thsp ? thsp->پلنوالی() : 0;
        Ast_غوټه_بیان* lhsCondp = تش_اشاره;
        Ast_غوټه_بیان* rhsCondp = تش_اشاره;
        Ast_غوټه_بیان* thsCondp = تش_اشاره;

        إذا (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = تش_اشاره;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlink_Fr_شا() : lhsp),
                         غوټه_بیان);
            lhsCondp = m_conditionp;
        }
        إذا (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = تش_اشاره;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlink_Fr_شا() : rhsp),
                         غوټه_بیان);
            rhsCondp = m_conditionp;
        }
        إذا (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = تش_اشاره;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlink_Fr_شا() : thsp),
                         غوټه_بیان);
            thsCondp = m_conditionp;
        }

        Ast_غوټه_بیان* argsp = تش_اشاره;
        لكل (string::iterator pos = smt_بیان.begin(); pos != smt_بیان.پای_ټکی(); ++pos) {
            إذا (pos[0] == '%') {
                ++pos;
                بدل (pos[0]) {
                حالة '%': اكسر;
                حالة 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_غوټه::جمع_کړه_بل(argsp, wrapWithCond(lhsp, lhsCondp, lhs_پلنوالی));
                    lhsp = تش_اشاره;
                    اكسر;
                حالة 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_غوټه::جمع_کړه_بل(argsp, wrapWithCond(rhsp, rhsCondp, rhs_پلنوالی));
                    rhsp = تش_اشاره;
                    اكسر;
                حالة 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_غوټه::جمع_کړه_بل(argsp, wrapWithCond(thsp, thsCondp, ths_پلنوالی));
                    thsp = تش_اشاره;
                    اكسر;
                افتراضي: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); اكسر;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ثابت newp = جديد AstSFormatF{nodep->fileline(), smt_بیان, خطأ, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع newp;
    }

    Ast_غوټه_بیان* editSingle(File_کرښه* fl, Ast_غوټه* itemsp) {
        إذا (!itemsp) أرجع تش_اشاره;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = ریښتیا;

        {
            AstBegin* ثابت tempp
                = جديد AstBegin{fl, "[EditWrapper]", itemsp->unlink_Fr_شا_With_بل(), خطأ};
            VL_DO_DANGLING(iterate_And_بل_Null(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            إذا (itemsp) itemsp->unlink_Fr_شا_With_بل();
            VL_DO_DANGLING(tempp->delete_ونه(), tempp);
        }
        إذا (!itemsp) أرجع تش_اشاره;

        Ast_غوټه_بیان* ثابت exprsp = VN_CAST(itemsp, غوټه_بیان);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        إذا (!exprsp->nextp()) أرجع exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        لكل (Ast_غوټه* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        أرجع جديد AstSFormatF{fl, fmt.str(), خطأ, exprsp};
    }

    Ast_غوټه_بیان* newSel(File_کرښه* fl, Ast_غوټه_بیان* arrayp, Ast_غوټه_بیان* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_غوټه_DType* ثابت arrDtp = arrayp->unlink_Fr_شا()->dtypep()->skipRefp();
        Ast_غوټه_بیان* selp = تش_اشاره;
        إذا (VN_IS(arrDtp, کتار_DType) || VN_IS(arrDtp, DynArrayDType))
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
    Ast_غوټه_بیان* adjust_پلنوالی(File_کرښه* fl, Ast_غوټه_بیان* exprp, صحيح موخه_پلنوالی,
                             VSigning موخه_Signing) {
        ثابت صحيح بیان_پلنوالی = exprp->پلنوالی();
        إذا (موخه_پلنوالی > بیان_پلنوالی) {
            // Extend to match target width
            Ast_غوټه_بیان* ثابت پایله = جديد AstExtend{fl, exprp, موخه_پلنوالی};
            پایله->dtypeChgSigned(موخه_Signing);
            أرجع پایله;
        } وإلا إذا (موخه_پلنوالی < بیان_پلنوالی) {
            // Truncate to match target width
            Ast_غوټه_بیان* ثابت پایله = جديد AstSel{fl, exprp, 0, موخه_پلنوالی};
            پایله->dtypeChgSigned(موخه_Signing);
            أرجع پایله;
        } وإلا {
            // Width already matches
            أرجع exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_غوټه_بیان* ایستل_Return_بیان(AstFunc* funcp) {
        AstVar* ثابت retVarp = VN_CAST(funcp->fvarp(), Var);
        إذا (!retVarp) أرجع تش_اشاره;
        Ast_غوټه_بیان* retExprp = تش_اشاره;
        صحيح ټاکل_شمېر = 0;
        funcp->foreach([&](Ast_ټاکل* assignp) {
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            إذا (!lhsp || lhsp->varp() != retVarp) أرجع;
            retExprp = assignp->rhsp();
            ++ټاکل_شمېر;
        });
        // Simple function: initializer + return (2) or just return (1)
        إذا (ټاکل_شمېر > 2 || !retExprp) أرجع تش_اشاره;
        أرجع retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    فراغ propagateUser1InlineRecurse(Ast_غوټه_بیان* nodep) {
        إذا (VN_IS(nodep, غوټه_Var_Ref)) {
            nodep->user1(VN_AS(nodep, غوټه_Var_Ref)->varp()->rand().isRandomizable());
            أرجع;
        }
        إذا (VN_IS(nodep, Const)) {
            nodep->user1(خطأ);
            أرجع;
        }
        صحيح any_ماشوم = خطأ;  // Used as bool
        إذا (Ast_غوټه_بیان* ثابت cp = VN_CAST(nodep->op1p(), غوټه_بیان)) {
            propagateUser1InlineRecurse(cp);
            any_ماشوم |= cp->user1();
        }
        إذا (Ast_غوټه_بیان* ثابت cp = VN_CAST(nodep->op2p(), غوټه_بیان)) {
            propagateUser1InlineRecurse(cp);
            any_ماشوم |= cp->user1();
        }
        إذا (Ast_غوټه_بیان* ثابت cp = VN_CAST(nodep->op3p(), غوټه_بیان)) {
            propagateUser1InlineRecurse(cp);
            any_ماشوم |= cp->user1();
        }
        إذا (Ast_غوټه_بیان* ثابت cp = VN_CAST(nodep->op4p(), غوټه_بیان)) {
            propagateUser1InlineRecurse(cp);
            any_ماشوم |= cp->user1();
        }
        nodep->user1(any_ماشوم);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ساكن Ast_غوټه* prependDistPreamble(AstConstraintForeach* nodep, Ast_غوټه* bodyp) {
        Ast_غوټه* ثابت preamblep = nodep->user3p();
        إذا (!preamblep) أرجع bodyp;
        nodep->user3p(تش_اشاره);
        preamblep->جمع_کړه_بل(bodyp);
        أرجع preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    منطقي referencesForeachIdx(ثابت Ast_غوټه* nodep) ثابت {
        إذا (m_foreachIdxVars.تشه()) أرجع خطأ;
        أرجع nodep->exists(
            [&](ثابت AstVarRef* refp) { أرجع m_foreachIdxVars.شمېر(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    فراغ جمع_کړه_Registration_Stmt(Ast_غوټه* ثابت stmtp, Ast_غوټه_FTask* ثابت initTaskp) {
        إذا (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->جمع_کړه_Bodyp(stmtp);
        } وإلا {
            initTaskp->جمع_کړه_Stmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_کرښه* ثابت fl, Ast_غوټه_بیان* ثابت arrExprp,
                                       Ast_غوټه_بیان* ثابت idxExprp) {
        Ast_غوټه_بیان* ثابت argsp = Ast_غوټه::جمع_کړه_بل(arrExprp, idxExprp);
        AstSFormatF* ثابت formatp = جديد AstSFormatF{fl, "(select %s %s)", خطأ, argsp};
        أرجع formatp;
    }

    فراغ سېټ_Rand_ډېرپېښ(AstVar* ثابت varp, AstMemberSel* ثابت memberselp,
                     ثابت std::string& smtName, ثابت Randomize_ډېرپېښ& rand_ډېرپېښ,
                     Ast_غوټه_FTask* ثابت initTaskp) {
        Ast_غوټه_Module* ثابت varClassp = VN_AS(varp->user2p(), غوټه_Module);
        AstVar* ثابت sub_Rand_ډېرپېښ_Varp = get_Rand_ډېرپېښ_Var_From_Class(varClassp);
        إذا (sub_Rand_ډېرپېښ_Varp) {
            Ast_غوټه_بیان* ثابت پلار_Accessp = memberselp->fromp()->clone_ونه(خطأ);
            AstMemberSel* ثابت rand_ډېرپېښ_Selp
                = جديد AstMemberSel{varp->fileline(), پلار_Accessp, sub_Rand_ډېرپېښ_Varp};
            rand_ډېرپېښ_Selp->dtypep(sub_Rand_ډېرپېښ_Varp->dtypep());
            AstCMethodHard* ثابت atp
                = جديد AstCMethodHard{varp->fileline(), rand_ډېرپېښ_Selp, VCMethod::ARRAY_AT,
                                     جديد AstConst{varp->fileline(), rand_ډېرپېښ.شاخص}};
            atp->dtype_سېټ_UInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ثابت enablep = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), غوټه_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtype_سېټ_Void();
            Ast_غوټه_بیان* ثابت nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : جديد AstSFormatF{varp->fileline(), smtName, خطأ, تش_اشاره};
            nameExprp->dtypep(varp->dtypep());
            enablep->جمع_کړه_Pinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ثابت disablep = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), غوټه_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_سېټ_VAR_DISABLED};
            disablep->dtype_سېټ_Void();
            Ast_غوټه_بیان* ثابت disnp = nameExprp->clone_ونه(خطأ);
            disablep->جمع_کړه_Pinsp(disnp);
            AstIf* ثابت ifp
                = جديد AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            جمع_کړه_Registration_Stmt(ifp, initTaskp);
        }
    }

    فراغ markRandc(AstVar* ثابت varp, ثابت std::string& smtName, Ast_غوټه_FTask* ثابت initTaskp) {
        AstCMethodHard* ثابت markp = جديد AstCMethodHard{
            varp->fileline(),
            جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), غوټه_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtype_سېټ_Void();
        Ast_غوټه_بیان* ثابت nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : جديد AstSFormatF{varp->fileline(), smtName, خطأ, تش_اشاره};
        nameExprp->dtypep(varp->dtypep());
        markp->جمع_کړه_Pinsp(nameExprp);
        جمع_کړه_Registration_Stmt(markp->makeStmt(), initTaskp);
    }

    Ast_غوټه_Module* getLeftmostVarModulep(AstMemberSel* ثابت memberselp,
                                         AstVar* ثابت varp) ثابت {
        إذا (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_غوټه* کیڼ_Most_Nodep = memberselp->fromp();
            طالما (AstMemberSel* ثابت nestedMemberSelp = VN_CAST(کیڼ_Most_Nodep, MemberSel)) {
                کیڼ_Most_Nodep = nestedMemberSelp->fromp();
            }
            إذا (Ast_غوټه_Var_Ref* ثابت ریښه_Var_Refp = VN_CAST(کیڼ_Most_Nodep, غوټه_Var_Ref)) {
                أرجع VN_AS(ریښه_Var_Refp->varp()->user2p(), غوټه_Module);
            }
            أرجع VN_AS(memberselp->user2p(), غوټه_Module);
        }
        أرجع VN_AS(varp->user2p(), غوټه_Module);
    }

    Ast_غوټه_FTask* getInitTaskp(AstVar* ثابت varp, ثابت منطقي memberselOrStructSel,
                               Ast_غوټه_Module* ثابت classp) ثابت {
        إذا (m_inlineInitTaskp) أرجع m_inlineInitTaskp;
        varp->user3(ریښتیا);
        إذا (memberselOrStructSel) {
            إذا (Ast_غوټه_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.پیدا_کړه_Member(classp, "randomize"), غوټه_FTask)) {
                أرجع initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            إذا (m_memberselInitTaskp) أرجع m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } وإلا {
            Ast_غوټه_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.پیدا_کړه_Member(classp, "new"), غوټه_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            أرجع initTaskp;
        }
        أرجع تش_اشاره;
    }

    فراغ createWriteVarForClassFields(AstClass* ثابت elemClassp, ثابت std::string& smtName,
                                      AstVar* ثابت iterVarp, ثابت منطقي isUnpackedClassRefArray,
                                      Ast_غوټه_Module* ثابت classOrPackagep, AstVar* ثابت varp,
                                      AstIf* ثابت ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ثابت {
        File_کرښه* ثابت fl = varp->fileline();
        Ast_غوټه_Module* ثابت varClassp = VN_AS(varp->user2p(), غوټه_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr صحيح idx_پلنوالی = 32;
        ثابت صحيح fmt_پلنوالی = VL_ټکي_I(idx_پلنوالی) * 8;

        لكل (ثابت AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : تش_اشاره) {
            لكل (Ast_غوټه* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ثابت memberVarp = VN_CAST(mnodep, Var);
                إذا (!memberVarp || !memberVarp->rand().isRandomizable()) استمر;
                Ast_غوټه_DType* ثابت memberDtp = memberVarp->dtypep()->skipRefp();
                إذا (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) استمر;
                ثابت صحيح member_پلنوالی = memberDtp->پلنوالی();

                AstCStmt* ثابت fmtp = جديد AstCStmt{fl};
                إذا (isUnpackedClassRefArray) {
                    fmtp->جمع_کړه("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } وإلا {
                    fmtp->جمع_کړه("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::منزل_string(fmt_پلنوالی) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->جمع_کړه(جديد AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->جمع_کړه(");\n");
                ifNonNullp->جمع_کړه_Thensp(fmtp);

                AstVarRef* ثابت arrayWrRefp = جديد AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_غوټه_بیان* atWritep;
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
                                         جديد AstVarRef{fl, VN_AS(m_genp->user2p(), غوټه_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtype_سېټ_Void();
                writeVarp->جمع_کړه_Pinsp(memberSelp);
                writeVarp->جمع_کړه_Pinsp(
                    جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(member_پلنوالی)});
                AstCExpr* ثابت nameRefp = جديد AstCExpr{fl, AstCExpr::خالص{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->جمع_کړه_Pinsp(nameRefp);
                writeVarp->جمع_کړه_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->جمع_کړه_Thensp(writeVarp->makeStmt());
            }
        }
    }

    فراغ createSolverArrayHandle(AstVar* ثابت varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_غوټه_Module* ثابت classOrPackagep,
                                 ثابت منطقي isUnpackedClassRefArray,
                                 ثابت std::string& smtName) ثابت {
        File_کرښه* ثابت fl = varp->fileline();
        AstClass* ثابت elemClassp = elemClassRefDtp->classp();
        Ast_غوټه_Module* ثابت varClassp = VN_AS(varp->user2p(), غوټه_Module);

        AstVar* ثابت iterVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, m_unique_نومونه.get("__Vi"),
                                            varp->پیدا_کړه_UInt32_DType()};
        iterVarp->funcLocal(ریښتیا);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_غوټه* ثابت stmtsp
            = جديد Ast_ټاکل{fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE}, جديد AstConst{fl, 0}};

        Ast_غوټه_بیان* sizep;
        إذا (isUnpackedClassRefArray) {
            ثابت صحيح array_اندازه_2
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = جديد AstConst{fl, static_cast<uint32_t>(array_اندازه_2)};
        } وإلا {
            AstVarRef* ثابت array_اندازه_Refp = جديد AstVarRef{fl, varClassp, varp, VAccess::READ};
            array_اندازه_Refp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ثابت dynSizep
                = جديد AstCMethodHard{fl, array_اندازه_Refp, VCMethod::DYN_اندازه, تش_اشاره};
            dynSizep->dtype_سېټ_UInt32();
            sizep = dynSizep;
        }

        AstLoop* ثابت loopp = جديد AstLoop{fl};
        stmtsp->جمع_کړه_بل(loopp);
        loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{
            fl, loopp, جديد AstLt{fl, جديد AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ثابت arrayReadRefp = جديد AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_غوټه_بیان* atReadp;
        إذا (isUnpackedClassRefArray) {
            atReadp
                = جديد AstArraySel{fl, arrayReadRefp, جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        } وإلا {
            atReadp = جديد AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ثابت ifNonNullp
            = جديد AstIf{fl, جديد AstNeq{fl, atReadp, جديد AstConst{fl, AstConst::Null{}}}, تش_اشاره};
        loopp->جمع_کړه_Stmtsp(ifNonNullp);

        AstCStmt* ثابت bufDeclp = جديد AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->جمع_کړه_Thensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{
            fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
            جديد Ast_جمع_کړه{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(ریښتیا);
        Ast_غوټه_FTask* initTaskp = m_inlineInitTaskp;
        إذا (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.پیدا_کړه_Member(varClassp, "new"), غوټه_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->جمع_کړه_Stmtsp(stmtsp);
    }

    فراغ createSolverVarHandle(AstVar* ثابت varp, ثابت منطقي structSelOrCMeth,
                               ثابت منطقي isGlobalConstrained, ثابت Randomize_ډېرپېښ rand_ډېرپېښ,
                               AstMemberSel* ثابت memberselp, ثابت std::string& smtName,
                               Ast_غوټه_Module* ثابت classOrPackagep, Ast_غوټه_Module* ثابت classp,
                               Ast_غوټه_FTask* ثابت initTaskp) {
        uint32_t unpackedDims = 0;
        إذا (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(خطأ).دوهم;
        }
        إذا (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ثابت methodp = جديد AstCMethodHard{
            varp->fileline(),
            جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), غوټه_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_سېټ_Void();
        إذا (memberselp) {
            methodp->جمع_کړه_Pinsp(memberselp);
        } وإلا {
            AstVarRef* ثابت varRefp
                = جديد AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->جمع_کړه_Pinsp(varRefp);
        }
        ثابت اندازه_t پلنوالی = arrayElementDTypep(varp->dtypep())->پلنوالی();
        methodp->جمع_کړه_Pinsp(جديد AstConst{varp->fileline(), AstConst::Unsized64{}, پلنوالی});
        Ast_غوټه_بیان* ثابت varnamep = جديد AstCExpr{varp->fileline(), AstCExpr::خالص{},
                                                   "\"" + smtName + "\"", varp->پلنوالی()};
        varnamep->dtypep(varp->dtypep());
        methodp->جمع_کړه_Pinsp(varnamep);
        methodp->جمع_کړه_Pinsp(
            جديد AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        إذا (rand_ډېرپېښ.کاروي_ډېرپېښ && !(isGlobalConstrained && memberselp)) {
            methodp->جمع_کړه_Pinsp(
                جديد AstConst{varp->fileline(), AstConst::Unsized64{}, rand_ډېرپېښ.شاخص});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->جمع_کړه_Stmtsp(methodp->makeStmt());
        إذا (varp->lifetime().isStatic() && rand_ډېرپېښ.کاروي_ډېرپېښ) {
            AstCMethodHard* ثابت markp = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), غوټه_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_غوټه_بیان* ثابت namep = جديد AstCExpr{varp->fileline(), AstCExpr::خالص{},
                                                    "\"" + smtName + "\"", varp->پلنوالی()};
            namep->dtypep(varp->dtypep());
            markp->جمع_کړه_Pinsp(namep);
            markp->dtype_سېټ_Void();
            initTaskp->جمع_کړه_Stmtsp(markp->makeStmt());
        }
        إذا (isGlobalConstrained && memberselp && rand_ډېرپېښ.کاروي_ډېرپېښ) {
            سېټ_Rand_ډېرپېښ(varp, memberselp, smtName, rand_ډېرپېښ, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        إذا (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_غوټه_بیان* createVarExprp(Ast_غوټه_Var_Ref* nodep, ثابت Randomize_ډېرپېښ rand_ډېرپېښ,
                                ثابت std::string& smtName, ثابت منطقي isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ثابت varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_غوټه_بیان* exprp = جديد AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, خطأ, تش_اشاره};
        إذا (!rand_ډېرپېښ.کاروي_ډېرپېښ) {
            // Global constraints keep nodep alive for write_var processing
            إذا (!isGlobalConstrained) VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_غوټه_بیان* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_ونه(خطأ)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_غوټه_بیان* rand_ډېرپېښ_Access;
        ثابت منطقي varIsStatic = varp->lifetime().isStatic();
        AstClass* ثابت varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : تش_اشاره;
        AstVar* ثابت owner_Static_Rand_ډېرپېښ_Varp
            = varOwningClassp ? پیدا_کړه_Static_Rand_ډېرپېښ_Var_Member(varOwningClassp) : تش_اشاره;
        إذا (owner_Static_Rand_ډېرپېښ_Varp) {
            rand_ډېرپېښ_Access = جديد AstVarRef{varp->fileline(),
                                           VN_AS(owner_Static_Rand_ډېرپېښ_Varp->user2p(), غوټه_Module),
                                           owner_Static_Rand_ډېرپېښ_Varp, VAccess::READ};
        } وإلا إذا (memberselp) {
            Ast_غوټه_Module* ثابت varClassp = VN_AS(varp->user2p(), غوټه_Module);
            AstVar* ثابت effective_Rand_ډېرپېښ_Varp = get_Rand_ډېرپېښ_Var_From_Class(varClassp);
            إذا (effective_Rand_ډېرپېښ_Varp) {
                // Member's class has randmode, use it
                Ast_غوټه_بیان* پلار_Access = memberselp->fromp()->clone_ونه(خطأ);
                AstMemberSel* rand_ډېرپېښ_Sel
                    = جديد AstMemberSel{varp->fileline(), پلار_Access, effective_Rand_ډېرپېښ_Varp};
                rand_ډېرپېښ_Sel->dtypep(effective_Rand_ډېرپېښ_Varp->dtypep());
                rand_ډېرپېښ_Access = rand_ډېرپېښ_Sel;
            } وإلا {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_rand_ډېرپېښ_Varp, nodep, "No m_randModeVarp");
                rand_ډېرپېښ_Access
                    = جديد AstVarRef{varp->fileline(), VN_AS(m_rand_ډېرپېښ_Varp->user2p(), غوټه_Module),
                                    m_rand_ډېرپېښ_Varp, VAccess::READ};
            }
        } وإلا {
            UASSERT_OBJ(m_rand_ډېرپېښ_Varp, nodep, "No m_randModeVarp");
            rand_ډېرپېښ_Access
                = جديد AstVarRef{varp->fileline(), VN_AS(m_rand_ډېرپېښ_Varp->user2p(), غوټه_Module),
                                m_rand_ډېرپېښ_Varp, VAccess::READ};
        }

        AstCMethodHard* ثابت atp
            = جديد AstCMethodHard{nodep->fileline(), rand_ډېرپېښ_Access, VCMethod::ARRAY_AT,
                                 جديد AstConst{nodep->fileline(), rand_ډېرپېښ.شاخص}};
        atp->dtype_سېټ_UInt32();
        exprp = جديد AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(ریښتیا);  // Mark as formatted
        أرجع exprp;
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        إذا (varp->user4p()) {
            منطقي is_اندازه_Ref = خطأ;
            إذا (AstCMethodHard* ثابت methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                إذا (methodp->لاره() == VCMethod::ASSOC_اندازه
                    || methodp->لاره() == VCMethod::DYN_اندازه) {
                    is_اندازه_Ref = ریښتیا;
                }
            }
            إذا (!is_اندازه_Ref && m_اندازه_Constrained_Arraysp) { m_اندازه_Constrained_Arraysp->ورننباسه(varp); }
        }

        // Check if this variable is marked as globally constrained
        ثابت منطقي isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = تش_اشاره;
        منطقي structSelOrCMeth = خطأ;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_غوټه_DType* selectedDtypep = تش_اشاره;
        std::string smtName;
        إذا (m_nestedAccess) {
            m_nestedAccess->جمع_کړه_Var_Name_Part(
                جديد AstSFormatF{varp->fileline(), nodep->name(), خطأ, تش_اشاره}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_ونه();
        } وإلا إذا (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_غوټه* سر_Member_Sel = nodep->backp();
            طالما (VN_IS(سر_Member_Sel->backp(), MemberSel)) {
                سر_Member_Sel = سر_Member_Sel->backp();
            }
            memberselp = VN_AS(سر_Member_Sel, MemberSel)->clone_ونه(خطأ);
            smtName = build_Member_لار(memberselp);
        } وإلا إذا (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_غوټه* سر_Nodep = nodep->backp();
            طالما (VN_IS(سر_Nodep->backp(), StructSel) || VN_IS(سر_Nodep->backp(), CMethodHard)) {
                سر_Nodep = سر_Nodep->backp();
            }

            إذا (VN_IS(سر_Nodep, StructSel) || VN_IS(سر_Nodep, CMethodHard)) {
                structSelOrCMeth = ریښتیا;
                selectedDtypep = VN_AS(سر_Nodep, غوټه_بیان)->dtypep();
            }

            memberselp = VN_CAST(سر_Nodep, MemberSel);
            إذا (memberselp) smtName = build_Member_لار(memberselp);

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
        إذا (dtype_لري_Real(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            أرجع;
        }
        Ast_غوټه_Module* ثابت classOrPackagep = nodep->classOrPackagep();
        ثابت Randomize_ډېرپېښ rand_ډېرپېښ = {.asUQuad = varp->user1()};
        إذا (!rand_ډېرپېښ.کاروي_ډېرپېښ && editFormat(nodep)) أرجع;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (enumDtp
            && (std::پیدا_کړه(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.پای_ټکی(), varp)
                == m_rangeConstrainedEnums.پای_ټکی())) {
            AstVar* genVarp = m_genp;
            إذا (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_غوټه_بیان* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_غوټه_FTask* موخه_Taskp = m_inlineInitTaskp;
            إذا (!موخه_Taskp) موخه_Taskp = get_Create_Constraint_تیاری_Func(m_classp, m_memberMap);
            UASSERT_OBJ(موخه_Taskp, nodep, "No function to inline enum range constraint into");
            موخه_Taskp->جمع_کړه_Stmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.ټېل_شا(varp);
        }

        VNRelinker relinker;
        nodep->unlink_Fr_شا(&relinker);
        Ast_غوټه_بیان* ثابت exprp
            = createVarExprp(nodep, rand_ډېرپېښ, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ثابت منطقي alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.شمېر(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.شمېر(smtName) > 0
                                    : memberselp          ? m_writtenVars.شمېر(smtName) > 0
                                                          : varp->user3();
        ثابت منطقي shouldWriteVar = !alreadyWritten;
        إذا (shouldWriteVar) {
            // Track this variable path as written
            إذا (m_inlineInitTaskp)
                m_inlineWrittenVars.ورننباسه(smtName);
            وإلا إذا (isGlobalConstrained || memberselp)
                m_writtenVars.ورننباسه(smtName);
            // For global constraints, delete nodep after processing
            إذا (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);

            منطقي isClassRefArray = خطأ;
            منطقي isUnpackedClassRefArray = خطأ;
            AstClassRefDType* elemClassRefDtp = تش_اشاره;
            {
                Ast_غوټه_DType* varDtp = varp->dtypep()->skipRefp();
                إذا (varDtp->isNonPackedArray()) {
                    Ast_غوټه_DType* ثابت elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    إذا (elemClassRefDtp) {
                        isClassRefArray = ریښتیا;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_غوټه_Module* ثابت classp = getLeftmostVarModulep(memberselp, varp);
            Ast_غوټه_FTask* ثابت initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            إذا (m_nestedAccess) {
                Ast_Stmt_بیان* ثابت writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                جمع_کړه_Registration_Stmt(writeVarStmtp, initTaskp);
                إذا (isGlobalConstrained && memberselp && rand_ډېرپېښ.کاروي_ډېرپېښ) {
                    سېټ_Rand_ډېرپېښ(varp, memberselp, smtName, rand_ډېرپېښ, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                إذا (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } وإلا إذا (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } وإلا {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, rand_ډېرپېښ,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } وإلا {
            // Variable already written, clean up cloned memberselp if any
            إذا (memberselp) VL_DO_DANGLING(memberselp->delete_ونه(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            إذا (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
        إذا (m_nestedAccess) { VL_DO_DANGLING(ړنګول m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_غوټه_بیان* build_شمېر_Ones_Expansion(File_کرښه* fl, Ast_غوټه_بیان* argp) {
        ثابت V3_شمېره عدد_One{fl, argp->پلنوالی(), 1};
        Ast_غوټه_بیان* sump = جديد AstAnd{fl, argp, جديد AstConst{fl, عدد_One}};
        sump->user1(ریښتیا);
        لكل (صحيح i = 1; i < argp->پلنوالی(); i++) {
            V3_شمېره عدد_بټ_ماسک{fl, argp->پلنوالی(), 0};
            عدد_بټ_ماسک.سېټ_بټ(i, 1);
            AstAnd* ثابت andp
                = جديد AstAnd{fl, argp->clone_ونه_خالص(خطأ), جديد AstConst{fl, عدد_بټ_ماسک}};
            andp->user1(ریښتیا);
            Ast_ښويدنه_R* ثابت shiftp
                = جديد Ast_ښويدنه_R{fl, andp,
                                جديد AstConst{fl, AstConst::Widthed_ارزښت{}, argp->پلنوالی(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(ریښتیا);
            sump = جديد Ast_جمع_کړه{fl, sump, shiftp};
            sump->user1(ریښتیا);
        }
        أرجع sump;
    }

    فراغ لیدنه(Ast_شمېر_Ones* nodep) override {
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت argp = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* sump = build_شمېر_Ones_Expansion(fl, argp);
        sump = adjust_پلنوالی(fl, sump, nodep->پلنوالی(), nodep->dtypep()->numeric());
        sump->user1(ریښتیا);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(sump);
    }
    فراغ لیدنه(AstRedXor* nodep) override {
        إذا (editFormat(nodep)) أرجع;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت argp = nodep->lhsp()->unlink_Fr_شا();

        Ast_غوټه_بیان* redxorp = جديد AstSel{fl, argp, 0, 1};
        redxorp->user1(ریښتیا);
        لكل (صحيح i = 1; i < argp->پلنوالی(); i++) {
            AstSel* ثابت selp = جديد AstSel{fl, argp->clone_ونه_خالص(خطأ), i, 1};
            selp->user1(ریښتیا);

            redxorp = جديد AstXor{fl, redxorp, selp};
            redxorp->user1(ریښتیا);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(redxorp);
    }
    فراغ لیدنه(AstRedAnd* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // Convert to (~x == 0)
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت argp = nodep->lhsp()->unlink_Fr_شا();
        ثابت V3_شمېره عدد_Zero{fl, argp->پلنوالی(), 0};
        Ast_غوټه_بیان* ثابت negp = جديد AstNot{fl, argp};
        negp->user1(ریښتیا);
        Ast_غوټه_بیان* ثابت eqp = جديد AstEq{fl, negp, جديد AstConst{fl, عدد_Zero}};
        eqp->user1(ریښتیا);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(eqp);
    }
    فراغ لیدنه(AstRedOr* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // Convert to (x != 0)
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت argp = nodep->lhsp()->unlink_Fr_شا();
        ثابت V3_شمېره عدد_Zero{fl, argp->پلنوالی(), 0};
        Ast_غوټه_بیان* ثابت neqp = جديد AstNeq{fl, argp, جديد AstConst{fl, عدد_Zero}};
        neqp->user1(ریښتیا);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(neqp);
    }
    فراغ لیدنه(AstOneHot* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت argp = nodep->lhsp()->unlink_Fr_شا();
        ثابت صحيح w = argp->پلنوالی();

        ثابت V3_شمېره عدد_Zero{fl, w, 0};
        AstNeq* ثابت neZerop
            = جديد AstNeq{fl, argp->clone_ونه_خالص(خطأ), جديد AstConst{fl, عدد_Zero}};
        neZerop->user1(ریښتیا);

        ثابت V3_شمېره عدد_One{fl, w, 1};
        AstSub* ثابت subp = جديد AstSub{fl, argp->clone_ونه_خالص(خطأ), جديد AstConst{fl, عدد_One}};
        subp->user1(ریښتیا);

        AstAnd* ثابت andp = جديد AstAnd{fl, argp, subp};
        andp->user1(ریښتیا);

        ثابت V3_شمېره عدد_Zero2{fl, w, 0};
        AstEq* ثابت eqZerop = جديد AstEq{fl, andp, جديد AstConst{fl, عدد_Zero2}};
        eqZerop->user1(ریښتیا);

        AstLogAnd* ثابت resultp = جديد AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(ریښتیا);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(resultp);
    }
    فراغ لیدنه(AstOneHot0* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $onehot0(x) = (x & (x-1)) == 0
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت argp = nodep->lhsp()->unlink_Fr_شا();
        ثابت صحيح w = argp->پلنوالی();

        ثابت V3_شمېره عدد_One{fl, w, 1};
        AstSub* ثابت subp = جديد AstSub{fl, argp->clone_ونه_خالص(خطأ), جديد AstConst{fl, عدد_One}};
        subp->dtypeFrom(argp);
        subp->user1(ریښتیا);

        AstAnd* ثابت andp = جديد AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(ریښتیا);

        ثابت V3_شمېره عدد_Zero{fl, w, 0};
        AstEq* ثابت eqp = جديد AstEq{fl, andp, جديد AstConst{fl, عدد_Zero}};
        eqp->user1(ریښتیا);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(eqp);
    }
    فراغ لیدنه(Ast_شمېر_بټونه* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_کرښه* ثابت fl = nodep->fileline();

        منطقي شمېر_Ones = خطأ;
        منطقي شمېر_Zeros = خطأ;
        لكل (Ast_غوټه_بیان* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ثابت AstConst* ثابت cp = VN_CAST(ctrlp, Const);
            إذا (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ثابت zerop
                    = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, nodep->پلنوالی(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                أرجع;
            }
            إذا (cp->عدد().بټ_Is1(0) && !شمېر_Ones)
                شمېر_Ones = ریښتیا;
            وإلا إذا (cp->عدد().بټ_Is0(0) && !شمېر_Zeros)
                شمېر_Zeros = ریښتیا;
        }

        Ast_غوټه_بیان* ثابت argp = nodep->lhsp()->unlink_Fr_شا();
        ثابت صحيح arg_پلنوالی = argp->پلنوالی();
        Ast_غوټه_بیان* sump = تش_اشاره;
        إذا (شمېر_Ones && شمېر_Zeros) {
            // ones + zeros = width for 2-state types
            sump = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, nodep->پلنوالی(), (uint32_t)arg_پلنوالی};
            VL_DO_DANGLING(argp->delete_ونه(), argp);
        } وإلا إذا (شمېر_Ones) {
            sump = build_شمېر_Ones_Expansion(fl, argp);
        } وإلا إذا (شمېر_Zeros) {
            // width - countones(x)
            Ast_غوټه_بیان* ثابت onesCountp = build_شمېر_Ones_Expansion(fl, argp);
            ثابت V3_شمېره پلنوالی_Val{nodep, onesCountp->پلنوالی(), (uint32_t)arg_پلنوالی};
            sump = جديد AstSub{fl, جديد AstConst{fl, پلنوالی_Val}, onesCountp};
            sump->user1(ریښتیا);
        } وإلا {
            sump = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, nodep->پلنوالی(), 0U};
            VL_DO_DANGLING(argp->delete_ونه(), argp);
        }

        sump = adjust_پلنوالی(fl, sump, nodep->پلنوالی(), nodep->dtypep()->numeric());
        sump->user1(ریښتیا);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(sump);
    }
    فراغ لیدنه(AstCLog2* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت argp = nodep->lhsp()->unlink_Fr_شا();
        ثابت صحيح arg_پلنوالی = argp->پلنوالی();
        ثابت صحيح پایله_پلنوالی = nodep->پلنوالی();

        Ast_غوټه_بیان* resultp
            = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, پایله_پلنوالی, (uint32_t)arg_پلنوالی};

        لكل (صحيح k = arg_پلنوالی - 1; k >= 0; k--) {
            V3_شمېره پوله{fl, arg_پلنوالی, 0};
            إذا (k < 32)
                پوله.سېټ_Long(1ULL << k);
            وإلا
                پوله.سېټ_بټ(k, 1);

            AstLte* ثابت ltep
                = جديد AstLte{fl, argp->clone_ونه_خالص(خطأ), جديد AstConst{fl, پوله}};
            ltep->user1(ریښتیا);

            AstConst* ثابت valuep
                = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, پایله_پلنوالی, (uint32_t)k};

            resultp = جديد AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(ریښتیا);
        }

        VL_DO_DANGLING(argp->delete_ونه(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        iterate(resultp);
    }
    فراغ لیدنه(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ثابت zerop = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    فراغ handlePow(Ast_غوټه_Biop* nodep) {
        إذا (AstConst* ثابت exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_بیان* ثابت basep = nodep->lhsp();
            ثابت V3_شمېره عدد_One{nodep, basep->پلنوالی(), 1};
            Ast_غوټه_بیان* powerp = جديد AstConst{fl, عدد_One};
            ثابت منطقي بنسټ_Signed = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ثابت int32_t exponent = بنسټ_Signed ? exponentp->منزل_SInt() : exponentp->منزل_UInt();
            إذا (exponent > 0) {
                لكل (int32_t i = 0; i < exponent; i++) {
                    إذا (بنسټ_Signed) {
                        powerp = جديد AstMulS{fl, powerp, basep->clone_ونه_خالص(خطأ)};
                    } وإلا {
                        powerp = جديد AstMul{fl, powerp, basep->clone_ونه_خالص(خطأ)};
                    }
                    powerp->user1(ریښتیا);
                }
            } وإلا إذا (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                إذا (بنسټ_Signed) {
                    powerp = جديد AstDivS{fl, powerp, basep->clone_ونه_خالص(خطأ)};
                    powerp->user1(ریښتیا);
                    powerp = جديد AstDivS{fl, powerp, basep->clone_ونه_خالص(خطأ)};
                    powerp->user1(ریښتیا);
                } وإلا {
                    powerp = جديد AstDiv{fl, powerp, basep->clone_ونه_خالص(خطأ)};
                    powerp->user1(ریښتیا);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            iterate(powerp);
        } وإلا إذا (ثابت AstConst* ثابت basep = VN_CAST(nodep->lhsp(), Const)) {
            إذا (basep->عدد().منزل_UInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_کرښه* ثابت fl = nodep->fileline();
                Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
                ثابت صحيح پلنوالی = nodep->پلنوالی();
                ثابت منطقي rhsDependent = rhsp->user1();
                AstConst* ثابت onep = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, پلنوالی, 1};
                Ast_ښويدنه_L* ثابت shiftp = جديد Ast_ښويدنه_L{fl, onep, rhsp, پلنوالی};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_غوټه_بیان* resultp = shiftp;
                إذا (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_غوټه_بیان* ثابت nClonep = rhsp->clone_ونه_خالص(خطأ);
                    nClonep->user1(rhsDependent);
                    AstConst* ثابت zeroNp
                        = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, rhsp->پلنوالی(), 0};
                    AstGteS* ثابت condp = جديد AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ثابت zeroRp = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, پلنوالی, 0};
                    resultp = جديد AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
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
    فراغ لیدنه(AstPow* nodep) override { handlePow(nodep); }
    فراغ لیدنه(AstPowSS* nodep) override { handlePow(nodep); }
    فراغ لیدنه(AstPowSU* nodep) override { handlePow(nodep); }
    فراغ لیدنه(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    فراغ handle_ښويدنه(Ast_غوټه_Biop* nodep) {
        إذا (editFormat(nodep)) أرجع;
        ثابت صحيح lhs_پلنوالی = nodep->lhsp()->پلنوالی();
        ثابت صحيح rhs_پلنوالی = nodep->rhsp()->پلنوالی();
        إذا (rhs_پلنوالی < lhs_پلنوالی) {
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
            AstExtend* ثابت extendp = جديد AstExtend{fl, rhsp, lhs_پلنوالی};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(ریښتیا);
            nodep->rhsp(extendp);
        } وإلا إذا (rhs_پلنوالی > lhs_پلنوالی) {
            File_کرښه* ثابت fl = nodep->fileline();
            Ast_غوټه_DType* ثابت dtypep = nodep->dtypep();
            Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت extendp
                = VN_IS(nodep, ښويدنه_RS)
                      ? static_cast<Ast_غوټه_بیان*>(جديد AstExtendS{fl, lhsp, rhs_پلنوالی})
                      : static_cast<Ast_غوټه_بیان*>(جديد AstExtend{fl, lhsp, rhs_پلنوالی});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(ریښتیا);
            nodep->lhsp(extendp);
            nodep->dtype_سېټ_Logic_Sized(rhs_پلنوالی, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlink_Fr_شا(&relinker);
            AstSel* ثابت selp = جديد AstSel{fl, nodep, 0, lhs_پلنوالی};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            أرجع;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ لیدنه(Ast_ښويدنه_L* nodep) override { handle_ښويدنه(nodep); }
    فراغ لیدنه(Ast_ښويدنه_R* nodep) override { handle_ښويدنه(nodep); }
    فراغ لیدنه(Ast_ښويدنه_RS* nodep) override { handle_ښويدنه(nodep); }
    فراغ لیدنه(Ast_غوټه_Biop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ لیدنه(Ast_غوټه_Uniop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp());
    }
    فراغ لیدنه(Ast_غوټه_Triop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    فراغ لیدنه(AstCond* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        إذا (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtype_سېټ_String();
            أرجع;
        }
        // Fall back to "(ite cond then else)"
        لیدنه(static_cast<Ast_غوټه_Triop*>(nodep));
    }
    فراغ لیدنه(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->srcp());
    }
    فراغ لیدنه(AstSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        VNRelinker handle;
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت msbp = جديد AstSFormatF{
            fl, "%1d", خطأ,
            جديد Ast_جمع_کړه{fl, nodep->lsbp()->clone_ونه_خالص(خطأ),
                       جديد AstConst{fl, static_cast<uint32_t>(nodep->پلنوالی_Const() - 1)}}};
        Ast_غوټه_بیان* ثابت lsbp
            = جديد AstSFormatF{fl, "%1d", خطأ, nodep->lsbp()->unlink_Fr_شا(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    فراغ لیدنه(AstStructSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        ثابت منطقي outermost = !m_defer_Rand_ډېرپېښ_Hoist;
        Ast_غوټه_بیان* ثابت origp = outermost ? nodep->clone_ونه(خطأ) : تش_اشاره;
        VL_RESTORER(m_structSel);
        m_structSel = ریښتیا;
        إذا (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_غوټه_بیان* ثابت fromp = nodep->fromp();
            AstStructDType* ثابت structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            إذا (is_Rand_Qualified_لار(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } وإلا {
                structp->markConstrainedRand(ریښتیا);
            }
        }
        // Mark Random for structArray
        إذا (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_غوټه_بیان* ثابت fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ثابت structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            إذا (is_Rand_Qualified_لار(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } وإلا {
                structp->markConstrainedRand(ریښتیا);
            }
        }
        {
            VL_RESTORER(m_defer_Rand_ډېرپېښ_Hoist);
            m_defer_Rand_ډېرپېښ_Hoist = ریښتیا;
            iterateChildren(nodep);
        }
        File_کرښه* ثابت fl = nodep->fileline();
        AstSFormatF* newp = تش_اشاره;
        إذا (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = جديد AstSFormatF{fl, "%s.%s." + nodep->name(), خطأ,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_ونه_خالص(ریښتیا)};
            إذا (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } وإلا {
            newp = جديد AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), خطأ,
                                   تش_اشاره};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        إذا (origp && !hoist_Rand_ډېرپېښ_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(ټېل_Deletep(origp), origp);
        }
    }
    فراغ جمع_کړه_String_Name_Part(Ast_غوټه_بیان* ثابت indexp, ثابت std::string& fmt = "%32x") {
        إذا (m_nestedAccess) {
            Ast_غوټه_بیان* ثابت bitp = indexp->clone_ونه_خالص(خطأ);
            Ast_غوټه_بیان* ثابت بټ_Formatp = جديد AstSFormatF{bitp->fileline(), fmt, خطأ, bitp};
            AstSFormatF* ثابت herep
                = جديد AstSFormatF{indexp->fileline(), "%s.%s", خطأ, بټ_Formatp};
            m_nestedAccess->جمع_کړه_Var_Name_Part(herep, indexp->name());
        }
    }
    فراغ لیدنه(AstAssocSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_کرښه* ثابت fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_غوټه_بیان* ثابت origp = nodep->clone_ونه(خطأ);
        AstSFormatF* newp = تش_اشاره;
        إذا (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            جمع_کړه_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_غوټه_بیان* ثابت idxp = جديد AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), خطأ,
                                                      nodep->bitp()->unlink_Fr_شا(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وإلا إذا (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ثابت stringp = VN_AS(nodep->bitp(), CvtPackString);
            ثابت اندازه_t string_اندازه = VN_AS(stringp->lhsp(), Const)->پلنوالی();
            إذا (string_اندازه > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << string_اندازه << "bits, limit is 128 bits");
            }
            جمع_کړه_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_غوټه_بیان* ثابت idxp = جديد AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), خطأ,
                                                      stringp->lhsp()->unlink_Fr_شا(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وإلا {
            إذا (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ثابت صحيح actual_پلنوالی = nodep->bitp()->پلنوالی();
                std::string fmt;
                // Normalize to standard bit width
                إذا (actual_پلنوالی <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } وإلا إذا (actual_پلنوالی <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } وإلا {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::منزل_string(VL_ټکي_I(actual_پلنوالی) * 8) + "x";
                }
                إذا (m_nestedAccess) جمع_کړه_String_Name_Part(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_غوټه_بیان* ثابت idxp
                    = جديد AstSFormatF{fl, fmt, خطأ, nodep->bitp()->unlink_Fr_شا(&handle)};
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
        إذا (!newp || !hoist_Rand_ډېرپېښ_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(origp->delete_ونه(), origp);
        }
    }
    فراغ rename_Array_بیان(AstSFormatF* ثابت newp) ثابت {
        إذا (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    فراغ لیدنه(AstArraySel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_کرښه* ثابت fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        منطقي شاخص_Is_Rand = خطأ;
        nodep->bitp()->foreach([&](ثابت Ast_غوټه_Var_Ref* vrefp) {
            إذا (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                شاخص_Is_Rand = ریښتیا;
            }
        });
        إذا (m_nestedAccess) {
            Ast_غوټه_بیان* ثابت bitp = nodep->bitp()->clone_ونه_خالص(خطأ);
            Ast_غوټه_بیان* ثابت بټ_Formatp
                = شاخص_Is_Rand ? جديد AstSFormatF{bitp->fileline(), bitp->name(), خطأ, تش_اشاره}
                              : جديد AstSFormatF{bitp->fileline(), "%x", خطأ, bitp};
            AstSFormatF* ثابت herep
                = جديد AstSFormatF{nodep->fileline(), "%s.%s", خطأ, بټ_Formatp};
            إذا (AstSel* ثابت selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->جمع_کړه_Var_Name_Part(herep, selp->fromp()->name());
            } وإلا {
                m_nestedAccess->جمع_کړه_Var_Name_Part(herep, bitp->name());
            }
            إذا (شاخص_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_ونه(), bitp);
                أرجع;
            }
        }
        إذا (شاخص_Is_Rand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_غوټه_بیان* indexp = nodep->bitp()->unlink_Fr_شا(&handle);
            إذا (indexp->پلنوالی() < 32) {
                AstExtend* ثابت extendp = جديد AstExtend{fl, indexp, 32};
                extendp->user1(ریښتیا);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_بیان(newp);
        } وإلا {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_غوټه_بیان* ثابت origp = nodep->clone_ونه(خطأ);
            Ast_غوټه_بیان* ثابت indexp
                = جديد AstSFormatF{fl, "#x%8x", خطأ, nodep->bitp()->unlink_Fr_شا(&handle)};
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_بیان(newp);
            إذا (!newp || !hoist_Rand_ډېرپېښ_Over_Select_And_Member(newp, origp)) {
                VL_DO_DANGLING(origp->delete_ونه(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ساكن منطقي is_Rand_ډېرپېښ_Gate(ثابت Ast_غوټه_بیان* exprp) {
        ثابت AstCMethodHard* ثابت atp = VN_CAST(exprp, CMethodHard);
        إذا (!atp || atp->لاره() != VCMethod::ARRAY_AT) أرجع خطأ;
        ثابت AstVar* varp = تش_اشاره;
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
    منطقي hoist_Rand_ډېرپېښ_Over_Select_And_Member(AstSFormatF* newp, Ast_غوټه_بیان* origp) {
        إذا (m_defer_Rand_ډېرپېښ_Hoist) أرجع خطأ;
        // Only for selects yielding a non-array element (full chains)
        إذا (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::ویکټور<AstSFormatF*> چوکاټونه;
        AstCond* ډېرپېښ_Condp = تش_اشاره;
        لكل (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_غوټه_بیان* ثابت firstp = curp->exprsp();
            چوکاټونه.ټېل_شا(curp);
            إذا (AstCond* ثابت condp = VN_CAST(firstp, Cond)) {
                إذا (is_Rand_ډېرپېښ_Gate(condp->condp())) ډېرپېښ_Condp = condp;
                اكسر;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        إذا (!ډېرپېښ_Condp) أرجع خطأ;
        File_کرښه* ثابت fl = newp->fileline();
        Ast_غوټه_بیان* ثابت modep = ډېرپېښ_Condp->condp()->unlink_Fr_شا();
        Ast_غوټه_بیان* activep = ډېرپېښ_Condp->thenp()->unlink_Fr_شا();
        // Rebuild the select chain text around the SMT name, innermost first
        لكل (خپلکاری it = چوکاټونه.rbegin(); it != چوکاټونه.rend(); ++it) {
            Ast_غوټه_بیان* ثابت پاتې_شوی_Argsp = VN_CAST((*it)->exprsp()->nextp(), غوټه_بیان);
            إذا (پاتې_شوی_Argsp) { پاتې_شوی_Argsp->unlink_Fr_شا_With_بل(); }
            activep = جديد AstSFormatF{fl, (*it)->name(), خطأ,
                                      Ast_غوټه::جمع_کړه_بل(activep, پاتې_شوی_Argsp)};
        }
        AstCond* ثابت hoistp = جديد AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(ریښتیا);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(ټېل_Deletep(newp), newp);
        أرجع ریښتیا;
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        إذا (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_کرښه* ثابت fl = nodep->fileline();
            إذا (m_nestedAccess) {
                AstSFormatF* ثابت formatp = جديد AstSFormatF{fl, nodep->name(), خطأ, تش_اشاره};
                m_nestedAccess->جمع_کړه_Var_Name_Part(جديد AstSFormatF{fl, "%s.%s", خطأ, formatp},
                                               nodep->name());
            }
            Ast_غوټه* ریښه_غوټه = nodep->fromp();
            طالما (ثابت AstMemberSel* ثابت selp = VN_CAST(ریښه_غوټه, MemberSel)) {
                ریښه_غوټه = selp->fromp();
            }

            Ast_غوټه_Sel* arraySelp = VN_CAST(ریښه_غوټه, ArraySel);
            إذا (!arraySelp) arraySelp = VN_CAST(ریښه_غوټه, AssocSel);

            إذا (arraySelp) {
                Ast_غوټه_DType* ثابت arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_غوټه_DType* ثابت elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : تش_اشاره;
                إذا (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ثابت منطقي isSimple
                        = nodep->fromp() == ریښه_غوټه && VN_IS(arraySelp->fromp(), VarRef);
                    إذا (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = جديد Nested_Access_لار(nodep->clone_ونه(خطأ),
                                                              &m_لومړی_افاده_Inside_Indexp);
                        AstSFormatF* ثابت formatp
                            = جديد AstSFormatF{fl, nodep->name(), خطأ, تش_اشاره};
                        m_nestedAccess->جمع_کړه_Var_Name_Part(
                            جديد AstSFormatF{fl, "%s.%s", خطأ, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = ریښتیا;
                    nodep->user1(ریښتیا);
                    arraySelp->user1(ریښتیا);
                    Ast_غوټه_بیان* ثابت origp = nodep->clone_ونه(خطأ);
                    iterateChildren(nodep);
                    AstSFormatF* ثابت newp = جديد AstSFormatF{fl, "%s." + nodep->name(), خطأ,
                                                              nodep->fromp()->unlink_Fr_شا()};
                    nodep->replaceWith(newp);
                    إذا (!hoist_Rand_ډېرپېښ_Over_Select_And_Member(newp, origp))
                        VL_DO_DANGLING(origp->delete_ونه(), origp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    VL_DO_DANGLING(ړنګول m_nestedAccess, m_nestedAccess);
                    أرجع;
                }
            }
            // Check if the root variable participates in global constraints
            إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(ریښه_غوټه, VarRef)) {
                AstVar* ثابت constrainedVar = varRefp->varp();
                إذا (constrainedVar->globalConstrained()) {
                    منطقي ثابت beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    إذا (beforeIterate) {
                        nodep->replaceWith(جديد AstSFormatF{fl, "%s." + nodep->name(), خطأ,
                                                           nodep->fromp()->unlink_Fr_شا()});
                    } وإلا {
                        nodep->replaceWith(nodep->fromp()->unlink_Fr_شا());
                    }
                    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                    أرجع;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        إذا (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), غوټه_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ثابت varRefp
                = جديد AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), غوټه_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            لیدنه(varRefp);
        } وإلا إذا (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->لاره() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ثابت cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_غوټه_DType* ثابت arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_غوټه_DType* ثابت elemDtp = arrayDtp->subDTypep()->skipRefp();
            إذا (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = ریښتیا;
                iterateChildren(nodep);
                File_کرښه* ثابت fl = nodep->fileline();
                AstSFormatF* newp = تش_اشاره;
                إذا (AstSFormatF* ثابت fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    إذا (fromp->name() == "%s.%s") {
                        newp = جديد AstSFormatF{fl, "%s.%s." + nodep->name(), خطأ,
                                               fromp->exprsp()->clone_ونه_خالص(ریښتیا)};
                    } وإلا {
                        newp = جديد AstSFormatF{fl, fromp->name() + "." + nodep->name(), خطأ,
                                               تش_اشاره};
                    }
                } وإلا {
                    newp = جديد AstSFormatF{fl, nodep->name(), خطأ, تش_اشاره};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_شا());
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        } وإلا إذا (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_شا());
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        } وإلا {
            editFormat(nodep);
        }
    }
    فراغ لیدنه(AstSFormatF* nodep) override {}
    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {}
    فراغ لیدنه(AstCStmt* nodep) override {}
    فراغ لیدنه(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    فراغ لیدنه(AstConstraintIf* nodep) override {
        Ast_غوټه_بیان* newp = تش_اشاره;
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت thenp = editSingle(fl, nodep->thensp());
        Ast_غوټه_بیان* ثابت elsep = editSingle(fl, nodep->elsesp());
        إذا (thenp && elsep) {
            newp = جديد AstCond{fl, nodep->condp()->unlink_Fr_شا(), thenp, elsep};
        } وإلا إذا (thenp) {
            newp = جديد AstLogIf{fl, nodep->condp()->unlink_Fr_شا(), thenp};
        } وإلا إذا (elsep) {
            newp = جديد AstLogIf{fl, جديد AstNot{fl, nodep->condp()->unlink_Fr_شا()}, elsep};
        }
        إذا (newp) {
            newp->dtype_سېټ_بټ();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(ریښتیا);  // Assume result-dependent
            nodep->replaceWith(جديد Ast_Constraint_بیان{fl, newp});
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_Gen_بلاک* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    فراغ لیدنه(AstBegin* nodep) override {}
    فراغ لیدنه(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_کرښه* ثابت fl = nodep->fileline();

        إذا (!nodep->bodyp()) {
            nodep->unlink_Fr_شا();
        } وإلا إذا (m_wantSingle) {
            Ast_غوټه_بیان* ثابت itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            cstmtp->جمع_کړه("ret += \" \";\n");
            cstmtp->جمع_کړه("ret += ");
            cstmtp->جمع_کړه(itemp);
            cstmtp->جمع_کړه(";");
            Ast_غوټه* ثابت bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->dtype_سېټ_String();
            cexprp->جمع_کړه("([&]{\nstd::string ret;\n");
            cexprp->جمع_کړه(جديد AstBegin{fl, m_unique_نومونه.get("__VrandBlock"),
                                     جديد AstForeach{fl, nodep->headerp()->unlink_Fr_شا(), bodyp},
                                     ریښتیا});
            cexprp->جمع_کړه("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
        } وإلا {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            لكل (Ast_غوټه* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                إذا (ثابت AstVar* ثابت varp = VN_CAST(elemp, Var)) m_foreachIdxVars.ورننباسه(varp);
            }
            iterate_And_بل_Null(nodep->bodyp());
            Ast_غوټه* ثابت bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlink_Fr_شا_With_بل());
            nodep->replaceWith(
                جديد AstBegin{fl, m_unique_نومونه.get("__VrandBlock"),
                             جديد AstForeach{fl, nodep->headerp()->unlink_Fr_شا(), bodyp}, ریښتیا});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_Module* ثابت genModp = VN_AS(m_genp->user2p(), غوټه_Module);

        لكل (Ast_غوټه_بیان* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), غوټه_بیان)) {
            Ast_غوټه_بیان* ثابت lhsTestp = build_Solve_Before_Name_بیان(fl, lhsp);
            إذا (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                استمر;
            }
            VL_DO_DANGLING(lhsTestp->delete_ونه(), lhsTestp);
            لكل (Ast_غوټه_بیان* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), غوټه_بیان)) {
                Ast_غوټه_بیان* ثابت rhsNamep = build_Solve_Before_Name_بیان(fl, rhsp);
                إذا (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    استمر;
                }
                AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                    fl, جديد AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtype_سېټ_Void();
                callp->جمع_کړه_Pinsp(build_Solve_Before_Name_بیان(fl, lhsp));
                callp->جمع_کړه_Pinsp(rhsNamep);
                nodep->جمع_کړه_دلته_This_As_بل(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstConstraintUnique* nodep) override {
        Ast_غوټه_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_Module* genModp = تش_اشاره;

        إذا (m_classp) {
            initTaskp = VN_AS(m_memberMap.پیدا_کړه_Member(m_classp, "new"), غوټه_FTask);
            genVarp = getRandomGenerator(m_classp);
        } وإلا {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        إذا (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        إذا (m_classp) {
            genModp = VN_AS(genVarp->user2p(), غوټه_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_غوټه* تیاری_Stmtsp = تش_اشاره;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::ویکټور<std::string> smtExprs;
        لكل (Ast_غوټه* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            إذا (AstVarRef* ثابت varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ثابت varp = varRefp->varp();
                Ast_غوټه_Module* ثابت varModp = VN_AS(varp->user2p(), غوټه_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_غوټه_DType* ثابت dtypep = varp->dtypep()->skipRefp();

                ثابت ساكن خپلکاری dynDTypeSupported
                    = [](ثابت Ast_غوټه_DType* ثابت dtypep) -> منطقي {
                    أرجع VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, کتار_DType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elem_پلنوالی;
                uint32_t static_اندازه = 0;  // Element count, zero when only known at run time
                إذا (ثابت AstUnpackArrayDType* ثابت پورته = VN_CAST(dtypep, UnpackArrayDType)) {
                    ثابت AstRange* ثابت rangep = پورته->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    static_اندازه = پورته->elementsConst();
                    إذا (static_اندازه > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        استمر;
                    }
                    elem_پلنوالی = static_cast<uint64_t>(varp->dtypep()->پلنوالی());
                } وإلا إذا (dynDTypeSupported(dtypep)) {
                    elem_پلنوالی = static_cast<uint64_t>(dtypep->subDTypep()->پلنوالی());
                } وإلا {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(خطأ));
                    استمر;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ثابت Ast_غوټه_DType* ثابت subp = dtypep->subDTypep()->skipRefp();
                إذا (VN_IS(subp, غوټه_Array_DType) || VN_IS(subp, کتار_DType)
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
                    varp->user3(ریښتیا);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_غوټه_بیان* ثابت varnamep = جديد AstCExpr{
                        fl, AstCExpr::خالص{}, "\"" + varp->name() + "\"", varp->پلنوالی()};

                    AstCMethodHard* ثابت writeVarCallp = جديد AstCMethodHard{
                        fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ثابت argVarRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                    إذا (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->جمع_کړه_Pinsp(argVarRefp);
                    writeVarCallp->جمع_کړه_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, elem_پلنوالی});
                    writeVarCallp->جمع_کړه_Pinsp(varnamep);
                    writeVarCallp->جمع_کړه_Pinsp(جديد AstConst{fl, 1});  // Dimension
                    ثابت Randomize_ډېرپېښ rand_ډېرپېښ = {.asUQuad = varp->user1()};
                    إذا (rand_ډېرپېښ.کاروي_ډېرپېښ) {
                        writeVarCallp->جمع_کړه_Pinsp(
                            جديد AstConst{fl, AstConst::Unsized64{}, rand_ډېرپېښ.شاخص});
                    }
                    writeVarCallp->dtype_سېټ_Void();
                    initTaskp->جمع_کړه_Stmtsp(جديد Ast_Stmt_بیان{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                إذا (varp->user4p() && m_اندازه_Constrained_Arraysp) {
                    m_اندازه_Constrained_Arraysp->ورننباسه(varp);
                }

                إذا (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    إذا (static_اندازه < 2) استمر;
                    std::string بیان_Str = "(__Vbv (distinct";
                    لكل (uint32_t i = 0; i < static_اندازه; ++i) {
                        بیان_Str += " (select " + varp->name() + " #x";
                        لكل (صحيح ښويدنه_2 = 28; ښويدنه_2 >= 0; ښويدنه_2 -= 4) {
                            بیان_Str += "0123456789abcdef"[(i >> ښويدنه_2) & 0xf];
                        }
                        بیان_Str += ')';
                    }
                    smtExprs.emplace_شا(بیان_Str + "))");
                    استمر;
                }

                Ast_غوټه_بیان* ثابت namep = جديد AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ثابت randUniqueCallp
                    = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->پیدا_کړه_Void_DType());
                randUniqueCallp->جمع_کړه_Pinsp(جديد AstConst{fl, *m_uniqueConstraintId});
                تیاری_Stmtsp = Ast_غوټه::جمع_کړه_بل(تیاری_Stmtsp, جديد Ast_Stmt_بیان{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        إذا (m_wantSingle && !smtExprs.تشه()) {
            std::string بیان_Str = smtExprs.front();
            إذا (smtExprs.اندازه() > 1) {
                بیان_Str = "(bvand";
                لكل (ثابت std::string& oneExprp : smtExprs) بیان_Str += " " + oneExprp;
                بیان_Str += ')';
            }
            AstSFormatF* ثابت newp = جديد AstSFormatF{fl, بیان_Str, خطأ, تش_اشاره};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (تیاری_Stmtsp) nodep->جمع_کړه_دلته_This_As_بل(تیاری_Stmtsp);
        nodep->unlink_Fr_شا();
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(Ast_Constraint_بیان* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_لومړی_افاده_Inside_Indexp);
        إذا (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_غوټه_بیان* ثابت exprp = nodep->exprp();
            إذا (exprp->پلنوالی() > 1) {
                File_کرښه* ثابت fl = exprp->fileline();
                ثابت V3_شمېره عدد_Zero{fl, exprp->پلنوالی(), 0};
                Ast_غوټه_بیان* ثابت neqp
                    = جديد AstNeq{fl, exprp->unlink_Fr_شا(), جديد AstConst{fl, عدد_Zero}};
                neqp->user1(ریښتیا);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        إذا (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_شا());
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ثابت VCMethod لاره
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), غوټه_Module), m_genp,
                          VAccess::READWRITE},
            لاره, nodep->exprp()->unlink_Fr_شا()};
        callp->dtype_سېټ_Void();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->جمع_کړه_Pinsp(جديد AstCExpr{nodep->fileline(), AstCExpr::خالص{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ثابت uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->جمع_کړه_Pinsp(جديد AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string pretty_متن;
        إذا (!v3Global.opt.protectIds()) {
            pretty_متن = nodep->fileline()->pretty_سرچینه();
            اندازه_t pos = 0;
            طالما ((pos = pretty_متن.پیدا_کړه('"', pos)) != std::string::npos) {
                pretty_متن.ورننباسه(pos, "\\");
                pos += std::مزي_اوږدوالی("\\\"");
            }
        }
        callp->جمع_کړه_Pinsp(
            جديد AstCExpr{nodep->fileline(), AstCExpr::خالص{}, "\"" + pretty_متن + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstCMethodHard* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_کرښه* ثابت fl = nodep->fileline();

        إذا (nodep->لاره() == VCMethod::ARRAY_AT) {
            جمع_کړه_String_Name_Part(nodep->pinsp(), "%x");
            ثابت منطقي شاخص_Is_Rand = nodep->pinsp()->exists([](ثابت Ast_غوټه_Var_Ref* ثابت vrefp) {
                أرجع vrefp->varp()->rand().isRandomizable();
            });
            إذا (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ثابت Ast_غوټه* ثابت basep = nodep->بنسټ_Fromp(ریښتیا);
                    إذا (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        أرجع;
                    }
                }
                Ast_غوټه_بیان* ثابت origp = شاخص_Is_Rand ? تش_اشاره : nodep->clone_ونه(خطأ);
                ثابت AstVarRef* ثابت بنسټ_Refp
                    = m_structSel ? VN_CAST(nodep->fromp()->بنسټ_Fromp(ریښتیا), VarRef) : تش_اشاره;
                إذا (بنسټ_Refp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(بنسټ_Refp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_غوټه_بیان* ثابت indexp = nodep->pinsp()->clone_ونه_خالص(خطأ);
                    Ast_غوټه_بیان* ثابت arrayp = nodep->fromp()->clone_ونه_خالص(خطأ);
                    AstCMethodHard* ثابت currentSizep
                        = جديد AstCMethodHard{fl, arrayp->clone_ونه_خالص(خطأ), VCMethod::DYN_اندازه};
                    currentSizep->dtype_سېټ_Int();
                    Ast_غوټه_بیان* ثابت دننه_Range_Requiredp = جديد AstLogAnd{
                        fl,
                        جديد AstLteS{fl,
                                    جديد AstConst{fl, AstConst::Widthed_ارزښت{}, indexp->پلنوالی(), 0},
                                    indexp->clone_ونه_خالص(خطأ)},
                        جديد AstGteS{fl, indexp->clone_ونه_خالص(خطأ), currentSizep}};
                    AstCMethodHard* ثابت resizep
                        = جديد AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             جديد Ast_جمع_کړه{fl, indexp, جديد AstConst{fl, 1}}};
                    resizep->dtype_سېټ_Void();
                    m_prepareConstrainedArraysp->جمع_کړه_Stmtsp(
                        جديد AstIf{fl, دننه_Range_Requiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ثابت sizep
                    = m_structSel ? جديد AstCMethodHard{fl, nodep->fromp()->clone_ونه_خالص(خطأ),
                                                       VCMethod::DYN_اندازه}
                                  : تش_اشاره;
                Ast_غوټه_بیان* ثابت originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_غوټه_بیان* ثابت pinp = nodep->pinsp()->unlink_Fr_شا();
                AstSFormatF* newp;
                إذا (m_structSel) {
                    إذا (AstSFormatF* ثابت sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_غوټه_بیان* ثابت argsp
                        = Ast_غوټه::جمع_کړه_بل(nodep->fromp()->unlink_Fr_شا(), pinp);
                    sizep->dtype_سېټ_Int();
                    AstLogAnd* ثابت condp
                        = جديد AstLogAnd{fl,
                                        جديد AstLteS{fl,
                                                    جديد AstConst{fl, AstConst::Widthed_ارزښت{},
                                                                 originalPinp->پلنوالی(), 0},
                                                    originalPinp->clone_ونه_خالص(خطأ)},
                                        جديد AstLtS{fl, originalPinp->clone_ونه_خالص(خطأ), sizep}};
                    m_conditionp = m_conditionp ? جديد AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = جديد AstSFormatF{fl, "%s.%s", خطأ, argsp};
                } وإلا {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_شا(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                إذا (origp && !hoist_Rand_ډېرپېښ_Over_Select_And_Member(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_ونه(), origp);
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

        إذا (nodep->لاره() == VCMethod::ARRAY_INSIDE) {
            ثابت منطقي randArr = nodep->fromp()->user1();

            AstVar* ثابت newVarp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->پیدا_کړه_Int_DType()};
            Ast_غوټه_بیان* ثابت idxRefp = جديد AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            Ast_Foreach_سرلیک* ثابت headerp
                = جديد Ast_Foreach_سرلیک{fl, nodep->fromp()->clone_ونه_خالص(خطأ), newVarp};
            Ast_غوټه_بیان* ثابت selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_غوټه* ثابت itemp = جديد AstEq{fl, selp, nodep->pinsp()->unlink_Fr_شا()};
            itemp->user1(ریښتیا);

            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            cstmtp->جمع_کړه("ret += \" \";\n");
            cstmtp->جمع_کړه("ret += ");
            cstmtp->جمع_کړه(iterateSubtreeReturnEdits(itemp));
            cstmtp->جمع_کړه(";");
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->dtype_سېټ_String();
            cexprp->جمع_کړه("([&]{\nstd::string ret;\n");
            cexprp->جمع_کړه(جديد AstBegin{fl, m_unique_نومونه.get("__VrandBlock"),
                                     جديد AstForeach{fl, headerp, cstmtp}, ریښتیا});
            cexprp->جمع_کړه("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        }

        // Array reduction methods (sum, product, and, or, xor)
        إذا (nodep->لاره() == VCMethod::ARRAY_R_مجموعه
            || nodep->لاره() == VCMethod::ARRAY_R_حاصل_ضرب
            || nodep->لاره() == VCMethod::ARRAY_R_AND || nodep->لاره() == VCMethod::ARRAY_R_OR
            || nodep->لاره() == VCMethod::ARRAY_R_XOR) {

            AstWith* ثابت withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            إذا (withp) {
                إذا (AstUnpackArrayDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ثابت صحيح array_اندازه_2 = adtypep->elementsConst();
                    إذا (array_اندازه_2 > v3Global.opt.constraint_Array_حد()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvt_منزل_Str(array_اندازه_2)
                                          + " exceeds --constraint-array-limit of "
                                          + cvt_منزل_Str(v3Global.opt.constraint_Array_حد())
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
                = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->پیدا_کړه_Int_DType()};
            Ast_Foreach_سرلیک* ثابت headerp
                = جديد Ast_Foreach_سرلیک{fl, nodep->fromp()->clone_ونه_خالص(خطأ), loopVarp};

            // Determine SMT operation and compute identity elements
            ثابت حرف* smtOp = تش_اشاره;
            std::string بې_اغېزه_عنصر;
            إذا (withp) {
                // For 'with' clause: use binary format, compute from result width
                ثابت صحيح پلنوالی = nodep->dtypep()->پلنوالی();
                إذا (nodep->لاره() == VCMethod::ARRAY_R_مجموعه) {
                    smtOp = "bvadd";
                    بې_اغېزه_عنصر = "#b" + std::string(پلنوالی, '0');
                } وإلا إذا (nodep->لاره() == VCMethod::ARRAY_R_حاصل_ضرب) {
                    smtOp = "bvmul";
                    بې_اغېزه_عنصر = (پلنوالی > 0) ? "#b" + std::string(پلنوالی - 1, '0') + "1" : "#b0";
                } وإلا إذا (nodep->لاره() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    بې_اغېزه_عنصر = "#b" + std::string(پلنوالی, '1');
                } وإلا إذا (nodep->لاره() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    بې_اغېزه_عنصر = "#b" + std::string(پلنوالی, '0');
                } وإلا {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    بې_اغېزه_عنصر = "#b" + std::string(پلنوالی, '0');
                }
            } وإلا {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ثابت arrVarp = arrRefp->varp();
                Ast_غوټه_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحيح elem_پلنوالی = elemDtp->پلنوالی();
                ثابت صحيح hexDigits = (elem_پلنوالی + 3) / 4;

                ثابت std::string zero_بې_اغېزه_عنصر = "#x" + std::string(hexDigits, '0');
                ثابت std::string one_بې_اغېزه_عنصر = "#x" + std::string(hexDigits - 1, '0') + "1";
                ثابت std::string all_Ones_بې_اغېزه_عنصر = "#x" + std::string(hexDigits, 'f');

                إذا (nodep->لاره() == VCMethod::ARRAY_R_مجموعه) {
                    smtOp = "bvadd";
                    بې_اغېزه_عنصر = zero_بې_اغېزه_عنصر;
                } وإلا إذا (nodep->لاره() == VCMethod::ARRAY_R_حاصل_ضرب) {
                    smtOp = "bvmul";
                    بې_اغېزه_عنصر = one_بې_اغېزه_عنصر;
                } وإلا إذا (nodep->لاره() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    بې_اغېزه_عنصر = all_Ones_بې_اغېزه_عنصر;
                } وإلا إذا (nodep->لاره() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    بې_اغېزه_عنصر = zero_بې_اغېزه_عنصر;
                } وإلا {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    بې_اغېزه_عنصر = zero_بې_اغېزه_عنصر;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            إذا (withp) {
                // With 'with' clause: evaluate expression for each element
                ثابت منطقي randArr = nodep->fromp()->user1();
                Ast_غوټه_بیان* ثابت idxRefp = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_غوټه_بیان* ثابت elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ثابت صحيح پایله_پلنوالی = nodep->dtypep()->پلنوالی();
                ثابت VSigning پایله_Signing = nodep->dtypep()->numeric();

                Ast_غوټه* فی_Elem_Exprp = withp->exprp()->clone_ونه_خالص(خطأ);
                إذا (AstLambdaArgRef* ثابت ریښه_Refp = VN_CAST(فی_Elem_Exprp, LambdaArgRef)) {
                    إذا (ریښه_Refp->شاخص()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ثابت loopRef = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                        فی_Elem_Exprp = adjust_پلنوالی(fl, loopRef, پایله_پلنوالی, پایله_Signing);
                    } وإلا {
                        // item at root -> replace with array element
                        فی_Elem_Exprp = elemSelp->clone_ونه_خالص(خطأ);
                    }
                    VL_DO_DANGLING(ریښه_Refp->delete_ونه(), ریښه_Refp);
                } وإلا {
                    فی_Elem_Exprp->foreach([&](AstLambdaArgRef* refp) {
                        إذا (refp->شاخص()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ثابت loopRef = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_غوټه* ثابت replacement
                                = adjust_پلنوالی(fl, loopRef, پایله_پلنوالی, پایله_Signing);
                            refp->replaceWith(replacement);
                        } وإلا {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_ونه_خالص(خطأ));
                        }
                        VL_DO_DANGLING(ټېل_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_ونه(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                فی_Elem_Exprp = V3Const::constifyEdit(فی_Elem_Exprp);

                فی_Elem_Exprp->foreach([&](Ast_غوټه* nodep) {
                    // Don't mark loop variable references as randomizable
                    إذا (!VN_IS(nodep, Const)) {
                        إذا (Ast_غوټه_Var_Ref* ثابت vrefp = VN_CAST(nodep, غوټه_Var_Ref)) {
                            إذا (vrefp->varp() != loopVarp) nodep->user1(ریښتیا);
                        } وإلا {
                            nodep->user1(ریښتیا);
                        }
                    }
                });

                cstmtp->جمع_کړه("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->جمع_کړه("ret += ");
                cstmtp->جمع_کړه(iterateSubtreeReturnEdits(فی_Elem_Exprp));
                cstmtp->جمع_کړه(";\n");
                cstmtp->جمع_کړه("ret += \")\";\n");
            } وإلا {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ثابت arrVarp = arrRefp->varp();
                ثابت std::string smtArrayName = arrVarp->name();
                Ast_غوټه_DType* ثابت elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحيح elem_پلنوالی = elemDtp->پلنوالی();
                Ast_غوټه_Module* ثابت arrModulep = VN_AS(arrVarp->user2p(), غوټه_Module);
                Ast_غوټه_Module* ثابت genModulep = VN_AS(m_genp->user2p(), غوټه_Module);
                arrVarp->user3(ریښتیا);

                // Add write_var call to init task
                Ast_غوټه_بیان* ثابت arrVarNamep
                    = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + smtArrayName + "\"", elem_پلنوالی};
                AstCMethodHard* ثابت writeVarCallp
                    = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->جمع_کړه_Pinsp(جديد AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->جمع_کړه_Pinsp(
                    جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elem_پلنوالی)});
                writeVarCallp->جمع_کړه_Pinsp(arrVarNamep);
                ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(خطأ).دوهم;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->جمع_کړه_Pinsp(جديد AstConst{fl, 1});  // Dimension

                ثابت Randomize_ډېرپېښ rand_ډېرپېښ = {.asUQuad = arrVarp->user1()};
                إذا (rand_ډېرپېښ.کاروي_ډېرپېښ) {
                    writeVarCallp->جمع_کړه_Pinsp(
                        جديد AstConst{fl, AstConst::Unsized64{}, rand_ډېرپېښ.شاخص});
                }
                writeVarCallp->dtype_سېټ_Void();
                Ast_غوټه_FTask* initTaskp = m_inlineInitTaskp;
                إذا (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.پیدا_کړه_Member(arrModulep, "new"), غوټه_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->جمع_کړه_Stmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ثابت idxFormatp = جديد AstSFormatF{
                    fl, "#x%08x", خطأ, جديد AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ثابت varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_شا(), idxFormatp);

                cstmtp->جمع_کړه("ret += \" \";\n");
                cstmtp->جمع_کړه("ret += ");
                cstmtp->جمع_کړه(varNamep);
                cstmtp->جمع_کړه(";\n");
            }

            // Build final expression
            إذا (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
                cexprp->dtype_سېټ_String();
                cexprp->جمع_کړه("([&]{\nstd::string ret = \"" + بې_اغېزه_عنصر + "\";\n");
                cexprp->جمع_کړه(جديد AstBegin{fl, m_unique_نومونه.get("__VrandBlock"),
                                         جديد AstForeach{fl, headerp, cstmtp}, ریښتیا});
                cexprp->جمع_کړه("return ret;\n})()");
                nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            } وإلا {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
                cexprp->dtype_سېټ_String();
                cexprp->جمع_کړه("([&]{\nstd::string ret;\n");
                cexprp->جمع_کړه(جديد AstBegin{fl, m_unique_نومونه.get("__VrandBlock"),
                                         جديد AstForeach{fl, headerp, cstmtp}, ریښتیا});
                cexprp->جمع_کړه("return ret.empty() ? \"" + بې_اغېزه_عنصر + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        }

        // We've handled DYN_SIZE already if they should be randomized
        إذا (nodep->لاره() == VCMethod::DYN_اندازه || nodep->لاره() == VCMethod::ASSOC_اندازه) {
            Ast_غوټه_بیان* غوټه_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            طالما (VN_IS(غوټه_Exprp, MemberSel)) {
                غوټه_Exprp = VN_AS(غوټه_Exprp, MemberSel)->fromp();
            }
            إذا (VN_IS(غوټه_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_ونه_خالص(خطأ)));
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                أرجع;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(خطأ);

        إذا (editFormat(nodep)) أرجع;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        إذا (editFormat(nodep)) أرجع;

        AstFunc* ثابت funcp = VN_CAST(nodep->taskp(), Func);
        إذا (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            أرجع;
        }

        Ast_غوټه_بیان* ثابت retExprp = ایستل_Return_بیان(funcp);
        إذا (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(خطأ);
            إذا (editFormat(nodep)) أرجع;
            أرجع;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), تش_اشاره, خطأ);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_غوټه_بیان* inlinedp = retExprp->clone_ونه_خالص(خطأ);
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.لومړی;
            AstArg* ثابت argp = tconnect.دوهم;
            إذا (!argp || !argp->exprp()) استمر;
            إذا (AstVarRef* ثابت ریښه_Refp = VN_CAST(inlinedp, VarRef)) {
                إذا (ریښه_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_ونه_خالص(خطأ);
                    VL_DO_DANGLING(ریښه_Refp->delete_ونه(), ریښه_Refp);
                    استمر;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                إذا (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_ونه_خالص(خطأ));
                    VL_DO_DANGLING(refp->delete_ونه(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {}
    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_غوټه* buildDisableSoftCallStmt(Ast_Constraint_بیان* nodep) {
        std::string varName;
        إذا (ثابت Ast_غوټه_Var_Ref* ثابت vrefp = VN_CAST(nodep->exprp(), غوټه_Var_Ref)) {
            varName = vrefp->name();
        } وإلا إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } وإلا {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), غوټه_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            جديد AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtype_سېټ_Void();
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
    Ast_غوټه* ایستل_Conditional_Disable_Softs(Ast_غوټه* itemsp, Ast_غوټه_بیان* outerCondp) {
        Ast_غوټه* hoistListp = تش_اشاره;
        لكل (Ast_غوټه *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (AstConstraintIf* ثابت ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_کرښه* ثابت fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_غوټه_بیان* ثابت thenCondp
                    = outerCondp ? static_cast<Ast_غوټه_بیان*>(
                                       جديد AstLogAnd{fl, outerCondp->clone_ونه_خالص(خطأ),
                                                     ifp->condp()->clone_ونه_خالص(خطأ)})
                                 : ifp->condp()->clone_ونه_خالص(خطأ);
                إذا (Ast_غوټه* ثابت thenHoistp
                    = ایستل_Conditional_Disable_Softs(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_غوټه::جمع_کړه_بل(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_ونه(), thenCondp);
                إذا (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_غوټه_بیان* ثابت notInnerp
                        = جديد AstLogNot{fl, ifp->condp()->clone_ونه_خالص(خطأ)};
                    Ast_غوټه_بیان* ثابت elseCondp
                        = outerCondp ? static_cast<Ast_غوټه_بیان*>(جديد AstLogAnd{
                                           fl, outerCondp->clone_ونه_خالص(خطأ), notInnerp})
                                     : notInnerp;
                    إذا (Ast_غوټه* ثابت elseHoistp
                        = ایستل_Conditional_Disable_Softs(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_غوټه::جمع_کړه_بل(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_ونه(), elseCondp);
                }
            } وإلا إذا (Ast_Constraint_بیان* ثابت exprp = VN_CAST(stmtp, Constraint_بیان)) {
                إذا (exprp->isDisableSoft() && outerCondp) {
                    File_کرښه* ثابت fl = exprp->fileline();
                    AstIf* ثابت hoistIfp = جديد AstIf{fl, outerCondp->clone_ونه_خالص(خطأ),
                                                      buildDisableSoftCallStmt(exprp), تش_اشاره};
                    hoistListp = hoistListp ? Ast_غوټه::جمع_کړه_بل(hoistListp,
                                                               static_cast<Ast_غوټه*>(hoistIfp))
                                            : static_cast<Ast_غوټه*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_بیان* ثابت truep
                        = جديد Ast_Constraint_بیان{fl, جديد AstConst{fl, AstConst::بټ_True{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_ونه(), exprp);
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
    څرګند Constraint_بیان_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_غوټه* nodep,
                                   Ast_غوټه_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* rand_ډېرپېښ_Varp, std::سېټ<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3_Unique_نومونه& unique_نومونه,
                                   Ast_غوټه_FTask* memberselInitTaskp = تش_اشاره,
                                   VInsertion_سېټ<AstVar*>* اندازه_Constrained_Arraysp = تش_اشاره,
                                   Ast_غوټه_FTask* prepareConstrainedArraysp = تش_اشاره)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_rand_ډېرپېښ_Varp{rand_ډېرپېښ_Varp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_اندازه_Constrained_Arraysp{اندازه_Constrained_Arraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_unique_نومونه{unique_نومونه} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        إذا (Ast_غوټه* ثابت hoistListp
            = nodep ? ایستل_Conditional_Disable_Softs(nodep, تش_اشاره) : تش_اشاره) {
            nodep->جمع_کړه_بل(hoistListp);
        }
        iterate_And_بل_Null(nodep);
    }
};

تعداد صنف Capture_ډېرپېښ : uint8_t {
    CAP_NO = 0x0,
    CAP_ارزښت = 0x01,
    CAP_THIS = 0x02,
    CAP_F_سېټ_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
Capture_ډېرپېښ چلوونکی|(Capture_ډېرپېښ a, Capture_ډېرپېښ b) {
    أرجع static_cast<Capture_ډېرپېښ>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
Capture_ډېرپېښ چلوونکی&(Capture_ډېرپېښ a, Capture_ډېرپېښ b) {
    أرجع static_cast<Capture_ډېرپېښ>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
Capture_ډېرپېښ ډېرپېښ(Capture_ډېرپېښ a) { أرجع a & static_cast<Capture_ډېرپېښ>(0x3); }
منطقي hasFlags(Capture_ډېرپېښ a, Capture_ډېرپېښ flags) {
    أرجع ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

صنف CaptureVisitor final : عام VNVisitor {
    AstArg* m_argsp = تش_اشاره;  // Original references turned into arguments
    Ast_غوټه_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ثابت AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::سېټ<Ast_غوټه*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = تش_اشاره;  // Variable for outer context's object, if necessary
    منطقي m_staticContext = خطأ;  // True when capturing from a static function context

    // METHODS

    منطقي captureVariable(File_کرښه* ثابت fileline, Ast_غوټه_Var_Ref* varrefp, AstVar*& varp) {
        خپلکاری it = m_varCloneMap.پیدا_کړه(varrefp->varp());
        إذا (it == m_varCloneMap.پای_ټکی()) {
            AstVar* ثابت newVarp = varrefp->varp()->clone_ونه(خطأ);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(ریښتیا);
            newVarp->direction(VDirection::ننوت);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            أرجع ریښتیا;
        }
        varp = it->دوهم;
        أرجع خطأ;
    }

    کينډۍ <ډول_نوم T_غوټه>
    فراغ fixupClassOrPackage(Ast_غوټه* memberp, T_غوټه refp) {
        Ast_غوټه_Module* ثابت declClassp = VN_AS(memberp->user2p(), غوټه_Module);
        إذا (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    کينډۍ <ډول_نوم T_غوټه>
    منطقي is_Reference_منزل_Inner_Member(T_غوټه nodep) {
        أرجع VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_کرښه* fl) {
        إذا (!m_thisp) {
            AstClassRefDType* ثابت refDTypep
                = جديد AstClassRefDType{fl, VN_AS(m_callerp, Class), تش_اشاره};
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(refDTypep);
            m_thisp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(ریښتیا);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::ننوت);
            m_argsp = Ast_غوټه::جمع_کړه_بل(m_argsp, جديد AstArg{fl, "", جديد AstThisRef{fl, refDTypep}});
        }
        أرجع m_thisp;
    }

    AstVar* getVar(AstVar* ثابت varp) ثابت {
        ثابت خپلکاری it = m_varCloneMap.پیدا_کړه(varp);
        إذا (it == m_varCloneMap.پای_ټکی()) أرجع تش_اشاره;
        إذا (it->دوهم->isStdRandomizeArg()) أرجع تش_اشاره;
        أرجع it->دوهم;
    }

    Capture_ډېرپېښ get_Var_Ref_Capture_ډېرپېښ(Ast_غوټه_Var_Ref* varRefp) {
        Ast_غوټه_Module* ثابت varModp = VN_AS(varRefp->varp()->user2p(), غوټه_Module);
        AstClass* ثابت varClassp = VN_CAST(varModp, Class);
        AstClass* ثابت callerClassp = VN_CAST(m_callerp, Class);

        ثابت منطقي callerIsClass = callerClassp;
        ثابت منطقي refIsXref = VN_IS(varRefp, VarXRef);
        ثابت منطقي varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ثابت منطقي varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ثابت منطقي var_Is_ډګر_د_Caller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ثابت منطقي varIsParam = varRefp->varp()->isParam();
        ثابت منطقي varIsConstraintIterator
            = VN_IS(varRefp->varp()->لومړی_Abovep(), Foreach_سرلیک)
              && VN_IS(varRefp->varp()->لومړی_Abovep()->لومړی_Abovep(), ConstraintForeach);
        إذا (refIsXref) أرجع Capture_ډېرپېښ::CAP_ارزښت | Capture_ډېرپېښ::CAP_F_XREF;
        إذا (varIsConstraintIterator) أرجع Capture_ډېرپېښ::CAP_NO;
        إذا (varIsFuncLocal && varHasAutomaticLifetime) أرجع Capture_ډېرپېښ::CAP_ارزښت;
        إذا (varIsParam) أرجع Capture_ډېرپېښ::CAP_ارزښت;
        // Static var in function (will not be inlined, because it's in class)
        إذا (callerIsClass && varIsFuncLocal) أرجع Capture_ډېرپېښ::CAP_ارزښت;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        إذا (m_staticContext && callerIsClass && var_Is_ډګر_د_Caller
            && varRefp->varp()->lifetime().isStatic())
            أرجع Capture_ډېرپېښ::CAP_NO;
        إذا (callerIsClass && var_Is_ډګر_د_Caller) أرجع Capture_ډېرپېښ::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        إذا (callerIsClass && varClassp && !var_Is_ډګر_د_Caller
            && varRefp->varp()->lifetime().isStatic())
            أرجع Capture_ډېرپېښ::CAP_ارزښت;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        أرجع Capture_ډېرپېښ::CAP_ارزښت;
    }

    فراغ capture_Ref_By_ارزښت(Ast_غوټه_Var_Ref* nodep, Capture_ډېرپېښ cap_ډېرپېښ_Flags) {
        AstVar* newVarp;
        ثابت منطقي newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_غوټه_Var_Ref* ثابت newVarRefp = newCapture ? nodep->clone_ونه(خطأ) : تش_اشاره;
        إذا (!hasFlags(cap_ډېرپېښ_Flags, Capture_ډېرپېښ::CAP_F_سېټ_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(تش_اشاره);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        إذا (!newCapture) أرجع;
        إذا (hasFlags(cap_ډېرپېښ_Flags, Capture_ډېرپېښ::CAP_F_XREF)) {
            AstVarRef* ثابت notXVarRefp
                = جديد AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_غوټه::جمع_کړه_بل(m_argsp, جديد AstArg{nodep->fileline(), "", newVarRefp});
    }

    فراغ captureRefByThis(Ast_غوټه_Var_Ref* nodep, Capture_ډېرپېښ /*capModeFlags*/) {
        AstVar* ثابت thisp = importThisp(nodep->fileline());
        AstVarRef* ثابت thisRefp = جديد AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(ریښتیا);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ثابت memberSelp
            = جديد AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        إذا (!m_targetp && nodep->user1()) memberSelp->user1(ریښتیا);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        إذا (m_ignore.شمېر(nodep)) أرجع;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        إذا (nodep->varp()->isStdRandomizeArg()) أرجع;
        ثابت Capture_ډېرپېښ cap_ډېرپېښ = get_Var_Ref_Capture_ډېرپېښ(nodep);
        إذا (ډېرپېښ(cap_ډېرپېښ) == Capture_ډېرپېښ::CAP_NO) أرجع;
        إذا (ډېرپېښ(cap_ډېرپېښ) == Capture_ډېرپېښ::CAP_ارزښت) capture_Ref_By_ارزښت(nodep, cap_ډېرپېښ);
        إذا (ډېرپېښ(cap_ډېرپېښ) == Capture_ډېرپېښ::CAP_THIS) captureRefByThis(nodep, cap_ډېرپېښ);
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        إذا (m_ignore.شمېر(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        إذا (VN_IS(nodep, لاره_Call) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            أرجع;
        }
        AstClass* ثابت classp = VN_CAST(nodep->taskp()->user2p(), Class);
        إذا ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ثابت argsp = nodep->argsp();
            إذا (argsp) argsp->unlink_Fr_شا();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ثابت thisp = importThisp(nodep->fileline());
            AstVarRef* ثابت thisRefp = جديد AstVarRef{
                nodep->fileline(), thisp, nodep->is_خالص() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            Ast_لاره_Call* ثابت لاره_Callp
                = جديد Ast_لاره_Call{nodep->fileline(), thisRefp, thisp->name(), argsp};
            لاره_Callp->taskp(nodep->taskp());
            لاره_Callp->dtypep(nodep->dtypep());
            nodep->replaceWith(لاره_Callp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            m_ignore.emplace(لاره_Callp);
        }
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        إذا (!is_Reference_منزل_Inner_Member(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        AstVarRef* ثابت varRefp
            = جديد AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    فراغ لیدنه(Ast_لاره_Call* nodep) override {
        إذا (!is_Reference_منزل_Inner_Member(nodep) || m_ignore.شمېر(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        AstArg* ثابت argsp = nodep->argsp();
        إذا (argsp) argsp->unlink_Fr_شا_With_بل();
        Ast_غوټه_FTask_Ref* taskRefp = تش_اشاره;
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
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    څرګند CaptureVisitor(Ast_غوټه* ثابت nodep, Ast_غوټه_Module* callerp, AstClass* ثابت targetp,
                            منطقي staticContext = خطأ)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterate_And_بل_Null(nodep);
    }

    // PUBLIC METHODS

    AstArg* get_دلیلونه() ثابت { أرجع m_argsp; }
    AstVar* getThisp() ثابت { أرجع m_thisp; }

    فراغ جمع_کړه_Function_Arguments(Ast_غوټه_FTask* funcp) ثابت {
        لكل (AstArg* argp = get_دلیلونه(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            إذا (Ast_غوټه_Var_Ref* varrefp = VN_CAST(argp->exprp(), غوټه_Var_Ref)) {
                إذا ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(تش_اشاره);
                }
                funcp->جمع_کړه_Stmtsp(getVar(varrefp->varp()));
            } وإلا {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->جمع_کړه_Stmtsp(m_thisp);
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
    ثابت VNUser3_دننه_Use m_inuser3;
    ثابت VNUser4_دننه_Use m_inuser4;

    // STATE
    V3_Unique_نومونه m_inline_Unique_نومونه;  // For generating unique function names
    V3_Unique_نومونه m_rand_Array_Unique_نومونه{"__Vrandarr"};  // Random array loop indices
    V3_Unique_نومونه m_ډېرپېښ_Unique_نومونه{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3_Unique_نومونه m_inlineUniqueStdName{"__VStdrand"};
    V3_Unique_نومونه m_unique_نومونه;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_غوټه_Module* m_modp = تش_اشاره;  // Current module
    std::unordered_map<Ast_غوټه_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    Ast_غوټه_Stmt* m_stmtp = تش_اشاره;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = تش_اشاره;  // Dynamic array type (for rand mode)
    صحيح m_rand_Case_عدد = 0;  // Randcase number within a module for var naming
    صحيح m_dist_عدد = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = تش_اشاره;  // Current constraint
    std::سېټ<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertion_سېټ<AstVar*>> m_اندازه_Constrained_Arrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_static_Constraint_ډېرپېښ_Vars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_static_Rand_ډېرپېښ_Vars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<منطقي, منطقي>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ساكن منطقي is_Similar_غوټه(ثابت Ast_غوټه_بیان* with_بیان, ثابت Ast_غوټه_بیان* arg_بیان) {
        // VarRef: compare variable pointers
        إذا (VN_IS(arg_بیان, VarRef) && VN_IS(with_بیان, VarRef)) {
            أرجع VN_AS(with_بیان, VarRef)->varp() == VN_AS(arg_بیان, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        إذا (VN_IS(arg_بیان, MemberSel) && VN_IS(with_بیان, MemberSel)) {
            ثابت AstMemberSel* ثابت withMSp = VN_AS(with_بیان, MemberSel);
            ثابت AstMemberSel* ثابت argMSp = VN_AS(arg_بیان, MemberSel);
            إذا (withMSp->varp() != argMSp->varp()) أرجع خطأ;
            // Recursively compare the base object expression
            أرجع is_Similar_غوټه(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        إذا (VN_IS(arg_بیان, ArraySel) && VN_IS(with_بیان, ArraySel)) {
            ثابت AstArraySel* ثابت withASp = VN_AS(with_بیان, ArraySel);
            ثابت AstArraySel* ثابت argASp = VN_AS(arg_بیان, ArraySel);
            // Before checking Sel type index, check if both are Const
            إذا (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ثابت AstConst* ثابت withIdxp = VN_AS(withASp->bitp(), Const);
                ثابت AstConst* ثابت argIdxp = VN_AS(argASp->bitp(), Const);
                أرجع is_Similar_غوټه(withASp->fromp(), argASp->fromp())
                       && withIdxp->عدد().isCaseEq(argIdxp->عدد());
            }
            // Index must be Sel type, extract VarRef using fromp()
            إذا (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) أرجع خطأ;
            ثابت Ast_غوټه_بیان* ثابت withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ثابت Ast_غوټه_بیان* ثابت argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            أرجع is_Similar_غوټه(withASp->fromp(), argASp->fromp())
                   && is_Similar_غوټه(withIdxp, argIdxp);
        }
        أرجع خطأ;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ساكن فراغ expandUniqueElementList(Ast_غوټه* itemsp) {
        Ast_غوټه* itemp = itemsp;
        طالما (itemp) {
            Ast_غوټه* ثابت nextp = itemp->nextp();
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

            std::ویکټور<Ast_غوټه_بیان*> بیان_توکي;
            منطقي hasArrayVarRef = خطأ;
            لكل (Ast_غوټه* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                إذا (ثابت AstVarRef* ثابت vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ثابت Ast_غوټه_DType* ثابت dtypep = vrp->varp()->dtypep()->skipRefp();
                    إذا (VN_IS(dtypep, غوټه_Array_DType) || VN_IS(dtypep, کتار_DType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = ریښتیا;
                        استمر;
                    }
                }
                بیان_توکي.ټېل_شا(VN_AS(rp, غوټه_بیان));
            }

            إذا (بیان_توکي.اندازه() >= 2) {
                File_کرښه* ثابت fl = uniquep->fileline();
                لكل (اندازه_t i = 0; i < بیان_توکي.اندازه(); i++) {
                    لكل (اندازه_t j = i + 1; j < بیان_توکي.اندازه(); j++) {
                        Ast_غوټه_بیان* ثابت lhsp = بیان_توکي[i]->clone_ونه(خطأ);
                        Ast_غوټه_بیان* ثابت rhsp = بیان_توکي[j]->clone_ونه(خطأ);
                        AstNeq* ثابت neqp = جديد AstNeq{fl, lhsp, rhsp};
                        neqp->user1(ریښتیا);
                        Ast_Constraint_بیان* ثابت cexprp = جديد Ast_Constraint_بیان{fl, neqp};
                        uniquep->جمع_کړه_بل_دلته(cexprp);
                    }
                }
                إذا (!hasArrayVarRef) {
                    uniquep->unlink_Fr_شا();
                    VL_DO_DANGLING(uniquep->delete_ونه(), uniquep);
                }
            } وإلا إذا (بیان_توکي.اندازه() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlink_Fr_شا();
                VL_DO_DANGLING(uniquep->delete_ونه(), uniquep);
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
                                        classp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->جمع_کړه_Membersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_غوټه_Module* ثابت modp) {
        خپلکاری it = m_stdMap.پیدا_کړه(modp);
        إذا (it == m_stdMap.پای_ټکی()) {
            AstVar* ثابت stdgenp
                = جديد AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3_Error_کوډ::IMPURE, ریښتیا);
            modp->جمع_کړه_Stmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            أرجع stdgenp;
        }
        أرجع it->دوهم;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ثابت classp) {
        ساكن ثابت حرف* ثابت name = "__Vprepare_constrained_arrays";
        AstTask* ثابت taskp = جديد AstTask{classp->fileline(), name, تش_اشاره};
        taskp->class_لاره(ریښتیا);
        classp->جمع_کړه_Membersp(taskp);
        m_memberMap.ورننباسه(classp, taskp);
        أرجع taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ثابت classp) {
        ساكن constexpr ثابت حرف* ثابت name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.پیدا_کړه_Member(classp, name), Task);
        إذا (resizeTaskp) أرجع resizeTaskp;
        resizeTaskp = جديد AstTask{classp->fileline(), name, تش_اشاره};
        resizeTaskp->class_لاره(ریښتیا);
        resizeTaskp->isVirtual(ریښتیا);
        classp->جمع_کړه_Membersp(resizeTaskp);
        m_memberMap.ورننباسه(classp, resizeTaskp);
        أرجع resizeTaskp;
    }
    AstVar* get_Create_Rand_ډېرپېښ_Var(AstClass* ثابت classp) {
        إذا (classp->user2p()) أرجع VN_AS(classp->user2p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Rand_ډېرپېښ_Var(extendsp->classp());
        }
        AstVar* ثابت rand_ډېرپېښ_Varp = create_ډېرپېښ_Var(classp, "__Vrandmode");
        classp->user2p(rand_ډېرپېښ_Varp);
        أرجع rand_ډېرپېښ_Varp;
    }
    AstVar* get_Create_Constraint_ډېرپېښ_Var(AstClass* ثابت classp) {
        إذا (classp->user4p()) أرجع VN_AS(classp->user4p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Constraint_ډېرپېښ_Var(extendsp->classp());
        }
        AstVar* ثابت constraint_ډېرپېښ_Varp = create_ډېرپېښ_Var(classp, "__Vconstraintmode");
        classp->user4p(constraint_ډېرپېښ_Varp);
        أرجع constraint_ډېرپېښ_Varp;
    }
    ساكن AstVar* get_Constraint_ډېرپېښ_Var(AstClass* ثابت classp) {
        إذا (classp->user4p()) أرجع VN_AS(classp->user4p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Constraint_ډېرپېښ_Var(extendsp->classp());
        }
        أرجع تش_اشاره;
    }
    AstVar* get_Create_Static_Constraint_ډېرپېښ_Var(AstClass* ثابت classp) {
        خپلکاری it = m_static_Constraint_ډېرپېښ_Vars.پیدا_کړه(classp);
        إذا (it != m_static_Constraint_ډېرپېښ_Vars.پای_ټکی()) أرجع it->دوهم;
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Static_Constraint_ډېرپېښ_Var(extendsp->classp());
        }
        AstVar* ثابت static_ډېرپېښ_Varp = create_Static_ډېرپېښ_Var(classp, "__Vstaticconstraintmode");
        m_static_Constraint_ډېرپېښ_Vars[classp] = static_ډېرپېښ_Varp;
        أرجع static_ډېرپېښ_Varp;
    }
    AstVar* get_Static_Constraint_ډېرپېښ_Var(AstClass* ثابت classp) {
        خپلکاری it = m_static_Constraint_ډېرپېښ_Vars.پیدا_کړه(classp);
        إذا (it != m_static_Constraint_ډېرپېښ_Vars.پای_ټکی()) أرجع it->دوهم;
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Static_Constraint_ډېرپېښ_Var(extendsp->classp());
        }
        أرجع تش_اشاره;
    }
    AstVar* get_Create_Static_Rand_ډېرپېښ_Var(AstClass* ثابت classp) {
        إذا (m_static_Rand_ډېرپېښ_Vars.شمېر(classp)) أرجع m_static_Rand_ډېرپېښ_Vars[classp];
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Static_Rand_ډېرپېښ_Var(extendsp->classp());
        }
        AstVar* ثابت static_ډېرپېښ_Varp = create_Static_ډېرپېښ_Var(classp, "__Vstaticrandmode");
        m_static_Rand_ډېرپېښ_Vars[classp] = static_ډېرپېښ_Varp;
        أرجع static_ډېرپېښ_Varp;
    }
    AstVar* get_Static_Rand_ډېرپېښ_Var(AstClass* ثابت classp) {
        إذا (m_static_Rand_ډېرپېښ_Vars.شمېر(classp)) أرجع m_static_Rand_ډېرپېښ_Vars[classp];
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Static_Rand_ډېرپېښ_Var(extendsp->classp());
        }
        أرجع تش_اشاره;
    }
    AstVar* create_ډېرپېښ_Var(AstClass* ثابت classp, ثابت حرف* ثابت name) {
        File_کرښه* ثابت fl = classp->fileline();
        إذا (!m_dynarrayDtp) {
            m_dynarrayDtp = جديد AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->پیدا_کړه_بټ_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ثابت ډېرپېښ_Varp = جديد AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        ډېرپېښ_Varp->user2p(classp);
        classp->جمع_کړه_Stmtsp(ډېرپېښ_Varp);
        أرجع ډېرپېښ_Varp;
    }
    AstVar* create_Static_ډېرپېښ_Var(AstClass* ثابت classp, ثابت حرف* ثابت name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_کرښه* ثابت fl = classp->fileline();
        إذا (!m_dynarrayDtp) {
            m_dynarrayDtp = جديد AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->پیدا_کړه_بټ_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(m_dynarrayDtp);
        }
        AstVar* ثابت ډېرپېښ_Varp = جديد AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        ډېرپېښ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        ډېرپېښ_Varp->user2p(classp);
        classp->جمع_کړه_Stmtsp(ډېرپېښ_Varp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.ورننباسه(classp, ډېرپېښ_Varp);
        أرجع ډېرپېښ_Varp;
    }
    ساكن فراغ جمع_کړه_سېټ_Rand_ډېرپېښ(Ast_غوټه_FTask* ثابت ftaskp, AstVar* ثابت genp,
                               AstVar* ثابت rand_ډېرپېښ_Varp) {
        File_کرښه* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت سېټ_Rand_Modep = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, VN_AS(genp->user2p(), غوټه_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_سېټ_RANDMODE,
            جديد AstVarRef{fl, VN_AS(rand_ډېرپېښ_Varp->user2p(), غوټه_Module), rand_ډېرپېښ_Varp,
                          VAccess::READ}};
        سېټ_Rand_Modep->dtype_سېټ_Void();
        ftaskp->جمع_کړه_Stmtsp(سېټ_Rand_Modep->makeStmt());
    }
    ساكن فراغ جمع_کړه_سېټ_Static_Rand_ډېرپېښ(Ast_غوټه_FTask* ثابت ftaskp, AstVar* ثابت genp,
                                     AstVar* ثابت static_Rand_ډېرپېښ_Varp) {
        File_کرښه* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت setp = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, VN_AS(genp->user2p(), غوټه_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_سېټ_STATIC_RANDMODE,
            جديد AstVarRef{fl, VN_AS(static_Rand_ډېرپېښ_Varp->user2p(), غوټه_Module), static_Rand_ډېرپېښ_Varp,
                          VAccess::READ}};
        setp->dtype_سېټ_Void();
        ftaskp->جمع_کړه_Stmtsp(setp->makeStmt());
    }
    فراغ createRandomizeClassVars(AstNetlist* ثابت netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> ریښه_Static_Rand_ډېرپېښ_شمېر;
        netlistp->foreach([هذا, &ریښه_Static_Rand_ډېرپېښ_شمېر](AstClass* ثابت classp) {
            منطقي hasConstraints = خطأ;
            uint32_t rand_ډېرپېښ_شمېر = 0;
            uint32_t static_Rand_ډېرپېښ_شمېر = 0;
            uint32_t constraint_ډېرپېښ_شمېر = 0;
            uint32_t static_Constraint_ډېرپېښ_شمېر = 0;
            classp->foreachMember([&](AstClass*, Ast_غوټه* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                إذا (AstConstraint* ثابت constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = ریښتیا;
                    Randomize_ډېرپېښ constraint_ډېرپېښ = {.asUQuad = memberp->user1()};
                    إذا (!constraint_ډېرپېښ.کاروي_ډېرپېښ) أرجع;
                    إذا (constraint_ډېرپېښ.شاخص == 0) {
                        // Use separate index counters for static vs non-static constraints
                        إذا (constrp->isStatic()) {
                            constraint_ډېرپېښ.شاخص = static_Constraint_ډېرپېښ_شمېر++;
                        } وإلا {
                            constraint_ډېرپېښ.شاخص = constraint_ډېرپېښ_شمېر++;
                        }
                        memberp->user1(constraint_ډېرپېښ.asUQuad);
                    } وإلا {
                        إذا (constrp->isStatic()) {
                            static_Constraint_ډېرپېښ_شمېر = constraint_ډېرپېښ.شاخص + 1;
                        } وإلا {
                            constraint_ډېرپېښ_شمېر = constraint_ډېرپېښ.شاخص + 1;
                        }
                    }
                } وإلا إذا (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
                    Randomize_ډېرپېښ rand_ډېرپېښ = {.asUQuad = memberp->user1()};
                    إذا (!rand_ډېرپېښ.کاروي_ډېرپېښ) أرجع;
                    ثابت منطقي isStaticVar = varp->lifetime().isStatic();
                    إذا (rand_ډېرپېښ.شاخص == 0) {
                        إذا (isStaticVar) {
                            rand_ډېرپېښ.شاخص = static_Rand_ډېرپېښ_شمېر++;
                        } وإلا {
                            rand_ډېرپېښ.شاخص = rand_ډېرپېښ_شمېر++;
                        }
                        memberp->user1(rand_ډېرپېښ.asUQuad);
                    } وإلا {
                        إذا (isStaticVar) {
                            static_Rand_ډېرپېښ_شمېر = rand_ډېرپېښ.شاخص + 1;
                        } وإلا {
                            rand_ډېرپېښ_شمېر = rand_ډېرپېښ.شاخص + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<فراغ(AstClass*)> پیدا_کړه_Sub_Obj_Rand_Modes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_غوټه* subMemberp) {
                        إذا (AstVar* ثابت subVarp = VN_CAST(subMemberp, Var)) {
                            ثابت Randomize_ډېرپېښ rm = {.asUQuad = subVarp->user1()};
                            إذا (!rm.کاروي_ډېرپېښ) أرجع;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            إذا (subVarp->lifetime().isStatic()) أرجع;
                            ثابت uint32_t needed = rm.شاخص + 1;
                            إذا (needed > rand_ډېرپېښ_شمېر) rand_ډېرپېښ_شمېر = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    إذا (!memberVarp->globalConstrained()) أرجع;
                    ثابت Ast_غوټه_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                    إذا (!classRefp) أرجع;
                    پیدا_کړه_Sub_Obj_Rand_Modes(classRefp->classp());
                });
            }
            إذا (hasConstraints) createRandomGenerator(classp);
            إذا (rand_ډېرپېښ_شمېر > 0) {
                AstVar* ثابت rand_ډېرپېښ_Varp = get_Create_Rand_ډېرپېښ_Var(classp);
                make_ډېرپېښ_Init(rand_ډېرپېښ_Varp, classp, rand_ډېرپېښ_شمېر);
            }
            إذا (constraint_ډېرپېښ_شمېر > 0) {
                AstVar* ثابت constraint_ډېرپېښ_Varp = get_Create_Constraint_ډېرپېښ_Var(classp);
                make_ډېرپېښ_Init(constraint_ډېرپېښ_Varp, classp, constraint_ډېرپېښ_شمېر);
            }
            إذا (static_Constraint_ډېرپېښ_شمېر > 0) {
                AstVar* ثابت static_Constraint_ډېرپېښ_Varp = get_Create_Static_Constraint_ډېرپېښ_Var(classp);
                make_Static_ډېرپېښ_Init(static_Constraint_ډېرپېښ_Varp, classp, static_Constraint_ډېرپېښ_شمېر);
            }
            إذا (static_Rand_ډېرپېښ_شمېر > 0) {
                get_Create_Static_Rand_ډېرپېښ_Var(classp);
                AstClass* rootp = classp;
                طالما (AstClassExtends* ثابت ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& خونه_2 = ریښه_Static_Rand_ډېرپېښ_شمېر[rootp];
                إذا (static_Rand_ډېرپېښ_شمېر > خونه_2) خونه_2 = static_Rand_ډېرپېښ_شمېر;
            }
        });
        لكل (ثابت خپلکاری& kv : ریښه_Static_Rand_ډېرپېښ_شمېر) emit_ریښه_Static_ډېرپېښ_Init(kv.لومړی, kv.دوهم);
    }
    فراغ emit_ریښه_Static_ډېرپېښ_Init(AstClass* ثابت rootp, ثابت uint32_t شمېر) {
        AstVar* ثابت static_Rand_ډېرپېښ_Varp = m_static_Rand_ډېرپېښ_Vars[rootp];
        UASSERT_OBJ(static_Rand_ډېرپېښ_Varp, rootp, "Root must have a static rand-mode var");
        make_Static_ډېرپېښ_Init(static_Rand_ډېرپېښ_Varp, rootp, شمېر);
    }
    فراغ make_ډېرپېښ_Init(AstVar* ډېرپېښ_Varp, AstClass* classp, uint32_t ډېرپېښ_شمېر) {
        Ast_غوټه_Module* ثابت ډېرپېښ_Var_Modp = VN_AS(ډېرپېښ_Varp->user2p(), غوټه_Module);
        File_کرښه* ثابت fl = ډېرپېښ_Varp->fileline();
        AstVar* ثابت old_اندازه_Varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, m_unique_نومونه.get("__VoldSize"), ډېرپېښ_Varp->پیدا_کړه_UInt32_DType()};
        old_اندازه_Varp->funcLocal(ریښتیا);
        old_اندازه_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت oldSizep
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, ډېرپېښ_Var_Modp, ډېرپېښ_Varp, VAccess::READ},
                                 VCMethod::DYN_اندازه, تش_اشاره};
        oldSizep->dtype_سېټ_UInt32();
        AstCMethodHard* ثابت dynarrayNewp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, ډېرپېښ_Var_Modp, ډېرپېښ_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جديد AstConst{fl, ډېرپېښ_شمېر}};
        dynarrayNewp->dtype_سېټ_Void();
        Ast_غوټه_FTask* ثابت ctorNewp = VN_AS(m_memberMap.پیدا_کړه_Member(classp, "new"), غوټه_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_غوټه* ثابت initFirstp = old_اندازه_Varp;
        initFirstp->جمع_کړه_بل(
            جديد Ast_ټاکل{fl, جديد AstVarRef{fl, old_اندازه_Varp, VAccess::WRITE}, oldSizep});
        initFirstp->جمع_کړه_بل(dynarrayNewp->makeStmt());
        initFirstp->جمع_کړه_بل(make_ډېرپېښ_سېټ_Loop(
            fl, جديد AstVarRef{fl, ډېرپېښ_Var_Modp, ډېرپېښ_Varp, VAccess::WRITE}, جديد AstConst{fl, 1},
            ریښتیا, جديد AstVarRef{fl, old_اندازه_Varp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        لكل (Ast_غوټه* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (!VN_IS(stmtp, غوټه_Stmt)) استمر;  // Skip var declarations
            إذا (ثابت Ast_Stmt_بیان* ثابت sep = VN_CAST(stmtp, Stmt_بیان)) {
                إذا (VN_IS(sep->exprp(), New)) استمر;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->جمع_کړه_دلته_This_As_بل(initFirstp);
            أرجع;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->جمع_کړه_Stmtsp(initFirstp);
    }
    فراغ make_Static_ډېرپېښ_Init(AstVar* ډېرپېښ_Varp, AstClass* classp, uint32_t ډېرپېښ_شمېر) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_غوټه_Module* ثابت ډېرپېښ_Var_Modp = VN_AS(ډېرپېښ_Varp->user2p(), غوټه_Module);
        File_کرښه* ثابت fl = ډېرپېښ_Varp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ثابت sizep
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, ډېرپېښ_Var_Modp, ډېرپېښ_Varp, VAccess::READ},
                                 VCMethod::DYN_اندازه, تش_اشاره};
        sizep->dtype_سېټ_UInt32();
        AstEq* ثابت condp = جديد AstEq{fl, sizep, جديد AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ثابت dynarrayNewp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, ډېرپېښ_Var_Modp, ډېرپېښ_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جديد AstConst{fl, ډېرپېښ_شمېر}};
        dynarrayNewp->dtype_سېټ_Void();
        Ast_غوټه* ثابت thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->جمع_کړه_بل(
            make_ډېرپېښ_سېټ_Loop(fl, جديد AstVarRef{fl, ډېرپېښ_Var_Modp, ډېرپېښ_Varp, VAccess::WRITE},
                            جديد AstConst{fl, 1}, ریښتیا));

        // Build the if statement
        AstIf* ثابت ifp = جديد AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_غوټه_FTask* ثابت newp = VN_AS(m_memberMap.پیدا_کړه_Member(classp, "new"), غوټه_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->جمع_کړه_Stmtsp(ifp);
    }
    Ast_غوټه* make_ډېرپېښ_سېټ_Loop(File_کرښه* ثابت fl, Ast_غوټه_بیان* ثابت lhsp, Ast_غوټه_بیان* ثابت rhsp,
                             منطقي دننه_Task, Ast_غوټه_بیان* ثابت startp = تش_اشاره) {
        AstVar* ثابت iterVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->پیدا_کړه_UInt32_DType()};
        iterVarp->funcLocal(دننه_Task);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت sizep = جديد AstCMethodHard{fl, lhsp, VCMethod::DYN_اندازه, تش_اشاره};
        sizep->dtype_سېټ_UInt32();
        AstCMethodHard* ثابت setp
            = جديد AstCMethodHard{fl, lhsp->clone_ونه(خطأ), VCMethod::ARRAY_AT_WRITE,
                                 جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtype_سېټ_UInt32();
        Ast_غوټه* ثابت stmtsp = iterVarp;
        stmtsp->جمع_کړه_بل(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : جديد AstConst{fl, 0}});

        AstLoop* ثابت loopp = جديد AstLoop{fl};
        stmtsp->جمع_کړه_بل(loopp);
        loopp->جمع_کړه_Stmtsp(جديد AstLoopTest{
            fl, loopp, جديد AstLt{fl, جديد AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, setp, rhsp});
        loopp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{
            fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
            جديد Ast_جمع_کړه{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, iterVarp, VAccess::READ}}});
        أرجع جديد AstBegin{fl, m_unique_نومونه.get("__VrandBlock"), stmtsp, ریښتیا};
    }
    فراغ جمع_کړه_لنډمهاله_Varsp(AstVar* varsp) {
        لكل (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (m_ftaskp) {
                varp->funcLocal(ریښتیا);
            } وإلا {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        إذا (!m_ftaskp) {
            m_modp->جمع_کړه_Stmtsp(varsp);
        } وإلا {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_غوټه_Stmt* wrap_If_Rand_ډېرپېښ(AstClass* classp, AstVar* ثابت varp, Ast_غوټه_Stmt* stmtp) {
        ثابت Randomize_ډېرپېښ rmode = {.asUQuad = varp->user1()};
        AstVar* ثابت ډېرپېښ_Varp = varp->lifetime().isStatic() ? get_Static_Rand_ډېرپېښ_Var(classp)
                                                             : get_Rand_ډېرپېښ_Var_From_Class(classp);
        أرجع VN_AS(wrap_If_ډېرپېښ(rmode, ډېرپېښ_Varp, stmtp), غوټه_Stmt);
    }
    Ast_غوټه* wrap_If_Constraint_ډېرپېښ(AstClass* classp, AstConstraint* ثابت constrp, Ast_غوټه* stmtp) {
        ثابت Randomize_ډېرپېښ rmode = {.asUQuad = constrp->user1()};
        AstVar* ثابت ډېرپېښ_Varp = constrp->isStatic() ? get_Static_Constraint_ډېرپېښ_Var(classp)
                                                     : get_Constraint_ډېرپېښ_Var(classp);
        أرجع wrap_If_ډېرپېښ(rmode, ډېرپېښ_Varp, stmtp);
    }
    ساكن Ast_غوټه* wrap_If_ډېرپېښ(ثابت Randomize_ډېرپېښ ډېرپېښ, AstVar* ډېرپېښ_Varp, Ast_غوټه* stmtp) {
        File_کرښه* ثابت fl = stmtp->fileline();
        إذا (ډېرپېښ.کاروي_ډېرپېښ) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_غوټه_Module* ثابت modp = VN_AS(ډېرپېښ_Varp->user2p(), غوټه_Module);
            AstCMethodHard* ثابت atp
                = جديد AstCMethodHard{fl, جديد AstVarRef{fl, modp, ډېرپېښ_Varp, VAccess::READ},
                                     VCMethod::ARRAY_AT, جديد AstConst{fl, ډېرپېښ.شاخص}};
            atp->dtype_سېټ_UInt32();
            أرجع جديد AstIf{fl, atp, stmtp};
        }
        أرجع stmtp;
    }
    AstVarRef* enum_ارزښت_Tab_Refp(AstEnumDType* ثابت nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), VarRef)->clone_ونه(خطأ);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_غوټه_Array_DType* ثابت vardtypep = جديد AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            جديد AstRange{nodep->fileline(), static_cast<صحيح>(nodep->توکی_شمېر()) - 1, 0}};
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(vardtypep);
        AstConsArray* ثابت consp = جديد AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t شاخص = 0;
        لكل (Ast_Enum_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_توکی)) {
            AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(شاخص++, vconstp->clone_ونه(خطأ));
        }
        // Share identical tables via the constant pool
        AstVarRef* ثابت refp = V3ConstPool::پیدا_کړه(consp);
        VL_DO_DANGLING(consp->delete_ونه(), consp);  // V3ConstPool::find clones it
        ټېل_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        أرجع refp->clone_ونه(خطأ);
    }

    AstCDType* پیدا_کړه_Vl_Rand_CDType(File_کرښه* ثابت fl, uint64_t توکي) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ثابت std::string type = AstCDType::type_منزل_Hold(V3_شمېره::log2bQuad(توکي) + 1);
        ثابت std::string name = "VlRandC<" + type + ", " + cvt_منزل_Str(توکي) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ثابت خپلکاری pair = m_randcDtypes.emplace(name, تش_اشاره);
        إذا (pair.دوهم) {
            AstCDType* ثابت newp = جديد AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newp);
            pair.لومړی->دوهم = newp;
        }
        أرجع pair.لومړی->دوهم;
    }

    AstVar* newRandcVarsp(AstVar* ثابت varp, AstClass* ثابت classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        إذا (!varp->isRandC()) أرجع تش_اشاره;

        uint64_t توکي = 0;
        إذا (AstEnumDType* ثابت enumDtp = VN_CAST(varp->dtypep()->skip_Ref_منزل_Enump(), EnumDType)) {
            توکي = static_cast<uint64_t>(enumDtp->توکی_شمېر());
        } وإلا إذا (AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp()) {
            إذا (basicp->پلنوالی() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                إذا (!V3Error::error_شمېر()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->پلنوالی() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                أرجع تش_اشاره;
            }
            توکي = 1ULL << basicp->پلنوالی();
        } وإلا إذا (AstStructDType* ثابت dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            أرجع تش_اشاره;
        } وإلا {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_شمېر(), varp, "Unexpected randc variable dtype");
            أرجع تش_اشاره;
        }
        AstCDType* ثابت newdtp = پیدا_کړه_Vl_Rand_CDType(varp->fileline(), توکي);
        AstVar* ثابت newp
            = جديد AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->is_دننی(ریښتیا);
        classp->جمع_کړه_Stmtsp(newp);
        UINFO(9, "created " << varp);
        أرجع newp;
    }
    Ast_غوټه_Stmt* createArrayForeachLoop(File_کرښه* ثابت fl, Ast_غوټه_DType* ثابت dtypep,
                                        Ast_غوټه_بیان* exprp, AstVar* ثابت outputVarp) {
        Ast_غوټه_DType* لنډمهاله_DTypep = dtypep;
        AstVar* randLoopIndxp = تش_اشاره;
        خپلکاری create_Loop_شاخص = [&](Ast_غوټه_DType* لنډمهاله_DTypep) {
            إذا (VN_IS(لنډمهاله_DTypep, AssocArrayDType)) {
                أرجع جديد AstVar{fl, VVarType::VAR, m_rand_Array_Unique_نومونه.get(""),
                                  VN_AS(لنډمهاله_DTypep, AssocArrayDType)->کلی_DTypep()};
            }
            أرجع جديد AstVar{fl, VVarType::VAR, m_rand_Array_Unique_نومونه.get(""),
                              dtypep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32)};
        };
        Ast_غوټه_بیان* لنډمهاله_Elementp = تش_اشاره;
        طالما (لنډمهاله_DTypep->isNonPackedArray()) {
            AstVar* ثابت newRandLoopIndxp = create_Loop_شاخص(لنډمهاله_DTypep);
            randLoopIndxp = Ast_غوټه::جمع_کړه_بل(randLoopIndxp, newRandLoopIndxp);
            Ast_غوټه_بیان* ثابت لنډمهاله_Exprp = لنډمهاله_Elementp ? لنډمهاله_Elementp : exprp;
            AstVarRef* ثابت لنډمهاله_Refp = جديد AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            إذا (VN_IS(لنډمهاله_DTypep, DynArrayDType)) {
                لنډمهاله_Elementp
                    = جديد AstCMethodHard{fl, لنډمهاله_Exprp, VCMethod::ARRAY_AT_WRITE, لنډمهاله_Refp};
            } وإلا إذا (VN_IS(لنډمهاله_DTypep, UnpackArrayDType)) {
                Ast_غوټه_Array_DType* ثابت aryDTypep = VN_CAST(لنډمهاله_DTypep, غوټه_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                لنډمهاله_Elementp = جديد AstArraySel{
                    fl, لنډمهاله_Exprp,
                    جديد AstSel{
                        fl,
                        جديد AstSub{fl, لنډمهاله_Refp,
                                   جديد AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        جديد AstConst{fl, 0}, V3_شمېره::log2b(aryDTypep->hi()) + 1}};
            } وإلا إذا (VN_IS(لنډمهاله_DTypep, AssocArrayDType)) {
                لنډمهاله_Elementp = جديد AstAssocSel{fl, لنډمهاله_Exprp, لنډمهاله_Refp};
            } وإلا إذا (VN_IS(لنډمهاله_DTypep, کتار_DType)) {
                لنډمهاله_Elementp
                    = جديد AstCMethodHard{fl, لنډمهاله_Exprp, VCMethod::DYN_AT_WRITE_APPEND, لنډمهاله_Refp};
            }
            لنډمهاله_Elementp->dtypep(لنډمهاله_DTypep->subDTypep());
            لنډمهاله_DTypep = لنډمهاله_DTypep->virtRefDTypep();
        }

        Ast_Foreach_سرلیک* ثابت headerp
            = جديد Ast_Foreach_سرلیک{fl, exprp->clone_ونه(خطأ), randLoopIndxp};
        Ast_غوټه_Stmt* ثابت randStmtsp = newRandStmtsp(fl, لنډمهاله_Elementp, تش_اشاره, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        إذا (!لنډمهاله_Elementp->backp()) VL_DO_DANGLING(ټېل_Deletep(لنډمهاله_Elementp), لنډمهاله_Elementp);
        أرجع جديد AstForeach{fl, headerp, randStmtsp};
    }
    Ast_غوټه_Stmt* newRandStmtsp(File_کرښه* fl, Ast_غوټه_بیان* exprp, AstVar* randcVarp,
                               AstVar* ثابت outputVarp, صحيح دننه_واټن = 0,
                               AstMemberDType* memberp = تش_اشاره) {
        Ast_غوټه_DType* ثابت memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        إذا (ثابت خپلکاری* ثابت structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_غوټه_Stmt* stmtsp = تش_اشاره;
            إذا (structDtp->packed()) دننه_واټن += memberp ? memberp->lsb() : 0;
            لكل (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_غوټه_Stmt* randp = تش_اشاره;
                إذا (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_ونه(خطأ) : exprp, تش_اشاره,
                                          outputVarp, دننه_واټن, smemberp);
                } وإلا إذا (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = جديد AstStructSel{fl, exprp->clone_ونه(خطأ), smemberp->name()};
                    structSelp->dtypep(smemberp->ماشوم_DTypep());
                    إذا (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, تش_اشاره, outputVarp);
                }
                stmtsp = Ast_غوټه::جمع_کړه_بل_Null(stmtsp, randp);
            }
            أرجع stmtsp;
        } وإلا إذا (ثابت خپلکاری* ثابت unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            إذا (!unionDtp->packed()) أرجع تش_اشاره;
            AstMemberDType* ثابت لومړی_Memberp = unionDtp->membersp();
            أرجع newRandStmtsp(fl, exprp, تش_اشاره, outputVarp, دننه_واټن, لومړی_Memberp);
        } وإلا إذا (ثابت AstClassRefDType* ثابت classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ثابت memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            Ast_لاره_Call* ثابت callp = جديد Ast_لاره_Call{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            Ast_ټاکل* ثابت assignp = جديد Ast_ټاکل{
                fl, جديد AstVarRef{fl, outputVarp, VAccess::WRITE},
                جديد AstAnd{fl, جديد AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            أرجع جديد AstIf{
                fl, جديد AstNeq{fl, exprp->clone_ونه(خطأ), جديد AstConst{fl, AstConst::Null{}}},
                assignp};
        } وإلا إذا (AstDynArrayDType* ثابت dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            أرجع createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } وإلا إذا (Ast_کتار_DType* ثابت کتار_Dtp = VN_CAST(memberDtp, کتار_DType)) {
            أرجع createArrayForeachLoop(fl, کتار_Dtp, exprp, outputVarp);
        } وإلا إذا (AstUnpackArrayDType* ثابت unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            أرجع createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } وإلا إذا (AstAssocArrayDType* ثابت assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            أرجع createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } وإلا {
            Ast_غوټه_بیان* valp;
            إذا (AstEnumDType* ثابت enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ثابت tabRefp = enum_ارزښت_Tab_Refp(enumDtp);
                Ast_غوټه_بیان* ثابت randp
                    = new_Rand_ارزښت(fl, randcVarp, exprp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32));
                Ast_غوټه_بیان* ثابت moddivp = جديد AstModDiv{
                    fl, randp, جديد AstConst{fl, static_cast<uint32_t>(enumDtp->توکی_شمېر())}};
                moddivp->dtypep(enumDtp);
                valp = جديد AstArraySel{fl, tabRefp, moddivp};
            } وإلا {
                valp
                    = new_Rand_ارزښت(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            Ast_ټاکل* assignp
                = جديد Ast_ټاکل{fl,
                                جديد AstSel{fl, exprp, دننه_واټن + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->پلنوالی() : exprp->پلنوالی()},
                                valp};
            AstVar* varp = تش_اشاره;
            exprp->exists([&](ثابت AstVarRef* varrefp) {
                إذا (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                أرجع varp != تش_اشاره;
            });
            أرجع wrap_If_Rand_ډېرپېښ(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_غوټه_بیان* new_Rand_ارزښت(File_کرښه* ثابت fl, AstVar* ثابت randcVarp,
                              Ast_غوټه_DType* ثابت dtypep) {
        إذا (randcVarp) {
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->جمع_کړه(جديد AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->جمع_کړه(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            أرجع cexprp;
        }

        أرجع جديد AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* پیدا_کړه_Pre_Post_Task(AstClass* classp, ثابت string& name) {
        لكل (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : تش_اشاره) {
            إذا (AstTask* ثابت taskp = VN_CAST(m_memberMap.پیدا_کړه_Member(cp, name), Task)) {
                أرجع taskp;
            }
        }
        أرجع تش_اشاره;
    }
    فراغ جمع_کړه_Pre_Post_Call(AstClass* ثابت classp, Ast_غوټه_FTask* ثابت funcp, ثابت string& name) {
        إذا (AstTask* ثابت userFuncp = پیدا_کړه_Pre_Post_Task(classp, name)) {
            AstTaskRef* ثابت callp = جديد AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->جمع_کړه_Stmtsp(callp->makeStmt());
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
        إذا (AstTask* ثابت existingp = VN_CAST(m_memberMap.پیدا_کړه_Member(classp, name), Task)) {
            أرجع existingp;
        }
        AstTask* ثابت taskp = جديد AstTask{classp->fileline(), name, تش_اشاره};
        taskp->class_لاره(ریښتیا);
        taskp->isVirtual(classp->isExtended());
        classp->جمع_کړه_Membersp(taskp);
        m_memberMap.ورننباسه(classp, taskp);
        جمع_کړه_Pre_Post_Call(classp, taskp, which);
        أرجع taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<منطقي, منطقي> buildPrePostVirtualWrappers(AstClass* ثابت classp) {
        ثابت خپلکاری cachedIt = m_prePostWrap.پیدا_کړه(classp);
        إذا (cachedIt != m_prePostWrap.پای_ټکی()) أرجع cachedIt->دوهم;
        std::ویکټور<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            إذا (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.ټېل_شا(subp);
        });
        منطقي hasPre = خطأ;
        منطقي hasPost = خطأ;
        لكل (AstClass* ثابت cp : hierp) {
            إذا (پیدا_کړه_Pre_Post_Task(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = ریښتیا;
            }
            إذا (پیدا_کړه_Pre_Post_Task(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = ریښتیا;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        إذا (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        إذا (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ثابت std::pair<منطقي, منطقي> پایله{hasPre, hasPost};
        m_prePostWrap.emplace(classp, پایله);
        أرجع پایله;
    }
    فراغ جمع_کړه_Virtual_Pre_Post_Call(AstFunc* ثابت randomizeFuncp, AstClass* ثابت classp,
                               ثابت string& which) {
        File_کرښه* ثابت fl = classp->fileline();
        AstTask* ثابت wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ثابت refDTypep = جديد AstClassRefDType{fl, classp, تش_اشاره};
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(refDTypep);
        Ast_لاره_Call* ثابت callp
            = جديد Ast_لاره_Call{fl, جديد AstThisRef{fl, refDTypep}, wrapperp->name(), تش_اشاره};
        callp->taskp(wrapperp);
        callp->dtype_سېټ_Void();
        randomizeFuncp->جمع_کړه_Stmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    منطقي classHasRandClassMembers(AstClass* classp) {
        أرجع classp->existsMember([](ثابت AstClass*, ثابت AstVar* varp) {
            إذا (!varp->rand().isRandomizable()) أرجع خطأ;
            ثابت Ast_غوټه_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            أرجع VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    منطقي classOwnsGlobalConstraint(ثابت AstClass* classp) ثابت {
        أرجع classp->existsMember([](ثابت AstClass*, ثابت AstConstraint* constrp) {
            منطقي owns = خطأ;
            constrp->foreach([&](ثابت AstMemberSel* memberSelp) {
                ثابت Ast_غوټه* ثابت rootp
                    = const_cast<AstMemberSel*>(memberSelp)->بنسټ_Fromp(ریښتیا);
                إذا (ثابت AstVarRef* ثابت refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ثابت Ast_غوټه_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    طالما (ثابت Ast_غوټه_DType* ثابت subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    إذا (VN_IS(dtypep, ClassRefDType)) owns = ریښتیا;
                }
            });
            أرجع owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ثابت string& suffix) {
        ثابت string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.پیدا_کړه_Member(classp, name), Task);
        إذا (taskp) أرجع taskp;
        taskp = جديد AstTask{classp->fileline(), name, تش_اشاره};
        taskp->class_لاره(ریښتیا);
        classp->جمع_کړه_Membersp(taskp);
        m_memberMap.ورننباسه(classp, taskp);
        أرجع taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    فراغ populateNestedCallbackTask(AstTask* ثابت callbackTaskp, AstClass* ثابت classp,
                                    ثابت string& cbName) {
        File_کرښه* ثابت fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            إذا (!memberVarp->rand().isRandomizable()) أرجع;
            ثابت Ast_غوټه_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
            ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
            إذا (!classRefp) أرجع;
            AstClass* ثابت memberClassp = classRefp->classp();
            إذا (memberClassp == classp) أرجع;  // Avoid self-reference

            // Force-visit member class if not yet processed
            إذا (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_غوټه* stmtsp = تش_اشاره;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            إذا (AstTask* ثابت userFuncp = پیدا_کړه_Pre_Post_Task(memberClassp, cbName)) {
                Ast_لاره_Call* ثابت callp = جديد Ast_لاره_Call{
                    fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtype_سېټ_Void();
                stmtsp = Ast_غوټه::جمع_کړه_بل(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            إذا (classHasRandClassMembers(memberClassp)) {
                ثابت string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ثابت nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                Ast_لاره_Call* ثابت recurseCallp = جديد Ast_لاره_Call{
                    fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtype_سېټ_Void();
                stmtsp = Ast_غوټه::جمع_کړه_بل(stmtsp, recurseCallp->makeStmt());
            }

            إذا (!stmtsp) أرجع;

            // Wrap in null check
            AstIf* ثابت nullCheckp = جديد AstIf{
                fl,
                جديد AstNeq{fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           جديد AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->جمع_کړه_Stmtsp(wrap_If_Rand_ډېرپېښ(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* new_تیاری_Constraint_Task(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp = جديد AstTask{nodep->fileline(), name + "_setup_constraint", تش_اشاره};
        taskp->class_لاره(ریښتیا);
        nodep->جمع_کړه_Membersp(taskp);
        أرجع taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp
            = جديد AstTask{nodep->fileline(), name + "_resize_constrained_array", تش_اشاره};
        taskp->class_لاره(ریښتیا);
        nodep->جمع_کړه_Membersp(taskp);
        أرجع taskp;
    }
    Ast_غوټه_Stmt* implementConstraintsClear(File_کرښه* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جديد AstCMethodHard{
            fileline,
            جديد AstVarRef{fileline, VN_AS(genp->user2p(), غوټه_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtype_سېټ_Void();
        أرجع clearp->makeStmt();
    }
    Ast_غوټه_Stmt* implementConstraintsClearAll(File_کرښه* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جديد AstCMethodHard{
            fileline,
            جديد AstVarRef{fileline, VN_AS(genp->user2p(), غوټه_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtype_سېټ_Void();
        أرجع clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_غوټه_بیان* ثابت exprp) {
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            أرجع memberSelp->varp();
        } وإلا إذا (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            أرجع varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        أرجع تش_اشاره;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_غوټه_بیان* make_ډېرپېښ_Var_Ref(Ast_غوټه_بیان* ثابت exprp, AstVar* ثابت ډېرپېښ_Varp,
                                ثابت VAccess access) {
        إذا (ډېرپېښ_Varp->lifetime().isStatic()) {
            أرجع جديد AstVarRef{exprp->fileline(), VN_AS(ډېرپېښ_Varp->user2p(), غوټه_Module),
                                 ډېرپېښ_Varp, access};
        }
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ثابت newMemberSelp = جديد AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_ونه(خطأ), ډېرپېښ_Varp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            أرجع newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        أرجع جديد AstVarRef{exprp->fileline(), VN_AS(ډېرپېښ_Varp->user2p(), Class), ډېرپېښ_Varp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* create_Or_Get_اندازه_Var(AstClass* ثابت classp, AstVar* ثابت arrVarp, File_کرښه* ثابت fl,
                               Ast_غوټه_DType* ثابت signed32DTypep, منطقي& wasCreated) {
        AstVar* اندازه_Varp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = خطأ;
        إذا (!اندازه_Varp) {
            اندازه_Varp = جديد AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->جمع_کړه_Membersp(اندازه_Varp);
            m_memberMap.ورننباسه(classp, اندازه_Varp);
            اندازه_Varp->user2p(classp);
            arrVarp->user4p(اندازه_Varp);
            wasCreated = ریښتیا;
        }
        أرجع اندازه_Varp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_بیان* create_اندازه_Gte_Zero_Constraint(File_کرښه* ثابت fl, AstVar* ثابت اندازه_Varp,
                                                   Ast_غوټه_بیان* derefFromp = تش_اشاره) {
        Ast_غوټه_بیان* اندازه_غوټه_Exprp = تش_اشاره;
        إذا (derefFromp) {
            اندازه_غوټه_Exprp = buildMemberSelExprp(derefFromp, اندازه_Varp);
        } وإلا {
            اندازه_غوټه_Exprp = جديد AstVarRef{fl, اندازه_Varp, VAccess::READ};
        }
        اندازه_غوټه_Exprp->user1(ریښتیا);
        AstGteS* ثابت اندازه_Gtep = جديد AstGteS{fl, اندازه_غوټه_Exprp, جديد AstConst{fl, 0}};
        اندازه_Gtep->user1(ریښتیا);
        أرجع جديد Ast_Constraint_بیان{fl, اندازه_Gtep};
    }
    Ast_غوټه_بیان* getFromp(Ast_غوټه_بیان* ثابت exprp) {
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            أرجع memberSelp->fromp();
        } وإلا إذا (Ast_لاره_Call* ثابت لاره_Callp = VN_CAST(exprp, لاره_Call)) {
            أرجع لاره_Callp->fromp();
        }
        أرجع تش_اشاره;
    }
    AstVar* make_Tmp_Rand_ډېرپېښ_Var(Ast_غوټه_بیان* siblingExprp, AstVar* rand_ډېرپېښ_Varp,
                               Ast_غوټه*& storeStmtspr, Ast_غوټه_Stmt*& restoreStmtspr) {
        File_کرښه* ثابت fl = rand_ډېرپېښ_Varp->fileline();
        AstVar* ثابت rand_ډېرپېښ_Tmp_Varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, m_ډېرپېښ_Unique_نومونه.get(rand_ډېرپېښ_Varp), rand_ډېرپېښ_Varp->dtypep()};
        rand_ډېرپېښ_Tmp_Varp->funcLocal(m_ftaskp);
        rand_ډېرپېښ_Tmp_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_غوټه::جمع_کړه_بل(
            storeStmtspr,
            جديد Ast_ټاکل{fl, جديد AstVarRef{fl, rand_ډېرپېښ_Tmp_Varp, VAccess::WRITE},
                          make_ډېرپېښ_Var_Ref(siblingExprp, rand_ډېرپېښ_Varp, VAccess::READ)});
        storeStmtspr = Ast_غوټه::جمع_کړه_بل(
            storeStmtspr,
            make_ډېرپېښ_سېټ_Loop(fl, make_ډېرپېښ_Var_Ref(siblingExprp, rand_ډېرپېښ_Varp, VAccess::WRITE),
                            جديد AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_غوټه::جمع_کړه_بل(
            restoreStmtspr,
            جديد Ast_ټاکل{fl, make_ډېرپېښ_Var_Ref(siblingExprp, rand_ډېرپېښ_Varp, VAccess::WRITE),
                          جديد AstVarRef{fl, rand_ډېرپېښ_Tmp_Varp, VAccess::READ}});
        أرجع rand_ډېرپېښ_Tmp_Varp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_غوټه_بیان* ټوټه_منزل_Common_مختاړی(Ast_غوټه_بیان* thisp, Ast_غوټه_بیان* otherp) {
        ساكن std::ویکټور<Ast_غوټه_بیان*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        طالما (thisp) {
            thisHier.ټېل_شا(thisp);
            thisp = getFromp(thisp);
        }
        طالما (otherp) {
            otherHier.ټېل_شا(otherp);
            otherp = getFromp(otherp);
        }
        Ast_غوټه_بیان* commonp = تش_اشاره;
        لكل (خپلکاری thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            إذا ((*thisIt)->type() != (*otherIt)->type()) اكسر;
            إذا (AstMemberSel* ثابت memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ثابت otherMemberSelp = VN_AS(*otherIt, MemberSel);
                إذا (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    استمر;
                }
            } وإلا إذا (Ast_لاره_Call* ثابت this_لاره_Callp = VN_CAST(*thisIt, لاره_Call)) {
                Ast_لاره_Call* ثابت other_لاره_Callp = VN_AS(*otherIt, لاره_Call);
                إذا (this_لاره_Callp->taskp() == other_لاره_Callp->taskp()) {
                    commonp = this_لاره_Callp;
                    استمر;
                }
            } وإلا إذا (AstVarRef* ثابت لومړی_Var_Refp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ثابت دوهم_Var_Refp = VN_AS(*otherIt, VarRef);
                إذا (لومړی_Var_Refp->varp() == دوهم_Var_Refp->varp()) {
                    commonp = لومړی_Var_Refp;
                    استمر;
                }
            }
            اكسر;
        }
        أرجع commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_غوټه_بیان* buildMemberSelExprp(Ast_غوټه_بیان* exprp, AstVar* تر_ټولو_ژور_Varp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_غوټه_بیان* resExprp = تش_اشاره;
        إذا (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), تش_اشاره);
            AstVar* varp = memberSelp->varp();
            إذا (تر_ټولو_ژور_Varp) varp = تر_ټولو_ژور_Varp;
            resExprp = جديد AstMemberSel{exprp->fileline(), resExprp, varp};
        } وإلا إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            إذا (تر_ټولو_ژور_Varp) varp = تر_ټولو_ژور_Varp;
            resExprp = جديد AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), غوټه_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(ریښتیا);
        أرجع resExprp;
    }

    فراغ جمع_کړه_Update_Rand_Vars_Body(AstClass* ثابت nodep, AstFunc* ثابت randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ثابت updatep
            = جديد AstFunc{nodep->fileline(), "__VupdateRandVars", تش_اشاره, تش_اشاره};
        updatep->class_لاره(ریښتیا);
        updatep->isVirtual(ریښتیا);
        updatep->keepAlive(ریښتیا);
        updatep->needsSyms(خطأ);
        nodep->جمع_کړه_Membersp(updatep);
        nodep->hasRandVarsUpdate(ریښتیا);

        لكل (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : تش_اشاره) {
            Ast_غوټه_FTask* ثابت newp = VN_AS(m_memberMap.پیدا_کړه_Member(classp, "new"), غوټه_FTask);
            لكل (Ast_غوټه* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_بیان* ثابت stmtExprp = VN_CAST(stmtp, Stmt_بیان);
                إذا (!stmtExprp) استمر;
                AstCMethodHard* ثابت methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                إذا (!methodp || methodp->لاره() != VCMethod::RANDOMIZER_WRITE_VAR) استمر;

                Ast_غوټه_Var_Ref* ثابت varRefp = VN_AS(methodp->pinsp(), غوټه_Var_Ref);
                إذا (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    استمر;
                }
                AstCMethodHard* ثابت updateMethodp = جديد AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_ونه_خالص(خطأ),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_ونه_خالص(خطأ)};
                updateMethodp->dtype_سېټ_Void();
                Ast_غوټه_بیان* ثابت namep = VN_AS(varRefp->nextp()->nextp(), غوټه_بیان);
                updateMethodp->جمع_کړه_Pinsp(namep->clone_ونه_خالص(خطأ));
                updateMethodp->لاره(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->جمع_کړه_Stmtsp(updateMethodp->makeStmt());
            }
        }
    }

    فراغ جمع_کړه_Basic_Randomize_Body(AstFunc* ثابت basicRandomizep, AstClass* ثابت nodep,
                               AstVar* rand_ډېرپېښ_Varp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        AstVar* ثابت basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ثابت basicFvarRefp = جديد AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ثابت beginBasicValp = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 1};
        basicRandomizep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, basicFvarRefp, beginBasicValp});
        Ast_غوټه_FTask* ثابت newp = VN_AS(m_memberMap.پیدا_کړه_Member(nodep, "new"), غوټه_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            إذا (!memberVarp->rand().isRandomizable()) أرجع;
            ثابت Randomize_ډېرپېښ rand_ډېرپېښ = {.asUQuad = memberVarp->user1()};
            إذا (rand_ډېرپېښ.کاروي_ډېرپېښ
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ثابت setp = جديد AstCMethodHard{
                    nodep->fileline(),
                    جديد AstVarRef{fl, VN_AS(rand_ډېرپېښ_Varp->user2p(), غوټه_Module), rand_ډېرپېښ_Varp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, جديد AstConst{nodep->fileline(), rand_ډېرپېښ.شاخص}};
                setp->dtype_سېټ_UInt32();
                newp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, setp, جديد AstConst{fl, 0}});
            }
            إذا (memberVarp->user3()) أرجع;  // Handled in constraints
            ثابت Ast_غوټه_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
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
                Ast_لاره_Call* ثابت callp
                    = memberVarp->globalConstrained()
                          ? جديد Ast_لاره_Call{fl,
                                              جديد AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : جديد Ast_لاره_Call{
                                fl, جديد AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ثابت basicFvarRefReadp = basicFvarRefp->clone_ونه(خطأ);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ثابت ټاکل_If_Not_Nullp = جديد AstIf{
                    fl,
                    جديد AstNeq{fl, جديد AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               جديد AstConst{fl, AstConst::Null{}}},
                    جديد Ast_ټاکل{fl, basicFvarRefp->clone_ونه(خطأ),
                                  جديد AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->جمع_کړه_Stmtsp(wrap_If_Rand_ډېرپېښ(nodep, memberVarp, ټاکل_If_Not_Nullp));
            } وإلا {
                AstVar* ثابت randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ثابت refp
                    = جديد AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_غوټه_Stmt* ثابت stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                إذا (!refp->backp()) VL_DO_DANGLING(refp->delete_ونه(), refp);
                basicRandomizep->جمع_کړه_Stmtsp(
                    جديد AstBegin{fl, m_unique_نومونه.get("__VrandBlock"), stmtp, خطأ});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_غوټه_بیان* make_ډېرپېښ_ټاکل_Lhs(File_کرښه* ثابت fl, AstClass* ثابت classp,
                                   Ast_غوټه_بیان* ثابت fromp, AstVar* ثابت ډېرپېښ_Varp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        إذا (ډېرپېښ_Varp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            إذا (fromp) VL_DO_DANGLING(fromp->unlink_Fr_شا()->delete_ونه(), fromp);
            أرجع جديد AstVarRef{fl, VN_AS(ډېرپېښ_Varp->user2p(), غوټه_Module), ډېرپېښ_Varp,
                                 VAccess::WRITE};
        } وإلا إذا (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            أرجع جديد AstVarRef{fl, VN_AS(ډېرپېښ_Varp->user2p(), غوټه_Module), ډېرپېښ_Varp,
                                 VAccess::WRITE};
        } وإلا {
            AstMemberSel* ثابت memberselp = جديد AstMemberSel{fl, fromp->unlink_Fr_شا(), ډېرپېښ_Varp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            أرجع memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    فراغ replace_With_ډېرپېښ_ټاکل(Ast_غوټه_FTask_Ref* ثابت ftaskRefp, Ast_غوټه* ثابت receiverp,
                               Ast_غوټه_بیان* ثابت lhsp) {
        replace_With_ډېرپېښ_ټاکل_And_Append(ftaskRefp, receiverp, lhsp, تش_اشاره);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    فراغ replace_With_ډېرپېښ_ټاکل_And_Append(Ast_غوټه_FTask_Ref* ثابت ftaskRefp, Ast_غوټه* ثابت receiverp,
                                        Ast_غوټه_بیان* ثابت lhsp, Ast_غوټه* ثابت appendStmtp) {
        File_کرښه* ثابت fl = ftaskRefp->fileline();
        إذا (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_بیان), ftaskRefp, "Should be a statement");
            Ast_غوټه_بیان* ثابت rhsp = ftaskRefp->argsp()->exprp()->unlink_Fr_شا();
            Ast_غوټه* newStmtp = تش_اشاره;
            إذا (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                //ډېرپېښ
                ثابت Randomize_ډېرپېښ rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.کاروي_ډېرپېښ, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ثابت setp = جديد AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                جديد AstConst{fl, rmode.شاخص}};
                setp->dtype_سېټ_UInt32();
                newStmtp = جديد Ast_ټاکل{fl, setp, rhsp};
            } وإلا {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = make_ډېرپېښ_سېټ_Loop(fl, lhsp, rhsp, m_ftaskp);
            }
            إذا (appendStmtp) newStmtp->جمع_کړه_بل(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(ټېل_Deletep(m_stmtp), m_stmtp = تش_اشاره);
        } وإلا {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ثابت Randomize_ډېرپېښ rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.کاروي_ډېرپېښ, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ثابت setp = جديد AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            جديد AstConst{fl, rmode.شاخص}};
            setp->dtype_سېټ_UInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(ټېل_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    فراغ wrapRandomizeCallWithNullGuard(Ast_غوټه_FTask_Ref* nodep) {
        Ast_لاره_Call* ثابت callp = VN_CAST(nodep, لاره_Call);
        إذا (!callp) أرجع;
        إذا (callp->user4_سېټ_Once()) أرجع;
        File_کرښه* ثابت fl = callp->fileline();
        Ast_غوټه_بیان* ثابت checkp
            = جديد AstNeq{fl, callp->fromp()->clone_ونه(خطأ), جديد AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlink_Fr_شا(&relinker);
        relinker.relink(جديد AstCond{fl, checkp, callp, جديد AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    فراغ handle_Randomize_دلیلونه(Ast_غوټه_FTask_Ref* ثابت nodep) {
        إذا (!nodep->argsp()) أرجع;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        منطقي hasNullArg = خطأ;
        لكل (AstArg *argp = nodep->argsp(), *nextp = تش_اشاره; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->عدد().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = ریښتیا;
                VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        لكل (AstArg *argp = nodep->argsp(), *nextp = تش_اشاره; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            لكل (AstArg *otherArgp = nextp, *otherNextp = تش_اشاره; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                إذا (Ast_غوټه_بیان* ثابت prefixp
                    = ټوټه_منزل_Common_مختاړی(argp->exprp(), otherArgp->exprp())) {
                    إذا (prefixp == argp->exprp()) {
                        إذا (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlink_Fr_شا()->delete_ونه(), otherArgp);
                        استمر;
                    }
                }
                إذا (Ast_غوټه_بیان* ثابت prefixp
                    = ټوټه_منزل_Common_مختاړی(otherArgp->exprp(), argp->exprp())) {
                    إذا (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);
                        اكسر;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::سېټ<AstVar*> saved_Rand_ډېرپېښ_Varps;
        AstVar* tmpVarps = تش_اشاره;
        Ast_غوټه* storeStmtsp = تش_اشاره;
        Ast_غوټه* سېټ_Stmtsp = تش_اشاره;
        Ast_غوټه_Stmt* restoreStmtsp = تش_اشاره;
        لكل (AstArg *argp = nodep->argsp(), *nextp = تش_اشاره; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_غوټه_بیان* exprp = argp->exprp();
            Ast_غوټه_بیان* ثابت commonPrefixp = ټوټه_منزل_Common_مختاړی(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_کرښه* ثابت fl = argp->fileline();
            طالما (exprp) {
                إذا (commonPrefixp == exprp) اكسر;
                AstVar* ثابت randVarp = getVarFromRef(exprp);
                AstClass* ثابت classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ثابت rand_ډېرپېښ_Varp = randVarp->lifetime().isStatic()
                                                 ? get_Static_Rand_ډېرپېښ_Var(classp)
                                                 : get_Rand_ډېرپېښ_Var_From_Class(classp);
                UASSERT_OBJ(rand_ډېرپېښ_Varp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                إذا (saved_Rand_ډېرپېښ_Varps.پیدا_کړه(rand_ډېرپېښ_Varp) == saved_Rand_ډېرپېښ_Varps.پای_ټکی()) {
                    AstVar* ثابت rand_ډېرپېښ_Tmp_Varp
                        = make_Tmp_Rand_ډېرپېښ_Var(exprp, rand_ډېرپېښ_Varp, storeStmtsp, restoreStmtsp);
                    saved_Rand_ډېرپېښ_Varps.ورننباسه(rand_ډېرپېښ_Varp);
                    tmpVarps = Ast_غوټه::جمع_کړه_بل(tmpVarps, rand_ډېرپېښ_Tmp_Varp);
                }
                ثابت Randomize_ډېرپېښ rand_ډېرپېښ = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = جديد AstCMethodHard{
                    fl, make_ډېرپېښ_Var_Ref(exprp, rand_ډېرپېښ_Varp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, جديد AstConst{fl, rand_ډېرپېښ.شاخص}};
                setp->dtype_سېټ_UInt32();
                سېټ_Stmtsp
                    = Ast_غوټه::جمع_کړه_بل(سېټ_Stmtsp, جديد Ast_ټاکل{fl, setp, جديد AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);
        }
        إذا (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* موخه_Classp = تش_اشاره;
            Ast_لاره_Call* ثابت لاره_Callp = VN_CAST(nodep, لاره_Call);
            إذا (لاره_Callp) {
                ثابت Ast_غوټه_DType* ثابت fromDTypep = لاره_Callp->fromp()->dtypep();
                ثابت AstClassRefDType* ثابت crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                موخه_Classp = crdtp->classp();
            } وإلا {
                موخه_Classp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(موخه_Classp, nodep, "randomize(null) target class unresolved");
            AstFunc* ثابت checkOnlyFuncp = getCreateRandomizeNullFunc(موخه_Classp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        إذا (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && سېټ_Stmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlink_Fr_شا(&relinker);
            Ast_غوټه* ثابت stmtsp = storeStmtsp;
            stmtsp->جمع_کړه_بل(سېټ_Stmtsp);
            stmtsp->جمع_کړه_بل(m_stmtp);
            stmtsp->جمع_کړه_بل(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            جمع_کړه_لنډمهاله_Varsp(tmpVarps);
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
        إذا (AstFunc* ثابت existingp = VN_AS(m_memberMap.پیدا_کړه_Member(classp, name), Func)) {
            أرجع existingp;
        }
        File_کرښه* ثابت fl = classp->fileline();
        AstFunc* ثابت funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ثابت fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        جمع_کړه_Pre_Post_Call(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ثابت classGenp = getRandomGenerator(classp);
        إذا (!classGenp) {
            funcp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE},
                                           جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 1}});
        } وإلا {
            Ast_غوټه_Module* ثابت genModp = VN_AS(classGenp->user2p(), غوټه_Module);
            funcp->جمع_کړه_Stmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ثابت تیاری_All_Taskp = get_Create_Constraint_تیاری_Func(classp, m_memberMap);
            funcp->جمع_کړه_Stmtsp((جديد AstTaskRef{fl, تیاری_All_Taskp})->makeStmt());
            AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
            solverCallp->dtype_سېټ_بټ();
            solverCallp->جمع_کړه(جديد AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->جمع_کړه(".next_check_only(__Vm_rng)");
            funcp->جمع_کړه_Stmtsp(
                جديد Ast_ټاکل{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(ریښتیا);
        }

        // 3. post_randomize -- only if result is non-zero
        إذا (AstTask* ثابت userPostp = پیدا_کړه_Pre_Post_Task(classp, "post_randomize")) {
            AstTaskRef* ثابت callp = جديد AstTaskRef{userPostp->fileline(), userPostp};
            funcp->جمع_کړه_Stmtsp(
                جديد AstIf{fl, جديد AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        أرجع funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* lift_Log_If_Chain_منزل_Constraint_If(AstLogIf* logIfp) {
        File_کرښه* ثابت fl = logIfp->fileline();
        Ast_غوټه_بیان* ثابت condp = logIfp->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhsp = logIfp->rhsp()->unlink_Fr_شا();
        Ast_غوټه* thenBodyp;
        إذا (AstLogIf* ثابت innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = lift_Log_If_Chain_منزل_Constraint_If(innerLogIfp);
            VL_DO_DANGLING(ټېل_Deletep(innerLogIfp), innerLogIfp);
        } وإلا {
            thenBodyp = جديد Ast_Constraint_بیان{fl, rhsp};
        }
        أرجع جديد AstConstraintIf{fl, condp, thenBodyp, تش_اشاره};
    }

    ساكن منطقي distBoundRefsRandVar(ثابت Ast_غوټه* boundp) {
        أرجع boundp->exists(
            [](ثابت AstVarRef* vrefp) { أرجع vrefp->varp()->rand().isRandomizable(); });
    }

    ساكن منطقي dist_Bound_Refs_ډېرپېښ_Var(ثابت Ast_غوټه* boundp) {
        أرجع boundp->exists([](ثابت AstVarRef* vrefp) {
            إذا (!vrefp->varp()->rand().isRandomizable()) أرجع خطأ;
            ثابت Randomize_ډېرپېښ rmode = {.asUQuad = vrefp->varp()->user1()};
            أرجع rmode.کاروي_ډېرپېښ;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ساكن Ast_غوټه_بیان* newDistRangeMembership(AstDist* distp, ثابت AstInsideRange* irp) {
        File_کرښه* ثابت fl = distp->fileline();
        ثابت منطقي isSigned = distp->exprp()->isSigned();
        Ast_غوټه_بیان* ثابت dist_بیان_Gtep = distp->exprp()->clone_ونه_خالص(خطأ);
        Ast_غوټه_بیان* ثابت dist_بیان_Ltep = distp->exprp()->clone_ونه_خالص(خطأ);
        dist_بیان_Gtep->user1(ریښتیا);
        dist_بیان_Ltep->user1(ریښتیا);
        Ast_غوټه_بیان* ثابت gep
            = isSigned ? static_cast<Ast_غوټه_بیان*>(
                             جديد AstGteS{fl, dist_بیان_Gtep, irp->lhsp()->clone_ونه_خالص(خطأ)})
                       : static_cast<Ast_غوټه_بیان*>(
                             جديد AstGte{fl, dist_بیان_Gtep, irp->lhsp()->clone_ونه_خالص(خطأ)});
        Ast_غوټه_بیان* ثابت lep
            = isSigned ? static_cast<Ast_غوټه_بیان*>(
                             جديد AstLteS{fl, dist_بیان_Ltep, irp->rhsp()->clone_ونه_خالص(خطأ)})
                       : static_cast<Ast_غوټه_بیان*>(
                             جديد AstLte{fl, dist_بیان_Ltep, irp->rhsp()->clone_ونه_خالص(خطأ)});
        gep->user1(ریښتیا);
        lep->user1(ریښتیا);
        Ast_غوټه_بیان* ثابت andp = جديد AstLogAnd{fl, gep, lep};
        andp->user1(ریښتیا);
        أرجع andp;
    }

    بنية Dist_لوښی final {
        Ast_غوټه_بیان* rangep;  // A single value or an InsideRange
        Ast_غوټه_بیان* وزن_Exprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::ویکټور<Dist_لوښی> collectDistBuckets(AstDist* ثابت distp) {
        File_کرښه* ثابت fl = distp->fileline();
        std::ویکټور<Dist_لوښی> buckets;
        لكل (Ast_Dist_توکی* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), Dist_توکی)) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(ditemp->weightp(), Const)) {
                إذا (constp->منزل_UQuad() == 0) استمر;
            }
            Ast_غوټه_بیان* وزن_Exprp
                = جديد AstExtend{fl, ditemp->weightp()->clone_ونه_خالص(خطأ), 64};
            // := on a range weights every element, so scale by the range size.
            إذا (!ditemp->is_ټولیز()) {
                إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ثابت AstConst* ثابت lop = VN_CAST(irp->lhsp(), Const);
                    ثابت AstConst* ثابت hip = VN_CAST(irp->rhsp(), Const);
                    Ast_غوټه_بیان* rangeSizep;
                    إذا (lop && hip) {
                        ثابت uint64_t range_اندازه = hip->منزل_UQuad() - lop->منزل_UQuad() + 1;
                        rangeSizep = جديد AstConst{fl, AstConst::Unsized64{}, range_اندازه};
                    } وإلا {
                        rangeSizep = جديد Ast_جمع_کړه{
                            fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                            جديد AstSub{fl,
                                       جديد AstExtend{fl, irp->rhsp()->clone_ونه_خالص(خطأ), 64},
                                       جديد AstExtend{fl, irp->lhsp()->clone_ونه_خالص(خطأ), 64}}};
                    }
                    وزن_Exprp = جديد AstMul{fl, وزن_Exprp, rangeSizep};
                }
            }
            buckets.ټېل_شا({ditemp->rangep(), وزن_Exprp});
        }
        أرجع buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ساكن Ast_غوټه_بیان* new_Dist_Membership_غړی(AstDist* distp, Ast_غوټه_بیان* rangep) {
        إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            أرجع newDistRangeMembership(distp, irp);
        }
        File_کرښه* ثابت fl = distp->fileline();
        Ast_غوټه_بیان* ثابت eqExprp = distp->exprp()->clone_ونه_خالص(خطأ);
        eqExprp->user1(ریښتیا);
        Ast_غوټه_بیان* ثابت eqp = جديد AstEq{fl, eqExprp, rangep->clone_ونه_خالص(خطأ)};
        eqp->user1(ریښتیا);
        أرجع eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_بیان* buildDistMembership(AstDist* distp,
                                           ثابت std::ویکټور<Dist_لوښی>& buckets) {
        File_کرښه* ثابت fl = distp->fileline();
        Ast_غوټه_بیان* unionExprp = تش_اشاره;
        لكل (ثابت خپلکاری& لوښی : buckets) {
            Ast_غوټه_بیان* ثابت memberp = new_Dist_Membership_غړی(distp, لوښی.rangep);
            إذا (!unionExprp) {
                unionExprp = memberp;
            } وإلا {
                unionExprp = جديد AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(ریښتیا);
            }
        }
        أرجع جديد Ast_Constraint_بیان{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_غوټه_بیان* newGateJoin(File_کرښه* fl, Ast_غوټه_بیان* ap, Ast_غوټه_بیان* bp, منطقي isAnd) {
        إذا (ap->same_ونه(bp)) {
            VL_DO_DANGLING(ټېل_Deletep(bp), bp);
            أرجع ap;
        }
        إذا (VN_IS(bp, Const)) std::بدلون(ap, bp);
        إذا (VN_IS(ap, Const)) {
            ثابت منطقي constWins = VN_AS(ap, Const)->isZero() == isAnd;
            إذا (!constWins) std::بدلون(ap, bp);
            VL_DO_DANGLING(ټېل_Deletep(bp), bp);
            أرجع ap;
        }
        إذا (isAnd) أرجع جديد AstLogAnd{fl, ap, bp};
        أرجع جديد AstLogOr{fl, ap, bp};
    }
    Ast_غوټه_بیان* newGateOr(File_کرښه* fl, Ast_غوټه_بیان* ap, Ast_غوټه_بیان* bp) {
        أرجع newGateJoin(fl, ap, bp, خطأ);
    }
    Ast_غوټه_بیان* newGateAnd(File_کرښه* fl, Ast_غوټه_بیان* ap, Ast_غوټه_بیان* bp) {
        أرجع newGateJoin(fl, ap, bp, ریښتیا);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_غوټه_بیان* new_ډېرپېښ_بټ_Read(AstVar* varp, ثابت AstMemberSel* mselp, AstVar* rand_ډېرپېښ_Varp,
                                File_کرښه* fl) {
        Ast_غوټه_بیان* arrayp = تش_اشاره;
        إذا (varp->lifetime().isStatic()) {
            AstClass* ثابت ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ثابت smodep = get_Static_Rand_ډېرپېښ_Var(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = جديد AstVarRef{fl, VN_AS(smodep->user2p(), غوټه_Module), smodep, VAccess::READ};
        } وإلا إذا (mselp) {
            AstVar* ثابت memberModep = get_Rand_ډېرپېښ_Var_From_Class(VN_AS(varp->user2p(), غوټه_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = جديد AstMemberSel{fl, mselp->fromp()->clone_ونه_خالص(خطأ), memberModep};
        } وإلا {
            UASSERT_OBJ(rand_ډېرپېښ_Varp, varp, "rand_mode variable without a class mode array");
            arrayp = جديد AstVarRef{fl, VN_AS(rand_ډېرپېښ_Varp->user2p(), غوټه_Module), rand_ډېرپېښ_Varp,
                                   VAccess::READ};
        }
        ثابت Randomize_ډېرپېښ rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ثابت atp
            = جديد AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, جديد AstConst{fl, rmode.شاخص}};
        atp->dtype_سېټ_UInt32();
        أرجع atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_غوټه_بیان* newVarGate(AstVar* varp, ثابت AstMemberSel* mselp, منطقي ownerLevel,
                            AstVar* rand_ډېرپېښ_Varp, File_کرښه* fl) {
        إذا (!varp->rand().isRandomizable()) {
            أرجع جديد AstConst{fl, AstConst::بټ_True{}, ownerLevel};
        }
        ثابت Randomize_ډېرپېښ rmode = {.asUQuad = varp->user1()};
        إذا (!rmode.کاروي_ډېرپېښ) أرجع جديد AstConst{fl, AstConst::بټ_True{}};
        أرجع new_ډېرپېښ_بټ_Read(varp, mselp, rand_ډېرپېښ_Varp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_غوټه_بیان* newAccessGate(ثابت Ast_غوټه* nodep, منطقي ownerLevel, AstVar* rand_ډېرپېښ_Varp,
                               File_کرښه* fl) {
        إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel)) {
            أرجع newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, rand_ډېرپېښ_Varp, fl),
                              newAccessGate(mselp->fromp(), ریښتیا, rand_ډېرپېښ_Varp, fl));
        }
        إذا (ثابت Ast_غوټه_Var_Ref* ثابت refp = VN_CAST(nodep, غوټه_Var_Ref)) {
            أرجع newVarGate(refp->varp(), تش_اشاره, ownerLevel, rand_ډېرپېښ_Varp, fl);
        }
        إذا (ثابت Ast_غوټه_Sel* ثابت selp = VN_CAST(nodep, غوټه_Sel)) {
            أرجع newAccessGate(selp->fromp(), ریښتیا, rand_ډېرپېښ_Varp, fl);
        }
        إذا (dist_Bound_Refs_ډېرپېښ_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        أرجع جديد AstConst{fl, AstConst::بټ_True{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_غوټه_بیان* newDistGate(ثابت Ast_غوټه* nodep, AstVar* rand_ډېرپېښ_Varp, File_کرښه* fl) {
        إذا (VN_IS(nodep, غوټه_Var_Ref) || VN_IS(nodep, MemberSel)) {
            أرجع newAccessGate(nodep, خطأ, rand_ډېرپېښ_Varp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        إذا (ثابت Ast_غوټه_Sel* ثابت selp = VN_CAST(nodep, غوټه_Sel)) {
            أرجع newDistGate(selp->fromp(), rand_ډېرپېښ_Varp, fl);
        }
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            أرجع newDistGate(selp->fromp(), rand_ډېرپېښ_Varp, fl);
        }
        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            أرجع newDistGate(selp->fromp(), rand_ډېرپېښ_Varp, fl);
        }
        إذا (ثابت AstCMethodHard* ثابت methodp = VN_CAST(nodep, CMethodHard)) {
            إذا (methodp->لاره() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                أرجع newDistGate(methodp->fromp(), rand_ډېرپېښ_Varp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        إذا (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            Ast_غوټه_بیان* ثابت thenGatep = newDistGate(condp->thenp(), rand_ډېرپېښ_Varp, fl);
            Ast_غوټه_بیان* ثابت elseGatep = newDistGate(condp->elsep(), rand_ډېرپېښ_Varp, fl);
            Ast_غوټه_بیان* armGatep = thenGatep;
            إذا (thenGatep->same_ونه(elseGatep)) {
                VL_DO_DANGLING(ټېل_Deletep(elseGatep), elseGatep);
            } وإلا {
                armGatep
                    = جديد AstCond{fl, condp->condp()->clone_ونه_خالص(خطأ), thenGatep, elseGatep};
            }
            أرجع newGateOr(fl, newDistGate(condp->condp(), rand_ډېرپېښ_Varp, fl), armGatep);
        }
        إذا (ثابت Ast_غوټه_Uniop* ثابت uopp = VN_CAST(nodep, غوټه_Uniop)) {
            أرجع newDistGate(uopp->lhsp(), rand_ډېرپېښ_Varp, fl);
        }
        إذا (ثابت Ast_غوټه_Biop* ثابت bopp = VN_CAST(nodep, غوټه_Biop)) {
            أرجع newGateOr(fl, newDistGate(bopp->lhsp(), rand_ډېرپېښ_Varp, fl),
                             newDistGate(bopp->rhsp(), rand_ډېرپېښ_Varp, fl));
        }
        إذا (dist_Bound_Refs_ډېرپېښ_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        أرجع جديد AstConst{fl, AstConst::بټ_True{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_بیان* buildFrozenMembership(AstDist* distp,
                                             ثابت std::ویکټور<Dist_لوښی>& buckets) {
        File_کرښه* ثابت fl = distp->fileline();
        Ast_غوټه_بیان* unionExprp = تش_اشاره;
        لكل (ثابت خپلکاری& لوښی : buckets) {
            Ast_غوټه_بیان* termp = new_Dist_Membership_غړی(distp, لوښی.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ثابت منطقي runtime_وزن
                = لوښی.وزن_Exprp->exists([](ثابت Ast_غوټه_Var_Ref*) { أرجع ریښتیا; });
            إذا (runtime_وزن) {
                AstNeq* ثابت nzp = جديد AstNeq{fl, لوښی.وزن_Exprp->clone_ونه_خالص(خطأ),
                                               جديد AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(ریښتیا);
                termp = جديد AstLogAnd{fl, termp, nzp};
                termp->user1(ریښتیا);
            }
            إذا (unionExprp) {
                unionExprp = جديد AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(ریښتیا);
            } وإلا {
                unionExprp = termp;
            }
        }
        أرجع جديد Ast_Constraint_بیان{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_غوټه* gateFrozenDist(Ast_غوټه* chainp, AstDist* distp,
                            ثابت std::ویکټور<Dist_لوښی>& buckets, AstVar* rand_ډېرپېښ_Varp) {
        File_کرښه* ثابت fl = distp->fileline();
        Ast_غوټه_بیان* gatep = newDistGate(distp->exprp(), rand_ډېرپېښ_Varp, fl);
        إذا (VN_IS(gatep, Const)) {
            ثابت منطقي drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(ټېل_Deletep(gatep), gatep);
            إذا (drawable) أرجع chainp;
            VL_DO_DANGLING(ټېل_Deletep(chainp), chainp);
            أرجع buildFrozenMembership(distp, buckets);
        }
        أرجع جديد AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_غوټه_بیان* new_یوشان_Range_Pick(AstDist* distp, ثابت AstInsideRange* irp) {
        File_کرښه* ثابت fl = distp->fileline();
        Ast_غوټه_بیان* ثابت dist_بیان_Copyp = distp->exprp()->clone_ونه_خالص(خطأ);
        dist_بیان_Copyp->user1(ریښتیا);
        ثابت صحيح dist_پلنوالی = distp->exprp()->پلنوالی();
        ثابت AstConst* ثابت lopC = VN_CAST(irp->lhsp(), Const);
        ثابت AstConst* ثابت hipC = VN_CAST(irp->rhsp(), Const);
        Ast_غوټه_بیان* rangeSzp;
        إذا (lopC && hipC) {
            ثابت uint64_t rsz = hipC->منزل_UQuad() - lopC->منزل_UQuad() + 1;
            rangeSzp = جديد AstConst{fl, AstConst::Unsized64{}, rsz};
        } وإلا {
            ثابت منطقي isSigned = irp->lhsp()->isSigned();
            Ast_غوټه_بیان* ثابت lo64p
                = isSigned ? static_cast<Ast_غوټه_بیان*>(
                                 جديد AstExtendS{fl, irp->lhsp()->clone_ونه_خالص(خطأ), 64})
                           : static_cast<Ast_غوټه_بیان*>(
                                 جديد AstExtend{fl, irp->lhsp()->clone_ونه_خالص(خطأ), 64});
            Ast_غوټه_بیان* ثابت hi64p
                = isSigned ? static_cast<Ast_غوټه_بیان*>(
                                 جديد AstExtendS{fl, irp->rhsp()->clone_ونه_خالص(خطأ), 64})
                           : static_cast<Ast_غوټه_بیان*>(
                                 جديد AstExtend{fl, irp->rhsp()->clone_ونه_خالص(خطأ), 64});
            rangeSzp = جديد Ast_جمع_کړه{fl, جديد AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  جديد AstSub{fl, hi64p, lo64p}};
        }
        Ast_غوټه_بیان* ثابت rand64p = جديد AstRand{fl, تش_اشاره, خطأ};
        rand64p->dtype_سېټ_UInt64();
        Ast_غوټه_بیان* ثابت offsetp
            = جديد AstCCast{fl, جديد AstModDiv{fl, rand64p, rangeSzp}, dist_پلنوالی};
        Ast_غوټه_بیان* ثابت valuep = جديد Ast_جمع_کړه{fl, irp->lhsp()->clone_ونه_خالص(خطأ), offsetp};
        Ast_غوټه_بیان* ثابت eqp = جديد AstEq{fl, dist_بیان_Copyp, valuep};
        eqp->user1(ریښتیا);
        أرجع eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_غوټه* build_Weighted_لوښی_Chain(AstDist* distp, ثابت std::ویکټور<Dist_لوښی>& buckets,
                                      AstVar* لوښی_Varp,
                                      ثابت std::ویکټور<Ast_غوټه_بیان*>& cumSums) {
        File_کرښه* ثابت fl = distp->fileline();
        Ast_غوټه_بیان* chainp = تش_اشاره;
        لكل (صحيح i = static_cast<صحيح>(buckets.اندازه()) - 1; i >= 0; --i) {
            Ast_غوټه_بیان* constraintExprp;
            ثابت AstInsideRange* ثابت irp = VN_CAST(buckets[i].rangep, InsideRange);
            إذا (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } وإلا إذا (irp) {
                constraintExprp = new_یوشان_Range_Pick(distp, irp);
            } وإلا {
                Ast_غوټه_بیان* ثابت dist_بیان_Copyp = distp->exprp()->clone_ونه_خالص(خطأ);
                dist_بیان_Copyp->user1(ریښتیا);
                constraintExprp
                    = جديد AstEq{fl, dist_بیان_Copyp, buckets[i].rangep->clone_ونه_خالص(خطأ)};
                constraintExprp->user1(ریښتیا);
            }
            إذا (!chainp) {
                chainp = constraintExprp;
            } وإلا {
                Ast_غوټه_بیان* ثابت condp
                    = جديد AstLte{fl, جديد AstVarRef{fl, لوښی_Varp, VAccess::READ}, cumSums[i]};
                condp->user1(ریښتیا);
                chainp = جديد AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(ریښتیا);
            }
        }
        Ast_Constraint_بیان* ثابت chainExprp = جديد Ast_Constraint_بیان{fl, chainp};
        chainExprp->isSoft(ریښتیا);
        أرجع chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    فراغ ښکتنی_Dist_Constraints(AstTask* taskp, Ast_غوټه* constrItemsp, AstVar* rand_ډېرپېښ_Varp,
                              AstConstraintForeach* foreachp = تش_اشاره) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_غوټه* foreachTailp = تش_اشاره;
        خپلکاری جمع_کړه_Stmt = [&](Ast_غوټه* nodep) {
            إذا (foreachp) {
                إذا (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } وإلا {
                    foreachTailp->جمع_کړه_بل(nodep);
                    foreachTailp = nodep;
                }
            } وإلا {
                taskp->جمع_کړه_Stmtsp(nodep);
            }
        };

        لكل (Ast_غوټه *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            إذا (AstConstraintIf* ثابت cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                ښکتنی_Dist_Constraints(taskp, cifp->thensp(), rand_ډېرپېښ_Varp, foreachp);
                إذا (cifp->elsesp()) {
                    ښکتنی_Dist_Constraints(taskp, cifp->elsesp(), rand_ډېرپېښ_Varp, foreachp);
                }
                استمر;
            }
            إذا (AstConstraintForeach* ثابت cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                ښکتنی_Dist_Constraints(taskp, cfep->bodyp(), rand_ډېرپېښ_Varp, cfep);
                استمر;
            }

            Ast_Constraint_بیان* ثابت constrExprp = VN_CAST(itemp, Constraint_بیان);
            إذا (!constrExprp) استمر;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            إذا (AstLogIf* ثابت سر_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_غوټه* chainEndp = سر_Log_Ifp->rhsp();
                طالما (AstLogIf* ثابت innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                إذا (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ثابت liftedp = lift_Log_If_Chain_منزل_Constraint_If(سر_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(ټېل_Deletep(constrExprp), constrExprp);
                    ښکتنی_Dist_Constraints(taskp, liftedp->thensp(), rand_ډېرپېښ_Varp, foreachp);
                    استمر;
                }
            }

            AstDist* ثابت distp = VN_CAST(constrExprp->exprp(), Dist);
            إذا (!distp) استمر;

            File_کرښه* ثابت fl = distp->fileline();

            std::ویکټور<Dist_لوښی> buckets = collectDistBuckets(distp);

            إذا (buckets.تشه()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_بیان* ثابت truep
                    = جديد Ast_Constraint_بیان{fl, جديد AstConst{fl, AstConst::بټ_True{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(ټېل_Deletep(constrExprp), constrExprp);
                استمر;
            }

            Ast_Constraint_بیان* ثابت membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_غوټه_بیان* ټول_وزن_Exprp = تش_اشاره;
            لكل (خپلکاری& لوښی : buckets) {
                إذا (!ټول_وزن_Exprp) {
                    ټول_وزن_Exprp = لوښی.وزن_Exprp->clone_ونه_خالص(خطأ);
                } وإلا {
                    ټول_وزن_Exprp = جديد Ast_جمع_کړه{fl, ټول_وزن_Exprp,
                                                  لوښی.وزن_Exprp->clone_ونه_خالص(خطأ)};
                    ټول_وزن_Exprp->dtype_سېټ_UInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ثابت صحيح distId = m_dist_عدد++;
            ثابت std::string ټول_Name = "__Vdist_total" + cvt_منزل_Str(distId);
            AstVar* ثابت ټول_Varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, ټول_Name, taskp->پیدا_کړه_UInt64_DType()};
            ټول_Varp->noSubst(ریښتیا);
            ټول_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ټول_Varp->funcLocal(ریښتیا);
            ټول_Varp->is_دننی(ریښتیا);
            جمع_کړه_Stmt(ټول_Varp);
            جمع_کړه_Stmt(
                جديد Ast_ټاکل{fl, جديد AstVarRef{fl, ټول_Varp, VAccess::WRITE}, ټول_وزن_Exprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ثابت std::string لوښی_Name = "__Vdist_bucket" + cvt_منزل_Str(distId);
            AstVar* ثابت لوښی_Varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, لوښی_Name, taskp->پیدا_کړه_UInt64_DType()};
            لوښی_Varp->noSubst(ریښتیا);
            لوښی_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            لوښی_Varp->funcLocal(ریښتیا);
            لوښی_Varp->is_دننی(ریښتیا);
            جمع_کړه_Stmt(لوښی_Varp);

            Ast_غوټه_بیان* ثابت randp = جديد AstRand{fl, تش_اشاره, خطأ};
            randp->dtype_سېټ_UInt64();
            جمع_کړه_Stmt(جديد Ast_ټاکل{
                fl, جديد AstVarRef{fl, لوښی_Varp, VAccess::WRITE},
                جديد Ast_جمع_کړه{
                    fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                    جديد AstModDiv{fl, randp, جديد AstVarRef{fl, ټول_Varp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::ویکټور<Ast_غوټه_بیان*> cumSums;
            Ast_غوټه_بیان* runningSump = تش_اشاره;
            لكل (اندازه_t i = 0; i < buckets.اندازه(); ++i) {
                إذا (!runningSump) {
                    runningSump = buckets[i].وزن_Exprp->clone_ونه_خالص(خطأ);
                } وإلا {
                    runningSump = جديد Ast_جمع_کړه{fl, runningSump,
                                             buckets[i].وزن_Exprp->clone_ونه_خالص(خطأ)};
                    runningSump->dtype_سېټ_UInt64();
                }
                cumSums.ټېل_شا(runningSump->clone_ونه_خالص(ریښتیا));
            }

            Ast_غوټه* chainp = build_Weighted_لوښی_Chain(distp, buckets, لوښی_Varp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, rand_ډېرپېښ_Varp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(ټېل_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->جمع_کړه_دلته_This_As_بل(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            لكل (خپلکاری& لوښی : buckets) {
                VL_DO_DANGLING(ټېل_Deletep(لوښی.وزن_Exprp), لوښی.وزن_Exprp);
            }
            VL_DO_DANGLING(ټېل_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            ټېل_Deletep(cumSums.شا());
        }
    }

    ساكن منطقي is_Dyn_Arr_د_Class_Type_Recurse(ثابت Ast_غوټه_DType* ثابت dtypep) {
        ثابت Ast_غوټه_DType* ثابت refp = dtypep->skipRefp();
        إذا (VN_IS(refp, DynArrayDType) || VN_IS(refp, کتار_DType)) {
            أرجع is_Dyn_Arr_د_Class_Type_Recurse(refp->subDTypep());
        } وإلا إذا (VN_IS(refp, ClassRefDType)) {
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    فراغ جمع_کړه_Array_Elem_Refresh(File_کرښه* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                             AstVar* ثابت genp) {
        Ast_غوټه_Module* ثابت genModp = VN_AS(genp->user2p(), غوټه_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(خطأ).دوهم;
        إذا (is_Dyn_Arr_د_Class_Type_Recurse(arrVarp->dtypep())) {
            إذا (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            أرجع;
        }
        AstCMethodHard* ثابت methodp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_سېټ_Void();

        Ast_غوټه_Module* ثابت classp = VN_AS(arrVarp->user2p(), غوټه_Module);
        AstVarRef* ثابت varRefp = جديد AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->جمع_کړه_Pinsp(varRefp);

        ثابت اندازه_t پلنوالی = arrayElementDTypep(arrVarp->dtypep())->پلنوالی();

        methodp->جمع_کړه_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, پلنوالی});
        Ast_غوټه_بیان* ثابت varnamep
            = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + arrVarp->name() + "\"", arrVarp->پلنوالی()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->جمع_کړه_Pinsp(varnamep);
        methodp->جمع_کړه_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ثابت Randomize_ډېرپېښ rmode = {.asUQuad = arrVarp->user1()};
        إذا (rmode.کاروي_ډېرپېښ) {
            methodp->جمع_کړه_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, rmode.شاخص});
        }

        randomizep->جمع_کړه_Stmtsp(methodp->makeStmt());
    }

    Ast_غوټه_Stmt* new_Pin_اندازه_Variable(File_کرښه* ثابت fl, AstVar* ثابت arrVarp,
                                    AstVar* ثابت genp) {
        Ast_غوټه_Module* ثابت genModp = VN_AS(genp->user2p(), غوټه_Module);
        AstVar* ثابت اندازه_Varp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ثابت pinp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtype_سېټ_Void();
        AstCExpr* ثابت namep = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + اندازه_Varp->name() + "\""};
        namep->dtype_سېټ_UInt32();
        pinp->جمع_کړه_Pinsp(namep);
        pinp->جمع_کړه_Pinsp(
            جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(اندازه_Varp->پلنوالی())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ثابت اندازه_Var_Refp = جديد AstVarRef{fl, اندازه_Varp, VAccess::READ};
        اندازه_Var_Refp->classOrPackagep(VN_AS(اندازه_Varp->user2p(), غوټه_Module));
        pinp->جمع_کړه_Pinsp(اندازه_Var_Refp);
        أرجع pinp->makeStmt();
    }

    فراغ جمع_کړه_Frozen_اندازه_Pin(File_کرښه* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                          AstVar* ثابت genp, AstVar* ثابت rand_ډېرپېښ_Varp) {
        ثابت Randomize_ډېرپېښ rmode = {.asUQuad = arrVarp->user1()};
        إذا (!rmode.کاروي_ډېرپېښ) أرجع;
        AstVar* ثابت اندازه_Varp = VN_AS(arrVarp->user4p(), Var);
        Ast_غوټه_Module* ثابت arrClassp = VN_AS(arrVarp->user2p(), غوټه_Module);
        Ast_غوټه_Module* ثابت اندازه_Classp = VN_AS(اندازه_Varp->user2p(), غوټه_Module);
        AstVarRef* ثابت اندازه_Writep = جديد AstVarRef{fl, اندازه_Classp, اندازه_Varp, VAccess::WRITE};
        ثابت VCMethod اندازه_لاره = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_اندازه
                                        : VCMethod::DYN_اندازه;
        AstCMethodHard* ثابت currentSizep = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, اندازه_لاره};
        currentSizep->dtypep(اندازه_Varp->dtypep());
        randomizep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, اندازه_Writep, currentSizep});
        Ast_غوټه_بیان* ثابت modep = new_ډېرپېښ_بټ_Read(arrVarp, تش_اشاره, rand_ډېرپېښ_Varp, fl);
        randomizep->جمع_کړه_Stmtsp(
            جديد AstIf{fl, جديد AstLogNot{fl, modep}, new_Pin_اندازه_Variable(fl, arrVarp, genp)});
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_عدد);
        VL_RESTORER_COPY(m_rand_Array_Unique_نومونه);
        m_modp = nodep;
        m_rand_Case_عدد = 0;
        m_rand_Array_Unique_نومونه.reset();
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_عدد);
        VL_RESTORER(m_dist_عدد);
        m_modp = nodep;
        m_rand_Case_عدد = 0;
        m_dist_عدد = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        إذا (!nodep->user1()) أرجع;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->بنسټ_Most_Classp()->needRNG(ریښتیا);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ثابت منطقي basic_لومړی = classOwnsGlobalConstraint(nodep);

        File_کرښه* ثابت fl = nodep->fileline();
        AstFunc* ثابت randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ثابت fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ثابت rand_ډېرپېښ_Varp = get_Rand_ډېرپېښ_Var_From_Class(nodep);
        جمع_کړه_Pre_Post_Call(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(nodep)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->جمع_کړه_Stmtsp((جديد AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_غوټه_بیان* beginValp = تش_اشاره;
        AstVar* genp = getRandomGenerator(nodep);
        إذا (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ثابت prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                إذا (!taskp) {
                    taskp = new_تیاری_Constraint_Task(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                إذا (AstTask* ثابت resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ثابت resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ثابت resizeTaskRefp
                        = جديد AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->جمع_کړه_Stmtsp(resizeTaskRefp->makeStmt());
                }

                إذا (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                إذا (constrp->itemsp()) {
                    ښکتنی_Dist_Constraints(taskp, constrp->itemsp(), rand_ډېرپېښ_Varp);
                }
                VInsertion_سېټ<AstVar*>& اندازه_Arrays = m_اندازه_Constrained_Arrays[classp];
                Constraint_بیان_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), تش_اشاره,
                    genp,          rand_ډېرپېښ_Varp, m_writtenVars,     &m_uniqueConstraintId,
                    m_unique_نومونه, randomizep,   &اندازه_Arrays,       prepareArraysTaskp};
                إذا (constrp->itemsp()) {
                    taskp->جمع_کړه_Stmtsp(wrap_If_Constraint_ډېرپېښ(
                        nodep, constrp, constrp->itemsp()->unlink_Fr_شا_With_بل()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ثابت خپلکاری constraint_ژوروالی = [](ثابت AstConstraint* constrp) {
                    صحيح ژوروالی = 0;
                    اندازه_t pos = 0;
                    طالما ((pos = constrp->name().پیدا_کړه(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++ژوروالی;
                        pos += std::مزي_اوږدوالی(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    أرجع ژوروالی;
                };
                صحيح max_ژوروالی = 0;
                nodep->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    max_ژوروالی = std::max(max_ژوروالی, constraint_ژوروالی(constrp));
                });
                AstTask* ثابت تیاری_All_Taskp = get_Create_Constraint_تیاری_Func(nodep, m_memberMap);
                لكل (صحيح d = max_ژوروالی; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                            إذا (constraint_ژوروالی(constrp) != d) أرجع;
                            AstTask* ثابت taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ثابت تیاری_Task_Refp
                                = جديد AstTaskRef{constrp->fileline(), taskp};
                            تیاری_Task_Refp->classOrPackagep(classp);
                            تیاری_All_Taskp->جمع_کړه_Stmtsp(تیاری_Task_Refp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertion_سېټ<AstVar*> اندازه_Array_Vars;
            إذا (nodep->extendsp()) {
                AstClass* پلار_Classp = nodep->extendsp()->classp();
                طالما (پلار_Classp) {
                    ثابت خپلکاری اندازه_Arrays_It = m_اندازه_Constrained_Arrays.پیدا_کړه(پلار_Classp);
                    إذا (اندازه_Arrays_It != m_اندازه_Constrained_Arrays.پای_ټکی()) {
                        لكل (AstVar* ثابت arrVarp : اندازه_Arrays_It->دوهم) {
                            اندازه_Array_Vars.ورننباسه(arrVarp);
                        }
                    }
                    AstFunc* ثابت پلار_Randomizep
                        = VN_CAST(m_memberMap.پیدا_کړه_Member(پلار_Classp, "randomize"), Func);
                    إذا (پلار_Randomizep && پلار_Randomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        لكل (Ast_غوټه* stmtp = پلار_Randomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            منطقي وموندل_Clear_Constraints = خطأ;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                إذا (methodp->لاره() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->لاره() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->جمع_کړه_Stmtsp(stmtp->clone_ونه(خطأ));
                                } وإلا إذا (methodp->لاره()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    وموندل_Clear_Constraints = ریښتیا;
                                }
                            });
                            إذا (وموندل_Clear_Constraints) اكسر;
                        }
                    }
                    پلار_Classp
                        = پلار_Classp->extendsp() ? پلار_Classp->extendsp()->classp() : تش_اشاره;
                }
            }
            randomizep->جمع_کړه_Stmtsp(implementConstraintsClear(fl, genp));

            ثابت خپلکاری اندازه_Arrays_It = m_اندازه_Constrained_Arrays.پیدا_کړه(nodep);
            إذا (اندازه_Arrays_It != m_اندازه_Constrained_Arrays.پای_ټکی()) {
                لكل (AstVar* ثابت arrVarp : اندازه_Arrays_It->دوهم) اندازه_Array_Vars.ورننباسه(arrVarp);
            }
            AstTask* ثابت تیاری_All_Taskp = get_Create_Constraint_تیاری_Func(nodep, m_memberMap);
            إذا (!اندازه_Array_Vars.تشه() && prepareArraysTaskp->stmtsp()) {
                Ast_غوټه_Stmt* ثابت preparep = (جديد AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                لكل (Ast_غوټه* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    منطقي writesArray = خطأ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->لاره() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    إذا (!writesArray) استمر;
                    stmtp->جمع_کړه_دلته_This_As_بل(preparep);
                    اكسر;
                }
            }
            AstTaskRef* ثابت تیاری_Task_Refp = جديد AstTaskRef{fl, تیاری_All_Taskp};
            randomizep->جمع_کړه_Stmtsp(تیاری_Task_Refp->makeStmt());

            Ast_غوټه_Module* ثابت genModp = VN_AS(genp->user2p(), غوټه_Module);

            AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
            solverCallp->dtype_سېټ_بټ();
            solverCallp->جمع_کړه(جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->جمع_کړه(".next(__Vm_rng)");
            إذا (!اندازه_Array_Vars.تشه()) {
                AstVar* ثابت اندازه_Ok_Varp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::بټ)};
                اندازه_Ok_Varp->funcLocal(ریښتیا);
                randomizep->جمع_کړه_Stmtsp(اندازه_Ok_Varp);

                AstVar* ثابت finalOkVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::بټ)};
                finalOkVarp->funcLocal(ریښتیا);
                randomizep->جمع_کړه_Stmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                لكل (AstVar* ثابت arrVarp : اندازه_Array_Vars)
                    جمع_کړه_Frozen_اندازه_Pin(fl, arrVarp, randomizep, genp, rand_ډېرپېښ_Varp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->جمع_کړه_Stmtsp(
                    جديد Ast_ټاکل{fl, جديد AstVarRef{fl, اندازه_Ok_Varp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                إذا (AstTask* ثابت resizeAllTaskp
                    = VN_AS(m_memberMap.پیدا_کړه_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ثابت resizeTaskRefp = جديد AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ثابت ifp = جديد AstIf{fl, جديد AstVarRef{fl, اندازه_Ok_Varp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), تش_اشاره};
                    randomizep->جمع_کړه_Stmtsp(ifp);
                }

                // Refresh array element tables after resize
                لكل (ثابت خپلکاری& arrVarp : اندازه_Array_Vars) {
                    جمع_کړه_Array_Elem_Refresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->جمع_کړه_Stmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ثابت تیاری_Task_Refp2 = جديد AstTaskRef{fl, تیاری_All_Taskp};
                randomizep->جمع_کړه_Stmtsp(تیاری_Task_Refp2->makeStmt());

                لكل (AstVar* ثابت arrVarp : اندازه_Array_Vars) {
                    randomizep->جمع_کړه_Stmtsp(new_Pin_اندازه_Variable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ثابت solverCallp2 = جديد AstCExpr{fl};
                solverCallp2->dtype_سېټ_بټ();
                solverCallp2->جمع_کړه(جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->جمع_کړه(".next(__Vm_rng)");
                randomizep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{
                    fl, جديد AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = جديد AstAnd{fl, جديد AstVarRef{fl, اندازه_Ok_Varp, VAccess::READ},
                                       جديد AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } وإلا {
                beginValp = solverCallp;
            }
            إذا (rand_ډېرپېښ_Varp) {
                Ast_غوټه_Module* ثابت rand_ډېرپېښ_Classp = VN_AS(rand_ډېرپېښ_Varp->user2p(), Class);
                Ast_غوټه_FTask* ثابت newp
                    = VN_AS(m_memberMap.پیدا_کړه_Member(rand_ډېرپېښ_Classp, "new"), غوټه_FTask);
                UASSERT_OBJ(newp, rand_ډېرپېښ_Classp, "No new() in class");
                جمع_کړه_سېټ_Rand_ډېرپېښ(newp, genp, rand_ډېرپېښ_Varp);
            }
            إذا (AstVar* ثابت static_Rand_ډېرپېښ_Varp = get_Static_Rand_ډېرپېښ_Var(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_غوټه_FTask* ثابت newp = VN_AS(m_memberMap.پیدا_کړه_Member(nodep, "new"), غوټه_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                جمع_کړه_سېټ_Static_Rand_ډېرپېښ(newp, genp, static_Rand_ډېرپېښ_Varp);
            }
        } وإلا {
            beginValp = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 32, 1};
        }

        جمع_کړه_Update_Rand_Vars_Body(nodep, randomizep);

        AstFunc* ثابت basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        جمع_کړه_Basic_Randomize_Body(basicRandomizep, nodep, rand_ډېرپېښ_Varp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ثابت fvarRefp = جديد AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{
            fl, fvarRefp, basic_لومړی ? جديد AstFuncRef{fl, basicRandomizep} : beginValp});

        ثابت خپلکاری اندازه_Arrays_It = m_اندازه_Constrained_Arrays.پیدا_کړه(nodep);
        ثابت منطقي needs_اندازه_Phase
            = اندازه_Arrays_It != m_اندازه_Constrained_Arrays.پای_ټکی() && !اندازه_Arrays_It->دوهم.تشه();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ثابت خپلکاری emitResizeFallback = [&]() {
            إذا (needs_اندازه_Phase) أرجع;
            إذا (AstTask* ثابت resizeAllTaskp
                = VN_AS(m_memberMap.پیدا_کړه_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ثابت resizeTaskRefp = جديد AstTaskRef{fl, resizeAllTaskp};
                randomizep->جمع_کړه_Stmtsp(resizeTaskRefp->makeStmt());
            }
        };
        إذا (!basic_لومړی) emitResizeFallback();

        AstVarRef* ثابت fvarRefReadp = fvarRefp->clone_ونه(خطأ);
        fvarRefReadp->access(VAccess::READ);

        Ast_غوټه_بیان* ثابت دوهم_Halfp
            = basic_لومړی ? beginValp : جديد AstFuncRef{fl, basicRandomizep};
        randomizep->جمع_کړه_Stmtsp(جديد Ast_ټاکل{fl, fvarRefp->clone_ونه(خطأ),
                                            جديد AstAnd{fl, fvarRefReadp, دوهم_Halfp}});

        إذا (basic_لومړی) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(nodep)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->جمع_کړه_Stmtsp((جديد AstTaskRef{fl, postTaskp})->makeStmt());
        }

        جمع_کړه_Pre_Post_Call(nodep, randomizep, "post_randomize");
        nodep->user1(خطأ);
    }
    فراغ لیدنه(AstRandCase* nodep) override {
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
        Ast_غوټه_DType* ثابت مجموعه_DTypep = nodep->پیدا_کړه_UInt64_DType();

        File_کرښه* ثابت fl = nodep->fileline();
        ثابت std::string name = "__Vrandcase" + cvt_منزل_Str(m_rand_Case_عدد++);
        AstVar* ثابت randVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, name, مجموعه_DTypep};
        randVarp->noSubst(ریښتیا);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        جمع_کړه_لنډمهاله_Varsp(randVarp);
        Ast_غوټه_بیان* sump = جديد AstConst{fl, AstConst::Widthed_ارزښت{}, 64, 0};
        Ast_غوټه_If* ثابت لومړی_Ifsp
            = جديد AstIf{fl, جديد AstConst{fl, AstConst::بټ_False{}}, تش_اشاره, تش_اشاره};
        Ast_غوټه_If* ifsp = لومړی_Ifsp;

        لكل (Ast_Case_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_توکی)) {
            Ast_غوټه_بیان* ثابت condp = itemp->condsp()->unlink_Fr_شا();
            sump
                = جديد Ast_جمع_کړه{condp->fileline(), sump, جديد AstExtend{itemp->fileline(), condp, 64}};
            Ast_غوټه* ثابت stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlink_Fr_شا_With_بل() : تش_اشاره;
            AstVarRef* ثابت randVarRefp = جديد AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_غوټه_If* ثابت newifp
                = جديد AstIf{itemp->fileline(),
                            جديد AstLte{condp->fileline(), randVarRefp, sump->clone_ونه_خالص(ریښتیا)},
                            stmtsp, تش_اشاره};
            ifsp->جمع_کړه_Elsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ثابت dispp = جديد AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            تش_اشاره, تش_اشاره};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->جمع_کړه_Elsesp(dispp);

        Ast_غوټه_بیان* ثابت randp = جديد AstRand{fl, تش_اشاره, خطأ};
        randp->dtype_سېټ_UInt64();
        AstVarRef* ثابت randVarRefp = جديد AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_غوټه* ثابت newp
            = جديد Ast_ټاکل{fl, randVarRefp,
                            جديد Ast_جمع_کړه{fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                                       جديد AstModDiv{fl, randp, sump}}};
        newp->جمع_کړه_بل(لومړی_Ifsp);
        إذا (debug() >= 9) newp->dump_ونه_And_بل(وتنه, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        إذا (nodep->name() == "rand_mode") {
            Ast_لاره_Call* ثابت لاره_Callp = VN_CAST(nodep, لاره_Call);
            Ast_غوټه_بیان* ثابت fromp = لاره_Callp ? لاره_Callp->fromp() : تش_اشاره;
            ثابت Rand_ډېرپېښ_موخه rand_ډېرپېښ_موخه = Rand_ډېرپېښ_موخه::get(fromp, m_modp);
            UASSERT_OBJ(rand_ډېرپېښ_موخه.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ثابت receiverp = rand_ډېرپېښ_موخه.receiverp;
            ثابت منطقي isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_غوټه* classLevelStaticLoopp = تش_اشاره;
            إذا (isClassLevel && nodep->argsp()) {
                إذا (AstVar* ثابت sVarp = get_Static_Rand_ډېرپېښ_Var(rand_ډېرپېښ_موخه.classp)) {
                    File_کرښه* ثابت fl = nodep->fileline();
                    Ast_غوټه_بیان* ثابت staticLhsp
                        = make_ډېرپېښ_ټاکل_Lhs(fl, rand_ډېرپېښ_موخه.classp, تش_اشاره, sVarp);
                    Ast_غوټه_بیان* ثابت argClonep = nodep->argsp()->exprp()->clone_ونه_خالص(خطأ);
                    classLevelStaticLoopp = make_ډېرپېښ_سېټ_Loop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ثابت منطقي receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ثابت rand_ډېرپېښ_Varp = receiverIsStaticRand
                                             ? get_Static_Rand_ډېرپېښ_Var(rand_ډېرپېښ_موخه.classp)
                                             : get_Rand_ډېرپېښ_Var_From_Class(rand_ډېرپېښ_موخه.classp);
            إذا (!rand_ډېرپېښ_Varp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_بیان), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(ټېل_Deletep(m_stmtp), m_stmtp = تش_اشاره);
                أرجع;
            }
            Ast_غوټه_بیان* ثابت lhsp = make_ډېرپېښ_ټاکل_Lhs(nodep->fileline(), rand_ډېرپېښ_موخه.classp,
                                                        rand_ډېرپېښ_موخه.fromp, rand_ډېرپېښ_Varp);
            replace_With_ډېرپېښ_ټاکل_And_Append(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : تش_اشاره, lhsp,
                classLevelStaticLoopp);
            أرجع;
        }

        إذا (nodep->name() == "constraint_mode") {
            Ast_لاره_Call* ثابت لاره_Callp = VN_CAST(nodep, لاره_Call);
            Ast_غوټه_بیان* fromp = لاره_Callp ? لاره_Callp->fromp() : تش_اشاره;
            AstConstraint* constrp = تش_اشاره;
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
            AstVar* ثابت constraint_ډېرپېښ_Varp = (constrp && constrp->isStatic())
                                                   ? get_Static_Constraint_ډېرپېښ_Var(classp)
                                                   : get_Constraint_ډېرپېښ_Var(classp);
            Ast_غوټه_بیان* ثابت lhsp
                = make_ډېرپېښ_ټاکل_Lhs(nodep->fileline(), classp, fromp, constraint_ډېرپېښ_Varp);
            replace_With_ډېرپېښ_ټاکل(nodep, constrp, lhsp);
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
            ثابت منطقي دننه_Static_Context = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ثابت stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ثابت randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            إذا (دننه_Static_Context) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(ریښتیا);
            }
            randomizeFuncp->جمع_کړه_Stmtsp(
                جديد Ast_ټاکل{nodep->fileline(),
                              جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              جديد AstConst{nodep->fileline(), AstConst::Widthed_ارزښت{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            صحيح argn = 0;
            AstWith* ثابت withp = nodep->withp();
            لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_غوټه_بیان* ثابت exprp = argp->exprp();
                AstCMethodHard* ثابت basicMethodp = جديد AstCMethodHard{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ثابت refvarp
                    = جديد AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::منزل_string(++argn), exprp->dtypep()};
                refvarp->سېټ_Std_Randomize_Arg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                إذا (withp) {
                    withp->foreach([&](Ast_غوټه_بیان* توانیز_تابع) {
                        إذا (is_Similar_غوټه(توانیز_تابع, exprp)) {
                            AstVarRef* ثابت replaceVar
                                = جديد AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            توانیز_تابع->replaceWith(replaceVar);
                            replaceVar->user1(توانیز_تابع->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(ټېل_Deletep(توانیز_تابع), توانیز_تابع);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(ریښتیا);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->جمع_کړه_Stmtsp(refvarp);

                ثابت اندازه_t پلنوالی = exprp->پلنوالی();
                basicMethodp->جمع_کړه_Pinsp(
                    جديد AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->جمع_کړه_Pinsp(
                    جديد AstConst{nodep->fileline(), AstConst::Unsized64{}, پلنوالی});
                basicMethodp->dtype_سېټ_بټ();

                randomizeFuncp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    جديد AstAnd{nodep->fileline(),
                               جديد AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            إذا (withp) {
                File_کرښه* ثابت fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, تش_اشاره,
                                                                دننه_Static_Context);
                withCapturep->جمع_کړه_Function_Arguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                إذا (stdrand) {
                    randomizeFuncp->جمع_کړه_Stmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_غوټه* ثابت capturedTreep = withp->exprp()->unlink_Fr_شا_With_بل();
                randomizeFuncp->جمع_کړه_Stmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_بیان_Visitor{
                        تش_اشاره, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        تش_اشاره, m_writtenVars, &m_uniqueConstraintId, m_unique_نومونه,  تش_اشاره};
                }
                AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
                solverCallp->dtype_سېټ_بټ();
                solverCallp->جمع_کړه(جديد AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->جمع_کړه(".next()");
                AstVar* ثابت fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_غوټه_بیان* ثابت andExprp
                    = جديد AstAnd{fl, جديد AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->جمع_کړه_Stmtsp(
                    جديد Ast_ټاکل{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlink_Fr_شا()->delete_ونه(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            إذا (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            إذا (withCapturep) nodep->جمع_کړه_Argsp(withCapturep->get_دلیلونه());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            أرجع;
        }
        handle_Randomize_دلیلونه(nodep);

        AstWith* ثابت withp = nodep->withp();
        إذا (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            أرجع;
        }
        withp->unlink_Fr_شا();
        iterateChildren(nodep);

        AstClass* classp = تش_اشاره;
        إذا (Ast_لاره_Call* ثابت callp = VN_CAST(nodep, لاره_Call)) {
            ثابت Ast_غوټه_DType* ثابت fromDTypep = callp->fromp()->dtypep();
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
                         classp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(ریښتیا);

        AstFunc* ثابت randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inline_Unique_نومونه.get(nodep), خطأ);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ثابت std::pair<منطقي, منطقي> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<منطقي, منطقي>{خطأ, خطأ};
        إذا (prePostWrap.لومړی) {
            جمع_کړه_Virtual_Pre_Post_Call(randomizeFuncp, classp, "pre_randomize");
        } وإلا {
            جمع_کړه_Pre_Post_Call(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(classp)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(classp, "pre");
            إذا (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->جمع_کړه_Stmtsp((جديد AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ثابت CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.جمع_کړه_Function_Arguments(randomizeFuncp);

        // Add constraints clearing code
        إذا (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ثابت mainRandomizep
                = VN_CAST(m_memberMap.پیدا_کړه_Member(classp, "randomize"), Func);
            إذا (mainRandomizep && mainRandomizep->stmtsp()) {
                لكل (Ast_غوټه* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    منطقي وموندل_Clear_Constraints = خطأ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        إذا (methodp->لاره() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->لاره() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->جمع_کړه_Stmtsp(stmtp->clone_ونه(خطأ));
                        } وإلا إذا (methodp->لاره() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            وموندل_Clear_Constraints = ریښتیا;
                        }
                    });
                    إذا (وموندل_Clear_Constraints) اكسر;
                }
            }
            randomizeFuncp->جمع_کړه_Stmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->جمع_کړه_Stmtsp(localGenp);

        AstFunc* ثابت basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ثابت basicRandomizeFuncCallp
            = جديد AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        إذا (classGenp) {
            AstTask* ثابت constr_تیاری_Funcp = get_Create_Constraint_تیاری_Func(classp, m_memberMap);
            AstTaskRef* ثابت callp = جديد AstTaskRef{nodep->fileline(), constr_تیاری_Funcp};
            randomizeFuncp->جمع_کړه_Stmtsp(callp->makeStmt());
            randomizeFuncp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                جديد AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), غوټه_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ثابت rand_ډېرپېښ_Varp = get_Rand_ډېرپېښ_Var_From_Class(classp);
        إذا (!classGenp && rand_ډېرپېښ_Varp) جمع_کړه_سېټ_Rand_ډېرپېښ(randomizeFuncp, localGenp, rand_ډېرپېښ_Varp);
        إذا (!classGenp) {
            إذا (AstVar* ثابت sVarp = get_Static_Rand_ډېرپېښ_Var(classp)) {
                جمع_کړه_سېټ_Static_Rand_ډېرپېښ(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_غوټه* ثابت capturedTreep = withp->exprp()->unlink_Fr_شا_With_بل();
        randomizeFuncp->جمع_کړه_Stmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_غوټه* inlineResizeStmtsp = تش_اشاره;
        {
            std::ویکټور<AstCMethodHard*> اندازه_Methodps;
            capturedTreep->foreach_And_بل([&](AstCMethodHard* methodp) {
                إذا (methodp->لاره() == VCMethod::DYN_اندازه
                    || methodp->لاره() == VCMethod::ASSOC_اندازه) {
                    اندازه_Methodps.ټېل_شا(methodp);
                }
            });
            لكل (AstCMethodHard* ثابت methodp : اندازه_Methodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = تش_اشاره;
                Ast_غوټه_بیان* ثابت ریښه_Exprp = methodp->fromp();
                إذا (AstMemberSel* memberSelp = VN_CAST(ریښه_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    طالما (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ثابت Ast_غوټه_بیان* ثابت fromp = memberSelp->fromp();
                    إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(fromp, VarRef)) {
                        إذا (varRefp->varp() == captured.getThisp()) استمر;
                    } وإلا {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } وإلا إذا (AstVarRef* ثابت varRefp = VN_CAST(ریښه_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                إذا (!arrVarp) استمر;
                // Only handle rand-declared dynamic/assoc array variables
                إذا (!arrVarp->rand().isRandomizable()) استمر;
                // Do not randomize associative array size (IEEE 18.4)
                إذا (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) استمر;
                File_کرښه* ثابت fl = methodp->fileline();
                منطقي wasCreated = خطأ;
                AstClass* ثابت varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ثابت اندازه_Varp = create_Or_Get_اندازه_Var(varClassp, arrVarp, fl,
                                                            methodp->پیدا_کړه_Int_DType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                إذا (wasCreated) {
                    Ast_غوټه_بیان* ثابت resizeVarRefp = buildMemberSelExprp(ریښه_Exprp, arrVarp);
                    Ast_غوټه_بیان* ثابت اندازه_Exprp = buildMemberSelExprp(ریښه_Exprp, اندازه_Varp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->جمع_کړه_بل(create_اندازه_Gte_Zero_Constraint(fl, اندازه_Varp, ریښه_Exprp));
                    AstCMethodHard* ثابت resizep
                        = جديد AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, اندازه_Exprp};
                    resizep->dtypep(methodp->پیدا_کړه_Void_DType());
                    inlineResizeStmtsp
                        = Ast_غوټه::جمع_کړه_بل(inlineResizeStmtsp, جديد Ast_Stmt_بیان{fl, resizep});
                }
                Ast_غوټه_بیان* ثابت اندازه_غوټه_Exprp = buildMemberSelExprp(ریښه_Exprp, اندازه_Varp);
                methodp->replaceWith(اندازه_غوټه_Exprp);
                VL_DO_DANGLING(methodp->delete_ونه(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_بیان_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                rand_ډېرپېښ_Varp, m_writtenVars, &m_uniqueConstraintId, m_unique_نومونه,  تش_اشاره};
        }

        // Call the solver and set return value
        AstCExpr* ثابت solverCallp = جديد AstCExpr{nodep->fileline()};
        solverCallp->dtype_سېټ_بټ();
        solverCallp->جمع_کړه(جديد AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->جمع_کړه(".next(__Vm_rng)");
        randomizeFuncp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            جديد AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        إذا (inlineResizeStmtsp) {
            AstVar* ثابت retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ثابت ifp = جديد AstIf{nodep->fileline(),
                                         جديد AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, تش_اشاره};
            randomizeFuncp->جمع_کړه_Stmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(classp)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(classp, "post");
            إذا (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->جمع_کړه_Stmtsp((جديد AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        إذا (prePostWrap.دوهم) {
            جمع_کړه_Virtual_Pre_Post_Call(randomizeFuncp, classp, "post_randomize");
        } وإلا {
            جمع_کړه_Pre_Post_Call(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->جمع_کړه_Argsp(captured.get_دلیلونه());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_ونه(), withp);
    }
    فراغ لیدنه(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_کرښه* ثابت fl = nodep->fileline();
        إذا (m_constraintp && nodep->fromp()->user1()
            && (nodep->لاره() == VCMethod::ASSOC_اندازه
                || nodep->لاره() == VCMethod::DYN_اندازه)) {
            AstClass* ثابت classp = VN_AS(m_modp, Class);
            AstVarRef* ثابت کتار_Var_Refp = VN_CAST(nodep->fromp(), VarRef);
            إذا (!کتار_Var_Refp) {
                // Warning from ConstraintExprVisitor will be thrown
                أرجع;
            }
            AstVar* ثابت کتار_Varp = کتار_Var_Refp->varp();
            منطقي wasCreated = خطأ;
            AstVar* ثابت اندازه_Varp
                = create_Or_Get_اندازه_Var(classp, کتار_Varp, fl, nodep->پیدا_کړه_Int_DType(), wasCreated);
            إذا (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                إذا (!VN_IS(کتار_Varp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    إذا (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ثابت اندازه_Var_Refp = جديد AstVarRef{
                        fl, VN_AS(اندازه_Varp->user2p(), غوټه_Module), اندازه_Varp, VAccess::READ};
                    AstCMethodHard* ثابت resizep = جديد AstCMethodHard{
                        fl, nodep->fromp()->unlink_Fr_شا(), VCMethod::DYN_RESIZE, اندازه_Var_Refp};
                    resizep->dtypep(nodep->پیدا_کړه_Void_DType());
                    resizerTaskp->جمع_کړه_Stmtsp(جديد Ast_Stmt_بیان{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->جمع_کړه_Itemsp(create_اندازه_Gte_Zero_Constraint(fl, اندازه_Varp));
            }
            AstVarRef* ثابت اندازه_Var_Refp = جديد AstVarRef{fl, VN_AS(اندازه_Varp->user2p(), غوټه_Module),
                                                         اندازه_Varp, VAccess::READ};
            اندازه_Var_Refp->user1(ریښتیا);
            nodep->replaceWith(اندازه_Var_Refp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند RandomizeVisitor(AstNetlist* nodep)
        : m_inline_Unique_نومونه{"__Vrandwith"} {
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
                    إذا (VN_AS(varp->user2p(), غوټه_Module) != ownerClassp) أرجع;
                    إذا (!varp->user3()) varp->user3(ریښتیا);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(ټېل_Deletep(constrp->unlink_Fr_شا()), constrp);
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
    V3Global::dump_Check_Global_ونه("randomize", 0, dump_ونه_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ثابت std::string& name, منطقي allowVirtual,
                                       منطقي ماشوم_DType) {
    AstFunc* funcp = VN_AS(memberMap.پیدا_کړه_Member(nodep, name), Func);
    إذا (!funcp) {
        v3Global.useRandomizeMethods(ریښتیا);
        Ast_غوټه_DType* ثابت dtypep
            = ماشوم_DType
                  ? جديد AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->پیدا_کړه_بټ_DType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ثابت fvarp = ماشوم_DType
                                  ? جديد AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlag_ماشوم_DType{}, dtypep}
                                  : جديد AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3_Error_کوډ::NORETURN, ریښتیا);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(ریښتیا);
        fvarp->funcReturn(ریښتیا);
        fvarp->direction(VDirection::OUTPUT);
        nodep->جمع_کړه_Membersp(funcp);
        funcp = جديد AstFunc{nodep->fileline(), name, تش_اشاره, fvarp};
        إذا (!ماشوم_DType) funcp->dtypep(dtypep);
        funcp->class_لاره(ریښتیا);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->جمع_کړه_Membersp(funcp);
        memberMap.ورننباسه(nodep, funcp);
    }
    أرجع funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_غوټه_Module* nodep, ثابت std::string& name) {
    AstFunc* funcp = تش_اشاره;
    v3Global.useRandomizeMethods(ریښتیا);
    Ast_غوټه_DType* ثابت dtypep = nodep->پیدا_کړه_بټ_DType(32, 32, VSigning::SIGNED);
    AstVar* ثابت fvarp = جديد AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(ریښتیا);
    fvarp->funcReturn(ریښتیا);
    fvarp->direction(VDirection::OUTPUT);
    funcp = جديد AstFunc{nodep->fileline(), name, تش_اشاره, fvarp};
    funcp->dtypep(dtypep);
    إذا (VN_IS(nodep, Class)) {
        funcp->class_لاره(ریښتیا);
    } وإلا {
        funcp->class_لاره(ریښتیا);
        funcp->isStatic(ریښتیا);
    }
    nodep->جمع_کړه_Stmtsp(funcp);
    أرجع funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ثابت basep = nodep->بنسټ_Most_Classp();
    AstFunc* funcp = VN_AS(memberMap.پیدا_کړه_Member(basep, "srandom"), Func);
    إذا (!funcp) {
        v3Global.useRandomizeMethods(ریښتیا);
        Ast_غوټه_DType* ثابت dtypep
            = basep->پیدا_کړه_بټ_DType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ثابت ivarp = جديد AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(ریښتیا);
        ivarp->direction(VDirection::ننوت);
        funcp = جديد AstFunc{basep->fileline(), "srandom", ivarp, تش_اشاره};
        funcp->dtypep(basep->پیدا_کړه_Void_DType());
        funcp->class_لاره(ریښتیا);
        funcp->isVirtual(خطأ);
        basep->جمع_کړه_Membersp(funcp);
        memberMap.ورننباسه(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        إذا (basep->name() == "process") {
            funcp->جمع_کړه_Stmtsp(جديد AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } وإلا {
            funcp->جمع_کړه_Stmtsp(جديد AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(ریښتیا);
        }
    }
    أرجع funcp;
}
