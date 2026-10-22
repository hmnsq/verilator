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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3List.h"
#ಅನ್ತರ್ಭಾವ "V3Rtti.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <functional>

class File_ಸಾಲು;
class V3Graph;
class V3GraphVertex;
class V3GraphEdge;
class GraphAcycEdge;
class OrderEitherVertex;
class OrderLogicVertex;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3EdgeFuncP = bool (*)(ಧ್ರುವ V3GraphEdge* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class GraphWay final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        FORWARD = 0,
        REVERSE = 1,
        ಅಂಕೆ_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    ಗಣನಾ en m_e;
    GraphWay()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr GraphWay(en _e)
        : m_e{_e} {}
    explicit constexpr GraphWay(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"FORWARD", "REVERSE"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    // METHODS unique to this class
    constexpr GraphWay invert() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ GraphWay{m_e ^ 1}; }
    constexpr bool forward() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == FORWARD; }
    constexpr bool reverse() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e != FORWARD; }
};
constexpr bool operator==(ಧ್ರುವ GraphWay& lhs, ಧ್ರುವ GraphWay& rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ಧ್ರುವ GraphWay& lhs, GraphWay::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(GraphWay::en lhs, ಧ್ರುವ GraphWay& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3GraphEdge VL_NOT_FINAL {
    VL_RTTI_IMPL_BASE(V3GraphEdge)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
ಸಾರ್ವಜನಿಕ:
    // ENUMS
    ಗಣನಾ Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
ರಕ್ಷಿತ:
    friend class V3Graph;
    friend class V3GraphVertex;
    friend class GraphAcyc;
    friend class GraphAcycEdge;

    V3ListLinks<V3GraphEdge> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3GraphEdge> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3GraphVertex* m_fromp;  // Vertices pointing to this edge
    V3GraphVertex* m_ತುದಿ;  // Vertices this edge points to
    ಪೂರ್ಣಾಙ್ಕ m_weight;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    ಸಙ್ಘ {
        ಶೂನ್ಯ* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    ಶೂನ್ಯ init(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight,
              bool cutable = false) VL_MT_DISABLED;
    ಶೂನ್ಯ cut() { m_weight = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ,
                ಧ್ರುವ V3GraphEdge& old) VL_MT_DISABLED {
        init(graphp, fromp, ತುದಿ, old.m_weight, old.m_cutable);
    }

ಗುಪ್ತ:
    V3ListLinks<V3GraphEdge>& oLinks() { ಪ್ರತ್ಯಾಗಮ m_oLinks; }
    V3ListLinks<V3GraphEdge>& iLinks() { ಪ್ರತ್ಯಾಗಮ m_iLinks; }

ಸಾರ್ವಜನಿಕ:
    // List types to store instances of this class
    using OList = V3List<V3GraphEdge, &V3GraphEdge::oLinks>;
    using IList = V3List<V3GraphEdge, &V3GraphEdge::iLinks>;

    //! Add DAG from one node to the specified node
    V3GraphEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, ತುದಿ, weight, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    ಆಭಾಸ V3GraphEdge* clone(V3Graph* graphp, V3GraphVertex* fromp,
                               V3GraphVertex* ತುದಿ) ಧ್ರುವ VL_MT_DISABLED {
        ಪ್ರತ್ಯಾಗಮ new V3GraphEdge{graphp, fromp, ತುದಿ, *this};
    }
    ಆಭಾಸ ~V3GraphEdge() = ಸ್ವಭಾವ;
    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ಧ್ರುವ {
        static_assert(std::is_base_of<V3GraphEdge, T>::ಮೌಲ್ಯ,
                      "'T' must be a subtype of V3GraphEdge");
        ಪ್ರತ್ಯಾಗಮ V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ಪ್ರತ್ಯಾಗಮ static_cast<T*>(this);
    }
    template <typename T>
    ಧ್ರುವ T* as() ಧ್ರುವ {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ಪ್ರತ್ಯಾಗಮ static_cast<ಧ್ರುವ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        ಪ್ರತ್ಯಾಗಮ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ಧ್ರುವ T* cast() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ is<T>() ? static_cast<ಧ್ರುವ T*>(this) : nullptr;
    }

    ಆಭಾಸ string name() ಧ್ರುವ VL_MT_DISABLED;
    ಆಭಾಸ string dotLabel() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ""; }
    ಆಭಾಸ string dotColor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ cutable() ? "yellowGreen" : "red"; }
    ಆಭಾಸ string dotStyle() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ cutable() ? "dashed" : ""; }
    ಆಭಾಸ ಪೂರ್ಣಾಙ್ಕ sortCmp(ಧ್ರುವ V3GraphEdge* rhsp) ಧ್ರುವ VL_MT_DISABLED;
    ಶೂನ್ಯ unlinkDelete() VL_MT_DISABLED;
    ಶೂನ್ಯ relinkFromp(V3GraphVertex* newFromp) VL_MT_DISABLED;
    ಶೂನ್ಯ relink_ತುದಿ(V3GraphVertex* new_ತುದಿ) VL_MT_DISABLED;
    // ACCESSORS
    ಪೂರ್ಣಾಙ್ಕ weight() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_weight; }
    ಶೂನ್ಯ weight(ಪೂರ್ಣಾಙ್ಕ weight) { m_weight = weight; }
    bool cutable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cutable; }
    ಶೂನ್ಯ cutable(bool cutable) { m_cutable = cutable; }
    ಶೂನ್ಯ userp(ಶೂನ್ಯ* user) { m_userp = user; }
    ಶೂನ್ಯ* userp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_userp; }
    ಶೂನ್ಯ user(uint64_t user) { m_user = user; }
    uint64_t user() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_user; }
    V3GraphVertex* fromp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fromp; }
    V3GraphVertex* ತುದಿ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ತುದಿ; }
    template <GraphWay::en N_Way>
    V3GraphVertex* furtherp() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ N_Way == GraphWay::FORWARD ? ತುದಿ() : fromp();
    }
    // STATIC ACCESSORS
    ಸ್ಥಿರ bool followNotCutable(ಧ್ರುವ V3GraphEdge* edgep) { ಪ್ರತ್ಯಾಗಮ !edgep->m_cutable; }
    ಸ್ಥಿರ bool followAlwaysTrue(ಧ್ರುವ V3GraphEdge*) { ಪ್ರತ್ಯಾಗಮ true; }
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
    ದ್ವಿಗುಣ m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    ಸಙ್ಘ {
        ಶೂನ್ಯ* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    ಶೂನ್ಯ fanout(ದ್ವಿಗುಣ fanout) { m_fanout = fanout; }

ರಕ್ಷಿತ:
    // CONSTRUCTORS
    V3GraphVertex(V3Graph* graphp, ಧ್ರುವ V3GraphVertex& old) VL_MT_DISABLED;

ಗುಪ್ತ:
    V3ListLinks<V3GraphVertex>& links() { ಪ್ರತ್ಯಾಗಮ m_links; }

ಸಾರ್ವಜನಿಕ:
    // List types to store instances of this class
    using List = V3List<V3GraphVertex, &V3GraphVertex::links>;

    explicit V3GraphVertex(V3Graph* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    ಆಭಾಸ V3GraphVertex* clone(V3Graph* graphp) ಧ್ರುವ VL_MT_DISABLED {
        ಪ್ರತ್ಯಾಗಮ new V3GraphVertex{graphp, *this};
    }
    ಆಭಾಸ ~V3GraphVertex() = ಸ್ವಭಾವ;
    ಶೂನ್ಯ unlinkEdges(V3Graph* graphp) VL_MT_DISABLED;
    ಶೂನ್ಯ unlinkDelete(V3Graph* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    template <typename T>
    bool is() ಧ್ರುವ {
        static_assert(std::is_base_of<V3GraphVertex, T>::ಮೌಲ್ಯ,
                      "'T' must be a subtype of V3GraphVertex");
        ಪ್ರತ್ಯಾಗಮ V3Rtti::isInstanceOf<T>(this);
    }

    // Return cast to subtype T and assert of that type
    template <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ಪ್ರತ್ಯಾಗಮ static_cast<T*>(this);
    }
    template <typename T>
    ಧ್ರುವ T* as() ಧ್ರುವ {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ಪ್ರತ್ಯಾಗಮ static_cast<ಧ್ರುವ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    template <typename T>
    T* cast() {
        ಪ್ರತ್ಯಾಗಮ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    template <typename T>
    ಧ್ರುವ T* cast() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ is<T>() ? static_cast<ಧ್ರುವ T*>(this) : nullptr;
    }

    // ACCESSORS
    ಆಭಾಸ string name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ""; }
    ಆಭಾಸ string dotColor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "black"; }
    ಆಭಾಸ string dotShape() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ""; }
    ಆಭಾಸ string dotStyle() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ""; }
    ಆಭಾಸ string dotName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ""; }
    ಆಭಾಸ string dotRank() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ""; }
    ಆಭಾಸ uint32_t rankAdder() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ 1; }
    ಆಭಾಸ File_ಸಾಲು* fileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ nullptr; }  // nullptr for unknown
    ಆಭಾಸ ಪೂರ್ಣಾಙ್ಕ sortCmp(ಧ್ರುವ V3GraphVertex* rhsp) ಧ್ರುವ {
        // LHS goes first if of lower rank, or lower fanout
        ಯದಿ (m_rank < rhsp->m_rank) ಪ್ರತ್ಯಾಗಮ -1;
        ಯದಿ (m_rank > rhsp->m_rank) ಪ್ರತ್ಯಾಗಮ 1;
        ಯದಿ (m_fanout < rhsp->m_fanout) ಪ್ರತ್ಯಾಗಮ -1;
        ಯದಿ (m_fanout > rhsp->m_fanout) ಪ್ರತ್ಯಾಗಮ 1;
        ಪ್ರತ್ಯಾಗಮ 0;
    }
    uint32_t color() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_color; }
    ಶೂನ್ಯ color(uint32_t color) { m_color = color; }
    uint32_t rank() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rank; }
    ಶೂನ್ಯ rank(uint32_t rank) { m_rank = rank; }
    ದ್ವಿಗುಣ fanout() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fanout; }
    ಶೂನ್ಯ user(uint32_t user) { m_user = user; }
    uint32_t user() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_user; }
    ಶೂನ್ಯ userp(ಶೂನ್ಯ* userp) { m_userp = userp; }
    ಶೂನ್ಯ* userp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_userp; }
    V3GraphEdge::IList& inEdges() { ಪ್ರತ್ಯಾಗಮ m_ins; }
    ಧ್ರುವ V3GraphEdge::IList& inEdges() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ins; }
    template <GraphWay::en N_Way>
    ಅನ್ತಃಸ್ಥ auto& edges();
    template <GraphWay::en N_Way>
    ಅನ್ತಃಸ್ಥ ಧ್ರುವ auto& edges() ಧ್ರುವ;
    bool inEmpty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ins.empty(); }
    bool inSize1() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ins.hasSingleElement(); }
    V3GraphEdge::OList& outEdges() { ಪ್ರತ್ಯಾಗಮ m_outs; }
    ಧ್ರುವ V3GraphEdge::OList& outEdges() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outs; }
    bool outEmpty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outs.empty(); }
    bool outSize1() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    ಶೂನ್ಯ v3errorEnd(ಧ್ರುವ std::ostringstream& str) ಧ್ರುವ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    ಶೂನ್ಯ v3errorEndFatal(ಧ್ರುವ std::ostringstream& str) ಧ್ರುವ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    ಶೂನ್ಯ rerouteEdges(V3Graph* graphp) VL_MT_DISABLED;
};

std::ostream& operator<<(std::ostream& os, V3GraphVertex* vertexp) VL_MT_DISABLED;

template <>
ಅನ್ತಃಸ್ಥ auto& V3GraphVertex::edges<GraphWay::FORWARD>() {
    ಪ್ರತ್ಯಾಗಮ m_outs;
}

template <>
ಅನ್ತಃಸ್ಥ auto& V3GraphVertex::edges<GraphWay::REVERSE>() {
    ಪ್ರತ್ಯಾಗಮ m_ins;
}

template <>
ಅನ್ತಃಸ್ಥ ಧ್ರುವ auto& V3GraphVertex::edges<GraphWay::FORWARD>() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ m_outs;
}

template <>
ಅನ್ತಃಸ್ಥ ಧ್ರುವ auto& V3GraphVertex::edges<GraphWay::REVERSE>() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ m_ins;
}

//============================================================================

class V3Graph VL_NOT_FINAL {
    // MEMBERS
    V3GraphVertex::List m_vertices;  // All vertices

ರಕ್ಷಿತ:
    friend class V3GraphVertex;
    friend class V3GraphEdge;
    friend class GraphAcyc;
    // METHODS
    ದ್ವಿಗುಣ orderDFSIterate(V3GraphVertex* vertexp) VL_MT_DISABLED;
    ಶೂನ್ಯ dumpEdge(std::ostream& os, ಧ್ರುವ V3GraphVertex& vertex,
                  ಧ್ರುವ V3GraphEdge& edge) ಧ್ರುವ VL_MT_DISABLED;
    // ACCESSORS

ಸಾರ್ವಜನಿಕ:
    V3Graph() VL_MT_DISABLED;
    ಆಭಾಸ ~V3Graph() VL_MT_DISABLED;
    ಆಭಾಸ string dotRankDir() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "TB"; }  // rankdir for dot plotting

    // METHODS
    ಶೂನ್ಯ clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool empty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vertices.empty(); }
    V3GraphVertex::List& vertices() { ಪ್ರತ್ಯಾಗಮ m_vertices; }
    ಧ್ರುವ V3GraphVertex::List& vertices() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    ಶೂನ್ಯ stronglyConnected(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ಶೂನ್ಯ rank(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    ಶೂನ್ಯ rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ಶೂನ್ಯ rankMin(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    ಶೂನ್ಯ rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    ಶೂನ್ಯ sortVertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    ಶೂನ್ಯ sortEdges() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    ಶೂನ್ಯ order() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    ಶೂನ್ಯ orderPreRanked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    ಶೂನ್ಯ acyclic(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    ಶೂನ್ಯ removeRedundantEdgesMax(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    ಶೂನ್ಯ removeRedundantEdgesSum(V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    ಶೂನ್ಯ removeTransitiveEdges() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    ಶೂನ್ಯ subtreeLoops(V3EdgeFuncP edgeFuncp, V3GraphVertex* vertexp,
                      V3Graph* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    ಶೂನ್ಯ userClearVertices() VL_MT_DISABLED;
    ಶೂನ್ಯ userClearEdges() VL_MT_DISABLED;

    /// Debugging
    ಶೂನ್ಯ dump(std::ostream& os = std::cout) ಧ್ರುವ VL_MT_DISABLED;
    ಶೂನ್ಯ dumpDotFile(ಧ್ರುವ string& filename, bool colorAsSubgraph) ಧ್ರುವ VL_MT_DISABLED;
    ಶೂನ್ಯ dumpDotFilePrefixed(ಧ್ರುವ string& nameComment,
                             bool colorAsSubgraph = false) ಧ್ರುವ VL_MT_DISABLED;
    ಶೂನ್ಯ dumpDotFilePrefixedAlways(ಧ್ರುವ string& nameComment,
                                   bool colorAsSubgraph = false) ಧ್ರುವ VL_MT_DISABLED;
    ಶೂನ್ಯ dumpEdges(std::ostream& os, ಧ್ರುವ V3GraphVertex& vertex) ಧ್ರುವ VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    ಶೂನ್ಯ hashGraphDebug(ಧ್ರುವ ವರ್ಣ* debugName) ಧ್ರುವ VL_MT_DISABLED;

    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        friend class GraphAlgParallelismReport;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_totalGraphCost = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_criticalPathCost = 0;

        uint64_t m_vertex_ಎಣಿಕೆ = 0;  // Number of vertexes in the graph
        uint64_t m_edge_ಎಣಿಕೆ = 0;  // Number of edges in the grap

        ParallelismReport() = ಸ್ವಭಾವ;

    ಸಾರ್ವಜನಿಕ:
        ~ParallelismReport() = ಸ್ವಭಾವ;
        ParallelismReport(ಧ್ರುವ ParallelismReport&) = ಸ್ವಭಾವ;
        ParallelismReport& operator=(ಧ್ರುವ ParallelismReport&) = ಸ್ವಭಾವ;

        uint64_t totalGraphCost() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_totalGraphCost; }
        uint64_t criticalPathCost() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_criticalPathCost; }
        uint64_t vertex_ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vertex_ಎಣಿಕೆ; }
        uint64_t edge_ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_edge_ಎಣಿಕೆ; }
        ದ್ವಿಗುಣ parallelismFactor() ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ (static_cast<ದ್ವಿಗುಣ>(m_totalGraphCost) / m_criticalPathCost);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ಧ್ರುವ V3GraphVertex*)> vertexCost) VL_MT_DISABLED;

    // CALLBACKS
    ಆಭಾಸ ಶೂನ್ಯ loopsMessageCb(V3GraphVertex* vertexp, V3EdgeFuncP edgeFuncp) VL_MT_DISABLED;
    ಆಭಾಸ string loopsVertexCb(V3GraphVertex* vertexp) VL_MT_DISABLED;
};

//============================================================================

#endif  // Guard
