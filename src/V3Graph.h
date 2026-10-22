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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3List.h"
#അന്തര്ഭാവ "V3Rtti.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <functional>

class File_വരി;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(സ്ഥിരം V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        അക്കം_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    എണ്ണൽ en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"FORWARD", "REVERSE"};
        തിരികെ names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() സ്ഥിരം { തിരികെ GraphWay{m_e ^ 1}; }
    constexpr bool forward() സ്ഥിരം { തിരികെ m_e == FORWARD; }
    constexpr bool reverse() സ്ഥിരം { തിരികെ m_e != FORWARD; }
};
constexpr bool operator==(സ്ഥിരം GraphWay& lhs, സ്ഥിരം GraphWay& rhs) { തിരികെ lhs.m_e == rhs.m_e; }
constexpr bool operator==(സ്ഥിരം GraphWay& lhs, GraphWay::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, സ്ഥിരം GraphWay& rhs) { തിരികെ lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
പൊതു:
    // ENUMS
    എണ്ണൽ Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
സംരക്ഷിത:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_മുകൾ;  // Vertices this edge points to
    പൂർണ്ണസംഖ്യ m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    സംഘം {
        ശൂന്യം* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    ശൂന്യം init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* മുകൾ, പൂർണ്ണസംഖ്യ weight,
              bool cutable = false) VL_MT_DISABLED;
    ശൂന്യം cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* മുകൾ,
                സ്ഥിരം V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, മുകൾ, old.m_weight, old.m_cutable);
    }

സ്വകാര്യ:
    V3ListLinks<V3GraphEdge>& oLinks() { തിരികെ m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { തിരികെ m_iLinks; }

പൊതു:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* മുകൾ, പൂർണ്ണസംഖ്യ weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, മുകൾ, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    മായാ V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* മുകൾ) സ്ഥിരം VL_MT_DISABLED {
        തിരികെ new V3GraphEdge{graphp, fromp, മുകൾ, *this};
    }
    മായാ ~V3GraphEdge() = സ്ഥിരസ്ഥിതി;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() സ്ഥിരം {
        static_assert(std::is_base_of<V3GraphEdge, T>::മൂല്യം,
                      "'T' must be a subtype of V3GraphEdge");
        തിരികെ V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        തിരികെ static_cast<T*>(this);
    }
    template <typename T>
    സ്ഥിരം T* as() സ്ഥിരം {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        തിരികെ static_cast<സ്ഥിരം T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        തിരികെ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    സ്ഥിരം T* cast() സ്ഥിരം {
        തിരികെ is<T>() ? static_cast<സ്ഥിരം T*>(this) : nullptr;
    }

    മായാ string name() സ്ഥിരം VL_MT_DISABLED;
    മായാ string dotLabel() സ്ഥിരം { തിരികെ ""; }
    മായാ string dotColor() സ്ഥിരം { തിരികെ cutable() ? "yellowGreen" : "red"; }
    മായാ string dotStyle() സ്ഥിരം { തിരികെ cutable() ? "dashed" : ""; }
    മായാ പൂർണ്ണസംഖ്യ sortCmp(സ്ഥിരം V3GraphEdge* rhsp) സ്ഥിരം VL_MT_DISABLED;
    ശൂന്യം unlinkDelete() VL_MT_DISABLED;
    ശൂന്യം relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    ശൂന്യം relink_മുകൾ(V3GraphVertex* new_മുകൾ) VL_MT_DISABLED;
    // ACCESSORS
    പൂർണ്ണസംഖ്യ weight() സ്ഥിരം { തിരികെ m_weight; }
    ശൂന്യം weight(പൂർണ്ണസംഖ്യ weight) { m_weight = weight; }
    bool cutable() സ്ഥിരം { തിരികെ m_cutable; }
    ശൂന്യം cutable(bool cutable) { m_cutable = cutable; }
    ശൂന്യം userp(ശൂന്യം* user) { m_userp = user; }
    ശൂന്യം* userp() സ്ഥിരം { തിരികെ m_userp; }
    ശൂന്യം user(uint64_t user) { m_user = user; }
    uint64_t user() സ്ഥിരം { തിരികെ m_user; }
    V3GraphVertex* fromp() സ്ഥിരം { തിരികെ m_fromp; }
    V3GraphVertex* മുകൾ() സ്ഥിരം { തിരികെ m_മുകൾ; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() സ്ഥിരം {
        തിരികെ N_Way == GraphWay::FORWARD ? മുകൾ() : fromp();
    }
    // STATIC ACCESSORS
    സ്ഥിതം bool followNotCutable(സ്ഥിരം V3GraphEdge* edgep) { തിരികെ !edgep->m_cutable; }
    സ്ഥിതം bool followAlwaysTrue(സ്ഥിരം V3GraphEdge*) { തിരികെ true; }
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
    ഇരട്ട m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    സംഘം {
        ശൂന്യം* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    ശൂന്യം fanout(ഇരട്ട fanout) { m_fanout = fanout; }

സംരക്ഷിത:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, സ്ഥിരം V3GraphVertex& old) VL_MT_DISABLED;

സ്വകാര്യ:
    V3ListLinks<V3GraphVertex>& links() { തിരികെ m_links; }

പൊതു:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    മായാ V3GraphVertex* clone(V3Graph* graphp) സ്ഥിരം VL_MT_DISABLED {
        തിരികെ new V3GraphVertex{graphp, *this};
    }
    മായാ ~V3GraphVertex() = സ്ഥിരസ്ഥിതി;
    ശൂന്യം unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    ശൂന്യം unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() സ്ഥിരം {
        static_assert(std::is_base_of<V3GraphVertex, T>::മൂല്യം,
                      "'T' must be a subtype of V3GraphVertex");
        തിരികെ V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        തിരികെ static_cast<T*>(this);
    }
    template <typename T>
    സ്ഥിരം T* as() സ്ഥിരം {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        തിരികെ static_cast<സ്ഥിരം T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        തിരികെ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    സ്ഥിരം T* cast() സ്ഥിരം {
        തിരികെ is<T>() ? static_cast<സ്ഥിരം T*>(this) : nullptr;
    }

    // ACCESSORS
    മായാ string name() സ്ഥിരം { തിരികെ ""; }
    മായാ string dotColor() സ്ഥിരം { തിരികെ "black"; }
    മായാ string dotShape() സ്ഥിരം { തിരികെ ""; }
    മായാ string dotStyle() സ്ഥിരം { തിരികെ ""; }
    മായാ string dotName() സ്ഥിരം { തിരികെ ""; }
    മായാ string dotRank() സ്ഥിരം { തിരികെ ""; }
    മായാ uint32_t rankAdder() സ്ഥിരം { തിരികെ 1; }
    മായാ File_വരി* fileline() സ്ഥിരം { തിരികെ nullptr; }  // nullptr for unknown
    മായാ പൂർണ്ണസംഖ്യ sortCmp(സ്ഥിരം V3GraphVertex* rhsp) സ്ഥിരം {
        // LHS goes first if of lower rank, or lower fanout
        എങ്കിൽ (m_rank < rhsp->m_rank) തിരികെ -1;
        എങ്കിൽ (m_rank > rhsp->m_rank) തിരികെ 1;
        എങ്കിൽ (m_fanout < rhsp->m_fanout) തിരികെ -1;
        എങ്കിൽ (m_fanout > rhsp->m_fanout) തിരികെ 1;
        തിരികെ 0;
    }
    uint32_t color() സ്ഥിരം { തിരികെ m_color; }
    ശൂന്യം color(uint32_t color) { m_color = color; }
    uint32_t rank() സ്ഥിരം { തിരികെ m_rank; }
    ശൂന്യം rank(uint32_t rank) { m_rank = rank; }
    ഇരട്ട fanout() സ്ഥിരം { തിരികെ m_fanout; }
    ശൂന്യം user(uint32_t user) { m_user = user; }
    uint32_t user() സ്ഥിരം VL_MT_STABLE { തിരികെ m_user; }
    ശൂന്യം userp(ശൂന്യം* userp) { m_userp = userp; }
    ശൂന്യം* userp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_userp; }
    V3GraphEdge::IList& inEdges() { തിരികെ m_ins; }
    സ്ഥിരം V3GraphEdge::IList& inEdges() സ്ഥിരം { തിരികെ m_ins; }
    template <GraphWay::en N_Way>
    വരിയിൽ auto& edges();
    template <GraphWay::en N_Way>
    വരിയിൽ സ്ഥിരം auto& edges() സ്ഥിരം;
    bool inEmpty() സ്ഥിരം { തിരികെ m_ins.empty(); }
    bool inSize1() സ്ഥിരം { തിരികെ m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { തിരികെ m_outs; }
    സ്ഥിരം V3GraphEdge::OList& outEdges() സ്ഥിരം { തിരികെ m_outs; }
    bool outEmpty() സ്ഥിരം { തിരികെ m_outs.empty(); }
    bool outSize1() സ്ഥിരം { തിരികെ m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    ശൂന്യം v3errorEnd(സ്ഥിരം std::ostringstream& str) സ്ഥിരം
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    ശൂന്യം v3errorEndFatal(സ്ഥിരം std::ostringstream& str) സ്ഥിരം
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    ശൂന്യം rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
വരിയിൽ auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    തിരികെ m_outs;
}

template <>
വരിയിൽ auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    തിരികെ m_ins;
}

template <>
വരിയിൽ സ്ഥിരം auto& V3GraphVertex::edges<GraphWay::FORWARD>() സ്ഥിരം {
    തിരികെ m_outs;
}

template <>
വരിയിൽ സ്ഥിരം auto& V3GraphVertex::edges<GraphWay::REVERSE>() സ്ഥിരം {
    തിരികെ m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

സംരക്ഷിത:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    ഇരട്ട orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    ശൂന്യം dumpEdge(std::ostream& os, സ്ഥിരം V3GraphVertex& vertex,
                  സ്ഥിരം V3GraphEdge& edge) സ്ഥിരം VL_MT_DISABLED;
    // ACCESSORS

പൊതു:
    V3Graph() VL_MT_DISABLED;
    മായാ ~V3Graph() VL_MT_DISABLED;
    മായാ string dotRankDir() സ്ഥിരം { തിരികെ "TB"; }  // rankdir for dot plotting

    // METHODS
    ശൂന്യം clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() സ്ഥിരം { തിരികെ m_vertices.empty(); }
    V3GraphVertex::List& vertices() { തിരികെ m_vertices; }
    സ്ഥിരം V3GraphVertex::List& vertices() സ്ഥിരം { തിരികെ m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    ശൂന്യം stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ശൂന്യം rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    ശൂന്യം rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ശൂന്യം rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    ശൂന്യം rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    ശൂന്യം sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    ശൂന്യം sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    ശൂന്യം order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    ശൂന്യം orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    ശൂന്യം acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    ശൂന്യം removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    ശൂന്യം removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    ശൂന്യം removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    ശൂന്യം subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    ശൂന്യം userClearVertices() VL_MT_DISABLED;
    ശൂന്യം userClearEdges() VL_MT_DISABLED;

    /// Debugging
    ശൂന്യം dump(std::ostream& os = std::cout) സ്ഥിരം VL_MT_DISABLED;
    ശൂന്യം dumpDotFile(സ്ഥിരം string& filename, bool colorAsSubgraph) സ്ഥിരം VL_MT_DISABLED;
    ശൂന്യം dumpDotFilePrefixed(സ്ഥിരം string& nameComment,
                             bool colorAsSubgraph = false) സ്ഥിരം VL_MT_DISABLED;
    ശൂന്യം dumpDotFilePrefixedAlways(സ്ഥിരം string& nameComment,
                                   bool colorAsSubgraph = false) സ്ഥിരം VL_MT_DISABLED;
    ശൂന്യം dumpEdges(std::ostream& os, സ്ഥിരം V3GraphVertex& vertex) സ്ഥിരം VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    ശൂന്യം hashGraphDebug(സ്ഥിരം അക്ഷരം* debugName) സ്ഥിരം VL_MT_DISABLED;

    സ്ഥിതം ശൂന്യം selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_എണ്ണം = 0;  // Number of vertexes in the graph
        uint64_t m_edge_എണ്ണം = 0;  // Number of edges in the grap

        ParallelismReport() = സ്ഥിരസ്ഥിതി;

    പൊതു:
        ~ParallelismReport() = സ്ഥിരസ്ഥിതി;
        ParallelismReport(സ്ഥിരം ParallelismReport&) = സ്ഥിരസ്ഥിതി;
        ParallelismReport& operator=(സ്ഥിരം ParallelismReport&) = സ്ഥിരസ്ഥിതി;

        uint64_t totalGraphCost() സ്ഥിരം { തിരികെ m_totalGraphCost; }
        uint64_t criticalPathCost() സ്ഥിരം { തിരികെ m_criticalPathCost; }
        uint64_t vertex_എണ്ണം() സ്ഥിരം { തിരികെ m_vertex_എണ്ണം; }
        uint64_t edge_എണ്ണം() സ്ഥിരം { തിരികെ m_edge_എണ്ണം; }
        ഇരട്ട parallelismFactor() സ്ഥിരം {
            തിരികെ (static_cast<ഇരട്ട>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(സ്ഥിരം V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    മായാ ശൂന്യം loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    മായാ string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
