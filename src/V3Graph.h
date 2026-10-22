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

#अगर_अघोषित VERILATOR_V3GRAPH_H_
#घोषणा VERILATOR_V3GRAPH_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3List.h"
#समावेश "V3Rtti.h"

#समावेश <algorithm>
#समावेश <functional>

class File_पंक्ति;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(स्थिर V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
खुला:
    क्रमागत en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        अंक_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    क्रमागत en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"FORWARD", "REVERSE"};
        वापस names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() स्थिर { वापस GraphWay{m_e ^ 1}; }
    constexpr bool forward() स्थिर { वापस m_e == FORWARD; }
    constexpr bool reverse() स्थिर { वापस m_e != FORWARD; }
};
constexpr bool चालक==(स्थिर GraphWay& lhs, स्थिर GraphWay& rhs) { वापस lhs.m_e == rhs.m_e; }
constexpr bool चालक==(स्थिर GraphWay& lhs, GraphWay::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(GraphWay::en lhs, स्थिर GraphWay& rhs) { वापस lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
खुला:
    // ENUMS
    क्रमागत Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
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
    पूर्णांक m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    जोड़ {
        व्योम* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    व्योम init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight,
              bool cutable = false) VL_MT_DISABLED;
    व्योम cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष,
                स्थिर V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, शीर्ष, old.m_weight, old.m_cutable);
    }

निजी:
    V3ListLinks<V3GraphEdge>& oLinks() { वापस m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { वापस m_iLinks; }

खुला:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, शीर्ष, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    भव V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* शीर्ष) स्थिर VL_MT_DISABLED {
        वापस new V3GraphEdge{graphp, fromp, शीर्ष, *this};
    }
    भव ~V3GraphEdge() = शेष;
    // METHODS
    // Return true iff of type T
    ढाँचा <typename T>
    bool is() स्थिर {
        static_assert(std::is_base_of<V3GraphEdge, T>::मूल्य,
                      "'T' must be a subtype of V3GraphEdge");
        वापस V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ढाँचा <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        वापस static_cast<T*>(this);
    }
    ढाँचा <typename T>
    स्थिर T* as() स्थिर {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        वापस static_cast<स्थिर T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ढाँचा <typename T>
    T* cast() {
        वापस is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ढाँचा <typename T>
    स्थिर T* cast() स्थिर {
        वापस is<T>() ? static_cast<स्थिर T*>(this) : nullptr;
    }

    भव string name() स्थिर VL_MT_DISABLED;
    भव string dotLabel() स्थिर { वापस ""; }
    भव string dotColor() स्थिर { वापस cutable() ? "yellowGreen" : "red"; }
    भव string dotStyle() स्थिर { वापस cutable() ? "dashed" : ""; }
    भव पूर्णांक sortCmp(स्थिर V3GraphEdge* rhsp) स्थिर VL_MT_DISABLED;
    व्योम unlinkDelete() VL_MT_DISABLED;
    व्योम relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    व्योम relink_शीर्ष(V3GraphVertex* new_शीर्ष) VL_MT_DISABLED;
    // ACCESSORS
    पूर्णांक weight() स्थिर { वापस m_weight; }
    व्योम weight(पूर्णांक weight) { m_weight = weight; }
    bool cutable() स्थिर { वापस m_cutable; }
    व्योम cutable(bool cutable) { m_cutable = cutable; }
    व्योम userp(व्योम* user) { m_userp = user; }
    व्योम* userp() स्थिर { वापस m_userp; }
    व्योम user(uint64_t user) { m_user = user; }
    uint64_t user() स्थिर { वापस m_user; }
    V3GraphVertex* fromp() स्थिर { वापस m_fromp; }
    V3GraphVertex* शीर्ष() स्थिर { वापस m_शीर्ष; }
    ढाँचा <GraphWay::en N_Way>
    V3GraphVertex* furtherp() स्थिर {
        वापस N_Way == GraphWay::FORWARD ? शीर्ष() : fromp();
    }
    // STATIC ACCESSORS
    अटल bool followNotCutable(स्थिर V3GraphEdge* edgep) { वापस !edgep->m_cutable; }
    अटल bool followAlwaysTrue(स्थिर V3GraphEdge*) { वापस true; }
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
    द्विगुन m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    जोड़ {
        व्योम* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    व्योम fanout(द्विगुन fanout) { m_fanout = fanout; }

रक्षित:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, स्थिर V3GraphVertex& old) VL_MT_DISABLED;

निजी:
    V3ListLinks<V3GraphVertex>& links() { वापस m_links; }

खुला:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    भव V3GraphVertex* clone(V3Graph* graphp) स्थिर VL_MT_DISABLED {
        वापस new V3GraphVertex{graphp, *this};
    }
    भव ~V3GraphVertex() = शेष;
    व्योम unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    व्योम unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    ढाँचा <typename T>
    bool is() स्थिर {
        static_assert(std::is_base_of<V3GraphVertex, T>::मूल्य,
                      "'T' must be a subtype of V3GraphVertex");
        वापस V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ढाँचा <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        वापस static_cast<T*>(this);
    }
    ढाँचा <typename T>
    स्थिर T* as() स्थिर {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        वापस static_cast<स्थिर T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ढाँचा <typename T>
    T* cast() {
        वापस is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ढाँचा <typename T>
    स्थिर T* cast() स्थिर {
        वापस is<T>() ? static_cast<स्थिर T*>(this) : nullptr;
    }

    // ACCESSORS
    भव string name() स्थिर { वापस ""; }
    भव string dotColor() स्थिर { वापस "black"; }
    भव string dotShape() स्थिर { वापस ""; }
    भव string dotStyle() स्थिर { वापस ""; }
    भव string dotName() स्थिर { वापस ""; }
    भव string dotRank() स्थिर { वापस ""; }
    भव uint32_t rankAdder() स्थिर { वापस 1; }
    भव File_पंक्ति* fileline() स्थिर { वापस nullptr; }  // nullptr for unknown
    भव पूर्णांक sortCmp(स्थिर V3GraphVertex* rhsp) स्थिर {
        // LHS goes first if of lower rank, or lower fanout
        अगर (m_rank < rhsp->m_rank) वापस -1;
        अगर (m_rank > rhsp->m_rank) वापस 1;
        अगर (m_fanout < rhsp->m_fanout) वापस -1;
        अगर (m_fanout > rhsp->m_fanout) वापस 1;
        वापस 0;
    }
    uint32_t color() स्थिर { वापस m_color; }
    व्योम color(uint32_t color) { m_color = color; }
    uint32_t rank() स्थिर { वापस m_rank; }
    व्योम rank(uint32_t rank) { m_rank = rank; }
    द्विगुन fanout() स्थिर { वापस m_fanout; }
    व्योम user(uint32_t user) { m_user = user; }
    uint32_t user() स्थिर VL_MT_STABLE { वापस m_user; }
    व्योम userp(व्योम* userp) { m_userp = userp; }
    व्योम* userp() स्थिर VL_MT_STABLE { वापस m_userp; }
    V3GraphEdge::IList& inEdges() { वापस m_ins; }
    स्थिर V3GraphEdge::IList& inEdges() स्थिर { वापस m_ins; }
    ढाँचा <GraphWay::en N_Way>
    अंतरभूत स्वतः& edges();
    ढाँचा <GraphWay::en N_Way>
    अंतरभूत स्थिर स्वतः& edges() स्थिर;
    bool inEmpty() स्थिर { वापस m_ins.empty(); }
    bool inSize1() स्थिर { वापस m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { वापस m_outs; }
    स्थिर V3GraphEdge::OList& outEdges() स्थिर { वापस m_outs; }
    bool outEmpty() स्थिर { वापस m_outs.empty(); }
    bool outSize1() स्थिर { वापस m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    व्योम v3errorEnd(स्थिर std::ostringstream& str) स्थिर
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    व्योम v3errorEndFatal(स्थिर std::ostringstream& str) स्थिर
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    व्योम rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& चालक<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

ढाँचा <>
अंतरभूत स्वतः& V3GraphVertex::edges<GraphWay::FORWARD>() {
    वापस m_outs;
}

ढाँचा <>
अंतरभूत स्वतः& V3GraphVertex::edges<GraphWay::REVERSE>() {
    वापस m_ins;
}

ढाँचा <>
अंतरभूत स्थिर स्वतः& V3GraphVertex::edges<GraphWay::FORWARD>() स्थिर {
    वापस m_outs;
}

ढाँचा <>
अंतरभूत स्थिर स्वतः& V3GraphVertex::edges<GraphWay::REVERSE>() स्थिर {
    वापस m_ins;
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
    द्विगुन orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    व्योम dumpEdge(std::ostream& os, स्थिर V3GraphVertex& vertex,
                  स्थिर V3GraphEdge& edge) स्थिर VL_MT_DISABLED;
    // ACCESSORS

खुला:
    V3Graph() VL_MT_DISABLED;
    भव ~V3Graph() VL_MT_DISABLED;
    भव string dotRankDir() स्थिर { वापस "TB"; }  // rankdir for dot plotting

    // METHODS
    व्योम clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() स्थिर { वापस m_vertices.empty(); }
    V3GraphVertex::List& vertices() { वापस m_vertices; }
    स्थिर V3GraphVertex::List& vertices() स्थिर { वापस m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    व्योम stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    व्योम rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    व्योम rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    व्योम rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    व्योम rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    व्योम sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    व्योम sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    व्योम order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    व्योम orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    व्योम acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    व्योम removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    व्योम remove_Redundant_Edges_योग(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    व्योम removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    व्योम subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    व्योम userClearVertices() VL_MT_DISABLED;
    व्योम userClearEdges() VL_MT_DISABLED;

    /// Debugging
    व्योम dump(std::ostream& os = std::cout) स्थिर VL_MT_DISABLED;
    व्योम dumpDotFile(स्थिर string& filename, bool colorAsSubgraph) स्थिर VL_MT_DISABLED;
    व्योम dumpDotFilePrefixed(स्थिर string& nameComment,
                             bool colorAsSubgraph = false) स्थिर VL_MT_DISABLED;
    व्योम dumpDotFilePrefixedAlways(स्थिर string& nameComment,
                                   bool colorAsSubgraph = false) स्थिर VL_MT_DISABLED;
    व्योम dumpEdges(std::ostream& os, स्थिर V3GraphVertex& vertex) स्थिर VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    व्योम hashGraphDebug(स्थिर अक्षर* debugName) स्थिर VL_MT_DISABLED;

    अटल व्योम selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        मित्र class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_योग_Graph_Cost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_गिनती = 0;  // Number of vertexes in the graph
        uint64_t m_edge_गिनती = 0;  // Number of edges in the grap

        ParallelismReport() = शेष;

    खुला:
        ~ParallelismReport() = शेष;
        ParallelismReport(स्थिर ParallelismReport&) = शेष;
        ParallelismReport& चालक=(स्थिर ParallelismReport&) = शेष;

        uint64_t योग_Graph_Cost() स्थिर { वापस m_योग_Graph_Cost; }
        uint64_t criticalPathCost() स्थिर { वापस m_criticalPathCost; }
        uint64_t vertex_गिनती() स्थिर { वापस m_vertex_गिनती; }
        uint64_t edge_गिनती() स्थिर { वापस m_edge_गिनती; }
        द्विगुन parallelismFactor() स्थिर {
            वापस (static_cast<द्विगुन>(m_योग_Graph_Cost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(स्थिर V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    भव व्योम loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    भव string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#पूर्ण_अगर  // Guard
