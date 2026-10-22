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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3CFG_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3CFG_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class Cfg_ਗ੍ਰਾਫ;
class Cfg_ਖੰਡ;
class Cfg_ਕਿਨਾਰਾ;

ਸਾਂਚਾ <typename T_ਕੁੰਜੀ, typename T_ਮੁੱਲ>
class CfgMap;
ਸਾਂਚਾ <typename T_ਮੁੱਲ>
class Cfg_ਖੰਡ_Map;
ਸਾਂਚਾ <typename T_ਮੁੱਲ>
class Cfg_ਕਿਨਾਰਾ_Map;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class Cfg_ਖੰਡ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    ਮਿੱਤਰ class Cfg_ਗ੍ਰਾਫ;
    ਸਾਂਚਾ <typename T_ਕੁੰਜੀ, typename T_ਮੁੱਲ>
    ਮਿੱਤਰ class CfgMap;
    ਮਿੱਤਰ class CfgBuilder;

    // STATE
    Cfg_ਗ੍ਰਾਫ* ਅਚਲ m_cfgp;  // The control flow graph this CfgBlock is under
    ਮਾਪ_t m_rpo_ਸੰਖਿਆ = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ਸਤਰਅੰਦਰ explicit Cfg_ਖੰਡ(Cfg_ਗ੍ਰਾਫ* cfgp);
    ~Cfg_ਖੰਡ() override = ਮੂਲ;

ਜਨਤਕ:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    ਸਤਰਅੰਦਰ ਮਾਪ_t id();
    ਸਤਰਅੰਦਰ ਮਾਪ_t id() ਅਚਲ;

    // Is this the entry block of the CFG?
    bool isEnter() ਅਚਲ { ਵਾਪਸ ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ(); }
    // Is this the exit block of the CFG?
    bool isExit() ਅਚਲ { ਵਾਪਸ ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ਅਚਲ { ਵਾਪਸ ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().has_ਗੁਣਜ_Elements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ਅਚਲ { ਵਾਪਸ ਵਿੱਚ_ਕਿਨਾਰੇ().has_ਗੁਣਜ_Elements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ਅਚਲ { ਵਾਪਸ ਵਿੱਚ_ਕਿਨਾਰੇ().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    ਸਤਰਅੰਦਰ Cfg_ਕਿਨਾਰਾ* ਲਿਆ_Edgep();
    ਸਤਰਅੰਦਰ ਅਚਲ Cfg_ਕਿਨਾਰਾ* ਲਿਆ_Edgep() ਅਚਲ;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    ਸਤਰਅੰਦਰ Cfg_ਕਿਨਾਰਾ* untknEdgep();
    ਸਤਰਅੰਦਰ ਅਚਲ Cfg_ਕਿਨਾਰਾ* untknEdgep() ਅਚਲ;
    // The taken successor block, or nullptr if exit block
    ਸਤਰਅੰਦਰ Cfg_ਖੰਡ* takenp();
    ਸਤਰਅੰਦਰ ਅਚਲ Cfg_ਖੰਡ* takenp() ਅਚਲ;
    // The untakens successor block, or nullptr if not a branch, or exit block
    ਸਤਰਅੰਦਰ Cfg_ਖੰਡ* untknp();
    ਸਤਰਅੰਦਰ ਅਚਲ Cfg_ਖੰਡ* untknp() ਅਚਲ;

    // The first predecessor edge, or nullptr if enter block
    ਸਤਰਅੰਦਰ Cfg_ਕਿਨਾਰਾ* ਪਹਿਲਾ_Predecessor_Edgep();
    ਸਤਰਅੰਦਰ ਅਚਲ Cfg_ਕਿਨਾਰਾ* ਪਹਿਲਾ_Predecessor_Edgep() ਅਚਲ;
    // The last predecessor edge, or nullptr if enter block
    ਸਤਰਅੰਦਰ Cfg_ਕਿਨਾਰਾ* ਆਖਰੀ_Predecessor_Edgep();
    ਸਤਰਅੰਦਰ ਅਚਲ Cfg_ਕਿਨਾਰਾ* ਆਖਰੀ_Predecessor_Edgep() ਅਚਲ;
    // The first predecessor block, or nullptr if enter block
    ਸਤਰਅੰਦਰ Cfg_ਖੰਡ* ਪਹਿਲਾ_Predecessorp();
    ਸਤਰਅੰਦਰ ਅਚਲ Cfg_ਖੰਡ* ਪਹਿਲਾ_Predecessorp() ਅਚਲ;
    // The last predecessor block, or nullptr if enter block
    ਸਤਰਅੰਦਰ Cfg_ਖੰਡ* ਆਖਰੀ_Predecessorp();
    ਸਤਰਅੰਦਰ ਅਚਲ Cfg_ਖੰਡ* ਆਖਰੀ_Predecessorp() ਅਚਲ;

    // Statements in this CfgBlock
    ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_Stmt*>& stmtps() ਅਚਲ { ਵਾਪਸ m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool ਸੰਚਾਲਕ<(ਅਚਲ Cfg_ਖੰਡ& that) ਅਚਲ { ਵਾਪਸ id() < that.id(); }
    bool ਸੰਚਾਲਕ>(ਅਚਲ Cfg_ਖੰਡ& that) ਅਚਲ { ਵਾਪਸ id() > that.id(); }
    bool ਸੰਚਾਲਕ==(ਅਚਲ Cfg_ਖੰਡ& that) ਅਚਲ { ਵਾਪਸ this == &that; }

    // Iterators
    ਖਾਲੀ forEachSuccessor(std::function<ਖਾਲੀ(ਅਚਲ Cfg_ਖੰਡ&)> f) ਅਚਲ {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) f(*static_cast<Cfg_ਖੰਡ*>(ਕਿਨਾਰਾ.ਸਿਖਰ()));
    }
    ਖਾਲੀ forEachPredecessor(std::function<ਖਾਲੀ(ਅਚਲ Cfg_ਖੰਡ&)> f) ਅਚਲ {
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : ਵਿੱਚ_ਕਿਨਾਰੇ()) f(*static_cast<Cfg_ਖੰਡ*>(ਕਿਨਾਰਾ.fromp()));
    }

    // Source location for debugging
    File_ਸਤਰ* fileline() ਅਚਲ override {
        ਵਾਪਸ !m_stmtps.ਖਾਲੀ_ਥਾਵਾਂ() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ਅਚਲ override;
    std::string dotShape() ਅਚਲ override;
    std::string dotRank() ਅਚਲ override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class Cfg_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    ਮਿੱਤਰ class Cfg_ਗ੍ਰਾਫ;
    ਸਾਂਚਾ <typename T_ਕੁੰਜੀ, typename T_ਮੁੱਲ>
    ਮਿੱਤਰ class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    Cfg_ਗ੍ਰਾਫ* ਅਚਲ m_cfgp;  // The control flow graph this CfgEdge is under
    ਮਾਪ_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    ਸਤਰਅੰਦਰ Cfg_ਕਿਨਾਰਾ(Cfg_ਗ੍ਰਾਫ* graphp, Cfg_ਖੰਡ* srcp, Cfg_ਖੰਡ* dstp);
    ~Cfg_ਕਿਨਾਰਾ() override = ਮੂਲ;

ਜਨਤਕ:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    ਸਤਰਅੰਦਰ ਮਾਪ_t id();
    ਸਤਰਅੰਦਰ ਮਾਪ_t id() ਅਚਲ;

    // Source/destination CfgBlock
    ਅਚਲ Cfg_ਖੰਡ* srcp() ਅਚਲ { ਵਾਪਸ static_cast<ਅਚਲ Cfg_ਖੰਡ*>(fromp()); }
    ਅਚਲ Cfg_ਖੰਡ* dstp() ਅਚਲ { ਵਾਪਸ static_cast<ਅਚਲ Cfg_ਖੰਡ*>(ਸਿਖਰ()); }
    Cfg_ਖੰਡ* srcp() { ਵਾਪਸ static_cast<Cfg_ਖੰਡ*>(fromp()); }
    Cfg_ਖੰਡ* dstp() { ਵਾਪਸ static_cast<Cfg_ਖੰਡ*>(ਸਿਖਰ()); }

    // For Graphviz dumps only
    std::string dotLabel() ਅਚਲ override;
};

//######################################################################
// CfgGraph - The control flow graph

class Cfg_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    ਮਿੱਤਰ class Cfg_ਖੰਡ;
    ਮਿੱਤਰ class Cfg_ਕਿਨਾਰਾ;
    ਸਾਂਚਾ <typename T_ਕੁੰਜੀ, typename T_ਮੁੱਲ>
    ਮਿੱਤਰ class CfgMap;
    ਮਿੱਤਰ class CfgBuilder;

    // STATE
    ਮਾਪ_t m_nEdits = 0;  // Edit count of this graph
    ਮਾਪ_t m_n_ਆਖਰੀ_Ordered = 0;  // Last edit count blocks were ordered
    Cfg_ਖੰਡ* m_enterp = nullptr;  // The singular entry vertex
    Cfg_ਖੰਡ* m_exitp = nullptr;  // The singular exit vertex
    ਮਾਪ_t m_n_ਖੰਡਾਂ = 0;  // Number of CfgBlocks in this CfgGraph
    ਮਾਪ_t m_n_ਕਿਨਾਰੇ = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    ਖਾਲੀ rpo_ਖੰਡਾਂ();

    // Add a new CfgBlock to this graph
    Cfg_ਖੰਡ* ਜੋੜੋ_ਖੰਡ() {
        ++m_nEdits;
        ++m_n_ਖੰਡਾਂ;
        ਵਾਪਸ new Cfg_ਖੰਡ{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    ਖਾਲੀ ਜੋੜੋ_ਲਿਆ_ਕਿਨਾਰਾ(Cfg_ਖੰਡ* srcp, Cfg_ਖੰਡ* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(!srcp->ਲਿਆ_Edgep(), srcp, "Taken edge should be added first");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_ਕਿਨਾਰੇ;
        new Cfg_ਕਿਨਾਰਾ{this, srcp, dstp};
    }

    // Add a new untaken CfgEdge to this CFG
    ਖਾਲੀ ਜੋੜੋ_Untkn_ਕਿਨਾਰਾ(Cfg_ਖੰਡ* srcp, Cfg_ਖੰਡ* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(srcp->ਲਿਆ_Edgep(), srcp, "Untaken edge should be added second");
        UASSERT_OBJ(srcp->takenp() != dstp, srcp, "Untaken branch targets the same block");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_ਕਿਨਾਰੇ;
        new Cfg_ਕਿਨਾਰਾ{this, srcp, dstp};
    }

    ਮਾਪ_t id_ਦਾ(ਅਚਲ Cfg_ਖੰਡ* bbp) ਅਚਲ {
        UASSERT_OBJ(m_nEdits == m_n_ਆਖਰੀ_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        ਵਾਪਸ bbp->m_rpo_ਸੰਖਿਆ;
    }

    ਮਾਪ_t id_ਦਾ(ਅਚਲ Cfg_ਕਿਨਾਰਾ* edgep) ਅਚਲ {
        UASSERT_OBJ(m_nEdits == m_n_ਆਖਰੀ_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        ਵਾਪਸ edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    Cfg_ਖੰਡ* getOrCreateTwoWayJoinFor(Cfg_ਖੰਡ* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    Cfg_ਗ੍ਰਾਫ() = ਮੂਲ;

ਜਨਤਕ:
    ~Cfg_ਗ੍ਰਾਫ() override = ਮੂਲ;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    ਸਥਿਰ std::unique_ptr<Cfg_ਗ੍ਰਾਫ> build(Ast_ਗੰਢ* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ਅਚਲ Cfg_ਖੰਡ& enter() ਅਚਲ { ਵਾਪਸ *m_enterp; }
    ਅਚਲ Cfg_ਖੰਡ& ਬਾਹਰ_ਨਿਕਲੋ() ਅਚਲ { ਵਾਪਸ *m_exitp; }

    // Number of basic blocks in this graph
    ਮਾਪ_t n_ਖੰਡਾਂ() ਅਚਲ { ਵਾਪਸ m_n_ਖੰਡਾਂ; }
    // Number of control flow edges in this graph
    ਮਾਪ_t n_ਕਿਨਾਰੇ() ਅਚਲ { ਵਾਪਸ m_n_ਕਿਨਾਰੇ; }

    // Create a CfgBlock map for this graph
    ਸਾਂਚਾ <typename T_ਮੁੱਲ>
    ਸਤਰਅੰਦਰ Cfg_ਖੰਡ_Map<T_ਮੁੱਲ> make_ਖੰਡ_Map() ਅਚਲ;
    // Create a CfgEdgeMap map for this graph
    ਸਾਂਚਾ <typename T_ਮੁੱਲ>
    ਸਤਰਅੰਦਰ Cfg_ਕਿਨਾਰਾ_Map<T_ਮੁੱਲ> make_ਕਿਨਾਰਾ_Map() ਅਚਲ;

    // Returns true iff the graph contains a loop (back-edge)
    bool ਸ਼ਾਮਲ_ਹੈ_Loop() ਅਚਲ;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    ਖਾਲੀ minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    ਖਾਲੀ break_Critical_ਕਿਨਾਰੇ();

    bool ਪਾਓ_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

ਸਾਂਚਾ <typename T_ਕੁੰਜੀ, typename T_ਮੁੱਲ>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ਅਚਲ Cfg_ਗ੍ਰਾਫ* m_cfgp;  // The control flow graph this map is for
    ਮਾਪ_t m_created;  // Edit count of CFG this map was created at
    std::ਸਦਿਸ਼<T_ਮੁੱਲ> m_map;  // The map, stored as a vector

ਸੁਰੱਖਿਅਤ:
    // CONSTRUCTOR
    explicit CfgMap(ਅਚਲ Cfg_ਗ੍ਰਾਫ* cfgp, ਮਾਪ_t ਮਾਪ)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{ਮਾਪ} {}

ਜਨਤਕ:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ਅਚਲ CfgMap<T_ਕੁੰਜੀ, T_ਮੁੱਲ>&) = ਮੂਲ;
    CfgMap(CfgMap<T_ਕੁੰਜੀ, T_ਮੁੱਲ>&&) = ਮੂਲ;
    CfgMap<T_ਕੁੰਜੀ, T_ਮੁੱਲ>& ਸੰਚਾਲਕ=(ਅਚਲ CfgMap<T_ਕੁੰਜੀ, T_ਮੁੱਲ>&) = ਮੂਲ;
    CfgMap<T_ਕੁੰਜੀ, T_ਮੁੱਲ>& ਸੰਚਾਲਕ=(CfgMap<T_ਕੁੰਜੀ, T_ਮੁੱਲ>&&) = ਮੂਲ;

    T_ਮੁੱਲ& ਸੰਚਾਲਕ[](ਅਚਲ T_ਕੁੰਜੀ& ਕੁੰਜੀ) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ਕੁੰਜੀ.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ਵਾਪਸ m_map.at(ਕੁੰਜੀ.id());
    }
    ਅਚਲ T_ਮੁੱਲ& ਸੰਚਾਲਕ[](ਅਚਲ T_ਕੁੰਜੀ& ਕੁੰਜੀ) ਅਚਲ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ਕੁੰਜੀ.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ਵਾਪਸ m_map.at(ਕੁੰਜੀ.id());
    }
    T_ਮੁੱਲ& ਸੰਚਾਲਕ[](ਅਚਲ T_ਕੁੰਜੀ* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ਵਾਪਸ m_map.at(keyp->id());
    }
    ਅਚਲ T_ਮੁੱਲ& ਸੰਚਾਲਕ[](ਅਚਲ T_ਕੁੰਜੀ* keyp) ਅਚਲ {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        ਵਾਪਸ m_map.at(keyp->id());
    }
};

ਸਾਂਚਾ <typename T_ਮੁੱਲ>
class Cfg_ਖੰਡ_Map final : ਜਨਤਕ CfgMap<Cfg_ਖੰਡ, T_ਮੁੱਲ> {
    ਮਿੱਤਰ class Cfg_ਗ੍ਰਾਫ;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit Cfg_ਖੰਡ_Map(ਅਚਲ Cfg_ਗ੍ਰਾਫ* cfgp)
        : CfgMap<Cfg_ਖੰਡ, T_ਮੁੱਲ>{cfgp, cfgp->n_ਖੰਡਾਂ()} {}

ਜਨਤਕ:
    // Can create an empty map
    Cfg_ਖੰਡ_Map() = ਮੂਲ;
    // Copyable, movable
    Cfg_ਖੰਡ_Map(ਅਚਲ Cfg_ਖੰਡ_Map&) = ਮੂਲ;
    Cfg_ਖੰਡ_Map(Cfg_ਖੰਡ_Map&&) = ਮੂਲ;
    Cfg_ਖੰਡ_Map& ਸੰਚਾਲਕ=(ਅਚਲ Cfg_ਖੰਡ_Map&) = ਮੂਲ;
    Cfg_ਖੰਡ_Map& ਸੰਚਾਲਕ=(Cfg_ਖੰਡ_Map&&) = ਮੂਲ;
};

ਸਾਂਚਾ <typename T_ਮੁੱਲ>
class Cfg_ਕਿਨਾਰਾ_Map final : ਜਨਤਕ CfgMap<Cfg_ਕਿਨਾਰਾ, T_ਮੁੱਲ> {
    ਮਿੱਤਰ class Cfg_ਗ੍ਰਾਫ;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit Cfg_ਕਿਨਾਰਾ_Map(ਅਚਲ Cfg_ਗ੍ਰਾਫ* cfgp)
        : CfgMap<Cfg_ਕਿਨਾਰਾ, T_ਮੁੱਲ>{cfgp, cfgp->n_ਕਿਨਾਰੇ()} {}

ਜਨਤਕ:
    // Can create an empty map
    Cfg_ਕਿਨਾਰਾ_Map() = ਮੂਲ;
    // Copyable, movable
    Cfg_ਕਿਨਾਰਾ_Map(ਅਚਲ Cfg_ਕਿਨਾਰਾ_Map&) = ਮੂਲ;
    Cfg_ਕਿਨਾਰਾ_Map(Cfg_ਕਿਨਾਰਾ_Map&&) = ਮੂਲ;
    Cfg_ਕਿਨਾਰਾ_Map& ਸੰਚਾਲਕ=(ਅਚਲ Cfg_ਕਿਨਾਰਾ_Map&) = ਮੂਲ;
    Cfg_ਕਿਨਾਰਾ_Map& ਸੰਚਾਲਕ=(Cfg_ਕਿਨਾਰਾ_Map&&) = ਮੂਲ;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

Cfg_ਖੰਡ::Cfg_ਖੰਡ(Cfg_ਗ੍ਰਾਫ* cfgp)
    : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{cfgp}
    , m_cfgp{cfgp} {}

ਮਾਪ_t Cfg_ਖੰਡ::id() { ਵਾਪਸ m_cfgp->id_ਦਾ(this); }
ਮਾਪ_t Cfg_ਖੰਡ::id() ਅਚਲ { ਵਾਪਸ m_cfgp->id_ਦਾ(this); }

// Successor edges
Cfg_ਕਿਨਾਰਾ* Cfg_ਖੰਡ::ਲਿਆ_Edgep() {  // It's always the first edge
    ਵਾਪਸ isExit() ? nullptr : static_cast<Cfg_ਕਿਨਾਰਾ*>(ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp());
}
ਅਚਲ Cfg_ਕਿਨਾਰਾ* Cfg_ਖੰਡ::ਲਿਆ_Edgep() ਅਚਲ {  // It's always the first edge
    ਵਾਪਸ isExit() ? nullptr : static_cast<ਅਚਲ Cfg_ਕਿਨਾਰਾ*>(ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp());
}
Cfg_ਕਿਨਾਰਾ* Cfg_ਖੰਡ::untknEdgep() {  // It's always the second (last) edge
    ਵਾਪਸ isBranch() ? static_cast<Cfg_ਕਿਨਾਰਾ*>(ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().backp()) : nullptr;
}
ਅਚਲ Cfg_ਕਿਨਾਰਾ* Cfg_ਖੰਡ::untknEdgep() ਅਚਲ {  // It's always the second (last) edge
    ਵਾਪਸ isBranch() ? static_cast<ਅਚਲ Cfg_ਕਿਨਾਰਾ*>(ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().backp()) : nullptr;
}
Cfg_ਖੰਡ* Cfg_ਖੰਡ::takenp() {
    ਵਾਪਸ isExit() ? nullptr : static_cast<Cfg_ਖੰਡ*>(ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp()->ਸਿਖਰ());
}
ਅਚਲ Cfg_ਖੰਡ* Cfg_ਖੰਡ::takenp() ਅਚਲ {
    ਵਾਪਸ isExit() ? nullptr : static_cast<Cfg_ਖੰਡ*>(ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp()->ਸਿਖਰ());
}
Cfg_ਖੰਡ* Cfg_ਖੰਡ::untknp() {
    ਵਾਪਸ isBranch() ? static_cast<Cfg_ਖੰਡ*>(ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().backp()->ਸਿਖਰ()) : nullptr;
}
ਅਚਲ Cfg_ਖੰਡ* Cfg_ਖੰਡ::untknp() ਅਚਲ {
    ਵਾਪਸ isBranch() ? static_cast<ਅਚਲ Cfg_ਖੰਡ*>(ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().backp()->ਸਿਖਰ()) : nullptr;
}

// Predecessor edges
Cfg_ਕਿਨਾਰਾ* Cfg_ਖੰਡ::ਪਹਿਲਾ_Predecessor_Edgep() {  //
    ਵਾਪਸ static_cast<Cfg_ਕਿਨਾਰਾ*>(ਵਿੱਚ_ਕਿਨਾਰੇ().frontp());
}
ਅਚਲ Cfg_ਕਿਨਾਰਾ* Cfg_ਖੰਡ::ਪਹਿਲਾ_Predecessor_Edgep() ਅਚਲ {  //
    ਵਾਪਸ static_cast<ਅਚਲ Cfg_ਕਿਨਾਰਾ*>(ਵਿੱਚ_ਕਿਨਾਰੇ().frontp());
}
Cfg_ਕਿਨਾਰਾ* Cfg_ਖੰਡ::ਆਖਰੀ_Predecessor_Edgep() {  //
    ਵਾਪਸ static_cast<Cfg_ਕਿਨਾਰਾ*>(ਵਿੱਚ_ਕਿਨਾਰੇ().backp());
}
ਅਚਲ Cfg_ਕਿਨਾਰਾ* Cfg_ਖੰਡ::ਆਖਰੀ_Predecessor_Edgep() ਅਚਲ {  //
    ਵਾਪਸ static_cast<ਅਚਲ Cfg_ਕਿਨਾਰਾ*>(ਵਿੱਚ_ਕਿਨਾਰੇ().backp());
}
Cfg_ਖੰਡ* Cfg_ਖੰਡ::ਪਹਿਲਾ_Predecessorp() {  //
    ਵਾਪਸ isEnter() ? nullptr : ਪਹਿਲਾ_Predecessor_Edgep()->srcp();
}
ਅਚਲ Cfg_ਖੰਡ* Cfg_ਖੰਡ::ਪਹਿਲਾ_Predecessorp() ਅਚਲ {  //
    ਵਾਪਸ isEnter() ? nullptr : ਪਹਿਲਾ_Predecessor_Edgep()->srcp();
}
Cfg_ਖੰਡ* Cfg_ਖੰਡ::ਆਖਰੀ_Predecessorp() {  //
    ਵਾਪਸ isEnter() ? nullptr : ਆਖਰੀ_Predecessor_Edgep()->srcp();
}
ਅਚਲ Cfg_ਖੰਡ* Cfg_ਖੰਡ::ਆਖਰੀ_Predecessorp() ਅਚਲ {  //
    ਵਾਪਸ isEnter() ? nullptr : ਆਖਰੀ_Predecessor_Edgep()->srcp();
}

// --- CfgEdge ---

Cfg_ਕਿਨਾਰਾ::Cfg_ਕਿਨਾਰਾ(Cfg_ਗ੍ਰਾਫ* cfgp, Cfg_ਖੰਡ* srcp, Cfg_ਖੰਡ* dstp)
    : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

ਮਾਪ_t Cfg_ਕਿਨਾਰਾ::id() { ਵਾਪਸ m_cfgp->id_ਦਾ(this); }
ਮਾਪ_t Cfg_ਕਿਨਾਰਾ::id() ਅਚਲ { ਵਾਪਸ m_cfgp->id_ਦਾ(this); }

// --- CfgGraph ---

ਸਾਂਚਾ <typename T_ਮੁੱਲ>
Cfg_ਖੰਡ_Map<T_ਮੁੱਲ> Cfg_ਗ੍ਰਾਫ::make_ਖੰਡ_Map() ਅਚਲ {
    ਵਾਪਸ Cfg_ਖੰਡ_Map<T_ਮੁੱਲ>{this};
}
ਸਾਂਚਾ <typename T_ਮੁੱਲ>
Cfg_ਕਿਨਾਰਾ_Map<T_ਮੁੱਲ> Cfg_ਗ੍ਰਾਫ::make_ਕਿਨਾਰਾ_Map() ਅਚਲ {
    ਵਾਪਸ Cfg_ਕਿਨਾਰਾ_Map<T_ਮੁੱਲ>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_ਰੁੱਖ final {
    // STATE
    Cfg_ਖੰਡ_Map<ਅਚਲ Cfg_ਖੰਡ*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ਅਚਲ Cfg_ਖੰਡ* intersect(ਅਚਲ Cfg_ਖੰਡ* ap, ਅਚਲ Cfg_ਖੰਡ* bp);

ਜਨਤਕ:
    // CONSTRUCTOR
    explicit Cfg_Dominator_ਰੁੱਖ(ਅਚਲ Cfg_ਗ੍ਰਾਫ& cfg);
    // Can create an empty map
    Cfg_Dominator_ਰੁੱਖ() = ਮੂਲ;
    // Copyable, movable
    Cfg_Dominator_ਰੁੱਖ(ਅਚਲ Cfg_Dominator_ਰੁੱਖ&) = ਮੂਲ;
    Cfg_Dominator_ਰੁੱਖ(Cfg_Dominator_ਰੁੱਖ&&) = ਮੂਲ;
    Cfg_Dominator_ਰੁੱਖ& ਸੰਚਾਲਕ=(ਅਚਲ Cfg_Dominator_ਰੁੱਖ&) = ਮੂਲ;
    Cfg_Dominator_ਰੁੱਖ& ਸੰਚਾਲਕ=(Cfg_Dominator_ਰੁੱਖ&&) = ਮੂਲ;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ਅਚਲ Cfg_ਖੰਡ* closestCommonDominator(ਅਚਲ Cfg_ਖੰਡ* ap, ਅਚਲ Cfg_ਖੰਡ* bp) ਅਚਲ {
        ਜਦੋਂ (ap != bp) {
            ਜੇ (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } ਨਹੀਂਤਾਂ {
                ap = m_bb2Idom[ap];
            }
        }
        ਵਾਪਸ ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ਅਚਲ Cfg_ਖੰਡ* ਅਚਲ ap, ਅਚਲ Cfg_ਖੰਡ* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        ਜਦੋਂ (true) {
            // True if 'ap' is above (or same as) 'bp'
            ਜੇ (ap == bp) ਵਾਪਸ true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            ਜੇ (!bp) ਵਾਪਸ false;
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
std::unique_ptr<std::ਸਦਿਸ਼<AstVarScope*>> ਜੀਵਤ_Var_Scopes(ਅਚਲ Cfg_ਗ੍ਰਾਫ&);

}  //namespace V3Cfg

#ਜੇ_ਅੰਤ  // VERILATOR_V3CFG_H_
