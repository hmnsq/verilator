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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Graph.h"

#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class CfgGraph;
class CfgBlock;
class CfgEdge;

template <typename T_താക്കോൽ, typename T_മൂല്യം>
class CfgMap;
template <typename T_മൂല്യം>
class CfgBlockMap;
template <typename T_മൂല്യം>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : പൊതു V3GraphVertex {
    friend class CfgGraph;
    template <typename T_താക്കോൽ, typename T_മൂല്യം>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    CfgGraph* സ്ഥിരം m_cfgp;  // The control flow graph this CfgBlock is under
    size_t m_rpo_സംഖ്യ = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_കണ്ണി_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    വരിയിൽ explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    വരിയിൽ size_t id();
    വരിയിൽ size_t id() സ്ഥിരം;

    // Is this the entry block of the CFG?
    bool isEnter() സ്ഥിരം { തിരികെ inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() സ്ഥിരം { തിരികെ outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() സ്ഥിരം { തിരികെ outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() സ്ഥിരം { തിരികെ inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() സ്ഥിരം { തിരികെ inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    വരിയിൽ CfgEdge* takenEdgep();
    വരിയിൽ സ്ഥിരം CfgEdge* takenEdgep() സ്ഥിരം;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    വരിയിൽ CfgEdge* untknEdgep();
    വരിയിൽ സ്ഥിരം CfgEdge* untknEdgep() സ്ഥിരം;
    // The taken successor block, or nullptr if exit block
    വരിയിൽ CfgBlock* takenp();
    വരിയിൽ സ്ഥിരം CfgBlock* takenp() സ്ഥിരം;
    // The untakens successor block, or nullptr if not a branch, or exit block
    വരിയിൽ CfgBlock* untknp();
    വരിയിൽ സ്ഥിരം CfgBlock* untknp() സ്ഥിരം;

    // The first predecessor edge, or nullptr if enter block
    വരിയിൽ CfgEdge* firstPredecessorEdgep();
    വരിയിൽ സ്ഥിരം CfgEdge* firstPredecessorEdgep() സ്ഥിരം;
    // The last predecessor edge, or nullptr if enter block
    വരിയിൽ CfgEdge* lastPredecessorEdgep();
    വരിയിൽ സ്ഥിരം CfgEdge* lastPredecessorEdgep() സ്ഥിരം;
    // The first predecessor block, or nullptr if enter block
    വരിയിൽ CfgBlock* firstPredecessorp();
    വരിയിൽ സ്ഥിരം CfgBlock* firstPredecessorp() സ്ഥിരം;
    // The last predecessor block, or nullptr if enter block
    വരിയിൽ CfgBlock* lastPredecessorp();
    വരിയിൽ സ്ഥിരം CfgBlock* lastPredecessorp() സ്ഥിരം;

    // Statements in this CfgBlock
    സ്ഥിരം std::vector<Ast_കണ്ണി_Stmt*>& stmtps() സ്ഥിരം { തിരികെ m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool operator<(സ്ഥിരം CfgBlock& that) സ്ഥിരം { തിരികെ id() < that.id(); }
    bool operator>(സ്ഥിരം CfgBlock& that) സ്ഥിരം { തിരികെ id() > that.id(); }
    bool operator==(സ്ഥിരം CfgBlock& that) സ്ഥിരം { തിരികെ this == &that; }

    // Iterators
    ശൂന്യം forEachSuccessor(std::function<ശൂന്യം(സ്ഥിരം CfgBlock&)> f) സ്ഥിരം {
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.മുകൾ()));
    }
    ശൂന്യം forEachPredecessor(std::function<ശൂന്യം(സ്ഥിരം CfgBlock&)> f) സ്ഥിരം {
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_വരി* fileline() സ്ഥിരം override {
        തിരികെ !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() സ്ഥിരം override;
    std::string dotShape() സ്ഥിരം override;
    std::string dotRank() സ്ഥിരം override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : പൊതു V3GraphEdge {
    friend class CfgGraph;
    template <typename T_താക്കോൽ, typename T_മൂല്യം>
    friend class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* സ്ഥിരം m_cfgp;  // The control flow graph this CfgEdge is under
    size_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    വരിയിൽ CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    വരിയിൽ size_t id();
    വരിയിൽ size_t id() സ്ഥിരം;

    // Source/destination CfgBlock
    സ്ഥിരം CfgBlock* srcp() സ്ഥിരം { തിരികെ static_cast<സ്ഥിരം CfgBlock*>(fromp()); }
    സ്ഥിരം CfgBlock* dstp() സ്ഥിരം { തിരികെ static_cast<സ്ഥിരം CfgBlock*>(മുകൾ()); }
    CfgBlock* srcp() { തിരികെ static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { തിരികെ static_cast<CfgBlock*>(മുകൾ()); }

    // For Graphviz dumps only
    std::string dotLabel() സ്ഥിരം override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : പൊതു V3Graph {
    friend class CfgBlock;
    friend class CfgEdge;
    template <typename T_താക്കോൽ, typename T_മൂല്യം>
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
    ശൂന്യം rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        തിരികെ new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    ശൂന്യം addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
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
    ശൂന്യം addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
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

    size_t idOf(സ്ഥിരം CfgBlock* bbp) സ്ഥിരം {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        തിരികെ bbp->m_rpo_സംഖ്യ;
    }

    size_t idOf(സ്ഥിരം CfgEdge* edgep) സ്ഥിരം {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        തിരികെ edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = സ്ഥിരസ്ഥിതി;

പൊതു:
    ~CfgGraph() override = സ്ഥിരസ്ഥിതി;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    സ്ഥിതം std::unique_ptr<CfgGraph> build(Ast_കണ്ണി* stmtsp);

    // PUBLIC METHODS

    // Accessors
    സ്ഥിരം CfgBlock& enter() സ്ഥിരം { തിരികെ *m_enterp; }
    സ്ഥിരം CfgBlock& exit() സ്ഥിരം { തിരികെ *m_exitp; }

    // Number of basic blocks in this graph
    size_t nBlocks() സ്ഥിരം { തിരികെ m_nBlocks; }
    // Number of control flow edges in this graph
    size_t nEdges() സ്ഥിരം { തിരികെ m_nEdges; }

    // Create a CfgBlock map for this graph
    template <typename T_മൂല്യം>
    വരിയിൽ CfgBlockMap<T_മൂല്യം> makeBlockMap() സ്ഥിരം;
    // Create a CfgEdgeMap map for this graph
    template <typename T_മൂല്യം>
    വരിയിൽ CfgEdgeMap<T_മൂല്യം> makeEdgeMap() സ്ഥിരം;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() സ്ഥിരം;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    ശൂന്യം minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    ശൂന്യം breakCriticalEdges();

    bool തിരുകുക_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

template <typename T_താക്കോൽ, typename T_മൂല്യം>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    സ്ഥിരം CfgGraph* m_cfgp;  // The control flow graph this map is for
    size_t m_created;  // Edit count of CFG this map was created at
    std::vector<T_മൂല്യം> m_map;  // The map, stored as a vector

സംരക്ഷിത:
    // CONSTRUCTOR
    explicit CfgMap(സ്ഥിരം CfgGraph* cfgp, size_t size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

പൊതു:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(സ്ഥിരം CfgMap<T_താക്കോൽ, T_മൂല്യം>&) = സ്ഥിരസ്ഥിതി;
    CfgMap(CfgMap<T_താക്കോൽ, T_മൂല്യം>&&) = സ്ഥിരസ്ഥിതി;
    CfgMap<T_താക്കോൽ, T_മൂല്യം>& operator=(സ്ഥിരം CfgMap<T_താക്കോൽ, T_മൂല്യം>&) = സ്ഥിരസ്ഥിതി;
    CfgMap<T_താക്കോൽ, T_മൂല്യം>& operator=(CfgMap<T_താക്കോൽ, T_മൂല്യം>&&) = സ്ഥിരസ്ഥിതി;

    T_മൂല്യം& operator[](സ്ഥിരം T_താക്കോൽ& താക്കോൽ) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == താക്കോൽ.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        തിരികെ m_map.at(താക്കോൽ.id());
    }
    സ്ഥിരം T_മൂല്യം& operator[](സ്ഥിരം T_താക്കോൽ& താക്കോൽ) സ്ഥിരം {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == താക്കോൽ.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        തിരികെ m_map.at(താക്കോൽ.id());
    }
    T_മൂല്യം& operator[](സ്ഥിരം T_താക്കോൽ* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        തിരികെ m_map.at(keyp->id());
    }
    സ്ഥിരം T_മൂല്യം& operator[](സ്ഥിരം T_താക്കോൽ* keyp) സ്ഥിരം {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        തിരികെ m_map.at(keyp->id());
    }
};

template <typename T_മൂല്യം>
class CfgBlockMap final : പൊതു CfgMap<CfgBlock, T_മൂല്യം> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(സ്ഥിരം CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_മൂല്യം>{cfgp, cfgp->nBlocks()} {}

പൊതു:
    // Can create an empty map
    CfgBlockMap() = സ്ഥിരസ്ഥിതി;
    // Copyable, movable
    CfgBlockMap(സ്ഥിരം CfgBlockMap&) = സ്ഥിരസ്ഥിതി;
    CfgBlockMap(CfgBlockMap&&) = സ്ഥിരസ്ഥിതി;
    CfgBlockMap& operator=(സ്ഥിരം CfgBlockMap&) = സ്ഥിരസ്ഥിതി;
    CfgBlockMap& operator=(CfgBlockMap&&) = സ്ഥിരസ്ഥിതി;
};

template <typename T_മൂല്യം>
class CfgEdgeMap final : പൊതു CfgMap<CfgEdge, T_മൂല്യം> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(സ്ഥിരം CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_മൂല്യം>{cfgp, cfgp->nEdges()} {}

പൊതു:
    // Can create an empty map
    CfgEdgeMap() = സ്ഥിരസ്ഥിതി;
    // Copyable, movable
    CfgEdgeMap(സ്ഥിരം CfgEdgeMap&) = സ്ഥിരസ്ഥിതി;
    CfgEdgeMap(CfgEdgeMap&&) = സ്ഥിരസ്ഥിതി;
    CfgEdgeMap& operator=(സ്ഥിരം CfgEdgeMap&) = സ്ഥിരസ്ഥിതി;
    CfgEdgeMap& operator=(CfgEdgeMap&&) = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

size_t CfgBlock::id() { തിരികെ m_cfgp->idOf(this); }
size_t CfgBlock::id() സ്ഥിരം { തിരികെ m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    തിരികെ isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
സ്ഥിരം CfgEdge* CfgBlock::takenEdgep() സ്ഥിരം {  // It's always the first edge
    തിരികെ isExit() ? nullptr : static_cast<സ്ഥിരം CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    തിരികെ isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
സ്ഥിരം CfgEdge* CfgBlock::untknEdgep() സ്ഥിരം {  // It's always the second (last) edge
    തിരികെ isBranch() ? static_cast<സ്ഥിരം CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    തിരികെ isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->മുകൾ());
}
സ്ഥിരം CfgBlock* CfgBlock::takenp() സ്ഥിരം {
    തിരികെ isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->മുകൾ());
}
CfgBlock* CfgBlock::untknp() {
    തിരികെ isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->മുകൾ()) : nullptr;
}
സ്ഥിരം CfgBlock* CfgBlock::untknp() സ്ഥിരം {
    തിരികെ isBranch() ? static_cast<സ്ഥിരം CfgBlock*>(outEdges().backp()->മുകൾ()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    തിരികെ static_cast<CfgEdge*>(inEdges().frontp());
}
സ്ഥിരം CfgEdge* CfgBlock::firstPredecessorEdgep() സ്ഥിരം {  //
    തിരികെ static_cast<സ്ഥിരം CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    തിരികെ static_cast<CfgEdge*>(inEdges().backp());
}
സ്ഥിരം CfgEdge* CfgBlock::lastPredecessorEdgep() സ്ഥിരം {  //
    തിരികെ static_cast<സ്ഥിരം CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    തിരികെ isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
സ്ഥിരം CfgBlock* CfgBlock::firstPredecessorp() സ്ഥിരം {  //
    തിരികെ isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    തിരികെ isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
സ്ഥിരം CfgBlock* CfgBlock::lastPredecessorp() സ്ഥിരം {  //
    തിരികെ isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

size_t CfgEdge::id() { തിരികെ m_cfgp->idOf(this); }
size_t CfgEdge::id() സ്ഥിരം { തിരികെ m_cfgp->idOf(this); }

// --- CfgGraph ---

template <typename T_മൂല്യം>
CfgBlockMap<T_മൂല്യം> CfgGraph::makeBlockMap() സ്ഥിരം {
    തിരികെ CfgBlockMap<T_മൂല്യം>{this};
}
template <typename T_മൂല്യം>
CfgEdgeMap<T_മൂല്യം> CfgGraph::makeEdgeMap() സ്ഥിരം {
    തിരികെ CfgEdgeMap<T_മൂല്യം>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_വൃക്ഷം final {
    // STATE
    CfgBlockMap<സ്ഥിരം CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    സ്ഥിരം CfgBlock* intersect(സ്ഥിരം CfgBlock* ap, സ്ഥിരം CfgBlock* bp);

പൊതു:
    // CONSTRUCTOR
    explicit Cfg_Dominator_വൃക്ഷം(സ്ഥിരം CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_വൃക്ഷം() = സ്ഥിരസ്ഥിതി;
    // Copyable, movable
    Cfg_Dominator_വൃക്ഷം(സ്ഥിരം Cfg_Dominator_വൃക്ഷം&) = സ്ഥിരസ്ഥിതി;
    Cfg_Dominator_വൃക്ഷം(Cfg_Dominator_വൃക്ഷം&&) = സ്ഥിരസ്ഥിതി;
    Cfg_Dominator_വൃക്ഷം& operator=(സ്ഥിരം Cfg_Dominator_വൃക്ഷം&) = സ്ഥിരസ്ഥിതി;
    Cfg_Dominator_വൃക്ഷം& operator=(Cfg_Dominator_വൃക്ഷം&&) = സ്ഥിരസ്ഥിതി;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    സ്ഥിരം CfgBlock* closestCommonDominator(സ്ഥിരം CfgBlock* ap, സ്ഥിരം CfgBlock* bp) സ്ഥിരം {
        ആകുമ്പോൾ (ap != bp) {
            എങ്കിൽ (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } അല്ലെങ്കിൽ {
                ap = m_bb2Idom[ap];
            }
        }
        തിരികെ ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(സ്ഥിരം CfgBlock* സ്ഥിരം ap, സ്ഥിരം CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        ആകുമ്പോൾ (true) {
            // True if 'ap' is above (or same as) 'bp'
            എങ്കിൽ (ap == bp) തിരികെ true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            എങ്കിൽ (!bp) തിരികെ false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(സ്ഥിരം CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
