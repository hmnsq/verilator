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
ಸಂರಚನಾ PairHash final {
    template <typename T, typename U>
    std::size_t operator()(ಧ್ರುವ std::pair<T, U>& x) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

ವರ್ಗ WidthCommitVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // STATE
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    ಧ್ರುವ ವರ್ಣ* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    ತಾರ್ಕಿಕ m_contReads = ಅಸತ್ಯ;  // Check read continuous automatic variables
    ತಾರ್ಕಿಕ m_dynsizedelem = ಅಸತ್ಯ;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    ತಾರ್ಕಿಕ m_taskRefWarn = ಸತ್ಯ;  // Allow task reference warnings
    ತಾರ್ಕಿಕ m_underSel = ಅಸತ್ಯ;  // Under AstMemberSel or AstSel
    ತಾರ್ಕಿಕ m_underAlwaysEdged = ಅಸತ್ಯ;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_ಗಂಟು_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

ಸಾರ್ವಜನಿಕ:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

ಗುಪ್ತ:
    // METHODS
    ಶೂನ್ಯ editDType(Ast_ಗಂಟು* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        ಯದಿ (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_ಗಂಟು_DType* editOneDType(Ast_ಗಂಟು_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        ಯದಿ (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        ಯದಿ (AstBasicDType* ಧ್ರುವ bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ಧ್ರುವ newp = nodep->find_ಸೇರಿಸು_Same_DType(bdtypep);
            ಯದಿ (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            ಪ್ರತ್ಯಾಗಮ newp;
        }
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    ಶೂನ್ಯ classEncapCheck(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        ತಾರ್ಕಿಕ local = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ prot = ಅಸತ್ಯ;
        ಯದಿ (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಧ್ರುವ auto anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ auto anodep = VN_CAST(defp, ಗಂಟು_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ auto anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        ಯದಿ (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ಧ್ರುವ AstClass* refClassp = VN_CAST(m_modp, Class);
            ಯದಿ (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ಧ್ರುವ ವರ್ಣ* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ಧ್ರುವ auto nestedAccess = [refClassp](ಧ್ರುವ AstClass*, ಧ್ರುವ Ast_ಗಂಟು* memberp) {
                ಪ್ರತ್ಯಾಗಮ memberp == refClassp;
            };
            ಯದಿ (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } ಅನ್ಯಥಾ ಯದಿ (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            ಯದಿ (how) {
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
        ಯದಿ (!m_underSel && m_contNbap) {
            std::string varType;
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ varDtp = varp->dtypep()->skipRefp();
            ಯದಿ (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            ಅನ್ಯಥಾ ಯದಿ (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            ಅನ್ಯಥಾ ಯದಿ (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            ಯದಿ (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    ಶೂನ್ಯ selLifetimeCheck(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ಧ್ರುವ Ast_ಗಂಟು* namep) {
        ಯದಿ (m_underSel || !m_contNbap || !m_dynsizedelem) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nodep->isLValue() && !m_contReads) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nodep->dtypep()->skipRefp()->isDynamicallySized()) ಪ್ರತ್ಯಾಗಮ;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    ಶೂನ್ಯ deadCheckTasks() {
        ಪ್ರತಿ (Ast_ಗಂಟು_FTask* taskp : m_tasksp) {
            ಯದಿ (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        ಪ್ರತಿ (AstNew* newp : m_virtualNewsp) {
            ಯದಿ (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_ಗಂಟು::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    ಶೂನ್ಯ virtualRecurse(Ast_ಗಂಟು_FTask* nodep, ಧ್ರುವ AstClass* classp, ತಾರ್ಕಿಕ& isVirtualr) {
        ಸ್ಥಿರ std::unordered_map<std::pair<ಧ್ರುವ AstClass*, ಧ್ರುವ Ast_ಗಂಟು_FTask*>, ತಾರ್ಕಿಕ, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        auto pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        ಯದಿ (!pair.second) {
            ಯದಿ (pair.first->second) isVirtualr = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ;
        }
        //
        ಯದಿ (nodep->isVirtual()) isVirtualr = ಸತ್ಯ;
        // Propagate value from extends/implements
        ಪ್ರತಿ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ಧ್ರುವ AstClass* ಧ್ರುವ eclassp = extendsp->classp();
            ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), ಗಂಟು_FTask)) {
                ಯದಿ (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    ಅನುವರ್ತ;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        ಯದಿ (isVirtualr) {
            nodep->isVirtual(ಸತ್ಯ);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(nodep, Class)) {
            ಪ್ರತಿ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ಧ್ರುವ AstClass* ಧ್ರುವ ebasep = extendsp->classp();
                ಯದಿ (ebasep->baseOverride().isFinal()) {
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
    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        ಯದಿ (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            ಧ್ರುವ ತಾರ್ಕಿಕ noReads = nodep->forall(
                [&](ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* refp) { ಪ್ರತ್ಯಾಗಮ !refp->access().isReadOrRW(); });
            ಯದಿ (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        ಯದಿ (!nodep->isTimingControl()) m_taskRefWarn = ಅಸತ್ಯ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        ಯದಿ (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstAttrOf* nodep) override {
        ವಿಕಲ್ಪ (nodep->attrType()) {
        ಪಕ್ಷ VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        ಪಕ್ಷ VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        ಸ್ವಭಾವ:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstClassExtends* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        ಯದಿ (nodep->argsp()) ತಳ್ಳು_Deletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        ಯದಿ (AstConst* ಧ್ರುವ newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            Ast_ಗಂಟು* ಧ್ರುವ oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(m_modp, Class);
            ಯದಿ (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_ಗಂಟು_DType(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        visit_Iterate_ಗಂಟು_DType(nodep);
    }
    ಶೂನ್ಯ visit(AstParamTypeDType* nodep) override {
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        visit_Iterate_ಗಂಟು_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        visit_Iterate_ಗಂಟು_DType(nodep);
        ಯದಿ (!nodep->typedefp()) ಪ್ರತ್ಯಾಗಮ;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    ಶೂನ್ಯ visit_Iterate_ಗಂಟು_DType(Ast_ಗಂಟು_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        ಯದಿ (nodep->user1SetOnce()) ಪ್ರತ್ಯಾಗಮ;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        ಯದಿ (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        ಯದಿ (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.ತಳ್ಳು_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ತಾರ್ಕಿಕ isVirtual = ಅಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        ಯದಿ (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(m_modp, Class);
            ಯದಿ (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        ತಾರ್ಕಿಕ extended = ಅಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            ಪ್ರತಿ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ಧ್ರುವ AstClass* ಧ್ರುವ eclassp = extendsp->classp();
                ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), ಗಂಟು_FTask)) {
                    ಯದಿ (fbasep != nodep) {
                        extended = ಸತ್ಯ;
                        ಯದಿ (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        ಯದಿ (fbasep->baseOverride().isFinal()) {
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
        ಯದಿ (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        ಯದಿ (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        ಯದಿ (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    ಶೂನ್ಯ visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        ಯದಿ (m_underAlwaysEdged) {
            ಧ್ರುವ ತಾರ್ಕಿಕ ignore = nodep->lhsp()->forall([&](ಧ್ರುವ AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                ಯದಿ (refp->access().isReadOnly()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                ಧ್ರುವ AstVar* ಧ್ರುವ varp = refp->varp();
                // Ignore ...
                ಪ್ರತ್ಯಾಗಮ varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            ಯದಿ (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    ಶೂನ್ಯ visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ಸತ್ಯ;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = ಅಸತ್ಯ;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ಅಸತ್ಯ;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        ಯದಿ (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
            && m_ftaskp->verilogFunction() && m_taskRefWarn) {
            nodep->v3warn(FUNCTIMECTL,
                          "Functions cannot invoke tasks (IEEE 1800-2023 13.4)\n"
                              << nodep->warnContextPrimary() << "\n"
                              << nodep->warnMore() << "... Suggest make caller 'function "
                              << m_ftaskp->prettyName() << "' a task\n"
                              << m_ftaskp->warnContextSecondary() << "\n"
                              << nodep->warnMore() << "... Or, suggest make called 'task "
                              << nodep->taskp()->prettyName() << "' a function void\n"
                              << nodep->taskp()->warnContextSecondary());
        }
        ಯದಿ (nodep->taskp()) nodep->taskp()->user2(1);
        ಯದಿ (AstNew* ಧ್ರುವ newp = VN_CAST(nodep, New)) {
            ಯದಿ (!VN_IS(newp->backp(), Assign)) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(newp->classOrPackagep(), Class)) {
                ಯದಿ (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.ತಳ್ಳು_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = ಸತ್ಯ;
            iterateChildren(nodep);
        }
        editDType(nodep);
        ಯದಿ (AstClassRefDType* ಧ್ರುವ classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        ಯದಿ (nodep->isFuncLocal() && nodep->direction() == VDirection::ಒಳಹರಿವು && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            ತಳ್ಳು_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        ಯದಿ (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = ಸತ್ಯ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = ಸತ್ಯ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = ಸತ್ಯ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ ಅಸತ್ಯ));
    }
    ಶೂನ್ಯ visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = ಸತ್ಯ;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ಧ್ರುವ VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_ಗಂಟು_Module* modp) {
                ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } ಅನ್ಯಥಾ ಯದಿ (AstIface* ಧ್ರುವ ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// V3WidthCommit class functions

ಶೂನ್ಯ V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("widthcommit", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
