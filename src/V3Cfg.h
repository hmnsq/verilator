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

#اگر_غیر_تعریف_شدہ VERILATOR_V3CFG_H_
#تعریف_کرو VERILATOR_V3CFG_H_

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

class Cfg_گراف;
class Cfg_بلاک;
class Cfg_کنارہ;

سانچہ <typename T_کلید, typename T_قدر>
class CfgMap;
سانچہ <typename T_قدر>
class Cfg_بلاک_Map;
سانچہ <typename T_قدر>
class Cfg_کنارہ_Map;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class Cfg_بلاک final : عوامی V3_گراف_راس {
    دوست class Cfg_گراف;
    سانچہ <typename T_کلید, typename T_قدر>
    دوست class CfgMap;
    دوست class CfgBuilder;

    // STATE
    Cfg_گراف* ثابت m_cfgp;  // The control flow graph this CfgBlock is under
    سائز_t m_rpo_نمبر = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::سمتیہ<Ast_گرہ_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    اندرونی explicit Cfg_بلاک(Cfg_گراف* cfgp);
    ~Cfg_بلاک() override = طےشدہ;

عوامی:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    اندرونی سائز_t id();
    اندرونی سائز_t id() ثابت;

    // Is this the entry block of the CFG?
    bool isEnter() ثابت { واپس اندر_خالی_خانے(); }
    // Is this the exit block of the CFG?
    bool isExit() ثابت { واپس برآمد_خالی_خانے(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ثابت { واپس برآمد_کنارے().has_مضرب_Elements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ثابت { واپس اندر_کنارے().has_مضرب_Elements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ثابت { واپس اندر_کنارے().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    اندرونی Cfg_کنارہ* لیا_گیا_Edgep();
    اندرونی ثابت Cfg_کنارہ* لیا_گیا_Edgep() ثابت;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    اندرونی Cfg_کنارہ* untknEdgep();
    اندرونی ثابت Cfg_کنارہ* untknEdgep() ثابت;
    // The taken successor block, or nullptr if exit block
    اندرونی Cfg_بلاک* takenp();
    اندرونی ثابت Cfg_بلاک* takenp() ثابت;
    // The untakens successor block, or nullptr if not a branch, or exit block
    اندرونی Cfg_بلاک* untknp();
    اندرونی ثابت Cfg_بلاک* untknp() ثابت;

    // The first predecessor edge, or nullptr if enter block
    اندرونی Cfg_کنارہ* پہلا_Predecessor_Edgep();
    اندرونی ثابت Cfg_کنارہ* پہلا_Predecessor_Edgep() ثابت;
    // The last predecessor edge, or nullptr if enter block
    اندرونی Cfg_کنارہ* آخری_Predecessor_Edgep();
    اندرونی ثابت Cfg_کنارہ* آخری_Predecessor_Edgep() ثابت;
    // The first predecessor block, or nullptr if enter block
    اندرونی Cfg_بلاک* پہلا_Predecessorp();
    اندرونی ثابت Cfg_بلاک* پہلا_Predecessorp() ثابت;
    // The last predecessor block, or nullptr if enter block
    اندرونی Cfg_بلاک* آخری_Predecessorp();
    اندرونی ثابت Cfg_بلاک* آخری_Predecessorp() ثابت;

    // Statements in this CfgBlock
    ثابت std::سمتیہ<Ast_گرہ_Stmt*>& stmtps() ثابت { واپس m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool عامل<(ثابت Cfg_بلاک& that) ثابت { واپس id() < that.id(); }
    bool عامل>(ثابت Cfg_بلاک& that) ثابت { واپس id() > that.id(); }
    bool عامل==(ثابت Cfg_بلاک& that) ثابت { واپس this == &that; }

    // Iterators
    خالی forEachSuccessor(std::function<خالی(ثابت Cfg_بلاک&)> f) ثابت {
        برائے (ثابت V3_گراف_کنارہ& کنارہ : برآمد_کنارے()) f(*static_cast<Cfg_بلاک*>(کنارہ.چوٹی()));
    }
    خالی forEachPredecessor(std::function<خالی(ثابت Cfg_بلاک&)> f) ثابت {
        برائے (ثابت V3_گراف_کنارہ& کنارہ : اندر_کنارے()) f(*static_cast<Cfg_بلاک*>(کنارہ.fromp()));
    }

    // Source location for debugging
    File_سطر* fileline() ثابت override {
        واپس !m_stmtps.خالی_خانے() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ثابت override;
    std::string dotShape() ثابت override;
    std::string dotRank() ثابت override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class Cfg_کنارہ final : عوامی V3_گراف_کنارہ {
    دوست class Cfg_گراف;
    سانچہ <typename T_کلید, typename T_قدر>
    دوست class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    Cfg_گراف* ثابت m_cfgp;  // The control flow graph this CfgEdge is under
    سائز_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    اندرونی Cfg_کنارہ(Cfg_گراف* graphp, Cfg_بلاک* srcp, Cfg_بلاک* dstp);
    ~Cfg_کنارہ() override = طےشدہ;

عوامی:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    اندرونی سائز_t id();
    اندرونی سائز_t id() ثابت;

    // Source/destination CfgBlock
    ثابت Cfg_بلاک* srcp() ثابت { واپس static_cast<ثابت Cfg_بلاک*>(fromp()); }
    ثابت Cfg_بلاک* dstp() ثابت { واپس static_cast<ثابت Cfg_بلاک*>(چوٹی()); }
    Cfg_بلاک* srcp() { واپس static_cast<Cfg_بلاک*>(fromp()); }
    Cfg_بلاک* dstp() { واپس static_cast<Cfg_بلاک*>(چوٹی()); }

    // For Graphviz dumps only
    std::string dotLabel() ثابت override;
};

//######################################################################
// CfgGraph - The control flow graph

class Cfg_گراف final : عوامی V3_گراف {
    دوست class Cfg_بلاک;
    دوست class Cfg_کنارہ;
    سانچہ <typename T_کلید, typename T_قدر>
    دوست class CfgMap;
    دوست class CfgBuilder;

    // STATE
    سائز_t m_nEdits = 0;  // Edit count of this graph
    سائز_t m_n_آخری_Ordered = 0;  // Last edit count blocks were ordered
    Cfg_بلاک* m_enterp = nullptr;  // The singular entry vertex
    Cfg_بلاک* m_exitp = nullptr;  // The singular exit vertex
    سائز_t m_n_بلاکیں = 0;  // Number of CfgBlocks in this CfgGraph
    سائز_t m_n_کنارے = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    خالی rpo_بلاکیں();

    // Add a new CfgBlock to this graph
    Cfg_بلاک* جوڑو_بلاک() {
        ++m_nEdits;
        ++m_n_بلاکیں;
        واپس new Cfg_بلاک{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    خالی جوڑو_لیا_گیا_کنارہ(Cfg_بلاک* srcp, Cfg_بلاک* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(!srcp->لیا_گیا_Edgep(), srcp, "Taken edge should be added first");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_کنارے;
        new Cfg_کنارہ{this, srcp, dstp};
    }

    // Add a new untaken CfgEdge to this CFG
    خالی جوڑو_Untkn_کنارہ(Cfg_بلاک* srcp, Cfg_بلاک* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(srcp->لیا_گیا_Edgep(), srcp, "Untaken edge should be added second");
        UASSERT_OBJ(srcp->takenp() != dstp, srcp, "Untaken branch targets the same block");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_کنارے;
        new Cfg_کنارہ{this, srcp, dstp};
    }

    سائز_t id_کا(ثابت Cfg_بلاک* bbp) ثابت {
        UASSERT_OBJ(m_nEdits == m_n_آخری_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        واپس bbp->m_rpo_نمبر;
    }

    سائز_t id_کا(ثابت Cfg_کنارہ* edgep) ثابت {
        UASSERT_OBJ(m_nEdits == m_n_آخری_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        واپس edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    Cfg_بلاک* getOrCreateTwoWayJoinFor(Cfg_بلاک* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    Cfg_گراف() = طےشدہ;

عوامی:
    ~Cfg_گراف() override = طےشدہ;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    ساکن std::unique_ptr<Cfg_گراف> build(Ast_گرہ* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ثابت Cfg_بلاک& enter() ثابت { واپس *m_enterp; }
    ثابت Cfg_بلاک& باہر_نکلو() ثابت { واپس *m_exitp; }

    // Number of basic blocks in this graph
    سائز_t n_بلاکیں() ثابت { واپس m_n_بلاکیں; }
    // Number of control flow edges in this graph
    سائز_t n_کنارے() ثابت { واپس m_n_کنارے; }

    // Create a CfgBlock map for this graph
    سانچہ <typename T_قدر>
    اندرونی Cfg_بلاک_Map<T_قدر> make_بلاک_Map() ثابت;
    // Create a CfgEdgeMap map for this graph
    سانچہ <typename T_قدر>
    اندرونی Cfg_کنارہ_Map<T_قدر> make_کنارہ_Map() ثابت;

    // Returns true iff the graph contains a loop (back-edge)
    bool شامل_ہے_Loop() ثابت;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    خالی minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    خالی break_Critical_کنارے();

    bool داخل_کرو_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

سانچہ <typename T_کلید, typename T_قدر>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ثابت Cfg_گراف* m_cfgp;  // The control flow graph this map is for
    سائز_t m_created;  // Edit count of CFG this map was created at
    std::سمتیہ<T_قدر> m_map;  // The map, stored as a vector

محفوظ:
    // CONSTRUCTOR
    explicit CfgMap(ثابت Cfg_گراف* cfgp, سائز_t سائز)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{سائز} {}

عوامی:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ثابت CfgMap<T_کلید, T_قدر>&) = طےشدہ;
    CfgMap(CfgMap<T_کلید, T_قدر>&&) = طےشدہ;
    CfgMap<T_کلید, T_قدر>& عامل=(ثابت CfgMap<T_کلید, T_قدر>&) = طےشدہ;
    CfgMap<T_کلید, T_قدر>& عامل=(CfgMap<T_کلید, T_قدر>&&) = طےشدہ;

    T_قدر& عامل[](ثابت T_کلید& کلید) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == کلید.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        واپس m_map.at(کلید.id());
    }
    ثابت T_قدر& عامل[](ثابت T_کلید& کلید) ثابت {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == کلید.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        واپس m_map.at(کلید.id());
    }
    T_قدر& عامل[](ثابت T_کلید* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        واپس m_map.at(keyp->id());
    }
    ثابت T_قدر& عامل[](ثابت T_کلید* keyp) ثابت {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        واپس m_map.at(keyp->id());
    }
};

سانچہ <typename T_قدر>
class Cfg_بلاک_Map final : عوامی CfgMap<Cfg_بلاک, T_قدر> {
    دوست class Cfg_گراف;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit Cfg_بلاک_Map(ثابت Cfg_گراف* cfgp)
        : CfgMap<Cfg_بلاک, T_قدر>{cfgp, cfgp->n_بلاکیں()} {}

عوامی:
    // Can create an empty map
    Cfg_بلاک_Map() = طےشدہ;
    // Copyable, movable
    Cfg_بلاک_Map(ثابت Cfg_بلاک_Map&) = طےشدہ;
    Cfg_بلاک_Map(Cfg_بلاک_Map&&) = طےشدہ;
    Cfg_بلاک_Map& عامل=(ثابت Cfg_بلاک_Map&) = طےشدہ;
    Cfg_بلاک_Map& عامل=(Cfg_بلاک_Map&&) = طےشدہ;
};

سانچہ <typename T_قدر>
class Cfg_کنارہ_Map final : عوامی CfgMap<Cfg_کنارہ, T_قدر> {
    دوست class Cfg_گراف;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit Cfg_کنارہ_Map(ثابت Cfg_گراف* cfgp)
        : CfgMap<Cfg_کنارہ, T_قدر>{cfgp, cfgp->n_کنارے()} {}

عوامی:
    // Can create an empty map
    Cfg_کنارہ_Map() = طےشدہ;
    // Copyable, movable
    Cfg_کنارہ_Map(ثابت Cfg_کنارہ_Map&) = طےشدہ;
    Cfg_کنارہ_Map(Cfg_کنارہ_Map&&) = طےشدہ;
    Cfg_کنارہ_Map& عامل=(ثابت Cfg_کنارہ_Map&) = طےشدہ;
    Cfg_کنارہ_Map& عامل=(Cfg_کنارہ_Map&&) = طےشدہ;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

Cfg_بلاک::Cfg_بلاک(Cfg_گراف* cfgp)
    : V3_گراف_راس{cfgp}
    , m_cfgp{cfgp} {}

سائز_t Cfg_بلاک::id() { واپس m_cfgp->id_کا(this); }
سائز_t Cfg_بلاک::id() ثابت { واپس m_cfgp->id_کا(this); }

// Successor edges
Cfg_کنارہ* Cfg_بلاک::لیا_گیا_Edgep() {  // It's always the first edge
    واپس isExit() ? nullptr : static_cast<Cfg_کنارہ*>(برآمد_کنارے().frontp());
}
ثابت Cfg_کنارہ* Cfg_بلاک::لیا_گیا_Edgep() ثابت {  // It's always the first edge
    واپس isExit() ? nullptr : static_cast<ثابت Cfg_کنارہ*>(برآمد_کنارے().frontp());
}
Cfg_کنارہ* Cfg_بلاک::untknEdgep() {  // It's always the second (last) edge
    واپس isBranch() ? static_cast<Cfg_کنارہ*>(برآمد_کنارے().backp()) : nullptr;
}
ثابت Cfg_کنارہ* Cfg_بلاک::untknEdgep() ثابت {  // It's always the second (last) edge
    واپس isBranch() ? static_cast<ثابت Cfg_کنارہ*>(برآمد_کنارے().backp()) : nullptr;
}
Cfg_بلاک* Cfg_بلاک::takenp() {
    واپس isExit() ? nullptr : static_cast<Cfg_بلاک*>(برآمد_کنارے().frontp()->چوٹی());
}
ثابت Cfg_بلاک* Cfg_بلاک::takenp() ثابت {
    واپس isExit() ? nullptr : static_cast<Cfg_بلاک*>(برآمد_کنارے().frontp()->چوٹی());
}
Cfg_بلاک* Cfg_بلاک::untknp() {
    واپس isBranch() ? static_cast<Cfg_بلاک*>(برآمد_کنارے().backp()->چوٹی()) : nullptr;
}
ثابت Cfg_بلاک* Cfg_بلاک::untknp() ثابت {
    واپس isBranch() ? static_cast<ثابت Cfg_بلاک*>(برآمد_کنارے().backp()->چوٹی()) : nullptr;
}

// Predecessor edges
Cfg_کنارہ* Cfg_بلاک::پہلا_Predecessor_Edgep() {  //
    واپس static_cast<Cfg_کنارہ*>(اندر_کنارے().frontp());
}
ثابت Cfg_کنارہ* Cfg_بلاک::پہلا_Predecessor_Edgep() ثابت {  //
    واپس static_cast<ثابت Cfg_کنارہ*>(اندر_کنارے().frontp());
}
Cfg_کنارہ* Cfg_بلاک::آخری_Predecessor_Edgep() {  //
    واپس static_cast<Cfg_کنارہ*>(اندر_کنارے().backp());
}
ثابت Cfg_کنارہ* Cfg_بلاک::آخری_Predecessor_Edgep() ثابت {  //
    واپس static_cast<ثابت Cfg_کنارہ*>(اندر_کنارے().backp());
}
Cfg_بلاک* Cfg_بلاک::پہلا_Predecessorp() {  //
    واپس isEnter() ? nullptr : پہلا_Predecessor_Edgep()->srcp();
}
ثابت Cfg_بلاک* Cfg_بلاک::پہلا_Predecessorp() ثابت {  //
    واپس isEnter() ? nullptr : پہلا_Predecessor_Edgep()->srcp();
}
Cfg_بلاک* Cfg_بلاک::آخری_Predecessorp() {  //
    واپس isEnter() ? nullptr : آخری_Predecessor_Edgep()->srcp();
}
ثابت Cfg_بلاک* Cfg_بلاک::آخری_Predecessorp() ثابت {  //
    واپس isEnter() ? nullptr : آخری_Predecessor_Edgep()->srcp();
}

// --- CfgEdge ---

Cfg_کنارہ::Cfg_کنارہ(Cfg_گراف* cfgp, Cfg_بلاک* srcp, Cfg_بلاک* dstp)
    : V3_گراف_کنارہ{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

سائز_t Cfg_کنارہ::id() { واپس m_cfgp->id_کا(this); }
سائز_t Cfg_کنارہ::id() ثابت { واپس m_cfgp->id_کا(this); }

// --- CfgGraph ---

سانچہ <typename T_قدر>
Cfg_بلاک_Map<T_قدر> Cfg_گراف::make_بلاک_Map() ثابت {
    واپس Cfg_بلاک_Map<T_قدر>{this};
}
سانچہ <typename T_قدر>
Cfg_کنارہ_Map<T_قدر> Cfg_گراف::make_کنارہ_Map() ثابت {
    واپس Cfg_کنارہ_Map<T_قدر>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_درخت final {
    // STATE
    Cfg_بلاک_Map<ثابت Cfg_بلاک*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ثابت Cfg_بلاک* intersect(ثابت Cfg_بلاک* ap, ثابت Cfg_بلاک* bp);

عوامی:
    // CONSTRUCTOR
    explicit Cfg_Dominator_درخت(ثابت Cfg_گراف& cfg);
    // Can create an empty map
    Cfg_Dominator_درخت() = طےشدہ;
    // Copyable, movable
    Cfg_Dominator_درخت(ثابت Cfg_Dominator_درخت&) = طےشدہ;
    Cfg_Dominator_درخت(Cfg_Dominator_درخت&&) = طےشدہ;
    Cfg_Dominator_درخت& عامل=(ثابت Cfg_Dominator_درخت&) = طےشدہ;
    Cfg_Dominator_درخت& عامل=(Cfg_Dominator_درخت&&) = طےشدہ;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ثابت Cfg_بلاک* closestCommonDominator(ثابت Cfg_بلاک* ap, ثابت Cfg_بلاک* bp) ثابت {
        جب (ap != bp) {
            اگر (*ap < *bp) {
                bp = m_bb2Idom[bp];
            } ورنہ {
                ap = m_bb2Idom[ap];
            }
        }
        واپس ap;
    }

    // Returns true if 'ap' dominates 'bp'
    bool dominates(ثابت Cfg_بلاک* ثابت ap, ثابت Cfg_بلاک* bp) {
        // Walk up the dominator tree from 'bp' until reaching 'ap' or the root
        جب (true) {
            // True if 'ap' is above (or same as) 'bp'
            اگر (ap == bp) واپس true;
            // Step up the dominator tree
            bp = m_bb2Idom[bp];
            // False if reached the root
            اگر (!bp) واپس false;
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
std::unique_ptr<std::سمتیہ<AstVarScope*>> زندہ_Var_Scopes(ثابت Cfg_گراف&);

}  //namespace V3Cfg

#اگر_ختم  // VERILATOR_V3CFG_H_
