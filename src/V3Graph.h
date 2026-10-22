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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3List.h"
#অন্তর্ভাব "V3Rtti.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <functional>

class File_সারি;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(ধ্রুব V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
সার্বজনিক:
    গণনা en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        অঙ্ক_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    গণনা en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুব { প্রত্যাগম m_e; }
    ধ্রুব বর্ণ* ascii() ধ্রুব {
        স্থির constexpr ধ্রুব বর্ণ* ধ্রুব names[] = {"FORWARD", "REVERSE"};
        প্রত্যাগম names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() ধ্রুব { প্রত্যাগম GraphWay{m_e ^ 1}; }
    constexpr bool forward() ধ্রুব { প্রত্যাগম m_e == FORWARD; }
    constexpr bool reverse() ধ্রুব { প্রত্যাগম m_e != FORWARD; }
};
constexpr bool operator==(ধ্রুব GraphWay& lhs, ধ্রুব GraphWay& rhs) { প্রত্যাগম lhs.m_e == rhs.m_e; }
constexpr bool operator==(ধ্রুব GraphWay& lhs, GraphWay::en rhs) { প্রত্যাগম lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, ধ্রুব GraphWay& rhs) { প্রত্যাগম lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
সার্বজনিক:
    // ENUMS
    গণনা Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
রক্ষিত:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_শীর্ষ;  // Vertices this edge points to
    পূর্ণাঙ্ক m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    সঙ্ঘ {
        শূন্য* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    শূন্য init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণাঙ্ক weight,
              bool cutable = false) VL_MT_DISABLED;
    শূন্য cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ,
                ধ্রুব V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, শীর্ষ, old.m_weight, old.m_cutable);
    }

গুপ্ত:
    V3ListLinks<V3GraphEdge>& oLinks() { প্রত্যাগম m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { প্রত্যাগম m_iLinks; }

সার্বজনিক:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণাঙ্ক weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, শীর্ষ, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    আভাস V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* শীর্ষ) ধ্রুব VL_MT_DISABLED {
        প্রত্যাগম new V3GraphEdge{graphp, fromp, শীর্ষ, *this};
    }
    আভাস ~V3GraphEdge() = স্বভাব;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ধ্রুব {
        static_assert(std::is_base_of<V3GraphEdge, T>::মান,
                      "'T' must be a subtype of V3GraphEdge");
        প্রত্যাগম V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        প্রত্যাগম static_cast<T*>(this);
    }
    template <typename T>
    ধ্রুব T* as() ধ্রুব {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        প্রত্যাগম static_cast<ধ্রুব T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        প্রত্যাগম is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ধ্রুব T* cast() ধ্রুব {
        প্রত্যাগম is<T>() ? static_cast<ধ্রুব T*>(this) : nullptr;
    }

    আভাস string name() ধ্রুব VL_MT_DISABLED;
    আভাস string dotLabel() ধ্রুব { প্রত্যাগম ""; }
    আভাস string dotColor() ধ্রুব { প্রত্যাগম cutable() ? "yellowGreen" : "red"; }
    আভাস string dotStyle() ধ্রুব { প্রত্যাগম cutable() ? "dashed" : ""; }
    আভাস পূর্ণাঙ্ক sortCmp(ধ্রুব V3GraphEdge* rhsp) ধ্রুব VL_MT_DISABLED;
    শূন্য unlinkDelete() VL_MT_DISABLED;
    শূন্য relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    শূন্য relink_শীর্ষ(V3GraphVertex* new_শীর্ষ) VL_MT_DISABLED;
    // ACCESSORS
    পূর্ণাঙ্ক weight() ধ্রুব { প্রত্যাগম m_weight; }
    শূন্য weight(পূর্ণাঙ্ক weight) { m_weight = weight; }
    bool cutable() ধ্রুব { প্রত্যাগম m_cutable; }
    শূন্য cutable(bool cutable) { m_cutable = cutable; }
    শূন্য userp(শূন্য* user) { m_userp = user; }
    শূন্য* userp() ধ্রুব { প্রত্যাগম m_userp; }
    শূন্য user(uint64_t user) { m_user = user; }
    uint64_t user() ধ্রুব { প্রত্যাগম m_user; }
    V3GraphVertex* fromp() ধ্রুব { প্রত্যাগম m_fromp; }
    V3GraphVertex* শীর্ষ() ধ্রুব { প্রত্যাগম m_শীর্ষ; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() ধ্রুব {
        প্রত্যাগম N_Way == GraphWay::FORWARD ? শীর্ষ() : fromp();
    }
    // STATIC ACCESSORS
    স্থির bool followNotCutable(ধ্রুব V3GraphEdge* edgep) { প্রত্যাগম !edgep->m_cutable; }
    স্থির bool followAlwaysTrue(ধ্রুব V3GraphEdge*) { প্রত্যাগম true; }
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
    দ্বিগুণ m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    সঙ্ঘ {
        শূন্য* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    শূন্য fanout(দ্বিগুণ fanout) { m_fanout = fanout; }

রক্ষিত:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, ধ্রুব V3GraphVertex& old) VL_MT_DISABLED;

গুপ্ত:
    V3ListLinks<V3GraphVertex>& links() { প্রত্যাগম m_links; }

সার্বজনিক:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    আভাস V3GraphVertex* clone(V3Graph* graphp) ধ্রুব VL_MT_DISABLED {
        প্রত্যাগম new V3GraphVertex{graphp, *this};
    }
    আভাস ~V3GraphVertex() = স্বভাব;
    শূন্য unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    শূন্য unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ধ্রুব {
        static_assert(std::is_base_of<V3GraphVertex, T>::মান,
                      "'T' must be a subtype of V3GraphVertex");
        প্রত্যাগম V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        প্রত্যাগম static_cast<T*>(this);
    }
    template <typename T>
    ধ্রুব T* as() ধ্রুব {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        প্রত্যাগম static_cast<ধ্রুব T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        প্রত্যাগম is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ধ্রুব T* cast() ধ্রুব {
        প্রত্যাগম is<T>() ? static_cast<ধ্রুব T*>(this) : nullptr;
    }

    // ACCESSORS
    আভাস string name() ধ্রুব { প্রত্যাগম ""; }
    আভাস string dotColor() ধ্রুব { প্রত্যাগম "black"; }
    আভাস string dotShape() ধ্রুব { প্রত্যাগম ""; }
    আভাস string dotStyle() ধ্রুব { প্রত্যাগম ""; }
    আভাস string dotName() ধ্রুব { প্রত্যাগম ""; }
    আভাস string dotRank() ধ্রুব { প্রত্যাগম ""; }
    আভাস uint32_t rankAdder() ধ্রুব { প্রত্যাগম 1; }
    আভাস File_সারি* fileline() ধ্রুব { প্রত্যাগম nullptr; }  // nullptr for unknown
    আভাস পূর্ণাঙ্ক sortCmp(ধ্রুব V3GraphVertex* rhsp) ধ্রুব {
        // LHS goes first if of lower rank, or lower fanout
        যদি (m_rank < rhsp->m_rank) প্রত্যাগম -1;
        যদি (m_rank > rhsp->m_rank) প্রত্যাগম 1;
        যদি (m_fanout < rhsp->m_fanout) প্রত্যাগম -1;
        যদি (m_fanout > rhsp->m_fanout) প্রত্যাগম 1;
        প্রত্যাগম 0;
    }
    uint32_t color() ধ্রুব { প্রত্যাগম m_color; }
    শূন্য color(uint32_t color) { m_color = color; }
    uint32_t rank() ধ্রুব { প্রত্যাগম m_rank; }
    শূন্য rank(uint32_t rank) { m_rank = rank; }
    দ্বিগুণ fanout() ধ্রুব { প্রত্যাগম m_fanout; }
    শূন্য user(uint32_t user) { m_user = user; }
    uint32_t user() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_user; }
    শূন্য userp(শূন্য* userp) { m_userp = userp; }
    শূন্য* userp() ধ্রুব VL_MT_STABLE { প্রত্যাগম m_userp; }
    V3GraphEdge::IList& inEdges() { প্রত্যাগম m_ins; }
    ধ্রুব V3GraphEdge::IList& inEdges() ধ্রুব { প্রত্যাগম m_ins; }
    template <GraphWay::en N_Way>
    অন্তঃস্থ auto& edges();
    template <GraphWay::en N_Way>
    অন্তঃস্থ ধ্রুব auto& edges() ধ্রুব;
    bool inEmpty() ধ্রুব { প্রত্যাগম m_ins.empty(); }
    bool inSize1() ধ্রুব { প্রত্যাগম m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { প্রত্যাগম m_outs; }
    ধ্রুব V3GraphEdge::OList& outEdges() ধ্রুব { প্রত্যাগম m_outs; }
    bool outEmpty() ধ্রুব { প্রত্যাগম m_outs.empty(); }
    bool outSize1() ধ্রুব { প্রত্যাগম m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    শূন্য v3errorEnd(ধ্রুব std::ostringstream& str) ধ্রুব
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    শূন্য v3errorEndFatal(ধ্রুব std::ostringstream& str) ধ্রুব
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    শূন্য rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
অন্তঃস্থ auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    প্রত্যাগম m_outs;
}

template <>
অন্তঃস্থ auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    প্রত্যাগম m_ins;
}

template <>
অন্তঃস্থ ধ্রুব auto& V3GraphVertex::edges<GraphWay::FORWARD>() ধ্রুব {
    প্রত্যাগম m_outs;
}

template <>
অন্তঃস্থ ধ্রুব auto& V3GraphVertex::edges<GraphWay::REVERSE>() ধ্রুব {
    প্রত্যাগম m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

রক্ষিত:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    দ্বিগুণ orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    শূন্য dumpEdge(std::ostream& os, ধ্রুব V3GraphVertex& vertex,
                  ধ্রুব V3GraphEdge& edge) ধ্রুব VL_MT_DISABLED;
    // ACCESSORS

সার্বজনিক:
    V3Graph() VL_MT_DISABLED;
    আভাস ~V3Graph() VL_MT_DISABLED;
    আভাস string dotRankDir() ধ্রুব { প্রত্যাগম "TB"; }  // rankdir for dot plotting

    // METHODS
    শূন্য clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() ধ্রুব { প্রত্যাগম m_vertices.empty(); }
    V3GraphVertex::List& vertices() { প্রত্যাগম m_vertices; }
    ধ্রুব V3GraphVertex::List& vertices() ধ্রুব { প্রত্যাগম m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    শূন্য stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    শূন্য rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    শূন্য rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    শূন্য rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    শূন্য rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    শূন্য sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    শূন্য sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    শূন্য order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    শূন্য orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    শূন্য acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    শূন্য removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    শূন্য removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    শূন্য removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    শূন্য subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    শূন্য userClearVertices() VL_MT_DISABLED;
    শূন্য userClearEdges() VL_MT_DISABLED;

    /// Debugging
    শূন্য dump(std::ostream& os = std::cout) ধ্রুব VL_MT_DISABLED;
    শূন্য dumpDotFile(ধ্রুব string& filename, bool colorAsSubgraph) ধ্রুব VL_MT_DISABLED;
    শূন্য dumpDotFilePrefixed(ধ্রুব string& nameComment,
                             bool colorAsSubgraph = false) ধ্রুব VL_MT_DISABLED;
    শূন্য dumpDotFilePrefixedAlways(ধ্রুব string& nameComment,
                                   bool colorAsSubgraph = false) ধ্রুব VL_MT_DISABLED;
    শূন্য dumpEdges(std::ostream& os, ধ্রুব V3GraphVertex& vertex) ধ্রুব VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    শূন্য hashGraphDebug(ধ্রুব বর্ণ* debugName) ধ্রুব VL_MT_DISABLED;

    স্থির শূন্য selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_গুনতি = 0;  // Number of vertexes in the graph
        uint64_t m_edge_গুনতি = 0;  // Number of edges in the grap

        ParallelismReport() = স্বভাব;

    সার্বজনিক:
        ~ParallelismReport() = স্বভাব;
        ParallelismReport(ধ্রুব ParallelismReport&) = স্বভাব;
        ParallelismReport& operator=(ধ্রুব ParallelismReport&) = স্বভাব;

        uint64_t totalGraphCost() ধ্রুব { প্রত্যাগম m_totalGraphCost; }
        uint64_t criticalPathCost() ধ্রুব { প্রত্যাগম m_criticalPathCost; }
        uint64_t vertex_গুনতি() ধ্রুব { প্রত্যাগম m_vertex_গুনতি; }
        uint64_t edge_গুনতি() ধ্রুব { প্রত্যাগম m_edge_গুনতি; }
        দ্বিগুণ parallelismFactor() ধ্রুব {
            প্রত্যাগম (static_cast<দ্বিগুণ>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ধ্রুব V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    আভাস শূন্য loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    আভাস string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
