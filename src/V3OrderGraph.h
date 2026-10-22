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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Sched.h"

#अन्तर्भाव <set>
#अन्तर्भाव <vector>

class OrderLogicVertex;
class OrderVarVertex;

//======================================================================

गणना OrderWeights : uint8_t {
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
    ध्रुव std::set<uint32_t> m_triggers;  // The trigger indices

सार्वजनिक:
    explicit OrderDomain(ध्रुव std::set<uint32_t>& triggers)
        : m_triggers{triggers} {}

    ध्रुव std::set<uint32_t>& triggers() ध्रुव { पटिनिवत्त m_triggers; }
    bool operator<(ध्रुव OrderDomain& other) ध्रुव { पटिनिवत्त m_triggers < other.m_triggers; }
};

class OrderGraph final : सार्वजनिक V3Graph {
    ध्रुव V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* ध्रुव m_trigVscp;  // The trigger vector for this graph
    std::set<OrderDomain> m_domains;  // The interned domains

सार्वजनिक:
    // CONSTRUCTOR
    OrderGraph(ध्रुव V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() ध्रुव { पटिनिवत्त m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    ध्रुव AstSenItem* trigSenItemp(uint32_t index) ध्रुव {
        पटिनिवत्त m_trigKit.trigIdx2SenItem().at(index);
    }

    // Get the interned instance of the given domain
    ध्रुव OrderDomain* domainp(ध्रुव std::set<uint32_t>& triggers) {
        पटिनिवत्त &*m_domains.emplace(triggers).first;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    अन्तःस्थ शून्य addHardEdge(OrderLogicVertex* fromp, OrderVarVertex* शिखरम्,
                            पूर्णाङ्क weight) VL_MT_DISABLED;
    अन्तःस्थ शून्य addHardEdge(OrderVarVertex* fromp, OrderLogicVertex* शिखरम्,
                            पूर्णाङ्क weight) VL_MT_DISABLED;
    अन्तःस्थ शून्य addSoftEdge(OrderLogicVertex* fromp, OrderVarVertex* शिखरम्,
                            पूर्णाङ्क weight) VL_MT_DISABLED;
    अन्तःस्थ शून्य addSoftEdge(OrderVarVertex* fromp, OrderLogicVertex* शिखरम्,
                            पूर्णाङ्क weight) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class OrderEitherVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(OrderEitherVertex, V3GraphVertex)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    ध्रुव OrderDomain* m_domainp;

रक्षित:
    // CONSTRUCTOR
    OrderEitherVertex(OrderGraph* graphp, ध्रुव OrderDomain* domainp) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_domainp{domainp} {}
    ~OrderEitherVertex() override = स्वभाव;

सार्वजनिक:
    // METHODS
    आभास bool domainMatters() = 0;

    // ACCESSORS
    ध्रुव OrderDomain* domainp() ध्रुव VL_MT_STABLE { पटिनिवत्त m_domainp; }
    शून्य domainp(ध्रुव OrderDomain* domainp) VL_MT_DISABLED {
#सचे VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#endif
        m_domainp = domainp;
    }
};

class OrderLogicVertex final : सार्वजनिक OrderEitherVertex {
    VL_RTTI_IMPL(OrderLogicVertex, OrderEitherVertex)

सार्वजनिक:
    // Variable access record
    संरचना VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

गुप्त:
    Ast_ग्रन्थिः* ध्रुव m_nodep;  // The logic this vertex represents
    AstScope* ध्रुव m_scopep;  // Scope the logic is under
    ध्रुव OrderDomain* ध्रुव m_hybridp;  // Additional sensitivities for hybrid combinational logic
    ध्रुव bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::vector<VarAccess> m_varAccesses;

सार्वजनिक:
    // CONSTRUCTOR
    OrderLogicVertex(OrderGraph* graphp, AstScope* scopep, ध्रुव OrderDomain* domainp,
                     ध्रुव OrderDomain* hybridp, Ast_ग्रन्थिः* nodep) VL_MT_DISABLED
        : OrderEitherVertex{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~OrderLogicVertex() override = स्वभाव;

    // METHODS
    bool domainMatters() override { पटिनिवत्त true; }

    // ACCESSORS
    Ast_ग्रन्थिः* nodep() ध्रुव VL_MT_STABLE { पटिनिवत्त m_nodep; }
    AstScope* scopep() ध्रुव VL_MT_STABLE { पटिनिवत्त m_scopep; }
    ध्रुव OrderDomain* hybridp() ध्रुव { पटिनिवत्त m_hybridp; }
    bool comb() ध्रुव { पटिनिवत्त m_comb; }
    ध्रुव std::vector<VarAccess>& varAccesses() ध्रुव { पटिनिवत्त m_varAccesses; }
    शून्य addVarAccess(AstVarScope* vscp, VAccess access) {
        m_varAccesses.प्रक्षिप_back({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() ध्रुव override VL_MT_STABLE {
        पटिनिवत्त (cvtToHex(m_nodep) + "\\n " + cvtToStr(nodep()->typeName()));
    }
    string dotShape() ध्रुव override { पटिनिवत्त VN_IS(m_nodep, Active) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class OrderVarVertex VL_NOT_FINAL : सार्वजनिक OrderEitherVertex {
    VL_RTTI_IMPL(OrderVarVertex, OrderEitherVertex)
    AstVarScope* ध्रुव m_vscp;

सार्वजनिक:
    // CONSTRUCTOR
    OrderVarVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderEitherVertex{graphp, nullptr},
          m_vscp{vscp} {}
    ~OrderVarVertex() override = स्वभाव;

    // ACCESSORS
    AstVarScope* vscp() ध्रुव { पटिनिवत्त m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() ध्रुव override final { पटिनिवत्त "ellipse"; }
    आभास string nameSuffix() ध्रुव VL_MT_SAFE = 0;
    string name() ध्रुव override final VL_MT_STABLE {
        पटिनिवत्त cvtToHex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class OrderVarStdVertex final : सार्वजनिक OrderVarVertex {
    VL_RTTI_IMPL(OrderVarStdVertex, OrderVarVertex)
सार्वजनिक:
    // CONSTRUCTOR
    OrderVarStdVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarStdVertex() override = स्वभाव;

    // METHODS
    bool domainMatters() override { पटिनिवत्त true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ध्रुव override VL_MT_SAFE { पटिनिवत्त ""; }
    string dotColor() ध्रुव override { पटिनिवत्त "grey"; }
    // LCOV_EXCL_STOP
};

class OrderVarPreVertex final : सार्वजनिक OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPreVertex, OrderVarVertex)
सार्वजनिक:
    // CONSTRUCTOR
    OrderVarPreVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPreVertex() override = स्वभाव;

    // METHODS
    bool domainMatters() override { पटिनिवत्त false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ध्रुव override VL_MT_SAFE { पटिनिवत्त "PRE"; }
    string dotColor() ध्रुव override { पटिनिवत्त "green"; }
    // LCOV_EXCL_STOP
};

class OrderVarPostVertex final : सार्वजनिक OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPostVertex, OrderVarVertex)
सार्वजनिक:
    // CONSTRUCTOR
    OrderVarPostVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPostVertex() override = स्वभाव;

    // METHODS
    bool domainMatters() override { पटिनिवत्त false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ध्रुव override VL_MT_SAFE { पटिनिवत्त "POST"; }
    string dotColor() ध्रुव override { पटिनिवत्त "red"; }
    // LCOV_EXCL_STOP
};

class OrderVarPordVertex final : सार्वजनिक OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPordVertex, OrderVarVertex)
सार्वजनिक:
    // CONSTRUCTOR
    OrderVarPordVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPordVertex() override = स्वभाव;

    // METHODS
    bool domainMatters() override { पटिनिवत्त false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ध्रुव override VL_MT_SAFE { पटिनिवत्त "PORD"; }
    string dotColor() ध्रुव override { पटिनिवत्त "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class OrderEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(OrderEdge, V3GraphEdge)
    friend class OrderGraph;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    OrderEdge(OrderGraph* graphp, OrderEitherVertex* fromp, OrderEitherVertex* शिखरम्, पूर्णाङ्क weight,
              bool cutable) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, शिखरम्, weight, cutable} {}
    ~OrderEdge() override = स्वभाव;

    // LCOV_EXCL_START // Debug code
    string dotColor() ध्रुव override { पटिनिवत्त cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

शून्य OrderGraph::addHardEdge(OrderLogicVertex* fromp, OrderVarVertex* शिखरम्,
                             पूर्णाङ्क weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, शिखरम्, weight, /* cutable: */ false};
}
शून्य OrderGraph::addHardEdge(OrderVarVertex* fromp, OrderLogicVertex* शिखरम्,
                             पूर्णाङ्क weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, शिखरम्, weight, /* cutable: */ false};
}
शून्य OrderGraph::addSoftEdge(OrderLogicVertex* fromp, OrderVarVertex* शिखरम्,
                             पूर्णाङ्क weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, शिखरम्, weight, /* cutable: */ true};
}
शून्य OrderGraph::addSoftEdge(OrderVarVertex* fromp, OrderLogicVertex* शिखरम्,
                             पूर्णाङ्क weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, शिखरम्, weight, /* cutable: */ true};
}

#endif  // Guard
