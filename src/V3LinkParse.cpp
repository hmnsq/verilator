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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkParse.h"

#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <set>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

વર્ગ LinkParseVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<File_લીટી*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    Ast_ગાંઠ_Module* m_કિંમત_Modp = nullptr;
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    તાર્કિક m_inInterface = ખોટું;  // True when inside interface declaration
    Ast_ગાંઠ_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    Ast_ગાંઠ_Block* m_blockp = nullptr;  // Current AstNodeBlock
    Ast_ગાંઠ_Stmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    Ast_ગાંઠ_Stmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    Ast_ગાંઠ_DType* m_dtypep = nullptr;  // Current data type
    Ast_ગાંઠ_પદાવલિ* m_defaultInSkewp = nullptr;  // Current default input skew
    Ast_ગાંઠ_પદાવલિ* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, અચલ AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    પૂર્ણાંક m_anonUdpId = 0;  // Counter for anonymous UDP instances
    પૂર્ણાંક m_coverpoint_અંક = 0;  // Counter for unnamed coverpoints within current covergroup
    પૂર્ણાંક m_genblkAbove = 0;  // Begin block number of if/case/for above
    પૂર્ણાંક m_genblk_અંક = 0;  // Begin block number, 0=none seen
    પૂર્ણાંક m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    પૂર્ણાંક m_rand_Sequence_અંક = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    તાર્કિક m_lifetimeAllowed = ખોટું;  // True to allow lifetime settings
    તાર્કિક m_hasTimingControl = ખોટું;  // If current task has timing control
    તાર્કિક m_moduleWithGenericIface = ખોટું;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    શૂન્ય cleanFileline(Ast_ગાંઠ* nodep) {
        જો (nodep->user2SetOnce()) પાછા;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        જો (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(નવું File_લીટી{nodep->fileline()});
        }
        m_filelines.ઉમેરો(nodep->fileline());
    }

    string nameFromTypedef(Ast_ગાંઠ* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        જો (અચલ AstTypedef* અચલ typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            જો (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } નહીંતર જો (m_modp) {
                above = m_modp->name() + ".";
            }
            પાછા above + typedefp->name();
        }
        પાછા "";
    }

    શૂન્ય visit_Iterate_ગાંઠ_DType(Ast_ગાંઠ_DType* nodep) {
        જો (nodep->user1SetOnce()) પાછા;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    તાર્કિક nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        અચલ Ast_ગાંઠ* અચલ backp = nodep->backp();
        પાછા (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    શૂન્ય checkIndent(Ast_ગાંઠ* nodep, Ast_ગાંઠ* childp) {
        // Try very hard to avoid false positives
        Ast_ગાંઠ* nextp = nodep->nextp();
        જો (!childp) પાછા;
        જો (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        જો (!nextp) પાછા;
        જો (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) પાછા;
        File_લીટી* અચલ ગાંઠ_Flp = nodep->fileline();
        File_લીટી* અચલ childFlp = childp->fileline();
        File_લીટી* અચલ nextFlp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        જો (ગાંઠ_Flp->filenameno() != childFlp->filenameno()) પાછા;
        જો (ગાંઠ_Flp->filenameno() != nextFlp->filenameno()) પાછા;
        જો (ગાંઠ_Flp->lastLineno() >= childFlp->firstLineno()) પાછા;
        જો (childFlp->lastLineno() >= nextFlp->firstLineno()) પાછા;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        જો (!(ગાંઠ_Flp->firstColumn() < childFlp->firstColumn()
              && nextFlp->firstColumn() >= childFlp->firstColumn()))
            પાછા;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        અચલ std::string ગાંઠ_Prefix = ગાંઠ_Flp->sourcePrefix(ગાંઠ_Flp->firstColumn());
        અચલ std::string childPrefix = childFlp->sourcePrefix(ગાંઠ_Flp->firstColumn());
        અચલ std::string nextPrefix = nextFlp->sourcePrefix(ગાંઠ_Flp->firstColumn());
        જો (childPrefix != ગાંઠ_Prefix) પાછા;
        જો (nextPrefix != childPrefix) પાછા;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        અચલ std::string ગાંઠ_Source = ગાંઠ_Flp->source();
        અચલ std::string childSource = childFlp->source();
        અચલ std::string nextSource = nextFlp->source();
        જો (!(VString::leading_Whitespace_ગણતરી(ગાંઠ_Source)
                  < VString::leading_Whitespace_ગણતરી(childSource)
              && VString::leading_Whitespace_ગણતરી(nextSource)
                     >= VString::leading_Whitespace_ગણતરી(childSource)))
            પાછા;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    શૂન્ય collectPorts(Ast_ગાંઠ* ગાંઠ_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        માટે (Ast_ગાંઠ* nodep = ગાંઠ_Listp; nodep; nodep = nodep->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                જો (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        માટે (Ast_ગાંઠ* nodep = ગાંઠ_Listp; nodep; nodep = nodep->nextp()) {
            જો (AstVar* અચલ varp = VN_CAST(nodep, Var)) {
                જો (varp->isIO()) ચાલુ;
                અચલ auto it = portNames.find(varp->name());
                જો (it == portNames.end()) ચાલુ;
                AstVar* અચલ iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                જો (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlinkFrBackWithNext());
                m_portDups.emplace(varp);
            }
        }
    }

    Ast_ગાંઠ* getVarsAndUnlink(Ast_ગાંઠ* stmtsp) {
        Ast_ગાંઠ* varsp = nullptr;
        માટે (Ast_ગાંઠ *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            જો (VN_IS(itemp, Var)) varsp = Ast_ગાંઠ::addNext(varsp, itemp->unlinkFrBack());
        }
        પાછા varsp;
    }
    Ast_ગાંઠ_Stmt* getBlockAdds() {
        Ast_ગાંઠ_Stmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        જો (m_blockAddStaticStmtp)
            addsp = Ast_ગાંઠ::addNext(addsp,
                                     નવું AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        જો (m_blockAddAutomaticStmtp)
            addsp = Ast_ગાંઠ::addNext(
                addsp, નવું AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        પાછા addsp;
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process only once.
        // Mark class methods
        જો (VN_IS(m_modp, Class)) nodep->classMethod(સાચું);

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
        m_lifetimeAllowed = સાચું;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](અચલ Ast_ગાંઠ* અચલ nodep) { પાછા nodep->isTimingControl(); });
        જો (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } નહીંતર {
            જો (nodep->classMethod()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } નહીંતર જો (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        જો (nodep->classMethod() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warnMore() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_ગાંઠ* stmtsp = nullptr;
        જો (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = Ast_ગાંઠ::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ગાંઠ::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    શૂન્ય visit(Ast_ગાંઠ_DType* nodep) override { visit_Iterate_ગાંઠ_DType(nodep); }
    શૂન્ય visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(સાચું);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstConstraintForeach* nodep) override {
        જો (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_પદાવલિ* exprp) { exprp->isSoft(સાચું); });
        }
    }
    શૂન્ય visit(AstEnumDType* nodep) override {
        જો (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_ગાંઠ_DType(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {
        જો (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_ગાંઠ_DType(nodep);
    }
    શૂન્ય visit(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        જો (nodep->rangep()) {
            જો (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t offset_from_init = 0;
            AstEnumItem* addp = nullptr;
            File_લીટી* અચલ flp = nodep->fileline();
            માટે (અચલ પૂર્ણાંક i : nodep->rangep()->seq_ડાબું_To_જમણું()) {
                અચલ string name = nodep->name() + cvtToStr(i);
                Ast_ગાંઠ_પદાવલિ* valuep = nullptr;
                જો (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = નવું AstAdd{flp, nodep->valuep()->clone_વૃક્ષ(સાચું),
                                     નવું AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = Ast_ગાંઠ::addNext(addp, નવું AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        }
    }

    શૂન્ય visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        અચલ AstClass* અચલ classp = VN_CAST(m_modp, Class);
        જો (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(સાચું);
        }
        જો (nodep->embeddedCovergroup()) {
            AstRefDType* અચલ refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        જો (nodep->valuep()) nodep->hasUserInit(સાચું);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        જો (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            જો (VN_IS(m_modp, Module) && m_ftaskp) {
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
            } નહીંતર {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnMore()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        જો (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        જો (!nodep->direction().isAny()) {  // Not a port
            જો (nodep->lifetime().isNone()) {
                જો (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } નહીંતર {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } નહીંતર જો (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            જો (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } નહીંતર {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } નહીંતર જો (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        જો (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            જો (m_beginDepth > 0
                || (m_beginDepth == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        જો (nodep->isGParam() && m_modp) m_modp->hasGParam(સાચું);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        અચલ AstBasicDType* અચલ basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        જો (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(સાચું);

        જો (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        જો (nodep->isParam() && m_inInterface) nodep->isIfaceParam(સાચું);
        જો (AstParseTypeDType* અચલ ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_ગાંઠ* dtypep = nodep->valuep();
            જો (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                જો (Ast_ગાંઠ_પદાવલિ* exprp = VN_CAST(dtypep, ગાંઠ_પદાવલિ)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    જ્યારે (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        જો (!rhsDotp) તોડો;
                        File_લીટી* અચલ fl = dotp->fileline();
                        અચલ તાર્કિક colon = dotp->colon();
                        Ast_ગાંઠ_પદાવલિ* અચલ lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
                        Ast_ગાંઠ_પદાવલિ* અચલ rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
                        Ast_ગાંઠ_પદાવલિ* અચલ rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
                        File_લીટી* અચલ rhsFl = rhsDotp->fileline();
                        અચલ તાર્કિક rhsColon = rhsDotp->colon();
                        AstDot* અચલ newLhs = નવું AstDot{fl, colon, lhs, rhsLhs};
                        exprp = નવું AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_વૃક્ષ(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } નહીંતર {
                dtypep = નવું AstVoidDType{nodep->fileline()};
            }

            Ast_ગાંઠ* અચલ newp = નવું AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, નવું AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(ખોટું), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        જો (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            જો (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(સાચું);
            } નહીંતર જો (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(સાચું);
            } નહીંતર જો (m_modp && v3Global.opt.publicDepth()) {
                જો ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(સાચું);
                } નહીંતર જો (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(સાચું);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        જો (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(ખોટું);
            nodep->trace(ખોટું);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        જો (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, સાચું);
        // Compute initial value
        જો (nodep->valuep() || nodep->needsCReset()) {
            File_લીટી* અચલ fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> Ast_ગાંઠ_પદાવલિ* {
                જો (nodep->valuep()) {
                    પાછા VN_AS(nodep->valuep()->unlinkFrBack(), ગાંઠ_પદાવલિ);
                } નહીંતર {
                    પાછા નવું AstCReset{fl, nodep, ખોટું};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            જો (m_portDups.find(nodep) != m_portDups.end()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } નહીંતર જો (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } નહીંતર જો (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_આગત()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                જો (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlinkFrBack()->delete_વૃક્ષ();
                }
            } નહીંતર જો (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(સાચું);
                File_લીટી* અચલ newfl = નવું File_લીટી{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, સાચું);
                AstAssign* અચલ assp
                    = નવું AstAssign{newfl, નવું AstParseRef{newfl, nodep->name()}, createValuep()};
                જો (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = Ast_ગાંઠ::addNext(m_blockAddAutomaticStmtp, assp);
                } નહીંતર {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = Ast_ગાંઠ::addNext(m_blockAddStaticStmtp, assp);
                }
            } નહીંતર જો (m_કિંમત_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                જો (nodep->valuep()) {
                    nodep->noCReset(સાચું);
                    File_લીટી* અચલ newfl = નવું File_લીટી{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, સાચું);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, સાચું);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* અચલ assp = નવું AstAssign{
                        newfl, નવું AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_ગાંઠ* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    જો (nodep->lifetime().isAutomatic()) {
                        newInitp = નવું AstInitialAutomatic{newfl, assp};
                    } નહીંતર {
                        newInitp = નવું AstInitialStatic{newfl, assp};
                    }
                    nodep->addNextHere(newInitp);
                }
            } નહીંતર જો (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    સ્થિર તાર્કિક isNegative(અચલ AstConst* constp) {
        પાછા constp->isSigned() && constp->અંક().bitIs1(constp->width() - 1);
    }
    શૂન્ય visit(AstRange* nodep) override {
        સ્થિર constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        અચલ AstConst* અચલ leftp = VN_CAST(nodep->leftp(), Const);
        અચલ AstConst* અચલ rightp = VN_CAST(nodep->rightp(), Const);
        જો (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            અચલ uint64_t ડાબું = leftp->toUQuad();
            અચલ uint64_t જમણું = rightp->toUQuad();
            અચલ uint64_t span = std::max(ડાબું, જમણું) - std::min(ડાબું, જમણું);
            જો (span >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << span + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstConst* nodep) override {
        જો (nodep->અંક().autoExtend() && nodep->fileline()->language() < V3LangCode::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    શૂન્ય visit(AstAttrOf* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        જો (nodep->attrType() == VAttrType::DT_PUBLIC) {
            AstTypedef* અચલ typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(સાચું);
            m_varp->sigModPublic(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_SPLIT_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            જો (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } નહીંતર {
                m_varp->attrSplitVar(સાચું);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(સાચું);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(સાચું);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(સાચું);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(સાચું);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(સાચું);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(સાચું);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(સાચું);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        }
    }

    શૂન્ય visit(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = nullptr;
        અચલ ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        જો (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } નહીંતર {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_ગાંઠ* backp = nodep->backp();
            માટે (; backp; backp = backp->backp()) {
                જો (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, ગાંઠ_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    તોડો;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_ગાંઠ_DType* અચલ dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            જો (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
                પાછા;
            } નહીંતર {
                defp = નવું AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.size()));
                UINFO(9, "New impltypedef " << defp);
                backp->addNextHere(defp);
            }
        }
        nodep->replaceWith(નવું AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }

    શૂન્ય visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Foreach* nodep) override {
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
        AstForeachHeader* અચલ headerp = nodep->headerp();
        જો (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstLoop* nodep) override {
        cleanFileline(nodep);
        જો (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } નહીંતર {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        જો (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_લીટી* અચલ newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, સાચું);
            nodep->fileline(newfl);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        જો (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_inInterface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_અંક);
        VL_RESTORER(m_beginDepth);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_અંક);
        VL_RESTORER(m_કિંમત_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        જો (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        જો (VN_IS(nodep, Iface)) m_inInterface = સાચું;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_અંક = 0;
        m_beginDepth = 0;
        m_કિંમત_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = ખોટું;
        m_rand_Sequence_અંક = 0;

        જો (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        જો (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        જો (AstModule* અચલ modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    શૂન્ય visit_Iterate_No_કિંમત_Mod(Ast_ગાંઠ* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_કિંમત_Modp);
        m_કિંમત_Modp = nullptr;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = સાચું;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        visit_Iterate_No_કિંમત_Mod(nodep);
    }
    શૂન્ય visit(AstCover* nodep) override { visit_Iterate_No_કિંમત_Mod(nodep); }
    શૂન્ય visit(AstRestrict* nodep) override { visit_Iterate_No_કિંમત_Mod(nodep); }

    શૂન્ય visit(AstGenBlock* nodep) override {
        V3Control::applyCoverageBlock(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_beginDepth);
        ++m_beginDepth;
        અચલ Ast_ગાંઠ* અચલ backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        અચલ તાર્કિક nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        પૂર્ણાંક assign_Gen_Blk_અંક = -1;
        જો (nodep->genforp()) {
            ++m_genblk_અંક;
            જો (nodep->name() == "") assign_Gen_Blk_અંક = m_genblk_અંક;
        } નહીંતર જો (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            assign_Gen_Blk_અંક = m_genblkAbove;
        }
        જો (assign_Gen_Blk_અંક != -1) {
            nodep->name("genblk" + cvtToStr(assign_Gen_Blk_અંક));
            જો (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warnMore()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        જો (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_અંક);
            m_genblkAbove = 0;
            m_genblk_અંક = 0;
            iterateChildren(nodep);
        } નહીંતર {
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstGenCase* nodep) override {
        ++m_genblk_અંક;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_અંક);
        m_genblkAbove = m_genblk_અંક;
        m_genblk_અંક = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        અચલ તાર્કિક nestedIf = (VN_IS(nodep->backp(), GenBlock)
                               && nestedIfBegin(VN_CAST(nodep->backp(), GenBlock)));
        જો (nestedIf) {
            iterateChildren(nodep);
        } નહીંતર {
            ++m_genblk_અંક;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_અંક);
            m_genblkAbove = m_genblk_અંક;
            m_genblk_અંક = 0;
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstCell* nodep) override {
        જો (nodep->origName().empty()) {
            જો (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            અચલ string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Block* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterateAndNextNull(nodep->declsp());
        }

        જો (AstBegin* અચલ beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterateAndNextNull(nodep->stmtsp());
        જો (AstFork* અચલ forkp = VN_CAST(nodep, Fork)) iterateAndNextNull(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_ગાંઠ* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = Ast_ગાંઠ::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_ગાંઠ::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    શૂન્ય visit(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        જો (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        જો (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(સાચું);
            Ast_ગાંઠ* અચલ nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->addNext(nodep);
        }
    }
    શૂન્ય visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        જો (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_ગાંઠ* scanp = nodep;
            // Skip over the New's statement
            માટે (; scanp && !VN_IS(scanp, Stmt_પદાવલિ); scanp = scanp->backp()) {}
            જો (VN_IS(scanp, Stmt_પદાવલિ)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                માટે (; scanp; scanp = scanp->backp()) {
                    જો (VN_IS(scanp, ગાંઠ_Stmt) || VN_IS(scanp, ગાંઠ_Module)
                        || VN_IS(scanp, ગાંઠ_FTask))
                        તોડો;
                }
                જો (!VN_IS(scanp, ગાંઠ_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    શૂન્ય visit(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    શૂન્ય visit(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::to_string(m_rand_Sequence_અંક++));
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    શૂન્ય visit(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    શૂન્ય visit(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    શૂન્ય visit(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    શૂન્ય visit(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    શૂન્ય visit(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    શૂન્ય visit(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* અચલ alwaysp = VN_CAST(nodep->backp(), Always);
        જો (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        } નહીંતર જો (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            જો (Ast_Sen_વૃક્ષ* અચલ sentreep = nodep->sentreep()) {
                sentreep->unlinkFrBackWithNext();
                alwaysp->sentreep(sentreep);
            }
            જો (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
        }
    }
    શૂન્ય visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        માટે (Ast_ગાંઠ *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            જો (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                જો (citemp->exprp() || citemp->assignp()) ચાલુ;
                જો (citemp->skewp()) {
                    જો (citemp->direction() == VDirection::આગત) {
                        // Disallow default redefinition; note some simulators allow this
                        જો (m_defaultInSkewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_defaultInSkewp = citemp->skewp();
                    } નહીંતર જો (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        જો (m_defaultOutSkewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_defaultOutSkewp = citemp->skewp();
                    } નહીંતર {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(ધકેલો_Deletep(citemp->unlinkFrBack()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        જો (nodep->direction() == VDirection::OUTPUT) {
            જો (!nodep->skewp()) {
                જો (m_defaultOutSkewp) {
                    nodep->skewp(m_defaultOutSkewp->clone_વૃક્ષ(ખોટું));
                } નહીંતર {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(નવું AstConst{nodep->fileline(), 0});
                }
            }
        } નહીંતર જો (nodep->direction() == VDirection::આગત) {
            જો (!nodep->skewp()) {
                જો (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->clone_વૃક્ષ(ખોટું));
                } નહીંતર {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(નવું AstConst{nodep->fileline(), AstConst::OneStep{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        જો (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    શૂન્ય visit(Ast_Tagged_પદાવલિ* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstTaggedPattern* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPatternVar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPatternStar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        જો (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    સ્થિર Ast_ગાંઠ* makeArgMemberCopies(Ast_ગાંઠ* argsp, તાર્કિક readOnlyRefs) {
        Ast_ગાંઠ* resultp = nullptr;
        માટે (Ast_ગાંઠ* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* અચલ origVarp = VN_AS(argp, Var);
            AstVar* અચલ paramp = origVarp->clone_વૃક્ષ(ખોટું);
            File_લીટી* અચલ fl = નવું File_લીટી{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, સાચું);
            paramp->fileline(fl);
            paramp->funcLocal(સાચું);
            paramp->direction(origVarp->direction());
            જો (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_ગાંઠ::addNextNull(resultp, paramp);
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = નવું AstDot{
                origVarp->fileline(), ખોટું, નવું AstParseRef{origVarp->fileline(), "this"},
                નવું AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstParseRef{paramp->fileline(), paramp->name()};
            resultp->addNext(નવું AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        પાછા resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    સ્થિર શૂન્ય createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_ગાંઠ* argsp,
                                        Ast_ગાંઠ* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* અચલ defaultVarp
            = નવું AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        જો (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, સાચું));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* અચલ varp
                = નવું AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             નવું AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             નવું AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* અચલ varp
                = નવું AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             નવું AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             નવું AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* અચલ funcp = નવું AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, ખોટું));
            funcp->classMethod(સાચું);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(સાચું);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        માટે (અચલ string& name : {"start"s, "stop"s}) {
            AstFunc* અચલ funcp = નવું AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(સાચું);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        માટે (અચલ string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* અચલ funcp = નવું AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, સાચું);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(સાચું);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* અચલ varp = નવું AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(સાચું);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(સાચું);
                funcp->fvarp(varp);
            }
            માટે (અચલ string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* અચલ varp = નવું AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(સાચું);
                varp->direction(VDirection::આગત);
                varp->valuep(નવું AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* અચલ funcp
                = નવું AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(સાચું);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* અચલ varp = નવું AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(સાચું);
            varp->direction(VDirection::આગત);
            funcp->addStmtsp(varp);
        }
    }

    તાર્કિક dropDeprecatedCoverageOption(AstCgOptionAssign* અચલ nodep) {
        જો (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) પાછા ખોટું;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        પાછા સાચું;
    }

    શૂન્ય visit(AstCgOptionAssign* nodep) override {
        જો (dropDeprecatedCoverageOption(nodep)) પાછા;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        જો (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) પાછા;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        જો (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        અચલ string libname = m_modp->libname();
        AstClass* અચલ cgClassp = નવું AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, સાચું);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, સાચું);
        cgClassp->isCovergroup(સાચું);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(સાચું);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        જો (Ast_Sen_વૃક્ષ* અચલ eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* અચલ cgNodep = નવું AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        માટે (Ast_ગાંઠ* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* અચલ origVarp = VN_AS(argp, Var);
            જો (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::આગત);
            }
            જો ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* અચલ memberp = origVarp->clone_વૃક્ષ(ખોટું);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ખોટું);
            memberp->direction(VDirection::NONE);
            જો (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(સાચું);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        માટે (Ast_ગાંઠ* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* અચલ origVarp = VN_AS(argp, Var);
            જો (!origVarp->is_આગત()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::આગત);
            }
            AstVar* અચલ memberp = origVarp->clone_વૃક્ષ(ખોટું);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(ખોટું);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* અચલ newFuncp = નવું AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, સાચું);
        newFuncp->classMethod(સાચું);
        newFuncp->isConstructor(સાચું);
        newFuncp->dtypep(cgClassp->dtypep());
        જો (Ast_ગાંઠ* અચલ bodyp = nodep->membersp()) {
            bodyp->unlinkFrBackWithNext();
            newFuncp->addStmtsp(bodyp);
        }
        cgClassp->addMembersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_અંક);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_અંક = 0;
        માટે (અચલ Ast_ગાંઠ* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (અચલ AstCoverpoint* અચલ pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    સ્થિર string coverpointName(અચલ AstCoverpoint* nodep) {
        જો (!nodep->name().empty()) પાછા nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        અચલ AstParseRef* અચલ refp = VN_CAST(nodep->exprp(), ParseRef);
        પાછા refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    અચલ AstCoverpoint* findCoverpoint(અચલ AstCoverpointRef* refp) અચલ {
        જો (refp->exprp()) પાછા nullptr;  // A hierarchical reference, unsupported
        અચલ auto it = m_coverpoints.find(refp->name());
        પાછા it == m_coverpoints.end() ? nullptr : it->second;
    }

    શૂન્ય visit(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        જો (nodep->name().empty()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            અચલ string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpoint_અંક++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        માટે (Ast_ગાંઠ *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            જો (AstCgOptionAssign* અચલ optp = VN_CAST(itemp, CgOptionAssign)) {
                જો (dropDeprecatedCoverageOption(optp)) ચાલુ;
                optp->unlinkFrBack();
                જો (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(નવું AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_વૃક્ષ(ખોટું)});
                } નહીંતર {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_વૃક્ષ(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        જો (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        જો (અચલ AstCoverpointRef* અચલ refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            જો (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_લીટી* અચલ fl = nodep->fileline();
        File_લીટી* અચલ flNoWarn = નવું File_લીટી{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, સાચું);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, સાચું);
        AstVar* અચલ varp = નવું AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        નવું AstRefDType{fl, AstRefDType::Flag_Type_Of_પદાવલિ{},
                                                        m_coverpointp->exprp()->clone_વૃક્ષ(ખોટું)}};
        varp->funcLocal(સાચું);
        varp->noReset(સાચું);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* અચલ refp = nodep->pointp();
        અચલ AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        અચલ AstParseRef* binp = nullptr;
        અચલ AstDot* અચલ dotp = VN_CAST(refp->exprp(), Dot);
        જો (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        જો (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
            પાછા;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        જો (binp) nodep->name(binp->name());
        refp->replaceWith(નવું AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        જો (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    શૂન્ય visit(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        જો (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    શૂન્ય visit(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        માટે (Ast_ગાંઠ *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            જો (VN_IS(itemp, CoverCrossBin)) ચાલુ;
            AstCgOptionAssign* અચલ optp = VN_AS(itemp, CgOptionAssign);
            જો (dropDeprecatedCoverageOption(optp)) ચાલુ;
            itemp->unlinkFrBack();
            અચલ VCoverOptionType optType = optp->optType();
            જો (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(નવું AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_વૃક્ષ(ખોટું)});
            VL_DO_DANGLING(optp->delete_વૃક્ષ(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        જો (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_લીટી* અચલ fl = nodep->fileline();
            File_લીટી* અચલ flNoWarn = નવું File_લીટી{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, સાચું);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, સાચું);
            std::set<string> names;
            માટે (અચલ Ast_ગાંઠ* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                અચલ AstCoverpointRef* અચલ refp = VN_AS(itemp, CoverpointRef);
                અચલ AstCoverpoint* અચલ pointp = findCoverpoint(refp);
                જો (!pointp || !names.ઉમેરો(refp->name()).second) ચાલુ;
                AstVar* અચલ varp
                    = નવું AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 નવું AstRefDType{fl, AstRefDType::Flag_Type_Of_પદાવલિ{},
                                                 pointp->exprp()->clone_વૃક્ષ(ખોટું)}};
                varp->funcLocal(સાચું);
                varp->noReset(સાચું);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        જો (!nodep->selectp()) VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    શૂન્ય visit(AstSystemCSection* nodep) override {
        જો (nodep->user1SetOnce()) પાછા;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::addStatSum(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

શૂન્ય V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("linkparse", 0, dump_વૃક્ષ_Either_Level() >= 6);
}
