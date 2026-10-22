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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Randomize.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Container.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <queue>
#ਅਨ੍ਤਰ੍ਭਾਵ <tuple>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

ਗਿਣਤੀ ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
ਸਥਿਰ AstVar* get_Rand_ਬਹੁਲਕ_Var_From_Class(Ast_ਗੰਢ_Module* classp) {
    ਜਦੋਂ (classp) {
        ਜੇ (classp->user2p()) ਵਾਪਸ VN_AS(classp->user2p(), Var);
        AstClass* ਅਚਲ cp = VN_CAST(classp, Class);
        ਜੇ (!cp || !cp->extendsp()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        classp = cp->extendsp()->classp();
    }
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ਸਥਿਰ AstVar* getRandomGenerator(AstClass* ਅਚਲ classp) {
    ਜੇ (classp->user3p()) ਵਾਪਸ VN_AS(classp->user3p(), Var);
    ਜੇ (classp->extendsp()) ਵਾਪਸ getRandomGenerator(classp->extendsp()->classp());
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ਸਥਿਰ AstTask* get_Create_Constraint_ਤਿਆਰੀ_Func(AstClass* classp, VMemberMap& memberMap) {
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ name = "__Vsetup_constraints";
    AstTask* ਤਿਆਰੀ_All_Taskp = VN_AS(memberMap.ਲੱਭੋ_Member(classp, name), Task);
    ਜੇ (ਤਿਆਰੀ_All_Taskp) ਵਾਪਸ ਤਿਆਰੀ_All_Taskp;
    ਤਿਆਰੀ_All_Taskp = ਨਵਾਂ AstTask{classp->fileline(), "__Vsetup_constraints", ਖਾਲੀ_ਸੂਚਕ};
    ਤਿਆਰੀ_All_Taskp->class_ਵਿਧੀ(ਸੱਚ);
    ਤਿਆਰੀ_All_Taskp->isVirtual(ਸੱਚ);
    classp->ਜੋੜੋ_Membersp(ਤਿਆਰੀ_All_Taskp);
    memberMap.ਪਾਓ(classp, ਤਿਆਰੀ_All_Taskp);
    ਵਾਪਸ ਤਿਆਰੀ_All_Taskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

ਬਣਤਰ Rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ final {
    // MEMBERS
    AstVar* ਅਚਲ receiverp;  // Variable that is the target of the rand_mode() call
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp;  // Expression from which the rand_mode() call originates
    AstClass* ਅਚਲ classp;  // Class in which rand_mode should be set

    // METHODS
    ਸਥਿਰ Rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ get(Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_Module* modp) {
        ਜੇ (!fromp) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, VN_AS(modp, Class)};
        ਜੇ (AstCMethodHard* ਅਚਲ ਵਿਧੀ_Hardp = VN_CAST(fromp, CMethodHard)) {
            ਵਾਪਸ Rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ::get(ਵਿਧੀ_Hardp->fromp(), modp);
        }
        AstVar* receiverp = ਖਾਲੀ_ਸੂਚਕ;
        AstClass* classp = VN_CAST(modp, Class);
        ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            ਜੇ (receiverp->isRand()) {
                fromp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            ਜੇ (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        ਜੇ (!receiverp->isRand()) {
            AstClassRefDType* ਅਚਲ classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            ਜੇ (classRefDtp) classp = classRefDtp->classp();
        }
        ਵਾਪਸ {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

ਸੰਘ Randomize_ਬਹੁਲਕ final {
    // MEMBERS
    ਬਣਤਰ {
        ਤਾਰਕਿਕ ਵਰਤਦਾ_ਬਹੁਲਕ : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t ਸੂਚਕਾਂਕ : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ਸਥਿਰ Ast_ਗੰਢ_DType* arrayElementDTypep(Ast_ਗੰਢ_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ਜਦੋਂ (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    ਵਾਪਸ dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ਸਥਿਰ ਤਾਰਕਿਕ dtype_ਸ਼ਾਮਲ_ਹੈ_Real(Ast_ਗੰਢ_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ subp = dtypep->subDTypep()) ਵਾਪਸ dtype_ਸ਼ਾਮਲ_ਹੈ_Real(subp);
    ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ structp = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType)) {
        ਜੇ (structp->packed()) ਵਾਪਸ ਝੂਠ;
        ਲਈ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            ਜੇ (dtype_ਸ਼ਾਮਲ_ਹੈ_Real(memberp->subDTypep())) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਅਚਲ AstBasicDType* ਅਚਲ basicp = VN_CAST(dtypep, BasicDType);
    ਵਾਪਸ basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
ਸਥਿਰ ਖਾਲੀ checkRandTypeEligibility(Ast_ਗੰਢ* contextp, Ast_ਗੰਢ_DType* dtypep, ਤਾਰਕਿਕ isRandc) {
    dtypep = dtypep->skipRefp();
    ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        ਵਾਪਸ;
    }
    ਜੇ (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        ਜੇ (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        ਵਾਪਸ;
    }
    ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        ਵਾਪਸ;
    }
    ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ structp = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType)) {
        ਜੇ (structp->packed()) ਵਾਪਸ;  // Integral by construction; members checked separately
        ਜੇ (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            ਵਾਪਸ;
        }
        ਲਈ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            ਜੇ (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        ਵਾਪਸ;
    }
    ਅਚਲ AstBasicDType* ਅਚਲ basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    ਜੇ (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        ਜੇ (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        ਵਾਪਸ;
    }
    ਚੋਣ (basicp->keyword()) {
    ਹਾਲਤ VBasicDTypeKwd::STRING:
    ਹਾਲਤ VBasicDTypeKwd::CHANDLE:
    ਹਾਲਤ VBasicDTypeKwd::EVENT: {
        ਅਚਲ ਅੱਖਰ* ਅਚਲ articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        ਰੋਕੋ;
    }
    ਮੂਲ: ਰੋਕੋ;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

ਵਰਗ RandomizeMarkVisitor final : ਜਨਤਕ VNVisitor {
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
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    ਵਰਤੋਂ Derived_ਸੈੱਟ = std::unordered_ਸੈੱਟ<AstClass*>;
    ਵਰਤੋਂ ਅਧਾਰ_ਮੰਜ਼ਿਲ_Derived_Map = std::unordered_map<ਅਚਲ AstClass*, Derived_ਸੈੱਟ>;

    ਅਧਾਰ_ਮੰਜ਼ਿਲ_Derived_Map m_ਅਧਾਰ_ਮੰਜ਼ਿਲ_Derived_Map;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;  // Current class
    Ast_ਗੰਢ* m_constraint_ਵਿਅੰਜਕ_Genp = ਖਾਲੀ_ਸੂਚਕ;  // Current constraint or constraint if expression
    Ast_ਗੰਢ_Module* m_modp;  // Current module
    Ast_ਗੰਢ_Stmt* m_stmtp = ਖਾਲੀ_ਸੂਚਕ;  // Current statement
    Ast_ਗੰਢ_FTask_Ref* m_stdRandCallp = ਖਾਲੀ_ਸੂਚਕ;  // Current std::randomize() call
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Std_With = ਝੂਠ;  // True when inside a 'with {}' clause
    std::ਸੈੱਟ<Ast_ਗੰਢ_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = ਖਾਲੀ_ਸੂਚਕ;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    ਖਾਲੀ markGlobalConstrainedVars(AstClass* classp) {
        ਲਈ (ਅਚਲ AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ) {
            ਲਈ (Ast_ਗੰਢ* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ਅਚਲ varp = VN_CAST(memberp, Var);
                ਜੇ (!varp) ਜਾਰੀ;
                ਜੇ (varp->rand().isRandomizable()) varp->globalConstrained(ਸੱਚ);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    ਤਾਰਕਿਕ is_Var_ਵਿੱਚ_Std_Randomize_ਦਲੀਲਾਂ(ਅਚਲ AstVar* varp) ਅਚਲ {
        ਜੇ (!m_ਵਿੱਚ_Std_With || !m_stdRandCallp) ਵਾਪਸ ਝੂਠ;

        ਲਈ (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            ਜਦੋਂ (exprp) {
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(exprp, ਗੰਢ_Var_Ref)) {
                    ਜੇ (varrefp->varp() == varp) ਵਾਪਸ ਸੱਚ;
                    ਰੋਕੋ;
                }
                ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ memberselp = VN_CAST(exprp, MemberSel)) {
                    ਜੇ (memberselp->varp() == varp) ਵਾਪਸ ਸੱਚ;
                    exprp = memberselp->fromp();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstCMethodHard* ਅਚਲ methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } ਨਹੀਂਤਾਂ {
                    ਰੋਕੋ;
                }
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਖਾਲੀ markMembers(ਅਚਲ AstClass* nodep) {
        ਲਈ (ਅਚਲ AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ) {
            ਲਈ (Ast_ਗੰਢ* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ਅਚਲ varp = VN_CAST(memberp, Var);
                ਜੇ (!varp) ਜਾਰੀ;
                // If member is randomizable and of class type, mark its class
                ਜੇ (varp->rand().isRandomizable()) {
                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ varDtypep = varp->dtypep()->skipRefp();
                    ਅਚਲ AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    ਜੇ (!classRefp) {
                        ਅਚਲ Ast_ਗੰਢ_DType* subDTypep = varDtypep->subDTypep();
                        ਜੇ (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    ਜੇ (classRefp) {
                        AstClass* ਅਚਲ rclassp = classRefp->classp();
                        ਜੇ (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    ਜੇ (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {};
                        rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ = ਸੱਚ;
                        varp->user1(rand_ਬਹੁਲਕ.asUQuad);
                    }
                }
            }
        }
    }
    ਖਾਲੀ markDerived(ਅਚਲ AstClass* nodep) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਅਧਾਰ_ਮੰਜ਼ਿਲ_Derived_Map.ਲੱਭੋ(nodep);
        ਜੇ (it != m_ਅਧਾਰ_ਮੰਜ਼ਿਲ_Derived_Map.ਸਿਰਾ()) {
            ਲਈ (ਸਵੈਚਾਲਿਤ* classp : it->ਦੂਜਾ) {
                ਜੇ (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    ਖਾਲੀ markAllDerived() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& p : m_ਅਧਾਰ_ਮੰਜ਼ਿਲ_Derived_Map) {
            ਜੇ (p.ਪਹਿਲਾ->user1()) markDerived(p.ਪਹਿਲਾ);
        }
    }
    ਖਾਲੀ ਸੈੱਟ_Package_Refs() {
        ਲਈ (Ast_ਗੰਢ_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), ਗੰਢ_Module));
        }
    }
    ਖਾਲੀ markNestedGlobalConstrainedRecurse(Ast_ਗੰਢ* nodep) {
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
            AstVar* ਅਚਲ varp = refp->varp();
            ਜੇ (varp->globalConstrained()) ਵਾਪਸ;
            varp->globalConstrained(ਸੱਚ);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ memberSelp = VN_CAST(nodep, MemberSel)) {
            ਜੇ (memberSelp->varp()) {
                AstVar* ਅਚਲ varp = memberSelp->varp();
                ਜੇ (varp->globalConstrained()) ਵਾਪਸ;
                varp->globalConstrained(ਸੱਚ);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* buildMemberSelChain(AstVarRef* ਜੜ੍ਹ_Var_Refp, ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& ਰਸਤਾ) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = ਜੜ੍ਹ_Var_Refp->clone_ਰੁੱਖ(ਝੂਠ);
        ਲਈ (AstVar* memberVarp : ਰਸਤਾ) {
            AstMemberSel* memberSelp
                = ਨਵਾਂ AstMemberSel{ਜੜ੍ਹ_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        ਵਾਪਸ exprp;
    }

    // Process a single constraint during nested constraint cloning
    ਖਾਲੀ processNestedConstraint(AstConstraint* ਅਚਲ constrp, AstVarRef* ਜੜ੍ਹ_Var_Refp,
                                 ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& new_ਰਸਤਾ, AstClass* ਨਿਸ਼ਾਨਾ_Classp) {
        std::string ਰਸਤਾ_ਅਗੇਤਰ = ਜੜ੍ਹ_Var_Refp->name();
        ਲਈ (AstVar* ਰਸਤਾ_Member_Varp : new_ਰਸਤਾ) {
            ਰਸਤਾ_ਅਗੇਤਰ += GLOBAL_CONSTRAINT_SEPARATOR + ਰਸਤਾ_Member_Varp->name();
        }

        ਅਚਲ std::string newName = ਰਸਤਾ_ਅਗੇਤਰ + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        ਤਾਰਕਿਕ isDuplicate = ਝੂਠ;
        ਨਿਸ਼ਾਨਾ_Classp->foreachMember([&](AstClass* ਅਚਲ, AstConstraint* ਅਚਲ existingConstrp) {
            ਜੇ (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = ਸੱਚ;
            }
        });
        ਜੇ (isDuplicate) ਵਾਪਸ;

        AstConstraint* ਅਚਲ cloneConstrp = constrp->clone_ਰੁੱਖ(ਝੂਠ);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::ਸਦਿਸ਼<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
            ਜੇ (AstVarRef* ਅਚਲ varRefp = VN_CAST(exprp, VarRef)) {
                ਜੇ (!varRefp->varp()->isClassMember()) ਵਾਪਸ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ chainp = buildMemberSelChain(ਜੜ੍ਹ_Var_Refp, new_ਰਸਤਾ);
                AstMemberSel* ਅਚਲ finalSelp
                    = ਨਵਾਂ AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(varRefp), varRefp);
            } ਨਹੀਂਤਾਂ ਜੇ (AstFuncRef* ਅਚਲ funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                ਜੇ (funcRefp->taskp()->class_ਵਿਧੀ() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.ਧੱਕੋ_ਪਿੱਠ(funcRefp);
                }
            }
        });
        ਲਈ (AstFuncRef* funcRefp : funcRefps) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ chainp = buildMemberSelChain(ਜੜ੍ਹ_Var_Refp, new_ਰਸਤਾ);
            AstArg* ਅਚਲ argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ() : ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਵਿਧੀ_Call* ਅਚਲ callp
                = ਨਵਾਂ Ast_ਵਿਧੀ_Call{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        ਨਿਸ਼ਾਨਾ_Classp->ਜੋੜੋ_Stmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    ਖਾਲੀ cloneNestedConstraintsRecurse(AstVarRef* ਜੜ੍ਹ_Var_Refp, AstClass* classp,
                                       ਅਚਲ std::ਸਦਿਸ਼<AstVar*>& ਰਸਤਾ_ਮੰਜ਼ਿਲ_Class,
                                       AstClass* ਨਿਸ਼ਾਨਾ_Classp) {
        ਲਈ (Ast_ਗੰਢ* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ਅਚਲ memberVarp = VN_CAST(memberNodep, Var);
            ਜੇ (!memberVarp) ਜਾਰੀ;
            ਜੇ (!memberVarp->rand().isRandomizable()) ਜਾਰੀ;
            ਅਚਲ AstClassRefDType* ਅਚਲ memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            ਜੇ (!memberClassRefp || !memberClassRefp->classp()) ਜਾਰੀ;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::ਸਦਿਸ਼<AstVar*> new_ਰਸਤਾ = ਰਸਤਾ_ਮੰਜ਼ਿਲ_Class;
            new_ਰਸਤਾ.ਧੱਕੋ_ਪਿੱਠ(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ਅਚਲ containingClassp, AstConstraint* ਅਚਲ constrp) {
                    processNestedConstraint(constrp, ਜੜ੍ਹ_Var_Refp, new_ਰਸਤਾ, ਨਿਸ਼ਾਨਾ_Classp);
                });
            cloneNestedConstraintsRecurse(ਜੜ੍ਹ_Var_Refp, nestedClassp, new_ਰਸਤਾ, ਨਿਸ਼ਾਨਾ_Classp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    ਖਾਲੀ handle_Randomize_ਦਲੀਲ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstVar* ਅਚਲ varp, ਅਚਲ ਤਾਰਕਿਕ stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        ਜਦੋਂ (exprp) {
            AstVar* randVarp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } ਨਹੀਂਤਾਂ ਜੇ (AstArraySel* ਅਚਲ arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                ਜਾਰੀ;  // Skip ArraySel, continue traversing
            } ਨਹੀਂਤਾਂ ਜੇ (AstStructSel* ਅਚਲ structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                ਜਾਰੀ;  // Skip StructSel, continue traversing
            } ਨਹੀਂਤਾਂ ਜੇ (AstCMethodHard* ਅਚਲ methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                ਜਾਰੀ;
            } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(ਸੱਚ);
                varrefp->access(VAccess::READWRITE);
                exprp = ਖਾਲੀ_ਸੂਚਕ;
            } ਨਹੀਂਤਾਂ {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            ਜੇ (randVarp == varp) ਵਾਪਸ;
            Ast_ਗੰਢ* backp = randVarp;
            ਜਦੋਂ (backp && !VN_IS(backp, ਗੰਢ_Module)) backp = backp->backp();
            ਜੇ (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } ਨਹੀਂਤਾਂ {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {};
                rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ = ਸੱਚ;
                randVarp->user1(rand_ਬਹੁਲਕ.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        ਜੇ (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ਅਚਲ AstClass* ਅਚਲ basep = nodep->extendsp()->classp();
            m_ਅਧਾਰ_ਮੰਜ਼ਿਲ_Derived_Map[basep].ਪਾਓ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        ਜੇ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        ਜੇ (nodep->name() == "rand_mode") {
            Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(nodep, ਵਿਧੀ_Call);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = ਵਿਧੀ_Callp ? ਵਿਧੀ_Callp->fromp() : ਖਾਲੀ_ਸੂਚਕ;
            ਤਾਰਕਿਕ valid = ਸੱਚ;
            ਜੇ (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = ਝੂਠ;
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = ਝੂਠ;
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = ਝੂਠ;
            } ਨਹੀਂਤਾਂ ਜੇ (AstCMethodHard* ਅਚਲ ਵਿਧੀ_Hardp = VN_CAST(fromp, CMethodHard)) {
                ਜੇ (ਵਿਧੀ_Hardp->ਵਿਧੀ() == VCMethod::ARRAY_AT
                    || ਵਿਧੀ_Hardp->ਵਿਧੀ() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = ਝੂਠ;
                } ਨਹੀਂਤਾਂ {
                    ਵਿਧੀ_Hardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            ਜੇ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_ਕੋਡ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = ਝੂਠ;
            }
            ਜੇ (valid) {
                ਅਚਲ Rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ = Rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ::get(fromp, m_classp);
                ਜੇ ((!rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.receiverp || !rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = ਝੂਠ;
                } ਨਹੀਂਤਾਂ ਜੇ (!rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = ਝੂਠ;
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = ਝੂਠ;
                } ਨਹੀਂਤਾਂ ਜੇ (rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.receiverp && rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.receiverp->isRand()) {
                    // Called on a rand member variable
                    Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {};
                    rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ = ਸੱਚ;
                    rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.receiverp->user1(rand_ਬਹੁਲਕ.asUQuad);
                } ਨਹੀਂਤਾਂ {
                    // Called on 'this' or a non-rand class instance
                    rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        ਜੇ (!varp->isRand()) ਵਾਪਸ;
                        Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {};
                        rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ = ਸੱਚ;
                        varp->user1(rand_ਬਹੁਲਕ.asUQuad);
                    });
                }
            }
            ਜੇ (!valid) {
                ਜੇ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) {
                    nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                } ਨਹੀਂਤਾਂ {
                    m_stmtp->unlink_Fr_ਪਿੱਠ();
                }
            }
            ਵਾਪਸ;
        }

        ਜੇ (nodep->name() == "constraint_mode") {
            ਤਾਰਕਿਕ valid = ਸੱਚ;
            ਜੇ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = ਝੂਠ;
            } ਨਹੀਂਤਾਂ ਜੇ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)
                       && !nodep->backp()->fileline()->warnIsOff(V3_Error_ਕੋਡ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = ਝੂਠ;
            }
            AstConstraint* constrp = ਖਾਲੀ_ਸੂਚਕ;
            AstClass* classp = m_classp;
            ਜੇ (ਅਚਲ Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(nodep, ਵਿਧੀ_Call)) {
                ਜੇ (ਅਚਲ AstConstraintRef* ਅਚਲ constrRefp
                    = VN_CAST(ਵਿਧੀ_Callp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    ਜੇ (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } ਨਹੀਂਤਾਂ ਜੇ (AstClassRefDType* classRefDtp
                           = VN_CAST(ਵਿਧੀ_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } ਨਹੀਂਤਾਂ {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = ਝੂਠ;
                }
            }
            ਜੇ (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = ਝੂਠ;
            }
            ਜੇ (valid) {
                Randomize_ਬਹੁਲਕ constraint_ਬਹੁਲਕ = {};
                constraint_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ = ਸੱਚ;
                ਜੇ (constrp) {
                    constrp->user1(constraint_ਬਹੁਲਕ.asUQuad);
                } ਨਹੀਂਤਾਂ {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraint_ਬਹੁਲਕ.asUQuad);
                    });
                }
            } ਨਹੀਂਤਾਂ {
                ਜੇ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) {
                    nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                } ਨਹੀਂਤਾਂ {
                    m_stmtp->unlink_Fr_ਪਿੱਠ();
                }
            }
            ਵਾਪਸ;
        }

        ਜੇ (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            ਜੇ (m_constraint_ਵਿਅੰਜਕ_Genp || m_ਵਿੱਚ_Std_With) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                ਲਈ (Ast_ਗੰਢ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    ਜੇ (argp->user1()) {
                        nodep->user1(ਸੱਚ);
                        ਰੋਕੋ;
                    }
                }
            }
            ਵਾਪਸ;
        }
        AstClass* classp = m_classp;
        ਜੇ (ਅਚਲ Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(nodep, ਵਿਧੀ_Call)) {
            ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ classRefp
                = VN_CAST(ਵਿਧੀ_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        ਜੇ (classp) {
            ਜੇ (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ਅਚਲ, AstVar* ਅਚਲ memberVarp) {
                ਜੇ (!memberVarp->rand().isRandomizable()) ਵਾਪਸ;
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = memberVarp->dtypep()->skipRefp();
                ਅਚਲ AstClassRefDType* ਅਚਲ classRefp = VN_CAST(dtypep, ClassRefDType);
                ਜੇ (!classRefp || !classRefp->classp()) ਵਾਪਸ;
                AstClass* ਅਚਲ memberClassp = classRefp->classp();
                ਜੇ (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) ਵਾਪਸ;
                memberVarp->globalConstrained(ਸੱਚ);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* ਜੜ੍ਹ_Var_Refp
                    = ਨਵਾਂ AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::ਸਦਿਸ਼<AstVar*> ਖਾਲੀ_ਥਾਵਾਂ_ਰਸਤਾ;
                memberClassp->foreachMember([&](AstClass* ਅਚਲ, AstConstraint* ਅਚਲ constrp) {
                    processNestedConstraint(constrp, ਜੜ੍ਹ_Var_Refp, ਖਾਲੀ_ਥਾਵਾਂ_ਰਸਤਾ, classp);
                });
                cloneNestedConstraintsRecurse(ਜੜ੍ਹ_Var_Refp, memberClassp, ਖਾਲੀ_ਥਾਵਾਂ_ਰਸਤਾ, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(ਜੜ੍ਹ_Var_Refp->delete_ਰੁੱਖ(), ਜੜ੍ਹ_Var_Refp);
            });
        }
        ਜੇ (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handle_Randomize_ਦਲੀਲ(argp->exprp(), ਖਾਲੀ_ਸੂਚਕ, ਸੱਚ);
            }
            ਵਾਪਸ;
        }
        ਲਈ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->ਅੰਕ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ਅਚਲ ਤਾਰਕਿਕ hasUnsupportedMember
                    = classp->existsMember([](ਅਚਲ AstClass*, ਅਚਲ AstVar* memberVarp) {
                          ਜੇ (!memberVarp->rand().isRandomizable()) ਵਾਪਸ ਝੂਠ;
                          ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtp = memberVarp->dtypep()->skipRefp();
                          ਵਾਪਸ VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, ਕਤਾਰ_DType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                ਜੇ (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                ਜਾਰੀ;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = ਖਾਲੀ_ਸੂਚਕ;  // If nodep is a method call, this is its receiver
            ਜੇ (Ast_ਵਿਧੀ_Call* ਵਿਧੀ_Callp = VN_CAST(nodep, ਵਿਧੀ_Call)) {
                ਜੇ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(ਵਿਧੀ_Callp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } ਨਹੀਂਤਾਂ {
                    AstVarRef* ਅਚਲ varrefp = VN_AS(ਵਿਧੀ_Callp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handle_Randomize_ਦਲੀਲ(argp->exprp(), fromVarp, ਝੂਠ);
        }
        // Re-mark members after the class became inline-randomized above.
        ਜੇ (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_ਵਿਅੰਜਕ_Genp);
        m_stmtp = nodep;
        m_constraint_ਵਿਅੰਜਕ_Genp = nodep;
        iterateChildren(nodep);
        ਜੇ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Constraint_ਵਿਅੰਜਕ* nodep) override {
        VL_RESTORER(m_constraint_ਵਿਅੰਜਕ_Genp);
        m_constraint_ਵਿਅੰਜਕ_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_ਵਿਅੰਜਕ_Genp);
            m_constraint_ਵਿਅੰਜਕ_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
        iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (!m_constraint_ਵਿਅੰਜਕ_Genp) ਵਾਪਸ;

        ਜੇ (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        ਜੇ (nodep->varp()->rand().isRandomizable() && !(m_ਵਿੱਚ_Std_With && m_stdRandCallp)) {
            nodep->user1(ਸੱਚ);
        } ਨਹੀਂਤਾਂ ਜੇ (is_Var_ਵਿੱਚ_Std_Randomize_ਦਲੀਲਾਂ(nodep->varp())) {
            nodep->user1(ਸੱਚ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਜੇ (!m_constraint_ਵਿਅੰਜਕ_Genp) ਵਾਪਸ;
        iterateChildrenConst(nodep);
        ਅਚਲ ਤਾਰਕਿਕ randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ਅਚਲ ਤਾਰਕਿਕ randMember = nodep->varp()->rand().isRandomizable();
        ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Std_With = m_ਵਿੱਚ_Std_With && m_stdRandCallp;
        ਜੇ (randObject && randMember && !ਵਿੱਚ_Std_With) {
            nodep->user1(ਸੱਚ);
        } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Std_With && is_Var_ਵਿੱਚ_Std_Randomize_ਦਲੀਲਾਂ(nodep->varp())) {
            nodep->user1(ਸੱਚ);
            // Mark parent object for constraint expression visitor
            ਜੇ (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(ਸੱਚ);
        }

        ਜੇ (m_withp) {
            Ast_ਗੰਢ* backp = m_withp;
            ਜਦੋਂ (backp->backp()) {
                ਜੇ (ਅਚਲ Ast_ਵਿਧੀ_Call* ਅਚਲ callp = VN_CAST(backp, ਵਿਧੀ_Call)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    ਰੋਕੋ;
                }
                backp = backp->backp();
            }
        } ਨਹੀਂਤਾਂ {
            nodep->user2p(m_modp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        nodep->user2p(m_modp);
        ਜੇ (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        ਜੇ (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_ਵਿੱਚ_Std_With = ਸੱਚ;
        iterateChildrenConst(nodep);
        m_ਵਿੱਚ_Std_With = ਝੂਠ;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        iterateChildrenConst(nodep);
        ਜੇ (!m_constraint_ਵਿਅੰਜਕ_Genp && !m_ਵਿੱਚ_Std_With) ਵਾਪਸ;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    ਖਾਲੀ ਦੇਖੋ(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        ਜੇ (!m_constraint_ਵਿਅੰਜਕ_Genp && !m_ਵਿੱਚ_Std_With) ਵਾਪਸ;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ RandomizeMarkVisitor(Ast_ਗੰਢ* nodep) {
        iterateConst(nodep);
        markAllDerived();
        ਸੈੱਟ_Package_Refs();
    }
    ~RandomizeMarkVisitor() override = ਮੂਲ;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

ਵਰਗ Constraint_ਵਿਅੰਜਕ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ਅਚਲ m_classp;
    Ast_ਗੰਢ_FTask* ਅਚਲ m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_ਗੰਢ_FTask* ਅਚਲ m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ਅਚਲ m_genp;  // VlRandomizer variable of the class
    AstVar* m_rand_ਬਹੁਲਕ_Varp;  // Relevant randmode state variable
    ਤਾਰਕਿਕ m_wantSingle = ਝੂਠ;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    ਤਾਰਕਿਕ m_structSel = ਝੂਠ;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    ਤਾਰਕਿਕ m_defer_Rand_ਬਹੁਲਕ_Hoist = ਝੂਠ;  // Wait for the complete struct member path
    std::ਸੈੱਟ<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::ਸੈੱਟ<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertion_ਸੈੱਟ<AstVar*>* m_ਮਾਪ_Constrained_Arraysp = ਖਾਲੀ_ਸੂਚਕ;  // Arrays with size constraints
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_conditionp = ਖਾਲੀ_ਸੂਚਕ;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_ਗੰਢ_FTask* m_prepareConstrainedArraysp = ਖਾਲੀ_ਸੂਚਕ;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = ਖਾਲੀ_ਸੂਚਕ;  // Current ID of unique call
    V3_Unique_ਨਾਂ& m_unique_ਨਾਂ;  // Unique names of temporaries, and of blocks holding them
    std::ਸਦਿਸ਼<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_ਗੰਢ* m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_Indexp = ਖਾਲੀ_ਸੂਚਕ;
    AstConstraintForeach* m_foreachp = ਖਾਲੀ_ਸੂਚਕ;  // Innermost constraint-foreach being processed
    std::ਸੈੱਟ<ਅਚਲ AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    ਵਰਗ Nested_Access_ਰਸਤਾ final {
        AstMemberSel* m_ਸਿਖਰ_Nested_Array_Member_Selp
            = ਖਾਲੀ_ਸੂਚਕ;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = ਖਾਲੀ_ਸੂਚਕ;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = ਖਾਲੀ_ਸੂਚਕ;  // Last node of variable's name format
        Ast_ਗੰਢ** m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_ਸੂਚਕਾਂਕ_Pointerp
            = ਖਾਲੀ_ਸੂਚਕ;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    ਜਨਤਕ:
        ਖਾਲੀ ਜੋੜੋ_Var_Name_Part(AstSFormatF* ਅਚਲ nodep, std::string&& name) {
            ਜੇ (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } ਨਹੀਂਤਾਂ {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.ਪਾਓ(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            ਜੇ (name == "") {
                ਜੇ (*m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_ਸੂਚਕਾਂਕ_Pointerp) {
                    (*m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_ਸੂਚਕਾਂਕ_Pointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_ਸੂਚਕਾਂਕ_Pointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_ਸੂਚਕਾਂਕ_Pointerp = nodep;
            }
        }

        AstMemberSel* access_ਰੁੱਖ() { ਵਾਪਸ m_ਸਿਖਰ_Nested_Array_Member_Selp; }
        ਅਚਲ std::string& smtName() { ਵਾਪਸ m_smtName; }

        Ast_Stmt_ਵਿਅੰਜਕ* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ਅਚਲ methodp = ਨਵਾਂ AstCMethodHard{
                varp->fileline(),
                ਨਵਾਂ AstVarRef{varp->fileline(), VN_AS(genp->user2p(), ਗੰਢ_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtype_ਸੈੱਟ_Void();

            methodp->ਜੋੜੋ_Pinsp(m_ਸਿਖਰ_Nested_Array_Member_Selp);  // Variable

            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep();  //ਚੌੜਾਈ
            ਅਚਲ ਮਾਪ_t ਚੌੜਾਈ = m_ਸਿਖਰ_Nested_Array_Member_Selp->varp()->dtypep()->ਚੌੜਾਈ();
            methodp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{dtypep->fileline(), AstConst::Unsized64{}, ਚੌੜਾਈ});

            methodp->ਜੋੜੋ_Pinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = ਖਾਲੀ_ਸੂਚਕ;

            methodp->ਜੋੜੋ_Pinsp(
                ਨਵਾਂ AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_ਸਿਖਰ_Nested_Array_Member_Selp = ਖਾਲੀ_ਸੂਚਕ;
            ਵਾਪਸ methodp->makeStmt();
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* cloneName() { ਵਾਪਸ m_nestedNameFormatTopp->clone_ਰੁੱਖ(ਝੂਠ); }

        ਖਾਲੀ error() {
            VL_DO_DANGLING(m_ਸਿਖਰ_Nested_Array_Member_Selp->delete_ਰੁੱਖ(), m_ਸਿਖਰ_Nested_Array_Member_Selp);
        }

        Nested_Access_ਰਸਤਾ(AstMemberSel* ਸਿਖਰ_Member_Selp, Ast_ਗੰਢ** ਸੂਚਕ_ਮੰਜ਼ਿਲ_ਪਹਿਲਾ_Exprp)
            : m_ਸਿਖਰ_Nested_Array_Member_Selp{ਸਿਖਰ_Member_Selp}
            , m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_ਸੂਚਕਾਂਕ_Pointerp{ਸੂਚਕ_ਮੰਜ਼ਿਲ_ਪਹਿਲਾ_Exprp} {}
        ~Nested_Access_ਰਸਤਾ() {
            ਜੇ (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_ਰੁੱਖ(),
                            m_nestedNameFormatTopp = ਖਾਲੀ_ਸੂਚਕ);
                m_nestedNameFormatp = ਖਾਲੀ_ਸੂਚਕ;
            }
        }
    };
    Nested_Access_ਰਸਤਾ* m_nestedAccess = ਖਾਲੀ_ਸੂਚਕ;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* createEnumConstraint(File_ਸਤਰ* ਅਚਲ fl, ਅਚਲ std::string& smtName,
                                      AstEnumDType* ਅਚਲ enumDtp, AstVar* ਅਚਲ genVarp) {
        Ast_ਗੰਢ_Module* ਅਚਲ genModp = VN_AS(genVarp->user2p(), ਗੰਢ_Module);
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = enumDtp->ਚੌੜਾਈ();
        std::string constraint = "(__Vbv (or";
        ਲਈ (Ast_Enum_ਵਸਤੂ* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
            ਅਚਲ AstConst* ਅਚਲ vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvt_ਮੰਜ਼ਿਲ_Str(vconstp->ਮੰਜ਼ਿਲ_UInt()) + " "
                          + cvt_ਮੰਜ਼ਿਲ_Str(ਚੌੜਾਈ) + "))";
        }
        constraint += "))";
        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
            fl, ਨਵਾਂ AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "\"" + constraint + "\""}};
        callp->dtype_ਸੈੱਟ_Void();
        ਵਾਪਸ callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* ਲੱਭੋ_Static_Rand_ਬਹੁਲਕ_Var_Member(AstClass* classp) ਅਚਲ {
        ਜਦੋਂ (ਸੱਚ) {
            ਜੇ (AstVar* ਅਚਲ varp
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(classp, "__Vstaticrandmode"), Var)) {
                ਵਾਪਸ varp;
            }
            AstClassExtends* ਅਚਲ extendsp = classp->extendsp();
            ਜੇ (!extendsp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    ਖਾਲੀ markConstrainedRandMember(AstStructDType* ਅਚਲ structp, ਅਚਲ std::string& name) {
        AstMemberDType* ਅਚਲ memberp
            = VN_CAST(m_memberMap.ਲੱਭੋ_Member(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(ਸੱਚ);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    ਤਾਰਕਿਕ is_Rand_Qualified_ਰਸਤਾ(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜਦੋਂ (nodep) {
            ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ selp = VN_CAST(nodep, StructSel)) {
                ਜੇ (AstStructDType* ਅਚਲ structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ਅਚਲ AstMemberDType* ਅਚਲ memberp
                        = VN_CAST(m_memberMap.ਲੱਭੋ_Member(structp, selp->name()), MemberDType);
                    ਜੇ (memberp && !memberp->rand().isRandomizable()) ਵਾਪਸ ਝੂਠ;
                }
                nodep = selp->fromp();
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstCMethodHard* ਅਚਲ methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } ਨਹੀਂਤਾਂ {
                ਰੋਕੋ;
            }
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    ਸਥਿਰ ਖਾਲੀ markStructConstrainedRandRecurse(Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        AstStructDType* ਅਚਲ structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        ਜੇ (!structp || structp->packed()) ਵਾਪਸ;
        structp->markConstrainedRand(ਸੱਚ);
        ਲਈ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            ਜੇ (!memberp->rand().isRandomizable()) ਜਾਰੀ;
            memberp->markConstrainedRand(ਸੱਚ);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string build_Member_ਰਸਤਾ(ਅਚਲ AstMemberSel* ਅਚਲ memberSelp) {
        ਅਚਲ Ast_ਗੰਢ* fromp = memberSelp->fromp();
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            ਵਾਪਸ refp->name() + "." + memberSelp->name();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            ਵਾਪਸ build_Member_ਰਸਤਾ(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        ਵਾਪਸ "";
    }

    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* getFromp(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep) {
        ਜੇ (ਅਚਲ AstCMethodHard* ਅਚਲ cmethp = VN_CAST(nodep, CMethodHard)) {
            ਵਾਪਸ cmethp->fromp();
        }
        ਵਾਪਸ getSelFromp(nodep);
    }

    ਸਥਿਰ std::string ਕੱਢੋ_Solve_Before_ਅਧਾਰ_Name(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਜਦੋਂ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = getFromp(exprp)) exprp = fromp;
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ vrefp = VN_CAST(exprp, VarRef)) ਵਾਪਸ vrefp->name();
        ਵਾਪਸ "";
    }

    ਸਥਿਰ ਖਾਲੀ build_Name_ਅਗੇਤਰ(AstCExpr* exprp, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep) {
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
            exprp->ਜੋੜੋ("(\""s + refp->name());
            ਵਾਪਸ;
        }

        ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = getFromp(nodep);
        ਜੇ (fromp) build_Name_ਅਗੇਤਰ(exprp, fromp);

        ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ selp = VN_CAST(nodep, StructSel)) {
            exprp->ਜੋੜੋ("." + selp->name());
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstCMethodHard* ਅਚਲ cmethp = VN_CAST(nodep, CMethodHard)) {
            ਜੇ (cmethp->ਵਿਧੀ() == VCMethod::ARRAY_AT) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = cmethp->pinsp();
                exprp->ਜੋੜੋ(".\" + ");
                AstCFuncHard* ਅਚਲ solverHexFuncp
                    = ਨਵਾਂ AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_ਮੰਜ਼ਿਲ_SOLVER_HEX};
                solverHexFuncp->ਜੋੜੋ_Pinsp(argp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                solverHexFuncp->dtype_ਸੈੱਟ_String();
                exprp->ਜੋੜੋ(solverHexFuncp);
                exprp->ਜੋੜੋ(" + \"");
            } ਨਹੀਂਤਾਂ {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                ਵਾਪਸ;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ਸਥਿਰ AstCExpr* build_ਰਸਤਾ_ਵਿਅੰਜਕ(File_ਸਤਰ* ਅਚਲ fl, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep,
                                   ਅਚਲ std::string selName) {
        AstCExpr* ਅਚਲ exprp = ਨਵਾਂ AstCExpr{fl, ""};
        build_Name_ਅਗੇਤਰ(exprp, nodep);
        ਜੇ (selName != "") {
            exprp->ਜੋੜੋ(".");
            exprp->ਜੋੜੋ(selName);
        }
        exprp->ਜੋੜੋ("\")");
        ਵਾਪਸ exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_ਵਿਅੰਜਕ(File_ਸਤਰ* fl, ਅਚਲ std::string& ਅਧਾਰ_Name,
                                    ਅਚਲ Ast_ਗੰਢ_Sel* selp) {
        AstCExpr* ਅਚਲ p = ਨਵਾਂ AstCExpr{fl, ""};
        p->ਜੋੜੋ("(\""s + ਅਧਾਰ_Name + ".\"");
        p->ਜੋੜੋ("+");
        AstCFuncHard* ਅਚਲ solverHexFuncp
            = ਨਵਾਂ AstCFuncHard{fl, VCFunction::RANDOMIZER_ਮੰਜ਼ਿਲ_SOLVER_HEX};
        solverHexFuncp->ਜੋੜੋ_Pinsp(selp->bitp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
        solverHexFuncp->dtype_ਸੈੱਟ_String();
        p->ਜੋੜੋ(solverHexFuncp);
        p->ਜੋੜੋ(")");
        p->dtype_ਸੈੱਟ_String();
        ਵਾਪਸ p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* getSelFromp(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ mp = VN_CAST(exprp, MemberSel)) ਵਾਪਸ mp->fromp();
        ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ sp = VN_CAST(exprp, StructSel)) ਵਾਪਸ sp->fromp();
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* build_Solve_Before_Name_ਵਿਅੰਜਕ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ਅਚਲ p = ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "\"" + varrefp->name() + "\"s"};
            p->dtype_ਸੈੱਟ_String();
            ਵਾਪਸ p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ selFromp = getSelFromp(exprp)) {
            ਅਚਲ std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string ਅਧਾਰ_Name;
                ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    ਅਧਾਰ_Name = vp->name();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    ਅਧਾਰ_Name = build_Member_ਰਸਤਾ(mp);
                }
                ਜੇ (ਅਧਾਰ_Name.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                AstCExpr* ਅਚਲ p = ਨਵਾਂ AstCExpr{fl, ""};
                p->ਜੋੜੋ("(\""s + ਅਧਾਰ_Name + ".\"");
                p->ਜੋੜੋ("+");
                AstCFuncHard* ਅਚਲ solverHexFuncp
                    = ਨਵਾਂ AstCFuncHard{fl, VCFunction::RANDOMIZER_ਮੰਜ਼ਿਲ_SOLVER_HEX};
                solverHexFuncp->ਜੋੜੋ_Pinsp(arrSelp->bitp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                solverHexFuncp->dtype_ਸੈੱਟ_String();
                p->ਜੋੜੋ(solverHexFuncp);
                p->ਜੋੜੋ(")");
                p->dtype_ਸੈੱਟ_String();
                ਵਾਪਸ p;
            }
            // Static member path (obj.field)
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ vp = VN_CAST(selFromp, VarRef)) {
                ਅਚਲ std::string ਰਸਤਾ = vp->name() + "." + selName;
                AstCExpr* ਅਚਲ p = ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "\"" + ਰਸਤਾ + "\"s"};
                p->dtype_ਸੈੱਟ_String();
                ਵਾਪਸ p;
            }
            ਜੇ (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ਅਚਲ p = build_ਰਸਤਾ_ਵਿਅੰਜਕ(fl, selFromp, selName);
                p->dtype_ਸੈੱਟ_String();
                ਵਾਪਸ p;
            }
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ਅਚਲ p = build_ਰਸਤਾ_ਵਿਅੰਜਕ(fl, exprp, "");
            p->dtype_ਸੈੱਟ_String();
            ਵਾਪਸ p;
        }
        ਅਚਲ Ast_ਗੰਢ_Sel* selp = VN_CAST(exprp, ArraySel);
        ਜੇ (!selp) selp = VN_CAST(exprp, AssocSel);
        ਜੇ (selp) {
            // arr[i] -> dynamic name
            std::string ਅਧਾਰ_Name;
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ vp = VN_CAST(selp->fromp(), VarRef)) {
                ਅਧਾਰ_Name = vp->name();
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ mp = VN_CAST(selp->fromp(), MemberSel)) {
                ਅਧਾਰ_Name = build_Member_ਰਸਤਾ(mp);
            }
            ਜੇ (ਅਧਾਰ_Name.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            ਵਾਪਸ build_Array_Sel_Name_ਵਿਅੰਜਕ(fl, ਅਧਾਰ_Name, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        ਜੇ (ਅਚਲ AstSel* ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_Selp = VN_CAST(exprp, Sel)) {
            ਵਾਪਸ build_Solve_Before_Name_ਵਿਅੰਜਕ(fl, ਦੋਆਧਾਰੀ_ਅੰਕ_Selp->fromp());
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    AstSFormatF* getConstFormat(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਵਾਪਸ ਨਵਾਂ AstSFormatF{nodep->fileline(), (nodep->ਚੌੜਾਈ() & 3) ? "#b%b" : "#x%x", ਝੂਠ,
                               nodep};
    }
    ਤਾਰਕਿਕ editFormat(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (nodep->user1()) ਵਾਪਸ ਝੂਠ;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlink_Fr_ਪਿੱਠ(&handle);
        handle.relink(getConstFormat(nodep));
        ਵਾਪਸ ਸੱਚ;
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* wrapWithCond(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp,
                              ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ) {
        ਜੇ (condp) {
            File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
            ਵਾਪਸ ਨਵਾਂ AstCond{
                flp, condp, exprp,
                getConstFormat(ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, ਚੌੜਾਈ, 0})};
        }
        ਵਾਪਸ exprp;
    }
    AstSFormatF* editSMT(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = ਖਾਲੀ_ਸੂਚਕ,
                         Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = ਖਾਲੀ_ਸੂਚਕ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsp = ਖਾਲੀ_ਸੂਚਕ) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_ਵਿਅੰਜਕ = nodep->emitSMT();  // Might need child width (AstExtend)
        ਜੇ (smt_ਵਿਅੰਜਕ == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }

        ਅਚਲ ਪੂਰਨ lhs_ਚੌੜਾਈ = lhsp ? lhsp->ਚੌੜਾਈ() : 0;
        ਅਚਲ ਪੂਰਨ rhs_ਚੌੜਾਈ = rhsp ? rhsp->ਚੌੜਾਈ() : 0;
        ਅਚਲ ਪੂਰਨ ths_ਚੌੜਾਈ = thsp ? thsp->ਚੌੜਾਈ() : 0;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsCondp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsCondp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* thsCondp = ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = ਖਾਲੀ_ਸੂਚਕ;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlink_Fr_ਪਿੱਠ() : lhsp),
                         ਗੰਢ_ਵਿਅੰਜਕ);
            lhsCondp = m_conditionp;
        }
        ਜੇ (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = ਖਾਲੀ_ਸੂਚਕ;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlink_Fr_ਪਿੱਠ() : rhsp),
                         ਗੰਢ_ਵਿਅੰਜਕ);
            rhsCondp = m_conditionp;
        }
        ਜੇ (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = ਖਾਲੀ_ਸੂਚਕ;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlink_Fr_ਪਿੱਠ() : thsp),
                         ਗੰਢ_ਵਿਅੰਜਕ);
            thsCondp = m_conditionp;
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (string::iterator pos = smt_ਵਿਅੰਜਕ.begin(); pos != smt_ਵਿਅੰਜਕ.ਸਿਰਾ(); ++pos) {
            ਜੇ (pos[0] == '%') {
                ++pos;
                ਚੋਣ (pos[0]) {
                ਹਾਲਤ '%': ਰੋਕੋ;
                ਹਾਲਤ 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(argsp, wrapWithCond(lhsp, lhsCondp, lhs_ਚੌੜਾਈ));
                    lhsp = ਖਾਲੀ_ਸੂਚਕ;
                    ਰੋਕੋ;
                ਹਾਲਤ 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(argsp, wrapWithCond(rhsp, rhsCondp, rhs_ਚੌੜਾਈ));
                    rhsp = ਖਾਲੀ_ਸੂਚਕ;
                    ਰੋਕੋ;
                ਹਾਲਤ 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(argsp, wrapWithCond(thsp, thsCondp, ths_ਚੌੜਾਈ));
                    thsp = ਖਾਲੀ_ਸੂਚਕ;
                    ਰੋਕੋ;
                ਮੂਲ: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); ਰੋਕੋ;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ਅਚਲ newp = ਨਵਾਂ AstSFormatF{nodep->fileline(), smt_ਵਿਅੰਜਕ, ਝੂਠ, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ newp;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* editSingle(File_ਸਤਰ* fl, Ast_ਗੰਢ* itemsp) {
        ਜੇ (!itemsp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = ਸੱਚ;

        {
            AstBegin* ਅਚਲ tempp
                = ਨਵਾਂ AstBegin{fl, "[EditWrapper]", itemsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(), ਝੂਠ};
            VL_DO_DANGLING(iterate_And_ਅਗਲਾ_Null(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            ਜੇ (itemsp) itemsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            VL_DO_DANGLING(tempp->delete_ਰੁੱਖ(), tempp);
        }
        ਜੇ (!itemsp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprsp = VN_CAST(itemsp, ਗੰਢ_ਵਿਅੰਜਕ);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        ਜੇ (!exprsp->nextp()) ਵਾਪਸ exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        ਲਈ (Ast_ਗੰਢ* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        ਵਾਪਸ ਨਵਾਂ AstSFormatF{fl, fmt.str(), ਝੂਠ, exprsp};
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newSel(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* arrayp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_ਗੰਢ_DType* ਅਚਲ arrDtp = arrayp->unlink_Fr_ਪਿੱਠ()->dtypep()->skipRefp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* selp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (VN_IS(arrDtp, ਕਤਾਰ_DType) || VN_IS(arrDtp, DynArrayDType))
            selp = ਨਵਾਂ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        ਨਹੀਂਤਾਂ ਜੇ (VN_IS(arrDtp, UnpackArrayDType))
            selp = ਨਵਾਂ AstArraySel{fl, arrayp, idxp};
        ਨਹੀਂਤਾਂ ਜੇ (VN_IS(arrDtp, AssocArrayDType))
            selp = ਨਵਾਂ AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        ਵਾਪਸ selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* adjust_ਚੌੜਾਈ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, ਪੂਰਨ ਨਿਸ਼ਾਨਾ_ਚੌੜਾਈ,
                             VSigning ਨਿਸ਼ਾਨਾ_Signing) {
        ਅਚਲ ਪੂਰਨ ਵਿਅੰਜਕ_ਚੌੜਾਈ = exprp->ਚੌੜਾਈ();
        ਜੇ (ਨਿਸ਼ਾਨਾ_ਚੌੜਾਈ > ਵਿਅੰਜਕ_ਚੌੜਾਈ) {
            // Extend to match target width
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਨਤੀਜਾ = ਨਵਾਂ AstExtend{fl, exprp, ਨਿਸ਼ਾਨਾ_ਚੌੜਾਈ};
            ਨਤੀਜਾ->dtypeChgSigned(ਨਿਸ਼ਾਨਾ_Signing);
            ਵਾਪਸ ਨਤੀਜਾ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਨਿਸ਼ਾਨਾ_ਚੌੜਾਈ < ਵਿਅੰਜਕ_ਚੌੜਾਈ) {
            // Truncate to match target width
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਨਤੀਜਾ = ਨਵਾਂ AstSel{fl, exprp, 0, ਨਿਸ਼ਾਨਾ_ਚੌੜਾਈ};
            ਨਤੀਜਾ->dtypeChgSigned(ਨਿਸ਼ਾਨਾ_Signing);
            ਵਾਪਸ ਨਤੀਜਾ;
        } ਨਹੀਂਤਾਂ {
            // Width already matches
            ਵਾਪਸ exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਕੱਢੋ_Return_ਵਿਅੰਜਕ(AstFunc* funcp) {
        AstVar* ਅਚਲ retVarp = VN_CAST(funcp->fvarp(), Var);
        ਜੇ (!retVarp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* retExprp = ਖਾਲੀ_ਸੂਚਕ;
        ਪੂਰਨ ਸੌਂਪੋ_ਗਿਣਤੀ = 0;
        funcp->foreach([&](Ast_ਸੌਂਪੋ* assignp) {
            AstVarRef* ਅਚਲ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ਜੇ (!lhsp || lhsp->varp() != retVarp) ਵਾਪਸ;
            retExprp = assignp->rhsp();
            ++ਸੌਂਪੋ_ਗਿਣਤੀ;
        });
        // Simple function: initializer + return (2) or just return (1)
        ਜੇ (ਸੌਂਪੋ_ਗਿਣਤੀ > 2 || !retExprp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    ਖਾਲੀ propagateUser1InlineRecurse(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (VN_IS(nodep, ਗੰਢ_Var_Ref)) {
            nodep->user1(VN_AS(nodep, ਗੰਢ_Var_Ref)->varp()->rand().isRandomizable());
            ਵਾਪਸ;
        }
        ਜੇ (VN_IS(nodep, Const)) {
            nodep->user1(ਝੂਠ);
            ਵਾਪਸ;
        }
        ਪੂਰਨ any_ਬੱਚਾ = ਝੂਠ;  // Used as bool
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp = VN_CAST(nodep->op1p(), ਗੰਢ_ਵਿਅੰਜਕ)) {
            propagateUser1InlineRecurse(cp);
            any_ਬੱਚਾ |= cp->user1();
        }
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp = VN_CAST(nodep->op2p(), ਗੰਢ_ਵਿਅੰਜਕ)) {
            propagateUser1InlineRecurse(cp);
            any_ਬੱਚਾ |= cp->user1();
        }
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp = VN_CAST(nodep->op3p(), ਗੰਢ_ਵਿਅੰਜਕ)) {
            propagateUser1InlineRecurse(cp);
            any_ਬੱਚਾ |= cp->user1();
        }
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cp = VN_CAST(nodep->op4p(), ਗੰਢ_ਵਿਅੰਜਕ)) {
            propagateUser1InlineRecurse(cp);
            any_ਬੱਚਾ |= cp->user1();
        }
        nodep->user1(any_ਬੱਚਾ);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ਸਥਿਰ Ast_ਗੰਢ* prependDistPreamble(AstConstraintForeach* nodep, Ast_ਗੰਢ* bodyp) {
        Ast_ਗੰਢ* ਅਚਲ preamblep = nodep->user3p();
        ਜੇ (!preamblep) ਵਾਪਸ bodyp;
        nodep->user3p(ਖਾਲੀ_ਸੂਚਕ);
        preamblep->ਜੋੜੋ_ਅਗਲਾ(bodyp);
        ਵਾਪਸ preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    ਤਾਰਕਿਕ referencesForeachIdx(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ {
        ਜੇ (m_foreachIdxVars.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ nodep->exists(
            [&](ਅਚਲ AstVarRef* refp) { ਵਾਪਸ m_foreachIdxVars.ਗਿਣਤੀ_2(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    ਖਾਲੀ ਜੋੜੋ_Registration_Stmt(Ast_ਗੰਢ* ਅਚਲ stmtp, Ast_ਗੰਢ_FTask* ਅਚਲ initTaskp) {
        ਜੇ (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->ਜੋੜੋ_Bodyp(stmtp);
        } ਨਹੀਂਤਾਂ {
            initTaskp->ਜੋੜੋ_Stmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_ਸਤਰ* ਅਚਲ fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ arrExprp,
                                       Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ idxExprp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(arrExprp, idxExprp);
        AstSFormatF* ਅਚਲ formatp = ਨਵਾਂ AstSFormatF{fl, "(select %s %s)", ਝੂਠ, argsp};
        ਵਾਪਸ formatp;
    }

    ਖਾਲੀ ਸੈੱਟ_Rand_ਬਹੁਲਕ(AstVar* ਅਚਲ varp, AstMemberSel* ਅਚਲ memberselp,
                     ਅਚਲ std::string& smtName, ਅਚਲ Randomize_ਬਹੁਲਕ& rand_ਬਹੁਲਕ,
                     Ast_ਗੰਢ_FTask* ਅਚਲ initTaskp) {
        Ast_ਗੰਢ_Module* ਅਚਲ varClassp = VN_AS(varp->user2p(), ਗੰਢ_Module);
        AstVar* ਅਚਲ sub_Rand_ਬਹੁਲਕ_Varp = get_Rand_ਬਹੁਲਕ_Var_From_Class(varClassp);
        ਜੇ (sub_Rand_ਬਹੁਲਕ_Varp) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮਾਪਾ_Accessp = memberselp->fromp()->clone_ਰੁੱਖ(ਝੂਠ);
            AstMemberSel* ਅਚਲ rand_ਬਹੁਲਕ_Selp
                = ਨਵਾਂ AstMemberSel{varp->fileline(), ਮਾਪਾ_Accessp, sub_Rand_ਬਹੁਲਕ_Varp};
            rand_ਬਹੁਲਕ_Selp->dtypep(sub_Rand_ਬਹੁਲਕ_Varp->dtypep());
            AstCMethodHard* ਅਚਲ atp
                = ਨਵਾਂ AstCMethodHard{varp->fileline(), rand_ਬਹੁਲਕ_Selp, VCMethod::ARRAY_AT,
                                     ਨਵਾਂ AstConst{varp->fileline(), rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ}};
            atp->dtype_ਸੈੱਟ_UInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ਅਚਲ enablep = ਨਵਾਂ AstCMethodHard{
                varp->fileline(),
                ਨਵਾਂ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ਗੰਢ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtype_ਸੈੱਟ_Void();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : ਨਵਾਂ AstSFormatF{varp->fileline(), smtName, ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ};
            nameExprp->dtypep(varp->dtypep());
            enablep->ਜੋੜੋ_Pinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ਅਚਲ disablep = ਨਵਾਂ AstCMethodHard{
                varp->fileline(),
                ਨਵਾਂ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ਗੰਢ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_ਸੈੱਟ_VAR_DISABLED};
            disablep->dtype_ਸੈੱਟ_Void();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ disnp = nameExprp->clone_ਰੁੱਖ(ਝੂਠ);
            disablep->ਜੋੜੋ_Pinsp(disnp);
            AstIf* ਅਚਲ ifp
                = ਨਵਾਂ AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            ਜੋੜੋ_Registration_Stmt(ifp, initTaskp);
        }
    }

    ਖਾਲੀ markRandc(AstVar* ਅਚਲ varp, ਅਚਲ std::string& smtName, Ast_ਗੰਢ_FTask* ਅਚਲ initTaskp) {
        AstCMethodHard* ਅਚਲ markp = ਨਵਾਂ AstCMethodHard{
            varp->fileline(),
            ਨਵਾਂ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ਗੰਢ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtype_ਸੈੱਟ_Void();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : ਨਵਾਂ AstSFormatF{varp->fileline(), smtName, ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ};
        nameExprp->dtypep(varp->dtypep());
        markp->ਜੋੜੋ_Pinsp(nameExprp);
        ਜੋੜੋ_Registration_Stmt(markp->makeStmt(), initTaskp);
    }

    Ast_ਗੰਢ_Module* getLeftmostVarModulep(AstMemberSel* ਅਚਲ memberselp,
                                         AstVar* ਅਚਲ varp) ਅਚਲ {
        ਜੇ (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_ਗੰਢ* ਖੱਬਾ_Most_Nodep = memberselp->fromp();
            ਜਦੋਂ (AstMemberSel* ਅਚਲ nestedMemberSelp = VN_CAST(ਖੱਬਾ_Most_Nodep, MemberSel)) {
                ਖੱਬਾ_Most_Nodep = nestedMemberSelp->fromp();
            }
            ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ ਜੜ੍ਹ_Var_Refp = VN_CAST(ਖੱਬਾ_Most_Nodep, ਗੰਢ_Var_Ref)) {
                ਵਾਪਸ VN_AS(ਜੜ੍ਹ_Var_Refp->varp()->user2p(), ਗੰਢ_Module);
            }
            ਵਾਪਸ VN_AS(memberselp->user2p(), ਗੰਢ_Module);
        }
        ਵਾਪਸ VN_AS(varp->user2p(), ਗੰਢ_Module);
    }

    Ast_ਗੰਢ_FTask* getInitTaskp(AstVar* ਅਚਲ varp, ਅਚਲ ਤਾਰਕਿਕ memberselOrStructSel,
                               Ast_ਗੰਢ_Module* ਅਚਲ classp) ਅਚਲ {
        ਜੇ (m_inlineInitTaskp) ਵਾਪਸ m_inlineInitTaskp;
        varp->user3(ਸੱਚ);
        ਜੇ (memberselOrStructSel) {
            ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ initTaskp
                = VN_AS(m_memberMap.ਲੱਭੋ_Member(classp, "randomize"), ਗੰਢ_FTask)) {
                ਵਾਪਸ initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            ਜੇ (m_memberselInitTaskp) ਵਾਪਸ m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_FTask* ਅਚਲ initTaskp
                = VN_AS(m_memberMap.ਲੱਭੋ_Member(classp, "new"), ਗੰਢ_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            ਵਾਪਸ initTaskp;
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    ਖਾਲੀ createWriteVarForClassFields(AstClass* ਅਚਲ elemClassp, ਅਚਲ std::string& smtName,
                                      AstVar* ਅਚਲ iterVarp, ਅਚਲ ਤਾਰਕਿਕ isUnpackedClassRefArray,
                                      Ast_ਗੰਢ_Module* ਅਚਲ classOrPackagep, AstVar* ਅਚਲ varp,
                                      AstIf* ਅਚਲ ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ਅਚਲ {
        File_ਸਤਰ* ਅਚਲ fl = varp->fileline();
        Ast_ਗੰਢ_Module* ਅਚਲ varClassp = VN_AS(varp->user2p(), ਗੰਢ_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr ਪੂਰਨ idx_ਚੌੜਾਈ = 32;
        ਅਚਲ ਪੂਰਨ fmt_ਚੌੜਾਈ = VL_ਸ਼ਬਦ_I(idx_ਚੌੜਾਈ) * 8;

        ਲਈ (ਅਚਲ AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ) {
            ਲਈ (Ast_ਗੰਢ* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ਅਚਲ memberVarp = VN_CAST(mnodep, Var);
                ਜੇ (!memberVarp || !memberVarp->rand().isRandomizable()) ਜਾਰੀ;
                Ast_ਗੰਢ_DType* ਅਚਲ memberDtp = memberVarp->dtypep()->skipRefp();
                ਜੇ (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) ਜਾਰੀ;
                ਅਚਲ ਪੂਰਨ member_ਚੌੜਾਈ = memberDtp->ਚੌੜਾਈ();

                AstCStmt* ਅਚਲ fmtp = ਨਵਾਂ AstCStmt{fl};
                ਜੇ (isUnpackedClassRefArray) {
                    fmtp->ਜੋੜੋ("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } ਨਹੀਂਤਾਂ {
                    fmtp->ਜੋੜੋ("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::ਮੰਜ਼ਿਲ_string(fmt_ਚੌੜਾਈ) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->ਜੋੜੋ(ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->ਜੋੜੋ(");\n");
                ifNonNullp->ਜੋੜੋ_Thensp(fmtp);

                AstVarRef* ਅਚਲ arrayWrRefp = ਨਵਾਂ AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* atWritep;
                ਜੇ (isUnpackedClassRefArray) {
                    atWritep = ਨਵਾਂ AstArraySel{fl, arrayWrRefp,
                                               ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}};
                } ਨਹੀਂਤਾਂ {
                    atWritep = ਨਵਾਂ AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* ਅਚਲ memberSelp = ਨਵਾਂ AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* ਅਚਲ writeVarp
                    = ਨਵਾਂ AstCMethodHard{fl,
                                         ਨਵਾਂ AstVarRef{fl, VN_AS(m_genp->user2p(), ਗੰਢ_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtype_ਸੈੱਟ_Void();
                writeVarp->ਜੋੜੋ_Pinsp(memberSelp);
                writeVarp->ਜੋੜੋ_Pinsp(
                    ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(member_ਚੌੜਾਈ)});
                AstCExpr* ਅਚਲ nameRefp = ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->ਜੋੜੋ_Pinsp(nameRefp);
                writeVarp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->ਜੋੜੋ_Thensp(writeVarp->makeStmt());
            }
        }
    }

    ਖਾਲੀ createSolverArrayHandle(AstVar* ਅਚਲ varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_ਗੰਢ_Module* ਅਚਲ classOrPackagep,
                                 ਅਚਲ ਤਾਰਕਿਕ isUnpackedClassRefArray,
                                 ਅਚਲ std::string& smtName) ਅਚਲ {
        File_ਸਤਰ* ਅਚਲ fl = varp->fileline();
        AstClass* ਅਚਲ elemClassp = elemClassRefDtp->classp();
        Ast_ਗੰਢ_Module* ਅਚਲ varClassp = VN_AS(varp->user2p(), ਗੰਢ_Module);

        AstVar* ਅਚਲ iterVarp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, m_unique_ਨਾਂ.get("__Vi"),
                                            varp->ਲੱਭੋ_UInt32_DType()};
        iterVarp->funcLocal(ਸੱਚ);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ਗੰਢ* ਅਚਲ stmtsp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::WRITE}, ਨਵਾਂ AstConst{fl, 0}};

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* sizep;
        ਜੇ (isUnpackedClassRefArray) {
            ਅਚਲ ਪੂਰਨ array_ਮਾਪ_2
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(array_ਮਾਪ_2)};
        } ਨਹੀਂਤਾਂ {
            AstVarRef* ਅਚਲ array_ਮਾਪ_Refp = ਨਵਾਂ AstVarRef{fl, varClassp, varp, VAccess::READ};
            array_ਮਾਪ_Refp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ਅਚਲ dynSizep
                = ਨਵਾਂ AstCMethodHard{fl, array_ਮਾਪ_Refp, VCMethod::DYN_ਮਾਪ, ਖਾਲੀ_ਸੂਚਕ};
            dynSizep->dtype_ਸੈੱਟ_UInt32();
            sizep = dynSizep;
        }

        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{fl};
        stmtsp->ਜੋੜੋ_ਅਗਲਾ(loopp);
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{
            fl, loopp, ਨਵਾਂ AstLt{fl, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ਅਚਲ arrayReadRefp = ਨਵਾਂ AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* atReadp;
        ਜੇ (isUnpackedClassRefArray) {
            atReadp
                = ਨਵਾਂ AstArraySel{fl, arrayReadRefp, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}};
        } ਨਹੀਂਤਾਂ {
            atReadp = ਨਵਾਂ AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ਅਚਲ ifNonNullp
            = ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstNeq{fl, atReadp, ਨਵਾਂ AstConst{fl, AstConst::Null{}}}, ਖਾਲੀ_ਸੂਚਕ};
        loopp->ਜੋੜੋ_Stmtsp(ifNonNullp);

        AstCStmt* ਅਚਲ bufDeclp = ਨਵਾਂ AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->ਜੋੜੋ_Thensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            fl, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::WRITE},
            ਨਵਾਂ Ast_ਜੋੜੋ{fl, ਨਵਾਂ AstConst{fl, 1}, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(ਸੱਚ);
        Ast_ਗੰਢ_FTask* initTaskp = m_inlineInitTaskp;
        ਜੇ (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.ਲੱਭੋ_Member(varClassp, "new"), ਗੰਢ_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->ਜੋੜੋ_Stmtsp(stmtsp);
    }

    ਖਾਲੀ createSolverVarHandle(AstVar* ਅਚਲ varp, ਅਚਲ ਤਾਰਕਿਕ structSelOrCMeth,
                               ਅਚਲ ਤਾਰਕਿਕ isGlobalConstrained, ਅਚਲ Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ,
                               AstMemberSel* ਅਚਲ memberselp, ਅਚਲ std::string& smtName,
                               Ast_ਗੰਢ_Module* ਅਚਲ classOrPackagep, Ast_ਗੰਢ_Module* ਅਚਲ classp,
                               Ast_ਗੰਢ_FTask* ਅਚਲ initTaskp) {
        uint32_t unpackedDims = 0;
        ਜੇ (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(ਝੂਠ).ਦੂਜਾ;
        }
        ਜੇ (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ਅਚਲ methodp = ਨਵਾਂ AstCMethodHard{
            varp->fileline(),
            ਨਵਾਂ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ਗੰਢ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_ਸੈੱਟ_Void();
        ਜੇ (memberselp) {
            methodp->ਜੋੜੋ_Pinsp(memberselp);
        } ਨਹੀਂਤਾਂ {
            AstVarRef* ਅਚਲ varRefp
                = ਨਵਾਂ AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->ਜੋੜੋ_Pinsp(varRefp);
        }
        ਅਚਲ ਮਾਪ_t ਚੌੜਾਈ = arrayElementDTypep(varp->dtypep())->ਚੌੜਾਈ();
        methodp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{varp->fileline(), AstConst::Unsized64{}, ਚੌੜਾਈ});
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ varnamep = ਨਵਾਂ AstCExpr{varp->fileline(), AstCExpr::ਸ਼ੁੱਧ{},
                                                   "\"" + smtName + "\"", varp->ਚੌੜਾਈ()};
        varnamep->dtypep(varp->dtypep());
        methodp->ਜੋੜੋ_Pinsp(varnamep);
        methodp->ਜੋੜੋ_Pinsp(
            ਨਵਾਂ AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        ਜੇ (rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ && !(isGlobalConstrained && memberselp)) {
            methodp->ਜੋੜੋ_Pinsp(
                ਨਵਾਂ AstConst{varp->fileline(), AstConst::Unsized64{}, rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->ਜੋੜੋ_Stmtsp(methodp->makeStmt());
        ਜੇ (varp->lifetime().isStatic() && rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) {
            AstCMethodHard* ਅਚਲ markp = ਨਵਾਂ AstCMethodHard{
                varp->fileline(),
                ਨਵਾਂ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ਗੰਢ_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ namep = ਨਵਾਂ AstCExpr{varp->fileline(), AstCExpr::ਸ਼ੁੱਧ{},
                                                    "\"" + smtName + "\"", varp->ਚੌੜਾਈ()};
            namep->dtypep(varp->dtypep());
            markp->ਜੋੜੋ_Pinsp(namep);
            markp->dtype_ਸੈੱਟ_Void();
            initTaskp->ਜੋੜੋ_Stmtsp(markp->makeStmt());
        }
        ਜੇ (isGlobalConstrained && memberselp && rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) {
            ਸੈੱਟ_Rand_ਬਹੁਲਕ(varp, memberselp, smtName, rand_ਬਹੁਲਕ, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        ਜੇ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* createVarExprp(Ast_ਗੰਢ_Var_Ref* nodep, ਅਚਲ Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ,
                                ਅਚਲ std::string& smtName, ਅਚਲ ਤਾਰਕਿਕ isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ਅਚਲ varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = ਨਵਾਂ AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ};
        ਜੇ (!rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) {
            // Global constraints keep nodep alive for write_var processing
            ਜੇ (!isGlobalConstrained) VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_ਰੁੱਖ(ਝੂਠ)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rand_ਬਹੁਲਕ_Access;
        ਅਚਲ ਤਾਰਕਿਕ varIsStatic = varp->lifetime().isStatic();
        AstClass* ਅਚਲ varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : ਖਾਲੀ_ਸੂਚਕ;
        AstVar* ਅਚਲ owner_Static_Rand_ਬਹੁਲਕ_Varp
            = varOwningClassp ? ਲੱਭੋ_Static_Rand_ਬਹੁਲਕ_Var_Member(varOwningClassp) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (owner_Static_Rand_ਬਹੁਲਕ_Varp) {
            rand_ਬਹੁਲਕ_Access = ਨਵਾਂ AstVarRef{varp->fileline(),
                                           VN_AS(owner_Static_Rand_ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module),
                                           owner_Static_Rand_ਬਹੁਲਕ_Varp, VAccess::READ};
        } ਨਹੀਂਤਾਂ ਜੇ (memberselp) {
            Ast_ਗੰਢ_Module* ਅਚਲ varClassp = VN_AS(varp->user2p(), ਗੰਢ_Module);
            AstVar* ਅਚਲ effective_Rand_ਬਹੁਲਕ_Varp = get_Rand_ਬਹੁਲਕ_Var_From_Class(varClassp);
            ਜੇ (effective_Rand_ਬਹੁਲਕ_Varp) {
                // Member's class has randmode, use it
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮਾਪਾ_Access = memberselp->fromp()->clone_ਰੁੱਖ(ਝੂਠ);
                AstMemberSel* rand_ਬਹੁਲਕ_Sel
                    = ਨਵਾਂ AstMemberSel{varp->fileline(), ਮਾਪਾ_Access, effective_Rand_ਬਹੁਲਕ_Varp};
                rand_ਬਹੁਲਕ_Sel->dtypep(effective_Rand_ਬਹੁਲਕ_Varp->dtypep());
                rand_ਬਹੁਲਕ_Access = rand_ਬਹੁਲਕ_Sel;
            } ਨਹੀਂਤਾਂ {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_rand_ਬਹੁਲਕ_Varp, nodep, "No m_randModeVarp");
                rand_ਬਹੁਲਕ_Access
                    = ਨਵਾਂ AstVarRef{varp->fileline(), VN_AS(m_rand_ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module),
                                    m_rand_ਬਹੁਲਕ_Varp, VAccess::READ};
            }
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(m_rand_ਬਹੁਲਕ_Varp, nodep, "No m_randModeVarp");
            rand_ਬਹੁਲਕ_Access
                = ਨਵਾਂ AstVarRef{varp->fileline(), VN_AS(m_rand_ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module),
                                m_rand_ਬਹੁਲਕ_Varp, VAccess::READ};
        }

        AstCMethodHard* ਅਚਲ atp
            = ਨਵਾਂ AstCMethodHard{nodep->fileline(), rand_ਬਹੁਲਕ_Access, VCMethod::ARRAY_AT,
                                 ਨਵਾਂ AstConst{nodep->fileline(), rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ}};
        atp->dtype_ਸੈੱਟ_UInt32();
        exprp = ਨਵਾਂ AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(ਸੱਚ);  // Mark as formatted
        ਵਾਪਸ exprp;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        ਜੇ (varp->user4p()) {
            ਤਾਰਕਿਕ is_ਮਾਪ_Ref = ਝੂਠ;
            ਜੇ (AstCMethodHard* ਅਚਲ methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                ਜੇ (methodp->ਵਿਧੀ() == VCMethod::ASSOC_ਮਾਪ
                    || methodp->ਵਿਧੀ() == VCMethod::DYN_ਮਾਪ) {
                    is_ਮਾਪ_Ref = ਸੱਚ;
                }
            }
            ਜੇ (!is_ਮਾਪ_Ref && m_ਮਾਪ_Constrained_Arraysp) { m_ਮਾਪ_Constrained_Arraysp->ਪਾਓ(varp); }
        }

        // Check if this variable is marked as globally constrained
        ਅਚਲ ਤਾਰਕਿਕ isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = ਖਾਲੀ_ਸੂਚਕ;
        ਤਾਰਕਿਕ structSelOrCMeth = ਝੂਠ;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_ਗੰਢ_DType* selectedDtypep = ਖਾਲੀ_ਸੂਚਕ;
        std::string smtName;
        ਜੇ (m_nestedAccess) {
            m_nestedAccess->ਜੋੜੋ_Var_Name_Part(
                ਨਵਾਂ AstSFormatF{varp->fileline(), nodep->name(), ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_ਰੁੱਖ();
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_ਗੰਢ* ਸਿਖਰ_Member_Sel = nodep->backp();
            ਜਦੋਂ (VN_IS(ਸਿਖਰ_Member_Sel->backp(), MemberSel)) {
                ਸਿਖਰ_Member_Sel = ਸਿਖਰ_Member_Sel->backp();
            }
            memberselp = VN_AS(ਸਿਖਰ_Member_Sel, MemberSel)->clone_ਰੁੱਖ(ਝੂਠ);
            smtName = build_Member_ਰਸਤਾ(memberselp);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_ਗੰਢ* ਸਿਖਰ_Nodep = nodep->backp();
            ਜਦੋਂ (VN_IS(ਸਿਖਰ_Nodep->backp(), StructSel) || VN_IS(ਸਿਖਰ_Nodep->backp(), CMethodHard)) {
                ਸਿਖਰ_Nodep = ਸਿਖਰ_Nodep->backp();
            }

            ਜੇ (VN_IS(ਸਿਖਰ_Nodep, StructSel) || VN_IS(ਸਿਖਰ_Nodep, CMethodHard)) {
                structSelOrCMeth = ਸੱਚ;
                selectedDtypep = VN_AS(ਸਿਖਰ_Nodep, ਗੰਢ_ਵਿਅੰਜਕ)->dtypep();
            }

            memberselp = VN_CAST(ਸਿਖਰ_Nodep, MemberSel);
            ਜੇ (memberselp) smtName = build_Member_ਰਸਤਾ(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            ਜੇ (smtName == "") smtName = nodep->name();
        } ਨਹੀਂਤਾਂ {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        ਜੇ (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        ਜੇ (dtype_ਸ਼ਾਮਲ_ਹੈ_Real(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            ਵਾਪਸ;
        }
        Ast_ਗੰਢ_Module* ਅਚਲ classOrPackagep = nodep->classOrPackagep();
        ਅਚਲ Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {.asUQuad = varp->user1()};
        ਜੇ (!rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ && editFormat(nodep)) ਵਾਪਸ;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType);
        ਜੇ (enumDtp
            && (std::ਲੱਭੋ(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.ਸਿਰਾ(), varp)
                == m_rangeConstrainedEnums.ਸਿਰਾ())) {
            AstVar* genVarp = m_genp;
            ਜੇ (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_ਗੰਢ_FTask* ਨਿਸ਼ਾਨਾ_Taskp = m_inlineInitTaskp;
            ਜੇ (!ਨਿਸ਼ਾਨਾ_Taskp) ਨਿਸ਼ਾਨਾ_Taskp = get_Create_Constraint_ਤਿਆਰੀ_Func(m_classp, m_memberMap);
            UASSERT_OBJ(ਨਿਸ਼ਾਨਾ_Taskp, nodep, "No function to inline enum range constraint into");
            ਨਿਸ਼ਾਨਾ_Taskp->ਜੋੜੋ_Stmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.ਧੱਕੋ_ਪਿੱਠ(varp);
        }

        VNRelinker relinker;
        nodep->unlink_Fr_ਪਿੱਠ(&relinker);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp
            = createVarExprp(nodep, rand_ਬਹੁਲਕ, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ਅਚਲ ਤਾਰਕਿਕ alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.ਗਿਣਤੀ_2(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.ਗਿਣਤੀ_2(smtName) > 0
                                    : memberselp          ? m_writtenVars.ਗਿਣਤੀ_2(smtName) > 0
                                                          : varp->user3();
        ਅਚਲ ਤਾਰਕਿਕ shouldWriteVar = !alreadyWritten;
        ਜੇ (shouldWriteVar) {
            // Track this variable path as written
            ਜੇ (m_inlineInitTaskp)
                m_inlineWrittenVars.ਪਾਓ(smtName);
            ਨਹੀਂਤਾਂ ਜੇ (isGlobalConstrained || memberselp)
                m_writtenVars.ਪਾਓ(smtName);
            // For global constraints, delete nodep after processing
            ਜੇ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);

            ਤਾਰਕਿਕ isClassRefArray = ਝੂਠ;
            ਤਾਰਕਿਕ isUnpackedClassRefArray = ਝੂਠ;
            AstClassRefDType* elemClassRefDtp = ਖਾਲੀ_ਸੂਚਕ;
            {
                Ast_ਗੰਢ_DType* varDtp = varp->dtypep()->skipRefp();
                ਜੇ (varDtp->isNonPackedArray()) {
                    Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    ਜੇ (elemClassRefDtp) {
                        isClassRefArray = ਸੱਚ;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_ਗੰਢ_Module* ਅਚਲ classp = getLeftmostVarModulep(memberselp, varp);
            Ast_ਗੰਢ_FTask* ਅਚਲ initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            ਜੇ (m_nestedAccess) {
                Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                ਜੋੜੋ_Registration_Stmt(writeVarStmtp, initTaskp);
                ਜੇ (isGlobalConstrained && memberselp && rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) {
                    ਸੈੱਟ_Rand_ਬਹੁਲਕ(varp, memberselp, smtName, rand_ਬਹੁਲਕ, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                ਜੇ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } ਨਹੀਂਤਾਂ ਜੇ (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } ਨਹੀਂਤਾਂ {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, rand_ਬਹੁਲਕ,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } ਨਹੀਂਤਾਂ {
            // Variable already written, clean up cloned memberselp if any
            ਜੇ (memberselp) VL_DO_DANGLING(memberselp->delete_ਰੁੱਖ(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            ਜੇ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
        ਜੇ (m_nestedAccess) { VL_DO_DANGLING(ਮਿਟਾਓ m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* build_ਗਿਣਤੀ_Ones_Expansion(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* argp) {
        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_One{fl, argp->ਚੌੜਾਈ(), 1};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* sump = ਨਵਾਂ AstAnd{fl, argp, ਨਵਾਂ AstConst{fl, ਅੰਕ_One}};
        sump->user1(ਸੱਚ);
        ਲਈ (ਪੂਰਨ i = 1; i < argp->ਚੌੜਾਈ(); i++) {
            V3_ਸੰਖਿਆ ਅੰਕ_ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ{fl, argp->ਚੌੜਾਈ(), 0};
            ਅੰਕ_ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(i, 1);
            AstAnd* ਅਚਲ andp
                = ਨਵਾਂ AstAnd{fl, argp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਨਵਾਂ AstConst{fl, ਅੰਕ_ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ}};
            andp->user1(ਸੱਚ);
            Ast_ਖਿਸਕਾਓ_R* ਅਚਲ shiftp
                = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{fl, andp,
                                ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, argp->ਚੌੜਾਈ(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(ਸੱਚ);
            sump = ਨਵਾਂ Ast_ਜੋੜੋ{fl, sump, shiftp};
            sump->user1(ਸੱਚ);
        }
        ਵਾਪਸ sump;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗਿਣਤੀ_Ones* nodep) override {
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* sump = build_ਗਿਣਤੀ_Ones_Expansion(fl, argp);
        sump = adjust_ਚੌੜਾਈ(fl, sump, nodep->ਚੌੜਾਈ(), nodep->dtypep()->numeric());
        sump->user1(ਸੱਚ);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(sump);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRedXor* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* redxorp = ਨਵਾਂ AstSel{fl, argp, 0, 1};
        redxorp->user1(ਸੱਚ);
        ਲਈ (ਪੂਰਨ i = 1; i < argp->ਚੌੜਾਈ(); i++) {
            AstSel* ਅਚਲ selp = ਨਵਾਂ AstSel{fl, argp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), i, 1};
            selp->user1(ਸੱਚ);

            redxorp = ਨਵਾਂ AstXor{fl, redxorp, selp};
            redxorp->user1(ਸੱਚ);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(redxorp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRedAnd* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        // Convert to (~x == 0)
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_Zero{fl, argp->ਚੌੜਾਈ(), 0};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ negp = ਨਵਾਂ AstNot{fl, argp};
        negp->user1(ਸੱਚ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eqp = ਨਵਾਂ AstEq{fl, negp, ਨਵਾਂ AstConst{fl, ਅੰਕ_Zero}};
        eqp->user1(ਸੱਚ);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(eqp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRedOr* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        // Convert to (x != 0)
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_Zero{fl, argp->ਚੌੜਾਈ(), 0};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ neqp = ਨਵਾਂ AstNeq{fl, argp, ਨਵਾਂ AstConst{fl, ਅੰਕ_Zero}};
        neqp->user1(ਸੱਚ);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(neqp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstOneHot* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ ਪੂਰਨ w = argp->ਚੌੜਾਈ();

        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_Zero{fl, w, 0};
        AstNeq* ਅਚਲ neZerop
            = ਨਵਾਂ AstNeq{fl, argp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਨਵਾਂ AstConst{fl, ਅੰਕ_Zero}};
        neZerop->user1(ਸੱਚ);

        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_One{fl, w, 1};
        AstSub* ਅਚਲ subp = ਨਵਾਂ AstSub{fl, argp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਨਵਾਂ AstConst{fl, ਅੰਕ_One}};
        subp->user1(ਸੱਚ);

        AstAnd* ਅਚਲ andp = ਨਵਾਂ AstAnd{fl, argp, subp};
        andp->user1(ਸੱਚ);

        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_Zero2{fl, w, 0};
        AstEq* ਅਚਲ eqZerop = ਨਵਾਂ AstEq{fl, andp, ਨਵਾਂ AstConst{fl, ਅੰਕ_Zero2}};
        eqZerop->user1(ਸੱਚ);

        AstLogAnd* ਅਚਲ resultp = ਨਵਾਂ AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(ਸੱਚ);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(resultp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstOneHot0* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        // $onehot0(x) = (x & (x-1)) == 0
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ ਪੂਰਨ w = argp->ਚੌੜਾਈ();

        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_One{fl, w, 1};
        AstSub* ਅਚਲ subp = ਨਵਾਂ AstSub{fl, argp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਨਵਾਂ AstConst{fl, ਅੰਕ_One}};
        subp->dtypeFrom(argp);
        subp->user1(ਸੱਚ);

        AstAnd* ਅਚਲ andp = ਨਵਾਂ AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(ਸੱਚ);

        ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_Zero{fl, w, 0};
        AstEq* ਅਚਲ eqp = ਨਵਾਂ AstEq{fl, andp, ਨਵਾਂ AstConst{fl, ਅੰਕ_Zero}};
        eqp->user1(ਸੱਚ);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(eqp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗਿਣਤੀ_ਬਿੱਟ* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();

        ਤਾਰਕਿਕ ਗਿਣਤੀ_Ones = ਝੂਠ;
        ਤਾਰਕਿਕ ਗਿਣਤੀ_Zeros = ਝੂਠ;
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ਅਚਲ AstConst* ਅਚਲ cp = VN_CAST(ctrlp, Const);
            ਜੇ (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ਅਚਲ zerop
                    = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, nodep->ਚੌੜਾਈ(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                ਵਾਪਸ;
            }
            ਜੇ (cp->ਅੰਕ().ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(0) && !ਗਿਣਤੀ_Ones)
                ਗਿਣਤੀ_Ones = ਸੱਚ;
            ਨਹੀਂਤਾਂ ਜੇ (cp->ਅੰਕ().ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(0) && !ਗਿਣਤੀ_Zeros)
                ਗਿਣਤੀ_Zeros = ਸੱਚ;
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ ਪੂਰਨ arg_ਚੌੜਾਈ = argp->ਚੌੜਾਈ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* sump = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਗਿਣਤੀ_Ones && ਗਿਣਤੀ_Zeros) {
            // ones + zeros = width for 2-state types
            sump = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, nodep->ਚੌੜਾਈ(), (uint32_t)arg_ਚੌੜਾਈ};
            VL_DO_DANGLING(argp->delete_ਰੁੱਖ(), argp);
        } ਨਹੀਂਤਾਂ ਜੇ (ਗਿਣਤੀ_Ones) {
            sump = build_ਗਿਣਤੀ_Ones_Expansion(fl, argp);
        } ਨਹੀਂਤਾਂ ਜੇ (ਗਿਣਤੀ_Zeros) {
            // width - countones(x)
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ onesCountp = build_ਗਿਣਤੀ_Ones_Expansion(fl, argp);
            ਅਚਲ V3_ਸੰਖਿਆ ਚੌੜਾਈ_Val{nodep, onesCountp->ਚੌੜਾਈ(), (uint32_t)arg_ਚੌੜਾਈ};
            sump = ਨਵਾਂ AstSub{fl, ਨਵਾਂ AstConst{fl, ਚੌੜਾਈ_Val}, onesCountp};
            sump->user1(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            sump = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, nodep->ਚੌੜਾਈ(), 0U};
            VL_DO_DANGLING(argp->delete_ਰੁੱਖ(), argp);
        }

        sump = adjust_ਚੌੜਾਈ(fl, sump, nodep->ਚੌੜਾਈ(), nodep->dtypep()->numeric());
        sump->user1(ਸੱਚ);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(sump);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCLog2* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        ਅਚਲ ਪੂਰਨ arg_ਚੌੜਾਈ = argp->ਚੌੜਾਈ();
        ਅਚਲ ਪੂਰਨ ਨਤੀਜਾ_ਚੌੜਾਈ = nodep->ਚੌੜਾਈ();

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp
            = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, ਨਤੀਜਾ_ਚੌੜਾਈ, (uint32_t)arg_ਚੌੜਾਈ};

        ਲਈ (ਪੂਰਨ k = arg_ਚੌੜਾਈ - 1; k >= 0; k--) {
            V3_ਸੰਖਿਆ ਹੱਦ{fl, arg_ਚੌੜਾਈ, 0};
            ਜੇ (k < 32)
                ਹੱਦ.ਸੈੱਟ_Long(1ULL << k);
            ਨਹੀਂਤਾਂ
                ਹੱਦ.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(k, 1);

            AstLte* ਅਚਲ ltep
                = ਨਵਾਂ AstLte{fl, argp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), ਨਵਾਂ AstConst{fl, ਹੱਦ}};
            ltep->user1(ਸੱਚ);

            AstConst* ਅਚਲ valuep
                = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, ਨਤੀਜਾ_ਚੌੜਾਈ, (uint32_t)k};

            resultp = ਨਵਾਂ AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(ਸੱਚ);
        }

        VL_DO_DANGLING(argp->delete_ਰੁੱਖ(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        iterate(resultp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ਅਚਲ zerop = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    ਖਾਲੀ handlePow(Ast_ਗੰਢ_Biop* nodep) {
        ਜੇ (AstConst* ਅਚਲ exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ basep = nodep->lhsp();
            ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_One{nodep, basep->ਚੌੜਾਈ(), 1};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* powerp = ਨਵਾਂ AstConst{fl, ਅੰਕ_One};
            ਅਚਲ ਤਾਰਕਿਕ ਅਧਾਰ_Signed = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ਅਚਲ int32_t exponent = ਅਧਾਰ_Signed ? exponentp->ਮੰਜ਼ਿਲ_SInt() : exponentp->ਮੰਜ਼ਿਲ_UInt();
            ਜੇ (exponent > 0) {
                ਲਈ (int32_t i = 0; i < exponent; i++) {
                    ਜੇ (ਅਧਾਰ_Signed) {
                        powerp = ਨਵਾਂ AstMulS{fl, powerp, basep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    } ਨਹੀਂਤਾਂ {
                        powerp = ਨਵਾਂ AstMul{fl, powerp, basep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    }
                    powerp->user1(ਸੱਚ);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                ਜੇ (ਅਧਾਰ_Signed) {
                    powerp = ਨਵਾਂ AstDivS{fl, powerp, basep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    powerp->user1(ਸੱਚ);
                    powerp = ਨਵਾਂ AstDivS{fl, powerp, basep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    powerp->user1(ਸੱਚ);
                } ਨਹੀਂਤਾਂ {
                    powerp = ਨਵਾਂ AstDiv{fl, powerp, basep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    powerp->user1(ਸੱਚ);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            iterate(powerp);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstConst* ਅਚਲ basep = VN_CAST(nodep->lhsp(), Const)) {
            ਜੇ (basep->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->ਚੌੜਾਈ();
                ਅਚਲ ਤਾਰਕਿਕ rhsDependent = rhsp->user1();
                AstConst* ਅਚਲ onep = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, ਚੌੜਾਈ, 1};
                Ast_ਖਿਸਕਾਓ_L* ਅਚਲ shiftp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{fl, onep, rhsp, ਚੌੜਾਈ};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = shiftp;
                ਜੇ (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nClonep = rhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    nClonep->user1(rhsDependent);
                    AstConst* ਅਚਲ zeroNp
                        = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, rhsp->ਚੌੜਾਈ(), 0};
                    AstGteS* ਅਚਲ condp = ਨਵਾਂ AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ਅਚਲ zeroRp = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, ਚੌੜਾਈ, 0};
                    resultp = ਨਵਾਂ AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                iterate(resultp);
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } ਨਹੀਂਤਾਂ {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPow* nodep) override { handlePow(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPowSS* nodep) override { handlePow(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPowSU* nodep) override { handlePow(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    ਖਾਲੀ handle_ਖਿਸਕਾਓ(Ast_ਗੰਢ_Biop* nodep) {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        ਅਚਲ ਪੂਰਨ lhs_ਚੌੜਾਈ = nodep->lhsp()->ਚੌੜਾਈ();
        ਅਚਲ ਪੂਰਨ rhs_ਚੌੜਾਈ = nodep->rhsp()->ਚੌੜਾਈ();
        ਜੇ (rhs_ਚੌੜਾਈ < lhs_ਚੌੜਾਈ) {
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            AstExtend* ਅਚਲ extendp = ਨਵਾਂ AstExtend{fl, rhsp, lhs_ਚੌੜਾਈ};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(ਸੱਚ);
            nodep->rhsp(extendp);
        } ਨਹੀਂਤਾਂ ਜੇ (rhs_ਚੌੜਾਈ > lhs_ਚੌੜਾਈ) {
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->dtypep();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ extendp
                = VN_IS(nodep, ਖਿਸਕਾਓ_RS)
                      ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtendS{fl, lhsp, rhs_ਚੌੜਾਈ})
                      : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtend{fl, lhsp, rhs_ਚੌੜਾਈ});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(ਸੱਚ);
            nodep->lhsp(extendp);
            nodep->dtype_ਸੈੱਟ_Logic_Sized(rhs_ਚੌੜਾਈ, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlink_Fr_ਪਿੱਠ(&relinker);
            AstSel* ਅਚਲ selp = ਨਵਾਂ AstSel{fl, nodep, 0, lhs_ਚੌੜਾਈ};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            ਵਾਪਸ;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_L* nodep) override { handle_ਖਿਸਕਾਓ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_R* nodep) override { handle_ਖਿਸਕਾਓ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_RS* nodep) override { handle_ਖਿਸਕਾਓ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Biop* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Uniop* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        editSMT(nodep, nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Triop* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        ਜੇ (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtype_ਸੈੱਟ_String();
            ਵਾਪਸ;
        }
        // Fall back to "(ite cond then else)"
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_Triop*>(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        editSMT(nodep, nodep->srcp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        VNRelinker handle;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ msbp = ਨਵਾਂ AstSFormatF{
            fl, "%1d", ਝੂਠ,
            ਨਵਾਂ Ast_ਜੋੜੋ{fl, nodep->lsbp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                       ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(nodep->ਚੌੜਾਈ_Const() - 1)}}};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp
            = ਨਵਾਂ AstSFormatF{fl, "%1d", ਝੂਠ, nodep->lsbp()->unlink_Fr_ਪਿੱਠ(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        ਅਚਲ ਤਾਰਕਿਕ outermost = !m_defer_Rand_ਬਹੁਲਕ_Hoist;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origp = outermost ? nodep->clone_ਰੁੱਖ(ਝੂਠ) : ਖਾਲੀ_ਸੂਚਕ;
        VL_RESTORER(m_structSel);
        m_structSel = ਸੱਚ;
        ਜੇ (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = nodep->fromp();
            AstStructDType* ਅਚਲ structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ਜੇ (is_Rand_Qualified_ਰਸਤਾ(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } ਨਹੀਂਤਾਂ {
                structp->markConstrainedRand(ਸੱਚ);
            }
        }
        // Mark Random for structArray
        ਜੇ (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ਅਚਲ structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ਜੇ (is_Rand_Qualified_ਰਸਤਾ(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } ਨਹੀਂਤਾਂ {
                structp->markConstrainedRand(ਸੱਚ);
            }
        }
        {
            VL_RESTORER(m_defer_Rand_ਬਹੁਲਕ_Hoist);
            m_defer_Rand_ਬਹੁਲਕ_Hoist = ਸੱਚ;
            iterateChildren(nodep);
        }
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        AstSFormatF* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = ਨਵਾਂ AstSFormatF{fl, "%s.%s." + nodep->name(), ਝੂਠ,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)};
            ਜੇ (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } ਨਹੀਂਤਾਂ {
            newp = ਨਵਾਂ AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), ਝੂਠ,
                                   ਖਾਲੀ_ਸੂਚਕ};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਜੇ (origp && !hoist_Rand_ਬਹੁਲਕ_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(origp), origp);
        }
    }
    ਖਾਲੀ ਜੋੜੋ_String_Name_Part(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ indexp, ਅਚਲ std::string& fmt = "%32x") {
        ਜੇ (m_nestedAccess) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bitp = indexp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_Formatp = ਨਵਾਂ AstSFormatF{bitp->fileline(), fmt, ਝੂਠ, bitp};
            AstSFormatF* ਅਚਲ herep
                = ਨਵਾਂ AstSFormatF{indexp->fileline(), "%s.%s", ਝੂਠ, ਦੋਆਧਾਰੀ_ਅੰਕ_Formatp};
            m_nestedAccess->ਜੋੜੋ_Var_Name_Part(herep, indexp->name());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssocSel* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origp = nodep->clone_ਰੁੱਖ(ਝੂਠ);
        AstSFormatF* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            ਜੋੜੋ_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ idxp = ਨਵਾਂ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), ਝੂਠ,
                                                      nodep->bitp()->unlink_Fr_ਪਿੱਠ(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ਅਚਲ stringp = VN_AS(nodep->bitp(), CvtPackString);
            ਅਚਲ ਮਾਪ_t string_ਮਾਪ = VN_AS(stringp->lhsp(), Const)->ਚੌੜਾਈ();
            ਜੇ (string_ਮਾਪ > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << string_ਮਾਪ << "bits, limit is 128 bits");
            }
            ਜੋੜੋ_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ idxp = ਨਵਾਂ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), ਝੂਠ,
                                                      stringp->lhsp()->unlink_Fr_ਪਿੱਠ(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } ਨਹੀਂਤਾਂ {
            ਜੇ (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ਅਚਲ ਪੂਰਨ actual_ਚੌੜਾਈ = nodep->bitp()->ਚੌੜਾਈ();
                std::string fmt;
                // Normalize to standard bit width
                ਜੇ (actual_ਚੌੜਾਈ <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } ਨਹੀਂਤਾਂ ਜੇ (actual_ਚੌੜਾਈ <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } ਨਹੀਂਤਾਂ {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::ਮੰਜ਼ਿਲ_string(VL_ਸ਼ਬਦ_I(actual_ਚੌੜਾਈ) * 8) + "x";
                }
                ਜੇ (m_nestedAccess) ਜੋੜੋ_String_Name_Part(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ idxp
                    = ਨਵਾਂ AstSFormatF{fl, fmt, ਝੂਠ, nodep->bitp()->unlink_Fr_ਪਿੱਠ(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } ਨਹੀਂਤਾਂ {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                ਜੇ (m_nestedAccess) m_nestedAccess->error();
            }
        }
        ਜੇ (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        ਜੇ (!newp || !hoist_Rand_ਬਹੁਲਕ_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(origp->delete_ਰੁੱਖ(), origp);
        }
    }
    ਖਾਲੀ rename_Array_ਵਿਅੰਜਕ(AstSFormatF* ਅਚਲ newp) ਅਚਲ {
        ਜੇ (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        ਤਾਰਕਿਕ ਸੂਚਕਾਂਕ_Is_Rand = ਝੂਠ;
        nodep->bitp()->foreach([&](ਅਚਲ Ast_ਗੰਢ_Var_Ref* vrefp) {
            ਜੇ (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                ਸੂਚਕਾਂਕ_Is_Rand = ਸੱਚ;
            }
        });
        ਜੇ (m_nestedAccess) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bitp = nodep->bitp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_Formatp
                = ਸੂਚਕਾਂਕ_Is_Rand ? ਨਵਾਂ AstSFormatF{bitp->fileline(), bitp->name(), ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ}
                              : ਨਵਾਂ AstSFormatF{bitp->fileline(), "%x", ਝੂਠ, bitp};
            AstSFormatF* ਅਚਲ herep
                = ਨਵਾਂ AstSFormatF{nodep->fileline(), "%s.%s", ਝੂਠ, ਦੋਆਧਾਰੀ_ਅੰਕ_Formatp};
            ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->ਜੋੜੋ_Var_Name_Part(herep, selp->fromp()->name());
            } ਨਹੀਂਤਾਂ {
                m_nestedAccess->ਜੋੜੋ_Var_Name_Part(herep, bitp->name());
            }
            ਜੇ (ਸੂਚਕਾਂਕ_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_ਰੁੱਖ(), bitp);
                ਵਾਪਸ;
            }
        }
        ਜੇ (ਸੂਚਕਾਂਕ_Is_Rand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* indexp = nodep->bitp()->unlink_Fr_ਪਿੱਠ(&handle);
            ਜੇ (indexp->ਚੌੜਾਈ() < 32) {
                AstExtend* ਅਚਲ extendp = ਨਵਾਂ AstExtend{fl, indexp, 32};
                extendp->user1(ਸੱਚ);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ਅਚਲ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ਵਿਅੰਜਕ(newp);
        } ਨਹੀਂਤਾਂ {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origp = nodep->clone_ਰੁੱਖ(ਝੂਠ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ indexp
                = ਨਵਾਂ AstSFormatF{fl, "#x%8x", ਝੂਠ, nodep->bitp()->unlink_Fr_ਪਿੱਠ(&handle)};
            handle.relink(indexp);
            AstSFormatF* ਅਚਲ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ਵਿਅੰਜਕ(newp);
            ਜੇ (!newp || !hoist_Rand_ਬਹੁਲਕ_Over_Select_And_Member(newp, origp)) {
                VL_DO_DANGLING(origp->delete_ਰੁੱਖ(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ਸਥਿਰ ਤਾਰਕਿਕ is_Rand_ਬਹੁਲਕ_Gate(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਅਚਲ AstCMethodHard* ਅਚਲ atp = VN_CAST(exprp, CMethodHard);
        ਜੇ (!atp || atp->ਵਿਧੀ() != VCMethod::ARRAY_AT) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstVar* varp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        ਜੇ (!varp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(varp->user2p(), Class);
        ਵਾਪਸ classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    ਤਾਰਕਿਕ hoist_Rand_ਬਹੁਲਕ_Over_Select_And_Member(AstSFormatF* newp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* origp) {
        ਜੇ (m_defer_Rand_ਬਹੁਲਕ_Hoist) ਵਾਪਸ ਝੂਠ;
        // Only for selects yielding a non-array element (full chains)
        ਜੇ (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) ਵਾਪਸ ਝੂਠ;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::ਸਦਿਸ਼<AstSFormatF*> ਚੌਖਟੇ;
        AstCond* ਬਹੁਲਕ_Condp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ firstp = curp->exprsp();
            ਚੌਖਟੇ.ਧੱਕੋ_ਪਿੱਠ(curp);
            ਜੇ (AstCond* ਅਚਲ condp = VN_CAST(firstp, Cond)) {
                ਜੇ (is_Rand_ਬਹੁਲਕ_Gate(condp->condp())) ਬਹੁਲਕ_Condp = condp;
                ਰੋਕੋ;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        ਜੇ (!ਬਹੁਲਕ_Condp) ਵਾਪਸ ਝੂਠ;
        File_ਸਤਰ* ਅਚਲ fl = newp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ modep = ਬਹੁਲਕ_Condp->condp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* activep = ਬਹੁਲਕ_Condp->thenp()->unlink_Fr_ਪਿੱਠ();
        // Rebuild the select chain text around the SMT name, innermost first
        ਲਈ (ਸਵੈਚਾਲਿਤ it = ਚੌਖਟੇ.rbegin(); it != ਚੌਖਟੇ.rend(); ++it) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਬਚਿਆ_Argsp = VN_CAST((*it)->exprsp()->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
            ਜੇ (ਬਚਿਆ_Argsp) { ਬਚਿਆ_Argsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(); }
            activep = ਨਵਾਂ AstSFormatF{fl, (*it)->name(), ਝੂਠ,
                                      Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(activep, ਬਚਿਆ_Argsp)};
        }
        AstCond* ਅਚਲ hoistp = ਨਵਾਂ AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(ਸੱਚ);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(newp), newp);
        ਵਾਪਸ ਸੱਚ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        ਜੇ (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
            ਜੇ (m_nestedAccess) {
                AstSFormatF* ਅਚਲ formatp = ਨਵਾਂ AstSFormatF{fl, nodep->name(), ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ};
                m_nestedAccess->ਜੋੜੋ_Var_Name_Part(ਨਵਾਂ AstSFormatF{fl, "%s.%s", ਝੂਠ, formatp},
                                               nodep->name());
            }
            Ast_ਗੰਢ* ਜੜ੍ਹ_ਗੰਢ = nodep->fromp();
            ਜਦੋਂ (ਅਚਲ AstMemberSel* ਅਚਲ selp = VN_CAST(ਜੜ੍ਹ_ਗੰਢ, MemberSel)) {
                ਜੜ੍ਹ_ਗੰਢ = selp->fromp();
            }

            Ast_ਗੰਢ_Sel* arraySelp = VN_CAST(ਜੜ੍ਹ_ਗੰਢ, ArraySel);
            ਜੇ (!arraySelp) arraySelp = VN_CAST(ਜੜ੍ਹ_ਗੰਢ, AssocSel);

            ਜੇ (arraySelp) {
                Ast_ਗੰਢ_DType* ਅਚਲ arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_ਗੰਢ_DType* ਅਚਲ elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ਅਚਲ ਤਾਰਕਿਕ isSimple
                        = nodep->fromp() == ਜੜ੍ਹ_ਗੰਢ && VN_IS(arraySelp->fromp(), VarRef);
                    ਜੇ (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = ਨਵਾਂ Nested_Access_ਰਸਤਾ(nodep->clone_ਰੁੱਖ(ਝੂਠ),
                                                              &m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_Indexp);
                        AstSFormatF* ਅਚਲ formatp
                            = ਨਵਾਂ AstSFormatF{fl, nodep->name(), ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ};
                        m_nestedAccess->ਜੋੜੋ_Var_Name_Part(
                            ਨਵਾਂ AstSFormatF{fl, "%s.%s", ਝੂਠ, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = ਸੱਚ;
                    nodep->user1(ਸੱਚ);
                    arraySelp->user1(ਸੱਚ);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origp = nodep->clone_ਰੁੱਖ(ਝੂਠ);
                    iterateChildren(nodep);
                    AstSFormatF* ਅਚਲ newp = ਨਵਾਂ AstSFormatF{fl, "%s." + nodep->name(), ਝੂਠ,
                                                              nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
                    nodep->replaceWith(newp);
                    ਜੇ (!hoist_Rand_ਬਹੁਲਕ_Over_Select_And_Member(newp, origp))
                        VL_DO_DANGLING(origp->delete_ਰੁੱਖ(), origp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    VL_DO_DANGLING(ਮਿਟਾਓ m_nestedAccess, m_nestedAccess);
                    ਵਾਪਸ;
                }
            }
            // Check if the root variable participates in global constraints
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ varRefp = VN_CAST(ਜੜ੍ਹ_ਗੰਢ, VarRef)) {
                AstVar* ਅਚਲ constrainedVar = varRefp->varp();
                ਜੇ (constrainedVar->globalConstrained()) {
                    ਤਾਰਕਿਕ ਅਚਲ beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    ਜੇ (beforeIterate) {
                        nodep->replaceWith(ਨਵਾਂ AstSFormatF{fl, "%s." + nodep->name(), ਝੂਠ,
                                                           nodep->fromp()->unlink_Fr_ਪਿੱਠ()});
                    } ਨਹੀਂਤਾਂ {
                        nodep->replaceWith(nodep->fromp()->unlink_Fr_ਪਿੱਠ());
                    }
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                    ਵਾਪਸ;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        ਜੇ (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), ਗੰਢ_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ਅਚਲ varRefp
                = ਨਵਾਂ AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), ਗੰਢ_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਦੇਖੋ(varRefp);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->ਵਿਧੀ() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ਅਚਲ cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_ਗੰਢ_DType* ਅਚਲ arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = arrayDtp->subDTypep()->skipRefp();
            ਜੇ (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = ਸੱਚ;
                iterateChildren(nodep);
                File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                AstSFormatF* newp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (AstSFormatF* ਅਚਲ fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    ਜੇ (fromp->name() == "%s.%s") {
                        newp = ਨਵਾਂ AstSFormatF{fl, "%s.%s." + nodep->name(), ਝੂਠ,
                                               fromp->exprsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)};
                    } ਨਹੀਂਤਾਂ {
                        newp = ਨਵਾਂ AstSFormatF{fl, fromp->name() + "." + nodep->name(), ਝੂਠ,
                                               ਖਾਲੀ_ਸੂਚਕ};
                    }
                } ਨਹੀਂਤਾਂ {
                    newp = ਨਵਾਂ AstSFormatF{fl, nodep->name(), ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ {
            editFormat(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(AstCStmt* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintIf* nodep) override {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thenp = editSingle(fl, nodep->thensp());
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elsep = editSingle(fl, nodep->elsesp());
        ਜੇ (thenp && elsep) {
            newp = ਨਵਾਂ AstCond{fl, nodep->condp()->unlink_Fr_ਪਿੱਠ(), thenp, elsep};
        } ਨਹੀਂਤਾਂ ਜੇ (thenp) {
            newp = ਨਵਾਂ AstLogIf{fl, nodep->condp()->unlink_Fr_ਪਿੱਠ(), thenp};
        } ਨਹੀਂਤਾਂ ਜੇ (elsep) {
            newp = ਨਵਾਂ AstLogIf{fl, ਨਵਾਂ AstNot{fl, nodep->condp()->unlink_Fr_ਪਿੱਠ()}, elsep};
        }
        ਜੇ (newp) {
            newp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(ਸੱਚ);  // Assume result-dependent
            nodep->replaceWith(ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, newp});
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();

        ਜੇ (!nodep->bodyp()) {
            nodep->unlink_Fr_ਪਿੱਠ();
        } ਨਹੀਂਤਾਂ ਜੇ (m_wantSingle) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{fl};
            cstmtp->ਜੋੜੋ("ret += \" \";\n");
            cstmtp->ਜੋੜੋ("ret += ");
            cstmtp->ਜੋੜੋ(itemp);
            cstmtp->ਜੋੜੋ(";");
            Ast_ਗੰਢ* ਅਚਲ bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ਅਚਲ cexprp = ਨਵਾਂ AstCExpr{fl};
            cexprp->dtype_ਸੈੱਟ_String();
            cexprp->ਜੋੜੋ("([&]{\nstd::string ret;\n");
            cexprp->ਜੋੜੋ(ਨਵਾਂ AstBegin{fl, m_unique_ਨਾਂ.get("__VrandBlock"),
                                     ਨਵਾਂ AstForeach{fl, nodep->headerp()->unlink_Fr_ਪਿੱਠ(), bodyp},
                                     ਸੱਚ});
            cexprp->ਜੋੜੋ("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(ਨਵਾਂ AstSFormatF{fl, "%s", ਝੂਠ, cexprp});
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            ਲਈ (Ast_ਗੰਢ* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(elemp, Var)) m_foreachIdxVars.ਪਾਓ(varp);
            }
            iterate_And_ਅਗਲਾ_Null(nodep->bodyp());
            Ast_ਗੰਢ* ਅਚਲ bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            nodep->replaceWith(
                ਨਵਾਂ AstBegin{fl, m_unique_ਨਾਂ.get("__VrandBlock"),
                             ਨਵਾਂ AstForeach{fl, nodep->headerp()->unlink_Fr_ਪਿੱਠ(), bodyp}, ਸੱਚ});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_Module* ਅਚਲ genModp = VN_AS(m_genp->user2p(), ਗੰਢ_Module);

        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsTestp = build_Solve_Before_Name_ਵਿਅੰਜਕ(fl, lhsp);
            ਜੇ (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                ਜਾਰੀ;
            }
            VL_DO_DANGLING(lhsTestp->delete_ਰੁੱਖ(), lhsTestp);
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsNamep = build_Solve_Before_Name_ਵਿਅੰਜਕ(fl, rhsp);
                ਜੇ (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    ਜਾਰੀ;
                }
                AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
                    fl, ਨਵਾਂ AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtype_ਸੈੱਟ_Void();
                callp->ਜੋੜੋ_Pinsp(build_Solve_Before_Name_ਵਿਅੰਜਕ(fl, lhsp));
                callp->ਜੋੜੋ_Pinsp(rhsNamep);
                nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintUnique* nodep) override {
        Ast_ਗੰਢ_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_Module* genModp = ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (m_classp) {
            initTaskp = VN_AS(m_memberMap.ਲੱਭੋ_Member(m_classp, "new"), ਗੰਢ_FTask);
            genVarp = getRandomGenerator(m_classp);
        } ਨਹੀਂਤਾਂ {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        ਜੇ (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (m_classp) {
            genModp = VN_AS(genVarp->user2p(), ਗੰਢ_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_ਗੰਢ* ਤਿਆਰੀ_Stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::ਸਦਿਸ਼<std::string> smtExprs;
        ਲਈ (Ast_ਗੰਢ* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (AstVarRef* ਅਚਲ varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ਅਚਲ varp = varRefp->varp();
                Ast_ਗੰਢ_Module* ਅਚਲ varModp = VN_AS(varp->user2p(), ਗੰਢ_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep()->skipRefp();

                ਅਚਲ ਸਥਿਰ ਸਵੈਚਾਲਿਤ dynDTypeSupported
                    = [](ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep) -> ਤਾਰਕਿਕ {
                    ਵਾਪਸ VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, ਕਤਾਰ_DType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elem_ਚੌੜਾਈ;
                uint32_t static_ਮਾਪ = 0;  // Element count, zero when only known at run time
                ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ ਉੱਪਰ = VN_CAST(dtypep, UnpackArrayDType)) {
                    ਅਚਲ AstRange* ਅਚਲ rangep = ਉੱਪਰ->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    static_ਮਾਪ = ਉੱਪਰ->elementsConst();
                    ਜੇ (static_ਮਾਪ > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        ਜਾਰੀ;
                    }
                    elem_ਚੌੜਾਈ = static_cast<uint64_t>(varp->dtypep()->ਚੌੜਾਈ());
                } ਨਹੀਂਤਾਂ ਜੇ (dynDTypeSupported(dtypep)) {
                    elem_ਚੌੜਾਈ = static_cast<uint64_t>(dtypep->subDTypep()->ਚੌੜਾਈ());
                } ਨਹੀਂਤਾਂ {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(ਝੂਠ));
                    ਜਾਰੀ;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ subp = dtypep->subDTypep()->skipRefp();
                ਜੇ (VN_IS(subp, ਗੰਢ_Array_DType) || VN_IS(subp, ਕਤਾਰ_DType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    ਜਾਰੀ;
                }

                ਜੇ (m_wantSingle && !static_ਮਾਪ) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    ਜਾਰੀ;
                }

                ਜੇ (!varp->user3()) {
                    varp->user3(ਸੱਚ);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ varnamep = ਨਵਾਂ AstCExpr{
                        fl, AstCExpr::ਸ਼ੁੱਧ{}, "\"" + varp->name() + "\"", varp->ਚੌੜਾਈ()};

                    AstCMethodHard* ਅਚਲ writeVarCallp = ਨਵਾਂ AstCMethodHard{
                        fl, ਨਵਾਂ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ਅਚਲ argVarRefp = ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
                    ਜੇ (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->ਜੋੜੋ_Pinsp(argVarRefp);
                    writeVarCallp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, elem_ਚੌੜਾਈ});
                    writeVarCallp->ਜੋੜੋ_Pinsp(varnamep);
                    writeVarCallp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{fl, 1});  // Dimension
                    ਅਚਲ Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {.asUQuad = varp->user1()};
                    ਜੇ (rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) {
                        writeVarCallp->ਜੋੜੋ_Pinsp(
                            ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ});
                    }
                    writeVarCallp->dtype_ਸੈੱਟ_Void();
                    initTaskp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                ਜੇ (varp->user4p() && m_ਮਾਪ_Constrained_Arraysp) {
                    m_ਮਾਪ_Constrained_Arraysp->ਪਾਓ(varp);
                }

                ਜੇ (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    ਜੇ (static_ਮਾਪ < 2) ਜਾਰੀ;
                    std::string ਵਿਅੰਜਕ_Str = "(__Vbv (distinct";
                    ਲਈ (uint32_t i = 0; i < static_ਮਾਪ; ++i) {
                        ਵਿਅੰਜਕ_Str += " (select " + varp->name() + " #x";
                        ਲਈ (ਪੂਰਨ ਖਿਸਕਾਓ_2 = 28; ਖਿਸਕਾਓ_2 >= 0; ਖਿਸਕਾਓ_2 -= 4) {
                            ਵਿਅੰਜਕ_Str += "0123456789abcdef"[(i >> ਖਿਸਕਾਓ_2) & 0xf];
                        }
                        ਵਿਅੰਜਕ_Str += ')';
                    }
                    smtExprs.emplace_ਪਿੱਠ(ਵਿਅੰਜਕ_Str + "))");
                    ਜਾਰੀ;
                }

                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ namep = ਨਵਾਂ AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ਅਚਲ randUniqueCallp
                    = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->ਲੱਭੋ_Void_DType());
                randUniqueCallp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{fl, *m_uniqueConstraintId});
                ਤਿਆਰੀ_Stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(ਤਿਆਰੀ_Stmtsp, ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        ਜੇ (m_wantSingle && !smtExprs.ਖਾਲੀ_ਥਾਵਾਂ()) {
            std::string ਵਿਅੰਜਕ_Str = smtExprs.front();
            ਜੇ (smtExprs.ਮਾਪ() > 1) {
                ਵਿਅੰਜਕ_Str = "(bvand";
                ਲਈ (ਅਚਲ std::string& oneExprp : smtExprs) ਵਿਅੰਜਕ_Str += " " + oneExprp;
                ਵਿਅੰਜਕ_Str += ')';
            }
            AstSFormatF* ਅਚਲ newp = ਨਵਾਂ AstSFormatF{fl, ਵਿਅੰਜਕ_Str, ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (ਤਿਆਰੀ_Stmtsp) nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(ਤਿਆਰੀ_Stmtsp);
        nodep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Constraint_ਵਿਅੰਜਕ* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_ਪਹਿਲਾ_ਵਿਅੰਜਕ_Inside_Indexp);
        ਜੇ (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->exprp();
            ਜੇ (exprp->ਚੌੜਾਈ() > 1) {
                File_ਸਤਰ* ਅਚਲ fl = exprp->fileline();
                ਅਚਲ V3_ਸੰਖਿਆ ਅੰਕ_Zero{fl, exprp->ਚੌੜਾਈ(), 0};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ neqp
                    = ਨਵਾਂ AstNeq{fl, exprp->unlink_Fr_ਪਿੱਠ(), ਨਵਾਂ AstConst{fl, ਅੰਕ_Zero}};
                neqp->user1(ਸੱਚ);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        ਜੇ (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ਅਚਲ VCMethod ਵਿਧੀ
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
            nodep->fileline(),
            ਨਵਾਂ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ਗੰਢ_Module), m_genp,
                          VAccess::READWRITE},
            ਵਿਧੀ, nodep->exprp()->unlink_Fr_ਪਿੱਠ()};
        callp->dtype_ਸੈੱਟ_Void();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstCExpr{nodep->fileline(), AstCExpr::ਸ਼ੁੱਧ{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ਅਚਲ uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string pretty_ਪਾਠ;
        ਜੇ (!v3Global.opt.protectIds()) {
            pretty_ਪਾਠ = nodep->fileline()->pretty_ਸਰੋਤ();
            ਮਾਪ_t pos = 0;
            ਜਦੋਂ ((pos = pretty_ਪਾਠ.ਲੱਭੋ('"', pos)) != std::string::npos) {
                pretty_ਪਾਠ.ਪਾਓ(pos, "\\");
                pos += std::ਲੜੀ_ਲੰਬਾਈ("\\\"");
            }
        }
        callp->ਜੋੜੋ_Pinsp(
            ਨਵਾਂ AstCExpr{nodep->fileline(), AstCExpr::ਸ਼ੁੱਧ{}, "\"" + pretty_ਪਾਠ + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();

        ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_AT) {
            ਜੋੜੋ_String_Name_Part(nodep->pinsp(), "%x");
            ਅਚਲ ਤਾਰਕਿਕ ਸੂਚਕਾਂਕ_Is_Rand = nodep->pinsp()->exists([](ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ vrefp) {
                ਵਾਪਸ vrefp->varp()->rand().isRandomizable();
            });
            ਜੇ (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ਅਚਲ Ast_ਗੰਢ* ਅਚਲ basep = nodep->ਅਧਾਰ_Fromp(ਸੱਚ);
                    ਜੇ (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        ਵਾਪਸ;
                    }
                }
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origp = ਸੂਚਕਾਂਕ_Is_Rand ? ਖਾਲੀ_ਸੂਚਕ : nodep->clone_ਰੁੱਖ(ਝੂਠ);
                ਅਚਲ AstVarRef* ਅਚਲ ਅਧਾਰ_Refp
                    = m_structSel ? VN_CAST(nodep->fromp()->ਅਧਾਰ_Fromp(ਸੱਚ), VarRef) : ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (ਅਧਾਰ_Refp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(ਅਧਾਰ_Refp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ indexp = nodep->pinsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ arrayp = nodep->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    AstCMethodHard* ਅਚਲ currentSizep
                        = ਨਵਾਂ AstCMethodHard{fl, arrayp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), VCMethod::DYN_ਮਾਪ};
                    currentSizep->dtype_ਸੈੱਟ_Int();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਵਿੱਚ_Range_Requiredp = ਨਵਾਂ AstLogAnd{
                        fl,
                        ਨਵਾਂ AstLteS{fl,
                                    ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, indexp->ਚੌੜਾਈ(), 0},
                                    indexp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)},
                        ਨਵਾਂ AstGteS{fl, indexp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), currentSizep}};
                    AstCMethodHard* ਅਚਲ resizep
                        = ਨਵਾਂ AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             ਨਵਾਂ Ast_ਜੋੜੋ{fl, indexp, ਨਵਾਂ AstConst{fl, 1}}};
                    resizep->dtype_ਸੈੱਟ_Void();
                    m_prepareConstrainedArraysp->ਜੋੜੋ_Stmtsp(
                        ਨਵਾਂ AstIf{fl, ਵਿੱਚ_Range_Requiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ਅਚਲ sizep
                    = m_structSel ? ਨਵਾਂ AstCMethodHard{fl, nodep->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                       VCMethod::DYN_ਮਾਪ}
                                  : ਖਾਲੀ_ਸੂਚਕ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinp = nodep->pinsp()->unlink_Fr_ਪਿੱਠ();
                AstSFormatF* newp;
                ਜੇ (m_structSel) {
                    ਜੇ (AstSFormatF* ਅਚਲ sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argsp
                        = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), pinp);
                    sizep->dtype_ਸੈੱਟ_Int();
                    AstLogAnd* ਅਚਲ condp
                        = ਨਵਾਂ AstLogAnd{fl,
                                        ਨਵਾਂ AstLteS{fl,
                                                    ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{},
                                                                 originalPinp->ਚੌੜਾਈ(), 0},
                                                    originalPinp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)},
                                        ਨਵਾਂ AstLtS{fl, originalPinp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), sizep}};
                    m_conditionp = m_conditionp ? ਨਵਾਂ AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = ਨਵਾਂ AstSFormatF{fl, "%s.%s", ਝੂਠ, argsp};
                } ਨਹੀਂਤਾਂ {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_ਪਿੱਠ(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                ਜੇ (origp && !hoist_Rand_ਬਹੁਲਕ_Over_Select_And_Member(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_ਰੁੱਖ(), origp);
                }

            } ਨਹੀਂਤਾਂ ਜੇ (ਸੂਚਕਾਂਕ_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } ਨਹੀਂਤਾਂ {
                nodep->user1(ਝੂਠ);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            ਵਾਪਸ;
        }

        ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_INSIDE) {
            ਅਚਲ ਤਾਰਕਿਕ randArr = nodep->fromp()->user1();

            AstVar* ਅਚਲ newVarp
                = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->ਲੱਭੋ_Int_DType()};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ idxRefp = ਨਵਾਂ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            Ast_Foreach_ਸਿਰਲੇਖ* ਅਚਲ headerp
                = ਨਵਾਂ Ast_Foreach_ਸਿਰਲੇਖ{fl, nodep->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), newVarp};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_ਗੰਢ* ਅਚਲ itemp = ਨਵਾਂ AstEq{fl, selp, nodep->pinsp()->unlink_Fr_ਪਿੱਠ()};
            itemp->user1(ਸੱਚ);

            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{fl};
            cstmtp->ਜੋੜੋ("ret += \" \";\n");
            cstmtp->ਜੋੜੋ("ret += ");
            cstmtp->ਜੋੜੋ(iterateSubtreeReturnEdits(itemp));
            cstmtp->ਜੋੜੋ(";");
            AstCExpr* ਅਚਲ cexprp = ਨਵਾਂ AstCExpr{fl};
            cexprp->dtype_ਸੈੱਟ_String();
            cexprp->ਜੋੜੋ("([&]{\nstd::string ret;\n");
            cexprp->ਜੋੜੋ(ਨਵਾਂ AstBegin{fl, m_unique_ਨਾਂ.get("__VrandBlock"),
                                     ਨਵਾਂ AstForeach{fl, headerp, cstmtp}, ਸੱਚ});
            cexprp->ਜੋੜੋ("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(ਨਵਾਂ AstSFormatF{fl, "%s", ਝੂਠ, cexprp});
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }

        // Array reduction methods (sum, product, and, or, xor)
        ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_ਜੋੜ
            || nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_ਗੁਣਨਫਲ
            || nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_AND || nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_OR
            || nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_XOR) {

            AstWith* ਅਚਲ withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            ਜੇ (withp) {
                ਜੇ (AstUnpackArrayDType* ਅਚਲ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ਅਚਲ ਪੂਰਨ array_ਮਾਪ_2 = adtypep->elementsConst();
                    ਜੇ (array_ਮਾਪ_2 > v3Global.opt.constraint_Array_ਹੱਦਬੰਦੀ()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvt_ਮੰਜ਼ਿਲ_Str(array_ਮਾਪ_2)
                                          + " exceeds --constraint-array-limit of "
                                          + cvt_ਮੰਜ਼ਿਲ_Str(v3Global.opt.constraint_Array_ਹੱਦਬੰਦੀ())
                                          + "), treating as state");
                        nodep->user1(ਝੂਠ);
                        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        ਵਾਪਸ;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ਅਚਲ ਤਾਰਕਿਕ hasRandomArray = nodep->fromp()->user1();
            ਜੇ (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(ਝੂਠ);
                ਜੇ (editFormat(nodep)) ਵਾਪਸ;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                ਵਾਪਸ;
            }
            ਜੇ (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                ਵਾਪਸ;
            }

            // Create loop variable and header
            AstVar* ਅਚਲ loopVarp
                = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->ਲੱਭੋ_Int_DType()};
            Ast_Foreach_ਸਿਰਲੇਖ* ਅਚਲ headerp
                = ਨਵਾਂ Ast_Foreach_ਸਿਰਲੇਖ{fl, nodep->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), loopVarp};

            // Determine SMT operation and compute identity elements
            ਅਚਲ ਅੱਖਰ* smtOp = ਖਾਲੀ_ਸੂਚਕ;
            std::string ਪਛਾਣ_ਤੱਤ;
            ਜੇ (withp) {
                // For 'with' clause: use binary format, compute from result width
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->dtypep()->ਚੌੜਾਈ();
                ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_ਜੋੜ) {
                    smtOp = "bvadd";
                    ਪਛਾਣ_ਤੱਤ = "#b" + std::string(ਚੌੜਾਈ, '0');
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_ਗੁਣਨਫਲ) {
                    smtOp = "bvmul";
                    ਪਛਾਣ_ਤੱਤ = (ਚੌੜਾਈ > 0) ? "#b" + std::string(ਚੌੜਾਈ - 1, '0') + "1" : "#b0";
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    ਪਛਾਣ_ਤੱਤ = "#b" + std::string(ਚੌੜਾਈ, '1');
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    ਪਛਾਣ_ਤੱਤ = "#b" + std::string(ਚੌੜਾਈ, '0');
                } ਨਹੀਂਤਾਂ {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    ਪਛਾਣ_ਤੱਤ = "#b" + std::string(ਚੌੜਾਈ, '0');
                }
            } ਨਹੀਂਤਾਂ {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ਅਚਲ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ਅਚਲ arrVarp = arrRefp->varp();
                Ast_ਗੰਢ_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ਅਚਲ ਪੂਰਨ elem_ਚੌੜਾਈ = elemDtp->ਚੌੜਾਈ();
                ਅਚਲ ਪੂਰਨ hexDigits = (elem_ਚੌੜਾਈ + 3) / 4;

                ਅਚਲ std::string zero_ਪਛਾਣ_ਤੱਤ = "#x" + std::string(hexDigits, '0');
                ਅਚਲ std::string one_ਪਛਾਣ_ਤੱਤ = "#x" + std::string(hexDigits - 1, '0') + "1";
                ਅਚਲ std::string all_Ones_ਪਛਾਣ_ਤੱਤ = "#x" + std::string(hexDigits, 'f');

                ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_ਜੋੜ) {
                    smtOp = "bvadd";
                    ਪਛਾਣ_ਤੱਤ = zero_ਪਛਾਣ_ਤੱਤ;
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_ਗੁਣਨਫਲ) {
                    smtOp = "bvmul";
                    ਪਛਾਣ_ਤੱਤ = one_ਪਛਾਣ_ਤੱਤ;
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    ਪਛਾਣ_ਤੱਤ = all_Ones_ਪਛਾਣ_ਤੱਤ;
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    ਪਛਾਣ_ਤੱਤ = zero_ਪਛਾਣ_ਤੱਤ;
                } ਨਹੀਂਤਾਂ {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    ਪਛਾਣ_ਤੱਤ = zero_ਪਛਾਣ_ਤੱਤ;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ਅਚਲ cstmtp = ਨਵਾਂ AstCStmt{fl};
            ਜੇ (withp) {
                // With 'with' clause: evaluate expression for each element
                ਅਚਲ ਤਾਰਕਿਕ randArr = nodep->fromp()->user1();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ idxRefp = ਨਵਾਂ AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ਅਚਲ ਪੂਰਨ ਨਤੀਜਾ_ਚੌੜਾਈ = nodep->dtypep()->ਚੌੜਾਈ();
                ਅਚਲ VSigning ਨਤੀਜਾ_Signing = nodep->dtypep()->numeric();

                Ast_ਗੰਢ* ਪ੍ਰਤੀ_Elem_Exprp = withp->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                ਜੇ (AstLambdaArgRef* ਅਚਲ ਜੜ੍ਹ_Refp = VN_CAST(ਪ੍ਰਤੀ_Elem_Exprp, LambdaArgRef)) {
                    ਜੇ (ਜੜ੍ਹ_Refp->ਸੂਚਕਾਂਕ()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ਅਚਲ loopRef = ਨਵਾਂ AstVarRef{fl, loopVarp, VAccess::READ};
                        ਪ੍ਰਤੀ_Elem_Exprp = adjust_ਚੌੜਾਈ(fl, loopRef, ਨਤੀਜਾ_ਚੌੜਾਈ, ਨਤੀਜਾ_Signing);
                    } ਨਹੀਂਤਾਂ {
                        // item at root -> replace with array element
                        ਪ੍ਰਤੀ_Elem_Exprp = elemSelp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    }
                    VL_DO_DANGLING(ਜੜ੍ਹ_Refp->delete_ਰੁੱਖ(), ਜੜ੍ਹ_Refp);
                } ਨਹੀਂਤਾਂ {
                    ਪ੍ਰਤੀ_Elem_Exprp->foreach([&](AstLambdaArgRef* refp) {
                        ਜੇ (refp->ਸੂਚਕਾਂਕ()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ਅਚਲ loopRef = ਨਵਾਂ AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_ਗੰਢ* ਅਚਲ replacement
                                = adjust_ਚੌੜਾਈ(fl, loopRef, ਨਤੀਜਾ_ਚੌੜਾਈ, ਨਤੀਜਾ_Signing);
                            refp->replaceWith(replacement);
                        } ਨਹੀਂਤਾਂ {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                        }
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_ਰੁੱਖ(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                ਪ੍ਰਤੀ_Elem_Exprp = V3Const::constifyEdit(ਪ੍ਰਤੀ_Elem_Exprp);

                ਪ੍ਰਤੀ_Elem_Exprp->foreach([&](Ast_ਗੰਢ* nodep) {
                    // Don't mark loop variable references as randomizable
                    ਜੇ (!VN_IS(nodep, Const)) {
                        ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ vrefp = VN_CAST(nodep, ਗੰਢ_Var_Ref)) {
                            ਜੇ (vrefp->varp() != loopVarp) nodep->user1(ਸੱਚ);
                        } ਨਹੀਂਤਾਂ {
                            nodep->user1(ਸੱਚ);
                        }
                    }
                });

                cstmtp->ਜੋੜੋ("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->ਜੋੜੋ("ret += ");
                cstmtp->ਜੋੜੋ(iterateSubtreeReturnEdits(ਪ੍ਰਤੀ_Elem_Exprp));
                cstmtp->ਜੋੜੋ(";\n");
                cstmtp->ਜੋੜੋ("ret += \")\";\n");
            } ਨਹੀਂਤਾਂ {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ਅਚਲ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ਅਚਲ arrVarp = arrRefp->varp();
                ਅਚਲ std::string smtArrayName = arrVarp->name();
                Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ਅਚਲ ਪੂਰਨ elem_ਚੌੜਾਈ = elemDtp->ਚੌੜਾਈ();
                Ast_ਗੰਢ_Module* ਅਚਲ arrModulep = VN_AS(arrVarp->user2p(), ਗੰਢ_Module);
                Ast_ਗੰਢ_Module* ਅਚਲ genModulep = VN_AS(m_genp->user2p(), ਗੰਢ_Module);
                arrVarp->user3(ਸੱਚ);

                // Add write_var call to init task
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ arrVarNamep
                    = ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "\"" + smtArrayName + "\"", elem_ਚੌੜਾਈ};
                AstCMethodHard* ਅਚਲ writeVarCallp
                    = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->ਜੋੜੋ_Pinsp(
                    ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elem_ਚੌੜਾਈ)});
                writeVarCallp->ਜੋੜੋ_Pinsp(arrVarNamep);
                ਅਚਲ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(ਝੂਠ).ਦੂਜਾ;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{fl, 1});  // Dimension

                ਅਚਲ Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {.asUQuad = arrVarp->user1()};
                ਜੇ (rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) {
                    writeVarCallp->ਜੋੜੋ_Pinsp(
                        ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ});
                }
                writeVarCallp->dtype_ਸੈੱਟ_Void();
                Ast_ਗੰਢ_FTask* initTaskp = m_inlineInitTaskp;
                ਜੇ (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.ਲੱਭੋ_Member(arrModulep, "new"), ਗੰਢ_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->ਜੋੜੋ_Stmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ਅਚਲ idxFormatp = ਨਵਾਂ AstSFormatF{
                    fl, "#x%08x", ਝੂਠ, ਨਵਾਂ AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ਅਚਲ varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_ਪਿੱਠ(), idxFormatp);

                cstmtp->ਜੋੜੋ("ret += \" \";\n");
                cstmtp->ਜੋੜੋ("ret += ");
                cstmtp->ਜੋੜੋ(varNamep);
                cstmtp->ਜੋੜੋ(";\n");
            }

            // Build final expression
            ਜੇ (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ਅਚਲ cexprp = ਨਵਾਂ AstCExpr{fl};
                cexprp->dtype_ਸੈੱਟ_String();
                cexprp->ਜੋੜੋ("([&]{\nstd::string ret = \"" + ਪਛਾਣ_ਤੱਤ + "\";\n");
                cexprp->ਜੋੜੋ(ਨਵਾਂ AstBegin{fl, m_unique_ਨਾਂ.get("__VrandBlock"),
                                         ਨਵਾਂ AstForeach{fl, headerp, cstmtp}, ਸੱਚ});
                cexprp->ਜੋੜੋ("return ret;\n})()");
                nodep->replaceWith(ਨਵਾਂ AstSFormatF{fl, "%s", ਝੂਠ, cexprp});
            } ਨਹੀਂਤਾਂ {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ਅਚਲ cexprp = ਨਵਾਂ AstCExpr{fl};
                cexprp->dtype_ਸੈੱਟ_String();
                cexprp->ਜੋੜੋ("([&]{\nstd::string ret;\n");
                cexprp->ਜੋੜੋ(ਨਵਾਂ AstBegin{fl, m_unique_ਨਾਂ.get("__VrandBlock"),
                                         ਨਵਾਂ AstForeach{fl, headerp, cstmtp}, ਸੱਚ});
                cexprp->ਜੋੜੋ("return ret.empty() ? \"" + ਪਛਾਣ_ਤੱਤ + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(ਨਵਾਂ AstSFormatF{fl, "%s", ਝੂਠ, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }

        // We've handled DYN_SIZE already if they should be randomized
        ਜੇ (nodep->ਵਿਧੀ() == VCMethod::DYN_ਮਾਪ || nodep->ਵਿਧੀ() == VCMethod::ASSOC_ਮਾਪ) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਗੰਢ_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            ਜਦੋਂ (VN_IS(ਗੰਢ_Exprp, MemberSel)) {
                ਗੰਢ_Exprp = VN_AS(ਗੰਢ_Exprp, MemberSel)->fromp();
            }
            ਜੇ (VN_IS(ਗੰਢ_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)));
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                ਵਾਪਸ;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(ਝੂਠ);

        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;

        AstFunc* ਅਚਲ funcp = VN_CAST(nodep->taskp(), Func);
        ਜੇ (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            ਵਾਪਸ;
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ retExprp = ਕੱਢੋ_Return_ਵਿਅੰਜਕ(funcp);
        ਜੇ (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(ਝੂਠ);
            ਜੇ (editFormat(nodep)) ਵਾਪਸ;
            ਵਾਪਸ;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ਅਚਲ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* inlinedp = retExprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            ਅਚਲ AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
            AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
            ਜੇ (!argp || !argp->exprp()) ਜਾਰੀ;
            ਜੇ (AstVarRef* ਅਚਲ ਜੜ੍ਹ_Refp = VN_CAST(inlinedp, VarRef)) {
                ਜੇ (ਜੜ੍ਹ_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    VL_DO_DANGLING(ਜੜ੍ਹ_Refp->delete_ਰੁੱਖ(), ਜੜ੍ਹ_Refp);
                    ਜਾਰੀ;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                ਜੇ (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                    VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (editFormat(nodep)) ਵਾਪਸ;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_ਗੰਢ* buildDisableSoftCallStmt(Ast_Constraint_ਵਿਅੰਜਕ* nodep) {
        std::string varName;
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ vrefp = VN_CAST(nodep->exprp(), ਗੰਢ_Var_Ref)) {
            varName = vrefp->name();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
            nodep->fileline(),
            ਨਵਾਂ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ਗੰਢ_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            ਨਵਾਂ AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtype_ਸੈੱਟ_Void();
        ਵਾਪਸ callp->makeStmt();
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
    Ast_ਗੰਢ* ਕੱਢੋ_Conditional_Disable_Softs(Ast_ਗੰਢ* itemsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* outerCondp) {
        Ast_ਗੰਢ* hoistListp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ਜੇ (AstConstraintIf* ਅਚਲ ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_ਸਤਰ* ਅਚਲ fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thenCondp
                    = outerCondp ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                       ਨਵਾਂ AstLogAnd{fl, outerCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                     ifp->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)})
                                 : ifp->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                ਜੇ (Ast_ਗੰਢ* ਅਚਲ thenHoistp
                    = ਕੱਢੋ_Conditional_Disable_Softs(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_ਰੁੱਖ(), thenCondp);
                ਜੇ (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ notInnerp
                        = ਨਵਾਂ AstLogNot{fl, ifp->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elseCondp
                        = outerCondp ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstLogAnd{
                                           fl, outerCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), notInnerp})
                                     : notInnerp;
                    ਜੇ (Ast_ਗੰਢ* ਅਚਲ elseHoistp
                        = ਕੱਢੋ_Conditional_Disable_Softs(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_ਰੁੱਖ(), elseCondp);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_CAST(stmtp, Constraint_ਵਿਅੰਜਕ)) {
                ਜੇ (exprp->isDisableSoft() && outerCondp) {
                    File_ਸਤਰ* ਅਚਲ fl = exprp->fileline();
                    AstIf* ਅਚਲ hoistIfp = ਨਵਾਂ AstIf{fl, outerCondp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                                      buildDisableSoftCallStmt(exprp), ਖਾਲੀ_ਸੂਚਕ};
                    hoistListp = hoistListp ? Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(hoistListp,
                                                               static_cast<Ast_ਗੰਢ*>(hoistIfp))
                                            : static_cast<Ast_ਗੰਢ*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ truep
                        = ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_ਰੁੱਖ(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        ਵਾਪਸ hoistListp;
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ Constraint_ਵਿਅੰਜਕ_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_ਗੰਢ* nodep,
                                   Ast_ਗੰਢ_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* rand_ਬਹੁਲਕ_Varp, std::ਸੈੱਟ<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3_Unique_ਨਾਂ& unique_ਨਾਂ,
                                   Ast_ਗੰਢ_FTask* memberselInitTaskp = ਖਾਲੀ_ਸੂਚਕ,
                                   VInsertion_ਸੈੱਟ<AstVar*>* ਮਾਪ_Constrained_Arraysp = ਖਾਲੀ_ਸੂਚਕ,
                                   Ast_ਗੰਢ_FTask* prepareConstrainedArraysp = ਖਾਲੀ_ਸੂਚਕ)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_rand_ਬਹੁਲਕ_Varp{rand_ਬਹੁਲਕ_Varp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_ਮਾਪ_Constrained_Arraysp{ਮਾਪ_Constrained_Arraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_unique_ਨਾਂ{unique_ਨਾਂ} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ hoistListp
            = nodep ? ਕੱਢੋ_Conditional_Disable_Softs(nodep, ਖਾਲੀ_ਸੂਚਕ) : ਖਾਲੀ_ਸੂਚਕ) {
            nodep->ਜੋੜੋ_ਅਗਲਾ(hoistListp);
        }
        iterate_And_ਅਗਲਾ_Null(nodep);
    }
};

ਗਿਣਤੀ ਵਰਗ Capture_ਬਹੁਲਕ : uint8_t {
    CAP_NO = 0x0,
    CAP_ਮੁੱਲ = 0x01,
    CAP_THIS = 0x02,
    CAP_F_ਸੈੱਟ_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
Capture_ਬਹੁਲਕ ਸੰਚਾਲਕ|(Capture_ਬਹੁਲਕ a, Capture_ਬਹੁਲਕ b) {
    ਵਾਪਸ static_cast<Capture_ਬਹੁਲਕ>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
Capture_ਬਹੁਲਕ ਸੰਚਾਲਕ&(Capture_ਬਹੁਲਕ a, Capture_ਬਹੁਲਕ b) {
    ਵਾਪਸ static_cast<Capture_ਬਹੁਲਕ>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
Capture_ਬਹੁਲਕ ਬਹੁਲਕ(Capture_ਬਹੁਲਕ a) { ਵਾਪਸ a & static_cast<Capture_ਬਹੁਲਕ>(0x3); }
ਤਾਰਕਿਕ hasFlags(Capture_ਬਹੁਲਕ a, Capture_ਬਹੁਲਕ flags) {
    ਵਾਪਸ ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

ਵਰਗ CaptureVisitor final : ਜਨਤਕ VNVisitor {
    AstArg* m_argsp = ਖਾਲੀ_ਸੂਚਕ;  // Original references turned into arguments
    Ast_ਗੰਢ_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ਅਚਲ AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::ਸੈੱਟ<Ast_ਗੰਢ*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = ਖਾਲੀ_ਸੂਚਕ;  // Variable for outer context's object, if necessary
    ਤਾਰਕਿਕ m_staticContext = ਝੂਠ;  // True when capturing from a static function context

    // METHODS

    ਤਾਰਕਿਕ captureVariable(File_ਸਤਰ* ਅਚਲ fileline, Ast_ਗੰਢ_Var_Ref* varrefp, AstVar*& varp) {
        ਸਵੈਚਾਲਿਤ it = m_varCloneMap.ਲੱਭੋ(varrefp->varp());
        ਜੇ (it == m_varCloneMap.ਸਿਰਾ()) {
            AstVar* ਅਚਲ newVarp = varrefp->varp()->clone_ਰੁੱਖ(ਝੂਠ);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(ਸੱਚ);
            newVarp->direction(VDirection::ਨਿਵੇਸ਼);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            ਵਾਪਸ ਸੱਚ;
        }
        varp = it->ਦੂਜਾ;
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਗੰਢ>
    ਖਾਲੀ fixupClassOrPackage(Ast_ਗੰਢ* memberp, T_ਗੰਢ refp) {
        Ast_ਗੰਢ_Module* ਅਚਲ declClassp = VN_AS(memberp->user2p(), ਗੰਢ_Module);
        ਜੇ (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਗੰਢ>
    ਤਾਰਕਿਕ is_Reference_ਮੰਜ਼ਿਲ_Inner_Member(T_ਗੰਢ nodep) {
        ਵਾਪਸ VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_ਸਤਰ* fl) {
        ਜੇ (!m_thisp) {
            AstClassRefDType* ਅਚਲ refDTypep
                = ਨਵਾਂ AstClassRefDType{fl, VN_AS(m_callerp, Class), ਖਾਲੀ_ਸੂਚਕ};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(refDTypep);
            m_thisp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(ਸੱਚ);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::ਨਿਵੇਸ਼);
            m_argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_argsp, ਨਵਾਂ AstArg{fl, "", ਨਵਾਂ AstThisRef{fl, refDTypep}});
        }
        ਵਾਪਸ m_thisp;
    }

    AstVar* getVar(AstVar* ਅਚਲ varp) ਅਚਲ {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_varCloneMap.ਲੱਭੋ(varp);
        ਜੇ (it == m_varCloneMap.ਸਿਰਾ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (it->ਦੂਜਾ->isStdRandomizeArg()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ it->ਦੂਜਾ;
    }

    Capture_ਬਹੁਲਕ get_Var_Ref_Capture_ਬਹੁਲਕ(Ast_ਗੰਢ_Var_Ref* varRefp) {
        Ast_ਗੰਢ_Module* ਅਚਲ varModp = VN_AS(varRefp->varp()->user2p(), ਗੰਢ_Module);
        AstClass* ਅਚਲ varClassp = VN_CAST(varModp, Class);
        AstClass* ਅਚਲ callerClassp = VN_CAST(m_callerp, Class);

        ਅਚਲ ਤਾਰਕਿਕ callerIsClass = callerClassp;
        ਅਚਲ ਤਾਰਕਿਕ refIsXref = VN_IS(varRefp, VarXRef);
        ਅਚਲ ਤਾਰਕਿਕ varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ਅਚਲ ਤਾਰਕਿਕ varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ਅਚਲ ਤਾਰਕਿਕ var_Is_ਖੇਤਰ_ਦਾ_Caller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ਅਚਲ ਤਾਰਕਿਕ varIsParam = varRefp->varp()->isParam();
        ਅਚਲ ਤਾਰਕਿਕ varIsConstraintIterator
            = VN_IS(varRefp->varp()->ਪਹਿਲਾ_Abovep(), Foreach_ਸਿਰਲੇਖ)
              && VN_IS(varRefp->varp()->ਪਹਿਲਾ_Abovep()->ਪਹਿਲਾ_Abovep(), ConstraintForeach);
        ਜੇ (refIsXref) ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_ਮੁੱਲ | Capture_ਬਹੁਲਕ::CAP_F_XREF;
        ਜੇ (varIsConstraintIterator) ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_NO;
        ਜੇ (varIsFuncLocal && varHasAutomaticLifetime) ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_ਮੁੱਲ;
        ਜੇ (varIsParam) ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_ਮੁੱਲ;
        // Static var in function (will not be inlined, because it's in class)
        ਜੇ (callerIsClass && varIsFuncLocal) ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_ਮੁੱਲ;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        ਜੇ (m_staticContext && callerIsClass && var_Is_ਖੇਤਰ_ਦਾ_Caller
            && varRefp->varp()->lifetime().isStatic())
            ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_NO;
        ਜੇ (callerIsClass && var_Is_ਖੇਤਰ_ਦਾ_Caller) ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        ਜੇ (callerIsClass && varClassp && !var_Is_ਖੇਤਰ_ਦਾ_Caller
            && varRefp->varp()->lifetime().isStatic())
            ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_ਮੁੱਲ;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        ਵਾਪਸ Capture_ਬਹੁਲਕ::CAP_ਮੁੱਲ;
    }

    ਖਾਲੀ capture_Ref_By_ਮੁੱਲ(Ast_ਗੰਢ_Var_Ref* nodep, Capture_ਬਹੁਲਕ cap_ਬਹੁਲਕ_Flags) {
        AstVar* newVarp;
        ਅਚਲ ਤਾਰਕਿਕ newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_ਗੰਢ_Var_Ref* ਅਚਲ newVarRefp = newCapture ? nodep->clone_ਰੁੱਖ(ਝੂਠ) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!hasFlags(cap_ਬਹੁਲਕ_Flags, Capture_ਬਹੁਲਕ::CAP_F_ਸੈੱਟ_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        ਜੇ (!newCapture) ਵਾਪਸ;
        ਜੇ (hasFlags(cap_ਬਹੁਲਕ_Flags, Capture_ਬਹੁਲਕ::CAP_F_XREF)) {
            AstVarRef* ਅਚਲ notXVarRefp
                = ਨਵਾਂ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(m_argsp, ਨਵਾਂ AstArg{nodep->fileline(), "", newVarRefp});
    }

    ਖਾਲੀ captureRefByThis(Ast_ਗੰਢ_Var_Ref* nodep, Capture_ਬਹੁਲਕ /*capModeFlags*/) {
        AstVar* ਅਚਲ thisp = importThisp(nodep->fileline());
        AstVarRef* ਅਚਲ thisRefp = ਨਵਾਂ AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(ਸੱਚ);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ਅਚਲ memberSelp
            = ਨਵਾਂ AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        ਜੇ (!m_targetp && nodep->user1()) memberSelp->user1(ਸੱਚ);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (m_ignore.ਗਿਣਤੀ_2(nodep)) ਵਾਪਸ;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        ਜੇ (nodep->varp()->isStdRandomizeArg()) ਵਾਪਸ;
        ਅਚਲ Capture_ਬਹੁਲਕ cap_ਬਹੁਲਕ = get_Var_Ref_Capture_ਬਹੁਲਕ(nodep);
        ਜੇ (ਬਹੁਲਕ(cap_ਬਹੁਲਕ) == Capture_ਬਹੁਲਕ::CAP_NO) ਵਾਪਸ;
        ਜੇ (ਬਹੁਲਕ(cap_ਬਹੁਲਕ) == Capture_ਬਹੁਲਕ::CAP_ਮੁੱਲ) capture_Ref_By_ਮੁੱਲ(nodep, cap_ਬਹੁਲਕ);
        ਜੇ (ਬਹੁਲਕ(cap_ਬਹੁਲਕ) == Capture_ਬਹੁਲਕ::CAP_THIS) captureRefByThis(nodep, cap_ਬਹੁਲਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (m_ignore.ਗਿਣਤੀ_2(nodep)) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        ਜੇ (VN_IS(nodep, ਵਿਧੀ_Call) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        AstClass* ਅਚਲ classp = VN_CAST(nodep->taskp()->user2p(), Class);
        ਜੇ ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ਅਚਲ argsp = nodep->argsp();
            ਜੇ (argsp) argsp->unlink_Fr_ਪਿੱਠ();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ਅਚਲ thisp = importThisp(nodep->fileline());
            AstVarRef* ਅਚਲ thisRefp = ਨਵਾਂ AstVarRef{
                nodep->fileline(), thisp, nodep->is_ਸ਼ੁੱਧ() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp
                = ਨਵਾਂ Ast_ਵਿਧੀ_Call{nodep->fileline(), thisRefp, thisp->name(), argsp};
            ਵਿਧੀ_Callp->taskp(nodep->taskp());
            ਵਿਧੀ_Callp->dtypep(nodep->dtypep());
            nodep->replaceWith(ਵਿਧੀ_Callp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            m_ignore.emplace(ਵਿਧੀ_Callp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        ਜੇ (!is_Reference_ਮੰਜ਼ਿਲ_Inner_Member(nodep)) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        AstVarRef* ਅਚਲ varRefp
            = ਨਵਾਂ AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਧੀ_Call* nodep) override {
        ਜੇ (!is_Reference_ਮੰਜ਼ਿਲ_Inner_Member(nodep) || m_ignore.ਗਿਣਤੀ_2(nodep)) {
            iterateChildren(nodep);
            ਵਾਪਸ;
        }
        AstArg* ਅਚਲ argsp = nodep->argsp();
        ਜੇ (argsp) argsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        Ast_ਗੰਢ_FTask_Ref* taskRefp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstTask* ਅਚਲ taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = ਨਵਾਂ AstTaskRef{nodep->fileline(), taskp, argsp};
        } ਨਹੀਂਤਾਂ ਜੇ (AstFunc* ਅਚਲ taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = ਨਵਾਂ AstFuncRef{nodep->fileline(), taskp, argsp};
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    ਸਪਸ਼ਟ CaptureVisitor(Ast_ਗੰਢ* ਅਚਲ nodep, Ast_ਗੰਢ_Module* callerp, AstClass* ਅਚਲ targetp,
                            ਤਾਰਕਿਕ staticContext = ਝੂਠ)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterate_And_ਅਗਲਾ_Null(nodep);
    }

    // PUBLIC METHODS

    AstArg* get_ਦਲੀਲਾਂ() ਅਚਲ { ਵਾਪਸ m_argsp; }
    AstVar* getThisp() ਅਚਲ { ਵਾਪਸ m_thisp; }

    ਖਾਲੀ ਜੋੜੋ_Function_Arguments(Ast_ਗੰਢ_FTask* funcp) ਅਚਲ {
        ਲਈ (AstArg* argp = get_ਦਲੀਲਾਂ(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ਜੇ (Ast_ਗੰਢ_Var_Ref* varrefp = VN_CAST(argp->exprp(), ਗੰਢ_Var_Ref)) {
                ਜੇ ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(ਖਾਲੀ_ਸੂਚਕ);
                }
                funcp->ਜੋੜੋ_Stmtsp(getVar(varrefp->varp()));
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->ਜੋੜੋ_Stmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

ਵਰਗ RandomizeVisitor final : ਜਨਤਕ VNVisitor {
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
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_inuser4;

    // STATE
    V3_Unique_ਨਾਂ m_inline_Unique_ਨਾਂ;  // For generating unique function names
    V3_Unique_ਨਾਂ m_rand_Array_Unique_ਨਾਂ{"__Vrandarr"};  // Random array loop indices
    V3_Unique_ਨਾਂ m_ਬਹੁਲਕ_Unique_ਨਾਂ{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3_Unique_ਨਾਂ m_inlineUniqueStdName{"__VStdrand"};
    V3_Unique_ਨਾਂ m_unique_ਨਾਂ;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    std::unordered_map<Ast_ਗੰਢ_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    Ast_ਗੰਢ_Stmt* m_stmtp = ਖਾਲੀ_ਸੂਚਕ;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = ਖਾਲੀ_ਸੂਚਕ;  // Dynamic array type (for rand mode)
    ਪੂਰਨ m_rand_Case_ਅੰਕ = 0;  // Randcase number within a module for var naming
    ਪੂਰਨ m_dist_ਅੰਕ = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = ਖਾਲੀ_ਸੂਚਕ;  // Current constraint
    std::ਸੈੱਟ<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertion_ਸੈੱਟ<AstVar*>> m_ਮਾਪ_Constrained_Arrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_static_Constraint_ਬਹੁਲਕ_Vars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_static_Rand_ਬਹੁਲਕ_Vars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<ਤਾਰਕਿਕ, ਤਾਰਕਿਕ>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ਸਥਿਰ ਤਾਰਕਿਕ is_Similar_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* with_ਵਿਅੰਜਕ, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* arg_ਵਿਅੰਜਕ) {
        // VarRef: compare variable pointers
        ਜੇ (VN_IS(arg_ਵਿਅੰਜਕ, VarRef) && VN_IS(with_ਵਿਅੰਜਕ, VarRef)) {
            ਵਾਪਸ VN_AS(with_ਵਿਅੰਜਕ, VarRef)->varp() == VN_AS(arg_ਵਿਅੰਜਕ, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        ਜੇ (VN_IS(arg_ਵਿਅੰਜਕ, MemberSel) && VN_IS(with_ਵਿਅੰਜਕ, MemberSel)) {
            ਅਚਲ AstMemberSel* ਅਚਲ withMSp = VN_AS(with_ਵਿਅੰਜਕ, MemberSel);
            ਅਚਲ AstMemberSel* ਅਚਲ argMSp = VN_AS(arg_ਵਿਅੰਜਕ, MemberSel);
            ਜੇ (withMSp->varp() != argMSp->varp()) ਵਾਪਸ ਝੂਠ;
            // Recursively compare the base object expression
            ਵਾਪਸ is_Similar_ਗੰਢ(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        ਜੇ (VN_IS(arg_ਵਿਅੰਜਕ, ArraySel) && VN_IS(with_ਵਿਅੰਜਕ, ArraySel)) {
            ਅਚਲ AstArraySel* ਅਚਲ withASp = VN_AS(with_ਵਿਅੰਜਕ, ArraySel);
            ਅਚਲ AstArraySel* ਅਚਲ argASp = VN_AS(arg_ਵਿਅੰਜਕ, ArraySel);
            // Before checking Sel type index, check if both are Const
            ਜੇ (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ਅਚਲ AstConst* ਅਚਲ withIdxp = VN_AS(withASp->bitp(), Const);
                ਅਚਲ AstConst* ਅਚਲ argIdxp = VN_AS(argASp->bitp(), Const);
                ਵਾਪਸ is_Similar_ਗੰਢ(withASp->fromp(), argASp->fromp())
                       && withIdxp->ਅੰਕ().isCaseEq(argIdxp->ਅੰਕ());
            }
            // Index must be Sel type, extract VarRef using fromp()
            ਜੇ (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) ਵਾਪਸ ਝੂਠ;
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            ਵਾਪਸ is_Similar_ਗੰਢ(withASp->fromp(), argASp->fromp())
                   && is_Similar_ਗੰਢ(withIdxp, argIdxp);
        }
        ਵਾਪਸ ਝੂਠ;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ਸਥਿਰ ਖਾਲੀ expandUniqueElementList(Ast_ਗੰਢ* itemsp) {
        Ast_ਗੰਢ* itemp = itemsp;
        ਜਦੋਂ (itemp) {
            Ast_ਗੰਢ* ਅਚਲ nextp = itemp->nextp();
            ਜੇ (ਅਚਲ AstConstraintIf* ਅਚਲ ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                ਜਾਰੀ;
            }
            ਜੇ (ਅਚਲ AstConstraintForeach* ਅਚਲ foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                ਜਾਰੀ;
            }
            AstConstraintUnique* ਅਚਲ uniquep = VN_CAST(itemp, ConstraintUnique);
            ਜੇ (!uniquep) {
                itemp = nextp;
                ਜਾਰੀ;
            }

            std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਵਿਅੰਜਕ_ਚੀਜ਼ਾਂ;
            ਤਾਰਕਿਕ hasArrayVarRef = ਝੂਠ;
            ਲਈ (Ast_ਗੰਢ* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = vrp->varp()->dtypep()->skipRefp();
                    ਜੇ (VN_IS(dtypep, ਗੰਢ_Array_DType) || VN_IS(dtypep, ਕਤਾਰ_DType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = ਸੱਚ;
                        ਜਾਰੀ;
                    }
                }
                ਵਿਅੰਜਕ_ਚੀਜ਼ਾਂ.ਧੱਕੋ_ਪਿੱਠ(VN_AS(rp, ਗੰਢ_ਵਿਅੰਜਕ));
            }

            ਜੇ (ਵਿਅੰਜਕ_ਚੀਜ਼ਾਂ.ਮਾਪ() >= 2) {
                File_ਸਤਰ* ਅਚਲ fl = uniquep->fileline();
                ਲਈ (ਮਾਪ_t i = 0; i < ਵਿਅੰਜਕ_ਚੀਜ਼ਾਂ.ਮਾਪ(); i++) {
                    ਲਈ (ਮਾਪ_t j = i + 1; j < ਵਿਅੰਜਕ_ਚੀਜ਼ਾਂ.ਮਾਪ(); j++) {
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = ਵਿਅੰਜਕ_ਚੀਜ਼ਾਂ[i]->clone_ਰੁੱਖ(ਝੂਠ);
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਵਿਅੰਜਕ_ਚੀਜ਼ਾਂ[j]->clone_ਰੁੱਖ(ਝੂਠ);
                        AstNeq* ਅਚਲ neqp = ਨਵਾਂ AstNeq{fl, lhsp, rhsp};
                        neqp->user1(ਸੱਚ);
                        Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ cexprp = ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, neqp};
                        uniquep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(cexprp);
                    }
                }
                ਜੇ (!hasArrayVarRef) {
                    uniquep->unlink_Fr_ਪਿੱਠ();
                    VL_DO_DANGLING(uniquep->delete_ਰੁੱਖ(), uniquep);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (ਵਿਅੰਜਕ_ਚੀਜ਼ਾਂ.ਮਾਪ() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlink_Fr_ਪਿੱਠ();
                VL_DO_DANGLING(uniquep->delete_ਰੁੱਖ(), uniquep);
            }
            itemp = nextp;
        }
    }
    ਖਾਲੀ createRandomGenerator(AstClass* ਅਚਲ classp) {
        ਜੇ (classp->user3p()) ਵਾਪਸ;
        ਜੇ (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            ਵਾਪਸ;
        }
        AstVar* ਅਚਲ genp = ਨਵਾਂ AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->ਜੋੜੋ_Membersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_ਗੰਢ_Module* ਅਚਲ modp) {
        ਸਵੈਚਾਲਿਤ it = m_stdMap.ਲੱਭੋ(modp);
        ਜੇ (it == m_stdMap.ਸਿਰਾ()) {
            AstVar* ਅਚਲ stdgenp
                = ਨਵਾਂ AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3_Error_ਕੋਡ::IMPURE, ਸੱਚ);
            modp->ਜੋੜੋ_Stmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            ਵਾਪਸ stdgenp;
        }
        ਵਾਪਸ it->ਦੂਜਾ;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ਅਚਲ classp) {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ name = "__Vprepare_constrained_arrays";
        AstTask* ਅਚਲ taskp = ਨਵਾਂ AstTask{classp->fileline(), name, ਖਾਲੀ_ਸੂਚਕ};
        taskp->class_ਵਿਧੀ(ਸੱਚ);
        classp->ਜੋੜੋ_Membersp(taskp);
        m_memberMap.ਪਾਓ(classp, taskp);
        ਵਾਪਸ taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ਅਚਲ classp) {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.ਲੱਭੋ_Member(classp, name), Task);
        ਜੇ (resizeTaskp) ਵਾਪਸ resizeTaskp;
        resizeTaskp = ਨਵਾਂ AstTask{classp->fileline(), name, ਖਾਲੀ_ਸੂਚਕ};
        resizeTaskp->class_ਵਿਧੀ(ਸੱਚ);
        resizeTaskp->isVirtual(ਸੱਚ);
        classp->ਜੋੜੋ_Membersp(resizeTaskp);
        m_memberMap.ਪਾਓ(classp, resizeTaskp);
        ਵਾਪਸ resizeTaskp;
    }
    AstVar* get_Create_Rand_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp) {
        ਜੇ (classp->user2p()) ਵਾਪਸ VN_AS(classp->user2p(), Var);
        ਜੇ (AstClassExtends* ਅਚਲ extendsp = classp->extendsp()) {
            ਵਾਪਸ get_Create_Rand_ਬਹੁਲਕ_Var(extendsp->classp());
        }
        AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Varp = create_ਬਹੁਲਕ_Var(classp, "__Vrandmode");
        classp->user2p(rand_ਬਹੁਲਕ_Varp);
        ਵਾਪਸ rand_ਬਹੁਲਕ_Varp;
    }
    AstVar* get_Create_Constraint_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp) {
        ਜੇ (classp->user4p()) ਵਾਪਸ VN_AS(classp->user4p(), Var);
        ਜੇ (AstClassExtends* ਅਚਲ extendsp = classp->extendsp()) {
            ਵਾਪਸ get_Create_Constraint_ਬਹੁਲਕ_Var(extendsp->classp());
        }
        AstVar* ਅਚਲ constraint_ਬਹੁਲਕ_Varp = create_ਬਹੁਲਕ_Var(classp, "__Vconstraintmode");
        classp->user4p(constraint_ਬਹੁਲਕ_Varp);
        ਵਾਪਸ constraint_ਬਹੁਲਕ_Varp;
    }
    ਸਥਿਰ AstVar* get_Constraint_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp) {
        ਜੇ (classp->user4p()) ਵਾਪਸ VN_AS(classp->user4p(), Var);
        ਜੇ (AstClassExtends* ਅਚਲ extendsp = classp->extendsp()) {
            ਵਾਪਸ get_Constraint_ਬਹੁਲਕ_Var(extendsp->classp());
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    AstVar* get_Create_Static_Constraint_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp) {
        ਸਵੈਚਾਲਿਤ it = m_static_Constraint_ਬਹੁਲਕ_Vars.ਲੱਭੋ(classp);
        ਜੇ (it != m_static_Constraint_ਬਹੁਲਕ_Vars.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;
        ਜੇ (AstClassExtends* ਅਚਲ extendsp = classp->extendsp()) {
            ਵਾਪਸ get_Create_Static_Constraint_ਬਹੁਲਕ_Var(extendsp->classp());
        }
        AstVar* ਅਚਲ static_ਬਹੁਲਕ_Varp = create_Static_ਬਹੁਲਕ_Var(classp, "__Vstaticconstraintmode");
        m_static_Constraint_ਬਹੁਲਕ_Vars[classp] = static_ਬਹੁਲਕ_Varp;
        ਵਾਪਸ static_ਬਹੁਲਕ_Varp;
    }
    AstVar* get_Static_Constraint_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp) {
        ਸਵੈਚਾਲਿਤ it = m_static_Constraint_ਬਹੁਲਕ_Vars.ਲੱਭੋ(classp);
        ਜੇ (it != m_static_Constraint_ਬਹੁਲਕ_Vars.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;
        ਜੇ (AstClassExtends* ਅਚਲ extendsp = classp->extendsp()) {
            ਵਾਪਸ get_Static_Constraint_ਬਹੁਲਕ_Var(extendsp->classp());
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    AstVar* get_Create_Static_Rand_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp) {
        ਜੇ (m_static_Rand_ਬਹੁਲਕ_Vars.ਗਿਣਤੀ_2(classp)) ਵਾਪਸ m_static_Rand_ਬਹੁਲਕ_Vars[classp];
        ਜੇ (AstClassExtends* ਅਚਲ extendsp = classp->extendsp()) {
            ਵਾਪਸ get_Create_Static_Rand_ਬਹੁਲਕ_Var(extendsp->classp());
        }
        AstVar* ਅਚਲ static_ਬਹੁਲਕ_Varp = create_Static_ਬਹੁਲਕ_Var(classp, "__Vstaticrandmode");
        m_static_Rand_ਬਹੁਲਕ_Vars[classp] = static_ਬਹੁਲਕ_Varp;
        ਵਾਪਸ static_ਬਹੁਲਕ_Varp;
    }
    AstVar* get_Static_Rand_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp) {
        ਜੇ (m_static_Rand_ਬਹੁਲਕ_Vars.ਗਿਣਤੀ_2(classp)) ਵਾਪਸ m_static_Rand_ਬਹੁਲਕ_Vars[classp];
        ਜੇ (AstClassExtends* ਅਚਲ extendsp = classp->extendsp()) {
            ਵਾਪਸ get_Static_Rand_ਬਹੁਲਕ_Var(extendsp->classp());
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    AstVar* create_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp, ਅਚਲ ਅੱਖਰ* ਅਚਲ name) {
        File_ਸਤਰ* ਅਚਲ fl = classp->fileline();
        ਜੇ (!m_dynarrayDtp) {
            m_dynarrayDtp = ਨਵਾਂ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ਅਚਲ ਬਹੁਲਕ_Varp = ਨਵਾਂ AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        ਬਹੁਲਕ_Varp->user2p(classp);
        classp->ਜੋੜੋ_Stmtsp(ਬਹੁਲਕ_Varp);
        ਵਾਪਸ ਬਹੁਲਕ_Varp;
    }
    AstVar* create_Static_ਬਹੁਲਕ_Var(AstClass* ਅਚਲ classp, ਅਚਲ ਅੱਖਰ* ਅਚਲ name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_ਸਤਰ* ਅਚਲ fl = classp->fileline();
        ਜੇ (!m_dynarrayDtp) {
            m_dynarrayDtp = ਨਵਾਂ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(m_dynarrayDtp);
        }
        AstVar* ਅਚਲ ਬਹੁਲਕ_Varp = ਨਵਾਂ AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        ਬਹੁਲਕ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        ਬਹੁਲਕ_Varp->user2p(classp);
        classp->ਜੋੜੋ_Stmtsp(ਬਹੁਲਕ_Varp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.ਪਾਓ(classp, ਬਹੁਲਕ_Varp);
        ਵਾਪਸ ਬਹੁਲਕ_Varp;
    }
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_ਸੈੱਟ_Rand_ਬਹੁਲਕ(Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp, AstVar* ਅਚਲ genp,
                               AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Varp) {
        File_ਸਤਰ* ਅਚਲ fl = ftaskp->fileline();
        AstCMethodHard* ਅਚਲ ਸੈੱਟ_Rand_Modep = ਨਵਾਂ AstCMethodHard{
            fl, ਨਵਾਂ AstVarRef{fl, VN_AS(genp->user2p(), ਗੰਢ_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_ਸੈੱਟ_RANDMODE,
            ਨਵਾਂ AstVarRef{fl, VN_AS(rand_ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module), rand_ਬਹੁਲਕ_Varp,
                          VAccess::READ}};
        ਸੈੱਟ_Rand_Modep->dtype_ਸੈੱਟ_Void();
        ftaskp->ਜੋੜੋ_Stmtsp(ਸੈੱਟ_Rand_Modep->makeStmt());
    }
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_ਸੈੱਟ_Static_Rand_ਬਹੁਲਕ(Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp, AstVar* ਅਚਲ genp,
                                     AstVar* ਅਚਲ static_Rand_ਬਹੁਲਕ_Varp) {
        File_ਸਤਰ* ਅਚਲ fl = ftaskp->fileline();
        AstCMethodHard* ਅਚਲ setp = ਨਵਾਂ AstCMethodHard{
            fl, ਨਵਾਂ AstVarRef{fl, VN_AS(genp->user2p(), ਗੰਢ_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_ਸੈੱਟ_STATIC_RANDMODE,
            ਨਵਾਂ AstVarRef{fl, VN_AS(static_Rand_ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module), static_Rand_ਬਹੁਲਕ_Varp,
                          VAccess::READ}};
        setp->dtype_ਸੈੱਟ_Void();
        ftaskp->ਜੋੜੋ_Stmtsp(setp->makeStmt());
    }
    ਖਾਲੀ createRandomizeClassVars(AstNetlist* ਅਚਲ netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> ਜੜ੍ਹ_Static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ;
        netlistp->foreach([ਇਹ, &ਜੜ੍ਹ_Static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ](AstClass* ਅਚਲ classp) {
            ਤਾਰਕਿਕ hasConstraints = ਝੂਠ;
            uint32_t rand_ਬਹੁਲਕ_ਗਿਣਤੀ = 0;
            uint32_t static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ = 0;
            uint32_t constraint_ਬਹੁਲਕ_ਗਿਣਤੀ = 0;
            uint32_t static_Constraint_ਬਹੁਲਕ_ਗਿਣਤੀ = 0;
            classp->foreachMember([&](AstClass*, Ast_ਗੰਢ* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                ਜੇ (AstConstraint* ਅਚਲ constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = ਸੱਚ;
                    Randomize_ਬਹੁਲਕ constraint_ਬਹੁਲਕ = {.asUQuad = memberp->user1()};
                    ਜੇ (!constraint_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) ਵਾਪਸ;
                    ਜੇ (constraint_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ == 0) {
                        // Use separate index counters for static vs non-static constraints
                        ਜੇ (constrp->isStatic()) {
                            constraint_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ = static_Constraint_ਬਹੁਲਕ_ਗਿਣਤੀ++;
                        } ਨਹੀਂਤਾਂ {
                            constraint_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ = constraint_ਬਹੁਲਕ_ਗਿਣਤੀ++;
                        }
                        memberp->user1(constraint_ਬਹੁਲਕ.asUQuad);
                    } ਨਹੀਂਤਾਂ {
                        ਜੇ (constrp->isStatic()) {
                            static_Constraint_ਬਹੁਲਕ_ਗਿਣਤੀ = constraint_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ + 1;
                        } ਨਹੀਂਤਾਂ {
                            constraint_ਬਹੁਲਕ_ਗਿਣਤੀ = constraint_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ + 1;
                        }
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(memberp, Var)) {
                    Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {.asUQuad = memberp->user1()};
                    ਜੇ (!rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) ਵਾਪਸ;
                    ਅਚਲ ਤਾਰਕਿਕ isStaticVar = varp->lifetime().isStatic();
                    ਜੇ (rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ == 0) {
                        ਜੇ (isStaticVar) {
                            rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ = static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ++;
                        } ਨਹੀਂਤਾਂ {
                            rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ = rand_ਬਹੁਲਕ_ਗਿਣਤੀ++;
                        }
                        memberp->user1(rand_ਬਹੁਲਕ.asUQuad);
                    } ਨਹੀਂਤਾਂ {
                        ਜੇ (isStaticVar) {
                            static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ = rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ + 1;
                        } ਨਹੀਂਤਾਂ {
                            rand_ਬਹੁਲਕ_ਗਿਣਤੀ = rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<ਖਾਲੀ(AstClass*)> ਲੱਭੋ_Sub_Obj_Rand_Modes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_ਗੰਢ* subMemberp) {
                        ਜੇ (AstVar* ਅਚਲ subVarp = VN_CAST(subMemberp, Var)) {
                            ਅਚਲ Randomize_ਬਹੁਲਕ rm = {.asUQuad = subVarp->user1()};
                            ਜੇ (!rm.ਵਰਤਦਾ_ਬਹੁਲਕ) ਵਾਪਸ;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            ਜੇ (subVarp->lifetime().isStatic()) ਵਾਪਸ;
                            ਅਚਲ uint32_t needed = rm.ਸੂਚਕਾਂਕ + 1;
                            ਜੇ (needed > rand_ਬਹੁਲਕ_ਗਿਣਤੀ) rand_ਬਹੁਲਕ_ਗਿਣਤੀ = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    ਜੇ (!memberVarp->globalConstrained()) ਵਾਪਸ;
                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = memberVarp->dtypep()->skipRefp();
                    ਅਚਲ AstClassRefDType* ਅਚਲ classRefp = VN_CAST(dtypep, ClassRefDType);
                    ਜੇ (!classRefp) ਵਾਪਸ;
                    ਲੱਭੋ_Sub_Obj_Rand_Modes(classRefp->classp());
                });
            }
            ਜੇ (hasConstraints) createRandomGenerator(classp);
            ਜੇ (rand_ਬਹੁਲਕ_ਗਿਣਤੀ > 0) {
                AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Varp = get_Create_Rand_ਬਹੁਲਕ_Var(classp);
                make_ਬਹੁਲਕ_Init(rand_ਬਹੁਲਕ_Varp, classp, rand_ਬਹੁਲਕ_ਗਿਣਤੀ);
            }
            ਜੇ (constraint_ਬਹੁਲਕ_ਗਿਣਤੀ > 0) {
                AstVar* ਅਚਲ constraint_ਬਹੁਲਕ_Varp = get_Create_Constraint_ਬਹੁਲਕ_Var(classp);
                make_ਬਹੁਲਕ_Init(constraint_ਬਹੁਲਕ_Varp, classp, constraint_ਬਹੁਲਕ_ਗਿਣਤੀ);
            }
            ਜੇ (static_Constraint_ਬਹੁਲਕ_ਗਿਣਤੀ > 0) {
                AstVar* ਅਚਲ static_Constraint_ਬਹੁਲਕ_Varp = get_Create_Static_Constraint_ਬਹੁਲਕ_Var(classp);
                make_Static_ਬਹੁਲਕ_Init(static_Constraint_ਬਹੁਲਕ_Varp, classp, static_Constraint_ਬਹੁਲਕ_ਗਿਣਤੀ);
            }
            ਜੇ (static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ > 0) {
                get_Create_Static_Rand_ਬਹੁਲਕ_Var(classp);
                AstClass* rootp = classp;
                ਜਦੋਂ (AstClassExtends* ਅਚਲ ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& ਥਾਂ_2 = ਜੜ੍ਹ_Static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ[rootp];
                ਜੇ (static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ > ਥਾਂ_2) ਥਾਂ_2 = static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ;
            }
        });
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& kv : ਜੜ੍ਹ_Static_Rand_ਬਹੁਲਕ_ਗਿਣਤੀ) emit_ਜੜ੍ਹ_Static_ਬਹੁਲਕ_Init(kv.ਪਹਿਲਾ, kv.ਦੂਜਾ);
    }
    ਖਾਲੀ emit_ਜੜ੍ਹ_Static_ਬਹੁਲਕ_Init(AstClass* ਅਚਲ rootp, ਅਚਲ uint32_t ਗਿਣਤੀ_2) {
        AstVar* ਅਚਲ static_Rand_ਬਹੁਲਕ_Varp = m_static_Rand_ਬਹੁਲਕ_Vars[rootp];
        UASSERT_OBJ(static_Rand_ਬਹੁਲਕ_Varp, rootp, "Root must have a static rand-mode var");
        make_Static_ਬਹੁਲਕ_Init(static_Rand_ਬਹੁਲਕ_Varp, rootp, ਗਿਣਤੀ_2);
    }
    ਖਾਲੀ make_ਬਹੁਲਕ_Init(AstVar* ਬਹੁਲਕ_Varp, AstClass* classp, uint32_t ਬਹੁਲਕ_ਗਿਣਤੀ) {
        Ast_ਗੰਢ_Module* ਅਚਲ ਬਹੁਲਕ_Var_Modp = VN_AS(ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module);
        File_ਸਤਰ* ਅਚਲ fl = ਬਹੁਲਕ_Varp->fileline();
        AstVar* ਅਚਲ old_ਮਾਪ_Varp = ਨਵਾਂ AstVar{
            fl, VVarType::BLOCKTEMP, m_unique_ਨਾਂ.get("__VoldSize"), ਬਹੁਲਕ_Varp->ਲੱਭੋ_UInt32_DType()};
        old_ਮਾਪ_Varp->funcLocal(ਸੱਚ);
        old_ਮਾਪ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ਅਚਲ oldSizep
            = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, ਬਹੁਲਕ_Var_Modp, ਬਹੁਲਕ_Varp, VAccess::READ},
                                 VCMethod::DYN_ਮਾਪ, ਖਾਲੀ_ਸੂਚਕ};
        oldSizep->dtype_ਸੈੱਟ_UInt32();
        AstCMethodHard* ਅਚਲ dynarrayNewp
            = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, ਬਹੁਲਕ_Var_Modp, ਬਹੁਲਕ_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, ਨਵਾਂ AstConst{fl, ਬਹੁਲਕ_ਗਿਣਤੀ}};
        dynarrayNewp->dtype_ਸੈੱਟ_Void();
        Ast_ਗੰਢ_FTask* ਅਚਲ ctorNewp = VN_AS(m_memberMap.ਲੱਭੋ_Member(classp, "new"), ਗੰਢ_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_ਗੰਢ* ਅਚਲ initFirstp = old_ਮਾਪ_Varp;
        initFirstp->ਜੋੜੋ_ਅਗਲਾ(
            ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, old_ਮਾਪ_Varp, VAccess::WRITE}, oldSizep});
        initFirstp->ਜੋੜੋ_ਅਗਲਾ(dynarrayNewp->makeStmt());
        initFirstp->ਜੋੜੋ_ਅਗਲਾ(make_ਬਹੁਲਕ_ਸੈੱਟ_Loop(
            fl, ਨਵਾਂ AstVarRef{fl, ਬਹੁਲਕ_Var_Modp, ਬਹੁਲਕ_Varp, VAccess::WRITE}, ਨਵਾਂ AstConst{fl, 1},
            ਸੱਚ, ਨਵਾਂ AstVarRef{fl, old_ਮਾਪ_Varp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        ਲਈ (Ast_ਗੰਢ* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (!VN_IS(stmtp, ਗੰਢ_Stmt)) ਜਾਰੀ;  // Skip var declarations
            ਜੇ (ਅਚਲ Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ sep = VN_CAST(stmtp, Stmt_ਵਿਅੰਜਕ)) {
                ਜੇ (VN_IS(sep->exprp(), New)) ਜਾਰੀ;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(initFirstp);
            ਵਾਪਸ;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->ਜੋੜੋ_Stmtsp(initFirstp);
    }
    ਖਾਲੀ make_Static_ਬਹੁਲਕ_Init(AstVar* ਬਹੁਲਕ_Varp, AstClass* classp, uint32_t ਬਹੁਲਕ_ਗਿਣਤੀ) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_ਗੰਢ_Module* ਅਚਲ ਬਹੁਲਕ_Var_Modp = VN_AS(ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module);
        File_ਸਤਰ* ਅਚਲ fl = ਬਹੁਲਕ_Varp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ਅਚਲ sizep
            = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, ਬਹੁਲਕ_Var_Modp, ਬਹੁਲਕ_Varp, VAccess::READ},
                                 VCMethod::DYN_ਮਾਪ, ਖਾਲੀ_ਸੂਚਕ};
        sizep->dtype_ਸੈੱਟ_UInt32();
        AstEq* ਅਚਲ condp = ਨਵਾਂ AstEq{fl, sizep, ਨਵਾਂ AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ਅਚਲ dynarrayNewp
            = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, ਬਹੁਲਕ_Var_Modp, ਬਹੁਲਕ_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, ਨਵਾਂ AstConst{fl, ਬਹੁਲਕ_ਗਿਣਤੀ}};
        dynarrayNewp->dtype_ਸੈੱਟ_Void();
        Ast_ਗੰਢ* ਅਚਲ thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->ਜੋੜੋ_ਅਗਲਾ(
            make_ਬਹੁਲਕ_ਸੈੱਟ_Loop(fl, ਨਵਾਂ AstVarRef{fl, ਬਹੁਲਕ_Var_Modp, ਬਹੁਲਕ_Varp, VAccess::WRITE},
                            ਨਵਾਂ AstConst{fl, 1}, ਸੱਚ));

        // Build the if statement
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_ਗੰਢ_FTask* ਅਚਲ newp = VN_AS(m_memberMap.ਲੱਭੋ_Member(classp, "new"), ਗੰਢ_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->ਜੋੜੋ_Stmtsp(ifp);
    }
    Ast_ਗੰਢ* make_ਬਹੁਲਕ_ਸੈੱਟ_Loop(File_ਸਤਰ* ਅਚਲ fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp,
                             ਤਾਰਕਿਕ ਵਿੱਚ_Task, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ startp = ਖਾਲੀ_ਸੂਚਕ) {
        AstVar* ਅਚਲ iterVarp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->ਲੱਭੋ_UInt32_DType()};
        iterVarp->funcLocal(ਵਿੱਚ_Task);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ਅਚਲ sizep = ਨਵਾਂ AstCMethodHard{fl, lhsp, VCMethod::DYN_ਮਾਪ, ਖਾਲੀ_ਸੂਚਕ};
        sizep->dtype_ਸੈੱਟ_UInt32();
        AstCMethodHard* ਅਚਲ setp
            = ਨਵਾਂ AstCMethodHard{fl, lhsp->clone_ਰੁੱਖ(ਝੂਠ), VCMethod::ARRAY_AT_WRITE,
                                 ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtype_ਸੈੱਟ_UInt32();
        Ast_ਗੰਢ* ਅਚਲ stmtsp = iterVarp;
        stmtsp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : ਨਵਾਂ AstConst{fl, 0}});

        AstLoop* ਅਚਲ loopp = ਨਵਾਂ AstLoop{fl};
        stmtsp->ਜੋੜੋ_ਅਗਲਾ(loopp);
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstLoopTest{
            fl, loopp, ਨਵਾਂ AstLt{fl, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, setp, rhsp});
        loopp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            fl, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::WRITE},
            ਨਵਾਂ Ast_ਜੋੜੋ{fl, ਨਵਾਂ AstConst{fl, 1}, ਨਵਾਂ AstVarRef{fl, iterVarp, VAccess::READ}}});
        ਵਾਪਸ ਨਵਾਂ AstBegin{fl, m_unique_ਨਾਂ.get("__VrandBlock"), stmtsp, ਸੱਚ};
    }
    ਖਾਲੀ ਜੋੜੋ_ਆਰਜ਼ੀ_Varsp(AstVar* varsp) {
        ਲਈ (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ਜੇ (m_ftaskp) {
                varp->funcLocal(ਸੱਚ);
            } ਨਹੀਂਤਾਂ {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        ਜੇ (!m_ftaskp) {
            m_modp->ਜੋੜੋ_Stmtsp(varsp);
        } ਨਹੀਂਤਾਂ {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_ਗੰਢ_Stmt* wrap_If_Rand_ਬਹੁਲਕ(AstClass* classp, AstVar* ਅਚਲ varp, Ast_ਗੰਢ_Stmt* stmtp) {
        ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = varp->user1()};
        AstVar* ਅਚਲ ਬਹੁਲਕ_Varp = varp->lifetime().isStatic() ? get_Static_Rand_ਬਹੁਲਕ_Var(classp)
                                                             : get_Rand_ਬਹੁਲਕ_Var_From_Class(classp);
        ਵਾਪਸ VN_AS(wrap_If_ਬਹੁਲਕ(rmode, ਬਹੁਲਕ_Varp, stmtp), ਗੰਢ_Stmt);
    }
    Ast_ਗੰਢ* wrap_If_Constraint_ਬਹੁਲਕ(AstClass* classp, AstConstraint* ਅਚਲ constrp, Ast_ਗੰਢ* stmtp) {
        ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = constrp->user1()};
        AstVar* ਅਚਲ ਬਹੁਲਕ_Varp = constrp->isStatic() ? get_Static_Constraint_ਬਹੁਲਕ_Var(classp)
                                                     : get_Constraint_ਬਹੁਲਕ_Var(classp);
        ਵਾਪਸ wrap_If_ਬਹੁਲਕ(rmode, ਬਹੁਲਕ_Varp, stmtp);
    }
    ਸਥਿਰ Ast_ਗੰਢ* wrap_If_ਬਹੁਲਕ(ਅਚਲ Randomize_ਬਹੁਲਕ ਬਹੁਲਕ, AstVar* ਬਹੁਲਕ_Varp, Ast_ਗੰਢ* stmtp) {
        File_ਸਤਰ* ਅਚਲ fl = stmtp->fileline();
        ਜੇ (ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_AS(ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module);
            AstCMethodHard* ਅਚਲ atp
                = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, modp, ਬਹੁਲਕ_Varp, VAccess::READ},
                                     VCMethod::ARRAY_AT, ਨਵਾਂ AstConst{fl, ਬਹੁਲਕ.ਸੂਚਕਾਂਕ}};
            atp->dtype_ਸੈੱਟ_UInt32();
            ਵਾਪਸ ਨਵਾਂ AstIf{fl, atp, stmtp};
        }
        ਵਾਪਸ stmtp;
    }
    AstVarRef* enum_ਮੁੱਲ_Tab_Refp(AstEnumDType* ਅਚਲ nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        ਜੇ (nodep->user2p()) ਵਾਪਸ VN_AS(nodep->user2p(), VarRef)->clone_ਰੁੱਖ(ਝੂਠ);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_ਗੰਢ_Array_DType* ਅਚਲ vardtypep = ਨਵਾਂ AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            ਨਵਾਂ AstRange{nodep->fileline(), static_cast<ਪੂਰਨ>(nodep->ਵਸਤੂ_ਗਿਣਤੀ()) - 1, 0}};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(vardtypep);
        AstConsArray* ਅਚਲ consp = ਨਵਾਂ AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t ਸੂਚਕਾਂਕ = 0;
        ਲਈ (Ast_Enum_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
            AstConst* ਅਚਲ vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(ਸੂਚਕਾਂਕ++, vconstp->clone_ਰੁੱਖ(ਝੂਠ));
        }
        // Share identical tables via the constant pool
        AstVarRef* ਅਚਲ refp = V3ConstPool::ਲੱਭੋ(consp);
        VL_DO_DANGLING(consp->delete_ਰੁੱਖ(), consp);  // V3ConstPool::find clones it
        ਧੱਕੋ_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        ਵਾਪਸ refp->clone_ਰੁੱਖ(ਝੂਠ);
    }

    AstCDType* ਲੱਭੋ_Vl_Rand_CDType(File_ਸਤਰ* ਅਚਲ fl, uint64_t ਚੀਜ਼ਾਂ) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ਅਚਲ std::string type = AstCDType::type_ਮੰਜ਼ਿਲ_Hold(V3_ਸੰਖਿਆ::log2bQuad(ਚੀਜ਼ਾਂ) + 1);
        ਅਚਲ std::string name = "VlRandC<" + type + ", " + cvt_ਮੰਜ਼ਿਲ_Str(ਚੀਜ਼ਾਂ) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_randcDtypes.emplace(name, ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (pair.ਦੂਜਾ) {
            AstCDType* ਅਚਲ newp = ਨਵਾਂ AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newp);
            pair.ਪਹਿਲਾ->ਦੂਜਾ = newp;
        }
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    AstVar* newRandcVarsp(AstVar* ਅਚਲ varp, AstClass* ਅਚਲ classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        ਜੇ (!varp->isRandC()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        uint64_t ਚੀਜ਼ਾਂ = 0;
        ਜੇ (AstEnumDType* ਅਚਲ enumDtp = VN_CAST(varp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType)) {
            ਚੀਜ਼ਾਂ = static_cast<uint64_t>(enumDtp->ਵਸਤੂ_ਗਿਣਤੀ());
        } ਨਹੀਂਤਾਂ ਜੇ (AstBasicDType* ਅਚਲ basicp = varp->dtypep()->skipRefp()->basicp()) {
            ਜੇ (basicp->ਚੌੜਾਈ() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                ਜੇ (!V3Error::error_ਗਿਣਤੀ()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->ਚੌੜਾਈ() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
            ਚੀਜ਼ਾਂ = 1ULL << basicp->ਚੌੜਾਈ();
        } ਨਹੀਂਤਾਂ ਜੇ (AstStructDType* ਅਚਲ dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        } ਨਹੀਂਤਾਂ {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_ਗਿਣਤੀ(), varp, "Unexpected randc variable dtype");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        AstCDType* ਅਚਲ newdtp = ਲੱਭੋ_Vl_Rand_CDType(varp->fileline(), ਚੀਜ਼ਾਂ);
        AstVar* ਅਚਲ newp
            = ਨਵਾਂ AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
        classp->ਜੋੜੋ_Stmtsp(newp);
        UINFO(9, "created " << varp);
        ਵਾਪਸ newp;
    }
    Ast_ਗੰਢ_Stmt* createArrayForeachLoop(File_ਸਤਰ* ਅਚਲ fl, Ast_ਗੰਢ_DType* ਅਚਲ dtypep,
                                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstVar* ਅਚਲ outputVarp) {
        Ast_ਗੰਢ_DType* ਆਰਜ਼ੀ_DTypep = dtypep;
        AstVar* randLoopIndxp = ਖਾਲੀ_ਸੂਚਕ;
        ਸਵੈਚਾਲਿਤ create_Loop_ਸੂਚਕਾਂਕ = [&](Ast_ਗੰਢ_DType* ਆਰਜ਼ੀ_DTypep) {
            ਜੇ (VN_IS(ਆਰਜ਼ੀ_DTypep, AssocArrayDType)) {
                ਵਾਪਸ ਨਵਾਂ AstVar{fl, VVarType::VAR, m_rand_Array_Unique_ਨਾਂ.get(""),
                                  VN_AS(ਆਰਜ਼ੀ_DTypep, AssocArrayDType)->ਕੁੰਜੀ_DTypep()};
            }
            ਵਾਪਸ ਨਵਾਂ AstVar{fl, VVarType::VAR, m_rand_Array_Unique_ਨਾਂ.get(""),
                              dtypep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32)};
        };
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਆਰਜ਼ੀ_Elementp = ਖਾਲੀ_ਸੂਚਕ;
        ਜਦੋਂ (ਆਰਜ਼ੀ_DTypep->isNonPackedArray()) {
            AstVar* ਅਚਲ newRandLoopIndxp = create_Loop_ਸੂਚਕਾਂਕ(ਆਰਜ਼ੀ_DTypep);
            randLoopIndxp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(randLoopIndxp, newRandLoopIndxp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਆਰਜ਼ੀ_Exprp = ਆਰਜ਼ੀ_Elementp ? ਆਰਜ਼ੀ_Elementp : exprp;
            AstVarRef* ਅਚਲ ਆਰਜ਼ੀ_Refp = ਨਵਾਂ AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            ਜੇ (VN_IS(ਆਰਜ਼ੀ_DTypep, DynArrayDType)) {
                ਆਰਜ਼ੀ_Elementp
                    = ਨਵਾਂ AstCMethodHard{fl, ਆਰਜ਼ੀ_Exprp, VCMethod::ARRAY_AT_WRITE, ਆਰਜ਼ੀ_Refp};
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਆਰਜ਼ੀ_DTypep, UnpackArrayDType)) {
                Ast_ਗੰਢ_Array_DType* ਅਚਲ aryDTypep = VN_CAST(ਆਰਜ਼ੀ_DTypep, ਗੰਢ_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                ਆਰਜ਼ੀ_Elementp = ਨਵਾਂ AstArraySel{
                    fl, ਆਰਜ਼ੀ_Exprp,
                    ਨਵਾਂ AstSel{
                        fl,
                        ਨਵਾਂ AstSub{fl, ਆਰਜ਼ੀ_Refp,
                                   ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        ਨਵਾਂ AstConst{fl, 0}, V3_ਸੰਖਿਆ::log2b(aryDTypep->hi()) + 1}};
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਆਰਜ਼ੀ_DTypep, AssocArrayDType)) {
                ਆਰਜ਼ੀ_Elementp = ਨਵਾਂ AstAssocSel{fl, ਆਰਜ਼ੀ_Exprp, ਆਰਜ਼ੀ_Refp};
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਆਰਜ਼ੀ_DTypep, ਕਤਾਰ_DType)) {
                ਆਰਜ਼ੀ_Elementp
                    = ਨਵਾਂ AstCMethodHard{fl, ਆਰਜ਼ੀ_Exprp, VCMethod::DYN_AT_WRITE_APPEND, ਆਰਜ਼ੀ_Refp};
            }
            ਆਰਜ਼ੀ_Elementp->dtypep(ਆਰਜ਼ੀ_DTypep->subDTypep());
            ਆਰਜ਼ੀ_DTypep = ਆਰਜ਼ੀ_DTypep->virtRefDTypep();
        }

        Ast_Foreach_ਸਿਰਲੇਖ* ਅਚਲ headerp
            = ਨਵਾਂ Ast_Foreach_ਸਿਰਲੇਖ{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), randLoopIndxp};
        Ast_ਗੰਢ_Stmt* ਅਚਲ randStmtsp = newRandStmtsp(fl, ਆਰਜ਼ੀ_Elementp, ਖਾਲੀ_ਸੂਚਕ, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        ਜੇ (!ਆਰਜ਼ੀ_Elementp->backp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਆਰਜ਼ੀ_Elementp), ਆਰਜ਼ੀ_Elementp);
        ਵਾਪਸ ਨਵਾਂ AstForeach{fl, headerp, randStmtsp};
    }
    Ast_ਗੰਢ_Stmt* newRandStmtsp(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstVar* randcVarp,
                               AstVar* ਅਚਲ outputVarp, ਪੂਰਨ ਵਿੱਥ = 0,
                               AstMemberDType* memberp = ਖਾਲੀ_ਸੂਚਕ) {
        Ast_ਗੰਢ_DType* ਅਚਲ memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_ਗੰਢ_Stmt* stmtsp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (structDtp->packed()) ਵਿੱਥ += memberp ? memberp->lsb() : 0;
            ਲਈ (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_ਗੰਢ_Stmt* randp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_ਰੁੱਖ(ਝੂਠ) : exprp, ਖਾਲੀ_ਸੂਚਕ,
                                          outputVarp, ਵਿੱਥ, smemberp);
                } ਨਹੀਂਤਾਂ ਜੇ (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = ਨਵਾਂ AstStructSel{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), smemberp->name()};
                    structSelp->dtypep(smemberp->ਬੱਚਾ_DTypep());
                    ਜੇ (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, ਖਾਲੀ_ਸੂਚਕ, outputVarp);
                }
                stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(stmtsp, randp);
            }
            ਵਾਪਸ stmtsp;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            ਜੇ (!unionDtp->packed()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            AstMemberDType* ਅਚਲ ਪਹਿਲਾ_Memberp = unionDtp->membersp();
            ਵਾਪਸ newRandStmtsp(fl, exprp, ਖਾਲੀ_ਸੂਚਕ, outputVarp, ਵਿੱਥ, ਪਹਿਲਾ_Memberp);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ਅਚਲ memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            Ast_ਵਿਧੀ_Call* ਅਚਲ callp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            Ast_ਸੌਂਪੋ* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                fl, ਨਵਾਂ AstVarRef{fl, outputVarp, VAccess::WRITE},
                ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            ਵਾਪਸ ਨਵਾਂ AstIf{
                fl, ਨਵਾਂ AstNeq{fl, exprp->clone_ਰੁੱਖ(ਝੂਠ), ਨਵਾਂ AstConst{fl, AstConst::Null{}}},
                assignp};
        } ਨਹੀਂਤਾਂ ਜੇ (AstDynArrayDType* ਅਚਲ dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            ਵਾਪਸ createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਕਤਾਰ_DType* ਅਚਲ ਕਤਾਰ_Dtp = VN_CAST(memberDtp, ਕਤਾਰ_DType)) {
            ਵਾਪਸ createArrayForeachLoop(fl, ਕਤਾਰ_Dtp, exprp, outputVarp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstUnpackArrayDType* ਅਚਲ unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            ਵਾਪਸ createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } ਨਹੀਂਤਾਂ ਜੇ (AstAssocArrayDType* ਅਚਲ assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            ਵਾਪਸ createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* valp;
            ਜੇ (AstEnumDType* ਅਚਲ enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ਅਚਲ tabRefp = enum_ਮੁੱਲ_Tab_Refp(enumDtp);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ randp
                    = new_Rand_ਮੁੱਲ(fl, randcVarp, exprp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32));
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ moddivp = ਨਵਾਂ AstModDiv{
                    fl, randp, ਨਵਾਂ AstConst{fl, static_cast<uint32_t>(enumDtp->ਵਸਤੂ_ਗਿਣਤੀ())}};
                moddivp->dtypep(enumDtp);
                valp = ਨਵਾਂ AstArraySel{fl, tabRefp, moddivp};
            } ਨਹੀਂਤਾਂ {
                valp
                    = new_Rand_ਮੁੱਲ(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            Ast_ਸੌਂਪੋ* assignp
                = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl,
                                ਨਵਾਂ AstSel{fl, exprp, ਵਿੱਥ + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->ਚੌੜਾਈ() : exprp->ਚੌੜਾਈ()},
                                valp};
            AstVar* varp = ਖਾਲੀ_ਸੂਚਕ;
            exprp->exists([&](ਅਚਲ AstVarRef* varrefp) {
                ਜੇ (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                ਵਾਪਸ varp != ਖਾਲੀ_ਸੂਚਕ;
            });
            ਵਾਪਸ wrap_If_Rand_ਬਹੁਲਕ(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_Rand_ਮੁੱਲ(File_ਸਤਰ* ਅਚਲ fl, AstVar* ਅਚਲ randcVarp,
                              Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        ਜੇ (randcVarp) {
            AstCExpr* ਅਚਲ cexprp = ਨਵਾਂ AstCExpr{fl};
            cexprp->ਜੋੜੋ(ਨਵਾਂ AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->ਜੋੜੋ(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            ਵਾਪਸ cexprp;
        }

        ਵਾਪਸ ਨਵਾਂ AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* ਲੱਭੋ_Pre_Post_Task(AstClass* classp, ਅਚਲ string& name) {
        ਲਈ (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ) {
            ਜੇ (AstTask* ਅਚਲ taskp = VN_CAST(m_memberMap.ਲੱਭੋ_Member(cp, name), Task)) {
                ਵਾਪਸ taskp;
            }
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਜੋੜੋ_Pre_Post_Call(AstClass* ਅਚਲ classp, Ast_ਗੰਢ_FTask* ਅਚਲ funcp, ਅਚਲ string& name) {
        ਜੇ (AstTask* ਅਚਲ userFuncp = ਲੱਭੋ_Pre_Post_Task(classp, name)) {
            AstTaskRef* ਅਚਲ callp = ਨਵਾਂ AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* ਅਚਲ classp, ਅਚਲ string& which) {
        ਅਚਲ string name = "__V" + which;
        ਜੇ (AstTask* ਅਚਲ existingp = VN_CAST(m_memberMap.ਲੱਭੋ_Member(classp, name), Task)) {
            ਵਾਪਸ existingp;
        }
        AstTask* ਅਚਲ taskp = ਨਵਾਂ AstTask{classp->fileline(), name, ਖਾਲੀ_ਸੂਚਕ};
        taskp->class_ਵਿਧੀ(ਸੱਚ);
        taskp->isVirtual(classp->isExtended());
        classp->ਜੋੜੋ_Membersp(taskp);
        m_memberMap.ਪਾਓ(classp, taskp);
        ਜੋੜੋ_Pre_Post_Call(classp, taskp, which);
        ਵਾਪਸ taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<ਤਾਰਕਿਕ, ਤਾਰਕਿਕ> buildPrePostVirtualWrappers(AstClass* ਅਚਲ classp) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ cachedIt = m_prePostWrap.ਲੱਭੋ(classp);
        ਜੇ (cachedIt != m_prePostWrap.ਸਿਰਾ()) ਵਾਪਸ cachedIt->ਦੂਜਾ;
        std::ਸਦਿਸ਼<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            ਜੇ (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.ਧੱਕੋ_ਪਿੱਠ(subp);
        });
        ਤਾਰਕਿਕ hasPre = ਝੂਠ;
        ਤਾਰਕਿਕ hasPost = ਝੂਠ;
        ਲਈ (AstClass* ਅਚਲ cp : hierp) {
            ਜੇ (ਲੱਭੋ_Pre_Post_Task(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = ਸੱਚ;
            }
            ਜੇ (ਲੱਭੋ_Pre_Post_Task(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = ਸੱਚ;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        ਜੇ (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        ਜੇ (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ਅਚਲ std::pair<ਤਾਰਕਿਕ, ਤਾਰਕਿਕ> ਨਤੀਜਾ{hasPre, hasPost};
        m_prePostWrap.emplace(classp, ਨਤੀਜਾ);
        ਵਾਪਸ ਨਤੀਜਾ;
    }
    ਖਾਲੀ ਜੋੜੋ_Virtual_Pre_Post_Call(AstFunc* ਅਚਲ randomizeFuncp, AstClass* ਅਚਲ classp,
                               ਅਚਲ string& which) {
        File_ਸਤਰ* ਅਚਲ fl = classp->fileline();
        AstTask* ਅਚਲ wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ਅਚਲ refDTypep = ਨਵਾਂ AstClassRefDType{fl, classp, ਖਾਲੀ_ਸੂਚਕ};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(refDTypep);
        Ast_ਵਿਧੀ_Call* ਅਚਲ callp
            = ਨਵਾਂ Ast_ਵਿਧੀ_Call{fl, ਨਵਾਂ AstThisRef{fl, refDTypep}, wrapperp->name(), ਖਾਲੀ_ਸੂਚਕ};
        callp->taskp(wrapperp);
        callp->dtype_ਸੈੱਟ_Void();
        randomizeFuncp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    ਤਾਰਕਿਕ classHasRandClassMembers(AstClass* classp) {
        ਵਾਪਸ classp->existsMember([](ਅਚਲ AstClass*, ਅਚਲ AstVar* varp) {
            ਜੇ (!varp->rand().isRandomizable()) ਵਾਪਸ ਝੂਠ;
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep()->skipRefp();
            ਵਾਪਸ VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    ਤਾਰਕਿਕ classOwnsGlobalConstraint(ਅਚਲ AstClass* classp) ਅਚਲ {
        ਵਾਪਸ classp->existsMember([](ਅਚਲ AstClass*, ਅਚਲ AstConstraint* constrp) {
            ਤਾਰਕਿਕ owns = ਝੂਠ;
            constrp->foreach([&](ਅਚਲ AstMemberSel* memberSelp) {
                ਅਚਲ Ast_ਗੰਢ* ਅਚਲ rootp
                    = const_cast<AstMemberSel*>(memberSelp)->ਅਧਾਰ_Fromp(ਸੱਚ);
                ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ਅਚਲ Ast_ਗੰਢ_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    ਜਦੋਂ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    ਜੇ (VN_IS(dtypep, ClassRefDType)) owns = ਸੱਚ;
                }
            });
            ਵਾਪਸ owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ਅਚਲ string& suffix) {
        ਅਚਲ string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.ਲੱਭੋ_Member(classp, name), Task);
        ਜੇ (taskp) ਵਾਪਸ taskp;
        taskp = ਨਵਾਂ AstTask{classp->fileline(), name, ਖਾਲੀ_ਸੂਚਕ};
        taskp->class_ਵਿਧੀ(ਸੱਚ);
        classp->ਜੋੜੋ_Membersp(taskp);
        m_memberMap.ਪਾਓ(classp, taskp);
        ਵਾਪਸ taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    ਖਾਲੀ populateNestedCallbackTask(AstTask* ਅਚਲ callbackTaskp, AstClass* ਅਚਲ classp,
                                    ਅਚਲ string& cbName) {
        File_ਸਤਰ* ਅਚਲ fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            ਜੇ (!memberVarp->rand().isRandomizable()) ਵਾਪਸ;
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = memberVarp->dtypep()->skipRefp();
            ਅਚਲ AstClassRefDType* ਅਚਲ classRefp = VN_CAST(dtypep, ClassRefDType);
            ਜੇ (!classRefp) ਵਾਪਸ;
            AstClass* ਅਚਲ memberClassp = classRefp->classp();
            ਜੇ (memberClassp == classp) ਵਾਪਸ;  // Avoid self-reference

            // Force-visit member class if not yet processed
            ਜੇ (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_ਗੰਢ* stmtsp = ਖਾਲੀ_ਸੂਚਕ;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            ਜੇ (AstTask* ਅਚਲ userFuncp = ਲੱਭੋ_Pre_Post_Task(memberClassp, cbName)) {
                Ast_ਵਿਧੀ_Call* ਅਚਲ callp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
                    fl, ਨਵਾਂ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtype_ਸੈੱਟ_Void();
                stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            ਜੇ (classHasRandClassMembers(memberClassp)) {
                ਅਚਲ string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ਅਚਲ nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                Ast_ਵਿਧੀ_Call* ਅਚਲ recurseCallp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
                    fl, ਨਵਾਂ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtype_ਸੈੱਟ_Void();
                stmtsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(stmtsp, recurseCallp->makeStmt());
            }

            ਜੇ (!stmtsp) ਵਾਪਸ;

            // Wrap in null check
            AstIf* ਅਚਲ nullCheckp = ਨਵਾਂ AstIf{
                fl,
                ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           ਨਵਾਂ AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->ਜੋੜੋ_Stmtsp(wrap_If_Rand_ਬਹੁਲਕ(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* new_ਤਿਆਰੀ_Constraint_Task(AstClass* ਅਚਲ nodep, ਅਚਲ std::string& name) {
        AstTask* ਅਚਲ taskp = ਨਵਾਂ AstTask{nodep->fileline(), name + "_setup_constraint", ਖਾਲੀ_ਸੂਚਕ};
        taskp->class_ਵਿਧੀ(ਸੱਚ);
        nodep->ਜੋੜੋ_Membersp(taskp);
        ਵਾਪਸ taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ਅਚਲ nodep, ਅਚਲ std::string& name) {
        AstTask* ਅਚਲ taskp
            = ਨਵਾਂ AstTask{nodep->fileline(), name + "_resize_constrained_array", ਖਾਲੀ_ਸੂਚਕ};
        taskp->class_ਵਿਧੀ(ਸੱਚ);
        nodep->ਜੋੜੋ_Membersp(taskp);
        ਵਾਪਸ taskp;
    }
    Ast_ਗੰਢ_Stmt* implementConstraintsClear(File_ਸਤਰ* ਅਚਲ fileline, AstVar* ਅਚਲ genp) {
        AstCMethodHard* ਅਚਲ clearp = ਨਵਾਂ AstCMethodHard{
            fileline,
            ਨਵਾਂ AstVarRef{fileline, VN_AS(genp->user2p(), ਗੰਢ_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtype_ਸੈੱਟ_Void();
        ਵਾਪਸ clearp->makeStmt();
    }
    Ast_ਗੰਢ_Stmt* implementConstraintsClearAll(File_ਸਤਰ* ਅਚਲ fileline, AstVar* ਅਚਲ genp) {
        AstCMethodHard* ਅਚਲ clearp = ਨਵਾਂ AstCMethodHard{
            fileline,
            ਨਵਾਂ AstVarRef{fileline, VN_AS(genp->user2p(), ਗੰਢ_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtype_ਸੈੱਟ_Void();
        ਵਾਪਸ clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp) {
        ਜੇ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(exprp, MemberSel)) {
            ਵਾਪਸ memberSelp->varp();
        } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(exprp, VarRef)) {
            ਵਾਪਸ varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* make_ਬਹੁਲਕ_Var_Ref(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp, AstVar* ਅਚਲ ਬਹੁਲਕ_Varp,
                                ਅਚਲ VAccess access) {
        ਜੇ (ਬਹੁਲਕ_Varp->lifetime().isStatic()) {
            ਵਾਪਸ ਨਵਾਂ AstVarRef{exprp->fileline(), VN_AS(ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module),
                                 ਬਹੁਲਕ_Varp, access};
        }
        ਜੇ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ਅਚਲ newMemberSelp = ਨਵਾਂ AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_ਰੁੱਖ(ਝੂਠ), ਬਹੁਲਕ_Varp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            ਵਾਪਸ newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        ਵਾਪਸ ਨਵਾਂ AstVarRef{exprp->fileline(), VN_AS(ਬਹੁਲਕ_Varp->user2p(), Class), ਬਹੁਲਕ_Varp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* create_Or_Get_ਮਾਪ_Var(AstClass* ਅਚਲ classp, AstVar* ਅਚਲ arrVarp, File_ਸਤਰ* ਅਚਲ fl,
                               Ast_ਗੰਢ_DType* ਅਚਲ signed32DTypep, ਤਾਰਕਿਕ& wasCreated) {
        AstVar* ਮਾਪ_Varp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = ਝੂਠ;
        ਜੇ (!ਮਾਪ_Varp) {
            ਮਾਪ_Varp = ਨਵਾਂ AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->ਜੋੜੋ_Membersp(ਮਾਪ_Varp);
            m_memberMap.ਪਾਓ(classp, ਮਾਪ_Varp);
            ਮਾਪ_Varp->user2p(classp);
            arrVarp->user4p(ਮਾਪ_Varp);
            wasCreated = ਸੱਚ;
        }
        ਵਾਪਸ ਮਾਪ_Varp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_ਵਿਅੰਜਕ* create_ਮਾਪ_Gte_Zero_Constraint(File_ਸਤਰ* ਅਚਲ fl, AstVar* ਅਚਲ ਮਾਪ_Varp,
                                                   Ast_ਗੰਢ_ਵਿਅੰਜਕ* derefFromp = ਖਾਲੀ_ਸੂਚਕ) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮਾਪ_ਗੰਢ_Exprp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (derefFromp) {
            ਮਾਪ_ਗੰਢ_Exprp = buildMemberSelExprp(derefFromp, ਮਾਪ_Varp);
        } ਨਹੀਂਤਾਂ {
            ਮਾਪ_ਗੰਢ_Exprp = ਨਵਾਂ AstVarRef{fl, ਮਾਪ_Varp, VAccess::READ};
        }
        ਮਾਪ_ਗੰਢ_Exprp->user1(ਸੱਚ);
        AstGteS* ਅਚਲ ਮਾਪ_Gtep = ਨਵਾਂ AstGteS{fl, ਮਾਪ_ਗੰਢ_Exprp, ਨਵਾਂ AstConst{fl, 0}};
        ਮਾਪ_Gtep->user1(ਸੱਚ);
        ਵਾਪਸ ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, ਮਾਪ_Gtep};
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* getFromp(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp) {
        ਜੇ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(exprp, MemberSel)) {
            ਵਾਪਸ memberSelp->fromp();
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(exprp, ਵਿਧੀ_Call)) {
            ਵਾਪਸ ਵਿਧੀ_Callp->fromp();
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    AstVar* make_Tmp_Rand_ਬਹੁਲਕ_Var(Ast_ਗੰਢ_ਵਿਅੰਜਕ* siblingExprp, AstVar* rand_ਬਹੁਲਕ_Varp,
                               Ast_ਗੰਢ*& storeStmtspr, Ast_ਗੰਢ_Stmt*& restoreStmtspr) {
        File_ਸਤਰ* ਅਚਲ fl = rand_ਬਹੁਲਕ_Varp->fileline();
        AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Tmp_Varp = ਨਵਾਂ AstVar{
            fl, VVarType::BLOCKTEMP, m_ਬਹੁਲਕ_Unique_ਨਾਂ.get(rand_ਬਹੁਲਕ_Varp), rand_ਬਹੁਲਕ_Varp->dtypep()};
        rand_ਬਹੁਲਕ_Tmp_Varp->funcLocal(m_ftaskp);
        rand_ਬਹੁਲਕ_Tmp_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
            storeStmtspr,
            ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, rand_ਬਹੁਲਕ_Tmp_Varp, VAccess::WRITE},
                          make_ਬਹੁਲਕ_Var_Ref(siblingExprp, rand_ਬਹੁਲਕ_Varp, VAccess::READ)});
        storeStmtspr = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
            storeStmtspr,
            make_ਬਹੁਲਕ_ਸੈੱਟ_Loop(fl, make_ਬਹੁਲਕ_Var_Ref(siblingExprp, rand_ਬਹੁਲਕ_Varp, VAccess::WRITE),
                            ਨਵਾਂ AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
            restoreStmtspr,
            ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, make_ਬਹੁਲਕ_Var_Ref(siblingExprp, rand_ਬਹੁਲਕ_Varp, VAccess::WRITE),
                          ਨਵਾਂ AstVarRef{fl, rand_ਬਹੁਲਕ_Tmp_Varp, VAccess::READ}});
        ਵਾਪਸ rand_ਬਹੁਲਕ_Tmp_Varp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਟੁਕੜਾ_ਮੰਜ਼ਿਲ_Common_ਅਗੇਤਰ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* thisp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* otherp) {
        ਸਥਿਰ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        ਜਦੋਂ (thisp) {
            thisHier.ਧੱਕੋ_ਪਿੱਠ(thisp);
            thisp = getFromp(thisp);
        }
        ਜਦੋਂ (otherp) {
            otherHier.ਧੱਕੋ_ਪਿੱਠ(otherp);
            otherp = getFromp(otherp);
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* commonp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਸਵੈਚਾਲਿਤ thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            ਜੇ ((*thisIt)->type() != (*otherIt)->type()) ਰੋਕੋ;
            ਜੇ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ਅਚਲ otherMemberSelp = VN_AS(*otherIt, MemberSel);
                ਜੇ (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    ਜਾਰੀ;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਵਿਧੀ_Call* ਅਚਲ this_ਵਿਧੀ_Callp = VN_CAST(*thisIt, ਵਿਧੀ_Call)) {
                Ast_ਵਿਧੀ_Call* ਅਚਲ other_ਵਿਧੀ_Callp = VN_AS(*otherIt, ਵਿਧੀ_Call);
                ਜੇ (this_ਵਿਧੀ_Callp->taskp() == other_ਵਿਧੀ_Callp->taskp()) {
                    commonp = this_ਵਿਧੀ_Callp;
                    ਜਾਰੀ;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ ਪਹਿਲਾ_Var_Refp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ਅਚਲ ਦੂਜਾ_Var_Refp = VN_AS(*otherIt, VarRef);
                ਜੇ (ਪਹਿਲਾ_Var_Refp->varp() == ਦੂਜਾ_Var_Refp->varp()) {
                    commonp = ਪਹਿਲਾ_Var_Refp;
                    ਜਾਰੀ;
                }
            }
            ਰੋਕੋ;
        }
        ਵਾਪਸ commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* buildMemberSelExprp(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, AstVar* ਸਭ_ਤੋਂ_ਡੂੰਘਾ_Varp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resExprp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), ਖਾਲੀ_ਸੂਚਕ);
            AstVar* varp = memberSelp->varp();
            ਜੇ (ਸਭ_ਤੋਂ_ਡੂੰਘਾ_Varp) varp = ਸਭ_ਤੋਂ_ਡੂੰਘਾ_Varp;
            resExprp = ਨਵਾਂ AstMemberSel{exprp->fileline(), resExprp, varp};
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            ਜੇ (ਸਭ_ਤੋਂ_ਡੂੰਘਾ_Varp) varp = ਸਭ_ਤੋਂ_ਡੂੰਘਾ_Varp;
            resExprp = ਨਵਾਂ AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), ਗੰਢ_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(ਸੱਚ);
        ਵਾਪਸ resExprp;
    }

    ਖਾਲੀ ਜੋੜੋ_Update_Rand_Vars_Body(AstClass* ਅਚਲ nodep, AstFunc* ਅਚਲ randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ਅਚਲ updatep
            = ਨਵਾਂ AstFunc{nodep->fileline(), "__VupdateRandVars", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        updatep->class_ਵਿਧੀ(ਸੱਚ);
        updatep->isVirtual(ਸੱਚ);
        updatep->keepAlive(ਸੱਚ);
        updatep->needsSyms(ਝੂਠ);
        nodep->ਜੋੜੋ_Membersp(updatep);
        nodep->hasRandVarsUpdate(ਸੱਚ);

        ਲਈ (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ) {
            Ast_ਗੰਢ_FTask* ਅਚਲ newp = VN_AS(m_memberMap.ਲੱਭੋ_Member(classp, "new"), ਗੰਢ_FTask);
            ਲਈ (Ast_ਗੰਢ* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ stmtExprp = VN_CAST(stmtp, Stmt_ਵਿਅੰਜਕ);
                ਜੇ (!stmtExprp) ਜਾਰੀ;
                AstCMethodHard* ਅਚਲ methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                ਜੇ (!methodp || methodp->ਵਿਧੀ() != VCMethod::RANDOMIZER_WRITE_VAR) ਜਾਰੀ;

                Ast_ਗੰਢ_Var_Ref* ਅਚਲ varRefp = VN_AS(methodp->pinsp(), ਗੰਢ_Var_Ref);
                ਜੇ (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    ਜਾਰੀ;
                }
                AstCMethodHard* ਅਚਲ updateMethodp = ਨਵਾਂ AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                updateMethodp->dtype_ਸੈੱਟ_Void();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ namep = VN_AS(varRefp->nextp()->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
                updateMethodp->ਜੋੜੋ_Pinsp(namep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ));
                updateMethodp->ਵਿਧੀ(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->ਜੋੜੋ_Stmtsp(updateMethodp->makeStmt());
            }
        }
    }

    ਖਾਲੀ ਜੋੜੋ_Basic_Randomize_Body(AstFunc* ਅਚਲ basicRandomizep, AstClass* ਅਚਲ nodep,
                               AstVar* rand_ਬਹੁਲਕ_Varp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        AstVar* ਅਚਲ basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ਅਚਲ basicFvarRefp = ਨਵਾਂ AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ਅਚਲ beginBasicValp = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 1};
        basicRandomizep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, basicFvarRefp, beginBasicValp});
        Ast_ਗੰਢ_FTask* ਅਚਲ newp = VN_AS(m_memberMap.ਲੱਭੋ_Member(nodep, "new"), ਗੰਢ_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            ਜੇ (!memberVarp->rand().isRandomizable()) ਵਾਪਸ;
            ਅਚਲ Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {.asUQuad = memberVarp->user1()};
            ਜੇ (rand_ਬਹੁਲਕ.ਵਰਤਦਾ_ਬਹੁਲਕ
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ਅਚਲ setp = ਨਵਾਂ AstCMethodHard{
                    nodep->fileline(),
                    ਨਵਾਂ AstVarRef{fl, VN_AS(rand_ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module), rand_ਬਹੁਲਕ_Varp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, ਨਵਾਂ AstConst{nodep->fileline(), rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ}};
                setp->dtype_ਸੈੱਟ_UInt32();
                newp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, setp, ਨਵਾਂ AstConst{fl, 0}});
            }
            ਜੇ (memberVarp->user3()) ਵਾਪਸ;  // Handled in constraints
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = memberVarp->dtypep()->skipRefp();
            ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                ਜੇ (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    ਵਾਪਸ;
                }
                AstFunc* ਅਚਲ memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                Ast_ਵਿਧੀ_Call* ਅਚਲ callp
                    = memberVarp->globalConstrained()
                          ? ਨਵਾਂ Ast_ਵਿਧੀ_Call{fl,
                                              ਨਵਾਂ AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : ਨਵਾਂ Ast_ਵਿਧੀ_Call{
                                fl, ਨਵਾਂ AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ਅਚਲ basicFvarRefReadp = basicFvarRefp->clone_ਰੁੱਖ(ਝੂਠ);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ਅਚਲ ਸੌਂਪੋ_If_Not_Nullp = ਨਵਾਂ AstIf{
                    fl,
                    ਨਵਾਂ AstNeq{fl, ਨਵਾਂ AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               ਨਵਾਂ AstConst{fl, AstConst::Null{}}},
                    ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, basicFvarRefp->clone_ਰੁੱਖ(ਝੂਠ),
                                  ਨਵਾਂ AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->ਜੋੜੋ_Stmtsp(wrap_If_Rand_ਬਹੁਲਕ(nodep, memberVarp, ਸੌਂਪੋ_If_Not_Nullp));
            } ਨਹੀਂਤਾਂ {
                AstVar* ਅਚਲ randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ਅਚਲ refp
                    = ਨਵਾਂ AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_ਗੰਢ_Stmt* ਅਚਲ stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                ਜੇ (!refp->backp()) VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
                basicRandomizep->ਜੋੜੋ_Stmtsp(
                    ਨਵਾਂ AstBegin{fl, m_unique_ਨਾਂ.get("__VrandBlock"), stmtp, ਝੂਠ});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* make_ਬਹੁਲਕ_ਸੌਂਪੋ_Lhs(File_ਸਤਰ* ਅਚਲ fl, AstClass* ਅਚਲ classp,
                                   Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp, AstVar* ਅਚਲ ਬਹੁਲਕ_Varp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        ਜੇ (ਬਹੁਲਕ_Varp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            ਜੇ (fromp) VL_DO_DANGLING(fromp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), fromp);
            ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, VN_AS(ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module), ਬਹੁਲਕ_Varp,
                                 VAccess::WRITE};
        } ਨਹੀਂਤਾਂ ਜੇ (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            ਵਾਪਸ ਨਵਾਂ AstVarRef{fl, VN_AS(ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module), ਬਹੁਲਕ_Varp,
                                 VAccess::WRITE};
        } ਨਹੀਂਤਾਂ {
            AstMemberSel* ਅਚਲ memberselp = ਨਵਾਂ AstMemberSel{fl, fromp->unlink_Fr_ਪਿੱਠ(), ਬਹੁਲਕ_Varp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            ਵਾਪਸ memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    ਖਾਲੀ replace_With_ਬਹੁਲਕ_ਸੌਂਪੋ(Ast_ਗੰਢ_FTask_Ref* ਅਚਲ ftaskRefp, Ast_ਗੰਢ* ਅਚਲ receiverp,
                               Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp) {
        replace_With_ਬਹੁਲਕ_ਸੌਂਪੋ_And_Append(ftaskRefp, receiverp, lhsp, ਖਾਲੀ_ਸੂਚਕ);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    ਖਾਲੀ replace_With_ਬਹੁਲਕ_ਸੌਂਪੋ_And_Append(Ast_ਗੰਢ_FTask_Ref* ਅਚਲ ftaskRefp, Ast_ਗੰਢ* ਅਚਲ receiverp,
                                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp, Ast_ਗੰਢ* ਅਚਲ appendStmtp) {
        File_ਸਤਰ* ਅਚਲ fl = ftaskRefp->fileline();
        ਜੇ (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_ਵਿਅੰਜਕ), ftaskRefp, "Should be a statement");
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ftaskRefp->argsp()->exprp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ* newStmtp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                //ਬਹੁਲਕ
                ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.ਵਰਤਦਾ_ਬਹੁਲਕ, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ਅਚਲ setp = ਨਵਾਂ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                ਨਵਾਂ AstConst{fl, rmode.ਸੂਚਕਾਂਕ}};
                setp->dtype_ਸੈੱਟ_UInt32();
                newStmtp = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, setp, rhsp};
            } ਨਹੀਂਤਾਂ {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = make_ਬਹੁਲਕ_ਸੈੱਟ_Loop(fl, lhsp, rhsp, m_ftaskp);
            }
            ਜੇ (appendStmtp) newStmtp->ਜੋੜੋ_ਅਗਲਾ(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(ਧੱਕੋ_Deletep(m_stmtp), m_stmtp = ਖਾਲੀ_ਸੂਚਕ);
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.ਵਰਤਦਾ_ਬਹੁਲਕ, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ਅਚਲ setp = ਨਵਾਂ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            ਨਵਾਂ AstConst{fl, rmode.ਸੂਚਕਾਂਕ}};
            setp->dtype_ਸੈੱਟ_UInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    ਖਾਲੀ wrapRandomizeCallWithNullGuard(Ast_ਗੰਢ_FTask_Ref* nodep) {
        Ast_ਵਿਧੀ_Call* ਅਚਲ callp = VN_CAST(nodep, ਵਿਧੀ_Call);
        ਜੇ (!callp) ਵਾਪਸ;
        ਜੇ (callp->user4_ਸੈੱਟ_Once()) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ fl = callp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ checkp
            = ਨਵਾਂ AstNeq{fl, callp->fromp()->clone_ਰੁੱਖ(ਝੂਠ), ਨਵਾਂ AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlink_Fr_ਪਿੱਠ(&relinker);
        relinker.relink(ਨਵਾਂ AstCond{fl, checkp, callp, ਨਵਾਂ AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    ਖਾਲੀ handle_Randomize_ਦਲੀਲਾਂ(Ast_ਗੰਢ_FTask_Ref* ਅਚਲ nodep) {
        ਜੇ (!nodep->argsp()) ਵਾਪਸ;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        ਤਾਰਕਿਕ hasNullArg = ਝੂਠ;
        ਲਈ (AstArg *argp = nodep->argsp(), *nextp = ਖਾਲੀ_ਸੂਚਕ; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->ਅੰਕ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = ਸੱਚ;
                VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        ਲਈ (AstArg *argp = nodep->argsp(), *nextp = ਖਾਲੀ_ਸੂਚਕ; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ਲਈ (AstArg *otherArgp = nextp, *otherNextp = ਖਾਲੀ_ਸੂਚਕ; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prefixp
                    = ਟੁਕੜਾ_ਮੰਜ਼ਿਲ_Common_ਅਗੇਤਰ(argp->exprp(), otherArgp->exprp())) {
                    ਜੇ (prefixp == argp->exprp()) {
                        ਜੇ (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), otherArgp);
                        ਜਾਰੀ;
                    }
                }
                ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ prefixp
                    = ਟੁਕੜਾ_ਮੰਜ਼ਿਲ_Common_ਅਗੇਤਰ(otherArgp->exprp(), argp->exprp())) {
                    ਜੇ (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);
                        ਰੋਕੋ;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::ਸੈੱਟ<AstVar*> saved_Rand_ਬਹੁਲਕ_Varps;
        AstVar* tmpVarps = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* storeStmtsp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* ਸੈੱਟ_Stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_Stmt* restoreStmtsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (AstArg *argp = nodep->argsp(), *nextp = ਖਾਲੀ_ਸੂਚਕ; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = argp->exprp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ commonPrefixp = ਟੁਕੜਾ_ਮੰਜ਼ਿਲ_Common_ਅਗੇਤਰ(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_ਸਤਰ* ਅਚਲ fl = argp->fileline();
            ਜਦੋਂ (exprp) {
                ਜੇ (commonPrefixp == exprp) ਰੋਕੋ;
                AstVar* ਅਚਲ randVarp = getVarFromRef(exprp);
                AstClass* ਅਚਲ classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Varp = randVarp->lifetime().isStatic()
                                                 ? get_Static_Rand_ਬਹੁਲਕ_Var(classp)
                                                 : get_Rand_ਬਹੁਲਕ_Var_From_Class(classp);
                UASSERT_OBJ(rand_ਬਹੁਲਕ_Varp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                ਜੇ (saved_Rand_ਬਹੁਲਕ_Varps.ਲੱਭੋ(rand_ਬਹੁਲਕ_Varp) == saved_Rand_ਬਹੁਲਕ_Varps.ਸਿਰਾ()) {
                    AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Tmp_Varp
                        = make_Tmp_Rand_ਬਹੁਲਕ_Var(exprp, rand_ਬਹੁਲਕ_Varp, storeStmtsp, restoreStmtsp);
                    saved_Rand_ਬਹੁਲਕ_Varps.ਪਾਓ(rand_ਬਹੁਲਕ_Varp);
                    tmpVarps = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(tmpVarps, rand_ਬਹੁਲਕ_Tmp_Varp);
                }
                ਅਚਲ Randomize_ਬਹੁਲਕ rand_ਬਹੁਲਕ = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = ਨਵਾਂ AstCMethodHard{
                    fl, make_ਬਹੁਲਕ_Var_Ref(exprp, rand_ਬਹੁਲਕ_Varp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, ਨਵਾਂ AstConst{fl, rand_ਬਹੁਲਕ.ਸੂਚਕਾਂਕ}};
                setp->dtype_ਸੈੱਟ_UInt32();
                ਸੈੱਟ_Stmtsp
                    = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(ਸੈੱਟ_Stmtsp, ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, setp, ਨਵਾਂ AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);
        }
        ਜੇ (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* ਨਿਸ਼ਾਨਾ_Classp = ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(nodep, ਵਿਧੀ_Call);
            ਜੇ (ਵਿਧੀ_Callp) {
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDTypep = ਵਿਧੀ_Callp->fromp()->dtypep();
                ਅਚਲ AstClassRefDType* ਅਚਲ crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                ਨਿਸ਼ਾਨਾ_Classp = crdtp->classp();
            } ਨਹੀਂਤਾਂ {
                ਨਿਸ਼ਾਨਾ_Classp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(ਨਿਸ਼ਾਨਾ_Classp, nodep, "randomize(null) target class unresolved");
            AstFunc* ਅਚਲ checkOnlyFuncp = getCreateRandomizeNullFunc(ਨਿਸ਼ਾਨਾ_Classp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        ਜੇ (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && ਸੈੱਟ_Stmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlink_Fr_ਪਿੱਠ(&relinker);
            Ast_ਗੰਢ* ਅਚਲ stmtsp = storeStmtsp;
            stmtsp->ਜੋੜੋ_ਅਗਲਾ(ਸੈੱਟ_Stmtsp);
            stmtsp->ਜੋੜੋ_ਅਗਲਾ(m_stmtp);
            stmtsp->ਜੋੜੋ_ਅਗਲਾ(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            ਜੋੜੋ_ਆਰਜ਼ੀ_Varsp(tmpVarps);
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
    AstFunc* getCreateRandomizeNullFunc(AstClass* ਅਚਲ classp) {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ name = "__Vrandomize_null";
        ਜੇ (AstFunc* ਅਚਲ existingp = VN_AS(m_memberMap.ਲੱਭੋ_Member(classp, name), Func)) {
            ਵਾਪਸ existingp;
        }
        File_ਸਤਰ* ਅਚਲ fl = classp->fileline();
        AstFunc* ਅਚਲ funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ਅਚਲ fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        ਜੋੜੋ_Pre_Post_Call(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ਅਚਲ classGenp = getRandomGenerator(classp);
        ਜੇ (!classGenp) {
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, fvarp, VAccess::WRITE},
                                           ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 1}});
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_Module* ਅਚਲ genModp = VN_AS(classGenp->user2p(), ਗੰਢ_Module);
            funcp->ਜੋੜੋ_Stmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ਅਚਲ ਤਿਆਰੀ_All_Taskp = get_Create_Constraint_ਤਿਆਰੀ_Func(classp, m_memberMap);
            funcp->ਜੋੜੋ_Stmtsp((ਨਵਾਂ AstTaskRef{fl, ਤਿਆਰੀ_All_Taskp})->makeStmt());
            AstCExpr* ਅਚਲ solverCallp = ਨਵਾਂ AstCExpr{fl};
            solverCallp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            solverCallp->ਜੋੜੋ(ਨਵਾਂ AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->ਜੋੜੋ(".next_check_only(__Vm_rng)");
            funcp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(ਸੱਚ);
        }

        // 3. post_randomize -- only if result is non-zero
        ਜੇ (AstTask* ਅਚਲ userPostp = ਲੱਭੋ_Pre_Post_Task(classp, "post_randomize")) {
            AstTaskRef* ਅਚਲ callp = ਨਵਾਂ AstTaskRef{userPostp->fileline(), userPostp};
            funcp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        ਵਾਪਸ funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* lift_Log_If_Chain_ਮੰਜ਼ਿਲ_Constraint_If(AstLogIf* logIfp) {
        File_ਸਤਰ* ਅਚਲ fl = logIfp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = logIfp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = logIfp->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ* thenBodyp;
        ਜੇ (AstLogIf* ਅਚਲ innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = lift_Log_If_Chain_ਮੰਜ਼ਿਲ_Constraint_If(innerLogIfp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(innerLogIfp), innerLogIfp);
        } ਨਹੀਂਤਾਂ {
            thenBodyp = ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, rhsp};
        }
        ਵਾਪਸ ਨਵਾਂ AstConstraintIf{fl, condp, thenBodyp, ਖਾਲੀ_ਸੂਚਕ};
    }

    ਸਥਿਰ ਤਾਰਕਿਕ distBoundRefsRandVar(ਅਚਲ Ast_ਗੰਢ* boundp) {
        ਵਾਪਸ boundp->exists(
            [](ਅਚਲ AstVarRef* vrefp) { ਵਾਪਸ vrefp->varp()->rand().isRandomizable(); });
    }

    ਸਥਿਰ ਤਾਰਕਿਕ dist_Bound_Refs_ਬਹੁਲਕ_Var(ਅਚਲ Ast_ਗੰਢ* boundp) {
        ਵਾਪਸ boundp->exists([](ਅਚਲ AstVarRef* vrefp) {
            ਜੇ (!vrefp->varp()->rand().isRandomizable()) ਵਾਪਸ ਝੂਠ;
            ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = vrefp->varp()->user1()};
            ਵਾਪਸ rmode.ਵਰਤਦਾ_ਬਹੁਲਕ;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* newDistRangeMembership(AstDist* distp, ਅਚਲ AstInsideRange* irp) {
        File_ਸਤਰ* ਅਚਲ fl = distp->fileline();
        ਅਚਲ ਤਾਰਕਿਕ isSigned = distp->exprp()->isSigned();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dist_ਵਿਅੰਜਕ_Gtep = distp->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dist_ਵਿਅੰਜਕ_Ltep = distp->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        dist_ਵਿਅੰਜਕ_Gtep->user1(ਸੱਚ);
        dist_ਵਿਅੰਜਕ_Ltep->user1(ਸੱਚ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ gep
            = isSigned ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                             ਨਵਾਂ AstGteS{fl, dist_ਵਿਅੰਜਕ_Gtep, irp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)})
                       : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                             ਨਵਾਂ AstGte{fl, dist_ਵਿਅੰਜਕ_Gtep, irp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)});
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lep
            = isSigned ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                             ਨਵਾਂ AstLteS{fl, dist_ਵਿਅੰਜਕ_Ltep, irp->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)})
                       : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                             ਨਵਾਂ AstLte{fl, dist_ਵਿਅੰਜਕ_Ltep, irp->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)});
        gep->user1(ਸੱਚ);
        lep->user1(ਸੱਚ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ andp = ਨਵਾਂ AstLogAnd{fl, gep, lep};
        andp->user1(ਸੱਚ);
        ਵਾਪਸ andp;
    }

    ਬਣਤਰ Dist_ਬਾਲਟੀ final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rangep;  // A single value or an InsideRange
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਭਾਰ_Exprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::ਸਦਿਸ਼<Dist_ਬਾਲਟੀ> collectDistBuckets(AstDist* ਅਚਲ distp) {
        File_ਸਤਰ* ਅਚਲ fl = distp->fileline();
        std::ਸਦਿਸ਼<Dist_ਬਾਲਟੀ> buckets;
        ਲਈ (Ast_Dist_ਵਸਤੂ* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), Dist_ਵਸਤੂ)) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(ditemp->weightp(), Const)) {
                ਜੇ (constp->ਮੰਜ਼ਿਲ_UQuad() == 0) ਜਾਰੀ;
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਭਾਰ_Exprp
                = ਨਵਾਂ AstExtend{fl, ditemp->weightp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 64};
            // := on a range weights every element, so scale by the range size.
            ਜੇ (!ditemp->is_ਸਾਰਾ()) {
                ਜੇ (ਅਚਲ AstInsideRange* ਅਚਲ irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ਅਚਲ AstConst* ਅਚਲ lop = VN_CAST(irp->lhsp(), Const);
                    ਅਚਲ AstConst* ਅਚਲ hip = VN_CAST(irp->rhsp(), Const);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* rangeSizep;
                    ਜੇ (lop && hip) {
                        ਅਚਲ uint64_t range_ਮਾਪ = hip->ਮੰਜ਼ਿਲ_UQuad() - lop->ਮੰਜ਼ਿਲ_UQuad() + 1;
                        rangeSizep = ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, range_ਮਾਪ};
                    } ਨਹੀਂਤਾਂ {
                        rangeSizep = ਨਵਾਂ Ast_ਜੋੜੋ{
                            fl, ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, 1},
                            ਨਵਾਂ AstSub{fl,
                                       ਨਵਾਂ AstExtend{fl, irp->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 64},
                                       ਨਵਾਂ AstExtend{fl, irp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 64}}};
                    }
                    ਭਾਰ_Exprp = ਨਵਾਂ AstMul{fl, ਭਾਰ_Exprp, rangeSizep};
                }
            }
            buckets.ਧੱਕੋ_ਪਿੱਠ({ditemp->rangep(), ਭਾਰ_Exprp});
        }
        ਵਾਪਸ buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_Dist_Membership_ਪਦ(AstDist* distp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rangep) {
        ਜੇ (ਅਚਲ AstInsideRange* ਅਚਲ irp = VN_CAST(rangep, InsideRange)) {
            ਵਾਪਸ newDistRangeMembership(distp, irp);
        }
        File_ਸਤਰ* ਅਚਲ fl = distp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eqExprp = distp->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        eqExprp->user1(ਸੱਚ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eqp = ਨਵਾਂ AstEq{fl, eqExprp, rangep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
        eqp->user1(ਸੱਚ);
        ਵਾਪਸ eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_ਵਿਅੰਜਕ* buildDistMembership(AstDist* distp,
                                           ਅਚਲ std::ਸਦਿਸ਼<Dist_ਬਾਲਟੀ>& buckets) {
        File_ਸਤਰ* ਅਚਲ fl = distp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* unionExprp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਬਾਲਟੀ : buckets) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ memberp = new_Dist_Membership_ਪਦ(distp, ਬਾਲਟੀ.rangep);
            ਜੇ (!unionExprp) {
                unionExprp = memberp;
            } ਨਹੀਂਤਾਂ {
                unionExprp = ਨਵਾਂ AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(ਸੱਚ);
            }
        }
        ਵਾਪਸ ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newGateJoin(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ap, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bp, ਤਾਰਕਿਕ isAnd) {
        ਜੇ (ap->same_ਰੁੱਖ(bp)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(bp), bp);
            ਵਾਪਸ ap;
        }
        ਜੇ (VN_IS(bp, Const)) std::ਅਦਲਾ_ਬਦਲੀ(ap, bp);
        ਜੇ (VN_IS(ap, Const)) {
            ਅਚਲ ਤਾਰਕਿਕ constWins = VN_AS(ap, Const)->isZero() == isAnd;
            ਜੇ (!constWins) std::ਅਦਲਾ_ਬਦਲੀ(ap, bp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(bp), bp);
            ਵਾਪਸ ap;
        }
        ਜੇ (isAnd) ਵਾਪਸ ਨਵਾਂ AstLogAnd{fl, ap, bp};
        ਵਾਪਸ ਨਵਾਂ AstLogOr{fl, ap, bp};
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newGateOr(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ap, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bp) {
        ਵਾਪਸ newGateJoin(fl, ap, bp, ਝੂਠ);
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newGateAnd(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ap, Ast_ਗੰਢ_ਵਿਅੰਜਕ* bp) {
        ਵਾਪਸ newGateJoin(fl, ap, bp, ਸੱਚ);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਬਹੁਲਕ_ਦੋਆਧਾਰੀ_ਅੰਕ_Read(AstVar* varp, ਅਚਲ AstMemberSel* mselp, AstVar* rand_ਬਹੁਲਕ_Varp,
                                File_ਸਤਰ* fl) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* arrayp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (varp->lifetime().isStatic()) {
            AstClass* ਅਚਲ ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ਅਚਲ smodep = get_Static_Rand_ਬਹੁਲਕ_Var(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = ਨਵਾਂ AstVarRef{fl, VN_AS(smodep->user2p(), ਗੰਢ_Module), smodep, VAccess::READ};
        } ਨਹੀਂਤਾਂ ਜੇ (mselp) {
            AstVar* ਅਚਲ memberModep = get_Rand_ਬਹੁਲਕ_Var_From_Class(VN_AS(varp->user2p(), ਗੰਢ_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = ਨਵਾਂ AstMemberSel{fl, mselp->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), memberModep};
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(rand_ਬਹੁਲਕ_Varp, varp, "rand_mode variable without a class mode array");
            arrayp = ਨਵਾਂ AstVarRef{fl, VN_AS(rand_ਬਹੁਲਕ_Varp->user2p(), ਗੰਢ_Module), rand_ਬਹੁਲਕ_Varp,
                                   VAccess::READ};
        }
        ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ਅਚਲ atp
            = ਨਵਾਂ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, ਨਵਾਂ AstConst{fl, rmode.ਸੂਚਕਾਂਕ}};
        atp->dtype_ਸੈੱਟ_UInt32();
        ਵਾਪਸ atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newVarGate(AstVar* varp, ਅਚਲ AstMemberSel* mselp, ਤਾਰਕਿਕ ownerLevel,
                            AstVar* rand_ਬਹੁਲਕ_Varp, File_ਸਤਰ* fl) {
        ਜੇ (!varp->rand().isRandomizable()) {
            ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}, ownerLevel};
        }
        ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = varp->user1()};
        ਜੇ (!rmode.ਵਰਤਦਾ_ਬਹੁਲਕ) ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
        ਵਾਪਸ new_ਬਹੁਲਕ_ਦੋਆਧਾਰੀ_ਅੰਕ_Read(varp, mselp, rand_ਬਹੁਲਕ_Varp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newAccessGate(ਅਚਲ Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ ownerLevel, AstVar* rand_ਬਹੁਲਕ_Varp,
                               File_ਸਤਰ* fl) {
        ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ mselp = VN_CAST(nodep, MemberSel)) {
            ਵਾਪਸ newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, rand_ਬਹੁਲਕ_Varp, fl),
                              newAccessGate(mselp->fromp(), ਸੱਚ, rand_ਬਹੁਲਕ_Varp, fl));
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ refp = VN_CAST(nodep, ਗੰਢ_Var_Ref)) {
            ਵਾਪਸ newVarGate(refp->varp(), ਖਾਲੀ_ਸੂਚਕ, ownerLevel, rand_ਬਹੁਲਕ_Varp, fl);
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Sel* ਅਚਲ selp = VN_CAST(nodep, ਗੰਢ_Sel)) {
            ਵਾਪਸ newAccessGate(selp->fromp(), ਸੱਚ, rand_ਬਹੁਲਕ_Varp, fl);
        }
        ਜੇ (dist_Bound_Refs_ਬਹੁਲਕ_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newDistGate(ਅਚਲ Ast_ਗੰਢ* nodep, AstVar* rand_ਬਹੁਲਕ_Varp, File_ਸਤਰ* fl) {
        ਜੇ (VN_IS(nodep, ਗੰਢ_Var_Ref) || VN_IS(nodep, MemberSel)) {
            ਵਾਪਸ newAccessGate(nodep, ਝੂਠ, rand_ਬਹੁਲਕ_Varp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Sel* ਅਚਲ selp = VN_CAST(nodep, ਗੰਢ_Sel)) {
            ਵਾਪਸ newDistGate(selp->fromp(), rand_ਬਹੁਲਕ_Varp, fl);
        }
        ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) {
            ਵਾਪਸ newDistGate(selp->fromp(), rand_ਬਹੁਲਕ_Varp, fl);
        }
        ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ selp = VN_CAST(nodep, StructSel)) {
            ਵਾਪਸ newDistGate(selp->fromp(), rand_ਬਹੁਲਕ_Varp, fl);
        }
        ਜੇ (ਅਚਲ AstCMethodHard* ਅਚਲ methodp = VN_CAST(nodep, CMethodHard)) {
            ਜੇ (methodp->ਵਿਧੀ() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                ਵਾਪਸ newDistGate(methodp->fromp(), rand_ਬਹੁਲਕ_Varp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        ਜੇ (ਅਚਲ AstCond* ਅਚਲ condp = VN_CAST(nodep, Cond)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thenGatep = newDistGate(condp->thenp(), rand_ਬਹੁਲਕ_Varp, fl);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elseGatep = newDistGate(condp->elsep(), rand_ਬਹੁਲਕ_Varp, fl);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* armGatep = thenGatep;
            ਜੇ (thenGatep->same_ਰੁੱਖ(elseGatep)) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(elseGatep), elseGatep);
            } ਨਹੀਂਤਾਂ {
                armGatep
                    = ਨਵਾਂ AstCond{fl, condp->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), thenGatep, elseGatep};
            }
            ਵਾਪਸ newGateOr(fl, newDistGate(condp->condp(), rand_ਬਹੁਲਕ_Varp, fl), armGatep);
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Uniop* ਅਚਲ uopp = VN_CAST(nodep, ਗੰਢ_Uniop)) {
            ਵਾਪਸ newDistGate(uopp->lhsp(), rand_ਬਹੁਲਕ_Varp, fl);
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ bopp = VN_CAST(nodep, ਗੰਢ_Biop)) {
            ਵਾਪਸ newGateOr(fl, newDistGate(bopp->lhsp(), rand_ਬਹੁਲਕ_Varp, fl),
                             newDistGate(bopp->rhsp(), rand_ਬਹੁਲਕ_Varp, fl));
        }
        ਜੇ (dist_Bound_Refs_ਬਹੁਲਕ_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        ਵਾਪਸ ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_ਵਿਅੰਜਕ* buildFrozenMembership(AstDist* distp,
                                             ਅਚਲ std::ਸਦਿਸ਼<Dist_ਬਾਲਟੀ>& buckets) {
        File_ਸਤਰ* ਅਚਲ fl = distp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* unionExprp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਬਾਲਟੀ : buckets) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* termp = new_Dist_Membership_ਪਦ(distp, ਬਾਲਟੀ.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ਅਚਲ ਤਾਰਕਿਕ runtime_ਭਾਰ
                = ਬਾਲਟੀ.ਭਾਰ_Exprp->exists([](ਅਚਲ Ast_ਗੰਢ_Var_Ref*) { ਵਾਪਸ ਸੱਚ; });
            ਜੇ (runtime_ਭਾਰ) {
                AstNeq* ਅਚਲ nzp = ਨਵਾਂ AstNeq{fl, ਬਾਲਟੀ.ਭਾਰ_Exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                                               ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(ਸੱਚ);
                termp = ਨਵਾਂ AstLogAnd{fl, termp, nzp};
                termp->user1(ਸੱਚ);
            }
            ਜੇ (unionExprp) {
                unionExprp = ਨਵਾਂ AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(ਸੱਚ);
            } ਨਹੀਂਤਾਂ {
                unionExprp = termp;
            }
        }
        ਵਾਪਸ ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_ਗੰਢ* gateFrozenDist(Ast_ਗੰਢ* chainp, AstDist* distp,
                            ਅਚਲ std::ਸਦਿਸ਼<Dist_ਬਾਲਟੀ>& buckets, AstVar* rand_ਬਹੁਲਕ_Varp) {
        File_ਸਤਰ* ਅਚਲ fl = distp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* gatep = newDistGate(distp->exprp(), rand_ਬਹੁਲਕ_Varp, fl);
        ਜੇ (VN_IS(gatep, Const)) {
            ਅਚਲ ਤਾਰਕਿਕ drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(gatep), gatep);
            ਜੇ (drawable) ਵਾਪਸ chainp;
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(chainp), chainp);
            ਵਾਪਸ buildFrozenMembership(distp, buckets);
        }
        ਵਾਪਸ ਨਵਾਂ AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਇਕਸਾਰ_Range_Pick(AstDist* distp, ਅਚਲ AstInsideRange* irp) {
        File_ਸਤਰ* ਅਚਲ fl = distp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dist_ਵਿਅੰਜਕ_Copyp = distp->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        dist_ਵਿਅੰਜਕ_Copyp->user1(ਸੱਚ);
        ਅਚਲ ਪੂਰਨ dist_ਚੌੜਾਈ = distp->exprp()->ਚੌੜਾਈ();
        ਅਚਲ AstConst* ਅਚਲ lopC = VN_CAST(irp->lhsp(), Const);
        ਅਚਲ AstConst* ਅਚਲ hipC = VN_CAST(irp->rhsp(), Const);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* rangeSzp;
        ਜੇ (lopC && hipC) {
            ਅਚਲ uint64_t rsz = hipC->ਮੰਜ਼ਿਲ_UQuad() - lopC->ਮੰਜ਼ਿਲ_UQuad() + 1;
            rangeSzp = ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, rsz};
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਤਾਰਕਿਕ isSigned = irp->lhsp()->isSigned();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lo64p
                = isSigned ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                 ਨਵਾਂ AstExtendS{fl, irp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 64})
                           : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                 ਨਵਾਂ AstExtend{fl, irp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 64});
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ hi64p
                = isSigned ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                 ਨਵਾਂ AstExtendS{fl, irp->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 64})
                           : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(
                                 ਨਵਾਂ AstExtend{fl, irp->rhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 64});
            rangeSzp = ਨਵਾਂ Ast_ਜੋੜੋ{fl, ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  ਨਵਾਂ AstSub{fl, hi64p, lo64p}};
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rand64p = ਨਵਾਂ AstRand{fl, ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
        rand64p->dtype_ਸੈੱਟ_UInt64();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ offsetp
            = ਨਵਾਂ AstCCast{fl, ਨਵਾਂ AstModDiv{fl, rand64p, rangeSzp}, dist_ਚੌੜਾਈ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = ਨਵਾਂ Ast_ਜੋੜੋ{fl, irp->lhsp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), offsetp};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ eqp = ਨਵਾਂ AstEq{fl, dist_ਵਿਅੰਜਕ_Copyp, valuep};
        eqp->user1(ਸੱਚ);
        ਵਾਪਸ eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_ਗੰਢ* build_Weighted_ਬਾਲਟੀ_Chain(AstDist* distp, ਅਚਲ std::ਸਦਿਸ਼<Dist_ਬਾਲਟੀ>& buckets,
                                      AstVar* ਬਾਲਟੀ_Varp,
                                      ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& cumSums) {
        File_ਸਤਰ* ਅਚਲ fl = distp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* chainp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਪੂਰਨ i = static_cast<ਪੂਰਨ>(buckets.ਮਾਪ()) - 1; i >= 0; --i) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* constraintExprp;
            ਅਚਲ AstInsideRange* ਅਚਲ irp = VN_CAST(buckets[i].rangep, InsideRange);
            ਜੇ (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } ਨਹੀਂਤਾਂ ਜੇ (irp) {
                constraintExprp = new_ਇਕਸਾਰ_Range_Pick(distp, irp);
            } ਨਹੀਂਤਾਂ {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dist_ਵਿਅੰਜਕ_Copyp = distp->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                dist_ਵਿਅੰਜਕ_Copyp->user1(ਸੱਚ);
                constraintExprp
                    = ਨਵਾਂ AstEq{fl, dist_ਵਿਅੰਜਕ_Copyp, buckets[i].rangep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                constraintExprp->user1(ਸੱਚ);
            }
            ਜੇ (!chainp) {
                chainp = constraintExprp;
            } ਨਹੀਂਤਾਂ {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp
                    = ਨਵਾਂ AstLte{fl, ਨਵਾਂ AstVarRef{fl, ਬਾਲਟੀ_Varp, VAccess::READ}, cumSums[i]};
                condp->user1(ਸੱਚ);
                chainp = ਨਵਾਂ AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(ਸੱਚ);
            }
        }
        Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ chainExprp = ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, chainp};
        chainExprp->isSoft(ਸੱਚ);
        ਵਾਪਸ chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    ਖਾਲੀ ਹੇਠਲਾ_Dist_Constraints(AstTask* taskp, Ast_ਗੰਢ* constrItemsp, AstVar* rand_ਬਹੁਲਕ_Varp,
                              AstConstraintForeach* foreachp = ਖਾਲੀ_ਸੂਚਕ) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_ਗੰਢ* foreachTailp = ਖਾਲੀ_ਸੂਚਕ;
        ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Stmt = [&](Ast_ਗੰਢ* nodep) {
            ਜੇ (foreachp) {
                ਜੇ (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } ਨਹੀਂਤਾਂ {
                    foreachTailp->ਜੋੜੋ_ਅਗਲਾ(nodep);
                    foreachTailp = nodep;
                }
            } ਨਹੀਂਤਾਂ {
                taskp->ਜੋੜੋ_Stmtsp(nodep);
            }
        };

        ਲਈ (Ast_ਗੰਢ *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            ਜੇ (AstConstraintIf* ਅਚਲ cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                ਹੇਠਲਾ_Dist_Constraints(taskp, cifp->thensp(), rand_ਬਹੁਲਕ_Varp, foreachp);
                ਜੇ (cifp->elsesp()) {
                    ਹੇਠਲਾ_Dist_Constraints(taskp, cifp->elsesp(), rand_ਬਹੁਲਕ_Varp, foreachp);
                }
                ਜਾਰੀ;
            }
            ਜੇ (AstConstraintForeach* ਅਚਲ cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                ਹੇਠਲਾ_Dist_Constraints(taskp, cfep->bodyp(), rand_ਬਹੁਲਕ_Varp, cfep);
                ਜਾਰੀ;
            }

            Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ constrExprp = VN_CAST(itemp, Constraint_ਵਿਅੰਜਕ);
            ਜੇ (!constrExprp) ਜਾਰੀ;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            ਜੇ (AstLogIf* ਅਚਲ ਸਿਖਰ_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_ਗੰਢ* chainEndp = ਸਿਖਰ_Log_Ifp->rhsp();
                ਜਦੋਂ (AstLogIf* ਅਚਲ innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                ਜੇ (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ਅਚਲ liftedp = lift_Log_If_Chain_ਮੰਜ਼ਿਲ_Constraint_If(ਸਿਖਰ_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(constrExprp), constrExprp);
                    ਹੇਠਲਾ_Dist_Constraints(taskp, liftedp->thensp(), rand_ਬਹੁਲਕ_Varp, foreachp);
                    ਜਾਰੀ;
                }
            }

            AstDist* ਅਚਲ distp = VN_CAST(constrExprp->exprp(), Dist);
            ਜੇ (!distp) ਜਾਰੀ;

            File_ਸਤਰ* ਅਚਲ fl = distp->fileline();

            std::ਸਦਿਸ਼<Dist_ਬਾਲਟੀ> buckets = collectDistBuckets(distp);

            ਜੇ (buckets.ਖਾਲੀ_ਥਾਵਾਂ()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ truep
                    = ਨਵਾਂ Ast_Constraint_ਵਿਅੰਜਕ{fl, ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(constrExprp), constrExprp);
                ਜਾਰੀ;
            }

            Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਕੁੱਲ_ਭਾਰ_Exprp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਸਵੈਚਾਲਿਤ& ਬਾਲਟੀ : buckets) {
                ਜੇ (!ਕੁੱਲ_ਭਾਰ_Exprp) {
                    ਕੁੱਲ_ਭਾਰ_Exprp = ਬਾਲਟੀ.ਭਾਰ_Exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                } ਨਹੀਂਤਾਂ {
                    ਕੁੱਲ_ਭਾਰ_Exprp = ਨਵਾਂ Ast_ਜੋੜੋ{fl, ਕੁੱਲ_ਭਾਰ_Exprp,
                                                  ਬਾਲਟੀ.ਭਾਰ_Exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    ਕੁੱਲ_ਭਾਰ_Exprp->dtype_ਸੈੱਟ_UInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ਅਚਲ ਪੂਰਨ distId = m_dist_ਅੰਕ++;
            ਅਚਲ std::string ਕੁੱਲ_Name = "__Vdist_total" + cvt_ਮੰਜ਼ਿਲ_Str(distId);
            AstVar* ਅਚਲ ਕੁੱਲ_Varp
                = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, ਕੁੱਲ_Name, taskp->ਲੱਭੋ_UInt64_DType()};
            ਕੁੱਲ_Varp->noSubst(ਸੱਚ);
            ਕੁੱਲ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ਕੁੱਲ_Varp->funcLocal(ਸੱਚ);
            ਕੁੱਲ_Varp->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
            ਜੋੜੋ_Stmt(ਕੁੱਲ_Varp);
            ਜੋੜੋ_Stmt(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, ਕੁੱਲ_Varp, VAccess::WRITE}, ਕੁੱਲ_ਭਾਰ_Exprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ਅਚਲ std::string ਬਾਲਟੀ_Name = "__Vdist_bucket" + cvt_ਮੰਜ਼ਿਲ_Str(distId);
            AstVar* ਅਚਲ ਬਾਲਟੀ_Varp
                = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, ਬਾਲਟੀ_Name, taskp->ਲੱਭੋ_UInt64_DType()};
            ਬਾਲਟੀ_Varp->noSubst(ਸੱਚ);
            ਬਾਲਟੀ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ਬਾਲਟੀ_Varp->funcLocal(ਸੱਚ);
            ਬਾਲਟੀ_Varp->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
            ਜੋੜੋ_Stmt(ਬਾਲਟੀ_Varp);

            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ randp = ਨਵਾਂ AstRand{fl, ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
            randp->dtype_ਸੈੱਟ_UInt64();
            ਜੋੜੋ_Stmt(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                fl, ਨਵਾਂ AstVarRef{fl, ਬਾਲਟੀ_Varp, VAccess::WRITE},
                ਨਵਾਂ Ast_ਜੋੜੋ{
                    fl, ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, 1},
                    ਨਵਾਂ AstModDiv{fl, randp, ਨਵਾਂ AstVarRef{fl, ਕੁੱਲ_Varp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> cumSums;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* runningSump = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਮਾਪ_t i = 0; i < buckets.ਮਾਪ(); ++i) {
                ਜੇ (!runningSump) {
                    runningSump = buckets[i].ਭਾਰ_Exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                } ਨਹੀਂਤਾਂ {
                    runningSump = ਨਵਾਂ Ast_ਜੋੜੋ{fl, runningSump,
                                             buckets[i].ਭਾਰ_Exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
                    runningSump->dtype_ਸੈੱਟ_UInt64();
                }
                cumSums.ਧੱਕੋ_ਪਿੱਠ(runningSump->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ));
            }

            Ast_ਗੰਢ* chainp = build_Weighted_ਬਾਲਟੀ_Chain(distp, buckets, ਬਾਲਟੀ_Varp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, rand_ਬਹੁਲਕ_Varp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            ਲਈ (ਸਵੈਚਾਲਿਤ& ਬਾਲਟੀ : buckets) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਬਾਲਟੀ.ਭਾਰ_Exprp), ਬਾਲਟੀ.ਭਾਰ_Exprp);
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            ਧੱਕੋ_Deletep(cumSums.ਪਿੱਠ());
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ is_Dyn_Arr_ਦਾ_Class_Type_Recurse(ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ refp = dtypep->skipRefp();
        ਜੇ (VN_IS(refp, DynArrayDType) || VN_IS(refp, ਕਤਾਰ_DType)) {
            ਵਾਪਸ is_Dyn_Arr_ਦਾ_Class_Type_Recurse(refp->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(refp, ClassRefDType)) {
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    ਖਾਲੀ ਜੋੜੋ_Array_Elem_Refresh(File_ਸਤਰ* ਅਚਲ fl, AstVar* ਅਚਲ arrVarp, AstFunc* ਅਚਲ randomizep,
                             AstVar* ਅਚਲ genp) {
        Ast_ਗੰਢ_Module* ਅਚਲ genModp = VN_AS(genp->user2p(), ਗੰਢ_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ਅਚਲ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(ਝੂਠ).ਦੂਜਾ;
        ਜੇ (is_Dyn_Arr_ਦਾ_Class_Type_Recurse(arrVarp->dtypep())) {
            ਜੇ (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            ਵਾਪਸ;
        }
        AstCMethodHard* ਅਚਲ methodp
            = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_ਸੈੱਟ_Void();

        Ast_ਗੰਢ_Module* ਅਚਲ classp = VN_AS(arrVarp->user2p(), ਗੰਢ_Module);
        AstVarRef* ਅਚਲ varRefp = ਨਵਾਂ AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->ਜੋੜੋ_Pinsp(varRefp);

        ਅਚਲ ਮਾਪ_t ਚੌੜਾਈ = arrayElementDTypep(arrVarp->dtypep())->ਚੌੜਾਈ();

        methodp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, ਚੌੜਾਈ});
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ varnamep
            = ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "\"" + arrVarp->name() + "\"", arrVarp->ਚੌੜਾਈ()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->ਜੋੜੋ_Pinsp(varnamep);
        methodp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = arrVarp->user1()};
        ਜੇ (rmode.ਵਰਤਦਾ_ਬਹੁਲਕ) {
            methodp->ਜੋੜੋ_Pinsp(ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, rmode.ਸੂਚਕਾਂਕ});
        }

        randomizep->ਜੋੜੋ_Stmtsp(methodp->makeStmt());
    }

    Ast_ਗੰਢ_Stmt* new_Pin_ਮਾਪ_Variable(File_ਸਤਰ* ਅਚਲ fl, AstVar* ਅਚਲ arrVarp,
                                    AstVar* ਅਚਲ genp) {
        Ast_ਗੰਢ_Module* ਅਚਲ genModp = VN_AS(genp->user2p(), ਗੰਢ_Module);
        AstVar* ਅਚਲ ਮਾਪ_Varp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ਅਚਲ pinp
            = ਨਵਾਂ AstCMethodHard{fl, ਨਵਾਂ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtype_ਸੈੱਟ_Void();
        AstCExpr* ਅਚਲ namep = ਨਵਾਂ AstCExpr{fl, AstCExpr::ਸ਼ੁੱਧ{}, "\"" + ਮਾਪ_Varp->name() + "\""};
        namep->dtype_ਸੈੱਟ_UInt32();
        pinp->ਜੋੜੋ_Pinsp(namep);
        pinp->ਜੋੜੋ_Pinsp(
            ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(ਮਾਪ_Varp->ਚੌੜਾਈ())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ਅਚਲ ਮਾਪ_Var_Refp = ਨਵਾਂ AstVarRef{fl, ਮਾਪ_Varp, VAccess::READ};
        ਮਾਪ_Var_Refp->classOrPackagep(VN_AS(ਮਾਪ_Varp->user2p(), ਗੰਢ_Module));
        pinp->ਜੋੜੋ_Pinsp(ਮਾਪ_Var_Refp);
        ਵਾਪਸ pinp->makeStmt();
    }

    ਖਾਲੀ ਜੋੜੋ_Frozen_ਮਾਪ_Pin(File_ਸਤਰ* ਅਚਲ fl, AstVar* ਅਚਲ arrVarp, AstFunc* ਅਚਲ randomizep,
                          AstVar* ਅਚਲ genp, AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Varp) {
        ਅਚਲ Randomize_ਬਹੁਲਕ rmode = {.asUQuad = arrVarp->user1()};
        ਜੇ (!rmode.ਵਰਤਦਾ_ਬਹੁਲਕ) ਵਾਪਸ;
        AstVar* ਅਚਲ ਮਾਪ_Varp = VN_AS(arrVarp->user4p(), Var);
        Ast_ਗੰਢ_Module* ਅਚਲ arrClassp = VN_AS(arrVarp->user2p(), ਗੰਢ_Module);
        Ast_ਗੰਢ_Module* ਅਚਲ ਮਾਪ_Classp = VN_AS(ਮਾਪ_Varp->user2p(), ਗੰਢ_Module);
        AstVarRef* ਅਚਲ ਮਾਪ_Writep = ਨਵਾਂ AstVarRef{fl, ਮਾਪ_Classp, ਮਾਪ_Varp, VAccess::WRITE};
        ਅਚਲ VCMethod ਮਾਪ_ਵਿਧੀ = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_ਮਾਪ
                                        : VCMethod::DYN_ਮਾਪ;
        AstCMethodHard* ਅਚਲ currentSizep = ਨਵਾਂ AstCMethodHard{
            fl, ਨਵਾਂ AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, ਮਾਪ_ਵਿਧੀ};
        currentSizep->dtypep(ਮਾਪ_Varp->dtypep());
        randomizep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਮਾਪ_Writep, currentSizep});
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ modep = new_ਬਹੁਲਕ_ਦੋਆਧਾਰੀ_ਅੰਕ_Read(arrVarp, ਖਾਲੀ_ਸੂਚਕ, rand_ਬਹੁਲਕ_Varp, fl);
        randomizep->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstLogNot{fl, modep}, new_Pin_ਮਾਪ_Variable(fl, arrVarp, genp)});
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_ਅੰਕ);
        VL_RESTORER_COPY(m_rand_Array_Unique_ਨਾਂ);
        m_modp = nodep;
        m_rand_Case_ਅੰਕ = 0;
        m_rand_Array_Unique_ਨਾਂ.reset();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_ਅੰਕ);
        VL_RESTORER(m_dist_ਅੰਕ);
        m_modp = nodep;
        m_rand_Case_ਅੰਕ = 0;
        m_dist_ਅੰਕ = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        ਜੇ (!nodep->user1()) ਵਾਪਸ;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->ਅਧਾਰ_Most_Classp()->needRNG(ਸੱਚ);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ਅਚਲ ਤਾਰਕਿਕ basic_ਪਹਿਲਾ = classOwnsGlobalConstraint(nodep);

        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        AstFunc* ਅਚਲ randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ਅਚਲ fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Varp = get_Rand_ਬਹੁਲਕ_Var_From_Class(nodep);
        ਜੋੜੋ_Pre_Post_Call(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ਜੇ (classHasRandClassMembers(nodep)) {
            AstTask* ਅਚਲ preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->ਜੋੜੋ_Stmtsp((ਨਵਾਂ AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* beginValp = ਖਾਲੀ_ਸੂਚਕ;
        AstVar* genp = getRandomGenerator(nodep);
        ਜੇ (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ਅਚਲ prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ਅਚਲ classp, AstConstraint* ਅਚਲ constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                ਜੇ (!taskp) {
                    taskp = new_ਤਿਆਰੀ_Constraint_Task(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                ਜੇ (AstTask* ਅਚਲ resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ਅਚਲ resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ਅਚਲ resizeTaskRefp
                        = ਨਵਾਂ AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->ਜੋੜੋ_Stmtsp(resizeTaskRefp->makeStmt());
                }

                ਜੇ (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                ਜੇ (constrp->itemsp()) {
                    ਹੇਠਲਾ_Dist_Constraints(taskp, constrp->itemsp(), rand_ਬਹੁਲਕ_Varp);
                }
                VInsertion_ਸੈੱਟ<AstVar*>& ਮਾਪ_Arrays = m_ਮਾਪ_Constrained_Arrays[classp];
                Constraint_ਵਿਅੰਜਕ_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), ਖਾਲੀ_ਸੂਚਕ,
                    genp,          rand_ਬਹੁਲਕ_Varp, m_writtenVars,     &m_uniqueConstraintId,
                    m_unique_ਨਾਂ, randomizep,   &ਮਾਪ_Arrays,       prepareArraysTaskp};
                ਜੇ (constrp->itemsp()) {
                    taskp->ਜੋੜੋ_Stmtsp(wrap_If_Constraint_ਬਹੁਲਕ(
                        nodep, constrp, constrp->itemsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ਅਚਲ ਸਵੈਚਾਲਿਤ constraint_ਡੂੰਘਾਈ = [](ਅਚਲ AstConstraint* constrp) {
                    ਪੂਰਨ ਡੂੰਘਾਈ = 0;
                    ਮਾਪ_t pos = 0;
                    ਜਦੋਂ ((pos = constrp->name().ਲੱਭੋ(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++ਡੂੰਘਾਈ;
                        pos += std::ਲੜੀ_ਲੰਬਾਈ(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    ਵਾਪਸ ਡੂੰਘਾਈ;
                };
                ਪੂਰਨ max_ਡੂੰਘਾਈ = 0;
                nodep->foreachMember([&](AstClass* ਅਚਲ, AstConstraint* ਅਚਲ constrp) {
                    max_ਡੂੰਘਾਈ = std::max(max_ਡੂੰਘਾਈ, constraint_ਡੂੰਘਾਈ(constrp));
                });
                AstTask* ਅਚਲ ਤਿਆਰੀ_All_Taskp = get_Create_Constraint_ਤਿਆਰੀ_Func(nodep, m_memberMap);
                ਲਈ (ਪੂਰਨ d = max_ਡੂੰਘਾਈ; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ਅਚਲ classp, AstConstraint* ਅਚਲ constrp) {
                            ਜੇ (constraint_ਡੂੰਘਾਈ(constrp) != d) ਵਾਪਸ;
                            AstTask* ਅਚਲ taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ਅਚਲ ਤਿਆਰੀ_Task_Refp
                                = ਨਵਾਂ AstTaskRef{constrp->fileline(), taskp};
                            ਤਿਆਰੀ_Task_Refp->classOrPackagep(classp);
                            ਤਿਆਰੀ_All_Taskp->ਜੋੜੋ_Stmtsp(ਤਿਆਰੀ_Task_Refp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertion_ਸੈੱਟ<AstVar*> ਮਾਪ_Array_Vars;
            ਜੇ (nodep->extendsp()) {
                AstClass* ਮਾਪਾ_Classp = nodep->extendsp()->classp();
                ਜਦੋਂ (ਮਾਪਾ_Classp) {
                    ਅਚਲ ਸਵੈਚਾਲਿਤ ਮਾਪ_Arrays_It = m_ਮਾਪ_Constrained_Arrays.ਲੱਭੋ(ਮਾਪਾ_Classp);
                    ਜੇ (ਮਾਪ_Arrays_It != m_ਮਾਪ_Constrained_Arrays.ਸਿਰਾ()) {
                        ਲਈ (AstVar* ਅਚਲ arrVarp : ਮਾਪ_Arrays_It->ਦੂਜਾ) {
                            ਮਾਪ_Array_Vars.ਪਾਓ(arrVarp);
                        }
                    }
                    AstFunc* ਅਚਲ ਮਾਪਾ_Randomizep
                        = VN_CAST(m_memberMap.ਲੱਭੋ_Member(ਮਾਪਾ_Classp, "randomize"), Func);
                    ਜੇ (ਮਾਪਾ_Randomizep && ਮਾਪਾ_Randomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        ਲਈ (Ast_ਗੰਢ* stmtp = ਮਾਪਾ_Randomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ਤਾਰਕਿਕ ਮਿਲਿਆ_Clear_Constraints = ਝੂਠ;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                ਜੇ (methodp->ਵਿਧੀ() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->ਵਿਧੀ() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->ਜੋੜੋ_Stmtsp(stmtp->clone_ਰੁੱਖ(ਝੂਠ));
                                } ਨਹੀਂਤਾਂ ਜੇ (methodp->ਵਿਧੀ()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    ਮਿਲਿਆ_Clear_Constraints = ਸੱਚ;
                                }
                            });
                            ਜੇ (ਮਿਲਿਆ_Clear_Constraints) ਰੋਕੋ;
                        }
                    }
                    ਮਾਪਾ_Classp
                        = ਮਾਪਾ_Classp->extendsp() ? ਮਾਪਾ_Classp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ;
                }
            }
            randomizep->ਜੋੜੋ_Stmtsp(implementConstraintsClear(fl, genp));

            ਅਚਲ ਸਵੈਚਾਲਿਤ ਮਾਪ_Arrays_It = m_ਮਾਪ_Constrained_Arrays.ਲੱਭੋ(nodep);
            ਜੇ (ਮਾਪ_Arrays_It != m_ਮਾਪ_Constrained_Arrays.ਸਿਰਾ()) {
                ਲਈ (AstVar* ਅਚਲ arrVarp : ਮਾਪ_Arrays_It->ਦੂਜਾ) ਮਾਪ_Array_Vars.ਪਾਓ(arrVarp);
            }
            AstTask* ਅਚਲ ਤਿਆਰੀ_All_Taskp = get_Create_Constraint_ਤਿਆਰੀ_Func(nodep, m_memberMap);
            ਜੇ (!ਮਾਪ_Array_Vars.ਖਾਲੀ_ਥਾਵਾਂ() && prepareArraysTaskp->stmtsp()) {
                Ast_ਗੰਢ_Stmt* ਅਚਲ preparep = (ਨਵਾਂ AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                ਲਈ (Ast_ਗੰਢ* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    ਤਾਰਕਿਕ writesArray = ਝੂਠ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->ਵਿਧੀ() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    ਜੇ (!writesArray) ਜਾਰੀ;
                    stmtp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(preparep);
                    ਰੋਕੋ;
                }
            }
            AstTaskRef* ਅਚਲ ਤਿਆਰੀ_Task_Refp = ਨਵਾਂ AstTaskRef{fl, ਤਿਆਰੀ_All_Taskp};
            randomizep->ਜੋੜੋ_Stmtsp(ਤਿਆਰੀ_Task_Refp->makeStmt());

            Ast_ਗੰਢ_Module* ਅਚਲ genModp = VN_AS(genp->user2p(), ਗੰਢ_Module);

            AstCExpr* ਅਚਲ solverCallp = ਨਵਾਂ AstCExpr{fl};
            solverCallp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            solverCallp->ਜੋੜੋ(ਨਵਾਂ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->ਜੋੜੋ(".next(__Vm_rng)");
            ਜੇ (!ਮਾਪ_Array_Vars.ਖਾਲੀ_ਥਾਵਾਂ()) {
                AstVar* ਅਚਲ ਮਾਪ_Ok_Varp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ)};
                ਮਾਪ_Ok_Varp->funcLocal(ਸੱਚ);
                randomizep->ਜੋੜੋ_Stmtsp(ਮਾਪ_Ok_Varp);

                AstVar* ਅਚਲ finalOkVarp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ)};
                finalOkVarp->funcLocal(ਸੱਚ);
                randomizep->ਜੋੜੋ_Stmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                ਲਈ (AstVar* ਅਚਲ arrVarp : ਮਾਪ_Array_Vars)
                    ਜੋੜੋ_Frozen_ਮਾਪ_Pin(fl, arrVarp, randomizep, genp, rand_ਬਹੁਲਕ_Varp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->ਜੋੜੋ_Stmtsp(
                    ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, ਮਾਪ_Ok_Varp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                ਜੇ (AstTask* ਅਚਲ resizeAllTaskp
                    = VN_AS(m_memberMap.ਲੱਭੋ_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ਅਚਲ resizeTaskRefp = ਨਵਾਂ AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstVarRef{fl, ਮਾਪ_Ok_Varp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), ਖਾਲੀ_ਸੂਚਕ};
                    randomizep->ਜੋੜੋ_Stmtsp(ifp);
                }

                // Refresh array element tables after resize
                ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& arrVarp : ਮਾਪ_Array_Vars) {
                    ਜੋੜੋ_Array_Elem_Refresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->ਜੋੜੋ_Stmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ਅਚਲ ਤਿਆਰੀ_Task_Refp2 = ਨਵਾਂ AstTaskRef{fl, ਤਿਆਰੀ_All_Taskp};
                randomizep->ਜੋੜੋ_Stmtsp(ਤਿਆਰੀ_Task_Refp2->makeStmt());

                ਲਈ (AstVar* ਅਚਲ arrVarp : ਮਾਪ_Array_Vars) {
                    randomizep->ਜੋੜੋ_Stmtsp(new_Pin_ਮਾਪ_Variable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ਅਚਲ solverCallp2 = ਨਵਾਂ AstCExpr{fl};
                solverCallp2->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                solverCallp2->ਜੋੜੋ(ਨਵਾਂ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->ਜੋੜੋ(".next(__Vm_rng)");
                randomizep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    fl, ਨਵਾਂ AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstVarRef{fl, ਮਾਪ_Ok_Varp, VAccess::READ},
                                       ਨਵਾਂ AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } ਨਹੀਂਤਾਂ {
                beginValp = solverCallp;
            }
            ਜੇ (rand_ਬਹੁਲਕ_Varp) {
                Ast_ਗੰਢ_Module* ਅਚਲ rand_ਬਹੁਲਕ_Classp = VN_AS(rand_ਬਹੁਲਕ_Varp->user2p(), Class);
                Ast_ਗੰਢ_FTask* ਅਚਲ newp
                    = VN_AS(m_memberMap.ਲੱਭੋ_Member(rand_ਬਹੁਲਕ_Classp, "new"), ਗੰਢ_FTask);
                UASSERT_OBJ(newp, rand_ਬਹੁਲਕ_Classp, "No new() in class");
                ਜੋੜੋ_ਸੈੱਟ_Rand_ਬਹੁਲਕ(newp, genp, rand_ਬਹੁਲਕ_Varp);
            }
            ਜੇ (AstVar* ਅਚਲ static_Rand_ਬਹੁਲਕ_Varp = get_Static_Rand_ਬਹੁਲਕ_Var(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_ਗੰਢ_FTask* ਅਚਲ newp = VN_AS(m_memberMap.ਲੱਭੋ_Member(nodep, "new"), ਗੰਢ_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                ਜੋੜੋ_ਸੈੱਟ_Static_Rand_ਬਹੁਲਕ(newp, genp, static_Rand_ਬਹੁਲਕ_Varp);
            }
        } ਨਹੀਂਤਾਂ {
            beginValp = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 32, 1};
        }

        ਜੋੜੋ_Update_Rand_Vars_Body(nodep, randomizep);

        AstFunc* ਅਚਲ basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        ਜੋੜੋ_Basic_Randomize_Body(basicRandomizep, nodep, rand_ਬਹੁਲਕ_Varp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ਅਚਲ fvarRefp = ਨਵਾਂ AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            fl, fvarRefp, basic_ਪਹਿਲਾ ? ਨਵਾਂ AstFuncRef{fl, basicRandomizep} : beginValp});

        ਅਚਲ ਸਵੈਚਾਲਿਤ ਮਾਪ_Arrays_It = m_ਮਾਪ_Constrained_Arrays.ਲੱਭੋ(nodep);
        ਅਚਲ ਤਾਰਕਿਕ needs_ਮਾਪ_Phase
            = ਮਾਪ_Arrays_It != m_ਮਾਪ_Constrained_Arrays.ਸਿਰਾ() && !ਮਾਪ_Arrays_It->ਦੂਜਾ.ਖਾਲੀ_ਥਾਵਾਂ();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ਅਚਲ ਸਵੈਚਾਲਿਤ emitResizeFallback = [&]() {
            ਜੇ (needs_ਮਾਪ_Phase) ਵਾਪਸ;
            ਜੇ (AstTask* ਅਚਲ resizeAllTaskp
                = VN_AS(m_memberMap.ਲੱਭੋ_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ਅਚਲ resizeTaskRefp = ਨਵਾਂ AstTaskRef{fl, resizeAllTaskp};
                randomizep->ਜੋੜੋ_Stmtsp(resizeTaskRefp->makeStmt());
            }
        };
        ਜੇ (!basic_ਪਹਿਲਾ) emitResizeFallback();

        AstVarRef* ਅਚਲ fvarRefReadp = fvarRefp->clone_ਰੁੱਖ(ਝੂਠ);
        fvarRefReadp->access(VAccess::READ);

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਦੂਜਾ_Halfp
            = basic_ਪਹਿਲਾ ? beginValp : ਨਵਾਂ AstFuncRef{fl, basicRandomizep};
        randomizep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, fvarRefp->clone_ਰੁੱਖ(ਝੂਠ),
                                            ਨਵਾਂ AstAnd{fl, fvarRefReadp, ਦੂਜਾ_Halfp}});

        ਜੇ (basic_ਪਹਿਲਾ) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ਜੇ (classHasRandClassMembers(nodep)) {
            AstTask* ਅਚਲ postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->ਜੋੜੋ_Stmtsp((ਨਵਾਂ AstTaskRef{fl, postTaskp})->makeStmt());
        }

        ਜੋੜੋ_Pre_Post_Call(nodep, randomizep, "post_randomize");
        nodep->user1(ਝੂਠ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRandCase* nodep) override {
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
        Ast_ਗੰਢ_DType* ਅਚਲ ਜੋੜ_DTypep = nodep->ਲੱਭੋ_UInt64_DType();

        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਅਚਲ std::string name = "__Vrandcase" + cvt_ਮੰਜ਼ਿਲ_Str(m_rand_Case_ਅੰਕ++);
        AstVar* ਅਚਲ randVarp = ਨਵਾਂ AstVar{fl, VVarType::BLOCKTEMP, name, ਜੋੜ_DTypep};
        randVarp->noSubst(ਸੱਚ);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ਜੋੜੋ_ਆਰਜ਼ੀ_Varsp(randVarp);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* sump = ਨਵਾਂ AstConst{fl, AstConst::Widthed_ਮੁੱਲ{}, 64, 0};
        Ast_ਗੰਢ_If* ਅਚਲ ਪਹਿਲਾ_Ifsp
            = ਨਵਾਂ AstIf{fl, ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}}, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        Ast_ਗੰਢ_If* ifsp = ਪਹਿਲਾ_Ifsp;

        ਲਈ (Ast_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = itemp->condsp()->unlink_Fr_ਪਿੱਠ();
            sump
                = ਨਵਾਂ Ast_ਜੋੜੋ{condp->fileline(), sump, ਨਵਾਂ AstExtend{itemp->fileline(), condp, 64}};
            Ast_ਗੰਢ* ਅਚਲ stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ() : ਖਾਲੀ_ਸੂਚਕ;
            AstVarRef* ਅਚਲ randVarRefp = ਨਵਾਂ AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_ਗੰਢ_If* ਅਚਲ newifp
                = ਨਵਾਂ AstIf{itemp->fileline(),
                            ਨਵਾਂ AstLte{condp->fileline(), randVarRefp, sump->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)},
                            stmtsp, ਖਾਲੀ_ਸੂਚਕ};
            ifsp->ਜੋੜੋ_Elsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ਅਚਲ dispp = ਨਵਾਂ AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->ਜੋੜੋ_Elsesp(dispp);

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ randp = ਨਵਾਂ AstRand{fl, ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
        randp->dtype_ਸੈੱਟ_UInt64();
        AstVarRef* ਅਚਲ randVarRefp = ਨਵਾਂ AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_ਗੰਢ* ਅਚਲ newp
            = ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, randVarRefp,
                            ਨਵਾਂ Ast_ਜੋੜੋ{fl, ਨਵਾਂ AstConst{fl, AstConst::Unsized64{}, 1},
                                       ਨਵਾਂ AstModDiv{fl, randp, sump}}};
        newp->ਜੋੜੋ_ਅਗਲਾ(ਪਹਿਲਾ_Ifsp);
        ਜੇ (debug() >= 9) newp->dump_ਰੁੱਖ_And_ਅਗਲਾ(ਬਾਹਰ, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (nodep->name() == "rand_mode") {
            Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(nodep, ਵਿਧੀ_Call);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = ਵਿਧੀ_Callp ? ਵਿਧੀ_Callp->fromp() : ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ Rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ = Rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ::get(fromp, m_modp);
            UASSERT_OBJ(rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ਅਚਲ receiverp = rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.receiverp;
            ਅਚਲ ਤਾਰਕਿਕ isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_ਗੰਢ* classLevelStaticLoopp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (isClassLevel && nodep->argsp()) {
                ਜੇ (AstVar* ਅਚਲ sVarp = get_Static_Rand_ਬਹੁਲਕ_Var(rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.classp)) {
                    File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ staticLhsp
                        = make_ਬਹੁਲਕ_ਸੌਂਪੋ_Lhs(fl, rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.classp, ਖਾਲੀ_ਸੂਚਕ, sVarp);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ argClonep = nodep->argsp()->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    classLevelStaticLoopp = make_ਬਹੁਲਕ_ਸੈੱਟ_Loop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ਅਚਲ ਤਾਰਕਿਕ receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Varp = receiverIsStaticRand
                                             ? get_Static_Rand_ਬਹੁਲਕ_Var(rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.classp)
                                             : get_Rand_ਬਹੁਲਕ_Var_From_Class(rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.classp);
            ਜੇ (!rand_ਬਹੁਲਕ_Varp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(ਧੱਕੋ_Deletep(m_stmtp), m_stmtp = ਖਾਲੀ_ਸੂਚਕ);
                ਵਾਪਸ;
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = make_ਬਹੁਲਕ_ਸੌਂਪੋ_Lhs(nodep->fileline(), rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.classp,
                                                        rand_ਬਹੁਲਕ_ਨਿਸ਼ਾਨਾ.fromp, rand_ਬਹੁਲਕ_Varp);
            replace_With_ਬਹੁਲਕ_ਸੌਂਪੋ_And_Append(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : ਖਾਲੀ_ਸੂਚਕ, lhsp,
                classLevelStaticLoopp);
            ਵਾਪਸ;
        }

        ਜੇ (nodep->name() == "constraint_mode") {
            Ast_ਵਿਧੀ_Call* ਅਚਲ ਵਿਧੀ_Callp = VN_CAST(nodep, ਵਿਧੀ_Call);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp = ਵਿਧੀ_Callp ? ਵਿਧੀ_Callp->fromp() : ਖਾਲੀ_ਸੂਚਕ;
            AstConstraint* constrp = ਖਾਲੀ_ਸੂਚਕ;
            AstClass* classp = VN_CAST(m_modp, Class);
            ਜੇ (AstConstraintRef* ਅਚਲ constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                ਜੇ (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } ਨਹੀਂਤਾਂ ਜੇ (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ਅਚਲ constraint_ਬਹੁਲਕ_Varp = (constrp && constrp->isStatic())
                                                   ? get_Static_Constraint_ਬਹੁਲਕ_Var(classp)
                                                   : get_Constraint_ਬਹੁਲਕ_Var(classp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp
                = make_ਬਹੁਲਕ_ਸੌਂਪੋ_Lhs(nodep->fileline(), classp, fromp, constraint_ਬਹੁਲਕ_Varp);
            replace_With_ਬਹੁਲਕ_ਸੌਂਪੋ(nodep, constrp, lhsp);
            ਵਾਪਸ;
        }

        ਜੇ (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            ਵਾਪਸ;
        }

        ਜੇ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Static_Context = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ਅਚਲ stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ਅਚਲ randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            ਜੇ (ਵਿੱਚ_Static_Context) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(ਸੱਚ);
            }
            randomizeFuncp->ਜੋੜੋ_Stmtsp(
                ਨਵਾਂ Ast_ਸੌਂਪੋ{nodep->fileline(),
                              ਨਵਾਂ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            ਪੂਰਨ argn = 0;
            AstWith* ਅਚਲ withp = nodep->withp();
            ਲਈ (ਅਚਲ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = argp->exprp();
                AstCMethodHard* ਅਚਲ basicMethodp = ਨਵਾਂ AstCMethodHard{
                    nodep->fileline(),
                    ਨਵਾਂ AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ਅਚਲ refvarp
                    = ਨਵਾਂ AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::ਮੰਜ਼ਿਲ_string(++argn), exprp->dtypep()};
                refvarp->ਸੈੱਟ_Std_Randomize_Arg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                ਜੇ (withp) {
                    withp->foreach([&](Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਘਾਤਾਂਕ) {
                        ਜੇ (is_Similar_ਗੰਢ(ਘਾਤਾਂਕ, exprp)) {
                            AstVarRef* ਅਚਲ replaceVar
                                = ਨਵਾਂ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            ਘਾਤਾਂਕ->replaceWith(replaceVar);
                            replaceVar->user1(ਘਾਤਾਂਕ->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਘਾਤਾਂਕ), ਘਾਤਾਂਕ);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(ਸੱਚ);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->ਜੋੜੋ_Stmtsp(refvarp);

                ਅਚਲ ਮਾਪ_t ਚੌੜਾਈ = exprp->ਚੌੜਾਈ();
                basicMethodp->ਜੋੜੋ_Pinsp(
                    ਨਵਾਂ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->ਜੋੜੋ_Pinsp(
                    ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Unsized64{}, ਚੌੜਾਈ});
                basicMethodp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();

                randomizeFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                    nodep->fileline(),
                    ਨਵਾਂ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    ਨਵਾਂ AstAnd{nodep->fileline(),
                               ਨਵਾਂ AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            ਜੇ (withp) {
                File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, ਖਾਲੀ_ਸੂਚਕ,
                                                                ਵਿੱਚ_Static_Context);
                withCapturep->ਜੋੜੋ_Function_Arguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                ਜੇ (stdrand) {
                    randomizeFuncp->ਜੋੜੋ_Stmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_ਗੰਢ* ਅਚਲ capturedTreep = withp->exprp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                randomizeFuncp->ਜੋੜੋ_Stmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_ਵਿਅੰਜਕ_Visitor{
                        ਖਾਲੀ_ਸੂਚਕ, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        ਖਾਲੀ_ਸੂਚਕ, m_writtenVars, &m_uniqueConstraintId, m_unique_ਨਾਂ,  ਖਾਲੀ_ਸੂਚਕ};
                }
                AstCExpr* ਅਚਲ solverCallp = ਨਵਾਂ AstCExpr{fl};
                solverCallp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                solverCallp->ਜੋੜੋ(ਨਵਾਂ AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->ਜੋੜੋ(".next()");
                AstVar* ਅਚਲ fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ andExprp
                    = ਨਵਾਂ AstAnd{fl, ਨਵਾਂ AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->ਜੋੜੋ_Stmtsp(
                    ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            ਜੇ (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            ਜੇ (withCapturep) nodep->ਜੋੜੋ_Argsp(withCapturep->get_ਦਲੀਲਾਂ());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            ਵਾਪਸ;
        }
        handle_Randomize_ਦਲੀਲਾਂ(nodep);

        AstWith* ਅਚਲ withp = nodep->withp();
        ਜੇ (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            ਵਾਪਸ;
        }
        withp->unlink_Fr_ਪਿੱਠ();
        iterateChildren(nodep);

        AstClass* classp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (Ast_ਵਿਧੀ_Call* ਅਚਲ callp = VN_CAST(nodep, ਵਿਧੀ_Call)) {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            ਅਚਲ AstClassRefDType* ਅਚਲ classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } ਨਹੀਂਤਾਂ {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        ਜੇ (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ਅਚਲ classGenp = getRandomGenerator(classp);
        AstVar* ਅਚਲ localGenp
            = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(ਸੱਚ);

        AstFunc* ਅਚਲ randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inline_Unique_ਨਾਂ.get(nodep), ਝੂਠ);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ਅਚਲ std::pair<ਤਾਰਕਿਕ, ਤਾਰਕਿਕ> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<ਤਾਰਕਿਕ, ਤਾਰਕਿਕ>{ਝੂਠ, ਝੂਠ};
        ਜੇ (prePostWrap.ਪਹਿਲਾ) {
            ਜੋੜੋ_Virtual_Pre_Post_Call(randomizeFuncp, classp, "pre_randomize");
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_Pre_Post_Call(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ਜੇ (classHasRandClassMembers(classp)) {
            AstTask* ਅਚਲ preTaskp = getCreateNestedCallbackTask(classp, "pre");
            ਜੇ (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->ਜੋੜੋ_Stmtsp((ਨਵਾਂ AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ਅਚਲ CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.ਜੋੜੋ_Function_Arguments(randomizeFuncp);

        // Add constraints clearing code
        ਜੇ (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ਅਚਲ mainRandomizep
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(classp, "randomize"), Func);
            ਜੇ (mainRandomizep && mainRandomizep->stmtsp()) {
                ਲਈ (Ast_ਗੰਢ* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ਤਾਰਕਿਕ ਮਿਲਿਆ_Clear_Constraints = ਝੂਠ;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        ਜੇ (methodp->ਵਿਧੀ() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->ਵਿਧੀ() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->ਜੋੜੋ_Stmtsp(stmtp->clone_ਰੁੱਖ(ਝੂਠ));
                        } ਨਹੀਂਤਾਂ ਜੇ (methodp->ਵਿਧੀ() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            ਮਿਲਿਆ_Clear_Constraints = ਸੱਚ;
                        }
                    });
                    ਜੇ (ਮਿਲਿਆ_Clear_Constraints) ਰੋਕੋ;
                }
            }
            randomizeFuncp->ਜੋੜੋ_Stmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->ਜੋੜੋ_Stmtsp(localGenp);

        AstFunc* ਅਚਲ basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ਅਚਲ basicRandomizeFuncCallp
            = ਨਵਾਂ AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        ਜੇ (classGenp) {
            AstTask* ਅਚਲ constr_ਤਿਆਰੀ_Funcp = get_Create_Constraint_ਤਿਆਰੀ_Func(classp, m_memberMap);
            AstTaskRef* ਅਚਲ callp = ਨਵਾਂ AstTaskRef{nodep->fileline(), constr_ਤਿਆਰੀ_Funcp};
            randomizeFuncp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
            randomizeFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
                nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                ਨਵਾਂ AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), ਗੰਢ_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ਅਚਲ rand_ਬਹੁਲਕ_Varp = get_Rand_ਬਹੁਲਕ_Var_From_Class(classp);
        ਜੇ (!classGenp && rand_ਬਹੁਲਕ_Varp) ਜੋੜੋ_ਸੈੱਟ_Rand_ਬਹੁਲਕ(randomizeFuncp, localGenp, rand_ਬਹੁਲਕ_Varp);
        ਜੇ (!classGenp) {
            ਜੇ (AstVar* ਅਚਲ sVarp = get_Static_Rand_ਬਹੁਲਕ_Var(classp)) {
                ਜੋੜੋ_ਸੈੱਟ_Static_Rand_ਬਹੁਲਕ(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_ਗੰਢ* ਅਚਲ capturedTreep = withp->exprp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        randomizeFuncp->ਜੋੜੋ_Stmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_ਗੰਢ* inlineResizeStmtsp = ਖਾਲੀ_ਸੂਚਕ;
        {
            std::ਸਦਿਸ਼<AstCMethodHard*> ਮਾਪ_Methodps;
            capturedTreep->foreach_And_ਅਗਲਾ([&](AstCMethodHard* methodp) {
                ਜੇ (methodp->ਵਿਧੀ() == VCMethod::DYN_ਮਾਪ
                    || methodp->ਵਿਧੀ() == VCMethod::ASSOC_ਮਾਪ) {
                    ਮਾਪ_Methodps.ਧੱਕੋ_ਪਿੱਠ(methodp);
                }
            });
            ਲਈ (AstCMethodHard* ਅਚਲ methodp : ਮਾਪ_Methodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = ਖਾਲੀ_ਸੂਚਕ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਜੜ੍ਹ_Exprp = methodp->fromp();
                ਜੇ (AstMemberSel* memberSelp = VN_CAST(ਜੜ੍ਹ_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    ਜਦੋਂ (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = memberSelp->fromp();
                    ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ varRefp = VN_CAST(fromp, VarRef)) {
                        ਜੇ (varRefp->varp() == captured.getThisp()) ਜਾਰੀ;
                    } ਨਹੀਂਤਾਂ {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ varRefp = VN_CAST(ਜੜ੍ਹ_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                ਜੇ (!arrVarp) ਜਾਰੀ;
                // Only handle rand-declared dynamic/assoc array variables
                ਜੇ (!arrVarp->rand().isRandomizable()) ਜਾਰੀ;
                // Do not randomize associative array size (IEEE 18.4)
                ਜੇ (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) ਜਾਰੀ;
                File_ਸਤਰ* ਅਚਲ fl = methodp->fileline();
                ਤਾਰਕਿਕ wasCreated = ਝੂਠ;
                AstClass* ਅਚਲ varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ਅਚਲ ਮਾਪ_Varp = create_Or_Get_ਮਾਪ_Var(varClassp, arrVarp, fl,
                                                            methodp->ਲੱਭੋ_Int_DType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                ਜੇ (wasCreated) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ resizeVarRefp = buildMemberSelExprp(ਜੜ੍ਹ_Exprp, arrVarp);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮਾਪ_Exprp = buildMemberSelExprp(ਜੜ੍ਹ_Exprp, ਮਾਪ_Varp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->ਜੋੜੋ_ਅਗਲਾ(create_ਮਾਪ_Gte_Zero_Constraint(fl, ਮਾਪ_Varp, ਜੜ੍ਹ_Exprp));
                    AstCMethodHard* ਅਚਲ resizep
                        = ਨਵਾਂ AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, ਮਾਪ_Exprp};
                    resizep->dtypep(methodp->ਲੱਭੋ_Void_DType());
                    inlineResizeStmtsp
                        = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(inlineResizeStmtsp, ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{fl, resizep});
                }
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਮਾਪ_ਗੰਢ_Exprp = buildMemberSelExprp(ਜੜ੍ਹ_Exprp, ਮਾਪ_Varp);
                methodp->replaceWith(ਮਾਪ_ਗੰਢ_Exprp);
                VL_DO_DANGLING(methodp->delete_ਰੁੱਖ(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_ਵਿਅੰਜਕ_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                rand_ਬਹੁਲਕ_Varp, m_writtenVars, &m_uniqueConstraintId, m_unique_ਨਾਂ,  ਖਾਲੀ_ਸੂਚਕ};
        }

        // Call the solver and set return value
        AstCExpr* ਅਚਲ solverCallp = ਨਵਾਂ AstCExpr{nodep->fileline()};
        solverCallp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        solverCallp->ਜੋੜੋ(ਨਵਾਂ AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->ਜੋੜੋ(".next(__Vm_rng)");
        randomizeFuncp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(),
            ਨਵਾਂ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            ਨਵਾਂ AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        ਜੇ (inlineResizeStmtsp) {
            AstVar* ਅਚਲ retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{nodep->fileline(),
                                         ਨਵਾਂ AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, ਖਾਲੀ_ਸੂਚਕ};
            randomizeFuncp->ਜੋੜੋ_Stmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ਜੇ (classHasRandClassMembers(classp)) {
            AstTask* ਅਚਲ postTaskp = getCreateNestedCallbackTask(classp, "post");
            ਜੇ (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->ਜੋੜੋ_Stmtsp((ਨਵਾਂ AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        ਜੇ (prePostWrap.ਦੂਜਾ) {
            ਜੋੜੋ_Virtual_Pre_Post_Call(randomizeFuncp, classp, "post_randomize");
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_Pre_Post_Call(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->ਜੋੜੋ_Argsp(captured.get_ਦਲੀਲਾਂ());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_ਰੁੱਖ(), withp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਜੇ (m_constraintp && nodep->fromp()->user1()
            && (nodep->ਵਿਧੀ() == VCMethod::ASSOC_ਮਾਪ
                || nodep->ਵਿਧੀ() == VCMethod::DYN_ਮਾਪ)) {
            AstClass* ਅਚਲ classp = VN_AS(m_modp, Class);
            AstVarRef* ਅਚਲ ਕਤਾਰ_Var_Refp = VN_CAST(nodep->fromp(), VarRef);
            ਜੇ (!ਕਤਾਰ_Var_Refp) {
                // Warning from ConstraintExprVisitor will be thrown
                ਵਾਪਸ;
            }
            AstVar* ਅਚਲ ਕਤਾਰ_Varp = ਕਤਾਰ_Var_Refp->varp();
            ਤਾਰਕਿਕ wasCreated = ਝੂਠ;
            AstVar* ਅਚਲ ਮਾਪ_Varp
                = create_Or_Get_ਮਾਪ_Var(classp, ਕਤਾਰ_Varp, fl, nodep->ਲੱਭੋ_Int_DType(), wasCreated);
            ਜੇ (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                ਜੇ (!VN_IS(ਕਤਾਰ_Varp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    ਜੇ (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ਅਚਲ ਮਾਪ_Var_Refp = ਨਵਾਂ AstVarRef{
                        fl, VN_AS(ਮਾਪ_Varp->user2p(), ਗੰਢ_Module), ਮਾਪ_Varp, VAccess::READ};
                    AstCMethodHard* ਅਚਲ resizep = ਨਵਾਂ AstCMethodHard{
                        fl, nodep->fromp()->unlink_Fr_ਪਿੱਠ(), VCMethod::DYN_RESIZE, ਮਾਪ_Var_Refp};
                    resizep->dtypep(nodep->ਲੱਭੋ_Void_DType());
                    resizerTaskp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_Stmt_ਵਿਅੰਜਕ{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->ਜੋੜੋ_Itemsp(create_ਮਾਪ_Gte_Zero_Constraint(fl, ਮਾਪ_Varp));
            }
            AstVarRef* ਅਚਲ ਮਾਪ_Var_Refp = ਨਵਾਂ AstVarRef{fl, VN_AS(ਮਾਪ_Varp->user2p(), ਗੰਢ_Module),
                                                         ਮਾਪ_Varp, VAccess::READ};
            ਮਾਪ_Var_Refp->user1(ਸੱਚ);
            nodep->replaceWith(ਮਾਪ_Var_Refp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ RandomizeVisitor(AstNetlist* nodep)
        : m_inline_Unique_ਨਾਂ{"__Vrandwith"} {
        createRandomizeClassVars(nodep);
        // Flag local constraint leaves as solver-owned so __VBasicRand skips them.
        // Runs before any class is lowered. Only a randomized class counts, and
        // only a leaf owned by the constraint's own class: a leaf reached through
        // a global constraint stays basic-randomized so a standalone randomize()
        // of its type still randomizes it (issue #7833); a class-typed sub-object
        // is skipped so its own members are still basic-randomized.
        nodep->foreach([&](AstClass* ਅਚਲ classp) {
            ਜੇ (!classp->user1()) ਵਾਪਸ;
            classp->foreachMember([&](AstClass* ਅਚਲ ownerClassp, AstConstraint* ਅਚਲ constrp) {
                constrp->foreach([&](AstMemberSel* ਅਚਲ memberSelp) {
                    AstVar* ਅਚਲ varp = memberSelp->varp();
                    ਜੇ (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) ਵਾਪਸ;
                    ਜੇ (VN_AS(varp->user2p(), ਗੰਢ_Module) != ownerClassp) ਵਾਪਸ;
                    ਜੇ (!varp->user3()) varp->user3(ਸੱਚ);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(constrp->unlink_Fr_ਪਿੱਠ()), constrp);
        });
    }
    ~RandomizeVisitor() override = ਮੂਲ;
};

//######################################################################
// Randomize method class functions

ਖਾਲੀ V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ਅਚਲ RandomizeMarkVisitor markVisitor{nodep};
        ਅਚਲ RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_ਰੁੱਖ("randomize", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ਅਚਲ std::string& name, ਤਾਰਕਿਕ allowVirtual,
                                       ਤਾਰਕਿਕ ਬੱਚਾ_DType) {
    AstFunc* funcp = VN_AS(memberMap.ਲੱਭੋ_Member(nodep, name), Func);
    ਜੇ (!funcp) {
        v3Global.useRandomizeMethods(ਸੱਚ);
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep
            = ਬੱਚਾ_DType
                  ? ਨਵਾਂ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ਅਚਲ fvarp = ਬੱਚਾ_DType
                                  ? ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlag_ਬੱਚਾ_DType{}, dtypep}
                                  : ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3_Error_ਕੋਡ::NORETURN, ਸੱਚ);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(ਸੱਚ);
        fvarp->funcReturn(ਸੱਚ);
        fvarp->direction(VDirection::OUTPUT);
        nodep->ਜੋੜੋ_Membersp(funcp);
        funcp = ਨਵਾਂ AstFunc{nodep->fileline(), name, ਖਾਲੀ_ਸੂਚਕ, fvarp};
        ਜੇ (!ਬੱਚਾ_DType) funcp->dtypep(dtypep);
        funcp->class_ਵਿਧੀ(ਸੱਚ);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->ਜੋੜੋ_Membersp(funcp);
        memberMap.ਪਾਓ(nodep, funcp);
    }
    ਵਾਪਸ funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_ਗੰਢ_Module* nodep, ਅਚਲ std::string& name) {
    AstFunc* funcp = ਖਾਲੀ_ਸੂਚਕ;
    v3Global.useRandomizeMethods(ਸੱਚ);
    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(32, 32, VSigning::SIGNED);
    AstVar* ਅਚਲ fvarp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(ਸੱਚ);
    fvarp->funcReturn(ਸੱਚ);
    fvarp->direction(VDirection::OUTPUT);
    funcp = ਨਵਾਂ AstFunc{nodep->fileline(), name, ਖਾਲੀ_ਸੂਚਕ, fvarp};
    funcp->dtypep(dtypep);
    ਜੇ (VN_IS(nodep, Class)) {
        funcp->class_ਵਿਧੀ(ਸੱਚ);
    } ਨਹੀਂਤਾਂ {
        funcp->class_ਵਿਧੀ(ਸੱਚ);
        funcp->isStatic(ਸੱਚ);
    }
    nodep->ਜੋੜੋ_Stmtsp(funcp);
    ਵਾਪਸ funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ਅਚਲ basep = nodep->ਅਧਾਰ_Most_Classp();
    AstFunc* funcp = VN_AS(memberMap.ਲੱਭੋ_Member(basep, "srandom"), Func);
    ਜੇ (!funcp) {
        v3Global.useRandomizeMethods(ਸੱਚ);
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep
            = basep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ਅਚਲ ivarp = ਨਵਾਂ AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(ਸੱਚ);
        ivarp->direction(VDirection::ਨਿਵੇਸ਼);
        funcp = ਨਵਾਂ AstFunc{basep->fileline(), "srandom", ivarp, ਖਾਲੀ_ਸੂਚਕ};
        funcp->dtypep(basep->ਲੱਭੋ_Void_DType());
        funcp->class_ਵਿਧੀ(ਸੱਚ);
        funcp->isVirtual(ਝੂਠ);
        basep->ਜੋੜੋ_Membersp(funcp);
        memberMap.ਪਾਓ(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        ਜੇ (basep->name() == "process") {
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } ਨਹੀਂਤਾਂ {
            funcp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(ਸੱਚ);
        }
    }
    ਵਾਪਸ funcp;
}
