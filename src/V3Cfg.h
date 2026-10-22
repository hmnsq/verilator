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

#यदि_अपरिभाषितम् VERILATOR_V3CFG_H_
#परिभाषय VERILATOR_V3CFG_H_

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
class Cfg_खण्डम्;
class CfgEdge;

प्रतिरूपम् <typename T_कुञ्चिका, typename T_मानम्>
class CfgMap;
प्रतिरूपम् <typename T_मानम्>
class Cfg_खण्डम्_Map;
प्रतिरूपम् <typename T_मानम्>
class CfgEdgeMap;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class Cfg_खण्डम् final : सार्वजनिक V3GraphVertex {
    मित्रम् class CfgGraph;
    प्रतिरूपम् <typename T_कुञ्चिका, typename T_मानम्>
    मित्रम् class CfgMap;
    मित्रम् class CfgBuilder;

    // STATE
    CfgGraph* ध्रुव m_cfgp;  // The control flow graph this CfgBlock is under
    आकार_t m_rpo_सङ्ख्या = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::सदिशम्<Ast_ग्रन्थिः_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    अन्तःस्थ explicit Cfg_खण्डम्(CfgGraph* cfgp);
    ~Cfg_खण्डम्() override = स्वभाव;

सार्वजनिक:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    अन्तःस्थ आकार_t id();
    अन्तःस्थ आकार_t id() ध्रुव;

    // Is this the entry block of the CFG?
    bool isEnter() ध्रुव { प्रत्यागम अन्तर्_रिक्तानि(); }
    // Is this the exit block of the CFG?
    bool isExit() ध्रुव { प्रत्यागम बहिर्गमनम्_रिक्तानि(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ध्रुव { प्रत्यागम बहिर्गमनम्_धाराः().has_गुणजः_Elements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ध्रुव { प्रत्यागम अन्तर्_धाराः().has_गुणजः_Elements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ध्रुव { प्रत्यागम अन्तर्_धाराः().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    अन्तःस्थ CfgEdge* गृहीतम्_Edgep();
    अन्तःस्थ ध्रुव CfgEdge* गृहीतम्_Edgep() ध्रुव;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    अन्तःस्थ CfgEdge* untknEdgep();
    अन्तःस्थ ध्रुव CfgEdge* untknEdgep() ध्रुव;
    // The taken successor block, or nullptr if exit block
    अन्तःस्थ Cfg_खण्डम्* takenp();
    अन्तःस्थ ध्रुव Cfg_खण्डम्* takenp() ध्रुव;
    // The untakens successor block, or nullptr if not a branch, or exit block
    अन्तःस्थ Cfg_खण्डम्* untknp();
    अन्तःस्थ ध्रुव Cfg_खण्डम्* untknp() ध्रुव;

    // The first predecessor edge, or nullptr if enter block
    अन्तःस्थ CfgEdge* firstPredecessorEdgep();
    अन्तःस्थ ध्रुव CfgEdge* firstPredecessorEdgep() ध्रुव;
    // The last predecessor edge, or nullptr if enter block
    अन्तःस्थ CfgEdge* चरमः_Predecessor_Edgep();
    अन्तःस्थ ध्रुव CfgEdge* चरमः_Predecessor_Edgep() ध्रुव;
    // The first predecessor block, or nullptr if enter block
    अन्तःस्थ Cfg_खण्डम्* firstPredecessorp();
    अन्तःस्थ ध्रुव Cfg_खण्डम्* firstPredecessorp() ध्रुव;
    // The last predecessor block, or nullptr if enter block
    अन्तःस्थ Cfg_खण्डम्* चरमः_Predecessorp();
    अन्तःस्थ ध्रुव Cfg_खण्डम्* चरमः_Predecessorp() ध्रुव;

    // Statements in this CfgBlock
    ध्रुव std::सदिशम्<Ast_ग्रन्थिः_Stmt*>& stmtps() ध्रुव { प्रत्यागम m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool संक्रियकः<(ध्रुव Cfg_खण्डम्& that) ध्रुव { प्रत्यागम id() < that.id(); }
    bool संक्रियकः>(ध्रुव Cfg_खण्डम्& that) ध्रुव { प्रत्यागम id() > that.id(); }
    bool संक्रियकः==(ध्रुव Cfg_खण्डम्& that) ध्रुव { प्रत्यागम this == &that; }

    // Iterators
    शून्य forEachSuccessor(std::function<शून्य(ध्रुव Cfg_खण्डम्&)> f) ध्रुव {
        प्रति (ध्रुव V3GraphEdge& edge : बहिर्गमनम्_धाराः()) f(*static_cast<Cfg_खण्डम्*>(edge.शिखरम्()));
    }
    शून्य forEachPredecessor(std::function<शून्य(ध्रुव Cfg_खण्डम्&)> f) ध्रुव {
        प्रति (ध्रुव V3GraphEdge& edge : अन्तर्_धाराः()) f(*static_cast<Cfg_खण्डम्*>(edge.fromp()));
    }

    // Source location for debugging
    File_पङ्क्तिः* fileline() ध्रुव override {
        प्रत्यागम !m_stmtps.रिक्तानि() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ध्रुव override;
    std::string dotShape() ध्रुव override;
    std::string dotRank() ध्रुव override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class CfgEdge final : सार्वजनिक V3GraphEdge {
    मित्रम् class CfgGraph;
    प्रतिरूपम् <typename T_कुञ्चिका, typename T_मानम्>
    मित्रम् class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    CfgGraph* ध्रुव m_cfgp;  // The control flow graph this CfgEdge is under
    आकार_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    अन्तःस्थ CfgEdge(CfgGraph* graphp, Cfg_खण्डम्* srcp, Cfg_खण्डम्* dstp);
    ~CfgEdge() override = स्वभाव;

सार्वजनिक:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    अन्तःस्थ आकार_t id();
    अन्तःस्थ आकार_t id() ध्रुव;

    // Source/destination CfgBlock
    ध्रुव Cfg_खण्डम्* srcp() ध्रुव { प्रत्यागम static_cast<ध्रुव Cfg_खण्डम्*>(fromp()); }
    ध्रुव Cfg_खण्डम्* dstp() ध्रुव { प्रत्यागम static_cast<ध्रुव Cfg_खण्डम्*>(शिखरम्()); }
    Cfg_खण्डम्* srcp() { प्रत्यागम static_cast<Cfg_खण्डम्*>(fromp()); }
    Cfg_खण्डम्* dstp() { प्रत्यागम static_cast<Cfg_खण्डम्*>(शिखरम्()); }

    // For Graphviz dumps only
    std::string dotLabel() ध्रुव override;
};

//######################################################################
// CfgGraph - The control flow graph

class CfgGraph final : सार्वजनिक V3Graph {
    मित्रम् class Cfg_खण्डम्;
    मित्रम् class CfgEdge;
    प्रतिरूपम् <typename T_कुञ्चिका, typename T_मानम्>
    मित्रम् class CfgMap;
    मित्रम् class CfgBuilder;

    // STATE
    आकार_t m_nEdits = 0;  // Edit count of this graph
    आकार_t m_n_चरमः_Ordered = 0;  // Last edit count blocks were ordered
    Cfg_खण्डम्* m_enterp = nullptr;  // The singular entry vertex
    Cfg_खण्डम्* m_exitp = nullptr;  // The singular exit vertex
    आकार_t m_n_खण्डानि = 0;  // Number of CfgBlocks in this CfgGraph
    आकार_t m_n_धाराः = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    शून्य rpo_खण्डानि();

    // Add a new CfgBlock to this graph
    Cfg_खण्डम्* सङ्कलय_खण्डम्() {
        ++m_nEdits;
        ++m_n_खण्डानि;
        प्रत्यागम new Cfg_खण्डम्{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    शून्य सङ्कलय_गृहीतम्_Edge(Cfg_खण्डम्* srcp, Cfg_खण्डम्* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(!srcp->गृहीतम्_Edgep(), srcp, "Taken edge should be added first");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_धाराः;
        new CfgEdge{this, srcp, dstp};
    }

    // Add a new untaken CfgEdge to this CFG
    शून्य सङ्कलय_Untkn_Edge(Cfg_खण्डम्* srcp, Cfg_खण्डम्* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(srcp->गृहीतम्_Edgep(), srcp, "Untaken edge should be added second");
        UASSERT_OBJ(srcp->takenp() != dstp, srcp, "Untaken branch targets the same block");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_धाराः;
        new CfgEdge{this, srcp, dstp};
    }

    आकार_t id_सम्बन्धि(ध्रुव Cfg_खण्डम्* bbp) ध्रुव {
        UASSERT_OBJ(m_nEdits == m_n_चरमः_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        प्रत्यागम bbp->m_rpo_सङ्ख्या;
    }

    आकार_t id_सम्बन्धि(ध्रुव CfgEdge* edgep) ध्रुव {
        UASSERT_OBJ(m_nEdits == m_n_चरमः_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        प्रत्यागम edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    Cfg_खण्डम्* getOrCreateTwoWayJoinFor(Cfg_खण्डम्* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    CfgGraph() = स्वभाव;

सार्वजनिक:
    ~CfgGraph() override = स्वभाव;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    स्थिर std::unique_ptr<CfgGraph> build(Ast_ग्रन्थिः* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ध्रुव Cfg_खण्डम्& enter() ध्रुव { प्रत्यागम *m_enterp; }
    ध्रुव Cfg_खण्डम्& निर्गच्छ() ध्रुव { प्रत्यागम *m_exitp; }

    // Number of basic blocks in this graph
    आकार_t n_खण्डानि() ध्रुव { प्रत्यागम m_n_खण्डानि; }
    // Number of control flow edges in this graph
    आकार_t n_धाराः() ध्रुव { प्रत्यागम m_n_धाराः; }

    // Create a CfgBlock map for this graph
    प्रतिरूपम् <typename T_मानम्>
    अन्तःस्थ Cfg_खण्डम्_Map<T_मानम्> make_खण्डम्_Map() ध्रुव;
    // Create a CfgEdgeMap map for this graph
    प्रतिरूपम् <typename T_मानम्>
    अन्तःस्थ CfgEdgeMap<T_मानम्> makeEdgeMap() ध्रुव;

    // Returns true iff the graph contains a loop (back-edge)
    bool अन्तर्भवति_Loop() ध्रुव;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    शून्य minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    शून्य break_Critical_धाराः();

    bool निवेशय_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

प्रतिरूपम् <typename T_कुञ्चिका, typename T_मानम्>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ध्रुव CfgGraph* m_cfgp;  // The control flow graph this map is for
    आकार_t m_created;  // Edit count of CFG this map was created at
    std::सदिशम्<T_मानम्> m_map;  // The map, stored as a vector

रक्षित:
    // CONSTRUCTOR
    explicit CfgMap(ध्रुव CfgGraph* cfgp, आकार_t आकार)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{आकार} {}

सार्वजनिक:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ध्रुव CfgMap<T_कुञ्चिका, T_मानम्>&) = स्वभाव;
    CfgMap(CfgMap<T_कुञ्चिका, T_मानम्>&&) = स्वभाव;
    CfgMap<T_कुञ्चिका, T_मानम्>& संक्रियकः=(ध्रुव CfgMap<T_कुञ्चिका, T_मानम्>&) = स्वभाव;
    CfgMap<T_कुञ्चिका, T_मानम्>& संक्रियकः=(CfgMap<T_कुञ्चिका, T_मानम्>&&) = स्वभाव;

    T_मानम्& संक्रियकः[](ध्रुव T_कुञ्चिका& कुञ्चिका) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == कुञ्चिका.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        प्रत्यागम m_map.at(कुञ्चिका.id());
    }
    ध्रुव T_मानम्& संक्रियकः[](ध्रुव T_कुञ्चिका& कुञ्चिका) ध्रुव {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == कुञ्चिका.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        प्रत्यागम m_map.at(कुञ्चिका.id());
    }
    T_मानम्& संक्रियकः[](ध्रुव T_कुञ्चिका* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        प्रत्यागम m_map.at(keyp->id());
    }
    ध्रुव T_मानम्& संक्रियकः[](ध्रुव T_कुञ्चिका* keyp) ध्रुव {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        प्रत्यागम m_map.at(keyp->id());
    }
};

प्रतिरूपम् <typename T_मानम्>
class Cfg_खण्डम्_Map final : सार्वजनिक CfgMap<Cfg_खण्डम्, T_मानम्> {
    मित्रम् class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit Cfg_खण्डम्_Map(ध्रुव CfgGraph* cfgp)
        : CfgMap<Cfg_खण्डम्, T_मानम्>{cfgp, cfgp->n_खण्डानि()} {}

सार्वजनिक:
    // Can create an empty map
    Cfg_खण्डम्_Map() = स्वभाव;
    // Copyable, movable
    Cfg_खण्डम्_Map(ध्रुव Cfg_खण्डम्_Map&) = स्वभाव;
    Cfg_खण्डम्_Map(Cfg_खण्डम्_Map&&) = स्वभाव;
    Cfg_खण्डम्_Map& संक्रियकः=(ध्रुव Cfg_खण्डम्_Map&) = स्वभाव;
    Cfg_खण्डम्_Map& संक्रियकः=(Cfg_खण्डम्_Map&&) = स्वभाव;
};

प्रतिरूपम् <typename T_मानम्>
class CfgEdgeMap final : सार्वजनिक CfgMap<CfgEdge, T_मानम्> {
    मित्रम् class CfgGraph;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit CfgEdgeMap(ध्रुव CfgGraph* cfgp)
        : CfgMap<CfgEdge, T_मानम्>{cfgp, cfgp->n_धाराः()} {}

सार्वजनिक:
    // Can create an empty map
    CfgEdgeMap() = स्वभाव;
    // Copyable, movable
    CfgEdgeMap(ध्रुव CfgEdgeMap&) = स्वभाव;
    CfgEdgeMap(CfgEdgeMap&&) = स्वभाव;
    CfgEdgeMap& संक्रियकः=(ध्रुव CfgEdgeMap&) = स्वभाव;
    CfgEdgeMap& संक्रियकः=(CfgEdgeMap&&) = स्वभाव;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

Cfg_खण्डम्::Cfg_खण्डम्(CfgGraph* cfgp)
    : V3GraphVertex{cfgp}
    , m_cfgp{cfgp} {}

आकार_t Cfg_खण्डम्::id() { प्रत्यागम m_cfgp->id_सम्बन्धि(this); }
आकार_t Cfg_खण्डम्::id() ध्रुव { प्रत्यागम m_cfgp->id_सम्बन्धि(this); }

// Successor edges
CfgEdge* Cfg_खण्डम्::गृहीतम्_Edgep() {  // It's always the first edge
    प्रत्यागम isExit() ? nullptr : static_cast<CfgEdge*>(बहिर्गमनम्_धाराः().frontp());
}
ध्रुव CfgEdge* Cfg_खण्डम्::गृहीतम्_Edgep() ध्रुव {  // It's always the first edge
    प्रत्यागम isExit() ? nullptr : static_cast<ध्रुव CfgEdge*>(बहिर्गमनम्_धाराः().frontp());
}
CfgEdge* Cfg_खण्डम्::untknEdgep() {  // It's always the second (last) edge
    प्रत्यागम isBranch() ? static_cast<CfgEdge*>(बहिर्गमनम्_धाराः().backp()) : nullptr;
}
ध्रुव CfgEdge* Cfg_खण्डम्::untknEdgep() ध्रुव {  // It's always the second (last) edge
    प्रत्यागम isBranch() ? static_cast<ध्रुव CfgEdge*>(बहिर्गमनम्_धाराः().backp()) : nullptr;
}
Cfg_खण्डम्* Cfg_खण्डम्::takenp() {
    प्रत्यागम isExit() ? nullptr : static_cast<Cfg_खण्डम्*>(बहिर्गमनम्_धाराः().frontp()->शिखरम्());
}
ध्रुव Cfg_खण्डम्* Cfg_खण्डम्::takenp() ध्रुव {
    प्रत्यागम isExit() ? nullptr : static_cast<Cfg_खण्डम्*>(बहिर्गमनम्_धाराः().frontp()->शिखरम्());
}
Cfg_खण्डम्* Cfg_खण्डम्::untknp() {
    प्रत्यागम isBranch() ? static_cast<Cfg_खण्डम्*>(बहिर्गमनम्_धाराः().backp()->शिखरम्()) : nullptr;
}
ध्रुव Cfg_खण्डम्* Cfg_खण्डम्::untknp() ध्रुव {
    प्रत्यागम isBranch() ? static_cast<ध्रुव Cfg_खण्डम्*>(बहिर्गमनम्_धाराः().backp()->शिखरम्()) : nullptr;
}

// Predecessor edges
CfgEdge* Cfg_खण्डम्::firstPredecessorEdgep() {  //
    प्रत्यागम static_cast<CfgEdge*>(अन्तर्_धाराः().frontp());
}
ध्रुव CfgEdge* Cfg_खण्डम्::firstPredecessorEdgep() ध्रुव {  //
    प्रत्यागम static_cast<ध्रुव CfgEdge*>(अन्तर्_धाराः().frontp());
}
CfgEdge* Cfg_खण्डम्::चरमः_Predecessor_Edgep() {  //
    प्रत्यागम static_cast<CfgEdge*>(अन्तर्_धाराः().backp());
}
ध्रुव CfgEdge* Cfg_खण्डम्::चरमः_Predecessor_Edgep() ध्रुव {  //
    प्रत्यागम static_cast<ध्रुव CfgEdge*>(अन्तर्_धाराः().backp());
}
Cfg_खण्डम्* Cfg_खण्डम्::firstPredecessorp() {  //
    प्रत्यागम isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
ध्रुव Cfg_खण्डम्* Cfg_खण्डम्::firstPredecessorp() ध्रुव {  //
    प्रत्यागम isEnter() ? nullptr : firstPredecessorEdgep()->srcp();
}
Cfg_खण्डम्* Cfg_खण्डम्::चरमः_Predecessorp() {  //
    प्रत्यागम isEnter() ? nullptr : चरमः_Predecessor_Edgep()->srcp();
}
ध्रुव Cfg_खण्डम्* Cfg_खण्डम्::चरमः_Predecessorp() ध्रुव {  //
    प्रत्यागम isEnter() ? nullptr : चरमः_Predecessor_Edgep()->srcp();
}

// --- CfgEdge ---

CfgEdge::CfgEdge(CfgGraph* cfgp, Cfg_खण्डम्* srcp, Cfg_खण्डम्* dstp)
    : V3GraphEdge{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

आकार_t CfgEdge::id() { प्रत्यागम m_cfgp->id_सम्बन्धि(this); }
आकार_t CfgEdge::id() ध्रुव { प्रत्यागम m_cfgp->id_सम्बन्धि(this); }

// --- CfgGraph ---

प्रतिरूपम् <typename T_मानम्>
Cfg_खण्डम्_Map<T_मानम्> CfgGraph::make_खण्डम्_Map() ध्रुव {
    प्रत्यागम Cfg_खण्डम्_Map<T_मानम्>{this};
}
प्रतिरूपम् <typename T_मानम्>
CfgEdgeMap<T_मानम्> CfgGraph::makeEdgeMap() ध्रुव {
    प्रत्यागम CfgEdgeMap<T_मानम्>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_वृक्षः final {
    // STATE
    Cfg_खण्डम्_Map<ध्रुव Cfg_खण्डम्*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ध्रुव Cfg_खण्डम्* intersect(ध्रुव Cfg_खण्डम्* ap, ध्रुव Cfg_खण्डम्* bp);

सार्वजनिक:
    // CONSTRUCTOR
    explicit Cfg_Dominator_वृक्षः(ध्रुव CfgGraph& cfg);
    // Can create an empty map
    Cfg_Dominator_वृक्षः() = स्वभाव;
    // Copyable, movable
    Cfg_Dominator_वृक्षः(ध्रुव Cfg_Dominator_वृक्षः&) = स्वभाव;
    Cfg_Dominator_वृक्षः(Cfg_Dominator_वृक्षः&&) = स्वभाव;
    Cfg_Dominator_वृक्षः& संक्रियकः=(ध्रुव Cfg_Dominator_वृक्षः&) = स्वभाव;
    Cfg_Dominator_वृक्षः& संक्रियकः=(Cfg_Dominator_वृक्षः&&) = स्वभाव;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ध्रुव Cfg_खण्डम्* closestCommonDominator(ध्रुव Cfg_खण्डम्* ap, ध्रुव Cfg_खण्डम्* bp) ध्रुव {
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
    bool dominates(ध्रुव Cfg_खण्डम्* ध्रुव ap, ध्रुव Cfg_खण्डम्* bp) {
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
std::unique_ptr<std::सदिशम्<AstVarScope*>> जीवितम्_Var_Scopes(ध्रुव CfgGraph&);

}  //namespace V3Cfg

#यदि_अन्तः  // VERILATOR_V3CFG_H_
