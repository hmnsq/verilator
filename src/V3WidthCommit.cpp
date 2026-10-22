// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Commit expression widths
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
//
// Remove all $signed, $unsigned, we're done with them.
//
// This step is only called on real V3Width, not intermediate e.g. widthParams
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3WidthCommit.h"

#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
ರಚನೆ PairHash final {
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T, ಬಗೆ_ಹೆಸರು U>
    std::ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ std::pair<T, U>& x) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ std::hash<T>()(x.ಮೊದಲ) + std::hash<U>()(x.ಎರಡನೇ);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

ವರ್ಗ ಅಗಲ_Commit_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // STATE
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_contNbap = ಶೂನ್ಯಸೂಚಕ;  // In continuous- or non-blocking assignment
    ತಾರ್ಕಿಕ m_contReads = ಸುಳ್ಳು;  // Check read continuous automatic variables
    ತಾರ್ಕಿಕ m_dynsizedelem = ಸುಳ್ಳು;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    ತಾರ್ಕಿಕ m_taskRefWarn = ನಿಜ;  // Allow task reference warnings
    ತಾರ್ಕಿಕ m_underSel = ಸುಳ್ಳು;  // Under AstMemberSel or AstSel
    ತಾರ್ಕಿಕ m_underAlwaysEdged = ಸುಳ್ಳು;  // Under always with sequential SenTree
    std::ಸದಿಶ<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::ಸದಿಶ<Ast_ಗಂಟು_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

ಸಾರ್ವಜನಿಕ:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

ಖಾಸಗಿ:
    // METHODS
    ಶೂನ್ಯ editDType(Ast_ಗಂಟು* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        ಆದರೆ (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warn_ಇನ್ನಷ್ಟು() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_ಗಂಟು_DType* editOneDType(Ast_ಗಂಟು_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // Recurse to handle the data type, as may change the size etc of this type
        ಆದರೆ (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        ಆದರೆ (AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ಸ್ಥಿರಾಂಕ newp = nodep->ಕಂಡುಹಿಡಿ_ಸೇರಿಸು_Same_DType(bdtypep);
            ಆದರೆ (newp != bdtypep && debug() >= 9) {
                UINFO_ಪೂರ್ವಪ್ರತ್ಯಯ("dtype replacement ");
                nodep->dumpSmall(std::ಹೊರಗೆ);
                std::ಹೊರಗೆ << "  ---->  ";
                newp->dumpSmall(std::ಹೊರಗೆ);
                std::ಹೊರಗೆ << '\n';
            }
            ಹಿಂತಿರುಗಿ newp;
        }
        ಹಿಂತಿರುಗಿ nodep;
    }
    ಶೂನ್ಯ classEncapCheck(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        ತಾರ್ಕಿಕ local = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ prot = ಸುಳ್ಳು;
        ಆದರೆ (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ anodep = VN_CAST(defp, ಗಂಟು_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        ಆದರೆ (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ಸ್ಥಿರಾಂಕ AstClass* refClassp = VN_CAST(m_modp, Class);
            ಆದರೆ (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* how = ಶೂನ್ಯಸೂಚಕ;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ nestedAccess = [refClassp](ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* memberp) {
                ಹಿಂತಿರುಗಿ memberp == refClassp;
            };
            ಆದರೆ (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            ಆದರೆ (how) {
                UINFO(9, "refclass " << refClassp);
                UINFO(9, "defclass " << defClassp);
                nodep->v3warn(ENCAPSULATED, nodep->prettyNameQ()
                                                << " is hidden as " << how
                                                << " within this context (IEEE 1800-2023 8.18)\n"
                                                << nodep->warnContextPrimary() << "\n"
                                                << nodep->warnOther()
                                                << "... Location of definition\n"
                                                << defp->warnContextSecondary());
            }
        }
    }
    ಶೂನ್ಯ varLifetimeCheck(Ast_ಗಂಟು* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        ಆದರೆ (!m_underSel && m_contNbap) {
            std::string varType;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ varDtp = varp->dtypep()->skipRefp();
            ಆದರೆ (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            ಆದರೆ (!varType.ಖಾಲಿಗಳು()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    ಶೂನ್ಯ selLifetimeCheck(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* namep) {
        ಆದರೆ (m_underSel || !m_contNbap || !m_dynsizedelem) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nodep->isLValue() && !m_contReads) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!nodep->dtypep()->skipRefp()->isDynamicallySized()) ಹಿಂತಿರುಗಿ;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    ಶೂನ್ಯ deadCheckTasks() {
        ಗಾಗಿ (Ast_ಗಂಟು_FTask* taskp : m_tasksp) {
            ಆದರೆ (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        ಗಾಗಿ (AstNew* newp : m_virtualNewsp) {
            ಆದರೆ (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_ಗಂಟು::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    ಶೂನ್ಯ virtualRecurse(Ast_ಗಂಟು_FTask* nodep, ಸ್ಥಿರಾಂಕ AstClass* classp, ತಾರ್ಕಿಕ& isVirtualr) {
        ಸ್ಥಿರ std::unordered_map<std::pair<ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask*>, ತಾರ್ಕಿಕ, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        ಸ್ವಯಂಚಾಲಿತ pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        ಆದರೆ (!pair.ಎರಡನೇ) {
            ಆದರೆ (pair.ಮೊದಲ->ಎರಡನೇ) isVirtualr = ನಿಜ;
            ಹಿಂತಿರುಗಿ;
        }
        //
        ಆದರೆ (nodep->isVirtual()) isVirtualr = ನಿಜ;
        // Propagate value from extends/implements
        ಗಾಗಿ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ eclassp = extendsp->classp();
            ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ fbasep
                = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(eclassp, nodep->name()), ಗಂಟು_FTask)) {
                ಆದರೆ (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    ಮುಂದುವರಿ;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        ಆದರೆ (isVirtualr) {
            nodep->isVirtual(ನಿಜ);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(nodep, Class)) {
            ಗಾಗಿ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ ebasep = extendsp->classp();
                ಆದರೆ (ebasep->ತಳ_Override().isFinal()) {
                    extendsp->v3error("Class " << nodep->prettyNameQ()
                                               << " is being extended from class marked ':final'"
                                                  " (IEEE 1800-2023 8.20)\n"
                                               << extendsp->warnContextPrimary() << "\n"
                                               << ebasep->warnOther()
                                               << "... Location of ':final' class being extended\n"
                                               << ebasep->warnContextSecondary());
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        ಆದರೆ (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->is_Combo_ನಕ್ಷತ್ರ()) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ noReads = nodep->forall(
                [&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* refp) { ಹಿಂತಿರುಗಿ !refp->access().isReadOrRW(); });
            ಆದರೆ (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warn_ಇನ್ನಷ್ಟು()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->has_ಅಂಚು();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        ಆದರೆ (!nodep->isTimingControl()) m_taskRefWarn = ಸುಳ್ಳು;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        ಆದರೆ (nodep->sensesp() && nodep->sensesp()->is_Combo_ನಕ್ಷತ್ರ()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Attr_ದ* nodep) override {
        ಆಯ್ಕೆ (nodep->attrType()) {
        ಸ್ಥಿತಿ VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        ಸ್ಥಿತಿ VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
            ಹಿಂತಿರುಗಿ;
        ಪೂರ್ವನಿಯೋಜಿತ:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassExtends* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        ಆದರೆ (nodep->argsp()) ತಳ್ಳು_Deletep(nodep->argsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ newp = V3_ಅಗಲ_Commit::new_If_Const_Commit_ಗಾತ್ರ(nodep)) {
            nodep->replaceWith(newp);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(m_modp, Class);
            ಆದರೆ (nodep->is_Kwd_ಶುದ್ಧ()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        ಭೇಟಿ_Iterate_ಗಂಟು_DType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        ಭೇಟಿ_Iterate_ಗಂಟು_DType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParamTypeDType* nodep) override {
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        ಭೇಟಿ_Iterate_ಗಂಟು_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlink_Fr_ಹಿಂಭಾಗ();  // Make non-child
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {
        ಭೇಟಿ_Iterate_ಗಂಟು_DType(nodep);
        ಆದರೆ (!nodep->typedefp()) ಹಿಂತಿರುಗಿ;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(ಶೂನ್ಯಸೂಚಕ);  // No longer needed
    }
    ಶೂನ್ಯ ಭೇಟಿ_Iterate_ಗಂಟು_DType(Ast_ಗಂಟು_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        ಆದರೆ (nodep->user1_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;  // Process once
        nodep->ಅಗಲ_Min_From_ಅಗಲ();
        // Too late to any unspecified sign to be anything but unsigned
        ಆದರೆ (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        ಆದರೆ (!nodep->task_ಬಹಿರಂಗ() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ತಾರ್ಕಿಕ isVirtual = ಸುಳ್ಳು;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        ಆದರೆ (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(m_modp, Class);
            ಆದರೆ (nodep->class_ವಿಧಾನ() && nodep->ಶುದ್ಧ_Virtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        ತಾರ್ಕಿಕ extended = ಸುಳ್ಳು;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            ಗಾಗಿ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ eclassp = extendsp->classp();
                ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ fbasep
                    = VN_CAST(m_memberMap.ಕಂಡುಹಿಡಿ_Member(eclassp, nodep->name()), ಗಂಟು_FTask)) {
                    ಆದರೆ (fbasep != nodep) {
                        extended = ನಿಜ;
                        ಆದರೆ (nodep->ತಳ_Override().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        ಆದರೆ (fbasep->ತಳ_Override().isFinal()) {
                            nodep->v3error(
                                "Member "
                                << nodep->prettyNameQ()
                                << " is being extended from member marked ':final'"
                                   " (IEEE 1800-2023 8.20)\n"
                                << nodep->warnContextPrimary() << "\n"
                                << fbasep->warnOther()
                                << "... Location of ':final' declaration being extended\n"
                                << fbasep->warnContextSecondary());
                        }
                    }
                }
            }
        }
        ಆದರೆ (!extended && nodep->ತಳ_Override().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        ಆದರೆ (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        ಆದರೆ (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        ಆದರೆ (m_underAlwaysEdged) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ignore = nodep->lhsp()->forall([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                ಆದರೆ (refp->access().isReadOnly()) ಹಿಂತಿರುಗಿ ನಿಜ;
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = refp->varp();
                // Ignore ...
                ಹಿಂತಿರುಗಿ varp->is_ಬಳಸಿದ_Loop_Idx()  // ... loop indices
                       || varp->is_ತಾತ್ಕಾಲಿಕ()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::BLKSEQ);  // ... user said so
            });
            ಆದರೆ (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warn_ಇನ್ನಷ್ಟು()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Cont* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
            iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->timingControlp());
        iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = ಸುಳ್ಳು;
            iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {
        iterate_And_ಮುಂದಿನ_Null(nodep->timingControlp());
        iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ಸುಳ್ಳು;
            iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        ಆದರೆ (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
            && m_ftaskp->verilogFunction() && m_taskRefWarn) {
            nodep->v3warn(FUNCTIMECTL,
                          "Functions cannot invoke tasks (IEEE 1800-2023 13.4)\n"
                              << nodep->warnContextPrimary() << "\n"
                              << nodep->warn_ಇನ್ನಷ್ಟು() << "... Suggest make caller 'function "
                              << m_ftaskp->prettyName() << "' a task\n"
                              << m_ftaskp->warnContextSecondary() << "\n"
                              << nodep->warn_ಇನ್ನಷ್ಟು() << "... Or, suggest make called 'task "
                              << nodep->taskp()->prettyName() << "' a function void\n"
                              << nodep->taskp()->warnContextSecondary());
        }
        ಆದರೆ (nodep->taskp()) nodep->taskp()->user2(1);
        ಆದರೆ (AstNew* ಸ್ಥಿರಾಂಕ newp = VN_CAST(nodep, New)) {
            ಆದರೆ (!VN_IS(newp->backp(), ನಿಯೋಜಿಸು)) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(newp->classOrPackagep(), Class)) {
                ಆದರೆ (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.ತಳ್ಳು_ಹಿಂಭಾಗ(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = ನಿಜ;
            iterateChildren(nodep);
        }
        editDType(nodep);
        ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        ಆದರೆ (nodep->isFuncLocal() && nodep->direction() == VDirection::ಒಳಹರಿವು && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            ತಳ್ಳು_Deletep(nodep->valuep()->unlink_Fr_ಹಿಂಭಾಗ());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        ಆದರೆ (nodep->ವಿಧಾನ() == VCMethod::ARRAY_AT || nodep->ವಿಧಾನ() == VCMethod::ARRAY_AT_WRITE
            || nodep->ವಿಧಾನ() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = ನಿಜ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = ನಿಜ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = ನಿಜ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->ತಳ_Fromp(/* overMembers: */ ಸುಳ್ಳು));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = ನಿಜ;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಅಗಲ_Commit_Visitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ಸ್ಥಿರಾಂಕ VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_ಗಂಟು_Module* modp) {
                ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstIface* ಸ್ಥಿರಾಂಕ ifacep = VN_CAST(modp, Iface)) {
                    ifacep->dtypeNameFreeze();
                }
            });
        }
        // Were changing widthMin's, so the table is now somewhat trashed
        nodep->typeTablep()->clearCache();
        iterate(nodep);
        deadCheckTasks();
        // Don't want to AstTypeTable::repairCache, as all needed nodes
        // have been added back in; a repair would prevent dead nodes from
        // being detected
    }
    ~ಅಗಲ_Commit_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// V3WidthCommit class functions

ಶೂನ್ಯ V3_ಅಗಲ_Commit::ಅಗಲ_Commit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ಅಗಲ_Commit_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("widthcommit", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
