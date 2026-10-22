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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3WidthCommit.h"

#അന്തര്ഭാവ "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Hash a std::pair automatically
ഘടന PairHash final {
    template <typename T, typename U>
    std::size_t operator()(സ്ഥിരം std::pair<T, U>& x) സ്ഥിരം {
        തിരികെ std::hash<T>()(x.first) + std::hash<U>()(x.second);
    }
};

//######################################################################
// Now that all widthing is complete,
// Copy all width() to widthMin().  V3Const expects this

വർഗ്ഗം WidthCommitVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  AstVar::user1p           -> bool.  Processed
    //  AstNodeFTask::user2()    -> uint64_t. Non-zero if ever referenced (called)
    //  AstNew::user2()          -> uint64_t. Count of number of references, minus references in
    //  functions never called
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    സ്ഥിരം അക്ഷരം* m_contNbap = nullptr;  // In continuous- or non-blocking assignment
    തർക്കം m_contReads = അസത്യം;  // Check read continuous automatic variables
    തർക്കം m_dynsizedelem = അസത്യം;  // Writing dynamically-sized array element, not the array itself
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    തർക്കം m_taskRefWarn = സത്യം;  // Allow task reference warnings
    തർക്കം m_underSel = അസത്യം;  // Under AstMemberSel or AstSel
    തർക്കം m_underAlwaysEdged = അസത്യം;  // Under always with sequential SenTree
    std::vector<AstNew*> m_virtualNewsp;  // Instantiations of virtual classes
    std::vector<Ast_കണ്ണി_FTask*> m_tasksp;  // All the tasks, we will check if they are ever called

പൊതു:
    // METHODS
    VL_DEFINE_DEBUG_FUNCTIONS;

സ്വകാര്യ:
    // METHODS
    ശൂന്യം editDType(Ast_കണ്ണി* nodep) {
        // Called by every visitor. Edit dtypes for this node, also check for some warnings
        nodep->dtypep(editOneDType(nodep->dtypep()));
        എങ്കിൽ (m_ftaskp && m_ftaskp->verilogFunction() && m_taskRefWarn && nodep->isTimingControl())
            nodep->v3warn(
                FUNCTIMECTL,
                "Functions cannot contain time-controlling statements (IEEE 1800-2023 13.4)\n"
                    << nodep->warnContextPrimary() << "\n"
                    << nodep->warnMore() << "... Suggest make caller 'function "
                    << m_ftaskp->prettyName() << "' a task\n"
                    << m_ftaskp->warnContextSecondary());
    }
    Ast_കണ്ണി_DType* editOneDType(Ast_കണ്ണി_DType* nodep) {
        // See if the dtype/refDType can be converted to a standard one
        // This reduces the number of dtypes in the system, and since
        // dtypep() figures into sameTree() results in better optimizations
        എങ്കിൽ (!nodep) തിരികെ nullptr;
        // Recurse to handle the data type, as may change the size etc of this type
        എങ്കിൽ (!nodep->user1()) iterate(nodep);
        // Look for duplicate
        എങ്കിൽ (AstBasicDType* സ്ഥിരം bdtypep = VN_CAST(nodep, BasicDType)) {
            AstBasicDType* സ്ഥിരം newp = nodep->find_തിരുകുക_Same_DType(bdtypep);
            എങ്കിൽ (newp != bdtypep && debug() >= 9) {
                UINFO_PREFIX("dtype replacement ");
                nodep->dumpSmall(std::cout);
                std::cout << "  ---->  ";
                newp->dumpSmall(std::cout);
                std::cout << '\n';
            }
            തിരികെ newp;
        }
        തിരികെ nodep;
    }
    ശൂന്യം classEncapCheck(Ast_കണ്ണി* nodep, Ast_കണ്ണി* defp, AstClass* defClassp) {
        // Check local/protected status and complain
        തർക്കം local = അസത്യം;
        തർക്കം prot = അസത്യം;
        എങ്കിൽ (!defp) {
            // rand_mode() / constraint_mode() handled in V3Randomize
            UASSERT_OBJ(nodep->name() == "rand_mode" || nodep->name() == "constraint_mode", nodep,
                        "Only rand_mode() and constraint_mode() can have no def");
            തിരികെ;
        }
        എങ്കിൽ (സ്ഥിരം auto anodep = VN_CAST(defp, Var)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം auto anodep = VN_CAST(defp, കണ്ണി_FTask)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം auto anodep = VN_CAST(defp, Typedef)) {
            local = anodep->isHideLocal();
            prot = anodep->isHideProtected();
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("ref to unhandled definition type " << defp->prettyTypeName());
        }
        എങ്കിൽ (local || prot) {
            // In case of covergroup, the reference is to the enclosing class, not the covergroup
            // itself
            സ്ഥിരം AstClass* refClassp = VN_CAST(m_modp, Class);
            എങ്കിൽ (refClassp && refClassp->isCovergroup())
                refClassp = refClassp->covergroupEnclosingClassp();
            സ്ഥിരം അക്ഷരം* how = nullptr;
            // Inner nested classes can access `local` or `protected` members of their outer class
            സ്ഥിരം auto nestedAccess = [refClassp](സ്ഥിരം AstClass*, സ്ഥിരം Ast_കണ്ണി* memberp) {
                തിരികെ memberp == refClassp;
            };
            എങ്കിൽ (local && defClassp
                && ((refClassp != defClassp) && !(defClassp->existsMember(nestedAccess)))) {
                how = "'local'";
            } അല്ലെങ്കിൽ എങ്കിൽ (prot && defClassp && !AstClass::isClassExtendedFrom(refClassp, defClassp)
                       && !(defClassp->existsMember(nestedAccess))) {
                how = "'protected'";
            }
            എങ്കിൽ (how) {
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
    ശൂന്യം varLifetimeCheck(Ast_കണ്ണി* nodep, AstVar* varp) {
        // Skip if we are under a member select (lhs of a dot)
        // We don't care about lifetime of anything else than rhs of a dot
        എങ്കിൽ (!m_underSel && m_contNbap) {
            std::string varType;
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം varDtp = varp->dtypep()->skipRefp();
            എങ്കിൽ (varp->lifetime().isAutomatic() && !VN_IS(varDtp, IfaceRefDType)
                && !(varp->isFuncLocal() && varp->isIO()))
                varType = "Automatic lifetime";
            അല്ലെങ്കിൽ എങ്കിൽ (varp->isClassMember() && !varp->lifetime().isStatic()
                     && !VN_IS(varDtp, IfaceRefDType))
                varType = "Class non-static";
            അല്ലെങ്കിൽ എങ്കിൽ (varDtp->isDynamicallySized() && m_dynsizedelem)
                varType = "Dynamically-sized";
            എങ്കിൽ (!varType.empty()) {
                UINFO(1, "    Related var dtype: " << varDtp);
                nodep->v3error(varType
                               << " variable not allowed in " << m_contNbap
                               << " assignment (IEEE 1800-2023 6.21): " << varp->prettyNameQ());
            }
        }
    }
    // Like varLifetimeCheck, for a member or element, which can be a dynamically-sized array too,
    // named by the given node
    ശൂന്യം selLifetimeCheck(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, സ്ഥിരം Ast_കണ്ണി* namep) {
        എങ്കിൽ (m_underSel || !m_contNbap || !m_dynsizedelem) തിരികെ;
        എങ്കിൽ (!nodep->isLValue() && !m_contReads) തിരികെ;
        എങ്കിൽ (!nodep->dtypep()->skipRefp()->isDynamicallySized()) തിരികെ;
        nodep->v3error("Dynamically-sized variable not allowed in "
                       << m_contNbap
                       << " assignment (IEEE 1800-2023 6.21): " << namep->prettyNameQ());
    }

    ശൂന്യം deadCheckTasks() {
        വേണ്ടി (Ast_കണ്ണി_FTask* taskp : m_tasksp) {
            എങ്കിൽ (!taskp->user2()) {
                taskp->foreach([](AstNew* newp) { newp->user2Inc(-1); });
            }
        }
        വേണ്ടി (AstNew* newp : m_virtualNewsp) {
            എങ്കിൽ (newp->user2() > 0)
                newp->v3error("Illegal to call 'new' using an abstract virtual class "
                              + Ast_കണ്ണി::prettyNameQ(newp->classOrPackagep()->origName())
                              + " (IEEE 1800-2023 8.21)");
        }
    }

    ശൂന്യം virtualRecurse(Ast_കണ്ണി_FTask* nodep, സ്ഥിരം AstClass* classp, തർക്കം& isVirtualr) {
        സ്ഥിതം std::unordered_map<std::pair<സ്ഥിരം AstClass*, സ്ഥിരം Ast_കണ്ണി_FTask*>, തർക്കം, PairHash>
            s_classFuncsChecked;  // Track what was recursed to avoid O(class*funcs^2)
        // IEEE doesn't require virtual marking at derived classes' functions.
        // Propagate virtual marking from base class function upwards.
        auto pair = s_classFuncsChecked.emplace(std::make_pair(classp, nodep), isVirtualr);
        എങ്കിൽ (!pair.second) {
            എങ്കിൽ (pair.first->second) isVirtualr = സത്യം;
            തിരികെ;
        }
        //
        എങ്കിൽ (nodep->isVirtual()) isVirtualr = സത്യം;
        // Propagate value from extends/implements
        വേണ്ടി (AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            സ്ഥിരം AstClass* സ്ഥിരം eclassp = extendsp->classp();
            എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം fbasep
                = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), കണ്ണി_FTask)) {
                എങ്കിൽ (fbasep != nodep) {
                    virtualRecurse(fbasep, eclassp, isVirtualr);
                    തുടരുക;
                }
            }
            virtualRecurse(nodep, eclassp, isVirtualr);
        }
        എങ്കിൽ (isVirtualr) {
            nodep->isVirtual(സത്യം);
            // Update memoize map in case had a false when first created
            s_classFuncsChecked[std::make_pair(classp, nodep)] = isVirtualr;
        }
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(nodep, Class)) {
            വേണ്ടി (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                സ്ഥിരം AstClass* സ്ഥിരം ebasep = extendsp->classp();
                എങ്കിൽ (ebasep->baseOverride().isFinal()) {
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
    ശൂന്യം visit(AstAlways* nodep) override {
        // As have not optimized SenTrees yet, an 'always .*' will be on first and only SenItem
        എങ്കിൽ (nodep->sentreep() && nodep->sentreep()->sensesp()
            && nodep->sentreep()->sensesp()->isComboStar()) {
            സ്ഥിരം തർക്കം noReads = nodep->forall(
                [&](സ്ഥിരം Ast_കണ്ണി_Var_Ref* refp) { തിരികെ !refp->access().isReadOrRW(); });
            എങ്കിൽ (noReads) {
                nodep->v3warn(ALWNEVER, "'always @*' will never execute as expression list is "
                                        "empty (no variables read)\n"
                                            << nodep->warnMore()
                                            << "... Suggest use 'always_comb'");
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
        }
        VL_RESTORER(m_underAlwaysEdged);
        m_underAlwaysEdged
            = nodep->sentreep() && nodep->sentreep()->sensesp() && nodep->sentreep()->hasEdge();
        // Iterate will delete ComboStar sentrees, so after above
        iterateChildren(nodep);
        editDType(nodep);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        VL_RESTORER(m_taskRefWarn);
        // fork..join_any is allowed to call tasks, and UVM does this
        എങ്കിൽ (!nodep->isTimingControl()) m_taskRefWarn = അസത്യം;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override {
        എങ്കിൽ (nodep->sensesp() && nodep->sensesp()->isComboStar()) {
            UASSERT_OBJ(!nodep->sensesp()->nextp(), nodep, "Shouldn't be senitems after .*");
            // Make look like standalone always
            // (Rest of code assumed this before .* existed)
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ശൂന്യം visit(AstAttrOf* nodep) override {
        തിരഞ്ഞെടുപ്പ് (nodep->attrType()) {
        അവസ്ഥ VAttrType::FUNC_ARG_PROTO:  // FALLTHRU
        അവസ്ഥ VAttrType::FUNC_RETURN_PROTO:
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        സ്ഥിരസ്ഥിതി:;
        }
        iterateChildren(nodep);
        editDType(nodep);
    }
    ശൂന്യം visit(AstClassExtends* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        // Extend arguments were converted to super.new arguments in V3LinkDot
        എങ്കിൽ (nodep->argsp()) തള്ളുക_Deletep(nodep->argsp()->unlinkFrBackWithNext());
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstConst* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype");
        iterate(nodep->dtypep());  // Do datatype first
        എങ്കിൽ (AstConst* സ്ഥിരം newp = V3WidthCommit::newIfConstCommitSize(nodep)) {
            nodep->replaceWith(newp);
            Ast_കണ്ണി* സ്ഥിരം oldp = nodep;
            nodep = newp;
            // UINFOTREE(5, oldp, "", "fixConstSize_old");
            // UINFOTREE(5, newp, "", "_new");
            VL_DO_DANGLING(തള്ളുക_Deletep(oldp), oldp);
        }
        editDType(nodep);
    }
    ശൂന്യം visit(AstCastWrap* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        UINFO(6, " Replace " << nodep << " w/ " << nodep->lhsp());
        nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstConstraint* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        {
            സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(m_modp, Class);
            എങ്കിൽ (nodep->isKwdPure()
                && (!classp || (!classp->isInterfaceClass() && !classp->isVirtual()))) {
                nodep->v3error("Illegal to have 'pure constraint' in non-abstract class"
                               " (IEEE 1800-2023 18.5.2)");
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_DType* nodep) override {
        // Note some specific dtypes have unique visitors
        visit_Iterate_കണ്ണി_DType(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        visit_Iterate_കണ്ണി_DType(nodep);
    }
    ശൂന്യം visit(AstParamTypeDType* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        visit_Iterate_കണ്ണി_DType(nodep);
        // Move to type table as all dtype pointers must resolve there
        nodep->unlinkFrBack();  // Make non-child
        v3Global.rootp()->typeTablep()->addTypesp(nodep);
    }
    ശൂന്യം visit(AstRefDType* nodep) override {
        visit_Iterate_കണ്ണി_DType(nodep);
        എങ്കിൽ (!nodep->typedefp()) തിരികെ;  // Already checked and cleared
        classEncapCheck(nodep, nodep->typedefp(), VN_CAST(nodep->classOrPackagep(), Class));
        nodep->typedefp(nullptr);  // No longer needed
    }
    ശൂന്യം visit_Iterate_കണ്ണി_DType(Ast_കണ്ണി_DType* nodep) {
        // Rather than use dtypeChg which may make new nodes, we edit in place,
        // as we don't need to preserve any widthMin's, and every dtype with the same width
        // gets an identical edit.
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process once
        nodep->widthMinFromWidth();
        // Too late to any unspecified sign to be anything but unsigned
        എങ്കിൽ (nodep->numeric().isNosign()) nodep->numeric(VSigning::UNSIGNED);
        iterateChildren(nodep);
        nodep->virtRefDTypep(editOneDType(nodep->virtRefDTypep()));
        nodep->virtRefDType2p(editOneDType(nodep->virtRefDType2p()));
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        എങ്കിൽ (!nodep->taskPublic() && !nodep->dpiExport() && !nodep->dpiImport())
            m_tasksp.തള്ളുക_back(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
        editDType(nodep);
        തർക്കം isVirtual = അസത്യം;
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(m_modp, Class)) {
            virtualRecurse(nodep, classp, isVirtual /*ref*/);
        }
        എങ്കിൽ (nodep->isStatic() && nodep->isVirtual())  // After propagated isVirtual
            nodep->v3error("Static methods cannot be virtual");
        {
            സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(m_modp, Class);
            എങ്കിൽ (nodep->classMethod() && nodep->pureVirtual() && classp
                && !classp->isInterfaceClass() && !classp->isVirtual()) {
                nodep->v3error(
                    "Illegal to have 'pure virtual' in non-virtual class (IEEE 1800-2023 8.21)");
            }
        }
        തർക്കം extended = അസത്യം;
        എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(m_modp, Class)) {
            // Walk down inheritance
            വേണ്ടി (AstClassExtends* extendsp = classp->extendsp(); extendsp;
                 extendsp = extendsp->classp()->extendsp()) {
                സ്ഥിരം AstClass* സ്ഥിരം eclassp = extendsp->classp();
                എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം fbasep
                    = VN_CAST(m_memberMap.findMember(eclassp, nodep->name()), കണ്ണി_FTask)) {
                    എങ്കിൽ (fbasep != nodep) {
                        extended = സത്യം;
                        എങ്കിൽ (nodep->baseOverride().isInitial()) {
                            nodep->v3error("Member "
                                           << nodep->prettyNameQ()
                                           << " is marked ':initial' but is being extended"
                                              " (IEEE 1800-2023 8.20)\n"
                                           << nodep->warnContextPrimary() << "\n"
                                           << fbasep->warnOther()
                                           << "... Location of declaration being extended\n"
                                           << fbasep->warnContextSecondary());
                        }
                        എങ്കിൽ (fbasep->baseOverride().isFinal()) {
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
        എങ്കിൽ (!extended && nodep->baseOverride().isExtends()) {
            nodep->v3error("Member " << nodep->prettyNameQ()
                                     << " marked ':extends' but no base class function is"
                                        " being extend (IEEE 1800-2023 8.20)");
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->varp(), VN_CAST(nodep->classOrPackagep(), Class));
        എങ്കിൽ (nodep->access().isWriteOrRW() || m_contReads) varLifetimeCheck(nodep, nodep->varp());
        എങ്കിൽ (VN_IS(nodep, VarRef))
            nodep->name("");  // Clear to save memory; nodep->name() will work via nodep->varp()
    }
    ശൂന്യം visit(AstAssign* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        // Lint
        എങ്കിൽ (m_underAlwaysEdged) {
            സ്ഥിരം തർക്കം ignore = nodep->lhsp()->forall([&](സ്ഥിരം AstVarRef* refp) {
                // Ignore reads (e.g.: index expressions)
                എങ്കിൽ (refp->access().isReadOnly()) തിരികെ സത്യം;
                സ്ഥിരം AstVar* സ്ഥിരം varp = refp->varp();
                // Ignore ...
                തിരികെ varp->isUsedLoopIdx()  // ... loop indices
                       || varp->isTemp()  // ... temporaries
                       || varp->fileline()->warnIsOff(V3ErrorCode::BLKSEQ);  // ... user said so
            });
            എങ്കിൽ (!ignore) {
                nodep->v3warn(BLKSEQ,
                              "Blocking assignment '=' in sequential logic process\n"
                                  << nodep->warnMore()  //
                                  << "... Suggest using delayed assignment '<='");
            }
        }
    }
    ശൂന്യം visit(AstAssignCont* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = സത്യം;
            iterateAndNextNull(nodep->lhsp());
            iterateAndNextNull(nodep->rhsp());
        }
        editDType(nodep);
    }
    ശൂന്യം visit(AstAssignDly* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "nonblocking";
            m_contReads = അസത്യം;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    ശൂന്യം visit(AstAssignW* nodep) override {
        iterateAndNextNull(nodep->timingControlp());
        iterateAndNextNull(nodep->rhsp());
        {
            VL_RESTORER(m_contNbap);
            VL_RESTORER(m_contReads);
            m_contNbap = "continuous";
            m_contReads = അസത്യം;
            iterateAndNextNull(nodep->lhsp());
        }
        editDType(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        classEncapCheck(nodep, nodep->taskp(), VN_CAST(nodep->classOrPackagep(), Class));
        എങ്കിൽ (nodep->taskp() && nodep->taskp()->verilogTask() && m_ftaskp
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
        എങ്കിൽ (nodep->taskp()) nodep->taskp()->user2(1);
        എങ്കിൽ (AstNew* സ്ഥിരം newp = VN_CAST(nodep, New)) {
            എങ്കിൽ (!VN_IS(newp->backp(), Assign)) തിരികെ;
            എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(newp->classOrPackagep(), Class)) {
                എങ്കിൽ (classp->isVirtual() || classp->isInterfaceClass()) {
                    m_virtualNewsp.തള്ളുക_back(newp);
                    newp->user2Inc();
                }
            }
        }
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = സത്യം;
            iterateChildren(nodep);
        }
        editDType(nodep);
        എങ്കിൽ (AstClassRefDType* സ്ഥിരം classrefp = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            classEncapCheck(nodep, nodep->varp(), classrefp->classp());
        }  // else might be struct, etc
        varLifetimeCheck(nodep, nodep->varp());
    }
    ശൂന്യം visit(AstVar* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        എങ്കിൽ (nodep->isFuncLocal() && nodep->direction() == VDirection::നിവേശം && nodep->valuep()) {
            // It's the default value of optional argument.
            // Such values are added to function calls in V3Width so can be removed now
            തള്ളുക_Deletep(nodep->valuep()->unlinkFrBack());
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Pre_Sel* nodep) override {  // LCOV_EXCL_LINE
        // This check could go anywhere after V3Param
        nodep->v3fatalSrc("Presels should have been removed before this point");
    }
    ശൂന്യം visit(AstCMethodHard* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        എങ്കിൽ (nodep->method() == VCMethod::ARRAY_AT || nodep->method() == VCMethod::ARRAY_AT_WRITE
            || nodep->method() == VCMethod::DYN_AT_WRITE_APPEND)
            m_dynsizedelem = സത്യം;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ശൂന്യം visit(AstAssocSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = സത്യം;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ശൂന്യം visit(AstWildcardSel* nodep) override {
        VL_RESTORER(m_dynsizedelem);
        m_dynsizedelem = സത്യം;
        iterateChildren(nodep);
        editDType(nodep);
    }
    ശൂന്യം visit(AstStructSel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep);
    }
    ശൂന്യം visit(AstArraySel* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
        selLifetimeCheck(nodep, nodep->baseFromp(/* overMembers: */ അസത്യം));
    }
    ശൂന്യം visit(AstSel* nodep) override {
        {
            VL_RESTORER(m_underSel);
            m_underSel = സത്യം;
            iterateChildren(nodep);
        }
        editDType(nodep);
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {
        // Reference must have been resolved, can delete these
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        iterateChildren(nodep);
        editDType(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit WidthCommitVisitor(AstNetlist* nodep) {
        {
            // Name classes and interfaces while they have their parameter types, as this visit
            // moves those to the type table, maybe from another module's types before their own,
            // with the scopes declaring them indexed, as a class may be named with others
            സ്ഥിരം VDTypeNameScopes scopes{nodep};
            nodep->foreach([](Ast_കണ്ണി_Module* modp) {
                എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(modp, Class)) {
                    classp->dtypeNameFreeze();
                } അല്ലെങ്കിൽ എങ്കിൽ (AstIface* സ്ഥിരം ifacep = VN_CAST(modp, Iface)) {
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
    ~WidthCommitVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// V3WidthCommit class functions

ശൂന്യം V3WidthCommit::widthCommit(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { WidthCommitVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("widthcommit", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
