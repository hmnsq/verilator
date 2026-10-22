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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class CfgGraph;
class CfgBlock;
class CfgEdge;

template <typename T_ଚାବି, typename T_ମୂଲ୍ୟ>
class CfgMap;
template <typename T_ମୂଲ୍ୟ>
class CfgBlockMap;
template <typename T_ମୂଲ୍ୟ>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class CfgBlock final : ସର୍ବସାଧାରଣ V3GraphVertex {
    friend class CfgGraph;
    template <typename T_ଚାବି, typename T_ମୂଲ୍ୟ>
    friend class CfgMap;
    friend class CfgBuilder;

    // STATE
    CfgGraph* ଅଚଳ m_cfgp;  // The control flow graph this CfgBlock is under
    size_t m_rpo_ସଂଖ୍ୟା = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::vector<Ast_ଗଣ୍ଠି_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ଧାଡ଼ିଭିତରେ explicit CfgBlock(CfgGraph* cfgp);
    ~CfgBlock() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    ଧାଡ଼ିଭିତରେ size_t id();
    ଧାଡ଼ିଭିତରେ size_t id() ଅଚଳ;

    // Is this the entry block of the CFG?
    bool isEnter() ଅଚଳ { ଫେରନ୍ତୁ inEmpty(); }
    // Is this the exit block of the CFG?
    bool isExit() ଅଚଳ { ଫେରନ୍ତୁ outEmpty(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ଅଚଳ { ଫେରନ୍ତୁ outEdges().hasMultipleElements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ଅଚଳ { ଫେରନ୍ତୁ inEdges().hasMultipleElements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ଅଚଳ { ଫେରନ୍ତୁ inEdges().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    ଧାଡ଼ିଭିତରେ CfgEdge* takenEdgep();
    ଧାଡ଼ିଭିତରେ ଅଚଳ CfgEdge* takenEdgep() ଅଚଳ;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    ଧାଡ଼ିଭିତରେ CfgEdge* untknEdgep();
    ଧାଡ଼ିଭିତରେ ଅଚଳ CfgEdge* untknEdgep() ଅଚଳ;
    // The taken successor block, or nullptr if exit block
    ଧାଡ଼ିଭିତରେ CfgBlock* takenp();
    ଧାଡ଼ିଭିତରେ ଅଚଳ CfgBlock* takenp() ଅଚଳ;
    // The untakens successor block, or nullptr if not a branch, or exit block
    ଧାଡ଼ିଭିତରେ CfgBlock* untknp();
    ଧାଡ଼ିଭିତରେ ଅଚଳ CfgBlock* untknp() ଅଚଳ;

    // The first predecessor edge, or nullptr if enter block
    ଧାଡ଼ିଭିତରେ CfgEdge* firstPredecessorEdgep();
    ଧାଡ଼ିଭିତରେ ଅଚଳ CfgEdge* firstPredecessorEdgep() ଅଚଳ;
    // The last predecessor edge, or nullptr if enter block
    ଧାଡ଼ିଭିତରେ CfgEdge* lastPredecessorEdgep();
    ଧାଡ଼ିଭିତରେ ଅଚଳ CfgEdge* lastPredecessorEdgep() ଅଚଳ;
    // The first predecessor block, or nullptr if enter block
    ଧାଡ଼ିଭିତରେ CfgBlock* firstPredecessorp();
    ଧାଡ଼ିଭିତରେ ଅଚଳ CfgBlock* firstPredecessorp() ଅଚଳ;
    // The last predecessor block, or nullptr if enter block
    ଧାଡ଼ିଭିତରେ CfgBlock* lastPredecessorp();
    ଧାଡ଼ିଭିତରେ ଅଚଳ CfgBlock* lastPredecessorp() ଅଚଳ;

    // Statements in this CfgBlock
    ଅଚଳ std::vector<Ast_ଗଣ୍ଠି_Stmt*>& stmtps() ଅଚଳ { ଫେରନ୍ତୁ m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool operator<(ଅଚଳ CfgBlock& that) ଅଚଳ { ଫେରନ୍ତୁ id() < that.id(); }
    bool operator>(ଅଚଳ CfgBlock& that) ଅଚଳ { ଫେରନ୍ତୁ id() > that.id(); }
    bool operator==(ଅଚଳ CfgBlock& that) ଅଚଳ { ଫେରନ୍ତୁ this == &that; }

    // Iterators
    ଶୂନ୍ୟ forEachSuccessor(std::function<ଶୂନ୍ୟ(ଅଚଳ CfgBlock&)> f) ଅଚଳ {
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : outEdges()) f(*static_cast<CfgBlock*>(edge.ଶୀର୍ଷ()));
    }
    ଶୂନ୍ୟ forEachPredecessor(std::function<ଶୂନ୍ୟ(ଅଚଳ CfgBlock&)> f) ଅଚଳ {
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : inEdges()) f(*static_cast<CfgBlock*>(edge.fromp()));
    }

    // Source location for debugging
    File_ଧାଡ଼ି* fileline() ଅଚଳ override {
        ଫେରନ୍ତୁ !m_stmtps.empty() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ଅଚଳ override;
    std::string dotShape() ଅଚଳ override;
    std::string dotRank() ଅଚଳ override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : ସର୍ବସାଧାରଣ V3GraphEdge {
    friend class CfgGraph;
    template <typename T_ଚାବି, typename T_ମୂଲ୍ୟ>
    friend class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* ଅଚଳ m_cfgp;  // The control flow graph this CfgEdge is under
    size_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ଧାଡ଼ିଭିତରେ CfgEdge(CfgGraph* graphp, CfgBlock* srcp, CfgBlock* dstp);
    ~CfgEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    ଧାଡ଼ିଭିତରେ size_t id();
    ଧାଡ଼ିଭିତରେ size_t id() ଅଚଳ;

    // Source/destination CfgBlock
    ଅଚଳ CfgBlock* srcp() ଅଚଳ { ଫେରନ୍ତୁ static_cast<ଅଚଳ CfgBlock*>(fromp()); }
    ଅଚଳ CfgBlock* dstp() ଅଚଳ { ଫେରନ୍ତୁ static_cast<ଅଚଳ CfgBlock*>(ଶୀର୍ଷ()); }
    CfgBlock* srcp() { ଫେରନ୍ତୁ static_cast<CfgBlock*>(fromp()); }
    CfgBlock* dstp() { ଫେରନ୍ତୁ static_cast<CfgBlock*>(ଶୀର୍ଷ()); }

    // For Graphviz dumps only
    std::string dotLabel() ଅଚଳ override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : ସର୍ବସାଧାରଣ V3Graph {
    friend class CfgBlock;
    friend class CfgEdge;
    template <typename T_ଚାବି, typename T_ମୂଲ୍ୟ>
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
    ଶୂନ୍ୟ rpoBlocks();

    // Add a new CfgBlock to this graph
    CfgBlock* addBlock() {
        ++m_nEdits;
        ++m_nBlocks;
        ଫେରନ୍ତୁ new CfgBlock{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    ଶୂନ୍ୟ addTakenEdge(CfgBlock* srcp, CfgBlock* dstp) {
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
    ଶୂନ୍ୟ addUntknEdge(CfgBlock* srcp, CfgBlock* dstp) {
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

    size_t idOf(ଅଚଳ CfgBlock* bbp) ଅଚଳ {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        ଫେରନ୍ତୁ bbp->m_rpo_ସଂଖ୍ୟା;
    }

    size_t idOf(ଅଚଳ CfgEdge* edgep) ଅଚଳ {
        UASSERT_OBJ(m_nEdits == m_nLastOrdered, m_enterp, "Cfg was edited but not re-ordered");
        ଫେରନ୍ତୁ edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    CfgBlock* getOrCreateTwoWayJoinFor(CfgBlock* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ~CfgGraph() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    ସ୍ଥିର std::unique_ptr<CfgGraph> build(Ast_ଗଣ୍ଠି* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ଅଚଳ CfgBlock& enter() ଅଚଳ { ଫେରନ୍ତୁ *m_enterp; }
    ଅଚଳ CfgBlock& exit() ଅଚଳ { ଫେରନ୍ତୁ *m_exitp; }

    // Number of basic blocks in this graph
    size_t nBlocks() ଅଚଳ { ଫେରନ୍ତୁ m_nBlocks; }
    // Number of control flow edges in this graph
    size_t nEdges() ଅଚଳ { ଫେରନ୍ତୁ m_nEdges; }

    // Create a CfgBlock map for this graph
    template <typename T_ମୂଲ୍ୟ>
    ଧାଡ଼ିଭିତରେ CfgBlockMap<T_ମୂଲ୍ୟ> makeBlockMap() ଅଚଳ;
    // Create a CfgEdgeMap map for this graph
    template <typename T_ମୂଲ୍ୟ>
    ଧାଡ଼ିଭିତରେ CfgEdgeMap<T_ମୂଲ୍ୟ> makeEdgeMap() ଅଚଳ;

    // Returns true iff the graph contains a loop (back-edge)
    bool containsLoop() ଅଚଳ;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    ଶୂନ୍ୟ minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    ଶୂନ୍ୟ breakCriticalEdges();

    bool ଭର୍ତ୍ତି_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

template <typename T_ଚାବି, typename T_ମୂଲ୍ୟ>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ଅଚଳ CfgGraph* m_cfgp;  // The control flow graph this map is for
    size_t m_created;  // Edit count of CFG this map was created at
    std::vector<T_ମୂଲ୍ୟ> m_map;  // The map, stored as a vector

ସୁରକ୍ଷିତ:
    // CONSTRUCTOR
    explicit CfgMap(ଅଚଳ CfgGraph* cfgp, size_t size)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{size} {}

ସର୍ବସାଧାରଣ:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ଅଚଳ CfgMap<T_ଚାବି, T_ମୂଲ୍ୟ>&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgMap(CfgMap<T_ଚାବି, T_ମୂଲ୍ୟ>&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgMap<T_ଚାବି, T_ମୂଲ୍ୟ>& operator=(ଅଚଳ CfgMap<T_ଚାବି, T_ମୂଲ୍ୟ>&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgMap<T_ଚାବି, T_ମୂଲ୍ୟ>& operator=(CfgMap<T_ଚାବି, T_ମୂଲ୍ୟ>&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    T_ମୂଲ୍ୟ& operator[](ଅଚଳ T_ଚାବି& ଚାବି) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ଚାବି.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ଫେରନ୍ତୁ m_map.at(ଚାବି.id());
    }
    ଅଚଳ T_ମୂଲ୍ୟ& operator[](ଅଚଳ T_ଚାବି& ଚାବି) ଅଚଳ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ଚାବି.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ଫେରନ୍ତୁ m_map.at(ଚାବି.id());
    }
    T_ମୂଲ୍ୟ& operator[](ଅଚଳ T_ଚାବି* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ଫେରନ୍ତୁ m_map.at(keyp->id());
    }
    ଅଚଳ T_ମୂଲ୍ୟ& operator[](ଅଚଳ T_ଚାବି* keyp) ଅଚଳ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ଫେରନ୍ତୁ m_map.at(keyp->id());
    }
};

template <typename T_ମୂଲ୍ୟ>
class CfgBlockMap final : ସର୍ବସାଧାରଣ CfgMap<CfgBlock, T_ମୂଲ୍ୟ> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit CfgBlockMap(ଅଚଳ CfgGraph* cfgp)
        : CfgMap<CfgBlock, T_ମୂଲ୍ୟ>{cfgp, cfgp->nBlocks()} {}

ସର୍ବସାଧାରଣ:
    // Can create an empty map
    CfgBlockMap() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // Copyable, movable
    CfgBlockMap(ଅଚଳ CfgBlockMap&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgBlockMap(CfgBlockMap&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgBlockMap& operator=(ଅଚଳ CfgBlockMap&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgBlockMap& operator=(CfgBlockMap&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

template <typename T_ମୂଲ୍ୟ>
class CfgEdgeMap final : ସର୍ବସାଧାରଣ CfgMap<CfgEdge, T_ମୂଲ୍ୟ> {
    friend class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(ଅଚଳ CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_ମୂଲ୍ୟ>{cfgp, cfgp->nEdges()} {}

ସର୍ବସାଧାରଣ:
    // Can create an empty map
    CfgEdgeMap() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // Copyable, movable
    CfgEdgeMap(ଅଚଳ CfgEdgeMap&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgEdgeMap(CfgEdgeMap&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgEdgeMap& operator=(ଅଚଳ CfgEdgeMap&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CfgEdgeMap& operator=(CfgEdgeMap&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

CfgBlock::CfgBlock(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

size_t CfgBlock::id() { ଫେରନ୍ତୁ m_cfgp->idOf(this); }
size_t CfgBlock::id() ଅଚଳ { ଫେରନ୍ତୁ m_cfgp->idOf(this); }

// Successor edges
CfgEdge* CfgBlock::takenEdgep() {  // It's always the first edge
    ଫେରନ୍ତୁ isExit() ? nullptr : static_cast<CfgEdge*>(outEdges().frontp());
}
ଅଚଳ CfgEdge* CfgBlock::takenEdgep() ଅଚଳ {  // It's always the first edge
    ଫେରନ୍ତୁ isExit() ? nullptr : static_cast<ଅଚଳ CfgEdge*>(outEdges().frontp());
}
CfgEdge* CfgBlock::untknEdgep() {  // It's always the second (last) edge
    ଫେରନ୍ତୁ isBranch() ? static_cast<CfgEdge*>(outEdges().backp()) : nullptr;
}
ଅଚଳ CfgEdge* CfgBlock::untknEdgep() ଅଚଳ {  // It's always the second (last) edge
    ଫେରନ୍ତୁ isBranch() ? static_cast<ଅଚଳ CfgEdge*>(outEdges().backp()) : nullptr;
}
CfgBlock* CfgBlock::takenp() {
    ଫେରନ୍ତୁ isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->ଶୀର୍ଷ());
}
ଅଚଳ CfgBlock* CfgBlock::takenp() ଅଚଳ {
    ଫେରନ୍ତୁ isExit() ? nullptr : static_cast<CfgBlock*>(outEdges().frontp()->ଶୀର୍ଷ());
}
CfgBlock* CfgBlock::untknp() {
    ଫେରନ୍ତୁ isBranch() ? static_cast<CfgBlock*>(outEdges().backp()->ଶୀର୍ଷ()) : nullptr;
}
ଅଚଳ CfgBlock* CfgBlock::untknp() ଅଚଳ {
    ଫେରନ୍ତୁ isBranch() ? static_cast<ଅଚଳ CfgBlock*>(outEdges().backp()->ଶୀର୍ଷ()) : nullptr;
}

// Predecessor edges
CfgEdge* CfgBlock::firstPredecessorEdgep() {  //
    ଫେରନ୍ତୁ static_cast<CfgEdge*>(inEdges().frontp());
}
ଅଚଳ CfgEdge* CfgBlock::firstPredecessorEdgep() ଅଚଳ {  //
    ଫେରନ୍ତୁ static_cast<ଅଚଳ CfgEdge*>(inEdges().frontp());
}
CfgEdge* CfgBlock::lastPredecessorEdgep() {  //
    ଫେରନ୍ତୁ static_cast<CfgEdge*>(inEdges().backp());
}
ଅଚଳ CfgEdge* CfgBlock::lastPredecessorEdgep() ଅଚଳ {  //
    ଫେରନ୍ତୁ static_cast<ଅଚଳ CfgEdge*>(inEdges().backp());
}
CfgBlock* CfgBlock::firstPredecessorp() {  //
    ଫେରନ୍ତୁ isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
ଅଚଳ CfgBlock* CfgBlock::firstPredecessorp() ଅଚଳ {  //
    ଫେରନ୍ତୁ isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
CfgBlock* CfgBlock::lastPredecessorp() {  //
    ଫେରନ୍ତୁ isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}
ଅଚଳ CfgBlock* CfgBlock::lastPredecessorp() ଅଚଳ {  //
    ଫେରନ୍ତୁ isEnter() ? nullptr : lastPredecessorEdgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, CfgBlock* srcp, CfgBlock* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

size_t CfgEdge::id() { ଫେରନ୍ତୁ m_cfgp->idOf(this); }
size_t CfgEdge::id() ଅଚଳ { ଫେରନ୍ତୁ m_cfgp->idOf(this); }

// --- CfgGraph ---

template <typename T_ମୂଲ୍ୟ>
CfgBlockMap<T_ମୂଲ୍ୟ> CfgGraph::makeBlockMap() ଅଚଳ {
    ଫେରନ୍ତୁ CfgBlockMap<T_ମୂଲ୍ୟ>{this};
}
template <typename T_ମୂଲ୍ୟ>
CfgEdgeMap<T_ମୂଲ୍ୟ> CfgGraph::makeEdgeMap() ଅଚଳ {
    ଫେରନ୍ତୁ CfgEdgeMap<T_ମୂଲ୍ୟ>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_ବୃକ୍ଷ final {
    // STATE
    CfgBlockMap<ଅଚଳ CfgBlock*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ଅଚଳ CfgBlock* intersect(ଅଚଳ CfgBlock* ap, ଅଚଳ CfgBlock* bp);

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    explicit Cfg_Dominator_ବୃକ୍ଷ(ଅଚଳ CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_ବୃକ୍ଷ() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // Copyable, movable
    Cfg_Dominator_ବୃକ୍ଷ(ଅଚଳ Cfg_Dominator_ବୃକ୍ଷ&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    Cfg_Dominator_ବୃକ୍ଷ(Cfg_Dominator_ବୃକ୍ଷ&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    Cfg_Dominator_ବୃକ୍ଷ& operator=(ଅଚଳ Cfg_Dominator_ବୃକ୍ଷ&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    Cfg_Dominator_ବୃକ୍ଷ& operator=(Cfg_Dominator_ବୃକ୍ଷ&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ଅଚଳ CfgBlock* closestCommonDominator(ଅଚଳ CfgBlock* ap, ଅଚଳ CfgBlock* bp) ଅଚଳ {
        ଯେତେବେଳେ (ap != bp) {
            ଯଦି (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } ଅନ୍ୟଥା {
                ap = m_bb2Idom[ap];
            }
        }
        ଫେରନ୍ତୁ ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ଅଚଳ CfgBlock* ଅଚଳ ap, ଅଚଳ CfgBlock* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        ଯେତେବେଳେ (true) {
            // True if 'ap' is above (or same as) 'bp'
            ଯଦି (ap == bp) ଫେରନ୍ତୁ true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            ଯଦି (!bp) ଫେରନ୍ତୁ false;
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
std::unique_ptr<std::vector<AstVarScope*>> liveVarScopes(ଅଚଳ CfgGraph&);

}  //namespace V3Cfg

#endif  // VERILATOR_V3CFG_H_
