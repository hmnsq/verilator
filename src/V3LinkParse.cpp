// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Parse module/signal name references
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
// LinkParse TRANSFORMATIONS:
//      Top-down traversal
//          Move some attributes around
//*************************************************************************

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3LinkParse.h"

#অন্তর্ভাৱ "V3Control.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

শ্ৰেণী LinkParseVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ধ্রুৱ VNUser1InUse m_inuser1;
    ধ্রুৱ VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<File_সাৰি*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    Ast_গ্ৰন্থি_Module* m_মান_Modp = nullptr;
    Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    তার্কিক m_inInterface = মিছা;  // True when inside interface declaration
    Ast_গ্ৰন্থি_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_গ্ৰন্থি_FTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    Ast_গ্ৰন্থি_Block* m_blockp = nullptr;  // Current AstNodeBlock
    Ast_গ্ৰন্থি_Stmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    Ast_গ্ৰন্থি_Stmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    Ast_গ্ৰন্থি_DType* m_dtypep = nullptr;  // Current data type
    Ast_গ্ৰন্থি_ৰাশিমালা* m_defaultInSkewp = nullptr;  // Current default input skew
    Ast_গ্ৰন্থি_ৰাশিমালা* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, ধ্রুৱ AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    পূর্ণাঙ্ক m_anonUdpId = 0;  // Counter for anonymous UDP instances
    পূর্ণাঙ্ক m_coverpoint_অঙ্ক = 0;  // Counter for unnamed coverpoints within current covergroup
    পূর্ণাঙ্ক m_genblkAbove = 0;  // Begin block number of if/case/for above
    পূর্ণাঙ্ক m_genblk_অঙ্ক = 0;  // Begin block number, 0=none seen
    পূর্ণাঙ্ক m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    পূর্ণাঙ্ক m_rand_Sequence_অঙ্ক = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    তার্কিক m_lifetimeAllowed = মিছা;  // True to allow lifetime settings
    তার্কিক m_hasTimingControl = মিছা;  // If current task has timing control
    তার্কিক m_moduleWithGenericIface = মিছা;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    শূন্য cleanFileline(Ast_গ্ৰন্থি* nodep) {
        যদি (nodep->user2SetOnce()) ঘূৰাই;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        যদি (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(নূতন File_সাৰি{nodep->fileline()});
        }
        m_filelines.ঢোকাও(nodep->fileline());
    }

    string nameFromTypedef(Ast_গ্ৰন্থি* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        যদি (ধ্রুৱ AstTypedef* ধ্রুৱ typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            যদি (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } অন্যথা যদি (m_modp) {
                above = m_modp->name() + ".";
            }
            ঘূৰাই above + typedefp->name();
        }
        ঘূৰাই "";
    }

    শূন্য visit_Iterate_গ্ৰন্থি_DType(Ast_গ্ৰন্থি_DType* nodep) {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    তার্কিক nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        //    1: GENIF
        // -> 1:3: GENBLOCK [IMPLIED]  // nodep passed to this function
        //    1:3:1: GENIF
        //    1:3:1:2: GENBLOCK genblk1 [IMPLIED]
        // Likewise for a generate case item holding only a generate if
        //    1: GENCASEITEM
        // -> 1:2: GENBLOCK [IMPLIED]  // nodep passed to this function
        //    1:2:1: GENIF
        ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ backp = nodep->backp();
        ঘূৰাই (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    শূন্য checkIndent(Ast_গ্ৰন্থি* nodep, Ast_গ্ৰন্থি* childp) {
        // Try very hard to avoid false positives
        Ast_গ্ৰন্থি* nextp = nodep->nextp();
        যদি (!childp) ঘূৰাই;
        যদি (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        যদি (!nextp) ঘূৰাই;
        যদি (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) ঘূৰাই;
        File_সাৰি* ধ্রুৱ গ্ৰন্থি_Flp = nodep->fileline();
        File_সাৰি* ধ্রুৱ childFlp = childp->fileline();
        File_সাৰি* ধ্রুৱ nextFlp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        যদি (গ্ৰন্থি_Flp->filenameno() != childFlp->filenameno()) ঘূৰাই;
        যদি (গ্ৰন্থি_Flp->filenameno() != nextFlp->filenameno()) ঘূৰাই;
        যদি (গ্ৰন্থি_Flp->lastLineno() >= childFlp->firstLineno()) ঘূৰাই;
        যদি (childFlp->lastLineno() >= nextFlp->firstLineno()) ঘূৰাই;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        যদি (!(গ্ৰন্থি_Flp->firstColumn() < childFlp->firstColumn()
              && nextFlp->firstColumn() >= childFlp->firstColumn()))
            ঘূৰাই;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ধ্রুৱ std::string গ্ৰন্থি_Prefix = গ্ৰন্থি_Flp->sourcePrefix(গ্ৰন্থি_Flp->firstColumn());
        ধ্রুৱ std::string childPrefix = childFlp->sourcePrefix(গ্ৰন্থি_Flp->firstColumn());
        ধ্রুৱ std::string nextPrefix = nextFlp->sourcePrefix(গ্ৰন্থি_Flp->firstColumn());
        যদি (childPrefix != গ্ৰন্থি_Prefix) ঘূৰাই;
        যদি (nextPrefix != childPrefix) ঘূৰাই;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ধ্রুৱ std::string গ্ৰন্থি_Source = গ্ৰন্থি_Flp->source();
        ধ্রুৱ std::string childSource = childFlp->source();
        ধ্রুৱ std::string nextSource = nextFlp->source();
        যদি (!(VString::leading_Whitespace_গুনতি(গ্ৰন্থি_Source)
                  < VString::leading_Whitespace_গুনতি(childSource)
              && VString::leading_Whitespace_গুনতি(nextSource)
                     >= VString::leading_Whitespace_গুনতি(childSource)))
            ঘূৰাই;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    শূন্য collectPorts(Ast_গ্ৰন্থি* গ্ৰন্থি_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        বাবে (Ast_গ্ৰন্থি* nodep = গ্ৰন্থি_Listp; nodep; nodep = nodep->nextp()) {
            যদি (AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) {
                যদি (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        বাবে (Ast_গ্ৰন্থি* nodep = গ্ৰন্থি_Listp; nodep; nodep = nodep->nextp()) {
            যদি (AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) {
                যদি (varp->isIO()) অনুৱর্ত;
                ধ্রুৱ auto it = portNames.find(varp->name());
                যদি (it == portNames.end()) অনুৱর্ত;
                AstVar* ধ্রুৱ iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                যদি (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlinkFrBackWithNext());
                m_portDups.emplace(varp);
            }
        }
    }

    Ast_গ্ৰন্থি* getVarsAndUnlink(Ast_গ্ৰন্থি* stmtsp) {
        Ast_গ্ৰন্থি* varsp = nullptr;
        বাবে (Ast_গ্ৰন্থি *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (VN_IS(itemp, Var)) varsp = Ast_গ্ৰন্থি::addNext(varsp, itemp->unlinkFrBack());
        }
        ঘূৰাই varsp;
    }
    Ast_গ্ৰন্থি_Stmt* getBlockAdds() {
        Ast_গ্ৰন্থি_Stmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        যদি (m_blockAddStaticStmtp)
            addsp = Ast_গ্ৰন্থি::addNext(addsp,
                                     নূতন AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        যদি (m_blockAddAutomaticStmtp)
            addsp = Ast_গ্ৰন্থি::addNext(
                addsp, নূতন AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        ঘূৰাই addsp;
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_FTask* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process only once.
        // Mark class methods
        যদি (VN_IS(m_modp, Class)) nodep->classMethod(সঁচা);

        V3Control::applyFTask(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = সঁচা;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ nodep) { ঘূৰাই nodep->isTimingControl(); });
        যদি (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } অন্যথা {
            যদি (nodep->classMethod()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } অন্যথা যদি (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        যদি (nodep->classMethod() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warnMore() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_গ্ৰন্থি* stmtsp = nullptr;
        যদি (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = Ast_গ্ৰন্থি::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_গ্ৰন্থি::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    শূন্য visit(Ast_গ্ৰন্থি_DType* nodep) override { visit_Iterate_গ্ৰন্থি_DType(nodep); }
    শূন্য visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(সঁচা);
        iterateChildren(nodep);
    }
    শূন্য visit(AstConstraintForeach* nodep) override {
        যদি (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_ৰাশিমালা* exprp) { exprp->isSoft(সঁচা); });
        }
    }
    শূন্য visit(AstEnumDType* nodep) override {
        যদি (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_গ্ৰন্থি_DType(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_UOr_Struct_DType* nodep) override {
        যদি (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_গ্ৰন্থি_DType(nodep);
    }
    শূন্য visit(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        যদি (nodep->rangep()) {
            যদি (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t offset_from_init = 0;
            AstEnumItem* addp = nullptr;
            File_সাৰি* ধ্রুৱ flp = nodep->fileline();
            বাবে (ধ্রুৱ পূর্ণাঙ্ক i : nodep->rangep()->seq_বাম_To_ডান()) {
                ধ্রুৱ string name = nodep->name() + cvtToStr(i);
                Ast_গ্ৰন্থি_ৰাশিমালা* valuep = nullptr;
                যদি (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = নূতন AstAdd{flp, nodep->valuep()->clone_বৃক্ষ(সঁচা),
                                     নূতন AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = Ast_গ্ৰন্থি::addNext(addp, নূতন AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }

    শূন্য visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ধ্রুৱ AstClass* ধ্রুৱ classp = VN_CAST(m_modp, Class);
        যদি (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(সঁচা);
        }
        যদি (nodep->embeddedCovergroup()) {
            AstRefDType* ধ্রুৱ refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        যদি (nodep->valuep()) nodep->hasUserInit(সঁচা);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        যদি (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            যদি (VN_IS(m_modp, Module) && m_ftaskp) {
                m_ftaskp->v3warn(
                    IMPLICITSTATIC,
                    "Function/task's lifetime implicitly set to static;"
                    " variables made static (IEEE 1800-2023 6.21)\n"
                        << m_ftaskp->warnMore() << "... Suggest use '" << m_ftaskp->verilogKwd()
                        << " automatic' or '" << m_ftaskp->verilogKwd() << " static'\n"
                        << m_ftaskp->warnContextPrimary() << '\n'
                        << nodep->warnOther() << "... Location of implicit static variable\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnContextSecondary());
            } অন্যথা {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnMore()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        যদি (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        যদি (!nodep->direction().isAny()) {  // Not a port
            যদি (nodep->lifetime().isNone()) {
                যদি (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } অন্যথা {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } অন্যথা যদি (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            যদি (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } অন্যথা {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } অন্যথা যদি (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        যদি (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            যদি (m_beginDepth > 0
                || (m_beginDepth == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        যদি (nodep->isGParam() && m_modp) m_modp->hasGParam(সঁচা);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ধ্রুৱ AstBasicDType* ধ্রুৱ basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        যদি (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(সঁচা);

        যদি (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        যদি (nodep->isParam() && m_inInterface) nodep->isIfaceParam(সঁচা);
        যদি (AstParseTypeDType* ধ্রুৱ ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_গ্ৰন্থি* dtypep = nodep->valuep();
            যদি (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                যদি (Ast_গ্ৰন্থি_ৰাশিমালা* exprp = VN_CAST(dtypep, গ্ৰন্থি_ৰাশিমালা)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    যেতিয়ালৈ (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        যদি (!rhsDotp) ভঙ্গ;
                        File_সাৰি* ধ্রুৱ fl = dotp->fileline();
                        ধ্রুৱ তার্কিক colon = dotp->colon();
                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), গ্ৰন্থি_ৰাশিমালা);
                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), গ্ৰন্থি_ৰাশিমালা);
                        Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), গ্ৰন্থি_ৰাশিমালা);
                        File_সাৰি* ধ্রুৱ rhsFl = rhsDotp->fileline();
                        ধ্রুৱ তার্কিক rhsColon = rhsDotp->colon();
                        AstDot* ধ্রুৱ newLhs = নূতন AstDot{fl, colon, lhs, rhsLhs};
                        exprp = নূতন AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_বৃক্ষ(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } অন্যথা {
                dtypep = নূতন AstVoidDType{nodep->fileline()};
            }

            Ast_গ্ৰন্থি* ধ্রুৱ newp = নূতন AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, নূতন AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        ধ্রুৱ Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(মিছা), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        যদি (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            যদি (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(সঁচা);
            } অন্যথা যদি (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(সঁচা);
            } অন্যথা যদি (m_modp && v3Global.opt.publicDepth()) {
                যদি ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(সঁচা);
                } অন্যথা যদি (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(সঁচা);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        যদি (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(মিছা);
            nodep->trace(মিছা);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        যদি (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, সঁচা);
        // Compute initial value
        যদি (nodep->valuep() || nodep->needsCReset()) {
            File_সাৰি* ধ্রুৱ fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> Ast_গ্ৰন্থি_ৰাশিমালা* {
                যদি (nodep->valuep()) {
                    ঘূৰাই VN_AS(nodep->valuep()->unlinkFrBack(), গ্ৰন্থি_ৰাশিমালা);
                } অন্যথা {
                    ঘূৰাই নূতন AstCReset{fl, nodep, মিছা};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            যদি (m_portDups.find(nodep) != m_portDups.end()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } অন্যথা যদি (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } অন্যথা যদি (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_প্ৰবেশ()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                যদি (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlinkFrBack()->delete_বৃক্ষ();
                }
            } অন্যথা যদি (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(সঁচা);
                File_সাৰি* ধ্রুৱ newfl = নূতন File_সাৰি{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, সঁচা);
                AstAssign* ধ্রুৱ assp
                    = নূতন AstAssign{newfl, নূতন AstParseRef{newfl, nodep->name()}, createValuep()};
                যদি (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = Ast_গ্ৰন্থি::addNext(m_blockAddAutomaticStmtp, assp);
                } অন্যথা {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = Ast_গ্ৰন্থি::addNext(m_blockAddStaticStmtp, assp);
                }
            } অন্যথা যদি (m_মান_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                যদি (nodep->valuep()) {
                    nodep->noCReset(সঁচা);
                    File_সাৰি* ধ্রুৱ newfl = নূতন File_সাৰি{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, সঁচা);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, সঁচা);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* ধ্রুৱ assp = নূতন AstAssign{
                        newfl, নূতন AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_গ্ৰন্থি* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    যদি (nodep->lifetime().isAutomatic()) {
                        newInitp = নূতন AstInitialAutomatic{newfl, assp};
                    } অন্যথা {
                        newInitp = নূতন AstInitialStatic{newfl, assp};
                    }
                    nodep->addNextHere(newInitp);
                }
            } অন্যথা যদি (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    স্থির তার্কিক isNegative(ধ্রুৱ AstConst* constp) {
        ঘূৰাই constp->isSigned() && constp->অঙ্ক().bitIs1(constp->width() - 1);
    }
    শূন্য visit(AstRange* nodep) override {
        স্থির constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ধ্রুৱ AstConst* ধ্রুৱ leftp = VN_CAST(nodep->leftp(), Const);
        ধ্রুৱ AstConst* ধ্রুৱ rightp = VN_CAST(nodep->rightp(), Const);
        যদি (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ধ্রুৱ uint64_t বাম = leftp->toUQuad();
            ধ্রুৱ uint64_t ডান = rightp->toUQuad();
            ধ্রুৱ uint64_t span = std::max(বাম, ডান) - std::min(বাম, ডান);
            যদি (span >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << span + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstConst* nodep) override {
        যদি (nodep->অঙ্ক().autoExtend() && nodep->fileline()->language() < V3LangCode::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    শূন্য visit(AstAttrOf* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        যদি (nodep->attrType() == VAttrType::DT_PUBLIC) {
            AstTypedef* ধ্রুৱ typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(সঁচা);
            m_varp->sigModPublic(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_SPLIT_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            যদি (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } অন্যথা {
                m_varp->attrSplitVar(সঁচা);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(সঁচা);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(সঁচা);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(সঁচা);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(সঁচা);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(সঁচা);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(সঁচা);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(সঁচা);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        }
    }

    শূন্য visit(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = nullptr;
        ধ্রুৱ ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        যদি (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } অন্যথা {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_গ্ৰন্থি* backp = nodep->backp();
            বাবে (; backp; backp = backp->backp()) {
                যদি (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, গ্ৰন্থি_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    ভঙ্গ;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_গ্ৰন্থি_DType* ধ্রুৱ dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            যদি (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                ঘূৰাই;
            } অন্যথা {
                defp = নূতন AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.size()));
                UINFO(9, "New impltypedef " << defp);
                backp->addNextHere(defp);
            }
        }
        nodep->replaceWith(নূতন AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }

    শূন্য visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি_Foreach* nodep) override {
        // FOREACH(array, loopvars, body)
        UINFO(9, "FOREACH " << nodep);
        cleanFileline(nodep);
        // Separate iteration vars from base from variable
        // Input:
        //      v--- arrayp
        //   1. DOT(DOT(first, second), ASTSELLOOPVARS(third, var0..var1))
        // Separated:
        //   bracketp = ASTSELLOOPVARS(...)
        //   arrayp = DOT(DOT(first, second), third)
        //   firstVarp = var0..var1
        // Other examples
        //   2. ASTSELBIT(first, var0))
        //   3. ASTSELLOOPVARS(first, var0..var1))
        //   4. DOT(DOT(first, second), ASTSELBIT(third, var0))
        AstForeachHeader* ধ্রুৱ headerp = nodep->headerp();
        যদি (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ঘূৰাই;
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    শূন্য visit(AstLoop* nodep) override {
        cleanFileline(nodep);
        যদি (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } অন্যথা {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        যদি (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_সাৰি* ধ্রুৱ newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, সঁচা);
            nodep->fileline(newfl);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        যদি (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_inInterface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_অঙ্ক);
        VL_RESTORER(m_beginDepth);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_অঙ্ক);
        VL_RESTORER(m_মান_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        যদি (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        যদি (VN_IS(nodep, Iface)) m_inInterface = সঁচা;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_অঙ্ক = 0;
        m_beginDepth = 0;
        m_মান_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = মিছা;
        m_rand_Sequence_অঙ্ক = 0;

        যদি (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        যদি (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        যদি (AstModule* ধ্রুৱ modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    শূন্য visit_Iterate_No_মান_Mod(Ast_গ্ৰন্থি* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_মান_Modp);
        m_মান_Modp = nullptr;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = সঁচা;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        visit_Iterate_No_মান_Mod(nodep);
    }
    শূন্য visit(AstCover* nodep) override { visit_Iterate_No_মান_Mod(nodep); }
    শূন্য visit(AstRestrict* nodep) override { visit_Iterate_No_মান_Mod(nodep); }

    শূন্য visit(AstGenBlock* nodep) override {
        V3Control::applyCoverageBlock(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_beginDepth);
        ++m_beginDepth;
        ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ধ্রুৱ তার্কিক nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        পূর্ণাঙ্ক assign_Gen_Blk_অঙ্ক = -1;
        যদি (nodep->genforp()) {
            ++m_genblk_অঙ্ক;
            যদি (nodep->name() == "") assign_Gen_Blk_অঙ্ক = m_genblk_অঙ্ক;
        } অন্যথা যদি (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            assign_Gen_Blk_অঙ্ক = m_genblkAbove;
        }
        যদি (assign_Gen_Blk_অঙ্ক != -1) {
            nodep->name("genblk" + cvtToStr(assign_Gen_Blk_অঙ্ক));
            যদি (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warnMore()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        যদি (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_অঙ্ক);
            m_genblkAbove = 0;
            m_genblk_অঙ্ক = 0;
            iterateChildren(nodep);
        } অন্যথা {
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstGenCase* nodep) override {
        ++m_genblk_অঙ্ক;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_অঙ্ক);
        m_genblkAbove = m_genblk_অঙ্ক;
        m_genblk_অঙ্ক = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ধ্রুৱ তার্কিক nestedIf = (VN_IS(nodep->backp(), GenBlock)
                               && nestedIfBegin(VN_CAST(nodep->backp(), GenBlock)));
        যদি (nestedIf) {
            iterateChildren(nodep);
        } অন্যথা {
            ++m_genblk_অঙ্ক;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_অঙ্ক);
            m_genblkAbove = m_genblk_অঙ্ক;
            m_genblk_অঙ্ক = 0;
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstCell* nodep) override {
        যদি (nodep->origName().empty()) {
            যদি (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ধ্রুৱ string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_Block* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterateAndNextNull(nodep->declsp());
        }

        যদি (AstBegin* ধ্রুৱ beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterateAndNextNull(nodep->stmtsp());
        যদি (AstFork* ধ্রুৱ forkp = VN_CAST(nodep, Fork)) iterateAndNextNull(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_গ্ৰন্থি* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = Ast_গ্ৰন্থি::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_গ্ৰন্থি::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    শূন্য visit(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        যদি (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        যদি (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(সঁচা);
            Ast_গ্ৰন্থি* ধ্রুৱ nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->addNext(nodep);
        }
    }
    শূন্য visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        যদি (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_গ্ৰন্থি* scanp = nodep;
            // Skip over the New's statement
            বাবে (; scanp && !VN_IS(scanp, Stmt_ৰাশিমালা); scanp = scanp->backp()) {}
            যদি (VN_IS(scanp, Stmt_ৰাশিমালা)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                বাবে (; scanp; scanp = scanp->backp()) {
                    যদি (VN_IS(scanp, গ্ৰন্থি_Stmt) || VN_IS(scanp, গ্ৰন্থি_Module)
                        || VN_IS(scanp, গ্ৰন্থি_FTask))
                        ভঙ্গ;
                }
                যদি (!VN_IS(scanp, গ্ৰন্থি_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    শূন্য visit(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    শূন্য visit(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    শূন্য visit(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::to_string(m_rand_Sequence_অঙ্ক++));
        iterateChildren(nodep);
    }
    শূন্য visit(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    শূন্য visit(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    শূন্য visit(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    শূন্য visit(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    শূন্য visit(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    শূন্য visit(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    শূন্য visit(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ধ্রুৱ alwaysp = VN_CAST(nodep->backp(), Always);
        যদি (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            যদি (Ast_Sen_বৃক্ষ* ধ্রুৱ sentreep = nodep->sentreep()) {
                sentreep->unlinkFrBackWithNext();
                alwaysp->sentreep(sentreep);
            }
            যদি (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        বাবে (Ast_গ্ৰন্থি *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                যদি (citemp->exprp() || citemp->assignp()) অনুৱর্ত;
                যদি (citemp->skewp()) {
                    যদি (citemp->direction() == VDirection::প্ৰবেশ) {
                        // Disallow default redefinition; note some simulators allow this
                        যদি (m_defaultInSkewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_defaultInSkewp = citemp->skewp();
                    } অন্যথা যদি (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        যদি (m_defaultOutSkewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_defaultOutSkewp = citemp->skewp();
                    } অন্যথা {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(ঠেলো_Deletep(citemp->unlinkFrBack()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        যদি (nodep->direction() == VDirection::OUTPUT) {
            যদি (!nodep->skewp()) {
                যদি (m_defaultOutSkewp) {
                    nodep->skewp(m_defaultOutSkewp->clone_বৃক্ষ(মিছা));
                } অন্যথা {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(নূতন AstConst{nodep->fileline(), 0});
                }
            }
        } অন্যথা যদি (nodep->direction() == VDirection::প্ৰবেশ) {
            যদি (!nodep->skewp()) {
                যদি (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->clone_বৃক্ষ(মিছা));
                } অন্যথা {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(নূতন AstConst{nodep->fileline(), AstConst::OneStep{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        যদি (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    শূন্য visit(Ast_Tagged_ৰাশিমালা* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstTaggedPattern* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstPatternVar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstPatternStar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        যদি (nodep->keyword() == VBasicDTypeKwd::CVOID) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: void (for tagged unions)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    // Return, for each arg in argsp, a parameter plus a "this.<member> = <param>"
    // assignment.  The parameter is a clone of the covergroup member and so shares its
    // name; 'this.' on the LHS targets the member, otherwise the same-named local parameter
    // shadows it and the assignment self-assigns the parameter, leaving the member unwritten.
    // argsp may be null (returns nullptr).
    স্থির Ast_গ্ৰন্থি* makeArgMemberCopies(Ast_গ্ৰন্থি* argsp, তার্কিক readOnlyRefs) {
        Ast_গ্ৰন্থি* resultp = nullptr;
        বাবে (Ast_গ্ৰন্থি* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ধ্রুৱ origVarp = VN_AS(argp, Var);
            AstVar* ধ্রুৱ paramp = origVarp->clone_বৃক্ষ(মিছা);
            File_সাৰি* ধ্রুৱ fl = নূতন File_সাৰি{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, সঁচা);
            paramp->fileline(fl);
            paramp->funcLocal(সঁচা);
            paramp->direction(origVarp->direction());
            যদি (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_গ্ৰন্থি::addNextNull(resultp, paramp);
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ lhsp = নূতন AstDot{
                origVarp->fileline(), মিছা, নূতন AstParseRef{origVarp->fileline(), "this"},
                নূতন AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ rhsp = নূতন AstParseRef{paramp->fileline(), paramp->name()};
            resultp->addNext(নূতন AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        ঘূৰাই resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    স্থির শূন্য createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_গ্ৰন্থি* argsp,
                                        Ast_গ্ৰন্থি* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ধ্রুৱ defaultVarp
            = নূতন AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        যদি (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, সঁচা));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ধ্রুৱ varp
                = নূতন AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             নূতন AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             নূতন AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ধ্রুৱ varp
                = নূতন AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             নূতন AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             নূতন AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ধ্রুৱ funcp = নূতন AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, মিছা));
            funcp->classMethod(সঁচা);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(সঁচা);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        বাবে (ধ্রুৱ string& name : {"start"s, "stop"s}) {
            AstFunc* ধ্রুৱ funcp = নূতন AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(সঁচা);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        বাবে (ধ্রুৱ string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ধ্রুৱ funcp = নূতন AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, সঁচা);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(সঁচা);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* ধ্রুৱ varp = নূতন AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(সঁচা);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(সঁচা);
                funcp->fvarp(varp);
            }
            বাবে (ধ্রুৱ string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ধ্রুৱ varp = নূতন AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(সঁচা);
                varp->direction(VDirection::প্ৰবেশ);
                varp->valuep(নূতন AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ধ্রুৱ funcp
                = নূতন AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(সঁচা);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* ধ্রুৱ varp = নূতন AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(সঁচা);
            varp->direction(VDirection::প্ৰবেশ);
            funcp->addStmtsp(varp);
        }
    }

    তার্কিক dropDeprecatedCoverageOption(AstCgOptionAssign* ধ্রুৱ nodep) {
        যদি (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) ঘূৰাই মিছা;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        ঘূৰাই সঁচা;
    }

    শূন্য visit(AstCgOptionAssign* nodep) override {
        যদি (dropDeprecatedCoverageOption(nodep)) ঘূৰাই;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    শূন্য visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        যদি (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) ঘূৰাই;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        যদি (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ধ্রুৱ string libname = m_modp->libname();
        AstClass* ধ্রুৱ cgClassp = নূতন AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, সঁচা);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, সঁচা);
        cgClassp->isCovergroup(সঁচা);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(সঁচা);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        যদি (Ast_Sen_বৃক্ষ* ধ্রুৱ eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* ধ্রুৱ cgNodep = নূতন AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        বাবে (Ast_গ্ৰন্থি* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ধ্রুৱ origVarp = VN_AS(argp, Var);
            যদি (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::প্ৰবেশ);
            }
            যদি ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ধ্রুৱ memberp = origVarp->clone_বৃক্ষ(মিছা);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(মিছা);
            memberp->direction(VDirection::NONE);
            যদি (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(সঁচা);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        বাবে (Ast_গ্ৰন্থি* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ধ্রুৱ origVarp = VN_AS(argp, Var);
            যদি (!origVarp->is_প্ৰবেশ()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::প্ৰবেশ);
            }
            AstVar* ধ্রুৱ memberp = origVarp->clone_বৃক্ষ(মিছা);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(মিছা);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ধ্রুৱ newFuncp = নূতন AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, সঁচা);
        newFuncp->classMethod(সঁচা);
        newFuncp->isConstructor(সঁচা);
        newFuncp->dtypep(cgClassp->dtypep());
        যদি (Ast_গ্ৰন্থি* ধ্রুৱ bodyp = nodep->membersp()) {
            bodyp->unlinkFrBackWithNext();
            newFuncp->addStmtsp(bodyp);
        }
        cgClassp->addMembersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_অঙ্ক);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_অঙ্ক = 0;
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (ধ্রুৱ AstCoverpoint* ধ্রুৱ pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    স্থির string coverpointName(ধ্রুৱ AstCoverpoint* nodep) {
        যদি (!nodep->name().empty()) ঘূৰাই nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ধ্রুৱ AstParseRef* ধ্রুৱ refp = VN_CAST(nodep->exprp(), ParseRef);
        ঘূৰাই refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ধ্রুৱ AstCoverpoint* findCoverpoint(ধ্রুৱ AstCoverpointRef* refp) ধ্রুৱ {
        যদি (refp->exprp()) ঘূৰাই nullptr;  // A hierarchical reference, unsupported
        ধ্রুৱ auto it = m_coverpoints.find(refp->name());
        ঘূৰাই it == m_coverpoints.end() ? nullptr : it->second;
    }

    শূন্য visit(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        যদি (nodep->name().empty()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ধ্রুৱ string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpoint_অঙ্ক++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        বাবে (Ast_গ্ৰন্থি *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (AstCgOptionAssign* ধ্রুৱ optp = VN_CAST(itemp, CgOptionAssign)) {
                যদি (dropDeprecatedCoverageOption(optp)) অনুৱর্ত;
                optp->unlinkFrBack();
                যদি (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(নূতন AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_বৃক্ষ(মিছা)});
                } অন্যথা {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_বৃক্ষ(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    শূন্য visit(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        যদি (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        যদি (ধ্রুৱ AstCoverpointRef* ধ্রুৱ refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            যদি (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_সাৰি* ধ্রুৱ fl = nodep->fileline();
        File_সাৰি* ধ্রুৱ flNoWarn = নূতন File_সাৰি{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, সঁচা);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, সঁচা);
        AstVar* ধ্রুৱ varp = নূতন AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        নূতন AstRefDType{fl, AstRefDType::Flag_Type_Of_ৰাশিমালা{},
                                                        m_coverpointp->exprp()->clone_বৃক্ষ(মিছা)}};
        varp->funcLocal(সঁচা);
        varp->noReset(সঁচা);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    শূন্য visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ধ্রুৱ refp = nodep->pointp();
        ধ্রুৱ AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ধ্রুৱ AstParseRef* binp = nullptr;
        ধ্রুৱ AstDot* ধ্রুৱ dotp = VN_CAST(refp->exprp(), Dot);
        যদি (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        যদি (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ঘূৰাই;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        যদি (binp) nodep->name(binp->name());
        refp->replaceWith(নূতন AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    শূন্য visit(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        যদি (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    শূন্য visit(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        যদি (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    শূন্য visit(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        বাবে (Ast_গ্ৰন্থি *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (VN_IS(itemp, CoverCrossBin)) অনুৱর্ত;
            AstCgOptionAssign* ধ্রুৱ optp = VN_AS(itemp, CgOptionAssign);
            যদি (dropDeprecatedCoverageOption(optp)) অনুৱর্ত;
            itemp->unlinkFrBack();
            ধ্রুৱ VCoverOptionType optType = optp->optType();
            যদি (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(নূতন AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_বৃক্ষ(মিছা)});
            VL_DO_DANGLING(optp->delete_বৃক্ষ(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    শূন্য visit(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        যদি (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_সাৰি* ধ্রুৱ fl = nodep->fileline();
            File_সাৰি* ধ্রুৱ flNoWarn = নূতন File_সাৰি{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, সঁচা);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, সঁচা);
            std::set<string> names;
            বাবে (ধ্রুৱ Ast_গ্ৰন্থি* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ধ্রুৱ AstCoverpointRef* ধ্রুৱ refp = VN_AS(itemp, CoverpointRef);
                ধ্রুৱ AstCoverpoint* ধ্রুৱ pointp = findCoverpoint(refp);
                যদি (!pointp || !names.ঢোকাও(refp->name()).second) অনুৱর্ত;
                AstVar* ধ্রুৱ varp
                    = নূতন AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 নূতন AstRefDType{fl, AstRefDType::Flag_Type_Of_ৰাশিমালা{},
                                                 pointp->exprp()->clone_বৃক্ষ(মিছা)}};
                varp->funcLocal(সঁচা);
                varp->noReset(সঁচা);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        যদি (!nodep->selectp()) VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    শূন্য visit(AstSystemCSection* nodep) override {
        যদি (nodep->user1SetOnce()) ঘূৰাই;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::addStatSum(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

শূন্য V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("linkparse", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
