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

ढाँचा <typename T_साँचो, typename T_मान>
class CfgMap;
ढाँचा <typename T_मान>
class CfgBlockMap;
ढाँचा <typename T_मान>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : सार्वजनिक V3GraphVertex {
    मित्र class CfgGraph;
    ढाँचा <typename T_साँचो, typename T_मान>
    मित्र class CfgMap;
    मित्र class CfgBuilder;

    // STATE
    CfgGraph* अचर m_cfgp;  // The control flow graph this CfgBlock is under
    माप_प्रकार m_rpo_सङ्ख्या = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_गाँठो_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    अन्तःस्थ explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = स्वभाव;

सार्वजनिक:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    अन्तःस्थ माप_प्रकार id();
    अन्तःस्थ माप_प्रकार id() अचर;

    // Is this the entry block of the CFG?
    bool isEnter() अचर { फर्काऊ inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() अचर { फर्काऊ outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() अचर { फर्काऊ outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() अचर { फर्काऊ inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() अचर { फर्काऊ inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    अन्तःस्थ CfgEdge* takenEdgep();
    अन्तःस्थ अचर CfgEdge* takenEdgep() अचर;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    अन्तःस्थ CfgEdge* untknEdgep();
    अन्तःस्थ अचर CfgEdge* untknEdgep() अचर;
    // The taken successor block, or nullptr if exit block
    अन्तःस्थ CfgBlock* takenp();
    अन्तःस्थ अचर CfgBlock* takenp() अचर;
    // The untakens successor block, or nullptr if not a branch, or exit block
    अन्तःस्थ CfgBlock* untknp();
    अन्तःस्थ अचर CfgBlock* untknp() अचर;

    // The first predecessor edge, or nullptr if enter block
    अन्तःस्थ CfgEdge* firstPredecessorEdgep();
    अन्तःस्थ अचर CfgEdge* firstPredecessorEdgep() अचर;
    // The last predecessor edge, or nullptr if enter block
    अन्तःस्थ CfgEdge* lastPredecessorEdgep();
    अन्तःस्थ अचर CfgEdge* lastPredecessorEdgep() अचर;
    // The first predecessor block, or nullptr if enter block
    अन्तःस्थ CfgBlock* firstPredecessorp();
    अन्तःस्थ अचर CfgBlock* firstPredecessorp() अचर;
    // The last predecessor block, or nullptr if enter block
    अन्तःस्थ CfgBlock* lastPredecessorp();
    अन्तःस्थ अचर CfgBlock* lastPredecessorp() अचर;

    // Statements in this CfgBlock
    अचर std::vector<Ast_गाँठो_Stmt*>& stmtps() अचर { फर्काऊ m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool चालक<(अचर CfgBlock& that) अचर { फर्काऊ id() < that.id(); }
    bool चालक>(अचर CfgBlock& that) अचर { फर्काऊ id() > that.id(); }
    bool चालक==(अचर CfgBlock& that) अचर { फर्काऊ this == &that; }

    // Iterators
    शून्य forEachSuccessor(std::function<शून्य(अचर CfgBlock&)> f) अचर {
        लागि (अचर V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.शीर्ष()));
    }
    शून्य forEachPredecessor(std::function<शून्य(अचर CfgBlock&)> f) अचर {
        लागि (अचर V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_पङ्क्ति* fileline() अचर override {
        फर्काऊ !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() अचर override;
    std::string dotShape() अचर override;
    std::string dotRank() अचर override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : सार्वजनिक V3GraphEdge {
    मित्र class CfgGraph;
    ढाँचा <typename T_साँचो, typename T_मान>
    मित्र class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* अचर m_cfgp;  // The control flow graph this CfgEdge is under
    माप_प्रकार m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    अन्तःस्थ CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = स्वभाव;

सार्वजनिक:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    अन्तःस्थ माप_प्रकार id();
    अन्तःस्थ माप_प्रकार id() अचर;

    // Source/destination CfgBlock
    अचर CfgBlock* srcp() अचर { फर्काऊ static_cast<अचर CfgBlock*>(fromp()); }
    अचर CfgBlock* dstp() अचर { फर्काऊ static_cast<अचर CfgBlock*>(शीर्ष()); }
    CfgBlock* srcp() { फर्काऊ static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { फर्काऊ static_cast<CfgBlock*>(शीर्ष()); }

    // For Graphviz dumps only
    std::string dotLabel() अचर override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : सार्वजनिक V3Graph {
    मित्र class CfgBlock;
    मित्र class CfgEdge;
    ढाँचा <typename T_साँचो, typename T_मान>
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
        फर्काऊ new CfgBlock{this};
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

    माप_प्रकार idOf(अचर CfgBlock* bbp) अचर {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        फर्काऊ bbp->m_rpo_सङ्ख्या;
    }

    माप_प्रकार idOf(अचर CfgEdge* edgep) अचर {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        फर्काऊ edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = स्वभाव;

सार्वजनिक:
    ~CfgGraph() override = स्वभाव;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    स्थिर std::unique_ptr<CfgGraph> build(Ast_गाँठो* stmtsp);

    // PUBLIC METHODS

    // Accessors
    अचर CfgBlock& enter() अचर { फर्काऊ *m_enterp; }
    अचर CfgBlock& निकास() अचर { फर्काऊ *m_exitp; }

    // Number of basic blocks in this graph
    माप_प्रकार nBlocks() अचर { फर्काऊ m_nBlocks; }
    // Number of control flow edges in this graph
    माप_प्रकार nEdges() अचर { फर्काऊ m_nEdges; }

    // Create a CfgBlock map for this graph
    ढाँचा <typename T_मान>
    अन्तःस्थ CfgBlockMap<T_मान> makeBlockMap() अचर;
    // Create a CfgEdgeMap map for this graph
    ढाँचा <typename T_मान>
    अन्तःस्थ CfgEdgeMap<T_मान> makeEdgeMap() अचर;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() अचर;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    शून्य minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    शून्य breakCriticalEdges();

    bool घुसाउ_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

ढाँचा <typename T_साँचो, typename T_मान>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    अचर CfgGraph* m_cfgp;  // The control flow graph this map is for
    माप_प्रकार m_created;  // Edit count of CFG this map was created at
    std::vector<T_मान> m_map;  // The map, stored as a vector

रक्षित:
    // CONSTRUCTOR
    explicit CfgMap(अचर CfgGraph* cfgp, माप_प्रकार size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

सार्वजनिक:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(अचर CfgMap<T_साँचो, T_मान>&) = स्वभाव;
    CfgMap(CfgMap<T_साँचो, T_मान>&&) = स्वभाव;
    CfgMap<T_साँचो, T_मान>& चालक=(अचर CfgMap<T_साँचो, T_मान>&) = स्वभाव;
    CfgMap<T_साँचो, T_मान>& चालक=(CfgMap<T_साँचो, T_मान>&&) = स्वभाव;

    T_मान& चालक[](अचर T_साँचो& साँचो) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == साँचो.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        फर्काऊ m_map.at(साँचो.id());
    }
    अचर T_मान& चालक[](अचर T_साँचो& साँचो) अचर {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == साँचो.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        फर्काऊ m_map.at(साँचो.id());
    }
    T_मान& चालक[](अचर T_साँचो* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        फर्काऊ m_map.at(keyp->id());
    }
    अचर T_मान& चालक[](अचर T_साँचो* keyp) अचर {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        फर्काऊ m_map.at(keyp->id());
    }
};

ढाँचा <typename T_मान>
class CfgBlockMap final : सार्वजनिक CfgMap<CfgBlock, T_मान> {
    मित्र class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(अचर CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_मान>{cfgp, cfgp->nBlocks()} {}

सार्वजनिक:
    // Can create an empty map
    CfgBlockMap() = स्वभाव;
    // Copyable, movable
    CfgBlockMap(अचर CfgBlockMap&) = स्वभाव;
    CfgBlockMap(CfgBlockMap&&) = स्वभाव;
    CfgBlockMap& चालक=(अचर CfgBlockMap&) = स्वभाव;
    CfgBlockMap& चालक=(CfgBlockMap&&) = स्वभाव;
};

ढाँचा <typename T_मान>
class CfgEdgeMap final : सार्वजनिक CfgMap<CfgEdge, T_मान> {
    मित्र class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(अचर CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_मान>{cfgp, cfgp->nEdges()} {}

सार्वजनिक:
    // Can create an empty map
    CfgEdgeMap() = स्वभाव;
    // Copyable, movable
    CfgEdgeMap(अचर CfgEdgeMap&) = स्वभाव;
    CfgEdgeMap(CfgEdgeMap&&) = स्वभाव;
    CfgEdgeMap& चालक=(अचर CfgEdgeMap&) = स्वभाव;
    CfgEdgeMap& चालक=(CfgEdgeMap&&) = स्वभाव;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

माप_प्रकार CfgBlock::id() { फर्काऊ m_cfgp->idOf(this); }
माप_प्रकार CfgBlock::id() अचर { फर्काऊ m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    फर्काऊ isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
अचर CfgEdge* CfgBlock::takenEdgep() अचर {  // It's always the first edge
    फर्काऊ isExit() ? nullptr : static_cast<अचर CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    फर्काऊ isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
अचर CfgEdge* CfgBlock::untknEdgep() अचर {  // It's always the second (last) edge
    फर्काऊ isBranch() ? static_cast<अचर CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    फर्काऊ isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->शीर्ष());
}
अचर CfgBlock* CfgBlock::takenp() अचर {
    फर्काऊ isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->शीर्ष());
}
CfgBlock* CfgBlock::untknp() {
    फर्काऊ isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->शीर्ष()) : nullptr;
}
अचर CfgBlock* CfgBlock::untknp() अचर {
    फर्काऊ isBranch() ? static_cast<अचर CfgBlock*>(outEdges().backp()->शीर्ष()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    फर्काऊ static_cast<CfgEdge*>(inEdges().frontp());
}
अचर CfgEdge* CfgBlock::firstPredecessorEdgep() अचर {  //
    फर्काऊ static_cast<अचर CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    फर्काऊ static_cast<CfgEdge*>(inEdges().backp());
}
अचर CfgEdge* CfgBlock::lastPredecessorEdgep() अचर {  //
    फर्काऊ static_cast<अचर CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    फर्काऊ isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
अचर CfgBlock* CfgBlock::firstPredecessorp() अचर {  //
    फर्काऊ isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    फर्काऊ isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
अचर CfgBlock* CfgBlock::lastPredecessorp() अचर {  //
    फर्काऊ isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

माप_प्रकार CfgEdge::id() { फर्काऊ m_cfgp->idOf(this); }
माप_प्रकार CfgEdge::id() अचर { फर्काऊ m_cfgp->idOf(this); }

// --- CfgGraph ---

ढाँचा <typename T_मान>
CfgBlockMap<T_मान> CfgGraph::makeBlockMap() अचर {
    फर्काऊ CfgBlockMap<T_मान>{this};
}
ढाँचा <typename T_मान>
CfgEdgeMap<T_मान> CfgGraph::makeEdgeMap() अचर {
    फर्काऊ CfgEdgeMap<T_मान>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_रूख final {
    // STATE
    CfgBlockMap<अचर CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    अचर CfgBlock* intersect(अचर CfgBlock* ap, अचर CfgBlock* bp);

सार्वजनिक:
    // CONSTRUCTOR
    explicit Cfg_Dominator_रूख(अचर CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_रूख() = स्वभाव;
    // Copyable, movable
    Cfg_Dominator_रूख(अचर Cfg_Dominator_रूख&) = स्वभाव;
    Cfg_Dominator_रूख(Cfg_Dominator_रूख&&) = स्वभाव;
    Cfg_Dominator_रूख& चालक=(अचर Cfg_Dominator_रूख&) = स्वभाव;
    Cfg_Dominator_रूख& चालक=(Cfg_Dominator_रूख&&) = स्वभाव;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    अचर CfgBlock* closestCommonDominator(अचर CfgBlock* ap, अचर CfgBlock* bp) अचर {
        जबसम्म (ap != bp) {
            यदि (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } अन्यथा {
                ap = m_bb2Idom[ap];
            }
        }
        फर्काऊ ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(अचर CfgBlock* अचर ap, अचर CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        जबसम्म (true) {
            // True if 'ap' is above (or same as) 'bp'
            यदि (ap == bp) फर्काऊ true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            यदि (!bp) फर्काऊ false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(अचर CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
