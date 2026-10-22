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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3WidthCommit.h"

#અન્તર્ભાવ "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
સંરચના PairHash final {
    template <typename T, typename U>
    std::size_t operator()(અચલ std::pair<T, U>& x) અચલ {
        પાછા std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

વર્ગ WidthCommitVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;

    // STATE
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    અચલ અક્ષર* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    તાર્કિક m_contReads = ખોટું;  // Check read continuous automatic variables
    તાર્કિક m_dynsizedelem = ખોટું;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    તાર્કિક m_taskRefWarn = સાચું;  // Allow task reference warnings
    તાર્કિક m_underSel = ખોટું;  // Under AstMemberSel or AstSel
    તાર્કિક m_underAlwaysEdged = ખોટું;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_ગાંઠ_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

જાહેર:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

ખાનગી:
    // METHODS
    શૂન્ય editDType(Ast_ગાંઠ* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        જો (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_ગાંઠ_DType* editOneDType(Ast_ગાંઠ_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        જો (!nodep) પાછા nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        જો (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        જો (AstBasicDType* અચલ bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* અચલ newp = nodep->find_ઉમેરો_Same_DType(bdtypep);
            જો (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            પાછા newp;
        }
        પાછા nodep;
    }
    શૂન્ય classEncapCheck(Ast_ગાંઠ* nodep, Ast_ગાંઠ* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        તાર્કિક local = ખોટું;
        તાર્કિક prot = ખોટું;
        જો (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            પાછા;
        }
        જો (અચલ auto anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } નહીંતર જો (અચલ auto anodep = VN_CAST(defp, ગાંઠ_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } નહીંતર જો (અચલ auto anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } નહીંતર {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        જો (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            અચલ AstClass* refClassp = VN_CAST(m_modp, Class);
            જો (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            અચલ અક્ષર* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            અચલ auto nestedAccess = [refClassp](અચલ AstClass*, અચલ Ast_ગાંઠ* memberp) {
                પાછા memberp == refClassp;
            };
            જો (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } નહીંતર જો (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            જો (how) {
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
    શૂન્ય varLifetimeCheck(Ast_ગાંઠ* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        જો (!m_underSel && m_contNbap) {
            std::string varType;
            અચલ Ast_ગાંઠ_DType* અચલ varDtp = varp->dtypep()->skipRefp();
            જો (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            નહીંતર જો (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            નહીંતર જો (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            જો (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    શૂન્ય selLifetimeCheck(Ast_ગાંઠ_પદાવલિ* nodep, અચલ Ast_ગાંઠ* namep) {
        જો (m_underSel || !m_contNbap || !m_dynsizedelem) પાછા;
        જો (!nodep->isLValue() && !m_contReads) પાછા;
        જો (!nodep->dtypep()->skipRefp()->isDynamicallySized()) પાછા;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    શૂન્ય deadCheckTasks() {
        માટે (Ast_ગાંઠ_FTask* taskp : m_tasksp) {
            જો (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        માટે (AstNew* newp : m_virtualNewsp) {
            જો (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_ગાંઠ::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    શૂન્ય virtualRecurse(Ast_ગાંઠ_FTask* nodep, અચલ AstClass* classp, તાર્કિક& isVirtualr) {
        સ્થિર std::unordered_map<std::pair<અચલ AstClass*, અચલ Ast_ગાંઠ_FTask*>, તાર્કિક, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        auto pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        જો (!pair.second) {
            જો (pair.first->second) isVirtualr = સાચું;
            પાછા;
        }
        //
        જો (nodep->isVirtual()) isVirtualr = સાચું;
        // Propagate value from extends/implements
        માટે (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            અચલ AstClass* અચલ eclassp = extendsp->classp();
            જો (Ast_ગાંઠ_FTask* અચલ fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), ગાંઠ_FTask)) {
                જો (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    ચાલુ;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        જો (isVirtualr) {
            nodep->isVirtual(સાચું);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        જો (AstClass* અચલ classp = VN_CAST(nodep, Class)) {
            માટે (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                અચલ AstClass* અચલ ebasep = extendsp->classp();
                જો (ebasep->baseOverride().isFinal()) {
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
    શૂન્ય visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        જો (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            અચલ તાર્કિક noReads = nodep->forall(
                [&](અચલ Ast_ગાંઠ_Var_Ref* refp) { પાછા !refp->access().isReadOrRW(); });
            જો (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    શૂન્ય visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        જો (!nodep->isTimingControl()) m_taskRefWarn = ખોટું;
        iterateChildren(nodep);
        editDType(nodep);
    }
    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override {
        જો (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    શૂન્ય visit(AstAttrOf* nodep) override {
        પસંદગી (nodep->attrType()) {
        સ્થિતિ VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        સ્થિતિ VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        મૂળભૂત:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    શૂન્ય visit(AstClassExtends* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        જો (nodep->argsp()) ધકેલો_Deletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstConst* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        જો (AstConst* અચલ newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            Ast_ગાંઠ* અચલ oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(ધકેલો_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    શૂન્ય visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }
    શૂન્ય visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            અચલ AstClass* અચલ classp = VN_CAST(m_modp, Class);
            જો (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_ગાંઠ_DType(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        visit_Iterate_ગાંઠ_DType(nodep);
    }
    શૂન્ય visit(AstParamTypeDType* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        visit_Iterate_ગાંઠ_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    શૂન્ય visit(AstRefDType* nodep) override {
        visit_Iterate_ગાંઠ_DType(nodep);
        જો (!nodep->typedefp()) પાછા;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    શૂન્ય visit_Iterate_ગાંઠ_DType(Ast_ગાંઠ_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        જો (nodep->user1SetOnce()) પાછા;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        જો (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        જો (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.ધકેલો_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        તાર્કિક isVirtual = ખોટું;
        જો (અચલ AstClass* અચલ classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        જો (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            અચલ AstClass* અચલ classp = VN_CAST(m_modp, Class);
            જો (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        તાર્કિક extended = ખોટું;
        જો (અચલ AstClass* અચલ classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            માટે (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                અચલ AstClass* અચલ eclassp = extendsp->classp();
                જો (Ast_ગાંઠ_FTask* અચલ fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), ગાંઠ_FTask)) {
                    જો (fbasep != nodep) {
                        extended = સાચું;
                        જો (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        જો (fbasep->baseOverride().isFinal()) {
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
        જો (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        જો (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        જો (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    શૂન્ય visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        જો (m_underAlwaysEdged) {
            અચલ તાર્કિક ignore = nodep->lhsp()->forall([&](અચલ AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                જો (refp->access().isReadOnly()) પાછા સાચું;
                અચલ AstVar* અચલ varp = refp->varp();
                // Ignore ...
                પાછા varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            જો (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    શૂન્ય visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = સાચું;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    શૂન્ય visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = ખોટું;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    શૂન્ય visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ખોટું;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        જો (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        જો (nodep->taskp()) nodep->taskp()->user2(1);
        જો (AstNew* અચલ newp = VN_CAST(nodep, New)) {
            જો (!VN_IS(newp->backp(), Assign)) પાછા;
            જો (AstClass* અચલ classp = VN_CAST(newp->classOrPackagep(), Class)) {
                જો (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.ધકેલો_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = સાચું;
            iterateChildren(nodep);
        }
        editDType(nodep);
        જો (AstClassRefDType* અચલ classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    શૂન્ય visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        જો (nodep->isFuncLocal() && nodep->direction() == VDirection::આગત && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            ધકેલો_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    શૂન્ય visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        જો (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = સાચું;
        iterateChildren(nodep);
        editDType(nodep);
    }
    શૂન્ય visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = સાચું;
        iterateChildren(nodep);
        editDType(nodep);
    }
    શૂન્ય visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = સાચું;
        iterateChildren(nodep);
        editDType(nodep);
    }
    શૂન્ય visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    શૂન્ય visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ ખોટું));
    }
    શૂન્ય visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = સાચું;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            અચલ VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_ગાંઠ_Module* modp) {
                જો (AstClass* અચલ classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } નહીંતર જો (AstIface* અચલ ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = મૂળભૂત;
};

//######################################################################
// V3WidthCommit class functions

શૂન્ય V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("widthcommit", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
