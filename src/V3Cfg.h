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

#నిర్వచితం_కాకపోతే VERILATOR_V3CFG_H_
#నిర్వచించు VERILATOR_V3CFG_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Graph.h"

#అన్తర్భావ <functional>
#అన్తర్భావ <limits>
#అన్తర్భావ <memory>
#అన్తర్భావ <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class Cfg_లేఖాచిత్రం;
class Cfg_ఖండం;
class Cfg_అంచు;

మూస <typename T_కీలకం, typename T_విలువ>
class CfgMap;
మూస <typename T_విలువ>
class Cfg_ఖండం_Map;
మూస <typename T_విలువ>
class Cfg_అంచు_Map;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class Cfg_ఖండం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    మిత్ర class Cfg_లేఖాచిత్రం;
    మూస <typename T_కీలకం, typename T_విలువ>
    మిత్ర class CfgMap;
    మిత్ర class CfgBuilder;

    // STATE
    Cfg_లేఖాచిత్రం* స్థిరరాశి m_cfgp;  // The control flow graph this CfgBlock is under
    కొలత_t m_rpo_సంఖ్య = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::సదిశ<Ast_కణుపు_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    అంతఃవరుస explicit Cfg_ఖండం(Cfg_లేఖాచిత్రం* cfgp);
    ~Cfg_ఖండం() override = అప్రమేయ;

బహిరంగ:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    అంతఃవరుస కొలత_t id();
    అంతఃవరుస కొలత_t id() స్థిరరాశి;

    // Is this the entry block of the CFG?
    bool isEnter() స్థిరరాశి { తిరిగి లోపల_ఖాళీలు(); }
    // Is this the exit block of the CFG?
    bool isExit() స్థిరరాశి { తిరిగి బహిర్గతం_ఖాళీలు(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() స్థిరరాశి { తిరిగి బహిర్గతం_అంచులు().has_గుణిజం_Elements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() స్థిరరాశి { తిరిగి లోపల_అంచులు().has_గుణిజం_Elements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() స్థిరరాశి { తిరిగి లోపల_అంచులు().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    అంతఃవరుస Cfg_అంచు* తీసుకున్న_Edgep();
    అంతఃవరుస స్థిరరాశి Cfg_అంచు* తీసుకున్న_Edgep() స్థిరరాశి;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    అంతఃవరుస Cfg_అంచు* untknEdgep();
    అంతఃవరుస స్థిరరాశి Cfg_అంచు* untknEdgep() స్థిరరాశి;
    // The taken successor block, or nullptr if exit block
    అంతఃవరుస Cfg_ఖండం* takenp();
    అంతఃవరుస స్థిరరాశి Cfg_ఖండం* takenp() స్థిరరాశి;
    // The untakens successor block, or nullptr if not a branch, or exit block
    అంతఃవరుస Cfg_ఖండం* untknp();
    అంతఃవరుస స్థిరరాశి Cfg_ఖండం* untknp() స్థిరరాశి;

    // The first predecessor edge, or nullptr if enter block
    అంతఃవరుస Cfg_అంచు* మొదటి_Predecessor_Edgep();
    అంతఃవరుస స్థిరరాశి Cfg_అంచు* మొదటి_Predecessor_Edgep() స్థిరరాశి;
    // The last predecessor edge, or nullptr if enter block
    అంతఃవరుస Cfg_అంచు* చివరి_Predecessor_Edgep();
    అంతఃవరుస స్థిరరాశి Cfg_అంచు* చివరి_Predecessor_Edgep() స్థిరరాశి;
    // The first predecessor block, or nullptr if enter block
    అంతఃవరుస Cfg_ఖండం* మొదటి_Predecessorp();
    అంతఃవరుస స్థిరరాశి Cfg_ఖండం* మొదటి_Predecessorp() స్థిరరాశి;
    // The last predecessor block, or nullptr if enter block
    అంతఃవరుస Cfg_ఖండం* చివరి_Predecessorp();
    అంతఃవరుస స్థిరరాశి Cfg_ఖండం* చివరి_Predecessorp() స్థిరరాశి;

    // Statements in this CfgBlock
    స్థిరరాశి std::సదిశ<Ast_కణుపు_Stmt*>& stmtps() స్థిరరాశి { తిరిగి m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool క్రియాకారకం<(స్థిరరాశి Cfg_ఖండం& that) స్థిరరాశి { తిరిగి id() < that.id(); }
    bool క్రియాకారకం>(స్థిరరాశి Cfg_ఖండం& that) స్థిరరాశి { తిరిగి id() > that.id(); }
    bool క్రియాకారకం==(స్థిరరాశి Cfg_ఖండం& that) స్థిరరాశి { తిరిగి this == &that; }

    // Iterators
    శూన్య forEachSuccessor(std::function<శూన్య(స్థిరరాశి Cfg_ఖండం&)> f) స్థిరరాశి {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : బహిర్గతం_అంచులు()) f(*static_cast<Cfg_ఖండం*>(అంచు.శిఖరం()));
    }
    శూన్య forEachPredecessor(std::function<శూన్య(స్థిరరాశి Cfg_ఖండం&)> f) స్థిరరాశి {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : లోపల_అంచులు()) f(*static_cast<Cfg_ఖండం*>(అంచు.fromp()));
    }

    // Source location for debugging
    File_పంక్తి* fileline() స్థిరరాశి override {
        తిరిగి !m_stmtps.ఖాళీలు() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() స్థిరరాశి override;
    std::string dotShape() స్థిరరాశి override;
    std::string dotRank() స్థిరరాశి override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class Cfg_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    మిత్ర class Cfg_లేఖాచిత్రం;
    మూస <typename T_కీలకం, typename T_విలువ>
    మిత్ర class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    Cfg_లేఖాచిత్రం* స్థిరరాశి m_cfgp;  // The control flow graph this CfgEdge is under
    కొలత_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    అంతఃవరుస Cfg_అంచు(Cfg_లేఖాచిత్రం* graphp, Cfg_ఖండం* srcp, Cfg_ఖండం* dstp);
    ~Cfg_అంచు() override = అప్రమేయ;

బహిరంగ:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    అంతఃవరుస కొలత_t id();
    అంతఃవరుస కొలత_t id() స్థిరరాశి;

    // Source/destination CfgBlock
    స్థిరరాశి Cfg_ఖండం* srcp() స్థిరరాశి { తిరిగి static_cast<స్థిరరాశి Cfg_ఖండం*>(fromp()); }
    స్థిరరాశి Cfg_ఖండం* dstp() స్థిరరాశి { తిరిగి static_cast<స్థిరరాశి Cfg_ఖండం*>(శిఖరం()); }
    Cfg_ఖండం* srcp() { తిరిగి static_cast<Cfg_ఖండం*>(fromp()); }
    Cfg_ఖండం* dstp() { తిరిగి static_cast<Cfg_ఖండం*>(శిఖరం()); }

    // For Graphviz dumps only
    std::string dotLabel() స్థిరరాశి override;
};

//######################################################################
// CfgGraph - The control flow graph

class Cfg_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    మిత్ర class Cfg_ఖండం;
    మిత్ర class Cfg_అంచు;
    మూస <typename T_కీలకం, typename T_విలువ>
    మిత్ర class CfgMap;
    మిత్ర class CfgBuilder;

    // STATE
    కొలత_t m_nEdits = 0;  // Edit count of this graph
    కొలత_t m_n_చివరి_Ordered = 0;  // Last edit count blocks were ordered
    Cfg_ఖండం* m_enterp = nullptr;  // The singular entry vertex
    Cfg_ఖండం* m_exitp = nullptr;  // The singular exit vertex
    కొలత_t m_n_ఖండాలు = 0;  // Number of CfgBlocks in this CfgGraph
    కొలత_t m_n_అంచులు = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    శూన్య rpo_ఖండాలు();

    // Add a new CfgBlock to this graph
    Cfg_ఖండం* కూడు_ఖండం() {
        ++m_nEdits;
        ++m_n_ఖండాలు;
        తిరిగి new Cfg_ఖండం{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    శూన్య కూడు_తీసుకున్న_అంచు(Cfg_ఖండం* srcp, Cfg_ఖండం* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(!srcp->తీసుకున్న_Edgep(), srcp, "Taken edge should be added first");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_అంచులు;
        new Cfg_అంచు{this, srcp, dstp};
    }

    // Add a new untaken CfgEdge to this CFG
    శూన్య కూడు_Untkn_అంచు(Cfg_ఖండం* srcp, Cfg_ఖండం* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(srcp->తీసుకున్న_Edgep(), srcp, "Untaken edge should be added second");
        UASSERT_OBJ(srcp->takenp() != dstp, srcp, "Untaken branch targets the same block");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_అంచులు;
        new Cfg_అంచు{this, srcp, dstp};
    }

    కొలత_t id_యొక్క(స్థిరరాశి Cfg_ఖండం* bbp) స్థిరరాశి {
        UASSERT_OBJ(m_nEdits == m_n_చివరి_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        తిరిగి bbp->m_rpo_సంఖ్య;
    }

    కొలత_t id_యొక్క(స్థిరరాశి Cfg_అంచు* edgep) స్థిరరాశి {
        UASSERT_OBJ(m_nEdits == m_n_చివరి_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        తిరిగి edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    Cfg_ఖండం* getOrCreateTwoWayJoinFor(Cfg_ఖండం* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    Cfg_లేఖాచిత్రం() = అప్రమేయ;

బహిరంగ:
    ~Cfg_లేఖాచిత్రం() override = అప్రమేయ;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    స్థిర std::unique_ptr<Cfg_లేఖాచిత్రం> build(Ast_కణుపు* stmtsp);

    // PUBLIC METHODS

    // Accessors
    స్థిరరాశి Cfg_ఖండం& enter() స్థిరరాశి { తిరిగి *m_enterp; }
    స్థిరరాశి Cfg_ఖండం& నిష్క్రమించు() స్థిరరాశి { తిరిగి *m_exitp; }

    // Number of basic blocks in this graph
    కొలత_t n_ఖండాలు() స్థిరరాశి { తిరిగి m_n_ఖండాలు; }
    // Number of control flow edges in this graph
    కొలత_t n_అంచులు() స్థిరరాశి { తిరిగి m_n_అంచులు; }

    // Create a CfgBlock map for this graph
    మూస <typename T_విలువ>
    అంతఃవరుస Cfg_ఖండం_Map<T_విలువ> make_ఖండం_Map() స్థిరరాశి;
    // Create a CfgEdgeMap map for this graph
    మూస <typename T_విలువ>
    అంతఃవరుస Cfg_అంచు_Map<T_విలువ> make_అంచు_Map() స్థిరరాశి;

    // Returns true iff the graph contains a loop (back-edge)
    bool కలిగియుంది_Loop() స్థిరరాశి;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    శూన్య minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    శూన్య break_Critical_అంచులు();

    bool చొప్పించు_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

మూస <typename T_కీలకం, typename T_విలువ>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    స్థిరరాశి Cfg_లేఖాచిత్రం* m_cfgp;  // The control flow graph this map is for
    కొలత_t m_created;  // Edit count of CFG this map was created at
    std::సదిశ<T_విలువ> m_map;  // The map, stored as a vector

రక్షిత:
    // CONSTRUCTOR
    explicit CfgMap(స్థిరరాశి Cfg_లేఖాచిత్రం* cfgp, కొలత_t కొలత)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{కొలత} {}

బహిరంగ:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(స్థిరరాశి CfgMap<T_కీలకం, T_విలువ>&) = అప్రమేయ;
    CfgMap(CfgMap<T_కీలకం, T_విలువ>&&) = అప్రమేయ;
    CfgMap<T_కీలకం, T_విలువ>& క్రియాకారకం=(స్థిరరాశి CfgMap<T_కీలకం, T_విలువ>&) = అప్రమేయ;
    CfgMap<T_కీలకం, T_విలువ>& క్రియాకారకం=(CfgMap<T_కీలకం, T_విలువ>&&) = అప్రమేయ;

    T_విలువ& క్రియాకారకం[](స్థిరరాశి T_కీలకం& కీలకం) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == కీలకం.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        తిరిగి m_map.at(కీలకం.id());
    }
    స్థిరరాశి T_విలువ& క్రియాకారకం[](స్థిరరాశి T_కీలకం& కీలకం) స్థిరరాశి {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == కీలకం.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        తిరిగి m_map.at(కీలకం.id());
    }
    T_విలువ& క్రియాకారకం[](స్థిరరాశి T_కీలకం* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        తిరిగి m_map.at(keyp->id());
    }
    స్థిరరాశి T_విలువ& క్రియాకారకం[](స్థిరరాశి T_కీలకం* keyp) స్థిరరాశి {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        తిరిగి m_map.at(keyp->id());
    }
};

మూస <typename T_విలువ>
class Cfg_ఖండం_Map final : బహిరంగ CfgMap<Cfg_ఖండం, T_విలువ> {
    మిత్ర class Cfg_లేఖాచిత్రం;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit Cfg_ఖండం_Map(స్థిరరాశి Cfg_లేఖాచిత్రం* cfgp)
        : CfgMap<Cfg_ఖండం, T_విలువ>{cfgp, cfgp->n_ఖండాలు()} {}

బహిరంగ:
    // Can create an empty map
    Cfg_ఖండం_Map() = అప్రమేయ;
    // Copyable, movable
    Cfg_ఖండం_Map(స్థిరరాశి Cfg_ఖండం_Map&) = అప్రమేయ;
    Cfg_ఖండం_Map(Cfg_ఖండం_Map&&) = అప్రమేయ;
    Cfg_ఖండం_Map& క్రియాకారకం=(స్థిరరాశి Cfg_ఖండం_Map&) = అప్రమేయ;
    Cfg_ఖండం_Map& క్రియాకారకం=(Cfg_ఖండం_Map&&) = అప్రమేయ;
};

మూస <typename T_విలువ>
class Cfg_అంచు_Map final : బహిరంగ CfgMap<Cfg_అంచు, T_విలువ> {
    మిత్ర class Cfg_లేఖాచిత్రం;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit Cfg_అంచు_Map(స్థిరరాశి Cfg_లేఖాచిత్రం* cfgp)
        : CfgMap<Cfg_అంచు, T_విలువ>{cfgp, cfgp->n_అంచులు()} {}

బహిరంగ:
    // Can create an empty map
    Cfg_అంచు_Map() = అప్రమేయ;
    // Copyable, movable
    Cfg_అంచు_Map(స్థిరరాశి Cfg_అంచు_Map&) = అప్రమేయ;
    Cfg_అంచు_Map(Cfg_అంచు_Map&&) = అప్రమేయ;
    Cfg_అంచు_Map& క్రియాకారకం=(స్థిరరాశి Cfg_అంచు_Map&) = అప్రమేయ;
    Cfg_అంచు_Map& క్రియాకారకం=(Cfg_అంచు_Map&&) = అప్రమేయ;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

Cfg_ఖండం::Cfg_ఖండం(Cfg_లేఖాచిత్రం* cfgp)
    : V3_లేఖాచిత్రం_శీర్షం{cfgp}
    , m_cfgp{cfgp} {}

కొలత_t Cfg_ఖండం::id() { తిరిగి m_cfgp->id_యొక్క(this); }
కొలత_t Cfg_ఖండం::id() స్థిరరాశి { తిరిగి m_cfgp->id_యొక్క(this); }

// Successor edges
Cfg_అంచు* Cfg_ఖండం::తీసుకున్న_Edgep() {  // It's always the first edge
    తిరిగి isExit() ? nullptr : static_cast<Cfg_అంచు*>(బహిర్గతం_అంచులు().frontp());
}
స్థిరరాశి Cfg_అంచు* Cfg_ఖండం::తీసుకున్న_Edgep() స్థిరరాశి {  // It's always the first edge
    తిరిగి isExit() ? nullptr : static_cast<స్థిరరాశి Cfg_అంచు*>(బహిర్గతం_అంచులు().frontp());
}
Cfg_అంచు* Cfg_ఖండం::untknEdgep() {  // It's always the second (last) edge
    తిరిగి isBranch() ? static_cast<Cfg_అంచు*>(బహిర్గతం_అంచులు().backp()) : nullptr;
}
స్థిరరాశి Cfg_అంచు* Cfg_ఖండం::untknEdgep() స్థిరరాశి {  // It's always the second (last) edge
    తిరిగి isBranch() ? static_cast<స్థిరరాశి Cfg_అంచు*>(బహిర్గతం_అంచులు().backp()) : nullptr;
}
Cfg_ఖండం* Cfg_ఖండం::takenp() {
    తిరిగి isExit() ? nullptr : static_cast<Cfg_ఖండం*>(బహిర్గతం_అంచులు().frontp()->శిఖరం());
}
స్థిరరాశి Cfg_ఖండం* Cfg_ఖండం::takenp() స్థిరరాశి {
    తిరిగి isExit() ? nullptr : static_cast<Cfg_ఖండం*>(బహిర్గతం_అంచులు().frontp()->శిఖరం());
}
Cfg_ఖండం* Cfg_ఖండం::untknp() {
    తిరిగి isBranch() ? static_cast<Cfg_ఖండం*>(బహిర్గతం_అంచులు().backp()->శిఖరం()) : nullptr;
}
స్థిరరాశి Cfg_ఖండం* Cfg_ఖండం::untknp() స్థిరరాశి {
    తిరిగి isBranch() ? static_cast<స్థిరరాశి Cfg_ఖండం*>(బహిర్గతం_అంచులు().backp()->శిఖరం()) : nullptr;
}

// Predecessor edges
Cfg_అంచు* Cfg_ఖండం::మొదటి_Predecessor_Edgep() {  //
    తిరిగి static_cast<Cfg_అంచు*>(లోపల_అంచులు().frontp());
}
స్థిరరాశి Cfg_అంచు* Cfg_ఖండం::మొదటి_Predecessor_Edgep() స్థిరరాశి {  //
    తిరిగి static_cast<స్థిరరాశి Cfg_అంచు*>(లోపల_అంచులు().frontp());
}
Cfg_అంచు* Cfg_ఖండం::చివరి_Predecessor_Edgep() {  //
    తిరిగి static_cast<Cfg_అంచు*>(లోపల_అంచులు().backp());
}
స్థిరరాశి Cfg_అంచు* Cfg_ఖండం::చివరి_Predecessor_Edgep() స్థిరరాశి {  //
    తిరిగి static_cast<స్థిరరాశి Cfg_అంచు*>(లోపల_అంచులు().backp());
}
Cfg_ఖండం* Cfg_ఖండం::మొదటి_Predecessorp() {  //
    తిరిగి isEnter() ? nullptr : మొదటి_Predecessor_Edgep()->srcp();
}
స్థిరరాశి Cfg_ఖండం* Cfg_ఖండం::మొదటి_Predecessorp() స్థిరరాశి {  //
    తిరిగి isEnter() ? nullptr : మొదటి_Predecessor_Edgep()->srcp();
}
Cfg_ఖండం* Cfg_ఖండం::చివరి_Predecessorp() {  //
    తిరిగి isEnter() ? nullptr : చివరి_Predecessor_Edgep()->srcp();
}
స్థిరరాశి Cfg_ఖండం* Cfg_ఖండం::చివరి_Predecessorp() స్థిరరాశి {  //
    తిరిగి isEnter() ? nullptr : చివరి_Predecessor_Edgep()->srcp();
}

// --- CfgEdge ---

Cfg_అంచు::Cfg_అంచు(Cfg_లేఖాచిత్రం* cfgp, Cfg_ఖండం* srcp, Cfg_ఖండం* dstp)
    : V3_లేఖాచిత్రం_అంచు{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

కొలత_t Cfg_అంచు::id() { తిరిగి m_cfgp->id_యొక్క(this); }
కొలత_t Cfg_అంచు::id() స్థిరరాశి { తిరిగి m_cfgp->id_యొక్క(this); }

// --- CfgGraph ---

మూస <typename T_విలువ>
Cfg_ఖండం_Map<T_విలువ> Cfg_లేఖాచిత్రం::make_ఖండం_Map() స్థిరరాశి {
    తిరిగి Cfg_ఖండం_Map<T_విలువ>{this};
}
మూస <typename T_విలువ>
Cfg_అంచు_Map<T_విలువ> Cfg_లేఖాచిత్రం::make_అంచు_Map() స్థిరరాశి {
    తిరిగి Cfg_అంచు_Map<T_విలువ>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_వృక్షం final {
    // STATE
    Cfg_ఖండం_Map<స్థిరరాశి Cfg_ఖండం*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    స్థిరరాశి Cfg_ఖండం* intersect(స్థిరరాశి Cfg_ఖండం* ap, స్థిరరాశి Cfg_ఖండం* bp);

బహిరంగ:
    // CONSTRUCTOR
    explicit Cfg_Dominator_వృక్షం(స్థిరరాశి Cfg_లేఖాచిత్రం& cfg);
    // Can create an empty map
    Cfg_Dominator_వృక్షం() = అప్రమేయ;
    // Copyable, movable
    Cfg_Dominator_వృక్షం(స్థిరరాశి Cfg_Dominator_వృక్షం&) = అప్రమేయ;
    Cfg_Dominator_వృక్షం(Cfg_Dominator_వృక్షం&&) = అప్రమేయ;
    Cfg_Dominator_వృక్షం& క్రియాకారకం=(స్థిరరాశి Cfg_Dominator_వృక్షం&) = అప్రమేయ;
    Cfg_Dominator_వృక్షం& క్రియాకారకం=(Cfg_Dominator_వృక్షం&&) = అప్రమేయ;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    స్థిరరాశి Cfg_ఖండం* closestCommonDominator(స్థిరరాశి Cfg_ఖండం* ap, స్థిరరాశి Cfg_ఖండం* bp) స్థిరరాశి {
        అయితే (ap != bp) {
            ఒకవేళ (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } లేకపోతే {
                ap = m_bb2Idom[ap];
            }
        }
        తిరిగి ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(స్థిరరాశి Cfg_ఖండం* స్థిరరాశి ap, స్థిరరాశి Cfg_ఖండం* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        అయితే (true) {
            // True if 'ap' is above (or same as) 'bp'
            ఒకవేళ (ap == bp) తిరిగి true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            ఒకవేళ (!bp) తిరిగి false;
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
std::unique_ptr<std::సదిశ<AstVarScope*>> సజీవ_Var_Scopes(స్థిరరాశి Cfg_లేఖాచిత్రం&);

}  //namespace V3Cfg

#అయితే_ముగింపు  // VERILATOR_V3CFG_H_
