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

#अगर_अघोषित VERILATOR_V3ORDERGRAPH_H_
#घोषणा VERILATOR_V3ORDERGRAPH_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Graph.h"
#समावेश "V3Sched.h"

#समावेश <set>
#समावेश <vector>

class OrderLogicVertex;
class OrderVarVertex;

//======================================================================

क्रमागत OrderWeights : uint8_t {
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
    स्थिर std::set<uint32_t> m_triggers;  // The trigger indices

खुला:
    explicit OrderDomain(स्थिर std::set<uint32_t>& triggers)
        : m_triggers{triggers} {}

    स्थिर std::set<uint32_t>& triggers() स्थिर { वापस m_triggers; }
    bool चालक<(स्थिर OrderDomain& other) स्थिर { वापस m_triggers < other.m_triggers; }
};

class OrderGraph final : खुला V3Graph {
    स्थिर V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* स्थिर m_trigVscp;  // The trigger vector for this graph
    std::set<OrderDomain> m_domains;  // The interned domains

खुला:
    // CONSTRUCTOR
    OrderGraph(स्थिर V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() स्थिर { वापस m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    स्थिर AstSenItem* trigSenItemp(uint32_t सूचकांक_2) स्थिर {
        वापस m_trigKit.trigIdx2SenItem().at(सूचकांक_2);
    }

    // Get the interned instance of the given domain
    स्थिर OrderDomain* domainp(स्थिर std::set<uint32_t>& triggers) {
        वापस &*m_domains.emplace(triggers).first;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    अंतरभूत व्योम addHardEdge(OrderLogicVertex* fromp, OrderVarVertex* शीर्ष,
                            पूर्णांक weight) VL_MT_DISABLED;
    अंतरभूत व्योम addHardEdge(OrderVarVertex* fromp, OrderLogicVertex* शीर्ष,
                            पूर्णांक weight) VL_MT_DISABLED;
    अंतरभूत व्योम addSoftEdge(OrderLogicVertex* fromp, OrderVarVertex* शीर्ष,
                            पूर्णांक weight) VL_MT_DISABLED;
    अंतरभूत व्योम addSoftEdge(OrderVarVertex* fromp, OrderLogicVertex* शीर्ष,
                            पूर्णांक weight) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class OrderEitherVertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(OrderEitherVertex, V3GraphVertex)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    स्थिर OrderDomain* m_domainp;

रक्षित:
    // CONSTRUCTOR
    OrderEitherVertex(OrderGraph* graphp, स्थिर OrderDomain* domainp) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_domainp{domainp} {}
    ~OrderEitherVertex() override = शेष;

खुला:
    // METHODS
    भव bool domainMatters() = 0;

    // ACCESSORS
    स्थिर OrderDomain* domainp() स्थिर VL_MT_STABLE { वापस m_domainp; }
    व्योम domainp(स्थिर OrderDomain* domainp) VL_MT_DISABLED {
#अगर VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#पूर्ण_अगर
        m_domainp = domainp;
    }
};

class OrderLogicVertex final : खुला OrderEitherVertex {
    VL_RTTI_IMPL(OrderLogicVertex, OrderEitherVertex)

खुला:
    // Variable access record
    काष्ठा VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

निजी:
    Ast_गाँठ* स्थिर m_nodep;  // The logic this vertex represents
    AstScope* स्थिर m_scopep;  // Scope the logic is under
    स्थिर OrderDomain* स्थिर m_hybridp;  // Additional sensitivities for hybrid combinational logic
    स्थिर bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::vector<VarAccess> m_varAccesses;

खुला:
    // CONSTRUCTOR
    OrderLogicVertex(OrderGraph* graphp, AstScope* scopep, स्थिर OrderDomain* domainp,
                     स्थिर OrderDomain* hybridp, Ast_गाँठ* nodep) VL_MT_DISABLED
        : OrderEitherVertex{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~OrderLogicVertex() override = शेष;

    // METHODS
    bool domainMatters() override { वापस true; }

    // ACCESSORS
    Ast_गाँठ* nodep() स्थिर VL_MT_STABLE { वापस m_nodep; }
    AstScope* scopep() स्थिर VL_MT_STABLE { वापस m_scopep; }
    स्थिर OrderDomain* hybridp() स्थिर { वापस m_hybridp; }
    bool comb() स्थिर { वापस m_comb; }
    स्थिर std::vector<VarAccess>& varAccesses() स्थिर { वापस m_varAccesses; }
    व्योम addVarAccess(AstVarScope* vscp, VAccess access) {
        m_varAccesses.धकेलो_back({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() स्थिर override VL_MT_STABLE {
        वापस (cvtToHex(m_nodep) + "\\n " + cvtToStr(nodep()->typeName()));
    }
    string dotShape() स्थिर override { वापस VN_IS(m_nodep, Active) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class OrderVarVertex VL_NOT_FINAL : खुला OrderEitherVertex {
    VL_RTTI_IMPL(OrderVarVertex, OrderEitherVertex)
    AstVarScope* स्थिर m_vscp;

खुला:
    // CONSTRUCTOR
    OrderVarVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderEitherVertex{graphp, nullptr},
          m_vscp{vscp} {}
    ~OrderVarVertex() override = शेष;

    // ACCESSORS
    AstVarScope* vscp() स्थिर { वापस m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() स्थिर override final { वापस "ellipse"; }
    भव string nameSuffix() स्थिर VL_MT_SAFE = 0;
    string name() स्थिर override final VL_MT_STABLE {
        वापस cvtToHex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class OrderVarStdVertex final : खुला OrderVarVertex {
    VL_RTTI_IMPL(OrderVarStdVertex, OrderVarVertex)
खुला:
    // CONSTRUCTOR
    OrderVarStdVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarStdVertex() override = शेष;

    // METHODS
    bool domainMatters() override { वापस true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() स्थिर override VL_MT_SAFE { वापस ""; }
    string dotColor() स्थिर override { वापस "grey"; }
    // LCOV_EXCL_STOP
};

class OrderVarPreVertex final : खुला OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPreVertex, OrderVarVertex)
खुला:
    // CONSTRUCTOR
    OrderVarPreVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPreVertex() override = शेष;

    // METHODS
    bool domainMatters() override { वापस false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() स्थिर override VL_MT_SAFE { वापस "PRE"; }
    string dotColor() स्थिर override { वापस "green"; }
    // LCOV_EXCL_STOP
};

class OrderVarPostVertex final : खुला OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPostVertex, OrderVarVertex)
खुला:
    // CONSTRUCTOR
    OrderVarPostVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPostVertex() override = शेष;

    // METHODS
    bool domainMatters() override { वापस false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() स्थिर override VL_MT_SAFE { वापस "POST"; }
    string dotColor() स्थिर override { वापस "red"; }
    // LCOV_EXCL_STOP
};

class OrderVarPordVertex final : खुला OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPordVertex, OrderVarVertex)
खुला:
    // CONSTRUCTOR
    OrderVarPordVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPordVertex() override = शेष;

    // METHODS
    bool domainMatters() override { वापस false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() स्थिर override VL_MT_SAFE { वापस "PORD"; }
    string dotColor() स्थिर override { वापस "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class OrderEdge final : खुला V3GraphEdge {
    VL_RTTI_IMPL(OrderEdge, V3GraphEdge)
    मित्र class OrderGraph;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    OrderEdge(OrderGraph* graphp, OrderEitherVertex* fromp, OrderEitherVertex* शीर्ष, पूर्णांक weight,
              bool cutable) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, शीर्ष, weight, cutable} {}
    ~OrderEdge() override = शेष;

    // LCOV_EXCL_START // Debug code
    string dotColor() स्थिर override { वापस cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

व्योम OrderGraph::addHardEdge(OrderLogicVertex* fromp, OrderVarVertex* शीर्ष,
                             पूर्णांक weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, शीर्ष, weight, /* cutable: */ false};
}
व्योम OrderGraph::addHardEdge(OrderVarVertex* fromp, OrderLogicVertex* शीर्ष,
                             पूर्णांक weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, शीर्ष, weight, /* cutable: */ false};
}
व्योम OrderGraph::addSoftEdge(OrderLogicVertex* fromp, OrderVarVertex* शीर्ष,
                             पूर्णांक weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, शीर्ष, weight, /* cutable: */ true};
}
व्योम OrderGraph::addSoftEdge(OrderVarVertex* fromp, OrderLogicVertex* शीर्ष,
                             पूर्णांक weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, शीर्ष, weight, /* cutable: */ true};
}

#पूर्ण_अगर  // Guard
