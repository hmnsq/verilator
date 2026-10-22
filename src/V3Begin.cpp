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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Begin.h"

#include "V3Stats.h"
#include "V3String.h"
#include "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

class BeginState final {
    // NODE STATE
    // Entire netlist:
    // AstNodeFTask::user1      -> bool, 1=processed
    const VNUser1InUse m_inuser1;
    bool m_anyFuncInBegin = false;

public:
    BeginState() = default;
    ~BeginState() = default;
    void userMarkChanged(AstNode* nodep) {
        nodep->user1(true);
        m_anyFuncInBegin = true;
    }
    bool anyFuncInBegin() const { ပြန်ပေး m_anyFuncInBegin; }
};

//######################################################################

class BeginVisitor final : public VNVisitor {
    // NODE STATE
    // AstCase::user1   -> bool, if already purified

    // STATE - across all visitors
    BeginState* const m_statep;  // Current global state

    // STATE - for current visit position (use VL_RESTORER)
    AstNodeModule* m_modp = nullptr;  // Current module
    AstCellInline* m_inlinesp = nullptr;  // AstCellInlines to add to m_modp
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    AstNodeProcedure* m_procedurep = nullptr;  // Current procedure
    AstNode* m_liftedp = nullptr;  // Local nodes we are lifting into m_ftaskp
    string m_displayScope;  // Name of %m in $display/AstScopeName
    string m_namedScope;  // Name of begin blocks above us
    string m_unnamedScope;  // Name of begin blocks, including unnamed blocks
    ကိန်းပြည့် m_ifDepth = 0;  // Current if depth
    bool m_keepBegins = false;  // True if begins should not be inlined

    // METHODS

    string dot(const string& a, const string& b) { ပြန်ပေး VString::dot(a, "__DOT__", b); }

    void dotNames(const std::string& name, FileLine* const flp, const var_na* const blockName) {
        UINFO(8, "nname " << m_namedScope);
        // If unneeded unnamed block, whatever that means :)
        အကယ်၍ (name == "") ပြန်ပေး;

        // Create data for dotted variable resolution
        std::string dottedname = name + "__DOT__";  // So always found
        std::string::size_type pos;
        နေစဉ် ((pos = dottedname.find("__DOT__")) != std::string::npos) {
            const std::string ident = dottedname.substr(0, pos);
            dottedname = dottedname.substr(pos + std::strlen("__DOT__"));
            အကယ်၍ (name != "") {
                m_displayScope = dot(m_displayScope, ident);
                m_namedScope = dot(m_namedScope, ident);
            }
            m_unnamedScope = dot(m_unnamedScope, ident);
            // Create CellInline for dotted var resolution
            အကယ်၍ (!m_ftaskp) {
                AstCellInline* const inlinep = new AstCellInline{flp, m_unnamedScope, blockName};
                m_inlinesp = AstNode::addNextNull(m_inlinesp, inlinep);
            }
        }
    }

    void liftNode(AstNode* nodep) {
        nodep->unlinkFrBack();
        အကယ်၍ (m_ftaskp) {
            // AstBegin under ftask, just move into the ftask
            အကယ်၍ (!m_liftedp) {
                m_liftedp = nodep;
            } မဟုတ်ပါက {
                m_liftedp->addNext(nodep);
            }
        } မဟုတ်ပါက {
            // Move to module
            m_modp->addStmtsp(nodep);
        }
    }

    // VISITORS
    void visit(AstForeach* nodep) override {
        VL_DO_DANGLING(V3Begin::convertToWhile(nodep), nodep);
    }
    void visit(AstNodeAssign* nodep) override {
        // Keep begin under assignment (in nodep->timingControlp())
        VL_RESTORER(m_keepBegins);
        m_keepBegins = true;
        iterateChildren(nodep);
    }
    void visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        // Rename it (e.g. class under a generate)
        အကယ်၍ (m_unnamedScope != "") {
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
    void visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        iterateChildren(nodep);
    }
    void visit(AstNodeFTask* nodep) override {
        UINFO(8, "  " << nodep);
        // Rename it
        အကယ်၍ (m_unnamedScope != "") {
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
        nodep->foreach([&](AstInitialStatic* const initp) {
            initp->unlinkFrBack();
            m_ftaskp->addHereThisAsNext(initp);
        });
        // Place lifted nodes at beginning of stmtsp, so Var nodes appear before referenced
        nodep->prependStmtsp(m_liftedp);
    }
    void visit(AstGenBlock* nodep) override {
        // GenBlocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        UASSERT_OBJ(!m_keepBegins, nodep, "Should be able to eliminate all AstGenBlock");
        dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
        iterateAndNextNull(nodep->itemsp());
        // Repalce node with body then delete
        အကယ်၍ (AstNode* const itemsp = nodep->itemsp()) {
            nodep->addNextHere(itemsp->unlinkFrBackWithNext());
        }
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    void visit(AstFork* nodep) override {
        dotNames(nodep->name(), nodep->fileline(), "__FORK__");
        iterateAndNextNull(nodep->stmtsp());
        {
            // Keep begins in forks to group their statements together
            VL_RESTORER(m_keepBegins);
            m_keepBegins = true;
            iterateAndNextNull(nodep->forksp());
        }
        AstNode* addsp = nullptr;
        အကယ်၍ (AstNode* const declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, declsp);
        }
        အကယ်၍ (AstNode* const stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, stmtsp);
        }
        အကယ်၍ (addsp) nodep->addHereThisAsNext(addsp);
        nodep->name("");
    }
    void visit(AstBegin* nodep) override {
        // Begin blocks were only useful in variable creation, change names and delete
        UINFO(8, "  " << nodep);
        VL_RESTORER_COPY(m_displayScope);
        VL_RESTORER_COPY(m_namedScope);
        VL_RESTORER_COPY(m_unnamedScope);
        {
            VL_RESTORER(m_keepBegins);
            m_keepBegins = false;
            dotNames(nodep->name(), nodep->fileline(), "__BEGIN__");
            iterateChildren(nodep);
        }

        // Cleanup
        အကယ်၍ (m_keepBegins) {
            nodep->name("");
            ပြန်ပေး;
        }
        AstNode* addsp = nullptr;
        အကယ်၍ (AstNode* const declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, declsp);
        }
        အကယ်၍ (AstNode* const stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, stmtsp);
        }
        အကယ်၍ (addsp) {
            nodep->replaceWith(addsp);
        } မဟုတ်ပါက {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstNodeBlock* nodep) override {
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
        အကယ်၍ (m_keepBegins) {
            nodep->name("");
            ပြန်ပေး;
        }
        AstNode* addsp = nullptr;
        အကယ်၍ (AstNode* const declsp = nodep->declsp()) {
            declsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, declsp);
        }
        အကယ်၍ (AstNode* const stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            addsp = AstNode::addNext(addsp, stmtsp);
        }
        အကယ်၍ (addsp) {
            nodep->replaceWith(addsp);
        } မဟုတ်ပါက {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstVar* nodep) override {
        အကယ်၍ (nodep->user1SetOnce()) { ပြန်ပေး; }
        // If static variable, move it outside a function.
        အကယ်၍ (nodep->lifetime().isStatic() && m_ftaskp) {
            const std::string newName
                = m_ftaskp->name() + "__Vstatic__" + dot(m_unnamedScope, nodep->name());
            အကယ်၍ (nodep->isIO()) {
                အကယ်၍ (nodep->direction().isRef()) {
                    nodep->v3error(
                        "It is illegal to use argument passing by reference for subroutines with "
                        "a lifetime of static (IEEE 1800-2023 13.5.2)");
                }
                // Create a port that is used for passing value between argument and static
                // variable
                AstVar* const portp = nodep->cloneTreePure(false);
                nodep->replaceWith(portp);

                အကယ်၍ (nodep->isInput() || nodep->isInout()) {
                    AstAssign* const initAssignp = new AstAssign{
                        nodep->fileline(), new AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                        new AstVarRef{portp->fileline(), portp, VAccess::READ}};
                    portp->addNextHere(initAssignp);
                }

                အကယ်၍ (nodep->isWritable()) {
                    AstAssign* const endAssignp = new AstAssign{
                        nodep->fileline(), new AstVarRef{portp->fileline(), portp, VAccess::WRITE},
                        new AstVarRef{nodep->fileline(), nodep, VAccess::READ}};
                    m_ftaskp->addStmtsp(endAssignp);
                }
            } မဟုတ်ပါက {
                nodep->unlinkFrBack();
            }
            nodep->name(newName);
            m_ftaskp->addHereThisAsNext(nodep);
            nodep->funcLocal(false);
        } မဟုတ်ပါက အကယ်၍ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            liftNode(nodep);
        }
    }
    void visit(AstInitialAutomaticStmt* nodep) override {
        // Automatic sets go at the current location
        nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstInitialStaticStmt* nodep) override {
        // As we moved static variables, move static initializers too
        အကယ်၍ (nodep->user1SetOnce()) ပြန်ပေး;  // Don't double-add text's
        AstNode* wasUnderp = m_ftaskp;
        အကယ်၍ (!m_ftaskp) wasUnderp = m_procedurep;
        အကယ်၍ (wasUnderp) {
            အကယ်၍ (nodep->stmtsp()) {
                AstNode* const newp = new AstInitialStatic{
                    nodep->fileline(), nodep->stmtsp()->unlinkFrBackWithNext()};
                wasUnderp->addHereThisAsNext(newp);
                iterateChildren(newp);
            }
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
        } မဟုတ်ပါက {
            nodep->v3fatalSrc("InitialStaticStmt under unexpected grand-parent");
        }
    }
    void visit(AstTypedef* nodep) override {
        အကယ်၍ (m_unnamedScope != "") {
            // Rename it
            nodep->name(dot(m_unnamedScope, nodep->name()));
            m_statep->userMarkChanged(nodep);
            // Move it under enclosing tree
            liftNode(nodep);
        }
    }
    void visit(AstCell* nodep) override {
        UINFO(8, "   CELL " << nodep);
        အကယ်၍ (m_namedScope != "") {
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
    void visit(AstVarXRef* nodep) override {
        UINFO(9, "   VARXREF " << nodep);
        အကယ်၍ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
    }
    void visit(AstNodeFTaskRef* nodep) override {
        UINFO(9, "   FTASKREF " << nodep);
        အကယ်၍ (m_namedScope != "" && nodep->inlinedDots() == "" && !m_ftaskp) {
            nodep->inlinedDots(m_namedScope);
            UINFO(9, "    rescope to " << nodep);
        }
        iterateChildren(nodep);
    }
    void visit(AstScopeName* nodep) override {
        // If there's a %m in the display text, we add a special node that will contain the name()
        // Similar code in V3Inline
        အကယ်၍ (nodep->user1SetOnce()) ပြန်ပေး;  // Don't double-add text's
        // DPI svGetScope doesn't include function name, but %m does
        const std::string scname = nodep->forFormat() ? m_displayScope : m_namedScope;
        // To keep correct visual order, must add before exising
        အကယ်၍ (!scname.empty()) nodep->scopeAttr("__DOT__"s + scname + nodep->scopeAttr());
        iterateChildren(nodep);
    }
    void visit(AstNodeCoverDecl* nodep) override {
        // Don't need to fix path in coverage statements, they're not under
        // any BEGINs, but V3Coverage adds them all under the module itself.
        iterateChildren(nodep);
    }
    void visit(AstCase* nodep) override { iterateChildren(nodep); }
    // VISITORS - LINT CHECK
    void visit(AstIf* nodep) override {  // not AstNodeIf; other types not covered
        VL_RESTORER(m_keepBegins);
        m_keepBegins = false;
        // Check IFDEPTH warning - could be in other transform files if desire
        VL_RESTORER(m_ifDepth);
        အကယ်၍ (m_ifDepth == -1 || v3Global.opt.ifDepth() < 1) {  // Turned off
        } မဟုတ်ပါက အကယ်၍ (nodep->uniquePragma() || nodep->unique0Pragma() || nodep->priorityPragma()) {
            m_ifDepth = -1;
        } မဟုတ်ပါက အကယ်၍ (++m_ifDepth > v3Global.opt.ifDepth()) {
            nodep->v3warn(IFDEPTH,
                          "Deep 'if' statement; suggest unique/priority to avoid slow logic");
            nodep->fileline()->modifyWarnOff(V3ErrorCode::IFDEPTH, true);  // Warn only once
            m_ifDepth = -1;
        }
        iterateChildren(nodep);
    }
    void visit(AstNode* nodep) override {
        VL_RESTORER(m_keepBegins);
        m_keepBegins = false;
        iterateChildren(nodep);
    }

public:
    // CONSTRUCTORS
    BeginVisitor(AstNetlist* nodep, BeginState* statep)
        : m_statep{statep} {
        iterate(nodep);
    }
    ~BeginVisitor() override = default;
};

//######################################################################

class BeginRelinkVisitor final : public VNVisitorConst {
    // Replace tasks with new pointer
private:
    // NODE STATE
    //  Input:
    //   AstNodeFTask::user1p           // bool.  Node replaced, rename it

    // VISITORS
    void visit(AstNodeFTaskRef* nodep) override {
        UASSERT_OBJ(nodep->taskp(), nodep, "unlinked");
        အကယ်၍ (nodep->taskp()->user1()) {  // It was converted
            UINFO(9, "    relinkFTask " << nodep);
            nodep->name(nodep->taskp()->name());
        }
        iterateChildrenConst(nodep);
    }
    void visit(AstVarRef* nodep) override {
        အကယ်၍ (nodep->varp()->user1()) {  // It was converted
            UINFO(9, "    relinVarRef " << nodep);
        }
        iterateChildrenConst(nodep);
    }
    void visit(AstIfaceRefDType* nodep) override {
        // May have changed cell names
        // TypeTable is always after all modules, so names are stable
        UINFO(8, "   IFACEREFDTYPE " << nodep);
        အကယ်၍ (nodep->cellp()) nodep->cellName(nodep->cellp()->name());
        UINFO(8, "       rename to " << nodep);
        iterateChildrenConst(nodep);
    }
    //--------------------
    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

public:
    // CONSTRUCTORS
    BeginRelinkVisitor(AstNetlist* nodep, BeginState*) { iterateConst(nodep); }
    ~BeginRelinkVisitor() override = default;
};

//######################################################################
// Task class functions

void V3Begin::debeginAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        BeginState state;
        { BeginVisitor{nodep, &state}; }
        အကယ်၍ (state.anyFuncInBegin()) BeginRelinkVisitor{nodep, &state};
    }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("begin", 0, dumpTreeEitherLevel() >= 3);
}

static AstNode* createForeachLoop(AstNodeForeach* /*nodep*/, AstNode* bodysp, bool arrayMayResize,
                                  AstNodeExpr* subfromp, AstVar* varp, AstNodeExpr* leftp,
                                  AstNodeExpr* rightp, VNType nodeType) {
    FileLine* const fl = varp->fileline();
    AstNodeExpr* varRefp = new AstVarRef{fl, varp, VAccess::READ};
    AstNodeExpr* condp;
    bool inc = true;
    switch (nodeType) {
    pak_sa VNType::LteS: condp = new AstLteS{fl, varRefp, rightp}; bha_nga;
    pak_sa VNType::Lt: condp = new AstLt{fl, varRefp, rightp}; bha_nga;
    pak_sa VNType::GteS:
        condp = new AstGteS{fl, varRefp, rightp};
        inc = false;
        bha_nga;
    default: varp->v3fatalSrc("Missing comparison handling"); bha_nga;
    }
    AstNodeExpr* incp;
    အကယ်၍ (inc)
        incp = new AstAdd{fl, varRefp->cloneTree(false), new AstConst{fl, 1}};
    မဟုတ်ပါက
        incp = new AstSub{fl, varRefp->cloneTree(false), new AstConst{fl, 1}};

    AstLoop* const loopp = new AstLoop{fl};
    loopp->addStmtsp(new AstLoopTest{fl, loopp, condp});
    AstVar* sizeVarp = nullptr;
    AstNodeExpr* sizeGetp = nullptr;
    အကယ်၍ (arrayMayResize) {
        sizeVarp = new AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vloopsize",
                              varp->findUInt32DType()};
        sizeVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        sizeVarp->usedLoopIdx(true);  // Not technically an index, but used only inside loop
        varp->addNext(sizeVarp);
        sizeGetp
            = new AstCMethodHard{fl, subfromp->cloneTreePure(false), VCMethod::DYN_SIZE, nullptr};
        sizeGetp->dtypeSetUInt32();
        loopp->addStmtsp(new AstAssign{fl, new AstVarRef{fl, sizeVarp, VAccess::WRITE}, sizeGetp});
    }
    loopp->addStmtsp(bodysp);
    AstNodeStmt* incStmtp = new AstAssign{fl, new AstVarRef{fl, varp, VAccess::WRITE}, incp};
    အကယ်၍ (arrayMayResize) {
        incStmtp = new AstIf{fl,
                             new AstLte{fl, new AstVarRef{fl, sizeVarp, VAccess::READ},
                                        sizeGetp->cloneTreePure(false)},
                             incStmtp};
    }
    loopp->addStmtsp(incStmtp);
    AstNode* const stmtsp = varp;  // New statements for outer loop
    stmtsp->addNext(new AstAssign{fl, new AstVarRef{fl, varp, VAccess::WRITE}, leftp});
    stmtsp->addNext(loopp);
    ပြန်ပေး stmtsp;
}
static AstNode* createForeachLoopRanged(AstNodeForeach* nodep, AstNode* bodysp,
                                        AstNodeExpr* subfromp, AstVar* varp,
                                        const VNumRange& declRange) {
    FileLine* const fl = varp->fileline();
    V3Number left{nodep, 32};
    V3Number right{nodep, 32};
    left.isSigned(true);
    right.isSigned(true);
    left.setLongS(declRange.left());
    right.setLongS(declRange.right());
    AstNodeExpr* const leftp = new AstConst{fl, left};
    AstNodeExpr* const rightp = new AstConst{fl, right};
    ပြန်ပေး createForeachLoop(nodep, bodysp, false /*fixed array size*/, subfromp, varp, leftp,
                             rightp,
                             declRange.left() <= declRange.right() ? VNType::LteS : VNType::GteS);
}
static AstNode* createForeachAssoc(FileLine* fl, AstVar* varp, AstNodeExpr* subfromp,
                                   AstNodeDType* fromDtp, AstNode* bodyPointp) {
    AstNode* loopp = varp;
    AstVar* const next_varp  // Iterator containing next element (to handle mid-array delete)
        = new AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vnext", varp};
    next_varp->usedLoopIdx(true);
    next_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(next_varp);

    AstVar* const more_varp  // bool var. 0 = loop empty/done, 1 = continue with loop
        = new AstVar{fl, VVarType::BLOCKTEMP, varp->name() + "__Vmore", VFlagBitPacked{}, 1};
    more_varp->usedLoopIdx(true);  // Not technically an index, but used only inside loop
    more_varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
    loopp->addNext(more_varp);

    AstNodeExpr* const firstp
        = new AstCMethodHard{fl, subfromp->cloneTreePure(false), VCMethod::ASSOC_FIRST,
                             new AstVarRef{fl, next_varp, VAccess::READWRITE}};
    firstp->dtypeSetInt();
    AstNodeExpr* const nextp
        = new AstCMethodHard{fl, subfromp->cloneTreePure(false), VCMethod::ASSOC_NEXT,
                             new AstVarRef{fl, next_varp, VAccess::READWRITE}};
    nextp->dtypeSetInt();

    // _Vmore = array.first(__Vnext)
    loopp->addNext(new AstAssign{fl, new AstVarRef{fl, more_varp, VAccess::WRITE},
                                 new AstNeq{fl, new AstConst{fl, 0}, firstp}});

    // LOOP(if (!_Vmore) break;  ...)
    AstLoop* const lp = new AstLoop{fl};
    loopp->addNext(lp);
    lp->addStmtsp(new AstLoopTest{fl, lp, new AstVarRef{fl, more_varp, VAccess::READ}});

    // index = __Vnext
    lp->addStmtsp(new AstAssign{fl, new AstVarRef{fl, varp, VAccess::WRITE},
                                new AstVarRef{fl, next_varp, VAccess::READ}});
    // _Vmore = array.next(__Vnext)
    lp->addStmtsp(new AstAssign{fl, new AstVarRef{fl, more_varp, VAccess::WRITE},
                                new AstNeq{fl, new AstConst{fl, 0}, nextp}});
    lp->addStmtsp(bodyPointp);
    ပြန်ပေး loopp;
}

static bool arrayMayResizeCheck(AstNode* nodesp, AstVar* fromVarp) {
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
    အကယ်၍ (!nodesp) ပြန်ပေး false;
    ပြန်ပေး nodesp->existsAndNext([&](const AstNode* nodep) -> bool {
        အကယ်၍ (const AstNodeVarRef* varrefp = VN_CAST(nodep, NodeVarRef)) {
            // Any variable written, as might have indirect ref to array
            အကယ်၍ (varrefp->access().isWriteOrRW()) {
                အကယ်၍ (!fromVarp) ပြန်ပေး true;
                အကယ်၍ (varrefp->varp() == fromVarp) ပြန်ပေး true;
            }
        }
        // No idea what a function might be doing
        အကယ်၍ (VN_IS(nodep, MethodCall) || VN_IS(nodep, NodeFTaskRef)) ပြန်ပေး true;
        ပြန်ပေး false;
    });
}

AstNode* V3Begin::convertToWhile(AstForeach* nodep) {
    // UINFOTREE(1, nodep, "", "foreach-old");
    const AstForeachHeader* const headerp = nodep->headerp();
    AstNodeExpr* const fromp = headerp->fromp();
    UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
    AstVar* const fromVarp = VN_IS(fromp, VarRef) ? VN_CAST(fromp, VarRef)->varp() : nullptr;
    AstNodeDType* fromDtp = fromp->dtypep()->skipRefp();
    const bool arrayMayResize = arrayMayResizeCheck(nodep->bodyp(), fromVarp);
    // Split into for loop
    // We record where the body needs to eventually go with bodyPointp
    AstNode* bodyPointp = new AstBegin{nodep->fileline(), "[EditWrapper]", nullptr, false};
    AstNode* newp = nullptr;
    AstNode* lastp = nodep;
    AstVar* nestedIndexp = nullptr;
    // subfromp used to traverse each dimension of multi-d variable-sized unpacked array (queue,
    // dyn-arr and associative-arr)
    AstNodeExpr* subfromp = fromp->cloneTreePure(false);
    // Major dimension first
    အတွက် (AstNode *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
        next_argsp = argsp->nextp();
        const bool empty = VN_IS(argsp, Empty);
        AstVar* const varp = VN_CAST(argsp, Var);
        UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
        အကယ်၍ (varp) varp->unlinkFrBack()->usedLoopIdx(true);
        UASSERT_OBJ(fromDtp, argsp, "more loop vars than dimensions");
        fromDtp = fromDtp->skipRefp();

        FileLine* const fl = argsp->fileline();
        အကယ်၍ (varp) {
            AstNode* loopp = nullptr;
            VNRelinker handle;
            lastp->unlinkFrBack(&handle);
            အကယ်၍ (const AstNodeArrayDType* const adtypep = VN_CAST(fromDtp, NodeArrayDType)) {
                loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                adtypep->declRange());
            } မဟုတ်ပါက အကယ်၍ (const AstBasicDType* const adtypep = VN_CAST(fromDtp, BasicDType)) {
                အကယ်၍ (adtypep->isString()) {
                    AstConst* const leftp = new AstConst{fl, 0};
                    AstNodeExpr* const rightp = new AstLenN{fl, fromp->cloneTreePure(false)};
                    loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp,
                                              leftp, rightp, VNType::Lt);
                } မဟုတ်ပါက {
                    UASSERT_OBJ(adtypep->isRanged(), varp, "foreach on basic " << adtypep);
                    loopp = createForeachLoopRanged(nodep, bodyPointp, subfromp, varp,
                                                    adtypep->declRange());
                }
            } မဟုတ်ပါက အကယ်၍ (VN_IS(fromDtp, DynArrayDType) || VN_IS(fromDtp, QueueDType)) {
                AstConst* const leftp = new AstConst{fl, 0};
                AstNodeExpr* const rightp = new AstCMethodHard{
                    fl,
                    VN_IS(subfromp->dtypep(), NodeArrayDType)
                        ? new AstArraySel{fl, subfromp->cloneTreePure(false),
                                          new AstVarRef{fl, nestedIndexp, VAccess::READ}}
                        : subfromp->cloneTreePure(false),
                    VCMethod::DYN_SIZE};
                AstVarRef* varRefp = new AstVarRef{fl, varp, VAccess::READ};
                rightp->dtypeSetInt();
                rightp->protect(false);
                loopp = createForeachLoop(nodep, bodyPointp, arrayMayResize, subfromp, varp, leftp,
                                          rightp, VNType::Lt);
                subfromp = new AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            } မဟုတ်ပါက အကယ်၍ (VN_IS(fromDtp, AssocArrayDType)) {
                // Make this: var KEY_TYPE index;
                //            bit index__Vfirst;
                //            index__Vfirst = 0;
                //            if (0 != array.first(index))
                //                 do body while (index__Vfirst || 0 != array.next(index))
                loopp = createForeachAssoc(fl, varp, subfromp, fromDtp, bodyPointp);
                AstVarRef* varRefp = new AstVarRef{fl, varp, VAccess::READ};
                subfromp = new AstCMethodHard{fl, subfromp, VCMethod::ARRAY_AT, varRefp};
                subfromp->dtypep(fromDtp);
            }
            UASSERT_OBJ(loopp, argsp, "unable to foreach " << fromDtp);
            // New loop goes UNDER previous loop
            handle.relink(loopp);
            lastp = bodyPointp;
            အကယ်၍ (!newp) newp = loopp;
        }
        // Prep for next
        nestedIndexp = varp;
        fromDtp = fromDtp->subDTypep();
    }
    VL_DO_DANGLING(subfromp->deleteTree(), subfromp);
    // The parser validates we don't have "foreach (array[,,,])"
    AstNode* const bodyp = nodep->bodyp();
    အကယ်၍ (!newp) {
        nodep->v3warn(NOEFFECT, "foreach with no loop variable has no effect");
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
        အကယ်၍ (!bodyPointp->backp()) VL_DO_DANGLING(bodyPointp->deleteTree(), bodyPointp);
        ပြန်ပေး nullptr;
    }
    အကယ်၍ (bodyp) {
        bodyPointp->replaceWith(bodyp->unlinkFrBackWithNext());
    } မဟုတ်ပါက {
        bodyPointp->unlinkFrBack();
    }
    VL_DO_DANGLING(bodyPointp->deleteTree(), bodyPointp);
    VL_DO_DANGLING(nodep->deleteTree(), nodep);
    // UINFOTREE(1, newp, "", "foreach-new");
    ပြန်ပေး newp;
}
