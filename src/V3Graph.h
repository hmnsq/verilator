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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3List.h"
#شامل "V3Rtti.h"

#شامل <algorithm>
#شامل <functional>

class File_سطر;
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
عمومی:
    شمارش en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        رقم_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    شمارش en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"FORWARD", "REVERSE"};
        بازگشت names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() ثابت { بازگشت GraphWay{m_e ^ 1}; }
    constexpr bool forward() ثابت { بازگشت m_e == FORWARD; }
    constexpr bool reverse() ثابت { بازگشت m_e != FORWARD; }
};
constexpr bool operator==(ثابت GraphWay& lhs, ثابت GraphWay& rhs) { بازگشت lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت GraphWay& lhs, GraphWay::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, ثابت GraphWay& rhs) { بازگشت lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
عمومی:
    // ENUMS
    شمارش Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
محافظتشده:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_بالاترین;  // Vertices this edge points to
    صحیح m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    اجتماع {
        تهی* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    تهی init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight,
              bool cutable = false) VL_MT_DISABLED;
    تهی cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین,
                ثابت V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, بالاترین, old.m_weight, old.m_cutable);
    }

خصوصی:
    V3ListLinks<V3GraphEdge>& oLinks() { بازگشت m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { بازگشت m_iLinks; }

عمومی:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, بالاترین, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    مجازی V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* بالاترین) ثابت VL_MT_DISABLED {
        بازگشت new V3GraphEdge{graphp, fromp, بالاترین, *this};
    }
    مجازی ~V3GraphEdge() = پیشفرض;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ثابت {
        static_assert(std::is_base_of<V3GraphEdge, T>::مقدار,
                      "'T' must be a subtype of V3GraphEdge");
        بازگشت V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        بازگشت static_cast<T*>(this);
    }
    template <typename T>
    ثابت T* as() ثابت {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        بازگشت static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        بازگشت is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ثابت T* cast() ثابت {
        بازگشت is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    مجازی string name() ثابت VL_MT_DISABLED;
    مجازی string dotLabel() ثابت { بازگشت ""; }
    مجازی string dotColor() ثابت { بازگشت cutable() ? "yellowGreen" : "red"; }
    مجازی string dotStyle() ثابت { بازگشت cutable() ? "dashed" : ""; }
    مجازی صحیح sortCmp(ثابت V3GraphEdge* rhsp) ثابت VL_MT_DISABLED;
    تهی unlinkDelete() VL_MT_DISABLED;
    تهی relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    تهی relink_بالاترین(V3GraphVertex* new_بالاترین) VL_MT_DISABLED;
    // ACCESSORS
    صحیح weight() ثابت { بازگشت m_weight; }
    تهی weight(صحیح weight) { m_weight = weight; }
    bool cutable() ثابت { بازگشت m_cutable; }
    تهی cutable(bool cutable) { m_cutable = cutable; }
    تهی userp(تهی* user) { m_userp = user; }
    تهی* userp() ثابت { بازگشت m_userp; }
    تهی user(uint64_t user) { m_user = user; }
    uint64_t user() ثابت { بازگشت m_user; }
    V3GraphVertex* fromp() ثابت { بازگشت m_fromp; }
    V3GraphVertex* بالاترین() ثابت { بازگشت m_بالاترین; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() ثابت {
        بازگشت N_Way == GraphWay::FORWARD ? بالاترین() : fromp();
    }
    // STATIC ACCESSORS
    ایستا bool followNotCutable(ثابت V3GraphEdge* edgep) { بازگشت !edgep->m_cutable; }
    ایستا bool followAlwaysTrue(ثابت V3GraphEdge*) { بازگشت true; }
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
    دوگانه m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    اجتماع {
        تهی* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    تهی fanout(دوگانه fanout) { m_fanout = fanout; }

محافظتشده:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, ثابت V3GraphVertex& old) VL_MT_DISABLED;

خصوصی:
    V3ListLinks<V3GraphVertex>& links() { بازگشت m_links; }

عمومی:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    مجازی V3GraphVertex* clone(V3Graph* graphp) ثابت VL_MT_DISABLED {
        بازگشت new V3GraphVertex{graphp, *this};
    }
    مجازی ~V3GraphVertex() = پیشفرض;
    تهی unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    تهی unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ثابت {
        static_assert(std::is_base_of<V3GraphVertex, T>::مقدار,
                      "'T' must be a subtype of V3GraphVertex");
        بازگشت V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        بازگشت static_cast<T*>(this);
    }
    template <typename T>
    ثابت T* as() ثابت {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        بازگشت static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        بازگشت is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ثابت T* cast() ثابت {
        بازگشت is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    // ACCESSORS
    مجازی string name() ثابت { بازگشت ""; }
    مجازی string dotColor() ثابت { بازگشت "black"; }
    مجازی string dotShape() ثابت { بازگشت ""; }
    مجازی string dotStyle() ثابت { بازگشت ""; }
    مجازی string dotName() ثابت { بازگشت ""; }
    مجازی string dotRank() ثابت { بازگشت ""; }
    مجازی uint32_t rankAdder() ثابت { بازگشت 1; }
    مجازی File_سطر* fileline() ثابت { بازگشت nullptr; }  // nullptr for unknown
    مجازی صحیح sortCmp(ثابت V3GraphVertex* rhsp) ثابت {
        // LHS goes first if of lower rank, or lower fanout
        اگر (m_rank < rhsp->m_rank) بازگشت -1;
        اگر (m_rank > rhsp->m_rank) بازگشت 1;
        اگر (m_fanout < rhsp->m_fanout) بازگشت -1;
        اگر (m_fanout > rhsp->m_fanout) بازگشت 1;
        بازگشت 0;
    }
    uint32_t color() ثابت { بازگشت m_color; }
    تهی color(uint32_t color) { m_color = color; }
    uint32_t rank() ثابت { بازگشت m_rank; }
    تهی rank(uint32_t rank) { m_rank = rank; }
    دوگانه fanout() ثابت { بازگشت m_fanout; }
    تهی user(uint32_t user) { m_user = user; }
    uint32_t user() ثابت VL_MT_STABLE { بازگشت m_user; }
    تهی userp(تهی* userp) { m_userp = userp; }
    تهی* userp() ثابت VL_MT_STABLE { بازگشت m_userp; }
    V3GraphEdge::IList& inEdges() { بازگشت m_ins; }
    ثابت V3GraphEdge::IList& inEdges() ثابت { بازگشت m_ins; }
    template <GraphWay::en N_Way>
    درونخطی auto& edges();
    template <GraphWay::en N_Way>
    درونخطی ثابت auto& edges() ثابت;
    bool inEmpty() ثابت { بازگشت m_ins.empty(); }
    bool inSize1() ثابت { بازگشت m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { بازگشت m_outs; }
    ثابت V3GraphEdge::OList& outEdges() ثابت { بازگشت m_outs; }
    bool outEmpty() ثابت { بازگشت m_outs.empty(); }
    bool outSize1() ثابت { بازگشت m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    تهی v3errorEnd(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    تهی v3errorEndFatal(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    تهی rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
درونخطی auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    بازگشت m_outs;
}

template <>
درونخطی auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    بازگشت m_ins;
}

template <>
درونخطی ثابت auto& V3GraphVertex::edges<GraphWay::FORWARD>() ثابت {
    بازگشت m_outs;
}

template <>
درونخطی ثابت auto& V3GraphVertex::edges<GraphWay::REVERSE>() ثابت {
    بازگشت m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

محافظتشده:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    دوگانه orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    تهی dumpEdge(std::ostream& os, ثابت V3GraphVertex& vertex,
                  ثابت V3GraphEdge& edge) ثابت VL_MT_DISABLED;
    // ACCESSORS

عمومی:
    V3Graph() VL_MT_DISABLED;
    مجازی ~V3Graph() VL_MT_DISABLED;
    مجازی string dotRankDir() ثابت { بازگشت "TB"; }  // rankdir for dot plotting

    // METHODS
    تهی clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() ثابت { بازگشت m_vertices.empty(); }
    V3GraphVertex::List& vertices() { بازگشت m_vertices; }
    ثابت V3GraphVertex::List& vertices() ثابت { بازگشت m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    تهی stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    تهی rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    تهی rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    تهی rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    تهی rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    تهی sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    تهی sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    تهی order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    تهی orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    تهی acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    تهی removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    تهی removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    تهی removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    تهی subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    تهی userClearVertices() VL_MT_DISABLED;
    تهی userClearEdges() VL_MT_DISABLED;

    /// Debugging
    تهی dump(std::ostream& os = std::cout) ثابت VL_MT_DISABLED;
    تهی dumpDotFile(ثابت string& filename, bool colorAsSubgraph) ثابت VL_MT_DISABLED;
    تهی dumpDotFilePrefixed(ثابت string& nameComment,
                             bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    تهی dumpDotFilePrefixedAlways(ثابت string& nameComment,
                                   bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    تهی dumpEdges(std::ostream& os, ثابت V3GraphVertex& vertex) ثابت VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    تهی hashGraphDebug(ثابت نویسه* debugName) ثابت VL_MT_DISABLED;

    ایستا تهی selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_شمار = 0;  // Number of vertexes in the graph
        uint64_t m_edge_شمار = 0;  // Number of edges in the grap

        ParallelismReport() = پیشفرض;

    عمومی:
        ~ParallelismReport() = پیشفرض;
        ParallelismReport(ثابت ParallelismReport&) = پیشفرض;
        ParallelismReport& operator=(ثابت ParallelismReport&) = پیشفرض;

        uint64_t totalGraphCost() ثابت { بازگشت m_totalGraphCost; }
        uint64_t criticalPathCost() ثابت { بازگشت m_criticalPathCost; }
        uint64_t vertex_شمار() ثابت { بازگشت m_vertex_شمار; }
        uint64_t edge_شمار() ثابت { بازگشت m_edge_شمار; }
        دوگانه parallelismFactor() ثابت {
            بازگشت (static_cast<دوگانه>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ثابت V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    مجازی تهی loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    مجازی string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
