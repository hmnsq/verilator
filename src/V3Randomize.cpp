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
ساكن AstVar* get_Rand_گهڻائي_Var_From_Class(Ast_ڳنڍ_Module* classp) {
    طالما (classp) {
        إذا (classp->user2p()) أرجع VN_AS(classp->user2p(), Var);
        AstClass* ثابت cp = VN_CAST(classp, Class);
        إذا (!cp || !cp->extendsp()) أرجع خالي_اشارو;
        classp = cp->extendsp()->classp();
    }
    أرجع خالي_اشارو;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ساكن AstVar* getRandomGenerator(AstClass* ثابت classp) {
    إذا (classp->user3p()) أرجع VN_AS(classp->user3p(), Var);
    إذا (classp->extendsp()) أرجع getRandomGenerator(classp->extendsp()->classp());
    أرجع خالي_اشارو;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ساكن AstTask* get_Create_Constraint_تياري_Func(AstClass* classp, VMemberMap& memberMap) {
    ساكن ثابت حرف* ثابت name = "__Vsetup_constraints";
    AstTask* تياري_All_Taskp = VN_AS(memberMap.ڳوليو_Member(classp, name), Task);
    إذا (تياري_All_Taskp) أرجع تياري_All_Taskp;
    تياري_All_Taskp = جديد AstTask{classp->fileline(), "__Vsetup_constraints", خالي_اشارو};
    تياري_All_Taskp->class_طريقو(سچ);
    تياري_All_Taskp->isVirtual(سچ);
    classp->جوڙيو_Membersp(تياري_All_Taskp);
    memberMap.وجهو(classp, تياري_All_Taskp);
    أرجع تياري_All_Taskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

بنية Rand_گهڻائي_هدف final {
    // MEMBERS
    AstVar* ثابت receiverp;  // Variable that is the target of the rand_mode() call
    Ast_ڳنڍ_اظهار* ثابت fromp;  // Expression from which the rand_mode() call originates
    AstClass* ثابت classp;  // Class in which rand_mode should be set

    // METHODS
    ساكن Rand_گهڻائي_هدف get(Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_Module* modp) {
        إذا (!fromp) أرجع {خالي_اشارو, خالي_اشارو, VN_AS(modp, Class)};
        إذا (AstCMethodHard* ثابت طريقو_Hardp = VN_CAST(fromp, CMethodHard)) {
            أرجع Rand_گهڻائي_هدف::get(طريقو_Hardp->fromp(), modp);
        }
        AstVar* receiverp = خالي_اشارو;
        AstClass* classp = VN_CAST(modp, Class);
        إذا (AstVarRef* ثابت varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            إذا (receiverp->isRand()) {
                fromp = خالي_اشارو;
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

اتحاد Randomize_گهڻائي final {
    // MEMBERS
    بنية {
        منطقي استعمال_ڪري_گهڻائي : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t اشاريو : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ساكن Ast_ڳنڍ_DType* arrayElementDTypep(Ast_ڳنڍ_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    طالما (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    أرجع dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ساكن منطقي dtype_شامل_آهي_Real(Ast_ڳنڍ_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    إذا (Ast_ڳنڍ_DType* ثابت subp = dtypep->subDTypep()) أرجع dtype_شامل_آهي_Real(subp);
    إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, ڳنڍ_UOr_Struct_DType)) {
        إذا (structp->packed()) أرجع خطأ;
        لكل (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            إذا (dtype_شامل_آهي_Real(memberp->subDTypep())) أرجع سچ;
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
ساكن فراغ checkRandTypeEligibility(Ast_ڳنڍ* contextp, Ast_ڳنڍ_DType* dtypep, منطقي isRandc) {
    dtypep = dtypep->skipRefp();
    إذا (Ast_ڳنڍ_DType* ثابت subp = dtypep->subDTypep()) {
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
    إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, ڳنڍ_UOr_Struct_DType)) {
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
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;

    استعمال Derived_سيٽ = std::unordered_سيٽ<AstClass*>;
    استعمال بنياد_منزل_Derived_Map = std::unordered_map<ثابت AstClass*, Derived_سيٽ>;

    بنياد_منزل_Derived_Map m_بنياد_منزل_Derived_Map;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = خالي_اشارو;  // Current class
    Ast_ڳنڍ* m_constraint_اظهار_Genp = خالي_اشارو;  // Current constraint or constraint if expression
    Ast_ڳنڍ_Module* m_modp;  // Current module
    Ast_ڳنڍ_Stmt* m_stmtp = خالي_اشارو;  // Current statement
    Ast_ڳنڍ_FTask_Ref* m_stdRandCallp = خالي_اشارو;  // Current std::randomize() call
    منطقي m_اندريون_Std_With = خطأ;  // True when inside a 'with {}' clause
    std::سيٽ<Ast_ڳنڍ_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = خالي_اشارو;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    فراغ markGlobalConstrainedVars(AstClass* classp) {
        لكل (ثابت AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : خالي_اشارو) {
            لكل (Ast_ڳنڍ* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                إذا (!varp) استمر;
                إذا (varp->rand().isRandomizable()) varp->globalConstrained(سچ);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    منطقي is_Var_اندريون_Std_Randomize_دليلون(ثابت AstVar* varp) ثابت {
        إذا (!m_اندريون_Std_With || !m_stdRandCallp) أرجع خطأ;

        لكل (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ثابت Ast_ڳنڍ_اظهار* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            طالما (exprp) {
                إذا (ثابت Ast_ڳنڍ_Var_Ref* ثابت varrefp = VN_CAST(exprp, ڳنڍ_Var_Ref)) {
                    إذا (varrefp->varp() == varp) أرجع سچ;
                    اكسر;
                }
                إذا (ثابت AstMemberSel* ثابت memberselp = VN_CAST(exprp, MemberSel)) {
                    إذا (memberselp->varp() == varp) أرجع سچ;
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
             classp = classp->extendsp() ? classp->extendsp()->classp() : خالي_اشارو) {
            لكل (Ast_ڳنڍ* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                إذا (!varp) استمر;
                // If member is randomizable and of class type, mark its class
                إذا (varp->rand().isRandomizable()) {
                    ثابت Ast_ڳنڍ_DType* ثابت varDtypep = varp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    إذا (!classRefp) {
                        ثابت Ast_ڳنڍ_DType* subDTypep = varDtypep->subDTypep();
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
                        Randomize_گهڻائي rand_گهڻائي = {};
                        rand_گهڻائي.استعمال_ڪري_گهڻائي = سچ;
                        varp->user1(rand_گهڻائي.asUQuad);
                    }
                }
            }
        }
    }
    فراغ markDerived(ثابت AstClass* nodep) {
        ثابت خودڪار it = m_بنياد_منزل_Derived_Map.ڳوليو(nodep);
        إذا (it != m_بنياد_منزل_Derived_Map.پڇاڙي()) {
            لكل (خودڪار* classp : it->ٻيو) {
                إذا (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    فراغ markAllDerived() {
        لكل (ثابت خودڪار& p : m_بنياد_منزل_Derived_Map) {
            إذا (p.پهريون->user1()) markDerived(p.پهريون);
        }
    }
    فراغ سيٽ_Package_Refs() {
        لكل (Ast_ڳنڍ_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), ڳنڍ_Module));
        }
    }
    فراغ markNestedGlobalConstrainedRecurse(Ast_ڳنڍ* nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            AstVar* ثابت varp = refp->varp();
            إذا (varp->globalConstrained()) أرجع;
            varp->globalConstrained(سچ);
        } وإلا إذا (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(nodep, MemberSel)) {
            إذا (memberSelp->varp()) {
                AstVar* ثابت varp = memberSelp->varp();
                إذا (varp->globalConstrained()) أرجع;
                varp->globalConstrained(سچ);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_ڳنڍ_اظهار* buildMemberSelChain(AstVarRef* پاڙ_Var_Refp, ثابت std::ويڪٽر<AstVar*>& رستو) {
        Ast_ڳنڍ_اظهار* exprp = پاڙ_Var_Refp->clone_وڻ(خطأ);
        لكل (AstVar* memberVarp : رستو) {
            AstMemberSel* memberSelp
                = جديد AstMemberSel{پاڙ_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        أرجع exprp;
    }

    // Process a single constraint during nested constraint cloning
    فراغ processNestedConstraint(AstConstraint* ثابت constrp, AstVarRef* پاڙ_Var_Refp,
                                 ثابت std::ويڪٽر<AstVar*>& new_رستو, AstClass* هدف_Classp) {
        std::string رستو_اڳياڙي = پاڙ_Var_Refp->name();
        لكل (AstVar* رستو_Member_Varp : new_رستو) {
            رستو_اڳياڙي += GLOBAL_CONSTRAINT_SEPARATOR + رستو_Member_Varp->name();
        }

        ثابت std::string newName = رستو_اڳياڙي + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        منطقي isDuplicate = خطأ;
        هدف_Classp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت existingConstrp) {
            إذا (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = سچ;
            }
        });
        إذا (isDuplicate) أرجع;

        AstConstraint* ثابت cloneConstrp = constrp->clone_وڻ(خطأ);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::ويڪٽر<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_ڳنڍ_اظهار* exprp) {
            إذا (AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
                إذا (!varRefp->varp()->isClassMember()) أرجع;
                Ast_ڳنڍ_اظهار* ثابت chainp = buildMemberSelChain(پاڙ_Var_Refp, new_رستو);
                AstMemberSel* ثابت finalSelp
                    = جديد AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(ڌڪو_Deletep(varRefp), varRefp);
            } وإلا إذا (AstFuncRef* ثابت funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                إذا (funcRefp->taskp()->class_طريقو() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.ڌڪو_پٺ(funcRefp);
                }
            }
        });
        لكل (AstFuncRef* funcRefp : funcRefps) {
            Ast_ڳنڍ_اظهار* ثابت chainp = buildMemberSelChain(پاڙ_Var_Refp, new_رستو);
            AstArg* ثابت argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlink_Fr_پٺ_With_ايندڙ() : خالي_اشارو;
            Ast_طريقو_Call* ثابت callp
                = جديد Ast_طريقو_Call{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(ڌڪو_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        هدف_Classp->جوڙيو_Stmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    فراغ cloneNestedConstraintsRecurse(AstVarRef* پاڙ_Var_Refp, AstClass* classp,
                                       ثابت std::ويڪٽر<AstVar*>& رستو_منزل_Class,
                                       AstClass* هدف_Classp) {
        لكل (Ast_ڳنڍ* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ثابت memberVarp = VN_CAST(memberNodep, Var);
            إذا (!memberVarp) استمر;
            إذا (!memberVarp->rand().isRandomizable()) استمر;
            ثابت AstClassRefDType* ثابت memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            إذا (!memberClassRefp || !memberClassRefp->classp()) استمر;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::ويڪٽر<AstVar*> new_رستو = رستو_منزل_Class;
            new_رستو.ڌڪو_پٺ(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ثابت containingClassp, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, پاڙ_Var_Refp, new_رستو, هدف_Classp);
                });
            cloneNestedConstraintsRecurse(پاڙ_Var_Refp, nestedClassp, new_رستو, هدف_Classp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    فراغ handle_Randomize_دليل(Ast_ڳنڍ_اظهار* exprp, AstVar* ثابت varp, ثابت منطقي stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        طالما (exprp) {
            AstVar* randVarp = خالي_اشارو;
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
                varrefp->user1(سچ);
                varrefp->access(VAccess::READWRITE);
                exprp = خالي_اشارو;
            } وإلا {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            إذا (randVarp == varp) أرجع;
            Ast_ڳنڍ* backp = randVarp;
            طالما (backp && !VN_IS(backp, ڳنڍ_Module)) backp = backp->backp();
            إذا (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } وإلا {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                Randomize_گهڻائي rand_گهڻائي = {};
                rand_گهڻائي.استعمال_ڪري_گهڻائي = سچ;
                randVarp->user1(rand_گهڻائي.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    فراغ گهمو(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        إذا (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ثابت AstClass* ثابت basep = nodep->extendsp()->classp();
            m_بنياد_منزل_Derived_Map[basep].وجهو(nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->backp()) VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        إذا (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        إذا (nodep->name() == "rand_mode") {
            Ast_طريقو_Call* ثابت طريقو_Callp = VN_CAST(nodep, طريقو_Call);
            Ast_ڳنڍ_اظهار* ثابت fromp = طريقو_Callp ? طريقو_Callp->fromp() : خالي_اشارو;
            منطقي valid = سچ;
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
            } وإلا إذا (AstCMethodHard* ثابت طريقو_Hardp = VN_CAST(fromp, CMethodHard)) {
                إذا (طريقو_Hardp->طريقو() == VCMethod::ARRAY_AT
                    || طريقو_Hardp->طريقو() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = خطأ;
                } وإلا {
                    طريقو_Hardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            إذا (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_اظهار)
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_ڪوڊ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = خطأ;
            }
            إذا (valid) {
                ثابت Rand_گهڻائي_هدف rand_گهڻائي_هدف = Rand_گهڻائي_هدف::get(fromp, m_classp);
                إذا ((!rand_گهڻائي_هدف.receiverp || !rand_گهڻائي_هدف.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = خطأ;
                } وإلا إذا (!rand_گهڻائي_هدف.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = خطأ;
                } وإلا إذا (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_اظهار)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = خطأ;
                } وإلا إذا (rand_گهڻائي_هدف.receiverp && rand_گهڻائي_هدف.receiverp->isRand()) {
                    // Called on a rand member variable
                    Randomize_گهڻائي rand_گهڻائي = {};
                    rand_گهڻائي.استعمال_ڪري_گهڻائي = سچ;
                    rand_گهڻائي_هدف.receiverp->user1(rand_گهڻائي.asUQuad);
                } وإلا {
                    // Called on 'this' or a non-rand class instance
                    rand_گهڻائي_هدف.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        إذا (!varp->isRand()) أرجع;
                        Randomize_گهڻائي rand_گهڻائي = {};
                        rand_گهڻائي.استعمال_ڪري_گهڻائي = سچ;
                        varp->user1(rand_گهڻائي.asUQuad);
                    });
                }
            }
            إذا (!valid) {
                إذا (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_اظهار)) {
                    nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
                } وإلا {
                    m_stmtp->unlink_Fr_پٺ();
                }
            }
            أرجع;
        }

        إذا (nodep->name() == "constraint_mode") {
            منطقي valid = سچ;
            إذا (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_اظهار)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = خطأ;
            } وإلا إذا (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_اظهار)
                       && !nodep->backp()->fileline()->warnIsOff(V3_Error_ڪوڊ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = خطأ;
            }
            AstConstraint* constrp = خالي_اشارو;
            AstClass* classp = m_classp;
            إذا (ثابت Ast_طريقو_Call* ثابت طريقو_Callp = VN_CAST(nodep, طريقو_Call)) {
                إذا (ثابت AstConstraintRef* ثابت constrRefp
                    = VN_CAST(طريقو_Callp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    إذا (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } وإلا إذا (AstClassRefDType* classRefDtp
                           = VN_CAST(طريقو_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
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
                Randomize_گهڻائي constraint_گهڻائي = {};
                constraint_گهڻائي.استعمال_ڪري_گهڻائي = سچ;
                إذا (constrp) {
                    constrp->user1(constraint_گهڻائي.asUQuad);
                } وإلا {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraint_گهڻائي.asUQuad);
                    });
                }
            } وإلا {
                إذا (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_اظهار)) {
                    nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
                } وإلا {
                    m_stmtp->unlink_Fr_پٺ();
                }
            }
            أرجع;
        }

        إذا (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            إذا (m_constraint_اظهار_Genp || m_اندريون_Std_With) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                لكل (Ast_ڳنڍ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    إذا (argp->user1()) {
                        nodep->user1(سچ);
                        اكسر;
                    }
                }
            }
            أرجع;
        }
        AstClass* classp = m_classp;
        إذا (ثابت Ast_طريقو_Call* ثابت طريقو_Callp = VN_CAST(nodep, طريقو_Call)) {
            إذا (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(طريقو_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        إذا (classp) {
            إذا (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت memberVarp) {
                إذا (!memberVarp->rand().isRandomizable()) أرجع;
                ثابت Ast_ڳنڍ_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                إذا (!classRefp || !classRefp->classp()) أرجع;
                AstClass* ثابت memberClassp = classRefp->classp();
                إذا (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) أرجع;
                memberVarp->globalConstrained(سچ);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* پاڙ_Var_Refp
                    = جديد AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::ويڪٽر<AstVar*> خالي_جايون_رستو;
                memberClassp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, پاڙ_Var_Refp, خالي_جايون_رستو, classp);
                });
                cloneNestedConstraintsRecurse(پاڙ_Var_Refp, memberClassp, خالي_جايون_رستو, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(پاڙ_Var_Refp->delete_وڻ(), پاڙ_Var_Refp);
            });
        }
        إذا (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = خالي_اشارو;
            لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handle_Randomize_دليل(argp->exprp(), خالي_اشارو, سچ);
            }
            أرجع;
        }
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            إذا (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->انگ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ثابت منطقي hasUnsupportedMember
                    = classp->existsMember([](ثابت AstClass*, ثابت AstVar* memberVarp) {
                          إذا (!memberVarp->rand().isRandomizable()) أرجع خطأ;
                          ثابت Ast_ڳنڍ_DType* ثابت dtp = memberVarp->dtypep()->skipRefp();
                          أرجع VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, قطار_DType) || VN_IS(dtp, AssocArrayDType)
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
            AstVar* fromVarp = خالي_اشارو;  // If nodep is a method call, this is its receiver
            إذا (Ast_طريقو_Call* طريقو_Callp = VN_CAST(nodep, طريقو_Call)) {
                إذا (AstMemberSel* ثابت memberSelp = VN_CAST(طريقو_Callp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } وإلا {
                    AstVarRef* ثابت varrefp = VN_AS(طريقو_Callp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handle_Randomize_دليل(argp->exprp(), fromVarp, خطأ);
        }
        // Re-mark members after the class became inline-randomized above.
        إذا (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    فراغ گهمو(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_اظهار_Genp);
        m_stmtp = nodep;
        m_constraint_اظهار_Genp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->backp()) VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(Ast_Constraint_اظهار* nodep) override {
        VL_RESTORER(m_constraint_اظهار_Genp);
        m_constraint_اظهار_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_اظهار_Genp);
            m_constraint_اظهار_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterate_And_ايندڙ_Const_Null(nodep->thensp());
        iterate_And_ايندڙ_Const_Null(nodep->elsesp());
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        إذا (!m_constraint_اظهار_Genp) أرجع;

        إذا (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        إذا (nodep->varp()->rand().isRandomizable() && !(m_اندريون_Std_With && m_stdRandCallp)) {
            nodep->user1(سچ);
        } وإلا إذا (is_Var_اندريون_Std_Randomize_دليلون(nodep->varp())) {
            nodep->user1(سچ);
        }
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        إذا (!m_constraint_اظهار_Genp) أرجع;
        iterateChildrenConst(nodep);
        ثابت منطقي randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ثابت منطقي randMember = nodep->varp()->rand().isRandomizable();
        ثابت منطقي اندريون_Std_With = m_اندريون_Std_With && m_stdRandCallp;
        إذا (randObject && randMember && !اندريون_Std_With) {
            nodep->user1(سچ);
        } وإلا إذا (اندريون_Std_With && is_Var_اندريون_Std_Randomize_دليلون(nodep->varp())) {
            nodep->user1(سچ);
            // Mark parent object for constraint expression visitor
            إذا (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(سچ);
        }

        إذا (m_withp) {
            Ast_ڳنڍ* backp = m_withp;
            طالما (backp->backp()) {
                إذا (ثابت Ast_طريقو_Call* ثابت callp = VN_CAST(backp, طريقو_Call)) {
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
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstVar* nodep) override {
        nodep->user2p(m_modp);
        إذا (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        إذا (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_اندريون_Std_With = سچ;
        iterateChildrenConst(nodep);
        m_اندريون_Std_With = خطأ;
    }

    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        iterateChildrenConst(nodep);
        إذا (!m_constraint_اظهار_Genp && !m_اندريون_Std_With) أرجع;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    فراغ گهمو(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        إذا (!m_constraint_اظهار_Genp && !m_اندريون_Std_With) أرجع;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    واضح RandomizeMarkVisitor(Ast_ڳنڍ* nodep) {
        iterateConst(nodep);
        markAllDerived();
        سيٽ_Package_Refs();
    }
    ~RandomizeMarkVisitor() override = افتراضي;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

صنف Constraint_اظهار_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ثابت m_classp;
    Ast_ڳنڍ_FTask* ثابت m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_ڳنڍ_FTask* ثابت m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ثابت m_genp;  // VlRandomizer variable of the class
    AstVar* m_rand_گهڻائي_Varp;  // Relevant randmode state variable
    منطقي m_wantSingle = خطأ;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    منطقي m_structSel = خطأ;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    منطقي m_defer_Rand_گهڻائي_Hoist = خطأ;  // Wait for the complete struct member path
    std::سيٽ<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::سيٽ<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertion_سيٽ<AstVar*>* m_سائيز_Constrained_Arraysp = خالي_اشارو;  // Arrays with size constraints
    Ast_ڳنڍ_اظهار* m_conditionp = خالي_اشارو;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_ڳنڍ_FTask* m_prepareConstrainedArraysp = خالي_اشارو;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = خالي_اشارو;  // Current ID of unique call
    V3_Unique_نالا& m_unique_نالا;  // Unique names of temporaries, and of blocks holding them
    std::ويڪٽر<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_ڳنڍ* m_پهريون_اظهار_Inside_Indexp = خالي_اشارو;
    AstConstraintForeach* m_foreachp = خالي_اشارو;  // Innermost constraint-foreach being processed
    std::سيٽ<ثابت AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    صنف Nested_Access_رستو final {
        AstMemberSel* m_چوٽي_Nested_Array_Member_Selp
            = خالي_اشارو;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = خالي_اشارو;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = خالي_اشارو;  // Last node of variable's name format
        Ast_ڳنڍ** m_پهريون_اظهار_Inside_اشاريو_Pointerp
            = خالي_اشارو;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    عام:
        فراغ جوڙيو_Var_Name_Part(AstSFormatF* ثابت nodep, std::string&& name) {
            إذا (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } وإلا {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.وجهو(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            إذا (name == "") {
                إذا (*m_پهريون_اظهار_Inside_اشاريو_Pointerp) {
                    (*m_پهريون_اظهار_Inside_اشاريو_Pointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_پهريون_اظهار_Inside_اشاريو_Pointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_پهريون_اظهار_Inside_اشاريو_Pointerp = nodep;
            }
        }

        AstMemberSel* access_وڻ() { أرجع m_چوٽي_Nested_Array_Member_Selp; }
        ثابت std::string& smtName() { أرجع m_smtName; }

        Ast_Stmt_اظهار* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ثابت methodp = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(genp->user2p(), ڳنڍ_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtype_سيٽ_Void();

            methodp->جوڙيو_Pinsp(m_چوٽي_Nested_Array_Member_Selp);  // Variable

            ثابت Ast_ڳنڍ_DType* ثابت dtypep = varp->dtypep();  //ويڪر
            ثابت سائيز_t ويڪر = m_چوٽي_Nested_Array_Member_Selp->varp()->dtypep()->ويڪر();
            methodp->جوڙيو_Pinsp(جديد AstConst{dtypep->fileline(), AstConst::Unsized64{}, ويڪر});

            methodp->جوڙيو_Pinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = خالي_اشارو;

            methodp->جوڙيو_Pinsp(
                جديد AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_چوٽي_Nested_Array_Member_Selp = خالي_اشارو;
            أرجع methodp->makeStmt();
        }

        Ast_ڳنڍ_اظهار* cloneName() { أرجع m_nestedNameFormatTopp->clone_وڻ(خطأ); }

        فراغ error() {
            VL_DO_DANGLING(m_چوٽي_Nested_Array_Member_Selp->delete_وڻ(), m_چوٽي_Nested_Array_Member_Selp);
        }

        Nested_Access_رستو(AstMemberSel* چوٽي_Member_Selp, Ast_ڳنڍ** اشارو_منزل_پهريون_Exprp)
            : m_چوٽي_Nested_Array_Member_Selp{چوٽي_Member_Selp}
            , m_پهريون_اظهار_Inside_اشاريو_Pointerp{اشارو_منزل_پهريون_Exprp} {}
        ~Nested_Access_رستو() {
            إذا (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_وڻ(),
                            m_nestedNameFormatTopp = خالي_اشارو);
                m_nestedNameFormatp = خالي_اشارو;
            }
        }
    };
    Nested_Access_رستو* m_nestedAccess = خالي_اشارو;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_ڳنڍ_اظهار* createEnumConstraint(File_سٽ* ثابت fl, ثابت std::string& smtName,
                                      AstEnumDType* ثابت enumDtp, AstVar* ثابت genVarp) {
        Ast_ڳنڍ_Module* ثابت genModp = VN_AS(genVarp->user2p(), ڳنڍ_Module);
        ثابت صحيح ويڪر = enumDtp->ويڪر();
        std::string constraint = "(__Vbv (or";
        لكل (Ast_Enum_شي* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شي)) {
            ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvt_منزل_Str(vconstp->منزل_UInt()) + " "
                          + cvt_منزل_Str(ويڪر) + "))";
        }
        constraint += "))";
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + constraint + "\""}};
        callp->dtype_سيٽ_Void();
        أرجع callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* ڳوليو_Static_Rand_گهڻائي_Var_Member(AstClass* classp) ثابت {
        طالما (سچ) {
            إذا (AstVar* ثابت varp
                = VN_CAST(m_memberMap.ڳوليو_Member(classp, "__Vstaticrandmode"), Var)) {
                أرجع varp;
            }
            AstClassExtends* ثابت extendsp = classp->extendsp();
            إذا (!extendsp) أرجع خالي_اشارو;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    فراغ markConstrainedRandMember(AstStructDType* ثابت structp, ثابت std::string& name) {
        AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.ڳوليو_Member(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(سچ);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    منطقي is_Rand_Qualified_رستو(ثابت Ast_ڳنڍ_اظهار* nodep) {
        طالما (nodep) {
            إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                إذا (AstStructDType* ثابت structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ثابت AstMemberDType* ثابت memberp
                        = VN_CAST(m_memberMap.ڳوليو_Member(structp, selp->name()), MemberDType);
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
        أرجع سچ;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    ساكن فراغ markStructConstrainedRandRecurse(Ast_ڳنڍ_DType* ثابت dtypep) {
        AstStructDType* ثابت structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        إذا (!structp || structp->packed()) أرجع;
        structp->markConstrainedRand(سچ);
        لكل (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            إذا (!memberp->rand().isRandomizable()) استمر;
            memberp->markConstrainedRand(سچ);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string build_Member_رستو(ثابت AstMemberSel* ثابت memberSelp) {
        ثابت Ast_ڳنڍ* fromp = memberSelp->fromp();
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            أرجع refp->name() + "." + memberSelp->name();
        } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            أرجع build_Member_رستو(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        أرجع "";
    }

    ساكن Ast_ڳنڍ_اظهار* getFromp(ثابت Ast_ڳنڍ_اظهار* ثابت nodep) {
        إذا (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            أرجع cmethp->fromp();
        }
        أرجع getSelFromp(nodep);
    }

    ساكن std::string ڪڍو_Solve_Before_بنياد_Name(ثابت Ast_ڳنڍ_اظهار* exprp) {
        طالما (ثابت Ast_ڳنڍ_اظهار* ثابت fromp = getFromp(exprp)) exprp = fromp;
        إذا (ثابت AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) أرجع vrefp->name();
        أرجع "";
    }

    ساكن فراغ build_Name_اڳياڙي(AstCExpr* exprp, ثابت Ast_ڳنڍ_اظهار* ثابت nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            exprp->جوڙيو("(\""s + refp->name());
            أرجع;
        }

        ثابت Ast_ڳنڍ_اظهار* ثابت fromp = getFromp(nodep);
        إذا (fromp) build_Name_اڳياڙي(exprp, fromp);

        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            exprp->جوڙيو("." + selp->name());
        } وإلا إذا (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            أرجع;
        } وإلا إذا (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            إذا (cmethp->طريقو() == VCMethod::ARRAY_AT) {
                Ast_ڳنڍ_اظهار* ثابت argp = cmethp->pinsp();
                exprp->جوڙيو(".\" + ");
                AstCFuncHard* ثابت solverHexFuncp
                    = جديد AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
                solverHexFuncp->جوڙيو_Pinsp(argp->clone_وڻ_خالص(خطأ));
                solverHexFuncp->dtype_سيٽ_String();
                exprp->جوڙيو(solverHexFuncp);
                exprp->جوڙيو(" + \"");
            } وإلا {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                أرجع;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ساكن AstCExpr* build_رستو_اظهار(File_سٽ* ثابت fl, ثابت Ast_ڳنڍ_اظهار* nodep,
                                   ثابت std::string selName) {
        AstCExpr* ثابت exprp = جديد AstCExpr{fl, ""};
        build_Name_اڳياڙي(exprp, nodep);
        إذا (selName != "") {
            exprp->جوڙيو(".");
            exprp->جوڙيو(selName);
        }
        exprp->جوڙيو("\")");
        أرجع exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_اظهار(File_سٽ* fl, ثابت std::string& بنياد_Name,
                                    ثابت Ast_ڳنڍ_Sel* selp) {
        AstCExpr* ثابت p = جديد AstCExpr{fl, ""};
        p->جوڙيو("(\""s + بنياد_Name + ".\"");
        p->جوڙيو("+");
        AstCFuncHard* ثابت solverHexFuncp
            = جديد AstCFuncHard{fl, VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
        solverHexFuncp->جوڙيو_Pinsp(selp->bitp()->clone_وڻ_خالص(خطأ));
        solverHexFuncp->dtype_سيٽ_String();
        p->جوڙيو(solverHexFuncp);
        p->جوڙيو(")");
        p->dtype_سيٽ_String();
        أرجع p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ساكن Ast_ڳنڍ_اظهار* getSelFromp(ثابت Ast_ڳنڍ_اظهار* exprp) {
        إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(exprp, MemberSel)) أرجع mp->fromp();
        إذا (ثابت AstStructSel* ثابت sp = VN_CAST(exprp, StructSel)) أرجع sp->fromp();
        أرجع خالي_اشارو;
    }

    Ast_ڳنڍ_اظهار* build_Solve_Before_Name_اظهار(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp) {
        إذا (ثابت AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ثابت p = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + varrefp->name() + "\"s"};
            p->dtype_سيٽ_String();
            أرجع p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        إذا (Ast_ڳنڍ_اظهار* ثابت selFromp = getSelFromp(exprp)) {
            ثابت std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            إذا (ثابت AstArraySel* ثابت arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string بنياد_Name;
                إذا (ثابت AstVarRef* ثابت vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    بنياد_Name = vp->name();
                } وإلا إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    بنياد_Name = build_Member_رستو(mp);
                }
                إذا (بنياد_Name.خالي_جايون()) أرجع خالي_اشارو;
                AstCExpr* ثابت p = جديد AstCExpr{fl, ""};
                p->جوڙيو("(\""s + بنياد_Name + ".\"");
                p->جوڙيو("+");
                AstCFuncHard* ثابت solverHexFuncp
                    = جديد AstCFuncHard{fl, VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
                solverHexFuncp->جوڙيو_Pinsp(arrSelp->bitp()->clone_وڻ_خالص(خطأ));
                solverHexFuncp->dtype_سيٽ_String();
                p->جوڙيو(solverHexFuncp);
                p->جوڙيو(")");
                p->dtype_سيٽ_String();
                أرجع p;
            }
            // Static member path (obj.field)
            إذا (ثابت AstVarRef* ثابت vp = VN_CAST(selFromp, VarRef)) {
                ثابت std::string رستو = vp->name() + "." + selName;
                AstCExpr* ثابت p = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + رستو + "\"s"};
                p->dtype_سيٽ_String();
                أرجع p;
            }
            إذا (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ثابت p = build_رستو_اظهار(fl, selFromp, selName);
                p->dtype_سيٽ_String();
                أرجع p;
            }
            أرجع خالي_اشارو;
        }
        إذا (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ثابت p = build_رستو_اظهار(fl, exprp, "");
            p->dtype_سيٽ_String();
            أرجع p;
        }
        ثابت Ast_ڳنڍ_Sel* selp = VN_CAST(exprp, ArraySel);
        إذا (!selp) selp = VN_CAST(exprp, AssocSel);
        إذا (selp) {
            // arr[i] -> dynamic name
            std::string بنياد_Name;
            إذا (ثابت AstVarRef* ثابت vp = VN_CAST(selp->fromp(), VarRef)) {
                بنياد_Name = vp->name();
            } وإلا إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(selp->fromp(), MemberSel)) {
                بنياد_Name = build_Member_رستو(mp);
            }
            إذا (بنياد_Name.خالي_جايون()) أرجع خالي_اشارو;
            أرجع build_Array_Sel_Name_اظهار(fl, بنياد_Name, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        إذا (ثابت AstSel* ثابت ٻٽو_انگ_Selp = VN_CAST(exprp, Sel)) {
            أرجع build_Solve_Before_Name_اظهار(fl, ٻٽو_انگ_Selp->fromp());
        }
        أرجع خالي_اشارو;
    }

    AstSFormatF* getConstFormat(Ast_ڳنڍ_اظهار* nodep) {
        أرجع جديد AstSFormatF{nodep->fileline(), (nodep->ويڪر() & 3) ? "#b%b" : "#x%x", خطأ,
                               nodep};
    }
    منطقي editFormat(Ast_ڳنڍ_اظهار* nodep) {
        إذا (nodep->user1()) أرجع خطأ;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlink_Fr_پٺ(&handle);
        handle.relink(getConstFormat(nodep));
        أرجع سچ;
    }
    Ast_ڳنڍ_اظهار* wrapWithCond(Ast_ڳنڍ_اظهار* ثابت exprp, Ast_ڳنڍ_اظهار* ثابت condp,
                              ثابت صحيح ويڪر) {
        إذا (condp) {
            File_سٽ* ثابت flp = exprp->fileline();
            أرجع جديد AstCond{
                flp, condp, exprp,
                getConstFormat(جديد AstConst{flp, AstConst::Widthed_قدر{}, ويڪر, 0})};
        }
        أرجع exprp;
    }
    AstSFormatF* editSMT(Ast_ڳنڍ_اظهار* nodep, Ast_ڳنڍ_اظهار* lhsp = خالي_اشارو,
                         Ast_ڳنڍ_اظهار* rhsp = خالي_اشارو, Ast_ڳنڍ_اظهار* thsp = خالي_اشارو) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_اظهار = nodep->emitSMT();  // Might need child width (AstExtend)
        إذا (smt_اظهار == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            أرجع خالي_اشارو;
        }

        ثابت صحيح lhs_ويڪر = lhsp ? lhsp->ويڪر() : 0;
        ثابت صحيح rhs_ويڪر = rhsp ? rhsp->ويڪر() : 0;
        ثابت صحيح ths_ويڪر = thsp ? thsp->ويڪر() : 0;
        Ast_ڳنڍ_اظهار* lhsCondp = خالي_اشارو;
        Ast_ڳنڍ_اظهار* rhsCondp = خالي_اشارو;
        Ast_ڳنڍ_اظهار* thsCondp = خالي_اشارو;

        إذا (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = خالي_اشارو;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlink_Fr_پٺ() : lhsp),
                         ڳنڍ_اظهار);
            lhsCondp = m_conditionp;
        }
        إذا (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = خالي_اشارو;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlink_Fr_پٺ() : rhsp),
                         ڳنڍ_اظهار);
            rhsCondp = m_conditionp;
        }
        إذا (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = خالي_اشارو;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlink_Fr_پٺ() : thsp),
                         ڳنڍ_اظهار);
            thsCondp = m_conditionp;
        }

        Ast_ڳنڍ_اظهار* argsp = خالي_اشارو;
        لكل (string::iterator pos = smt_اظهار.begin(); pos != smt_اظهار.پڇاڙي(); ++pos) {
            إذا (pos[0] == '%') {
                ++pos;
                بدل (pos[0]) {
                حالة '%': اكسر;
                حالة 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(argsp, wrapWithCond(lhsp, lhsCondp, lhs_ويڪر));
                    lhsp = خالي_اشارو;
                    اكسر;
                حالة 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(argsp, wrapWithCond(rhsp, rhsCondp, rhs_ويڪر));
                    rhsp = خالي_اشارو;
                    اكسر;
                حالة 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(argsp, wrapWithCond(thsp, thsCondp, ths_ويڪر));
                    thsp = خالي_اشارو;
                    اكسر;
                افتراضي: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); اكسر;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ثابت newp = جديد AstSFormatF{nodep->fileline(), smt_اظهار, خطأ, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        أرجع newp;
    }

    Ast_ڳنڍ_اظهار* editSingle(File_سٽ* fl, Ast_ڳنڍ* itemsp) {
        إذا (!itemsp) أرجع خالي_اشارو;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = سچ;

        {
            AstBegin* ثابت tempp
                = جديد AstBegin{fl, "[EditWrapper]", itemsp->unlink_Fr_پٺ_With_ايندڙ(), خطأ};
            VL_DO_DANGLING(iterate_And_ايندڙ_Null(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            إذا (itemsp) itemsp->unlink_Fr_پٺ_With_ايندڙ();
            VL_DO_DANGLING(tempp->delete_وڻ(), tempp);
        }
        إذا (!itemsp) أرجع خالي_اشارو;

        Ast_ڳنڍ_اظهار* ثابت exprsp = VN_CAST(itemsp, ڳنڍ_اظهار);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        إذا (!exprsp->nextp()) أرجع exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        لكل (Ast_ڳنڍ* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        أرجع جديد AstSFormatF{fl, fmt.str(), خطأ, exprsp};
    }

    Ast_ڳنڍ_اظهار* newSel(File_سٽ* fl, Ast_ڳنڍ_اظهار* arrayp, Ast_ڳنڍ_اظهار* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_ڳنڍ_DType* ثابت arrDtp = arrayp->unlink_Fr_پٺ()->dtypep()->skipRefp();
        Ast_ڳنڍ_اظهار* selp = خالي_اشارو;
        إذا (VN_IS(arrDtp, قطار_DType) || VN_IS(arrDtp, DynArrayDType))
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
    Ast_ڳنڍ_اظهار* adjust_ويڪر(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, صحيح هدف_ويڪر,
                             VSigning هدف_Signing) {
        ثابت صحيح اظهار_ويڪر = exprp->ويڪر();
        إذا (هدف_ويڪر > اظهار_ويڪر) {
            // Extend to match target width
            Ast_ڳنڍ_اظهار* ثابت نتيجو = جديد AstExtend{fl, exprp, هدف_ويڪر};
            نتيجو->dtypeChgSigned(هدف_Signing);
            أرجع نتيجو;
        } وإلا إذا (هدف_ويڪر < اظهار_ويڪر) {
            // Truncate to match target width
            Ast_ڳنڍ_اظهار* ثابت نتيجو = جديد AstSel{fl, exprp, 0, هدف_ويڪر};
            نتيجو->dtypeChgSigned(هدف_Signing);
            أرجع نتيجو;
        } وإلا {
            // Width already matches
            أرجع exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_ڳنڍ_اظهار* ڪڍو_Return_اظهار(AstFunc* funcp) {
        AstVar* ثابت retVarp = VN_CAST(funcp->fvarp(), Var);
        إذا (!retVarp) أرجع خالي_اشارو;
        Ast_ڳنڍ_اظهار* retExprp = خالي_اشارو;
        صحيح سونپيو_ڳڻپ = 0;
        funcp->foreach([&](Ast_سونپيو* assignp) {
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            إذا (!lhsp || lhsp->varp() != retVarp) أرجع;
            retExprp = assignp->rhsp();
            ++سونپيو_ڳڻپ;
        });
        // Simple function: initializer + return (2) or just return (1)
        إذا (سونپيو_ڳڻپ > 2 || !retExprp) أرجع خالي_اشارو;
        أرجع retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    فراغ propagateUser1InlineRecurse(Ast_ڳنڍ_اظهار* nodep) {
        إذا (VN_IS(nodep, ڳنڍ_Var_Ref)) {
            nodep->user1(VN_AS(nodep, ڳنڍ_Var_Ref)->varp()->rand().isRandomizable());
            أرجع;
        }
        إذا (VN_IS(nodep, Const)) {
            nodep->user1(خطأ);
            أرجع;
        }
        صحيح any_ٻار = خطأ;  // Used as bool
        إذا (Ast_ڳنڍ_اظهار* ثابت cp = VN_CAST(nodep->op1p(), ڳنڍ_اظهار)) {
            propagateUser1InlineRecurse(cp);
            any_ٻار |= cp->user1();
        }
        إذا (Ast_ڳنڍ_اظهار* ثابت cp = VN_CAST(nodep->op2p(), ڳنڍ_اظهار)) {
            propagateUser1InlineRecurse(cp);
            any_ٻار |= cp->user1();
        }
        إذا (Ast_ڳنڍ_اظهار* ثابت cp = VN_CAST(nodep->op3p(), ڳنڍ_اظهار)) {
            propagateUser1InlineRecurse(cp);
            any_ٻار |= cp->user1();
        }
        إذا (Ast_ڳنڍ_اظهار* ثابت cp = VN_CAST(nodep->op4p(), ڳنڍ_اظهار)) {
            propagateUser1InlineRecurse(cp);
            any_ٻار |= cp->user1();
        }
        nodep->user1(any_ٻار);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ساكن Ast_ڳنڍ* prependDistPreamble(AstConstraintForeach* nodep, Ast_ڳنڍ* bodyp) {
        Ast_ڳنڍ* ثابت preamblep = nodep->user3p();
        إذا (!preamblep) أرجع bodyp;
        nodep->user3p(خالي_اشارو);
        preamblep->جوڙيو_ايندڙ(bodyp);
        أرجع preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    منطقي referencesForeachIdx(ثابت Ast_ڳنڍ* nodep) ثابت {
        إذا (m_foreachIdxVars.خالي_جايون()) أرجع خطأ;
        أرجع nodep->exists(
            [&](ثابت AstVarRef* refp) { أرجع m_foreachIdxVars.ڳڻپ(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    فراغ جوڙيو_Registration_Stmt(Ast_ڳنڍ* ثابت stmtp, Ast_ڳنڍ_FTask* ثابت initTaskp) {
        إذا (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->جوڙيو_Bodyp(stmtp);
        } وإلا {
            initTaskp->جوڙيو_Stmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_سٽ* ثابت fl, Ast_ڳنڍ_اظهار* ثابت arrExprp,
                                       Ast_ڳنڍ_اظهار* ثابت idxExprp) {
        Ast_ڳنڍ_اظهار* ثابت argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(arrExprp, idxExprp);
        AstSFormatF* ثابت formatp = جديد AstSFormatF{fl, "(select %s %s)", خطأ, argsp};
        أرجع formatp;
    }

    فراغ سيٽ_Rand_گهڻائي(AstVar* ثابت varp, AstMemberSel* ثابت memberselp,
                     ثابت std::string& smtName, ثابت Randomize_گهڻائي& rand_گهڻائي,
                     Ast_ڳنڍ_FTask* ثابت initTaskp) {
        Ast_ڳنڍ_Module* ثابت varClassp = VN_AS(varp->user2p(), ڳنڍ_Module);
        AstVar* ثابت sub_Rand_گهڻائي_Varp = get_Rand_گهڻائي_Var_From_Class(varClassp);
        إذا (sub_Rand_گهڻائي_Varp) {
            Ast_ڳنڍ_اظهار* ثابت والد_Accessp = memberselp->fromp()->clone_وڻ(خطأ);
            AstMemberSel* ثابت rand_گهڻائي_Selp
                = جديد AstMemberSel{varp->fileline(), والد_Accessp, sub_Rand_گهڻائي_Varp};
            rand_گهڻائي_Selp->dtypep(sub_Rand_گهڻائي_Varp->dtypep());
            AstCMethodHard* ثابت atp
                = جديد AstCMethodHard{varp->fileline(), rand_گهڻائي_Selp, VCMethod::ARRAY_AT,
                                     جديد AstConst{varp->fileline(), rand_گهڻائي.اشاريو}};
            atp->dtype_سيٽ_UInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ثابت enablep = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ڳنڍ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtype_سيٽ_Void();
            Ast_ڳنڍ_اظهار* ثابت nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : جديد AstSFormatF{varp->fileline(), smtName, خطأ, خالي_اشارو};
            nameExprp->dtypep(varp->dtypep());
            enablep->جوڙيو_Pinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ثابت disablep = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ڳنڍ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_سيٽ_VAR_DISABLED};
            disablep->dtype_سيٽ_Void();
            Ast_ڳنڍ_اظهار* ثابت disnp = nameExprp->clone_وڻ(خطأ);
            disablep->جوڙيو_Pinsp(disnp);
            AstIf* ثابت ifp
                = جديد AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            جوڙيو_Registration_Stmt(ifp, initTaskp);
        }
    }

    فراغ markRandc(AstVar* ثابت varp, ثابت std::string& smtName, Ast_ڳنڍ_FTask* ثابت initTaskp) {
        AstCMethodHard* ثابت markp = جديد AstCMethodHard{
            varp->fileline(),
            جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ڳنڍ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtype_سيٽ_Void();
        Ast_ڳنڍ_اظهار* ثابت nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : جديد AstSFormatF{varp->fileline(), smtName, خطأ, خالي_اشارو};
        nameExprp->dtypep(varp->dtypep());
        markp->جوڙيو_Pinsp(nameExprp);
        جوڙيو_Registration_Stmt(markp->makeStmt(), initTaskp);
    }

    Ast_ڳنڍ_Module* getLeftmostVarModulep(AstMemberSel* ثابت memberselp,
                                         AstVar* ثابت varp) ثابت {
        إذا (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_ڳنڍ* کاٻو_Most_Nodep = memberselp->fromp();
            طالما (AstMemberSel* ثابت nestedMemberSelp = VN_CAST(کاٻو_Most_Nodep, MemberSel)) {
                کاٻو_Most_Nodep = nestedMemberSelp->fromp();
            }
            إذا (Ast_ڳنڍ_Var_Ref* ثابت پاڙ_Var_Refp = VN_CAST(کاٻو_Most_Nodep, ڳنڍ_Var_Ref)) {
                أرجع VN_AS(پاڙ_Var_Refp->varp()->user2p(), ڳنڍ_Module);
            }
            أرجع VN_AS(memberselp->user2p(), ڳنڍ_Module);
        }
        أرجع VN_AS(varp->user2p(), ڳنڍ_Module);
    }

    Ast_ڳنڍ_FTask* getInitTaskp(AstVar* ثابت varp, ثابت منطقي memberselOrStructSel,
                               Ast_ڳنڍ_Module* ثابت classp) ثابت {
        إذا (m_inlineInitTaskp) أرجع m_inlineInitTaskp;
        varp->user3(سچ);
        إذا (memberselOrStructSel) {
            إذا (Ast_ڳنڍ_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.ڳوليو_Member(classp, "randomize"), ڳنڍ_FTask)) {
                أرجع initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            إذا (m_memberselInitTaskp) أرجع m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } وإلا {
            Ast_ڳنڍ_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.ڳوليو_Member(classp, "new"), ڳنڍ_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            أرجع initTaskp;
        }
        أرجع خالي_اشارو;
    }

    فراغ createWriteVarForClassFields(AstClass* ثابت elemClassp, ثابت std::string& smtName,
                                      AstVar* ثابت iterVarp, ثابت منطقي isUnpackedClassRefArray,
                                      Ast_ڳنڍ_Module* ثابت classOrPackagep, AstVar* ثابت varp,
                                      AstIf* ثابت ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ثابت {
        File_سٽ* ثابت fl = varp->fileline();
        Ast_ڳنڍ_Module* ثابت varClassp = VN_AS(varp->user2p(), ڳنڍ_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr صحيح idx_ويڪر = 32;
        ثابت صحيح fmt_ويڪر = VL_لفظ_I(idx_ويڪر) * 8;

        لكل (ثابت AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : خالي_اشارو) {
            لكل (Ast_ڳنڍ* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ثابت memberVarp = VN_CAST(mnodep, Var);
                إذا (!memberVarp || !memberVarp->rand().isRandomizable()) استمر;
                Ast_ڳنڍ_DType* ثابت memberDtp = memberVarp->dtypep()->skipRefp();
                إذا (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) استمر;
                ثابت صحيح member_ويڪر = memberDtp->ويڪر();

                AstCStmt* ثابت fmtp = جديد AstCStmt{fl};
                إذا (isUnpackedClassRefArray) {
                    fmtp->جوڙيو("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } وإلا {
                    fmtp->جوڙيو("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::منزل_string(fmt_ويڪر) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->جوڙيو(جديد AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->جوڙيو(");\n");
                ifNonNullp->جوڙيو_Thensp(fmtp);

                AstVarRef* ثابت arrayWrRefp = جديد AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_ڳنڍ_اظهار* atWritep;
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
                                         جديد AstVarRef{fl, VN_AS(m_genp->user2p(), ڳنڍ_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtype_سيٽ_Void();
                writeVarp->جوڙيو_Pinsp(memberSelp);
                writeVarp->جوڙيو_Pinsp(
                    جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(member_ويڪر)});
                AstCExpr* ثابت nameRefp = جديد AstCExpr{fl, AstCExpr::خالص{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->جوڙيو_Pinsp(nameRefp);
                writeVarp->جوڙيو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->جوڙيو_Thensp(writeVarp->makeStmt());
            }
        }
    }

    فراغ createSolverArrayHandle(AstVar* ثابت varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_ڳنڍ_Module* ثابت classOrPackagep,
                                 ثابت منطقي isUnpackedClassRefArray,
                                 ثابت std::string& smtName) ثابت {
        File_سٽ* ثابت fl = varp->fileline();
        AstClass* ثابت elemClassp = elemClassRefDtp->classp();
        Ast_ڳنڍ_Module* ثابت varClassp = VN_AS(varp->user2p(), ڳنڍ_Module);

        AstVar* ثابت iterVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, m_unique_نالا.get("__Vi"),
                                            varp->ڳوليو_UInt32_DType()};
        iterVarp->funcLocal(سچ);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ڳنڍ* ثابت stmtsp
            = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE}, جديد AstConst{fl, 0}};

        Ast_ڳنڍ_اظهار* sizep;
        إذا (isUnpackedClassRefArray) {
            ثابت صحيح array_سائيز_2
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = جديد AstConst{fl, static_cast<uint32_t>(array_سائيز_2)};
        } وإلا {
            AstVarRef* ثابت array_سائيز_Refp = جديد AstVarRef{fl, varClassp, varp, VAccess::READ};
            array_سائيز_Refp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ثابت dynSizep
                = جديد AstCMethodHard{fl, array_سائيز_Refp, VCMethod::DYN_سائيز, خالي_اشارو};
            dynSizep->dtype_سيٽ_UInt32();
            sizep = dynSizep;
        }

        AstLoop* ثابت loopp = جديد AstLoop{fl};
        stmtsp->جوڙيو_ايندڙ(loopp);
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{
            fl, loopp, جديد AstLt{fl, جديد AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ثابت arrayReadRefp = جديد AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_ڳنڍ_اظهار* atReadp;
        إذا (isUnpackedClassRefArray) {
            atReadp
                = جديد AstArraySel{fl, arrayReadRefp, جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        } وإلا {
            atReadp = جديد AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ثابت ifNonNullp
            = جديد AstIf{fl, جديد AstNeq{fl, atReadp, جديد AstConst{fl, AstConst::Null{}}}, خالي_اشارو};
        loopp->جوڙيو_Stmtsp(ifNonNullp);

        AstCStmt* ثابت bufDeclp = جديد AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->جوڙيو_Thensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{
            fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
            جديد Ast_جوڙيو{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(سچ);
        Ast_ڳنڍ_FTask* initTaskp = m_inlineInitTaskp;
        إذا (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.ڳوليو_Member(varClassp, "new"), ڳنڍ_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->جوڙيو_Stmtsp(stmtsp);
    }

    فراغ createSolverVarHandle(AstVar* ثابت varp, ثابت منطقي structSelOrCMeth,
                               ثابت منطقي isGlobalConstrained, ثابت Randomize_گهڻائي rand_گهڻائي,
                               AstMemberSel* ثابت memberselp, ثابت std::string& smtName,
                               Ast_ڳنڍ_Module* ثابت classOrPackagep, Ast_ڳنڍ_Module* ثابت classp,
                               Ast_ڳنڍ_FTask* ثابت initTaskp) {
        uint32_t unpackedDims = 0;
        إذا (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(خطأ).ٻيو;
        }
        إذا (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ثابت methodp = جديد AstCMethodHard{
            varp->fileline(),
            جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ڳنڍ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_سيٽ_Void();
        إذا (memberselp) {
            methodp->جوڙيو_Pinsp(memberselp);
        } وإلا {
            AstVarRef* ثابت varRefp
                = جديد AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->جوڙيو_Pinsp(varRefp);
        }
        ثابت سائيز_t ويڪر = arrayElementDTypep(varp->dtypep())->ويڪر();
        methodp->جوڙيو_Pinsp(جديد AstConst{varp->fileline(), AstConst::Unsized64{}, ويڪر});
        Ast_ڳنڍ_اظهار* ثابت varnamep = جديد AstCExpr{varp->fileline(), AstCExpr::خالص{},
                                                   "\"" + smtName + "\"", varp->ويڪر()};
        varnamep->dtypep(varp->dtypep());
        methodp->جوڙيو_Pinsp(varnamep);
        methodp->جوڙيو_Pinsp(
            جديد AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        إذا (rand_گهڻائي.استعمال_ڪري_گهڻائي && !(isGlobalConstrained && memberselp)) {
            methodp->جوڙيو_Pinsp(
                جديد AstConst{varp->fileline(), AstConst::Unsized64{}, rand_گهڻائي.اشاريو});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->جوڙيو_Stmtsp(methodp->makeStmt());
        إذا (varp->lifetime().isStatic() && rand_گهڻائي.استعمال_ڪري_گهڻائي) {
            AstCMethodHard* ثابت markp = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ڳنڍ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_ڳنڍ_اظهار* ثابت namep = جديد AstCExpr{varp->fileline(), AstCExpr::خالص{},
                                                    "\"" + smtName + "\"", varp->ويڪر()};
            namep->dtypep(varp->dtypep());
            markp->جوڙيو_Pinsp(namep);
            markp->dtype_سيٽ_Void();
            initTaskp->جوڙيو_Stmtsp(markp->makeStmt());
        }
        إذا (isGlobalConstrained && memberselp && rand_گهڻائي.استعمال_ڪري_گهڻائي) {
            سيٽ_Rand_گهڻائي(varp, memberselp, smtName, rand_گهڻائي, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        إذا (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_ڳنڍ_اظهار* createVarExprp(Ast_ڳنڍ_Var_Ref* nodep, ثابت Randomize_گهڻائي rand_گهڻائي,
                                ثابت std::string& smtName, ثابت منطقي isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ثابت varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_ڳنڍ_اظهار* exprp = جديد AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, خطأ, خالي_اشارو};
        إذا (!rand_گهڻائي.استعمال_ڪري_گهڻائي) {
            // Global constraints keep nodep alive for write_var processing
            إذا (!isGlobalConstrained) VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_ڳنڍ_اظهار* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_وڻ(خطأ)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_ڳنڍ_اظهار* rand_گهڻائي_Access;
        ثابت منطقي varIsStatic = varp->lifetime().isStatic();
        AstClass* ثابت varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : خالي_اشارو;
        AstVar* ثابت owner_Static_Rand_گهڻائي_Varp
            = varOwningClassp ? ڳوليو_Static_Rand_گهڻائي_Var_Member(varOwningClassp) : خالي_اشارو;
        إذا (owner_Static_Rand_گهڻائي_Varp) {
            rand_گهڻائي_Access = جديد AstVarRef{varp->fileline(),
                                           VN_AS(owner_Static_Rand_گهڻائي_Varp->user2p(), ڳنڍ_Module),
                                           owner_Static_Rand_گهڻائي_Varp, VAccess::READ};
        } وإلا إذا (memberselp) {
            Ast_ڳنڍ_Module* ثابت varClassp = VN_AS(varp->user2p(), ڳنڍ_Module);
            AstVar* ثابت effective_Rand_گهڻائي_Varp = get_Rand_گهڻائي_Var_From_Class(varClassp);
            إذا (effective_Rand_گهڻائي_Varp) {
                // Member's class has randmode, use it
                Ast_ڳنڍ_اظهار* والد_Access = memberselp->fromp()->clone_وڻ(خطأ);
                AstMemberSel* rand_گهڻائي_Sel
                    = جديد AstMemberSel{varp->fileline(), والد_Access, effective_Rand_گهڻائي_Varp};
                rand_گهڻائي_Sel->dtypep(effective_Rand_گهڻائي_Varp->dtypep());
                rand_گهڻائي_Access = rand_گهڻائي_Sel;
            } وإلا {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_rand_گهڻائي_Varp, nodep, "No m_randModeVarp");
                rand_گهڻائي_Access
                    = جديد AstVarRef{varp->fileline(), VN_AS(m_rand_گهڻائي_Varp->user2p(), ڳنڍ_Module),
                                    m_rand_گهڻائي_Varp, VAccess::READ};
            }
        } وإلا {
            UASSERT_OBJ(m_rand_گهڻائي_Varp, nodep, "No m_randModeVarp");
            rand_گهڻائي_Access
                = جديد AstVarRef{varp->fileline(), VN_AS(m_rand_گهڻائي_Varp->user2p(), ڳنڍ_Module),
                                m_rand_گهڻائي_Varp, VAccess::READ};
        }

        AstCMethodHard* ثابت atp
            = جديد AstCMethodHard{nodep->fileline(), rand_گهڻائي_Access, VCMethod::ARRAY_AT,
                                 جديد AstConst{nodep->fileline(), rand_گهڻائي.اشاريو}};
        atp->dtype_سيٽ_UInt32();
        exprp = جديد AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(سچ);  // Mark as formatted
        أرجع exprp;
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        إذا (varp->user4p()) {
            منطقي is_سائيز_Ref = خطأ;
            إذا (AstCMethodHard* ثابت methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                إذا (methodp->طريقو() == VCMethod::ASSOC_سائيز
                    || methodp->طريقو() == VCMethod::DYN_سائيز) {
                    is_سائيز_Ref = سچ;
                }
            }
            إذا (!is_سائيز_Ref && m_سائيز_Constrained_Arraysp) { m_سائيز_Constrained_Arraysp->وجهو(varp); }
        }

        // Check if this variable is marked as globally constrained
        ثابت منطقي isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = خالي_اشارو;
        منطقي structSelOrCMeth = خطأ;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_ڳنڍ_DType* selectedDtypep = خالي_اشارو;
        std::string smtName;
        إذا (m_nestedAccess) {
            m_nestedAccess->جوڙيو_Var_Name_Part(
                جديد AstSFormatF{varp->fileline(), nodep->name(), خطأ, خالي_اشارو}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_وڻ();
        } وإلا إذا (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_ڳنڍ* چوٽي_Member_Sel = nodep->backp();
            طالما (VN_IS(چوٽي_Member_Sel->backp(), MemberSel)) {
                چوٽي_Member_Sel = چوٽي_Member_Sel->backp();
            }
            memberselp = VN_AS(چوٽي_Member_Sel, MemberSel)->clone_وڻ(خطأ);
            smtName = build_Member_رستو(memberselp);
        } وإلا إذا (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_ڳنڍ* چوٽي_Nodep = nodep->backp();
            طالما (VN_IS(چوٽي_Nodep->backp(), StructSel) || VN_IS(چوٽي_Nodep->backp(), CMethodHard)) {
                چوٽي_Nodep = چوٽي_Nodep->backp();
            }

            إذا (VN_IS(چوٽي_Nodep, StructSel) || VN_IS(چوٽي_Nodep, CMethodHard)) {
                structSelOrCMeth = سچ;
                selectedDtypep = VN_AS(چوٽي_Nodep, ڳنڍ_اظهار)->dtypep();
            }

            memberselp = VN_CAST(چوٽي_Nodep, MemberSel);
            إذا (memberselp) smtName = build_Member_رستو(memberselp);

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
        إذا (dtype_شامل_آهي_Real(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            أرجع;
        }
        Ast_ڳنڍ_Module* ثابت classOrPackagep = nodep->classOrPackagep();
        ثابت Randomize_گهڻائي rand_گهڻائي = {.asUQuad = varp->user1()};
        إذا (!rand_گهڻائي.استعمال_ڪري_گهڻائي && editFormat(nodep)) أرجع;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (enumDtp
            && (std::ڳوليو(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.پڇاڙي(), varp)
                == m_rangeConstrainedEnums.پڇاڙي())) {
            AstVar* genVarp = m_genp;
            إذا (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_ڳنڍ_اظهار* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_ڳنڍ_FTask* هدف_Taskp = m_inlineInitTaskp;
            إذا (!هدف_Taskp) هدف_Taskp = get_Create_Constraint_تياري_Func(m_classp, m_memberMap);
            UASSERT_OBJ(هدف_Taskp, nodep, "No function to inline enum range constraint into");
            هدف_Taskp->جوڙيو_Stmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.ڌڪو_پٺ(varp);
        }

        VNRelinker relinker;
        nodep->unlink_Fr_پٺ(&relinker);
        Ast_ڳنڍ_اظهار* ثابت exprp
            = createVarExprp(nodep, rand_گهڻائي, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ثابت منطقي alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.ڳڻپ(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.ڳڻپ(smtName) > 0
                                    : memberselp          ? m_writtenVars.ڳڻپ(smtName) > 0
                                                          : varp->user3();
        ثابت منطقي shouldWriteVar = !alreadyWritten;
        إذا (shouldWriteVar) {
            // Track this variable path as written
            إذا (m_inlineInitTaskp)
                m_inlineWrittenVars.وجهو(smtName);
            وإلا إذا (isGlobalConstrained || memberselp)
                m_writtenVars.وجهو(smtName);
            // For global constraints, delete nodep after processing
            إذا (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);

            منطقي isClassRefArray = خطأ;
            منطقي isUnpackedClassRefArray = خطأ;
            AstClassRefDType* elemClassRefDtp = خالي_اشارو;
            {
                Ast_ڳنڍ_DType* varDtp = varp->dtypep()->skipRefp();
                إذا (varDtp->isNonPackedArray()) {
                    Ast_ڳنڍ_DType* ثابت elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    إذا (elemClassRefDtp) {
                        isClassRefArray = سچ;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_ڳنڍ_Module* ثابت classp = getLeftmostVarModulep(memberselp, varp);
            Ast_ڳنڍ_FTask* ثابت initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            إذا (m_nestedAccess) {
                Ast_Stmt_اظهار* ثابت writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                جوڙيو_Registration_Stmt(writeVarStmtp, initTaskp);
                إذا (isGlobalConstrained && memberselp && rand_گهڻائي.استعمال_ڪري_گهڻائي) {
                    سيٽ_Rand_گهڻائي(varp, memberselp, smtName, rand_گهڻائي, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                إذا (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } وإلا إذا (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } وإلا {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, rand_گهڻائي,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } وإلا {
            // Variable already written, clean up cloned memberselp if any
            إذا (memberselp) VL_DO_DANGLING(memberselp->delete_وڻ(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            إذا (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
        إذا (m_nestedAccess) { VL_DO_DANGLING(ميٽيو m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_ڳنڍ_اظهار* build_ڳڻپ_Ones_Expansion(File_سٽ* fl, Ast_ڳنڍ_اظهار* argp) {
        ثابت V3_عدد انگ_One{fl, argp->ويڪر(), 1};
        Ast_ڳنڍ_اظهار* sump = جديد AstAnd{fl, argp, جديد AstConst{fl, انگ_One}};
        sump->user1(سچ);
        لكل (صحيح i = 1; i < argp->ويڪر(); i++) {
            V3_عدد انگ_ٻٽو_انگ_نقاب{fl, argp->ويڪر(), 0};
            انگ_ٻٽو_انگ_نقاب.سيٽ_ٻٽو_انگ(i, 1);
            AstAnd* ثابت andp
                = جديد AstAnd{fl, argp->clone_وڻ_خالص(خطأ), جديد AstConst{fl, انگ_ٻٽو_انگ_نقاب}};
            andp->user1(سچ);
            Ast_سرڪايو_R* ثابت shiftp
                = جديد Ast_سرڪايو_R{fl, andp,
                                جديد AstConst{fl, AstConst::Widthed_قدر{}, argp->ويڪر(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(سچ);
            sump = جديد Ast_جوڙيو{fl, sump, shiftp};
            sump->user1(سچ);
        }
        أرجع sump;
    }

    فراغ گهمو(Ast_ڳڻپ_Ones* nodep) override {
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت argp = nodep->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* sump = build_ڳڻپ_Ones_Expansion(fl, argp);
        sump = adjust_ويڪر(fl, sump, nodep->ويڪر(), nodep->dtypep()->numeric());
        sump->user1(سچ);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        iterate(sump);
    }
    فراغ گهمو(AstRedXor* nodep) override {
        إذا (editFormat(nodep)) أرجع;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت argp = nodep->lhsp()->unlink_Fr_پٺ();

        Ast_ڳنڍ_اظهار* redxorp = جديد AstSel{fl, argp, 0, 1};
        redxorp->user1(سچ);
        لكل (صحيح i = 1; i < argp->ويڪر(); i++) {
            AstSel* ثابت selp = جديد AstSel{fl, argp->clone_وڻ_خالص(خطأ), i, 1};
            selp->user1(سچ);

            redxorp = جديد AstXor{fl, redxorp, selp};
            redxorp->user1(سچ);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        iterate(redxorp);
    }
    فراغ گهمو(AstRedAnd* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // Convert to (~x == 0)
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت argp = nodep->lhsp()->unlink_Fr_پٺ();
        ثابت V3_عدد انگ_Zero{fl, argp->ويڪر(), 0};
        Ast_ڳنڍ_اظهار* ثابت negp = جديد AstNot{fl, argp};
        negp->user1(سچ);
        Ast_ڳنڍ_اظهار* ثابت eqp = جديد AstEq{fl, negp, جديد AstConst{fl, انگ_Zero}};
        eqp->user1(سچ);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        iterate(eqp);
    }
    فراغ گهمو(AstRedOr* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // Convert to (x != 0)
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت argp = nodep->lhsp()->unlink_Fr_پٺ();
        ثابت V3_عدد انگ_Zero{fl, argp->ويڪر(), 0};
        Ast_ڳنڍ_اظهار* ثابت neqp = جديد AstNeq{fl, argp, جديد AstConst{fl, انگ_Zero}};
        neqp->user1(سچ);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        iterate(neqp);
    }
    فراغ گهمو(AstOneHot* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت argp = nodep->lhsp()->unlink_Fr_پٺ();
        ثابت صحيح w = argp->ويڪر();

        ثابت V3_عدد انگ_Zero{fl, w, 0};
        AstNeq* ثابت neZerop
            = جديد AstNeq{fl, argp->clone_وڻ_خالص(خطأ), جديد AstConst{fl, انگ_Zero}};
        neZerop->user1(سچ);

        ثابت V3_عدد انگ_One{fl, w, 1};
        AstSub* ثابت subp = جديد AstSub{fl, argp->clone_وڻ_خالص(خطأ), جديد AstConst{fl, انگ_One}};
        subp->user1(سچ);

        AstAnd* ثابت andp = جديد AstAnd{fl, argp, subp};
        andp->user1(سچ);

        ثابت V3_عدد انگ_Zero2{fl, w, 0};
        AstEq* ثابت eqZerop = جديد AstEq{fl, andp, جديد AstConst{fl, انگ_Zero2}};
        eqZerop->user1(سچ);

        AstLogAnd* ثابت resultp = جديد AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(سچ);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        iterate(resultp);
    }
    فراغ گهمو(AstOneHot0* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $onehot0(x) = (x & (x-1)) == 0
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت argp = nodep->lhsp()->unlink_Fr_پٺ();
        ثابت صحيح w = argp->ويڪر();

        ثابت V3_عدد انگ_One{fl, w, 1};
        AstSub* ثابت subp = جديد AstSub{fl, argp->clone_وڻ_خالص(خطأ), جديد AstConst{fl, انگ_One}};
        subp->dtypeFrom(argp);
        subp->user1(سچ);

        AstAnd* ثابت andp = جديد AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(سچ);

        ثابت V3_عدد انگ_Zero{fl, w, 0};
        AstEq* ثابت eqp = جديد AstEq{fl, andp, جديد AstConst{fl, انگ_Zero}};
        eqp->user1(سچ);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        iterate(eqp);
    }
    فراغ گهمو(Ast_ڳڻپ_بٽ* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سٽ* ثابت fl = nodep->fileline();

        منطقي ڳڻپ_Ones = خطأ;
        منطقي ڳڻپ_Zeros = خطأ;
        لكل (Ast_ڳنڍ_اظهار* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ثابت AstConst* ثابت cp = VN_CAST(ctrlp, Const);
            إذا (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ثابت zerop
                    = جديد AstConst{fl, AstConst::Widthed_قدر{}, nodep->ويڪر(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
                أرجع;
            }
            إذا (cp->انگ().ٻٽو_انگ_Is1(0) && !ڳڻپ_Ones)
                ڳڻپ_Ones = سچ;
            وإلا إذا (cp->انگ().ٻٽو_انگ_Is0(0) && !ڳڻپ_Zeros)
                ڳڻپ_Zeros = سچ;
        }

        Ast_ڳنڍ_اظهار* ثابت argp = nodep->lhsp()->unlink_Fr_پٺ();
        ثابت صحيح arg_ويڪر = argp->ويڪر();
        Ast_ڳنڍ_اظهار* sump = خالي_اشارو;
        إذا (ڳڻپ_Ones && ڳڻپ_Zeros) {
            // ones + zeros = width for 2-state types
            sump = جديد AstConst{fl, AstConst::Widthed_قدر{}, nodep->ويڪر(), (uint32_t)arg_ويڪر};
            VL_DO_DANGLING(argp->delete_وڻ(), argp);
        } وإلا إذا (ڳڻپ_Ones) {
            sump = build_ڳڻپ_Ones_Expansion(fl, argp);
        } وإلا إذا (ڳڻپ_Zeros) {
            // width - countones(x)
            Ast_ڳنڍ_اظهار* ثابت onesCountp = build_ڳڻپ_Ones_Expansion(fl, argp);
            ثابت V3_عدد ويڪر_Val{nodep, onesCountp->ويڪر(), (uint32_t)arg_ويڪر};
            sump = جديد AstSub{fl, جديد AstConst{fl, ويڪر_Val}, onesCountp};
            sump->user1(سچ);
        } وإلا {
            sump = جديد AstConst{fl, AstConst::Widthed_قدر{}, nodep->ويڪر(), 0U};
            VL_DO_DANGLING(argp->delete_وڻ(), argp);
        }

        sump = adjust_ويڪر(fl, sump, nodep->ويڪر(), nodep->dtypep()->numeric());
        sump->user1(سچ);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        iterate(sump);
    }
    فراغ گهمو(AstCLog2* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت argp = nodep->lhsp()->unlink_Fr_پٺ();
        ثابت صحيح arg_ويڪر = argp->ويڪر();
        ثابت صحيح نتيجو_ويڪر = nodep->ويڪر();

        Ast_ڳنڍ_اظهار* resultp
            = جديد AstConst{fl, AstConst::Widthed_قدر{}, نتيجو_ويڪر, (uint32_t)arg_ويڪر};

        لكل (صحيح k = arg_ويڪر - 1; k >= 0; k--) {
            V3_عدد حد{fl, arg_ويڪر, 0};
            إذا (k < 32)
                حد.سيٽ_Long(1ULL << k);
            وإلا
                حد.سيٽ_ٻٽو_انگ(k, 1);

            AstLte* ثابت ltep
                = جديد AstLte{fl, argp->clone_وڻ_خالص(خطأ), جديد AstConst{fl, حد}};
            ltep->user1(سچ);

            AstConst* ثابت valuep
                = جديد AstConst{fl, AstConst::Widthed_قدر{}, نتيجو_ويڪر, (uint32_t)k};

            resultp = جديد AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(سچ);
        }

        VL_DO_DANGLING(argp->delete_وڻ(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        iterate(resultp);
    }
    فراغ گهمو(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ثابت zerop = جديد AstConst{nodep->fileline(), AstConst::ٻٽو_انگ_False{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    فراغ handlePow(Ast_ڳنڍ_Biop* nodep) {
        إذا (AstConst* ثابت exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_سٽ* ثابت fl = nodep->fileline();
            Ast_ڳنڍ_اظهار* ثابت basep = nodep->lhsp();
            ثابت V3_عدد انگ_One{nodep, basep->ويڪر(), 1};
            Ast_ڳنڍ_اظهار* powerp = جديد AstConst{fl, انگ_One};
            ثابت منطقي بنياد_Signed = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ثابت int32_t exponent = بنياد_Signed ? exponentp->منزل_SInt() : exponentp->منزل_UInt();
            إذا (exponent > 0) {
                لكل (int32_t i = 0; i < exponent; i++) {
                    إذا (بنياد_Signed) {
                        powerp = جديد AstMulS{fl, powerp, basep->clone_وڻ_خالص(خطأ)};
                    } وإلا {
                        powerp = جديد AstMul{fl, powerp, basep->clone_وڻ_خالص(خطأ)};
                    }
                    powerp->user1(سچ);
                }
            } وإلا إذا (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                إذا (بنياد_Signed) {
                    powerp = جديد AstDivS{fl, powerp, basep->clone_وڻ_خالص(خطأ)};
                    powerp->user1(سچ);
                    powerp = جديد AstDivS{fl, powerp, basep->clone_وڻ_خالص(خطأ)};
                    powerp->user1(سچ);
                } وإلا {
                    powerp = جديد AstDiv{fl, powerp, basep->clone_وڻ_خالص(خطأ)};
                    powerp->user1(سچ);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            iterate(powerp);
        } وإلا إذا (ثابت AstConst* ثابت basep = VN_CAST(nodep->lhsp(), Const)) {
            إذا (basep->انگ().منزل_UInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_سٽ* ثابت fl = nodep->fileline();
                Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
                ثابت صحيح ويڪر = nodep->ويڪر();
                ثابت منطقي rhsDependent = rhsp->user1();
                AstConst* ثابت onep = جديد AstConst{fl, AstConst::Widthed_قدر{}, ويڪر, 1};
                Ast_سرڪايو_L* ثابت shiftp = جديد Ast_سرڪايو_L{fl, onep, rhsp, ويڪر};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_ڳنڍ_اظهار* resultp = shiftp;
                إذا (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_ڳنڍ_اظهار* ثابت nClonep = rhsp->clone_وڻ_خالص(خطأ);
                    nClonep->user1(rhsDependent);
                    AstConst* ثابت zeroNp
                        = جديد AstConst{fl, AstConst::Widthed_قدر{}, rhsp->ويڪر(), 0};
                    AstGteS* ثابت condp = جديد AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ثابت zeroRp = جديد AstConst{fl, AstConst::Widthed_قدر{}, ويڪر, 0};
                    resultp = جديد AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
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
    فراغ گهمو(AstPow* nodep) override { handlePow(nodep); }
    فراغ گهمو(AstPowSS* nodep) override { handlePow(nodep); }
    فراغ گهمو(AstPowSU* nodep) override { handlePow(nodep); }
    فراغ گهمو(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    فراغ handle_سرڪايو(Ast_ڳنڍ_Biop* nodep) {
        إذا (editFormat(nodep)) أرجع;
        ثابت صحيح lhs_ويڪر = nodep->lhsp()->ويڪر();
        ثابت صحيح rhs_ويڪر = nodep->rhsp()->ويڪر();
        إذا (rhs_ويڪر < lhs_ويڪر) {
            File_سٽ* ثابت fl = nodep->fileline();
            Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٺ();
            AstExtend* ثابت extendp = جديد AstExtend{fl, rhsp, lhs_ويڪر};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(سچ);
            nodep->rhsp(extendp);
        } وإلا إذا (rhs_ويڪر > lhs_ويڪر) {
            File_سٽ* ثابت fl = nodep->fileline();
            Ast_ڳنڍ_DType* ثابت dtypep = nodep->dtypep();
            Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٺ();
            Ast_ڳنڍ_اظهار* ثابت extendp
                = VN_IS(nodep, سرڪايو_RS)
                      ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstExtendS{fl, lhsp, rhs_ويڪر})
                      : static_cast<Ast_ڳنڍ_اظهار*>(جديد AstExtend{fl, lhsp, rhs_ويڪر});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(سچ);
            nodep->lhsp(extendp);
            nodep->dtype_سيٽ_Logic_Sized(rhs_ويڪر, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlink_Fr_پٺ(&relinker);
            AstSel* ثابت selp = جديد AstSel{fl, nodep, 0, lhs_ويڪر};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            أرجع;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ گهمو(Ast_سرڪايو_L* nodep) override { handle_سرڪايو(nodep); }
    فراغ گهمو(Ast_سرڪايو_R* nodep) override { handle_سرڪايو(nodep); }
    فراغ گهمو(Ast_سرڪايو_RS* nodep) override { handle_سرڪايو(nodep); }
    فراغ گهمو(Ast_ڳنڍ_Biop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Uniop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp());
    }
    فراغ گهمو(Ast_ڳنڍ_Triop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    فراغ گهمو(AstCond* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        إذا (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtype_سيٽ_String();
            أرجع;
        }
        // Fall back to "(ite cond then else)"
        گهمو(static_cast<Ast_ڳنڍ_Triop*>(nodep));
    }
    فراغ گهمو(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->srcp());
    }
    فراغ گهمو(AstSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        VNRelinker handle;
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت msbp = جديد AstSFormatF{
            fl, "%1d", خطأ,
            جديد Ast_جوڙيو{fl, nodep->lsbp()->clone_وڻ_خالص(خطأ),
                       جديد AstConst{fl, static_cast<uint32_t>(nodep->ويڪر_Const() - 1)}}};
        Ast_ڳنڍ_اظهار* ثابت lsbp
            = جديد AstSFormatF{fl, "%1d", خطأ, nodep->lsbp()->unlink_Fr_پٺ(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    فراغ گهمو(AstStructSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        ثابت منطقي outermost = !m_defer_Rand_گهڻائي_Hoist;
        Ast_ڳنڍ_اظهار* ثابت origp = outermost ? nodep->clone_وڻ(خطأ) : خالي_اشارو;
        VL_RESTORER(m_structSel);
        m_structSel = سچ;
        إذا (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_ڳنڍ_اظهار* ثابت fromp = nodep->fromp();
            AstStructDType* ثابت structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            إذا (is_Rand_Qualified_رستو(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } وإلا {
                structp->markConstrainedRand(سچ);
            }
        }
        // Mark Random for structArray
        إذا (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_ڳنڍ_اظهار* ثابت fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ثابت structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            إذا (is_Rand_Qualified_رستو(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } وإلا {
                structp->markConstrainedRand(سچ);
            }
        }
        {
            VL_RESTORER(m_defer_Rand_گهڻائي_Hoist);
            m_defer_Rand_گهڻائي_Hoist = سچ;
            iterateChildren(nodep);
        }
        File_سٽ* ثابت fl = nodep->fileline();
        AstSFormatF* newp = خالي_اشارو;
        إذا (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = جديد AstSFormatF{fl, "%s.%s." + nodep->name(), خطأ,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_وڻ_خالص(سچ)};
            إذا (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } وإلا {
            newp = جديد AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), خطأ,
                                   خالي_اشارو};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        إذا (origp && !hoist_Rand_گهڻائي_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(ڌڪو_Deletep(origp), origp);
        }
    }
    فراغ جوڙيو_String_Name_Part(Ast_ڳنڍ_اظهار* ثابت indexp, ثابت std::string& fmt = "%32x") {
        إذا (m_nestedAccess) {
            Ast_ڳنڍ_اظهار* ثابت bitp = indexp->clone_وڻ_خالص(خطأ);
            Ast_ڳنڍ_اظهار* ثابت ٻٽو_انگ_Formatp = جديد AstSFormatF{bitp->fileline(), fmt, خطأ, bitp};
            AstSFormatF* ثابت herep
                = جديد AstSFormatF{indexp->fileline(), "%s.%s", خطأ, ٻٽو_انگ_Formatp};
            m_nestedAccess->جوڙيو_Var_Name_Part(herep, indexp->name());
        }
    }
    فراغ گهمو(AstAssocSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سٽ* ثابت fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_ڳنڍ_اظهار* ثابت origp = nodep->clone_وڻ(خطأ);
        AstSFormatF* newp = خالي_اشارو;
        إذا (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            جوڙيو_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_ڳنڍ_اظهار* ثابت idxp = جديد AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), خطأ,
                                                      nodep->bitp()->unlink_Fr_پٺ(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وإلا إذا (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ثابت stringp = VN_AS(nodep->bitp(), CvtPackString);
            ثابت سائيز_t string_سائيز = VN_AS(stringp->lhsp(), Const)->ويڪر();
            إذا (string_سائيز > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << string_سائيز << "bits, limit is 128 bits");
            }
            جوڙيو_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_ڳنڍ_اظهار* ثابت idxp = جديد AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), خطأ,
                                                      stringp->lhsp()->unlink_Fr_پٺ(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وإلا {
            إذا (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ثابت صحيح actual_ويڪر = nodep->bitp()->ويڪر();
                std::string fmt;
                // Normalize to standard bit width
                إذا (actual_ويڪر <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } وإلا إذا (actual_ويڪر <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } وإلا {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::منزل_string(VL_لفظ_I(actual_ويڪر) * 8) + "x";
                }
                إذا (m_nestedAccess) جوڙيو_String_Name_Part(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_ڳنڍ_اظهار* ثابت idxp
                    = جديد AstSFormatF{fl, fmt, خطأ, nodep->bitp()->unlink_Fr_پٺ(&handle)};
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
        إذا (!newp || !hoist_Rand_گهڻائي_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(origp->delete_وڻ(), origp);
        }
    }
    فراغ rename_Array_اظهار(AstSFormatF* ثابت newp) ثابت {
        إذا (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    فراغ گهمو(AstArraySel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سٽ* ثابت fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        منطقي اشاريو_Is_Rand = خطأ;
        nodep->bitp()->foreach([&](ثابت Ast_ڳنڍ_Var_Ref* vrefp) {
            إذا (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                اشاريو_Is_Rand = سچ;
            }
        });
        إذا (m_nestedAccess) {
            Ast_ڳنڍ_اظهار* ثابت bitp = nodep->bitp()->clone_وڻ_خالص(خطأ);
            Ast_ڳنڍ_اظهار* ثابت ٻٽو_انگ_Formatp
                = اشاريو_Is_Rand ? جديد AstSFormatF{bitp->fileline(), bitp->name(), خطأ, خالي_اشارو}
                              : جديد AstSFormatF{bitp->fileline(), "%x", خطأ, bitp};
            AstSFormatF* ثابت herep
                = جديد AstSFormatF{nodep->fileline(), "%s.%s", خطأ, ٻٽو_انگ_Formatp};
            إذا (AstSel* ثابت selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->جوڙيو_Var_Name_Part(herep, selp->fromp()->name());
            } وإلا {
                m_nestedAccess->جوڙيو_Var_Name_Part(herep, bitp->name());
            }
            إذا (اشاريو_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_وڻ(), bitp);
                أرجع;
            }
        }
        إذا (اشاريو_Is_Rand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_ڳنڍ_اظهار* indexp = nodep->bitp()->unlink_Fr_پٺ(&handle);
            إذا (indexp->ويڪر() < 32) {
                AstExtend* ثابت extendp = جديد AstExtend{fl, indexp, 32};
                extendp->user1(سچ);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_اظهار(newp);
        } وإلا {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_ڳنڍ_اظهار* ثابت origp = nodep->clone_وڻ(خطأ);
            Ast_ڳنڍ_اظهار* ثابت indexp
                = جديد AstSFormatF{fl, "#x%8x", خطأ, nodep->bitp()->unlink_Fr_پٺ(&handle)};
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_اظهار(newp);
            إذا (!newp || !hoist_Rand_گهڻائي_Over_Select_And_Member(newp, origp)) {
                VL_DO_DANGLING(origp->delete_وڻ(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ساكن منطقي is_Rand_گهڻائي_Gate(ثابت Ast_ڳنڍ_اظهار* exprp) {
        ثابت AstCMethodHard* ثابت atp = VN_CAST(exprp, CMethodHard);
        إذا (!atp || atp->طريقو() != VCMethod::ARRAY_AT) أرجع خطأ;
        ثابت AstVar* varp = خالي_اشارو;
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
    منطقي hoist_Rand_گهڻائي_Over_Select_And_Member(AstSFormatF* newp, Ast_ڳنڍ_اظهار* origp) {
        إذا (m_defer_Rand_گهڻائي_Hoist) أرجع خطأ;
        // Only for selects yielding a non-array element (full chains)
        إذا (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::ويڪٽر<AstSFormatF*> چوکٺا;
        AstCond* گهڻائي_Condp = خالي_اشارو;
        لكل (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_ڳنڍ_اظهار* ثابت firstp = curp->exprsp();
            چوکٺا.ڌڪو_پٺ(curp);
            إذا (AstCond* ثابت condp = VN_CAST(firstp, Cond)) {
                إذا (is_Rand_گهڻائي_Gate(condp->condp())) گهڻائي_Condp = condp;
                اكسر;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        إذا (!گهڻائي_Condp) أرجع خطأ;
        File_سٽ* ثابت fl = newp->fileline();
        Ast_ڳنڍ_اظهار* ثابت modep = گهڻائي_Condp->condp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* activep = گهڻائي_Condp->thenp()->unlink_Fr_پٺ();
        // Rebuild the select chain text around the SMT name, innermost first
        لكل (خودڪار it = چوکٺا.rbegin(); it != چوکٺا.rend(); ++it) {
            Ast_ڳنڍ_اظهار* ثابت بچيل_Argsp = VN_CAST((*it)->exprsp()->nextp(), ڳنڍ_اظهار);
            إذا (بچيل_Argsp) { بچيل_Argsp->unlink_Fr_پٺ_With_ايندڙ(); }
            activep = جديد AstSFormatF{fl, (*it)->name(), خطأ,
                                      Ast_ڳنڍ::جوڙيو_ايندڙ(activep, بچيل_Argsp)};
        }
        AstCond* ثابت hoistp = جديد AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(سچ);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(ڌڪو_Deletep(newp), newp);
        أرجع سچ;
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        إذا (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_سٽ* ثابت fl = nodep->fileline();
            إذا (m_nestedAccess) {
                AstSFormatF* ثابت formatp = جديد AstSFormatF{fl, nodep->name(), خطأ, خالي_اشارو};
                m_nestedAccess->جوڙيو_Var_Name_Part(جديد AstSFormatF{fl, "%s.%s", خطأ, formatp},
                                               nodep->name());
            }
            Ast_ڳنڍ* پاڙ_ڳنڍ = nodep->fromp();
            طالما (ثابت AstMemberSel* ثابت selp = VN_CAST(پاڙ_ڳنڍ, MemberSel)) {
                پاڙ_ڳنڍ = selp->fromp();
            }

            Ast_ڳنڍ_Sel* arraySelp = VN_CAST(پاڙ_ڳنڍ, ArraySel);
            إذا (!arraySelp) arraySelp = VN_CAST(پاڙ_ڳنڍ, AssocSel);

            إذا (arraySelp) {
                Ast_ڳنڍ_DType* ثابت arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_ڳنڍ_DType* ثابت elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : خالي_اشارو;
                إذا (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ثابت منطقي isSimple
                        = nodep->fromp() == پاڙ_ڳنڍ && VN_IS(arraySelp->fromp(), VarRef);
                    إذا (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = جديد Nested_Access_رستو(nodep->clone_وڻ(خطأ),
                                                              &m_پهريون_اظهار_Inside_Indexp);
                        AstSFormatF* ثابت formatp
                            = جديد AstSFormatF{fl, nodep->name(), خطأ, خالي_اشارو};
                        m_nestedAccess->جوڙيو_Var_Name_Part(
                            جديد AstSFormatF{fl, "%s.%s", خطأ, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = سچ;
                    nodep->user1(سچ);
                    arraySelp->user1(سچ);
                    Ast_ڳنڍ_اظهار* ثابت origp = nodep->clone_وڻ(خطأ);
                    iterateChildren(nodep);
                    AstSFormatF* ثابت newp = جديد AstSFormatF{fl, "%s." + nodep->name(), خطأ,
                                                              nodep->fromp()->unlink_Fr_پٺ()};
                    nodep->replaceWith(newp);
                    إذا (!hoist_Rand_گهڻائي_Over_Select_And_Member(newp, origp))
                        VL_DO_DANGLING(origp->delete_وڻ(), origp);
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    VL_DO_DANGLING(ميٽيو m_nestedAccess, m_nestedAccess);
                    أرجع;
                }
            }
            // Check if the root variable participates in global constraints
            إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(پاڙ_ڳنڍ, VarRef)) {
                AstVar* ثابت constrainedVar = varRefp->varp();
                إذا (constrainedVar->globalConstrained()) {
                    منطقي ثابت beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    إذا (beforeIterate) {
                        nodep->replaceWith(جديد AstSFormatF{fl, "%s." + nodep->name(), خطأ,
                                                           nodep->fromp()->unlink_Fr_پٺ()});
                    } وإلا {
                        nodep->replaceWith(nodep->fromp()->unlink_Fr_پٺ());
                    }
                    VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
                    أرجع;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        إذا (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), ڳنڍ_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ثابت varRefp
                = جديد AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), ڳنڍ_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            گهمو(varRefp);
        } وإلا إذا (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->طريقو() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ثابت cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_ڳنڍ_DType* ثابت arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_ڳنڍ_DType* ثابت elemDtp = arrayDtp->subDTypep()->skipRefp();
            إذا (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = سچ;
                iterateChildren(nodep);
                File_سٽ* ثابت fl = nodep->fileline();
                AstSFormatF* newp = خالي_اشارو;
                إذا (AstSFormatF* ثابت fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    إذا (fromp->name() == "%s.%s") {
                        newp = جديد AstSFormatF{fl, "%s.%s." + nodep->name(), خطأ,
                                               fromp->exprsp()->clone_وڻ_خالص(سچ)};
                    } وإلا {
                        newp = جديد AstSFormatF{fl, fromp->name() + "." + nodep->name(), خطأ,
                                               خالي_اشارو};
                    }
                } وإلا {
                    newp = جديد AstSFormatF{fl, nodep->name(), خطأ, خالي_اشارو};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                أرجع;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_پٺ());
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            أرجع;
        } وإلا إذا (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_پٺ());
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            أرجع;
        } وإلا {
            editFormat(nodep);
        }
    }
    فراغ گهمو(AstSFormatF* nodep) override {}
    فراغ گهمو(Ast_Stmt_اظهار* nodep) override {}
    فراغ گهمو(AstCStmt* nodep) override {}
    فراغ گهمو(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    فراغ گهمو(AstConstraintIf* nodep) override {
        Ast_ڳنڍ_اظهار* newp = خالي_اشارو;
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_اظهار* ثابت thenp = editSingle(fl, nodep->thensp());
        Ast_ڳنڍ_اظهار* ثابت elsep = editSingle(fl, nodep->elsesp());
        إذا (thenp && elsep) {
            newp = جديد AstCond{fl, nodep->condp()->unlink_Fr_پٺ(), thenp, elsep};
        } وإلا إذا (thenp) {
            newp = جديد AstLogIf{fl, nodep->condp()->unlink_Fr_پٺ(), thenp};
        } وإلا إذا (elsep) {
            newp = جديد AstLogIf{fl, جديد AstNot{fl, nodep->condp()->unlink_Fr_پٺ()}, elsep};
        }
        إذا (newp) {
            newp->dtype_سيٽ_ٻٽو_انگ();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(سچ);  // Assume result-dependent
            nodep->replaceWith(جديد Ast_Constraint_اظهار{fl, newp});
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    فراغ گهمو(AstBegin* nodep) override {}
    فراغ گهمو(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_سٽ* ثابت fl = nodep->fileline();

        إذا (!nodep->bodyp()) {
            nodep->unlink_Fr_پٺ();
        } وإلا إذا (m_wantSingle) {
            Ast_ڳنڍ_اظهار* ثابت itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            cstmtp->جوڙيو("ret += \" \";\n");
            cstmtp->جوڙيو("ret += ");
            cstmtp->جوڙيو(itemp);
            cstmtp->جوڙيو(";");
            Ast_ڳنڍ* ثابت bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->dtype_سيٽ_String();
            cexprp->جوڙيو("([&]{\nstd::string ret;\n");
            cexprp->جوڙيو(جديد AstBegin{fl, m_unique_نالا.get("__VrandBlock"),
                                     جديد AstForeach{fl, nodep->headerp()->unlink_Fr_پٺ(), bodyp},
                                     سچ});
            cexprp->جوڙيو("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
        } وإلا {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            لكل (Ast_ڳنڍ* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                إذا (ثابت AstVar* ثابت varp = VN_CAST(elemp, Var)) m_foreachIdxVars.وجهو(varp);
            }
            iterate_And_ايندڙ_Null(nodep->bodyp());
            Ast_ڳنڍ* ثابت bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlink_Fr_پٺ_With_ايندڙ());
            nodep->replaceWith(
                جديد AstBegin{fl, m_unique_نالا.get("__VrandBlock"),
                             جديد AstForeach{fl, nodep->headerp()->unlink_Fr_پٺ(), bodyp}, سچ});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_Module* ثابت genModp = VN_AS(m_genp->user2p(), ڳنڍ_Module);

        لكل (Ast_ڳنڍ_اظهار* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), ڳنڍ_اظهار)) {
            Ast_ڳنڍ_اظهار* ثابت lhsTestp = build_Solve_Before_Name_اظهار(fl, lhsp);
            إذا (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                استمر;
            }
            VL_DO_DANGLING(lhsTestp->delete_وڻ(), lhsTestp);
            لكل (Ast_ڳنڍ_اظهار* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), ڳنڍ_اظهار)) {
                Ast_ڳنڍ_اظهار* ثابت rhsNamep = build_Solve_Before_Name_اظهار(fl, rhsp);
                إذا (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    استمر;
                }
                AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                    fl, جديد AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtype_سيٽ_Void();
                callp->جوڙيو_Pinsp(build_Solve_Before_Name_اظهار(fl, lhsp));
                callp->جوڙيو_Pinsp(rhsNamep);
                nodep->جوڙيو_هتي_This_As_ايندڙ(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstConstraintUnique* nodep) override {
        Ast_ڳنڍ_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_سٽ* ثابت fl = nodep->fileline();
        Ast_ڳنڍ_Module* genModp = خالي_اشارو;

        إذا (m_classp) {
            initTaskp = VN_AS(m_memberMap.ڳوليو_Member(m_classp, "new"), ڳنڍ_FTask);
            genVarp = getRandomGenerator(m_classp);
        } وإلا {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        إذا (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }
        إذا (m_classp) {
            genModp = VN_AS(genVarp->user2p(), ڳنڍ_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_ڳنڍ* تياري_Stmtsp = خالي_اشارو;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::ويڪٽر<std::string> smtExprs;
        لكل (Ast_ڳنڍ* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            إذا (AstVarRef* ثابت varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ثابت varp = varRefp->varp();
                Ast_ڳنڍ_Module* ثابت varModp = VN_AS(varp->user2p(), ڳنڍ_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_ڳنڍ_DType* ثابت dtypep = varp->dtypep()->skipRefp();

                ثابت ساكن خودڪار dynDTypeSupported
                    = [](ثابت Ast_ڳنڍ_DType* ثابت dtypep) -> منطقي {
                    أرجع VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, قطار_DType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elem_ويڪر;
                uint32_t static_سائيز = 0;  // Element count, zero when only known at run time
                إذا (ثابت AstUnpackArrayDType* ثابت مٿي = VN_CAST(dtypep, UnpackArrayDType)) {
                    ثابت AstRange* ثابت rangep = مٿي->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    static_سائيز = مٿي->elementsConst();
                    إذا (static_سائيز > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        استمر;
                    }
                    elem_ويڪر = static_cast<uint64_t>(varp->dtypep()->ويڪر());
                } وإلا إذا (dynDTypeSupported(dtypep)) {
                    elem_ويڪر = static_cast<uint64_t>(dtypep->subDTypep()->ويڪر());
                } وإلا {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(خطأ));
                    استمر;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ثابت Ast_ڳنڍ_DType* ثابت subp = dtypep->subDTypep()->skipRefp();
                إذا (VN_IS(subp, ڳنڍ_Array_DType) || VN_IS(subp, قطار_DType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    استمر;
                }

                إذا (m_wantSingle && !static_سائيز) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    استمر;
                }

                إذا (!varp->user3()) {
                    varp->user3(سچ);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_ڳنڍ_اظهار* ثابت varnamep = جديد AstCExpr{
                        fl, AstCExpr::خالص{}, "\"" + varp->name() + "\"", varp->ويڪر()};

                    AstCMethodHard* ثابت writeVarCallp = جديد AstCMethodHard{
                        fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ثابت argVarRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                    إذا (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->جوڙيو_Pinsp(argVarRefp);
                    writeVarCallp->جوڙيو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, elem_ويڪر});
                    writeVarCallp->جوڙيو_Pinsp(varnamep);
                    writeVarCallp->جوڙيو_Pinsp(جديد AstConst{fl, 1});  // Dimension
                    ثابت Randomize_گهڻائي rand_گهڻائي = {.asUQuad = varp->user1()};
                    إذا (rand_گهڻائي.استعمال_ڪري_گهڻائي) {
                        writeVarCallp->جوڙيو_Pinsp(
                            جديد AstConst{fl, AstConst::Unsized64{}, rand_گهڻائي.اشاريو});
                    }
                    writeVarCallp->dtype_سيٽ_Void();
                    initTaskp->جوڙيو_Stmtsp(جديد Ast_Stmt_اظهار{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                إذا (varp->user4p() && m_سائيز_Constrained_Arraysp) {
                    m_سائيز_Constrained_Arraysp->وجهو(varp);
                }

                إذا (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    إذا (static_سائيز < 2) استمر;
                    std::string اظهار_Str = "(__Vbv (distinct";
                    لكل (uint32_t i = 0; i < static_سائيز; ++i) {
                        اظهار_Str += " (select " + varp->name() + " #x";
                        لكل (صحيح سرڪايو_2 = 28; سرڪايو_2 >= 0; سرڪايو_2 -= 4) {
                            اظهار_Str += "0123456789abcdef"[(i >> سرڪايو_2) & 0xf];
                        }
                        اظهار_Str += ')';
                    }
                    smtExprs.emplace_پٺ(اظهار_Str + "))");
                    استمر;
                }

                Ast_ڳنڍ_اظهار* ثابت namep = جديد AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ثابت randUniqueCallp
                    = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->ڳوليو_Void_DType());
                randUniqueCallp->جوڙيو_Pinsp(جديد AstConst{fl, *m_uniqueConstraintId});
                تياري_Stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(تياري_Stmtsp, جديد Ast_Stmt_اظهار{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        إذا (m_wantSingle && !smtExprs.خالي_جايون()) {
            std::string اظهار_Str = smtExprs.front();
            إذا (smtExprs.سائيز() > 1) {
                اظهار_Str = "(bvand";
                لكل (ثابت std::string& oneExprp : smtExprs) اظهار_Str += " " + oneExprp;
                اظهار_Str += ')';
            }
            AstSFormatF* ثابت newp = جديد AstSFormatF{fl, اظهار_Str, خطأ, خالي_اشارو};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (تياري_Stmtsp) nodep->جوڙيو_هتي_This_As_ايندڙ(تياري_Stmtsp);
        nodep->unlink_Fr_پٺ();
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }

    فراغ گهمو(Ast_Constraint_اظهار* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_پهريون_اظهار_Inside_Indexp);
        إذا (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            أرجع;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_ڳنڍ_اظهار* ثابت exprp = nodep->exprp();
            إذا (exprp->ويڪر() > 1) {
                File_سٽ* ثابت fl = exprp->fileline();
                ثابت V3_عدد انگ_Zero{fl, exprp->ويڪر(), 0};
                Ast_ڳنڍ_اظهار* ثابت neqp
                    = جديد AstNeq{fl, exprp->unlink_Fr_پٺ(), جديد AstConst{fl, انگ_Zero}};
                neqp->user1(سچ);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        إذا (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_پٺ());
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            أرجع;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ثابت VCMethod طريقو
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ڳنڍ_Module), m_genp,
                          VAccess::READWRITE},
            طريقو, nodep->exprp()->unlink_Fr_پٺ()};
        callp->dtype_سيٽ_Void();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->جوڙيو_Pinsp(جديد AstCExpr{nodep->fileline(), AstCExpr::خالص{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ثابت uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->جوڙيو_Pinsp(جديد AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string pretty_متن;
        إذا (!v3Global.opt.protectIds()) {
            pretty_متن = nodep->fileline()->pretty_ذريعو();
            سائيز_t pos = 0;
            طالما ((pos = pretty_متن.ڳوليو('"', pos)) != std::string::npos) {
                pretty_متن.وجهو(pos, "\\");
                pos += std::لڙي_ڊيگهه("\\\"");
            }
        }
        callp->جوڙيو_Pinsp(
            جديد AstCExpr{nodep->fileline(), AstCExpr::خالص{}, "\"" + pretty_متن + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(AstCMethodHard* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سٽ* ثابت fl = nodep->fileline();

        إذا (nodep->طريقو() == VCMethod::ARRAY_AT) {
            جوڙيو_String_Name_Part(nodep->pinsp(), "%x");
            ثابت منطقي اشاريو_Is_Rand = nodep->pinsp()->exists([](ثابت Ast_ڳنڍ_Var_Ref* ثابت vrefp) {
                أرجع vrefp->varp()->rand().isRandomizable();
            });
            إذا (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ثابت Ast_ڳنڍ* ثابت basep = nodep->بنياد_Fromp(سچ);
                    إذا (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        أرجع;
                    }
                }
                Ast_ڳنڍ_اظهار* ثابت origp = اشاريو_Is_Rand ? خالي_اشارو : nodep->clone_وڻ(خطأ);
                ثابت AstVarRef* ثابت بنياد_Refp
                    = m_structSel ? VN_CAST(nodep->fromp()->بنياد_Fromp(سچ), VarRef) : خالي_اشارو;
                إذا (بنياد_Refp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(بنياد_Refp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_ڳنڍ_اظهار* ثابت indexp = nodep->pinsp()->clone_وڻ_خالص(خطأ);
                    Ast_ڳنڍ_اظهار* ثابت arrayp = nodep->fromp()->clone_وڻ_خالص(خطأ);
                    AstCMethodHard* ثابت currentSizep
                        = جديد AstCMethodHard{fl, arrayp->clone_وڻ_خالص(خطأ), VCMethod::DYN_سائيز};
                    currentSizep->dtype_سيٽ_Int();
                    Ast_ڳنڍ_اظهار* ثابت اندريون_Range_Requiredp = جديد AstLogAnd{
                        fl,
                        جديد AstLteS{fl,
                                    جديد AstConst{fl, AstConst::Widthed_قدر{}, indexp->ويڪر(), 0},
                                    indexp->clone_وڻ_خالص(خطأ)},
                        جديد AstGteS{fl, indexp->clone_وڻ_خالص(خطأ), currentSizep}};
                    AstCMethodHard* ثابت resizep
                        = جديد AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             جديد Ast_جوڙيو{fl, indexp, جديد AstConst{fl, 1}}};
                    resizep->dtype_سيٽ_Void();
                    m_prepareConstrainedArraysp->جوڙيو_Stmtsp(
                        جديد AstIf{fl, اندريون_Range_Requiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ثابت sizep
                    = m_structSel ? جديد AstCMethodHard{fl, nodep->fromp()->clone_وڻ_خالص(خطأ),
                                                       VCMethod::DYN_سائيز}
                                  : خالي_اشارو;
                Ast_ڳنڍ_اظهار* ثابت originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_ڳنڍ_اظهار* ثابت pinp = nodep->pinsp()->unlink_Fr_پٺ();
                AstSFormatF* newp;
                إذا (m_structSel) {
                    إذا (AstSFormatF* ثابت sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_ڳنڍ_اظهار* ثابت argsp
                        = Ast_ڳنڍ::جوڙيو_ايندڙ(nodep->fromp()->unlink_Fr_پٺ(), pinp);
                    sizep->dtype_سيٽ_Int();
                    AstLogAnd* ثابت condp
                        = جديد AstLogAnd{fl,
                                        جديد AstLteS{fl,
                                                    جديد AstConst{fl, AstConst::Widthed_قدر{},
                                                                 originalPinp->ويڪر(), 0},
                                                    originalPinp->clone_وڻ_خالص(خطأ)},
                                        جديد AstLtS{fl, originalPinp->clone_وڻ_خالص(خطأ), sizep}};
                    m_conditionp = m_conditionp ? جديد AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = جديد AstSFormatF{fl, "%s.%s", خطأ, argsp};
                } وإلا {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_پٺ(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
                إذا (origp && !hoist_Rand_گهڻائي_Over_Select_And_Member(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_وڻ(), origp);
                }

            } وإلا إذا (اشاريو_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } وإلا {
                nodep->user1(خطأ);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            أرجع;
        }

        إذا (nodep->طريقو() == VCMethod::ARRAY_INSIDE) {
            ثابت منطقي randArr = nodep->fromp()->user1();

            AstVar* ثابت newVarp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->ڳوليو_Int_DType()};
            Ast_ڳنڍ_اظهار* ثابت idxRefp = جديد AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            Ast_Foreach_سرو* ثابت headerp
                = جديد Ast_Foreach_سرو{fl, nodep->fromp()->clone_وڻ_خالص(خطأ), newVarp};
            Ast_ڳنڍ_اظهار* ثابت selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_ڳنڍ* ثابت itemp = جديد AstEq{fl, selp, nodep->pinsp()->unlink_Fr_پٺ()};
            itemp->user1(سچ);

            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            cstmtp->جوڙيو("ret += \" \";\n");
            cstmtp->جوڙيو("ret += ");
            cstmtp->جوڙيو(iterateSubtreeReturnEdits(itemp));
            cstmtp->جوڙيو(";");
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->dtype_سيٽ_String();
            cexprp->جوڙيو("([&]{\nstd::string ret;\n");
            cexprp->جوڙيو(جديد AstBegin{fl, m_unique_نالا.get("__VrandBlock"),
                                     جديد AstForeach{fl, headerp, cstmtp}, سچ});
            cexprp->جوڙيو("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            أرجع;
        }

        // Array reduction methods (sum, product, and, or, xor)
        إذا (nodep->طريقو() == VCMethod::ARRAY_R_جوڙ
            || nodep->طريقو() == VCMethod::ARRAY_R_حاصل_ضرب
            || nodep->طريقو() == VCMethod::ARRAY_R_AND || nodep->طريقو() == VCMethod::ARRAY_R_OR
            || nodep->طريقو() == VCMethod::ARRAY_R_XOR) {

            AstWith* ثابت withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            إذا (withp) {
                إذا (AstUnpackArrayDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ثابت صحيح array_سائيز_2 = adtypep->elementsConst();
                    إذا (array_سائيز_2 > v3Global.opt.constraint_Array_حد_بندي()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvt_منزل_Str(array_سائيز_2)
                                          + " exceeds --constraint-array-limit of "
                                          + cvt_منزل_Str(v3Global.opt.constraint_Array_حد_بندي())
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
                = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->ڳوليو_Int_DType()};
            Ast_Foreach_سرو* ثابت headerp
                = جديد Ast_Foreach_سرو{fl, nodep->fromp()->clone_وڻ_خالص(خطأ), loopVarp};

            // Determine SMT operation and compute identity elements
            ثابت حرف* smtOp = خالي_اشارو;
            std::string سڃاڻپ_عنصر;
            إذا (withp) {
                // For 'with' clause: use binary format, compute from result width
                ثابت صحيح ويڪر = nodep->dtypep()->ويڪر();
                إذا (nodep->طريقو() == VCMethod::ARRAY_R_جوڙ) {
                    smtOp = "bvadd";
                    سڃاڻپ_عنصر = "#b" + std::string(ويڪر, '0');
                } وإلا إذا (nodep->طريقو() == VCMethod::ARRAY_R_حاصل_ضرب) {
                    smtOp = "bvmul";
                    سڃاڻپ_عنصر = (ويڪر > 0) ? "#b" + std::string(ويڪر - 1, '0') + "1" : "#b0";
                } وإلا إذا (nodep->طريقو() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    سڃاڻپ_عنصر = "#b" + std::string(ويڪر, '1');
                } وإلا إذا (nodep->طريقو() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    سڃاڻپ_عنصر = "#b" + std::string(ويڪر, '0');
                } وإلا {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    سڃاڻپ_عنصر = "#b" + std::string(ويڪر, '0');
                }
            } وإلا {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ثابت arrVarp = arrRefp->varp();
                Ast_ڳنڍ_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحيح elem_ويڪر = elemDtp->ويڪر();
                ثابت صحيح hexDigits = (elem_ويڪر + 3) / 4;

                ثابت std::string zero_سڃاڻپ_عنصر = "#x" + std::string(hexDigits, '0');
                ثابت std::string one_سڃاڻپ_عنصر = "#x" + std::string(hexDigits - 1, '0') + "1";
                ثابت std::string all_Ones_سڃاڻپ_عنصر = "#x" + std::string(hexDigits, 'f');

                إذا (nodep->طريقو() == VCMethod::ARRAY_R_جوڙ) {
                    smtOp = "bvadd";
                    سڃاڻپ_عنصر = zero_سڃاڻپ_عنصر;
                } وإلا إذا (nodep->طريقو() == VCMethod::ARRAY_R_حاصل_ضرب) {
                    smtOp = "bvmul";
                    سڃاڻپ_عنصر = one_سڃاڻپ_عنصر;
                } وإلا إذا (nodep->طريقو() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    سڃاڻپ_عنصر = all_Ones_سڃاڻپ_عنصر;
                } وإلا إذا (nodep->طريقو() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    سڃاڻپ_عنصر = zero_سڃاڻپ_عنصر;
                } وإلا {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    سڃاڻپ_عنصر = zero_سڃاڻپ_عنصر;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            إذا (withp) {
                // With 'with' clause: evaluate expression for each element
                ثابت منطقي randArr = nodep->fromp()->user1();
                Ast_ڳنڍ_اظهار* ثابت idxRefp = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_ڳنڍ_اظهار* ثابت elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ثابت صحيح نتيجو_ويڪر = nodep->dtypep()->ويڪر();
                ثابت VSigning نتيجو_Signing = nodep->dtypep()->numeric();

                Ast_ڳنڍ* في_Elem_Exprp = withp->exprp()->clone_وڻ_خالص(خطأ);
                إذا (AstLambdaArgRef* ثابت پاڙ_Refp = VN_CAST(في_Elem_Exprp, LambdaArgRef)) {
                    إذا (پاڙ_Refp->اشاريو()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ثابت loopRef = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                        في_Elem_Exprp = adjust_ويڪر(fl, loopRef, نتيجو_ويڪر, نتيجو_Signing);
                    } وإلا {
                        // item at root -> replace with array element
                        في_Elem_Exprp = elemSelp->clone_وڻ_خالص(خطأ);
                    }
                    VL_DO_DANGLING(پاڙ_Refp->delete_وڻ(), پاڙ_Refp);
                } وإلا {
                    في_Elem_Exprp->foreach([&](AstLambdaArgRef* refp) {
                        إذا (refp->اشاريو()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ثابت loopRef = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_ڳنڍ* ثابت replacement
                                = adjust_ويڪر(fl, loopRef, نتيجو_ويڪر, نتيجو_Signing);
                            refp->replaceWith(replacement);
                        } وإلا {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_وڻ_خالص(خطأ));
                        }
                        VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_وڻ(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                في_Elem_Exprp = V3Const::constifyEdit(في_Elem_Exprp);

                في_Elem_Exprp->foreach([&](Ast_ڳنڍ* nodep) {
                    // Don't mark loop variable references as randomizable
                    إذا (!VN_IS(nodep, Const)) {
                        إذا (Ast_ڳنڍ_Var_Ref* ثابت vrefp = VN_CAST(nodep, ڳنڍ_Var_Ref)) {
                            إذا (vrefp->varp() != loopVarp) nodep->user1(سچ);
                        } وإلا {
                            nodep->user1(سچ);
                        }
                    }
                });

                cstmtp->جوڙيو("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->جوڙيو("ret += ");
                cstmtp->جوڙيو(iterateSubtreeReturnEdits(في_Elem_Exprp));
                cstmtp->جوڙيو(";\n");
                cstmtp->جوڙيو("ret += \")\";\n");
            } وإلا {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ثابت arrVarp = arrRefp->varp();
                ثابت std::string smtArrayName = arrVarp->name();
                Ast_ڳنڍ_DType* ثابت elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحيح elem_ويڪر = elemDtp->ويڪر();
                Ast_ڳنڍ_Module* ثابت arrModulep = VN_AS(arrVarp->user2p(), ڳنڍ_Module);
                Ast_ڳنڍ_Module* ثابت genModulep = VN_AS(m_genp->user2p(), ڳنڍ_Module);
                arrVarp->user3(سچ);

                // Add write_var call to init task
                Ast_ڳنڍ_اظهار* ثابت arrVarNamep
                    = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + smtArrayName + "\"", elem_ويڪر};
                AstCMethodHard* ثابت writeVarCallp
                    = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->جوڙيو_Pinsp(جديد AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->جوڙيو_Pinsp(
                    جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elem_ويڪر)});
                writeVarCallp->جوڙيو_Pinsp(arrVarNamep);
                ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(خطأ).ٻيو;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->جوڙيو_Pinsp(جديد AstConst{fl, 1});  // Dimension

                ثابت Randomize_گهڻائي rand_گهڻائي = {.asUQuad = arrVarp->user1()};
                إذا (rand_گهڻائي.استعمال_ڪري_گهڻائي) {
                    writeVarCallp->جوڙيو_Pinsp(
                        جديد AstConst{fl, AstConst::Unsized64{}, rand_گهڻائي.اشاريو});
                }
                writeVarCallp->dtype_سيٽ_Void();
                Ast_ڳنڍ_FTask* initTaskp = m_inlineInitTaskp;
                إذا (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.ڳوليو_Member(arrModulep, "new"), ڳنڍ_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->جوڙيو_Stmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ثابت idxFormatp = جديد AstSFormatF{
                    fl, "#x%08x", خطأ, جديد AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ثابت varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_پٺ(), idxFormatp);

                cstmtp->جوڙيو("ret += \" \";\n");
                cstmtp->جوڙيو("ret += ");
                cstmtp->جوڙيو(varNamep);
                cstmtp->جوڙيو(";\n");
            }

            // Build final expression
            إذا (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
                cexprp->dtype_سيٽ_String();
                cexprp->جوڙيو("([&]{\nstd::string ret = \"" + سڃاڻپ_عنصر + "\";\n");
                cexprp->جوڙيو(جديد AstBegin{fl, m_unique_نالا.get("__VrandBlock"),
                                         جديد AstForeach{fl, headerp, cstmtp}, سچ});
                cexprp->جوڙيو("return ret;\n})()");
                nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            } وإلا {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
                cexprp->dtype_سيٽ_String();
                cexprp->جوڙيو("([&]{\nstd::string ret;\n");
                cexprp->جوڙيو(جديد AstBegin{fl, m_unique_نالا.get("__VrandBlock"),
                                         جديد AstForeach{fl, headerp, cstmtp}, سچ});
                cexprp->جوڙيو("return ret.empty() ? \"" + سڃاڻپ_عنصر + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            أرجع;
        }

        // We've handled DYN_SIZE already if they should be randomized
        إذا (nodep->طريقو() == VCMethod::DYN_سائيز || nodep->طريقو() == VCMethod::ASSOC_سائيز) {
            Ast_ڳنڍ_اظهار* ڳنڍ_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            طالما (VN_IS(ڳنڍ_Exprp, MemberSel)) {
                ڳنڍ_Exprp = VN_AS(ڳنڍ_Exprp, MemberSel)->fromp();
            }
            إذا (VN_IS(ڳنڍ_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_وڻ_خالص(خطأ)));
                VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
                أرجع;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(خطأ);

        إذا (editFormat(nodep)) أرجع;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        إذا (editFormat(nodep)) أرجع;

        AstFunc* ثابت funcp = VN_CAST(nodep->taskp(), Func);
        إذا (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            أرجع;
        }

        Ast_ڳنڍ_اظهار* ثابت retExprp = ڪڍو_Return_اظهار(funcp);
        إذا (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(خطأ);
            إذا (editFormat(nodep)) أرجع;
            أرجع;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), خالي_اشارو, خطأ);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_ڳنڍ_اظهار* inlinedp = retExprp->clone_وڻ_خالص(خطأ);
        لكل (ثابت خودڪار& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.پهريون;
            AstArg* ثابت argp = tconnect.ٻيو;
            إذا (!argp || !argp->exprp()) استمر;
            إذا (AstVarRef* ثابت پاڙ_Refp = VN_CAST(inlinedp, VarRef)) {
                إذا (پاڙ_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_وڻ_خالص(خطأ);
                    VL_DO_DANGLING(پاڙ_Refp->delete_وڻ(), پاڙ_Refp);
                    استمر;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                إذا (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_وڻ_خالص(خطأ));
                    VL_DO_DANGLING(refp->delete_وڻ(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {}
    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_ڳنڍ* buildDisableSoftCallStmt(Ast_Constraint_اظهار* nodep) {
        std::string varName;
        إذا (ثابت Ast_ڳنڍ_Var_Ref* ثابت vrefp = VN_CAST(nodep->exprp(), ڳنڍ_Var_Ref)) {
            varName = vrefp->name();
        } وإلا إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } وإلا {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ڳنڍ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            جديد AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtype_سيٽ_Void();
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
    Ast_ڳنڍ* ڪڍو_Conditional_Disable_Softs(Ast_ڳنڍ* itemsp, Ast_ڳنڍ_اظهار* outerCondp) {
        Ast_ڳنڍ* hoistListp = خالي_اشارو;
        لكل (Ast_ڳنڍ *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (AstConstraintIf* ثابت ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_سٽ* ثابت fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_ڳنڍ_اظهار* ثابت thenCondp
                    = outerCondp ? static_cast<Ast_ڳنڍ_اظهار*>(
                                       جديد AstLogAnd{fl, outerCondp->clone_وڻ_خالص(خطأ),
                                                     ifp->condp()->clone_وڻ_خالص(خطأ)})
                                 : ifp->condp()->clone_وڻ_خالص(خطأ);
                إذا (Ast_ڳنڍ* ثابت thenHoistp
                    = ڪڍو_Conditional_Disable_Softs(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_ڳنڍ::جوڙيو_ايندڙ(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_وڻ(), thenCondp);
                إذا (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_ڳنڍ_اظهار* ثابت notInnerp
                        = جديد AstLogNot{fl, ifp->condp()->clone_وڻ_خالص(خطأ)};
                    Ast_ڳنڍ_اظهار* ثابت elseCondp
                        = outerCondp ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstLogAnd{
                                           fl, outerCondp->clone_وڻ_خالص(خطأ), notInnerp})
                                     : notInnerp;
                    إذا (Ast_ڳنڍ* ثابت elseHoistp
                        = ڪڍو_Conditional_Disable_Softs(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_ڳنڍ::جوڙيو_ايندڙ(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_وڻ(), elseCondp);
                }
            } وإلا إذا (Ast_Constraint_اظهار* ثابت exprp = VN_CAST(stmtp, Constraint_اظهار)) {
                إذا (exprp->isDisableSoft() && outerCondp) {
                    File_سٽ* ثابت fl = exprp->fileline();
                    AstIf* ثابت hoistIfp = جديد AstIf{fl, outerCondp->clone_وڻ_خالص(خطأ),
                                                      buildDisableSoftCallStmt(exprp), خالي_اشارو};
                    hoistListp = hoistListp ? Ast_ڳنڍ::جوڙيو_ايندڙ(hoistListp,
                                                               static_cast<Ast_ڳنڍ*>(hoistIfp))
                                            : static_cast<Ast_ڳنڍ*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_اظهار* ثابت truep
                        = جديد Ast_Constraint_اظهار{fl, جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_وڻ(), exprp);
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
    واضح Constraint_اظهار_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_ڳنڍ* nodep,
                                   Ast_ڳنڍ_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* rand_گهڻائي_Varp, std::سيٽ<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3_Unique_نالا& unique_نالا,
                                   Ast_ڳنڍ_FTask* memberselInitTaskp = خالي_اشارو,
                                   VInsertion_سيٽ<AstVar*>* سائيز_Constrained_Arraysp = خالي_اشارو,
                                   Ast_ڳنڍ_FTask* prepareConstrainedArraysp = خالي_اشارو)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_rand_گهڻائي_Varp{rand_گهڻائي_Varp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_سائيز_Constrained_Arraysp{سائيز_Constrained_Arraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_unique_نالا{unique_نالا} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        إذا (Ast_ڳنڍ* ثابت hoistListp
            = nodep ? ڪڍو_Conditional_Disable_Softs(nodep, خالي_اشارو) : خالي_اشارو) {
            nodep->جوڙيو_ايندڙ(hoistListp);
        }
        iterate_And_ايندڙ_Null(nodep);
    }
};

تعداد صنف Capture_گهڻائي : uint8_t {
    CAP_NO = 0x0,
    CAP_قدر = 0x01,
    CAP_THIS = 0x02,
    CAP_F_سيٽ_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
Capture_گهڻائي عامل|(Capture_گهڻائي a, Capture_گهڻائي b) {
    أرجع static_cast<Capture_گهڻائي>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
Capture_گهڻائي عامل&(Capture_گهڻائي a, Capture_گهڻائي b) {
    أرجع static_cast<Capture_گهڻائي>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
Capture_گهڻائي گهڻائي(Capture_گهڻائي a) { أرجع a & static_cast<Capture_گهڻائي>(0x3); }
منطقي hasFlags(Capture_گهڻائي a, Capture_گهڻائي flags) {
    أرجع ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

صنف CaptureVisitor final : عام VNVisitor {
    AstArg* m_argsp = خالي_اشارو;  // Original references turned into arguments
    Ast_ڳنڍ_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ثابت AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::سيٽ<Ast_ڳنڍ*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = خالي_اشارو;  // Variable for outer context's object, if necessary
    منطقي m_staticContext = خطأ;  // True when capturing from a static function context

    // METHODS

    منطقي captureVariable(File_سٽ* ثابت fileline, Ast_ڳنڍ_Var_Ref* varrefp, AstVar*& varp) {
        خودڪار it = m_varCloneMap.ڳوليو(varrefp->varp());
        إذا (it == m_varCloneMap.پڇاڙي()) {
            AstVar* ثابت newVarp = varrefp->varp()->clone_وڻ(خطأ);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(سچ);
            newVarp->direction(VDirection::داخلا);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            أرجع سچ;
        }
        varp = it->ٻيو;
        أرجع خطأ;
    }

    سانچو <قسم_نالو T_ڳنڍ>
    فراغ fixupClassOrPackage(Ast_ڳنڍ* memberp, T_ڳنڍ refp) {
        Ast_ڳنڍ_Module* ثابت declClassp = VN_AS(memberp->user2p(), ڳنڍ_Module);
        إذا (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    سانچو <قسم_نالو T_ڳنڍ>
    منطقي is_Reference_منزل_Inner_Member(T_ڳنڍ nodep) {
        أرجع VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_سٽ* fl) {
        إذا (!m_thisp) {
            AstClassRefDType* ثابت refDTypep
                = جديد AstClassRefDType{fl, VN_AS(m_callerp, Class), خالي_اشارو};
            v3Global.rootp()->typeTablep()->جوڙيو_Typesp(refDTypep);
            m_thisp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(سچ);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::داخلا);
            m_argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(m_argsp, جديد AstArg{fl, "", جديد AstThisRef{fl, refDTypep}});
        }
        أرجع m_thisp;
    }

    AstVar* getVar(AstVar* ثابت varp) ثابت {
        ثابت خودڪار it = m_varCloneMap.ڳوليو(varp);
        إذا (it == m_varCloneMap.پڇاڙي()) أرجع خالي_اشارو;
        إذا (it->ٻيو->isStdRandomizeArg()) أرجع خالي_اشارو;
        أرجع it->ٻيو;
    }

    Capture_گهڻائي get_Var_Ref_Capture_گهڻائي(Ast_ڳنڍ_Var_Ref* varRefp) {
        Ast_ڳنڍ_Module* ثابت varModp = VN_AS(varRefp->varp()->user2p(), ڳنڍ_Module);
        AstClass* ثابت varClassp = VN_CAST(varModp, Class);
        AstClass* ثابت callerClassp = VN_CAST(m_callerp, Class);

        ثابت منطقي callerIsClass = callerClassp;
        ثابت منطقي refIsXref = VN_IS(varRefp, VarXRef);
        ثابت منطقي varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ثابت منطقي varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ثابت منطقي var_Is_ميدان_جو_Caller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ثابت منطقي varIsParam = varRefp->varp()->isParam();
        ثابت منطقي varIsConstraintIterator
            = VN_IS(varRefp->varp()->پهريون_Abovep(), Foreach_سرو)
              && VN_IS(varRefp->varp()->پهريون_Abovep()->پهريون_Abovep(), ConstraintForeach);
        إذا (refIsXref) أرجع Capture_گهڻائي::CAP_قدر | Capture_گهڻائي::CAP_F_XREF;
        إذا (varIsConstraintIterator) أرجع Capture_گهڻائي::CAP_NO;
        إذا (varIsFuncLocal && varHasAutomaticLifetime) أرجع Capture_گهڻائي::CAP_قدر;
        إذا (varIsParam) أرجع Capture_گهڻائي::CAP_قدر;
        // Static var in function (will not be inlined, because it's in class)
        إذا (callerIsClass && varIsFuncLocal) أرجع Capture_گهڻائي::CAP_قدر;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        إذا (m_staticContext && callerIsClass && var_Is_ميدان_جو_Caller
            && varRefp->varp()->lifetime().isStatic())
            أرجع Capture_گهڻائي::CAP_NO;
        إذا (callerIsClass && var_Is_ميدان_جو_Caller) أرجع Capture_گهڻائي::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        إذا (callerIsClass && varClassp && !var_Is_ميدان_جو_Caller
            && varRefp->varp()->lifetime().isStatic())
            أرجع Capture_گهڻائي::CAP_قدر;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        أرجع Capture_گهڻائي::CAP_قدر;
    }

    فراغ capture_Ref_By_قدر(Ast_ڳنڍ_Var_Ref* nodep, Capture_گهڻائي cap_گهڻائي_Flags) {
        AstVar* newVarp;
        ثابت منطقي newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_ڳنڍ_Var_Ref* ثابت newVarRefp = newCapture ? nodep->clone_وڻ(خطأ) : خالي_اشارو;
        إذا (!hasFlags(cap_گهڻائي_Flags, Capture_گهڻائي::CAP_F_سيٽ_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(خالي_اشارو);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        إذا (!newCapture) أرجع;
        إذا (hasFlags(cap_گهڻائي_Flags, Capture_گهڻائي::CAP_F_XREF)) {
            AstVarRef* ثابت notXVarRefp
                = جديد AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_ڳنڍ::جوڙيو_ايندڙ(m_argsp, جديد AstArg{nodep->fileline(), "", newVarRefp});
    }

    فراغ captureRefByThis(Ast_ڳنڍ_Var_Ref* nodep, Capture_گهڻائي /*capModeFlags*/) {
        AstVar* ثابت thisp = importThisp(nodep->fileline());
        AstVarRef* ثابت thisRefp = جديد AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(سچ);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ثابت memberSelp
            = جديد AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        إذا (!m_targetp && nodep->user1()) memberSelp->user1(سچ);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        إذا (m_ignore.ڳڻپ(nodep)) أرجع;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        إذا (nodep->varp()->isStdRandomizeArg()) أرجع;
        ثابت Capture_گهڻائي cap_گهڻائي = get_Var_Ref_Capture_گهڻائي(nodep);
        إذا (گهڻائي(cap_گهڻائي) == Capture_گهڻائي::CAP_NO) أرجع;
        إذا (گهڻائي(cap_گهڻائي) == Capture_گهڻائي::CAP_قدر) capture_Ref_By_قدر(nodep, cap_گهڻائي);
        إذا (گهڻائي(cap_گهڻائي) == Capture_گهڻائي::CAP_THIS) captureRefByThis(nodep, cap_گهڻائي);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        إذا (m_ignore.ڳڻپ(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        إذا (VN_IS(nodep, طريقو_Call) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            أرجع;
        }
        AstClass* ثابت classp = VN_CAST(nodep->taskp()->user2p(), Class);
        إذا ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ثابت argsp = nodep->argsp();
            إذا (argsp) argsp->unlink_Fr_پٺ();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ثابت thisp = importThisp(nodep->fileline());
            AstVarRef* ثابت thisRefp = جديد AstVarRef{
                nodep->fileline(), thisp, nodep->is_خالص() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            Ast_طريقو_Call* ثابت طريقو_Callp
                = جديد Ast_طريقو_Call{nodep->fileline(), thisRefp, thisp->name(), argsp};
            طريقو_Callp->taskp(nodep->taskp());
            طريقو_Callp->dtypep(nodep->dtypep());
            nodep->replaceWith(طريقو_Callp);
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
            m_ignore.emplace(طريقو_Callp);
        }
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        إذا (!is_Reference_منزل_Inner_Member(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        AstVarRef* ثابت varRefp
            = جديد AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    فراغ گهمو(Ast_طريقو_Call* nodep) override {
        إذا (!is_Reference_منزل_Inner_Member(nodep) || m_ignore.ڳڻپ(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        AstArg* ثابت argsp = nodep->argsp();
        إذا (argsp) argsp->unlink_Fr_پٺ_With_ايندڙ();
        Ast_ڳنڍ_FTask_Ref* taskRefp = خالي_اشارو;
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
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    واضح CaptureVisitor(Ast_ڳنڍ* ثابت nodep, Ast_ڳنڍ_Module* callerp, AstClass* ثابت targetp,
                            منطقي staticContext = خطأ)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterate_And_ايندڙ_Null(nodep);
    }

    // PUBLIC METHODS

    AstArg* get_دليلون() ثابت { أرجع m_argsp; }
    AstVar* getThisp() ثابت { أرجع m_thisp; }

    فراغ جوڙيو_Function_Arguments(Ast_ڳنڍ_FTask* funcp) ثابت {
        لكل (AstArg* argp = get_دليلون(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            إذا (Ast_ڳنڍ_Var_Ref* varrefp = VN_CAST(argp->exprp(), ڳنڍ_Var_Ref)) {
                إذا ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(خالي_اشارو);
                }
                funcp->جوڙيو_Stmtsp(getVar(varrefp->varp()));
            } وإلا {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->جوڙيو_Stmtsp(m_thisp);
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
    ثابت VNUser3_اندريون_Use m_inuser3;
    ثابت VNUser4_اندريون_Use m_inuser4;

    // STATE
    V3_Unique_نالا m_inline_Unique_نالا;  // For generating unique function names
    V3_Unique_نالا m_rand_Array_Unique_نالا{"__Vrandarr"};  // Random array loop indices
    V3_Unique_نالا m_گهڻائي_Unique_نالا{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3_Unique_نالا m_inlineUniqueStdName{"__VStdrand"};
    V3_Unique_نالا m_unique_نالا;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    std::unordered_map<Ast_ڳنڍ_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Current function/task
    Ast_ڳنڍ_Stmt* m_stmtp = خالي_اشارو;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = خالي_اشارو;  // Dynamic array type (for rand mode)
    صحيح m_rand_Case_انگ = 0;  // Randcase number within a module for var naming
    صحيح m_dist_انگ = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = خالي_اشارو;  // Current constraint
    std::سيٽ<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertion_سيٽ<AstVar*>> m_سائيز_Constrained_Arrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_static_Constraint_گهڻائي_Vars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_static_Rand_گهڻائي_Vars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<منطقي, منطقي>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ساكن منطقي is_Similar_ڳنڍ(ثابت Ast_ڳنڍ_اظهار* with_اظهار, ثابت Ast_ڳنڍ_اظهار* arg_اظهار) {
        // VarRef: compare variable pointers
        إذا (VN_IS(arg_اظهار, VarRef) && VN_IS(with_اظهار, VarRef)) {
            أرجع VN_AS(with_اظهار, VarRef)->varp() == VN_AS(arg_اظهار, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        إذا (VN_IS(arg_اظهار, MemberSel) && VN_IS(with_اظهار, MemberSel)) {
            ثابت AstMemberSel* ثابت withMSp = VN_AS(with_اظهار, MemberSel);
            ثابت AstMemberSel* ثابت argMSp = VN_AS(arg_اظهار, MemberSel);
            إذا (withMSp->varp() != argMSp->varp()) أرجع خطأ;
            // Recursively compare the base object expression
            أرجع is_Similar_ڳنڍ(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        إذا (VN_IS(arg_اظهار, ArraySel) && VN_IS(with_اظهار, ArraySel)) {
            ثابت AstArraySel* ثابت withASp = VN_AS(with_اظهار, ArraySel);
            ثابت AstArraySel* ثابت argASp = VN_AS(arg_اظهار, ArraySel);
            // Before checking Sel type index, check if both are Const
            إذا (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ثابت AstConst* ثابت withIdxp = VN_AS(withASp->bitp(), Const);
                ثابت AstConst* ثابت argIdxp = VN_AS(argASp->bitp(), Const);
                أرجع is_Similar_ڳنڍ(withASp->fromp(), argASp->fromp())
                       && withIdxp->انگ().isCaseEq(argIdxp->انگ());
            }
            // Index must be Sel type, extract VarRef using fromp()
            إذا (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) أرجع خطأ;
            ثابت Ast_ڳنڍ_اظهار* ثابت withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ثابت Ast_ڳنڍ_اظهار* ثابت argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            أرجع is_Similar_ڳنڍ(withASp->fromp(), argASp->fromp())
                   && is_Similar_ڳنڍ(withIdxp, argIdxp);
        }
        أرجع خطأ;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ساكن فراغ expandUniqueElementList(Ast_ڳنڍ* itemsp) {
        Ast_ڳنڍ* itemp = itemsp;
        طالما (itemp) {
            Ast_ڳنڍ* ثابت nextp = itemp->nextp();
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

            std::ويڪٽر<Ast_ڳنڍ_اظهار*> اظهار_شيون;
            منطقي hasArrayVarRef = خطأ;
            لكل (Ast_ڳنڍ* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                إذا (ثابت AstVarRef* ثابت vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ثابت Ast_ڳنڍ_DType* ثابت dtypep = vrp->varp()->dtypep()->skipRefp();
                    إذا (VN_IS(dtypep, ڳنڍ_Array_DType) || VN_IS(dtypep, قطار_DType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = سچ;
                        استمر;
                    }
                }
                اظهار_شيون.ڌڪو_پٺ(VN_AS(rp, ڳنڍ_اظهار));
            }

            إذا (اظهار_شيون.سائيز() >= 2) {
                File_سٽ* ثابت fl = uniquep->fileline();
                لكل (سائيز_t i = 0; i < اظهار_شيون.سائيز(); i++) {
                    لكل (سائيز_t j = i + 1; j < اظهار_شيون.سائيز(); j++) {
                        Ast_ڳنڍ_اظهار* ثابت lhsp = اظهار_شيون[i]->clone_وڻ(خطأ);
                        Ast_ڳنڍ_اظهار* ثابت rhsp = اظهار_شيون[j]->clone_وڻ(خطأ);
                        AstNeq* ثابت neqp = جديد AstNeq{fl, lhsp, rhsp};
                        neqp->user1(سچ);
                        Ast_Constraint_اظهار* ثابت cexprp = جديد Ast_Constraint_اظهار{fl, neqp};
                        uniquep->جوڙيو_ايندڙ_هتي(cexprp);
                    }
                }
                إذا (!hasArrayVarRef) {
                    uniquep->unlink_Fr_پٺ();
                    VL_DO_DANGLING(uniquep->delete_وڻ(), uniquep);
                }
            } وإلا إذا (اظهار_شيون.سائيز() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlink_Fr_پٺ();
                VL_DO_DANGLING(uniquep->delete_وڻ(), uniquep);
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
                                        classp->ڳوليو_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->جوڙيو_Membersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_ڳنڍ_Module* ثابت modp) {
        خودڪار it = m_stdMap.ڳوليو(modp);
        إذا (it == m_stdMap.پڇاڙي()) {
            AstVar* ثابت stdgenp
                = جديد AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->ڳوليو_Basic_DType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3_Error_ڪوڊ::IMPURE, سچ);
            modp->جوڙيو_Stmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            أرجع stdgenp;
        }
        أرجع it->ٻيو;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ثابت classp) {
        ساكن ثابت حرف* ثابت name = "__Vprepare_constrained_arrays";
        AstTask* ثابت taskp = جديد AstTask{classp->fileline(), name, خالي_اشارو};
        taskp->class_طريقو(سچ);
        classp->جوڙيو_Membersp(taskp);
        m_memberMap.وجهو(classp, taskp);
        أرجع taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ثابت classp) {
        ساكن constexpr ثابت حرف* ثابت name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.ڳوليو_Member(classp, name), Task);
        إذا (resizeTaskp) أرجع resizeTaskp;
        resizeTaskp = جديد AstTask{classp->fileline(), name, خالي_اشارو};
        resizeTaskp->class_طريقو(سچ);
        resizeTaskp->isVirtual(سچ);
        classp->جوڙيو_Membersp(resizeTaskp);
        m_memberMap.وجهو(classp, resizeTaskp);
        أرجع resizeTaskp;
    }
    AstVar* get_Create_Rand_گهڻائي_Var(AstClass* ثابت classp) {
        إذا (classp->user2p()) أرجع VN_AS(classp->user2p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Rand_گهڻائي_Var(extendsp->classp());
        }
        AstVar* ثابت rand_گهڻائي_Varp = create_گهڻائي_Var(classp, "__Vrandmode");
        classp->user2p(rand_گهڻائي_Varp);
        أرجع rand_گهڻائي_Varp;
    }
    AstVar* get_Create_Constraint_گهڻائي_Var(AstClass* ثابت classp) {
        إذا (classp->user4p()) أرجع VN_AS(classp->user4p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Constraint_گهڻائي_Var(extendsp->classp());
        }
        AstVar* ثابت constraint_گهڻائي_Varp = create_گهڻائي_Var(classp, "__Vconstraintmode");
        classp->user4p(constraint_گهڻائي_Varp);
        أرجع constraint_گهڻائي_Varp;
    }
    ساكن AstVar* get_Constraint_گهڻائي_Var(AstClass* ثابت classp) {
        إذا (classp->user4p()) أرجع VN_AS(classp->user4p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Constraint_گهڻائي_Var(extendsp->classp());
        }
        أرجع خالي_اشارو;
    }
    AstVar* get_Create_Static_Constraint_گهڻائي_Var(AstClass* ثابت classp) {
        خودڪار it = m_static_Constraint_گهڻائي_Vars.ڳوليو(classp);
        إذا (it != m_static_Constraint_گهڻائي_Vars.پڇاڙي()) أرجع it->ٻيو;
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Static_Constraint_گهڻائي_Var(extendsp->classp());
        }
        AstVar* ثابت static_گهڻائي_Varp = create_Static_گهڻائي_Var(classp, "__Vstaticconstraintmode");
        m_static_Constraint_گهڻائي_Vars[classp] = static_گهڻائي_Varp;
        أرجع static_گهڻائي_Varp;
    }
    AstVar* get_Static_Constraint_گهڻائي_Var(AstClass* ثابت classp) {
        خودڪار it = m_static_Constraint_گهڻائي_Vars.ڳوليو(classp);
        إذا (it != m_static_Constraint_گهڻائي_Vars.پڇاڙي()) أرجع it->ٻيو;
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Static_Constraint_گهڻائي_Var(extendsp->classp());
        }
        أرجع خالي_اشارو;
    }
    AstVar* get_Create_Static_Rand_گهڻائي_Var(AstClass* ثابت classp) {
        إذا (m_static_Rand_گهڻائي_Vars.ڳڻپ(classp)) أرجع m_static_Rand_گهڻائي_Vars[classp];
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Static_Rand_گهڻائي_Var(extendsp->classp());
        }
        AstVar* ثابت static_گهڻائي_Varp = create_Static_گهڻائي_Var(classp, "__Vstaticrandmode");
        m_static_Rand_گهڻائي_Vars[classp] = static_گهڻائي_Varp;
        أرجع static_گهڻائي_Varp;
    }
    AstVar* get_Static_Rand_گهڻائي_Var(AstClass* ثابت classp) {
        إذا (m_static_Rand_گهڻائي_Vars.ڳڻپ(classp)) أرجع m_static_Rand_گهڻائي_Vars[classp];
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Static_Rand_گهڻائي_Var(extendsp->classp());
        }
        أرجع خالي_اشارو;
    }
    AstVar* create_گهڻائي_Var(AstClass* ثابت classp, ثابت حرف* ثابت name) {
        File_سٽ* ثابت fl = classp->fileline();
        إذا (!m_dynarrayDtp) {
            m_dynarrayDtp = جديد AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->ڳوليو_ٻٽو_انگ_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->جوڙيو_Typesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ثابت گهڻائي_Varp = جديد AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        گهڻائي_Varp->user2p(classp);
        classp->جوڙيو_Stmtsp(گهڻائي_Varp);
        أرجع گهڻائي_Varp;
    }
    AstVar* create_Static_گهڻائي_Var(AstClass* ثابت classp, ثابت حرف* ثابت name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_سٽ* ثابت fl = classp->fileline();
        إذا (!m_dynarrayDtp) {
            m_dynarrayDtp = جديد AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->ڳوليو_ٻٽو_انگ_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->جوڙيو_Typesp(m_dynarrayDtp);
        }
        AstVar* ثابت گهڻائي_Varp = جديد AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        گهڻائي_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        گهڻائي_Varp->user2p(classp);
        classp->جوڙيو_Stmtsp(گهڻائي_Varp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.وجهو(classp, گهڻائي_Varp);
        أرجع گهڻائي_Varp;
    }
    ساكن فراغ جوڙيو_سيٽ_Rand_گهڻائي(Ast_ڳنڍ_FTask* ثابت ftaskp, AstVar* ثابت genp,
                               AstVar* ثابت rand_گهڻائي_Varp) {
        File_سٽ* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت سيٽ_Rand_Modep = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, VN_AS(genp->user2p(), ڳنڍ_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_سيٽ_RANDMODE,
            جديد AstVarRef{fl, VN_AS(rand_گهڻائي_Varp->user2p(), ڳنڍ_Module), rand_گهڻائي_Varp,
                          VAccess::READ}};
        سيٽ_Rand_Modep->dtype_سيٽ_Void();
        ftaskp->جوڙيو_Stmtsp(سيٽ_Rand_Modep->makeStmt());
    }
    ساكن فراغ جوڙيو_سيٽ_Static_Rand_گهڻائي(Ast_ڳنڍ_FTask* ثابت ftaskp, AstVar* ثابت genp,
                                     AstVar* ثابت static_Rand_گهڻائي_Varp) {
        File_سٽ* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت setp = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, VN_AS(genp->user2p(), ڳنڍ_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_سيٽ_STATIC_RANDMODE,
            جديد AstVarRef{fl, VN_AS(static_Rand_گهڻائي_Varp->user2p(), ڳنڍ_Module), static_Rand_گهڻائي_Varp,
                          VAccess::READ}};
        setp->dtype_سيٽ_Void();
        ftaskp->جوڙيو_Stmtsp(setp->makeStmt());
    }
    فراغ createRandomizeClassVars(AstNetlist* ثابت netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> پاڙ_Static_Rand_گهڻائي_ڳڻپ;
        netlistp->foreach([هذا, &پاڙ_Static_Rand_گهڻائي_ڳڻپ](AstClass* ثابت classp) {
            منطقي hasConstraints = خطأ;
            uint32_t rand_گهڻائي_ڳڻپ = 0;
            uint32_t static_Rand_گهڻائي_ڳڻپ = 0;
            uint32_t constraint_گهڻائي_ڳڻپ = 0;
            uint32_t static_Constraint_گهڻائي_ڳڻپ = 0;
            classp->foreachMember([&](AstClass*, Ast_ڳنڍ* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                إذا (AstConstraint* ثابت constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = سچ;
                    Randomize_گهڻائي constraint_گهڻائي = {.asUQuad = memberp->user1()};
                    إذا (!constraint_گهڻائي.استعمال_ڪري_گهڻائي) أرجع;
                    إذا (constraint_گهڻائي.اشاريو == 0) {
                        // Use separate index counters for static vs non-static constraints
                        إذا (constrp->isStatic()) {
                            constraint_گهڻائي.اشاريو = static_Constraint_گهڻائي_ڳڻپ++;
                        } وإلا {
                            constraint_گهڻائي.اشاريو = constraint_گهڻائي_ڳڻپ++;
                        }
                        memberp->user1(constraint_گهڻائي.asUQuad);
                    } وإلا {
                        إذا (constrp->isStatic()) {
                            static_Constraint_گهڻائي_ڳڻپ = constraint_گهڻائي.اشاريو + 1;
                        } وإلا {
                            constraint_گهڻائي_ڳڻپ = constraint_گهڻائي.اشاريو + 1;
                        }
                    }
                } وإلا إذا (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
                    Randomize_گهڻائي rand_گهڻائي = {.asUQuad = memberp->user1()};
                    إذا (!rand_گهڻائي.استعمال_ڪري_گهڻائي) أرجع;
                    ثابت منطقي isStaticVar = varp->lifetime().isStatic();
                    إذا (rand_گهڻائي.اشاريو == 0) {
                        إذا (isStaticVar) {
                            rand_گهڻائي.اشاريو = static_Rand_گهڻائي_ڳڻپ++;
                        } وإلا {
                            rand_گهڻائي.اشاريو = rand_گهڻائي_ڳڻپ++;
                        }
                        memberp->user1(rand_گهڻائي.asUQuad);
                    } وإلا {
                        إذا (isStaticVar) {
                            static_Rand_گهڻائي_ڳڻپ = rand_گهڻائي.اشاريو + 1;
                        } وإلا {
                            rand_گهڻائي_ڳڻپ = rand_گهڻائي.اشاريو + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<فراغ(AstClass*)> ڳوليو_Sub_Obj_Rand_Modes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_ڳنڍ* subMemberp) {
                        إذا (AstVar* ثابت subVarp = VN_CAST(subMemberp, Var)) {
                            ثابت Randomize_گهڻائي rm = {.asUQuad = subVarp->user1()};
                            إذا (!rm.استعمال_ڪري_گهڻائي) أرجع;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            إذا (subVarp->lifetime().isStatic()) أرجع;
                            ثابت uint32_t needed = rm.اشاريو + 1;
                            إذا (needed > rand_گهڻائي_ڳڻپ) rand_گهڻائي_ڳڻپ = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    إذا (!memberVarp->globalConstrained()) أرجع;
                    ثابت Ast_ڳنڍ_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                    إذا (!classRefp) أرجع;
                    ڳوليو_Sub_Obj_Rand_Modes(classRefp->classp());
                });
            }
            إذا (hasConstraints) createRandomGenerator(classp);
            إذا (rand_گهڻائي_ڳڻپ > 0) {
                AstVar* ثابت rand_گهڻائي_Varp = get_Create_Rand_گهڻائي_Var(classp);
                make_گهڻائي_Init(rand_گهڻائي_Varp, classp, rand_گهڻائي_ڳڻپ);
            }
            إذا (constraint_گهڻائي_ڳڻپ > 0) {
                AstVar* ثابت constraint_گهڻائي_Varp = get_Create_Constraint_گهڻائي_Var(classp);
                make_گهڻائي_Init(constraint_گهڻائي_Varp, classp, constraint_گهڻائي_ڳڻپ);
            }
            إذا (static_Constraint_گهڻائي_ڳڻپ > 0) {
                AstVar* ثابت static_Constraint_گهڻائي_Varp = get_Create_Static_Constraint_گهڻائي_Var(classp);
                make_Static_گهڻائي_Init(static_Constraint_گهڻائي_Varp, classp, static_Constraint_گهڻائي_ڳڻپ);
            }
            إذا (static_Rand_گهڻائي_ڳڻپ > 0) {
                get_Create_Static_Rand_گهڻائي_Var(classp);
                AstClass* rootp = classp;
                طالما (AstClassExtends* ثابت ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& جڳهه_2 = پاڙ_Static_Rand_گهڻائي_ڳڻپ[rootp];
                إذا (static_Rand_گهڻائي_ڳڻپ > جڳهه_2) جڳهه_2 = static_Rand_گهڻائي_ڳڻپ;
            }
        });
        لكل (ثابت خودڪار& kv : پاڙ_Static_Rand_گهڻائي_ڳڻپ) emit_پاڙ_Static_گهڻائي_Init(kv.پهريون, kv.ٻيو);
    }
    فراغ emit_پاڙ_Static_گهڻائي_Init(AstClass* ثابت rootp, ثابت uint32_t ڳڻپ) {
        AstVar* ثابت static_Rand_گهڻائي_Varp = m_static_Rand_گهڻائي_Vars[rootp];
        UASSERT_OBJ(static_Rand_گهڻائي_Varp, rootp, "Root must have a static rand-mode var");
        make_Static_گهڻائي_Init(static_Rand_گهڻائي_Varp, rootp, ڳڻپ);
    }
    فراغ make_گهڻائي_Init(AstVar* گهڻائي_Varp, AstClass* classp, uint32_t گهڻائي_ڳڻپ) {
        Ast_ڳنڍ_Module* ثابت گهڻائي_Var_Modp = VN_AS(گهڻائي_Varp->user2p(), ڳنڍ_Module);
        File_سٽ* ثابت fl = گهڻائي_Varp->fileline();
        AstVar* ثابت old_سائيز_Varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, m_unique_نالا.get("__VoldSize"), گهڻائي_Varp->ڳوليو_UInt32_DType()};
        old_سائيز_Varp->funcLocal(سچ);
        old_سائيز_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت oldSizep
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, گهڻائي_Var_Modp, گهڻائي_Varp, VAccess::READ},
                                 VCMethod::DYN_سائيز, خالي_اشارو};
        oldSizep->dtype_سيٽ_UInt32();
        AstCMethodHard* ثابت dynarrayNewp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, گهڻائي_Var_Modp, گهڻائي_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جديد AstConst{fl, گهڻائي_ڳڻپ}};
        dynarrayNewp->dtype_سيٽ_Void();
        Ast_ڳنڍ_FTask* ثابت ctorNewp = VN_AS(m_memberMap.ڳوليو_Member(classp, "new"), ڳنڍ_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_ڳنڍ* ثابت initFirstp = old_سائيز_Varp;
        initFirstp->جوڙيو_ايندڙ(
            جديد Ast_سونپيو{fl, جديد AstVarRef{fl, old_سائيز_Varp, VAccess::WRITE}, oldSizep});
        initFirstp->جوڙيو_ايندڙ(dynarrayNewp->makeStmt());
        initFirstp->جوڙيو_ايندڙ(make_گهڻائي_سيٽ_Loop(
            fl, جديد AstVarRef{fl, گهڻائي_Var_Modp, گهڻائي_Varp, VAccess::WRITE}, جديد AstConst{fl, 1},
            سچ, جديد AstVarRef{fl, old_سائيز_Varp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        لكل (Ast_ڳنڍ* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (!VN_IS(stmtp, ڳنڍ_Stmt)) استمر;  // Skip var declarations
            إذا (ثابت Ast_Stmt_اظهار* ثابت sep = VN_CAST(stmtp, Stmt_اظهار)) {
                إذا (VN_IS(sep->exprp(), New)) استمر;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->جوڙيو_هتي_This_As_ايندڙ(initFirstp);
            أرجع;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->جوڙيو_Stmtsp(initFirstp);
    }
    فراغ make_Static_گهڻائي_Init(AstVar* گهڻائي_Varp, AstClass* classp, uint32_t گهڻائي_ڳڻپ) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_ڳنڍ_Module* ثابت گهڻائي_Var_Modp = VN_AS(گهڻائي_Varp->user2p(), ڳنڍ_Module);
        File_سٽ* ثابت fl = گهڻائي_Varp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ثابت sizep
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, گهڻائي_Var_Modp, گهڻائي_Varp, VAccess::READ},
                                 VCMethod::DYN_سائيز, خالي_اشارو};
        sizep->dtype_سيٽ_UInt32();
        AstEq* ثابت condp = جديد AstEq{fl, sizep, جديد AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ثابت dynarrayNewp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, گهڻائي_Var_Modp, گهڻائي_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جديد AstConst{fl, گهڻائي_ڳڻپ}};
        dynarrayNewp->dtype_سيٽ_Void();
        Ast_ڳنڍ* ثابت thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->جوڙيو_ايندڙ(
            make_گهڻائي_سيٽ_Loop(fl, جديد AstVarRef{fl, گهڻائي_Var_Modp, گهڻائي_Varp, VAccess::WRITE},
                            جديد AstConst{fl, 1}, سچ));

        // Build the if statement
        AstIf* ثابت ifp = جديد AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_ڳنڍ_FTask* ثابت newp = VN_AS(m_memberMap.ڳوليو_Member(classp, "new"), ڳنڍ_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->جوڙيو_Stmtsp(ifp);
    }
    Ast_ڳنڍ* make_گهڻائي_سيٽ_Loop(File_سٽ* ثابت fl, Ast_ڳنڍ_اظهار* ثابت lhsp, Ast_ڳنڍ_اظهار* ثابت rhsp,
                             منطقي اندريون_Task, Ast_ڳنڍ_اظهار* ثابت startp = خالي_اشارو) {
        AstVar* ثابت iterVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->ڳوليو_UInt32_DType()};
        iterVarp->funcLocal(اندريون_Task);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت sizep = جديد AstCMethodHard{fl, lhsp, VCMethod::DYN_سائيز, خالي_اشارو};
        sizep->dtype_سيٽ_UInt32();
        AstCMethodHard* ثابت setp
            = جديد AstCMethodHard{fl, lhsp->clone_وڻ(خطأ), VCMethod::ARRAY_AT_WRITE,
                                 جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtype_سيٽ_UInt32();
        Ast_ڳنڍ* ثابت stmtsp = iterVarp;
        stmtsp->جوڙيو_ايندڙ(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : جديد AstConst{fl, 0}});

        AstLoop* ثابت loopp = جديد AstLoop{fl};
        stmtsp->جوڙيو_ايندڙ(loopp);
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{
            fl, loopp, جديد AstLt{fl, جديد AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, setp, rhsp});
        loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{
            fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
            جديد Ast_جوڙيو{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, iterVarp, VAccess::READ}}});
        أرجع جديد AstBegin{fl, m_unique_نالا.get("__VrandBlock"), stmtsp, سچ};
    }
    فراغ جوڙيو_عارضي_Varsp(AstVar* varsp) {
        لكل (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (m_ftaskp) {
                varp->funcLocal(سچ);
            } وإلا {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        إذا (!m_ftaskp) {
            m_modp->جوڙيو_Stmtsp(varsp);
        } وإلا {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_ڳنڍ_Stmt* wrap_If_Rand_گهڻائي(AstClass* classp, AstVar* ثابت varp, Ast_ڳنڍ_Stmt* stmtp) {
        ثابت Randomize_گهڻائي rmode = {.asUQuad = varp->user1()};
        AstVar* ثابت گهڻائي_Varp = varp->lifetime().isStatic() ? get_Static_Rand_گهڻائي_Var(classp)
                                                             : get_Rand_گهڻائي_Var_From_Class(classp);
        أرجع VN_AS(wrap_If_گهڻائي(rmode, گهڻائي_Varp, stmtp), ڳنڍ_Stmt);
    }
    Ast_ڳنڍ* wrap_If_Constraint_گهڻائي(AstClass* classp, AstConstraint* ثابت constrp, Ast_ڳنڍ* stmtp) {
        ثابت Randomize_گهڻائي rmode = {.asUQuad = constrp->user1()};
        AstVar* ثابت گهڻائي_Varp = constrp->isStatic() ? get_Static_Constraint_گهڻائي_Var(classp)
                                                     : get_Constraint_گهڻائي_Var(classp);
        أرجع wrap_If_گهڻائي(rmode, گهڻائي_Varp, stmtp);
    }
    ساكن Ast_ڳنڍ* wrap_If_گهڻائي(ثابت Randomize_گهڻائي گهڻائي, AstVar* گهڻائي_Varp, Ast_ڳنڍ* stmtp) {
        File_سٽ* ثابت fl = stmtp->fileline();
        إذا (گهڻائي.استعمال_ڪري_گهڻائي) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_ڳنڍ_Module* ثابت modp = VN_AS(گهڻائي_Varp->user2p(), ڳنڍ_Module);
            AstCMethodHard* ثابت atp
                = جديد AstCMethodHard{fl, جديد AstVarRef{fl, modp, گهڻائي_Varp, VAccess::READ},
                                     VCMethod::ARRAY_AT, جديد AstConst{fl, گهڻائي.اشاريو}};
            atp->dtype_سيٽ_UInt32();
            أرجع جديد AstIf{fl, atp, stmtp};
        }
        أرجع stmtp;
    }
    AstVarRef* enum_قدر_Tab_Refp(AstEnumDType* ثابت nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), VarRef)->clone_وڻ(خطأ);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_ڳنڍ_Array_DType* ثابت vardtypep = جديد AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            جديد AstRange{nodep->fileline(), static_cast<صحيح>(nodep->شي_ڳڻپ()) - 1, 0}};
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(vardtypep);
        AstConsArray* ثابت consp = جديد AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t اشاريو = 0;
        لكل (Ast_Enum_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شي)) {
            AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(اشاريو++, vconstp->clone_وڻ(خطأ));
        }
        // Share identical tables via the constant pool
        AstVarRef* ثابت refp = V3ConstPool::ڳوليو(consp);
        VL_DO_DANGLING(consp->delete_وڻ(), consp);  // V3ConstPool::find clones it
        ڌڪو_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        أرجع refp->clone_وڻ(خطأ);
    }

    AstCDType* ڳوليو_Vl_Rand_CDType(File_سٽ* ثابت fl, uint64_t شيون) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ثابت std::string type = AstCDType::type_منزل_Hold(V3_عدد::log2bQuad(شيون) + 1);
        ثابت std::string name = "VlRandC<" + type + ", " + cvt_منزل_Str(شيون) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ثابت خودڪار pair = m_randcDtypes.emplace(name, خالي_اشارو);
        إذا (pair.ٻيو) {
            AstCDType* ثابت newp = جديد AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->جوڙيو_Typesp(newp);
            pair.پهريون->ٻيو = newp;
        }
        أرجع pair.پهريون->ٻيو;
    }

    AstVar* newRandcVarsp(AstVar* ثابت varp, AstClass* ثابت classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        إذا (!varp->isRandC()) أرجع خالي_اشارو;

        uint64_t شيون = 0;
        إذا (AstEnumDType* ثابت enumDtp = VN_CAST(varp->dtypep()->skip_Ref_منزل_Enump(), EnumDType)) {
            شيون = static_cast<uint64_t>(enumDtp->شي_ڳڻپ());
        } وإلا إذا (AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp()) {
            إذا (basicp->ويڪر() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                إذا (!V3Error::error_ڳڻپ()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->ويڪر() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                أرجع خالي_اشارو;
            }
            شيون = 1ULL << basicp->ويڪر();
        } وإلا إذا (AstStructDType* ثابت dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            أرجع خالي_اشارو;
        } وإلا {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_ڳڻپ(), varp, "Unexpected randc variable dtype");
            أرجع خالي_اشارو;
        }
        AstCDType* ثابت newdtp = ڳوليو_Vl_Rand_CDType(varp->fileline(), شيون);
        AstVar* ثابت newp
            = جديد AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->is_اندروني(سچ);
        classp->جوڙيو_Stmtsp(newp);
        UINFO(9, "created " << varp);
        أرجع newp;
    }
    Ast_ڳنڍ_Stmt* createArrayForeachLoop(File_سٽ* ثابت fl, Ast_ڳنڍ_DType* ثابت dtypep,
                                        Ast_ڳنڍ_اظهار* exprp, AstVar* ثابت outputVarp) {
        Ast_ڳنڍ_DType* عارضي_DTypep = dtypep;
        AstVar* randLoopIndxp = خالي_اشارو;
        خودڪار create_Loop_اشاريو = [&](Ast_ڳنڍ_DType* عارضي_DTypep) {
            إذا (VN_IS(عارضي_DTypep, AssocArrayDType)) {
                أرجع جديد AstVar{fl, VVarType::VAR, m_rand_Array_Unique_نالا.get(""),
                                  VN_AS(عارضي_DTypep, AssocArrayDType)->ڪنجي_DTypep()};
            }
            أرجع جديد AstVar{fl, VVarType::VAR, m_rand_Array_Unique_نالا.get(""),
                              dtypep->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32)};
        };
        Ast_ڳنڍ_اظهار* عارضي_Elementp = خالي_اشارو;
        طالما (عارضي_DTypep->isNonPackedArray()) {
            AstVar* ثابت newRandLoopIndxp = create_Loop_اشاريو(عارضي_DTypep);
            randLoopIndxp = Ast_ڳنڍ::جوڙيو_ايندڙ(randLoopIndxp, newRandLoopIndxp);
            Ast_ڳنڍ_اظهار* ثابت عارضي_Exprp = عارضي_Elementp ? عارضي_Elementp : exprp;
            AstVarRef* ثابت عارضي_Refp = جديد AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            إذا (VN_IS(عارضي_DTypep, DynArrayDType)) {
                عارضي_Elementp
                    = جديد AstCMethodHard{fl, عارضي_Exprp, VCMethod::ARRAY_AT_WRITE, عارضي_Refp};
            } وإلا إذا (VN_IS(عارضي_DTypep, UnpackArrayDType)) {
                Ast_ڳنڍ_Array_DType* ثابت aryDTypep = VN_CAST(عارضي_DTypep, ڳنڍ_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                عارضي_Elementp = جديد AstArraySel{
                    fl, عارضي_Exprp,
                    جديد AstSel{
                        fl,
                        جديد AstSub{fl, عارضي_Refp,
                                   جديد AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        جديد AstConst{fl, 0}, V3_عدد::log2b(aryDTypep->hi()) + 1}};
            } وإلا إذا (VN_IS(عارضي_DTypep, AssocArrayDType)) {
                عارضي_Elementp = جديد AstAssocSel{fl, عارضي_Exprp, عارضي_Refp};
            } وإلا إذا (VN_IS(عارضي_DTypep, قطار_DType)) {
                عارضي_Elementp
                    = جديد AstCMethodHard{fl, عارضي_Exprp, VCMethod::DYN_AT_WRITE_APPEND, عارضي_Refp};
            }
            عارضي_Elementp->dtypep(عارضي_DTypep->subDTypep());
            عارضي_DTypep = عارضي_DTypep->virtRefDTypep();
        }

        Ast_Foreach_سرو* ثابت headerp
            = جديد Ast_Foreach_سرو{fl, exprp->clone_وڻ(خطأ), randLoopIndxp};
        Ast_ڳنڍ_Stmt* ثابت randStmtsp = newRandStmtsp(fl, عارضي_Elementp, خالي_اشارو, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        إذا (!عارضي_Elementp->backp()) VL_DO_DANGLING(ڌڪو_Deletep(عارضي_Elementp), عارضي_Elementp);
        أرجع جديد AstForeach{fl, headerp, randStmtsp};
    }
    Ast_ڳنڍ_Stmt* newRandStmtsp(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, AstVar* randcVarp,
                               AstVar* ثابت outputVarp, صحيح وٿي = 0,
                               AstMemberDType* memberp = خالي_اشارو) {
        Ast_ڳنڍ_DType* ثابت memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        إذا (ثابت خودڪار* ثابت structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_ڳنڍ_Stmt* stmtsp = خالي_اشارو;
            إذا (structDtp->packed()) وٿي += memberp ? memberp->lsb() : 0;
            لكل (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_ڳنڍ_Stmt* randp = خالي_اشارو;
                إذا (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_وڻ(خطأ) : exprp, خالي_اشارو,
                                          outputVarp, وٿي, smemberp);
                } وإلا إذا (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = جديد AstStructSel{fl, exprp->clone_وڻ(خطأ), smemberp->name()};
                    structSelp->dtypep(smemberp->ٻار_DTypep());
                    إذا (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, خالي_اشارو, outputVarp);
                }
                stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ_Null(stmtsp, randp);
            }
            أرجع stmtsp;
        } وإلا إذا (ثابت خودڪار* ثابت unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            إذا (!unionDtp->packed()) أرجع خالي_اشارو;
            AstMemberDType* ثابت پهريون_Memberp = unionDtp->membersp();
            أرجع newRandStmtsp(fl, exprp, خالي_اشارو, outputVarp, وٿي, پهريون_Memberp);
        } وإلا إذا (ثابت AstClassRefDType* ثابت classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ثابت memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            Ast_طريقو_Call* ثابت callp = جديد Ast_طريقو_Call{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{
                fl, جديد AstVarRef{fl, outputVarp, VAccess::WRITE},
                جديد AstAnd{fl, جديد AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            أرجع جديد AstIf{
                fl, جديد AstNeq{fl, exprp->clone_وڻ(خطأ), جديد AstConst{fl, AstConst::Null{}}},
                assignp};
        } وإلا إذا (AstDynArrayDType* ثابت dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            أرجع createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } وإلا إذا (Ast_قطار_DType* ثابت قطار_Dtp = VN_CAST(memberDtp, قطار_DType)) {
            أرجع createArrayForeachLoop(fl, قطار_Dtp, exprp, outputVarp);
        } وإلا إذا (AstUnpackArrayDType* ثابت unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            أرجع createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } وإلا إذا (AstAssocArrayDType* ثابت assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            أرجع createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } وإلا {
            Ast_ڳنڍ_اظهار* valp;
            إذا (AstEnumDType* ثابت enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ثابت tabRefp = enum_قدر_Tab_Refp(enumDtp);
                Ast_ڳنڍ_اظهار* ثابت randp
                    = new_Rand_قدر(fl, randcVarp, exprp->ڳوليو_Basic_DType(VBasicDTypeKwd::UINT32));
                Ast_ڳنڍ_اظهار* ثابت moddivp = جديد AstModDiv{
                    fl, randp, جديد AstConst{fl, static_cast<uint32_t>(enumDtp->شي_ڳڻپ())}};
                moddivp->dtypep(enumDtp);
                valp = جديد AstArraySel{fl, tabRefp, moddivp};
            } وإلا {
                valp
                    = new_Rand_قدر(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            Ast_سونپيو* assignp
                = جديد Ast_سونپيو{fl,
                                جديد AstSel{fl, exprp, وٿي + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->ويڪر() : exprp->ويڪر()},
                                valp};
            AstVar* varp = خالي_اشارو;
            exprp->exists([&](ثابت AstVarRef* varrefp) {
                إذا (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                أرجع varp != خالي_اشارو;
            });
            أرجع wrap_If_Rand_گهڻائي(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_ڳنڍ_اظهار* new_Rand_قدر(File_سٽ* ثابت fl, AstVar* ثابت randcVarp,
                              Ast_ڳنڍ_DType* ثابت dtypep) {
        إذا (randcVarp) {
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->جوڙيو(جديد AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->جوڙيو(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            أرجع cexprp;
        }

        أرجع جديد AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* ڳوليو_Pre_Post_Task(AstClass* classp, ثابت string& name) {
        لكل (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : خالي_اشارو) {
            إذا (AstTask* ثابت taskp = VN_CAST(m_memberMap.ڳوليو_Member(cp, name), Task)) {
                أرجع taskp;
            }
        }
        أرجع خالي_اشارو;
    }
    فراغ جوڙيو_Pre_Post_Call(AstClass* ثابت classp, Ast_ڳنڍ_FTask* ثابت funcp, ثابت string& name) {
        إذا (AstTask* ثابت userFuncp = ڳوليو_Pre_Post_Task(classp, name)) {
            AstTaskRef* ثابت callp = جديد AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->جوڙيو_Stmtsp(callp->makeStmt());
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
        إذا (AstTask* ثابت existingp = VN_CAST(m_memberMap.ڳوليو_Member(classp, name), Task)) {
            أرجع existingp;
        }
        AstTask* ثابت taskp = جديد AstTask{classp->fileline(), name, خالي_اشارو};
        taskp->class_طريقو(سچ);
        taskp->isVirtual(classp->isExtended());
        classp->جوڙيو_Membersp(taskp);
        m_memberMap.وجهو(classp, taskp);
        جوڙيو_Pre_Post_Call(classp, taskp, which);
        أرجع taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<منطقي, منطقي> buildPrePostVirtualWrappers(AstClass* ثابت classp) {
        ثابت خودڪار cachedIt = m_prePostWrap.ڳوليو(classp);
        إذا (cachedIt != m_prePostWrap.پڇاڙي()) أرجع cachedIt->ٻيو;
        std::ويڪٽر<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            إذا (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.ڌڪو_پٺ(subp);
        });
        منطقي hasPre = خطأ;
        منطقي hasPost = خطأ;
        لكل (AstClass* ثابت cp : hierp) {
            إذا (ڳوليو_Pre_Post_Task(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = سچ;
            }
            إذا (ڳوليو_Pre_Post_Task(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = سچ;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        إذا (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        إذا (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ثابت std::pair<منطقي, منطقي> نتيجو{hasPre, hasPost};
        m_prePostWrap.emplace(classp, نتيجو);
        أرجع نتيجو;
    }
    فراغ جوڙيو_Virtual_Pre_Post_Call(AstFunc* ثابت randomizeFuncp, AstClass* ثابت classp,
                               ثابت string& which) {
        File_سٽ* ثابت fl = classp->fileline();
        AstTask* ثابت wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ثابت refDTypep = جديد AstClassRefDType{fl, classp, خالي_اشارو};
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(refDTypep);
        Ast_طريقو_Call* ثابت callp
            = جديد Ast_طريقو_Call{fl, جديد AstThisRef{fl, refDTypep}, wrapperp->name(), خالي_اشارو};
        callp->taskp(wrapperp);
        callp->dtype_سيٽ_Void();
        randomizeFuncp->جوڙيو_Stmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    منطقي classHasRandClassMembers(AstClass* classp) {
        أرجع classp->existsMember([](ثابت AstClass*, ثابت AstVar* varp) {
            إذا (!varp->rand().isRandomizable()) أرجع خطأ;
            ثابت Ast_ڳنڍ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            أرجع VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    منطقي classOwnsGlobalConstraint(ثابت AstClass* classp) ثابت {
        أرجع classp->existsMember([](ثابت AstClass*, ثابت AstConstraint* constrp) {
            منطقي owns = خطأ;
            constrp->foreach([&](ثابت AstMemberSel* memberSelp) {
                ثابت Ast_ڳنڍ* ثابت rootp
                    = const_cast<AstMemberSel*>(memberSelp)->بنياد_Fromp(سچ);
                إذا (ثابت AstVarRef* ثابت refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ثابت Ast_ڳنڍ_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    طالما (ثابت Ast_ڳنڍ_DType* ثابت subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    إذا (VN_IS(dtypep, ClassRefDType)) owns = سچ;
                }
            });
            أرجع owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ثابت string& suffix) {
        ثابت string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.ڳوليو_Member(classp, name), Task);
        إذا (taskp) أرجع taskp;
        taskp = جديد AstTask{classp->fileline(), name, خالي_اشارو};
        taskp->class_طريقو(سچ);
        classp->جوڙيو_Membersp(taskp);
        m_memberMap.وجهو(classp, taskp);
        أرجع taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    فراغ populateNestedCallbackTask(AstTask* ثابت callbackTaskp, AstClass* ثابت classp,
                                    ثابت string& cbName) {
        File_سٽ* ثابت fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            إذا (!memberVarp->rand().isRandomizable()) أرجع;
            ثابت Ast_ڳنڍ_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
            ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
            إذا (!classRefp) أرجع;
            AstClass* ثابت memberClassp = classRefp->classp();
            إذا (memberClassp == classp) أرجع;  // Avoid self-reference

            // Force-visit member class if not yet processed
            إذا (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_ڳنڍ* stmtsp = خالي_اشارو;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            إذا (AstTask* ثابت userFuncp = ڳوليو_Pre_Post_Task(memberClassp, cbName)) {
                Ast_طريقو_Call* ثابت callp = جديد Ast_طريقو_Call{
                    fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtype_سيٽ_Void();
                stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            إذا (classHasRandClassMembers(memberClassp)) {
                ثابت string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ثابت nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                Ast_طريقو_Call* ثابت recurseCallp = جديد Ast_طريقو_Call{
                    fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtype_سيٽ_Void();
                stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(stmtsp, recurseCallp->makeStmt());
            }

            إذا (!stmtsp) أرجع;

            // Wrap in null check
            AstIf* ثابت nullCheckp = جديد AstIf{
                fl,
                جديد AstNeq{fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           جديد AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->جوڙيو_Stmtsp(wrap_If_Rand_گهڻائي(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* new_تياري_Constraint_Task(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp = جديد AstTask{nodep->fileline(), name + "_setup_constraint", خالي_اشارو};
        taskp->class_طريقو(سچ);
        nodep->جوڙيو_Membersp(taskp);
        أرجع taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp
            = جديد AstTask{nodep->fileline(), name + "_resize_constrained_array", خالي_اشارو};
        taskp->class_طريقو(سچ);
        nodep->جوڙيو_Membersp(taskp);
        أرجع taskp;
    }
    Ast_ڳنڍ_Stmt* implementConstraintsClear(File_سٽ* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جديد AstCMethodHard{
            fileline,
            جديد AstVarRef{fileline, VN_AS(genp->user2p(), ڳنڍ_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtype_سيٽ_Void();
        أرجع clearp->makeStmt();
    }
    Ast_ڳنڍ_Stmt* implementConstraintsClearAll(File_سٽ* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جديد AstCMethodHard{
            fileline,
            جديد AstVarRef{fileline, VN_AS(genp->user2p(), ڳنڍ_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtype_سيٽ_Void();
        أرجع clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_ڳنڍ_اظهار* ثابت exprp) {
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            أرجع memberSelp->varp();
        } وإلا إذا (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            أرجع varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        أرجع خالي_اشارو;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_ڳنڍ_اظهار* make_گهڻائي_Var_Ref(Ast_ڳنڍ_اظهار* ثابت exprp, AstVar* ثابت گهڻائي_Varp,
                                ثابت VAccess access) {
        إذا (گهڻائي_Varp->lifetime().isStatic()) {
            أرجع جديد AstVarRef{exprp->fileline(), VN_AS(گهڻائي_Varp->user2p(), ڳنڍ_Module),
                                 گهڻائي_Varp, access};
        }
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ثابت newMemberSelp = جديد AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_وڻ(خطأ), گهڻائي_Varp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            أرجع newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        أرجع جديد AstVarRef{exprp->fileline(), VN_AS(گهڻائي_Varp->user2p(), Class), گهڻائي_Varp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* create_Or_Get_سائيز_Var(AstClass* ثابت classp, AstVar* ثابت arrVarp, File_سٽ* ثابت fl,
                               Ast_ڳنڍ_DType* ثابت signed32DTypep, منطقي& wasCreated) {
        AstVar* سائيز_Varp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = خطأ;
        إذا (!سائيز_Varp) {
            سائيز_Varp = جديد AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->جوڙيو_Membersp(سائيز_Varp);
            m_memberMap.وجهو(classp, سائيز_Varp);
            سائيز_Varp->user2p(classp);
            arrVarp->user4p(سائيز_Varp);
            wasCreated = سچ;
        }
        أرجع سائيز_Varp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_اظهار* create_سائيز_Gte_Zero_Constraint(File_سٽ* ثابت fl, AstVar* ثابت سائيز_Varp,
                                                   Ast_ڳنڍ_اظهار* derefFromp = خالي_اشارو) {
        Ast_ڳنڍ_اظهار* سائيز_ڳنڍ_Exprp = خالي_اشارو;
        إذا (derefFromp) {
            سائيز_ڳنڍ_Exprp = buildMemberSelExprp(derefFromp, سائيز_Varp);
        } وإلا {
            سائيز_ڳنڍ_Exprp = جديد AstVarRef{fl, سائيز_Varp, VAccess::READ};
        }
        سائيز_ڳنڍ_Exprp->user1(سچ);
        AstGteS* ثابت سائيز_Gtep = جديد AstGteS{fl, سائيز_ڳنڍ_Exprp, جديد AstConst{fl, 0}};
        سائيز_Gtep->user1(سچ);
        أرجع جديد Ast_Constraint_اظهار{fl, سائيز_Gtep};
    }
    Ast_ڳنڍ_اظهار* getFromp(Ast_ڳنڍ_اظهار* ثابت exprp) {
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            أرجع memberSelp->fromp();
        } وإلا إذا (Ast_طريقو_Call* ثابت طريقو_Callp = VN_CAST(exprp, طريقو_Call)) {
            أرجع طريقو_Callp->fromp();
        }
        أرجع خالي_اشارو;
    }
    AstVar* make_Tmp_Rand_گهڻائي_Var(Ast_ڳنڍ_اظهار* siblingExprp, AstVar* rand_گهڻائي_Varp,
                               Ast_ڳنڍ*& storeStmtspr, Ast_ڳنڍ_Stmt*& restoreStmtspr) {
        File_سٽ* ثابت fl = rand_گهڻائي_Varp->fileline();
        AstVar* ثابت rand_گهڻائي_Tmp_Varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, m_گهڻائي_Unique_نالا.get(rand_گهڻائي_Varp), rand_گهڻائي_Varp->dtypep()};
        rand_گهڻائي_Tmp_Varp->funcLocal(m_ftaskp);
        rand_گهڻائي_Tmp_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_ڳنڍ::جوڙيو_ايندڙ(
            storeStmtspr,
            جديد Ast_سونپيو{fl, جديد AstVarRef{fl, rand_گهڻائي_Tmp_Varp, VAccess::WRITE},
                          make_گهڻائي_Var_Ref(siblingExprp, rand_گهڻائي_Varp, VAccess::READ)});
        storeStmtspr = Ast_ڳنڍ::جوڙيو_ايندڙ(
            storeStmtspr,
            make_گهڻائي_سيٽ_Loop(fl, make_گهڻائي_Var_Ref(siblingExprp, rand_گهڻائي_Varp, VAccess::WRITE),
                            جديد AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_ڳنڍ::جوڙيو_ايندڙ(
            restoreStmtspr,
            جديد Ast_سونپيو{fl, make_گهڻائي_Var_Ref(siblingExprp, rand_گهڻائي_Varp, VAccess::WRITE),
                          جديد AstVarRef{fl, rand_گهڻائي_Tmp_Varp, VAccess::READ}});
        أرجع rand_گهڻائي_Tmp_Varp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_ڳنڍ_اظهار* ٽڪرو_منزل_Common_اڳياڙي(Ast_ڳنڍ_اظهار* thisp, Ast_ڳنڍ_اظهار* otherp) {
        ساكن std::ويڪٽر<Ast_ڳنڍ_اظهار*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        طالما (thisp) {
            thisHier.ڌڪو_پٺ(thisp);
            thisp = getFromp(thisp);
        }
        طالما (otherp) {
            otherHier.ڌڪو_پٺ(otherp);
            otherp = getFromp(otherp);
        }
        Ast_ڳنڍ_اظهار* commonp = خالي_اشارو;
        لكل (خودڪار thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            إذا ((*thisIt)->type() != (*otherIt)->type()) اكسر;
            إذا (AstMemberSel* ثابت memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ثابت otherMemberSelp = VN_AS(*otherIt, MemberSel);
                إذا (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    استمر;
                }
            } وإلا إذا (Ast_طريقو_Call* ثابت this_طريقو_Callp = VN_CAST(*thisIt, طريقو_Call)) {
                Ast_طريقو_Call* ثابت other_طريقو_Callp = VN_AS(*otherIt, طريقو_Call);
                إذا (this_طريقو_Callp->taskp() == other_طريقو_Callp->taskp()) {
                    commonp = this_طريقو_Callp;
                    استمر;
                }
            } وإلا إذا (AstVarRef* ثابت پهريون_Var_Refp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ثابت ٻيو_Var_Refp = VN_AS(*otherIt, VarRef);
                إذا (پهريون_Var_Refp->varp() == ٻيو_Var_Refp->varp()) {
                    commonp = پهريون_Var_Refp;
                    استمر;
                }
            }
            اكسر;
        }
        أرجع commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_ڳنڍ_اظهار* buildMemberSelExprp(Ast_ڳنڍ_اظهار* exprp, AstVar* سڀ_کان_اونهو_Varp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_ڳنڍ_اظهار* resExprp = خالي_اشارو;
        إذا (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), خالي_اشارو);
            AstVar* varp = memberSelp->varp();
            إذا (سڀ_کان_اونهو_Varp) varp = سڀ_کان_اونهو_Varp;
            resExprp = جديد AstMemberSel{exprp->fileline(), resExprp, varp};
        } وإلا إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            إذا (سڀ_کان_اونهو_Varp) varp = سڀ_کان_اونهو_Varp;
            resExprp = جديد AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), ڳنڍ_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(سچ);
        أرجع resExprp;
    }

    فراغ جوڙيو_Update_Rand_Vars_Body(AstClass* ثابت nodep, AstFunc* ثابت randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ثابت updatep
            = جديد AstFunc{nodep->fileline(), "__VupdateRandVars", خالي_اشارو, خالي_اشارو};
        updatep->class_طريقو(سچ);
        updatep->isVirtual(سچ);
        updatep->keepAlive(سچ);
        updatep->needsSyms(خطأ);
        nodep->جوڙيو_Membersp(updatep);
        nodep->hasRandVarsUpdate(سچ);

        لكل (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : خالي_اشارو) {
            Ast_ڳنڍ_FTask* ثابت newp = VN_AS(m_memberMap.ڳوليو_Member(classp, "new"), ڳنڍ_FTask);
            لكل (Ast_ڳنڍ* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_اظهار* ثابت stmtExprp = VN_CAST(stmtp, Stmt_اظهار);
                إذا (!stmtExprp) استمر;
                AstCMethodHard* ثابت methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                إذا (!methodp || methodp->طريقو() != VCMethod::RANDOMIZER_WRITE_VAR) استمر;

                Ast_ڳنڍ_Var_Ref* ثابت varRefp = VN_AS(methodp->pinsp(), ڳنڍ_Var_Ref);
                إذا (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    استمر;
                }
                AstCMethodHard* ثابت updateMethodp = جديد AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_وڻ_خالص(خطأ),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_وڻ_خالص(خطأ)};
                updateMethodp->dtype_سيٽ_Void();
                Ast_ڳنڍ_اظهار* ثابت namep = VN_AS(varRefp->nextp()->nextp(), ڳنڍ_اظهار);
                updateMethodp->جوڙيو_Pinsp(namep->clone_وڻ_خالص(خطأ));
                updateMethodp->طريقو(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->جوڙيو_Stmtsp(updateMethodp->makeStmt());
            }
        }
    }

    فراغ جوڙيو_Basic_Randomize_Body(AstFunc* ثابت basicRandomizep, AstClass* ثابت nodep,
                               AstVar* rand_گهڻائي_Varp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_سٽ* ثابت fl = nodep->fileline();
        AstVar* ثابت basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ثابت basicFvarRefp = جديد AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ثابت beginBasicValp = جديد AstConst{fl, AstConst::Widthed_قدر{}, 32, 1};
        basicRandomizep->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, basicFvarRefp, beginBasicValp});
        Ast_ڳنڍ_FTask* ثابت newp = VN_AS(m_memberMap.ڳوليو_Member(nodep, "new"), ڳنڍ_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            إذا (!memberVarp->rand().isRandomizable()) أرجع;
            ثابت Randomize_گهڻائي rand_گهڻائي = {.asUQuad = memberVarp->user1()};
            إذا (rand_گهڻائي.استعمال_ڪري_گهڻائي
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ثابت setp = جديد AstCMethodHard{
                    nodep->fileline(),
                    جديد AstVarRef{fl, VN_AS(rand_گهڻائي_Varp->user2p(), ڳنڍ_Module), rand_گهڻائي_Varp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, جديد AstConst{nodep->fileline(), rand_گهڻائي.اشاريو}};
                setp->dtype_سيٽ_UInt32();
                newp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, setp, جديد AstConst{fl, 0}});
            }
            إذا (memberVarp->user3()) أرجع;  // Handled in constraints
            ثابت Ast_ڳنڍ_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
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
                Ast_طريقو_Call* ثابت callp
                    = memberVarp->globalConstrained()
                          ? جديد Ast_طريقو_Call{fl,
                                              جديد AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : جديد Ast_طريقو_Call{
                                fl, جديد AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ثابت basicFvarRefReadp = basicFvarRefp->clone_وڻ(خطأ);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ثابت سونپيو_If_Not_Nullp = جديد AstIf{
                    fl,
                    جديد AstNeq{fl, جديد AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               جديد AstConst{fl, AstConst::Null{}}},
                    جديد Ast_سونپيو{fl, basicFvarRefp->clone_وڻ(خطأ),
                                  جديد AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->جوڙيو_Stmtsp(wrap_If_Rand_گهڻائي(nodep, memberVarp, سونپيو_If_Not_Nullp));
            } وإلا {
                AstVar* ثابت randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ثابت refp
                    = جديد AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_ڳنڍ_Stmt* ثابت stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                إذا (!refp->backp()) VL_DO_DANGLING(refp->delete_وڻ(), refp);
                basicRandomizep->جوڙيو_Stmtsp(
                    جديد AstBegin{fl, m_unique_نالا.get("__VrandBlock"), stmtp, خطأ});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_ڳنڍ_اظهار* make_گهڻائي_سونپيو_Lhs(File_سٽ* ثابت fl, AstClass* ثابت classp,
                                   Ast_ڳنڍ_اظهار* ثابت fromp, AstVar* ثابت گهڻائي_Varp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        إذا (گهڻائي_Varp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            إذا (fromp) VL_DO_DANGLING(fromp->unlink_Fr_پٺ()->delete_وڻ(), fromp);
            أرجع جديد AstVarRef{fl, VN_AS(گهڻائي_Varp->user2p(), ڳنڍ_Module), گهڻائي_Varp,
                                 VAccess::WRITE};
        } وإلا إذا (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            أرجع جديد AstVarRef{fl, VN_AS(گهڻائي_Varp->user2p(), ڳنڍ_Module), گهڻائي_Varp,
                                 VAccess::WRITE};
        } وإلا {
            AstMemberSel* ثابت memberselp = جديد AstMemberSel{fl, fromp->unlink_Fr_پٺ(), گهڻائي_Varp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            أرجع memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    فراغ replace_With_گهڻائي_سونپيو(Ast_ڳنڍ_FTask_Ref* ثابت ftaskRefp, Ast_ڳنڍ* ثابت receiverp,
                               Ast_ڳنڍ_اظهار* ثابت lhsp) {
        replace_With_گهڻائي_سونپيو_And_Append(ftaskRefp, receiverp, lhsp, خالي_اشارو);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    فراغ replace_With_گهڻائي_سونپيو_And_Append(Ast_ڳنڍ_FTask_Ref* ثابت ftaskRefp, Ast_ڳنڍ* ثابت receiverp,
                                        Ast_ڳنڍ_اظهار* ثابت lhsp, Ast_ڳنڍ* ثابت appendStmtp) {
        File_سٽ* ثابت fl = ftaskRefp->fileline();
        إذا (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_اظهار), ftaskRefp, "Should be a statement");
            Ast_ڳنڍ_اظهار* ثابت rhsp = ftaskRefp->argsp()->exprp()->unlink_Fr_پٺ();
            Ast_ڳنڍ* newStmtp = خالي_اشارو;
            إذا (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                //گهڻائي
                ثابت Randomize_گهڻائي rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.استعمال_ڪري_گهڻائي, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ثابت setp = جديد AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                جديد AstConst{fl, rmode.اشاريو}};
                setp->dtype_سيٽ_UInt32();
                newStmtp = جديد Ast_سونپيو{fl, setp, rhsp};
            } وإلا {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = make_گهڻائي_سيٽ_Loop(fl, lhsp, rhsp, m_ftaskp);
            }
            إذا (appendStmtp) newStmtp->جوڙيو_ايندڙ(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(ڌڪو_Deletep(m_stmtp), m_stmtp = خالي_اشارو);
        } وإلا {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ثابت Randomize_گهڻائي rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.استعمال_ڪري_گهڻائي, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ثابت setp = جديد AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            جديد AstConst{fl, rmode.اشاريو}};
            setp->dtype_سيٽ_UInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(ڌڪو_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    فراغ wrapRandomizeCallWithNullGuard(Ast_ڳنڍ_FTask_Ref* nodep) {
        Ast_طريقو_Call* ثابت callp = VN_CAST(nodep, طريقو_Call);
        إذا (!callp) أرجع;
        إذا (callp->user4_سيٽ_Once()) أرجع;
        File_سٽ* ثابت fl = callp->fileline();
        Ast_ڳنڍ_اظهار* ثابت checkp
            = جديد AstNeq{fl, callp->fromp()->clone_وڻ(خطأ), جديد AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlink_Fr_پٺ(&relinker);
        relinker.relink(جديد AstCond{fl, checkp, callp, جديد AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    فراغ handle_Randomize_دليلون(Ast_ڳنڍ_FTask_Ref* ثابت nodep) {
        إذا (!nodep->argsp()) أرجع;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        منطقي hasNullArg = خطأ;
        لكل (AstArg *argp = nodep->argsp(), *nextp = خالي_اشارو; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->انگ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = سچ;
                VL_DO_DANGLING(argp->unlink_Fr_پٺ()->delete_وڻ(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        لكل (AstArg *argp = nodep->argsp(), *nextp = خالي_اشارو; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            لكل (AstArg *otherArgp = nextp, *otherNextp = خالي_اشارو; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                إذا (Ast_ڳنڍ_اظهار* ثابت prefixp
                    = ٽڪرو_منزل_Common_اڳياڙي(argp->exprp(), otherArgp->exprp())) {
                    إذا (prefixp == argp->exprp()) {
                        إذا (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlink_Fr_پٺ()->delete_وڻ(), otherArgp);
                        استمر;
                    }
                }
                إذا (Ast_ڳنڍ_اظهار* ثابت prefixp
                    = ٽڪرو_منزل_Common_اڳياڙي(otherArgp->exprp(), argp->exprp())) {
                    إذا (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlink_Fr_پٺ()->delete_وڻ(), argp);
                        اكسر;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::سيٽ<AstVar*> saved_Rand_گهڻائي_Varps;
        AstVar* tmpVarps = خالي_اشارو;
        Ast_ڳنڍ* storeStmtsp = خالي_اشارو;
        Ast_ڳنڍ* سيٽ_Stmtsp = خالي_اشارو;
        Ast_ڳنڍ_Stmt* restoreStmtsp = خالي_اشارو;
        لكل (AstArg *argp = nodep->argsp(), *nextp = خالي_اشارو; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ڳنڍ_اظهار* exprp = argp->exprp();
            Ast_ڳنڍ_اظهار* ثابت commonPrefixp = ٽڪرو_منزل_Common_اڳياڙي(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_سٽ* ثابت fl = argp->fileline();
            طالما (exprp) {
                إذا (commonPrefixp == exprp) اكسر;
                AstVar* ثابت randVarp = getVarFromRef(exprp);
                AstClass* ثابت classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ثابت rand_گهڻائي_Varp = randVarp->lifetime().isStatic()
                                                 ? get_Static_Rand_گهڻائي_Var(classp)
                                                 : get_Rand_گهڻائي_Var_From_Class(classp);
                UASSERT_OBJ(rand_گهڻائي_Varp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                إذا (saved_Rand_گهڻائي_Varps.ڳوليو(rand_گهڻائي_Varp) == saved_Rand_گهڻائي_Varps.پڇاڙي()) {
                    AstVar* ثابت rand_گهڻائي_Tmp_Varp
                        = make_Tmp_Rand_گهڻائي_Var(exprp, rand_گهڻائي_Varp, storeStmtsp, restoreStmtsp);
                    saved_Rand_گهڻائي_Varps.وجهو(rand_گهڻائي_Varp);
                    tmpVarps = Ast_ڳنڍ::جوڙيو_ايندڙ(tmpVarps, rand_گهڻائي_Tmp_Varp);
                }
                ثابت Randomize_گهڻائي rand_گهڻائي = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = جديد AstCMethodHard{
                    fl, make_گهڻائي_Var_Ref(exprp, rand_گهڻائي_Varp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, جديد AstConst{fl, rand_گهڻائي.اشاريو}};
                setp->dtype_سيٽ_UInt32();
                سيٽ_Stmtsp
                    = Ast_ڳنڍ::جوڙيو_ايندڙ(سيٽ_Stmtsp, جديد Ast_سونپيو{fl, setp, جديد AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlink_Fr_پٺ()->delete_وڻ(), argp);
        }
        إذا (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* هدف_Classp = خالي_اشارو;
            Ast_طريقو_Call* ثابت طريقو_Callp = VN_CAST(nodep, طريقو_Call);
            إذا (طريقو_Callp) {
                ثابت Ast_ڳنڍ_DType* ثابت fromDTypep = طريقو_Callp->fromp()->dtypep();
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
            UASSERT_OBJ(storeStmtsp && سيٽ_Stmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlink_Fr_پٺ(&relinker);
            Ast_ڳنڍ* ثابت stmtsp = storeStmtsp;
            stmtsp->جوڙيو_ايندڙ(سيٽ_Stmtsp);
            stmtsp->جوڙيو_ايندڙ(m_stmtp);
            stmtsp->جوڙيو_ايندڙ(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            جوڙيو_عارضي_Varsp(tmpVarps);
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
        إذا (AstFunc* ثابت existingp = VN_AS(m_memberMap.ڳوليو_Member(classp, name), Func)) {
            أرجع existingp;
        }
        File_سٽ* ثابت fl = classp->fileline();
        AstFunc* ثابت funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ثابت fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        جوڙيو_Pre_Post_Call(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ثابت classGenp = getRandomGenerator(classp);
        إذا (!classGenp) {
            funcp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE},
                                           جديد AstConst{fl, AstConst::Widthed_قدر{}, 32, 1}});
        } وإلا {
            Ast_ڳنڍ_Module* ثابت genModp = VN_AS(classGenp->user2p(), ڳنڍ_Module);
            funcp->جوڙيو_Stmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ثابت تياري_All_Taskp = get_Create_Constraint_تياري_Func(classp, m_memberMap);
            funcp->جوڙيو_Stmtsp((جديد AstTaskRef{fl, تياري_All_Taskp})->makeStmt());
            AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
            solverCallp->dtype_سيٽ_ٻٽو_انگ();
            solverCallp->جوڙيو(جديد AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->جوڙيو(".next_check_only(__Vm_rng)");
            funcp->جوڙيو_Stmtsp(
                جديد Ast_سونپيو{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(سچ);
        }

        // 3. post_randomize -- only if result is non-zero
        إذا (AstTask* ثابت userPostp = ڳوليو_Pre_Post_Task(classp, "post_randomize")) {
            AstTaskRef* ثابت callp = جديد AstTaskRef{userPostp->fileline(), userPostp};
            funcp->جوڙيو_Stmtsp(
                جديد AstIf{fl, جديد AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        أرجع funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* lift_Log_If_Chain_منزل_Constraint_If(AstLogIf* logIfp) {
        File_سٽ* ثابت fl = logIfp->fileline();
        Ast_ڳنڍ_اظهار* ثابت condp = logIfp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rhsp = logIfp->rhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ* thenBodyp;
        إذا (AstLogIf* ثابت innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = lift_Log_If_Chain_منزل_Constraint_If(innerLogIfp);
            VL_DO_DANGLING(ڌڪو_Deletep(innerLogIfp), innerLogIfp);
        } وإلا {
            thenBodyp = جديد Ast_Constraint_اظهار{fl, rhsp};
        }
        أرجع جديد AstConstraintIf{fl, condp, thenBodyp, خالي_اشارو};
    }

    ساكن منطقي distBoundRefsRandVar(ثابت Ast_ڳنڍ* boundp) {
        أرجع boundp->exists(
            [](ثابت AstVarRef* vrefp) { أرجع vrefp->varp()->rand().isRandomizable(); });
    }

    ساكن منطقي dist_Bound_Refs_گهڻائي_Var(ثابت Ast_ڳنڍ* boundp) {
        أرجع boundp->exists([](ثابت AstVarRef* vrefp) {
            إذا (!vrefp->varp()->rand().isRandomizable()) أرجع خطأ;
            ثابت Randomize_گهڻائي rmode = {.asUQuad = vrefp->varp()->user1()};
            أرجع rmode.استعمال_ڪري_گهڻائي;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ساكن Ast_ڳنڍ_اظهار* newDistRangeMembership(AstDist* distp, ثابت AstInsideRange* irp) {
        File_سٽ* ثابت fl = distp->fileline();
        ثابت منطقي isSigned = distp->exprp()->isSigned();
        Ast_ڳنڍ_اظهار* ثابت dist_اظهار_Gtep = distp->exprp()->clone_وڻ_خالص(خطأ);
        Ast_ڳنڍ_اظهار* ثابت dist_اظهار_Ltep = distp->exprp()->clone_وڻ_خالص(خطأ);
        dist_اظهار_Gtep->user1(سچ);
        dist_اظهار_Ltep->user1(سچ);
        Ast_ڳنڍ_اظهار* ثابت gep
            = isSigned ? static_cast<Ast_ڳنڍ_اظهار*>(
                             جديد AstGteS{fl, dist_اظهار_Gtep, irp->lhsp()->clone_وڻ_خالص(خطأ)})
                       : static_cast<Ast_ڳنڍ_اظهار*>(
                             جديد AstGte{fl, dist_اظهار_Gtep, irp->lhsp()->clone_وڻ_خالص(خطأ)});
        Ast_ڳنڍ_اظهار* ثابت lep
            = isSigned ? static_cast<Ast_ڳنڍ_اظهار*>(
                             جديد AstLteS{fl, dist_اظهار_Ltep, irp->rhsp()->clone_وڻ_خالص(خطأ)})
                       : static_cast<Ast_ڳنڍ_اظهار*>(
                             جديد AstLte{fl, dist_اظهار_Ltep, irp->rhsp()->clone_وڻ_خالص(خطأ)});
        gep->user1(سچ);
        lep->user1(سچ);
        Ast_ڳنڍ_اظهار* ثابت andp = جديد AstLogAnd{fl, gep, lep};
        andp->user1(سچ);
        أرجع andp;
    }

    بنية Dist_ٻالٽي final {
        Ast_ڳنڍ_اظهار* rangep;  // A single value or an InsideRange
        Ast_ڳنڍ_اظهار* وزن_Exprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::ويڪٽر<Dist_ٻالٽي> collectDistBuckets(AstDist* ثابت distp) {
        File_سٽ* ثابت fl = distp->fileline();
        std::ويڪٽر<Dist_ٻالٽي> buckets;
        لكل (Ast_Dist_شي* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), Dist_شي)) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(ditemp->weightp(), Const)) {
                إذا (constp->منزل_UQuad() == 0) استمر;
            }
            Ast_ڳنڍ_اظهار* وزن_Exprp
                = جديد AstExtend{fl, ditemp->weightp()->clone_وڻ_خالص(خطأ), 64};
            // := on a range weights every element, so scale by the range size.
            إذا (!ditemp->is_سڄو()) {
                إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ثابت AstConst* ثابت lop = VN_CAST(irp->lhsp(), Const);
                    ثابت AstConst* ثابت hip = VN_CAST(irp->rhsp(), Const);
                    Ast_ڳنڍ_اظهار* rangeSizep;
                    إذا (lop && hip) {
                        ثابت uint64_t range_سائيز = hip->منزل_UQuad() - lop->منزل_UQuad() + 1;
                        rangeSizep = جديد AstConst{fl, AstConst::Unsized64{}, range_سائيز};
                    } وإلا {
                        rangeSizep = جديد Ast_جوڙيو{
                            fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                            جديد AstSub{fl,
                                       جديد AstExtend{fl, irp->rhsp()->clone_وڻ_خالص(خطأ), 64},
                                       جديد AstExtend{fl, irp->lhsp()->clone_وڻ_خالص(خطأ), 64}}};
                    }
                    وزن_Exprp = جديد AstMul{fl, وزن_Exprp, rangeSizep};
                }
            }
            buckets.ڌڪو_پٺ({ditemp->rangep(), وزن_Exprp});
        }
        أرجع buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ساكن Ast_ڳنڍ_اظهار* new_Dist_Membership_رڪن(AstDist* distp, Ast_ڳنڍ_اظهار* rangep) {
        إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            أرجع newDistRangeMembership(distp, irp);
        }
        File_سٽ* ثابت fl = distp->fileline();
        Ast_ڳنڍ_اظهار* ثابت eqExprp = distp->exprp()->clone_وڻ_خالص(خطأ);
        eqExprp->user1(سچ);
        Ast_ڳنڍ_اظهار* ثابت eqp = جديد AstEq{fl, eqExprp, rangep->clone_وڻ_خالص(خطأ)};
        eqp->user1(سچ);
        أرجع eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_اظهار* buildDistMembership(AstDist* distp,
                                           ثابت std::ويڪٽر<Dist_ٻالٽي>& buckets) {
        File_سٽ* ثابت fl = distp->fileline();
        Ast_ڳنڍ_اظهار* unionExprp = خالي_اشارو;
        لكل (ثابت خودڪار& ٻالٽي : buckets) {
            Ast_ڳنڍ_اظهار* ثابت memberp = new_Dist_Membership_رڪن(distp, ٻالٽي.rangep);
            إذا (!unionExprp) {
                unionExprp = memberp;
            } وإلا {
                unionExprp = جديد AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(سچ);
            }
        }
        أرجع جديد Ast_Constraint_اظهار{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_ڳنڍ_اظهار* newGateJoin(File_سٽ* fl, Ast_ڳنڍ_اظهار* ap, Ast_ڳنڍ_اظهار* bp, منطقي isAnd) {
        إذا (ap->same_وڻ(bp)) {
            VL_DO_DANGLING(ڌڪو_Deletep(bp), bp);
            أرجع ap;
        }
        إذا (VN_IS(bp, Const)) std::مٽا_سٽا(ap, bp);
        إذا (VN_IS(ap, Const)) {
            ثابت منطقي constWins = VN_AS(ap, Const)->isZero() == isAnd;
            إذا (!constWins) std::مٽا_سٽا(ap, bp);
            VL_DO_DANGLING(ڌڪو_Deletep(bp), bp);
            أرجع ap;
        }
        إذا (isAnd) أرجع جديد AstLogAnd{fl, ap, bp};
        أرجع جديد AstLogOr{fl, ap, bp};
    }
    Ast_ڳنڍ_اظهار* newGateOr(File_سٽ* fl, Ast_ڳنڍ_اظهار* ap, Ast_ڳنڍ_اظهار* bp) {
        أرجع newGateJoin(fl, ap, bp, خطأ);
    }
    Ast_ڳنڍ_اظهار* newGateAnd(File_سٽ* fl, Ast_ڳنڍ_اظهار* ap, Ast_ڳنڍ_اظهار* bp) {
        أرجع newGateJoin(fl, ap, bp, سچ);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_ڳنڍ_اظهار* new_گهڻائي_ٻٽو_انگ_Read(AstVar* varp, ثابت AstMemberSel* mselp, AstVar* rand_گهڻائي_Varp,
                                File_سٽ* fl) {
        Ast_ڳنڍ_اظهار* arrayp = خالي_اشارو;
        إذا (varp->lifetime().isStatic()) {
            AstClass* ثابت ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ثابت smodep = get_Static_Rand_گهڻائي_Var(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = جديد AstVarRef{fl, VN_AS(smodep->user2p(), ڳنڍ_Module), smodep, VAccess::READ};
        } وإلا إذا (mselp) {
            AstVar* ثابت memberModep = get_Rand_گهڻائي_Var_From_Class(VN_AS(varp->user2p(), ڳنڍ_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = جديد AstMemberSel{fl, mselp->fromp()->clone_وڻ_خالص(خطأ), memberModep};
        } وإلا {
            UASSERT_OBJ(rand_گهڻائي_Varp, varp, "rand_mode variable without a class mode array");
            arrayp = جديد AstVarRef{fl, VN_AS(rand_گهڻائي_Varp->user2p(), ڳنڍ_Module), rand_گهڻائي_Varp,
                                   VAccess::READ};
        }
        ثابت Randomize_گهڻائي rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ثابت atp
            = جديد AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, جديد AstConst{fl, rmode.اشاريو}};
        atp->dtype_سيٽ_UInt32();
        أرجع atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_ڳنڍ_اظهار* newVarGate(AstVar* varp, ثابت AstMemberSel* mselp, منطقي ownerLevel,
                            AstVar* rand_گهڻائي_Varp, File_سٽ* fl) {
        إذا (!varp->rand().isRandomizable()) {
            أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}, ownerLevel};
        }
        ثابت Randomize_گهڻائي rmode = {.asUQuad = varp->user1()};
        إذا (!rmode.استعمال_ڪري_گهڻائي) أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}};
        أرجع new_گهڻائي_ٻٽو_انگ_Read(varp, mselp, rand_گهڻائي_Varp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_ڳنڍ_اظهار* newAccessGate(ثابت Ast_ڳنڍ* nodep, منطقي ownerLevel, AstVar* rand_گهڻائي_Varp,
                               File_سٽ* fl) {
        إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel)) {
            أرجع newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, rand_گهڻائي_Varp, fl),
                              newAccessGate(mselp->fromp(), سچ, rand_گهڻائي_Varp, fl));
        }
        إذا (ثابت Ast_ڳنڍ_Var_Ref* ثابت refp = VN_CAST(nodep, ڳنڍ_Var_Ref)) {
            أرجع newVarGate(refp->varp(), خالي_اشارو, ownerLevel, rand_گهڻائي_Varp, fl);
        }
        إذا (ثابت Ast_ڳنڍ_Sel* ثابت selp = VN_CAST(nodep, ڳنڍ_Sel)) {
            أرجع newAccessGate(selp->fromp(), سچ, rand_گهڻائي_Varp, fl);
        }
        إذا (dist_Bound_Refs_گهڻائي_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_ڳنڍ_اظهار* newDistGate(ثابت Ast_ڳنڍ* nodep, AstVar* rand_گهڻائي_Varp, File_سٽ* fl) {
        إذا (VN_IS(nodep, ڳنڍ_Var_Ref) || VN_IS(nodep, MemberSel)) {
            أرجع newAccessGate(nodep, خطأ, rand_گهڻائي_Varp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        إذا (ثابت Ast_ڳنڍ_Sel* ثابت selp = VN_CAST(nodep, ڳنڍ_Sel)) {
            أرجع newDistGate(selp->fromp(), rand_گهڻائي_Varp, fl);
        }
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            أرجع newDistGate(selp->fromp(), rand_گهڻائي_Varp, fl);
        }
        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            أرجع newDistGate(selp->fromp(), rand_گهڻائي_Varp, fl);
        }
        إذا (ثابت AstCMethodHard* ثابت methodp = VN_CAST(nodep, CMethodHard)) {
            إذا (methodp->طريقو() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                أرجع newDistGate(methodp->fromp(), rand_گهڻائي_Varp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        إذا (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            Ast_ڳنڍ_اظهار* ثابت thenGatep = newDistGate(condp->thenp(), rand_گهڻائي_Varp, fl);
            Ast_ڳنڍ_اظهار* ثابت elseGatep = newDistGate(condp->elsep(), rand_گهڻائي_Varp, fl);
            Ast_ڳنڍ_اظهار* armGatep = thenGatep;
            إذا (thenGatep->same_وڻ(elseGatep)) {
                VL_DO_DANGLING(ڌڪو_Deletep(elseGatep), elseGatep);
            } وإلا {
                armGatep
                    = جديد AstCond{fl, condp->condp()->clone_وڻ_خالص(خطأ), thenGatep, elseGatep};
            }
            أرجع newGateOr(fl, newDistGate(condp->condp(), rand_گهڻائي_Varp, fl), armGatep);
        }
        إذا (ثابت Ast_ڳنڍ_Uniop* ثابت uopp = VN_CAST(nodep, ڳنڍ_Uniop)) {
            أرجع newDistGate(uopp->lhsp(), rand_گهڻائي_Varp, fl);
        }
        إذا (ثابت Ast_ڳنڍ_Biop* ثابت bopp = VN_CAST(nodep, ڳنڍ_Biop)) {
            أرجع newGateOr(fl, newDistGate(bopp->lhsp(), rand_گهڻائي_Varp, fl),
                             newDistGate(bopp->rhsp(), rand_گهڻائي_Varp, fl));
        }
        إذا (dist_Bound_Refs_گهڻائي_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_اظهار* buildFrozenMembership(AstDist* distp,
                                             ثابت std::ويڪٽر<Dist_ٻالٽي>& buckets) {
        File_سٽ* ثابت fl = distp->fileline();
        Ast_ڳنڍ_اظهار* unionExprp = خالي_اشارو;
        لكل (ثابت خودڪار& ٻالٽي : buckets) {
            Ast_ڳنڍ_اظهار* termp = new_Dist_Membership_رڪن(distp, ٻالٽي.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ثابت منطقي runtime_وزن
                = ٻالٽي.وزن_Exprp->exists([](ثابت Ast_ڳنڍ_Var_Ref*) { أرجع سچ; });
            إذا (runtime_وزن) {
                AstNeq* ثابت nzp = جديد AstNeq{fl, ٻالٽي.وزن_Exprp->clone_وڻ_خالص(خطأ),
                                               جديد AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(سچ);
                termp = جديد AstLogAnd{fl, termp, nzp};
                termp->user1(سچ);
            }
            إذا (unionExprp) {
                unionExprp = جديد AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(سچ);
            } وإلا {
                unionExprp = termp;
            }
        }
        أرجع جديد Ast_Constraint_اظهار{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_ڳنڍ* gateFrozenDist(Ast_ڳنڍ* chainp, AstDist* distp,
                            ثابت std::ويڪٽر<Dist_ٻالٽي>& buckets, AstVar* rand_گهڻائي_Varp) {
        File_سٽ* ثابت fl = distp->fileline();
        Ast_ڳنڍ_اظهار* gatep = newDistGate(distp->exprp(), rand_گهڻائي_Varp, fl);
        إذا (VN_IS(gatep, Const)) {
            ثابت منطقي drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(ڌڪو_Deletep(gatep), gatep);
            إذا (drawable) أرجع chainp;
            VL_DO_DANGLING(ڌڪو_Deletep(chainp), chainp);
            أرجع buildFrozenMembership(distp, buckets);
        }
        أرجع جديد AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_ڳنڍ_اظهار* new_هڪجهڙو_Range_Pick(AstDist* distp, ثابت AstInsideRange* irp) {
        File_سٽ* ثابت fl = distp->fileline();
        Ast_ڳنڍ_اظهار* ثابت dist_اظهار_Copyp = distp->exprp()->clone_وڻ_خالص(خطأ);
        dist_اظهار_Copyp->user1(سچ);
        ثابت صحيح dist_ويڪر = distp->exprp()->ويڪر();
        ثابت AstConst* ثابت lopC = VN_CAST(irp->lhsp(), Const);
        ثابت AstConst* ثابت hipC = VN_CAST(irp->rhsp(), Const);
        Ast_ڳنڍ_اظهار* rangeSzp;
        إذا (lopC && hipC) {
            ثابت uint64_t rsz = hipC->منزل_UQuad() - lopC->منزل_UQuad() + 1;
            rangeSzp = جديد AstConst{fl, AstConst::Unsized64{}, rsz};
        } وإلا {
            ثابت منطقي isSigned = irp->lhsp()->isSigned();
            Ast_ڳنڍ_اظهار* ثابت lo64p
                = isSigned ? static_cast<Ast_ڳنڍ_اظهار*>(
                                 جديد AstExtendS{fl, irp->lhsp()->clone_وڻ_خالص(خطأ), 64})
                           : static_cast<Ast_ڳنڍ_اظهار*>(
                                 جديد AstExtend{fl, irp->lhsp()->clone_وڻ_خالص(خطأ), 64});
            Ast_ڳنڍ_اظهار* ثابت hi64p
                = isSigned ? static_cast<Ast_ڳنڍ_اظهار*>(
                                 جديد AstExtendS{fl, irp->rhsp()->clone_وڻ_خالص(خطأ), 64})
                           : static_cast<Ast_ڳنڍ_اظهار*>(
                                 جديد AstExtend{fl, irp->rhsp()->clone_وڻ_خالص(خطأ), 64});
            rangeSzp = جديد Ast_جوڙيو{fl, جديد AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  جديد AstSub{fl, hi64p, lo64p}};
        }
        Ast_ڳنڍ_اظهار* ثابت rand64p = جديد AstRand{fl, خالي_اشارو, خطأ};
        rand64p->dtype_سيٽ_UInt64();
        Ast_ڳنڍ_اظهار* ثابت offsetp
            = جديد AstCCast{fl, جديد AstModDiv{fl, rand64p, rangeSzp}, dist_ويڪر};
        Ast_ڳنڍ_اظهار* ثابت valuep = جديد Ast_جوڙيو{fl, irp->lhsp()->clone_وڻ_خالص(خطأ), offsetp};
        Ast_ڳنڍ_اظهار* ثابت eqp = جديد AstEq{fl, dist_اظهار_Copyp, valuep};
        eqp->user1(سچ);
        أرجع eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_ڳنڍ* build_Weighted_ٻالٽي_Chain(AstDist* distp, ثابت std::ويڪٽر<Dist_ٻالٽي>& buckets,
                                      AstVar* ٻالٽي_Varp,
                                      ثابت std::ويڪٽر<Ast_ڳنڍ_اظهار*>& cumSums) {
        File_سٽ* ثابت fl = distp->fileline();
        Ast_ڳنڍ_اظهار* chainp = خالي_اشارو;
        لكل (صحيح i = static_cast<صحيح>(buckets.سائيز()) - 1; i >= 0; --i) {
            Ast_ڳنڍ_اظهار* constraintExprp;
            ثابت AstInsideRange* ثابت irp = VN_CAST(buckets[i].rangep, InsideRange);
            إذا (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } وإلا إذا (irp) {
                constraintExprp = new_هڪجهڙو_Range_Pick(distp, irp);
            } وإلا {
                Ast_ڳنڍ_اظهار* ثابت dist_اظهار_Copyp = distp->exprp()->clone_وڻ_خالص(خطأ);
                dist_اظهار_Copyp->user1(سچ);
                constraintExprp
                    = جديد AstEq{fl, dist_اظهار_Copyp, buckets[i].rangep->clone_وڻ_خالص(خطأ)};
                constraintExprp->user1(سچ);
            }
            إذا (!chainp) {
                chainp = constraintExprp;
            } وإلا {
                Ast_ڳنڍ_اظهار* ثابت condp
                    = جديد AstLte{fl, جديد AstVarRef{fl, ٻالٽي_Varp, VAccess::READ}, cumSums[i]};
                condp->user1(سچ);
                chainp = جديد AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(سچ);
            }
        }
        Ast_Constraint_اظهار* ثابت chainExprp = جديد Ast_Constraint_اظهار{fl, chainp};
        chainExprp->isSoft(سچ);
        أرجع chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    فراغ هيٺاهون_Dist_Constraints(AstTask* taskp, Ast_ڳنڍ* constrItemsp, AstVar* rand_گهڻائي_Varp,
                              AstConstraintForeach* foreachp = خالي_اشارو) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_ڳنڍ* foreachTailp = خالي_اشارو;
        خودڪار جوڙيو_Stmt = [&](Ast_ڳنڍ* nodep) {
            إذا (foreachp) {
                إذا (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } وإلا {
                    foreachTailp->جوڙيو_ايندڙ(nodep);
                    foreachTailp = nodep;
                }
            } وإلا {
                taskp->جوڙيو_Stmtsp(nodep);
            }
        };

        لكل (Ast_ڳنڍ *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            إذا (AstConstraintIf* ثابت cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                هيٺاهون_Dist_Constraints(taskp, cifp->thensp(), rand_گهڻائي_Varp, foreachp);
                إذا (cifp->elsesp()) {
                    هيٺاهون_Dist_Constraints(taskp, cifp->elsesp(), rand_گهڻائي_Varp, foreachp);
                }
                استمر;
            }
            إذا (AstConstraintForeach* ثابت cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                هيٺاهون_Dist_Constraints(taskp, cfep->bodyp(), rand_گهڻائي_Varp, cfep);
                استمر;
            }

            Ast_Constraint_اظهار* ثابت constrExprp = VN_CAST(itemp, Constraint_اظهار);
            إذا (!constrExprp) استمر;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            إذا (AstLogIf* ثابت چوٽي_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_ڳنڍ* chainEndp = چوٽي_Log_Ifp->rhsp();
                طالما (AstLogIf* ثابت innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                إذا (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ثابت liftedp = lift_Log_If_Chain_منزل_Constraint_If(چوٽي_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(ڌڪو_Deletep(constrExprp), constrExprp);
                    هيٺاهون_Dist_Constraints(taskp, liftedp->thensp(), rand_گهڻائي_Varp, foreachp);
                    استمر;
                }
            }

            AstDist* ثابت distp = VN_CAST(constrExprp->exprp(), Dist);
            إذا (!distp) استمر;

            File_سٽ* ثابت fl = distp->fileline();

            std::ويڪٽر<Dist_ٻالٽي> buckets = collectDistBuckets(distp);

            إذا (buckets.خالي_جايون()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_اظهار* ثابت truep
                    = جديد Ast_Constraint_اظهار{fl, جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(ڌڪو_Deletep(constrExprp), constrExprp);
                استمر;
            }

            Ast_Constraint_اظهار* ثابت membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_ڳنڍ_اظهار* ڪل_وزن_Exprp = خالي_اشارو;
            لكل (خودڪار& ٻالٽي : buckets) {
                إذا (!ڪل_وزن_Exprp) {
                    ڪل_وزن_Exprp = ٻالٽي.وزن_Exprp->clone_وڻ_خالص(خطأ);
                } وإلا {
                    ڪل_وزن_Exprp = جديد Ast_جوڙيو{fl, ڪل_وزن_Exprp,
                                                  ٻالٽي.وزن_Exprp->clone_وڻ_خالص(خطأ)};
                    ڪل_وزن_Exprp->dtype_سيٽ_UInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ثابت صحيح distId = m_dist_انگ++;
            ثابت std::string ڪل_Name = "__Vdist_total" + cvt_منزل_Str(distId);
            AstVar* ثابت ڪل_Varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, ڪل_Name, taskp->ڳوليو_UInt64_DType()};
            ڪل_Varp->noSubst(سچ);
            ڪل_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ڪل_Varp->funcLocal(سچ);
            ڪل_Varp->is_اندروني(سچ);
            جوڙيو_Stmt(ڪل_Varp);
            جوڙيو_Stmt(
                جديد Ast_سونپيو{fl, جديد AstVarRef{fl, ڪل_Varp, VAccess::WRITE}, ڪل_وزن_Exprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ثابت std::string ٻالٽي_Name = "__Vdist_bucket" + cvt_منزل_Str(distId);
            AstVar* ثابت ٻالٽي_Varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, ٻالٽي_Name, taskp->ڳوليو_UInt64_DType()};
            ٻالٽي_Varp->noSubst(سچ);
            ٻالٽي_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ٻالٽي_Varp->funcLocal(سچ);
            ٻالٽي_Varp->is_اندروني(سچ);
            جوڙيو_Stmt(ٻالٽي_Varp);

            Ast_ڳنڍ_اظهار* ثابت randp = جديد AstRand{fl, خالي_اشارو, خطأ};
            randp->dtype_سيٽ_UInt64();
            جوڙيو_Stmt(جديد Ast_سونپيو{
                fl, جديد AstVarRef{fl, ٻالٽي_Varp, VAccess::WRITE},
                جديد Ast_جوڙيو{
                    fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                    جديد AstModDiv{fl, randp, جديد AstVarRef{fl, ڪل_Varp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::ويڪٽر<Ast_ڳنڍ_اظهار*> cumSums;
            Ast_ڳنڍ_اظهار* runningSump = خالي_اشارو;
            لكل (سائيز_t i = 0; i < buckets.سائيز(); ++i) {
                إذا (!runningSump) {
                    runningSump = buckets[i].وزن_Exprp->clone_وڻ_خالص(خطأ);
                } وإلا {
                    runningSump = جديد Ast_جوڙيو{fl, runningSump,
                                             buckets[i].وزن_Exprp->clone_وڻ_خالص(خطأ)};
                    runningSump->dtype_سيٽ_UInt64();
                }
                cumSums.ڌڪو_پٺ(runningSump->clone_وڻ_خالص(سچ));
            }

            Ast_ڳنڍ* chainp = build_Weighted_ٻالٽي_Chain(distp, buckets, ٻالٽي_Varp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, rand_گهڻائي_Varp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(ڌڪو_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->جوڙيو_هتي_This_As_ايندڙ(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            لكل (خودڪار& ٻالٽي : buckets) {
                VL_DO_DANGLING(ڌڪو_Deletep(ٻالٽي.وزن_Exprp), ٻالٽي.وزن_Exprp);
            }
            VL_DO_DANGLING(ڌڪو_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            ڌڪو_Deletep(cumSums.پٺ());
        }
    }

    ساكن منطقي is_Dyn_Arr_جو_Class_Type_Recurse(ثابت Ast_ڳنڍ_DType* ثابت dtypep) {
        ثابت Ast_ڳنڍ_DType* ثابت refp = dtypep->skipRefp();
        إذا (VN_IS(refp, DynArrayDType) || VN_IS(refp, قطار_DType)) {
            أرجع is_Dyn_Arr_جو_Class_Type_Recurse(refp->subDTypep());
        } وإلا إذا (VN_IS(refp, ClassRefDType)) {
            أرجع سچ;
        }
        أرجع خطأ;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    فراغ جوڙيو_Array_Elem_Refresh(File_سٽ* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                             AstVar* ثابت genp) {
        Ast_ڳنڍ_Module* ثابت genModp = VN_AS(genp->user2p(), ڳنڍ_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(خطأ).ٻيو;
        إذا (is_Dyn_Arr_جو_Class_Type_Recurse(arrVarp->dtypep())) {
            إذا (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            أرجع;
        }
        AstCMethodHard* ثابت methodp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_سيٽ_Void();

        Ast_ڳنڍ_Module* ثابت classp = VN_AS(arrVarp->user2p(), ڳنڍ_Module);
        AstVarRef* ثابت varRefp = جديد AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->جوڙيو_Pinsp(varRefp);

        ثابت سائيز_t ويڪر = arrayElementDTypep(arrVarp->dtypep())->ويڪر();

        methodp->جوڙيو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, ويڪر});
        Ast_ڳنڍ_اظهار* ثابت varnamep
            = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + arrVarp->name() + "\"", arrVarp->ويڪر()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->جوڙيو_Pinsp(varnamep);
        methodp->جوڙيو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ثابت Randomize_گهڻائي rmode = {.asUQuad = arrVarp->user1()};
        إذا (rmode.استعمال_ڪري_گهڻائي) {
            methodp->جوڙيو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, rmode.اشاريو});
        }

        randomizep->جوڙيو_Stmtsp(methodp->makeStmt());
    }

    Ast_ڳنڍ_Stmt* new_Pin_سائيز_Variable(File_سٽ* ثابت fl, AstVar* ثابت arrVarp,
                                    AstVar* ثابت genp) {
        Ast_ڳنڍ_Module* ثابت genModp = VN_AS(genp->user2p(), ڳنڍ_Module);
        AstVar* ثابت سائيز_Varp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ثابت pinp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtype_سيٽ_Void();
        AstCExpr* ثابت namep = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + سائيز_Varp->name() + "\""};
        namep->dtype_سيٽ_UInt32();
        pinp->جوڙيو_Pinsp(namep);
        pinp->جوڙيو_Pinsp(
            جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(سائيز_Varp->ويڪر())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ثابت سائيز_Var_Refp = جديد AstVarRef{fl, سائيز_Varp, VAccess::READ};
        سائيز_Var_Refp->classOrPackagep(VN_AS(سائيز_Varp->user2p(), ڳنڍ_Module));
        pinp->جوڙيو_Pinsp(سائيز_Var_Refp);
        أرجع pinp->makeStmt();
    }

    فراغ جوڙيو_Frozen_سائيز_Pin(File_سٽ* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                          AstVar* ثابت genp, AstVar* ثابت rand_گهڻائي_Varp) {
        ثابت Randomize_گهڻائي rmode = {.asUQuad = arrVarp->user1()};
        إذا (!rmode.استعمال_ڪري_گهڻائي) أرجع;
        AstVar* ثابت سائيز_Varp = VN_AS(arrVarp->user4p(), Var);
        Ast_ڳنڍ_Module* ثابت arrClassp = VN_AS(arrVarp->user2p(), ڳنڍ_Module);
        Ast_ڳنڍ_Module* ثابت سائيز_Classp = VN_AS(سائيز_Varp->user2p(), ڳنڍ_Module);
        AstVarRef* ثابت سائيز_Writep = جديد AstVarRef{fl, سائيز_Classp, سائيز_Varp, VAccess::WRITE};
        ثابت VCMethod سائيز_طريقو = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_سائيز
                                        : VCMethod::DYN_سائيز;
        AstCMethodHard* ثابت currentSizep = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, سائيز_طريقو};
        currentSizep->dtypep(سائيز_Varp->dtypep());
        randomizep->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, سائيز_Writep, currentSizep});
        Ast_ڳنڍ_اظهار* ثابت modep = new_گهڻائي_ٻٽو_انگ_Read(arrVarp, خالي_اشارو, rand_گهڻائي_Varp, fl);
        randomizep->جوڙيو_Stmtsp(
            جديد AstIf{fl, جديد AstLogNot{fl, modep}, new_Pin_سائيز_Variable(fl, arrVarp, genp)});
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_انگ);
        VL_RESTORER_COPY(m_rand_Array_Unique_نالا);
        m_modp = nodep;
        m_rand_Case_انگ = 0;
        m_rand_Array_Unique_نالا.reset();
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_انگ);
        VL_RESTORER(m_dist_انگ);
        m_modp = nodep;
        m_rand_Case_انگ = 0;
        m_dist_انگ = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        إذا (!nodep->user1()) أرجع;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->بنياد_Most_Classp()->needRNG(سچ);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ثابت منطقي basic_پهريون = classOwnsGlobalConstraint(nodep);

        File_سٽ* ثابت fl = nodep->fileline();
        AstFunc* ثابت randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ثابت fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ثابت rand_گهڻائي_Varp = get_Rand_گهڻائي_Var_From_Class(nodep);
        جوڙيو_Pre_Post_Call(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(nodep)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->جوڙيو_Stmtsp((جديد AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_ڳنڍ_اظهار* beginValp = خالي_اشارو;
        AstVar* genp = getRandomGenerator(nodep);
        إذا (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ثابت prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                إذا (!taskp) {
                    taskp = new_تياري_Constraint_Task(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                إذا (AstTask* ثابت resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ثابت resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ثابت resizeTaskRefp
                        = جديد AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->جوڙيو_Stmtsp(resizeTaskRefp->makeStmt());
                }

                إذا (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                إذا (constrp->itemsp()) {
                    هيٺاهون_Dist_Constraints(taskp, constrp->itemsp(), rand_گهڻائي_Varp);
                }
                VInsertion_سيٽ<AstVar*>& سائيز_Arrays = m_سائيز_Constrained_Arrays[classp];
                Constraint_اظهار_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), خالي_اشارو,
                    genp,          rand_گهڻائي_Varp, m_writtenVars,     &m_uniqueConstraintId,
                    m_unique_نالا, randomizep,   &سائيز_Arrays,       prepareArraysTaskp};
                إذا (constrp->itemsp()) {
                    taskp->جوڙيو_Stmtsp(wrap_If_Constraint_گهڻائي(
                        nodep, constrp, constrp->itemsp()->unlink_Fr_پٺ_With_ايندڙ()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ثابت خودڪار constraint_اونهائي = [](ثابت AstConstraint* constrp) {
                    صحيح اونهائي = 0;
                    سائيز_t pos = 0;
                    طالما ((pos = constrp->name().ڳوليو(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++اونهائي;
                        pos += std::لڙي_ڊيگهه(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    أرجع اونهائي;
                };
                صحيح max_اونهائي = 0;
                nodep->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    max_اونهائي = std::max(max_اونهائي, constraint_اونهائي(constrp));
                });
                AstTask* ثابت تياري_All_Taskp = get_Create_Constraint_تياري_Func(nodep, m_memberMap);
                لكل (صحيح d = max_اونهائي; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                            إذا (constraint_اونهائي(constrp) != d) أرجع;
                            AstTask* ثابت taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ثابت تياري_Task_Refp
                                = جديد AstTaskRef{constrp->fileline(), taskp};
                            تياري_Task_Refp->classOrPackagep(classp);
                            تياري_All_Taskp->جوڙيو_Stmtsp(تياري_Task_Refp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertion_سيٽ<AstVar*> سائيز_Array_Vars;
            إذا (nodep->extendsp()) {
                AstClass* والد_Classp = nodep->extendsp()->classp();
                طالما (والد_Classp) {
                    ثابت خودڪار سائيز_Arrays_It = m_سائيز_Constrained_Arrays.ڳوليو(والد_Classp);
                    إذا (سائيز_Arrays_It != m_سائيز_Constrained_Arrays.پڇاڙي()) {
                        لكل (AstVar* ثابت arrVarp : سائيز_Arrays_It->ٻيو) {
                            سائيز_Array_Vars.وجهو(arrVarp);
                        }
                    }
                    AstFunc* ثابت والد_Randomizep
                        = VN_CAST(m_memberMap.ڳوليو_Member(والد_Classp, "randomize"), Func);
                    إذا (والد_Randomizep && والد_Randomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        لكل (Ast_ڳنڍ* stmtp = والد_Randomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            منطقي مليو_Clear_Constraints = خطأ;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                إذا (methodp->طريقو() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->طريقو() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->جوڙيو_Stmtsp(stmtp->clone_وڻ(خطأ));
                                } وإلا إذا (methodp->طريقو()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    مليو_Clear_Constraints = سچ;
                                }
                            });
                            إذا (مليو_Clear_Constraints) اكسر;
                        }
                    }
                    والد_Classp
                        = والد_Classp->extendsp() ? والد_Classp->extendsp()->classp() : خالي_اشارو;
                }
            }
            randomizep->جوڙيو_Stmtsp(implementConstraintsClear(fl, genp));

            ثابت خودڪار سائيز_Arrays_It = m_سائيز_Constrained_Arrays.ڳوليو(nodep);
            إذا (سائيز_Arrays_It != m_سائيز_Constrained_Arrays.پڇاڙي()) {
                لكل (AstVar* ثابت arrVarp : سائيز_Arrays_It->ٻيو) سائيز_Array_Vars.وجهو(arrVarp);
            }
            AstTask* ثابت تياري_All_Taskp = get_Create_Constraint_تياري_Func(nodep, m_memberMap);
            إذا (!سائيز_Array_Vars.خالي_جايون() && prepareArraysTaskp->stmtsp()) {
                Ast_ڳنڍ_Stmt* ثابت preparep = (جديد AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                لكل (Ast_ڳنڍ* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    منطقي writesArray = خطأ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->طريقو() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    إذا (!writesArray) استمر;
                    stmtp->جوڙيو_هتي_This_As_ايندڙ(preparep);
                    اكسر;
                }
            }
            AstTaskRef* ثابت تياري_Task_Refp = جديد AstTaskRef{fl, تياري_All_Taskp};
            randomizep->جوڙيو_Stmtsp(تياري_Task_Refp->makeStmt());

            Ast_ڳنڍ_Module* ثابت genModp = VN_AS(genp->user2p(), ڳنڍ_Module);

            AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
            solverCallp->dtype_سيٽ_ٻٽو_انگ();
            solverCallp->جوڙيو(جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->جوڙيو(".next(__Vm_rng)");
            إذا (!سائيز_Array_Vars.خالي_جايون()) {
                AstVar* ثابت سائيز_Ok_Varp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::ٻٽو_انگ)};
                سائيز_Ok_Varp->funcLocal(سچ);
                randomizep->جوڙيو_Stmtsp(سائيز_Ok_Varp);

                AstVar* ثابت finalOkVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->ڳوليو_Basic_DType(VBasicDTypeKwd::ٻٽو_انگ)};
                finalOkVarp->funcLocal(سچ);
                randomizep->جوڙيو_Stmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                لكل (AstVar* ثابت arrVarp : سائيز_Array_Vars)
                    جوڙيو_Frozen_سائيز_Pin(fl, arrVarp, randomizep, genp, rand_گهڻائي_Varp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->جوڙيو_Stmtsp(
                    جديد Ast_سونپيو{fl, جديد AstVarRef{fl, سائيز_Ok_Varp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                إذا (AstTask* ثابت resizeAllTaskp
                    = VN_AS(m_memberMap.ڳوليو_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ثابت resizeTaskRefp = جديد AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ثابت ifp = جديد AstIf{fl, جديد AstVarRef{fl, سائيز_Ok_Varp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), خالي_اشارو};
                    randomizep->جوڙيو_Stmtsp(ifp);
                }

                // Refresh array element tables after resize
                لكل (ثابت خودڪار& arrVarp : سائيز_Array_Vars) {
                    جوڙيو_Array_Elem_Refresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->جوڙيو_Stmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ثابت تياري_Task_Refp2 = جديد AstTaskRef{fl, تياري_All_Taskp};
                randomizep->جوڙيو_Stmtsp(تياري_Task_Refp2->makeStmt());

                لكل (AstVar* ثابت arrVarp : سائيز_Array_Vars) {
                    randomizep->جوڙيو_Stmtsp(new_Pin_سائيز_Variable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ثابت solverCallp2 = جديد AstCExpr{fl};
                solverCallp2->dtype_سيٽ_ٻٽو_انگ();
                solverCallp2->جوڙيو(جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->جوڙيو(".next(__Vm_rng)");
                randomizep->جوڙيو_Stmtsp(جديد Ast_سونپيو{
                    fl, جديد AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = جديد AstAnd{fl, جديد AstVarRef{fl, سائيز_Ok_Varp, VAccess::READ},
                                       جديد AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } وإلا {
                beginValp = solverCallp;
            }
            إذا (rand_گهڻائي_Varp) {
                Ast_ڳنڍ_Module* ثابت rand_گهڻائي_Classp = VN_AS(rand_گهڻائي_Varp->user2p(), Class);
                Ast_ڳنڍ_FTask* ثابت newp
                    = VN_AS(m_memberMap.ڳوليو_Member(rand_گهڻائي_Classp, "new"), ڳنڍ_FTask);
                UASSERT_OBJ(newp, rand_گهڻائي_Classp, "No new() in class");
                جوڙيو_سيٽ_Rand_گهڻائي(newp, genp, rand_گهڻائي_Varp);
            }
            إذا (AstVar* ثابت static_Rand_گهڻائي_Varp = get_Static_Rand_گهڻائي_Var(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_ڳنڍ_FTask* ثابت newp = VN_AS(m_memberMap.ڳوليو_Member(nodep, "new"), ڳنڍ_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                جوڙيو_سيٽ_Static_Rand_گهڻائي(newp, genp, static_Rand_گهڻائي_Varp);
            }
        } وإلا {
            beginValp = جديد AstConst{fl, AstConst::Widthed_قدر{}, 32, 1};
        }

        جوڙيو_Update_Rand_Vars_Body(nodep, randomizep);

        AstFunc* ثابت basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        جوڙيو_Basic_Randomize_Body(basicRandomizep, nodep, rand_گهڻائي_Varp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ثابت fvarRefp = جديد AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->جوڙيو_Stmtsp(جديد Ast_سونپيو{
            fl, fvarRefp, basic_پهريون ? جديد AstFuncRef{fl, basicRandomizep} : beginValp});

        ثابت خودڪار سائيز_Arrays_It = m_سائيز_Constrained_Arrays.ڳوليو(nodep);
        ثابت منطقي needs_سائيز_Phase
            = سائيز_Arrays_It != m_سائيز_Constrained_Arrays.پڇاڙي() && !سائيز_Arrays_It->ٻيو.خالي_جايون();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ثابت خودڪار emitResizeFallback = [&]() {
            إذا (needs_سائيز_Phase) أرجع;
            إذا (AstTask* ثابت resizeAllTaskp
                = VN_AS(m_memberMap.ڳوليو_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ثابت resizeTaskRefp = جديد AstTaskRef{fl, resizeAllTaskp};
                randomizep->جوڙيو_Stmtsp(resizeTaskRefp->makeStmt());
            }
        };
        إذا (!basic_پهريون) emitResizeFallback();

        AstVarRef* ثابت fvarRefReadp = fvarRefp->clone_وڻ(خطأ);
        fvarRefReadp->access(VAccess::READ);

        Ast_ڳنڍ_اظهار* ثابت ٻيو_Halfp
            = basic_پهريون ? beginValp : جديد AstFuncRef{fl, basicRandomizep};
        randomizep->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, fvarRefp->clone_وڻ(خطأ),
                                            جديد AstAnd{fl, fvarRefReadp, ٻيو_Halfp}});

        إذا (basic_پهريون) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(nodep)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->جوڙيو_Stmtsp((جديد AstTaskRef{fl, postTaskp})->makeStmt());
        }

        جوڙيو_Pre_Post_Call(nodep, randomizep, "post_randomize");
        nodep->user1(خطأ);
    }
    فراغ گهمو(AstRandCase* nodep) override {
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
        Ast_ڳنڍ_DType* ثابت جوڙ_DTypep = nodep->ڳوليو_UInt64_DType();

        File_سٽ* ثابت fl = nodep->fileline();
        ثابت std::string name = "__Vrandcase" + cvt_منزل_Str(m_rand_Case_انگ++);
        AstVar* ثابت randVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, name, جوڙ_DTypep};
        randVarp->noSubst(سچ);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        جوڙيو_عارضي_Varsp(randVarp);
        Ast_ڳنڍ_اظهار* sump = جديد AstConst{fl, AstConst::Widthed_قدر{}, 64, 0};
        Ast_ڳنڍ_If* ثابت پهريون_Ifsp
            = جديد AstIf{fl, جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}}, خالي_اشارو, خالي_اشارو};
        Ast_ڳنڍ_If* ifsp = پهريون_Ifsp;

        لكل (Ast_Case_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شي)) {
            Ast_ڳنڍ_اظهار* ثابت condp = itemp->condsp()->unlink_Fr_پٺ();
            sump
                = جديد Ast_جوڙيو{condp->fileline(), sump, جديد AstExtend{itemp->fileline(), condp, 64}};
            Ast_ڳنڍ* ثابت stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ() : خالي_اشارو;
            AstVarRef* ثابت randVarRefp = جديد AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_ڳنڍ_If* ثابت newifp
                = جديد AstIf{itemp->fileline(),
                            جديد AstLte{condp->fileline(), randVarRefp, sump->clone_وڻ_خالص(سچ)},
                            stmtsp, خالي_اشارو};
            ifsp->جوڙيو_Elsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ثابت dispp = جديد AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            خالي_اشارو, خالي_اشارو};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->جوڙيو_Elsesp(dispp);

        Ast_ڳنڍ_اظهار* ثابت randp = جديد AstRand{fl, خالي_اشارو, خطأ};
        randp->dtype_سيٽ_UInt64();
        AstVarRef* ثابت randVarRefp = جديد AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_ڳنڍ* ثابت newp
            = جديد Ast_سونپيو{fl, randVarRefp,
                            جديد Ast_جوڙيو{fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                                       جديد AstModDiv{fl, randp, sump}}};
        newp->جوڙيو_ايندڙ(پهريون_Ifsp);
        إذا (debug() >= 9) newp->dump_وڻ_And_ايندڙ(ٻاهر, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        إذا (nodep->name() == "rand_mode") {
            Ast_طريقو_Call* ثابت طريقو_Callp = VN_CAST(nodep, طريقو_Call);
            Ast_ڳنڍ_اظهار* ثابت fromp = طريقو_Callp ? طريقو_Callp->fromp() : خالي_اشارو;
            ثابت Rand_گهڻائي_هدف rand_گهڻائي_هدف = Rand_گهڻائي_هدف::get(fromp, m_modp);
            UASSERT_OBJ(rand_گهڻائي_هدف.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ثابت receiverp = rand_گهڻائي_هدف.receiverp;
            ثابت منطقي isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_ڳنڍ* classLevelStaticLoopp = خالي_اشارو;
            إذا (isClassLevel && nodep->argsp()) {
                إذا (AstVar* ثابت sVarp = get_Static_Rand_گهڻائي_Var(rand_گهڻائي_هدف.classp)) {
                    File_سٽ* ثابت fl = nodep->fileline();
                    Ast_ڳنڍ_اظهار* ثابت staticLhsp
                        = make_گهڻائي_سونپيو_Lhs(fl, rand_گهڻائي_هدف.classp, خالي_اشارو, sVarp);
                    Ast_ڳنڍ_اظهار* ثابت argClonep = nodep->argsp()->exprp()->clone_وڻ_خالص(خطأ);
                    classLevelStaticLoopp = make_گهڻائي_سيٽ_Loop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ثابت منطقي receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ثابت rand_گهڻائي_Varp = receiverIsStaticRand
                                             ? get_Static_Rand_گهڻائي_Var(rand_گهڻائي_هدف.classp)
                                             : get_Rand_گهڻائي_Var_From_Class(rand_گهڻائي_هدف.classp);
            إذا (!rand_گهڻائي_Varp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_اظهار), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(ڌڪو_Deletep(m_stmtp), m_stmtp = خالي_اشارو);
                أرجع;
            }
            Ast_ڳنڍ_اظهار* ثابت lhsp = make_گهڻائي_سونپيو_Lhs(nodep->fileline(), rand_گهڻائي_هدف.classp,
                                                        rand_گهڻائي_هدف.fromp, rand_گهڻائي_Varp);
            replace_With_گهڻائي_سونپيو_And_Append(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : خالي_اشارو, lhsp,
                classLevelStaticLoopp);
            أرجع;
        }

        إذا (nodep->name() == "constraint_mode") {
            Ast_طريقو_Call* ثابت طريقو_Callp = VN_CAST(nodep, طريقو_Call);
            Ast_ڳنڍ_اظهار* fromp = طريقو_Callp ? طريقو_Callp->fromp() : خالي_اشارو;
            AstConstraint* constrp = خالي_اشارو;
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
            AstVar* ثابت constraint_گهڻائي_Varp = (constrp && constrp->isStatic())
                                                   ? get_Static_Constraint_گهڻائي_Var(classp)
                                                   : get_Constraint_گهڻائي_Var(classp);
            Ast_ڳنڍ_اظهار* ثابت lhsp
                = make_گهڻائي_سونپيو_Lhs(nodep->fileline(), classp, fromp, constraint_گهڻائي_Varp);
            replace_With_گهڻائي_سونپيو(nodep, constrp, lhsp);
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
            ثابت منطقي اندريون_Static_Context = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ثابت stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ثابت randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            إذا (اندريون_Static_Context) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(سچ);
            }
            randomizeFuncp->جوڙيو_Stmtsp(
                جديد Ast_سونپيو{nodep->fileline(),
                              جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              جديد AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            صحيح argn = 0;
            AstWith* ثابت withp = nodep->withp();
            لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_ڳنڍ_اظهار* ثابت exprp = argp->exprp();
                AstCMethodHard* ثابت basicMethodp = جديد AstCMethodHard{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ثابت refvarp
                    = جديد AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::منزل_string(++argn), exprp->dtypep()};
                refvarp->سيٽ_Std_Randomize_Arg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                إذا (withp) {
                    withp->foreach([&](Ast_ڳنڍ_اظهار* قوت_نما) {
                        إذا (is_Similar_ڳنڍ(قوت_نما, exprp)) {
                            AstVarRef* ثابت replaceVar
                                = جديد AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            قوت_نما->replaceWith(replaceVar);
                            replaceVar->user1(قوت_نما->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(ڌڪو_Deletep(قوت_نما), قوت_نما);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(سچ);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->جوڙيو_Stmtsp(refvarp);

                ثابت سائيز_t ويڪر = exprp->ويڪر();
                basicMethodp->جوڙيو_Pinsp(
                    جديد AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->جوڙيو_Pinsp(
                    جديد AstConst{nodep->fileline(), AstConst::Unsized64{}, ويڪر});
                basicMethodp->dtype_سيٽ_ٻٽو_انگ();

                randomizeFuncp->جوڙيو_Stmtsp(جديد Ast_سونپيو{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    جديد AstAnd{nodep->fileline(),
                               جديد AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            إذا (withp) {
                File_سٽ* ثابت fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, خالي_اشارو,
                                                                اندريون_Static_Context);
                withCapturep->جوڙيو_Function_Arguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                إذا (stdrand) {
                    randomizeFuncp->جوڙيو_Stmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_ڳنڍ* ثابت capturedTreep = withp->exprp()->unlink_Fr_پٺ_With_ايندڙ();
                randomizeFuncp->جوڙيو_Stmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_اظهار_Visitor{
                        خالي_اشارو, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        خالي_اشارو, m_writtenVars, &m_uniqueConstraintId, m_unique_نالا,  خالي_اشارو};
                }
                AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
                solverCallp->dtype_سيٽ_ٻٽو_انگ();
                solverCallp->جوڙيو(جديد AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->جوڙيو(".next()");
                AstVar* ثابت fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_ڳنڍ_اظهار* ثابت andExprp
                    = جديد AstAnd{fl, جديد AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->جوڙيو_Stmtsp(
                    جديد Ast_سونپيو{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlink_Fr_پٺ()->delete_وڻ(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            إذا (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            إذا (withCapturep) nodep->جوڙيو_Argsp(withCapturep->get_دليلون());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            أرجع;
        }
        handle_Randomize_دليلون(nodep);

        AstWith* ثابت withp = nodep->withp();
        إذا (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            أرجع;
        }
        withp->unlink_Fr_پٺ();
        iterateChildren(nodep);

        AstClass* classp = خالي_اشارو;
        إذا (Ast_طريقو_Call* ثابت callp = VN_CAST(nodep, طريقو_Call)) {
            ثابت Ast_ڳنڍ_DType* ثابت fromDTypep = callp->fromp()->dtypep();
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
                         classp->ڳوليو_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(سچ);

        AstFunc* ثابت randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inline_Unique_نالا.get(nodep), خطأ);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ثابت std::pair<منطقي, منطقي> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<منطقي, منطقي>{خطأ, خطأ};
        إذا (prePostWrap.پهريون) {
            جوڙيو_Virtual_Pre_Post_Call(randomizeFuncp, classp, "pre_randomize");
        } وإلا {
            جوڙيو_Pre_Post_Call(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(classp)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(classp, "pre");
            إذا (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->جوڙيو_Stmtsp((جديد AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ثابت CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.جوڙيو_Function_Arguments(randomizeFuncp);

        // Add constraints clearing code
        إذا (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ثابت mainRandomizep
                = VN_CAST(m_memberMap.ڳوليو_Member(classp, "randomize"), Func);
            إذا (mainRandomizep && mainRandomizep->stmtsp()) {
                لكل (Ast_ڳنڍ* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    منطقي مليو_Clear_Constraints = خطأ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        إذا (methodp->طريقو() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->طريقو() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->جوڙيو_Stmtsp(stmtp->clone_وڻ(خطأ));
                        } وإلا إذا (methodp->طريقو() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            مليو_Clear_Constraints = سچ;
                        }
                    });
                    إذا (مليو_Clear_Constraints) اكسر;
                }
            }
            randomizeFuncp->جوڙيو_Stmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->جوڙيو_Stmtsp(localGenp);

        AstFunc* ثابت basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ثابت basicRandomizeFuncCallp
            = جديد AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        إذا (classGenp) {
            AstTask* ثابت constr_تياري_Funcp = get_Create_Constraint_تياري_Func(classp, m_memberMap);
            AstTaskRef* ثابت callp = جديد AstTaskRef{nodep->fileline(), constr_تياري_Funcp};
            randomizeFuncp->جوڙيو_Stmtsp(callp->makeStmt());
            randomizeFuncp->جوڙيو_Stmtsp(جديد Ast_سونپيو{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                جديد AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), ڳنڍ_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ثابت rand_گهڻائي_Varp = get_Rand_گهڻائي_Var_From_Class(classp);
        إذا (!classGenp && rand_گهڻائي_Varp) جوڙيو_سيٽ_Rand_گهڻائي(randomizeFuncp, localGenp, rand_گهڻائي_Varp);
        إذا (!classGenp) {
            إذا (AstVar* ثابت sVarp = get_Static_Rand_گهڻائي_Var(classp)) {
                جوڙيو_سيٽ_Static_Rand_گهڻائي(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_ڳنڍ* ثابت capturedTreep = withp->exprp()->unlink_Fr_پٺ_With_ايندڙ();
        randomizeFuncp->جوڙيو_Stmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_ڳنڍ* inlineResizeStmtsp = خالي_اشارو;
        {
            std::ويڪٽر<AstCMethodHard*> سائيز_Methodps;
            capturedTreep->foreach_And_ايندڙ([&](AstCMethodHard* methodp) {
                إذا (methodp->طريقو() == VCMethod::DYN_سائيز
                    || methodp->طريقو() == VCMethod::ASSOC_سائيز) {
                    سائيز_Methodps.ڌڪو_پٺ(methodp);
                }
            });
            لكل (AstCMethodHard* ثابت methodp : سائيز_Methodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = خالي_اشارو;
                Ast_ڳنڍ_اظهار* ثابت پاڙ_Exprp = methodp->fromp();
                إذا (AstMemberSel* memberSelp = VN_CAST(پاڙ_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    طالما (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ثابت Ast_ڳنڍ_اظهار* ثابت fromp = memberSelp->fromp();
                    إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(fromp, VarRef)) {
                        إذا (varRefp->varp() == captured.getThisp()) استمر;
                    } وإلا {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } وإلا إذا (AstVarRef* ثابت varRefp = VN_CAST(پاڙ_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                إذا (!arrVarp) استمر;
                // Only handle rand-declared dynamic/assoc array variables
                إذا (!arrVarp->rand().isRandomizable()) استمر;
                // Do not randomize associative array size (IEEE 18.4)
                إذا (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) استمر;
                File_سٽ* ثابت fl = methodp->fileline();
                منطقي wasCreated = خطأ;
                AstClass* ثابت varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ثابت سائيز_Varp = create_Or_Get_سائيز_Var(varClassp, arrVarp, fl,
                                                            methodp->ڳوليو_Int_DType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                إذا (wasCreated) {
                    Ast_ڳنڍ_اظهار* ثابت resizeVarRefp = buildMemberSelExprp(پاڙ_Exprp, arrVarp);
                    Ast_ڳنڍ_اظهار* ثابت سائيز_Exprp = buildMemberSelExprp(پاڙ_Exprp, سائيز_Varp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->جوڙيو_ايندڙ(create_سائيز_Gte_Zero_Constraint(fl, سائيز_Varp, پاڙ_Exprp));
                    AstCMethodHard* ثابت resizep
                        = جديد AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, سائيز_Exprp};
                    resizep->dtypep(methodp->ڳوليو_Void_DType());
                    inlineResizeStmtsp
                        = Ast_ڳنڍ::جوڙيو_ايندڙ(inlineResizeStmtsp, جديد Ast_Stmt_اظهار{fl, resizep});
                }
                Ast_ڳنڍ_اظهار* ثابت سائيز_ڳنڍ_Exprp = buildMemberSelExprp(پاڙ_Exprp, سائيز_Varp);
                methodp->replaceWith(سائيز_ڳنڍ_Exprp);
                VL_DO_DANGLING(methodp->delete_وڻ(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_اظهار_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                rand_گهڻائي_Varp, m_writtenVars, &m_uniqueConstraintId, m_unique_نالا,  خالي_اشارو};
        }

        // Call the solver and set return value
        AstCExpr* ثابت solverCallp = جديد AstCExpr{nodep->fileline()};
        solverCallp->dtype_سيٽ_ٻٽو_انگ();
        solverCallp->جوڙيو(جديد AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->جوڙيو(".next(__Vm_rng)");
        randomizeFuncp->جوڙيو_Stmtsp(جديد Ast_سونپيو{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            جديد AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        إذا (inlineResizeStmtsp) {
            AstVar* ثابت retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ثابت ifp = جديد AstIf{nodep->fileline(),
                                         جديد AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, خالي_اشارو};
            randomizeFuncp->جوڙيو_Stmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(classp)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(classp, "post");
            إذا (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->جوڙيو_Stmtsp((جديد AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        إذا (prePostWrap.ٻيو) {
            جوڙيو_Virtual_Pre_Post_Call(randomizeFuncp, classp, "post_randomize");
        } وإلا {
            جوڙيو_Pre_Post_Call(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->جوڙيو_Argsp(captured.get_دليلون());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_وڻ(), withp);
    }
    فراغ گهمو(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_سٽ* ثابت fl = nodep->fileline();
        إذا (m_constraintp && nodep->fromp()->user1()
            && (nodep->طريقو() == VCMethod::ASSOC_سائيز
                || nodep->طريقو() == VCMethod::DYN_سائيز)) {
            AstClass* ثابت classp = VN_AS(m_modp, Class);
            AstVarRef* ثابت قطار_Var_Refp = VN_CAST(nodep->fromp(), VarRef);
            إذا (!قطار_Var_Refp) {
                // Warning from ConstraintExprVisitor will be thrown
                أرجع;
            }
            AstVar* ثابت قطار_Varp = قطار_Var_Refp->varp();
            منطقي wasCreated = خطأ;
            AstVar* ثابت سائيز_Varp
                = create_Or_Get_سائيز_Var(classp, قطار_Varp, fl, nodep->ڳوليو_Int_DType(), wasCreated);
            إذا (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                إذا (!VN_IS(قطار_Varp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    إذا (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ثابت سائيز_Var_Refp = جديد AstVarRef{
                        fl, VN_AS(سائيز_Varp->user2p(), ڳنڍ_Module), سائيز_Varp, VAccess::READ};
                    AstCMethodHard* ثابت resizep = جديد AstCMethodHard{
                        fl, nodep->fromp()->unlink_Fr_پٺ(), VCMethod::DYN_RESIZE, سائيز_Var_Refp};
                    resizep->dtypep(nodep->ڳوليو_Void_DType());
                    resizerTaskp->جوڙيو_Stmtsp(جديد Ast_Stmt_اظهار{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->جوڙيو_Itemsp(create_سائيز_Gte_Zero_Constraint(fl, سائيز_Varp));
            }
            AstVarRef* ثابت سائيز_Var_Refp = جديد AstVarRef{fl, VN_AS(سائيز_Varp->user2p(), ڳنڍ_Module),
                                                         سائيز_Varp, VAccess::READ};
            سائيز_Var_Refp->user1(سچ);
            nodep->replaceWith(سائيز_Var_Refp);
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح RandomizeVisitor(AstNetlist* nodep)
        : m_inline_Unique_نالا{"__Vrandwith"} {
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
                    إذا (VN_AS(varp->user2p(), ڳنڍ_Module) != ownerClassp) أرجع;
                    إذا (!varp->user3()) varp->user3(سچ);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(ڌڪو_Deletep(constrp->unlink_Fr_پٺ()), constrp);
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
    V3Global::dump_Check_Global_وڻ("randomize", 0, dump_وڻ_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ثابت std::string& name, منطقي allowVirtual,
                                       منطقي ٻار_DType) {
    AstFunc* funcp = VN_AS(memberMap.ڳوليو_Member(nodep, name), Func);
    إذا (!funcp) {
        v3Global.useRandomizeMethods(سچ);
        Ast_ڳنڍ_DType* ثابت dtypep
            = ٻار_DType
                  ? جديد AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->ڳوليو_ٻٽو_انگ_DType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ثابت fvarp = ٻار_DType
                                  ? جديد AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlag_ٻار_DType{}, dtypep}
                                  : جديد AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3_Error_ڪوڊ::NORETURN, سچ);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(سچ);
        fvarp->funcReturn(سچ);
        fvarp->direction(VDirection::OUTPUT);
        nodep->جوڙيو_Membersp(funcp);
        funcp = جديد AstFunc{nodep->fileline(), name, خالي_اشارو, fvarp};
        إذا (!ٻار_DType) funcp->dtypep(dtypep);
        funcp->class_طريقو(سچ);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->جوڙيو_Membersp(funcp);
        memberMap.وجهو(nodep, funcp);
    }
    أرجع funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_ڳنڍ_Module* nodep, ثابت std::string& name) {
    AstFunc* funcp = خالي_اشارو;
    v3Global.useRandomizeMethods(سچ);
    Ast_ڳنڍ_DType* ثابت dtypep = nodep->ڳوليو_ٻٽو_انگ_DType(32, 32, VSigning::SIGNED);
    AstVar* ثابت fvarp = جديد AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(سچ);
    fvarp->funcReturn(سچ);
    fvarp->direction(VDirection::OUTPUT);
    funcp = جديد AstFunc{nodep->fileline(), name, خالي_اشارو, fvarp};
    funcp->dtypep(dtypep);
    إذا (VN_IS(nodep, Class)) {
        funcp->class_طريقو(سچ);
    } وإلا {
        funcp->class_طريقو(سچ);
        funcp->isStatic(سچ);
    }
    nodep->جوڙيو_Stmtsp(funcp);
    أرجع funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ثابت basep = nodep->بنياد_Most_Classp();
    AstFunc* funcp = VN_AS(memberMap.ڳوليو_Member(basep, "srandom"), Func);
    إذا (!funcp) {
        v3Global.useRandomizeMethods(سچ);
        Ast_ڳنڍ_DType* ثابت dtypep
            = basep->ڳوليو_ٻٽو_انگ_DType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ثابت ivarp = جديد AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(سچ);
        ivarp->direction(VDirection::داخلا);
        funcp = جديد AstFunc{basep->fileline(), "srandom", ivarp, خالي_اشارو};
        funcp->dtypep(basep->ڳوليو_Void_DType());
        funcp->class_طريقو(سچ);
        funcp->isVirtual(خطأ);
        basep->جوڙيو_Membersp(funcp);
        memberMap.وجهو(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        إذا (basep->name() == "process") {
            funcp->جوڙيو_Stmtsp(جديد AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } وإلا {
            funcp->جوڙيو_Stmtsp(جديد AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(سچ);
        }
    }
    أرجع funcp;
}
