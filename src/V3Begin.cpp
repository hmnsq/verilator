// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Removal of named begin blocks
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
// V3Begin's Transformations:
//
// Each module:
//      Look for BEGINs
//          BEGIN(VAR...) -> VAR ... {renamed}
//      FOR -> WHILEs
//      Move static function variables and their AstInitialStatic blocks before a function
//
// There are two scopes; named BEGINs change %m and variable scopes.
// Unnamed BEGINs change only variable, not $display("%m") scope.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Begin.h"

#ضمن "V3Stats.h"
#ضمن "V3String.h"
#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    ثابت VNUser1InUse m_inuser1;
    منطقي m_anyFuncInBegin = خطأ;

عام:
    BeginState() = افتراضي;
    ~BeginState() = افتراضي;
    فراغ userMarkChanged(AstNode* nodep) {
        nodep->user1(true);
        m_anyFuncInBegin = true;
    }
    منطقي anyFuncInBegin() ثابت { أرجع m_anyFuncInBegin; }
};

//######################################################################

صنف BeginVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* ثابت m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    AstNodeModule* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    AstNodeProcedure* m_procedurep = nullptr;  // Current procedure
    AstNode* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    صحيح m_ifDepth = 0;  // Current if depth
    منطقي m_keepBegins = خطأ;  // True if begins should not be inlined

    // METHODS

    string dot(ثابت string& a, ثابت string& b) { أرجع VString::dot(a, "__DOT__", b); }

    فراغ dotNames(ثابت std::string& name, FileLine* ثابت flp, ثابت حرف* ثابت blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        𐤀𐤌 (name == "") أرجع;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        طالما ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            ثابت std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            𐤀𐤌 (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            𐤀𐤌 (!m_ftaskp) {
                AstCellInline* ثابت inlinep = جديد AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = AstNode::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    فراغ liftNode(AstNode* nodep) {
        nodep->unlinkFrBack();
        𐤀𐤌 (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            𐤀𐤌 (!m_liftedp) {
                m_liftedp = nodep;
            } وإلا {
                m_liftedp->addNext(nodep);
            }
        } وإلا {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    فراغ visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    فراغ visit(AstNodeAssign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = true;
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        𐤀𐤌 (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->userMarkChanged(nodep);
        }
        VL_RESTORER_CLEAR(m_displayScope);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        VL_RESTORER(m_inlinesp);
        m_inlinesp = nullptr;
        iterateChildren(nodep);
        // Must be parsed before any AstCells, so put them first
        nodep->prependStmtsp(m_inlinesp);
    }
    فراغ visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeFTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        𐤀𐤌 (m_unnamedScope != "") {
            nodep->name(dot(m_unnamedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            m_statep->userMarkChanged(nodep);
        }
        // BEGIN wrapping a function rename that function, but don't affect
        // the inside function's variables.  We then restart with empty
        // naming; so that any begin's inside the function will rename
        // inside the function.
        // Process children
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_liftedp);
        VL_RESTORER_CLEAR(m_namedScope);
        VL_RESTORER_CLEAR(m_unnamedScope);
        m_displayScope = dot(m_displayScope, nodep->name());

        m_ftaskp = nodep;
        m_liftedp = nullptr;
        iterateChildren(nodep);
        nodep->foreach([&](AstInitialStatic* ثابت initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    فراغ visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        𐤀𐤌 (AstNode* ثابت itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = true;
            iterateAndNextNull(nodep->forksp());
        }
        AstNode* addsp = nullptr;
        𐤀𐤌 (AstNode* ثابت declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, declsp);
        }
        𐤀𐤌 (AstNode* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, stmtsp);
        }
        𐤀𐤌 (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    فراغ visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = خطأ;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        𐤀𐤌 (m_keepBegins) {
            nodep->name("");
            أرجع;
        }
        AstNode* addsp = nullptr;
        𐤀𐤌 (AstNode* ثابت declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, declsp);
        }
        𐤀𐤌 (AstNode* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, stmtsp);
        }
        𐤀𐤌 (addsp) {
            nodep->replaceWith(addsp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstNodeBlock* nodep) override {
        // Begin/Fork blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = VN_IS(nodep, Fork);
            dotNames(nodep->name(), nodep->fileline(),
                     VN_IS(nodep, Fork) ? "__FORK__" : "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        𐤀𐤌 (m_keepBegins) {
            nodep->name("");
            أرجع;
        }
        AstNode* addsp = nullptr;
        𐤀𐤌 (AstNode* ثابت declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, declsp);
        }
        𐤀𐤌 (AstNode* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, stmtsp);
        }
        𐤀𐤌 (addsp) {
            nodep->replaceWith(addsp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstVar* nodep) override {
        𐤀𐤌 (nodep->user1SetOnce()) { أرجع; }
        // If static variable, move it outside a function.
        𐤀𐤌 (nodep->lifetime().isStatic() && m_ftaskp) {
            ثابت std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            𐤀𐤌 (nodep->isIO()) {
                𐤀𐤌 (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* ثابت portp = nodep->cloneTreePure(خطأ);
                nodep->replaceWith(portp);

                𐤀𐤌 (nodep->isInput() || nodep->isInout()) {
                    AstAssign* ثابت initAssignp = جديد AstAssign{
                        nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        جديد AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                𐤀𐤌 (nodep->isWritable()) {
                    AstAssign* ثابت endAssignp = جديد AstAssign{
                        nodep->fileline(), جديد AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        جديد AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } وإلا {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(خطأ);
        } وإلا 𐤀𐤌 (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            liftNode(nodep);
        }
    }
    فراغ visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    فراغ visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        𐤀𐤌 (nodep->user1SetOnce()) أرجع;  // Don't double-add text's
        AstNode* wasUnderp = m_ftaskp;
        𐤀𐤌 (!m_ftaskp) wasUnderp = m_procedurep;
        𐤀𐤌 (wasUnderp) {
            𐤀𐤌 (nodep->stmtsp()) {
                AstNode* ثابت newp = جديد AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        } وإلا {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    فراغ visit(AstTypedef* nodep) override {
        𐤀𐤌 (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            liftNode(nodep);
        }
    }
    فراغ visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        𐤀𐤌 (m_namedScope != "") {
            m_statep->userMarkChanged(nodep);
            // Rename it
            nodep->name(dot(m_namedScope, nodep->name()));
            UINFO(8, "     rename to " << nodep->name());
            // Move to module
            nodep->unlinkFrBack();
            m_modp->addStmtsp(nodep);
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        𐤀𐤌 (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        𐤀𐤌 (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        𐤀𐤌 (nodep->user1SetOnce()) أرجع;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        ثابت std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        𐤀𐤌 (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeCoverDecl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    فراغ visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    فراغ visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = خطأ;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        𐤀𐤌 (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } وإلا 𐤀𐤌 (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } وإلا 𐤀𐤌 (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, true);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstNode* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = خطأ;
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = افتراضي;
};

//######################################################################

صنف BeginRelinkVisitor final : عام VNVisitorConst {
    // Replace tasks with new pointer
خاص:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    فراغ visit(AstNodeFTaskRef* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        𐤀𐤌 (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstVarRef* nodep) override {
        𐤀𐤌 (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        𐤀𐤌 (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = افتراضي;
};

//######################################################################
// Task class functions

فراغ V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        𐤀𐤌 (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("begin", 0, dumpTreeEitherLevel() >= 3);
}

ساكن AstNode* createForeachLoop(AstNodeForeach* /*nodep*/, AstNode* bodysp, منطقي arrayMayResize,
                                  AstNodeExpr* subfromp, AstVar* varp, AstNodeExpr* leftp,
                                  AstNodeExpr* rightp, VNType nodeType) {
    FileLine* ثابت fl = varp->fileline();
    AstNodeExpr* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
    AstNodeExpr* condp;
    منطقي inc = true;
    بدل (nodeType) {
    حالة VNType::LteS: condp = جديد AstLteS{fl, varRefp, rightp}; اكسر;
    حالة VNType::Lt: condp = جديد AstLt{fl, varRefp, rightp}; اكسر;
    حالة VNType::GteS:
        condp = جديد AstGteS{fl, varRefp, rightp};
        inc = خطأ;
        اكسر;
    افتراضي: varp->v3fatalSrc("Missing comparison handling"); اكسر;
    }
    AstNodeExpr* incp;
    𐤀𐤌 (inc)
        incp = جديد AstAdd{fl, varRefp->cloneTree(خطأ), جديد AstConst{fl, 1}};
    وإلا
        incp = جديد AstSub{fl, varRefp->cloneTree(خطأ), جديد AstConst{fl, 1}};

    AstLoop* ثابت loopp = جديد AstLoop{fl};
    loopp->addStmtsp(جديد AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    AstNodeExpr* sizeGetp = nullptr;
    𐤀𐤌 (arrayMayResize) {
        sizeVarp = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(true);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = جديد AstCMethodHard{fl, subfromp->cloneTreePure(خطأ), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(جديد AstAssign{fl, جديد AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    AstNodeStmt* incStmtp = جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, incp};
    𐤀𐤌 (arrayMayResize) {
        incStmtp = جديد AstIf{fl,
                             جديد AstLte{fl, جديد AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->cloneTreePure(خطأ)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    AstNode* ثابت stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    أرجع stmtsp;
}
ساكن AstNode* createForeachLoopRanged(AstNodeForeach* nodep, AstNode* bodysp,
                                        AstNodeExpr* subfromp, AstVar* varp,
                                        ثابت VNumRange& declRange) {
    FileLine* ثابت fl = varp->fileline();
    V3Number left{nodep, 32};
    V3Number right{nodep, 32};
    left.isSigned(true);
    right.isSigned(true);
    left.setLongS(declRange.left());
    right.setLongS(declRange.right());
    AstNodeExpr* ثابت leftp = جديد AstConst{fl, left};
    AstNodeExpr* ثابت rightp = جديد AstConst{fl, right};
    أرجع createForeachLoop(nodep, bodysp, خطأ /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.left() <= declRange.right() ? VNType::LteS : VNType::GteS);
}
ساكن AstNode* createForeachAssoc(FileLine* fl, AstVar* varp, AstNodeExpr* subfromp,
                                   AstNodeDType* fromDtp, AstNode* bodyPointp) {
    AstNode* loopp = varp;
    AstVar* ثابت next_varp  // Iterator containing next element (to handle mid-array delete)
        = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(true);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* ثابت more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = جديد AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(true);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    AstNodeExpr* ثابت firstp
        = جديد AstCMethodHard{fl, subfromp->cloneTreePure(خطأ), VCMethod::ASSOC_FIRST,
                             جديد AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    AstNodeExpr* ثابت nextp
        = جديد AstCMethodHard{fl, subfromp->cloneTreePure(خطأ), VCMethod::ASSOC_NEXT,
                             جديد AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(جديد AstAssign{fl, جديد AstVarRef{fl, more_varp, VAccess::WRITE},
                                 جديد AstNeq{fl, جديد AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* ثابت lp = جديد AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(جديد AstLoopTest{fl, lp, جديد AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(جديد AstAssign{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                جديد AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(جديد AstAssign{fl, جديد AstVarRef{fl, more_varp, VAccess::WRITE},
                                جديد AstNeq{fl, جديد AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    أرجع loopp;
}

ساكن منطقي arrayMayResizeCheck(AstNode* nodesp, AstVar* fromVarp) {
    // IEEE 1800-2023 12.7.3 "If the dimensions of a dynamically sized
    // array are changed while iterating over a foreach-loop construct, th
    // results are undefined".  However UVM calls delete within a foreach
    // in the uvm_reg code and in the tests, and expects the foreach to see
    // all elements post-delete.
    //
    // Cannot just check body for delete() because the body may call a
    // function that does a delete.  Cannot easily have the verilated_types
    // code set a flag on delete, because might be multiple foreach in
    // flight. Could use generation counter and each loop checks, but adds
    // time to every array modification. So, we assume if the array
    // shrinks, we should repeat the loop.
    𐤀𐤌 (!nodesp) أرجع خطأ;
    أرجع nodesp->existsAndNext([&](ثابت AstNode* nodep) -> منطقي {
        𐤀𐤌 (ثابت AstNodeVarRef* varrefp = VN_CAST(nodep, NodeVarRef)) {
            // Any variable written, as might have indirect ref to array
            𐤀𐤌 (varrefp->access().isWriteOrRW()) {
                𐤀𐤌 (!fromVarp) أرجع true;
                𐤀𐤌 (varrefp->varp() == fromVarp) أرجع true;
            }
        }
        // No idea what a function might be doing
        𐤀𐤌 (VN_IS(nodep, MethodCall) || VN_IS(nodep, NodeFTaskRef)) أرجع true;
        أرجع خطأ;
    });
}

AstNode* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    ثابت AstForeachHeader* ثابت headerp = nodep->headerp();
    AstNodeExpr* ثابت fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* ثابت fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    AstNodeDType* fromDtp = fromp->dtypep()->skipRefp();
    ثابت منطقي arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    AstNode* bodyPointp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, خطأ};
    AstNode* newp = nullptr;
    AstNode* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    AstNodeExpr* subfromp = fromp->cloneTreePure(خطأ);
    // Major dimension first
    لكل (AstNode *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        ثابت منطقي empty = VN_IS(argsp, Empty);
        AstVar* ثابت varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        𐤀𐤌 (varp) varp->unlinkFrBack()->usedLoopIdx(true);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        FileLine* ثابت fl = argsp->fileline();
        𐤀𐤌 (varp) {
            AstNode* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            𐤀𐤌 (ثابت AstNodeArrayDType* ثابت adtypep = VN_CAST(fromDtp, NodeArrayDType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } وإلا 𐤀𐤌 (ثابت AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                𐤀𐤌 (adtypep->isString()) {
                    AstConst* ثابت leftp = جديد AstConst{fl, 0};
                    AstNodeExpr* ثابت rightp = جديد AstLenN{fl, fromp->cloneTreePure(خطأ)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } وإلا {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } وإلا 𐤀𐤌 (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* ثابت leftp = جديد AstConst{fl, 0};
                AstNodeExpr* ثابت rightp = جديد AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), NodeArrayDType)
                        ? جديد AstArraySel{fl, subfromp->cloneTreePure(خطأ),
                                          جديد AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->cloneTreePure(خطأ),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(خطأ);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = جديد AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } وإلا 𐤀𐤌 (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = جديد AstVarRef{fl, varp, VAccess::READ};
                subfromp = جديد AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            𐤀𐤌 (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->deleteTree(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    AstNode* ثابت bodyp = nodep->bodyp();
    𐤀𐤌 (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        𐤀𐤌 (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->deleteTree(), bodyPointp);
        أرجع nullptr;
    }
    𐤀𐤌 (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } وإلا {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->deleteTree(), bodyPointp);
    VL_DO_DANGLING(nodep->deleteTree(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    أرجع newp;
}
