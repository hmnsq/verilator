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

ढाँचा <typename T_किल्ली, typename T_मूल्य>
class CfgMap;
ढाँचा <typename T_मूल्य>
class CfgBlockMap;
ढाँचा <typename T_मूल्य>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : सार्वजनिक V3GraphVertex {
    मित्र class CfgGraph;
    ढाँचा <typename T_किल्ली, typename T_मूल्य>
    मित्र class CfgMap;
    मित्र class CfgBuilder;

    // STATE
    CfgGraph* स्थिरांक m_cfgp;  // The control flow graph this CfgBlock is under
    माप_प्रकार m_rpo_संख्या = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_गाठ_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ओळीत explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = पूर्वनिर्धारित;

सार्वजनिक:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    ओळीत माप_प्रकार id();
    ओळीत माप_प्रकार id() स्थिरांक;

    // Is this the entry block of the CFG?
    bool isEnter() स्थिरांक { परत inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() स्थिरांक { परत outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() स्थिरांक { परत outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() स्थिरांक { परत inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() स्थिरांक { परत inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    ओळीत CfgEdge* takenEdgep();
    ओळीत स्थिरांक CfgEdge* takenEdgep() स्थिरांक;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    ओळीत CfgEdge* untknEdgep();
    ओळीत स्थिरांक CfgEdge* untknEdgep() स्थिरांक;
    // The taken successor block, or nullptr if exit block
    ओळीत CfgBlock* takenp();
    ओळीत स्थिरांक CfgBlock* takenp() स्थिरांक;
    // The untakens successor block, or nullptr if not a branch, or exit block
    ओळीत CfgBlock* untknp();
    ओळीत स्थिरांक CfgBlock* untknp() स्थिरांक;

    // The first predecessor edge, or nullptr if enter block
    ओळीत CfgEdge* firstPredecessorEdgep();
    ओळीत स्थिरांक CfgEdge* firstPredecessorEdgep() स्थिरांक;
    // The last predecessor edge, or nullptr if enter block
    ओळीत CfgEdge* lastPredecessorEdgep();
    ओळीत स्थिरांक CfgEdge* lastPredecessorEdgep() स्थिरांक;
    // The first predecessor block, or nullptr if enter block
    ओळीत CfgBlock* firstPredecessorp();
    ओळीत स्थिरांक CfgBlock* firstPredecessorp() स्थिरांक;
    // The last predecessor block, or nullptr if enter block
    ओळीत CfgBlock* lastPredecessorp();
    ओळीत स्थिरांक CfgBlock* lastPredecessorp() स्थिरांक;

    // Statements in this CfgBlock
    स्थिरांक std::vector<Ast_गाठ_Stmt*>& stmtps() स्थिरांक { परत m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool चालक<(स्थिरांक CfgBlock& that) स्थिरांक { परत id() < that.id(); }
    bool चालक>(स्थिरांक CfgBlock& that) स्थिरांक { परत id() > that.id(); }
    bool चालक==(स्थिरांक CfgBlock& that) स्थिरांक { परत this == &that; }

    // Iterators
    शून्य forEachSuccessor(std::function<शून्य(स्थिरांक CfgBlock&)> f) स्थिरांक {
        साठी (स्थिरांक V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.शीर्ष()));
    }
    शून्य forEachPredecessor(std::function<शून्य(स्थिरांक CfgBlock&)> f) स्थिरांक {
        साठी (स्थिरांक V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_ओळ* fileline() स्थिरांक override {
        परत !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() स्थिरांक override;
    std::string dotShape() स्थिरांक override;
    std::string dotRank() स्थिरांक override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : सार्वजनिक V3GraphEdge {
    मित्र class CfgGraph;
    ढाँचा <typename T_किल्ली, typename T_मूल्य>
    मित्र class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* स्थिरांक m_cfgp;  // The control flow graph this CfgEdge is under
    माप_प्रकार m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ओळीत CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = पूर्वनिर्धारित;

सार्वजनिक:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    ओळीत माप_प्रकार id();
    ओळीत माप_प्रकार id() स्थिरांक;

    // Source/destination CfgBlock
    स्थिरांक CfgBlock* srcp() स्थिरांक { परत static_cast<स्थिरांक CfgBlock*>(fromp()); }
    स्थिरांक CfgBlock* dstp() स्थिरांक { परत static_cast<स्थिरांक CfgBlock*>(शीर्ष()); }
    CfgBlock* srcp() { परत static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { परत static_cast<CfgBlock*>(शीर्ष()); }

    // For Graphviz dumps only
    std::string dotLabel() स्थिरांक override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : सार्वजनिक V3Graph {
    मित्र class CfgBlock;
    मित्र class CfgEdge;
    ढाँचा <typename T_किल्ली, typename T_मूल्य>
    मित्र class CfgMap;
    मित्र class CfgBuilder;

    // STATE
    माप_प्रकार m_nEdits = 0;  // Edit count of this graph
    माप_प्रकार m_nLastOrdered = 0;  // Last edit count blocks were ordered
    CfgBlock* m_enterp = nullptr;  // The singular entry vertex
    CfgBlock* m_exitp = nullptr;  // The singular exit vertex
    माप_प्रकार m_nBlocks = 0;  // Number of CfgBlocks in this CfgGraph
    माप_प्रकार m_nEdges = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    शून्य rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        परत new CfgBlock{this};
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

    माप_प्रकार idOf(स्थिरांक CfgBlock* bbp) स्थिरांक {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        परत bbp->m_rpo_संख्या;
    }

    माप_प्रकार idOf(स्थिरांक CfgEdge* edgep) स्थिरांक {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        परत edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = पूर्वनिर्धारित;

सार्वजनिक:
    ~CfgGraph() override = पूर्वनिर्धारित;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    स्थिर std::unique_ptr<CfgGraph> build(Ast_गाठ* stmtsp);

    // PUBLIC METHODS

    // Accessors
    स्थिरांक CfgBlock& enter() स्थिरांक { परत *m_enterp; }
    स्थिरांक CfgBlock& निकास() स्थिरांक { परत *m_exitp; }

    // Number of basic blocks in this graph
    माप_प्रकार nBlocks() स्थिरांक { परत m_nBlocks; }
    // Number of control flow edges in this graph
    माप_प्रकार nEdges() स्थिरांक { परत m_nEdges; }

    // Create a CfgBlock map for this graph
    ढाँचा <typename T_मूल्य>
    ओळीत CfgBlockMap<T_मूल्य> makeBlockMap() स्थिरांक;
    // Create a CfgEdgeMap map for this graph
    ढाँचा <typename T_मूल्य>
    ओळीत CfgEdgeMap<T_मूल्य> makeEdgeMap() स्थिरांक;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() स्थिरांक;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    शून्य minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    शून्य breakCriticalEdges();

    bool घाला_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

ढाँचा <typename T_किल्ली, typename T_मूल्य>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    स्थिरांक CfgGraph* m_cfgp;  // The control flow graph this map is for
    माप_प्रकार m_created;  // Edit count of CFG this map was created at
    std::vector<T_मूल्य> m_map;  // The map, stored as a vector

संरक्षित:
    // CONSTRUCTOR
    explicit CfgMap(स्थिरांक CfgGraph* cfgp, माप_प्रकार size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

सार्वजनिक:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(स्थिरांक CfgMap<T_किल्ली, T_मूल्य>&) = पूर्वनिर्धारित;
    CfgMap(CfgMap<T_किल्ली, T_मूल्य>&&) = पूर्वनिर्धारित;
    CfgMap<T_किल्ली, T_मूल्य>& चालक=(स्थिरांक CfgMap<T_किल्ली, T_मूल्य>&) = पूर्वनिर्धारित;
    CfgMap<T_किल्ली, T_मूल्य>& चालक=(CfgMap<T_किल्ली, T_मूल्य>&&) = पूर्वनिर्धारित;

    T_मूल्य& चालक[](स्थिरांक T_किल्ली& किल्ली) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == किल्ली.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        परत m_map.at(किल्ली.id());
    }
    स्थिरांक T_मूल्य& चालक[](स्थिरांक T_किल्ली& किल्ली) स्थिरांक {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == किल्ली.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        परत m_map.at(किल्ली.id());
    }
    T_मूल्य& चालक[](स्थिरांक T_किल्ली* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        परत m_map.at(keyp->id());
    }
    स्थिरांक T_मूल्य& चालक[](स्थिरांक T_किल्ली* keyp) स्थिरांक {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        परत m_map.at(keyp->id());
    }
};

ढाँचा <typename T_मूल्य>
class CfgBlockMap final : सार्वजनिक CfgMap<CfgBlock, T_मूल्य> {
    मित्र class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(स्थिरांक CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_मूल्य>{cfgp, cfgp->nBlocks()} {}

सार्वजनिक:
    // Can create an empty map
    CfgBlockMap() = पूर्वनिर्धारित;
    // Copyable, movable
    CfgBlockMap(स्थिरांक CfgBlockMap&) = पूर्वनिर्धारित;
    CfgBlockMap(CfgBlockMap&&) = पूर्वनिर्धारित;
    CfgBlockMap& चालक=(स्थिरांक CfgBlockMap&) = पूर्वनिर्धारित;
    CfgBlockMap& चालक=(CfgBlockMap&&) = पूर्वनिर्धारित;
};

ढाँचा <typename T_मूल्य>
class CfgEdgeMap final : सार्वजनिक CfgMap<CfgEdge, T_मूल्य> {
    मित्र class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(स्थिरांक CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_मूल्य>{cfgp, cfgp->nEdges()} {}

सार्वजनिक:
    // Can create an empty map
    CfgEdgeMap() = पूर्वनिर्धारित;
    // Copyable, movable
    CfgEdgeMap(स्थिरांक CfgEdgeMap&) = पूर्वनिर्धारित;
    CfgEdgeMap(CfgEdgeMap&&) = पूर्वनिर्धारित;
    CfgEdgeMap& चालक=(स्थिरांक CfgEdgeMap&) = पूर्वनिर्धारित;
    CfgEdgeMap& चालक=(CfgEdgeMap&&) = पूर्वनिर्धारित;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

माप_प्रकार CfgBlock::id() { परत m_cfgp->idOf(this); }
माप_प्रकार CfgBlock::id() स्थिरांक { परत m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    परत isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
स्थिरांक CfgEdge* CfgBlock::takenEdgep() स्थिरांक {  // It's always the first edge
    परत isExit() ? nullptr : static_cast<स्थिरांक CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    परत isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
स्थिरांक CfgEdge* CfgBlock::untknEdgep() स्थिरांक {  // It's always the second (last) edge
    परत isBranch() ? static_cast<स्थिरांक CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    परत isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->शीर्ष());
}
स्थिरांक CfgBlock* CfgBlock::takenp() स्थिरांक {
    परत isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->शीर्ष());
}
CfgBlock* CfgBlock::untknp() {
    परत isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->शीर्ष()) : nullptr;
}
स्थिरांक CfgBlock* CfgBlock::untknp() स्थिरांक {
    परत isBranch() ? static_cast<स्थिरांक CfgBlock*>(outEdges().backp()->शीर्ष()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    परत static_cast<CfgEdge*>(inEdges().frontp());
}
स्थिरांक CfgEdge* CfgBlock::firstPredecessorEdgep() स्थिरांक {  //
    परत static_cast<स्थिरांक CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    परत static_cast<CfgEdge*>(inEdges().backp());
}
स्थिरांक CfgEdge* CfgBlock::lastPredecessorEdgep() स्थिरांक {  //
    परत static_cast<स्थिरांक CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    परत isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
स्थिरांक CfgBlock* CfgBlock::firstPredecessorp() स्थिरांक {  //
    परत isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    परत isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
स्थिरांक CfgBlock* CfgBlock::lastPredecessorp() स्थिरांक {  //
    परत isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

माप_प्रकार CfgEdge::id() { परत m_cfgp->idOf(this); }
माप_प्रकार CfgEdge::id() स्थिरांक { परत m_cfgp->idOf(this); }

// --- CfgGraph ---

ढाँचा <typename T_मूल्य>
CfgBlockMap<T_मूल्य> CfgGraph::makeBlockMap() स्थिरांक {
    परत CfgBlockMap<T_मूल्य>{this};
}
ढाँचा <typename T_मूल्य>
CfgEdgeMap<T_मूल्य> CfgGraph::makeEdgeMap() स्थिरांक {
    परत CfgEdgeMap<T_मूल्य>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_वृक्ष final {
    // STATE
    CfgBlockMap<स्थिरांक CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    स्थिरांक CfgBlock* intersect(स्थिरांक CfgBlock* ap, स्थिरांक CfgBlock* bp);

सार्वजनिक:
    // CONSTRUCTOR
    explicit Cfg_Dominator_वृक्ष(स्थिरांक CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_वृक्ष() = पूर्वनिर्धारित;
    // Copyable, movable
    Cfg_Dominator_वृक्ष(स्थिरांक Cfg_Dominator_वृक्ष&) = पूर्वनिर्धारित;
    Cfg_Dominator_वृक्ष(Cfg_Dominator_वृक्ष&&) = पूर्वनिर्धारित;
    Cfg_Dominator_वृक्ष& चालक=(स्थिरांक Cfg_Dominator_वृक्ष&) = पूर्वनिर्धारित;
    Cfg_Dominator_वृक्ष& चालक=(Cfg_Dominator_वृक्ष&&) = पूर्वनिर्धारित;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    स्थिरांक CfgBlock* closestCommonDominator(स्थिरांक CfgBlock* ap, स्थिरांक CfgBlock* bp) स्थिरांक {
        पर्यंत (ap != bp) {
            जर (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } अन्यथा {
                ap = m_bb2Idom[ap];
            }
        }
        परत ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(स्थिरांक CfgBlock* स्थिरांक ap, स्थिरांक CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        पर्यंत (true) {
            // True if 'ap' is above (or same as) 'bp'
            जर (ap == bp) परत true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            जर (!bp) परत false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(स्थिरांक CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
