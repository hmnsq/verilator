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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkParse.h"

#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

വർഗ്ഗം LinkParseVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<File_വരി*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    Ast_കണ്ണി_Module* m_മൂല്യം_Modp = nullptr;
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    തർക്കം m_inInterface = അസത്യം;  // True when inside interface declaration
    Ast_കണ്ണി_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    Ast_കണ്ണി_Block* m_blockp = nullptr;  // Current AstNodeBlock
    Ast_കണ്ണി_Stmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    Ast_കണ്ണി_Stmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    Ast_കണ്ണി_DType* m_dtypep = nullptr;  // Current data type
    Ast_കണ്ണി_വ്യഞ്ജകം* m_defaultInSkewp = nullptr;  // Current default input skew
    Ast_കണ്ണി_വ്യഞ്ജകം* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, സ്ഥിരം AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    പൂർണ്ണസംഖ്യ m_anonUdpId = 0;  // Counter for anonymous UDP instances
    പൂർണ്ണസംഖ്യ m_coverpoint_അക്കം = 0;  // Counter for unnamed coverpoints within current covergroup
    പൂർണ്ണസംഖ്യ m_genblkAbove = 0;  // Begin block number of if/case/for above
    പൂർണ്ണസംഖ്യ m_genblk_അക്കം = 0;  // Begin block number, 0=none seen
    പൂർണ്ണസംഖ്യ m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    പൂർണ്ണസംഖ്യ m_rand_Sequence_അക്കം = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    തർക്കം m_lifetimeAllowed = അസത്യം;  // True to allow lifetime settings
    തർക്കം m_hasTimingControl = അസത്യം;  // If current task has timing control
    തർക്കം m_moduleWithGenericIface = അസത്യം;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    ശൂന്യം cleanFileline(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (nodep->user2SetOnce()) തിരികെ;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        എങ്കിൽ (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(പുതിയ File_വരി{nodep->fileline()});
        }
        m_filelines.തിരുകുക(nodep->fileline());
    }

    string nameFromTypedef(Ast_കണ്ണി* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        എങ്കിൽ (സ്ഥിരം AstTypedef* സ്ഥിരം typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            എങ്കിൽ (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } അല്ലെങ്കിൽ എങ്കിൽ (m_modp) {
                above = m_modp->name() + ".";
            }
            തിരികെ above + typedefp->name();
        }
        തിരികെ "";
    }

    ശൂന്യം visit_Iterate_കണ്ണി_DType(Ast_കണ്ണി_DType* nodep) {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    തർക്കം nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം backp = nodep->backp();
        തിരികെ (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    ശൂന്യം checkIndent(Ast_കണ്ണി* nodep, Ast_കണ്ണി* childp) {
        // Try very hard to avoid false positives
        Ast_കണ്ണി* nextp = nodep->nextp();
        എങ്കിൽ (!childp) തിരികെ;
        എങ്കിൽ (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        എങ്കിൽ (!nextp) തിരികെ;
        എങ്കിൽ (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) തിരികെ;
        File_വരി* സ്ഥിരം കണ്ണി_Flp = nodep->fileline();
        File_വരി* സ്ഥിരം childFlp = childp->fileline();
        File_വരി* സ്ഥിരം nextFlp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        എങ്കിൽ (കണ്ണി_Flp->filenameno() != childFlp->filenameno()) തിരികെ;
        എങ്കിൽ (കണ്ണി_Flp->filenameno() != nextFlp->filenameno()) തിരികെ;
        എങ്കിൽ (കണ്ണി_Flp->lastLineno() >= childFlp->firstLineno()) തിരികെ;
        എങ്കിൽ (childFlp->lastLineno() >= nextFlp->firstLineno()) തിരികെ;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        എങ്കിൽ (!(കണ്ണി_Flp->firstColumn() < childFlp->firstColumn()
              && nextFlp->firstColumn() >= childFlp->firstColumn()))
            തിരികെ;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        സ്ഥിരം std::string കണ്ണി_Prefix = കണ്ണി_Flp->sourcePrefix(കണ്ണി_Flp->firstColumn());
        സ്ഥിരം std::string childPrefix = childFlp->sourcePrefix(കണ്ണി_Flp->firstColumn());
        സ്ഥിരം std::string nextPrefix = nextFlp->sourcePrefix(കണ്ണി_Flp->firstColumn());
        എങ്കിൽ (childPrefix != കണ്ണി_Prefix) തിരികെ;
        എങ്കിൽ (nextPrefix != childPrefix) തിരികെ;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        സ്ഥിരം std::string കണ്ണി_Source = കണ്ണി_Flp->source();
        സ്ഥിരം std::string childSource = childFlp->source();
        സ്ഥിരം std::string nextSource = nextFlp->source();
        എങ്കിൽ (!(VString::leading_Whitespace_എണ്ണം(കണ്ണി_Source)
                  < VString::leading_Whitespace_എണ്ണം(childSource)
              && VString::leading_Whitespace_എണ്ണം(nextSource)
                     >= VString::leading_Whitespace_എണ്ണം(childSource)))
            തിരികെ;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    ശൂന്യം collectPorts(Ast_കണ്ണി* കണ്ണി_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        വേണ്ടി (Ast_കണ്ണി* nodep = കണ്ണി_Listp; nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                എങ്കിൽ (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        വേണ്ടി (Ast_കണ്ണി* nodep = കണ്ണി_Listp; nodep; nodep = nodep->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
                എങ്കിൽ (varp->isIO()) തുടരുക;
                സ്ഥിരം auto it = portNames.find(varp->name());
                എങ്കിൽ (it == portNames.end()) തുടരുക;
                AstVar* സ്ഥിരം iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                എങ്കിൽ (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlinkFrBackWithNext());
                m_portDups.emplace(varp);
            }
        }
    }

    Ast_കണ്ണി* getVarsAndUnlink(Ast_കണ്ണി* stmtsp) {
        Ast_കണ്ണി* varsp = nullptr;
        വേണ്ടി (Ast_കണ്ണി *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            എങ്കിൽ (VN_IS(itemp, Var)) varsp = Ast_കണ്ണി::addNext(varsp, itemp->unlinkFrBack());
        }
        തിരികെ varsp;
    }
    Ast_കണ്ണി_Stmt* getBlockAdds() {
        Ast_കണ്ണി_Stmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        എങ്കിൽ (m_blockAddStaticStmtp)
            addsp = Ast_കണ്ണി::addNext(addsp,
                                     പുതിയ AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        എങ്കിൽ (m_blockAddAutomaticStmtp)
            addsp = Ast_കണ്ണി::addNext(
                addsp, പുതിയ AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        തിരികെ addsp;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process only once.
        // Mark class methods
        എങ്കിൽ (VN_IS(m_modp, Class)) nodep->classMethod(സത്യം);

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
        m_lifetimeAllowed = സത്യം;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep) { തിരികെ nodep->isTimingControl(); });
        എങ്കിൽ (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->classMethod()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        എങ്കിൽ (nodep->classMethod() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warnMore() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_കണ്ണി* stmtsp = nullptr;
        എങ്കിൽ (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = Ast_കണ്ണി::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_കണ്ണി::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    ശൂന്യം visit(Ast_കണ്ണി_DType* nodep) override { visit_Iterate_കണ്ണി_DType(nodep); }
    ശൂന്യം visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(സത്യം);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstConstraintForeach* nodep) override {
        എങ്കിൽ (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_വ്യഞ്ജകം* exprp) { exprp->isSoft(സത്യം); });
        }
    }
    ശൂന്യം visit(AstEnumDType* nodep) override {
        എങ്കിൽ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_കണ്ണി_DType(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* nodep) override {
        എങ്കിൽ (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_കണ്ണി_DType(nodep);
    }
    ശൂന്യം visit(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (nodep->rangep()) {
            എങ്കിൽ (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t offset_from_init = 0;
            AstEnumItem* addp = nullptr;
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ i : nodep->rangep()->seq_ഇടത്_To_വലത്()) {
                സ്ഥിരം string name = nodep->name() + cvtToStr(i);
                Ast_കണ്ണി_വ്യഞ്ജകം* valuep = nullptr;
                എങ്കിൽ (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = പുതിയ AstAdd{flp, nodep->valuep()->clone_വൃക്ഷം(സത്യം),
                                     പുതിയ AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = Ast_കണ്ണി::addNext(addp, പുതിയ AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        }
    }

    ശൂന്യം visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(m_modp, Class);
        എങ്കിൽ (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(സത്യം);
        }
        എങ്കിൽ (nodep->embeddedCovergroup()) {
            AstRefDType* സ്ഥിരം refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        എങ്കിൽ (nodep->valuep()) nodep->hasUserInit(സത്യം);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        എങ്കിൽ (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            എങ്കിൽ (VN_IS(m_modp, Module) && m_ftaskp) {
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
            } അല്ലെങ്കിൽ {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnMore()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        എങ്കിൽ (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        എങ്കിൽ (!nodep->direction().isAny()) {  // Not a port
            എങ്കിൽ (nodep->lifetime().isNone()) {
                എങ്കിൽ (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } അല്ലെങ്കിൽ {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            എങ്കിൽ (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } അല്ലെങ്കിൽ {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        എങ്കിൽ (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            എങ്കിൽ (m_beginDepth > 0
                || (m_beginDepth == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        എങ്കിൽ (nodep->isGParam() && m_modp) m_modp->hasGParam(സത്യം);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        എങ്കിൽ (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(സത്യം);

        എങ്കിൽ (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        എങ്കിൽ (nodep->isParam() && m_inInterface) nodep->isIfaceParam(സത്യം);
        എങ്കിൽ (AstParseTypeDType* സ്ഥിരം ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_കണ്ണി* dtypep = nodep->valuep();
            എങ്കിൽ (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* exprp = VN_CAST(dtypep, കണ്ണി_വ്യഞ്ജകം)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    ആകുമ്പോൾ (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        എങ്കിൽ (!rhsDotp) നിർത്തുക;
                        File_വരി* സ്ഥിരം fl = dotp->fileline();
                        സ്ഥിരം തർക്കം colon = dotp->colon();
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
                        File_വരി* സ്ഥിരം rhsFl = rhsDotp->fileline();
                        സ്ഥിരം തർക്കം rhsColon = rhsDotp->colon();
                        AstDot* സ്ഥിരം newLhs = പുതിയ AstDot{fl, colon, lhs, rhsLhs};
                        exprp = പുതിയ AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_വൃക്ഷം(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } അല്ലെങ്കിൽ {
                dtypep = പുതിയ AstVoidDType{nodep->fileline()};
            }

            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, പുതിയ AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(അസത്യം), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        എങ്കിൽ (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            എങ്കിൽ (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(സത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(സത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (m_modp && v3Global.opt.publicDepth()) {
                എങ്കിൽ ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(സത്യം);
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(സത്യം);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        എങ്കിൽ (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(അസത്യം);
            nodep->trace(അസത്യം);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        എങ്കിൽ (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, സത്യം);
        // Compute initial value
        എങ്കിൽ (nodep->valuep() || nodep->needsCReset()) {
            File_വരി* സ്ഥിരം fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                എങ്കിൽ (nodep->valuep()) {
                    തിരികെ VN_AS(nodep->valuep()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
                } അല്ലെങ്കിൽ {
                    തിരികെ പുതിയ AstCReset{fl, nodep, അസത്യം};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            എങ്കിൽ (m_portDups.find(nodep) != m_portDups.end()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_നിവേശം()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                എങ്കിൽ (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlinkFrBack()->delete_വൃക്ഷം();
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(സത്യം);
                File_വരി* സ്ഥിരം newfl = പുതിയ File_വരി{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, സത്യം);
                AstAssign* സ്ഥിരം assp
                    = പുതിയ AstAssign{newfl, പുതിയ AstParseRef{newfl, nodep->name()}, createValuep()};
                എങ്കിൽ (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = Ast_കണ്ണി::addNext(m_blockAddAutomaticStmtp, assp);
                } അല്ലെങ്കിൽ {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = Ast_കണ്ണി::addNext(m_blockAddStaticStmtp, assp);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (m_മൂല്യം_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                എങ്കിൽ (nodep->valuep()) {
                    nodep->noCReset(സത്യം);
                    File_വരി* സ്ഥിരം newfl = പുതിയ File_വരി{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, സത്യം);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, സത്യം);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* സ്ഥിരം assp = പുതിയ AstAssign{
                        newfl, പുതിയ AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_കണ്ണി* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    എങ്കിൽ (nodep->lifetime().isAutomatic()) {
                        newInitp = പുതിയ AstInitialAutomatic{newfl, assp};
                    } അല്ലെങ്കിൽ {
                        newInitp = പുതിയ AstInitialStatic{newfl, assp};
                    }
                    nodep->addNextHere(newInitp);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    സ്ഥിതം തർക്കം isNegative(സ്ഥിരം AstConst* constp) {
        തിരികെ constp->isSigned() && constp->അക്കം().bitIs1(constp->width() - 1);
    }
    ശൂന്യം visit(AstRange* nodep) override {
        സ്ഥിതം constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        സ്ഥിരം AstConst* സ്ഥിരം leftp = VN_CAST(nodep->leftp(), Const);
        സ്ഥിരം AstConst* സ്ഥിരം rightp = VN_CAST(nodep->rightp(), Const);
        എങ്കിൽ (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            സ്ഥിരം uint64_t ഇടത് = leftp->toUQuad();
            സ്ഥിരം uint64_t വലത് = rightp->toUQuad();
            സ്ഥിരം uint64_t span = std::max(ഇടത്, വലത്) - std::min(ഇടത്, വലത്);
            എങ്കിൽ (span >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << span + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstConst* nodep) override {
        എങ്കിൽ (nodep->അക്കം().autoExtend() && nodep->fileline()->language() < V3LangCode::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    ശൂന്യം visit(AstAttrOf* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (nodep->attrType() == VAttrType::DT_PUBLIC) {
            AstTypedef* സ്ഥിരം typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(സത്യം);
            m_varp->sigModPublic(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_SPLIT_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            എങ്കിൽ (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } അല്ലെങ്കിൽ {
                m_varp->attrSplitVar(സത്യം);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(സത്യം);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(സത്യം);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(സത്യം);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(സത്യം);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(സത്യം);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(സത്യം);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(സത്യം);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        }
    }

    ശൂന്യം visit(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = nullptr;
        സ്ഥിരം ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        എങ്കിൽ (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } അല്ലെങ്കിൽ {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_കണ്ണി* backp = nodep->backp();
            വേണ്ടി (; backp; backp = backp->backp()) {
                എങ്കിൽ (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, കണ്ണി_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    നിർത്തുക;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            എങ്കിൽ (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ {
                defp = പുതിയ AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.size()));
                UINFO(9, "New impltypedef " << defp);
                backp->addNextHere(defp);
            }
        }
        nodep->replaceWith(പുതിയ AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }

    ശൂന്യം visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_Foreach* nodep) override {
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
        AstForeachHeader* സ്ഥിരം headerp = nodep->headerp();
        എങ്കിൽ (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstLoop* nodep) override {
        cleanFileline(nodep);
        എങ്കിൽ (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } അല്ലെങ്കിൽ {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_വരി* സ്ഥിരം newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, സത്യം);
            nodep->fileline(newfl);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        എങ്കിൽ (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_inInterface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_അക്കം);
        VL_RESTORER(m_beginDepth);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_അക്കം);
        VL_RESTORER(m_മൂല്യം_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        എങ്കിൽ (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        എങ്കിൽ (VN_IS(nodep, Iface)) m_inInterface = സത്യം;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_അക്കം = 0;
        m_beginDepth = 0;
        m_മൂല്യം_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = അസത്യം;
        m_rand_Sequence_അക്കം = 0;

        എങ്കിൽ (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        എങ്കിൽ (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        എങ്കിൽ (AstModule* സ്ഥിരം modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    ശൂന്യം visit_Iterate_No_മൂല്യം_Mod(Ast_കണ്ണി* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_മൂല്യം_Modp);
        m_മൂല്യം_Modp = nullptr;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = സത്യം;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        visit_Iterate_No_മൂല്യം_Mod(nodep);
    }
    ശൂന്യം visit(AstCover* nodep) override { visit_Iterate_No_മൂല്യം_Mod(nodep); }
    ശൂന്യം visit(AstRestrict* nodep) override { visit_Iterate_No_മൂല്യം_Mod(nodep); }

    ശൂന്യം visit(AstGenBlock* nodep) override {
        V3Control::applyCoverageBlock(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_beginDepth);
        ++m_beginDepth;
        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        സ്ഥിരം തർക്കം nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        പൂർണ്ണസംഖ്യ assign_Gen_Blk_അക്കം = -1;
        എങ്കിൽ (nodep->genforp()) {
            ++m_genblk_അക്കം;
            എങ്കിൽ (nodep->name() == "") assign_Gen_Blk_അക്കം = m_genblk_അക്കം;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            assign_Gen_Blk_അക്കം = m_genblkAbove;
        }
        എങ്കിൽ (assign_Gen_Blk_അക്കം != -1) {
            nodep->name("genblk" + cvtToStr(assign_Gen_Blk_അക്കം));
            എങ്കിൽ (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warnMore()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        എങ്കിൽ (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_അക്കം);
            m_genblkAbove = 0;
            m_genblk_അക്കം = 0;
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstGenCase* nodep) override {
        ++m_genblk_അക്കം;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_അക്കം);
        m_genblkAbove = m_genblk_അക്കം;
        m_genblk_അക്കം = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        സ്ഥിരം തർക്കം nestedIf = (VN_IS(nodep->backp(), GenBlock)
                               && nestedIfBegin(VN_CAST(nodep->backp(), GenBlock)));
        എങ്കിൽ (nestedIf) {
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            ++m_genblk_അക്കം;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_അക്കം);
            m_genblkAbove = m_genblk_അക്കം;
            m_genblk_അക്കം = 0;
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstCell* nodep) override {
        എങ്കിൽ (nodep->origName().empty()) {
            എങ്കിൽ (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            സ്ഥിരം string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Block* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterateAndNextNull(nodep->declsp());
        }

        എങ്കിൽ (AstBegin* സ്ഥിരം beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterateAndNextNull(nodep->stmtsp());
        എങ്കിൽ (AstFork* സ്ഥിരം forkp = VN_CAST(nodep, Fork)) iterateAndNextNull(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_കണ്ണി* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = Ast_കണ്ണി::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_കണ്ണി::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    ശൂന്യം visit(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        എങ്കിൽ (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(സത്യം);
            Ast_കണ്ണി* സ്ഥിരം nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->addNext(nodep);
        }
    }
    ശൂന്യം visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_കണ്ണി* scanp = nodep;
            // Skip over the New's statement
            വേണ്ടി (; scanp && !VN_IS(scanp, Stmt_വ്യഞ്ജകം); scanp = scanp->backp()) {}
            എങ്കിൽ (VN_IS(scanp, Stmt_വ്യഞ്ജകം)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                വേണ്ടി (; scanp; scanp = scanp->backp()) {
                    എങ്കിൽ (VN_IS(scanp, കണ്ണി_Stmt) || VN_IS(scanp, കണ്ണി_Module)
                        || VN_IS(scanp, കണ്ണി_FTask))
                        നിർത്തുക;
                }
                എങ്കിൽ (!VN_IS(scanp, കണ്ണി_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    ശൂന്യം visit(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    ശൂന്യം visit(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::to_string(m_rand_Sequence_അക്കം++));
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ശൂന്യം visit(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ശൂന്യം visit(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ശൂന്യം visit(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ശൂന്യം visit(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ശൂന്യം visit(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    ശൂന്യം visit(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* സ്ഥിരം alwaysp = VN_CAST(nodep->backp(), Always);
        എങ്കിൽ (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            എങ്കിൽ (Ast_Sen_വൃക്ഷം* സ്ഥിരം sentreep = nodep->sentreep()) {
                sentreep->unlinkFrBackWithNext();
                alwaysp->sentreep(sentreep);
            }
            എങ്കിൽ (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        }
    }
    ശൂന്യം visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        വേണ്ടി (Ast_കണ്ണി *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            എങ്കിൽ (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                എങ്കിൽ (citemp->exprp() || citemp->assignp()) തുടരുക;
                എങ്കിൽ (citemp->skewp()) {
                    എങ്കിൽ (citemp->direction() == VDirection::നിവേശം) {
                        // Disallow default redefinition; note some simulators allow this
                        എങ്കിൽ (m_defaultInSkewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_defaultInSkewp = citemp->skewp();
                    } അല്ലെങ്കിൽ എങ്കിൽ (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        എങ്കിൽ (m_defaultOutSkewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_defaultOutSkewp = citemp->skewp();
                    } അല്ലെങ്കിൽ {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(തള്ളുക_Deletep(citemp->unlinkFrBack()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        എങ്കിൽ (nodep->direction() == VDirection::OUTPUT) {
            എങ്കിൽ (!nodep->skewp()) {
                എങ്കിൽ (m_defaultOutSkewp) {
                    nodep->skewp(m_defaultOutSkewp->clone_വൃക്ഷം(അസത്യം));
                } അല്ലെങ്കിൽ {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(പുതിയ AstConst{nodep->fileline(), 0});
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->direction() == VDirection::നിവേശം) {
            എങ്കിൽ (!nodep->skewp()) {
                എങ്കിൽ (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->clone_വൃക്ഷം(അസത്യം));
                } അല്ലെങ്കിൽ {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(പുതിയ AstConst{nodep->fileline(), AstConst::OneStep{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        എങ്കിൽ (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    ശൂന്യം visit(Ast_Tagged_വ്യഞ്ജകം* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstTaggedPattern* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPatternVar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPatternStar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        എങ്കിൽ (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    സ്ഥിതം Ast_കണ്ണി* makeArgMemberCopies(Ast_കണ്ണി* argsp, തർക്കം readOnlyRefs) {
        Ast_കണ്ണി* resultp = nullptr;
        വേണ്ടി (Ast_കണ്ണി* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* സ്ഥിരം origVarp = VN_AS(argp, Var);
            AstVar* സ്ഥിരം paramp = origVarp->clone_വൃക്ഷം(അസത്യം);
            File_വരി* സ്ഥിരം fl = പുതിയ File_വരി{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, സത്യം);
            paramp->fileline(fl);
            paramp->funcLocal(സത്യം);
            paramp->direction(origVarp->direction());
            എങ്കിൽ (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_കണ്ണി::addNextNull(resultp, paramp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = പുതിയ AstDot{
                origVarp->fileline(), അസത്യം, പുതിയ AstParseRef{origVarp->fileline(), "this"},
                പുതിയ AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstParseRef{paramp->fileline(), paramp->name()};
            resultp->addNext(പുതിയ AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        തിരികെ resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    സ്ഥിതം ശൂന്യം createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_കണ്ണി* argsp,
                                        Ast_കണ്ണി* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* സ്ഥിരം defaultVarp
            = പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        എങ്കിൽ (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, സത്യം));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* സ്ഥിരം varp
                = പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             പുതിയ AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             പുതിയ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* സ്ഥിരം varp
                = പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             പുതിയ AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             പുതിയ AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* സ്ഥിരം funcp = പുതിയ AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, അസത്യം));
            funcp->classMethod(സത്യം);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(സത്യം);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        വേണ്ടി (സ്ഥിരം string& name : {"start"s, "stop"s}) {
            AstFunc* സ്ഥിരം funcp = പുതിയ AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(സത്യം);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        വേണ്ടി (സ്ഥിരം string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* സ്ഥിരം funcp = പുതിയ AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, സത്യം);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(സത്യം);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* സ്ഥിരം varp = പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(സത്യം);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(സത്യം);
                funcp->fvarp(varp);
            }
            വേണ്ടി (സ്ഥിരം string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* സ്ഥിരം varp = പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(സത്യം);
                varp->direction(VDirection::നിവേശം);
                varp->valuep(പുതിയ AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* സ്ഥിരം funcp
                = പുതിയ AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(സത്യം);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* സ്ഥിരം varp = പുതിയ AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(സത്യം);
            varp->direction(VDirection::നിവേശം);
            funcp->addStmtsp(varp);
        }
    }

    തർക്കം dropDeprecatedCoverageOption(AstCgOptionAssign* സ്ഥിരം nodep) {
        എങ്കിൽ (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) തിരികെ അസത്യം;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        തിരികെ സത്യം;
    }

    ശൂന്യം visit(AstCgOptionAssign* nodep) override {
        എങ്കിൽ (dropDeprecatedCoverageOption(nodep)) തിരികെ;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        എങ്കിൽ (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) തിരികെ;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        എങ്കിൽ (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        സ്ഥിരം string libname = m_modp->libname();
        AstClass* സ്ഥിരം cgClassp = പുതിയ AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, സത്യം);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, സത്യം);
        cgClassp->isCovergroup(സത്യം);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(സത്യം);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        എങ്കിൽ (Ast_Sen_വൃക്ഷം* സ്ഥിരം eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* സ്ഥിരം cgNodep = പുതിയ AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        വേണ്ടി (Ast_കണ്ണി* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* സ്ഥിരം origVarp = VN_AS(argp, Var);
            എങ്കിൽ (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::നിവേശം);
            }
            എങ്കിൽ ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* സ്ഥിരം memberp = origVarp->clone_വൃക്ഷം(അസത്യം);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(അസത്യം);
            memberp->direction(VDirection::NONE);
            എങ്കിൽ (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(സത്യം);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        വേണ്ടി (Ast_കണ്ണി* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* സ്ഥിരം origVarp = VN_AS(argp, Var);
            എങ്കിൽ (!origVarp->is_നിവേശം()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::നിവേശം);
            }
            AstVar* സ്ഥിരം memberp = origVarp->clone_വൃക്ഷം(അസത്യം);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(അസത്യം);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* സ്ഥിരം newFuncp = പുതിയ AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, സത്യം);
        newFuncp->classMethod(സത്യം);
        newFuncp->isConstructor(സത്യം);
        newFuncp->dtypep(cgClassp->dtypep());
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം bodyp = nodep->membersp()) {
            bodyp->unlinkFrBackWithNext();
            newFuncp->addStmtsp(bodyp);
        }
        cgClassp->addMembersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_അക്കം);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_അക്കം = 0;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstCoverpoint* സ്ഥിരം pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    സ്ഥിതം string coverpointName(സ്ഥിരം AstCoverpoint* nodep) {
        എങ്കിൽ (!nodep->name().empty()) തിരികെ nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        സ്ഥിരം AstParseRef* സ്ഥിരം refp = VN_CAST(nodep->exprp(), ParseRef);
        തിരികെ refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    സ്ഥിരം AstCoverpoint* findCoverpoint(സ്ഥിരം AstCoverpointRef* refp) സ്ഥിരം {
        എങ്കിൽ (refp->exprp()) തിരികെ nullptr;  // A hierarchical reference, unsupported
        സ്ഥിരം auto it = m_coverpoints.find(refp->name());
        തിരികെ it == m_coverpoints.end() ? nullptr : it->second;
    }

    ശൂന്യം visit(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        എങ്കിൽ (nodep->name().empty()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            സ്ഥിരം string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpoint_അക്കം++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        വേണ്ടി (Ast_കണ്ണി *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            എങ്കിൽ (AstCgOptionAssign* സ്ഥിരം optp = VN_CAST(itemp, CgOptionAssign)) {
                എങ്കിൽ (dropDeprecatedCoverageOption(optp)) തുടരുക;
                optp->unlinkFrBack();
                എങ്കിൽ (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(പുതിയ AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_വൃക്ഷം(അസത്യം)});
                } അല്ലെങ്കിൽ {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_വൃക്ഷം(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        എങ്കിൽ (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        എങ്കിൽ (സ്ഥിരം AstCoverpointRef* സ്ഥിരം refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            എങ്കിൽ (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        File_വരി* സ്ഥിരം flNoWarn = പുതിയ File_വരി{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, സത്യം);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, സത്യം);
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        പുതിയ AstRefDType{fl, AstRefDType::Flag_Type_Of_വ്യഞ്ജകം{},
                                                        m_coverpointp->exprp()->clone_വൃക്ഷം(അസത്യം)}};
        varp->funcLocal(സത്യം);
        varp->noReset(സത്യം);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* സ്ഥിരം refp = nodep->pointp();
        സ്ഥിരം AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        സ്ഥിരം AstParseRef* binp = nullptr;
        സ്ഥിരം AstDot* സ്ഥിരം dotp = VN_CAST(refp->exprp(), Dot);
        എങ്കിൽ (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        എങ്കിൽ (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        എങ്കിൽ (binp) nodep->name(binp->name());
        refp->replaceWith(പുതിയ AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    ശൂന്യം visit(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    ശൂന്യം visit(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        വേണ്ടി (Ast_കണ്ണി *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            എങ്കിൽ (VN_IS(itemp, CoverCrossBin)) തുടരുക;
            AstCgOptionAssign* സ്ഥിരം optp = VN_AS(itemp, CgOptionAssign);
            എങ്കിൽ (dropDeprecatedCoverageOption(optp)) തുടരുക;
            itemp->unlinkFrBack();
            സ്ഥിരം VCoverOptionType optType = optp->optType();
            എങ്കിൽ (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(പുതിയ AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_വൃക്ഷം(അസത്യം)});
            VL_DO_DANGLING(optp->delete_വൃക്ഷം(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        എങ്കിൽ (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_വരി* സ്ഥിരം fl = nodep->fileline();
            File_വരി* സ്ഥിരം flNoWarn = പുതിയ File_വരി{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, സത്യം);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, സത്യം);
            std::set<string> names;
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                സ്ഥിരം AstCoverpointRef* സ്ഥിരം refp = VN_AS(itemp, CoverpointRef);
                സ്ഥിരം AstCoverpoint* സ്ഥിരം pointp = findCoverpoint(refp);
                എങ്കിൽ (!pointp || !names.തിരുകുക(refp->name()).second) തുടരുക;
                AstVar* സ്ഥിരം varp
                    = പുതിയ AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 പുതിയ AstRefDType{fl, AstRefDType::Flag_Type_Of_വ്യഞ്ജകം{},
                                                 pointp->exprp()->clone_വൃക്ഷം(അസത്യം)}};
                varp->funcLocal(സത്യം);
                varp->noReset(സത്യം);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        എങ്കിൽ (!nodep->selectp()) VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ശൂന്യം visit(AstSystemCSection* nodep) override {
        എങ്കിൽ (nodep->user1SetOnce()) തിരികെ;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::addStatSum(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

ശൂന്യം V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("linkparse", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}
