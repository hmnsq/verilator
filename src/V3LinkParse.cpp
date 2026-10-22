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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3LinkParse.h"

#include "V3Control.h"
#include "V3Stats.h"

#include <set>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Link state, as a visitor of each AstNode

class LinkParseVisitor final : public VNVisitor {
    // NODE STATE
    // Cleared on netlist
    //  AstNode::user1()        -> bool.  True if processed
    //  AstNode::user2()        -> bool.  True if fileline recomputed
    const VNUser1InUse m_inuser1;
    const VNUser2InUse m_inuser2;

    // TYPES
    using ImplTypedefMap = std::map<std::string, AstTypedef*>;

    // STATE - across all visitors
    std::unordered_set<FileLine*> m_filelines;  // Filelines that have been seen

    // STATE - for current visit position (use VL_RESTORER)
    // If set, move AstVar->valuep() initial values to this module
    ImplTypedefMap m_implTypedef;  // Created typedefs for each <container,name>
    AstVar* m_varp = nullptr;  // Variable we're under
    AstNodeModule* m_valueModp = nullptr;
    AstNodeModule* m_modp = nullptr;  // Current module
    bool m_inInterface = false;  // True when inside interface declaration
    AstNodeProcedure* m_procedurep = nullptr;  // Current procedure
    AstNodeFTask* m_ftaskp = nullptr;  // Current task
    AstRSProd* m_rsProdp = nullptr;  // Current randsequence production
    AstNodeBlock* m_blockp = nullptr;  // Current AstNodeBlock
    AstNodeStmt* m_blockAddAutomaticStmtp = nullptr;  // Initial statements to add to block
    AstNodeStmt* m_blockAddStaticStmtp = nullptr;  // Initial statements to add to block
    AstNodeDType* m_dtypep = nullptr;  // Current data type
    AstNodeExpr* m_defaultInSkewp = nullptr;  // Current default input skew
    AstNodeExpr* m_defaultOutSkewp = nullptr;  // Current default output skew
    AstCoverpoint* m_coverpointp = nullptr;  // Current coverpoint
    AstCoverCross* m_crossp = nullptr;  // Current cross
    std::map<string, const AstCoverpoint*> m_coverpoints;  // Current covergroup's, by name
    整数 m_anonUdpId = 0;  // Counter for anonymous UDP instances
    整数 m_coverpointNum = 0;  // Counter for unnamed coverpoints within current covergroup
    整数 m_genblkAbove = 0;  // Begin block number of if/case/for above
    整数 m_genblkNum = 0;  // Begin block number, 0=none seen
    整数 m_beginDepth = 0;  // How many begin blocks above current node within current AstNodeModule
    整数 m_randSequenceNum = 0;  // RandSequence uniqify number
    VLifetime m_lifetime = VLifetime::STATIC_IMPLICIT;  // Propagating lifetime
    bool m_lifetimeAllowed = false;  // True to allow lifetime settings
    bool m_hasTimingControl = false;  // If current task has timing control
    bool m_moduleWithGenericIface = false;  // If current module contains generic interface
    std::set<AstVar*> m_portDups;  // Non-ANSI port datatype duplicating input/output decls

    // STATE - Statistic tracking
    VDouble0 m_statModules;  // Number of modules seen

    // METHODS
    void cleanFileline(AstNode* nodep) {
        如果 (nodep->user2SetOnce()) 返回;  // Process once
        // We make all filelines unique per AstNode.  This allows us to
        // later turn off messages on a fileline when an issue is found
        // so that messages on replicated blocks occur only once,
        // without suppressing other token's messages as a side effect.
        // We could have verilog.l create a new one on every token,
        // but that's a lot more structures than only doing AST nodes.
        // TODO: Many places copy the filename when suppressing warnings,
        // perhaps audit to make consistent and this is no longer needed
        如果 (m_filelines.find(nodep->fileline()) != m_filelines.end()) {
            nodep->fileline(new FileLine{nodep->fileline()});
        }
        m_filelines.insert(nodep->fileline());
    }

    string nameFromTypedef(AstNode* nodep) {
        // Try to find a name for a typedef'ed enum/struct
        如果 (const AstTypedef* const typedefp = VN_CAST(nodep->backp(), Typedef)) {
            // Create a name for the enum, to aid debug and tracing
            // This name is not guaranteed to be globally unique (due to later parameterization)
            string above;
            如果 (m_modp && VN_IS(m_modp, Package)) {
                above = m_modp->name() + "::";
            } 否则 如果 (m_modp) {
                above = m_modp->name() + ".";
            }
            返回 above + typedefp->name();
        }
        返回 "";
    }

    void visitIterateNodeDType(AstNodeDType* nodep) {
        如果 (nodep->user1SetOnce()) 返回;  // Process only once.
        cleanFileline(nodep);
        VL_RESTORER(m_dtypep);
        m_dtypep = nodep;
        iterateChildren(nodep);
    }

    bool nestedIfBegin(AstGenBlock* nodep) {  // Point at begin inside the GenIf/GenCaseItem
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
        const AstNode* const backp = nodep->backp();
        返回 (nodep->implied()  // User didn't provide begin/end
                && ((VN_IS(backp, GenIf) && VN_CAST(backp, GenIf)->elsesp() == nodep)
                    || VN_IS(backp, GenCaseItem))
                && !nodep->nextp()  // No other statements under upper genif else/case item
                && (VN_IS(nodep->itemsp(), GenIf))  // Begin has if underneath
                && !nodep->itemsp()->nextp());  // Has only one item
    }

    void checkIndent(AstNode* nodep, AstNode* childp) {
        // Try very hard to avoid false positives
        AstNode* nextp = nodep->nextp();
        如果 (!childp) 返回;
        如果 (!nextp && VN_IS(nodep, Loop) && VN_IS(nodep->backp(), Begin))
            nextp = nodep->backp()->nextp();
        如果 (!nextp) 返回;
        如果 (VN_IS(childp, Begin) || VN_IS(childp, GenBlock)) 返回;
        FileLine* const nodeFlp = nodep->fileline();
        FileLine* const childFlp = childp->fileline();
        FileLine* const nextFlp = nextp->fileline();
        // UINFO(0, "checkInd " << nodeFlp->firstColumn() << " " << nodep);
        // UINFO(0, "  child  " << childFlp->firstColumn() << " " << childp);
        // UINFO(0, " next    " << nextFlp->firstColumn() << " " << nextp);
        // Same filename, later line numbers (no macro magic going on)
        如果 (nodeFlp->filenameno() != childFlp->filenameno()) 返回;
        如果 (nodeFlp->filenameno() != nextFlp->filenameno()) 返回;
        如果 (nodeFlp->lastLineno() >= childFlp->firstLineno()) 返回;
        如果 (childFlp->lastLineno() >= nextFlp->firstLineno()) 返回;
        // This block has indent 'a'
        // Child block has indent 'b' where indent('b') > indent('a')
        // Next block has indent 'b'
        // (note similar code below)
        如果 (!(nodeFlp->firstColumn() < childFlp->firstColumn()
              && nextFlp->firstColumn() >= childFlp->firstColumn()))
            返回;
        // Might be a tab difference in spaces up to the node prefix, if so
        // just ignore this warning
        // Note it's correct we look at nodep's column in all of these
        const std::string nodePrefix = nodeFlp->sourcePrefix(nodeFlp->firstColumn());
        const std::string childPrefix = childFlp->sourcePrefix(nodeFlp->firstColumn());
        const std::string nextPrefix = nextFlp->sourcePrefix(nodeFlp->firstColumn());
        如果 (childPrefix != nodePrefix) 返回;
        如果 (nextPrefix != childPrefix) 返回;
        // Some file lines start after the indentation, so make another check
        // using actual file contents
        const std::string nodeSource = nodeFlp->source();
        const std::string childSource = childFlp->source();
        const std::string nextSource = nextFlp->source();
        如果 (!(VString::leadingWhitespaceCount(nodeSource)
                  < VString::leadingWhitespaceCount(childSource)
              && VString::leadingWhitespaceCount(nextSource)
                     >= VString::leadingWhitespaceCount(childSource)))
            返回;
        nextp->v3warn(MISINDENT,
                      "Misleading indentation\n"
                          << nextp->warnContextPrimary() << '\n'
                          << nodep->warnOther()
                          << "... Expected indentation matching this earlier statement's line:\n"
                          << nodep->warnContextSecondary());
    }

    void collectPorts(AstNode* nodeListp) {
        // V3LinkDot hasn't run yet, so have VAR for pre-IEEE 'input' and
        // separate var for pre-IEEE 'integer'.
        std::unordered_map<std::string, AstVar*> portNames;
        为 (AstNode* nodep = nodeListp; nodep; nodep = nodep->nextp()) {
            如果 (AstVar* const varp = VN_CAST(nodep, Var)) {
                如果 (varp->isIO()) portNames.emplace(varp->name(), varp);
            }
        }
        为 (AstNode* nodep = nodeListp; nodep; nodep = nodep->nextp()) {
            如果 (AstVar* const varp = VN_CAST(nodep, Var)) {
                如果 (varp->isIO()) continue;
                const auto it = portNames.find(varp->name());
                如果 (it == portNames.end()) continue;
                AstVar* const iop = it->second;
                UINFO(9, "Non-ANSI port dtype declaration " << varp);
                如果 (!iop->valuep() && varp->valuep())
                    iop->valuep(varp->valuep()->unlinkFrBackWithNext());
                m_portDups.emplace(varp);
            }
        }
    }

    AstNode* getVarsAndUnlink(AstNode* stmtsp) {
        AstNode* varsp = nullptr;
        为 (AstNode *nextp, *itemp = stmtsp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            如果 (VN_IS(itemp, Var)) varsp = AstNode::addNext(varsp, itemp->unlinkFrBack());
        }
        返回 varsp;
    }
    AstNodeStmt* getBlockAdds() {
        AstNodeStmt* addsp = nullptr;
        // Add a single AstInitial...Stmt as the statements within may
        // depend on each other, e.g. "var a=1; var b=a;" (but must be a constant expression)
        如果 (m_blockAddStaticStmtp)
            addsp = AstNode::addNext(addsp,
                                     new AstInitialStaticStmt{m_blockAddStaticStmtp->fileline(),
                                                              m_blockAddStaticStmtp});
        如果 (m_blockAddAutomaticStmtp)
            addsp = AstNode::addNext(
                addsp, new AstInitialAutomaticStmt{m_blockAddAutomaticStmtp->fileline(),
                                                   m_blockAddAutomaticStmtp});
        m_blockAddStaticStmtp = nullptr;
        m_blockAddAutomaticStmtp = nullptr;
        返回 addsp;
    }

    // VISITORS
    void visit(AstNodeFTask* nodep) override {
        如果 (nodep->user1SetOnce()) 返回;  // Process only once.
        // Mark class methods
        如果 (VN_IS(m_modp, Class)) nodep->classMethod(true);

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
        m_lifetimeAllowed = true;
        VL_RESTORER(m_hasTimingControl);
        m_hasTimingControl
            = nodep->exists([](const AstNode* const nodep) { 返回 nodep->isTimingControl(); });
        如果 (!nodep->lifetime().isNone()) {
            m_lifetime = nodep->lifetime().makeImplicit();
        } 否则 {
            如果 (nodep->classMethod()) {
                // Class methods are automatic by default
                m_lifetime = VLifetime::AUTOMATIC_IMPLICIT;
            } 否则 如果 (nodep->dpiImport() || VN_IS(nodep, Property)) {
                // DPI-imported functions and properties don't have lifetime specifiers
                m_lifetime = VLifetime::NONE;
            }
            nodep->lifetime(m_lifetime);
        }
        如果 (nodep->classMethod() && nodep->lifetime().isStatic()) {
            nodep->v3error("Class function/task cannot be static lifetime ('"
                           << nodep->verilogKwd() << " static') (IEEE 1800-2023 6.21)\n"
                           << nodep->warnMore() << "... May have intended 'static "
                           << nodep->verilogKwd() << "'");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);

        // If let, the statement must go first
        AstNode* stmtsp = nullptr;
        如果 (VN_IS(nodep, Let) && nodep->stmtsp()) stmtsp = nodep->stmtsp()->unlinkFrBack();
        // Move all Vars to front of function
        stmtsp = AstNode::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = AstNode::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    void visit(AstNodeDType* nodep) override { visitIterateNodeDType(nodep); }
    void visit(AstConstraint* nodep) override {
        v3Global.useRandomizeMethods(true);
        iterateChildren(nodep);
    }
    void visit(AstConstraintForeach* nodep) override {
        如果 (nodep->isSoft()) {
            nodep->v3warn(NONSTD, "Non-standard soft foreach");
            nodep->foreach([](AstConstraintExpr* exprp) { exprp->isSoft(true); });
        }
    }
    void visit(AstEnumDType* nodep) override {
        如果 (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visitIterateNodeDType(nodep);
    }
    void visit(AstNodeUOrStructDType* nodep) override {
        如果 (nodep->name() == "") {
            nodep->name(nameFromTypedef(nodep));  // Might still remain ""
        }
        visitIterateNodeDType(nodep);
    }
    void visit(AstEnumItem* nodep) override {
        // Expand ranges
        cleanFileline(nodep);
        iterateChildren(nodep);
        如果 (nodep->rangep()) {
            如果 (VL_UNCOVERABLE(!VN_IS(nodep->rangep()->leftp(), Const)  // LCOV_EXCL_START
                               || !VN_IS(nodep->rangep()->rightp(), Const))) {
                // We check this rule in the parser, so shouldn't fire
                nodep->v3error("Enum ranges must be integral, per spec");
            }  // LCOV_EXCL_STOP
            uint32_t offset_from_init = 0;
            AstEnumItem* addp = nullptr;
            FileLine* const flp = nodep->fileline();
            为 (const 整数 i : nodep->rangep()->seqLeftToRight()) {
                const string name = nodep->name() + cvtToStr(i);
                AstNodeExpr* valuep = nullptr;
                如果 (nodep->valuep()) {
                    // V3Width looks for Adds with same fileline as the EnumItem
                    valuep
                        = new AstAdd{flp, nodep->valuep()->cloneTree(true),
                                     new AstConst{flp, AstConst::Unsized32{}, offset_from_init}};
                }
                addp = AstNode::addNext(addp, new AstEnumItem{flp, name, nullptr, valuep});
                ++offset_from_init;
            }
            nodep->replaceWith(addp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        }
    }

    void visit(AstVar* nodep) override {
        cleanFileline(nodep);
        UINFO(9, "VAR " << nodep);
        const AstClass* const classp = VN_CAST(m_modp, Class);
        如果 (classp && classp->isCovergroup() && nodep->isClassMember() && !nodep->isFuncLocal()
            && (nodep->declDirection().isRef() || nodep->declDirection().isConstRef())) {
            nodep->covergroupRefMember(true);
        }
        如果 (nodep->embeddedCovergroup()) {
            AstRefDType* const refp = VN_AS(nodep->childDTypep(), RefDType);
            refp->name(AstClass::embeddedCovergroupTypeName(refp->name()));
        }
        如果 (nodep->valuep()) nodep->hasUserInit(true);
        // IEEE 1800-2023 6.21: for loop variables are automatic. verilog.y is
        // responsible for marking those.
        如果 (nodep->valuep() && nodep->lifetime().isNone() && m_lifetime.isStatic()
            && !nodep->isIO()
            && !nodep->isParam()
            // In task, or a procedure but not Initial/Final as executed only once
            && ((m_ftaskp && !m_ftaskp->lifetime().isStaticExplicit()) || m_procedurep)) {
            如果 (VN_IS(m_modp, Module) && m_ftaskp) {
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
            } 否则 {
                nodep->v3warn(
                    IMPLICITSTATIC,
                    "Variable's lifetime implicitly set to static (IEEE 1800-2023 6.21)\n"
                        << nodep->warnMore() << "... The initializer value will only be set once\n"
                        << nodep->warnMore()
                        << "... Suggest use 'static' before variable declaration'");
            }
        }
        如果 (!m_lifetimeAllowed && nodep->lifetime().isAutomatic()) {
            nodep->v3error(
                "Module variables cannot have automatic lifetime (IEEE 1800-2023 6.21): "
                << nodep->prettyNameQ());
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }
        如果 (!nodep->direction().isAny()) {  // Not a port
            如果 (nodep->lifetime().isNone()) {
                如果 (m_lifetimeAllowed) {
                    nodep->lifetime(m_lifetime);
                } 否则 {  // Module's always static per IEEE 1800-2023 6.21
                    nodep->lifetime(VLifetime::STATIC_IMPLICIT);
                }
            }
        } 否则 如果 (m_ftaskp) {
            // Variables in static tasks with timing control can be used after the task has ended
            // with use of join..fork_none, so they need to be static
            如果 (m_ftaskp->lifetime().isStatic() && m_hasTimingControl) {
                nodep->lifetime(VLifetime::STATIC_IMPLICIT);
            } 否则 {
                nodep->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            }
        } 否则 如果 (nodep->lifetime().isNone()) {
            // lifetime shouldn't be unknown, set static if none
            nodep->lifetime(VLifetime::STATIC_IMPLICIT);
        }

        如果 (nodep->isGParam() && !nodep->isAnsi()) {  // shadow some parameters into localparams
            如果 (m_beginDepth > 0
                || (m_beginDepth == 0
                    && (m_modp->hasParameterList() || VN_IS(m_modp, Class)
                        || VN_IS(m_modp, Package)))) {
                nodep->varType(VVarType::LPARAM);
            }
        }
        如果 (nodep->isGParam() && m_modp) m_modp->hasGParam(true);
        // An untyped parameter is of the type of its value (IEEE 1800-2023 6.20.2), which may
        // differ between specializations; recorded before V3Param gives it that type
        const AstBasicDType* const basicp = VN_CAST(nodep->childDTypep(), BasicDType);
        如果 (nodep->isGParam() && basicp && basicp->implicit()) nodep->untypedParam(true);

        如果 (nodep->isParam() && !nodep->valuep()
            && nodep->fileline()->language() < V3LangCode::L1800_2009) {
            nodep->v3warn(NEWERSTD,
                          "Parameter requires default value, or use IEEE 1800-2009 or later.");
        }

        // Mark parameters declared inside interfaces
        如果 (nodep->isParam() && m_inInterface) nodep->isIfaceParam(true);
        如果 (AstParseTypeDType* const ptypep = VN_CAST(nodep->subDTypep(), ParseTypeDType)) {
            // It's a parameter type. Use a different node type for this.
            AstNode* dtypep = nodep->valuep();
            如果 (dtypep) {
                dtypep->unlinkFrBack();
                // Transform right-associative Dot tree to left-associative
                // This handles typedef with arrayed interface on first component:
                //   typedef if0[0].x_if0.rq_t my_t;
                // Grammar produces: DOT(SELBIT, DOT(x_if0, rq_t))
                // We need:          DOT(DOT(SELBIT, x_if0), rq_t)
                如果 (AstNodeExpr* exprp = VN_CAST(dtypep, NodeExpr)) {
                    AstDot* dotp = VN_CAST(exprp, Dot);
                    当 (dotp) {
                        AstDot* rhsDotp = VN_CAST(dotp->rhsp(), Dot);
                        如果 (!rhsDotp) break;
                        FileLine* const fl = dotp->fileline();
                        const bool colon = dotp->colon();
                        AstNodeExpr* const lhs = VN_AS(dotp->lhsp()->unlinkFrBack(), NodeExpr);
                        AstNodeExpr* const rhsLhs
                            = VN_AS(rhsDotp->lhsp()->unlinkFrBack(), NodeExpr);
                        AstNodeExpr* const rhsRhs
                            = VN_AS(rhsDotp->rhsp()->unlinkFrBack(), NodeExpr);
                        FileLine* const rhsFl = rhsDotp->fileline();
                        const bool rhsColon = rhsDotp->colon();
                        AstDot* const newLhs = new AstDot{fl, colon, lhs, rhsLhs};
                        exprp = new AstDot{rhsFl, rhsColon, newLhs, rhsRhs};
                        VL_DO_DANGLING(dotp->deleteTree(), dotp);
                        dotp = VN_CAST(exprp, Dot);
                    }
                    dtypep = exprp;
                }
            } 否则 {
                dtypep = new AstVoidDType{nodep->fileline()};
            }

            AstNode* const newp = new AstParamTypeDType{
                nodep->fileline(), nodep->varType(),
                ptypep->fwdType(), nodep->name(),
                VFlagChildDType{}, new AstRequireDType{nodep->fileline(), dtypep}};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            返回;
        }
        const AstNodeDType* const dtypep = nodep->childDTypep();
        m_moduleWithGenericIface |= dtypep && VN_IS(dtypep->elemDTypep(false), IfaceGenericDType);

        // Maybe this variable has a signal attribute
        V3Control::applyVarAttr(m_modp, m_ftaskp, nodep);

        如果 (v3Global.opt.anyPublicFlat() && nodep->varType().isVPIAccessible()) {
            如果 (v3Global.opt.publicFlatRW()) {
                nodep->sigUserRWPublic(true);
            } 否则 如果 (v3Global.opt.publicParams() && nodep->isParam()) {
                nodep->sigUserRWPublic(true);
            } 否则 如果 (m_modp && v3Global.opt.publicDepth()) {
                如果 ((m_modp->depth() - 1) <= v3Global.opt.publicDepth()) {
                    nodep->sigUserRWPublic(true);
                } 否则 如果 (VN_IS(m_modp, Package) && nodep->isParam()) {
                    nodep->sigUserRWPublic(true);
                }
            }
        }

        // We used modTrace before leveling, and we may now
        // want to turn it off now that we know the levelizations
        如果 (v3Global.opt.traceDepth() && m_modp
            && (m_modp->depth() - 1) > v3Global.opt.traceDepth()) {
            m_modp->modTrace(false);
            nodep->trace(false);
        }
        m_varp = nodep;

        iterateChildren(nodep);
        m_varp = nullptr;
        // temporaries under an always aren't expected to be blocking
        如果 (m_procedurep && VN_IS(m_procedurep, Always))
            nodep->fileline()->modifyWarnOff(V3ErrorCode::BLKSEQ, true);
        // Compute initial value
        如果 (nodep->valuep() || nodep->needsCReset()) {
            FileLine* const fl = nodep->valuep() ? nodep->valuep()->fileline() : nodep->fileline();
            auto createValuep = [&]() -> AstNodeExpr* {
                如果 (nodep->valuep()) {
                    返回 VN_AS(nodep->valuep()->unlinkFrBack(), NodeExpr);
                } 否则 {
                    返回 new AstCReset{fl, nodep, false};
                }
            };
            // A variable can be in these positions related to having an initial value (or not):
            如果 (m_portDups.find(nodep) != m_portDups.end()) {
                // 0. Non-ANSI type declaration that is really a port, but we haven't resolved yet
                // Earlier moved any valuep() under the duplicate to the IO declaration
                UINFO(9, "VarInit case0 " << nodep);
            } 否则 如果 (nodep->isParam() || nodep->isGenVar()
                       || (m_ftaskp && (nodep->isNonOutput() || nodep->isFuncReturn()))
                       || (m_rsProdp && nodep->isNonOutput())) {
                // 1. Parameters, function inputs, and randsequence production formal
                // ports (IEEE 1800-2023 18.17.7): default to use if not overridden
                UINFO(9, "VarInit case1 " << nodep);
            } 否则 如果 (!m_ftaskp && !VN_IS(m_modp, Class) && nodep->isNonOutput()
                       && !nodep->isInput()) {
                // 2. Module inout/ref/constref: const default to use
                UINFO(9, "VarInit case2 " << nodep);
                如果 (nodep->valuep()) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: Default value on module inout/ref/constref: "
                                      << nodep->prettyNameQ());
                    nodep->valuep()->unlinkFrBack()->deleteTree();
                }
            } 否则 如果 (m_blockp || m_ftaskp) {
                // 3. Under blocks, it's an initial value to be under an assign
                UINFO(9, "VarInit case3 " << nodep);
                nodep->noCReset(true);
                FileLine* const newfl = new FileLine{fl};
                newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, true);
                AstAssign* const assp
                    = new AstAssign{newfl, new AstParseRef{newfl, nodep->name()}, createValuep()};
                如果 (nodep->lifetime().isAutomatic()) {
                    // May later make: new AstInitialAutomaticStmt{newfl, assp};
                    m_blockAddAutomaticStmtp = AstNode::addNext(m_blockAddAutomaticStmtp, assp);
                } 否则 {
                    // May later make: new AstInitialStaticStmt{newfl, assp};
                    m_blockAddStaticStmtp = AstNode::addNext(m_blockAddStaticStmtp, assp);
                }
            } 否则 如果 (m_valueModp) {
                // 4. Under modules/class, it's the time 0 initialziation value
                // Making an AstAssign (vs AstAssignW) to a wire is an error, suppress it
                UINFO(9, "VarInit case4 " << nodep);
                如果 (nodep->valuep()) {
                    nodep->noCReset(true);
                    FileLine* const newfl = new FileLine{fl};
                    newfl->warnOff(V3ErrorCode::PROCASSWIRE, true);
                    newfl->warnOff(V3ErrorCode::E_CONSTWRITTEN, true);
                    // Create a ParseRef to the wire. We cannot use the var as it may be deleted if
                    // it's a port (see t_var_set_link.v)
                    AstAssign* const assp = new AstAssign{
                        newfl, new AstParseRef{newfl, nodep->name()}, createValuep()};
                    AstNode* newInitp;
                    // Must make a unique InitialAutomatic/Static for each
                    // variable or otherwise V3Gate will assume ordering
                    // within, and not properly optimize.
                    如果 (nodep->lifetime().isAutomatic()) {
                        newInitp = new AstInitialAutomatic{newfl, assp};
                    } 否则 {
                        newInitp = new AstInitialStatic{newfl, assp};
                    }
                    nodep->addNextHere(newInitp);
                }
            } 否则 如果 (nodep->valuep()) {
                nodep->v3fatalSrc("Variable with initializer in unexpected position");
            }
        }
    }
    static bool isNegative(const AstConst* constp) {
        返回 constp->isSigned() && constp->num().bitIs1(constp->width() - 1);
    }
    void visit(AstRange* nodep) override {
        static constexpr uint64_t MAX_RANGE_ELEMENTS = 1 << 28;  // Same limit as V3Width
        // Literal bounds are checked here as V3Width sees only int-wrapped values; a literal
        // 32'hFFFF_FFFF reads there as -1, indistinguishable from a legitimate negative bound
        const AstConst* const leftp = VN_CAST(nodep->leftp(), Const);
        const AstConst* const rightp = VN_CAST(nodep->rightp(), Const);
        如果 (leftp && rightp && leftp->width() <= 64 && rightp->width() <= 64 && !isNegative(leftp)
            && !isNegative(rightp)) {
            const uint64_t left = leftp->toUQuad();
            const uint64_t right = rightp->toUQuad();
            const uint64_t span = std::max(left, right) - std::min(left, right);
            如果 (span >= MAX_RANGE_ELEMENTS) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << span + 1 << std::dec);
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstConst* nodep) override {
        如果 (nodep->num().autoExtend() && nodep->fileline()->language() < V3LangCode::L1800_2005) {
            nodep->v3warn(NEWERSTD, "Unbased unsized literals require IEEE 1800-2005 or later.");
        }
    }

    void visit(AstAttrOf* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        如果 (nodep->attrType() == VAttrType::DT_PUBLIC) {
            AstTypedef* const typep = VN_AS(nodep->backp(), Typedef);
            UASSERT_OBJ(typep, nodep, "Attribute not attached to typedef");
            typep->attrPublic(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FORCEABLE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->setForceable();
            v3Global.setHasForceableSignals();
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_PUBLIC) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(true);
            m_varp->sigModPublic(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RD) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRdPublic(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_PUBLIC_FLAT_RW) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->sigUserRWPublic(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_SFORMAT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrSFormat(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_SPLIT_VAR) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            如果 (!VN_IS(m_modp, Module)) {
                m_varp->v3warn(
                    SPLITVAR,
                    m_varp->prettyNameQ()
                        << " has split_var metacomment, "
                           "but will not be split because it is not declared in a module.");
            } 否则 {
                m_varp->attrSplitVar(true);
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(true);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(true);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FSM_ARC_INCLUDE_COND_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmArcInclCond(true);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FSM_STATE) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(true);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FSM_STATE_AUTO_EXPAND) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(true);
            m_varp->attrFsmStateExpand(VFsmExpandType::AUTO_EXPAND);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_FSM_STATE_FULL) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrFsmState(true);
            m_varp->attrFsmStateExpand(VFsmExpandType::FULL);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_SC_BIGUINT) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBigUint(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (nodep->attrType() == VAttrType::VAR_SC_BV) {
            UASSERT_OBJ(m_varp, nodep, "Attribute not attached to variable");
            m_varp->attrScBv(true);
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        }
    }

    void visit(AstDefImplicitDType* nodep) override {
        cleanFileline(nodep);
        UINFO(8, "   DEFIMPLICIT " << nodep);
        // Must remember what names we've already created, and combine duplicates
        // so that for "var enum {...} a,b" a & b will share a common typedef.
        // Change to unique name space per module so that an addition of
        // a new type won't change every verilated module.
        AstTypedef* defp = nullptr;
        const ImplTypedefMap::iterator it = m_implTypedef.find(nodep->name());
        如果 (it != m_implTypedef.end()) {
            defp = it->second;
            UINFO(9, "Reused impltypedef " << nodep << "  -->  " << defp);
        } 否则 {
            // Definition must be inserted right after the variable (etc) that needed it
            // AstVar, AstTypedef, AstNodeFTask, AstParamTypeDType are common containers
            AstNode* backp = nodep->backp();
            为 (; backp; backp = backp->backp()) {
                如果 (VN_IS(backp, Var) || VN_IS(backp, Typedef) || VN_IS(backp, NodeFTask)
                    || VN_IS(backp, ParamTypeDType))
                    break;
            }
            UASSERT_OBJ(backp, nodep,
                        "Implicit enum/struct type created under unexpected node type");
            AstNodeDType* const dtypep = nodep->childDTypep();
            dtypep->unlinkFrBack();
            如果 (VN_IS(backp, Typedef) || VN_IS(backp, ParamTypeDType)) {
                // A typedef doesn't need us to make yet another level of typedefing
                // For typedefs just remove the AstRefDType level of abstraction
                nodep->replaceWith(dtypep);
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
                返回;
            } 否则 {
                defp = new AstTypedef{nodep->fileline(), nodep->name(), nullptr, VFlagChildDType{},
                                      dtypep};
                m_implTypedef.emplace(defp->name(), defp);
                // Rename so that name doesn't change if a type is added/removed elsewhere
                // But the m_implTypedef is stil by old name so we can find it for next new lookups
                defp->name("__typeimpmod" + cvtToStr(m_implTypedef.size()));
                UINFO(9, "New impltypedef " << defp);
                backp->addNextHere(defp);
            }
        }
        nodep->replaceWith(new AstRefDType{nodep->fileline(), defp->name()});
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }

    void visit(AstDelay* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_modp, nodep, "Delay not under module");
        nodep->timeunit(m_modp->timeunit());
        iterateChildren(nodep);
    }

    void visit(AstNodeForeach* nodep) override {
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
        AstForeachHeader* const headerp = nodep->headerp();
        如果 (!headerp->elementsp()) {
            nodep->v3error("Foreach missing bracketed loop variable is no-operation"
                           " (IEEE 1800-2023 12.7.3)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            返回;
        }
        iterateChildren(nodep);
    }
    void visit(AstRepeat* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->stmtsp());
        iterateChildren(nodep);
    }
    void visit(AstLoop* nodep) override {
        cleanFileline(nodep);
        如果 (VN_IS(nodep->stmtsp(), LoopTest)) {
            checkIndent(nodep, nodep->stmtsp()->nextp());
        } 否则 {
            checkIndent(nodep, nodep->stmtsp());
        }
        iterateChildren(nodep);
    }
    void visit(AstWait* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        如果 (nodep->condp()->isZero()) {
            // Special case "wait(0)" we won't throw WAITCONST as user wrote
            // it that way with presumed intent - UVM does this.
            FileLine* const newfl = nodep->fileline();
            newfl->warnOff(V3ErrorCode::WAITCONST, true);
            nodep->fileline(newfl);
        }
    }
    void visit(AstNodeModule* nodep) override {
        V3Control::applyModule(nodep);
        ++m_statModules;
        如果 (VN_IS(nodep, Class) && VN_CAST(nodep, Class)->isInterfaceClass()
            && VN_IS(m_modp, Class) && VN_CAST(m_modp, Class)->isInterfaceClass()) {
            nodep->v3error("Interface class shall not be nested within another interface class."
                           " (IEEE 1800-2023 8.26)");
        }

        VL_RESTORER(m_modp);
        VL_RESTORER(m_inInterface);
        VL_RESTORER(m_anonUdpId);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblkNum);
        VL_RESTORER(m_beginDepth);
        VL_RESTORER(m_lifetime);
        VL_RESTORER(m_lifetimeAllowed);
        VL_RESTORER(m_moduleWithGenericIface);
        VL_RESTORER(m_randSequenceNum);
        VL_RESTORER(m_valueModp);
        VL_RESTORER_CLEAR(m_implTypedef);

        // Module: Create sim table for entire module and iterate
        cleanFileline(nodep);
        // Classes inherit from upper package
        如果 (m_modp && nodep->timeunit().isNone()) nodep->timeunit(m_modp->timeunit());
        m_modp = nodep;
        如果 (VN_IS(nodep, Iface)) m_inInterface = true;  // Start or stay within interface
        m_anonUdpId = 0;
        m_genblkAbove = 0;
        m_genblkNum = 0;
        m_beginDepth = 0;
        m_valueModp = nodep;
        m_lifetime = nodep->lifetime().makeImplicit();
        m_lifetimeAllowed = VN_IS(nodep, Class);
        m_moduleWithGenericIface = false;
        m_randSequenceNum = 0;

        如果 (m_lifetime.isNone()) {
            m_lifetime
                = VN_IS(nodep, Class) ? VLifetime::AUTOMATIC_IMPLICIT : VLifetime::STATIC_IMPLICIT;
        }
        如果 (nodep->name() == "TOP") {
            // May mess up scope resolution and cause infinite loop
            nodep->v3warn(E_UNSUPPORTED, "Module cannot be named 'TOP' as conflicts with "
                                         "Verilator top-level internals");
        }

        VL_RESTORER_COPY(m_portDups);
        collectPorts(nodep->stmtsp());

        iterateChildren(nodep);
        如果 (AstModule* const modp = VN_CAST(nodep, Module)) {
            modp->hasGenericIface(m_moduleWithGenericIface);
        }
    }
    void visitIterateNoValueMod(AstNode* nodep) {
        // Iterate a node which any Var within shouldn't create an InitialAutomatic procedure
        cleanFileline(nodep);
        VL_RESTORER(m_valueModp);
        m_valueModp = nullptr;
        iterateChildren(nodep);
    }
    void visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_lifetimeAllowed);
        m_lifetimeAllowed = true;
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        visitIterateNoValueMod(nodep);
    }
    void visit(AstCover* nodep) override { visitIterateNoValueMod(nodep); }
    void visit(AstRestrict* nodep) override { visitIterateNoValueMod(nodep); }

    void visit(AstGenBlock* nodep) override {
        V3Control::applyCoverageBlock(m_modp, nodep);
        cleanFileline(nodep);
        VL_RESTORER(m_beginDepth);
        ++m_beginDepth;
        const AstNode* const backp = nodep->backp();
        // IEEE says directly nested item is not a new block
        // The genblk name will get attached to the if true/false LOWER begin block(s)
        const bool nestedIf = nestedIfBegin(nodep);
        // It's not FOR(BEGIN(...)) but we earlier changed it to BEGIN(FOR(...))
        整数 assignGenBlkNum = -1;
        如果 (nodep->genforp()) {
            ++m_genblkNum;
            如果 (nodep->name() == "") assignGenBlkNum = m_genblkNum;
        } 否则 如果 (nodep->name() == "" && (VN_IS(backp, GenCaseItem) || VN_IS(backp, GenIf))
                   && !nestedIf) {
            assignGenBlkNum = m_genblkAbove;
        }
        如果 (assignGenBlkNum != -1) {
            nodep->name("genblk" + cvtToStr(assignGenBlkNum));
            如果 (nodep->itemsp()) {
                nodep->v3warn(GENUNNAMED,
                              "Unnamed generate block "
                                  << nodep->prettyNameQ() << " (IEEE 1800-2023 27.6)\n"
                                  << nodep->warnMore()
                                  << "... Suggest assign a label with 'begin : gen_<label_name>'");
            }
        }

        如果 (nodep->name() != "") {
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblkNum);
            m_genblkAbove = 0;
            m_genblkNum = 0;
            iterateChildren(nodep);
        } 否则 {
            iterateChildren(nodep);
        }
    }
    void visit(AstGenCase* nodep) override {
        ++m_genblkNum;
        cleanFileline(nodep);
        VL_RESTORER(m_genblkAbove);
        VL_RESTORER(m_genblkNum);
        m_genblkAbove = m_genblkNum;
        m_genblkNum = 0;
        iterateChildren(nodep);
    }
    void visit(AstGenIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        const bool nestedIf = (VN_IS(nodep->backp(), GenBlock)
                               && nestedIfBegin(VN_CAST(nodep->backp(), GenBlock)));
        如果 (nestedIf) {
            iterateChildren(nodep);
        } 否则 {
            ++m_genblkNum;
            VL_RESTORER(m_genblkAbove);
            VL_RESTORER(m_genblkNum);
            m_genblkAbove = m_genblkNum;
            m_genblkNum = 0;
            iterateChildren(nodep);
        }
    }
    void visit(AstCell* nodep) override {
        如果 (nodep->origName().empty()) {
            如果 (!VN_IS(nodep->modp(), Primitive)) {  // Module/Program/Iface
                nodep->modNameFileline()->v3error("Instance of " << nodep->modp()->verilogKwd()
                                                                 << " must be named");
            }
            // UDPs can have empty instance names. Assigning unique names for them to prevent any
            // conflicts
            const string newName = "$unnamedudp" + cvtToStr(++m_anonUdpId);
            nodep->name(newName);
            nodep->origName(newName);
        }
        iterateChildren(nodep);
    }
    void visit(AstRSProd* nodep) override {
        VL_RESTORER(m_rsProdp);
        m_rsProdp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstNodeBlock* nodep) override {
        VL_RESTORER(m_blockAddAutomaticStmtp);
        m_blockAddAutomaticStmtp = nullptr;
        VL_RESTORER(m_blockAddStaticStmtp);
        m_blockAddStaticStmtp = nullptr;
        {
            VL_RESTORER(m_blockp);
            m_blockp = nodep;
            iterateAndNextNull(nodep->declsp());
        }

        如果 (AstBegin* const beginp = VN_CAST(nodep, Begin)) {
            V3Control::applyCoverageBlock(m_modp, beginp);
        }
        cleanFileline(nodep);
        iterateAndNextNull(nodep->stmtsp());
        如果 (AstFork* const forkp = VN_CAST(nodep, Fork)) iterateAndNextNull(forkp->forksp());
        // Add statements created by AstVar vistor; can't do as-go because iteration
        // would then get confused, additionally we did already iterate the contents
        AstNode* stmtsp = nullptr;
        // Move all Vars to front of function
        stmtsp = AstNode::addNextNull(stmtsp, getVarsAndUnlink(nodep->stmtsp()));
        // Follow vars by m_blockAddp's (if any)
        stmtsp = AstNode::addNextNull(stmtsp, getBlockAdds());
        nodep->prependStmtsp(stmtsp);
    }
    void visit(AstCase* nodep) override {
        V3Control::applyCase(nodep);
        // Check for unsupported case matches (for tagged union)
        如果 (nodep->caseMatches()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: case matches (for tagged union)");
        }
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    void visit(AstCaseItem* nodep) override {
        // Move default caseItems to the bottom of the list
        // That saves us from having to search each case list twice, for non-defaults and defaults
        iterateChildren(nodep);
        如果 (!nodep->user2() && nodep->isDefault() && nodep->nextp()) {
            nodep->user2(true);
            AstNode* const nextp = nodep->nextp();
            nodep->unlinkFrBack();
            nextp->addNext(nodep);
        }
    }
    void visit(AstDot* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        如果 (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super"
            && VN_IS(nodep->rhsp(), New)) {
            // Look for other statements until hit function start
            AstNode* scanp = nodep;
            // Skip over the New's statement
            为 (; scanp && !VN_IS(scanp, StmtExpr); scanp = scanp->backp()) {}
            如果 (VN_IS(scanp, StmtExpr)) {  // Ignore warning if something not understood
                scanp = scanp->backp();
                为 (; scanp; scanp = scanp->backp()) {
                    如果 (VN_IS(scanp, NodeStmt) || VN_IS(scanp, NodeModule)
                        || VN_IS(scanp, NodeFTask))
                        break;
                }
                如果 (!VN_IS(scanp, NodeFTask)) {
                    nodep->rhsp()->v3error(
                        "'super.new' not first statement in new function (IEEE 1800-2023 8.15)\n"
                        << nodep->rhsp()->warnContextPrimary() << scanp->warnOther()
                        << "... Location of earlier statement\n"
                        << scanp->warnContextSecondary());
                }
            }
        }
    }
    void visit(AstIf* nodep) override {
        cleanFileline(nodep);
        checkIndent(nodep, nodep->elsesp() ? nodep->elsesp() : nodep->thensp());
        iterateChildren(nodep);
    }
    void visit(AstPrintTimeScale* nodep) override {
        // Inlining may change hierarchy, so just save timescale where needed
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->name(m_modp->name());
        nodep->timeunit(m_modp->timeunit());
    }
    void visit(AstRandSequence* nodep) override {
        cleanFileline(nodep);
        nodep->name("__Vrs" + std::to_string(m_randSequenceNum++));
        iterateChildren(nodep);
    }
    void visit(AstSFormatF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    void visit(AstSScanF* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    void visit(AstTime* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    void visit(AstTimeD* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    void visit(AstTimeImport* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    void visit(AstTimeUnit* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        nodep->timeunit(m_modp->timeunit());
    }
    void visit(AstEventControl* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        AstAlways* const alwaysp = VN_CAST(nodep->backp(), Always);
        如果 (alwaysp && alwaysp->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            alwaysp->v3error("Event control statements not legal under always_comb "
                             "(IEEE 1800-2023 9.2.2.2.2)\n"
                             << alwaysp->warnMore() << "... Suggest use a normal 'always'");
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        } 否则 如果 (alwaysp && !alwaysp->sentreep()) {
            // If the event control is at the top, move the sentree to the always
            如果 (AstSenTree* const sentreep = nodep->sentreep()) {
                sentreep->unlinkFrBackWithNext();
                alwaysp->sentreep(sentreep);
            }
            如果 (nodep->stmtsp()) alwaysp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        }
    }
    void visit(AstClocking* nodep) override {
        cleanFileline(nodep);
        VL_RESTORER(m_defaultInSkewp);
        VL_RESTORER(m_defaultOutSkewp);
        // Find default input and output skews
        为 (AstNode *nextp, *itemp = nodep->itemsp(); itemp; itemp = nextp) {
            nextp = itemp->nextp();
            如果 (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                如果 (citemp->exprp() || citemp->assignp()) continue;
                如果 (citemp->skewp()) {
                    如果 (citemp->direction() == VDirection::INPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        如果 (m_defaultInSkewp) {
                            citemp->skewp()->v3error("Multiple default input skews not allowed");
                        }
                        m_defaultInSkewp = citemp->skewp();
                    } 否则 如果 (citemp->direction() == VDirection::OUTPUT) {
                        // Disallow default redefinition; note some simulators allow this
                        如果 (m_defaultOutSkewp) {
                            citemp->skewp()->v3error("Multiple default output skews not allowed");
                        }
                        m_defaultOutSkewp = citemp->skewp();
                    } 否则 {
                        citemp->v3fatalSrc("Incorrect direction");
                    }
                }
                VL_DO_DANGLING2(pushDeletep(citemp->unlinkFrBack()), citemp, itemp);
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstClockingItem* nodep) override {
        cleanFileline(nodep);
        如果 (nodep->direction() == VDirection::OUTPUT) {
            如果 (!nodep->skewp()) {
                如果 (m_defaultOutSkewp) {
                    nodep->skewp(m_defaultOutSkewp->cloneTree(false));
                } 否则 {
                    // Default is 0 (IEEE 1800-2023 14.3)
                    nodep->skewp(new AstConst{nodep->fileline(), 0});
                }
            }
        } 否则 如果 (nodep->direction() == VDirection::INPUT) {
            如果 (!nodep->skewp()) {
                如果 (m_defaultInSkewp) {
                    nodep->skewp(m_defaultInSkewp->cloneTree(false));
                } 否则 {
                    // Default is 1step (IEEE 1800-2023 14.3)
                    nodep->skewp(new AstConst{nodep->fileline(), AstConst::OneStep{}});
                }
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstPackageImport* nodep) override {
        cleanFileline(nodep);
        如果 (m_modp && !m_ftaskp && VN_IS(m_modp, Class)) {
            nodep->v3error("Import statement directly within a class scope is illegal");
        }
        iterateChildren(nodep);
    }

    // Tagged union features - flag as unsupported early
    void visit(AstTaggedExpr* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    void visit(AstTaggedPattern* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    void visit(AstPatternVar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    void visit(AstPatternStar* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    void visit(AstMatches* nodep) override {
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        cleanFileline(nodep);
        iterateChildren(nodep);
    }
    void visit(AstBasicDType* nodep) override {
        // Check for void type used in tagged unions
        如果 (nodep->keyword() == VBasicDTypeKwd::CVOID) {
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
    static AstNode* makeArgMemberCopies(AstNode* argsp, bool readOnlyRefs) {
        AstNode* resultp = nullptr;
        为 (AstNode* argp = argsp; argp; argp = argp->nextp()) {
            AstVar* const origVarp = VN_AS(argp, Var);
            AstVar* const paramp = origVarp->cloneTree(false);
            FileLine* const fl = new FileLine{paramp->fileline()};
            fl->warnOff(V3ErrorCode::VARHIDDEN, true);
            paramp->fileline(fl);
            paramp->funcLocal(true);
            paramp->direction(origVarp->direction());
            如果 (readOnlyRefs && origVarp->isRef()) paramp->direction(VDirection::CONSTREF);
            resultp = AstNode::addNextNull(resultp, paramp);
            AstNodeExpr* const lhsp = new AstDot{
                origVarp->fileline(), false, new AstParseRef{origVarp->fileline(), "this"},
                new AstParseRef{origVarp->fileline(), origVarp->name()}};
            AstNodeExpr* const rhsp = new AstParseRef{paramp->fileline(), paramp->name()};
            resultp->addNext(new AstAssign{origVarp->fileline(), lhsp, rhsp});
        }
        返回 resultp;
    }

    // Create boilerplate covergroup methods on the given AstClass.
    // argsp/sampleArgsp are the raw arg lists still owned by the caller; they are iterated
    // (cloned) but not deleted here.
    static void createCovergroupMethods(AstClass* nodep, AstFunc* newFuncp, AstNode* argsp,
                                        AstNode* sampleArgsp) {
        // Hidden static to take unspecified reference argument results
        AstVar* const defaultVarp
            = new AstVar{nodep->fileline(), VVarType::MEMBER, "__Vint", nodep->findIntDType()};
        defaultVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->addStmtsp(defaultVarp);

        // Handle constructor arguments - add function parameters and assignments
        如果 (argsp) {
            UASSERT_OBJ(newFuncp, nodep,
                        "Covergroup class must have a 'new' constructor function");
            // The arg assignments run before the coverage body
            newFuncp->prependStmtsp(makeArgMemberCopies(argsp, true));
        }

        // IEEE: option / type_option members allow external access (cg_inst.option.X)
        // and require std:: types; std:: is kept because setUsesStdPackage() is called
        // at parse time for every covergroup declaration.
        {
            AstVar* const varp
                = new AstVar{nodep->fileline(), VVarType::MEMBER, "option", VFlagChildDType{},
                             new AstRefDType{nodep->fileline(), "vl_covergroup_options_t",
                                             new AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            nodep->addMembersp(varp);
        }
        {
            // IEEE 1800-2023 19.10: type_option is a static member, shared by all instances
            AstVar* const varp
                = new AstVar{nodep->fileline(), VVarType::MEMBER, "type_option", VFlagChildDType{},
                             new AstRefDType{nodep->fileline(), "vl_covergroup_type_options_t",
                                             new AstClassOrPackageRef{nodep->fileline(), "std",
                                                                      nullptr, nullptr},
                                             nullptr}};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            nodep->addMembersp(varp);
        }

        // IEEE: function void sample([arguments])
        {
            AstFunc* const funcp = new AstFunc{nodep->fileline(), "sample", nullptr, nullptr};
            funcp->addStmtsp(makeArgMemberCopies(sampleArgsp, false));
            funcp->classMethod(true);
            funcp->dtypep(funcp->findVoidDType());
            funcp->keepAlive(true);  // TODO create AstFuncRef and hold until findMethod("sample")
            nodep->addMembersp(funcp);
        }

        // IEEE: function void start(), void stop()
        为 (const string& name : {"start"s, "stop"s}) {
            AstFunc* const funcp = new AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->classMethod(true);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
        }

        // IEEE: static function real get_coverage(optional ref int, optional ref int)
        // IEEE: function real get_inst_coverage(optional ref int, optional ref int)
        为 (const string& name : {"get_coverage"s, "get_inst_coverage"s}) {
            AstFunc* const funcp = new AstFunc{nodep->fileline(), name, nullptr, nullptr};
            funcp->fileline()->warnOff(V3ErrorCode::NORETURN, true);
            funcp->isStatic(name == "get_coverage");
            funcp->classMethod(true);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            {
                AstVar* const varp = new AstVar{nodep->fileline(), VVarType::MEMBER, name,
                                                nodep->findDoubleDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(true);
                varp->direction(VDirection::OUTPUT);
                varp->funcReturn(true);
                funcp->fvarp(varp);
            }
            为 (const string& varname : {"covered_bins"s, "total_bins"s}) {
                AstVar* const varp = new AstVar{nodep->fileline(), VVarType::MEMBER, varname,
                                                nodep->findStringDType()};
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                varp->funcLocal(true);
                varp->direction(VDirection::INPUT);
                varp->valuep(new AstVarRef{nodep->fileline(), defaultVarp, VAccess::READ});
                funcp->addStmtsp(varp);
            }
        }

        // IEEE: function void set_inst_name(string)
        {
            AstFunc* const funcp
                = new AstFunc{nodep->fileline(), "set_inst_name", nullptr, nullptr};
            funcp->classMethod(true);
            funcp->dtypep(funcp->findVoidDType());
            nodep->addMembersp(funcp);
            AstVar* const varp = new AstVar{nodep->fileline(), VVarType::MEMBER, "name",
                                            nodep->findStringDType()};
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            varp->funcLocal(true);
            varp->direction(VDirection::INPUT);
            funcp->addStmtsp(varp);
        }
    }

    bool dropDeprecatedCoverageOption(AstCgOptionAssign* const nodep) {
        如果 (!(nodep->optType() == VCoverOptionType::CROSS_AUTO_BIN_MAX)) 返回 false;
        cleanFileline(nodep);
        nodep->v3warn(NONSTD, "Coverage option 'option."
                                  << nodep->optType().ascii()
                                  << "' is deprecated and ignored; it was removed from the "
                                     "IEEE LRM because it was poorly defined.");
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        返回 true;
    }

    void visit(AstCgOptionAssign* nodep) override {
        如果 (dropDeprecatedCoverageOption(nodep)) 返回;
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

    void visit(AstCovergroup* nodep) override {
        // AstCovergroup can only appear inside a module/class/package; never at root level.
        UASSERT_OBJ(m_modp, nodep, "AstCovergroup not under module");
        // If we're already inside a covergroup class, this is the sentinel AstCovergroup
        // node carrying the clocking event for V3Covergroup - don't re-transform it.
        如果 (VN_IS(m_modp, Class) && VN_AS(m_modp, Class)->isCovergroup()) 返回;

        // Transform raw parse-time AstCovergroup into a fully-formed AstClass
        cleanFileline(nodep);
        // Embedded, so of an anonymous type, the instance variable having the covergroup's name
        如果 (VN_IS(m_modp, Class)) nodep->name(AstClass::embeddedCovergroupTypeName(nodep->name()));

        const string libname = m_modp->libname();
        AstClass* const cgClassp = new AstClass{nodep->fileline(), nodep->name(), libname};
        cgClassp->fileline()->warnOff(V3ErrorCode::UNUSEDSIGNAL, true);
        cgClassp->fileline()->warnOff(V3ErrorCode::UNDRIVEN, true);
        cgClassp->isCovergroup(true);
        cgClassp->covergroupEnclosingClassp(VN_CAST(m_modp, Class));
        v3Global.useCovergroup(true);

        // Clocking event: unlink before deleteTree, attach as AstCovergroup child on class
        如果 (AstSenTree* const eventp = nodep->eventp()) {
            eventp->unlinkFrBack();
            AstCovergroup* const cgNodep = new AstCovergroup{
                nodep->fileline(), nodep->name(), nullptr, nullptr, nullptr, eventp};
            cgClassp->addMembersp(cgNodep);
        }

        // Convert constructor args to member variables
        为 (AstNode* argp = nodep->argsp(); argp; argp = argp->nextp()) {
            AstVar* const origVarp = VN_AS(argp, Var);
            如果 (origVarp->direction() == VDirection::OUTPUT
                || origVarp->direction() == VDirection::INOUT) {
                origVarp->v3error("Covergroup formal arguments cannot be output or inout"
                                  " (IEEE 1800-2023 19.3)");
                origVarp->direction(VDirection::INPUT);
            }
            如果 ((origVarp->isRef() || origVarp->isConstRef()) && origVarp->valuep()) {
                origVarp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: default value on ref or const ref covergroup "
                                 "formal argument");
            }
            AstVar* const memberp = origVarp->cloneTree(false);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(false);
            memberp->direction(VDirection::NONE);
            如果 (origVarp->isRef() || origVarp->isConstRef()) memberp->noReset(true);
            cgClassp->addMembersp(memberp);
        }

        // Convert sample args to member variables
        为 (AstNode* argp = nodep->sampleArgsp(); argp; argp = argp->nextp()) {
            AstVar* const origVarp = VN_AS(argp, Var);
            如果 (!origVarp->isInput()) {
                origVarp->v3error("Covergroup sample formal argument must have input direction "
                                  "(IEEE 1800-2023 19.8.1).");
                origVarp->direction(VDirection::INPUT);
            }
            AstVar* const memberp = origVarp->cloneTree(false);
            memberp->varType(VVarType::MEMBER);
            memberp->funcLocal(false);
            memberp->direction(VDirection::NONE);
            cgClassp->addMembersp(memberp);
        }

        // Create the constructor; detach membersp (coverage body) and use as its body
        AstFunc* const newFuncp = new AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        newFuncp->fileline()->warnOff(V3ErrorCode::NORETURN, true);
        newFuncp->classMethod(true);
        newFuncp->isConstructor(true);
        newFuncp->dtypep(cgClassp->dtypep());
        如果 (AstNode* const bodyp = nodep->membersp()) {
            bodyp->unlinkFrBackWithNext();
            newFuncp->addStmtsp(bodyp);
        }
        cgClassp->addMembersp(newFuncp);

        // Add all boilerplate covergroup methods (reads argsp/sampleArgsp from nodep)
        createCovergroupMethods(cgClassp, newFuncp, nodep->argsp(), nodep->sampleArgsp());

        // Replace AstCovergroup with AstClass and process the new class normally.
        // Reset the unnamed-coverpoint counter so synthesized names are stable and
        // independent of unrelated covergroups elsewhere in the file.
        VL_RESTORER(m_coverpointNum);
        VL_RESTORER_CLEAR(m_coverpoints);
        m_coverpointNum = 0;
        为 (const AstNode* stmtp = newFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            如果 (const AstCoverpoint* const pointp = VN_CAST(stmtp, Coverpoint)) {
                m_coverpoints.emplace(coverpointName(pointp), pointp);
            }
        }
        nodep->replaceWith(cgClassp);
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
        iterate(cgClassp);
    }

    // The name of a coverpoint, before visit(AstCoverpoint) names an unlabeled one: its label,
    // else that of its single variable (IEEE 1800-2023 19.5), else empty
    static string coverpointName(const AstCoverpoint* nodep) {
        如果 (!nodep->name().empty()) 返回 nodep->name();
        // A single-identifier coverpoint expression is the only form that parses to an
        // AstParseRef with a usable name here; a dotted/scoped/select/concatenation/call
        // expression is either a different node (so the cast yields null) or a name-less
        // AstParseRef (e.g. a member-select).
        const AstParseRef* const refp = VN_CAST(nodep->exprp(), ParseRef);
        返回 refp ? refp->name() : "";
    }

    // The coverpoint of the current covergroup a cross item names, or null
    const AstCoverpoint* findCoverpoint(const AstCoverpointRef* refp) const {
        如果 (refp->exprp()) 返回 nullptr;  // A hierarchical reference, unsupported
        const auto it = m_coverpoints.find(refp->name());
        返回 it == m_coverpoints.end() ? nullptr : it->second;
    }

    void visit(AstCoverpoint* nodep) override {
        cleanFileline(nodep);
        // Give every coverpoint a guaranteed-unique, deterministic name so all downstream
        // consumers (generated bin-variable names, the cross coverpoint map, hierarchical
        // report names) see a consistent identifier.  Unlabeled coverpoints arrive from the
        // grammar with an empty name; left empty, two of them in one covergroup collide on
        // the generated "__Vcov__<bin>" variable name (e.g. duplicate "__Vcov__auto_0").
        如果 (nodep->name().empty()) {
            // Compound expression (member/part select, concatenation, ...): synthesize
            // a unique name.  Leading "__V" keeps it out of the user namespace.
            const string name = coverpointName(nodep);
            nodep->name(name.empty() ? "__Vcoverpoint" + cvtToStr(m_coverpointNum++) : name);
        }
        // Re-sort the parse-time mixed bins list (AstCoverBin + AstCgOptionAssign)
        // into the typed binsp and optionsp slots.  The grammar attaches both node types
        // to binsp (op2) as a raw List[AstNode]; now that they are properly parented we
        // can iterate and split them without any temporary-parent tricks.
        为 (AstNode *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            如果 (AstCgOptionAssign* const optp = VN_CAST(itemp, CgOptionAssign)) {
                如果 (dropDeprecatedCoverageOption(optp)) continue;
                optp->unlinkFrBack();
                如果 (optp->optType() == VCoverOptionType::AT_LEAST
                    || optp->optType() == VCoverOptionType::AUTO_BIN_MAX
                    || optp->optType() == VCoverOptionType::WEIGHT) {
                    nodep->addOptionsp(new AstCoverOption{optp->fileline(), optp->typeOption(),
                                                          optp->optType(),
                                                          optp->valuep()->cloneTree(false)});
                } 否则 {
                    optp->v3warn(COVERIGN,
                                 "Ignoring unsupported coverage option: " + optp->prettyNameQ());
                }
                VL_DO_DANGLING(optp->deleteTree(), optp);
            }
        }
        VL_RESTORER(m_coverpointp);
        m_coverpointp = nodep;
        iterateChildren(nodep);
    }

    void visit(AstCoverBin* nodep) override {
        cleanFileline(nodep);
        如果 (!m_coverpointp && VN_IS(nodep->rangesp(), CoverWith)) {
            // A 'with' filter's candidates are of its coverpoint's type
            nodep->rangesp()->v3warn(COVERIGN, "Unsupported: 'with' in cover bin outside a "
                                               "coverpoint; bin "
                                                   << nodep->prettyNameQ() << " ignored");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            返回;
        }
        iterateChildren(nodep);
    }

    void visit(AstCoverWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_coverpointp, nodep, "Bin 'with' filter outside a coverpoint");
        如果 (const AstCoverpointRef* const refp = VN_CAST(nodep->subp(), CoverpointRef)) {
            如果 (refp->name() != m_coverpointp->name()) {
                refp->v3error("A bin 'with' filter may name only its own coverpoint "
                              << m_coverpointp->prettyNameQ() << ", not " << refp->prettyNameQ()
                              << " (IEEE 1800-2023 19.5.1.1)");
            }
        }
        // The candidate value, of the coverpoint's type, which a filter need not read.  The
        // standard names it, so it hides another 'item' in the filter without a warning.
        FileLine* const fl = nodep->fileline();
        FileLine* const flNoWarn = new FileLine{fl};
        flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
        flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, true);
        AstVar* const varp = new AstVar{flNoWarn, VVarType::VAR, "item", VFlagChildDType{},
                                        new AstRefDType{fl, AstRefDType::FlagTypeOfExpr{},
                                                        m_coverpointp->exprp()->cloneTree(false)}};
        varp->funcLocal(true);
        varp->noReset(true);
        varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        nodep->itemp(varp);
        iterateChildren(nodep);
    }

    void visit(AstCoverBinsof* nodep) override {
        cleanFileline(nodep);
        AstCoverpointRef* const refp = nodep->pointp();
        const AstParseRef* pointp = VN_CAST(refp->exprp(), ParseRef);
        const AstParseRef* binp = nullptr;
        const AstDot* const dotp = VN_CAST(refp->exprp(), Dot);
        如果 (dotp) {
            pointp = VN_CAST(dotp->lhsp(), ParseRef);
            binp = VN_CAST(dotp->rhsp(), ParseRef);
        }
        如果 (!pointp || (dotp && !binp)) {
            nodep->v3warn(COVERIGN, "Unsupported: 'binsof' in coverage select expression");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            返回;
        }
        // These names belong to the coverage namespace, not the sampled variables.
        如果 (binp) nodep->name(binp->name());
        refp->replaceWith(new AstCoverpointRef{pointp->fileline(), pointp->name()});
        VL_DO_DANGLING(pushDeletep(refp), refp);
        iterateChildren(nodep);
    }

    void visit(AstCoverCrossBin* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        如果 (!nodep->selectp()) {
            nodep->v3warn(COVERIGN, "Unsupported: explicit coverage cross bins");
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        }
    }

    void visit(AstCoverCrossSelect* nodep) override {
        cleanFileline(nodep);
        iterateChildren(nodep);
        如果 (!nodep->lhsp()
            || !nodep->rhsp()) {  // Due to earlier Unsupported errors dropping only one operand
                                  // would silently change the selected set.
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        }
    }

    void visit(AstCoverCross* nodep) override {
        cleanFileline(nodep);
        // Move options out of the mixed parse-time body, leaving only cross bins.
        为 (AstNode *itemp = nodep->binsp(), *nextp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            如果 (VN_IS(itemp, CoverCrossBin)) continue;
            AstCgOptionAssign* const optp = VN_AS(itemp, CgOptionAssign);
            如果 (dropDeprecatedCoverageOption(optp)) continue;
            itemp->unlinkFrBack();
            const VCoverOptionType optType = optp->optType();
            如果 (!(optType == VCoverOptionType::AT_LEAST || optType == VCoverOptionType::WEIGHT)) {
                optp->v3warn(COVERIGN,
                             "Ignoring unsupported coverage cross option: " + optp->prettyNameQ());
            }
            // Always preserve the option node so V3Coverage can track its source line
            // for coverage annotation, even when the option itself is unsupported.
            nodep->addOptionsp(new AstCoverOption{optp->fileline(), optp->typeOption(), optType,
                                                  optp->valuep()->cloneTree(false)});
            VL_DO_DANGLING(optp->deleteTree(), optp);
        }
        VL_RESTORER(m_crossp);
        m_crossp = nodep;
        iterateChildren(nodep);
    }

    void visit(AstCoverCrossWith* nodep) override {
        cleanFileline(nodep);
        UASSERT_OBJ(m_crossp, nodep, "Cross bin 'with' filter outside a cross");
        如果 (nodep->selectp()) {
            // The candidate value of each cross item, of its coverpoint's type (IEEE 1800-2023
            // 19.6.1.3).  The standard names them, so they hide others in the filter without a
            // warning.  A cross of an item that is not a coverpoint is unsupported (V3Covergroup).
            FileLine* const fl = nodep->fileline();
            FileLine* const flNoWarn = new FileLine{fl};
            flNoWarn->modifyWarnOff(V3ErrorCode::UNUSEDSIGNAL, true);
            flNoWarn->modifyWarnOff(V3ErrorCode::VARHIDDEN, true);
            std::set<string> names;
            为 (const AstNode* itemp = m_crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                const AstCoverpointRef* const refp = VN_AS(itemp, CoverpointRef);
                const AstCoverpoint* const pointp = findCoverpoint(refp);
                如果 (!pointp || !names.insert(refp->name()).second) continue;
                AstVar* const varp
                    = new AstVar{flNoWarn, VVarType::VAR, refp->name(), VFlagChildDType{},
                                 new AstRefDType{fl, AstRefDType::FlagTypeOfExpr{},
                                                 pointp->exprp()->cloneTree(false)}};
                varp->funcLocal(true);
                varp->noReset(true);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                nodep->addVarsp(varp);
            }
        }
        iterateChildren(nodep);
        // A filter of an unsupported selection, which was dropped, is dropped in turn
        如果 (!nodep->selectp()) VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    void visit(AstSystemCSection* nodep) override {
        如果 (nodep->user1SetOnce()) 返回;  // Process only once.
        cleanFileline(nodep);
        m_modp->addScSectionsp(nodep->unlinkFrBack());
    }

    void visit(AstNode* nodep) override {
        // Default: Just iterate
        cleanFileline(nodep);
        iterateChildren(nodep);
    }

public:
    // CONSTRUCTORS
    explicit LinkParseVisitor(AstNetlist* rootp) { iterate(rootp); }
    ~LinkParseVisitor() override {
        V3Stats::addStatSum(V3Stats::STAT_SOURCE_MODULES, m_statModules);
    }
};

//######################################################################
// Link class functions

void V3LinkParse::linkParse(AstNetlist* rootp) {
    UINFO(4, __FUNCTION__ << ": ");
    { LinkParseVisitor{rootp}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("linkparse", 0, dumpTreeEitherLevel() >= 6);
}
