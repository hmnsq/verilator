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

#اگر_غیر_تعریف_شدہ VERILATOR_V3_گراف_H
#تعریف_کرو VERILATOR_V3_گراف_H

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3List.h"
#شامل "V3Rtti.h"

#شامل <algorithm>
#شامل <functional>

class File_سطر;
class V3_گراف;
class V3_گراف_راس;
class V3_گراف_کنارہ;
class گراف_Acyc_کنارہ;
class ترتیب_Either_راس;
class ترتیب_Logic_راس;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3_کنارہ_Func_P = bool (*)(ثابت V3_گراف_کنارہ* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class گراف_Way final {
عوامی:
    شمار en : uint8_t {
        FORWARD = 0,
        الٹ_2 = 1,
        عدد_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    شمار en m_e;
    گراف_Way()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr گراف_Way(en _e)
        : m_e{_e} {}
    explicit constexpr گراف_Way(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"FORWARD", "REVERSE"};
        واپس نام[m_e];
    }
    // METHODS unique to this class
    constexpr گراف_Way invert() ثابت { واپس گراف_Way{m_e ^ 1}; }
    constexpr bool forward() ثابت { واپس m_e == FORWARD; }
    constexpr bool الٹ() ثابت { واپس m_e != FORWARD; }
};
constexpr bool عامل==(ثابت گراف_Way& lhs, ثابت گراف_Way& rhs) { واپس lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت گراف_Way& lhs, گراف_Way::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(گراف_Way::en lhs, ثابت گراف_Way& rhs) { واپس lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3_گراف_کنارہ VL_NOT_FINAL {
    VL_RTTI_IMPL_بنیاد(V3_گراف_کنارہ)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
عوامی:
    // ENUMS
    شمار Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
محفوظ:
    دوست class V3_گراف;
    دوست class V3_گراف_راس;
    دوست class گراف_Acyc;
    دوست class گراف_Acyc_کنارہ;

    V3ListLinks<V3_گراف_کنارہ> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3_گراف_کنارہ> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3_گراف_راس* m_fromp;  // Vertices pointing to this edge
    V3_گراف_راس* m_چوٹی;  // Vertices this edge points to
    صحیح m_وزن;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    اتحاد {
        خالی* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    خالی init(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2,
              bool cutable = false) VL_MT_DISABLED;
    خالی cut() { m_وزن = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3_گراف_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی,
                ثابت V3_گراف_کنارہ& old) VL_MT_DISABLED {
        init(graphp, fromp, چوٹی, old.m_وزن, old.m_cutable);
    }

نجی:
    V3ListLinks<V3_گراف_کنارہ>& oLinks() { واپس m_oLinks; }
    V3ListLinks<V3_گراف_کنارہ>& iLinks() { واپس m_iLinks; }

عوامی:
    // List types to store instances of this class
    using OList = V3List<V3_گراف_کنارہ, &V3_گراف_کنارہ::oLinks>;
    using IList = V3List<V3_گراف_کنارہ, &V3_گراف_کنارہ::iLinks>;

    //! Add DAG from one node to the specified node
    V3_گراف_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, چوٹی, وزن_2, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    مجازی V3_گراف_کنارہ* clone(V3_گراف* graphp, V3_گراف_راس* fromp,
                               V3_گراف_راس* چوٹی) ثابت VL_MT_DISABLED {
        واپس new V3_گراف_کنارہ{graphp, fromp, چوٹی, *this};
    }
    مجازی ~V3_گراف_کنارہ() = طےشدہ;
    // METHODS
    // Return true iff of type T
    سانچہ <typename T>
    bool is() ثابت {
        static_assert(std::is_بنیاد_کا<V3_گراف_کنارہ, T>::قدر,
                      "'T' must be a subtype of V3GraphEdge");
        واپس V3Rtti::is_Instance_کا<T>(this);
    }

    // Return cast to subtype T and assert of that type
    سانچہ <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        واپس static_cast<T*>(this);
    }
    سانچہ <typename T>
    ثابت T* as() ثابت {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        واپس static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    سانچہ <typename T>
    T* cast() {
        واپس is<T>() ? static_cast<T*>(this) : nullptr;
    }
    سانچہ <typename T>
    ثابت T* cast() ثابت {
        واپس is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    مجازی string name() ثابت VL_MT_DISABLED;
    مجازی string dotLabel() ثابت { واپس ""; }
    مجازی string dotColor() ثابت { واپس cutable() ? "yellowGreen" : "red"; }
    مجازی string dotStyle() ثابت { واپس cutable() ? "dashed" : ""; }
    مجازی صحیح ترتیب_دو_Cmp(ثابت V3_گراف_کنارہ* rhsp) ثابت VL_MT_DISABLED;
    خالی unlinkDelete() VL_MT_DISABLED;
    خالی relinkFromp(V3_گراف_راس* newFromp) VL_MT_DISABLED;
    خالی relink_چوٹی(V3_گراف_راس* new_چوٹی) VL_MT_DISABLED;
    // ACCESSORS
    صحیح وزن_2() ثابت { واپس m_وزن; }
    خالی وزن_2(صحیح وزن_2) { m_وزن = وزن_2; }
    bool cutable() ثابت { واپس m_cutable; }
    خالی cutable(bool cutable) { m_cutable = cutable; }
    خالی userp(خالی* user) { m_userp = user; }
    خالی* userp() ثابت { واپس m_userp; }
    خالی user(uint64_t user) { m_user = user; }
    uint64_t user() ثابت { واپس m_user; }
    V3_گراف_راس* fromp() ثابت { واپس m_fromp; }
    V3_گراف_راس* چوٹی() ثابت { واپس m_چوٹی; }
    سانچہ <گراف_Way::en N_Way>
    V3_گراف_راس* furtherp() ثابت {
        واپس N_Way == گراف_Way::FORWARD ? چوٹی() : fromp();
    }
    // STATIC ACCESSORS
    ساکن bool followNotCutable(ثابت V3_گراف_کنارہ* edgep) { واپس !edgep->m_cutable; }
    ساکن bool followAlwaysTrue(ثابت V3_گراف_کنارہ*) { واپس true; }
};

//============================================================================

class V3_گراف_راس VL_NOT_FINAL {
    VL_RTTI_IMPL_بنیاد(V3_گراف_راس)
    دوست class V3_گراف;
    دوست class V3_گراف_کنارہ;
    دوست class گراف_Acyc;
    دوست class گراف_Alg_Rank;
    دوست class گراف_Alg_Rank_گہرائی;
    V3ListLinks<V3_گراف_راس> m_links;  // List links to store instances of this class
    V3_گراف_کنارہ::OList m_outs;  // List of outbound edges
    V3_گراف_کنارہ::IList m_ins;  // List of inbound edges
    دوہرا m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    اتحاد {
        خالی* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    خالی fanout(دوہرا fanout) { m_fanout = fanout; }

محفوظ:
    // CONSTRUCTORS
    V3_گراف_راس(V3_گراف* graphp, ثابت V3_گراف_راس& old) VL_MT_DISABLED;

نجی:
    V3ListLinks<V3_گراف_راس>& links() { واپس m_links; }

عوامی:
    // List types to store instances of this class
    using List = V3List<V3_گراف_راس, &V3_گراف_راس::links>;

    explicit V3_گراف_راس(V3_گراف* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    مجازی V3_گراف_راس* clone(V3_گراف* graphp) ثابت VL_MT_DISABLED {
        واپس new V3_گراف_راس{graphp, *this};
    }
    مجازی ~V3_گراف_راس() = طےشدہ;
    خالی unlink_کنارے(V3_گراف* graphp) VL_MT_DISABLED;
    خالی unlinkDelete(V3_گراف* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    سانچہ <typename T>
    bool is() ثابت {
        static_assert(std::is_بنیاد_کا<V3_گراف_راس, T>::قدر,
                      "'T' must be a subtype of V3GraphVertex");
        واپس V3Rtti::is_Instance_کا<T>(this);
    }

    // Return cast to subtype T and assert of that type
    سانچہ <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        واپس static_cast<T*>(this);
    }
    سانچہ <typename T>
    ثابت T* as() ثابت {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        واپس static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    سانچہ <typename T>
    T* cast() {
        واپس is<T>() ? static_cast<T*>(this) : nullptr;
    }
    سانچہ <typename T>
    ثابت T* cast() ثابت {
        واپس is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    // ACCESSORS
    مجازی string name() ثابت { واپس ""; }
    مجازی string dotColor() ثابت { واپس "black"; }
    مجازی string dotShape() ثابت { واپس ""; }
    مجازی string dotStyle() ثابت { واپس ""; }
    مجازی string dotName() ثابت { واپس ""; }
    مجازی string dotRank() ثابت { واپس ""; }
    مجازی uint32_t rankAdder() ثابت { واپس 1; }
    مجازی File_سطر* fileline() ثابت { واپس nullptr; }  // nullptr for unknown
    مجازی صحیح ترتیب_دو_Cmp(ثابت V3_گراف_راس* rhsp) ثابت {
        // LHS goes first if of lower rank, or lower fanout
        اگر (m_rank < rhsp->m_rank) واپس -1;
        اگر (m_rank > rhsp->m_rank) واپس 1;
        اگر (m_fanout < rhsp->m_fanout) واپس -1;
        اگر (m_fanout > rhsp->m_fanout) واپس 1;
        واپس 0;
    }
    uint32_t color() ثابت { واپس m_color; }
    خالی color(uint32_t color) { m_color = color; }
    uint32_t rank() ثابت { واپس m_rank; }
    خالی rank(uint32_t rank) { m_rank = rank; }
    دوہرا fanout() ثابت { واپس m_fanout; }
    خالی user(uint32_t user) { m_user = user; }
    uint32_t user() ثابت VL_MT_STABLE { واپس m_user; }
    خالی userp(خالی* userp) { m_userp = userp; }
    خالی* userp() ثابت VL_MT_STABLE { واپس m_userp; }
    V3_گراف_کنارہ::IList& اندر_کنارے() { واپس m_ins; }
    ثابت V3_گراف_کنارہ::IList& اندر_کنارے() ثابت { واپس m_ins; }
    سانچہ <گراف_Way::en N_Way>
    اندرونی خودکار& کنارے();
    سانچہ <گراف_Way::en N_Way>
    اندرونی ثابت خودکار& کنارے() ثابت;
    bool اندر_خالی_خانے() ثابت { واپس m_ins.خالی_خانے(); }
    bool اندر_Size1() ثابت { واپس m_ins.hasSingleElement(); }
    V3_گراف_کنارہ::OList& برآمد_کنارے() { واپس m_outs; }
    ثابت V3_گراف_کنارہ::OList& برآمد_کنارے() ثابت { واپس m_outs; }
    bool برآمد_خالی_خانے() ثابت { واپس m_outs.خالی_خانے(); }
    bool برآمد_Size1() ثابت { واپس m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    خالی v3error_انتہا(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    خالی v3error_انتہا_Fatal(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    خالی reroute_کنارے(V3_گراف* graphp) VL_MT_DISABLED;
};

std::ostream& عامل<<(std::ostream& os, V3_گراف_راس* vertexp) VL_MT_DISABLED;

سانچہ <>
اندرونی خودکار& V3_گراف_راس::کنارے<گراف_Way::FORWARD>() {
    واپس m_outs;
}

سانچہ <>
اندرونی خودکار& V3_گراف_راس::کنارے<گراف_Way::الٹ_2>() {
    واپس m_ins;
}

سانچہ <>
اندرونی ثابت خودکار& V3_گراف_راس::کنارے<گراف_Way::FORWARD>() ثابت {
    واپس m_outs;
}

سانچہ <>
اندرونی ثابت خودکار& V3_گراف_راس::کنارے<گراف_Way::الٹ_2>() ثابت {
    واپس m_ins;
}

//============================================================================

class V3_گراف VL_NOT_FINAL {
    // MEMBERS
    V3_گراف_راس::List m_vertices;  // All vertices

محفوظ:
    دوست class V3_گراف_راس;
    دوست class V3_گراف_کنارہ;
    دوست class گراف_Acyc;
    // METHODS
    دوہرا ترتیب_DFSIterate(V3_گراف_راس* vertexp) VL_MT_DISABLED;
    خالی dump_کنارہ(std::ostream& os, ثابت V3_گراف_راس& راس,
                  ثابت V3_گراف_کنارہ& کنارہ) ثابت VL_MT_DISABLED;
    // ACCESSORS

عوامی:
    V3_گراف() VL_MT_DISABLED;
    مجازی ~V3_گراف() VL_MT_DISABLED;
    مجازی string dotRankDir() ثابت { واپس "TB"; }  // rankdir for dot plotting

    // METHODS
    خالی clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool خالی_خانے() ثابت { واپس m_vertices.خالی_خانے(); }
    V3_گراف_راس::List& vertices() { واپس m_vertices; }
    ثابت V3_گراف_راس::List& vertices() ثابت { واپس m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3_کنارہ_Func_P کنارہ_Funcp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    خالی stronglyConnected(V3_کنارہ_Func_P کنارہ_Funcp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    خالی rank(V3_کنارہ_Func_P کنارہ_Funcp) VL_MT_DISABLED;
    خالی rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    خالی rankMin(V3_کنارہ_Func_P کنارہ_Funcp) VL_MT_DISABLED;
    خالی rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    خالی ترتیب_دو_Vertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    خالی ترتیب_دو_کنارے() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    خالی ترتیب() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    خالی ترتیب_Pre_Ranked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    خالی acyclic(V3_کنارہ_Func_P کنارہ_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    خالی remove_Redundant_کنارے_Max(V3_کنارہ_Func_P کنارہ_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    خالی remove_Redundant_کنارے_مجموع(V3_کنارہ_Func_P کنارہ_Funcp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    خالی remove_متعدی_کنارے() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3_کنارہ_Func_P کنارہ_Funcp, V3_گراف_راس* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    خالی subtreeLoops(V3_کنارہ_Func_P کنارہ_Funcp, V3_گراف_راس* vertexp,
                      V3_گراف* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    خالی userClearVertices() VL_MT_DISABLED;
    خالی user_Clear_کنارے() VL_MT_DISABLED;

    /// Debugging
    خالی dump(std::ostream& os = std::cout) ثابت VL_MT_DISABLED;
    خالی dumpDotFile(ثابت string& filename, bool colorAsSubgraph) ثابت VL_MT_DISABLED;
    خالی dumpDotFilePrefixed(ثابت string& nameComment,
                             bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    خالی dumpDotFilePrefixedAlways(ثابت string& nameComment,
                                   bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    خالی dump_کنارے(std::ostream& os, ثابت V3_گراف_راس& راس) ثابت VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    خالی hash_گراف_Debug(ثابت حرف* debugName) ثابت VL_MT_DISABLED;

    ساکن خالی selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        دوست class گراف_Alg_Parallelism_Report;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_کل_گراف_لاگت = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_critical_راستہ_لاگت = 0;

        uint64_t m_راس_گنتی = 0;  // Number of vertexes in the graph
        uint64_t m_کنارہ_گنتی = 0;  // Number of edges in the grap

        ParallelismReport() = طےشدہ;

    عوامی:
        ~ParallelismReport() = طےشدہ;
        ParallelismReport(ثابت ParallelismReport&) = طےشدہ;
        ParallelismReport& عامل=(ثابت ParallelismReport&) = طےشدہ;

        uint64_t کل_گراف_لاگت() ثابت { واپس m_کل_گراف_لاگت; }
        uint64_t critical_راستہ_لاگت() ثابت { واپس m_critical_راستہ_لاگت; }
        uint64_t راس_گنتی() ثابت { واپس m_راس_گنتی; }
        uint64_t کنارہ_گنتی() ثابت { واپس m_کنارہ_گنتی; }
        دوہرا parallelism_ضربی_عدد() ثابت {
            واپس (static_cast<دوہرا>(m_کل_گراف_لاگت) / m_critical_راستہ_لاگت);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ثابت V3_گراف_راس*)> راس_لاگت) VL_MT_DISABLED;

    // CALLBACKS
    مجازی خالی loops_پیغام_Cb(V3_گراف_راس* vertexp, V3_کنارہ_Func_P کنارہ_Funcp) VL_MT_DISABLED;
    مجازی string loops_راس_Cb(V3_گراف_راس* vertexp) VL_MT_DISABLED;
};

//============================================================================

#اگر_ختم  // Guard
