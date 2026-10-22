// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Ordering constraint graph
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
// OrderGraph is a bipartite graph, with the two parts being formed of only OrderLogicVertex and
// OrderVarVertex vertices respectively (i.e.: edges are always between OrderLogicVertex and
// OrderVarVertex, and never between two OrderLogicVertex or OrderVarVertex). The graph represents
// both fine-grained dependencies, and additional ordering constraints between logic blocks and
// variables. The fact that OrderGraph is bipartite is important and we take advantage of this fact
// in various algorithms, so this property must be maintained.
//
// Both OrderLogicVertex and OrderVarVertex derives from OrderEitherVertex, so OrderGraph is
// composed only of OrderEitherVertex vertices.
//
// OrderLogicVertex holds a 'logic block', which is just some computational construct that is
// ordered as a single unit. Ordering of these logic blocks is determined by the variables they
// read and write, which is represented by the edges between OrderLogicVertex and OrderVarVertex
// instances (and hence the graph is bipartite).
//
// OrderVarVertex is abstract, and has various concrete subtypes that represent various ordering
// constraints imposed by variables accessed by logic blocks. The concrete subtypes and their
// roles are:
//
// OrderVarStdVertex:   Data dependencies for combinational logic and delayed assignment
//                      updates (AlwaysPost).
// OrderVarPostVertex:  Ensures all sequential logic blocks reading a signal do so before any
//                      combinational or delayed assignments update that signal.
// OrderVarPordVertex:  Ensures a _d = _q AlwaysPre used to implement delayed (non-blocking)
//                      assignments is the first write of a _d, before any sequential blocks
//                      write to that _d.
// OrderVarPreVertex:   This is an optimization. Try to ensure that a _d = _q AlwaysPre is the
//                      last read of a _q, after all reads of that _q by sequential logic. The
//                      model is still correct if we cannot satisfy this due to other interfering
//                      constraints. If respecting this constraint is possible, then combined
//                      with the OrderVarPordVertex constraint we get that all writes to _d are
//                      after all reads of a _q, which then allows us to eliminate the _d
//                      completely and assign to the _q directly. This means these delayed
//                      assignments can be implemented without temporary storage (the redundant
//                      storage is eliminated in V3LifePost).
//
// Ordering constraints are represented by directed edges, where the source of an edge needs to be
// ordered before the sink of an edge. A constraint can be either hard (must be satisfied),
// represented by a non-cutable edge, or a constraint can be soft (ideally should be satisfied, but
// is ok not to if other hard constraints interfere), represented by a cutable edge. Edges
// otherwise carry no additional information. TODO: what about weight?
//
// Note: It is required for hard (non-cutable) constraints to form a DAG, but together with the
// soft constraints the graph can be arbitrary so long as it remains bipartite.
//
//*************************************************************************

#ifndef VERILATOR_V3ORDERGRAPH_H_
#define VERILATOR_V3ORDERGRAPH_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Sched.h"

#அந்தர்பாவ <set>
#அந்தர்பாவ <vector>

class OrderLogicVertex;
class OrderVarVertex;

//======================================================================

எண்ணிக்கை OrderWeights : uint8_t {
    WEIGHT_COMBO = 1,  // Breakable combo logic
    WEIGHT_POST = 2,  // Post-delayed used var
    WEIGHT_PRE = 3,  // Breakable pre-delayed used var
    WEIGHT_MEDIUM = 8,  // Medium weight just so dot graph looks nice
    WEIGHT_NORMAL = 32  // High weight just so dot graph looks nice
};

//======================================================================
// Graph type

// The sensitivity domain of a vertex: the set of trigger indices,
// any of which being set means the logic needs to be executed.
// Instances are interned in the OrderGraph, so can compare pointers.
class OrderDomain final {
    மாறிலி std::set<uint32_t> m_triggers;  // The trigger indices

பொது:
    explicit OrderDomain(மாறிலி std::set<uint32_t>& triggers)
        : m_triggers{triggers} {}

    மாறிலி std::set<uint32_t>& triggers() மாறிலி { திரும்பு m_triggers; }
    bool operator<(மாறிலி OrderDomain& other) மாறிலி { திரும்பு m_triggers < other.m_triggers; }
};

class OrderGraph final : பொது V3Graph {
    மாறிலி V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* மாறிலி m_trigVscp;  // The trigger vector for this graph
    std::set<OrderDomain> m_domains;  // The interned domains

பொது:
    // CONSTRUCTOR
    OrderGraph(மாறிலி V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() மாறிலி { திரும்பு m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    மாறிலி AstSenItem* trigSenItemp(uint32_t index) மாறிலி {
        திரும்பு m_trigKit.trigIdx2SenItem().at(index);
    }

    // Get the interned instance of the given domain
    மாறிலி OrderDomain* domainp(மாறிலி std::set<uint32_t>& triggers) {
        திரும்பு &*m_domains.emplace(triggers).first;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    உள்வரி வெறுமை addHardEdge(OrderLogicVertex* fromp, OrderVarVertex* உச்சி,
                            முழு weight) VL_MT_DISABLED;
    உள்வரி வெறுமை addHardEdge(OrderVarVertex* fromp, OrderLogicVertex* உச்சி,
                            முழு weight) VL_MT_DISABLED;
    உள்வரி வெறுமை addSoftEdge(OrderLogicVertex* fromp, OrderVarVertex* உச்சி,
                            முழு weight) VL_MT_DISABLED;
    உள்வரி வெறுமை addSoftEdge(OrderVarVertex* fromp, OrderLogicVertex* உச்சி,
                            முழு weight) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class OrderEitherVertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(OrderEitherVertex, V3GraphVertex)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    மாறிலி OrderDomain* m_domainp;

பாதுகாக்கப்பட்ட:
    // CONSTRUCTOR
    OrderEitherVertex(OrderGraph* graphp, மாறிலி OrderDomain* domainp) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_domainp{domainp} {}
    ~OrderEitherVertex() override = இயல்பு;

பொது:
    // METHODS
    மெய்நிகர் bool domainMatters() = 0;

    // ACCESSORS
    மாறிலி OrderDomain* domainp() மாறிலி VL_MT_STABLE { திரும்பு m_domainp; }
    வெறுமை domainp(மாறிலி OrderDomain* domainp) VL_MT_DISABLED {
#எனில் VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#endif
        m_domainp = domainp;
    }
};

class OrderLogicVertex final : பொது OrderEitherVertex {
    VL_RTTI_IMPL(OrderLogicVertex, OrderEitherVertex)

பொது:
    // Variable access record
    கட்டமைப்பு VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

தனிப்பட்ட:
    Ast_முனை* மாறிலி m_nodep;  // The logic this vertex represents
    AstScope* மாறிலி m_scopep;  // Scope the logic is under
    மாறிலி OrderDomain* மாறிலி m_hybridp;  // Additional sensitivities for hybrid combinational logic
    மாறிலி bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::vector<VarAccess> m_varAccesses;

பொது:
    // CONSTRUCTOR
    OrderLogicVertex(OrderGraph* graphp, AstScope* scopep, மாறிலி OrderDomain* domainp,
                     மாறிலி OrderDomain* hybridp, Ast_முனை* nodep) VL_MT_DISABLED
        : OrderEitherVertex{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~OrderLogicVertex() override = இயல்பு;

    // METHODS
    bool domainMatters() override { திரும்பு true; }

    // ACCESSORS
    Ast_முனை* nodep() மாறிலி VL_MT_STABLE { திரும்பு m_nodep; }
    AstScope* scopep() மாறிலி VL_MT_STABLE { திரும்பு m_scopep; }
    மாறிலி OrderDomain* hybridp() மாறிலி { திரும்பு m_hybridp; }
    bool comb() மாறிலி { திரும்பு m_comb; }
    மாறிலி std::vector<VarAccess>& varAccesses() மாறிலி { திரும்பு m_varAccesses; }
    வெறுமை addVarAccess(AstVarScope* vscp, VAccess access) {
        m_varAccesses.தள்ளு_back({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() மாறிலி override VL_MT_STABLE {
        திரும்பு (cvtToHex(m_nodep) + "\\n " + cvtToStr(nodep()->typeName()));
    }
    string dotShape() மாறிலி override { திரும்பு VN_IS(m_nodep, Active) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class OrderVarVertex VL_NOT_FINAL : பொது OrderEitherVertex {
    VL_RTTI_IMPL(OrderVarVertex, OrderEitherVertex)
    AstVarScope* மாறிலி m_vscp;

பொது:
    // CONSTRUCTOR
    OrderVarVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderEitherVertex{graphp, nullptr},
          m_vscp{vscp} {}
    ~OrderVarVertex() override = இயல்பு;

    // ACCESSORS
    AstVarScope* vscp() மாறிலி { திரும்பு m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() மாறிலி override final { திரும்பு "ellipse"; }
    மெய்நிகர் string nameSuffix() மாறிலி VL_MT_SAFE = 0;
    string name() மாறிலி override final VL_MT_STABLE {
        திரும்பு cvtToHex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class OrderVarStdVertex final : பொது OrderVarVertex {
    VL_RTTI_IMPL(OrderVarStdVertex, OrderVarVertex)
பொது:
    // CONSTRUCTOR
    OrderVarStdVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarStdVertex() override = இயல்பு;

    // METHODS
    bool domainMatters() override { திரும்பு true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() மாறிலி override VL_MT_SAFE { திரும்பு ""; }
    string dotColor() மாறிலி override { திரும்பு "grey"; }
    // LCOV_EXCL_STOP
};

class OrderVarPreVertex final : பொது OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPreVertex, OrderVarVertex)
பொது:
    // CONSTRUCTOR
    OrderVarPreVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPreVertex() override = இயல்பு;

    // METHODS
    bool domainMatters() override { திரும்பு false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() மாறிலி override VL_MT_SAFE { திரும்பு "PRE"; }
    string dotColor() மாறிலி override { திரும்பு "green"; }
    // LCOV_EXCL_STOP
};

class OrderVarPostVertex final : பொது OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPostVertex, OrderVarVertex)
பொது:
    // CONSTRUCTOR
    OrderVarPostVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPostVertex() override = இயல்பு;

    // METHODS
    bool domainMatters() override { திரும்பு false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() மாறிலி override VL_MT_SAFE { திரும்பு "POST"; }
    string dotColor() மாறிலி override { திரும்பு "red"; }
    // LCOV_EXCL_STOP
};

class OrderVarPordVertex final : பொது OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPordVertex, OrderVarVertex)
பொது:
    // CONSTRUCTOR
    OrderVarPordVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPordVertex() override = இயல்பு;

    // METHODS
    bool domainMatters() override { திரும்பு false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() மாறிலி override VL_MT_SAFE { திரும்பு "PORD"; }
    string dotColor() மாறிலி override { திரும்பு "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class OrderEdge final : பொது V3GraphEdge {
    VL_RTTI_IMPL(OrderEdge, V3GraphEdge)
    friend class OrderGraph;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    OrderEdge(OrderGraph* graphp, OrderEitherVertex* fromp, OrderEitherVertex* உச்சி, முழு weight,
              bool cutable) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, உச்சி, weight, cutable} {}
    ~OrderEdge() override = இயல்பு;

    // LCOV_EXCL_START // Debug code
    string dotColor() மாறிலி override { திரும்பு cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

வெறுமை OrderGraph::addHardEdge(OrderLogicVertex* fromp, OrderVarVertex* உச்சி,
                             முழு weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, உச்சி, weight, /* cutable: */ false};
}
வெறுமை OrderGraph::addHardEdge(OrderVarVertex* fromp, OrderLogicVertex* உச்சி,
                             முழு weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, உச்சி, weight, /* cutable: */ false};
}
வெறுமை OrderGraph::addSoftEdge(OrderLogicVertex* fromp, OrderVarVertex* உச்சி,
                             முழு weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, உச்சி, weight, /* cutable: */ true};
}
வெறுமை OrderGraph::addSoftEdge(OrderVarVertex* fromp, OrderLogicVertex* உச்சி,
                             முழு weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, உச்சி, weight, /* cutable: */ true};
}

#endif  // Guard
