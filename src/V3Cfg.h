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

#ifndef VERILATOR_V3CFG_H_
#define VERILATOR_V3CFG_H_

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

class CfgGraph;
class CfgBlock;
class CfgEdge;

template <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
class CfgMap;
template <typename T_ಮೌಲ್ಯ>
class CfgBlockMap;
template <typename T_ಮೌಲ್ಯ>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    friend class CfgGraph;
    template <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    CfgGraph* ಧ್ರುವ m_cfgp;  // The control flow graph this CfgBlock is under
    size_t m_rpo_ಸಂಖ್ಯೆ = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_ಗಂಟು_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ಅನ್ತಃಸ್ಥ explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    ಅನ್ತಃಸ್ಥ size_t id();
    ಅನ್ತಃಸ್ಥ size_t id() ಧ್ರುವ;

    // Is this the entry block of the CFG?
    bool isEnter() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    ಅನ್ತಃಸ್ಥ CfgEdge* takenEdgep();
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ CfgEdge* takenEdgep() ಧ್ರುವ;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    ಅನ್ತಃಸ್ಥ CfgEdge* untknEdgep();
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ CfgEdge* untknEdgep() ಧ್ರುವ;
    // The taken successor block, or nullptr if exit block
    ಅನ್ತಃಸ್ಥ CfgBlock* takenp();
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ CfgBlock* takenp() ಧ್ರುವ;
    // The untakens successor block, or nullptr if not a branch, or exit block
    ಅನ್ತಃಸ್ಥ CfgBlock* untknp();
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ CfgBlock* untknp() ಧ್ರುವ;

    // The first predecessor edge, or nullptr if enter block
    ಅನ್ತಃಸ್ಥ CfgEdge* firstPredecessorEdgep();
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ CfgEdge* firstPredecessorEdgep() ಧ್ರುವ;
    // The last predecessor edge, or nullptr if enter block
    ಅನ್ತಃಸ್ಥ CfgEdge* lastPredecessorEdgep();
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ CfgEdge* lastPredecessorEdgep() ಧ್ರುವ;
    // The first predecessor block, or nullptr if enter block
    ಅನ್ತಃಸ್ಥ CfgBlock* firstPredecessorp();
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ CfgBlock* firstPredecessorp() ಧ್ರುವ;
    // The last predecessor block, or nullptr if enter block
    ಅನ್ತಃಸ್ಥ CfgBlock* lastPredecessorp();
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ CfgBlock* lastPredecessorp() ಧ್ರುವ;

    // Statements in this CfgBlock
    ಧ್ರುವ std::vector<Ast_ಗಂಟು_Stmt*>& stmtps() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool operator<(ಧ್ರುವ CfgBlock& that) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ id() < that.id(); }
    bool operator>(ಧ್ರುವ CfgBlock& that) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ id() > that.id(); }
    bool operator==(ಧ್ರುವ CfgBlock& that) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ this == &that; }

    // Iterators
    ಶೂನ್ಯ forEachSuccessor(std::function<ಶೂನ್ಯ(ಧ್ರುವ CfgBlock&)> f) ಧ್ರುವ {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.ತುದಿ()));
    }
    ಶೂನ್ಯ forEachPredecessor(std::function<ಶೂನ್ಯ(ಧ್ರುವ CfgBlock&)> f) ಧ್ರುವ {
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_ಸಾಲು* fileline() ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ಧ್ರುವ override;
    std::string dotShape() ಧ್ರುವ override;
    std::string dotRank() ಧ್ರುವ override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : ಸಾರ್ವಜನಿಕ V3GraphEdge {
    friend class CfgGraph;
    template <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
    friend class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* ಧ್ರುವ m_cfgp;  // The control flow graph this CfgEdge is under
    size_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ಅನ್ತಃಸ್ಥ CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    ಅನ್ತಃಸ್ಥ size_t id();
    ಅನ್ತಃಸ್ಥ size_t id() ಧ್ರುವ;

    // Source/destination CfgBlock
    ಧ್ರುವ CfgBlock* srcp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ static_cast<ಧ್ರುವ CfgBlock*>(fromp()); }
    ಧ್ರುವ CfgBlock* dstp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ static_cast<ಧ್ರುವ CfgBlock*>(ತುದಿ()); }
    CfgBlock* srcp() { ಪ್ರತ್ಯಾಗಮ static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { ಪ್ರತ್ಯಾಗಮ static_cast<CfgBlock*>(ತುದಿ()); }

    // For Graphviz dumps only
    std::string dotLabel() ಧ್ರುವ override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
    friend class CfgBlock;
    friend class CfgEdge;
    template <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    size_t m_nEdits = 0;  // Edit count of this graph
    size_t m_nLastOrdered = 0;  // Last edit count blocks were ordered
    CfgBlock* m_enterp = nullptr;  // The singular entry vertex
    CfgBlock* m_exitp = nullptr;  // The singular exit vertex
    size_t m_nBlocks = 0;  // Number of CfgBlocks in this CfgGraph
    size_t m_nEdges = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    ಶೂನ್ಯ rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        ಪ್ರತ್ಯಾಗಮ new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    ಶೂನ್ಯ addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(!srcp->takenEdgep(), srcp, "Taken edge should be added first");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_nEdges;
        new CfgEdge{this, srcp, dstp};
    }

    // Add a new untaken CfgEdge to this CFG
    ಶೂನ್ಯ addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(srcp->takenEdgep(), srcp, "Untaken edge should be added second");
        UASSERT_OBJ(srcp->takenp() != dstp, srcp, "Untaken branch targets the same block");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_nEdges;
        new CfgEdge{this, srcp, dstp};
    }

    size_t idOf(ಧ್ರುವ CfgBlock* bbp) ಧ್ರುವ {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        ಪ್ರತ್ಯಾಗಮ bbp->m_rpo_ಸಂಖ್ಯೆ;
    }

    size_t idOf(ಧ್ರುವ CfgEdge* edgep) ಧ್ರುವ {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        ಪ್ರತ್ಯಾಗಮ edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ~CfgGraph() override = ಸ್ವಭಾವ;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    ಸ್ಥಿರ std::unique_ptr<CfgGraph> build(Ast_ಗಂಟು* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ಧ್ರುವ CfgBlock& enter() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ *m_enterp; }
    ಧ್ರುವ CfgBlock& exit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ *m_exitp; }

    // Number of basic blocks in this graph
    size_t nBlocks() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nBlocks; }
    // Number of control flow edges in this graph
    size_t nEdges() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nEdges; }

    // Create a CfgBlock map for this graph
    template <typename T_ಮೌಲ್ಯ>
    ಅನ್ತಃಸ್ಥ CfgBlockMap<T_ಮೌಲ್ಯ> makeBlockMap() ಧ್ರುವ;
    // Create a CfgEdgeMap map for this graph
    template <typename T_ಮೌಲ್ಯ>
    ಅನ್ತಃಸ್ಥ CfgEdgeMap<T_ಮೌಲ್ಯ> makeEdgeMap() ಧ್ರುವ;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() ಧ್ರುವ;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    ಶೂನ್ಯ minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    ಶೂನ್ಯ breakCriticalEdges();

    bool ಸೇರಿಸು_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

template <typename T_ಕೀಲಿ, typename T_ಮೌಲ್ಯ>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ಧ್ರುವ CfgGraph* m_cfgp;  // The control flow graph this map is for
    size_t m_created;  // Edit count of CFG this map was created at
    std::vector<T_ಮೌಲ್ಯ> m_map;  // The map, stored as a vector

ರಕ್ಷಿತ:
    // CONSTRUCTOR
    explicit CfgMap(ಧ್ರುವ CfgGraph* cfgp, size_t size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

ಸಾರ್ವಜನಿಕ:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ಧ್ರುವ CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>&) = ಸ್ವಭಾವ;
    CfgMap(CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>&&) = ಸ್ವಭಾವ;
    CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>& operator=(ಧ್ರುವ CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>&) = ಸ್ವಭಾವ;
    CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>& operator=(CfgMap<T_ಕೀಲಿ, T_ಮೌಲ್ಯ>&&) = ಸ್ವಭಾವ;

    T_ಮೌಲ್ಯ& operator[](ಧ್ರುವ T_ಕೀಲಿ& ಕೀಲಿ) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ಕೀಲಿ.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ಪ್ರತ್ಯಾಗಮ m_map.at(ಕೀಲಿ.id());
    }
    ಧ್ರುವ T_ಮೌಲ್ಯ& operator[](ಧ್ರುವ T_ಕೀಲಿ& ಕೀಲಿ) ಧ್ರುವ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ಕೀಲಿ.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ಪ್ರತ್ಯಾಗಮ m_map.at(ಕೀಲಿ.id());
    }
    T_ಮೌಲ್ಯ& operator[](ಧ್ರುವ T_ಕೀಲಿ* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ಪ್ರತ್ಯಾಗಮ m_map.at(keyp->id());
    }
    ಧ್ರುವ T_ಮೌಲ್ಯ& operator[](ಧ್ರುವ T_ಕೀಲಿ* keyp) ಧ್ರುವ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ಪ್ರತ್ಯಾಗಮ m_map.at(keyp->id());
    }
};

template <typename T_ಮೌಲ್ಯ>
class CfgBlockMap final : ಸಾರ್ವಜನಿಕ CfgMap<CfgBlock, T_ಮೌಲ್ಯ> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(ಧ್ರುವ CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_ಮೌಲ್ಯ>{cfgp, cfgp->nBlocks()} {}

ಸಾರ್ವಜನಿಕ:
    // Can create an empty map
    CfgBlockMap() = ಸ್ವಭಾವ;
    // Copyable, movable
    CfgBlockMap(ಧ್ರುವ CfgBlockMap&) = ಸ್ವಭಾವ;
    CfgBlockMap(CfgBlockMap&&) = ಸ್ವಭಾವ;
    CfgBlockMap& operator=(ಧ್ರುವ CfgBlockMap&) = ಸ್ವಭಾವ;
    CfgBlockMap& operator=(CfgBlockMap&&) = ಸ್ವಭಾವ;
};

template <typename T_ಮೌಲ್ಯ>
class CfgEdgeMap final : ಸಾರ್ವಜನಿಕ CfgMap<CfgEdge, T_ಮೌಲ್ಯ> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(ಧ್ರುವ CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_ಮೌಲ್ಯ>{cfgp, cfgp->nEdges()} {}

ಸಾರ್ವಜನಿಕ:
    // Can create an empty map
    CfgEdgeMap() = ಸ್ವಭಾವ;
    // Copyable, movable
    CfgEdgeMap(ಧ್ರುವ CfgEdgeMap&) = ಸ್ವಭಾವ;
    CfgEdgeMap(CfgEdgeMap&&) = ಸ್ವಭಾವ;
    CfgEdgeMap& operator=(ಧ್ರುವ CfgEdgeMap&) = ಸ್ವಭಾವ;
    CfgEdgeMap& operator=(CfgEdgeMap&&) = ಸ್ವಭಾವ;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

size_t CfgBlock::id() { ಪ್ರತ್ಯಾಗಮ m_cfgp->idOf(this); }
size_t CfgBlock::id() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    ಪ್ರತ್ಯಾಗಮ isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
ಧ್ರುವ CfgEdge* CfgBlock::takenEdgep() ಧ್ರುವ {  // It's always the first edge
    ಪ್ರತ್ಯಾಗಮ isExit() ? nullptr : static_cast<ಧ್ರುವ CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    ಪ್ರತ್ಯಾಗಮ isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
ಧ್ರುವ CfgEdge* CfgBlock::untknEdgep() ಧ್ರುವ {  // It's always the second (last) edge
    ಪ್ರತ್ಯಾಗಮ isBranch() ? static_cast<ಧ್ರುವ CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    ಪ್ರತ್ಯಾಗಮ isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->ತುದಿ());
}
ಧ್ರುವ CfgBlock* CfgBlock::takenp() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->ತುದಿ());
}
CfgBlock* CfgBlock::untknp() {
    ಪ್ರತ್ಯಾಗಮ isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->ತುದಿ()) : nullptr;
}
ಧ್ರುವ CfgBlock* CfgBlock::untknp() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ isBranch() ? static_cast<ಧ್ರುವ CfgBlock*>(outEdges().backp()->ತುದಿ()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    ಪ್ರತ್ಯಾಗಮ static_cast<CfgEdge*>(inEdges().frontp());
}
ಧ್ರುವ CfgEdge* CfgBlock::firstPredecessorEdgep() ಧ್ರುವ {  //
    ಪ್ರತ್ಯಾಗಮ static_cast<ಧ್ರುವ CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    ಪ್ರತ್ಯಾಗಮ static_cast<CfgEdge*>(inEdges().backp());
}
ಧ್ರುವ CfgEdge* CfgBlock::lastPredecessorEdgep() ಧ್ರುವ {  //
    ಪ್ರತ್ಯಾಗಮ static_cast<ಧ್ರುವ CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    ಪ್ರತ್ಯಾಗಮ isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
ಧ್ರುವ CfgBlock* CfgBlock::firstPredecessorp() ಧ್ರುವ {  //
    ಪ್ರತ್ಯಾಗಮ isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    ಪ್ರತ್ಯಾಗಮ isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
ಧ್ರುವ CfgBlock* CfgBlock::lastPredecessorp() ಧ್ರುವ {  //
    ಪ್ರತ್ಯಾಗಮ isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

size_t CfgEdge::id() { ಪ್ರತ್ಯಾಗಮ m_cfgp->idOf(this); }
size_t CfgEdge::id() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cfgp->idOf(this); }

// --- CfgGraph ---

template <typename T_ಮೌಲ್ಯ>
CfgBlockMap<T_ಮೌಲ್ಯ> CfgGraph::makeBlockMap() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ CfgBlockMap<T_ಮೌಲ್ಯ>{this};
}
template <typename T_ಮೌಲ್ಯ>
CfgEdgeMap<T_ಮೌಲ್ಯ> CfgGraph::makeEdgeMap() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ CfgEdgeMap<T_ಮೌಲ್ಯ>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_ವೃಕ್ಷ final {
    // STATE
    CfgBlockMap<ಧ್ರುವ CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ಧ್ರುವ CfgBlock* intersect(ಧ್ರುವ CfgBlock* ap, ಧ್ರುವ CfgBlock* bp);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    explicit Cfg_Dominator_ವೃಕ್ಷ(ಧ್ರುವ CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_ವೃಕ್ಷ() = ಸ್ವಭಾವ;
    // Copyable, movable
    Cfg_Dominator_ವೃಕ್ಷ(ಧ್ರುವ Cfg_Dominator_ವೃಕ್ಷ&) = ಸ್ವಭಾವ;
    Cfg_Dominator_ವೃಕ್ಷ(Cfg_Dominator_ವೃಕ್ಷ&&) = ಸ್ವಭಾವ;
    Cfg_Dominator_ವೃಕ್ಷ& operator=(ಧ್ರುವ Cfg_Dominator_ವೃಕ್ಷ&) = ಸ್ವಭಾವ;
    Cfg_Dominator_ವೃಕ್ಷ& operator=(Cfg_Dominator_ವೃಕ್ಷ&&) = ಸ್ವಭಾವ;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ಧ್ರುವ CfgBlock* closestCommonDominator(ಧ್ರುವ CfgBlock* ap, ಧ್ರುವ CfgBlock* bp) ಧ್ರುವ {
        ಯಾವತ್ (ap != bp) {
            ಯದಿ (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } ಅನ್ಯಥಾ {
                ap = m_bb2Idom[ap];
            }
        }
        ಪ್ರತ್ಯಾಗಮ ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ಧ್ರುವ CfgBlock* ಧ್ರುವ ap, ಧ್ರುವ CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        ಯಾವತ್ (true) {
            // True if 'ap' is above (or same as) 'bp'
            ಯದಿ (ap == bp) ಪ್ರತ್ಯಾಗಮ true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            ಯದಿ (!bp) ಪ್ರತ್ಯಾಗಮ false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(ಧ್ರುವ CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
