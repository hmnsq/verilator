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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3WidthCommit.h"

#அந்தர்பாவ "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
கட்டமைப்பு PairHash final {
    template <typename T, typename U>
    std::size_t operator()(மாறிலி std::pair<T, U>& x) மாறிலி {
        திரும்பு std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

வகுப்பு WidthCommitVisitor final : பொது VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;

    // STATE
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    மாறிலி எழுத்து* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    தருக்க m_contReads = பொய்;  // Check read continuous automatic variables
    தருக்க m_dynsizedelem = பொய்;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    தருக்க m_taskRefWarn = உண்மை;  // Allow task reference warnings
    தருக்க m_underSel = பொய்;  // Under AstMemberSel or AstSel
    தருக்க m_underAlwaysEdged = பொய்;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_முனை_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

பொது:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

தனிப்பட்ட:
    // METHODS
    வெறுமை editDType(Ast_முனை* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        எனில் (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_முனை_DType* editOneDType(Ast_முனை_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        எனில் (!nodep) திரும்பு nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        எனில் (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        எனில் (AstBasicDType* மாறிலி bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* மாறிலி newp = nodep->find_செருகு_Same_DType(bdtypep);
            எனில் (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            திரும்பு newp;
        }
        திரும்பு nodep;
    }
    வெறுமை classEncapCheck(Ast_முனை* nodep, Ast_முனை* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        தருக்க local = பொய்;
        தருக்க prot = பொய்;
        எனில் (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            திரும்பு;
        }
        எனில் (மாறிலி auto anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } இல்லையெனில் எனில் (மாறிலி auto anodep = VN_CAST(defp, முனை_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } இல்லையெனில் எனில் (மாறிலி auto anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } இல்லையெனில் {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        எனில் (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            மாறிலி AstClass* refClassp = VN_CAST(m_modp, Class);
            எனில் (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            மாறிலி எழுத்து* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            மாறிலி auto nestedAccess = [refClassp](மாறிலி AstClass*, மாறிலி Ast_முனை* memberp) {
                திரும்பு memberp == refClassp;
            };
            எனில் (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } இல்லையெனில் எனில் (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            எனில் (how) {
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
    வெறுமை varLifetimeCheck(Ast_முனை* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        எனில் (!m_underSel && m_contNbap) {
            std::string varType;
            மாறிலி Ast_முனை_DType* மாறிலி varDtp = varp->dtypep()->skipRefp();
            எனில் (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            இல்லையெனில் எனில் (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            இல்லையெனில் எனில் (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            எனில் (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    வெறுமை selLifetimeCheck(Ast_முனை_கோவை* nodep, மாறிலி Ast_முனை* namep) {
        எனில் (m_underSel || !m_contNbap || !m_dynsizedelem) திரும்பு;
        எனில் (!nodep->isLValue() && !m_contReads) திரும்பு;
        எனில் (!nodep->dtypep()->skipRefp()->isDynamicallySized()) திரும்பு;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    வெறுமை deadCheckTasks() {
        க்கு (Ast_முனை_FTask* taskp : m_tasksp) {
            எனில் (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        க்கு (AstNew* newp : m_virtualNewsp) {
            எனில் (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_முனை::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    வெறுமை virtualRecurse(Ast_முனை_FTask* nodep, மாறிலி AstClass* classp, தருக்க& isVirtualr) {
        நிலையான std::unordered_map<std::pair<மாறிலி AstClass*, மாறிலி Ast_முனை_FTask*>, தருக்க, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        auto pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        எனில் (!pair.second) {
            எனில் (pair.first->second) isVirtualr = உண்மை;
            திரும்பு;
        }
        //
        எனில் (nodep->isVirtual()) isVirtualr = உண்மை;
        // Propagate value from extends/implements
        க்கு (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            மாறிலி AstClass* மாறிலி eclassp = extendsp->classp();
            எனில் (Ast_முனை_FTask* மாறிலி fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), முனை_FTask)) {
                எனில் (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    தொடர்;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        எனில் (isVirtualr) {
            nodep->isVirtual(உண்மை);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        எனில் (AstClass* மாறிலி classp = VN_CAST(nodep, Class)) {
            க்கு (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                மாறிலி AstClass* மாறிலி ebasep = extendsp->classp();
                எனில் (ebasep->baseOverride().isFinal()) {
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
    வெறுமை visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        எனில் (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            மாறிலி தருக்க noReads = nodep->forall(
                [&](மாறிலி Ast_முனை_Var_Ref* refp) { திரும்பு !refp->access().isReadOrRW(); });
            எனில் (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    வெறுமை visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        எனில் (!nodep->isTimingControl()) m_taskRefWarn = பொய்;
        iterateChildren(nodep);
        editDType(nodep);
    }
    வெறுமை visit(Ast_Sen_மரம்* nodep) override {
        எனில் (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    வெறுமை visit(AstAttrOf* nodep) override {
        தேர்வு (nodep->attrType()) {
        நிலை VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        நிலை VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        இயல்பு:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    வெறுமை visit(AstClassExtends* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        எனில் (nodep->argsp()) தள்ளு_Deletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    வெறுமை visit(AstConst* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        எனில் (AstConst* மாறிலி newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            Ast_முனை* மாறிலி oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(தள்ளு_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    வெறுமை visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }
    வெறுமை visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            மாறிலி AstClass* மாறிலி classp = VN_CAST(m_modp, Class);
            எனில் (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    வெறுமை visit(Ast_முனை_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_முனை_DType(nodep);
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        visit_Iterate_முனை_DType(nodep);
    }
    வெறுமை visit(AstParamTypeDType* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        visit_Iterate_முனை_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    வெறுமை visit(AstRefDType* nodep) override {
        visit_Iterate_முனை_DType(nodep);
        எனில் (!nodep->typedefp()) திரும்பு;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    வெறுமை visit_Iterate_முனை_DType(Ast_முனை_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        எனில் (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        எனில் (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.தள்ளு_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        தருக்க isVirtual = பொய்;
        எனில் (மாறிலி AstClass* மாறிலி classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        எனில் (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            மாறிலி AstClass* மாறிலி classp = VN_CAST(m_modp, Class);
            எனில் (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        தருக்க extended = பொய்;
        எனில் (மாறிலி AstClass* மாறிலி classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            க்கு (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                மாறிலி AstClass* மாறிலி eclassp = extendsp->classp();
                எனில் (Ast_முனை_FTask* மாறிலி fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), முனை_FTask)) {
                    எனில் (fbasep != nodep) {
                        extended = உண்மை;
                        எனில் (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        எனில் (fbasep->baseOverride().isFinal()) {
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
        எனில் (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        எனில் (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        எனில் (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    வெறுமை visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        எனில் (m_underAlwaysEdged) {
            மாறிலி தருக்க ignore = nodep->lhsp()->forall([&](மாறிலி AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                எனில் (refp->access().isReadOnly()) திரும்பு உண்மை;
                மாறிலி AstVar* மாறிலி varp = refp->varp();
                // Ignore ...
                திரும்பு varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            எனில் (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    வெறுமை visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = உண்மை;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    வெறுமை visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = பொய்;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    வெறுமை visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = பொய்;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        எனில் (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        எனில் (nodep->taskp()) nodep->taskp()->user2(1);
        எனில் (AstNew* மாறிலி newp = VN_CAST(nodep, New)) {
            எனில் (!VN_IS(newp->backp(), Assign)) திரும்பு;
            எனில் (AstClass* மாறிலி classp = VN_CAST(newp->classOrPackagep(), Class)) {
                எனில் (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.தள்ளு_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = உண்மை;
            iterateChildren(nodep);
        }
        editDType(nodep);
        எனில் (AstClassRefDType* மாறிலி classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    வெறுமை visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        எனில் (nodep->isFuncLocal() && nodep->direction() == VDirection::உள்ளீடு && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            தள்ளு_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    வெறுமை visit(Ast_முனை_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    வெறுமை visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        எனில் (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = உண்மை;
        iterateChildren(nodep);
        editDType(nodep);
    }
    வெறுமை visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = உண்மை;
        iterateChildren(nodep);
        editDType(nodep);
    }
    வெறுமை visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = உண்மை;
        iterateChildren(nodep);
        editDType(nodep);
    }
    வெறுமை visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    வெறுமை visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ பொய்));
    }
    வெறுமை visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = உண்மை;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            மாறிலி VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_முனை_Module* modp) {
                எனில் (AstClass* மாறிலி classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } இல்லையெனில் எனில் (AstIface* மாறிலி ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = இயல்பு;
};

//######################################################################
// V3WidthCommit class functions

வெறுமை V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("widthcommit", 0, dump_மரம்_Either_Level() >= 6);
}
