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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Graph.h"

#অন্তর্ভাব <functional>
#অন্তর্ভাব <limits>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class CfgGraph;
class CfgBlock;
class CfgEdge;

template <typename T_চাবি, typename T_মান>
class CfgMap;
template <typename T_মান>
class CfgBlockMap;
template <typename T_মান>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : সার্বজনিক V3GraphVertex {
    friend class CfgGraph;
    template <typename T_চাবি, typename T_মান>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    CfgGraph* ধ্রুব m_cfgp;  // The control flow graph this CfgBlock is under
    size_t m_rpo_সংখ্যা = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_গ্রন্থি_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    অন্তঃস্থ explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = স্বভাব;

সার্বজনিক:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    অন্তঃস্থ size_t id();
    অন্তঃস্থ size_t id() ধ্রুব;

    // Is this the entry block of the CFG?
    bool isEnter() ধ্রুব { প্রত্যাগম inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() ধ্রুব { প্রত্যাগম outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ধ্রুব { প্রত্যাগম outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ধ্রুব { প্রত্যাগম inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ধ্রুব { প্রত্যাগম inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    অন্তঃস্থ CfgEdge* takenEdgep();
    অন্তঃস্থ ধ্রুব CfgEdge* takenEdgep() ধ্রুব;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    অন্তঃস্থ CfgEdge* untknEdgep();
    অন্তঃস্থ ধ্রুব CfgEdge* untknEdgep() ধ্রুব;
    // The taken successor block, or nullptr if exit block
    অন্তঃস্থ CfgBlock* takenp();
    অন্তঃস্থ ধ্রুব CfgBlock* takenp() ধ্রুব;
    // The untakens successor block, or nullptr if not a branch, or exit block
    অন্তঃস্থ CfgBlock* untknp();
    অন্তঃস্থ ধ্রুব CfgBlock* untknp() ধ্রুব;

    // The first predecessor edge, or nullptr if enter block
    অন্তঃস্থ CfgEdge* firstPredecessorEdgep();
    অন্তঃস্থ ধ্রুব CfgEdge* firstPredecessorEdgep() ধ্রুব;
    // The last predecessor edge, or nullptr if enter block
    অন্তঃস্থ CfgEdge* lastPredecessorEdgep();
    অন্তঃস্থ ধ্রুব CfgEdge* lastPredecessorEdgep() ধ্রুব;
    // The first predecessor block, or nullptr if enter block
    অন্তঃস্থ CfgBlock* firstPredecessorp();
    অন্তঃস্থ ধ্রুব CfgBlock* firstPredecessorp() ধ্রুব;
    // The last predecessor block, or nullptr if enter block
    অন্তঃস্থ CfgBlock* lastPredecessorp();
    অন্তঃস্থ ধ্রুব CfgBlock* lastPredecessorp() ধ্রুব;

    // Statements in this CfgBlock
    ধ্রুব std::vector<Ast_গ্রন্থি_Stmt*>& stmtps() ধ্রুব { প্রত্যাগম m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool operator<(ধ্রুব CfgBlock& that) ধ্রুব { প্রত্যাগম id() < that.id(); }
    bool operator>(ধ্রুব CfgBlock& that) ধ্রুব { প্রত্যাগম id() > that.id(); }
    bool operator==(ধ্রুব CfgBlock& that) ধ্রুব { প্রত্যাগম this == &that; }

    // Iterators
    শূন্য forEachSuccessor(std::function<শূন্য(ধ্রুব CfgBlock&)> f) ধ্রুব {
        প্রতি (ধ্রুব V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.শীর্ষ()));
    }
    শূন্য forEachPredecessor(std::function<শূন্য(ধ্রুব CfgBlock&)> f) ধ্রুব {
        প্রতি (ধ্রুব V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_সারি* fileline() ধ্রুব override {
        প্রত্যাগম !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ধ্রুব override;
    std::string dotShape() ধ্রুব override;
    std::string dotRank() ধ্রুব override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : সার্বজনিক V3GraphEdge {
    friend class CfgGraph;
    template <typename T_চাবি, typename T_মান>
    friend class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* ধ্রুব m_cfgp;  // The control flow graph this CfgEdge is under
    size_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    অন্তঃস্থ CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = স্বভাব;

সার্বজনিক:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    অন্তঃস্থ size_t id();
    অন্তঃস্থ size_t id() ধ্রুব;

    // Source/destination CfgBlock
    ধ্রুব CfgBlock* srcp() ধ্রুব { প্রত্যাগম static_cast<ধ্রুব CfgBlock*>(fromp()); }
    ধ্রুব CfgBlock* dstp() ধ্রুব { প্রত্যাগম static_cast<ধ্রুব CfgBlock*>(শীর্ষ()); }
    CfgBlock* srcp() { প্রত্যাগম static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { প্রত্যাগম static_cast<CfgBlock*>(শীর্ষ()); }

    // For Graphviz dumps only
    std::string dotLabel() ধ্রুব override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : সার্বজনিক V3Graph {
    friend class CfgBlock;
    friend class CfgEdge;
    template <typename T_চাবি, typename T_মান>
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
    শূন্য rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        প্রত্যাগম new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    শূন্য addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
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
    শূন্য addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
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

    size_t idOf(ধ্রুব CfgBlock* bbp) ধ্রুব {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        প্রত্যাগম bbp->m_rpo_সংখ্যা;
    }

    size_t idOf(ধ্রুব CfgEdge* edgep) ধ্রুব {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        প্রত্যাগম edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = স্বভাব;

সার্বজনিক:
    ~CfgGraph() override = স্বভাব;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    স্থির std::unique_ptr<CfgGraph> build(Ast_গ্রন্থি* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ধ্রুব CfgBlock& enter() ধ্রুব { প্রত্যাগম *m_enterp; }
    ধ্রুব CfgBlock& exit() ধ্রুব { প্রত্যাগম *m_exitp; }

    // Number of basic blocks in this graph
    size_t nBlocks() ধ্রুব { প্রত্যাগম m_nBlocks; }
    // Number of control flow edges in this graph
    size_t nEdges() ধ্রুব { প্রত্যাগম m_nEdges; }

    // Create a CfgBlock map for this graph
    template <typename T_মান>
    অন্তঃস্থ CfgBlockMap<T_মান> makeBlockMap() ধ্রুব;
    // Create a CfgEdgeMap map for this graph
    template <typename T_মান>
    অন্তঃস্থ CfgEdgeMap<T_মান> makeEdgeMap() ধ্রুব;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() ধ্রুব;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    শূন্য minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    শূন্য breakCriticalEdges();

    bool ঢোকাও_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

template <typename T_চাবি, typename T_মান>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ধ্রুব CfgGraph* m_cfgp;  // The control flow graph this map is for
    size_t m_created;  // Edit count of CFG this map was created at
    std::vector<T_মান> m_map;  // The map, stored as a vector

রক্ষিত:
    // CONSTRUCTOR
    explicit CfgMap(ধ্রুব CfgGraph* cfgp, size_t size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

সার্বজনিক:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ধ্রুব CfgMap<T_চাবি, T_মান>&) = স্বভাব;
    CfgMap(CfgMap<T_চাবি, T_মান>&&) = স্বভাব;
    CfgMap<T_চাবি, T_মান>& operator=(ধ্রুব CfgMap<T_চাবি, T_মান>&) = স্বভাব;
    CfgMap<T_চাবি, T_মান>& operator=(CfgMap<T_চাবি, T_মান>&&) = স্বভাব;

    T_মান& operator[](ধ্রুব T_চাবি& চাবি) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == চাবি.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        প্রত্যাগম m_map.at(চাবি.id());
    }
    ধ্রুব T_মান& operator[](ধ্রুব T_চাবি& চাবি) ধ্রুব {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == চাবি.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        প্রত্যাগম m_map.at(চাবি.id());
    }
    T_মান& operator[](ধ্রুব T_চাবি* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        প্রত্যাগম m_map.at(keyp->id());
    }
    ধ্রুব T_মান& operator[](ধ্রুব T_চাবি* keyp) ধ্রুব {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        প্রত্যাগম m_map.at(keyp->id());
    }
};

template <typename T_মান>
class CfgBlockMap final : সার্বজনিক CfgMap<CfgBlock, T_মান> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(ধ্রুব CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_মান>{cfgp, cfgp->nBlocks()} {}

সার্বজনিক:
    // Can create an empty map
    CfgBlockMap() = স্বভাব;
    // Copyable, movable
    CfgBlockMap(ধ্রুব CfgBlockMap&) = স্বভাব;
    CfgBlockMap(CfgBlockMap&&) = স্বভাব;
    CfgBlockMap& operator=(ধ্রুব CfgBlockMap&) = স্বভাব;
    CfgBlockMap& operator=(CfgBlockMap&&) = স্বভাব;
};

template <typename T_মান>
class CfgEdgeMap final : সার্বজনিক CfgMap<CfgEdge, T_মান> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(ধ্রুব CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_মান>{cfgp, cfgp->nEdges()} {}

সার্বজনিক:
    // Can create an empty map
    CfgEdgeMap() = স্বভাব;
    // Copyable, movable
    CfgEdgeMap(ধ্রুব CfgEdgeMap&) = স্বভাব;
    CfgEdgeMap(CfgEdgeMap&&) = স্বভাব;
    CfgEdgeMap& operator=(ধ্রুব CfgEdgeMap&) = স্বভাব;
    CfgEdgeMap& operator=(CfgEdgeMap&&) = স্বভাব;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

size_t CfgBlock::id() { প্রত্যাগম m_cfgp->idOf(this); }
size_t CfgBlock::id() ধ্রুব { প্রত্যাগম m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    প্রত্যাগম isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
ধ্রুব CfgEdge* CfgBlock::takenEdgep() ধ্রুব {  // It's always the first edge
    প্রত্যাগম isExit() ? nullptr : static_cast<ধ্রুব CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    প্রত্যাগম isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
ধ্রুব CfgEdge* CfgBlock::untknEdgep() ধ্রুব {  // It's always the second (last) edge
    প্রত্যাগম isBranch() ? static_cast<ধ্রুব CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    প্রত্যাগম isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->শীর্ষ());
}
ধ্রুব CfgBlock* CfgBlock::takenp() ধ্রুব {
    প্রত্যাগম isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->শীর্ষ());
}
CfgBlock* CfgBlock::untknp() {
    প্রত্যাগম isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->শীর্ষ()) : nullptr;
}
ধ্রুব CfgBlock* CfgBlock::untknp() ধ্রুব {
    প্রত্যাগম isBranch() ? static_cast<ধ্রুব CfgBlock*>(outEdges().backp()->শীর্ষ()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    প্রত্যাগম static_cast<CfgEdge*>(inEdges().frontp());
}
ধ্রুব CfgEdge* CfgBlock::firstPredecessorEdgep() ধ্রুব {  //
    প্রত্যাগম static_cast<ধ্রুব CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    প্রত্যাগম static_cast<CfgEdge*>(inEdges().backp());
}
ধ্রুব CfgEdge* CfgBlock::lastPredecessorEdgep() ধ্রুব {  //
    প্রত্যাগম static_cast<ধ্রুব CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    প্রত্যাগম isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
ধ্রুব CfgBlock* CfgBlock::firstPredecessorp() ধ্রুব {  //
    প্রত্যাগম isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    প্রত্যাগম isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
ধ্রুব CfgBlock* CfgBlock::lastPredecessorp() ধ্রুব {  //
    প্রত্যাগম isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

size_t CfgEdge::id() { প্রত্যাগম m_cfgp->idOf(this); }
size_t CfgEdge::id() ধ্রুব { প্রত্যাগম m_cfgp->idOf(this); }

// --- CfgGraph ---

template <typename T_মান>
CfgBlockMap<T_মান> CfgGraph::makeBlockMap() ধ্রুব {
    প্রত্যাগম CfgBlockMap<T_মান>{this};
}
template <typename T_মান>
CfgEdgeMap<T_মান> CfgGraph::makeEdgeMap() ধ্রুব {
    প্রত্যাগম CfgEdgeMap<T_মান>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_বৃক্ষ final {
    // STATE
    CfgBlockMap<ধ্রুব CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ধ্রুব CfgBlock* intersect(ধ্রুব CfgBlock* ap, ধ্রুব CfgBlock* bp);

সার্বজনিক:
    // CONSTRUCTOR
    explicit Cfg_Dominator_বৃক্ষ(ধ্রুব CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_বৃক্ষ() = স্বভাব;
    // Copyable, movable
    Cfg_Dominator_বৃক্ষ(ধ্রুব Cfg_Dominator_বৃক্ষ&) = স্বভাব;
    Cfg_Dominator_বৃক্ষ(Cfg_Dominator_বৃক্ষ&&) = স্বভাব;
    Cfg_Dominator_বৃক্ষ& operator=(ধ্রুব Cfg_Dominator_বৃক্ষ&) = স্বভাব;
    Cfg_Dominator_বৃক্ষ& operator=(Cfg_Dominator_বৃক্ষ&&) = স্বভাব;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ধ্রুব CfgBlock* closestCommonDominator(ধ্রুব CfgBlock* ap, ধ্রুব CfgBlock* bp) ধ্রুব {
        যাবত্ (ap != bp) {
            যদি (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } অন্যথা {
                ap = m_bb2Idom[ap];
            }
        }
        প্রত্যাগম ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ধ্রুব CfgBlock* ধ্রুব ap, ধ্রুব CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        যাবত্ (true) {
            // True if 'ap' is above (or same as) 'bp'
            যদি (ap == bp) প্রত্যাগম true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            যদি (!bp) প্রত্যাগম false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(ধ্রুব CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
