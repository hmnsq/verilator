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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3WidthCommit.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
ਬਣਤਰ PairHash final {
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T, ਕਿਸਮ_ਨਾਂ U>
    std::ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ std::pair<T, U>& x) ਅਚਲ {
        ਵਾਪਸ std::hash<T>()(x.ਪਹਿਲਾ) + std::hash<U>()(x.ਦੂਜਾ);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

ਵਰਗ ਚੌੜਾਈ_Commit_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    ਅਚਲ ਅੱਖਰ* m_contNbap = ਖਾਲੀ_ਸੂਚਕ;  // In continuous- or non-blocking assignment
    ਤਾਰਕਿਕ m_contReads = ਝੂਠ;  // Check read continuous automatic variables
    ਤਾਰਕਿਕ m_dynsizedelem = ਝੂਠ;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    ਤਾਰਕਿਕ m_taskRefWarn = ਸੱਚ;  // Allow task reference warnings
    ਤਾਰਕਿਕ m_underSel = ਝੂਠ;  // Under AstMemberSel or AstSel
    ਤਾਰਕਿਕ m_underAlwaysEdged = ਝੂਠ;  // Under always with sequential SenTree
    std::ਸਦਿਸ਼<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

ਜਨਤਕ:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

ਨਿਜੀ:
    // METHODS
    ਖਾਲੀ editDType(Ast_ਗੰਢ* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        ਜੇ (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warn_ਹੋਰ() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_ਗੰਢ_DType* editOneDType(Ast_ਗੰਢ_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        ਜੇ (!nodep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // Recurse to handle the data type, as may change the size etc of this type
        ਜੇ (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        ਜੇ (AstBasicDType* ਅਚਲ bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* ਅਚਲ newp = nodep->ਲੱਭੋ_ਪਾਓ_Same_DType(bdtypep);
            ਜੇ (newp != bdtypep && debug() >= 9) {
                UINFO_ਅਗੇਤਰ("dtype replacement ");
                nodep->dumpSmall(std::ਬਾਹਰ);
                std::ਬਾਹਰ << "  ---->  ";
                newp->dumpSmall(std::ਬਾਹਰ);
                std::ਬਾਹਰ << '\n';
            }
            ਵਾਪਸ newp;
        }
        ਵਾਪਸ nodep;
    }
    ਖਾਲੀ classEncapCheck(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        ਤਾਰਕਿਕ local = ਝੂਠ;
        ਤਾਰਕਿਕ prot = ਝੂਠ;
        ਜੇ (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            ਵਾਪਸ;
        }
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ anodep = VN_CAST(defp, ਗੰਢ_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        ਜੇ (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            ਅਚਲ AstClass* refClassp = VN_CAST(m_modp, Class);
            ਜੇ (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            ਅਚਲ ਅੱਖਰ* how = ਖਾਲੀ_ਸੂਚਕ;
            // Inner nested classes can access `local` or `protected` members of their outer class
            ਅਚਲ ਸਵੈਚਾਲਿਤ nestedAccess = [refClassp](ਅਚਲ AstClass*, ਅਚਲ Ast_ਗੰਢ* memberp) {
                ਵਾਪਸ memberp == refClassp;
            };
            ਜੇ (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } ਨਹੀਂਤਾਂ ਜੇ (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            ਜੇ (how) {
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
    ਖਾਲੀ varLifetimeCheck(Ast_ਗੰਢ* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        ਜੇ (!m_underSel && m_contNbap) {
            std::string varType;
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ varDtp = varp->dtypep()->skipRefp();
            ਜੇ (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            ਨਹੀਂਤਾਂ ਜੇ (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            ਨਹੀਂਤਾਂ ਜੇ (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            ਜੇ (!varType.ਖਾਲੀ_ਥਾਵਾਂ()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    ਖਾਲੀ selLifetimeCheck(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਅਚਲ Ast_ਗੰਢ* namep) {
        ਜੇ (m_underSel || !m_contNbap || !m_dynsizedelem) ਵਾਪਸ;
        ਜੇ (!nodep->isLValue() && !m_contReads) ਵਾਪਸ;
        ਜੇ (!nodep->dtypep()->skipRefp()->isDynamicallySized()) ਵਾਪਸ;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    ਖਾਲੀ deadCheckTasks() {
        ਲਈ (Ast_ਗੰਢ_FTask* taskp : m_tasksp) {
            ਜੇ (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        ਲਈ (AstNew* newp : m_virtualNewsp) {
            ਜੇ (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_ਗੰਢ::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    ਖਾਲੀ virtualRecurse(Ast_ਗੰਢ_FTask* nodep, ਅਚਲ AstClass* classp, ਤਾਰਕਿਕ& isVirtualr) {
        ਸਥਿਰ std::unordered_map<std::pair<ਅਚਲ AstClass*, ਅਚਲ Ast_ਗੰਢ_FTask*>, ਤਾਰਕਿਕ, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        ਸਵੈਚਾਲਿਤ pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        ਜੇ (!pair.ਦੂਜਾ) {
            ਜੇ (pair.ਪਹਿਲਾ->ਦੂਜਾ) isVirtualr = ਸੱਚ;
            ਵਾਪਸ;
        }
        //
        ਜੇ (nodep->isVirtual()) isVirtualr = ਸੱਚ;
        // Propagate value from extends/implements
        ਲਈ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ਅਚਲ AstClass* ਅਚਲ eclassp = extendsp->classp();
            ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ fbasep
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(eclassp, nodep->name()), ਗੰਢ_FTask)) {
                ਜੇ (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    ਜਾਰੀ;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        ਜੇ (isVirtualr) {
            nodep->isVirtual(ਸੱਚ);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(nodep, Class)) {
            ਲਈ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ਅਚਲ AstClass* ਅਚਲ ebasep = extendsp->classp();
                ਜੇ (ebasep->ਅਧਾਰ_Override().isFinal()) {
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
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        ਜੇ (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->is_Combo_ਤਾਰਾ()) {
            ਅਚਲ ਤਾਰਕਿਕ noReads = nodep->forall(
                [&](ਅਚਲ Ast_ਗੰਢ_Var_Ref* refp) { ਵਾਪਸ !refp->access().isReadOrRW(); });
            ਜੇ (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warn_ਹੋਰ()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->has_ਕਿਨਾਰਾ();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        ਜੇ (!nodep->isTimingControl()) m_taskRefWarn = ਝੂਠ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override {
        ਜੇ (nodep->sensesp() && nodep->sensesp()->is_Combo_ਤਾਰਾ()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Attr_ਦਾ* nodep) override {
        ਚੋਣ (nodep->attrType()) {
        ਹਾਲਤ VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        ਹਾਲਤ VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        ਮੂਲ:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassExtends* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        ਜੇ (nodep->argsp()) ਧੱਕੋ_Deletep(nodep->argsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        ਜੇ (AstConst* ਅਚਲ newp = V3_ਚੌੜਾਈ_Commit::new_If_Const_Commit_ਮਾਪ(nodep)) {
            nodep->replaceWith(newp);
            Ast_ਗੰਢ* ਅਚਲ oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(m_modp, Class);
            ਜੇ (nodep->is_Kwd_ਸ਼ੁੱਧ()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        ਦੇਖੋ_Iterate_ਗੰਢ_DType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        ਦੇਖੋ_Iterate_ਗੰਢ_DType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstParamTypeDType* nodep) override {
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        ਦੇਖੋ_Iterate_ਗੰਢ_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlink_Fr_ਪਿੱਠ();  // Make non-child
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        ਦੇਖੋ_Iterate_ਗੰਢ_DType(nodep);
        ਜੇ (!nodep->typedefp()) ਵਾਪਸ;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(ਖਾਲੀ_ਸੂਚਕ);  // No longer needed
    }
    ਖਾਲੀ ਦੇਖੋ_Iterate_ਗੰਢ_DType(Ast_ਗੰਢ_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        ਜੇ (nodep->user1_ਸੈੱਟ_Once()) ਵਾਪਸ;  // Process once
        nodep->ਚੌੜਾਈ_Min_From_ਚੌੜਾਈ();
        // Too late to any unspecified sign to be anything but unsigned
        ਜੇ (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        ਜੇ (!nodep->task_ਜਨਤਕ() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.ਧੱਕੋ_ਪਿੱਠ(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ਤਾਰਕਿਕ isVirtual = ਝੂਠ;
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        ਜੇ (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(m_modp, Class);
            ਜੇ (nodep->class_ਵਿਧੀ() && nodep->ਸ਼ੁੱਧ_Virtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        ਤਾਰਕਿਕ extended = ਝੂਠ;
        ਜੇ (ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            ਲਈ (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                ਅਚਲ AstClass* ਅਚਲ eclassp = extendsp->classp();
                ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ fbasep
                    = VN_CAST(m_memberMap.ਲੱਭੋ_Member(eclassp, nodep->name()), ਗੰਢ_FTask)) {
                    ਜੇ (fbasep != nodep) {
                        extended = ਸੱਚ;
                        ਜੇ (nodep->ਅਧਾਰ_Override().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        ਜੇ (fbasep->ਅਧਾਰ_Override().isFinal()) {
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
        ਜੇ (!extended && nodep->ਅਧਾਰ_Override().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        ਜੇ (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        ਜੇ (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        ਜੇ (m_underAlwaysEdged) {
            ਅਚਲ ਤਾਰਕਿਕ ignore = nodep->lhsp()->forall([&](ਅਚਲ AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                ਜੇ (refp->access().isReadOnly()) ਵਾਪਸ ਸੱਚ;
                ਅਚਲ AstVar* ਅਚਲ varp = refp->varp();
                // Ignore ...
                ਵਾਪਸ varp->is_ਵਰਤਿਆ_Loop_Idx()  // ... loop indices
                       || varp->is_ਆਰਜ਼ੀ()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3_Error_ਕੋਡ::BLKSEQ);  // ... user said so
            });
            ਜੇ (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warn_ਹੋਰ()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Cont* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
            iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
        }
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->timingControlp());
        iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = ਝੂਠ;
            iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {
        iterate_And_ਅਗਲਾ_Null(nodep->timingControlp());
        iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = ਝੂਠ;
            iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        ਜੇ (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
            && m_ftaskp->verilogFunction() && m_taskRefWarn) {
            nodep->v3warn(FUNCTIMECTL,
                          "Functions cannot invoke tasks (IEEE 1800-2023 13.4)\n"
                              << nodep->warnContextPrimary() << "\n"
                              << nodep->warn_ਹੋਰ() << "... Suggest make caller 'function "
                              << m_ftaskp->prettyName() << "' a task\n"
                              << m_ftaskp->warnContextSecondary() << "\n"
                              << nodep->warn_ਹੋਰ() << "... Or, suggest make called 'task "
                              << nodep->taskp()->prettyName() << "' a function void\n"
                              << nodep->taskp()->warnContextSecondary());
        }
        ਜੇ (nodep->taskp()) nodep->taskp()->user2(1);
        ਜੇ (AstNew* ਅਚਲ newp = VN_CAST(nodep, New)) {
            ਜੇ (!VN_IS(newp->backp(), ਸੌਂਪੋ)) ਵਾਪਸ;
            ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(newp->classOrPackagep(), Class)) {
                ਜੇ (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.ਧੱਕੋ_ਪਿੱਠ(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = ਸੱਚ;
            iterateChildren(nodep);
        }
        editDType(nodep);
        ਜੇ (AstClassRefDType* ਅਚਲ classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        ਜੇ (nodep->isFuncLocal() && nodep->direction() == VDirection::ਨਿਵੇਸ਼ && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            ਧੱਕੋ_Deletep(nodep->valuep()->unlink_Fr_ਪਿੱਠ());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        ਜੇ (nodep->ਵਿਧੀ() == VCMethod::ARRAY_AT || nodep->ਵਿਧੀ() == VCMethod::ARRAY_AT_WRITE
            || nodep->ਵਿਧੀ() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = ਸੱਚ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = ਸੱਚ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = ਸੱਚ;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->ਅਧਾਰ_Fromp(/* overMembers: */ ਝੂਠ));
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = ਸੱਚ;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਚੌੜਾਈ_Commit_Visitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            ਅਚਲ VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_ਗੰਢ_Module* modp) {
                ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } ਨਹੀਂਤਾਂ ਜੇ (AstIface* ਅਚਲ ifacep = VN_CAST(modp, Iface)) {
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
    ~ਚੌੜਾਈ_Commit_Visitor() override = ਮੂਲ;
};

//######################################################################
// V3WidthCommit class functions

ਖਾਲੀ V3_ਚੌੜਾਈ_Commit::ਚੌੜਾਈ_Commit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਚੌੜਾਈ_Commit_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("widthcommit", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
