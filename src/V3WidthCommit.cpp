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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3WidthCommit.h"

#অন্তর্ভাব "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
সংরচনা PairHash final {
    template <typename T, typename U>
    std::size_t operator()(ধ্রুব std::pair<T, U>& x) ধ্রুব {
        প্রত্যাগম std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

বর্গ WidthCommitVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;

    // STATE
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    ধ্রুব বর্ণ* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    তার্কিক m_contReads = অসত্য;  // Check read continuous automatic variables
    তার্কিক m_dynsizedelem = অসত্য;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    তার্কিক m_taskRefWarn = সত্য;  // Allow task reference warnings
    তার্কিক m_underSel = অসত্য;  // Under AstMemberSel or AstSel
    তার্কিক m_underAlwaysEdged = অসত্য;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_গ্রন্থি_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

সার্বজনিক:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

গুপ্ত:
    // METHODS
    শূন্য editDType(Ast_গ্রন্থি* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        যদি (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_গ্রন্থি_DType* editOneDType(Ast_গ্রন্থি_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        যদি (!nodep) প্রত্যাগম nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        যদি (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        যদি (AstBasicDType* ধ্রুব bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ধ্রুব newp = nodep->find_ঢোকাও_Same_DType(bdtypep);
            যদি (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            প্রত্যাগম newp;
        }
        প্রত্যাগম nodep;
    }
    শূন্য classEncapCheck(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        তার্কিক local = অসত্য;
        তার্কিক prot = অসত্য;
        যদি (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            প্রত্যাগম;
        }
        যদি (ধ্রুব auto anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } অন্যথা যদি (ধ্রুব auto anodep = VN_CAST(defp, গ্রন্থি_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } অন্যথা যদি (ধ্রুব auto anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } অন্যথা {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        যদি (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ধ্রুব AstClass* refClassp = VN_CAST(m_modp, Class);
            যদি (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ধ্রুব বর্ণ* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ধ্রুব auto nestedAccess = [refClassp](ধ্রুব AstClass*, ধ্রুব Ast_গ্রন্থি* memberp) {
                প্রত্যাগম memberp == refClassp;
            };
            যদি (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } অন্যথা যদি (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            যদি (how) {
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
    শূন্য varLifetimeCheck(Ast_গ্রন্থি* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        যদি (!m_underSel && m_contNbap) {
            std::string varType;
            ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব varDtp = varp->dtypep()->skipRefp();
            যদি (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            অন্যথা যদি (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            অন্যথা যদি (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            যদি (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    শূন্য selLifetimeCheck(Ast_গ্রন্থি_রাশিমালা* nodep, ধ্রুব Ast_গ্রন্থি* namep) {
        যদি (m_underSel || !m_contNbap || !m_dynsizedelem) প্রত্যাগম;
        যদি (!nodep->isLValue() && !m_contReads) প্রত্যাগম;
        যদি (!nodep->dtypep()->skipRefp()->isDynamicallySized()) প্রত্যাগম;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    শূন্য deadCheckTasks() {
        প্রতি (Ast_গ্রন্থি_FTask* taskp : m_tasksp) {
            যদি (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        প্রতি (AstNew* newp : m_virtualNewsp) {
            যদি (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_গ্রন্থি::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    শূন্য virtualRecurse(Ast_গ্রন্থি_FTask* nodep, ধ্রুব AstClass* classp, তার্কিক& isVirtualr) {
        স্থির std::unordered_map<std::pair<ধ্রুব AstClass*, ধ্রুব Ast_গ্রন্থি_FTask*>, তার্কিক, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        auto pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        যদি (!pair.second) {
            যদি (pair.first->second) isVirtualr = সত্য;
            প্রত্যাগম;
        }
        //
        যদি (nodep->isVirtual()) isVirtualr = সত্য;
        // Propagate value from extends/implements
        প্রতি (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ধ্রুব AstClass* ধ্রুব eclassp = extendsp->classp();
            যদি (Ast_গ্রন্থি_FTask* ধ্রুব fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), গ্রন্থি_FTask)) {
                যদি (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    অনুবর্ত;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        যদি (isVirtualr) {
            nodep->isVirtual(সত্য);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        যদি (AstClass* ধ্রুব classp = VN_CAST(nodep, Class)) {
            প্রতি (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ধ্রুব AstClass* ধ্রুব ebasep = extendsp->classp();
                যদি (ebasep->baseOverride().isFinal()) {
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
    শূন্য visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        যদি (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            ধ্রুব তার্কিক noReads = nodep->forall(
                [&](ধ্রুব Ast_গ্রন্থি_Var_Ref* refp) { প্রত্যাগম !refp->access().isReadOrRW(); });
            যদি (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                প্রত্যাগম;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    শূন্য visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        যদি (!nodep->isTimingControl()) m_taskRefWarn = অসত্য;
        iterateChildren(nodep);
        editDType(nodep);
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        যদি (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            প্রত্যাগম;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    শূন্য visit(AstAttrOf* nodep) override {
        বিকল্প (nodep->attrType()) {
        পক্ষ VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        পক্ষ VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            প্রত্যাগম;
        স্বভাব:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    শূন্য visit(AstClassExtends* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        যদি (nodep->argsp()) ঠেলো_Deletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    শূন্য visit(AstConst* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        যদি (AstConst* ধ্রুব newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            Ast_গ্রন্থি* ধ্রুব oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(ঠেলো_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    শূন্য visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }
    শূন্য visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ধ্রুব AstClass* ধ্রুব classp = VN_CAST(m_modp, Class);
            যদি (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_গ্রন্থি_DType(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_UOr_Struct_DType* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Process once
        visit_Iterate_গ্রন্থি_DType(nodep);
    }
    শূন্য visit(AstParamTypeDType* nodep) override {
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Process once
        visit_Iterate_গ্রন্থি_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    শূন্য visit(AstRefDType* nodep) override {
        visit_Iterate_গ্রন্থি_DType(nodep);
        যদি (!nodep->typedefp()) প্রত্যাগম;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    শূন্য visit_Iterate_গ্রন্থি_DType(Ast_গ্রন্থি_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        যদি (nodep->user1SetOnce()) প্রত্যাগম;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        যদি (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        যদি (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.ঠেলো_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        তার্কিক isVirtual = অসত্য;
        যদি (ধ্রুব AstClass* ধ্রুব classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        যদি (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ধ্রুব AstClass* ধ্রুব classp = VN_CAST(m_modp, Class);
            যদি (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        তার্কিক extended = অসত্য;
        যদি (ধ্রুব AstClass* ধ্রুব classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            প্রতি (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ধ্রুব AstClass* ধ্রুব eclassp = extendsp->classp();
                যদি (Ast_গ্রন্থি_FTask* ধ্রুব fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), গ্রন্থি_FTask)) {
                    যদি (fbasep != nodep) {
                        extended = সত্য;
                        যদি (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        যদি (fbasep->baseOverride().isFinal()) {
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
        যদি (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        যদি (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        যদি (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    শূন্য visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        যদি (m_underAlwaysEdged) {
            ধ্রুব তার্কিক ignore = nodep->lhsp()->forall([&](ধ্রুব AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                যদি (refp->access().isReadOnly()) প্রত্যাগম সত্য;
                ধ্রুব AstVar* ধ্রুব varp = refp->varp();
                // Ignore ...
                প্রত্যাগম varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            যদি (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    শূন্য visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = সত্য;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    শূন্য visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = অসত্য;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    শূন্য visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = অসত্য;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        যদি (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        যদি (nodep->taskp()) nodep->taskp()->user2(1);
        যদি (AstNew* ধ্রুব newp = VN_CAST(nodep, New)) {
            যদি (!VN_IS(newp->backp(), Assign)) প্রত্যাগম;
            যদি (AstClass* ধ্রুব classp = VN_CAST(newp->classOrPackagep(), Class)) {
                যদি (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.ঠেলো_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    শূন্য visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = সত্য;
            iterateChildren(nodep);
        }
        editDType(nodep);
        যদি (AstClassRefDType* ধ্রুব classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    শূন্য visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        যদি (nodep->isFuncLocal() && nodep->direction() == VDirection::প্রবেশ && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            ঠেলো_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    শূন্য visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        যদি (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = সত্য;
        iterateChildren(nodep);
        editDType(nodep);
    }
    শূন্য visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = সত্য;
        iterateChildren(nodep);
        editDType(nodep);
    }
    শূন্য visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = সত্য;
        iterateChildren(nodep);
        editDType(nodep);
    }
    শূন্য visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    শূন্য visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ অসত্য));
    }
    শূন্য visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = সত্য;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

সার্বজনিক:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ধ্রুব VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_গ্রন্থি_Module* modp) {
                যদি (AstClass* ধ্রুব classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } অন্যথা যদি (AstIface* ধ্রুব ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = স্বভাব;
};

//######################################################################
// V3WidthCommit class functions

শূন্য V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("widthcommit", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
