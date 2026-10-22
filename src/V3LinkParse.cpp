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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkParse.h"

#অন্তর্ভাব "V3Control.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <set>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

শ্রেণী LinkParseVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ধ্রুবক VNUser1InUse m_inuser1;
    ধ্রুবক VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<File_সারি*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    Ast_গ্রন্থি_Module* m_মান_Modp = nullptr;
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    যৌক্তিক m_inInterface = মিথ্যা;  // True when inside interface declaration
    Ast_গ্রন্থি_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    Ast_গ্রন্থি_Block* m_blockp = nullptr;  // Current AstNodeBlock
    Ast_গ্রন্থি_Stmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    Ast_গ্রন্থি_Stmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    Ast_গ্রন্থি_DType* m_dtypep = nullptr;  // Current data type
    Ast_গ্রন্থি_রাশিমালা* m_defaultInSkewp = nullptr;  // Current default input skew
    Ast_গ্রন্থি_রাশিমালা* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, ধ্রুবক AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    পূর্ণসংখ্যা m_anonUdpId = 0;  // Counter for anonymous UDP instances
    পূর্ণসংখ্যা m_coverpoint_অঙ্ক = 0;  // Counter for unnamed coverpoints within current covergroup
    পূর্ণসংখ্যা m_genblkAbove = 0;  // Begin block number of if/case/for above
    পূর্ণসংখ্যা m_genblk_অঙ্ক = 0;  // Begin block number, 0=none seen
    পূর্ণসংখ্যা m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    পূর্ণসংখ্যা m_rand_Sequence_অঙ্ক = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    যৌক্তিক m_lifetimeAllowed = মিথ্যা;  // True to allow lifetime settings
    যৌক্তিক m_hasTimingControl = মিথ্যা;  // If current task has timing control
    যৌক্তিক m_moduleWithGenericIface = মিথ্যা;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    শূন্য cleanFileline(Ast_গ্রন্থি* nodep) {
        যদি (nodep->user2SetOnce()) ফেরত;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        যদি (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(নতুন File_সারি{nodep->fileline()});
        }
        m_filelines.ঢোকাও(nodep->fileline());
    }

    string nameFromTypedef(Ast_গ্রন্থি* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        যদি (ধ্রুবক AstTypedef* ধ্রুবক typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            যদি (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } অন্যথা যদি (m_modp) {
                above = m_modp->name() + ".";
            }
            ফেরত above + typedefp->name();
        }
        ফেরত "";
    }

    শূন্য visit_Iterate_গ্রন্থি_DType(Ast_গ্রন্থি_DType* nodep) {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    যৌক্তিক nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ধ্রুবক Ast_গ্রন্থি* ধ্রুবক backp = nodep->backp();
        ফেরত (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    শূন্য checkIndent(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* childp) {
        // Try very hard to avoid false positives
        Ast_গ্রন্থি* nextp = nodep->nextp();
        যদি (!childp) ফেরত;
        যদি (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        যদি (!nextp) ফেরত;
        যদি (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) ফেরত;
        File_সারি* ধ্রুবক গ্রন্থি_Flp = nodep->fileline();
        File_সারি* ধ্রুবক childFlp = childp->fileline();
        File_সারি* ধ্রুবক পরবর্তী_Flp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        যদি (গ্রন্থি_Flp->filenameno() != childFlp->filenameno()) ফেরত;
        যদি (গ্রন্থি_Flp->filenameno() != পরবর্তী_Flp->filenameno()) ফেরত;
        যদি (গ্রন্থি_Flp->lastLineno() >= childFlp->firstLineno()) ফেরত;
        যদি (childFlp->lastLineno() >= পরবর্তী_Flp->firstLineno()) ফেরত;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        যদি (!(গ্রন্থি_Flp->firstColumn() < childFlp->firstColumn()
              && পরবর্তী_Flp->firstColumn() >= childFlp->firstColumn()))
            ফেরত;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ধ্রুবক std::string গ্রন্থি_Prefix = গ্রন্থি_Flp->sourcePrefix(গ্রন্থি_Flp->firstColumn());
        ধ্রুবক std::string childPrefix = childFlp->sourcePrefix(গ্রন্থি_Flp->firstColumn());
        ধ্রুবক std::string পরবর্তী_Prefix = পরবর্তী_Flp->sourcePrefix(গ্রন্থি_Flp->firstColumn());
        যদি (childPrefix != গ্রন্থি_Prefix) ফেরত;
        যদি (পরবর্তী_Prefix != childPrefix) ফেরত;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ধ্রুবক std::string গ্রন্থি_Source = গ্রন্থি_Flp->source();
        ধ্রুবক std::string childSource = childFlp->source();
        ধ্রুবক std::string পরবর্তী_Source = পরবর্তী_Flp->source();
        যদি (!(VString::leading_Whitespace_গুনতি(গ্রন্থি_Source)
                  < VString::leading_Whitespace_গুনতি(childSource)
              && VString::leading_Whitespace_গুনতি(পরবর্তী_Source)
                     >= VString::leading_Whitespace_গুনতি(childSource)))
            ফেরত;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    শূন্য collectPorts(Ast_গ্রন্থি* গ্রন্থি_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        জন্য (Ast_গ্রন্থি* nodep = গ্রন্থি_Listp; nodep; nodep = nodep->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
                যদি (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        জন্য (Ast_গ্রন্থি* nodep = গ্রন্থি_Listp; nodep; nodep = nodep->nextp()) {
            যদি (AstVar* ধ্রুবক varp = VN_CAST(nodep, Var)) {
                যদি (varp->isIO()) চালু;
                ধ্রুবক auto it = portNames.find(varp->name());
                যদি (it == portNames.end()) চালু;
                AstVar* ধ্রুবক iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                যদি (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlink_Fr_Back_With_পরবর্তী());
                m_portDups.emplace(varp);
            }
        }
    }

    Ast_গ্রন্থি* getVarsAndUnlink(Ast_গ্রন্থি* stmtsp) {
        Ast_গ্রন্থি* varsp = nullptr;
        জন্য (Ast_গ্রন্থি *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (VN_IS(itemp, Var)) varsp = Ast_গ্রন্থি::add_পরবর্তী(varsp, itemp->unlinkFrBack());
        }
        ফেরত varsp;
    }
    Ast_গ্রন্থি_Stmt* getBlockAdds() {
        Ast_গ্রন্থি_Stmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        যদি (m_blockAddStaticStmtp)
            addsp = Ast_গ্রন্থি::add_পরবর্তী(addsp,
                                     নতুন AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        যদি (m_blockAddAutomaticStmtp)
            addsp = Ast_গ্রন্থি::add_পরবর্তী(
                addsp, নতুন AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        ফেরত addsp;
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process only once.
        // Mark class methods
        যদি (VN_IS(m_modp, Class)) nodep->classMethod(সত্য);

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
        m_lifetimeAllowed = সত্য;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nodep) { ফেরত nodep->isTimingControl(); });
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
        Ast_গ্রন্থি* stmtsp = nullptr;
        যদি (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = Ast_গ্রন্থি::add_পরবর্তী_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_গ্রন্থি::add_পরবর্তী_Null(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    শূন্য visit(Ast_গ্রন্থি_DType* nodep) override { visit_Iterate_গ্রন্থি_DType(nodep); }
    শূন্য visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(সত্য);
        iterateChildren(nodep);
    }
    শূন্য visit(AstConstraintForeach* nodep) override {
        যদি (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_রাশিমালা* exprp) { exprp->isSoft(সত্য); });
        }
    }
    শূন্য visit(AstEnumDType* nodep) override {
        যদি (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_গ্রন্থি_DType(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_UOr_Struct_DType* nodep) override {
        যদি (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_গ্রন্থি_DType(nodep);
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
            File_সারি* ধ্রুবক flp = nodep->fileline();
            জন্য (ধ্রুবক পূর্ণসংখ্যা i : nodep->rangep()->seq_বাম_To_ডান()) {
                ধ্রুবক string name = nodep->name() + cvtToStr(i);
                Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;
                যদি (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = নতুন AstAdd{flp, nodep->valuep()->clone_বৃক্ষ(সত্য),
                                     নতুন AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = Ast_গ্রন্থি::add_পরবর্তী(addp, নতুন AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        }
    }

    শূন্য visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ধ্রুবক AstClass* ধ্রুবক classp = VN_CAST(m_modp, Class);
        যদি (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(সত্য);
        }
        যদি (nodep->embeddedCovergroup()) {
            AstRefDType* ধ্রুবক refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        যদি (nodep->valuep()) nodep->hasUserInit(সত্য);
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
        যদি (nodep->isGParam() && m_modp) m_modp->hasGParam(সত্য);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ধ্রুবক AstBasicDType* ধ্রুবক basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        যদি (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(সত্য);

        যদি (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        যদি (nodep->isParam() && m_inInterface) nodep->isIfaceParam(সত্য);
        যদি (AstParseTypeDType* ধ্রুবক ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_গ্রন্থি* dtypep = nodep->valuep();
            যদি (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                যদি (Ast_গ্রন্থি_রাশিমালা* exprp = VN_CAST(dtypep, গ্রন্থি_রাশিমালা)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    যখন (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        যদি (!rhsDotp) বিরতি;
                        File_সারি* ধ্রুবক fl = dotp->fileline();
                        ধ্রুবক যৌক্তিক colon = dotp->colon();
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
                        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
                        File_সারি* ধ্রুবক rhsFl = rhsDotp->fileline();
                        ধ্রুবক যৌক্তিক rhsColon = rhsDotp->colon();
                        AstDot* ধ্রুবক newLhs = নতুন AstDot{fl, colon, lhs, rhsLhs};
                        exprp = নতুন AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_বৃক্ষ(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } অন্যথা {
                dtypep = নতুন AstVoidDType{nodep->fileline()};
            }

            Ast_গ্রন্থি* ধ্রুবক newp = নতুন AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, নতুন AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            ফেরত;
        }
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(মিথ্যা), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        যদি (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            যদি (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(সত্য);
            } অন্যথা যদি (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(সত্য);
            } অন্যথা যদি (m_modp && v3Global.opt.publicDepth()) {
                যদি ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(সত্য);
                } অন্যথা যদি (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(সত্য);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        যদি (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(মিথ্যা);
            nodep->trace(মিথ্যা);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        যদি (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, সত্য);
        // Compute initial value
        যদি (nodep->valuep() || nodep->needsCReset()) {
            File_সারি* ধ্রুবক fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
                যদি (nodep->valuep()) {
                    ফেরত VN_AS(nodep->valuep()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
                } অন্যথা {
                    ফেরত নতুন AstCReset{fl, nodep, মিথ্যা};
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
                       && !nodep->is_প্রবেশ()) {
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
                nodep->noCReset(সত্য);
                File_সারি* ধ্রুবক newfl = নতুন File_সারি{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, সত্য);
                AstAssign* ধ্রুবক assp
                    = নতুন AstAssign{newfl, নতুন AstParseRef{newfl, nodep->name()}, createValuep()};
                যদি (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = Ast_গ্রন্থি::add_পরবর্তী(m_blockAddAutomaticStmtp, assp);
                } অন্যথা {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = Ast_গ্রন্থি::add_পরবর্তী(m_blockAddStaticStmtp, assp);
                }
            } অন্যথা যদি (m_মান_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                যদি (nodep->valuep()) {
                    nodep->noCReset(সত্য);
                    File_সারি* ধ্রুবক newfl = নতুন File_সারি{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, সত্য);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, সত্য);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* ধ্রুবক assp = নতুন AstAssign{
                        newfl, নতুন AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_গ্রন্থি* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    যদি (nodep->lifetime().isAutomatic()) {
                        newInitp = নতুন AstInitialAutomatic{newfl, assp};
                    } অন্যথা {
                        newInitp = নতুন AstInitialStatic{newfl, assp};
                    }
                    nodep->add_পরবর্তী_Here(newInitp);
                }
            } অন্যথা যদি (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    স্থির যৌক্তিক isNegative(ধ্রুবক AstConst* constp) {
        ফেরত constp->isSigned() && constp->অঙ্ক().bitIs1(constp->width() - 1);
    }
    শূন্য visit(AstRange* nodep) override {
        স্থির constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ধ্রুবক AstConst* ধ্রুবক leftp = VN_CAST(nodep->leftp(), Const);
        ধ্রুবক AstConst* ধ্রুবক rightp = VN_CAST(nodep->rightp(), Const);
        যদি (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ধ্রুবক uint64_t বাম = leftp->toUQuad();
            ধ্রুবক uint64_t ডান = rightp->toUQuad();
            ধ্রুবক uint64_t span = std::max(বাম, ডান) - std::min(বাম, ডান);
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
            AstTypedef* ধ্রুবক typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(সত্য);
            m_varp->sigModPublic(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(সত্য);
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
                m_varp->attrSplitVar(সত্য);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(সত্য);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(সত্য);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(সত্য);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(সত্য);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(সত্য);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(সত্য);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(সত্য);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(সত্য);
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
        ধ্রুবক ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        যদি (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } অন্যথা {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_গ্রন্থি* backp = nodep->backp();
            জন্য (; backp; backp = backp->backp()) {
                যদি (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, গ্রন্থি_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    বিরতি;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            যদি (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
                ফেরত;
            } অন্যথা {
                defp = নতুন AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.আকার_2()));
                UINFO(9, "New impltypedef " << defp);
                backp->add_পরবর্তী_Here(defp);
            }
        }
        nodep->replaceWith(নতুন AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }

    শূন্য visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি_Foreach* nodep) override {
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
        AstForeachHeader* ধ্রুবক headerp = nodep->headerp();
        যদি (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
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
            File_সারি* ধ্রুবক newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, সত্য);
            nodep->fileline(newfl);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
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
        যদি (VN_IS(nodep, Iface)) m_inInterface = সত্য;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_অঙ্ক = 0;
        m_beginDepth = 0;
        m_মান_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = মিথ্যা;
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
        যদি (AstModule* ধ্রুবক modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    শূন্য visit_Iterate_No_মান_Mod(Ast_গ্রন্থি* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_মান_Modp);
        m_মান_Modp = nullptr;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = সত্য;
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
        ধ্রুবক Ast_গ্রন্থি* ধ্রুবক backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ধ্রুবক যৌক্তিক nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        পূর্ণসংখ্যা assign_Gen_Blk_অঙ্ক = -1;
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
        ধ্রুবক যৌক্তিক nestedIf = (VN_IS(nodep->backp(), GenBlock)
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
            ধ্রুবক string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
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
    শূন্য visit(Ast_গ্রন্থি_Block* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterate_And_পরবর্তী_Null(nodep->declsp());
        }

        যদি (AstBegin* ধ্রুবক beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterate_And_পরবর্তী_Null(nodep->stmtsp());
        যদি (AstFork* ধ্রুবক forkp = VN_CAST(nodep, Fork)) iterate_And_পরবর্তী_Null(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_গ্রন্থি* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = Ast_গ্রন্থি::add_পরবর্তী_Null(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_গ্রন্থি::add_পরবর্তী_Null(stmtsp, getBlockAdds());
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
            nodep->user2(সত্য);
            Ast_গ্রন্থি* ধ্রুবক nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->add_পরবর্তী(nodep);
        }
    }
    শূন্য visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        যদি (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_গ্রন্থি* scanp = nodep;
            // Skip over the New's statement
            জন্য (; scanp && !VN_IS(scanp, Stmt_রাশিমালা); scanp = scanp->backp()) {}
            যদি (VN_IS(scanp, Stmt_রাশিমালা)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                জন্য (; scanp; scanp = scanp->backp()) {
                    যদি (VN_IS(scanp, গ্রন্থি_Stmt) || VN_IS(scanp, গ্রন্থি_Module)
                        || VN_IS(scanp, গ্রন্থি_FTask))
                        বিরতি;
                }
                যদি (!VN_IS(scanp, গ্রন্থি_FTask)) {
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
        AstAlways* ধ্রুবক alwaysp = VN_CAST(nodep->backp(), Always);
        যদি (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        } অন্যথা যদি (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            যদি (Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = nodep->sentreep()) {
                sentreep->unlink_Fr_Back_With_পরবর্তী();
                alwaysp->sentreep(sentreep);
            }
            যদি (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlink_Fr_Back_With_পরবর্তী());
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
        }
    }
    শূন্য visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        জন্য (Ast_গ্রন্থি *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                যদি (citemp->exprp() || citemp->assignp()) চালু;
                যদি (citemp->skewp()) {
                    যদি (citemp->direction() == VDirection::প্রবেশ) {
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
                    nodep->skewp(m_defaultOutSkewp->clone_বৃক্ষ(মিথ্যা));
                } অন্যথা {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(নতুন AstConst{nodep->fileline(), 0});
                }
            }
        } অন্যথা যদি (nodep->direction() == VDirection::প্রবেশ) {
            যদি (!nodep->skewp()) {
                যদি (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->clone_বৃক্ষ(মিথ্যা));
                } অন্যথা {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(নতুন AstConst{nodep->fileline(), AstConst::OneStep{}});
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
    শূন্য visit(Ast_Tagged_রাশিমালা* nodep) override {
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
    স্থির Ast_গ্রন্থি* makeArgMemberCopies(Ast_গ্রন্থি* argsp, যৌক্তিক readOnlyRefs) {
        Ast_গ্রন্থি* resultp = nullptr;
        জন্য (Ast_গ্রন্থি* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ধ্রুবক origVarp = VN_AS(argp, Var);
            AstVar* ধ্রুবক paramp = origVarp->clone_বৃক্ষ(মিথ্যা);
            File_সারি* ধ্রুবক fl = নতুন File_সারি{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, সত্য);
            paramp->fileline(fl);
            paramp->funcLocal(সত্য);
            paramp->direction(origVarp->direction());
            যদি (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_গ্রন্থি::add_পরবর্তী_Null(resultp, paramp);
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = নতুন AstDot{
                origVarp->fileline(), মিথ্যা, নতুন AstParseRef{origVarp->fileline(), "this"},
                নতুন AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = নতুন AstParseRef{paramp->fileline(), paramp->name()};
            resultp->add_পরবর্তী(নতুন AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        ফেরত resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    স্থির শূন্য createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_গ্রন্থি* argsp,
                                        Ast_গ্রন্থি* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ধ্রুবক defaultVarp
            = নতুন AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        যদি (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, সত্য));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ধ্রুবক varp
                = নতুন AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             নতুন AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             নতুন AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ধ্রুবক varp
                = নতুন AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             নতুন AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             নতুন AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ধ্রুবক funcp = নতুন AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, মিথ্যা));
            funcp->classMethod(সত্য);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(সত্য);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        জন্য (ধ্রুবক string& name : {"start"s, "stop"s}) {
            AstFunc* ধ্রুবক funcp = নতুন AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(সত্য);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        জন্য (ধ্রুবক string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ধ্রুবক funcp = নতুন AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, সত্য);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(সত্য);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* ধ্রুবক varp = নতুন AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(সত্য);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(সত্য);
                funcp->fvarp(varp);
            }
            জন্য (ধ্রুবক string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ধ্রুবক varp = নতুন AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(সত্য);
                varp->direction(VDirection::প্রবেশ);
                varp->valuep(নতুন AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ধ্রুবক funcp
                = নতুন AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(সত্য);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* ধ্রুবক varp = নতুন AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(সত্য);
            varp->direction(VDirection::প্রবেশ);
            funcp->addStmtsp(varp);
        }
    }

    যৌক্তিক dropDeprecatedCoverageOption(AstCgOptionAssign* ধ্রুবক nodep) {
        যদি (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) ফেরত মিথ্যা;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
        ফেরত সত্য;
    }

    শূন্য visit(AstCgOptionAssign* nodep) override {
        যদি (dropDeprecatedCoverageOption(nodep)) ফেরত;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    শূন্য visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        যদি (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) ফেরত;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        যদি (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ধ্রুবক string libname = m_modp->libname();
        AstClass* ধ্রুবক cgClassp = নতুন AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, সত্য);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, সত্য);
        cgClassp->isCovergroup(সত্য);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(সত্য);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        যদি (Ast_Sen_বৃক্ষ* ধ্রুবক eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* ধ্রুবক cgNodep = নতুন AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        জন্য (Ast_গ্রন্থি* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ধ্রুবক origVarp = VN_AS(argp, Var);
            যদি (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::প্রবেশ);
            }
            যদি ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ধ্রুবক memberp = origVarp->clone_বৃক্ষ(মিথ্যা);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(মিথ্যা);
            memberp->direction(VDirection::NONE);
            যদি (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(সত্য);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        জন্য (Ast_গ্রন্থি* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ধ্রুবক origVarp = VN_AS(argp, Var);
            যদি (!origVarp->is_প্রবেশ()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::প্রবেশ);
            }
            AstVar* ধ্রুবক memberp = origVarp->clone_বৃক্ষ(মিথ্যা);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(মিথ্যা);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ধ্রুবক newFuncp = নতুন AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, সত্য);
        newFuncp->classMethod(সত্য);
        newFuncp->isConstructor(সত্য);
        newFuncp->dtypep(cgClassp->dtypep());
        যদি (Ast_গ্রন্থি* ধ্রুবক bodyp = nodep->membersp()) {
            bodyp->unlink_Fr_Back_With_পরবর্তী();
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
        জন্য (ধ্রুবক Ast_গ্রন্থি* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (ধ্রুবক AstCoverpoint* ধ্রুবক pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    স্থির string coverpointName(ধ্রুবক AstCoverpoint* nodep) {
        যদি (!nodep->name().empty()) ফেরত nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ধ্রুবক AstParseRef* ধ্রুবক refp = VN_CAST(nodep->exprp(), ParseRef);
        ফেরত refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ধ্রুবক AstCoverpoint* findCoverpoint(ধ্রুবক AstCoverpointRef* refp) ধ্রুবক {
        যদি (refp->exprp()) ফেরত nullptr;  // A hierarchical reference, unsupported
        ধ্রুবক auto it = m_coverpoints.find(refp->name());
        ফেরত it == m_coverpoints.end() ? nullptr : it->second;
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
            ধ্রুবক string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpoint_অঙ্ক++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        জন্য (Ast_গ্রন্থি *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (AstCgOptionAssign* ধ্রুবক optp = VN_CAST(itemp, CgOptionAssign)) {
                যদি (dropDeprecatedCoverageOption(optp)) চালু;
                optp->unlinkFrBack();
                যদি (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(নতুন AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_বৃক্ষ(মিথ্যা)});
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
            ফেরত;
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        যদি (ধ্রুবক AstCoverpointRef* ধ্রুবক refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            যদি (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_সারি* ধ্রুবক fl = nodep->fileline();
        File_সারি* ধ্রুবক flNoWarn = নতুন File_সারি{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, সত্য);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, সত্য);
        AstVar* ধ্রুবক varp = নতুন AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        নতুন AstRefDType{fl, AstRefDType::Flag_Type_Of_রাশিমালা{},
                                                        m_coverpointp->exprp()->clone_বৃক্ষ(মিথ্যা)}};
        varp->funcLocal(সত্য);
        varp->noReset(সত্য);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    শূন্য visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ধ্রুবক refp = nodep->pointp();
        ধ্রুবক AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ধ্রুবক AstParseRef* binp = nullptr;
        ধ্রুবক AstDot* ধ্রুবক dotp = VN_CAST(refp->exprp(), Dot);
        যদি (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        যদি (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
            ফেরত;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        যদি (binp) nodep->name(binp->name());
        refp->replaceWith(নতুন AstCoverpointRef{pointp->fileline(), pointp->name()});
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
        জন্য (Ast_গ্রন্থি *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            যদি (VN_IS(itemp, CoverCrossBin)) চালু;
            AstCgOptionAssign* ধ্রুবক optp = VN_AS(itemp, CgOptionAssign);
            যদি (dropDeprecatedCoverageOption(optp)) চালু;
            itemp->unlinkFrBack();
            ধ্রুবক VCoverOptionType optType = optp->optType();
            যদি (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(নতুন AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_বৃক্ষ(মিথ্যা)});
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
            File_সারি* ধ্রুবক fl = nodep->fileline();
            File_সারি* ধ্রুবক flNoWarn = নতুন File_সারি{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, সত্য);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, সত্য);
            std::set<string> names;
            জন্য (ধ্রুবক Ast_গ্রন্থি* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ধ্রুবক AstCoverpointRef* ধ্রুবক refp = VN_AS(itemp, CoverpointRef);
                ধ্রুবক AstCoverpoint* ধ্রুবক pointp = findCoverpoint(refp);
                যদি (!pointp || !names.ঢোকাও(refp->name()).second) চালু;
                AstVar* ধ্রুবক varp
                    = নতুন AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 নতুন AstRefDType{fl, AstRefDType::Flag_Type_Of_রাশিমালা{},
                                                 pointp->exprp()->clone_বৃক্ষ(মিথ্যা)}};
                varp->funcLocal(সত্য);
                varp->noReset(সত্য);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        যদি (!nodep->selectp()) VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    শূন্য visit(AstSystemCSection* nodep) override {
        যদি (nodep->user1SetOnce()) ফেরত;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

সর্বজনীন:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::add_Stat_যোগ(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

শূন্য V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("linkparse", 0, dump_বৃক্ষ_Either_Level() >= 6);
}
