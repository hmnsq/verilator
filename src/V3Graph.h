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

using V3EdgeFuncP = bool (*)(ধ্রুবক V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
সর্বজনীন:
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
    explicit constexpr GraphWay(পূর্ণসংখ্যা _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুবক { ফেরত m_e; }
    ধ্রুবক অক্ষর* ascii() ধ্রুবক {
        স্থির constexpr ধ্রুবক অক্ষর* ধ্রুবক names[] = {"FORWARD", "REVERSE"};
        ফেরত names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() ধ্রুবক { ফেরত GraphWay{m_e ^ 1}; }
    constexpr bool forward() ধ্রুবক { ফেরত m_e == FORWARD; }
    constexpr bool reverse() ধ্রুবক { ফেরত m_e != FORWARD; }
};
constexpr bool operator==(ধ্রুবক GraphWay& lhs, ধ্রুবক GraphWay& rhs) { ফেরত lhs.m_e == rhs.m_e; }
constexpr bool operator==(ধ্রুবক GraphWay& lhs, GraphWay::en rhs) { ফেরত lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, ধ্রুবক GraphWay& rhs) { ফেরত lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
সর্বজনীন:
    // ENUMS
    গণনা Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
সুরক্ষিত:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_শীর্ষ;  // Vertices this edge points to
    পূর্ণসংখ্যা m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    সংঘ {
        শূন্য* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    শূন্য init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণসংখ্যা weight,
              bool cutable = false) VL_MT_DISABLED;
    শূন্য cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ,
                ধ্রুবক V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, শীর্ষ, old.m_weight, old.m_cutable);
    }

ব্যক্তিগত:
    V3ListLinks<V3GraphEdge>& oLinks() { ফেরত m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { ফেরত m_iLinks; }

সর্বজনীন:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণসংখ্যা weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, শীর্ষ, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    আভাসী V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* শীর্ষ) ধ্রুবক VL_MT_DISABLED {
        ফেরত new V3GraphEdge{graphp, fromp, শীর্ষ, *this};
    }
    আভাসী ~V3GraphEdge() = পূর্বনির্ধারিত;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ধ্রুবক {
        static_assert(std::is_base_of<V3GraphEdge, T>::মান,
                      "'T' must be a subtype of V3GraphEdge");
        ফেরত V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ফেরত static_cast<T*>(this);
    }
    template <typename T>
    ধ্রুবক T* as() ধ্রুবক {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ফেরত static_cast<ধ্রুবক T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        ফেরত is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ধ্রুবক T* cast() ধ্রুবক {
        ফেরত is<T>() ? static_cast<ধ্রুবক T*>(this) : nullptr;
    }

    আভাসী string name() ধ্রুবক VL_MT_DISABLED;
    আভাসী string dotLabel() ধ্রুবক { ফেরত ""; }
    আভাসী string dotColor() ধ্রুবক { ফেরত cutable() ? "yellowGreen" : "red"; }
    আভাসী string dotStyle() ধ্রুবক { ফেরত cutable() ? "dashed" : ""; }
    আভাসী পূর্ণসংখ্যা sortCmp(ধ্রুবক V3GraphEdge* rhsp) ধ্রুবক VL_MT_DISABLED;
    শূন্য unlinkDelete() VL_MT_DISABLED;
    শূন্য relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    শূন্য relink_শীর্ষ(V3GraphVertex* new_শীর্ষ) VL_MT_DISABLED;
    // ACCESSORS
    পূর্ণসংখ্যা weight() ধ্রুবক { ফেরত m_weight; }
    শূন্য weight(পূর্ণসংখ্যা weight) { m_weight = weight; }
    bool cutable() ধ্রুবক { ফেরত m_cutable; }
    শূন্য cutable(bool cutable) { m_cutable = cutable; }
    শূন্য userp(শূন্য* user) { m_userp = user; }
    শূন্য* userp() ধ্রুবক { ফেরত m_userp; }
    শূন্য user(uint64_t user) { m_user = user; }
    uint64_t user() ধ্রুবক { ফেরত m_user; }
    V3GraphVertex* fromp() ধ্রুবক { ফেরত m_fromp; }
    V3GraphVertex* শীর্ষ() ধ্রুবক { ফেরত m_শীর্ষ; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() ধ্রুবক {
        ফেরত N_Way == GraphWay::FORWARD ? শীর্ষ() : fromp();
    }
    // STATIC ACCESSORS
    স্থির bool followNotCutable(ধ্রুবক V3GraphEdge* edgep) { ফেরত !edgep->m_cutable; }
    স্থির bool followAlwaysTrue(ধ্রুবক V3GraphEdge*) { ফেরত true; }
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
    সংঘ {
        শূন্য* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    শূন্য fanout(দ্বিগুণ fanout) { m_fanout = fanout; }

সুরক্ষিত:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, ধ্রুবক V3GraphVertex& old) VL_MT_DISABLED;

ব্যক্তিগত:
    V3ListLinks<V3GraphVertex>& links() { ফেরত m_links; }

সর্বজনীন:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    আভাসী V3GraphVertex* clone(V3Graph* graphp) ধ্রুবক VL_MT_DISABLED {
        ফেরত new V3GraphVertex{graphp, *this};
    }
    আভাসী ~V3GraphVertex() = পূর্বনির্ধারিত;
    শূন্য unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    শূন্য unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ধ্রুবক {
        static_assert(std::is_base_of<V3GraphVertex, T>::মান,
                      "'T' must be a subtype of V3GraphVertex");
        ফেরত V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ফেরত static_cast<T*>(this);
    }
    template <typename T>
    ধ্রুবক T* as() ধ্রুবক {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ফেরত static_cast<ধ্রুবক T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        ফেরত is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ধ্রুবক T* cast() ধ্রুবক {
        ফেরত is<T>() ? static_cast<ধ্রুবক T*>(this) : nullptr;
    }

    // ACCESSORS
    আভাসী string name() ধ্রুবক { ফেরত ""; }
    আভাসী string dotColor() ধ্রুবক { ফেরত "black"; }
    আভাসী string dotShape() ধ্রুবক { ফেরত ""; }
    আভাসী string dotStyle() ধ্রুবক { ফেরত ""; }
    আভাসী string dotName() ধ্রুবক { ফেরত ""; }
    আভাসী string dotRank() ধ্রুবক { ফেরত ""; }
    আভাসী uint32_t rankAdder() ধ্রুবক { ফেরত 1; }
    আভাসী File_সারি* fileline() ধ্রুবক { ফেরত nullptr; }  // nullptr for unknown
    আভাসী পূর্ণসংখ্যা sortCmp(ধ্রুবক V3GraphVertex* rhsp) ধ্রুবক {
        // LHS goes first if of lower rank, or lower fanout
        যদি (m_rank < rhsp->m_rank) ফেরত -1;
        যদি (m_rank > rhsp->m_rank) ফেরত 1;
        যদি (m_fanout < rhsp->m_fanout) ফেরত -1;
        যদি (m_fanout > rhsp->m_fanout) ফেরত 1;
        ফেরত 0;
    }
    uint32_t color() ধ্রুবক { ফেরত m_color; }
    শূন্য color(uint32_t color) { m_color = color; }
    uint32_t rank() ধ্রুবক { ফেরত m_rank; }
    শূন্য rank(uint32_t rank) { m_rank = rank; }
    দ্বিগুণ fanout() ধ্রুবক { ফেরত m_fanout; }
    শূন্য user(uint32_t user) { m_user = user; }
    uint32_t user() ধ্রুবক VL_MT_STABLE { ফেরত m_user; }
    শূন্য userp(শূন্য* userp) { m_userp = userp; }
    শূন্য* userp() ধ্রুবক VL_MT_STABLE { ফেরত m_userp; }
    V3GraphEdge::IList& inEdges() { ফেরত m_ins; }
    ধ্রুবক V3GraphEdge::IList& inEdges() ধ্রুবক { ফেরত m_ins; }
    template <GraphWay::en N_Way>
    অন্তঃরেখ auto& edges();
    template <GraphWay::en N_Way>
    অন্তঃরেখ ধ্রুবক auto& edges() ধ্রুবক;
    bool inEmpty() ধ্রুবক { ফেরত m_ins.empty(); }
    bool inSize1() ধ্রুবক { ফেরত m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { ফেরত m_outs; }
    ধ্রুবক V3GraphEdge::OList& outEdges() ধ্রুবক { ফেরত m_outs; }
    bool outEmpty() ধ্রুবক { ফেরত m_outs.empty(); }
    bool outSize1() ধ্রুবক { ফেরত m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    শূন্য v3errorEnd(ধ্রুবক std::ostringstream& str) ধ্রুবক
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    শূন্য v3errorEndFatal(ধ্রুবক std::ostringstream& str) ধ্রুবক
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    শূন্য rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
অন্তঃরেখ auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    ফেরত m_outs;
}

template <>
অন্তঃরেখ auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    ফেরত m_ins;
}

template <>
অন্তঃরেখ ধ্রুবক auto& V3GraphVertex::edges<GraphWay::FORWARD>() ধ্রুবক {
    ফেরত m_outs;
}

template <>
অন্তঃরেখ ধ্রুবক auto& V3GraphVertex::edges<GraphWay::REVERSE>() ধ্রুবক {
    ফেরত m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

সুরক্ষিত:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    দ্বিগুণ orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    শূন্য dumpEdge(std::ostream& os, ধ্রুবক V3GraphVertex& vertex,
                  ধ্রুবক V3GraphEdge& edge) ধ্রুবক VL_MT_DISABLED;
    // ACCESSORS

সর্বজনীন:
    V3Graph() VL_MT_DISABLED;
    আভাসী ~V3Graph() VL_MT_DISABLED;
    আভাসী string dotRankDir() ধ্রুবক { ফেরত "TB"; }  // rankdir for dot plotting

    // METHODS
    শূন্য clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() ধ্রুবক { ফেরত m_vertices.empty(); }
    V3GraphVertex::List& vertices() { ফেরত m_vertices; }
    ধ্রুবক V3GraphVertex::List& vertices() ধ্রুবক { ফেরত m_vertices; }

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
    শূন্য remove_Redundant_Edges_যোগ(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

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
    শূন্য dump(std::ostream& os = std::cout) ধ্রুবক VL_MT_DISABLED;
    শূন্য dumpDotFile(ধ্রুবক string& filename, bool colorAsSubgraph) ধ্রুবক VL_MT_DISABLED;
    শূন্য dumpDotFilePrefixed(ধ্রুবক string& nameComment,
                             bool colorAsSubgraph = false) ধ্রুবক VL_MT_DISABLED;
    শূন্য dumpDotFilePrefixedAlways(ধ্রুবক string& nameComment,
                                   bool colorAsSubgraph = false) ধ্রুবক VL_MT_DISABLED;
    শূন্য dumpEdges(std::ostream& os, ধ্রুবক V3GraphVertex& vertex) ধ্রুবক VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    শূন্য hashGraphDebug(ধ্রুবক অক্ষর* debugName) ধ্রুবক VL_MT_DISABLED;

    স্থির শূন্য selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_যোগ_Graph_Cost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_গুনতি = 0;  // Number of vertexes in the graph
        uint64_t m_edge_গুনতি = 0;  // Number of edges in the grap

        ParallelismReport() = পূর্বনির্ধারিত;

    সর্বজনীন:
        ~ParallelismReport() = পূর্বনির্ধারিত;
        ParallelismReport(ধ্রুবক ParallelismReport&) = পূর্বনির্ধারিত;
        ParallelismReport& operator=(ধ্রুবক ParallelismReport&) = পূর্বনির্ধারিত;

        uint64_t যোগ_Graph_Cost() ধ্রুবক { ফেরত m_যোগ_Graph_Cost; }
        uint64_t criticalPathCost() ধ্রুবক { ফেরত m_criticalPathCost; }
        uint64_t vertex_গুনতি() ধ্রুবক { ফেরত m_vertex_গুনতি; }
        uint64_t edge_গুনতি() ধ্রুবক { ফেরত m_edge_গুনতি; }
        দ্বিগুণ parallelismFactor() ধ্রুবক {
            ফেরত (static_cast<দ্বিগুণ>(m_যোগ_Graph_Cost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ধ্রুবক V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    আভাসী শূন্য loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    আভাসী string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
