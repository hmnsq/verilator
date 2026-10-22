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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Randomize.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3Container.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3MemberMap.h"
#അന്തര്ഭാവ "V3Task.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <queue>
#അന്തര്ഭാവ <tuple>
#അന്തര്ഭാവ <utility>

VL_DEFINE_DEBUG_FUNCTIONS;

// ######################################################################
// Determines if a class is used with randomization

എണ്ണൽ ClassRandom : uint8_t {
    NONE,  // randomize() is not called
    IS_RANDOMIZED,  // randomize() is called
    IS_RANDOMIZED_GLOBAL,  // randomize() is called with global constraints
    IS_RANDOMIZED_INLINE,  // randomize() with args is called
    IS_STD_RANDOMIZED,  // std::randomize() is called
};

// ######################################################################
// Constants for global constraint processing

സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* GLOBAL_CONSTRAINT_SEPARATOR = "__DT__";
സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* BASIC_RANDOMIZE_FUNC_NAME = "__VBasicRand";

// Walk extends chain to find __Vrandmode variable (stored in AstClass::user2p).
// user2p is only set on the root class where __Vrandmode was created, so
// derived classes need this chain walk. Accepts AstNodeModule* for flexibility.
സ്ഥിതം AstVar* getRandModeVarFromClass(Ast_കണ്ണി_Module* classp) {
    ആകുമ്പോൾ (classp) {
        എങ്കിൽ (classp->user2p()) തിരികെ VN_AS(classp->user2p(), Var);
        AstClass* സ്ഥിരം cp = VN_CAST(classp, Class);
        എങ്കിൽ (!cp || !cp->extendsp()) തിരികെ nullptr;
        classp = cp->extendsp()->classp();
    }
    തിരികെ nullptr;
}

// Walk extends chain to find random generator variable (stored in AstClass::user3p).
സ്ഥിതം AstVar* getRandomGenerator(AstClass* സ്ഥിരം classp) {
    എങ്കിൽ (classp->user3p()) തിരികെ VN_AS(classp->user3p(), Var);
    എങ്കിൽ (classp->extendsp()) തിരികെ getRandomGenerator(classp->extendsp()->classp());
    തിരികെ nullptr;
}

// Create __Vsetup_constraints function if non existant inside memberMap,
// if existant return one already present
സ്ഥിതം AstTask* getCreateConstraintSetupFunc(AstClass* classp, VMemberMap& memberMap) {
    സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം name = "__Vsetup_constraints";
    AstTask* setupAllTaskp = VN_AS(memberMap.findMember(classp, name), Task);
    എങ്കിൽ (setupAllTaskp) തിരികെ setupAllTaskp;
    setupAllTaskp = പുതിയ AstTask{classp->fileline(), "__Vsetup_constraints", nullptr};
    setupAllTaskp->classMethod(സത്യം);
    setupAllTaskp->isVirtual(സത്യം);
    classp->addMembersp(setupAllTaskp);
    memberMap.തിരുകുക(classp, setupAllTaskp);
    തിരികെ setupAllTaskp;
}

// ######################################################################
// Establishes the target of a rand_mode() call

ഘടന RandModeTarget final {
    // MEMBERS
    AstVar* സ്ഥിരം receiverp;  // Variable that is the target of the rand_mode() call
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp;  // Expression from which the rand_mode() call originates
    AstClass* സ്ഥിരം classp;  // Class in which rand_mode should be set

    // METHODS
    സ്ഥിതം RandModeTarget get(Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_Module* modp) {
        എങ്കിൽ (!fromp) തിരികെ {nullptr, nullptr, VN_AS(modp, Class)};
        എങ്കിൽ (AstCMethodHard* സ്ഥിരം methodHardp = VN_CAST(fromp, CMethodHard)) {
            തിരികെ RandModeTarget::get(methodHardp->fromp(), modp);
        }
        AstVar* receiverp = nullptr;
        AstClass* classp = VN_CAST(modp, Class);
        എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(fromp, VarRef)) {
            receiverp = varrefp->varp();
            എങ്കിൽ (receiverp->isRand()) {
                fromp = nullptr;
                എങ്കിൽ (receiverp->lifetime().isStatic()) {
                    classp = VN_AS(varrefp->classOrPackagep(), Class);
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(fromp, MemberSel)) {
            receiverp = memberSelp->varp();
            എങ്കിൽ (receiverp->isRand()) {
                fromp = memberSelp->fromp();
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
        }
        UASSERT_OBJ(receiverp, fromp, "Unknown rand_mode() receiver");
        എങ്കിൽ (!receiverp->isRand()) {
            AstClassRefDType* സ്ഥിരം classRefDtp
                = VN_CAST(receiverp->dtypep()->skipRefp(), ClassRefDType);
            എങ്കിൽ (classRefDtp) classp = classRefDtp->classp();
        }
        തിരികെ {receiverp, fromp, classp};
    }
};

// ######################################################################
// Stores info about a variable's rand_mode state/a constraint's constraint_mode state

സംഘം RandomizeMode final {
    // MEMBERS
    ഘടന {
        തർക്കം usesMode : 1;  // Variable/constraint uses rand_mode/constraint_mode
        uint32_t index : 31;  // Index of var/constraint in rand_mode/constraint_mode vector
    };
    uint64_t asUQuad;  // Representation as int to be stored in nodep->user*
};

// Look through unpacked array dimensions to the element type
സ്ഥിതം Ast_കണ്ണി_DType* arrayElementDTypep(Ast_കണ്ണി_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    ആകുമ്പോൾ (dtypep->isNonPackedArray()) dtypep = dtypep->subDTypep()->skipRefp();
    തിരികെ dtypep;
}

// Whether dtypep is, or contains anywhere through unpacked array/struct
// nesting, a real value. A packed struct can't hold one (IEEE disallows
// real as a packed-struct member), so it's never worth descending into.
സ്ഥിതം തർക്കം dtypeContainsReal(Ast_കണ്ണി_DType* dtypep) {
    dtypep = dtypep->skipRefp();
    എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം subp = dtypep->subDTypep()) തിരികെ dtypeContainsReal(subp);
    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം structp = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType)) {
        എങ്കിൽ (structp->packed()) തിരികെ അസത്യം;
        വേണ്ടി (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            എങ്കിൽ (dtypeContainsReal(memberp->subDTypep())) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = VN_CAST(dtypep, BasicDType);
    തിരികെ basicp && basicp->isDouble();
}

// Check a rand/randc variable's type against IEEE 1800-2023 18.4's
// allowed list. Purely diagnostic: codegen sites that would otherwise
// mishandle one of these types guard themselves on V3Error::errorCount()
// rather than relying on anything checked here.
സ്ഥിതം ശൂന്യം checkRandTypeEligibility(Ast_കണ്ണി* contextp, Ast_കണ്ണി_DType* dtypep, തർക്കം isRandc) {
    dtypep = dtypep->skipRefp();
    എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം subp = dtypep->subDTypep()) {
        checkRandTypeEligibility(contextp, subp, isRandc);  // Containers inherit rand/randc kind
        തിരികെ;
    }
    എങ്കിൽ (VN_IS(dtypep, ClassRefDType)) {
        // rand on a class handle is legal (recursive randomization).
        // Only randc is disallowed.
        എങ്കിൽ (isRandc) {
            contextp->v3error("'randc' on an object handle (IEEE 1800-2023 18.4: object "
                              "handles shall not be declared randc)");
        }
        തിരികെ;
    }
    എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifacep = VN_CAST(dtypep, IfaceRefDType)) {
        // A class property can only hold a *virtual* interface handle --
        // a plain interface can't be a class member at all -- so a
        // rand-qualified var landing here is always the virtual kind.
        UASSERT_OBJ(ifacep->isVirtual(), dtypep, "Non-virtual interface as class property type");
        // Not in 18.4's enumerated random-variable domain, same reasoning
        // as chandle/string/event; also generates C++ that does not
        // compile if left unchecked.
        contextp->v3error("'rand'/'randc' on a virtual interface handle (not "
                          "in IEEE 1800-2023 18.4's random-variable type domain)");
        തിരികെ;
    }
    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം structp = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType)) {
        എങ്കിൽ (structp->packed()) തിരികെ;  // Integral by construction; members checked separately
        എങ്കിൽ (VN_IS(structp, UnionDType)) {
            contextp->v3error("'rand'/'randc' on an unpacked union (IEEE "
                              "1800-2023 18.4: unpacked unions shall not be declared as rand "
                              "or randc)");
            തിരികെ;
        }
        വേണ്ടി (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Each unpacked-struct member independently declares its own rand/randc.
            എങ്കിൽ (memberp->rand().isRandomizable()) {
                checkRandTypeEligibility(memberp, memberp->subDTypep(), memberp->rand().isRandC());
            }
        }
        തിരികെ;
    }
    സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = VN_CAST(dtypep, BasicDType);
    // V3Width has already resolved every dtype by this pass, and every
    // other leaf kind (class handle, virtual interface, struct/union) was
    // already dispatched above, so a rand-qualified var's dtype can only
    // be a BasicDType here.
    UASSERT_OBJ(basicp, dtypep, "Unexpected non-basic leaf dtype for rand var");
    എങ്കിൽ (basicp->isDouble()) {
        // rand on a real variable is legal; only randc is disallowed.
        എങ്കിൽ (isRandc) {
            contextp->v3error("'randc' on a real variable (IEEE 1800-2023 18.4: "
                              "real variables shall not be declared randc)");
        }
        തിരികെ;
    }
    തിരഞ്ഞെടുപ്പ് (basicp->keyword()) {
    അവസ്ഥ VBasicDTypeKwd::STRING:
    അവസ്ഥ VBasicDTypeKwd::CHANDLE:
    അവസ്ഥ VBasicDTypeKwd::EVENT: {
        സ്ഥിരം അക്ഷരം* സ്ഥിരം articlep = basicp->keyword() == VBasicDTypeKwd::EVENT ? "an" : "a";
        contextp->v3error("'rand'/'randc' on " << articlep << " " << basicp->keyword().typeName()
                                               << " variable (not in IEEE 1800-2023 18.4's "
                                                  "random-variable type domain)");
        നിർത്തുക;
    }
    സ്ഥിരസ്ഥിതി: നിർത്തുക;
    }
}

//######################################################################
// Visitor that marks classes needing a randomize() method

വർഗ്ഗം RandomizeMarkVisitor final : പൊതു VNVisitor {
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
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;

    using DerivedSet = std::unordered_set<AstClass*>;
    using BaseToDerivedMap = std::unordered_map<സ്ഥിരം AstClass*, DerivedSet>;

    BaseToDerivedMap m_baseToDerivedMap;  // Mapping from base classes to classes that extend them
    AstClass* m_classp = nullptr;  // Current class
    Ast_കണ്ണി* m_constraint_വ്യഞ്ജകം_Genp = nullptr;  // Current constraint or constraint if expression
    Ast_കണ്ണി_Module* m_modp;  // Current module
    Ast_കണ്ണി_Stmt* m_stmtp = nullptr;  // Current statement
    Ast_കണ്ണി_FTask_Ref* m_stdRandCallp = nullptr;  // Current std::randomize() call
    തർക്കം m_inStdWith = അസത്യം;  // True when inside a 'with {}' clause
    std::set<Ast_കണ്ണി_Var_Ref*> m_staticRefs;  // References to static variables under `with` clauses
    AstWith* m_withp = nullptr;  // Current 'with' constraint node

    // METHODS
    // Mark all rand variables in IS_RANDOMIZED_GLOBAL classes as globalConstrained
    ശൂന്യം markGlobalConstrainedVars(AstClass* classp) {
        വേണ്ടി (സ്ഥിരം AstClass* cp = classp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            വേണ്ടി (Ast_കണ്ണി* memberp = cp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* സ്ഥിരം varp = VN_CAST(memberp, Var);
                എങ്കിൽ (!varp) തുടരുക;
                എങ്കിൽ (varp->rand().isRandomizable()) varp->globalConstrained(സത്യം);
            }
        }
    }
    // Check if a variable is listed in std::randomize() arguments
    തർക്കം isVarInStdRandomizeArgs(സ്ഥിരം AstVar* varp) സ്ഥിരം {
        എങ്കിൽ (!m_inStdWith || !m_stdRandCallp) തിരികെ അസത്യം;

        വേണ്ടി (AstArg* argp = m_stdRandCallp->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp = argp->exprp();
            // Traverse through expression to find the base variable
            ആകുമ്പോൾ (exprp) {
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(exprp, കണ്ണി_Var_Ref)) {
                    എങ്കിൽ (varrefp->varp() == varp) തിരികെ സത്യം;
                    നിർത്തുക;
                }
                എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം memberselp = VN_CAST(exprp, MemberSel)) {
                    എങ്കിൽ (memberselp->varp() == varp) തിരികെ സത്യം;
                    exprp = memberselp->fromp();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം arrselp = VN_CAST(exprp, ArraySel)) {
                    exprp = arrselp->fromp();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം strselp = VN_CAST(exprp, StructSel)) {
                    exprp = strselp->fromp();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCMethodHard* സ്ഥിരം methodp = VN_CAST(exprp, CMethodHard)) {
                    exprp = methodp->fromp();
                } അല്ലെങ്കിൽ {
                    നിർത്തുക;
                }
            }
        }
        തിരികെ അസത്യം;
    }
    ശൂന്യം markMembers(സ്ഥിരം AstClass* nodep) {
        വേണ്ടി (സ്ഥിരം AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            വേണ്ടി (Ast_കണ്ണി* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                AstVar* സ്ഥിരം varp = VN_CAST(memberp, Var);
                എങ്കിൽ (!varp) തുടരുക;
                // If member is randomizable and of class type, mark its class
                എങ്കിൽ (varp->rand().isRandomizable()) {
                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം varDtypep = varp->dtypep()->skipRefp();
                    സ്ഥിരം AstClassRefDType* classRefp = VN_CAST(varDtypep, ClassRefDType);
                    എങ്കിൽ (!classRefp) {
                        സ്ഥിരം Ast_കണ്ണി_DType* subDTypep = varDtypep->subDTypep();
                        എങ്കിൽ (subDTypep) subDTypep = subDTypep->skipRefp();
                        classRefp = VN_CAST(subDTypep, ClassRefDType);
                    }
                    എങ്കിൽ (classRefp) {
                        AstClass* സ്ഥിരം rclassp = classRefp->classp();
                        എങ്കിൽ (!rclassp->user1()) {
                            rclassp->user1(IS_RANDOMIZED_GLOBAL);
                            markGlobalConstrainedVars(rclassp);
                            markMembers(rclassp);
                            markDerived(rclassp);
                        }
                    }
                    // If the class is randomized inline, all members use rand mode
                    എങ്കിൽ (nodep->user1() == IS_RANDOMIZED_INLINE) {
                        RandomizeMode randMode = {};
                        randMode.usesMode = സത്യം;
                        varp->user1(randMode.asUQuad);
                    }
                }
            }
        }
    }
    ശൂന്യം markDerived(സ്ഥിരം AstClass* nodep) {
        സ്ഥിരം auto it = m_baseToDerivedMap.find(nodep);
        എങ്കിൽ (it != m_baseToDerivedMap.end()) {
            വേണ്ടി (auto* classp : it->second) {
                എങ്കിൽ (classp->user1() < nodep->user1()) {
                    classp->user1(nodep->user1());
                    markMembers(classp);
                    markDerived(classp);
                }
            }
        }
    }
    ശൂന്യം markAllDerived() {
        വേണ്ടി (സ്ഥിരം auto& p : m_baseToDerivedMap) {
            എങ്കിൽ (p.first->user1()) markDerived(p.first);
        }
    }
    ശൂന്യം setPackageRefs() {
        വേണ്ടി (Ast_കണ്ണി_Var_Ref* staticRefp : m_staticRefs) {
            UINFO(9, "Updated classOrPackage ref for " << staticRefp->name());
            staticRefp->classOrPackagep(VN_AS(staticRefp->varp()->user2p(), കണ്ണി_Module));
        }
    }
    ശൂന്യം markNestedGlobalConstrainedRecurse(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
            AstVar* സ്ഥിരം varp = refp->varp();
            എങ്കിൽ (varp->globalConstrained()) തിരികെ;
            varp->globalConstrained(സത്യം);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(nodep, MemberSel)) {
            എങ്കിൽ (memberSelp->varp()) {
                AstVar* സ്ഥിരം varp = memberSelp->varp();
                എങ്കിൽ (varp->globalConstrained()) തിരികെ;
                varp->globalConstrained(സത്യം);
            }
            markNestedGlobalConstrainedRecurse(memberSelp->fromp());
        }
    }

    // Build MemberSel chain from variable path
    Ast_കണ്ണി_വ്യഞ്ജകം* buildMemberSelChain(AstVarRef* വേര്_Var_Refp, സ്ഥിരം std::vector<AstVar*>& path) {
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = വേര്_Var_Refp->clone_വൃക്ഷം(അസത്യം);
        വേണ്ടി (AstVar* memberVarp : path) {
            AstMemberSel* memberSelp
                = പുതിയ AstMemberSel{വേര്_Var_Refp->fileline(), exprp, memberVarp};
            memberSelp->user2p(m_classp);
            exprp = memberSelp;
        }
        തിരികെ exprp;
    }

    // Process a single constraint during nested constraint cloning
    ശൂന്യം processNestedConstraint(AstConstraint* സ്ഥിരം constrp, AstVarRef* വേര്_Var_Refp,
                                 സ്ഥിരം std::vector<AstVar*>& newPath, AstClass* targetClassp) {
        std::string pathPrefix = വേര്_Var_Refp->name();
        വേണ്ടി (AstVar* pathMemberVarp : newPath) {
            pathPrefix += GLOBAL_CONSTRAINT_SEPARATOR + pathMemberVarp->name();
        }

        സ്ഥിരം std::string newName = pathPrefix + GLOBAL_CONSTRAINT_SEPARATOR + constrp->name();

        // Check if this constraint already exists in the target class
        തർക്കം isDuplicate = അസത്യം;
        targetClassp->foreachMember([&](AstClass* സ്ഥിരം, AstConstraint* സ്ഥിരം existingConstrp) {
            എങ്കിൽ (existingConstrp->name() == newName) {
                // Multiple paths lead to same constraint - unsupported pattern
                isDuplicate = സത്യം;
            }
        });
        എങ്കിൽ (isDuplicate) തിരികെ;

        AstConstraint* സ്ഥിരം cloneConstrp = constrp->clone_വൃക്ഷം(അസത്യം);
        cloneConstrp->name(newName);

        // FuncRefs are converted after the traversal. Converting moves the args under the new
        // MethodCall, where foreach would not visit VarRefs or nested calls such as add(lim()).
        std::vector<AstFuncRef*> funcRefps;
        cloneConstrp->foreach([&](Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
            എങ്കിൽ (AstVarRef* സ്ഥിരം varRefp = VN_CAST(exprp, VarRef)) {
                എങ്കിൽ (!varRefp->varp()->isClassMember()) തിരികെ;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം chainp = buildMemberSelChain(വേര്_Var_Refp, newPath);
                AstMemberSel* സ്ഥിരം finalSelp
                    = പുതിയ AstMemberSel{varRefp->fileline(), chainp, varRefp->varp()};
                finalSelp->user2p(m_classp);
                varRefp->replaceWith(finalSelp);
                VL_DO_DANGLING(തള്ളുക_Deletep(varRefp), varRefp);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstFuncRef* സ്ഥിരം funcRefp = VN_CAST(exprp, FuncRef)) {
                // Static and non-member calls are correctly qualified already
                എങ്കിൽ (funcRefp->taskp()->classMethod() && !funcRefp->taskp()->isStatic()) {
                    funcRefps.തള്ളുക_back(funcRefp);
                }
            }
        });
        വേണ്ടി (AstFuncRef* funcRefp : funcRefps) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം chainp = buildMemberSelChain(വേര്_Var_Refp, newPath);
            AstArg* സ്ഥിരം argsp
                = funcRefp->argsp() ? funcRefp->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* സ്ഥിരം callp
                = പുതിയ AstMethodCall{funcRefp->fileline(), chainp, funcRefp->name(), argsp};
            callp->taskp(funcRefp->taskp());
            callp->classOrPackagep(funcRefp->classOrPackagep());
            callp->dtypep(funcRefp->dtypep());
            callp->superReference(funcRefp->superReference());
            funcRefp->replaceWith(callp);
            VL_DO_DANGLING(തള്ളുക_Deletep(funcRefp), funcRefp);
        }
        funcRefps.clear();

        // Add constraint directly to the target class
        targetClassp->addStmtsp(cloneConstrp);
        // Immediately iterate to set user1 marks
        iterateConst(cloneConstrp);
    }

    // Clone constraints from nested rand class members
    ശൂന്യം cloneNestedConstraintsRecurse(AstVarRef* വേര്_Var_Refp, AstClass* classp,
                                       സ്ഥിരം std::vector<AstVar*>& pathToClass,
                                       AstClass* targetClassp) {
        വേണ്ടി (Ast_കണ്ണി* memberNodep = classp->membersp(); memberNodep;
             memberNodep = memberNodep->nextp()) {
            AstVar* സ്ഥിരം memberVarp = VN_CAST(memberNodep, Var);
            എങ്കിൽ (!memberVarp) തുടരുക;
            എങ്കിൽ (!memberVarp->rand().isRandomizable()) തുടരുക;
            സ്ഥിരം AstClassRefDType* സ്ഥിരം memberClassRefp
                = VN_CAST(memberVarp->dtypep()->skipRefp(), ClassRefDType);
            എങ്കിൽ (!memberClassRefp || !memberClassRefp->classp()) തുടരുക;

            AstClass* nestedClassp = memberClassRefp->classp();

            std::vector<AstVar*> newPath = pathToClass;
            newPath.തള്ളുക_back(memberVarp);
            // Replace all variable references inside the cloned constraint with proper
            // member selections
            nestedClassp->foreachMember(
                [&](AstClass* സ്ഥിരം containingClassp, AstConstraint* സ്ഥിരം constrp) {
                    processNestedConstraint(constrp, വേര്_Var_Refp, newPath, targetClassp);
                });
            cloneNestedConstraintsRecurse(വേര്_Var_Refp, nestedClassp, newPath, targetClassp);
        }
    }

    // Get randomized variables from (std::)randomize() arguments
    // and mark AstNodeModule nodes in which they are defined
    ശൂന്യം handleRandomizeArgument(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstVar* സ്ഥിരം varp, സ്ഥിരം തർക്കം stdRandomize) {
        // IEEE 1800-2023 18.11: "Arguments are limited to the names of properties
        // of the calling object; expressions are not allowed."
        // However, for compatibility with other simulators, we support complex
        // expressions like obj.member[idx].field in inline randomize().
        ആകുമ്പോൾ (exprp) {
            AstVar* randVarp = nullptr;
            എങ്കിൽ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(exprp, MemberSel)) {
                randVarp = memberSelp->varp();
                exprp = memberSelp->fromp();
            } അല്ലെങ്കിൽ എങ്കിൽ (AstArraySel* സ്ഥിരം arraySelp = VN_CAST(exprp, ArraySel)) {
                exprp = arraySelp->fromp();
                തുടരുക;  // Skip ArraySel, continue traversing
            } അല്ലെങ്കിൽ എങ്കിൽ (AstStructSel* സ്ഥിരം structSelp = VN_CAST(exprp, StructSel)) {
                exprp = structSelp->fromp();
                തുടരുക;  // Skip StructSel, continue traversing
            } അല്ലെങ്കിൽ എങ്കിൽ (AstCMethodHard* സ്ഥിരം methodp = VN_CAST(exprp, CMethodHard)) {
                exprp = methodp->fromp();
                തുടരുക;
            } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(exprp, VarRef)) {
                randVarp = varrefp->varp();
                varrefp->user1(സത്യം);
                varrefp->access(VAccess::READWRITE);
                exprp = nullptr;
            } അല്ലെങ്കിൽ {
                // All invalid and unsupported expressions should be caught in V3Width
                exprp->v3fatalSrc("Unexpected expression type in randomize() argument");
            }
            UASSERT_OBJ(randVarp, exprp, "No rand variable found");
            എങ്കിൽ (randVarp == varp) തിരികെ;
            Ast_കണ്ണി* backp = randVarp;
            ആകുമ്പോൾ (backp && !VN_IS(backp, കണ്ണി_Module)) backp = backp->backp();
            എങ്കിൽ (stdRandomize) {
                UASSERT_OBJ(backp, randVarp, "No class or module found for rand variable");
                backp->user1(IS_STD_RANDOMIZED);
            } അല്ലെങ്കിൽ {
                // Inline randomized then
                UASSERT_OBJ(VN_IS(backp, Class), randVarp,
                            "No class found for inline randomized variable");
                RandomizeMode randMode = {};
                randMode.usesMode = സത്യം;
                randVarp->user1(randMode.asUQuad);
                backp->user1(IS_RANDOMIZED_INLINE);
            }
        }
    }

    // VISITORS
    ശൂന്യം visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_modp);
        m_modp = m_classp = nodep;
        iterateChildrenConst(nodep);
        എങ്കിൽ (nodep->extendsp()) {
            // Record derived class for inheritance hierarchy tracking
            സ്ഥിരം AstClass* സ്ഥിരം basep = nodep->extendsp()->classp();
            m_baseToDerivedMap[basep].തിരുകുക(nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std")
            m_stdRandCallp = nodep;
        iterateChildrenConst(nodep);
        എങ്കിൽ (nodep->name() == "rand_mode") {
            AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(nodep, MethodCall);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = methodCallp ? methodCallp->fromp() : nullptr;
            തർക്കം valid = സത്യം;
            എങ്കിൽ (VN_IS(fromp, ArraySel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked array element");
                valid = അസത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(fromp, StructSel)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: 'rand_mode()' on unpacked struct element");
                valid = അസത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(fromp, Sel)) {
                nodep->v3error("Cannot call 'rand_mode()' on packed array element");
                valid = അസത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (AstCMethodHard* സ്ഥിരം methodHardp = VN_CAST(fromp, CMethodHard)) {
                എങ്കിൽ (methodHardp->method() == VCMethod::ARRAY_AT
                    || methodHardp->method() == VCMethod::ARRAY_AT_WRITE) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'rand_mode()' on dynamic array element");
                    valid = അസത്യം;
                } അല്ലെങ്കിൽ {
                    methodHardp->v3fatalSrc("Unknown rand_mode() receiver");
                }
            }
            എങ്കിൽ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)
                && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = അസത്യം;
            }
            എങ്കിൽ (valid) {
                സ്ഥിരം RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_classp);
                എങ്കിൽ ((!randModeTarget.receiverp || !randModeTarget.receiverp->isRand())
                    && !nodep->argsp()) {
                    nodep->v3error(
                        "Cannot call 'rand_mode()' as a function on non-random variable");
                    valid = അസത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (!randModeTarget.classp) {
                    nodep->v3error("Cannot call 'rand_mode()' on non-random, non-class variable");
                    valid = അസത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) {
                    nodep->v3error("'rand_mode()' with arguments cannot be called as a function");
                    valid = അസത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (randModeTarget.receiverp && randModeTarget.receiverp->isRand()) {
                    // Called on a rand member variable
                    RandomizeMode randMode = {};
                    randMode.usesMode = സത്യം;
                    randModeTarget.receiverp->user1(randMode.asUQuad);
                } അല്ലെങ്കിൽ {
                    // Called on 'this' or a non-rand class instance
                    randModeTarget.classp->foreachMember([&](AstClass*, AstVar* varp) {
                        എങ്കിൽ (!varp->isRand()) തിരികെ;
                        RandomizeMode randMode = {};
                        randMode.usesMode = സത്യം;
                        varp->user1(randMode.asUQuad);
                    });
                }
            }
            എങ്കിൽ (!valid) {
                എങ്കിൽ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) {
                    nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                } അല്ലെങ്കിൽ {
                    m_stmtp->unlinkFrBack();
                }
            }
            തിരികെ;
        }

        എങ്കിൽ (nodep->name() == "constraint_mode") {
            തർക്കം valid = സത്യം;
            എങ്കിൽ (nodep->argsp() && !VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) {
                nodep->v3error(
                    "'constraint_mode()' with arguments cannot be called as a function");
                valid = അസത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->argsp() && VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)
                       && !nodep->backp()->fileline()->warnIsOff(V3ErrorCode::IGNOREDRETURN)) {
                nodep->v3warn(
                    IGNOREDRETURN,
                    "Ignoring return value of non-void function (IEEE 1800-2023 13.4.1)");
                valid = അസത്യം;
            }
            AstConstraint* constrp = nullptr;
            AstClass* classp = m_classp;
            എങ്കിൽ (സ്ഥിരം AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(nodep, MethodCall)) {
                എങ്കിൽ (സ്ഥിരം AstConstraintRef* സ്ഥിരം constrRefp
                    = VN_CAST(methodCallp->fromp(), ConstraintRef)) {
                    constrp = constrRefp->constrp();
                    എങ്കിൽ (constrRefp->fromp()) classp = VN_AS(constrRefp->classOrPackagep(), Class);
                } അല്ലെങ്കിൽ എങ്കിൽ (AstClassRefDType* classRefDtp
                           = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                    classp = classRefDtp->classp();
                } അല്ലെങ്കിൽ {
                    nodep->v3error("Cannot call 'constraint_mode()' on a non-class variable");
                    valid = അസത്യം;
                }
            }
            എങ്കിൽ (!nodep->argsp() && !constrp) {
                nodep->v3error("Cannot call 'constraint_mode()' as a function on a variable");
                valid = അസത്യം;
            }
            എങ്കിൽ (valid) {
                RandomizeMode constraintMode = {};
                constraintMode.usesMode = സത്യം;
                എങ്കിൽ (constrp) {
                    constrp->user1(constraintMode.asUQuad);
                } അല്ലെങ്കിൽ {
                    classp->foreachMember([=](AstClass*, AstConstraint* constrp) {
                        constrp->user1(constraintMode.asUQuad);
                    });
                }
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (!nodep->argsp() && !VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) {
                    nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), 0});
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                } അല്ലെങ്കിൽ {
                    m_stmtp->unlinkFrBack();
                }
            }
            തിരികെ;
        }

        എങ്കിൽ (nodep->name() != "randomize") {
            // Propagate user1 from children (same pattern as visit(AstNodeExpr*))
            എങ്കിൽ (m_constraint_വ്യഞ്ജകം_Genp || m_inStdWith) {
                // Only the arguments decide whether a call depends on rand variables. The object
                // it's called on doesn't count, so a.f() isn't marked just because 'a' is rand
                വേണ്ടി (Ast_കണ്ണി* argp = nodep->argsp(); argp; argp = argp->nextp()) {
                    എങ്കിൽ (argp->user1()) {
                        nodep->user1(സത്യം);
                        നിർത്തുക;
                    }
                }
            }
            തിരികെ;
        }
        AstClass* classp = m_classp;
        എങ്കിൽ (സ്ഥിരം AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(nodep, MethodCall)) {
            എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp
                = VN_CAST(methodCallp->fromp()->dtypep()->skipRefp(), ClassRefDType)) {
                classp = classRefp->classp();
            }
        }
        എങ്കിൽ (classp) {
            എങ്കിൽ (!classp->user1()) classp->user1(IS_RANDOMIZED);
            markMembers(classp);
            // Clone constraints from all IS_RANDOMIZED_GLOBAL members
            classp->foreachMember([&](AstClass* സ്ഥിരം, AstVar* സ്ഥിരം memberVarp) {
                എങ്കിൽ (!memberVarp->rand().isRandomizable()) തിരികെ;
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = memberVarp->dtypep()->skipRefp();
                സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(dtypep, ClassRefDType);
                എങ്കിൽ (!classRefp || !classRefp->classp()) തിരികെ;
                AstClass* സ്ഥിരം memberClassp = classRefp->classp();
                എങ്കിൽ (memberClassp->user1() != IS_RANDOMIZED_GLOBAL) തിരികെ;
                memberVarp->globalConstrained(സത്യം);
                // Clone constraints from this IS_RANDOMIZED_GLOBAL member class
                AstVarRef* വേര്_Var_Refp
                    = പുതിയ AstVarRef{nodep->fileline(), classp, memberVarp, VAccess::READ};
                std::vector<AstVar*> emptyPath;
                memberClassp->foreachMember([&](AstClass* സ്ഥിരം, AstConstraint* സ്ഥിരം constrp) {
                    processNestedConstraint(constrp, വേര്_Var_Refp, emptyPath, classp);
                });
                cloneNestedConstraintsRecurse(വേര്_Var_Refp, memberClassp, emptyPath, classp);
                // Delete the temporary VarRef created for constraint cloning
                VL_DO_DANGLING(വേര്_Var_Refp->delete_വൃക്ഷം(), വേര്_Var_Refp);
            });
        }
        എങ്കിൽ (nodep->classOrPackagep()->name() == "std") {
            m_stdRandCallp = nullptr;
            വേണ്ടി (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                handleRandomizeArgument(argp->exprp(), nullptr, സത്യം);
            }
            തിരികെ;
        }
        വേണ്ടി (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            // randomize(null): handle before IS_RANDOMIZED_INLINE so the
            // check-only path does not allocate unused __Vrandmode slots.
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->അക്കം().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                // SMT pin only handles scalars; nested-class constraints don't cascade.
                സ്ഥിരം തർക്കം hasUnsupportedMember
                    = classp->existsMember([](സ്ഥിരം AstClass*, സ്ഥിരം AstVar* memberVarp) {
                          എങ്കിൽ (!memberVarp->rand().isRandomizable()) തിരികെ അസത്യം;
                          സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtp = memberVarp->dtypep()->skipRefp();
                          തിരികെ VN_IS(dtp, UnpackArrayDType) || VN_IS(dtp, DynArrayDType)
                                 || VN_IS(dtp, QueueDType) || VN_IS(dtp, AssocArrayDType)
                                 || VN_IS(dtp, WildcardArrayDType) || VN_IS(dtp, ClassRefDType);
                      });
                എങ്കിൽ (hasUnsupportedMember) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: 'randomize(null)' on class with rand "
                                  "container or class member");
                }
                തുടരുക;
            }
            classp->user1(IS_RANDOMIZED_INLINE);
            AstVar* fromVarp = nullptr;  // If nodep is a method call, this is its receiver
            എങ്കിൽ (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                എങ്കിൽ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(methodCallp->fromp(), MemberSel)) {
                    fromVarp = memberSelp->varp();
                } അല്ലെങ്കിൽ {
                    AstVarRef* സ്ഥിരം varrefp = VN_AS(methodCallp->fromp(), VarRef);
                    fromVarp = varrefp->varp();
                }
            }
            handleRandomizeArgument(argp->exprp(), fromVarp, അസത്യം);
        }
        // Re-mark members after the class became inline-randomized above.
        എങ്കിൽ (classp && classp->user1() == IS_RANDOMIZED_INLINE) markMembers(classp);
    }
    ശൂന്യം visit(AstConstraintUnique* nodep) override {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_constraint_വ്യഞ്ജകം_Genp);
        m_stmtp = nodep;
        m_constraint_വ്യഞ്ജകം_Genp = nodep;
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->backp()) VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(Ast_Constraint_വ്യഞ്ജകം* nodep) override {
        VL_RESTORER(m_constraint_വ്യഞ്ജകം_Genp);
        m_constraint_വ്യഞ്ജകം_Genp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstConstraintIf* nodep) override {
        {
            VL_RESTORER(m_constraint_വ്യഞ്ജകം_Genp);
            m_constraint_വ്യഞ്ജകം_Genp = nodep;
            iterateConst(nodep->condp());
        }
        iterateAndNextConstNull(nodep->thensp());
        iterateAndNextConstNull(nodep->elsesp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (!m_constraint_വ്യഞ്ജകം_Genp) തിരികെ;

        എങ്കിൽ (nodep->varp()->lifetime().isStatic()) m_staticRefs.emplace(nodep);

        // Mark as randomizable if: rand-declared, or listed in std::randomize() args
        എങ്കിൽ (nodep->varp()->rand().isRandomizable() && !(m_inStdWith && m_stdRandCallp)) {
            nodep->user1(സത്യം);
        } അല്ലെങ്കിൽ എങ്കിൽ (isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(സത്യം);
        }
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        എങ്കിൽ (!m_constraint_വ്യഞ്ജകം_Genp) തിരികെ;
        iterateChildrenConst(nodep);
        സ്ഥിരം തർക്കം randObject = nodep->fromp()->user1() || VN_IS(nodep->fromp(), LambdaArgRef);
        സ്ഥിരം തർക്കം randMember = nodep->varp()->rand().isRandomizable();
        സ്ഥിരം തർക്കം inStdWith = m_inStdWith && m_stdRandCallp;
        എങ്കിൽ (randObject && randMember && !inStdWith) {
            nodep->user1(സത്യം);
        } അല്ലെങ്കിൽ എങ്കിൽ (inStdWith && isVarInStdRandomizeArgs(nodep->varp())) {
            nodep->user1(സത്യം);
            // Mark parent object for constraint expression visitor
            എങ്കിൽ (VN_IS(nodep->fromp(), VarRef)) nodep->fromp()->user1(സത്യം);
        }

        എങ്കിൽ (m_withp) {
            Ast_കണ്ണി* backp = m_withp;
            ആകുമ്പോൾ (backp->backp()) {
                എങ്കിൽ (സ്ഥിരം AstMethodCall* സ്ഥിരം callp = VN_CAST(backp, MethodCall)) {
                    AstClassRefDType* classdtype
                        = VN_AS(callp->fromp()->dtypep()->skipRefp(), ClassRefDType);
                    nodep->user2p(classdtype->classp());
                    നിർത്തുക;
                }
                backp = backp->backp();
            }
        } അല്ലെങ്കിൽ {
            nodep->user2p(m_modp);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        nodep->user2p(m_modp);
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {
        nodep->user2p(m_modp);
        എങ്കിൽ (nodep->rand().isRandomizable()) {
            checkRandTypeEligibility(nodep, nodep->dtypep(), nodep->rand().isRandC());
        }
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstWith* nodep) override {
        VL_RESTORER(m_withp);
        m_withp = nodep;
        എങ്കിൽ (m_stdRandCallp && nodep == m_stdRandCallp->withp()) m_inStdWith = സത്യം;
        iterateChildrenConst(nodep);
        m_inStdWith = അസത്യം;
    }

    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        iterateChildrenConst(nodep);
        എങ്കിൽ (!m_constraint_വ്യഞ്ജകം_Genp && !m_inStdWith) തിരികെ;
        nodep->user1((nodep->op1p() && nodep->op1p()->user1())
                     || (nodep->op2p() && nodep->op2p()->user1())
                     || (nodep->op3p() && nodep->op3p()->user1())
                     || (nodep->op4p() && nodep->op4p()->user1()));
    }
    ശൂന്യം visit(AstArg* nodep) override {
        iterateChildrenConst(nodep);
        എങ്കിൽ (!m_constraint_വ്യഞ്ജകം_Genp && !m_inStdWith) തിരികെ;
        nodep->user1(nodep->exprp() && nodep->exprp()->user1());
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit RandomizeMarkVisitor(Ast_കണ്ണി* nodep) {
        iterateConst(nodep);
        markAllDerived();
        setPackageRefs();
    }
    ~RandomizeMarkVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Visitor that turns constraints into template strings for solvers

വർഗ്ഗം Constraint_വ്യഞ്ജകം_Visitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstVar::user3() -> bool. Handled in constraints
    // AstNodeExpr::user1()    -> bool. Depending on a randomized variable
    // AstMemberSel::user2p()  -> AstNodeModule*. Pointer to containing module
    // VNuser3InUse m_inuser3; (Allocated for use in RandomizeVisitor)

    AstClass* സ്ഥിരം m_classp;
    Ast_കണ്ണി_FTask* സ്ഥിരം m_inlineInitTaskp;  // Method to add write_var calls to
                                            // (may be null, then new() is used)
    Ast_കണ്ണി_FTask* സ്ഥിരം m_memberselInitTaskp;  // Fallback for membersel write_var when
                                               // classp has no randomize() (inherited members)
    AstVar* സ്ഥിരം m_genp;  // VlRandomizer variable of the class
    AstVar* m_randModeVarp;  // Relevant randmode state variable
    തർക്കം m_wantSingle = അസത്യം;  // Whether to merge constraint expressions with LOGAND
    VMemberMap& m_memberMap;  // Member names cached for fast lookup
    തർക്കം m_structSel = അസത്യം;  // Marks when inside structSel
                               // (used to format "%s.%s" for struct arrays and nested structs)
    തർക്കം m_deferRandModeHoist = അസത്യം;  // Wait for the complete struct member path
    std::set<std::string>& m_writtenVars;  // Track which variable paths have write_var generated
                                           // (shared across all constraints)
    std::set<std::string> m_inlineWrittenVars;  // Per-instance tracking for inline constraints
    VInsertionSet<AstVar*>* m_sizeConstrainedArraysp = nullptr;  // Arrays with size constraints
    Ast_കണ്ണി_വ്യഞ്ജകം* m_conditionp = nullptr;  // Condition under which current expression is defined
                                          // (nullptr == always defined)
    Ast_കണ്ണി_FTask* m_prepareConstrainedArraysp = nullptr;  // Grow arrays for indexed struct access
    uint32_t* m_uniqueConstraintId = nullptr;  // Current ID of unique call
    V3UniqueNames& m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    std::vector<AstVar*> m_rangeConstrainedEnums;  // Enums that are already range-constrained
    Ast_കണ്ണി* m_firstExpressionInsideIndexp = nullptr;
    AstConstraintForeach* m_foreachp = nullptr;  // Innermost constraint-foreach being processed
    std::set<സ്ഥിരം AstVar*> m_foreachIdxVars;  // Index vars of all enclosing constraint-foreaches

    വർഗ്ഗം NestedAccessPath final {
        AstMemberSel* m_മുകൾ_Nested_Array_Member_Selp
            = nullptr;  // Top node of nested array/member access
        AstSFormatF* m_nestedNameFormatTopp = nullptr;  // Top node of variable's name format
        AstSFormatF* m_nestedNameFormatp = nullptr;  // Last node of variable's name format
        Ast_കണ്ണി** m_firstExpressionInsideIndexPointerp
            = nullptr;  // First expression with no name, used for "Multiple expressions inside
                        // indices of complex expression in constraint" error

        std::string m_smtName;  // string name for shouldWriteVar checking

    പൊതു:
        ശൂന്യം addVarNamePart(AstSFormatF* സ്ഥിരം nodep, std::string&& name) {
            എങ്കിൽ (!m_nestedNameFormatp) {
                m_nestedNameFormatTopp = nodep;
                m_nestedNameFormatp = m_nestedNameFormatTopp;
            } അല്ലെങ്കിൽ {
                m_nestedNameFormatp->prependExprsp(nodep);
            }
            m_nestedNameFormatp = nodep;

            m_smtName.തിരുകുക(0, "." + name);
            // Some expressions have empty name which can cause some variables to have the same
            // name and only first one will be written
            എങ്കിൽ (name == "") {
                എങ്കിൽ (*m_firstExpressionInsideIndexPointerp) {
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

        AstMemberSel* access_വൃക്ഷം() { തിരികെ m_മുകൾ_Nested_Array_Member_Selp; }
        സ്ഥിരം std::string& smtName() { തിരികെ m_smtName; }

        Ast_Stmt_വ്യഞ്ജകം* makeWriteVarStmt(AstVar* varp, AstVar* genp) {
            AstCMethodHard* സ്ഥിരം methodp = പുതിയ AstCMethodHard{
                varp->fileline(),
                പുതിയ AstVarRef{varp->fileline(), VN_AS(genp->user2p(), കണ്ണി_Module), genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_WRITE_VAR};
            methodp->dtypeSetVoid();

            methodp->addPinsp(m_മുകൾ_Nested_Array_Member_Selp);  // Variable

            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep();  // Width
            സ്ഥിരം size_t width = m_മുകൾ_Nested_Array_Member_Selp->varp()->dtypep()->width();
            methodp->addPinsp(പുതിയ AstConst{dtypep->fileline(), AstConst::Unsized64{}, width});

            methodp->addPinsp(m_nestedNameFormatTopp);  // Solver name
            m_nestedNameFormatp = nullptr;

            methodp->addPinsp(
                പുതിയ AstConst{dtypep->fileline(), AstConst::Unsized64{}, 0});  // Dimension

            m_മുകൾ_Nested_Array_Member_Selp = nullptr;
            തിരികെ methodp->makeStmt();
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* cloneName() { തിരികെ m_nestedNameFormatTopp->clone_വൃക്ഷം(അസത്യം); }

        ശൂന്യം error() {
            VL_DO_DANGLING(m_മുകൾ_Nested_Array_Member_Selp->delete_വൃക്ഷം(), m_മുകൾ_Nested_Array_Member_Selp);
        }

        NestedAccessPath(AstMemberSel* മുകൾ_Member_Selp, Ast_കണ്ണി** pointerToFirstExprp)
            : m_മുകൾ_Nested_Array_Member_Selp{മുകൾ_Member_Selp}
            , m_firstExpressionInsideIndexPointerp{pointerToFirstExprp} {}
        ~NestedAccessPath() {
            എങ്കിൽ (m_nestedNameFormatp) {
                VL_DO_CLEAR(m_nestedNameFormatTopp->delete_വൃക്ഷം(),
                            m_nestedNameFormatTopp = nullptr);
                m_nestedNameFormatp = nullptr;
            }
        }
    };
    NestedAccessPath* m_nestedAccess = nullptr;  // Indicates state of nested access

    // Emit enum range hard constraint for a single variable
    Ast_കണ്ണി_വ്യഞ്ജകം* createEnumConstraint(File_വരി* സ്ഥിരം fl, സ്ഥിരം std::string& smtName,
                                      AstEnumDType* സ്ഥിരം enumDtp, AstVar* സ്ഥിരം genVarp) {
        Ast_കണ്ണി_Module* സ്ഥിരം genModp = VN_AS(genVarp->user2p(), കണ്ണി_Module);
        സ്ഥിരം പൂർണ്ണസംഖ്യ width = enumDtp->width();
        std::string constraint = "(__Vbv (or";
        വേണ്ടി (AstEnumItem* itemp = enumDtp->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            സ്ഥിരം AstConst* സ്ഥിരം vconstp = VN_AS(itemp->valuep(), Const);
            constraint += " (= " + smtName + " (_ bv" + cvtToStr(vconstp->toUInt()) + " "
                          + cvtToStr(width) + "))";
        }
        constraint += "))";
        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
            fl, പുതിയ AstVarRef{fl, genModp, genVarp, VAccess::READWRITE}, VCMethod::RANDOMIZER_HARD,
            പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "\"" + constraint + "\""}};
        callp->dtypeSetVoid();
        തിരികെ callp;
    };

    // Routes nested sub-objects with static rand vars when the outer class has none.
    AstVar* findStaticRandModeVarMember(AstClass* classp) സ്ഥിരം {
        ആകുമ്പോൾ (സത്യം) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp
                = VN_CAST(m_memberMap.findMember(classp, "__Vstaticrandmode"), Var)) {
                തിരികെ varp;
            }
            AstClassExtends* സ്ഥിരം extendsp = classp->extendsp();
            എങ്കിൽ (!extendsp) തിരികെ nullptr;
            classp = extendsp->classp();
        }
    }

    // Mark the one directly-referenced member; it must reach the solver even when it
    // carries no rand qualifier (the constraint refers to it).
    ശൂന്യം markConstrainedRandMember(AstStructDType* സ്ഥിരം structp, സ്ഥിരം std::string& name) {
        AstMemberDType* സ്ഥിരം memberp
            = VN_CAST(m_memberMap.findMember(structp, name), MemberDType);
        UASSERT_OBJ(memberp, structp, "Constraint references unknown struct member " << name);
        memberp->markConstrainedRand(സത്യം);
    }

    // True when the aggregate is reached through rand-qualified struct members only
    തർക്കം isRandQualifiedPath(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        ആകുമ്പോൾ (nodep) {
            എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം selp = VN_CAST(nodep, StructSel)) {
                എങ്കിൽ (AstStructDType* സ്ഥിരം structp
                    = VN_CAST(selp->fromp()->dtypep()->skipRefp(), StructDType)) {
                    സ്ഥിരം AstMemberDType* സ്ഥിരം memberp
                        = VN_CAST(m_memberMap.findMember(structp, selp->name()), MemberDType);
                    എങ്കിൽ (memberp && !memberp->rand().isRandomizable()) തിരികെ അസത്യം;
                }
                nodep = selp->fromp();
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം selp = VN_CAST(nodep, ArraySel)) {
                nodep = selp->fromp();
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCMethodHard* സ്ഥിരം methodp = VN_CAST(nodep, CMethodHard)) {
                nodep = methodp->fromp();
            } അല്ലെങ്കിൽ {
                നിർത്തുക;
            }
        }
        തിരികെ സത്യം;
    }

    // IEEE 1800-2023 18.4: all rand members of a rand unpacked struct are solved
    // concurrently, so unreferenced rand members must still reach the solver.
    സ്ഥിതം ശൂന്യം markStructConstrainedRandRecurse(Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        AstStructDType* സ്ഥിരം structp = VN_CAST(arrayElementDTypep(dtypep), StructDType);
        എങ്കിൽ (!structp || structp->packed()) തിരികെ;
        structp->markConstrainedRand(സത്യം);
        വേണ്ടി (AstMemberDType* memberp = structp->membersp(); memberp;
             memberp = VN_AS(memberp->nextp(), MemberDType)) {
            // Non-rand members keep their value. TODO: randc members are solved as
            // plain rand (uniform); per-member cyclic state is not generated for structs.
            എങ്കിൽ (!memberp->rand().isRandomizable()) തുടരുക;
            memberp->markConstrainedRand(സത്യം);
            markStructConstrainedRandRecurse(memberp->subDTypep());
        }
    }

    // Build full path for a MemberSel chain (e.g., "obj.l2.l3.l4")
    std::string buildMemberPath(സ്ഥിരം AstMemberSel* സ്ഥിരം memberSelp) {
        സ്ഥിരം Ast_കണ്ണി* fromp = memberSelp->fromp();
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(fromp, VarRef)) {
            // Base case: reached root VarRef
            തിരികെ refp->name() + "." + memberSelp->name();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം selp = VN_CAST(fromp, MemberSel)) {
            // Recursive case: build path from outer levels
            തിരികെ buildMemberPath(selp) + "." + memberSelp->name();
        }
        memberSelp->v3fatalSrc("Unexpected node type in MemberSel chain");
        തിരികെ "";
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* getFromp(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep) {
        എങ്കിൽ (സ്ഥിരം AstCMethodHard* സ്ഥിരം cmethp = VN_CAST(nodep, CMethodHard)) {
            തിരികെ cmethp->fromp();
        }
        തിരികെ getSelFromp(nodep);
    }

    സ്ഥിതം std::string extractSolveBeforeBaseName(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        ആകുമ്പോൾ (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = getFromp(exprp)) exprp = fromp;
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം vrefp = VN_CAST(exprp, VarRef)) തിരികെ vrefp->name();
        തിരികെ "";
    }

    സ്ഥിതം ശൂന്യം buildNamePrefix(AstCExpr* exprp, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep) {
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
            exprp->add("(\""s + refp->name());
            തിരികെ;
        }

        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = getFromp(nodep);
        എങ്കിൽ (fromp) buildNamePrefix(exprp, fromp);

        എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം selp = VN_CAST(nodep, StructSel)) {
            exprp->add("." + selp->name());
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, MemberSel)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Nested array element access in global constraint");
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstCMethodHard* സ്ഥിരം cmethp = VN_CAST(nodep, CMethodHard)) {
            എങ്കിൽ (cmethp->method() == VCMethod::ARRAY_AT) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = cmethp->pinsp();
                exprp->add(".\" + ");
                AstCFuncHard* സ്ഥിരം solverHexFuncp
                    = പുതിയ AstCFuncHard{nodep->fileline(), VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(argp->clone_വൃക്ഷം_Pure(അസത്യം));
                solverHexFuncp->dtypeSetString();
                exprp->add(solverHexFuncp);
                exprp->add(" + \"");
            } അല്ലെങ്കിൽ {
                cmethp->v3fatalSrc("Unexpected CMethodHard: " << cmethp->prettyTypeName());
                തിരികെ;
            }
        }
    }

    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, struct selects, and array element selects (for foreach).
    സ്ഥിതം AstCExpr* build_Path_വ്യഞ്ജകം(File_വരി* സ്ഥിരം fl, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep,
                                   സ്ഥിരം std::string selName) {
        AstCExpr* സ്ഥിരം exprp = പുതിയ AstCExpr{fl, ""};
        buildNamePrefix(exprp, nodep);
        എങ്കിൽ (selName != "") {
            exprp->add(".");
            exprp->add(selName);
        }
        exprp->add("\")");
        തിരികെ exprp;
    }
    // Build a C++ expression (as AstNodeExpr) that evaluates to a const char*
    // containing the SMT variable name for a solve-before variable reference.
    // Handles simple vars, member selects, and array element selects (for foreach).
    // Returns nullptr for unsupported expression types.
    // Helper: build a dynamic AstCExpr for "baseName[idx]" pattern
    AstCExpr* build_Array_Sel_Name_വ്യഞ്ജകം(File_വരി* fl, സ്ഥിരം std::string& baseName,
                                    സ്ഥിരം Ast_കണ്ണി_Sel* selp) {
        AstCExpr* സ്ഥിരം p = പുതിയ AstCExpr{fl, ""};
        p->add("(\""s + baseName + ".\"");
        p->add("+");
        AstCFuncHard* സ്ഥിരം solverHexFuncp
            = പുതിയ AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
        solverHexFuncp->addPinsp(selp->bitp()->clone_വൃക്ഷം_Pure(അസത്യം));
        solverHexFuncp->dtypeSetString();
        p->add(solverHexFuncp);
        p->add(")");
        p->dtypeSetString();
        തിരികെ p;
    }

    // Helper: get fromp from MemberSel or StructSel
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* getSelFromp(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം mp = VN_CAST(exprp, MemberSel)) തിരികെ mp->fromp();
        എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം sp = VN_CAST(exprp, StructSel)) തിരികെ sp->fromp();
        തിരികെ nullptr;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* build_Solve_Before_Name_വ്യഞ്ജകം(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(exprp, VarRef)) {
            AstCExpr* സ്ഥിരം p = പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "\"" + varrefp->name() + "\"s"};
            p->dtypeSetString();
            തിരികെ p;
        }
        // Handle MemberSel or StructSel (V3Width converts MemberSel -> StructSel for structs)
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം selFromp = getSelFromp(exprp)) {
            സ്ഥിരം std::string selName = exprp->name();
            // Check if fromp chain contains ArraySel (e.g., cfg[i].w)
            എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം arrSelp = VN_CAST(selFromp, ArraySel)) {
                std::string baseName;
                എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം vp = VN_CAST(arrSelp->fromp(), VarRef)) {
                    baseName = vp->name();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം mp = VN_CAST(arrSelp->fromp(), MemberSel)) {
                    baseName = buildMemberPath(mp);
                }
                എങ്കിൽ (baseName.empty()) തിരികെ nullptr;
                AstCExpr* സ്ഥിരം p = പുതിയ AstCExpr{fl, ""};
                p->add("(\""s + baseName + ".\"");
                p->add("+");
                AstCFuncHard* സ്ഥിരം solverHexFuncp
                    = പുതിയ AstCFuncHard{fl, VCFunction::RANDOMIZER_TO_SOLVER_HEX};
                solverHexFuncp->addPinsp(arrSelp->bitp()->clone_വൃക്ഷം_Pure(അസത്യം));
                solverHexFuncp->dtypeSetString();
                p->add(solverHexFuncp);
                p->add(")");
                p->dtypeSetString();
                തിരികെ p;
            }
            // Static member path (obj.field)
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം vp = VN_CAST(selFromp, VarRef)) {
                സ്ഥിരം std::string path = vp->name() + "." + selName;
                AstCExpr* സ്ഥിരം p = പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "\"" + path + "\"s"};
                p->dtypeSetString();
                തിരികെ p;
            }
            എങ്കിൽ (VN_IS(selFromp, MemberSel) || VN_IS(selFromp, StructSel)
                || VN_IS(selFromp, CMethodHard)) {
                AstCExpr* സ്ഥിരം p = build_Path_വ്യഞ്ജകം(fl, selFromp, selName);
                p->dtypeSetString();
                തിരികെ p;
            }
            തിരികെ nullptr;
        }
        എങ്കിൽ (VN_IS(exprp, CMethodHard)) {
            AstCExpr* സ്ഥിരം p = build_Path_വ്യഞ്ജകം(fl, exprp, "");
            p->dtypeSetString();
            തിരികെ p;
        }
        സ്ഥിരം Ast_കണ്ണി_Sel* selp = VN_CAST(exprp, ArraySel);
        എങ്കിൽ (!selp) selp = VN_CAST(exprp, AssocSel);
        എങ്കിൽ (selp) {
            // arr[i] -> dynamic name
            std::string baseName;
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം vp = VN_CAST(selp->fromp(), VarRef)) {
                baseName = vp->name();
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം mp = VN_CAST(selp->fromp(), MemberSel)) {
                baseName = buildMemberPath(mp);
            }
            എങ്കിൽ (baseName.empty()) തിരികെ nullptr;
            തിരികെ build_Array_Sel_Name_വ്യഞ്ജകം(fl, baseName, selp);
        }
        // Packed struct member: SEL(ARRAYSEL(...)) -- bit select on array element.
        // Solver registers the whole element, so promote to array element level.
        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം bitSelp = VN_CAST(exprp, Sel)) {
            തിരികെ build_Solve_Before_Name_വ്യഞ്ജകം(fl, bitSelp->fromp());
        }
        തിരികെ nullptr;
    }

    AstSFormatF* getConstFormat(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        തിരികെ പുതിയ AstSFormatF{nodep->fileline(), (nodep->width() & 3) ? "#b%b" : "#x%x", അസത്യം,
                               nodep};
    }
    തർക്കം editFormat(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (nodep->user1()) തിരികെ അസത്യം;
        // Replace computable expression with SMT constant
        VNRelinker handle;
        nodep->unlinkFrBack(&handle);
        handle.relink(getConstFormat(nodep));
        തിരികെ സത്യം;
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* wrapWithCond(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp,
                              സ്ഥിരം പൂർണ്ണസംഖ്യ width) {
        എങ്കിൽ (condp) {
            File_വരി* സ്ഥിരം flp = exprp->fileline();
            തിരികെ പുതിയ AstCond{
                flp, condp, exprp,
                getConstFormat(പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, width, 0})};
        }
        തിരികെ exprp;
    }
    AstSFormatF* editSMT(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nullptr,
                         Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = nullptr, Ast_കണ്ണി_വ്യഞ്ജകം* thsp = nullptr) {
        // Replace incomputable (result-dependent) expression with SMT expression
        std::string smt_വ്യഞ്ജകം = nodep->emitSMT();  // Might need child width (AstExtend)
        എങ്കിൽ (smt_വ്യഞ്ജകം == "") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported expression inside constraint");
            തിരികെ nullptr;
        }

        സ്ഥിരം പൂർണ്ണസംഖ്യ lhsWidth = lhsp ? lhsp->width() : 0;
        സ്ഥിരം പൂർണ്ണസംഖ്യ rhsWidth = rhsp ? rhsp->width() : 0;
        സ്ഥിരം പൂർണ്ണസംഖ്യ thsWidth = thsp ? thsp->width() : 0;
        Ast_കണ്ണി_വ്യഞ്ജകം* lhsCondp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* rhsCondp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* thsCondp = nullptr;

        എങ്കിൽ (lhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            lhsp = VN_AS(iterateSubtreeReturnEdits(lhsp->backp() ? lhsp->unlinkFrBack() : lhsp),
                         കണ്ണി_വ്യഞ്ജകം);
            lhsCondp = m_conditionp;
        }
        എങ്കിൽ (rhsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            rhsp = VN_AS(iterateSubtreeReturnEdits(rhsp->backp() ? rhsp->unlinkFrBack() : rhsp),
                         കണ്ണി_വ്യഞ്ജകം);
            rhsCondp = m_conditionp;
        }
        എങ്കിൽ (thsp) {
            VL_RESTORER(m_conditionp);
            m_conditionp = nullptr;
            thsp = VN_AS(iterateSubtreeReturnEdits(thsp->backp() ? thsp->unlinkFrBack() : thsp),
                         കണ്ണി_വ്യഞ്ജകം);
            thsCondp = m_conditionp;
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* argsp = nullptr;
        വേണ്ടി (string::iterator pos = smt_വ്യഞ്ജകം.begin(); pos != smt_വ്യഞ്ജകം.end(); ++pos) {
            എങ്കിൽ (pos[0] == '%') {
                ++pos;
                തിരഞ്ഞെടുപ്പ് (pos[0]) {
                അവസ്ഥ '%': നിർത്തുക;
                അവസ്ഥ 'l':
                    pos[0] = 's';
                    UASSERT_OBJ(lhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_കണ്ണി::addNext(argsp, wrapWithCond(lhsp, lhsCondp, lhsWidth));
                    lhsp = nullptr;
                    നിർത്തുക;
                അവസ്ഥ 'r':
                    pos[0] = 's';
                    UASSERT_OBJ(rhsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_കണ്ണി::addNext(argsp, wrapWithCond(rhsp, rhsCondp, rhsWidth));
                    rhsp = nullptr;
                    നിർത്തുക;
                അവസ്ഥ 't':
                    pos[0] = 's';
                    UASSERT_OBJ(thsp, nodep, "emitSMT() references undef node");
                    argsp = Ast_കണ്ണി::addNext(argsp, wrapWithCond(thsp, thsCondp, thsWidth));
                    thsp = nullptr;
                    നിർത്തുക;
                സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("Unknown emitSMT format code: %" << pos[0]); നിർത്തുക;
                }
            }
        }
        UASSERT_OBJ(!lhsp, nodep, "Missing emitSMT %l for " << lhsp);
        UASSERT_OBJ(!rhsp, nodep, "Missing emitSMT %r for " << rhsp);
        UASSERT_OBJ(!thsp, nodep, "Missing emitSMT %t for " << thsp);
        AstSFormatF* സ്ഥിരം newp = പുതിയ AstSFormatF{nodep->fileline(), smt_വ്യഞ്ജകം, അസത്യം, argsp};

        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ newp;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* editSingle(File_വരി* fl, Ast_കണ്ണി* itemsp) {
        എങ്കിൽ (!itemsp) തിരികെ nullptr;

        VL_RESTORER(m_wantSingle);
        m_wantSingle = സത്യം;

        {
            AstBegin* സ്ഥിരം tempp
                = പുതിയ AstBegin{fl, "[EditWrapper]", itemsp->unlinkFrBackWithNext(), അസത്യം};
            VL_DO_DANGLING(iterateAndNextNull(tempp->stmtsp()), itemsp);
            itemsp = tempp->stmtsp();
            എങ്കിൽ (itemsp) itemsp->unlinkFrBackWithNext();
            VL_DO_DANGLING(tempp->delete_വൃക്ഷം(), tempp);
        }
        എങ്കിൽ (!itemsp) തിരികെ nullptr;

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprsp = VN_CAST(itemsp, കണ്ണി_വ്യഞ്ജകം);
        UASSERT_OBJ(exprsp, itemsp, "Single not expression?");

        എങ്കിൽ (!exprsp->nextp()) തിരികെ exprsp;

        std::ostringstream fmt;
        fmt << "(bvand";
        വേണ്ടി (Ast_കണ്ണി* itemp = exprsp; itemp; itemp = itemp->nextp()) fmt << " %s";
        fmt << ')';
        തിരികെ പുതിയ AstSFormatF{fl, fmt.str(), അസത്യം, exprsp};
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newSel(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* arrayp, Ast_കണ്ണി_വ്യഞ്ജകം* idxp) {
        // similar to V3WidthSel.cpp
        // skipRefp() unwraps typedef wrappers (AstRefDType) so the dtype kind
        // checks below recognize types spelled as `typedef <scalar> <name>[$]`.
        Ast_കണ്ണി_DType* സ്ഥിരം arrDtp = arrayp->unlinkFrBack()->dtypep()->skipRefp();
        Ast_കണ്ണി_വ്യഞ്ജകം* selp = nullptr;
        എങ്കിൽ (VN_IS(arrDtp, QueueDType) || VN_IS(arrDtp, DynArrayDType))
            selp = പുതിയ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, idxp};
        അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(arrDtp, UnpackArrayDType))
            selp = പുതിയ AstArraySel{fl, arrayp, idxp};
        അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(arrDtp, AssocArrayDType))
            selp = പുതിയ AstAssocSel{fl, arrayp, idxp};
        UASSERT_OBJ(selp, arrayp, "Selecting from non-array?");
        selp->dtypep(arrDtp->subDTypep());
        തിരികെ selp;
    }

    // Convert expression to target width using extend or truncate as needed
    Ast_കണ്ണി_വ്യഞ്ജകം* adjustWidth(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, പൂർണ്ണസംഖ്യ targetWidth,
                             VSigning targetSigning) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ വ്യഞ്ജകം_Width = exprp->width();
        എങ്കിൽ (targetWidth > വ്യഞ്ജകം_Width) {
            // Extend to match target width
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം result = പുതിയ AstExtend{fl, exprp, targetWidth};
            result->dtypeChgSigned(targetSigning);
            തിരികെ result;
        } അല്ലെങ്കിൽ എങ്കിൽ (targetWidth < വ്യഞ്ജകം_Width) {
            // Truncate to match target width
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം result = പുതിയ AstSel{fl, exprp, 0, targetWidth};
            result->dtypeChgSigned(targetSigning);
            തിരികെ result;
        } അല്ലെങ്കിൽ {
            // Width already matches
            തിരികെ exprp;
        }
    }

    // Extract return expression from a simple function body, or nullptr if too complex.
    // Uses foreach to walk all assigns regardless of body organization (JumpBlock nesting etc.).
    // Takes the last assignment to the return variable -- the first is the initializer.
    Ast_കണ്ണി_വ്യഞ്ജകം* extract_Return_വ്യഞ്ജകം(AstFunc* funcp) {
        AstVar* സ്ഥിരം retVarp = VN_CAST(funcp->fvarp(), Var);
        എങ്കിൽ (!retVarp) തിരികെ nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* retExprp = nullptr;
        പൂർണ്ണസംഖ്യ assign_എണ്ണം = 0;
        funcp->foreach([&](AstAssign* assignp) {
            AstVarRef* സ്ഥിരം lhsp = VN_CAST(assignp->lhsp(), VarRef);
            എങ്കിൽ (!lhsp || lhsp->varp() != retVarp) തിരികെ;
            retExprp = assignp->rhsp();
            ++assign_എണ്ണം;
        });
        // Simple function: initializer + return (2) or just return (1)
        എങ്കിൽ (assign_എണ്ണം > 2 || !retExprp) തിരികെ nullptr;
        തിരികെ retExprp;
    }

    // Bottom-up user1 propagation on inlined expression tree
    ശൂന്യം propagateUser1InlineRecurse(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (VN_IS(nodep, കണ്ണി_Var_Ref)) {
            nodep->user1(VN_AS(nodep, കണ്ണി_Var_Ref)->varp()->rand().isRandomizable());
            തിരികെ;
        }
        എങ്കിൽ (VN_IS(nodep, Const)) {
            nodep->user1(അസത്യം);
            തിരികെ;
        }
        പൂർണ്ണസംഖ്യ anyChild = അസത്യം;  // Used as bool
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp = VN_CAST(nodep->op1p(), കണ്ണി_വ്യഞ്ജകം)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp = VN_CAST(nodep->op2p(), കണ്ണി_വ്യഞ്ജകം)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp = VN_CAST(nodep->op3p(), കണ്ണി_വ്യഞ്ജകം)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp = VN_CAST(nodep->op4p(), കണ്ണി_വ്യഞ്ജകം)) {
            propagateUser1InlineRecurse(cp);
            anyChild |= cp->user1();
        }
        nodep->user1(anyChild);
    }

    // Prepend the bucket preamble stmts stored on user3p by lowerDistConstraints to the
    // statements that will become the body of the generated AstForeach, so the per-element
    // bucket draw is declared and evaluated in the same scope as its references.
    സ്ഥിതം Ast_കണ്ണി* prependDistPreamble(AstConstraintForeach* nodep, Ast_കണ്ണി* bodyp) {
        Ast_കണ്ണി* സ്ഥിരം preamblep = nodep->user3p();
        എങ്കിൽ (!preamblep) തിരികെ bodyp;
        nodep->user3p(nullptr);
        preamblep->addNext(bodyp);
        തിരികെ preamblep;
    }

    // True if the statement references an index variable of an enclosing
    // constraint-foreach. Such a statement must stay inside the loop body; hoisting
    // it out leaves a dangling reference once task inlining deletes the index.
    തർക്കം referencesForeachIdx(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം {
        എങ്കിൽ (m_foreachIdxVars.empty()) തിരികെ അസത്യം;
        തിരികെ nodep->exists(
            [&](സ്ഥിരം AstVarRef* refp) { തിരികെ m_foreachIdxVars.എണ്ണം(refp->varp()) != 0; });
    }

    // Place a solver-registration statement: into the enclosing foreach body when it
    // references an enclosing foreach index (so it is not hoisted out of the index's
    // scope), otherwise into the init task.
    ശൂന്യം addRegistrationStmt(Ast_കണ്ണി* സ്ഥിരം stmtp, Ast_കണ്ണി_FTask* സ്ഥിരം initTaskp) {
        എങ്കിൽ (m_foreachp && referencesForeachIdx(stmtp)) {
            // Appending to the body mid-iteration is safe: visit(AstStmtExpr) is a
            // no-op so the appended node is not re-processed, and prependDistPreamble()
            // correctly keeps it as a loop-body statement.
            m_foreachp->addBodyp(stmtp);
        } അല്ലെങ്കിൽ {
            initTaskp->addStmtsp(stmtp);
        }
    }

    // Create SFormatF for array dereference inside solver
    AstSFormatF* createSolverArrDerefp(File_വരി* സ്ഥിരം fl, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം arrExprp,
                                       Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം idxExprp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argsp = Ast_കണ്ണി::addNext(arrExprp, idxExprp);
        AstSFormatF* സ്ഥിരം formatp = പുതിയ AstSFormatF{fl, "(select %s %s)", അസത്യം, argsp};
        തിരികെ formatp;
    }

    ശൂന്യം setRandMode(AstVar* സ്ഥിരം varp, AstMemberSel* സ്ഥിരം memberselp,
                     സ്ഥിരം std::string& smtName, സ്ഥിരം RandomizeMode& randMode,
                     Ast_കണ്ണി_FTask* സ്ഥിരം initTaskp) {
        Ast_കണ്ണി_Module* സ്ഥിരം varClassp = VN_AS(varp->user2p(), കണ്ണി_Module);
        AstVar* സ്ഥിരം subRandModeVarp = getRandModeVarFromClass(varClassp);
        എങ്കിൽ (subRandModeVarp) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം parentAccessp = memberselp->fromp()->clone_വൃക്ഷം(അസത്യം);
            AstMemberSel* സ്ഥിരം randModeSelp
                = പുതിയ AstMemberSel{varp->fileline(), parentAccessp, subRandModeVarp};
            randModeSelp->dtypep(subRandModeVarp->dtypep());
            AstCMethodHard* സ്ഥിരം atp
                = പുതിയ AstCMethodHard{varp->fileline(), randModeSelp, VCMethod::ARRAY_AT,
                                     പുതിയ AstConst{varp->fileline(), randMode.index}};
            atp->dtypeSetUInt32();
            // rand_mode ON: clear disabled state
            AstCMethodHard* സ്ഥിരം enablep = പുതിയ AstCMethodHard{
                varp->fileline(),
                പുതിയ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), കണ്ണി_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_CLEAR_VAR_DISABLED};
            enablep->dtypeSetVoid();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nameExprp
                = m_nestedAccess ? m_nestedAccess->cloneName()
                                 : പുതിയ AstSFormatF{varp->fileline(), smtName, അസത്യം, nullptr};
            nameExprp->dtypep(varp->dtypep());
            enablep->addPinsp(nameExprp);
            // rand_mode OFF: set disabled state
            AstCMethodHard* സ്ഥിരം disablep = പുതിയ AstCMethodHard{
                varp->fileline(),
                പുതിയ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), കണ്ണി_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_SET_VAR_DISABLED};
            disablep->dtypeSetVoid();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം disnp = nameExprp->clone_വൃക്ഷം(അസത്യം);
            disablep->addPinsp(disnp);
            AstIf* സ്ഥിരം ifp
                = പുതിയ AstIf{varp->fileline(), atp, enablep->makeStmt(), disablep->makeStmt()};
            addRegistrationStmt(ifp, initTaskp);
        }
    }

    ശൂന്യം markRandc(AstVar* സ്ഥിരം varp, സ്ഥിരം std::string& smtName, Ast_കണ്ണി_FTask* സ്ഥിരം initTaskp) {
        AstCMethodHard* സ്ഥിരം markp = പുതിയ AstCMethodHard{
            varp->fileline(),
            പുതിയ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), കണ്ണി_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_MARK_RANDC};
        markp->dtypeSetVoid();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nameExprp
            = m_nestedAccess ? m_nestedAccess->cloneName()
                             : പുതിയ AstSFormatF{varp->fileline(), smtName, അസത്യം, nullptr};
        nameExprp->dtypep(varp->dtypep());
        markp->addPinsp(nameExprp);
        addRegistrationStmt(markp->makeStmt(), initTaskp);
    }

    Ast_കണ്ണി_Module* getLeftmostVarModulep(AstMemberSel* സ്ഥിരം memberselp,
                                         AstVar* സ്ഥിരം varp) സ്ഥിരം {
        എങ്കിൽ (memberselp) {
            // For memberselp, find the leftmost varref to get the class
            Ast_കണ്ണി* ഇടത്_Most_Nodep = memberselp->fromp();
            ആകുമ്പോൾ (AstMemberSel* സ്ഥിരം nestedMemberSelp = VN_CAST(ഇടത്_Most_Nodep, MemberSel)) {
                ഇടത്_Most_Nodep = nestedMemberSelp->fromp();
            }
            എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം വേര്_Var_Refp = VN_CAST(ഇടത്_Most_Nodep, കണ്ണി_Var_Ref)) {
                തിരികെ VN_AS(വേര്_Var_Refp->varp()->user2p(), കണ്ണി_Module);
            }
            തിരികെ VN_AS(memberselp->user2p(), കണ്ണി_Module);
        }
        തിരികെ VN_AS(varp->user2p(), കണ്ണി_Module);
    }

    Ast_കണ്ണി_FTask* getInitTaskp(AstVar* സ്ഥിരം varp, സ്ഥിരം തർക്കം memberselOrStructSel,
                               Ast_കണ്ണി_Module* സ്ഥിരം classp) സ്ഥിരം {
        എങ്കിൽ (m_inlineInitTaskp) തിരികെ m_inlineInitTaskp;
        varp->user3(സത്യം);
        എങ്കിൽ (memberselOrStructSel) {
            എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം initTaskp
                = VN_AS(m_memberMap.findMember(classp, "randomize"), കണ്ണി_FTask)) {
                തിരികെ initTaskp;
            }
            // Inherited rand members may belong to a base class
            // that has no randomize(); use the caller's function
            എങ്കിൽ (m_memberselInitTaskp) തിരികെ m_memberselInitTaskp;
            classp->v3fatalSrc("No randomize() in class");
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_FTask* സ്ഥിരം initTaskp
                = VN_AS(m_memberMap.findMember(classp, "new"), കണ്ണി_FTask);
            UASSERT_OBJ(initTaskp, classp, "No new() in class");
            തിരികെ initTaskp;
        }
        തിരികെ nullptr;
    }

    ശൂന്യം createWriteVarForClassFields(AstClass* സ്ഥിരം elemClassp, സ്ഥിരം std::string& smtName,
                                      AstVar* സ്ഥിരം iterVarp, സ്ഥിരം തർക്കം isUnpackedClassRefArray,
                                      Ast_കണ്ണി_Module* സ്ഥിരം classOrPackagep, AstVar* സ്ഥിരം varp,
                                      AstIf* സ്ഥിരം ifNonNullp,
                                      AstClassRefDType* elemClassRefDtp) സ്ഥിരം {
        File_വരി* സ്ഥിരം fl = varp->fileline();
        Ast_കണ്ണി_Module* സ്ഥിരം varClassp = VN_AS(varp->user2p(), കണ്ണി_Module);
        // 32-bit index hex chars for SMT name formatting
        constexpr പൂർണ്ണസംഖ്യ idxWidth = 32;
        സ്ഥിരം പൂർണ്ണസംഖ്യ fmtWidth = VL_വാക്കുകൾ_I(idxWidth) * 8;

        വേണ്ടി (സ്ഥിരം AstClass* cp = elemClassp; cp;
             cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            വേണ്ടി (Ast_കണ്ണി* mnodep = cp->stmtsp(); mnodep; mnodep = mnodep->nextp()) {
                AstVar* സ്ഥിരം memberVarp = VN_CAST(mnodep, Var);
                എങ്കിൽ (!memberVarp || !memberVarp->rand().isRandomizable()) തുടരുക;
                Ast_കണ്ണി_DType* സ്ഥിരം memberDtp = memberVarp->dtypep()->skipRefp();
                എങ്കിൽ (VN_IS(memberDtp, ClassRefDType) || memberDtp->isNonPackedArray()) തുടരുക;
                സ്ഥിരം പൂർണ്ണസംഖ്യ memberWidth = memberDtp->width();

                AstCStmt* സ്ഥിരം fmtp = പുതിയ AstCStmt{fl};
                എങ്കിൽ (isUnpackedClassRefArray) {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%x."
                              + memberVarp->name() + "\", (unsigned)");
                } അല്ലെങ്കിൽ {
                    fmtp->add("VL_SNPRINTF(__Vn, sizeof(__Vn), \"" + smtName + ".%0"
                              + std::to_string(fmtWidth) + "x." + memberVarp->name()
                              + "\", (unsigned)");
                }
                fmtp->add(പുതിയ AstVarRef{fl, iterVarp, VAccess::READ});
                fmtp->add(");\n");
                ifNonNullp->addThensp(fmtp);

                AstVarRef* സ്ഥിരം arrayWrRefp = പുതിയ AstVarRef{fl, varClassp, varp, VAccess::WRITE};
                arrayWrRefp->classOrPackagep(classOrPackagep);
                Ast_കണ്ണി_വ്യഞ്ജകം* atWritep;
                എങ്കിൽ (isUnpackedClassRefArray) {
                    atWritep = പുതിയ AstArraySel{fl, arrayWrRefp,
                                               പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}};
                } അല്ലെങ്കിൽ {
                    atWritep = പുതിയ AstCMethodHard{fl, arrayWrRefp, VCMethod::ARRAY_AT_WRITE,
                                                  പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}};
                }
                atWritep->dtypep(elemClassRefDtp);
                AstMemberSel* സ്ഥിരം memberSelp = പുതിയ AstMemberSel{fl, atWritep, memberVarp};

                AstCMethodHard* സ്ഥിരം writeVarp
                    = പുതിയ AstCMethodHard{fl,
                                         പുതിയ AstVarRef{fl, VN_AS(m_genp->user2p(), കണ്ണി_Module),
                                                       m_genp, VAccess::READWRITE},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarp->dtypeSetVoid();
                writeVarp->addPinsp(memberSelp);
                writeVarp->addPinsp(
                    പുതിയ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(memberWidth)});
                AstCExpr* സ്ഥിരം nameRefp = പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "__Vn", 0};
                nameRefp->dtypep(varp->dtypep());
                writeVarp->addPinsp(nameRefp);
                writeVarp->addPinsp(പുതിയ AstConst{fl, AstConst::Unsized64{}, 0ULL});
                ifNonNullp->addThensp(writeVarp->makeStmt());
            }
        }
    }

    ശൂന്യം createSolverArrayHandle(AstVar* സ്ഥിരം varp, AstClassRefDType* elemClassRefDtp,
                                 Ast_കണ്ണി_Module* സ്ഥിരം classOrPackagep,
                                 സ്ഥിരം തർക്കം isUnpackedClassRefArray,
                                 സ്ഥിരം std::string& smtName) സ്ഥിരം {
        File_വരി* സ്ഥിരം fl = varp->fileline();
        AstClass* സ്ഥിരം elemClassp = elemClassRefDtp->classp();
        Ast_കണ്ണി_Module* സ്ഥിരം varClassp = VN_AS(varp->user2p(), കണ്ണി_Module);

        AstVar* സ്ഥിരം iterVarp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__Vi"),
                                            varp->findUInt32DType()};
        iterVarp->funcLocal(സത്യം);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);

        Ast_കണ്ണി* സ്ഥിരം stmtsp
            = പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, iterVarp, VAccess::WRITE}, പുതിയ AstConst{fl, 0}};

        Ast_കണ്ണി_വ്യഞ്ജകം* sizep;
        എങ്കിൽ (isUnpackedClassRefArray) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ arraySize
                = VN_AS(varp->dtypep()->skipRefp(), UnpackArrayDType)->elementsConst();
            sizep = പുതിയ AstConst{fl, static_cast<uint32_t>(arraySize)};
        } അല്ലെങ്കിൽ {
            AstVarRef* സ്ഥിരം arraySizeRefp = പുതിയ AstVarRef{fl, varClassp, varp, VAccess::READ};
            arraySizeRefp->classOrPackagep(classOrPackagep);
            AstCMethodHard* സ്ഥിരം dynSizep
                = പുതിയ AstCMethodHard{fl, arraySizeRefp, VCMethod::DYN_SIZE, nullptr};
            dynSizep->dtypeSetUInt32();
            sizep = dynSizep;
        }

        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(പുതിയ AstLoopTest{
            fl, loopp, പുതിയ AstLt{fl, പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});

        AstVarRef* സ്ഥിരം arrayReadRefp = പുതിയ AstVarRef{fl, varClassp, varp, VAccess::READ};
        arrayReadRefp->classOrPackagep(classOrPackagep);
        Ast_കണ്ണി_വ്യഞ്ജകം* atReadp;
        എങ്കിൽ (isUnpackedClassRefArray) {
            atReadp
                = പുതിയ AstArraySel{fl, arrayReadRefp, പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}};
        } അല്ലെങ്കിൽ {
            atReadp = പുതിയ AstCMethodHard{fl, arrayReadRefp, VCMethod::ARRAY_AT,
                                         പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}};
        }
        atReadp->dtypep(elemClassRefDtp);
        AstIf* സ്ഥിരം ifNonNullp
            = പുതിയ AstIf{fl, പുതിയ AstNeq{fl, atReadp, പുതിയ AstConst{fl, AstConst::Null{}}}, nullptr};
        loopp->addStmtsp(ifNonNullp);

        AstCStmt* സ്ഥിരം bufDeclp = പുതിയ AstCStmt{fl, "char __Vn[256];\n"};
        ifNonNullp->addThensp(bufDeclp);

        createWriteVarForClassFields(elemClassp, smtName, iterVarp, isUnpackedClassRefArray,
                                     classOrPackagep, varp, ifNonNullp, elemClassRefDtp);

        loopp->addStmtsp(പുതിയ AstAssign{
            fl, പുതിയ AstVarRef{fl, iterVarp, VAccess::WRITE},
            പുതിയ AstAdd{fl, പുതിയ AstConst{fl, 1}, പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}}});

        varp->user3(സത്യം);
        Ast_കണ്ണി_FTask* initTaskp = m_inlineInitTaskp;
        എങ്കിൽ (!initTaskp) {
            initTaskp = VN_AS(m_memberMap.findMember(varClassp, "new"), കണ്ണി_FTask);
            UASSERT_OBJ(initTaskp, varClassp, "No new() in class");
        }
        // At the beginning, so declared before referenced
        initTaskp->prependStmtsp(iterVarp);
        initTaskp->addStmtsp(stmtsp);
    }

    ശൂന്യം createSolverVarHandle(AstVar* സ്ഥിരം varp, സ്ഥിരം തർക്കം structSelOrCMeth,
                               സ്ഥിരം തർക്കം isGlobalConstrained, സ്ഥിരം RandomizeMode randMode,
                               AstMemberSel* സ്ഥിരം memberselp, സ്ഥിരം std::string& smtName,
                               Ast_കണ്ണി_Module* സ്ഥിരം classOrPackagep, Ast_കണ്ണി_Module* സ്ഥിരം classp,
                               Ast_കണ്ണി_FTask* സ്ഥിരം initTaskp) {
        uint32_t unpackedDims = 0;
        എങ്കിൽ (varp->dtypeSkipRefp()->isNonPackedArray()) {
            unpackedDims = varp->dtypep()->dimensions(അസത്യം).second;
        }
        എങ്കിൽ (VN_IS(varp->dtypeSkipRefp(), StructDType)
            && !VN_AS(varp->dtypeSkipRefp(), StructDType)->packed()) {
            markStructConstrainedRandRecurse(varp->dtypeSkipRefp());
            unpackedDims = 1;
        }
        AstCMethodHard* സ്ഥിരം methodp = പുതിയ AstCMethodHard{
            varp->fileline(),
            പുതിയ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), കണ്ണി_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();
        എങ്കിൽ (memberselp) {
            methodp->addPinsp(memberselp);
        } അല്ലെങ്കിൽ {
            AstVarRef* സ്ഥിരം varRefp
                = പുതിയ AstVarRef{varp->fileline(), classp, varp, VAccess::WRITE};
            varRefp->classOrPackagep(classOrPackagep);
            methodp->addPinsp(varRefp);
        }
        സ്ഥിരം size_t width = arrayElementDTypep(varp->dtypep())->width();
        methodp->addPinsp(പുതിയ AstConst{varp->fileline(), AstConst::Unsized64{}, width});
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം varnamep = പുതിയ AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                   "\"" + smtName + "\"", varp->width()};
        varnamep->dtypep(varp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(
            പുതിയ AstConst{varp->dtypep()->fileline(), AstConst::Unsized64{}, unpackedDims});
        എങ്കിൽ (randMode.usesMode && !(isGlobalConstrained && memberselp)) {
            methodp->addPinsp(
                പുതിയ AstConst{varp->fileline(), AstConst::Unsized64{}, randMode.index});
        }
        // For globalConstrained sub-object members with rand_mode:
        // Always call write_var (keeps variable in solver for constraint
        // evaluation), but toggle disabled state so the solver skips
        // write-back when rand_mode is off.
        initTaskp->addStmtsp(methodp->makeStmt());
        എങ്കിൽ (varp->lifetime().isStatic() && randMode.usesMode) {
            AstCMethodHard* സ്ഥിരം markp = പുതിയ AstCMethodHard{
                varp->fileline(),
                പുതിയ AstVarRef{varp->fileline(), VN_AS(m_genp->user2p(), കണ്ണി_Module), m_genp,
                              VAccess::READWRITE},
                VCMethod::RANDOMIZER_MARK_VAR_STATIC};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം namep = പുതിയ AstCExpr{varp->fileline(), AstCExpr::Pure{},
                                                    "\"" + smtName + "\"", varp->width()};
            namep->dtypep(varp->dtypep());
            markp->addPinsp(namep);
            markp->dtypeSetVoid();
            initTaskp->addStmtsp(markp->makeStmt());
        }
        എങ്കിൽ (isGlobalConstrained && memberselp && randMode.usesMode) {
            setRandMode(varp, memberselp, smtName, randMode, initTaskp);
        }
        // If randc, also emit markRandc() for cyclic tracking
        എങ്കിൽ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* createVarExprp(Ast_കണ്ണി_Var_Ref* nodep, സ്ഥിരം RandomizeMode randMode,
                                സ്ഥിരം std::string& smtName, സ്ഥിരം തർക്കം isGlobalConstrained,
                                AstMemberSel* memberselp, AstVar* സ്ഥിരം varp) {
        // Use AstSFormatF (not AstConst{String}) to prevent editFormat/V3Const
        // from reformatting the SMT variable name into a hex literal
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp = പുതിയ AstSFormatF{
            nodep->fileline(), m_nestedAccess ? nodep->name() : smtName, അസത്യം, nullptr};
        എങ്കിൽ (!randMode.usesMode) {
            // Global constraints keep nodep alive for write_var processing
            എങ്കിൽ (!isGlobalConstrained) VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ exprp;
        }

        // Get const format, using memberselp if available for correct width/value
        Ast_കണ്ണി_വ്യഞ്ജകം* constFormatp
            = memberselp ? getConstFormat(memberselp->clone_വൃക്ഷം(അസത്യം)) : getConstFormat(nodep);

        // Static rand vars route through the var's owning class's static array
        // (may differ from m_classp when the rand var lives in a sub-object).
        Ast_കണ്ണി_വ്യഞ്ജകം* randModeAccess;
        സ്ഥിരം തർക്കം varIsStatic = varp->lifetime().isStatic();
        AstClass* സ്ഥിരം varOwningClassp = varIsStatic ? VN_CAST(varp->user2p(), Class) : nullptr;
        AstVar* സ്ഥിരം ownerStaticRandModeVarp
            = varOwningClassp ? findStaticRandModeVarMember(varOwningClassp) : nullptr;
        എങ്കിൽ (ownerStaticRandModeVarp) {
            randModeAccess = പുതിയ AstVarRef{varp->fileline(),
                                           VN_AS(ownerStaticRandModeVarp->user2p(), കണ്ണി_Module),
                                           ownerStaticRandModeVarp, VAccess::READ};
        } അല്ലെങ്കിൽ എങ്കിൽ (memberselp) {
            Ast_കണ്ണി_Module* സ്ഥിരം varClassp = VN_AS(varp->user2p(), കണ്ണി_Module);
            AstVar* സ്ഥിരം effectiveRandModeVarp = getRandModeVarFromClass(varClassp);
            എങ്കിൽ (effectiveRandModeVarp) {
                // Member's class has randmode, use it
                Ast_കണ്ണി_വ്യഞ്ജകം* parentAccess = memberselp->fromp()->clone_വൃക്ഷം(അസത്യം);
                AstMemberSel* randModeSel
                    = പുതിയ AstMemberSel{varp->fileline(), parentAccess, effectiveRandModeVarp};
                randModeSel->dtypep(effectiveRandModeVarp->dtypep());
                randModeAccess = randModeSel;
            } അല്ലെങ്കിൽ {
                // Member's class has no randmode, use current scope's randmode
                UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
                randModeAccess
                    = പുതിയ AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), കണ്ണി_Module),
                                    m_randModeVarp, VAccess::READ};
            }
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(m_randModeVarp, nodep, "No m_randModeVarp");
            randModeAccess
                = പുതിയ AstVarRef{varp->fileline(), VN_AS(m_randModeVarp->user2p(), കണ്ണി_Module),
                                m_randModeVarp, VAccess::READ};
        }

        AstCMethodHard* സ്ഥിരം atp
            = പുതിയ AstCMethodHard{nodep->fileline(), randModeAccess, VCMethod::ARRAY_AT,
                                 പുതിയ AstConst{nodep->fileline(), randMode.index}};
        atp->dtypeSetUInt32();
        exprp = പുതിയ AstCond{varp->fileline(), atp, exprp, constFormatp};
        exprp->user1(സത്യം);  // Mark as formatted
        തിരികെ exprp;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        AstVar* varp = nodep->varp();
        എങ്കിൽ (varp->user4p()) {
            തർക്കം isSizeRef = അസത്യം;
            എങ്കിൽ (AstCMethodHard* സ്ഥിരം methodp = VN_CAST(nodep->backp(), CMethodHard)) {
                എങ്കിൽ (methodp->method() == VCMethod::ASSOC_SIZE
                    || methodp->method() == VCMethod::DYN_SIZE) {
                    isSizeRef = സത്യം;
                }
            }
            എങ്കിൽ (!isSizeRef && m_sizeConstrainedArraysp) { m_sizeConstrainedArraysp->തിരുകുക(varp); }
        }

        // Check if this variable is marked as globally constrained
        സ്ഥിരം തർക്കം isGlobalConstrained = nodep->varp()->globalConstrained();

        AstMemberSel* memberselp = nullptr;
        തർക്കം structSelOrCMeth = അസത്യം;
        // Set below only when narrowing to one struct field/array element;
        // staying null means "check varp's own type" (see its use further down).
        Ast_കണ്ണി_DType* selectedDtypep = nullptr;
        std::string smtName;
        എങ്കിൽ (m_nestedAccess) {
            m_nestedAccess->addVarNamePart(
                പുതിയ AstSFormatF{varp->fileline(), nodep->name(), അസത്യം, nullptr}, nodep->name());
            smtName = m_nestedAccess->smtName();
            memberselp = m_nestedAccess->access_വൃക്ഷം();
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->backp(), MemberSel)) {
            // Build complete path from topmost MemberSel
            Ast_കണ്ണി* മുകൾ_Member_Sel = nodep->backp();
            ആകുമ്പോൾ (VN_IS(മുകൾ_Member_Sel->backp(), MemberSel)) {
                മുകൾ_Member_Sel = മുകൾ_Member_Sel->backp();
            }
            memberselp = VN_AS(മുകൾ_Member_Sel, MemberSel)->clone_വൃക്ഷം(അസത്യം);
            smtName = buildMemberPath(memberselp);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->backp(), StructSel) || VN_IS(nodep->backp(), CMethodHard)) {
            // Build complete path for topmost StructSel or CMethodHard::ARRAY_AT
            Ast_കണ്ണി* മുകൾ_Nodep = nodep->backp();
            ആകുമ്പോൾ (VN_IS(മുകൾ_Nodep->backp(), StructSel) || VN_IS(മുകൾ_Nodep->backp(), CMethodHard)) {
                മുകൾ_Nodep = മുകൾ_Nodep->backp();
            }

            എങ്കിൽ (VN_IS(മുകൾ_Nodep, StructSel) || VN_IS(മുകൾ_Nodep, CMethodHard)) {
                structSelOrCMeth = സത്യം;
                selectedDtypep = VN_AS(മുകൾ_Nodep, കണ്ണി_വ്യഞ്ജകം)->dtypep();
            }

            memberselp = VN_CAST(മുകൾ_Nodep, MemberSel);
            എങ്കിൽ (memberselp) smtName = buildMemberPath(memberselp);

            // Above functions returned empty string, unrecognized node type,
            // Using nodep->name();
            എങ്കിൽ (smtName == "") smtName = nodep->name();
        } അല്ലെങ്കിൽ {
            // No MemberSel, StructSel or CMethodHard:at : just variable name
            smtName = nodep->name();
        }

        എങ്കിൽ (memberselp) varp = memberselp->varp();
        // The SMT translation assumes bit-vector operands and does not
        // type-check; embedding a real value produces a malformed width.
        // Check the selected field/element's type, not the whole
        // struct/array's -- a real sibling field elsewhere is irrelevant.
        എങ്കിൽ (dtypeContainsReal(structSelOrCMeth ? selectedDtypep : varp->dtypep())) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: real value in this constraint expression");
            തിരികെ;
        }
        Ast_കണ്ണി_Module* സ്ഥിരം classOrPackagep = nodep->classOrPackagep();
        സ്ഥിരം RandomizeMode randMode = {.asUQuad = varp->user1()};
        എങ്കിൽ (!randMode.usesMode && editFormat(nodep)) തിരികെ;

        AstEnumDType* enumDtp = VN_CAST(nodep->dtypep()->skipRefToEnump(), EnumDType);
        എങ്കിൽ (enumDtp
            && (std::find(m_rangeConstrainedEnums.begin(), m_rangeConstrainedEnums.end(), varp)
                == m_rangeConstrainedEnums.end())) {
            AstVar* genVarp = m_genp;
            എങ്കിൽ (m_classp && m_classp->user3p()) genVarp = VN_AS(m_classp->user3p(), Var);
            UASSERT_OBJ(genVarp, nodep, "No 'randomize' variable in m_genp or m_classp");
            Ast_കണ്ണി_വ്യഞ്ജകം* enumRangeExprp
                = createEnumConstraint(nodep->fileline(), varp->name(), enumDtp, genVarp);
            Ast_കണ്ണി_FTask* targetTaskp = m_inlineInitTaskp;
            എങ്കിൽ (!targetTaskp) targetTaskp = getCreateConstraintSetupFunc(m_classp, m_memberMap);
            UASSERT_OBJ(targetTaskp, nodep, "No function to inline enum range constraint into");
            targetTaskp->addStmtsp(enumRangeExprp->makeStmt());
            m_rangeConstrainedEnums.തള്ളുക_back(varp);
        }

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp
            = createVarExprp(nodep, randMode, smtName, isGlobalConstrained, memberselp, varp);
        relinker.relink(exprp);

        // Global / inline / class-level member-select refs key on the full path
        // (so same-type sub-objects c1.x, c2.x stay distinct); a plain class-level
        // variable keys on user3(). Inline 'with' dedups per function (own solver).
        സ്ഥിരം തർക്കം alreadyWritten = m_inlineInitTaskp     ? m_inlineWrittenVars.എണ്ണം(smtName) > 0
                                    : isGlobalConstrained ? m_writtenVars.എണ്ണം(smtName) > 0
                                    : memberselp          ? m_writtenVars.എണ്ണം(smtName) > 0
                                                          : varp->user3();
        സ്ഥിരം തർക്കം shouldWriteVar = !alreadyWritten;
        എങ്കിൽ (shouldWriteVar) {
            // Track this variable path as written
            എങ്കിൽ (m_inlineInitTaskp)
                m_inlineWrittenVars.തിരുകുക(smtName);
            അല്ലെങ്കിൽ എങ്കിൽ (isGlobalConstrained || memberselp)
                m_writtenVars.തിരുകുക(smtName);
            // For global constraints, delete nodep after processing
            എങ്കിൽ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);

            തർക്കം isClassRefArray = അസത്യം;
            തർക്കം isUnpackedClassRefArray = അസത്യം;
            AstClassRefDType* elemClassRefDtp = nullptr;
            {
                Ast_കണ്ണി_DType* varDtp = varp->dtypep()->skipRefp();
                എങ്കിൽ (varDtp->isNonPackedArray()) {
                    Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = varDtp->subDTypep()->skipRefp();
                    elemClassRefDtp = VN_CAST(elemDtp, ClassRefDType);
                    എങ്കിൽ (elemClassRefDtp) {
                        isClassRefArray = സത്യം;
                        isUnpackedClassRefArray = VN_IS(varDtp, UnpackArrayDType);
                    }
                }
            }

            Ast_കണ്ണി_Module* സ്ഥിരം classp = getLeftmostVarModulep(memberselp, varp);
            Ast_കണ്ണി_FTask* സ്ഥിരം initTaskp
                = getInitTaskp(varp, memberselp || structSelOrCMeth, classp);
            എങ്കിൽ (m_nestedAccess) {
                Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം writeVarStmtp = m_nestedAccess->makeWriteVarStmt(varp, m_genp);
                addRegistrationStmt(writeVarStmtp, initTaskp);
                എങ്കിൽ (isGlobalConstrained && memberselp && randMode.usesMode) {
                    setRandMode(varp, memberselp, smtName, randMode, initTaskp);
                }
                // If randc, also emit markRandc() for cyclic tracking
                എങ്കിൽ (varp->isRandC()) markRandc(varp, smtName, initTaskp);
            } അല്ലെങ്കിൽ എങ്കിൽ (isClassRefArray && !memberselp) {
                createSolverArrayHandle(varp, elemClassRefDtp, classOrPackagep,
                                        isUnpackedClassRefArray, smtName);
            } അല്ലെങ്കിൽ {
                createSolverVarHandle(varp, structSelOrCMeth, isGlobalConstrained, randMode,
                                      memberselp, smtName, classOrPackagep, classp, initTaskp);
            }
        } അല്ലെങ്കിൽ {
            // Variable already written, clean up cloned memberselp if any
            എങ്കിൽ (memberselp) VL_DO_DANGLING(memberselp->delete_വൃക്ഷം(), memberselp);
            // Delete nodep if it's a global constraint (not deleted yet)
            എങ്കിൽ (isGlobalConstrained && !nodep->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
        എങ്കിൽ (m_nestedAccess) { VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess); }
    }
    // Build popcount expansion: (x & 1) + ((x & 2) >> 1) + ...
    // argp is consumed; caller must clone if reusing.
    Ast_കണ്ണി_വ്യഞ്ജകം* build_എണ്ണം_Ones_Expansion(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* argp) {
        സ്ഥിരം V3_സംഖ്യ അക്കം_One{fl, argp->width(), 1};
        Ast_കണ്ണി_വ്യഞ്ജകം* sump = പുതിയ AstAnd{fl, argp, പുതിയ AstConst{fl, അക്കം_One}};
        sump->user1(സത്യം);
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1; i < argp->width(); i++) {
            V3_സംഖ്യ അക്കം_Bit_Mask{fl, argp->width(), 0};
            അക്കം_Bit_Mask.setBit(i, 1);
            AstAnd* സ്ഥിരം andp
                = പുതിയ AstAnd{fl, argp->clone_വൃക്ഷം_Pure(അസത്യം), പുതിയ AstConst{fl, അക്കം_Bit_Mask}};
            andp->user1(സത്യം);
            AstShiftR* സ്ഥിരം shiftp
                = പുതിയ AstShiftR{fl, andp,
                                പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, argp->width(),
                                             static_cast<uint32_t>(i)}};
            shiftp->user1(സത്യം);
            sump = പുതിയ AstAdd{fl, sump, shiftp};
            sump->user1(സത്യം);
        }
        തിരികെ sump;
    }

    ശൂന്യം visit(Ast_എണ്ണം_Ones* nodep) override {
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* sump = build_എണ്ണം_Ones_Expansion(fl, argp);
        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(സത്യം);
        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(sump);
    }
    ശൂന്യം visit(AstRedXor* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;

        // Build popcount expansion: (extract x 1 1) ^ (extract x 2 2) ^ ...
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = nodep->lhsp()->unlinkFrBack();

        Ast_കണ്ണി_വ്യഞ്ജകം* redxorp = പുതിയ AstSel{fl, argp, 0, 1};
        redxorp->user1(സത്യം);
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1; i < argp->width(); i++) {
            AstSel* സ്ഥിരം selp = പുതിയ AstSel{fl, argp->clone_വൃക്ഷം_Pure(അസത്യം), i, 1};
            selp->user1(സത്യം);

            redxorp = പുതിയ AstXor{fl, redxorp, selp};
            redxorp->user1(സത്യം);
        }

        nodep->replaceWith(redxorp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(redxorp);
    }
    ശൂന്യം visit(AstRedAnd* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        // Convert to (~x == 0)
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = nodep->lhsp()->unlinkFrBack();
        സ്ഥിരം V3_സംഖ്യ അക്കം_Zero{fl, argp->width(), 0};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം negp = പുതിയ AstNot{fl, argp};
        negp->user1(സത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eqp = പുതിയ AstEq{fl, negp, പുതിയ AstConst{fl, അക്കം_Zero}};
        eqp->user1(സത്യം);
        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(eqp);
    }
    ശൂന്യം visit(AstRedOr* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        // Convert to (x != 0)
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = nodep->lhsp()->unlinkFrBack();
        സ്ഥിരം V3_സംഖ്യ അക്കം_Zero{fl, argp->width(), 0};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം neqp = പുതിയ AstNeq{fl, argp, പുതിയ AstConst{fl, അക്കം_Zero}};
        neqp->user1(സത്യം);
        nodep->replaceWith(neqp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(neqp);
    }
    ശൂന്യം visit(AstOneHot* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        // $onehot(x) = (x != 0) && ((x & (x-1)) == 0)
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = nodep->lhsp()->unlinkFrBack();
        സ്ഥിരം പൂർണ്ണസംഖ്യ w = argp->width();

        സ്ഥിരം V3_സംഖ്യ അക്കം_Zero{fl, w, 0};
        AstNeq* സ്ഥിരം neZerop
            = പുതിയ AstNeq{fl, argp->clone_വൃക്ഷം_Pure(അസത്യം), പുതിയ AstConst{fl, അക്കം_Zero}};
        neZerop->user1(സത്യം);

        സ്ഥിരം V3_സംഖ്യ അക്കം_One{fl, w, 1};
        AstSub* സ്ഥിരം subp = പുതിയ AstSub{fl, argp->clone_വൃക്ഷം_Pure(അസത്യം), പുതിയ AstConst{fl, അക്കം_One}};
        subp->user1(സത്യം);

        AstAnd* സ്ഥിരം andp = പുതിയ AstAnd{fl, argp, subp};
        andp->user1(സത്യം);

        സ്ഥിരം V3_സംഖ്യ അക്കം_Zero2{fl, w, 0};
        AstEq* സ്ഥിരം eqZerop = പുതിയ AstEq{fl, andp, പുതിയ AstConst{fl, അക്കം_Zero2}};
        eqZerop->user1(സത്യം);

        AstLogAnd* സ്ഥിരം resultp = പുതിയ AstLogAnd{fl, neZerop, eqZerop};
        resultp->user1(സത്യം);

        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(resultp);
    }
    ശൂന്യം visit(AstOneHot0* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        // $onehot0(x) = (x & (x-1)) == 0
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = nodep->lhsp()->unlinkFrBack();
        സ്ഥിരം പൂർണ്ണസംഖ്യ w = argp->width();

        സ്ഥിരം V3_സംഖ്യ അക്കം_One{fl, w, 1};
        AstSub* സ്ഥിരം subp = പുതിയ AstSub{fl, argp->clone_വൃക്ഷം_Pure(അസത്യം), പുതിയ AstConst{fl, അക്കം_One}};
        subp->dtypeFrom(argp);
        subp->user1(സത്യം);

        AstAnd* സ്ഥിരം andp = പുതിയ AstAnd{fl, argp, subp};
        andp->dtypeFrom(argp);
        andp->user1(സത്യം);

        സ്ഥിരം V3_സംഖ്യ അക്കം_Zero{fl, w, 0};
        AstEq* സ്ഥിരം eqp = പുതിയ AstEq{fl, andp, പുതിയ AstConst{fl, അക്കം_Zero}};
        eqp->user1(സത്യം);

        nodep->replaceWith(eqp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(eqp);
    }
    ശൂന്യം visit(Ast_എണ്ണം_Bits* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        File_വരി* സ്ഥിരം fl = nodep->fileline();

        തർക്കം എണ്ണം_Ones = അസത്യം;
        തർക്കം എണ്ണം_Zeros = അസത്യം;
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* ctrlp : {nodep->rhsp(), nodep->thsp(), nodep->fhsp()}) {
            സ്ഥിരം AstConst* സ്ഥിരം cp = VN_CAST(ctrlp, Const);
            എങ്കിൽ (!cp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: non-constant control in $countbits inside constraint");
                AstConst* സ്ഥിരം zerop
                    = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, nodep->width(), 0U};
                nodep->replaceWith(zerop);
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                തിരികെ;
            }
            എങ്കിൽ (cp->അക്കം().bitIs1(0) && !എണ്ണം_Ones)
                എണ്ണം_Ones = സത്യം;
            അല്ലെങ്കിൽ എങ്കിൽ (cp->അക്കം().bitIs0(0) && !എണ്ണം_Zeros)
                എണ്ണം_Zeros = സത്യം;
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = nodep->lhsp()->unlinkFrBack();
        സ്ഥിരം പൂർണ്ണസംഖ്യ argWidth = argp->width();
        Ast_കണ്ണി_വ്യഞ്ജകം* sump = nullptr;
        എങ്കിൽ (എണ്ണം_Ones && എണ്ണം_Zeros) {
            // ones + zeros = width for 2-state types
            sump = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, nodep->width(), (uint32_t)argWidth};
            VL_DO_DANGLING(argp->delete_വൃക്ഷം(), argp);
        } അല്ലെങ്കിൽ എങ്കിൽ (എണ്ണം_Ones) {
            sump = build_എണ്ണം_Ones_Expansion(fl, argp);
        } അല്ലെങ്കിൽ എങ്കിൽ (എണ്ണം_Zeros) {
            // width - countones(x)
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം onesCountp = build_എണ്ണം_Ones_Expansion(fl, argp);
            സ്ഥിരം V3_സംഖ്യ widthVal{nodep, onesCountp->width(), (uint32_t)argWidth};
            sump = പുതിയ AstSub{fl, പുതിയ AstConst{fl, widthVal}, onesCountp};
            sump->user1(സത്യം);
        } അല്ലെങ്കിൽ {
            sump = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, nodep->width(), 0U};
            VL_DO_DANGLING(argp->delete_വൃക്ഷം(), argp);
        }

        sump = adjustWidth(fl, sump, nodep->width(), nodep->dtypep()->numeric());
        sump->user1(സത്യം);

        nodep->replaceWith(sump);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(sump);
    }
    ശൂന്യം visit(AstCLog2* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        // $clog2(x): ITE chain (x<=1)?0 : (x<=2)?1 : ... : argWidth
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argp = nodep->lhsp()->unlinkFrBack();
        സ്ഥിരം പൂർണ്ണസംഖ്യ argWidth = argp->width();
        സ്ഥിരം പൂർണ്ണസംഖ്യ resultWidth = nodep->width();

        Ast_കണ്ണി_വ്യഞ്ജകം* resultp
            = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, resultWidth, (uint32_t)argWidth};

        വേണ്ടി (പൂർണ്ണസംഖ്യ k = argWidth - 1; k >= 0; k--) {
            V3_സംഖ്യ threshold{fl, argWidth, 0};
            എങ്കിൽ (k < 32)
                threshold.setLong(1ULL << k);
            അല്ലെങ്കിൽ
                threshold.setBit(k, 1);

            AstLte* സ്ഥിരം ltep
                = പുതിയ AstLte{fl, argp->clone_വൃക്ഷം_Pure(അസത്യം), പുതിയ AstConst{fl, threshold}};
            ltep->user1(സത്യം);

            AstConst* സ്ഥിരം valuep
                = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, resultWidth, (uint32_t)k};

            resultp = പുതിയ AstCond{fl, ltep, valuep, resultp};
            resultp->dtypeChgSigned(VSigning::SIGNED);
            resultp->user1(സത്യം);
        }

        VL_DO_DANGLING(argp->delete_വൃക്ഷം(), argp);
        nodep->replaceWith(resultp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(resultp);
    }
    ശൂന്യം visit(AstIsUnbounded* nodep) override {
        // Only true for the literal '$' token, never a real expression, so
        // this always folds to false. It has no runtime implementation of
        // its own, so it must be substituted now rather than left for later.
        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: $isunbounded() in a constraint, treating as constant");
        AstConst* സ്ഥിരം zerop = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        nodep->replaceWith(zerop);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        // So the constant is rendered as an SMT literal like every other
        // folded constant here, instead of being left behind unconverted.
        iterate(zerop);
    }
    ശൂന്യം handlePow(Ast_കണ്ണി_Biop* nodep) {
        എങ്കിൽ (AstConst* സ്ഥിരം exponentp = VN_CAST(nodep->rhsp(), Const)) {
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം basep = nodep->lhsp();
            സ്ഥിരം V3_സംഖ്യ അക്കം_One{nodep, basep->width(), 1};
            Ast_കണ്ണി_വ്യഞ്ജകം* powerp = പുതിയ AstConst{fl, അക്കം_One};
            സ്ഥിരം തർക്കം baseSigned = VN_IS(nodep, PowSS) || VN_IS(nodep, PowSU);
            സ്ഥിരം int32_t exponent = baseSigned ? exponentp->toSInt() : exponentp->toUInt();
            എങ്കിൽ (exponent > 0) {
                വേണ്ടി (int32_t i = 0; i < exponent; i++) {
                    എങ്കിൽ (baseSigned) {
                        powerp = പുതിയ AstMulS{fl, powerp, basep->clone_വൃക്ഷം_Pure(അസത്യം)};
                    } അല്ലെങ്കിൽ {
                        powerp = പുതിയ AstMul{fl, powerp, basep->clone_വൃക്ഷം_Pure(അസത്യം)};
                    }
                    powerp->user1(സത്യം);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (exponent < 0) {
                // Limit chain of divisions to max 2, because operations are on integers.
                // Two divisions are needed to preserve the sign.
                എങ്കിൽ (baseSigned) {
                    powerp = പുതിയ AstDivS{fl, powerp, basep->clone_വൃക്ഷം_Pure(അസത്യം)};
                    powerp->user1(സത്യം);
                    powerp = പുതിയ AstDivS{fl, powerp, basep->clone_വൃക്ഷം_Pure(അസത്യം)};
                    powerp->user1(സത്യം);
                } അല്ലെങ്കിൽ {
                    powerp = പുതിയ AstDiv{fl, powerp, basep->clone_വൃക്ഷം_Pure(അസത്യം)};
                    powerp->user1(സത്യം);
                }
            }
            nodep->replaceWith(powerp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            iterate(powerp);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം basep = VN_CAST(nodep->lhsp(), Const)) {
            എങ്കിൽ (basep->അക്കം().toUInt() == 2) {
                // Non-constant exponent, base == 2: transform to shift-based SMT expression.
                // AstPow/AstPowSU (unsigned exponent): 2**n -> 1<<n
                // AstPowSS/AstPowUS (signed exponent): 2**n -> n>=0 ? 1<<n : 0
                // IEEE 1800-2023 ss11.4.3: integer base with negative exponent gives 0,
                // which ShiftL cannot represent; use SMT ite (AstCond -> bvsge + bvshl).
                File_വരി* സ്ഥിരം fl = nodep->fileline();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
                സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->width();
                സ്ഥിരം തർക്കം rhsDependent = rhsp->user1();
                AstConst* സ്ഥിരം onep = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, width, 1};
                AstShiftL* സ്ഥിരം shiftp = പുതിയ AstShiftL{fl, onep, rhsp, width};
                shiftp->dtypeFrom(nodep);
                shiftp->user1(rhsDependent);
                Ast_കണ്ണി_വ്യഞ്ജകം* resultp = shiftp;
                എങ്കിൽ (VN_IS(nodep, PowSS) || VN_IS(nodep, PowUS)) {
                    // Signed exponent: guard 1<<n with n>=0 check
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nClonep = rhsp->clone_വൃക്ഷം_Pure(അസത്യം);
                    nClonep->user1(rhsDependent);
                    AstConst* സ്ഥിരം zeroNp
                        = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, rhsp->width(), 0};
                    AstGteS* സ്ഥിരം condp = പുതിയ AstGteS{fl, nClonep, zeroNp};
                    condp->user1(rhsDependent);
                    AstConst* സ്ഥിരം zeroRp = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, width, 0};
                    resultp = പുതിയ AstCond{fl, condp, shiftp, zeroRp};
                    resultp->dtypeFrom(nodep);
                    resultp->user1(nodep->user1());
                }
                nodep->replaceWith(resultp);
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                iterate(resultp);
            } അല്ലെങ്കിൽ {
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: Power (**) expression with non-2 base in constraint");
            }
        } അല്ലെങ്കിൽ {
            nodep->v3warn(
                CONSTRAINTIGN,
                "Unsupported: Power (**) expression with non-constant exponent in constraint");
        }
    }
    ശൂന്യം visit(AstPow* nodep) override { handlePow(nodep); }
    ശൂന്യം visit(AstPowSS* nodep) override { handlePow(nodep); }
    ശൂന്യം visit(AstPowSU* nodep) override { handlePow(nodep); }
    ശൂന്യം visit(AstPowUS* nodep) override { handlePow(nodep); }
    // SMT-LIB2 shift operations (bvshl/bvlshr/bvashr) require both operands
    // to have the same bitvector width. Extend the narrower operand, then
    // truncate the result if the value was widened. Never truncate the shift
    // amount, as large shifts must still shift out all of the original bits.
    ശൂന്യം handleShift(Ast_കണ്ണി_Biop* nodep) {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        സ്ഥിരം പൂർണ്ണസംഖ്യ lhsWidth = nodep->lhsp()->width();
        സ്ഥിരം പൂർണ്ണസംഖ്യ rhsWidth = nodep->rhsp()->width();
        എങ്കിൽ (rhsWidth < lhsWidth) {
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
            AstExtend* സ്ഥിരം extendp = പുതിയ AstExtend{fl, rhsp, lhsWidth};
            // Always visit the wrapper: a rand MemberSel can have user1 unset until its
            // own visitor translates it. Copying that flag would format the entire
            // extension from the pre-randomize value. Non-rand children are still
            // formatted as constants by their own visitors.
            extendp->user1(സത്യം);
            nodep->rhsp(extendp);
        } അല്ലെങ്കിൽ എങ്കിൽ (rhsWidth > lhsWidth) {
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->dtypep();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം extendp
                = VN_IS(nodep, ShiftRS)
                      ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtendS{fl, lhsp, rhsWidth})
                      : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtend{fl, lhsp, rhsWidth});
            // Likewise, visit the extension instead of prematurely formatting rand
            // member selections as constants.
            extendp->user1(സത്യം);
            nodep->lhsp(extendp);
            nodep->dtypeSetLogicSized(rhsWidth, dtypep->numeric());
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            AstSel* സ്ഥിരം selp = പുതിയ AstSel{fl, nodep, 0, lhsWidth};
            selp->dtypep(dtypep);
            selp->user1(nodep->user1());
            relinker.relink(selp);
            iterate(selp);
            തിരികെ;
        }
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ശൂന്യം visit(AstShiftL* nodep) override { handleShift(nodep); }
    ശൂന്യം visit(AstShiftR* nodep) override { handleShift(nodep); }
    ശൂന്യം visit(AstShiftRS* nodep) override { handleShift(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_Biop* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Uniop* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        editSMT(nodep, nodep->lhsp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Triop* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        editSMT(nodep, nodep->lhsp(), nodep->rhsp(), nodep->thsp());
    }
    ശൂന്യം visit(AstCond* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        എങ്കിൽ (!nodep->condp()->user1()) {
            // Do not burden the solver if cond computable: (cond ? "then" : "else")
            iterate(nodep->thenp());
            iterate(nodep->elsep());
            UASSERT_OBJ(
                nodep->thenp()->isString() && nodep->elsep()->isString(), nodep,
                "Branches of conditional expression in constraint not converted to strings");
            nodep->dtypeSetString();
            തിരികെ;
        }
        // Fall back to "(ite cond then else)"
        visit(static_cast<Ast_കണ്ണി_Triop*>(nodep));
    }
    ശൂന്യം visit(AstReplicate* nodep) override {
        // Biop, but RHS is harmful
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        editSMT(nodep, nodep->srcp());
    }
    ശൂന്യം visit(AstSel* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        VNRelinker handle;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം msbp = പുതിയ AstSFormatF{
            fl, "%1d", അസത്യം,
            പുതിയ AstAdd{fl, nodep->lsbp()->clone_വൃക്ഷം_Pure(അസത്യം),
                       പുതിയ AstConst{fl, static_cast<uint32_t>(nodep->widthConst() - 1)}}};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp
            = പുതിയ AstSFormatF{fl, "%1d", അസത്യം, nodep->lsbp()->unlinkFrBack(&handle)};
        handle.relink(lsbp);

        editSMT(nodep, nodep->fromp(), lsbp, msbp);
    }
    ശൂന്യം visit(AstStructSel* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        സ്ഥിരം തർക്കം outermost = !m_deferRandModeHoist;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origp = outermost ? nodep->clone_വൃക്ഷം(അസത്യം) : nullptr;
        VL_RESTORER(m_structSel);
        m_structSel = സത്യം;
        എങ്കിൽ (VN_IS(nodep->fromp()->dtypep()->skipRefp(), StructDType)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = nodep->fromp();
            AstStructDType* സ്ഥിരം structp = VN_AS(fromp->dtypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            എങ്കിൽ (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep());
            } അല്ലെങ്കിൽ {
                structp->markConstrainedRand(സത്യം);
            }
        }
        // Mark Random for structArray
        എങ്കിൽ (VN_IS(nodep->fromp(), ArraySel) || VN_IS(nodep->fromp(), CMethodHard)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = VN_IS(nodep->fromp(), ArraySel)
                                           ? VN_AS(nodep->fromp(), ArraySel)->fromp()
                                           : VN_AS(nodep->fromp(), CMethodHard)->fromp();
            AstStructDType* സ്ഥിരം structp
                = VN_AS(fromp->dtypep()->skipRefp()->subDTypep()->skipRefp(), StructDType);
            markConstrainedRandMember(structp, nodep->name());
            എങ്കിൽ (isRandQualifiedPath(fromp)) {
                markStructConstrainedRandRecurse(fromp->dtypep()->skipRefp()->subDTypep());
            } അല്ലെങ്കിൽ {
                structp->markConstrainedRand(സത്യം);
            }
        }
        {
            VL_RESTORER(m_deferRandModeHoist);
            m_deferRandModeHoist = സത്യം;
            iterateChildren(nodep);
        }
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        AstSFormatF* newp = nullptr;
        എങ്കിൽ (VN_AS(nodep->fromp(), SFormatF)->name() == "%s.%s") {
            newp = പുതിയ AstSFormatF{fl, "%s.%s." + nodep->name(), അസത്യം,
                                   VN_AS(nodep->fromp(), SFormatF)->exprsp()->clone_വൃക്ഷം_Pure(സത്യം)};
            എങ്കിൽ (newp->exprsp()->nextp()->name().rfind("#x", 0) == 0)
                newp->exprsp()->nextp()->name("%x");  //  for #x%x to %x
        } അല്ലെങ്കിൽ {
            newp = പുതിയ AstSFormatF{fl, nodep->fromp()->name() + "." + nodep->name(), അസത്യം,
                                   nullptr};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        എങ്കിൽ (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(origp), origp);
        }
    }
    ശൂന്യം addStringNamePart(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം indexp, സ്ഥിരം std::string& fmt = "%32x") {
        എങ്കിൽ (m_nestedAccess) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bitp = indexp->clone_വൃക്ഷം_Pure(അസത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bitFormatp = പുതിയ AstSFormatF{bitp->fileline(), fmt, അസത്യം, bitp};
            AstSFormatF* സ്ഥിരം herep
                = പുതിയ AstSFormatF{indexp->fileline(), "%s.%s", അസത്യം, bitFormatp};
            m_nestedAccess->addVarNamePart(herep, indexp->name());
        }
    }
    ശൂന്യം visit(AstAssocSel* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        // Keep a pre-edit clone for the rand_mode hoist below.
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origp = nodep->clone_വൃക്ഷം(അസത്യം);
        AstSFormatF* newp = nullptr;
        എങ്കിൽ (VN_IS(nodep->bitp(), VarRef) && VN_AS(nodep->bitp(), VarRef)->isString()) {
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം idxp = പുതിയ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), അസത്യം,
                                                      nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->bitp(), CvtPackString)
                   && VN_IS(nodep->bitp()->dtypep(), BasicDType)) {
            AstCvtPackString* സ്ഥിരം stringp = VN_AS(nodep->bitp(), CvtPackString);
            സ്ഥിരം size_t stringSize = VN_AS(stringp->lhsp(), Const)->width();
            എങ്കിൽ (stringSize > 128) {
                stringp->v3warn(
                    CONSTRAINTIGN,
                    "Unsupported: Constrained randomization of associative array keys of "
                        << stringSize << "bits, limit is 128 bits");
            }
            addStringNamePart(nodep->bitp());
            VNRelinker handle;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം idxp = പുതിയ AstSFormatF{fl, (m_structSel ? "%32x" : "#x%32x"), അസത്യം,
                                                      stringp->lhsp()->unlinkFrBack(&handle)};
            handle.relink(idxp);
            newp = editSMT(nodep, nodep->fromp(), idxp);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (VN_IS(nodep->bitp()->dtypep(), BasicDType)
                || (VN_IS(nodep->bitp()->dtypep(), StructDType)
                    && VN_AS(nodep->bitp()->dtypep(), StructDType)->packed())
                || VN_IS(nodep->bitp()->dtypep(), EnumDType)
                || VN_IS(nodep->bitp()->dtypep(), PackArrayDType)) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ actual_width = nodep->bitp()->width();
                std::string fmt;
                // Normalize to standard bit width
                എങ്കിൽ (actual_width <= 8) {
                    fmt = m_structSel ? "%2x" : "#x%2x";
                } അല്ലെങ്കിൽ എങ്കിൽ (actual_width <= 16) {
                    fmt = m_structSel ? "%4x" : "#x%4x";
                } അല്ലെങ്കിൽ {
                    fmt = (m_structSel ? "%" : "#x%")
                          + std::to_string(VL_വാക്കുകൾ_I(actual_width) * 8) + "x";
                }
                എങ്കിൽ (m_nestedAccess) addStringNamePart(nodep->bitp(), fmt);
                VNRelinker handle;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം idxp
                    = പുതിയ AstSFormatF{fl, fmt, അസത്യം, nodep->bitp()->unlinkFrBack(&handle)};
                handle.relink(idxp);
                newp = editSMT(nodep, nodep->fromp(), idxp);
            } അല്ലെങ്കിൽ {
                nodep->bitp()->v3error(
                    "Illegal non-integral expression or subexpression in random constraint."
                    " (IEEE 1800-2023 18.3)");
                എങ്കിൽ (m_nestedAccess) m_nestedAccess->error();
            }
        }
        എങ്കിൽ (newp && m_structSel && newp->name() == "(select %s %s)") { newp->name("%s.%s"); }
        എങ്കിൽ (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
            VL_DO_DANGLING(origp->delete_വൃക്ഷം(), origp);
        }
    }
    ശൂന്യം rename_Array_വ്യഞ്ജകം(AstSFormatF* സ്ഥിരം newp) സ്ഥിരം {
        എങ്കിൽ (m_structSel) {
            newp->name("%s.%s");
            newp->exprsp()->nextp()->name("%x");
        }
    }
    ശൂന്യം visit(AstArraySel* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        VNRelinker handle;
        // Check if index actually references a rand variable (not just user1,
        // which can be over-marked in sum/with expansion contexts). A
        // std::randomize() with-clause argument carries no rand qualifier of
        // its own but is still part of the solve.
        തർക്കം indexIsRand = അസത്യം;
        nodep->bitp()->foreach([&](സ്ഥിരം Ast_കണ്ണി_Var_Ref* vrefp) {
            എങ്കിൽ (vrefp->varp()->rand().isRandomizable() || vrefp->varp()->isStdRandomizeArg()) {
                indexIsRand = സത്യം;
            }
        });
        എങ്കിൽ (m_nestedAccess) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bitp = nodep->bitp()->clone_വൃക്ഷം_Pure(അസത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bitFormatp
                = indexIsRand ? പുതിയ AstSFormatF{bitp->fileline(), bitp->name(), അസത്യം, nullptr}
                              : പുതിയ AstSFormatF{bitp->fileline(), "%x", അസത്യം, bitp};
            AstSFormatF* സ്ഥിരം herep
                = പുതിയ AstSFormatF{nodep->fileline(), "%s.%s", അസത്യം, bitFormatp};
            എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(bitp, Sel)) {
                m_nestedAccess->addVarNamePart(herep, selp->fromp()->name());
            } അല്ലെങ്കിൽ {
                m_nestedAccess->addVarNamePart(herep, bitp->name());
            }
            എങ്കിൽ (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomized index in complex expression");
                m_nestedAccess->error();
                VL_DO_DANGLING(bitp->delete_വൃക്ഷം(), bitp);
                തിരികെ;
            }
        }
        എങ്കിൽ (indexIsRand) {
            // Index depends on rand variable -- keep as SMT symbol.
            // Array index sort is 32-bit, so zero-extend narrower indices.
            Ast_കണ്ണി_വ്യഞ്ജകം* indexp = nodep->bitp()->unlinkFrBack(&handle);
            എങ്കിൽ (indexp->width() < 32) {
                AstExtend* സ്ഥിരം extendp = പുതിയ AstExtend{fl, indexp, 32};
                extendp->user1(സത്യം);
                indexp = extendp;
            }
            handle.relink(indexp);
            AstSFormatF* സ്ഥിരം newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_വ്യഞ്ജകം(newp);
        } അല്ലെങ്കിൽ {
            // Index is constant or non-rand -- format as hex literal.
            // Keep a pre-edit clone for the rand_mode hoist below.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origp = nodep->clone_വൃക്ഷം(അസത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം indexp
                = പുതിയ AstSFormatF{fl, "#x%8x", അസത്യം, nodep->bitp()->unlinkFrBack(&handle)};
            handle.relink(indexp);
            AstSFormatF* സ്ഥിരം newp = editSMT(nodep, nodep->fromp(), indexp);
            rename_Array_വ്യഞ്ജകം(newp);
            എങ്കിൽ (!newp || !hoistRandModeOverSelectAndMember(newp, origp)) {
                VL_DO_DANGLING(origp->delete_വൃക്ഷം(), origp);
            }
        }
    }
    // True iff exprp is an ARRAY_AT read of a class's own instance mode array.
    സ്ഥിതം തർക്കം isRandModeGate(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        സ്ഥിരം AstCMethodHard* സ്ഥിരം atp = VN_CAST(exprp, CMethodHard);
        എങ്കിൽ (!atp || atp->method() != VCMethod::ARRAY_AT) തിരികെ അസത്യം;
        സ്ഥിരം AstVar* varp = nullptr;
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(atp->fromp(), VarRef)) {
            varp = refp->varp();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം mselp = VN_CAST(atp->fromp(), MemberSel)) {
            varp = mselp->varp();
        }
        എങ്കിൽ (!varp) തിരികെ അസത്യം;
        സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(varp->user2p(), Class);
        തിരികെ classp && classp->user2p() == varp;
    }
    // Lift a rand_mode Cond above the (select ...) chain of a frozen array element.
    തർക്കം hoistRandModeOverSelectAndMember(AstSFormatF* newp, Ast_കണ്ണി_വ്യഞ്ജകം* origp) {
        എങ്കിൽ (m_deferRandModeHoist) തിരികെ അസത്യം;
        // Only for selects yielding a non-array element (full chains)
        എങ്കിൽ (VN_IS(origp->dtypep()->skipRefp(), UnpackArrayDType)) തിരികെ അസത്യം;
        // Walk nested "(select %s %s)" frames down to a mode-gating AstCond
        std::vector<AstSFormatF*> frames;
        AstCond* modeCondp = nullptr;
        വേണ്ടി (AstSFormatF* curp = newp;
             curp
             && (curp->name() == "(select %s %s)" || curp->name() == "%s.%s"
                 || curp->name().rfind("%s.", 0) == 0);) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം firstp = curp->exprsp();
            frames.തള്ളുക_back(curp);
            എങ്കിൽ (AstCond* സ്ഥിരം condp = VN_CAST(firstp, Cond)) {
                എങ്കിൽ (isRandModeGate(condp->condp())) modeCondp = condp;
                നിർത്തുക;
            }
            curp = VN_CAST(firstp, SFormatF);
        }
        എങ്കിൽ (!modeCondp) തിരികെ അസത്യം;
        File_വരി* സ്ഥിരം fl = newp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം modep = modeCondp->condp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* activep = modeCondp->thenp()->unlinkFrBack();
        // Rebuild the select chain text around the SMT name, innermost first
        വേണ്ടി (auto it = frames.rbegin(); it != frames.rend(); ++it) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം remainingArgsp = VN_CAST((*it)->exprsp()->nextp(), കണ്ണി_വ്യഞ്ജകം);
            എങ്കിൽ (remainingArgsp) { remainingArgsp->unlinkFrBackWithNext(); }
            activep = പുതിയ AstSFormatF{fl, (*it)->name(), അസത്യം,
                                      Ast_കണ്ണി::addNext(activep, remainingArgsp)};
        }
        AstCond* സ്ഥിരം hoistp = പുതിയ AstCond{fl, modep, activep, getConstFormat(origp)};
        hoistp->user1(സത്യം);  // Mark as formatted
        newp->replaceWith(hoistp);
        VL_DO_DANGLING(തള്ളുക_Deletep(newp), newp);
        തിരികെ സത്യം;
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        // Check if rootVar is globalConstrained
        എങ്കിൽ (nodep->varp()->rand().isRandomizable() && nodep->fromp()) {
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            എങ്കിൽ (m_nestedAccess) {
                AstSFormatF* സ്ഥിരം formatp = പുതിയ AstSFormatF{fl, nodep->name(), അസത്യം, nullptr};
                m_nestedAccess->addVarNamePart(പുതിയ AstSFormatF{fl, "%s.%s", അസത്യം, formatp},
                                               nodep->name());
            }
            Ast_കണ്ണി* വേര്_കണ്ണി = nodep->fromp();
            ആകുമ്പോൾ (സ്ഥിരം AstMemberSel* സ്ഥിരം selp = VN_CAST(വേര്_കണ്ണി, MemberSel)) {
                വേര്_കണ്ണി = selp->fromp();
            }

            Ast_കണ്ണി_Sel* arraySelp = VN_CAST(വേര്_കണ്ണി, ArraySel);
            എങ്കിൽ (!arraySelp) arraySelp = VN_CAST(വേര്_കണ്ണി, AssocSel);

            എങ്കിൽ (arraySelp) {
                Ast_കണ്ണി_DType* സ്ഥിരം arrayDtp = arraySelp->fromp()->dtypep()->skipRefp();
                Ast_കണ്ണി_DType* സ്ഥിരം elemDtp
                    = arrayDtp->subDTypep() ? arrayDtp->subDTypep()->skipRefp() : nullptr;
                എങ്കിൽ (elemDtp && VN_IS(elemDtp, ClassRefDType)) {
                    സ്ഥിരം തർക്കം isSimple
                        = nodep->fromp() == വേര്_കണ്ണി && VN_IS(arraySelp->fromp(), VarRef);
                    എങ്കിൽ (!isSimple && !m_nestedAccess) {
                        m_nestedAccess = പുതിയ NestedAccessPath(nodep->clone_വൃക്ഷം(അസത്യം),
                                                              &m_firstExpressionInsideIndexp);
                        AstSFormatF* സ്ഥിരം formatp
                            = പുതിയ AstSFormatF{fl, nodep->name(), അസത്യം, nullptr};
                        m_nestedAccess->addVarNamePart(
                            പുതിയ AstSFormatF{fl, "%s.%s", അസത്യം, formatp}, nodep->name());
                    }
                    VL_RESTORER(m_structSel);
                    m_structSel = സത്യം;
                    nodep->user1(സത്യം);
                    arraySelp->user1(സത്യം);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origp = nodep->clone_വൃക്ഷം(അസത്യം);
                    iterateChildren(nodep);
                    AstSFormatF* സ്ഥിരം newp = പുതിയ AstSFormatF{fl, "%s." + nodep->name(), അസത്യം,
                                                              nodep->fromp()->unlinkFrBack()};
                    nodep->replaceWith(newp);
                    എങ്കിൽ (!hoistRandModeOverSelectAndMember(newp, origp))
                        VL_DO_DANGLING(origp->delete_വൃക്ഷം(), origp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    VL_DO_DANGLING(delete m_nestedAccess, m_nestedAccess);
                    തിരികെ;
                }
            }
            // Check if the root variable participates in global constraints
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം varRefp = VN_CAST(വേര്_കണ്ണി, VarRef)) {
                AstVar* സ്ഥിരം constrainedVar = varRefp->varp();
                എങ്കിൽ (constrainedVar->globalConstrained()) {
                    തർക്കം സ്ഥിരം beforeIterate = m_nestedAccess;
                    // Global constraint - unwrap the MemberSel
                    iterateChildren(nodep);
                    എങ്കിൽ (beforeIterate) {
                        nodep->replaceWith(പുതിയ AstSFormatF{fl, "%s." + nodep->name(), അസത്യം,
                                                           nodep->fromp()->unlinkFrBack()});
                    } അല്ലെങ്കിൽ {
                        nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                    }
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                    തിരികെ;
                }
            }
        }
        // Handle MemberSel references created by captureRefByThis()
        എങ്കിൽ (VN_IS(nodep->fromp(), VarRef)
            && nodep->fromp()->user1()  // Depending on a randomized variable
            && nodep->user2p()  // Pointer to containing module
            && VN_AS(nodep->user2p(), കണ്ണി_Module) == nodep->varp()->user2p()) {
            // Convert to VarRef
            AstVarRef* സ്ഥിരം varRefp
                = പുതിയ AstVarRef{nodep->fileline(), nodep->varp(), VAccess::READ};
            varRefp->user1(nodep->varp()->rand().isRandomizable());
            varRefp->classOrPackagep(VN_AS(nodep->user2p(), കണ്ണി_Module));
            nodep->replaceWith(varRefp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            visit(varRefp);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->user1() && VN_IS(nodep->fromp(), CMethodHard)
                   && VN_AS(nodep->fromp(), CMethodHard)->method() == VCMethod::ARRAY_AT) {
            // Class ref array element member access (e.g., items[i].val)
            AstCMethodHard* സ്ഥിരം cmethodp = VN_AS(nodep->fromp(), CMethodHard);
            Ast_കണ്ണി_DType* സ്ഥിരം arrayDtp = cmethodp->fromp()->dtypep()->skipRefp();
            Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = arrayDtp->subDTypep()->skipRefp();
            എങ്കിൽ (VN_IS(elemDtp, ClassRefDType)) {
                VL_RESTORER(m_structSel);
                m_structSel = സത്യം;
                iterateChildren(nodep);
                File_വരി* സ്ഥിരം fl = nodep->fileline();
                AstSFormatF* newp = nullptr;
                എങ്കിൽ (AstSFormatF* സ്ഥിരം fromp = VN_CAST(nodep->fromp(), SFormatF)) {
                    എങ്കിൽ (fromp->name() == "%s.%s") {
                        newp = പുതിയ AstSFormatF{fl, "%s.%s." + nodep->name(), അസത്യം,
                                               fromp->exprsp()->clone_വൃക്ഷം_Pure(സത്യം)};
                    } അല്ലെങ്കിൽ {
                        newp = പുതിയ AstSFormatF{fl, fromp->name() + "." + nodep->name(), അസത്യം,
                                               nullptr};
                    }
                } അല്ലെങ്കിൽ {
                    newp = പുതിയ AstSFormatF{fl, nodep->name(), അസത്യം, nullptr};
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }
            // Not a class ref element, fall through to normal handling
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->user1()) {
            iterateChildren(nodep);
            nodep->replaceWith(nodep->fromp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ {
            editFormat(nodep);
        }
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {}
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {}
    ശൂന്യം visit(AstCStmt* nodep) override {}
    ശൂന്യം visit(AstIf* nodep) override {
        // Hoisted runtime-conditional disable_soft statements parked in the
        // constraint-items list by the pre-pass; already fully lowered.
    }
    ശൂന്യം visit(AstConstraintIf* nodep) override {
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thenp = editSingle(fl, nodep->thensp());
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elsep = editSingle(fl, nodep->elsesp());
        എങ്കിൽ (thenp && elsep) {
            newp = പുതിയ AstCond{fl, nodep->condp()->unlinkFrBack(), thenp, elsep};
        } അല്ലെങ്കിൽ എങ്കിൽ (thenp) {
            newp = പുതിയ AstLogIf{fl, nodep->condp()->unlinkFrBack(), thenp};
        } അല്ലെങ്കിൽ എങ്കിൽ (elsep) {
            newp = പുതിയ AstLogIf{fl, പുതിയ AstNot{fl, nodep->condp()->unlinkFrBack()}, elsep};
        }
        എങ്കിൽ (newp) {
            newp->dtypeSetBit();  // Result is boolean (prevents bare-var != 0 wrapping)
            newp->user1(സത്യം);  // Assume result-dependent
            nodep->replaceWith(പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, newp});
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        // Dubious but this is what we used to do. Does that mean no randomzie
        // methods work under a generage block?
    }
    ശൂന്യം visit(AstBegin* nodep) override {}
    ശൂന്യം visit(AstConstraintForeach* nodep) override {
        // Convert to plain foreach
        File_വരി* സ്ഥിരം fl = nodep->fileline();

        എങ്കിൽ (!nodep->bodyp()) {
            nodep->unlinkFrBack();
        } അല്ലെങ്കിൽ എങ്കിൽ (m_wantSingle) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം itemp = editSingle(fl, nodep->bodyp());
            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(itemp);
            cstmtp->add(";");
            Ast_കണ്ണി* സ്ഥിരം bodyp = prependDistPreamble(nodep, cstmtp);
            AstCExpr* സ്ഥിരം cexprp = പുതിയ AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(പുതിയ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     പുതിയ AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp},
                                     സത്യം});
            cexprp->add("return ret.empty() ? \"#b1\" : \"(bvand\" + ret + \")\";\n})()");
            nodep->replaceWith(പുതിയ AstSFormatF{fl, "%s", അസത്യം, cexprp});
        } അല്ലെങ്കിൽ {
            VL_RESTORER(m_foreachp);
            VL_RESTORER_COPY(m_foreachIdxVars);
            m_foreachp = nodep;
            വേണ്ടി (Ast_കണ്ണി* elemp = nodep->headerp()->elementsp(); elemp; elemp = elemp->nextp()) {
                എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(elemp, Var)) m_foreachIdxVars.തിരുകുക(varp);
            }
            iterateAndNextNull(nodep->bodyp());
            Ast_കണ്ണി* സ്ഥിരം bodyp
                = prependDistPreamble(nodep, nodep->bodyp()->unlinkFrBackWithNext());
            nodep->replaceWith(
                പുതിയ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                             പുതിയ AstForeach{fl, nodep->headerp()->unlinkFrBack(), bodyp}, സത്യം});
        }
        UASSERT_OBJ(!nodep->user3p(), nodep, "Dist bucket preamble not injected into foreach");
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstConstraintBefore* nodep) override {
        // Generate solveBefore() calls for each (lhs, rhs) variable pair.
        // Do NOT iterate children -- these are variable references, not constraint expressions.
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_Module* സ്ഥിരം genModp = VN_AS(m_genp->user2p(), കണ്ണി_Module);

        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nodep->lhssp(); lhsp; lhsp = VN_CAST(lhsp->nextp(), കണ്ണി_വ്യഞ്ജകം)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsTestp = build_Solve_Before_Name_വ്യഞ്ജകം(fl, lhsp);
            എങ്കിൽ (!lhsTestp) {
                lhsp->v3fatalSrc("Unexpected expression type in solve...before lhs");
                തുടരുക;
            }
            VL_DO_DANGLING(lhsTestp->delete_വൃക്ഷം(), lhsTestp);
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = nodep->rhssp(); rhsp;
                 rhsp = VN_CAST(rhsp->nextp(), കണ്ണി_വ്യഞ്ജകം)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsNamep = build_Solve_Before_Name_വ്യഞ്ജകം(fl, rhsp);
                എങ്കിൽ (!rhsNamep) {
                    rhsp->v3fatalSrc("Unexpected expression type in solve...before rhs");
                    തുടരുക;
                }
                AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
                    fl, പുതിയ AstVarRef{fl, genModp, m_genp, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_SOLVE_BEFORE};
                callp->dtypeSetVoid();
                callp->addPinsp(build_Solve_Before_Name_വ്യഞ്ജകം(fl, lhsp));
                callp->addPinsp(rhsNamep);
                nodep->addHereThisAsNext(callp->makeStmt());
            }
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstConstraintUnique* nodep) override {
        Ast_കണ്ണി_FTask* initTaskp;
        AstVar* genVarp = m_genp;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_Module* genModp = nullptr;

        എങ്കിൽ (m_classp) {
            initTaskp = VN_AS(m_memberMap.findMember(m_classp, "new"), കണ്ണി_FTask);
            genVarp = getRandomGenerator(m_classp);
        } അല്ലെങ്കിൽ {
            initTaskp = m_inlineInitTaskp;
        }
        UASSERT_OBJ(initTaskp, nodep, "No init Task for unique constraint");

        // UASSERT_OBJ(genVarp, nodep, "No generator variable");
        എങ്കിൽ (!genVarp) {
            // This shall be substituted with an assert when it will be supported
            nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint in randomize() with {}");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        എങ്കിൽ (m_classp) {
            genModp = VN_AS(genVarp->user2p(), കണ്ണി_Module);
            UASSERT_OBJ(genModp, nodep, "genVarp inside m_classp has no NodeModule set");
        }

        // Registration calls emitted where the unique statement stood, so they end up
        // in the constraint setup task and re-run on every randomize()
        Ast_കണ്ണി* setupStmtsp = nullptr;
        // Under a condition the distinctness must instead become part of the guarded
        // constraint expression, which needs an element count known at verilation time
        std::vector<std::string> smtExprs;
        വേണ്ടി (Ast_കണ്ണി* itemp = nodep->rangesp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (AstVarRef* സ്ഥിരം varRefp = VN_CAST(itemp, VarRef)) {
                AstVar* സ്ഥിരം varp = varRefp->varp();
                Ast_കണ്ണി_Module* സ്ഥിരം varModp = VN_AS(varp->user2p(), കണ്ണി_Module);
                UASSERT_OBJ(varModp, nodep, "varp has no NodeModule set");
                Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep()->skipRefp();

                സ്ഥിരം സ്ഥിതം auto dynDTypeSupported
                    = [](സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep) -> തർക്കം {
                    തിരികെ VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)
                           || VN_IS(dtypep, AssocArrayDType);
                };

                // Establish it is an array before asking for its element type
                uint64_t elemWidth;
                uint32_t staticSize = 0;  // Element count, zero when only known at run time
                എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം up = VN_CAST(dtypep, UnpackArrayDType)) {
                    സ്ഥിരം AstRange* സ്ഥിരം rangep = up->rangep();
                    UASSERT_OBJ(rangep && VN_IS(rangep->leftp(), Const)
                                    && VN_IS(rangep->rightp(), Const),
                                nodep, "Unpack array does not have a constant range");
                    staticSize = up->elementsConst();
                    എങ്കിൽ (staticSize > 100) {
                        nodep->v3warn(
                            CONSTRAINTIGN,
                            "Unsupported: Unique constraint on static arrays of size > 100");
                        തുടരുക;
                    }
                    elemWidth = static_cast<uint64_t>(varp->dtypep()->width());
                } അല്ലെങ്കിൽ എങ്കിൽ (dynDTypeSupported(dtypep)) {
                    elemWidth = static_cast<uint64_t>(dtypep->subDTypep()->width());
                } അല്ലെങ്കിൽ {
                    nodep->v3warn(CONSTRAINTIGN, "Unsupported: Unique constraint on "
                                                     << dtypep->prettyDTypeName(അസത്യം));
                    തുടരുക;
                }

                // Ensure it is ONLY 1-D by checking that the element type is not an array/queue
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം subp = dtypep->subDTypep()->skipRefp();
                എങ്കിൽ (VN_IS(subp, കണ്ണി_Array_DType) || VN_IS(subp, QueueDType)
                    || VN_IS(subp, DynArrayDType) || VN_IS(subp, AssocArrayDType)
                    || VN_IS(subp, WildcardArrayDType)) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on multidimensional arrays");
                    തുടരുക;
                }

                എങ്കിൽ (m_wantSingle && !staticSize) {
                    nodep->v3warn(CONSTRAINTIGN,
                                  "Unsupported: Unique constraint on dynamically sized array "
                                  "inside conditional constraint");
                    തുടരുക;
                }

                എങ്കിൽ (!varp->user3()) {
                    varp->user3(സത്യം);  // Solver owns it; __VBasicRand must not overwrite
                    // Convert to C string
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം varnamep = പുതിയ AstCExpr{
                        fl, AstCExpr::Pure{}, "\"" + varp->name() + "\"", varp->width()};

                    AstCMethodHard* സ്ഥിരം writeVarCallp = പുതിയ AstCMethodHard{
                        fl, പുതിയ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                        VCMethod::RANDOMIZER_WRITE_VAR};
                    AstVarRef* സ്ഥിരം argVarRefp = പുതിയ AstVarRef{fl, varp, VAccess::READ};
                    എങ്കിൽ (m_classp) argVarRefp->classOrPackagep(varModp);
                    writeVarCallp->addPinsp(argVarRefp);
                    writeVarCallp->addPinsp(പുതിയ AstConst{fl, AstConst::Unsized64{}, elemWidth});
                    writeVarCallp->addPinsp(varnamep);
                    writeVarCallp->addPinsp(പുതിയ AstConst{fl, 1});  // Dimension
                    സ്ഥിരം RandomizeMode randMode = {.asUQuad = varp->user1()};
                    എങ്കിൽ (randMode.usesMode) {
                        writeVarCallp->addPinsp(
                            പുതിയ AstConst{fl, AstConst::Unsized64{}, randMode.index});
                    }
                    writeVarCallp->dtypeSetVoid();
                    initTaskp->addStmtsp(പുതിയ Ast_Stmt_വ്യഞ്ജകം{fl, writeVarCallp});
                }

                // A solver-sized container is resized between solve passes; joining the
                // size-constrained set gets its element table refreshed in between
                എങ്കിൽ (varp->user4p() && m_sizeConstrainedArraysp) {
                    m_sizeConstrainedArraysp->തിരുകുക(varp);
                }

                എങ്കിൽ (m_wantSingle) {
                    // Fewer than two elements are trivially distinct
                    എങ്കിൽ (staticSize < 2) തുടരുക;
                    std::string വ്യഞ്ജകം_Str = "(__Vbv (distinct";
                    വേണ്ടി (uint32_t i = 0; i < staticSize; ++i) {
                        വ്യഞ്ജകം_Str += " (select " + varp->name() + " #x";
                        വേണ്ടി (പൂർണ്ണസംഖ്യ shift = 28; shift >= 0; shift -= 4) {
                            വ്യഞ്ജകം_Str += "0123456789abcdef"[(i >> shift) & 0xf];
                        }
                        വ്യഞ്ജകം_Str += ')';
                    }
                    smtExprs.emplace_back(വ്യഞ്ജകം_Str + "))");
                    തുടരുക;
                }

                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം namep = പുതിയ AstConst{fl, AstConst::String{}, varp->name()};
                AstCMethodHard* സ്ഥിരം randUniqueCallp
                    = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, genModp, genVarp, VAccess::READ},
                                         VCMethod::RANDOMIZER_UNIQUE, namep};
                randUniqueCallp->dtypep(nodep->findVoidDType());
                randUniqueCallp->addPinsp(പുതിയ AstConst{fl, *m_uniqueConstraintId});
                setupStmtsp = Ast_കണ്ണി::addNext(setupStmtsp, പുതിയ Ast_Stmt_വ്യഞ്ജകം{fl, randUniqueCallp});
            }
        }
        ++(*m_uniqueConstraintId);
        എങ്കിൽ (m_wantSingle && !smtExprs.empty()) {
            std::string വ്യഞ്ജകം_Str = smtExprs.front();
            എങ്കിൽ (smtExprs.size() > 1) {
                വ്യഞ്ജകം_Str = "(bvand";
                വേണ്ടി (സ്ഥിരം std::string& oneExprp : smtExprs) വ്യഞ്ജകം_Str += " " + oneExprp;
                വ്യഞ്ജകം_Str += ')';
            }
            AstSFormatF* സ്ഥിരം newp = പുതിയ AstSFormatF{fl, വ്യഞ്ജകം_Str, അസത്യം, nullptr};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        എങ്കിൽ (setupStmtsp) nodep->addHereThisAsNext(setupStmtsp);
        nodep->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(Ast_Constraint_വ്യഞ്ജകം* nodep) override {
        // IEEE 1800-2023 18.5.13: 'disable soft' is a meta-level directive on
        // the constraint graph, lowered to a void RANDOMIZER_DISABLE_SOFT
        // call.  Conditional occurrences (if / foreach / implication body)
        // are rewritten by extractConditionalDisableSofts() before this
        // visitor runs, so anything we see here is unconditional.
        VL_RESTORER(m_firstExpressionInsideIndexp);
        എങ്കിൽ (nodep->isDisableSoft()) {
            nodep->replaceWith(buildDisableSoftCallStmt(nodep));
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        // IEEE 1800-2023 18.5.1: A bare expression used as a constraint is
        // implicitly treated as "expr != 0" when wider than 1 bit.
        // Must wrap before iterateChildren, which converts to SMT format.
        {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->exprp();
            എങ്കിൽ (exprp->width() > 1) {
                File_വരി* സ്ഥിരം fl = exprp->fileline();
                സ്ഥിരം V3_സംഖ്യ അക്കം_Zero{fl, exprp->width(), 0};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം neqp
                    = പുതിയ AstNeq{fl, exprp->unlinkFrBack(), പുതിയ AstConst{fl, അക്കം_Zero}};
                neqp->user1(സത്യം);  // Mark as rand-dependent for SMT path
                nodep->exprp(neqp);
            }
        }
        iterateChildren(nodep);
        എങ്കിൽ (m_wantSingle) {
            nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        // Emit as soft or hard constraint per IEEE 1800-2023 18.5.13
        സ്ഥിരം VCMethod method
            = nodep->isSoft() ? VCMethod::RANDOMIZER_SOFT : VCMethod::RANDOMIZER_HARD;
        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
            nodep->fileline(),
            പുതിയ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), കണ്ണി_Module), m_genp,
                          VAccess::READWRITE},
            method, nodep->exprp()->unlinkFrBack()};
        callp->dtypeSetVoid();
        // Pass filename, lineno, and source as separate arguments
        // This allows EmitC to call protect() on filename, similar to VL_STOP
        // Add filename parameter
        callp->addPinsp(പുതിയ AstCExpr{nodep->fileline(), AstCExpr::Pure{},
                                     "\"" + nodep->fileline()->filename() + "\""});
        // Add line number parameter
        സ്ഥിരം uint32_t lineno = static_cast<uint32_t>(nodep->fileline()->lineno());
        callp->addPinsp(പുതിയ AstConst{nodep->fileline(), lineno});
        // Add source text parameter (empty if --protect-ids to avoid source leakage)
        std::string prettyText;
        എങ്കിൽ (!v3Global.opt.protectIds()) {
            prettyText = nodep->fileline()->prettySource();
            size_t pos = 0;
            ആകുമ്പോൾ ((pos = prettyText.find('"', pos)) != std::string::npos) {
                prettyText.തിരുകുക(pos, "\\");
                pos += std::strlen("\\\"");
            }
        }
        callp->addPinsp(
            പുതിയ AstCExpr{nodep->fileline(), AstCExpr::Pure{}, "\"" + prettyText + "\""});
        nodep->replaceWith(callp->makeStmt());
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        File_വരി* സ്ഥിരം fl = nodep->fileline();

        എങ്കിൽ (nodep->method() == VCMethod::ARRAY_AT) {
            addStringNamePart(nodep->pinsp(), "%x");
            സ്ഥിരം തർക്കം indexIsRand = nodep->pinsp()->exists([](സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം vrefp) {
                തിരികെ vrefp->varp()->rand().isRandomizable();
            });
            എങ്കിൽ (nodep->fromp()->user1()) {
                // Queue/dynamic element: pre-edit clone for the rand_mode hoist, non-rand index
                // only. A std::randomize() with-clause argument carries no rand qualifier of its
                // own but is still part of the solve.
                {
                    സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം basep = nodep->baseFromp(സത്യം);
                    എങ്കിൽ (basep->name() == "__Vthis"
                        && VN_AS(basep->backp(), MemberSel)->varp()->isRand()) {
                        nodep->fromp()->v3warn(E_UNSUPPORTED,
                                               "Unsupported: Complex expression captured from "
                                               "current scope with randomized variable");
                        തിരികെ;
                    }
                }
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origp = indexIsRand ? nullptr : nodep->clone_വൃക്ഷം(അസത്യം);
                സ്ഥിരം AstVarRef* സ്ഥിരം baseRefp
                    = m_structSel ? VN_CAST(nodep->fromp()->baseFromp(സത്യം), VarRef) : nullptr;
                എങ്കിൽ (baseRefp && VN_IS(nodep->pinsp(), Const)
                    && VN_IS(baseRefp->varp()->dtypep()->skipRefp(), DynArrayDType)
                    && m_prepareConstrainedArraysp) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം indexp = nodep->pinsp()->clone_വൃക്ഷം_Pure(അസത്യം);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം arrayp = nodep->fromp()->clone_വൃക്ഷം_Pure(അസത്യം);
                    AstCMethodHard* സ്ഥിരം currentSizep
                        = പുതിയ AstCMethodHard{fl, arrayp->clone_വൃക്ഷം_Pure(അസത്യം), VCMethod::DYN_SIZE};
                    currentSizep->dtypeSetInt();
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം inRangeRequiredp = പുതിയ AstLogAnd{
                        fl,
                        പുതിയ AstLteS{fl,
                                    പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, indexp->width(), 0},
                                    indexp->clone_വൃക്ഷം_Pure(അസത്യം)},
                        പുതിയ AstGteS{fl, indexp->clone_വൃക്ഷം_Pure(അസത്യം), currentSizep}};
                    AstCMethodHard* സ്ഥിരം resizep
                        = പുതിയ AstCMethodHard{fl, arrayp, VCMethod::DYN_RESIZE,
                                             പുതിയ AstAdd{fl, indexp, പുതിയ AstConst{fl, 1}}};
                    resizep->dtypeSetVoid();
                    m_prepareConstrainedArraysp->addStmtsp(
                        പുതിയ AstIf{fl, inRangeRequiredp, resizep->makeStmt()});
                }
                AstCMethodHard* സ്ഥിരം sizep
                    = m_structSel ? പുതിയ AstCMethodHard{fl, nodep->fromp()->clone_വൃക്ഷം_Pure(അസത്യം),
                                                       VCMethod::DYN_SIZE}
                                  : nullptr;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം originalPinp = nodep->pinsp();
                iterateChildren(nodep);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinp = nodep->pinsp()->unlinkFrBack();
                AstSFormatF* newp;
                എങ്കിൽ (m_structSel) {
                    എങ്കിൽ (AstSFormatF* സ്ഥിരം sformatfp = VN_CAST(pinp, SFormatF)) {
                        sformatfp->name("%x");
                    }
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argsp
                        = Ast_കണ്ണി::addNext(nodep->fromp()->unlinkFrBack(), pinp);
                    sizep->dtypeSetInt();
                    AstLogAnd* സ്ഥിരം condp
                        = പുതിയ AstLogAnd{fl,
                                        പുതിയ AstLteS{fl,
                                                    പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{},
                                                                 originalPinp->width(), 0},
                                                    originalPinp->clone_വൃക്ഷം_Pure(അസത്യം)},
                                        പുതിയ AstLtS{fl, originalPinp->clone_വൃക്ഷം_Pure(അസത്യം), sizep}};
                    m_conditionp = m_conditionp ? പുതിയ AstLogAnd{fl, m_conditionp, condp} : condp;
                    newp = പുതിയ AstSFormatF{fl, "%s.%s", അസത്യം, argsp};
                } അല്ലെങ്കിൽ {
                    newp = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), pinp);
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                എങ്കിൽ (origp && !hoistRandModeOverSelectAndMember(newp, origp)) {
                    VL_DO_DANGLING(origp->delete_വൃക്ഷം(), origp);
                }

            } അല്ലെങ്കിൽ എങ്കിൽ (indexIsRand) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Randomization of an index to a non-random variable");

            } അല്ലെങ്കിൽ {
                nodep->user1(അസത്യം);
                UASSERT_OBJ(editFormat(nodep), nodep,
                            "editFormat should return true when user1 is false");
            }
            തിരികെ;
        }

        എങ്കിൽ (nodep->method() == VCMethod::ARRAY_INSIDE) {
            സ്ഥിരം തർക്കം randArr = nodep->fromp()->user1();

            AstVar* സ്ഥിരം newVarp
                = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, "__Vinside", nodep->findIntDType()};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം idxRefp = പുതിയ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            AstForeachHeader* സ്ഥിരം headerp
                = പുതിയ AstForeachHeader{fl, nodep->fromp()->clone_വൃക്ഷം_Pure(അസത്യം), newVarp};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം selp = newSel(nodep->fileline(), nodep->fromp(), idxRefp);
            selp->user1(randArr);
            Ast_കണ്ണി* സ്ഥിരം itemp = പുതിയ AstEq{fl, selp, nodep->pinsp()->unlinkFrBack()};
            itemp->user1(സത്യം);

            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{fl};
            cstmtp->add("ret += \" \";\n");
            cstmtp->add("ret += ");
            cstmtp->add(iterateSubtreeReturnEdits(itemp));
            cstmtp->add(";");
            AstCExpr* സ്ഥിരം cexprp = പുതിയ AstCExpr{fl};
            cexprp->dtypeSetString();
            cexprp->add("([&]{\nstd::string ret;\n");
            cexprp->add(പുതിയ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                     പുതിയ AstForeach{fl, headerp, cstmtp}, സത്യം});
            cexprp->add("return ret.empty() ? \"#b0\" : \"(bvor\" + ret + \")\";\n})()");
            nodep->replaceWith(പുതിയ AstSFormatF{fl, "%s", അസത്യം, cexprp});
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }

        // Array reduction methods (sum, product, and, or, xor)
        എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_SUM
            || nodep->method() == VCMethod::ARRAY_R_PRODUCT
            || nodep->method() == VCMethod::ARRAY_R_AND || nodep->method() == VCMethod::ARRAY_R_OR
            || nodep->method() == VCMethod::ARRAY_R_XOR) {

            AstWith* സ്ഥിരം withp = nodep->withp();

            // For 'with' clause: check array size limit and item.index support
            എങ്കിൽ (withp) {
                എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType)) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ arraySize = adtypep->elementsConst();
                    എങ്കിൽ (arraySize > v3Global.opt.constraintArrayLimit()) {
                        nodep->v3warn(CONSTRAINTIGN,
                                      "Constraint array reduction ignored (array size "
                                          + cvtToStr(arraySize)
                                          + " exceeds --constraint-array-limit of "
                                          + cvtToStr(v3Global.opt.constraintArrayLimit())
                                          + "), treating as state");
                        nodep->user1(അസത്യം);
                        എങ്കിൽ (editFormat(nodep)) തിരികെ;
                        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                        തിരികെ;
                    }
                }
            }

            // For without 'with' clause: need random array for variable registration
            സ്ഥിരം തർക്കം hasRandomArray = nodep->fromp()->user1();
            എങ്കിൽ (!withp && !hasRandomArray) {
                // No 'with' clause and not a random array - can't handle
                nodep->v3warn(CONSTRAINTIGN,
                              "Unsupported: randomizing this expression, treating as state");
                nodep->user1(അസത്യം);
                എങ്കിൽ (editFormat(nodep)) തിരികെ;
                nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
                തിരികെ;
            }
            എങ്കിൽ (!VN_IS(nodep->fromp(), VarRef)) {
                // Non-dynamic subarrays and member arrays are handled in other parts of code
                nodep->v3warn(CONSTRAINTIGN, "Unsupported: Array reduction constraint on dynamic "
                                             "array inside array or struct");
                തിരികെ;
            }

            // Create loop variable and header
            AstVar* സ്ഥിരം loopVarp
                = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, "__Vreduce", nodep->findIntDType()};
            AstForeachHeader* സ്ഥിരം headerp
                = പുതിയ AstForeachHeader{fl, nodep->fromp()->clone_വൃക്ഷം_Pure(അസത്യം), loopVarp};

            // Determine SMT operation and compute identity elements
            സ്ഥിരം അക്ഷരം* smtOp = nullptr;
            std::string identity;
            എങ്കിൽ (withp) {
                // For 'with' clause: use binary format, compute from result width
                സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->dtypep()->width();
                എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = "#b" + std::string(width, '0');
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = (width > 0) ? "#b" + std::string(width - 1, '0') + "1" : "#b0";
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = "#b" + std::string(width, '1');
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = "#b" + std::string(width, '0');
                } അല്ലെങ്കിൽ {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = "#b" + std::string(width, '0');
                }
            } അല്ലെങ്കിൽ {
                // For without 'with' clause: use hex format, compute from element width
                AstVarRef* സ്ഥിരം arrRefp = VN_CAST(nodep->fromp(), VarRef);
                UASSERT_OBJ(arrRefp, nodep, "Array reduction in constraint has non-VarRef source");
                AstVar* സ്ഥിരം arrVarp = arrRefp->varp();
                Ast_കണ്ണി_DType* elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                സ്ഥിരം പൂർണ്ണസംഖ്യ elemWidth = elemDtp->width();
                സ്ഥിരം പൂർണ്ണസംഖ്യ hexDigits = (elemWidth + 3) / 4;

                സ്ഥിരം std::string zeroIdentity = "#x" + std::string(hexDigits, '0');
                സ്ഥിരം std::string oneIdentity = "#x" + std::string(hexDigits - 1, '0') + "1";
                സ്ഥിരം std::string allOnesIdentity = "#x" + std::string(hexDigits, 'f');

                എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_SUM) {
                    smtOp = "bvadd";
                    identity = zeroIdentity;
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_PRODUCT) {
                    smtOp = "bvmul";
                    identity = oneIdentity;
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_AND) {
                    smtOp = "bvand";
                    identity = allOnesIdentity;
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->method() == VCMethod::ARRAY_R_OR) {
                    smtOp = "bvor";
                    identity = zeroIdentity;
                } അല്ലെങ്കിൽ {  // ARRAY_R_XOR
                    smtOp = "bvxor";
                    identity = zeroIdentity;
                }
            }

            // Build loop body based on whether we have a 'with' clause
            AstCStmt* സ്ഥിരം cstmtp = പുതിയ AstCStmt{fl};
            എങ്കിൽ (withp) {
                // With 'with' clause: evaluate expression for each element
                സ്ഥിരം തർക്കം randArr = nodep->fromp()->user1();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം idxRefp = പുതിയ AstVarRef{fl, loopVarp, VAccess::READ};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elemSelp = newSel(fl, nodep->fromp(), idxRefp);
                elemSelp->user1(randArr);

                // Get the result width for the reduction
                സ്ഥിരം പൂർണ്ണസംഖ്യ resultWidth = nodep->dtypep()->width();
                സ്ഥിരം VSigning resultSigning = nodep->dtypep()->numeric();

                Ast_കണ്ണി* perElemExprp = withp->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
                എങ്കിൽ (AstLambdaArgRef* സ്ഥിരം വേര്_Refp = VN_CAST(perElemExprp, LambdaArgRef)) {
                    എങ്കിൽ (വേര്_Refp->index()) {
                        // item.index at root -> replace with loop variable, adjust width
                        AstVarRef* സ്ഥിരം loopRef = പുതിയ AstVarRef{fl, loopVarp, VAccess::READ};
                        perElemExprp = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                    } അല്ലെങ്കിൽ {
                        // item at root -> replace with array element
                        perElemExprp = elemSelp->clone_വൃക്ഷം_Pure(അസത്യം);
                    }
                    VL_DO_DANGLING(വേര്_Refp->delete_വൃക്ഷം(), വേര്_Refp);
                } അല്ലെങ്കിൽ {
                    perElemExprp->foreach([&](AstLambdaArgRef* refp) {
                        എങ്കിൽ (refp->index()) {
                            // item.index -> replace with loop variable, adjust width
                            AstVarRef* സ്ഥിരം loopRef = പുതിയ AstVarRef{fl, loopVarp, VAccess::READ};
                            Ast_കണ്ണി* സ്ഥിരം replacement
                                = adjustWidth(fl, loopRef, resultWidth, resultSigning);
                            refp->replaceWith(replacement);
                        } അല്ലെങ്കിൽ {
                            // item -> replace with array element
                            refp->replaceWith(elemSelp->clone_വൃക്ഷം_Pure(അസത്യം));
                        }
                        VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
                    });
                }
                VL_DO_DANGLING(elemSelp->delete_വൃക്ഷം(), elemSelp);

                // enum-literal folding pass already ran -- re-fold the literals.
                perElemExprp = V3Const::constifyEdit(perElemExprp);

                perElemExprp->foreach([&](Ast_കണ്ണി* nodep) {
                    // Don't mark loop variable references as randomizable
                    എങ്കിൽ (!VN_IS(nodep, Const)) {
                        എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം vrefp = VN_CAST(nodep, കണ്ണി_Var_Ref)) {
                            എങ്കിൽ (vrefp->varp() != loopVarp) nodep->user1(സത്യം);
                        } അല്ലെങ്കിൽ {
                            nodep->user1(സത്യം);
                        }
                    }
                });

                cstmtp->add("ret = \"(" + std::string(smtOp) + " \" + ret + \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(iterateSubtreeReturnEdits(perElemExprp));
                cstmtp->add(";\n");
                cstmtp->add("ret += \")\";\n");
            } അല്ലെങ്കിൽ {
                // Without 'with' clause: register each element as solver variable
                AstVarRef* സ്ഥിരം arrRefp = VN_CAST(nodep->fromp(), VarRef);
                AstVar* സ്ഥിരം arrVarp = arrRefp->varp();
                സ്ഥിരം std::string smtArrayName = arrVarp->name();
                Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = arrVarp->dtypep()->skipRefp()->subDTypep();
                സ്ഥിരം പൂർണ്ണസംഖ്യ elemWidth = elemDtp->width();
                Ast_കണ്ണി_Module* സ്ഥിരം arrModulep = VN_AS(arrVarp->user2p(), കണ്ണി_Module);
                Ast_കണ്ണി_Module* സ്ഥിരം genModulep = VN_AS(m_genp->user2p(), കണ്ണി_Module);
                arrVarp->user3(സത്യം);

                // Add write_var call to init task
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം arrVarNamep
                    = പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "\"" + smtArrayName + "\"", elemWidth};
                AstCMethodHard* സ്ഥിരം writeVarCallp
                    = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, genModulep, m_genp, VAccess::READ},
                                         VCMethod::RANDOMIZER_WRITE_VAR};
                writeVarCallp->addPinsp(പുതിയ AstVarRef{fl, arrModulep, arrVarp, VAccess::READ});
                writeVarCallp->addPinsp(
                    പുതിയ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(elemWidth)});
                writeVarCallp->addPinsp(arrVarNamep);
                സ്ഥിരം uint32_t unpackedDims = arrVarp->dtypep()->dimensions(അസത്യം).second;
                UASSERT_OBJ(unpackedDims == 1, arrVarp, "Array isn't 1-D");
                writeVarCallp->addPinsp(പുതിയ AstConst{fl, 1});  // Dimension

                സ്ഥിരം RandomizeMode randMode = {.asUQuad = arrVarp->user1()};
                എങ്കിൽ (randMode.usesMode) {
                    writeVarCallp->addPinsp(
                        പുതിയ AstConst{fl, AstConst::Unsized64{}, randMode.index});
                }
                writeVarCallp->dtypeSetVoid();
                Ast_കണ്ണി_FTask* initTaskp = m_inlineInitTaskp;
                എങ്കിൽ (!initTaskp) {
                    initTaskp = VN_AS(m_memberMap.findMember(arrModulep, "new"), കണ്ണി_FTask);
                    UASSERT_OBJ(initTaskp, arrModulep, "No new() in class");
                }
                initTaskp->addStmtsp(writeVarCallp->makeStmt());

                // Create solver constraints
                AstSFormatF* സ്ഥിരം idxFormatp = പുതിയ AstSFormatF{
                    fl, "#x%08x", അസത്യം, പുതിയ AstVarRef{fl, loopVarp, VAccess::READ}};
                iterateChildren(nodep);
                AstSFormatF* സ്ഥിരം varNamep
                    = createSolverArrDerefp(fl, nodep->fromp()->unlinkFrBack(), idxFormatp);

                cstmtp->add("ret += \" \";\n");
                cstmtp->add("ret += ");
                cstmtp->add(varNamep);
                cstmtp->add(";\n");
            }

            // Build final expression
            എങ്കിൽ (withp) {
                // For 'with' clause: use lambda expression with AstCExpr
                AstCExpr* സ്ഥിരം cexprp = പുതിയ AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret = \"" + identity + "\";\n");
                cexprp->add(പുതിയ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         പുതിയ AstForeach{fl, headerp, cstmtp}, സത്യം});
                cexprp->add("return ret;\n})()");
                nodep->replaceWith(പുതിയ AstSFormatF{fl, "%s", അസത്യം, cexprp});
            } അല്ലെങ്കിൽ {
                // For without 'with' clause: use AstCExpr with conditional
                AstCExpr* സ്ഥിരം cexprp = പുതിയ AstCExpr{fl};
                cexprp->dtypeSetString();
                cexprp->add("([&]{\nstd::string ret;\n");
                cexprp->add(പുതിയ AstBegin{fl, m_uniqueNames.get("__VrandBlock"),
                                         പുതിയ AstForeach{fl, headerp, cstmtp}, സത്യം});
                cexprp->add("return ret.empty() ? \"" + identity + "\" : \"(" + smtOp
                            + "\" + ret + \")\";\n})()");
                nodep->replaceWith(പുതിയ AstSFormatF{fl, "%s", അസത്യം, cexprp});
            }
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }

        // We've handled DYN_SIZE already if they should be randomized
        എങ്കിൽ (nodep->method() == VCMethod::DYN_SIZE || nodep->method() == VCMethod::ASSOC_SIZE) {
            Ast_കണ്ണി_വ്യഞ്ജകം* കണ്ണി_Exprp = nodep->fromp();
            // Check if it is a pure chain of MemberSel ended with VarRef
            // if not throw warning
            ആകുമ്പോൾ (VN_IS(കണ്ണി_Exprp, MemberSel)) {
                കണ്ണി_Exprp = VN_AS(കണ്ണി_Exprp, MemberSel)->fromp();
            }
            എങ്കിൽ (VN_IS(കണ്ണി_Exprp, VarRef)) {
                nodep->replaceWith(getConstFormat(nodep->clone_വൃക്ഷം_Pure(അസത്യം)));
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                തിരികെ;
            }
        }

        nodep->v3warn(CONSTRAINTIGN,
                      "Unsupported: randomizing this expression, treating as state");
        nodep->user1(അസത്യം);

        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        nodep->v3fatalSrc("Method not handled in constraints? " << nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;

        AstFunc* സ്ഥിരം funcp = VN_CAST(nodep->taskp(), Func);
        എങ്കിൽ (!funcp) {
            // Tasks have no return value and can't appear in expressions.
            // Parser rejects this, so reaching here indicates a compiler bug.
            nodep->v3fatalSrc("Unexpected task call in constraint expression");
            തിരികെ;
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം retExprp = extract_Return_വ്യഞ്ജകം(funcp);
        എങ്കിൽ (!retExprp) {
            nodep->v3warn(CONSTRAINTIGN,
                          "Unsupported: complex function in constraint, treating as state");
            nodep->user1(അസത്യം);
            എങ്കിൽ (editFormat(nodep)) തിരികെ;
            തിരികെ;
        }

        // Map formal parameters to actual arguments using V3Task infrastructure
        സ്ഥിരം V3TaskConnects tconnects
            = V3Task::taskConnects(nodep, funcp->stmtsp(), nullptr, അസത്യം);

        // Clone return expression, substitute params with args.
        // Track the root via a local pointer: when the root itself is a VarRef to a port
        // (e.g. body `F = d;`), the cloned root has no back pointer, so foreach's
        // replaceWith would trip the "no back" assertion -- substitute the root pointer
        // directly instead.
        Ast_കണ്ണി_വ്യഞ്ജകം* inlinedp = retExprp->clone_വൃക്ഷം_Pure(അസത്യം);
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            സ്ഥിരം AstVar* സ്ഥിരം portp = tconnect.first;
            AstArg* സ്ഥിരം argp = tconnect.second;
            എങ്കിൽ (!argp || !argp->exprp()) തുടരുക;
            എങ്കിൽ (AstVarRef* സ്ഥിരം വേര്_Refp = VN_CAST(inlinedp, VarRef)) {
                എങ്കിൽ (വേര്_Refp->varp() == portp) {
                    inlinedp = argp->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
                    VL_DO_DANGLING(വേര്_Refp->delete_വൃക്ഷം(), വേര്_Refp);
                    തുടരുക;
                }
            }
            inlinedp->foreach([&](AstVarRef* refp) {
                എങ്കിൽ (refp->varp() == portp) {
                    refp->replaceWith(argp->exprp()->clone_വൃക്ഷം_Pure(അസത്യം));
                    VL_DO_DANGLING(refp->delete_വൃക്ഷം(), refp);
                }
            });
        }

        inlinedp->dtypep(nodep->dtypep());
        propagateUser1InlineRecurse(inlinedp);
        nodep->replaceWith(inlinedp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        iterate(inlinedp);
    }
    // Skip non-constraint stmts appended to the iterating list during inline randomize-with
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {}
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (editFormat(nodep)) തിരികെ;
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for math node: " << nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        nodep->v3fatalSrc(
            "Visit function missing? Constraint function missing for node: " << nodep);
    }

    // Build the runtime statement that calls RANDOMIZER_DISABLE_SOFT on the
    // variable referenced by the given AstConstraintExpr.  Parser enforces
    // constraint_primary operand (verilog.y, IEEE 1800-2023 18.5.13).
    Ast_കണ്ണി* buildDisableSoftCallStmt(Ast_Constraint_വ്യഞ്ജകം* nodep) {
        std::string varName;
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം vrefp = VN_CAST(nodep->exprp(), കണ്ണി_Var_Ref)) {
            varName = vrefp->name();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം mselp = VN_CAST(nodep->exprp(), MemberSel)) {
            varName = mselp->name();
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Unexpected expression type in disable soft");
        }
        AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
            nodep->fileline(),
            പുതിയ AstVarRef{nodep->fileline(), VN_AS(m_genp->user2p(), കണ്ണി_Module), m_genp,
                          VAccess::READWRITE},
            VCMethod::RANDOMIZER_DISABLE_SOFT,
            പുതിയ AstConst{nodep->fileline(), AstConst::String{}, varName}};
        callp->dtypeSetVoid();
        തിരികെ callp->makeStmt();
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
    Ast_കണ്ണി* extractConditionalDisableSofts(Ast_കണ്ണി* itemsp, Ast_കണ്ണി_വ്യഞ്ജകം* outerCondp) {
        Ast_കണ്ണി* hoistListp = nullptr;
        വേണ്ടി (Ast_കണ്ണി *stmtp = itemsp, *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            എങ്കിൽ (AstConstraintIf* സ്ഥിരം ifp = VN_CAST(stmtp, ConstraintIf)) {
                File_വരി* സ്ഥിരം fl = ifp->fileline();
                // then branch: accumulated = outer && this.cond
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thenCondp
                    = outerCondp ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                       പുതിയ AstLogAnd{fl, outerCondp->clone_വൃക്ഷം_Pure(അസത്യം),
                                                     ifp->condp()->clone_വൃക്ഷം_Pure(അസത്യം)})
                                 : ifp->condp()->clone_വൃക്ഷം_Pure(അസത്യം);
                എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം thenHoistp
                    = extractConditionalDisableSofts(ifp->thensp(), thenCondp)) {
                    hoistListp
                        = hoistListp ? Ast_കണ്ണി::addNext(hoistListp, thenHoistp) : thenHoistp;
                }
                VL_DO_DANGLING(thenCondp->delete_വൃക്ഷം(), thenCondp);
                എങ്കിൽ (ifp->elsesp()) {
                    // else branch: accumulated = outer && !this.cond
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notInnerp
                        = പുതിയ AstLogNot{fl, ifp->condp()->clone_വൃക്ഷം_Pure(അസത്യം)};
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elseCondp
                        = outerCondp ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstLogAnd{
                                           fl, outerCondp->clone_വൃക്ഷം_Pure(അസത്യം), notInnerp})
                                     : notInnerp;
                    എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം elseHoistp
                        = extractConditionalDisableSofts(ifp->elsesp(), elseCondp)) {
                        hoistListp
                            = hoistListp ? Ast_കണ്ണി::addNext(hoistListp, elseHoistp) : elseHoistp;
                    }
                    VL_DO_DANGLING(elseCondp->delete_വൃക്ഷം(), elseCondp);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (Ast_Constraint_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_CAST(stmtp, Constraint_വ്യഞ്ജകം)) {
                എങ്കിൽ (exprp->isDisableSoft() && outerCondp) {
                    File_വരി* സ്ഥിരം fl = exprp->fileline();
                    AstIf* സ്ഥിരം hoistIfp = പുതിയ AstIf{fl, outerCondp->clone_വൃക്ഷം_Pure(അസത്യം),
                                                      buildDisableSoftCallStmt(exprp), nullptr};
                    hoistListp = hoistListp ? Ast_കണ്ണി::addNext(hoistListp,
                                                               static_cast<Ast_കണ്ണി*>(hoistIfp))
                                            : static_cast<Ast_കണ്ണി*>(hoistIfp);
                    // Replace in-tree disable-soft with a no-op boolean
                    // constraint so editSingle() folds cleanly.
                    Ast_Constraint_വ്യഞ്ജകം* സ്ഥിരം truep
                        = പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, പുതിയ AstConst{fl, AstConst::BitTrue{}}};
                    exprp->replaceWith(truep);
                    VL_DO_DANGLING(exprp->delete_വൃക്ഷം(), exprp);
                }
            }
            // foreach/unique bodies are left untouched: disable-soft inside
            // a foreach would need per-iteration condition support, and
            // unique cannot legally contain disable-soft.
        }
        തിരികെ hoistListp;
    }

പൊതു:
    // CONSTRUCTORS
    explicit Constraint_വ്യഞ്ജകം_Visitor(AstClass* classp, VMemberMap& memberMap, Ast_കണ്ണി* nodep,
                                   Ast_കണ്ണി_FTask* inlineInitTaskp, AstVar* genp,
                                   AstVar* randModeVarp, std::set<std::string>& writtenVars,
                                   uint32_t* uniqueConstraintId, V3UniqueNames& uniqueNames,
                                   Ast_കണ്ണി_FTask* memberselInitTaskp = nullptr,
                                   VInsertionSet<AstVar*>* sizeConstrainedArraysp = nullptr,
                                   Ast_കണ്ണി_FTask* prepareConstrainedArraysp = nullptr)
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
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം hoistListp
            = nodep ? extractConditionalDisableSofts(nodep, nullptr) : nullptr) {
            nodep->addNext(hoistListp);
        }
        iterateAndNextNull(nodep);
    }
};

എണ്ണൽ വർഗ്ഗം CaptureMode : uint8_t {
    CAP_NO = 0x0,
    CAP_മൂല്യം = 0x01,
    CAP_THIS = 0x02,
    CAP_F_SET_CLASSORPACKAGEP = 0x4,
    CAP_F_XREF = 0x8
};
CaptureMode operator|(CaptureMode a, CaptureMode b) {
    തിരികെ static_cast<CaptureMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
CaptureMode operator&(CaptureMode a, CaptureMode b) {
    തിരികെ static_cast<CaptureMode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
CaptureMode mode(CaptureMode a) { തിരികെ a & static_cast<CaptureMode>(0x3); }
തർക്കം hasFlags(CaptureMode a, CaptureMode flags) {
    തിരികെ ((static_cast<uint8_t>(a) & 0xc & static_cast<uint8_t>(flags))
            == static_cast<uint8_t>(flags));
}

വർഗ്ഗം CaptureVisitor final : പൊതു VNVisitor {
    AstArg* m_argsp = nullptr;  // Original references turned into arguments
    Ast_കണ്ണി_Module* m_callerp;  // Module of the outer context (for capturing `this`)
    AstClass* m_targetp;  // Module of inner context (for symbol lookup)
    std::map<സ്ഥിരം AstVar*, AstVar*> m_varCloneMap;  // Map original var nodes to their clones
    std::set<Ast_കണ്ണി*> m_ignore;  // Nodes to ignore for capturing
    AstVar* m_thisp = nullptr;  // Variable for outer context's object, if necessary
    തർക്കം m_staticContext = അസത്യം;  // True when capturing from a static function context

    // METHODS

    തർക്കം captureVariable(File_വരി* സ്ഥിരം fileline, Ast_കണ്ണി_Var_Ref* varrefp, AstVar*& varp) {
        auto it = m_varCloneMap.find(varrefp->varp());
        എങ്കിൽ (it == m_varCloneMap.end()) {
            AstVar* സ്ഥിരം newVarp = varrefp->varp()->clone_വൃക്ഷം(അസത്യം);
            newVarp->fileline(fileline);
            newVarp->varType(VVarType::BLOCKTEMP);
            newVarp->funcLocal(സത്യം);
            newVarp->direction(VDirection::നിവേശം);
            newVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_varCloneMap.emplace(varrefp->varp(), newVarp);
            varp = newVarp;
            തിരികെ സത്യം;
        }
        varp = it->second;
        തിരികെ അസത്യം;
    }

    template <typename T_കണ്ണി>
    ശൂന്യം fixupClassOrPackage(Ast_കണ്ണി* memberp, T_കണ്ണി refp) {
        Ast_കണ്ണി_Module* സ്ഥിരം declClassp = VN_AS(memberp->user2p(), കണ്ണി_Module);
        എങ്കിൽ (declClassp != m_targetp) refp->classOrPackagep(declClassp);
    }

    template <typename T_കണ്ണി>
    തർക്കം isReferenceToInnerMember(T_കണ്ണി nodep) {
        തിരികെ VN_IS(nodep->fromp(), LambdaArgRef);
    }

    AstVar* importThisp(File_വരി* fl) {
        എങ്കിൽ (!m_thisp) {
            AstClassRefDType* സ്ഥിരം refDTypep
                = പുതിയ AstClassRefDType{fl, VN_AS(m_callerp, Class), nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
            m_thisp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, "__Vthis", refDTypep};
            m_thisp->funcLocal(സത്യം);
            m_thisp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            m_thisp->direction(VDirection::നിവേശം);
            m_argsp = Ast_കണ്ണി::addNext(m_argsp, പുതിയ AstArg{fl, "", പുതിയ AstThisRef{fl, refDTypep}});
        }
        തിരികെ m_thisp;
    }

    AstVar* getVar(AstVar* സ്ഥിരം varp) സ്ഥിരം {
        സ്ഥിരം auto it = m_varCloneMap.find(varp);
        എങ്കിൽ (it == m_varCloneMap.end()) തിരികെ nullptr;
        എങ്കിൽ (it->second->isStdRandomizeArg()) തിരികെ nullptr;
        തിരികെ it->second;
    }

    CaptureMode getVarRefCaptureMode(Ast_കണ്ണി_Var_Ref* varRefp) {
        Ast_കണ്ണി_Module* സ്ഥിരം varModp = VN_AS(varRefp->varp()->user2p(), കണ്ണി_Module);
        AstClass* സ്ഥിരം varClassp = VN_CAST(varModp, Class);
        AstClass* സ്ഥിരം callerClassp = VN_CAST(m_callerp, Class);

        സ്ഥിരം തർക്കം callerIsClass = callerClassp;
        സ്ഥിരം തർക്കം refIsXref = VN_IS(varRefp, VarXRef);
        സ്ഥിരം തർക്കം varIsFuncLocal = varRefp->varp()->isFuncLocal();
        സ്ഥിരം തർക്കം varHasAutomaticLifetime = varRefp->varp()->lifetime().isAutomatic();
        സ്ഥിരം തർക്കം varIsFieldOfCaller = AstClass::isClassExtendedFrom(callerClassp, varClassp);
        സ്ഥിരം തർക്കം varIsParam = varRefp->varp()->isParam();
        സ്ഥിരം തർക്കം varIsConstraintIterator
            = VN_IS(varRefp->varp()->firstAbovep(), ForeachHeader)
              && VN_IS(varRefp->varp()->firstAbovep()->firstAbovep(), ConstraintForeach);
        എങ്കിൽ (refIsXref) തിരികെ CaptureMode::CAP_മൂല്യം | CaptureMode::CAP_F_XREF;
        എങ്കിൽ (varIsConstraintIterator) തിരികെ CaptureMode::CAP_NO;
        എങ്കിൽ (varIsFuncLocal && varHasAutomaticLifetime) തിരികെ CaptureMode::CAP_മൂല്യം;
        എങ്കിൽ (varIsParam) തിരികെ CaptureMode::CAP_മൂല്യം;
        // Static var in function (will not be inlined, because it's in class)
        എങ്കിൽ (callerIsClass && varIsFuncLocal) തിരികെ CaptureMode::CAP_മൂല്യം;
        // Static class members in static context don't need 'this' capture;
        // V3Class will move both the function and the member to __Vclpkg
        എങ്കിൽ (m_staticContext && callerIsClass && varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            തിരികെ CaptureMode::CAP_NO;
        എങ്കിൽ (callerIsClass && varIsFieldOfCaller) തിരികെ CaptureMode::CAP_THIS;
        // Static member of a different class: V3Width replaces AstMemberSel with a bare
        // AstVarRef (classOrPackagep set). Capture current value as a function argument.
        എങ്കിൽ (callerIsClass && varClassp && !varIsFieldOfCaller
            && varRefp->varp()->lifetime().isStatic())
            തിരികെ CaptureMode::CAP_മൂല്യം;
        UASSERT_OBJ(!callerIsClass, varRefp, "Invalid reference?");
        തിരികെ CaptureMode::CAP_മൂല്യം;
    }

    ശൂന്യം capture_Ref_By_മൂല്യം(Ast_കണ്ണി_Var_Ref* nodep, CaptureMode capModeFlags) {
        AstVar* newVarp;
        സ്ഥിരം തർക്കം newCapture = captureVariable(nodep->fileline(), nodep, newVarp /*ref*/);
        Ast_കണ്ണി_Var_Ref* സ്ഥിരം newVarRefp = newCapture ? nodep->clone_വൃക്ഷം(അസത്യം) : nullptr;
        എങ്കിൽ (!hasFlags(capModeFlags, CaptureMode::CAP_F_SET_CLASSORPACKAGEP)) {
            // Keeping classOrPackagep will cause a broken link after inlining
            nodep->classOrPackagep(nullptr);  // AstScope will figure this out
        }
        nodep->varp(newVarp);
        എങ്കിൽ (!newCapture) തിരികെ;
        എങ്കിൽ (hasFlags(capModeFlags, CaptureMode::CAP_F_XREF)) {
            AstVarRef* സ്ഥിരം notXVarRefp
                = പുതിയ AstVarRef{nodep->fileline(), newVarp, VAccess::READ};
            notXVarRefp->classOrPackagep(nodep->classOrPackagep());
            nodep->replaceWith(notXVarRefp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            nodep = notXVarRefp;
        }
        m_ignore.emplace(nodep);
        m_argsp = Ast_കണ്ണി::addNext(m_argsp, പുതിയ AstArg{nodep->fileline(), "", newVarRefp});
    }

    ശൂന്യം captureRefByThis(Ast_കണ്ണി_Var_Ref* nodep, CaptureMode /*capModeFlags*/) {
        AstVar* സ്ഥിരം thisp = importThisp(nodep->fileline());
        AstVarRef* സ്ഥിരം thisRefp = പുതിയ AstVarRef{nodep->fileline(), thisp, nodep->access()};
        thisRefp->user1(സത്യം);
        m_ignore.emplace(thisRefp);
        AstMemberSel* സ്ഥിരം memberSelp
            = പുതിയ AstMemberSel{nodep->fileline(), thisRefp, nodep->varp()};
        // Propagate random-dependence marking; forcing it on read-only operands
        // would route them through the solver write path.
        എങ്കിൽ (!m_targetp && nodep->user1()) memberSelp->user1(സത്യം);
        memberSelp->user2p(m_targetp);
        nodep->replaceWith(memberSelp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        m_ignore.emplace(memberSelp);
    }

    // VISITORS

    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (m_ignore.എണ്ണം(nodep)) തിരികെ;
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->varp(), nodep, "Variable unlinked");
        എങ്കിൽ (nodep->varp()->isStdRandomizeArg()) തിരികെ;
        സ്ഥിരം CaptureMode capMode = getVarRefCaptureMode(nodep);
        എങ്കിൽ (mode(capMode) == CaptureMode::CAP_NO) തിരികെ;
        എങ്കിൽ (mode(capMode) == CaptureMode::CAP_മൂല്യം) capture_Ref_By_മൂല്യം(nodep, capMode);
        എങ്കിൽ (mode(capMode) == CaptureMode::CAP_THIS) captureRefByThis(nodep, capMode);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (m_ignore.എണ്ണം(nodep)) {
            iterateChildren(nodep);
            തിരികെ;
        }
        m_ignore.emplace(nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Task unlinked");
        // We assume that constraint targets are not referenced this way.
        എങ്കിൽ (VN_IS(nodep, MethodCall) || VN_IS(nodep, New)) {
            m_ignore.emplace(nodep);
            iterateChildren(nodep);
            തിരികെ;
        }
        AstClass* സ്ഥിരം classp = VN_CAST(nodep->taskp()->user2p(), Class);
        എങ്കിൽ ((classp == m_callerp) && VN_IS(m_callerp, Class)) {
            AstArg* സ്ഥിരം argsp = nodep->argsp();
            എങ്കിൽ (argsp) argsp->unlinkFrBack();  // TODO: should this be unlinkFrBackWithNext?
            AstVar* സ്ഥിരം thisp = importThisp(nodep->fileline());
            AstVarRef* സ്ഥിരം thisRefp = പുതിയ AstVarRef{
                nodep->fileline(), thisp, nodep->isPure() ? VAccess::READ : VAccess::READWRITE};
            m_ignore.emplace(thisRefp);
            AstMethodCall* സ്ഥിരം methodCallp
                = പുതിയ AstMethodCall{nodep->fileline(), thisRefp, thisp->name(), argsp};
            methodCallp->taskp(nodep->taskp());
            methodCallp->dtypep(nodep->dtypep());
            nodep->replaceWith(methodCallp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            m_ignore.emplace(methodCallp);
        }
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        എങ്കിൽ (!isReferenceToInnerMember(nodep)) {
            iterateChildren(nodep);
            തിരികെ;
        }
        AstVarRef* സ്ഥിരം varRefp
            = പുതിയ AstVarRef{nodep->fileline(), nodep->varp(), nodep->access()};
        fixupClassOrPackage(nodep->varp(), varRefp);
        varRefp->user1(nodep->user1());
        nodep->replaceWith(varRefp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        m_ignore.emplace(varRefp);
    }
    ശൂന്യം visit(AstMethodCall* nodep) override {
        എങ്കിൽ (!isReferenceToInnerMember(nodep) || m_ignore.എണ്ണം(nodep)) {
            iterateChildren(nodep);
            തിരികെ;
        }
        AstArg* സ്ഥിരം argsp = nodep->argsp();
        എങ്കിൽ (argsp) argsp->unlinkFrBackWithNext();
        Ast_കണ്ണി_FTask_Ref* taskRefp = nullptr;
        എങ്കിൽ (AstTask* സ്ഥിരം taskp = VN_CAST(nodep->taskp(), Task)) {
            taskRefp = പുതിയ AstTaskRef{nodep->fileline(), taskp, argsp};
        } അല്ലെങ്കിൽ എങ്കിൽ (AstFunc* സ്ഥിരം taskp = VN_CAST(nodep->taskp(), Func)) {
            taskRefp = പുതിയ AstFuncRef{nodep->fileline(), taskp, argsp};
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Node needs to point to regular method");
        }
        fixupClassOrPackage(nodep->taskp(), taskRefp);
        taskRefp->user1(nodep->user1());
        nodep->replaceWith(taskRefp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        m_ignore.emplace(taskRefp);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    explicit CaptureVisitor(Ast_കണ്ണി* സ്ഥിരം nodep, Ast_കണ്ണി_Module* callerp, AstClass* സ്ഥിരം targetp,
                            തർക്കം staticContext = അസത്യം)
        : m_callerp{callerp}
        , m_targetp{targetp}
        , m_staticContext{staticContext} {
        iterateAndNextNull(nodep);
    }

    // PUBLIC METHODS

    AstArg* getArgs() സ്ഥിരം { തിരികെ m_argsp; }
    AstVar* getThisp() സ്ഥിരം { തിരികെ m_thisp; }

    ശൂന്യം addFunctionArguments(Ast_കണ്ണി_FTask* funcp) സ്ഥിരം {
        വേണ്ടി (AstArg* argp = getArgs(); argp; argp = VN_AS(argp->nextp(), Arg)) {
            എങ്കിൽ (Ast_കണ്ണി_Var_Ref* varrefp = VN_CAST(argp->exprp(), കണ്ണി_Var_Ref)) {
                എങ്കിൽ ((varrefp->classOrPackagep() == m_callerp) || VN_IS(varrefp, VarXRef)) {
                    // Keeping classOrPackagep will cause a broken link after inlining
                    varrefp->classOrPackagep(nullptr);
                }
                funcp->addStmtsp(getVar(varrefp->varp()));
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(VN_IS(argp->exprp(), ThisRef), argp->exprp(), "Wrong arg expression");
                funcp->addStmtsp(m_thisp);
            }
        }
    }
};

//######################################################################
// Visitor that defines a randomize method where needed

വർഗ്ഗം RandomizeVisitor final : പൊതു VNVisitor {
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
    സ്ഥിരം VNUser3InUse m_inuser3;
    സ്ഥിരം VNUser4InUse m_inuser4;

    // STATE
    V3UniqueNames m_inlineUniqueNames;  // For generating unique function names
    V3UniqueNames m_randArrayUniqueNames{"__Vrandarr"};  // Random array loop indices
    V3UniqueNames m_modeUniqueNames{"__Vmode"};  // For generating unique rand/constraint
                                                 // mode state var names
    V3UniqueNames m_inlineUniqueStdName{"__VStdrand"};
    V3UniqueNames m_uniqueNames;  // Unique names of temporaries, and of blocks holding them
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    std::unordered_map<Ast_കണ്ണി_Module*, AstVar*> m_stdMap;  // Map from module/class to AST Var
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_കണ്ണി_Stmt* m_stmtp = nullptr;  // Current statement
    AstDynArrayDType* m_dynarrayDtp = nullptr;  // Dynamic array type (for rand mode)
    പൂർണ്ണസംഖ്യ m_rand_Case_അക്കം = 0;  // Randcase number within a module for var naming
    പൂർണ്ണസംഖ്യ m_dist_അക്കം = 0;  // Dist bucket variable counter within a module for var naming
    std::map<std::string, AstCDType*> m_randcDtypes;  // RandC data type deduplication
    AstConstraint* m_constraintp = nullptr;  // Current constraint
    std::set<std::string> m_writtenVars;  // Track write_var calls per class to avoid duplicates
    std::map<AstClass*, VInsertionSet<AstVar*>> m_sizeConstrainedArrays;  // Per-class arrays
    std::map<AstClass*, AstVar*>
        m_staticConstraintModeVars;  // Static constraint mode vars per class
    std::map<AstClass*, AstVar*> m_staticRandModeVars;  // Static rand mode vars per class
    std::map<AstClass*, std::pair<തർക്കം, തർക്കം>>
        m_prePostWrap;  // Per-handle-type pre/post virtual wrapper presence
    uint32_t m_uniqueConstraintId = 0;  // current ID of unique call

    // METHODS
    // Check if two nodes are semantically equivalent (not pointer equality):
    സ്ഥിതം തർക്കം is_Similar_കണ്ണി(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* with_വ്യഞ്ജകം, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* arg_വ്യഞ്ജകം) {
        // VarRef: compare variable pointers
        എങ്കിൽ (VN_IS(arg_വ്യഞ്ജകം, VarRef) && VN_IS(with_വ്യഞ്ജകം, VarRef)) {
            തിരികെ VN_AS(with_വ്യഞ്ജകം, VarRef)->varp() == VN_AS(arg_വ്യഞ്ജകം, VarRef)->varp();
        }
        // MemberSel: compare object and member (obj.y)
        എങ്കിൽ (VN_IS(arg_വ്യഞ്ജകം, MemberSel) && VN_IS(with_വ്യഞ്ജകം, MemberSel)) {
            സ്ഥിരം AstMemberSel* സ്ഥിരം withMSp = VN_AS(with_വ്യഞ്ജകം, MemberSel);
            സ്ഥിരം AstMemberSel* സ്ഥിരം argMSp = VN_AS(arg_വ്യഞ്ജകം, MemberSel);
            എങ്കിൽ (withMSp->varp() != argMSp->varp()) തിരികെ അസത്യം;
            // Recursively compare the base object expression
            തിരികെ is_Similar_കണ്ണി(withMSp->fromp(), argMSp->fromp());
        }
        // ArraySel: compare array base and index (arr[i])
        എങ്കിൽ (VN_IS(arg_വ്യഞ്ജകം, ArraySel) && VN_IS(with_വ്യഞ്ജകം, ArraySel)) {
            സ്ഥിരം AstArraySel* സ്ഥിരം withASp = VN_AS(with_വ്യഞ്ജകം, ArraySel);
            സ്ഥിരം AstArraySel* സ്ഥിരം argASp = VN_AS(arg_വ്യഞ്ജകം, ArraySel);
            // Before checking Sel type index, check if both are Const
            എങ്കിൽ (VN_IS(withASp->bitp(), Const) && VN_IS(argASp->bitp(), Const)) {
                സ്ഥിരം AstConst* സ്ഥിരം withIdxp = VN_AS(withASp->bitp(), Const);
                സ്ഥിരം AstConst* സ്ഥിരം argIdxp = VN_AS(argASp->bitp(), Const);
                തിരികെ is_Similar_കണ്ണി(withASp->fromp(), argASp->fromp())
                       && withIdxp->അക്കം().isCaseEq(argIdxp->അക്കം());
            }
            // Index must be Sel type, extract VarRef using fromp()
            എങ്കിൽ (!VN_IS(withASp->bitp(), Sel) || !VN_IS(argASp->bitp(), Sel)) തിരികെ അസത്യം;
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം withIdxp = VN_AS(withASp->bitp(), Sel)->fromp();
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argIdxp = VN_AS(argASp->bitp(), Sel)->fromp();
            തിരികെ is_Similar_കണ്ണി(withASp->fromp(), argASp->fromp())
                   && is_Similar_കണ്ണി(withIdxp, argIdxp);
        }
        തിരികെ അസത്യം;
    }
    // Expand unique{a,b,c} with explicit elements into pairwise != constraints.
    // Whole-array unique{arr} is left for ConstraintExprVisitor's rand_unique handling.
    // Recurses into conditional and foreach bodies, so that the pairwise
    // constraints stay under the guard they were written under.
    സ്ഥിതം ശൂന്യം expandUniqueElementList(Ast_കണ്ണി* itemsp) {
        Ast_കണ്ണി* itemp = itemsp;
        ആകുമ്പോൾ (itemp) {
            Ast_കണ്ണി* സ്ഥിരം nextp = itemp->nextp();
            എങ്കിൽ (സ്ഥിരം AstConstraintIf* സ്ഥിരം ifp = VN_CAST(itemp, ConstraintIf)) {
                expandUniqueElementList(ifp->thensp());
                expandUniqueElementList(ifp->elsesp());
                itemp = nextp;
                തുടരുക;
            }
            എങ്കിൽ (സ്ഥിരം AstConstraintForeach* സ്ഥിരം foreachp = VN_CAST(itemp, ConstraintForeach)) {
                expandUniqueElementList(foreachp->bodyp());
                itemp = nextp;
                തുടരുക;
            }
            AstConstraintUnique* സ്ഥിരം uniquep = VN_CAST(itemp, ConstraintUnique);
            എങ്കിൽ (!uniquep) {
                itemp = nextp;
                തുടരുക;
            }

            std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> വ്യഞ്ജകം_ഇനങ്ങൾ;
            തർക്കം hasArrayVarRef = അസത്യം;
            വേണ്ടി (Ast_കണ്ണി* rp = uniquep->rangesp(); rp; rp = rp->nextp()) {
                എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം vrp = VN_CAST(rp, VarRef)) {
                    // A container's elements are made distinct by rand_unique, not pairwise
                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = vrp->varp()->dtypep()->skipRefp();
                    എങ്കിൽ (VN_IS(dtypep, കണ്ണി_Array_DType) || VN_IS(dtypep, QueueDType)
                        || VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, AssocArrayDType)
                        || VN_IS(dtypep, WildcardArrayDType)) {
                        hasArrayVarRef = സത്യം;
                        തുടരുക;
                    }
                }
                വ്യഞ്ജകം_ഇനങ്ങൾ.തള്ളുക_back(VN_AS(rp, കണ്ണി_വ്യഞ്ജകം));
            }

            എങ്കിൽ (വ്യഞ്ജകം_ഇനങ്ങൾ.size() >= 2) {
                File_വരി* സ്ഥിരം fl = uniquep->fileline();
                വേണ്ടി (size_t i = 0; i < വ്യഞ്ജകം_ഇനങ്ങൾ.size(); i++) {
                    വേണ്ടി (size_t j = i + 1; j < വ്യഞ്ജകം_ഇനങ്ങൾ.size(); j++) {
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = വ്യഞ്ജകം_ഇനങ്ങൾ[i]->clone_വൃക്ഷം(അസത്യം);
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = വ്യഞ്ജകം_ഇനങ്ങൾ[j]->clone_വൃക്ഷം(അസത്യം);
                        AstNeq* സ്ഥിരം neqp = പുതിയ AstNeq{fl, lhsp, rhsp};
                        neqp->user1(സത്യം);
                        Ast_Constraint_വ്യഞ്ജകം* സ്ഥിരം cexprp = പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, neqp};
                        uniquep->addNextHere(cexprp);
                    }
                }
                എങ്കിൽ (!hasArrayVarRef) {
                    uniquep->unlinkFrBack();
                    VL_DO_DANGLING(uniquep->delete_വൃക്ഷം(), uniquep);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (വ്യഞ്ജകം_ഇനങ്ങൾ.size() == 1 && !hasArrayVarRef) {
                // A set of one element is unique whatever that element holds
                uniquep->unlinkFrBack();
                VL_DO_DANGLING(uniquep->delete_വൃക്ഷം(), uniquep);
            }
            itemp = nextp;
        }
    }
    ശൂന്യം createRandomGenerator(AstClass* സ്ഥിരം classp) {
        എങ്കിൽ (classp->user3p()) തിരികെ;
        എങ്കിൽ (classp->extendsp()) {
            createRandomGenerator(classp->extendsp()->classp());
            തിരികെ;
        }
        AstVar* സ്ഥിരം genp = പുതിയ AstVar{classp->fileline(), VVarType::MEMBER, "constraint",
                                        classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        genp->user2p(classp);
        classp->addMembersp(genp);
        classp->user3p(genp);
    }
    AstVar* createStdRandomGenerator(Ast_കണ്ണി_Module* സ്ഥിരം modp) {
        auto it = m_stdMap.find(modp);
        എങ്കിൽ (it == m_stdMap.end()) {
            AstVar* സ്ഥിരം stdgenp
                = പുതിയ AstVar{modp->fileline(), VVarType::MEMBER, "stdrand",
                             modp->findBasicDType(VBasicDTypeKwd::RANDOM_STDGENERATOR)};
            stdgenp->fileline()->warnOff(V3ErrorCode::IMPURE, സത്യം);
            modp->addStmtsp(stdgenp);
            m_stdMap.emplace(modp, stdgenp);
            തിരികെ stdgenp;
        }
        തിരികെ it->second;
    }
    AstTask* createPrepareConstrainedArraysTask(AstClass* സ്ഥിരം classp) {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം name = "__Vprepare_constrained_arrays";
        AstTask* സ്ഥിരം taskp = പുതിയ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(സത്യം);
        classp->addMembersp(taskp);
        m_memberMap.തിരുകുക(classp, taskp);
        തിരികെ taskp;
    }
    AstTask* getCreateAggrResizeTask(AstClass* സ്ഥിരം classp) {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം name = "__Vresize_constrained_arrays";
        AstTask* resizeTaskp = VN_AS(m_memberMap.findMember(classp, name), Task);
        എങ്കിൽ (resizeTaskp) തിരികെ resizeTaskp;
        resizeTaskp = പുതിയ AstTask{classp->fileline(), name, nullptr};
        resizeTaskp->classMethod(സത്യം);
        resizeTaskp->isVirtual(സത്യം);
        classp->addMembersp(resizeTaskp);
        m_memberMap.തിരുകുക(classp, resizeTaskp);
        തിരികെ resizeTaskp;
    }
    AstVar* getCreateRandModeVar(AstClass* സ്ഥിരം classp) {
        എങ്കിൽ (classp->user2p()) തിരികെ VN_AS(classp->user2p(), Var);
        എങ്കിൽ (AstClassExtends* സ്ഥിരം extendsp = classp->extendsp()) {
            തിരികെ getCreateRandModeVar(extendsp->classp());
        }
        AstVar* സ്ഥിരം randModeVarp = createModeVar(classp, "__Vrandmode");
        classp->user2p(randModeVarp);
        തിരികെ randModeVarp;
    }
    AstVar* getCreateConstraintModeVar(AstClass* സ്ഥിരം classp) {
        എങ്കിൽ (classp->user4p()) തിരികെ VN_AS(classp->user4p(), Var);
        എങ്കിൽ (AstClassExtends* സ്ഥിരം extendsp = classp->extendsp()) {
            തിരികെ getCreateConstraintModeVar(extendsp->classp());
        }
        AstVar* സ്ഥിരം constraintModeVarp = createModeVar(classp, "__Vconstraintmode");
        classp->user4p(constraintModeVarp);
        തിരികെ constraintModeVarp;
    }
    സ്ഥിതം AstVar* getConstraintModeVar(AstClass* സ്ഥിരം classp) {
        എങ്കിൽ (classp->user4p()) തിരികെ VN_AS(classp->user4p(), Var);
        എങ്കിൽ (AstClassExtends* സ്ഥിരം extendsp = classp->extendsp()) {
            തിരികെ getConstraintModeVar(extendsp->classp());
        }
        തിരികെ nullptr;
    }
    AstVar* getCreateStaticConstraintModeVar(AstClass* സ്ഥിരം classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        എങ്കിൽ (it != m_staticConstraintModeVars.end()) തിരികെ it->second;
        എങ്കിൽ (AstClassExtends* സ്ഥിരം extendsp = classp->extendsp()) {
            തിരികെ getCreateStaticConstraintModeVar(extendsp->classp());
        }
        AstVar* സ്ഥിരം staticModeVarp = createStaticModeVar(classp, "__Vstaticconstraintmode");
        m_staticConstraintModeVars[classp] = staticModeVarp;
        തിരികെ staticModeVarp;
    }
    AstVar* getStaticConstraintModeVar(AstClass* സ്ഥിരം classp) {
        auto it = m_staticConstraintModeVars.find(classp);
        എങ്കിൽ (it != m_staticConstraintModeVars.end()) തിരികെ it->second;
        എങ്കിൽ (AstClassExtends* സ്ഥിരം extendsp = classp->extendsp()) {
            തിരികെ getStaticConstraintModeVar(extendsp->classp());
        }
        തിരികെ nullptr;
    }
    AstVar* getCreateStaticRandModeVar(AstClass* സ്ഥിരം classp) {
        എങ്കിൽ (m_staticRandModeVars.എണ്ണം(classp)) തിരികെ m_staticRandModeVars[classp];
        എങ്കിൽ (AstClassExtends* സ്ഥിരം extendsp = classp->extendsp()) {
            തിരികെ getCreateStaticRandModeVar(extendsp->classp());
        }
        AstVar* സ്ഥിരം staticModeVarp = createStaticModeVar(classp, "__Vstaticrandmode");
        m_staticRandModeVars[classp] = staticModeVarp;
        തിരികെ staticModeVarp;
    }
    AstVar* getStaticRandModeVar(AstClass* സ്ഥിരം classp) {
        എങ്കിൽ (m_staticRandModeVars.എണ്ണം(classp)) തിരികെ m_staticRandModeVars[classp];
        എങ്കിൽ (AstClassExtends* സ്ഥിരം extendsp = classp->extendsp()) {
            തിരികെ getStaticRandModeVar(extendsp->classp());
        }
        തിരികെ nullptr;
    }
    AstVar* createModeVar(AstClass* സ്ഥിരം classp, സ്ഥിരം അക്ഷരം* സ്ഥിരം name) {
        File_വരി* സ്ഥിരം fl = classp->fileline();
        എങ്കിൽ (!m_dynarrayDtp) {
            m_dynarrayDtp = പുതിയ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        // MEMBER, not MODULETEMP, so V3Localize does not localize it into new().
        AstVar* സ്ഥിരം modeVarp = പുതിയ AstVar{fl, VVarType::MEMBER, name, m_dynarrayDtp};
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        തിരികെ modeVarp;
    }
    AstVar* createStaticModeVar(AstClass* സ്ഥിരം classp, സ്ഥിരം അക്ഷരം* സ്ഥിരം name) {
        // Create a static variable that will be shared across all instances.
        // By setting lifetime to STATIC_EXPLICIT, V3Class will move this to the class package.
        File_വരി* സ്ഥിരം fl = classp->fileline();
        എങ്കിൽ (!m_dynarrayDtp) {
            m_dynarrayDtp = പുതിയ AstDynArrayDType{
                fl, v3Global.rootp()->typeTablep()->findBitDType()->dtypep()};
            m_dynarrayDtp->dtypep(m_dynarrayDtp);
            v3Global.rootp()->typeTablep()->addTypesp(m_dynarrayDtp);
        }
        AstVar* സ്ഥിരം modeVarp = പുതിയ AstVar{fl, VVarType::MODULETEMP, name, m_dynarrayDtp};
        modeVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        // Note: user2p is set to classp here. V3Scope will later update varScopep
        // to point to the package scope when the variable is moved by V3Class.
        modeVarp->user2p(classp);
        classp->addStmtsp(modeVarp);
        // Register so findStaticRandModeVarMember sees it through the already
        // scanned member cache
        m_memberMap.തിരുകുക(classp, modeVarp);
        തിരികെ modeVarp;
    }
    സ്ഥിതം ശൂന്യം addSetRandMode(Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp, AstVar* സ്ഥിരം genp,
                               AstVar* സ്ഥിരം randModeVarp) {
        File_വരി* സ്ഥിരം fl = ftaskp->fileline();
        AstCMethodHard* സ്ഥിരം setRandModep = പുതിയ AstCMethodHard{
            fl, പുതിയ AstVarRef{fl, VN_AS(genp->user2p(), കണ്ണി_Module), genp, VAccess::WRITE},
            VCMethod::CLASS_SET_RANDMODE,
            പുതിയ AstVarRef{fl, VN_AS(randModeVarp->user2p(), കണ്ണി_Module), randModeVarp,
                          VAccess::READ}};
        setRandModep->dtypeSetVoid();
        ftaskp->addStmtsp(setRandModep->makeStmt());
    }
    സ്ഥിതം ശൂന്യം addSetStaticRandMode(Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp, AstVar* സ്ഥിരം genp,
                                     AstVar* സ്ഥിരം staticRandModeVarp) {
        File_വരി* സ്ഥിരം fl = ftaskp->fileline();
        AstCMethodHard* സ്ഥിരം setp = പുതിയ AstCMethodHard{
            fl, പുതിയ AstVarRef{fl, VN_AS(genp->user2p(), കണ്ണി_Module), genp, VAccess::WRITE},
            VCMethod::RANDOMIZER_SET_STATIC_RANDMODE,
            പുതിയ AstVarRef{fl, VN_AS(staticRandModeVarp->user2p(), കണ്ണി_Module), staticRandModeVarp,
                          VAccess::READ}};
        setp->dtypeSetVoid();
        ftaskp->addStmtsp(setp->makeStmt());
    }
    ശൂന്യം createRandomizeClassVars(AstNetlist* സ്ഥിരം netlistp) {
        // Defer init to one emission per root with max descendant count;
        // super.new() runs Base ctor before Derived can resize the static array.
        std::map<AstClass*, uint32_t> വേര്_Static_Rand_Mode_എണ്ണം;
        netlistp->foreach([ഇത്, &വേര്_Static_Rand_Mode_എണ്ണം](AstClass* സ്ഥിരം classp) {
            തർക്കം hasConstraints = അസത്യം;
            uint32_t rand_Mode_എണ്ണം = 0;
            uint32_t static_Rand_Mode_എണ്ണം = 0;
            uint32_t constraint_Mode_എണ്ണം = 0;
            uint32_t static_Constraint_Mode_എണ്ണം = 0;
            classp->foreachMember([&](AstClass*, Ast_കണ്ണി* memberp) {
                // SystemVerilog only allows single inheritance, so we don't need to worry about
                // index overlap. If the index > 0, it's already been set.
                എങ്കിൽ (AstConstraint* സ്ഥിരം constrp = VN_CAST(memberp, Constraint)) {
                    hasConstraints = സത്യം;
                    RandomizeMode constraintMode = {.asUQuad = memberp->user1()};
                    എങ്കിൽ (!constraintMode.usesMode) തിരികെ;
                    എങ്കിൽ (constraintMode.index == 0) {
                        // Use separate index counters for static vs non-static constraints
                        എങ്കിൽ (constrp->isStatic()) {
                            constraintMode.index = static_Constraint_Mode_എണ്ണം++;
                        } അല്ലെങ്കിൽ {
                            constraintMode.index = constraint_Mode_എണ്ണം++;
                        }
                        memberp->user1(constraintMode.asUQuad);
                    } അല്ലെങ്കിൽ {
                        എങ്കിൽ (constrp->isStatic()) {
                            static_Constraint_Mode_എണ്ണം = constraintMode.index + 1;
                        } അല്ലെങ്കിൽ {
                            constraint_Mode_എണ്ണം = constraintMode.index + 1;
                        }
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(memberp, Var)) {
                    RandomizeMode randMode = {.asUQuad = memberp->user1()};
                    എങ്കിൽ (!randMode.usesMode) തിരികെ;
                    സ്ഥിരം തർക്കം isStaticVar = varp->lifetime().isStatic();
                    എങ്കിൽ (randMode.index == 0) {
                        എങ്കിൽ (isStaticVar) {
                            randMode.index = static_Rand_Mode_എണ്ണം++;
                        } അല്ലെങ്കിൽ {
                            randMode.index = rand_Mode_എണ്ണം++;
                        }
                        memberp->user1(randMode.asUQuad);
                    } അല്ലെങ്കിൽ {
                        എങ്കിൽ (isStaticVar) {
                            static_Rand_Mode_എണ്ണം = randMode.index + 1;
                        } അല്ലെങ്കിൽ {
                            rand_Mode_എണ്ണം = randMode.index + 1;
                        }
                    }
                }
            });
            // Also account for rand_mode indices from globalConstrained sub-objects.
            // When constraints from sub-objects are flattened into this class's constraint
            // setup functions, the generated code references this->__Vrandmode.at(index).
            // We must ensure this class's __Vrandmode is initialized to cover those indices.
            {
                std::function<ശൂന്യം(AstClass*)> findSubObjRandModes = [&](AstClass* subClassp) {
                    subClassp->foreachMember([&](AstClass*, Ast_കണ്ണി* subMemberp) {
                        എങ്കിൽ (AstVar* സ്ഥിരം subVarp = VN_CAST(subMemberp, Var)) {
                            സ്ഥിരം RandomizeMode rm = {.asUQuad = subVarp->user1()};
                            എങ്കിൽ (!rm.usesMode) തിരികെ;
                            // Static rand vars index into their own class's static
                            // rand mode array, not into the outer __Vrandmode.
                            എങ്കിൽ (subVarp->lifetime().isStatic()) തിരികെ;
                            സ്ഥിരം uint32_t needed = rm.index + 1;
                            എങ്കിൽ (needed > rand_Mode_എണ്ണം) rand_Mode_എണ്ണം = needed;
                        }
                    });
                };
                classp->foreachMember([&](AstClass*, AstVar* memberVarp) {
                    എങ്കിൽ (!memberVarp->globalConstrained()) തിരികെ;
                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = memberVarp->dtypep()->skipRefp();
                    സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(dtypep, ClassRefDType);
                    എങ്കിൽ (!classRefp) തിരികെ;
                    findSubObjRandModes(classRefp->classp());
                });
            }
            എങ്കിൽ (hasConstraints) createRandomGenerator(classp);
            എങ്കിൽ (rand_Mode_എണ്ണം > 0) {
                AstVar* സ്ഥിരം randModeVarp = getCreateRandModeVar(classp);
                makeModeInit(randModeVarp, classp, rand_Mode_എണ്ണം);
            }
            എങ്കിൽ (constraint_Mode_എണ്ണം > 0) {
                AstVar* സ്ഥിരം constraintModeVarp = getCreateConstraintModeVar(classp);
                makeModeInit(constraintModeVarp, classp, constraint_Mode_എണ്ണം);
            }
            എങ്കിൽ (static_Constraint_Mode_എണ്ണം > 0) {
                AstVar* സ്ഥിരം staticConstraintModeVarp = getCreateStaticConstraintModeVar(classp);
                makeStaticModeInit(staticConstraintModeVarp, classp, static_Constraint_Mode_എണ്ണം);
            }
            എങ്കിൽ (static_Rand_Mode_എണ്ണം > 0) {
                getCreateStaticRandModeVar(classp);
                AstClass* rootp = classp;
                ആകുമ്പോൾ (AstClassExtends* സ്ഥിരം ep = rootp->extendsp()) rootp = ep->classp();
                uint32_t& slot = വേര്_Static_Rand_Mode_എണ്ണം[rootp];
                എങ്കിൽ (static_Rand_Mode_എണ്ണം > slot) slot = static_Rand_Mode_എണ്ണം;
            }
        });
        വേണ്ടി (സ്ഥിരം auto& kv : വേര്_Static_Rand_Mode_എണ്ണം) emit_വേര്_Static_Mode_Init(kv.first, kv.second);
    }
    ശൂന്യം emit_വേര്_Static_Mode_Init(AstClass* സ്ഥിരം rootp, സ്ഥിരം uint32_t എണ്ണം) {
        AstVar* സ്ഥിരം staticRandModeVarp = m_staticRandModeVars[rootp];
        UASSERT_OBJ(staticRandModeVarp, rootp, "Root must have a static rand-mode var");
        makeStaticModeInit(staticRandModeVarp, rootp, എണ്ണം);
    }
    ശൂന്യം makeModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_എണ്ണം) {
        Ast_കണ്ണി_Module* സ്ഥിരം modeVarModp = VN_AS(modeVarp->user2p(), കണ്ണി_Module);
        File_വരി* സ്ഥിരം fl = modeVarp->fileline();
        AstVar* സ്ഥിരം oldSizeVarp = പുതിയ AstVar{
            fl, VVarType::BLOCKTEMP, m_uniqueNames.get("__VoldSize"), modeVarp->findUInt32DType()};
        oldSizeVarp->funcLocal(സത്യം);
        oldSizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* സ്ഥിരം oldSizep
            = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        oldSizep->dtypeSetUInt32();
        AstCMethodHard* സ്ഥിരം dynarrayNewp
            = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, പുതിയ AstConst{fl, mode_എണ്ണം}};
        dynarrayNewp->dtypeSetVoid();
        Ast_കണ്ണി_FTask* സ്ഥിരം ctorNewp = VN_AS(m_memberMap.findMember(classp, "new"), കണ്ണി_FTask);
        UASSERT_OBJ(ctorNewp, classp, "No new() in class");
        // Preserve inherited modes set by super.new() and initialize only newly added entries.
        Ast_കണ്ണി* സ്ഥിരം initFirstp = oldSizeVarp;
        initFirstp->addNext(
            പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, oldSizeVarp, VAccess::WRITE}, oldSizep});
        initFirstp->addNext(dynarrayNewp->makeStmt());
        initFirstp->addNext(makeModeSetLoop(
            fl, പുതിയ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE}, പുതിയ AstConst{fl, 1},
            സത്യം, പുതിയ AstVarRef{fl, oldSizeVarp, VAccess::READ}));
        // Prepend init code before user statements in constructor body, but after
        // var declarations and super.new(). This ensures that user's constraint_mode()
        // or rand_mode() calls in the constructor execute after mode arrays are initialized.
        // Pattern from V3LinkDot::addImplicitSuperNewCall.
        വേണ്ടി (Ast_കണ്ണി* stmtp = ctorNewp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (!VN_IS(stmtp, കണ്ണി_Stmt)) തുടരുക;  // Skip var declarations
            എങ്കിൽ (സ്ഥിരം Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം sep = VN_CAST(stmtp, Stmt_വ്യഞ്ജകം)) {
                എങ്കിൽ (VN_IS(sep->exprp(), New)) തുടരുക;  // Skip super.new()
            }
            // Found first user statement - insert init code before it
            stmtp->addHereThisAsNext(initFirstp);
            തിരികെ;
        }
        // No user statements (empty constructor or only var decls/super.new)
        ctorNewp->addStmtsp(initFirstp);
    }
    ശൂന്യം makeStaticModeInit(AstVar* modeVarp, AstClass* classp, uint32_t mode_എണ്ണം) {
        // For static constraint mode, we need lazy initialization since it's shared across
        // instances. Generate: if (size() == 0) { resize(N); set all to 1; }
        Ast_കണ്ണി_Module* സ്ഥിരം modeVarModp = VN_AS(modeVarp->user2p(), കണ്ണി_Module);
        File_വരി* സ്ഥിരം fl = modeVarp->fileline();

        // Build the condition: size() == 0
        AstCMethodHard* സ്ഥിരം sizep
            = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, modeVarModp, modeVarp, VAccess::READ},
                                 VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstEq* സ്ഥിരം condp = പുതിയ AstEq{fl, sizep, പുതിയ AstConst{fl, 0}};

        // Build the then-block: resize and set all to 1
        AstCMethodHard* സ്ഥിരം dynarrayNewp
            = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                                 VCMethod::DYN_RESIZE, പുതിയ AstConst{fl, mode_എണ്ണം}};
        dynarrayNewp->dtypeSetVoid();
        Ast_കണ്ണി* സ്ഥിരം thenStmtsp = dynarrayNewp->makeStmt();
        thenStmtsp->addNext(
            makeModeSetLoop(fl, പുതിയ AstVarRef{fl, modeVarModp, modeVarp, VAccess::WRITE},
                            പുതിയ AstConst{fl, 1}, സത്യം));

        // Build the if statement
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{fl, condp, thenStmtsp};

        // Add to new() constructor
        Ast_കണ്ണി_FTask* സ്ഥിരം newp = VN_AS(m_memberMap.findMember(classp, "new"), കണ്ണി_FTask);
        UASSERT_OBJ(newp, classp, "No new() in class");
        newp->addStmtsp(ifp);
    }
    Ast_കണ്ണി* makeModeSetLoop(File_വരി* സ്ഥിരം fl, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp,
                             തർക്കം inTask, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം startp = nullptr) {
        AstVar* സ്ഥിരം iterVarp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, "i", lhsp->findUInt32DType()};
        iterVarp->funcLocal(inTask);
        iterVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        AstCMethodHard* സ്ഥിരം sizep = പുതിയ AstCMethodHard{fl, lhsp, VCMethod::DYN_SIZE, nullptr};
        sizep->dtypeSetUInt32();
        AstCMethodHard* സ്ഥിരം setp
            = പുതിയ AstCMethodHard{fl, lhsp->clone_വൃക്ഷം(അസത്യം), VCMethod::ARRAY_AT_WRITE,
                                 പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}};
        setp->dtypeSetUInt32();
        Ast_കണ്ണി* സ്ഥിരം stmtsp = iterVarp;
        stmtsp->addNext(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, iterVarp, VAccess::WRITE},
                                      startp ? startp : പുതിയ AstConst{fl, 0}});

        AstLoop* സ്ഥിരം loopp = പുതിയ AstLoop{fl};
        stmtsp->addNext(loopp);
        loopp->addStmtsp(പുതിയ AstLoopTest{
            fl, loopp, പുതിയ AstLt{fl, പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}, sizep}});
        loopp->addStmtsp(പുതിയ AstAssign{fl, setp, rhsp});
        loopp->addStmtsp(പുതിയ AstAssign{
            fl, പുതിയ AstVarRef{fl, iterVarp, VAccess::WRITE},
            പുതിയ AstAdd{fl, പുതിയ AstConst{fl, 1}, പുതിയ AstVarRef{fl, iterVarp, VAccess::READ}}});
        തിരികെ പുതിയ AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtsp, സത്യം};
    }
    ശൂന്യം addTempVarsp(AstVar* varsp) {
        വേണ്ടി (AstVar* varp = varsp; varp; varp = VN_AS(varp->nextp(), Var)) {
            എങ്കിൽ (m_ftaskp) {
                varp->funcLocal(സത്യം);
            } അല്ലെങ്കിൽ {
                varp->varType(VVarType::MODULETEMP);
                varp->lifetime(VLifetime::STATIC_EXPLICIT);
            }
        }
        എങ്കിൽ (!m_ftaskp) {
            m_modp->addStmtsp(varsp);
        } അല്ലെങ്കിൽ {
            m_ftaskp->prependStmtsp(varsp);
        }
    }
    Ast_കണ്ണി_Stmt* wrapIfRandMode(AstClass* classp, AstVar* സ്ഥിരം varp, Ast_കണ്ണി_Stmt* stmtp) {
        സ്ഥിരം RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstVar* സ്ഥിരം modeVarp = varp->lifetime().isStatic() ? getStaticRandModeVar(classp)
                                                             : getRandModeVarFromClass(classp);
        തിരികെ VN_AS(wrapIfMode(rmode, modeVarp, stmtp), കണ്ണി_Stmt);
    }
    Ast_കണ്ണി* wrapIfConstraintMode(AstClass* classp, AstConstraint* സ്ഥിരം constrp, Ast_കണ്ണി* stmtp) {
        സ്ഥിരം RandomizeMode rmode = {.asUQuad = constrp->user1()};
        AstVar* സ്ഥിരം modeVarp = constrp->isStatic() ? getStaticConstraintModeVar(classp)
                                                     : getConstraintModeVar(classp);
        തിരികെ wrapIfMode(rmode, modeVarp, stmtp);
    }
    സ്ഥിതം Ast_കണ്ണി* wrapIfMode(സ്ഥിരം RandomizeMode mode, AstVar* modeVarp, Ast_കണ്ണി* stmtp) {
        File_വരി* സ്ഥിരം fl = stmtp->fileline();
        എങ്കിൽ (mode.usesMode) {
            // user2p can be either AstClass or AstClassPackage (for static constraints)
            Ast_കണ്ണി_Module* സ്ഥിരം modp = VN_AS(modeVarp->user2p(), കണ്ണി_Module);
            AstCMethodHard* സ്ഥിരം atp
                = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, modp, modeVarp, VAccess::READ},
                                     VCMethod::ARRAY_AT, പുതിയ AstConst{fl, mode.index}};
            atp->dtypeSetUInt32();
            തിരികെ പുതിയ AstIf{fl, atp, stmtp};
        }
        തിരികെ stmtp;
    }
    AstVarRef* enum_മൂല്യം_Tab_Refp(AstEnumDType* സ്ഥിരം nodep) {
        // Return a reference to a constant table of the values of the given enum. The reference
        // is cached, so the table is built only once.
        എങ്കിൽ (nodep->user2p()) തിരികെ VN_AS(nodep->user2p(), VarRef)->clone_വൃക്ഷം(അസത്യം);
        UINFO(9, "Construct enum value table " << nodep);
        Ast_കണ്ണി_Array_DType* സ്ഥിരം vardtypep = പുതിയ AstUnpackArrayDType{
            nodep->fileline(), nodep->dtypep(),
            പുതിയ AstRange{nodep->fileline(), static_cast<പൂർണ്ണസംഖ്യ>(nodep->item_എണ്ണം()) - 1, 0}};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        AstConsArray* സ്ഥിരം consp = പുതിയ AstConsArray{nodep->fileline(), vardtypep};
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        uint64_t index = 0;
        വേണ്ടി (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* സ്ഥിരം vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            consp->valuep(index++, vconstp->clone_വൃക്ഷം(അസത്യം));
        }
        // Share identical tables via the constant pool
        AstVarRef* സ്ഥിരം refp = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_വൃക്ഷം(), consp);  // V3ConstPool::find clones it
        തള്ളുക_Deletep(refp);  // Deleted with the visitor - always cloned
        nodep->user2p(refp);
        തിരികെ refp->clone_വൃക്ഷം(അസത്യം);
    }

    AstCDType* findVlRandCDType(File_വരി* സ്ഥിരം fl, uint64_t ഇനങ്ങൾ) {
        // For 8 items we need to have a 9 item LFSR so items is max count
        // width(items) = log2(items) + 1
        സ്ഥിരം std::string type = AstCDType::typeToHold(V3_സംഖ്യ::log2bQuad(ഇനങ്ങൾ) + 1);
        സ്ഥിരം std::string name = "VlRandC<" + type + ", " + cvtToStr(ഇനങ്ങൾ) + "ULL>";
        // Create or reuse (to avoid duplicates) randomization object dtype
        സ്ഥിരം auto pair = m_randcDtypes.emplace(name, nullptr);
        എങ്കിൽ (pair.second) {
            AstCDType* സ്ഥിരം newp = പുതിയ AstCDType{fl, name};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            pair.first->second = newp;
        }
        തിരികെ pair.first->second;
    }

    AstVar* newRandcVarsp(AstVar* സ്ഥിരം varp, AstClass* സ്ഥിരം classp) {
        // Might be called multiple times on same var, if var is referenced in class extends,
        // If so, each is randomized separately, so the select lands in the extending class

        // If a randc, make a VlRandC object to hold the state
        എങ്കിൽ (!varp->isRandC()) തിരികെ nullptr;

        uint64_t ഇനങ്ങൾ = 0;
        എങ്കിൽ (AstEnumDType* സ്ഥിരം enumDtp = VN_CAST(varp->dtypep()->skipRefToEnump(), EnumDType)) {
            ഇനങ്ങൾ = static_cast<uint64_t>(enumDtp->item_എണ്ണം());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstBasicDType* സ്ഥിരം basicp = varp->dtypep()->skipRefp()->basicp()) {
            എങ്കിൽ (basicp->width() > 32) {
                // Real/chandle/string/event are rejected by type upstream already;
                // an over-width integral type isn't, so still needs its own message.
                എങ്കിൽ (!V3Error::error_എണ്ണം()) {
                    varp->v3error("Maximum implemented width for randc is 32 bits, "
                                  << varp->prettyNameQ() << " is " << basicp->width() << " bits");
                }
                varp->rand(VRandAttr::RAND);
                തിരികെ nullptr;
            }
            ഇനങ്ങൾ = 1ULL << basicp->width();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstStructDType* സ്ഥിരം dtp = VN_CAST(varp->dtypep()->skipRefp(), StructDType)) {
            UASSERT_OBJ(!dtp->packed(), dtp, "skipRef should have hidden packed before got here");
            dtp->v3error("Unpacked structs shall not be declared as randc"
                         " (IEEE 1800-2023 18.4)");
            തിരികെ nullptr;
        } അല്ലെങ്കിൽ {
            // Only remaining possibility: an object handle, virtual interface,
            // or unpacked union declared randc -- checkRandTypeEligibility()
            // already rejected each of those with its own error, earlier in
            // this same pass, before newRandcVarsp() ever runs on this var.
            UASSERT_OBJ(V3Error::error_എണ്ണം(), varp, "Unexpected randc variable dtype");
            തിരികെ nullptr;
        }
        AstCDType* സ്ഥിരം newdtp = findVlRandCDType(varp->fileline(), ഇനങ്ങൾ);
        AstVar* സ്ഥിരം newp
            = പുതിയ AstVar{varp->fileline(), VVarType::MEMBER, varp->name() + "__Vrandc", newdtp};
        newp->isInternal(സത്യം);
        classp->addStmtsp(newp);
        UINFO(9, "created " << varp);
        തിരികെ newp;
    }
    Ast_കണ്ണി_Stmt* createArrayForeachLoop(File_വരി* സ്ഥിരം fl, Ast_കണ്ണി_DType* സ്ഥിരം dtypep,
                                        Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstVar* സ്ഥിരം outputVarp) {
        Ast_കണ്ണി_DType* tempDTypep = dtypep;
        AstVar* randLoopIndxp = nullptr;
        auto createLoopIndex = [&](Ast_കണ്ണി_DType* tempDTypep) {
            എങ്കിൽ (VN_IS(tempDTypep, AssocArrayDType)) {
                തിരികെ പുതിയ AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                                  VN_AS(tempDTypep, AssocArrayDType)->താക്കോൽ_DTypep()};
            }
            തിരികെ പുതിയ AstVar{fl, VVarType::VAR, m_randArrayUniqueNames.get(""),
                              dtypep->findBasicDType(VBasicDTypeKwd::UINT32)};
        };
        Ast_കണ്ണി_വ്യഞ്ജകം* tempElementp = nullptr;
        ആകുമ്പോൾ (tempDTypep->isNonPackedArray()) {
            AstVar* സ്ഥിരം newRandLoopIndxp = createLoopIndex(tempDTypep);
            randLoopIndxp = Ast_കണ്ണി::addNext(randLoopIndxp, newRandLoopIndxp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം tempExprp = tempElementp ? tempElementp : exprp;
            AstVarRef* സ്ഥിരം tempRefp = പുതിയ AstVarRef{fl, newRandLoopIndxp, VAccess::READ};
            എങ്കിൽ (VN_IS(tempDTypep, DynArrayDType)) {
                tempElementp
                    = പുതിയ AstCMethodHard{fl, tempExprp, VCMethod::ARRAY_AT_WRITE, tempRefp};
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(tempDTypep, UnpackArrayDType)) {
                Ast_കണ്ണി_Array_DType* സ്ഥിരം aryDTypep = VN_CAST(tempDTypep, കണ്ണി_Array_DType);
                // Adjust the bitp to ensure it covers all possible indices
                tempElementp = പുതിയ AstArraySel{
                    fl, tempExprp,
                    പുതിയ AstSel{
                        fl,
                        പുതിയ AstSub{fl, tempRefp,
                                   പുതിയ AstConst{fl, static_cast<uint32_t>(aryDTypep->lo())}},
                        പുതിയ AstConst{fl, 0}, V3_സംഖ്യ::log2b(aryDTypep->hi()) + 1}};
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(tempDTypep, AssocArrayDType)) {
                tempElementp = പുതിയ AstAssocSel{fl, tempExprp, tempRefp};
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(tempDTypep, QueueDType)) {
                tempElementp
                    = പുതിയ AstCMethodHard{fl, tempExprp, VCMethod::DYN_AT_WRITE_APPEND, tempRefp};
            }
            tempElementp->dtypep(tempDTypep->subDTypep());
            tempDTypep = tempDTypep->virtRefDTypep();
        }

        AstForeachHeader* സ്ഥിരം headerp
            = പുതിയ AstForeachHeader{fl, exprp->clone_വൃക്ഷം(അസത്യം), randLoopIndxp};
        Ast_കണ്ണി_Stmt* സ്ഥിരം randStmtsp = newRandStmtsp(fl, tempElementp, nullptr, outputVarp);
        // TODO: we should just not clone in 'newRandStmtsp' if not necessary
        എങ്കിൽ (!tempElementp->backp()) VL_DO_DANGLING(തള്ളുക_Deletep(tempElementp), tempElementp);
        തിരികെ പുതിയ AstForeach{fl, headerp, randStmtsp};
    }
    Ast_കണ്ണി_Stmt* newRandStmtsp(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstVar* randcVarp,
                               AstVar* സ്ഥിരം outputVarp, പൂർണ്ണസംഖ്യ offset = 0,
                               AstMemberDType* memberp = nullptr) {
        Ast_കണ്ണി_DType* സ്ഥിരം memberDtp
            = memberp ? memberp->subDTypep()->skipRefp() : exprp->dtypep()->skipRefp();
        എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം structDtp = VN_CAST(memberDtp, StructDType)) {
            Ast_കണ്ണി_Stmt* stmtsp = nullptr;
            എങ്കിൽ (structDtp->packed()) offset += memberp ? memberp->lsb() : 0;
            വേണ്ടി (AstMemberDType* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                Ast_കണ്ണി_Stmt* randp = nullptr;
                എങ്കിൽ (structDtp->packed()) {
                    randp = newRandStmtsp(fl, stmtsp ? exprp->clone_വൃക്ഷം(അസത്യം) : exprp, nullptr,
                                          outputVarp, offset, smemberp);
                } അല്ലെങ്കിൽ എങ്കിൽ (smemberp->rand().isRandomizable()) {
                    // IEEE 1800-2023 18.4: an unpacked struct member is only
                    // randomized if its own declaration carries rand/randc.
                    AstStructSel* structSelp
                        = പുതിയ AstStructSel{fl, exprp->clone_വൃക്ഷം(അസത്യം), smemberp->name()};
                    structSelp->dtypep(smemberp->childDTypep());
                    എങ്കിൽ (!structSelp->dtypep()) structSelp->dtypep(smemberp->subDTypep());
                    randp = newRandStmtsp(fl, structSelp, nullptr, outputVarp);
                }
                stmtsp = Ast_കണ്ണി::addNextNull(stmtsp, randp);
            }
            തിരികെ stmtsp;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം unionDtp = VN_CAST(memberDtp, UnionDType)) {
            // Illegal for rand/randc, rejected by type upstream for every such
            // case; nothing valid to build a randomize statement for here.
            എങ്കിൽ (!unionDtp->packed()) തിരികെ nullptr;
            AstMemberDType* സ്ഥിരം firstMemberp = unionDtp->membersp();
            തിരികെ newRandStmtsp(fl, exprp, nullptr, outputVarp, offset, firstMemberp);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefDtp = VN_CAST(memberDtp, ClassRefDType)) {
            AstFunc* സ്ഥിരം memberFuncp
                = V3Randomize::newRandomizeFunc(m_memberMap, classRefDtp->classp());
            AstMethodCall* സ്ഥിരം callp = പുതിയ AstMethodCall{fl, exprp, "randomize"};
            callp->taskp(memberFuncp);
            callp->dtypeFrom(memberFuncp);
            AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{
                fl, പുതിയ AstVarRef{fl, outputVarp, VAccess::WRITE},
                പുതിയ AstAnd{fl, പുതിയ AstVarRef{fl, outputVarp, VAccess::READ}, callp}};
            തിരികെ പുതിയ AstIf{
                fl, പുതിയ AstNeq{fl, exprp->clone_വൃക്ഷം(അസത്യം), പുതിയ AstConst{fl, AstConst::Null{}}},
                assignp};
        } അല്ലെങ്കിൽ എങ്കിൽ (AstDynArrayDType* സ്ഥിരം dynarrayDtp = VN_CAST(memberDtp, DynArrayDType)) {
            തിരികെ createArrayForeachLoop(fl, dynarrayDtp, exprp, outputVarp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstQueueDType* സ്ഥിരം queueDtp = VN_CAST(memberDtp, QueueDType)) {
            തിരികെ createArrayForeachLoop(fl, queueDtp, exprp, outputVarp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം unpackarrayDtp
                   = VN_CAST(memberDtp, UnpackArrayDType)) {
            തിരികെ createArrayForeachLoop(fl, unpackarrayDtp, exprp, outputVarp);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstAssocArrayDType* സ്ഥിരം assocarrayDtp = VN_CAST(memberDtp, AssocArrayDType)) {
            തിരികെ createArrayForeachLoop(fl, assocarrayDtp, exprp, outputVarp);
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_വ്യഞ്ജകം* valp;
            എങ്കിൽ (AstEnumDType* സ്ഥിരം enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                              : exprp->dtypep()->subDTypep(),
                                                      EnumDType)) {
                AstVarRef* സ്ഥിരം tabRefp = enum_മൂല്യം_Tab_Refp(enumDtp);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം randp
                    = new_Rand_മൂല്യം(fl, randcVarp, exprp->findBasicDType(VBasicDTypeKwd::UINT32));
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം moddivp = പുതിയ AstModDiv{
                    fl, randp, പുതിയ AstConst{fl, static_cast<uint32_t>(enumDtp->item_എണ്ണം())}};
                moddivp->dtypep(enumDtp);
                valp = പുതിയ AstArraySel{fl, tabRefp, moddivp};
            } അല്ലെങ്കിൽ {
                valp
                    = new_Rand_മൂല്യം(fl, randcVarp, (memberp ? memberp->dtypep() : exprp->dtypep()));
            }
            AstAssign* assignp
                = പുതിയ AstAssign{fl,
                                പുതിയ AstSel{fl, exprp, offset + (memberp ? memberp->lsb() : 0),
                                           memberp ? memberp->width() : exprp->width()},
                                valp};
            AstVar* varp = nullptr;
            exprp->exists([&](സ്ഥിരം AstVarRef* varrefp) {
                എങ്കിൽ (varrefp->access().isWriteOrRW()) varp = varrefp->varp();
                തിരികെ varp != nullptr;
            });
            തിരികെ wrapIfRandMode(VN_AS(m_modp, Class), varp, assignp);
        }
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* new_Rand_മൂല്യം(File_വരി* സ്ഥിരം fl, AstVar* സ്ഥിരം randcVarp,
                              Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        എങ്കിൽ (randcVarp) {
            AstCExpr* സ്ഥിരം cexprp = പുതിയ AstCExpr{fl};
            cexprp->add(പുതിയ AstVarRef{fl, randcVarp, VAccess::READWRITE});
            cexprp->add(".randomize(__Vm_rng)");
            cexprp->dtypep(dtypep);
            തിരികെ cexprp;
        }

        തിരികെ പുതിയ AstRandRNG{fl, dtypep};
    }
    // Find pre_randomize/post_randomize task in class hierarchy (walks extendsp chain)
    AstTask* findPrePostTask(AstClass* classp, സ്ഥിരം string& name) {
        വേണ്ടി (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            എങ്കിൽ (AstTask* സ്ഥിരം taskp = VN_CAST(m_memberMap.findMember(cp, name), Task)) {
                തിരികെ taskp;
            }
        }
        തിരികെ nullptr;
    }
    ശൂന്യം addPrePostCall(AstClass* സ്ഥിരം classp, Ast_കണ്ണി_FTask* സ്ഥിരം funcp, സ്ഥിരം string& name) {
        എങ്കിൽ (AstTask* സ്ഥിരം userFuncp = findPrePostTask(classp, name)) {
            AstTaskRef* സ്ഥിരം callp = പുതിയ AstTaskRef{userFuncp->fileline(), userFuncp};
            funcp->addStmtsp(callp->makeStmt());
        }
    }
    // Per-class virtual wrapper that invokes the class's effective
    // pre_randomize/post_randomize. IEEE 1800-2023 18.6.2: pre_randomize and
    // post_randomize "appear to behave as virtual methods" because randomize()
    // is virtual. The inline `randomize() with` path builds a non-virtual
    // function on the static handle type, so it dispatches pre/post through
    // this wrapper to reach the dynamic type's override.
    AstTask* getCreatePrePostCallback(AstClass* സ്ഥിരം classp, സ്ഥിരം string& which) {
        സ്ഥിരം string name = "__V" + which;
        എങ്കിൽ (AstTask* സ്ഥിരം existingp = VN_CAST(m_memberMap.findMember(classp, name), Task)) {
            തിരികെ existingp;
        }
        AstTask* സ്ഥിരം taskp = പുതിയ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(സത്യം);
        taskp->isVirtual(classp->isExtended());
        classp->addMembersp(taskp);
        m_memberMap.തിരുകുക(classp, taskp);
        addPrePostCall(classp, taskp, which);
        തിരികെ taskp;
    }
    // Build the virtual pre/post wrappers across classp's whole hierarchy so a
    // `randomize() with` through a base handle dispatches to a derived
    // override. Returns whether a pre/post wrapper exists anywhere in the
    // hierarchy (cached per static handle type).
    std::pair<തർക്കം, തർക്കം> buildPrePostVirtualWrappers(AstClass* സ്ഥിരം classp) {
        സ്ഥിരം auto cachedIt = m_prePostWrap.find(classp);
        എങ്കിൽ (cachedIt != m_prePostWrap.end()) തിരികെ cachedIt->second;
        std::vector<AstClass*> hierp{classp};
        v3Global.rootp()->foreach([&](AstClass* subp) {
            എങ്കിൽ (subp != classp && AstClass::isClassExtendedFrom(subp, classp))
                hierp.തള്ളുക_back(subp);
        });
        തർക്കം hasPre = അസത്യം;
        തർക്കം hasPost = അസത്യം;
        വേണ്ടി (AstClass* സ്ഥിരം cp : hierp) {
            എങ്കിൽ (findPrePostTask(cp, "pre_randomize")) {
                getCreatePrePostCallback(cp, "pre_randomize");
                hasPre = സത്യം;
            }
            എങ്കിൽ (findPrePostTask(cp, "post_randomize")) {
                getCreatePrePostCallback(cp, "post_randomize");
                hasPost = സത്യം;
            }
        }
        // Ensure the static handle type owns the slot whenever a subclass
        // overrides, so the virtual call resolves on a base handle.
        എങ്കിൽ (hasPre) getCreatePrePostCallback(classp, "pre_randomize");
        എങ്കിൽ (hasPost) getCreatePrePostCallback(classp, "post_randomize");
        സ്ഥിരം std::pair<തർക്കം, തർക്കം> result{hasPre, hasPost};
        m_prePostWrap.emplace(classp, result);
        തിരികെ result;
    }
    ശൂന്യം addVirtualPrePostCall(AstFunc* സ്ഥിരം randomizeFuncp, AstClass* സ്ഥിരം classp,
                               സ്ഥിരം string& which) {
        File_വരി* സ്ഥിരം fl = classp->fileline();
        AstTask* സ്ഥിരം wrapperp = getCreatePrePostCallback(classp, which);
        AstClassRefDType* സ്ഥിരം refDTypep = പുതിയ AstClassRefDType{fl, classp, nullptr};
        v3Global.rootp()->typeTablep()->addTypesp(refDTypep);
        AstMethodCall* സ്ഥിരം callp
            = പുതിയ AstMethodCall{fl, പുതിയ AstThisRef{fl, refDTypep}, wrapperp->name(), nullptr};
        callp->taskp(wrapperp);
        callp->dtypeSetVoid();
        randomizeFuncp->addStmtsp(callp->makeStmt());
    }
    // Check if a class (including inherited members) has any rand class-type members
    തർക്കം classHasRandClassMembers(AstClass* classp) {
        തിരികെ classp->existsMember([](സ്ഥിരം AstClass*, സ്ഥിരം AstVar* varp) {
            എങ്കിൽ (!varp->rand().isRandomizable()) തിരികെ അസത്യം;
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->dtypep()->skipRefp();
            തിരികെ VN_IS(dtypep, ClassRefDType);
        });
    }
    // True if this class owns a global constraint: a member-select chain rooted
    // at a rand class-typed handle reaching into a sub-object.
    തർക്കം classOwnsGlobalConstraint(സ്ഥിരം AstClass* classp) സ്ഥിരം {
        തിരികെ classp->existsMember([](സ്ഥിരം AstClass*, സ്ഥിരം AstConstraint* constrp) {
            തർക്കം owns = അസത്യം;
            constrp->foreach([&](സ്ഥിരം AstMemberSel* memberSelp) {
                സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം rootp
                    = const_cast<AstMemberSel*>(memberSelp)->baseFromp(സത്യം);
                എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(rootp, VarRef)) {
                    // Owns a sub-object constraint if the root is a class handle, or
                    // an array/assoc/queue/unpacked array of class handles.
                    സ്ഥിരം Ast_കണ്ണി_DType* dtypep = refp->varp()->dtypep()->skipRefp();
                    ആകുമ്പോൾ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം subp = dtypep->subDTypep())
                        dtypep = subp->skipRefp();
                    എങ്കിൽ (VN_IS(dtypep, ClassRefDType)) owns = സത്യം;
                }
            });
            തിരികെ owns;
        });
    }
    // Get or create __VrandCb_pre/__VrandCb_post task for nested callbacks
    AstTask* getCreateNestedCallbackTask(AstClass* classp, സ്ഥിരം string& suffix) {
        സ്ഥിരം string name = "__VrandCb_" + suffix;
        AstTask* taskp = VN_CAST(m_memberMap.findMember(classp, name), Task);
        എങ്കിൽ (taskp) തിരികെ taskp;
        taskp = പുതിയ AstTask{classp->fileline(), name, nullptr};
        taskp->classMethod(സത്യം);
        classp->addMembersp(taskp);
        m_memberMap.തിരുകുക(classp, taskp);
        തിരികെ taskp;
    }
    // Populate nested callback task body: calls pre/post_randomize on nested rand class members
    ശൂന്യം populateNestedCallbackTask(AstTask* സ്ഥിരം callbackTaskp, AstClass* സ്ഥിരം classp,
                                    സ്ഥിരം string& cbName) {
        File_വരി* സ്ഥിരം fl = classp->fileline();
        classp->foreachMember([&](AstClass* ownerClassp, AstVar* memberVarp) {
            എങ്കിൽ (!memberVarp->rand().isRandomizable()) തിരികെ;
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = memberVarp->dtypep()->skipRefp();
            സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(dtypep, ClassRefDType);
            എങ്കിൽ (!classRefp) തിരികെ;
            AstClass* സ്ഥിരം memberClassp = classRefp->classp();
            എങ്കിൽ (memberClassp == classp) തിരികെ;  // Avoid self-reference

            // Force-visit member class if not yet processed
            എങ്കിൽ (memberClassp->user1()) {
                iterate(memberClassp);
                m_writtenVars.clear();
            }

            Ast_കണ്ണി* stmtsp = nullptr;

            // 1. Call member.pre/post_randomize() if exists in hierarchy
            എങ്കിൽ (AstTask* സ്ഥിരം userFuncp = findPrePostTask(memberClassp, cbName)) {
                AstMethodCall* സ്ഥിരം callp = പുതിയ AstMethodCall{
                    fl, പുതിയ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE}, cbName};
                callp->taskp(userFuncp);
                callp->dtypeSetVoid();
                stmtsp = Ast_കണ്ണി::addNext(stmtsp, callp->makeStmt());
            }

            // 2. Call member.__VrandCb_pre/post() for deeper recursion
            എങ്കിൽ (classHasRandClassMembers(memberClassp)) {
                സ്ഥിരം string suffix = (cbName == "pre_randomize") ? "pre" : "post";
                AstTask* സ്ഥിരം nestedTaskp = getCreateNestedCallbackTask(memberClassp, suffix);
                AstMethodCall* സ്ഥിരം recurseCallp = പുതിയ AstMethodCall{
                    fl, പുതിയ AstVarRef{fl, ownerClassp, memberVarp, VAccess::WRITE},
                    nestedTaskp->name()};
                recurseCallp->taskp(nestedTaskp);
                recurseCallp->dtypeSetVoid();
                stmtsp = Ast_കണ്ണി::addNext(stmtsp, recurseCallp->makeStmt());
            }

            എങ്കിൽ (!stmtsp) തിരികെ;

            // Wrap in null check
            AstIf* സ്ഥിരം nullCheckp = പുതിയ AstIf{
                fl,
                പുതിയ AstNeq{fl, പുതിയ AstVarRef{fl, ownerClassp, memberVarp, VAccess::READ},
                           പുതിയ AstConst{fl, AstConst::Null{}}},
                stmtsp};

            // Wrap in rand_mode check
            callbackTaskp->addStmtsp(wrapIfRandMode(classp, memberVarp, nullCheckp));
        });
    }
    AstTask* newSetupConstraintTask(AstClass* സ്ഥിരം nodep, സ്ഥിരം std::string& name) {
        AstTask* സ്ഥിരം taskp = പുതിയ AstTask{nodep->fileline(), name + "_setup_constraint", nullptr};
        taskp->classMethod(സത്യം);
        nodep->addMembersp(taskp);
        തിരികെ taskp;
    }
    AstTask* newResizeConstrainedArrayTask(AstClass* സ്ഥിരം nodep, സ്ഥിരം std::string& name) {
        AstTask* സ്ഥിരം taskp
            = പുതിയ AstTask{nodep->fileline(), name + "_resize_constrained_array", nullptr};
        taskp->classMethod(സത്യം);
        nodep->addMembersp(taskp);
        തിരികെ taskp;
    }
    Ast_കണ്ണി_Stmt* implementConstraintsClear(File_വരി* സ്ഥിരം fileline, AstVar* സ്ഥിരം genp) {
        AstCMethodHard* സ്ഥിരം clearp = പുതിയ AstCMethodHard{
            fileline,
            പുതിയ AstVarRef{fileline, VN_AS(genp->user2p(), കണ്ണി_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARCONSTRAINTS};
        clearp->dtypeSetVoid();
        തിരികെ clearp->makeStmt();
    }
    Ast_കണ്ണി_Stmt* implementConstraintsClearAll(File_വരി* സ്ഥിരം fileline, AstVar* സ്ഥിരം genp) {
        AstCMethodHard* സ്ഥിരം clearp = പുതിയ AstCMethodHard{
            fileline,
            പുതിയ AstVarRef{fileline, VN_AS(genp->user2p(), കണ്ണി_Module), genp, VAccess::READWRITE},
            VCMethod::RANDOMIZER_CLEARALL};
        clearp->dtypeSetVoid();
        തിരികെ clearp->makeStmt();
    }
    AstVar* getVarFromRef(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp) {
        എങ്കിൽ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(exprp, MemberSel)) {
            തിരികെ memberSelp->varp();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(exprp, VarRef)) {
            തിരികെ varrefp->varp();
        }
        exprp->v3fatalSrc("Not a MemberSel nor VarRef");
        തിരികെ nullptr;  // LCOV_EXCL_LINE
    }
    // Build a reference to a rand_mode/constraint_mode dyn-array.
    // Static-mode vars live on the class package; V3Scope resolves the VarRef later.
    Ast_കണ്ണി_വ്യഞ്ജകം* makeModeVarRef(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp, AstVar* സ്ഥിരം modeVarp,
                                സ്ഥിരം VAccess access) {
        എങ്കിൽ (modeVarp->lifetime().isStatic()) {
            തിരികെ പുതിയ AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), കണ്ണി_Module),
                                 modeVarp, access};
        }
        എങ്കിൽ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(exprp, MemberSel)) {
            AstMemberSel* സ്ഥിരം newMemberSelp = പുതിയ AstMemberSel{
                exprp->fileline(), memberSelp->fromp()->clone_വൃക്ഷം(അസത്യം), modeVarp};
            newMemberSelp->foreach([access](AstVarRef* varrefp) { varrefp->access(access); });
            തിരികെ newMemberSelp;
        }
        UASSERT_OBJ(VN_IS(exprp, VarRef), exprp, "Should be a VarRef");
        തിരികെ പുതിയ AstVarRef{exprp->fileline(), VN_AS(modeVarp->user2p(), Class), modeVarp,
                             access};
    }
    // Get or create a size variable for a constrained dynamic/queue/assoc array.
    // Returns the size variable. Sets wasCreated=true if a new variable was made.
    AstVar* createOrGetSizeVar(AstClass* സ്ഥിരം classp, AstVar* സ്ഥിരം arrVarp, File_വരി* സ്ഥിരം fl,
                               Ast_കണ്ണി_DType* സ്ഥിരം signed32DTypep, തർക്കം& wasCreated) {
        AstVar* sizeVarp = VN_CAST(arrVarp->user4p(), Var);
        wasCreated = അസത്യം;
        എങ്കിൽ (!sizeVarp) {
            sizeVarp = പുതിയ AstVar{fl, VVarType::MEMBER, "__V" + arrVarp->name() + "_size",
                                  signed32DTypep};
            classp->addMembersp(sizeVarp);
            m_memberMap.തിരുകുക(classp, sizeVarp);
            sizeVarp->user2p(classp);
            arrVarp->user4p(sizeVarp);
            wasCreated = സത്യം;
        }
        തിരികെ sizeVarp;
    }
    // Build a size >= 0 constraint expression (signed int size must be non-negative).
    // Returns a new AstConstraintExpr with user1 bits set for solver visibility.
    Ast_Constraint_വ്യഞ്ജകം* createSizeGteZeroConstraint(File_വരി* സ്ഥിരം fl, AstVar* സ്ഥിരം sizeVarp,
                                                   Ast_കണ്ണി_വ്യഞ്ജകം* derefFromp = nullptr) {
        Ast_കണ്ണി_വ്യഞ്ജകം* size_കണ്ണി_Exprp = nullptr;
        എങ്കിൽ (derefFromp) {
            size_കണ്ണി_Exprp = buildMemberSelExprp(derefFromp, sizeVarp);
        } അല്ലെങ്കിൽ {
            size_കണ്ണി_Exprp = പുതിയ AstVarRef{fl, sizeVarp, VAccess::READ};
        }
        size_കണ്ണി_Exprp->user1(സത്യം);
        AstGteS* സ്ഥിരം sizeGtep = പുതിയ AstGteS{fl, size_കണ്ണി_Exprp, പുതിയ AstConst{fl, 0}};
        sizeGtep->user1(സത്യം);
        തിരികെ പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, sizeGtep};
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* getFromp(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp) {
        എങ്കിൽ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(exprp, MemberSel)) {
            തിരികെ memberSelp->fromp();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(exprp, MethodCall)) {
            തിരികെ methodCallp->fromp();
        }
        തിരികെ nullptr;
    }
    AstVar* makeTmpRandModeVar(Ast_കണ്ണി_വ്യഞ്ജകം* siblingExprp, AstVar* randModeVarp,
                               Ast_കണ്ണി*& storeStmtspr, Ast_കണ്ണി_Stmt*& restoreStmtspr) {
        File_വരി* സ്ഥിരം fl = randModeVarp->fileline();
        AstVar* സ്ഥിരം randModeTmpVarp = പുതിയ AstVar{
            fl, VVarType::BLOCKTEMP, m_modeUniqueNames.get(randModeVarp), randModeVarp->dtypep()};
        randModeTmpVarp->funcLocal(m_ftaskp);
        randModeTmpVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        storeStmtspr = Ast_കണ്ണി::addNext(
            storeStmtspr,
            പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, randModeTmpVarp, VAccess::WRITE},
                          makeModeVarRef(siblingExprp, randModeVarp, VAccess::READ)});
        storeStmtspr = Ast_കണ്ണി::addNext(
            storeStmtspr,
            makeModeSetLoop(fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                            പുതിയ AstConst{fl, 0}, m_ftaskp));
        restoreStmtspr = Ast_കണ്ണി::addNext(
            restoreStmtspr,
            പുതിയ AstAssign{fl, makeModeVarRef(siblingExprp, randModeVarp, VAccess::WRITE),
                          പുതിയ AstVarRef{fl, randModeTmpVarp, VAccess::READ}});
        തിരികെ randModeTmpVarp;
    }
    // Returns the common prefix of two hierarchical accesses, or nullptr if there is none
    // e.g. a.b.c and a.b.d -> a.b
    Ast_കണ്ണി_വ്യഞ്ജകം* sliceToCommonPrefix(Ast_കണ്ണി_വ്യഞ്ജകം* thisp, Ast_കണ്ണി_വ്യഞ്ജകം* otherp) {
        സ്ഥിതം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> thisHier, otherHier;  // Keep around
                                                               // to avoid reallocations
        thisHier.clear();
        otherHier.clear();
        ആകുമ്പോൾ (thisp) {
            thisHier.തള്ളുക_back(thisp);
            thisp = getFromp(thisp);
        }
        ആകുമ്പോൾ (otherp) {
            otherHier.തള്ളുക_back(otherp);
            otherp = getFromp(otherp);
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* commonp = nullptr;
        വേണ്ടി (auto thisIt = thisHier.rbegin(), otherIt = otherHier.rbegin();
             thisIt != thisHier.rend() && otherIt != otherHier.rend(); ++thisIt, ++otherIt) {
            എങ്കിൽ ((*thisIt)->type() != (*otherIt)->type()) നിർത്തുക;
            എങ്കിൽ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(*thisIt, MemberSel)) {
                AstMemberSel* സ്ഥിരം otherMemberSelp = VN_AS(*otherIt, MemberSel);
                എങ്കിൽ (memberSelp->varp() == otherMemberSelp->varp()) {
                    commonp = memberSelp;
                    തുടരുക;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstMethodCall* സ്ഥിരം thisMethodCallp = VN_CAST(*thisIt, MethodCall)) {
                AstMethodCall* സ്ഥിരം otherMethodCallp = VN_AS(*otherIt, MethodCall);
                എങ്കിൽ (thisMethodCallp->taskp() == otherMethodCallp->taskp()) {
                    commonp = thisMethodCallp;
                    തുടരുക;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം firstVarRefp = VN_CAST(*thisIt, VarRef)) {
                AstVarRef* സ്ഥിരം secondVarRefp = VN_AS(*otherIt, VarRef);
                എങ്കിൽ (firstVarRefp->varp() == secondVarRefp->varp()) {
                    commonp = firstVarRefp;
                    തുടരുക;
                }
            }
            നിർത്തുക;
        }
        തിരികെ commonp;
    }
    // Returns MemberSel chain, with deepest VarRef being reference to deepestVarp
    Ast_കണ്ണി_വ്യഞ്ജകം* buildMemberSelExprp(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstVar* deepestVarp) {
        UASSERT_OBJ(VN_IS(exprp, MemberSel) || VN_IS(exprp, VarRef), exprp,
                    "Should be MemberSel or VarRef");

        Ast_കണ്ണി_വ്യഞ്ജകം* resExprp = nullptr;
        എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(exprp, MemberSel)) {
            resExprp = buildMemberSelExprp(memberSelp->fromp(), nullptr);
            AstVar* varp = memberSelp->varp();
            എങ്കിൽ (deepestVarp) varp = deepestVarp;
            resExprp = പുതിയ AstMemberSel{exprp->fileline(), resExprp, varp};
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം varRefp = VN_CAST(exprp, VarRef)) {
            AstVar* varp = varRefp->varp();
            എങ്കിൽ (deepestVarp) varp = deepestVarp;
            resExprp = പുതിയ AstVarRef{exprp->fileline(), VN_AS(varp->user2p(), കണ്ണി_Module), varp,
                                     VAccess::READWRITE};
        }

        resExprp->user1(സത്യം);
        തിരികെ resExprp;
    }

    ശൂന്യം addUpdateRandVarsBody(AstClass* സ്ഥിരം nodep, AstFunc* സ്ഥിരം randomizep) {
        UASSERT_OBJ(!nodep->hasRandVarsUpdate(), nodep, "__VupdateRandVars already exists");
        AstFunc* സ്ഥിരം updatep
            = പുതിയ AstFunc{nodep->fileline(), "__VupdateRandVars", nullptr, nullptr};
        updatep->classMethod(സത്യം);
        updatep->isVirtual(സത്യം);
        updatep->keepAlive(സത്യം);
        updatep->needsSyms(അസത്യം);
        nodep->addMembersp(updatep);
        nodep->hasRandVarsUpdate(സത്യം);

        വേണ്ടി (AstClass* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            Ast_കണ്ണി_FTask* സ്ഥിരം newp = VN_AS(m_memberMap.findMember(classp, "new"), കണ്ണി_FTask);
            വേണ്ടി (Ast_കണ്ണി* stmtp = newp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം stmtExprp = VN_CAST(stmtp, Stmt_വ്യഞ്ജകം);
                എങ്കിൽ (!stmtExprp) തുടരുക;
                AstCMethodHard* സ്ഥിരം methodp = VN_CAST(stmtExprp->exprp(), CMethodHard);
                എങ്കിൽ (!methodp || methodp->method() != VCMethod::RANDOMIZER_WRITE_VAR) തുടരുക;

                Ast_കണ്ണി_Var_Ref* സ്ഥിരം varRefp = VN_AS(methodp->pinsp(), കണ്ണി_Var_Ref);
                എങ്കിൽ (varRefp->varp()->lifetime().isStatic()) {
                    // There is no need to update pointer to a static variable
                    തുടരുക;
                }
                AstCMethodHard* സ്ഥിരം updateMethodp = പുതിയ AstCMethodHard{
                    methodp->fileline(), methodp->fromp()->clone_വൃക്ഷം_Pure(അസത്യം),
                    VCMethod::RANDOMIZER_UPDATE_VAR, varRefp->clone_വൃക്ഷം_Pure(അസത്യം)};
                updateMethodp->dtypeSetVoid();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം namep = VN_AS(varRefp->nextp()->nextp(), കണ്ണി_വ്യഞ്ജകം);
                updateMethodp->addPinsp(namep->clone_വൃക്ഷം_Pure(അസത്യം));
                updateMethodp->method(VCMethod::RANDOMIZER_UPDATE_VAR);
                updatep->addStmtsp(updateMethodp->makeStmt());
            }
        }
    }

    ശൂന്യം addBasicRandomizeBody(AstFunc* സ്ഥിരം basicRandomizep, AstClass* സ്ഥിരം nodep,
                               AstVar* randModeVarp) {
        UINFO(9, "addBasicRTB " << nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        AstVar* സ്ഥിരം basicFvarp = VN_AS(basicRandomizep->fvarp(), Var);
        AstVarRef* സ്ഥിരം basicFvarRefp = പുതിയ AstVarRef{fl, basicFvarp, VAccess::WRITE};
        AstConst* സ്ഥിരം beginBasicValp = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 1};
        basicRandomizep->addStmtsp(പുതിയ AstAssign{fl, basicFvarRefp, beginBasicValp});
        Ast_കണ്ണി_FTask* സ്ഥിരം newp = VN_AS(m_memberMap.findMember(nodep, "new"), കണ്ണി_FTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        nodep->foreachMember([&](AstClass* classp, AstVar* memberVarp) {
            എങ്കിൽ (!memberVarp->rand().isRandomizable()) തിരികെ;
            സ്ഥിരം RandomizeMode randMode = {.asUQuad = memberVarp->user1()};
            എങ്കിൽ (randMode.usesMode
                && !memberVarp->rand().isRand()) {  // Not randomizable by default
                AstCMethodHard* സ്ഥിരം setp = പുതിയ AstCMethodHard{
                    nodep->fileline(),
                    പുതിയ AstVarRef{fl, VN_AS(randModeVarp->user2p(), കണ്ണി_Module), randModeVarp,
                                  VAccess::WRITE},
                    VCMethod::ARRAY_AT_WRITE, പുതിയ AstConst{nodep->fileline(), randMode.index}};
                setp->dtypeSetUInt32();
                newp->addStmtsp(പുതിയ AstAssign{fl, setp, പുതിയ AstConst{fl, 0}});
            }
            എങ്കിൽ (memberVarp->user3()) തിരികെ;  // Handled in constraints
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = memberVarp->dtypep()->skipRefp();
            എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(dtypep, ClassRefDType)) {
                എങ്കിൽ (classRefp->classp() == nodep) {
                    memberVarp->v3warn(E_UNSUPPORTED,
                                       "Unsupported: random member variable with the "
                                       "type of the containing class");
                    തിരികെ;
                }
                AstFunc* സ്ഥിരം memberFuncp
                    = memberVarp->globalConstrained()
                          ? V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp(),
                                                          BASIC_RANDOMIZE_FUNC_NAME)
                          : V3Randomize::newRandomizeFunc(m_memberMap, classRefp->classp());
                AstMethodCall* സ്ഥിരം callp
                    = memberVarp->globalConstrained()
                          ? പുതിയ AstMethodCall{fl,
                                              പുതിയ AstVarRef{fl, classp, memberVarp,
                                                            VAccess::WRITE},
                                              BASIC_RANDOMIZE_FUNC_NAME}
                          : പുതിയ AstMethodCall{
                                fl, പുതിയ AstVarRef{fl, classp, memberVarp, VAccess::WRITE},
                                "randomize"};
                callp->taskp(memberFuncp);
                callp->dtypeFrom(memberFuncp);
                AstVarRef* സ്ഥിരം basicFvarRefReadp = basicFvarRefp->clone_വൃക്ഷം(അസത്യം);
                basicFvarRefReadp->access(VAccess::READ);
                AstIf* സ്ഥിരം assignIfNotNullp = പുതിയ AstIf{
                    fl,
                    പുതിയ AstNeq{fl, പുതിയ AstVarRef{fl, classp, memberVarp, VAccess::READ},
                               പുതിയ AstConst{fl, AstConst::Null{}}},
                    പുതിയ AstAssign{fl, basicFvarRefp->clone_വൃക്ഷം(അസത്യം),
                                  പുതിയ AstAnd{fl, basicFvarRefReadp, callp}}};
                basicRandomizep->addStmtsp(wrapIfRandMode(nodep, memberVarp, assignIfNotNullp));
            } അല്ലെങ്കിൽ {
                AstVar* സ്ഥിരം randcVarp = newRandcVarsp(memberVarp, nodep);
                AstVarRef* സ്ഥിരം refp
                    = പുതിയ AstVarRef{memberVarp->fileline(), classp, memberVarp, VAccess::WRITE};
                Ast_കണ്ണി_Stmt* സ്ഥിരം stmtp = newRandStmtsp(fl, refp, randcVarp, basicFvarp);
                എങ്കിൽ (!refp->backp()) VL_DO_DANGLING(refp->delete_വൃക്ഷം(), refp);
                basicRandomizep->addStmtsp(
                    പുതിയ AstBegin{fl, m_uniqueNames.get("__VrandBlock"), stmtp, അസത്യം});
            }
        });
    }

    // Creates a lvalue reference to the randomize mode var. Called by visit(AstNodeFTaskRef*)
    Ast_കണ്ണി_വ്യഞ്ജകം* makeModeAssignLhs(File_വരി* സ്ഥിരം fl, AstClass* സ്ഥിരം classp,
                                   Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp, AstVar* സ്ഥിരം modeVarp) {
        // For static constraint mode vars, always use VarRef (not MemberSel).
        // At this point V3Class hasn't run yet, so user2p may be nullptr.
        // Generate VarRef with classp as module; V3Scope will update varScopep later
        // when the variable is moved to the class package.
        എങ്കിൽ (modeVarp->lifetime().isStatic()) {
            // Hint owning class so V3Scope resolves from derived call sites.
            എങ്കിൽ (fromp) VL_DO_DANGLING(fromp->unlinkFrBack()->delete_വൃക്ഷം(), fromp);
            തിരികെ പുതിയ AstVarRef{fl, VN_AS(modeVarp->user2p(), കണ്ണി_Module), modeVarp,
                                 VAccess::WRITE};
        } അല്ലെങ്കിൽ എങ്കിൽ (classp == m_modp) {
            // Called on 'this' or a member of 'this'
            തിരികെ പുതിയ AstVarRef{fl, VN_AS(modeVarp->user2p(), കണ്ണി_Module), modeVarp,
                                 VAccess::WRITE};
        } അല്ലെങ്കിൽ {
            AstMemberSel* സ്ഥിരം memberselp = പുതിയ AstMemberSel{fl, fromp->unlinkFrBack(), modeVarp};
            memberselp->foreach([](AstVarRef* varrefp) { varrefp->access(VAccess::WRITE); });
            തിരികെ memberselp;
        }
    }
    // Replace the node with an assignment to the mode variable. Called by visit(AstNodeFTaskRef*)
    ശൂന്യം replaceWithModeAssign(Ast_കണ്ണി_FTask_Ref* സ്ഥിരം ftaskRefp, Ast_കണ്ണി* സ്ഥിരം receiverp,
                               Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp) {
        replaceWithModeAssignAndAppend(ftaskRefp, receiverp, lhsp, nullptr);
    }
    // Append BEFORE swap; backp()/nextp() unreliable after replaceWith.
    ശൂന്യം replaceWithModeAssignAndAppend(Ast_കണ്ണി_FTask_Ref* സ്ഥിരം ftaskRefp, Ast_കണ്ണി* സ്ഥിരം receiverp,
                                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp, Ast_കണ്ണി* സ്ഥിരം appendStmtp) {
        File_വരി* സ്ഥിരം fl = ftaskRefp->fileline();
        എങ്കിൽ (ftaskRefp->argsp()) {
            UASSERT_OBJ(VN_IS(ftaskRefp->backp(), Stmt_വ്യഞ്ജകം), ftaskRefp, "Should be a statement");
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = ftaskRefp->argsp()->exprp()->unlinkFrBack();
            Ast_കണ്ണി* newStmtp = nullptr;
            എങ്കിൽ (receiverp) {
                // Called on a rand member variable/constraint. Set the variable/constraint's
                // mode
                സ്ഥിരം RandomizeMode rmode = {.asUQuad = receiverp->user1()};
                UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
                AstCMethodHard* സ്ഥിരം setp = പുതിയ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                                പുതിയ AstConst{fl, rmode.index}};
                setp->dtypeSetUInt32();
                newStmtp = പുതിയ AstAssign{fl, setp, rhsp};
            } അല്ലെങ്കിൽ {
                // For rand_mode: Called on 'this' or a non-rand class instance.
                // For constraint_mode: Called on a class instance.
                // Set the rand mode of all members
                newStmtp = makeModeSetLoop(fl, lhsp, rhsp, m_ftaskp);
            }
            എങ്കിൽ (appendStmtp) newStmtp->addNext(appendStmtp);
            m_stmtp->replaceWith(newStmtp);
            VL_DO_CLEAR(തള്ളുക_Deletep(m_stmtp), m_stmtp = nullptr);
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(receiverp, ftaskRefp, "Should have receiver");
            UASSERT_OBJ(!appendStmtp, ftaskRefp, "Append path requires arg-form rand_mode");
            സ്ഥിരം RandomizeMode rmode = {.asUQuad = receiverp->user1()};
            UASSERT_OBJ(rmode.usesMode, ftaskRefp, "Failed to set usesMode");
            AstCMethodHard* സ്ഥിരം setp = പുതിയ AstCMethodHard{fl, lhsp, VCMethod::ARRAY_AT_WRITE,
                                                            പുതിയ AstConst{fl, rmode.index}};
            setp->dtypeSetUInt32();
            ftaskRefp->replaceWith(setp);
            VL_DO_DANGLING(തള്ളുക_Deletep(ftaskRefp), ftaskRefp);
        }
    };

    // Wrap obj.randomize() with null guard: (obj != null) ? obj.randomize() : 0
    // IEEE 1800 requires randomize() on null handle to return 0.
    // Uses user4() to prevent re-wrapping during iterateChildren.
    ശൂന്യം wrapRandomizeCallWithNullGuard(Ast_കണ്ണി_FTask_Ref* nodep) {
        AstMethodCall* സ്ഥിരം callp = VN_CAST(nodep, MethodCall);
        എങ്കിൽ (!callp) തിരികെ;
        എങ്കിൽ (callp->user4SetOnce()) തിരികെ;
        File_വരി* സ്ഥിരം fl = callp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം checkp
            = പുതിയ AstNeq{fl, callp->fromp()->clone_വൃക്ഷം(അസത്യം), പുതിയ AstConst{fl, AstConst::Null{}}};
        VNRelinker relinker;
        callp->unlinkFrBack(&relinker);
        relinker.relink(പുതിയ AstCond{fl, checkp, callp, പുതിയ AstConst{fl, 0}});
    }

    // Handle inline random variable control. After this, the randomize() call has no args
    ശൂന്യം handleRandomizeArgs(Ast_കണ്ണി_FTask_Ref* സ്ഥിരം nodep) {
        എങ്കിൽ (!nodep->argsp()) തിരികെ;
        // Strip the null literal arg. V3Width already rejected mixed/non-null
        // AstConst args.
        തർക്കം hasNullArg = അസത്യം;
        വേണ്ടി (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(argp->exprp(), Const)) {
                UASSERT_OBJ(constp->അക്കം().isNull(), constp,
                            "Non-null AstConst arg to randomize() should have been "
                            "rejected by V3Width");
                hasNullArg = സത്യം;
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);
            }
        }
        // This assumes arguments to always be a member sel from nodep->fromp(), if applicable
        // e.g. LinkDot transformed a.randomize(b, a.c) -> a.randomize(a.b, a.c)
        // Merge pins with common prefixes so that setting their rand mode doesn't interfere
        // with each other.
        // e.g. a.randomize(a.b, a.c, a.b.d) -> a.randomize(a.b, a.c)
        വേണ്ടി (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            വേണ്ടി (AstArg *otherArgp = nextp, *otherNextp = nullptr; otherArgp;
                 otherArgp = otherNextp) {
                otherNextp = VN_AS(otherArgp->nextp(), Arg);
                എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prefixp
                    = sliceToCommonPrefix(argp->exprp(), otherArgp->exprp())) {
                    എങ്കിൽ (prefixp == argp->exprp()) {
                        എങ്കിൽ (nextp == otherArgp) nextp = VN_AS(nextp->nextp(), Arg);
                        VL_DO_DANGLING(otherArgp->unlinkFrBack()->delete_വൃക്ഷം(), otherArgp);
                        തുടരുക;
                    }
                }
                എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prefixp
                    = sliceToCommonPrefix(otherArgp->exprp(), argp->exprp())) {
                    എങ്കിൽ (prefixp == otherArgp->exprp()) {
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);
                        നിർത്തുക;
                    }
                }
            }
        }
        // Construct temp vars, and store and restore statements
        std::set<AstVar*> savedRandModeVarps;
        AstVar* tmpVarps = nullptr;
        Ast_കണ്ണി* storeStmtsp = nullptr;
        Ast_കണ്ണി* setStmtsp = nullptr;
        Ast_കണ്ണി_Stmt* restoreStmtsp = nullptr;
        വേണ്ടി (AstArg *argp = nodep->argsp(), *nextp = nullptr; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_കണ്ണി_വ്യഞ്ജകം* exprp = argp->exprp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം commonPrefixp = sliceToCommonPrefix(exprp, nodep);
            UASSERT_OBJ(commonPrefixp != exprp, nodep,
                        "Common prefix should be different than pin");
            File_വരി* സ്ഥിരം fl = argp->fileline();
            ആകുമ്പോൾ (exprp) {
                എങ്കിൽ (commonPrefixp == exprp) നിർത്തുക;
                AstVar* സ്ഥിരം randVarp = getVarFromRef(exprp);
                AstClass* സ്ഥിരം classp = VN_AS(randVarp->user2p(), Class);
                AstVar* സ്ഥിരം randModeVarp = randVarp->lifetime().isStatic()
                                                 ? getStaticRandModeVar(classp)
                                                 : getRandModeVarFromClass(classp);
                UASSERT_OBJ(randModeVarp, randVarp,
                            "Rand var with rand_mode must have a mode array");
                എങ്കിൽ (savedRandModeVarps.find(randModeVarp) == savedRandModeVarps.end()) {
                    AstVar* സ്ഥിരം randModeTmpVarp
                        = makeTmpRandModeVar(exprp, randModeVarp, storeStmtsp, restoreStmtsp);
                    savedRandModeVarps.തിരുകുക(randModeVarp);
                    tmpVarps = Ast_കണ്ണി::addNext(tmpVarps, randModeTmpVarp);
                }
                സ്ഥിരം RandomizeMode randMode = {.asUQuad = randVarp->user1()};
                AstCMethodHard* setp = പുതിയ AstCMethodHard{
                    fl, makeModeVarRef(exprp, randModeVarp, VAccess::WRITE),
                    VCMethod::ARRAY_AT_WRITE, പുതിയ AstConst{fl, randMode.index}};
                setp->dtypeSetUInt32();
                setStmtsp
                    = Ast_കണ്ണി::addNext(setStmtsp, പുതിയ AstAssign{fl, setp, പുതിയ AstConst{fl, 1}});
                exprp = getFromp(exprp);
            }
            VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);
        }
        എങ്കിൽ (hasNullArg) {  // Re-point to the per-class __Vrandomize_null wrapper
            AstClass* targetClassp = nullptr;
            AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(nodep, MethodCall);
            എങ്കിൽ (methodCallp) {
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDTypep = methodCallp->fromp()->dtypep();
                സ്ഥിരം AstClassRefDType* സ്ഥിരം crdtp
                    = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
                UASSERT_OBJ(crdtp, nodep, "randomize(null) receiver is not a class type");
                targetClassp = crdtp->classp();
            } അല്ലെങ്കിൽ {
                targetClassp = VN_CAST(m_modp, Class);
            }
            UASSERT_OBJ(targetClassp, nodep, "randomize(null) target class unresolved");
            AstFunc* സ്ഥിരം checkOnlyFuncp = getCreateRandomizeNullFunc(targetClassp);
            nodep->name(checkOnlyFuncp->name());
            nodep->taskp(checkOnlyFuncp);
            nodep->dtypeFrom(checkOnlyFuncp->dtypep());
        }
        എങ്കിൽ (tmpVarps) {
            UASSERT_OBJ(storeStmtsp && setStmtsp && restoreStmtsp, nodep, "Should have stmts");
            VNRelinker relinker;
            m_stmtp->unlinkFrBack(&relinker);
            Ast_കണ്ണി* സ്ഥിരം stmtsp = storeStmtsp;
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
    AstFunc* getCreateRandomizeNullFunc(AstClass* സ്ഥിരം classp) {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം name = "__Vrandomize_null";
        എങ്കിൽ (AstFunc* സ്ഥിരം existingp = VN_AS(m_memberMap.findMember(classp, name), Func)) {
            തിരികെ existingp;
        }
        File_വരി* സ്ഥിരം fl = classp->fileline();
        AstFunc* സ്ഥിരം funcp = V3Randomize::newRandomizeFunc(m_memberMap, classp, name);
        AstVar* സ്ഥിരം fvarp = VN_AS(funcp->fvarp(), Var);

        // 1. pre_randomize -- always
        addPrePostCall(classp, funcp, "pre_randomize");

        // 2. Compute result
        AstVar* സ്ഥിരം classGenp = getRandomGenerator(classp);
        എങ്കിൽ (!classGenp) {
            funcp->addStmtsp(പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, fvarp, VAccess::WRITE},
                                           പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 1}});
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_Module* സ്ഥിരം genModp = VN_AS(classGenp->user2p(), കണ്ണി_Module);
            funcp->addStmtsp(implementConstraintsClear(fl, classGenp));
            AstTask* സ്ഥിരം setupAllTaskp = getCreateConstraintSetupFunc(classp, m_memberMap);
            funcp->addStmtsp((പുതിയ AstTaskRef{fl, setupAllTaskp})->makeStmt());
            AstCExpr* സ്ഥിരം solverCallp = പുതിയ AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(പുതിയ AstVarRef{fl, genModp, classGenp, VAccess::READWRITE});
            solverCallp->add(".next_check_only(__Vm_rng)");
            funcp->addStmtsp(
                പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, fvarp, VAccess::WRITE}, solverCallp});
            classp->needRNG(സത്യം);
        }

        // 3. post_randomize -- only if result is non-zero
        എങ്കിൽ (AstTask* സ്ഥിരം userPostp = findPrePostTask(classp, "post_randomize")) {
            AstTaskRef* സ്ഥിരം callp = പുതിയ AstTaskRef{userPostp->fileline(), userPostp};
            funcp->addStmtsp(
                പുതിയ AstIf{fl, പുതിയ AstVarRef{fl, fvarp, VAccess::READ}, callp->makeStmt()});
        }

        തിരികെ funcp;
    }

    // Rewrite a LogIf-of-Dist chain into nested AstConstraintIf. The outermost
    // AstLogIf shell is left for the caller's AstConstraintExpr to free; inner
    // shells are deleted here once their children are transplanted.
    AstConstraintIf* liftLogIfChainToConstraintIf(AstLogIf* logIfp) {
        File_വരി* സ്ഥിരം fl = logIfp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = logIfp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = logIfp->rhsp()->unlinkFrBack();
        Ast_കണ്ണി* thenBodyp;
        എങ്കിൽ (AstLogIf* സ്ഥിരം innerLogIfp = VN_CAST(rhsp, LogIf)) {
            thenBodyp = liftLogIfChainToConstraintIf(innerLogIfp);
            VL_DO_DANGLING(തള്ളുക_Deletep(innerLogIfp), innerLogIfp);
        } അല്ലെങ്കിൽ {
            thenBodyp = പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, rhsp};
        }
        തിരികെ പുതിയ AstConstraintIf{fl, condp, thenBodyp, nullptr};
    }

    സ്ഥിതം തർക്കം distBoundRefsRandVar(സ്ഥിരം Ast_കണ്ണി* boundp) {
        തിരികെ boundp->exists(
            [](സ്ഥിരം AstVarRef* vrefp) { തിരികെ vrefp->varp()->rand().isRandomizable(); });
    }

    സ്ഥിതം തർക്കം distBoundRefsModeVar(സ്ഥിരം Ast_കണ്ണി* boundp) {
        തിരികെ boundp->exists([](സ്ഥിരം AstVarRef* vrefp) {
            എങ്കിൽ (!vrefp->varp()->rand().isRandomizable()) തിരികെ അസത്യം;
            സ്ഥിരം RandomizeMode rmode = {.asUQuad = vrefp->varp()->user1()};
            തിരികെ rmode.usesMode;
        });
    }

    // (distExpr >= lo) && (distExpr <= hi); signed comparisons for signed vars
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newDistRangeMembership(AstDist* distp, സ്ഥിരം AstInsideRange* irp) {
        File_വരി* സ്ഥിരം fl = distp->fileline();
        സ്ഥിരം തർക്കം isSigned = distp->exprp()->isSigned();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dist_വ്യഞ്ജകം_Gtep = distp->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dist_വ്യഞ്ജകം_Ltep = distp->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
        dist_വ്യഞ്ജകം_Gtep->user1(സത്യം);
        dist_വ്യഞ്ജകം_Ltep->user1(സത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം gep
            = isSigned ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                             പുതിയ AstGteS{fl, dist_വ്യഞ്ജകം_Gtep, irp->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം)})
                       : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                             പുതിയ AstGte{fl, dist_വ്യഞ്ജകം_Gtep, irp->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം)});
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lep
            = isSigned ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                             പുതിയ AstLteS{fl, dist_വ്യഞ്ജകം_Ltep, irp->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം)})
                       : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                             പുതിയ AstLte{fl, dist_വ്യഞ്ജകം_Ltep, irp->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം)});
        gep->user1(സത്യം);
        lep->user1(സത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം andp = പുതിയ AstLogAnd{fl, gep, lep};
        andp->user1(സത്യം);
        തിരികെ andp;
    }

    ഘടന DistBucket final {
        Ast_കണ്ണി_വ്യഞ്ജകം* rangep;  // A single value or an InsideRange
        Ast_കണ്ണി_വ്യഞ്ജകം* weightExprp;  // Effective 64-bit weight (range weight scaled by size)
    };

    // Non-zero-weight buckets with each weight extended to 64 bits.
    std::vector<DistBucket> collectDistBuckets(AstDist* സ്ഥിരം distp) {
        File_വരി* സ്ഥിരം fl = distp->fileline();
        std::vector<DistBucket> buckets;
        വേണ്ടി (AstDistItem* ditemp = distp->itemsp(); ditemp;
             ditemp = VN_AS(ditemp->nextp(), DistItem)) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(ditemp->weightp(), Const)) {
                എങ്കിൽ (constp->toUQuad() == 0) തുടരുക;
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* weightExprp
                = പുതിയ AstExtend{fl, ditemp->weightp()->clone_വൃക്ഷം_Pure(അസത്യം), 64};
            // := on a range weights every element, so scale by the range size.
            എങ്കിൽ (!ditemp->isWhole()) {
                എങ്കിൽ (സ്ഥിരം AstInsideRange* സ്ഥിരം irp = VN_CAST(ditemp->rangep(), InsideRange)) {
                    സ്ഥിരം AstConst* സ്ഥിരം lop = VN_CAST(irp->lhsp(), Const);
                    സ്ഥിരം AstConst* സ്ഥിരം hip = VN_CAST(irp->rhsp(), Const);
                    Ast_കണ്ണി_വ്യഞ്ജകം* rangeSizep;
                    എങ്കിൽ (lop && hip) {
                        സ്ഥിരം uint64_t rangeSize = hip->toUQuad() - lop->toUQuad() + 1;
                        rangeSizep = പുതിയ AstConst{fl, AstConst::Unsized64{}, rangeSize};
                    } അല്ലെങ്കിൽ {
                        rangeSizep = പുതിയ AstAdd{
                            fl, പുതിയ AstConst{fl, AstConst::Unsized64{}, 1},
                            പുതിയ AstSub{fl,
                                       പുതിയ AstExtend{fl, irp->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം), 64},
                                       പുതിയ AstExtend{fl, irp->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം), 64}}};
                    }
                    weightExprp = പുതിയ AstMul{fl, weightExprp, rangeSizep};
                }
            }
            buckets.തള്ളുക_back({ditemp->rangep(), weightExprp});
        }
        തിരികെ buckets;
    }

    // Membership test for one bucket: a range comparison or an equality.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newDistMembershipTerm(AstDist* distp, Ast_കണ്ണി_വ്യഞ്ജകം* rangep) {
        എങ്കിൽ (സ്ഥിരം AstInsideRange* സ്ഥിരം irp = VN_CAST(rangep, InsideRange)) {
            തിരികെ newDistRangeMembership(distp, irp);
        }
        File_വരി* സ്ഥിരം fl = distp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eqExprp = distp->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
        eqExprp->user1(സത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eqp = പുതിയ AstEq{fl, eqExprp, rangep->clone_വൃക്ഷം_Pure(അസത്യം)};
        eqp->user1(സത്യം);
        തിരികെ eqp;
    }

    // Hard constraint that the dist value stays inside the union of its ranges
    // (IEEE 1800-2023 18.5.3: values outside the set must never appear).
    Ast_Constraint_വ്യഞ്ജകം* buildDistMembership(AstDist* distp,
                                           സ്ഥിരം std::vector<DistBucket>& buckets) {
        File_വരി* സ്ഥിരം fl = distp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* unionExprp = nullptr;
        വേണ്ടി (സ്ഥിരം auto& bucket : buckets) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം memberp = newDistMembershipTerm(distp, bucket.rangep);
            എങ്കിൽ (!unionExprp) {
                unionExprp = memberp;
            } അല്ലെങ്കിൽ {
                unionExprp = പുതിയ AstLogOr{fl, memberp, unionExprp};
                unionExprp->user1(സത്യം);
            }
        }
        തിരികെ പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, unionExprp};
    }

    // Gate junction with constants folded and identical terms merged.
    Ast_കണ്ണി_വ്യഞ്ജകം* newGateJoin(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* ap, Ast_കണ്ണി_വ്യഞ്ജകം* bp, തർക്കം isAnd) {
        എങ്കിൽ (ap->same_വൃക്ഷം(bp)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(bp), bp);
            തിരികെ ap;
        }
        എങ്കിൽ (VN_IS(bp, Const)) std::swap(ap, bp);
        എങ്കിൽ (VN_IS(ap, Const)) {
            സ്ഥിരം തർക്കം constWins = VN_AS(ap, Const)->isZero() == isAnd;
            എങ്കിൽ (!constWins) std::swap(ap, bp);
            VL_DO_DANGLING(തള്ളുക_Deletep(bp), bp);
            തിരികെ ap;
        }
        എങ്കിൽ (isAnd) തിരികെ പുതിയ AstLogAnd{fl, ap, bp};
        തിരികെ പുതിയ AstLogOr{fl, ap, bp};
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* newGateOr(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* ap, Ast_കണ്ണി_വ്യഞ്ജകം* bp) {
        തിരികെ newGateJoin(fl, ap, bp, അസത്യം);
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* newGateAnd(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* ap, Ast_കണ്ണി_വ്യഞ്ജകം* bp) {
        തിരികെ newGateJoin(fl, ap, bp, സത്യം);
    }

    // The rand_mode bit of varp: static class var, owner object's array via the
    // member select, or this object's array.
    Ast_കണ്ണി_വ്യഞ്ജകം* newModeBitRead(AstVar* varp, സ്ഥിരം AstMemberSel* mselp, AstVar* randModeVarp,
                                File_വരി* fl) {
        Ast_കണ്ണി_വ്യഞ്ജകം* arrayp = nullptr;
        എങ്കിൽ (varp->lifetime().isStatic()) {
            AstClass* സ്ഥിരം ownerp = VN_CAST(varp->user2p(), Class);
            UASSERT_OBJ(ownerp, varp, "usesMode static var without an owning class");
            AstVar* സ്ഥിരം smodep = getStaticRandModeVar(ownerp);
            UASSERT_OBJ(smodep, varp, "usesMode static var without a static rand-mode array");
            arrayp = പുതിയ AstVarRef{fl, VN_AS(smodep->user2p(), കണ്ണി_Module), smodep, VAccess::READ};
        } അല്ലെങ്കിൽ എങ്കിൽ (mselp) {
            AstVar* സ്ഥിരം memberModep = getRandModeVarFromClass(VN_AS(varp->user2p(), കണ്ണി_Module));
            UASSERT_OBJ(memberModep, varp, "usesMode member without a class rand-mode array");
            arrayp = പുതിയ AstMemberSel{fl, mselp->fromp()->clone_വൃക്ഷം_Pure(അസത്യം), memberModep};
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(randModeVarp, varp, "rand_mode variable without a class mode array");
            arrayp = പുതിയ AstVarRef{fl, VN_AS(randModeVarp->user2p(), കണ്ണി_Module), randModeVarp,
                                   VAccess::READ};
        }
        സ്ഥിരം RandomizeMode rmode = {.asUQuad = varp->user1()};
        AstCMethodHard* സ്ഥിരം atp
            = പുതിയ AstCMethodHard{fl, arrayp, VCMethod::ARRAY_AT, പുതിയ AstConst{fl, rmode.index}};
        atp->dtypeSetUInt32();
        തിരികെ atp;
    }

    // The rand_mode bit of one variable access, or a constant when it has none.
    // A non-rand value can never be re-drawn; a non-rand owner level is skipped.
    Ast_കണ്ണി_വ്യഞ്ജകം* newVarGate(AstVar* varp, സ്ഥിരം AstMemberSel* mselp, തർക്കം ownerLevel,
                            AstVar* randModeVarp, File_വരി* fl) {
        എങ്കിൽ (!varp->rand().isRandomizable()) {
            തിരികെ പുതിയ AstConst{fl, AstConst::BitTrue{}, ownerLevel};
        }
        സ്ഥിരം RandomizeMode rmode = {.asUQuad = varp->user1()};
        എങ്കിൽ (!rmode.usesMode) തിരികെ പുതിയ AstConst{fl, AstConst::BitTrue{}};
        തിരികെ newModeBitRead(varp, mselp, randModeVarp, fl);
    }

    // Change bit of one variable access, ANDed level by level down the
    // member-select owner path.
    Ast_കണ്ണി_വ്യഞ്ജകം* newAccessGate(സ്ഥിരം Ast_കണ്ണി* nodep, തർക്കം ownerLevel, AstVar* randModeVarp,
                               File_വരി* fl) {
        എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം mselp = VN_CAST(nodep, MemberSel)) {
            തിരികെ newGateAnd(fl, newVarGate(mselp->varp(), mselp, ownerLevel, randModeVarp, fl),
                              newAccessGate(mselp->fromp(), സത്യം, randModeVarp, fl));
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം refp = VN_CAST(nodep, കണ്ണി_Var_Ref)) {
            തിരികെ newVarGate(refp->varp(), nullptr, ownerLevel, randModeVarp, fl);
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Sel* സ്ഥിരം selp = VN_CAST(nodep, കണ്ണി_Sel)) {
            തിരികെ newAccessGate(selp->fromp(), സത്യം, randModeVarp, fl);
        }
        എങ്കിൽ (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        തിരികെ പുതിയ AstConst{fl, AstConst::BitTrue{}};
    }

    // Gate of one dist expression node: nonzero while its value can still be
    // freshly drawn. Constant when no runtime rand_mode bit is involved.
    Ast_കണ്ണി_വ്യഞ്ജകം* newDistGate(സ്ഥിരം Ast_കണ്ണി* nodep, AstVar* randModeVarp, File_വരി* fl) {
        എങ്കിൽ (VN_IS(nodep, കണ്ണി_Var_Ref) || VN_IS(nodep, MemberSel)) {
            തിരികെ newAccessGate(nodep, അസത്യം, randModeVarp, fl);
        }
        // A selection is drawn through its base; the index never gates.
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Sel* സ്ഥിരം selp = VN_CAST(nodep, കണ്ണി_Sel)) {
            തിരികെ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) {
            തിരികെ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം selp = VN_CAST(nodep, StructSel)) {
            തിരികെ newDistGate(selp->fromp(), randModeVarp, fl);
        }
        എങ്കിൽ (സ്ഥിരം AstCMethodHard* സ്ഥിരം methodp = VN_CAST(nodep, CMethodHard)) {
            എങ്കിൽ (methodp->method() == VCMethod::ARRAY_AT
                && VN_IS(methodp->fromp()->dtypep()->skipRefp(),  // LCOV_EXCL_BR_LINE
                         DynArrayDType)) {  // Queue form is rejected before dist lowering
                തിരികെ newDistGate(methodp->fromp(), randModeVarp, fl);
            }
        }
        // A conditional re-draws through an active selector or the selected arm.
        എങ്കിൽ (സ്ഥിരം AstCond* സ്ഥിരം condp = VN_CAST(nodep, Cond)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thenGatep = newDistGate(condp->thenp(), randModeVarp, fl);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elseGatep = newDistGate(condp->elsep(), randModeVarp, fl);
            Ast_കണ്ണി_വ്യഞ്ജകം* armGatep = thenGatep;
            എങ്കിൽ (thenGatep->same_വൃക്ഷം(elseGatep)) {
                VL_DO_DANGLING(തള്ളുക_Deletep(elseGatep), elseGatep);
            } അല്ലെങ്കിൽ {
                armGatep
                    = പുതിയ AstCond{fl, condp->condp()->clone_വൃക്ഷം_Pure(അസത്യം), thenGatep, elseGatep};
            }
            തിരികെ newGateOr(fl, newDistGate(condp->condp(), randModeVarp, fl), armGatep);
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Uniop* സ്ഥിരം uopp = VN_CAST(nodep, കണ്ണി_Uniop)) {
            തിരികെ newDistGate(uopp->lhsp(), randModeVarp, fl);
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം bopp = VN_CAST(nodep, കണ്ണി_Biop)) {
            തിരികെ newGateOr(fl, newDistGate(bopp->lhsp(), randModeVarp, fl),
                             newDistGate(bopp->rhsp(), randModeVarp, fl));
        }
        എങ്കിൽ (distBoundRefsModeVar(nodep)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: 'rand_mode' on a variable used inside this form of"
                          " 'dist' expression");
        }
        // Constants and remaining forms re-draw only if a rand var is inside.
        തിരികെ പുതിയ AstConst{fl, AstConst::BitTrue{}, distBoundRefsRandVar(nodep)};
    }

    // Membership union of the dist set, with runtime-zero weights excluded.
    Ast_Constraint_വ്യഞ്ജകം* buildFrozenMembership(AstDist* distp,
                                             സ്ഥിരം std::vector<DistBucket>& buckets) {
        File_വരി* സ്ഥിരം fl = distp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* unionExprp = nullptr;
        വേണ്ടി (സ്ഥിരം auto& bucket : buckets) {
            Ast_കണ്ണി_വ്യഞ്ജകം* termp = newDistMembershipTerm(distp, bucket.rangep);
            // A weight that is zero only at runtime excludes the bucket's values too.
            സ്ഥിരം തർക്കം runtimeWeight
                = bucket.weightExprp->exists([](സ്ഥിരം Ast_കണ്ണി_Var_Ref*) { തിരികെ സത്യം; });
            എങ്കിൽ (runtimeWeight) {
                AstNeq* സ്ഥിരം nzp = പുതിയ AstNeq{fl, bucket.weightExprp->clone_വൃക്ഷം_Pure(അസത്യം),
                                               പുതിയ AstConst{fl, AstConst::Unsized64{}, 0}};
                nzp->user1(സത്യം);
                termp = പുതിയ AstLogAnd{fl, termp, nzp};
                termp->user1(സത്യം);
            }
            എങ്കിൽ (unionExprp) {
                unionExprp = പുതിയ AstLogOr{fl, unionExprp, termp};
                unionExprp->user1(സത്യം);
            } അല്ലെങ്കിൽ {
                unionExprp = termp;
            }
        }
        തിരികെ പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, unionExprp};
    }

    // Weighted chain while the dist value can still be freshly drawn, membership
    // once it is frozen; membership only when it can never be re-drawn.
    Ast_കണ്ണി* gateFrozenDist(Ast_കണ്ണി* chainp, AstDist* distp,
                            സ്ഥിരം std::vector<DistBucket>& buckets, AstVar* randModeVarp) {
        File_വരി* സ്ഥിരം fl = distp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* gatep = newDistGate(distp->exprp(), randModeVarp, fl);
        എങ്കിൽ (VN_IS(gatep, Const)) {
            സ്ഥിരം തർക്കം drawable = !VN_AS(gatep, Const)->isZero();
            VL_DO_DANGLING(തള്ളുക_Deletep(gatep), gatep);
            എങ്കിൽ (drawable) തിരികെ chainp;
            VL_DO_DANGLING(തള്ളുക_Deletep(chainp), chainp);
            തിരികെ buildFrozenMembership(distp, buckets);
        }
        തിരികെ പുതിയ AstConstraintIf{fl, gatep, chainp, buildFrozenMembership(distp, buckets)};
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newUniformRangePick(AstDist* distp, സ്ഥിരം AstInsideRange* irp) {
        File_വരി* സ്ഥിരം fl = distp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dist_വ്യഞ്ജകം_Copyp = distp->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
        dist_വ്യഞ്ജകം_Copyp->user1(സത്യം);
        സ്ഥിരം പൂർണ്ണസംഖ്യ distWidth = distp->exprp()->width();
        സ്ഥിരം AstConst* സ്ഥിരം lopC = VN_CAST(irp->lhsp(), Const);
        സ്ഥിരം AstConst* സ്ഥിരം hipC = VN_CAST(irp->rhsp(), Const);
        Ast_കണ്ണി_വ്യഞ്ജകം* rangeSzp;
        എങ്കിൽ (lopC && hipC) {
            സ്ഥിരം uint64_t rsz = hipC->toUQuad() - lopC->toUQuad() + 1;
            rangeSzp = പുതിയ AstConst{fl, AstConst::Unsized64{}, rsz};
        } അല്ലെങ്കിൽ {
            സ്ഥിരം തർക്കം isSigned = irp->lhsp()->isSigned();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lo64p
                = isSigned ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                 പുതിയ AstExtendS{fl, irp->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം), 64})
                           : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                 പുതിയ AstExtend{fl, irp->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം), 64});
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം hi64p
                = isSigned ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                 പുതിയ AstExtendS{fl, irp->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം), 64})
                           : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                                 പുതിയ AstExtend{fl, irp->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം), 64});
            rangeSzp = പുതിയ AstAdd{fl, പുതിയ AstConst{fl, AstConst::Unsized64{}, 1ULL},
                                  പുതിയ AstSub{fl, hi64p, lo64p}};
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rand64p = പുതിയ AstRand{fl, nullptr, അസത്യം};
        rand64p->dtypeSetUInt64();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം offsetp
            = പുതിയ AstCCast{fl, പുതിയ AstModDiv{fl, rand64p, rangeSzp}, distWidth};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = പുതിയ AstAdd{fl, irp->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം), offsetp};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം eqp = പുതിയ AstEq{fl, dist_വ്യഞ്ജകം_Copyp, valuep};
        eqp->user1(സത്യം);
        തിരികെ eqp;
    }

    // Soft weighted bucket chain: select a bucket by bucketVar against cumulative weights.
    Ast_കണ്ണി* buildWeightedBucketChain(AstDist* distp, സ്ഥിരം std::vector<DistBucket>& buckets,
                                      AstVar* bucketVarp,
                                      സ്ഥിരം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& cumSums) {
        File_വരി* സ്ഥിരം fl = distp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* chainp = nullptr;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = static_cast<പൂർണ്ണസംഖ്യ>(buckets.size()) - 1; i >= 0; --i) {
            Ast_കണ്ണി_വ്യഞ്ജകം* constraintExprp;
            സ്ഥിരം AstInsideRange* സ്ഥിരം irp = VN_CAST(buckets[i].rangep, InsideRange);
            എങ്കിൽ (irp && (distBoundRefsRandVar(irp->lhsp()) || distBoundRefsRandVar(irp->rhsp()))) {
                constraintExprp = newDistRangeMembership(distp, irp);
            } അല്ലെങ്കിൽ എങ്കിൽ (irp) {
                constraintExprp = newUniformRangePick(distp, irp);
            } അല്ലെങ്കിൽ {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dist_വ്യഞ്ജകം_Copyp = distp->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
                dist_വ്യഞ്ജകം_Copyp->user1(സത്യം);
                constraintExprp
                    = പുതിയ AstEq{fl, dist_വ്യഞ്ജകം_Copyp, buckets[i].rangep->clone_വൃക്ഷം_Pure(അസത്യം)};
                constraintExprp->user1(സത്യം);
            }
            എങ്കിൽ (!chainp) {
                chainp = constraintExprp;
            } അല്ലെങ്കിൽ {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp
                    = പുതിയ AstLte{fl, പുതിയ AstVarRef{fl, bucketVarp, VAccess::READ}, cumSums[i]};
                condp->user1(സത്യം);
                chainp = പുതിയ AstCond{fl, condp, constraintExprp, chainp};
                chainp->user1(സത്യം);
            }
        }
        Ast_Constraint_വ്യഞ്ജകം* സ്ഥിരം chainExprp = പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, chainp};
        chainExprp->isSoft(സത്യം);
        തിരികെ chainExprp;
    }

    // Replace AstDist with weighted bucket selection via AstConstraintIf chain.
    // Supports both constant and variable weight expressions.
    ശൂന്യം lowerDistConstraints(AstTask* taskp, Ast_കണ്ണി* constrItemsp, AstVar* randModeVarp,
                              AstConstraintForeach* foreachp = nullptr) {
        // When inside a foreach, bucket preamble stmts are stored in foreachp->user3p()
        // (as a linked list) so visit(AstConstraintForeach*) can inject them into the
        // real AstForeach body. Outside a foreach, they go directly into taskp.
        Ast_കണ്ണി* foreachTailp = nullptr;
        auto addStmt = [&](Ast_കണ്ണി* nodep) {
            എങ്കിൽ (foreachp) {
                എങ്കിൽ (!foreachTailp) {
                    foreachp->user3p(nodep);
                    foreachTailp = nodep;
                } അല്ലെങ്കിൽ {
                    foreachTailp->addNext(nodep);
                    foreachTailp = nodep;
                }
            } അല്ലെങ്കിൽ {
                taskp->addStmtsp(nodep);
            }
        };

        വേണ്ടി (Ast_കണ്ണി *nextip, *itemp = constrItemsp; itemp; itemp = nextip) {
            nextip = itemp->nextp();

            // dist can appear inside an if/else or foreach constraint.
            എങ്കിൽ (AstConstraintIf* സ്ഥിരം cifp = VN_CAST(itemp, ConstraintIf)) {
                UASSERT_OBJ(cifp->thensp(), cifp, "constraint if without a then body");
                lowerDistConstraints(taskp, cifp->thensp(), randModeVarp, foreachp);
                എങ്കിൽ (cifp->elsesp()) {
                    lowerDistConstraints(taskp, cifp->elsesp(), randModeVarp, foreachp);
                }
                തുടരുക;
            }
            എങ്കിൽ (AstConstraintForeach* സ്ഥിരം cfep = VN_CAST(itemp, ConstraintForeach)) {
                UASSERT_OBJ(cfep->bodyp(), cfep, "constraint foreach without a body");
                lowerDistConstraints(taskp, cfep->bodyp(), randModeVarp, cfep);
                തുടരുക;
            }

            Ast_Constraint_വ്യഞ്ജകം* സ്ഥിരം constrExprp = VN_CAST(itemp, Constraint_വ്യഞ്ജകം);
            എങ്കിൽ (!constrExprp) തുടരുക;

            // `cond -> x dist {...}` parses as ConstraintExpr(LogIf(cond, Dist)).
            // This pass only scans ConstraintExpr/If/Foreach for Dist, so lift the
            // LogIf chain into nested ConstraintIf first. Chains like
            // `a -> b -> dist` nest accordingly.
            എങ്കിൽ (AstLogIf* സ്ഥിരം മുകൾ_Log_Ifp = VN_CAST(constrExprp->exprp(), LogIf)) {
                Ast_കണ്ണി* chainEndp = മുകൾ_Log_Ifp->rhsp();
                ആകുമ്പോൾ (AstLogIf* സ്ഥിരം innerp = VN_CAST(chainEndp, LogIf)) {
                    chainEndp = innerp->rhsp();
                }
                എങ്കിൽ (VN_IS(chainEndp, Dist)) {
                    AstConstraintIf* സ്ഥിരം liftedp = liftLogIfChainToConstraintIf(മുകൾ_Log_Ifp);
                    constrExprp->replaceWith(liftedp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(constrExprp), constrExprp);
                    lowerDistConstraints(taskp, liftedp->thensp(), randModeVarp, foreachp);
                    തുടരുക;
                }
            }

            AstDist* സ്ഥിരം distp = VN_CAST(constrExprp->exprp(), Dist);
            എങ്കിൽ (!distp) തുടരുക;

            File_വരി* സ്ഥിരം fl = distp->fileline();

            std::vector<DistBucket> buckets = collectDistBuckets(distp);

            എങ്കിൽ (buckets.empty()) {
                // All weights are zero: dist is vacuously true (unconstrained)
                Ast_Constraint_വ്യഞ്ജകം* സ്ഥിരം truep
                    = പുതിയ Ast_Constraint_വ്യഞ്ജകം{fl, പുതിയ AstConst{fl, AstConst::BitTrue{}}};
                constrExprp->replaceWith(truep);
                VL_DO_DANGLING(തള്ളുക_Deletep(constrExprp), constrExprp);
                തുടരുക;
            }

            Ast_Constraint_വ്യഞ്ജകം* സ്ഥിരം membershipp = buildDistMembership(distp, buckets);

            // Build totalWeight expression: w[0] + w[1] + ... + w[N-1]
            Ast_കണ്ണി_വ്യഞ്ജകം* totalWeightExprp = nullptr;
            വേണ്ടി (auto& bucket : buckets) {
                എങ്കിൽ (!totalWeightExprp) {
                    totalWeightExprp = bucket.weightExprp->clone_വൃക്ഷം_Pure(അസത്യം);
                } അല്ലെങ്കിൽ {
                    totalWeightExprp = പുതിയ AstAdd{fl, totalWeightExprp,
                                                  bucket.weightExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                    totalWeightExprp->dtypeSetUInt64();
                }
            }

            // Store totalWeight in temp var (evaluated once, used twice)
            സ്ഥിരം പൂർണ്ണസംഖ്യ distId = m_dist_അക്കം++;
            സ്ഥിരം std::string totalName = "__Vdist_total" + cvtToStr(distId);
            AstVar* സ്ഥിരം totalVarp
                = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, totalName, taskp->findUInt64DType()};
            totalVarp->noSubst(സത്യം);
            totalVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            totalVarp->funcLocal(സത്യം);
            totalVarp->isInternal(സത്യം);
            addStmt(totalVarp);
            addStmt(
                പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, totalVarp, VAccess::WRITE}, totalWeightExprp});

            // bucketVar = (rand64() % totalWeight) + 1
            സ്ഥിരം std::string bucketName = "__Vdist_bucket" + cvtToStr(distId);
            AstVar* സ്ഥിരം bucketVarp
                = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, bucketName, taskp->findUInt64DType()};
            bucketVarp->noSubst(സത്യം);
            bucketVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            bucketVarp->funcLocal(സത്യം);
            bucketVarp->isInternal(സത്യം);
            addStmt(bucketVarp);

            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം randp = പുതിയ AstRand{fl, nullptr, അസത്യം};
            randp->dtypeSetUInt64();
            addStmt(പുതിയ AstAssign{
                fl, പുതിയ AstVarRef{fl, bucketVarp, VAccess::WRITE},
                പുതിയ AstAdd{
                    fl, പുതിയ AstConst{fl, AstConst::Unsized64{}, 1},
                    പുതിയ AstModDiv{fl, randp, പുതിയ AstVarRef{fl, totalVarp, VAccess::READ}}}});

            // Build cumulative sum expressions forward: cumSum[i] = w[0]+...+w[i]
            std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> cumSums;
            Ast_കണ്ണി_വ്യഞ്ജകം* runningSump = nullptr;
            വേണ്ടി (size_t i = 0; i < buckets.size(); ++i) {
                എങ്കിൽ (!runningSump) {
                    runningSump = buckets[i].weightExprp->clone_വൃക്ഷം_Pure(അസത്യം);
                } അല്ലെങ്കിൽ {
                    runningSump = പുതിയ AstAdd{fl, runningSump,
                                             buckets[i].weightExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                    runningSump->dtypeSetUInt64();
                }
                cumSums.തള്ളുക_back(runningSump->clone_വൃക്ഷം_Pure(സത്യം));
            }

            Ast_കണ്ണി* chainp = buildWeightedBucketChain(distp, buckets, bucketVarp, cumSums);

            chainp = gateFrozenDist(chainp, distp, buckets, randModeVarp);

            constrExprp->replaceWith(chainp);
            VL_DO_DANGLING(തള്ളുക_Deletep(constrExprp), constrExprp);
            // Hard membership precedes the soft bucket chain in the constraint list.
            chainp->addHereThisAsNext(membershipp);

            // Clean up nodes used only as clone templates (never inserted into tree)
            വേണ്ടി (auto& bucket : buckets) {
                VL_DO_DANGLING(തള്ളുക_Deletep(bucket.weightExprp), bucket.weightExprp);
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(runningSump), runningSump);
            // Last cumSum is unused (last bucket is unconditional default)
            തള്ളുക_Deletep(cumSums.back());
        }
    }

    സ്ഥിതം തർക്കം isDynArrOfClassTypeRecurse(സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം refp = dtypep->skipRefp();
        എങ്കിൽ (VN_IS(refp, DynArrayDType) || VN_IS(refp, QueueDType)) {
            തിരികെ isDynArrOfClassTypeRecurse(refp->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(refp, ClassRefDType)) {
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // Add write_var calls for each array element after resizing it with array-size constraint
    ശൂന്യം addArrayElemRefresh(File_വരി* സ്ഥിരം fl, AstVar* സ്ഥിരം arrVarp, AstFunc* സ്ഥിരം randomizep,
                             AstVar* സ്ഥിരം genp) {
        Ast_കണ്ണി_Module* സ്ഥിരം genModp = VN_AS(genp->user2p(), കണ്ണി_Module);
        // Array elements of class data type are passed to the solver as separate
        // variables, so passing the original array variable is redundant, because it
        // won't be referenced
        സ്ഥിരം uint32_t unpackedDims = arrVarp->dtypep()->dimensions(അസത്യം).second;
        എങ്കിൽ (isDynArrOfClassTypeRecurse(arrVarp->dtypep())) {
            എങ്കിൽ (unpackedDims > 1) {
                arrVarp->v3warn(E_UNSUPPORTED,
                                "Unsupported: Nested array element access in global constraint");
            }
            തിരികെ;
        }
        AstCMethodHard* സ്ഥിരം methodp
            = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_WRITE_VAR};
        methodp->dtypeSetVoid();

        Ast_കണ്ണി_Module* സ്ഥിരം classp = VN_AS(arrVarp->user2p(), കണ്ണി_Module);
        AstVarRef* സ്ഥിരം varRefp = പുതിയ AstVarRef{fl, classp, arrVarp, VAccess::WRITE};
        varRefp->classOrPackagep(classp);
        methodp->addPinsp(varRefp);

        സ്ഥിരം size_t width = arrayElementDTypep(arrVarp->dtypep())->width();

        methodp->addPinsp(പുതിയ AstConst{fl, AstConst::Unsized64{}, width});
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം varnamep
            = പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "\"" + arrVarp->name() + "\"", arrVarp->width()};
        varnamep->dtypep(arrVarp->dtypep());
        methodp->addPinsp(varnamep);
        methodp->addPinsp(പുതിയ AstConst{fl, AstConst::Unsized64{}, unpackedDims});
        സ്ഥിരം RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        എങ്കിൽ (rmode.usesMode) {
            methodp->addPinsp(പുതിയ AstConst{fl, AstConst::Unsized64{}, rmode.index});
        }

        randomizep->addStmtsp(methodp->makeStmt());
    }

    Ast_കണ്ണി_Stmt* newPinSizeVariable(File_വരി* സ്ഥിരം fl, AstVar* സ്ഥിരം arrVarp,
                                    AstVar* സ്ഥിരം genp) {
        Ast_കണ്ണി_Module* സ്ഥിരം genModp = VN_AS(genp->user2p(), കണ്ണി_Module);
        AstVar* സ്ഥിരം sizeVarp = VN_AS(arrVarp->user4p(), Var);
        AstCMethodHard* സ്ഥിരം pinp
            = പുതിയ AstCMethodHard{fl, പുതിയ AstVarRef{fl, genModp, genp, VAccess::READWRITE},
                                 VCMethod::RANDOMIZER_PIN_VAR};
        pinp->dtypeSetVoid();
        AstCExpr* സ്ഥിരം namep = പുതിയ AstCExpr{fl, AstCExpr::Pure{}, "\"" + sizeVarp->name() + "\""};
        namep->dtypeSetUInt32();
        pinp->addPinsp(namep);
        pinp->addPinsp(
            പുതിയ AstConst{fl, AstConst::Unsized64{}, static_cast<uint64_t>(sizeVarp->width())});
        // sizeVarp may live in a base class when the constrained array is inherited; route the
        // reference through its declaring class so V3Scope can resolve it.
        AstVarRef* സ്ഥിരം sizeVarRefp = പുതിയ AstVarRef{fl, sizeVarp, VAccess::READ};
        sizeVarRefp->classOrPackagep(VN_AS(sizeVarp->user2p(), കണ്ണി_Module));
        pinp->addPinsp(sizeVarRefp);
        തിരികെ pinp->makeStmt();
    }

    ശൂന്യം addFrozenSizePin(File_വരി* സ്ഥിരം fl, AstVar* സ്ഥിരം arrVarp, AstFunc* സ്ഥിരം randomizep,
                          AstVar* സ്ഥിരം genp, AstVar* സ്ഥിരം randModeVarp) {
        സ്ഥിരം RandomizeMode rmode = {.asUQuad = arrVarp->user1()};
        എങ്കിൽ (!rmode.usesMode) തിരികെ;
        AstVar* സ്ഥിരം sizeVarp = VN_AS(arrVarp->user4p(), Var);
        Ast_കണ്ണി_Module* സ്ഥിരം arrClassp = VN_AS(arrVarp->user2p(), കണ്ണി_Module);
        Ast_കണ്ണി_Module* സ്ഥിരം sizeClassp = VN_AS(sizeVarp->user2p(), കണ്ണി_Module);
        AstVarRef* സ്ഥിരം sizeWritep = പുതിയ AstVarRef{fl, sizeClassp, sizeVarp, VAccess::WRITE};
        സ്ഥിരം VCMethod sizeMethod = VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)
                                        ? VCMethod::ASSOC_SIZE
                                        : VCMethod::DYN_SIZE;
        AstCMethodHard* സ്ഥിരം currentSizep = പുതിയ AstCMethodHard{
            fl, പുതിയ AstVarRef{fl, arrClassp, arrVarp, VAccess::READ}, sizeMethod};
        currentSizep->dtypep(sizeVarp->dtypep());
        randomizep->addStmtsp(പുതിയ AstAssign{fl, sizeWritep, currentSizep});
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം modep = newModeBitRead(arrVarp, nullptr, randModeVarp, fl);
        randomizep->addStmtsp(
            പുതിയ AstIf{fl, പുതിയ AstLogNot{fl, modep}, newPinSizeVariable(fl, arrVarp, genp)});
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_അക്കം);
        VL_RESTORER_COPY(m_randArrayUniqueNames);
        m_modp = nodep;
        m_rand_Case_അക്കം = 0;
        m_randArrayUniqueNames.reset();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_rand_Case_അക്കം);
        VL_RESTORER(m_dist_അക്കം);
        m_modp = nodep;
        m_rand_Case_അക്കം = 0;
        m_dist_അക്കം = 0;
        m_writtenVars.clear();  // Each class has its own set of written variables

        iterateChildren(nodep);
        എങ്കിൽ (!nodep->user1()) തിരികെ;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep);
        nodep->baseMostClassp()->needRNG(സത്യം);

        // Detect global-constraint ownership BEFORE the constraint items are
        // unlinked into setup tasks below (after that the member-selects are gone).
        സ്ഥിരം തർക്കം basicFirst = classOwnsGlobalConstraint(nodep);

        File_വരി* സ്ഥിരം fl = nodep->fileline();
        AstFunc* സ്ഥിരം randomizep = V3Randomize::newRandomizeFunc(m_memberMap, nodep);
        AstVar* സ്ഥിരം fvarp = VN_AS(randomizep->fvarp(), Var);
        AstVar* സ്ഥിരം randModeVarp = getRandModeVarFromClass(nodep);
        addPrePostCall(nodep, randomizep, "pre_randomize");

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        എങ്കിൽ (classHasRandClassMembers(nodep)) {
            AstTask* സ്ഥിരം preTaskp = getCreateNestedCallbackTask(nodep, "pre");
            populateNestedCallbackTask(preTaskp, nodep, "pre_randomize");
            randomizep->addStmtsp((പുതിയ AstTaskRef{fl, preTaskp})->makeStmt());
        }

        // Both IS_RANDOMIZED and IS_RANDOMIZED_GLOBAL classes need full constraint support
        // IS_RANDOMIZED_GLOBAL classes can be randomized independently
        Ast_കണ്ണി_വ്യഞ്ജകം* beginValp = nullptr;
        AstVar* genp = getRandomGenerator(nodep);
        എങ്കിൽ (genp) {
            // Phase 1: Process all constraints (create tasks, run ConstraintExprVisitor)
            // Setup task refs are NOT added to setupAllTaskp here -- done in phase 2
            AstTask* സ്ഥിരം prepareArraysTaskp = createPrepareConstrainedArraysTask(nodep);
            nodep->foreachMember([&](AstClass* സ്ഥിരം classp, AstConstraint* സ്ഥിരം constrp) {
                AstTask* taskp = VN_AS(constrp->user2p(), Task);
                എങ്കിൽ (!taskp) {
                    taskp = newSetupConstraintTask(classp, constrp->name());
                    constrp->user2p(taskp);
                }

                എങ്കിൽ (AstTask* സ്ഥിരം resizeTaskp = VN_CAST(constrp->user3p(), Task)) {
                    AstTask* സ്ഥിരം resizeAllTaskp = getCreateAggrResizeTask(nodep);
                    AstTaskRef* സ്ഥിരം resizeTaskRefp
                        = പുതിയ AstTaskRef{constrp->fileline(), resizeTaskp};
                    resizeTaskRefp->classOrPackagep(classp);
                    resizeAllTaskp->addStmtsp(resizeTaskRefp->makeStmt());
                }

                എങ്കിൽ (constrp->itemsp()) expandUniqueElementList(constrp->itemsp());
                എങ്കിൽ (constrp->itemsp()) {
                    lowerDistConstraints(taskp, constrp->itemsp(), randModeVarp);
                }
                VInsertionSet<AstVar*>& sizeArrays = m_sizeConstrainedArrays[classp];
                Constraint_വ്യഞ്ജകം_Visitor{
                    classp,        m_memberMap,  constrp->itemsp(), nullptr,
                    genp,          randModeVarp, m_writtenVars,     &m_uniqueConstraintId,
                    m_uniqueNames, randomizep,   &sizeArrays,       prepareArraysTaskp};
                എങ്കിൽ (constrp->itemsp()) {
                    taskp->addStmtsp(wrapIfConstraintMode(
                        nodep, constrp, constrp->itemsp()->unlinkFrBackWithNext()));
                }
            });

            // Phase 2: Add setup task refs in depth order (deepest first = lowest priority)
            // Ensures outer-scope soft constraints override inner-scope (IEEE 18.5.13)
            {
                // Count nesting depth by GLOBAL_CONSTRAINT_SEPARATOR occurrences
                സ്ഥിരം auto constraintDepth = [](സ്ഥിരം AstConstraint* constrp) {
                    പൂർണ്ണസംഖ്യ depth = 0;
                    size_t pos = 0;
                    ആകുമ്പോൾ ((pos = constrp->name().find(GLOBAL_CONSTRAINT_SEPARATOR, pos))
                           != std::string::npos) {
                        ++depth;
                        pos += std::strlen(GLOBAL_CONSTRAINT_SEPARATOR);
                    }
                    തിരികെ depth;
                };
                പൂർണ്ണസംഖ്യ maxDepth = 0;
                nodep->foreachMember([&](AstClass* സ്ഥിരം, AstConstraint* സ്ഥിരം constrp) {
                    maxDepth = std::max(maxDepth, constraintDepth(constrp));
                });
                AstTask* സ്ഥിരം setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
                വേണ്ടി (പൂർണ്ണസംഖ്യ d = maxDepth; d >= 0; --d) {
                    nodep->foreachMember(
                        [&](AstClass* സ്ഥിരം classp, AstConstraint* സ്ഥിരം constrp) {
                            എങ്കിൽ (constraintDepth(constrp) != d) തിരികെ;
                            AstTask* സ്ഥിരം taskp = VN_AS(constrp->user2p(), Task);
                            AstTaskRef* സ്ഥിരം setupTaskRefp
                                = പുതിയ AstTaskRef{constrp->fileline(), taskp};
                            setupTaskRefp->classOrPackagep(classp);
                            setupAllTaskp->addStmtsp(setupTaskRefp->makeStmt());
                        });
                }
            }
            // For derived classes: clone write_var calls from parent's randomize()
            // and save every array that has is size-constrained array to generate
            // array element refresh later
            VInsertionSet<AstVar*> sizeArrayVars;
            എങ്കിൽ (nodep->extendsp()) {
                AstClass* parentClassp = nodep->extendsp()->classp();
                ആകുമ്പോൾ (parentClassp) {
                    സ്ഥിരം auto sizeArraysIt = m_sizeConstrainedArrays.find(parentClassp);
                    എങ്കിൽ (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                        വേണ്ടി (AstVar* സ്ഥിരം arrVarp : sizeArraysIt->second) {
                            sizeArrayVars.തിരുകുക(arrVarp);
                        }
                    }
                    AstFunc* സ്ഥിരം parentRandomizep
                        = VN_CAST(m_memberMap.findMember(parentClassp, "randomize"), Func);
                    എങ്കിൽ (parentRandomizep && parentRandomizep->stmtsp()) {
                        // Clone write_var statements from parent (stop at clearConstraints)
                        വേണ്ടി (Ast_കണ്ണി* stmtp = parentRandomizep->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            തർക്കം foundClearConstraints = അസത്യം;
                            stmtp->foreach([&](AstCMethodHard* methodp) {
                                എങ്കിൽ (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                                    || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                                    randomizep->addStmtsp(stmtp->clone_വൃക്ഷം(അസത്യം));
                                } അല്ലെങ്കിൽ എങ്കിൽ (methodp->method()
                                           == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                                    foundClearConstraints = സത്യം;
                                }
                            });
                            എങ്കിൽ (foundClearConstraints) നിർത്തുക;
                        }
                    }
                    parentClassp
                        = parentClassp->extendsp() ? parentClassp->extendsp()->classp() : nullptr;
                }
            }
            randomizep->addStmtsp(implementConstraintsClear(fl, genp));

            സ്ഥിരം auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
            എങ്കിൽ (sizeArraysIt != m_sizeConstrainedArrays.end()) {
                വേണ്ടി (AstVar* സ്ഥിരം arrVarp : sizeArraysIt->second) sizeArrayVars.തിരുകുക(arrVarp);
            }
            AstTask* സ്ഥിരം setupAllTaskp = getCreateConstraintSetupFunc(nodep, m_memberMap);
            എങ്കിൽ (!sizeArrayVars.empty() && prepareArraysTaskp->stmtsp()) {
                Ast_കണ്ണി_Stmt* സ്ഥിരം preparep = (പുതിയ AstTaskRef{fl, prepareArraysTaskp})->makeStmt();
                // A size-constrained array always has a preceding write_var registration.
                വേണ്ടി (Ast_കണ്ണി* stmtp = randomizep->stmtsp(); stmtp;  // LCOV_EXCL_BR_LINE
                     stmtp = stmtp->nextp()) {
                    തർക്കം writesArray = അസത്യം;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        writesArray |= methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR;
                    });
                    എങ്കിൽ (!writesArray) തുടരുക;
                    stmtp->addHereThisAsNext(preparep);
                    നിർത്തുക;
                }
            }
            AstTaskRef* സ്ഥിരം setupTaskRefp = പുതിയ AstTaskRef{fl, setupAllTaskp};
            randomizep->addStmtsp(setupTaskRefp->makeStmt());

            Ast_കണ്ണി_Module* സ്ഥിരം genModp = VN_AS(genp->user2p(), കണ്ണി_Module);

            AstCExpr* സ്ഥിരം solverCallp = പുതിയ AstCExpr{fl};
            solverCallp->dtypeSetBit();
            solverCallp->add(പുതിയ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
            solverCallp->add(".next(__Vm_rng)");
            എങ്കിൽ (!sizeArrayVars.empty()) {
                AstVar* സ്ഥിരം sizeOkVarp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, "__Vsize_ok",
                                                      nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                sizeOkVarp->funcLocal(സത്യം);
                randomizep->addStmtsp(sizeOkVarp);

                AstVar* സ്ഥിരം finalOkVarp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, "__Vfinal_ok",
                                                       nodep->findBasicDType(VBasicDTypeKwd::BIT)};
                finalOkVarp->funcLocal(സത്യം);
                randomizep->addStmtsp(finalOkVarp);

                // A disabled array keeps its current size. Pin its size proxy before solving so
                // an incompatible size constraint fails instead of resizing the frozen array.
                വേണ്ടി (AstVar* സ്ഥിരം arrVarp : sizeArrayVars)
                    addFrozenSizePin(fl, arrVarp, randomizep, genp, randModeVarp);

                // First pass: solve size variables (and other constraints) to determine sizes
                randomizep->addStmtsp(
                    പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, sizeOkVarp, VAccess::WRITE}, solverCallp});

                // Resize constrained arrays before rebuilding constraints
                എങ്കിൽ (AstTask* സ്ഥിരം resizeAllTaskp
                    = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                    AstTaskRef* സ്ഥിരം resizeTaskRefp = പുതിയ AstTaskRef{fl, resizeAllTaskp};
                    AstIf* സ്ഥിരം ifp = പുതിയ AstIf{fl, പുതിയ AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                                 resizeTaskRefp->makeStmt(), nullptr};
                    randomizep->addStmtsp(ifp);
                }

                // Refresh array element tables after resize
                വേണ്ടി (സ്ഥിരം auto& arrVarp : sizeArrayVars) {
                    addArrayElemRefresh(fl, arrVarp, randomizep, genp);
                }

                // Rebuild constraints after resize and pin size variables
                randomizep->addStmtsp(implementConstraintsClear(fl, genp));
                AstTaskRef* സ്ഥിരം setupTaskRefp2 = പുതിയ AstTaskRef{fl, setupAllTaskp};
                randomizep->addStmtsp(setupTaskRefp2->makeStmt());

                വേണ്ടി (AstVar* സ്ഥിരം arrVarp : sizeArrayVars) {
                    randomizep->addStmtsp(newPinSizeVariable(fl, arrVarp, genp));
                }

                // Final pass: solve full constraints with sizes pinned
                AstCExpr* സ്ഥിരം solverCallp2 = പുതിയ AstCExpr{fl};
                solverCallp2->dtypeSetBit();
                solverCallp2->add(പുതിയ AstVarRef{fl, genModp, genp, VAccess::READWRITE});
                solverCallp2->add(".next(__Vm_rng)");
                randomizep->addStmtsp(പുതിയ AstAssign{
                    fl, പുതിയ AstVarRef{fl, finalOkVarp, VAccess::WRITE}, solverCallp2});

                beginValp = പുതിയ AstAnd{fl, പുതിയ AstVarRef{fl, sizeOkVarp, VAccess::READ},
                                       പുതിയ AstVarRef{fl, finalOkVarp, VAccess::READ}};
            } അല്ലെങ്കിൽ {
                beginValp = solverCallp;
            }
            എങ്കിൽ (randModeVarp) {
                Ast_കണ്ണി_Module* സ്ഥിരം randModeClassp = VN_AS(randModeVarp->user2p(), Class);
                Ast_കണ്ണി_FTask* സ്ഥിരം newp
                    = VN_AS(m_memberMap.findMember(randModeClassp, "new"), കണ്ണി_FTask);
                UASSERT_OBJ(newp, randModeClassp, "No new() in class");
                addSetRandMode(newp, genp, randModeVarp);
            }
            എങ്കിൽ (AstVar* സ്ഥിരം staticRandModeVarp = getStaticRandModeVar(nodep)) {
                // Wire the shared static rand_mode queue into the class generator.
                Ast_കണ്ണി_FTask* സ്ഥിരം newp = VN_AS(m_memberMap.findMember(nodep, "new"), കണ്ണി_FTask);
                UASSERT_OBJ(newp, nodep, "No new() in class");
                addSetStaticRandMode(newp, genp, staticRandModeVarp);
            }
        } അല്ലെങ്കിൽ {
            beginValp = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 32, 1};
        }

        addUpdateRandVarsBody(nodep, randomizep);

        AstFunc* സ്ഥിരം basicRandomizep
            = V3Randomize::newRandomizeFunc(m_memberMap, nodep, BASIC_RANDOMIZE_FUNC_NAME);
        addBasicRandomizeBody(basicRandomizep, nodep, randModeVarp);
        // A basicFirst owner basic-randomizes first, then the solver overrides the
        // constrained leaves, so a globally-constrained leaf (not user3) is still
        // basic-randomized when its type is randomized standalone.
        AstVarRef* സ്ഥിരം fvarRefp = പുതിയ AstVarRef{fl, fvarp, VAccess::WRITE};
        randomizep->addStmtsp(പുതിയ AstAssign{
            fl, fvarRefp, basicFirst ? പുതിയ AstFuncRef{fl, basicRandomizep} : beginValp});

        സ്ഥിരം auto sizeArraysIt = m_sizeConstrainedArrays.find(nodep);
        സ്ഥിരം തർക്കം needsSizePhase
            = sizeArraysIt != m_sizeConstrainedArrays.end() && !sizeArraysIt->second.empty();
        // Size-only resize fallback (no element constraint, so no two-pass phase).
        // Must run after the solver .next() that sets the size variable; emit it
        // after whichever assignment below holds the solver call.
        സ്ഥിരം auto emitResizeFallback = [&]() {
            എങ്കിൽ (needsSizePhase) തിരികെ;
            എങ്കിൽ (AstTask* സ്ഥിരം resizeAllTaskp
                = VN_AS(m_memberMap.findMember(nodep, "__Vresize_constrained_arrays"), Task)) {
                AstTaskRef* സ്ഥിരം resizeTaskRefp = പുതിയ AstTaskRef{fl, resizeAllTaskp};
                randomizep->addStmtsp(resizeTaskRefp->makeStmt());
            }
        };
        എങ്കിൽ (!basicFirst) emitResizeFallback();

        AstVarRef* സ്ഥിരം fvarRefReadp = fvarRefp->clone_വൃക്ഷം(അസത്യം);
        fvarRefReadp->access(VAccess::READ);

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം secondHalfp
            = basicFirst ? beginValp : പുതിയ AstFuncRef{fl, basicRandomizep};
        randomizep->addStmtsp(പുതിയ AstAssign{fl, fvarRefp->clone_വൃക്ഷം(അസത്യം),
                                            പുതിയ AstAnd{fl, fvarRefReadp, secondHalfp}});

        എങ്കിൽ (basicFirst) emitResizeFallback();

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        എങ്കിൽ (classHasRandClassMembers(nodep)) {
            AstTask* സ്ഥിരം postTaskp = getCreateNestedCallbackTask(nodep, "post");
            populateNestedCallbackTask(postTaskp, nodep, "post_randomize");
            randomizep->addStmtsp((പുതിയ AstTaskRef{fl, postTaskp})->makeStmt());
        }

        addPrePostCall(nodep, randomizep, "post_randomize");
        nodep->user1(അസത്യം);
    }
    ശൂന്യം visit(AstRandCase* nodep) override {
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
        Ast_കണ്ണി_DType* സ്ഥിരം sumDTypep = nodep->findUInt64DType();

        File_വരി* സ്ഥിരം fl = nodep->fileline();
        സ്ഥിരം std::string name = "__Vrandcase" + cvtToStr(m_rand_Case_അക്കം++);
        AstVar* സ്ഥിരം randVarp = പുതിയ AstVar{fl, VVarType::BLOCKTEMP, name, sumDTypep};
        randVarp->noSubst(സത്യം);
        randVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        addTempVarsp(randVarp);
        Ast_കണ്ണി_വ്യഞ്ജകം* sump = പുതിയ AstConst{fl, AstConst::Widthed_മൂല്യം{}, 64, 0};
        Ast_കണ്ണി_If* സ്ഥിരം firstIfsp
            = പുതിയ AstIf{fl, പുതിയ AstConst{fl, AstConst::BitFalse{}}, nullptr, nullptr};
        Ast_കണ്ണി_If* ifsp = firstIfsp;

        വേണ്ടി (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = itemp->condsp()->unlinkFrBack();
            sump
                = പുതിയ AstAdd{condp->fileline(), sump, പുതിയ AstExtend{itemp->fileline(), condp, 64}};
            Ast_കണ്ണി* സ്ഥിരം stmtsp
                = itemp->stmtsp() ? itemp->stmtsp()->unlinkFrBackWithNext() : nullptr;
            AstVarRef* സ്ഥിരം randVarRefp = പുതിയ AstVarRef{fl, randVarp, VAccess::WRITE};
            Ast_കണ്ണി_If* സ്ഥിരം newifp
                = പുതിയ AstIf{itemp->fileline(),
                            പുതിയ AstLte{condp->fileline(), randVarRefp, sump->clone_വൃക്ഷം_Pure(സത്യം)},
                            stmtsp, nullptr};
            ifsp->addElsesp(newifp);
            ifsp = newifp;
        }
        AstDisplay* സ്ഥിരം dispp = പുതിയ AstDisplay{
            fl, VDisplayType::DT_ERROR, "All randcase items had 0 weights (IEEE 1800-2023 18.16)",
            nullptr, nullptr};
        UASSERT_OBJ(m_modp, nodep, "randcase not under module");
        dispp->fmtp()->timeunit(m_modp->timeunit());
        ifsp->addElsesp(dispp);

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം randp = പുതിയ AstRand{fl, nullptr, അസത്യം};
        randp->dtypeSetUInt64();
        AstVarRef* സ്ഥിരം randVarRefp = പുതിയ AstVarRef{fl, randVarp, VAccess::WRITE};
        Ast_കണ്ണി* സ്ഥിരം newp
            = പുതിയ AstAssign{fl, randVarRefp,
                            പുതിയ AstAdd{fl, പുതിയ AstConst{fl, AstConst::Unsized64{}, 1},
                                       പുതിയ AstModDiv{fl, randp, sump}}};
        newp->addNext(firstIfsp);
        എങ്കിൽ (debug() >= 9) newp->dump_വൃക്ഷം_And_Next(cout, "-  rcnew: ");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (nodep->name() == "rand_mode") {
            AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(nodep, MethodCall);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = methodCallp ? methodCallp->fromp() : nullptr;
            സ്ഥിരം RandModeTarget randModeTarget = RandModeTarget::get(fromp, m_modp);
            UASSERT_OBJ(randModeTarget.classp, nodep,
                        "Should have checked in RandomizeMarkVisitor");
            AstVar* സ്ഥിരം receiverp = randModeTarget.receiverp;
            സ്ഥിരം തർക്കം isClassLevel = !(receiverp && receiverp->rand().isRand());
            // Class-level rand_mode(N) must also flush the shared static array.
            Ast_കണ്ണി* classLevelStaticLoopp = nullptr;
            എങ്കിൽ (isClassLevel && nodep->argsp()) {
                എങ്കിൽ (AstVar* സ്ഥിരം sVarp = getStaticRandModeVar(randModeTarget.classp)) {
                    File_വരി* സ്ഥിരം fl = nodep->fileline();
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം staticLhsp
                        = makeModeAssignLhs(fl, randModeTarget.classp, nullptr, sVarp);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം argClonep = nodep->argsp()->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
                    classLevelStaticLoopp = makeModeSetLoop(fl, staticLhsp, argClonep, m_ftaskp);
                }
            }
            സ്ഥിരം തർക്കം receiverIsStaticRand
                = receiverp && receiverp->rand().isRand() && receiverp->lifetime().isStatic();
            AstVar* സ്ഥിരം randModeVarp = receiverIsStaticRand
                                             ? getStaticRandModeVar(randModeTarget.classp)
                                             : getRandModeVarFromClass(randModeTarget.classp);
            എങ്കിൽ (!randModeVarp) {
                UASSERT_OBJ(isClassLevel && classLevelStaticLoopp, nodep,
                            "Per-instance rand_mode var missing without static fallback");
                UASSERT_OBJ(VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം), nodep, "Should be a statement");
                m_stmtp->replaceWith(classLevelStaticLoopp);
                VL_DO_CLEAR(തള്ളുക_Deletep(m_stmtp), m_stmtp = nullptr);
                തിരികെ;
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = makeModeAssignLhs(nodep->fileline(), randModeTarget.classp,
                                                        randModeTarget.fromp, randModeVarp);
            replaceWithModeAssignAndAppend(
                nodep, receiverp && receiverp->rand().isRand() ? receiverp : nullptr, lhsp,
                classLevelStaticLoopp);
            തിരികെ;
        }

        എങ്കിൽ (nodep->name() == "constraint_mode") {
            AstMethodCall* സ്ഥിരം methodCallp = VN_CAST(nodep, MethodCall);
            Ast_കണ്ണി_വ്യഞ്ജകം* fromp = methodCallp ? methodCallp->fromp() : nullptr;
            AstConstraint* constrp = nullptr;
            AstClass* classp = VN_CAST(m_modp, Class);
            എങ്കിൽ (AstConstraintRef* സ്ഥിരം constrRefp = VN_CAST(fromp, ConstraintRef)) {
                constrp = constrRefp->constrp();
                എങ്കിൽ (constrRefp->fromp()) {
                    fromp = constrRefp->fromp();
                    classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (fromp) {
                classp = VN_AS(fromp->dtypep()->skipRefp(), ClassRefDType)->classp();
            }
            UASSERT_OBJ(classp, nodep, "Failed to find class");
            // Use correct mode variable based on whether constraint is static
            AstVar* സ്ഥിരം constraintModeVarp = (constrp && constrp->isStatic())
                                                   ? getStaticConstraintModeVar(classp)
                                                   : getConstraintModeVar(classp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp
                = makeModeAssignLhs(nodep->fileline(), classp, fromp, constraintModeVarp);
            replaceWithModeAssign(nodep, constrp, lhsp);
            തിരികെ;
        }

        എങ്കിൽ (nodep->name() != "randomize") {
            // Iterate children so nested rand_mode/constraint_mode/randomize calls
            // inside function arguments are still visited and transformed
            iterateChildren(nodep);
            തിരികെ;
        }

        എങ്കിൽ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
            // Handle std::randomize; create wrapper function that calls basicStdRandomization on
            // each varref argument, then transform nodep to call that wrapper
            സ്ഥിരം തർക്കം inStaticContext = m_ftaskp && m_ftaskp->isStatic();
            AstVar* സ്ഥിരം stdrand = createStdRandomGenerator(m_modp);
            AstFunc* സ്ഥിരം randomizeFuncp
                = V3Randomize::newRandomizeStdFunc(m_modp, m_inlineUniqueStdName.get(nodep));
            // When called from a static function, mark helper and stdrand as static
            // so V3Class moves them to __Vclpkg alongside the calling function
            എങ്കിൽ (inStaticContext) {
                stdrand->lifetime(VLifetime::STATIC_EXPLICIT);
                randomizeFuncp->isStatic(സത്യം);
            }
            randomizeFuncp->addStmtsp(
                പുതിയ AstAssign{nodep->fileline(),
                              പുതിയ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                            VAccess::WRITE},
                              പുതിയ AstConst{nodep->fileline(), AstConst::Widthed_മൂല്യം{}, 32, 1}});
            std::unique_ptr<CaptureVisitor> withCapturep;
            പൂർണ്ണസംഖ്യ argn = 0;
            AstWith* സ്ഥിരം withp = nodep->withp();
            വേണ്ടി (സ്ഥിരം AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = argp->exprp();
                AstCMethodHard* സ്ഥിരം basicMethodp = പുതിയ AstCMethodHard{
                    nodep->fileline(),
                    പുതിയ AstVarRef{nodep->fileline(), stdrand, VAccess::READWRITE},
                    VCMethod::RANDOMIZER_BASIC_STD_RANDOMIZATION};
                AstVar* സ്ഥിരം refvarp
                    = പുതിയ AstVar{exprp->fileline(), VVarType::MEMBER,
                                 "__Varg"s + std::to_string(++argn), exprp->dtypep()};
                refvarp->setStdRandomizeArg();
                // Replace argument occurrences in 'with' clause with __Varg* reference.
                എങ്കിൽ (withp) {
                    withp->foreach([&](Ast_കണ്ണി_വ്യഞ്ജകം* exp) {
                        എങ്കിൽ (is_Similar_കണ്ണി(exp, exprp)) {
                            AstVarRef* സ്ഥിരം replaceVar
                                = പുതിയ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE};
                            exp->replaceWith(replaceVar);
                            replaceVar->user1(exp->user1());
                            replaceVar->varp()->user2p(m_modp);
                            VL_DO_DANGLING(തള്ളുക_Deletep(exp), exp);
                        }
                    });
                }
                refvarp->direction(VDirection::REF);
                refvarp->funcLocal(സത്യം);
                refvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                randomizeFuncp->addStmtsp(refvarp);

                സ്ഥിരം size_t width = exprp->width();
                basicMethodp->addPinsp(
                    പുതിയ AstVarRef{exprp->fileline(), refvarp, VAccess::READWRITE});

                basicMethodp->addPinsp(
                    പുതിയ AstConst{nodep->fileline(), AstConst::Unsized64{}, width});
                basicMethodp->dtypeSetBit();

                randomizeFuncp->addStmtsp(പുതിയ AstAssign{
                    nodep->fileline(),
                    പുതിയ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var),
                                  VAccess::WRITE},
                    പുതിയ AstAnd{nodep->fileline(),
                               പുതിയ AstVarRef{nodep->fileline(),
                                             VN_AS(randomizeFuncp->fvarp(), Var), VAccess::READ},
                               basicMethodp}});
            }
            എങ്കിൽ (withp) {
                File_വരി* സ്ഥിരം fl = nodep->fileline();
                withCapturep = std::make_unique<CaptureVisitor>(withp->exprp(), m_modp, nullptr,
                                                                inStaticContext);
                withCapturep->addFunctionArguments(randomizeFuncp);
                // Clear old constraints and variables for std::randomize with clause
                എങ്കിൽ (stdrand) {
                    randomizeFuncp->addStmtsp(
                        implementConstraintsClearAll(randomizeFuncp->fileline(), stdrand));
                }
                Ast_കണ്ണി* സ്ഥിരം capturedTreep = withp->exprp()->unlinkFrBackWithNext();
                randomizeFuncp->addStmtsp(capturedTreep);
                {
                    expandUniqueElementList(capturedTreep);
                    Constraint_വ്യഞ്ജകം_Visitor{
                        nullptr, m_memberMap,   capturedTreep,         randomizeFuncp, stdrand,
                        nullptr, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
                }
                AstCExpr* സ്ഥിരം solverCallp = പുതിയ AstCExpr{fl};
                solverCallp->dtypeSetBit();
                solverCallp->add(പുതിയ AstVarRef{fl, stdrand, VAccess::READWRITE});
                solverCallp->add(".next()");
                AstVar* സ്ഥിരം fvarp = VN_AS(randomizeFuncp->fvarp(), Var);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം andExprp
                    = പുതിയ AstAnd{fl, പുതിയ AstVarRef{fl, fvarp, VAccess::READ}, solverCallp};
                randomizeFuncp->addStmtsp(
                    പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, fvarp, VAccess::WRITE}, andExprp});
                // Remove With nodes as processed
                VL_DO_DANGLING(withp->unlinkFrBack()->delete_വൃക്ഷം(), withp);
            }
            // Replace the node with a call to that function
            nodep->name(randomizeFuncp->name());
            nodep->taskp(randomizeFuncp);
            nodep->dtypeFrom(randomizeFuncp->dtypep());
            എങ്കിൽ (VN_IS(m_modp, Class)) nodep->classOrPackagep(m_modp);
            എങ്കിൽ (withCapturep) nodep->addArgsp(withCapturep->getArgs());
            UINFOTREE(9, nodep, "", "std::rnd-call");
            UINFOTREE(9, randomizeFuncp, "", "std::rnd-func");
            തിരികെ;
        }
        handleRandomizeArgs(nodep);

        AstWith* സ്ഥിരം withp = nodep->withp();
        എങ്കിൽ (!withp) {
            iterateChildren(nodep);
            wrapRandomizeCallWithNullGuard(nodep);
            തിരികെ;
        }
        withp->unlinkFrBack();
        iterateChildren(nodep);

        AstClass* classp = nullptr;
        എങ്കിൽ (AstMethodCall* സ്ഥിരം callp = VN_CAST(nodep, MethodCall)) {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDTypep = callp->fromp()->dtypep();
            UASSERT_OBJ(fromDTypep, callp->fromp(), "Object dtype is not linked");
            സ്ഥിരം AstClassRefDType* സ്ഥിരം classrefdtypep
                = VN_CAST(fromDTypep->skipRefp(), ClassRefDType);
            UASSERT_OBJ(classrefdtypep, callp->fromp(),
                        "Randomize called on expression of non-class type "
                            << fromDTypep->skipRefp()->prettyDTypeNameQ()
                            << " (it should be detected earlier)");
            classp = classrefdtypep->classp();
            UASSERT_OBJ(classp, classrefdtypep, "Class type is unlinked to its ref type");
        } അല്ലെങ്കിൽ {
            classp = VN_CAST(m_modp, Class);
            UASSERT_OBJ(classp, m_modp, "Module not class, should have failed in V3Width");
        }
        എങ്കിൽ (classp->user1()) {
            // We need to first ensure that the class constraints are transformed
            // NOTE: This is safe only because AstClass visit function overwrites all
            // nesting-dependent state variables
            iterate(classp);
        }

        AstVar* സ്ഥിരം classGenp = getRandomGenerator(classp);
        AstVar* സ്ഥിരം localGenp
            = പുതിയ AstVar{nodep->fileline(), VVarType::BLOCKTEMP, "randomizer",
                         classp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR)};
        localGenp->funcLocal(സത്യം);

        AstFunc* സ്ഥിരം randomizeFuncp = V3Randomize::newRandomizeFunc(
            m_memberMap, classp, m_inlineUniqueNames.get(nodep), അസത്യം);

        // A base-handle `randomize() with` must still reach a derived
        // pre/post_randomize. Route them through per-class virtual wrappers
        // when the static handle type participates in inheritance.
        സ്ഥിരം std::pair<തർക്കം, തർക്കം> prePostWrap = classp->isExtended()
                                                      ? buildPrePostVirtualWrappers(classp)
                                                      : std::pair<തർക്കം, തർക്കം>{അസത്യം, അസത്യം};
        എങ്കിൽ (prePostWrap.first) {
            addVirtualPrePostCall(randomizeFuncp, classp, "pre_randomize");
        } അല്ലെങ്കിൽ {
            addPrePostCall(classp, randomizeFuncp, "pre_randomize");
        }

        // Call nested pre_randomize on rand class-type members (IEEE 18.4.1)
        എങ്കിൽ (classHasRandClassMembers(classp)) {
            AstTask* സ്ഥിരം preTaskp = getCreateNestedCallbackTask(classp, "pre");
            എങ്കിൽ (!preTaskp->stmtsp()) {
                populateNestedCallbackTask(preTaskp, classp, "pre_randomize");
            }
            randomizeFuncp->addStmtsp((പുതിയ AstTaskRef{nodep->fileline(), preTaskp})->makeStmt());
        }

        // Detach the expression and prepare variable copies
        സ്ഥിരം CaptureVisitor captured{withp->exprp(), m_modp, classp};
        // Add function arguments
        captured.addFunctionArguments(randomizeFuncp);

        // Add constraints clearing code
        എങ്കിൽ (classGenp) {
            // Clone write_var calls from main randomize() for path-connected variables
            AstFunc* സ്ഥിരം mainRandomizep
                = VN_CAST(m_memberMap.findMember(classp, "randomize"), Func);
            എങ്കിൽ (mainRandomizep && mainRandomizep->stmtsp()) {
                വേണ്ടി (Ast_കണ്ണി* stmtp = mainRandomizep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    തർക്കം foundClearConstraints = അസത്യം;
                    stmtp->foreach([&](AstCMethodHard* methodp) {
                        എങ്കിൽ (methodp->method() == VCMethod::RANDOMIZER_WRITE_VAR
                            || methodp->method() == VCMethod::RANDOMIZER_MARK_RANDC) {
                            randomizeFuncp->addStmtsp(stmtp->clone_വൃക്ഷം(അസത്യം));
                        } അല്ലെങ്കിൽ എങ്കിൽ (methodp->method() == VCMethod::RANDOMIZER_CLEARCONSTRAINTS) {
                            foundClearConstraints = സത്യം;
                        }
                    });
                    എങ്കിൽ (foundClearConstraints) നിർത്തുക;
                }
            }
            randomizeFuncp->addStmtsp(
                implementConstraintsClear(randomizeFuncp->fileline(), classGenp));
        }

        randomizeFuncp->addStmtsp(localGenp);

        AstFunc* സ്ഥിരം basicRandomizeFuncp
            = V3Randomize::newRandomizeFunc(m_memberMap, classp, BASIC_RANDOMIZE_FUNC_NAME);
        AstFuncRef* സ്ഥിരം basicRandomizeFuncCallp
            = പുതിയ AstFuncRef{nodep->fileline(), basicRandomizeFuncp};

        // Copy (derive) class constraints if present
        എങ്കിൽ (classGenp) {
            AstTask* സ്ഥിരം constrSetupFuncp = getCreateConstraintSetupFunc(classp, m_memberMap);
            AstTaskRef* സ്ഥിരം callp = പുതിയ AstTaskRef{nodep->fileline(), constrSetupFuncp};
            randomizeFuncp->addStmtsp(callp->makeStmt());
            randomizeFuncp->addStmtsp(പുതിയ AstAssign{
                nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), localGenp, VAccess::WRITE},
                പുതിയ AstVarRef{nodep->fileline(), VN_AS(classGenp->user2p(), കണ്ണി_Module), classGenp,
                              VAccess::READ}});
        }

        // Set rand mode if present (not needed if classGenp exists and was copied)
        AstVar* സ്ഥിരം randModeVarp = getRandModeVarFromClass(classp);
        എങ്കിൽ (!classGenp && randModeVarp) addSetRandMode(randomizeFuncp, localGenp, randModeVarp);
        എങ്കിൽ (!classGenp) {
            എങ്കിൽ (AstVar* സ്ഥിരം sVarp = getStaticRandModeVar(classp)) {
                addSetStaticRandMode(randomizeFuncp, localGenp, sVarp);
            }
        }

        // Generate constraint setup code and a hardcoded call to the solver
        Ast_കണ്ണി* സ്ഥിരം capturedTreep = withp->exprp()->unlinkFrBackWithNext();
        randomizeFuncp->addStmtsp(capturedTreep);

        // Pre-scan captured tree for .size on dynamic/assoc arrays and replace
        // with size variables, using shared createOrGetSizeVar helper.
        // After CaptureVisitor, fromp() of .size is AstMemberSel (not VarRef),
        // so extract the underlying variable from the MemberSel chain.
        // Collect resize statements to add after solver call, guarded by success.
        Ast_കണ്ണി* inlineResizeStmtsp = nullptr;
        {
            std::vector<AstCMethodHard*> sizeMethodps;
            capturedTreep->foreachAndNext([&](AstCMethodHard* methodp) {
                എങ്കിൽ (methodp->method() == VCMethod::DYN_SIZE
                    || methodp->method() == VCMethod::ASSOC_SIZE) {
                    sizeMethodps.തള്ളുക_back(methodp);
                }
            });
            വേണ്ടി (AstCMethodHard* സ്ഥിരം methodp : sizeMethodps) {
                // Extract array variable from fromp (VarRef or MemberSel after capture)
                AstVar* arrVarp = nullptr;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം വേര്_Exprp = methodp->fromp();
                എങ്കിൽ (AstMemberSel* memberSelp = VN_CAST(വേര്_Exprp, MemberSel)) {
                    arrVarp = memberSelp->varp();
                    ആകുമ്പോൾ (VN_IS(memberSelp->fromp(), MemberSel)) {
                        memberSelp = VN_CAST(memberSelp->fromp(), MemberSel);
                    }
                    // Only variables that are part of object that randomize is called on shall be
                    // randomized (18.6.1), skip randomization if deepest variable is reference to
                    // __Vthis
                    സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = memberSelp->fromp();
                    എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം varRefp = VN_CAST(fromp, VarRef)) {
                        എങ്കിൽ (varRefp->varp() == captured.getThisp()) തുടരുക;
                    } അല്ലെങ്കിൽ {
                        fromp->v3warn(E_UNSUPPORTED, "Unsupported: Unsupported size constraint "
                                                     "inside 'with' statement of type "
                                                         << fromp->dtypep()->prettyTypeName());
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം varRefp = VN_CAST(വേര്_Exprp, VarRef)) {
                    arrVarp = varRefp->varp();
                }
                എങ്കിൽ (!arrVarp) തുടരുക;
                // Only handle rand-declared dynamic/assoc array variables
                എങ്കിൽ (!arrVarp->rand().isRandomizable()) തുടരുക;
                // Do not randomize associative array size (IEEE 18.4)
                എങ്കിൽ (VN_IS(arrVarp->dtypep()->skipRefp(), AssocArrayDType)) തുടരുക;
                File_വരി* സ്ഥിരം fl = methodp->fileline();
                തർക്കം wasCreated = അസത്യം;
                AstClass* സ്ഥിരം varClassp = VN_AS(arrVarp->user2p(), Class);
                AstVar* സ്ഥിരം sizeVarp = createOrGetSizeVar(varClassp, arrVarp, fl,
                                                            methodp->findIntDType(), wasCreated);
                // arrVarp and sizeVarp may live in a base class when the
                // array is inherited; route VarRefs through their declaring
                // class so V3Scope can resolve them.
                എങ്കിൽ (wasCreated) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resizeVarRefp = buildMemberSelExprp(വേര്_Exprp, arrVarp);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sizeExprp = buildMemberSelExprp(വേര്_Exprp, sizeVarp);
                    // Append size >= 0 constraint so ConstraintExprVisitor processes it
                    capturedTreep->addNext(createSizeGteZeroConstraint(fl, sizeVarp, വേര്_Exprp));
                    AstCMethodHard* സ്ഥിരം resizep
                        = പുതിയ AstCMethodHard{fl, resizeVarRefp, VCMethod::DYN_RESIZE, sizeExprp};
                    resizep->dtypep(methodp->findVoidDType());
                    inlineResizeStmtsp
                        = Ast_കണ്ണി::addNext(inlineResizeStmtsp, പുതിയ Ast_Stmt_വ്യഞ്ജകം{fl, resizep});
                }
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം size_കണ്ണി_Exprp = buildMemberSelExprp(വേര്_Exprp, sizeVarp);
                methodp->replaceWith(size_കണ്ണി_Exprp);
                VL_DO_DANGLING(methodp->delete_വൃക്ഷം(), methodp);
            }
        }

        {
            expandUniqueElementList(capturedTreep);
            Constraint_വ്യഞ്ജകം_Visitor{
                classp,       m_memberMap,   capturedTreep,         randomizeFuncp, localGenp,
                randModeVarp, m_writtenVars, &m_uniqueConstraintId, m_uniqueNames,  nullptr};
        }

        // Call the solver and set return value
        AstCExpr* സ്ഥിരം solverCallp = പുതിയ AstCExpr{nodep->fileline()};
        solverCallp->dtypeSetBit();
        solverCallp->add(പുതിയ AstVarRef{nodep->fileline(), localGenp, VAccess::READWRITE});
        solverCallp->add(".next(__Vm_rng)");
        randomizeFuncp->addStmtsp(പുതിയ AstAssign{
            nodep->fileline(),
            പുതിയ AstVarRef{nodep->fileline(), VN_AS(randomizeFuncp->fvarp(), Var), VAccess::WRITE},
            പുതിയ AstAnd{nodep->fileline(), basicRandomizeFuncCallp, solverCallp}});

        // Resize constrained arrays only when solver succeeds, mirroring
        // the class-level if(__Vsize_ok) guard pattern
        എങ്കിൽ (inlineResizeStmtsp) {
            AstVar* സ്ഥിരം retVarp = VN_AS(randomizeFuncp->fvarp(), Var);
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{nodep->fileline(),
                                         പുതിയ AstVarRef{nodep->fileline(), retVarp, VAccess::READ},
                                         inlineResizeStmtsp, nullptr};
            randomizeFuncp->addStmtsp(ifp);
        }

        // Call nested post_randomize on rand class-type members (IEEE 18.4.1)
        എങ്കിൽ (classHasRandClassMembers(classp)) {
            AstTask* സ്ഥിരം postTaskp = getCreateNestedCallbackTask(classp, "post");
            എങ്കിൽ (!postTaskp->stmtsp()) {
                populateNestedCallbackTask(postTaskp, classp, "post_randomize");
            }
            randomizeFuncp->addStmtsp((പുതിയ AstTaskRef{nodep->fileline(), postTaskp})->makeStmt());
        }

        എങ്കിൽ (prePostWrap.second) {
            addVirtualPrePostCall(randomizeFuncp, classp, "post_randomize");
        } അല്ലെങ്കിൽ {
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
        VL_DO_DANGLING(withp->delete_വൃക്ഷം(), withp);
    }
    ശൂന്യം visit(AstConstraint* nodep) override {
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        iterateChildren(nodep);
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        എങ്കിൽ (m_constraintp && nodep->fromp()->user1()
            && (nodep->method() == VCMethod::ASSOC_SIZE
                || nodep->method() == VCMethod::DYN_SIZE)) {
            AstClass* സ്ഥിരം classp = VN_AS(m_modp, Class);
            AstVarRef* സ്ഥിരം queueVarRefp = VN_CAST(nodep->fromp(), VarRef);
            എങ്കിൽ (!queueVarRefp) {
                // Warning from ConstraintExprVisitor will be thrown
                തിരികെ;
            }
            AstVar* സ്ഥിരം queueVarp = queueVarRefp->varp();
            തർക്കം wasCreated = അസത്യം;
            AstVar* സ്ഥിരം sizeVarp
                = createOrGetSizeVar(classp, queueVarp, fl, nodep->findIntDType(), wasCreated);
            എങ്കിൽ (wasCreated) {
                // Associative arrays have no resize(); only generate resize
                // for dynamic arrays and queues
                എങ്കിൽ (!VN_IS(queueVarp->dtypep()->skipRefp(), AssocArrayDType)) {
                    AstTask* resizerTaskp = VN_AS(m_constraintp->user3p(), Task);
                    എങ്കിൽ (!resizerTaskp) {
                        resizerTaskp
                            = newResizeConstrainedArrayTask(classp, m_constraintp->name());
                        m_constraintp->user3p(resizerTaskp);
                    }
                    AstVarRef* സ്ഥിരം sizeVarRefp = പുതിയ AstVarRef{
                        fl, VN_AS(sizeVarp->user2p(), കണ്ണി_Module), sizeVarp, VAccess::READ};
                    AstCMethodHard* സ്ഥിരം resizep = പുതിയ AstCMethodHard{
                        fl, nodep->fromp()->unlinkFrBack(), VCMethod::DYN_RESIZE, sizeVarRefp};
                    resizep->dtypep(nodep->findVoidDType());
                    resizerTaskp->addStmtsp(പുതിയ Ast_Stmt_വ്യഞ്ജകം{fl, resizep});
                }

                // Since size variable is signed int, we need additional constraint
                // to make sure it is always >= 0.
                m_constraintp->addItemsp(createSizeGteZeroConstraint(fl, sizeVarp));
            }
            AstVarRef* സ്ഥിരം sizeVarRefp = പുതിയ AstVarRef{fl, VN_AS(sizeVarp->user2p(), കണ്ണി_Module),
                                                         sizeVarp, VAccess::READ};
            sizeVarRefp->user1(സത്യം);
            nodep->replaceWith(sizeVarRefp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        VL_RESTORER(m_stmtp);
        m_stmtp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
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
        nodep->foreach([&](AstClass* സ്ഥിരം classp) {
            എങ്കിൽ (!classp->user1()) തിരികെ;
            classp->foreachMember([&](AstClass* സ്ഥിരം ownerClassp, AstConstraint* സ്ഥിരം constrp) {
                constrp->foreach([&](AstMemberSel* സ്ഥിരം memberSelp) {
                    AstVar* സ്ഥിരം varp = memberSelp->varp();
                    എങ്കിൽ (VN_IS(varp->dtypep()->skipRefp(), ClassRefDType)) തിരികെ;
                    എങ്കിൽ (VN_AS(varp->user2p(), കണ്ണി_Module) != ownerClassp) തിരികെ;
                    എങ്കിൽ (!varp->user3()) varp->user3(സത്യം);
                });
            });
        });

        iterate(nodep);
        nodep->foreach([&](AstConstraint* constrp) {
            VL_DO_DANGLING(തള്ളുക_Deletep(constrp->unlinkFrBack()), constrp);
        });
    }
    ~RandomizeVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Randomize method class functions

ശൂന്യം V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        സ്ഥിരം RandomizeMarkVisitor markVisitor{nodep};
        സ്ഥിരം RandomizeVisitor randomizeVisitor{nodep};
    }
    V3Global::dump_Check_Global_വൃക്ഷം("randomize", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(VMemberMap& memberMap, AstClass* nodep,
                                       സ്ഥിരം std::string& name, തർക്കം allowVirtual,
                                       തർക്കം childDType) {
    AstFunc* funcp = VN_AS(memberMap.findMember(nodep, name), Func);
    എങ്കിൽ (!funcp) {
        v3Global.useRandomizeMethods(സത്യം);
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep
            = childDType
                  ? പുതിയ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::INT}
                  : nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        AstVar* സ്ഥിരം fvarp = childDType
                                  ? പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                               VFlagChildDType{}, dtypep}
                                  : പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
        fvarp->fileline()->warnOff(V3ErrorCode::NORETURN, സത്യം);
        fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        fvarp->funcLocal(സത്യം);
        fvarp->funcReturn(സത്യം);
        fvarp->direction(VDirection::OUTPUT);
        nodep->addMembersp(funcp);
        funcp = പുതിയ AstFunc{nodep->fileline(), name, nullptr, fvarp};
        എങ്കിൽ (!childDType) funcp->dtypep(dtypep);
        funcp->classMethod(സത്യം);
        funcp->isVirtual(allowVirtual && nodep->isExtended());
        nodep->addMembersp(funcp);
        memberMap.തിരുകുക(nodep, funcp);
    }
    തിരികെ funcp;
}

AstFunc* V3Randomize::newRandomizeStdFunc(Ast_കണ്ണി_Module* nodep, സ്ഥിരം std::string& name) {
    AstFunc* funcp = nullptr;
    v3Global.useRandomizeMethods(സത്യം);
    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
    AstVar* സ്ഥിരം fvarp = പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, name, dtypep};
    fvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    fvarp->funcLocal(സത്യം);
    fvarp->funcReturn(സത്യം);
    fvarp->direction(VDirection::OUTPUT);
    funcp = പുതിയ AstFunc{nodep->fileline(), name, nullptr, fvarp};
    funcp->dtypep(dtypep);
    എങ്കിൽ (VN_IS(nodep, Class)) {
        funcp->classMethod(സത്യം);
    } അല്ലെങ്കിൽ {
        funcp->classMethod(സത്യം);
        funcp->isStatic(സത്യം);
    }
    nodep->addStmtsp(funcp);
    തിരികെ funcp;
}

AstFunc* V3Randomize::newSRandomFunc(VMemberMap& memberMap, AstClass* nodep) {
    AstClass* സ്ഥിരം basep = nodep->baseMostClassp();
    AstFunc* funcp = VN_AS(memberMap.findMember(basep, "srandom"), Func);
    എങ്കിൽ (!funcp) {
        v3Global.useRandomizeMethods(സത്യം);
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep
            = basep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says argument 0/1
        AstVar* സ്ഥിരം ivarp = പുതിയ AstVar{basep->fileline(), VVarType::MEMBER, "seed", dtypep};
        ivarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        ivarp->funcLocal(സത്യം);
        ivarp->direction(VDirection::നിവേശം);
        funcp = പുതിയ AstFunc{basep->fileline(), "srandom", ivarp, nullptr};
        funcp->dtypep(basep->findVoidDType());
        funcp->classMethod(സത്യം);
        funcp->isVirtual(അസത്യം);
        basep->addMembersp(funcp);
        memberMap.തിരുകുക(nodep, funcp);
        // For std::process, seed the per-process RNG via m_process->srandom()
        // For regular classes, seed the per-object RNG via __Vm_rng
        എങ്കിൽ (basep->name() == "process") {
            funcp->addStmtsp(പുതിയ AstCStmt{basep->fileline(), "m_process->srandom(seed);"});
        } അല്ലെങ്കിൽ {
            funcp->addStmtsp(പുതിയ AstCStmt{basep->fileline(), "__Vm_rng.srandom(seed);"});
            basep->needRNG(സത്യം);
        }
    }
    തിരികെ funcp;
}
