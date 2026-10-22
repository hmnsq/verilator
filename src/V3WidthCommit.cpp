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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3WidthCommit.h"

#समावेश "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
काष्ठा PairHash final {
    ढाँचा <typename T, typename U>
    std::माप_प्रकार चालक()(स्थिर std::pair<T, U>& x) स्थिर {
        वापस std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

श्रेणी WidthCommitVisitor final : खुला VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;

    // STATE
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    स्थिर अक्षर* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    bool m_contReads = false;  // Check read continuous automatic variables
    bool m_dynsizedelem = false;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    bool m_taskRefWarn = true;  // Allow task reference warnings
    bool m_underSel = false;  // Under AstMemberSel or AstSel
    bool m_underAlwaysEdged = false;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_गाँठ_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

खुला:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

निजी:
    // METHODS
    व्योम editDType(Ast_गाँठ* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        अगर (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_गाँठ_DType* editOneDType(Ast_गाँठ_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        अगर (!nodep) वापस nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        अगर (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        अगर (AstBasicDType* स्थिर bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* स्थिर newp = nodep->find_जोड़ो_Same_DType(bdtypep);
            अगर (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::अ_बाहर);
                std::अ_बाहर << "  ---->  ";
                newp->dumpSmall(std::अ_बाहर);
                std::अ_बाहर << '\n';
            }
            वापस newp;
        }
        वापस nodep;
    }
    व्योम classEncapCheck(Ast_गाँठ* nodep, Ast_गाँठ* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        bool local = false;
        bool prot = false;
        अगर (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            वापस;
        }
        अगर (स्थिर स्वतः anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } अन्यथा अगर (स्थिर स्वतः anodep = VN_CAST(defp, गाँठ_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } अन्यथा अगर (स्थिर स्वतः anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } अन्यथा {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        अगर (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            स्थिर AstClass* refClassp = VN_CAST(m_modp, Class);
            अगर (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            स्थिर अक्षर* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            स्थिर स्वतः nestedAccess = [refClassp](स्थिर AstClass*, स्थिर Ast_गाँठ* memberp) {
                वापस memberp == refClassp;
            };
            अगर (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } अन्यथा अगर (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            अगर (how) {
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
    व्योम varLifetimeCheck(Ast_गाँठ* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        अगर (!m_underSel && m_contNbap) {
            std::string varType;
            स्थिर Ast_गाँठ_DType* स्थिर varDtp = varp->dtypep()->skipRefp();
            अगर (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            अन्यथा अगर (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            अन्यथा अगर (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            अगर (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    व्योम selLifetimeCheck(Ast_गाँठ_व्यंजक* nodep, स्थिर Ast_गाँठ* namep) {
        अगर (m_underSel || !m_contNbap || !m_dynsizedelem) वापस;
        अगर (!nodep->isLValue() && !m_contReads) वापस;
        अगर (!nodep->dtypep()->skipRefp()->isDynamicallySized()) वापस;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    व्योम deadCheckTasks() {
        क्रम (Ast_गाँठ_FTask* taskp : m_tasksp) {
            अगर (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        क्रम (AstNew* newp : m_virtualNewsp) {
            अगर (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_गाँठ::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    व्योम virtualRecurse(Ast_गाँठ_FTask* nodep, स्थिर AstClass* classp, bool& isVirtualr) {
        अटल std::unordered_map<std::pair<स्थिर AstClass*, स्थिर Ast_गाँठ_FTask*>, bool, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        स्वतः pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        अगर (!pair.second) {
            अगर (pair.first->second) isVirtualr = true;
            वापस;
        }
        //
        अगर (nodep->isVirtual()) isVirtualr = true;
        // Propagate value from extends/implements
        क्रम (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            स्थिर AstClass* स्थिर eclassp = extendsp->classp();
            अगर (Ast_गाँठ_FTask* स्थिर fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), गाँठ_FTask)) {
                अगर (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    जारी;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        अगर (isVirtualr) {
            nodep->isVirtual(true);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        अगर (AstClass* स्थिर classp = VN_CAST(nodep, Class)) {
            क्रम (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                स्थिर AstClass* स्थिर ebasep = extendsp->classp();
                अगर (ebasep->baseOverride().isFinal()) {
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
    व्योम visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        अगर (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            स्थिर bool noReads = nodep->forall(
                [&](स्थिर Ast_गाँठ_Var_Ref* refp) { वापस !refp->access().isReadOrRW(); });
            अगर (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    व्योम visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        अगर (!nodep->isTimingControl()) m_taskRefWarn = false;
        iterateChildren(nodep);
        editDType(nodep);
    }
    व्योम visit(Ast_Sen_वृक्ष* nodep) override {
        अगर (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    व्योम visit(AstAttrOf* nodep) override {
        चयन (nodep->attrType()) {
        हाल VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        हाल VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            वापस;
        शेष:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    व्योम visit(AstClassExtends* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        अगर (nodep->argsp()) धकेलो_Deletep(nodep->argsp()->unlink_Fr_Back_With_अग्र());
        iterateChildren(nodep);
    }
    व्योम visit(AstConst* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        अगर (AstConst* स्थिर newp = V3WidthCommit::new_If_Const_Commit_आकार(nodep)) {
            nodep->replaceWith(newp);
            Ast_गाँठ* स्थिर oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(धकेलो_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    व्योम visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            स्थिर AstClass* स्थिर classp = VN_CAST(m_modp, Class);
            अगर (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    व्योम visit(Ast_गाँठ_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_गाँठ_DType(nodep);
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        visit_Iterate_गाँठ_DType(nodep);
    }
    व्योम visit(AstParamTypeDType* nodep) override {
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        visit_Iterate_गाँठ_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    व्योम visit(AstRefDType* nodep) override {
        visit_Iterate_गाँठ_DType(nodep);
        अगर (!nodep->typedefp()) वापस;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    व्योम visit_Iterate_गाँठ_DType(Ast_गाँठ_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        अगर (nodep->user1SetOnce()) वापस;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        अगर (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        अगर (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.धकेलो_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        bool isVirtual = false;
        अगर (स्थिर AstClass* स्थिर classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        अगर (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            स्थिर AstClass* स्थिर classp = VN_CAST(m_modp, Class);
            अगर (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        bool extended = false;
        अगर (स्थिर AstClass* स्थिर classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            क्रम (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                स्थिर AstClass* स्थिर eclassp = extendsp->classp();
                अगर (Ast_गाँठ_FTask* स्थिर fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), गाँठ_FTask)) {
                    अगर (fbasep != nodep) {
                        extended = true;
                        अगर (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        अगर (fbasep->baseOverride().isFinal()) {
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
        अगर (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        अगर (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        अगर (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    व्योम visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        अगर (m_underAlwaysEdged) {
            स्थिर bool ignore = nodep->lhsp()->forall([&](स्थिर AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                अगर (refp->access().isReadOnly()) वापस true;
                स्थिर AstVar* स्थिर varp = refp->varp();
                // Ignore ...
                वापस varp->isUsedLoopIdx()  // ... loop indices
                       || varp->is_अस्थायी()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            अगर (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    व्योम visit(AstAssignCont* nodep) override {
        iterate_And_अग्र_Null(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = true;
            iterate_And_अग्र_Null(nodep->lhsp());
            iterate_And_अग्र_Null(nodep->rhsp());
        }
        editDType(nodep);
    }
    व्योम visit(AstAssignDly* nodep) override {
        iterate_And_अग्र_Null(nodep->timingControlp());
        iterate_And_अग्र_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = false;
            iterate_And_अग्र_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    व्योम visit(AstAssignW* nodep) override {
        iterate_And_अग्र_Null(nodep->timingControlp());
        iterate_And_अग्र_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = false;
            iterate_And_अग्र_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        अगर (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        अगर (nodep->taskp()) nodep->taskp()->user2(1);
        अगर (AstNew* स्थिर newp = VN_CAST(nodep, New)) {
            अगर (!VN_IS(newp->backp(), Assign)) वापस;
            अगर (AstClass* स्थिर classp = VN_CAST(newp->classOrPackagep(), Class)) {
                अगर (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.धकेलो_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    व्योम visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = true;
            iterateChildren(nodep);
        }
        editDType(nodep);
        अगर (AstClassRefDType* स्थिर classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    व्योम visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        अगर (nodep->isFuncLocal() && nodep->direction() == VDirection::निवेश && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            धकेलो_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    व्योम visit(Ast_गाँठ_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    व्योम visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        अगर (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = true;
        iterateChildren(nodep);
        editDType(nodep);
    }
    व्योम visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = true;
        iterateChildren(nodep);
        editDType(nodep);
    }
    व्योम visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = true;
        iterateChildren(nodep);
        editDType(nodep);
    }
    व्योम visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    व्योम visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ false));
    }
    व्योम visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = true;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    व्योम visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            स्थिर VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_गाँठ_Module* modp) {
                अगर (AstClass* स्थिर classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } अन्यथा अगर (AstIface* स्थिर ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = शेष;
};

//######################################################################
// V3WidthCommit class functions

व्योम V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("widthcommit", 0, dump_वृक्ष_Either_Level() >= 6);
}
