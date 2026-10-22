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
    سانچو <قسم_نالو T, قسم_نالو U>
    std::سائيز_t عامل()(ثابت std::pair<T, U>& x) ثابت {
        أرجع std::hash<T>()(x.پهريون) + std::hash<U>()(x.ٻيو);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

صنف ويڪر_Commit_Visitor final : عام VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;

    // STATE
    Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;  // Current function/task
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    ثابت حرف* m_contNbap = خالي_اشارو;  // In continuous- or non-blocking assignment
    منطقي m_contReads = خطأ;  // Check read continuous automatic variables
    منطقي m_dynsizedelem = خطأ;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    منطقي m_taskRefWarn = سچ;  // Allow task reference warnings
    منطقي m_underSel = خطأ;  // Under AstMemberSel or AstSel
    منطقي m_underAlwaysEdged = خطأ;  // Under always with sequential SenTree
    std::ويڪٽر<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::ويڪٽر<Ast_ڳنڍ_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

عام:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

خاص:
    // METHODS
    فراغ editDType(Ast_ڳنڍ* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        إذا (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warn_وڌيڪ() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_ڳنڍ_DType* editOneDType(Ast_ڳنڍ_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        إذا (!nodep) أرجع خالي_اشارو;
        // Recurse to handle the data type, as may change the size etc of this type
        إذا (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        إذا (AstBasicDType* ثابت bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ثابت newp = nodep->ڳوليو_وجهو_Same_DType(bdtypep);
            إذا (newp != bdtypep && debug() >= 9) {
                UINFO_اڳياڙي("dtype replacement ");
                nodep->dumpSmall(std::ٻاهر);
                std::ٻاهر << "  ---->  ";
                newp->dumpSmall(std::ٻاهر);
                std::ٻاهر << '\n';
            }
            أرجع newp;
        }
        أرجع nodep;
    }
    فراغ classEncapCheck(Ast_ڳنڍ* nodep, Ast_ڳنڍ* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        منطقي local = خطأ;
        منطقي prot = خطأ;
        إذا (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            أرجع;
        }
        إذا (ثابت خودڪار anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وإلا إذا (ثابت خودڪار anodep = VN_CAST(defp, ڳنڍ_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وإلا إذا (ثابت خودڪار anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } وإلا {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        إذا (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ثابت AstClass* refClassp = VN_CAST(m_modp, Class);
            إذا (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ثابت حرف* how = خالي_اشارو;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ثابت خودڪار nestedAccess = [refClassp](ثابت AstClass*, ثابت Ast_ڳنڍ* memberp) {
                أرجع memberp == refClassp;
            };
            إذا (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } وإلا إذا (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            إذا (how) {
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
    فراغ varLifetimeCheck(Ast_ڳنڍ* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        إذا (!m_underSel && m_contNbap) {
            std::string varType;
            ثابت Ast_ڳنڍ_DType* ثابت varDtp = varp->dtypep()->skipRefp();
            إذا (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            وإلا إذا (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            وإلا إذا (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            إذا (!varType.خالي_جايون()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    فراغ selLifetimeCheck(Ast_ڳنڍ_اظهار* nodep, ثابت Ast_ڳنڍ* namep) {
        إذا (m_underSel || !m_contNbap || !m_dynsizedelem) أرجع;
        إذا (!nodep->isLValue() && !m_contReads) أرجع;
        إذا (!nodep->dtypep()->skipRefp()->isDynamicallySized()) أرجع;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    فراغ deadCheckTasks() {
        لكل (Ast_ڳنڍ_FTask* taskp : m_tasksp) {
            إذا (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        لكل (AstNew* newp : m_virtualNewsp) {
            إذا (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_ڳنڍ::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    فراغ virtualRecurse(Ast_ڳنڍ_FTask* nodep, ثابت AstClass* classp, منطقي& isVirtualr) {
        ساكن std::unordered_map<std::pair<ثابت AstClass*, ثابت Ast_ڳنڍ_FTask*>, منطقي, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        خودڪار pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        إذا (!pair.ٻيو) {
            إذا (pair.پهريون->ٻيو) isVirtualr = سچ;
            أرجع;
        }
        //
        إذا (nodep->isVirtual()) isVirtualr = سچ;
        // Propagate value from extends/implements
        لكل (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ثابت AstClass* ثابت eclassp = extendsp->classp();
            إذا (Ast_ڳنڍ_FTask* ثابت fbasep
                = VN_CAST(m_memberMap.ڳوليو_Member(eclassp, nodep->name()), ڳنڍ_FTask)) {
                إذا (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    استمر;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        إذا (isVirtualr) {
            nodep->isVirtual(سچ);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            لكل (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ثابت AstClass* ثابت ebasep = extendsp->classp();
                إذا (ebasep->بنياد_Override().isFinal()) {
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
    فراغ گهمو(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        إذا (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->is_Combo_تارو()) {
            ثابت منطقي noReads = nodep->forall(
                [&](ثابت Ast_ڳنڍ_Var_Ref* refp) { أرجع !refp->access().isReadOrRW(); });
            إذا (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warn_وڌيڪ()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                أرجع;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->has_ڪنارو();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ گهمو(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        إذا (!nodep->isTimingControl()) m_taskRefWarn = خطأ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ گهمو(Ast_Sen_وڻ* nodep) override {
        إذا (nodep->sensesp() && nodep->sensesp()->is_Combo_تارو()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ گهمو(Ast_Attr_جو* nodep) override {
        بدل (nodep->attrType()) {
        حالة VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        حالة VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            أرجع;
        افتراضي:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ گهمو(AstClassExtends* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        إذا (nodep->argsp()) ڌڪو_Deletep(nodep->argsp()->unlink_Fr_پٺ_With_ايندڙ());
        iterateChildren(nodep);
    }
    فراغ گهمو(AstConst* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        إذا (AstConst* ثابت newp = V3_ويڪر_Commit::new_If_Const_Commit_سائيز(nodep)) {
            nodep->replaceWith(newp);
            Ast_ڳنڍ* ثابت oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(ڌڪو_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    فراغ گهمو(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlink_Fr_پٺ());
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
            إذا (nodep->is_Kwd_خالص()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    فراغ گهمو(Ast_ڳنڍ_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        گهمو_Iterate_ڳنڍ_DType(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_UOr_Struct_DType* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;  // Process once
        گهمو_Iterate_ڳنڍ_DType(nodep);
    }
    فراغ گهمو(AstParamTypeDType* nodep) override {
        إذا (nodep->user1_سيٽ_Once()) أرجع;  // Process once
        گهمو_Iterate_ڳنڍ_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlink_Fr_پٺ();  // Make non-child
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(nodep);
    }
    فراغ گهمو(AstRefDType* nodep) override {
        گهمو_Iterate_ڳنڍ_DType(nodep);
        إذا (!nodep->typedefp()) أرجع;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(خالي_اشارو);  // No longer needed
    }
    فراغ گهمو_Iterate_ڳنڍ_DType(Ast_ڳنڍ_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        إذا (nodep->user1_سيٽ_Once()) أرجع;  // Process once
        nodep->ويڪر_Min_From_ويڪر();
        // Too late to any unspecified sign to be anything but unsigned
        إذا (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        إذا (!nodep->task_عوامي() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.ڌڪو_پٺ(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        منطقي isVirtual = خطأ;
        إذا (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        إذا (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
            إذا (nodep->class_طريقو() && nodep->خالص_Virtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        منطقي extended = خطأ;
        إذا (ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            لكل (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ثابت AstClass* ثابت eclassp = extendsp->classp();
                إذا (Ast_ڳنڍ_FTask* ثابت fbasep
                    = VN_CAST(m_memberMap.ڳوليو_Member(eclassp, nodep->name()), ڳنڍ_FTask)) {
                    إذا (fbasep != nodep) {
                        extended = سچ;
                        إذا (nodep->بنياد_Override().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        إذا (fbasep->بنياد_Override().isFinal()) {
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
        إذا (!extended && nodep->بنياد_Override().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        إذا (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        إذا (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    فراغ گهمو(Ast_سونپيو* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        إذا (m_underAlwaysEdged) {
            ثابت منطقي ignore = nodep->lhsp()->forall([&](ثابت AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                إذا (refp->access().isReadOnly()) أرجع سچ;
                ثابت AstVar* ثابت varp = refp->varp();
                // Ignore ...
                أرجع varp->is_استعمال_ٿيل_Loop_Idx()  // ... loop indices
                       || varp->is_عارضي()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3_Error_ڪوڊ::BLKSEQ);  // ... user said so
            });
            إذا (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warn_وڌيڪ()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    فراغ گهمو(Ast_سونپيو_Cont* nodep) override {
        iterate_And_ايندڙ_Null(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = سچ;
            iterate_And_ايندڙ_Null(nodep->lhsp());
            iterate_And_ايندڙ_Null(nodep->rhsp());
        }
        editDType(nodep);
    }
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override {
        iterate_And_ايندڙ_Null(nodep->timingControlp());
        iterate_And_ايندڙ_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = خطأ;
            iterate_And_ايندڙ_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    فراغ گهمو(Ast_سونپيو_W* nodep) override {
        iterate_And_ايندڙ_Null(nodep->timingControlp());
        iterate_And_ايندڙ_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = خطأ;
            iterate_And_ايندڙ_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        إذا (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
            && m_ftaskp->verilogFunction() && m_taskRefWarn) {
            nodep->v3warn(FUNCTIMECTL,
                          "Functions cannot invoke tasks (IEEE 1800-2023 13.4)\n"
                              << nodep->warnContextPrimary() << "\n"
                              << nodep->warn_وڌيڪ() << "... Suggest make caller 'function "
                              << m_ftaskp->prettyName() << "' a task\n"
                              << m_ftaskp->warnContextSecondary() << "\n"
                              << nodep->warn_وڌيڪ() << "... Or, suggest make called 'task "
                              << nodep->taskp()->prettyName() << "' a function void\n"
                              << nodep->taskp()->warnContextSecondary());
        }
        إذا (nodep->taskp()) nodep->taskp()->user2(1);
        إذا (AstNew* ثابت newp = VN_CAST(nodep, New)) {
            إذا (!VN_IS(newp->backp(), سونپيو)) أرجع;
            إذا (AstClass* ثابت classp = VN_CAST(newp->classOrPackagep(), Class)) {
                إذا (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.ڌڪو_پٺ(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = سچ;
            iterateChildren(nodep);
        }
        editDType(nodep);
        إذا (AstClassRefDType* ثابت classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    فراغ گهمو(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        إذا (nodep->isFuncLocal() && nodep->direction() == VDirection::داخلا && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            ڌڪو_Deletep(nodep->valuep()->unlink_Fr_پٺ());
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    فراغ گهمو(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        إذا (nodep->طريقو() == VCMethod::ARRAY_AT || nodep->طريقو() == VCMethod::ARRAY_AT_WRITE
            || nodep->طريقو() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = سچ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ گهمو(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = سچ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ گهمو(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = سچ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    فراغ گهمو(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    فراغ گهمو(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->بنياد_Fromp(/* overMembers: */ خطأ));
    }
    فراغ گهمو(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = سچ;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    فراغ گهمو(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

عام:
    // CONSTRUCTORS
    واضح ويڪر_Commit_Visitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ثابت VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_ڳنڍ_Module* modp) {
                إذا (AstClass* ثابت classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } وإلا إذا (AstIface* ثابت ifacep = VN_CAST(modp, Iface)) {
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
    ~ويڪر_Commit_Visitor() override = افتراضي;
};

//######################################################################
// V3WidthCommit class functions

فراغ V3_ويڪر_Commit::ويڪر_Commit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ويڪر_Commit_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("widthcommit", 0, dump_وڻ_Either_Level() >= 6);
}
