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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3CFG_H_
#وصف_ڪريو VERILATOR_V3CFG_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"

#ضمن <functional>
#ضمن <limits>
#ضمن <memory>
#ضمن <vector>

//######################################################################
// Control Flow Graph (CFG) and related data structures

class Cfg_گراف;
class Cfg_بلاڪ;
class Cfg_ڪنارو;

سانچو <typename T_ڪنجي, typename T_قدر>
class CfgMap;
سانچو <typename T_قدر>
class Cfg_بلاڪ_Map;
سانچو <typename T_قدر>
class Cfg_ڪنارو_Map;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class Cfg_بلاڪ final : عام V3_گراف_راس {
    دوست class Cfg_گراف;
    سانچو <typename T_ڪنجي, typename T_قدر>
    دوست class CfgMap;
    دوست class CfgBuilder;

    // STATE
    Cfg_گراف* ثابت m_cfgp;  // The control flow graph this CfgBlock is under
    سائيز_t m_rpo_عدد = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::ويڪٽر<Ast_ڳنڍ_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    مضمن explicit Cfg_بلاڪ(Cfg_گراف* cfgp);
    ~Cfg_بلاڪ() override = افتراضي;

عام:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    مضمن سائيز_t id();
    مضمن سائيز_t id() ثابت;

    // Is this the entry block of the CFG?
    bool isEnter() ثابت { أرجع اندريون_خالي_جايون(); }
    // Is this the exit block of the CFG?
    bool isExit() ثابت { أرجع ٻاهريون_خالي_جايون(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ثابت { أرجع ٻاهريون_ڪنارا().has_گڻو_Elements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ثابت { أرجع اندريون_ڪنارا().has_گڻو_Elements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ثابت { أرجع اندريون_ڪنارا().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    مضمن Cfg_ڪنارو* ورتل_Edgep();
    مضمن ثابت Cfg_ڪنارو* ورتل_Edgep() ثابت;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    مضمن Cfg_ڪنارو* untknEdgep();
    مضمن ثابت Cfg_ڪنارو* untknEdgep() ثابت;
    // The taken successor block, or nullptr if exit block
    مضمن Cfg_بلاڪ* takenp();
    مضمن ثابت Cfg_بلاڪ* takenp() ثابت;
    // The untakens successor block, or nullptr if not a branch, or exit block
    مضمن Cfg_بلاڪ* untknp();
    مضمن ثابت Cfg_بلاڪ* untknp() ثابت;

    // The first predecessor edge, or nullptr if enter block
    مضمن Cfg_ڪنارو* پهريون_Predecessor_Edgep();
    مضمن ثابت Cfg_ڪنارو* پهريون_Predecessor_Edgep() ثابت;
    // The last predecessor edge, or nullptr if enter block
    مضمن Cfg_ڪنارو* آخري_Predecessor_Edgep();
    مضمن ثابت Cfg_ڪنارو* آخري_Predecessor_Edgep() ثابت;
    // The first predecessor block, or nullptr if enter block
    مضمن Cfg_بلاڪ* پهريون_Predecessorp();
    مضمن ثابت Cfg_بلاڪ* پهريون_Predecessorp() ثابت;
    // The last predecessor block, or nullptr if enter block
    مضمن Cfg_بلاڪ* آخري_Predecessorp();
    مضمن ثابت Cfg_بلاڪ* آخري_Predecessorp() ثابت;

    // Statements in this CfgBlock
    ثابت std::ويڪٽر<Ast_ڳنڍ_Stmt*>& stmtps() ثابت { أرجع m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool عامل<(ثابت Cfg_بلاڪ& that) ثابت { أرجع id() < that.id(); }
    bool عامل>(ثابت Cfg_بلاڪ& that) ثابت { أرجع id() > that.id(); }
    bool عامل==(ثابت Cfg_بلاڪ& that) ثابت { أرجع this == &that; }

    // Iterators
    فراغ forEachSuccessor(std::function<فراغ(ثابت Cfg_بلاڪ&)> f) ثابت {
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : ٻاهريون_ڪنارا()) f(*static_cast<Cfg_بلاڪ*>(ڪنارو.چوٽي()));
    }
    فراغ forEachPredecessor(std::function<فراغ(ثابت Cfg_بلاڪ&)> f) ثابت {
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : اندريون_ڪنارا()) f(*static_cast<Cfg_بلاڪ*>(ڪنارو.fromp()));
    }

    // Source location for debugging
    File_سٽ* fileline() ثابت override {
        أرجع !m_stmtps.خالي_جايون() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ثابت override;
    std::string dotShape() ثابت override;
    std::string dotRank() ثابت override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class Cfg_ڪنارو final : عام V3_گراف_ڪنارو {
    دوست class Cfg_گراف;
    سانچو <typename T_ڪنجي, typename T_قدر>
    دوست class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    Cfg_گراف* ثابت m_cfgp;  // The control flow graph this CfgEdge is under
    سائيز_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    مضمن Cfg_ڪنارو(Cfg_گراف* graphp, Cfg_بلاڪ* srcp, Cfg_بلاڪ* dstp);
    ~Cfg_ڪنارو() override = افتراضي;

عام:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    مضمن سائيز_t id();
    مضمن سائيز_t id() ثابت;

    // Source/destination CfgBlock
    ثابت Cfg_بلاڪ* srcp() ثابت { أرجع static_cast<ثابت Cfg_بلاڪ*>(fromp()); }
    ثابت Cfg_بلاڪ* dstp() ثابت { أرجع static_cast<ثابت Cfg_بلاڪ*>(چوٽي()); }
    Cfg_بلاڪ* srcp() { أرجع static_cast<Cfg_بلاڪ*>(fromp()); }
    Cfg_بلاڪ* dstp() { أرجع static_cast<Cfg_بلاڪ*>(چوٽي()); }

    // For Graphviz dumps only
    std::string dotLabel() ثابت override;
};

//######################################################################
// CfgGraph - The control flow graph

class Cfg_گراف final : عام V3_گراف {
    دوست class Cfg_بلاڪ;
    دوست class Cfg_ڪنارو;
    سانچو <typename T_ڪنجي, typename T_قدر>
    دوست class CfgMap;
    دوست class CfgBuilder;

    // STATE
    سائيز_t m_nEdits = 0;  // Edit count of this graph
    سائيز_t m_n_آخري_Ordered = 0;  // Last edit count blocks were ordered
    Cfg_بلاڪ* m_enterp = nullptr;  // The singular entry vertex
    Cfg_بلاڪ* m_exitp = nullptr;  // The singular exit vertex
    سائيز_t m_n_بلاڪون = 0;  // Number of CfgBlocks in this CfgGraph
    سائيز_t m_n_ڪنارا = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    فراغ rpo_بلاڪون();

    // Add a new CfgBlock to this graph
    Cfg_بلاڪ* جوڙيو_بلاڪ() {
        ++m_nEdits;
        ++m_n_بلاڪون;
        أرجع new Cfg_بلاڪ{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    فراغ جوڙيو_ورتل_ڪنارو(Cfg_بلاڪ* srcp, Cfg_بلاڪ* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(!srcp->ورتل_Edgep(), srcp, "Taken edge should be added first");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_ڪنارا;
        new Cfg_ڪنارو{this, srcp, dstp};
    }

    // Add a new untaken CfgEdge to this CFG
    فراغ جوڙيو_Untkn_ڪنارو(Cfg_بلاڪ* srcp, Cfg_بلاڪ* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(srcp->ورتل_Edgep(), srcp, "Untaken edge should be added second");
        UASSERT_OBJ(srcp->takenp() != dstp, srcp, "Untaken branch targets the same block");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_ڪنارا;
        new Cfg_ڪنارو{this, srcp, dstp};
    }

    سائيز_t id_جو(ثابت Cfg_بلاڪ* bbp) ثابت {
        UASSERT_OBJ(m_nEdits == m_n_آخري_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        أرجع bbp->m_rpo_عدد;
    }

    سائيز_t id_جو(ثابت Cfg_ڪنارو* edgep) ثابت {
        UASSERT_OBJ(m_nEdits == m_n_آخري_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        أرجع edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    Cfg_بلاڪ* getOrCreateTwoWayJoinFor(Cfg_بلاڪ* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    Cfg_گراف() = افتراضي;

عام:
    ~Cfg_گراف() override = افتراضي;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    ساكن std::unique_ptr<Cfg_گراف> build(Ast_ڳنڍ* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ثابت Cfg_بلاڪ& enter() ثابت { أرجع *m_enterp; }
    ثابت Cfg_بلاڪ& ٻاهر_نڪرو() ثابت { أرجع *m_exitp; }

    // Number of basic blocks in this graph
    سائيز_t n_بلاڪون() ثابت { أرجع m_n_بلاڪون; }
    // Number of control flow edges in this graph
    سائيز_t n_ڪنارا() ثابت { أرجع m_n_ڪنارا; }

    // Create a CfgBlock map for this graph
    سانچو <typename T_قدر>
    مضمن Cfg_بلاڪ_Map<T_قدر> make_بلاڪ_Map() ثابت;
    // Create a CfgEdgeMap map for this graph
    سانچو <typename T_قدر>
    مضمن Cfg_ڪنارو_Map<T_قدر> make_ڪنارو_Map() ثابت;

    // Returns true iff the graph contains a loop (back-edge)
    bool شامل_آهي_Loop() ثابت;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    فراغ minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    فراغ break_Critical_ڪنارا();

    bool وجهو_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

سانچو <typename T_ڪنجي, typename T_قدر>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ثابت Cfg_گراف* m_cfgp;  // The control flow graph this map is for
    سائيز_t m_created;  // Edit count of CFG this map was created at
    std::ويڪٽر<T_قدر> m_map;  // The map, stored as a vector

محمي:
    // CONSTRUCTOR
    explicit CfgMap(ثابت Cfg_گراف* cfgp, سائيز_t سائيز)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{سائيز} {}

عام:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ثابت CfgMap<T_ڪنجي, T_قدر>&) = افتراضي;
    CfgMap(CfgMap<T_ڪنجي, T_قدر>&&) = افتراضي;
    CfgMap<T_ڪنجي, T_قدر>& عامل=(ثابت CfgMap<T_ڪنجي, T_قدر>&) = افتراضي;
    CfgMap<T_ڪنجي, T_قدر>& عامل=(CfgMap<T_ڪنجي, T_قدر>&&) = افتراضي;

    T_قدر& عامل[](ثابت T_ڪنجي& ڪنجي) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ڪنجي.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        أرجع m_map.at(ڪنجي.id());
    }
    ثابت T_قدر& عامل[](ثابت T_ڪنجي& ڪنجي) ثابت {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == ڪنجي.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        أرجع m_map.at(ڪنجي.id());
    }
    T_قدر& عامل[](ثابت T_ڪنجي* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        أرجع m_map.at(keyp->id());
    }
    ثابت T_قدر& عامل[](ثابت T_ڪنجي* keyp) ثابت {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        أرجع m_map.at(keyp->id());
    }
};

سانچو <typename T_قدر>
class Cfg_بلاڪ_Map final : عام CfgMap<Cfg_بلاڪ, T_قدر> {
    دوست class Cfg_گراف;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit Cfg_بلاڪ_Map(ثابت Cfg_گراف* cfgp)
        : CfgMap<Cfg_بلاڪ, T_قدر>{cfgp, cfgp->n_بلاڪون()} {}

عام:
    // Can create an empty map
    Cfg_بلاڪ_Map() = افتراضي;
    // Copyable, movable
    Cfg_بلاڪ_Map(ثابت Cfg_بلاڪ_Map&) = افتراضي;
    Cfg_بلاڪ_Map(Cfg_بلاڪ_Map&&) = افتراضي;
    Cfg_بلاڪ_Map& عامل=(ثابت Cfg_بلاڪ_Map&) = افتراضي;
    Cfg_بلاڪ_Map& عامل=(Cfg_بلاڪ_Map&&) = افتراضي;
};

سانچو <typename T_قدر>
class Cfg_ڪنارو_Map final : عام CfgMap<Cfg_ڪنارو, T_قدر> {
    دوست class Cfg_گراف;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit Cfg_ڪنارو_Map(ثابت Cfg_گراف* cfgp)
        : CfgMap<Cfg_ڪنارو, T_قدر>{cfgp, cfgp->n_ڪنارا()} {}

عام:
    // Can create an empty map
    Cfg_ڪنارو_Map() = افتراضي;
    // Copyable, movable
    Cfg_ڪنارو_Map(ثابت Cfg_ڪنارو_Map&) = افتراضي;
    Cfg_ڪنارو_Map(Cfg_ڪنارو_Map&&) = افتراضي;
    Cfg_ڪنارو_Map& عامل=(ثابت Cfg_ڪنارو_Map&) = افتراضي;
    Cfg_ڪنارو_Map& عامل=(Cfg_ڪنارو_Map&&) = افتراضي;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

Cfg_بلاڪ::Cfg_بلاڪ(Cfg_گراف* cfgp)
    : V3_گراف_راس{cfgp}
    , m_cfgp{cfgp} {}

سائيز_t Cfg_بلاڪ::id() { أرجع m_cfgp->id_جو(this); }
سائيز_t Cfg_بلاڪ::id() ثابت { أرجع m_cfgp->id_جو(this); }

// Successor edges
Cfg_ڪنارو* Cfg_بلاڪ::ورتل_Edgep() {  // It's always the first edge
    أرجع isExit() ? nullptr : static_cast<Cfg_ڪنارو*>(ٻاهريون_ڪنارا().frontp());
}
ثابت Cfg_ڪنارو* Cfg_بلاڪ::ورتل_Edgep() ثابت {  // It's always the first edge
    أرجع isExit() ? nullptr : static_cast<ثابت Cfg_ڪنارو*>(ٻاهريون_ڪنارا().frontp());
}
Cfg_ڪنارو* Cfg_بلاڪ::untknEdgep() {  // It's always the second (last) edge
    أرجع isBranch() ? static_cast<Cfg_ڪنارو*>(ٻاهريون_ڪنارا().backp()) : nullptr;
}
ثابت Cfg_ڪنارو* Cfg_بلاڪ::untknEdgep() ثابت {  // It's always the second (last) edge
    أرجع isBranch() ? static_cast<ثابت Cfg_ڪنارو*>(ٻاهريون_ڪنارا().backp()) : nullptr;
}
Cfg_بلاڪ* Cfg_بلاڪ::takenp() {
    أرجع isExit() ? nullptr : static_cast<Cfg_بلاڪ*>(ٻاهريون_ڪنارا().frontp()->چوٽي());
}
ثابت Cfg_بلاڪ* Cfg_بلاڪ::takenp() ثابت {
    أرجع isExit() ? nullptr : static_cast<Cfg_بلاڪ*>(ٻاهريون_ڪنارا().frontp()->چوٽي());
}
Cfg_بلاڪ* Cfg_بلاڪ::untknp() {
    أرجع isBranch() ? static_cast<Cfg_بلاڪ*>(ٻاهريون_ڪنارا().backp()->چوٽي()) : nullptr;
}
ثابت Cfg_بلاڪ* Cfg_بلاڪ::untknp() ثابت {
    أرجع isBranch() ? static_cast<ثابت Cfg_بلاڪ*>(ٻاهريون_ڪنارا().backp()->چوٽي()) : nullptr;
}

// Predecessor edges
Cfg_ڪنارو* Cfg_بلاڪ::پهريون_Predecessor_Edgep() {  //
    أرجع static_cast<Cfg_ڪنارو*>(اندريون_ڪنارا().frontp());
}
ثابت Cfg_ڪنارو* Cfg_بلاڪ::پهريون_Predecessor_Edgep() ثابت {  //
    أرجع static_cast<ثابت Cfg_ڪنارو*>(اندريون_ڪنارا().frontp());
}
Cfg_ڪنارو* Cfg_بلاڪ::آخري_Predecessor_Edgep() {  //
    أرجع static_cast<Cfg_ڪنارو*>(اندريون_ڪنارا().backp());
}
ثابت Cfg_ڪنارو* Cfg_بلاڪ::آخري_Predecessor_Edgep() ثابت {  //
    أرجع static_cast<ثابت Cfg_ڪنارو*>(اندريون_ڪنارا().backp());
}
Cfg_بلاڪ* Cfg_بلاڪ::پهريون_Predecessorp() {  //
    أرجع isEnter() ? nullptr : پهريون_Predecessor_Edgep()->srcp();
}
ثابت Cfg_بلاڪ* Cfg_بلاڪ::پهريون_Predecessorp() ثابت {  //
    أرجع isEnter() ? nullptr : پهريون_Predecessor_Edgep()->srcp();
}
Cfg_بلاڪ* Cfg_بلاڪ::آخري_Predecessorp() {  //
    أرجع isEnter() ? nullptr : آخري_Predecessor_Edgep()->srcp();
}
ثابت Cfg_بلاڪ* Cfg_بلاڪ::آخري_Predecessorp() ثابت {  //
    أرجع isEnter() ? nullptr : آخري_Predecessor_Edgep()->srcp();
}

// --- CfgEdge ---

Cfg_ڪنارو::Cfg_ڪنارو(Cfg_گراف* cfgp, Cfg_بلاڪ* srcp, Cfg_بلاڪ* dstp)
    : V3_گراف_ڪنارو{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

سائيز_t Cfg_ڪنارو::id() { أرجع m_cfgp->id_جو(this); }
سائيز_t Cfg_ڪنارو::id() ثابت { أرجع m_cfgp->id_جو(this); }

// --- CfgGraph ---

سانچو <typename T_قدر>
Cfg_بلاڪ_Map<T_قدر> Cfg_گراف::make_بلاڪ_Map() ثابت {
    أرجع Cfg_بلاڪ_Map<T_قدر>{this};
}
سانچو <typename T_قدر>
Cfg_ڪنارو_Map<T_قدر> Cfg_گراف::make_ڪنارو_Map() ثابت {
    أرجع Cfg_ڪنارو_Map<T_قدر>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_وڻ final {
    // STATE
    Cfg_بلاڪ_Map<ثابت Cfg_بلاڪ*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ثابت Cfg_بلاڪ* intersect(ثابت Cfg_بلاڪ* ap, ثابت Cfg_بلاڪ* bp);

عام:
    // CONSTRUCTOR
    explicit Cfg_Dominator_وڻ(ثابت Cfg_گراف& cfg);
    // Can create an empty map
    Cfg_Dominator_وڻ() = افتراضي;
    // Copyable, movable
    Cfg_Dominator_وڻ(ثابت Cfg_Dominator_وڻ&) = افتراضي;
    Cfg_Dominator_وڻ(Cfg_Dominator_وڻ&&) = افتراضي;
    Cfg_Dominator_وڻ& عامل=(ثابت Cfg_Dominator_وڻ&) = افتراضي;
    Cfg_Dominator_وڻ& عامل=(Cfg_Dominator_وڻ&&) = افتراضي;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ثابت Cfg_بلاڪ* closestCommonDominator(ثابت Cfg_بلاڪ* ap, ثابت Cfg_بلاڪ* bp) ثابت {
        طالما (ap != bp) {
            إذا (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } وإلا {
                ap = m_bb2Idom[ap];
            }
        }
        أرجع ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ثابت Cfg_بلاڪ* ثابت ap, ثابت Cfg_بلاڪ* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        طالما (true) {
            // True if 'ap' is above (or same as) 'bp'
            إذا (ap == bp) أرجع true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            إذا (!bp) أرجع false;
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
std::unique_ptr<std::ويڪٽر<AstVarScope*>> جيئرو_Var_Scopes(ثابت Cfg_گراف&);

}  //namespace V3Cfg

#جيڪڏهن_پورو  // VERILATOR_V3CFG_H_
