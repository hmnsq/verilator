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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Graph.h"

#अन्तर्भाव <functional>
#अन्तर्भाव <limits>
#अन्तर्भाव <memory>
#अन्तर्भाव <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class CfgGraph;
class CfgBlock;
class CfgEdge;

template <typename T_कुंजी, typename T_मूल्य>
class CfgMap;
template <typename T_मूल्य>
class CfgBlockMap;
template <typename T_मूल्य>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : सार्वजनिक V3GraphVertex {
    friend class CfgGraph;
    template <typename T_कुंजी, typename T_मूल्य>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    CfgGraph* ध्रुव m_cfgp;  // The control flow graph this CfgBlock is under
    size_t m_rpo_संख्या = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_गाँठ_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    अन्तःस्थ explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = स्वभाव;

सार्वजनिक:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    अन्तःस्थ size_t id();
    अन्तःस्थ size_t id() ध्रुव;

    // Is this the entry block of the CFG?
    bool isEnter() ध्रुव { प्रत्यागम inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() ध्रुव { प्रत्यागम outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ध्रुव { प्रत्यागम outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ध्रुव { प्रत्यागम inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ध्रुव { प्रत्यागम inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    अन्तःस्थ CfgEdge* takenEdgep();
    अन्तःस्थ ध्रुव CfgEdge* takenEdgep() ध्रुव;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    अन्तःस्थ CfgEdge* untknEdgep();
    अन्तःस्थ ध्रुव CfgEdge* untknEdgep() ध्रुव;
    // The taken successor block, or nullptr if exit block
    अन्तःस्थ CfgBlock* takenp();
    अन्तःस्थ ध्रुव CfgBlock* takenp() ध्रुव;
    // The untakens successor block, or nullptr if not a branch, or exit block
    अन्तःस्थ CfgBlock* untknp();
    अन्तःस्थ ध्रुव CfgBlock* untknp() ध्रुव;

    // The first predecessor edge, or nullptr if enter block
    अन्तःस्थ CfgEdge* firstPredecessorEdgep();
    अन्तःस्थ ध्रुव CfgEdge* firstPredecessorEdgep() ध्रुव;
    // The last predecessor edge, or nullptr if enter block
    अन्तःस्थ CfgEdge* lastPredecessorEdgep();
    अन्तःस्थ ध्रुव CfgEdge* lastPredecessorEdgep() ध्रुव;
    // The first predecessor block, or nullptr if enter block
    अन्तःस्थ CfgBlock* firstPredecessorp();
    अन्तःस्थ ध्रुव CfgBlock* firstPredecessorp() ध्रुव;
    // The last predecessor block, or nullptr if enter block
    अन्तःस्थ CfgBlock* lastPredecessorp();
    अन्तःस्थ ध्रुव CfgBlock* lastPredecessorp() ध्रुव;

    // Statements in this CfgBlock
    ध्रुव std::vector<Ast_गाँठ_Stmt*>& stmtps() ध्रुव { प्रत्यागम m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool operator<(ध्रुव CfgBlock& that) ध्रुव { प्रत्यागम id() < that.id(); }
    bool operator>(ध्रुव CfgBlock& that) ध्रुव { प्रत्यागम id() > that.id(); }
    bool operator==(ध्रुव CfgBlock& that) ध्रुव { प्रत्यागम this == &that; }

    // Iterators
    शून्य forEachSuccessor(std::function<शून्य(ध्रुव CfgBlock&)> f) ध्रुव {
        प्रति (ध्रुव V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.शीर्ष()));
    }
    शून्य forEachPredecessor(std::function<शून्य(ध्रुव CfgBlock&)> f) ध्रुव {
        प्रति (ध्रुव V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_पंक्ति* fileline() ध्रुव override {
        प्रत्यागम !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ध्रुव override;
    std::string dotShape() ध्रुव override;
    std::string dotRank() ध्रुव override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : सार्वजनिक V3GraphEdge {
    friend class CfgGraph;
    template <typename T_कुंजी, typename T_मूल्य>
    friend class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* ध्रुव m_cfgp;  // The control flow graph this CfgEdge is under
    size_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    अन्तःस्थ CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = स्वभाव;

सार्वजनिक:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    अन्तःस्थ size_t id();
    अन्तःस्थ size_t id() ध्रुव;

    // Source/destination CfgBlock
    ध्रुव CfgBlock* srcp() ध्रुव { प्रत्यागम static_cast<ध्रुव CfgBlock*>(fromp()); }
    ध्रुव CfgBlock* dstp() ध्रुव { प्रत्यागम static_cast<ध्रुव CfgBlock*>(शीर्ष()); }
    CfgBlock* srcp() { प्रत्यागम static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { प्रत्यागम static_cast<CfgBlock*>(शीर्ष()); }

    // For Graphviz dumps only
    std::string dotLabel() ध्रुव override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : सार्वजनिक V3Graph {
    friend class CfgBlock;
    friend class CfgEdge;
    template <typename T_कुंजी, typename T_मूल्य>
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
    शून्य rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        प्रत्यागम new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    शून्य addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
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
    शून्य addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
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

    size_t idOf(ध्रुव CfgBlock* bbp) ध्रुव {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        प्रत्यागम bbp->m_rpo_संख्या;
    }

    size_t idOf(ध्रुव CfgEdge* edgep) ध्रुव {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        प्रत्यागम edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = स्वभाव;

सार्वजनिक:
    ~CfgGraph() override = स्वभाव;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    स्थिर std::unique_ptr<CfgGraph> build(Ast_गाँठ* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ध्रुव CfgBlock& enter() ध्रुव { प्रत्यागम *m_enterp; }
    ध्रुव CfgBlock& exit() ध्रुव { प्रत्यागम *m_exitp; }

    // Number of basic blocks in this graph
    size_t nBlocks() ध्रुव { प्रत्यागम m_nBlocks; }
    // Number of control flow edges in this graph
    size_t nEdges() ध्रुव { प्रत्यागम m_nEdges; }

    // Create a CfgBlock map for this graph
    template <typename T_मूल्य>
    अन्तःस्थ CfgBlockMap<T_मूल्य> makeBlockMap() ध्रुव;
    // Create a CfgEdgeMap map for this graph
    template <typename T_मूल्य>
    अन्तःस्थ CfgEdgeMap<T_मूल्य> makeEdgeMap() ध्रुव;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() ध्रुव;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    शून्य minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    शून्य breakCriticalEdges();

    bool जोड़ो_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

template <typename T_कुंजी, typename T_मूल्य>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ध्रुव CfgGraph* m_cfgp;  // The control flow graph this map is for
    size_t m_created;  // Edit count of CFG this map was created at
    std::vector<T_मूल्य> m_map;  // The map, stored as a vector

रक्षित:
    // CONSTRUCTOR
    explicit CfgMap(ध्रुव CfgGraph* cfgp, size_t size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

सार्वजनिक:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ध्रुव CfgMap<T_कुंजी, T_मूल्य>&) = स्वभाव;
    CfgMap(CfgMap<T_कुंजी, T_मूल्य>&&) = स्वभाव;
    CfgMap<T_कुंजी, T_मूल्य>& operator=(ध्रुव CfgMap<T_कुंजी, T_मूल्य>&) = स्वभाव;
    CfgMap<T_कुंजी, T_मूल्य>& operator=(CfgMap<T_कुंजी, T_मूल्य>&&) = स्वभाव;

    T_मूल्य& operator[](ध्रुव T_कुंजी& कुंजी) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == कुंजी.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        प्रत्यागम m_map.at(कुंजी.id());
    }
    ध्रुव T_मूल्य& operator[](ध्रुव T_कुंजी& कुंजी) ध्रुव {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == कुंजी.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        प्रत्यागम m_map.at(कुंजी.id());
    }
    T_मूल्य& operator[](ध्रुव T_कुंजी* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        प्रत्यागम m_map.at(keyp->id());
    }
    ध्रुव T_मूल्य& operator[](ध्रुव T_कुंजी* keyp) ध्रुव {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        प्रत्यागम m_map.at(keyp->id());
    }
};

template <typename T_मूल्य>
class CfgBlockMap final : सार्वजनिक CfgMap<CfgBlock, T_मूल्य> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(ध्रुव CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_मूल्य>{cfgp, cfgp->nBlocks()} {}

सार्वजनिक:
    // Can create an empty map
    CfgBlockMap() = स्वभाव;
    // Copyable, movable
    CfgBlockMap(ध्रुव CfgBlockMap&) = स्वभाव;
    CfgBlockMap(CfgBlockMap&&) = स्वभाव;
    CfgBlockMap& operator=(ध्रुव CfgBlockMap&) = स्वभाव;
    CfgBlockMap& operator=(CfgBlockMap&&) = स्वभाव;
};

template <typename T_मूल्य>
class CfgEdgeMap final : सार्वजनिक CfgMap<CfgEdge, T_मूल्य> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(ध्रुव CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_मूल्य>{cfgp, cfgp->nEdges()} {}

सार्वजनिक:
    // Can create an empty map
    CfgEdgeMap() = स्वभाव;
    // Copyable, movable
    CfgEdgeMap(ध्रुव CfgEdgeMap&) = स्वभाव;
    CfgEdgeMap(CfgEdgeMap&&) = स्वभाव;
    CfgEdgeMap& operator=(ध्रुव CfgEdgeMap&) = स्वभाव;
    CfgEdgeMap& operator=(CfgEdgeMap&&) = स्वभाव;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

size_t CfgBlock::id() { प्रत्यागम m_cfgp->idOf(this); }
size_t CfgBlock::id() ध्रुव { प्रत्यागम m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    प्रत्यागम isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
ध्रुव CfgEdge* CfgBlock::takenEdgep() ध्रुव {  // It's always the first edge
    प्रत्यागम isExit() ? nullptr : static_cast<ध्रुव CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    प्रत्यागम isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
ध्रुव CfgEdge* CfgBlock::untknEdgep() ध्रुव {  // It's always the second (last) edge
    प्रत्यागम isBranch() ? static_cast<ध्रुव CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    प्रत्यागम isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->शीर्ष());
}
ध्रुव CfgBlock* CfgBlock::takenp() ध्रुव {
    प्रत्यागम isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->शीर्ष());
}
CfgBlock* CfgBlock::untknp() {
    प्रत्यागम isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->शीर्ष()) : nullptr;
}
ध्रुव CfgBlock* CfgBlock::untknp() ध्रुव {
    प्रत्यागम isBranch() ? static_cast<ध्रुव CfgBlock*>(outEdges().backp()->शीर्ष()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    प्रत्यागम static_cast<CfgEdge*>(inEdges().frontp());
}
ध्रुव CfgEdge* CfgBlock::firstPredecessorEdgep() ध्रुव {  //
    प्रत्यागम static_cast<ध्रुव CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    प्रत्यागम static_cast<CfgEdge*>(inEdges().backp());
}
ध्रुव CfgEdge* CfgBlock::lastPredecessorEdgep() ध्रुव {  //
    प्रत्यागम static_cast<ध्रुव CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    प्रत्यागम isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
ध्रुव CfgBlock* CfgBlock::firstPredecessorp() ध्रुव {  //
    प्रत्यागम isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    प्रत्यागम isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
ध्रुव CfgBlock* CfgBlock::lastPredecessorp() ध्रुव {  //
    प्रत्यागम isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

size_t CfgEdge::id() { प्रत्यागम m_cfgp->idOf(this); }
size_t CfgEdge::id() ध्रुव { प्रत्यागम m_cfgp->idOf(this); }

// --- CfgGraph ---

template <typename T_मूल्य>
CfgBlockMap<T_मूल्य> CfgGraph::makeBlockMap() ध्रुव {
    प्रत्यागम CfgBlockMap<T_मूल्य>{this};
}
template <typename T_मूल्य>
CfgEdgeMap<T_मूल्य> CfgGraph::makeEdgeMap() ध्रुव {
    प्रत्यागम CfgEdgeMap<T_मूल्य>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_वृक्ष final {
    // STATE
    CfgBlockMap<ध्रुव CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ध्रुव CfgBlock* intersect(ध्रुव CfgBlock* ap, ध्रुव CfgBlock* bp);

सार्वजनिक:
    // CONSTRUCTOR
    explicit Cfg_Dominator_वृक्ष(ध्रुव CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_वृक्ष() = स्वभाव;
    // Copyable, movable
    Cfg_Dominator_वृक्ष(ध्रुव Cfg_Dominator_वृक्ष&) = स्वभाव;
    Cfg_Dominator_वृक्ष(Cfg_Dominator_वृक्ष&&) = स्वभाव;
    Cfg_Dominator_वृक्ष& operator=(ध्रुव Cfg_Dominator_वृक्ष&) = स्वभाव;
    Cfg_Dominator_वृक्ष& operator=(Cfg_Dominator_वृक्ष&&) = स्वभाव;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ध्रुव CfgBlock* closestCommonDominator(ध्रुव CfgBlock* ap, ध्रुव CfgBlock* bp) ध्रुव {
        यावत् (ap != bp) {
            यदि (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } अन्यथा {
                ap = m_bb2Idom[ap];
            }
        }
        प्रत्यागम ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ध्रुव CfgBlock* ध्रुव ap, ध्रुव CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        यावत् (true) {
            // True if 'ap' is above (or same as) 'bp'
            यदि (ap == bp) प्रत्यागम true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            यदि (!bp) प्रत्यागम false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(ध्रुव CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
