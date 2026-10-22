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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3WidthCommit.h"

#अन्तर्भाव "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
संरचना PairHash final {
    ढाँचा <typename T, typename U>
    std::माप_प्रकार चालक()(ध्रुव std::pair<T, U>& x) ध्रुव {
        प्रत्यागम std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

वर्ग WidthCommitVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ध्रुव VNUser1InUse m_inuser1;
    ध्रुव VNUser2InUse m_inuser2;

    // STATE
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    ध्रुव वर्ण* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    तार्किक m_contReads = असत्य;  // Check read continuous automatic variables
    तार्किक m_dynsizedelem = असत्य;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    तार्किक m_taskRefWarn = सत्य;  // Allow task reference warnings
    तार्किक m_underSel = असत्य;  // Under AstMemberSel or AstSel
    तार्किक m_underAlwaysEdged = असत्य;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_गाँठ_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

सार्वजनिक:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

गुप्त:
    // METHODS
    शून्य editDType(Ast_गाँठ* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        यदि (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
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
        यदि (!nodep) प्रत्यागम nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        यदि (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        यदि (AstBasicDType* ध्रुव bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ध्रुव newp = nodep->find_जोड़ो_Same_DType(bdtypep);
            यदि (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            प्रत्यागम newp;
        }
        प्रत्यागम nodep;
    }
    शून्य classEncapCheck(Ast_गाँठ* nodep, Ast_गाँठ* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        तार्किक local = असत्य;
        तार्किक prot = असत्य;
        यदि (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            प्रत्यागम;
        }
        यदि (ध्रुव स्वतः anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } अन्यथा यदि (ध्रुव स्वतः anodep = VN_CAST(defp, गाँठ_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } अन्यथा यदि (ध्रुव स्वतः anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } अन्यथा {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        यदि (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ध्रुव AstClass* refClassp = VN_CAST(m_modp, Class);
            यदि (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ध्रुव वर्ण* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ध्रुव स्वतः nestedAccess = [refClassp](ध्रुव AstClass*, ध्रुव Ast_गाँठ* memberp) {
                प्रत्यागम memberp == refClassp;
            };
            यदि (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } अन्यथा यदि (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            यदि (how) {
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
    शून्य varLifetimeCheck(Ast_गाँठ* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        यदि (!m_underSel && m_contNbap) {
            std::string varType;
            ध्रुव Ast_गाँठ_DType* ध्रुव varDtp = varp->dtypep()->skipRefp();
            यदि (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            अन्यथा यदि (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            अन्यथा यदि (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            यदि (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    शून्य selLifetimeCheck(Ast_गाँठ_व्यंजक* nodep, ध्रुव Ast_गाँठ* namep) {
        यदि (m_underSel || !m_contNbap || !m_dynsizedelem) प्रत्यागम;
        यदि (!nodep->isLValue() && !m_contReads) प्रत्यागम;
        यदि (!nodep->dtypep()->skipRefp()->isDynamicallySized()) प्रत्यागम;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    शून्य deadCheckTasks() {
        प्रति (Ast_गाँठ_FTask* taskp : m_tasksp) {
            यदि (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        प्रति (AstNew* newp : m_virtualNewsp) {
            यदि (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_गाँठ::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    शून्य virtualRecurse(Ast_गाँठ_FTask* nodep, ध्रुव AstClass* classp, तार्किक& isVirtualr) {
        स्थिर std::unordered_map<std::pair<ध्रुव AstClass*, ध्रुव Ast_गाँठ_FTask*>, तार्किक, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        स्वतः pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        यदि (!pair.second) {
            यदि (pair.first->second) isVirtualr = सत्य;
            प्रत्यागम;
        }
        //
        यदि (nodep->isVirtual()) isVirtualr = सत्य;
        // Propagate value from extends/implements
        प्रति (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ध्रुव AstClass* ध्रुव eclassp = extendsp->classp();
            यदि (Ast_गाँठ_FTask* ध्रुव fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), गाँठ_FTask)) {
                यदि (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    अनुवर्त;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        यदि (isVirtualr) {
            nodep->isVirtual(सत्य);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    शून्य visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        यदि (AstClass* ध्रुव classp = VN_CAST(nodep, Class)) {
            प्रति (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ध्रुव AstClass* ध्रुव ebasep = extendsp->classp();
                यदि (ebasep->baseOverride().isFinal()) {
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
    शून्य visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        यदि (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            ध्रुव तार्किक noReads = nodep->forall(
                [&](ध्रुव Ast_गाँठ_Var_Ref* refp) { प्रत्यागम !refp->access().isReadOrRW(); });
            यदि (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                प्रत्यागम;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    शून्य visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        यदि (!nodep->isTimingControl()) m_taskRefWarn = असत्य;
        iterateChildren(nodep);
        editDType(nodep);
    }
    शून्य visit(Ast_Sen_वृक्ष* nodep) override {
        यदि (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            प्रत्यागम;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    शून्य visit(AstAttrOf* nodep) override {
        विकल्प (nodep->attrType()) {
        पक्ष VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        पक्ष VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            प्रत्यागम;
        स्वभाव:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    शून्य visit(AstClassExtends* nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        यदि (nodep->argsp()) धकेलो_Deletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    शून्य visit(AstConst* nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        यदि (AstConst* ध्रुव newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            Ast_गाँठ* ध्रुव oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(धकेलो_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    शून्य visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    शून्य visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ध्रुव AstClass* ध्रुव classp = VN_CAST(m_modp, Class);
            यदि (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    शून्य visit(Ast_गाँठ_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_गाँठ_DType(nodep);
    }
    शून्य visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;  // Process once
        visit_Iterate_गाँठ_DType(nodep);
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        यदि (nodep->user1SetOnce()) प्रत्यागम;  // Process once
        visit_Iterate_गाँठ_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    शून्य visit(AstRefDType* nodep) override {
        visit_Iterate_गाँठ_DType(nodep);
        यदि (!nodep->typedefp()) प्रत्यागम;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    शून्य visit_Iterate_गाँठ_DType(Ast_गाँठ_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        यदि (nodep->user1SetOnce()) प्रत्यागम;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        यदि (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    शून्य visit(Ast_गाँठ_FTask* nodep) override {
        यदि (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.धकेलो_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        तार्किक isVirtual = असत्य;
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        यदि (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ध्रुव AstClass* ध्रुव classp = VN_CAST(m_modp, Class);
            यदि (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        तार्किक extended = असत्य;
        यदि (ध्रुव AstClass* ध्रुव classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            प्रति (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ध्रुव AstClass* ध्रुव eclassp = extendsp->classp();
                यदि (Ast_गाँठ_FTask* ध्रुव fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), गाँठ_FTask)) {
                    यदि (fbasep != nodep) {
                        extended = सत्य;
                        यदि (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        यदि (fbasep->baseOverride().isFinal()) {
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
        यदि (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    शून्य visit(Ast_गाँठ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        यदि (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        यदि (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    शून्य visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        यदि (m_underAlwaysEdged) {
            ध्रुव तार्किक ignore = nodep->lhsp()->forall([&](ध्रुव AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                यदि (refp->access().isReadOnly()) प्रत्यागम सत्य;
                ध्रुव AstVar* ध्रुव varp = refp->varp();
                // Ignore ...
                प्रत्यागम varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            यदि (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    शून्य visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = सत्य;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    शून्य visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = असत्य;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    शून्य visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = असत्य;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    शून्य visit(Ast_गाँठ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        यदि (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        यदि (nodep->taskp()) nodep->taskp()->user2(1);
        यदि (AstNew* ध्रुव newp = VN_CAST(nodep, New)) {
            यदि (!VN_IS(newp->backp(), Assign)) प्रत्यागम;
            यदि (AstClass* ध्रुव classp = VN_CAST(newp->classOrPackagep(), Class)) {
                यदि (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.धकेलो_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    शून्य visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = सत्य;
            iterateChildren(nodep);
        }
        editDType(nodep);
        यदि (AstClassRefDType* ध्रुव classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    शून्य visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        यदि (nodep->isFuncLocal() && nodep->direction() == VDirection::निवेश && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            धकेलो_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    शून्य visit(Ast_गाँठ_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    शून्य visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        यदि (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = सत्य;
        iterateChildren(nodep);
        editDType(nodep);
    }
    शून्य visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = सत्य;
        iterateChildren(nodep);
        editDType(nodep);
    }
    शून्य visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = सत्य;
        iterateChildren(nodep);
        editDType(nodep);
    }
    शून्य visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    शून्य visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ असत्य));
    }
    शून्य visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = सत्य;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(Ast_गाँठ* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ध्रुव VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_गाँठ_Module* modp) {
                यदि (AstClass* ध्रुव classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } अन्यथा यदि (AstIface* ध्रुव ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = स्वभाव;
};

//######################################################################
// V3WidthCommit class functions

शून्य V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("widthcommit", 0, dump_वृक्ष_Either_Level() >= 6);
}
