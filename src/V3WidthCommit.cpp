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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3WidthCommit.h"

#شامل "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
ساختار PairHash final {
    template <typename T, typename U>
    std::size_t operator()(ثابت std::pair<T, U>& x) ثابت {
        بازگشت std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

رده WidthCommitVisitor final : عمومی VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // STATE
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_گره_Module* m_modp = nullptr;  // Current module
    ثابت نویسه* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    منطقی m_contReads = نادرست;  // Check read continuous automatic variables
    منطقی m_dynsizedelem = نادرست;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    منطقی m_taskRefWarn = درست;  // Allow task reference warnings
    منطقی m_underSel = نادرست;  // Under AstMemberSel or AstSel
    منطقی m_underAlwaysEdged = نادرست;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_گره_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

عمومی:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

خصوصی:
    // METHODS
    تهی editDType(Ast_گره* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        اگر (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_گره_DType* editOneDType(Ast_گره_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        اگر (!nodep) بازگشت nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        اگر (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        اگر (AstBasicDType* ثابت bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ثابت newp = nodep->find_درج_Same_DType(bdtypep);
            اگر (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            بازگشت newp;
        }
        بازگشت nodep;
    }
    تهی classEncapCheck(Ast_گره* nodep, Ast_گره* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        منطقی local = نادرست;
        منطقی prot = نادرست;
        اگر (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            بازگشت;
        }
        اگر (ثابت auto anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وگرنه اگر (ثابت auto anodep = VN_CAST(defp, گره_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وگرنه اگر (ثابت auto anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وگرنه {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        اگر (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ثابت AstClass* refClassp = VN_CAST(m_modp, Class);
            اگر (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ثابت نویسه* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ثابت auto nestedAccess = [refClassp](ثابت AstClass*, ثابت Ast_گره* memberp) {
                بازگشت memberp == refClassp;
            };
            اگر (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } وگرنه اگر (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            اگر (how) {
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
    تهی varLifetimeCheck(Ast_گره* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        اگر (!m_underSel && m_contNbap) {
            std::string varType;
            ثابت Ast_گره_DType* ثابت varDtp = varp->dtypep()->skipRefp();
            اگر (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            وگرنه اگر (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            وگرنه اگر (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            اگر (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    تهی selLifetimeCheck(Ast_گره_عبارت* nodep, ثابت Ast_گره* namep) {
        اگر (m_underSel || !m_contNbap || !m_dynsizedelem) بازگشت;
        اگر (!nodep->isLValue() && !m_contReads) بازگشت;
        اگر (!nodep->dtypep()->skipRefp()->isDynamicallySized()) بازگشت;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    تهی deadCheckTasks() {
        برای (Ast_گره_FTask* taskp : m_tasksp) {
            اگر (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        برای (AstNew* newp : m_virtualNewsp) {
            اگر (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_گره::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    تهی virtualRecurse(Ast_گره_FTask* nodep, ثابت AstClass* classp, منطقی& isVirtualr) {
        ایستا std::unordered_map<std::pair<ثابت AstClass*, ثابت Ast_گره_FTask*>, منطقی, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        auto pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        اگر (!pair.second) {
            اگر (pair.first->second) isVirtualr = درست;
            بازگشت;
        }
        //
        اگر (nodep->isVirtual()) isVirtualr = درست;
        // Propagate value from extends/implements
        برای (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ثابت AstClass* ثابت eclassp = extendsp->classp();
            اگر (Ast_گره_FTask* ثابت fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), گره_FTask)) {
                اگر (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    ادامه;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        اگر (isVirtualr) {
            nodep->isVirtual(درست);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            برای (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ثابت AstClass* ثابت ebasep = extendsp->classp();
                اگر (ebasep->baseOverride().isFinal()) {
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
    تهی visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        اگر (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            ثابت منطقی noReads = nodep->forall(
                [&](ثابت Ast_گره_Var_Ref* refp) { بازگشت !refp->access().isReadOrRW(); });
            اگر (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
                بازگشت;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    تهی visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        اگر (!nodep->isTimingControl()) m_taskRefWarn = نادرست;
        iterateChildren(nodep);
        editDType(nodep);
    }
    تهی visit(Ast_Sen_درخت* nodep) override {
        اگر (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    تهی visit(AstAttrOf* nodep) override {
        انتخاب (nodep->attrType()) {
        حالت VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        حالت VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        پیشفرض:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    تهی visit(AstClassExtends* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        اگر (nodep->argsp()) افزودن_Deletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    تهی visit(AstConst* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        اگر (AstConst* ثابت newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            Ast_گره* ثابت oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(افزودن_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    تهی visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    تهی visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
            اگر (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    تهی visit(Ast_گره_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_گره_DType(nodep);
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        visit_Iterate_گره_DType(nodep);
    }
    تهی visit(AstParamTypeDType* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        visit_Iterate_گره_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    تهی visit(AstRefDType* nodep) override {
        visit_Iterate_گره_DType(nodep);
        اگر (!nodep->typedefp()) بازگشت;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    تهی visit_Iterate_گره_DType(Ast_گره_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        اگر (nodep->user1SetOnce()) بازگشت;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        اگر (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    تهی visit(Ast_گره_FTask* nodep) override {
        اگر (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.افزودن_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        منطقی isVirtual = نادرست;
        اگر (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        اگر (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
            اگر (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        منطقی extended = نادرست;
        اگر (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            برای (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ثابت AstClass* ثابت eclassp = extendsp->classp();
                اگر (Ast_گره_FTask* ثابت fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), گره_FTask)) {
                    اگر (fbasep != nodep) {
                        extended = درست;
                        اگر (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        اگر (fbasep->baseOverride().isFinal()) {
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
        اگر (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        اگر (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        اگر (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    تهی visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        اگر (m_underAlwaysEdged) {
            ثابت منطقی ignore = nodep->lhsp()->forall([&](ثابت AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                اگر (refp->access().isReadOnly()) بازگشت درست;
                ثابت AstVar* ثابت varp = refp->varp();
                // Ignore ...
                بازگشت varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            اگر (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    تهی visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = درست;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    تهی visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = نادرست;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    تهی visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = نادرست;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    تهی visit(Ast_گره_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        اگر (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        اگر (nodep->taskp()) nodep->taskp()->user2(1);
        اگر (AstNew* ثابت newp = VN_CAST(nodep, New)) {
            اگر (!VN_IS(newp->backp(), Assign)) بازگشت;
            اگر (AstClass* ثابت classp = VN_CAST(newp->classOrPackagep(), Class)) {
                اگر (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.افزودن_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    تهی visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = درست;
            iterateChildren(nodep);
        }
        editDType(nodep);
        اگر (AstClassRefDType* ثابت classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    تهی visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        اگر (nodep->isFuncLocal() && nodep->direction() == VDirection::درون_داد && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            افزودن_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    تهی visit(Ast_گره_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    تهی visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        اگر (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = درست;
        iterateChildren(nodep);
        editDType(nodep);
    }
    تهی visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = درست;
        iterateChildren(nodep);
        editDType(nodep);
    }
    تهی visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = درست;
        iterateChildren(nodep);
        editDType(nodep);
    }
    تهی visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    تهی visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ نادرست));
    }
    تهی visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = درست;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    تهی visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }
    تهی visit(Ast_گره* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ثابت VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_گره_Module* modp) {
                اگر (AstClass* ثابت classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } وگرنه اگر (AstIface* ثابت ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = پیشفرض;
};

//######################################################################
// V3WidthCommit class functions

تهی V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("widthcommit", 0, dump_درخت_Either_Level() >= 6);
}
