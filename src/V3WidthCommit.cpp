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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3WidthCommit.h"

#ضمن "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
بنية PairHash final {
    template <typename T, typename U>
    std::size_t operator()(ثابت std::pair<T, U>& x) ثابت {
        أرجع std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

صنف WidthCommitVisitor final : عام VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // STATE
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    AstNodeModule* m_modp = nullptr;  // Current module
    ثابت حرف* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    منطقي m_contReads = خطأ;  // Check read continuous automatic variables
    منطقي m_dynsizedelem = خطأ;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    منطقي m_taskRefWarn = true;  // Allow task reference warnings
    منطقي m_underSel = خطأ;  // Under AstMemberSel or AstSel
    منطقي m_underAlwaysEdged = خطأ;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<AstNodeFTask*> m_tasksp;  // All the tasks, we will check if they are ever called

عام:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

خاص:
    // METHODS
    فراغ editDType(AstNode* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        𐤀𐤌 (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    AstNodeDType* editOneDType(AstNodeDType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        𐤀𐤌 (!nodep) أرجع nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        𐤀𐤌 (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        𐤀𐤌 (AstBasicDType* ثابت bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ثابت newp = nodep->findInsertSameDType(bdtypep);
            𐤀𐤌 (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            أرجع newp;
        }
        أرجع nodep;
    }
    فراغ classEncapCheck(AstNode* nodep, AstNode* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        منطقي local = خطأ;
        منطقي prot = خطأ;
        𐤀𐤌 (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            أرجع;
        }
        𐤀𐤌 (ثابت auto anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وإلا 𐤀𐤌 (ثابت auto anodep = VN_CAST(defp, NodeFTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وإلا 𐤀𐤌 (ثابت auto anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وإلا {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        𐤀𐤌 (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ثابت AstClass* refClassp = VN_CAST(m_modp, Class);
            𐤀𐤌 (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ثابت حرف* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ثابت auto nestedAccess = [refClassp](ثابت AstClass*, ثابت AstNode* memberp) {
                أرجع memberp == refClassp;
            };
            𐤀𐤌 (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } وإلا 𐤀𐤌 (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            𐤀𐤌 (how) {
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
    فراغ varLifetimeCheck(AstNode* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        𐤀𐤌 (!m_underSel && m_contNbap) {
            std::string varType;
            ثابت AstNodeDType* ثابت varDtp = varp->dtypep()->skipRefp();
            𐤀𐤌 (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            وإلا 𐤀𐤌 (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            وإلا 𐤀𐤌 (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            𐤀𐤌 (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    فراغ selLifetimeCheck(AstNodeExpr* nodep, ثابت AstNode* namep) {
        𐤀𐤌 (m_underSel || !m_contNbap || !m_dynsizedelem) أرجع;
        𐤀𐤌 (!nodep->isLValue() && !m_contReads) أرجع;
        𐤀𐤌 (!nodep->dtypep()->skipRefp()->isDynamicallySized()) أرجع;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    فراغ deadCheckTasks() {
        لكل (AstNodeFTask* taskp : m_tasksp) {
            𐤀𐤌 (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        لكل (AstNew* newp : m_virtualNewsp) {
            𐤀𐤌 (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + AstNode::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    فراغ virtualRecurse(AstNodeFTask* nodep, ثابت AstClass* classp, منطقي& isVirtualr) {
        ساكن std::unordered_map<std::pair<ثابت AstClass*, ثابت AstNodeFTask*>, منطقي, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        auto pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        𐤀𐤌 (!pair.second) {
            𐤀𐤌 (pair.first->second) isVirtualr = true;
            أرجع;
        }
        //
        𐤀𐤌 (nodep->isVirtual()) isVirtualr = true;
        // Propagate value from extends/implements
        لكل (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ثابت AstClass* ثابت eclassp = extendsp->classp();
            𐤀𐤌 (AstNodeFTask* ثابت fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), NodeFTask)) {
                𐤀𐤌 (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    استمر;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        𐤀𐤌 (isVirtualr) {
            nodep->isVirtual(true);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        𐤀𐤌 (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            لكل (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ثابت AstClass* ثابت ebasep = extendsp->classp();
                𐤀𐤌 (ebasep->baseOverride().isFinal()) {
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
    فراغ visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        𐤀𐤌 (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            ثابت منطقي noReads = nodep->forall(
                [&](ثابت AstNodeVarRef* refp) { أرجع !refp->access().isReadOrRW(); });
            𐤀𐤌 (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        𐤀𐤌 (!nodep->isTimingControl()) m_taskRefWarn = خطأ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ visit(AstSenTree* nodep) override {
        𐤀𐤌 (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ visit(AstAttrOf* nodep) override {
        بدل (nodep->attrType()) {
        حالة VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        حالة VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            أرجع;
        افتراضي:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ visit(AstClassExtends* nodep) override {
        𐤀𐤌 (nodep->user1SetOnce()) أرجع;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        𐤀𐤌 (nodep->argsp()) pushDeletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    فراغ visit(AstConst* nodep) override {
        𐤀𐤌 (nodep->user1SetOnce()) أرجع;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        𐤀𐤌 (AstConst* ثابت newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            AstNode* ثابت oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(pushDeletep(oldp), oldp);
        }
        editDType(nodep);
    }
    فراغ visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
            𐤀𐤌 (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    فراغ visit(AstNodeDType* nodep) override {
        // Note some specific dtypes have unique visitors
        visitIterateNodeDType(nodep);
    }
    فراغ visit(AstNodeUOrStructDType* nodep) override {
        𐤀𐤌 (nodep->user1SetOnce()) أرجع;  // Process once
        visitIterateNodeDType(nodep);
    }
    فراغ visit(AstParamTypeDType* nodep) override {
        𐤀𐤌 (nodep->user1SetOnce()) أرجع;  // Process once
        visitIterateNodeDType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    فراغ visit(AstRefDType* nodep) override {
        visitIterateNodeDType(nodep);
        𐤀𐤌 (!nodep->typedefp()) أرجع;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    فراغ visitIterateNodeDType(AstNodeDType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        𐤀𐤌 (nodep->user1SetOnce()) أرجع;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        𐤀𐤌 (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    فراغ visit(AstNodeFTask* nodep) override {
        𐤀𐤌 (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.push_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        منطقي isVirtual = خطأ;
        𐤀𐤌 (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        𐤀𐤌 (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
            𐤀𐤌 (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        منطقي extended = خطأ;
        𐤀𐤌 (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            لكل (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ثابت AstClass* ثابت eclassp = extendsp->classp();
                𐤀𐤌 (AstNodeFTask* ثابت fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), NodeFTask)) {
                    𐤀𐤌 (fbasep != nodep) {
                        extended = true;
                        𐤀𐤌 (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        𐤀𐤌 (fbasep->baseOverride().isFinal()) {
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
        𐤀𐤌 (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        𐤀𐤌 (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        𐤀𐤌 (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    فراغ visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        𐤀𐤌 (m_underAlwaysEdged) {
            ثابت منطقي ignore = nodep->lhsp()->forall([&](ثابت AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                𐤀𐤌 (refp->access().isReadOnly()) أرجع true;
                ثابت AstVar* ثابت varp = refp->varp();
                // Ignore ...
                أرجع varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            𐤀𐤌 (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    فراغ visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = true;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    فراغ visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = خطأ;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    فراغ visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = خطأ;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        𐤀𐤌 (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        𐤀𐤌 (nodep->taskp()) nodep->taskp()->user2(1);
        𐤀𐤌 (AstNew* ثابت newp = VN_CAST(nodep, New)) {
            𐤀𐤌 (!VN_IS(newp->backp(), Assign)) أرجع;
            𐤀𐤌 (AstClass* ثابت classp = VN_CAST(newp->classOrPackagep(), Class)) {
                𐤀𐤌 (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.push_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    فراغ visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = true;
            iterateChildren(nodep);
        }
        editDType(nodep);
        𐤀𐤌 (AstClassRefDType* ثابت classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    فراغ visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        𐤀𐤌 (nodep->isFuncLocal() && nodep->direction() == VDirection::INPUT && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            pushDeletep(nodep->valuep()->unlinkFrBack());
        }
    }
    فراغ visit(AstNodePreSel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    فراغ visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        𐤀𐤌 (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = true;
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = true;
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = true;
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    فراغ visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ خطأ));
    }
    فراغ visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = true;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    فراغ visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstNode* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

عام:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ثابت VDTypeNameScopes scopes{nodep};
            nodep->foreach([](AstNodeModule* modp) {
                𐤀𐤌 (AstClass* ثابت classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } وإلا 𐤀𐤌 (AstIface* ثابت ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = افتراضي;
};

//######################################################################
// V3WidthCommit class functions

فراغ V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("widthcommit", 0, dumpTreeEitherLevel() >= 6);
}
