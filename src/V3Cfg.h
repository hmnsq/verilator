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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Graph.h"

#شامل <functional>
#شامل <limits>
#شامل <memory>
#شامل <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class CfgGraph;
class CfgBlock;
class CfgEdge;

template <typename T_کلید, typename T_مقدار>
class CfgMap;
template <typename T_مقدار>
class CfgBlockMap;
template <typename T_مقدار>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : عمومی V3GraphVertex {
    friend class CfgGraph;
    template <typename T_کلید, typename T_مقدار>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    CfgGraph* ثابت m_cfgp;  // The control flow graph this CfgBlock is under
    size_t m_rpo_عدد = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_گره_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    درونخطی explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = پیشفرض;

عمومی:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    درونخطی size_t id();
    درونخطی size_t id() ثابت;

    // Is this the entry block of the CFG?
    bool isEnter() ثابت { بازگشت inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() ثابت { بازگشت outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ثابت { بازگشت outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ثابت { بازگشت inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ثابت { بازگشت inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    درونخطی CfgEdge* takenEdgep();
    درونخطی ثابت CfgEdge* takenEdgep() ثابت;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    درونخطی CfgEdge* untknEdgep();
    درونخطی ثابت CfgEdge* untknEdgep() ثابت;
    // The taken successor block, or nullptr if exit block
    درونخطی CfgBlock* takenp();
    درونخطی ثابت CfgBlock* takenp() ثابت;
    // The untakens successor block, or nullptr if not a branch, or exit block
    درونخطی CfgBlock* untknp();
    درونخطی ثابت CfgBlock* untknp() ثابت;

    // The first predecessor edge, or nullptr if enter block
    درونخطی CfgEdge* firstPredecessorEdgep();
    درونخطی ثابت CfgEdge* firstPredecessorEdgep() ثابت;
    // The last predecessor edge, or nullptr if enter block
    درونخطی CfgEdge* lastPredecessorEdgep();
    درونخطی ثابت CfgEdge* lastPredecessorEdgep() ثابت;
    // The first predecessor block, or nullptr if enter block
    درونخطی CfgBlock* firstPredecessorp();
    درونخطی ثابت CfgBlock* firstPredecessorp() ثابت;
    // The last predecessor block, or nullptr if enter block
    درونخطی CfgBlock* lastPredecessorp();
    درونخطی ثابت CfgBlock* lastPredecessorp() ثابت;

    // Statements in this CfgBlock
    ثابت std::vector<Ast_گره_Stmt*>& stmtps() ثابت { بازگشت m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool operator<(ثابت CfgBlock& that) ثابت { بازگشت id() < that.id(); }
    bool operator>(ثابت CfgBlock& that) ثابت { بازگشت id() > that.id(); }
    bool operator==(ثابت CfgBlock& that) ثابت { بازگشت this == &that; }

    // Iterators
    تهی forEachSuccessor(std::function<تهی(ثابت CfgBlock&)> f) ثابت {
        برای (ثابت V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.بالاترین()));
    }
    تهی forEachPredecessor(std::function<تهی(ثابت CfgBlock&)> f) ثابت {
        برای (ثابت V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_سطر* fileline() ثابت override {
        بازگشت !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ثابت override;
    std::string dotShape() ثابت override;
    std::string dotRank() ثابت override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : عمومی V3GraphEdge {
    friend class CfgGraph;
    template <typename T_کلید, typename T_مقدار>
    friend class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* ثابت m_cfgp;  // The control flow graph this CfgEdge is under
    size_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    درونخطی CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = پیشفرض;

عمومی:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    درونخطی size_t id();
    درونخطی size_t id() ثابت;

    // Source/destination CfgBlock
    ثابت CfgBlock* srcp() ثابت { بازگشت static_cast<ثابت CfgBlock*>(fromp()); }
    ثابت CfgBlock* dstp() ثابت { بازگشت static_cast<ثابت CfgBlock*>(بالاترین()); }
    CfgBlock* srcp() { بازگشت static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { بازگشت static_cast<CfgBlock*>(بالاترین()); }

    // For Graphviz dumps only
    std::string dotLabel() ثابت override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : عمومی V3Graph {
    friend class CfgBlock;
    friend class CfgEdge;
    template <typename T_کلید, typename T_مقدار>
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
    تهی rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        بازگشت new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    تهی addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
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
    تهی addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
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

    size_t idOf(ثابت CfgBlock* bbp) ثابت {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        بازگشت bbp->m_rpo_عدد;
    }

    size_t idOf(ثابت CfgEdge* edgep) ثابت {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        بازگشت edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = پیشفرض;

عمومی:
    ~CfgGraph() override = پیشفرض;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    ایستا std::unique_ptr<CfgGraph> build(Ast_گره* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ثابت CfgBlock& enter() ثابت { بازگشت *m_enterp; }
    ثابت CfgBlock& exit() ثابت { بازگشت *m_exitp; }

    // Number of basic blocks in this graph
    size_t nBlocks() ثابت { بازگشت m_nBlocks; }
    // Number of control flow edges in this graph
    size_t nEdges() ثابت { بازگشت m_nEdges; }

    // Create a CfgBlock map for this graph
    template <typename T_مقدار>
    درونخطی CfgBlockMap<T_مقدار> makeBlockMap() ثابت;
    // Create a CfgEdgeMap map for this graph
    template <typename T_مقدار>
    درونخطی CfgEdgeMap<T_مقدار> makeEdgeMap() ثابت;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() ثابت;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    تهی minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    تهی breakCriticalEdges();

    bool درج_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

template <typename T_کلید, typename T_مقدار>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ثابت CfgGraph* m_cfgp;  // The control flow graph this map is for
    size_t m_created;  // Edit count of CFG this map was created at
    std::vector<T_مقدار> m_map;  // The map, stored as a vector

محافظتشده:
    // CONSTRUCTOR
    explicit CfgMap(ثابت CfgGraph* cfgp, size_t size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

عمومی:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ثابت CfgMap<T_کلید, T_مقدار>&) = پیشفرض;
    CfgMap(CfgMap<T_کلید, T_مقدار>&&) = پیشفرض;
    CfgMap<T_کلید, T_مقدار>& operator=(ثابت CfgMap<T_کلید, T_مقدار>&) = پیشفرض;
    CfgMap<T_کلید, T_مقدار>& operator=(CfgMap<T_کلید, T_مقدار>&&) = پیشفرض;

    T_مقدار& operator[](ثابت T_کلید& کلید) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == کلید.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        بازگشت m_map.at(کلید.id());
    }
    ثابت T_مقدار& operator[](ثابت T_کلید& کلید) ثابت {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == کلید.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        بازگشت m_map.at(کلید.id());
    }
    T_مقدار& operator[](ثابت T_کلید* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        بازگشت m_map.at(keyp->id());
    }
    ثابت T_مقدار& operator[](ثابت T_کلید* keyp) ثابت {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        بازگشت m_map.at(keyp->id());
    }
};

template <typename T_مقدار>
class CfgBlockMap final : عمومی CfgMap<CfgBlock, T_مقدار> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(ثابت CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_مقدار>{cfgp, cfgp->nBlocks()} {}

عمومی:
    // Can create an empty map
    CfgBlockMap() = پیشفرض;
    // Copyable, movable
    CfgBlockMap(ثابت CfgBlockMap&) = پیشفرض;
    CfgBlockMap(CfgBlockMap&&) = پیشفرض;
    CfgBlockMap& operator=(ثابت CfgBlockMap&) = پیشفرض;
    CfgBlockMap& operator=(CfgBlockMap&&) = پیشفرض;
};

template <typename T_مقدار>
class CfgEdgeMap final : عمومی CfgMap<CfgEdge, T_مقدار> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(ثابت CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_مقدار>{cfgp, cfgp->nEdges()} {}

عمومی:
    // Can create an empty map
    CfgEdgeMap() = پیشفرض;
    // Copyable, movable
    CfgEdgeMap(ثابت CfgEdgeMap&) = پیشفرض;
    CfgEdgeMap(CfgEdgeMap&&) = پیشفرض;
    CfgEdgeMap& operator=(ثابت CfgEdgeMap&) = پیشفرض;
    CfgEdgeMap& operator=(CfgEdgeMap&&) = پیشفرض;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

size_t CfgBlock::id() { بازگشت m_cfgp->idOf(this); }
size_t CfgBlock::id() ثابت { بازگشت m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    بازگشت isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
ثابت CfgEdge* CfgBlock::takenEdgep() ثابت {  // It's always the first edge
    بازگشت isExit() ? nullptr : static_cast<ثابت CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    بازگشت isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
ثابت CfgEdge* CfgBlock::untknEdgep() ثابت {  // It's always the second (last) edge
    بازگشت isBranch() ? static_cast<ثابت CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    بازگشت isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->بالاترین());
}
ثابت CfgBlock* CfgBlock::takenp() ثابت {
    بازگشت isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->بالاترین());
}
CfgBlock* CfgBlock::untknp() {
    بازگشت isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->بالاترین()) : nullptr;
}
ثابت CfgBlock* CfgBlock::untknp() ثابت {
    بازگشت isBranch() ? static_cast<ثابت CfgBlock*>(outEdges().backp()->بالاترین()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    بازگشت static_cast<CfgEdge*>(inEdges().frontp());
}
ثابت CfgEdge* CfgBlock::firstPredecessorEdgep() ثابت {  //
    بازگشت static_cast<ثابت CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    بازگشت static_cast<CfgEdge*>(inEdges().backp());
}
ثابت CfgEdge* CfgBlock::lastPredecessorEdgep() ثابت {  //
    بازگشت static_cast<ثابت CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    بازگشت isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
ثابت CfgBlock* CfgBlock::firstPredecessorp() ثابت {  //
    بازگشت isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    بازگشت isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
ثابت CfgBlock* CfgBlock::lastPredecessorp() ثابت {  //
    بازگشت isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

size_t CfgEdge::id() { بازگشت m_cfgp->idOf(this); }
size_t CfgEdge::id() ثابت { بازگشت m_cfgp->idOf(this); }

// --- CfgGraph ---

template <typename T_مقدار>
CfgBlockMap<T_مقدار> CfgGraph::makeBlockMap() ثابت {
    بازگشت CfgBlockMap<T_مقدار>{this};
}
template <typename T_مقدار>
CfgEdgeMap<T_مقدار> CfgGraph::makeEdgeMap() ثابت {
    بازگشت CfgEdgeMap<T_مقدار>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_درخت final {
    // STATE
    CfgBlockMap<ثابت CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ثابت CfgBlock* intersect(ثابت CfgBlock* ap, ثابت CfgBlock* bp);

عمومی:
    // CONSTRUCTOR
    explicit Cfg_Dominator_درخت(ثابت CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_درخت() = پیشفرض;
    // Copyable, movable
    Cfg_Dominator_درخت(ثابت Cfg_Dominator_درخت&) = پیشفرض;
    Cfg_Dominator_درخت(Cfg_Dominator_درخت&&) = پیشفرض;
    Cfg_Dominator_درخت& operator=(ثابت Cfg_Dominator_درخت&) = پیشفرض;
    Cfg_Dominator_درخت& operator=(Cfg_Dominator_درخت&&) = پیشفرض;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ثابت CfgBlock* closestCommonDominator(ثابت CfgBlock* ap, ثابت CfgBlock* bp) ثابت {
        وقتی (ap != bp) {
            اگر (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } وگرنه {
                ap = m_bb2Idom[ap];
            }
        }
        بازگشت ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ثابت CfgBlock* ثابت ap, ثابت CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        وقتی (true) {
            // True if 'ap' is above (or same as) 'bp'
            اگر (ap == bp) بازگشت true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            اگر (!bp) بازگشت false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(ثابت CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
