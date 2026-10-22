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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Graph.h"

#અન્તર્ભાવ <functional>
#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class CfgGraph;
class CfgBlock;
class CfgEdge;

template <typename T_ચાવી, typename T_કિંમત>
class CfgMap;
template <typename T_કિંમત>
class CfgBlockMap;
template <typename T_કિંમત>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : જાહેર V3GraphVertex {
    friend class CfgGraph;
    template <typename T_ચાવી, typename T_કિંમત>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    CfgGraph* અચલ m_cfgp;  // The control flow graph this CfgBlock is under
    size_t m_rpo_સંખ્યા = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_ગાંઠ_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    અંતઃરેખ explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = મૂળભૂત;

જાહેર:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    અંતઃરેખ size_t id();
    અંતઃરેખ size_t id() અચલ;

    // Is this the entry block of the CFG?
    bool isEnter() અચલ { પાછા inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() અચલ { પાછા outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() અચલ { પાછા outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() અચલ { પાછા inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() અચલ { પાછા inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    અંતઃરેખ CfgEdge* takenEdgep();
    અંતઃરેખ અચલ CfgEdge* takenEdgep() અચલ;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    અંતઃરેખ CfgEdge* untknEdgep();
    અંતઃરેખ અચલ CfgEdge* untknEdgep() અચલ;
    // The taken successor block, or nullptr if exit block
    અંતઃરેખ CfgBlock* takenp();
    અંતઃરેખ અચલ CfgBlock* takenp() અચલ;
    // The untakens successor block, or nullptr if not a branch, or exit block
    અંતઃરેખ CfgBlock* untknp();
    અંતઃરેખ અચલ CfgBlock* untknp() અચલ;

    // The first predecessor edge, or nullptr if enter block
    અંતઃરેખ CfgEdge* firstPredecessorEdgep();
    અંતઃરેખ અચલ CfgEdge* firstPredecessorEdgep() અચલ;
    // The last predecessor edge, or nullptr if enter block
    અંતઃરેખ CfgEdge* lastPredecessorEdgep();
    અંતઃરેખ અચલ CfgEdge* lastPredecessorEdgep() અચલ;
    // The first predecessor block, or nullptr if enter block
    અંતઃરેખ CfgBlock* firstPredecessorp();
    અંતઃરેખ અચલ CfgBlock* firstPredecessorp() અચલ;
    // The last predecessor block, or nullptr if enter block
    અંતઃરેખ CfgBlock* lastPredecessorp();
    અંતઃરેખ અચલ CfgBlock* lastPredecessorp() અચલ;

    // Statements in this CfgBlock
    અચલ std::vector<Ast_ગાંઠ_Stmt*>& stmtps() અચલ { પાછા m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool operator<(અચલ CfgBlock& that) અચલ { પાછા id() < that.id(); }
    bool operator>(અચલ CfgBlock& that) અચલ { પાછા id() > that.id(); }
    bool operator==(અચલ CfgBlock& that) અચલ { પાછા this == &that; }

    // Iterators
    શૂન્ય forEachSuccessor(std::function<શૂન્ય(અચલ CfgBlock&)> f) અચલ {
        માટે (અચલ V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.ટોચ()));
    }
    શૂન્ય forEachPredecessor(std::function<શૂન્ય(અચલ CfgBlock&)> f) અચલ {
        માટે (અચલ V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_લીટી* fileline() અચલ override {
        પાછા !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() અચલ override;
    std::string dotShape() અચલ override;
    std::string dotRank() અચલ override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : જાહેર V3GraphEdge {
    friend class CfgGraph;
    template <typename T_ચાવી, typename T_કિંમત>
    friend class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* અચલ m_cfgp;  // The control flow graph this CfgEdge is under
    size_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    અંતઃરેખ CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = મૂળભૂત;

જાહેર:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    અંતઃરેખ size_t id();
    અંતઃરેખ size_t id() અચલ;

    // Source/destination CfgBlock
    અચલ CfgBlock* srcp() અચલ { પાછા static_cast<અચલ CfgBlock*>(fromp()); }
    અચલ CfgBlock* dstp() અચલ { પાછા static_cast<અચલ CfgBlock*>(ટોચ()); }
    CfgBlock* srcp() { પાછા static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { પાછા static_cast<CfgBlock*>(ટોચ()); }

    // For Graphviz dumps only
    std::string dotLabel() અચલ override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : જાહેર V3Graph {
    friend class CfgBlock;
    friend class CfgEdge;
    template <typename T_ચાવી, typename T_કિંમત>
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
    શૂન્ય rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        પાછા new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    શૂન્ય addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
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
    શૂન્ય addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
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

    size_t idOf(અચલ CfgBlock* bbp) અચલ {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        પાછા bbp->m_rpo_સંખ્યા;
    }

    size_t idOf(અચલ CfgEdge* edgep) અચલ {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        પાછા edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = મૂળભૂત;

જાહેર:
    ~CfgGraph() override = મૂળભૂત;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    સ્થિર std::unique_ptr<CfgGraph> build(Ast_ગાંઠ* stmtsp);

    // PUBLIC METHODS

    // Accessors
    અચલ CfgBlock& enter() અચલ { પાછા *m_enterp; }
    અચલ CfgBlock& exit() અચલ { પાછા *m_exitp; }

    // Number of basic blocks in this graph
    size_t nBlocks() અચલ { પાછા m_nBlocks; }
    // Number of control flow edges in this graph
    size_t nEdges() અચલ { પાછા m_nEdges; }

    // Create a CfgBlock map for this graph
    template <typename T_કિંમત>
    અંતઃરેખ CfgBlockMap<T_કિંમત> makeBlockMap() અચલ;
    // Create a CfgEdgeMap map for this graph
    template <typename T_કિંમત>
    અંતઃરેખ CfgEdgeMap<T_કિંમત> makeEdgeMap() અચલ;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() અચલ;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    શૂન્ય minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    શૂન્ય breakCriticalEdges();

    bool ઉમેરો_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

template <typename T_ચાવી, typename T_કિંમત>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    અચલ CfgGraph* m_cfgp;  // The control flow graph this map is for
    size_t m_created;  // Edit count of CFG this map was created at
    std::vector<T_કિંમત> m_map;  // The map, stored as a vector

સુરક્ષિત:
    // CONSTRUCTOR
    explicit CfgMap(અચલ CfgGraph* cfgp, size_t size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

જાહેર:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(અચલ CfgMap<T_ચાવી, T_કિંમત>&) = મૂળભૂત;
    CfgMap(CfgMap<T_ચાવી, T_કિંમત>&&) = મૂળભૂત;
    CfgMap<T_ચાવી, T_કિંમત>& operator=(અચલ CfgMap<T_ચાવી, T_કિંમત>&) = મૂળભૂત;
    CfgMap<T_ચાવી, T_કિંમત>& operator=(CfgMap<T_ચાવી, T_કિંમત>&&) = મૂળભૂત;

    T_કિંમત& operator[](અચલ T_ચાવી& ચાવી) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ચાવી.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        પાછા m_map.at(ચાવી.id());
    }
    અચલ T_કિંમત& operator[](અચલ T_ચાવી& ચાવી) અચલ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ચાવી.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        પાછા m_map.at(ચાવી.id());
    }
    T_કિંમત& operator[](અચલ T_ચાવી* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        પાછા m_map.at(keyp->id());
    }
    અચલ T_કિંમત& operator[](અચલ T_ચાવી* keyp) અચલ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        પાછા m_map.at(keyp->id());
    }
};

template <typename T_કિંમત>
class CfgBlockMap final : જાહેર CfgMap<CfgBlock, T_કિંમત> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(અચલ CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_કિંમત>{cfgp, cfgp->nBlocks()} {}

જાહેર:
    // Can create an empty map
    CfgBlockMap() = મૂળભૂત;
    // Copyable, movable
    CfgBlockMap(અચલ CfgBlockMap&) = મૂળભૂત;
    CfgBlockMap(CfgBlockMap&&) = મૂળભૂત;
    CfgBlockMap& operator=(અચલ CfgBlockMap&) = મૂળભૂત;
    CfgBlockMap& operator=(CfgBlockMap&&) = મૂળભૂત;
};

template <typename T_કિંમત>
class CfgEdgeMap final : જાહેર CfgMap<CfgEdge, T_કિંમત> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(અચલ CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_કિંમત>{cfgp, cfgp->nEdges()} {}

જાહેર:
    // Can create an empty map
    CfgEdgeMap() = મૂળભૂત;
    // Copyable, movable
    CfgEdgeMap(અચલ CfgEdgeMap&) = મૂળભૂત;
    CfgEdgeMap(CfgEdgeMap&&) = મૂળભૂત;
    CfgEdgeMap& operator=(અચલ CfgEdgeMap&) = મૂળભૂત;
    CfgEdgeMap& operator=(CfgEdgeMap&&) = મૂળભૂત;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

size_t CfgBlock::id() { પાછા m_cfgp->idOf(this); }
size_t CfgBlock::id() અચલ { પાછા m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    પાછા isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
અચલ CfgEdge* CfgBlock::takenEdgep() અચલ {  // It's always the first edge
    પાછા isExit() ? nullptr : static_cast<અચલ CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    પાછા isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
અચલ CfgEdge* CfgBlock::untknEdgep() અચલ {  // It's always the second (last) edge
    પાછા isBranch() ? static_cast<અચલ CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    પાછા isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->ટોચ());
}
અચલ CfgBlock* CfgBlock::takenp() અચલ {
    પાછા isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->ટોચ());
}
CfgBlock* CfgBlock::untknp() {
    પાછા isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->ટોચ()) : nullptr;
}
અચલ CfgBlock* CfgBlock::untknp() અચલ {
    પાછા isBranch() ? static_cast<અચલ CfgBlock*>(outEdges().backp()->ટોચ()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    પાછા static_cast<CfgEdge*>(inEdges().frontp());
}
અચલ CfgEdge* CfgBlock::firstPredecessorEdgep() અચલ {  //
    પાછા static_cast<અચલ CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    પાછા static_cast<CfgEdge*>(inEdges().backp());
}
અચલ CfgEdge* CfgBlock::lastPredecessorEdgep() અચલ {  //
    પાછા static_cast<અચલ CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    પાછા isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
અચલ CfgBlock* CfgBlock::firstPredecessorp() અચલ {  //
    પાછા isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    પાછા isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
અચલ CfgBlock* CfgBlock::lastPredecessorp() અચલ {  //
    પાછા isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

size_t CfgEdge::id() { પાછા m_cfgp->idOf(this); }
size_t CfgEdge::id() અચલ { પાછા m_cfgp->idOf(this); }

// --- CfgGraph ---

template <typename T_કિંમત>
CfgBlockMap<T_કિંમત> CfgGraph::makeBlockMap() અચલ {
    પાછા CfgBlockMap<T_કિંમત>{this};
}
template <typename T_કિંમત>
CfgEdgeMap<T_કિંમત> CfgGraph::makeEdgeMap() અચલ {
    પાછા CfgEdgeMap<T_કિંમત>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_વૃક્ષ final {
    // STATE
    CfgBlockMap<અચલ CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    અચલ CfgBlock* intersect(અચલ CfgBlock* ap, અચલ CfgBlock* bp);

જાહેર:
    // CONSTRUCTOR
    explicit Cfg_Dominator_વૃક્ષ(અચલ CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_વૃક્ષ() = મૂળભૂત;
    // Copyable, movable
    Cfg_Dominator_વૃક્ષ(અચલ Cfg_Dominator_વૃક્ષ&) = મૂળભૂત;
    Cfg_Dominator_વૃક્ષ(Cfg_Dominator_વૃક્ષ&&) = મૂળભૂત;
    Cfg_Dominator_વૃક્ષ& operator=(અચલ Cfg_Dominator_વૃક્ષ&) = મૂળભૂત;
    Cfg_Dominator_વૃક્ષ& operator=(Cfg_Dominator_વૃક્ષ&&) = મૂળભૂત;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    અચલ CfgBlock* closestCommonDominator(અચલ CfgBlock* ap, અચલ CfgBlock* bp) અચલ {
        જ્યારે (ap != bp) {
            જો (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } નહીંતર {
                ap = m_bb2Idom[ap];
            }
        }
        પાછા ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(અચલ CfgBlock* અચલ ap, અચલ CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        જ્યારે (true) {
            // True if 'ap' is above (or same as) 'bp'
            જો (ap == bp) પાછા true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            જો (!bp) પાછા false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(અચલ CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
