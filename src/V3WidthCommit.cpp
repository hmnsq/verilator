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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3WidthCommit.h"

#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
ସଂରଚନା PairHash final {
    template <typename T, typename U>
    std::size_t operator()(ଅଚଳ std::pair<T, U>& x) ଅଚଳ {
        ଫେରନ୍ତୁ std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

ଶ୍ରେଣୀ WidthCommitVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    ଅଚଳ ଅକ୍ଷର* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    ତାର୍କିକ m_contReads = ମିଥ୍ୟା;  // Check read continuous automatic variables
    ତାର୍କିକ m_dynsizedelem = ମିଥ୍ୟା;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    ତାର୍କିକ m_taskRefWarn = ସତ୍ୟ;  // Allow task reference warnings
    ତାର୍କିକ m_underSel = ମିଥ୍ୟା;  // Under AstMemberSel or AstSel
    ତାର୍କିକ m_underAlwaysEdged = ମିଥ୍ୟା;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_ଗଣ୍ଠି_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

ସର୍ବସାଧାରଣ:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

ବ୍ୟକ୍ତିଗତ:
    // METHODS
    ଶୂନ୍ୟ editDType(Ast_ଗଣ୍ଠି* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        ଯଦି (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_ଗଣ୍ଠି_DType* editOneDType(Ast_ଗଣ୍ଠି_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        ଯଦି (!nodep) ଫେରନ୍ତୁ nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        ଯଦି (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        ଯଦି (AstBasicDType* ଅଚଳ bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ଅଚଳ newp = nodep->find_ଭର୍ତ୍ତି_Same_DType(bdtypep);
            ଯଦି (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            ଫେରନ୍ତୁ newp;
        }
        ଫେରନ୍ତୁ nodep;
    }
    ଶୂନ୍ୟ classEncapCheck(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        ତାର୍କିକ local = ମିଥ୍ୟା;
        ତାର୍କିକ prot = ମିଥ୍ୟା;
        ଯଦି (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (ଅଚଳ auto anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ auto anodep = VN_CAST(defp, ଗଣ୍ଠି_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ auto anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ଅନ୍ୟଥା {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        ଯଦି (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ଅଚଳ AstClass* refClassp = VN_CAST(m_modp, Class);
            ଯଦି (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ଅଚଳ ଅକ୍ଷର* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ଅଚଳ auto nestedAccess = [refClassp](ଅଚଳ AstClass*, ଅଚଳ Ast_ଗଣ୍ଠି* memberp) {
                ଫେରନ୍ତୁ memberp == refClassp;
            };
            ଯଦି (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } ଅନ୍ୟଥା ଯଦି (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            ଯଦି (how) {
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
    ଶୂନ୍ୟ varLifetimeCheck(Ast_ଗଣ୍ଠି* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        ଯଦି (!m_underSel && m_contNbap) {
            std::string varType;
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ varDtp = varp->dtypep()->skipRefp();
            ଯଦି (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            ଅନ୍ୟଥା ଯଦି (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            ଅନ୍ୟଥା ଯଦି (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            ଯଦି (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    ଶୂନ୍ୟ selLifetimeCheck(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି* namep) {
        ଯଦି (m_underSel || !m_contNbap || !m_dynsizedelem) ଫେରନ୍ତୁ;
        ଯଦି (!nodep->isLValue() && !m_contReads) ଫେରନ୍ତୁ;
        ଯଦି (!nodep->dtypep()->skipRefp()->isDynamicallySized()) ଫେରନ୍ତୁ;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    ଶୂନ୍ୟ deadCheckTasks() {
        ପାଇଁ (Ast_ଗଣ୍ଠି_FTask* taskp : m_tasksp) {
            ଯଦି (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        ପାଇଁ (AstNew* newp : m_virtualNewsp) {
            ଯଦି (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_ଗଣ୍ଠି::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    ଶୂନ୍ୟ virtualRecurse(Ast_ଗଣ୍ଠି_FTask* nodep, ଅଚଳ AstClass* classp, ତାର୍କିକ& isVirtualr) {
        ସ୍ଥିର std::unordered_map<std::pair<ଅଚଳ AstClass*, ଅଚଳ Ast_ଗଣ୍ଠି_FTask*>, ତାର୍କିକ, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        auto pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        ଯଦି (!pair.second) {
            ଯଦି (pair.first->second) isVirtualr = ସତ୍ୟ;
            ଫେରନ୍ତୁ;
        }
        //
        ଯଦି (nodep->isVirtual()) isVirtualr = ସତ୍ୟ;
        // Propagate value from extends/implements
        ପାଇଁ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ଅଚଳ AstClass* ଅଚଳ eclassp = extendsp->classp();
            ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), ଗଣ୍ଠି_FTask)) {
                ଯଦି (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    ଜାରି;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        ଯଦି (isVirtualr) {
            nodep->isVirtual(ସତ୍ୟ);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(nodep, Class)) {
            ପାଇଁ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ଅଚଳ AstClass* ଅଚଳ ebasep = extendsp->classp();
                ଯଦି (ebasep->baseOverride().isFinal()) {
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
    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        ଯଦି (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            ଅଚଳ ତାର୍କିକ noReads = nodep->forall(
                [&](ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* refp) { ଫେରନ୍ତୁ !refp->access().isReadOrRW(); });
            ଯଦି (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        ଯଦି (!nodep->isTimingControl()) m_taskRefWarn = ମିଥ୍ୟା;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override {
        ଯଦି (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstAttrOf* nodep) override {
        ବାଛ (nodep->attrType()) {
        ସ୍ଥିତି VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        ସ୍ଥିତି VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstClassExtends* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        ଯଦି (nodep->argsp()) ଠେଲ_Deletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        ଯଦି (AstConst* ଅଚଳ newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            Ast_ଗଣ୍ଠି* ଅଚଳ oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(ଠେଲ_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }
    ଶୂନ୍ୟ visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(m_modp, Class);
            ଯଦି (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_ଗଣ୍ଠି_DType(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        visit_Iterate_ଗଣ୍ଠି_DType(nodep);
    }
    ଶୂନ୍ୟ visit(AstParamTypeDType* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        visit_Iterate_ଗଣ୍ଠି_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        visit_Iterate_ଗଣ୍ଠି_DType(nodep);
        ଯଦି (!nodep->typedefp()) ଫେରନ୍ତୁ;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    ଶୂନ୍ୟ visit_Iterate_ଗଣ୍ଠି_DType(Ast_ଗଣ୍ଠି_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        ଯଦି (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        ଯଦି (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.ଠେଲ_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ତାର୍କିକ isVirtual = ମିଥ୍ୟା;
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        ଯଦି (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(m_modp, Class);
            ଯଦି (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        ତାର୍କିକ extended = ମିଥ୍ୟା;
        ଯଦି (ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            ପାଇଁ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ଅଚଳ AstClass* ଅଚଳ eclassp = extendsp->classp();
                ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), ଗଣ୍ଠି_FTask)) {
                    ଯଦି (fbasep != nodep) {
                        extended = ସତ୍ୟ;
                        ଯଦି (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        ଯଦି (fbasep->baseOverride().isFinal()) {
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
        ଯଦି (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        ଯଦି (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        ଯଦି (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    ଶୂନ୍ୟ visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        ଯଦି (m_underAlwaysEdged) {
            ଅଚଳ ତାର୍କିକ ignore = nodep->lhsp()->forall([&](ଅଚଳ AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                ଯଦି (refp->access().isReadOnly()) ଫେରନ୍ତୁ ସତ୍ୟ;
                ଅଚଳ AstVar* ଅଚଳ varp = refp->varp();
                // Ignore ...
                ଫେରନ୍ତୁ varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            ଯଦି (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    ଶୂନ୍ୟ visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ସତ୍ୟ;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = ମିଥ୍ୟା;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ମିଥ୍ୟା;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        ଯଦି (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        ଯଦି (nodep->taskp()) nodep->taskp()->user2(1);
        ଯଦି (AstNew* ଅଚଳ newp = VN_CAST(nodep, New)) {
            ଯଦି (!VN_IS(newp->backp(), Assign)) ଫେରନ୍ତୁ;
            ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(newp->classOrPackagep(), Class)) {
                ଯଦି (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.ଠେଲ_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = ସତ୍ୟ;
            iterateChildren(nodep);
        }
        editDType(nodep);
        ଯଦି (AstClassRefDType* ଅଚଳ classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        ଯଦି (nodep->isFuncLocal() && nodep->direction() == VDirection::ନିବେଶ && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            ଠେଲ_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    ଶୂନ୍ୟ visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        ଯଦି (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = ସତ୍ୟ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = ସତ୍ୟ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = ସତ୍ୟ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    ଶୂନ୍ୟ visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ ମିଥ୍ୟା));
    }
    ଶୂନ୍ୟ visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = ସତ୍ୟ;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ଅଚଳ VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_ଗଣ୍ଠି_Module* modp) {
                ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } ଅନ୍ୟଥା ଯଦି (AstIface* ଅଚଳ ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// V3WidthCommit class functions

ଶୂନ୍ୟ V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("widthcommit", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
