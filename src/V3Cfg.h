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

#اگر_تعریف_نشده VERILATOR_V3CFG_H_
#تعریف_کن VERILATOR_V3CFG_H_

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
class Cfg_بلوک;
class Cfg_یال;

قالب <typename T_کلید, typename T_مقدار>
class CfgMap;
قالب <typename T_مقدار>
class Cfg_بلوک_Map;
قالب <typename T_مقدار>
class Cfg_یال_Map;

//######################################################################
// CfgBlock - A basic block (verticies of the control flow graph)

class Cfg_بلوک final : عام V3_گراف_رأس {
    دوست class Cfg_گراف;
    قالب <typename T_کلید, typename T_مقدار>
    دوست class CfgMap;
    دوست class CfgBuilder;

    // STATE
    Cfg_گراف* ثابت m_cfgp;  // The control flow graph this CfgBlock is under
    اندازه_t m_rpo_عدد = 0;  // Reverse post-order number and unique ID of this CfgBlock

    // V3GraphEdge::user() is set to the unique id by CfgBuilder
    std::وکتور<Ast_گره_Stmt*> m_stmtps;  // statements in this CfgBlock

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    مضمن explicit Cfg_بلوک(Cfg_گراف* cfgp);
    ~Cfg_بلوک() override = افتراضي;

عام:
    // PUBLIC METHODS

    // ID (reverse post-order number) of this block
    مضمن اندازه_t id();
    مضمن اندازه_t id() ثابت;

    // Is this the entry block of the CFG?
    bool isEnter() ثابت { أرجع درون_خالی(); }
    // Is this the exit block of the CFG?
    bool isExit() ثابت { أرجع بیرون_خالی(); }
    // Is this a branching block (multiple successors)?
    bool isBranch() ثابت { أرجع بیرون_یالها().has_مضرب_Elements(); }
    // Is this a control flow convergence block (multiple predecessors)?
    bool isJoin() ثابت { أرجع درون_یالها().has_مضرب_Elements(); }
    // Is this a join of exactly 2 paths?
    bool isTwoWayJoin() ثابت { أرجع درون_یالها().hasTwoElements(); }

    // The edge going to the taken (or uncinditional) successor, or nullptr if exit block
    مضمن Cfg_یال* گرفته_Edgep();
    مضمن ثابت Cfg_یال* گرفته_Edgep() ثابت;
    // The edge going to the untaken successor, or nullptr if not a branch, or exit block
    مضمن Cfg_یال* untknEdgep();
    مضمن ثابت Cfg_یال* untknEdgep() ثابت;
    // The taken successor block, or nullptr if exit block
    مضمن Cfg_بلوک* takenp();
    مضمن ثابت Cfg_بلوک* takenp() ثابت;
    // The untakens successor block, or nullptr if not a branch, or exit block
    مضمن Cfg_بلوک* untknp();
    مضمن ثابت Cfg_بلوک* untknp() ثابت;

    // The first predecessor edge, or nullptr if enter block
    مضمن Cfg_یال* اولی_Predecessor_Edgep();
    مضمن ثابت Cfg_یال* اولی_Predecessor_Edgep() ثابت;
    // The last predecessor edge, or nullptr if enter block
    مضمن Cfg_یال* آخرین_Predecessor_Edgep();
    مضمن ثابت Cfg_یال* آخرین_Predecessor_Edgep() ثابت;
    // The first predecessor block, or nullptr if enter block
    مضمن Cfg_بلوک* اولی_Predecessorp();
    مضمن ثابت Cfg_بلوک* اولی_Predecessorp() ثابت;
    // The last predecessor block, or nullptr if enter block
    مضمن Cfg_بلوک* آخرین_Predecessorp();
    مضمن ثابت Cfg_بلوک* آخرین_Predecessorp() ثابت;

    // Statements in this CfgBlock
    ثابت std::وکتور<Ast_گره_Stmt*>& stmtps() ثابت { أرجع m_stmtps; }

    // Ordering is done on reverese post-order numbering. For loop-free graph
    // this ensures that a block that compares less than another is not a
    // successor of the other block (it is an ancestor, or sibling).
    bool عملگر<(ثابت Cfg_بلوک& that) ثابت { أرجع id() < that.id(); }
    bool عملگر>(ثابت Cfg_بلوک& that) ثابت { أرجع id() > that.id(); }
    bool عملگر==(ثابت Cfg_بلوک& that) ثابت { أرجع this == &that; }

    // Iterators
    فراغ forEachSuccessor(std::function<فراغ(ثابت Cfg_بلوک&)> f) ثابت {
        لكل (ثابت V3_گراف_یال& یال : بیرون_یالها()) f(*static_cast<Cfg_بلوک*>(یال.بالاترین()));
    }
    فراغ forEachPredecessor(std::function<فراغ(ثابت Cfg_بلوک&)> f) ثابت {
        لكل (ثابت V3_گراف_یال& یال : درون_یالها()) f(*static_cast<Cfg_بلوک*>(یال.fromp()));
    }

    // Source location for debugging
    File_سطر* fileline() ثابت override {
        أرجع !m_stmtps.خالی() ? m_stmtps.front()->fileline() : nullptr;
    }

    // For Graphviz dumps only
    std::string name() ثابت override;
    std::string dotShape() ثابت override;
    std::string dotRank() ثابت override;
};

//######################################################################
// CfgEdge - An edges of the control flow graph

class Cfg_یال final : عام V3_گراف_یال {
    دوست class Cfg_گراف;
    قالب <typename T_کلید, typename T_مقدار>
    دوست class CfgMap;

    // STATE - Immutable after construction, set by CfgBuilder
    Cfg_گراف* ثابت m_cfgp;  // The control flow graph this CfgEdge is under
    اندازه_t m_id = 0;  // Unique ID of this vertex

    // PRIVATE METHODS

    // CONSTRUCTOR/DESTRUCTOR - via CfgGraph only
    مضمن Cfg_یال(Cfg_گراف* graphp, Cfg_بلوک* srcp, Cfg_بلوک* dstp);
    ~Cfg_یال() override = افتراضي;

عام:
    // METHODS

    // Unique ID of this CfgEdge - no particular meaning
    مضمن اندازه_t id();
    مضمن اندازه_t id() ثابت;

    // Source/destination CfgBlock
    ثابت Cfg_بلوک* srcp() ثابت { أرجع static_cast<ثابت Cfg_بلوک*>(fromp()); }
    ثابت Cfg_بلوک* dstp() ثابت { أرجع static_cast<ثابت Cfg_بلوک*>(بالاترین()); }
    Cfg_بلوک* srcp() { أرجع static_cast<Cfg_بلوک*>(fromp()); }
    Cfg_بلوک* dstp() { أرجع static_cast<Cfg_بلوک*>(بالاترین()); }

    // For Graphviz dumps only
    std::string dotLabel() ثابت override;
};

//######################################################################
// CfgGraph - The control flow graph

class Cfg_گراف final : عام V3_گراف {
    دوست class Cfg_بلوک;
    دوست class Cfg_یال;
    قالب <typename T_کلید, typename T_مقدار>
    دوست class CfgMap;
    دوست class CfgBuilder;

    // STATE
    اندازه_t m_nEdits = 0;  // Edit count of this graph
    اندازه_t m_n_آخرین_Ordered = 0;  // Last edit count blocks were ordered
    Cfg_بلوک* m_enterp = nullptr;  // The singular entry vertex
    Cfg_بلوک* m_exitp = nullptr;  // The singular exit vertex
    اندازه_t m_n_بلوکها = 0;  // Number of CfgBlocks in this CfgGraph
    اندازه_t m_n_یالها = 0;  // Number of CfgEdges in this CfgGraph

    // PRIVATE METHODS

    // Compute reverse post-order enumeration of blocks, and sort them
    // accordingly. Assign blocks, and edge IDs. Invalidates all previous IDs.
    فراغ rpo_بلوکها();

    // Add a new CfgBlock to this graph
    Cfg_بلوک* جمع_کن_بلوک() {
        ++m_nEdits;
        ++m_n_بلوکها;
        أرجع new Cfg_بلوک{this};
    }

    // Add a new taken (or unconditional) CfgEdge to this CFG
    فراغ جمع_کن_گرفته_یال(Cfg_بلوک* srcp, Cfg_بلوک* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(!srcp->گرفته_Edgep(), srcp, "Taken edge should be added first");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_یالها;
        new Cfg_یال{this, srcp, dstp};
    }

    // Add a new untaken CfgEdge to this CFG
    فراغ جمع_کن_Untkn_یال(Cfg_بلوک* srcp, Cfg_بلوک* dstp) {
        UASSERT_OBJ(srcp->m_cfgp == this, srcp, "'srcp' is not in this graph");
        UASSERT_OBJ(dstp->m_cfgp == this, dstp, "'dstp' is not in this graph");
        UASSERT_OBJ(srcp->گرفته_Edgep(), srcp, "Untaken edge should be added second");
        UASSERT_OBJ(srcp->takenp() != dstp, srcp, "Untaken branch targets the same block");
        //
        UASSERT_OBJ(dstp != m_enterp, dstp, "Enter block cannot have a predecessor");
        UASSERT_OBJ(srcp != m_exitp, srcp, "Exit block cannot have a successor");
        ++m_nEdits;
        ++m_n_یالها;
        new Cfg_یال{this, srcp, dstp};
    }

    اندازه_t id_از(ثابت Cfg_بلوک* bbp) ثابت {
        UASSERT_OBJ(m_nEdits == m_n_آخرین_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        أرجع bbp->m_rpo_عدد;
    }

    اندازه_t id_از(ثابت Cfg_یال* edgep) ثابت {
        UASSERT_OBJ(m_nEdits == m_n_آخرین_Ordered, m_enterp, "Cfg was edited but not re-ordered");
        أرجع edgep->m_id;
    }

    // Implementation for insertTwoWayJoins
    Cfg_بلوک* getOrCreateTwoWayJoinFor(Cfg_بلوک* bbp);

    // CONSTRUCTOR - use CfgGraph::build, which might fail, so this can't be public
    Cfg_گراف() = افتراضي;

عام:
    ~Cfg_گراف() override = افتراضي;

    // STATIC FUNCTIONS

    // Build CFG for the given list of statements
    ساكن std::unique_ptr<Cfg_گراف> build(Ast_گره* stmtsp);

    // PUBLIC METHODS

    // Accessors
    ثابت Cfg_بلوک& enter() ثابت { أرجع *m_enterp; }
    ثابت Cfg_بلوک& خروج() ثابت { أرجع *m_exitp; }

    // Number of basic blocks in this graph
    اندازه_t n_بلوکها() ثابت { أرجع m_n_بلوکها; }
    // Number of control flow edges in this graph
    اندازه_t n_یالها() ثابت { أرجع m_n_یالها; }

    // Create a CfgBlock map for this graph
    قالب <typename T_مقدار>
    مضمن Cfg_بلوک_Map<T_مقدار> make_بلوک_Map() ثابت;
    // Create a CfgEdgeMap map for this graph
    قالب <typename T_مقدار>
    مضمن Cfg_یال_Map<T_مقدار> make_یال_Map() ثابت;

    // Returns true iff the graph contains a loop (back-edge)
    bool شامل_است_Loop() ثابت;

    //------------------------------------------------------------------
    // The following methods mutate this CFG and invalidate CfgBlock and
    // CfgEdge IDs and associated CfgBlockMap, CfgEdgeMap and other
    // query instances.

    // Remove empty blocks, combine sequential blocks. Keeps the enter/exit block, even if empty.
    فراغ minimize();

    // Insert empty blocks to fix critical edges (edges that have a source with
    // multiple successors, and a destination with multiple predecessors)
    فراغ break_Critical_یالها();

    bool درج_Two_Way_Joins();
};

//######################################################################
// CfgMap - Map from CfgBlock or CfgEdge to generic values

قالب <typename T_کلید, typename T_مقدار>
class CfgMap VL_NOT_FINAL {
    // As opposed to a std::map/std::unordered_map, all entries are value
    // initialized at construction, and the map is always dense. This can
    // be improved if necessary but is sufficient for our current purposes.

    ثابت Cfg_گراف* m_cfgp;  // The control flow graph this map is for
    اندازه_t m_created;  // Edit count of CFG this map was created at
    std::وکتور<T_مقدار> m_map;  // The map, stored as a vector

محمي:
    // CONSTRUCTOR
    explicit CfgMap(ثابت Cfg_گراف* cfgp, اندازه_t اندازه)
        : m_cfgp{cfgp}
        , m_created{cfgp->m_nEdits}
        , m_map{اندازه} {}

عام:
    // Can create an empty map
    CfgMap()
        : m_cfgp{nullptr}
        , m_created{0} {}

    // Copyable, movable
    CfgMap(ثابت CfgMap<T_کلید, T_مقدار>&) = افتراضي;
    CfgMap(CfgMap<T_کلید, T_مقدار>&&) = افتراضي;
    CfgMap<T_کلید, T_مقدار>& عملگر=(ثابت CfgMap<T_کلید, T_مقدار>&) = افتراضي;
    CfgMap<T_کلید, T_مقدار>& عملگر=(CfgMap<T_کلید, T_مقدار>&&) = افتراضي;

    T_مقدار& عملگر[](ثابت T_کلید& کلید) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == کلید.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        أرجع m_map.at(کلید.id());
    }
    ثابت T_مقدار& عملگر[](ثابت T_کلید& کلید) ثابت {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == کلید.m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        أرجع m_map.at(کلید.id());
    }
    T_مقدار& عملگر[](ثابت T_کلید* keyp) {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        أرجع m_map.at(keyp->id());
    }
    ثابت T_مقدار& عملگر[](ثابت T_کلید* keyp) ثابت {
        UASSERT_OBJ(m_created == m_cfgp->m_nEdits, m_cfgp->m_enterp, "Map is stale");
        UASSERT_OBJ(m_cfgp == keyp->m_cfgp, m_cfgp->m_enterp, "Key not in this CFG");
        أرجع m_map.at(keyp->id());
    }
};

قالب <typename T_مقدار>
class Cfg_بلوک_Map final : عام CfgMap<Cfg_بلوک, T_مقدار> {
    دوست class Cfg_گراف;
    // CONSTRUCTOR - Create one via CfgGraph::makeBlockMap
    explicit Cfg_بلوک_Map(ثابت Cfg_گراف* cfgp)
        : CfgMap<Cfg_بلوک, T_مقدار>{cfgp, cfgp->n_بلوکها()} {}

عام:
    // Can create an empty map
    Cfg_بلوک_Map() = افتراضي;
    // Copyable, movable
    Cfg_بلوک_Map(ثابت Cfg_بلوک_Map&) = افتراضي;
    Cfg_بلوک_Map(Cfg_بلوک_Map&&) = افتراضي;
    Cfg_بلوک_Map& عملگر=(ثابت Cfg_بلوک_Map&) = افتراضي;
    Cfg_بلوک_Map& عملگر=(Cfg_بلوک_Map&&) = افتراضي;
};

قالب <typename T_مقدار>
class Cfg_یال_Map final : عام CfgMap<Cfg_یال, T_مقدار> {
    دوست class Cfg_گراف;
    // CONSTRUCTOR - Create one via CfgGraph::makeEdgeMap
    explicit Cfg_یال_Map(ثابت Cfg_گراف* cfgp)
        : CfgMap<Cfg_یال, T_مقدار>{cfgp, cfgp->n_یالها()} {}

عام:
    // Can create an empty map
    Cfg_یال_Map() = افتراضي;
    // Copyable, movable
    Cfg_یال_Map(ثابت Cfg_یال_Map&) = افتراضي;
    Cfg_یال_Map(Cfg_یال_Map&&) = افتراضي;
    Cfg_یال_Map& عملگر=(ثابت Cfg_یال_Map&) = افتراضي;
    Cfg_یال_Map& عملگر=(Cfg_یال_Map&&) = افتراضي;
};

//######################################################################
// Innline method definitions

// --- CfgBlock ---

Cfg_بلوک::Cfg_بلوک(Cfg_گراف* cfgp)
    : V3_گراف_رأس{cfgp}
    , m_cfgp{cfgp} {}

اندازه_t Cfg_بلوک::id() { أرجع m_cfgp->id_از(this); }
اندازه_t Cfg_بلوک::id() ثابت { أرجع m_cfgp->id_از(this); }

// Successor edges
Cfg_یال* Cfg_بلوک::گرفته_Edgep() {  // It's always the first edge
    أرجع isExit() ? nullptr : static_cast<Cfg_یال*>(بیرون_یالها().frontp());
}
ثابت Cfg_یال* Cfg_بلوک::گرفته_Edgep() ثابت {  // It's always the first edge
    أرجع isExit() ? nullptr : static_cast<ثابت Cfg_یال*>(بیرون_یالها().frontp());
}
Cfg_یال* Cfg_بلوک::untknEdgep() {  // It's always the second (last) edge
    أرجع isBranch() ? static_cast<Cfg_یال*>(بیرون_یالها().backp()) : nullptr;
}
ثابت Cfg_یال* Cfg_بلوک::untknEdgep() ثابت {  // It's always the second (last) edge
    أرجع isBranch() ? static_cast<ثابت Cfg_یال*>(بیرون_یالها().backp()) : nullptr;
}
Cfg_بلوک* Cfg_بلوک::takenp() {
    أرجع isExit() ? nullptr : static_cast<Cfg_بلوک*>(بیرون_یالها().frontp()->بالاترین());
}
ثابت Cfg_بلوک* Cfg_بلوک::takenp() ثابت {
    أرجع isExit() ? nullptr : static_cast<Cfg_بلوک*>(بیرون_یالها().frontp()->بالاترین());
}
Cfg_بلوک* Cfg_بلوک::untknp() {
    أرجع isBranch() ? static_cast<Cfg_بلوک*>(بیرون_یالها().backp()->بالاترین()) : nullptr;
}
ثابت Cfg_بلوک* Cfg_بلوک::untknp() ثابت {
    أرجع isBranch() ? static_cast<ثابت Cfg_بلوک*>(بیرون_یالها().backp()->بالاترین()) : nullptr;
}

// Predecessor edges
Cfg_یال* Cfg_بلوک::اولی_Predecessor_Edgep() {  //
    أرجع static_cast<Cfg_یال*>(درون_یالها().frontp());
}
ثابت Cfg_یال* Cfg_بلوک::اولی_Predecessor_Edgep() ثابت {  //
    أرجع static_cast<ثابت Cfg_یال*>(درون_یالها().frontp());
}
Cfg_یال* Cfg_بلوک::آخرین_Predecessor_Edgep() {  //
    أرجع static_cast<Cfg_یال*>(درون_یالها().backp());
}
ثابت Cfg_یال* Cfg_بلوک::آخرین_Predecessor_Edgep() ثابت {  //
    أرجع static_cast<ثابت Cfg_یال*>(درون_یالها().backp());
}
Cfg_بلوک* Cfg_بلوک::اولی_Predecessorp() {  //
    أرجع isEnter() ? nullptr : اولی_Predecessor_Edgep()->srcp();
}
ثابت Cfg_بلوک* Cfg_بلوک::اولی_Predecessorp() ثابت {  //
    أرجع isEnter() ? nullptr : اولی_Predecessor_Edgep()->srcp();
}
Cfg_بلوک* Cfg_بلوک::آخرین_Predecessorp() {  //
    أرجع isEnter() ? nullptr : آخرین_Predecessor_Edgep()->srcp();
}
ثابت Cfg_بلوک* Cfg_بلوک::آخرین_Predecessorp() ثابت {  //
    أرجع isEnter() ? nullptr : آخرین_Predecessor_Edgep()->srcp();
}

// --- CfgEdge ---

Cfg_یال::Cfg_یال(Cfg_گراف* cfgp, Cfg_بلوک* srcp, Cfg_بلوک* dstp)
    : V3_گراف_یال{cfgp, srcp, dstp, 1, false}
    , m_cfgp{cfgp} {}

اندازه_t Cfg_یال::id() { أرجع m_cfgp->id_از(this); }
اندازه_t Cfg_یال::id() ثابت { أرجع m_cfgp->id_از(this); }

// --- CfgGraph ---

قالب <typename T_مقدار>
Cfg_بلوک_Map<T_مقدار> Cfg_گراف::make_بلوک_Map() ثابت {
    أرجع Cfg_بلوک_Map<T_مقدار>{this};
}
قالب <typename T_مقدار>
Cfg_یال_Map<T_مقدار> Cfg_گراف::make_یال_Map() ثابت {
    أرجع Cfg_یال_Map<T_مقدار>{this};
}

//######################################################################
// CfgDominatorTree

class Cfg_Dominator_درخت final {
    // STATE
    Cfg_بلوک_Map<ثابت Cfg_بلوک*> m_bb2Idom;  // Map from CfgBlock to its immediate dominator

    // PRIVATE METHODS

    // Part of algorithm to compute m_bb2Idom, see consructor
    ثابت Cfg_بلوک* intersect(ثابت Cfg_بلوک* ap, ثابت Cfg_بلوک* bp);

عام:
    // CONSTRUCTOR
    explicit Cfg_Dominator_درخت(ثابت Cfg_گراف& cfg);
    // Can create an empty map
    Cfg_Dominator_درخت() = افتراضي;
    // Copyable, movable
    Cfg_Dominator_درخت(ثابت Cfg_Dominator_درخت&) = افتراضي;
    Cfg_Dominator_درخت(Cfg_Dominator_درخت&&) = افتراضي;
    Cfg_Dominator_درخت& عملگر=(ثابت Cfg_Dominator_درخت&) = افتراضي;
    Cfg_Dominator_درخت& عملگر=(Cfg_Dominator_درخت&&) = افتراضي;

    // PUBLIC METHODS

    // Return unique CfgBlock that dominates both of the given blocks, but does
    // not strictly dominate any other block that dominates both blocks. It
    // will return 'ap' or 'bp' if one dominates the other (or are the same)
    ثابت Cfg_بلوک* closestCommonDominator(ثابت Cfg_بلوک* ap, ثابت Cfg_بلوک* bp) ثابت {
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
    bool dominates(ثابت Cfg_بلوک* ثابت ap, ثابت Cfg_بلوک* bp) {
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
std::unique_ptr<std::وکتور<AstVarScope*>> زنده_Var_Scopes(ثابت Cfg_گراف&);

}  //namespace V3Cfg

#پایان_اگر  // VERILATOR_V3CFG_H_
