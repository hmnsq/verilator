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

#که_نه_تعریف_شوی VERILATOR_V3_ګراف_H
#تعریف_کړه VERILATOR_V3_ګراف_H

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3List.h"
#ضمن "V3Rtti.h"

#ضمن <algorithm>
#ضمن <functional>

class File_کرښه;
class V3_ګراف;
class V3_ګراف_راس;
class V3_ګراف_څنډه;
class ګراف_Acyc_څنډه;
class ترتیب_Either_راس;
class ترتیب_Logic_راس;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3_څنډه_Func_P = bool (*)(ثابت V3_ګراف_څنډه* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class ګراف_Way final {
عام:
    تعداد en : uint8_t {
        FORWARD = 0,
        سرچپه_2 = 1,
        عدد_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    تعداد en m_e;
    ګراف_Way()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr ګراف_Way(en _e)
        : m_e{_e} {}
    explicit constexpr ګراف_Way(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"FORWARD", "REVERSE"};
        أرجع نومونه[m_e];
    }
    // METHODS unique to this class
    constexpr ګراف_Way invert() ثابت { أرجع ګراف_Way{m_e ^ 1}; }
    constexpr bool forward() ثابت { أرجع m_e == FORWARD; }
    constexpr bool سرچپه() ثابت { أرجع m_e != FORWARD; }
};
constexpr bool چلوونکی==(ثابت ګراف_Way& lhs, ثابت ګراف_Way& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool چلوونکی==(ثابت ګراف_Way& lhs, ګراف_Way::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(ګراف_Way::en lhs, ثابت ګراف_Way& rhs) { أرجع lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3_ګراف_څنډه VL_NOT_FINAL {
    VL_RTTI_IMPL_بنسټ(V3_ګراف_څنډه)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
عام:
    // ENUMS
    تعداد Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
محمي:
    ملګری class V3_ګراف;
    ملګری class V3_ګراف_راس;
    ملګری class ګراف_Acyc;
    ملګری class ګراف_Acyc_څنډه;

    V3ListLinks<V3_ګراف_څنډه> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3_ګراف_څنډه> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3_ګراف_راس* m_fromp;  // Vertices pointing to this edge
    V3_ګراف_راس* m_سر;  // Vertices this edge points to
    صحيح m_وزن;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    اتحاد {
        فراغ* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    فراغ init(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر, صحيح وزن_2,
              bool cutable = false) VL_MT_DISABLED;
    فراغ cut() { m_وزن = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3_ګراف_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر,
                ثابت V3_ګراف_څنډه& old) VL_MT_DISABLED {
        init(graphp, fromp, سر, old.m_وزن, old.m_cutable);
    }

خاص:
    V3ListLinks<V3_ګراف_څنډه>& oLinks() { أرجع m_oLinks; }
    V3ListLinks<V3_ګراف_څنډه>& iLinks() { أرجع m_iLinks; }

عام:
    // List types to store instances of this class
    using OList = V3List<V3_ګراف_څنډه, &V3_ګراف_څنډه::oLinks>;
    using IList = V3List<V3_ګراف_څنډه, &V3_ګراف_څنډه::iLinks>;

    //! Add DAG from one node to the specified node
    V3_ګراف_څنډه(V3_ګراف* graphp, V3_ګراف_راس* fromp, V3_ګراف_راس* سر, صحيح وزن_2,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, سر, وزن_2, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    مجازي V3_ګراف_څنډه* clone(V3_ګراف* graphp, V3_ګراف_راس* fromp,
                               V3_ګراف_راس* سر) ثابت VL_MT_DISABLED {
        أرجع new V3_ګراف_څنډه{graphp, fromp, سر, *this};
    }
    مجازي ~V3_ګراف_څنډه() = افتراضي;
    // METHODS
    // Return true iff of type T
    کينډۍ <typename T>
    bool is() ثابت {
        static_assert(std::is_بنسټ_د<V3_ګراف_څنډه, T>::ارزښت,
                      "'T' must be a subtype of V3GraphEdge");
        أرجع V3Rtti::is_Instance_د<T>(this);
    }

    // Return cast to subtype T and assert of that type
    کينډۍ <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        أرجع static_cast<T*>(this);
    }
    کينډۍ <typename T>
    ثابت T* as() ثابت {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        أرجع static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    کينډۍ <typename T>
    T* cast() {
        أرجع is<T>() ? static_cast<T*>(this) : nullptr;
    }
    کينډۍ <typename T>
    ثابت T* cast() ثابت {
        أرجع is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    مجازي string name() ثابت VL_MT_DISABLED;
    مجازي string dotLabel() ثابت { أرجع ""; }
    مجازي string dotColor() ثابت { أرجع cutable() ? "yellowGreen" : "red"; }
    مجازي string dotStyle() ثابت { أرجع cutable() ? "dashed" : ""; }
    مجازي صحيح ترتیبول_Cmp(ثابت V3_ګراف_څنډه* rhsp) ثابت VL_MT_DISABLED;
    فراغ unlinkDelete() VL_MT_DISABLED;
    فراغ relinkFromp(V3_ګراف_راس* newFromp) VL_MT_DISABLED;
    فراغ relink_سر(V3_ګراف_راس* new_سر) VL_MT_DISABLED;
    // ACCESSORS
    صحيح وزن_2() ثابت { أرجع m_وزن; }
    فراغ وزن_2(صحيح وزن_2) { m_وزن = وزن_2; }
    bool cutable() ثابت { أرجع m_cutable; }
    فراغ cutable(bool cutable) { m_cutable = cutable; }
    فراغ userp(فراغ* user) { m_userp = user; }
    فراغ* userp() ثابت { أرجع m_userp; }
    فراغ user(uint64_t user) { m_user = user; }
    uint64_t user() ثابت { أرجع m_user; }
    V3_ګراف_راس* fromp() ثابت { أرجع m_fromp; }
    V3_ګراف_راس* سر() ثابت { أرجع m_سر; }
    کينډۍ <ګراف_Way::en N_Way>
    V3_ګراف_راس* furtherp() ثابت {
        أرجع N_Way == ګراف_Way::FORWARD ? سر() : fromp();
    }
    // STATIC ACCESSORS
    ساكن bool followNotCutable(ثابت V3_ګراف_څنډه* edgep) { أرجع !edgep->m_cutable; }
    ساكن bool followAlwaysTrue(ثابت V3_ګراف_څنډه*) { أرجع true; }
};

//============================================================================

class V3_ګراف_راس VL_NOT_FINAL {
    VL_RTTI_IMPL_بنسټ(V3_ګراف_راس)
    ملګری class V3_ګراف;
    ملګری class V3_ګراف_څنډه;
    ملګری class ګراف_Acyc;
    ملګری class ګراف_Alg_Rank;
    ملګری class ګراف_Alg_Rank_ژوروالی;
    V3ListLinks<V3_ګراف_راس> m_links;  // List links to store instances of this class
    V3_ګراف_څنډه::OList m_outs;  // List of outbound edges
    V3_ګراف_څنډه::IList m_ins;  // List of inbound edges
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
    V3_ګراف_راس(V3_ګراف* graphp, ثابت V3_ګراف_راس& old) VL_MT_DISABLED;

خاص:
    V3ListLinks<V3_ګراف_راس>& links() { أرجع m_links; }

عام:
    // List types to store instances of this class
    using List = V3List<V3_ګراف_راس, &V3_ګراف_راس::links>;

    explicit V3_ګراف_راس(V3_ګراف* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    مجازي V3_ګراف_راس* clone(V3_ګراف* graphp) ثابت VL_MT_DISABLED {
        أرجع new V3_ګراف_راس{graphp, *this};
    }
    مجازي ~V3_ګراف_راس() = افتراضي;
    فراغ unlink_څنډې(V3_ګراف* graphp) VL_MT_DISABLED;
    فراغ unlinkDelete(V3_ګراف* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    کينډۍ <typename T>
    bool is() ثابت {
        static_assert(std::is_بنسټ_د<V3_ګراف_راس, T>::ارزښت,
                      "'T' must be a subtype of V3GraphVertex");
        أرجع V3Rtti::is_Instance_د<T>(this);
    }

    // Return cast to subtype T and assert of that type
    کينډۍ <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        أرجع static_cast<T*>(this);
    }
    کينډۍ <typename T>
    ثابت T* as() ثابت {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        أرجع static_cast<ثابت T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    کينډۍ <typename T>
    T* cast() {
        أرجع is<T>() ? static_cast<T*>(this) : nullptr;
    }
    کينډۍ <typename T>
    ثابت T* cast() ثابت {
        أرجع is<T>() ? static_cast<ثابت T*>(this) : nullptr;
    }

    // ACCESSORS
    مجازي string name() ثابت { أرجع ""; }
    مجازي string dotColor() ثابت { أرجع "black"; }
    مجازي string dotShape() ثابت { أرجع ""; }
    مجازي string dotStyle() ثابت { أرجع ""; }
    مجازي string dotName() ثابت { أرجع ""; }
    مجازي string dotRank() ثابت { أرجع ""; }
    مجازي uint32_t rankAdder() ثابت { أرجع 1; }
    مجازي File_کرښه* fileline() ثابت { أرجع nullptr; }  // nullptr for unknown
    مجازي صحيح ترتیبول_Cmp(ثابت V3_ګراف_راس* rhsp) ثابت {
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
    V3_ګراف_څنډه::IList& دننه_څنډې() { أرجع m_ins; }
    ثابت V3_ګراف_څنډه::IList& دننه_څنډې() ثابت { أرجع m_ins; }
    کينډۍ <ګراف_Way::en N_Way>
    مضمن خپلکاری& څنډې();
    کينډۍ <ګراف_Way::en N_Way>
    مضمن ثابت خپلکاری& څنډې() ثابت;
    bool دننه_تشه() ثابت { أرجع m_ins.تشه(); }
    bool دننه_Size1() ثابت { أرجع m_ins.hasSingleElement(); }
    V3_ګراف_څنډه::OList& بهر_څنډې() { أرجع m_outs; }
    ثابت V3_ګراف_څنډه::OList& بهر_څنډې() ثابت { أرجع m_outs; }
    bool بهر_تشه() ثابت { أرجع m_outs.تشه(); }
    bool بهر_Size1() ثابت { أرجع m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    فراغ v3error_پای_ټکی(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    فراغ v3error_پای_ټکی_Fatal(ثابت std::ostringstream& str) ثابت
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    فراغ reroute_څنډې(V3_ګراف* graphp) VL_MT_DISABLED;
};

std::ostream& چلوونکی<<(std::ostream& os, V3_ګراف_راس* vertexp) VL_MT_DISABLED;

کينډۍ <>
مضمن خپلکاری& V3_ګراف_راس::څنډې<ګراف_Way::FORWARD>() {
    أرجع m_outs;
}

کينډۍ <>
مضمن خپلکاری& V3_ګراف_راس::څنډې<ګراف_Way::سرچپه_2>() {
    أرجع m_ins;
}

کينډۍ <>
مضمن ثابت خپلکاری& V3_ګراف_راس::څنډې<ګراف_Way::FORWARD>() ثابت {
    أرجع m_outs;
}

کينډۍ <>
مضمن ثابت خپلکاری& V3_ګراف_راس::څنډې<ګراف_Way::سرچپه_2>() ثابت {
    أرجع m_ins;
}

//============================================================================

class V3_ګراف VL_NOT_FINAL {
    // MEMBERS
    V3_ګراف_راس::List m_vertices;  // All vertices

محمي:
    ملګری class V3_ګراف_راس;
    ملګری class V3_ګراف_څنډه;
    ملګری class ګراف_Acyc;
    // METHODS
    مزدوج ترتیب_DFSIterate(V3_ګراف_راس* vertexp) VL_MT_DISABLED;
    فراغ dump_څنډه(std::ostream& os, ثابت V3_ګراف_راس& راس,
                  ثابت V3_ګراف_څنډه& څنډه) ثابت VL_MT_DISABLED;
    // ACCESSORS

عام:
    V3_ګراف() VL_MT_DISABLED;
    مجازي ~V3_ګراف() VL_MT_DISABLED;
    مجازي string dotRankDir() ثابت { أرجع "TB"; }  // rankdir for dot plotting

    // METHODS
    فراغ clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool تشه() ثابت { أرجع m_vertices.تشه(); }
    V3_ګراف_راس::List& vertices() { أرجع m_vertices; }
    ثابت V3_ګراف_راس::List& vertices() ثابت { أرجع m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3_څنډه_Func_P څنډه_Funcp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    فراغ stronglyConnected(V3_څنډه_Func_P څنډه_Funcp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    فراغ rank(V3_څنډه_Func_P څنډه_Funcp) VL_MT_DISABLED;
    فراغ rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    فراغ rankMin(V3_څنډه_Func_P څنډه_Funcp) VL_MT_DISABLED;
    فراغ rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    فراغ ترتیبول_Vertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    فراغ ترتیبول_څنډې() VL_MT_DISABLED;

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
    فراغ acyclic(V3_څنډه_Func_P څنډه_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    فراغ remove_Redundant_څنډې_Max(V3_څنډه_Func_P څنډه_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    فراغ remove_Redundant_څنډې_مجموعه(V3_څنډه_Func_P څنډه_Funcp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    فراغ remove_متعدي_څنډې() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3_څنډه_Func_P څنډه_Funcp, V3_ګراف_راس* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    فراغ subtreeLoops(V3_څنډه_Func_P څنډه_Funcp, V3_ګراف_راس* vertexp,
                      V3_ګراف* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    فراغ userClearVertices() VL_MT_DISABLED;
    فراغ user_Clear_څنډې() VL_MT_DISABLED;

    /// Debugging
    فراغ dump(std::ostream& os = std::cout) ثابت VL_MT_DISABLED;
    فراغ dumpDotFile(ثابت string& filename, bool colorAsSubgraph) ثابت VL_MT_DISABLED;
    فراغ dumpDotFilePrefixed(ثابت string& nameComment,
                             bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    فراغ dumpDotFilePrefixedAlways(ثابت string& nameComment,
                                   bool colorAsSubgraph = false) ثابت VL_MT_DISABLED;
    فراغ dump_څنډې(std::ostream& os, ثابت V3_ګراف_راس& راس) ثابت VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    فراغ hash_ګراف_Debug(ثابت حرف* debugName) ثابت VL_MT_DISABLED;

    ساكن فراغ selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        ملګری class ګراف_Alg_Parallelism_Report;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_ټول_ګراف_لګښت = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_critical_لار_لګښت = 0;

        uint64_t m_راس_شمېر = 0;  // Number of vertexes in the graph
        uint64_t m_څنډه_شمېر = 0;  // Number of edges in the grap

        ParallelismReport() = افتراضي;

    عام:
        ~ParallelismReport() = افتراضي;
        ParallelismReport(ثابت ParallelismReport&) = افتراضي;
        ParallelismReport& چلوونکی=(ثابت ParallelismReport&) = افتراضي;

        uint64_t ټول_ګراف_لګښت() ثابت { أرجع m_ټول_ګراف_لګښت; }
        uint64_t critical_لار_لګښت() ثابت { أرجع m_critical_لار_لګښت; }
        uint64_t راس_شمېر() ثابت { أرجع m_راس_شمېر; }
        uint64_t څنډه_شمېر() ثابت { أرجع m_څنډه_شمېر; }
        مزدوج parallelism_ضریب() ثابت {
            أرجع (static_cast<مزدوج>(m_ټول_ګراف_لګښت) / m_critical_لار_لګښت);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ثابت V3_ګراف_راس*)> راس_لګښت) VL_MT_DISABLED;

    // CALLBACKS
    مجازي فراغ loops_پیغام_Cb(V3_ګراف_راس* vertexp, V3_څنډه_Func_P څنډه_Funcp) VL_MT_DISABLED;
    مجازي string loops_راس_Cb(V3_ګراف_راس* vertexp) VL_MT_DISABLED;
};

//============================================================================

#د_که_پای  // Guard
