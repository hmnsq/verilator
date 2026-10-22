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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <vector>

class OrderLogicVertex;
class OrderVarVertex;

//======================================================================

ଗଣନା OrderWeights : uint8_t {
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
    ଅଚଳ std::set<uint32_t> m_triggers;  // The trigger indices

ସର୍ବସାଧାରଣ:
    explicit OrderDomain(ଅଚଳ std::set<uint32_t>& triggers)
        : m_triggers{triggers} {}

    ଅଚଳ std::set<uint32_t>& triggers() ଅଚଳ { ଫେରନ୍ତୁ m_triggers; }
    bool operator<(ଅଚଳ OrderDomain& other) ଅଚଳ { ଫେରନ୍ତୁ m_triggers < other.m_triggers; }
};

class OrderGraph final : ସର୍ବସାଧାରଣ V3Graph {
    ଅଚଳ V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* ଅଚଳ m_trigVscp;  // The trigger vector for this graph
    std::set<OrderDomain> m_domains;  // The interned domains

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    OrderGraph(ଅଚଳ V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() ଅଚଳ { ଫେରନ୍ତୁ m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    ଅଚଳ AstSenItem* trigSenItemp(uint32_t index) ଅଚଳ {
        ଫେରନ୍ତୁ m_trigKit.trigIdx2SenItem().at(index);
    }

    // Get the interned instance of the given domain
    ଅଚଳ OrderDomain* domainp(ଅଚଳ std::set<uint32_t>& triggers) {
        ଫେରନ୍ତୁ &*m_domains.emplace(triggers).first;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ addHardEdge(OrderLogicVertex* fromp, OrderVarVertex* ଶୀର୍ଷ,
                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) VL_MT_DISABLED;
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ addHardEdge(OrderVarVertex* fromp, OrderLogicVertex* ଶୀର୍ଷ,
                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) VL_MT_DISABLED;
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ addSoftEdge(OrderLogicVertex* fromp, OrderVarVertex* ଶୀର୍ଷ,
                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) VL_MT_DISABLED;
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ addSoftEdge(OrderVarVertex* fromp, OrderLogicVertex* ଶୀର୍ଷ,
                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class OrderEitherVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(OrderEitherVertex, V3GraphVertex)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    ଅଚଳ OrderDomain* m_domainp;

ସୁରକ୍ଷିତ:
    // CONSTRUCTOR
    OrderEitherVertex(OrderGraph* graphp, ଅଚଳ OrderDomain* domainp) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_domainp{domainp} {}
    ~OrderEitherVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    // METHODS
    ଆଭାସୀ bool domainMatters() = 0;

    // ACCESSORS
    ଅଚଳ OrderDomain* domainp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_domainp; }
    ଶୂନ୍ୟ domainp(ଅଚଳ OrderDomain* domainp) VL_MT_DISABLED {
#ଯଦି VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#endif
        m_domainp = domainp;
    }
};

class OrderLogicVertex final : ସର୍ବସାଧାରଣ OrderEitherVertex {
    VL_RTTI_IMPL(OrderLogicVertex, OrderEitherVertex)

ସର୍ବସାଧାରଣ:
    // Variable access record
    ସଂରଚନା VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

ବ୍ୟକ୍ତିଗତ:
    Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;  // The logic this vertex represents
    AstScope* ଅଚଳ m_scopep;  // Scope the logic is under
    ଅଚଳ OrderDomain* ଅଚଳ m_hybridp;  // Additional sensitivities for hybrid combinational logic
    ଅଚଳ bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::vector<VarAccess> m_varAccesses;

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    OrderLogicVertex(OrderGraph* graphp, AstScope* scopep, ଅଚଳ OrderDomain* domainp,
                     ଅଚଳ OrderDomain* hybridp, Ast_ଗଣ୍ଠି* nodep) VL_MT_DISABLED
        : OrderEitherVertex{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~OrderLogicVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    bool domainMatters() override { ଫେରନ୍ତୁ true; }

    // ACCESSORS
    Ast_ଗଣ୍ଠି* nodep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_nodep; }
    AstScope* scopep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_scopep; }
    ଅଚଳ OrderDomain* hybridp() ଅଚଳ { ଫେରନ୍ତୁ m_hybridp; }
    bool comb() ଅଚଳ { ଫେରନ୍ତୁ m_comb; }
    ଅଚଳ std::vector<VarAccess>& varAccesses() ଅଚଳ { ଫେରନ୍ତୁ m_varAccesses; }
    ଶୂନ୍ୟ addVarAccess(AstVarScope* vscp, VAccess access) {
        m_varAccesses.ଠେଲ_back({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() ଅଚଳ override VL_MT_STABLE {
        ଫେରନ୍ତୁ (cvtToHex(m_nodep) + "\\n " + cvtToStr(nodep()->typeName()));
    }
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ VN_IS(m_nodep, Active) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class OrderVarVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ OrderEitherVertex {
    VL_RTTI_IMPL(OrderVarVertex, OrderEitherVertex)
    AstVarScope* ଅଚଳ m_vscp;

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    OrderVarVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderEitherVertex{graphp, nullptr},
          m_vscp{vscp} {}
    ~OrderVarVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    AstVarScope* vscp() ଅଚଳ { ଫେରନ୍ତୁ m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() ଅଚଳ override final { ଫେରନ୍ତୁ "ellipse"; }
    ଆଭାସୀ string nameSuffix() ଅଚଳ VL_MT_SAFE = 0;
    string name() ଅଚଳ override final VL_MT_STABLE {
        ଫେରନ୍ତୁ cvtToHex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class OrderVarStdVertex final : ସର୍ବସାଧାରଣ OrderVarVertex {
    VL_RTTI_IMPL(OrderVarStdVertex, OrderVarVertex)
ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    OrderVarStdVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarStdVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    bool domainMatters() override { ଫେରନ୍ତୁ true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ ""; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "grey"; }
    // LCOV_EXCL_STOP
};

class OrderVarPreVertex final : ସର୍ବସାଧାରଣ OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPreVertex, OrderVarVertex)
ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    OrderVarPreVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPreVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    bool domainMatters() override { ଫେରନ୍ତୁ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ "PRE"; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "green"; }
    // LCOV_EXCL_STOP
};

class OrderVarPostVertex final : ସର୍ବସାଧାରଣ OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPostVertex, OrderVarVertex)
ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    OrderVarPostVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPostVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    bool domainMatters() override { ଫେରନ୍ତୁ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ "POST"; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "red"; }
    // LCOV_EXCL_STOP
};

class OrderVarPordVertex final : ସର୍ବସାଧାରଣ OrderVarVertex {
    VL_RTTI_IMPL(OrderVarPordVertex, OrderVarVertex)
ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    OrderVarPordVertex(OrderGraph* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : OrderVarVertex{graphp, vscp} {}
    ~OrderVarPordVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    bool domainMatters() override { ଫେରନ୍ତୁ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ "PORD"; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class OrderEdge final : ସର୍ବସାଧାରଣ V3GraphEdge {
    VL_RTTI_IMPL(OrderEdge, V3GraphEdge)
    friend class OrderGraph;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    OrderEdge(OrderGraph* graphp, OrderEitherVertex* fromp, OrderEitherVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight,
              bool cutable) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, weight, cutable} {}
    ~OrderEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // LCOV_EXCL_START // Debug code
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

ଶୂନ୍ୟ OrderGraph::addHardEdge(OrderLogicVertex* fromp, OrderVarVertex* ଶୀର୍ଷ,
                             ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, ଶୀର୍ଷ, weight, /* cutable: */ false};
}
ଶୂନ୍ୟ OrderGraph::addHardEdge(OrderVarVertex* fromp, OrderLogicVertex* ଶୀର୍ଷ,
                             ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, ଶୀର୍ଷ, weight, /* cutable: */ false};
}
ଶୂନ୍ୟ OrderGraph::addSoftEdge(OrderLogicVertex* fromp, OrderVarVertex* ଶୀର୍ଷ,
                             ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, ଶୀର୍ଷ, weight, /* cutable: */ true};
}
ଶୂନ୍ୟ OrderGraph::addSoftEdge(OrderVarVertex* fromp, OrderLogicVertex* ଶୀର୍ଷ,
                             ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) VL_MT_DISABLED {
    new OrderEdge{this, fromp, ଶୀର୍ଷ, weight, /* cutable: */ true};
}

#endif  // Guard
