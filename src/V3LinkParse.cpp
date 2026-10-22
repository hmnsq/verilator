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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3LinkParse.h"

#شامل "V3Control.h"
#شامل "V3Stats.h"

#شامل <set>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

رده LinkParseVisitor final : عمومی VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    ثابت VNUser1InUse m_inuser1;
    ثابت VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<File_سطر*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    Ast_گره_Module* m_مقدار_Modp = nullptr;
    Ast_گره_Module* m_modp = nullptr;  // Current module
    منطقی m_inInterface = نادرست;  // True when inside interface declaration
    Ast_گره_Procedure* m_procedurep = nullptr;  // Current procedure
    Ast_گره_FTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    Ast_گره_Block* m_blockp = nullptr;  // Current AstNodeBlock
    Ast_گره_Stmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    Ast_گره_Stmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    Ast_گره_DType* m_dtypep = nullptr;  // Current data type
    Ast_گره_عبارت* m_defaultInSkewp = nullptr;  // Current default input skew
    Ast_گره_عبارت* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, ثابت AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    صحیح m_anonUdpId = 0;  // Counter for anonymous UDP instances
    صحیح m_coverpoint_رقم = 0;  // Counter for unnamed coverpoints within current covergroup
    صحیح m_genblkAbove = 0;  // Begin block number of if/case/for above
    صحیح m_genblk_رقم = 0;  // Begin block number, 0=none seen
    صحیح m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    صحیح m_rand_Sequence_رقم = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    منطقی m_lifetimeAllowed = نادرست;  // True to allow lifetime settings
    منطقی m_hasTimingControl = نادرست;  // If current task has timing control
    منطقی m_moduleWithGenericIface = نادرست;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    تهی cleanFileline(Ast_گره* nodep) {
        اگر (nodep->user2SetOnce()) بازگشت;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        اگر (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(جدید File_سطر{nodep->fileline()});
        }
        m_filelines.درج(nodep->fileline());
    }

    string nameFromTypedef(Ast_گره* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        اگر (ثابت AstTypedef* ثابت typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            اگر (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } وگرنه اگر (m_modp) {
                above = m_modp->name() + ".";
            }
            بازگشت above + typedefp->name();
        }
        بازگشت "";
    }

    تهی visit_Iterate_گره_DType(Ast_گره_DType* nodep) {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    منطقی nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        ثابت Ast_گره* ثابت backp = nodep->backp();
        بازگشت (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    تهی checkIndent(Ast_گره* nodep, Ast_گره* childp) {
        // Try very hard to avoid false positives
        Ast_گره* nextp = nodep->nextp();
        اگر (!childp) بازگشت;
        اگر (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        اگر (!nextp) بازگشت;
        اگر (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) بازگشت;
        File_سطر* ثابت گره_Flp = nodep->fileline();
        File_سطر* ثابت childFlp = childp->fileline();
        File_سطر* ثابت nextFlp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        اگر (گره_Flp->filenameno() != childFlp->filenameno()) بازگشت;
        اگر (گره_Flp->filenameno() != nextFlp->filenameno()) بازگشت;
        اگر (گره_Flp->lastLineno() >= childFlp->firstLineno()) بازگشت;
        اگر (childFlp->lastLineno() >= nextFlp->firstLineno()) بازگشت;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        اگر (!(گره_Flp->firstColumn() < childFlp->firstColumn()
              && nextFlp->firstColumn() >= childFlp->firstColumn()))
            بازگشت;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        ثابت std::string گره_Prefix = گره_Flp->sourcePrefix(گره_Flp->firstColumn());
        ثابت std::string childPrefix = childFlp->sourcePrefix(گره_Flp->firstColumn());
        ثابت std::string nextPrefix = nextFlp->sourcePrefix(گره_Flp->firstColumn());
        اگر (childPrefix != گره_Prefix) بازگشت;
        اگر (nextPrefix != childPrefix) بازگشت;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        ثابت std::string گره_Source = گره_Flp->source();
        ثابت std::string childSource = childFlp->source();
        ثابت std::string nextSource = nextFlp->source();
        اگر (!(VString::leading_Whitespace_شمار(گره_Source)
                  < VString::leading_Whitespace_شمار(childSource)
              && VString::leading_Whitespace_شمار(nextSource)
                     >= VString::leading_Whitespace_شمار(childSource)))
            بازگشت;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    تهی collectPorts(Ast_گره* گره_Listp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        برای (Ast_گره* nodep = گره_Listp; nodep; nodep = nodep->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        برای (Ast_گره* nodep = گره_Listp; nodep; nodep = nodep->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                اگر (varp->isIO()) ادامه;
                ثابت auto it = portNames.find(varp->name());
                اگر (it == portNames.end()) ادامه;
                AstVar* ثابت iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                اگر (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlinkFrBackWithNext());
                m_portDups.emplace(varp);
            }
        }
    }

    Ast_گره* getVarsAndUnlink(Ast_گره* stmtsp) {
        Ast_گره* varsp = nullptr;
        برای (Ast_گره *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (VN_IS(itemp, Var)) varsp = Ast_گره::addNext(varsp, itemp->unlinkFrBack());
        }
        بازگشت varsp;
    }
    Ast_گره_Stmt* getBlockAdds() {
        Ast_گره_Stmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        اگر (m_blockAddStaticStmtp)
            addsp = Ast_گره::addNext(addsp,
                                     جدید AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        اگر (m_blockAddAutomaticStmtp)
            addsp = Ast_گره::addNext(
                addsp, جدید AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        بازگشت addsp;
    }

    // VISITORS
    تهی visit(Ast_گره_FTask* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process only once.
        // Mark class methods
        اگر (VN_IS(m_modp, Class)) nodep->classMethod(درست);

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
        m_lifetimeAllowed = درست;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](ثابت Ast_گره* ثابت nodep) { بازگشت nodep->isTimingControl(); });
        اگر (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } وگرنه {
            اگر (nodep->classMethod()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } وگرنه اگر (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        اگر (nodep->classMethod() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warnMore() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        Ast_گره* stmtsp = nullptr;
        اگر (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = Ast_گره::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_گره::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    تهی visit(Ast_گره_DType* nodep) override { visit_Iterate_گره_DType(nodep); }
    تهی visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(درست);
        iterateChildren(nodep);
    }
    تهی visit(AstConstraintForeach* nodep) override {
        اگر (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](Ast_Constraint_عبارت* exprp) { exprp->isSoft(درست); });
        }
    }
    تهی visit(AstEnumDType* nodep) override {
        اگر (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_گره_DType(nodep);
    }
    تهی visit(Ast_گره_UOr_Struct_DType* nodep) override {
        اگر (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visit_Iterate_گره_DType(nodep);
    }
    تهی visit(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (nodep->rangep()) {
            اگر (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t offset_from_init = 0;
            AstEnumItem* addp = nullptr;
            File_سطر* ثابت flp = nodep->fileline();
            برای (ثابت صحیح i : nodep->rangep()->seq_چپ_To_راست()) {
                ثابت string name = nodep->name() + cvtToStr(i);
                Ast_گره_عبارت* valuep = nullptr;
                اگر (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = جدید AstAdd{flp, nodep->valuep()->clone_درخت(درست),
                                     جدید AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = Ast_گره::addNext(addp, جدید AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }

    تهی visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        ثابت AstClass* ثابت classp = VN_CAST(m_modp, Class);
        اگر (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(درست);
        }
        اگر (nodep->embeddedCovergroup()) {
            AstRefDType* ثابت refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        اگر (nodep->valuep()) nodep->hasUserInit(درست);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        اگر (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            اگر (VN_IS(m_modp, Module) && m_ftaskp) {
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
            } وگرنه {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnMore()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        اگر (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        اگر (!nodep->direction().isAny()) {  // Not a port
            اگر (nodep->lifetime().isNone()) {
                اگر (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } وگرنه {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } وگرنه اگر (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            اگر (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } وگرنه {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } وگرنه اگر (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        اگر (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            اگر (m_beginDepth > 0
                || (m_beginDepth == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        اگر (nodep->isGParam() && m_modp) m_modp->hasGParam(درست);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        ثابت AstBasicDType* ثابت basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        اگر (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(درست);

        اگر (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        اگر (nodep->isParam() && m_inInterface) nodep->isIfaceParam(درست);
        اگر (AstParseTypeDType* ثابت ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            Ast_گره* dtypep = nodep->valuep();
            اگر (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                اگر (Ast_گره_عبارت* exprp = VN_CAST(dtypep, گره_عبارت)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    وقتی (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        اگر (!rhsDotp) بشکن;
                        File_سطر* ثابت fl = dotp->fileline();
                        ثابت منطقی colon = dotp->colon();
                        Ast_گره_عبارت* ثابت lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), گره_عبارت);
                        Ast_گره_عبارت* ثابت rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), گره_عبارت);
                        Ast_گره_عبارت* ثابت rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), گره_عبارت);
                        File_سطر* ثابت rhsFl = rhsDotp->fileline();
                        ثابت منطقی rhsColon = rhsDotp->colon();
                        AstDot* ثابت newLhs = جدید AstDot{fl, colon, lhs, rhsLhs};
                        exprp = جدید AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->delete_درخت(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } وگرنه {
                dtypep = جدید AstVoidDType{nodep->fileline()};
            }

            Ast_گره* ثابت newp = جدید AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, جدید AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            بازگشت;
        }
        ثابت Ast_گره_DType* ثابت dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(نادرست), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        اگر (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            اگر (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(درست);
            } وگرنه اگر (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(درست);
            } وگرنه اگر (m_modp && v3Global.opt.publicDepth()) {
                اگر ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(درست);
                } وگرنه اگر (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(درست);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        اگر (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(نادرست);
            nodep->trace(نادرست);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        اگر (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, درست);
        // Compute initial value
        اگر (nodep->valuep() || nodep->needsCReset()) {
            File_سطر* ثابت fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> Ast_گره_عبارت* {
                اگر (nodep->valuep()) {
                    بازگشت VN_AS(nodep->valuep()->unlinkFrBack(), گره_عبارت);
                } وگرنه {
                    بازگشت جدید AstCReset{fl, nodep, نادرست};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            اگر (m_portDups.find(nodep) != m_portDups.end()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } وگرنه اگر (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } وگرنه اگر (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->is_درون_داد()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                اگر (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlinkFrBack()->delete_درخت();
                }
            } وگرنه اگر (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(درست);
                File_سطر* ثابت newfl = جدید File_سطر{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, درست);
                AstAssign* ثابت assp
                    = جدید AstAssign{newfl, جدید AstParseRef{newfl, nodep->name()}, createValuep()};
                اگر (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = Ast_گره::addNext(m_blockAddAutomaticStmtp, assp);
                } وگرنه {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = Ast_گره::addNext(m_blockAddStaticStmtp, assp);
                }
            } وگرنه اگر (m_مقدار_Modp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                اگر (nodep->valuep()) {
                    nodep->noCReset(درست);
                    File_سطر* ثابت newfl = جدید File_سطر{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, درست);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, درست);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* ثابت assp = جدید AstAssign{
                        newfl, جدید AstParseRef{newfl, nodep->name()}, createValuep()};
                    Ast_گره* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    اگر (nodep->lifetime().isAutomatic()) {
                        newInitp = جدید AstInitialAutomatic{newfl, assp};
                    } وگرنه {
                        newInitp = جدید AstInitialStatic{newfl, assp};
                    }
                    nodep->addNextHere(newInitp);
                }
            } وگرنه اگر (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    ایستا منطقی isNegative(ثابت AstConst* constp) {
        بازگشت constp->isSigned() && constp->رقم().bitIs1(constp->width() - 1);
    }
    تهی visit(AstRange* nodep) override {
        ایستا constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        ثابت AstConst* ثابت leftp = VN_CAST(nodep->leftp(), Const);
        ثابت AstConst* ثابت rightp = VN_CAST(nodep->rightp(), Const);
        اگر (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            ثابت uint64_t چپ = leftp->toUQuad();
            ثابت uint64_t راست = rightp->toUQuad();
            ثابت uint64_t span = std::max(چپ, راست) - std::min(چپ, راست);
            اگر (span >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << span + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstConst* nodep) override {
        اگر (nodep->رقم().autoExtend() && nodep->fileline()->language() < V3LangCode::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    تهی visit(AstAttrOf* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (nodep->attrType() == VAttrType::DT_PUBLIC) {
            AstTypedef* ثابت typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(درست);
            m_varp->sigModPublic(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_SPLIT_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            اگر (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } وگرنه {
                m_varp->attrSplitVar(درست);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(درست);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(درست);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(درست);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(درست);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(درست);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(درست);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(درست);
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        }
    }

    تهی visit(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = nullptr;
        ثابت ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        اگر (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } وگرنه {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            Ast_گره* backp = nodep->backp();
            برای (; backp; backp = backp->backp()) {
                اگر (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, گره_FTask)
                    || VN_IS(backp, ParamTypeDType))
                    بشکن;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            Ast_گره_DType* ثابت dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            اگر (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                بازگشت;
            } وگرنه {
                defp = جدید AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.size()));
                UINFO(9, "New impltypedef " << defp);
                backp->addNextHere(defp);
            }
        }
        nodep->replaceWith(جدید AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    تهی visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    تهی visit(Ast_گره_Foreach* nodep) override {
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
        AstForeachHeader* ثابت headerp = nodep->headerp();
        اگر (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }
        iterateChildren(nodep);
    }
    تهی visit(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    تهی visit(AstLoop* nodep) override {
        cleanFileline(nodep);
        اگر (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } وگرنه {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    تهی visit(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            File_سطر* ثابت newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, درست);
            nodep->fileline(newfl);
        }
    }
    تهی visit(Ast_گره_Module* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        اگر (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_inInterface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_رقم);
        VL_RESTORER(m_beginDepth);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_rand_Sequence_رقم);
        VL_RESTORER(m_مقدار_Modp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        اگر (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        اگر (VN_IS(nodep, Iface)) m_inInterface = درست;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblk_رقم = 0;
        m_beginDepth = 0;
        m_مقدار_Modp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = نادرست;
        m_rand_Sequence_رقم = 0;

        اگر (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        اگر (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        اگر (AstModule* ثابت modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    تهی visit_Iterate_No_مقدار_Mod(Ast_گره* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_مقدار_Modp);
        m_مقدار_Modp = nullptr;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = درست;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        visit_Iterate_No_مقدار_Mod(nodep);
    }
    تهی visit(AstCover* nodep) override { visit_Iterate_No_مقدار_Mod(nodep); }
    تهی visit(AstRestrict* nodep) override { visit_Iterate_No_مقدار_Mod(nodep); }

    تهی visit(AstGenBlock* nodep) override {
        V3Control::applyCoverageBlock(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_beginDepth);
        ++m_beginDepth;
        ثابت Ast_گره* ثابت backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        ثابت منطقی nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        صحیح assign_Gen_Blk_رقم = -1;
        اگر (nodep->genforp()) {
            ++m_genblk_رقم;
            اگر (nodep->name() == "") assign_Gen_Blk_رقم = m_genblk_رقم;
        } وگرنه اگر (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            assign_Gen_Blk_رقم = m_genblkAbove;
        }
        اگر (assign_Gen_Blk_رقم != -1) {
            nodep->name("genblk" + cvtToStr(assign_Gen_Blk_رقم));
            اگر (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warnMore()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        اگر (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_رقم);
            m_genblkAbove = 0;
            m_genblk_رقم = 0;
            iterateChildren(nodep);
        } وگرنه {
            iterateChildren(nodep);
        }
    }
    تهی visit(AstGenCase* nodep) override {
        ++m_genblk_رقم;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblk_رقم);
        m_genblkAbove = m_genblk_رقم;
        m_genblk_رقم = 0;
        iterateChildren(nodep);
    }
    تهی visit(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        ثابت منطقی nestedIf = (VN_IS(nodep->backp(), GenBlock)
                               && nestedIfBegin(VN_CAST(nodep->backp(), GenBlock)));
        اگر (nestedIf) {
            iterateChildren(nodep);
        } وگرنه {
            ++m_genblk_رقم;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblk_رقم);
            m_genblkAbove = m_genblk_رقم;
            m_genblk_رقم = 0;
            iterateChildren(nodep);
        }
    }
    تهی visit(AstCell* nodep) override {
        اگر (nodep->origName().empty()) {
            اگر (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            ثابت string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    تهی visit(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره_Block* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterateAndNextNull(nodep->declsp());
        }

        اگر (AstBegin* ثابت beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterateAndNextNull(nodep->stmtsp());
        اگر (AstFork* ثابت forkp = VN_CAST(nodep, Fork)) iterateAndNextNull(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        Ast_گره* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = Ast_گره::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = Ast_گره::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    تهی visit(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        اگر (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        اگر (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(درست);
            Ast_گره* ثابت nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->addNext(nodep);
        }
    }
    تهی visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            Ast_گره* scanp = nodep;
            // Skip over the New's statement
            برای (; scanp && !VN_IS(scanp, Stmt_عبارت); scanp = scanp->backp()) {}
            اگر (VN_IS(scanp, Stmt_عبارت)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                برای (; scanp; scanp = scanp->backp()) {
                    اگر (VN_IS(scanp, گره_Stmt) || VN_IS(scanp, گره_Module)
                        || VN_IS(scanp, گره_FTask))
                        بشکن;
                }
                اگر (!VN_IS(scanp, گره_FTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    تهی visit(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    تهی visit(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    تهی visit(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::to_string(m_rand_Sequence_رقم++));
        iterateChildren(nodep);
    }
    تهی visit(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    تهی visit(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    تهی visit(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    تهی visit(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    تهی visit(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    تهی visit(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    تهی visit(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* ثابت alwaysp = VN_CAST(nodep->backp(), Always);
        اگر (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        } وگرنه اگر (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            اگر (Ast_Sen_درخت* ثابت sentreep = nodep->sentreep()) {
                sentreep->unlinkFrBackWithNext();
                alwaysp->sentreep(sentreep);
            }
            اگر (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
        }
    }
    تهی visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        برای (Ast_گره *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                اگر (citemp->exprp() || citemp->assignp()) ادامه;
                اگر (citemp->skewp()) {
                    اگر (citemp->direction() == VDirection::درون_داد) {
                        // Disallow default redefinition; note some simulators allow this
                        اگر (m_defaultInSkewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_defaultInSkewp = citemp->skewp();
                    } وگرنه اگر (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        اگر (m_defaultOutSkewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_defaultOutSkewp = citemp->skewp();
                    } وگرنه {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(افزودن_Deletep(citemp->unlinkFrBack()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        اگر (nodep->direction() == VDirection::OUTPUT) {
            اگر (!nodep->skewp()) {
                اگر (m_defaultOutSkewp) {
                    nodep->skewp(m_defaultOutSkewp->clone_درخت(نادرست));
                } وگرنه {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(جدید AstConst{nodep->fileline(), 0});
                }
            }
        } وگرنه اگر (nodep->direction() == VDirection::درون_داد) {
            اگر (!nodep->skewp()) {
                اگر (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->clone_درخت(نادرست));
                } وگرنه {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(جدید AstConst{nodep->fileline(), AstConst::OneStep{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    تهی visit(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        اگر (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    تهی visit(Ast_Tagged_عبارت* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstTaggedPattern* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstPatternVar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstPatternStar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        اگر (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    ایستا Ast_گره* makeArgMemberCopies(Ast_گره* argsp, منطقی readOnlyRefs) {
        Ast_گره* resultp = nullptr;
        برای (Ast_گره* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            AstVar* ثابت paramp = origVarp->clone_درخت(نادرست);
            File_سطر* ثابت fl = جدید File_سطر{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, درست);
            paramp->fileline(fl);
            paramp->funcLocal(درست);
            paramp->direction(origVarp->direction());
            اگر (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = Ast_گره::addNextNull(resultp, paramp);
            Ast_گره_عبارت* ثابت lhsp = جدید AstDot{
                origVarp->fileline(), نادرست, جدید AstParseRef{origVarp->fileline(), "this"},
                جدید AstParseRef{origVarp->fileline(), origVarp->name()}};
            Ast_گره_عبارت* ثابت rhsp = جدید AstParseRef{paramp->fileline(), paramp->name()};
            resultp->addNext(جدید AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        بازگشت resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    ایستا تهی createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, Ast_گره* argsp,
                                        Ast_گره* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* ثابت defaultVarp
            = جدید AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        اگر (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, درست));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* ثابت varp
                = جدید AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             جدید AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             جدید AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* ثابت varp
                = جدید AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             جدید AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             جدید AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* ثابت funcp = جدید AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, نادرست));
            funcp->classMethod(درست);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(درست);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        برای (ثابت string& name : {"start"s, "stop"s}) {
            AstFunc* ثابت funcp = جدید AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(درست);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        برای (ثابت string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* ثابت funcp = جدید AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, درست);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(درست);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* ثابت varp = جدید AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(درست);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(درست);
                funcp->fvarp(varp);
            }
            برای (ثابت string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* ثابت varp = جدید AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(درست);
                varp->direction(VDirection::درون_داد);
                varp->valuep(جدید AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* ثابت funcp
                = جدید AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(درست);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* ثابت varp = جدید AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(درست);
            varp->direction(VDirection::درون_داد);
            funcp->addStmtsp(varp);
        }
    }

    منطقی dropDeprecatedCoverageOption(AstCgOptionAssign* ثابت nodep) {
        اگر (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) بازگشت نادرست;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        بازگشت درست;
    }

    تهی visit(AstCgOptionAssign* nodep) override {
        اگر (dropDeprecatedCoverageOption(nodep)) بازگشت;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    تهی visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        اگر (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) بازگشت;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        اگر (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        ثابت string libname = m_modp->libname();
        AstClass* ثابت cgClassp = جدید AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, درست);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, درست);
        cgClassp->isCovergroup(درست);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(درست);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        اگر (Ast_Sen_درخت* ثابت eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* ثابت cgNodep = جدید AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        برای (Ast_گره* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            اگر (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::درون_داد);
            }
            اگر ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* ثابت memberp = origVarp->clone_درخت(نادرست);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(نادرست);
            memberp->direction(VDirection::NONE);
            اگر (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(درست);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        برای (Ast_گره* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* ثابت origVarp = VN_AS(argp, Var);
            اگر (!origVarp->is_درون_داد()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::درون_داد);
            }
            AstVar* ثابت memberp = origVarp->clone_درخت(نادرست);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(نادرست);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* ثابت newFuncp = جدید AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, درست);
        newFuncp->classMethod(درست);
        newFuncp->isConstructor(درست);
        newFuncp->dtypep(cgClassp->dtypep());
        اگر (Ast_گره* ثابت bodyp = nodep->membersp()) {
            bodyp->unlinkFrBackWithNext();
            newFuncp->addStmtsp(bodyp);
        }
        cgClassp->addMembersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpoint_رقم);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpoint_رقم = 0;
        برای (ثابت Ast_گره* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (ثابت AstCoverpoint* ثابت pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    ایستا string coverpointName(ثابت AstCoverpoint* nodep) {
        اگر (!nodep->name().empty()) بازگشت nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        ثابت AstParseRef* ثابت refp = VN_CAST(nodep->exprp(), ParseRef);
        بازگشت refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    ثابت AstCoverpoint* findCoverpoint(ثابت AstCoverpointRef* refp) ثابت {
        اگر (refp->exprp()) بازگشت nullptr;  // A hierarchical reference, unsupported
        ثابت auto it = m_coverpoints.find(refp->name());
        بازگشت it == m_coverpoints.end() ? nullptr : it->second;
    }

    تهی visit(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        اگر (nodep->name().empty()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            ثابت string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpoint_رقم++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        برای (Ast_گره *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (AstCgOptionAssign* ثابت optp = VN_CAST(itemp, CgOptionAssign)) {
                اگر (dropDeprecatedCoverageOption(optp)) ادامه;
                optp->unlinkFrBack();
                اگر (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(جدید AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->clone_درخت(نادرست)});
                } وگرنه {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->delete_درخت(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    تهی visit(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        اگر (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        iterateChildren(nodep);
    }

    تهی visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        اگر (ثابت AstCoverpointRef* ثابت refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            اگر (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        File_سطر* ثابت fl = nodep->fileline();
        File_سطر* ثابت flNoWarn = جدید File_سطر{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, درست);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, درست);
        AstVar* ثابت varp = جدید AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        جدید AstRefDType{fl, AstRefDType::Flag_Type_Of_عبارت{},
                                                        m_coverpointp->exprp()->clone_درخت(نادرست)}};
        varp->funcLocal(درست);
        varp->noReset(درست);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    تهی visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* ثابت refp = nodep->pointp();
        ثابت AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        ثابت AstParseRef* binp = nullptr;
        ثابت AstDot* ثابت dotp = VN_CAST(refp->exprp(), Dot);
        اگر (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        اگر (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
            بازگشت;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        اگر (binp) nodep->name(binp->name());
        refp->replaceWith(جدید AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
        iterateChildren(nodep);
    }

    تهی visit(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    تهی visit(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        اگر (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
        }
    }

    تهی visit(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        برای (Ast_گره *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            اگر (VN_IS(itemp, CoverCrossBin)) ادامه;
            AstCgOptionAssign* ثابت optp = VN_AS(itemp, CgOptionAssign);
            اگر (dropDeprecatedCoverageOption(optp)) ادامه;
            itemp->unlinkFrBack();
            ثابت VCoverOptionType optType = optp->optType();
            اگر (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(جدید AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->clone_درخت(نادرست)});
            VL_DO_DANGLING(optp->delete_درخت(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    تهی visit(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        اگر (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            File_سطر* ثابت fl = nodep->fileline();
            File_سطر* ثابت flNoWarn = جدید File_سطر{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, درست);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, درست);
            std::set<string> names;
            برای (ثابت Ast_گره* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                ثابت AstCoverpoint* ثابت pointp = findCoverpoint(refp);
                اگر (!pointp || !names.درج(refp->name()).second) ادامه;
                AstVar* ثابت varp
                    = جدید AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 جدید AstRefDType{fl, AstRefDType::Flag_Type_Of_عبارت{},
                                                 pointp->exprp()->clone_درخت(نادرست)}};
                varp->funcLocal(درست);
                varp->noReset(درست);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        اگر (!nodep->selectp()) VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);
    }

    تهی visit(AstSystemCSection* nodep) override {
        اگر (nodep->user1SetOnce()) بازگشت;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    تهی visit(Ast_گره* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::addStatSum(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

تهی V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("linkparse", 0, dump_درخت_Either_Level() >= 6);
}
