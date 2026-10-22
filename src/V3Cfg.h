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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Graph.h"

#அந்தர்பாவ <functional>
#அந்தர்பாவ <limits>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class CfgGraph;
class CfgBlock;
class CfgEdge;

template <typename T_சாவி, typename T_மதிப்பு>
class CfgMap;
template <typename T_மதிப்பு>
class CfgBlockMap;
template <typename T_மதிப்பு>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : பொது V3GraphVertex {
    friend class CfgGraph;
    template <typename T_சாவி, typename T_மதிப்பு>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    CfgGraph* மாறிலி m_cfgp;  // The control flow graph this CfgBlock is under
    size_t m_rpo_எண் = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_முனை_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    உள்வரி explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = இயல்பு;

பொது:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    உள்வரி size_t id();
    உள்வரி size_t id() மாறிலி;

    // Is this the entry block of the CFG?
    bool isEnter() மாறிலி { திரும்பு inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() மாறிலி { திரும்பு outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() மாறிலி { திரும்பு outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() மாறிலி { திரும்பு inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() மாறிலி { திரும்பு inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    உள்வரி CfgEdge* takenEdgep();
    உள்வரி மாறிலி CfgEdge* takenEdgep() மாறிலி;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    உள்வரி CfgEdge* untknEdgep();
    உள்வரி மாறிலி CfgEdge* untknEdgep() மாறிலி;
    // The taken successor block, or nullptr if exit block
    உள்வரி CfgBlock* takenp();
    உள்வரி மாறிலி CfgBlock* takenp() மாறிலி;
    // The untakens successor block, or nullptr if not a branch, or exit block
    உள்வரி CfgBlock* untknp();
    உள்வரி மாறிலி CfgBlock* untknp() மாறிலி;

    // The first predecessor edge, or nullptr if enter block
    உள்வரி CfgEdge* firstPredecessorEdgep();
    உள்வரி மாறிலி CfgEdge* firstPredecessorEdgep() மாறிலி;
    // The last predecessor edge, or nullptr if enter block
    உள்வரி CfgEdge* lastPredecessorEdgep();
    உள்வரி மாறிலி CfgEdge* lastPredecessorEdgep() மாறிலி;
    // The first predecessor block, or nullptr if enter block
    உள்வரி CfgBlock* firstPredecessorp();
    உள்வரி மாறிலி CfgBlock* firstPredecessorp() மாறிலி;
    // The last predecessor block, or nullptr if enter block
    உள்வரி CfgBlock* lastPredecessorp();
    உள்வரி மாறிலி CfgBlock* lastPredecessorp() மாறிலி;

    // Statements in this CfgBlock
    மாறிலி std::vector<Ast_முனை_Stmt*>& stmtps() மாறிலி { திரும்பு m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool operator<(மாறிலி CfgBlock& that) மாறிலி { திரும்பு id() < that.id(); }
    bool operator>(மாறிலி CfgBlock& that) மாறிலி { திரும்பு id() > that.id(); }
    bool operator==(மாறிலி CfgBlock& that) மாறிலி { திரும்பு this == &that; }

    // Iterators
    வெறுமை forEachSuccessor(std::function<வெறுமை(மாறிலி CfgBlock&)> f) மாறிலி {
        க்கு (மாறிலி V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.உச்சி()));
    }
    வெறுமை forEachPredecessor(std::function<வெறுமை(மாறிலி CfgBlock&)> f) மாறிலி {
        க்கு (மாறிலி V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_வரி* fileline() மாறிலி override {
        திரும்பு !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() மாறிலி override;
    std::string dotShape() மாறிலி override;
    std::string dotRank() மாறிலி override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : பொது V3GraphEdge {
    friend class CfgGraph;
    template <typename T_சாவி, typename T_மதிப்பு>
    friend class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* மாறிலி m_cfgp;  // The control flow graph this CfgEdge is under
    size_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    உள்வரி CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = இயல்பு;

பொது:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    உள்வரி size_t id();
    உள்வரி size_t id() மாறிலி;

    // Source/destination CfgBlock
    மாறிலி CfgBlock* srcp() மாறிலி { திரும்பு static_cast<மாறிலி CfgBlock*>(fromp()); }
    மாறிலி CfgBlock* dstp() மாறிலி { திரும்பு static_cast<மாறிலி CfgBlock*>(உச்சி()); }
    CfgBlock* srcp() { திரும்பு static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { திரும்பு static_cast<CfgBlock*>(உச்சி()); }

    // For Graphviz dumps only
    std::string dotLabel() மாறிலி override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : பொது V3Graph {
    friend class CfgBlock;
    friend class CfgEdge;
    template <typename T_சாவி, typename T_மதிப்பு>
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
    வெறுமை rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        திரும்பு new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    வெறுமை addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
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
    வெறுமை addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
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

    size_t idOf(மாறிலி CfgBlock* bbp) மாறிலி {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        திரும்பு bbp->m_rpo_எண்;
    }

    size_t idOf(மாறிலி CfgEdge* edgep) மாறிலி {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        திரும்பு edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = இயல்பு;

பொது:
    ~CfgGraph() override = இயல்பு;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    நிலையான std::unique_ptr<CfgGraph> build(Ast_முனை* stmtsp);

    // PUBLIC METHODS

    // Accessors
    மாறிலி CfgBlock& enter() மாறிலி { திரும்பு *m_enterp; }
    மாறிலி CfgBlock& exit() மாறிலி { திரும்பு *m_exitp; }

    // Number of basic blocks in this graph
    size_t nBlocks() மாறிலி { திரும்பு m_nBlocks; }
    // Number of control flow edges in this graph
    size_t nEdges() மாறிலி { திரும்பு m_nEdges; }

    // Create a CfgBlock map for this graph
    template <typename T_மதிப்பு>
    உள்வரி CfgBlockMap<T_மதிப்பு> makeBlockMap() மாறிலி;
    // Create a CfgEdgeMap map for this graph
    template <typename T_மதிப்பு>
    உள்வரி CfgEdgeMap<T_மதிப்பு> makeEdgeMap() மாறிலி;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() மாறிலி;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    வெறுமை minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    வெறுமை breakCriticalEdges();

    bool செருகு_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

template <typename T_சாவி, typename T_மதிப்பு>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    மாறிலி CfgGraph* m_cfgp;  // The control flow graph this map is for
    size_t m_created;  // Edit count of CFG this map was created at
    std::vector<T_மதிப்பு> m_map;  // The map, stored as a vector

பாதுகாக்கப்பட்ட:
    // CONSTRUCTOR
    explicit CfgMap(மாறிலி CfgGraph* cfgp, size_t size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

பொது:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(மாறிலி CfgMap<T_சாவி, T_மதிப்பு>&) = இயல்பு;
    CfgMap(CfgMap<T_சாவி, T_மதிப்பு>&&) = இயல்பு;
    CfgMap<T_சாவி, T_மதிப்பு>& operator=(மாறிலி CfgMap<T_சாவி, T_மதிப்பு>&) = இயல்பு;
    CfgMap<T_சாவி, T_மதிப்பு>& operator=(CfgMap<T_சாவி, T_மதிப்பு>&&) = இயல்பு;

    T_மதிப்பு& operator[](மாறிலி T_சாவி& சாவி) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == சாவி.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        திரும்பு m_map.at(சாவி.id());
    }
    மாறிலி T_மதிப்பு& operator[](மாறிலி T_சாவி& சாவி) மாறிலி {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == சாவி.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        திரும்பு m_map.at(சாவி.id());
    }
    T_மதிப்பு& operator[](மாறிலி T_சாவி* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        திரும்பு m_map.at(keyp->id());
    }
    மாறிலி T_மதிப்பு& operator[](மாறிலி T_சாவி* keyp) மாறிலி {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        திரும்பு m_map.at(keyp->id());
    }
};

template <typename T_மதிப்பு>
class CfgBlockMap final : பொது CfgMap<CfgBlock, T_மதிப்பு> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(மாறிலி CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_மதிப்பு>{cfgp, cfgp->nBlocks()} {}

பொது:
    // Can create an empty map
    CfgBlockMap() = இயல்பு;
    // Copyable, movable
    CfgBlockMap(மாறிலி CfgBlockMap&) = இயல்பு;
    CfgBlockMap(CfgBlockMap&&) = இயல்பு;
    CfgBlockMap& operator=(மாறிலி CfgBlockMap&) = இயல்பு;
    CfgBlockMap& operator=(CfgBlockMap&&) = இயல்பு;
};

template <typename T_மதிப்பு>
class CfgEdgeMap final : பொது CfgMap<CfgEdge, T_மதிப்பு> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(மாறிலி CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_மதிப்பு>{cfgp, cfgp->nEdges()} {}

பொது:
    // Can create an empty map
    CfgEdgeMap() = இயல்பு;
    // Copyable, movable
    CfgEdgeMap(மாறிலி CfgEdgeMap&) = இயல்பு;
    CfgEdgeMap(CfgEdgeMap&&) = இயல்பு;
    CfgEdgeMap& operator=(மாறிலி CfgEdgeMap&) = இயல்பு;
    CfgEdgeMap& operator=(CfgEdgeMap&&) = இயல்பு;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

size_t CfgBlock::id() { திரும்பு m_cfgp->idOf(this); }
size_t CfgBlock::id() மாறிலி { திரும்பு m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    திரும்பு isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
மாறிலி CfgEdge* CfgBlock::takenEdgep() மாறிலி {  // It's always the first edge
    திரும்பு isExit() ? nullptr : static_cast<மாறிலி CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    திரும்பு isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
மாறிலி CfgEdge* CfgBlock::untknEdgep() மாறிலி {  // It's always the second (last) edge
    திரும்பு isBranch() ? static_cast<மாறிலி CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    திரும்பு isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->உச்சி());
}
மாறிலி CfgBlock* CfgBlock::takenp() மாறிலி {
    திரும்பு isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->உச்சி());
}
CfgBlock* CfgBlock::untknp() {
    திரும்பு isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->உச்சி()) : nullptr;
}
மாறிலி CfgBlock* CfgBlock::untknp() மாறிலி {
    திரும்பு isBranch() ? static_cast<மாறிலி CfgBlock*>(outEdges().backp()->உச்சி()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    திரும்பு static_cast<CfgEdge*>(inEdges().frontp());
}
மாறிலி CfgEdge* CfgBlock::firstPredecessorEdgep() மாறிலி {  //
    திரும்பு static_cast<மாறிலி CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    திரும்பு static_cast<CfgEdge*>(inEdges().backp());
}
மாறிலி CfgEdge* CfgBlock::lastPredecessorEdgep() மாறிலி {  //
    திரும்பு static_cast<மாறிலி CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    திரும்பு isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
மாறிலி CfgBlock* CfgBlock::firstPredecessorp() மாறிலி {  //
    திரும்பு isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    திரும்பு isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
மாறிலி CfgBlock* CfgBlock::lastPredecessorp() மாறிலி {  //
    திரும்பு isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

size_t CfgEdge::id() { திரும்பு m_cfgp->idOf(this); }
size_t CfgEdge::id() மாறிலி { திரும்பு m_cfgp->idOf(this); }

// --- CfgGraph ---

template <typename T_மதிப்பு>
CfgBlockMap<T_மதிப்பு> CfgGraph::makeBlockMap() மாறிலி {
    திரும்பு CfgBlockMap<T_மதிப்பு>{this};
}
template <typename T_மதிப்பு>
CfgEdgeMap<T_மதிப்பு> CfgGraph::makeEdgeMap() மாறிலி {
    திரும்பு CfgEdgeMap<T_மதிப்பு>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_மரம் final {
    // STATE
    CfgBlockMap<மாறிலி CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    மாறிலி CfgBlock* intersect(மாறிலி CfgBlock* ap, மாறிலி CfgBlock* bp);

பொது:
    // CONSTRUCTOR
    explicit Cfg_Dominator_மரம்(மாறிலி CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_மரம்() = இயல்பு;
    // Copyable, movable
    Cfg_Dominator_மரம்(மாறிலி Cfg_Dominator_மரம்&) = இயல்பு;
    Cfg_Dominator_மரம்(Cfg_Dominator_மரம்&&) = இயல்பு;
    Cfg_Dominator_மரம்& operator=(மாறிலி Cfg_Dominator_மரம்&) = இயல்பு;
    Cfg_Dominator_மரம்& operator=(Cfg_Dominator_மரம்&&) = இயல்பு;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    மாறிலி CfgBlock* closestCommonDominator(மாறிலி CfgBlock* ap, மாறிலி CfgBlock* bp) மாறிலி {
        போது (ap != bp) {
            எனில் (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } இல்லையெனில் {
                ap = m_bb2Idom[ap];
            }
        }
        திரும்பு ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(மாறிலி CfgBlock* மாறிலி ap, மாறிலி CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        போது (true) {
            // True if 'ap' is above (or same as) 'bp'
            எனில் (ap == bp) திரும்பு true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            எனில் (!bp) திரும்பு false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(மாறிலி CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
