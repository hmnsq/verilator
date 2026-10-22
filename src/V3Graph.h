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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3List.h"
#ଅନ୍ତର୍ଭାଵ "V3Rtti.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <functional>

class File_ଧାଡ଼ି;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(ଅଚଳ V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        ଅଙ୍କ_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    ଗଣନା en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"FORWARD", "REVERSE"};
        ଫେରନ୍ତୁ names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() ଅଚଳ { ଫେରନ୍ତୁ GraphWay{m_e ^ 1}; }
    constexpr bool forward() ଅଚଳ { ଫେରନ୍ତୁ m_e == FORWARD; }
    constexpr bool reverse() ଅଚଳ { ଫେରନ୍ତୁ m_e != FORWARD; }
};
constexpr bool operator==(ଅଚଳ GraphWay& lhs, ଅଚଳ GraphWay& rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ଅଚଳ GraphWay& lhs, GraphWay::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, ଅଚଳ GraphWay& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
ସର୍ବସାଧାରଣ:
    // ENUMS
    ଗଣନା Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
ସୁରକ୍ଷିତ:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_ଶୀର୍ଷ;  // Vertices this edge points to
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    ସଂଘ {
        ଶୂନ୍ୟ* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    ଶୂନ୍ୟ init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight,
              bool cutable = false) VL_MT_DISABLED;
    ଶୂନ୍ୟ cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ,
                ଅଚଳ V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, ଶୀର୍ଷ, old.m_weight, old.m_cutable);
    }

ବ୍ୟକ୍ତିଗତ:
    V3ListLinks<V3GraphEdge>& oLinks() { ଫେରନ୍ତୁ m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { ଫେରନ୍ତୁ m_iLinks; }

ସର୍ବସାଧାରଣ:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, ଶୀର୍ଷ, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    ଆଭାସୀ V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* ଶୀର୍ଷ) ଅଚଳ VL_MT_DISABLED {
        ଫେରନ୍ତୁ new V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, *this};
    }
    ଆଭାସୀ ~V3GraphEdge() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ଅଚଳ {
        static_assert(std::is_base_of<V3GraphEdge, T>::ମୂଲ୍ୟ,
                      "'T' must be a subtype of V3GraphEdge");
        ଫେରନ୍ତୁ V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ଫେରନ୍ତୁ static_cast<T*>(this);
    }
    template <typename T>
    ଅଚଳ T* as() ଅଚଳ {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ଫେରନ୍ତୁ static_cast<ଅଚଳ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        ଫେରନ୍ତୁ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ଅଚଳ T* cast() ଅଚଳ {
        ଫେରନ୍ତୁ is<T>() ? static_cast<ଅଚଳ T*>(this) : nullptr;
    }

    ଆଭାସୀ string name() ଅଚଳ VL_MT_DISABLED;
    ଆଭାସୀ string dotLabel() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    ଆଭାସୀ string dotColor() ଅଚଳ { ଫେରନ୍ତୁ cutable() ? "yellowGreen" : "red"; }
    ଆଭାସୀ string dotStyle() ଅଚଳ { ଫେରନ୍ତୁ cutable() ? "dashed" : ""; }
    ଆଭାସୀ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sortCmp(ଅଚଳ V3GraphEdge* rhsp) ଅଚଳ VL_MT_DISABLED;
    ଶୂନ୍ୟ unlinkDelete() VL_MT_DISABLED;
    ଶୂନ୍ୟ relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    ଶୂନ୍ୟ relink_ଶୀର୍ଷ(V3GraphVertex* new_ଶୀର୍ଷ) VL_MT_DISABLED;
    // ACCESSORS
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight() ଅଚଳ { ଫେରନ୍ତୁ m_weight; }
    ଶୂନ୍ୟ weight(ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) { m_weight = weight; }
    bool cutable() ଅଚଳ { ଫେରନ୍ତୁ m_cutable; }
    ଶୂନ୍ୟ cutable(bool cutable) { m_cutable = cutable; }
    ଶୂନ୍ୟ userp(ଶୂନ୍ୟ* user) { m_userp = user; }
    ଶୂନ୍ୟ* userp() ଅଚଳ { ଫେରନ୍ତୁ m_userp; }
    ଶୂନ୍ୟ user(uint64_t user) { m_user = user; }
    uint64_t user() ଅଚଳ { ଫେରନ୍ତୁ m_user; }
    V3GraphVertex* fromp() ଅଚଳ { ଫେରନ୍ତୁ m_fromp; }
    V3GraphVertex* ଶୀର୍ଷ() ଅଚଳ { ଫେରନ୍ତୁ m_ଶୀର୍ଷ; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() ଅଚଳ {
        ଫେରନ୍ତୁ N_Way == GraphWay::FORWARD ? ଶୀର୍ଷ() : fromp();
    }
    // STATIC ACCESSORS
    ସ୍ଥିର bool followNotCutable(ଅଚଳ V3GraphEdge* edgep) { ଫେରନ୍ତୁ !edgep->m_cutable; }
    ସ୍ଥିର bool followAlwaysTrue(ଅଚଳ V3GraphEdge*) { ଫେରନ୍ତୁ true; }
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
    ଦ୍ୱିଗୁଣ m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    ସଂଘ {
        ଶୂନ୍ୟ* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    ଶୂନ୍ୟ fanout(ଦ୍ୱିଗୁଣ fanout) { m_fanout = fanout; }

ସୁରକ୍ଷିତ:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, ଅଚଳ V3GraphVertex& old) VL_MT_DISABLED;

ବ୍ୟକ୍ତିଗତ:
    V3ListLinks<V3GraphVertex>& links() { ଫେରନ୍ତୁ m_links; }

ସର୍ବସାଧାରଣ:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    ଆଭାସୀ V3GraphVertex* clone(V3Graph* graphp) ଅଚଳ VL_MT_DISABLED {
        ଫେରନ୍ତୁ new V3GraphVertex{graphp, *this};
    }
    ଆଭାସୀ ~V3GraphVertex() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    ଶୂନ୍ୟ unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ଅଚଳ {
        static_assert(std::is_base_of<V3GraphVertex, T>::ମୂଲ୍ୟ,
                      "'T' must be a subtype of V3GraphVertex");
        ଫେରନ୍ତୁ V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ଫେରନ୍ତୁ static_cast<T*>(this);
    }
    template <typename T>
    ଅଚଳ T* as() ଅଚଳ {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ଫେରନ୍ତୁ static_cast<ଅଚଳ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        ଫେରନ୍ତୁ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ଅଚଳ T* cast() ଅଚଳ {
        ଫେରନ୍ତୁ is<T>() ? static_cast<ଅଚଳ T*>(this) : nullptr;
    }

    // ACCESSORS
    ଆଭାସୀ string name() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    ଆଭାସୀ string dotColor() ଅଚଳ { ଫେରନ୍ତୁ "black"; }
    ଆଭାସୀ string dotShape() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    ଆଭାସୀ string dotStyle() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    ଆଭାସୀ string dotName() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    ଆଭାସୀ string dotRank() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    ଆଭାସୀ uint32_t rankAdder() ଅଚଳ { ଫେରନ୍ତୁ 1; }
    ଆଭାସୀ File_ଧାଡ଼ି* fileline() ଅଚଳ { ଫେରନ୍ତୁ nullptr; }  // nullptr for unknown
    ଆଭାସୀ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sortCmp(ଅଚଳ V3GraphVertex* rhsp) ଅଚଳ {
        // LHS goes first if of lower rank, or lower fanout
        ଯଦି (m_rank < rhsp->m_rank) ଫେରନ୍ତୁ -1;
        ଯଦି (m_rank > rhsp->m_rank) ଫେରନ୍ତୁ 1;
        ଯଦି (m_fanout < rhsp->m_fanout) ଫେରନ୍ତୁ -1;
        ଯଦି (m_fanout > rhsp->m_fanout) ଫେରନ୍ତୁ 1;
        ଫେରନ୍ତୁ 0;
    }
    uint32_t color() ଅଚଳ { ଫେରନ୍ତୁ m_color; }
    ଶୂନ୍ୟ color(uint32_t color) { m_color = color; }
    uint32_t rank() ଅଚଳ { ଫେରନ୍ତୁ m_rank; }
    ଶୂନ୍ୟ rank(uint32_t rank) { m_rank = rank; }
    ଦ୍ୱିଗୁଣ fanout() ଅଚଳ { ଫେରନ୍ତୁ m_fanout; }
    ଶୂନ୍ୟ user(uint32_t user) { m_user = user; }
    uint32_t user() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_user; }
    ଶୂନ୍ୟ userp(ଶୂନ୍ୟ* userp) { m_userp = userp; }
    ଶୂନ୍ୟ* userp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_userp; }
    V3GraphEdge::IList& inEdges() { ଫେରନ୍ତୁ m_ins; }
    ଅଚଳ V3GraphEdge::IList& inEdges() ଅଚଳ { ଫେରନ୍ତୁ m_ins; }
    template <GraphWay::en N_Way>
    ଧାଡ଼ିଭିତରେ auto& edges();
    template <GraphWay::en N_Way>
    ଧାଡ଼ିଭିତରେ ଅଚଳ auto& edges() ଅଚଳ;
    bool inEmpty() ଅଚଳ { ଫେରନ୍ତୁ m_ins.empty(); }
    bool inSize1() ଅଚଳ { ଫେରନ୍ତୁ m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { ଫେରନ୍ତୁ m_outs; }
    ଅଚଳ V3GraphEdge::OList& outEdges() ଅଚଳ { ଫେରନ୍ତୁ m_outs; }
    bool outEmpty() ଅଚଳ { ଫେରନ୍ତୁ m_outs.empty(); }
    bool outSize1() ଅଚଳ { ଫେରନ୍ତୁ m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    ଶୂନ୍ୟ v3errorEnd(ଅଚଳ std::ostringstream& str) ଅଚଳ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    ଶୂନ୍ୟ v3errorEndFatal(ଅଚଳ std::ostringstream& str) ଅଚଳ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    ଶୂନ୍ୟ rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
ଧାଡ଼ିଭିତରେ auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    ଫେରନ୍ତୁ m_outs;
}

template <>
ଧାଡ଼ିଭିତରେ auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    ଫେରନ୍ତୁ m_ins;
}

template <>
ଧାଡ଼ିଭିତରେ ଅଚଳ auto& V3GraphVertex::edges<GraphWay::FORWARD>() ଅଚଳ {
    ଫେରନ୍ତୁ m_outs;
}

template <>
ଧାଡ଼ିଭିତରେ ଅଚଳ auto& V3GraphVertex::edges<GraphWay::REVERSE>() ଅଚଳ {
    ଫେରନ୍ତୁ m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

ସୁରକ୍ଷିତ:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    ଦ୍ୱିଗୁଣ orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpEdge(std::ostream& os, ଅଚଳ V3GraphVertex& vertex,
                  ଅଚଳ V3GraphEdge& edge) ଅଚଳ VL_MT_DISABLED;
    // ACCESSORS

ସର୍ବସାଧାରଣ:
    V3Graph() VL_MT_DISABLED;
    ଆଭାସୀ ~V3Graph() VL_MT_DISABLED;
    ଆଭାସୀ string dotRankDir() ଅଚଳ { ଫେରନ୍ତୁ "TB"; }  // rankdir for dot plotting

    // METHODS
    ଶୂନ୍ୟ clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() ଅଚଳ { ଫେରନ୍ତୁ m_vertices.empty(); }
    V3GraphVertex::List& vertices() { ଫେରନ୍ତୁ m_vertices; }
    ଅଚଳ V3GraphVertex::List& vertices() ଅଚଳ { ଫେରନ୍ତୁ m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    ଶୂନ୍ୟ stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ଶୂନ୍ୟ rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    ଶୂନ୍ୟ rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ଶୂନ୍ୟ rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    ଶୂନ୍ୟ rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    ଶୂନ୍ୟ sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    ଶୂନ୍ୟ sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    ଶୂନ୍ୟ order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    ଶୂନ୍ୟ orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    ଶୂନ୍ୟ acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    ଶୂନ୍ୟ removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    ଶୂନ୍ୟ removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    ଶୂନ୍ୟ removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    ଶୂନ୍ୟ subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    ଶୂନ୍ୟ userClearVertices() VL_MT_DISABLED;
    ଶୂନ୍ୟ userClearEdges() VL_MT_DISABLED;

    /// Debugging
    ଶୂନ୍ୟ dump(std::ostream& os = std::cout) ଅଚଳ VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpDotFile(ଅଚଳ string& filename, bool colorAsSubgraph) ଅଚଳ VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpDotFilePrefixed(ଅଚଳ string& nameComment,
                             bool colorAsSubgraph = false) ଅଚଳ VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpDotFilePrefixedAlways(ଅଚଳ string& nameComment,
                                   bool colorAsSubgraph = false) ଅଚଳ VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpEdges(std::ostream& os, ଅଚଳ V3GraphVertex& vertex) ଅଚଳ VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    ଶୂନ୍ୟ hashGraphDebug(ଅଚଳ ଅକ୍ଷର* debugName) ଅଚଳ VL_MT_DISABLED;

    ସ୍ଥିର ଶୂନ୍ୟ selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_ଗଣନା = 0;  // Number of vertexes in the graph
        uint64_t m_edge_ଗଣନା = 0;  // Number of edges in the grap

        ParallelismReport() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ସର୍ବସାଧାରଣ:
        ~ParallelismReport() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ParallelismReport(ଅଚଳ ParallelismReport&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ParallelismReport& operator=(ଅଚଳ ParallelismReport&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

        uint64_t totalGraphCost() ଅଚଳ { ଫେରନ୍ତୁ m_totalGraphCost; }
        uint64_t criticalPathCost() ଅଚଳ { ଫେରନ୍ତୁ m_criticalPathCost; }
        uint64_t vertex_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_vertex_ଗଣନା; }
        uint64_t edge_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_edge_ଗଣନା; }
        ଦ୍ୱିଗୁଣ parallelismFactor() ଅଚଳ {
            ଫେରନ୍ତୁ (static_cast<ଦ୍ୱିଗୁଣ>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ଅଚଳ V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    ଆଭାସୀ ଶୂନ୍ୟ loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    ଆଭାସୀ string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
