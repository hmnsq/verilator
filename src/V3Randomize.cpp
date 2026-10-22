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
ساكن AstVar* get_Rand_منوال_Var_From_Class(Ast_گرہ_Module* classp) {
    طالما (classp) {
        إذا (classp->user2p()) أرجع VN_AS(classp->user2p(), Var);
        AstClass* ثابت cp = VN_CAST(classp, Class);
        إذا (!cp || !cp->extendsp()) أرجع خالی_سوچک;
        classp = cp->extendsp()->classp();
    }
    أرجع خالی_سوچک;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ساكن AstVar* getRandomGenerator(AstClass* ثابت classp) {
    إذا (classp->user3p()) أرجع VN_AS(classp->user3p(), Var);
    إذا (classp->extendsp()) أرجع getRandomGenerator(classp->extendsp()->classp());
    أرجع خالی_سوچک;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ساكن AstTask* get_Create_Constraint_تیاری_Func(AstClass* classp, VMemberMap& memberMap) {
    ساكن ثابت حرف* ثابت name = "__Vsetup_constraints";
    AstTask* تیاری_All_Taskp = VN_AS(memberMap.لبھو_Member(classp, name), Task);
    إذا (تیاری_All_Taskp) أرجع تیاری_All_Taskp;
    تیاری_All_Taskp = جديد AstTask{classp->fileline(), "__Vsetup_constraints", خالی_سوچک};
    تیاری_All_Taskp->class_طریقہ(سچ);
    تیاری_All_Taskp->isVirtual(سچ);
    classp->جوڑو_Membersp(تیاری_All_Taskp);
    memberMap.داخل_کرو(classp, تیاری_All_Taskp);
    أرجع تیاری_All_Taskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

بنية Rand_منوال_نشانہ final {
    // MEMBERS
    AstVar* ثابت receiverp;  // Variable that is the target of the rand_mode() call
    Ast_گرہ_اظہاریہ* ثابت fromp;  // Expression from which the rand_mode() call originates
    AstClass* ثابت classp;  // Class in which rand_mode should be set

    // METHODS
    ساكن Rand_منوال_نشانہ get(Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_Module* modp) {
        إذا (!fromp) أرجع {خالی_سوچک, خالی_سوچک, VN_AS(modp, Class)};
        إذا (AstCMethodHard* ثابت طریقہ_Hardp = VN_CAST(fromp, CMethodHard)) {
            أرجع Rand_منوال_نشانہ::get(طریقہ_Hardp->fromp(), modp);
        }
        AstVar* receiverp = خالی_سوچک;
        AstClass* classp = VN_CAST(modp, Class);
        إذا (AstVarRef* ثابت varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            إذا (receiverp->isRand()) {
                fromp = خالی_سوچک;
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

اتحاد Randomize_منوال final {
    // MEMBERS
    بنية {
        منطقي ورتدا_منوال : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t اشاریہ : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ساكن Ast_گرہ_DType* arrayElementDTypep(Ast_گرہ_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    طالما (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    أرجع dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ساكن منطقي dtype_شامل_اے_Real(Ast_گرہ_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    إذا (Ast_گرہ_DType* ثابت subp = dtypep->subDTypep()) أرجع dtype_شامل_اے_Real(subp);
    إذا (ثابت Ast_گرہ_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
        إذا (structp->packed()) أرجع خطأ;
        لكل (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            إذا (dtype_شامل_اے_Real(memberp->subDTypep())) أرجع سچ;
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
ساكن فراغ checkRandTypeEligibility(Ast_گرہ* contextp, Ast_گرہ_DType* dtypep, منطقي isRandc) {
    dtypep = dtypep->skipRefp();
    إذا (Ast_گرہ_DType* ثابت subp = dtypep->subDTypep()) {
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
    إذا (ثابت Ast_گرہ_UOr_Struct_DType* ثابت structp = VN_CAST(dtypep, گرہ_UOr_Struct_DType)) {
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
    ثابت VNUser1_وچ_Use m_inuser1;
    ثابت VNUser2_وچ_Use m_inuser2;

    ورتوں Derived_سیٹ = std::unordered_سیٹ<AstClass*>;
    ورتوں ادھار_منزل_Derived_Map = std::unordered_map<ثابت AstClass*, Derived_سیٹ>;

    ادھار_منزل_Derived_Map m_ادھار_منزل_Derived_Map;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = خالی_سوچک;  // Current class
    Ast_گرہ* m_constraint_اظہاریہ_Genp = خالی_سوچک;  // Current constraint or constraint if expression
    Ast_گرہ_Module* m_modp;  // Current module
    Ast_گرہ_Stmt* m_stmtp = خالی_سوچک;  // Current statement
    Ast_گرہ_FTask_Ref* m_stdRandCallp = خالی_سوچک;  // Current std::randomize() call
    منطقي m_وچ_Std_With = خطأ;  // True when inside a 'with {}' clause
    std::سیٹ<Ast_گرہ_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = خالی_سوچک;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    فراغ markGlobalConstrainedVars(AstClass* classp) {
        لكل (ثابت AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : خالی_سوچک) {
            لكل (Ast_گرہ* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                إذا (!varp) استمر;
                إذا (varp->rand().isRandomizable()) varp->globalConstrained(سچ);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    منطقي is_Var_وچ_Std_Randomize_دلیلاں(ثابت AstVar* varp) ثابت {
        إذا (!m_وچ_Std_With || !m_stdRandCallp) أرجع خطأ;

        لكل (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ثابت Ast_گرہ_اظہاریہ* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            طالما (exprp) {
                إذا (ثابت Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(exprp, گرہ_Var_Ref)) {
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
             classp = classp->extendsp() ? classp->extendsp()->classp() : خالی_سوچک) {
            لكل (Ast_گرہ* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ثابت varp = VN_CAST(memberp, Var);
                إذا (!varp) استمر;
                // If member is randomizable and of class type, mark its class
                إذا (varp->rand().isRandomizable()) {
                    ثابت Ast_گرہ_DType* ثابت varDtypep = varp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    إذا (!classRefp) {
                        ثابت Ast_گرہ_DType* subDTypep = varDtypep->subDTypep();
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
                        Randomize_منوال rand_منوال = {};
                        rand_منوال.ورتدا_منوال = سچ;
                        varp->user1(rand_منوال.asUQuad);
                    }
                }
            }
        }
    }
    فراغ markDerived(ثابت AstClass* nodep) {
        ثابت خودکار it = m_ادھار_منزل_Derived_Map.لبھو(nodep);
        إذا (it != m_ادھار_منزل_Derived_Map.سرا()) {
            لكل (خودکار* classp : it->دوجا) {
                إذا (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    فراغ markAllDerived() {
        لكل (ثابت خودکار& p : m_ادھار_منزل_Derived_Map) {
            إذا (p.پہلا->user1()) markDerived(p.پہلا);
        }
    }
    فراغ سیٹ_Package_Refs() {
        لكل (Ast_گرہ_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), گرہ_Module));
        }
    }
    فراغ markNestedGlobalConstrainedRecurse(Ast_گرہ* nodep) {
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
    Ast_گرہ_اظہاریہ* buildMemberSelChain(AstVarRef* جڑ_Var_Refp, ثابت std::سمتیہ<AstVar*>& رستہ) {
        Ast_گرہ_اظہاریہ* exprp = جڑ_Var_Refp->clone_درخت(خطأ);
        لكل (AstVar* memberVarp : رستہ) {
            AstMemberSel* memberSelp
                = جديد AstMemberSel{جڑ_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        أرجع exprp;
    }

    // Process a single constraint during nested constraint cloning
    فراغ processNestedConstraint(AstConstraint* ثابت constrp, AstVarRef* جڑ_Var_Refp,
                                 ثابت std::سمتیہ<AstVar*>& new_رستہ, AstClass* نشانہ_Classp) {
        std::string رستہ_اگیتر = جڑ_Var_Refp->name();
        لكل (AstVar* رستہ_Member_Varp : new_رستہ) {
            رستہ_اگیتر += GLOBAL_CONSTRAINT_SEPARATOR + رستہ_Member_Varp->name();
        }

        ثابت std::string newName = رستہ_اگیتر + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        منطقي isDuplicate = خطأ;
        نشانہ_Classp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت existingConstrp) {
            إذا (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = سچ;
            }
        });
        إذا (isDuplicate) أرجع;

        AstConstraint* ثابت cloneConstrp = constrp->clone_درخت(خطأ);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::سمتیہ<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_گرہ_اظہاریہ* exprp) {
            إذا (AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
                إذا (!varRefp->varp()->isClassMember()) أرجع;
                Ast_گرہ_اظہاریہ* ثابت chainp = buildMemberSelChain(جڑ_Var_Refp, new_رستہ);
                AstMemberSel* ثابت finalSelp
                    = جديد AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(ڈالو_Deletep(varRefp), varRefp);
            } وإلا إذا (AstFuncRef* ثابت funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                إذا (funcRefp->taskp()->class_طریقہ() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.ڈالو_پٹھ(funcRefp);
                }
            }
        });
        لكل (AstFuncRef* funcRefp : funcRefps) {
            Ast_گرہ_اظہاریہ* ثابت chainp = buildMemberSelChain(جڑ_Var_Refp, new_رستہ);
            AstArg* ثابت argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlink_Fr_پٹھ_With_اگلا() : خالی_سوچک;
            Ast_طریقہ_Call* ثابت callp
                = جديد Ast_طریقہ_Call{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(ڈالو_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        نشانہ_Classp->جوڑو_Stmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    فراغ cloneNestedConstraintsRecurse(AstVarRef* جڑ_Var_Refp, AstClass* classp,
                                       ثابت std::سمتیہ<AstVar*>& رستہ_منزل_Class,
                                       AstClass* نشانہ_Classp) {
        لكل (Ast_گرہ* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ثابت memberVarp = VN_CAST(memberNodep, Var);
            إذا (!memberVarp) استمر;
            إذا (!memberVarp->rand().isRandomizable()) استمر;
            ثابت AstClassRefDType* ثابت memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            إذا (!memberClassRefp || !memberClassRefp->classp()) استمر;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::سمتیہ<AstVar*> new_رستہ = رستہ_منزل_Class;
            new_رستہ.ڈالو_پٹھ(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ثابت containingClassp, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, جڑ_Var_Refp, new_رستہ, نشانہ_Classp);
                });
            cloneNestedConstraintsRecurse(جڑ_Var_Refp, nestedClassp, new_رستہ, نشانہ_Classp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    فراغ handle_Randomize_دلیل(Ast_گرہ_اظہاریہ* exprp, AstVar* ثابت varp, ثابت منطقي stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        طالما (exprp) {
            AstVar* randVarp = خالی_سوچک;
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
                exprp = خالی_سوچک;
            } وإلا {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            إذا (randVarp == varp) أرجع;
            Ast_گرہ* backp = randVarp;
            طالما (backp && !VN_IS(backp, گرہ_Module)) backp = backp->backp();
            إذا (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } وإلا {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                Randomize_منوال rand_منوال = {};
                rand_منوال.ورتدا_منوال = سچ;
                randVarp->user1(rand_منوال.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    فراغ ویکھو(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        إذا (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ثابت AstClass* ثابت basep = nodep->extendsp()->classp();
            m_ادھار_منزل_Derived_Map[basep].داخل_کرو(nodep);
        }
    }
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->backp()) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        إذا (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        إذا (nodep->name() == "rand_mode") {
            Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(nodep, طریقہ_Call);
            Ast_گرہ_اظہاریہ* ثابت fromp = طریقہ_Callp ? طریقہ_Callp->fromp() : خالی_سوچک;
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
            } وإلا إذا (AstCMethodHard* ثابت طریقہ_Hardp = VN_CAST(fromp, CMethodHard)) {
                إذا (طریقہ_Hardp->طریقہ() == VCMethod::ARRAY_AT
                    || طریقہ_Hardp->طریقہ() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = خطأ;
                } وإلا {
                    طریقہ_Hardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            إذا (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_اظہاریہ)
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_کوڈ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = خطأ;
            }
            إذا (valid) {
                ثابت Rand_منوال_نشانہ rand_منوال_نشانہ = Rand_منوال_نشانہ::get(fromp, m_classp);
                إذا ((!rand_منوال_نشانہ.receiverp || !rand_منوال_نشانہ.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = خطأ;
                } وإلا إذا (!rand_منوال_نشانہ.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = خطأ;
                } وإلا إذا (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = خطأ;
                } وإلا إذا (rand_منوال_نشانہ.receiverp && rand_منوال_نشانہ.receiverp->isRand()) {
                    // Called on a rand member variable
                    Randomize_منوال rand_منوال = {};
                    rand_منوال.ورتدا_منوال = سچ;
                    rand_منوال_نشانہ.receiverp->user1(rand_منوال.asUQuad);
                } وإلا {
                    // Called on 'this' or a non-rand class instance
                    rand_منوال_نشانہ.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        إذا (!varp->isRand()) أرجع;
                        Randomize_منوال rand_منوال = {};
                        rand_منوال.ورتدا_منوال = سچ;
                        varp->user1(rand_منوال.asUQuad);
                    });
                }
            }
            إذا (!valid) {
                إذا (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
                    nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } وإلا {
                    m_stmtp->unlink_Fr_پٹھ();
                }
            }
            أرجع;
        }

        إذا (nodep->name() == "constraint_mode") {
            منطقي valid = سچ;
            إذا (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = خطأ;
            } وإلا إذا (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_اظہاریہ)
                       && !nodep->backp()->fileline()->warnIsOff(V3_Error_کوڈ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = خطأ;
            }
            AstConstraint* constrp = خالی_سوچک;
            AstClass* classp = m_classp;
            إذا (ثابت Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(nodep, طریقہ_Call)) {
                إذا (ثابت AstConstraintRef* ثابت constrRefp
                    = VN_CAST(طریقہ_Callp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    إذا (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } وإلا إذا (AstClassRefDType* classRefDtp
                           = VN_CAST(طریقہ_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
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
                Randomize_منوال constraint_منوال = {};
                constraint_منوال.ورتدا_منوال = سچ;
                إذا (constrp) {
                    constrp->user1(constraint_منوال.asUQuad);
                } وإلا {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraint_منوال.asUQuad);
                    });
                }
            } وإلا {
                إذا (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_اظہاریہ)) {
                    nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } وإلا {
                    m_stmtp->unlink_Fr_پٹھ();
                }
            }
            أرجع;
        }

        إذا (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            إذا (m_constraint_اظہاریہ_Genp || m_وچ_Std_With) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                لكل (Ast_گرہ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    إذا (argp->user1()) {
                        nodep->user1(سچ);
                        اكسر;
                    }
                }
            }
            أرجع;
        }
        AstClass* classp = m_classp;
        إذا (ثابت Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(nodep, طریقہ_Call)) {
            إذا (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(طریقہ_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        إذا (classp) {
            إذا (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ثابت, AstVar* ثابت memberVarp) {
                إذا (!memberVarp->rand().isRandomizable()) أرجع;
                ثابت Ast_گرہ_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                إذا (!classRefp || !classRefp->classp()) أرجع;
                AstClass* ثابت memberClassp = classRefp->classp();
                إذا (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) أرجع;
                memberVarp->globalConstrained(سچ);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* جڑ_Var_Refp
                    = جديد AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::سمتیہ<AstVar*> خالی_تھاواں_رستہ;
                memberClassp->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    processNestedConstraint(constrp, جڑ_Var_Refp, خالی_تھاواں_رستہ, classp);
                });
                cloneNestedConstraintsRecurse(جڑ_Var_Refp, memberClassp, خالی_تھاواں_رستہ, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(جڑ_Var_Refp->delete_درخت(), جڑ_Var_Refp);
            });
        }
        إذا (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = خالی_سوچک;
            لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handle_Randomize_دلیل(argp->exprp(), خالی_سوچک, سچ);
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
                          ثابت Ast_گرہ_DType* ثابت dtp = memberVarp->dtypep()->skipRefp();
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
            AstVar* fromVarp = خالی_سوچک;  // If nodep is a method call, this is its receiver
            إذا (Ast_طریقہ_Call* طریقہ_Callp = VN_CAST(nodep, طریقہ_Call)) {
                إذا (AstMemberSel* ثابت memberSelp = VN_CAST(طریقہ_Callp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } وإلا {
                    AstVarRef* ثابت varrefp = VN_AS(طریقہ_Callp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handle_Randomize_دلیل(argp->exprp(), fromVarp, خطأ);
        }
        // Re-mark members after the class became inline-randomized above.
        إذا (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    فراغ ویکھو(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_اظہاریہ_Genp);
        m_stmtp = nodep;
        m_constraint_اظہاریہ_Genp = nodep;
        iterateChildren(nodep);
        إذا (!nodep->backp()) VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(Ast_Constraint_اظہاریہ* nodep) override {
        VL_RESTORER(m_constraint_اظہاریہ_Genp);
        m_constraint_اظہاریہ_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_اظہاریہ_Genp);
            m_constraint_اظہاریہ_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterate_And_اگلا_Const_Null(nodep->thensp());
        iterate_And_اگلا_Const_Null(nodep->elsesp());
    }
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        إذا (!m_constraint_اظہاریہ_Genp) أرجع;

        إذا (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        إذا (nodep->varp()->rand().isRandomizable() && !(m_وچ_Std_With && m_stdRandCallp)) {
            nodep->user1(سچ);
        } وإلا إذا (is_Var_وچ_Std_Randomize_دلیلاں(nodep->varp())) {
            nodep->user1(سچ);
        }
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        إذا (!m_constraint_اظہاریہ_Genp) أرجع;
        iterateChildrenConst(nodep);
        ثابت منطقي randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ثابت منطقي randMember = nodep->varp()->rand().isRandomizable();
        ثابت منطقي وچ_Std_With = m_وچ_Std_With && m_stdRandCallp;
        إذا (randObject && randMember && !وچ_Std_With) {
            nodep->user1(سچ);
        } وإلا إذا (وچ_Std_With && is_Var_وچ_Std_Randomize_دلیلاں(nodep->varp())) {
            nodep->user1(سچ);
            // Mark parent object for constraint expression visitor
            إذا (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(سچ);
        }

        إذا (m_withp) {
            Ast_گرہ* backp = m_withp;
            طالما (backp->backp()) {
                إذا (ثابت Ast_طریقہ_Call* ثابت callp = VN_CAST(backp, طریقہ_Call)) {
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
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstVar* nodep) override {
        nodep->user2p(m_modp);
        إذا (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        إذا (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_وچ_Std_With = سچ;
        iterateChildrenConst(nodep);
        m_وچ_Std_With = خطأ;
    }

    فراغ ویکھو(Ast_گرہ_اظہاریہ* nodep) override {
        iterateChildrenConst(nodep);
        إذا (!m_constraint_اظہاریہ_Genp && !m_وچ_Std_With) أرجع;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    فراغ ویکھو(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        إذا (!m_constraint_اظہاریہ_Genp && !m_وچ_Std_With) أرجع;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    واضح RandomizeMarkVisitor(Ast_گرہ* nodep) {
        iterateConst(nodep);
        markAllDerived();
        سیٹ_Package_Refs();
    }
    ~RandomizeMarkVisitor() override = افتراضي;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

صنف Constraint_اظہاریہ_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ثابت m_classp;
    Ast_گرہ_FTask* ثابت m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_گرہ_FTask* ثابت m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ثابت m_genp;  // VlRandomizer variable of the class
    AstVar* m_rand_منوال_Varp;  // Relevant randmode state variable
    منطقي m_wantSingle = خطأ;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    منطقي m_structSel = خطأ;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    منطقي m_defer_Rand_منوال_Hoist = خطأ;  // Wait for the complete struct member path
    std::سیٹ<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::سیٹ<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertion_سیٹ<AstVar*>* m_ناپ_Constrained_Arraysp = خالی_سوچک;  // Arrays with size constraints
    Ast_گرہ_اظہاریہ* m_conditionp = خالی_سوچک;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_گرہ_FTask* m_prepareConstrainedArraysp = خالی_سوچک;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = خالی_سوچک;  // Current ID of unique call
    V3_Unique_ناں& m_unique_ناں;  // Unique names of temporaries, and of blocks holding them
    std::سمتیہ<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_گرہ* m_پہلا_عبارت_Inside_Indexp = خالی_سوچک;
    AstConstraintForeach* m_foreachp = خالی_سوچک;  // Innermost constraint-foreach being processed
    std::سیٹ<ثابت AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    صنف Nested_Access_رستہ final {
        AstMemberSel* m_چوٹی_Nested_Array_Member_Selp
            = خالی_سوچک;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = خالی_سوچک;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = خالی_سوچک;  // Last node of variable's name format
        Ast_گرہ** m_پہلا_عبارت_Inside_اشاریہ_Pointerp
            = خالی_سوچک;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    عام:
        فراغ جوڑو_Var_Name_Part(AstSFormatF* ثابت nodep, std::string&& name) {
            إذا (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } وإلا {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.داخل_کرو(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            إذا (name == "") {
                إذا (*m_پہلا_عبارت_Inside_اشاریہ_Pointerp) {
                    (*m_پہلا_عبارت_Inside_اشاریہ_Pointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_پہلا_عبارت_Inside_اشاریہ_Pointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_پہلا_عبارت_Inside_اشاریہ_Pointerp = nodep;
            }
        }

        AstMemberSel* access_درخت() { أرجع m_چوٹی_Nested_Array_Member_Selp; }
        ثابت std::string& smtName() { أرجع m_smtName; }

        Ast_Stmt_اظہاریہ* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ثابت methodp = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(genp->user2p(), گرہ_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtype_سیٹ_Void();

            methodp->جوڑو_Pinsp(m_چوٹی_Nested_Array_Member_Selp);  // Variable

            ثابت Ast_گرہ_DType* ثابت dtypep = varp->dtypep();  //چوڑائی
            ثابت ناپ_t چوڑائی = m_چوٹی_Nested_Array_Member_Selp->varp()->dtypep()->چوڑائی();
            methodp->جوڑو_Pinsp(جديد AstConst{dtypep->fileline(), AstConst::Unsized64{}, چوڑائی});

            methodp->جوڑو_Pinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = خالی_سوچک;

            methodp->جوڑو_Pinsp(
                جديد AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_چوٹی_Nested_Array_Member_Selp = خالی_سوچک;
            أرجع methodp->makeStmt();
        }

        Ast_گرہ_اظہاریہ* cloneName() { أرجع m_nestedNameFormatTopp->clone_درخت(خطأ); }

        فراغ error() {
            VL_DO_DANGLING(m_چوٹی_Nested_Array_Member_Selp->delete_درخت(), m_چوٹی_Nested_Array_Member_Selp);
        }

        Nested_Access_رستہ(AstMemberSel* چوٹی_Member_Selp, Ast_گرہ** اشارہ_گر_منزل_پہلا_Exprp)
            : m_چوٹی_Nested_Array_Member_Selp{چوٹی_Member_Selp}
            , m_پہلا_عبارت_Inside_اشاریہ_Pointerp{اشارہ_گر_منزل_پہلا_Exprp} {}
        ~Nested_Access_رستہ() {
            إذا (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_درخت(),
                            m_nestedNameFormatTopp = خالی_سوچک);
                m_nestedNameFormatp = خالی_سوچک;
            }
        }
    };
    Nested_Access_رستہ* m_nestedAccess = خالی_سوچک;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_گرہ_اظہاریہ* createEnumConstraint(File_سطر* ثابت fl, ثابت std::string& smtName,
                                      AstEnumDType* ثابت enumDtp, AstVar* ثابت genVarp) {
        Ast_گرہ_Module* ثابت genModp = VN_AS(genVarp->user2p(), گرہ_Module);
        ثابت صحيح چوڑائی = enumDtp->چوڑائی();
        std::string constraint = "(__Vbv (or";
        لكل (Ast_Enum_شے* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شے)) {
            ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvt_منزل_Str(vconstp->منزل_UInt()) + " "
                          + cvt_منزل_Str(چوڑائی) + "))";
        }
        constraint += "))";
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + constraint + "\""}};
        callp->dtype_سیٹ_Void();
        أرجع callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* لبھو_Static_Rand_منوال_Var_Member(AstClass* classp) ثابت {
        طالما (سچ) {
            إذا (AstVar* ثابت varp
                = VN_CAST(m_memberMap.لبھو_Member(classp, "__Vstaticrandmode"), Var)) {
                أرجع varp;
            }
            AstClassExtends* ثابت extendsp = classp->extendsp();
            إذا (!extendsp) أرجع خالی_سوچک;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    فراغ markConstrainedRandMember(AstStructDType* ثابت structp, ثابت std::string& name) {
        AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.لبھو_Member(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(سچ);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    منطقي is_Rand_Qualified_رستہ(ثابت Ast_گرہ_اظہاریہ* nodep) {
        طالما (nodep) {
            إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
                إذا (AstStructDType* ثابت structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ثابت AstMemberDType* ثابت memberp
                        = VN_CAST(m_memberMap.لبھو_Member(structp, selp->name()), MemberDType);
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
    ساكن فراغ markStructConstrainedRandRecurse(Ast_گرہ_DType* ثابت dtypep) {
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
    std::string build_Member_رستہ(ثابت AstMemberSel* ثابت memberSelp) {
        ثابت Ast_گرہ* fromp = memberSelp->fromp();
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            أرجع refp->name() + "." + memberSelp->name();
        } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            أرجع build_Member_رستہ(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        أرجع "";
    }

    ساكن Ast_گرہ_اظہاریہ* getFromp(ثابت Ast_گرہ_اظہاریہ* ثابت nodep) {
        إذا (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            أرجع cmethp->fromp();
        }
        أرجع getSelFromp(nodep);
    }

    ساكن std::string کڈھو_Solve_Before_ادھار_Name(ثابت Ast_گرہ_اظہاریہ* exprp) {
        طالما (ثابت Ast_گرہ_اظہاریہ* ثابت fromp = getFromp(exprp)) exprp = fromp;
        إذا (ثابت AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) أرجع vrefp->name();
        أرجع "";
    }

    ساكن فراغ build_Name_اگیتر(AstCExpr* exprp, ثابت Ast_گرہ_اظہاریہ* ثابت nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            exprp->جوڑو("(\""s + refp->name());
            أرجع;
        }

        ثابت Ast_گرہ_اظہاریہ* ثابت fromp = getFromp(nodep);
        إذا (fromp) build_Name_اگیتر(exprp, fromp);

        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            exprp->جوڑو("." + selp->name());
        } وإلا إذا (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            أرجع;
        } وإلا إذا (ثابت AstCMethodHard* ثابت cmethp = VN_CAST(nodep, CMethodHard)) {
            إذا (cmethp->طریقہ() == VCMethod::ARRAY_AT) {
                Ast_گرہ_اظہاریہ* ثابت argp = cmethp->pinsp();
                exprp->جوڑو(".\" + ");
                AstCFuncHard* ثابت solverHexFuncp
                    = جديد AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
                solverHexFuncp->جوڑو_Pinsp(argp->clone_درخت_خالص(خطأ));
                solverHexFuncp->dtype_سیٹ_String();
                exprp->جوڑو(solverHexFuncp);
                exprp->جوڑو(" + \"");
            } وإلا {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                أرجع;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ساكن AstCExpr* build_رستہ_اظہاریہ(File_سطر* ثابت fl, ثابت Ast_گرہ_اظہاریہ* nodep,
                                   ثابت std::string selName) {
        AstCExpr* ثابت exprp = جديد AstCExpr{fl, ""};
        build_Name_اگیتر(exprp, nodep);
        إذا (selName != "") {
            exprp->جوڑو(".");
            exprp->جوڑو(selName);
        }
        exprp->جوڑو("\")");
        أرجع exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_اظہاریہ(File_سطر* fl, ثابت std::string& ادھار_Name,
                                    ثابت Ast_گرہ_Sel* selp) {
        AstCExpr* ثابت p = جديد AstCExpr{fl, ""};
        p->جوڑو("(\""s + ادھار_Name + ".\"");
        p->جوڑو("+");
        AstCFuncHard* ثابت solverHexFuncp
            = جديد AstCFuncHard{fl, VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
        solverHexFuncp->جوڑو_Pinsp(selp->bitp()->clone_درخت_خالص(خطأ));
        solverHexFuncp->dtype_سیٹ_String();
        p->جوڑو(solverHexFuncp);
        p->جوڑو(")");
        p->dtype_سیٹ_String();
        أرجع p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ساكن Ast_گرہ_اظہاریہ* getSelFromp(ثابت Ast_گرہ_اظہاریہ* exprp) {
        إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(exprp, MemberSel)) أرجع mp->fromp();
        إذا (ثابت AstStructSel* ثابت sp = VN_CAST(exprp, StructSel)) أرجع sp->fromp();
        أرجع خالی_سوچک;
    }

    Ast_گرہ_اظہاریہ* build_Solve_Before_Name_اظہاریہ(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp) {
        إذا (ثابت AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ثابت p = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + varrefp->name() + "\"s"};
            p->dtype_سیٹ_String();
            أرجع p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        إذا (Ast_گرہ_اظہاریہ* ثابت selFromp = getSelFromp(exprp)) {
            ثابت std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            إذا (ثابت AstArraySel* ثابت arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string ادھار_Name;
                إذا (ثابت AstVarRef* ثابت vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    ادھار_Name = vp->name();
                } وإلا إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    ادھار_Name = build_Member_رستہ(mp);
                }
                إذا (ادھار_Name.خالی_تھاواں()) أرجع خالی_سوچک;
                AstCExpr* ثابت p = جديد AstCExpr{fl, ""};
                p->جوڑو("(\""s + ادھار_Name + ".\"");
                p->جوڑو("+");
                AstCFuncHard* ثابت solverHexFuncp
                    = جديد AstCFuncHard{fl, VCFunction::RANDOMIZER_منزل_SOLVER_HEX};
                solverHexFuncp->جوڑو_Pinsp(arrSelp->bitp()->clone_درخت_خالص(خطأ));
                solverHexFuncp->dtype_سیٹ_String();
                p->جوڑو(solverHexFuncp);
                p->جوڑو(")");
                p->dtype_سیٹ_String();
                أرجع p;
            }
            // Static member path (obj.field)
            إذا (ثابت AstVarRef* ثابت vp = VN_CAST(selFromp, VarRef)) {
                ثابت std::string رستہ = vp->name() + "." + selName;
                AstCExpr* ثابت p = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + رستہ + "\"s"};
                p->dtype_سیٹ_String();
                أرجع p;
            }
            إذا (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ثابت p = build_رستہ_اظہاریہ(fl, selFromp, selName);
                p->dtype_سیٹ_String();
                أرجع p;
            }
            أرجع خالی_سوچک;
        }
        إذا (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ثابت p = build_رستہ_اظہاریہ(fl, exprp, "");
            p->dtype_سیٹ_String();
            أرجع p;
        }
        ثابت Ast_گرہ_Sel* selp = VN_CAST(exprp, ArraySel);
        إذا (!selp) selp = VN_CAST(exprp, AssocSel);
        إذا (selp) {
            // arr[i] -> dynamic name
            std::string ادھار_Name;
            إذا (ثابت AstVarRef* ثابت vp = VN_CAST(selp->fromp(), VarRef)) {
                ادھار_Name = vp->name();
            } وإلا إذا (ثابت AstMemberSel* ثابت mp = VN_CAST(selp->fromp(), MemberSel)) {
                ادھار_Name = build_Member_رستہ(mp);
            }
            إذا (ادھار_Name.خالی_تھاواں()) أرجع خالی_سوچک;
            أرجع build_Array_Sel_Name_اظہاریہ(fl, ادھار_Name, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        إذا (ثابت AstSel* ثابت ثنائی_ہندسہ_Selp = VN_CAST(exprp, Sel)) {
            أرجع build_Solve_Before_Name_اظہاریہ(fl, ثنائی_ہندسہ_Selp->fromp());
        }
        أرجع خالی_سوچک;
    }

    AstSFormatF* getConstFormat(Ast_گرہ_اظہاریہ* nodep) {
        أرجع جديد AstSFormatF{nodep->fileline(), (nodep->چوڑائی() & 3) ? "#b%b" : "#x%x", خطأ,
                               nodep};
    }
    منطقي editFormat(Ast_گرہ_اظہاریہ* nodep) {
        إذا (nodep->user1()) أرجع خطأ;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlink_Fr_پٹھ(&handle);
        handle.relink(getConstFormat(nodep));
        أرجع سچ;
    }
    Ast_گرہ_اظہاریہ* wrapWithCond(Ast_گرہ_اظہاریہ* ثابت exprp, Ast_گرہ_اظہاریہ* ثابت condp,
                              ثابت صحيح چوڑائی) {
        إذا (condp) {
            File_سطر* ثابت flp = exprp->fileline();
            أرجع جديد AstCond{
                flp, condp, exprp,
                getConstFormat(جديد AstConst{flp, AstConst::Widthed_قدر{}, چوڑائی, 0})};
        }
        أرجع exprp;
    }
    AstSFormatF* editSMT(Ast_گرہ_اظہاریہ* nodep, Ast_گرہ_اظہاریہ* lhsp = خالی_سوچک,
                         Ast_گرہ_اظہاریہ* rhsp = خالی_سوچک, Ast_گرہ_اظہاریہ* thsp = خالی_سوچک) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_اظہاریہ = nodep->emitSMT();  // Might need child width (AstExtend)
        إذا (smt_اظہاریہ == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            أرجع خالی_سوچک;
        }

        ثابت صحيح lhs_چوڑائی = lhsp ? lhsp->چوڑائی() : 0;
        ثابت صحيح rhs_چوڑائی = rhsp ? rhsp->چوڑائی() : 0;
        ثابت صحيح ths_چوڑائی = thsp ? thsp->چوڑائی() : 0;
        Ast_گرہ_اظہاریہ* lhsCondp = خالی_سوچک;
        Ast_گرہ_اظہاریہ* rhsCondp = خالی_سوچک;
        Ast_گرہ_اظہاریہ* thsCondp = خالی_سوچک;

        إذا (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = خالی_سوچک;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlink_Fr_پٹھ() : lhsp),
                         گرہ_اظہاریہ);
            lhsCondp = m_conditionp;
        }
        إذا (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = خالی_سوچک;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlink_Fr_پٹھ() : rhsp),
                         گرہ_اظہاریہ);
            rhsCondp = m_conditionp;
        }
        إذا (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = خالی_سوچک;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlink_Fr_پٹھ() : thsp),
                         گرہ_اظہاریہ);
            thsCondp = m_conditionp;
        }

        Ast_گرہ_اظہاریہ* argsp = خالی_سوچک;
        لكل (string::iterator pos = smt_اظہاریہ.begin(); pos != smt_اظہاریہ.سرا(); ++pos) {
            إذا (pos[0] == '%') {
                ++pos;
                بدل (pos[0]) {
                حالة '%': اكسر;
                حالة 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, wrapWithCond(lhsp, lhsCondp, lhs_چوڑائی));
                    lhsp = خالی_سوچک;
                    اكسر;
                حالة 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, wrapWithCond(rhsp, rhsCondp, rhs_چوڑائی));
                    rhsp = خالی_سوچک;
                    اكسر;
                حالة 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_گرہ::جوڑو_اگلا(argsp, wrapWithCond(thsp, thsCondp, ths_چوڑائی));
                    thsp = خالی_سوچک;
                    اكسر;
                افتراضي: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); اكسر;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ثابت newp = جديد AstSFormatF{nodep->fileline(), smt_اظہاریہ, خطأ, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        أرجع newp;
    }

    Ast_گرہ_اظہاریہ* editSingle(File_سطر* fl, Ast_گرہ* itemsp) {
        إذا (!itemsp) أرجع خالی_سوچک;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = سچ;

        {
            AstBegin* ثابت tempp
                = جديد AstBegin{fl, "[EditWrapper]", itemsp->unlink_Fr_پٹھ_With_اگلا(), خطأ};
            VL_DO_DANGLING(iterate_And_اگلا_Null(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            إذا (itemsp) itemsp->unlink_Fr_پٹھ_With_اگلا();
            VL_DO_DANGLING(tempp->delete_درخت(), tempp);
        }
        إذا (!itemsp) أرجع خالی_سوچک;

        Ast_گرہ_اظہاریہ* ثابت exprsp = VN_CAST(itemsp, گرہ_اظہاریہ);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        إذا (!exprsp->nextp()) أرجع exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        لكل (Ast_گرہ* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        أرجع جديد AstSFormatF{fl, fmt.str(), خطأ, exprsp};
    }

    Ast_گرہ_اظہاریہ* newSel(File_سطر* fl, Ast_گرہ_اظہاریہ* arrayp, Ast_گرہ_اظہاریہ* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_گرہ_DType* ثابت arrDtp = arrayp->unlink_Fr_پٹھ()->dtypep()->skipRefp();
        Ast_گرہ_اظہاریہ* selp = خالی_سوچک;
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
    Ast_گرہ_اظہاریہ* adjust_چوڑائی(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, صحيح نشانہ_چوڑائی,
                             VSigning نشانہ_Signing) {
        ثابت صحيح اظہاریہ_چوڑائی = exprp->چوڑائی();
        إذا (نشانہ_چوڑائی > اظہاریہ_چوڑائی) {
            // Extend to match target width
            Ast_گرہ_اظہاریہ* ثابت نتیجہ = جديد AstExtend{fl, exprp, نشانہ_چوڑائی};
            نتیجہ->dtypeChgSigned(نشانہ_Signing);
            أرجع نتیجہ;
        } وإلا إذا (نشانہ_چوڑائی < اظہاریہ_چوڑائی) {
            // Truncate to match target width
            Ast_گرہ_اظہاریہ* ثابت نتیجہ = جديد AstSel{fl, exprp, 0, نشانہ_چوڑائی};
            نتیجہ->dtypeChgSigned(نشانہ_Signing);
            أرجع نتیجہ;
        } وإلا {
            // Width already matches
            أرجع exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_گرہ_اظہاریہ* کڈھو_Return_اظہاریہ(AstFunc* funcp) {
        AstVar* ثابت retVarp = VN_CAST(funcp->fvarp(), Var);
        إذا (!retVarp) أرجع خالی_سوچک;
        Ast_گرہ_اظہاریہ* retExprp = خالی_سوچک;
        صحيح سونپو_گنتی = 0;
        funcp->foreach([&](Ast_سونپو* assignp) {
            AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
            إذا (!lhsp || lhsp->varp() != retVarp) أرجع;
            retExprp = assignp->rhsp();
            ++سونپو_گنتی;
        });
        // Simple function: initializer + return (2) or just return (1)
        إذا (سونپو_گنتی > 2 || !retExprp) أرجع خالی_سوچک;
        أرجع retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    فراغ propagateUser1InlineRecurse(Ast_گرہ_اظہاریہ* nodep) {
        إذا (VN_IS(nodep, گرہ_Var_Ref)) {
            nodep->user1(VN_AS(nodep, گرہ_Var_Ref)->varp()->rand().isRandomizable());
            أرجع;
        }
        إذا (VN_IS(nodep, Const)) {
            nodep->user1(خطأ);
            أرجع;
        }
        صحيح any_بچہ = خطأ;  // Used as bool
        إذا (Ast_گرہ_اظہاریہ* ثابت cp = VN_CAST(nodep->op1p(), گرہ_اظہاریہ)) {
            propagateUser1InlineRecurse(cp);
            any_بچہ |= cp->user1();
        }
        إذا (Ast_گرہ_اظہاریہ* ثابت cp = VN_CAST(nodep->op2p(), گرہ_اظہاریہ)) {
            propagateUser1InlineRecurse(cp);
            any_بچہ |= cp->user1();
        }
        إذا (Ast_گرہ_اظہاریہ* ثابت cp = VN_CAST(nodep->op3p(), گرہ_اظہاریہ)) {
            propagateUser1InlineRecurse(cp);
            any_بچہ |= cp->user1();
        }
        إذا (Ast_گرہ_اظہاریہ* ثابت cp = VN_CAST(nodep->op4p(), گرہ_اظہاریہ)) {
            propagateUser1InlineRecurse(cp);
            any_بچہ |= cp->user1();
        }
        nodep->user1(any_بچہ);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ساكن Ast_گرہ* prependDistPreamble(AstConstraintForeach* nodep, Ast_گرہ* bodyp) {
        Ast_گرہ* ثابت preamblep = nodep->user3p();
        إذا (!preamblep) أرجع bodyp;
        nodep->user3p(خالی_سوچک);
        preamblep->جوڑو_اگلا(bodyp);
        أرجع preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    منطقي referencesForeachIdx(ثابت Ast_گرہ* nodep) ثابت {
        إذا (m_foreachIdxVars.خالی_تھاواں()) أرجع خطأ;
        أرجع nodep->exists(
            [&](ثابت AstVarRef* refp) { أرجع m_foreachIdxVars.گنتی(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    فراغ جوڑو_Registration_Stmt(Ast_گرہ* ثابت stmtp, Ast_گرہ_FTask* ثابت initTaskp) {
        إذا (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->جوڑو_Bodyp(stmtp);
        } وإلا {
            initTaskp->جوڑو_Stmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_سطر* ثابت fl, Ast_گرہ_اظہاریہ* ثابت arrExprp,
                                       Ast_گرہ_اظہاریہ* ثابت idxExprp) {
        Ast_گرہ_اظہاریہ* ثابت argsp = Ast_گرہ::جوڑو_اگلا(arrExprp, idxExprp);
        AstSFormatF* ثابت formatp = جديد AstSFormatF{fl, "(select %s %s)", خطأ, argsp};
        أرجع formatp;
    }

    فراغ سیٹ_Rand_منوال(AstVar* ثابت varp, AstMemberSel* ثابت memberselp,
                     ثابت std::string& smtName, ثابت Randomize_منوال& rand_منوال,
                     Ast_گرہ_FTask* ثابت initTaskp) {
        Ast_گرہ_Module* ثابت varClassp = VN_AS(varp->user2p(), گرہ_Module);
        AstVar* ثابت sub_Rand_منوال_Varp = get_Rand_منوال_Var_From_Class(varClassp);
        إذا (sub_Rand_منوال_Varp) {
            Ast_گرہ_اظہاریہ* ثابت ماپا_Accessp = memberselp->fromp()->clone_درخت(خطأ);
            AstMemberSel* ثابت rand_منوال_Selp
                = جديد AstMemberSel{varp->fileline(), ماپا_Accessp, sub_Rand_منوال_Varp};
            rand_منوال_Selp->dtypep(sub_Rand_منوال_Varp->dtypep());
            AstCMethodHard* ثابت atp
                = جديد AstCMethodHard{varp->fileline(), rand_منوال_Selp, VCMethod::ARRAY_AT,
                                     جديد AstConst{varp->fileline(), rand_منوال.اشاریہ}};
            atp->dtype_سیٹ_UInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ثابت enablep = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گرہ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtype_سیٹ_Void();
            Ast_گرہ_اظہاریہ* ثابت nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : جديد AstSFormatF{varp->fileline(), smtName, خطأ, خالی_سوچک};
            nameExprp->dtypep(varp->dtypep());
            enablep->جوڑو_Pinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ثابت disablep = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گرہ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_سیٹ_VAR_DISABLED};
            disablep->dtype_سیٹ_Void();
            Ast_گرہ_اظہاریہ* ثابت disnp = nameExprp->clone_درخت(خطأ);
            disablep->جوڑو_Pinsp(disnp);
            AstIf* ثابت ifp
                = جديد AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            جوڑو_Registration_Stmt(ifp, initTaskp);
        }
    }

    فراغ markRandc(AstVar* ثابت varp, ثابت std::string& smtName, Ast_گرہ_FTask* ثابت initTaskp) {
        AstCMethodHard* ثابت markp = جديد AstCMethodHard{
            varp->fileline(),
            جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گرہ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtype_سیٹ_Void();
        Ast_گرہ_اظہاریہ* ثابت nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : جديد AstSFormatF{varp->fileline(), smtName, خطأ, خالی_سوچک};
        nameExprp->dtypep(varp->dtypep());
        markp->جوڑو_Pinsp(nameExprp);
        جوڑو_Registration_Stmt(markp->makeStmt(), initTaskp);
    }

    Ast_گرہ_Module* getLeftmostVarModulep(AstMemberSel* ثابت memberselp,
                                         AstVar* ثابت varp) ثابت {
        إذا (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_گرہ* بایاں_Most_Nodep = memberselp->fromp();
            طالما (AstMemberSel* ثابت nestedMemberSelp = VN_CAST(بایاں_Most_Nodep, MemberSel)) {
                بایاں_Most_Nodep = nestedMemberSelp->fromp();
            }
            إذا (Ast_گرہ_Var_Ref* ثابت جڑ_Var_Refp = VN_CAST(بایاں_Most_Nodep, گرہ_Var_Ref)) {
                أرجع VN_AS(جڑ_Var_Refp->varp()->user2p(), گرہ_Module);
            }
            أرجع VN_AS(memberselp->user2p(), گرہ_Module);
        }
        أرجع VN_AS(varp->user2p(), گرہ_Module);
    }

    Ast_گرہ_FTask* getInitTaskp(AstVar* ثابت varp, ثابت منطقي memberselOrStructSel,
                               Ast_گرہ_Module* ثابت classp) ثابت {
        إذا (m_inlineInitTaskp) أرجع m_inlineInitTaskp;
        varp->user3(سچ);
        إذا (memberselOrStructSel) {
            إذا (Ast_گرہ_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.لبھو_Member(classp, "randomize"), گرہ_FTask)) {
                أرجع initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            إذا (m_memberselInitTaskp) أرجع m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } وإلا {
            Ast_گرہ_FTask* ثابت initTaskp
                = VN_AS(m_memberMap.لبھو_Member(classp, "new"), گرہ_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            أرجع initTaskp;
        }
        أرجع خالی_سوچک;
    }

    فراغ createWriteVarForClassFields(AstClass* ثابت elemClassp, ثابت std::string& smtName,
                                      AstVar* ثابت iterVarp, ثابت منطقي isUnpackedClassRefArray,
                                      Ast_گرہ_Module* ثابت classOrPackagep, AstVar* ثابت varp,
                                      AstIf* ثابت ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ثابت {
        File_سطر* ثابت fl = varp->fileline();
        Ast_گرہ_Module* ثابت varClassp = VN_AS(varp->user2p(), گرہ_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr صحيح idx_چوڑائی = 32;
        ثابت صحيح fmt_چوڑائی = VL_الفاظ_I(idx_چوڑائی) * 8;

        لكل (ثابت AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : خالی_سوچک) {
            لكل (Ast_گرہ* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ثابت memberVarp = VN_CAST(mnodep, Var);
                إذا (!memberVarp || !memberVarp->rand().isRandomizable()) استمر;
                Ast_گرہ_DType* ثابت memberDtp = memberVarp->dtypep()->skipRefp();
                إذا (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) استمر;
                ثابت صحيح member_چوڑائی = memberDtp->چوڑائی();

                AstCStmt* ثابت fmtp = جديد AstCStmt{fl};
                إذا (isUnpackedClassRefArray) {
                    fmtp->جوڑو("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } وإلا {
                    fmtp->جوڑو("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::منزل_string(fmt_چوڑائی) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->جوڑو(جديد AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->جوڑو(");\n");
                ifNonNullp->جوڑو_Thensp(fmtp);

                AstVarRef* ثابت arrayWrRefp = جديد AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_گرہ_اظہاریہ* atWritep;
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
                                         جديد AstVarRef{fl, VN_AS(m_genp->user2p(), گرہ_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtype_سیٹ_Void();
                writeVarp->جوڑو_Pinsp(memberSelp);
                writeVarp->جوڑو_Pinsp(
                    جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(member_چوڑائی)});
                AstCExpr* ثابت nameRefp = جديد AstCExpr{fl, AstCExpr::خالص{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->جوڑو_Pinsp(nameRefp);
                writeVarp->جوڑو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->جوڑو_Thensp(writeVarp->makeStmt());
            }
        }
    }

    فراغ createSolverArrayHandle(AstVar* ثابت varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_گرہ_Module* ثابت classOrPackagep,
                                 ثابت منطقي isUnpackedClassRefArray,
                                 ثابت std::string& smtName) ثابت {
        File_سطر* ثابت fl = varp->fileline();
        AstClass* ثابت elemClassp = elemClassRefDtp->classp();
        Ast_گرہ_Module* ثابت varClassp = VN_AS(varp->user2p(), گرہ_Module);

        AstVar* ثابت iterVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, m_unique_ناں.get("__Vi"),
                                            varp->لبھو_UInt32_DType()};
        iterVarp->funcLocal(سچ);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_گرہ* ثابت stmtsp
            = جديد Ast_سونپو{fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE}, جديد AstConst{fl, 0}};

        Ast_گرہ_اظہاریہ* sizep;
        إذا (isUnpackedClassRefArray) {
            ثابت صحيح array_ناپ_2
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = جديد AstConst{fl, static_cast<uint32_t>(array_ناپ_2)};
        } وإلا {
            AstVarRef* ثابت array_ناپ_Refp = جديد AstVarRef{fl, varClassp, varp, VAccess::READ};
            array_ناپ_Refp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ثابت dynSizep
                = جديد AstCMethodHard{fl, array_ناپ_Refp, VCMethod::DYN_ناپ, خالی_سوچک};
            dynSizep->dtype_سیٹ_UInt32();
            sizep = dynSizep;
        }

        AstLoop* ثابت loopp = جديد AstLoop{fl};
        stmtsp->جوڑو_اگلا(loopp);
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{
            fl, loopp, جديد AstLt{fl, جديد AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ثابت arrayReadRefp = جديد AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_گرہ_اظہاریہ* atReadp;
        إذا (isUnpackedClassRefArray) {
            atReadp
                = جديد AstArraySel{fl, arrayReadRefp, جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        } وإلا {
            atReadp = جديد AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ثابت ifNonNullp
            = جديد AstIf{fl, جديد AstNeq{fl, atReadp, جديد AstConst{fl, AstConst::Null{}}}, خالی_سوچک};
        loopp->جوڑو_Stmtsp(ifNonNullp);

        AstCStmt* ثابت bufDeclp = جديد AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->جوڑو_Thensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->جوڑو_Stmtsp(جديد Ast_سونپو{
            fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
            جديد Ast_جوڑو{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(سچ);
        Ast_گرہ_FTask* initTaskp = m_inlineInitTaskp;
        إذا (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.لبھو_Member(varClassp, "new"), گرہ_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->جوڑو_Stmtsp(stmtsp);
    }

    فراغ createSolverVarHandle(AstVar* ثابت varp, ثابت منطقي structSelOrCMeth,
                               ثابت منطقي isGlobalConstrained, ثابت Randomize_منوال rand_منوال,
                               AstMemberSel* ثابت memberselp, ثابت std::string& smtName,
                               Ast_گرہ_Module* ثابت classOrPackagep, Ast_گرہ_Module* ثابت classp,
                               Ast_گرہ_FTask* ثابت initTaskp) {
        uint32_t unpackedDims = 0;
        إذا (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(خطأ).دوجا;
        }
        إذا (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ثابت methodp = جديد AstCMethodHard{
            varp->fileline(),
            جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گرہ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_سیٹ_Void();
        إذا (memberselp) {
            methodp->جوڑو_Pinsp(memberselp);
        } وإلا {
            AstVarRef* ثابت varRefp
                = جديد AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->جوڑو_Pinsp(varRefp);
        }
        ثابت ناپ_t چوڑائی = arrayElementDTypep(varp->dtypep())->چوڑائی();
        methodp->جوڑو_Pinsp(جديد AstConst{varp->fileline(), AstConst::Unsized64{}, چوڑائی});
        Ast_گرہ_اظہاریہ* ثابت varnamep = جديد AstCExpr{varp->fileline(), AstCExpr::خالص{},
                                                   "\"" + smtName + "\"", varp->چوڑائی()};
        varnamep->dtypep(varp->dtypep());
        methodp->جوڑو_Pinsp(varnamep);
        methodp->جوڑو_Pinsp(
            جديد AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        إذا (rand_منوال.ورتدا_منوال && !(isGlobalConstrained && memberselp)) {
            methodp->جوڑو_Pinsp(
                جديد AstConst{varp->fileline(), AstConst::Unsized64{}, rand_منوال.اشاریہ});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->جوڑو_Stmtsp(methodp->makeStmt());
        إذا (varp->lifetime().isStatic() && rand_منوال.ورتدا_منوال) {
            AstCMethodHard* ثابت markp = جديد AstCMethodHard{
                varp->fileline(),
                جديد AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), گرہ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_گرہ_اظہاریہ* ثابت namep = جديد AstCExpr{varp->fileline(), AstCExpr::خالص{},
                                                    "\"" + smtName + "\"", varp->چوڑائی()};
            namep->dtypep(varp->dtypep());
            markp->جوڑو_Pinsp(namep);
            markp->dtype_سیٹ_Void();
            initTaskp->جوڑو_Stmtsp(markp->makeStmt());
        }
        إذا (isGlobalConstrained && memberselp && rand_منوال.ورتدا_منوال) {
            سیٹ_Rand_منوال(varp, memberselp, smtName, rand_منوال, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        إذا (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_گرہ_اظہاریہ* createVarExprp(Ast_گرہ_Var_Ref* nodep, ثابت Randomize_منوال rand_منوال,
                                ثابت std::string& smtName, ثابت منطقي isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ثابت varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_گرہ_اظہاریہ* exprp = جديد AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, خطأ, خالی_سوچک};
        إذا (!rand_منوال.ورتدا_منوال) {
            // Global constraints keep nodep alive for write_var processing
            إذا (!isGlobalConstrained) VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_گرہ_اظہاریہ* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_درخت(خطأ)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_گرہ_اظہاریہ* rand_منوال_Access;
        ثابت منطقي varIsStatic = varp->lifetime().isStatic();
        AstClass* ثابت varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : خالی_سوچک;
        AstVar* ثابت owner_Static_Rand_منوال_Varp
            = varOwningClassp ? لبھو_Static_Rand_منوال_Var_Member(varOwningClassp) : خالی_سوچک;
        إذا (owner_Static_Rand_منوال_Varp) {
            rand_منوال_Access = جديد AstVarRef{varp->fileline(),
                                           VN_AS(owner_Static_Rand_منوال_Varp->user2p(), گرہ_Module),
                                           owner_Static_Rand_منوال_Varp, VAccess::READ};
        } وإلا إذا (memberselp) {
            Ast_گرہ_Module* ثابت varClassp = VN_AS(varp->user2p(), گرہ_Module);
            AstVar* ثابت effective_Rand_منوال_Varp = get_Rand_منوال_Var_From_Class(varClassp);
            إذا (effective_Rand_منوال_Varp) {
                // Member's class has randmode, use it
                Ast_گرہ_اظہاریہ* ماپا_Access = memberselp->fromp()->clone_درخت(خطأ);
                AstMemberSel* rand_منوال_Sel
                    = جديد AstMemberSel{varp->fileline(), ماپا_Access, effective_Rand_منوال_Varp};
                rand_منوال_Sel->dtypep(effective_Rand_منوال_Varp->dtypep());
                rand_منوال_Access = rand_منوال_Sel;
            } وإلا {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_rand_منوال_Varp, nodep, "No m_randModeVarp");
                rand_منوال_Access
                    = جديد AstVarRef{varp->fileline(), VN_AS(m_rand_منوال_Varp->user2p(), گرہ_Module),
                                    m_rand_منوال_Varp, VAccess::READ};
            }
        } وإلا {
            UASSERT_OBJ(m_rand_منوال_Varp, nodep, "No m_randModeVarp");
            rand_منوال_Access
                = جديد AstVarRef{varp->fileline(), VN_AS(m_rand_منوال_Varp->user2p(), گرہ_Module),
                                m_rand_منوال_Varp, VAccess::READ};
        }

        AstCMethodHard* ثابت atp
            = جديد AstCMethodHard{nodep->fileline(), rand_منوال_Access, VCMethod::ARRAY_AT,
                                 جديد AstConst{nodep->fileline(), rand_منوال.اشاریہ}};
        atp->dtype_سیٹ_UInt32();
        exprp = جديد AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(سچ);  // Mark as formatted
        أرجع exprp;
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        إذا (varp->user4p()) {
            منطقي is_ناپ_Ref = خطأ;
            إذا (AstCMethodHard* ثابت methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                إذا (methodp->طریقہ() == VCMethod::ASSOC_ناپ
                    || methodp->طریقہ() == VCMethod::DYN_ناپ) {
                    is_ناپ_Ref = سچ;
                }
            }
            إذا (!is_ناپ_Ref && m_ناپ_Constrained_Arraysp) { m_ناپ_Constrained_Arraysp->داخل_کرو(varp); }
        }

        // Check if this variable is marked as globally constrained
        ثابت منطقي isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = خالی_سوچک;
        منطقي structSelOrCMeth = خطأ;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_گرہ_DType* selectedDtypep = خالی_سوچک;
        std::string smtName;
        إذا (m_nestedAccess) {
            m_nestedAccess->جوڑو_Var_Name_Part(
                جديد AstSFormatF{varp->fileline(), nodep->name(), خطأ, خالی_سوچک}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_درخت();
        } وإلا إذا (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_گرہ* چوٹی_Member_Sel = nodep->backp();
            طالما (VN_IS(چوٹی_Member_Sel->backp(), MemberSel)) {
                چوٹی_Member_Sel = چوٹی_Member_Sel->backp();
            }
            memberselp = VN_AS(چوٹی_Member_Sel, MemberSel)->clone_درخت(خطأ);
            smtName = build_Member_رستہ(memberselp);
        } وإلا إذا (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_گرہ* چوٹی_Nodep = nodep->backp();
            طالما (VN_IS(چوٹی_Nodep->backp(), StructSel) || VN_IS(چوٹی_Nodep->backp(), CMethodHard)) {
                چوٹی_Nodep = چوٹی_Nodep->backp();
            }

            إذا (VN_IS(چوٹی_Nodep, StructSel) || VN_IS(چوٹی_Nodep, CMethodHard)) {
                structSelOrCMeth = سچ;
                selectedDtypep = VN_AS(چوٹی_Nodep, گرہ_اظہاریہ)->dtypep();
            }

            memberselp = VN_CAST(چوٹی_Nodep, MemberSel);
            إذا (memberselp) smtName = build_Member_رستہ(memberselp);

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
        إذا (dtype_شامل_اے_Real(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            أرجع;
        }
        Ast_گرہ_Module* ثابت classOrPackagep = nodep->classOrPackagep();
        ثابت Randomize_منوال rand_منوال = {.asUQuad = varp->user1()};
        إذا (!rand_منوال.ورتدا_منوال && editFormat(nodep)) أرجع;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (enumDtp
            && (std::لبھو(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.سرا(), varp)
                == m_rangeConstrainedEnums.سرا())) {
            AstVar* genVarp = m_genp;
            إذا (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_گرہ_اظہاریہ* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_گرہ_FTask* نشانہ_Taskp = m_inlineInitTaskp;
            إذا (!نشانہ_Taskp) نشانہ_Taskp = get_Create_Constraint_تیاری_Func(m_classp, m_memberMap);
            UASSERT_OBJ(نشانہ_Taskp, nodep, "No function to inline enum range constraint into");
            نشانہ_Taskp->جوڑو_Stmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.ڈالو_پٹھ(varp);
        }

        VNRelinker relinker;
        nodep->unlink_Fr_پٹھ(&relinker);
        Ast_گرہ_اظہاریہ* ثابت exprp
            = createVarExprp(nodep, rand_منوال, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ثابت منطقي alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.گنتی(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.گنتی(smtName) > 0
                                    : memberselp          ? m_writtenVars.گنتی(smtName) > 0
                                                          : varp->user3();
        ثابت منطقي shouldWriteVar = !alreadyWritten;
        إذا (shouldWriteVar) {
            // Track this variable path as written
            إذا (m_inlineInitTaskp)
                m_inlineWrittenVars.داخل_کرو(smtName);
            وإلا إذا (isGlobalConstrained || memberselp)
                m_writtenVars.داخل_کرو(smtName);
            // For global constraints, delete nodep after processing
            إذا (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);

            منطقي isClassRefArray = خطأ;
            منطقي isUnpackedClassRefArray = خطأ;
            AstClassRefDType* elemClassRefDtp = خالی_سوچک;
            {
                Ast_گرہ_DType* varDtp = varp->dtypep()->skipRefp();
                إذا (varDtp->isNonPackedArray()) {
                    Ast_گرہ_DType* ثابت elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    إذا (elemClassRefDtp) {
                        isClassRefArray = سچ;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_گرہ_Module* ثابت classp = getLeftmostVarModulep(memberselp, varp);
            Ast_گرہ_FTask* ثابت initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            إذا (m_nestedAccess) {
                Ast_Stmt_اظہاریہ* ثابت writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                جوڑو_Registration_Stmt(writeVarStmtp, initTaskp);
                إذا (isGlobalConstrained && memberselp && rand_منوال.ورتدا_منوال) {
                    سیٹ_Rand_منوال(varp, memberselp, smtName, rand_منوال, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                إذا (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } وإلا إذا (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } وإلا {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, rand_منوال,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } وإلا {
            // Variable already written, clean up cloned memberselp if any
            إذا (memberselp) VL_DO_DANGLING(memberselp->delete_درخت(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            إذا (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
        إذا (m_nestedAccess) { VL_DO_DANGLING(مٹاؤ m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_گرہ_اظہاریہ* build_گنتی_Ones_Expansion(File_سطر* fl, Ast_گرہ_اظہاریہ* argp) {
        ثابت V3_نمبر عدد_One{fl, argp->چوڑائی(), 1};
        Ast_گرہ_اظہاریہ* sump = جديد AstAnd{fl, argp, جديد AstConst{fl, عدد_One}};
        sump->user1(سچ);
        لكل (صحيح i = 1; i < argp->چوڑائی(); i++) {
            V3_نمبر عدد_ثنائی_ہندسہ_نقاب{fl, argp->چوڑائی(), 0};
            عدد_ثنائی_ہندسہ_نقاب.سیٹ_ثنائی_ہندسہ(i, 1);
            AstAnd* ثابت andp
                = جديد AstAnd{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, عدد_ثنائی_ہندسہ_نقاب}};
            andp->user1(سچ);
            Ast_کھسکاؤ_R* ثابت shiftp
                = جديد Ast_کھسکاؤ_R{fl, andp,
                                جديد AstConst{fl, AstConst::Widthed_قدر{}, argp->چوڑائی(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(سچ);
            sump = جديد Ast_جوڑو{fl, sump, shiftp};
            sump->user1(سچ);
        }
        أرجع sump;
    }

    فراغ ویکھو(Ast_گنتی_Ones* nodep) override {
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت argp = nodep->lhsp()->unlink_Fr_پٹھ();
        Ast_گرہ_اظہاریہ* sump = build_گنتی_Ones_Expansion(fl, argp);
        sump = adjust_چوڑائی(fl, sump, nodep->چوڑائی(), nodep->dtypep()->numeric());
        sump->user1(سچ);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(sump);
    }
    فراغ ویکھو(AstRedXor* nodep) override {
        إذا (editFormat(nodep)) أرجع;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت argp = nodep->lhsp()->unlink_Fr_پٹھ();

        Ast_گرہ_اظہاریہ* redxorp = جديد AstSel{fl, argp, 0, 1};
        redxorp->user1(سچ);
        لكل (صحيح i = 1; i < argp->چوڑائی(); i++) {
            AstSel* ثابت selp = جديد AstSel{fl, argp->clone_درخت_خالص(خطأ), i, 1};
            selp->user1(سچ);

            redxorp = جديد AstXor{fl, redxorp, selp};
            redxorp->user1(سچ);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(redxorp);
    }
    فراغ ویکھو(AstRedAnd* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // Convert to (~x == 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت argp = nodep->lhsp()->unlink_Fr_پٹھ();
        ثابت V3_نمبر عدد_Zero{fl, argp->چوڑائی(), 0};
        Ast_گرہ_اظہاریہ* ثابت negp = جديد AstNot{fl, argp};
        negp->user1(سچ);
        Ast_گرہ_اظہاریہ* ثابت eqp = جديد AstEq{fl, negp, جديد AstConst{fl, عدد_Zero}};
        eqp->user1(سچ);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(eqp);
    }
    فراغ ویکھو(AstRedOr* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // Convert to (x != 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت argp = nodep->lhsp()->unlink_Fr_پٹھ();
        ثابت V3_نمبر عدد_Zero{fl, argp->چوڑائی(), 0};
        Ast_گرہ_اظہاریہ* ثابت neqp = جديد AstNeq{fl, argp, جديد AstConst{fl, عدد_Zero}};
        neqp->user1(سچ);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(neqp);
    }
    فراغ ویکھو(AstOneHot* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت argp = nodep->lhsp()->unlink_Fr_پٹھ();
        ثابت صحيح w = argp->چوڑائی();

        ثابت V3_نمبر عدد_Zero{fl, w, 0};
        AstNeq* ثابت neZerop
            = جديد AstNeq{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, عدد_Zero}};
        neZerop->user1(سچ);

        ثابت V3_نمبر عدد_One{fl, w, 1};
        AstSub* ثابت subp = جديد AstSub{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, عدد_One}};
        subp->user1(سچ);

        AstAnd* ثابت andp = جديد AstAnd{fl, argp, subp};
        andp->user1(سچ);

        ثابت V3_نمبر عدد_Zero2{fl, w, 0};
        AstEq* ثابت eqZerop = جديد AstEq{fl, andp, جديد AstConst{fl, عدد_Zero2}};
        eqZerop->user1(سچ);

        AstLogAnd* ثابت resultp = جديد AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(سچ);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(resultp);
    }
    فراغ ویکھو(AstOneHot0* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $onehot0(x) = (x & (x-1)) == 0
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت argp = nodep->lhsp()->unlink_Fr_پٹھ();
        ثابت صحيح w = argp->چوڑائی();

        ثابت V3_نمبر عدد_One{fl, w, 1};
        AstSub* ثابت subp = جديد AstSub{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, عدد_One}};
        subp->dtypeFrom(argp);
        subp->user1(سچ);

        AstAnd* ثابت andp = جديد AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(سچ);

        ثابت V3_نمبر عدد_Zero{fl, w, 0};
        AstEq* ثابت eqp = جديد AstEq{fl, andp, جديد AstConst{fl, عدد_Zero}};
        eqp->user1(سچ);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(eqp);
    }
    فراغ ویکھو(Ast_گنتی_بٹ* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سطر* ثابت fl = nodep->fileline();

        منطقي گنتی_Ones = خطأ;
        منطقي گنتی_Zeros = خطأ;
        لكل (Ast_گرہ_اظہاریہ* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ثابت AstConst* ثابت cp = VN_CAST(ctrlp, Const);
            إذا (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ثابت zerop
                    = جديد AstConst{fl, AstConst::Widthed_قدر{}, nodep->چوڑائی(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                أرجع;
            }
            إذا (cp->عدد().ثنائی_ہندسہ_Is1(0) && !گنتی_Ones)
                گنتی_Ones = سچ;
            وإلا إذا (cp->عدد().ثنائی_ہندسہ_Is0(0) && !گنتی_Zeros)
                گنتی_Zeros = سچ;
        }

        Ast_گرہ_اظہاریہ* ثابت argp = nodep->lhsp()->unlink_Fr_پٹھ();
        ثابت صحيح arg_چوڑائی = argp->چوڑائی();
        Ast_گرہ_اظہاریہ* sump = خالی_سوچک;
        إذا (گنتی_Ones && گنتی_Zeros) {
            // ones + zeros = width for 2-state types
            sump = جديد AstConst{fl, AstConst::Widthed_قدر{}, nodep->چوڑائی(), (uint32_t)arg_چوڑائی};
            VL_DO_DANGLING(argp->delete_درخت(), argp);
        } وإلا إذا (گنتی_Ones) {
            sump = build_گنتی_Ones_Expansion(fl, argp);
        } وإلا إذا (گنتی_Zeros) {
            // width - countones(x)
            Ast_گرہ_اظہاریہ* ثابت onesCountp = build_گنتی_Ones_Expansion(fl, argp);
            ثابت V3_نمبر چوڑائی_Val{nodep, onesCountp->چوڑائی(), (uint32_t)arg_چوڑائی};
            sump = جديد AstSub{fl, جديد AstConst{fl, چوڑائی_Val}, onesCountp};
            sump->user1(سچ);
        } وإلا {
            sump = جديد AstConst{fl, AstConst::Widthed_قدر{}, nodep->چوڑائی(), 0U};
            VL_DO_DANGLING(argp->delete_درخت(), argp);
        }

        sump = adjust_چوڑائی(fl, sump, nodep->چوڑائی(), nodep->dtypep()->numeric());
        sump->user1(سچ);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(sump);
    }
    فراغ ویکھو(AstCLog2* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت argp = nodep->lhsp()->unlink_Fr_پٹھ();
        ثابت صحيح arg_چوڑائی = argp->چوڑائی();
        ثابت صحيح نتیجہ_چوڑائی = nodep->چوڑائی();

        Ast_گرہ_اظہاریہ* resultp
            = جديد AstConst{fl, AstConst::Widthed_قدر{}, نتیجہ_چوڑائی, (uint32_t)arg_چوڑائی};

        لكل (صحيح k = arg_چوڑائی - 1; k >= 0; k--) {
            V3_نمبر حد{fl, arg_چوڑائی, 0};
            إذا (k < 32)
                حد.سیٹ_Long(1ULL << k);
            وإلا
                حد.سیٹ_ثنائی_ہندسہ(k, 1);

            AstLte* ثابت ltep
                = جديد AstLte{fl, argp->clone_درخت_خالص(خطأ), جديد AstConst{fl, حد}};
            ltep->user1(سچ);

            AstConst* ثابت valuep
                = جديد AstConst{fl, AstConst::Widthed_قدر{}, نتیجہ_چوڑائی, (uint32_t)k};

            resultp = جديد AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(سچ);
        }

        VL_DO_DANGLING(argp->delete_درخت(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(resultp);
    }
    فراغ ویکھو(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ثابت zerop = جديد AstConst{nodep->fileline(), AstConst::ثنائی_ہندسہ_False{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    فراغ handlePow(Ast_گرہ_Biop* nodep) {
        إذا (AstConst* ثابت exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* ثابت basep = nodep->lhsp();
            ثابت V3_نمبر عدد_One{nodep, basep->چوڑائی(), 1};
            Ast_گرہ_اظہاریہ* powerp = جديد AstConst{fl, عدد_One};
            ثابت منطقي ادھار_Signed = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ثابت int32_t exponent = ادھار_Signed ? exponentp->منزل_SInt() : exponentp->منزل_UInt();
            إذا (exponent > 0) {
                لكل (int32_t i = 0; i < exponent; i++) {
                    إذا (ادھار_Signed) {
                        powerp = جديد AstMulS{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    } وإلا {
                        powerp = جديد AstMul{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    }
                    powerp->user1(سچ);
                }
            } وإلا إذا (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                إذا (ادھار_Signed) {
                    powerp = جديد AstDivS{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    powerp->user1(سچ);
                    powerp = جديد AstDivS{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    powerp->user1(سچ);
                } وإلا {
                    powerp = جديد AstDiv{fl, powerp, basep->clone_درخت_خالص(خطأ)};
                    powerp->user1(سچ);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            iterate(powerp);
        } وإلا إذا (ثابت AstConst* ثابت basep = VN_CAST(nodep->lhsp(), Const)) {
            إذا (basep->عدد().منزل_UInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_سطر* ثابت fl = nodep->fileline();
                Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
                ثابت صحيح چوڑائی = nodep->چوڑائی();
                ثابت منطقي rhsDependent = rhsp->user1();
                AstConst* ثابت onep = جديد AstConst{fl, AstConst::Widthed_قدر{}, چوڑائی, 1};
                Ast_کھسکاؤ_L* ثابت shiftp = جديد Ast_کھسکاؤ_L{fl, onep, rhsp, چوڑائی};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_گرہ_اظہاریہ* resultp = shiftp;
                إذا (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_گرہ_اظہاریہ* ثابت nClonep = rhsp->clone_درخت_خالص(خطأ);
                    nClonep->user1(rhsDependent);
                    AstConst* ثابت zeroNp
                        = جديد AstConst{fl, AstConst::Widthed_قدر{}, rhsp->چوڑائی(), 0};
                    AstGteS* ثابت condp = جديد AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ثابت zeroRp = جديد AstConst{fl, AstConst::Widthed_قدر{}, چوڑائی, 0};
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
    فراغ ویکھو(AstPow* nodep) override { handlePow(nodep); }
    فراغ ویکھو(AstPowSS* nodep) override { handlePow(nodep); }
    فراغ ویکھو(AstPowSU* nodep) override { handlePow(nodep); }
    فراغ ویکھو(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    فراغ handle_کھسکاؤ(Ast_گرہ_Biop* nodep) {
        إذا (editFormat(nodep)) أرجع;
        ثابت صحيح lhs_چوڑائی = nodep->lhsp()->چوڑائی();
        ثابت صحيح rhs_چوڑائی = nodep->rhsp()->چوڑائی();
        إذا (rhs_چوڑائی < lhs_چوڑائی) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp()->unlink_Fr_پٹھ();
            AstExtend* ثابت extendp = جديد AstExtend{fl, rhsp, lhs_چوڑائی};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(سچ);
            nodep->rhsp(extendp);
        } وإلا إذا (rhs_چوڑائی > lhs_چوڑائی) {
            File_سطر* ثابت fl = nodep->fileline();
            Ast_گرہ_DType* ثابت dtypep = nodep->dtypep();
            Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp()->unlink_Fr_پٹھ();
            Ast_گرہ_اظہاریہ* ثابت extendp
                = VN_IS(nodep, کھسکاؤ_RS)
                      ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstExtendS{fl, lhsp, rhs_چوڑائی})
                      : static_cast<Ast_گرہ_اظہاریہ*>(جديد AstExtend{fl, lhsp, rhs_چوڑائی});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(سچ);
            nodep->lhsp(extendp);
            nodep->dtype_سیٹ_Logic_Sized(rhs_چوڑائی, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlink_Fr_پٹھ(&relinker);
            AstSel* ثابت selp = جديد AstSel{fl, nodep, 0, lhs_چوڑائی};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            أرجع;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ ویکھو(Ast_کھسکاؤ_L* nodep) override { handle_کھسکاؤ(nodep); }
    فراغ ویکھو(Ast_کھسکاؤ_R* nodep) override { handle_کھسکاؤ(nodep); }
    فراغ ویکھو(Ast_کھسکاؤ_RS* nodep) override { handle_کھسکاؤ(nodep); }
    فراغ ویکھو(Ast_گرہ_Biop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    فراغ ویکھو(Ast_گرہ_Uniop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp());
    }
    فراغ ویکھو(Ast_گرہ_Triop* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    فراغ ویکھو(AstCond* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        إذا (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtype_سیٹ_String();
            أرجع;
        }
        // Fall back to "(ite cond then else)"
        ویکھو(static_cast<Ast_گرہ_Triop*>(nodep));
    }
    فراغ ویکھو(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        إذا (editFormat(nodep)) أرجع;
        editSMT(nodep, nodep->srcp());
    }
    فراغ ویکھو(AstSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        VNRelinker handle;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت msbp = جديد AstSFormatF{
            fl, "%1d", خطأ,
            جديد Ast_جوڑو{fl, nodep->lsbp()->clone_درخت_خالص(خطأ),
                       جديد AstConst{fl, static_cast<uint32_t>(nodep->چوڑائی_Const() - 1)}}};
        Ast_گرہ_اظہاریہ* ثابت lsbp
            = جديد AstSFormatF{fl, "%1d", خطأ, nodep->lsbp()->unlink_Fr_پٹھ(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    فراغ ویکھو(AstStructSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        ثابت منطقي outermost = !m_defer_Rand_منوال_Hoist;
        Ast_گرہ_اظہاریہ* ثابت origp = outermost ? nodep->clone_درخت(خطأ) : خالی_سوچک;
        VL_RESTORER(m_structSel);
        m_structSel = سچ;
        إذا (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_گرہ_اظہاریہ* ثابت fromp = nodep->fromp();
            AstStructDType* ثابت structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            إذا (is_Rand_Qualified_رستہ(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } وإلا {
                structp->markConstrainedRand(سچ);
            }
        }
        // Mark Random for structArray
        إذا (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_گرہ_اظہاریہ* ثابت fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ثابت structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            إذا (is_Rand_Qualified_رستہ(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } وإلا {
                structp->markConstrainedRand(سچ);
            }
        }
        {
            VL_RESTORER(m_defer_Rand_منوال_Hoist);
            m_defer_Rand_منوال_Hoist = سچ;
            iterateChildren(nodep);
        }
        File_سطر* ثابت fl = nodep->fileline();
        AstSFormatF* newp = خالی_سوچک;
        إذا (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = جديد AstSFormatF{fl, "%s.%s." + nodep->name(), خطأ,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_درخت_خالص(سچ)};
            إذا (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } وإلا {
            newp = جديد AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), خطأ,
                                   خالی_سوچک};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        إذا (origp && !hoist_Rand_منوال_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(ڈالو_Deletep(origp), origp);
        }
    }
    فراغ جوڑو_String_Name_Part(Ast_گرہ_اظہاریہ* ثابت indexp, ثابت std::string& fmt = "%32x") {
        إذا (m_nestedAccess) {
            Ast_گرہ_اظہاریہ* ثابت bitp = indexp->clone_درخت_خالص(خطأ);
            Ast_گرہ_اظہاریہ* ثابت ثنائی_ہندسہ_Formatp = جديد AstSFormatF{bitp->fileline(), fmt, خطأ, bitp};
            AstSFormatF* ثابت herep
                = جديد AstSFormatF{indexp->fileline(), "%s.%s", خطأ, ثنائی_ہندسہ_Formatp};
            m_nestedAccess->جوڑو_Var_Name_Part(herep, indexp->name());
        }
    }
    فراغ ویکھو(AstAssocSel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سطر* ثابت fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_گرہ_اظہاریہ* ثابت origp = nodep->clone_درخت(خطأ);
        AstSFormatF* newp = خالی_سوچک;
        إذا (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            جوڑو_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_گرہ_اظہاریہ* ثابت idxp = جديد AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), خطأ,
                                                      nodep->bitp()->unlink_Fr_پٹھ(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وإلا إذا (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ثابت stringp = VN_AS(nodep->bitp(), CvtPackString);
            ثابت ناپ_t string_ناپ = VN_AS(stringp->lhsp(), Const)->چوڑائی();
            إذا (string_ناپ > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << string_ناپ << "bits, limit is 128 bits");
            }
            جوڑو_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_گرہ_اظہاریہ* ثابت idxp = جديد AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), خطأ,
                                                      stringp->lhsp()->unlink_Fr_پٹھ(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } وإلا {
            إذا (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ثابت صحيح actual_چوڑائی = nodep->bitp()->چوڑائی();
                std::string fmt;
                // Normalize to standard bit width
                إذا (actual_چوڑائی <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } وإلا إذا (actual_چوڑائی <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } وإلا {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::منزل_string(VL_الفاظ_I(actual_چوڑائی) * 8) + "x";
                }
                إذا (m_nestedAccess) جوڑو_String_Name_Part(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_گرہ_اظہاریہ* ثابت idxp
                    = جديد AstSFormatF{fl, fmt, خطأ, nodep->bitp()->unlink_Fr_پٹھ(&handle)};
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
        إذا (!newp || !hoist_Rand_منوال_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(origp->delete_درخت(), origp);
        }
    }
    فراغ rename_Array_اظہاریہ(AstSFormatF* ثابت newp) ثابت {
        إذا (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    فراغ ویکھو(AstArraySel* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سطر* ثابت fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        منطقي اشاریہ_Is_Rand = خطأ;
        nodep->bitp()->foreach([&](ثابت Ast_گرہ_Var_Ref* vrefp) {
            إذا (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                اشاریہ_Is_Rand = سچ;
            }
        });
        إذا (m_nestedAccess) {
            Ast_گرہ_اظہاریہ* ثابت bitp = nodep->bitp()->clone_درخت_خالص(خطأ);
            Ast_گرہ_اظہاریہ* ثابت ثنائی_ہندسہ_Formatp
                = اشاریہ_Is_Rand ? جديد AstSFormatF{bitp->fileline(), bitp->name(), خطأ, خالی_سوچک}
                              : جديد AstSFormatF{bitp->fileline(), "%x", خطأ, bitp};
            AstSFormatF* ثابت herep
                = جديد AstSFormatF{nodep->fileline(), "%s.%s", خطأ, ثنائی_ہندسہ_Formatp};
            إذا (AstSel* ثابت selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->جوڑو_Var_Name_Part(herep, selp->fromp()->name());
            } وإلا {
                m_nestedAccess->جوڑو_Var_Name_Part(herep, bitp->name());
            }
            إذا (اشاریہ_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_درخت(), bitp);
                أرجع;
            }
        }
        إذا (اشاریہ_Is_Rand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_گرہ_اظہاریہ* indexp = nodep->bitp()->unlink_Fr_پٹھ(&handle);
            إذا (indexp->چوڑائی() < 32) {
                AstExtend* ثابت extendp = جديد AstExtend{fl, indexp, 32};
                extendp->user1(سچ);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_اظہاریہ(newp);
        } وإلا {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_گرہ_اظہاریہ* ثابت origp = nodep->clone_درخت(خطأ);
            Ast_گرہ_اظہاریہ* ثابت indexp
                = جديد AstSFormatF{fl, "#x%8x", خطأ, nodep->bitp()->unlink_Fr_پٹھ(&handle)};
            handle.relink(indexp);
            AstSFormatF* ثابت newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_اظہاریہ(newp);
            إذا (!newp || !hoist_Rand_منوال_Over_Select_And_Member(newp, origp)) {
                VL_DO_DANGLING(origp->delete_درخت(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ساكن منطقي is_Rand_منوال_Gate(ثابت Ast_گرہ_اظہاریہ* exprp) {
        ثابت AstCMethodHard* ثابت atp = VN_CAST(exprp, CMethodHard);
        إذا (!atp || atp->طریقہ() != VCMethod::ARRAY_AT) أرجع خطأ;
        ثابت AstVar* varp = خالی_سوچک;
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
    منطقي hoist_Rand_منوال_Over_Select_And_Member(AstSFormatF* newp, Ast_گرہ_اظہاریہ* origp) {
        إذا (m_defer_Rand_منوال_Hoist) أرجع خطأ;
        // Only for selects yielding a non-array element (full chains)
        إذا (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::سمتیہ<AstSFormatF*> چوکھٹے;
        AstCond* منوال_Condp = خالی_سوچک;
        لكل (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_گرہ_اظہاریہ* ثابت firstp = curp->exprsp();
            چوکھٹے.ڈالو_پٹھ(curp);
            إذا (AstCond* ثابت condp = VN_CAST(firstp, Cond)) {
                إذا (is_Rand_منوال_Gate(condp->condp())) منوال_Condp = condp;
                اكسر;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        إذا (!منوال_Condp) أرجع خطأ;
        File_سطر* ثابت fl = newp->fileline();
        Ast_گرہ_اظہاریہ* ثابت modep = منوال_Condp->condp()->unlink_Fr_پٹھ();
        Ast_گرہ_اظہاریہ* activep = منوال_Condp->thenp()->unlink_Fr_پٹھ();
        // Rebuild the select chain text around the SMT name, innermost first
        لكل (خودکار it = چوکھٹے.rbegin(); it != چوکھٹے.rend(); ++it) {
            Ast_گرہ_اظہاریہ* ثابت بچیا_Argsp = VN_CAST((*it)->exprsp()->nextp(), گرہ_اظہاریہ);
            إذا (بچیا_Argsp) { بچیا_Argsp->unlink_Fr_پٹھ_With_اگلا(); }
            activep = جديد AstSFormatF{fl, (*it)->name(), خطأ,
                                      Ast_گرہ::جوڑو_اگلا(activep, بچیا_Argsp)};
        }
        AstCond* ثابت hoistp = جديد AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(سچ);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(ڈالو_Deletep(newp), newp);
        أرجع سچ;
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        إذا (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_سطر* ثابت fl = nodep->fileline();
            إذا (m_nestedAccess) {
                AstSFormatF* ثابت formatp = جديد AstSFormatF{fl, nodep->name(), خطأ, خالی_سوچک};
                m_nestedAccess->جوڑو_Var_Name_Part(جديد AstSFormatF{fl, "%s.%s", خطأ, formatp},
                                               nodep->name());
            }
            Ast_گرہ* جڑ_گرہ = nodep->fromp();
            طالما (ثابت AstMemberSel* ثابت selp = VN_CAST(جڑ_گرہ, MemberSel)) {
                جڑ_گرہ = selp->fromp();
            }

            Ast_گرہ_Sel* arraySelp = VN_CAST(جڑ_گرہ, ArraySel);
            إذا (!arraySelp) arraySelp = VN_CAST(جڑ_گرہ, AssocSel);

            إذا (arraySelp) {
                Ast_گرہ_DType* ثابت arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_گرہ_DType* ثابت elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : خالی_سوچک;
                إذا (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ثابت منطقي isSimple
                        = nodep->fromp() == جڑ_گرہ && VN_IS(arraySelp->fromp(), VarRef);
                    إذا (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = جديد Nested_Access_رستہ(nodep->clone_درخت(خطأ),
                                                              &m_پہلا_عبارت_Inside_Indexp);
                        AstSFormatF* ثابت formatp
                            = جديد AstSFormatF{fl, nodep->name(), خطأ, خالی_سوچک};
                        m_nestedAccess->جوڑو_Var_Name_Part(
                            جديد AstSFormatF{fl, "%s.%s", خطأ, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = سچ;
                    nodep->user1(سچ);
                    arraySelp->user1(سچ);
                    Ast_گرہ_اظہاریہ* ثابت origp = nodep->clone_درخت(خطأ);
                    iterateChildren(nodep);
                    AstSFormatF* ثابت newp = جديد AstSFormatF{fl, "%s." + nodep->name(), خطأ,
                                                              nodep->fromp()->unlink_Fr_پٹھ()};
                    nodep->replaceWith(newp);
                    إذا (!hoist_Rand_منوال_Over_Select_And_Member(newp, origp))
                        VL_DO_DANGLING(origp->delete_درخت(), origp);
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    VL_DO_DANGLING(مٹاؤ m_nestedAccess, m_nestedAccess);
                    أرجع;
                }
            }
            // Check if the root variable participates in global constraints
            إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(جڑ_گرہ, VarRef)) {
                AstVar* ثابت constrainedVar = varRefp->varp();
                إذا (constrainedVar->globalConstrained()) {
                    منطقي ثابت beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    إذا (beforeIterate) {
                        nodep->replaceWith(جديد AstSFormatF{fl, "%s." + nodep->name(), خطأ,
                                                           nodep->fromp()->unlink_Fr_پٹھ()});
                    } وإلا {
                        nodep->replaceWith(nodep->fromp()->unlink_Fr_پٹھ());
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
            && VN_AS(nodep->user2p(), گرہ_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ثابت varRefp
                = جديد AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), گرہ_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            ویکھو(varRefp);
        } وإلا إذا (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->طریقہ() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ثابت cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_گرہ_DType* ثابت arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_گرہ_DType* ثابت elemDtp = arrayDtp->subDTypep()->skipRefp();
            إذا (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = سچ;
                iterateChildren(nodep);
                File_سطر* ثابت fl = nodep->fileline();
                AstSFormatF* newp = خالی_سوچک;
                إذا (AstSFormatF* ثابت fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    إذا (fromp->name() == "%s.%s") {
                        newp = جديد AstSFormatF{fl, "%s.%s." + nodep->name(), خطأ,
                                               fromp->exprsp()->clone_درخت_خالص(سچ)};
                    } وإلا {
                        newp = جديد AstSFormatF{fl, fromp->name() + "." + nodep->name(), خطأ,
                                               خالی_سوچک};
                    }
                } وإلا {
                    newp = جديد AstSFormatF{fl, nodep->name(), خطأ, خالی_سوچک};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                أرجع;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_پٹھ());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        } وإلا إذا (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_پٹھ());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        } وإلا {
            editFormat(nodep);
        }
    }
    فراغ ویکھو(AstSFormatF* nodep) override {}
    فراغ ویکھو(Ast_Stmt_اظہاریہ* nodep) override {}
    فراغ ویکھو(AstCStmt* nodep) override {}
    فراغ ویکھو(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    فراغ ویکھو(AstConstraintIf* nodep) override {
        Ast_گرہ_اظہاریہ* newp = خالی_سوچک;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_اظہاریہ* ثابت thenp = editSingle(fl, nodep->thensp());
        Ast_گرہ_اظہاریہ* ثابت elsep = editSingle(fl, nodep->elsesp());
        إذا (thenp && elsep) {
            newp = جديد AstCond{fl, nodep->condp()->unlink_Fr_پٹھ(), thenp, elsep};
        } وإلا إذا (thenp) {
            newp = جديد AstLogIf{fl, nodep->condp()->unlink_Fr_پٹھ(), thenp};
        } وإلا إذا (elsep) {
            newp = جديد AstLogIf{fl, جديد AstNot{fl, nodep->condp()->unlink_Fr_پٹھ()}, elsep};
        }
        إذا (newp) {
            newp->dtype_سیٹ_ثنائی_ہندسہ();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(سچ);  // Assume result-dependent
            nodep->replaceWith(جديد Ast_Constraint_اظہاریہ{fl, newp});
        } وإلا {
            nodep->unlink_Fr_پٹھ();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(Ast_Gen_بلاک* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    فراغ ویکھو(AstBegin* nodep) override {}
    فراغ ویکھو(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_سطر* ثابت fl = nodep->fileline();

        إذا (!nodep->bodyp()) {
            nodep->unlink_Fr_پٹھ();
        } وإلا إذا (m_wantSingle) {
            Ast_گرہ_اظہاریہ* ثابت itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            cstmtp->جوڑو("ret += \" \";\n");
            cstmtp->جوڑو("ret += ");
            cstmtp->جوڑو(itemp);
            cstmtp->جوڑو(";");
            Ast_گرہ* ثابت bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->dtype_سیٹ_String();
            cexprp->جوڑو("([&]{\nstd::string ret;\n");
            cexprp->جوڑو(جديد AstBegin{fl, m_unique_ناں.get("__VrandBlock"),
                                     جديد AstForeach{fl, nodep->headerp()->unlink_Fr_پٹھ(), bodyp},
                                     سچ});
            cexprp->جوڑو("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
        } وإلا {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            لكل (Ast_گرہ* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                إذا (ثابت AstVar* ثابت varp = VN_CAST(elemp, Var)) m_foreachIdxVars.داخل_کرو(varp);
            }
            iterate_And_اگلا_Null(nodep->bodyp());
            Ast_گرہ* ثابت bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlink_Fr_پٹھ_With_اگلا());
            nodep->replaceWith(
                جديد AstBegin{fl, m_unique_ناں.get("__VrandBlock"),
                             جديد AstForeach{fl, nodep->headerp()->unlink_Fr_پٹھ(), bodyp}, سچ});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_Module* ثابت genModp = VN_AS(m_genp->user2p(), گرہ_Module);

        لكل (Ast_گرہ_اظہاریہ* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), گرہ_اظہاریہ)) {
            Ast_گرہ_اظہاریہ* ثابت lhsTestp = build_Solve_Before_Name_اظہاریہ(fl, lhsp);
            إذا (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                استمر;
            }
            VL_DO_DANGLING(lhsTestp->delete_درخت(), lhsTestp);
            لكل (Ast_گرہ_اظہاریہ* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), گرہ_اظہاریہ)) {
                Ast_گرہ_اظہاریہ* ثابت rhsNamep = build_Solve_Before_Name_اظہاریہ(fl, rhsp);
                إذا (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    استمر;
                }
                AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                    fl, جديد AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtype_سیٹ_Void();
                callp->جوڑو_Pinsp(build_Solve_Before_Name_اظہاریہ(fl, lhsp));
                callp->جوڑو_Pinsp(rhsNamep);
                nodep->جوڑو_ایتھے_This_As_اگلا(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstConstraintUnique* nodep) override {
        Ast_گرہ_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گرہ_Module* genModp = خالی_سوچک;

        إذا (m_classp) {
            initTaskp = VN_AS(m_memberMap.لبھو_Member(m_classp, "new"), گرہ_FTask);
            genVarp = getRandomGenerator(m_classp);
        } وإلا {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        إذا (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_پٹھ()), nodep);
            أرجع;
        }
        إذا (m_classp) {
            genModp = VN_AS(genVarp->user2p(), گرہ_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_گرہ* تیاری_Stmtsp = خالی_سوچک;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::سمتیہ<std::string> smtExprs;
        لكل (Ast_گرہ* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            إذا (AstVarRef* ثابت varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ثابت varp = varRefp->varp();
                Ast_گرہ_Module* ثابت varModp = VN_AS(varp->user2p(), گرہ_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();

                ثابت ساكن خودکار dynDTypeSupported
                    = [](ثابت Ast_گرہ_DType* ثابت dtypep) -> منطقي {
                    أرجع VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, قطار_DType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elem_چوڑائی;
                uint32_t static_ناپ = 0;  // Element count, zero when only known at run time
                إذا (ثابت AstUnpackArrayDType* ثابت اتے = VN_CAST(dtypep, UnpackArrayDType)) {
                    ثابت AstRange* ثابت rangep = اتے->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    static_ناپ = اتے->elementsConst();
                    إذا (static_ناپ > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        استمر;
                    }
                    elem_چوڑائی = static_cast<uint64_t>(varp->dtypep()->چوڑائی());
                } وإلا إذا (dynDTypeSupported(dtypep)) {
                    elem_چوڑائی = static_cast<uint64_t>(dtypep->subDTypep()->چوڑائی());
                } وإلا {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(خطأ));
                    استمر;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ثابت Ast_گرہ_DType* ثابت subp = dtypep->subDTypep()->skipRefp();
                إذا (VN_IS(subp, گرہ_Array_DType) || VN_IS(subp, قطار_DType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    استمر;
                }

                إذا (m_wantSingle && !static_ناپ) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    استمر;
                }

                إذا (!varp->user3()) {
                    varp->user3(سچ);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_گرہ_اظہاریہ* ثابت varnamep = جديد AstCExpr{
                        fl, AstCExpr::خالص{}, "\"" + varp->name() + "\"", varp->چوڑائی()};

                    AstCMethodHard* ثابت writeVarCallp = جديد AstCMethodHard{
                        fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ثابت argVarRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                    إذا (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->جوڑو_Pinsp(argVarRefp);
                    writeVarCallp->جوڑو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, elem_چوڑائی});
                    writeVarCallp->جوڑو_Pinsp(varnamep);
                    writeVarCallp->جوڑو_Pinsp(جديد AstConst{fl, 1});  // Dimension
                    ثابت Randomize_منوال rand_منوال = {.asUQuad = varp->user1()};
                    إذا (rand_منوال.ورتدا_منوال) {
                        writeVarCallp->جوڑو_Pinsp(
                            جديد AstConst{fl, AstConst::Unsized64{}, rand_منوال.اشاریہ});
                    }
                    writeVarCallp->dtype_سیٹ_Void();
                    initTaskp->جوڑو_Stmtsp(جديد Ast_Stmt_اظہاریہ{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                إذا (varp->user4p() && m_ناپ_Constrained_Arraysp) {
                    m_ناپ_Constrained_Arraysp->داخل_کرو(varp);
                }

                إذا (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    إذا (static_ناپ < 2) استمر;
                    std::string اظہاریہ_Str = "(__Vbv (distinct";
                    لكل (uint32_t i = 0; i < static_ناپ; ++i) {
                        اظہاریہ_Str += " (select " + varp->name() + " #x";
                        لكل (صحيح کھسکاؤ_2 = 28; کھسکاؤ_2 >= 0; کھسکاؤ_2 -= 4) {
                            اظہاریہ_Str += "0123456789abcdef"[(i >> کھسکاؤ_2) & 0xf];
                        }
                        اظہاریہ_Str += ')';
                    }
                    smtExprs.emplace_پٹھ(اظہاریہ_Str + "))");
                    استمر;
                }

                Ast_گرہ_اظہاریہ* ثابت namep = جديد AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ثابت randUniqueCallp
                    = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->لبھو_Void_DType());
                randUniqueCallp->جوڑو_Pinsp(جديد AstConst{fl, *m_uniqueConstraintId});
                تیاری_Stmtsp = Ast_گرہ::جوڑو_اگلا(تیاری_Stmtsp, جديد Ast_Stmt_اظہاریہ{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        إذا (m_wantSingle && !smtExprs.خالی_تھاواں()) {
            std::string اظہاریہ_Str = smtExprs.front();
            إذا (smtExprs.ناپ() > 1) {
                اظہاریہ_Str = "(bvand";
                لكل (ثابت std::string& oneExprp : smtExprs) اظہاریہ_Str += " " + oneExprp;
                اظہاریہ_Str += ')';
            }
            AstSFormatF* ثابت newp = جديد AstSFormatF{fl, اظہاریہ_Str, خطأ, خالی_سوچک};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (تیاری_Stmtsp) nodep->جوڑو_ایتھے_This_As_اگلا(تیاری_Stmtsp);
        nodep->unlink_Fr_پٹھ();
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }

    فراغ ویکھو(Ast_Constraint_اظہاریہ* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_پہلا_عبارت_Inside_Indexp);
        إذا (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_گرہ_اظہاریہ* ثابت exprp = nodep->exprp();
            إذا (exprp->چوڑائی() > 1) {
                File_سطر* ثابت fl = exprp->fileline();
                ثابت V3_نمبر عدد_Zero{fl, exprp->چوڑائی(), 0};
                Ast_گرہ_اظہاریہ* ثابت neqp
                    = جديد AstNeq{fl, exprp->unlink_Fr_پٹھ(), جديد AstConst{fl, عدد_Zero}};
                neqp->user1(سچ);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        إذا (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_پٹھ());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ثابت VCMethod طریقہ
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), گرہ_Module), m_genp,
                          VAccess::READWRITE},
            طریقہ, nodep->exprp()->unlink_Fr_پٹھ()};
        callp->dtype_سیٹ_Void();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->جوڑو_Pinsp(جديد AstCExpr{nodep->fileline(), AstCExpr::خالص{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ثابت uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->جوڑو_Pinsp(جديد AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string pretty_متن;
        إذا (!v3Global.opt.protectIds()) {
            pretty_متن = nodep->fileline()->pretty_سروت();
            ناپ_t pos = 0;
            طالما ((pos = pretty_متن.لبھو('"', pos)) != std::string::npos) {
                pretty_متن.داخل_کرو(pos, "\\");
                pos += std::لڑی_لمبائی("\\\"");
            }
        }
        callp->جوڑو_Pinsp(
            جديد AstCExpr{nodep->fileline(), AstCExpr::خالص{}, "\"" + pretty_متن + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ ویکھو(AstCMethodHard* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        File_سطر* ثابت fl = nodep->fileline();

        إذا (nodep->طریقہ() == VCMethod::ARRAY_AT) {
            جوڑو_String_Name_Part(nodep->pinsp(), "%x");
            ثابت منطقي اشاریہ_Is_Rand = nodep->pinsp()->exists([](ثابت Ast_گرہ_Var_Ref* ثابت vrefp) {
                أرجع vrefp->varp()->rand().isRandomizable();
            });
            إذا (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ثابت Ast_گرہ* ثابت basep = nodep->ادھار_Fromp(سچ);
                    إذا (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        أرجع;
                    }
                }
                Ast_گرہ_اظہاریہ* ثابت origp = اشاریہ_Is_Rand ? خالی_سوچک : nodep->clone_درخت(خطأ);
                ثابت AstVarRef* ثابت ادھار_Refp
                    = m_structSel ? VN_CAST(nodep->fromp()->ادھار_Fromp(سچ), VarRef) : خالی_سوچک;
                إذا (ادھار_Refp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(ادھار_Refp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_گرہ_اظہاریہ* ثابت indexp = nodep->pinsp()->clone_درخت_خالص(خطأ);
                    Ast_گرہ_اظہاریہ* ثابت arrayp = nodep->fromp()->clone_درخت_خالص(خطأ);
                    AstCMethodHard* ثابت currentSizep
                        = جديد AstCMethodHard{fl, arrayp->clone_درخت_خالص(خطأ), VCMethod::DYN_ناپ};
                    currentSizep->dtype_سیٹ_Int();
                    Ast_گرہ_اظہاریہ* ثابت وچ_Range_Requiredp = جديد AstLogAnd{
                        fl,
                        جديد AstLteS{fl,
                                    جديد AstConst{fl, AstConst::Widthed_قدر{}, indexp->چوڑائی(), 0},
                                    indexp->clone_درخت_خالص(خطأ)},
                        جديد AstGteS{fl, indexp->clone_درخت_خالص(خطأ), currentSizep}};
                    AstCMethodHard* ثابت resizep
                        = جديد AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             جديد Ast_جوڑو{fl, indexp, جديد AstConst{fl, 1}}};
                    resizep->dtype_سیٹ_Void();
                    m_prepareConstrainedArraysp->جوڑو_Stmtsp(
                        جديد AstIf{fl, وچ_Range_Requiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ثابت sizep
                    = m_structSel ? جديد AstCMethodHard{fl, nodep->fromp()->clone_درخت_خالص(خطأ),
                                                       VCMethod::DYN_ناپ}
                                  : خالی_سوچک;
                Ast_گرہ_اظہاریہ* ثابت originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_گرہ_اظہاریہ* ثابت pinp = nodep->pinsp()->unlink_Fr_پٹھ();
                AstSFormatF* newp;
                إذا (m_structSel) {
                    إذا (AstSFormatF* ثابت sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_گرہ_اظہاریہ* ثابت argsp
                        = Ast_گرہ::جوڑو_اگلا(nodep->fromp()->unlink_Fr_پٹھ(), pinp);
                    sizep->dtype_سیٹ_Int();
                    AstLogAnd* ثابت condp
                        = جديد AstLogAnd{fl,
                                        جديد AstLteS{fl,
                                                    جديد AstConst{fl, AstConst::Widthed_قدر{},
                                                                 originalPinp->چوڑائی(), 0},
                                                    originalPinp->clone_درخت_خالص(خطأ)},
                                        جديد AstLtS{fl, originalPinp->clone_درخت_خالص(خطأ), sizep}};
                    m_conditionp = m_conditionp ? جديد AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = جديد AstSFormatF{fl, "%s.%s", خطأ, argsp};
                } وإلا {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_پٹھ(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                إذا (origp && !hoist_Rand_منوال_Over_Select_And_Member(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_درخت(), origp);
                }

            } وإلا إذا (اشاریہ_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } وإلا {
                nodep->user1(خطأ);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            أرجع;
        }

        إذا (nodep->طریقہ() == VCMethod::ARRAY_INSIDE) {
            ثابت منطقي randArr = nodep->fromp()->user1();

            AstVar* ثابت newVarp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->لبھو_Int_DType()};
            Ast_گرہ_اظہاریہ* ثابت idxRefp = جديد AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            Ast_Foreach_سرخی* ثابت headerp
                = جديد Ast_Foreach_سرخی{fl, nodep->fromp()->clone_درخت_خالص(خطأ), newVarp};
            Ast_گرہ_اظہاریہ* ثابت selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_گرہ* ثابت itemp = جديد AstEq{fl, selp, nodep->pinsp()->unlink_Fr_پٹھ()};
            itemp->user1(سچ);

            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            cstmtp->جوڑو("ret += \" \";\n");
            cstmtp->جوڑو("ret += ");
            cstmtp->جوڑو(iterateSubtreeReturnEdits(itemp));
            cstmtp->جوڑو(";");
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->dtype_سیٹ_String();
            cexprp->جوڑو("([&]{\nstd::string ret;\n");
            cexprp->جوڑو(جديد AstBegin{fl, m_unique_ناں.get("__VrandBlock"),
                                     جديد AstForeach{fl, headerp, cstmtp}, سچ});
            cexprp->جوڑو("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }

        // Array reduction methods (sum, product, and, or, xor)
        إذا (nodep->طریقہ() == VCMethod::ARRAY_R_جوڑ
            || nodep->طریقہ() == VCMethod::ARRAY_R_ضرب_حاصل
            || nodep->طریقہ() == VCMethod::ARRAY_R_AND || nodep->طریقہ() == VCMethod::ARRAY_R_OR
            || nodep->طریقہ() == VCMethod::ARRAY_R_XOR) {

            AstWith* ثابت withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            إذا (withp) {
                إذا (AstUnpackArrayDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ثابت صحيح array_ناپ_2 = adtypep->elementsConst();
                    إذا (array_ناپ_2 > v3Global.opt.constraint_Array_حد_بندی()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvt_منزل_Str(array_ناپ_2)
                                          + " exceeds --constraint-array-limit of "
                                          + cvt_منزل_Str(v3Global.opt.constraint_Array_حد_بندی())
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
                = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->لبھو_Int_DType()};
            Ast_Foreach_سرخی* ثابت headerp
                = جديد Ast_Foreach_سرخی{fl, nodep->fromp()->clone_درخت_خالص(خطأ), loopVarp};

            // Determine SMT operation and compute identity elements
            ثابت حرف* smtOp = خالی_سوچک;
            std::string پچھان_تت;
            إذا (withp) {
                // For 'with' clause: use binary format, compute from result width
                ثابت صحيح چوڑائی = nodep->dtypep()->چوڑائی();
                إذا (nodep->طریقہ() == VCMethod::ARRAY_R_جوڑ) {
                    smtOp = "bvadd";
                    پچھان_تت = "#b" + std::string(چوڑائی, '0');
                } وإلا إذا (nodep->طریقہ() == VCMethod::ARRAY_R_ضرب_حاصل) {
                    smtOp = "bvmul";
                    پچھان_تت = (چوڑائی > 0) ? "#b" + std::string(چوڑائی - 1, '0') + "1" : "#b0";
                } وإلا إذا (nodep->طریقہ() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    پچھان_تت = "#b" + std::string(چوڑائی, '1');
                } وإلا إذا (nodep->طریقہ() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    پچھان_تت = "#b" + std::string(چوڑائی, '0');
                } وإلا {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    پچھان_تت = "#b" + std::string(چوڑائی, '0');
                }
            } وإلا {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ثابت arrVarp = arrRefp->varp();
                Ast_گرہ_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحيح elem_چوڑائی = elemDtp->چوڑائی();
                ثابت صحيح hexDigits = (elem_چوڑائی + 3) / 4;

                ثابت std::string zero_پچھان_تت = "#x" + std::string(hexDigits, '0');
                ثابت std::string one_پچھان_تت = "#x" + std::string(hexDigits - 1, '0') + "1";
                ثابت std::string all_Ones_پچھان_تت = "#x" + std::string(hexDigits, 'f');

                إذا (nodep->طریقہ() == VCMethod::ARRAY_R_جوڑ) {
                    smtOp = "bvadd";
                    پچھان_تت = zero_پچھان_تت;
                } وإلا إذا (nodep->طریقہ() == VCMethod::ARRAY_R_ضرب_حاصل) {
                    smtOp = "bvmul";
                    پچھان_تت = one_پچھان_تت;
                } وإلا إذا (nodep->طریقہ() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    پچھان_تت = all_Ones_پچھان_تت;
                } وإلا إذا (nodep->طریقہ() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    پچھان_تت = zero_پچھان_تت;
                } وإلا {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    پچھان_تت = zero_پچھان_تت;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ثابت cstmtp = جديد AstCStmt{fl};
            إذا (withp) {
                // With 'with' clause: evaluate expression for each element
                ثابت منطقي randArr = nodep->fromp()->user1();
                Ast_گرہ_اظہاریہ* ثابت idxRefp = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_گرہ_اظہاریہ* ثابت elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ثابت صحيح نتیجہ_چوڑائی = nodep->dtypep()->چوڑائی();
                ثابت VSigning نتیجہ_Signing = nodep->dtypep()->numeric();

                Ast_گرہ* فی_Elem_Exprp = withp->exprp()->clone_درخت_خالص(خطأ);
                إذا (AstLambdaArgRef* ثابت جڑ_Refp = VN_CAST(فی_Elem_Exprp, LambdaArgRef)) {
                    إذا (جڑ_Refp->اشاریہ()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ثابت loopRef = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                        فی_Elem_Exprp = adjust_چوڑائی(fl, loopRef, نتیجہ_چوڑائی, نتیجہ_Signing);
                    } وإلا {
                        // item at root -> replace with array element
                        فی_Elem_Exprp = elemSelp->clone_درخت_خالص(خطأ);
                    }
                    VL_DO_DANGLING(جڑ_Refp->delete_درخت(), جڑ_Refp);
                } وإلا {
                    فی_Elem_Exprp->foreach([&](AstLambdaArgRef* refp) {
                        إذا (refp->اشاریہ()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ثابت loopRef = جديد AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_گرہ* ثابت replacement
                                = adjust_چوڑائی(fl, loopRef, نتیجہ_چوڑائی, نتیجہ_Signing);
                            refp->replaceWith(replacement);
                        } وإلا {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_درخت_خالص(خطأ));
                        }
                        VL_DO_DANGLING(ڈالو_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_درخت(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                فی_Elem_Exprp = V3Const::constifyEdit(فی_Elem_Exprp);

                فی_Elem_Exprp->foreach([&](Ast_گرہ* nodep) {
                    // Don't mark loop variable references as randomizable
                    إذا (!VN_IS(nodep, Const)) {
                        إذا (Ast_گرہ_Var_Ref* ثابت vrefp = VN_CAST(nodep, گرہ_Var_Ref)) {
                            إذا (vrefp->varp() != loopVarp) nodep->user1(سچ);
                        } وإلا {
                            nodep->user1(سچ);
                        }
                    }
                });

                cstmtp->جوڑو("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->جوڑو("ret += ");
                cstmtp->جوڑو(iterateSubtreeReturnEdits(فی_Elem_Exprp));
                cstmtp->جوڑو(";\n");
                cstmtp->جوڑو("ret += \")\";\n");
            } وإلا {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ثابت arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ثابت arrVarp = arrRefp->varp();
                ثابت std::string smtArrayName = arrVarp->name();
                Ast_گرہ_DType* ثابت elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ثابت صحيح elem_چوڑائی = elemDtp->چوڑائی();
                Ast_گرہ_Module* ثابت arrModulep = VN_AS(arrVarp->user2p(), گرہ_Module);
                Ast_گرہ_Module* ثابت genModulep = VN_AS(m_genp->user2p(), گرہ_Module);
                arrVarp->user3(سچ);

                // Add write_var call to init task
                Ast_گرہ_اظہاریہ* ثابت arrVarNamep
                    = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + smtArrayName + "\"", elem_چوڑائی};
                AstCMethodHard* ثابت writeVarCallp
                    = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->جوڑو_Pinsp(جديد AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->جوڑو_Pinsp(
                    جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elem_چوڑائی)});
                writeVarCallp->جوڑو_Pinsp(arrVarNamep);
                ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(خطأ).دوجا;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->جوڑو_Pinsp(جديد AstConst{fl, 1});  // Dimension

                ثابت Randomize_منوال rand_منوال = {.asUQuad = arrVarp->user1()};
                إذا (rand_منوال.ورتدا_منوال) {
                    writeVarCallp->جوڑو_Pinsp(
                        جديد AstConst{fl, AstConst::Unsized64{}, rand_منوال.اشاریہ});
                }
                writeVarCallp->dtype_سیٹ_Void();
                Ast_گرہ_FTask* initTaskp = m_inlineInitTaskp;
                إذا (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.لبھو_Member(arrModulep, "new"), گرہ_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->جوڑو_Stmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ثابت idxFormatp = جديد AstSFormatF{
                    fl, "#x%08x", خطأ, جديد AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ثابت varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_پٹھ(), idxFormatp);

                cstmtp->جوڑو("ret += \" \";\n");
                cstmtp->جوڑو("ret += ");
                cstmtp->جوڑو(varNamep);
                cstmtp->جوڑو(";\n");
            }

            // Build final expression
            إذا (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
                cexprp->dtype_سیٹ_String();
                cexprp->جوڑو("([&]{\nstd::string ret = \"" + پچھان_تت + "\";\n");
                cexprp->جوڑو(جديد AstBegin{fl, m_unique_ناں.get("__VrandBlock"),
                                         جديد AstForeach{fl, headerp, cstmtp}, سچ});
                cexprp->جوڑو("return ret;\n})()");
                nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            } وإلا {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
                cexprp->dtype_سیٹ_String();
                cexprp->جوڑو("([&]{\nstd::string ret;\n");
                cexprp->جوڑو(جديد AstBegin{fl, m_unique_ناں.get("__VrandBlock"),
                                         جديد AstForeach{fl, headerp, cstmtp}, سچ});
                cexprp->جوڑو("return ret.empty() ? \"" + پچھان_تت + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(جديد AstSFormatF{fl, "%s", خطأ, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }

        // We've handled DYN_SIZE already if they should be randomized
        إذا (nodep->طریقہ() == VCMethod::DYN_ناپ || nodep->طریقہ() == VCMethod::ASSOC_ناپ) {
            Ast_گرہ_اظہاریہ* گرہ_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            طالما (VN_IS(گرہ_Exprp, MemberSel)) {
                گرہ_Exprp = VN_AS(گرہ_Exprp, MemberSel)->fromp();
            }
            إذا (VN_IS(گرہ_Exprp, VarRef)) {
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
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        إذا (editFormat(nodep)) أرجع;

        AstFunc* ثابت funcp = VN_CAST(nodep->taskp(), Func);
        إذا (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            أرجع;
        }

        Ast_گرہ_اظہاریہ* ثابت retExprp = کڈھو_Return_اظہاریہ(funcp);
        إذا (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(خطأ);
            إذا (editFormat(nodep)) أرجع;
            أرجع;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ثابت V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), خالی_سوچک, خطأ);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_گرہ_اظہاریہ* inlinedp = retExprp->clone_درخت_خالص(خطأ);
        لكل (ثابت خودکار& tconnect : tconnects) {
            ثابت AstVar* ثابت portp = tconnect.پہلا;
            AstArg* ثابت argp = tconnect.دوجا;
            إذا (!argp || !argp->exprp()) استمر;
            إذا (AstVarRef* ثابت جڑ_Refp = VN_CAST(inlinedp, VarRef)) {
                إذا (جڑ_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_درخت_خالص(خطأ);
                    VL_DO_DANGLING(جڑ_Refp->delete_درخت(), جڑ_Refp);
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
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {}
    فراغ ویکھو(Ast_گرہ_اظہاریہ* nodep) override {
        إذا (editFormat(nodep)) أرجع;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_گرہ* buildDisableSoftCallStmt(Ast_Constraint_اظہاریہ* nodep) {
        std::string varName;
        إذا (ثابت Ast_گرہ_Var_Ref* ثابت vrefp = VN_CAST(nodep->exprp(), گرہ_Var_Ref)) {
            varName = vrefp->name();
        } وإلا إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } وإلا {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), گرہ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            جديد AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtype_سیٹ_Void();
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
    Ast_گرہ* کڈھو_Conditional_Disable_Softs(Ast_گرہ* itemsp, Ast_گرہ_اظہاریہ* outerCondp) {
        Ast_گرہ* hoistListp = خالی_سوچک;
        لكل (Ast_گرہ *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (AstConstraintIf* ثابت ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_سطر* ثابت fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_گرہ_اظہاریہ* ثابت thenCondp
                    = outerCondp ? static_cast<Ast_گرہ_اظہاریہ*>(
                                       جديد AstLogAnd{fl, outerCondp->clone_درخت_خالص(خطأ),
                                                     ifp->condp()->clone_درخت_خالص(خطأ)})
                                 : ifp->condp()->clone_درخت_خالص(خطأ);
                إذا (Ast_گرہ* ثابت thenHoistp
                    = کڈھو_Conditional_Disable_Softs(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_گرہ::جوڑو_اگلا(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_درخت(), thenCondp);
                إذا (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_گرہ_اظہاریہ* ثابت notInnerp
                        = جديد AstLogNot{fl, ifp->condp()->clone_درخت_خالص(خطأ)};
                    Ast_گرہ_اظہاریہ* ثابت elseCondp
                        = outerCondp ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstLogAnd{
                                           fl, outerCondp->clone_درخت_خالص(خطأ), notInnerp})
                                     : notInnerp;
                    إذا (Ast_گرہ* ثابت elseHoistp
                        = کڈھو_Conditional_Disable_Softs(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_گرہ::جوڑو_اگلا(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_درخت(), elseCondp);
                }
            } وإلا إذا (Ast_Constraint_اظہاریہ* ثابت exprp = VN_CAST(stmtp, Constraint_اظہاریہ)) {
                إذا (exprp->isDisableSoft() && outerCondp) {
                    File_سطر* ثابت fl = exprp->fileline();
                    AstIf* ثابت hoistIfp = جديد AstIf{fl, outerCondp->clone_درخت_خالص(خطأ),
                                                      buildDisableSoftCallStmt(exprp), خالی_سوچک};
                    hoistListp = hoistListp ? Ast_گرہ::جوڑو_اگلا(hoistListp,
                                                               static_cast<Ast_گرہ*>(hoistIfp))
                                            : static_cast<Ast_گرہ*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_اظہاریہ* ثابت truep
                        = جديد Ast_Constraint_اظہاریہ{fl, جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}};
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
    واضح Constraint_اظہاریہ_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_گرہ* nodep,
                                   Ast_گرہ_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* rand_منوال_Varp, std::سیٹ<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3_Unique_ناں& unique_ناں,
                                   Ast_گرہ_FTask* memberselInitTaskp = خالی_سوچک,
                                   VInsertion_سیٹ<AstVar*>* ناپ_Constrained_Arraysp = خالی_سوچک,
                                   Ast_گرہ_FTask* prepareConstrainedArraysp = خالی_سوچک)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_rand_منوال_Varp{rand_منوال_Varp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_ناپ_Constrained_Arraysp{ناپ_Constrained_Arraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_unique_ناں{unique_ناں} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        إذا (Ast_گرہ* ثابت hoistListp
            = nodep ? کڈھو_Conditional_Disable_Softs(nodep, خالی_سوچک) : خالی_سوچک) {
            nodep->جوڑو_اگلا(hoistListp);
        }
        iterate_And_اگلا_Null(nodep);
    }
};

تعداد صنف Capture_منوال : uint8_t {
    CAP_NO = 0x0,
    CAP_قدر = 0x01,
    CAP_THIS = 0x02,
    CAP_F_سیٹ_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
Capture_منوال عامل|(Capture_منوال a, Capture_منوال b) {
    أرجع static_cast<Capture_منوال>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
Capture_منوال عامل&(Capture_منوال a, Capture_منوال b) {
    أرجع static_cast<Capture_منوال>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
Capture_منوال منوال(Capture_منوال a) { أرجع a & static_cast<Capture_منوال>(0x3); }
منطقي hasFlags(Capture_منوال a, Capture_منوال flags) {
    أرجع ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

صنف CaptureVisitor final : عام VNVisitor {
    AstArg* m_argsp = خالی_سوچک;  // Original references turned into arguments
    Ast_گرہ_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ثابت AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::سیٹ<Ast_گرہ*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = خالی_سوچک;  // Variable for outer context's object, if necessary
    منطقي m_staticContext = خطأ;  // True when capturing from a static function context

    // METHODS

    منطقي captureVariable(File_سطر* ثابت fileline, Ast_گرہ_Var_Ref* varrefp, AstVar*& varp) {
        خودکار it = m_varCloneMap.لبھو(varrefp->varp());
        إذا (it == m_varCloneMap.سرا()) {
            AstVar* ثابت newVarp = varrefp->varp()->clone_درخت(خطأ);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(سچ);
            newVarp->direction(VDirection::داخلہ);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            أرجع سچ;
        }
        varp = it->دوجا;
        أرجع خطأ;
    }

    سانچا <قسم_ناں T_گرہ>
    فراغ fixupClassOrPackage(Ast_گرہ* memberp, T_گرہ refp) {
        Ast_گرہ_Module* ثابت declClassp = VN_AS(memberp->user2p(), گرہ_Module);
        إذا (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    سانچا <قسم_ناں T_گرہ>
    منطقي is_Reference_منزل_Inner_Member(T_گرہ nodep) {
        أرجع VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_سطر* fl) {
        إذا (!m_thisp) {
            AstClassRefDType* ثابت refDTypep
                = جديد AstClassRefDType{fl, VN_AS(m_callerp, Class), خالی_سوچک};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(refDTypep);
            m_thisp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(سچ);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::داخلہ);
            m_argsp = Ast_گرہ::جوڑو_اگلا(m_argsp, جديد AstArg{fl, "", جديد AstThisRef{fl, refDTypep}});
        }
        أرجع m_thisp;
    }

    AstVar* getVar(AstVar* ثابت varp) ثابت {
        ثابت خودکار it = m_varCloneMap.لبھو(varp);
        إذا (it == m_varCloneMap.سرا()) أرجع خالی_سوچک;
        إذا (it->دوجا->isStdRandomizeArg()) أرجع خالی_سوچک;
        أرجع it->دوجا;
    }

    Capture_منوال get_Var_Ref_Capture_منوال(Ast_گرہ_Var_Ref* varRefp) {
        Ast_گرہ_Module* ثابت varModp = VN_AS(varRefp->varp()->user2p(), گرہ_Module);
        AstClass* ثابت varClassp = VN_CAST(varModp, Class);
        AstClass* ثابت callerClassp = VN_CAST(m_callerp, Class);

        ثابت منطقي callerIsClass = callerClassp;
        ثابت منطقي refIsXref = VN_IS(varRefp, VarXRef);
        ثابت منطقي varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ثابت منطقي varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ثابت منطقي var_Is_میدان_دا_Caller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ثابت منطقي varIsParam = varRefp->varp()->isParam();
        ثابت منطقي varIsConstraintIterator
            = VN_IS(varRefp->varp()->پہلا_Abovep(), Foreach_سرخی)
              && VN_IS(varRefp->varp()->پہلا_Abovep()->پہلا_Abovep(), ConstraintForeach);
        إذا (refIsXref) أرجع Capture_منوال::CAP_قدر | Capture_منوال::CAP_F_XREF;
        إذا (varIsConstraintIterator) أرجع Capture_منوال::CAP_NO;
        إذا (varIsFuncLocal && varHasAutomaticLifetime) أرجع Capture_منوال::CAP_قدر;
        إذا (varIsParam) أرجع Capture_منوال::CAP_قدر;
        // Static var in function (will not be inlined, because it's in class)
        إذا (callerIsClass && varIsFuncLocal) أرجع Capture_منوال::CAP_قدر;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        إذا (m_staticContext && callerIsClass && var_Is_میدان_دا_Caller
            && varRefp->varp()->lifetime().isStatic())
            أرجع Capture_منوال::CAP_NO;
        إذا (callerIsClass && var_Is_میدان_دا_Caller) أرجع Capture_منوال::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        إذا (callerIsClass && varClassp && !var_Is_میدان_دا_Caller
            && varRefp->varp()->lifetime().isStatic())
            أرجع Capture_منوال::CAP_قدر;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        أرجع Capture_منوال::CAP_قدر;
    }

    فراغ capture_Ref_By_قدر(Ast_گرہ_Var_Ref* nodep, Capture_منوال cap_منوال_Flags) {
        AstVar* newVarp;
        ثابت منطقي newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_گرہ_Var_Ref* ثابت newVarRefp = newCapture ? nodep->clone_درخت(خطأ) : خالی_سوچک;
        إذا (!hasFlags(cap_منوال_Flags, Capture_منوال::CAP_F_سیٹ_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(خالی_سوچک);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        إذا (!newCapture) أرجع;
        إذا (hasFlags(cap_منوال_Flags, Capture_منوال::CAP_F_XREF)) {
            AstVarRef* ثابت notXVarRefp
                = جديد AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_گرہ::جوڑو_اگلا(m_argsp, جديد AstArg{nodep->fileline(), "", newVarRefp});
    }

    فراغ captureRefByThis(Ast_گرہ_Var_Ref* nodep, Capture_منوال /*capModeFlags*/) {
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
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        إذا (m_ignore.گنتی(nodep)) أرجع;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        إذا (nodep->varp()->isStdRandomizeArg()) أرجع;
        ثابت Capture_منوال cap_منوال = get_Var_Ref_Capture_منوال(nodep);
        إذا (منوال(cap_منوال) == Capture_منوال::CAP_NO) أرجع;
        إذا (منوال(cap_منوال) == Capture_منوال::CAP_قدر) capture_Ref_By_قدر(nodep, cap_منوال);
        إذا (منوال(cap_منوال) == Capture_منوال::CAP_THIS) captureRefByThis(nodep, cap_منوال);
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        إذا (m_ignore.گنتی(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        إذا (VN_IS(nodep, طریقہ_Call) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            أرجع;
        }
        AstClass* ثابت classp = VN_CAST(nodep->taskp()->user2p(), Class);
        إذا ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ثابت argsp = nodep->argsp();
            إذا (argsp) argsp->unlink_Fr_پٹھ();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ثابت thisp = importThisp(nodep->fileline());
            AstVarRef* ثابت thisRefp = جديد AstVarRef{
                nodep->fileline(), thisp, nodep->is_خالص() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            Ast_طریقہ_Call* ثابت طریقہ_Callp
                = جديد Ast_طریقہ_Call{nodep->fileline(), thisRefp, thisp->name(), argsp};
            طریقہ_Callp->taskp(nodep->taskp());
            طریقہ_Callp->dtypep(nodep->dtypep());
            nodep->replaceWith(طریقہ_Callp);
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
            m_ignore.emplace(طریقہ_Callp);
        }
    }
    فراغ ویکھو(AstMemberSel* nodep) override {
        إذا (!is_Reference_منزل_Inner_Member(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        AstVarRef* ثابت varRefp
            = جديد AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    فراغ ویکھو(Ast_طریقہ_Call* nodep) override {
        إذا (!is_Reference_منزل_Inner_Member(nodep) || m_ignore.گنتی(nodep)) {
            iterateChildren(nodep);
            أرجع;
        }
        AstArg* ثابت argsp = nodep->argsp();
        إذا (argsp) argsp->unlink_Fr_پٹھ_With_اگلا();
        Ast_گرہ_FTask_Ref* taskRefp = خالی_سوچک;
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
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    واضح CaptureVisitor(Ast_گرہ* ثابت nodep, Ast_گرہ_Module* callerp, AstClass* ثابت targetp,
                            منطقي staticContext = خطأ)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterate_And_اگلا_Null(nodep);
    }

    // PUBLIC METHODS

    AstArg* get_دلیلاں() ثابت { أرجع m_argsp; }
    AstVar* getThisp() ثابت { أرجع m_thisp; }

    فراغ جوڑو_Function_Arguments(Ast_گرہ_FTask* funcp) ثابت {
        لكل (AstArg* argp = get_دلیلاں(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            إذا (Ast_گرہ_Var_Ref* varrefp = VN_CAST(argp->exprp(), گرہ_Var_Ref)) {
                إذا ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(خالی_سوچک);
                }
                funcp->جوڑو_Stmtsp(getVar(varrefp->varp()));
            } وإلا {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->جوڑو_Stmtsp(m_thisp);
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
    ثابت VNUser3_وچ_Use m_inuser3;
    ثابت VNUser4_وچ_Use m_inuser4;

    // STATE
    V3_Unique_ناں m_inline_Unique_ناں;  // For generating unique function names
    V3_Unique_ناں m_rand_Array_Unique_ناں{"__Vrandarr"};  // Random array loop indices
    V3_Unique_ناں m_منوال_Unique_ناں{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3_Unique_ناں m_inlineUniqueStdName{"__VStdrand"};
    V3_Unique_ناں m_unique_ناں;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    std::unordered_map<Ast_گرہ_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_گرہ_FTask* m_ftaskp = خالی_سوچک;  // Current function/task
    Ast_گرہ_Stmt* m_stmtp = خالی_سوچک;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = خالی_سوچک;  // Dynamic array type (for rand mode)
    صحيح m_rand_Case_عدد = 0;  // Randcase number within a module for var naming
    صحيح m_dist_عدد = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = خالی_سوچک;  // Current constraint
    std::سیٹ<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertion_سیٹ<AstVar*>> m_ناپ_Constrained_Arrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_static_Constraint_منوال_Vars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_static_Rand_منوال_Vars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<منطقي, منطقي>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ساكن منطقي is_Similar_گرہ(ثابت Ast_گرہ_اظہاریہ* with_اظہاریہ, ثابت Ast_گرہ_اظہاریہ* arg_اظہاریہ) {
        // VarRef: compare variable pointers
        إذا (VN_IS(arg_اظہاریہ, VarRef) && VN_IS(with_اظہاریہ, VarRef)) {
            أرجع VN_AS(with_اظہاریہ, VarRef)->varp() == VN_AS(arg_اظہاریہ, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        إذا (VN_IS(arg_اظہاریہ, MemberSel) && VN_IS(with_اظہاریہ, MemberSel)) {
            ثابت AstMemberSel* ثابت withMSp = VN_AS(with_اظہاریہ, MemberSel);
            ثابت AstMemberSel* ثابت argMSp = VN_AS(arg_اظہاریہ, MemberSel);
            إذا (withMSp->varp() != argMSp->varp()) أرجع خطأ;
            // Recursively compare the base object expression
            أرجع is_Similar_گرہ(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        إذا (VN_IS(arg_اظہاریہ, ArraySel) && VN_IS(with_اظہاریہ, ArraySel)) {
            ثابت AstArraySel* ثابت withASp = VN_AS(with_اظہاریہ, ArraySel);
            ثابت AstArraySel* ثابت argASp = VN_AS(arg_اظہاریہ, ArraySel);
            // Before checking Sel type index, check if both are Const
            إذا (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ثابت AstConst* ثابت withIdxp = VN_AS(withASp->bitp(), Const);
                ثابت AstConst* ثابت argIdxp = VN_AS(argASp->bitp(), Const);
                أرجع is_Similar_گرہ(withASp->fromp(), argASp->fromp())
                       && withIdxp->عدد().isCaseEq(argIdxp->عدد());
            }
            // Index must be Sel type, extract VarRef using fromp()
            إذا (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) أرجع خطأ;
            ثابت Ast_گرہ_اظہاریہ* ثابت withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ثابت Ast_گرہ_اظہاریہ* ثابت argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            أرجع is_Similar_گرہ(withASp->fromp(), argASp->fromp())
                   && is_Similar_گرہ(withIdxp, argIdxp);
        }
        أرجع خطأ;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ساكن فراغ expandUniqueElementList(Ast_گرہ* itemsp) {
        Ast_گرہ* itemp = itemsp;
        طالما (itemp) {
            Ast_گرہ* ثابت nextp = itemp->nextp();
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

            std::سمتیہ<Ast_گرہ_اظہاریہ*> اظہاریہ_اشیاء;
            منطقي hasArrayVarRef = خطأ;
            لكل (Ast_گرہ* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                إذا (ثابت AstVarRef* ثابت vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ثابت Ast_گرہ_DType* ثابت dtypep = vrp->varp()->dtypep()->skipRefp();
                    إذا (VN_IS(dtypep, گرہ_Array_DType) || VN_IS(dtypep, قطار_DType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = سچ;
                        استمر;
                    }
                }
                اظہاریہ_اشیاء.ڈالو_پٹھ(VN_AS(rp, گرہ_اظہاریہ));
            }

            إذا (اظہاریہ_اشیاء.ناپ() >= 2) {
                File_سطر* ثابت fl = uniquep->fileline();
                لكل (ناپ_t i = 0; i < اظہاریہ_اشیاء.ناپ(); i++) {
                    لكل (ناپ_t j = i + 1; j < اظہاریہ_اشیاء.ناپ(); j++) {
                        Ast_گرہ_اظہاریہ* ثابت lhsp = اظہاریہ_اشیاء[i]->clone_درخت(خطأ);
                        Ast_گرہ_اظہاریہ* ثابت rhsp = اظہاریہ_اشیاء[j]->clone_درخت(خطأ);
                        AstNeq* ثابت neqp = جديد AstNeq{fl, lhsp, rhsp};
                        neqp->user1(سچ);
                        Ast_Constraint_اظہاریہ* ثابت cexprp = جديد Ast_Constraint_اظہاریہ{fl, neqp};
                        uniquep->جوڑو_اگلا_ایتھے(cexprp);
                    }
                }
                إذا (!hasArrayVarRef) {
                    uniquep->unlink_Fr_پٹھ();
                    VL_DO_DANGLING(uniquep->delete_درخت(), uniquep);
                }
            } وإلا إذا (اظہاریہ_اشیاء.ناپ() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlink_Fr_پٹھ();
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
                                        classp->لبھو_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->جوڑو_Membersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_گرہ_Module* ثابت modp) {
        خودکار it = m_stdMap.لبھو(modp);
        إذا (it == m_stdMap.سرا()) {
            AstVar* ثابت stdgenp
                = جديد AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->لبھو_Basic_DType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3_Error_کوڈ::IMPURE, سچ);
            modp->جوڑو_Stmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            أرجع stdgenp;
        }
        أرجع it->دوجا;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ثابت classp) {
        ساكن ثابت حرف* ثابت name = "__Vprepare_constrained_arrays";
        AstTask* ثابت taskp = جديد AstTask{classp->fileline(), name, خالی_سوچک};
        taskp->class_طریقہ(سچ);
        classp->جوڑو_Membersp(taskp);
        m_memberMap.داخل_کرو(classp, taskp);
        أرجع taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ثابت classp) {
        ساكن constexpr ثابت حرف* ثابت name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.لبھو_Member(classp, name), Task);
        إذا (resizeTaskp) أرجع resizeTaskp;
        resizeTaskp = جديد AstTask{classp->fileline(), name, خالی_سوچک};
        resizeTaskp->class_طریقہ(سچ);
        resizeTaskp->isVirtual(سچ);
        classp->جوڑو_Membersp(resizeTaskp);
        m_memberMap.داخل_کرو(classp, resizeTaskp);
        أرجع resizeTaskp;
    }
    AstVar* get_Create_Rand_منوال_Var(AstClass* ثابت classp) {
        إذا (classp->user2p()) أرجع VN_AS(classp->user2p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Rand_منوال_Var(extendsp->classp());
        }
        AstVar* ثابت rand_منوال_Varp = create_منوال_Var(classp, "__Vrandmode");
        classp->user2p(rand_منوال_Varp);
        أرجع rand_منوال_Varp;
    }
    AstVar* get_Create_Constraint_منوال_Var(AstClass* ثابت classp) {
        إذا (classp->user4p()) أرجع VN_AS(classp->user4p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Constraint_منوال_Var(extendsp->classp());
        }
        AstVar* ثابت constraint_منوال_Varp = create_منوال_Var(classp, "__Vconstraintmode");
        classp->user4p(constraint_منوال_Varp);
        أرجع constraint_منوال_Varp;
    }
    ساكن AstVar* get_Constraint_منوال_Var(AstClass* ثابت classp) {
        إذا (classp->user4p()) أرجع VN_AS(classp->user4p(), Var);
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Constraint_منوال_Var(extendsp->classp());
        }
        أرجع خالی_سوچک;
    }
    AstVar* get_Create_Static_Constraint_منوال_Var(AstClass* ثابت classp) {
        خودکار it = m_static_Constraint_منوال_Vars.لبھو(classp);
        إذا (it != m_static_Constraint_منوال_Vars.سرا()) أرجع it->دوجا;
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Static_Constraint_منوال_Var(extendsp->classp());
        }
        AstVar* ثابت static_منوال_Varp = create_Static_منوال_Var(classp, "__Vstaticconstraintmode");
        m_static_Constraint_منوال_Vars[classp] = static_منوال_Varp;
        أرجع static_منوال_Varp;
    }
    AstVar* get_Static_Constraint_منوال_Var(AstClass* ثابت classp) {
        خودکار it = m_static_Constraint_منوال_Vars.لبھو(classp);
        إذا (it != m_static_Constraint_منوال_Vars.سرا()) أرجع it->دوجا;
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Static_Constraint_منوال_Var(extendsp->classp());
        }
        أرجع خالی_سوچک;
    }
    AstVar* get_Create_Static_Rand_منوال_Var(AstClass* ثابت classp) {
        إذا (m_static_Rand_منوال_Vars.گنتی(classp)) أرجع m_static_Rand_منوال_Vars[classp];
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Create_Static_Rand_منوال_Var(extendsp->classp());
        }
        AstVar* ثابت static_منوال_Varp = create_Static_منوال_Var(classp, "__Vstaticrandmode");
        m_static_Rand_منوال_Vars[classp] = static_منوال_Varp;
        أرجع static_منوال_Varp;
    }
    AstVar* get_Static_Rand_منوال_Var(AstClass* ثابت classp) {
        إذا (m_static_Rand_منوال_Vars.گنتی(classp)) أرجع m_static_Rand_منوال_Vars[classp];
        إذا (AstClassExtends* ثابت extendsp = classp->extendsp()) {
            أرجع get_Static_Rand_منوال_Var(extendsp->classp());
        }
        أرجع خالی_سوچک;
    }
    AstVar* create_منوال_Var(AstClass* ثابت classp, ثابت حرف* ثابت name) {
        File_سطر* ثابت fl = classp->fileline();
        إذا (!m_dynarrayDtp) {
            m_dynarrayDtp = جديد AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->لبھو_ثنائی_ہندسہ_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ثابت منوال_Varp = جديد AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        منوال_Varp->user2p(classp);
        classp->جوڑو_Stmtsp(منوال_Varp);
        أرجع منوال_Varp;
    }
    AstVar* create_Static_منوال_Var(AstClass* ثابت classp, ثابت حرف* ثابت name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_سطر* ثابت fl = classp->fileline();
        إذا (!m_dynarrayDtp) {
            m_dynarrayDtp = جديد AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->لبھو_ثنائی_ہندسہ_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(m_dynarrayDtp);
        }
        AstVar* ثابت منوال_Varp = جديد AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        منوال_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        منوال_Varp->user2p(classp);
        classp->جوڑو_Stmtsp(منوال_Varp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.داخل_کرو(classp, منوال_Varp);
        أرجع منوال_Varp;
    }
    ساكن فراغ جوڑو_سیٹ_Rand_منوال(Ast_گرہ_FTask* ثابت ftaskp, AstVar* ثابت genp,
                               AstVar* ثابت rand_منوال_Varp) {
        File_سطر* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت سیٹ_Rand_Modep = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, VN_AS(genp->user2p(), گرہ_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_سیٹ_RANDMODE,
            جديد AstVarRef{fl, VN_AS(rand_منوال_Varp->user2p(), گرہ_Module), rand_منوال_Varp,
                          VAccess::READ}};
        سیٹ_Rand_Modep->dtype_سیٹ_Void();
        ftaskp->جوڑو_Stmtsp(سیٹ_Rand_Modep->makeStmt());
    }
    ساكن فراغ جوڑو_سیٹ_Static_Rand_منوال(Ast_گرہ_FTask* ثابت ftaskp, AstVar* ثابت genp,
                                     AstVar* ثابت static_Rand_منوال_Varp) {
        File_سطر* ثابت fl = ftaskp->fileline();
        AstCMethodHard* ثابت setp = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, VN_AS(genp->user2p(), گرہ_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_سیٹ_STATIC_RANDMODE,
            جديد AstVarRef{fl, VN_AS(static_Rand_منوال_Varp->user2p(), گرہ_Module), static_Rand_منوال_Varp,
                          VAccess::READ}};
        setp->dtype_سیٹ_Void();
        ftaskp->جوڑو_Stmtsp(setp->makeStmt());
    }
    فراغ createRandomizeClassVars(AstNetlist* ثابت netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> جڑ_Static_Rand_منوال_گنتی;
        netlistp->foreach([هذا, &جڑ_Static_Rand_منوال_گنتی](AstClass* ثابت classp) {
            منطقي hasConstraints = خطأ;
            uint32_t rand_منوال_گنتی = 0;
            uint32_t static_Rand_منوال_گنتی = 0;
            uint32_t constraint_منوال_گنتی = 0;
            uint32_t static_Constraint_منوال_گنتی = 0;
            classp->foreachMember([&](AstClass*, Ast_گرہ* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                إذا (AstConstraint* ثابت constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = سچ;
                    Randomize_منوال constraint_منوال = {.asUQuad = memberp->user1()};
                    إذا (!constraint_منوال.ورتدا_منوال) أرجع;
                    إذا (constraint_منوال.اشاریہ == 0) {
                        // Use separate index counters for static vs non-static constraints
                        إذا (constrp->isStatic()) {
                            constraint_منوال.اشاریہ = static_Constraint_منوال_گنتی++;
                        } وإلا {
                            constraint_منوال.اشاریہ = constraint_منوال_گنتی++;
                        }
                        memberp->user1(constraint_منوال.asUQuad);
                    } وإلا {
                        إذا (constrp->isStatic()) {
                            static_Constraint_منوال_گنتی = constraint_منوال.اشاریہ + 1;
                        } وإلا {
                            constraint_منوال_گنتی = constraint_منوال.اشاریہ + 1;
                        }
                    }
                } وإلا إذا (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
                    Randomize_منوال rand_منوال = {.asUQuad = memberp->user1()};
                    إذا (!rand_منوال.ورتدا_منوال) أرجع;
                    ثابت منطقي isStaticVar = varp->lifetime().isStatic();
                    إذا (rand_منوال.اشاریہ == 0) {
                        إذا (isStaticVar) {
                            rand_منوال.اشاریہ = static_Rand_منوال_گنتی++;
                        } وإلا {
                            rand_منوال.اشاریہ = rand_منوال_گنتی++;
                        }
                        memberp->user1(rand_منوال.asUQuad);
                    } وإلا {
                        إذا (isStaticVar) {
                            static_Rand_منوال_گنتی = rand_منوال.اشاریہ + 1;
                        } وإلا {
                            rand_منوال_گنتی = rand_منوال.اشاریہ + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<فراغ(AstClass*)> لبھو_Sub_Obj_Rand_Modes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_گرہ* subMemberp) {
                        إذا (AstVar* ثابت subVarp = VN_CAST(subMemberp, Var)) {
                            ثابت Randomize_منوال rm = {.asUQuad = subVarp->user1()};
                            إذا (!rm.ورتدا_منوال) أرجع;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            إذا (subVarp->lifetime().isStatic()) أرجع;
                            ثابت uint32_t needed = rm.اشاریہ + 1;
                            إذا (needed > rand_منوال_گنتی) rand_منوال_گنتی = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    إذا (!memberVarp->globalConstrained()) أرجع;
                    ثابت Ast_گرہ_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
                    ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
                    إذا (!classRefp) أرجع;
                    لبھو_Sub_Obj_Rand_Modes(classRefp->classp());
                });
            }
            إذا (hasConstraints) createRandomGenerator(classp);
            إذا (rand_منوال_گنتی > 0) {
                AstVar* ثابت rand_منوال_Varp = get_Create_Rand_منوال_Var(classp);
                make_منوال_Init(rand_منوال_Varp, classp, rand_منوال_گنتی);
            }
            إذا (constraint_منوال_گنتی > 0) {
                AstVar* ثابت constraint_منوال_Varp = get_Create_Constraint_منوال_Var(classp);
                make_منوال_Init(constraint_منوال_Varp, classp, constraint_منوال_گنتی);
            }
            إذا (static_Constraint_منوال_گنتی > 0) {
                AstVar* ثابت static_Constraint_منوال_Varp = get_Create_Static_Constraint_منوال_Var(classp);
                make_Static_منوال_Init(static_Constraint_منوال_Varp, classp, static_Constraint_منوال_گنتی);
            }
            إذا (static_Rand_منوال_گنتی > 0) {
                get_Create_Static_Rand_منوال_Var(classp);
                AstClass* rootp = classp;
                طالما (AstClassExtends* ثابت ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& تھاں_2 = جڑ_Static_Rand_منوال_گنتی[rootp];
                إذا (static_Rand_منوال_گنتی > تھاں_2) تھاں_2 = static_Rand_منوال_گنتی;
            }
        });
        لكل (ثابت خودکار& kv : جڑ_Static_Rand_منوال_گنتی) emit_جڑ_Static_منوال_Init(kv.پہلا, kv.دوجا);
    }
    فراغ emit_جڑ_Static_منوال_Init(AstClass* ثابت rootp, ثابت uint32_t گنتی) {
        AstVar* ثابت static_Rand_منوال_Varp = m_static_Rand_منوال_Vars[rootp];
        UASSERT_OBJ(static_Rand_منوال_Varp, rootp, "Root must have a static rand-mode var");
        make_Static_منوال_Init(static_Rand_منوال_Varp, rootp, گنتی);
    }
    فراغ make_منوال_Init(AstVar* منوال_Varp, AstClass* classp, uint32_t منوال_گنتی) {
        Ast_گرہ_Module* ثابت منوال_Var_Modp = VN_AS(منوال_Varp->user2p(), گرہ_Module);
        File_سطر* ثابت fl = منوال_Varp->fileline();
        AstVar* ثابت old_ناپ_Varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, m_unique_ناں.get("__VoldSize"), منوال_Varp->لبھو_UInt32_DType()};
        old_ناپ_Varp->funcLocal(سچ);
        old_ناپ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت oldSizep
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, منوال_Var_Modp, منوال_Varp, VAccess::READ},
                                 VCMethod::DYN_ناپ, خالی_سوچک};
        oldSizep->dtype_سیٹ_UInt32();
        AstCMethodHard* ثابت dynarrayNewp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, منوال_Var_Modp, منوال_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جديد AstConst{fl, منوال_گنتی}};
        dynarrayNewp->dtype_سیٹ_Void();
        Ast_گرہ_FTask* ثابت ctorNewp = VN_AS(m_memberMap.لبھو_Member(classp, "new"), گرہ_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_گرہ* ثابت initFirstp = old_ناپ_Varp;
        initFirstp->جوڑو_اگلا(
            جديد Ast_سونپو{fl, جديد AstVarRef{fl, old_ناپ_Varp, VAccess::WRITE}, oldSizep});
        initFirstp->جوڑو_اگلا(dynarrayNewp->makeStmt());
        initFirstp->جوڑو_اگلا(make_منوال_سیٹ_Loop(
            fl, جديد AstVarRef{fl, منوال_Var_Modp, منوال_Varp, VAccess::WRITE}, جديد AstConst{fl, 1},
            سچ, جديد AstVarRef{fl, old_ناپ_Varp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        لكل (Ast_گرہ* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (!VN_IS(stmtp, گرہ_Stmt)) استمر;  // Skip var declarations
            إذا (ثابت Ast_Stmt_اظہاریہ* ثابت sep = VN_CAST(stmtp, Stmt_اظہاریہ)) {
                إذا (VN_IS(sep->exprp(), New)) استمر;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->جوڑو_ایتھے_This_As_اگلا(initFirstp);
            أرجع;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->جوڑو_Stmtsp(initFirstp);
    }
    فراغ make_Static_منوال_Init(AstVar* منوال_Varp, AstClass* classp, uint32_t منوال_گنتی) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_گرہ_Module* ثابت منوال_Var_Modp = VN_AS(منوال_Varp->user2p(), گرہ_Module);
        File_سطر* ثابت fl = منوال_Varp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ثابت sizep
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, منوال_Var_Modp, منوال_Varp, VAccess::READ},
                                 VCMethod::DYN_ناپ, خالی_سوچک};
        sizep->dtype_سیٹ_UInt32();
        AstEq* ثابت condp = جديد AstEq{fl, sizep, جديد AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ثابت dynarrayNewp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, منوال_Var_Modp, منوال_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, جديد AstConst{fl, منوال_گنتی}};
        dynarrayNewp->dtype_سیٹ_Void();
        Ast_گرہ* ثابت thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->جوڑو_اگلا(
            make_منوال_سیٹ_Loop(fl, جديد AstVarRef{fl, منوال_Var_Modp, منوال_Varp, VAccess::WRITE},
                            جديد AstConst{fl, 1}, سچ));

        // Build the if statement
        AstIf* ثابت ifp = جديد AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_گرہ_FTask* ثابت newp = VN_AS(m_memberMap.لبھو_Member(classp, "new"), گرہ_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->جوڑو_Stmtsp(ifp);
    }
    Ast_گرہ* make_منوال_سیٹ_Loop(File_سطر* ثابت fl, Ast_گرہ_اظہاریہ* ثابت lhsp, Ast_گرہ_اظہاریہ* ثابت rhsp,
                             منطقي وچ_Task, Ast_گرہ_اظہاریہ* ثابت startp = خالی_سوچک) {
        AstVar* ثابت iterVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->لبھو_UInt32_DType()};
        iterVarp->funcLocal(وچ_Task);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ثابت sizep = جديد AstCMethodHard{fl, lhsp, VCMethod::DYN_ناپ, خالی_سوچک};
        sizep->dtype_سیٹ_UInt32();
        AstCMethodHard* ثابت setp
            = جديد AstCMethodHard{fl, lhsp->clone_درخت(خطأ), VCMethod::ARRAY_AT_WRITE,
                                 جديد AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtype_سیٹ_UInt32();
        Ast_گرہ* ثابت stmtsp = iterVarp;
        stmtsp->جوڑو_اگلا(جديد Ast_سونپو{fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : جديد AstConst{fl, 0}});

        AstLoop* ثابت loopp = جديد AstLoop{fl};
        stmtsp->جوڑو_اگلا(loopp);
        loopp->جوڑو_Stmtsp(جديد AstLoopTest{
            fl, loopp, جديد AstLt{fl, جديد AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, setp, rhsp});
        loopp->جوڑو_Stmtsp(جديد Ast_سونپو{
            fl, جديد AstVarRef{fl, iterVarp, VAccess::WRITE},
            جديد Ast_جوڑو{fl, جديد AstConst{fl, 1}, جديد AstVarRef{fl, iterVarp, VAccess::READ}}});
        أرجع جديد AstBegin{fl, m_unique_ناں.get("__VrandBlock"), stmtsp, سچ};
    }
    فراغ جوڑو_عارضی_Varsp(AstVar* varsp) {
        لكل (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (m_ftaskp) {
                varp->funcLocal(سچ);
            } وإلا {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        إذا (!m_ftaskp) {
            m_modp->جوڑو_Stmtsp(varsp);
        } وإلا {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_گرہ_Stmt* wrap_If_Rand_منوال(AstClass* classp, AstVar* ثابت varp, Ast_گرہ_Stmt* stmtp) {
        ثابت Randomize_منوال rmode = {.asUQuad = varp->user1()};
        AstVar* ثابت منوال_Varp = varp->lifetime().isStatic() ? get_Static_Rand_منوال_Var(classp)
                                                             : get_Rand_منوال_Var_From_Class(classp);
        أرجع VN_AS(wrap_If_منوال(rmode, منوال_Varp, stmtp), گرہ_Stmt);
    }
    Ast_گرہ* wrap_If_Constraint_منوال(AstClass* classp, AstConstraint* ثابت constrp, Ast_گرہ* stmtp) {
        ثابت Randomize_منوال rmode = {.asUQuad = constrp->user1()};
        AstVar* ثابت منوال_Varp = constrp->isStatic() ? get_Static_Constraint_منوال_Var(classp)
                                                     : get_Constraint_منوال_Var(classp);
        أرجع wrap_If_منوال(rmode, منوال_Varp, stmtp);
    }
    ساكن Ast_گرہ* wrap_If_منوال(ثابت Randomize_منوال منوال, AstVar* منوال_Varp, Ast_گرہ* stmtp) {
        File_سطر* ثابت fl = stmtp->fileline();
        إذا (منوال.ورتدا_منوال) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_گرہ_Module* ثابت modp = VN_AS(منوال_Varp->user2p(), گرہ_Module);
            AstCMethodHard* ثابت atp
                = جديد AstCMethodHard{fl, جديد AstVarRef{fl, modp, منوال_Varp, VAccess::READ},
                                     VCMethod::ARRAY_AT, جديد AstConst{fl, منوال.اشاریہ}};
            atp->dtype_سیٹ_UInt32();
            أرجع جديد AstIf{fl, atp, stmtp};
        }
        أرجع stmtp;
    }
    AstVarRef* enum_قدر_Tab_Refp(AstEnumDType* ثابت nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        إذا (nodep->user2p()) أرجع VN_AS(nodep->user2p(), VarRef)->clone_درخت(خطأ);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_گرہ_Array_DType* ثابت vardtypep = جديد AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            جديد AstRange{nodep->fileline(), static_cast<صحيح>(nodep->شے_گنتی()) - 1, 0}};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(vardtypep);
        AstConsArray* ثابت consp = جديد AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t اشاریہ = 0;
        لكل (Ast_Enum_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شے)) {
            AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(اشاریہ++, vconstp->clone_درخت(خطأ));
        }
        // Share identical tables via the constant pool
        AstVarRef* ثابت refp = V3ConstPool::لبھو(consp);
        VL_DO_DANGLING(consp->delete_درخت(), consp);  // V3ConstPool::find clones it
        ڈالو_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        أرجع refp->clone_درخت(خطأ);
    }

    AstCDType* لبھو_Vl_Rand_CDType(File_سطر* ثابت fl, uint64_t اشیاء) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ثابت std::string type = AstCDType::type_منزل_Hold(V3_نمبر::log2bQuad(اشیاء) + 1);
        ثابت std::string name = "VlRandC<" + type + ", " + cvt_منزل_Str(اشیاء) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ثابت خودکار pair = m_randcDtypes.emplace(name, خالی_سوچک);
        إذا (pair.دوجا) {
            AstCDType* ثابت newp = جديد AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->جوڑو_Typesp(newp);
            pair.پہلا->دوجا = newp;
        }
        أرجع pair.پہلا->دوجا;
    }

    AstVar* newRandcVarsp(AstVar* ثابت varp, AstClass* ثابت classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        إذا (!varp->isRandC()) أرجع خالی_سوچک;

        uint64_t اشیاء = 0;
        إذا (AstEnumDType* ثابت enumDtp = VN_CAST(varp->dtypep()->skip_Ref_منزل_Enump(), EnumDType)) {
            اشیاء = static_cast<uint64_t>(enumDtp->شے_گنتی());
        } وإلا إذا (AstBasicDType* ثابت basicp = varp->dtypep()->skipRefp()->basicp()) {
            إذا (basicp->چوڑائی() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                إذا (!V3Error::error_گنتی()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->چوڑائی() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                أرجع خالی_سوچک;
            }
            اشیاء = 1ULL << basicp->چوڑائی();
        } وإلا إذا (AstStructDType* ثابت dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            أرجع خالی_سوچک;
        } وإلا {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_گنتی(), varp, "Unexpected randc variable dtype");
            أرجع خالی_سوچک;
        }
        AstCDType* ثابت newdtp = لبھو_Vl_Rand_CDType(varp->fileline(), اشیاء);
        AstVar* ثابت newp
            = جديد AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->is_اندرونی(سچ);
        classp->جوڑو_Stmtsp(newp);
        UINFO(9, "created " << varp);
        أرجع newp;
    }
    Ast_گرہ_Stmt* createArrayForeachLoop(File_سطر* ثابت fl, Ast_گرہ_DType* ثابت dtypep,
                                        Ast_گرہ_اظہاریہ* exprp, AstVar* ثابت outputVarp) {
        Ast_گرہ_DType* عارضی_DTypep = dtypep;
        AstVar* randLoopIndxp = خالی_سوچک;
        خودکار create_Loop_اشاریہ = [&](Ast_گرہ_DType* عارضی_DTypep) {
            إذا (VN_IS(عارضی_DTypep, AssocArrayDType)) {
                أرجع جديد AstVar{fl, VVarType::VAR, m_rand_Array_Unique_ناں.get(""),
                                  VN_AS(عارضی_DTypep, AssocArrayDType)->کلید_DTypep()};
            }
            أرجع جديد AstVar{fl, VVarType::VAR, m_rand_Array_Unique_ناں.get(""),
                              dtypep->لبھو_Basic_DType(VBasicDTypeKwd::UINT32)};
        };
        Ast_گرہ_اظہاریہ* عارضی_Elementp = خالی_سوچک;
        طالما (عارضی_DTypep->isNonPackedArray()) {
            AstVar* ثابت newRandLoopIndxp = create_Loop_اشاریہ(عارضی_DTypep);
            randLoopIndxp = Ast_گرہ::جوڑو_اگلا(randLoopIndxp, newRandLoopIndxp);
            Ast_گرہ_اظہاریہ* ثابت عارضی_Exprp = عارضی_Elementp ? عارضی_Elementp : exprp;
            AstVarRef* ثابت عارضی_Refp = جديد AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            إذا (VN_IS(عارضی_DTypep, DynArrayDType)) {
                عارضی_Elementp
                    = جديد AstCMethodHard{fl, عارضی_Exprp, VCMethod::ARRAY_AT_WRITE, عارضی_Refp};
            } وإلا إذا (VN_IS(عارضی_DTypep, UnpackArrayDType)) {
                Ast_گرہ_Array_DType* ثابت aryDTypep = VN_CAST(عارضی_DTypep, گرہ_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                عارضی_Elementp = جديد AstArraySel{
                    fl, عارضی_Exprp,
                    جديد AstSel{
                        fl,
                        جديد AstSub{fl, عارضی_Refp,
                                   جديد AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        جديد AstConst{fl, 0}, V3_نمبر::log2b(aryDTypep->hi()) + 1}};
            } وإلا إذا (VN_IS(عارضی_DTypep, AssocArrayDType)) {
                عارضی_Elementp = جديد AstAssocSel{fl, عارضی_Exprp, عارضی_Refp};
            } وإلا إذا (VN_IS(عارضی_DTypep, قطار_DType)) {
                عارضی_Elementp
                    = جديد AstCMethodHard{fl, عارضی_Exprp, VCMethod::DYN_AT_WRITE_APPEND, عارضی_Refp};
            }
            عارضی_Elementp->dtypep(عارضی_DTypep->subDTypep());
            عارضی_DTypep = عارضی_DTypep->virtRefDTypep();
        }

        Ast_Foreach_سرخی* ثابت headerp
            = جديد Ast_Foreach_سرخی{fl, exprp->clone_درخت(خطأ), randLoopIndxp};
        Ast_گرہ_Stmt* ثابت randStmtsp = newRandStmtsp(fl, عارضی_Elementp, خالی_سوچک, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        إذا (!عارضی_Elementp->backp()) VL_DO_DANGLING(ڈالو_Deletep(عارضی_Elementp), عارضی_Elementp);
        أرجع جديد AstForeach{fl, headerp, randStmtsp};
    }
    Ast_گرہ_Stmt* newRandStmtsp(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, AstVar* randcVarp,
                               AstVar* ثابت outputVarp, صحيح وتھ = 0,
                               AstMemberDType* memberp = خالی_سوچک) {
        Ast_گرہ_DType* ثابت memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        إذا (ثابت خودکار* ثابت structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_گرہ_Stmt* stmtsp = خالی_سوچک;
            إذا (structDtp->packed()) وتھ += memberp ? memberp->lsb() : 0;
            لكل (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_گرہ_Stmt* randp = خالی_سوچک;
                إذا (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_درخت(خطأ) : exprp, خالی_سوچک,
                                          outputVarp, وتھ, smemberp);
                } وإلا إذا (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = جديد AstStructSel{fl, exprp->clone_درخت(خطأ), smemberp->name()};
                    structSelp->dtypep(smemberp->بچہ_DTypep());
                    إذا (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, خالی_سوچک, outputVarp);
                }
                stmtsp = Ast_گرہ::جوڑو_اگلا_Null(stmtsp, randp);
            }
            أرجع stmtsp;
        } وإلا إذا (ثابت خودکار* ثابت unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            إذا (!unionDtp->packed()) أرجع خالی_سوچک;
            AstMemberDType* ثابت پہلا_Memberp = unionDtp->membersp();
            أرجع newRandStmtsp(fl, exprp, خالی_سوچک, outputVarp, وتھ, پہلا_Memberp);
        } وإلا إذا (ثابت AstClassRefDType* ثابت classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ثابت memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            Ast_طریقہ_Call* ثابت callp = جديد Ast_طریقہ_Call{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            Ast_سونپو* ثابت assignp = جديد Ast_سونپو{
                fl, جديد AstVarRef{fl, outputVarp, VAccess::WRITE},
                جديد AstAnd{fl, جديد AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            أرجع جديد AstIf{
                fl, جديد AstNeq{fl, exprp->clone_درخت(خطأ), جديد AstConst{fl, AstConst::Null{}}},
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
            Ast_گرہ_اظہاریہ* valp;
            إذا (AstEnumDType* ثابت enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ثابت tabRefp = enum_قدر_Tab_Refp(enumDtp);
                Ast_گرہ_اظہاریہ* ثابت randp
                    = new_Rand_قدر(fl, randcVarp, exprp->لبھو_Basic_DType(VBasicDTypeKwd::UINT32));
                Ast_گرہ_اظہاریہ* ثابت moddivp = جديد AstModDiv{
                    fl, randp, جديد AstConst{fl, static_cast<uint32_t>(enumDtp->شے_گنتی())}};
                moddivp->dtypep(enumDtp);
                valp = جديد AstArraySel{fl, tabRefp, moddivp};
            } وإلا {
                valp
                    = new_Rand_قدر(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            Ast_سونپو* assignp
                = جديد Ast_سونپو{fl,
                                جديد AstSel{fl, exprp, وتھ + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->چوڑائی() : exprp->چوڑائی()},
                                valp};
            AstVar* varp = خالی_سوچک;
            exprp->exists([&](ثابت AstVarRef* varrefp) {
                إذا (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                أرجع varp != خالی_سوچک;
            });
            أرجع wrap_If_Rand_منوال(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_گرہ_اظہاریہ* new_Rand_قدر(File_سطر* ثابت fl, AstVar* ثابت randcVarp,
                              Ast_گرہ_DType* ثابت dtypep) {
        إذا (randcVarp) {
            AstCExpr* ثابت cexprp = جديد AstCExpr{fl};
            cexprp->جوڑو(جديد AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->جوڑو(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            أرجع cexprp;
        }

        أرجع جديد AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* لبھو_Pre_Post_Task(AstClass* classp, ثابت string& name) {
        لكل (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : خالی_سوچک) {
            إذا (AstTask* ثابت taskp = VN_CAST(m_memberMap.لبھو_Member(cp, name), Task)) {
                أرجع taskp;
            }
        }
        أرجع خالی_سوچک;
    }
    فراغ جوڑو_Pre_Post_Call(AstClass* ثابت classp, Ast_گرہ_FTask* ثابت funcp, ثابت string& name) {
        إذا (AstTask* ثابت userFuncp = لبھو_Pre_Post_Task(classp, name)) {
            AstTaskRef* ثابت callp = جديد AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->جوڑو_Stmtsp(callp->makeStmt());
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
        إذا (AstTask* ثابت existingp = VN_CAST(m_memberMap.لبھو_Member(classp, name), Task)) {
            أرجع existingp;
        }
        AstTask* ثابت taskp = جديد AstTask{classp->fileline(), name, خالی_سوچک};
        taskp->class_طریقہ(سچ);
        taskp->isVirtual(classp->isExtended());
        classp->جوڑو_Membersp(taskp);
        m_memberMap.داخل_کرو(classp, taskp);
        جوڑو_Pre_Post_Call(classp, taskp, which);
        أرجع taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<منطقي, منطقي> buildPrePostVirtualWrappers(AstClass* ثابت classp) {
        ثابت خودکار cachedIt = m_prePostWrap.لبھو(classp);
        إذا (cachedIt != m_prePostWrap.سرا()) أرجع cachedIt->دوجا;
        std::سمتیہ<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            إذا (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.ڈالو_پٹھ(subp);
        });
        منطقي hasPre = خطأ;
        منطقي hasPost = خطأ;
        لكل (AstClass* ثابت cp : hierp) {
            إذا (لبھو_Pre_Post_Task(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = سچ;
            }
            إذا (لبھو_Pre_Post_Task(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = سچ;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        إذا (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        إذا (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ثابت std::pair<منطقي, منطقي> نتیجہ{hasPre, hasPost};
        m_prePostWrap.emplace(classp, نتیجہ);
        أرجع نتیجہ;
    }
    فراغ جوڑو_Virtual_Pre_Post_Call(AstFunc* ثابت randomizeFuncp, AstClass* ثابت classp,
                               ثابت string& which) {
        File_سطر* ثابت fl = classp->fileline();
        AstTask* ثابت wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ثابت refDTypep = جديد AstClassRefDType{fl, classp, خالی_سوچک};
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(refDTypep);
        Ast_طریقہ_Call* ثابت callp
            = جديد Ast_طریقہ_Call{fl, جديد AstThisRef{fl, refDTypep}, wrapperp->name(), خالی_سوچک};
        callp->taskp(wrapperp);
        callp->dtype_سیٹ_Void();
        randomizeFuncp->جوڑو_Stmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    منطقي classHasRandClassMembers(AstClass* classp) {
        أرجع classp->existsMember([](ثابت AstClass*, ثابت AstVar* varp) {
            إذا (!varp->rand().isRandomizable()) أرجع خطأ;
            ثابت Ast_گرہ_DType* ثابت dtypep = varp->dtypep()->skipRefp();
            أرجع VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    منطقي classOwnsGlobalConstraint(ثابت AstClass* classp) ثابت {
        أرجع classp->existsMember([](ثابت AstClass*, ثابت AstConstraint* constrp) {
            منطقي owns = خطأ;
            constrp->foreach([&](ثابت AstMemberSel* memberSelp) {
                ثابت Ast_گرہ* ثابت rootp
                    = const_cast<AstMemberSel*>(memberSelp)->ادھار_Fromp(سچ);
                إذا (ثابت AstVarRef* ثابت refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ثابت Ast_گرہ_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    طالما (ثابت Ast_گرہ_DType* ثابت subp = dtypep->subDTypep())
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
        AstTask* taskp = VN_CAST(m_memberMap.لبھو_Member(classp, name), Task);
        إذا (taskp) أرجع taskp;
        taskp = جديد AstTask{classp->fileline(), name, خالی_سوچک};
        taskp->class_طریقہ(سچ);
        classp->جوڑو_Membersp(taskp);
        m_memberMap.داخل_کرو(classp, taskp);
        أرجع taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    فراغ populateNestedCallbackTask(AstTask* ثابت callbackTaskp, AstClass* ثابت classp,
                                    ثابت string& cbName) {
        File_سطر* ثابت fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            إذا (!memberVarp->rand().isRandomizable()) أرجع;
            ثابت Ast_گرہ_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
            ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType);
            إذا (!classRefp) أرجع;
            AstClass* ثابت memberClassp = classRefp->classp();
            إذا (memberClassp == classp) أرجع;  // Avoid self-reference

            // Force-visit member class if not yet processed
            إذا (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_گرہ* stmtsp = خالی_سوچک;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            إذا (AstTask* ثابت userFuncp = لبھو_Pre_Post_Task(memberClassp, cbName)) {
                Ast_طریقہ_Call* ثابت callp = جديد Ast_طریقہ_Call{
                    fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtype_سیٹ_Void();
                stmtsp = Ast_گرہ::جوڑو_اگلا(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            إذا (classHasRandClassMembers(memberClassp)) {
                ثابت string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ثابت nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                Ast_طریقہ_Call* ثابت recurseCallp = جديد Ast_طریقہ_Call{
                    fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtype_سیٹ_Void();
                stmtsp = Ast_گرہ::جوڑو_اگلا(stmtsp, recurseCallp->makeStmt());
            }

            إذا (!stmtsp) أرجع;

            // Wrap in null check
            AstIf* ثابت nullCheckp = جديد AstIf{
                fl,
                جديد AstNeq{fl, جديد AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           جديد AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->جوڑو_Stmtsp(wrap_If_Rand_منوال(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* new_تیاری_Constraint_Task(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp = جديد AstTask{nodep->fileline(), name + "_setup_constraint", خالی_سوچک};
        taskp->class_طریقہ(سچ);
        nodep->جوڑو_Membersp(taskp);
        أرجع taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ثابت nodep, ثابت std::string& name) {
        AstTask* ثابت taskp
            = جديد AstTask{nodep->fileline(), name + "_resize_constrained_array", خالی_سوچک};
        taskp->class_طریقہ(سچ);
        nodep->جوڑو_Membersp(taskp);
        أرجع taskp;
    }
    Ast_گرہ_Stmt* implementConstraintsClear(File_سطر* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جديد AstCMethodHard{
            fileline,
            جديد AstVarRef{fileline, VN_AS(genp->user2p(), گرہ_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtype_سیٹ_Void();
        أرجع clearp->makeStmt();
    }
    Ast_گرہ_Stmt* implementConstraintsClearAll(File_سطر* ثابت fileline, AstVar* ثابت genp) {
        AstCMethodHard* ثابت clearp = جديد AstCMethodHard{
            fileline,
            جديد AstVarRef{fileline, VN_AS(genp->user2p(), گرہ_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtype_سیٹ_Void();
        أرجع clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_گرہ_اظہاریہ* ثابت exprp) {
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            أرجع memberSelp->varp();
        } وإلا إذا (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
            أرجع varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        أرجع خالی_سوچک;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_گرہ_اظہاریہ* make_منوال_Var_Ref(Ast_گرہ_اظہاریہ* ثابت exprp, AstVar* ثابت منوال_Varp,
                                ثابت VAccess access) {
        إذا (منوال_Varp->lifetime().isStatic()) {
            أرجع جديد AstVarRef{exprp->fileline(), VN_AS(منوال_Varp->user2p(), گرہ_Module),
                                 منوال_Varp, access};
        }
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ثابت newMemberSelp = جديد AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_درخت(خطأ), منوال_Varp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            أرجع newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        أرجع جديد AstVarRef{exprp->fileline(), VN_AS(منوال_Varp->user2p(), Class), منوال_Varp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* create_Or_Get_ناپ_Var(AstClass* ثابت classp, AstVar* ثابت arrVarp, File_سطر* ثابت fl,
                               Ast_گرہ_DType* ثابت signed32DTypep, منطقي& wasCreated) {
        AstVar* ناپ_Varp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = خطأ;
        إذا (!ناپ_Varp) {
            ناپ_Varp = جديد AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->جوڑو_Membersp(ناپ_Varp);
            m_memberMap.داخل_کرو(classp, ناپ_Varp);
            ناپ_Varp->user2p(classp);
            arrVarp->user4p(ناپ_Varp);
            wasCreated = سچ;
        }
        أرجع ناپ_Varp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_اظہاریہ* create_ناپ_Gte_Zero_Constraint(File_سطر* ثابت fl, AstVar* ثابت ناپ_Varp,
                                                   Ast_گرہ_اظہاریہ* derefFromp = خالی_سوچک) {
        Ast_گرہ_اظہاریہ* ناپ_گرہ_Exprp = خالی_سوچک;
        إذا (derefFromp) {
            ناپ_گرہ_Exprp = buildMemberSelExprp(derefFromp, ناپ_Varp);
        } وإلا {
            ناپ_گرہ_Exprp = جديد AstVarRef{fl, ناپ_Varp, VAccess::READ};
        }
        ناپ_گرہ_Exprp->user1(سچ);
        AstGteS* ثابت ناپ_Gtep = جديد AstGteS{fl, ناپ_گرہ_Exprp, جديد AstConst{fl, 0}};
        ناپ_Gtep->user1(سچ);
        أرجع جديد Ast_Constraint_اظہاریہ{fl, ناپ_Gtep};
    }
    Ast_گرہ_اظہاریہ* getFromp(Ast_گرہ_اظہاریہ* ثابت exprp) {
        إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            أرجع memberSelp->fromp();
        } وإلا إذا (Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(exprp, طریقہ_Call)) {
            أرجع طریقہ_Callp->fromp();
        }
        أرجع خالی_سوچک;
    }
    AstVar* make_Tmp_Rand_منوال_Var(Ast_گرہ_اظہاریہ* siblingExprp, AstVar* rand_منوال_Varp,
                               Ast_گرہ*& storeStmtspr, Ast_گرہ_Stmt*& restoreStmtspr) {
        File_سطر* ثابت fl = rand_منوال_Varp->fileline();
        AstVar* ثابت rand_منوال_Tmp_Varp = جديد AstVar{
            fl, VVarType::BLOCKTEMP, m_منوال_Unique_ناں.get(rand_منوال_Varp), rand_منوال_Varp->dtypep()};
        rand_منوال_Tmp_Varp->funcLocal(m_ftaskp);
        rand_منوال_Tmp_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_گرہ::جوڑو_اگلا(
            storeStmtspr,
            جديد Ast_سونپو{fl, جديد AstVarRef{fl, rand_منوال_Tmp_Varp, VAccess::WRITE},
                          make_منوال_Var_Ref(siblingExprp, rand_منوال_Varp, VAccess::READ)});
        storeStmtspr = Ast_گرہ::جوڑو_اگلا(
            storeStmtspr,
            make_منوال_سیٹ_Loop(fl, make_منوال_Var_Ref(siblingExprp, rand_منوال_Varp, VAccess::WRITE),
                            جديد AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_گرہ::جوڑو_اگلا(
            restoreStmtspr,
            جديد Ast_سونپو{fl, make_منوال_Var_Ref(siblingExprp, rand_منوال_Varp, VAccess::WRITE),
                          جديد AstVarRef{fl, rand_منوال_Tmp_Varp, VAccess::READ}});
        أرجع rand_منوال_Tmp_Varp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_گرہ_اظہاریہ* ٹکڑا_منزل_Common_اگیتر(Ast_گرہ_اظہاریہ* thisp, Ast_گرہ_اظہاریہ* otherp) {
        ساكن std::سمتیہ<Ast_گرہ_اظہاریہ*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        طالما (thisp) {
            thisHier.ڈالو_پٹھ(thisp);
            thisp = getFromp(thisp);
        }
        طالما (otherp) {
            otherHier.ڈالو_پٹھ(otherp);
            otherp = getFromp(otherp);
        }
        Ast_گرہ_اظہاریہ* commonp = خالی_سوچک;
        لكل (خودکار thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            إذا ((*thisIt)->type() != (*otherIt)->type()) اكسر;
            إذا (AstMemberSel* ثابت memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ثابت otherMemberSelp = VN_AS(*otherIt, MemberSel);
                إذا (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    استمر;
                }
            } وإلا إذا (Ast_طریقہ_Call* ثابت this_طریقہ_Callp = VN_CAST(*thisIt, طریقہ_Call)) {
                Ast_طریقہ_Call* ثابت other_طریقہ_Callp = VN_AS(*otherIt, طریقہ_Call);
                إذا (this_طریقہ_Callp->taskp() == other_طریقہ_Callp->taskp()) {
                    commonp = this_طریقہ_Callp;
                    استمر;
                }
            } وإلا إذا (AstVarRef* ثابت پہلا_Var_Refp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ثابت دوجا_Var_Refp = VN_AS(*otherIt, VarRef);
                إذا (پہلا_Var_Refp->varp() == دوجا_Var_Refp->varp()) {
                    commonp = پہلا_Var_Refp;
                    استمر;
                }
            }
            اكسر;
        }
        أرجع commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_گرہ_اظہاریہ* buildMemberSelExprp(Ast_گرہ_اظہاریہ* exprp, AstVar* سبھ_توں_ڈونگھا_Varp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_گرہ_اظہاریہ* resExprp = خالی_سوچک;
        إذا (ثابت AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), خالی_سوچک);
            AstVar* varp = memberSelp->varp();
            إذا (سبھ_توں_ڈونگھا_Varp) varp = سبھ_توں_ڈونگھا_Varp;
            resExprp = جديد AstMemberSel{exprp->fileline(), resExprp, varp};
        } وإلا إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            إذا (سبھ_توں_ڈونگھا_Varp) varp = سبھ_توں_ڈونگھا_Varp;
            resExprp = جديد AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), گرہ_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(سچ);
        أرجع resExprp;
    }

    فراغ جوڑو_Update_Rand_Vars_Body(AstClass* ثابت nodep, AstFunc* ثابت randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ثابت updatep
            = جديد AstFunc{nodep->fileline(), "__VupdateRandVars", خالی_سوچک, خالی_سوچک};
        updatep->class_طریقہ(سچ);
        updatep->isVirtual(سچ);
        updatep->keepAlive(سچ);
        updatep->needsSyms(خطأ);
        nodep->جوڑو_Membersp(updatep);
        nodep->hasRandVarsUpdate(سچ);

        لكل (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : خالی_سوچک) {
            Ast_گرہ_FTask* ثابت newp = VN_AS(m_memberMap.لبھو_Member(classp, "new"), گرہ_FTask);
            لكل (Ast_گرہ* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_اظہاریہ* ثابت stmtExprp = VN_CAST(stmtp, Stmt_اظہاریہ);
                إذا (!stmtExprp) استمر;
                AstCMethodHard* ثابت methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                إذا (!methodp || methodp->طریقہ() != VCMethod::RANDOMIZER_WRITE_VAR) استمر;

                Ast_گرہ_Var_Ref* ثابت varRefp = VN_AS(methodp->pinsp(), گرہ_Var_Ref);
                إذا (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    استمر;
                }
                AstCMethodHard* ثابت updateMethodp = جديد AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_درخت_خالص(خطأ),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_درخت_خالص(خطأ)};
                updateMethodp->dtype_سیٹ_Void();
                Ast_گرہ_اظہاریہ* ثابت namep = VN_AS(varRefp->nextp()->nextp(), گرہ_اظہاریہ);
                updateMethodp->جوڑو_Pinsp(namep->clone_درخت_خالص(خطأ));
                updateMethodp->طریقہ(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->جوڑو_Stmtsp(updateMethodp->makeStmt());
            }
        }
    }

    فراغ جوڑو_Basic_Randomize_Body(AstFunc* ثابت basicRandomizep, AstClass* ثابت nodep,
                               AstVar* rand_منوال_Varp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_سطر* ثابت fl = nodep->fileline();
        AstVar* ثابت basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ثابت basicFvarRefp = جديد AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ثابت beginBasicValp = جديد AstConst{fl, AstConst::Widthed_قدر{}, 32, 1};
        basicRandomizep->جوڑو_Stmtsp(جديد Ast_سونپو{fl, basicFvarRefp, beginBasicValp});
        Ast_گرہ_FTask* ثابت newp = VN_AS(m_memberMap.لبھو_Member(nodep, "new"), گرہ_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            إذا (!memberVarp->rand().isRandomizable()) أرجع;
            ثابت Randomize_منوال rand_منوال = {.asUQuad = memberVarp->user1()};
            إذا (rand_منوال.ورتدا_منوال
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ثابت setp = جديد AstCMethodHard{
                    nodep->fileline(),
                    جديد AstVarRef{fl, VN_AS(rand_منوال_Varp->user2p(), گرہ_Module), rand_منوال_Varp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, جديد AstConst{nodep->fileline(), rand_منوال.اشاریہ}};
                setp->dtype_سیٹ_UInt32();
                newp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, setp, جديد AstConst{fl, 0}});
            }
            إذا (memberVarp->user3()) أرجع;  // Handled in constraints
            ثابت Ast_گرہ_DType* ثابت dtypep = memberVarp->dtypep()->skipRefp();
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
                Ast_طریقہ_Call* ثابت callp
                    = memberVarp->globalConstrained()
                          ? جديد Ast_طریقہ_Call{fl,
                                              جديد AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : جديد Ast_طریقہ_Call{
                                fl, جديد AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ثابت basicFvarRefReadp = basicFvarRefp->clone_درخت(خطأ);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ثابت سونپو_If_Not_Nullp = جديد AstIf{
                    fl,
                    جديد AstNeq{fl, جديد AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               جديد AstConst{fl, AstConst::Null{}}},
                    جديد Ast_سونپو{fl, basicFvarRefp->clone_درخت(خطأ),
                                  جديد AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->جوڑو_Stmtsp(wrap_If_Rand_منوال(nodep, memberVarp, سونپو_If_Not_Nullp));
            } وإلا {
                AstVar* ثابت randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ثابت refp
                    = جديد AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_گرہ_Stmt* ثابت stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                إذا (!refp->backp()) VL_DO_DANGLING(refp->delete_درخت(), refp);
                basicRandomizep->جوڑو_Stmtsp(
                    جديد AstBegin{fl, m_unique_ناں.get("__VrandBlock"), stmtp, خطأ});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_گرہ_اظہاریہ* make_منوال_سونپو_Lhs(File_سطر* ثابت fl, AstClass* ثابت classp,
                                   Ast_گرہ_اظہاریہ* ثابت fromp, AstVar* ثابت منوال_Varp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        إذا (منوال_Varp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            إذا (fromp) VL_DO_DANGLING(fromp->unlink_Fr_پٹھ()->delete_درخت(), fromp);
            أرجع جديد AstVarRef{fl, VN_AS(منوال_Varp->user2p(), گرہ_Module), منوال_Varp,
                                 VAccess::WRITE};
        } وإلا إذا (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            أرجع جديد AstVarRef{fl, VN_AS(منوال_Varp->user2p(), گرہ_Module), منوال_Varp,
                                 VAccess::WRITE};
        } وإلا {
            AstMemberSel* ثابت memberselp = جديد AstMemberSel{fl, fromp->unlink_Fr_پٹھ(), منوال_Varp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            أرجع memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    فراغ replace_With_منوال_سونپو(Ast_گرہ_FTask_Ref* ثابت ftaskRefp, Ast_گرہ* ثابت receiverp,
                               Ast_گرہ_اظہاریہ* ثابت lhsp) {
        replace_With_منوال_سونپو_And_Append(ftaskRefp, receiverp, lhsp, خالی_سوچک);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    فراغ replace_With_منوال_سونپو_And_Append(Ast_گرہ_FTask_Ref* ثابت ftaskRefp, Ast_گرہ* ثابت receiverp,
                                        Ast_گرہ_اظہاریہ* ثابت lhsp, Ast_گرہ* ثابت appendStmtp) {
        File_سطر* ثابت fl = ftaskRefp->fileline();
        إذا (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_اظہاریہ), ftaskRefp, "Should be a statement");
            Ast_گرہ_اظہاریہ* ثابت rhsp = ftaskRefp->argsp()->exprp()->unlink_Fr_پٹھ();
            Ast_گرہ* newStmtp = خالی_سوچک;
            إذا (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                //منوال
                ثابت Randomize_منوال rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.ورتدا_منوال, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ثابت setp = جديد AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                جديد AstConst{fl, rmode.اشاریہ}};
                setp->dtype_سیٹ_UInt32();
                newStmtp = جديد Ast_سونپو{fl, setp, rhsp};
            } وإلا {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = make_منوال_سیٹ_Loop(fl, lhsp, rhsp, m_ftaskp);
            }
            إذا (appendStmtp) newStmtp->جوڑو_اگلا(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(ڈالو_Deletep(m_stmtp), m_stmtp = خالی_سوچک);
        } وإلا {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ثابت Randomize_منوال rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.ورتدا_منوال, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ثابت setp = جديد AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            جديد AstConst{fl, rmode.اشاریہ}};
            setp->dtype_سیٹ_UInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(ڈالو_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    فراغ wrapRandomizeCallWithNullGuard(Ast_گرہ_FTask_Ref* nodep) {
        Ast_طریقہ_Call* ثابت callp = VN_CAST(nodep, طریقہ_Call);
        إذا (!callp) أرجع;
        إذا (callp->user4_سیٹ_Once()) أرجع;
        File_سطر* ثابت fl = callp->fileline();
        Ast_گرہ_اظہاریہ* ثابت checkp
            = جديد AstNeq{fl, callp->fromp()->clone_درخت(خطأ), جديد AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlink_Fr_پٹھ(&relinker);
        relinker.relink(جديد AstCond{fl, checkp, callp, جديد AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    فراغ handle_Randomize_دلیلاں(Ast_گرہ_FTask_Ref* ثابت nodep) {
        إذا (!nodep->argsp()) أرجع;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        منطقي hasNullArg = خطأ;
        لكل (AstArg *argp = nodep->argsp(), *nextp = خالی_سوچک; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->عدد().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = سچ;
                VL_DO_DANGLING(argp->unlink_Fr_پٹھ()->delete_درخت(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        لكل (AstArg *argp = nodep->argsp(), *nextp = خالی_سوچک; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            لكل (AstArg *otherArgp = nextp, *otherNextp = خالی_سوچک; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                إذا (Ast_گرہ_اظہاریہ* ثابت prefixp
                    = ٹکڑا_منزل_Common_اگیتر(argp->exprp(), otherArgp->exprp())) {
                    إذا (prefixp == argp->exprp()) {
                        إذا (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlink_Fr_پٹھ()->delete_درخت(), otherArgp);
                        استمر;
                    }
                }
                إذا (Ast_گرہ_اظہاریہ* ثابت prefixp
                    = ٹکڑا_منزل_Common_اگیتر(otherArgp->exprp(), argp->exprp())) {
                    إذا (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlink_Fr_پٹھ()->delete_درخت(), argp);
                        اكسر;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::سیٹ<AstVar*> saved_Rand_منوال_Varps;
        AstVar* tmpVarps = خالی_سوچک;
        Ast_گرہ* storeStmtsp = خالی_سوچک;
        Ast_گرہ* سیٹ_Stmtsp = خالی_سوچک;
        Ast_گرہ_Stmt* restoreStmtsp = خالی_سوچک;
        لكل (AstArg *argp = nodep->argsp(), *nextp = خالی_سوچک; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_گرہ_اظہاریہ* exprp = argp->exprp();
            Ast_گرہ_اظہاریہ* ثابت commonPrefixp = ٹکڑا_منزل_Common_اگیتر(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_سطر* ثابت fl = argp->fileline();
            طالما (exprp) {
                إذا (commonPrefixp == exprp) اكسر;
                AstVar* ثابت randVarp = getVarFromRef(exprp);
                AstClass* ثابت classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ثابت rand_منوال_Varp = randVarp->lifetime().isStatic()
                                                 ? get_Static_Rand_منوال_Var(classp)
                                                 : get_Rand_منوال_Var_From_Class(classp);
                UASSERT_OBJ(rand_منوال_Varp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                إذا (saved_Rand_منوال_Varps.لبھو(rand_منوال_Varp) == saved_Rand_منوال_Varps.سرا()) {
                    AstVar* ثابت rand_منوال_Tmp_Varp
                        = make_Tmp_Rand_منوال_Var(exprp, rand_منوال_Varp, storeStmtsp, restoreStmtsp);
                    saved_Rand_منوال_Varps.داخل_کرو(rand_منوال_Varp);
                    tmpVarps = Ast_گرہ::جوڑو_اگلا(tmpVarps, rand_منوال_Tmp_Varp);
                }
                ثابت Randomize_منوال rand_منوال = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = جديد AstCMethodHard{
                    fl, make_منوال_Var_Ref(exprp, rand_منوال_Varp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, جديد AstConst{fl, rand_منوال.اشاریہ}};
                setp->dtype_سیٹ_UInt32();
                سیٹ_Stmtsp
                    = Ast_گرہ::جوڑو_اگلا(سیٹ_Stmtsp, جديد Ast_سونپو{fl, setp, جديد AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlink_Fr_پٹھ()->delete_درخت(), argp);
        }
        إذا (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* نشانہ_Classp = خالی_سوچک;
            Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(nodep, طریقہ_Call);
            إذا (طریقہ_Callp) {
                ثابت Ast_گرہ_DType* ثابت fromDTypep = طریقہ_Callp->fromp()->dtypep();
                ثابت AstClassRefDType* ثابت crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                نشانہ_Classp = crdtp->classp();
            } وإلا {
                نشانہ_Classp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(نشانہ_Classp, nodep, "randomize(null) target class unresolved");
            AstFunc* ثابت checkOnlyFuncp = getCreateRandomizeNullFunc(نشانہ_Classp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        إذا (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && سیٹ_Stmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlink_Fr_پٹھ(&relinker);
            Ast_گرہ* ثابت stmtsp = storeStmtsp;
            stmtsp->جوڑو_اگلا(سیٹ_Stmtsp);
            stmtsp->جوڑو_اگلا(m_stmtp);
            stmtsp->جوڑو_اگلا(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            جوڑو_عارضی_Varsp(tmpVarps);
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
        إذا (AstFunc* ثابت existingp = VN_AS(m_memberMap.لبھو_Member(classp, name), Func)) {
            أرجع existingp;
        }
        File_سطر* ثابت fl = classp->fileline();
        AstFunc* ثابت funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ثابت fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        جوڑو_Pre_Post_Call(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ثابت classGenp = getRandomGenerator(classp);
        إذا (!classGenp) {
            funcp->جوڑو_Stmtsp(جديد Ast_سونپو{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE},
                                           جديد AstConst{fl, AstConst::Widthed_قدر{}, 32, 1}});
        } وإلا {
            Ast_گرہ_Module* ثابت genModp = VN_AS(classGenp->user2p(), گرہ_Module);
            funcp->جوڑو_Stmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ثابت تیاری_All_Taskp = get_Create_Constraint_تیاری_Func(classp, m_memberMap);
            funcp->جوڑو_Stmtsp((جديد AstTaskRef{fl, تیاری_All_Taskp})->makeStmt());
            AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
            solverCallp->dtype_سیٹ_ثنائی_ہندسہ();
            solverCallp->جوڑو(جديد AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->جوڑو(".next_check_only(__Vm_rng)");
            funcp->جوڑو_Stmtsp(
                جديد Ast_سونپو{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(سچ);
        }

        // 3. post_randomize -- only if result is non-zero
        إذا (AstTask* ثابت userPostp = لبھو_Pre_Post_Task(classp, "post_randomize")) {
            AstTaskRef* ثابت callp = جديد AstTaskRef{userPostp->fileline(), userPostp};
            funcp->جوڑو_Stmtsp(
                جديد AstIf{fl, جديد AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        أرجع funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* lift_Log_If_Chain_منزل_Constraint_If(AstLogIf* logIfp) {
        File_سطر* ثابت fl = logIfp->fileline();
        Ast_گرہ_اظہاریہ* ثابت condp = logIfp->lhsp()->unlink_Fr_پٹھ();
        Ast_گرہ_اظہاریہ* ثابت rhsp = logIfp->rhsp()->unlink_Fr_پٹھ();
        Ast_گرہ* thenBodyp;
        إذا (AstLogIf* ثابت innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = lift_Log_If_Chain_منزل_Constraint_If(innerLogIfp);
            VL_DO_DANGLING(ڈالو_Deletep(innerLogIfp), innerLogIfp);
        } وإلا {
            thenBodyp = جديد Ast_Constraint_اظہاریہ{fl, rhsp};
        }
        أرجع جديد AstConstraintIf{fl, condp, thenBodyp, خالی_سوچک};
    }

    ساكن منطقي distBoundRefsRandVar(ثابت Ast_گرہ* boundp) {
        أرجع boundp->exists(
            [](ثابت AstVarRef* vrefp) { أرجع vrefp->varp()->rand().isRandomizable(); });
    }

    ساكن منطقي dist_Bound_Refs_منوال_Var(ثابت Ast_گرہ* boundp) {
        أرجع boundp->exists([](ثابت AstVarRef* vrefp) {
            إذا (!vrefp->varp()->rand().isRandomizable()) أرجع خطأ;
            ثابت Randomize_منوال rmode = {.asUQuad = vrefp->varp()->user1()};
            أرجع rmode.ورتدا_منوال;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ساكن Ast_گرہ_اظہاریہ* newDistRangeMembership(AstDist* distp, ثابت AstInsideRange* irp) {
        File_سطر* ثابت fl = distp->fileline();
        ثابت منطقي isSigned = distp->exprp()->isSigned();
        Ast_گرہ_اظہاریہ* ثابت dist_اظہاریہ_Gtep = distp->exprp()->clone_درخت_خالص(خطأ);
        Ast_گرہ_اظہاریہ* ثابت dist_اظہاریہ_Ltep = distp->exprp()->clone_درخت_خالص(خطأ);
        dist_اظہاریہ_Gtep->user1(سچ);
        dist_اظہاریہ_Ltep->user1(سچ);
        Ast_گرہ_اظہاریہ* ثابت gep
            = isSigned ? static_cast<Ast_گرہ_اظہاریہ*>(
                             جديد AstGteS{fl, dist_اظہاریہ_Gtep, irp->lhsp()->clone_درخت_خالص(خطأ)})
                       : static_cast<Ast_گرہ_اظہاریہ*>(
                             جديد AstGte{fl, dist_اظہاریہ_Gtep, irp->lhsp()->clone_درخت_خالص(خطأ)});
        Ast_گرہ_اظہاریہ* ثابت lep
            = isSigned ? static_cast<Ast_گرہ_اظہاریہ*>(
                             جديد AstLteS{fl, dist_اظہاریہ_Ltep, irp->rhsp()->clone_درخت_خالص(خطأ)})
                       : static_cast<Ast_گرہ_اظہاریہ*>(
                             جديد AstLte{fl, dist_اظہاریہ_Ltep, irp->rhsp()->clone_درخت_خالص(خطأ)});
        gep->user1(سچ);
        lep->user1(سچ);
        Ast_گرہ_اظہاریہ* ثابت andp = جديد AstLogAnd{fl, gep, lep};
        andp->user1(سچ);
        أرجع andp;
    }

    بنية Dist_بالٹی final {
        Ast_گرہ_اظہاریہ* rangep;  // A single value or an InsideRange
        Ast_گرہ_اظہاریہ* بھار_Exprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::سمتیہ<Dist_بالٹی> collectDistBuckets(AstDist* ثابت distp) {
        File_سطر* ثابت fl = distp->fileline();
        std::سمتیہ<Dist_بالٹی> buckets;
        لكل (Ast_Dist_شے* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), Dist_شے)) {
            إذا (ثابت AstConst* ثابت constp = VN_CAST(ditemp->weightp(), Const)) {
                إذا (constp->منزل_UQuad() == 0) استمر;
            }
            Ast_گرہ_اظہاریہ* بھار_Exprp
                = جديد AstExtend{fl, ditemp->weightp()->clone_درخت_خالص(خطأ), 64};
            // := on a range weights every element, so scale by the range size.
            إذا (!ditemp->is_سارا()) {
                إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ثابت AstConst* ثابت lop = VN_CAST(irp->lhsp(), Const);
                    ثابت AstConst* ثابت hip = VN_CAST(irp->rhsp(), Const);
                    Ast_گرہ_اظہاریہ* rangeSizep;
                    إذا (lop && hip) {
                        ثابت uint64_t range_ناپ = hip->منزل_UQuad() - lop->منزل_UQuad() + 1;
                        rangeSizep = جديد AstConst{fl, AstConst::Unsized64{}, range_ناپ};
                    } وإلا {
                        rangeSizep = جديد Ast_جوڑو{
                            fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                            جديد AstSub{fl,
                                       جديد AstExtend{fl, irp->rhsp()->clone_درخت_خالص(خطأ), 64},
                                       جديد AstExtend{fl, irp->lhsp()->clone_درخت_خالص(خطأ), 64}}};
                    }
                    بھار_Exprp = جديد AstMul{fl, بھار_Exprp, rangeSizep};
                }
            }
            buckets.ڈالو_پٹھ({ditemp->rangep(), بھار_Exprp});
        }
        أرجع buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ساكن Ast_گرہ_اظہاریہ* new_Dist_Membership_رکن(AstDist* distp, Ast_گرہ_اظہاریہ* rangep) {
        إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            أرجع newDistRangeMembership(distp, irp);
        }
        File_سطر* ثابت fl = distp->fileline();
        Ast_گرہ_اظہاریہ* ثابت eqExprp = distp->exprp()->clone_درخت_خالص(خطأ);
        eqExprp->user1(سچ);
        Ast_گرہ_اظہاریہ* ثابت eqp = جديد AstEq{fl, eqExprp, rangep->clone_درخت_خالص(خطأ)};
        eqp->user1(سچ);
        أرجع eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_اظہاریہ* buildDistMembership(AstDist* distp,
                                           ثابت std::سمتیہ<Dist_بالٹی>& buckets) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گرہ_اظہاریہ* unionExprp = خالی_سوچک;
        لكل (ثابت خودکار& بالٹی : buckets) {
            Ast_گرہ_اظہاریہ* ثابت memberp = new_Dist_Membership_رکن(distp, بالٹی.rangep);
            إذا (!unionExprp) {
                unionExprp = memberp;
            } وإلا {
                unionExprp = جديد AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(سچ);
            }
        }
        أرجع جديد Ast_Constraint_اظہاریہ{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_گرہ_اظہاریہ* newGateJoin(File_سطر* fl, Ast_گرہ_اظہاریہ* ap, Ast_گرہ_اظہاریہ* bp, منطقي isAnd) {
        إذا (ap->same_درخت(bp)) {
            VL_DO_DANGLING(ڈالو_Deletep(bp), bp);
            أرجع ap;
        }
        إذا (VN_IS(bp, Const)) std::وٹاندرا(ap, bp);
        إذا (VN_IS(ap, Const)) {
            ثابت منطقي constWins = VN_AS(ap, Const)->isZero() == isAnd;
            إذا (!constWins) std::وٹاندرا(ap, bp);
            VL_DO_DANGLING(ڈالو_Deletep(bp), bp);
            أرجع ap;
        }
        إذا (isAnd) أرجع جديد AstLogAnd{fl, ap, bp};
        أرجع جديد AstLogOr{fl, ap, bp};
    }
    Ast_گرہ_اظہاریہ* newGateOr(File_سطر* fl, Ast_گرہ_اظہاریہ* ap, Ast_گرہ_اظہاریہ* bp) {
        أرجع newGateJoin(fl, ap, bp, خطأ);
    }
    Ast_گرہ_اظہاریہ* newGateAnd(File_سطر* fl, Ast_گرہ_اظہاریہ* ap, Ast_گرہ_اظہاریہ* bp) {
        أرجع newGateJoin(fl, ap, bp, سچ);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_گرہ_اظہاریہ* new_منوال_ثنائی_ہندسہ_Read(AstVar* varp, ثابت AstMemberSel* mselp, AstVar* rand_منوال_Varp,
                                File_سطر* fl) {
        Ast_گرہ_اظہاریہ* arrayp = خالی_سوچک;
        إذا (varp->lifetime().isStatic()) {
            AstClass* ثابت ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ثابت smodep = get_Static_Rand_منوال_Var(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = جديد AstVarRef{fl, VN_AS(smodep->user2p(), گرہ_Module), smodep, VAccess::READ};
        } وإلا إذا (mselp) {
            AstVar* ثابت memberModep = get_Rand_منوال_Var_From_Class(VN_AS(varp->user2p(), گرہ_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = جديد AstMemberSel{fl, mselp->fromp()->clone_درخت_خالص(خطأ), memberModep};
        } وإلا {
            UASSERT_OBJ(rand_منوال_Varp, varp, "rand_mode variable without a class mode array");
            arrayp = جديد AstVarRef{fl, VN_AS(rand_منوال_Varp->user2p(), گرہ_Module), rand_منوال_Varp,
                                   VAccess::READ};
        }
        ثابت Randomize_منوال rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ثابت atp
            = جديد AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, جديد AstConst{fl, rmode.اشاریہ}};
        atp->dtype_سیٹ_UInt32();
        أرجع atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_گرہ_اظہاریہ* newVarGate(AstVar* varp, ثابت AstMemberSel* mselp, منطقي ownerLevel,
                            AstVar* rand_منوال_Varp, File_سطر* fl) {
        إذا (!varp->rand().isRandomizable()) {
            أرجع جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}, ownerLevel};
        }
        ثابت Randomize_منوال rmode = {.asUQuad = varp->user1()};
        إذا (!rmode.ورتدا_منوال) أرجع جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}};
        أرجع new_منوال_ثنائی_ہندسہ_Read(varp, mselp, rand_منوال_Varp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_گرہ_اظہاریہ* newAccessGate(ثابت Ast_گرہ* nodep, منطقي ownerLevel, AstVar* rand_منوال_Varp,
                               File_سطر* fl) {
        إذا (ثابت AstMemberSel* ثابت mselp = VN_CAST(nodep, MemberSel)) {
            أرجع newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, rand_منوال_Varp, fl),
                              newAccessGate(mselp->fromp(), سچ, rand_منوال_Varp, fl));
        }
        إذا (ثابت Ast_گرہ_Var_Ref* ثابت refp = VN_CAST(nodep, گرہ_Var_Ref)) {
            أرجع newVarGate(refp->varp(), خالی_سوچک, ownerLevel, rand_منوال_Varp, fl);
        }
        إذا (ثابت Ast_گرہ_Sel* ثابت selp = VN_CAST(nodep, گرہ_Sel)) {
            أرجع newAccessGate(selp->fromp(), سچ, rand_منوال_Varp, fl);
        }
        إذا (dist_Bound_Refs_منوال_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        أرجع جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_گرہ_اظہاریہ* newDistGate(ثابت Ast_گرہ* nodep, AstVar* rand_منوال_Varp, File_سطر* fl) {
        إذا (VN_IS(nodep, گرہ_Var_Ref) || VN_IS(nodep, MemberSel)) {
            أرجع newAccessGate(nodep, خطأ, rand_منوال_Varp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        إذا (ثابت Ast_گرہ_Sel* ثابت selp = VN_CAST(nodep, گرہ_Sel)) {
            أرجع newDistGate(selp->fromp(), rand_منوال_Varp, fl);
        }
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            أرجع newDistGate(selp->fromp(), rand_منوال_Varp, fl);
        }
        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {
            أرجع newDistGate(selp->fromp(), rand_منوال_Varp, fl);
        }
        إذا (ثابت AstCMethodHard* ثابت methodp = VN_CAST(nodep, CMethodHard)) {
            إذا (methodp->طریقہ() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                أرجع newDistGate(methodp->fromp(), rand_منوال_Varp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        إذا (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            Ast_گرہ_اظہاریہ* ثابت thenGatep = newDistGate(condp->thenp(), rand_منوال_Varp, fl);
            Ast_گرہ_اظہاریہ* ثابت elseGatep = newDistGate(condp->elsep(), rand_منوال_Varp, fl);
            Ast_گرہ_اظہاریہ* armGatep = thenGatep;
            إذا (thenGatep->same_درخت(elseGatep)) {
                VL_DO_DANGLING(ڈالو_Deletep(elseGatep), elseGatep);
            } وإلا {
                armGatep
                    = جديد AstCond{fl, condp->condp()->clone_درخت_خالص(خطأ), thenGatep, elseGatep};
            }
            أرجع newGateOr(fl, newDistGate(condp->condp(), rand_منوال_Varp, fl), armGatep);
        }
        إذا (ثابت Ast_گرہ_Uniop* ثابت uopp = VN_CAST(nodep, گرہ_Uniop)) {
            أرجع newDistGate(uopp->lhsp(), rand_منوال_Varp, fl);
        }
        إذا (ثابت Ast_گرہ_Biop* ثابت bopp = VN_CAST(nodep, گرہ_Biop)) {
            أرجع newGateOr(fl, newDistGate(bopp->lhsp(), rand_منوال_Varp, fl),
                             newDistGate(bopp->rhsp(), rand_منوال_Varp, fl));
        }
        إذا (dist_Bound_Refs_منوال_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        أرجع جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_اظہاریہ* buildFrozenMembership(AstDist* distp,
                                             ثابت std::سمتیہ<Dist_بالٹی>& buckets) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گرہ_اظہاریہ* unionExprp = خالی_سوچک;
        لكل (ثابت خودکار& بالٹی : buckets) {
            Ast_گرہ_اظہاریہ* termp = new_Dist_Membership_رکن(distp, بالٹی.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ثابت منطقي runtime_بھار
                = بالٹی.بھار_Exprp->exists([](ثابت Ast_گرہ_Var_Ref*) { أرجع سچ; });
            إذا (runtime_بھار) {
                AstNeq* ثابت nzp = جديد AstNeq{fl, بالٹی.بھار_Exprp->clone_درخت_خالص(خطأ),
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
        أرجع جديد Ast_Constraint_اظہاریہ{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_گرہ* gateFrozenDist(Ast_گرہ* chainp, AstDist* distp,
                            ثابت std::سمتیہ<Dist_بالٹی>& buckets, AstVar* rand_منوال_Varp) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گرہ_اظہاریہ* gatep = newDistGate(distp->exprp(), rand_منوال_Varp, fl);
        إذا (VN_IS(gatep, Const)) {
            ثابت منطقي drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(ڈالو_Deletep(gatep), gatep);
            إذا (drawable) أرجع chainp;
            VL_DO_DANGLING(ڈالو_Deletep(chainp), chainp);
            أرجع buildFrozenMembership(distp, buckets);
        }
        أرجع جديد AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_گرہ_اظہاریہ* new_اکسار_Range_Pick(AstDist* distp, ثابت AstInsideRange* irp) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گرہ_اظہاریہ* ثابت dist_اظہاریہ_Copyp = distp->exprp()->clone_درخت_خالص(خطأ);
        dist_اظہاریہ_Copyp->user1(سچ);
        ثابت صحيح dist_چوڑائی = distp->exprp()->چوڑائی();
        ثابت AstConst* ثابت lopC = VN_CAST(irp->lhsp(), Const);
        ثابت AstConst* ثابت hipC = VN_CAST(irp->rhsp(), Const);
        Ast_گرہ_اظہاریہ* rangeSzp;
        إذا (lopC && hipC) {
            ثابت uint64_t rsz = hipC->منزل_UQuad() - lopC->منزل_UQuad() + 1;
            rangeSzp = جديد AstConst{fl, AstConst::Unsized64{}, rsz};
        } وإلا {
            ثابت منطقي isSigned = irp->lhsp()->isSigned();
            Ast_گرہ_اظہاریہ* ثابت lo64p
                = isSigned ? static_cast<Ast_گرہ_اظہاریہ*>(
                                 جديد AstExtendS{fl, irp->lhsp()->clone_درخت_خالص(خطأ), 64})
                           : static_cast<Ast_گرہ_اظہاریہ*>(
                                 جديد AstExtend{fl, irp->lhsp()->clone_درخت_خالص(خطأ), 64});
            Ast_گرہ_اظہاریہ* ثابت hi64p
                = isSigned ? static_cast<Ast_گرہ_اظہاریہ*>(
                                 جديد AstExtendS{fl, irp->rhsp()->clone_درخت_خالص(خطأ), 64})
                           : static_cast<Ast_گرہ_اظہاریہ*>(
                                 جديد AstExtend{fl, irp->rhsp()->clone_درخت_خالص(خطأ), 64});
            rangeSzp = جديد Ast_جوڑو{fl, جديد AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  جديد AstSub{fl, hi64p, lo64p}};
        }
        Ast_گرہ_اظہاریہ* ثابت rand64p = جديد AstRand{fl, خالی_سوچک, خطأ};
        rand64p->dtype_سیٹ_UInt64();
        Ast_گرہ_اظہاریہ* ثابت offsetp
            = جديد AstCCast{fl, جديد AstModDiv{fl, rand64p, rangeSzp}, dist_چوڑائی};
        Ast_گرہ_اظہاریہ* ثابت valuep = جديد Ast_جوڑو{fl, irp->lhsp()->clone_درخت_خالص(خطأ), offsetp};
        Ast_گرہ_اظہاریہ* ثابت eqp = جديد AstEq{fl, dist_اظہاریہ_Copyp, valuep};
        eqp->user1(سچ);
        أرجع eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_گرہ* build_Weighted_بالٹی_Chain(AstDist* distp, ثابت std::سمتیہ<Dist_بالٹی>& buckets,
                                      AstVar* بالٹی_Varp,
                                      ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*>& cumSums) {
        File_سطر* ثابت fl = distp->fileline();
        Ast_گرہ_اظہاریہ* chainp = خالی_سوچک;
        لكل (صحيح i = static_cast<صحيح>(buckets.ناپ()) - 1; i >= 0; --i) {
            Ast_گرہ_اظہاریہ* constraintExprp;
            ثابت AstInsideRange* ثابت irp = VN_CAST(buckets[i].rangep, InsideRange);
            إذا (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } وإلا إذا (irp) {
                constraintExprp = new_اکسار_Range_Pick(distp, irp);
            } وإلا {
                Ast_گرہ_اظہاریہ* ثابت dist_اظہاریہ_Copyp = distp->exprp()->clone_درخت_خالص(خطأ);
                dist_اظہاریہ_Copyp->user1(سچ);
                constraintExprp
                    = جديد AstEq{fl, dist_اظہاریہ_Copyp, buckets[i].rangep->clone_درخت_خالص(خطأ)};
                constraintExprp->user1(سچ);
            }
            إذا (!chainp) {
                chainp = constraintExprp;
            } وإلا {
                Ast_گرہ_اظہاریہ* ثابت condp
                    = جديد AstLte{fl, جديد AstVarRef{fl, بالٹی_Varp, VAccess::READ}, cumSums[i]};
                condp->user1(سچ);
                chainp = جديد AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(سچ);
            }
        }
        Ast_Constraint_اظہاریہ* ثابت chainExprp = جديد Ast_Constraint_اظہاریہ{fl, chainp};
        chainExprp->isSoft(سچ);
        أرجع chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    فراغ ہیٹھلا_Dist_Constraints(AstTask* taskp, Ast_گرہ* constrItemsp, AstVar* rand_منوال_Varp,
                              AstConstraintForeach* foreachp = خالی_سوچک) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_گرہ* foreachTailp = خالی_سوچک;
        خودکار جوڑو_Stmt = [&](Ast_گرہ* nodep) {
            إذا (foreachp) {
                إذا (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } وإلا {
                    foreachTailp->جوڑو_اگلا(nodep);
                    foreachTailp = nodep;
                }
            } وإلا {
                taskp->جوڑو_Stmtsp(nodep);
            }
        };

        لكل (Ast_گرہ *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            إذا (AstConstraintIf* ثابت cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                ہیٹھلا_Dist_Constraints(taskp, cifp->thensp(), rand_منوال_Varp, foreachp);
                إذا (cifp->elsesp()) {
                    ہیٹھلا_Dist_Constraints(taskp, cifp->elsesp(), rand_منوال_Varp, foreachp);
                }
                استمر;
            }
            إذا (AstConstraintForeach* ثابت cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                ہیٹھلا_Dist_Constraints(taskp, cfep->bodyp(), rand_منوال_Varp, cfep);
                استمر;
            }

            Ast_Constraint_اظہاریہ* ثابت constrExprp = VN_CAST(itemp, Constraint_اظہاریہ);
            إذا (!constrExprp) استمر;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            إذا (AstLogIf* ثابت چوٹی_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_گرہ* chainEndp = چوٹی_Log_Ifp->rhsp();
                طالما (AstLogIf* ثابت innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                إذا (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ثابت liftedp = lift_Log_If_Chain_منزل_Constraint_If(چوٹی_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(ڈالو_Deletep(constrExprp), constrExprp);
                    ہیٹھلا_Dist_Constraints(taskp, liftedp->thensp(), rand_منوال_Varp, foreachp);
                    استمر;
                }
            }

            AstDist* ثابت distp = VN_CAST(constrExprp->exprp(), Dist);
            إذا (!distp) استمر;

            File_سطر* ثابت fl = distp->fileline();

            std::سمتیہ<Dist_بالٹی> buckets = collectDistBuckets(distp);

            إذا (buckets.خالی_تھاواں()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_اظہاریہ* ثابت truep
                    = جديد Ast_Constraint_اظہاریہ{fl, جديد AstConst{fl, AstConst::ثنائی_ہندسہ_True{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(ڈالو_Deletep(constrExprp), constrExprp);
                استمر;
            }

            Ast_Constraint_اظہاریہ* ثابت membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_گرہ_اظہاریہ* کل_بھار_Exprp = خالی_سوچک;
            لكل (خودکار& بالٹی : buckets) {
                إذا (!کل_بھار_Exprp) {
                    کل_بھار_Exprp = بالٹی.بھار_Exprp->clone_درخت_خالص(خطأ);
                } وإلا {
                    کل_بھار_Exprp = جديد Ast_جوڑو{fl, کل_بھار_Exprp,
                                                  بالٹی.بھار_Exprp->clone_درخت_خالص(خطأ)};
                    کل_بھار_Exprp->dtype_سیٹ_UInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ثابت صحيح distId = m_dist_عدد++;
            ثابت std::string کل_Name = "__Vdist_total" + cvt_منزل_Str(distId);
            AstVar* ثابت کل_Varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, کل_Name, taskp->لبھو_UInt64_DType()};
            کل_Varp->noSubst(سچ);
            کل_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            کل_Varp->funcLocal(سچ);
            کل_Varp->is_اندرونی(سچ);
            جوڑو_Stmt(کل_Varp);
            جوڑو_Stmt(
                جديد Ast_سونپو{fl, جديد AstVarRef{fl, کل_Varp, VAccess::WRITE}, کل_بھار_Exprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ثابت std::string بالٹی_Name = "__Vdist_bucket" + cvt_منزل_Str(distId);
            AstVar* ثابت بالٹی_Varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP, بالٹی_Name, taskp->لبھو_UInt64_DType()};
            بالٹی_Varp->noSubst(سچ);
            بالٹی_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            بالٹی_Varp->funcLocal(سچ);
            بالٹی_Varp->is_اندرونی(سچ);
            جوڑو_Stmt(بالٹی_Varp);

            Ast_گرہ_اظہاریہ* ثابت randp = جديد AstRand{fl, خالی_سوچک, خطأ};
            randp->dtype_سیٹ_UInt64();
            جوڑو_Stmt(جديد Ast_سونپو{
                fl, جديد AstVarRef{fl, بالٹی_Varp, VAccess::WRITE},
                جديد Ast_جوڑو{
                    fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                    جديد AstModDiv{fl, randp, جديد AstVarRef{fl, کل_Varp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::سمتیہ<Ast_گرہ_اظہاریہ*> cumSums;
            Ast_گرہ_اظہاریہ* runningSump = خالی_سوچک;
            لكل (ناپ_t i = 0; i < buckets.ناپ(); ++i) {
                إذا (!runningSump) {
                    runningSump = buckets[i].بھار_Exprp->clone_درخت_خالص(خطأ);
                } وإلا {
                    runningSump = جديد Ast_جوڑو{fl, runningSump,
                                             buckets[i].بھار_Exprp->clone_درخت_خالص(خطأ)};
                    runningSump->dtype_سیٹ_UInt64();
                }
                cumSums.ڈالو_پٹھ(runningSump->clone_درخت_خالص(سچ));
            }

            Ast_گرہ* chainp = build_Weighted_بالٹی_Chain(distp, buckets, بالٹی_Varp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, rand_منوال_Varp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(ڈالو_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->جوڑو_ایتھے_This_As_اگلا(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            لكل (خودکار& بالٹی : buckets) {
                VL_DO_DANGLING(ڈالو_Deletep(بالٹی.بھار_Exprp), بالٹی.بھار_Exprp);
            }
            VL_DO_DANGLING(ڈالو_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            ڈالو_Deletep(cumSums.پٹھ());
        }
    }

    ساكن منطقي is_Dyn_Arr_دا_Class_Type_Recurse(ثابت Ast_گرہ_DType* ثابت dtypep) {
        ثابت Ast_گرہ_DType* ثابت refp = dtypep->skipRefp();
        إذا (VN_IS(refp, DynArrayDType) || VN_IS(refp, قطار_DType)) {
            أرجع is_Dyn_Arr_دا_Class_Type_Recurse(refp->subDTypep());
        } وإلا إذا (VN_IS(refp, ClassRefDType)) {
            أرجع سچ;
        }
        أرجع خطأ;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    فراغ جوڑو_Array_Elem_Refresh(File_سطر* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                             AstVar* ثابت genp) {
        Ast_گرہ_Module* ثابت genModp = VN_AS(genp->user2p(), گرہ_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ثابت uint32_t unpackedDims = arrVarp->dtypep()->dimensions(خطأ).دوجا;
        إذا (is_Dyn_Arr_دا_Class_Type_Recurse(arrVarp->dtypep())) {
            إذا (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            أرجع;
        }
        AstCMethodHard* ثابت methodp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_سیٹ_Void();

        Ast_گرہ_Module* ثابت classp = VN_AS(arrVarp->user2p(), گرہ_Module);
        AstVarRef* ثابت varRefp = جديد AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->جوڑو_Pinsp(varRefp);

        ثابت ناپ_t چوڑائی = arrayElementDTypep(arrVarp->dtypep())->چوڑائی();

        methodp->جوڑو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, چوڑائی});
        Ast_گرہ_اظہاریہ* ثابت varnamep
            = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + arrVarp->name() + "\"", arrVarp->چوڑائی()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->جوڑو_Pinsp(varnamep);
        methodp->جوڑو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ثابت Randomize_منوال rmode = {.asUQuad = arrVarp->user1()};
        إذا (rmode.ورتدا_منوال) {
            methodp->جوڑو_Pinsp(جديد AstConst{fl, AstConst::Unsized64{}, rmode.اشاریہ});
        }

        randomizep->جوڑو_Stmtsp(methodp->makeStmt());
    }

    Ast_گرہ_Stmt* new_Pin_ناپ_Variable(File_سطر* ثابت fl, AstVar* ثابت arrVarp,
                                    AstVar* ثابت genp) {
        Ast_گرہ_Module* ثابت genModp = VN_AS(genp->user2p(), گرہ_Module);
        AstVar* ثابت ناپ_Varp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ثابت pinp
            = جديد AstCMethodHard{fl, جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtype_سیٹ_Void();
        AstCExpr* ثابت namep = جديد AstCExpr{fl, AstCExpr::خالص{}, "\"" + ناپ_Varp->name() + "\""};
        namep->dtype_سیٹ_UInt32();
        pinp->جوڑو_Pinsp(namep);
        pinp->جوڑو_Pinsp(
            جديد AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(ناپ_Varp->چوڑائی())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ثابت ناپ_Var_Refp = جديد AstVarRef{fl, ناپ_Varp, VAccess::READ};
        ناپ_Var_Refp->classOrPackagep(VN_AS(ناپ_Varp->user2p(), گرہ_Module));
        pinp->جوڑو_Pinsp(ناپ_Var_Refp);
        أرجع pinp->makeStmt();
    }

    فراغ جوڑو_Frozen_ناپ_Pin(File_سطر* ثابت fl, AstVar* ثابت arrVarp, AstFunc* ثابت randomizep,
                          AstVar* ثابت genp, AstVar* ثابت rand_منوال_Varp) {
        ثابت Randomize_منوال rmode = {.asUQuad = arrVarp->user1()};
        إذا (!rmode.ورتدا_منوال) أرجع;
        AstVar* ثابت ناپ_Varp = VN_AS(arrVarp->user4p(), Var);
        Ast_گرہ_Module* ثابت arrClassp = VN_AS(arrVarp->user2p(), گرہ_Module);
        Ast_گرہ_Module* ثابت ناپ_Classp = VN_AS(ناپ_Varp->user2p(), گرہ_Module);
        AstVarRef* ثابت ناپ_Writep = جديد AstVarRef{fl, ناپ_Classp, ناپ_Varp, VAccess::WRITE};
        ثابت VCMethod ناپ_طریقہ = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_ناپ
                                        : VCMethod::DYN_ناپ;
        AstCMethodHard* ثابت currentSizep = جديد AstCMethodHard{
            fl, جديد AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, ناپ_طریقہ};
        currentSizep->dtypep(ناپ_Varp->dtypep());
        randomizep->جوڑو_Stmtsp(جديد Ast_سونپو{fl, ناپ_Writep, currentSizep});
        Ast_گرہ_اظہاریہ* ثابت modep = new_منوال_ثنائی_ہندسہ_Read(arrVarp, خالی_سوچک, rand_منوال_Varp, fl);
        randomizep->جوڑو_Stmtsp(
            جديد AstIf{fl, جديد AstLogNot{fl, modep}, new_Pin_ناپ_Variable(fl, arrVarp, genp)});
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_عدد);
        VL_RESTORER_COPY(m_rand_Array_Unique_ناں);
        m_modp = nodep;
        m_rand_Case_عدد = 0;
        m_rand_Array_Unique_ناں.reset();
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstClass* nodep) override {
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
        nodep->ادھار_Most_Classp()->needRNG(سچ);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ثابت منطقي basic_پہلا = classOwnsGlobalConstraint(nodep);

        File_سطر* ثابت fl = nodep->fileline();
        AstFunc* ثابت randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ثابت fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ثابت rand_منوال_Varp = get_Rand_منوال_Var_From_Class(nodep);
        جوڑو_Pre_Post_Call(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(nodep)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->جوڑو_Stmtsp((جديد AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_گرہ_اظہاریہ* beginValp = خالی_سوچک;
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
                    resizeAllTaskp->جوڑو_Stmtsp(resizeTaskRefp->makeStmt());
                }

                إذا (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                إذا (constrp->itemsp()) {
                    ہیٹھلا_Dist_Constraints(taskp, constrp->itemsp(), rand_منوال_Varp);
                }
                VInsertion_سیٹ<AstVar*>& ناپ_Arrays = m_ناپ_Constrained_Arrays[classp];
                Constraint_اظہاریہ_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), خالی_سوچک,
                    genp,          rand_منوال_Varp, m_writtenVars,     &m_uniqueConstraintId,
                    m_unique_ناں, randomizep,   &ناپ_Arrays,       prepareArraysTaskp};
                إذا (constrp->itemsp()) {
                    taskp->جوڑو_Stmtsp(wrap_If_Constraint_منوال(
                        nodep, constrp, constrp->itemsp()->unlink_Fr_پٹھ_With_اگلا()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ثابت خودکار constraint_ڈونگھائی = [](ثابت AstConstraint* constrp) {
                    صحيح ڈونگھائی = 0;
                    ناپ_t pos = 0;
                    طالما ((pos = constrp->name().لبھو(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++ڈونگھائی;
                        pos += std::لڑی_لمبائی(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    أرجع ڈونگھائی;
                };
                صحيح max_ڈونگھائی = 0;
                nodep->foreachMember([&](AstClass* ثابت, AstConstraint* ثابت constrp) {
                    max_ڈونگھائی = std::max(max_ڈونگھائی, constraint_ڈونگھائی(constrp));
                });
                AstTask* ثابت تیاری_All_Taskp = get_Create_Constraint_تیاری_Func(nodep, m_memberMap);
                لكل (صحيح d = max_ڈونگھائی; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ثابت classp, AstConstraint* ثابت constrp) {
                            إذا (constraint_ڈونگھائی(constrp) != d) أرجع;
                            AstTask* ثابت taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ثابت تیاری_Task_Refp
                                = جديد AstTaskRef{constrp->fileline(), taskp};
                            تیاری_Task_Refp->classOrPackagep(classp);
                            تیاری_All_Taskp->جوڑو_Stmtsp(تیاری_Task_Refp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertion_سیٹ<AstVar*> ناپ_Array_Vars;
            إذا (nodep->extendsp()) {
                AstClass* ماپا_Classp = nodep->extendsp()->classp();
                طالما (ماپا_Classp) {
                    ثابت خودکار ناپ_Arrays_It = m_ناپ_Constrained_Arrays.لبھو(ماپا_Classp);
                    إذا (ناپ_Arrays_It != m_ناپ_Constrained_Arrays.سرا()) {
                        لكل (AstVar* ثابت arrVarp : ناپ_Arrays_It->دوجا) {
                            ناپ_Array_Vars.داخل_کرو(arrVarp);
                        }
                    }
                    AstFunc* ثابت ماپا_Randomizep
                        = VN_CAST(m_memberMap.لبھو_Member(ماپا_Classp, "randomize"), Func);
                    إذا (ماپا_Randomizep && ماپا_Randomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        لكل (Ast_گرہ* stmtp = ماپا_Randomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            منطقي لبھیا_Clear_Constraints = خطأ;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                إذا (methodp->طریقہ() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->طریقہ() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->جوڑو_Stmtsp(stmtp->clone_درخت(خطأ));
                                } وإلا إذا (methodp->طریقہ()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    لبھیا_Clear_Constraints = سچ;
                                }
                            });
                            إذا (لبھیا_Clear_Constraints) اكسر;
                        }
                    }
                    ماپا_Classp
                        = ماپا_Classp->extendsp() ? ماپا_Classp->extendsp()->classp() : خالی_سوچک;
                }
            }
            randomizep->جوڑو_Stmtsp(implementConstraintsClear(fl, genp));

            ثابت خودکار ناپ_Arrays_It = m_ناپ_Constrained_Arrays.لبھو(nodep);
            إذا (ناپ_Arrays_It != m_ناپ_Constrained_Arrays.سرا()) {
                لكل (AstVar* ثابت arrVarp : ناپ_Arrays_It->دوجا) ناپ_Array_Vars.داخل_کرو(arrVarp);
            }
            AstTask* ثابت تیاری_All_Taskp = get_Create_Constraint_تیاری_Func(nodep, m_memberMap);
            إذا (!ناپ_Array_Vars.خالی_تھاواں() && prepareArraysTaskp->stmtsp()) {
                Ast_گرہ_Stmt* ثابت preparep = (جديد AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                لكل (Ast_گرہ* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    منطقي writesArray = خطأ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->طریقہ() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    إذا (!writesArray) استمر;
                    stmtp->جوڑو_ایتھے_This_As_اگلا(preparep);
                    اكسر;
                }
            }
            AstTaskRef* ثابت تیاری_Task_Refp = جديد AstTaskRef{fl, تیاری_All_Taskp};
            randomizep->جوڑو_Stmtsp(تیاری_Task_Refp->makeStmt());

            Ast_گرہ_Module* ثابت genModp = VN_AS(genp->user2p(), گرہ_Module);

            AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
            solverCallp->dtype_سیٹ_ثنائی_ہندسہ();
            solverCallp->جوڑو(جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->جوڑو(".next(__Vm_rng)");
            إذا (!ناپ_Array_Vars.خالی_تھاواں()) {
                AstVar* ثابت ناپ_Ok_Varp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->لبھو_Basic_DType(VBasicDTypeKwd::ثنائی_ہندسہ)};
                ناپ_Ok_Varp->funcLocal(سچ);
                randomizep->جوڑو_Stmtsp(ناپ_Ok_Varp);

                AstVar* ثابت finalOkVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->لبھو_Basic_DType(VBasicDTypeKwd::ثنائی_ہندسہ)};
                finalOkVarp->funcLocal(سچ);
                randomizep->جوڑو_Stmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                لكل (AstVar* ثابت arrVarp : ناپ_Array_Vars)
                    جوڑو_Frozen_ناپ_Pin(fl, arrVarp, randomizep, genp, rand_منوال_Varp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->جوڑو_Stmtsp(
                    جديد Ast_سونپو{fl, جديد AstVarRef{fl, ناپ_Ok_Varp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                إذا (AstTask* ثابت resizeAllTaskp
                    = VN_AS(m_memberMap.لبھو_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ثابت resizeTaskRefp = جديد AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ثابت ifp = جديد AstIf{fl, جديد AstVarRef{fl, ناپ_Ok_Varp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), خالی_سوچک};
                    randomizep->جوڑو_Stmtsp(ifp);
                }

                // Refresh array element tables after resize
                لكل (ثابت خودکار& arrVarp : ناپ_Array_Vars) {
                    جوڑو_Array_Elem_Refresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->جوڑو_Stmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ثابت تیاری_Task_Refp2 = جديد AstTaskRef{fl, تیاری_All_Taskp};
                randomizep->جوڑو_Stmtsp(تیاری_Task_Refp2->makeStmt());

                لكل (AstVar* ثابت arrVarp : ناپ_Array_Vars) {
                    randomizep->جوڑو_Stmtsp(new_Pin_ناپ_Variable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ثابت solverCallp2 = جديد AstCExpr{fl};
                solverCallp2->dtype_سیٹ_ثنائی_ہندسہ();
                solverCallp2->جوڑو(جديد AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->جوڑو(".next(__Vm_rng)");
                randomizep->جوڑو_Stmtsp(جديد Ast_سونپو{
                    fl, جديد AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = جديد AstAnd{fl, جديد AstVarRef{fl, ناپ_Ok_Varp, VAccess::READ},
                                       جديد AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } وإلا {
                beginValp = solverCallp;
            }
            إذا (rand_منوال_Varp) {
                Ast_گرہ_Module* ثابت rand_منوال_Classp = VN_AS(rand_منوال_Varp->user2p(), Class);
                Ast_گرہ_FTask* ثابت newp
                    = VN_AS(m_memberMap.لبھو_Member(rand_منوال_Classp, "new"), گرہ_FTask);
                UASSERT_OBJ(newp, rand_منوال_Classp, "No new() in class");
                جوڑو_سیٹ_Rand_منوال(newp, genp, rand_منوال_Varp);
            }
            إذا (AstVar* ثابت static_Rand_منوال_Varp = get_Static_Rand_منوال_Var(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_گرہ_FTask* ثابت newp = VN_AS(m_memberMap.لبھو_Member(nodep, "new"), گرہ_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                جوڑو_سیٹ_Static_Rand_منوال(newp, genp, static_Rand_منوال_Varp);
            }
        } وإلا {
            beginValp = جديد AstConst{fl, AstConst::Widthed_قدر{}, 32, 1};
        }

        جوڑو_Update_Rand_Vars_Body(nodep, randomizep);

        AstFunc* ثابت basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        جوڑو_Basic_Randomize_Body(basicRandomizep, nodep, rand_منوال_Varp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ثابت fvarRefp = جديد AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->جوڑو_Stmtsp(جديد Ast_سونپو{
            fl, fvarRefp, basic_پہلا ? جديد AstFuncRef{fl, basicRandomizep} : beginValp});

        ثابت خودکار ناپ_Arrays_It = m_ناپ_Constrained_Arrays.لبھو(nodep);
        ثابت منطقي needs_ناپ_Phase
            = ناپ_Arrays_It != m_ناپ_Constrained_Arrays.سرا() && !ناپ_Arrays_It->دوجا.خالی_تھاواں();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ثابت خودکار emitResizeFallback = [&]() {
            إذا (needs_ناپ_Phase) أرجع;
            إذا (AstTask* ثابت resizeAllTaskp
                = VN_AS(m_memberMap.لبھو_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ثابت resizeTaskRefp = جديد AstTaskRef{fl, resizeAllTaskp};
                randomizep->جوڑو_Stmtsp(resizeTaskRefp->makeStmt());
            }
        };
        إذا (!basic_پہلا) emitResizeFallback();

        AstVarRef* ثابت fvarRefReadp = fvarRefp->clone_درخت(خطأ);
        fvarRefReadp->access(VAccess::READ);

        Ast_گرہ_اظہاریہ* ثابت دوجا_Halfp
            = basic_پہلا ? beginValp : جديد AstFuncRef{fl, basicRandomizep};
        randomizep->جوڑو_Stmtsp(جديد Ast_سونپو{fl, fvarRefp->clone_درخت(خطأ),
                                            جديد AstAnd{fl, fvarRefReadp, دوجا_Halfp}});

        إذا (basic_پہلا) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(nodep)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->جوڑو_Stmtsp((جديد AstTaskRef{fl, postTaskp})->makeStmt());
        }

        جوڑو_Pre_Post_Call(nodep, randomizep, "post_randomize");
        nodep->user1(خطأ);
    }
    فراغ ویکھو(AstRandCase* nodep) override {
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
        Ast_گرہ_DType* ثابت جوڑ_DTypep = nodep->لبھو_UInt64_DType();

        File_سطر* ثابت fl = nodep->fileline();
        ثابت std::string name = "__Vrandcase" + cvt_منزل_Str(m_rand_Case_عدد++);
        AstVar* ثابت randVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, name, جوڑ_DTypep};
        randVarp->noSubst(سچ);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        جوڑو_عارضی_Varsp(randVarp);
        Ast_گرہ_اظہاریہ* sump = جديد AstConst{fl, AstConst::Widthed_قدر{}, 64, 0};
        Ast_گرہ_If* ثابت پہلا_Ifsp
            = جديد AstIf{fl, جديد AstConst{fl, AstConst::ثنائی_ہندسہ_False{}}, خالی_سوچک, خالی_سوچک};
        Ast_گرہ_If* ifsp = پہلا_Ifsp;

        لكل (Ast_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {
            Ast_گرہ_اظہاریہ* ثابت condp = itemp->condsp()->unlink_Fr_پٹھ();
            sump
                = جديد Ast_جوڑو{condp->fileline(), sump, جديد AstExtend{itemp->fileline(), condp, 64}};
            Ast_گرہ* ثابت stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlink_Fr_پٹھ_With_اگلا() : خالی_سوچک;
            AstVarRef* ثابت randVarRefp = جديد AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_گرہ_If* ثابت newifp
                = جديد AstIf{itemp->fileline(),
                            جديد AstLte{condp->fileline(), randVarRefp, sump->clone_درخت_خالص(سچ)},
                            stmtsp, خالی_سوچک};
            ifsp->جوڑو_Elsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ثابت dispp = جديد AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            خالی_سوچک, خالی_سوچک};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->جوڑو_Elsesp(dispp);

        Ast_گرہ_اظہاریہ* ثابت randp = جديد AstRand{fl, خالی_سوچک, خطأ};
        randp->dtype_سیٹ_UInt64();
        AstVarRef* ثابت randVarRefp = جديد AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_گرہ* ثابت newp
            = جديد Ast_سونپو{fl, randVarRefp,
                            جديد Ast_جوڑو{fl, جديد AstConst{fl, AstConst::Unsized64{}, 1},
                                       جديد AstModDiv{fl, randp, sump}}};
        newp->جوڑو_اگلا(پہلا_Ifsp);
        إذا (debug() >= 9) newp->dump_درخت_And_اگلا(باہر, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    فراغ ویکھو(Ast_گرہ_FTask_Ref* nodep) override {
        إذا (nodep->name() == "rand_mode") {
            Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(nodep, طریقہ_Call);
            Ast_گرہ_اظہاریہ* ثابت fromp = طریقہ_Callp ? طریقہ_Callp->fromp() : خالی_سوچک;
            ثابت Rand_منوال_نشانہ rand_منوال_نشانہ = Rand_منوال_نشانہ::get(fromp, m_modp);
            UASSERT_OBJ(rand_منوال_نشانہ.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ثابت receiverp = rand_منوال_نشانہ.receiverp;
            ثابت منطقي isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_گرہ* classLevelStaticLoopp = خالی_سوچک;
            إذا (isClassLevel && nodep->argsp()) {
                إذا (AstVar* ثابت sVarp = get_Static_Rand_منوال_Var(rand_منوال_نشانہ.classp)) {
                    File_سطر* ثابت fl = nodep->fileline();
                    Ast_گرہ_اظہاریہ* ثابت staticLhsp
                        = make_منوال_سونپو_Lhs(fl, rand_منوال_نشانہ.classp, خالی_سوچک, sVarp);
                    Ast_گرہ_اظہاریہ* ثابت argClonep = nodep->argsp()->exprp()->clone_درخت_خالص(خطأ);
                    classLevelStaticLoopp = make_منوال_سیٹ_Loop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ثابت منطقي receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ثابت rand_منوال_Varp = receiverIsStaticRand
                                             ? get_Static_Rand_منوال_Var(rand_منوال_نشانہ.classp)
                                             : get_Rand_منوال_Var_From_Class(rand_منوال_نشانہ.classp);
            إذا (!rand_منوال_Varp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_اظہاریہ), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(ڈالو_Deletep(m_stmtp), m_stmtp = خالی_سوچک);
                أرجع;
            }
            Ast_گرہ_اظہاریہ* ثابت lhsp = make_منوال_سونپو_Lhs(nodep->fileline(), rand_منوال_نشانہ.classp,
                                                        rand_منوال_نشانہ.fromp, rand_منوال_Varp);
            replace_With_منوال_سونپو_And_Append(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : خالی_سوچک, lhsp,
                classLevelStaticLoopp);
            أرجع;
        }

        إذا (nodep->name() == "constraint_mode") {
            Ast_طریقہ_Call* ثابت طریقہ_Callp = VN_CAST(nodep, طریقہ_Call);
            Ast_گرہ_اظہاریہ* fromp = طریقہ_Callp ? طریقہ_Callp->fromp() : خالی_سوچک;
            AstConstraint* constrp = خالی_سوچک;
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
            AstVar* ثابت constraint_منوال_Varp = (constrp && constrp->isStatic())
                                                   ? get_Static_Constraint_منوال_Var(classp)
                                                   : get_Constraint_منوال_Var(classp);
            Ast_گرہ_اظہاریہ* ثابت lhsp
                = make_منوال_سونپو_Lhs(nodep->fileline(), classp, fromp, constraint_منوال_Varp);
            replace_With_منوال_سونپو(nodep, constrp, lhsp);
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
            ثابت منطقي وچ_Static_Context = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ثابت stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ثابت randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            إذا (وچ_Static_Context) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(سچ);
            }
            randomizeFuncp->جوڑو_Stmtsp(
                جديد Ast_سونپو{nodep->fileline(),
                              جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              جديد AstConst{nodep->fileline(), AstConst::Widthed_قدر{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            صحيح argn = 0;
            AstWith* ثابت withp = nodep->withp();
            لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_گرہ_اظہاریہ* ثابت exprp = argp->exprp();
                AstCMethodHard* ثابت basicMethodp = جديد AstCMethodHard{
                    nodep->fileline(),
                    جديد AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ثابت refvarp
                    = جديد AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::منزل_string(++argn), exprp->dtypep()};
                refvarp->سیٹ_Std_Randomize_Arg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                إذا (withp) {
                    withp->foreach([&](Ast_گرہ_اظہاریہ* قوت_نما) {
                        إذا (is_Similar_گرہ(قوت_نما, exprp)) {
                            AstVarRef* ثابت replaceVar
                                = جديد AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            قوت_نما->replaceWith(replaceVar);
                            replaceVar->user1(قوت_نما->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(ڈالو_Deletep(قوت_نما), قوت_نما);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(سچ);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->جوڑو_Stmtsp(refvarp);

                ثابت ناپ_t چوڑائی = exprp->چوڑائی();
                basicMethodp->جوڑو_Pinsp(
                    جديد AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->جوڑو_Pinsp(
                    جديد AstConst{nodep->fileline(), AstConst::Unsized64{}, چوڑائی});
                basicMethodp->dtype_سیٹ_ثنائی_ہندسہ();

                randomizeFuncp->جوڑو_Stmtsp(جديد Ast_سونپو{
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
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, خالی_سوچک,
                                                                وچ_Static_Context);
                withCapturep->جوڑو_Function_Arguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                إذا (stdrand) {
                    randomizeFuncp->جوڑو_Stmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_گرہ* ثابت capturedTreep = withp->exprp()->unlink_Fr_پٹھ_With_اگلا();
                randomizeFuncp->جوڑو_Stmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_اظہاریہ_Visitor{
                        خالی_سوچک, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        خالی_سوچک, m_writtenVars, &m_uniqueConstraintId, m_unique_ناں,  خالی_سوچک};
                }
                AstCExpr* ثابت solverCallp = جديد AstCExpr{fl};
                solverCallp->dtype_سیٹ_ثنائی_ہندسہ();
                solverCallp->جوڑو(جديد AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->جوڑو(".next()");
                AstVar* ثابت fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_گرہ_اظہاریہ* ثابت andExprp
                    = جديد AstAnd{fl, جديد AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->جوڑو_Stmtsp(
                    جديد Ast_سونپو{fl, جديد AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlink_Fr_پٹھ()->delete_درخت(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            إذا (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            إذا (withCapturep) nodep->جوڑو_Argsp(withCapturep->get_دلیلاں());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            أرجع;
        }
        handle_Randomize_دلیلاں(nodep);

        AstWith* ثابت withp = nodep->withp();
        إذا (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            أرجع;
        }
        withp->unlink_Fr_پٹھ();
        iterateChildren(nodep);

        AstClass* classp = خالی_سوچک;
        إذا (Ast_طریقہ_Call* ثابت callp = VN_CAST(nodep, طریقہ_Call)) {
            ثابت Ast_گرہ_DType* ثابت fromDTypep = callp->fromp()->dtypep();
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
                         classp->لبھو_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(سچ);

        AstFunc* ثابت randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inline_Unique_ناں.get(nodep), خطأ);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ثابت std::pair<منطقي, منطقي> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<منطقي, منطقي>{خطأ, خطأ};
        إذا (prePostWrap.پہلا) {
            جوڑو_Virtual_Pre_Post_Call(randomizeFuncp, classp, "pre_randomize");
        } وإلا {
            جوڑو_Pre_Post_Call(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(classp)) {
            AstTask* ثابت preTaskp = getCreateNestedCallbackTask(classp, "pre");
            إذا (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->جوڑو_Stmtsp((جديد AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ثابت CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.جوڑو_Function_Arguments(randomizeFuncp);

        // Add constraints clearing code
        إذا (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ثابت mainRandomizep
                = VN_CAST(m_memberMap.لبھو_Member(classp, "randomize"), Func);
            إذا (mainRandomizep && mainRandomizep->stmtsp()) {
                لكل (Ast_گرہ* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    منطقي لبھیا_Clear_Constraints = خطأ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        إذا (methodp->طریقہ() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->طریقہ() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->جوڑو_Stmtsp(stmtp->clone_درخت(خطأ));
                        } وإلا إذا (methodp->طریقہ() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            لبھیا_Clear_Constraints = سچ;
                        }
                    });
                    إذا (لبھیا_Clear_Constraints) اكسر;
                }
            }
            randomizeFuncp->جوڑو_Stmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->جوڑو_Stmtsp(localGenp);

        AstFunc* ثابت basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ثابت basicRandomizeFuncCallp
            = جديد AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        إذا (classGenp) {
            AstTask* ثابت constr_تیاری_Funcp = get_Create_Constraint_تیاری_Func(classp, m_memberMap);
            AstTaskRef* ثابت callp = جديد AstTaskRef{nodep->fileline(), constr_تیاری_Funcp};
            randomizeFuncp->جوڑو_Stmtsp(callp->makeStmt());
            randomizeFuncp->جوڑو_Stmtsp(جديد Ast_سونپو{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                جديد AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), گرہ_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ثابت rand_منوال_Varp = get_Rand_منوال_Var_From_Class(classp);
        إذا (!classGenp && rand_منوال_Varp) جوڑو_سیٹ_Rand_منوال(randomizeFuncp, localGenp, rand_منوال_Varp);
        إذا (!classGenp) {
            إذا (AstVar* ثابت sVarp = get_Static_Rand_منوال_Var(classp)) {
                جوڑو_سیٹ_Static_Rand_منوال(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_گرہ* ثابت capturedTreep = withp->exprp()->unlink_Fr_پٹھ_With_اگلا();
        randomizeFuncp->جوڑو_Stmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_گرہ* inlineResizeStmtsp = خالی_سوچک;
        {
            std::سمتیہ<AstCMethodHard*> ناپ_Methodps;
            capturedTreep->foreach_And_اگلا([&](AstCMethodHard* methodp) {
                إذا (methodp->طریقہ() == VCMethod::DYN_ناپ
                    || methodp->طریقہ() == VCMethod::ASSOC_ناپ) {
                    ناپ_Methodps.ڈالو_پٹھ(methodp);
                }
            });
            لكل (AstCMethodHard* ثابت methodp : ناپ_Methodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = خالی_سوچک;
                Ast_گرہ_اظہاریہ* ثابت جڑ_Exprp = methodp->fromp();
                إذا (AstMemberSel* memberSelp = VN_CAST(جڑ_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    طالما (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ثابت Ast_گرہ_اظہاریہ* ثابت fromp = memberSelp->fromp();
                    إذا (ثابت AstVarRef* ثابت varRefp = VN_CAST(fromp, VarRef)) {
                        إذا (varRefp->varp() == captured.getThisp()) استمر;
                    } وإلا {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } وإلا إذا (AstVarRef* ثابت varRefp = VN_CAST(جڑ_Exprp, VarRef)) {
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
                AstVar* ثابت ناپ_Varp = create_Or_Get_ناپ_Var(varClassp, arrVarp, fl,
                                                            methodp->لبھو_Int_DType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                إذا (wasCreated) {
                    Ast_گرہ_اظہاریہ* ثابت resizeVarRefp = buildMemberSelExprp(جڑ_Exprp, arrVarp);
                    Ast_گرہ_اظہاریہ* ثابت ناپ_Exprp = buildMemberSelExprp(جڑ_Exprp, ناپ_Varp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->جوڑو_اگلا(create_ناپ_Gte_Zero_Constraint(fl, ناپ_Varp, جڑ_Exprp));
                    AstCMethodHard* ثابت resizep
                        = جديد AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, ناپ_Exprp};
                    resizep->dtypep(methodp->لبھو_Void_DType());
                    inlineResizeStmtsp
                        = Ast_گرہ::جوڑو_اگلا(inlineResizeStmtsp, جديد Ast_Stmt_اظہاریہ{fl, resizep});
                }
                Ast_گرہ_اظہاریہ* ثابت ناپ_گرہ_Exprp = buildMemberSelExprp(جڑ_Exprp, ناپ_Varp);
                methodp->replaceWith(ناپ_گرہ_Exprp);
                VL_DO_DANGLING(methodp->delete_درخت(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_اظہاریہ_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                rand_منوال_Varp, m_writtenVars, &m_uniqueConstraintId, m_unique_ناں,  خالی_سوچک};
        }

        // Call the solver and set return value
        AstCExpr* ثابت solverCallp = جديد AstCExpr{nodep->fileline()};
        solverCallp->dtype_سیٹ_ثنائی_ہندسہ();
        solverCallp->جوڑو(جديد AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->جوڑو(".next(__Vm_rng)");
        randomizeFuncp->جوڑو_Stmtsp(جديد Ast_سونپو{
            nodep->fileline(),
            جديد AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            جديد AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        إذا (inlineResizeStmtsp) {
            AstVar* ثابت retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ثابت ifp = جديد AstIf{nodep->fileline(),
                                         جديد AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, خالی_سوچک};
            randomizeFuncp->جوڑو_Stmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        إذا (classHasRandClassMembers(classp)) {
            AstTask* ثابت postTaskp = getCreateNestedCallbackTask(classp, "post");
            إذا (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->جوڑو_Stmtsp((جديد AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        إذا (prePostWrap.دوجا) {
            جوڑو_Virtual_Pre_Post_Call(randomizeFuncp, classp, "post_randomize");
        } وإلا {
            جوڑو_Pre_Post_Call(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->جوڑو_Argsp(captured.get_دلیلاں());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_درخت(), withp);
    }
    فراغ ویکھو(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_سطر* ثابت fl = nodep->fileline();
        إذا (m_constraintp && nodep->fromp()->user1()
            && (nodep->طریقہ() == VCMethod::ASSOC_ناپ
                || nodep->طریقہ() == VCMethod::DYN_ناپ)) {
            AstClass* ثابت classp = VN_AS(m_modp, Class);
            AstVarRef* ثابت قطار_Var_Refp = VN_CAST(nodep->fromp(), VarRef);
            إذا (!قطار_Var_Refp) {
                // Warning from ConstraintExprVisitor will be thrown
                أرجع;
            }
            AstVar* ثابت قطار_Varp = قطار_Var_Refp->varp();
            منطقي wasCreated = خطأ;
            AstVar* ثابت ناپ_Varp
                = create_Or_Get_ناپ_Var(classp, قطار_Varp, fl, nodep->لبھو_Int_DType(), wasCreated);
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
                    AstVarRef* ثابت ناپ_Var_Refp = جديد AstVarRef{
                        fl, VN_AS(ناپ_Varp->user2p(), گرہ_Module), ناپ_Varp, VAccess::READ};
                    AstCMethodHard* ثابت resizep = جديد AstCMethodHard{
                        fl, nodep->fromp()->unlink_Fr_پٹھ(), VCMethod::DYN_RESIZE, ناپ_Var_Refp};
                    resizep->dtypep(nodep->لبھو_Void_DType());
                    resizerTaskp->جوڑو_Stmtsp(جديد Ast_Stmt_اظہاریہ{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->جوڑو_Itemsp(create_ناپ_Gte_Zero_Constraint(fl, ناپ_Varp));
            }
            AstVarRef* ثابت ناپ_Var_Refp = جديد AstVarRef{fl, VN_AS(ناپ_Varp->user2p(), گرہ_Module),
                                                         ناپ_Varp, VAccess::READ};
            ناپ_Var_Refp->user1(سچ);
            nodep->replaceWith(ناپ_Var_Refp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح RandomizeVisitor(AstNetlist* nodep)
        : m_inline_Unique_ناں{"__Vrandwith"} {
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
                    إذا (VN_AS(varp->user2p(), گرہ_Module) != ownerClassp) أرجع;
                    إذا (!varp->user3()) varp->user3(سچ);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(ڈالو_Deletep(constrp->unlink_Fr_پٹھ()), constrp);
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
                                       منطقي بچہ_DType) {
    AstFunc* funcp = VN_AS(memberMap.لبھو_Member(nodep, name), Func);
    إذا (!funcp) {
        v3Global.useRandomizeMethods(سچ);
        Ast_گرہ_DType* ثابت dtypep
            = بچہ_DType
                  ? جديد AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->لبھو_ثنائی_ہندسہ_DType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ثابت fvarp = بچہ_DType
                                  ? جديد AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlag_بچہ_DType{}, dtypep}
                                  : جديد AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3_Error_کوڈ::NORETURN, سچ);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(سچ);
        fvarp->funcReturn(سچ);
        fvarp->direction(VDirection::OUTPUT);
        nodep->جوڑو_Membersp(funcp);
        funcp = جديد AstFunc{nodep->fileline(), name, خالی_سوچک, fvarp};
        إذا (!بچہ_DType) funcp->dtypep(dtypep);
        funcp->class_طریقہ(سچ);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->جوڑو_Membersp(funcp);
        memberMap.داخل_کرو(nodep, funcp);
    }
    أرجع funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_گرہ_Module* nodep, ثابت std::string& name) {
    AstFunc* funcp = خالی_سوچک;
    v3Global.useRandomizeMethods(سچ);
    Ast_گرہ_DType* ثابت dtypep = nodep->لبھو_ثنائی_ہندسہ_DType(32, 32, VSigning::SIGNED);
    AstVar* ثابت fvarp = جديد AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(سچ);
    fvarp->funcReturn(سچ);
    fvarp->direction(VDirection::OUTPUT);
    funcp = جديد AstFunc{nodep->fileline(), name, خالی_سوچک, fvarp};
    funcp->dtypep(dtypep);
    إذا (VN_IS(nodep, Class)) {
        funcp->class_طریقہ(سچ);
    } وإلا {
        funcp->class_طریقہ(سچ);
        funcp->isStatic(سچ);
    }
    nodep->جوڑو_Stmtsp(funcp);
    أرجع funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ثابت basep = nodep->ادھار_Most_Classp();
    AstFunc* funcp = VN_AS(memberMap.لبھو_Member(basep, "srandom"), Func);
    إذا (!funcp) {
        v3Global.useRandomizeMethods(سچ);
        Ast_گرہ_DType* ثابت dtypep
            = basep->لبھو_ثنائی_ہندسہ_DType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ثابت ivarp = جديد AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(سچ);
        ivarp->direction(VDirection::داخلہ);
        funcp = جديد AstFunc{basep->fileline(), "srandom", ivarp, خالی_سوچک};
        funcp->dtypep(basep->لبھو_Void_DType());
        funcp->class_طریقہ(سچ);
        funcp->isVirtual(خطأ);
        basep->جوڑو_Membersp(funcp);
        memberMap.داخل_کرو(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        إذا (basep->name() == "process") {
            funcp->جوڑو_Stmtsp(جديد AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } وإلا {
            funcp->جوڑو_Stmtsp(جديد AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(سچ);
        }
    }
    أرجع funcp;
}
