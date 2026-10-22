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

class File_पङ्क्ति;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(अचर V3GraphEdge* edgep);

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
        अङ्क_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"FORWARD", "REVERSE"};
        फर्काऊ names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() अचर { फर्काऊ GraphWay{m_e ^ 1}; }
    constexpr bool forward() अचर { फर्काऊ m_e == FORWARD; }
    constexpr bool reverse() अचर { फर्काऊ m_e != FORWARD; }
};
constexpr bool चालक==(अचर GraphWay& lhs, अचर GraphWay& rhs) { फर्काऊ lhs.m_e == rhs.m_e; }
constexpr bool चालक==(अचर GraphWay& lhs, GraphWay::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(GraphWay::en lhs, अचर GraphWay& rhs) { फर्काऊ lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
सार्वजनिक:
    // ENUMS
    गणना Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
रक्षित:
    मित्र class V3Graph;
    मित्र class V3GraphVertex;
    मित्र class GraphAcyc;
    मित्र class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_शीर्ष;  // Vertices this edge points to
    पूर्णाङ्क m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    सङ्घ {
        शून्य* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    शून्य init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight,
              bool cutable = false) VL_MT_DISABLED;
    शून्य cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष,
                अचर V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, शीर्ष, old.m_weight, old.m_cutable);
    }

गुप्त:
    V3ListLinks<V3GraphEdge>& oLinks() { फर्काऊ m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { फर्काऊ m_iLinks; }

सार्वजनिक:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, शीर्ष, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    आभास V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* शीर्ष) अचर VL_MT_DISABLED {
        फर्काऊ new V3GraphEdge{graphp, fromp, शीर्ष, *this};
    }
    आभास ~V3GraphEdge() = स्वभाव;
    // METHODS
    // Return true iff of type T
    ढाँचा <typename T>
    bool is() अचर {
        static_assert(std::is_base_of<V3GraphEdge, T>::मान,
                      "'T' must be a subtype of V3GraphEdge");
        फर्काऊ V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ढाँचा <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        फर्काऊ static_cast<T*>(this);
    }
    ढाँचा <typename T>
    अचर T* as() अचर {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        फर्काऊ static_cast<अचर T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ढाँचा <typename T>
    T* cast() {
        फर्काऊ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ढाँचा <typename T>
    अचर T* cast() अचर {
        फर्काऊ is<T>() ? static_cast<अचर T*>(this) : nullptr;
    }

    आभास string name() अचर VL_MT_DISABLED;
    आभास string dotLabel() अचर { फर्काऊ ""; }
    आभास string dotColor() अचर { फर्काऊ cutable() ? "yellowGreen" : "red"; }
    आभास string dotStyle() अचर { फर्काऊ cutable() ? "dashed" : ""; }
    आभास पूर्णाङ्क sortCmp(अचर V3GraphEdge* rhsp) अचर VL_MT_DISABLED;
    शून्य unlinkDelete() VL_MT_DISABLED;
    शून्य relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    शून्य relink_शीर्ष(V3GraphVertex* new_शीर्ष) VL_MT_DISABLED;
    // ACCESSORS
    पूर्णाङ्क weight() अचर { फर्काऊ m_weight; }
    शून्य weight(पूर्णाङ्क weight) { m_weight = weight; }
    bool cutable() अचर { फर्काऊ m_cutable; }
    शून्य cutable(bool cutable) { m_cutable = cutable; }
    शून्य userp(शून्य* user) { m_userp = user; }
    शून्य* userp() अचर { फर्काऊ m_userp; }
    शून्य user(uint64_t user) { m_user = user; }
    uint64_t user() अचर { फर्काऊ m_user; }
    V3GraphVertex* fromp() अचर { फर्काऊ m_fromp; }
    V3GraphVertex* शीर्ष() अचर { फर्काऊ m_शीर्ष; }
    ढाँचा <GraphWay::en N_Way>
    V3GraphVertex* furtherp() अचर {
        फर्काऊ N_Way == GraphWay::FORWARD ? शीर्ष() : fromp();
    }
    // STATIC ACCESSORS
    स्थिर bool followNotCutable(अचर V3GraphEdge* edgep) { फर्काऊ !edgep->m_cutable; }
    स्थिर bool followAlwaysTrue(अचर V3GraphEdge*) { फर्काऊ true; }
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
    सङ्घ {
        शून्य* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    शून्य fanout(द्विगुण fanout) { m_fanout = fanout; }

रक्षित:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, अचर V3GraphVertex& old) VL_MT_DISABLED;

गुप्त:
    V3ListLinks<V3GraphVertex>& links() { फर्काऊ m_links; }

सार्वजनिक:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    आभास V3GraphVertex* clone(V3Graph* graphp) अचर VL_MT_DISABLED {
        फर्काऊ new V3GraphVertex{graphp, *this};
    }
    आभास ~V3GraphVertex() = स्वभाव;
    शून्य unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    शून्य unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    ढाँचा <typename T>
    bool is() अचर {
        static_assert(std::is_base_of<V3GraphVertex, T>::मान,
                      "'T' must be a subtype of V3GraphVertex");
        फर्काऊ V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ढाँचा <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        फर्काऊ static_cast<T*>(this);
    }
    ढाँचा <typename T>
    अचर T* as() अचर {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        फर्काऊ static_cast<अचर T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ढाँचा <typename T>
    T* cast() {
        फर्काऊ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ढाँचा <typename T>
    अचर T* cast() अचर {
        फर्काऊ is<T>() ? static_cast<अचर T*>(this) : nullptr;
    }

    // ACCESSORS
    आभास string name() अचर { फर्काऊ ""; }
    आभास string dotColor() अचर { फर्काऊ "black"; }
    आभास string dotShape() अचर { फर्काऊ ""; }
    आभास string dotStyle() अचर { फर्काऊ ""; }
    आभास string dotName() अचर { फर्काऊ ""; }
    आभास string dotRank() अचर { फर्काऊ ""; }
    आभास uint32_t rankAdder() अचर { फर्काऊ 1; }
    आभास File_पङ्क्ति* fileline() अचर { फर्काऊ nullptr; }  // nullptr for unknown
    आभास पूर्णाङ्क sortCmp(अचर V3GraphVertex* rhsp) अचर {
        // LHS goes first if of lower rank, or lower fanout
        यदि (m_rank < rhsp->m_rank) फर्काऊ -1;
        यदि (m_rank > rhsp->m_rank) फर्काऊ 1;
        यदि (m_fanout < rhsp->m_fanout) फर्काऊ -1;
        यदि (m_fanout > rhsp->m_fanout) फर्काऊ 1;
        फर्काऊ 0;
    }
    uint32_t color() अचर { फर्काऊ m_color; }
    शून्य color(uint32_t color) { m_color = color; }
    uint32_t rank() अचर { फर्काऊ m_rank; }
    शून्य rank(uint32_t rank) { m_rank = rank; }
    द्विगुण fanout() अचर { फर्काऊ m_fanout; }
    शून्य user(uint32_t user) { m_user = user; }
    uint32_t user() अचर VL_MT_STABLE { फर्काऊ m_user; }
    शून्य userp(शून्य* userp) { m_userp = userp; }
    शून्य* userp() अचर VL_MT_STABLE { फर्काऊ m_userp; }
    V3GraphEdge::IList& inEdges() { फर्काऊ m_ins; }
    अचर V3GraphEdge::IList& inEdges() अचर { फर्काऊ m_ins; }
    ढाँचा <GraphWay::en N_Way>
    अन्तःस्थ स्वतः& edges();
    ढाँचा <GraphWay::en N_Way>
    अन्तःस्थ अचर स्वतः& edges() अचर;
    bool inEmpty() अचर { फर्काऊ m_ins.empty(); }
    bool inSize1() अचर { फर्काऊ m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { फर्काऊ m_outs; }
    अचर V3GraphEdge::OList& outEdges() अचर { फर्काऊ m_outs; }
    bool outEmpty() अचर { फर्काऊ m_outs.empty(); }
    bool outSize1() अचर { फर्काऊ m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    शून्य v3errorEnd(अचर std::ostringstream& str) अचर
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    शून्य v3errorEndFatal(अचर std::ostringstream& str) अचर
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    शून्य rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& चालक<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

ढाँचा <>
अन्तःस्थ स्वतः& V3GraphVertex::edges<GraphWay::FORWARD>() {
    फर्काऊ m_outs;
}

ढाँचा <>
अन्तःस्थ स्वतः& V3GraphVertex::edges<GraphWay::REVERSE>() {
    फर्काऊ m_ins;
}

ढाँचा <>
अन्तःस्थ अचर स्वतः& V3GraphVertex::edges<GraphWay::FORWARD>() अचर {
    फर्काऊ m_outs;
}

ढाँचा <>
अन्तःस्थ अचर स्वतः& V3GraphVertex::edges<GraphWay::REVERSE>() अचर {
    फर्काऊ m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

रक्षित:
    मित्र class V3GraphVertex;
    मित्र class V3GraphEdge;
    मित्र class GraphAcyc;
    // METHODS
    द्विगुण orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    शून्य dumpEdge(std::ostream& os, अचर V3GraphVertex& vertex,
                  अचर V3GraphEdge& edge) अचर VL_MT_DISABLED;
    // ACCESSORS

सार्वजनिक:
    V3Graph() VL_MT_DISABLED;
    आभास ~V3Graph() VL_MT_DISABLED;
    आभास string dotRankDir() अचर { फर्काऊ "TB"; }  // rankdir for dot plotting

    // METHODS
    शून्य clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() अचर { फर्काऊ m_vertices.empty(); }
    V3GraphVertex::List& vertices() { फर्काऊ m_vertices; }
    अचर V3GraphVertex::List& vertices() अचर { फर्काऊ m_vertices; }

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
    शून्य dump(std::ostream& os = std::cout) अचर VL_MT_DISABLED;
    शून्य dumpDotFile(अचर string& filename, bool colorAsSubgraph) अचर VL_MT_DISABLED;
    शून्य dumpDotFilePrefixed(अचर string& nameComment,
                             bool colorAsSubgraph = false) अचर VL_MT_DISABLED;
    शून्य dumpDotFilePrefixedAlways(अचर string& nameComment,
                                   bool colorAsSubgraph = false) अचर VL_MT_DISABLED;
    शून्य dumpEdges(std::ostream& os, अचर V3GraphVertex& vertex) अचर VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    शून्य hashGraphDebug(अचर वर्ण* debugName) अचर VL_MT_DISABLED;

    स्थिर शून्य selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        मित्र class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_गन्ती = 0;  // Number of vertexes in the graph
        uint64_t m_edge_गन्ती = 0;  // Number of edges in the grap

        ParallelismReport() = स्वभाव;

    सार्वजनिक:
        ~ParallelismReport() = स्वभाव;
        ParallelismReport(अचर ParallelismReport&) = स्वभाव;
        ParallelismReport& चालक=(अचर ParallelismReport&) = स्वभाव;

        uint64_t totalGraphCost() अचर { फर्काऊ m_totalGraphCost; }
        uint64_t criticalPathCost() अचर { फर्काऊ m_criticalPathCost; }
        uint64_t vertex_गन्ती() अचर { फर्काऊ m_vertex_गन्ती; }
        uint64_t edge_गन्ती() अचर { फर्काऊ m_edge_गन्ती; }
        द्विगुण parallelismFactor() अचर {
            फर्काऊ (static_cast<द्विगुण>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(अचर V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    आभास शून्य loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    आभास string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
