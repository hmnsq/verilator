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

class File_ओळ;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(स्थिरांक V3GraphEdge* edgep);

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
        अंक_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    गणना en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिरांक { परत m_e; }
    स्थिरांक अक्षर* ascii() स्थिरांक {
        स्थिर constexpr स्थिरांक अक्षर* स्थिरांक names[] = {"FORWARD", "REVERSE"};
        परत names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() स्थिरांक { परत GraphWay{m_e ^ 1}; }
    constexpr bool forward() स्थिरांक { परत m_e == FORWARD; }
    constexpr bool reverse() स्थिरांक { परत m_e != FORWARD; }
};
constexpr bool चालक==(स्थिरांक GraphWay& lhs, स्थिरांक GraphWay& rhs) { परत lhs.m_e == rhs.m_e; }
constexpr bool चालक==(स्थिरांक GraphWay& lhs, GraphWay::en rhs) { परत lhs.m_e == rhs; }
constexpr bool चालक==(GraphWay::en lhs, स्थिरांक GraphWay& rhs) { परत lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
सार्वजनिक:
    // ENUMS
    गणना Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
संरक्षित:
    मित्र class V3Graph;
    मित्र class V3GraphVertex;
    मित्र class GraphAcyc;
    मित्र class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_शीर्ष;  // Vertices this edge points to
    पूर्णांक m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    संघ {
        शून्य* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    शून्य init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight,
              bool cutable = false) VL_MT_DISABLED;
    शून्य cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष,
                स्थिरांक V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, शीर्ष, old.m_weight, old.m_cutable);
    }

खाजगी:
    V3ListLinks<V3GraphEdge>& oLinks() { परत m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { परत m_iLinks; }

सार्वजनिक:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, शीर्ष, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    आभासी V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* शीर्ष) स्थिरांक VL_MT_DISABLED {
        परत new V3GraphEdge{graphp, fromp, शीर्ष, *this};
    }
    आभासी ~V3GraphEdge() = पूर्वनिर्धारित;
    // METHODS
    // Return true iff of type T
    ढाँचा <typename T>
    bool is() स्थिरांक {
        static_assert(std::is_base_of<V3GraphEdge, T>::मूल्य,
                      "'T' must be a subtype of V3GraphEdge");
        परत V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ढाँचा <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        परत static_cast<T*>(this);
    }
    ढाँचा <typename T>
    स्थिरांक T* as() स्थिरांक {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        परत static_cast<स्थिरांक T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ढाँचा <typename T>
    T* cast() {
        परत is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ढाँचा <typename T>
    स्थिरांक T* cast() स्थिरांक {
        परत is<T>() ? static_cast<स्थिरांक T*>(this) : nullptr;
    }

    आभासी string name() स्थिरांक VL_MT_DISABLED;
    आभासी string dotLabel() स्थिरांक { परत ""; }
    आभासी string dotColor() स्थिरांक { परत cutable() ? "yellowGreen" : "red"; }
    आभासी string dotStyle() स्थिरांक { परत cutable() ? "dashed" : ""; }
    आभासी पूर्णांक sortCmp(स्थिरांक V3GraphEdge* rhsp) स्थिरांक VL_MT_DISABLED;
    शून्य unlinkDelete() VL_MT_DISABLED;
    शून्य relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    शून्य relink_शीर्ष(V3GraphVertex* new_शीर्ष) VL_MT_DISABLED;
    // ACCESSORS
    पूर्णांक weight() स्थिरांक { परत m_weight; }
    शून्य weight(पूर्णांक weight) { m_weight = weight; }
    bool cutable() स्थिरांक { परत m_cutable; }
    शून्य cutable(bool cutable) { m_cutable = cutable; }
    शून्य userp(शून्य* user) { m_userp = user; }
    शून्य* userp() स्थिरांक { परत m_userp; }
    शून्य user(uint64_t user) { m_user = user; }
    uint64_t user() स्थिरांक { परत m_user; }
    V3GraphVertex* fromp() स्थिरांक { परत m_fromp; }
    V3GraphVertex* शीर्ष() स्थिरांक { परत m_शीर्ष; }
    ढाँचा <GraphWay::en N_Way>
    V3GraphVertex* furtherp() स्थिरांक {
        परत N_Way == GraphWay::FORWARD ? शीर्ष() : fromp();
    }
    // STATIC ACCESSORS
    स्थिर bool followNotCutable(स्थिरांक V3GraphEdge* edgep) { परत !edgep->m_cutable; }
    स्थिर bool followAlwaysTrue(स्थिरांक V3GraphEdge*) { परत true; }
};

//============================================================================

class V3GraphVertex VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphVertex)
    मित्र class V3Graph;
    मित्र class V3GraphEdge;
    मित्र class GraphAcyc;
    मित्र class GraphAlgRank;
    मित्र class GraphAlgRankDepth;
    V3ListLinks<V3GraphVertex> m_links;  // List links to store instances of this class
    V3GraphEdge::OList m_outs;  // List of outbound edges
    V3GraphEdge::IList m_ins;  // List of inbound edges
    द्विगुण m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    संघ {
        शून्य* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    शून्य fanout(द्विगुण fanout) { m_fanout = fanout; }

संरक्षित:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, स्थिरांक V3GraphVertex& old) VL_MT_DISABLED;

खाजगी:
    V3ListLinks<V3GraphVertex>& links() { परत m_links; }

सार्वजनिक:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    आभासी V3GraphVertex* clone(V3Graph* graphp) स्थिरांक VL_MT_DISABLED {
        परत new V3GraphVertex{graphp, *this};
    }
    आभासी ~V3GraphVertex() = पूर्वनिर्धारित;
    शून्य unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    शून्य unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    ढाँचा <typename T>
    bool is() स्थिरांक {
        static_assert(std::is_base_of<V3GraphVertex, T>::मूल्य,
                      "'T' must be a subtype of V3GraphVertex");
        परत V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ढाँचा <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        परत static_cast<T*>(this);
    }
    ढाँचा <typename T>
    स्थिरांक T* as() स्थिरांक {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        परत static_cast<स्थिरांक T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ढाँचा <typename T>
    T* cast() {
        परत is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ढाँचा <typename T>
    स्थिरांक T* cast() स्थिरांक {
        परत is<T>() ? static_cast<स्थिरांक T*>(this) : nullptr;
    }

    // ACCESSORS
    आभासी string name() स्थिरांक { परत ""; }
    आभासी string dotColor() स्थिरांक { परत "black"; }
    आभासी string dotShape() स्थिरांक { परत ""; }
    आभासी string dotStyle() स्थिरांक { परत ""; }
    आभासी string dotName() स्थिरांक { परत ""; }
    आभासी string dotRank() स्थिरांक { परत ""; }
    आभासी uint32_t rankAdder() स्थिरांक { परत 1; }
    आभासी File_ओळ* fileline() स्थिरांक { परत nullptr; }  // nullptr for unknown
    आभासी पूर्णांक sortCmp(स्थिरांक V3GraphVertex* rhsp) स्थिरांक {
        // LHS goes first if of lower rank, or lower fanout
        जर (m_rank < rhsp->m_rank) परत -1;
        जर (m_rank > rhsp->m_rank) परत 1;
        जर (m_fanout < rhsp->m_fanout) परत -1;
        जर (m_fanout > rhsp->m_fanout) परत 1;
        परत 0;
    }
    uint32_t color() स्थिरांक { परत m_color; }
    शून्य color(uint32_t color) { m_color = color; }
    uint32_t rank() स्थिरांक { परत m_rank; }
    शून्य rank(uint32_t rank) { m_rank = rank; }
    द्विगुण fanout() स्थिरांक { परत m_fanout; }
    शून्य user(uint32_t user) { m_user = user; }
    uint32_t user() स्थिरांक VL_MT_STABLE { परत m_user; }
    शून्य userp(शून्य* userp) { m_userp = userp; }
    शून्य* userp() स्थिरांक VL_MT_STABLE { परत m_userp; }
    V3GraphEdge::IList& inEdges() { परत m_ins; }
    स्थिरांक V3GraphEdge::IList& inEdges() स्थिरांक { परत m_ins; }
    ढाँचा <GraphWay::en N_Way>
    ओळीत स्वतः& edges();
    ढाँचा <GraphWay::en N_Way>
    ओळीत स्थिरांक स्वतः& edges() स्थिरांक;
    bool inEmpty() स्थिरांक { परत m_ins.empty(); }
    bool inSize1() स्थिरांक { परत m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { परत m_outs; }
    स्थिरांक V3GraphEdge::OList& outEdges() स्थिरांक { परत m_outs; }
    bool outEmpty() स्थिरांक { परत m_outs.empty(); }
    bool outSize1() स्थिरांक { परत m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    शून्य v3errorEnd(स्थिरांक std::ostringstream& str) स्थिरांक
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    शून्य v3errorEndFatal(स्थिरांक std::ostringstream& str) स्थिरांक
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    शून्य rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& चालक<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

ढाँचा <>
ओळीत स्वतः& V3GraphVertex::edges<GraphWay::FORWARD>() {
    परत m_outs;
}

ढाँचा <>
ओळीत स्वतः& V3GraphVertex::edges<GraphWay::REVERSE>() {
    परत m_ins;
}

ढाँचा <>
ओळीत स्थिरांक स्वतः& V3GraphVertex::edges<GraphWay::FORWARD>() स्थिरांक {
    परत m_outs;
}

ढाँचा <>
ओळीत स्थिरांक स्वतः& V3GraphVertex::edges<GraphWay::REVERSE>() स्थिरांक {
    परत m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

संरक्षित:
    मित्र class V3GraphVertex;
    मित्र class V3GraphEdge;
    मित्र class GraphAcyc;
    // METHODS
    द्विगुण orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    शून्य dumpEdge(std::ostream& os, स्थिरांक V3GraphVertex& vertex,
                  स्थिरांक V3GraphEdge& edge) स्थिरांक VL_MT_DISABLED;
    // ACCESSORS

सार्वजनिक:
    V3Graph() VL_MT_DISABLED;
    आभासी ~V3Graph() VL_MT_DISABLED;
    आभासी string dotRankDir() स्थिरांक { परत "TB"; }  // rankdir for dot plotting

    // METHODS
    शून्य clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() स्थिरांक { परत m_vertices.empty(); }
    V3GraphVertex::List& vertices() { परत m_vertices; }
    स्थिरांक V3GraphVertex::List& vertices() स्थिरांक { परत m_vertices; }

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
    शून्य dump(std::ostream& os = std::cout) स्थिरांक VL_MT_DISABLED;
    शून्य dumpDotFile(स्थिरांक string& filename, bool colorAsSubgraph) स्थिरांक VL_MT_DISABLED;
    शून्य dumpDotFilePrefixed(स्थिरांक string& nameComment,
                             bool colorAsSubgraph = false) स्थिरांक VL_MT_DISABLED;
    शून्य dumpDotFilePrefixedAlways(स्थिरांक string& nameComment,
                                   bool colorAsSubgraph = false) स्थिरांक VL_MT_DISABLED;
    शून्य dumpEdges(std::ostream& os, स्थिरांक V3GraphVertex& vertex) स्थिरांक VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    शून्य hashGraphDebug(स्थिरांक अक्षर* debugName) स्थिरांक VL_MT_DISABLED;

    स्थिर शून्य selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        मित्र class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_मोजणी = 0;  // Number of vertexes in the graph
        uint64_t m_edge_मोजणी = 0;  // Number of edges in the grap

        ParallelismReport() = पूर्वनिर्धारित;

    सार्वजनिक:
        ~ParallelismReport() = पूर्वनिर्धारित;
        ParallelismReport(स्थिरांक ParallelismReport&) = पूर्वनिर्धारित;
        ParallelismReport& चालक=(स्थिरांक ParallelismReport&) = पूर्वनिर्धारित;

        uint64_t totalGraphCost() स्थिरांक { परत m_totalGraphCost; }
        uint64_t criticalPathCost() स्थिरांक { परत m_criticalPathCost; }
        uint64_t vertex_मोजणी() स्थिरांक { परत m_vertex_मोजणी; }
        uint64_t edge_मोजणी() स्थिरांक { परत m_edge_मोजणी; }
        द्विगुण parallelismFactor() स्थिरांक {
            परत (static_cast<द्विगुण>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(स्थिरांक V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    आभासी शून्य loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    आभासी string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
