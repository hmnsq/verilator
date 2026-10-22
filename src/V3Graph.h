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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3List.h"
#அந்தர்பாவ "V3Rtti.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <functional>

class File_வரி;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(மாறிலி V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
பொது:
    எண்ணிக்கை en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        இலக்கம்_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    எண்ணிக்கை en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(முழு _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() மாறிலி { திரும்பு m_e; }
    மாறிலி எழுத்து* ascii() மாறிலி {
        நிலையான constexpr மாறிலி எழுத்து* மாறிலி names[] = {"FORWARD", "REVERSE"};
        திரும்பு names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() மாறிலி { திரும்பு GraphWay{m_e ^ 1}; }
    constexpr bool forward() மாறிலி { திரும்பு m_e == FORWARD; }
    constexpr bool reverse() மாறிலி { திரும்பு m_e != FORWARD; }
};
constexpr bool operator==(மாறிலி GraphWay& lhs, மாறிலி GraphWay& rhs) { திரும்பு lhs.m_e == rhs.m_e; }
constexpr bool operator==(மாறிலி GraphWay& lhs, GraphWay::en rhs) { திரும்பு lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, மாறிலி GraphWay& rhs) { திரும்பு lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
பொது:
    // ENUMS
    எண்ணிக்கை Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
பாதுகாக்கப்பட்ட:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_உச்சி;  // Vertices this edge points to
    முழு m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    ஒன்றிப்பு {
        வெறுமை* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    வெறுமை init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight,
              bool cutable = false) VL_MT_DISABLED;
    வெறுமை cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி,
                மாறிலி V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, உச்சி, old.m_weight, old.m_cutable);
    }

தனிப்பட்ட:
    V3ListLinks<V3GraphEdge>& oLinks() { திரும்பு m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { திரும்பு m_iLinks; }

பொது:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, உச்சி, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    மெய்நிகர் V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* உச்சி) மாறிலி VL_MT_DISABLED {
        திரும்பு new V3GraphEdge{graphp, fromp, உச்சி, *this};
    }
    மெய்நிகர் ~V3GraphEdge() = இயல்பு;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() மாறிலி {
        static_assert(std::is_base_of<V3GraphEdge, T>::மதிப்பு,
                      "'T' must be a subtype of V3GraphEdge");
        திரும்பு V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        திரும்பு static_cast<T*>(this);
    }
    template <typename T>
    மாறிலி T* as() மாறிலி {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        திரும்பு static_cast<மாறிலி T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        திரும்பு is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    மாறிலி T* cast() மாறிலி {
        திரும்பு is<T>() ? static_cast<மாறிலி T*>(this) : nullptr;
    }

    மெய்நிகர் string name() மாறிலி VL_MT_DISABLED;
    மெய்நிகர் string dotLabel() மாறிலி { திரும்பு ""; }
    மெய்நிகர் string dotColor() மாறிலி { திரும்பு cutable() ? "yellowGreen" : "red"; }
    மெய்நிகர் string dotStyle() மாறிலி { திரும்பு cutable() ? "dashed" : ""; }
    மெய்நிகர் முழு sortCmp(மாறிலி V3GraphEdge* rhsp) மாறிலி VL_MT_DISABLED;
    வெறுமை unlinkDelete() VL_MT_DISABLED;
    வெறுமை relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    வெறுமை relink_உச்சி(V3GraphVertex* new_உச்சி) VL_MT_DISABLED;
    // ACCESSORS
    முழு weight() மாறிலி { திரும்பு m_weight; }
    வெறுமை weight(முழு weight) { m_weight = weight; }
    bool cutable() மாறிலி { திரும்பு m_cutable; }
    வெறுமை cutable(bool cutable) { m_cutable = cutable; }
    வெறுமை userp(வெறுமை* user) { m_userp = user; }
    வெறுமை* userp() மாறிலி { திரும்பு m_userp; }
    வெறுமை user(uint64_t user) { m_user = user; }
    uint64_t user() மாறிலி { திரும்பு m_user; }
    V3GraphVertex* fromp() மாறிலி { திரும்பு m_fromp; }
    V3GraphVertex* உச்சி() மாறிலி { திரும்பு m_உச்சி; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() மாறிலி {
        திரும்பு N_Way == GraphWay::FORWARD ? உச்சி() : fromp();
    }
    // STATIC ACCESSORS
    நிலையான bool followNotCutable(மாறிலி V3GraphEdge* edgep) { திரும்பு !edgep->m_cutable; }
    நிலையான bool followAlwaysTrue(மாறிலி V3GraphEdge*) { திரும்பு true; }
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
    இரட்டை m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    ஒன்றிப்பு {
        வெறுமை* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    வெறுமை fanout(இரட்டை fanout) { m_fanout = fanout; }

பாதுகாக்கப்பட்ட:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, மாறிலி V3GraphVertex& old) VL_MT_DISABLED;

தனிப்பட்ட:
    V3ListLinks<V3GraphVertex>& links() { திரும்பு m_links; }

பொது:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    மெய்நிகர் V3GraphVertex* clone(V3Graph* graphp) மாறிலி VL_MT_DISABLED {
        திரும்பு new V3GraphVertex{graphp, *this};
    }
    மெய்நிகர் ~V3GraphVertex() = இயல்பு;
    வெறுமை unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    வெறுமை unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() மாறிலி {
        static_assert(std::is_base_of<V3GraphVertex, T>::மதிப்பு,
                      "'T' must be a subtype of V3GraphVertex");
        திரும்பு V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        திரும்பு static_cast<T*>(this);
    }
    template <typename T>
    மாறிலி T* as() மாறிலி {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        திரும்பு static_cast<மாறிலி T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        திரும்பு is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    மாறிலி T* cast() மாறிலி {
        திரும்பு is<T>() ? static_cast<மாறிலி T*>(this) : nullptr;
    }

    // ACCESSORS
    மெய்நிகர் string name() மாறிலி { திரும்பு ""; }
    மெய்நிகர் string dotColor() மாறிலி { திரும்பு "black"; }
    மெய்நிகர் string dotShape() மாறிலி { திரும்பு ""; }
    மெய்நிகர் string dotStyle() மாறிலி { திரும்பு ""; }
    மெய்நிகர் string dotName() மாறிலி { திரும்பு ""; }
    மெய்நிகர் string dotRank() மாறிலி { திரும்பு ""; }
    மெய்நிகர் uint32_t rankAdder() மாறிலி { திரும்பு 1; }
    மெய்நிகர் File_வரி* fileline() மாறிலி { திரும்பு nullptr; }  // nullptr for unknown
    மெய்நிகர் முழு sortCmp(மாறிலி V3GraphVertex* rhsp) மாறிலி {
        // LHS goes first if of lower rank, or lower fanout
        எனில் (m_rank < rhsp->m_rank) திரும்பு -1;
        எனில் (m_rank > rhsp->m_rank) திரும்பு 1;
        எனில் (m_fanout < rhsp->m_fanout) திரும்பு -1;
        எனில் (m_fanout > rhsp->m_fanout) திரும்பு 1;
        திரும்பு 0;
    }
    uint32_t color() மாறிலி { திரும்பு m_color; }
    வெறுமை color(uint32_t color) { m_color = color; }
    uint32_t rank() மாறிலி { திரும்பு m_rank; }
    வெறுமை rank(uint32_t rank) { m_rank = rank; }
    இரட்டை fanout() மாறிலி { திரும்பு m_fanout; }
    வெறுமை user(uint32_t user) { m_user = user; }
    uint32_t user() மாறிலி VL_MT_STABLE { திரும்பு m_user; }
    வெறுமை userp(வெறுமை* userp) { m_userp = userp; }
    வெறுமை* userp() மாறிலி VL_MT_STABLE { திரும்பு m_userp; }
    V3GraphEdge::IList& inEdges() { திரும்பு m_ins; }
    மாறிலி V3GraphEdge::IList& inEdges() மாறிலி { திரும்பு m_ins; }
    template <GraphWay::en N_Way>
    உள்வரி auto& edges();
    template <GraphWay::en N_Way>
    உள்வரி மாறிலி auto& edges() மாறிலி;
    bool inEmpty() மாறிலி { திரும்பு m_ins.empty(); }
    bool inSize1() மாறிலி { திரும்பு m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { திரும்பு m_outs; }
    மாறிலி V3GraphEdge::OList& outEdges() மாறிலி { திரும்பு m_outs; }
    bool outEmpty() மாறிலி { திரும்பு m_outs.empty(); }
    bool outSize1() மாறிலி { திரும்பு m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    வெறுமை v3errorEnd(மாறிலி std::ostringstream& str) மாறிலி
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    வெறுமை v3errorEndFatal(மாறிலி std::ostringstream& str) மாறிலி
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    வெறுமை rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
உள்வரி auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    திரும்பு m_outs;
}

template <>
உள்வரி auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    திரும்பு m_ins;
}

template <>
உள்வரி மாறிலி auto& V3GraphVertex::edges<GraphWay::FORWARD>() மாறிலி {
    திரும்பு m_outs;
}

template <>
உள்வரி மாறிலி auto& V3GraphVertex::edges<GraphWay::REVERSE>() மாறிலி {
    திரும்பு m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

பாதுகாக்கப்பட்ட:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    இரட்டை orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    வெறுமை dumpEdge(std::ostream& os, மாறிலி V3GraphVertex& vertex,
                  மாறிலி V3GraphEdge& edge) மாறிலி VL_MT_DISABLED;
    // ACCESSORS

பொது:
    V3Graph() VL_MT_DISABLED;
    மெய்நிகர் ~V3Graph() VL_MT_DISABLED;
    மெய்நிகர் string dotRankDir() மாறிலி { திரும்பு "TB"; }  // rankdir for dot plotting

    // METHODS
    வெறுமை clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() மாறிலி { திரும்பு m_vertices.empty(); }
    V3GraphVertex::List& vertices() { திரும்பு m_vertices; }
    மாறிலி V3GraphVertex::List& vertices() மாறிலி { திரும்பு m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    வெறுமை stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    வெறுமை rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    வெறுமை rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    வெறுமை rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    வெறுமை rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    வெறுமை sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    வெறுமை sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    வெறுமை order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    வெறுமை orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    வெறுமை acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    வெறுமை removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    வெறுமை removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    வெறுமை removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    வெறுமை subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    வெறுமை userClearVertices() VL_MT_DISABLED;
    வெறுமை userClearEdges() VL_MT_DISABLED;

    /// Debugging
    வெறுமை dump(std::ostream& os = std::cout) மாறிலி VL_MT_DISABLED;
    வெறுமை dumpDotFile(மாறிலி string& filename, bool colorAsSubgraph) மாறிலி VL_MT_DISABLED;
    வெறுமை dumpDotFilePrefixed(மாறிலி string& nameComment,
                             bool colorAsSubgraph = false) மாறிலி VL_MT_DISABLED;
    வெறுமை dumpDotFilePrefixedAlways(மாறிலி string& nameComment,
                                   bool colorAsSubgraph = false) மாறிலி VL_MT_DISABLED;
    வெறுமை dumpEdges(std::ostream& os, மாறிலி V3GraphVertex& vertex) மாறிலி VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    வெறுமை hashGraphDebug(மாறிலி எழுத்து* debugName) மாறிலி VL_MT_DISABLED;

    நிலையான வெறுமை selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_மொத்தம் = 0;  // Number of vertexes in the graph
        uint64_t m_edge_மொத்தம் = 0;  // Number of edges in the grap

        ParallelismReport() = இயல்பு;

    பொது:
        ~ParallelismReport() = இயல்பு;
        ParallelismReport(மாறிலி ParallelismReport&) = இயல்பு;
        ParallelismReport& operator=(மாறிலி ParallelismReport&) = இயல்பு;

        uint64_t totalGraphCost() மாறிலி { திரும்பு m_totalGraphCost; }
        uint64_t criticalPathCost() மாறிலி { திரும்பு m_criticalPathCost; }
        uint64_t vertex_மொத்தம்() மாறிலி { திரும்பு m_vertex_மொத்தம்; }
        uint64_t edge_மொத்தம்() மாறிலி { திரும்பு m_edge_மொத்தம்; }
        இரட்டை parallelismFactor() மாறிலி {
            திரும்பு (static_cast<இரட்டை>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(மாறிலி V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    மெய்நிகர் வெறுமை loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    மெய்நிகர் string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
