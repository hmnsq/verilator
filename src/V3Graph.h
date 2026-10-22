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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3List.h"
#અન્તર્ભાવ "V3Rtti.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <functional>

class File_લીટી;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(અચલ V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
જાહેર:
    ગણના en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        અંક_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    ગણના en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"FORWARD", "REVERSE"};
        પાછા names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() અચલ { પાછા GraphWay{m_e ^ 1}; }
    constexpr bool forward() અચલ { પાછા m_e == FORWARD; }
    constexpr bool reverse() અચલ { પાછા m_e != FORWARD; }
};
constexpr bool operator==(અચલ GraphWay& lhs, અચલ GraphWay& rhs) { પાછા lhs.m_e == rhs.m_e; }
constexpr bool operator==(અચલ GraphWay& lhs, GraphWay::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, અચલ GraphWay& rhs) { પાછા lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
જાહેર:
    // ENUMS
    ગણના Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
સુરક્ષિત:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_ટોચ;  // Vertices this edge points to
    પૂર્ણાંક m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    સંઘ {
        શૂન્ય* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    શૂન્ય init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ, પૂર્ણાંક weight,
              bool cutable = false) VL_MT_DISABLED;
    શૂન્ય cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ,
                અચલ V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, ટોચ, old.m_weight, old.m_cutable);
    }

ખાનગી:
    V3ListLinks<V3GraphEdge>& oLinks() { પાછા m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { પાછા m_iLinks; }

જાહેર:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ, પૂર્ણાંક weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, ટોચ, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    આભાસી V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* ટોચ) અચલ VL_MT_DISABLED {
        પાછા new V3GraphEdge{graphp, fromp, ટોચ, *this};
    }
    આભાસી ~V3GraphEdge() = મૂળભૂત;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() અચલ {
        static_assert(std::is_base_of<V3GraphEdge, T>::કિંમત,
                      "'T' must be a subtype of V3GraphEdge");
        પાછા V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        પાછા static_cast<T*>(this);
    }
    template <typename T>
    અચલ T* as() અચલ {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        પાછા static_cast<અચલ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        પાછા is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    અચલ T* cast() અચલ {
        પાછા is<T>() ? static_cast<અચલ T*>(this) : nullptr;
    }

    આભાસી string name() અચલ VL_MT_DISABLED;
    આભાસી string dotLabel() અચલ { પાછા ""; }
    આભાસી string dotColor() અચલ { પાછા cutable() ? "yellowGreen" : "red"; }
    આભાસી string dotStyle() અચલ { પાછા cutable() ? "dashed" : ""; }
    આભાસી પૂર્ણાંક sortCmp(અચલ V3GraphEdge* rhsp) અચલ VL_MT_DISABLED;
    શૂન્ય unlinkDelete() VL_MT_DISABLED;
    શૂન્ય relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    શૂન્ય relink_ટોચ(V3GraphVertex* new_ટોચ) VL_MT_DISABLED;
    // ACCESSORS
    પૂર્ણાંક weight() અચલ { પાછા m_weight; }
    શૂન્ય weight(પૂર્ણાંક weight) { m_weight = weight; }
    bool cutable() અચલ { પાછા m_cutable; }
    શૂન્ય cutable(bool cutable) { m_cutable = cutable; }
    શૂન્ય userp(શૂન્ય* user) { m_userp = user; }
    શૂન્ય* userp() અચલ { પાછા m_userp; }
    શૂન્ય user(uint64_t user) { m_user = user; }
    uint64_t user() અચલ { પાછા m_user; }
    V3GraphVertex* fromp() અચલ { પાછા m_fromp; }
    V3GraphVertex* ટોચ() અચલ { પાછા m_ટોચ; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() અચલ {
        પાછા N_Way == GraphWay::FORWARD ? ટોચ() : fromp();
    }
    // STATIC ACCESSORS
    સ્થિર bool followNotCutable(અચલ V3GraphEdge* edgep) { પાછા !edgep->m_cutable; }
    સ્થિર bool followAlwaysTrue(અચલ V3GraphEdge*) { પાછા true; }
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
    દ્વિગુણ m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    સંઘ {
        શૂન્ય* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    શૂન્ય fanout(દ્વિગુણ fanout) { m_fanout = fanout; }

સુરક્ષિત:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, અચલ V3GraphVertex& old) VL_MT_DISABLED;

ખાનગી:
    V3ListLinks<V3GraphVertex>& links() { પાછા m_links; }

જાહેર:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    આભાસી V3GraphVertex* clone(V3Graph* graphp) અચલ VL_MT_DISABLED {
        પાછા new V3GraphVertex{graphp, *this};
    }
    આભાસી ~V3GraphVertex() = મૂળભૂત;
    શૂન્ય unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    શૂન્ય unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() અચલ {
        static_assert(std::is_base_of<V3GraphVertex, T>::કિંમત,
                      "'T' must be a subtype of V3GraphVertex");
        પાછા V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        પાછા static_cast<T*>(this);
    }
    template <typename T>
    અચલ T* as() અચલ {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        પાછા static_cast<અચલ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        પાછા is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    અચલ T* cast() અચલ {
        પાછા is<T>() ? static_cast<અચલ T*>(this) : nullptr;
    }

    // ACCESSORS
    આભાસી string name() અચલ { પાછા ""; }
    આભાસી string dotColor() અચલ { પાછા "black"; }
    આભાસી string dotShape() અચલ { પાછા ""; }
    આભાસી string dotStyle() અચલ { પાછા ""; }
    આભાસી string dotName() અચલ { પાછા ""; }
    આભાસી string dotRank() અચલ { પાછા ""; }
    આભાસી uint32_t rankAdder() અચલ { પાછા 1; }
    આભાસી File_લીટી* fileline() અચલ { પાછા nullptr; }  // nullptr for unknown
    આભાસી પૂર્ણાંક sortCmp(અચલ V3GraphVertex* rhsp) અચલ {
        // LHS goes first if of lower rank, or lower fanout
        જો (m_rank < rhsp->m_rank) પાછા -1;
        જો (m_rank > rhsp->m_rank) પાછા 1;
        જો (m_fanout < rhsp->m_fanout) પાછા -1;
        જો (m_fanout > rhsp->m_fanout) પાછા 1;
        પાછા 0;
    }
    uint32_t color() અચલ { પાછા m_color; }
    શૂન્ય color(uint32_t color) { m_color = color; }
    uint32_t rank() અચલ { પાછા m_rank; }
    શૂન્ય rank(uint32_t rank) { m_rank = rank; }
    દ્વિગુણ fanout() અચલ { પાછા m_fanout; }
    શૂન્ય user(uint32_t user) { m_user = user; }
    uint32_t user() અચલ VL_MT_STABLE { પાછા m_user; }
    શૂન્ય userp(શૂન્ય* userp) { m_userp = userp; }
    શૂન્ય* userp() અચલ VL_MT_STABLE { પાછા m_userp; }
    V3GraphEdge::IList& inEdges() { પાછા m_ins; }
    અચલ V3GraphEdge::IList& inEdges() અચલ { પાછા m_ins; }
    template <GraphWay::en N_Way>
    અંતઃરેખ auto& edges();
    template <GraphWay::en N_Way>
    અંતઃરેખ અચલ auto& edges() અચલ;
    bool inEmpty() અચલ { પાછા m_ins.empty(); }
    bool inSize1() અચલ { પાછા m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { પાછા m_outs; }
    અચલ V3GraphEdge::OList& outEdges() અચલ { પાછા m_outs; }
    bool outEmpty() અચલ { પાછા m_outs.empty(); }
    bool outSize1() અચલ { પાછા m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    શૂન્ય v3errorEnd(અચલ std::ostringstream& str) અચલ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    શૂન્ય v3errorEndFatal(અચલ std::ostringstream& str) અચલ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    શૂન્ય rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
અંતઃરેખ auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    પાછા m_outs;
}

template <>
અંતઃરેખ auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    પાછા m_ins;
}

template <>
અંતઃરેખ અચલ auto& V3GraphVertex::edges<GraphWay::FORWARD>() અચલ {
    પાછા m_outs;
}

template <>
અંતઃરેખ અચલ auto& V3GraphVertex::edges<GraphWay::REVERSE>() અચલ {
    પાછા m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

સુરક્ષિત:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    દ્વિગુણ orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    શૂન્ય dumpEdge(std::ostream& os, અચલ V3GraphVertex& vertex,
                  અચલ V3GraphEdge& edge) અચલ VL_MT_DISABLED;
    // ACCESSORS

જાહેર:
    V3Graph() VL_MT_DISABLED;
    આભાસી ~V3Graph() VL_MT_DISABLED;
    આભાસી string dotRankDir() અચલ { પાછા "TB"; }  // rankdir for dot plotting

    // METHODS
    શૂન્ય clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() અચલ { પાછા m_vertices.empty(); }
    V3GraphVertex::List& vertices() { પાછા m_vertices; }
    અચલ V3GraphVertex::List& vertices() અચલ { પાછા m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    શૂન્ય stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    શૂન્ય rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    શૂન્ય rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    શૂન્ય rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    શૂન્ય rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    શૂન્ય sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    શૂન્ય sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    શૂન્ય order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    શૂન્ય orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    શૂન્ય acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    શૂન્ય removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    શૂન્ય removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    શૂન્ય removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    શૂન્ય subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    શૂન્ય userClearVertices() VL_MT_DISABLED;
    શૂન્ય userClearEdges() VL_MT_DISABLED;

    /// Debugging
    શૂન્ય dump(std::ostream& os = std::cout) અચલ VL_MT_DISABLED;
    શૂન્ય dumpDotFile(અચલ string& filename, bool colorAsSubgraph) અચલ VL_MT_DISABLED;
    શૂન્ય dumpDotFilePrefixed(અચલ string& nameComment,
                             bool colorAsSubgraph = false) અચલ VL_MT_DISABLED;
    શૂન્ય dumpDotFilePrefixedAlways(અચલ string& nameComment,
                                   bool colorAsSubgraph = false) અચલ VL_MT_DISABLED;
    શૂન્ય dumpEdges(std::ostream& os, અચલ V3GraphVertex& vertex) અચલ VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    શૂન્ય hashGraphDebug(અચલ અક્ષર* debugName) અચલ VL_MT_DISABLED;

    સ્થિર શૂન્ય selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_ગણતરી = 0;  // Number of vertexes in the graph
        uint64_t m_edge_ગણતરી = 0;  // Number of edges in the grap

        ParallelismReport() = મૂળભૂત;

    જાહેર:
        ~ParallelismReport() = મૂળભૂત;
        ParallelismReport(અચલ ParallelismReport&) = મૂળભૂત;
        ParallelismReport& operator=(અચલ ParallelismReport&) = મૂળભૂત;

        uint64_t totalGraphCost() અચલ { પાછા m_totalGraphCost; }
        uint64_t criticalPathCost() અચલ { પાછા m_criticalPathCost; }
        uint64_t vertex_ગણતરી() અચલ { પાછા m_vertex_ગણતરી; }
        uint64_t edge_ગણતરી() અચલ { પાછા m_edge_ગણતરી; }
        દ્વિગુણ parallelismFactor() અચલ {
            પાછા (static_cast<દ્વિગુણ>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(અચલ V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    આભાસી શૂન્ય loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    આભાસી string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
