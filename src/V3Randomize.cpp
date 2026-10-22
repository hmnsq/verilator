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

ಎಣಿಕೆ ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
ಸ್ಥಿರ AstVar* get_Rand_ಬಹುಲಕ_Var_From_Class(Ast_ಗಂಟು_Module* classp) {
    ಆಗ (classp) {
        ಆದರೆ (classp->user2p()) ಹಿಂತಿರುಗಿ VN_AS(classp->user2p(), Var);
        AstClass* ಸ್ಥಿರಾಂಕ cp = VN_CAST(classp, Class);
        ಆದರೆ (!cp || !cp->extendsp()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        classp = cp->extendsp()->classp();
    }
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
ಸ್ಥಿರ AstVar* getRandomGenerator(AstClass* ಸ್ಥಿರಾಂಕ classp) {
    ಆದರೆ (classp->user3p()) ಹಿಂತಿರುಗಿ VN_AS(classp->user3p(), Var);
    ಆದರೆ (classp->extendsp()) ಹಿಂತಿರುಗಿ getRandomGenerator(classp->extendsp()->classp());
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
ಸ್ಥಿರ AstTask* get_Create_Constraint_ಸಿದ್ಧತೆ_Func(AstClass* classp, VMemberMap& memberMap) {
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ name = "__Vsetup_constraints";
    AstTask* ಸಿದ್ಧತೆ_All_Taskp = VN_AS(memberMap.ಕಂಡುಹಿಡಿ_Member(classp, name), Task);
    ಆದರೆ (ಸಿದ್ಧತೆ_All_Taskp) ಹಿಂತಿರುಗಿ ಸಿದ್ಧತೆ_All_Taskp;
    ಸಿದ್ಧತೆ_All_Taskp = ಹೊಸ AstTask{classp->fileline(), "__Vsetup_constraints", ಶೂನ್ಯಸೂಚಕ};
    ಸಿದ್ಧತೆ_All_Taskp->class_ವಿಧಾನ(ನಿಜ);
    ಸಿದ್ಧತೆ_All_Taskp->isVirtual(ನಿಜ);
    classp->ಕೂಡಿಸು_Membersp(ಸಿದ್ಧತೆ_All_Taskp);
    memberMap.ಸೇರಿಸು(classp, ಸಿದ್ಧತೆ_All_Taskp);
    ಹಿಂತಿರುಗಿ ಸಿದ್ಧತೆ_All_Taskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

ರಚನೆ Rand_ಬಹುಲಕ_ಗುರಿ final {
    // MEMBERS
    AstVar* ಸ್ಥಿರಾಂಕ receiverp;  // Variable that is the target of the rand_mode() call
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp;  // Expression from which the rand_mode() call originates
    AstClass* ಸ್ಥಿರಾಂಕ classp;  // Class in which rand_mode should be set

    // METHODS
    ಸ್ಥಿರ Rand_ಬಹುಲಕ_ಗುರಿ get(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_Module* modp) {
        ಆದರೆ (!fromp) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ, VN_AS(modp, Class)};
        ಆದರೆ (AstCMethodHard* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Hardp = VN_CAST(fromp, CMethodHard)) {
            ಹಿಂತಿರುಗಿ Rand_ಬಹುಲಕ_ಗುರಿ::get(ವಿಧಾನ_Hardp->fromp(), modp);
        }
        AstVar* receiverp = ಶೂನ್ಯಸೂಚಕ;
        AstClass* classp = VN_CAST(modp, Class);
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            ಆದರೆ (receiverp->isRand()) {
                fromp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            ಆದರೆ (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        ಆದರೆ (!receiverp->isRand()) {
            AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            ಆದರೆ (classRefDtp) classp = classRefDtp->classp();
        }
        ಹಿಂತಿರುಗಿ {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

ಒಕ್ಕೂಟ Randomize_ಬಹುಲಕ final {
    // MEMBERS
    ರಚನೆ {
        ತಾರ್ಕಿಕ ಬಳಸುತ್ತದೆ_ಬಹುಲಕ : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t ಸೂಚ್ಯಂಕ : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
ಸ್ಥಿರ Ast_ಗಂಟು_DType* arrayElementDTypep(Ast_ಗಂಟು_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ಆಗ (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    ಹಿಂತಿರುಗಿ dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
ಸ್ಥಿರ ತಾರ್ಕಿಕ dtype_ಒಳಗೊಂಡಿದೆ_Real(Ast_ಗಂಟು_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subp = dtypep->subDTypep()) ಹಿಂತಿರುಗಿ dtype_ಒಳಗೊಂಡಿದೆ_Real(subp);
    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ structp = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
        ಆದರೆ (structp->packed()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಗಾಗಿ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            ಆದರೆ (dtype_ಒಳಗೊಂಡಿದೆ_Real(memberp->subDTypep())) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(dtypep, BasicDType);
    ಹಿಂತಿರುಗಿ basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
ಸ್ಥಿರ ಶೂನ್ಯ checkRandTypeEligibility(Ast_ಗಂಟು* contextp, Ast_ಗಂಟು_DType* dtypep, ತಾರ್ಕಿಕ isRandc) {
    dtypep = dtypep->skipRefp();
    ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        ಹಿಂತಿರುಗಿ;
    }
    ಆದರೆ (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        ಆದರೆ (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        ಹಿಂತಿರುಗಿ;
    }
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        ಹಿಂತಿರುಗಿ;
    }
    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_UOr_Struct_DType* ಸ್ಥಿರಾಂಕ structp = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
        ಆದರೆ (structp->packed()) ಹಿಂತಿರುಗಿ;  // Integral by construction; members checked separately
        ಆದರೆ (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            ಹಿಂತಿರುಗಿ;
        }
        ಗಾಗಿ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            ಆದರೆ (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    ಆದರೆ (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        ಆದರೆ (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        ಹಿಂತಿರುಗಿ;
    }
    ಆಯ್ಕೆ (basicp->keyword()) {
    ಸ್ಥಿತಿ VBasicDTypeKwd::STRING:
    ಸ್ಥಿತಿ VBasicDTypeKwd::CHANDLE:
    ಸ್ಥಿತಿ VBasicDTypeKwd::EVENT: {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        ನಿಲ್ಲಿಸು;
    }
    ಪೂರ್ವನಿಯೋಜಿತ: ನಿಲ್ಲಿಸು;
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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    ಬಳಸಿ Derived_ಗಣ = std::unordered_ಗಣ<AstClass*>;
    ಬಳಸಿ ತಳ_ಗಮ್ಯ_Derived_Map = std::unordered_map<ಸ್ಥಿರಾಂಕ AstClass*, Derived_ಗಣ>;

    ತಳ_ಗಮ್ಯ_Derived_Map m_ತಳ_ಗಮ್ಯ_Derived_Map;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;  // Current class
    Ast_ಗಂಟು* m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp = ಶೂನ್ಯಸೂಚಕ;  // Current constraint or constraint if expression
    Ast_ಗಂಟು_Module* m_modp;  // Current module
    Ast_ಗಂಟು_Stmt* m_stmtp = ಶೂನ್ಯಸೂಚಕ;  // Current statement
    Ast_ಗಂಟು_FTask_Ref* m_stdRandCallp = ಶೂನ್ಯಸೂಚಕ;  // Current std::randomize() call
    ತಾರ್ಕಿಕ m_ಒಳ_Std_With = ಸುಳ್ಳು;  // True when inside a 'with {}' clause
    std::ಗಣ<Ast_ಗಂಟು_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = ಶೂನ್ಯಸೂಚಕ;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    ಶೂನ್ಯ markGlobalConstrainedVars(AstClass* classp) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ) {
            ಗಾಗಿ (Ast_ಗಂಟು* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(memberp, Var);
                ಆದರೆ (!varp) ಮುಂದುವರಿ;
                ಆದರೆ (varp->rand().isRandomizable()) varp->globalConstrained(ನಿಜ);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    ತಾರ್ಕಿಕ is_Var_ಒಳ_Std_Randomize_ವಾದಗಳು(ಸ್ಥಿರಾಂಕ AstVar* varp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (!m_ಒಳ_Std_With || !m_stdRandCallp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಗಾಗಿ (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            ಆಗ (exprp) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(exprp, ಗಂಟು_Var_Ref)) {
                    ಆದರೆ (varrefp->varp() == varp) ಹಿಂತಿರುಗಿ ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                }
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ memberselp = VN_CAST(exprp, MemberSel)) {
                    ಆದರೆ (memberselp->varp() == varp) ಹಿಂತಿರುಗಿ ನಿಜ;
                    exprp = memberselp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ markMembers(ಸ್ಥಿರಾಂಕ AstClass* nodep) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ) {
            ಗಾಗಿ (Ast_ಗಂಟು* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(memberp, Var);
                ಆದರೆ (!varp) ಮುಂದುವರಿ;
                // If member is randomizable and of class type, mark its class
                ಆದರೆ (varp->rand().isRandomizable()) {
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ varDtypep = varp->dtypep()->skipRefp();
                    ಸ್ಥಿರಾಂಕ AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    ಆದರೆ (!classRefp) {
                        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* subDTypep = varDtypep->subDTypep();
                        ಆದರೆ (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    ಆದರೆ (classRefp) {
                        AstClass* ಸ್ಥಿರಾಂಕ rclassp = classRefp->classp();
                        ಆದರೆ (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    ಆದರೆ (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {};
                        rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ = ನಿಜ;
                        varp->user1(rand_ಬಹುಲಕ.asUQuad);
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ markDerived(ಸ್ಥಿರಾಂಕ AstClass* nodep) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ತಳ_ಗಮ್ಯ_Derived_Map.ಕಂಡುಹಿಡಿ(nodep);
        ಆದರೆ (it != m_ತಳ_ಗಮ್ಯ_Derived_Map.ಕೊನೆ()) {
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ* classp : it->ಎರಡನೇ) {
                ಆದರೆ (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    ಶೂನ್ಯ markAllDerived() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& p : m_ತಳ_ಗಮ್ಯ_Derived_Map) {
            ಆದರೆ (p.ಮೊದಲ->user1()) markDerived(p.ಮೊದಲ);
        }
    }
    ಶೂನ್ಯ ಗಣ_Package_Refs() {
        ಗಾಗಿ (Ast_ಗಂಟು_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), ಗಂಟು_Module));
        }
    }
    ಶೂನ್ಯ markNestedGlobalConstrainedRecurse(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
            ಆದರೆ (varp->globalConstrained()) ಹಿಂತಿರುಗಿ;
            varp->globalConstrained(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(nodep, MemberSel)) {
            ಆದರೆ (memberSelp->varp()) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = memberSelp->varp();
                ಆದರೆ (varp->globalConstrained()) ಹಿಂತಿರುಗಿ;
                varp->globalConstrained(ನಿಜ);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildMemberSelChain(AstVarRef* ಬೇರು_Var_Refp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& ಮಾರ್ಗ) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = ಬೇರು_Var_Refp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        ಗಾಗಿ (AstVar* memberVarp : ಮಾರ್ಗ) {
            AstMemberSel* memberSelp
                = ಹೊಸ AstMemberSel{ಬೇರು_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        ಹಿಂತಿರುಗಿ exprp;
    }

    // Process a single constraint during nested constraint cloning
    ಶೂನ್ಯ processNestedConstraint(AstConstraint* ಸ್ಥಿರಾಂಕ constrp, AstVarRef* ಬೇರು_Var_Refp,
                                 ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& new_ಮಾರ್ಗ, AstClass* ಗುರಿ_Classp) {
        std::string ಮಾರ್ಗ_ಪೂರ್ವಪ್ರತ್ಯಯ = ಬೇರು_Var_Refp->name();
        ಗಾಗಿ (AstVar* ಮಾರ್ಗ_Member_Varp : new_ಮಾರ್ಗ) {
            ಮಾರ್ಗ_ಪೂರ್ವಪ್ರತ್ಯಯ += GLOBAL_CONSTRAINT_SEPARATOR + ಮಾರ್ಗ_Member_Varp->name();
        }

        ಸ್ಥಿರಾಂಕ std::string newName = ಮಾರ್ಗ_ಪೂರ್ವಪ್ರತ್ಯಯ + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        ತಾರ್ಕಿಕ isDuplicate = ಸುಳ್ಳು;
        ಗುರಿ_Classp->foreachMember([&](AstClass* ಸ್ಥಿರಾಂಕ, AstConstraint* ಸ್ಥಿರಾಂಕ existingConstrp) {
            ಆದರೆ (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = ನಿಜ;
            }
        });
        ಆದರೆ (isDuplicate) ಹಿಂತಿರುಗಿ;

        AstConstraint* ಸ್ಥಿರಾಂಕ cloneConstrp = constrp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::ಸದಿಶ<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(exprp, VarRef)) {
                ಆದರೆ (!varRefp->varp()->isClassMember()) ಹಿಂತಿರುಗಿ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ chainp = buildMemberSelChain(ಬೇರು_Var_Refp, new_ಮಾರ್ಗ);
                AstMemberSel* ಸ್ಥಿರಾಂಕ finalSelp
                    = ಹೊಸ AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(varRefp), varRefp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstFuncRef* ಸ್ಥಿರಾಂಕ funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                ಆದರೆ (funcRefp->taskp()->class_ವಿಧಾನ() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.ತಳ್ಳು_ಹಿಂಭಾಗ(funcRefp);
                }
            }
        });
        ಗಾಗಿ (AstFuncRef* funcRefp : funcRefps) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ chainp = buildMemberSelChain(ಬೇರು_Var_Refp, new_ಮಾರ್ಗ);
            AstArg* ಸ್ಥಿರಾಂಕ argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ() : ಶೂನ್ಯಸೂಚಕ;
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp
                = ಹೊಸ Ast_ವಿಧಾನ_Call{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        ಗುರಿ_Classp->ಕೂಡಿಸು_Stmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    ಶೂನ್ಯ cloneNestedConstraintsRecurse(AstVarRef* ಬೇರು_Var_Refp, AstClass* classp,
                                       ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& ಮಾರ್ಗ_ಗಮ್ಯ_Class,
                                       AstClass* ಗುರಿ_Classp) {
        ಗಾಗಿ (Ast_ಗಂಟು* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ memberVarp = VN_CAST(memberNodep, Var);
            ಆದರೆ (!memberVarp) ಮುಂದುವರಿ;
            ಆದರೆ (!memberVarp->rand().isRandomizable()) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            ಆದರೆ (!memberClassRefp || !memberClassRefp->classp()) ಮುಂದುವರಿ;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::ಸದಿಶ<AstVar*> new_ಮಾರ್ಗ = ಮಾರ್ಗ_ಗಮ್ಯ_Class;
            new_ಮಾರ್ಗ.ತಳ್ಳು_ಹಿಂಭಾಗ(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ಸ್ಥಿರಾಂಕ containingClassp, AstConstraint* ಸ್ಥಿರಾಂಕ constrp) {
                    processNestedConstraint(constrp, ಬೇರು_Var_Refp, new_ಮಾರ್ಗ, ಗುರಿ_Classp);
                });
            cloneNestedConstraintsRecurse(ಬೇರು_Var_Refp, nestedClassp, new_ಮಾರ್ಗ, ಗುರಿ_Classp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    ಶೂನ್ಯ handle_Randomize_ವಾದ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar* ಸ್ಥಿರಾಂಕ varp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        ಆಗ (exprp) {
            AstVar* randVarp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstArraySel* ಸ್ಥಿರಾಂಕ arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                ಮುಂದುವರಿ;  // Skip ArraySel, continue traversing
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstStructSel* ಸ್ಥಿರಾಂಕ structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                ಮುಂದುವರಿ;  // Skip StructSel, continue traversing
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                ಮುಂದುವರಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(ನಿಜ);
                varrefp->access(VAccess::READWRITE);
                exprp = ಶೂನ್ಯಸೂಚಕ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            ಆದರೆ (randVarp == varp) ಹಿಂತಿರುಗಿ;
            Ast_ಗಂಟು* backp = randVarp;
            ಆಗ (backp && !VN_IS(backp, ಗಂಟು_Module)) backp = backp->backp();
            ಆದರೆ (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {};
                rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ = ನಿಜ;
                randVarp->user1(rand_ಬಹುಲಕ.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        ಆದರೆ (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ basep = nodep->extendsp()->classp();
            m_ತಳ_ಗಮ್ಯ_Derived_Map[basep].ಸೇರಿಸು(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        ಆದರೆ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        ಆದರೆ (nodep->name() == "rand_mode") {
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(nodep, ವಿಧಾನ_Call);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = ವಿಧಾನ_Callp ? ವಿಧಾನ_Callp->fromp() : ಶೂನ್ಯಸೂಚಕ;
            ತಾರ್ಕಿಕ valid = ನಿಜ;
            ಆದರೆ (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = ಸುಳ್ಳು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = ಸುಳ್ಳು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = ಸುಳ್ಳು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstCMethodHard* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Hardp = VN_CAST(fromp, CMethodHard)) {
                ಆದರೆ (ವಿಧಾನ_Hardp->ವಿಧಾನ() == VCMethod::ARRAY_AT
                    || ವಿಧಾನ_Hardp->ವಿಧಾನ() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = ಸುಳ್ಳು;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ವಿಧಾನ_Hardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            ಆದರೆ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = ಸುಳ್ಳು;
            }
            ಆದರೆ (valid) {
                ಸ್ಥಿರಾಂಕ Rand_ಬಹುಲಕ_ಗುರಿ rand_ಬಹುಲಕ_ಗುರಿ = Rand_ಬಹುಲಕ_ಗುರಿ::get(fromp, m_classp);
                ಆದರೆ ((!rand_ಬಹುಲಕ_ಗುರಿ.receiverp || !rand_ಬಹುಲಕ_ಗುರಿ.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = ಸುಳ್ಳು;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!rand_ಬಹುಲಕ_ಗುರಿ.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = ಸುಳ್ಳು;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = ಸುಳ್ಳು;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rand_ಬಹುಲಕ_ಗುರಿ.receiverp && rand_ಬಹುಲಕ_ಗುರಿ.receiverp->isRand()) {
                    // Called on a rand member variable
                    Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {};
                    rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ = ನಿಜ;
                    rand_ಬಹುಲಕ_ಗುರಿ.receiverp->user1(rand_ಬಹುಲಕ.asUQuad);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Called on 'this' or a non-rand class instance
                    rand_ಬಹುಲಕ_ಗುರಿ.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        ಆದರೆ (!varp->isRand()) ಹಿಂತಿರುಗಿ;
                        Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {};
                        rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ = ನಿಜ;
                        varp->user1(rand_ಬಹುಲಕ.asUQuad);
                    });
                }
            }
            ಆದರೆ (!valid) {
                ಆದರೆ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                    nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_stmtp->unlink_Fr_ಹಿಂಭಾಗ();
                }
            }
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (nodep->name() == "constraint_mode") {
            ತಾರ್ಕಿಕ valid = ನಿಜ;
            ಆದರೆ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = ಸುಳ್ಳು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)
                       && !nodep->backp()->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = ಸುಳ್ಳು;
            }
            AstConstraint* constrp = ಶೂನ್ಯಸೂಚಕ;
            AstClass* classp = m_classp;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(nodep, ವಿಧಾನ_Call)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConstraintRef* ಸ್ಥಿರಾಂಕ constrRefp
                    = VN_CAST(ವಿಧಾನ_Callp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    ಆದರೆ (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClassRefDType* classRefDtp
                           = VN_CAST(ವಿಧಾನ_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = ಸುಳ್ಳು;
                }
            }
            ಆದರೆ (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = ಸುಳ್ಳು;
            }
            ಆದರೆ (valid) {
                Randomize_ಬಹುಲಕ constraint_ಬಹುಲಕ = {};
                constraint_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ = ನಿಜ;
                ಆದರೆ (constrp) {
                    constrp->user1(constraint_ಬಹುಲಕ.asUQuad);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraint_ಬಹುಲಕ.asUQuad);
                    });
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                    nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_stmtp->unlink_Fr_ಹಿಂಭಾಗ();
                }
            }
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            ಆದರೆ (m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp || m_ಒಳ_Std_With) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                ಗಾಗಿ (Ast_ಗಂಟು* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    ಆದರೆ (argp->user1()) {
                        nodep->user1(ನಿಜ);
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
            ಹಿಂತಿರುಗಿ;
        }
        AstClass* classp = m_classp;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(nodep, ವಿಧಾನ_Call)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp
                = VN_CAST(ವಿಧಾನ_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        ಆದರೆ (classp) {
            ಆದರೆ (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ಸ್ಥಿರಾಂಕ, AstVar* ಸ್ಥಿರಾಂಕ memberVarp) {
                ಆದರೆ (!memberVarp->rand().isRandomizable()) ಹಿಂತಿರುಗಿ;
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = memberVarp->dtypep()->skipRefp();
                ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(dtypep, ClassRefDType);
                ಆದರೆ (!classRefp || !classRefp->classp()) ಹಿಂತಿರುಗಿ;
                AstClass* ಸ್ಥಿರಾಂಕ memberClassp = classRefp->classp();
                ಆದರೆ (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) ಹಿಂತಿರುಗಿ;
                memberVarp->globalConstrained(ನಿಜ);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* ಬೇರು_Var_Refp
                    = ಹೊಸ AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::ಸದಿಶ<AstVar*> ಖಾಲಿಗಳು_ಮಾರ್ಗ;
                memberClassp->foreachMember([&](AstClass* ಸ್ಥಿರಾಂಕ, AstConstraint* ಸ್ಥಿರಾಂಕ constrp) {
                    processNestedConstraint(constrp, ಬೇರು_Var_Refp, ಖಾಲಿಗಳು_ಮಾರ್ಗ, classp);
                });
                cloneNestedConstraintsRecurse(ಬೇರು_Var_Refp, memberClassp, ಖಾಲಿಗಳು_ಮಾರ್ಗ, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(ಬೇರು_Var_Refp->delete_ವೃಕ್ಷ(), ಬೇರು_Var_Refp);
            });
        }
        ಆದರೆ (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handle_Randomize_ವಾದ(argp->exprp(), ಶೂನ್ಯಸೂಚಕ, ನಿಜ);
            }
            ಹಿಂತಿರುಗಿ;
        }
        ಗಾಗಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->ಅಂಕೆ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasUnsupportedMember
                    = classp->existsMember([](ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ AstVar* memberVarp) {
                          ಆದರೆ (!memberVarp->rand().isRandomizable()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                          ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = memberVarp->dtypep()->skipRefp();
                          ಹಿಂತಿರುಗಿ VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, ಸರತಿ_DType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                ಆದರೆ (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                ಮುಂದುವರಿ;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = ಶೂನ್ಯಸೂಚಕ;  // If nodep is a method call, this is its receiver
            ಆದರೆ (Ast_ವಿಧಾನ_Call* ವಿಧಾನ_Callp = VN_CAST(nodep, ವಿಧಾನ_Call)) {
                ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(ವಿಧಾನ_Callp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_AS(ವಿಧಾನ_Callp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handle_Randomize_ವಾದ(argp->exprp(), fromVarp, ಸುಳ್ಳು);
        }
        // Re-mark members after the class became inline-randomized above.
        ಆದರೆ (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp);
        m_stmtp = nodep;
        m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp = nodep;
        iterateChildren(nodep);
        ಆದರೆ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        VL_RESTORER(m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp);
        m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp);
            m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (!m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        ಆದರೆ (nodep->varp()->rand().isRandomizable() && !(m_ಒಳ_Std_With && m_stdRandCallp)) {
            nodep->user1(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (is_Var_ಒಳ_Std_Randomize_ವಾದಗಳು(nodep->varp())) {
            nodep->user1(ನಿಜ);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಆದರೆ (!m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp) ಹಿಂತಿರುಗಿ;
        iterateChildrenConst(nodep);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ randMember = nodep->varp()->rand().isRandomizable();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Std_With = m_ಒಳ_Std_With && m_stdRandCallp;
        ಆದರೆ (randObject && randMember && !ಒಳ_Std_With) {
            nodep->user1(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಒಳ_Std_With && is_Var_ಒಳ_Std_Randomize_ವಾದಗಳು(nodep->varp())) {
            nodep->user1(ನಿಜ);
            // Mark parent object for constraint expression visitor
            ಆದರೆ (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(ನಿಜ);
        }

        ಆದರೆ (m_withp) {
            Ast_ಗಂಟು* backp = m_withp;
            ಆಗ (backp->backp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp = VN_CAST(backp, ವಿಧಾನ_Call)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    ನಿಲ್ಲಿಸು;
                }
                backp = backp->backp();
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->user2p(m_modp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        nodep->user2p(m_modp);
        ಆದರೆ (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        ಆದರೆ (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_ಒಳ_Std_With = ನಿಜ;
        iterateChildrenConst(nodep);
        m_ಒಳ_Std_With = ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        iterateChildrenConst(nodep);
        ಆದರೆ (!m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp && !m_ಒಳ_Std_With) ಹಿಂತಿರುಗಿ;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        ಆದರೆ (!m_constraint_ಅಭಿವ್ಯಕ್ತಿ_Genp && !m_ಒಳ_Std_With) ಹಿಂತಿರುಗಿ;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ RandomizeMarkVisitor(Ast_ಗಂಟು* nodep) {
        iterateConst(nodep);
        markAllDerived();
        ಗಣ_Package_Refs();
    }
    ~RandomizeMarkVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

ವರ್ಗ Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* ಸ್ಥಿರಾಂಕ m_classp;
    Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* ಸ್ಥಿರಾಂಕ m_genp;  // VlRandomizer variable of the class
    AstVar* m_rand_ಬಹುಲಕ_Varp;  // Relevant randmode state variable
    ತಾರ್ಕಿಕ m_wantSingle = ಸುಳ್ಳು;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    ತಾರ್ಕಿಕ m_structSel = ಸುಳ್ಳು;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    ತಾರ್ಕಿಕ m_defer_Rand_ಬಹುಲಕ_Hoist = ಸುಳ್ಳು;  // Wait for the complete struct member path
    std::ಗಣ<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::ಗಣ<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertion_ಗಣ<AstVar*>* m_ಗಾತ್ರ_Constrained_Arraysp = ಶೂನ್ಯಸೂಚಕ;  // Arrays with size constraints
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_conditionp = ಶೂನ್ಯಸೂಚಕ;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_ಗಂಟು_FTask* m_prepareConstrainedArraysp = ಶೂನ್ಯಸೂಚಕ;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = ಶೂನ್ಯಸೂಚಕ;  // Current ID of unique call
    V3_Unique_ಹೆಸರುಗಳು& m_unique_ಹೆಸರುಗಳು;  // Unique names of temporaries, and of blocks holding them
    std::ಸದಿಶ<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_ಗಂಟು* m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_Indexp = ಶೂನ್ಯಸೂಚಕ;
    AstConstraintForeach* m_foreachp = ಶೂನ್ಯಸೂಚಕ;  // Innermost constraint-foreach being processed
    std::ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    ವರ್ಗ Nested_Access_ಮಾರ್ಗ final {
        AstMemberSel* m_ತುದಿ_Nested_Array_Member_Selp
            = ಶೂನ್ಯಸೂಚಕ;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = ಶೂನ್ಯಸೂಚಕ;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = ಶೂನ್ಯಸೂಚಕ;  // Last node of variable's name format
        Ast_ಗಂಟು** m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_ಸೂಚ್ಯಂಕ_Pointerp
            = ಶೂನ್ಯಸೂಚಕ;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    ಸಾರ್ವಜನಿಕ:
        ಶೂನ್ಯ ಕೂಡಿಸು_Var_Name_Part(AstSFormatF* ಸ್ಥಿರಾಂಕ nodep, std::string&& name) {
            ಆದರೆ (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.ಸೇರಿಸು(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            ಆದರೆ (name == "") {
                ಆದರೆ (*m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_ಸೂಚ್ಯಂಕ_Pointerp) {
                    (*m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_ಸೂಚ್ಯಂಕ_Pointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_ಸೂಚ್ಯಂಕ_Pointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_ಸೂಚ್ಯಂಕ_Pointerp = nodep;
            }
        }

        AstMemberSel* access_ವೃಕ್ಷ() { ಹಿಂತಿರುಗಿ m_ತುದಿ_Nested_Array_Member_Selp; }
        ಸ್ಥಿರಾಂಕ std::string& smtName() { ಹಿಂತಿರುಗಿ m_smtName; }

        Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = ಹೊಸ AstCMethodHard{
                varp->fileline(),
                ಹೊಸ AstVarRef{varp->fileline(), VN_AS(genp->user2p(), ಗಂಟು_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtype_ಗಣ_Void();

            methodp->ಕೂಡಿಸು_Pinsp(m_ತುದಿ_Nested_Array_Member_Selp);  // Variable

            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep();  //ಅಗಲ
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಅಗಲ = m_ತುದಿ_Nested_Array_Member_Selp->varp()->dtypep()->ಅಗಲ();
            methodp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{dtypep->fileline(), AstConst::Unsized64{}, ಅಗಲ});

            methodp->ಕೂಡಿಸು_Pinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = ಶೂನ್ಯಸೂಚಕ;

            methodp->ಕೂಡಿಸು_Pinsp(
                ಹೊಸ AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_ತುದಿ_Nested_Array_Member_Selp = ಶೂನ್ಯಸೂಚಕ;
            ಹಿಂತಿರುಗಿ methodp->makeStmt();
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cloneName() { ಹಿಂತಿರುಗಿ m_nestedNameFormatTopp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು); }

        ಶೂನ್ಯ error() {
            VL_DO_DANGLING(m_ತುದಿ_Nested_Array_Member_Selp->delete_ವೃಕ್ಷ(), m_ತುದಿ_Nested_Array_Member_Selp);
        }

        Nested_Access_ಮಾರ್ಗ(AstMemberSel* ತುದಿ_Member_Selp, Ast_ಗಂಟು** ಸೂಚಕ_ಗಮ್ಯ_ಮೊದಲ_Exprp)
            : m_ತುದಿ_Nested_Array_Member_Selp{ತುದಿ_Member_Selp}
            , m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_ಸೂಚ್ಯಂಕ_Pointerp{ಸೂಚಕ_ಗಮ್ಯ_ಮೊದಲ_Exprp} {}
        ~Nested_Access_ಮಾರ್ಗ() {
            ಆದರೆ (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_ವೃಕ್ಷ(),
                            m_nestedNameFormatTopp = ಶೂನ್ಯಸೂಚಕ);
                m_nestedNameFormatp = ಶೂನ್ಯಸೂಚಕ;
            }
        }
    };
    Nested_Access_ಮಾರ್ಗ* m_nestedAccess = ಶೂನ್ಯಸೂಚಕ;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createEnumConstraint(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, ಸ್ಥಿರಾಂಕ std::string& smtName,
                                      AstEnumDType* ಸ್ಥಿರಾಂಕ enumDtp, AstVar* ಸ್ಥಿರಾಂಕ genVarp) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ genModp = VN_AS(genVarp->user2p(), ಗಂಟು_Module);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = enumDtp->ಅಗಲ();
        std::string constraint = "(__Vbv (or";
        ಗಾಗಿ (Ast_Enum_ಅಂಶ* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvt_ಗಮ್ಯ_Str(vconstp->ಗಮ್ಯ_UInt()) + " "
                          + cvt_ಗಮ್ಯ_Str(ಅಗಲ) + "))";
        }
        constraint += "))";
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
            fl, ಹೊಸ AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "\"" + constraint + "\""}};
        callp->dtype_ಗಣ_Void();
        ಹಿಂತಿರುಗಿ callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* ಕಂಡುಹಿಡಿ_Static_Rand_ಬಹುಲಕ_Var_Member(AstClass* classp) ಸ್ಥಿರಾಂಕ {
        ಆಗ (ನಿಜ) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "__Vstaticrandmode"), Var)) {
                ಹಿಂತಿರುಗಿ varp;
            }
            AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classp->extendsp();
            ಆದರೆ (!extendsp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    ಶೂನ್ಯ markConstrainedRandMember(AstStructDType* ಸ್ಥಿರಾಂಕ structp, ಸ್ಥಿರಾಂಕ std::string& name) {
        AstMemberDType* ಸ್ಥಿರಾಂಕ memberp
            = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(ನಿಜ);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    ತಾರ್ಕಿಕ is_Rand_Qualified_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆಗ (nodep) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, StructSel)) {
                ಆದರೆ (AstStructDType* ಸ್ಥಿರಾಂಕ structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ಸ್ಥಿರಾಂಕ AstMemberDType* ಸ್ಥಿರಾಂಕ memberp
                        = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(structp, selp->name()), MemberDType);
                    ಆದರೆ (memberp && !memberp->rand().isRandomizable()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                nodep = selp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ನಿಲ್ಲಿಸು;
            }
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    ಸ್ಥಿರ ಶೂನ್ಯ markStructConstrainedRandRecurse(Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        ಆದರೆ (!structp || structp->packed()) ಹಿಂತಿರುಗಿ;
        structp->markConstrainedRand(ನಿಜ);
        ಗಾಗಿ (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            ಆದರೆ (!memberp->rand().isRandomizable()) ಮುಂದುವರಿ;
            memberp->markConstrainedRand(ನಿಜ);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string build_Member_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromp = memberSelp->fromp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            ಹಿಂತಿರುಗಿ refp->name() + "." + memberSelp->name();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            ಹಿಂತಿರುಗಿ build_Member_ಮಾರ್ಗ(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        ಹಿಂತಿರುಗಿ "";
    }

    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getFromp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ cmethp = VN_CAST(nodep, CMethodHard)) {
            ಹಿಂತಿರುಗಿ cmethp->fromp();
        }
        ಹಿಂತಿರುಗಿ getSelFromp(nodep);
    }

    ಸ್ಥಿರ std::string ಹೊರತೆಗೆ_Solve_Before_ತಳ_Name(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಆಗ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = getFromp(exprp)) exprp = fromp;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(exprp, VarRef)) ಹಿಂತಿರುಗಿ vrefp->name();
        ಹಿಂತಿರುಗಿ "";
    }

    ಸ್ಥಿರ ಶೂನ್ಯ build_Name_ಪೂರ್ವಪ್ರತ್ಯಯ(AstCExpr* exprp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
            exprp->ಕೂಡಿಸು("(\""s + refp->name());
            ಹಿಂತಿರುಗಿ;
        }

        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = getFromp(nodep);
        ಆದರೆ (fromp) build_Name_ಪೂರ್ವಪ್ರತ್ಯಯ(exprp, fromp);

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, StructSel)) {
            exprp->ಕೂಡಿಸು("." + selp->name());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ cmethp = VN_CAST(nodep, CMethodHard)) {
            ಆದರೆ (cmethp->ವಿಧಾನ() == VCMethod::ARRAY_AT) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = cmethp->pinsp();
                exprp->ಕೂಡಿಸು(".\" + ");
                AstCFuncHard* ಸ್ಥಿರಾಂಕ solverHexFuncp
                    = ಹೊಸ AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_ಗಮ್ಯ_SOLVER_HEX};
                solverHexFuncp->ಕೂಡಿಸು_Pinsp(argp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                solverHexFuncp->dtype_ಗಣ_String();
                exprp->ಕೂಡಿಸು(solverHexFuncp);
                exprp->ಕೂಡಿಸು(" + \"");
            } ಇಲ್ಲದಿದ್ದರೆ {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    ಸ್ಥಿರ AstCExpr* build_ಮಾರ್ಗ_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep,
                                   ಸ್ಥಿರಾಂಕ std::string selName) {
        AstCExpr* ಸ್ಥಿರಾಂಕ exprp = ಹೊಸ AstCExpr{fl, ""};
        build_Name_ಪೂರ್ವಪ್ರತ್ಯಯ(exprp, nodep);
        ಆದರೆ (selName != "") {
            exprp->ಕೂಡಿಸು(".");
            exprp->ಕೂಡಿಸು(selName);
        }
        exprp->ಕೂಡಿಸು("\")");
        ಹಿಂತಿರುಗಿ exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ std::string& ತಳ_Name,
                                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Sel* selp) {
        AstCExpr* ಸ್ಥಿರಾಂಕ p = ಹೊಸ AstCExpr{fl, ""};
        p->ಕೂಡಿಸು("(\""s + ತಳ_Name + ".\"");
        p->ಕೂಡಿಸು("+");
        AstCFuncHard* ಸ್ಥಿರಾಂಕ solverHexFuncp
            = ಹೊಸ AstCFuncHard{fl, VCFunction::RANDOMIZER_ಗಮ್ಯ_SOLVER_HEX};
        solverHexFuncp->ಕೂಡಿಸು_Pinsp(selp->bitp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
        solverHexFuncp->dtype_ಗಣ_String();
        p->ಕೂಡಿಸು(solverHexFuncp);
        p->ಕೂಡಿಸು(")");
        p->dtype_ಗಣ_String();
        ಹಿಂತಿರುಗಿ p;
    }

    // Helper: get fromp from MemberSel or StructSel
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getSelFromp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ mp = VN_CAST(exprp, MemberSel)) ಹಿಂತಿರುಗಿ mp->fromp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ sp = VN_CAST(exprp, StructSel)) ಹಿಂತಿರುಗಿ sp->fromp();
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ಸ್ಥಿರಾಂಕ p = ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "\"" + varrefp->name() + "\"s"};
            p->dtype_ಗಣ_String();
            ಹಿಂತಿರುಗಿ p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ selFromp = getSelFromp(exprp)) {
            ಸ್ಥಿರಾಂಕ std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string ತಳ_Name;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    ತಳ_Name = vp->name();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    ತಳ_Name = build_Member_ಮಾರ್ಗ(mp);
                }
                ಆದರೆ (ತಳ_Name.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                AstCExpr* ಸ್ಥಿರಾಂಕ p = ಹೊಸ AstCExpr{fl, ""};
                p->ಕೂಡಿಸು("(\""s + ತಳ_Name + ".\"");
                p->ಕೂಡಿಸು("+");
                AstCFuncHard* ಸ್ಥಿರಾಂಕ solverHexFuncp
                    = ಹೊಸ AstCFuncHard{fl, VCFunction::RANDOMIZER_ಗಮ್ಯ_SOLVER_HEX};
                solverHexFuncp->ಕೂಡಿಸು_Pinsp(arrSelp->bitp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                solverHexFuncp->dtype_ಗಣ_String();
                p->ಕೂಡಿಸು(solverHexFuncp);
                p->ಕೂಡಿಸು(")");
                p->dtype_ಗಣ_String();
                ಹಿಂತಿರುಗಿ p;
            }
            // Static member path (obj.field)
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vp = VN_CAST(selFromp, VarRef)) {
                ಸ್ಥಿರಾಂಕ std::string ಮಾರ್ಗ = vp->name() + "." + selName;
                AstCExpr* ಸ್ಥಿರಾಂಕ p = ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "\"" + ಮಾರ್ಗ + "\"s"};
                p->dtype_ಗಣ_String();
                ಹಿಂತಿರುಗಿ p;
            }
            ಆದರೆ (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ಸ್ಥಿರಾಂಕ p = build_ಮಾರ್ಗ_ಅಭಿವ್ಯಕ್ತಿ(fl, selFromp, selName);
                p->dtype_ಗಣ_String();
                ಹಿಂತಿರುಗಿ p;
            }
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ಸ್ಥಿರಾಂಕ p = build_ಮಾರ್ಗ_ಅಭಿವ್ಯಕ್ತಿ(fl, exprp, "");
            p->dtype_ಗಣ_String();
            ಹಿಂತಿರುಗಿ p;
        }
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Sel* selp = VN_CAST(exprp, ArraySel);
        ಆದರೆ (!selp) selp = VN_CAST(exprp, AssocSel);
        ಆದರೆ (selp) {
            // arr[i] -> dynamic name
            std::string ತಳ_Name;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vp = VN_CAST(selp->fromp(), VarRef)) {
                ತಳ_Name = vp->name();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ mp = VN_CAST(selp->fromp(), MemberSel)) {
                ತಳ_Name = build_Member_ಮಾರ್ಗ(mp);
            }
            ಆದರೆ (ತಳ_Name.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            ಹಿಂತಿರುಗಿ build_Array_Sel_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, ತಳ_Name, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ ಬಿಟ್_Selp = VN_CAST(exprp, Sel)) {
            ಹಿಂತಿರುಗಿ build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, ಬಿಟ್_Selp->fromp());
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    AstSFormatF* getConstFormat(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಹಿಂತಿರುಗಿ ಹೊಸ AstSFormatF{nodep->fileline(), (nodep->ಅಗಲ() & 3) ? "#b%b" : "#x%x", ಸುಳ್ಳು,
                               nodep};
    }
    ತಾರ್ಕಿಕ editFormat(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (nodep->user1()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&handle);
        handle.relink(getConstFormat(nodep));
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* wrapWithCond(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp,
                              ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ) {
        ಆದರೆ (condp) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = exprp->fileline();
            ಹಿಂತಿರುಗಿ ಹೊಸ AstCond{
                flp, condp, exprp,
                getConstFormat(ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, ಅಗಲ, 0})};
        }
        ಹಿಂತಿರುಗಿ exprp;
    }
    AstSFormatF* editSMT(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = ಶೂನ್ಯಸೂಚಕ,
                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = ಶೂನ್ಯಸೂಚಕ, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsp = ಶೂನ್ಯಸೂಚಕ) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_ಅಭಿವ್ಯಕ್ತಿ = nodep->emitSMT();  // Might need child width (AstExtend)
        ಆದರೆ (smt_ಅಭಿವ್ಯಕ್ತಿ == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lhs_ಅಗಲ = lhsp ? lhsp->ಅಗಲ() : 0;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rhs_ಅಗಲ = rhsp ? rhsp->ಅಗಲ() : 0;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ths_ಅಗಲ = thsp ? thsp->ಅಗಲ() : 0;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsCondp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsCondp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thsCondp = ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = ಶೂನ್ಯಸೂಚಕ;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlink_Fr_ಹಿಂಭಾಗ() : lhsp),
                         ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            lhsCondp = m_conditionp;
        }
        ಆದರೆ (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = ಶೂನ್ಯಸೂಚಕ;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlink_Fr_ಹಿಂಭಾಗ() : rhsp),
                         ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            rhsCondp = m_conditionp;
        }
        ಆದರೆ (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = ಶೂನ್ಯಸೂಚಕ;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlink_Fr_ಹಿಂಭಾಗ() : thsp),
                         ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            thsCondp = m_conditionp;
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (string::iterator pos = smt_ಅಭಿವ್ಯಕ್ತಿ.begin(); pos != smt_ಅಭಿವ್ಯಕ್ತಿ.ಕೊನೆ(); ++pos) {
            ಆದರೆ (pos[0] == '%') {
                ++pos;
                ಆಯ್ಕೆ (pos[0]) {
                ಸ್ಥಿತಿ '%': ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(argsp, wrapWithCond(lhsp, lhsCondp, lhs_ಅಗಲ));
                    lhsp = ಶೂನ್ಯಸೂಚಕ;
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(argsp, wrapWithCond(rhsp, rhsCondp, rhs_ಅಗಲ));
                    rhsp = ಶೂನ್ಯಸೂಚಕ;
                    ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(argsp, wrapWithCond(thsp, thsCondp, ths_ಅಗಲ));
                    thsp = ಶೂನ್ಯಸೂಚಕ;
                    ನಿಲ್ಲಿಸು;
                ಪೂರ್ವನಿಯೋಜಿತ: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); ನಿಲ್ಲಿಸು;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSFormatF{nodep->fileline(), smt_ಅಭಿವ್ಯಕ್ತಿ, ಸುಳ್ಳು, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಹಿಂತಿರುಗಿ newp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* editSingle(File_ಸಾಲು* fl, Ast_ಗಂಟು* itemsp) {
        ಆದರೆ (!itemsp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = ನಿಜ;

        {
            AstBegin* ಸ್ಥಿರಾಂಕ tempp
                = ಹೊಸ AstBegin{fl, "[EditWrapper]", itemsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(), ಸುಳ್ಳು};
            VL_DO_DANGLING(iterate_And_ಮುಂದಿನ_Null(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            ಆದರೆ (itemsp) itemsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            VL_DO_DANGLING(tempp->delete_ವೃಕ್ಷ(), tempp);
        }
        ಆದರೆ (!itemsp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprsp = VN_CAST(itemsp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        ಆದರೆ (!exprsp->nextp()) ಹಿಂತಿರುಗಿ exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        ಹಿಂತಿರುಗಿ ಹೊಸ AstSFormatF{fl, fmt.str(), ಸುಳ್ಳು, exprsp};
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newSel(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arrayp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ arrDtp = arrayp->unlink_Fr_ಹಿಂಭಾಗ()->dtypep()->skipRefp();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (VN_IS(arrDtp, ಸರತಿ_DType) || VN_IS(arrDtp, DynArrayDType))
            selp = ಹೊಸ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(arrDtp, UnpackArrayDType))
            selp = ಹೊಸ AstArraySel{fl, arrayp, idxp};
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(arrDtp, AssocArrayDType))
            selp = ಹೊಸ AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        ಹಿಂತಿರುಗಿ selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* adjust_ಅಗಲ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, ಪೂರ್ಣಾಂಕ ಗುರಿ_ಅಗಲ,
                             VSigning ಗುರಿ_Signing) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ = exprp->ಅಗಲ();
        ಆದರೆ (ಗುರಿ_ಅಗಲ > ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ) {
            // Extend to match target width
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ = ಹೊಸ AstExtend{fl, exprp, ಗುರಿ_ಅಗಲ};
            ಫಲಿತಾಂಶ->dtypeChgSigned(ಗುರಿ_Signing);
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಗುರಿ_ಅಗಲ < ಅಭಿವ್ಯಕ್ತಿ_ಅಗಲ) {
            // Truncate to match target width
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಫಲಿತಾಂಶ = ಹೊಸ AstSel{fl, exprp, 0, ಗುರಿ_ಅಗಲ};
            ಫಲಿತಾಂಶ->dtypeChgSigned(ಗುರಿ_Signing);
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Width already matches
            ಹಿಂತಿರುಗಿ exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಹೊರತೆಗೆ_Return_ಅಭಿವ್ಯಕ್ತಿ(AstFunc* funcp) {
        AstVar* ಸ್ಥಿರಾಂಕ retVarp = VN_CAST(funcp->fvarp(), Var);
        ಆದರೆ (!retVarp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* retExprp = ಶೂನ್ಯಸೂಚಕ;
        ಪೂರ್ಣಾಂಕ ನಿಯೋಜಿಸು_ಎಣಿಕೆ = 0;
        funcp->foreach([&](Ast_ನಿಯೋಜಿಸು* assignp) {
            AstVarRef* ಸ್ಥಿರಾಂಕ lhsp = VN_CAST(assignp->lhsp(), VarRef);
            ಆದರೆ (!lhsp || lhsp->varp() != retVarp) ಹಿಂತಿರುಗಿ;
            retExprp = assignp->rhsp();
            ++ನಿಯೋಜಿಸು_ಎಣಿಕೆ;
        });
        // Simple function: initializer + return (2) or just return (1)
        ಆದರೆ (ನಿಯೋಜಿಸು_ಎಣಿಕೆ > 2 || !retExprp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    ಶೂನ್ಯ propagateUser1InlineRecurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಆದರೆ (VN_IS(nodep, ಗಂಟು_Var_Ref)) {
            nodep->user1(VN_AS(nodep, ಗಂಟು_Var_Ref)->varp()->rand().isRandomizable());
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (VN_IS(nodep, Const)) {
            nodep->user1(ಸುಳ್ಳು);
            ಹಿಂತಿರುಗಿ;
        }
        ಪೂರ್ಣಾಂಕ any_ಮಗು = ಸುಳ್ಳು;  // Used as bool
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp = VN_CAST(nodep->op1p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            propagateUser1InlineRecurse(cp);
            any_ಮಗು |= cp->user1();
        }
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp = VN_CAST(nodep->op2p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            propagateUser1InlineRecurse(cp);
            any_ಮಗು |= cp->user1();
        }
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp = VN_CAST(nodep->op3p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            propagateUser1InlineRecurse(cp);
            any_ಮಗು |= cp->user1();
        }
        ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cp = VN_CAST(nodep->op4p(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            propagateUser1InlineRecurse(cp);
            any_ಮಗು |= cp->user1();
        }
        nodep->user1(any_ಮಗು);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    ಸ್ಥಿರ Ast_ಗಂಟು* prependDistPreamble(AstConstraintForeach* nodep, Ast_ಗಂಟು* bodyp) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ preamblep = nodep->user3p();
        ಆದರೆ (!preamblep) ಹಿಂತಿರುಗಿ bodyp;
        nodep->user3p(ಶೂನ್ಯಸೂಚಕ);
        preamblep->ಕೂಡಿಸು_ಮುಂದಿನ(bodyp);
        ಹಿಂತಿರುಗಿ preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    ತಾರ್ಕಿಕ referencesForeachIdx(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_foreachIdxVars.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ nodep->exists(
            [&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) { ಹಿಂತಿರುಗಿ m_foreachIdxVars.ಎಣಿಕೆ_2(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    ಶೂನ್ಯ ಕೂಡಿಸು_Registration_Stmt(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtp, Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ initTaskp) {
        ಆದರೆ (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->ಕೂಡಿಸು_Bodyp(stmtp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            initTaskp->ಕೂಡಿಸು_Stmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ arrExprp,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ idxExprp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(arrExprp, idxExprp);
        AstSFormatF* ಸ್ಥಿರಾಂಕ formatp = ಹೊಸ AstSFormatF{fl, "(select %s %s)", ಸುಳ್ಳು, argsp};
        ಹಿಂತಿರುಗಿ formatp;
    }

    ಶೂನ್ಯ ಗಣ_Rand_ಬಹುಲಕ(AstVar* ಸ್ಥಿರಾಂಕ varp, AstMemberSel* ಸ್ಥಿರಾಂಕ memberselp,
                     ಸ್ಥಿರಾಂಕ std::string& smtName, ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ& rand_ಬಹುಲಕ,
                     Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ initTaskp) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varClassp = VN_AS(varp->user2p(), ಗಂಟು_Module);
        AstVar* ಸ್ಥಿರಾಂಕ sub_Rand_ಬಹುಲಕ_Varp = get_Rand_ಬಹುಲಕ_Var_From_Class(varClassp);
        ಆದರೆ (sub_Rand_ಬಹುಲಕ_Varp) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Accessp = memberselp->fromp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            AstMemberSel* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Selp
                = ಹೊಸ AstMemberSel{varp->fileline(), ಪೋಷಕ_Accessp, sub_Rand_ಬಹುಲಕ_Varp};
            rand_ಬಹುಲಕ_Selp->dtypep(sub_Rand_ಬಹುಲಕ_Varp->dtypep());
            AstCMethodHard* ಸ್ಥಿರಾಂಕ atp
                = ಹೊಸ AstCMethodHard{varp->fileline(), rand_ಬಹುಲಕ_Selp, VCMethod::ARRAY_AT,
                                     ಹೊಸ AstConst{varp->fileline(), rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ}};
            atp->dtype_ಗಣ_UInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ಸ್ಥಿರಾಂಕ enablep = ಹೊಸ AstCMethodHard{
                varp->fileline(),
                ಹೊಸ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtype_ಗಣ_Void();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : ಹೊಸ AstSFormatF{varp->fileline(), smtName, ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ};
            nameExprp->dtypep(varp->dtypep());
            enablep->ಕೂಡಿಸು_Pinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ಸ್ಥಿರಾಂಕ disablep = ಹೊಸ AstCMethodHard{
                varp->fileline(),
                ಹೊಸ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_ಗಣ_VAR_DISABLED};
            disablep->dtype_ಗಣ_Void();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ disnp = nameExprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            disablep->ಕೂಡಿಸು_Pinsp(disnp);
            AstIf* ಸ್ಥಿರಾಂಕ ifp
                = ಹೊಸ AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            ಕೂಡಿಸು_Registration_Stmt(ifp, initTaskp);
        }
    }

    ಶೂನ್ಯ markRandc(AstVar* ಸ್ಥಿರಾಂಕ varp, ಸ್ಥಿರಾಂಕ std::string& smtName, Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ initTaskp) {
        AstCMethodHard* ಸ್ಥಿರಾಂಕ markp = ಹೊಸ AstCMethodHard{
            varp->fileline(),
            ಹೊಸ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtype_ಗಣ_Void();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : ಹೊಸ AstSFormatF{varp->fileline(), smtName, ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ};
        nameExprp->dtypep(varp->dtypep());
        markp->ಕೂಡಿಸು_Pinsp(nameExprp);
        ಕೂಡಿಸು_Registration_Stmt(markp->makeStmt(), initTaskp);
    }

    Ast_ಗಂಟು_Module* getLeftmostVarModulep(AstMemberSel* ಸ್ಥಿರಾಂಕ memberselp,
                                         AstVar* ಸ್ಥಿರಾಂಕ varp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_ಗಂಟು* ಎಡ_Most_Nodep = memberselp->fromp();
            ಆಗ (AstMemberSel* ಸ್ಥಿರಾಂಕ nestedMemberSelp = VN_CAST(ಎಡ_Most_Nodep, MemberSel)) {
                ಎಡ_Most_Nodep = nestedMemberSelp->fromp();
            }
            ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ ಬೇರು_Var_Refp = VN_CAST(ಎಡ_Most_Nodep, ಗಂಟು_Var_Ref)) {
                ಹಿಂತಿರುಗಿ VN_AS(ಬೇರು_Var_Refp->varp()->user2p(), ಗಂಟು_Module);
            }
            ಹಿಂತಿರುಗಿ VN_AS(memberselp->user2p(), ಗಂಟು_Module);
        }
        ಹಿಂತಿರುಗಿ VN_AS(varp->user2p(), ಗಂಟು_Module);
    }

    Ast_ಗಂಟು_FTask* getInitTaskp(AstVar* ಸ್ಥಿರಾಂಕ varp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ memberselOrStructSel,
                               Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_inlineInitTaskp) ಹಿಂತಿರುಗಿ m_inlineInitTaskp;
        varp->user3(ನಿಜ);
        ಆದರೆ (memberselOrStructSel) {
            ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ initTaskp
                = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "randomize"), ಗಂಟು_FTask)) {
                ಹಿಂತಿರುಗಿ initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            ಆದರೆ (m_memberselInitTaskp) ಹಿಂತಿರುಗಿ m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ initTaskp
                = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "new"), ಗಂಟು_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            ಹಿಂತಿರುಗಿ initTaskp;
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    ಶೂನ್ಯ createWriteVarForClassFields(AstClass* ಸ್ಥಿರಾಂಕ elemClassp, ಸ್ಥಿರಾಂಕ std::string& smtName,
                                      AstVar* ಸ್ಥಿರಾಂಕ iterVarp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isUnpackedClassRefArray,
                                      Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classOrPackagep, AstVar* ಸ್ಥಿರಾಂಕ varp,
                                      AstIf* ಸ್ಥಿರಾಂಕ ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ಸ್ಥಿರಾಂಕ {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = varp->fileline();
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varClassp = VN_AS(varp->user2p(), ಗಂಟು_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr ಪೂರ್ಣಾಂಕ idx_ಅಗಲ = 32;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ fmt_ಅಗಲ = VL_ಪದಗಳು_I(idx_ಅಗಲ) * 8;

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ) {
            ಗಾಗಿ (Ast_ಗಂಟು* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ಸ್ಥಿರಾಂಕ memberVarp = VN_CAST(mnodep, Var);
                ಆದರೆ (!memberVarp || !memberVarp->rand().isRandomizable()) ಮುಂದುವರಿ;
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ memberDtp = memberVarp->dtypep()->skipRefp();
                ಆದರೆ (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) ಮುಂದುವರಿ;
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ member_ಅಗಲ = memberDtp->ಅಗಲ();

                AstCStmt* ಸ್ಥಿರಾಂಕ fmtp = ಹೊಸ AstCStmt{fl};
                ಆದರೆ (isUnpackedClassRefArray) {
                    fmtp->ಕೂಡಿಸು("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    fmtp->ಕೂಡಿಸು("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::ಗಮ್ಯ_string(fmt_ಅಗಲ) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->ಕೂಡಿಸು(ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->ಕೂಡಿಸು(");\n");
                ifNonNullp->ಕೂಡಿಸು_Thensp(fmtp);

                AstVarRef* ಸ್ಥಿರಾಂಕ arrayWrRefp = ಹೊಸ AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* atWritep;
                ಆದರೆ (isUnpackedClassRefArray) {
                    atWritep = ಹೊಸ AstArraySel{fl, arrayWrRefp,
                                               ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    atWritep = ಹೊಸ AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = ಹೊಸ AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* ಸ್ಥಿರಾಂಕ writeVarp
                    = ಹೊಸ AstCMethodHard{fl,
                                         ಹೊಸ AstVarRef{fl, VN_AS(m_genp->user2p(), ಗಂಟು_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtype_ಗಣ_Void();
                writeVarp->ಕೂಡಿಸು_Pinsp(memberSelp);
                writeVarp->ಕೂಡಿಸು_Pinsp(
                    ಹೊಸ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(member_ಅಗಲ)});
                AstCExpr* ಸ್ಥಿರಾಂಕ nameRefp = ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->ಕೂಡಿಸು_Pinsp(nameRefp);
                writeVarp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->ಕೂಡಿಸು_Thensp(writeVarp->makeStmt());
            }
        }
    }

    ಶೂನ್ಯ createSolverArrayHandle(AstVar* ಸ್ಥಿರಾಂಕ varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classOrPackagep,
                                 ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isUnpackedClassRefArray,
                                 ಸ್ಥಿರಾಂಕ std::string& smtName) ಸ್ಥಿರಾಂಕ {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = varp->fileline();
        AstClass* ಸ್ಥಿರಾಂಕ elemClassp = elemClassRefDtp->classp();
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varClassp = VN_AS(varp->user2p(), ಗಂಟು_Module);

        AstVar* ಸ್ಥಿರಾಂಕ iterVarp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, m_unique_ಹೆಸರುಗಳು.get("__Vi"),
                                            varp->ಕಂಡುಹಿಡಿ_UInt32_DType()};
        iterVarp->funcLocal(ನಿಜ);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::WRITE}, ಹೊಸ AstConst{fl, 0}};

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizep;
        ಆದರೆ (isUnpackedClassRefArray) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ array_ಗಾತ್ರ_2
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = ಹೊಸ AstConst{fl, static_cast<uint32_t>(array_ಗಾತ್ರ_2)};
        } ಇಲ್ಲದಿದ್ದರೆ {
            AstVarRef* ಸ್ಥಿರಾಂಕ array_ಗಾತ್ರ_Refp = ಹೊಸ AstVarRef{fl, varClassp, varp, VAccess::READ};
            array_ಗಾತ್ರ_Refp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ಸ್ಥಿರಾಂಕ dynSizep
                = ಹೊಸ AstCMethodHard{fl, array_ಗಾತ್ರ_Refp, VCMethod::DYN_ಗಾತ್ರ, ಶೂನ್ಯಸೂಚಕ};
            dynSizep->dtype_ಗಣ_UInt32();
            sizep = dynSizep;
        }

        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{fl};
        stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(loopp);
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{
            fl, loopp, ಹೊಸ AstLt{fl, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ಸ್ಥಿರಾಂಕ arrayReadRefp = ಹೊಸ AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* atReadp;
        ಆದರೆ (isUnpackedClassRefArray) {
            atReadp
                = ಹೊಸ AstArraySel{fl, arrayReadRefp, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}};
        } ಇಲ್ಲದಿದ್ದರೆ {
            atReadp = ಹೊಸ AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ಸ್ಥಿರಾಂಕ ifNonNullp
            = ಹೊಸ AstIf{fl, ಹೊಸ AstNeq{fl, atReadp, ಹೊಸ AstConst{fl, AstConst::Null{}}}, ಶೂನ್ಯಸೂಚಕ};
        loopp->ಕೂಡಿಸು_Stmtsp(ifNonNullp);

        AstCStmt* ಸ್ಥಿರಾಂಕ bufDeclp = ಹೊಸ AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->ಕೂಡಿಸು_Thensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            fl, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::WRITE},
            ಹೊಸ Ast_ಕೂಡಿಸು{fl, ಹೊಸ AstConst{fl, 1}, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(ನಿಜ);
        Ast_ಗಂಟು_FTask* initTaskp = m_inlineInitTaskp;
        ಆದರೆ (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(varClassp, "new"), ಗಂಟು_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->ಕೂಡಿಸು_Stmtsp(stmtsp);
    }

    ಶೂನ್ಯ createSolverVarHandle(AstVar* ಸ್ಥಿರಾಂಕ varp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ structSelOrCMeth,
                               ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isGlobalConstrained, ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ,
                               AstMemberSel* ಸ್ಥಿರಾಂಕ memberselp, ಸ್ಥಿರಾಂಕ std::string& smtName,
                               Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classOrPackagep, Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classp,
                               Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ initTaskp) {
        uint32_t unpackedDims = 0;
        ಆದರೆ (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(ಸುಳ್ಳು).ಎರಡನೇ;
        }
        ಆದರೆ (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = ಹೊಸ AstCMethodHard{
            varp->fileline(),
            ಹೊಸ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_ಗಣ_Void();
        ಆದರೆ (memberselp) {
            methodp->ಕೂಡಿಸು_Pinsp(memberselp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            AstVarRef* ಸ್ಥಿರಾಂಕ varRefp
                = ಹೊಸ AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->ಕೂಡಿಸು_Pinsp(varRefp);
        }
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಅಗಲ = arrayElementDTypep(varp->dtypep())->ಅಗಲ();
        methodp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{varp->fileline(), AstConst::Unsized64{}, ಅಗಲ});
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ varnamep = ಹೊಸ AstCExpr{varp->fileline(), AstCExpr::ಶುದ್ಧ{},
                                                   "\"" + smtName + "\"", varp->ಅಗಲ()};
        varnamep->dtypep(varp->dtypep());
        methodp->ಕೂಡಿಸು_Pinsp(varnamep);
        methodp->ಕೂಡಿಸು_Pinsp(
            ಹೊಸ AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        ಆದರೆ (rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ && !(isGlobalConstrained && memberselp)) {
            methodp->ಕೂಡಿಸು_Pinsp(
                ಹೊಸ AstConst{varp->fileline(), AstConst::Unsized64{}, rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->ಕೂಡಿಸು_Stmtsp(methodp->makeStmt());
        ಆದರೆ (varp->lifetime().isStatic() && rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) {
            AstCMethodHard* ಸ್ಥಿರಾಂಕ markp = ಹೊಸ AstCMethodHard{
                varp->fileline(),
                ಹೊಸ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ namep = ಹೊಸ AstCExpr{varp->fileline(), AstCExpr::ಶುದ್ಧ{},
                                                    "\"" + smtName + "\"", varp->ಅಗಲ()};
            namep->dtypep(varp->dtypep());
            markp->ಕೂಡಿಸು_Pinsp(namep);
            markp->dtype_ಗಣ_Void();
            initTaskp->ಕೂಡಿಸು_Stmtsp(markp->makeStmt());
        }
        ಆದರೆ (isGlobalConstrained && memberselp && rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) {
            ಗಣ_Rand_ಬಹುಲಕ(varp, memberselp, smtName, rand_ಬಹುಲಕ, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        ಆದರೆ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createVarExprp(Ast_ಗಂಟು_Var_Ref* nodep, ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ,
                                ಸ್ಥಿರಾಂಕ std::string& smtName, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ಸ್ಥಿರಾಂಕ varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = ಹೊಸ AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ};
        ಆದರೆ (!rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) {
            // Global constraints keep nodep alive for write_var processing
            ಆದರೆ (!isGlobalConstrained) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rand_ಬಹುಲಕ_Access;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ varIsStatic = varp->lifetime().isStatic();
        AstClass* ಸ್ಥಿರಾಂಕ varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : ಶೂನ್ಯಸೂಚಕ;
        AstVar* ಸ್ಥಿರಾಂಕ owner_Static_Rand_ಬಹುಲಕ_Varp
            = varOwningClassp ? ಕಂಡುಹಿಡಿ_Static_Rand_ಬಹುಲಕ_Var_Member(varOwningClassp) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (owner_Static_Rand_ಬಹುಲಕ_Varp) {
            rand_ಬಹುಲಕ_Access = ಹೊಸ AstVarRef{varp->fileline(),
                                           VN_AS(owner_Static_Rand_ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module),
                                           owner_Static_Rand_ಬಹುಲಕ_Varp, VAccess::READ};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (memberselp) {
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varClassp = VN_AS(varp->user2p(), ಗಂಟು_Module);
            AstVar* ಸ್ಥಿರಾಂಕ effective_Rand_ಬಹುಲಕ_Varp = get_Rand_ಬಹುಲಕ_Var_From_Class(varClassp);
            ಆದರೆ (effective_Rand_ಬಹುಲಕ_Varp) {
                // Member's class has randmode, use it
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಪೋಷಕ_Access = memberselp->fromp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                AstMemberSel* rand_ಬಹುಲಕ_Sel
                    = ಹೊಸ AstMemberSel{varp->fileline(), ಪೋಷಕ_Access, effective_Rand_ಬಹುಲಕ_Varp};
                rand_ಬಹುಲಕ_Sel->dtypep(effective_Rand_ಬಹುಲಕ_Varp->dtypep());
                rand_ಬಹುಲಕ_Access = rand_ಬಹುಲಕ_Sel;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_rand_ಬಹುಲಕ_Varp, nodep, "No m_randModeVarp");
                rand_ಬಹುಲಕ_Access
                    = ಹೊಸ AstVarRef{varp->fileline(), VN_AS(m_rand_ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module),
                                    m_rand_ಬಹುಲಕ_Varp, VAccess::READ};
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(m_rand_ಬಹುಲಕ_Varp, nodep, "No m_randModeVarp");
            rand_ಬಹುಲಕ_Access
                = ಹೊಸ AstVarRef{varp->fileline(), VN_AS(m_rand_ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module),
                                m_rand_ಬಹುಲಕ_Varp, VAccess::READ};
        }

        AstCMethodHard* ಸ್ಥಿರಾಂಕ atp
            = ಹೊಸ AstCMethodHard{nodep->fileline(), rand_ಬಹುಲಕ_Access, VCMethod::ARRAY_AT,
                                 ಹೊಸ AstConst{nodep->fileline(), rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ}};
        atp->dtype_ಗಣ_UInt32();
        exprp = ಹೊಸ AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(ನಿಜ);  // Mark as formatted
        ಹಿಂತಿರುಗಿ exprp;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        ಆದರೆ (varp->user4p()) {
            ತಾರ್ಕಿಕ is_ಗಾತ್ರ_Ref = ಸುಳ್ಳು;
            ಆದರೆ (AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                ಆದರೆ (methodp->ವಿಧಾನ() == VCMethod::ASSOC_ಗಾತ್ರ
                    || methodp->ವಿಧಾನ() == VCMethod::DYN_ಗಾತ್ರ) {
                    is_ಗಾತ್ರ_Ref = ನಿಜ;
                }
            }
            ಆದರೆ (!is_ಗಾತ್ರ_Ref && m_ಗಾತ್ರ_Constrained_Arraysp) { m_ಗಾತ್ರ_Constrained_Arraysp->ಸೇರಿಸು(varp); }
        }

        // Check if this variable is marked as globally constrained
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = ಶೂನ್ಯಸೂಚಕ;
        ತಾರ್ಕಿಕ structSelOrCMeth = ಸುಳ್ಳು;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_ಗಂಟು_DType* selectedDtypep = ಶೂನ್ಯಸೂಚಕ;
        std::string smtName;
        ಆದರೆ (m_nestedAccess) {
            m_nestedAccess->ಕೂಡಿಸು_Var_Name_Part(
                ಹೊಸ AstSFormatF{varp->fileline(), nodep->name(), ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_ವೃಕ್ಷ();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_ಗಂಟು* ತುದಿ_Member_Sel = nodep->backp();
            ಆಗ (VN_IS(ತುದಿ_Member_Sel->backp(), MemberSel)) {
                ತುದಿ_Member_Sel = ತುದಿ_Member_Sel->backp();
            }
            memberselp = VN_AS(ತುದಿ_Member_Sel, MemberSel)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            smtName = build_Member_ಮಾರ್ಗ(memberselp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_ಗಂಟು* ತುದಿ_Nodep = nodep->backp();
            ಆಗ (VN_IS(ತುದಿ_Nodep->backp(), StructSel) || VN_IS(ತುದಿ_Nodep->backp(), CMethodHard)) {
                ತುದಿ_Nodep = ತುದಿ_Nodep->backp();
            }

            ಆದರೆ (VN_IS(ತುದಿ_Nodep, StructSel) || VN_IS(ತುದಿ_Nodep, CMethodHard)) {
                structSelOrCMeth = ನಿಜ;
                selectedDtypep = VN_AS(ತುದಿ_Nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->dtypep();
            }

            memberselp = VN_CAST(ತುದಿ_Nodep, MemberSel);
            ಆದರೆ (memberselp) smtName = build_Member_ಮಾರ್ಗ(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            ಆದರೆ (smtName == "") smtName = nodep->name();
        } ಇಲ್ಲದಿದ್ದರೆ {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        ಆದರೆ (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        ಆದರೆ (dtype_ಒಳಗೊಂಡಿದೆ_Real(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            ಹಿಂತಿರುಗಿ;
        }
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classOrPackagep = nodep->classOrPackagep();
        ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {.asUQuad = varp->user1()};
        ಆದರೆ (!rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ && editFormat(nodep)) ಹಿಂತಿರುಗಿ;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType);
        ಆದರೆ (enumDtp
            && (std::ಕಂಡುಹಿಡಿ(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.ಕೊನೆ(), varp)
                == m_rangeConstrainedEnums.ಕೊನೆ())) {
            AstVar* genVarp = m_genp;
            ಆದರೆ (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_ಗಂಟು_FTask* ಗುರಿ_Taskp = m_inlineInitTaskp;
            ಆದರೆ (!ಗುರಿ_Taskp) ಗುರಿ_Taskp = get_Create_Constraint_ಸಿದ್ಧತೆ_Func(m_classp, m_memberMap);
            UASSERT_OBJ(ಗುರಿ_Taskp, nodep, "No function to inline enum range constraint into");
            ಗುರಿ_Taskp->ಕೂಡಿಸು_Stmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.ತಳ್ಳು_ಹಿಂಭಾಗ(varp);
        }

        VNRelinker relinker;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp
            = createVarExprp(nodep, rand_ಬಹುಲಕ, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.ಎಣಿಕೆ_2(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.ಎಣಿಕೆ_2(smtName) > 0
                                    : memberselp          ? m_writtenVars.ಎಣಿಕೆ_2(smtName) > 0
                                                          : varp->user3();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ shouldWriteVar = !alreadyWritten;
        ಆದರೆ (shouldWriteVar) {
            // Track this variable path as written
            ಆದರೆ (m_inlineInitTaskp)
                m_inlineWrittenVars.ಸೇರಿಸು(smtName);
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isGlobalConstrained || memberselp)
                m_writtenVars.ಸೇರಿಸು(smtName);
            // For global constraints, delete nodep after processing
            ಆದರೆ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);

            ತಾರ್ಕಿಕ isClassRefArray = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ isUnpackedClassRefArray = ಸುಳ್ಳು;
            AstClassRefDType* elemClassRefDtp = ಶೂನ್ಯಸೂಚಕ;
            {
                Ast_ಗಂಟು_DType* varDtp = varp->dtypep()->skipRefp();
                ಆದರೆ (varDtp->isNonPackedArray()) {
                    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    ಆದರೆ (elemClassRefDtp) {
                        isClassRefArray = ನಿಜ;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classp = getLeftmostVarModulep(memberselp, varp);
            Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            ಆದರೆ (m_nestedAccess) {
                Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                ಕೂಡಿಸು_Registration_Stmt(writeVarStmtp, initTaskp);
                ಆದರೆ (isGlobalConstrained && memberselp && rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) {
                    ಗಣ_Rand_ಬಹುಲಕ(varp, memberselp, smtName, rand_ಬಹುಲಕ, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                ಆದರೆ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } ಇಲ್ಲದಿದ್ದರೆ {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, rand_ಬಹುಲಕ,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Variable already written, clean up cloned memberselp if any
            ಆದರೆ (memberselp) VL_DO_DANGLING(memberselp->delete_ವೃಕ್ಷ(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            ಆದರೆ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        ಆದರೆ (m_nestedAccess) { VL_DO_DANGLING(ಅಳಿಸು m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* build_ಎಣಿಕೆ_Ones_Expansion(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argp) {
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, argp->ಅಗಲ(), 1};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sump = ಹೊಸ AstAnd{fl, argp, ಹೊಸ AstConst{fl, ಅಂಕೆ_One}};
        sump->user1(ನಿಜ);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i < argp->ಅಗಲ(); i++) {
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ_ಬಿಟ್_ಮುಖವಾಡ{fl, argp->ಅಗಲ(), 0};
            ಅಂಕೆ_ಬಿಟ್_ಮುಖವಾಡ.ಗಣ_ಬಿಟ್(i, 1);
            AstAnd* ಸ್ಥಿರಾಂಕ andp
                = ಹೊಸ AstAnd{fl, argp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, ಅಂಕೆ_ಬಿಟ್_ಮುಖವಾಡ}};
            andp->user1(ನಿಜ);
            Ast_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ shiftp
                = ಹೊಸ Ast_ಸರಿಸು_R{fl, andp,
                                ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, argp->ಅಗಲ(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(ನಿಜ);
            sump = ಹೊಸ Ast_ಕೂಡಿಸು{fl, sump, shiftp};
            sump->user1(ನಿಜ);
        }
        ಹಿಂತಿರುಗಿ sump;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಎಣಿಕೆ_Ones* nodep) override {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sump = build_ಎಣಿಕೆ_Ones_Expansion(fl, argp);
        sump = adjust_ಅಗಲ(fl, sump, nodep->ಅಗಲ(), nodep->dtypep()->numeric());
        sump->user1(ನಿಜ);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(sump);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedXor* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* redxorp = ಹೊಸ AstSel{fl, argp, 0, 1};
        redxorp->user1(ನಿಜ);
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i < argp->ಅಗಲ(); i++) {
            AstSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstSel{fl, argp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), i, 1};
            selp->user1(ನಿಜ);

            redxorp = ಹೊಸ AstXor{fl, redxorp, selp};
            redxorp->user1(ನಿಜ);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(redxorp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedAnd* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        // Convert to (~x == 0)
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, argp->ಅಗಲ(), 0};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ negp = ಹೊಸ AstNot{fl, argp};
        negp->user1(ನಿಜ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ eqp = ಹೊಸ AstEq{fl, negp, ಹೊಸ AstConst{fl, ಅಂಕೆ_Zero}};
        eqp->user1(ನಿಜ);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(eqp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRedOr* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        // Convert to (x != 0)
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, argp->ಅಗಲ(), 0};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ neqp = ಹೊಸ AstNeq{fl, argp, ಹೊಸ AstConst{fl, ಅಂಕೆ_Zero}};
        neqp->user1(ನಿಜ);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(neqp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstOneHot* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ w = argp->ಅಗಲ();

        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, w, 0};
        AstNeq* ಸ್ಥಿರಾಂಕ neZerop
            = ಹೊಸ AstNeq{fl, argp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, ಅಂಕೆ_Zero}};
        neZerop->user1(ನಿಜ);

        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, w, 1};
        AstSub* ಸ್ಥಿರಾಂಕ subp = ಹೊಸ AstSub{fl, argp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, ಅಂಕೆ_One}};
        subp->user1(ನಿಜ);

        AstAnd* ಸ್ಥಿರಾಂಕ andp = ಹೊಸ AstAnd{fl, argp, subp};
        andp->user1(ನಿಜ);

        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero2{fl, w, 0};
        AstEq* ಸ್ಥಿರಾಂಕ eqZerop = ಹೊಸ AstEq{fl, andp, ಹೊಸ AstConst{fl, ಅಂಕೆ_Zero2}};
        eqZerop->user1(ನಿಜ);

        AstLogAnd* ಸ್ಥಿರಾಂಕ resultp = ಹೊಸ AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(ನಿಜ);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(resultp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstOneHot0* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        // $onehot0(x) = (x & (x-1)) == 0
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ w = argp->ಅಗಲ();

        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{fl, w, 1};
        AstSub* ಸ್ಥಿರಾಂಕ subp = ಹೊಸ AstSub{fl, argp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, ಅಂಕೆ_One}};
        subp->dtypeFrom(argp);
        subp->user1(ನಿಜ);

        AstAnd* ಸ್ಥಿರಾಂಕ andp = ಹೊಸ AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(ನಿಜ);

        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, w, 0};
        AstEq* ಸ್ಥಿರಾಂಕ eqp = ಹೊಸ AstEq{fl, andp, ಹೊಸ AstConst{fl, ಅಂಕೆ_Zero}};
        eqp->user1(ನಿಜ);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(eqp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();

        ತಾರ್ಕಿಕ ಎಣಿಕೆ_Ones = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ ಎಣಿಕೆ_Zeros = ಸುಳ್ಳು;
        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cp = VN_CAST(ctrlp, Const);
            ಆದರೆ (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ಸ್ಥಿರಾಂಕ zerop
                    = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->ಅಗಲ(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (cp->ಅಂಕೆ().ಬಿಟ್_Is1(0) && !ಎಣಿಕೆ_Ones)
                ಎಣಿಕೆ_Ones = ನಿಜ;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cp->ಅಂಕೆ().ಬಿಟ್_Is0(0) && !ಎಣಿಕೆ_Zeros)
                ಎಣಿಕೆ_Zeros = ನಿಜ;
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ arg_ಅಗಲ = argp->ಅಗಲ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sump = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಎಣಿಕೆ_Ones && ಎಣಿಕೆ_Zeros) {
            // ones + zeros = width for 2-state types
            sump = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->ಅಗಲ(), (uint32_t)arg_ಅಗಲ};
            VL_DO_DANGLING(argp->delete_ವೃಕ್ಷ(), argp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಎಣಿಕೆ_Ones) {
            sump = build_ಎಣಿಕೆ_Ones_Expansion(fl, argp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಎಣಿಕೆ_Zeros) {
            // width - countones(x)
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ onesCountp = build_ಎಣಿಕೆ_Ones_Expansion(fl, argp);
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಗಲ_Val{nodep, onesCountp->ಅಗಲ(), (uint32_t)arg_ಅಗಲ};
            sump = ಹೊಸ AstSub{fl, ಹೊಸ AstConst{fl, ಅಗಲ_Val}, onesCountp};
            sump->user1(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            sump = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, nodep->ಅಗಲ(), 0U};
            VL_DO_DANGLING(argp->delete_ವೃಕ್ಷ(), argp);
        }

        sump = adjust_ಅಗಲ(fl, sump, nodep->ಅಗಲ(), nodep->dtypep()->numeric());
        sump->user1(ನಿಜ);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(sump);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCLog2* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ arg_ಅಗಲ = argp->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಫಲಿತಾಂಶ_ಅಗಲ = nodep->ಅಗಲ();

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp
            = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, ಫಲಿತಾಂಶ_ಅಗಲ, (uint32_t)arg_ಅಗಲ};

        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ k = arg_ಅಗಲ - 1; k >= 0; k--) {
            V3_ಸಂಖ್ಯೆ ಮಿತಿ{fl, arg_ಅಗಲ, 0};
            ಆದರೆ (k < 32)
                ಮಿತಿ.ಗಣ_Long(1ULL << k);
            ಇಲ್ಲದಿದ್ದರೆ
                ಮಿತಿ.ಗಣ_ಬಿಟ್(k, 1);

            AstLte* ಸ್ಥಿರಾಂಕ ltep
                = ಹೊಸ AstLte{fl, argp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, ಮಿತಿ}};
            ltep->user1(ನಿಜ);

            AstConst* ಸ್ಥಿರಾಂಕ valuep
                = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, ಫಲಿತಾಂಶ_ಅಗಲ, (uint32_t)k};

            resultp = ಹೊಸ AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(ನಿಜ);
        }

        VL_DO_DANGLING(argp->delete_ವೃಕ್ಷ(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        iterate(resultp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* ಸ್ಥಿರಾಂಕ zerop = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    ಶೂನ್ಯ handlePow(Ast_ಗಂಟು_Biop* nodep) {
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ basep = nodep->lhsp();
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_One{nodep, basep->ಅಗಲ(), 1};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* powerp = ಹೊಸ AstConst{fl, ಅಂಕೆ_One};
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ತಳ_Signed = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ಸ್ಥಿರಾಂಕ int32_t exponent = ತಳ_Signed ? exponentp->ಗಮ್ಯ_SInt() : exponentp->ಗಮ್ಯ_UInt();
            ಆದರೆ (exponent > 0) {
                ಗಾಗಿ (int32_t i = 0; i < exponent; i++) {
                    ಆದರೆ (ತಳ_Signed) {
                        powerp = ಹೊಸ AstMulS{fl, powerp, basep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        powerp = ಹೊಸ AstMul{fl, powerp, basep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    }
                    powerp->user1(ನಿಜ);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                ಆದರೆ (ತಳ_Signed) {
                    powerp = ಹೊಸ AstDivS{fl, powerp, basep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    powerp->user1(ನಿಜ);
                    powerp = ಹೊಸ AstDivS{fl, powerp, basep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    powerp->user1(ನಿಜ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    powerp = ಹೊಸ AstDiv{fl, powerp, basep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    powerp->user1(ನಿಜ);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            iterate(powerp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ basep = VN_CAST(nodep->lhsp(), Const)) {
            ಆದರೆ (basep->ಅಂಕೆ().ಗಮ್ಯ_UInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ rhsDependent = rhsp->user1();
                AstConst* ಸ್ಥಿರಾಂಕ onep = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, ಅಗಲ, 1};
                Ast_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ shiftp = ಹೊಸ Ast_ಸರಿಸು_L{fl, onep, rhsp, ಅಗಲ};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resultp = shiftp;
                ಆದರೆ (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ nClonep = rhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    nClonep->user1(rhsDependent);
                    AstConst* ಸ್ಥಿರಾಂಕ zeroNp
                        = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, rhsp->ಅಗಲ(), 0};
                    AstGteS* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ಸ್ಥಿರಾಂಕ zeroRp = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, ಅಗಲ, 0};
                    resultp = ಹೊಸ AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                iterate(resultp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPow* nodep) override { handlePow(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPowSS* nodep) override { handlePow(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPowSU* nodep) override { handlePow(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    ಶೂನ್ಯ handle_ಸರಿಸು(Ast_ಗಂಟು_Biop* nodep) {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lhs_ಅಗಲ = nodep->lhsp()->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ rhs_ಅಗಲ = nodep->rhsp()->ಅಗಲ();
        ಆದರೆ (rhs_ಅಗಲ < lhs_ಅಗಲ) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            AstExtend* ಸ್ಥಿರಾಂಕ extendp = ಹೊಸ AstExtend{fl, rhsp, lhs_ಅಗಲ};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(ನಿಜ);
            nodep->rhsp(extendp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rhs_ಅಗಲ > lhs_ಅಗಲ) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->dtypep();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ extendp
                = VN_IS(nodep, ಸರಿಸು_RS)
                      ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtendS{fl, lhsp, rhs_ಅಗಲ})
                      : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstExtend{fl, lhsp, rhs_ಅಗಲ});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(ನಿಜ);
            nodep->lhsp(extendp);
            nodep->dtype_ಗಣ_Logic_Sized(rhs_ಅಗಲ, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
            AstSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ AstSel{fl, nodep, 0, lhs_ಅಗಲ};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            ಹಿಂತಿರುಗಿ;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_L* nodep) override { handle_ಸರಿಸು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_R* nodep) override { handle_ಸರಿಸು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸರಿಸು_RS* nodep) override { handle_ಸರಿಸು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Biop* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Uniop* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        editSMT(nodep, nodep->lhsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Triop* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCond* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtype_ಗಣ_String();
            ಹಿಂತಿರುಗಿ;
        }
        // Fall back to "(ite cond then else)"
        ಭೇಟಿ(static_cast<Ast_ಗಂಟು_Triop*>(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        editSMT(nodep, nodep->srcp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        VNRelinker handle;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ msbp = ಹೊಸ AstSFormatF{
            fl, "%1d", ಸುಳ್ಳು,
            ಹೊಸ Ast_ಕೂಡಿಸು{fl, nodep->lsbp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                       ಹೊಸ AstConst{fl, static_cast<uint32_t>(nodep->ಅಗಲ_Const() - 1)}}};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lsbp
            = ಹೊಸ AstSFormatF{fl, "%1d", ಸುಳ್ಳು, nodep->lsbp()->unlink_Fr_ಹಿಂಭಾಗ(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ outermost = !m_defer_Rand_ಬಹುಲಕ_Hoist;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origp = outermost ? nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು) : ಶೂನ್ಯಸೂಚಕ;
        VL_RESTORER(m_structSel);
        m_structSel = ನಿಜ;
        ಆದರೆ (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = nodep->fromp();
            AstStructDType* ಸ್ಥಿರಾಂಕ structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ಆದರೆ (is_Rand_Qualified_ಮಾರ್ಗ(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } ಇಲ್ಲದಿದ್ದರೆ {
                structp->markConstrainedRand(ನಿಜ);
            }
        }
        // Mark Random for structArray
        ಆದರೆ (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ಸ್ಥಿರಾಂಕ structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            ಆದರೆ (is_Rand_Qualified_ಮಾರ್ಗ(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } ಇಲ್ಲದಿದ್ದರೆ {
                structp->markConstrainedRand(ನಿಜ);
            }
        }
        {
            VL_RESTORER(m_defer_Rand_ಬಹುಲಕ_Hoist);
            m_defer_Rand_ಬಹುಲಕ_Hoist = ನಿಜ;
            iterateChildren(nodep);
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        AstSFormatF* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = ಹೊಸ AstSFormatF{fl, "%s.%s." + nodep->name(), ಸುಳ್ಳು,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)};
            ಆದರೆ (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp = ಹೊಸ AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), ಸುಳ್ಳು,
                                   ಶೂನ್ಯಸೂಚಕ};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಆದರೆ (origp && !hoist_Rand_ಬಹುಲಕ_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(origp), origp);
        }
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_String_Name_Part(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ indexp, ಸ್ಥಿರಾಂಕ std::string& fmt = "%32x") {
        ಆದರೆ (m_nestedAccess) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bitp = indexp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಬಿಟ್_Formatp = ಹೊಸ AstSFormatF{bitp->fileline(), fmt, ಸುಳ್ಳು, bitp};
            AstSFormatF* ಸ್ಥಿರಾಂಕ herep
                = ಹೊಸ AstSFormatF{indexp->fileline(), "%s.%s", ಸುಳ್ಳು, ಬಿಟ್_Formatp};
            m_nestedAccess->ಕೂಡಿಸು_Var_Name_Part(herep, indexp->name());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssocSel* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origp = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        AstSFormatF* newp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            ಕೂಡಿಸು_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ idxp = ಹೊಸ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), ಸುಳ್ಳು,
                                                      nodep->bitp()->unlink_Fr_ಹಿಂಭಾಗ(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ಸ್ಥಿರಾಂಕ stringp = VN_AS(nodep->bitp(), CvtPackString);
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t string_ಗಾತ್ರ = VN_AS(stringp->lhsp(), Const)->ಅಗಲ();
            ಆದರೆ (string_ಗಾತ್ರ > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << string_ಗಾತ್ರ << "bits, limit is 128 bits");
            }
            ಕೂಡಿಸು_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ idxp = ಹೊಸ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), ಸುಳ್ಳು,
                                                      stringp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ actual_ಅಗಲ = nodep->bitp()->ಅಗಲ();
                std::string fmt;
                // Normalize to standard bit width
                ಆದರೆ (actual_ಅಗಲ <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (actual_ಅಗಲ <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } ಇಲ್ಲದಿದ್ದರೆ {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::ಗಮ್ಯ_string(VL_ಪದಗಳು_I(actual_ಅಗಲ) * 8) + "x";
                }
                ಆದರೆ (m_nestedAccess) ಕೂಡಿಸು_String_Name_Part(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ idxp
                    = ಹೊಸ AstSFormatF{fl, fmt, ಸುಳ್ಳು, nodep->bitp()->unlink_Fr_ಹಿಂಭಾಗ(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                ಆದರೆ (m_nestedAccess) m_nestedAccess->error();
            }
        }
        ಆದರೆ (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        ಆದರೆ (!newp || !hoist_Rand_ಬಹುಲಕ_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(origp->delete_ವೃಕ್ಷ(), origp);
        }
    }
    ಶೂನ್ಯ rename_Array_ಅಭಿವ್ಯಕ್ತಿ(AstSFormatF* ಸ್ಥಿರಾಂಕ newp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        ತಾರ್ಕಿಕ ಸೂಚ್ಯಂಕ_Is_Rand = ಸುಳ್ಳು;
        nodep->bitp()->foreach([&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* vrefp) {
            ಆದರೆ (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                ಸೂಚ್ಯಂಕ_Is_Rand = ನಿಜ;
            }
        });
        ಆದರೆ (m_nestedAccess) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ bitp = nodep->bitp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಬಿಟ್_Formatp
                = ಸೂಚ್ಯಂಕ_Is_Rand ? ಹೊಸ AstSFormatF{bitp->fileline(), bitp->name(), ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ}
                              : ಹೊಸ AstSFormatF{bitp->fileline(), "%x", ಸುಳ್ಳು, bitp};
            AstSFormatF* ಸ್ಥಿರಾಂಕ herep
                = ಹೊಸ AstSFormatF{nodep->fileline(), "%s.%s", ಸುಳ್ಳು, ಬಿಟ್_Formatp};
            ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->ಕೂಡಿಸು_Var_Name_Part(herep, selp->fromp()->name());
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_nestedAccess->ಕೂಡಿಸು_Var_Name_Part(herep, bitp->name());
            }
            ಆದರೆ (ಸೂಚ್ಯಂಕ_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_ವೃಕ್ಷ(), bitp);
                ಹಿಂತಿರುಗಿ;
            }
        }
        ಆದರೆ (ಸೂಚ್ಯಂಕ_Is_Rand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp = nodep->bitp()->unlink_Fr_ಹಿಂಭಾಗ(&handle);
            ಆದರೆ (indexp->ಅಗಲ() < 32) {
                AstExtend* ಸ್ಥಿರಾಂಕ extendp = ಹೊಸ AstExtend{fl, indexp, 32};
                extendp->user1(ನಿಜ);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ಸ್ಥಿರಾಂಕ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ಅಭಿವ್ಯಕ್ತಿ(newp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origp = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ indexp
                = ಹೊಸ AstSFormatF{fl, "#x%8x", ಸುಳ್ಳು, nodep->bitp()->unlink_Fr_ಹಿಂಭಾಗ(&handle)};
            handle.relink(indexp);
            AstSFormatF* ಸ್ಥಿರಾಂಕ newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_ಅಭಿವ್ಯಕ್ತಿ(newp);
            ಆದರೆ (!newp || !hoist_Rand_ಬಹುಲಕ_Over_Select_And_Member(newp, origp)) {
                VL_DO_DANGLING(origp->delete_ವೃಕ್ಷ(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Rand_ಬಹುಲಕ_Gate(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ atp = VN_CAST(exprp, CMethodHard);
        ಆದರೆ (!atp || atp->ವಿಧಾನ() != VCMethod::ARRAY_AT) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstVar* varp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        ಆದರೆ (!varp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(varp->user2p(), Class);
        ಹಿಂತಿರುಗಿ classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    ತಾರ್ಕಿಕ hoist_Rand_ಬಹುಲಕ_Over_Select_And_Member(AstSFormatF* newp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* origp) {
        ಆದರೆ (m_defer_Rand_ಬಹುಲಕ_Hoist) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Only for selects yielding a non-array element (full chains)
        ಆದರೆ (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::ಸದಿಶ<AstSFormatF*> ಚೌಕಟ್ಟುಗಳು;
        AstCond* ಬಹುಲಕ_Condp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ firstp = curp->exprsp();
            ಚೌಕಟ್ಟುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(curp);
            ಆದರೆ (AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(firstp, Cond)) {
                ಆದರೆ (is_Rand_ಬಹುಲಕ_Gate(condp->condp())) ಬಹುಲಕ_Condp = condp;
                ನಿಲ್ಲಿಸು;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        ಆದರೆ (!ಬಹುಲಕ_Condp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = newp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ modep = ಬಹುಲಕ_Condp->condp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* activep = ಬಹುಲಕ_Condp->thenp()->unlink_Fr_ಹಿಂಭಾಗ();
        // Rebuild the select chain text around the SMT name, innermost first
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = ಚೌಕಟ್ಟುಗಳು.rbegin(); it != ಚೌಕಟ್ಟುಗಳು.rend(); ++it) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಉಳಿದ_Argsp = VN_CAST((*it)->exprsp()->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಆದರೆ (ಉಳಿದ_Argsp) { ಉಳಿದ_Argsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(); }
            activep = ಹೊಸ AstSFormatF{fl, (*it)->name(), ಸುಳ್ಳು,
                                      Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(activep, ಉಳಿದ_Argsp)};
        }
        AstCond* ಸ್ಥಿರಾಂಕ hoistp = ಹೊಸ AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(ನಿಜ);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(newp), newp);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        ಆದರೆ (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
            ಆದರೆ (m_nestedAccess) {
                AstSFormatF* ಸ್ಥಿರಾಂಕ formatp = ಹೊಸ AstSFormatF{fl, nodep->name(), ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ};
                m_nestedAccess->ಕೂಡಿಸು_Var_Name_Part(ಹೊಸ AstSFormatF{fl, "%s.%s", ಸುಳ್ಳು, formatp},
                                               nodep->name());
            }
            Ast_ಗಂಟು* ಬೇರು_ಗಂಟು = nodep->fromp();
            ಆಗ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(ಬೇರು_ಗಂಟು, MemberSel)) {
                ಬೇರು_ಗಂಟು = selp->fromp();
            }

            Ast_ಗಂಟು_Sel* arraySelp = VN_CAST(ಬೇರು_ಗಂಟು, ArraySel);
            ಆದರೆ (!arraySelp) arraySelp = VN_CAST(ಬೇರು_ಗಂಟು, AssocSel);

            ಆದರೆ (arraySelp) {
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isSimple
                        = nodep->fromp() == ಬೇರು_ಗಂಟು && VN_IS(arraySelp->fromp(), VarRef);
                    ಆದರೆ (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = ಹೊಸ Nested_Access_ಮಾರ್ಗ(nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                                              &m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_Indexp);
                        AstSFormatF* ಸ್ಥಿರಾಂಕ formatp
                            = ಹೊಸ AstSFormatF{fl, nodep->name(), ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ};
                        m_nestedAccess->ಕೂಡಿಸು_Var_Name_Part(
                            ಹೊಸ AstSFormatF{fl, "%s.%s", ಸುಳ್ಳು, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = ನಿಜ;
                    nodep->user1(ನಿಜ);
                    arraySelp->user1(ನಿಜ);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origp = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                    iterateChildren(nodep);
                    AstSFormatF* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSFormatF{fl, "%s." + nodep->name(), ಸುಳ್ಳು,
                                                              nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()};
                    nodep->replaceWith(newp);
                    ಆದರೆ (!hoist_Rand_ಬಹುಲಕ_Over_Select_And_Member(newp, origp))
                        VL_DO_DANGLING(origp->delete_ವೃಕ್ಷ(), origp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    VL_DO_DANGLING(ಅಳಿಸು m_nestedAccess, m_nestedAccess);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            // Check if the root variable participates in global constraints
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(ಬೇರು_ಗಂಟು, VarRef)) {
                AstVar* ಸ್ಥಿರಾಂಕ constrainedVar = varRefp->varp();
                ಆದರೆ (constrainedVar->globalConstrained()) {
                    ತಾರ್ಕಿಕ ಸ್ಥಿರಾಂಕ beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    ಆದರೆ (beforeIterate) {
                        nodep->replaceWith(ಹೊಸ AstSFormatF{fl, "%s." + nodep->name(), ಸುಳ್ಳು,
                                                           nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ()});
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->replaceWith(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ());
                    }
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        ಆದರೆ (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), ಗಂಟು_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ಸ್ಥಿರಾಂಕ varRefp
                = ಹೊಸ AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), ಗಂಟು_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಭೇಟಿ(varRefp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->ವಿಧಾನ() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ಸ್ಥಿರಾಂಕ cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = arrayDtp->subDTypep()->skipRefp();
            ಆದರೆ (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = ನಿಜ;
                iterateChildren(nodep);
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                AstSFormatF* newp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (AstSFormatF* ಸ್ಥಿರಾಂಕ fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    ಆದರೆ (fromp->name() == "%s.%s") {
                        newp = ಹೊಸ AstSFormatF{fl, "%s.%s." + nodep->name(), ಸುಳ್ಳು,
                                               fromp->exprsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        newp = ಹೊಸ AstSFormatF{fl, fromp->name() + "." + nodep->name(), ಸುಳ್ಳು,
                                               ಶೂನ್ಯಸೂಚಕ};
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newp = ಹೊಸ AstSFormatF{fl, nodep->name(), ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            editFormat(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatF* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmt* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintIf* nodep) override {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = ಶೂನ್ಯಸೂಚಕ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thenp = editSingle(fl, nodep->thensp());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elsep = editSingle(fl, nodep->elsesp());
        ಆದರೆ (thenp && elsep) {
            newp = ಹೊಸ AstCond{fl, nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ(), thenp, elsep};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (thenp) {
            newp = ಹೊಸ AstLogIf{fl, nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ(), thenp};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (elsep) {
            newp = ಹೊಸ AstLogIf{fl, ಹೊಸ AstNot{fl, nodep->condp()->unlink_Fr_ಹಿಂಭಾಗ()}, elsep};
        }
        ಆದರೆ (newp) {
            newp->dtype_ಗಣ_ಬಿಟ್();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(ನಿಜ);  // Assume result-dependent
            nodep->replaceWith(ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, newp});
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstBegin* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();

        ಆದರೆ (!nodep->bodyp()) {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_wantSingle) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{fl};
            cstmtp->ಕೂಡಿಸು("ret += \" \";\n");
            cstmtp->ಕೂಡಿಸು("ret += ");
            cstmtp->ಕೂಡಿಸು(itemp);
            cstmtp->ಕೂಡಿಸು(";");
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ಸ್ಥಿರಾಂಕ cexprp = ಹೊಸ AstCExpr{fl};
            cexprp->dtype_ಗಣ_String();
            cexprp->ಕೂಡಿಸು("([&]{\nstd::string ret;\n");
            cexprp->ಕೂಡಿಸು(ಹೊಸ AstBegin{fl, m_unique_ಹೆಸರುಗಳು.get("__VrandBlock"),
                                     ಹೊಸ AstForeach{fl, nodep->headerp()->unlink_Fr_ಹಿಂಭಾಗ(), bodyp},
                                     ನಿಜ});
            cexprp->ಕೂಡಿಸು("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(ಹೊಸ AstSFormatF{fl, "%s", ಸುಳ್ಳು, cexprp});
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            ಗಾಗಿ (Ast_ಗಂಟು* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(elemp, Var)) m_foreachIdxVars.ಸೇರಿಸು(varp);
            }
            iterate_And_ಮುಂದಿನ_Null(nodep->bodyp());
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            nodep->replaceWith(
                ಹೊಸ AstBegin{fl, m_unique_ಹೆಸರುಗಳು.get("__VrandBlock"),
                             ಹೊಸ AstForeach{fl, nodep->headerp()->unlink_Fr_ಹಿಂಭಾಗ(), bodyp}, ನಿಜ});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ genModp = VN_AS(m_genp->user2p(), ಗಂಟು_Module);

        ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsTestp = build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, lhsp);
            ಆದರೆ (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                ಮುಂದುವರಿ;
            }
            VL_DO_DANGLING(lhsTestp->delete_ವೃಕ್ಷ(), lhsTestp);
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsNamep = build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, rhsp);
                ಆದರೆ (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    ಮುಂದುವರಿ;
                }
                AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
                    fl, ಹೊಸ AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtype_ಗಣ_Void();
                callp->ಕೂಡಿಸು_Pinsp(build_Solve_Before_Name_ಅಭಿವ್ಯಕ್ತಿ(fl, lhsp));
                callp->ಕೂಡಿಸು_Pinsp(rhsNamep);
                nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintUnique* nodep) override {
        Ast_ಗಂಟು_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        Ast_ಗಂಟು_Module* genModp = ಶೂನ್ಯಸೂಚಕ;

        ಆದರೆ (m_classp) {
            initTaskp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(m_classp, "new"), ಗಂಟು_FTask);
            genVarp = getRandomGenerator(m_classp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        ಆದರೆ (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_classp) {
            genModp = VN_AS(genVarp->user2p(), ಗಂಟು_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_ಗಂಟು* ಸಿದ್ಧತೆ_Stmtsp = ಶೂನ್ಯಸೂಚಕ;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::ಸದಿಶ<std::string> smtExprs;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ಸ್ಥಿರಾಂಕ varp = varRefp->varp();
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varModp = VN_AS(varp->user2p(), ಗಂಟು_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep()->skipRefp();

                ಸ್ಥಿರಾಂಕ ಸ್ಥಿರ ಸ್ವಯಂಚಾಲಿತ dynDTypeSupported
                    = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) -> ತಾರ್ಕಿಕ {
                    ಹಿಂತಿರುಗಿ VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, ಸರತಿ_DType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elem_ಅಗಲ;
                uint32_t static_ಗಾತ್ರ = 0;  // Element count, zero when only known at run time
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ ಮೇಲೆ = VN_CAST(dtypep, UnpackArrayDType)) {
                    ಸ್ಥಿರಾಂಕ AstRange* ಸ್ಥಿರಾಂಕ rangep = ಮೇಲೆ->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    static_ಗಾತ್ರ = ಮೇಲೆ->elementsConst();
                    ಆದರೆ (static_ಗಾತ್ರ > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        ಮುಂದುವರಿ;
                    }
                    elem_ಅಗಲ = static_cast<uint64_t>(varp->dtypep()->ಅಗಲ());
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dynDTypeSupported(dtypep)) {
                    elem_ಅಗಲ = static_cast<uint64_t>(dtypep->subDTypep()->ಅಗಲ());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(ಸುಳ್ಳು));
                    ಮುಂದುವರಿ;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subp = dtypep->subDTypep()->skipRefp();
                ಆದರೆ (VN_IS(subp, ಗಂಟು_Array_DType) || VN_IS(subp, ಸರತಿ_DType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    ಮುಂದುವರಿ;
                }

                ಆದರೆ (m_wantSingle && !static_ಗಾತ್ರ) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    ಮುಂದುವರಿ;
                }

                ಆದರೆ (!varp->user3()) {
                    varp->user3(ನಿಜ);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ varnamep = ಹೊಸ AstCExpr{
                        fl, AstCExpr::ಶುದ್ಧ{}, "\"" + varp->name() + "\"", varp->ಅಗಲ()};

                    AstCMethodHard* ಸ್ಥಿರಾಂಕ writeVarCallp = ಹೊಸ AstCMethodHard{
                        fl, ಹೊಸ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ಸ್ಥಿರಾಂಕ argVarRefp = ಹೊಸ AstVarRef{fl, varp, VAccess::READ};
                    ಆದರೆ (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->ಕೂಡಿಸು_Pinsp(argVarRefp);
                    writeVarCallp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{fl, AstConst::Unsized64{}, elem_ಅಗಲ});
                    writeVarCallp->ಕೂಡಿಸು_Pinsp(varnamep);
                    writeVarCallp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{fl, 1});  // Dimension
                    ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {.asUQuad = varp->user1()};
                    ಆದರೆ (rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) {
                        writeVarCallp->ಕೂಡಿಸು_Pinsp(
                            ಹೊಸ AstConst{fl, AstConst::Unsized64{}, rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ});
                    }
                    writeVarCallp->dtype_ಗಣ_Void();
                    initTaskp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                ಆದರೆ (varp->user4p() && m_ಗಾತ್ರ_Constrained_Arraysp) {
                    m_ಗಾತ್ರ_Constrained_Arraysp->ಸೇರಿಸು(varp);
                }

                ಆದರೆ (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    ಆದರೆ (static_ಗಾತ್ರ < 2) ಮುಂದುವರಿ;
                    std::string ಅಭಿವ್ಯಕ್ತಿ_Str = "(__Vbv (distinct";
                    ಗಾಗಿ (uint32_t i = 0; i < static_ಗಾತ್ರ; ++i) {
                        ಅಭಿವ್ಯಕ್ತಿ_Str += " (select " + varp->name() + " #x";
                        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಸರಿಸು_2 = 28; ಸರಿಸು_2 >= 0; ಸರಿಸು_2 -= 4) {
                            ಅಭಿವ್ಯಕ್ತಿ_Str += "0123456789abcdef"[(i >> ಸರಿಸು_2) & 0xf];
                        }
                        ಅಭಿವ್ಯಕ್ತಿ_Str += ')';
                    }
                    smtExprs.emplace_ಹಿಂಭಾಗ(ಅಭಿವ್ಯಕ್ತಿ_Str + "))");
                    ಮುಂದುವರಿ;
                }

                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ namep = ಹೊಸ AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ಸ್ಥಿರಾಂಕ randUniqueCallp
                    = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->ಕಂಡುಹಿಡಿ_Void_DType());
                randUniqueCallp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{fl, *m_uniqueConstraintId});
                ಸಿದ್ಧತೆ_Stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(ಸಿದ್ಧತೆ_Stmtsp, ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        ಆದರೆ (m_wantSingle && !smtExprs.ಖಾಲಿಗಳು()) {
            std::string ಅಭಿವ್ಯಕ್ತಿ_Str = smtExprs.front();
            ಆದರೆ (smtExprs.ಗಾತ್ರ_2() > 1) {
                ಅಭಿವ್ಯಕ್ತಿ_Str = "(bvand";
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& oneExprp : smtExprs) ಅಭಿವ್ಯಕ್ತಿ_Str += " " + oneExprp;
                ಅಭಿವ್ಯಕ್ತಿ_Str += ')';
            }
            AstSFormatF* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstSFormatF{fl, ಅಭಿವ್ಯಕ್ತಿ_Str, ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಸಿದ್ಧತೆ_Stmtsp) nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(ಸಿದ್ಧತೆ_Stmtsp);
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_ಮೊದಲ_ಅಭಿವ್ಯಕ್ತಿ_Inside_Indexp);
        ಆದರೆ (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->exprp();
            ಆದರೆ (exprp->ಅಗಲ() > 1) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = exprp->fileline();
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{fl, exprp->ಅಗಲ(), 0};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ neqp
                    = ಹೊಸ AstNeq{fl, exprp->unlink_Fr_ಹಿಂಭಾಗ(), ಹೊಸ AstConst{fl, ಅಂಕೆ_Zero}};
                neqp->user1(ನಿಜ);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        ಆದರೆ (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ಸ್ಥಿರಾಂಕ VCMethod ವಿಧಾನ
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
            nodep->fileline(),
            ಹೊಸ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                          VAccess::READWRITE},
            ವಿಧಾನ, nodep->exprp()->unlink_Fr_ಹಿಂಭಾಗ()};
        callp->dtype_ಗಣ_Void();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstCExpr{nodep->fileline(), AstCExpr::ಶುದ್ಧ{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ಸ್ಥಿರಾಂಕ uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string pretty_ಪಠ್ಯ;
        ಆದರೆ (!v3Global.opt.protectIds()) {
            pretty_ಪಠ್ಯ = nodep->fileline()->pretty_ಮೂಲ();
            ಗಾತ್ರ_t pos = 0;
            ಆಗ ((pos = pretty_ಪಠ್ಯ.ಕಂಡುಹಿಡಿ('"', pos)) != std::string::npos) {
                pretty_ಪಠ್ಯ.ಸೇರಿಸು(pos, "\\");
                pos += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("\\\"");
            }
        }
        callp->ಕೂಡಿಸು_Pinsp(
            ಹೊಸ AstCExpr{nodep->fileline(), AstCExpr::ಶುದ್ಧ{}, "\"" + pretty_ಪಠ್ಯ + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();

        ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_AT) {
            ಕೂಡಿಸು_String_Name_Part(nodep->pinsp(), "%x");
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಸೂಚ್ಯಂಕ_Is_Rand = nodep->pinsp()->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ vrefp) {
                ಹಿಂತಿರುಗಿ vrefp->varp()->rand().isRandomizable();
            });
            ಆದರೆ (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ basep = nodep->ತಳ_Fromp(ನಿಜ);
                    ಆದರೆ (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        ಹಿಂತಿರುಗಿ;
                    }
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origp = ಸೂಚ್ಯಂಕ_Is_Rand ? ಶೂನ್ಯಸೂಚಕ : nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ ತಳ_Refp
                    = m_structSel ? VN_CAST(nodep->fromp()->ತಳ_Fromp(ನಿಜ), VarRef) : ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (ತಳ_Refp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(ತಳ_Refp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ indexp = nodep->pinsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ arrayp = nodep->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    AstCMethodHard* ಸ್ಥಿರಾಂಕ currentSizep
                        = ಹೊಸ AstCMethodHard{fl, arrayp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), VCMethod::DYN_ಗಾತ್ರ};
                    currentSizep->dtype_ಗಣ_Int();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಒಳ_Range_Requiredp = ಹೊಸ AstLogAnd{
                        fl,
                        ಹೊಸ AstLteS{fl,
                                    ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, indexp->ಅಗಲ(), 0},
                                    indexp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)},
                        ಹೊಸ AstGteS{fl, indexp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), currentSizep}};
                    AstCMethodHard* ಸ್ಥಿರಾಂಕ resizep
                        = ಹೊಸ AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             ಹೊಸ Ast_ಕೂಡಿಸು{fl, indexp, ಹೊಸ AstConst{fl, 1}}};
                    resizep->dtype_ಗಣ_Void();
                    m_prepareConstrainedArraysp->ಕೂಡಿಸು_Stmtsp(
                        ಹೊಸ AstIf{fl, ಒಳ_Range_Requiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ಸ್ಥಿರಾಂಕ sizep
                    = m_structSel ? ಹೊಸ AstCMethodHard{fl, nodep->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                       VCMethod::DYN_ಗಾತ್ರ}
                                  : ಶೂನ್ಯಸೂಚಕ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ pinp = nodep->pinsp()->unlink_Fr_ಹಿಂಭಾಗ();
                AstSFormatF* newp;
                ಆದರೆ (m_structSel) {
                    ಆದರೆ (AstSFormatF* ಸ್ಥಿರಾಂಕ sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argsp
                        = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), pinp);
                    sizep->dtype_ಗಣ_Int();
                    AstLogAnd* ಸ್ಥಿರಾಂಕ condp
                        = ಹೊಸ AstLogAnd{fl,
                                        ಹೊಸ AstLteS{fl,
                                                    ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{},
                                                                 originalPinp->ಅಗಲ(), 0},
                                                    originalPinp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)},
                                        ಹೊಸ AstLtS{fl, originalPinp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), sizep}};
                    m_conditionp = m_conditionp ? ಹೊಸ AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = ಹೊಸ AstSFormatF{fl, "%s.%s", ಸುಳ್ಳು, argsp};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಆದರೆ (origp && !hoist_Rand_ಬಹುಲಕ_Over_Select_And_Member(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_ವೃಕ್ಷ(), origp);
                }

            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸೂಚ್ಯಂಕ_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->user1(ಸುಳ್ಳು);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_INSIDE) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ randArr = nodep->fromp()->user1();

            AstVar* ಸ್ಥಿರಾಂಕ newVarp
                = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->ಕಂಡುಹಿಡಿ_Int_DType()};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ idxRefp = ಹೊಸ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            Ast_Foreach_ಶೀರ್ಷಿಕೆ* ಸ್ಥಿರಾಂಕ headerp
                = ಹೊಸ Ast_Foreach_ಶೀರ್ಷಿಕೆ{fl, nodep->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), newVarp};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemp = ಹೊಸ AstEq{fl, selp, nodep->pinsp()->unlink_Fr_ಹಿಂಭಾಗ()};
            itemp->user1(ನಿಜ);

            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{fl};
            cstmtp->ಕೂಡಿಸು("ret += \" \";\n");
            cstmtp->ಕೂಡಿಸು("ret += ");
            cstmtp->ಕೂಡಿಸು(iterateSubtreeReturnEdits(itemp));
            cstmtp->ಕೂಡಿಸು(";");
            AstCExpr* ಸ್ಥಿರಾಂಕ cexprp = ಹೊಸ AstCExpr{fl};
            cexprp->dtype_ಗಣ_String();
            cexprp->ಕೂಡಿಸು("([&]{\nstd::string ret;\n");
            cexprp->ಕೂಡಿಸು(ಹೊಸ AstBegin{fl, m_unique_ಹೆಸರುಗಳು.get("__VrandBlock"),
                                     ಹೊಸ AstForeach{fl, headerp, cstmtp}, ನಿಜ});
            cexprp->ಕೂಡಿಸು("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(ಹೊಸ AstSFormatF{fl, "%s", ಸುಳ್ಳು, cexprp});
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Array reduction methods (sum, product, and, or, xor)
        ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_ಮೊತ್ತ
            || nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_ಗುಣಲಬ್ಧ
            || nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_AND || nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_OR
            || nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_XOR) {

            AstWith* ಸ್ಥಿರಾಂಕ withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            ಆದರೆ (withp) {
                ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ array_ಗಾತ್ರ_2 = adtypep->elementsConst();
                    ಆದರೆ (array_ಗಾತ್ರ_2 > v3Global.opt.constraint_Array_ಎಲ್ಲೆ()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvt_ಗಮ್ಯ_Str(array_ಗಾತ್ರ_2)
                                          + " exceeds --constraint-array-limit of "
                                          + cvt_ಗಮ್ಯ_Str(v3Global.opt.constraint_Array_ಎಲ್ಲೆ())
                                          + "), treating as state");
                        nodep->user1(ಸುಳ್ಳು);
                        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasRandomArray = nodep->fromp()->user1();
            ಆದರೆ (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(ಸುಳ್ಳು);
                ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                ಹಿಂತಿರುಗಿ;
            }

            // Create loop variable and header
            AstVar* ಸ್ಥಿರಾಂಕ loopVarp
                = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->ಕಂಡುಹಿಡಿ_Int_DType()};
            Ast_Foreach_ಶೀರ್ಷಿಕೆ* ಸ್ಥಿರಾಂಕ headerp
                = ಹೊಸ Ast_Foreach_ಶೀರ್ಷಿಕೆ{fl, nodep->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), loopVarp};

            // Determine SMT operation and compute identity elements
            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* smtOp = ಶೂನ್ಯಸೂಚಕ;
            std::string ತಟಸ್ಥ;
            ಆದರೆ (withp) {
                // For 'with' clause: use binary format, compute from result width
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->dtypep()->ಅಗಲ();
                ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_ಮೊತ್ತ) {
                    smtOp = "bvadd";
                    ತಟಸ್ಥ = "#b" + std::string(ಅಗಲ, '0');
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_ಗುಣಲಬ್ಧ) {
                    smtOp = "bvmul";
                    ತಟಸ್ಥ = (ಅಗಲ > 0) ? "#b" + std::string(ಅಗಲ - 1, '0') + "1" : "#b0";
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    ತಟಸ್ಥ = "#b" + std::string(ಅಗಲ, '1');
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    ತಟಸ್ಥ = "#b" + std::string(ಅಗಲ, '0');
                } ಇಲ್ಲದಿದ್ದರೆ {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    ತಟಸ್ಥ = "#b" + std::string(ಅಗಲ, '0');
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ಸ್ಥಿರಾಂಕ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ಸ್ಥಿರಾಂಕ arrVarp = arrRefp->varp();
                Ast_ಗಂಟು_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elem_ಅಗಲ = elemDtp->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ hexDigits = (elem_ಅಗಲ + 3) / 4;

                ಸ್ಥಿರಾಂಕ std::string zero_ತಟಸ್ಥ = "#x" + std::string(hexDigits, '0');
                ಸ್ಥಿರಾಂಕ std::string one_ತಟಸ್ಥ = "#x" + std::string(hexDigits - 1, '0') + "1";
                ಸ್ಥಿರಾಂಕ std::string all_Ones_ತಟಸ್ಥ = "#x" + std::string(hexDigits, 'f');

                ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_ಮೊತ್ತ) {
                    smtOp = "bvadd";
                    ತಟಸ್ಥ = zero_ತಟಸ್ಥ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_ಗುಣಲಬ್ಧ) {
                    smtOp = "bvmul";
                    ತಟಸ್ಥ = one_ತಟಸ್ಥ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    ತಟಸ್ಥ = all_Ones_ತಟಸ್ಥ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    ತಟಸ್ಥ = zero_ತಟಸ್ಥ;
                } ಇಲ್ಲದಿದ್ದರೆ {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    ತಟಸ್ಥ = zero_ತಟಸ್ಥ;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ಸ್ಥಿರಾಂಕ cstmtp = ಹೊಸ AstCStmt{fl};
            ಆದರೆ (withp) {
                // With 'with' clause: evaluate expression for each element
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ randArr = nodep->fromp()->user1();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ idxRefp = ಹೊಸ AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಫಲಿತಾಂಶ_ಅಗಲ = nodep->dtypep()->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ VSigning ಫಲಿತಾಂಶ_Signing = nodep->dtypep()->numeric();

                Ast_ಗಂಟು* ಪ್ರತಿ_Elem_Exprp = withp->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                ಆದರೆ (AstLambdaArgRef* ಸ್ಥಿರಾಂಕ ಬೇರು_Refp = VN_CAST(ಪ್ರತಿ_Elem_Exprp, LambdaArgRef)) {
                    ಆದರೆ (ಬೇರು_Refp->ಸೂಚ್ಯಂಕ()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ಸ್ಥಿರಾಂಕ loopRef = ಹೊಸ AstVarRef{fl, loopVarp, VAccess::READ};
                        ಪ್ರತಿ_Elem_Exprp = adjust_ಅಗಲ(fl, loopRef, ಫಲಿತಾಂಶ_ಅಗಲ, ಫಲಿತಾಂಶ_Signing);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // item at root -> replace with array element
                        ಪ್ರತಿ_Elem_Exprp = elemSelp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    }
                    VL_DO_DANGLING(ಬೇರು_Refp->delete_ವೃಕ್ಷ(), ಬೇರು_Refp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಪ್ರತಿ_Elem_Exprp->foreach([&](AstLambdaArgRef* refp) {
                        ಆದರೆ (refp->ಸೂಚ್ಯಂಕ()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ಸ್ಥಿರಾಂಕ loopRef = ಹೊಸ AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ replacement
                                = adjust_ಅಗಲ(fl, loopRef, ಫಲಿತಾಂಶ_ಅಗಲ, ಫಲಿತಾಂಶ_Signing);
                            refp->replaceWith(replacement);
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                        }
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_ವೃಕ್ಷ(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                ಪ್ರತಿ_Elem_Exprp = V3Const::constifyEdit(ಪ್ರತಿ_Elem_Exprp);

                ಪ್ರತಿ_Elem_Exprp->foreach([&](Ast_ಗಂಟು* nodep) {
                    // Don't mark loop variable references as randomizable
                    ಆದರೆ (!VN_IS(nodep, Const)) {
                        ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
                            ಆದರೆ (vrefp->varp() != loopVarp) nodep->user1(ನಿಜ);
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            nodep->user1(ನಿಜ);
                        }
                    }
                });

                cstmtp->ಕೂಡಿಸು("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->ಕೂಡಿಸು("ret += ");
                cstmtp->ಕೂಡಿಸು(iterateSubtreeReturnEdits(ಪ್ರತಿ_Elem_Exprp));
                cstmtp->ಕೂಡಿಸು(";\n");
                cstmtp->ಕೂಡಿಸು("ret += \")\";\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ಸ್ಥಿರಾಂಕ arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ಸ್ಥಿರಾಂಕ arrVarp = arrRefp->varp();
                ಸ್ಥಿರಾಂಕ std::string smtArrayName = arrVarp->name();
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elem_ಅಗಲ = elemDtp->ಅಗಲ();
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ arrModulep = VN_AS(arrVarp->user2p(), ಗಂಟು_Module);
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ genModulep = VN_AS(m_genp->user2p(), ಗಂಟು_Module);
                arrVarp->user3(ನಿಜ);

                // Add write_var call to init task
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ arrVarNamep
                    = ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "\"" + smtArrayName + "\"", elem_ಅಗಲ};
                AstCMethodHard* ಸ್ಥಿರಾಂಕ writeVarCallp
                    = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->ಕೂಡಿಸು_Pinsp(
                    ಹೊಸ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elem_ಅಗಲ)});
                writeVarCallp->ಕೂಡಿಸು_Pinsp(arrVarNamep);
                ಸ್ಥಿರಾಂಕ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(ಸುಳ್ಳು).ಎರಡನೇ;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{fl, 1});  // Dimension

                ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {.asUQuad = arrVarp->user1()};
                ಆದರೆ (rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) {
                    writeVarCallp->ಕೂಡಿಸು_Pinsp(
                        ಹೊಸ AstConst{fl, AstConst::Unsized64{}, rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ});
                }
                writeVarCallp->dtype_ಗಣ_Void();
                Ast_ಗಂಟು_FTask* initTaskp = m_inlineInitTaskp;
                ಆದರೆ (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(arrModulep, "new"), ಗಂಟು_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->ಕೂಡಿಸು_Stmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ಸ್ಥಿರಾಂಕ idxFormatp = ಹೊಸ AstSFormatF{
                    fl, "#x%08x", ಸುಳ್ಳು, ಹೊಸ AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ಸ್ಥಿರಾಂಕ varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), idxFormatp);

                cstmtp->ಕೂಡಿಸು("ret += \" \";\n");
                cstmtp->ಕೂಡಿಸು("ret += ");
                cstmtp->ಕೂಡಿಸು(varNamep);
                cstmtp->ಕೂಡಿಸು(";\n");
            }

            // Build final expression
            ಆದರೆ (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ಸ್ಥಿರಾಂಕ cexprp = ಹೊಸ AstCExpr{fl};
                cexprp->dtype_ಗಣ_String();
                cexprp->ಕೂಡಿಸು("([&]{\nstd::string ret = \"" + ತಟಸ್ಥ + "\";\n");
                cexprp->ಕೂಡಿಸು(ಹೊಸ AstBegin{fl, m_unique_ಹೆಸರುಗಳು.get("__VrandBlock"),
                                         ಹೊಸ AstForeach{fl, headerp, cstmtp}, ನಿಜ});
                cexprp->ಕೂಡಿಸು("return ret;\n})()");
                nodep->replaceWith(ಹೊಸ AstSFormatF{fl, "%s", ಸುಳ್ಳು, cexprp});
            } ಇಲ್ಲದಿದ್ದರೆ {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ಸ್ಥಿರಾಂಕ cexprp = ಹೊಸ AstCExpr{fl};
                cexprp->dtype_ಗಣ_String();
                cexprp->ಕೂಡಿಸು("([&]{\nstd::string ret;\n");
                cexprp->ಕೂಡಿಸು(ಹೊಸ AstBegin{fl, m_unique_ಹೆಸರುಗಳು.get("__VrandBlock"),
                                         ಹೊಸ AstForeach{fl, headerp, cstmtp}, ನಿಜ});
                cexprp->ಕೂಡಿಸು("return ret.empty() ? \"" + ತಟಸ್ಥ + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(ಹೊಸ AstSFormatF{fl, "%s", ಸುಳ್ಳು, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // We've handled DYN_SIZE already if they should be randomized
        ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::DYN_ಗಾತ್ರ || nodep->ವಿಧಾನ() == VCMethod::ASSOC_ಗಾತ್ರ) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಗಂಟು_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            ಆಗ (VN_IS(ಗಂಟು_Exprp, MemberSel)) {
                ಗಂಟು_Exprp = VN_AS(ಗಂಟು_Exprp, MemberSel)->fromp();
            }
            ಆದರೆ (VN_IS(ಗಂಟು_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)));
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(ಸುಳ್ಳು);

        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;

        AstFunc* ಸ್ಥಿರಾಂಕ funcp = VN_CAST(nodep->taskp(), Func);
        ಆದರೆ (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            ಹಿಂತಿರುಗಿ;
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ retExprp = ಹೊರತೆಗೆ_Return_ಅಭಿವ್ಯಕ್ತಿ(funcp);
        ಆದರೆ (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(ಸುಳ್ಳು);
            ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
            ಹಿಂತಿರುಗಿ;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ಸ್ಥಿರಾಂಕ V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inlinedp = retExprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tconnect : tconnects) {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ portp = tconnect.ಮೊದಲ;
            AstArg* ಸ್ಥಿರಾಂಕ argp = tconnect.ಎರಡನೇ;
            ಆದರೆ (!argp || !argp->exprp()) ಮುಂದುವರಿ;
            ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ ಬೇರು_Refp = VN_CAST(inlinedp, VarRef)) {
                ಆದರೆ (ಬೇರು_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    VL_DO_DANGLING(ಬೇರು_Refp->delete_ವೃಕ್ಷ(), ಬೇರು_Refp);
                    ಮುಂದುವರಿ;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                ಆದರೆ (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
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
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಆದರೆ (editFormat(nodep)) ಹಿಂತಿರುಗಿ;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_ಗಂಟು* buildDisableSoftCallStmt(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        std::string varName;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ vrefp = VN_CAST(nodep->exprp(), ಗಂಟು_Var_Ref)) {
            varName = vrefp->name();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCMethodHard{
            nodep->fileline(),
            ಹೊಸ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ಗಂಟು_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            ಹೊಸ AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtype_ಗಣ_Void();
        ಹಿಂತಿರುಗಿ callp->makeStmt();
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
    Ast_ಗಂಟು* ಹೊರತೆಗೆ_Conditional_Disable_Softs(Ast_ಗಂಟು* itemsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outerCondp) {
        Ast_ಗಂಟು* hoistListp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ಆದರೆ (AstConstraintIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thenCondp
                    = outerCondp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                       ಹೊಸ AstLogAnd{fl, outerCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                     ifp->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)})
                                 : ifp->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thenHoistp
                    = ಹೊರತೆಗೆ_Conditional_Disable_Softs(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_ವೃಕ್ಷ(), thenCondp);
                ಆದರೆ (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ notInnerp
                        = ಹೊಸ AstLogNot{fl, ifp->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elseCondp
                        = outerCondp ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ಹೊಸ AstLogAnd{
                                           fl, outerCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), notInnerp})
                                     : notInnerp;
                    ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ elseHoistp
                        = ಹೊರತೆಗೆ_Conditional_Disable_Softs(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_ವೃಕ್ಷ(), elseCondp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(stmtp, Constraint_ಅಭಿವ್ಯಕ್ತಿ)) {
                ಆದರೆ (exprp->isDisableSoft() && outerCondp) {
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = exprp->fileline();
                    AstIf* ಸ್ಥಿರಾಂಕ hoistIfp = ಹೊಸ AstIf{fl, outerCondp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                                      buildDisableSoftCallStmt(exprp), ಶೂನ್ಯಸೂಚಕ};
                    hoistListp = hoistListp ? Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(hoistListp,
                                                               static_cast<Ast_ಗಂಟು*>(hoistIfp))
                                            : static_cast<Ast_ಗಂಟು*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ truep
                        = ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        ಹಿಂತಿರುಗಿ hoistListp;
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_ಗಂಟು* nodep,
                                   Ast_ಗಂಟು_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* rand_ಬಹುಲಕ_Varp, std::ಗಣ<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3_Unique_ಹೆಸರುಗಳು& unique_ಹೆಸರುಗಳು,
                                   Ast_ಗಂಟು_FTask* memberselInitTaskp = ಶೂನ್ಯಸೂಚಕ,
                                   VInsertion_ಗಣ<AstVar*>* ಗಾತ್ರ_Constrained_Arraysp = ಶೂನ್ಯಸೂಚಕ,
                                   Ast_ಗಂಟು_FTask* prepareConstrainedArraysp = ಶೂನ್ಯಸೂಚಕ)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_rand_ಬಹುಲಕ_Varp{rand_ಬಹುಲಕ_Varp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_ಗಾತ್ರ_Constrained_Arraysp{ಗಾತ್ರ_Constrained_Arraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_unique_ಹೆಸರುಗಳು{unique_ಹೆಸರುಗಳು} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ hoistListp
            = nodep ? ಹೊರತೆಗೆ_Conditional_Disable_Softs(nodep, ಶೂನ್ಯಸೂಚಕ) : ಶೂನ್ಯಸೂಚಕ) {
            nodep->ಕೂಡಿಸು_ಮುಂದಿನ(hoistListp);
        }
        iterate_And_ಮುಂದಿನ_Null(nodep);
    }
};

ಎಣಿಕೆ ವರ್ಗ Capture_ಬಹುಲಕ : uint8_t {
    CAP_NO = 0x0,
    CAP_ಮೌಲ್ಯ = 0x01,
    CAP_THIS = 0x02,
    CAP_F_ಗಣ_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
Capture_ಬಹುಲಕ ಕ್ರಿಯಾಕಾರಕ|(Capture_ಬಹುಲಕ a, Capture_ಬಹುಲಕ b) {
    ಹಿಂತಿರುಗಿ static_cast<Capture_ಬಹುಲಕ>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
Capture_ಬಹುಲಕ ಕ್ರಿಯಾಕಾರಕ&(Capture_ಬಹುಲಕ a, Capture_ಬಹುಲಕ b) {
    ಹಿಂತಿರುಗಿ static_cast<Capture_ಬಹುಲಕ>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
Capture_ಬಹುಲಕ ಬಹುಲಕ(Capture_ಬಹುಲಕ a) { ಹಿಂತಿರುಗಿ a & static_cast<Capture_ಬಹುಲಕ>(0x3); }
ತಾರ್ಕಿಕ hasFlags(Capture_ಬಹುಲಕ a, Capture_ಬಹುಲಕ flags) {
    ಹಿಂತಿರುಗಿ ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

ವರ್ಗ CaptureVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    AstArg* m_argsp = ಶೂನ್ಯಸೂಚಕ;  // Original references turned into arguments
    Ast_ಗಂಟು_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ಸ್ಥಿರಾಂಕ AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::ಗಣ<Ast_ಗಂಟು*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = ಶೂನ್ಯಸೂಚಕ;  // Variable for outer context's object, if necessary
    ತಾರ್ಕಿಕ m_staticContext = ಸುಳ್ಳು;  // True when capturing from a static function context

    // METHODS

    ತಾರ್ಕಿಕ captureVariable(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fileline, Ast_ಗಂಟು_Var_Ref* varrefp, AstVar*& varp) {
        ಸ್ವಯಂಚಾಲಿತ it = m_varCloneMap.ಕಂಡುಹಿಡಿ(varrefp->varp());
        ಆದರೆ (it == m_varCloneMap.ಕೊನೆ()) {
            AstVar* ಸ್ಥಿರಾಂಕ newVarp = varrefp->varp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(ನಿಜ);
            newVarp->direction(VDirection::ಒಳಹರಿವು);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        varp = it->ಎರಡನೇ;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಗಂಟು>
    ಶೂನ್ಯ fixupClassOrPackage(Ast_ಗಂಟು* memberp, T_ಗಂಟು refp) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ declClassp = VN_AS(memberp->user2p(), ಗಂಟು_Module);
        ಆದರೆ (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_ಗಂಟು>
    ತಾರ್ಕಿಕ is_Reference_ಗಮ್ಯ_Inner_Member(T_ಗಂಟು nodep) {
        ಹಿಂತಿರುಗಿ VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_ಸಾಲು* fl) {
        ಆದರೆ (!m_thisp) {
            AstClassRefDType* ಸ್ಥಿರಾಂಕ refDTypep
                = ಹೊಸ AstClassRefDType{fl, VN_AS(m_callerp, Class), ಶೂನ್ಯಸೂಚಕ};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(refDTypep);
            m_thisp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(ನಿಜ);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::ಒಳಹರಿವು);
            m_argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_argsp, ಹೊಸ AstArg{fl, "", ಹೊಸ AstThisRef{fl, refDTypep}});
        }
        ಹಿಂತಿರುಗಿ m_thisp;
    }

    AstVar* getVar(AstVar* ಸ್ಥಿರಾಂಕ varp) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_varCloneMap.ಕಂಡುಹಿಡಿ(varp);
        ಆದರೆ (it == m_varCloneMap.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (it->ಎರಡನೇ->isStdRandomizeArg()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    }

    Capture_ಬಹುಲಕ get_Var_Ref_Capture_ಬಹುಲಕ(Ast_ಗಂಟು_Var_Ref* varRefp) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ varModp = VN_AS(varRefp->varp()->user2p(), ಗಂಟು_Module);
        AstClass* ಸ್ಥಿರಾಂಕ varClassp = VN_CAST(varModp, Class);
        AstClass* ಸ್ಥಿರಾಂಕ callerClassp = VN_CAST(m_callerp, Class);

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ callerIsClass = callerClassp;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ refIsXref = VN_IS(varRefp, VarXRef);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ var_Is_ಕ್ಷೇತ್ರ_ದ_Caller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ varIsParam = varRefp->varp()->isParam();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ varIsConstraintIterator
            = VN_IS(varRefp->varp()->ಮೊದಲ_Abovep(), Foreach_ಶೀರ್ಷಿಕೆ)
              && VN_IS(varRefp->varp()->ಮೊದಲ_Abovep()->ಮೊದಲ_Abovep(), ConstraintForeach);
        ಆದರೆ (refIsXref) ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_ಮೌಲ್ಯ | Capture_ಬಹುಲಕ::CAP_F_XREF;
        ಆದರೆ (varIsConstraintIterator) ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_NO;
        ಆದರೆ (varIsFuncLocal && varHasAutomaticLifetime) ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_ಮೌಲ್ಯ;
        ಆದರೆ (varIsParam) ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_ಮೌಲ್ಯ;
        // Static var in function (will not be inlined, because it's in class)
        ಆದರೆ (callerIsClass && varIsFuncLocal) ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_ಮೌಲ್ಯ;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        ಆದರೆ (m_staticContext && callerIsClass && var_Is_ಕ್ಷೇತ್ರ_ದ_Caller
            && varRefp->varp()->lifetime().isStatic())
            ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_NO;
        ಆದರೆ (callerIsClass && var_Is_ಕ್ಷೇತ್ರ_ದ_Caller) ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        ಆದರೆ (callerIsClass && varClassp && !var_Is_ಕ್ಷೇತ್ರ_ದ_Caller
            && varRefp->varp()->lifetime().isStatic())
            ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_ಮೌಲ್ಯ;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        ಹಿಂತಿರುಗಿ Capture_ಬಹುಲಕ::CAP_ಮೌಲ್ಯ;
    }

    ಶೂನ್ಯ capture_Ref_By_ಮೌಲ್ಯ(Ast_ಗಂಟು_Var_Ref* nodep, Capture_ಬಹುಲಕ cap_ಬಹುಲಕ_Flags) {
        AstVar* newVarp;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ newVarRefp = newCapture ? nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!hasFlags(cap_ಬಹುಲಕ_Flags, Capture_ಬಹುಲಕ::CAP_F_ಗಣ_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        ಆದರೆ (!newCapture) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (hasFlags(cap_ಬಹುಲಕ_Flags, Capture_ಬಹುಲಕ::CAP_F_XREF)) {
            AstVarRef* ಸ್ಥಿರಾಂಕ notXVarRefp
                = ಹೊಸ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(m_argsp, ಹೊಸ AstArg{nodep->fileline(), "", newVarRefp});
    }

    ಶೂನ್ಯ captureRefByThis(Ast_ಗಂಟು_Var_Ref* nodep, Capture_ಬಹುಲಕ /*capModeFlags*/) {
        AstVar* ಸ್ಥಿರಾಂಕ thisp = importThisp(nodep->fileline());
        AstVarRef* ಸ್ಥಿರಾಂಕ thisRefp = ಹೊಸ AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(ನಿಜ);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp
            = ಹೊಸ AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        ಆದರೆ (!m_targetp && nodep->user1()) memberSelp->user1(ನಿಜ);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (m_ignore.ಎಣಿಕೆ_2(nodep)) ಹಿಂತಿರುಗಿ;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        ಆದರೆ (nodep->varp()->isStdRandomizeArg()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ Capture_ಬಹುಲಕ cap_ಬಹುಲಕ = get_Var_Ref_Capture_ಬಹುಲಕ(nodep);
        ಆದರೆ (ಬಹುಲಕ(cap_ಬಹುಲಕ) == Capture_ಬಹುಲಕ::CAP_NO) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (ಬಹುಲಕ(cap_ಬಹುಲಕ) == Capture_ಬಹುಲಕ::CAP_ಮೌಲ್ಯ) capture_Ref_By_ಮೌಲ್ಯ(nodep, cap_ಬಹುಲಕ);
        ಆದರೆ (ಬಹುಲಕ(cap_ಬಹುಲಕ) == Capture_ಬಹುಲಕ::CAP_THIS) captureRefByThis(nodep, cap_ಬಹುಲಕ);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (m_ignore.ಎಣಿಕೆ_2(nodep)) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        ಆದರೆ (VN_IS(nodep, ವಿಧಾನ_Call) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep->taskp()->user2p(), Class);
        ಆದರೆ ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ಸ್ಥಿರಾಂಕ argsp = nodep->argsp();
            ಆದರೆ (argsp) argsp->unlink_Fr_ಹಿಂಭಾಗ();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ಸ್ಥಿರಾಂಕ thisp = importThisp(nodep->fileline());
            AstVarRef* ಸ್ಥಿರಾಂಕ thisRefp = ಹೊಸ AstVarRef{
                nodep->fileline(), thisp, nodep->is_ಶುದ್ಧ() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp
                = ಹೊಸ Ast_ವಿಧಾನ_Call{nodep->fileline(), thisRefp, thisp->name(), argsp};
            ವಿಧಾನ_Callp->taskp(nodep->taskp());
            ವಿಧಾನ_Callp->dtypep(nodep->dtypep());
            nodep->replaceWith(ವಿಧಾನ_Callp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            m_ignore.emplace(ವಿಧಾನ_Callp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        ಆದರೆ (!is_Reference_ಗಮ್ಯ_Inner_Member(nodep)) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        AstVarRef* ಸ್ಥಿರಾಂಕ varRefp
            = ಹೊಸ AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ವಿಧಾನ_Call* nodep) override {
        ಆದರೆ (!is_Reference_ಗಮ್ಯ_Inner_Member(nodep) || m_ignore.ಎಣಿಕೆ_2(nodep)) {
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        AstArg* ಸ್ಥಿರಾಂಕ argsp = nodep->argsp();
        ಆದರೆ (argsp) argsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        Ast_ಗಂಟು_FTask_Ref* taskRefp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = ಹೊಸ AstTaskRef{nodep->fileline(), taskp, argsp};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstFunc* ಸ್ಥಿರಾಂಕ taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = ಹೊಸ AstFuncRef{nodep->fileline(), taskp, argsp};
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ CaptureVisitor(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, Ast_ಗಂಟು_Module* callerp, AstClass* ಸ್ಥಿರಾಂಕ targetp,
                            ತಾರ್ಕಿಕ staticContext = ಸುಳ್ಳು)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterate_And_ಮುಂದಿನ_Null(nodep);
    }

    // PUBLIC METHODS

    AstArg* get_ವಾದಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_argsp; }
    AstVar* getThisp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_thisp; }

    ಶೂನ್ಯ ಕೂಡಿಸು_Function_Arguments(Ast_ಗಂಟು_FTask* funcp) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (AstArg* argp = get_ವಾದಗಳು(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ಆದರೆ (Ast_ಗಂಟು_Var_Ref* varrefp = VN_CAST(argp->exprp(), ಗಂಟು_Var_Ref)) {
                ಆದರೆ ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(ಶೂನ್ಯಸೂಚಕ);
                }
                funcp->ಕೂಡಿಸು_Stmtsp(getVar(varrefp->varp()));
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->ಕೂಡಿಸು_Stmtsp(m_thisp);
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
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_inuser4;

    // STATE
    V3_Unique_ಹೆಸರುಗಳು m_inline_Unique_ಹೆಸರುಗಳು;  // For generating unique function names
    V3_Unique_ಹೆಸರುಗಳು m_rand_Array_Unique_ಹೆಸರುಗಳು{"__Vrandarr"};  // Random array loop indices
    V3_Unique_ಹೆಸರುಗಳು m_ಬಹುಲಕ_Unique_ಹೆಸರುಗಳು{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3_Unique_ಹೆಸರುಗಳು m_inlineUniqueStdName{"__VStdrand"};
    V3_Unique_ಹೆಸರುಗಳು m_unique_ಹೆಸರುಗಳು;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    std::unordered_map<Ast_ಗಂಟು_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    Ast_ಗಂಟು_Stmt* m_stmtp = ಶೂನ್ಯಸೂಚಕ;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = ಶೂನ್ಯಸೂಚಕ;  // Dynamic array type (for rand mode)
    ಪೂರ್ಣಾಂಕ m_rand_Case_ಅಂಕೆ = 0;  // Randcase number within a module for var naming
    ಪೂರ್ಣಾಂಕ m_dist_ಅಂಕೆ = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = ಶೂನ್ಯಸೂಚಕ;  // Current constraint
    std::ಗಣ<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertion_ಗಣ<AstVar*>> m_ಗಾತ್ರ_Constrained_Arrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_static_Constraint_ಬಹುಲಕ_Vars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_static_Rand_ಬಹುಲಕ_Vars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Similar_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* with_ಅಭಿವ್ಯಕ್ತಿ, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arg_ಅಭಿವ್ಯಕ್ತಿ) {
        // VarRef: compare variable pointers
        ಆದರೆ (VN_IS(arg_ಅಭಿವ್ಯಕ್ತಿ, VarRef) && VN_IS(with_ಅಭಿವ್ಯಕ್ತಿ, VarRef)) {
            ಹಿಂತಿರುಗಿ VN_AS(with_ಅಭಿವ್ಯಕ್ತಿ, VarRef)->varp() == VN_AS(arg_ಅಭಿವ್ಯಕ್ತಿ, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        ಆದರೆ (VN_IS(arg_ಅಭಿವ್ಯಕ್ತಿ, MemberSel) && VN_IS(with_ಅಭಿವ್ಯಕ್ತಿ, MemberSel)) {
            ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ withMSp = VN_AS(with_ಅಭಿವ್ಯಕ್ತಿ, MemberSel);
            ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ argMSp = VN_AS(arg_ಅಭಿವ್ಯಕ್ತಿ, MemberSel);
            ಆದರೆ (withMSp->varp() != argMSp->varp()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Recursively compare the base object expression
            ಹಿಂತಿರುಗಿ is_Similar_ಗಂಟು(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        ಆದರೆ (VN_IS(arg_ಅಭಿವ್ಯಕ್ತಿ, ArraySel) && VN_IS(with_ಅಭಿವ್ಯಕ್ತಿ, ArraySel)) {
            ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ withASp = VN_AS(with_ಅಭಿವ್ಯಕ್ತಿ, ArraySel);
            ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ argASp = VN_AS(arg_ಅಭಿವ್ಯಕ್ತಿ, ArraySel);
            // Before checking Sel type index, check if both are Const
            ಆದರೆ (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ withIdxp = VN_AS(withASp->bitp(), Const);
                ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ argIdxp = VN_AS(argASp->bitp(), Const);
                ಹಿಂತಿರುಗಿ is_Similar_ಗಂಟು(withASp->fromp(), argASp->fromp())
                       && withIdxp->ಅಂಕೆ().isCaseEq(argIdxp->ಅಂಕೆ());
            }
            // Index must be Sel type, extract VarRef using fromp()
            ಆದರೆ (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            ಹಿಂತಿರುಗಿ is_Similar_ಗಂಟು(withASp->fromp(), argASp->fromp())
                   && is_Similar_ಗಂಟು(withIdxp, argIdxp);
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    ಸ್ಥಿರ ಶೂನ್ಯ expandUniqueElementList(Ast_ಗಂಟು* itemsp) {
        Ast_ಗಂಟು* itemp = itemsp;
        ಆಗ (itemp) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = itemp->nextp();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConstraintIf* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConstraintForeach* ಸ್ಥಿರಾಂಕ foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                ಮುಂದುವರಿ;
            }
            AstConstraintUnique* ಸ್ಥಿರಾಂಕ uniquep = VN_CAST(itemp, ConstraintUnique);
            ಆದರೆ (!uniquep) {
                itemp = nextp;
                ಮುಂದುವರಿ;
            }

            std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು;
            ತಾರ್ಕಿಕ hasArrayVarRef = ಸುಳ್ಳು;
            ಗಾಗಿ (Ast_ಗಂಟು* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = vrp->varp()->dtypep()->skipRefp();
                    ಆದರೆ (VN_IS(dtypep, ಗಂಟು_Array_DType) || VN_IS(dtypep, ಸರತಿ_DType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = ನಿಜ;
                        ಮುಂದುವರಿ;
                    }
                }
                ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(VN_AS(rp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            }

            ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.ಗಾತ್ರ_2() >= 2) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = uniquep->fileline();
                ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.ಗಾತ್ರ_2(); i++) {
                    ಗಾಗಿ (ಗಾತ್ರ_t j = i + 1; j < ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.ಗಾತ್ರ_2(); j++) {
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು[i]->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು[j]->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                        AstNeq* ಸ್ಥಿರಾಂಕ neqp = ಹೊಸ AstNeq{fl, lhsp, rhsp};
                        neqp->user1(ನಿಜ);
                        Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ cexprp = ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, neqp};
                        uniquep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(cexprp);
                    }
                }
                ಆದರೆ (!hasArrayVarRef) {
                    uniquep->unlink_Fr_ಹಿಂಭಾಗ();
                    VL_DO_DANGLING(uniquep->delete_ವೃಕ್ಷ(), uniquep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಭಿವ್ಯಕ್ತಿ_ವಸ್ತುಗಳು.ಗಾತ್ರ_2() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlink_Fr_ಹಿಂಭಾಗ();
                VL_DO_DANGLING(uniquep->delete_ವೃಕ್ಷ(), uniquep);
            }
            itemp = nextp;
        }
    }
    ಶೂನ್ಯ createRandomGenerator(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಆದರೆ (classp->user3p()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            ಹಿಂತಿರುಗಿ;
        }
        AstVar* ಸ್ಥಿರಾಂಕ genp = ಹೊಸ AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->ಕೂಡಿಸು_Membersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp) {
        ಸ್ವಯಂಚಾಲಿತ it = m_stdMap.ಕಂಡುಹಿಡಿ(modp);
        ಆದರೆ (it == m_stdMap.ಕೊನೆ()) {
            AstVar* ಸ್ಥಿರಾಂಕ stdgenp
                = ಹೊಸ AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3_Error_ಸಂಕೇತ::IMPURE, ನಿಜ);
            modp->ಕೂಡಿಸು_Stmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            ಹಿಂತಿರುಗಿ stdgenp;
        }
        ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ name = "__Vprepare_constrained_arrays";
        AstTask* ಸ್ಥಿರಾಂಕ taskp = ಹೊಸ AstTask{classp->fileline(), name, ಶೂನ್ಯಸೂಚಕ};
        taskp->class_ವಿಧಾನ(ನಿಜ);
        classp->ಕೂಡಿಸು_Membersp(taskp);
        m_memberMap.ಸೇರಿಸು(classp, taskp);
        ಹಿಂತಿರುಗಿ taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, name), Task);
        ಆದರೆ (resizeTaskp) ಹಿಂತಿರುಗಿ resizeTaskp;
        resizeTaskp = ಹೊಸ AstTask{classp->fileline(), name, ಶೂನ್ಯಸೂಚಕ};
        resizeTaskp->class_ವಿಧಾನ(ನಿಜ);
        resizeTaskp->isVirtual(ನಿಜ);
        classp->ಕೂಡಿಸು_Membersp(resizeTaskp);
        m_memberMap.ಸೇರಿಸು(classp, resizeTaskp);
        ಹಿಂತಿರುಗಿ resizeTaskp;
    }
    AstVar* get_Create_Rand_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಆದರೆ (classp->user2p()) ಹಿಂತಿರುಗಿ VN_AS(classp->user2p(), Var);
        ಆದರೆ (AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classp->extendsp()) {
            ಹಿಂತಿರುಗಿ get_Create_Rand_ಬಹುಲಕ_Var(extendsp->classp());
        }
        AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Varp = create_ಬಹುಲಕ_Var(classp, "__Vrandmode");
        classp->user2p(rand_ಬಹುಲಕ_Varp);
        ಹಿಂತಿರುಗಿ rand_ಬಹುಲಕ_Varp;
    }
    AstVar* get_Create_Constraint_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಆದರೆ (classp->user4p()) ಹಿಂತಿರುಗಿ VN_AS(classp->user4p(), Var);
        ಆದರೆ (AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classp->extendsp()) {
            ಹಿಂತಿರುಗಿ get_Create_Constraint_ಬಹುಲಕ_Var(extendsp->classp());
        }
        AstVar* ಸ್ಥಿರಾಂಕ constraint_ಬಹುಲಕ_Varp = create_ಬಹುಲಕ_Var(classp, "__Vconstraintmode");
        classp->user4p(constraint_ಬಹುಲಕ_Varp);
        ಹಿಂತಿರುಗಿ constraint_ಬಹುಲಕ_Varp;
    }
    ಸ್ಥಿರ AstVar* get_Constraint_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಆದರೆ (classp->user4p()) ಹಿಂತಿರುಗಿ VN_AS(classp->user4p(), Var);
        ಆದರೆ (AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classp->extendsp()) {
            ಹಿಂತಿರುಗಿ get_Constraint_ಬಹುಲಕ_Var(extendsp->classp());
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    AstVar* get_Create_Static_Constraint_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಸ್ವಯಂಚಾಲಿತ it = m_static_Constraint_ಬಹುಲಕ_Vars.ಕಂಡುಹಿಡಿ(classp);
        ಆದರೆ (it != m_static_Constraint_ಬಹುಲಕ_Vars.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
        ಆದರೆ (AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classp->extendsp()) {
            ಹಿಂತಿರುಗಿ get_Create_Static_Constraint_ಬಹುಲಕ_Var(extendsp->classp());
        }
        AstVar* ಸ್ಥಿರಾಂಕ static_ಬಹುಲಕ_Varp = create_Static_ಬಹುಲಕ_Var(classp, "__Vstaticconstraintmode");
        m_static_Constraint_ಬಹುಲಕ_Vars[classp] = static_ಬಹುಲಕ_Varp;
        ಹಿಂತಿರುಗಿ static_ಬಹುಲಕ_Varp;
    }
    AstVar* get_Static_Constraint_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಸ್ವಯಂಚಾಲಿತ it = m_static_Constraint_ಬಹುಲಕ_Vars.ಕಂಡುಹಿಡಿ(classp);
        ಆದರೆ (it != m_static_Constraint_ಬಹುಲಕ_Vars.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
        ಆದರೆ (AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classp->extendsp()) {
            ಹಿಂತಿರುಗಿ get_Static_Constraint_ಬಹುಲಕ_Var(extendsp->classp());
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    AstVar* get_Create_Static_Rand_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಆದರೆ (m_static_Rand_ಬಹುಲಕ_Vars.ಎಣಿಕೆ_2(classp)) ಹಿಂತಿರುಗಿ m_static_Rand_ಬಹುಲಕ_Vars[classp];
        ಆದರೆ (AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classp->extendsp()) {
            ಹಿಂತಿರುಗಿ get_Create_Static_Rand_ಬಹುಲಕ_Var(extendsp->classp());
        }
        AstVar* ಸ್ಥಿರಾಂಕ static_ಬಹುಲಕ_Varp = create_Static_ಬಹುಲಕ_Var(classp, "__Vstaticrandmode");
        m_static_Rand_ಬಹುಲಕ_Vars[classp] = static_ಬಹುಲಕ_Varp;
        ಹಿಂತಿರುಗಿ static_ಬಹುಲಕ_Varp;
    }
    AstVar* get_Static_Rand_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಆದರೆ (m_static_Rand_ಬಹುಲಕ_Vars.ಎಣಿಕೆ_2(classp)) ಹಿಂತಿರುಗಿ m_static_Rand_ಬಹುಲಕ_Vars[classp];
        ಆದರೆ (AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classp->extendsp()) {
            ಹಿಂತಿರುಗಿ get_Static_Rand_ಬಹುಲಕ_Var(extendsp->classp());
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    AstVar* create_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ name) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = classp->fileline();
        ಆದರೆ (!m_dynarrayDtp) {
            m_dynarrayDtp = ಹೊಸ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ಸ್ಥಿರಾಂಕ ಬಹುಲಕ_Varp = ಹೊಸ AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        ಬಹುಲಕ_Varp->user2p(classp);
        classp->ಕೂಡಿಸು_Stmtsp(ಬಹುಲಕ_Varp);
        ಹಿಂತಿರುಗಿ ಬಹುಲಕ_Varp;
    }
    AstVar* create_Static_ಬಹುಲಕ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = classp->fileline();
        ಆದರೆ (!m_dynarrayDtp) {
            m_dynarrayDtp = ಹೊಸ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(m_dynarrayDtp);
        }
        AstVar* ಸ್ಥಿರಾಂಕ ಬಹುಲಕ_Varp = ಹೊಸ AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        ಬಹುಲಕ_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        ಬಹುಲಕ_Varp->user2p(classp);
        classp->ಕೂಡಿಸು_Stmtsp(ಬಹುಲಕ_Varp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.ಸೇರಿಸು(classp, ಬಹುಲಕ_Varp);
        ಹಿಂತಿರುಗಿ ಬಹುಲಕ_Varp;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_ಗಣ_Rand_ಬಹುಲಕ(Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp, AstVar* ಸ್ಥಿರಾಂಕ genp,
                               AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Varp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ftaskp->fileline();
        AstCMethodHard* ಸ್ಥಿರಾಂಕ ಗಣ_Rand_Modep = ಹೊಸ AstCMethodHard{
            fl, ಹೊಸ AstVarRef{fl, VN_AS(genp->user2p(), ಗಂಟು_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_ಗಣ_RANDMODE,
            ಹೊಸ AstVarRef{fl, VN_AS(rand_ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module), rand_ಬಹುಲಕ_Varp,
                          VAccess::READ}};
        ಗಣ_Rand_Modep->dtype_ಗಣ_Void();
        ftaskp->ಕೂಡಿಸು_Stmtsp(ಗಣ_Rand_Modep->makeStmt());
    }
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_ಗಣ_Static_Rand_ಬಹುಲಕ(Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp, AstVar* ಸ್ಥಿರಾಂಕ genp,
                                     AstVar* ಸ್ಥಿರಾಂಕ static_Rand_ಬಹುಲಕ_Varp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ftaskp->fileline();
        AstCMethodHard* ಸ್ಥಿರಾಂಕ setp = ಹೊಸ AstCMethodHard{
            fl, ಹೊಸ AstVarRef{fl, VN_AS(genp->user2p(), ಗಂಟು_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_ಗಣ_STATIC_RANDMODE,
            ಹೊಸ AstVarRef{fl, VN_AS(static_Rand_ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module), static_Rand_ಬಹುಲಕ_Varp,
                          VAccess::READ}};
        setp->dtype_ಗಣ_Void();
        ftaskp->ಕೂಡಿಸು_Stmtsp(setp->makeStmt());
    }
    ಶೂನ್ಯ createRandomizeClassVars(AstNetlist* ಸ್ಥಿರಾಂಕ netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> ಬೇರು_Static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ;
        netlistp->foreach([ಇದು, &ಬೇರು_Static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ](AstClass* ಸ್ಥಿರಾಂಕ classp) {
            ತಾರ್ಕಿಕ hasConstraints = ಸುಳ್ಳು;
            uint32_t rand_ಬಹುಲಕ_ಎಣಿಕೆ = 0;
            uint32_t static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ = 0;
            uint32_t constraint_ಬಹುಲಕ_ಎಣಿಕೆ = 0;
            uint32_t static_Constraint_ಬಹುಲಕ_ಎಣಿಕೆ = 0;
            classp->foreachMember([&](AstClass*, Ast_ಗಂಟು* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                ಆದರೆ (AstConstraint* ಸ್ಥಿರಾಂಕ constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = ನಿಜ;
                    Randomize_ಬಹುಲಕ constraint_ಬಹುಲಕ = {.asUQuad = memberp->user1()};
                    ಆದರೆ (!constraint_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) ಹಿಂತಿರುಗಿ;
                    ಆದರೆ (constraint_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ == 0) {
                        // Use separate index counters for static vs non-static constraints
                        ಆದರೆ (constrp->isStatic()) {
                            constraint_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ = static_Constraint_ಬಹುಲಕ_ಎಣಿಕೆ++;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            constraint_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ = constraint_ಬಹುಲಕ_ಎಣಿಕೆ++;
                        }
                        memberp->user1(constraint_ಬಹುಲಕ.asUQuad);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಆದರೆ (constrp->isStatic()) {
                            static_Constraint_ಬಹುಲಕ_ಎಣಿಕೆ = constraint_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ + 1;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            constraint_ಬಹುಲಕ_ಎಣಿಕೆ = constraint_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ + 1;
                        }
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(memberp, Var)) {
                    Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {.asUQuad = memberp->user1()};
                    ಆದರೆ (!rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) ಹಿಂತಿರುಗಿ;
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isStaticVar = varp->lifetime().isStatic();
                    ಆದರೆ (rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ == 0) {
                        ಆದರೆ (isStaticVar) {
                            rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ = static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ++;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ = rand_ಬಹುಲಕ_ಎಣಿಕೆ++;
                        }
                        memberp->user1(rand_ಬಹುಲಕ.asUQuad);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಆದರೆ (isStaticVar) {
                            static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ = rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ + 1;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            rand_ಬಹುಲಕ_ಎಣಿಕೆ = rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<ಶೂನ್ಯ(AstClass*)> ಕಂಡುಹಿಡಿ_Sub_Obj_Rand_Modes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_ಗಂಟು* subMemberp) {
                        ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ subVarp = VN_CAST(subMemberp, Var)) {
                            ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rm = {.asUQuad = subVarp->user1()};
                            ಆದರೆ (!rm.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) ಹಿಂತಿರುಗಿ;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            ಆದರೆ (subVarp->lifetime().isStatic()) ಹಿಂತಿರುಗಿ;
                            ಸ್ಥಿರಾಂಕ uint32_t needed = rm.ಸೂಚ್ಯಂಕ + 1;
                            ಆದರೆ (needed > rand_ಬಹುಲಕ_ಎಣಿಕೆ) rand_ಬಹುಲಕ_ಎಣಿಕೆ = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    ಆದರೆ (!memberVarp->globalConstrained()) ಹಿಂತಿರುಗಿ;
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = memberVarp->dtypep()->skipRefp();
                    ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(dtypep, ClassRefDType);
                    ಆದರೆ (!classRefp) ಹಿಂತಿರುಗಿ;
                    ಕಂಡುಹಿಡಿ_Sub_Obj_Rand_Modes(classRefp->classp());
                });
            }
            ಆದರೆ (hasConstraints) createRandomGenerator(classp);
            ಆದರೆ (rand_ಬಹುಲಕ_ಎಣಿಕೆ > 0) {
                AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Varp = get_Create_Rand_ಬಹುಲಕ_Var(classp);
                make_ಬಹುಲಕ_Init(rand_ಬಹುಲಕ_Varp, classp, rand_ಬಹುಲಕ_ಎಣಿಕೆ);
            }
            ಆದರೆ (constraint_ಬಹುಲಕ_ಎಣಿಕೆ > 0) {
                AstVar* ಸ್ಥಿರಾಂಕ constraint_ಬಹುಲಕ_Varp = get_Create_Constraint_ಬಹುಲಕ_Var(classp);
                make_ಬಹುಲಕ_Init(constraint_ಬಹುಲಕ_Varp, classp, constraint_ಬಹುಲಕ_ಎಣಿಕೆ);
            }
            ಆದರೆ (static_Constraint_ಬಹುಲಕ_ಎಣಿಕೆ > 0) {
                AstVar* ಸ್ಥಿರಾಂಕ static_Constraint_ಬಹುಲಕ_Varp = get_Create_Static_Constraint_ಬಹುಲಕ_Var(classp);
                make_Static_ಬಹುಲಕ_Init(static_Constraint_ಬಹುಲಕ_Varp, classp, static_Constraint_ಬಹುಲಕ_ಎಣಿಕೆ);
            }
            ಆದರೆ (static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ > 0) {
                get_Create_Static_Rand_ಬಹುಲಕ_Var(classp);
                AstClass* rootp = classp;
                ಆಗ (AstClassExtends* ಸ್ಥಿರಾಂಕ ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& ಸ್ಥಾನ_2 = ಬೇರು_Static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ[rootp];
                ಆದರೆ (static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ > ಸ್ಥಾನ_2) ಸ್ಥಾನ_2 = static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ;
            }
        });
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& kv : ಬೇರು_Static_Rand_ಬಹುಲಕ_ಎಣಿಕೆ) emit_ಬೇರು_Static_ಬಹುಲಕ_Init(kv.ಮೊದಲ, kv.ಎರಡನೇ);
    }
    ಶೂನ್ಯ emit_ಬೇರು_Static_ಬಹುಲಕ_Init(AstClass* ಸ್ಥಿರಾಂಕ rootp, ಸ್ಥಿರಾಂಕ uint32_t ಎಣಿಕೆ_2) {
        AstVar* ಸ್ಥಿರಾಂಕ static_Rand_ಬಹುಲಕ_Varp = m_static_Rand_ಬಹುಲಕ_Vars[rootp];
        UASSERT_OBJ(static_Rand_ಬಹುಲಕ_Varp, rootp, "Root must have a static rand-mode var");
        make_Static_ಬಹುಲಕ_Init(static_Rand_ಬಹುಲಕ_Varp, rootp, ಎಣಿಕೆ_2);
    }
    ಶೂನ್ಯ make_ಬಹುಲಕ_Init(AstVar* ಬಹುಲಕ_Varp, AstClass* classp, uint32_t ಬಹುಲಕ_ಎಣಿಕೆ) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ಬಹುಲಕ_Var_Modp = VN_AS(ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ಬಹುಲಕ_Varp->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ old_ಗಾತ್ರ_Varp = ಹೊಸ AstVar{
            fl, VVarType::BLOCKTEMP, m_unique_ಹೆಸರುಗಳು.get("__VoldSize"), ಬಹುಲಕ_Varp->ಕಂಡುಹಿಡಿ_UInt32_DType()};
        old_ಗಾತ್ರ_Varp->funcLocal(ನಿಜ);
        old_ಗಾತ್ರ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ಸ್ಥಿರಾಂಕ oldSizep
            = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, ಬಹುಲಕ_Var_Modp, ಬಹುಲಕ_Varp, VAccess::READ},
                                 VCMethod::DYN_ಗಾತ್ರ, ಶೂನ್ಯಸೂಚಕ};
        oldSizep->dtype_ಗಣ_UInt32();
        AstCMethodHard* ಸ್ಥಿರಾಂಕ dynarrayNewp
            = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, ಬಹುಲಕ_Var_Modp, ಬಹುಲಕ_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, ಹೊಸ AstConst{fl, ಬಹುಲಕ_ಎಣಿಕೆ}};
        dynarrayNewp->dtype_ಗಣ_Void();
        Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ctorNewp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "new"), ಗಂಟು_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ initFirstp = old_ಗಾತ್ರ_Varp;
        initFirstp->ಕೂಡಿಸು_ಮುಂದಿನ(
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, old_ಗಾತ್ರ_Varp, VAccess::WRITE}, oldSizep});
        initFirstp->ಕೂಡಿಸು_ಮುಂದಿನ(dynarrayNewp->makeStmt());
        initFirstp->ಕೂಡಿಸು_ಮುಂದಿನ(make_ಬಹುಲಕ_ಗಣ_Loop(
            fl, ಹೊಸ AstVarRef{fl, ಬಹುಲಕ_Var_Modp, ಬಹುಲಕ_Varp, VAccess::WRITE}, ಹೊಸ AstConst{fl, 1},
            ನಿಜ, ಹೊಸ AstVarRef{fl, old_ಗಾತ್ರ_Varp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (!VN_IS(stmtp, ಗಂಟು_Stmt)) ಮುಂದುವರಿ;  // Skip var declarations
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ sep = VN_CAST(stmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                ಆದರೆ (VN_IS(sep->exprp(), New)) ಮುಂದುವರಿ;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(initFirstp);
            ಹಿಂತಿರುಗಿ;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->ಕೂಡಿಸು_Stmtsp(initFirstp);
    }
    ಶೂನ್ಯ make_Static_ಬಹುಲಕ_Init(AstVar* ಬಹುಲಕ_Varp, AstClass* classp, uint32_t ಬಹುಲಕ_ಎಣಿಕೆ) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ಬಹುಲಕ_Var_Modp = VN_AS(ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ಬಹುಲಕ_Varp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ಸ್ಥಿರಾಂಕ sizep
            = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, ಬಹುಲಕ_Var_Modp, ಬಹುಲಕ_Varp, VAccess::READ},
                                 VCMethod::DYN_ಗಾತ್ರ, ಶೂನ್ಯಸೂಚಕ};
        sizep->dtype_ಗಣ_UInt32();
        AstEq* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstEq{fl, sizep, ಹೊಸ AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ಸ್ಥಿರಾಂಕ dynarrayNewp
            = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, ಬಹುಲಕ_Var_Modp, ಬಹುಲಕ_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, ಹೊಸ AstConst{fl, ಬಹುಲಕ_ಎಣಿಕೆ}};
        dynarrayNewp->dtype_ಗಣ_Void();
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(
            make_ಬಹುಲಕ_ಗಣ_Loop(fl, ಹೊಸ AstVarRef{fl, ಬಹುಲಕ_Var_Modp, ಬಹುಲಕ_Varp, VAccess::WRITE},
                            ಹೊಸ AstConst{fl, 1}, ನಿಜ));

        // Build the if statement
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ newp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "new"), ಗಂಟು_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->ಕೂಡಿಸು_Stmtsp(ifp);
    }
    Ast_ಗಂಟು* make_ಬಹುಲಕ_ಗಣ_Loop(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp,
                             ತಾರ್ಕಿಕ ಒಳ_Task, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ startp = ಶೂನ್ಯಸೂಚಕ) {
        AstVar* ಸ್ಥಿರಾಂಕ iterVarp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->ಕಂಡುಹಿಡಿ_UInt32_DType()};
        iterVarp->funcLocal(ಒಳ_Task);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ಸ್ಥಿರಾಂಕ sizep = ಹೊಸ AstCMethodHard{fl, lhsp, VCMethod::DYN_ಗಾತ್ರ, ಶೂನ್ಯಸೂಚಕ};
        sizep->dtype_ಗಣ_UInt32();
        AstCMethodHard* ಸ್ಥಿರಾಂಕ setp
            = ಹೊಸ AstCMethodHard{fl, lhsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), VCMethod::ARRAY_AT_WRITE,
                                 ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtype_ಗಣ_UInt32();
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = iterVarp;
        stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : ಹೊಸ AstConst{fl, 0}});

        AstLoop* ಸ್ಥಿರಾಂಕ loopp = ಹೊಸ AstLoop{fl};
        stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(loopp);
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstLoopTest{
            fl, loopp, ಹೊಸ AstLt{fl, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, setp, rhsp});
        loopp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            fl, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::WRITE},
            ಹೊಸ Ast_ಕೂಡಿಸು{fl, ಹೊಸ AstConst{fl, 1}, ಹೊಸ AstVarRef{fl, iterVarp, VAccess::READ}}});
        ಹಿಂತಿರುಗಿ ಹೊಸ AstBegin{fl, m_unique_ಹೆಸರುಗಳು.get("__VrandBlock"), stmtsp, ನಿಜ};
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_ತಾತ್ಕಾಲಿಕ_Varsp(AstVar* varsp) {
        ಗಾಗಿ (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            ಆದರೆ (m_ftaskp) {
                varp->funcLocal(ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        ಆದರೆ (!m_ftaskp) {
            m_modp->ಕೂಡಿಸು_Stmtsp(varsp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_ಗಂಟು_Stmt* wrap_If_Rand_ಬಹುಲಕ(AstClass* classp, AstVar* ಸ್ಥಿರಾಂಕ varp, Ast_ಗಂಟು_Stmt* stmtp) {
        ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = varp->user1()};
        AstVar* ಸ್ಥಿರಾಂಕ ಬಹುಲಕ_Varp = varp->lifetime().isStatic() ? get_Static_Rand_ಬಹುಲಕ_Var(classp)
                                                             : get_Rand_ಬಹುಲಕ_Var_From_Class(classp);
        ಹಿಂತಿರುಗಿ VN_AS(wrap_If_ಬಹುಲಕ(rmode, ಬಹುಲಕ_Varp, stmtp), ಗಂಟು_Stmt);
    }
    Ast_ಗಂಟು* wrap_If_Constraint_ಬಹುಲಕ(AstClass* classp, AstConstraint* ಸ್ಥಿರಾಂಕ constrp, Ast_ಗಂಟು* stmtp) {
        ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = constrp->user1()};
        AstVar* ಸ್ಥಿರಾಂಕ ಬಹುಲಕ_Varp = constrp->isStatic() ? get_Static_Constraint_ಬಹುಲಕ_Var(classp)
                                                     : get_Constraint_ಬಹುಲಕ_Var(classp);
        ಹಿಂತಿರುಗಿ wrap_If_ಬಹುಲಕ(rmode, ಬಹುಲಕ_Varp, stmtp);
    }
    ಸ್ಥಿರ Ast_ಗಂಟು* wrap_If_ಬಹುಲಕ(ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ ಬಹುಲಕ, AstVar* ಬಹುಲಕ_Varp, Ast_ಗಂಟು* stmtp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = stmtp->fileline();
        ಆದರೆ (ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_AS(ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module);
            AstCMethodHard* ಸ್ಥಿರಾಂಕ atp
                = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, modp, ಬಹುಲಕ_Varp, VAccess::READ},
                                     VCMethod::ARRAY_AT, ಹೊಸ AstConst{fl, ಬಹುಲಕ.ಸೂಚ್ಯಂಕ}};
            atp->dtype_ಗಣ_UInt32();
            ಹಿಂತಿರುಗಿ ಹೊಸ AstIf{fl, atp, stmtp};
        }
        ಹಿಂತಿರುಗಿ stmtp;
    }
    AstVarRef* enum_ಮೌಲ್ಯ_Tab_Refp(AstEnumDType* ಸ್ಥಿರಾಂಕ nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        ಆದರೆ (nodep->user2p()) ಹಿಂತಿರುಗಿ VN_AS(nodep->user2p(), VarRef)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ vardtypep = ಹೊಸ AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            ಹೊಸ AstRange{nodep->fileline(), static_cast<ಪೂರ್ಣಾಂಕ>(nodep->ಅಂಶ_ಎಣಿಕೆ()) - 1, 0}};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(vardtypep);
        AstConsArray* ಸ್ಥಿರಾಂಕ consp = ಹೊಸ AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t ಸೂಚ್ಯಂಕ = 0;
        ಗಾಗಿ (Ast_Enum_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
            AstConst* ಸ್ಥಿರಾಂಕ vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(ಸೂಚ್ಯಂಕ++, vconstp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        }
        // Share identical tables via the constant pool
        AstVarRef* ಸ್ಥಿರಾಂಕ refp = V3ConstPool::ಕಂಡುಹಿಡಿ(consp);
        VL_DO_DANGLING(consp->delete_ವೃಕ್ಷ(), consp);  // V3ConstPool::find clones it
        ತಳ್ಳು_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        ಹಿಂತಿರುಗಿ refp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
    }

    AstCDType* ಕಂಡುಹಿಡಿ_Vl_Rand_CDType(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, uint64_t ವಸ್ತುಗಳು) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ಸ್ಥಿರಾಂಕ std::string type = AstCDType::type_ಗಮ್ಯ_Hold(V3_ಸಂಖ್ಯೆ::log2bQuad(ವಸ್ತುಗಳು) + 1);
        ಸ್ಥಿರಾಂಕ std::string name = "VlRandC<" + type + ", " + cvt_ಗಮ್ಯ_Str(ವಸ್ತುಗಳು) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_randcDtypes.emplace(name, ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (pair.ಎರಡನೇ) {
            AstCDType* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(newp);
            pair.ಮೊದಲ->ಎರಡನೇ = newp;
        }
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    AstVar* newRandcVarsp(AstVar* ಸ್ಥಿರಾಂಕ varp, AstClass* ಸ್ಥಿರಾಂಕ classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        ಆದರೆ (!varp->isRandC()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        uint64_t ವಸ್ತುಗಳು = 0;
        ಆದರೆ (AstEnumDType* ಸ್ಥಿರಾಂಕ enumDtp = VN_CAST(varp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType)) {
            ವಸ್ತುಗಳು = static_cast<uint64_t>(enumDtp->ಅಂಶ_ಎಣಿಕೆ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = varp->dtypep()->skipRefp()->basicp()) {
            ಆದರೆ (basicp->ಅಗಲ() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                ಆದರೆ (!V3Error::error_ಎಣಿಕೆ()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->ಅಗಲ() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
            ವಸ್ತುಗಳು = 1ULL << basicp->ಅಗಲ();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstStructDType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_ಎಣಿಕೆ(), varp, "Unexpected randc variable dtype");
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        AstCDType* ಸ್ಥಿರಾಂಕ newdtp = ಕಂಡುಹಿಡಿ_Vl_Rand_CDType(varp->fileline(), ವಸ್ತುಗಳು);
        AstVar* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->is_ಆಂತರಿಕ(ನಿಜ);
        classp->ಕೂಡಿಸು_Stmtsp(newp);
        UINFO(9, "created " << varp);
        ಹಿಂತಿರುಗಿ newp;
    }
    Ast_ಗಂಟು_Stmt* createArrayForeachLoop(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep,
                                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar* ಸ್ಥಿರಾಂಕ outputVarp) {
        Ast_ಗಂಟು_DType* ತಾತ್ಕಾಲಿಕ_DTypep = dtypep;
        AstVar* randLoopIndxp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ವಯಂಚಾಲಿತ create_Loop_ಸೂಚ್ಯಂಕ = [&](Ast_ಗಂಟು_DType* ತಾತ್ಕಾಲಿಕ_DTypep) {
            ಆದರೆ (VN_IS(ತಾತ್ಕಾಲಿಕ_DTypep, AssocArrayDType)) {
                ಹಿಂತಿರುಗಿ ಹೊಸ AstVar{fl, VVarType::VAR, m_rand_Array_Unique_ಹೆಸರುಗಳು.get(""),
                                  VN_AS(ತಾತ್ಕಾಲಿಕ_DTypep, AssocArrayDType)->ಕೀಲಿ_DTypep()};
            }
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVar{fl, VVarType::VAR, m_rand_Array_Unique_ಹೆಸರುಗಳು.get(""),
                              dtypep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32)};
        };
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತಾತ್ಕಾಲಿಕ_Elementp = ಶೂನ್ಯಸೂಚಕ;
        ಆಗ (ತಾತ್ಕಾಲಿಕ_DTypep->isNonPackedArray()) {
            AstVar* ಸ್ಥಿರಾಂಕ newRandLoopIndxp = create_Loop_ಸೂಚ್ಯಂಕ(ತಾತ್ಕಾಲಿಕ_DTypep);
            randLoopIndxp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(randLoopIndxp, newRandLoopIndxp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_Exprp = ತಾತ್ಕಾಲಿಕ_Elementp ? ತಾತ್ಕಾಲಿಕ_Elementp : exprp;
            AstVarRef* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_Refp = ಹೊಸ AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            ಆದರೆ (VN_IS(ತಾತ್ಕಾಲಿಕ_DTypep, DynArrayDType)) {
                ತಾತ್ಕಾಲಿಕ_Elementp
                    = ಹೊಸ AstCMethodHard{fl, ತಾತ್ಕಾಲಿಕ_Exprp, VCMethod::ARRAY_AT_WRITE, ತಾತ್ಕಾಲಿಕ_Refp};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ತಾತ್ಕಾಲಿಕ_DTypep, UnpackArrayDType)) {
                Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ aryDTypep = VN_CAST(ತಾತ್ಕಾಲಿಕ_DTypep, ಗಂಟು_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                ತಾತ್ಕಾಲಿಕ_Elementp = ಹೊಸ AstArraySel{
                    fl, ತಾತ್ಕಾಲಿಕ_Exprp,
                    ಹೊಸ AstSel{
                        fl,
                        ಹೊಸ AstSub{fl, ತಾತ್ಕಾಲಿಕ_Refp,
                                   ಹೊಸ AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        ಹೊಸ AstConst{fl, 0}, V3_ಸಂಖ್ಯೆ::log2b(aryDTypep->hi()) + 1}};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ತಾತ್ಕಾಲಿಕ_DTypep, AssocArrayDType)) {
                ತಾತ್ಕಾಲಿಕ_Elementp = ಹೊಸ AstAssocSel{fl, ತಾತ್ಕಾಲಿಕ_Exprp, ತಾತ್ಕಾಲಿಕ_Refp};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ತಾತ್ಕಾಲಿಕ_DTypep, ಸರತಿ_DType)) {
                ತಾತ್ಕಾಲಿಕ_Elementp
                    = ಹೊಸ AstCMethodHard{fl, ತಾತ್ಕಾಲಿಕ_Exprp, VCMethod::DYN_AT_WRITE_APPEND, ತಾತ್ಕಾಲಿಕ_Refp};
            }
            ತಾತ್ಕಾಲಿಕ_Elementp->dtypep(ತಾತ್ಕಾಲಿಕ_DTypep->subDTypep());
            ತಾತ್ಕಾಲಿಕ_DTypep = ತಾತ್ಕಾಲಿಕ_DTypep->virtRefDTypep();
        }

        Ast_Foreach_ಶೀರ್ಷಿಕೆ* ಸ್ಥಿರಾಂಕ headerp
            = ಹೊಸ Ast_Foreach_ಶೀರ್ಷಿಕೆ{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), randLoopIndxp};
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ randStmtsp = newRandStmtsp(fl, ತಾತ್ಕಾಲಿಕ_Elementp, ಶೂನ್ಯಸೂಚಕ, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        ಆದರೆ (!ತಾತ್ಕಾಲಿಕ_Elementp->backp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(ತಾತ್ಕಾಲಿಕ_Elementp), ತಾತ್ಕಾಲಿಕ_Elementp);
        ಹಿಂತಿರುಗಿ ಹೊಸ AstForeach{fl, headerp, randStmtsp};
    }
    Ast_ಗಂಟು_Stmt* newRandStmtsp(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar* randcVarp,
                               AstVar* ಸ್ಥಿರಾಂಕ outputVarp, ಪೂರ್ಣಾಂಕ ಸ್ಥಾನಾಂತರ = 0,
                               AstMemberDType* memberp = ಶೂನ್ಯಸೂಚಕ) {
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_ಗಂಟು_Stmt* stmtsp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (structDtp->packed()) ಸ್ಥಾನಾಂತರ += memberp ? memberp->lsb() : 0;
            ಗಾಗಿ (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_ಗಂಟು_Stmt* randp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು) : exprp, ಶೂನ್ಯಸೂಚಕ,
                                          outputVarp, ಸ್ಥಾನಾಂತರ, smemberp);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = ಹೊಸ AstStructSel{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), smemberp->name()};
                    structSelp->dtypep(smemberp->ಮಗು_DTypep());
                    ಆದರೆ (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, ಶೂನ್ಯಸೂಚಕ, outputVarp);
                }
                stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(stmtsp, randp);
            }
            ಹಿಂತಿರುಗಿ stmtsp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            ಆದರೆ (!unionDtp->packed()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            AstMemberDType* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Memberp = unionDtp->membersp();
            ಹಿಂತಿರುಗಿ newRandStmtsp(fl, exprp, ಶೂನ್ಯಸೂಚಕ, outputVarp, ಸ್ಥಾನಾಂತರ, ಮೊದಲ_Memberp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ಸ್ಥಿರಾಂಕ memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ Ast_ವಿಧಾನ_Call{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                fl, ಹೊಸ AstVarRef{fl, outputVarp, VAccess::WRITE},
                ಹೊಸ AstAnd{fl, ಹೊಸ AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            ಹಿಂತಿರುಗಿ ಹೊಸ AstIf{
                fl, ಹೊಸ AstNeq{fl, exprp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, AstConst::Null{}}},
                assignp};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstDynArrayDType* ಸ್ಥಿರಾಂಕ dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            ಹಿಂತಿರುಗಿ createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಸರತಿ_DType* ಸ್ಥಿರಾಂಕ ಸರತಿ_Dtp = VN_CAST(memberDtp, ಸರತಿ_DType)) {
            ಹಿಂತಿರುಗಿ createArrayForeachLoop(fl, ಸರತಿ_Dtp, exprp, outputVarp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            ಹಿಂತಿರುಗಿ createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstAssocArrayDType* ಸ್ಥಿರಾಂಕ assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            ಹಿಂತಿರುಗಿ createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valp;
            ಆದರೆ (AstEnumDType* ಸ್ಥಿರಾಂಕ enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ಸ್ಥಿರಾಂಕ tabRefp = enum_ಮೌಲ್ಯ_Tab_Refp(enumDtp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ randp
                    = new_Rand_ಮೌಲ್ಯ(fl, randcVarp, exprp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32));
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ moddivp = ಹೊಸ AstModDiv{
                    fl, randp, ಹೊಸ AstConst{fl, static_cast<uint32_t>(enumDtp->ಅಂಶ_ಎಣಿಕೆ())}};
                moddivp->dtypep(enumDtp);
                valp = ಹೊಸ AstArraySel{fl, tabRefp, moddivp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                valp
                    = new_Rand_ಮೌಲ್ಯ(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            Ast_ನಿಯೋಜಿಸು* assignp
                = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl,
                                ಹೊಸ AstSel{fl, exprp, ಸ್ಥಾನಾಂತರ + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->ಅಗಲ() : exprp->ಅಗಲ()},
                                valp};
            AstVar* varp = ಶೂನ್ಯಸೂಚಕ;
            exprp->exists([&](ಸ್ಥಿರಾಂಕ AstVarRef* varrefp) {
                ಆದರೆ (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                ಹಿಂತಿರುಗಿ varp != ಶೂನ್ಯಸೂಚಕ;
            });
            ಹಿಂತಿರುಗಿ wrap_If_Rand_ಬಹುಲಕ(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Rand_ಮೌಲ್ಯ(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstVar* ಸ್ಥಿರಾಂಕ randcVarp,
                              Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        ಆದರೆ (randcVarp) {
            AstCExpr* ಸ್ಥಿರಾಂಕ cexprp = ಹೊಸ AstCExpr{fl};
            cexprp->ಕೂಡಿಸು(ಹೊಸ AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->ಕೂಡಿಸು(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            ಹಿಂತಿರುಗಿ cexprp;
        }

        ಹಿಂತಿರುಗಿ ಹೊಸ AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* ಕಂಡುಹಿಡಿ_Pre_Post_Task(AstClass* classp, ಸ್ಥಿರಾಂಕ string& name) {
        ಗಾಗಿ (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ) {
            ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ taskp = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(cp, name), Task)) {
                ಹಿಂತಿರುಗಿ taskp;
            }
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Pre_Post_Call(AstClass* ಸ್ಥಿರಾಂಕ classp, Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ funcp, ಸ್ಥಿರಾಂಕ string& name) {
        ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ userFuncp = ಕಂಡುಹಿಡಿ_Pre_Post_Task(classp, name)) {
            AstTaskRef* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* ಸ್ಥಿರಾಂಕ classp, ಸ್ಥಿರಾಂಕ string& which) {
        ಸ್ಥಿರಾಂಕ string name = "__V" + which;
        ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ existingp = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, name), Task)) {
            ಹಿಂತಿರುಗಿ existingp;
        }
        AstTask* ಸ್ಥಿರಾಂಕ taskp = ಹೊಸ AstTask{classp->fileline(), name, ಶೂನ್ಯಸೂಚಕ};
        taskp->class_ವಿಧಾನ(ನಿಜ);
        taskp->isVirtual(classp->isExtended());
        classp->ಕೂಡಿಸು_Membersp(taskp);
        m_memberMap.ಸೇರಿಸು(classp, taskp);
        ಕೂಡಿಸು_Pre_Post_Call(classp, taskp, which);
        ಹಿಂತಿರುಗಿ taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ> buildPrePostVirtualWrappers(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ cachedIt = m_prePostWrap.ಕಂಡುಹಿಡಿ(classp);
        ಆದರೆ (cachedIt != m_prePostWrap.ಕೊನೆ()) ಹಿಂತಿರುಗಿ cachedIt->ಎರಡನೇ;
        std::ಸದಿಶ<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            ಆದರೆ (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.ತಳ್ಳು_ಹಿಂಭಾಗ(subp);
        });
        ತಾರ್ಕಿಕ hasPre = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ hasPost = ಸುಳ್ಳು;
        ಗಾಗಿ (AstClass* ಸ್ಥಿರಾಂಕ cp : hierp) {
            ಆದರೆ (ಕಂಡುಹಿಡಿ_Pre_Post_Task(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = ನಿಜ;
            }
            ಆದರೆ (ಕಂಡುಹಿಡಿ_Pre_Post_Task(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = ನಿಜ;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        ಆದರೆ (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        ಆದರೆ (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ಸ್ಥಿರಾಂಕ std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ> ಫಲಿತಾಂಶ{hasPre, hasPost};
        m_prePostWrap.emplace(classp, ಫಲಿತಾಂಶ);
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Virtual_Pre_Post_Call(AstFunc* ಸ್ಥಿರಾಂಕ randomizeFuncp, AstClass* ಸ್ಥಿರಾಂಕ classp,
                               ಸ್ಥಿರಾಂಕ string& which) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = classp->fileline();
        AstTask* ಸ್ಥಿರಾಂಕ wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ಸ್ಥಿರಾಂಕ refDTypep = ಹೊಸ AstClassRefDType{fl, classp, ಶೂನ್ಯಸೂಚಕ};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(refDTypep);
        Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp
            = ಹೊಸ Ast_ವಿಧಾನ_Call{fl, ಹೊಸ AstThisRef{fl, refDTypep}, wrapperp->name(), ಶೂನ್ಯಸೂಚಕ};
        callp->taskp(wrapperp);
        callp->dtype_ಗಣ_Void();
        randomizeFuncp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    ತಾರ್ಕಿಕ classHasRandClassMembers(AstClass* classp) {
        ಹಿಂತಿರುಗಿ classp->existsMember([](ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ AstVar* varp) {
            ಆದರೆ (!varp->rand().isRandomizable()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep()->skipRefp();
            ಹಿಂತಿರುಗಿ VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    ತಾರ್ಕಿಕ classOwnsGlobalConstraint(ಸ್ಥಿರಾಂಕ AstClass* classp) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ classp->existsMember([](ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ AstConstraint* constrp) {
            ತಾರ್ಕಿಕ owns = ಸುಳ್ಳು;
            constrp->foreach([&](ಸ್ಥಿರಾಂಕ AstMemberSel* memberSelp) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rootp
                    = const_cast<AstMemberSel*>(memberSelp)->ತಳ_Fromp(ನಿಜ);
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    ಆಗ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    ಆದರೆ (VN_IS(dtypep, ClassRefDType)) owns = ನಿಜ;
                }
            });
            ಹಿಂತಿರುಗಿ owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ಸ್ಥಿರಾಂಕ string& suffix) {
        ಸ್ಥಿರಾಂಕ string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, name), Task);
        ಆದರೆ (taskp) ಹಿಂತಿರುಗಿ taskp;
        taskp = ಹೊಸ AstTask{classp->fileline(), name, ಶೂನ್ಯಸೂಚಕ};
        taskp->class_ವಿಧಾನ(ನಿಜ);
        classp->ಕೂಡಿಸು_Membersp(taskp);
        m_memberMap.ಸೇರಿಸು(classp, taskp);
        ಹಿಂತಿರುಗಿ taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    ಶೂನ್ಯ populateNestedCallbackTask(AstTask* ಸ್ಥಿರಾಂಕ callbackTaskp, AstClass* ಸ್ಥಿರಾಂಕ classp,
                                    ಸ್ಥಿರಾಂಕ string& cbName) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            ಆದರೆ (!memberVarp->rand().isRandomizable()) ಹಿಂತಿರುಗಿ;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = memberVarp->dtypep()->skipRefp();
            ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(dtypep, ClassRefDType);
            ಆದರೆ (!classRefp) ಹಿಂತಿರುಗಿ;
            AstClass* ಸ್ಥಿರಾಂಕ memberClassp = classRefp->classp();
            ಆದರೆ (memberClassp == classp) ಹಿಂತಿರುಗಿ;  // Avoid self-reference

            // Force-visit member class if not yet processed
            ಆದರೆ (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_ಗಂಟು* stmtsp = ಶೂನ್ಯಸೂಚಕ;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ userFuncp = ಕಂಡುಹಿಡಿ_Pre_Post_Task(memberClassp, cbName)) {
                Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ Ast_ವಿಧಾನ_Call{
                    fl, ಹೊಸ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtype_ಗಣ_Void();
                stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            ಆದರೆ (classHasRandClassMembers(memberClassp)) {
                ಸ್ಥಿರಾಂಕ string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ಸ್ಥಿರಾಂಕ nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ recurseCallp = ಹೊಸ Ast_ವಿಧಾನ_Call{
                    fl, ಹೊಸ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtype_ಗಣ_Void();
                stmtsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(stmtsp, recurseCallp->makeStmt());
            }

            ಆದರೆ (!stmtsp) ಹಿಂತಿರುಗಿ;

            // Wrap in null check
            AstIf* ಸ್ಥಿರಾಂಕ nullCheckp = ಹೊಸ AstIf{
                fl,
                ಹೊಸ AstNeq{fl, ಹೊಸ AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           ಹೊಸ AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->ಕೂಡಿಸು_Stmtsp(wrap_If_Rand_ಬಹುಲಕ(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* new_ಸಿದ್ಧತೆ_Constraint_Task(AstClass* ಸ್ಥಿರಾಂಕ nodep, ಸ್ಥಿರಾಂಕ std::string& name) {
        AstTask* ಸ್ಥಿರಾಂಕ taskp = ಹೊಸ AstTask{nodep->fileline(), name + "_setup_constraint", ಶೂನ್ಯಸೂಚಕ};
        taskp->class_ವಿಧಾನ(ನಿಜ);
        nodep->ಕೂಡಿಸು_Membersp(taskp);
        ಹಿಂತಿರುಗಿ taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ಸ್ಥಿರಾಂಕ nodep, ಸ್ಥಿರಾಂಕ std::string& name) {
        AstTask* ಸ್ಥಿರಾಂಕ taskp
            = ಹೊಸ AstTask{nodep->fileline(), name + "_resize_constrained_array", ಶೂನ್ಯಸೂಚಕ};
        taskp->class_ವಿಧಾನ(ನಿಜ);
        nodep->ಕೂಡಿಸು_Membersp(taskp);
        ಹಿಂತಿರುಗಿ taskp;
    }
    Ast_ಗಂಟು_Stmt* implementConstraintsClear(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fileline, AstVar* ಸ್ಥಿರಾಂಕ genp) {
        AstCMethodHard* ಸ್ಥಿರಾಂಕ clearp = ಹೊಸ AstCMethodHard{
            fileline,
            ಹೊಸ AstVarRef{fileline, VN_AS(genp->user2p(), ಗಂಟು_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtype_ಗಣ_Void();
        ಹಿಂತಿರುಗಿ clearp->makeStmt();
    }
    Ast_ಗಂಟು_Stmt* implementConstraintsClearAll(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fileline, AstVar* ಸ್ಥಿರಾಂಕ genp) {
        AstCMethodHard* ಸ್ಥಿರಾಂಕ clearp = ಹೊಸ AstCMethodHard{
            fileline,
            ಹೊಸ AstVarRef{fileline, VN_AS(genp->user2p(), ಗಂಟು_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtype_ಗಣ_Void();
        ಹಿಂತಿರುಗಿ clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp) {
        ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(exprp, MemberSel)) {
            ಹಿಂತಿರುಗಿ memberSelp->varp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(exprp, VarRef)) {
            ಹಿಂತಿರುಗಿ varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* make_ಬಹುಲಕ_Var_Ref(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp, AstVar* ಸ್ಥಿರಾಂಕ ಬಹುಲಕ_Varp,
                                ಸ್ಥಿರಾಂಕ VAccess access) {
        ಆದರೆ (ಬಹುಲಕ_Varp->lifetime().isStatic()) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{exprp->fileline(), VN_AS(ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module),
                                 ಬಹುಲಕ_Varp, access};
        }
        ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ಸ್ಥಿರಾಂಕ newMemberSelp = ಹೊಸ AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಬಹುಲಕ_Varp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            ಹಿಂತಿರುಗಿ newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{exprp->fileline(), VN_AS(ಬಹುಲಕ_Varp->user2p(), Class), ಬಹುಲಕ_Varp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* create_Or_Get_ಗಾತ್ರ_Var(AstClass* ಸ್ಥಿರಾಂಕ classp, AstVar* ಸ್ಥಿರಾಂಕ arrVarp, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl,
                               Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ signed32DTypep, ತಾರ್ಕಿಕ& wasCreated) {
        AstVar* ಗಾತ್ರ_Varp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = ಸುಳ್ಳು;
        ಆದರೆ (!ಗಾತ್ರ_Varp) {
            ಗಾತ್ರ_Varp = ಹೊಸ AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->ಕೂಡಿಸು_Membersp(ಗಾತ್ರ_Varp);
            m_memberMap.ಸೇರಿಸು(classp, ಗಾತ್ರ_Varp);
            ಗಾತ್ರ_Varp->user2p(classp);
            arrVarp->user4p(ಗಾತ್ರ_Varp);
            wasCreated = ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಗಾತ್ರ_Varp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* create_ಗಾತ್ರ_Gte_Zero_Constraint(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstVar* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Varp,
                                                   Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* derefFromp = ಶೂನ್ಯಸೂಚಕ) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಗಾತ್ರ_ಗಂಟು_Exprp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (derefFromp) {
            ಗಾತ್ರ_ಗಂಟು_Exprp = buildMemberSelExprp(derefFromp, ಗಾತ್ರ_Varp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾತ್ರ_ಗಂಟು_Exprp = ಹೊಸ AstVarRef{fl, ಗಾತ್ರ_Varp, VAccess::READ};
        }
        ಗಾತ್ರ_ಗಂಟು_Exprp->user1(ನಿಜ);
        AstGteS* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Gtep = ಹೊಸ AstGteS{fl, ಗಾತ್ರ_ಗಂಟು_Exprp, ಹೊಸ AstConst{fl, 0}};
        ಗಾತ್ರ_Gtep->user1(ನಿಜ);
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, ಗಾತ್ರ_Gtep};
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* getFromp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp) {
        ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(exprp, MemberSel)) {
            ಹಿಂತಿರುಗಿ memberSelp->fromp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(exprp, ವಿಧಾನ_Call)) {
            ಹಿಂತಿರುಗಿ ವಿಧಾನ_Callp->fromp();
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    AstVar* make_Tmp_Rand_ಬಹುಲಕ_Var(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* siblingExprp, AstVar* rand_ಬಹುಲಕ_Varp,
                               Ast_ಗಂಟು*& storeStmtspr, Ast_ಗಂಟು_Stmt*& restoreStmtspr) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = rand_ಬಹುಲಕ_Varp->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Tmp_Varp = ಹೊಸ AstVar{
            fl, VVarType::BLOCKTEMP, m_ಬಹುಲಕ_Unique_ಹೆಸರುಗಳು.get(rand_ಬಹುಲಕ_Varp), rand_ಬಹುಲಕ_Varp->dtypep()};
        rand_ಬಹುಲಕ_Tmp_Varp->funcLocal(m_ftaskp);
        rand_ಬಹುಲಕ_Tmp_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
            storeStmtspr,
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, rand_ಬಹುಲಕ_Tmp_Varp, VAccess::WRITE},
                          make_ಬಹುಲಕ_Var_Ref(siblingExprp, rand_ಬಹುಲಕ_Varp, VAccess::READ)});
        storeStmtspr = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
            storeStmtspr,
            make_ಬಹುಲಕ_ಗಣ_Loop(fl, make_ಬಹುಲಕ_Var_Ref(siblingExprp, rand_ಬಹುಲಕ_Varp, VAccess::WRITE),
                            ಹೊಸ AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
            restoreStmtspr,
            ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, make_ಬಹುಲಕ_Var_Ref(siblingExprp, rand_ಬಹುಲಕ_Varp, VAccess::WRITE),
                          ಹೊಸ AstVarRef{fl, rand_ಬಹುಲಕ_Tmp_Varp, VAccess::READ}});
        ಹಿಂತಿರುಗಿ rand_ಬಹುಲಕ_Tmp_Varp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತುಂಡು_ಗಮ್ಯ_Common_ಪೂರ್ವಪ್ರತ್ಯಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* thisp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* otherp) {
        ಸ್ಥಿರ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        ಆಗ (thisp) {
            thisHier.ತಳ್ಳು_ಹಿಂಭಾಗ(thisp);
            thisp = getFromp(thisp);
        }
        ಆಗ (otherp) {
            otherHier.ತಳ್ಳು_ಹಿಂಭಾಗ(otherp);
            otherp = getFromp(otherp);
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* commonp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            ಆದರೆ ((*thisIt)->type() != (*otherIt)->type()) ನಿಲ್ಲಿಸು;
            ಆದರೆ (AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ಸ್ಥಿರಾಂಕ otherMemberSelp = VN_AS(*otherIt, MemberSel);
                ಆದರೆ (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    ಮುಂದುವರಿ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ this_ವಿಧಾನ_Callp = VN_CAST(*thisIt, ವಿಧಾನ_Call)) {
                Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ other_ವಿಧಾನ_Callp = VN_AS(*otherIt, ವಿಧಾನ_Call);
                ಆದರೆ (this_ವಿಧಾನ_Callp->taskp() == other_ವಿಧಾನ_Callp->taskp()) {
                    commonp = this_ವಿಧಾನ_Callp;
                    ಮುಂದುವರಿ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Var_Refp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ಸ್ಥಿರಾಂಕ ಎರಡನೇ_Var_Refp = VN_AS(*otherIt, VarRef);
                ಆದರೆ (ಮೊದಲ_Var_Refp->varp() == ಎರಡನೇ_Var_Refp->varp()) {
                    commonp = ಮೊದಲ_Var_Refp;
                    ಮುಂದುವರಿ;
                }
            }
            ನಿಲ್ಲಿಸು;
        }
        ಹಿಂತಿರುಗಿ commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* buildMemberSelExprp(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstVar* ಅತ್ಯಂತಆಳ_Varp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* resExprp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), ಶೂನ್ಯಸೂಚಕ);
            AstVar* varp = memberSelp->varp();
            ಆದರೆ (ಅತ್ಯಂತಆಳ_Varp) varp = ಅತ್ಯಂತಆಳ_Varp;
            resExprp = ಹೊಸ AstMemberSel{exprp->fileline(), resExprp, varp};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            ಆದರೆ (ಅತ್ಯಂತಆಳ_Varp) varp = ಅತ್ಯಂತಆಳ_Varp;
            resExprp = ಹೊಸ AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), ಗಂಟು_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(ನಿಜ);
        ಹಿಂತಿರುಗಿ resExprp;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Update_Rand_Vars_Body(AstClass* ಸ್ಥಿರಾಂಕ nodep, AstFunc* ಸ್ಥಿರಾಂಕ randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ಸ್ಥಿರಾಂಕ updatep
            = ಹೊಸ AstFunc{nodep->fileline(), "__VupdateRandVars", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        updatep->class_ವಿಧಾನ(ನಿಜ);
        updatep->isVirtual(ನಿಜ);
        updatep->keepAlive(ನಿಜ);
        updatep->needsSyms(ಸುಳ್ಳು);
        nodep->ಕೂಡಿಸು_Membersp(updatep);
        nodep->hasRandVarsUpdate(ನಿಜ);

        ಗಾಗಿ (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ) {
            Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ newp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "new"), ಗಂಟು_FTask);
            ಗಾಗಿ (Ast_ಗಂಟು* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ stmtExprp = VN_CAST(stmtp, Stmt_ಅಭಿವ್ಯಕ್ತಿ);
                ಆದರೆ (!stmtExprp) ಮುಂದುವರಿ;
                AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                ಆದರೆ (!methodp || methodp->ವಿಧಾನ() != VCMethod::RANDOMIZER_WRITE_VAR) ಮುಂದುವರಿ;

                Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varRefp = VN_AS(methodp->pinsp(), ಗಂಟು_Var_Ref);
                ಆದರೆ (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    ಮುಂದುವರಿ;
                }
                AstCMethodHard* ಸ್ಥಿರಾಂಕ updateMethodp = ಹೊಸ AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                updateMethodp->dtype_ಗಣ_Void();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ namep = VN_AS(varRefp->nextp()->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                updateMethodp->ಕೂಡಿಸು_Pinsp(namep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು));
                updateMethodp->ವಿಧಾನ(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->ಕೂಡಿಸು_Stmtsp(updateMethodp->makeStmt());
            }
        }
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Basic_Randomize_Body(AstFunc* ಸ್ಥಿರಾಂಕ basicRandomizep, AstClass* ಸ್ಥಿರಾಂಕ nodep,
                               AstVar* rand_ಬಹುಲಕ_Varp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        AstVar* ಸ್ಥಿರಾಂಕ basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ಸ್ಥಿರಾಂಕ basicFvarRefp = ಹೊಸ AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ಸ್ಥಿರಾಂಕ beginBasicValp = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1};
        basicRandomizep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, basicFvarRefp, beginBasicValp});
        Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ newp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "new"), ಗಂಟು_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            ಆದರೆ (!memberVarp->rand().isRandomizable()) ಹಿಂತಿರುಗಿ;
            ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {.asUQuad = memberVarp->user1()};
            ಆದರೆ (rand_ಬಹುಲಕ.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* ಸ್ಥಿರಾಂಕ setp = ಹೊಸ AstCMethodHard{
                    nodep->fileline(),
                    ಹೊಸ AstVarRef{fl, VN_AS(rand_ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module), rand_ಬಹುಲಕ_Varp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, ಹೊಸ AstConst{nodep->fileline(), rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ}};
                setp->dtype_ಗಣ_UInt32();
                newp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, setp, ಹೊಸ AstConst{fl, 0}});
            }
            ಆದರೆ (memberVarp->user3()) ಹಿಂತಿರುಗಿ;  // Handled in constraints
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = memberVarp->dtypep()->skipRefp();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                ಆದರೆ (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    ಹಿಂತಿರುಗಿ;
                }
                AstFunc* ಸ್ಥಿರಾಂಕ memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp
                    = memberVarp->globalConstrained()
                          ? ಹೊಸ Ast_ವಿಧಾನ_Call{fl,
                                              ಹೊಸ AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : ಹೊಸ Ast_ವಿಧಾನ_Call{
                                fl, ಹೊಸ AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ಸ್ಥಿರಾಂಕ basicFvarRefReadp = basicFvarRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ಸ್ಥಿರಾಂಕ ನಿಯೋಜಿಸು_If_Not_Nullp = ಹೊಸ AstIf{
                    fl,
                    ಹೊಸ AstNeq{fl, ಹೊಸ AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               ಹೊಸ AstConst{fl, AstConst::Null{}}},
                    ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, basicFvarRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                  ಹೊಸ AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->ಕೂಡಿಸು_Stmtsp(wrap_If_Rand_ಬಹುಲಕ(nodep, memberVarp, ನಿಯೋಜಿಸು_If_Not_Nullp));
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstVar* ಸ್ಥಿರಾಂಕ randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ಸ್ಥಿರಾಂಕ refp
                    = ಹೊಸ AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                ಆದರೆ (!refp->backp()) VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
                basicRandomizep->ಕೂಡಿಸು_Stmtsp(
                    ಹೊಸ AstBegin{fl, m_unique_ಹೆಸರುಗಳು.get("__VrandBlock"), stmtp, ಸುಳ್ಳು});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* make_ಬಹುಲಕ_ನಿಯೋಜಿಸು_Lhs(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstClass* ಸ್ಥಿರಾಂಕ classp,
                                   Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp, AstVar* ಸ್ಥಿರಾಂಕ ಬಹುಲಕ_Varp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        ಆದರೆ (ಬಹುಲಕ_Varp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            ಆದರೆ (fromp) VL_DO_DANGLING(fromp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), fromp);
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, VN_AS(ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module), ಬಹುಲಕ_Varp,
                                 VAccess::WRITE};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            ಹಿಂತಿರುಗಿ ಹೊಸ AstVarRef{fl, VN_AS(ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module), ಬಹುಲಕ_Varp,
                                 VAccess::WRITE};
        } ಇಲ್ಲದಿದ್ದರೆ {
            AstMemberSel* ಸ್ಥಿರಾಂಕ memberselp = ಹೊಸ AstMemberSel{fl, fromp->unlink_Fr_ಹಿಂಭಾಗ(), ಬಹುಲಕ_Varp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            ಹಿಂತಿರುಗಿ memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    ಶೂನ್ಯ replace_With_ಬಹುಲಕ_ನಿಯೋಜಿಸು(Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ ftaskRefp, Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ receiverp,
                               Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp) {
        replace_With_ಬಹುಲಕ_ನಿಯೋಜಿಸು_And_Append(ftaskRefp, receiverp, lhsp, ಶೂನ್ಯಸೂಚಕ);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    ಶೂನ್ಯ replace_With_ಬಹುಲಕ_ನಿಯೋಜಿಸು_And_Append(Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ ftaskRefp, Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ receiverp,
                                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp, Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ appendStmtp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ftaskRefp->fileline();
        ಆದರೆ (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ), ftaskRefp, "Should be a statement");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ftaskRefp->argsp()->exprp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು* newStmtp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                //ಬಹುಲಕ
                ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ಸ್ಥಿರಾಂಕ setp = ಹೊಸ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                ಹೊಸ AstConst{fl, rmode.ಸೂಚ್ಯಂಕ}};
                setp->dtype_ಗಣ_UInt32();
                newStmtp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, setp, rhsp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = make_ಬಹುಲಕ_ಗಣ_Loop(fl, lhsp, rhsp, m_ftaskp);
            }
            ಆದರೆ (appendStmtp) newStmtp->ಕೂಡಿಸು_ಮುಂದಿನ(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(ತಳ್ಳು_Deletep(m_stmtp), m_stmtp = ಶೂನ್ಯಸೂಚಕ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ಸ್ಥಿರಾಂಕ setp = ಹೊಸ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            ಹೊಸ AstConst{fl, rmode.ಸೂಚ್ಯಂಕ}};
            setp->dtype_ಗಣ_UInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    ಶೂನ್ಯ wrapRandomizeCallWithNullGuard(Ast_ಗಂಟು_FTask_Ref* nodep) {
        Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp = VN_CAST(nodep, ವಿಧಾನ_Call);
        ಆದರೆ (!callp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (callp->user4_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = callp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ checkp
            = ಹೊಸ AstNeq{fl, callp->fromp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), ಹೊಸ AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
        relinker.relink(ಹೊಸ AstCond{fl, checkp, callp, ಹೊಸ AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    ಶೂನ್ಯ handle_Randomize_ವಾದಗಳು(Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (!nodep->argsp()) ಹಿಂತಿರುಗಿ;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        ತಾರ್ಕಿಕ hasNullArg = ಸುಳ್ಳು;
        ಗಾಗಿ (AstArg *argp = nodep->argsp(), *nextp = ಶೂನ್ಯಸೂಚಕ; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->ಅಂಕೆ().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = ನಿಜ;
                VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        ಗಾಗಿ (AstArg *argp = nodep->argsp(), *nextp = ಶೂನ್ಯಸೂಚಕ; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            ಗಾಗಿ (AstArg *otherArgp = nextp, *otherNextp = ಶೂನ್ಯಸೂಚಕ; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prefixp
                    = ತುಂಡು_ಗಮ್ಯ_Common_ಪೂರ್ವಪ್ರತ್ಯಯ(argp->exprp(), otherArgp->exprp())) {
                    ಆದರೆ (prefixp == argp->exprp()) {
                        ಆದರೆ (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), otherArgp);
                        ಮುಂದುವರಿ;
                    }
                }
                ಆದರೆ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ prefixp
                    = ತುಂಡು_ಗಮ್ಯ_Common_ಪೂರ್ವಪ್ರತ್ಯಯ(otherArgp->exprp(), argp->exprp())) {
                    ಆದರೆ (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::ಗಣ<AstVar*> saved_Rand_ಬಹುಲಕ_Varps;
        AstVar* tmpVarps = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* storeStmtsp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು* ಗಣ_Stmtsp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_Stmt* restoreStmtsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (AstArg *argp = nodep->argsp(), *nextp = ಶೂನ್ಯಸೂಚಕ; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = argp->exprp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ commonPrefixp = ತುಂಡು_ಗಮ್ಯ_Common_ಪೂರ್ವಪ್ರತ್ಯಯ(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = argp->fileline();
            ಆಗ (exprp) {
                ಆದರೆ (commonPrefixp == exprp) ನಿಲ್ಲಿಸು;
                AstVar* ಸ್ಥಿರಾಂಕ randVarp = getVarFromRef(exprp);
                AstClass* ಸ್ಥಿರಾಂಕ classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Varp = randVarp->lifetime().isStatic()
                                                 ? get_Static_Rand_ಬಹುಲಕ_Var(classp)
                                                 : get_Rand_ಬಹುಲಕ_Var_From_Class(classp);
                UASSERT_OBJ(rand_ಬಹುಲಕ_Varp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                ಆದರೆ (saved_Rand_ಬಹುಲಕ_Varps.ಕಂಡುಹಿಡಿ(rand_ಬಹುಲಕ_Varp) == saved_Rand_ಬಹುಲಕ_Varps.ಕೊನೆ()) {
                    AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Tmp_Varp
                        = make_Tmp_Rand_ಬಹುಲಕ_Var(exprp, rand_ಬಹುಲಕ_Varp, storeStmtsp, restoreStmtsp);
                    saved_Rand_ಬಹುಲಕ_Varps.ಸೇರಿಸು(rand_ಬಹುಲಕ_Varp);
                    tmpVarps = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(tmpVarps, rand_ಬಹುಲಕ_Tmp_Varp);
                }
                ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rand_ಬಹುಲಕ = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = ಹೊಸ AstCMethodHard{
                    fl, make_ಬಹುಲಕ_Var_Ref(exprp, rand_ಬಹುಲಕ_Varp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, ಹೊಸ AstConst{fl, rand_ಬಹುಲಕ.ಸೂಚ್ಯಂಕ}};
                setp->dtype_ಗಣ_UInt32();
                ಗಣ_Stmtsp
                    = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(ಗಣ_Stmtsp, ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, setp, ಹೊಸ AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), argp);
        }
        ಆದರೆ (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* ಗುರಿ_Classp = ಶೂನ್ಯಸೂಚಕ;
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(nodep, ವಿಧಾನ_Call);
            ಆದರೆ (ವಿಧಾನ_Callp) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDTypep = ವಿಧಾನ_Callp->fromp()->dtypep();
                ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                ಗುರಿ_Classp = crdtp->classp();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಗುರಿ_Classp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(ಗುರಿ_Classp, nodep, "randomize(null) target class unresolved");
            AstFunc* ಸ್ಥಿರಾಂಕ checkOnlyFuncp = getCreateRandomizeNullFunc(ಗುರಿ_Classp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        ಆದರೆ (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && ಗಣ_Stmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlink_Fr_ಹಿಂಭಾಗ(&relinker);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = storeStmtsp;
            stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(ಗಣ_Stmtsp);
            stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(m_stmtp);
            stmtsp->ಕೂಡಿಸು_ಮುಂದಿನ(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            ಕೂಡಿಸು_ತಾತ್ಕಾಲಿಕ_Varsp(tmpVarps);
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
    AstFunc* getCreateRandomizeNullFunc(AstClass* ಸ್ಥಿರಾಂಕ classp) {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ name = "__Vrandomize_null";
        ಆದರೆ (AstFunc* ಸ್ಥಿರಾಂಕ existingp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, name), Func)) {
            ಹಿಂತಿರುಗಿ existingp;
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = classp->fileline();
        AstFunc* ಸ್ಥಿರಾಂಕ funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ಸ್ಥಿರಾಂಕ fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        ಕೂಡಿಸು_Pre_Post_Call(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ಸ್ಥಿರಾಂಕ classGenp = getRandomGenerator(classp);
        ಆದರೆ (!classGenp) {
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, fvarp, VAccess::WRITE},
                                           ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}});
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ genModp = VN_AS(classGenp->user2p(), ಗಂಟು_Module);
            funcp->ಕೂಡಿಸು_Stmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ಸ್ಥಿರಾಂಕ ಸಿದ್ಧತೆ_All_Taskp = get_Create_Constraint_ಸಿದ್ಧತೆ_Func(classp, m_memberMap);
            funcp->ಕೂಡಿಸು_Stmtsp((ಹೊಸ AstTaskRef{fl, ಸಿದ್ಧತೆ_All_Taskp})->makeStmt());
            AstCExpr* ಸ್ಥಿರಾಂಕ solverCallp = ಹೊಸ AstCExpr{fl};
            solverCallp->dtype_ಗಣ_ಬಿಟ್();
            solverCallp->ಕೂಡಿಸು(ಹೊಸ AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->ಕೂಡಿಸು(".next_check_only(__Vm_rng)");
            funcp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(ನಿಜ);
        }

        // 3. post_randomize -- only if result is non-zero
        ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ userPostp = ಕಂಡುಹಿಡಿ_Pre_Post_Task(classp, "post_randomize")) {
            AstTaskRef* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstTaskRef{userPostp->fileline(), userPostp};
            funcp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ AstIf{fl, ಹೊಸ AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        ಹಿಂತಿರುಗಿ funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* lift_Log_If_Chain_ಗಮ್ಯ_Constraint_If(AstLogIf* logIfp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = logIfp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = logIfp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = logIfp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು* thenBodyp;
        ಆದರೆ (AstLogIf* ಸ್ಥಿರಾಂಕ innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = lift_Log_If_Chain_ಗಮ್ಯ_Constraint_If(innerLogIfp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(innerLogIfp), innerLogIfp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            thenBodyp = ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, rhsp};
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConstraintIf{fl, condp, thenBodyp, ಶೂನ್ಯಸೂಚಕ};
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ distBoundRefsRandVar(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* boundp) {
        ಹಿಂತಿರುಗಿ boundp->exists(
            [](ಸ್ಥಿರಾಂಕ AstVarRef* vrefp) { ಹಿಂತಿರುಗಿ vrefp->varp()->rand().isRandomizable(); });
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ dist_Bound_Refs_ಬಹುಲಕ_Var(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* boundp) {
        ಹಿಂತಿರುಗಿ boundp->exists([](ಸ್ಥಿರಾಂಕ AstVarRef* vrefp) {
            ಆದರೆ (!vrefp->varp()->rand().isRandomizable()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = vrefp->varp()->user1()};
            ಹಿಂತಿರುಗಿ rmode.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newDistRangeMembership(AstDist* distp, ಸ್ಥಿರಾಂಕ AstInsideRange* irp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isSigned = distp->exprp()->isSigned();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dist_ಅಭಿವ್ಯಕ್ತಿ_Gtep = distp->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dist_ಅಭಿವ್ಯಕ್ತಿ_Ltep = distp->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        dist_ಅಭಿವ್ಯಕ್ತಿ_Gtep->user1(ನಿಜ);
        dist_ಅಭಿವ್ಯಕ್ತಿ_Ltep->user1(ನಿಜ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ gep
            = isSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                             ಹೊಸ AstGteS{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Gtep, irp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)})
                       : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                             ಹೊಸ AstGte{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Gtep, irp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)});
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lep
            = isSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                             ಹೊಸ AstLteS{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Ltep, irp->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)})
                       : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                             ಹೊಸ AstLte{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Ltep, irp->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)});
        gep->user1(ನಿಜ);
        lep->user1(ನಿಜ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ andp = ಹೊಸ AstLogAnd{fl, gep, lep};
        andp->user1(ನಿಜ);
        ಹಿಂತಿರುಗಿ andp;
    }

    ರಚನೆ Dist_ಪಾತ್ರೆ final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangep;  // A single value or an InsideRange
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತೂಕ_Exprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::ಸದಿಶ<Dist_ಪಾತ್ರೆ> collectDistBuckets(AstDist* ಸ್ಥಿರಾಂಕ distp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();
        std::ಸದಿಶ<Dist_ಪಾತ್ರೆ> buckets;
        ಗಾಗಿ (Ast_Dist_ಅಂಶ* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), Dist_ಅಂಶ)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(ditemp->weightp(), Const)) {
                ಆದರೆ (constp->ಗಮ್ಯ_UQuad() == 0) ಮುಂದುವರಿ;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ತೂಕ_Exprp
                = ಹೊಸ AstExtend{fl, ditemp->weightp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 64};
            // := on a range weights every element, so scale by the range size.
            ಆದರೆ (!ditemp->is_ಸಂಪೂರ್ಣ()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lop = VN_CAST(irp->lhsp(), Const);
                    ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ hip = VN_CAST(irp->rhsp(), Const);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangeSizep;
                    ಆದರೆ (lop && hip) {
                        ಸ್ಥಿರಾಂಕ uint64_t range_ಗಾತ್ರ = hip->ಗಮ್ಯ_UQuad() - lop->ಗಮ್ಯ_UQuad() + 1;
                        rangeSizep = ಹೊಸ AstConst{fl, AstConst::Unsized64{}, range_ಗಾತ್ರ};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        rangeSizep = ಹೊಸ Ast_ಕೂಡಿಸು{
                            fl, ಹೊಸ AstConst{fl, AstConst::Unsized64{}, 1},
                            ಹೊಸ AstSub{fl,
                                       ಹೊಸ AstExtend{fl, irp->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 64},
                                       ಹೊಸ AstExtend{fl, irp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 64}}};
                    }
                    ತೂಕ_Exprp = ಹೊಸ AstMul{fl, ತೂಕ_Exprp, rangeSizep};
                }
            }
            buckets.ತಳ್ಳು_ಹಿಂಭಾಗ({ditemp->rangep(), ತೂಕ_Exprp});
        }
        ಹಿಂತಿರುಗಿ buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Dist_Membership_ಪದ(AstDist* distp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(rangep, InsideRange)) {
            ಹಿಂತಿರುಗಿ newDistRangeMembership(distp, irp);
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ eqExprp = distp->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        eqExprp->user1(ನಿಜ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ eqp = ಹೊಸ AstEq{fl, eqExprp, rangep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
        eqp->user1(ನಿಜ);
        ಹಿಂತಿರುಗಿ eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* buildDistMembership(AstDist* distp,
                                           ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Dist_ಪಾತ್ರೆ>& buckets) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* unionExprp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಪಾತ್ರೆ : buckets) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ memberp = new_Dist_Membership_ಪದ(distp, ಪಾತ್ರೆ.rangep);
            ಆದರೆ (!unionExprp) {
                unionExprp = memberp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                unionExprp = ಹೊಸ AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(ನಿಜ);
            }
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newGateJoin(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp, ತಾರ್ಕಿಕ isAnd) {
        ಆದರೆ (ap->same_ವೃಕ್ಷ(bp)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(bp), bp);
            ಹಿಂತಿರುಗಿ ap;
        }
        ಆದರೆ (VN_IS(bp, Const)) std::ಅದಲುಬದಲು(ap, bp);
        ಆದರೆ (VN_IS(ap, Const)) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ constWins = VN_AS(ap, Const)->isZero() == isAnd;
            ಆದರೆ (!constWins) std::ಅದಲುಬದಲು(ap, bp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(bp), bp);
            ಹಿಂತಿರುಗಿ ap;
        }
        ಆದರೆ (isAnd) ಹಿಂತಿರುಗಿ ಹೊಸ AstLogAnd{fl, ap, bp};
        ಹಿಂತಿರುಗಿ ಹೊಸ AstLogOr{fl, ap, bp};
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newGateOr(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) {
        ಹಿಂತಿರುಗಿ newGateJoin(fl, ap, bp, ಸುಳ್ಳು);
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newGateAnd(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ap, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* bp) {
        ಹಿಂತಿರುಗಿ newGateJoin(fl, ap, bp, ನಿಜ);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_ಬಹುಲಕ_ಬಿಟ್_Read(AstVar* varp, ಸ್ಥಿರಾಂಕ AstMemberSel* mselp, AstVar* rand_ಬಹುಲಕ_Varp,
                                File_ಸಾಲು* fl) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* arrayp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (varp->lifetime().isStatic()) {
            AstClass* ಸ್ಥಿರಾಂಕ ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ಸ್ಥಿರಾಂಕ smodep = get_Static_Rand_ಬಹುಲಕ_Var(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = ಹೊಸ AstVarRef{fl, VN_AS(smodep->user2p(), ಗಂಟು_Module), smodep, VAccess::READ};
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (mselp) {
            AstVar* ಸ್ಥಿರಾಂಕ memberModep = get_Rand_ಬಹುಲಕ_Var_From_Class(VN_AS(varp->user2p(), ಗಂಟು_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = ಹೊಸ AstMemberSel{fl, mselp->fromp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), memberModep};
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(rand_ಬಹುಲಕ_Varp, varp, "rand_mode variable without a class mode array");
            arrayp = ಹೊಸ AstVarRef{fl, VN_AS(rand_ಬಹುಲಕ_Varp->user2p(), ಗಂಟು_Module), rand_ಬಹುಲಕ_Varp,
                                   VAccess::READ};
        }
        ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ಸ್ಥಿರಾಂಕ atp
            = ಹೊಸ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, ಹೊಸ AstConst{fl, rmode.ಸೂಚ್ಯಂಕ}};
        atp->dtype_ಗಣ_UInt32();
        ಹಿಂತಿರುಗಿ atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newVarGate(AstVar* varp, ಸ್ಥಿರಾಂಕ AstMemberSel* mselp, ತಾರ್ಕಿಕ ownerLevel,
                            AstVar* rand_ಬಹುಲಕ_Varp, File_ಸಾಲು* fl) {
        ಆದರೆ (!varp->rand().isRandomizable()) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}, ownerLevel};
        }
        ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = varp->user1()};
        ಆದರೆ (!rmode.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}};
        ಹಿಂತಿರುಗಿ new_ಬಹುಲಕ_ಬಿಟ್_Read(varp, mselp, rand_ಬಹುಲಕ_Varp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newAccessGate(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ ownerLevel, AstVar* rand_ಬಹುಲಕ_Varp,
                               File_ಸಾಲು* fl) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ mselp = VN_CAST(nodep, MemberSel)) {
            ಹಿಂತಿರುಗಿ newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, rand_ಬಹುಲಕ_Varp, fl),
                              newAccessGate(mselp->fromp(), ನಿಜ, rand_ಬಹುಲಕ_Varp, fl));
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
            ಹಿಂತಿರುಗಿ newVarGate(refp->varp(), ಶೂನ್ಯಸೂಚಕ, ownerLevel, rand_ಬಹುಲಕ_Varp, fl);
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
            ಹಿಂತಿರುಗಿ newAccessGate(selp->fromp(), ನಿಜ, rand_ಬಹುಲಕ_Varp, fl);
        }
        ಆದರೆ (dist_Bound_Refs_ಬಹುಲಕ_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newDistGate(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, AstVar* rand_ಬಹುಲಕ_Varp, File_ಸಾಲು* fl) {
        ಆದರೆ (VN_IS(nodep, ಗಂಟು_Var_Ref) || VN_IS(nodep, MemberSel)) {
            ಹಿಂತಿರುಗಿ newAccessGate(nodep, ಸುಳ್ಳು, rand_ಬಹುಲಕ_Varp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Sel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
            ಹಿಂತಿರುಗಿ newDistGate(selp->fromp(), rand_ಬಹುಲಕ_Varp, fl);
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, Sel)) {
            ಹಿಂತಿರುಗಿ newDistGate(selp->fromp(), rand_ಬಹುಲಕ_Varp, fl);
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstStructSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(nodep, StructSel)) {
            ಹಿಂತಿರುಗಿ newDistGate(selp->fromp(), rand_ಬಹುಲಕ_Varp, fl);
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp = VN_CAST(nodep, CMethodHard)) {
            ಆದರೆ (methodp->ವಿಧಾನ() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                ಹಿಂತಿರುಗಿ newDistGate(methodp->fromp(), rand_ಬಹುಲಕ_Varp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(nodep, Cond)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thenGatep = newDistGate(condp->thenp(), rand_ಬಹುಲಕ_Varp, fl);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elseGatep = newDistGate(condp->elsep(), rand_ಬಹುಲಕ_Varp, fl);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* armGatep = thenGatep;
            ಆದರೆ (thenGatep->same_ವೃಕ್ಷ(elseGatep)) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(elseGatep), elseGatep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                armGatep
                    = ಹೊಸ AstCond{fl, condp->condp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), thenGatep, elseGatep};
            }
            ಹಿಂತಿರುಗಿ newGateOr(fl, newDistGate(condp->condp(), rand_ಬಹುಲಕ_Varp, fl), armGatep);
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Uniop* ಸ್ಥಿರಾಂಕ uopp = VN_CAST(nodep, ಗಂಟು_Uniop)) {
            ಹಿಂತಿರುಗಿ newDistGate(uopp->lhsp(), rand_ಬಹುಲಕ_Varp, fl);
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ bopp = VN_CAST(nodep, ಗಂಟು_Biop)) {
            ಹಿಂತಿರುಗಿ newGateOr(fl, newDistGate(bopp->lhsp(), rand_ಬಹುಲಕ_Varp, fl),
                             newDistGate(bopp->rhsp(), rand_ಬಹುಲಕ_Varp, fl));
        }
        ಆದರೆ (dist_Bound_Refs_ಬಹುಲಕ_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* buildFrozenMembership(AstDist* distp,
                                             ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Dist_ಪಾತ್ರೆ>& buckets) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* unionExprp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಪಾತ್ರೆ : buckets) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* termp = new_Dist_Membership_ಪದ(distp, ಪಾತ್ರೆ.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ runtime_ತೂಕ
                = ಪಾತ್ರೆ.ತೂಕ_Exprp->exists([](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref*) { ಹಿಂತಿರುಗಿ ನಿಜ; });
            ಆದರೆ (runtime_ತೂಕ) {
                AstNeq* ಸ್ಥಿರಾಂಕ nzp = ಹೊಸ AstNeq{fl, ಪಾತ್ರೆ.ತೂಕ_Exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು),
                                               ಹೊಸ AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(ನಿಜ);
                termp = ಹೊಸ AstLogAnd{fl, termp, nzp};
                termp->user1(ನಿಜ);
            }
            ಆದರೆ (unionExprp) {
                unionExprp = ಹೊಸ AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                unionExprp = termp;
            }
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_ಗಂಟು* gateFrozenDist(Ast_ಗಂಟು* chainp, AstDist* distp,
                            ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Dist_ಪಾತ್ರೆ>& buckets, AstVar* rand_ಬಹುಲಕ_Varp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* gatep = newDistGate(distp->exprp(), rand_ಬಹುಲಕ_Varp, fl);
        ಆದರೆ (VN_IS(gatep, Const)) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(gatep), gatep);
            ಆದರೆ (drawable) ಹಿಂತಿರುಗಿ chainp;
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(chainp), chainp);
            ಹಿಂತಿರುಗಿ buildFrozenMembership(distp, buckets);
        }
        ಹಿಂತಿರುಗಿ ಹೊಸ AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_ಏಕರೂಪ_Range_Pick(AstDist* distp, ಸ್ಥಿರಾಂಕ AstInsideRange* irp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp = distp->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp->user1(ನಿಜ);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ dist_ಅಗಲ = distp->exprp()->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ lopC = VN_CAST(irp->lhsp(), Const);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ hipC = VN_CAST(irp->rhsp(), Const);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rangeSzp;
        ಆದರೆ (lopC && hipC) {
            ಸ್ಥಿರಾಂಕ uint64_t rsz = hipC->ಗಮ್ಯ_UQuad() - lopC->ಗಮ್ಯ_UQuad() + 1;
            rangeSzp = ಹೊಸ AstConst{fl, AstConst::Unsized64{}, rsz};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isSigned = irp->lhsp()->isSigned();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lo64p
                = isSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                 ಹೊಸ AstExtendS{fl, irp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 64})
                           : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                 ಹೊಸ AstExtend{fl, irp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 64});
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ hi64p
                = isSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                 ಹೊಸ AstExtendS{fl, irp->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 64})
                           : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(
                                 ಹೊಸ AstExtend{fl, irp->rhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 64});
            rangeSzp = ಹೊಸ Ast_ಕೂಡಿಸು{fl, ಹೊಸ AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  ಹೊಸ AstSub{fl, hi64p, lo64p}};
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rand64p = ಹೊಸ AstRand{fl, ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
        rand64p->dtype_ಗಣ_UInt64();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ offsetp
            = ಹೊಸ AstCCast{fl, ಹೊಸ AstModDiv{fl, rand64p, rangeSzp}, dist_ಅಗಲ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ valuep = ಹೊಸ Ast_ಕೂಡಿಸು{fl, irp->lhsp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), offsetp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ eqp = ಹೊಸ AstEq{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp, valuep};
        eqp->user1(ನಿಜ);
        ಹಿಂತಿರುಗಿ eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_ಗಂಟು* build_Weighted_ಪಾತ್ರೆ_Chain(AstDist* distp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Dist_ಪಾತ್ರೆ>& buckets,
                                      AstVar* ಪಾತ್ರೆ_Varp,
                                      ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& cumSums) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* chainp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = static_cast<ಪೂರ್ಣಾಂಕ>(buckets.ಗಾತ್ರ_2()) - 1; i >= 0; --i) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* constraintExprp;
            ಸ್ಥಿರಾಂಕ AstInsideRange* ಸ್ಥಿರಾಂಕ irp = VN_CAST(buckets[i].rangep, InsideRange);
            ಆದರೆ (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (irp) {
                constraintExprp = new_ಏಕರೂಪ_Range_Pick(distp, irp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp = distp->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp->user1(ನಿಜ);
                constraintExprp
                    = ಹೊಸ AstEq{fl, dist_ಅಭಿವ್ಯಕ್ತಿ_Copyp, buckets[i].rangep->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                constraintExprp->user1(ನಿಜ);
            }
            ಆದರೆ (!chainp) {
                chainp = constraintExprp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp
                    = ಹೊಸ AstLte{fl, ಹೊಸ AstVarRef{fl, ಪಾತ್ರೆ_Varp, VAccess::READ}, cumSums[i]};
                condp->user1(ನಿಜ);
                chainp = ಹೊಸ AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(ನಿಜ);
            }
        }
        Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ chainExprp = ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, chainp};
        chainExprp->isSoft(ನಿಜ);
        ಹಿಂತಿರುಗಿ chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    ಶೂನ್ಯ ಅಧೋ_Dist_Constraints(AstTask* taskp, Ast_ಗಂಟು* constrItemsp, AstVar* rand_ಬಹುಲಕ_Varp,
                              AstConstraintForeach* foreachp = ಶೂನ್ಯಸೂಚಕ) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_ಗಂಟು* foreachTailp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Stmt = [&](Ast_ಗಂಟು* nodep) {
            ಆದರೆ (foreachp) {
                ಆದರೆ (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    foreachTailp->ಕೂಡಿಸು_ಮುಂದಿನ(nodep);
                    foreachTailp = nodep;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                taskp->ಕೂಡಿಸು_Stmtsp(nodep);
            }
        };

        ಗಾಗಿ (Ast_ಗಂಟು *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            ಆದರೆ (AstConstraintIf* ಸ್ಥಿರಾಂಕ cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                ಅಧೋ_Dist_Constraints(taskp, cifp->thensp(), rand_ಬಹುಲಕ_Varp, foreachp);
                ಆದರೆ (cifp->elsesp()) {
                    ಅಧೋ_Dist_Constraints(taskp, cifp->elsesp(), rand_ಬಹುಲಕ_Varp, foreachp);
                }
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (AstConstraintForeach* ಸ್ಥಿರಾಂಕ cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                ಅಧೋ_Dist_Constraints(taskp, cfep->bodyp(), rand_ಬಹುಲಕ_Varp, cfep);
                ಮುಂದುವರಿ;
            }

            Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ constrExprp = VN_CAST(itemp, Constraint_ಅಭಿವ್ಯಕ್ತಿ);
            ಆದರೆ (!constrExprp) ಮುಂದುವರಿ;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            ಆದರೆ (AstLogIf* ಸ್ಥಿರಾಂಕ ತುದಿ_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_ಗಂಟು* chainEndp = ತುದಿ_Log_Ifp->rhsp();
                ಆಗ (AstLogIf* ಸ್ಥಿರಾಂಕ innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                ಆದರೆ (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ಸ್ಥಿರಾಂಕ liftedp = lift_Log_If_Chain_ಗಮ್ಯ_Constraint_If(ತುದಿ_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(constrExprp), constrExprp);
                    ಅಧೋ_Dist_Constraints(taskp, liftedp->thensp(), rand_ಬಹುಲಕ_Varp, foreachp);
                    ಮುಂದುವರಿ;
                }
            }

            AstDist* ಸ್ಥಿರಾಂಕ distp = VN_CAST(constrExprp->exprp(), Dist);
            ಆದರೆ (!distp) ಮುಂದುವರಿ;

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = distp->fileline();

            std::ಸದಿಶ<Dist_ಪಾತ್ರೆ> buckets = collectDistBuckets(distp);

            ಆದರೆ (buckets.ಖಾಲಿಗಳು()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ truep
                    = ಹೊಸ Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ{fl, ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_True{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(constrExprp), constrExprp);
                ಮುಂದುವರಿ;
            }

            Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಒಟ್ಟು_ತೂಕ_Exprp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& ಪಾತ್ರೆ : buckets) {
                ಆದರೆ (!ಒಟ್ಟು_ತೂಕ_Exprp) {
                    ಒಟ್ಟು_ತೂಕ_Exprp = ಪಾತ್ರೆ.ತೂಕ_Exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಒಟ್ಟು_ತೂಕ_Exprp = ಹೊಸ Ast_ಕೂಡಿಸು{fl, ಒಟ್ಟು_ತೂಕ_Exprp,
                                                  ಪಾತ್ರೆ.ತೂಕ_Exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    ಒಟ್ಟು_ತೂಕ_Exprp->dtype_ಗಣ_UInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ distId = m_dist_ಅಂಕೆ++;
            ಸ್ಥಿರಾಂಕ std::string ಒಟ್ಟು_Name = "__Vdist_total" + cvt_ಗಮ್ಯ_Str(distId);
            AstVar* ಸ್ಥಿರಾಂಕ ಒಟ್ಟು_Varp
                = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, ಒಟ್ಟು_Name, taskp->ಕಂಡುಹಿಡಿ_UInt64_DType()};
            ಒಟ್ಟು_Varp->noSubst(ನಿಜ);
            ಒಟ್ಟು_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ಒಟ್ಟು_Varp->funcLocal(ನಿಜ);
            ಒಟ್ಟು_Varp->is_ಆಂತರಿಕ(ನಿಜ);
            ಕೂಡಿಸು_Stmt(ಒಟ್ಟು_Varp);
            ಕೂಡಿಸು_Stmt(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, ಒಟ್ಟು_Varp, VAccess::WRITE}, ಒಟ್ಟು_ತೂಕ_Exprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ಸ್ಥಿರಾಂಕ std::string ಪಾತ್ರೆ_Name = "__Vdist_bucket" + cvt_ಗಮ್ಯ_Str(distId);
            AstVar* ಸ್ಥಿರಾಂಕ ಪಾತ್ರೆ_Varp
                = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, ಪಾತ್ರೆ_Name, taskp->ಕಂಡುಹಿಡಿ_UInt64_DType()};
            ಪಾತ್ರೆ_Varp->noSubst(ನಿಜ);
            ಪಾತ್ರೆ_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            ಪಾತ್ರೆ_Varp->funcLocal(ನಿಜ);
            ಪಾತ್ರೆ_Varp->is_ಆಂತರಿಕ(ನಿಜ);
            ಕೂಡಿಸು_Stmt(ಪಾತ್ರೆ_Varp);

            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ randp = ಹೊಸ AstRand{fl, ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
            randp->dtype_ಗಣ_UInt64();
            ಕೂಡಿಸು_Stmt(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                fl, ಹೊಸ AstVarRef{fl, ಪಾತ್ರೆ_Varp, VAccess::WRITE},
                ಹೊಸ Ast_ಕೂಡಿಸು{
                    fl, ಹೊಸ AstConst{fl, AstConst::Unsized64{}, 1},
                    ಹೊಸ AstModDiv{fl, randp, ಹೊಸ AstVarRef{fl, ಒಟ್ಟು_Varp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> cumSums;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* runningSump = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < buckets.ಗಾತ್ರ_2(); ++i) {
                ಆದರೆ (!runningSump) {
                    runningSump = buckets[i].ತೂಕ_Exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    runningSump = ಹೊಸ Ast_ಕೂಡಿಸು{fl, runningSump,
                                             buckets[i].ತೂಕ_Exprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು)};
                    runningSump->dtype_ಗಣ_UInt64();
                }
                cumSums.ತಳ್ಳು_ಹಿಂಭಾಗ(runningSump->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ));
            }

            Ast_ಗಂಟು* chainp = build_Weighted_ಪಾತ್ರೆ_Chain(distp, buckets, ಪಾತ್ರೆ_Varp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, rand_ಬಹುಲಕ_Varp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& ಪಾತ್ರೆ : buckets) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(ಪಾತ್ರೆ.ತೂಕ_Exprp), ಪಾತ್ರೆ.ತೂಕ_Exprp);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            ತಳ್ಳು_Deletep(cumSums.ಹಿಂಭಾಗ());
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Dyn_Arr_ದ_Class_Type_Recurse(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep) {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ refp = dtypep->skipRefp();
        ಆದರೆ (VN_IS(refp, DynArrayDType) || VN_IS(refp, ಸರತಿ_DType)) {
            ಹಿಂತಿರುಗಿ is_Dyn_Arr_ದ_Class_Type_Recurse(refp->subDTypep());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(refp, ClassRefDType)) {
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    ಶೂನ್ಯ ಕೂಡಿಸು_Array_Elem_Refresh(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstVar* ಸ್ಥಿರಾಂಕ arrVarp, AstFunc* ಸ್ಥಿರಾಂಕ randomizep,
                             AstVar* ಸ್ಥಿರಾಂಕ genp) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ genModp = VN_AS(genp->user2p(), ಗಂಟು_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ಸ್ಥಿರಾಂಕ uint32_t unpackedDims = arrVarp->dtypep()->dimensions(ಸುಳ್ಳು).ಎರಡನೇ;
        ಆದರೆ (is_Dyn_Arr_ದ_Class_Type_Recurse(arrVarp->dtypep())) {
            ಆದರೆ (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            ಹಿಂತಿರುಗಿ;
        }
        AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp
            = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_ಗಣ_Void();

        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ classp = VN_AS(arrVarp->user2p(), ಗಂಟು_Module);
        AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = ಹೊಸ AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->ಕೂಡಿಸು_Pinsp(varRefp);

        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಅಗಲ = arrayElementDTypep(arrVarp->dtypep())->ಅಗಲ();

        methodp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{fl, AstConst::Unsized64{}, ಅಗಲ});
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ varnamep
            = ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "\"" + arrVarp->name() + "\"", arrVarp->ಅಗಲ()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->ಕೂಡಿಸು_Pinsp(varnamep);
        methodp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = arrVarp->user1()};
        ಆದರೆ (rmode.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) {
            methodp->ಕೂಡಿಸು_Pinsp(ಹೊಸ AstConst{fl, AstConst::Unsized64{}, rmode.ಸೂಚ್ಯಂಕ});
        }

        randomizep->ಕೂಡಿಸು_Stmtsp(methodp->makeStmt());
    }

    Ast_ಗಂಟು_Stmt* new_Pin_ಗಾತ್ರ_Variable(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstVar* ಸ್ಥಿರಾಂಕ arrVarp,
                                    AstVar* ಸ್ಥಿರಾಂಕ genp) {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ genModp = VN_AS(genp->user2p(), ಗಂಟು_Module);
        AstVar* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Varp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ಸ್ಥಿರಾಂಕ pinp
            = ಹೊಸ AstCMethodHard{fl, ಹೊಸ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtype_ಗಣ_Void();
        AstCExpr* ಸ್ಥಿರಾಂಕ namep = ಹೊಸ AstCExpr{fl, AstCExpr::ಶುದ್ಧ{}, "\"" + ಗಾತ್ರ_Varp->name() + "\""};
        namep->dtype_ಗಣ_UInt32();
        pinp->ಕೂಡಿಸು_Pinsp(namep);
        pinp->ಕೂಡಿಸು_Pinsp(
            ಹೊಸ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(ಗಾತ್ರ_Varp->ಅಗಲ())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Var_Refp = ಹೊಸ AstVarRef{fl, ಗಾತ್ರ_Varp, VAccess::READ};
        ಗಾತ್ರ_Var_Refp->classOrPackagep(VN_AS(ಗಾತ್ರ_Varp->user2p(), ಗಂಟು_Module));
        pinp->ಕೂಡಿಸು_Pinsp(ಗಾತ್ರ_Var_Refp);
        ಹಿಂತಿರುಗಿ pinp->makeStmt();
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Frozen_ಗಾತ್ರ_Pin(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, AstVar* ಸ್ಥಿರಾಂಕ arrVarp, AstFunc* ಸ್ಥಿರಾಂಕ randomizep,
                          AstVar* ಸ್ಥಿರಾಂಕ genp, AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Varp) {
        ಸ್ಥಿರಾಂಕ Randomize_ಬಹುಲಕ rmode = {.asUQuad = arrVarp->user1()};
        ಆದರೆ (!rmode.ಬಳಸುತ್ತದೆ_ಬಹುಲಕ) ಹಿಂತಿರುಗಿ;
        AstVar* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Varp = VN_AS(arrVarp->user4p(), Var);
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ arrClassp = VN_AS(arrVarp->user2p(), ಗಂಟು_Module);
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Classp = VN_AS(ಗಾತ್ರ_Varp->user2p(), ಗಂಟು_Module);
        AstVarRef* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Writep = ಹೊಸ AstVarRef{fl, ಗಾತ್ರ_Classp, ಗಾತ್ರ_Varp, VAccess::WRITE};
        ಸ್ಥಿರಾಂಕ VCMethod ಗಾತ್ರ_ವಿಧಾನ = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_ಗಾತ್ರ
                                        : VCMethod::DYN_ಗಾತ್ರ;
        AstCMethodHard* ಸ್ಥಿರಾಂಕ currentSizep = ಹೊಸ AstCMethodHard{
            fl, ಹೊಸ AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, ಗಾತ್ರ_ವಿಧಾನ};
        currentSizep->dtypep(ಗಾತ್ರ_Varp->dtypep());
        randomizep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಗಾತ್ರ_Writep, currentSizep});
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ modep = new_ಬಹುಲಕ_ಬಿಟ್_Read(arrVarp, ಶೂನ್ಯಸೂಚಕ, rand_ಬಹುಲಕ_Varp, fl);
        randomizep->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstIf{fl, ಹೊಸ AstLogNot{fl, modep}, new_Pin_ಗಾತ್ರ_Variable(fl, arrVarp, genp)});
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_ಅಂಕೆ);
        VL_RESTORER_COPY(m_rand_Array_Unique_ಹೆಸರುಗಳು);
        m_modp = nodep;
        m_rand_Case_ಅಂಕೆ = 0;
        m_rand_Array_Unique_ಹೆಸರುಗಳು.reset();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_ಅಂಕೆ);
        VL_RESTORER(m_dist_ಅಂಕೆ);
        m_modp = nodep;
        m_rand_Case_ಅಂಕೆ = 0;
        m_dist_ಅಂಕೆ = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        ಆದರೆ (!nodep->user1()) ಹಿಂತಿರುಗಿ;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->ತಳ_Most_Classp()->needRNG(ನಿಜ);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ basic_ಮೊದಲ = classOwnsGlobalConstraint(nodep);

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        AstFunc* ಸ್ಥಿರಾಂಕ randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ಸ್ಥಿರಾಂಕ fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Varp = get_Rand_ಬಹುಲಕ_Var_From_Class(nodep);
        ಕೂಡಿಸು_Pre_Post_Call(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ಆದರೆ (classHasRandClassMembers(nodep)) {
            AstTask* ಸ್ಥಿರಾಂಕ preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->ಕೂಡಿಸು_Stmtsp((ಹೊಸ AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* beginValp = ಶೂನ್ಯಸೂಚಕ;
        AstVar* genp = getRandomGenerator(nodep);
        ಆದರೆ (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ಸ್ಥಿರಾಂಕ prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ಸ್ಥಿರಾಂಕ classp, AstConstraint* ಸ್ಥಿರಾಂಕ constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                ಆದರೆ (!taskp) {
                    taskp = new_ಸಿದ್ಧತೆ_Constraint_Task(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ಸ್ಥಿರಾಂಕ resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ಸ್ಥಿರಾಂಕ resizeTaskRefp
                        = ಹೊಸ AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->ಕೂಡಿಸು_Stmtsp(resizeTaskRefp->makeStmt());
                }

                ಆದರೆ (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                ಆದರೆ (constrp->itemsp()) {
                    ಅಧೋ_Dist_Constraints(taskp, constrp->itemsp(), rand_ಬಹುಲಕ_Varp);
                }
                VInsertion_ಗಣ<AstVar*>& ಗಾತ್ರ_Arrays = m_ಗಾತ್ರ_Constrained_Arrays[classp];
                Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), ಶೂನ್ಯಸೂಚಕ,
                    genp,          rand_ಬಹುಲಕ_Varp, m_writtenVars,     &m_uniqueConstraintId,
                    m_unique_ಹೆಸರುಗಳು, randomizep,   &ಗಾತ್ರ_Arrays,       prepareArraysTaskp};
                ಆದರೆ (constrp->itemsp()) {
                    taskp->ಕೂಡಿಸು_Stmtsp(wrap_If_Constraint_ಬಹುಲಕ(
                        nodep, constrp, constrp->itemsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ constraint_ಆಳ = [](ಸ್ಥಿರಾಂಕ AstConstraint* constrp) {
                    ಪೂರ್ಣಾಂಕ ಆಳ = 0;
                    ಗಾತ್ರ_t pos = 0;
                    ಆಗ ((pos = constrp->name().ಕಂಡುಹಿಡಿ(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++ಆಳ;
                        pos += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    ಹಿಂತಿರುಗಿ ಆಳ;
                };
                ಪೂರ್ಣಾಂಕ max_ಆಳ = 0;
                nodep->foreachMember([&](AstClass* ಸ್ಥಿರಾಂಕ, AstConstraint* ಸ್ಥಿರಾಂಕ constrp) {
                    max_ಆಳ = std::max(max_ಆಳ, constraint_ಆಳ(constrp));
                });
                AstTask* ಸ್ಥಿರಾಂಕ ಸಿದ್ಧತೆ_All_Taskp = get_Create_Constraint_ಸಿದ್ಧತೆ_Func(nodep, m_memberMap);
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ d = max_ಆಳ; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ಸ್ಥಿರಾಂಕ classp, AstConstraint* ಸ್ಥಿರಾಂಕ constrp) {
                            ಆದರೆ (constraint_ಆಳ(constrp) != d) ಹಿಂತಿರುಗಿ;
                            AstTask* ಸ್ಥಿರಾಂಕ taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ಸ್ಥಿರಾಂಕ ಸಿದ್ಧತೆ_Task_Refp
                                = ಹೊಸ AstTaskRef{constrp->fileline(), taskp};
                            ಸಿದ್ಧತೆ_Task_Refp->classOrPackagep(classp);
                            ಸಿದ್ಧತೆ_All_Taskp->ಕೂಡಿಸು_Stmtsp(ಸಿದ್ಧತೆ_Task_Refp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertion_ಗಣ<AstVar*> ಗಾತ್ರ_Array_Vars;
            ಆದರೆ (nodep->extendsp()) {
                AstClass* ಪೋಷಕ_Classp = nodep->extendsp()->classp();
                ಆಗ (ಪೋಷಕ_Classp) {
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಗಾತ್ರ_Arrays_It = m_ಗಾತ್ರ_Constrained_Arrays.ಕಂಡುಹಿಡಿ(ಪೋಷಕ_Classp);
                    ಆದರೆ (ಗಾತ್ರ_Arrays_It != m_ಗಾತ್ರ_Constrained_Arrays.ಕೊನೆ()) {
                        ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ arrVarp : ಗಾತ್ರ_Arrays_It->ಎರಡನೇ) {
                            ಗಾತ್ರ_Array_Vars.ಸೇರಿಸು(arrVarp);
                        }
                    }
                    AstFunc* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Randomizep
                        = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(ಪೋಷಕ_Classp, "randomize"), Func);
                    ಆದರೆ (ಪೋಷಕ_Randomizep && ಪೋಷಕ_Randomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = ಪೋಷಕ_Randomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು_Clear_Constraints = ಸುಳ್ಳು;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                ಆದರೆ (methodp->ವಿಧಾನ() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->ವಿಧಾನ() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->ಕೂಡಿಸು_Stmtsp(stmtp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (methodp->ವಿಧಾನ()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    ಸಿಕ್ಕಿತು_Clear_Constraints = ನಿಜ;
                                }
                            });
                            ಆದರೆ (ಸಿಕ್ಕಿತು_Clear_Constraints) ನಿಲ್ಲಿಸು;
                        }
                    }
                    ಪೋಷಕ_Classp
                        = ಪೋಷಕ_Classp->extendsp() ? ಪೋಷಕ_Classp->extendsp()->classp() : ಶೂನ್ಯಸೂಚಕ;
                }
            }
            randomizep->ಕೂಡಿಸು_Stmtsp(implementConstraintsClear(fl, genp));

            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಗಾತ್ರ_Arrays_It = m_ಗಾತ್ರ_Constrained_Arrays.ಕಂಡುಹಿಡಿ(nodep);
            ಆದರೆ (ಗಾತ್ರ_Arrays_It != m_ಗಾತ್ರ_Constrained_Arrays.ಕೊನೆ()) {
                ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ arrVarp : ಗಾತ್ರ_Arrays_It->ಎರಡನೇ) ಗಾತ್ರ_Array_Vars.ಸೇರಿಸು(arrVarp);
            }
            AstTask* ಸ್ಥಿರಾಂಕ ಸಿದ್ಧತೆ_All_Taskp = get_Create_Constraint_ಸಿದ್ಧತೆ_Func(nodep, m_memberMap);
            ಆದರೆ (!ಗಾತ್ರ_Array_Vars.ಖಾಲಿಗಳು() && prepareArraysTaskp->stmtsp()) {
                Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ preparep = (ಹೊಸ AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    ತಾರ್ಕಿಕ writesArray = ಸುಳ್ಳು;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->ವಿಧಾನ() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    ಆದರೆ (!writesArray) ಮುಂದುವರಿ;
                    stmtp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(preparep);
                    ನಿಲ್ಲಿಸು;
                }
            }
            AstTaskRef* ಸ್ಥಿರಾಂಕ ಸಿದ್ಧತೆ_Task_Refp = ಹೊಸ AstTaskRef{fl, ಸಿದ್ಧತೆ_All_Taskp};
            randomizep->ಕೂಡಿಸು_Stmtsp(ಸಿದ್ಧತೆ_Task_Refp->makeStmt());

            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ genModp = VN_AS(genp->user2p(), ಗಂಟು_Module);

            AstCExpr* ಸ್ಥಿರಾಂಕ solverCallp = ಹೊಸ AstCExpr{fl};
            solverCallp->dtype_ಗಣ_ಬಿಟ್();
            solverCallp->ಕೂಡಿಸು(ಹೊಸ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->ಕೂಡಿಸು(".next(__Vm_rng)");
            ಆದರೆ (!ಗಾತ್ರ_Array_Vars.ಖಾಲಿಗಳು()) {
                AstVar* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Ok_Varp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::ಬಿಟ್)};
                ಗಾತ್ರ_Ok_Varp->funcLocal(ನಿಜ);
                randomizep->ಕೂಡಿಸು_Stmtsp(ಗಾತ್ರ_Ok_Varp);

                AstVar* ಸ್ಥಿರಾಂಕ finalOkVarp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::ಬಿಟ್)};
                finalOkVarp->funcLocal(ನಿಜ);
                randomizep->ಕೂಡಿಸು_Stmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ arrVarp : ಗಾತ್ರ_Array_Vars)
                    ಕೂಡಿಸು_Frozen_ಗಾತ್ರ_Pin(fl, arrVarp, randomizep, genp, rand_ಬಹುಲಕ_Varp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->ಕೂಡಿಸು_Stmtsp(
                    ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, ಗಾತ್ರ_Ok_Varp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ resizeAllTaskp
                    = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ಸ್ಥಿರಾಂಕ resizeTaskRefp = ಹೊಸ AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{fl, ಹೊಸ AstVarRef{fl, ಗಾತ್ರ_Ok_Varp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), ಶೂನ್ಯಸೂಚಕ};
                    randomizep->ಕೂಡಿಸು_Stmtsp(ifp);
                }

                // Refresh array element tables after resize
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& arrVarp : ಗಾತ್ರ_Array_Vars) {
                    ಕೂಡಿಸು_Array_Elem_Refresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->ಕೂಡಿಸು_Stmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ಸ್ಥಿರಾಂಕ ಸಿದ್ಧತೆ_Task_Refp2 = ಹೊಸ AstTaskRef{fl, ಸಿದ್ಧತೆ_All_Taskp};
                randomizep->ಕೂಡಿಸು_Stmtsp(ಸಿದ್ಧತೆ_Task_Refp2->makeStmt());

                ಗಾಗಿ (AstVar* ಸ್ಥಿರಾಂಕ arrVarp : ಗಾತ್ರ_Array_Vars) {
                    randomizep->ಕೂಡಿಸು_Stmtsp(new_Pin_ಗಾತ್ರ_Variable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ಸ್ಥಿರಾಂಕ solverCallp2 = ಹೊಸ AstCExpr{fl};
                solverCallp2->dtype_ಗಣ_ಬಿಟ್();
                solverCallp2->ಕೂಡಿಸು(ಹೊಸ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->ಕೂಡಿಸು(".next(__Vm_rng)");
                randomizep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    fl, ಹೊಸ AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = ಹೊಸ AstAnd{fl, ಹೊಸ AstVarRef{fl, ಗಾತ್ರ_Ok_Varp, VAccess::READ},
                                       ಹೊಸ AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } ಇಲ್ಲದಿದ್ದರೆ {
                beginValp = solverCallp;
            }
            ಆದರೆ (rand_ಬಹುಲಕ_Varp) {
                Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Classp = VN_AS(rand_ಬಹುಲಕ_Varp->user2p(), Class);
                Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ newp
                    = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(rand_ಬಹುಲಕ_Classp, "new"), ಗಂಟು_FTask);
                UASSERT_OBJ(newp, rand_ಬಹುಲಕ_Classp, "No new() in class");
                ಕೂಡಿಸು_ಗಣ_Rand_ಬಹುಲಕ(newp, genp, rand_ಬಹುಲಕ_Varp);
            }
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ static_Rand_ಬಹುಲಕ_Varp = get_Static_Rand_ಬಹುಲಕ_Var(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ newp = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "new"), ಗಂಟು_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                ಕೂಡಿಸು_ಗಣ_Static_Rand_ಬಹುಲಕ(newp, genp, static_Rand_ಬಹುಲಕ_Varp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            beginValp = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1};
        }

        ಕೂಡಿಸು_Update_Rand_Vars_Body(nodep, randomizep);

        AstFunc* ಸ್ಥಿರಾಂಕ basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        ಕೂಡಿಸು_Basic_Randomize_Body(basicRandomizep, nodep, rand_ಬಹುಲಕ_Varp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ಸ್ಥಿರಾಂಕ fvarRefp = ಹೊಸ AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            fl, fvarRefp, basic_ಮೊದಲ ? ಹೊಸ AstFuncRef{fl, basicRandomizep} : beginValp});

        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಗಾತ್ರ_Arrays_It = m_ಗಾತ್ರ_Constrained_Arrays.ಕಂಡುಹಿಡಿ(nodep);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ needs_ಗಾತ್ರ_Phase
            = ಗಾತ್ರ_Arrays_It != m_ಗಾತ್ರ_Constrained_Arrays.ಕೊನೆ() && !ಗಾತ್ರ_Arrays_It->ಎರಡನೇ.ಖಾಲಿಗಳು();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ emitResizeFallback = [&]() {
            ಆದರೆ (needs_ಗಾತ್ರ_Phase) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (AstTask* ಸ್ಥಿರಾಂಕ resizeAllTaskp
                = VN_AS(m_memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ಸ್ಥಿರಾಂಕ resizeTaskRefp = ಹೊಸ AstTaskRef{fl, resizeAllTaskp};
                randomizep->ಕೂಡಿಸು_Stmtsp(resizeTaskRefp->makeStmt());
            }
        };
        ಆದರೆ (!basic_ಮೊದಲ) emitResizeFallback();

        AstVarRef* ಸ್ಥಿರಾಂಕ fvarRefReadp = fvarRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        fvarRefReadp->access(VAccess::READ);

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಎರಡನೇ_Halfp
            = basic_ಮೊದಲ ? beginValp : ಹೊಸ AstFuncRef{fl, basicRandomizep};
        randomizep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, fvarRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                                            ಹೊಸ AstAnd{fl, fvarRefReadp, ಎರಡನೇ_Halfp}});

        ಆದರೆ (basic_ಮೊದಲ) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ಆದರೆ (classHasRandClassMembers(nodep)) {
            AstTask* ಸ್ಥಿರಾಂಕ postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->ಕೂಡಿಸು_Stmtsp((ಹೊಸ AstTaskRef{fl, postTaskp})->makeStmt());
        }

        ಕೂಡಿಸು_Pre_Post_Call(nodep, randomizep, "post_randomize");
        nodep->user1(ಸುಳ್ಳು);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRandCase* nodep) override {
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
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಮೊತ್ತ_DTypep = nodep->ಕಂಡುಹಿಡಿ_UInt64_DType();

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಸ್ಥಿರಾಂಕ std::string name = "__Vrandcase" + cvt_ಗಮ್ಯ_Str(m_rand_Case_ಅಂಕೆ++);
        AstVar* ಸ್ಥಿರಾಂಕ randVarp = ಹೊಸ AstVar{fl, VVarType::BLOCKTEMP, name, ಮೊತ್ತ_DTypep};
        randVarp->noSubst(ನಿಜ);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ಕೂಡಿಸು_ತಾತ್ಕಾಲಿಕ_Varsp(randVarp);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sump = ಹೊಸ AstConst{fl, AstConst::Widthed_ಮೌಲ್ಯ{}, 64, 0};
        Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ ಮೊದಲ_Ifsp
            = ಹೊಸ AstIf{fl, ಹೊಸ AstConst{fl, AstConst::ಬಿಟ್_False{}}, ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        Ast_ಗಂಟು_If* ifsp = ಮೊದಲ_Ifsp;

        ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = itemp->condsp()->unlink_Fr_ಹಿಂಭಾಗ();
            sump
                = ಹೊಸ Ast_ಕೂಡಿಸು{condp->fileline(), sump, ಹೊಸ AstExtend{itemp->fileline(), condp, 64}};
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ() : ಶೂನ್ಯಸೂಚಕ;
            AstVarRef* ಸ್ಥಿರಾಂಕ randVarRefp = ಹೊಸ AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_ಗಂಟು_If* ಸ್ಥಿರಾಂಕ newifp
                = ಹೊಸ AstIf{itemp->fileline(),
                            ಹೊಸ AstLte{condp->fileline(), randVarRefp, sump->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)},
                            stmtsp, ಶೂನ್ಯಸೂಚಕ};
            ifsp->ಕೂಡಿಸು_Elsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ಸ್ಥಿರಾಂಕ dispp = ಹೊಸ AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->ಕೂಡಿಸು_Elsesp(dispp);

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ randp = ಹೊಸ AstRand{fl, ಶೂನ್ಯಸೂಚಕ, ಸುಳ್ಳು};
        randp->dtype_ಗಣ_UInt64();
        AstVarRef* ಸ್ಥಿರಾಂಕ randVarRefp = ಹೊಸ AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, randVarRefp,
                            ಹೊಸ Ast_ಕೂಡಿಸು{fl, ಹೊಸ AstConst{fl, AstConst::Unsized64{}, 1},
                                       ಹೊಸ AstModDiv{fl, randp, sump}}};
        newp->ಕೂಡಿಸು_ಮುಂದಿನ(ಮೊದಲ_Ifsp);
        ಆದರೆ (debug() >= 9) newp->dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(ಹೊರಗೆ, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (nodep->name() == "rand_mode") {
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(nodep, ವಿಧಾನ_Call);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = ವಿಧಾನ_Callp ? ವಿಧಾನ_Callp->fromp() : ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ Rand_ಬಹುಲಕ_ಗುರಿ rand_ಬಹುಲಕ_ಗುರಿ = Rand_ಬಹುಲಕ_ಗುರಿ::get(fromp, m_modp);
            UASSERT_OBJ(rand_ಬಹುಲಕ_ಗುರಿ.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ಸ್ಥಿರಾಂಕ receiverp = rand_ಬಹುಲಕ_ಗುರಿ.receiverp;
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_ಗಂಟು* classLevelStaticLoopp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (isClassLevel && nodep->argsp()) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ sVarp = get_Static_Rand_ಬಹುಲಕ_Var(rand_ಬಹುಲಕ_ಗುರಿ.classp)) {
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ staticLhsp
                        = make_ಬಹುಲಕ_ನಿಯೋಜಿಸು_Lhs(fl, rand_ಬಹುಲಕ_ಗುರಿ.classp, ಶೂನ್ಯಸೂಚಕ, sVarp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ argClonep = nodep->argsp()->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
                    classLevelStaticLoopp = make_ಬಹುಲಕ_ಗಣ_Loop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Varp = receiverIsStaticRand
                                             ? get_Static_Rand_ಬಹುಲಕ_Var(rand_ಬಹುಲಕ_ಗುರಿ.classp)
                                             : get_Rand_ಬಹುಲಕ_Var_From_Class(rand_ಬಹುಲಕ_ಗುರಿ.classp);
            ಆದರೆ (!rand_ಬಹುಲಕ_Varp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(ತಳ್ಳು_Deletep(m_stmtp), m_stmtp = ಶೂನ್ಯಸೂಚಕ);
                ಹಿಂತಿರುಗಿ;
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = make_ಬಹುಲಕ_ನಿಯೋಜಿಸು_Lhs(nodep->fileline(), rand_ಬಹುಲಕ_ಗುರಿ.classp,
                                                        rand_ಬಹುಲಕ_ಗುರಿ.fromp, rand_ಬಹುಲಕ_Varp);
            replace_With_ಬಹುಲಕ_ನಿಯೋಜಿಸು_And_Append(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : ಶೂನ್ಯಸೂಚಕ, lhsp,
                classLevelStaticLoopp);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (nodep->name() == "constraint_mode") {
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ ವಿಧಾನ_Callp = VN_CAST(nodep, ವಿಧಾನ_Call);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp = ವಿಧಾನ_Callp ? ವಿಧಾನ_Callp->fromp() : ಶೂನ್ಯಸೂಚಕ;
            AstConstraint* constrp = ಶೂನ್ಯಸೂಚಕ;
            AstClass* classp = VN_CAST(m_modp, Class);
            ಆದರೆ (AstConstraintRef* ಸ್ಥಿರಾಂಕ constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                ಆದರೆ (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ಸ್ಥಿರಾಂಕ constraint_ಬಹುಲಕ_Varp = (constrp && constrp->isStatic())
                                                   ? get_Static_Constraint_ಬಹುಲಕ_Var(classp)
                                                   : get_Constraint_ಬಹುಲಕ_Var(classp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp
                = make_ಬಹುಲಕ_ನಿಯೋಜಿಸು_Lhs(nodep->fileline(), classp, fromp, constraint_ಬಹುಲಕ_Varp);
            replace_With_ಬಹುಲಕ_ನಿಯೋಜಿಸು(nodep, constrp, lhsp);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಒಳ_Static_Context = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ಸ್ಥಿರಾಂಕ stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ಸ್ಥಿರಾಂಕ randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            ಆದರೆ (ಒಳ_Static_Context) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(ನಿಜ);
            }
            randomizeFuncp->ಕೂಡಿಸು_Stmtsp(
                ಹೊಸ Ast_ನಿಯೋಜಿಸು{nodep->fileline(),
                              ಹೊಸ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              ಹೊಸ AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            ಪೂರ್ಣಾಂಕ argn = 0;
            AstWith* ಸ್ಥಿರಾಂಕ withp = nodep->withp();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = argp->exprp();
                AstCMethodHard* ಸ್ಥಿರಾಂಕ basicMethodp = ಹೊಸ AstCMethodHard{
                    nodep->fileline(),
                    ಹೊಸ AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ಸ್ಥಿರಾಂಕ refvarp
                    = ಹೊಸ AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::ಗಮ್ಯ_string(++argn), exprp->dtypep()};
                refvarp->ಗಣ_Std_Randomize_Arg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                ಆದರೆ (withp) {
                    withp->foreach([&](Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಘಾತಾಂಕ) {
                        ಆದರೆ (is_Similar_ಗಂಟು(ಘಾತಾಂಕ, exprp)) {
                            AstVarRef* ಸ್ಥಿರಾಂಕ replaceVar
                                = ಹೊಸ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            ಘಾತಾಂಕ->replaceWith(replaceVar);
                            replaceVar->user1(ಘಾತಾಂಕ->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ಘಾತಾಂಕ), ಘಾತಾಂಕ);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(ನಿಜ);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->ಕೂಡಿಸು_Stmtsp(refvarp);

                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಅಗಲ = exprp->ಅಗಲ();
                basicMethodp->ಕೂಡಿಸು_Pinsp(
                    ಹೊಸ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->ಕೂಡಿಸು_Pinsp(
                    ಹೊಸ AstConst{nodep->fileline(), AstConst::Unsized64{}, ಅಗಲ});
                basicMethodp->dtype_ಗಣ_ಬಿಟ್();

                randomizeFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                    nodep->fileline(),
                    ಹೊಸ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    ಹೊಸ AstAnd{nodep->fileline(),
                               ಹೊಸ AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            ಆದರೆ (withp) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, ಶೂನ್ಯಸೂಚಕ,
                                                                ಒಳ_Static_Context);
                withCapturep->ಕೂಡಿಸು_Function_Arguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                ಆದರೆ (stdrand) {
                    randomizeFuncp->ಕೂಡಿಸು_Stmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ capturedTreep = withp->exprp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                randomizeFuncp->ಕೂಡಿಸು_Stmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor{
                        ಶೂನ್ಯಸೂಚಕ, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        ಶೂನ್ಯಸೂಚಕ, m_writtenVars, &m_uniqueConstraintId, m_unique_ಹೆಸರುಗಳು,  ಶೂನ್ಯಸೂಚಕ};
                }
                AstCExpr* ಸ್ಥಿರಾಂಕ solverCallp = ಹೊಸ AstCExpr{fl};
                solverCallp->dtype_ಗಣ_ಬಿಟ್();
                solverCallp->ಕೂಡಿಸು(ಹೊಸ AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->ಕೂಡಿಸು(".next()");
                AstVar* ಸ್ಥಿರಾಂಕ fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ andExprp
                    = ಹೊಸ AstAnd{fl, ಹೊಸ AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->ಕೂಡಿಸು_Stmtsp(
                    ಹೊಸ Ast_ನಿಯೋಜಿಸು{fl, ಹೊಸ AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            ಆದರೆ (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            ಆದರೆ (withCapturep) nodep->ಕೂಡಿಸು_Argsp(withCapturep->get_ವಾದಗಳು());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            ಹಿಂತಿರುಗಿ;
        }
        handle_Randomize_ವಾದಗಳು(nodep);

        AstWith* ಸ್ಥಿರಾಂಕ withp = nodep->withp();
        ಆದರೆ (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        withp->unlink_Fr_ಹಿಂಭಾಗ();
        iterateChildren(nodep);

        AstClass* classp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ callp = VN_CAST(nodep, ವಿಧಾನ_Call)) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } ಇಲ್ಲದಿದ್ದರೆ {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        ಆದರೆ (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ಸ್ಥಿರಾಂಕ classGenp = getRandomGenerator(classp);
        AstVar* ಸ್ಥಿರಾಂಕ localGenp
            = ಹೊಸ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(ನಿಜ);

        AstFunc* ಸ್ಥಿರಾಂಕ randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inline_Unique_ಹೆಸರುಗಳು.get(nodep), ಸುಳ್ಳು);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ಸ್ಥಿರಾಂಕ std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<ತಾರ್ಕಿಕ, ತಾರ್ಕಿಕ>{ಸುಳ್ಳು, ಸುಳ್ಳು};
        ಆದರೆ (prePostWrap.ಮೊದಲ) {
            ಕೂಡಿಸು_Virtual_Pre_Post_Call(randomizeFuncp, classp, "pre_randomize");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_Pre_Post_Call(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        ಆದರೆ (classHasRandClassMembers(classp)) {
            AstTask* ಸ್ಥಿರಾಂಕ preTaskp = getCreateNestedCallbackTask(classp, "pre");
            ಆದರೆ (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->ಕೂಡಿಸು_Stmtsp((ಹೊಸ AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ಸ್ಥಿರಾಂಕ CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.ಕೂಡಿಸು_Function_Arguments(randomizeFuncp);

        // Add constraints clearing code
        ಆದರೆ (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ಸ್ಥಿರಾಂಕ mainRandomizep
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(classp, "randomize"), Func);
            ಆದರೆ (mainRandomizep && mainRandomizep->stmtsp()) {
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು_Clear_Constraints = ಸುಳ್ಳು;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        ಆದರೆ (methodp->ವಿಧಾನ() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->ವಿಧಾನ() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->ಕೂಡಿಸು_Stmtsp(stmtp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (methodp->ವಿಧಾನ() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            ಸಿಕ್ಕಿತು_Clear_Constraints = ನಿಜ;
                        }
                    });
                    ಆದರೆ (ಸಿಕ್ಕಿತು_Clear_Constraints) ನಿಲ್ಲಿಸು;
                }
            }
            randomizeFuncp->ಕೂಡಿಸು_Stmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->ಕೂಡಿಸು_Stmtsp(localGenp);

        AstFunc* ಸ್ಥಿರಾಂಕ basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ಸ್ಥಿರಾಂಕ basicRandomizeFuncCallp
            = ಹೊಸ AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        ಆದರೆ (classGenp) {
            AstTask* ಸ್ಥಿರಾಂಕ constr_ಸಿದ್ಧತೆ_Funcp = get_Create_Constraint_ಸಿದ್ಧತೆ_Func(classp, m_memberMap);
            AstTaskRef* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstTaskRef{nodep->fileline(), constr_ಸಿದ್ಧತೆ_Funcp};
            randomizeFuncp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
            randomizeFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
                nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                ಹೊಸ AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), ಗಂಟು_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ಸ್ಥಿರಾಂಕ rand_ಬಹುಲಕ_Varp = get_Rand_ಬಹುಲಕ_Var_From_Class(classp);
        ಆದರೆ (!classGenp && rand_ಬಹುಲಕ_Varp) ಕೂಡಿಸು_ಗಣ_Rand_ಬಹುಲಕ(randomizeFuncp, localGenp, rand_ಬಹುಲಕ_Varp);
        ಆದರೆ (!classGenp) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ sVarp = get_Static_Rand_ಬಹುಲಕ_Var(classp)) {
                ಕೂಡಿಸು_ಗಣ_Static_Rand_ಬಹುಲಕ(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ capturedTreep = withp->exprp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        randomizeFuncp->ಕೂಡಿಸು_Stmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_ಗಂಟು* inlineResizeStmtsp = ಶೂನ್ಯಸೂಚಕ;
        {
            std::ಸದಿಶ<AstCMethodHard*> ಗಾತ್ರ_Methodps;
            capturedTreep->foreach_And_ಮುಂದಿನ([&](AstCMethodHard* methodp) {
                ಆದರೆ (methodp->ವಿಧಾನ() == VCMethod::DYN_ಗಾತ್ರ
                    || methodp->ವಿಧಾನ() == VCMethod::ASSOC_ಗಾತ್ರ) {
                    ಗಾತ್ರ_Methodps.ತಳ್ಳು_ಹಿಂಭಾಗ(methodp);
                }
            });
            ಗಾಗಿ (AstCMethodHard* ಸ್ಥಿರಾಂಕ methodp : ಗಾತ್ರ_Methodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = ಶೂನ್ಯಸೂಚಕ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಬೇರು_Exprp = methodp->fromp();
                ಆದರೆ (AstMemberSel* memberSelp = VN_CAST(ಬೇರು_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    ಆಗ (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ fromp = memberSelp->fromp();
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(fromp, VarRef)) {
                        ಆದರೆ (varRefp->varp() == captured.getThisp()) ಮುಂದುವರಿ;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(ಬೇರು_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                ಆದರೆ (!arrVarp) ಮುಂದುವರಿ;
                // Only handle rand-declared dynamic/assoc array variables
                ಆದರೆ (!arrVarp->rand().isRandomizable()) ಮುಂದುವರಿ;
                // Do not randomize associative array size (IEEE 18.4)
                ಆದರೆ (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) ಮುಂದುವರಿ;
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = methodp->fileline();
                ತಾರ್ಕಿಕ wasCreated = ಸುಳ್ಳು;
                AstClass* ಸ್ಥಿರಾಂಕ varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Varp = create_Or_Get_ಗಾತ್ರ_Var(varClassp, arrVarp, fl,
                                                            methodp->ಕಂಡುಹಿಡಿ_Int_DType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                ಆದರೆ (wasCreated) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ resizeVarRefp = buildMemberSelExprp(ಬೇರು_Exprp, arrVarp);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Exprp = buildMemberSelExprp(ಬೇರು_Exprp, ಗಾತ್ರ_Varp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->ಕೂಡಿಸು_ಮುಂದಿನ(create_ಗಾತ್ರ_Gte_Zero_Constraint(fl, ಗಾತ್ರ_Varp, ಬೇರು_Exprp));
                    AstCMethodHard* ಸ್ಥಿರಾಂಕ resizep
                        = ಹೊಸ AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, ಗಾತ್ರ_Exprp};
                    resizep->dtypep(methodp->ಕಂಡುಹಿಡಿ_Void_DType());
                    inlineResizeStmtsp
                        = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(inlineResizeStmtsp, ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, resizep});
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_ಗಂಟು_Exprp = buildMemberSelExprp(ಬೇರು_Exprp, ಗಾತ್ರ_Varp);
                methodp->replaceWith(ಗಾತ್ರ_ಗಂಟು_Exprp);
                VL_DO_DANGLING(methodp->delete_ವೃಕ್ಷ(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_ಅಭಿವ್ಯಕ್ತಿ_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                rand_ಬಹುಲಕ_Varp, m_writtenVars, &m_uniqueConstraintId, m_unique_ಹೆಸರುಗಳು,  ಶೂನ್ಯಸೂಚಕ};
        }

        // Call the solver and set return value
        AstCExpr* ಸ್ಥಿರಾಂಕ solverCallp = ಹೊಸ AstCExpr{nodep->fileline()};
        solverCallp->dtype_ಗಣ_ಬಿಟ್();
        solverCallp->ಕೂಡಿಸು(ಹೊಸ AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->ಕೂಡಿಸು(".next(__Vm_rng)");
        randomizeFuncp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(),
            ಹೊಸ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            ಹೊಸ AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        ಆದರೆ (inlineResizeStmtsp) {
            AstVar* ಸ್ಥಿರಾಂಕ retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{nodep->fileline(),
                                         ಹೊಸ AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, ಶೂನ್ಯಸೂಚಕ};
            randomizeFuncp->ಕೂಡಿಸು_Stmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        ಆದರೆ (classHasRandClassMembers(classp)) {
            AstTask* ಸ್ಥಿರಾಂಕ postTaskp = getCreateNestedCallbackTask(classp, "post");
            ಆದರೆ (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->ಕೂಡಿಸು_Stmtsp((ಹೊಸ AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        ಆದರೆ (prePostWrap.ಎರಡನೇ) {
            ಕೂಡಿಸು_Virtual_Pre_Post_Call(randomizeFuncp, classp, "post_randomize");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_Pre_Post_Call(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->ಕೂಡಿಸು_Argsp(captured.get_ವಾದಗಳು());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_ವೃಕ್ಷ(), withp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        ಆದರೆ (m_constraintp && nodep->fromp()->user1()
            && (nodep->ವಿಧಾನ() == VCMethod::ASSOC_ಗಾತ್ರ
                || nodep->ವಿಧಾನ() == VCMethod::DYN_ಗಾತ್ರ)) {
            AstClass* ಸ್ಥಿರಾಂಕ classp = VN_AS(m_modp, Class);
            AstVarRef* ಸ್ಥಿರಾಂಕ ಸರತಿ_Var_Refp = VN_CAST(nodep->fromp(), VarRef);
            ಆದರೆ (!ಸರತಿ_Var_Refp) {
                // Warning from ConstraintExprVisitor will be thrown
                ಹಿಂತಿರುಗಿ;
            }
            AstVar* ಸ್ಥಿರಾಂಕ ಸರತಿ_Varp = ಸರತಿ_Var_Refp->varp();
            ತಾರ್ಕಿಕ wasCreated = ಸುಳ್ಳು;
            AstVar* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Varp
                = create_Or_Get_ಗಾತ್ರ_Var(classp, ಸರತಿ_Varp, fl, nodep->ಕಂಡುಹಿಡಿ_Int_DType(), wasCreated);
            ಆದರೆ (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                ಆದರೆ (!VN_IS(ಸರತಿ_Varp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    ಆದರೆ (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Var_Refp = ಹೊಸ AstVarRef{
                        fl, VN_AS(ಗಾತ್ರ_Varp->user2p(), ಗಂಟು_Module), ಗಾತ್ರ_Varp, VAccess::READ};
                    AstCMethodHard* ಸ್ಥಿರಾಂಕ resizep = ಹೊಸ AstCMethodHard{
                        fl, nodep->fromp()->unlink_Fr_ಹಿಂಭಾಗ(), VCMethod::DYN_RESIZE, ಗಾತ್ರ_Var_Refp};
                    resizep->dtypep(nodep->ಕಂಡುಹಿಡಿ_Void_DType());
                    resizerTaskp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->ಕೂಡಿಸು_Itemsp(create_ಗಾತ್ರ_Gte_Zero_Constraint(fl, ಗಾತ್ರ_Varp));
            }
            AstVarRef* ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_Var_Refp = ಹೊಸ AstVarRef{fl, VN_AS(ಗಾತ್ರ_Varp->user2p(), ಗಂಟು_Module),
                                                         ಗಾತ್ರ_Varp, VAccess::READ};
            ಗಾತ್ರ_Var_Refp->user1(ನಿಜ);
            nodep->replaceWith(ಗಾತ್ರ_Var_Refp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ RandomizeVisitor(AstNetlist* nodep)
        : m_inline_Unique_ಹೆಸರುಗಳು{"__Vrandwith"} {
        createRandomizeClassVars(nodep);
        // Flag local constraint leaves as solver-owned so __VBasicRand skips them.
        // Runs before any class is lowered. Only a randomized class counts, and
        // only a leaf owned by the constraint's own class: a leaf reached through
        // a global constraint stays basic-randomized so a standalone randomize()
        // of its type still randomizes it (issue #7833); a class-typed sub-object
        // is skipped so its own members are still basic-randomized.
        nodep->foreach([&](AstClass* ಸ್ಥಿರಾಂಕ classp) {
            ಆದರೆ (!classp->user1()) ಹಿಂತಿರುಗಿ;
            classp->foreachMember([&](AstClass* ಸ್ಥಿರಾಂಕ ownerClassp, AstConstraint* ಸ್ಥಿರಾಂಕ constrp) {
                constrp->foreach([&](AstMemberSel* ಸ್ಥಿರಾಂಕ memberSelp) {
                    AstVar* ಸ್ಥಿರಾಂಕ varp = memberSelp->varp();
                    ಆದರೆ (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) ಹಿಂತಿರುಗಿ;
                    ಆದರೆ (VN_AS(varp->user2p(), ಗಂಟು_Module) != ownerClassp) ಹಿಂತಿರುಗಿ;
                    ಆದರೆ (!varp->user3()) varp->user3(ನಿಜ);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(constrp->unlink_Fr_ಹಿಂಭಾಗ()), constrp);
        });
    }
    ~RandomizeVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Randomize method class functions

ಶೂನ್ಯ V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        ಸ್ಥಿರಾಂಕ RandomizeMarkVisitor markVisitor{nodep};
        ಸ್ಥಿರಾಂಕ RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("randomize", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       ಸ್ಥಿರಾಂಕ std::string& name, ತಾರ್ಕಿಕ allowVirtual,
                                       ತಾರ್ಕಿಕ ಮಗು_DType) {
    AstFunc* funcp = VN_AS(memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, name), Func);
    ಆದರೆ (!funcp) {
        v3Global.useRandomizeMethods(ನಿಜ);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep
            = ಮಗು_DType
                  ? ಹೊಸ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ಸ್ಥಿರಾಂಕ fvarp = ಮಗು_DType
                                  ? ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlag_ಮಗು_DType{}, dtypep}
                                  : ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3_Error_ಸಂಕೇತ::NORETURN, ನಿಜ);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(ನಿಜ);
        fvarp->funcReturn(ನಿಜ);
        fvarp->direction(VDirection::OUTPUT);
        nodep->ಕೂಡಿಸು_Membersp(funcp);
        funcp = ಹೊಸ AstFunc{nodep->fileline(), name, ಶೂನ್ಯಸೂಚಕ, fvarp};
        ಆದರೆ (!ಮಗು_DType) funcp->dtypep(dtypep);
        funcp->class_ವಿಧಾನ(ನಿಜ);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->ಕೂಡಿಸು_Membersp(funcp);
        memberMap.ಸೇರಿಸು(nodep, funcp);
    }
    ಹಿಂತಿರುಗಿ funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_ಗಂಟು_Module* nodep, ಸ್ಥಿರಾಂಕ std::string& name) {
    AstFunc* funcp = ಶೂನ್ಯಸೂಚಕ;
    v3Global.useRandomizeMethods(ನಿಜ);
    Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(32, 32, VSigning::SIGNED);
    AstVar* ಸ್ಥಿರಾಂಕ fvarp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(ನಿಜ);
    fvarp->funcReturn(ನಿಜ);
    fvarp->direction(VDirection::OUTPUT);
    funcp = ಹೊಸ AstFunc{nodep->fileline(), name, ಶೂನ್ಯಸೂಚಕ, fvarp};
    funcp->dtypep(dtypep);
    ಆದರೆ (VN_IS(nodep, Class)) {
        funcp->class_ವಿಧಾನ(ನಿಜ);
    } ಇಲ್ಲದಿದ್ದರೆ {
        funcp->class_ವಿಧಾನ(ನಿಜ);
        funcp->isStatic(ನಿಜ);
    }
    nodep->ಕೂಡಿಸು_Stmtsp(funcp);
    ಹಿಂತಿರುಗಿ funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ಸ್ಥಿರಾಂಕ basep = nodep->ತಳ_Most_Classp();
    AstFunc* funcp = VN_AS(memberMap.ಕಂಡುಹಿಡಿ_Member(basep, "srandom"), Func);
    ಆದರೆ (!funcp) {
        v3Global.useRandomizeMethods(ನಿಜ);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep
            = basep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ಸ್ಥಿರಾಂಕ ivarp = ಹೊಸ AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(ನಿಜ);
        ivarp->direction(VDirection::ಒಳಹರಿವು);
        funcp = ಹೊಸ AstFunc{basep->fileline(), "srandom", ivarp, ಶೂನ್ಯಸೂಚಕ};
        funcp->dtypep(basep->ಕಂಡುಹಿಡಿ_Void_DType());
        funcp->class_ವಿಧಾನ(ನಿಜ);
        funcp->isVirtual(ಸುಳ್ಳು);
        basep->ಕೂಡಿಸು_Membersp(funcp);
        memberMap.ಸೇರಿಸು(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        ಆದರೆ (basep->name() == "process") {
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } ಇಲ್ಲದಿದ್ದರೆ {
            funcp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(ನಿಜ);
        }
    }
    ಹಿಂತಿರುಗಿ funcp;
}
