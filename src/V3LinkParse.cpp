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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkParse.h"

#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

ଶ୍ରେଣୀ LinkParseVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<File_ଧାଡ଼ି*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    Ast_ଗଣ୍ଠି_Module* m_ମୂଲ୍ୟ_Modp = nullptr;
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    ତାର୍କିକ m_inInterface = ମିଥ୍ୟା;  // True when inside interface declaration
    Ast_ଗଣ୍ଠି_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    Ast_ଗଣ୍ଠି_Block* m_blockp = nullptr;  // Current AstNodeBlock
    Ast_ଗଣ୍ଠି_Stmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    Ast_ଗଣ୍ଠି_Stmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    Ast_ଗଣ୍ଠି_DType* m_dtypep = nullptr;  // Current data type
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_defaultInSkewp = nullptr;  // Current default input skew
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, ଅଚଳ AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_anonUdpId = 0;  // Counter for anonymous UDP instances
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_coverpoint_ଅଙ୍କ = 0;  // Counter for unnamed coverpoints within current covergroup
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_genblkAbove = 0;  // Begin block number of if/case/for above
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_genblk_ଅଙ୍କ = 0;  // Begin block number, 0=none seen
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_rand_Sequence_ଅଙ୍କ = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    ତାର୍କିକ m_lifetimeAllowed = ମିଥ୍ୟା;  // True to allow lifetime settings
    ତାର୍କିକ m_hasTimingControl = ମିଥ୍ୟା;  // If current task has timing control
    ତାର୍କିକ m_moduleWithGenericIface = ମିଥ୍ୟା;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    ଶୂନ୍ୟ cleanFileline(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (nodep->user2SetOnce()) ଫେରନ୍ତୁ;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        ଯଦି (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(ନୂତନ File_ଧାଡ଼ି{nodep->fileline()});
        }
        m_filelines.ଭର୍ତ୍ତି(nodep->fileline());
    }

    string nameFromTypedef(Ast_ଗଣ୍ଠି* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        ଯଦି (ଅଚଳ AstTypedef* ଅଚଳ typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            ଯଦି (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } ଅନ୍ୟଥା ଯଦି (m_modp) {
                above = m_modp->name() + ".";
            }
            ଫେରନ୍ତୁ above + typedefp->name();
        }
        ଫେରନ୍ତୁ "";
    }

    ଶୂନ୍ୟ visit_Iterate_ଗଣ୍ଠି_DType(Ast_ଗଣ୍ଠି_DType* nodep) {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    ତାର୍କିକ nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ backp = nodep->backp();
        ଫେରନ୍ତୁ (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    ଶୂନ୍ୟ checkIndent(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* childp) {
        // Try very hard to avoid false positives
        Ast_ଗଣ୍ଠି* nextp = nodep->nextp();
        ଯଦି (!childp) ଫେରନ୍ତୁ;
        ଯଦି (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        ଯଦି (!nextp) ଫେରନ୍ତୁ;
        ଯଦି (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) ଫେରନ୍ତୁ;
        File_ଧାଡ଼ି* ଅଚଳ ଗଣ୍ଠି_Flp = nodep->fileline();
        File_ଧାଡ଼ି* ଅଚଳ childFlp = childp->fileline();
        File_ଧାଡ଼ି* ଅଚଳ nextFlp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        ଯଦି (ଗଣ୍ଠି_Flp->filenameno() != childFlp->filenameno()) ଫେରନ୍ତୁ;
        ଯଦି (ଗଣ୍ଠି_Flp->filenameno() != nextFlp->filenameno()) ଫେରନ୍ତୁ;
        ଯଦି (ଗଣ୍ଠି_Flp->lastLineno() >= childFlp->firstLineno()) ଫେରନ୍ତୁ;
        ଯଦି (childFlp->lastLineno() >= nextFlp->firstLineno()) ଫେରନ୍ତୁ;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        ଯଦି (!(ଗଣ୍ଠି_Flp->firstColumn() < childFlp->firstColumn()
              && nextFlp->firstColumn() >= childFlp->firstColumn()))
            ଫେରନ୍ତୁ;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ଅଚଳ std::string ଗଣ୍ଠି_Prefix = ଗଣ୍ଠି_Flp->sourcePrefix(ଗଣ୍ଠି_Flp->firstColumn());
        ଅଚଳ std::string childPrefix = childFlp->sourcePrefix(ଗଣ୍ଠି_Flp->firstColumn());
        ଅଚଳ std::string nextPrefix = nextFlp->sourcePrefix(ଗଣ୍ଠି_Flp->firstColumn());
        ଯଦି (childPrefix != ଗଣ୍ଠି_Prefix) ଫେରନ୍ତୁ;
        ଯଦି (nextPrefix != childPrefix) ଫେରନ୍ତୁ;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ଅଚଳ std::string ଗଣ୍ଠି_Source = ଗଣ୍ଠି_Flp->source();
        ଅଚଳ std::string childSource = childFlp->source();
        ଅଚଳ std::string nextSource = nextFlp->source();
        ଯଦି (!(VString::leading_Whitespace_ଗଣନା(ଗଣ୍ଠି_Source)
                  < VString::leading_Whitespace_ଗଣନା(childSource)
              && VString::leading_Whitespace_ଗଣନା(nextSource)
                     >= VString::leading_Whitespace_ଗଣନା(childSource)))
            ଫେରନ୍ତୁ;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    ଶୂନ୍ୟ collectPorts(Ast_ଗଣ୍ଠି* ଗଣ୍ଠି_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = ଗଣ୍ଠି_Listp; nodep; nodep = nodep->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                ଯଦି (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = ଗଣ୍ଠି_Listp; nodep; nodep = nodep->nextp()) {
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
                ଯଦି (varp->isIO()) ଜାରି;
                ଅଚଳ auto it = portNames.find(varp->name());
                ଯଦି (it == portNames.end()) ଜାରି;
                AstVar* ଅଚଳ iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                ଯଦି (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlinkFrBackWithNext());
                m_portDups.emplace(varp);
            }
        }
    }

    Ast_ଗଣ୍ଠି* getVarsAndUnlink(Ast_ଗଣ୍ଠି* stmtsp) {
        Ast_ଗଣ୍ଠି* varsp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ଯଦି (VN_IS(itemp, Var)) varsp = Ast_ଗଣ୍ଠି::addNext(varsp, itemp->unlinkFrBack());
        }
        ଫେରନ୍ତୁ varsp;
    }
    Ast_ଗଣ୍ଠି_Stmt* getBlockAdds() {
        Ast_ଗଣ୍ଠି_Stmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        ଯଦି (m_blockAddStaticStmtp)
            addsp = Ast_ଗଣ୍ଠି::addNext(addsp,
                                     ନୂତନ AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        ଯଦି (m_blockAddAutomaticStmtp)
            addsp = Ast_ଗଣ୍ଠି::addNext(
                addsp, ନୂତନ AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        ଫେରନ୍ତୁ addsp;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process only once.
        // Mark class methods
        ଯଦି (VN_IS(m_modp, Class)) nodep->classMethod(ସତ୍ୟ);

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
        m_lifetimeAllowed = ସତ୍ୟ;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep) { ଫେରନ୍ତୁ nodep->isTimingControl(); });
        ଯଦି (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } ଅନ୍ୟଥା {
            ଯଦି (nodep->classMethod()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } ଅନ୍ୟଥା ଯଦି (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        ଯଦି (nodep->classMethod() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warnMore() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_ଗଣ୍ଠି* stmtsp = nullptr;
        ଯଦି (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = Ast_ଗଣ୍ଠି::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ଗଣ୍ଠି::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_DType* nodep) override { visit_Iterate_ଗଣ୍ଠି_DType(nodep); }
    ଶୂନ୍ୟ visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(ସତ୍ୟ);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstConstraintForeach* nodep) override {
        ଯଦି (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_ପଦସମଷ୍ଟି* exprp) { exprp->isSoft(ସତ୍ୟ); });
        }
    }
    ଶୂନ୍ୟ visit(AstEnumDType* nodep) override {
        ଯଦି (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_ଗଣ୍ଠି_DType(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {
        ଯଦି (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_ଗଣ୍ଠି_DType(nodep);
    }
    ଶୂନ୍ୟ visit(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        ଯଦି (nodep->rangep()) {
            ଯଦି (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t offset_from_init = 0;
            AstEnumItem* addp = nullptr;
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା i : nodep->rangep()->seq_ବାମ_To_ଡାହାଣ()) {
                ଅଚଳ string name = nodep->name() + cvtToStr(i);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = nullptr;
                ଯଦି (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = ନୂତନ AstAdd{flp, nodep->valuep()->clone_ବୃକ୍ଷ(ସତ୍ୟ),
                                     ନୂତନ AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = Ast_ଗଣ୍ଠି::addNext(addp, ନୂତନ AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(m_modp, Class);
        ଯଦି (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(ସତ୍ୟ);
        }
        ଯଦି (nodep->embeddedCovergroup()) {
            AstRefDType* ଅଚଳ refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        ଯଦି (nodep->valuep()) nodep->hasUserInit(ସତ୍ୟ);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        ଯଦି (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            ଯଦି (VN_IS(m_modp, Module) && m_ftaskp) {
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
            } ଅନ୍ୟଥା {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnMore()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        ଯଦି (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        ଯଦି (!nodep->direction().isAny()) {  // Not a port
            ଯଦି (nodep->lifetime().isNone()) {
                ଯଦି (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } ଅନ୍ୟଥା {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } ଅନ୍ୟଥା ଯଦି (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            ଯଦି (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } ଅନ୍ୟଥା {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        ଯଦି (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            ଯଦି (m_beginDepth > 0
                || (m_beginDepth == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        ଯଦି (nodep->isGParam() && m_modp) m_modp->hasGParam(ସତ୍ୟ);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ଅଚଳ AstBasicDType* ଅଚଳ basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        ଯଦି (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(ସତ୍ୟ);

        ଯଦି (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        ଯଦି (nodep->isParam() && m_inInterface) nodep->isIfaceParam(ସତ୍ୟ);
        ଯଦି (AstParseTypeDType* ଅଚଳ ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_ଗଣ୍ଠି* dtypep = nodep->valuep();
            ଯଦି (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                ଯଦି (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp = VN_CAST(dtypep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    ଯେତେବେଳେ (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        ଯଦି (!rhsDotp) ଭାଙ୍ଗ;
                        File_ଧାଡ଼ି* ଅଚଳ fl = dotp->fileline();
                        ଅଚଳ ତାର୍କିକ colon = dotp->colon();
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                        File_ଧାଡ଼ି* ଅଚଳ rhsFl = rhsDotp->fileline();
                        ଅଚଳ ତାର୍କିକ rhsColon = rhsDotp->colon();
                        AstDot* ଅଚଳ newLhs = ନୂତନ AstDot{fl, colon, lhs, rhsLhs};
                        exprp = ନୂତନ AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_ବୃକ୍ଷ(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } ଅନ୍ୟଥା {
                dtypep = ନୂତନ AstVoidDType{nodep->fileline()};
            }

            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, ନୂତନ AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(ମିଥ୍ୟା), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        ଯଦି (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            ଯଦି (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(ସତ୍ୟ);
            } ଅନ୍ୟଥା ଯଦି (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(ସତ୍ୟ);
            } ଅନ୍ୟଥା ଯଦି (m_modp && v3Global.opt.publicDepth()) {
                ଯଦି ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(ସତ୍ୟ);
                } ଅନ୍ୟଥା ଯଦି (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(ସତ୍ୟ);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        ଯଦି (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(ମିଥ୍ୟା);
            nodep->trace(ମିଥ୍ୟା);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        ଯଦି (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, ସତ୍ୟ);
        // Compute initial value
        ଯଦି (nodep->valuep() || nodep->needsCReset()) {
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
                ଯଦି (nodep->valuep()) {
                    ଫେରନ୍ତୁ VN_AS(nodep->valuep()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                } ଅନ୍ୟଥା {
                    ଫେରନ୍ତୁ ନୂତନ AstCReset{fl, nodep, ମିଥ୍ୟା};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            ଯଦି (m_portDups.find(nodep) != m_portDups.end()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } ଅନ୍ୟଥା ଯଦି (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } ଅନ୍ୟଥା ଯଦି (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_ନିବେଶ()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                ଯଦି (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                }
            } ଅନ୍ୟଥା ଯଦି (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(ସତ୍ୟ);
                File_ଧାଡ଼ି* ଅଚଳ newfl = ନୂତନ File_ଧାଡ଼ି{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, ସତ୍ୟ);
                AstAssign* ଅଚଳ assp
                    = ନୂତନ AstAssign{newfl, ନୂତନ AstParseRef{newfl, nodep->name()}, createValuep()};
                ଯଦି (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = Ast_ଗଣ୍ଠି::addNext(m_blockAddAutomaticStmtp, assp);
                } ଅନ୍ୟଥା {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = Ast_ଗଣ୍ଠି::addNext(m_blockAddStaticStmtp, assp);
                }
            } ଅନ୍ୟଥା ଯଦି (m_ମୂଲ୍ୟ_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                ଯଦି (nodep->valuep()) {
                    nodep->noCReset(ସତ୍ୟ);
                    File_ଧାଡ଼ି* ଅଚଳ newfl = ନୂତନ File_ଧାଡ଼ି{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, ସତ୍ୟ);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, ସତ୍ୟ);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* ଅଚଳ assp = ନୂତନ AstAssign{
                        newfl, ନୂତନ AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_ଗଣ୍ଠି* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    ଯଦି (nodep->lifetime().isAutomatic()) {
                        newInitp = ନୂତନ AstInitialAutomatic{newfl, assp};
                    } ଅନ୍ୟଥା {
                        newInitp = ନୂତନ AstInitialStatic{newfl, assp};
                    }
                    nodep->addNextHere(newInitp);
                }
            } ଅନ୍ୟଥା ଯଦି (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    ସ୍ଥିର ତାର୍କିକ isNegative(ଅଚଳ AstConst* constp) {
        ଫେରନ୍ତୁ constp->isSigned() && constp->ଅଙ୍କ().bitIs1(constp->width() - 1);
    }
    ଶୂନ୍ୟ visit(AstRange* nodep) override {
        ସ୍ଥିର constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ଅଚଳ AstConst* ଅଚଳ leftp = VN_CAST(nodep->leftp(), Const);
        ଅଚଳ AstConst* ଅଚଳ rightp = VN_CAST(nodep->rightp(), Const);
        ଯଦି (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ଅଚଳ uint64_t ବାମ = leftp->toUQuad();
            ଅଚଳ uint64_t ଡାହାଣ = rightp->toUQuad();
            ଅଚଳ uint64_t span = std::max(ବାମ, ଡାହାଣ) - std::min(ବାମ, ଡାହାଣ);
            ଯଦି (span >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << span + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        ଯଦି (nodep->ଅଙ୍କ().autoExtend() && nodep->fileline()->language() < V3LangCode::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    ଶୂନ୍ୟ visit(AstAttrOf* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ଯଦି (nodep->attrType() == VAttrType::DT_PUBLIC) {
            AstTypedef* ଅଚଳ typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ସତ୍ୟ);
            m_varp->sigModPublic(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_SPLIT_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            ଯଦି (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } ଅନ୍ୟଥା {
                m_varp->attrSplitVar(ସତ୍ୟ);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ସତ୍ୟ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ସତ୍ୟ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(ସତ୍ୟ);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ସତ୍ୟ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ସତ୍ୟ);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(ସତ୍ୟ);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(ସତ୍ୟ);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = nullptr;
        ଅଚଳ ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        ଯଦି (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } ଅନ୍ୟଥା {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_ଗଣ୍ଠି* backp = nodep->backp();
            ପାଇଁ (; backp; backp = backp->backp()) {
                ଯଦି (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, ଗଣ୍ଠି_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    ଭାଙ୍ଗ;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            ଯଦି (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                defp = ନୂତନ AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.size()));
                UINFO(9, "New impltypedef " << defp);
                backp->addNextHere(defp);
            }
        }
        nodep->replaceWith(ନୂତନ AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }

    ଶୂନ୍ୟ visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Foreach* nodep) override {
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
        AstForeachHeader* ଅଚଳ headerp = nodep->headerp();
        ଯଦି (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstLoop* nodep) override {
        cleanFileline(nodep);
        ଯଦି (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } ଅନ୍ୟଥା {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ଯଦି (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_ଧାଡ଼ି* ଅଚଳ newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, ସତ୍ୟ);
            nodep->fileline(newfl);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        ଯଦି (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_inInterface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_ଅଙ୍କ);
        VL_RESTORER(m_beginDepth);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_ଅଙ୍କ);
        VL_RESTORER(m_ମୂଲ୍ୟ_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        ଯଦି (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        ଯଦି (VN_IS(nodep, Iface)) m_inInterface = ସତ୍ୟ;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_ଅଙ୍କ = 0;
        m_beginDepth = 0;
        m_ମୂଲ୍ୟ_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = ମିଥ୍ୟା;
        m_rand_Sequence_ଅଙ୍କ = 0;

        ଯଦି (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        ଯଦି (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        ଯଦି (AstModule* ଅଚଳ modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    ଶୂନ୍ୟ visit_Iterate_No_ମୂଲ୍ୟ_Mod(Ast_ଗଣ୍ଠି* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_ମୂଲ୍ୟ_Modp);
        m_ମୂଲ୍ୟ_Modp = nullptr;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = ସତ୍ୟ;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        visit_Iterate_No_ମୂଲ୍ୟ_Mod(nodep);
    }
    ଶୂନ୍ୟ visit(AstCover* nodep) override { visit_Iterate_No_ମୂଲ୍ୟ_Mod(nodep); }
    ଶୂନ୍ୟ visit(AstRestrict* nodep) override { visit_Iterate_No_ମୂଲ୍ୟ_Mod(nodep); }

    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        V3Control::applyCoverageBlock(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_beginDepth);
        ++m_beginDepth;
        ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ଅଚଳ ତାର୍କିକ nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା assign_Gen_Blk_ଅଙ୍କ = -1;
        ଯଦି (nodep->genforp()) {
            ++m_genblk_ଅଙ୍କ;
            ଯଦି (nodep->name() == "") assign_Gen_Blk_ଅଙ୍କ = m_genblk_ଅଙ୍କ;
        } ଅନ୍ୟଥା ଯଦି (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            assign_Gen_Blk_ଅଙ୍କ = m_genblkAbove;
        }
        ଯଦି (assign_Gen_Blk_ଅଙ୍କ != -1) {
            nodep->name("genblk" + cvtToStr(assign_Gen_Blk_ଅଙ୍କ));
            ଯଦି (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warnMore()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        ଯଦି (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_ଅଙ୍କ);
            m_genblkAbove = 0;
            m_genblk_ଅଙ୍କ = 0;
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstGenCase* nodep) override {
        ++m_genblk_ଅଙ୍କ;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_ଅଙ୍କ);
        m_genblkAbove = m_genblk_ଅଙ୍କ;
        m_genblk_ଅଙ୍କ = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ଅଚଳ ତାର୍କିକ nestedIf = (VN_IS(nodep->backp(), GenBlock)
                               && nestedIfBegin(VN_CAST(nodep->backp(), GenBlock)));
        ଯଦି (nestedIf) {
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            ++m_genblk_ଅଙ୍କ;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_ଅଙ୍କ);
            m_genblkAbove = m_genblk_ଅଙ୍କ;
            m_genblk_ଅଙ୍କ = 0;
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        ଯଦି (nodep->origName().empty()) {
            ଯଦି (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ଅଚଳ string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Block* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterateAndNextNull(nodep->declsp());
        }

        ଯଦି (AstBegin* ଅଚଳ beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterateAndNextNull(nodep->stmtsp());
        ଯଦି (AstFork* ଅଚଳ forkp = VN_CAST(nodep, Fork)) iterateAndNextNull(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_ଗଣ୍ଠି* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = Ast_ଗଣ୍ଠି::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ଗଣ୍ଠି::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    ଶୂନ୍ୟ visit(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        ଯଦି (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        ଯଦି (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(ସତ୍ୟ);
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->addNext(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ଯଦି (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_ଗଣ୍ଠି* scanp = nodep;
            // Skip over the New's statement
            ପାଇଁ (; scanp && !VN_IS(scanp, Stmt_ପଦସମଷ୍ଟି); scanp = scanp->backp()) {}
            ଯଦି (VN_IS(scanp, Stmt_ପଦସମଷ୍ଟି)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                ପାଇଁ (; scanp; scanp = scanp->backp()) {
                    ଯଦି (VN_IS(scanp, ଗଣ୍ଠି_Stmt) || VN_IS(scanp, ଗଣ୍ଠି_Module)
                        || VN_IS(scanp, ଗଣ୍ଠି_FTask))
                        ଭାଙ୍ଗ;
                }
                ଯଦି (!VN_IS(scanp, ଗଣ୍ଠି_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    ଶୂନ୍ୟ visit(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::to_string(m_rand_Sequence_ଅଙ୍କ++));
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ଶୂନ୍ୟ visit(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ଶୂନ୍ୟ visit(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ଶୂନ୍ୟ visit(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ଶୂନ୍ୟ visit(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ଶୂନ୍ୟ visit(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ଶୂନ୍ୟ visit(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ଅଚଳ alwaysp = VN_CAST(nodep->backp(), Always);
        ଯଦି (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        } ଅନ୍ୟଥା ଯଦି (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            ଯଦି (Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sentreep = nodep->sentreep()) {
                sentreep->unlinkFrBackWithNext();
                alwaysp->sentreep(sentreep);
            }
            ଯଦି (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ଯଦି (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                ଯଦି (citemp->exprp() || citemp->assignp()) ଜାରି;
                ଯଦି (citemp->skewp()) {
                    ଯଦି (citemp->direction() == VDirection::ନିବେଶ) {
                        // Disallow default redefinition; note some simulators allow this
                        ଯଦି (m_defaultInSkewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_defaultInSkewp = citemp->skewp();
                    } ଅନ୍ୟଥା ଯଦି (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        ଯଦି (m_defaultOutSkewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_defaultOutSkewp = citemp->skewp();
                    } ଅନ୍ୟଥା {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(ଠେଲ_Deletep(citemp->unlinkFrBack()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        ଯଦି (nodep->direction() == VDirection::OUTPUT) {
            ଯଦି (!nodep->skewp()) {
                ଯଦି (m_defaultOutSkewp) {
                    nodep->skewp(m_defaultOutSkewp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                } ଅନ୍ୟଥା {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(ନୂତନ AstConst{nodep->fileline(), 0});
                }
            }
        } ଅନ୍ୟଥା ଯଦି (nodep->direction() == VDirection::ନିବେଶ) {
            ଯଦି (!nodep->skewp()) {
                ଯଦି (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                } ଅନ୍ୟଥା {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(ନୂତନ AstConst{nodep->fileline(), AstConst::OneStep{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        ଯଦି (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    ଶୂନ୍ୟ visit(Ast_Tagged_ପଦସମଷ୍ଟି* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstTaggedPattern* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPatternVar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPatternStar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        ଯଦି (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    ସ୍ଥିର Ast_ଗଣ୍ଠି* makeArgMemberCopies(Ast_ଗଣ୍ଠି* argsp, ତାର୍କିକ readOnlyRefs) {
        Ast_ଗଣ୍ଠି* resultp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ଅଚଳ origVarp = VN_AS(argp, Var);
            AstVar* ଅଚଳ paramp = origVarp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            File_ଧାଡ଼ି* ଅଚଳ fl = ନୂତନ File_ଧାଡ଼ି{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, ସତ୍ୟ);
            paramp->fileline(fl);
            paramp->funcLocal(ସତ୍ୟ);
            paramp->direction(origVarp->direction());
            ଯଦି (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_ଗଣ୍ଠି::addNextNull(resultp, paramp);
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = ନୂତନ AstDot{
                origVarp->fileline(), ମିଥ୍ୟା, ନୂତନ AstParseRef{origVarp->fileline(), "this"},
                ନୂତନ AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstParseRef{paramp->fileline(), paramp->name()};
            resultp->addNext(ନୂତନ AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        ଫେରନ୍ତୁ resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    ସ୍ଥିର ଶୂନ୍ୟ createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_ଗଣ୍ଠି* argsp,
                                        Ast_ଗଣ୍ଠି* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ଅଚଳ defaultVarp
            = ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        ଯଦି (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, ସତ୍ୟ));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ଅଚଳ varp
                = ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             ନୂତନ AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             ନୂତନ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ଅଚଳ varp
                = ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             ନୂତନ AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             ନୂତନ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ଅଚଳ funcp = ନୂତନ AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, ମିଥ୍ୟା));
            funcp->classMethod(ସତ୍ୟ);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(ସତ୍ୟ);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        ପାଇଁ (ଅଚଳ string& name : {"start"s, "stop"s}) {
            AstFunc* ଅଚଳ funcp = ନୂତନ AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(ସତ୍ୟ);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        ପାଇଁ (ଅଚଳ string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ଅଚଳ funcp = ନୂତନ AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, ସତ୍ୟ);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(ସତ୍ୟ);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* ଅଚଳ varp = ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ସତ୍ୟ);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(ସତ୍ୟ);
                funcp->fvarp(varp);
            }
            ପାଇଁ (ଅଚଳ string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ଅଚଳ varp = ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(ସତ୍ୟ);
                varp->direction(VDirection::ନିବେଶ);
                varp->valuep(ନୂତନ AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ଅଚଳ funcp
                = ନୂତନ AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(ସତ୍ୟ);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* ଅଚଳ varp = ନୂତନ AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(ସତ୍ୟ);
            varp->direction(VDirection::ନିବେଶ);
            funcp->addStmtsp(varp);
        }
    }

    ତାର୍କିକ dropDeprecatedCoverageOption(AstCgOptionAssign* ଅଚଳ nodep) {
        ଯଦି (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(AstCgOptionAssign* nodep) override {
        ଯଦି (dropDeprecatedCoverageOption(nodep)) ଫେରନ୍ତୁ;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        ଯଦି (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) ଫେରନ୍ତୁ;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        ଯଦି (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ଅଚଳ string libname = m_modp->libname();
        AstClass* ଅଚଳ cgClassp = ନୂତନ AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, ସତ୍ୟ);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, ସତ୍ୟ);
        cgClassp->isCovergroup(ସତ୍ୟ);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(ସତ୍ୟ);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        ଯଦି (Ast_Sen_ବୃକ୍ଷ* ଅଚଳ eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* ଅଚଳ cgNodep = ନୂତନ AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        ପାଇଁ (Ast_ଗଣ୍ଠି* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ଅଚଳ origVarp = VN_AS(argp, Var);
            ଯଦି (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::ନିବେଶ);
            }
            ଯଦି ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ଅଚଳ memberp = origVarp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ମିଥ୍ୟା);
            memberp->direction(VDirection::NONE);
            ଯଦି (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(ସତ୍ୟ);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        ପାଇଁ (Ast_ଗଣ୍ଠି* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ଅଚଳ origVarp = VN_AS(argp, Var);
            ଯଦି (!origVarp->is_ନିବେଶ()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::ନିବେଶ);
            }
            AstVar* ଅଚଳ memberp = origVarp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ମିଥ୍ୟା);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ଅଚଳ newFuncp = ନୂତନ AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, ସତ୍ୟ);
        newFuncp->classMethod(ସତ୍ୟ);
        newFuncp->isConstructor(ସତ୍ୟ);
        newFuncp->dtypep(cgClassp->dtypep());
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ bodyp = nodep->membersp()) {
            bodyp->unlinkFrBackWithNext();
            newFuncp->addStmtsp(bodyp);
        }
        cgClassp->addMembersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_ଅଙ୍କ);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_ଅଙ୍କ = 0;
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (ଅଚଳ AstCoverpoint* ଅଚଳ pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    ସ୍ଥିର string coverpointName(ଅଚଳ AstCoverpoint* nodep) {
        ଯଦି (!nodep->name().empty()) ଫେରନ୍ତୁ nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ଅଚଳ AstParseRef* ଅଚଳ refp = VN_CAST(nodep->exprp(), ParseRef);
        ଫେରନ୍ତୁ refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ଅଚଳ AstCoverpoint* findCoverpoint(ଅଚଳ AstCoverpointRef* refp) ଅଚଳ {
        ଯଦି (refp->exprp()) ଫେରନ୍ତୁ nullptr;  // A hierarchical reference, unsupported
        ଅଚଳ auto it = m_coverpoints.find(refp->name());
        ଫେରନ୍ତୁ it == m_coverpoints.end() ? nullptr : it->second;
    }

    ଶୂନ୍ୟ visit(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        ଯଦି (nodep->name().empty()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ଅଚଳ string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpoint_ଅଙ୍କ++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        ପାଇଁ (Ast_ଗଣ୍ଠି *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ଯଦି (AstCgOptionAssign* ଅଚଳ optp = VN_CAST(itemp, CgOptionAssign)) {
                ଯଦି (dropDeprecatedCoverageOption(optp)) ଜାରି;
                optp->unlinkFrBack();
                ଯଦି (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(ନୂତନ AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
                } ଅନ୍ୟଥା {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_ବୃକ୍ଷ(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        ଯଦି (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        ଯଦି (ଅଚଳ AstCoverpointRef* ଅଚଳ refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            ଯଦି (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        File_ଧାଡ଼ି* ଅଚଳ flNoWarn = ନୂତନ File_ଧାଡ଼ି{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ସତ୍ୟ);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, ସତ୍ୟ);
        AstVar* ଅଚଳ varp = ନୂତନ AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        ନୂତନ AstRefDType{fl, AstRefDType::Flag_Type_Of_ପଦସମଷ୍ଟି{},
                                                        m_coverpointp->exprp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}};
        varp->funcLocal(ସତ୍ୟ);
        varp->noReset(ସତ୍ୟ);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ଅଚଳ refp = nodep->pointp();
        ଅଚଳ AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ଅଚଳ AstParseRef* binp = nullptr;
        ଅଚଳ AstDot* ଅଚଳ dotp = VN_CAST(refp->exprp(), Dot);
        ଯଦି (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        ଯଦି (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
            ଫେରନ୍ତୁ;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        ଯଦି (binp) nodep->name(binp->name());
        refp->replaceWith(ନୂତନ AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ଯଦି (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        ଯଦି (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        ପାଇଁ (Ast_ଗଣ୍ଠି *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ଯଦି (VN_IS(itemp, CoverCrossBin)) ଜାରି;
            AstCgOptionAssign* ଅଚଳ optp = VN_AS(itemp, CgOptionAssign);
            ଯଦି (dropDeprecatedCoverageOption(optp)) ଜାରି;
            itemp->unlinkFrBack();
            ଅଚଳ VCoverOptionType optType = optp->optType();
            ଯଦି (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(ନୂତନ AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
            VL_DO_DANGLING(optp->delete_ବୃକ୍ଷ(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        ଯଦି (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
            File_ଧାଡ଼ି* ଅଚଳ flNoWarn = ନୂତନ File_ଧାଡ଼ି{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, ସତ୍ୟ);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, ସତ୍ୟ);
            std::set<string> names;
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ଅଚଳ AstCoverpointRef* ଅଚଳ refp = VN_AS(itemp, CoverpointRef);
                ଅଚଳ AstCoverpoint* ଅଚଳ pointp = findCoverpoint(refp);
                ଯଦି (!pointp || !names.ଭର୍ତ୍ତି(refp->name()).second) ଜାରି;
                AstVar* ଅଚଳ varp
                    = ନୂତନ AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 ନୂତନ AstRefDType{fl, AstRefDType::Flag_Type_Of_ପଦସମଷ୍ଟି{},
                                                 pointp->exprp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)}};
                varp->funcLocal(ସତ୍ୟ);
                varp->noReset(ସତ୍ୟ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        ଯଦି (!nodep->selectp()) VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ଶୂନ୍ୟ visit(AstSystemCSection* nodep) override {
        ଯଦି (nodep->user1SetOnce()) ଫେରନ୍ତୁ;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::addStatSum(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

ଶୂନ୍ୟ V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linkparse", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}
