// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Control flow graph (CFG)
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
//
// Control flow graph (CFG) and related data structures.
//
// A control flow graph is a directed graph with with basic blocks as
// vertices connected by control flow edges. A basic block is a sequence
// of statements that are always executed as a unit (there are no branches
// targeting the middle of a basic block). The last statement in a basic
// block is called the terminator statemet. The terminator statements are
// the control flow transfer statements (branches) in the program. In this
// implementation, an unconditoinal jump (goto) is implicit and not stored
// in the basic block. A consequence of the implicit jump is that a basic
// block might be empty (not contain any statements). Conditional branches
// are represented by storing the corresponding conditional AstNodeStmt as
// the last statement in the basic block. Most importantly, only the actual
// condition check (e.g.: the 'condp' part of an AstIf) and branch is
// executed in the host basic block, but not any of the body statements of
// the conditoinal. The control flow graph has 2 unique basic blocks. The
// 'enter' block is the unique entry point of the represented program. It
// has no predecessors and itself dominates all other basic blocks. The
// 'exit' block is the unique exit point. It has no successors, and itself
// post-dominates all other basic blocks.
//
// The current implementation is designed to be immutable after
// construction. This can be relaxed in the future if necessary, however
// it is unlikely to be necessary as we cannot as of now convert a control
// flow graph back to Ast or any other form of output. We can also only
// represent 2-way conditionals, therefore all basic blocks have up to 2
// successors. The exit block has 0, blocks terminated by an unconditional
// implicit jump have exactly 1, and blocks terminated by a conditional
// branch have exactly 2 successors.
//
// Basic blocks have a unique ID, these are assigned based on the reverse
// post-ordering of the basic blocks within the control flow graph, and
// therefore they define a topological ordering of the basic blocks. The
// basic blocks within the graph are stored in this order. This means that
// in control flow graphs without back edges (loops), a basic block is
// always stored after its predecessors.
//
//*************************************************************************

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3CFG_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3CFG_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class Cfg_ನಕ್ಷೆ;
class Cfg_ಖಂಡ;
class Cfg_ಅಂಚು;

ಮಾದರಿ <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
class CfgMap;
ಮಾದರಿ <typename T_ಮೌಲ್ಯ>
class Cfg_ಖಂಡ_Map;
ಮಾದರಿ <typename T_ಮೌಲ್ಯ>
class Cfg_ಅಂಚು_Map;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class Cfg_ಖಂಡ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    ಮಿತ್ರ class Cfg_ನಕ್ಷೆ;
    ಮಾದರಿ <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
    ಮಿತ್ರ class CfgMap;
    ಮಿತ್ರ class CfgBuilder;

    // STATE
    Cfg_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_cfgp;  // The control flow graph this CfgBlock is under
    ಗಾತ್ರ_t m_rpo_ಸಂಖ್ಯೆ = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ಸಾಲೊಳಗೆ explicit Cfg_ಖಂಡ(Cfg_ನಕ್ಷೆ* cfgp);
    ~Cfg_ಖಂಡ() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    ಸಾಲೊಳಗೆ ಗಾತ್ರ_t id();
    ಸಾಲೊಳಗೆ ಗಾತ್ರ_t id() ಸ್ಥಿರಾಂಕ;

    // Is this the entry block of the CFG?
    bool isEnter() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಒಳ_ಖಾಲಿಗಳು(); }
    // Is this the exit block of the CFG?
    bool isExit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು_ಖಾಲಿಗಳು(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು_ಅಂಚುಗಳು().has_ಅಪವರ್ತ್ಯ_Elements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಒಳ_ಅಂಚುಗಳು().has_ಅಪವರ್ತ್ಯ_Elements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಒಳ_ಅಂಚುಗಳು().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    ಸಾಲೊಳಗೆ Cfg_ಅಂಚು* ತೆಗೆದುಕೊಂಡ_Edgep();
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* ತೆಗೆದುಕೊಂಡ_Edgep() ಸ್ಥಿರಾಂಕ;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    ಸಾಲೊಳಗೆ Cfg_ಅಂಚು* untknEdgep();
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* untknEdgep() ಸ್ಥಿರಾಂಕ;
    // The taken successor block, or nullptr if exit block
    ಸಾಲೊಳಗೆ Cfg_ಖಂಡ* takenp();
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* takenp() ಸ್ಥಿರಾಂಕ;
    // The untakens successor block, or nullptr if not a branch, or exit block
    ಸಾಲೊಳಗೆ Cfg_ಖಂಡ* untknp();
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* untknp() ಸ್ಥಿರಾಂಕ;

    // The first predecessor edge, or nullptr if enter block
    ಸಾಲೊಳಗೆ Cfg_ಅಂಚು* ಮೊದಲ_Predecessor_Edgep();
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* ಮೊದಲ_Predecessor_Edgep() ಸ್ಥಿರಾಂಕ;
    // The last predecessor edge, or nullptr if enter block
    ಸಾಲೊಳಗೆ Cfg_ಅಂಚು* ಕೊನೆಯ_Predecessor_Edgep();
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* ಕೊನೆಯ_Predecessor_Edgep() ಸ್ಥಿರಾಂಕ;
    // The first predecessor block, or nullptr if enter block
    ಸಾಲೊಳಗೆ Cfg_ಖಂಡ* ಮೊದಲ_Predecessorp();
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ಮೊದಲ_Predecessorp() ಸ್ಥಿರಾಂಕ;
    // The last predecessor block, or nullptr if enter block
    ಸಾಲೊಳಗೆ Cfg_ಖಂಡ* ಕೊನೆಯ_Predecessorp();
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ಕೊನೆಯ_Predecessorp() ಸ್ಥಿರಾಂಕ;

    // Statements in this CfgBlock
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು_Stmt*>& stmtps() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ id() < that.id(); }
    bool ಕ್ರಿಯಾಕಾರಕ>(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ id() > that.id(); }
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ this == &that; }

    // Iterators
    ಶೂನ್ಯ forEachSuccessor(std::function<ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&)> f) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಹೊರಹೋಗು_ಅಂಚುಗಳು()) f(*static_cast<Cfg_ಖಂಡ*>(ಅಂಚು.ತುದಿ()));
    }
    ಶೂನ್ಯ forEachPredecessor(std::function<ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ&)> f) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : ಒಳ_ಅಂಚುಗಳು()) f(*static_cast<Cfg_ಖಂಡ*>(ಅಂಚು.fromp()));
    }

    // Source location for debugging
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ !m_stmtps.ಖಾಲಿಗಳು() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ಸ್ಥಿರಾಂಕ override;
    std::string dotShape() ಸ್ಥಿರಾಂಕ override;
    std::string dotRank() ಸ್ಥಿರಾಂಕ override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class Cfg_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    ಮಿತ್ರ class Cfg_ನಕ್ಷೆ;
    ಮಾದರಿ <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
    ಮಿತ್ರ class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    Cfg_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ m_cfgp;  // The control flow graph this CfgEdge is under
    ಗಾತ್ರ_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ಸಾಲೊಳಗೆ Cfg_ಅಂಚು(Cfg_ನಕ್ಷೆ* graphp, Cfg_ಖಂಡ* srcp, Cfg_ಖಂಡ* dstp);
    ~Cfg_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    ಸಾಲೊಳಗೆ ಗಾತ್ರ_t id();
    ಸಾಲೊಳಗೆ ಗಾತ್ರ_t id() ಸ್ಥಿರಾಂಕ;

    // Source/destination CfgBlock
    ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* srcp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*>(fromp()); }
    ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* dstp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*>(ತುದಿ()); }
    Cfg_ಖಂಡ* srcp() { ಹಿಂತಿರುಗಿ static_cast<Cfg_ಖಂಡ*>(fromp()); }
    Cfg_ಖಂಡ* dstp() { ಹಿಂತಿರುಗಿ static_cast<Cfg_ಖಂಡ*>(ತುದಿ()); }

    // For Graphviz dumps only
    std::string dotLabel() ಸ್ಥಿರಾಂಕ override;
};

//######################################################################
// CfgGraph - The control flow graph

class Cfg_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    ಮಿತ್ರ class Cfg_ಖಂಡ;
    ಮಿತ್ರ class Cfg_ಅಂಚು;
    ಮಾದರಿ <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
    ಮಿತ್ರ class CfgMap;
    ಮಿತ್ರ class CfgBuilder;

    // STATE
    ಗಾತ್ರ_t m_nEdits = 0;  // Edit count of this graph
    ಗಾತ್ರ_t m_n_ಕೊನೆಯ_Ordered = 0;  // Last edit count blocks were ordered
    Cfg_ಖಂಡ* m_enterp = nullptr;  // The singular entry vertex
    Cfg_ಖಂಡ* m_exitp = nullptr;  // The singular exit vertex
    ಗಾತ್ರ_t m_n_ಖಂಡಗಳು = 0;  // Number of CfgBlocks in this CfgGraph
    ಗಾತ್ರ_t m_n_ಅಂಚುಗಳು = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    ಶೂನ್ಯ rpo_ಖಂಡಗಳು();

    // Add a new CfgBlock to this graph
    Cfg_ಖಂಡ* ಕೂಡಿಸು_ಖಂಡ() {
        ++m_nEdits;
        ++m_n_ಖಂಡಗಳು;
        ಹಿಂತಿರುಗಿ new Cfg_ಖಂಡ{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    ಶೂನ್ಯ ಕೂಡಿಸು_ತೆಗೆದುಕೊಂಡ_ಅಂಚು(Cfg_ಖಂಡ* srcp, Cfg_ಖಂಡ* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(!srcp->ತೆಗೆದುಕೊಂಡ_Edgep(), srcp, "Taken edge should be added first");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_ಅಂಚುಗಳು;
        new Cfg_ಅಂಚು{this, srcp, dstp};
    }

    // Add a new untaken CfgEdge to this CFG
    ಶೂನ್ಯ ಕೂಡಿಸು_Untkn_ಅಂಚು(Cfg_ಖಂಡ* srcp, Cfg_ಖಂಡ* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(srcp->ತೆಗೆದುಕೊಂಡ_Edgep(), srcp, "Untaken edge should be added second");
        UASSERT_OBJ(srcp->takenp() != dstp, srcp, "Untaken branch targets the same block");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_ಅಂಚುಗಳು;
        new Cfg_ಅಂಚು{this, srcp, dstp};
    }

    ಗಾತ್ರ_t id_ದ(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* bbp) ಸ್ಥಿರಾಂಕ {
        UASSERT_OBJ(m_nEdits == m_n_ಕೊನೆಯ_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        ಹಿಂತಿರುಗಿ bbp->m_rpo_ಸಂಖ್ಯೆ;
    }

    ಗಾತ್ರ_t id_ದ(ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* edgep) ಸ್ಥಿರಾಂಕ {
        UASSERT_OBJ(m_nEdits == m_n_ಕೊನೆಯ_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        ಹಿಂತಿರುಗಿ edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    Cfg_ಖಂಡ* getOrCreateTwoWayJoinFor(Cfg_ಖಂಡ* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    Cfg_ನಕ್ಷೆ() = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ~Cfg_ನಕ್ಷೆ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    ಸ್ಥಿರ std::unique_ptr<Cfg_ನಕ್ಷೆ> build(Ast_ಗಂಟು* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& enter() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ *m_enterp; }
    ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ& ನಿರ್ಗಮಿಸು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ *m_exitp; }

    // Number of basic blocks in this graph
    ಗಾತ್ರ_t n_ಖಂಡಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_n_ಖಂಡಗಳು; }
    // Number of control flow edges in this graph
    ಗಾತ್ರ_t n_ಅಂಚುಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_n_ಅಂಚುಗಳು; }

    // Create a CfgBlock map for this graph
    ಮಾದರಿ <typename T_ಮೌಲ್ಯ>
    ಸಾಲೊಳಗೆ Cfg_ಖಂಡ_Map<T_ಮೌಲ್ಯ> make_ಖಂಡ_Map() ಸ್ಥಿರಾಂಕ;
    // Create a CfgEdgeMap map for this graph
    ಮಾದರಿ <typename T_ಮೌಲ್ಯ>
    ಸಾಲೊಳಗೆ Cfg_ಅಂಚು_Map<T_ಮೌಲ್ಯ> make_ಅಂಚು_Map() ಸ್ಥಿರಾಂಕ;

    // Returns true iff the graph contains a loop (back-edge)
    bool ಒಳಗೊಂಡಿದೆ_Loop() ಸ್ಥಿರಾಂಕ;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    ಶೂನ್ಯ minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    ಶೂನ್ಯ break_Critical_ಅಂಚುಗಳು();

    bool ಸೇರಿಸು_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

ಮಾದರಿ <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ* m_cfgp;  // The control flow graph this map is for
    ಗಾತ್ರ_t m_created;  // Edit count of CFG this map was created at
    std::ಸದಿಶ<T_ಮೌಲ್ಯ> m_map;  // The map, stored as a vector

ಸಂರಕ್ಷಿತ:
    // CONSTRUCTOR
    explicit CfgMap(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ* cfgp, ಗಾತ್ರ_t ಗಾತ್ರ_2)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{ಗಾತ್ರ_2} {}

ಸಾರ್ವಜನಿಕ:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ಸ್ಥಿರಾಂಕ CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>&) = ಪೂರ್ವನಿಯೋಜಿತ;
    CfgMap(CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>&) = ಪೂರ್ವನಿಯೋಜಿತ;
    CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>& ಕ್ರಿಯಾಕಾರಕ=(CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>&&) = ಪೂರ್ವನಿಯೋಜಿತ;

    T_ಮೌಲ್ಯ& ಕ್ರಿಯಾಕಾರಕ[](ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ& ಕೀಲಿ) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ಕೀಲಿ.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ಹಿಂತಿರುಗಿ m_map.at(ಕೀಲಿ.id());
    }
    ಸ್ಥಿರಾಂಕ T_ಮೌಲ್ಯ& ಕ್ರಿಯಾಕಾರಕ[](ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ& ಕೀಲಿ) ಸ್ಥಿರಾಂಕ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ಕೀಲಿ.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ಹಿಂತಿರುಗಿ m_map.at(ಕೀಲಿ.id());
    }
    T_ಮೌಲ್ಯ& ಕ್ರಿಯಾಕಾರಕ[](ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ಹಿಂತಿರುಗಿ m_map.at(keyp->id());
    }
    ಸ್ಥಿರಾಂಕ T_ಮೌಲ್ಯ& ಕ್ರಿಯಾಕಾರಕ[](ಸ್ಥಿರಾಂಕ T_ಕೀಲಿ* keyp) ಸ್ಥಿರಾಂಕ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ಹಿಂತಿರುಗಿ m_map.at(keyp->id());
    }
};

ಮಾದರಿ <typename T_ಮೌಲ್ಯ>
class Cfg_ಖಂಡ_Map final : ಸಾರ್ವಜನಿಕ CfgMap<Cfg_ಖಂಡ, T_ಮೌಲ್ಯ> {
    ಮಿತ್ರ class Cfg_ನಕ್ಷೆ;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit Cfg_ಖಂಡ_Map(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ* cfgp)
        : CfgMap<Cfg_ಖಂಡ, T_ಮೌಲ್ಯ>{cfgp, cfgp->n_ಖಂಡಗಳು()} {}

ಸಾರ್ವಜನಿಕ:
    // Can create an empty map
    Cfg_ಖಂಡ_Map() = ಪೂರ್ವನಿಯೋಜಿತ;
    // Copyable, movable
    Cfg_ಖಂಡ_Map(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ_Map&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_ಖಂಡ_Map(Cfg_ಖಂಡ_Map&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_ಖಂಡ_Map& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ_Map&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_ಖಂಡ_Map& ಕ್ರಿಯಾಕಾರಕ=(Cfg_ಖಂಡ_Map&&) = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಮಾದರಿ <typename T_ಮೌಲ್ಯ>
class Cfg_ಅಂಚು_Map final : ಸಾರ್ವಜನಿಕ CfgMap<Cfg_ಅಂಚು, T_ಮೌಲ್ಯ> {
    ಮಿತ್ರ class Cfg_ನಕ್ಷೆ;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit Cfg_ಅಂಚು_Map(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ* cfgp)
        : CfgMap<Cfg_ಅಂಚು, T_ಮೌಲ್ಯ>{cfgp, cfgp->n_ಅಂಚುಗಳು()} {}

ಸಾರ್ವಜನಿಕ:
    // Can create an empty map
    Cfg_ಅಂಚು_Map() = ಪೂರ್ವನಿಯೋಜಿತ;
    // Copyable, movable
    Cfg_ಅಂಚು_Map(ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು_Map&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_ಅಂಚು_Map(Cfg_ಅಂಚು_Map&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_ಅಂಚು_Map& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು_Map&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_ಅಂಚು_Map& ಕ್ರಿಯಾಕಾರಕ=(Cfg_ಅಂಚು_Map&&) = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

Cfg_ಖಂಡ::Cfg_ಖಂಡ(Cfg_ನಕ್ಷೆ* cfgp)
    : V3_ನಕ್ಷೆ_ಶೃಂಗ{cfgp}
    , m_cfgp{cfgp} {}

ಗಾತ್ರ_t Cfg_ಖಂಡ::id() { ಹಿಂತಿರುಗಿ m_cfgp->id_ದ(this); }
ಗಾತ್ರ_t Cfg_ಖಂಡ::id() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cfgp->id_ದ(this); }

// Successor edges
Cfg_ಅಂಚು* Cfg_ಖಂಡ::ತೆಗೆದುಕೊಂಡ_Edgep() {  // It's always the first edge
    ಹಿಂತಿರುಗಿ isExit() ? nullptr : static_cast<Cfg_ಅಂಚು*>(ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp());
}
ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* Cfg_ಖಂಡ::ತೆಗೆದುಕೊಂಡ_Edgep() ಸ್ಥಿರಾಂಕ {  // It's always the first edge
    ಹಿಂತಿರುಗಿ isExit() ? nullptr : static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು*>(ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp());
}
Cfg_ಅಂಚು* Cfg_ಖಂಡ::untknEdgep() {  // It's always the second (last) edge
    ಹಿಂತಿರುಗಿ isBranch() ? static_cast<Cfg_ಅಂಚು*>(ಹೊರಹೋಗು_ಅಂಚುಗಳು().backp()) : nullptr;
}
ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* Cfg_ಖಂಡ::untknEdgep() ಸ್ಥಿರಾಂಕ {  // It's always the second (last) edge
    ಹಿಂತಿರುಗಿ isBranch() ? static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು*>(ಹೊರಹೋಗು_ಅಂಚುಗಳು().backp()) : nullptr;
}
Cfg_ಖಂಡ* Cfg_ಖಂಡ::takenp() {
    ಹಿಂತಿರುಗಿ isExit() ? nullptr : static_cast<Cfg_ಖಂಡ*>(ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp()->ತುದಿ());
}
ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* Cfg_ಖಂಡ::takenp() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ isExit() ? nullptr : static_cast<Cfg_ಖಂಡ*>(ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp()->ತುದಿ());
}
Cfg_ಖಂಡ* Cfg_ಖಂಡ::untknp() {
    ಹಿಂತಿರುಗಿ isBranch() ? static_cast<Cfg_ಖಂಡ*>(ಹೊರಹೋಗು_ಅಂಚುಗಳು().backp()->ತುದಿ()) : nullptr;
}
ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* Cfg_ಖಂಡ::untknp() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ isBranch() ? static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*>(ಹೊರಹೋಗು_ಅಂಚುಗಳು().backp()->ತುದಿ()) : nullptr;
}

// Predecessor edges
Cfg_ಅಂಚು* Cfg_ಖಂಡ::ಮೊದಲ_Predecessor_Edgep() {  //
    ಹಿಂತಿರುಗಿ static_cast<Cfg_ಅಂಚು*>(ಒಳ_ಅಂಚುಗಳು().frontp());
}
ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* Cfg_ಖಂಡ::ಮೊದಲ_Predecessor_Edgep() ಸ್ಥಿರಾಂಕ {  //
    ಹಿಂತಿರುಗಿ static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು*>(ಒಳ_ಅಂಚುಗಳು().frontp());
}
Cfg_ಅಂಚು* Cfg_ಖಂಡ::ಕೊನೆಯ_Predecessor_Edgep() {  //
    ಹಿಂತಿರುಗಿ static_cast<Cfg_ಅಂಚು*>(ಒಳ_ಅಂಚುಗಳು().backp());
}
ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು* Cfg_ಖಂಡ::ಕೊನೆಯ_Predecessor_Edgep() ಸ್ಥಿರಾಂಕ {  //
    ಹಿಂತಿರುಗಿ static_cast<ಸ್ಥಿರಾಂಕ Cfg_ಅಂಚು*>(ಒಳ_ಅಂಚುಗಳು().backp());
}
Cfg_ಖಂಡ* Cfg_ಖಂಡ::ಮೊದಲ_Predecessorp() {  //
    ಹಿಂತಿರುಗಿ isEnter() ? nullptr : ಮೊದಲ_Predecessor_Edgep()->srcp();
}
ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* Cfg_ಖಂಡ::ಮೊದಲ_Predecessorp() ಸ್ಥಿರಾಂಕ {  //
    ಹಿಂತಿರುಗಿ isEnter() ? nullptr : ಮೊದಲ_Predecessor_Edgep()->srcp();
}
Cfg_ಖಂಡ* Cfg_ಖಂಡ::ಕೊನೆಯ_Predecessorp() {  //
    ಹಿಂತಿರುಗಿ isEnter() ? nullptr : ಕೊನೆಯ_Predecessor_Edgep()->srcp();
}
ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* Cfg_ಖಂಡ::ಕೊನೆಯ_Predecessorp() ಸ್ಥಿರಾಂಕ {  //
    ಹಿಂತಿರುಗಿ isEnter() ? nullptr : ಕೊನೆಯ_Predecessor_Edgep()->srcp();
}

// --- CfgEdge ---

Cfg_ಅಂಚು::Cfg_ಅಂಚು(Cfg_ನಕ್ಷೆ* cfgp, Cfg_ಖಂಡ* srcp, Cfg_ಖಂಡ* dstp)
    : V3_ನಕ್ಷೆ_ಅಂಚು{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

ಗಾತ್ರ_t Cfg_ಅಂಚು::id() { ಹಿಂತಿರುಗಿ m_cfgp->id_ದ(this); }
ಗಾತ್ರ_t Cfg_ಅಂಚು::id() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cfgp->id_ದ(this); }

// --- CfgGraph ---

ಮಾದರಿ <typename T_ಮೌಲ್ಯ>
Cfg_ಖಂಡ_Map<T_ಮೌಲ್ಯ> Cfg_ನಕ್ಷೆ::make_ಖಂಡ_Map() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ Cfg_ಖಂಡ_Map<T_ಮೌಲ್ಯ>{this};
}
ಮಾದರಿ <typename T_ಮೌಲ್ಯ>
Cfg_ಅಂಚು_Map<T_ಮೌಲ್ಯ> Cfg_ನಕ್ಷೆ::make_ಅಂಚು_Map() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ Cfg_ಅಂಚು_Map<T_ಮೌಲ್ಯ>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_ವೃಕ್ಷ final {
    // STATE
    Cfg_ಖಂಡ_Map<ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* intersect(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ap, ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* bp);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    explicit Cfg_Dominator_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ& cfg);
    // Can create an empty map
    Cfg_Dominator_ವೃಕ್ಷ() = ಪೂರ್ವನಿಯೋಜಿತ;
    // Copyable, movable
    Cfg_Dominator_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Cfg_Dominator_ವೃಕ್ಷ&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_Dominator_ವೃಕ್ಷ(Cfg_Dominator_ವೃಕ್ಷ&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_Dominator_ವೃಕ್ಷ& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ Cfg_Dominator_ವೃಕ್ಷ&) = ಪೂರ್ವನಿಯೋಜಿತ;
    Cfg_Dominator_ವೃಕ್ಷ& ಕ್ರಿಯಾಕಾರಕ=(Cfg_Dominator_ವೃಕ್ಷ&&) = ಪೂರ್ವನಿಯೋಜಿತ;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* closestCommonDominator(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ap, ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* bp) ಸ್ಥಿರಾಂಕ {
        ಆಗ (ap != bp) {
            ಆದರೆ (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } ಇಲ್ಲದಿದ್ದರೆ {
                ap = m_bb2Idom[ap];
            }
        }
        ಹಿಂತಿರುಗಿ ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* ಸ್ಥಿರಾಂಕ ap, ಸ್ಥಿರಾಂಕ Cfg_ಖಂಡ* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        ಆಗ (true) {
            // True if 'ap' is above (or same as) 'bp'
            ಆದರೆ (ap == bp) ಹಿಂತಿರುಗಿ true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            ಆದರೆ (!bp) ಹಿಂತಿರುಗಿ false;
        }
    }
};

//######################################################################
// V3Cfg

namespace V3Cfg {

// Compute AstVarScopes live on entry to given CFG. That is, variables that might
// be read before wholly assigned in the CFG. Returns nullptr if the analysis
// failed due to unhandled statements involved in the CFG. The analysis is
// conservative and may return variables that are not actually live on entry.
// On success, returns a vector of AstVarScope nodes that might be live on entry.
std::unique_ptr<std::ಸದಿಶ<AstVarScope*>> ಜೀವಂತ_Var_Scopes(ಸ್ಥಿರಾಂಕ Cfg_ನಕ್ಷೆ&);

}  //namespace V3Cfg

#ಆದರೆ_ಅಂತ್ಯ  // VERILATOR_V3CFG_H_
