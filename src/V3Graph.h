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

#جے_بنا_تعریف VERILATOR_V3_گراف_H
#تعریف_کرو VERILATOR_V3_گراف_H

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3List.h"
#ضمن "V3Rtti.h"

#ضمن <algorithm>
#ضمن <functional>

class File_سطر;
class V3_گراف;
class V3_گراف_راس;
class V3_گراف_کنارا;
class گراف_Acyc_کنارا;
class ترتیب_Either_راس;
class ترتیب_Logic_راس;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3_کنارا_Func_P = bool (*)(ثابت V3_گراف_کنارا* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class گراف_Way final {
عام:
    تعداد en : uint8_t {
        FORWARD = 0,
        الٹ_2 = 1,
        عدد_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    تعداد en m_e;
    گراف_Way()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr گراف_Way(en _e)
        : m_e{_e} {}
    explicit constexpr گراف_Way(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت ناں[] = {"FORWARD", "REVERSE"};
        أرجع ناں[m_e];
    }
    // METHODS unique to this class
    constexpr گراف_Way invert() ثابت { أرجع گراف_Way{m_e ^ 1}; }
    constexpr bool forward() ثابت { أرجع m_e == FORWARD; }
    constexpr bool الٹ() ثابت { أرجع m_e != FORWARD; }
};
constexpr bool عامل==(ثابت گراف_Way& lhs, ثابت گراف_Way& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت گراف_Way& lhs, گراف_Way::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(گراف_Way::en lhs, ثابت گراف_Way& rhs) { أرجع lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3_گراف_کنارا VL_NOT_FINAL {
    VL_RTTI_IMPL_ادھار(V3_گراف_کنارا)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
عام:
    // ENUMS
    تعداد Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
محمي:
    یار class V3_گراف;
    یار class V3_گراف_راس;
    یار class گراف_Acyc;
    یار class گراف_Acyc_کنارا;

    V3ListLinks<V3_گراف_کنارا> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3_گراف_کنارا> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3_گراف_راس* m_fromp;  // Vertices pointing to this edge
    V3_گراف_راس* m_چوٹی;  // Vertices this edge points to
    صحيح m_بھار;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    اتحاد {
        فراغ* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    فراغ init(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحيح بھار_2,
              bool cutable = false) VL_MT_DISABLED;
    فراغ cut() { m_بھار = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3_گراف_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی,
                ثابت V3_گراف_کنارا& old) VL_MT_DISABLED {
        init(graphp, fromp, چوٹی, old.m_بھار, old.m_cutable);
    }

خاص:
    V3ListLinks<V3_گراف_کنارا>& oLinks() { أرجع m_oLinks; }
    V3ListLinks<V3_گراف_کنارا>& iLinks() { أرجع m_iLinks; }

عام:
    // List types to store instances of this class
    using OList = V3List<V3_گراف_کنارا, &V3_گراف_کنارا::oLinks>;
    using IList = V3List<V3_گراف_کنارا, &V3_گراف_کنارا::iLinks>;

    //! Add DAG from one node to the specified node
    V3_گراف_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحيح بھار_2,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, چوٹی, بھار_2, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    مجازی V3_گراف_کنارا* clone(V3_گراف* graphp, V3_گراف_راس* fromp,
                               V3_گراف_راس* چوٹی) ثابت VL_MT_DISABLED {
        أرجع new V3_گراف_کنارا{graphp, fromp, چوٹی, *this};
    }
    مجازی ~V3_گراف_کنارا() = افتراضي;
    // METHODS
    // Return true iff of type T
    سانچا <typename T>
    bool is() ثابت {
        static_assert(std::is_ادھار_دا<V3_گراف_کنارا, T>::قدر,
                      "'T' must be a subtype of V3GraphEdge");
        أرجع V3Rtti::is_Instance_دا<T>(this);
    }

    // Return cast to subtype T and assert of that type
    سانچا <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        أرجع static_cast<T*>(this);
    }
    سانچا <typename T>
    ثابت T* as() ثابت {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        أرجع static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    سانچا <typename T>
    T* cast() {
        أرجع is<T>() ? static_cast<T*>(this) : nullptr;
    }
    سانچا <typename T>
    ثابت T* cast() ثابت {
        أرجع is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    مجازی string name() ثابت VL_MT_DISABLED;
    مجازی string dotLabel() ثابت { أرجع ""; }
    مجازی string dotColor() ثابت { أرجع cutable() ? "yellowGreen" : "red"; }
    مجازی string dotStyle() ثابت { أرجع cutable() ? "dashed" : ""; }
    مجازی صحيح ترتیب_دیو_Cmp(ثابت V3_گراف_کنارا* rhsp) ثابت VL_MT_DISABLED;
    فراغ unlinkDelete() VL_MT_DISABLED;
    فراغ relinkFromp(V3_گراف_راس* newFromp) VL_MT_DISABLED;
    فراغ relink_چوٹی(V3_گراف_راس* new_چوٹی) VL_MT_DISABLED;
    // ACCESSORS
    صحيح بھار_2() ثابت { أرجع m_بھار; }
    فراغ بھار_2(صحيح بھار_2) { m_بھار = بھار_2; }
    bool cutable() ثابت { أرجع m_cutable; }
    فراغ cutable(bool cutable) { m_cutable = cutable; }
    فراغ userp(فراغ* user) { m_userp = user; }
    فراغ* userp() ثابت { أرجع m_userp; }
    فراغ user(uint64_t user) { m_user = user; }
    uint64_t user() ثابت { أرجع m_user; }
    V3_گراف_راس* fromp() ثابت { أرجع m_fromp; }
    V3_گراف_راس* چوٹی() ثابت { أرجع m_چوٹی; }
    سانچا <گراف_Way::en N_Way>
    V3_گراف_راس* furtherp() ثابت {
        أرجع N_Way == گراف_Way::FORWARD ? چوٹی() : fromp();
    }
    // STATIC ACCESSORS
    ساكن bool followNotCutable(ثابت V3_گراف_کنارا* edgep) { أرجع !edgep->m_cutable; }
    ساكن bool followAlwaysTrue(ثابت V3_گراف_کنارا*) { أرجع true; }
};

//============================================================================

class V3_گراف_راس VL_NOT_FINAL {
    VL_RTTI_IMPL_ادھار(V3_گراف_راس)
    یار class V3_گراف;
    یار class V3_گراف_کنارا;
    یار class گراف_Acyc;
    یار class گراف_Alg_Rank;
    یار class گراف_Alg_Rank_ڈونگھائی;
    V3ListLinks<V3_گراف_راس> m_links;  // List links to store instances of this class
    V3_گراف_کنارا::OList m_outs;  // List of outbound edges
    V3_گراف_کنارا::IList m_ins;  // List of inbound edges
    مزدوج m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    اتحاد {
        فراغ* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    فراغ fanout(مزدوج fanout) { m_fanout = fanout; }

محمي:
    // CONSTRUCTORS
    V3_گراف_راس(V3_گراف* graphp, ثابت V3_گراف_راس& old) VL_MT_DISABLED;

خاص:
    V3ListLinks<V3_گراف_راس>& links() { أرجع m_links; }

عام:
    // List types to store instances of this class
    using List = V3List<V3_گراف_راس, &V3_گراف_راس::links>;

    explicit V3_گراف_راس(V3_گراف* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    مجازی V3_گراف_راس* clone(V3_گراف* graphp) ثابت VL_MT_DISABLED {
        أرجع new V3_گراف_راس{graphp, *this};
    }
    مجازی ~V3_گراف_راس() = افتراضي;
    فراغ unlink_کنارے(V3_گراف* graphp) VL_MT_DISABLED;
    فراغ unlinkDelete(V3_گراف* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    سانچا <typename T>
    bool is() ثابت {
        static_assert(std::is_ادھار_دا<V3_گراف_راس, T>::قدر,
                      "'T' must be a subtype of V3GraphVertex");
        أرجع V3Rtti::is_Instance_دا<T>(this);
    }

    // Return cast to subtype T and assert of that type
    سانچا <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        أرجع static_cast<T*>(this);
    }
    سانچا <typename T>
    ثابت T* as() ثابت {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        أرجع static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    سانچا <typename T>
    T* cast() {
        أرجع is<T>() ? static_cast<T*>(this) : nullptr;
    }
    سانچا <typename T>
    ثابت T* cast() ثابت {
        أرجع is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    // ACCESSORS
    مجازی string name() ثابت { أرجع ""; }
    مجازی string dotColor() ثابت { أرجع "black"; }
    مجازی string dotShape() ثابت { أرجع ""; }
    مجازی string dotStyle() ثابت { أرجع ""; }
    مجازی string dotName() ثابت { أرجع ""; }
    مجازی string dotRank() ثابت { أرجع ""; }
    مجازی uint32_t rankAdder() ثابت { أرجع 1; }
    مجازی File_سطر* fileline() ثابت { أرجع nullptr; }  // nullptr for unknown
    مجازی صحيح ترتیب_دیو_Cmp(ثابت V3_گراف_راس* rhsp) ثابت {
        // LHS goes first if of lower rank, or lower fanout
        إذا (m_rank < rhsp->m_rank) أرجع -1;
        إذا (m_rank > rhsp->m_rank) أرجع 1;
        إذا (m_fanout < rhsp->m_fanout) أرجع -1;
        إذا (m_fanout > rhsp->m_fanout) أرجع 1;
        أرجع 0;
    }
    uint32_t color() ثابت { أرجع m_color; }
    فراغ color(uint32_t color) { m_color = color; }
    uint32_t rank() ثابت { أرجع m_rank; }
    فراغ rank(uint32_t rank) { m_rank = rank; }
    مزدوج fanout() ثابت { أرجع m_fanout; }
    فراغ user(uint32_t user) { m_user = user; }
    uint32_t user() ثابت VL_MT_STABLE { أرجع m_user; }
    فراغ userp(فراغ* userp) { m_userp = userp; }
    فراغ* userp() ثابت VL_MT_STABLE { أرجع m_userp; }
    V3_گراف_کنارا::IList& وچ_کنارے() { أرجع m_ins; }
    ثابت V3_گراف_کنارا::IList& وچ_کنارے() ثابت { أرجع m_ins; }
    سانچا <گراف_Way::en N_Way>
    مضمن خودکار& کنارے();
    سانچا <گراف_Way::en N_Way>
    مضمن ثابت خودکار& کنارے() ثابت;
    bool وچ_خالی_تھاواں() ثابت { أرجع m_ins.خالی_تھاواں(); }
    bool وچ_Size1() ثابت { أرجع m_ins.hasSingleElement(); }
    V3_گراف_کنارا::OList& باہرلا_کنارے() { أرجع m_outs; }
    ثابت V3_گراف_کنارا::OList& باہرلا_کنارے() ثابت { أرجع m_outs; }
    bool باہرلا_خالی_تھاواں() ثابت { أرجع m_outs.خالی_تھاواں(); }
    bool باہرلا_Size1() ثابت { أرجع m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    فراغ v3error_سرا(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    فراغ v3error_سرا_Fatal(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    فراغ reroute_کنارے(V3_گراف* graphp) VL_MT_DISABLED;
};

std::ostream& عامل<<(std::ostream& os, V3_گراف_راس* vertexp) VL_MT_DISABLED;

سانچا <>
مضمن خودکار& V3_گراف_راس::کنارے<گراف_Way::FORWARD>() {
    أرجع m_outs;
}

سانچا <>
مضمن خودکار& V3_گراف_راس::کنارے<گراف_Way::الٹ_2>() {
    أرجع m_ins;
}

سانچا <>
مضمن ثابت خودکار& V3_گراف_راس::کنارے<گراف_Way::FORWARD>() ثابت {
    أرجع m_outs;
}

سانچا <>
مضمن ثابت خودکار& V3_گراف_راس::کنارے<گراف_Way::الٹ_2>() ثابت {
    أرجع m_ins;
}

//============================================================================

class V3_گراف VL_NOT_FINAL {
    // MEMBERS
    V3_گراف_راس::List m_vertices;  // All vertices

محمي:
    یار class V3_گراف_راس;
    یار class V3_گراف_کنارا;
    یار class گراف_Acyc;
    // METHODS
    مزدوج ترتیب_DFSIterate(V3_گراف_راس* vertexp) VL_MT_DISABLED;
    فراغ dump_کنارا(std::ostream& os, ثابت V3_گراف_راس& راس,
                  ثابت V3_گراف_کنارا& کنارا) ثابت VL_MT_DISABLED;
    // ACCESSORS

عام:
    V3_گراف() VL_MT_DISABLED;
    مجازی ~V3_گراف() VL_MT_DISABLED;
    مجازی string dotRankDir() ثابت { أرجع "TB"; }  // rankdir for dot plotting

    // METHODS
    فراغ clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool خالی_تھاواں() ثابت { أرجع m_vertices.خالی_تھاواں(); }
    V3_گراف_راس::List& vertices() { أرجع m_vertices; }
    ثابت V3_گراف_راس::List& vertices() ثابت { أرجع m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3_کنارا_Func_P کنارا_Funcp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    فراغ stronglyConnected(V3_کنارا_Func_P کنارا_Funcp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    فراغ rank(V3_کنارا_Func_P کنارا_Funcp) VL_MT_DISABLED;
    فراغ rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    فراغ rankMin(V3_کنارا_Func_P کنارا_Funcp) VL_MT_DISABLED;
    فراغ rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    فراغ ترتیب_دیو_Vertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    فراغ ترتیب_دیو_کنارے() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    فراغ ترتیب() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    فراغ ترتیب_Pre_Ranked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    فراغ acyclic(V3_کنارا_Func_P کنارا_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    فراغ remove_Redundant_کنارے_Max(V3_کنارا_Func_P کنارا_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    فراغ remove_Redundant_کنارے_جوڑ(V3_کنارا_Func_P کنارا_Funcp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    فراغ remove_متعدی_کنارے() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3_کنارا_Func_P کنارا_Funcp, V3_گراف_راس* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    فراغ subtreeLoops(V3_کنارا_Func_P کنارا_Funcp, V3_گراف_راس* vertexp,
                      V3_گراف* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    فراغ userClearVertices() VL_MT_DISABLED;
    فراغ user_Clear_کنارے() VL_MT_DISABLED;

    /// Debugging
    فراغ dump(std::ostream& os = std::cout) ثابت VL_MT_DISABLED;
    فراغ dumpDotFile(ثابت string& filename, bool colorAsSubgraph) ثابت VL_MT_DISABLED;
    فراغ dumpDotFilePrefixed(ثابت string& nameComment,
                             bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    فراغ dumpDotFilePrefixedAlways(ثابت string& nameComment,
                                   bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    فراغ dump_کنارے(std::ostream& os, ثابت V3_گراف_راس& راس) ثابت VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    فراغ hash_گراف_Debug(ثابت حرف* debugName) ثابت VL_MT_DISABLED;

    ساكن فراغ selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        یار class گراف_Alg_Parallelism_Report;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_کل_گراف_لاگت = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_critical_رستہ_لاگت = 0;

        uint64_t m_راس_گنتی = 0;  // Number of vertexes in the graph
        uint64_t m_کنارا_گنتی = 0;  // Number of edges in the grap

        ParallelismReport() = افتراضي;

    عام:
        ~ParallelismReport() = افتراضي;
        ParallelismReport(ثابت ParallelismReport&) = افتراضي;
        ParallelismReport& عامل=(ثابت ParallelismReport&) = افتراضي;

        uint64_t کل_گراف_لاگت() ثابت { أرجع m_کل_گراف_لاگت; }
        uint64_t critical_رستہ_لاگت() ثابت { أرجع m_critical_رستہ_لاگت; }
        uint64_t راس_گنتی() ثابت { أرجع m_راس_گنتی; }
        uint64_t کنارا_گنتی() ثابت { أرجع m_کنارا_گنتی; }
        مزدوج parallelism_ضارب() ثابت {
            أرجع (static_cast<مزدوج>(m_کل_گراف_لاگت) / m_critical_رستہ_لاگت);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ثابت V3_گراف_راس*)> راس_لاگت) VL_MT_DISABLED;

    // CALLBACKS
    مجازی فراغ loops_سنیہا_Cb(V3_گراف_راس* vertexp, V3_کنارا_Func_P کنارا_Funcp) VL_MT_DISABLED;
    مجازی string loops_راس_Cb(V3_گراف_راس* vertexp) VL_MT_DISABLED;
};

//============================================================================

#جے_انت  // Guard
