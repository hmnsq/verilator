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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3WidthCommit.h"

#అన్తర్భావ "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
నిర్మాణం PairHash final {
    మూస <రకం_పేరు T, రకం_పేరు U>
    std::కొలత_t క్రియాకారకం()(స్థిరరాశి std::pair<T, U>& x) స్థిరరాశి {
        తిరిగి std::hash<T>()(x.మొదటి) + std::hash<U>()(x.రెండవ);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

తరగతి వెడల్పు_Commit_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    స్థిరరాశి అక్షరం* m_contNbap = శూన్యసూచిక;  // In continuous- or non-blocking assignment
    తార్కిక m_contReads = అబద్ధం;  // Check read continuous automatic variables
    తార్కిక m_dynsizedelem = అబద్ధం;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    తార్కిక m_taskRefWarn = నిజం;  // Allow task reference warnings
    తార్కిక m_underSel = అబద్ధం;  // Under AstMemberSel or AstSel
    తార్కిక m_underAlwaysEdged = అబద్ధం;  // Under always with sequential SenTree
    std::సదిశ<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::సదిశ<Ast_కణుపు_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

బహిరంగ:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

అంతరంగ:
    // METHODS
    శూన్య editDType(Ast_కణుపు* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        ఒకవేళ (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warn_మరిన్ని() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_కణుపు_DType* editOneDType(Ast_కణుపు_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        ఒకవేళ (!nodep) తిరిగి శూన్యసూచిక;
        // Recurse to handle the data type, as may change the size etc of this type
        ఒకవేళ (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        ఒకవేళ (AstBasicDType* స్థిరరాశి bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* స్థిరరాశి newp = nodep->కనుగొను_చొప్పించు_Same_DType(bdtypep);
            ఒకవేళ (newp != bdtypep && debug() >= 9) {
                UINFO_ఉపసర్గ("dtype replacement ");
                nodep->dumpSmall(std::బయటకు);
                std::బయటకు << "  ---->  ";
                newp->dumpSmall(std::బయటకు);
                std::బయటకు << '\n';
            }
            తిరిగి newp;
        }
        తిరిగి nodep;
    }
    శూన్య classEncapCheck(Ast_కణుపు* nodep, Ast_కణుపు* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        తార్కిక local = అబద్ధం;
        తార్కిక prot = అబద్ధం;
        ఒకవేళ (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            తిరిగి;
        }
        ఒకవేళ (స్థిరరాశి స్వయంచాలక anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } లేకపోతే ఒకవేళ (స్థిరరాశి స్వయంచాలక anodep = VN_CAST(defp, కణుపు_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } లేకపోతే ఒకవేళ (స్థిరరాశి స్వయంచాలక anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } లేకపోతే {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        ఒకవేళ (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            స్థిరరాశి AstClass* refClassp = VN_CAST(m_modp, Class);
            ఒకవేళ (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            స్థిరరాశి అక్షరం* how = శూన్యసూచిక;
            // Inner nested classes can access `local` or `protected` members of their outer class
            స్థిరరాశి స్వయంచాలక nestedAccess = [refClassp](స్థిరరాశి AstClass*, స్థిరరాశి Ast_కణుపు* memberp) {
                తిరిగి memberp == refClassp;
            };
            ఒకవేళ (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } లేకపోతే ఒకవేళ (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            ఒకవేళ (how) {
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
    శూన్య varLifetimeCheck(Ast_కణుపు* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        ఒకవేళ (!m_underSel && m_contNbap) {
            std::string varType;
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి varDtp = varp->dtypep()->skipRefp();
            ఒకవేళ (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            లేకపోతే ఒకవేళ (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            లేకపోతే ఒకవేళ (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            ఒకవేళ (!varType.ఖాళీలు()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    శూన్య selLifetimeCheck(Ast_కణుపు_సమాసం* nodep, స్థిరరాశి Ast_కణుపు* namep) {
        ఒకవేళ (m_underSel || !m_contNbap || !m_dynsizedelem) తిరిగి;
        ఒకవేళ (!nodep->isLValue() && !m_contReads) తిరిగి;
        ఒకవేళ (!nodep->dtypep()->skipRefp()->isDynamicallySized()) తిరిగి;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    శూన్య deadCheckTasks() {
        కొరకు (Ast_కణుపు_FTask* taskp : m_tasksp) {
            ఒకవేళ (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        కొరకు (AstNew* newp : m_virtualNewsp) {
            ఒకవేళ (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_కణుపు::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    శూన్య virtualRecurse(Ast_కణుపు_FTask* nodep, స్థిరరాశి AstClass* classp, తార్కిక& isVirtualr) {
        స్థిర std::unordered_map<std::pair<స్థిరరాశి AstClass*, స్థిరరాశి Ast_కణుపు_FTask*>, తార్కిక, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        స్వయంచాలక pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        ఒకవేళ (!pair.రెండవ) {
            ఒకవేళ (pair.మొదటి->రెండవ) isVirtualr = నిజం;
            తిరిగి;
        }
        //
        ఒకవేళ (nodep->isVirtual()) isVirtualr = నిజం;
        // Propagate value from extends/implements
        కొరకు (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            స్థిరరాశి AstClass* స్థిరరాశి eclassp = extendsp->classp();
            ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి fbasep
                = VN_CAST(m_memberMap.కనుగొను_Member(eclassp, nodep->name()), కణుపు_FTask)) {
                ఒకవేళ (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    కొనసాగు;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        ఒకవేళ (isVirtualr) {
            nodep->isVirtual(నిజం);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(nodep, Class)) {
            కొరకు (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                స్థిరరాశి AstClass* స్థిరరాశి ebasep = extendsp->classp();
                ఒకవేళ (ebasep->పునాది_Override().isFinal()) {
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
    శూన్య సందర్శించు(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        ఒకవేళ (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->is_Combo_నక్షత్రం()) {
            స్థిరరాశి తార్కిక noReads = nodep->forall(
                [&](స్థిరరాశి Ast_కణుపు_Var_Ref* refp) { తిరిగి !refp->access().isReadOrRW(); });
            ఒకవేళ (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warn_మరిన్ని()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->has_అంచు();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        ఒకవేళ (!nodep->isTimingControl()) m_taskRefWarn = అబద్ధం;
        iterateChildren(nodep);
        editDType(nodep);
    }
    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override {
        ఒకవేళ (nodep->sensesp() && nodep->sensesp()->is_Combo_నక్షత్రం()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    శూన్య సందర్శించు(Ast_Attr_యొక్క* nodep) override {
        ఎంపిక (nodep->attrType()) {
        స్థితి VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        స్థితి VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        అప్రమేయ:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    శూన్య సందర్శించు(AstClassExtends* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        ఒకవేళ (nodep->argsp()) నెట్టు_Deletep(nodep->argsp()->unlink_Fr_వెనుక_With_తదుపరి());
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        ఒకవేళ (AstConst* స్థిరరాశి newp = V3_వెడల్పు_Commit::new_If_Const_Commit_కొలత(nodep)) {
            nodep->replaceWith(newp);
            Ast_కణుపు* స్థిరరాశి oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(నెట్టు_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    శూన్య సందర్శించు(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlink_Fr_వెనుక());
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(m_modp, Class);
            ఒకవేళ (nodep->is_Kwd_శుద్ధ()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        సందర్శించు_Iterate_కణుపు_DType(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        సందర్శించు_Iterate_కణుపు_DType(nodep);
    }
    శూన్య సందర్శించు(AstParamTypeDType* nodep) override {
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        సందర్శించు_Iterate_కణుపు_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlink_Fr_వెనుక();  // Make non-child
        v3Global.rootp()->typeTablep()->కూడు_Typesp(nodep);
    }
    శూన్య సందర్శించు(AstRefDType* nodep) override {
        సందర్శించు_Iterate_కణుపు_DType(nodep);
        ఒకవేళ (!nodep->typedefp()) తిరిగి;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(శూన్యసూచిక);  // No longer needed
    }
    శూన్య సందర్శించు_Iterate_కణుపు_DType(Ast_కణుపు_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        ఒకవేళ (nodep->user1_సమితి_Once()) తిరిగి;  // Process once
        nodep->వెడల్పు_Min_From_వెడల్పు();
        // Too late to any unspecified sign to be anything but unsigned
        ఒకవేళ (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        ఒకవేళ (!nodep->task_ప్రజా() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.నెట్టు_వెనుక(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        తార్కిక isVirtual = అబద్ధం;
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        ఒకవేళ (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(m_modp, Class);
            ఒకవేళ (nodep->class_పద్ధతి() && nodep->శుద్ధ_Virtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        తార్కిక extended = అబద్ధం;
        ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            కొరకు (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                స్థిరరాశి AstClass* స్థిరరాశి eclassp = extendsp->classp();
                ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి fbasep
                    = VN_CAST(m_memberMap.కనుగొను_Member(eclassp, nodep->name()), కణుపు_FTask)) {
                    ఒకవేళ (fbasep != nodep) {
                        extended = నిజం;
                        ఒకవేళ (nodep->పునాది_Override().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        ఒకవేళ (fbasep->పునాది_Override().isFinal()) {
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
        ఒకవేళ (!extended && nodep->పునాది_Override().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        ఒకవేళ (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        ఒకవేళ (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    శూన్య సందర్శించు(Ast_కేటాయించు* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        ఒకవేళ (m_underAlwaysEdged) {
            స్థిరరాశి తార్కిక ignore = nodep->lhsp()->forall([&](స్థిరరాశి AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                ఒకవేళ (refp->access().isReadOnly()) తిరిగి నిజం;
                స్థిరరాశి AstVar* స్థిరరాశి varp = refp->varp();
                // Ignore ...
                తిరిగి varp->is_వాడిన_Loop_Idx()  // ... loop indices
                       || varp->is_తాత్కాలిక()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3_Error_సంకేతం::BLKSEQ);  // ... user said so
            });
            ఒకవేళ (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warn_మరిన్ని()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Cont* nodep) override {
        iterate_And_తదుపరి_Null(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = నిజం;
            iterate_And_తదుపరి_Null(nodep->lhsp());
            iterate_And_తదుపరి_Null(nodep->rhsp());
        }
        editDType(nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_Dly* nodep) override {
        iterate_And_తదుపరి_Null(nodep->timingControlp());
        iterate_And_తదుపరి_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = అబద్ధం;
            iterate_And_తదుపరి_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {
        iterate_And_తదుపరి_Null(nodep->timingControlp());
        iterate_And_తదుపరి_Null(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = అబద్ధం;
            iterate_And_తదుపరి_Null(nodep->lhsp());
        }
        editDType(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        ఒకవేళ (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
            && m_ftaskp->verilogFunction() && m_taskRefWarn) {
            nodep->v3warn(FUNCTIMECTL,
                          "Functions cannot invoke tasks (IEEE 1800-2023 13.4)\n"
                              << nodep->warnContextPrimary() << "\n"
                              << nodep->warn_మరిన్ని() << "... Suggest make caller 'function "
                              << m_ftaskp->prettyName() << "' a task\n"
                              << m_ftaskp->warnContextSecondary() << "\n"
                              << nodep->warn_మరిన్ని() << "... Or, suggest make called 'task "
                              << nodep->taskp()->prettyName() << "' a function void\n"
                              << nodep->taskp()->warnContextSecondary());
        }
        ఒకవేళ (nodep->taskp()) nodep->taskp()->user2(1);
        ఒకవేళ (AstNew* స్థిరరాశి newp = VN_CAST(nodep, New)) {
            ఒకవేళ (!VN_IS(newp->backp(), కేటాయించు)) తిరిగి;
            ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(newp->classOrPackagep(), Class)) {
                ఒకవేళ (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.నెట్టు_వెనుక(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = నిజం;
            iterateChildren(nodep);
        }
        editDType(nodep);
        ఒకవేళ (AstClassRefDType* స్థిరరాశి classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        ఒకవేళ (nodep->isFuncLocal() && nodep->direction() == VDirection::ఆదానం && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            నెట్టు_Deletep(nodep->valuep()->unlink_Fr_వెనుక());
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        ఒకవేళ (nodep->పద్ధతి() == VCMethod::ARRAY_AT || nodep->పద్ధతి() == VCMethod::ARRAY_AT_WRITE
            || nodep->పద్ధతి() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = నిజం;
        iterateChildren(nodep);
        editDType(nodep);
    }
    శూన్య సందర్శించు(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = నిజం;
        iterateChildren(nodep);
        editDType(nodep);
    }
    శూన్య సందర్శించు(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = నిజం;
        iterateChildren(nodep);
        editDType(nodep);
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    శూన్య సందర్శించు(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->పునాది_Fromp(/* overMembers: */ అబద్ధం));
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = నిజం;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన వెడల్పు_Commit_Visitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            స్థిరరాశి VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_కణుపు_Module* modp) {
                ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } లేకపోతే ఒకవేళ (AstIface* స్థిరరాశి ifacep = VN_CAST(modp, Iface)) {
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
    ~వెడల్పు_Commit_Visitor() override = అప్రమేయ;
};

//######################################################################
// V3WidthCommit class functions

శూన్య V3_వెడల్పు_Commit::వెడల్పు_Commit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { వెడల్పు_Commit_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("widthcommit", 0, dump_వృక్షం_Either_Level() >= 6);
}
