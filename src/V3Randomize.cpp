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
स्थिर AstVar* get_Rand_बहुलकम्_Var_From_Class(Ast_ग्रन्थिः_Module* classp) {
    यावत् (classp) {
        यदि (classp->user2p()) प्रत्यागम VN_AS(classp->user2p(), Var);
        AstClass* ध्रुव cp = VN_CAST(classp, Class);
        यदि (!cp || !cp->extendsp()) प्रत्यागम शून्यसूचकः;
        classp = cp->extendsp()->classp();
    }
    प्रत्यागम शून्यसूचकः;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
स्थिर AstVar* getRandomGenerator(AstClass* ध्रुव classp) {
    यदि (classp->user3p()) प्रत्यागम VN_AS(classp->user3p(), Var);
    यदि (classp->extendsp()) प्रत्यागम getRandomGenerator(classp->extendsp()->classp());
    प्रत्यागम शून्यसूचकः;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
स्थिर AstTask* get_Create_Constraint_सज्जता_Func(AstClass* classp, VMemberMap& memberMap) {
    स्थिर ध्रुव वर्ण* ध्रुव name = "__Vsetup_constraints";
    AstTask* सज्जता_All_Taskp = VN_AS(memberMap.findMember(classp, name), Task);
    यदि (सज्जता_All_Taskp) प्रत्यागम सज्जता_All_Taskp;
    सज्जता_All_Taskp = नूतन AstTask{classp->fileline(), "__Vsetup_constraints", शून्यसूचकः};
    सज्जता_All_Taskp->class_विधानम्(सत्य);
    सज्जता_All_Taskp->isVirtual(सत्य);
    classp->सङ्कलय_Membersp(सज्जता_All_Taskp);
    memberMap.निवेशय(classp, सज्जता_All_Taskp);
    प्रत्यागम सज्जता_All_Taskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

संरचना Rand_बहुलकम्_Target final {
    // MEMBERS
    AstVar* ध्रुव receiverp;  // Variable that is the target of the rand_mode() call
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp;  // Expression from which the rand_mode() call originates
    AstClass* ध्रुव classp;  // Class in which rand_mode should be set

    // METHODS
    स्थिर Rand_बहुलकम्_Target get(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_Module* modp) {
        यदि (!fromp) प्रत्यागम {शून्यसूचकः, शून्यसूचकः, VN_AS(modp, Class)};
        यदि (AstCMethodHard* ध्रुव विधानम्_Hardp = VN_CAST(fromp, CMethodHard)) {
            प्रत्यागम Rand_बहुलकम्_Target::get(विधानम्_Hardp->fromp(), modp);
        }
        AstVar* receiverp = शून्यसूचकः;
        AstClass* classp = VN_CAST(modp, Class);
        यदि (AstVarRef* ध्रुव varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            यदि (receiverp->isRand()) {
                fromp = शून्यसूचकः;
                यदि (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } अन्यथा यदि (AstMemberSel* ध्रुव memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            यदि (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        यदि (!receiverp->isRand()) {
            AstClassRefDType* ध्रुव classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            यदि (classRefDtp) classp = classRefDtp->classp();
        }
        प्रत्यागम {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

सङ्घ Randomize_बहुलकम् final {
    // MEMBERS
    संरचना {
        तार्किक उपयुङ्क्ते_बहुलकम् : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t सूचकांक : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
स्थिर Ast_ग्रन्थिः_DType* arrayElementDTypep(Ast_ग्रन्थिः_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    यावत् (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    प्रत्यागम dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
स्थिर तार्किक dtype_अन्तर्भवति_Real(Ast_ग्रन्थिः_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    यदि (Ast_ग्रन्थिः_DType* ध्रुव subp = dtypep->subDTypep()) प्रत्यागम dtype_अन्तर्भवति_Real(subp);
    यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव structp = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
        यदि (structp->packed()) प्रत्यागम असत्य;
        प्रति (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            यदि (dtype_अन्तर्भवति_Real(memberp->subDTypep())) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(dtypep, BasicDType);
    प्रत्यागम basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
स्थिर शून्य checkRandTypeEligibility(Ast_ग्रन्थिः* contextp, Ast_ग्रन्थिः_DType* dtypep, तार्किक isRandc) {
    dtypep = dtypep->skipRefp();
    यदि (Ast_ग्रन्थिः_DType* ध्रुव subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        प्रत्यागम;
    }
    यदि (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        यदि (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        प्रत्यागम;
    }
    यदि (ध्रुव AstIfaceRefDType* ध्रुव ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        प्रत्यागम;
    }
    यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव structp = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
        यदि (structp->packed()) प्रत्यागम;  // Integral by construction; members checked separately
        यदि (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            प्रत्यागम;
        }
        प्रति (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            यदि (memberp->यादृच्छिकम्().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->यादृच्छिकम्().isRandC());
            }
        }
        प्रत्यागम;
    }
    ध्रुव AstBasicDType* ध्रुव basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    यदि (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        यदि (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        प्रत्यागम;
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

वर्ग RandomizeMarkVisitor final : सार्वजनिक VNVisitor {
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
    ध्रुव VNUser1_अन्तर्_Use m_inuser1;
    ध्रुव VNUser2_अन्तर्_Use m_inuser2;

    उपयुज्य Derived_समुच्चयः = std::unordered_समुच्चयः<AstClass*>;
    उपयुज्य आधारः_गन्तव्यम्_Derived_Map = std::unordered_map<ध्रुव AstClass*, Derived_समुच्चयः>;

    आधारः_गन्तव्यम्_Derived_Map m_आधारः_गन्तव्यम्_Derived_Map;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = शून्यसूचकः;  // Current class
    Ast_ग्रन्थिः* m_constraint_व्यञ्जकम्_Genp = शून्यसूचकः;  // Current constraint or constraint if expression
    Ast_ग्रन्थिः_Module* m_modp;  // Current module
    Ast_ग्रन्थिः_Stmt* m_stmtp = शून्यसूचकः;  // Current statement
    Ast_ग्रन्थिः_FTask_Ref* m_stdRandCallp = शून्यसूचकः;  // Current std::randomize() call
    तार्किक m_अन्तर्_Std_With = असत्य;  // True when inside a 'with {}' clause
    std::समुच्चयः<Ast_ग्रन्थिः_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = शून्यसूचकः;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    शून्य markGlobalConstrainedVars(AstClass* classp) {
        प्रति (ध्रुव AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : शून्यसूचकः) {
            प्रति (Ast_ग्रन्थिः* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ध्रुव varp = VN_CAST(memberp, Var);
                यदि (!varp) अनुवर्त;
                यदि (varp->यादृच्छिकम्().isRandomizable()) varp->globalConstrained(सत्य);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    तार्किक is_Var_अन्तर्_Std_Randomize_तर्काः(ध्रुव AstVar* varp) ध्रुव {
        यदि (!m_अन्तर्_Std_With || !m_stdRandCallp) प्रत्यागम असत्य;

        प्रति (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            यावत् (exprp) {
                यदि (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(exprp, ग्रन्थिः_Var_Ref)) {
                    यदि (varrefp->varp() == varp) प्रत्यागम सत्य;
                    भङ्ग;
                }
                यदि (ध्रुव AstMemberSel* ध्रुव memberselp = VN_CAST(exprp, MemberSel)) {
                    यदि (memberselp->varp() == varp) प्रत्यागम सत्य;
                    exprp = memberselp->fromp();
                } अन्यथा यदि (ध्रुव AstArraySel* ध्रुव arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } अन्यथा यदि (ध्रुव AstStructSel* ध्रुव strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } अन्यथा यदि (ध्रुव AstCMethodHard* ध्रुव methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } अन्यथा {
                    भङ्ग;
                }
            }
        }
        प्रत्यागम असत्य;
    }
    शून्य markMembers(ध्रुव AstClass* nodep) {
        प्रति (ध्रुव AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : शून्यसूचकः) {
            प्रति (Ast_ग्रन्थिः* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* ध्रुव varp = VN_CAST(memberp, Var);
                यदि (!varp) अनुवर्त;
                // If member is randomizable and of class type, mark its class
                यदि (varp->यादृच्छिकम्().isRandomizable()) {
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव varDtypep = varp->dtypep()->skipRefp();
                    ध्रुव AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    यदि (!classRefp) {
                        ध्रुव Ast_ग्रन्थिः_DType* subDTypep = varDtypep->subDTypep();
                        यदि (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    यदि (classRefp) {
                        AstClass* ध्रुव rclassp = classRefp->classp();
                        यदि (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    यदि (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        Randomize_बहुलकम् rand_बहुलकम् = {};
                        rand_बहुलकम्.उपयुङ्क्ते_बहुलकम् = सत्य;
                        varp->user1(rand_बहुलकम्.asUQuad);
                    }
                }
            }
        }
    }
    शून्य markDerived(ध्रुव AstClass* nodep) {
        ध्रुव स्वचालित it = m_आधारः_गन्तव्यम्_Derived_Map.find(nodep);
        यदि (it != m_आधारः_गन्तव्यम्_Derived_Map.अन्तः_2()) {
            प्रति (स्वचालित* classp : it->second) {
                यदि (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    शून्य markAllDerived() {
        प्रति (ध्रुव स्वचालित& p : m_आधारः_गन्तव्यम्_Derived_Map) {
            यदि (p.first->user1()) markDerived(p.first);
        }
    }
    शून्य समुच्चयः_Package_Refs() {
        प्रति (Ast_ग्रन्थिः_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), ग्रन्थिः_Module));
        }
    }
    शून्य markNestedGlobalConstrainedRecurse(Ast_ग्रन्थिः* nodep) {
        यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            AstVar* ध्रुव varp = refp->varp();
            यदि (varp->globalConstrained()) प्रत्यागम;
            varp->globalConstrained(सत्य);
        } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव memberSelp = VN_CAST(nodep, MemberSel)) {
            यदि (memberSelp->varp()) {
                AstVar* ध्रुव varp = memberSelp->varp();
                यदि (varp->globalConstrained()) प्रत्यागम;
                varp->globalConstrained(सत्य);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_ग्रन्थिः_व्यञ्जकम्* buildMemberSelChain(AstVarRef* मूलम्_Var_Refp, ध्रुव std::सदिशम्<AstVar*>& मार्गः) {
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = मूलम्_Var_Refp->clone_वृक्षः(असत्य);
        प्रति (AstVar* memberVarp : मार्गः) {
            AstMemberSel* memberSelp
                = नूतन AstMemberSel{मूलम्_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        प्रत्यागम exprp;
    }

    // Process a single constraint during nested constraint cloning
    शून्य processNestedConstraint(AstConstraint* ध्रुव constrp, AstVarRef* मूलम्_Var_Refp,
                                 ध्रुव std::सदिशम्<AstVar*>& new_मार्गः, AstClass* targetClassp) {
        std::string मार्गः_उपसर्गः = मूलम्_Var_Refp->name();
        प्रति (AstVar* मार्गः_Member_Varp : new_मार्गः) {
            मार्गः_उपसर्गः += GLOBAL_CONSTRAINT_SEPARATOR + मार्गः_Member_Varp->name();
        }

        ध्रुव std::string newName = मार्गः_उपसर्गः + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        तार्किक isDuplicate = असत्य;
        targetClassp->foreachMember([&](AstClass* ध्रुव, AstConstraint* ध्रुव existingConstrp) {
            यदि (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = सत्य;
            }
        });
        यदि (isDuplicate) प्रत्यागम;

        AstConstraint* ध्रुव cloneConstrp = constrp->clone_वृक्षः(असत्य);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::सदिशम्<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
            यदि (AstVarRef* ध्रुव varRefp = VN_CAST(exprp, VarRef)) {
                यदि (!varRefp->varp()->isClassMember()) प्रत्यागम;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव chainp = buildMemberSelChain(मूलम्_Var_Refp, new_मार्गः);
                AstMemberSel* ध्रुव finalSelp
                    = नूतन AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(varRefp), varRefp);
            } अन्यथा यदि (AstFuncRef* ध्रुव funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                यदि (funcRefp->taskp()->class_विधानम्() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.प्रक्षिप_पृष्ठभागः(funcRefp);
                }
            }
        });
        प्रति (AstFuncRef* funcRefp : funcRefps) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव chainp = buildMemberSelChain(मूलम्_Var_Refp, new_मार्गः);
            AstArg* ध्रुव argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlink_Fr_पृष्ठभागः_With_अग्र() : शून्यसूचकः;
            Ast_विधानम्_Call* ध्रुव callp
                = नूतन Ast_विधानम्_Call{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        targetClassp->सङ्कलय_Stmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    शून्य cloneNestedConstraintsRecurse(AstVarRef* मूलम्_Var_Refp, AstClass* classp,
                                       ध्रुव std::सदिशम्<AstVar*>& मार्गः_गन्तव्यम्_Class,
                                       AstClass* targetClassp) {
        प्रति (Ast_ग्रन्थिः* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* ध्रुव memberVarp = VN_CAST(memberNodep, Var);
            यदि (!memberVarp) अनुवर्त;
            यदि (!memberVarp->यादृच्छिकम्().isRandomizable()) अनुवर्त;
            ध्रुव AstClassRefDType* ध्रुव memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            यदि (!memberClassRefp || !memberClassRefp->classp()) अनुवर्त;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::सदिशम्<AstVar*> new_मार्गः = मार्गः_गन्तव्यम्_Class;
            new_मार्गः.प्रक्षिप_पृष्ठभागः(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* ध्रुव containingClassp, AstConstraint* ध्रुव constrp) {
                    processNestedConstraint(constrp, मूलम्_Var_Refp, new_मार्गः, targetClassp);
                });
            cloneNestedConstraintsRecurse(मूलम्_Var_Refp, nestedClassp, new_मार्गः, targetClassp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    शून्य handle_Randomize_तर्कः(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar* ध्रुव varp, ध्रुव तार्किक stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        यावत् (exprp) {
            AstVar* randVarp = शून्यसूचकः;
            यदि (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } अन्यथा यदि (AstArraySel* ध्रुव arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                अनुवर्त;  // Skip ArraySel, continue traversing
            } अन्यथा यदि (AstStructSel* ध्रुव structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                अनुवर्त;  // Skip StructSel, continue traversing
            } अन्यथा यदि (AstCMethodHard* ध्रुव methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                अनुवर्त;
            } अन्यथा यदि (AstVarRef* ध्रुव varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(सत्य);
                varrefp->access(VAccess::READWRITE);
                exprp = शून्यसूचकः;
            } अन्यथा {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            यदि (randVarp == varp) प्रत्यागम;
            Ast_ग्रन्थिः* backp = randVarp;
            यावत् (backp && !VN_IS(backp, ग्रन्थिः_Module)) backp = backp->backp();
            यदि (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } अन्यथा {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                Randomize_बहुलकम् rand_बहुलकम् = {};
                rand_बहुलकम्.उपयुङ्क्ते_बहुलकम् = सत्य;
                randVarp->user1(rand_बहुलकम्.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    शून्य दर्शय(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        यदि (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            ध्रुव AstClass* ध्रुव basep = nodep->extendsp()->classp();
            m_आधारः_गन्तव्यम्_Derived_Map[basep].निवेशय(nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        यदि (!nodep->backp()) VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        यदि (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        यदि (nodep->name() == "rand_mode") {
            Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(nodep, विधानम्_Call);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = विधानम्_Callp ? विधानम्_Callp->fromp() : शून्यसूचकः;
            तार्किक valid = सत्य;
            यदि (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = असत्य;
            } अन्यथा यदि (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = असत्य;
            } अन्यथा यदि (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = असत्य;
            } अन्यथा यदि (AstCMethodHard* ध्रुव विधानम्_Hardp = VN_CAST(fromp, CMethodHard)) {
                यदि (विधानम्_Hardp->विधानम्() == VCMethod::ARRAY_AT
                    || विधानम्_Hardp->विधानम्() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = असत्य;
                } अन्यथा {
                    विधानम्_Hardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            यदि (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)
                && !nodep->backp()->fileline()->warnIsOff(V3_Error_संहिता::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = असत्य;
            }
            यदि (valid) {
                ध्रुव Rand_बहुलकम्_Target rand_बहुलकम्_Target = Rand_बहुलकम्_Target::get(fromp, m_classp);
                यदि ((!rand_बहुलकम्_Target.receiverp || !rand_बहुलकम्_Target.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = असत्य;
                } अन्यथा यदि (!rand_बहुलकम्_Target.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = असत्य;
                } अन्यथा यदि (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = असत्य;
                } अन्यथा यदि (rand_बहुलकम्_Target.receiverp && rand_बहुलकम्_Target.receiverp->isRand()) {
                    // Called on a rand member variable
                    Randomize_बहुलकम् rand_बहुलकम् = {};
                    rand_बहुलकम्.उपयुङ्क्ते_बहुलकम् = सत्य;
                    rand_बहुलकम्_Target.receiverp->user1(rand_बहुलकम्.asUQuad);
                } अन्यथा {
                    // Called on 'this' or a non-rand class instance
                    rand_बहुलकम्_Target.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        यदि (!varp->isRand()) प्रत्यागम;
                        Randomize_बहुलकम् rand_बहुलकम् = {};
                        rand_बहुलकम्.उपयुङ्क्ते_बहुलकम् = सत्य;
                        varp->user1(rand_बहुलकम्.asUQuad);
                    });
                }
            }
            यदि (!valid) {
                यदि (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                    nodep->replaceWith(नूतन AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                } अन्यथा {
                    m_stmtp->unlink_Fr_पृष्ठभागः();
                }
            }
            प्रत्यागम;
        }

        यदि (nodep->name() == "constraint_mode") {
            तार्किक valid = सत्य;
            यदि (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = असत्य;
            } अन्यथा यदि (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)
                       && !nodep->backp()->fileline()->warnIsOff(V3_Error_संहिता::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = असत्य;
            }
            AstConstraint* constrp = शून्यसूचकः;
            AstClass* classp = m_classp;
            यदि (ध्रुव Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(nodep, विधानम्_Call)) {
                यदि (ध्रुव AstConstraintRef* ध्रुव constrRefp
                    = VN_CAST(विधानम्_Callp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    यदि (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } अन्यथा यदि (AstClassRefDType* classRefDtp
                           = VN_CAST(विधानम्_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } अन्यथा {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = असत्य;
                }
            }
            यदि (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = असत्य;
            }
            यदि (valid) {
                Randomize_बहुलकम् constraint_बहुलकम् = {};
                constraint_बहुलकम्.उपयुङ्क्ते_बहुलकम् = सत्य;
                यदि (constrp) {
                    constrp->user1(constraint_बहुलकम्.asUQuad);
                } अन्यथा {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraint_बहुलकम्.asUQuad);
                    });
                }
            } अन्यथा {
                यदि (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                    nodep->replaceWith(नूतन AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                } अन्यथा {
                    m_stmtp->unlink_Fr_पृष्ठभागः();
                }
            }
            प्रत्यागम;
        }

        यदि (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            यदि (m_constraint_व्यञ्जकम्_Genp || m_अन्तर्_Std_With) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                प्रति (Ast_ग्रन्थिः* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    यदि (argp->user1()) {
                        nodep->user1(सत्य);
                        भङ्ग;
                    }
                }
            }
            प्रत्यागम;
        }
        AstClass* classp = m_classp;
        यदि (ध्रुव Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(nodep, विधानम्_Call)) {
            यदि (ध्रुव AstClassRefDType* ध्रुव classRefp
                = VN_CAST(विधानम्_Callp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        यदि (classp) {
            यदि (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* ध्रुव, AstVar* ध्रुव memberVarp) {
                यदि (!memberVarp->यादृच्छिकम्().isRandomizable()) प्रत्यागम;
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = memberVarp->dtypep()->skipRefp();
                ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
                यदि (!classRefp || !classRefp->classp()) प्रत्यागम;
                AstClass* ध्रुव memberClassp = classRefp->classp();
                यदि (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) प्रत्यागम;
                memberVarp->globalConstrained(सत्य);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* मूलम्_Var_Refp
                    = नूतन AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::सदिशम्<AstVar*> रिक्तानि_मार्गः;
                memberClassp->foreachMember([&](AstClass* ध्रुव, AstConstraint* ध्रुव constrp) {
                    processNestedConstraint(constrp, मूलम्_Var_Refp, रिक्तानि_मार्गः, classp);
                });
                cloneNestedConstraintsRecurse(मूलम्_Var_Refp, memberClassp, रिक्तानि_मार्गः, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(मूलम्_Var_Refp->delete_वृक्षः(), मूलम्_Var_Refp);
            });
        }
        यदि (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = शून्यसूचकः;
            प्रति (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handle_Randomize_तर्कः(argp->exprp(), शून्यसूचकः, सत्य);
            }
            प्रत्यागम;
        }
        प्रति (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->अङ्कः().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                ध्रुव तार्किक hasUnsupportedMember
                    = classp->existsMember([](ध्रुव AstClass*, ध्रुव AstVar* memberVarp) {
                          यदि (!memberVarp->यादृच्छिकम्().isRandomizable()) प्रत्यागम असत्य;
                          ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtp = memberVarp->dtypep()->skipRefp();
                          प्रत्यागम VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, QueueDType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                यदि (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                अनुवर्त;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = शून्यसूचकः;  // If nodep is a method call, this is its receiver
            यदि (Ast_विधानम्_Call* विधानम्_Callp = VN_CAST(nodep, विधानम्_Call)) {
                यदि (AstMemberSel* ध्रुव memberSelp = VN_CAST(विधानम्_Callp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } अन्यथा {
                    AstVarRef* ध्रुव varrefp = VN_AS(विधानम्_Callp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handle_Randomize_तर्कः(argp->exprp(), fromVarp, असत्य);
        }
        // Re-mark members after the class became inline-randomized above.
        यदि (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    शून्य दर्शय(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_व्यञ्जकम्_Genp);
        m_stmtp = nodep;
        m_constraint_व्यञ्जकम्_Genp = nodep;
        iterateChildren(nodep);
        यदि (!nodep->backp()) VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(Ast_Constraint_व्यञ्जकम्* nodep) override {
        VL_RESTORER(m_constraint_व्यञ्जकम्_Genp);
        m_constraint_व्यञ्जकम्_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_व्यञ्जकम्_Genp);
            m_constraint_व्यञ्जकम्_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterate_And_अग्र_Const_Null(nodep->thensp());
        iterate_And_अग्र_Const_Null(nodep->elsesp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (!m_constraint_व्यञ्जकम्_Genp) प्रत्यागम;

        यदि (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        यदि (nodep->varp()->यादृच्छिकम्().isRandomizable() && !(m_अन्तर्_Std_With && m_stdRandCallp)) {
            nodep->user1(सत्य);
        } अन्यथा यदि (is_Var_अन्तर्_Std_Randomize_तर्काः(nodep->varp())) {
            nodep->user1(सत्य);
        }
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        यदि (!m_constraint_व्यञ्जकम्_Genp) प्रत्यागम;
        iterateChildrenConst(nodep);
        ध्रुव तार्किक randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        ध्रुव तार्किक randMember = nodep->varp()->यादृच्छिकम्().isRandomizable();
        ध्रुव तार्किक अन्तर्_Std_With = m_अन्तर्_Std_With && m_stdRandCallp;
        यदि (randObject && randMember && !अन्तर्_Std_With) {
            nodep->user1(सत्य);
        } अन्यथा यदि (अन्तर्_Std_With && is_Var_अन्तर्_Std_Randomize_तर्काः(nodep->varp())) {
            nodep->user1(सत्य);
            // Mark parent object for constraint expression visitor
            यदि (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(सत्य);
        }

        यदि (m_withp) {
            Ast_ग्रन्थिः* backp = m_withp;
            यावत् (backp->backp()) {
                यदि (ध्रुव Ast_विधानम्_Call* ध्रुव callp = VN_CAST(backp, विधानम्_Call)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    भङ्ग;
                }
                backp = backp->backp();
            }
        } अन्यथा {
            nodep->user2p(m_modp);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstVar* nodep) override {
        nodep->user2p(m_modp);
        यदि (nodep->यादृच्छिकम्().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->यादृच्छिकम्().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    शून्य दर्शय(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        यदि (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_अन्तर्_Std_With = सत्य;
        iterateChildrenConst(nodep);
        m_अन्तर्_Std_With = असत्य;
    }

    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        iterateChildrenConst(nodep);
        यदि (!m_constraint_व्यञ्जकम्_Genp && !m_अन्तर्_Std_With) प्रत्यागम;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    शून्य दर्शय(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        यदि (!m_constraint_व्यञ्जकम्_Genp && !m_अन्तर्_Std_With) प्रत्यागम;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट RandomizeMarkVisitor(Ast_ग्रन्थिः* nodep) {
        iterateConst(nodep);
        markAllDerived();
        समुच्चयः_Package_Refs();
    }
    ~RandomizeMarkVisitor() override = स्वभाव;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

वर्ग Constraint_व्यञ्जकम्_Visitor final : सार्वजनिक VNVisitor {
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
    AstVar* m_rand_बहुलकम्_Varp;  // Relevant randmode state variable
    तार्किक m_wantSingle = असत्य;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    तार्किक m_structSel = असत्य;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    तार्किक m_defer_Rand_बहुलकम्_Hoist = असत्य;  // Wait for the complete struct member path
    std::समुच्चयः<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::समुच्चयः<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertion_समुच्चयः<AstVar*>* m_आकार_Constrained_Arraysp = शून्यसूचकः;  // Arrays with size constraints
    Ast_ग्रन्थिः_व्यञ्जकम्* m_conditionp = शून्यसूचकः;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_ग्रन्थिः_FTask* m_prepareConstrainedArraysp = शून्यसूचकः;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = शून्यसूचकः;  // Current ID of unique call
    V3_Unique_नामानि& m_unique_नामानि;  // Unique names of temporaries, and of blocks holding them
    std::सदिशम्<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_ग्रन्थिः* m_firstExpressionInsideIndexp = शून्यसूचकः;
    AstConstraintForeach* m_foreachp = शून्यसूचकः;  // Innermost constraint-foreach being processed
    std::समुच्चयः<ध्रुव AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    वर्ग Nested_Access_मार्गः final {
        AstMemberSel* m_शिखरम्_Nested_Array_Member_Selp
            = शून्यसूचकः;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = शून्यसूचकः;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = शून्यसूचकः;  // Last node of variable's name format
        Ast_ग्रन्थिः** m_first_Expression_Inside_सूचकांक_Pointerp
            = शून्यसूचकः;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    सार्वजनिक:
        शून्य सङ्कलय_Var_Name_Part(AstSFormatF* ध्रुव nodep, std::string&& name) {
            यदि (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } अन्यथा {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.निवेशय(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            यदि (name == "") {
                यदि (*m_first_Expression_Inside_सूचकांक_Pointerp) {
                    (*m_first_Expression_Inside_सूचकांक_Pointerp)
                        ->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Multiple expressions inside indices of complex "
                            "expression in constraint\n"
                                << (*m_first_Expression_Inside_सूचकांक_Pointerp)->warnContextPrimary()
                                << nodep->warnOther() << "... Location of second expression\n"
                                << nodep->warnContextSecondary());
                }
                *m_first_Expression_Inside_सूचकांक_Pointerp = nodep;
            }
        }

        AstMemberSel* access_वृक्षः() { प्रत्यागम m_शिखरम्_Nested_Array_Member_Selp; }
        ध्रुव std::string& smtName() { प्रत्यागम m_smtName; }

        Ast_Stmt_व्यञ्जकम्* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* ध्रुव methodp = नूतन AstCMethodHard{
                varp->fileline(),
                नूतन AstVarRef{varp->fileline(), VN_AS(genp->user2p(), ग्रन्थिः_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtype_समुच्चयः_Void();

            methodp->सङ्कलय_Pinsp(m_शिखरम्_Nested_Array_Member_Selp);  // Variable

            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep();  //विस्तारः
            ध्रुव आकार_t विस्तारः = m_शिखरम्_Nested_Array_Member_Selp->varp()->dtypep()->विस्तारः();
            methodp->सङ्कलय_Pinsp(नूतन AstConst{dtypep->fileline(), AstConst::Unsized64{}, विस्तारः});

            methodp->सङ्कलय_Pinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = शून्यसूचकः;

            methodp->सङ्कलय_Pinsp(
                नूतन AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_शिखरम्_Nested_Array_Member_Selp = शून्यसूचकः;
            प्रत्यागम methodp->makeStmt();
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* cloneName() { प्रत्यागम m_nestedNameFormatTopp->clone_वृक्षः(असत्य); }

        शून्य error() {
            VL_DO_DANGLING(m_शिखरम्_Nested_Array_Member_Selp->delete_वृक्षः(), m_शिखरम्_Nested_Array_Member_Selp);
        }

        Nested_Access_मार्गः(AstMemberSel* शिखरम्_Member_Selp, Ast_ग्रन्थिः** pointer_गन्तव्यम्_First_Exprp)
            : m_शिखरम्_Nested_Array_Member_Selp{शिखरम्_Member_Selp}
            , m_first_Expression_Inside_सूचकांक_Pointerp{pointer_गन्तव्यम्_First_Exprp} {}
        ~Nested_Access_मार्गः() {
            यदि (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_वृक्षः(),
                            m_nestedNameFormatTopp = शून्यसूचकः);
                m_nestedNameFormatp = शून्यसूचकः;
            }
        }
    };
    Nested_Access_मार्गः* m_nestedAccess = शून्यसूचकः;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_ग्रन्थिः_व्यञ्जकम्* createEnumConstraint(File_पङ्क्तिः* ध्रुव fl, ध्रुव std::string& smtName,
                                      AstEnumDType* ध्रुव enumDtp, AstVar* ध्रुव genVarp) {
        Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(genVarp->user2p(), ग्रन्थिः_Module);
        ध्रुव पूर्णाङ्क विस्तारः = enumDtp->विस्तारः();
        std::string constraint = "(__Vbv (or";
        प्रति (AstEnumItem* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ध्रुव AstConst* ध्रुव vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvt_गन्तव्यम्_Str(vconstp->गन्तव्यम्_UInt()) + " "
                          + cvt_गन्तव्यम्_Str(विस्तारः) + "))";
        }
        constraint += "))";
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            fl, नूतन AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "\"" + constraint + "\""}};
        callp->dtype_समुच्चयः_Void();
        प्रत्यागम callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* find_Static_Rand_बहुलकम्_Var_Member(AstClass* classp) ध्रुव {
        यावत् (सत्य) {
            यदि (AstVar* ध्रुव varp
                = VN_CAST(m_memberMap.findMember(classp, "__Vstaticrandmode"), Var)) {
                प्रत्यागम varp;
            }
            AstClassExtends* ध्रुव extendsp = classp->extendsp();
            यदि (!extendsp) प्रत्यागम शून्यसूचकः;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    शून्य markConstrainedRandMember(AstStructDType* ध्रुव structp, ध्रुव std::string& name) {
        AstMemberDType* ध्रुव memberp
            = VN_CAST(m_memberMap.findMember(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(सत्य);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    तार्किक is_Rand_Qualified_मार्गः(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यावत् (nodep) {
            यदि (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
                यदि (AstStructDType* ध्रुव structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    ध्रुव AstMemberDType* ध्रुव memberp
                        = VN_CAST(m_memberMap.findMember(structp, selp->name()), MemberDType);
                    यदि (memberp && !memberp->यादृच्छिकम्().isRandomizable()) प्रत्यागम असत्य;
                }
                nodep = selp->fromp();
            } अन्यथा यदि (ध्रुव AstArraySel* ध्रुव selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } अन्यथा यदि (ध्रुव AstCMethodHard* ध्रुव methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } अन्यथा {
                भङ्ग;
            }
        }
        प्रत्यागम सत्य;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    स्थिर शून्य markStructConstrainedRandRecurse(Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        AstStructDType* ध्रुव structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        यदि (!structp || structp->packed()) प्रत्यागम;
        structp->markConstrainedRand(सत्य);
        प्रति (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            यदि (!memberp->यादृच्छिकम्().isRandomizable()) अनुवर्त;
            memberp->markConstrainedRand(सत्य);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string build_Member_मार्गः(ध्रुव AstMemberSel* ध्रुव memberSelp) {
        ध्रुव Ast_ग्रन्थिः* fromp = memberSelp->fromp();
        यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            प्रत्यागम refp->name() + "." + memberSelp->name();
        } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            प्रत्यागम build_Member_मार्गः(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        प्रत्यागम "";
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* getFromp(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
        यदि (ध्रुव AstCMethodHard* ध्रुव cmethp = VN_CAST(nodep, CMethodHard)) {
            प्रत्यागम cmethp->fromp();
        }
        प्रत्यागम getSelFromp(nodep);
    }

    स्थिर std::string extract_Solve_Before_आधारः_Name(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        यावत् (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = getFromp(exprp)) exprp = fromp;
        यदि (ध्रुव AstVarRef* ध्रुव vrefp = VN_CAST(exprp, VarRef)) प्रत्यागम vrefp->name();
        प्रत्यागम "";
    }

    स्थिर शून्य build_Name_उपसर्गः(AstCExpr* exprp, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
        यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            exprp->सङ्कलय("(\""s + refp->name());
            प्रत्यागम;
        }

        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = getFromp(nodep);
        यदि (fromp) build_Name_उपसर्गः(exprp, fromp);

        यदि (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
            exprp->सङ्कलय("." + selp->name());
        } अन्यथा यदि (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            प्रत्यागम;
        } अन्यथा यदि (ध्रुव AstCMethodHard* ध्रुव cmethp = VN_CAST(nodep, CMethodHard)) {
            यदि (cmethp->विधानम्() == VCMethod::ARRAY_AT) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = cmethp->pinsp();
                exprp->सङ्कलय(".\" + ");
                AstCFuncHard* ध्रुव solverHexFuncp
                    = नूतन AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_गन्तव्यम्_SOLVER_HEX};
                solverHexFuncp->सङ्कलय_Pinsp(argp->clone_वृक्षः_विशुद्धम्(असत्य));
                solverHexFuncp->dtype_समुच्चयः_String();
                exprp->सङ्कलय(solverHexFuncp);
                exprp->सङ्कलय(" + \"");
            } अन्यथा {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                प्रत्यागम;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    स्थिर AstCExpr* build_मार्गः_व्यञ्जकम्(File_पङ्क्तिः* ध्रुव fl, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep,
                                   ध्रुव std::string selName) {
        AstCExpr* ध्रुव exprp = नूतन AstCExpr{fl, ""};
        build_Name_उपसर्गः(exprp, nodep);
        यदि (selName != "") {
            exprp->सङ्कलय(".");
            exprp->सङ्कलय(selName);
        }
        exprp->सङ्कलय("\")");
        प्रत्यागम exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_व्यञ्जकम्(File_पङ्क्तिः* fl, ध्रुव std::string& आधारः_Name,
                                    ध्रुव Ast_ग्रन्थिः_Sel* selp) {
        AstCExpr* ध्रुव p = नूतन AstCExpr{fl, ""};
        p->सङ्कलय("(\""s + आधारः_Name + ".\"");
        p->सङ्कलय("+");
        AstCFuncHard* ध्रुव solverHexFuncp
            = नूतन AstCFuncHard{fl, VCFunction::RANDOMIZER_गन्तव्यम्_SOLVER_HEX};
        solverHexFuncp->सङ्कलय_Pinsp(selp->bitp()->clone_वृक्षः_विशुद्धम्(असत्य));
        solverHexFuncp->dtype_समुच्चयः_String();
        p->सङ्कलय(solverHexFuncp);
        p->सङ्कलय(")");
        p->dtype_समुच्चयः_String();
        प्रत्यागम p;
    }

    // Helper: get fromp from MemberSel or StructSel
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* getSelFromp(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        यदि (ध्रुव AstMemberSel* ध्रुव mp = VN_CAST(exprp, MemberSel)) प्रत्यागम mp->fromp();
        यदि (ध्रुव AstStructSel* ध्रुव sp = VN_CAST(exprp, StructSel)) प्रत्यागम sp->fromp();
        प्रत्यागम शून्यसूचकः;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* build_Solve_Before_Name_व्यञ्जकम्(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        यदि (ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* ध्रुव p = नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "\"" + varrefp->name() + "\"s"};
            p->dtype_समुच्चयः_String();
            प्रत्यागम p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selFromp = getSelFromp(exprp)) {
            ध्रुव std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            यदि (ध्रुव AstArraySel* ध्रुव arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string आधारः_Name;
                यदि (ध्रुव AstVarRef* ध्रुव vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    आधारः_Name = vp->name();
                } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    आधारः_Name = build_Member_मार्गः(mp);
                }
                यदि (आधारः_Name.रिक्तानि()) प्रत्यागम शून्यसूचकः;
                AstCExpr* ध्रुव p = नूतन AstCExpr{fl, ""};
                p->सङ्कलय("(\""s + आधारः_Name + ".\"");
                p->सङ्कलय("+");
                AstCFuncHard* ध्रुव solverHexFuncp
                    = नूतन AstCFuncHard{fl, VCFunction::RANDOMIZER_गन्तव्यम्_SOLVER_HEX};
                solverHexFuncp->सङ्कलय_Pinsp(arrSelp->bitp()->clone_वृक्षः_विशुद्धम्(असत्य));
                solverHexFuncp->dtype_समुच्चयः_String();
                p->सङ्कलय(solverHexFuncp);
                p->सङ्कलय(")");
                p->dtype_समुच्चयः_String();
                प्रत्यागम p;
            }
            // Static member path (obj.field)
            यदि (ध्रुव AstVarRef* ध्रुव vp = VN_CAST(selFromp, VarRef)) {
                ध्रुव std::string मार्गः = vp->name() + "." + selName;
                AstCExpr* ध्रुव p = नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "\"" + मार्गः + "\"s"};
                p->dtype_समुच्चयः_String();
                प्रत्यागम p;
            }
            यदि (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* ध्रुव p = build_मार्गः_व्यञ्जकम्(fl, selFromp, selName);
                p->dtype_समुच्चयः_String();
                प्रत्यागम p;
            }
            प्रत्यागम शून्यसूचकः;
        }
        यदि (VN_IS(exprp, CMethodHard)) {
            AstCExpr* ध्रुव p = build_मार्गः_व्यञ्जकम्(fl, exprp, "");
            p->dtype_समुच्चयः_String();
            प्रत्यागम p;
        }
        ध्रुव Ast_ग्रन्थिः_Sel* selp = VN_CAST(exprp, ArraySel);
        यदि (!selp) selp = VN_CAST(exprp, AssocSel);
        यदि (selp) {
            // arr[i] -> dynamic name
            std::string आधारः_Name;
            यदि (ध्रुव AstVarRef* ध्रुव vp = VN_CAST(selp->fromp(), VarRef)) {
                आधारः_Name = vp->name();
            } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव mp = VN_CAST(selp->fromp(), MemberSel)) {
                आधारः_Name = build_Member_मार्गः(mp);
            }
            यदि (आधारः_Name.रिक्तानि()) प्रत्यागम शून्यसूचकः;
            प्रत्यागम build_Array_Sel_Name_व्यञ्जकम्(fl, आधारः_Name, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        यदि (ध्रुव AstSel* ध्रुव bitSelp = VN_CAST(exprp, Sel)) {
            प्रत्यागम build_Solve_Before_Name_व्यञ्जकम्(fl, bitSelp->fromp());
        }
        प्रत्यागम शून्यसूचकः;
    }

    AstSFormatF* getConstFormat(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        प्रत्यागम नूतन AstSFormatF{nodep->fileline(), (nodep->विस्तारः() & 3) ? "#b%b" : "#x%x", असत्य,
                               nodep};
    }
    तार्किक editFormat(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (nodep->user1()) प्रत्यागम असत्य;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlink_Fr_पृष्ठभागः(&handle);
        handle.relink(getConstFormat(nodep));
        प्रत्यागम सत्य;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* wrapWithCond(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp,
                              ध्रुव पूर्णाङ्क विस्तारः) {
        यदि (condp) {
            File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
            प्रत्यागम नूतन AstCond{
                flp, condp, exprp,
                getConstFormat(नूतन AstConst{flp, AstConst::Widthed_मानम्{}, विस्तारः, 0})};
        }
        प्रत्यागम exprp;
    }
    AstSFormatF* editSMT(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = शून्यसूचकः,
                         Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = शून्यसूचकः, Ast_ग्रन्थिः_व्यञ्जकम्* thsp = शून्यसूचकः) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_व्यञ्जकम् = nodep->emitSMT();  // Might need child width (AstExtend)
        यदि (smt_व्यञ्जकम् == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            प्रत्यागम शून्यसूचकः;
        }

        ध्रुव पूर्णाङ्क lhs_विस्तारः = lhsp ? lhsp->विस्तारः() : 0;
        ध्रुव पूर्णाङ्क rhs_विस्तारः = rhsp ? rhsp->विस्तारः() : 0;
        ध्रुव पूर्णाङ्क ths_विस्तारः = thsp ? thsp->विस्तारः() : 0;
        Ast_ग्रन्थिः_व्यञ्जकम्* lhsCondp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* rhsCondp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* thsCondp = शून्यसूचकः;

        यदि (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = शून्यसूचकः;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlink_Fr_पृष्ठभागः() : lhsp),
                         ग्रन्थिः_व्यञ्जकम्);
            lhsCondp = m_conditionp;
        }
        यदि (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = शून्यसूचकः;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlink_Fr_पृष्ठभागः() : rhsp),
                         ग्रन्थिः_व्यञ्जकम्);
            rhsCondp = m_conditionp;
        }
        यदि (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = शून्यसूचकः;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlink_Fr_पृष्ठभागः() : thsp),
                         ग्रन्थिः_व्यञ्जकम्);
            thsCondp = m_conditionp;
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* argsp = शून्यसूचकः;
        प्रति (string::iterator pos = smt_व्यञ्जकम्.begin(); pos != smt_व्यञ्जकम्.अन्तः_2(); ++pos) {
            यदि (pos[0] == '%') {
                ++pos;
                विकल्प (pos[0]) {
                पक्ष '%': भङ्ग;
                पक्ष 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(argsp, wrapWithCond(lhsp, lhsCondp, lhs_विस्तारः));
                    lhsp = शून्यसूचकः;
                    भङ्ग;
                पक्ष 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(argsp, wrapWithCond(rhsp, rhsCondp, rhs_विस्तारः));
                    rhsp = शून्यसूचकः;
                    भङ्ग;
                पक्ष 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(argsp, wrapWithCond(thsp, thsCondp, ths_विस्तारः));
                    thsp = शून्यसूचकः;
                    भङ्ग;
                स्वभाव: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); भङ्ग;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* ध्रुव newp = नूतन AstSFormatF{nodep->fileline(), smt_व्यञ्जकम्, असत्य, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम newp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* editSingle(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* itemsp) {
        यदि (!itemsp) प्रत्यागम शून्यसूचकः;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = सत्य;

        {
            AstBegin* ध्रुव tempp
                = नूतन AstBegin{fl, "[EditWrapper]", itemsp->unlink_Fr_पृष्ठभागः_With_अग्र(), असत्य};
            VL_DO_DANGLING(iterate_And_अग्र_Null(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            यदि (itemsp) itemsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            VL_DO_DANGLING(tempp->delete_वृक्षः(), tempp);
        }
        यदि (!itemsp) प्रत्यागम शून्यसूचकः;

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprsp = VN_CAST(itemsp, ग्रन्थिः_व्यञ्जकम्);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        यदि (!exprsp->nextp()) प्रत्यागम exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        प्रति (Ast_ग्रन्थिः* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        प्रत्यागम नूतन AstSFormatF{fl, fmt.str(), असत्य, exprsp};
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newSel(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* arrayp, Ast_ग्रन्थिः_व्यञ्जकम्* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_ग्रन्थिः_DType* ध्रुव arrDtp = arrayp->unlink_Fr_पृष्ठभागः()->dtypep()->skipRefp();
        Ast_ग्रन्थिः_व्यञ्जकम्* selp = शून्यसूचकः;
        यदि (VN_IS(arrDtp, QueueDType) || VN_IS(arrDtp, DynArrayDType))
            selp = नूतन AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        अन्यथा यदि (VN_IS(arrDtp, UnpackArrayDType))
            selp = नूतन AstArraySel{fl, arrayp, idxp};
        अन्यथा यदि (VN_IS(arrDtp, AssocArrayDType))
            selp = नूतन AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        प्रत्यागम selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_ग्रन्थिः_व्यञ्जकम्* adjust_विस्तारः(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, पूर्णाङ्क target_विस्तारः,
                             VSigning targetSigning) {
        ध्रुव पूर्णाङ्क व्यञ्जकम्_विस्तारः = exprp->विस्तारः();
        यदि (target_विस्तारः > व्यञ्जकम्_विस्तारः) {
            // Extend to match target width
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव परिणाम = नूतन AstExtend{fl, exprp, target_विस्तारः};
            परिणाम->dtypeChgSigned(targetSigning);
            प्रत्यागम परिणाम;
        } अन्यथा यदि (target_विस्तारः < व्यञ्जकम्_विस्तारः) {
            // Truncate to match target width
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव परिणाम = नूतन AstSel{fl, exprp, 0, target_विस्तारः};
            परिणाम->dtypeChgSigned(targetSigning);
            प्रत्यागम परिणाम;
        } अन्यथा {
            // Width already matches
            प्रत्यागम exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_ग्रन्थिः_व्यञ्जकम्* extract_Return_व्यञ्जकम्(AstFunc* funcp) {
        AstVar* ध्रुव retVarp = VN_CAST(funcp->fvarp(), Var);
        यदि (!retVarp) प्रत्यागम शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* retExprp = शून्यसूचकः;
        पूर्णाङ्क नियोजय_गणनम् = 0;
        funcp->foreach([&](Ast_नियोजय* assignp) {
            AstVarRef* ध्रुव lhsp = VN_CAST(assignp->lhsp(), VarRef);
            यदि (!lhsp || lhsp->varp() != retVarp) प्रत्यागम;
            retExprp = assignp->rhsp();
            ++नियोजय_गणनम्;
        });
        // Simple function: initializer + return (2) or just return (1)
        यदि (नियोजय_गणनम् > 2 || !retExprp) प्रत्यागम शून्यसूचकः;
        प्रत्यागम retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    शून्य propagateUser1InlineRecurse(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (VN_IS(nodep, ग्रन्थिः_Var_Ref)) {
            nodep->user1(VN_AS(nodep, ग्रन्थिः_Var_Ref)->varp()->यादृच्छिकम्().isRandomizable());
            प्रत्यागम;
        }
        यदि (VN_IS(nodep, Const)) {
            nodep->user1(असत्य);
            प्रत्यागम;
        }
        पूर्णाङ्क anyChild = असत्य;  // Used as bool
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = VN_CAST(nodep->op1p(), ग्रन्थिः_व्यञ्जकम्)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = VN_CAST(nodep->op2p(), ग्रन्थिः_व्यञ्जकम्)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = VN_CAST(nodep->op3p(), ग्रन्थिः_व्यञ्जकम्)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp = VN_CAST(nodep->op4p(), ग्रन्थिः_व्यञ्जकम्)) {
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
        यदि (!preamblep) प्रत्यागम bodyp;
        nodep->user3p(शून्यसूचकः);
        preamblep->सङ्कलय_अग्र(bodyp);
        प्रत्यागम preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    तार्किक referencesForeachIdx(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {
        यदि (m_foreachIdxVars.रिक्तानि()) प्रत्यागम असत्य;
        प्रत्यागम nodep->exists(
            [&](ध्रुव AstVarRef* refp) { प्रत्यागम m_foreachIdxVars.गणनम्(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    शून्य सङ्कलय_Registration_Stmt(Ast_ग्रन्थिः* ध्रुव stmtp, Ast_ग्रन्थिः_FTask* ध्रुव initTaskp) {
        यदि (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->सङ्कलय_Bodyp(stmtp);
        } अन्यथा {
            initTaskp->सङ्कलय_Stmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_पङ्क्तिः* ध्रुव fl, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrExprp,
                                       Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxExprp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(arrExprp, idxExprp);
        AstSFormatF* ध्रुव formatp = नूतन AstSFormatF{fl, "(select %s %s)", असत्य, argsp};
        प्रत्यागम formatp;
    }

    शून्य समुच्चयः_Rand_बहुलकम्(AstVar* ध्रुव varp, AstMemberSel* ध्रुव memberselp,
                     ध्रुव std::string& smtName, ध्रुव Randomize_बहुलकम्& rand_बहुलकम्,
                     Ast_ग्रन्थिः_FTask* ध्रुव initTaskp) {
        Ast_ग्रन्थिः_Module* ध्रुव varClassp = VN_AS(varp->user2p(), ग्रन्थिः_Module);
        AstVar* ध्रुव sub_Rand_बहुलकम्_Varp = get_Rand_बहुलकम्_Var_From_Class(varClassp);
        यदि (sub_Rand_बहुलकम्_Varp) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव parentAccessp = memberselp->fromp()->clone_वृक्षः(असत्य);
            AstMemberSel* ध्रुव rand_बहुलकम्_Selp
                = नूतन AstMemberSel{varp->fileline(), parentAccessp, sub_Rand_बहुलकम्_Varp};
            rand_बहुलकम्_Selp->dtypep(sub_Rand_बहुलकम्_Varp->dtypep());
            AstCMethodHard* ध्रुव atp
                = नूतन AstCMethodHard{varp->fileline(), rand_बहुलकम्_Selp, VCMethod::ARRAY_AT,
                                     नूतन AstConst{varp->fileline(), rand_बहुलकम्.सूचकांक}};
            atp->dtype_समुच्चयः_UInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* ध्रुव enablep = नूतन AstCMethodHard{
                varp->fileline(),
                नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtype_समुच्चयः_Void();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : नूतन AstSFormatF{varp->fileline(), smtName, असत्य, शून्यसूचकः};
            nameExprp->dtypep(varp->dtypep());
            enablep->सङ्कलय_Pinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* ध्रुव disablep = नूतन AstCMethodHard{
                varp->fileline(),
                नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_समुच्चयः_VAR_DISABLED};
            disablep->dtype_समुच्चयः_Void();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disnp = nameExprp->clone_वृक्षः(असत्य);
            disablep->सङ्कलय_Pinsp(disnp);
            AstIf* ध्रुव ifp
                = नूतन AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            सङ्कलय_Registration_Stmt(ifp, initTaskp);
        }
    }

    शून्य markRandc(AstVar* ध्रुव varp, ध्रुव std::string& smtName, Ast_ग्रन्थिः_FTask* ध्रुव initTaskp) {
        AstCMethodHard* ध्रुव markp = नूतन AstCMethodHard{
            varp->fileline(),
            नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtype_समुच्चयः_Void();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : नूतन AstSFormatF{varp->fileline(), smtName, असत्य, शून्यसूचकः};
        nameExprp->dtypep(varp->dtypep());
        markp->सङ्कलय_Pinsp(nameExprp);
        सङ्कलय_Registration_Stmt(markp->makeStmt(), initTaskp);
    }

    Ast_ग्रन्थिः_Module* getLeftmostVarModulep(AstMemberSel* ध्रुव memberselp,
                                         AstVar* ध्रुव varp) ध्रुव {
        यदि (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_ग्रन्थिः* वामः_Most_Nodep = memberselp->fromp();
            यावत् (AstMemberSel* ध्रुव nestedMemberSelp = VN_CAST(वामः_Most_Nodep, MemberSel)) {
                वामः_Most_Nodep = nestedMemberSelp->fromp();
            }
            यदि (Ast_ग्रन्थिः_Var_Ref* ध्रुव मूलम्_Var_Refp = VN_CAST(वामः_Most_Nodep, ग्रन्थिः_Var_Ref)) {
                प्रत्यागम VN_AS(मूलम्_Var_Refp->varp()->user2p(), ग्रन्थिः_Module);
            }
            प्रत्यागम VN_AS(memberselp->user2p(), ग्रन्थिः_Module);
        }
        प्रत्यागम VN_AS(varp->user2p(), ग्रन्थिः_Module);
    }

    Ast_ग्रन्थिः_FTask* getInitTaskp(AstVar* ध्रुव varp, ध्रुव तार्किक memberselOrStructSel,
                               Ast_ग्रन्थिः_Module* ध्रुव classp) ध्रुव {
        यदि (m_inlineInitTaskp) प्रत्यागम m_inlineInitTaskp;
        varp->user3(सत्य);
        यदि (memberselOrStructSel) {
            यदि (Ast_ग्रन्थिः_FTask* ध्रुव initTaskp
                = VN_AS(m_memberMap.findMember(classp, "randomize"), ग्रन्थिः_FTask)) {
                प्रत्यागम initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            यदि (m_memberselInitTaskp) प्रत्यागम m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } अन्यथा {
            Ast_ग्रन्थिः_FTask* ध्रुव initTaskp
                = VN_AS(m_memberMap.findMember(classp, "new"), ग्रन्थिः_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            प्रत्यागम initTaskp;
        }
        प्रत्यागम शून्यसूचकः;
    }

    शून्य createWriteVarForClassFields(AstClass* ध्रुव elemClassp, ध्रुव std::string& smtName,
                                      AstVar* ध्रुव iterVarp, ध्रुव तार्किक isUnpackedClassRefArray,
                                      Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep, AstVar* ध्रुव varp,
                                      AstIf* ध्रुव ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) ध्रुव {
        File_पङ्क्तिः* ध्रुव fl = varp->fileline();
        Ast_ग्रन्थिः_Module* ध्रुव varClassp = VN_AS(varp->user2p(), ग्रन्थिः_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr पूर्णाङ्क idx_विस्तारः = 32;
        ध्रुव पूर्णाङ्क fmt_विस्तारः = VL_शब्दाः_I(idx_विस्तारः) * 8;

        प्रति (ध्रुव AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : शून्यसूचकः) {
            प्रति (Ast_ग्रन्थिः* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* ध्रुव memberVarp = VN_CAST(mnodep, Var);
                यदि (!memberVarp || !memberVarp->यादृच्छिकम्().isRandomizable()) अनुवर्त;
                Ast_ग्रन्थिः_DType* ध्रुव memberDtp = memberVarp->dtypep()->skipRefp();
                यदि (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) अनुवर्त;
                ध्रुव पूर्णाङ्क member_विस्तारः = memberDtp->विस्तारः();

                AstCStmt* ध्रुव fmtp = नूतन AstCStmt{fl};
                यदि (isUnpackedClassRefArray) {
                    fmtp->सङ्कलय("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } अन्यथा {
                    fmtp->सङ्कलय("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::गन्तव्यम्_string(fmt_विस्तारः) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->सङ्कलय(नूतन AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->सङ्कलय(");\n");
                ifNonNullp->सङ्कलय_Thensp(fmtp);

                AstVarRef* ध्रुव arrayWrRefp = नूतन AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_ग्रन्थिः_व्यञ्जकम्* atWritep;
                यदि (isUnpackedClassRefArray) {
                    atWritep = नूतन AstArraySel{fl, arrayWrRefp,
                                               नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
                } अन्यथा {
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
                writeVarp->dtype_समुच्चयः_Void();
                writeVarp->सङ्कलय_Pinsp(memberSelp);
                writeVarp->सङ्कलय_Pinsp(
                    नूतन AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(member_विस्तारः)});
                AstCExpr* ध्रुव nameRefp = नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->सङ्कलय_Pinsp(nameRefp);
                writeVarp->सङ्कलय_Pinsp(नूतन AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->सङ्कलय_Thensp(writeVarp->makeStmt());
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

        AstVar* ध्रुव iterVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, m_unique_नामानि.get("__Vi"),
                                            varp->findUInt32DType()};
        iterVarp->funcLocal(सत्य);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_ग्रन्थिः* ध्रुव stmtsp
            = नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, iterVarp, VAccess::WRITE}, नूतन AstConst{fl, 0}};

        Ast_ग्रन्थिः_व्यञ्जकम्* sizep;
        यदि (isUnpackedClassRefArray) {
            ध्रुव पूर्णाङ्क array_आकार_2
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = नूतन AstConst{fl, static_cast<uint32_t>(array_आकार_2)};
        } अन्यथा {
            AstVarRef* ध्रुव array_आकार_Refp = नूतन AstVarRef{fl, varClassp, varp, VAccess::READ};
            array_आकार_Refp->classOrPackagep(classOrPackagep);
            AstCMethodHard* ध्रुव dynSizep
                = नूतन AstCMethodHard{fl, array_आकार_Refp, VCMethod::DYN_आकार, शून्यसूचकः};
            dynSizep->dtype_समुच्चयः_UInt32();
            sizep = dynSizep;
        }

        AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
        stmtsp->सङ्कलय_अग्र(loopp);
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{
            fl, loopp, नूतन AstLt{fl, नूतन AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* ध्रुव arrayReadRefp = नूतन AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_ग्रन्थिः_व्यञ्जकम्* atReadp;
        यदि (isUnpackedClassRefArray) {
            atReadp
                = नूतन AstArraySel{fl, arrayReadRefp, नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
        } अन्यथा {
            atReadp = नूतन AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* ध्रुव ifNonNullp
            = नूतन AstIf{fl, नूतन AstNeq{fl, atReadp, नूतन AstConst{fl, AstConst::Null{}}}, शून्यसूचकः};
        loopp->सङ्कलय_Stmtsp(ifNonNullp);

        AstCStmt* ध्रुव bufDeclp = नूतन AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->सङ्कलय_Thensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
            fl, नूतन AstVarRef{fl, iterVarp, VAccess::WRITE},
            नूतन Ast_सङ्कलय{fl, नूतन AstConst{fl, 1}, नूतन AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(सत्य);
        Ast_ग्रन्थिः_FTask* initTaskp = m_inlineInitTaskp;
        यदि (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.findMember(varClassp, "new"), ग्रन्थिः_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->सङ्कलय_Stmtsp(stmtsp);
    }

    शून्य createSolverVarHandle(AstVar* ध्रुव varp, ध्रुव तार्किक structSelOrCMeth,
                               ध्रुव तार्किक isGlobalConstrained, ध्रुव Randomize_बहुलकम् rand_बहुलकम्,
                               AstMemberSel* ध्रुव memberselp, ध्रुव std::string& smtName,
                               Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep, Ast_ग्रन्थिः_Module* ध्रुव classp,
                               Ast_ग्रन्थिः_FTask* ध्रुव initTaskp) {
        uint32_t unpackedDims = 0;
        यदि (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(असत्य).second;
        }
        यदि (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* ध्रुव methodp = नूतन AstCMethodHard{
            varp->fileline(),
            नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_समुच्चयः_Void();
        यदि (memberselp) {
            methodp->सङ्कलय_Pinsp(memberselp);
        } अन्यथा {
            AstVarRef* ध्रुव varRefp
                = नूतन AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->सङ्कलय_Pinsp(varRefp);
        }
        ध्रुव आकार_t विस्तारः = arrayElementDTypep(varp->dtypep())->विस्तारः();
        methodp->सङ्कलय_Pinsp(नूतन AstConst{varp->fileline(), AstConst::Unsized64{}, विस्तारः});
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varnamep = नूतन AstCExpr{varp->fileline(), AstCExpr::विशुद्धम्{},
                                                   "\"" + smtName + "\"", varp->विस्तारः()};
        varnamep->dtypep(varp->dtypep());
        methodp->सङ्कलय_Pinsp(varnamep);
        methodp->सङ्कलय_Pinsp(
            नूतन AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        यदि (rand_बहुलकम्.उपयुङ्क्ते_बहुलकम् && !(isGlobalConstrained && memberselp)) {
            methodp->सङ्कलय_Pinsp(
                नूतन AstConst{varp->fileline(), AstConst::Unsized64{}, rand_बहुलकम्.सूचकांक});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->सङ्कलय_Stmtsp(methodp->makeStmt());
        यदि (varp->lifetime().isStatic() && rand_बहुलकम्.उपयुङ्क्ते_बहुलकम्) {
            AstCMethodHard* ध्रुव markp = नूतन AstCMethodHard{
                varp->fileline(),
                नूतन AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव namep = नूतन AstCExpr{varp->fileline(), AstCExpr::विशुद्धम्{},
                                                    "\"" + smtName + "\"", varp->विस्तारः()};
            namep->dtypep(varp->dtypep());
            markp->सङ्कलय_Pinsp(namep);
            markp->dtype_समुच्चयः_Void();
            initTaskp->सङ्कलय_Stmtsp(markp->makeStmt());
        }
        यदि (isGlobalConstrained && memberselp && rand_बहुलकम्.उपयुङ्क्ते_बहुलकम्) {
            समुच्चयः_Rand_बहुलकम्(varp, memberselp, smtName, rand_बहुलकम्, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        यदि (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* createVarExprp(Ast_ग्रन्थिः_Var_Ref* nodep, ध्रुव Randomize_बहुलकम् rand_बहुलकम्,
                                ध्रुव std::string& smtName, ध्रुव तार्किक isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* ध्रुव varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp = नूतन AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, असत्य, शून्यसूचकः};
        यदि (!rand_बहुलकम्.उपयुङ्क्ते_बहुलकम्) {
            // Global constraints keep nodep alive for write_var processing
            यदि (!isGlobalConstrained) VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_ग्रन्थिः_व्यञ्जकम्* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_वृक्षः(असत्य)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_ग्रन्थिः_व्यञ्जकम्* rand_बहुलकम्_Access;
        ध्रुव तार्किक varIsStatic = varp->lifetime().isStatic();
        AstClass* ध्रुव varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : शून्यसूचकः;
        AstVar* ध्रुव owner_Static_Rand_बहुलकम्_Varp
            = varOwningClassp ? find_Static_Rand_बहुलकम्_Var_Member(varOwningClassp) : शून्यसूचकः;
        यदि (owner_Static_Rand_बहुलकम्_Varp) {
            rand_बहुलकम्_Access = नूतन AstVarRef{varp->fileline(),
                                           VN_AS(owner_Static_Rand_बहुलकम्_Varp->user2p(), ग्रन्थिः_Module),
                                           owner_Static_Rand_बहुलकम्_Varp, VAccess::READ};
        } अन्यथा यदि (memberselp) {
            Ast_ग्रन्थिः_Module* ध्रुव varClassp = VN_AS(varp->user2p(), ग्रन्थिः_Module);
            AstVar* ध्रुव effective_Rand_बहुलकम्_Varp = get_Rand_बहुलकम्_Var_From_Class(varClassp);
            यदि (effective_Rand_बहुलकम्_Varp) {
                // Member's class has randmode, use it
                Ast_ग्रन्थिः_व्यञ्जकम्* parentAccess = memberselp->fromp()->clone_वृक्षः(असत्य);
                AstMemberSel* rand_बहुलकम्_Sel
                    = नूतन AstMemberSel{varp->fileline(), parentAccess, effective_Rand_बहुलकम्_Varp};
                rand_बहुलकम्_Sel->dtypep(effective_Rand_बहुलकम्_Varp->dtypep());
                rand_बहुलकम्_Access = rand_बहुलकम्_Sel;
            } अन्यथा {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_rand_बहुलकम्_Varp, nodep, "No m_randModeVarp");
                rand_बहुलकम्_Access
                    = नूतन AstVarRef{varp->fileline(), VN_AS(m_rand_बहुलकम्_Varp->user2p(), ग्रन्थिः_Module),
                                    m_rand_बहुलकम्_Varp, VAccess::READ};
            }
        } अन्यथा {
            UASSERT_OBJ(m_rand_बहुलकम्_Varp, nodep, "No m_randModeVarp");
            rand_बहुलकम्_Access
                = नूतन AstVarRef{varp->fileline(), VN_AS(m_rand_बहुलकम्_Varp->user2p(), ग्रन्थिः_Module),
                                m_rand_बहुलकम्_Varp, VAccess::READ};
        }

        AstCMethodHard* ध्रुव atp
            = नूतन AstCMethodHard{nodep->fileline(), rand_बहुलकम्_Access, VCMethod::ARRAY_AT,
                                 नूतन AstConst{nodep->fileline(), rand_बहुलकम्.सूचकांक}};
        atp->dtype_समुच्चयः_UInt32();
        exprp = नूतन AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(सत्य);  // Mark as formatted
        प्रत्यागम exprp;
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        यदि (varp->user4p()) {
            तार्किक is_आकार_Ref = असत्य;
            यदि (AstCMethodHard* ध्रुव methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                यदि (methodp->विधानम्() == VCMethod::ASSOC_आकार
                    || methodp->विधानम्() == VCMethod::DYN_आकार) {
                    is_आकार_Ref = सत्य;
                }
            }
            यदि (!is_आकार_Ref && m_आकार_Constrained_Arraysp) { m_आकार_Constrained_Arraysp->निवेशय(varp); }
        }

        // Check if this variable is marked as globally constrained
        ध्रुव तार्किक isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = शून्यसूचकः;
        तार्किक structSelOrCMeth = असत्य;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_ग्रन्थिः_DType* selectedDtypep = शून्यसूचकः;
        std::string smtName;
        यदि (m_nestedAccess) {
            m_nestedAccess->सङ्कलय_Var_Name_Part(
                नूतन AstSFormatF{varp->fileline(), nodep->name(), असत्य, शून्यसूचकः}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_वृक्षः();
        } अन्यथा यदि (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_ग्रन्थिः* शिखरम्_Member_Sel = nodep->backp();
            यावत् (VN_IS(शिखरम्_Member_Sel->backp(), MemberSel)) {
                शिखरम्_Member_Sel = शिखरम्_Member_Sel->backp();
            }
            memberselp = VN_AS(शिखरम्_Member_Sel, MemberSel)->clone_वृक्षः(असत्य);
            smtName = build_Member_मार्गः(memberselp);
        } अन्यथा यदि (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_ग्रन्थिः* शिखरम्_Nodep = nodep->backp();
            यावत् (VN_IS(शिखरम्_Nodep->backp(), StructSel) || VN_IS(शिखरम्_Nodep->backp(), CMethodHard)) {
                शिखरम्_Nodep = शिखरम्_Nodep->backp();
            }

            यदि (VN_IS(शिखरम्_Nodep, StructSel) || VN_IS(शिखरम्_Nodep, CMethodHard)) {
                structSelOrCMeth = सत्य;
                selectedDtypep = VN_AS(शिखरम्_Nodep, ग्रन्थिः_व्यञ्जकम्)->dtypep();
            }

            memberselp = VN_CAST(शिखरम्_Nodep, MemberSel);
            यदि (memberselp) smtName = build_Member_मार्गः(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            यदि (smtName == "") smtName = nodep->name();
        } अन्यथा {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        यदि (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        यदि (dtype_अन्तर्भवति_Real(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            प्रत्यागम;
        }
        Ast_ग्रन्थिः_Module* ध्रुव classOrPackagep = nodep->classOrPackagep();
        ध्रुव Randomize_बहुलकम् rand_बहुलकम् = {.asUQuad = varp->user1()};
        यदि (!rand_बहुलकम्.उपयुङ्क्ते_बहुलकम् && editFormat(nodep)) प्रत्यागम;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType);
        यदि (enumDtp
            && (std::find(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.अन्तः_2(), varp)
                == m_rangeConstrainedEnums.अन्तः_2())) {
            AstVar* genVarp = m_genp;
            यदि (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_ग्रन्थिः_व्यञ्जकम्* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_ग्रन्थिः_FTask* targetTaskp = m_inlineInitTaskp;
            यदि (!targetTaskp) targetTaskp = get_Create_Constraint_सज्जता_Func(m_classp, m_memberMap);
            UASSERT_OBJ(targetTaskp, nodep, "No function to inline enum range constraint into");
            targetTaskp->सङ्कलय_Stmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.प्रक्षिप_पृष्ठभागः(varp);
        }

        VNRelinker relinker;
        nodep->unlink_Fr_पृष्ठभागः(&relinker);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp
            = createVarExprp(nodep, rand_बहुलकम्, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        ध्रुव तार्किक alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.गणनम्(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.गणनम्(smtName) > 0
                                    : memberselp          ? m_writtenVars.गणनम्(smtName) > 0
                                                          : varp->user3();
        ध्रुव तार्किक shouldWriteVar = !alreadyWritten;
        यदि (shouldWriteVar) {
            // Track this variable path as written
            यदि (m_inlineInitTaskp)
                m_inlineWrittenVars.निवेशय(smtName);
            अन्यथा यदि (isGlobalConstrained || memberselp)
                m_writtenVars.निवेशय(smtName);
            // For global constraints, delete nodep after processing
            यदि (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);

            तार्किक isClassRefArray = असत्य;
            तार्किक isUnpackedClassRefArray = असत्य;
            AstClassRefDType* elemClassRefDtp = शून्यसूचकः;
            {
                Ast_ग्रन्थिः_DType* varDtp = varp->dtypep()->skipRefp();
                यदि (varDtp->isNonPackedArray()) {
                    Ast_ग्रन्थिः_DType* ध्रुव elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    यदि (elemClassRefDtp) {
                        isClassRefArray = सत्य;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_ग्रन्थिः_Module* ध्रुव classp = getLeftmostVarModulep(memberselp, varp);
            Ast_ग्रन्थिः_FTask* ध्रुव initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            यदि (m_nestedAccess) {
                Ast_Stmt_व्यञ्जकम्* ध्रुव writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                सङ्कलय_Registration_Stmt(writeVarStmtp, initTaskp);
                यदि (isGlobalConstrained && memberselp && rand_बहुलकम्.उपयुङ्क्ते_बहुलकम्) {
                    समुच्चयः_Rand_बहुलकम्(varp, memberselp, smtName, rand_बहुलकम्, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                यदि (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } अन्यथा यदि (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } अन्यथा {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, rand_बहुलकम्,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } अन्यथा {
            // Variable already written, clean up cloned memberselp if any
            यदि (memberselp) VL_DO_DANGLING(memberselp->delete_वृक्षः(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            यदि (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
        यदि (m_nestedAccess) { VL_DO_DANGLING(विलोपय m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_ग्रन्थिः_व्यञ्जकम्* build_गणनम्_Ones_Expansion(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* argp) {
        ध्रुव V3_सङ्ख्या अङ्कः_One{fl, argp->विस्तारः(), 1};
        Ast_ग्रन्थिः_व्यञ्जकम्* sump = नूतन AstAnd{fl, argp, नूतन AstConst{fl, अङ्कः_One}};
        sump->user1(सत्य);
        प्रति (पूर्णाङ्क i = 1; i < argp->विस्तारः(); i++) {
            V3_सङ्ख्या अङ्कः_Bit_आवरणम्{fl, argp->विस्तारः(), 0};
            अङ्कः_Bit_आवरणम्.समुच्चयः_Bit(i, 1);
            AstAnd* ध्रुव andp
                = नूतन AstAnd{fl, argp->clone_वृक्षः_विशुद्धम्(असत्य), नूतन AstConst{fl, अङ्कः_Bit_आवरणम्}};
            andp->user1(सत्य);
            Ast_सरणम्_R* ध्रुव shiftp
                = नूतन Ast_सरणम्_R{fl, andp,
                                नूतन AstConst{fl, AstConst::Widthed_मानम्{}, argp->विस्तारः(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(सत्य);
            sump = नूतन Ast_सङ्कलय{fl, sump, shiftp};
            sump->user1(सत्य);
        }
        प्रत्यागम sump;
    }

    शून्य दर्शय(Ast_गणनम्_Ones* nodep) override {
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* sump = build_गणनम्_Ones_Expansion(fl, argp);
        sump = adjust_विस्तारः(fl, sump, nodep->विस्तारः(), nodep->dtypep()->numeric());
        sump->user1(सत्य);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(sump);
    }
    शून्य दर्शय(AstRedXor* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();

        Ast_ग्रन्थिः_व्यञ्जकम्* redxorp = नूतन AstSel{fl, argp, 0, 1};
        redxorp->user1(सत्य);
        प्रति (पूर्णाङ्क i = 1; i < argp->विस्तारः(); i++) {
            AstSel* ध्रुव selp = नूतन AstSel{fl, argp->clone_वृक्षः_विशुद्धम्(असत्य), i, 1};
            selp->user1(सत्य);

            redxorp = नूतन AstXor{fl, redxorp, selp};
            redxorp->user1(सत्य);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(redxorp);
    }
    शून्य दर्शय(AstRedAnd* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        // Convert to (~x == 0)
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, argp->विस्तारः(), 0};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव negp = नूतन AstNot{fl, argp};
        negp->user1(सत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = नूतन AstEq{fl, negp, नूतन AstConst{fl, अङ्कः_Zero}};
        eqp->user1(सत्य);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(eqp);
    }
    शून्य दर्शय(AstRedOr* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        // Convert to (x != 0)
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, argp->विस्तारः(), 0};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव neqp = नूतन AstNeq{fl, argp, नूतन AstConst{fl, अङ्कः_Zero}};
        neqp->user1(सत्य);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(neqp);
    }
    शून्य दर्शय(AstOneHot* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        ध्रुव पूर्णाङ्क w = argp->विस्तारः();

        ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, w, 0};
        AstNeq* ध्रुव neZerop
            = नूतन AstNeq{fl, argp->clone_वृक्षः_विशुद्धम्(असत्य), नूतन AstConst{fl, अङ्कः_Zero}};
        neZerop->user1(सत्य);

        ध्रुव V3_सङ्ख्या अङ्कः_One{fl, w, 1};
        AstSub* ध्रुव subp = नूतन AstSub{fl, argp->clone_वृक्षः_विशुद्धम्(असत्य), नूतन AstConst{fl, अङ्कः_One}};
        subp->user1(सत्य);

        AstAnd* ध्रुव andp = नूतन AstAnd{fl, argp, subp};
        andp->user1(सत्य);

        ध्रुव V3_सङ्ख्या अङ्कः_Zero2{fl, w, 0};
        AstEq* ध्रुव eqZerop = नूतन AstEq{fl, andp, नूतन AstConst{fl, अङ्कः_Zero2}};
        eqZerop->user1(सत्य);

        AstLogAnd* ध्रुव resultp = नूतन AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(सत्य);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(resultp);
    }
    शून्य दर्शय(AstOneHot0* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        // $onehot0(x) = (x & (x-1)) == 0
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        ध्रुव पूर्णाङ्क w = argp->विस्तारः();

        ध्रुव V3_सङ्ख्या अङ्कः_One{fl, w, 1};
        AstSub* ध्रुव subp = नूतन AstSub{fl, argp->clone_वृक्षः_विशुद्धम्(असत्य), नूतन AstConst{fl, अङ्कः_One}};
        subp->dtypeFrom(argp);
        subp->user1(सत्य);

        AstAnd* ध्रुव andp = नूतन AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(सत्य);

        ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, w, 0};
        AstEq* ध्रुव eqp = नूतन AstEq{fl, andp, नूतन AstConst{fl, अङ्कः_Zero}};
        eqp->user1(सत्य);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(eqp);
    }
    शून्य दर्शय(Ast_गणनम्_द्वयङ्काः* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

        तार्किक गणनम्_Ones = असत्य;
        तार्किक गणनम्_Zeros = असत्य;
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            ध्रुव AstConst* ध्रुव cp = VN_CAST(ctrlp, Const);
            यदि (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* ध्रुव zerop
                    = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, nodep->विस्तारः(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                प्रत्यागम;
            }
            यदि (cp->अङ्कः().bitIs1(0) && !गणनम्_Ones)
                गणनम्_Ones = सत्य;
            अन्यथा यदि (cp->अङ्कः().bitIs0(0) && !गणनम्_Zeros)
                गणनम्_Zeros = सत्य;
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        ध्रुव पूर्णाङ्क arg_विस्तारः = argp->विस्तारः();
        Ast_ग्रन्थिः_व्यञ्जकम्* sump = शून्यसूचकः;
        यदि (गणनम्_Ones && गणनम्_Zeros) {
            // ones + zeros = width for 2-state types
            sump = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, nodep->विस्तारः(), (uint32_t)arg_विस्तारः};
            VL_DO_DANGLING(argp->delete_वृक्षः(), argp);
        } अन्यथा यदि (गणनम्_Ones) {
            sump = build_गणनम्_Ones_Expansion(fl, argp);
        } अन्यथा यदि (गणनम्_Zeros) {
            // width - countones(x)
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव onesCountp = build_गणनम्_Ones_Expansion(fl, argp);
            ध्रुव V3_सङ्ख्या विस्तारः_Val{nodep, onesCountp->विस्तारः(), (uint32_t)arg_विस्तारः};
            sump = नूतन AstSub{fl, नूतन AstConst{fl, विस्तारः_Val}, onesCountp};
            sump->user1(सत्य);
        } अन्यथा {
            sump = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, nodep->विस्तारः(), 0U};
            VL_DO_DANGLING(argp->delete_वृक्षः(), argp);
        }

        sump = adjust_विस्तारः(fl, sump, nodep->विस्तारः(), nodep->dtypep()->numeric());
        sump->user1(सत्य);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(sump);
    }
    शून्य दर्शय(AstCLog2* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        ध्रुव पूर्णाङ्क arg_विस्तारः = argp->विस्तारः();
        ध्रुव पूर्णाङ्क परिणाम_विस्तारः = nodep->विस्तारः();

        Ast_ग्रन्थिः_व्यञ्जकम्* resultp
            = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, परिणाम_विस्तारः, (uint32_t)arg_विस्तारः};

        प्रति (पूर्णाङ्क k = arg_विस्तारः - 1; k >= 0; k--) {
            V3_सङ्ख्या देहली{fl, arg_विस्तारः, 0};
            यदि (k < 32)
                देहली.समुच्चयः_Long(1ULL << k);
            अन्यथा
                देहली.समुच्चयः_Bit(k, 1);

            AstLte* ध्रुव ltep
                = नूतन AstLte{fl, argp->clone_वृक्षः_विशुद्धम्(असत्य), नूतन AstConst{fl, देहली}};
            ltep->user1(सत्य);

            AstConst* ध्रुव valuep
                = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, परिणाम_विस्तारः, (uint32_t)k};

            resultp = नूतन AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(सत्य);
        }

        VL_DO_DANGLING(argp->delete_वृक्षः(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        iterate(resultp);
    }
    शून्य दर्शय(AstIsUnbounded* nodep) override {
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
        यदि (AstConst* ध्रुव exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव basep = nodep->lhsp();
            ध्रुव V3_सङ्ख्या अङ्कः_One{nodep, basep->विस्तारः(), 1};
            Ast_ग्रन्थिः_व्यञ्जकम्* powerp = नूतन AstConst{fl, अङ्कः_One};
            ध्रुव तार्किक आधारः_Signed = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            ध्रुव int32_t exponent = आधारः_Signed ? exponentp->गन्तव्यम्_SInt() : exponentp->गन्तव्यम्_UInt();
            यदि (exponent > 0) {
                प्रति (int32_t i = 0; i < exponent; i++) {
                    यदि (आधारः_Signed) {
                        powerp = नूतन AstMulS{fl, powerp, basep->clone_वृक्षः_विशुद्धम्(असत्य)};
                    } अन्यथा {
                        powerp = नूतन AstMul{fl, powerp, basep->clone_वृक्षः_विशुद्धम्(असत्य)};
                    }
                    powerp->user1(सत्य);
                }
            } अन्यथा यदि (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                यदि (आधारः_Signed) {
                    powerp = नूतन AstDivS{fl, powerp, basep->clone_वृक्षः_विशुद्धम्(असत्य)};
                    powerp->user1(सत्य);
                    powerp = नूतन AstDivS{fl, powerp, basep->clone_वृक्षः_विशुद्धम्(असत्य)};
                    powerp->user1(सत्य);
                } अन्यथा {
                    powerp = नूतन AstDiv{fl, powerp, basep->clone_वृक्षः_विशुद्धम्(असत्य)};
                    powerp->user1(सत्य);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            iterate(powerp);
        } अन्यथा यदि (ध्रुव AstConst* ध्रुव basep = VN_CAST(nodep->lhsp(), Const)) {
            यदि (basep->अङ्कः().गन्तव्यम्_UInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                ध्रुव पूर्णाङ्क विस्तारः = nodep->विस्तारः();
                ध्रुव तार्किक rhsDependent = rhsp->user1();
                AstConst* ध्रुव onep = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, विस्तारः, 1};
                Ast_सरणम्_L* ध्रुव shiftp = नूतन Ast_सरणम्_L{fl, onep, rhsp, विस्तारः};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_ग्रन्थिः_व्यञ्जकम्* resultp = shiftp;
                यदि (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nClonep = rhsp->clone_वृक्षः_विशुद्धम्(असत्य);
                    nClonep->user1(rhsDependent);
                    AstConst* ध्रुव zeroNp
                        = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, rhsp->विस्तारः(), 0};
                    AstGteS* ध्रुव condp = नूतन AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* ध्रुव zeroRp = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, विस्तारः, 0};
                    resultp = नूतन AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                iterate(resultp);
            } अन्यथा {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } अन्यथा {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    शून्य दर्शय(AstPow* nodep) override { handlePow(nodep); }
    शून्य दर्शय(AstPowSS* nodep) override { handlePow(nodep); }
    शून्य दर्शय(AstPowSU* nodep) override { handlePow(nodep); }
    शून्य दर्शय(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    शून्य handle_सरणम्(Ast_ग्रन्थिः_Biop* nodep) {
        यदि (editFormat(nodep)) प्रत्यागम;
        ध्रुव पूर्णाङ्क lhs_विस्तारः = nodep->lhsp()->विस्तारः();
        ध्रुव पूर्णाङ्क rhs_विस्तारः = nodep->rhsp()->विस्तारः();
        यदि (rhs_विस्तारः < lhs_विस्तारः) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
            AstExtend* ध्रुव extendp = नूतन AstExtend{fl, rhsp, lhs_विस्तारः};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(सत्य);
            nodep->rhsp(extendp);
        } अन्यथा यदि (rhs_विस्तारः > lhs_विस्तारः) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->dtypep();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव extendp
                = VN_IS(nodep, सरणम्_RS)
                      ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{fl, lhsp, rhs_विस्तारः})
                      : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtend{fl, lhsp, rhs_विस्तारः});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(सत्य);
            nodep->lhsp(extendp);
            nodep->dtype_समुच्चयः_Logic_Sized(rhs_विस्तारः, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlink_Fr_पृष्ठभागः(&relinker);
            AstSel* ध्रुव selp = नूतन AstSel{fl, nodep, 0, lhs_विस्तारः};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            प्रत्यागम;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    शून्य दर्शय(Ast_सरणम्_L* nodep) override { handle_सरणम्(nodep); }
    शून्य दर्शय(Ast_सरणम्_R* nodep) override { handle_सरणम्(nodep); }
    शून्य दर्शय(Ast_सरणम्_RS* nodep) override { handle_सरणम्(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः_Biop* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Uniop* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        editSMT(nodep, nodep->lhsp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Triop* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    शून्य दर्शय(AstCond* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        यदि (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtype_समुच्चयः_String();
            प्रत्यागम;
        }
        // Fall back to "(ite cond then else)"
        दर्शय(static_cast<Ast_ग्रन्थिः_Triop*>(nodep));
    }
    शून्य दर्शय(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        यदि (editFormat(nodep)) प्रत्यागम;
        editSMT(nodep, nodep->srcp());
    }
    शून्य दर्शय(AstSel* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        VNRelinker handle;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव msbp = नूतन AstSFormatF{
            fl, "%1d", असत्य,
            नूतन Ast_सङ्कलय{fl, nodep->lsbp()->clone_वृक्षः_विशुद्धम्(असत्य),
                       नूतन AstConst{fl, static_cast<uint32_t>(nodep->विस्तारः_Const() - 1)}}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp
            = नूतन AstSFormatF{fl, "%1d", असत्य, nodep->lsbp()->unlink_Fr_पृष्ठभागः(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    शून्य दर्शय(AstStructSel* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        ध्रुव तार्किक outermost = !m_defer_Rand_बहुलकम्_Hoist;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = outermost ? nodep->clone_वृक्षः(असत्य) : शून्यसूचकः;
        VL_RESTORER(m_structSel);
        m_structSel = सत्य;
        यदि (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = nodep->fromp();
            AstStructDType* ध्रुव structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            यदि (is_Rand_Qualified_मार्गः(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } अन्यथा {
                structp->markConstrainedRand(सत्य);
            }
        }
        // Mark Random for structArray
        यदि (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* ध्रुव structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            यदि (is_Rand_Qualified_मार्गः(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } अन्यथा {
                structp->markConstrainedRand(सत्य);
            }
        }
        {
            VL_RESTORER(m_defer_Rand_बहुलकम्_Hoist);
            m_defer_Rand_बहुलकम्_Hoist = सत्य;
            iterateChildren(nodep);
        }
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstSFormatF* newp = शून्यसूचकः;
        यदि (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = नूतन AstSFormatF{fl, "%s.%s." + nodep->name(), असत्य,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_वृक्षः_विशुद्धम्(सत्य)};
            यदि (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } अन्यथा {
            newp = नूतन AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), असत्य,
                                   शून्यसूचकः};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        यदि (origp && !hoist_Rand_बहुलकम्_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(origp), origp);
        }
    }
    शून्य सङ्कलय_String_Name_Part(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव indexp, ध्रुव std::string& fmt = "%32x") {
        यदि (m_nestedAccess) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitp = indexp->clone_वृक्षः_विशुद्धम्(असत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitFormatp = नूतन AstSFormatF{bitp->fileline(), fmt, असत्य, bitp};
            AstSFormatF* ध्रुव herep
                = नूतन AstSFormatF{indexp->fileline(), "%s.%s", असत्य, bitFormatp};
            m_nestedAccess->सङ्कलय_Var_Name_Part(herep, indexp->name());
        }
    }
    शून्य दर्शय(AstAssocSel* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->clone_वृक्षः(असत्य);
        AstSFormatF* newp = शून्यसूचकः;
        यदि (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            सङ्कलय_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxp = नूतन AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), असत्य,
                                                      nodep->bitp()->unlink_Fr_पृष्ठभागः(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } अन्यथा यदि (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* ध्रुव stringp = VN_AS(nodep->bitp(), CvtPackString);
            ध्रुव आकार_t string_आकार = VN_AS(stringp->lhsp(), Const)->विस्तारः();
            यदि (string_आकार > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << string_आकार << "bits, limit is 128 bits");
            }
            सङ्कलय_String_Name_Part(nodep->bitp());
            VNRelinker handle;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxp = नूतन AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), असत्य,
                                                      stringp->lhsp()->unlink_Fr_पृष्ठभागः(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } अन्यथा {
            यदि (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                ध्रुव पूर्णाङ्क actual_विस्तारः = nodep->bitp()->विस्तारः();
                std::string fmt;
                // Normalize to standard bit width
                यदि (actual_विस्तारः <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } अन्यथा यदि (actual_विस्तारः <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } अन्यथा {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::गन्तव्यम्_string(VL_शब्दाः_I(actual_विस्तारः) * 8) + "x";
                }
                यदि (m_nestedAccess) सङ्कलय_String_Name_Part(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxp
                    = नूतन AstSFormatF{fl, fmt, असत्य, nodep->bitp()->unlink_Fr_पृष्ठभागः(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } अन्यथा {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                यदि (m_nestedAccess) m_nestedAccess->error();
            }
        }
        यदि (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        यदि (!newp || !hoist_Rand_बहुलकम्_Over_Select_And_Member(newp, origp)) {
            VL_DO_DANGLING(origp->delete_वृक्षः(), origp);
        }
    }
    शून्य rename_Array_व्यञ्जकम्(AstSFormatF* ध्रुव newp) ध्रुव {
        यदि (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    शून्य दर्शय(AstArraySel* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        तार्किक सूचकांक_Is_Rand = असत्य;
        nodep->bitp()->foreach([&](ध्रुव Ast_ग्रन्थिः_Var_Ref* vrefp) {
            यदि (vrefp->varp()->यादृच्छिकम्().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                सूचकांक_Is_Rand = सत्य;
            }
        });
        यदि (m_nestedAccess) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitp = nodep->bitp()->clone_वृक्षः_विशुद्धम्(असत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitFormatp
                = सूचकांक_Is_Rand ? नूतन AstSFormatF{bitp->fileline(), bitp->name(), असत्य, शून्यसूचकः}
                              : नूतन AstSFormatF{bitp->fileline(), "%x", असत्य, bitp};
            AstSFormatF* ध्रुव herep
                = नूतन AstSFormatF{nodep->fileline(), "%s.%s", असत्य, bitFormatp};
            यदि (AstSel* ध्रुव selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->सङ्कलय_Var_Name_Part(herep, selp->fromp()->name());
            } अन्यथा {
                m_nestedAccess->सङ्कलय_Var_Name_Part(herep, bitp->name());
            }
            यदि (सूचकांक_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_वृक्षः(), bitp);
                प्रत्यागम;
            }
        }
        यदि (सूचकांक_Is_Rand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_ग्रन्थिः_व्यञ्जकम्* indexp = nodep->bitp()->unlink_Fr_पृष्ठभागः(&handle);
            यदि (indexp->विस्तारः() < 32) {
                AstExtend* ध्रुव extendp = नूतन AstExtend{fl, indexp, 32};
                extendp->user1(सत्य);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* ध्रुव newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_व्यञ्जकम्(newp);
        } अन्यथा {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->clone_वृक्षः(असत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव indexp
                = नूतन AstSFormatF{fl, "#x%8x", असत्य, nodep->bitp()->unlink_Fr_पृष्ठभागः(&handle)};
            handle.relink(indexp);
            AstSFormatF* ध्रुव newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_व्यञ्जकम्(newp);
            यदि (!newp || !hoist_Rand_बहुलकम्_Over_Select_And_Member(newp, origp)) {
                VL_DO_DANGLING(origp->delete_वृक्षः(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    स्थिर तार्किक is_Rand_बहुलकम्_Gate(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        ध्रुव AstCMethodHard* ध्रुव atp = VN_CAST(exprp, CMethodHard);
        यदि (!atp || atp->विधानम्() != VCMethod::ARRAY_AT) प्रत्यागम असत्य;
        ध्रुव AstVar* varp = शून्यसूचकः;
        यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        यदि (!varp) प्रत्यागम असत्य;
        ध्रुव AstClass* ध्रुव classp = VN_CAST(varp->user2p(), Class);
        प्रत्यागम classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    तार्किक hoist_Rand_बहुलकम्_Over_Select_And_Member(AstSFormatF* newp, Ast_ग्रन्थिः_व्यञ्जकम्* origp) {
        यदि (m_defer_Rand_बहुलकम्_Hoist) प्रत्यागम असत्य;
        // Only for selects yielding a non-array element (full chains)
        यदि (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) प्रत्यागम असत्य;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::सदिशम्<AstSFormatF*> चौकटाः;
        AstCond* बहुलकम्_Condp = शून्यसूचकः;
        प्रति (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firstp = curp->exprsp();
            चौकटाः.प्रक्षिप_पृष्ठभागः(curp);
            यदि (AstCond* ध्रुव condp = VN_CAST(firstp, Cond)) {
                यदि (is_Rand_बहुलकम्_Gate(condp->condp())) बहुलकम्_Condp = condp;
                भङ्ग;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        यदि (!बहुलकम्_Condp) प्रत्यागम असत्य;
        File_पङ्क्तिः* ध्रुव fl = newp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव modep = बहुलकम्_Condp->condp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* activep = बहुलकम्_Condp->thenp()->unlink_Fr_पृष्ठभागः();
        // Rebuild the select chain text around the SMT name, innermost first
        प्रति (स्वचालित it = चौकटाः.rbegin(); it != चौकटाः.rend(); ++it) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव remainingArgsp = VN_CAST((*it)->exprsp()->nextp(), ग्रन्थिः_व्यञ्जकम्);
            यदि (remainingArgsp) { remainingArgsp->unlink_Fr_पृष्ठभागः_With_अग्र(); }
            activep = नूतन AstSFormatF{fl, (*it)->name(), असत्य,
                                      Ast_ग्रन्थिः::सङ्कलय_अग्र(activep, remainingArgsp)};
        }
        AstCond* ध्रुव hoistp = नूतन AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(सत्य);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(newp), newp);
        प्रत्यागम सत्य;
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        यदि (nodep->varp()->यादृच्छिकम्().isRandomizable() && nodep->fromp()) {
            File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
            यदि (m_nestedAccess) {
                AstSFormatF* ध्रुव formatp = नूतन AstSFormatF{fl, nodep->name(), असत्य, शून्यसूचकः};
                m_nestedAccess->सङ्कलय_Var_Name_Part(नूतन AstSFormatF{fl, "%s.%s", असत्य, formatp},
                                               nodep->name());
            }
            Ast_ग्रन्थिः* मूलम्_ग्रन्थिः = nodep->fromp();
            यावत् (ध्रुव AstMemberSel* ध्रुव selp = VN_CAST(मूलम्_ग्रन्थिः, MemberSel)) {
                मूलम्_ग्रन्थिः = selp->fromp();
            }

            Ast_ग्रन्थिः_Sel* arraySelp = VN_CAST(मूलम्_ग्रन्थिः, ArraySel);
            यदि (!arraySelp) arraySelp = VN_CAST(मूलम्_ग्रन्थिः, AssocSel);

            यदि (arraySelp) {
                Ast_ग्रन्थिः_DType* ध्रुव arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_ग्रन्थिः_DType* ध्रुव elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : शून्यसूचकः;
                यदि (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    ध्रुव तार्किक isSimple
                        = nodep->fromp() == मूलम्_ग्रन्थिः && VN_IS(arraySelp->fromp(), VarRef);
                    यदि (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = नूतन Nested_Access_मार्गः(nodep->clone_वृक्षः(असत्य),
                                                              &m_firstExpressionInsideIndexp);
                        AstSFormatF* ध्रुव formatp
                            = नूतन AstSFormatF{fl, nodep->name(), असत्य, शून्यसूचकः};
                        m_nestedAccess->सङ्कलय_Var_Name_Part(
                            नूतन AstSFormatF{fl, "%s.%s", असत्य, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = सत्य;
                    nodep->user1(सत्य);
                    arraySelp->user1(सत्य);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->clone_वृक्षः(असत्य);
                    iterateChildren(nodep);
                    AstSFormatF* ध्रुव newp = नूतन AstSFormatF{fl, "%s." + nodep->name(), असत्य,
                                                              nodep->fromp()->unlink_Fr_पृष्ठभागः()};
                    nodep->replaceWith(newp);
                    यदि (!hoist_Rand_बहुलकम्_Over_Select_And_Member(newp, origp))
                        VL_DO_DANGLING(origp->delete_वृक्षः(), origp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    VL_DO_DANGLING(विलोपय m_nestedAccess, m_nestedAccess);
                    प्रत्यागम;
                }
            }
            // Check if the root variable participates in global constraints
            यदि (ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(मूलम्_ग्रन्थिः, VarRef)) {
                AstVar* ध्रुव constrainedVar = varRefp->varp();
                यदि (constrainedVar->globalConstrained()) {
                    तार्किक ध्रुव beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    यदि (beforeIterate) {
                        nodep->replaceWith(नूतन AstSFormatF{fl, "%s." + nodep->name(), असत्य,
                                                           nodep->fromp()->unlink_Fr_पृष्ठभागः()});
                    } अन्यथा {
                        nodep->replaceWith(nodep->fromp()->unlink_Fr_पृष्ठभागः());
                    }
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    प्रत्यागम;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        यदि (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), ग्रन्थिः_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* ध्रुव varRefp
                = नूतन AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->यादृच्छिकम्().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), ग्रन्थिः_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            दर्शय(varRefp);
        } अन्यथा यदि (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->विधानम्() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* ध्रुव cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_ग्रन्थिः_DType* ध्रुव arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_ग्रन्थिः_DType* ध्रुव elemDtp = arrayDtp->subDTypep()->skipRefp();
            यदि (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = सत्य;
                iterateChildren(nodep);
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                AstSFormatF* newp = शून्यसूचकः;
                यदि (AstSFormatF* ध्रुव fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    यदि (fromp->name() == "%s.%s") {
                        newp = नूतन AstSFormatF{fl, "%s.%s." + nodep->name(), असत्य,
                                               fromp->exprsp()->clone_वृक्षः_विशुद्धम्(सत्य)};
                    } अन्यथा {
                        newp = नूतन AstSFormatF{fl, fromp->name() + "." + nodep->name(), असत्य,
                                               शून्यसूचकः};
                    }
                } अन्यथा {
                    newp = नूतन AstSFormatF{fl, nodep->name(), असत्य, शून्यसूचकः};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            प्रत्यागम;
        } अन्यथा यदि (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            प्रत्यागम;
        } अन्यथा {
            editFormat(nodep);
        }
    }
    शून्य दर्शय(AstSFormatF* nodep) override {}
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {}
    शून्य दर्शय(AstCStmt* nodep) override {}
    शून्य दर्शय(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    शून्य दर्शय(AstConstraintIf* nodep) override {
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenp = editSingle(fl, nodep->thensp());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elsep = editSingle(fl, nodep->elsesp());
        यदि (thenp && elsep) {
            newp = नूतन AstCond{fl, nodep->condp()->unlink_Fr_पृष्ठभागः(), thenp, elsep};
        } अन्यथा यदि (thenp) {
            newp = नूतन AstLogIf{fl, nodep->condp()->unlink_Fr_पृष्ठभागः(), thenp};
        } अन्यथा यदि (elsep) {
            newp = नूतन AstLogIf{fl, नूतन AstNot{fl, nodep->condp()->unlink_Fr_पृष्ठभागः()}, elsep};
        }
        यदि (newp) {
            newp->dtype_समुच्चयः_Bit();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(सत्य);  // Assume result-dependent
            nodep->replaceWith(नूतन Ast_Constraint_व्यञ्जकम्{fl, newp});
        } अन्यथा {
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    शून्य दर्शय(AstBegin* nodep) override {}
    शून्य दर्शय(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

        यदि (!nodep->bodyp()) {
            nodep->unlink_Fr_पृष्ठभागः();
        } अन्यथा यदि (m_wantSingle) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            cstmtp->सङ्कलय("ret += \" \";\n");
            cstmtp->सङ्कलय("ret += ");
            cstmtp->सङ्कलय(itemp);
            cstmtp->सङ्कलय(";");
            Ast_ग्रन्थिः* ध्रुव bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
            cexprp->dtype_समुच्चयः_String();
            cexprp->सङ्कलय("([&]{\nstd::string ret;\n");
            cexprp->सङ्कलय(नूतन AstBegin{fl, m_unique_नामानि.get("__VrandBlock"),
                                     नूतन AstForeach{fl, nodep->headerp()->unlink_Fr_पृष्ठभागः(), bodyp},
                                     सत्य});
            cexprp->सङ्कलय("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(नूतन AstSFormatF{fl, "%s", असत्य, cexprp});
        } अन्यथा {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            प्रति (Ast_ग्रन्थिः* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(elemp, Var)) m_foreachIdxVars.निवेशय(varp);
            }
            iterate_And_अग्र_Null(nodep->bodyp());
            Ast_ग्रन्थिः* ध्रुव bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            nodep->replaceWith(
                नूतन AstBegin{fl, m_unique_नामानि.get("__VrandBlock"),
                             नूतन AstForeach{fl, nodep->headerp()->unlink_Fr_पृष्ठभागः(), bodyp}, सत्य});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(m_genp->user2p(), ग्रन्थिः_Module);

        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsTestp = build_Solve_Before_Name_व्यञ्जकम्(fl, lhsp);
            यदि (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                अनुवर्त;
            }
            VL_DO_DANGLING(lhsTestp->delete_वृक्षः(), lhsTestp);
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsNamep = build_Solve_Before_Name_व्यञ्जकम्(fl, rhsp);
                यदि (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    अनुवर्त;
                }
                AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                    fl, नूतन AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtype_समुच्चयः_Void();
                callp->सङ्कलय_Pinsp(build_Solve_Before_Name_व्यञ्जकम्(fl, lhsp));
                callp->सङ्कलय_Pinsp(rhsNamep);
                nodep->सङ्कलय_अत्र_This_As_अग्र(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstConstraintUnique* nodep) override {
        Ast_ग्रन्थिः_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_Module* genModp = शून्यसूचकः;

        यदि (m_classp) {
            initTaskp = VN_AS(m_memberMap.findMember(m_classp, "new"), ग्रन्थिः_FTask);
            genVarp = getRandomGenerator(m_classp);
        } अन्यथा {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        यदि (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        यदि (m_classp) {
            genModp = VN_AS(genVarp->user2p(), ग्रन्थिः_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_ग्रन्थिः* सज्जता_Stmtsp = शून्यसूचकः;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::सदिशम्<std::string> smtExprs;
        प्रति (Ast_ग्रन्थिः* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            यदि (AstVarRef* ध्रुव varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* ध्रुव varp = varRefp->varp();
                Ast_ग्रन्थिः_Module* ध्रुव varModp = VN_AS(varp->user2p(), ग्रन्थिः_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();

                ध्रुव स्थिर स्वचालित dynDTypeSupported
                    = [](ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) -> तार्किक {
                    प्रत्यागम VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elem_विस्तारः;
                uint32_t static_आकार = 0;  // Element count, zero when only known at run time
                यदि (ध्रुव AstUnpackArrayDType* ध्रुव ऊर्ध्वम् = VN_CAST(dtypep, UnpackArrayDType)) {
                    ध्रुव AstRange* ध्रुव rangep = ऊर्ध्वम्->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    static_आकार = ऊर्ध्वम्->elementsConst();
                    यदि (static_आकार > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        अनुवर्त;
                    }
                    elem_विस्तारः = static_cast<uint64_t>(varp->dtypep()->विस्तारः());
                } अन्यथा यदि (dynDTypeSupported(dtypep)) {
                    elem_विस्तारः = static_cast<uint64_t>(dtypep->subDTypep()->विस्तारः());
                } अन्यथा {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(असत्य));
                    अनुवर्त;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subp = dtypep->subDTypep()->skipRefp();
                यदि (VN_IS(subp, ग्रन्थिः_Array_DType) || VN_IS(subp, QueueDType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    अनुवर्त;
                }

                यदि (m_wantSingle && !static_आकार) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    अनुवर्त;
                }

                यदि (!varp->user3()) {
                    varp->user3(सत्य);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varnamep = नूतन AstCExpr{
                        fl, AstCExpr::विशुद्धम्{}, "\"" + varp->name() + "\"", varp->विस्तारः()};

                    AstCMethodHard* ध्रुव writeVarCallp = नूतन AstCMethodHard{
                        fl, नूतन AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* ध्रुव argVarRefp = नूतन AstVarRef{fl, varp, VAccess::READ};
                    यदि (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->सङ्कलय_Pinsp(argVarRefp);
                    writeVarCallp->सङ्कलय_Pinsp(नूतन AstConst{fl, AstConst::Unsized64{}, elem_विस्तारः});
                    writeVarCallp->सङ्कलय_Pinsp(varnamep);
                    writeVarCallp->सङ्कलय_Pinsp(नूतन AstConst{fl, 1});  // Dimension
                    ध्रुव Randomize_बहुलकम् rand_बहुलकम् = {.asUQuad = varp->user1()};
                    यदि (rand_बहुलकम्.उपयुङ्क्ते_बहुलकम्) {
                        writeVarCallp->सङ्कलय_Pinsp(
                            नूतन AstConst{fl, AstConst::Unsized64{}, rand_बहुलकम्.सूचकांक});
                    }
                    writeVarCallp->dtype_समुच्चयः_Void();
                    initTaskp->सङ्कलय_Stmtsp(नूतन Ast_Stmt_व्यञ्जकम्{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                यदि (varp->user4p() && m_आकार_Constrained_Arraysp) {
                    m_आकार_Constrained_Arraysp->निवेशय(varp);
                }

                यदि (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    यदि (static_आकार < 2) अनुवर्त;
                    std::string व्यञ्जकम्_Str = "(__Vbv (distinct";
                    प्रति (uint32_t i = 0; i < static_आकार; ++i) {
                        व्यञ्जकम्_Str += " (select " + varp->name() + " #x";
                        प्रति (पूर्णाङ्क सरणम्_2 = 28; सरणम्_2 >= 0; सरणम्_2 -= 4) {
                            व्यञ्जकम्_Str += "0123456789abcdef"[(i >> सरणम्_2) & 0xf];
                        }
                        व्यञ्जकम्_Str += ')';
                    }
                    smtExprs.emplace_पृष्ठभागः(व्यञ्जकम्_Str + "))");
                    अनुवर्त;
                }

                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव namep = नूतन AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* ध्रुव randUniqueCallp
                    = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->findVoidDType());
                randUniqueCallp->सङ्कलय_Pinsp(नूतन AstConst{fl, *m_uniqueConstraintId});
                सज्जता_Stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(सज्जता_Stmtsp, नूतन Ast_Stmt_व्यञ्जकम्{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        यदि (m_wantSingle && !smtExprs.रिक्तानि()) {
            std::string व्यञ्जकम्_Str = smtExprs.front();
            यदि (smtExprs.आकार() > 1) {
                व्यञ्जकम्_Str = "(bvand";
                प्रति (ध्रुव std::string& oneExprp : smtExprs) व्यञ्जकम्_Str += " " + oneExprp;
                व्यञ्जकम्_Str += ')';
            }
            AstSFormatF* ध्रुव newp = नूतन AstSFormatF{fl, व्यञ्जकम्_Str, असत्य, शून्यसूचकः};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        यदि (सज्जता_Stmtsp) nodep->सङ्कलय_अत्र_This_As_अग्र(सज्जता_Stmtsp);
        nodep->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(Ast_Constraint_व्यञ्जकम्* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_firstExpressionInsideIndexp);
        यदि (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->exprp();
            यदि (exprp->विस्तारः() > 1) {
                File_पङ्क्तिः* ध्रुव fl = exprp->fileline();
                ध्रुव V3_सङ्ख्या अङ्कः_Zero{fl, exprp->विस्तारः(), 0};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव neqp
                    = नूतन AstNeq{fl, exprp->unlink_Fr_पृष्ठभागः(), नूतन AstConst{fl, अङ्कः_Zero}};
                neqp->user1(सत्य);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        यदि (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        ध्रुव VCMethod विधानम्
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            nodep->fileline(),
            नूतन AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                          VAccess::READWRITE},
            विधानम्, nodep->exprp()->unlink_Fr_पृष्ठभागः()};
        callp->dtype_समुच्चयः_Void();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->सङ्कलय_Pinsp(नूतन AstCExpr{nodep->fileline(), AstCExpr::विशुद्धम्{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        ध्रुव uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->सङ्कलय_Pinsp(नूतन AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string prettyText;
        यदि (!v3Global.opt.protectIds()) {
            prettyText = nodep->fileline()->prettySource();
            आकार_t pos = 0;
            यावत् ((pos = prettyText.find('"', pos)) != std::string::npos) {
                prettyText.निवेशय(pos, "\\");
                pos += std::वाक्यदैर्घ्यम्("\\\"");
            }
        }
        callp->सङ्कलय_Pinsp(
            नूतन AstCExpr{nodep->fileline(), AstCExpr::विशुद्धम्{}, "\"" + prettyText + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstCMethodHard* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();

        यदि (nodep->विधानम्() == VCMethod::ARRAY_AT) {
            सङ्कलय_String_Name_Part(nodep->pinsp(), "%x");
            ध्रुव तार्किक सूचकांक_Is_Rand = nodep->pinsp()->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp) {
                प्रत्यागम vrefp->varp()->यादृच्छिकम्().isRandomizable();
            });
            यदि (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    ध्रुव Ast_ग्रन्थिः* ध्रुव basep = nodep->आधारः_Fromp(सत्य);
                    यदि (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        प्रत्यागम;
                    }
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = सूचकांक_Is_Rand ? शून्यसूचकः : nodep->clone_वृक्षः(असत्य);
                ध्रुव AstVarRef* ध्रुव आधारः_Refp
                    = m_structSel ? VN_CAST(nodep->fromp()->आधारः_Fromp(सत्य), VarRef) : शून्यसूचकः;
                यदि (आधारः_Refp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(आधारः_Refp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव indexp = nodep->pinsp()->clone_वृक्षः_विशुद्धम्(असत्य);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrayp = nodep->fromp()->clone_वृक्षः_विशुद्धम्(असत्य);
                    AstCMethodHard* ध्रुव currentSizep
                        = नूतन AstCMethodHard{fl, arrayp->clone_वृक्षः_विशुद्धम्(असत्य), VCMethod::DYN_आकार};
                    currentSizep->dtype_समुच्चयः_Int();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव अन्तर्_Range_Requiredp = नूतन AstLogAnd{
                        fl,
                        नूतन AstLteS{fl,
                                    नूतन AstConst{fl, AstConst::Widthed_मानम्{}, indexp->विस्तारः(), 0},
                                    indexp->clone_वृक्षः_विशुद्धम्(असत्य)},
                        नूतन AstGteS{fl, indexp->clone_वृक्षः_विशुद्धम्(असत्य), currentSizep}};
                    AstCMethodHard* ध्रुव resizep
                        = नूतन AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             नूतन Ast_सङ्कलय{fl, indexp, नूतन AstConst{fl, 1}}};
                    resizep->dtype_समुच्चयः_Void();
                    m_prepareConstrainedArraysp->सङ्कलय_Stmtsp(
                        नूतन AstIf{fl, अन्तर्_Range_Requiredp, resizep->makeStmt()});
                }
                AstCMethodHard* ध्रुव sizep
                    = m_structSel ? नूतन AstCMethodHard{fl, nodep->fromp()->clone_वृक्षः_विशुद्धम्(असत्य),
                                                       VCMethod::DYN_आकार}
                                  : शून्यसूचकः;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = nodep->pinsp()->unlink_Fr_पृष्ठभागः();
                AstSFormatF* newp;
                यदि (m_structSel) {
                    यदि (AstSFormatF* ध्रुव sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argsp
                        = Ast_ग्रन्थिः::सङ्कलय_अग्र(nodep->fromp()->unlink_Fr_पृष्ठभागः(), pinp);
                    sizep->dtype_समुच्चयः_Int();
                    AstLogAnd* ध्रुव condp
                        = नूतन AstLogAnd{fl,
                                        नूतन AstLteS{fl,
                                                    नूतन AstConst{fl, AstConst::Widthed_मानम्{},
                                                                 originalPinp->विस्तारः(), 0},
                                                    originalPinp->clone_वृक्षः_विशुद्धम्(असत्य)},
                                        नूतन AstLtS{fl, originalPinp->clone_वृक्षः_विशुद्धम्(असत्य), sizep}};
                    m_conditionp = m_conditionp ? नूतन AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = नूतन AstSFormatF{fl, "%s.%s", असत्य, argsp};
                } अन्यथा {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_पृष्ठभागः(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                यदि (origp && !hoist_Rand_बहुलकम्_Over_Select_And_Member(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_वृक्षः(), origp);
                }

            } अन्यथा यदि (सूचकांक_Is_Rand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } अन्यथा {
                nodep->user1(असत्य);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            प्रत्यागम;
        }

        यदि (nodep->विधानम्() == VCMethod::ARRAY_INSIDE) {
            ध्रुव तार्किक randArr = nodep->fromp()->user1();

            AstVar* ध्रुव newVarp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->findIntDType()};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxRefp = नूतन AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            Ast_Foreach_शीर्षकम्* ध्रुव headerp
                = नूतन Ast_Foreach_शीर्षकम्{fl, nodep->fromp()->clone_वृक्षः_विशुद्धम्(असत्य), newVarp};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_ग्रन्थिः* ध्रुव itemp = नूतन AstEq{fl, selp, nodep->pinsp()->unlink_Fr_पृष्ठभागः()};
            itemp->user1(सत्य);

            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            cstmtp->सङ्कलय("ret += \" \";\n");
            cstmtp->सङ्कलय("ret += ");
            cstmtp->सङ्कलय(iterateSubtreeReturnEdits(itemp));
            cstmtp->सङ्कलय(";");
            AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
            cexprp->dtype_समुच्चयः_String();
            cexprp->सङ्कलय("([&]{\nstd::string ret;\n");
            cexprp->सङ्कलय(नूतन AstBegin{fl, m_unique_नामानि.get("__VrandBlock"),
                                     नूतन AstForeach{fl, headerp, cstmtp}, सत्य});
            cexprp->सङ्कलय("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(नूतन AstSFormatF{fl, "%s", असत्य, cexprp});
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }

        // Array reduction methods (sum, product, and, or, xor)
        यदि (nodep->विधानम्() == VCMethod::ARRAY_R_योग
            || nodep->विधानम्() == VCMethod::ARRAY_R_गुणनफलम्
            || nodep->विधानम्() == VCMethod::ARRAY_R_AND || nodep->विधानम्() == VCMethod::ARRAY_R_OR
            || nodep->विधानम्() == VCMethod::ARRAY_R_XOR) {

            AstWith* ध्रुव withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            यदि (withp) {
                यदि (AstUnpackArrayDType* ध्रुव adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    ध्रुव पूर्णाङ्क array_आकार_2 = adtypep->elementsConst();
                    यदि (array_आकार_2 > v3Global.opt.constraint_Array_सीमा()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvt_गन्तव्यम्_Str(array_आकार_2)
                                          + " exceeds --constraint-array-limit of "
                                          + cvt_गन्तव्यम्_Str(v3Global.opt.constraint_Array_सीमा())
                                          + "), treating as state");
                        nodep->user1(असत्य);
                        यदि (editFormat(nodep)) प्रत्यागम;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        प्रत्यागम;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            ध्रुव तार्किक hasRandomArray = nodep->fromp()->user1();
            यदि (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(असत्य);
                यदि (editFormat(nodep)) प्रत्यागम;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                प्रत्यागम;
            }
            यदि (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                प्रत्यागम;
            }

            // Create loop variable and header
            AstVar* ध्रुव loopVarp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->findIntDType()};
            Ast_Foreach_शीर्षकम्* ध्रुव headerp
                = नूतन Ast_Foreach_शीर्षकम्{fl, nodep->fromp()->clone_वृक्षः_विशुद्धम्(असत्य), loopVarp};

            // Determine SMT operation and compute identity elements
            ध्रुव वर्ण* smtOp = शून्यसूचकः;
            std::string तटस्थः;
            यदि (withp) {
                // For 'with' clause: use binary format, compute from result width
                ध्रुव पूर्णाङ्क विस्तारः = nodep->dtypep()->विस्तारः();
                यदि (nodep->विधानम्() == VCMethod::ARRAY_R_योग) {
                    smtOp = "bvadd";
                    तटस्थः = "#b" + std::string(विस्तारः, '0');
                } अन्यथा यदि (nodep->विधानम्() == VCMethod::ARRAY_R_गुणनफलम्) {
                    smtOp = "bvmul";
                    तटस्थः = (विस्तारः > 0) ? "#b" + std::string(विस्तारः - 1, '0') + "1" : "#b0";
                } अन्यथा यदि (nodep->विधानम्() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    तटस्थः = "#b" + std::string(विस्तारः, '1');
                } अन्यथा यदि (nodep->विधानम्() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    तटस्थः = "#b" + std::string(विस्तारः, '0');
                } अन्यथा {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    तटस्थः = "#b" + std::string(विस्तारः, '0');
                }
            } अन्यथा {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* ध्रुव arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* ध्रुव arrVarp = arrRefp->varp();
                Ast_ग्रन्थिः_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ध्रुव पूर्णाङ्क elem_विस्तारः = elemDtp->विस्तारः();
                ध्रुव पूर्णाङ्क hexDigits = (elem_विस्तारः + 3) / 4;

                ध्रुव std::string zero_तटस्थः = "#x" + std::string(hexDigits, '0');
                ध्रुव std::string one_तटस्थः = "#x" + std::string(hexDigits - 1, '0') + "1";
                ध्रुव std::string all_Ones_तटस्थः = "#x" + std::string(hexDigits, 'f');

                यदि (nodep->विधानम्() == VCMethod::ARRAY_R_योग) {
                    smtOp = "bvadd";
                    तटस्थः = zero_तटस्थः;
                } अन्यथा यदि (nodep->विधानम्() == VCMethod::ARRAY_R_गुणनफलम्) {
                    smtOp = "bvmul";
                    तटस्थः = one_तटस्थः;
                } अन्यथा यदि (nodep->विधानम्() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    तटस्थः = all_Ones_तटस्थः;
                } अन्यथा यदि (nodep->विधानम्() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    तटस्थः = zero_तटस्थः;
                } अन्यथा {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    तटस्थः = zero_तटस्थः;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* ध्रुव cstmtp = नूतन AstCStmt{fl};
            यदि (withp) {
                // With 'with' clause: evaluate expression for each element
                ध्रुव तार्किक randArr = nodep->fromp()->user1();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव idxRefp = नूतन AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                ध्रुव पूर्णाङ्क परिणाम_विस्तारः = nodep->dtypep()->विस्तारः();
                ध्रुव VSigning परिणाम_Signing = nodep->dtypep()->numeric();

                Ast_ग्रन्थिः* प्रत्येकम्_Elem_Exprp = withp->exprp()->clone_वृक्षः_विशुद्धम्(असत्य);
                यदि (AstLambdaArgRef* ध्रुव मूलम्_Refp = VN_CAST(प्रत्येकम्_Elem_Exprp, LambdaArgRef)) {
                    यदि (मूलम्_Refp->सूचकांक()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* ध्रुव loopRef = नूतन AstVarRef{fl, loopVarp, VAccess::READ};
                        प्रत्येकम्_Elem_Exprp = adjust_विस्तारः(fl, loopRef, परिणाम_विस्तारः, परिणाम_Signing);
                    } अन्यथा {
                        // item at root -> replace with array element
                        प्रत्येकम्_Elem_Exprp = elemSelp->clone_वृक्षः_विशुद्धम्(असत्य);
                    }
                    VL_DO_DANGLING(मूलम्_Refp->delete_वृक्षः(), मूलम्_Refp);
                } अन्यथा {
                    प्रत्येकम्_Elem_Exprp->foreach([&](AstLambdaArgRef* refp) {
                        यदि (refp->सूचकांक()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* ध्रुव loopRef = नूतन AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_ग्रन्थिः* ध्रुव replacement
                                = adjust_विस्तारः(fl, loopRef, परिणाम_विस्तारः, परिणाम_Signing);
                            refp->replaceWith(replacement);
                        } अन्यथा {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_वृक्षः_विशुद्धम्(असत्य));
                        }
                        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_वृक्षः(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                प्रत्येकम्_Elem_Exprp = V3Const::constifyEdit(प्रत्येकम्_Elem_Exprp);

                प्रत्येकम्_Elem_Exprp->foreach([&](Ast_ग्रन्थिः* nodep) {
                    // Don't mark loop variable references as randomizable
                    यदि (!VN_IS(nodep, Const)) {
                        यदि (Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
                            यदि (vrefp->varp() != loopVarp) nodep->user1(सत्य);
                        } अन्यथा {
                            nodep->user1(सत्य);
                        }
                    }
                });

                cstmtp->सङ्कलय("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->सङ्कलय("ret += ");
                cstmtp->सङ्कलय(iterateSubtreeReturnEdits(प्रत्येकम्_Elem_Exprp));
                cstmtp->सङ्कलय(";\n");
                cstmtp->सङ्कलय("ret += \")\";\n");
            } अन्यथा {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* ध्रुव arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* ध्रुव arrVarp = arrRefp->varp();
                ध्रुव std::string smtArrayName = arrVarp->name();
                Ast_ग्रन्थिः_DType* ध्रुव elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                ध्रुव पूर्णाङ्क elem_विस्तारः = elemDtp->विस्तारः();
                Ast_ग्रन्थिः_Module* ध्रुव arrModulep = VN_AS(arrVarp->user2p(), ग्रन्थिः_Module);
                Ast_ग्रन्थिः_Module* ध्रुव genModulep = VN_AS(m_genp->user2p(), ग्रन्थिः_Module);
                arrVarp->user3(सत्य);

                // Add write_var call to init task
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrVarNamep
                    = नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "\"" + smtArrayName + "\"", elem_विस्तारः};
                AstCMethodHard* ध्रुव writeVarCallp
                    = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->सङ्कलय_Pinsp(नूतन AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->सङ्कलय_Pinsp(
                    नूतन AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elem_विस्तारः)});
                writeVarCallp->सङ्कलय_Pinsp(arrVarNamep);
                ध्रुव uint32_t unpackedDims = arrVarp->dtypep()->dimensions(असत्य).second;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->सङ्कलय_Pinsp(नूतन AstConst{fl, 1});  // Dimension

                ध्रुव Randomize_बहुलकम् rand_बहुलकम् = {.asUQuad = arrVarp->user1()};
                यदि (rand_बहुलकम्.उपयुङ्क्ते_बहुलकम्) {
                    writeVarCallp->सङ्कलय_Pinsp(
                        नूतन AstConst{fl, AstConst::Unsized64{}, rand_बहुलकम्.सूचकांक});
                }
                writeVarCallp->dtype_समुच्चयः_Void();
                Ast_ग्रन्थिः_FTask* initTaskp = m_inlineInitTaskp;
                यदि (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.findMember(arrModulep, "new"), ग्रन्थिः_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->सङ्कलय_Stmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* ध्रुव idxFormatp = नूतन AstSFormatF{
                    fl, "#x%08x", असत्य, नूतन AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* ध्रुव varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlink_Fr_पृष्ठभागः(), idxFormatp);

                cstmtp->सङ्कलय("ret += \" \";\n");
                cstmtp->सङ्कलय("ret += ");
                cstmtp->सङ्कलय(varNamep);
                cstmtp->सङ्कलय(";\n");
            }

            // Build final expression
            यदि (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
                cexprp->dtype_समुच्चयः_String();
                cexprp->सङ्कलय("([&]{\nstd::string ret = \"" + तटस्थः + "\";\n");
                cexprp->सङ्कलय(नूतन AstBegin{fl, m_unique_नामानि.get("__VrandBlock"),
                                         नूतन AstForeach{fl, headerp, cstmtp}, सत्य});
                cexprp->सङ्कलय("return ret;\n})()");
                nodep->replaceWith(नूतन AstSFormatF{fl, "%s", असत्य, cexprp});
            } अन्यथा {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
                cexprp->dtype_समुच्चयः_String();
                cexprp->सङ्कलय("([&]{\nstd::string ret;\n");
                cexprp->सङ्कलय(नूतन AstBegin{fl, m_unique_नामानि.get("__VrandBlock"),
                                         नूतन AstForeach{fl, headerp, cstmtp}, सत्य});
                cexprp->सङ्कलय("return ret.empty() ? \"" + तटस्थः + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(नूतन AstSFormatF{fl, "%s", असत्य, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }

        // We've handled DYN_SIZE already if they should be randomized
        यदि (nodep->विधानम्() == VCMethod::DYN_आकार || nodep->विधानम्() == VCMethod::ASSOC_आकार) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ग्रन्थिः_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            यावत् (VN_IS(ग्रन्थिः_Exprp, MemberSel)) {
                ग्रन्थिः_Exprp = VN_AS(ग्रन्थिः_Exprp, MemberSel)->fromp();
            }
            यदि (VN_IS(ग्रन्थिः_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_वृक्षः_विशुद्धम्(असत्य)));
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                प्रत्यागम;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(असत्य);

        यदि (editFormat(nodep)) प्रत्यागम;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;

        AstFunc* ध्रुव funcp = VN_CAST(nodep->taskp(), Func);
        यदि (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            प्रत्यागम;
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव retExprp = extract_Return_व्यञ्जकम्(funcp);
        यदि (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(असत्य);
            यदि (editFormat(nodep)) प्रत्यागम;
            प्रत्यागम;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        ध्रुव V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), शून्यसूचकः, असत्य);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_ग्रन्थिः_व्यञ्जकम्* inlinedp = retExprp->clone_वृक्षः_विशुद्धम्(असत्य);
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            ध्रुव AstVar* ध्रुव portp = tconnect.first;
            AstArg* ध्रुव argp = tconnect.second;
            यदि (!argp || !argp->exprp()) अनुवर्त;
            यदि (AstVarRef* ध्रुव मूलम्_Refp = VN_CAST(inlinedp, VarRef)) {
                यदि (मूलम्_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_वृक्षः_विशुद्धम्(असत्य);
                    VL_DO_DANGLING(मूलम्_Refp->delete_वृक्षः(), मूलम्_Refp);
                    अनुवर्त;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                यदि (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_वृक्षः_विशुद्धम्(असत्य));
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
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {}
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        यदि (editFormat(nodep)) प्रत्यागम;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_ग्रन्थिः* buildDisableSoftCallStmt(Ast_Constraint_व्यञ्जकम्* nodep) {
        std::string varName;
        यदि (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp = VN_CAST(nodep->exprp(), ग्रन्थिः_Var_Ref)) {
            varName = vrefp->name();
        } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } अन्यथा {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
            nodep->fileline(),
            नूतन AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), ग्रन्थिः_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            नूतन AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtype_समुच्चयः_Void();
        प्रत्यागम callp->makeStmt();
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
        Ast_ग्रन्थिः* hoistListp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            यदि (AstConstraintIf* ध्रुव ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_पङ्क्तिः* ध्रुव fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenCondp
                    = outerCondp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                       नूतन AstLogAnd{fl, outerCondp->clone_वृक्षः_विशुद्धम्(असत्य),
                                                     ifp->condp()->clone_वृक्षः_विशुद्धम्(असत्य)})
                                 : ifp->condp()->clone_वृक्षः_विशुद्धम्(असत्य);
                यदि (Ast_ग्रन्थिः* ध्रुव thenHoistp
                    = extractConditionalDisableSofts(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_ग्रन्थिः::सङ्कलय_अग्र(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_वृक्षः(), thenCondp);
                यदि (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notInnerp
                        = नूतन AstLogNot{fl, ifp->condp()->clone_वृक्षः_विशुद्धम्(असत्य)};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elseCondp
                        = outerCondp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{
                                           fl, outerCondp->clone_वृक्षः_विशुद्धम्(असत्य), notInnerp})
                                     : notInnerp;
                    यदि (Ast_ग्रन्थिः* ध्रुव elseHoistp
                        = extractConditionalDisableSofts(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_ग्रन्थिः::सङ्कलय_अग्र(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_वृक्षः(), elseCondp);
                }
            } अन्यथा यदि (Ast_Constraint_व्यञ्जकम्* ध्रुव exprp = VN_CAST(stmtp, Constraint_व्यञ्जकम्)) {
                यदि (exprp->isDisableSoft() && outerCondp) {
                    File_पङ्क्तिः* ध्रुव fl = exprp->fileline();
                    AstIf* ध्रुव hoistIfp = नूतन AstIf{fl, outerCondp->clone_वृक्षः_विशुद्धम्(असत्य),
                                                      buildDisableSoftCallStmt(exprp), शून्यसूचकः};
                    hoistListp = hoistListp ? Ast_ग्रन्थिः::सङ्कलय_अग्र(hoistListp,
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
        प्रत्यागम hoistListp;
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट Constraint_व्यञ्जकम्_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_ग्रन्थिः* nodep,
                                   Ast_ग्रन्थिः_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* rand_बहुलकम्_Varp, std::समुच्चयः<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3_Unique_नामानि& unique_नामानि,
                                   Ast_ग्रन्थिः_FTask* memberselInitTaskp = शून्यसूचकः,
                                   VInsertion_समुच्चयः<AstVar*>* आकार_Constrained_Arraysp = शून्यसूचकः,
                                   Ast_ग्रन्थिः_FTask* prepareConstrainedArraysp = शून्यसूचकः)
        : m_classp{classp}
        , m_inlineInitTaskp{inlineInitTaskp}
        , m_memberselInitTaskp{memberselInitTaskp}
        , m_genp{genp}
        , m_rand_बहुलकम्_Varp{rand_बहुलकम्_Varp}
        , m_memberMap{memberMap}
        , m_writtenVars{writtenVars}
        , m_आकार_Constrained_Arraysp{आकार_Constrained_Arraysp}
        , m_prepareConstrainedArraysp{prepareConstrainedArraysp}
        , m_uniqueConstraintId{uniqueConstraintId}
        , m_unique_नामानि{unique_नामानि} {
        // Pre-pass before SMT lowering: extract conditional disable-soft
        // directives as runtime AstIf statements and append them to the
        // constraint-items chain so they reach the setup task body.  The SMT
        // visitor skips these via visit(AstIf) above.
        यदि (Ast_ग्रन्थिः* ध्रुव hoistListp
            = nodep ? extractConditionalDisableSofts(nodep, शून्यसूचकः) : शून्यसूचकः) {
            nodep->सङ्कलय_अग्र(hoistListp);
        }
        iterate_And_अग्र_Null(nodep);
    }
};

गणना वर्ग Capture_बहुलकम् : uint8_t {
    CAP_NO = 0x0,
    CAP_मानम् = 0x01,
    CAP_THIS = 0x02,
    CAP_F_समुच्चयः_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
Capture_बहुलकम् संक्रियकः|(Capture_बहुलकम् a, Capture_बहुलकम् b) {
    प्रत्यागम static_cast<Capture_बहुलकम्>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
Capture_बहुलकम् संक्रियकः&(Capture_बहुलकम् a, Capture_बहुलकम् b) {
    प्रत्यागम static_cast<Capture_बहुलकम्>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
Capture_बहुलकम् बहुलकम्(Capture_बहुलकम् a) { प्रत्यागम a & static_cast<Capture_बहुलकम्>(0x3); }
तार्किक hasFlags(Capture_बहुलकम् a, Capture_बहुलकम् flags) {
    प्रत्यागम ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

वर्ग CaptureVisitor final : सार्वजनिक VNVisitor {
    AstArg* m_argsp = शून्यसूचकः;  // Original references turned into arguments
    Ast_ग्रन्थिः_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<ध्रुव AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::समुच्चयः<Ast_ग्रन्थिः*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = शून्यसूचकः;  // Variable for outer context's object, if necessary
    तार्किक m_staticContext = असत्य;  // True when capturing from a static function context

    // METHODS

    तार्किक captureVariable(File_पङ्क्तिः* ध्रुव fileline, Ast_ग्रन्थिः_Var_Ref* varrefp, AstVar*& varp) {
        स्वचालित it = m_varCloneMap.find(varrefp->varp());
        यदि (it == m_varCloneMap.अन्तः_2()) {
            AstVar* ध्रुव newVarp = varrefp->varp()->clone_वृक्षः(असत्य);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(सत्य);
            newVarp->direction(VDirection::निवेशः);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            प्रत्यागम सत्य;
        }
        varp = it->second;
        प्रत्यागम असत्य;
    }

    प्रतिरूपम् <प्रकारनाम T_ग्रन्थिः>
    शून्य fixupClassOrPackage(Ast_ग्रन्थिः* memberp, T_ग्रन्थिः refp) {
        Ast_ग्रन्थिः_Module* ध्रुव declClassp = VN_AS(memberp->user2p(), ग्रन्थिः_Module);
        यदि (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    प्रतिरूपम् <प्रकारनाम T_ग्रन्थिः>
    तार्किक is_Reference_गन्तव्यम्_Inner_Member(T_ग्रन्थिः nodep) {
        प्रत्यागम VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_पङ्क्तिः* fl) {
        यदि (!m_thisp) {
            AstClassRefDType* ध्रुव refDTypep
                = नूतन AstClassRefDType{fl, VN_AS(m_callerp, Class), शून्यसूचकः};
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(refDTypep);
            m_thisp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(सत्य);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::निवेशः);
            m_argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_argsp, नूतन AstArg{fl, "", नूतन AstThisRef{fl, refDTypep}});
        }
        प्रत्यागम m_thisp;
    }

    AstVar* getVar(AstVar* ध्रुव varp) ध्रुव {
        ध्रुव स्वचालित it = m_varCloneMap.find(varp);
        यदि (it == m_varCloneMap.अन्तः_2()) प्रत्यागम शून्यसूचकः;
        यदि (it->second->isStdRandomizeArg()) प्रत्यागम शून्यसूचकः;
        प्रत्यागम it->second;
    }

    Capture_बहुलकम् get_Var_Ref_Capture_बहुलकम्(Ast_ग्रन्थिः_Var_Ref* varRefp) {
        Ast_ग्रन्थिः_Module* ध्रुव varModp = VN_AS(varRefp->varp()->user2p(), ग्रन्थिः_Module);
        AstClass* ध्रुव varClassp = VN_CAST(varModp, Class);
        AstClass* ध्रुव callerClassp = VN_CAST(m_callerp, Class);

        ध्रुव तार्किक callerIsClass = callerClassp;
        ध्रुव तार्किक refIsXref = VN_IS(varRefp, VarXRef);
        ध्रुव तार्किक varIsFuncLocal = varRefp->varp()->isFuncLocal();
        ध्रुव तार्किक varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        ध्रुव तार्किक var_Is_क्षेत्रम्_सम्बन्धि_Caller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        ध्रुव तार्किक varIsParam = varRefp->varp()->isParam();
        ध्रुव तार्किक varIsConstraintIterator
            = VN_IS(varRefp->varp()->firstAbovep(), Foreach_शीर्षकम्)
              && VN_IS(varRefp->varp()->firstAbovep()->firstAbovep(), ConstraintForeach);
        यदि (refIsXref) प्रत्यागम Capture_बहुलकम्::CAP_मानम् | Capture_बहुलकम्::CAP_F_XREF;
        यदि (varIsConstraintIterator) प्रत्यागम Capture_बहुलकम्::CAP_NO;
        यदि (varIsFuncLocal && varHasAutomaticLifetime) प्रत्यागम Capture_बहुलकम्::CAP_मानम्;
        यदि (varIsParam) प्रत्यागम Capture_बहुलकम्::CAP_मानम्;
        // Static var in function (will not be inlined, because it's in class)
        यदि (callerIsClass && varIsFuncLocal) प्रत्यागम Capture_बहुलकम्::CAP_मानम्;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        यदि (m_staticContext && callerIsClass && var_Is_क्षेत्रम्_सम्बन्धि_Caller
            && varRefp->varp()->lifetime().isStatic())
            प्रत्यागम Capture_बहुलकम्::CAP_NO;
        यदि (callerIsClass && var_Is_क्षेत्रम्_सम्बन्धि_Caller) प्रत्यागम Capture_बहुलकम्::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        यदि (callerIsClass && varClassp && !var_Is_क्षेत्रम्_सम्बन्धि_Caller
            && varRefp->varp()->lifetime().isStatic())
            प्रत्यागम Capture_बहुलकम्::CAP_मानम्;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        प्रत्यागम Capture_बहुलकम्::CAP_मानम्;
    }

    शून्य capture_Ref_By_मानम्(Ast_ग्रन्थिः_Var_Ref* nodep, Capture_बहुलकम् cap_बहुलकम्_Flags) {
        AstVar* newVarp;
        ध्रुव तार्किक newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_ग्रन्थिः_Var_Ref* ध्रुव newVarRefp = newCapture ? nodep->clone_वृक्षः(असत्य) : शून्यसूचकः;
        यदि (!hasFlags(cap_बहुलकम्_Flags, Capture_बहुलकम्::CAP_F_समुच्चयः_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(शून्यसूचकः);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        यदि (!newCapture) प्रत्यागम;
        यदि (hasFlags(cap_बहुलकम्_Flags, Capture_बहुलकम्::CAP_F_XREF)) {
            AstVarRef* ध्रुव notXVarRefp
                = नूतन AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_argsp, नूतन AstArg{nodep->fileline(), "", newVarRefp});
    }

    शून्य captureRefByThis(Ast_ग्रन्थिः_Var_Ref* nodep, Capture_बहुलकम् /*capModeFlags*/) {
        AstVar* ध्रुव thisp = importThisp(nodep->fileline());
        AstVarRef* ध्रुव thisRefp = नूतन AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(सत्य);
        m_ignore.emplace(thisRefp);
        AstMemberSel* ध्रुव memberSelp
            = नूतन AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        यदि (!m_targetp && nodep->user1()) memberSelp->user1(सत्य);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (m_ignore.गणनम्(nodep)) प्रत्यागम;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        यदि (nodep->varp()->isStdRandomizeArg()) प्रत्यागम;
        ध्रुव Capture_बहुलकम् cap_बहुलकम् = get_Var_Ref_Capture_बहुलकम्(nodep);
        यदि (बहुलकम्(cap_बहुलकम्) == Capture_बहुलकम्::CAP_NO) प्रत्यागम;
        यदि (बहुलकम्(cap_बहुलकम्) == Capture_बहुलकम्::CAP_मानम्) capture_Ref_By_मानम्(nodep, cap_बहुलकम्);
        यदि (बहुलकम्(cap_बहुलकम्) == Capture_बहुलकम्::CAP_THIS) captureRefByThis(nodep, cap_बहुलकम्);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        यदि (m_ignore.गणनम्(nodep)) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        यदि (VN_IS(nodep, विधानम्_Call) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            प्रत्यागम;
        }
        AstClass* ध्रुव classp = VN_CAST(nodep->taskp()->user2p(), Class);
        यदि ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* ध्रुव argsp = nodep->argsp();
            यदि (argsp) argsp->unlink_Fr_पृष्ठभागः();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* ध्रुव thisp = importThisp(nodep->fileline());
            AstVarRef* ध्रुव thisRefp = नूतन AstVarRef{
                nodep->fileline(), thisp, nodep->is_विशुद्धम्() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            Ast_विधानम्_Call* ध्रुव विधानम्_Callp
                = नूतन Ast_विधानम्_Call{nodep->fileline(), thisRefp, thisp->name(), argsp};
            विधानम्_Callp->taskp(nodep->taskp());
            विधानम्_Callp->dtypep(nodep->dtypep());
            nodep->replaceWith(विधानम्_Callp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            m_ignore.emplace(विधानम्_Callp);
        }
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        यदि (!is_Reference_गन्तव्यम्_Inner_Member(nodep)) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        AstVarRef* ध्रुव varRefp
            = नूतन AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    शून्य दर्शय(Ast_विधानम्_Call* nodep) override {
        यदि (!is_Reference_गन्तव्यम्_Inner_Member(nodep) || m_ignore.गणनम्(nodep)) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        AstArg* ध्रुव argsp = nodep->argsp();
        यदि (argsp) argsp->unlink_Fr_पृष्ठभागः_With_अग्र();
        Ast_ग्रन्थिः_FTask_Ref* taskRefp = शून्यसूचकः;
        यदि (AstTask* ध्रुव taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = नूतन AstTaskRef{nodep->fileline(), taskp, argsp};
        } अन्यथा यदि (AstFunc* ध्रुव taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = नूतन AstFuncRef{nodep->fileline(), taskp, argsp};
        } अन्यथा {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    स्पष्ट CaptureVisitor(Ast_ग्रन्थिः* ध्रुव nodep, Ast_ग्रन्थिः_Module* callerp, AstClass* ध्रुव targetp,
                            तार्किक staticContext = असत्य)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterate_And_अग्र_Null(nodep);
    }

    // PUBLIC METHODS

    AstArg* get_तर्काः() ध्रुव { प्रत्यागम m_argsp; }
    AstVar* getThisp() ध्रुव { प्रत्यागम m_thisp; }

    शून्य सङ्कलय_Function_Arguments(Ast_ग्रन्थिः_FTask* funcp) ध्रुव {
        प्रति (AstArg* argp = get_तर्काः(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            यदि (Ast_ग्रन्थिः_Var_Ref* varrefp = VN_CAST(argp->exprp(), ग्रन्थिः_Var_Ref)) {
                यदि ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(शून्यसूचकः);
                }
                funcp->सङ्कलय_Stmtsp(getVar(varrefp->varp()));
            } अन्यथा {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->सङ्कलय_Stmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

वर्ग RandomizeVisitor final : सार्वजनिक VNVisitor {
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
    ध्रुव VNUser3_अन्तर्_Use m_inuser3;
    ध्रुव VNUser4_अन्तर्_Use m_inuser4;

    // STATE
    V3_Unique_नामानि m_inline_Unique_नामानि;  // For generating unique function names
    V3_Unique_नामानि m_rand_Array_Unique_नामानि{"__Vrandarr"};  // Random array loop indices
    V3_Unique_नामानि m_बहुलकम्_Unique_नामानि{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3_Unique_नामानि m_inlineUniqueStdName{"__VStdrand"};
    V3_Unique_नामानि m_unique_नामानि;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    std::unordered_map<Ast_ग्रन्थिः_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current function/task
    Ast_ग्रन्थिः_Stmt* m_stmtp = शून्यसूचकः;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = शून्यसूचकः;  // Dynamic array type (for rand mode)
    पूर्णाङ्क m_rand_Case_अङ्कः = 0;  // Randcase number within a module for var naming
    पूर्णाङ्क m_dist_अङ्कः = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = शून्यसूचकः;  // Current constraint
    std::समुच्चयः<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertion_समुच्चयः<AstVar*>> m_आकार_Constrained_Arrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_static_Constraint_बहुलकम्_Vars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_static_Rand_बहुलकम्_Vars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<तार्किक, तार्किक>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    स्थिर तार्किक is_Similar_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* with_व्यञ्जकम्, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* arg_व्यञ्जकम्) {
        // VarRef: compare variable pointers
        यदि (VN_IS(arg_व्यञ्जकम्, VarRef) && VN_IS(with_व्यञ्जकम्, VarRef)) {
            प्रत्यागम VN_AS(with_व्यञ्जकम्, VarRef)->varp() == VN_AS(arg_व्यञ्जकम्, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        यदि (VN_IS(arg_व्यञ्जकम्, MemberSel) && VN_IS(with_व्यञ्जकम्, MemberSel)) {
            ध्रुव AstMemberSel* ध्रुव withMSp = VN_AS(with_व्यञ्जकम्, MemberSel);
            ध्रुव AstMemberSel* ध्रुव argMSp = VN_AS(arg_व्यञ्जकम्, MemberSel);
            यदि (withMSp->varp() != argMSp->varp()) प्रत्यागम असत्य;
            // Recursively compare the base object expression
            प्रत्यागम is_Similar_ग्रन्थिः(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        यदि (VN_IS(arg_व्यञ्जकम्, ArraySel) && VN_IS(with_व्यञ्जकम्, ArraySel)) {
            ध्रुव AstArraySel* ध्रुव withASp = VN_AS(with_व्यञ्जकम्, ArraySel);
            ध्रुव AstArraySel* ध्रुव argASp = VN_AS(arg_व्यञ्जकम्, ArraySel);
            // Before checking Sel type index, check if both are Const
            यदि (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                ध्रुव AstConst* ध्रुव withIdxp = VN_AS(withASp->bitp(), Const);
                ध्रुव AstConst* ध्रुव argIdxp = VN_AS(argASp->bitp(), Const);
                प्रत्यागम is_Similar_ग्रन्थिः(withASp->fromp(), argASp->fromp())
                       && withIdxp->अङ्कः().isCaseEq(argIdxp->अङ्कः());
            }
            // Index must be Sel type, extract VarRef using fromp()
            यदि (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) प्रत्यागम असत्य;
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            प्रत्यागम is_Similar_ग्रन्थिः(withASp->fromp(), argASp->fromp())
                   && is_Similar_ग्रन्थिः(withIdxp, argIdxp);
        }
        प्रत्यागम असत्य;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    स्थिर शून्य expandUniqueElementList(Ast_ग्रन्थिः* itemsp) {
        Ast_ग्रन्थिः* itemp = itemsp;
        यावत् (itemp) {
            Ast_ग्रन्थिः* ध्रुव nextp = itemp->nextp();
            यदि (ध्रुव AstConstraintIf* ध्रुव ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                अनुवर्त;
            }
            यदि (ध्रुव AstConstraintForeach* ध्रुव foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                अनुवर्त;
            }
            AstConstraintUnique* ध्रुव uniquep = VN_CAST(itemp, ConstraintUnique);
            यदि (!uniquep) {
                itemp = nextp;
                अनुवर्त;
            }

            std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> व्यञ्जकम्_वस्तूनि;
            तार्किक hasArrayVarRef = असत्य;
            प्रति (Ast_ग्रन्थिः* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                यदि (ध्रुव AstVarRef* ध्रुव vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = vrp->varp()->dtypep()->skipRefp();
                    यदि (VN_IS(dtypep, ग्रन्थिः_Array_DType) || VN_IS(dtypep, QueueDType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = सत्य;
                        अनुवर्त;
                    }
                }
                व्यञ्जकम्_वस्तूनि.प्रक्षिप_पृष्ठभागः(VN_AS(rp, ग्रन्थिः_व्यञ्जकम्));
            }

            यदि (व्यञ्जकम्_वस्तूनि.आकार() >= 2) {
                File_पङ्क्तिः* ध्रुव fl = uniquep->fileline();
                प्रति (आकार_t i = 0; i < व्यञ्जकम्_वस्तूनि.आकार(); i++) {
                    प्रति (आकार_t j = i + 1; j < व्यञ्जकम्_वस्तूनि.आकार(); j++) {
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = व्यञ्जकम्_वस्तूनि[i]->clone_वृक्षः(असत्य);
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = व्यञ्जकम्_वस्तूनि[j]->clone_वृक्षः(असत्य);
                        AstNeq* ध्रुव neqp = नूतन AstNeq{fl, lhsp, rhsp};
                        neqp->user1(सत्य);
                        Ast_Constraint_व्यञ्जकम्* ध्रुव cexprp = नूतन Ast_Constraint_व्यञ्जकम्{fl, neqp};
                        uniquep->सङ्कलय_अग्र_अत्र(cexprp);
                    }
                }
                यदि (!hasArrayVarRef) {
                    uniquep->unlink_Fr_पृष्ठभागः();
                    VL_DO_DANGLING(uniquep->delete_वृक्षः(), uniquep);
                }
            } अन्यथा यदि (व्यञ्जकम्_वस्तूनि.आकार() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlink_Fr_पृष्ठभागः();
                VL_DO_DANGLING(uniquep->delete_वृक्षः(), uniquep);
            }
            itemp = nextp;
        }
    }
    शून्य createRandomGenerator(AstClass* ध्रुव classp) {
        यदि (classp->user3p()) प्रत्यागम;
        यदि (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            प्रत्यागम;
        }
        AstVar* ध्रुव genp = नूतन AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->सङ्कलय_Membersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_ग्रन्थिः_Module* ध्रुव modp) {
        स्वचालित it = m_stdMap.find(modp);
        यदि (it == m_stdMap.अन्तः_2()) {
            AstVar* ध्रुव stdgenp
                = नूतन AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->findBasicDType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3_Error_संहिता::IMPURE, सत्य);
            modp->सङ्कलय_Stmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            प्रत्यागम stdgenp;
        }
        प्रत्यागम it->second;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* ध्रुव classp) {
        स्थिर ध्रुव वर्ण* ध्रुव name = "__Vprepare_constrained_arrays";
        AstTask* ध्रुव taskp = नूतन AstTask{classp->fileline(), name, शून्यसूचकः};
        taskp->class_विधानम्(सत्य);
        classp->सङ्कलय_Membersp(taskp);
        m_memberMap.निवेशय(classp, taskp);
        प्रत्यागम taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* ध्रुव classp) {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.findMember(classp, name), Task);
        यदि (resizeTaskp) प्रत्यागम resizeTaskp;
        resizeTaskp = नूतन AstTask{classp->fileline(), name, शून्यसूचकः};
        resizeTaskp->class_विधानम्(सत्य);
        resizeTaskp->isVirtual(सत्य);
        classp->सङ्कलय_Membersp(resizeTaskp);
        m_memberMap.निवेशय(classp, resizeTaskp);
        प्रत्यागम resizeTaskp;
    }
    AstVar* get_Create_Rand_बहुलकम्_Var(AstClass* ध्रुव classp) {
        यदि (classp->user2p()) प्रत्यागम VN_AS(classp->user2p(), Var);
        यदि (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            प्रत्यागम get_Create_Rand_बहुलकम्_Var(extendsp->classp());
        }
        AstVar* ध्रुव rand_बहुलकम्_Varp = create_बहुलकम्_Var(classp, "__Vrandmode");
        classp->user2p(rand_बहुलकम्_Varp);
        प्रत्यागम rand_बहुलकम्_Varp;
    }
    AstVar* get_Create_Constraint_बहुलकम्_Var(AstClass* ध्रुव classp) {
        यदि (classp->user4p()) प्रत्यागम VN_AS(classp->user4p(), Var);
        यदि (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            प्रत्यागम get_Create_Constraint_बहुलकम्_Var(extendsp->classp());
        }
        AstVar* ध्रुव constraint_बहुलकम्_Varp = create_बहुलकम्_Var(classp, "__Vconstraintmode");
        classp->user4p(constraint_बहुलकम्_Varp);
        प्रत्यागम constraint_बहुलकम्_Varp;
    }
    स्थिर AstVar* get_Constraint_बहुलकम्_Var(AstClass* ध्रुव classp) {
        यदि (classp->user4p()) प्रत्यागम VN_AS(classp->user4p(), Var);
        यदि (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            प्रत्यागम get_Constraint_बहुलकम्_Var(extendsp->classp());
        }
        प्रत्यागम शून्यसूचकः;
    }
    AstVar* get_Create_Static_Constraint_बहुलकम्_Var(AstClass* ध्रुव classp) {
        स्वचालित it = m_static_Constraint_बहुलकम्_Vars.find(classp);
        यदि (it != m_static_Constraint_बहुलकम्_Vars.अन्तः_2()) प्रत्यागम it->second;
        यदि (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            प्रत्यागम get_Create_Static_Constraint_बहुलकम्_Var(extendsp->classp());
        }
        AstVar* ध्रुव static_बहुलकम्_Varp = create_Static_बहुलकम्_Var(classp, "__Vstaticconstraintmode");
        m_static_Constraint_बहुलकम्_Vars[classp] = static_बहुलकम्_Varp;
        प्रत्यागम static_बहुलकम्_Varp;
    }
    AstVar* get_Static_Constraint_बहुलकम्_Var(AstClass* ध्रुव classp) {
        स्वचालित it = m_static_Constraint_बहुलकम्_Vars.find(classp);
        यदि (it != m_static_Constraint_बहुलकम्_Vars.अन्तः_2()) प्रत्यागम it->second;
        यदि (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            प्रत्यागम get_Static_Constraint_बहुलकम्_Var(extendsp->classp());
        }
        प्रत्यागम शून्यसूचकः;
    }
    AstVar* get_Create_Static_Rand_बहुलकम्_Var(AstClass* ध्रुव classp) {
        यदि (m_static_Rand_बहुलकम्_Vars.गणनम्(classp)) प्रत्यागम m_static_Rand_बहुलकम्_Vars[classp];
        यदि (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            प्रत्यागम get_Create_Static_Rand_बहुलकम्_Var(extendsp->classp());
        }
        AstVar* ध्रुव static_बहुलकम्_Varp = create_Static_बहुलकम्_Var(classp, "__Vstaticrandmode");
        m_static_Rand_बहुलकम्_Vars[classp] = static_बहुलकम्_Varp;
        प्रत्यागम static_बहुलकम्_Varp;
    }
    AstVar* get_Static_Rand_बहुलकम्_Var(AstClass* ध्रुव classp) {
        यदि (m_static_Rand_बहुलकम्_Vars.गणनम्(classp)) प्रत्यागम m_static_Rand_बहुलकम्_Vars[classp];
        यदि (AstClassExtends* ध्रुव extendsp = classp->extendsp()) {
            प्रत्यागम get_Static_Rand_बहुलकम्_Var(extendsp->classp());
        }
        प्रत्यागम शून्यसूचकः;
    }
    AstVar* create_बहुलकम्_Var(AstClass* ध्रुव classp, ध्रुव वर्ण* ध्रुव name) {
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        यदि (!m_dynarrayDtp) {
            m_dynarrayDtp = नूतन AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* ध्रुव बहुलकम्_Varp = नूतन AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        बहुलकम्_Varp->user2p(classp);
        classp->सङ्कलय_Stmtsp(बहुलकम्_Varp);
        प्रत्यागम बहुलकम्_Varp;
    }
    AstVar* create_Static_बहुलकम्_Var(AstClass* ध्रुव classp, ध्रुव वर्ण* ध्रुव name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        यदि (!m_dynarrayDtp) {
            m_dynarrayDtp = नूतन AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(m_dynarrayDtp);
        }
        AstVar* ध्रुव बहुलकम्_Varp = नूतन AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        बहुलकम्_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        बहुलकम्_Varp->user2p(classp);
        classp->सङ्कलय_Stmtsp(बहुलकम्_Varp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.निवेशय(classp, बहुलकम्_Varp);
        प्रत्यागम बहुलकम्_Varp;
    }
    स्थिर शून्य सङ्कलय_समुच्चयः_Rand_बहुलकम्(Ast_ग्रन्थिः_FTask* ध्रुव ftaskp, AstVar* ध्रुव genp,
                               AstVar* ध्रुव rand_बहुलकम्_Varp) {
        File_पङ्क्तिः* ध्रुव fl = ftaskp->fileline();
        AstCMethodHard* ध्रुव समुच्चयः_Rand_Modep = नूतन AstCMethodHard{
            fl, नूतन AstVarRef{fl, VN_AS(genp->user2p(), ग्रन्थिः_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_समुच्चयः_RANDMODE,
            नूतन AstVarRef{fl, VN_AS(rand_बहुलकम्_Varp->user2p(), ग्रन्थिः_Module), rand_बहुलकम्_Varp,
                          VAccess::READ}};
        समुच्चयः_Rand_Modep->dtype_समुच्चयः_Void();
        ftaskp->सङ्कलय_Stmtsp(समुच्चयः_Rand_Modep->makeStmt());
    }
    स्थिर शून्य सङ्कलय_समुच्चयः_Static_Rand_बहुलकम्(Ast_ग्रन्थिः_FTask* ध्रुव ftaskp, AstVar* ध्रुव genp,
                                     AstVar* ध्रुव static_Rand_बहुलकम्_Varp) {
        File_पङ्क्तिः* ध्रुव fl = ftaskp->fileline();
        AstCMethodHard* ध्रुव setp = नूतन AstCMethodHard{
            fl, नूतन AstVarRef{fl, VN_AS(genp->user2p(), ग्रन्थिः_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_समुच्चयः_STATIC_RANDMODE,
            नूतन AstVarRef{fl, VN_AS(static_Rand_बहुलकम्_Varp->user2p(), ग्रन्थिः_Module), static_Rand_बहुलकम्_Varp,
                          VAccess::READ}};
        setp->dtype_समुच्चयः_Void();
        ftaskp->सङ्कलय_Stmtsp(setp->makeStmt());
    }
    शून्य createRandomizeClassVars(AstNetlist* ध्रुव netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> मूलम्_Static_Rand_बहुलकम्_गणनम्;
        netlistp->foreach([एतत्, &मूलम्_Static_Rand_बहुलकम्_गणनम्](AstClass* ध्रुव classp) {
            तार्किक hasConstraints = असत्य;
            uint32_t rand_बहुलकम्_गणनम् = 0;
            uint32_t static_Rand_बहुलकम्_गणनम् = 0;
            uint32_t constraint_बहुलकम्_गणनम् = 0;
            uint32_t static_Constraint_बहुलकम्_गणनम् = 0;
            classp->foreachMember([&](AstClass*, Ast_ग्रन्थिः* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                यदि (AstConstraint* ध्रुव constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = सत्य;
                    Randomize_बहुलकम् constraint_बहुलकम् = {.asUQuad = memberp->user1()};
                    यदि (!constraint_बहुलकम्.उपयुङ्क्ते_बहुलकम्) प्रत्यागम;
                    यदि (constraint_बहुलकम्.सूचकांक == 0) {
                        // Use separate index counters for static vs non-static constraints
                        यदि (constrp->isStatic()) {
                            constraint_बहुलकम्.सूचकांक = static_Constraint_बहुलकम्_गणनम्++;
                        } अन्यथा {
                            constraint_बहुलकम्.सूचकांक = constraint_बहुलकम्_गणनम्++;
                        }
                        memberp->user1(constraint_बहुलकम्.asUQuad);
                    } अन्यथा {
                        यदि (constrp->isStatic()) {
                            static_Constraint_बहुलकम्_गणनम् = constraint_बहुलकम्.सूचकांक + 1;
                        } अन्यथा {
                            constraint_बहुलकम्_गणनम् = constraint_बहुलकम्.सूचकांक + 1;
                        }
                    }
                } अन्यथा यदि (AstVar* ध्रुव varp = VN_CAST(memberp, Var)) {
                    Randomize_बहुलकम् rand_बहुलकम् = {.asUQuad = memberp->user1()};
                    यदि (!rand_बहुलकम्.उपयुङ्क्ते_बहुलकम्) प्रत्यागम;
                    ध्रुव तार्किक isStaticVar = varp->lifetime().isStatic();
                    यदि (rand_बहुलकम्.सूचकांक == 0) {
                        यदि (isStaticVar) {
                            rand_बहुलकम्.सूचकांक = static_Rand_बहुलकम्_गणनम्++;
                        } अन्यथा {
                            rand_बहुलकम्.सूचकांक = rand_बहुलकम्_गणनम्++;
                        }
                        memberp->user1(rand_बहुलकम्.asUQuad);
                    } अन्यथा {
                        यदि (isStaticVar) {
                            static_Rand_बहुलकम्_गणनम् = rand_बहुलकम्.सूचकांक + 1;
                        } अन्यथा {
                            rand_बहुलकम्_गणनम् = rand_बहुलकम्.सूचकांक + 1;
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
                        यदि (AstVar* ध्रुव subVarp = VN_CAST(subMemberp, Var)) {
                            ध्रुव Randomize_बहुलकम् rm = {.asUQuad = subVarp->user1()};
                            यदि (!rm.उपयुङ्क्ते_बहुलकम्) प्रत्यागम;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            यदि (subVarp->lifetime().isStatic()) प्रत्यागम;
                            ध्रुव uint32_t needed = rm.सूचकांक + 1;
                            यदि (needed > rand_बहुलकम्_गणनम्) rand_बहुलकम्_गणनम् = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    यदि (!memberVarp->globalConstrained()) प्रत्यागम;
                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = memberVarp->dtypep()->skipRefp();
                    ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
                    यदि (!classRefp) प्रत्यागम;
                    findSubObjRandModes(classRefp->classp());
                });
            }
            यदि (hasConstraints) createRandomGenerator(classp);
            यदि (rand_बहुलकम्_गणनम् > 0) {
                AstVar* ध्रुव rand_बहुलकम्_Varp = get_Create_Rand_बहुलकम्_Var(classp);
                make_बहुलकम्_Init(rand_बहुलकम्_Varp, classp, rand_बहुलकम्_गणनम्);
            }
            यदि (constraint_बहुलकम्_गणनम् > 0) {
                AstVar* ध्रुव constraint_बहुलकम्_Varp = get_Create_Constraint_बहुलकम्_Var(classp);
                make_बहुलकम्_Init(constraint_बहुलकम्_Varp, classp, constraint_बहुलकम्_गणनम्);
            }
            यदि (static_Constraint_बहुलकम्_गणनम् > 0) {
                AstVar* ध्रुव static_Constraint_बहुलकम्_Varp = get_Create_Static_Constraint_बहुलकम्_Var(classp);
                make_Static_बहुलकम्_Init(static_Constraint_बहुलकम्_Varp, classp, static_Constraint_बहुलकम्_गणनम्);
            }
            यदि (static_Rand_बहुलकम्_गणनम् > 0) {
                get_Create_Static_Rand_बहुलकम्_Var(classp);
                AstClass* rootp = classp;
                यावत् (AstClassExtends* ध्रुव ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& slot = मूलम्_Static_Rand_बहुलकम्_गणनम्[rootp];
                यदि (static_Rand_बहुलकम्_गणनम् > slot) slot = static_Rand_बहुलकम्_गणनम्;
            }
        });
        प्रति (ध्रुव स्वचालित& kv : मूलम्_Static_Rand_बहुलकम्_गणनम्) emit_मूलम्_Static_बहुलकम्_Init(kv.first, kv.second);
    }
    शून्य emit_मूलम्_Static_बहुलकम्_Init(AstClass* ध्रुव rootp, ध्रुव uint32_t गणनम्) {
        AstVar* ध्रुव static_Rand_बहुलकम्_Varp = m_static_Rand_बहुलकम्_Vars[rootp];
        UASSERT_OBJ(static_Rand_बहुलकम्_Varp, rootp, "Root must have a static rand-mode var");
        make_Static_बहुलकम्_Init(static_Rand_बहुलकम्_Varp, rootp, गणनम्);
    }
    शून्य make_बहुलकम्_Init(AstVar* बहुलकम्_Varp, AstClass* classp, uint32_t बहुलकम्_गणनम्) {
        Ast_ग्रन्थिः_Module* ध्रुव बहुलकम्_Var_Modp = VN_AS(बहुलकम्_Varp->user2p(), ग्रन्थिः_Module);
        File_पङ्क्तिः* ध्रुव fl = बहुलकम्_Varp->fileline();
        AstVar* ध्रुव old_आकार_Varp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, m_unique_नामानि.get("__VoldSize"), बहुलकम्_Varp->findUInt32DType()};
        old_आकार_Varp->funcLocal(सत्य);
        old_आकार_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ध्रुव oldSizep
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, बहुलकम्_Var_Modp, बहुलकम्_Varp, VAccess::READ},
                                 VCMethod::DYN_आकार, शून्यसूचकः};
        oldSizep->dtype_समुच्चयः_UInt32();
        AstCMethodHard* ध्रुव dynarrayNewp
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, बहुलकम्_Var_Modp, बहुलकम्_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, नूतन AstConst{fl, बहुलकम्_गणनम्}};
        dynarrayNewp->dtype_समुच्चयः_Void();
        Ast_ग्रन्थिः_FTask* ध्रुव ctorNewp = VN_AS(m_memberMap.findMember(classp, "new"), ग्रन्थिः_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_ग्रन्थिः* ध्रुव initFirstp = old_आकार_Varp;
        initFirstp->सङ्कलय_अग्र(
            नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, old_आकार_Varp, VAccess::WRITE}, oldSizep});
        initFirstp->सङ्कलय_अग्र(dynarrayNewp->makeStmt());
        initFirstp->सङ्कलय_अग्र(make_बहुलकम्_समुच्चयः_Loop(
            fl, नूतन AstVarRef{fl, बहुलकम्_Var_Modp, बहुलकम्_Varp, VAccess::WRITE}, नूतन AstConst{fl, 1},
            सत्य, नूतन AstVarRef{fl, old_आकार_Varp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        प्रति (Ast_ग्रन्थिः* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (!VN_IS(stmtp, ग्रन्थिः_Stmt)) अनुवर्त;  // Skip var declarations
            यदि (ध्रुव Ast_Stmt_व्यञ्जकम्* ध्रुव sep = VN_CAST(stmtp, Stmt_व्यञ्जकम्)) {
                यदि (VN_IS(sep->exprp(), New)) अनुवर्त;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->सङ्कलय_अत्र_This_As_अग्र(initFirstp);
            प्रत्यागम;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->सङ्कलय_Stmtsp(initFirstp);
    }
    शून्य make_Static_बहुलकम्_Init(AstVar* बहुलकम्_Varp, AstClass* classp, uint32_t बहुलकम्_गणनम्) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_ग्रन्थिः_Module* ध्रुव बहुलकम्_Var_Modp = VN_AS(बहुलकम्_Varp->user2p(), ग्रन्थिः_Module);
        File_पङ्क्तिः* ध्रुव fl = बहुलकम्_Varp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* ध्रुव sizep
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, बहुलकम्_Var_Modp, बहुलकम्_Varp, VAccess::READ},
                                 VCMethod::DYN_आकार, शून्यसूचकः};
        sizep->dtype_समुच्चयः_UInt32();
        AstEq* ध्रुव condp = नूतन AstEq{fl, sizep, नूतन AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* ध्रुव dynarrayNewp
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, बहुलकम्_Var_Modp, बहुलकम्_Varp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, नूतन AstConst{fl, बहुलकम्_गणनम्}};
        dynarrayNewp->dtype_समुच्चयः_Void();
        Ast_ग्रन्थिः* ध्रुव thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->सङ्कलय_अग्र(
            make_बहुलकम्_समुच्चयः_Loop(fl, नूतन AstVarRef{fl, बहुलकम्_Var_Modp, बहुलकम्_Varp, VAccess::WRITE},
                            नूतन AstConst{fl, 1}, सत्य));

        // Build the if statement
        AstIf* ध्रुव ifp = नूतन AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(m_memberMap.findMember(classp, "new"), ग्रन्थिः_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->सङ्कलय_Stmtsp(ifp);
    }
    Ast_ग्रन्थिः* make_बहुलकम्_समुच्चयः_Loop(File_पङ्क्तिः* ध्रुव fl, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp,
                             तार्किक अन्तर्_Task, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव startp = शून्यसूचकः) {
        AstVar* ध्रुव iterVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->findUInt32DType()};
        iterVarp->funcLocal(अन्तर्_Task);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* ध्रुव sizep = नूतन AstCMethodHard{fl, lhsp, VCMethod::DYN_आकार, शून्यसूचकः};
        sizep->dtype_समुच्चयः_UInt32();
        AstCMethodHard* ध्रुव setp
            = नूतन AstCMethodHard{fl, lhsp->clone_वृक्षः(असत्य), VCMethod::ARRAY_AT_WRITE,
                                 नूतन AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtype_समुच्चयः_UInt32();
        Ast_ग्रन्थिः* ध्रुव stmtsp = iterVarp;
        stmtsp->सङ्कलय_अग्र(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : नूतन AstConst{fl, 0}});

        AstLoop* ध्रुव loopp = नूतन AstLoop{fl};
        stmtsp->सङ्कलय_अग्र(loopp);
        loopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{
            fl, loopp, नूतन AstLt{fl, नूतन AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, setp, rhsp});
        loopp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
            fl, नूतन AstVarRef{fl, iterVarp, VAccess::WRITE},
            नूतन Ast_सङ्कलय{fl, नूतन AstConst{fl, 1}, नूतन AstVarRef{fl, iterVarp, VAccess::READ}}});
        प्रत्यागम नूतन AstBegin{fl, m_unique_नामानि.get("__VrandBlock"), stmtsp, सत्य};
    }
    शून्य सङ्कलय_अस्थायी_Varsp(AstVar* varsp) {
        प्रति (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            यदि (m_ftaskp) {
                varp->funcLocal(सत्य);
            } अन्यथा {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        यदि (!m_ftaskp) {
            m_modp->सङ्कलय_Stmtsp(varsp);
        } अन्यथा {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_ग्रन्थिः_Stmt* wrap_If_Rand_बहुलकम्(AstClass* classp, AstVar* ध्रुव varp, Ast_ग्रन्थिः_Stmt* stmtp) {
        ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = varp->user1()};
        AstVar* ध्रुव बहुलकम्_Varp = varp->lifetime().isStatic() ? get_Static_Rand_बहुलकम्_Var(classp)
                                                             : get_Rand_बहुलकम्_Var_From_Class(classp);
        प्रत्यागम VN_AS(wrap_If_बहुलकम्(rmode, बहुलकम्_Varp, stmtp), ग्रन्थिः_Stmt);
    }
    Ast_ग्रन्थिः* wrap_If_Constraint_बहुलकम्(AstClass* classp, AstConstraint* ध्रुव constrp, Ast_ग्रन्थिः* stmtp) {
        ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = constrp->user1()};
        AstVar* ध्रुव बहुलकम्_Varp = constrp->isStatic() ? get_Static_Constraint_बहुलकम्_Var(classp)
                                                     : get_Constraint_बहुलकम्_Var(classp);
        प्रत्यागम wrap_If_बहुलकम्(rmode, बहुलकम्_Varp, stmtp);
    }
    स्थिर Ast_ग्रन्थिः* wrap_If_बहुलकम्(ध्रुव Randomize_बहुलकम् बहुलकम्, AstVar* बहुलकम्_Varp, Ast_ग्रन्थिः* stmtp) {
        File_पङ्क्तिः* ध्रुव fl = stmtp->fileline();
        यदि (बहुलकम्.उपयुङ्क्ते_बहुलकम्) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_ग्रन्थिः_Module* ध्रुव modp = VN_AS(बहुलकम्_Varp->user2p(), ग्रन्थिः_Module);
            AstCMethodHard* ध्रुव atp
                = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, modp, बहुलकम्_Varp, VAccess::READ},
                                     VCMethod::ARRAY_AT, नूतन AstConst{fl, बहुलकम्.सूचकांक}};
            atp->dtype_समुच्चयः_UInt32();
            प्रत्यागम नूतन AstIf{fl, atp, stmtp};
        }
        प्रत्यागम stmtp;
    }
    AstVarRef* enum_मानम्_Tab_Refp(AstEnumDType* ध्रुव nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        यदि (nodep->user2p()) प्रत्यागम VN_AS(nodep->user2p(), VarRef)->clone_वृक्षः(असत्य);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_ग्रन्थिः_Array_DType* ध्रुव vardtypep = नूतन AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            नूतन AstRange{nodep->fileline(), static_cast<पूर्णाङ्क>(nodep->item_गणनम्()) - 1, 0}};
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(vardtypep);
        AstConsArray* ध्रुव consp = नूतन AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t सूचकांक = 0;
        प्रति (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* ध्रुव vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(सूचकांक++, vconstp->clone_वृक्षः(असत्य));
        }
        // Share identical tables via the constant pool
        AstVarRef* ध्रुव refp = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_वृक्षः(), consp);  // V3ConstPool::find clones it
        प्रक्षिप_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        प्रत्यागम refp->clone_वृक्षः(असत्य);
    }

    AstCDType* findVlRandCDType(File_पङ्क्तिः* ध्रुव fl, uint64_t वस्तूनि) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        ध्रुव std::string type = AstCDType::type_गन्तव्यम्_Hold(V3_सङ्ख्या::log2bQuad(वस्तूनि) + 1);
        ध्रुव std::string name = "VlRandC<" + type + ", " + cvt_गन्तव्यम्_Str(वस्तूनि) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        ध्रुव स्वचालित pair = m_randcDtypes.emplace(name, शून्यसूचकः);
        यदि (pair.second) {
            AstCDType* ध्रुव newp = नूतन AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newp);
            pair.first->second = newp;
        }
        प्रत्यागम pair.first->second;
    }

    AstVar* newRandcVarsp(AstVar* ध्रुव varp, AstClass* ध्रुव classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        यदि (!varp->isRandC()) प्रत्यागम शून्यसूचकः;

        uint64_t वस्तूनि = 0;
        यदि (AstEnumDType* ध्रुव enumDtp = VN_CAST(varp->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType)) {
            वस्तूनि = static_cast<uint64_t>(enumDtp->item_गणनम्());
        } अन्यथा यदि (AstBasicDType* ध्रुव basicp = varp->dtypep()->skipRefp()->basicp()) {
            यदि (basicp->विस्तारः() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                यदि (!V3Error::error_गणनम्()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->विस्तारः() << " bits");
                }
                varp->यादृच्छिकम्(VRandAttr::RAND);
                प्रत्यागम शून्यसूचकः;
            }
            वस्तूनि = 1ULL << basicp->विस्तारः();
        } अन्यथा यदि (AstStructDType* ध्रुव dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            प्रत्यागम शून्यसूचकः;
        } अन्यथा {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_गणनम्(), varp, "Unexpected randc variable dtype");
            प्रत्यागम शून्यसूचकः;
        }
        AstCDType* ध्रुव newdtp = findVlRandCDType(varp->fileline(), वस्तूनि);
        AstVar* ध्रुव newp
            = नूतन AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->is_आन्तरिकम्(सत्य);
        classp->सङ्कलय_Stmtsp(newp);
        UINFO(9, "created " << varp);
        प्रत्यागम newp;
    }
    Ast_ग्रन्थिः_Stmt* createArrayForeachLoop(File_पङ्क्तिः* ध्रुव fl, Ast_ग्रन्थिः_DType* ध्रुव dtypep,
                                        Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar* ध्रुव outputVarp) {
        Ast_ग्रन्थिः_DType* अस्थायी_DTypep = dtypep;
        AstVar* randLoopIndxp = शून्यसूचकः;
        स्वचालित create_Loop_सूचकांक = [&](Ast_ग्रन्थिः_DType* अस्थायी_DTypep) {
            यदि (VN_IS(अस्थायी_DTypep, AssocArrayDType)) {
                प्रत्यागम नूतन AstVar{fl, VVarType::VAR, m_rand_Array_Unique_नामानि.get(""),
                                  VN_AS(अस्थायी_DTypep, AssocArrayDType)->कुञ्चिका_DTypep()};
            }
            प्रत्यागम नूतन AstVar{fl, VVarType::VAR, m_rand_Array_Unique_नामानि.get(""),
                              dtypep->findBasicDType(VBasicDTypeKwd::UINT32)};
        };
        Ast_ग्रन्थिः_व्यञ्जकम्* अस्थायी_Elementp = शून्यसूचकः;
        यावत् (अस्थायी_DTypep->isNonPackedArray()) {
            AstVar* ध्रुव newRandLoopIndxp = create_Loop_सूचकांक(अस्थायी_DTypep);
            randLoopIndxp = Ast_ग्रन्थिः::सङ्कलय_अग्र(randLoopIndxp, newRandLoopIndxp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव अस्थायी_Exprp = अस्थायी_Elementp ? अस्थायी_Elementp : exprp;
            AstVarRef* ध्रुव अस्थायी_Refp = नूतन AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            यदि (VN_IS(अस्थायी_DTypep, DynArrayDType)) {
                अस्थायी_Elementp
                    = नूतन AstCMethodHard{fl, अस्थायी_Exprp, VCMethod::ARRAY_AT_WRITE, अस्थायी_Refp};
            } अन्यथा यदि (VN_IS(अस्थायी_DTypep, UnpackArrayDType)) {
                Ast_ग्रन्थिः_Array_DType* ध्रुव aryDTypep = VN_CAST(अस्थायी_DTypep, ग्रन्थिः_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                अस्थायी_Elementp = नूतन AstArraySel{
                    fl, अस्थायी_Exprp,
                    नूतन AstSel{
                        fl,
                        नूतन AstSub{fl, अस्थायी_Refp,
                                   नूतन AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        नूतन AstConst{fl, 0}, V3_सङ्ख्या::log2b(aryDTypep->hi()) + 1}};
            } अन्यथा यदि (VN_IS(अस्थायी_DTypep, AssocArrayDType)) {
                अस्थायी_Elementp = नूतन AstAssocSel{fl, अस्थायी_Exprp, अस्थायी_Refp};
            } अन्यथा यदि (VN_IS(अस्थायी_DTypep, QueueDType)) {
                अस्थायी_Elementp
                    = नूतन AstCMethodHard{fl, अस्थायी_Exprp, VCMethod::DYN_AT_WRITE_APPEND, अस्थायी_Refp};
            }
            अस्थायी_Elementp->dtypep(अस्थायी_DTypep->subDTypep());
            अस्थायी_DTypep = अस्थायी_DTypep->virtRefDTypep();
        }

        Ast_Foreach_शीर्षकम्* ध्रुव headerp
            = नूतन Ast_Foreach_शीर्षकम्{fl, exprp->clone_वृक्षः(असत्य), randLoopIndxp};
        Ast_ग्रन्थिः_Stmt* ध्रुव randStmtsp = newRandStmtsp(fl, अस्थायी_Elementp, शून्यसूचकः, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        यदि (!अस्थायी_Elementp->backp()) VL_DO_DANGLING(प्रक्षिप_Deletep(अस्थायी_Elementp), अस्थायी_Elementp);
        प्रत्यागम नूतन AstForeach{fl, headerp, randStmtsp};
    }
    Ast_ग्रन्थिः_Stmt* newRandStmtsp(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar* randcVarp,
                               AstVar* ध्रुव outputVarp, पूर्णाङ्क विस्थापनम् = 0,
                               AstMemberDType* memberp = शून्यसूचकः) {
        Ast_ग्रन्थिः_DType* ध्रुव memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        यदि (ध्रुव स्वचालित* ध्रुव structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_ग्रन्थिः_Stmt* stmtsp = शून्यसूचकः;
            यदि (structDtp->packed()) विस्थापनम् += memberp ? memberp->lsb() : 0;
            प्रति (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_ग्रन्थिः_Stmt* randp = शून्यसूचकः;
                यदि (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_वृक्षः(असत्य) : exprp, शून्यसूचकः,
                                          outputVarp, विस्थापनम्, smemberp);
                } अन्यथा यदि (smemberp->यादृच्छिकम्().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = नूतन AstStructSel{fl, exprp->clone_वृक्षः(असत्य), smemberp->name()};
                    structSelp->dtypep(smemberp->childDTypep());
                    यदि (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, शून्यसूचकः, outputVarp);
                }
                stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(stmtsp, randp);
            }
            प्रत्यागम stmtsp;
        } अन्यथा यदि (ध्रुव स्वचालित* ध्रुव unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            यदि (!unionDtp->packed()) प्रत्यागम शून्यसूचकः;
            AstMemberDType* ध्रुव firstMemberp = unionDtp->membersp();
            प्रत्यागम newRandStmtsp(fl, exprp, शून्यसूचकः, outputVarp, विस्थापनम्, firstMemberp);
        } अन्यथा यदि (ध्रुव AstClassRefDType* ध्रुव classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* ध्रुव memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            Ast_विधानम्_Call* ध्रुव callp = नूतन Ast_विधानम्_Call{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{
                fl, नूतन AstVarRef{fl, outputVarp, VAccess::WRITE},
                नूतन AstAnd{fl, नूतन AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            प्रत्यागम नूतन AstIf{
                fl, नूतन AstNeq{fl, exprp->clone_वृक्षः(असत्य), नूतन AstConst{fl, AstConst::Null{}}},
                assignp};
        } अन्यथा यदि (AstDynArrayDType* ध्रुव dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            प्रत्यागम createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } अन्यथा यदि (AstQueueDType* ध्रुव queueDtp = VN_CAST(memberDtp, QueueDType)) {
            प्रत्यागम createArrayForeachLoop(fl, queueDtp, exprp, outputVarp);
        } अन्यथा यदि (AstUnpackArrayDType* ध्रुव unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            प्रत्यागम createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } अन्यथा यदि (AstAssocArrayDType* ध्रुव assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            प्रत्यागम createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } अन्यथा {
            Ast_ग्रन्थिः_व्यञ्जकम्* valp;
            यदि (AstEnumDType* ध्रुव enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* ध्रुव tabRefp = enum_मानम्_Tab_Refp(enumDtp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव randp
                    = new_Rand_मानम्(fl, randcVarp, exprp->findBasicDType(VBasicDTypeKwd::UINT32));
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव moddivp = नूतन AstModDiv{
                    fl, randp, नूतन AstConst{fl, static_cast<uint32_t>(enumDtp->item_गणनम्())}};
                moddivp->dtypep(enumDtp);
                valp = नूतन AstArraySel{fl, tabRefp, moddivp};
            } अन्यथा {
                valp
                    = new_Rand_मानम्(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            Ast_नियोजय* assignp
                = नूतन Ast_नियोजय{fl,
                                नूतन AstSel{fl, exprp, विस्थापनम् + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->विस्तारः() : exprp->विस्तारः()},
                                valp};
            AstVar* varp = शून्यसूचकः;
            exprp->exists([&](ध्रुव AstVarRef* varrefp) {
                यदि (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                प्रत्यागम varp != शून्यसूचकः;
            });
            प्रत्यागम wrap_If_Rand_बहुलकम्(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* new_Rand_मानम्(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव randcVarp,
                              Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        यदि (randcVarp) {
            AstCExpr* ध्रुव cexprp = नूतन AstCExpr{fl};
            cexprp->सङ्कलय(नूतन AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->सङ्कलय(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            प्रत्यागम cexprp;
        }

        प्रत्यागम नूतन AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* findPrePostTask(AstClass* classp, ध्रुव string& name) {
        प्रति (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : शून्यसूचकः) {
            यदि (AstTask* ध्रुव taskp = VN_CAST(m_memberMap.findMember(cp, name), Task)) {
                प्रत्यागम taskp;
            }
        }
        प्रत्यागम शून्यसूचकः;
    }
    शून्य सङ्कलय_Pre_Post_Call(AstClass* ध्रुव classp, Ast_ग्रन्थिः_FTask* ध्रुव funcp, ध्रुव string& name) {
        यदि (AstTask* ध्रुव userFuncp = findPrePostTask(classp, name)) {
            AstTaskRef* ध्रुव callp = नूतन AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->सङ्कलय_Stmtsp(callp->makeStmt());
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
        यदि (AstTask* ध्रुव existingp = VN_CAST(m_memberMap.findMember(classp, name), Task)) {
            प्रत्यागम existingp;
        }
        AstTask* ध्रुव taskp = नूतन AstTask{classp->fileline(), name, शून्यसूचकः};
        taskp->class_विधानम्(सत्य);
        taskp->isVirtual(classp->isExtended());
        classp->सङ्कलय_Membersp(taskp);
        m_memberMap.निवेशय(classp, taskp);
        सङ्कलय_Pre_Post_Call(classp, taskp, which);
        प्रत्यागम taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<तार्किक, तार्किक> buildPrePostVirtualWrappers(AstClass* ध्रुव classp) {
        ध्रुव स्वचालित cachedIt = m_prePostWrap.find(classp);
        यदि (cachedIt != m_prePostWrap.अन्तः_2()) प्रत्यागम cachedIt->second;
        std::सदिशम्<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            यदि (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.प्रक्षिप_पृष्ठभागः(subp);
        });
        तार्किक hasPre = असत्य;
        तार्किक hasPost = असत्य;
        प्रति (AstClass* ध्रुव cp : hierp) {
            यदि (findPrePostTask(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = सत्य;
            }
            यदि (findPrePostTask(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = सत्य;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        यदि (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        यदि (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        ध्रुव std::pair<तार्किक, तार्किक> परिणाम{hasPre, hasPost};
        m_prePostWrap.emplace(classp, परिणाम);
        प्रत्यागम परिणाम;
    }
    शून्य सङ्कलय_Virtual_Pre_Post_Call(AstFunc* ध्रुव randomizeFuncp, AstClass* ध्रुव classp,
                               ध्रुव string& which) {
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        AstTask* ध्रुव wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* ध्रुव refDTypep = नूतन AstClassRefDType{fl, classp, शून्यसूचकः};
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(refDTypep);
        Ast_विधानम्_Call* ध्रुव callp
            = नूतन Ast_विधानम्_Call{fl, नूतन AstThisRef{fl, refDTypep}, wrapperp->name(), शून्यसूचकः};
        callp->taskp(wrapperp);
        callp->dtype_समुच्चयः_Void();
        randomizeFuncp->सङ्कलय_Stmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    तार्किक classHasRandClassMembers(AstClass* classp) {
        प्रत्यागम classp->existsMember([](ध्रुव AstClass*, ध्रुव AstVar* varp) {
            यदि (!varp->यादृच्छिकम्().isRandomizable()) प्रत्यागम असत्य;
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->dtypep()->skipRefp();
            प्रत्यागम VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    तार्किक classOwnsGlobalConstraint(ध्रुव AstClass* classp) ध्रुव {
        प्रत्यागम classp->existsMember([](ध्रुव AstClass*, ध्रुव AstConstraint* constrp) {
            तार्किक owns = असत्य;
            constrp->foreach([&](ध्रुव AstMemberSel* memberSelp) {
                ध्रुव Ast_ग्रन्थिः* ध्रुव rootp
                    = const_cast<AstMemberSel*>(memberSelp)->आधारः_Fromp(सत्य);
                यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    ध्रुव Ast_ग्रन्थिः_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    यावत् (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    यदि (VN_IS(dtypep, ClassRefDType)) owns = सत्य;
                }
            });
            प्रत्यागम owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, ध्रुव string& suffix) {
        ध्रुव string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.findMember(classp, name), Task);
        यदि (taskp) प्रत्यागम taskp;
        taskp = नूतन AstTask{classp->fileline(), name, शून्यसूचकः};
        taskp->class_विधानम्(सत्य);
        classp->सङ्कलय_Membersp(taskp);
        m_memberMap.निवेशय(classp, taskp);
        प्रत्यागम taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    शून्य populateNestedCallbackTask(AstTask* ध्रुव callbackTaskp, AstClass* ध्रुव classp,
                                    ध्रुव string& cbName) {
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            यदि (!memberVarp->यादृच्छिकम्().isRandomizable()) प्रत्यागम;
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = memberVarp->dtypep()->skipRefp();
            ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType);
            यदि (!classRefp) प्रत्यागम;
            AstClass* ध्रुव memberClassp = classRefp->classp();
            यदि (memberClassp == classp) प्रत्यागम;  // Avoid self-reference

            // Force-visit member class if not yet processed
            यदि (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_ग्रन्थिः* stmtsp = शून्यसूचकः;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            यदि (AstTask* ध्रुव userFuncp = findPrePostTask(memberClassp, cbName)) {
                Ast_विधानम्_Call* ध्रुव callp = नूतन Ast_विधानम्_Call{
                    fl, नूतन AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtype_समुच्चयः_Void();
                stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            यदि (classHasRandClassMembers(memberClassp)) {
                ध्रुव string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* ध्रुव nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                Ast_विधानम्_Call* ध्रुव recurseCallp = नूतन Ast_विधानम्_Call{
                    fl, नूतन AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtype_समुच्चयः_Void();
                stmtsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(stmtsp, recurseCallp->makeStmt());
            }

            यदि (!stmtsp) प्रत्यागम;

            // Wrap in null check
            AstIf* ध्रुव nullCheckp = नूतन AstIf{
                fl,
                नूतन AstNeq{fl, नूतन AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           नूतन AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->सङ्कलय_Stmtsp(wrap_If_Rand_बहुलकम्(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* new_सज्जता_Constraint_Task(AstClass* ध्रुव nodep, ध्रुव std::string& name) {
        AstTask* ध्रुव taskp = नूतन AstTask{nodep->fileline(), name + "_setup_constraint", शून्यसूचकः};
        taskp->class_विधानम्(सत्य);
        nodep->सङ्कलय_Membersp(taskp);
        प्रत्यागम taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* ध्रुव nodep, ध्रुव std::string& name) {
        AstTask* ध्रुव taskp
            = नूतन AstTask{nodep->fileline(), name + "_resize_constrained_array", शून्यसूचकः};
        taskp->class_विधानम्(सत्य);
        nodep->सङ्कलय_Membersp(taskp);
        प्रत्यागम taskp;
    }
    Ast_ग्रन्थिः_Stmt* implementConstraintsClear(File_पङ्क्तिः* ध्रुव fileline, AstVar* ध्रुव genp) {
        AstCMethodHard* ध्रुव clearp = नूतन AstCMethodHard{
            fileline,
            नूतन AstVarRef{fileline, VN_AS(genp->user2p(), ग्रन्थिः_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtype_समुच्चयः_Void();
        प्रत्यागम clearp->makeStmt();
    }
    Ast_ग्रन्थिः_Stmt* implementConstraintsClearAll(File_पङ्क्तिः* ध्रुव fileline, AstVar* ध्रुव genp) {
        AstCMethodHard* ध्रुव clearp = नूतन AstCMethodHard{
            fileline,
            नूतन AstVarRef{fileline, VN_AS(genp->user2p(), ग्रन्थिः_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtype_समुच्चयः_Void();
        प्रत्यागम clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
        यदि (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
            प्रत्यागम memberSelp->varp();
        } अन्यथा यदि (AstVarRef* ध्रुव varrefp = VN_CAST(exprp, VarRef)) {
            प्रत्यागम varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        प्रत्यागम शून्यसूचकः;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_ग्रन्थिः_व्यञ्जकम्* make_बहुलकम्_Var_Ref(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp, AstVar* ध्रुव बहुलकम्_Varp,
                                ध्रुव VAccess access) {
        यदि (बहुलकम्_Varp->lifetime().isStatic()) {
            प्रत्यागम नूतन AstVarRef{exprp->fileline(), VN_AS(बहुलकम्_Varp->user2p(), ग्रन्थिः_Module),
                                 बहुलकम्_Varp, access};
        }
        यदि (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* ध्रुव newMemberSelp = नूतन AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_वृक्षः(असत्य), बहुलकम्_Varp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            प्रत्यागम newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        प्रत्यागम नूतन AstVarRef{exprp->fileline(), VN_AS(बहुलकम्_Varp->user2p(), Class), बहुलकम्_Varp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* create_Or_Get_आकार_Var(AstClass* ध्रुव classp, AstVar* ध्रुव arrVarp, File_पङ्क्तिः* ध्रुव fl,
                               Ast_ग्रन्थिः_DType* ध्रुव signed32DTypep, तार्किक& wasCreated) {
        AstVar* आकार_Varp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = असत्य;
        यदि (!आकार_Varp) {
            आकार_Varp = नूतन AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->सङ्कलय_Membersp(आकार_Varp);
            m_memberMap.निवेशय(classp, आकार_Varp);
            आकार_Varp->user2p(classp);
            arrVarp->user4p(आकार_Varp);
            wasCreated = सत्य;
        }
        प्रत्यागम आकार_Varp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_व्यञ्जकम्* create_आकार_Gte_Zero_Constraint(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव आकार_Varp,
                                                   Ast_ग्रन्थिः_व्यञ्जकम्* derefFromp = शून्यसूचकः) {
        Ast_ग्रन्थिः_व्यञ्जकम्* आकार_ग्रन्थिः_Exprp = शून्यसूचकः;
        यदि (derefFromp) {
            आकार_ग्रन्थिः_Exprp = buildMemberSelExprp(derefFromp, आकार_Varp);
        } अन्यथा {
            आकार_ग्रन्थिः_Exprp = नूतन AstVarRef{fl, आकार_Varp, VAccess::READ};
        }
        आकार_ग्रन्थिः_Exprp->user1(सत्य);
        AstGteS* ध्रुव आकार_Gtep = नूतन AstGteS{fl, आकार_ग्रन्थिः_Exprp, नूतन AstConst{fl, 0}};
        आकार_Gtep->user1(सत्य);
        प्रत्यागम नूतन Ast_Constraint_व्यञ्जकम्{fl, आकार_Gtep};
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* getFromp(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
        यदि (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
            प्रत्यागम memberSelp->fromp();
        } अन्यथा यदि (Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(exprp, विधानम्_Call)) {
            प्रत्यागम विधानम्_Callp->fromp();
        }
        प्रत्यागम शून्यसूचकः;
    }
    AstVar* make_Tmp_Rand_बहुलकम्_Var(Ast_ग्रन्थिः_व्यञ्जकम्* siblingExprp, AstVar* rand_बहुलकम्_Varp,
                               Ast_ग्रन्थिः*& storeStmtspr, Ast_ग्रन्थिः_Stmt*& restoreStmtspr) {
        File_पङ्क्तिः* ध्रुव fl = rand_बहुलकम्_Varp->fileline();
        AstVar* ध्रुव rand_बहुलकम्_Tmp_Varp = नूतन AstVar{
            fl, VVarType::BLOCKTEMP, m_बहुलकम्_Unique_नामानि.get(rand_बहुलकम्_Varp), rand_बहुलकम्_Varp->dtypep()};
        rand_बहुलकम्_Tmp_Varp->funcLocal(m_ftaskp);
        rand_बहुलकम्_Tmp_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_ग्रन्थिः::सङ्कलय_अग्र(
            storeStmtspr,
            नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, rand_बहुलकम्_Tmp_Varp, VAccess::WRITE},
                          make_बहुलकम्_Var_Ref(siblingExprp, rand_बहुलकम्_Varp, VAccess::READ)});
        storeStmtspr = Ast_ग्रन्थिः::सङ्कलय_अग्र(
            storeStmtspr,
            make_बहुलकम्_समुच्चयः_Loop(fl, make_बहुलकम्_Var_Ref(siblingExprp, rand_बहुलकम्_Varp, VAccess::WRITE),
                            नूतन AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_ग्रन्थिः::सङ्कलय_अग्र(
            restoreStmtspr,
            नूतन Ast_नियोजय{fl, make_बहुलकम्_Var_Ref(siblingExprp, rand_बहुलकम्_Varp, VAccess::WRITE),
                          नूतन AstVarRef{fl, rand_बहुलकम्_Tmp_Varp, VAccess::READ}});
        प्रत्यागम rand_बहुलकम्_Tmp_Varp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_ग्रन्थिः_व्यञ्जकम्* खण्डिका_गन्तव्यम्_Common_उपसर्गः(Ast_ग्रन्थिः_व्यञ्जकम्* thisp, Ast_ग्रन्थिः_व्यञ्जकम्* otherp) {
        स्थिर std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        यावत् (thisp) {
            thisHier.प्रक्षिप_पृष्ठभागः(thisp);
            thisp = getFromp(thisp);
        }
        यावत् (otherp) {
            otherHier.प्रक्षिप_पृष्ठभागः(otherp);
            otherp = getFromp(otherp);
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* commonp = शून्यसूचकः;
        प्रति (स्वचालित thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            यदि ((*thisIt)->type() != (*otherIt)->type()) भङ्ग;
            यदि (AstMemberSel* ध्रुव memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* ध्रुव otherMemberSelp = VN_AS(*otherIt, MemberSel);
                यदि (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    अनुवर्त;
                }
            } अन्यथा यदि (Ast_विधानम्_Call* ध्रुव this_विधानम्_Callp = VN_CAST(*thisIt, विधानम्_Call)) {
                Ast_विधानम्_Call* ध्रुव other_विधानम्_Callp = VN_AS(*otherIt, विधानम्_Call);
                यदि (this_विधानम्_Callp->taskp() == other_विधानम्_Callp->taskp()) {
                    commonp = this_विधानम्_Callp;
                    अनुवर्त;
                }
            } अन्यथा यदि (AstVarRef* ध्रुव firstVarRefp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* ध्रुव secondVarRefp = VN_AS(*otherIt, VarRef);
                यदि (firstVarRefp->varp() == secondVarRefp->varp()) {
                    commonp = firstVarRefp;
                    अनुवर्त;
                }
            }
            भङ्ग;
        }
        प्रत्यागम commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_ग्रन्थिः_व्यञ्जकम्* buildMemberSelExprp(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstVar* गभीरतमम्_Varp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_ग्रन्थिः_व्यञ्जकम्* resExprp = शून्यसूचकः;
        यदि (ध्रुव AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), शून्यसूचकः);
            AstVar* varp = memberSelp->varp();
            यदि (गभीरतमम्_Varp) varp = गभीरतमम्_Varp;
            resExprp = नूतन AstMemberSel{exprp->fileline(), resExprp, varp};
        } अन्यथा यदि (ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            यदि (गभीरतमम्_Varp) varp = गभीरतमम्_Varp;
            resExprp = नूतन AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), ग्रन्थिः_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(सत्य);
        प्रत्यागम resExprp;
    }

    शून्य सङ्कलय_Update_Rand_Vars_Body(AstClass* ध्रुव nodep, AstFunc* ध्रुव randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* ध्रुव updatep
            = नूतन AstFunc{nodep->fileline(), "__VupdateRandVars", शून्यसूचकः, शून्यसूचकः};
        updatep->class_विधानम्(सत्य);
        updatep->isVirtual(सत्य);
        updatep->keepAlive(सत्य);
        updatep->needsSyms(असत्य);
        nodep->सङ्कलय_Membersp(updatep);
        nodep->hasRandVarsUpdate(सत्य);

        प्रति (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : शून्यसूचकः) {
            Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(m_memberMap.findMember(classp, "new"), ग्रन्थिः_FTask);
            प्रति (Ast_ग्रन्थिः* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_व्यञ्जकम्* ध्रुव stmtExprp = VN_CAST(stmtp, Stmt_व्यञ्जकम्);
                यदि (!stmtExprp) अनुवर्त;
                AstCMethodHard* ध्रुव methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                यदि (!methodp || methodp->विधानम्() != VCMethod::RANDOMIZER_WRITE_VAR) अनुवर्त;

                Ast_ग्रन्थिः_Var_Ref* ध्रुव varRefp = VN_AS(methodp->pinsp(), ग्रन्थिः_Var_Ref);
                यदि (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    अनुवर्त;
                }
                AstCMethodHard* ध्रुव updateMethodp = नूतन AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_वृक्षः_विशुद्धम्(असत्य),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_वृक्षः_विशुद्धम्(असत्य)};
                updateMethodp->dtype_समुच्चयः_Void();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव namep = VN_AS(varRefp->nextp()->nextp(), ग्रन्थिः_व्यञ्जकम्);
                updateMethodp->सङ्कलय_Pinsp(namep->clone_वृक्षः_विशुद्धम्(असत्य));
                updateMethodp->विधानम्(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->सङ्कलय_Stmtsp(updateMethodp->makeStmt());
            }
        }
    }

    शून्य सङ्कलय_Basic_Randomize_Body(AstFunc* ध्रुव basicRandomizep, AstClass* ध्रुव nodep,
                               AstVar* rand_बहुलकम्_Varp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstVar* ध्रुव basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* ध्रुव basicFvarRefp = नूतन AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* ध्रुव beginBasicValp = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1};
        basicRandomizep->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, basicFvarRefp, beginBasicValp});
        Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(m_memberMap.findMember(nodep, "new"), ग्रन्थिः_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            यदि (!memberVarp->यादृच्छिकम्().isRandomizable()) प्रत्यागम;
            ध्रुव Randomize_बहुलकम् rand_बहुलकम् = {.asUQuad = memberVarp->user1()};
            यदि (rand_बहुलकम्.उपयुङ्क्ते_बहुलकम्
                && !memberVarp->यादृच्छिकम्().isRand()) {  // Not randomizable by default
                AstCMethodHard* ध्रुव setp = नूतन AstCMethodHard{
                    nodep->fileline(),
                    नूतन AstVarRef{fl, VN_AS(rand_बहुलकम्_Varp->user2p(), ग्रन्थिः_Module), rand_बहुलकम्_Varp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, नूतन AstConst{nodep->fileline(), rand_बहुलकम्.सूचकांक}};
                setp->dtype_समुच्चयः_UInt32();
                newp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, setp, नूतन AstConst{fl, 0}});
            }
            यदि (memberVarp->user3()) प्रत्यागम;  // Handled in constraints
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = memberVarp->dtypep()->skipRefp();
            यदि (ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType)) {
                यदि (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    प्रत्यागम;
                }
                AstFunc* ध्रुव memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                Ast_विधानम्_Call* ध्रुव callp
                    = memberVarp->globalConstrained()
                          ? नूतन Ast_विधानम्_Call{fl,
                                              नूतन AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : नूतन Ast_विधानम्_Call{
                                fl, नूतन AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* ध्रुव basicFvarRefReadp = basicFvarRefp->clone_वृक्षः(असत्य);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* ध्रुव नियोजय_If_Not_Nullp = नूतन AstIf{
                    fl,
                    नूतन AstNeq{fl, नूतन AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               नूतन AstConst{fl, AstConst::Null{}}},
                    नूतन Ast_नियोजय{fl, basicFvarRefp->clone_वृक्षः(असत्य),
                                  नूतन AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->सङ्कलय_Stmtsp(wrap_If_Rand_बहुलकम्(nodep, memberVarp, नियोजय_If_Not_Nullp));
            } अन्यथा {
                AstVar* ध्रुव randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* ध्रुव refp
                    = नूतन AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                यदि (!refp->backp()) VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
                basicRandomizep->सङ्कलय_Stmtsp(
                    नूतन AstBegin{fl, m_unique_नामानि.get("__VrandBlock"), stmtp, असत्य});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_ग्रन्थिः_व्यञ्जकम्* make_बहुलकम्_नियोजय_Lhs(File_पङ्क्तिः* ध्रुव fl, AstClass* ध्रुव classp,
                                   Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp, AstVar* ध्रुव बहुलकम्_Varp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        यदि (बहुलकम्_Varp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            यदि (fromp) VL_DO_DANGLING(fromp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), fromp);
            प्रत्यागम नूतन AstVarRef{fl, VN_AS(बहुलकम्_Varp->user2p(), ग्रन्थिः_Module), बहुलकम्_Varp,
                                 VAccess::WRITE};
        } अन्यथा यदि (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            प्रत्यागम नूतन AstVarRef{fl, VN_AS(बहुलकम्_Varp->user2p(), ग्रन्थिः_Module), बहुलकम्_Varp,
                                 VAccess::WRITE};
        } अन्यथा {
            AstMemberSel* ध्रुव memberselp = नूतन AstMemberSel{fl, fromp->unlink_Fr_पृष्ठभागः(), बहुलकम्_Varp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            प्रत्यागम memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    शून्य replace_With_बहुलकम्_नियोजय(Ast_ग्रन्थिः_FTask_Ref* ध्रुव ftaskRefp, Ast_ग्रन्थिः* ध्रुव receiverp,
                               Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp) {
        replace_With_बहुलकम्_नियोजय_And_Append(ftaskRefp, receiverp, lhsp, शून्यसूचकः);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    शून्य replace_With_बहुलकम्_नियोजय_And_Append(Ast_ग्रन्थिः_FTask_Ref* ध्रुव ftaskRefp, Ast_ग्रन्थिः* ध्रुव receiverp,
                                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp, Ast_ग्रन्थिः* ध्रुव appendStmtp) {
        File_पङ्क्तिः* ध्रुव fl = ftaskRefp->fileline();
        यदि (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_व्यञ्जकम्), ftaskRefp, "Should be a statement");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = ftaskRefp->argsp()->exprp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः* newStmtp = शून्यसूचकः;
            यदि (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                //बहुलकम्
                ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.उपयुङ्क्ते_बहुलकम्, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* ध्रुव setp = नूतन AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                नूतन AstConst{fl, rmode.सूचकांक}};
                setp->dtype_समुच्चयः_UInt32();
                newStmtp = नूतन Ast_नियोजय{fl, setp, rhsp};
            } अन्यथा {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = make_बहुलकम्_समुच्चयः_Loop(fl, lhsp, rhsp, m_ftaskp);
            }
            यदि (appendStmtp) newStmtp->सङ्कलय_अग्र(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(प्रक्षिप_Deletep(m_stmtp), m_stmtp = शून्यसूचकः);
        } अन्यथा {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.उपयुङ्क्ते_बहुलकम्, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* ध्रुव setp = नूतन AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            नूतन AstConst{fl, rmode.सूचकांक}};
            setp->dtype_समुच्चयः_UInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    शून्य wrapRandomizeCallWithNullGuard(Ast_ग्रन्थिः_FTask_Ref* nodep) {
        Ast_विधानम्_Call* ध्रुव callp = VN_CAST(nodep, विधानम्_Call);
        यदि (!callp) प्रत्यागम;
        यदि (callp->user4_समुच्चयः_Once()) प्रत्यागम;
        File_पङ्क्तिः* ध्रुव fl = callp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव checkp
            = नूतन AstNeq{fl, callp->fromp()->clone_वृक्षः(असत्य), नूतन AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlink_Fr_पृष्ठभागः(&relinker);
        relinker.relink(नूतन AstCond{fl, checkp, callp, नूतन AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    शून्य handle_Randomize_तर्काः(Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep) {
        यदि (!nodep->argsp()) प्रत्यागम;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        तार्किक hasNullArg = असत्य;
        प्रति (AstArg *argp = nodep->argsp(), *nextp = शून्यसूचकः; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->अङ्कः().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = सत्य;
                VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        प्रति (AstArg *argp = nodep->argsp(), *nextp = शून्यसूचकः; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            प्रति (AstArg *otherArgp = nextp, *otherNextp = शून्यसूचकः; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prefixp
                    = खण्डिका_गन्तव्यम्_Common_उपसर्गः(argp->exprp(), otherArgp->exprp())) {
                    यदि (prefixp == argp->exprp()) {
                        यदि (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), otherArgp);
                        अनुवर्त;
                    }
                }
                यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prefixp
                    = खण्डिका_गन्तव्यम्_Common_उपसर्गः(otherArgp->exprp(), argp->exprp())) {
                    यदि (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);
                        भङ्ग;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::समुच्चयः<AstVar*> saved_Rand_बहुलकम्_Varps;
        AstVar* tmpVarps = शून्यसूचकः;
        Ast_ग्रन्थिः* storeStmtsp = शून्यसूचकः;
        Ast_ग्रन्थिः* समुच्चयः_Stmtsp = शून्यसूचकः;
        Ast_ग्रन्थिः_Stmt* restoreStmtsp = शून्यसूचकः;
        प्रति (AstArg *argp = nodep->argsp(), *nextp = शून्यसूचकः; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ग्रन्थिः_व्यञ्जकम्* exprp = argp->exprp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव commonPrefixp = खण्डिका_गन्तव्यम्_Common_उपसर्गः(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_पङ्क्तिः* ध्रुव fl = argp->fileline();
            यावत् (exprp) {
                यदि (commonPrefixp == exprp) भङ्ग;
                AstVar* ध्रुव randVarp = getVarFromRef(exprp);
                AstClass* ध्रुव classp = VN_AS(randVarp->user2p(), Class);
                AstVar* ध्रुव rand_बहुलकम्_Varp = randVarp->lifetime().isStatic()
                                                 ? get_Static_Rand_बहुलकम्_Var(classp)
                                                 : get_Rand_बहुलकम्_Var_From_Class(classp);
                UASSERT_OBJ(rand_बहुलकम्_Varp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                यदि (saved_Rand_बहुलकम्_Varps.find(rand_बहुलकम्_Varp) == saved_Rand_बहुलकम्_Varps.अन्तः_2()) {
                    AstVar* ध्रुव rand_बहुलकम्_Tmp_Varp
                        = make_Tmp_Rand_बहुलकम्_Var(exprp, rand_बहुलकम्_Varp, storeStmtsp, restoreStmtsp);
                    saved_Rand_बहुलकम्_Varps.निवेशय(rand_बहुलकम्_Varp);
                    tmpVarps = Ast_ग्रन्थिः::सङ्कलय_अग्र(tmpVarps, rand_बहुलकम्_Tmp_Varp);
                }
                ध्रुव Randomize_बहुलकम् rand_बहुलकम् = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = नूतन AstCMethodHard{
                    fl, make_बहुलकम्_Var_Ref(exprp, rand_बहुलकम्_Varp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, नूतन AstConst{fl, rand_बहुलकम्.सूचकांक}};
                setp->dtype_समुच्चयः_UInt32();
                समुच्चयः_Stmtsp
                    = Ast_ग्रन्थिः::सङ्कलय_अग्र(समुच्चयः_Stmtsp, नूतन Ast_नियोजय{fl, setp, नूतन AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);
        }
        यदि (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* targetClassp = शून्यसूचकः;
            Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(nodep, विधानम्_Call);
            यदि (विधानम्_Callp) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDTypep = विधानम्_Callp->fromp()->dtypep();
                ध्रुव AstClassRefDType* ध्रुव crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                targetClassp = crdtp->classp();
            } अन्यथा {
                targetClassp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(targetClassp, nodep, "randomize(null) target class unresolved");
            AstFunc* ध्रुव checkOnlyFuncp = getCreateRandomizeNullFunc(targetClassp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        यदि (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && समुच्चयः_Stmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlink_Fr_पृष्ठभागः(&relinker);
            Ast_ग्रन्थिः* ध्रुव stmtsp = storeStmtsp;
            stmtsp->सङ्कलय_अग्र(समुच्चयः_Stmtsp);
            stmtsp->सङ्कलय_अग्र(m_stmtp);
            stmtsp->सङ्कलय_अग्र(restoreStmtsp);
            relinker.relink(stmtsp);
            // After relinking, so the function body is complete
            सङ्कलय_अस्थायी_Varsp(tmpVarps);
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
        यदि (AstFunc* ध्रुव existingp = VN_AS(m_memberMap.findMember(classp, name), Func)) {
            प्रत्यागम existingp;
        }
        File_पङ्क्तिः* ध्रुव fl = classp->fileline();
        AstFunc* ध्रुव funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* ध्रुव fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        सङ्कलय_Pre_Post_Call(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* ध्रुव classGenp = getRandomGenerator(classp);
        यदि (!classGenp) {
            funcp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, fvarp, VAccess::WRITE},
                                           नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1}});
        } अन्यथा {
            Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(classGenp->user2p(), ग्रन्थिः_Module);
            funcp->सङ्कलय_Stmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* ध्रुव सज्जता_All_Taskp = get_Create_Constraint_सज्जता_Func(classp, m_memberMap);
            funcp->सङ्कलय_Stmtsp((नूतन AstTaskRef{fl, सज्जता_All_Taskp})->makeStmt());
            AstCExpr* ध्रुव solverCallp = नूतन AstCExpr{fl};
            solverCallp->dtype_समुच्चयः_Bit();
            solverCallp->सङ्कलय(नूतन AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->सङ्कलय(".next_check_only(__Vm_rng)");
            funcp->सङ्कलय_Stmtsp(
                नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(सत्य);
        }

        // 3. post_randomize -- only if result is non-zero
        यदि (AstTask* ध्रुव userPostp = findPrePostTask(classp, "post_randomize")) {
            AstTaskRef* ध्रुव callp = नूतन AstTaskRef{userPostp->fileline(), userPostp};
            funcp->सङ्कलय_Stmtsp(
                नूतन AstIf{fl, नूतन AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        प्रत्यागम funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* lift_Log_If_Chain_गन्तव्यम्_Constraint_If(AstLogIf* logIfp) {
        File_पङ्क्तिः* ध्रुव fl = logIfp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = logIfp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = logIfp->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः* thenBodyp;
        यदि (AstLogIf* ध्रुव innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = lift_Log_If_Chain_गन्तव्यम्_Constraint_If(innerLogIfp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(innerLogIfp), innerLogIfp);
        } अन्यथा {
            thenBodyp = नूतन Ast_Constraint_व्यञ्जकम्{fl, rhsp};
        }
        प्रत्यागम नूतन AstConstraintIf{fl, condp, thenBodyp, शून्यसूचकः};
    }

    स्थिर तार्किक distBoundRefsRandVar(ध्रुव Ast_ग्रन्थिः* boundp) {
        प्रत्यागम boundp->exists(
            [](ध्रुव AstVarRef* vrefp) { प्रत्यागम vrefp->varp()->यादृच्छिकम्().isRandomizable(); });
    }

    स्थिर तार्किक dist_Bound_Refs_बहुलकम्_Var(ध्रुव Ast_ग्रन्थिः* boundp) {
        प्रत्यागम boundp->exists([](ध्रुव AstVarRef* vrefp) {
            यदि (!vrefp->varp()->यादृच्छिकम्().isRandomizable()) प्रत्यागम असत्य;
            ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = vrefp->varp()->user1()};
            प्रत्यागम rmode.उपयुङ्क्ते_बहुलकम्;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newDistRangeMembership(AstDist* distp, ध्रुव AstInsideRange* irp) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        ध्रुव तार्किक isSigned = distp->exprp()->isSigned();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dist_व्यञ्जकम्_Gtep = distp->exprp()->clone_वृक्षः_विशुद्धम्(असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dist_व्यञ्जकम्_Ltep = distp->exprp()->clone_वृक्षः_विशुद्धम्(असत्य);
        dist_व्यञ्जकम्_Gtep->user1(सत्य);
        dist_व्यञ्जकम्_Ltep->user1(सत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव gep
            = isSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                             नूतन AstGteS{fl, dist_व्यञ्जकम्_Gtep, irp->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य)})
                       : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                             नूतन AstGte{fl, dist_व्यञ्जकम्_Gtep, irp->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य)});
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lep
            = isSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                             नूतन AstLteS{fl, dist_व्यञ्जकम्_Ltep, irp->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य)})
                       : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                             नूतन AstLte{fl, dist_व्यञ्जकम्_Ltep, irp->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य)});
        gep->user1(सत्य);
        lep->user1(सत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव andp = नूतन AstLogAnd{fl, gep, lep};
        andp->user1(सत्य);
        प्रत्यागम andp;
    }

    संरचना DistBucket final {
        Ast_ग्रन्थिः_व्यञ्जकम्* rangep;  // A single value or an InsideRange
        Ast_ग्रन्थिः_व्यञ्जकम्* weightExprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::सदिशम्<DistBucket> collectDistBuckets(AstDist* ध्रुव distp) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        std::सदिशम्<DistBucket> buckets;
        प्रति (AstDistItem* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), DistItem)) {
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(ditemp->weightp(), Const)) {
                यदि (constp->गन्तव्यम्_UQuad() == 0) अनुवर्त;
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* weightExprp
                = नूतन AstExtend{fl, ditemp->weightp()->clone_वृक्षः_विशुद्धम्(असत्य), 64};
            // := on a range weights every element, so scale by the range size.
            यदि (!ditemp->is_सम्पूर्णम्()) {
                यदि (ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    ध्रुव AstConst* ध्रुव lop = VN_CAST(irp->lhsp(), Const);
                    ध्रुव AstConst* ध्रुव hip = VN_CAST(irp->rhsp(), Const);
                    Ast_ग्रन्थिः_व्यञ्जकम्* rangeSizep;
                    यदि (lop && hip) {
                        ध्रुव uint64_t range_आकार = hip->गन्तव्यम्_UQuad() - lop->गन्तव्यम्_UQuad() + 1;
                        rangeSizep = नूतन AstConst{fl, AstConst::Unsized64{}, range_आकार};
                    } अन्यथा {
                        rangeSizep = नूतन Ast_सङ्कलय{
                            fl, नूतन AstConst{fl, AstConst::Unsized64{}, 1},
                            नूतन AstSub{fl,
                                       नूतन AstExtend{fl, irp->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य), 64},
                                       नूतन AstExtend{fl, irp->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य), 64}}};
                    }
                    weightExprp = नूतन AstMul{fl, weightExprp, rangeSizep};
                }
            }
            buckets.प्रक्षिप_पृष्ठभागः({ditemp->rangep(), weightExprp});
        }
        प्रत्यागम buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* new_Dist_Membership_पदांशः(AstDist* distp, Ast_ग्रन्थिः_व्यञ्जकम्* rangep) {
        यदि (ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(rangep, InsideRange)) {
            प्रत्यागम newDistRangeMembership(distp, irp);
        }
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqExprp = distp->exprp()->clone_वृक्षः_विशुद्धम्(असत्य);
        eqExprp->user1(सत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = नूतन AstEq{fl, eqExprp, rangep->clone_वृक्षः_विशुद्धम्(असत्य)};
        eqp->user1(सत्य);
        प्रत्यागम eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_व्यञ्जकम्* buildDistMembership(AstDist* distp,
                                           ध्रुव std::सदिशम्<DistBucket>& buckets) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* unionExprp = शून्यसूचकः;
        प्रति (ध्रुव स्वचालित& bucket : buckets) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव memberp = new_Dist_Membership_पदांशः(distp, bucket.rangep);
            यदि (!unionExprp) {
                unionExprp = memberp;
            } अन्यथा {
                unionExprp = नूतन AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(सत्य);
            }
        }
        प्रत्यागम नूतन Ast_Constraint_व्यञ्जकम्{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_ग्रन्थिः_व्यञ्जकम्* newGateJoin(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp, तार्किक isAnd) {
        यदि (ap->same_वृक्षः(bp)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(bp), bp);
            प्रत्यागम ap;
        }
        यदि (VN_IS(bp, Const)) std::swap(ap, bp);
        यदि (VN_IS(ap, Const)) {
            ध्रुव तार्किक constWins = VN_AS(ap, Const)->isZero() == isAnd;
            यदि (!constWins) std::swap(ap, bp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(bp), bp);
            प्रत्यागम ap;
        }
        यदि (isAnd) प्रत्यागम नूतन AstLogAnd{fl, ap, bp};
        प्रत्यागम नूतन AstLogOr{fl, ap, bp};
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* newGateOr(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp) {
        प्रत्यागम newGateJoin(fl, ap, bp, असत्य);
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* newGateAnd(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp) {
        प्रत्यागम newGateJoin(fl, ap, bp, सत्य);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_ग्रन्थिः_व्यञ्जकम्* new_बहुलकम्_Bit_Read(AstVar* varp, ध्रुव AstMemberSel* mselp, AstVar* rand_बहुलकम्_Varp,
                                File_पङ्क्तिः* fl) {
        Ast_ग्रन्थिः_व्यञ्जकम्* arrayp = शून्यसूचकः;
        यदि (varp->lifetime().isStatic()) {
            AstClass* ध्रुव ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* ध्रुव smodep = get_Static_Rand_बहुलकम्_Var(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = नूतन AstVarRef{fl, VN_AS(smodep->user2p(), ग्रन्थिः_Module), smodep, VAccess::READ};
        } अन्यथा यदि (mselp) {
            AstVar* ध्रुव memberModep = get_Rand_बहुलकम्_Var_From_Class(VN_AS(varp->user2p(), ग्रन्थिः_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = नूतन AstMemberSel{fl, mselp->fromp()->clone_वृक्षः_विशुद्धम्(असत्य), memberModep};
        } अन्यथा {
            UASSERT_OBJ(rand_बहुलकम्_Varp, varp, "rand_mode variable without a class mode array");
            arrayp = नूतन AstVarRef{fl, VN_AS(rand_बहुलकम्_Varp->user2p(), ग्रन्थिः_Module), rand_बहुलकम्_Varp,
                                   VAccess::READ};
        }
        ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* ध्रुव atp
            = नूतन AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, नूतन AstConst{fl, rmode.सूचकांक}};
        atp->dtype_समुच्चयः_UInt32();
        प्रत्यागम atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_ग्रन्थिः_व्यञ्जकम्* newVarGate(AstVar* varp, ध्रुव AstMemberSel* mselp, तार्किक ownerLevel,
                            AstVar* rand_बहुलकम्_Varp, File_पङ्क्तिः* fl) {
        यदि (!varp->यादृच्छिकम्().isRandomizable()) {
            प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}, ownerLevel};
        }
        ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = varp->user1()};
        यदि (!rmode.उपयुङ्क्ते_बहुलकम्) प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}};
        प्रत्यागम new_बहुलकम्_Bit_Read(varp, mselp, rand_बहुलकम्_Varp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_ग्रन्थिः_व्यञ्जकम्* newAccessGate(ध्रुव Ast_ग्रन्थिः* nodep, तार्किक ownerLevel, AstVar* rand_बहुलकम्_Varp,
                               File_पङ्क्तिः* fl) {
        यदि (ध्रुव AstMemberSel* ध्रुव mselp = VN_CAST(nodep, MemberSel)) {
            प्रत्यागम newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, rand_बहुलकम्_Varp, fl),
                              newAccessGate(mselp->fromp(), सत्य, rand_बहुलकम्_Varp, fl));
        }
        यदि (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव refp = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
            प्रत्यागम newVarGate(refp->varp(), शून्यसूचकः, ownerLevel, rand_बहुलकम्_Varp, fl);
        }
        यदि (ध्रुव Ast_ग्रन्थिः_Sel* ध्रुव selp = VN_CAST(nodep, ग्रन्थिः_Sel)) {
            प्रत्यागम newAccessGate(selp->fromp(), सत्य, rand_बहुलकम्_Varp, fl);
        }
        यदि (dist_Bound_Refs_बहुलकम्_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_ग्रन्थिः_व्यञ्जकम्* newDistGate(ध्रुव Ast_ग्रन्थिः* nodep, AstVar* rand_बहुलकम्_Varp, File_पङ्क्तिः* fl) {
        यदि (VN_IS(nodep, ग्रन्थिः_Var_Ref) || VN_IS(nodep, MemberSel)) {
            प्रत्यागम newAccessGate(nodep, असत्य, rand_बहुलकम्_Varp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        यदि (ध्रुव Ast_ग्रन्थिः_Sel* ध्रुव selp = VN_CAST(nodep, ग्रन्थिः_Sel)) {
            प्रत्यागम newDistGate(selp->fromp(), rand_बहुलकम्_Varp, fl);
        }
        यदि (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            प्रत्यागम newDistGate(selp->fromp(), rand_बहुलकम्_Varp, fl);
        }
        यदि (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {
            प्रत्यागम newDistGate(selp->fromp(), rand_बहुलकम्_Varp, fl);
        }
        यदि (ध्रुव AstCMethodHard* ध्रुव methodp = VN_CAST(nodep, CMethodHard)) {
            यदि (methodp->विधानम्() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                प्रत्यागम newDistGate(methodp->fromp(), rand_बहुलकम्_Varp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        यदि (ध्रुव AstCond* ध्रुव condp = VN_CAST(nodep, Cond)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenGatep = newDistGate(condp->thenp(), rand_बहुलकम्_Varp, fl);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elseGatep = newDistGate(condp->elsep(), rand_बहुलकम्_Varp, fl);
            Ast_ग्रन्थिः_व्यञ्जकम्* armGatep = thenGatep;
            यदि (thenGatep->same_वृक्षः(elseGatep)) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(elseGatep), elseGatep);
            } अन्यथा {
                armGatep
                    = नूतन AstCond{fl, condp->condp()->clone_वृक्षः_विशुद्धम्(असत्य), thenGatep, elseGatep};
            }
            प्रत्यागम newGateOr(fl, newDistGate(condp->condp(), rand_बहुलकम्_Varp, fl), armGatep);
        }
        यदि (ध्रुव Ast_ग्रन्थिः_Uniop* ध्रुव uopp = VN_CAST(nodep, ग्रन्थिः_Uniop)) {
            प्रत्यागम newDistGate(uopp->lhsp(), rand_बहुलकम्_Varp, fl);
        }
        यदि (ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव bopp = VN_CAST(nodep, ग्रन्थिः_Biop)) {
            प्रत्यागम newGateOr(fl, newDistGate(bopp->lhsp(), rand_बहुलकम्_Varp, fl),
                             newDistGate(bopp->rhsp(), rand_बहुलकम्_Varp, fl));
        }
        यदि (dist_Bound_Refs_बहुलकम्_Var(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        प्रत्यागम नूतन AstConst{fl, AstConst::BitTrue{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_व्यञ्जकम्* buildFrozenMembership(AstDist* distp,
                                             ध्रुव std::सदिशम्<DistBucket>& buckets) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* unionExprp = शून्यसूचकः;
        प्रति (ध्रुव स्वचालित& bucket : buckets) {
            Ast_ग्रन्थिः_व्यञ्जकम्* termp = new_Dist_Membership_पदांशः(distp, bucket.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            ध्रुव तार्किक runtimeWeight
                = bucket.weightExprp->exists([](ध्रुव Ast_ग्रन्थिः_Var_Ref*) { प्रत्यागम सत्य; });
            यदि (runtimeWeight) {
                AstNeq* ध्रुव nzp = नूतन AstNeq{fl, bucket.weightExprp->clone_वृक्षः_विशुद्धम्(असत्य),
                                               नूतन AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(सत्य);
                termp = नूतन AstLogAnd{fl, termp, nzp};
                termp->user1(सत्य);
            }
            यदि (unionExprp) {
                unionExprp = नूतन AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(सत्य);
            } अन्यथा {
                unionExprp = termp;
            }
        }
        प्रत्यागम नूतन Ast_Constraint_व्यञ्जकम्{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_ग्रन्थिः* gateFrozenDist(Ast_ग्रन्थिः* chainp, AstDist* distp,
                            ध्रुव std::सदिशम्<DistBucket>& buckets, AstVar* rand_बहुलकम्_Varp) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* gatep = newDistGate(distp->exprp(), rand_बहुलकम्_Varp, fl);
        यदि (VN_IS(gatep, Const)) {
            ध्रुव तार्किक drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(प्रक्षिप_Deletep(gatep), gatep);
            यदि (drawable) प्रत्यागम chainp;
            VL_DO_DANGLING(प्रक्षिप_Deletep(chainp), chainp);
            प्रत्यागम buildFrozenMembership(distp, buckets);
        }
        प्रत्यागम नूतन AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* new_एकरूपम्_Range_Pick(AstDist* distp, ध्रुव AstInsideRange* irp) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dist_व्यञ्जकम्_Copyp = distp->exprp()->clone_वृक्षः_विशुद्धम्(असत्य);
        dist_व्यञ्जकम्_Copyp->user1(सत्य);
        ध्रुव पूर्णाङ्क dist_विस्तारः = distp->exprp()->विस्तारः();
        ध्रुव AstConst* ध्रुव lopC = VN_CAST(irp->lhsp(), Const);
        ध्रुव AstConst* ध्रुव hipC = VN_CAST(irp->rhsp(), Const);
        Ast_ग्रन्थिः_व्यञ्जकम्* rangeSzp;
        यदि (lopC && hipC) {
            ध्रुव uint64_t rsz = hipC->गन्तव्यम्_UQuad() - lopC->गन्तव्यम्_UQuad() + 1;
            rangeSzp = नूतन AstConst{fl, AstConst::Unsized64{}, rsz};
        } अन्यथा {
            ध्रुव तार्किक isSigned = irp->lhsp()->isSigned();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lo64p
                = isSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                 नूतन AstExtendS{fl, irp->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य), 64})
                           : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                 नूतन AstExtend{fl, irp->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य), 64});
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव hi64p
                = isSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                 नूतन AstExtendS{fl, irp->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य), 64})
                           : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                                 नूतन AstExtend{fl, irp->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य), 64});
            rangeSzp = नूतन Ast_सङ्कलय{fl, नूतन AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  नूतन AstSub{fl, hi64p, lo64p}};
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rand64p = नूतन AstRand{fl, शून्यसूचकः, असत्य};
        rand64p->dtype_समुच्चयः_UInt64();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव offsetp
            = नूतन AstCCast{fl, नूतन AstModDiv{fl, rand64p, rangeSzp}, dist_विस्तारः};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = नूतन Ast_सङ्कलय{fl, irp->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य), offsetp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव eqp = नूतन AstEq{fl, dist_व्यञ्जकम्_Copyp, valuep};
        eqp->user1(सत्य);
        प्रत्यागम eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_ग्रन्थिः* buildWeightedBucketChain(AstDist* distp, ध्रुव std::सदिशम्<DistBucket>& buckets,
                                      AstVar* bucketVarp,
                                      ध्रुव std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>& cumSums) {
        File_पङ्क्तिः* ध्रुव fl = distp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* chainp = शून्यसूचकः;
        प्रति (पूर्णाङ्क i = static_cast<पूर्णाङ्क>(buckets.आकार()) - 1; i >= 0; --i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* constraintExprp;
            ध्रुव AstInsideRange* ध्रुव irp = VN_CAST(buckets[i].rangep, InsideRange);
            यदि (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } अन्यथा यदि (irp) {
                constraintExprp = new_एकरूपम्_Range_Pick(distp, irp);
            } अन्यथा {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dist_व्यञ्जकम्_Copyp = distp->exprp()->clone_वृक्षः_विशुद्धम्(असत्य);
                dist_व्यञ्जकम्_Copyp->user1(सत्य);
                constraintExprp
                    = नूतन AstEq{fl, dist_व्यञ्जकम्_Copyp, buckets[i].rangep->clone_वृक्षः_विशुद्धम्(असत्य)};
                constraintExprp->user1(सत्य);
            }
            यदि (!chainp) {
                chainp = constraintExprp;
            } अन्यथा {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp
                    = नूतन AstLte{fl, नूतन AstVarRef{fl, bucketVarp, VAccess::READ}, cumSums[i]};
                condp->user1(सत्य);
                chainp = नूतन AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(सत्य);
            }
        }
        Ast_Constraint_व्यञ्जकम्* ध्रुव chainExprp = नूतन Ast_Constraint_व्यञ्जकम्{fl, chainp};
        chainExprp->isSoft(सत्य);
        प्रत्यागम chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    शून्य अधस्तनम्_Dist_Constraints(AstTask* taskp, Ast_ग्रन्थिः* constrItemsp, AstVar* rand_बहुलकम्_Varp,
                              AstConstraintForeach* foreachp = शून्यसूचकः) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_ग्रन्थिः* foreachTailp = शून्यसूचकः;
        स्वचालित सङ्कलय_Stmt = [&](Ast_ग्रन्थिः* nodep) {
            यदि (foreachp) {
                यदि (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } अन्यथा {
                    foreachTailp->सङ्कलय_अग्र(nodep);
                    foreachTailp = nodep;
                }
            } अन्यथा {
                taskp->सङ्कलय_Stmtsp(nodep);
            }
        };

        प्रति (Ast_ग्रन्थिः *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            यदि (AstConstraintIf* ध्रुव cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                अधस्तनम्_Dist_Constraints(taskp, cifp->thensp(), rand_बहुलकम्_Varp, foreachp);
                यदि (cifp->elsesp()) {
                    अधस्तनम्_Dist_Constraints(taskp, cifp->elsesp(), rand_बहुलकम्_Varp, foreachp);
                }
                अनुवर्त;
            }
            यदि (AstConstraintForeach* ध्रुव cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                अधस्तनम्_Dist_Constraints(taskp, cfep->bodyp(), rand_बहुलकम्_Varp, cfep);
                अनुवर्त;
            }

            Ast_Constraint_व्यञ्जकम्* ध्रुव constrExprp = VN_CAST(itemp, Constraint_व्यञ्जकम्);
            यदि (!constrExprp) अनुवर्त;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            यदि (AstLogIf* ध्रुव शिखरम्_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_ग्रन्थिः* chainEndp = शिखरम्_Log_Ifp->rhsp();
                यावत् (AstLogIf* ध्रुव innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                यदि (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* ध्रुव liftedp = lift_Log_If_Chain_गन्तव्यम्_Constraint_If(शिखरम्_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(constrExprp), constrExprp);
                    अधस्तनम्_Dist_Constraints(taskp, liftedp->thensp(), rand_बहुलकम्_Varp, foreachp);
                    अनुवर्त;
                }
            }

            AstDist* ध्रुव distp = VN_CAST(constrExprp->exprp(), Dist);
            यदि (!distp) अनुवर्त;

            File_पङ्क्तिः* ध्रुव fl = distp->fileline();

            std::सदिशम्<DistBucket> buckets = collectDistBuckets(distp);

            यदि (buckets.रिक्तानि()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_व्यञ्जकम्* ध्रुव truep
                    = नूतन Ast_Constraint_व्यञ्जकम्{fl, नूतन AstConst{fl, AstConst::BitTrue{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(प्रक्षिप_Deletep(constrExprp), constrExprp);
                अनुवर्त;
            }

            Ast_Constraint_व्यञ्जकम्* ध्रुव membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_ग्रन्थिः_व्यञ्जकम्* योग_Weight_Exprp = शून्यसूचकः;
            प्रति (स्वचालित& bucket : buckets) {
                यदि (!योग_Weight_Exprp) {
                    योग_Weight_Exprp = bucket.weightExprp->clone_वृक्षः_विशुद्धम्(असत्य);
                } अन्यथा {
                    योग_Weight_Exprp = नूतन Ast_सङ्कलय{fl, योग_Weight_Exprp,
                                                  bucket.weightExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                    योग_Weight_Exprp->dtype_समुच्चयः_UInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            ध्रुव पूर्णाङ्क distId = m_dist_अङ्कः++;
            ध्रुव std::string योग_Name = "__Vdist_total" + cvt_गन्तव्यम्_Str(distId);
            AstVar* ध्रुव योग_Varp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP, योग_Name, taskp->findUInt64DType()};
            योग_Varp->noSubst(सत्य);
            योग_Varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            योग_Varp->funcLocal(सत्य);
            योग_Varp->is_आन्तरिकम्(सत्य);
            सङ्कलय_Stmt(योग_Varp);
            सङ्कलय_Stmt(
                नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, योग_Varp, VAccess::WRITE}, योग_Weight_Exprp});

            // bucketVar = (rand64() % totalWeight) + 1
            ध्रुव std::string bucketName = "__Vdist_bucket" + cvt_गन्तव्यम्_Str(distId);
            AstVar* ध्रुव bucketVarp
                = नूतन AstVar{fl, VVarType::BLOCKTEMP, bucketName, taskp->findUInt64DType()};
            bucketVarp->noSubst(सत्य);
            bucketVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            bucketVarp->funcLocal(सत्य);
            bucketVarp->is_आन्तरिकम्(सत्य);
            सङ्कलय_Stmt(bucketVarp);

            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव randp = नूतन AstRand{fl, शून्यसूचकः, असत्य};
            randp->dtype_समुच्चयः_UInt64();
            सङ्कलय_Stmt(नूतन Ast_नियोजय{
                fl, नूतन AstVarRef{fl, bucketVarp, VAccess::WRITE},
                नूतन Ast_सङ्कलय{
                    fl, नूतन AstConst{fl, AstConst::Unsized64{}, 1},
                    नूतन AstModDiv{fl, randp, नूतन AstVarRef{fl, योग_Varp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> cumSums;
            Ast_ग्रन्थिः_व्यञ्जकम्* runningSump = शून्यसूचकः;
            प्रति (आकार_t i = 0; i < buckets.आकार(); ++i) {
                यदि (!runningSump) {
                    runningSump = buckets[i].weightExprp->clone_वृक्षः_विशुद्धम्(असत्य);
                } अन्यथा {
                    runningSump = नूतन Ast_सङ्कलय{fl, runningSump,
                                             buckets[i].weightExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                    runningSump->dtype_समुच्चयः_UInt64();
                }
                cumSums.प्रक्षिप_पृष्ठभागः(runningSump->clone_वृक्षः_विशुद्धम्(सत्य));
            }

            Ast_ग्रन्थिः* chainp = buildWeightedBucketChain(distp, buckets, bucketVarp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, rand_बहुलकम्_Varp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->सङ्कलय_अत्र_This_As_अग्र(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            प्रति (स्वचालित& bucket : buckets) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(bucket.weightExprp), bucket.weightExprp);
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            प्रक्षिप_Deletep(cumSums.पृष्ठभागः());
        }
    }

    स्थिर तार्किक is_Dyn_Arr_सम्बन्धि_Class_Type_Recurse(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव refp = dtypep->skipRefp();
        यदि (VN_IS(refp, DynArrayDType) || VN_IS(refp, QueueDType)) {
            प्रत्यागम is_Dyn_Arr_सम्बन्धि_Class_Type_Recurse(refp->subDTypep());
        } अन्यथा यदि (VN_IS(refp, ClassRefDType)) {
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    शून्य सङ्कलय_Array_Elem_Refresh(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव arrVarp, AstFunc* ध्रुव randomizep,
                             AstVar* ध्रुव genp) {
        Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(genp->user2p(), ग्रन्थिः_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        ध्रुव uint32_t unpackedDims = arrVarp->dtypep()->dimensions(असत्य).second;
        यदि (is_Dyn_Arr_सम्बन्धि_Class_Type_Recurse(arrVarp->dtypep())) {
            यदि (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            प्रत्यागम;
        }
        AstCMethodHard* ध्रुव methodp
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtype_समुच्चयः_Void();

        Ast_ग्रन्थिः_Module* ध्रुव classp = VN_AS(arrVarp->user2p(), ग्रन्थिः_Module);
        AstVarRef* ध्रुव varRefp = नूतन AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->सङ्कलय_Pinsp(varRefp);

        ध्रुव आकार_t विस्तारः = arrayElementDTypep(arrVarp->dtypep())->विस्तारः();

        methodp->सङ्कलय_Pinsp(नूतन AstConst{fl, AstConst::Unsized64{}, विस्तारः});
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव varnamep
            = नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "\"" + arrVarp->name() + "\"", arrVarp->विस्तारः()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->सङ्कलय_Pinsp(varnamep);
        methodp->सङ्कलय_Pinsp(नूतन AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = arrVarp->user1()};
        यदि (rmode.उपयुङ्क्ते_बहुलकम्) {
            methodp->सङ्कलय_Pinsp(नूतन AstConst{fl, AstConst::Unsized64{}, rmode.सूचकांक});
        }

        randomizep->सङ्कलय_Stmtsp(methodp->makeStmt());
    }

    Ast_ग्रन्थिः_Stmt* new_Pin_आकार_Variable(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव arrVarp,
                                    AstVar* ध्रुव genp) {
        Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(genp->user2p(), ग्रन्थिः_Module);
        AstVar* ध्रुव आकार_Varp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* ध्रुव pinp
            = नूतन AstCMethodHard{fl, नूतन AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtype_समुच्चयः_Void();
        AstCExpr* ध्रुव namep = नूतन AstCExpr{fl, AstCExpr::विशुद्धम्{}, "\"" + आकार_Varp->name() + "\""};
        namep->dtype_समुच्चयः_UInt32();
        pinp->सङ्कलय_Pinsp(namep);
        pinp->सङ्कलय_Pinsp(
            नूतन AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(आकार_Varp->विस्तारः())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* ध्रुव आकार_Var_Refp = नूतन AstVarRef{fl, आकार_Varp, VAccess::READ};
        आकार_Var_Refp->classOrPackagep(VN_AS(आकार_Varp->user2p(), ग्रन्थिः_Module));
        pinp->सङ्कलय_Pinsp(आकार_Var_Refp);
        प्रत्यागम pinp->makeStmt();
    }

    शून्य सङ्कलय_Frozen_आकार_Pin(File_पङ्क्तिः* ध्रुव fl, AstVar* ध्रुव arrVarp, AstFunc* ध्रुव randomizep,
                          AstVar* ध्रुव genp, AstVar* ध्रुव rand_बहुलकम्_Varp) {
        ध्रुव Randomize_बहुलकम् rmode = {.asUQuad = arrVarp->user1()};
        यदि (!rmode.उपयुङ्क्ते_बहुलकम्) प्रत्यागम;
        AstVar* ध्रुव आकार_Varp = VN_AS(arrVarp->user4p(), Var);
        Ast_ग्रन्थिः_Module* ध्रुव arrClassp = VN_AS(arrVarp->user2p(), ग्रन्थिः_Module);
        Ast_ग्रन्थिः_Module* ध्रुव आकार_Classp = VN_AS(आकार_Varp->user2p(), ग्रन्थिः_Module);
        AstVarRef* ध्रुव आकार_Writep = नूतन AstVarRef{fl, आकार_Classp, आकार_Varp, VAccess::WRITE};
        ध्रुव VCMethod आकार_विधानम् = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_आकार
                                        : VCMethod::DYN_आकार;
        AstCMethodHard* ध्रुव currentSizep = नूतन AstCMethodHard{
            fl, नूतन AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, आकार_विधानम्};
        currentSizep->dtypep(आकार_Varp->dtypep());
        randomizep->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, आकार_Writep, currentSizep});
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव modep = new_बहुलकम्_Bit_Read(arrVarp, शून्यसूचकः, rand_बहुलकम्_Varp, fl);
        randomizep->सङ्कलय_Stmtsp(
            नूतन AstIf{fl, नूतन AstLogNot{fl, modep}, new_Pin_आकार_Variable(fl, arrVarp, genp)});
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_अङ्कः);
        VL_RESTORER_COPY(m_rand_Array_Unique_नामानि);
        m_modp = nodep;
        m_rand_Case_अङ्कः = 0;
        m_rand_Array_Unique_नामानि.reset();
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_अङ्कः);
        VL_RESTORER(m_dist_अङ्कः);
        m_modp = nodep;
        m_rand_Case_अङ्कः = 0;
        m_dist_अङ्कः = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        यदि (!nodep->user1()) प्रत्यागम;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->आधारः_Most_Classp()->needRNG(सत्य);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        ध्रुव तार्किक basicFirst = classOwnsGlobalConstraint(nodep);

        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        AstFunc* ध्रुव randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* ध्रुव fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* ध्रुव rand_बहुलकम्_Varp = get_Rand_बहुलकम्_Var_From_Class(nodep);
        सङ्कलय_Pre_Post_Call(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        यदि (classHasRandClassMembers(nodep)) {
            AstTask* ध्रुव preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->सङ्कलय_Stmtsp((नूतन AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_ग्रन्थिः_व्यञ्जकम्* beginValp = शून्यसूचकः;
        AstVar* genp = getRandomGenerator(nodep);
        यदि (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* ध्रुव prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* ध्रुव classp, AstConstraint* ध्रुव constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                यदि (!taskp) {
                    taskp = new_सज्जता_Constraint_Task(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                यदि (AstTask* ध्रुव resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* ध्रुव resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* ध्रुव resizeTaskRefp
                        = नूतन AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->सङ्कलय_Stmtsp(resizeTaskRefp->makeStmt());
                }

                यदि (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                यदि (constrp->itemsp()) {
                    अधस्तनम्_Dist_Constraints(taskp, constrp->itemsp(), rand_बहुलकम्_Varp);
                }
                VInsertion_समुच्चयः<AstVar*>& आकार_Arrays = m_आकार_Constrained_Arrays[classp];
                Constraint_व्यञ्जकम्_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), शून्यसूचकः,
                    genp,          rand_बहुलकम्_Varp, m_writtenVars,     &m_uniqueConstraintId,
                    m_unique_नामानि, randomizep,   &आकार_Arrays,       prepareArraysTaskp};
                यदि (constrp->itemsp()) {
                    taskp->सङ्कलय_Stmtsp(wrap_If_Constraint_बहुलकम्(
                        nodep, constrp, constrp->itemsp()->unlink_Fr_पृष्ठभागः_With_अग्र()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                ध्रुव स्वचालित constraint_गभीरता = [](ध्रुव AstConstraint* constrp) {
                    पूर्णाङ्क गभीरता = 0;
                    आकार_t pos = 0;
                    यावत् ((pos = constrp->name().find(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++गभीरता;
                        pos += std::वाक्यदैर्घ्यम्(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    प्रत्यागम गभीरता;
                };
                पूर्णाङ्क max_गभीरता = 0;
                nodep->foreachMember([&](AstClass* ध्रुव, AstConstraint* ध्रुव constrp) {
                    max_गभीरता = std::max(max_गभीरता, constraint_गभीरता(constrp));
                });
                AstTask* ध्रुव सज्जता_All_Taskp = get_Create_Constraint_सज्जता_Func(nodep, m_memberMap);
                प्रति (पूर्णाङ्क d = max_गभीरता; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* ध्रुव classp, AstConstraint* ध्रुव constrp) {
                            यदि (constraint_गभीरता(constrp) != d) प्रत्यागम;
                            AstTask* ध्रुव taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* ध्रुव सज्जता_Task_Refp
                                = नूतन AstTaskRef{constrp->fileline(), taskp};
                            सज्जता_Task_Refp->classOrPackagep(classp);
                            सज्जता_All_Taskp->सङ्कलय_Stmtsp(सज्जता_Task_Refp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertion_समुच्चयः<AstVar*> आकार_Array_Vars;
            यदि (nodep->extendsp()) {
                AstClass* parentClassp = nodep->extendsp()->classp();
                यावत् (parentClassp) {
                    ध्रुव स्वचालित आकार_Arrays_It = m_आकार_Constrained_Arrays.find(parentClassp);
                    यदि (आकार_Arrays_It != m_आकार_Constrained_Arrays.अन्तः_2()) {
                        प्रति (AstVar* ध्रुव arrVarp : आकार_Arrays_It->second) {
                            आकार_Array_Vars.निवेशय(arrVarp);
                        }
                    }
                    AstFunc* ध्रुव parentRandomizep
                        = VN_CAST(m_memberMap.findMember(parentClassp, "randomize"), Func);
                    यदि (parentRandomizep && parentRandomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        प्रति (Ast_ग्रन्थिः* stmtp = parentRandomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            तार्किक प्राप्तम्_Clear_Constraints = असत्य;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                यदि (methodp->विधानम्() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->विधानम्() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->सङ्कलय_Stmtsp(stmtp->clone_वृक्षः(असत्य));
                                } अन्यथा यदि (methodp->विधानम्()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    प्राप्तम्_Clear_Constraints = सत्य;
                                }
                            });
                            यदि (प्राप्तम्_Clear_Constraints) भङ्ग;
                        }
                    }
                    parentClassp
                        = parentClassp->extendsp() ? parentClassp->extendsp()->classp() : शून्यसूचकः;
                }
            }
            randomizep->सङ्कलय_Stmtsp(implementConstraintsClear(fl, genp));

            ध्रुव स्वचालित आकार_Arrays_It = m_आकार_Constrained_Arrays.find(nodep);
            यदि (आकार_Arrays_It != m_आकार_Constrained_Arrays.अन्तः_2()) {
                प्रति (AstVar* ध्रुव arrVarp : आकार_Arrays_It->second) आकार_Array_Vars.निवेशय(arrVarp);
            }
            AstTask* ध्रुव सज्जता_All_Taskp = get_Create_Constraint_सज्जता_Func(nodep, m_memberMap);
            यदि (!आकार_Array_Vars.रिक्तानि() && prepareArraysTaskp->stmtsp()) {
                Ast_ग्रन्थिः_Stmt* ध्रुव preparep = (नूतन AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                प्रति (Ast_ग्रन्थिः* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    तार्किक writesArray = असत्य;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->विधानम्() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    यदि (!writesArray) अनुवर्त;
                    stmtp->सङ्कलय_अत्र_This_As_अग्र(preparep);
                    भङ्ग;
                }
            }
            AstTaskRef* ध्रुव सज्जता_Task_Refp = नूतन AstTaskRef{fl, सज्जता_All_Taskp};
            randomizep->सङ्कलय_Stmtsp(सज्जता_Task_Refp->makeStmt());

            Ast_ग्रन्थिः_Module* ध्रुव genModp = VN_AS(genp->user2p(), ग्रन्थिः_Module);

            AstCExpr* ध्रुव solverCallp = नूतन AstCExpr{fl};
            solverCallp->dtype_समुच्चयः_Bit();
            solverCallp->सङ्कलय(नूतन AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->सङ्कलय(".next(__Vm_rng)");
            यदि (!आकार_Array_Vars.रिक्तानि()) {
                AstVar* ध्रुव आकार_Ok_Varp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                आकार_Ok_Varp->funcLocal(सत्य);
                randomizep->सङ्कलय_Stmtsp(आकार_Ok_Varp);

                AstVar* ध्रुव finalOkVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                finalOkVarp->funcLocal(सत्य);
                randomizep->सङ्कलय_Stmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                प्रति (AstVar* ध्रुव arrVarp : आकार_Array_Vars)
                    सङ्कलय_Frozen_आकार_Pin(fl, arrVarp, randomizep, genp, rand_बहुलकम्_Varp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->सङ्कलय_Stmtsp(
                    नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, आकार_Ok_Varp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                यदि (AstTask* ध्रुव resizeAllTaskp
                    = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* ध्रुव resizeTaskRefp = नूतन AstTaskRef{fl, resizeAllTaskp};
                    AstIf* ध्रुव ifp = नूतन AstIf{fl, नूतन AstVarRef{fl, आकार_Ok_Varp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), शून्यसूचकः};
                    randomizep->सङ्कलय_Stmtsp(ifp);
                }

                // Refresh array element tables after resize
                प्रति (ध्रुव स्वचालित& arrVarp : आकार_Array_Vars) {
                    सङ्कलय_Array_Elem_Refresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->सङ्कलय_Stmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* ध्रुव सज्जता_Task_Refp2 = नूतन AstTaskRef{fl, सज्जता_All_Taskp};
                randomizep->सङ्कलय_Stmtsp(सज्जता_Task_Refp2->makeStmt());

                प्रति (AstVar* ध्रुव arrVarp : आकार_Array_Vars) {
                    randomizep->सङ्कलय_Stmtsp(new_Pin_आकार_Variable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* ध्रुव solverCallp2 = नूतन AstCExpr{fl};
                solverCallp2->dtype_समुच्चयः_Bit();
                solverCallp2->सङ्कलय(नूतन AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->सङ्कलय(".next(__Vm_rng)");
                randomizep->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
                    fl, नूतन AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = नूतन AstAnd{fl, नूतन AstVarRef{fl, आकार_Ok_Varp, VAccess::READ},
                                       नूतन AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } अन्यथा {
                beginValp = solverCallp;
            }
            यदि (rand_बहुलकम्_Varp) {
                Ast_ग्रन्थिः_Module* ध्रुव rand_बहुलकम्_Classp = VN_AS(rand_बहुलकम्_Varp->user2p(), Class);
                Ast_ग्रन्थिः_FTask* ध्रुव newp
                    = VN_AS(m_memberMap.findMember(rand_बहुलकम्_Classp, "new"), ग्रन्थिः_FTask);
                UASSERT_OBJ(newp, rand_बहुलकम्_Classp, "No new() in class");
                सङ्कलय_समुच्चयः_Rand_बहुलकम्(newp, genp, rand_बहुलकम्_Varp);
            }
            यदि (AstVar* ध्रुव static_Rand_बहुलकम्_Varp = get_Static_Rand_बहुलकम्_Var(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_ग्रन्थिः_FTask* ध्रुव newp = VN_AS(m_memberMap.findMember(nodep, "new"), ग्रन्थिः_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                सङ्कलय_समुच्चयः_Static_Rand_बहुलकम्(newp, genp, static_Rand_बहुलकम्_Varp);
            }
        } अन्यथा {
            beginValp = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 32, 1};
        }

        सङ्कलय_Update_Rand_Vars_Body(nodep, randomizep);

        AstFunc* ध्रुव basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        सङ्कलय_Basic_Randomize_Body(basicRandomizep, nodep, rand_बहुलकम्_Varp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* ध्रुव fvarRefp = नूतन AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
            fl, fvarRefp, basicFirst ? नूतन AstFuncRef{fl, basicRandomizep} : beginValp});

        ध्रुव स्वचालित आकार_Arrays_It = m_आकार_Constrained_Arrays.find(nodep);
        ध्रुव तार्किक needs_आकार_Phase
            = आकार_Arrays_It != m_आकार_Constrained_Arrays.अन्तः_2() && !आकार_Arrays_It->second.रिक्तानि();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        ध्रुव स्वचालित emitResizeFallback = [&]() {
            यदि (needs_आकार_Phase) प्रत्यागम;
            यदि (AstTask* ध्रुव resizeAllTaskp
                = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* ध्रुव resizeTaskRefp = नूतन AstTaskRef{fl, resizeAllTaskp};
                randomizep->सङ्कलय_Stmtsp(resizeTaskRefp->makeStmt());
            }
        };
        यदि (!basicFirst) emitResizeFallback();

        AstVarRef* ध्रुव fvarRefReadp = fvarRefp->clone_वृक्षः(असत्य);
        fvarRefReadp->access(VAccess::READ);

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव secondHalfp
            = basicFirst ? beginValp : नूतन AstFuncRef{fl, basicRandomizep};
        randomizep->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{fl, fvarRefp->clone_वृक्षः(असत्य),
                                            नूतन AstAnd{fl, fvarRefReadp, secondHalfp}});

        यदि (basicFirst) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        यदि (classHasRandClassMembers(nodep)) {
            AstTask* ध्रुव postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->सङ्कलय_Stmtsp((नूतन AstTaskRef{fl, postTaskp})->makeStmt());
        }

        सङ्कलय_Pre_Post_Call(nodep, randomizep, "post_randomize");
        nodep->user1(असत्य);
    }
    शून्य दर्शय(AstRandCase* nodep) override {
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
        Ast_ग्रन्थिः_DType* ध्रुव योग_DTypep = nodep->findUInt64DType();

        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        ध्रुव std::string name = "__Vrandcase" + cvt_गन्तव्यम्_Str(m_rand_Case_अङ्कः++);
        AstVar* ध्रुव randVarp = नूतन AstVar{fl, VVarType::BLOCKTEMP, name, योग_DTypep};
        randVarp->noSubst(सत्य);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        सङ्कलय_अस्थायी_Varsp(randVarp);
        Ast_ग्रन्थिः_व्यञ्जकम्* sump = नूतन AstConst{fl, AstConst::Widthed_मानम्{}, 64, 0};
        Ast_ग्रन्थिः_If* ध्रुव firstIfsp
            = नूतन AstIf{fl, नूतन AstConst{fl, AstConst::BitFalse{}}, शून्यसूचकः, शून्यसूचकः};
        Ast_ग्रन्थिः_If* ifsp = firstIfsp;

        प्रति (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = itemp->condsp()->unlink_Fr_पृष्ठभागः();
            sump
                = नूतन Ast_सङ्कलय{condp->fileline(), sump, नूतन AstExtend{itemp->fileline(), condp, 64}};
            Ast_ग्रन्थिः* ध्रुव stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र() : शून्यसूचकः;
            AstVarRef* ध्रुव randVarRefp = नूतन AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_ग्रन्थिः_If* ध्रुव newifp
                = नूतन AstIf{itemp->fileline(),
                            नूतन AstLte{condp->fileline(), randVarRefp, sump->clone_वृक्षः_विशुद्धम्(सत्य)},
                            stmtsp, शून्यसूचकः};
            ifsp->सङ्कलय_Elsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* ध्रुव dispp = नूतन AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            शून्यसूचकः, शून्यसूचकः};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->सङ्कलय_Elsesp(dispp);

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव randp = नूतन AstRand{fl, शून्यसूचकः, असत्य};
        randp->dtype_समुच्चयः_UInt64();
        AstVarRef* ध्रुव randVarRefp = नूतन AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_ग्रन्थिः* ध्रुव newp
            = नूतन Ast_नियोजय{fl, randVarRefp,
                            नूतन Ast_सङ्कलय{fl, नूतन AstConst{fl, AstConst::Unsized64{}, 1},
                                       नूतन AstModDiv{fl, randp, sump}}};
        newp->सङ्कलय_अग्र(firstIfsp);
        यदि (debug() >= 9) newp->dump_वृक्षः_And_अग्र(निर्गमः, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        यदि (nodep->name() == "rand_mode") {
            Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(nodep, विधानम्_Call);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = विधानम्_Callp ? विधानम्_Callp->fromp() : शून्यसूचकः;
            ध्रुव Rand_बहुलकम्_Target rand_बहुलकम्_Target = Rand_बहुलकम्_Target::get(fromp, m_modp);
            UASSERT_OBJ(rand_बहुलकम्_Target.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* ध्रुव receiverp = rand_बहुलकम्_Target.receiverp;
            ध्रुव तार्किक isClassLevel = !(receiverp && receiverp->यादृच्छिकम्().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_ग्रन्थिः* classLevelStaticLoopp = शून्यसूचकः;
            यदि (isClassLevel && nodep->argsp()) {
                यदि (AstVar* ध्रुव sVarp = get_Static_Rand_बहुलकम्_Var(rand_बहुलकम्_Target.classp)) {
                    File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव staticLhsp
                        = make_बहुलकम्_नियोजय_Lhs(fl, rand_बहुलकम्_Target.classp, शून्यसूचकः, sVarp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव argClonep = nodep->argsp()->exprp()->clone_वृक्षः_विशुद्धम्(असत्य);
                    classLevelStaticLoopp = make_बहुलकम्_समुच्चयः_Loop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            ध्रुव तार्किक receiverIsStaticRand
                = receiverp && receiverp->यादृच्छिकम्().isRand() && receiverp->lifetime().isStatic();
            AstVar* ध्रुव rand_बहुलकम्_Varp = receiverIsStaticRand
                                             ? get_Static_Rand_बहुलकम्_Var(rand_बहुलकम्_Target.classp)
                                             : get_Rand_बहुलकम्_Var_From_Class(rand_बहुलकम्_Target.classp);
            यदि (!rand_बहुलकम्_Varp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_व्यञ्जकम्), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(प्रक्षिप_Deletep(m_stmtp), m_stmtp = शून्यसूचकः);
                प्रत्यागम;
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = make_बहुलकम्_नियोजय_Lhs(nodep->fileline(), rand_बहुलकम्_Target.classp,
                                                        rand_बहुलकम्_Target.fromp, rand_बहुलकम्_Varp);
            replace_With_बहुलकम्_नियोजय_And_Append(
                nodep, receiverp && receiverp->यादृच्छिकम्().isRand() ? receiverp : शून्यसूचकः, lhsp,
                classLevelStaticLoopp);
            प्रत्यागम;
        }

        यदि (nodep->name() == "constraint_mode") {
            Ast_विधानम्_Call* ध्रुव विधानम्_Callp = VN_CAST(nodep, विधानम्_Call);
            Ast_ग्रन्थिः_व्यञ्जकम्* fromp = विधानम्_Callp ? विधानम्_Callp->fromp() : शून्यसूचकः;
            AstConstraint* constrp = शून्यसूचकः;
            AstClass* classp = VN_CAST(m_modp, Class);
            यदि (AstConstraintRef* ध्रुव constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                यदि (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } अन्यथा यदि (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* ध्रुव constraint_बहुलकम्_Varp = (constrp && constrp->isStatic())
                                                   ? get_Static_Constraint_बहुलकम्_Var(classp)
                                                   : get_Constraint_बहुलकम्_Var(classp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp
                = make_बहुलकम्_नियोजय_Lhs(nodep->fileline(), classp, fromp, constraint_बहुलकम्_Varp);
            replace_With_बहुलकम्_नियोजय(nodep, constrp, lhsp);
            प्रत्यागम;
        }

        यदि (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            प्रत्यागम;
        }

        यदि (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            ध्रुव तार्किक अन्तर्_Static_Context = m_ftaskp && m_ftaskp->isStatic();
            AstVar* ध्रुव stdrand = createStdRandomGenerator(m_modp);
            AstFunc* ध्रुव randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            यदि (अन्तर्_Static_Context) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(सत्य);
            }
            randomizeFuncp->सङ्कलय_Stmtsp(
                नूतन Ast_नियोजय{nodep->fileline(),
                              नूतन AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              नूतन AstConst{nodep->fileline(), AstConst::Widthed_मानम्{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            पूर्णाङ्क argn = 0;
            AstWith* ध्रुव withp = nodep->withp();
            प्रति (ध्रुव AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = argp->exprp();
                AstCMethodHard* ध्रुव basicMethodp = नूतन AstCMethodHard{
                    nodep->fileline(),
                    नूतन AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* ध्रुव refvarp
                    = नूतन AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::गन्तव्यम्_string(++argn), exprp->dtypep()};
                refvarp->समुच्चयः_Std_Randomize_Arg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                यदि (withp) {
                    withp->foreach([&](Ast_ग्रन्थिः_व्यञ्जकम्* घातांकः) {
                        यदि (is_Similar_ग्रन्थिः(घातांकः, exprp)) {
                            AstVarRef* ध्रुव replaceVar
                                = नूतन AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            घातांकः->replaceWith(replaceVar);
                            replaceVar->user1(घातांकः->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(प्रक्षिप_Deletep(घातांकः), घातांकः);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(सत्य);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->सङ्कलय_Stmtsp(refvarp);

                ध्रुव आकार_t विस्तारः = exprp->विस्तारः();
                basicMethodp->सङ्कलय_Pinsp(
                    नूतन AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->सङ्कलय_Pinsp(
                    नूतन AstConst{nodep->fileline(), AstConst::Unsized64{}, विस्तारः});
                basicMethodp->dtype_समुच्चयः_Bit();

                randomizeFuncp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
                    nodep->fileline(),
                    नूतन AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    नूतन AstAnd{nodep->fileline(),
                               नूतन AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            यदि (withp) {
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, शून्यसूचकः,
                                                                अन्तर्_Static_Context);
                withCapturep->सङ्कलय_Function_Arguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                यदि (stdrand) {
                    randomizeFuncp->सङ्कलय_Stmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_ग्रन्थिः* ध्रुव capturedTreep = withp->exprp()->unlink_Fr_पृष्ठभागः_With_अग्र();
                randomizeFuncp->सङ्कलय_Stmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_व्यञ्जकम्_Visitor{
                        शून्यसूचकः, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        शून्यसूचकः, m_writtenVars, &m_uniqueConstraintId, m_unique_नामानि,  शून्यसूचकः};
                }
                AstCExpr* ध्रुव solverCallp = नूतन AstCExpr{fl};
                solverCallp->dtype_समुच्चयः_Bit();
                solverCallp->सङ्कलय(नूतन AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->सङ्कलय(".next()");
                AstVar* ध्रुव fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव andExprp
                    = नूतन AstAnd{fl, नूतन AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->सङ्कलय_Stmtsp(
                    नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            यदि (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            यदि (withCapturep) nodep->सङ्कलय_Argsp(withCapturep->get_तर्काः());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            प्रत्यागम;
        }
        handle_Randomize_तर्काः(nodep);

        AstWith* ध्रुव withp = nodep->withp();
        यदि (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            प्रत्यागम;
        }
        withp->unlink_Fr_पृष्ठभागः();
        iterateChildren(nodep);

        AstClass* classp = शून्यसूचकः;
        यदि (Ast_विधानम्_Call* ध्रुव callp = VN_CAST(nodep, विधानम्_Call)) {
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
        } अन्यथा {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        यदि (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* ध्रुव classGenp = getRandomGenerator(classp);
        AstVar* ध्रुव localGenp
            = नूतन AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(सत्य);

        AstFunc* ध्रुव randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inline_Unique_नामानि.get(nodep), असत्य);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        ध्रुव std::pair<तार्किक, तार्किक> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<तार्किक, तार्किक>{असत्य, असत्य};
        यदि (prePostWrap.first) {
            सङ्कलय_Virtual_Pre_Post_Call(randomizeFuncp, classp, "pre_randomize");
        } अन्यथा {
            सङ्कलय_Pre_Post_Call(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        यदि (classHasRandClassMembers(classp)) {
            AstTask* ध्रुव preTaskp = getCreateNestedCallbackTask(classp, "pre");
            यदि (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->सङ्कलय_Stmtsp((नूतन AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        ध्रुव CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.सङ्कलय_Function_Arguments(randomizeFuncp);

        // Add constraints clearing code
        यदि (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* ध्रुव mainRandomizep
                = VN_CAST(m_memberMap.findMember(classp, "randomize"), Func);
            यदि (mainRandomizep && mainRandomizep->stmtsp()) {
                प्रति (Ast_ग्रन्थिः* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    तार्किक प्राप्तम्_Clear_Constraints = असत्य;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        यदि (methodp->विधानम्() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->विधानम्() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->सङ्कलय_Stmtsp(stmtp->clone_वृक्षः(असत्य));
                        } अन्यथा यदि (methodp->विधानम्() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            प्राप्तम्_Clear_Constraints = सत्य;
                        }
                    });
                    यदि (प्राप्तम्_Clear_Constraints) भङ्ग;
                }
            }
            randomizeFuncp->सङ्कलय_Stmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->सङ्कलय_Stmtsp(localGenp);

        AstFunc* ध्रुव basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* ध्रुव basicRandomizeFuncCallp
            = नूतन AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        यदि (classGenp) {
            AstTask* ध्रुव constr_सज्जता_Funcp = get_Create_Constraint_सज्जता_Func(classp, m_memberMap);
            AstTaskRef* ध्रुव callp = नूतन AstTaskRef{nodep->fileline(), constr_सज्जता_Funcp};
            randomizeFuncp->सङ्कलय_Stmtsp(callp->makeStmt());
            randomizeFuncp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
                nodep->fileline(), नूतन AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                नूतन AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), ग्रन्थिः_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* ध्रुव rand_बहुलकम्_Varp = get_Rand_बहुलकम्_Var_From_Class(classp);
        यदि (!classGenp && rand_बहुलकम्_Varp) सङ्कलय_समुच्चयः_Rand_बहुलकम्(randomizeFuncp, localGenp, rand_बहुलकम्_Varp);
        यदि (!classGenp) {
            यदि (AstVar* ध्रुव sVarp = get_Static_Rand_बहुलकम्_Var(classp)) {
                सङ्कलय_समुच्चयः_Static_Rand_बहुलकम्(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_ग्रन्थिः* ध्रुव capturedTreep = withp->exprp()->unlink_Fr_पृष्ठभागः_With_अग्र();
        randomizeFuncp->सङ्कलय_Stmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_ग्रन्थिः* inlineResizeStmtsp = शून्यसूचकः;
        {
            std::सदिशम्<AstCMethodHard*> आकार_Methodps;
            capturedTreep->foreach_And_अग्र([&](AstCMethodHard* methodp) {
                यदि (methodp->विधानम्() == VCMethod::DYN_आकार
                    || methodp->विधानम्() == VCMethod::ASSOC_आकार) {
                    आकार_Methodps.प्रक्षिप_पृष्ठभागः(methodp);
                }
            });
            प्रति (AstCMethodHard* ध्रुव methodp : आकार_Methodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = शून्यसूचकः;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मूलम्_Exprp = methodp->fromp();
                यदि (AstMemberSel* memberSelp = VN_CAST(मूलम्_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    यावत् (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = memberSelp->fromp();
                    यदि (ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(fromp, VarRef)) {
                        यदि (varRefp->varp() == captured.getThisp()) अनुवर्त;
                    } अन्यथा {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } अन्यथा यदि (AstVarRef* ध्रुव varRefp = VN_CAST(मूलम्_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                यदि (!arrVarp) अनुवर्त;
                // Only handle rand-declared dynamic/assoc array variables
                यदि (!arrVarp->यादृच्छिकम्().isRandomizable()) अनुवर्त;
                // Do not randomize associative array size (IEEE 18.4)
                यदि (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) अनुवर्त;
                File_पङ्क्तिः* ध्रुव fl = methodp->fileline();
                तार्किक wasCreated = असत्य;
                AstClass* ध्रुव varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* ध्रुव आकार_Varp = create_Or_Get_आकार_Var(varClassp, arrVarp, fl,
                                                            methodp->findIntDType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                यदि (wasCreated) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resizeVarRefp = buildMemberSelExprp(मूलम्_Exprp, arrVarp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव आकार_Exprp = buildMemberSelExprp(मूलम्_Exprp, आकार_Varp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->सङ्कलय_अग्र(create_आकार_Gte_Zero_Constraint(fl, आकार_Varp, मूलम्_Exprp));
                    AstCMethodHard* ध्रुव resizep
                        = नूतन AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, आकार_Exprp};
                    resizep->dtypep(methodp->findVoidDType());
                    inlineResizeStmtsp
                        = Ast_ग्रन्थिः::सङ्कलय_अग्र(inlineResizeStmtsp, नूतन Ast_Stmt_व्यञ्जकम्{fl, resizep});
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव आकार_ग्रन्थिः_Exprp = buildMemberSelExprp(मूलम्_Exprp, आकार_Varp);
                methodp->replaceWith(आकार_ग्रन्थिः_Exprp);
                VL_DO_DANGLING(methodp->delete_वृक्षः(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_व्यञ्जकम्_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                rand_बहुलकम्_Varp, m_writtenVars, &m_uniqueConstraintId, m_unique_नामानि,  शून्यसूचकः};
        }

        // Call the solver and set return value
        AstCExpr* ध्रुव solverCallp = नूतन AstCExpr{nodep->fileline()};
        solverCallp->dtype_समुच्चयः_Bit();
        solverCallp->सङ्कलय(नूतन AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->सङ्कलय(".next(__Vm_rng)");
        randomizeFuncp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{
            nodep->fileline(),
            नूतन AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            नूतन AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        यदि (inlineResizeStmtsp) {
            AstVar* ध्रुव retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* ध्रुव ifp = नूतन AstIf{nodep->fileline(),
                                         नूतन AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, शून्यसूचकः};
            randomizeFuncp->सङ्कलय_Stmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        यदि (classHasRandClassMembers(classp)) {
            AstTask* ध्रुव postTaskp = getCreateNestedCallbackTask(classp, "post");
            यदि (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->सङ्कलय_Stmtsp((नूतन AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        यदि (prePostWrap.second) {
            सङ्कलय_Virtual_Pre_Post_Call(randomizeFuncp, classp, "post_randomize");
        } अन्यथा {
            सङ्कलय_Pre_Post_Call(classp, randomizeFuncp, "post_randomize");
        }

        // Replace the node with a call to that function
        nodep->name(randomizeFuncp->name());
        nodep->सङ्कलय_Argsp(captured.get_तर्काः());
        nodep->taskp(randomizeFuncp);
        nodep->dtypeFrom(randomizeFuncp->dtypep());
        nodep->classOrPackagep(classp);
        UINFO(9, "Added `%s` randomization procedure");
        wrapRandomizeCallWithNullGuard(nodep);
        VL_DO_DANGLING(withp->delete_वृक्षः(), withp);
    }
    शून्य दर्शय(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        यदि (m_constraintp && nodep->fromp()->user1()
            && (nodep->विधानम्() == VCMethod::ASSOC_आकार
                || nodep->विधानम्() == VCMethod::DYN_आकार)) {
            AstClass* ध्रुव classp = VN_AS(m_modp, Class);
            AstVarRef* ध्रुव queueVarRefp = VN_CAST(nodep->fromp(), VarRef);
            यदि (!queueVarRefp) {
                // Warning from ConstraintExprVisitor will be thrown
                प्रत्यागम;
            }
            AstVar* ध्रुव queueVarp = queueVarRefp->varp();
            तार्किक wasCreated = असत्य;
            AstVar* ध्रुव आकार_Varp
                = create_Or_Get_आकार_Var(classp, queueVarp, fl, nodep->findIntDType(), wasCreated);
            यदि (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                यदि (!VN_IS(queueVarp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    यदि (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* ध्रुव आकार_Var_Refp = नूतन AstVarRef{
                        fl, VN_AS(आकार_Varp->user2p(), ग्रन्थिः_Module), आकार_Varp, VAccess::READ};
                    AstCMethodHard* ध्रुव resizep = नूतन AstCMethodHard{
                        fl, nodep->fromp()->unlink_Fr_पृष्ठभागः(), VCMethod::DYN_RESIZE, आकार_Var_Refp};
                    resizep->dtypep(nodep->findVoidDType());
                    resizerTaskp->सङ्कलय_Stmtsp(नूतन Ast_Stmt_व्यञ्जकम्{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->सङ्कलय_Itemsp(create_आकार_Gte_Zero_Constraint(fl, आकार_Varp));
            }
            AstVarRef* ध्रुव आकार_Var_Refp = नूतन AstVarRef{fl, VN_AS(आकार_Varp->user2p(), ग्रन्थिः_Module),
                                                         आकार_Varp, VAccess::READ};
            आकार_Var_Refp->user1(सत्य);
            nodep->replaceWith(आकार_Var_Refp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट RandomizeVisitor(AstNetlist* nodep)
        : m_inline_Unique_नामानि{"__Vrandwith"} {
        createRandomizeClassVars(nodep);
        // Flag local constraint leaves as solver-owned so __VBasicRand skips them.
        // Runs before any class is lowered. Only a randomized class counts, and
        // only a leaf owned by the constraint's own class: a leaf reached through
        // a global constraint stays basic-randomized so a standalone randomize()
        // of its type still randomizes it (issue #7833); a class-typed sub-object
        // is skipped so its own members are still basic-randomized.
        nodep->foreach([&](AstClass* ध्रुव classp) {
            यदि (!classp->user1()) प्रत्यागम;
            classp->foreachMember([&](AstClass* ध्रुव ownerClassp, AstConstraint* ध्रुव constrp) {
                constrp->foreach([&](AstMemberSel* ध्रुव memberSelp) {
                    AstVar* ध्रुव varp = memberSelp->varp();
                    यदि (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) प्रत्यागम;
                    यदि (VN_AS(varp->user2p(), ग्रन्थिः_Module) != ownerClassp) प्रत्यागम;
                    यदि (!varp->user3()) varp->user3(सत्य);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(constrp->unlink_Fr_पृष्ठभागः()), constrp);
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
    यदि (!funcp) {
        v3Global.useRandomizeMethods(सत्य);
        Ast_ग्रन्थिः_DType* ध्रुव dtypep
            = childDType
                  ? नूतन AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* ध्रुव fvarp = childDType
                                  ? नूतन AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlagChildDType{}, dtypep}
                                  : नूतन AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3_Error_संहिता::NORETURN, सत्य);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(सत्य);
        fvarp->funcReturn(सत्य);
        fvarp->direction(VDirection::OUTPUT);
        nodep->सङ्कलय_Membersp(funcp);
        funcp = नूतन AstFunc{nodep->fileline(), name, शून्यसूचकः, fvarp};
        यदि (!childDType) funcp->dtypep(dtypep);
        funcp->class_विधानम्(सत्य);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->सङ्कलय_Membersp(funcp);
        memberMap.निवेशय(nodep, funcp);
    }
    प्रत्यागम funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_ग्रन्थिः_Module* nodep, ध्रुव std::string& name) {
    AstFunc* funcp = शून्यसूचकः;
    v3Global.useRandomizeMethods(सत्य);
    Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
    AstVar* ध्रुव fvarp = नूतन AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(सत्य);
    fvarp->funcReturn(सत्य);
    fvarp->direction(VDirection::OUTPUT);
    funcp = नूतन AstFunc{nodep->fileline(), name, शून्यसूचकः, fvarp};
    funcp->dtypep(dtypep);
    यदि (VN_IS(nodep, Class)) {
        funcp->class_विधानम्(सत्य);
    } अन्यथा {
        funcp->class_विधानम्(सत्य);
        funcp->isStatic(सत्य);
    }
    nodep->सङ्कलय_Stmtsp(funcp);
    प्रत्यागम funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* ध्रुव basep = nodep->आधारः_Most_Classp();
    AstFunc* funcp = VN_AS(memberMap.findMember(basep, "srandom"), Func);
    यदि (!funcp) {
        v3Global.useRandomizeMethods(सत्य);
        Ast_ग्रन्थिः_DType* ध्रुव dtypep
            = basep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* ध्रुव ivarp = नूतन AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(सत्य);
        ivarp->direction(VDirection::निवेशः);
        funcp = नूतन AstFunc{basep->fileline(), "srandom", ivarp, शून्यसूचकः};
        funcp->dtypep(basep->findVoidDType());
        funcp->class_विधानम्(सत्य);
        funcp->isVirtual(असत्य);
        basep->सङ्कलय_Membersp(funcp);
        memberMap.निवेशय(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        यदि (basep->name() == "process") {
            funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } अन्यथा {
            funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(सत्य);
        }
    }
    प्रत्यागम funcp;
}
