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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LinkParse.h"

#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <set>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

வகுப்பு LinkParseVisitor final : பொது VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<File_வரி*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    Ast_முனை_Module* m_மதிப்பு_Modp = nullptr;
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    தருக்க m_inInterface = பொய்;  // True when inside interface declaration
    Ast_முனை_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    Ast_முனை_Block* m_blockp = nullptr;  // Current AstNodeBlock
    Ast_முனை_Stmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    Ast_முனை_Stmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    Ast_முனை_DType* m_dtypep = nullptr;  // Current data type
    Ast_முனை_கோவை* m_defaultInSkewp = nullptr;  // Current default input skew
    Ast_முனை_கோவை* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, மாறிலி AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    முழு m_anonUdpId = 0;  // Counter for anonymous UDP instances
    முழு m_coverpoint_இலக்கம் = 0;  // Counter for unnamed coverpoints within current covergroup
    முழு m_genblkAbove = 0;  // Begin block number of if/case/for above
    முழு m_genblk_இலக்கம் = 0;  // Begin block number, 0=none seen
    முழு m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    முழு m_rand_Sequence_இலக்கம் = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    தருக்க m_lifetimeAllowed = பொய்;  // True to allow lifetime settings
    தருக்க m_hasTimingControl = பொய்;  // If current task has timing control
    தருக்க m_moduleWithGenericIface = பொய்;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    வெறுமை cleanFileline(Ast_முனை* nodep) {
        எனில் (nodep->user2SetOnce()) திரும்பு;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        எனில் (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(புதிய File_வரி{nodep->fileline()});
        }
        m_filelines.செருகு(nodep->fileline());
    }

    string nameFromTypedef(Ast_முனை* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        எனில் (மாறிலி AstTypedef* மாறிலி typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            எனில் (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } இல்லையெனில் எனில் (m_modp) {
                above = m_modp->name() + ".";
            }
            திரும்பு above + typedefp->name();
        }
        திரும்பு "";
    }

    வெறுமை visit_Iterate_முனை_DType(Ast_முனை_DType* nodep) {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    தருக்க nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        மாறிலி Ast_முனை* மாறிலி backp = nodep->backp();
        திரும்பு (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    வெறுமை checkIndent(Ast_முனை* nodep, Ast_முனை* childp) {
        // Try very hard to avoid false positives
        Ast_முனை* nextp = nodep->nextp();
        எனில் (!childp) திரும்பு;
        எனில் (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        எனில் (!nextp) திரும்பு;
        எனில் (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) திரும்பு;
        File_வரி* மாறிலி முனை_Flp = nodep->fileline();
        File_வரி* மாறிலி childFlp = childp->fileline();
        File_வரி* மாறிலி nextFlp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        எனில் (முனை_Flp->filenameno() != childFlp->filenameno()) திரும்பு;
        எனில் (முனை_Flp->filenameno() != nextFlp->filenameno()) திரும்பு;
        எனில் (முனை_Flp->lastLineno() >= childFlp->firstLineno()) திரும்பு;
        எனில் (childFlp->lastLineno() >= nextFlp->firstLineno()) திரும்பு;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        எனில் (!(முனை_Flp->firstColumn() < childFlp->firstColumn()
              && nextFlp->firstColumn() >= childFlp->firstColumn()))
            திரும்பு;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        மாறிலி std::string முனை_Prefix = முனை_Flp->sourcePrefix(முனை_Flp->firstColumn());
        மாறிலி std::string childPrefix = childFlp->sourcePrefix(முனை_Flp->firstColumn());
        மாறிலி std::string nextPrefix = nextFlp->sourcePrefix(முனை_Flp->firstColumn());
        எனில் (childPrefix != முனை_Prefix) திரும்பு;
        எனில் (nextPrefix != childPrefix) திரும்பு;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        மாறிலி std::string முனை_Source = முனை_Flp->source();
        மாறிலி std::string childSource = childFlp->source();
        மாறிலி std::string nextSource = nextFlp->source();
        எனில் (!(VString::leading_Whitespace_மொத்தம்(முனை_Source)
                  < VString::leading_Whitespace_மொத்தம்(childSource)
              && VString::leading_Whitespace_மொத்தம்(nextSource)
                     >= VString::leading_Whitespace_மொத்தம்(childSource)))
            திரும்பு;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    வெறுமை collectPorts(Ast_முனை* முனை_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        க்கு (Ast_முனை* nodep = முனை_Listp; nodep; nodep = nodep->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                எனில் (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        க்கு (Ast_முனை* nodep = முனை_Listp; nodep; nodep = nodep->nextp()) {
            எனில் (AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
                எனில் (varp->isIO()) தொடர்;
                மாறிலி auto it = portNames.find(varp->name());
                எனில் (it == portNames.end()) தொடர்;
                AstVar* மாறிலி iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                எனில் (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlinkFrBackWithNext());
                m_portDups.emplace(varp);
            }
        }
    }

    Ast_முனை* getVarsAndUnlink(Ast_முனை* stmtsp) {
        Ast_முனை* varsp = nullptr;
        க்கு (Ast_முனை *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            எனில் (VN_IS(itemp, Var)) varsp = Ast_முனை::addNext(varsp, itemp->unlinkFrBack());
        }
        திரும்பு varsp;
    }
    Ast_முனை_Stmt* getBlockAdds() {
        Ast_முனை_Stmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        எனில் (m_blockAddStaticStmtp)
            addsp = Ast_முனை::addNext(addsp,
                                     புதிய AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        எனில் (m_blockAddAutomaticStmtp)
            addsp = Ast_முனை::addNext(
                addsp, புதிய AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        திரும்பு addsp;
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process only once.
        // Mark class methods
        எனில் (VN_IS(m_modp, Class)) nodep->classMethod(உண்மை);

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
        m_lifetimeAllowed = உண்மை;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](மாறிலி Ast_முனை* மாறிலி nodep) { திரும்பு nodep->isTimingControl(); });
        எனில் (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } இல்லையெனில் {
            எனில் (nodep->classMethod()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } இல்லையெனில் எனில் (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        எனில் (nodep->classMethod() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warnMore() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_முனை* stmtsp = nullptr;
        எனில் (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = Ast_முனை::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_முனை::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    வெறுமை visit(Ast_முனை_DType* nodep) override { visit_Iterate_முனை_DType(nodep); }
    வெறுமை visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(உண்மை);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstConstraintForeach* nodep) override {
        எனில் (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_கோவை* exprp) { exprp->isSoft(உண்மை); });
        }
    }
    வெறுமை visit(AstEnumDType* nodep) override {
        எனில் (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_முனை_DType(nodep);
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {
        எனில் (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_முனை_DType(nodep);
    }
    வெறுமை visit(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        எனில் (nodep->rangep()) {
            எனில் (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t offset_from_init = 0;
            AstEnumItem* addp = nullptr;
            File_வரி* மாறிலி flp = nodep->fileline();
            க்கு (மாறிலி முழு i : nodep->rangep()->seq_இடது_To_வலது()) {
                மாறிலி string name = nodep->name() + cvtToStr(i);
                Ast_முனை_கோவை* valuep = nullptr;
                எனில் (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = புதிய AstAdd{flp, nodep->valuep()->clone_மரம்(உண்மை),
                                     புதிய AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = Ast_முனை::addNext(addp, புதிய AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        }
    }

    வெறுமை visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        மாறிலி AstClass* மாறிலி classp = VN_CAST(m_modp, Class);
        எனில் (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(உண்மை);
        }
        எனில் (nodep->embeddedCovergroup()) {
            AstRefDType* மாறிலி refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        எனில் (nodep->valuep()) nodep->hasUserInit(உண்மை);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        எனில் (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            எனில் (VN_IS(m_modp, Module) && m_ftaskp) {
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
            } இல்லையெனில் {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnMore()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        எனில் (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        எனில் (!nodep->direction().isAny()) {  // Not a port
            எனில் (nodep->lifetime().isNone()) {
                எனில் (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } இல்லையெனில் {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } இல்லையெனில் எனில் (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            எனில் (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } இல்லையெனில் {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } இல்லையெனில் எனில் (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        எனில் (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            எனில் (m_beginDepth > 0
                || (m_beginDepth == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        எனில் (nodep->isGParam() && m_modp) m_modp->hasGParam(உண்மை);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        மாறிலி AstBasicDType* மாறிலி basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        எனில் (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(உண்மை);

        எனில் (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        எனில் (nodep->isParam() && m_inInterface) nodep->isIfaceParam(உண்மை);
        எனில் (AstParseTypeDType* மாறிலி ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_முனை* dtypep = nodep->valuep();
            எனில் (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                எனில் (Ast_முனை_கோவை* exprp = VN_CAST(dtypep, முனை_கோவை)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    போது (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        எனில் (!rhsDotp) நிறுத்து;
                        File_வரி* மாறிலி fl = dotp->fileline();
                        மாறிலி தருக்க colon = dotp->colon();
                        Ast_முனை_கோவை* மாறிலி lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), முனை_கோவை);
                        Ast_முனை_கோவை* மாறிலி rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), முனை_கோவை);
                        Ast_முனை_கோவை* மாறிலி rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), முனை_கோவை);
                        File_வரி* மாறிலி rhsFl = rhsDotp->fileline();
                        மாறிலி தருக்க rhsColon = rhsDotp->colon();
                        AstDot* மாறிலி newLhs = புதிய AstDot{fl, colon, lhs, rhsLhs};
                        exprp = புதிய AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_மரம்(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } இல்லையெனில் {
                dtypep = புதிய AstVoidDType{nodep->fileline()};
            }

            Ast_முனை* மாறிலி newp = புதிய AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, புதிய AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            திரும்பு;
        }
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(பொய்), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        எனில் (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            எனில் (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(உண்மை);
            } இல்லையெனில் எனில் (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(உண்மை);
            } இல்லையெனில் எனில் (m_modp && v3Global.opt.publicDepth()) {
                எனில் ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(உண்மை);
                } இல்லையெனில் எனில் (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(உண்மை);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        எனில் (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(பொய்);
            nodep->trace(பொய்);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        எனில் (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, உண்மை);
        // Compute initial value
        எனில் (nodep->valuep() || nodep->needsCReset()) {
            File_வரி* மாறிலி fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> Ast_முனை_கோவை* {
                எனில் (nodep->valuep()) {
                    திரும்பு VN_AS(nodep->valuep()->unlinkFrBack(), முனை_கோவை);
                } இல்லையெனில் {
                    திரும்பு புதிய AstCReset{fl, nodep, பொய்};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            எனில் (m_portDups.find(nodep) != m_portDups.end()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } இல்லையெனில் எனில் (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } இல்லையெனில் எனில் (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_உள்ளீடு()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                எனில் (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlinkFrBack()->delete_மரம்();
                }
            } இல்லையெனில் எனில் (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(உண்மை);
                File_வரி* மாறிலி newfl = புதிய File_வரி{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, உண்மை);
                AstAssign* மாறிலி assp
                    = புதிய AstAssign{newfl, புதிய AstParseRef{newfl, nodep->name()}, createValuep()};
                எனில் (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = Ast_முனை::addNext(m_blockAddAutomaticStmtp, assp);
                } இல்லையெனில் {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = Ast_முனை::addNext(m_blockAddStaticStmtp, assp);
                }
            } இல்லையெனில் எனில் (m_மதிப்பு_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                எனில் (nodep->valuep()) {
                    nodep->noCReset(உண்மை);
                    File_வரி* மாறிலி newfl = புதிய File_வரி{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, உண்மை);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, உண்மை);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* மாறிலி assp = புதிய AstAssign{
                        newfl, புதிய AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_முனை* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    எனில் (nodep->lifetime().isAutomatic()) {
                        newInitp = புதிய AstInitialAutomatic{newfl, assp};
                    } இல்லையெனில் {
                        newInitp = புதிய AstInitialStatic{newfl, assp};
                    }
                    nodep->addNextHere(newInitp);
                }
            } இல்லையெனில் எனில் (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    நிலையான தருக்க isNegative(மாறிலி AstConst* constp) {
        திரும்பு constp->isSigned() && constp->இலக்கம்().bitIs1(constp->width() - 1);
    }
    வெறுமை visit(AstRange* nodep) override {
        நிலையான constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        மாறிலி AstConst* மாறிலி leftp = VN_CAST(nodep->leftp(), Const);
        மாறிலி AstConst* மாறிலி rightp = VN_CAST(nodep->rightp(), Const);
        எனில் (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            மாறிலி uint64_t இடது = leftp->toUQuad();
            மாறிலி uint64_t வலது = rightp->toUQuad();
            மாறிலி uint64_t span = std::max(இடது, வலது) - std::min(இடது, வலது);
            எனில் (span >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << span + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstConst* nodep) override {
        எனில் (nodep->இலக்கம்().autoExtend() && nodep->fileline()->language() < V3LangCode::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    வெறுமை visit(AstAttrOf* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        எனில் (nodep->attrType() == VAttrType::DT_PUBLIC) {
            AstTypedef* மாறிலி typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(உண்மை);
            m_varp->sigModPublic(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_SPLIT_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            எனில் (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } இல்லையெனில் {
                m_varp->attrSplitVar(உண்மை);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(உண்மை);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(உண்மை);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(உண்மை);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(உண்மை);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(உண்மை);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(உண்மை);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(உண்மை);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        }
    }

    வெறுமை visit(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = nullptr;
        மாறிலி ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        எனில் (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } இல்லையெனில் {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_முனை* backp = nodep->backp();
            க்கு (; backp; backp = backp->backp()) {
                எனில் (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, முனை_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    நிறுத்து;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_முனை_DType* மாறிலி dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            எனில் (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
                திரும்பு;
            } இல்லையெனில் {
                defp = புதிய AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.size()));
                UINFO(9, "New impltypedef " << defp);
                backp->addNextHere(defp);
            }
        }
        nodep->replaceWith(புதிய AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }

    வெறுமை visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை_Foreach* nodep) override {
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
        AstForeachHeader* மாறிலி headerp = nodep->headerp();
        எனில் (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    வெறுமை visit(AstLoop* nodep) override {
        cleanFileline(nodep);
        எனில் (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } இல்லையெனில் {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        எனில் (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_வரி* மாறிலி newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, உண்மை);
            nodep->fileline(newfl);
        }
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        எனில் (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_inInterface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_இலக்கம்);
        VL_RESTORER(m_beginDepth);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_இலக்கம்);
        VL_RESTORER(m_மதிப்பு_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        எனில் (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        எனில் (VN_IS(nodep, Iface)) m_inInterface = உண்மை;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_இலக்கம் = 0;
        m_beginDepth = 0;
        m_மதிப்பு_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = பொய்;
        m_rand_Sequence_இலக்கம் = 0;

        எனில் (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        எனில் (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        எனில் (AstModule* மாறிலி modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    வெறுமை visit_Iterate_No_மதிப்பு_Mod(Ast_முனை* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_மதிப்பு_Modp);
        m_மதிப்பு_Modp = nullptr;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = உண்மை;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        visit_Iterate_No_மதிப்பு_Mod(nodep);
    }
    வெறுமை visit(AstCover* nodep) override { visit_Iterate_No_மதிப்பு_Mod(nodep); }
    வெறுமை visit(AstRestrict* nodep) override { visit_Iterate_No_மதிப்பு_Mod(nodep); }

    வெறுமை visit(AstGenBlock* nodep) override {
        V3Control::applyCoverageBlock(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_beginDepth);
        ++m_beginDepth;
        மாறிலி Ast_முனை* மாறிலி backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        மாறிலி தருக்க nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        முழு assign_Gen_Blk_இலக்கம் = -1;
        எனில் (nodep->genforp()) {
            ++m_genblk_இலக்கம்;
            எனில் (nodep->name() == "") assign_Gen_Blk_இலக்கம் = m_genblk_இலக்கம்;
        } இல்லையெனில் எனில் (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            assign_Gen_Blk_இலக்கம் = m_genblkAbove;
        }
        எனில் (assign_Gen_Blk_இலக்கம் != -1) {
            nodep->name("genblk" + cvtToStr(assign_Gen_Blk_இலக்கம்));
            எனில் (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warnMore()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        எனில் (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_இலக்கம்);
            m_genblkAbove = 0;
            m_genblk_இலக்கம் = 0;
            iterateChildren(nodep);
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstGenCase* nodep) override {
        ++m_genblk_இலக்கம்;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_இலக்கம்);
        m_genblkAbove = m_genblk_இலக்கம்;
        m_genblk_இலக்கம் = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        மாறிலி தருக்க nestedIf = (VN_IS(nodep->backp(), GenBlock)
                               && nestedIfBegin(VN_CAST(nodep->backp(), GenBlock)));
        எனில் (nestedIf) {
            iterateChildren(nodep);
        } இல்லையெனில் {
            ++m_genblk_இலக்கம்;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_இலக்கம்);
            m_genblkAbove = m_genblk_இலக்கம்;
            m_genblk_இலக்கம் = 0;
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstCell* nodep) override {
        எனில் (nodep->origName().empty()) {
            எனில் (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            மாறிலி string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Block* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterateAndNextNull(nodep->declsp());
        }

        எனில் (AstBegin* மாறிலி beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterateAndNextNull(nodep->stmtsp());
        எனில் (AstFork* மாறிலி forkp = VN_CAST(nodep, Fork)) iterateAndNextNull(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_முனை* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = Ast_முனை::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_முனை::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    வெறுமை visit(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        எனில் (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        எனில் (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(உண்மை);
            Ast_முனை* மாறிலி nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->addNext(nodep);
        }
    }
    வெறுமை visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        எனில் (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_முனை* scanp = nodep;
            // Skip over the New's statement
            க்கு (; scanp && !VN_IS(scanp, Stmt_கோவை); scanp = scanp->backp()) {}
            எனில் (VN_IS(scanp, Stmt_கோவை)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                க்கு (; scanp; scanp = scanp->backp()) {
                    எனில் (VN_IS(scanp, முனை_Stmt) || VN_IS(scanp, முனை_Module)
                        || VN_IS(scanp, முனை_FTask))
                        நிறுத்து;
                }
                எனில் (!VN_IS(scanp, முனை_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    வெறுமை visit(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    வெறுமை visit(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::to_string(m_rand_Sequence_இலக்கம்++));
        iterateChildren(nodep);
    }
    வெறுமை visit(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    வெறுமை visit(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    வெறுமை visit(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    வெறுமை visit(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    வெறுமை visit(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    வெறுமை visit(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    வெறுமை visit(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* மாறிலி alwaysp = VN_CAST(nodep->backp(), Always);
        எனில் (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        } இல்லையெனில் எனில் (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            எனில் (Ast_Sen_மரம்* மாறிலி sentreep = nodep->sentreep()) {
                sentreep->unlinkFrBackWithNext();
                alwaysp->sentreep(sentreep);
            }
            எனில் (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
        }
    }
    வெறுமை visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        க்கு (Ast_முனை *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            எனில் (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                எனில் (citemp->exprp() || citemp->assignp()) தொடர்;
                எனில் (citemp->skewp()) {
                    எனில் (citemp->direction() == VDirection::உள்ளீடு) {
                        // Disallow default redefinition; note some simulators allow this
                        எனில் (m_defaultInSkewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_defaultInSkewp = citemp->skewp();
                    } இல்லையெனில் எனில் (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        எனில் (m_defaultOutSkewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_defaultOutSkewp = citemp->skewp();
                    } இல்லையெனில் {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(தள்ளு_Deletep(citemp->unlinkFrBack()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        எனில் (nodep->direction() == VDirection::OUTPUT) {
            எனில் (!nodep->skewp()) {
                எனில் (m_defaultOutSkewp) {
                    nodep->skewp(m_defaultOutSkewp->clone_மரம்(பொய்));
                } இல்லையெனில் {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(புதிய AstConst{nodep->fileline(), 0});
                }
            }
        } இல்லையெனில் எனில் (nodep->direction() == VDirection::உள்ளீடு) {
            எனில் (!nodep->skewp()) {
                எனில் (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->clone_மரம்(பொய்));
                } இல்லையெனில் {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(புதிய AstConst{nodep->fileline(), AstConst::OneStep{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        எனில் (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    வெறுமை visit(Ast_Tagged_கோவை* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstTaggedPattern* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPatternVar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPatternStar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        எனில் (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    நிலையான Ast_முனை* makeArgMemberCopies(Ast_முனை* argsp, தருக்க readOnlyRefs) {
        Ast_முனை* resultp = nullptr;
        க்கு (Ast_முனை* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* மாறிலி origVarp = VN_AS(argp, Var);
            AstVar* மாறிலி paramp = origVarp->clone_மரம்(பொய்);
            File_வரி* மாறிலி fl = புதிய File_வரி{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, உண்மை);
            paramp->fileline(fl);
            paramp->funcLocal(உண்மை);
            paramp->direction(origVarp->direction());
            எனில் (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_முனை::addNextNull(resultp, paramp);
            Ast_முனை_கோவை* மாறிலி lhsp = புதிய AstDot{
                origVarp->fileline(), பொய், புதிய AstParseRef{origVarp->fileline(), "this"},
                புதிய AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstParseRef{paramp->fileline(), paramp->name()};
            resultp->addNext(புதிய AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        திரும்பு resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    நிலையான வெறுமை createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_முனை* argsp,
                                        Ast_முனை* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* மாறிலி defaultVarp
            = புதிய AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        எனில் (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, உண்மை));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* மாறிலி varp
                = புதிய AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             புதிய AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             புதிய AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* மாறிலி varp
                = புதிய AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             புதிய AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             புதிய AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* மாறிலி funcp = புதிய AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, பொய்));
            funcp->classMethod(உண்மை);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(உண்மை);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        க்கு (மாறிலி string& name : {"start"s, "stop"s}) {
            AstFunc* மாறிலி funcp = புதிய AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(உண்மை);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        க்கு (மாறிலி string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* மாறிலி funcp = புதிய AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, உண்மை);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(உண்மை);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* மாறிலி varp = புதிய AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(உண்மை);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(உண்மை);
                funcp->fvarp(varp);
            }
            க்கு (மாறிலி string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* மாறிலி varp = புதிய AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(உண்மை);
                varp->direction(VDirection::உள்ளீடு);
                varp->valuep(புதிய AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* மாறிலி funcp
                = புதிய AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(உண்மை);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* மாறிலி varp = புதிய AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(உண்மை);
            varp->direction(VDirection::உள்ளீடு);
            funcp->addStmtsp(varp);
        }
    }

    தருக்க dropDeprecatedCoverageOption(AstCgOptionAssign* மாறிலி nodep) {
        எனில் (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) திரும்பு பொய்;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        திரும்பு உண்மை;
    }

    வெறுமை visit(AstCgOptionAssign* nodep) override {
        எனில் (dropDeprecatedCoverageOption(nodep)) திரும்பு;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    வெறுமை visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        எனில் (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) திரும்பு;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        எனில் (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        மாறிலி string libname = m_modp->libname();
        AstClass* மாறிலி cgClassp = புதிய AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, உண்மை);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, உண்மை);
        cgClassp->isCovergroup(உண்மை);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(உண்மை);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        எனில் (Ast_Sen_மரம்* மாறிலி eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* மாறிலி cgNodep = புதிய AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        க்கு (Ast_முனை* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* மாறிலி origVarp = VN_AS(argp, Var);
            எனில் (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::உள்ளீடு);
            }
            எனில் ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* மாறிலி memberp = origVarp->clone_மரம்(பொய்);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(பொய்);
            memberp->direction(VDirection::NONE);
            எனில் (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(உண்மை);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        க்கு (Ast_முனை* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* மாறிலி origVarp = VN_AS(argp, Var);
            எனில் (!origVarp->is_உள்ளீடு()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::உள்ளீடு);
            }
            AstVar* மாறிலி memberp = origVarp->clone_மரம்(பொய்);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(பொய்);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* மாறிலி newFuncp = புதிய AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, உண்மை);
        newFuncp->classMethod(உண்மை);
        newFuncp->isConstructor(உண்மை);
        newFuncp->dtypep(cgClassp->dtypep());
        எனில் (Ast_முனை* மாறிலி bodyp = nodep->membersp()) {
            bodyp->unlinkFrBackWithNext();
            newFuncp->addStmtsp(bodyp);
        }
        cgClassp->addMembersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_இலக்கம்);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_இலக்கம் = 0;
        க்கு (மாறிலி Ast_முனை* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (மாறிலி AstCoverpoint* மாறிலி pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    நிலையான string coverpointName(மாறிலி AstCoverpoint* nodep) {
        எனில் (!nodep->name().empty()) திரும்பு nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        மாறிலி AstParseRef* மாறிலி refp = VN_CAST(nodep->exprp(), ParseRef);
        திரும்பு refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    மாறிலி AstCoverpoint* findCoverpoint(மாறிலி AstCoverpointRef* refp) மாறிலி {
        எனில் (refp->exprp()) திரும்பு nullptr;  // A hierarchical reference, unsupported
        மாறிலி auto it = m_coverpoints.find(refp->name());
        திரும்பு it == m_coverpoints.end() ? nullptr : it->second;
    }

    வெறுமை visit(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        எனில் (nodep->name().empty()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            மாறிலி string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpoint_இலக்கம்++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        க்கு (Ast_முனை *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            எனில் (AstCgOptionAssign* மாறிலி optp = VN_CAST(itemp, CgOptionAssign)) {
                எனில் (dropDeprecatedCoverageOption(optp)) தொடர்;
                optp->unlinkFrBack();
                எனில் (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(புதிய AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_மரம்(பொய்)});
                } இல்லையெனில் {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_மரம்(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        எனில் (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        எனில் (மாறிலி AstCoverpointRef* மாறிலி refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            எனில் (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_வரி* மாறிலி fl = nodep->fileline();
        File_வரி* மாறிலி flNoWarn = புதிய File_வரி{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, உண்மை);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, உண்மை);
        AstVar* மாறிலி varp = புதிய AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        புதிய AstRefDType{fl, AstRefDType::Flag_Type_Of_கோவை{},
                                                        m_coverpointp->exprp()->clone_மரம்(பொய்)}};
        varp->funcLocal(உண்மை);
        varp->noReset(உண்மை);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    வெறுமை visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* மாறிலி refp = nodep->pointp();
        மாறிலி AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        மாறிலி AstParseRef* binp = nullptr;
        மாறிலி AstDot* மாறிலி dotp = VN_CAST(refp->exprp(), Dot);
        எனில் (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        எனில் (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
            திரும்பு;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        எனில் (binp) nodep->name(binp->name());
        refp->replaceWith(புதிய AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    வெறுமை visit(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        எனில் (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    வெறுமை visit(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        எனில் (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    வெறுமை visit(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        க்கு (Ast_முனை *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            எனில் (VN_IS(itemp, CoverCrossBin)) தொடர்;
            AstCgOptionAssign* மாறிலி optp = VN_AS(itemp, CgOptionAssign);
            எனில் (dropDeprecatedCoverageOption(optp)) தொடர்;
            itemp->unlinkFrBack();
            மாறிலி VCoverOptionType optType = optp->optType();
            எனில் (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(புதிய AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_மரம்(பொய்)});
            VL_DO_DANGLING(optp->delete_மரம்(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        எனில் (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_வரி* மாறிலி fl = nodep->fileline();
            File_வரி* மாறிலி flNoWarn = புதிய File_வரி{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, உண்மை);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, உண்மை);
            std::set<string> names;
            க்கு (மாறிலி Ast_முனை* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                மாறிலி AstCoverpointRef* மாறிலி refp = VN_AS(itemp, CoverpointRef);
                மாறிலி AstCoverpoint* மாறிலி pointp = findCoverpoint(refp);
                எனில் (!pointp || !names.செருகு(refp->name()).second) தொடர்;
                AstVar* மாறிலி varp
                    = புதிய AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 புதிய AstRefDType{fl, AstRefDType::Flag_Type_Of_கோவை{},
                                                 pointp->exprp()->clone_மரம்(பொய்)}};
                varp->funcLocal(உண்மை);
                varp->noReset(உண்மை);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        எனில் (!nodep->selectp()) VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    வெறுமை visit(AstSystemCSection* nodep) override {
        எனில் (nodep->user1SetOnce()) திரும்பு;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    வெறுமை visit(Ast_முனை* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

பொது:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::addStatSum(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

வெறுமை V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("linkparse", 0, dump_மரம்_Either_Level() >= 6);
}
