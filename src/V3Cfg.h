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

#अगर_अघोषित VERILATOR_V3CFG_H_
#घोषणा VERILATOR_V3CFG_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Graph.h"

#समावेश <functional>
#समावेश <limits>
#समावेश <memory>
#समावेश <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class CfgGraph;
class CfgBlock;
class CfgEdge;

ढाँचा <typename T_कुंजी, typename T_मूल्य>
class CfgMap;
ढाँचा <typename T_मूल्य>
class CfgBlockMap;
ढाँचा <typename T_मूल्य>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : खुला V3GraphVertex {
    मित्र class CfgGraph;
    ढाँचा <typename T_कुंजी, typename T_मूल्य>
    मित्र class CfgMap;
    मित्र class CfgBuilder;

    // STATE
    CfgGraph* स्थिर m_cfgp;  // The control flow graph this CfgBlock is under
    माप_प्रकार m_rpo_संख्या = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_गाँठ_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    अंतरभूत explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = शेष;

खुला:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    अंतरभूत माप_प्रकार id();
    अंतरभूत माप_प्रकार id() स्थिर;

    // Is this the entry block of the CFG?
    bool isEnter() स्थिर { वापस inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() स्थिर { वापस outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() स्थिर { वापस outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() स्थिर { वापस inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() स्थिर { वापस inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    अंतरभूत CfgEdge* takenEdgep();
    अंतरभूत स्थिर CfgEdge* takenEdgep() स्थिर;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    अंतरभूत CfgEdge* untknEdgep();
    अंतरभूत स्थिर CfgEdge* untknEdgep() स्थिर;
    // The taken successor block, or nullptr if exit block
    अंतरभूत CfgBlock* takenp();
    अंतरभूत स्थिर CfgBlock* takenp() स्थिर;
    // The untakens successor block, or nullptr if not a branch, or exit block
    अंतरभूत CfgBlock* untknp();
    अंतरभूत स्थिर CfgBlock* untknp() स्थिर;

    // The first predecessor edge, or nullptr if enter block
    अंतरभूत CfgEdge* firstPredecessorEdgep();
    अंतरभूत स्थिर CfgEdge* firstPredecessorEdgep() स्थिर;
    // The last predecessor edge, or nullptr if enter block
    अंतरभूत CfgEdge* lastPredecessorEdgep();
    अंतरभूत स्थिर CfgEdge* lastPredecessorEdgep() स्थिर;
    // The first predecessor block, or nullptr if enter block
    अंतरभूत CfgBlock* firstPredecessorp();
    अंतरभूत स्थिर CfgBlock* firstPredecessorp() स्थिर;
    // The last predecessor block, or nullptr if enter block
    अंतरभूत CfgBlock* lastPredecessorp();
    अंतरभूत स्थिर CfgBlock* lastPredecessorp() स्थिर;

    // Statements in this CfgBlock
    स्थिर std::vector<Ast_गाँठ_Stmt*>& stmtps() स्थिर { वापस m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool चालक<(स्थिर CfgBlock& that) स्थिर { वापस id() < that.id(); }
    bool चालक>(स्थिर CfgBlock& that) स्थिर { वापस id() > that.id(); }
    bool चालक==(स्थिर CfgBlock& that) स्थिर { वापस this == &that; }

    // Iterators
    व्योम forEachSuccessor(std::function<व्योम(स्थिर CfgBlock&)> f) स्थिर {
        क्रम (स्थिर V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.शीर्ष()));
    }
    व्योम forEachPredecessor(std::function<व्योम(स्थिर CfgBlock&)> f) स्थिर {
        क्रम (स्थिर V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_पंक्ति* fileline() स्थिर override {
        वापस !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() स्थिर override;
    std::string dotShape() स्थिर override;
    std::string dotRank() स्थिर override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : खुला V3GraphEdge {
    मित्र class CfgGraph;
    ढाँचा <typename T_कुंजी, typename T_मूल्य>
    मित्र class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* स्थिर m_cfgp;  // The control flow graph this CfgEdge is under
    माप_प्रकार m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    अंतरभूत CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = शेष;

खुला:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    अंतरभूत माप_प्रकार id();
    अंतरभूत माप_प्रकार id() स्थिर;

    // Source/destination CfgBlock
    स्थिर CfgBlock* srcp() स्थिर { वापस static_cast<स्थिर CfgBlock*>(fromp()); }
    स्थिर CfgBlock* dstp() स्थिर { वापस static_cast<स्थिर CfgBlock*>(शीर्ष()); }
    CfgBlock* srcp() { वापस static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { वापस static_cast<CfgBlock*>(शीर्ष()); }

    // For Graphviz dumps only
    std::string dotLabel() स्थिर override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : खुला V3Graph {
    मित्र class CfgBlock;
    मित्र class CfgEdge;
    ढाँचा <typename T_कुंजी, typename T_मूल्य>
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
    व्योम rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        वापस new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    व्योम addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
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
    व्योम addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
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

    माप_प्रकार idOf(स्थिर CfgBlock* bbp) स्थिर {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        वापस bbp->m_rpo_संख्या;
    }

    माप_प्रकार idOf(स्थिर CfgEdge* edgep) स्थिर {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        वापस edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = शेष;

खुला:
    ~CfgGraph() override = शेष;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    अटल std::unique_ptr<CfgGraph> build(Ast_गाँठ* stmtsp);

    // PUBLIC METHODS

    // Accessors
    स्थिर CfgBlock& enter() स्थिर { वापस *m_enterp; }
    स्थिर CfgBlock& निकास() स्थिर { वापस *m_exitp; }

    // Number of basic blocks in this graph
    माप_प्रकार nBlocks() स्थिर { वापस m_nBlocks; }
    // Number of control flow edges in this graph
    माप_प्रकार nEdges() स्थिर { वापस m_nEdges; }

    // Create a CfgBlock map for this graph
    ढाँचा <typename T_मूल्य>
    अंतरभूत CfgBlockMap<T_मूल्य> makeBlockMap() स्थिर;
    // Create a CfgEdgeMap map for this graph
    ढाँचा <typename T_मूल्य>
    अंतरभूत CfgEdgeMap<T_मूल्य> makeEdgeMap() स्थिर;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() स्थिर;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    व्योम minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    व्योम breakCriticalEdges();

    bool जोड़ो_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

ढाँचा <typename T_कुंजी, typename T_मूल्य>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    स्थिर CfgGraph* m_cfgp;  // The control flow graph this map is for
    माप_प्रकार m_created;  // Edit count of CFG this map was created at
    std::vector<T_मूल्य> m_map;  // The map, stored as a vector

रक्षित:
    // CONSTRUCTOR
    explicit CfgMap(स्थिर CfgGraph* cfgp, माप_प्रकार आकार)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{आकार} {}

खुला:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(स्थिर CfgMap<T_कुंजी, T_मूल्य>&) = शेष;
    CfgMap(CfgMap<T_कुंजी, T_मूल्य>&&) = शेष;
    CfgMap<T_कुंजी, T_मूल्य>& चालक=(स्थिर CfgMap<T_कुंजी, T_मूल्य>&) = शेष;
    CfgMap<T_कुंजी, T_मूल्य>& चालक=(CfgMap<T_कुंजी, T_मूल्य>&&) = शेष;

    T_मूल्य& चालक[](स्थिर T_कुंजी& कुंजी) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == कुंजी.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        वापस m_map.at(कुंजी.id());
    }
    स्थिर T_मूल्य& चालक[](स्थिर T_कुंजी& कुंजी) स्थिर {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == कुंजी.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        वापस m_map.at(कुंजी.id());
    }
    T_मूल्य& चालक[](स्थिर T_कुंजी* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        वापस m_map.at(keyp->id());
    }
    स्थिर T_मूल्य& चालक[](स्थिर T_कुंजी* keyp) स्थिर {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        वापस m_map.at(keyp->id());
    }
};

ढाँचा <typename T_मूल्य>
class CfgBlockMap final : खुला CfgMap<CfgBlock, T_मूल्य> {
    मित्र class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(स्थिर CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_मूल्य>{cfgp, cfgp->nBlocks()} {}

खुला:
    // Can create an empty map
    CfgBlockMap() = शेष;
    // Copyable, movable
    CfgBlockMap(स्थिर CfgBlockMap&) = शेष;
    CfgBlockMap(CfgBlockMap&&) = शेष;
    CfgBlockMap& चालक=(स्थिर CfgBlockMap&) = शेष;
    CfgBlockMap& चालक=(CfgBlockMap&&) = शेष;
};

ढाँचा <typename T_मूल्य>
class CfgEdgeMap final : खुला CfgMap<CfgEdge, T_मूल्य> {
    मित्र class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(स्थिर CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_मूल्य>{cfgp, cfgp->nEdges()} {}

खुला:
    // Can create an empty map
    CfgEdgeMap() = शेष;
    // Copyable, movable
    CfgEdgeMap(स्थिर CfgEdgeMap&) = शेष;
    CfgEdgeMap(CfgEdgeMap&&) = शेष;
    CfgEdgeMap& चालक=(स्थिर CfgEdgeMap&) = शेष;
    CfgEdgeMap& चालक=(CfgEdgeMap&&) = शेष;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

माप_प्रकार CfgBlock::id() { वापस m_cfgp->idOf(this); }
माप_प्रकार CfgBlock::id() स्थिर { वापस m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    वापस isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
स्थिर CfgEdge* CfgBlock::takenEdgep() स्थिर {  // It's always the first edge
    वापस isExit() ? nullptr : static_cast<स्थिर CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    वापस isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
स्थिर CfgEdge* CfgBlock::untknEdgep() स्थिर {  // It's always the second (last) edge
    वापस isBranch() ? static_cast<स्थिर CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    वापस isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->शीर्ष());
}
स्थिर CfgBlock* CfgBlock::takenp() स्थिर {
    वापस isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->शीर्ष());
}
CfgBlock* CfgBlock::untknp() {
    वापस isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->शीर्ष()) : nullptr;
}
स्थिर CfgBlock* CfgBlock::untknp() स्थिर {
    वापस isBranch() ? static_cast<स्थिर CfgBlock*>(outEdges().backp()->शीर्ष()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    वापस static_cast<CfgEdge*>(inEdges().frontp());
}
स्थिर CfgEdge* CfgBlock::firstPredecessorEdgep() स्थिर {  //
    वापस static_cast<स्थिर CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    वापस static_cast<CfgEdge*>(inEdges().backp());
}
स्थिर CfgEdge* CfgBlock::lastPredecessorEdgep() स्थिर {  //
    वापस static_cast<स्थिर CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    वापस isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
स्थिर CfgBlock* CfgBlock::firstPredecessorp() स्थिर {  //
    वापस isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    वापस isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
स्थिर CfgBlock* CfgBlock::lastPredecessorp() स्थिर {  //
    वापस isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

माप_प्रकार CfgEdge::id() { वापस m_cfgp->idOf(this); }
माप_प्रकार CfgEdge::id() स्थिर { वापस m_cfgp->idOf(this); }

// --- CfgGraph ---

ढाँचा <typename T_मूल्य>
CfgBlockMap<T_मूल्य> CfgGraph::makeBlockMap() स्थिर {
    वापस CfgBlockMap<T_मूल्य>{this};
}
ढाँचा <typename T_मूल्य>
CfgEdgeMap<T_मूल्य> CfgGraph::makeEdgeMap() स्थिर {
    वापस CfgEdgeMap<T_मूल्य>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_वृक्ष final {
    // STATE
    CfgBlockMap<स्थिर CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    स्थिर CfgBlock* intersect(स्थिर CfgBlock* ap, स्थिर CfgBlock* bp);

खुला:
    // CONSTRUCTOR
    explicit Cfg_Dominator_वृक्ष(स्थिर CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_वृक्ष() = शेष;
    // Copyable, movable
    Cfg_Dominator_वृक्ष(स्थिर Cfg_Dominator_वृक्ष&) = शेष;
    Cfg_Dominator_वृक्ष(Cfg_Dominator_वृक्ष&&) = शेष;
    Cfg_Dominator_वृक्ष& चालक=(स्थिर Cfg_Dominator_वृक्ष&) = शेष;
    Cfg_Dominator_वृक्ष& चालक=(Cfg_Dominator_वृक्ष&&) = शेष;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    स्थिर CfgBlock* closestCommonDominator(स्थिर CfgBlock* ap, स्थिर CfgBlock* bp) स्थिर {
        जबतक (ap != bp) {
            अगर (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } अन्यथा {
                ap = m_bb2Idom[ap];
            }
        }
        वापस ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(स्थिर CfgBlock* स्थिर ap, स्थिर CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        जबतक (true) {
            // True if 'ap' is above (or same as) 'bp'
            अगर (ap == bp) वापस true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            अगर (!bp) वापस false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(स्थिर CfgGraph&);

}  //namespace V3Cfg

#पूर्ण_अगर  // VERILATOR_V3CFG_H_
