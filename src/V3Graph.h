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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3List.h"
#अन्तर्भाव "V3Rtti.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <functional>

class File_पङ्क्तिः;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(ध्रुव V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
सार्वजनिक:
    गणना en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        अङ्कः_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    गणना en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"FORWARD", "REVERSE"};
        प्रत्यागम names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() ध्रुव { प्रत्यागम GraphWay{m_e ^ 1}; }
    constexpr bool forward() ध्रुव { प्रत्यागम m_e == FORWARD; }
    constexpr bool reverse() ध्रुव { प्रत्यागम m_e != FORWARD; }
};
constexpr bool operator==(ध्रुव GraphWay& lhs, ध्रुव GraphWay& rhs) { प्रत्यागम lhs.m_e == rhs.m_e; }
constexpr bool operator==(ध्रुव GraphWay& lhs, GraphWay::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, ध्रुव GraphWay& rhs) { प्रत्यागम lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
सार्वजनिक:
    // ENUMS
    गणना Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
रक्षित:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_शिखरम्;  // Vertices this edge points to
    पूर्णाङ्क m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    सङ्घ {
        शून्य* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    शून्य init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्, पूर्णाङ्क weight,
              bool cutable = false) VL_MT_DISABLED;
    शून्य cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्,
                ध्रुव V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, शिखरम्, old.m_weight, old.m_cutable);
    }

गुप्त:
    V3ListLinks<V3GraphEdge>& oLinks() { प्रत्यागम m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { प्रत्यागम m_iLinks; }

सार्वजनिक:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्, पूर्णाङ्क weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, शिखरम्, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    आभास V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* शिखरम्) ध्रुव VL_MT_DISABLED {
        प्रत्यागम new V3GraphEdge{graphp, fromp, शिखरम्, *this};
    }
    आभास ~V3GraphEdge() = स्वभाव;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ध्रुव {
        static_assert(std::is_base_of<V3GraphEdge, T>::मानम्,
                      "'T' must be a subtype of V3GraphEdge");
        प्रत्यागम V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        प्रत्यागम static_cast<T*>(this);
    }
    template <typename T>
    ध्रुव T* as() ध्रुव {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        प्रत्यागम static_cast<ध्रुव T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        प्रत्यागम is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ध्रुव T* cast() ध्रुव {
        प्रत्यागम is<T>() ? static_cast<ध्रुव T*>(this) : nullptr;
    }

    आभास string name() ध्रुव VL_MT_DISABLED;
    आभास string dotLabel() ध्रुव { प्रत्यागम ""; }
    आभास string dotColor() ध्रुव { प्रत्यागम cutable() ? "yellowGreen" : "red"; }
    आभास string dotStyle() ध्रुव { प्रत्यागम cutable() ? "dashed" : ""; }
    आभास पूर्णाङ्क sortCmp(ध्रुव V3GraphEdge* rhsp) ध्रुव VL_MT_DISABLED;
    शून्य unlinkDelete() VL_MT_DISABLED;
    शून्य relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    शून्य relink_शिखरम्(V3GraphVertex* new_शिखरम्) VL_MT_DISABLED;
    // ACCESSORS
    पूर्णाङ्क weight() ध्रुव { प्रत्यागम m_weight; }
    शून्य weight(पूर्णाङ्क weight) { m_weight = weight; }
    bool cutable() ध्रुव { प्रत्यागम m_cutable; }
    शून्य cutable(bool cutable) { m_cutable = cutable; }
    शून्य userp(शून्य* user) { m_userp = user; }
    शून्य* userp() ध्रुव { प्रत्यागम m_userp; }
    शून्य user(uint64_t user) { m_user = user; }
    uint64_t user() ध्रुव { प्रत्यागम m_user; }
    V3GraphVertex* fromp() ध्रुव { प्रत्यागम m_fromp; }
    V3GraphVertex* शिखरम्() ध्रुव { प्रत्यागम m_शिखरम्; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() ध्रुव {
        प्रत्यागम N_Way == GraphWay::FORWARD ? शिखरम्() : fromp();
    }
    // STATIC ACCESSORS
    स्थिर bool followNotCutable(ध्रुव V3GraphEdge* edgep) { प्रत्यागम !edgep->m_cutable; }
    स्थिर bool followAlwaysTrue(ध्रुव V3GraphEdge*) { प्रत्यागम true; }
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
    द्विगुण m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    सङ्घ {
        शून्य* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    शून्य fanout(द्विगुण fanout) { m_fanout = fanout; }

रक्षित:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, ध्रुव V3GraphVertex& old) VL_MT_DISABLED;

गुप्त:
    V3ListLinks<V3GraphVertex>& links() { प्रत्यागम m_links; }

सार्वजनिक:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    आभास V3GraphVertex* clone(V3Graph* graphp) ध्रुव VL_MT_DISABLED {
        प्रत्यागम new V3GraphVertex{graphp, *this};
    }
    आभास ~V3GraphVertex() = स्वभाव;
    शून्य unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    शून्य unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ध्रुव {
        static_assert(std::is_base_of<V3GraphVertex, T>::मानम्,
                      "'T' must be a subtype of V3GraphVertex");
        प्रत्यागम V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        प्रत्यागम static_cast<T*>(this);
    }
    template <typename T>
    ध्रुव T* as() ध्रुव {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        प्रत्यागम static_cast<ध्रुव T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        प्रत्यागम is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ध्रुव T* cast() ध्रुव {
        प्रत्यागम is<T>() ? static_cast<ध्रुव T*>(this) : nullptr;
    }

    // ACCESSORS
    आभास string name() ध्रुव { प्रत्यागम ""; }
    आभास string dotColor() ध्रुव { प्रत्यागम "black"; }
    आभास string dotShape() ध्रुव { प्रत्यागम ""; }
    आभास string dotStyle() ध्रुव { प्रत्यागम ""; }
    आभास string dotName() ध्रुव { प्रत्यागम ""; }
    आभास string dotRank() ध्रुव { प्रत्यागम ""; }
    आभास uint32_t rankAdder() ध्रुव { प्रत्यागम 1; }
    आभास File_पङ्क्तिः* fileline() ध्रुव { प्रत्यागम nullptr; }  // nullptr for unknown
    आभास पूर्णाङ्क sortCmp(ध्रुव V3GraphVertex* rhsp) ध्रुव {
        // LHS goes first if of lower rank, or lower fanout
        जइ (m_rank < rhsp->m_rank) प्रत्यागम -1;
        जइ (m_rank > rhsp->m_rank) प्रत्यागम 1;
        जइ (m_fanout < rhsp->m_fanout) प्रत्यागम -1;
        जइ (m_fanout > rhsp->m_fanout) प्रत्यागम 1;
        प्रत्यागम 0;
    }
    uint32_t color() ध्रुव { प्रत्यागम m_color; }
    शून्य color(uint32_t color) { m_color = color; }
    uint32_t rank() ध्रुव { प्रत्यागम m_rank; }
    शून्य rank(uint32_t rank) { m_rank = rank; }
    द्विगुण fanout() ध्रुव { प्रत्यागम m_fanout; }
    शून्य user(uint32_t user) { m_user = user; }
    uint32_t user() ध्रुव VL_MT_STABLE { प्रत्यागम m_user; }
    शून्य userp(शून्य* userp) { m_userp = userp; }
    शून्य* userp() ध्रुव VL_MT_STABLE { प्रत्यागम m_userp; }
    V3GraphEdge::IList& inEdges() { प्रत्यागम m_ins; }
    ध्रुव V3GraphEdge::IList& inEdges() ध्रुव { प्रत्यागम m_ins; }
    template <GraphWay::en N_Way>
    अन्तःस्थ auto& edges();
    template <GraphWay::en N_Way>
    अन्तःस्थ ध्रुव auto& edges() ध्रुव;
    bool inEmpty() ध्रुव { प्रत्यागम m_ins.empty(); }
    bool inSize1() ध्रुव { प्रत्यागम m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { प्रत्यागम m_outs; }
    ध्रुव V3GraphEdge::OList& outEdges() ध्रुव { प्रत्यागम m_outs; }
    bool outEmpty() ध्रुव { प्रत्यागम m_outs.empty(); }
    bool outSize1() ध्रुव { प्रत्यागम m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    शून्य v3errorEnd(ध्रुव std::ostringstream& str) ध्रुव
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    शून्य v3errorEndFatal(ध्रुव std::ostringstream& str) ध्रुव
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    शून्य rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
अन्तःस्थ auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    प्रत्यागम m_outs;
}

template <>
अन्तःस्थ auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    प्रत्यागम m_ins;
}

template <>
अन्तःस्थ ध्रुव auto& V3GraphVertex::edges<GraphWay::FORWARD>() ध्रुव {
    प्रत्यागम m_outs;
}

template <>
अन्तःस्थ ध्रुव auto& V3GraphVertex::edges<GraphWay::REVERSE>() ध्रुव {
    प्रत्यागम m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

रक्षित:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    द्विगुण orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    शून्य dumpEdge(std::ostream& os, ध्रुव V3GraphVertex& vertex,
                  ध्रुव V3GraphEdge& edge) ध्रुव VL_MT_DISABLED;
    // ACCESSORS

सार्वजनिक:
    V3Graph() VL_MT_DISABLED;
    आभास ~V3Graph() VL_MT_DISABLED;
    आभास string dotRankDir() ध्रुव { प्रत्यागम "TB"; }  // rankdir for dot plotting

    // METHODS
    शून्य clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() ध्रुव { प्रत्यागम m_vertices.empty(); }
    V3GraphVertex::List& vertices() { प्रत्यागम m_vertices; }
    ध्रुव V3GraphVertex::List& vertices() ध्रुव { प्रत्यागम m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    शून्य stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    शून्य rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    शून्य rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    शून्य rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    शून्य rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    शून्य sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    शून्य sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    शून्य order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    शून्य orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    शून्य acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    शून्य removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    शून्य removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    शून्य removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    शून्य subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    शून्य userClearVertices() VL_MT_DISABLED;
    शून्य userClearEdges() VL_MT_DISABLED;

    /// Debugging
    शून्य dump(std::ostream& os = std::cout) ध्रुव VL_MT_DISABLED;
    शून्य dumpDotFile(ध्रुव string& filename, bool colorAsSubgraph) ध्रुव VL_MT_DISABLED;
    शून्य dumpDotFilePrefixed(ध्रुव string& nameComment,
                             bool colorAsSubgraph = false) ध्रुव VL_MT_DISABLED;
    शून्य dumpDotFilePrefixedAlways(ध्रुव string& nameComment,
                                   bool colorAsSubgraph = false) ध्रुव VL_MT_DISABLED;
    शून्य dumpEdges(std::ostream& os, ध्रुव V3GraphVertex& vertex) ध्रुव VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    शून्य hashGraphDebug(ध्रुव वर्ण* debugName) ध्रुव VL_MT_DISABLED;

    स्थिर शून्य selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_गणनम् = 0;  // Number of vertexes in the graph
        uint64_t m_edge_गणनम् = 0;  // Number of edges in the grap

        ParallelismReport() = स्वभाव;

    सार्वजनिक:
        ~ParallelismReport() = स्वभाव;
        ParallelismReport(ध्रुव ParallelismReport&) = स्वभाव;
        ParallelismReport& operator=(ध्रुव ParallelismReport&) = स्वभाव;

        uint64_t totalGraphCost() ध्रुव { प्रत्यागम m_totalGraphCost; }
        uint64_t criticalPathCost() ध्रुव { प्रत्यागम m_criticalPathCost; }
        uint64_t vertex_गणनम्() ध्रुव { प्रत्यागम m_vertex_गणनम्; }
        uint64_t edge_गणनम्() ध्रुव { प्रत्यागम m_edge_गणनम्; }
        द्विगुण parallelismFactor() ध्रुव {
            प्रत्यागम (static_cast<द्विगुण>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ध्रुव V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    आभास शून्य loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    आभास string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
