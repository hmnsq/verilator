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
ساخت PairHash final {
    سانچہ <قسم_نام T, قسم_نام U>
    std::سائز_t عامل()(ثابت std::pair<T, U>& x) ثابت {
        واپس std::hash<T>()(x.پہلا) + std::hash<U>()(x.دوسرا);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

جماعت چوڑائی_Commit_Visitor final : عوامی VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ثابت VNUser1_اندر_Use m_inuser1;
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE
    Ast_گرہ_FTask* m_ftaskp = خالی_اشاریہ;  // Current function/task
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    ثابت حرف* m_contNbap = خالی_اشاریہ;  // In continuous- or non-blocking assignment
    منطقی m_contReads = جھوٹ;  // Check read continuous automatic variables
    منطقی m_dynsizedelem = جھوٹ;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    منطقی m_taskRefWarn = سچ;  // Allow task reference warnings
    منطقی m_underSel = جھوٹ;  // Under AstMemberSel or AstSel
    منطقی m_underAlwaysEdged = جھوٹ;  // Under always with sequential SenTree
    std::سمتیہ<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::سمتیہ<Ast_گرہ_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

عوامی:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

نجی:
    // METHODS
    خالی editDType(Ast_گرہ* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        اگر (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warn_مزید() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_گرہ_DType* editOneDType(Ast_گرہ_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        اگر (!nodep) واپس خالی_اشاریہ;
        // Recurse to handle the data type, as may change the size etc of this type
        اگر (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        اگر (AstBasicDType* ثابت bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ثابت newp = nodep->ڈھونڈو_داخل_کرو_Same_DType(bdtypep);
            اگر (newp != bdtypep && debug() >= 9) {
                UINFO_سابقہ("dtype replacement ");
                nodep->dumpSmall(std::خارج);
                std::خارج << "  ---->  ";
                newp->dumpSmall(std::خارج);
                std::خارج << '\n';
            }
            واپس newp;
        }
        واپس nodep;
    }
    خالی classEncapCheck(Ast_گرہ* nodep, Ast_گرہ* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        منطقی local = جھوٹ;
        منطقی prot = جھوٹ;
        اگر (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            واپس;
        }
        اگر (ثابت خودکار anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ورنہ اگر (ثابت خودکار anodep = VN_CAST(defp, گرہ_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ورنہ اگر (ثابت خودکار anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ورنہ {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        اگر (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ثابت AstClass* refClassp = VN_CAST(m_modp, Class);
            اگر (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ثابت حرف* how = خالی_اشاریہ;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ثابت خودکار nestedAccess = [refClassp](ثابت AstClass*, ثابت Ast_گرہ* memberp) {
                واپس memberp == refClassp;
            };
            اگر (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } ورنہ اگر (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
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
    خالی varLifetimeCheck(Ast_گرہ* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        اگر (!m_underSel && m_contNbap) {
            std::string varType;
            ثابت Ast_گرہ_DType* ثابت varDtp = varp->dtypep()->skipRefp();
            اگر (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            ورنہ اگر (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            ورنہ اگر (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            اگر (!varType.خالی_خانے()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    خالی selLifetimeCheck(Ast_گرہ_اظہاریہ* nodep, ثابت Ast_گرہ* namep) {
        اگر (m_underSel || !m_contNbap || !m_dynsizedelem) واپس;
        اگر (!nodep->isLValue() && !m_contReads) واپس;
        اگر (!nodep->dtypep()->skipRefp()->isDynamicallySized()) واپس;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    خالی deadCheckTasks() {
        برائے (Ast_گرہ_FTask* taskp : m_tasksp) {
            اگر (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        برائے (AstNew* newp : m_virtualNewsp) {
            اگر (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_گرہ::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    خالی virtualRecurse(Ast_گرہ_FTask* nodep, ثابت AstClass* classp, منطقی& isVirtualr) {
        ساکن std::unordered_map<std::pair<ثابت AstClass*, ثابت Ast_گرہ_FTask*>, منطقی, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        خودکار pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        اگر (!pair.دوسرا) {
            اگر (pair.پہلا->دوسرا) isVirtualr = سچ;
            واپس;
        }
        //
        اگر (nodep->isVirtual()) isVirtualr = سچ;
        // Propagate value from extends/implements
        برائے (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ثابت AstClass* ثابت eclassp = extendsp->classp();
            اگر (Ast_گرہ_FTask* ثابت fbasep
                = VN_CAST(m_memberMap.ڈھونڈو_Member(eclassp, nodep->name()), گرہ_FTask)) {
                اگر (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    جاری;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        اگر (isVirtualr) {
            nodep->isVirtual(سچ);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            برائے (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ثابت AstClass* ثابت ebasep = extendsp->classp();
                اگر (ebasep->بنیاد_Override().isFinal()) {
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
    خالی دورہ(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        اگر (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->is_Combo_ستارہ()) {
            ثابت منطقی noReads = nodep->forall(
                [&](ثابت Ast_گرہ_Var_Ref* refp) { واپس !refp->access().isReadOrRW(); });
            اگر (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warn_مزید()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                واپس;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->has_کنارہ();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    خالی دورہ(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        اگر (!nodep->isTimingControl()) m_taskRefWarn = جھوٹ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    خالی دورہ(Ast_Sen_درخت* nodep) override {
        اگر (nodep->sensesp() && nodep->sensesp()->is_Combo_ستارہ()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    خالی دورہ(Ast_Attr_کا* nodep) override {
        انتخاب (nodep->attrType()) {
        حالت VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        حالت VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            واپس;
        طےشدہ:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    خالی دورہ(AstClassExtends* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        اگر (nodep->argsp()) ڈالو_Deletep(nodep->argsp()->unlink_Fr_عقب_With_اگلا());
        iterateChildren(nodep);
    }
    خالی دورہ(AstConst* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        اگر (AstConst* ثابت newp = V3_چوڑائی_Commit::new_If_Const_Commit_سائز(nodep)) {
            nodep->replaceWith(newp);
            Ast_گرہ* ثابت oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(ڈالو_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    خالی دورہ(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlink_Fr_عقب());
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
            اگر (nodep->is_Kwd_خالص()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    خالی دورہ(Ast_گرہ_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        دورہ_Iterate_گرہ_DType(nodep);
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        دورہ_Iterate_گرہ_DType(nodep);
    }
    خالی دورہ(AstParamTypeDType* nodep) override {
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        دورہ_Iterate_گرہ_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlink_Fr_عقب();  // Make non-child
        v3Global.rootp()->typeTablep()->جوڑو_Typesp(nodep);
    }
    خالی دورہ(AstRefDType* nodep) override {
        دورہ_Iterate_گرہ_DType(nodep);
        اگر (!nodep->typedefp()) واپس;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(خالی_اشاریہ);  // No longer needed
    }
    خالی دورہ_Iterate_گرہ_DType(Ast_گرہ_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        اگر (nodep->user1_سیٹ_Once()) واپس;  // Process once
        nodep->چوڑائی_Min_From_چوڑائی();
        // Too late to any unspecified sign to be anything but unsigned
        اگر (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        اگر (!nodep->task_عوامی() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.ڈالو_عقب(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        منطقی isVirtual = جھوٹ;
        اگر (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        اگر (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
            اگر (nodep->class_طریقہ() && nodep->خالص_Virtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        منطقی extended = جھوٹ;
        اگر (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            برائے (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ثابت AstClass* ثابت eclassp = extendsp->classp();
                اگر (Ast_گرہ_FTask* ثابت fbasep
                    = VN_CAST(m_memberMap.ڈھونڈو_Member(eclassp, nodep->name()), گرہ_FTask)) {
                    اگر (fbasep != nodep) {
                        extended = سچ;
                        اگر (nodep->بنیاد_Override().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        اگر (fbasep->بنیاد_Override().isFinal()) {
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
        اگر (!extended && nodep->بنیاد_Override().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        اگر (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        اگر (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    خالی دورہ(Ast_تفویض* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        اگر (m_underAlwaysEdged) {
            ثابت منطقی ignore = nodep->lhsp()->forall([&](ثابت AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                اگر (refp->access().isReadOnly()) واپس سچ;
                ثابت AstVar* ثابت varp = refp->varp();
                // Ignore ...
                واپس varp->is_مستعمل_Loop_Idx()  // ... loop indices
                       || varp->is_عارضی()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3_Error_رمز::BLKSEQ);  // ... user said so
            });
            اگر (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warn_مزید()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    خالی دورہ(Ast_تفویض_Cont* nodep) override {
        iterate_And_اگلا_Null(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = سچ;
            iterate_And_اگلا_Null(nodep->lhsp());
            iterate_And_اگلا_Null(nodep->rhsp());
        }
        editDType(nodep);
    }
    خالی دورہ(Ast_تفویض_Dly* nodep) override {
        iterate_And_اگلا_Null(nodep->timingControlp());
        iterate_And_اگلا_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = جھوٹ;
            iterate_And_اگلا_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    خالی دورہ(Ast_تفویض_W* nodep) override {
        iterate_And_اگلا_Null(nodep->timingControlp());
        iterate_And_اگلا_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = جھوٹ;
            iterate_And_اگلا_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        اگر (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
            && m_ftaskp->verilogFunction() && m_taskRefWarn) {
            nodep->v3warn(FUNCTIMECTL,
                          "Functions cannot invoke tasks (IEEE 1800-2023 13.4)\n"
                              << nodep->warnContextPrimary() << "\n"
                              << nodep->warn_مزید() << "... Suggest make caller 'function "
                              << m_ftaskp->prettyName() << "' a task\n"
                              << m_ftaskp->warnContextSecondary() << "\n"
                              << nodep->warn_مزید() << "... Or, suggest make called 'task "
                              << nodep->taskp()->prettyName() << "' a function void\n"
                              << nodep->taskp()->warnContextSecondary());
        }
        اگر (nodep->taskp()) nodep->taskp()->user2(1);
        اگر (AstNew* ثابت newp = VN_CAST(nodep, New)) {
            اگر (!VN_IS(newp->backp(), تفویض)) واپس;
            اگر (AstClass* ثابت classp = VN_CAST(newp->classOrPackagep(), Class)) {
                اگر (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.ڈالو_عقب(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    خالی دورہ(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = سچ;
            iterateChildren(nodep);
        }
        editDType(nodep);
        اگر (AstClassRefDType* ثابت classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    خالی دورہ(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        اگر (nodep->isFuncLocal() && nodep->direction() == VDirection::داخلہ && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            ڈالو_Deletep(nodep->valuep()->unlink_Fr_عقب());
        }
    }
    خالی دورہ(Ast_گرہ_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    خالی دورہ(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        اگر (nodep->طریقہ() == VCMethod::ARRAY_AT || nodep->طریقہ() == VCMethod::ARRAY_AT_WRITE
            || nodep->طریقہ() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = سچ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    خالی دورہ(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = سچ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    خالی دورہ(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = سچ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    خالی دورہ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    خالی دورہ(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->بنیاد_Fromp(/* overMembers: */ جھوٹ));
    }
    خالی دورہ(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = سچ;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    خالی دورہ(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
    }
    خالی دورہ(Ast_گرہ* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

عوامی:
    // CONSTRUCTORS
    صریح چوڑائی_Commit_Visitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ثابت VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_گرہ_Module* modp) {
                اگر (AstClass* ثابت classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } ورنہ اگر (AstIface* ثابت ifacep = VN_CAST(modp, Iface)) {
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
    ~چوڑائی_Commit_Visitor() override = طےشدہ;
};

//######################################################################
// V3WidthCommit class functions

خالی V3_چوڑائی_Commit::چوڑائی_Commit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { چوڑائی_Commit_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("widthcommit", 0, dump_درخت_Either_Level() >= 6);
}
