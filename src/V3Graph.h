// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph optimizations
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

#ifndef VERILATOR_V3GRAPH_H_
#define VERILATOR_V3GRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3List.h"
#ضمن "V3Rtti.h"

#ضمن <algorithm>
#ضمن <functional>

class File_هێڵ;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(ثابت V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
عام:
    تعداد en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        ژمارە_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    تعداد en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"FORWARD", "REVERSE"};
        أرجع names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() ثابت { أرجع GraphWay{m_e ^ 1}; }
    constexpr bool forward() ثابت { أرجع m_e == FORWARD; }
    constexpr bool reverse() ثابت { أرجع m_e != FORWARD; }
};
constexpr bool operator==(ثابت GraphWay& lhs, ثابت GraphWay& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت GraphWay& lhs, GraphWay::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, ثابت GraphWay& rhs) { أرجع lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
عام:
    // ENUMS
    تعداد Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
محمي:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_لووتکە;  // Vertices this edge points to
    صحيح m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    اتحاد {
        فراغ* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    فراغ init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* لووتکە, صحيح weight,
              bool cutable = false) VL_MT_DISABLED;
    فراغ cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* لووتکە,
                ثابت V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, لووتکە, old.m_weight, old.m_cutable);
    }

خاص:
    V3ListLinks<V3GraphEdge>& oLinks() { أرجع m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { أرجع m_iLinks; }

عام:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* لووتکە, صحيح weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, لووتکە, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    virtual V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* لووتکە) ثابت VL_MT_DISABLED {
        أرجع new V3GraphEdge{graphp, fromp, لووتکە, *this};
    }
    virtual ~V3GraphEdge() = افتراضي;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ثابت {
        static_assert(std::is_base_of<V3GraphEdge, T>::بەها,
                      "'T' must be a subtype of V3GraphEdge");
        أرجع V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        أرجع static_cast<T*>(this);
    }
    template <typename T>
    ثابت T* as() ثابت {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        أرجع static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        أرجع is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ثابت T* cast() ثابت {
        أرجع is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    virtual string name() ثابت VL_MT_DISABLED;
    virtual string dotLabel() ثابت { أرجع ""; }
    virtual string dotColor() ثابت { أرجع cutable() ? "yellowGreen" : "red"; }
    virtual string dotStyle() ثابت { أرجع cutable() ? "dashed" : ""; }
    virtual صحيح sortCmp(ثابت V3GraphEdge* rhsp) ثابت VL_MT_DISABLED;
    فراغ unlinkDelete() VL_MT_DISABLED;
    فراغ relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    فراغ relink_لووتکە(V3GraphVertex* new_لووتکە) VL_MT_DISABLED;
    // ACCESSORS
    صحيح weight() ثابت { أرجع m_weight; }
    فراغ weight(صحيح weight) { m_weight = weight; }
    bool cutable() ثابت { أرجع m_cutable; }
    فراغ cutable(bool cutable) { m_cutable = cutable; }
    فراغ userp(فراغ* user) { m_userp = user; }
    فراغ* userp() ثابت { أرجع m_userp; }
    فراغ user(uint64_t user) { m_user = user; }
    uint64_t user() ثابت { أرجع m_user; }
    V3GraphVertex* fromp() ثابت { أرجع m_fromp; }
    V3GraphVertex* لووتکە() ثابت { أرجع m_لووتکە; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() ثابت {
        أرجع N_Way == GraphWay::FORWARD ? لووتکە() : fromp();
    }
    // STATIC ACCESSORS
    ساكن bool followNotCutable(ثابت V3GraphEdge* edgep) { أرجع !edgep->m_cutable; }
    ساكن bool followAlwaysTrue(ثابت V3GraphEdge*) { أرجع true; }
};

//============================================================================

class V3GraphVertex VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphVertex)
    friend class V3Graph;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    friend class GraphAlgRank;
    friend class GraphAlgRankDepth;
    V3ListLinks<V3GraphVertex> m_links;  // List links to store instances of this class
    V3GraphEdge::OList m_outs;  // List of outbound edges
    V3GraphEdge::IList m_ins;  // List of inbound edges
    مزدوج m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    اتحاد {
        فراغ* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    فراغ fanout(مزدوج fanout) { m_fanout = fanout; }

محمي:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, ثابت V3GraphVertex& old) VL_MT_DISABLED;

خاص:
    V3ListLinks<V3GraphVertex>& links() { أرجع m_links; }

عام:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    virtual V3GraphVertex* clone(V3Graph* graphp) ثابت VL_MT_DISABLED {
        أرجع new V3GraphVertex{graphp, *this};
    }
    virtual ~V3GraphVertex() = افتراضي;
    فراغ unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    فراغ unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ثابت {
        static_assert(std::is_base_of<V3GraphVertex, T>::بەها,
                      "'T' must be a subtype of V3GraphVertex");
        أرجع V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        أرجع static_cast<T*>(this);
    }
    template <typename T>
    ثابت T* as() ثابت {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        أرجع static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        أرجع is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ثابت T* cast() ثابت {
        أرجع is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    // ACCESSORS
    virtual string name() ثابت { أرجع ""; }
    virtual string dotColor() ثابت { أرجع "black"; }
    virtual string dotShape() ثابت { أرجع ""; }
    virtual string dotStyle() ثابت { أرجع ""; }
    virtual string dotName() ثابت { أرجع ""; }
    virtual string dotRank() ثابت { أرجع ""; }
    virtual uint32_t rankAdder() ثابت { أرجع 1; }
    virtual File_هێڵ* fileline() ثابت { أرجع nullptr; }  // nullptr for unknown
    virtual صحيح sortCmp(ثابت V3GraphVertex* rhsp) ثابت {
        // LHS goes first if of lower rank, or lower fanout
        إذا (m_rank < rhsp->m_rank) أرجع -1;
        إذا (m_rank > rhsp->m_rank) أرجع 1;
        إذا (m_fanout < rhsp->m_fanout) أرجع -1;
        إذا (m_fanout > rhsp->m_fanout) أرجع 1;
        أرجع 0;
    }
    uint32_t color() ثابت { أرجع m_color; }
    فراغ color(uint32_t color) { m_color = color; }
    uint32_t rank() ثابت { أرجع m_rank; }
    فراغ rank(uint32_t rank) { m_rank = rank; }
    مزدوج fanout() ثابت { أرجع m_fanout; }
    فراغ user(uint32_t user) { m_user = user; }
    uint32_t user() ثابت VL_MT_STABLE { أرجع m_user; }
    فراغ userp(فراغ* userp) { m_userp = userp; }
    فراغ* userp() ثابت VL_MT_STABLE { أرجع m_userp; }
    V3GraphEdge::IList& inEdges() { أرجع m_ins; }
    ثابت V3GraphEdge::IList& inEdges() ثابت { أرجع m_ins; }
    template <GraphWay::en N_Way>
    مضمن auto& edges();
    template <GraphWay::en N_Way>
    مضمن ثابت auto& edges() ثابت;
    bool inEmpty() ثابت { أرجع m_ins.empty(); }
    bool inSize1() ثابت { أرجع m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { أرجع m_outs; }
    ثابت V3GraphEdge::OList& outEdges() ثابت { أرجع m_outs; }
    bool outEmpty() ثابت { أرجع m_outs.empty(); }
    bool outSize1() ثابت { أرجع m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    فراغ v3errorEnd(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    فراغ v3errorEndFatal(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    فراغ rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
مضمن auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    أرجع m_outs;
}

template <>
مضمن auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    أرجع m_ins;
}

template <>
مضمن ثابت auto& V3GraphVertex::edges<GraphWay::FORWARD>() ثابت {
    أرجع m_outs;
}

template <>
مضمن ثابت auto& V3GraphVertex::edges<GraphWay::REVERSE>() ثابت {
    أرجع m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

محمي:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    مزدوج orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    فراغ dumpEdge(std::ostream& os, ثابت V3GraphVertex& vertex,
                  ثابت V3GraphEdge& edge) ثابت VL_MT_DISABLED;
    // ACCESSORS

عام:
    V3Graph() VL_MT_DISABLED;
    virtual ~V3Graph() VL_MT_DISABLED;
    virtual string dotRankDir() ثابت { أرجع "TB"; }  // rankdir for dot plotting

    // METHODS
    فراغ clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() ثابت { أرجع m_vertices.empty(); }
    V3GraphVertex::List& vertices() { أرجع m_vertices; }
    ثابت V3GraphVertex::List& vertices() ثابت { أرجع m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    فراغ stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    فراغ rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    فراغ rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    فراغ rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    فراغ rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    فراغ sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    فراغ sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    فراغ order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    فراغ orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    فراغ acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    فراغ removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    فراغ removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    فراغ removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    فراغ subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    فراغ userClearVertices() VL_MT_DISABLED;
    فراغ userClearEdges() VL_MT_DISABLED;

    /// Debugging
    فراغ dump(std::ostream& os = std::cout) ثابت VL_MT_DISABLED;
    فراغ dumpDotFile(ثابت string& filename, bool colorAsSubgraph) ثابت VL_MT_DISABLED;
    فراغ dumpDotFilePrefixed(ثابت string& nameComment,
                             bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    فراغ dumpDotFilePrefixedAlways(ثابت string& nameComment,
                                   bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    فراغ dumpEdges(std::ostream& os, ثابت V3GraphVertex& vertex) ثابت VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    فراغ hashGraphDebug(ثابت حرف* debugName) ثابت VL_MT_DISABLED;

    ساكن فراغ selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_ژماردن = 0;  // Number of vertexes in the graph
        uint64_t m_edge_ژماردن = 0;  // Number of edges in the grap

        ParallelismReport() = افتراضي;

    عام:
        ~ParallelismReport() = افتراضي;
        ParallelismReport(ثابت ParallelismReport&) = افتراضي;
        ParallelismReport& operator=(ثابت ParallelismReport&) = افتراضي;

        uint64_t totalGraphCost() ثابت { أرجع m_totalGraphCost; }
        uint64_t criticalPathCost() ثابت { أرجع m_criticalPathCost; }
        uint64_t vertex_ژماردن() ثابت { أرجع m_vertex_ژماردن; }
        uint64_t edge_ژماردن() ثابت { أرجع m_edge_ژماردن; }
        مزدوج parallelismFactor() ثابت {
            أرجع (static_cast<مزدوج>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ثابت V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    virtual فراغ loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    virtual string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
