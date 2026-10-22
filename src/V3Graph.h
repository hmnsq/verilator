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

#నిర్వచితం_కాకపోతే VERILATOR_V3_లేఖాచిత్రం_H
#నిర్వచించు VERILATOR_V3_లేఖాచిత్రం_H

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3List.h"
#అన్తర్భావ "V3Rtti.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <functional>

class File_పంక్తి;
class V3_లేఖాచిత్రం;
class V3_లేఖాచిత్రం_శీర్షం;
class V3_లేఖాచిత్రం_అంచు;
class లేఖాచిత్రం_Acyc_అంచు;
class క్రమం_Either_శీర్షం;
class క్రమం_Logic_శీర్షం;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3_అంచు_Func_P = bool (*)(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class లేఖాచిత్రం_Way final {
బహిరంగ:
    గణన en : uint8_t {
        FORWARD = 0,
        తిరగేయి_2 = 1,
        అంకె_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    గణన en m_e;
    లేఖాచిత్రం_Way()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr లేఖాచిత్రం_Way(en _e)
        : m_e{_e} {}
    explicit constexpr లేఖాచిత్రం_Way(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"FORWARD", "REVERSE"};
        తిరిగి పేర్లు[m_e];
    }
    // METHODS unique to this class
    constexpr లేఖాచిత్రం_Way invert() స్థిరరాశి { తిరిగి లేఖాచిత్రం_Way{m_e ^ 1}; }
    constexpr bool forward() స్థిరరాశి { తిరిగి m_e == FORWARD; }
    constexpr bool తిరగేయి() స్థిరరాశి { తిరిగి m_e != FORWARD; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి లేఖాచిత్రం_Way& lhs, స్థిరరాశి లేఖాచిత్రం_Way& rhs) { తిరిగి lhs.m_e == rhs.m_e; }
constexpr bool క్రియాకారకం==(స్థిరరాశి లేఖాచిత్రం_Way& lhs, లేఖాచిత్రం_Way::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(లేఖాచిత్రం_Way::en lhs, స్థిరరాశి లేఖాచిత్రం_Way& rhs) { తిరిగి lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3_లేఖాచిత్రం_అంచు VL_NOT_FINAL {
    VL_RTTI_IMPL_పునాది(V3_లేఖాచిత్రం_అంచు)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
బహిరంగ:
    // ENUMS
    గణన Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
రక్షిత:
    మిత్ర class V3_లేఖాచిత్రం;
    మిత్ర class V3_లేఖాచిత్రం_శీర్షం;
    మిత్ర class లేఖాచిత్రం_Acyc;
    మిత్ర class లేఖాచిత్రం_Acyc_అంచు;

    V3ListLinks<V3_లేఖాచిత్రం_అంచు> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3_లేఖాచిత్రం_అంచు> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3_లేఖాచిత్రం_శీర్షం* m_fromp;  // Vertices pointing to this edge
    V3_లేఖాచిత్రం_శీర్షం* m_శిఖరం;  // Vertices this edge points to
    పూర్ణాంకం m_బరువు;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    సంఘం {
        శూన్య* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    శూన్య init(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2,
              bool cutable = false) VL_MT_DISABLED;
    శూన్య cut() { m_బరువు = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3_లేఖాచిత్రం_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం,
                స్థిరరాశి V3_లేఖాచిత్రం_అంచు& old) VL_MT_DISABLED {
        init(graphp, fromp, శిఖరం, old.m_బరువు, old.m_cutable);
    }

అంతరంగ:
    V3ListLinks<V3_లేఖాచిత్రం_అంచు>& oLinks() { తిరిగి m_oLinks; }
    V3ListLinks<V3_లేఖాచిత్రం_అంచు>& iLinks() { తిరిగి m_iLinks; }

బహిరంగ:
    // List types to store instances of this class
    using OList = V3List<V3_లేఖాచిత్రం_అంచు, &V3_లేఖాచిత్రం_అంచు::oLinks>;
    using IList = V3List<V3_లేఖాచిత్రం_అంచు, &V3_లేఖాచిత్రం_అంచు::iLinks>;

    //! Add DAG from one node to the specified node
    V3_లేఖాచిత్రం_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, శిఖరం, బరువు_2, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    మాయా V3_లేఖాచిత్రం_అంచు* clone(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp,
                               V3_లేఖాచిత్రం_శీర్షం* శిఖరం) స్థిరరాశి VL_MT_DISABLED {
        తిరిగి new V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, *this};
    }
    మాయా ~V3_లేఖాచిత్రం_అంచు() = అప్రమేయ;
    // METHODS
    // Return true iff of type T
    మూస <typename T>
    bool is() స్థిరరాశి {
        static_assert(std::is_పునాది_యొక్క<V3_లేఖాచిత్రం_అంచు, T>::విలువ,
                      "'T' must be a subtype of V3GraphEdge");
        తిరిగి V3Rtti::is_Instance_యొక్క<T>(this);
    }

    // Return cast to subtype T and assert of that type
    మూస <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        తిరిగి static_cast<T*>(this);
    }
    మూస <typename T>
    స్థిరరాశి T* as() స్థిరరాశి {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        తిరిగి static_cast<స్థిరరాశి T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    మూస <typename T>
    T* cast() {
        తిరిగి is<T>() ? static_cast<T*>(this) : nullptr;
    }
    మూస <typename T>
    స్థిరరాశి T* cast() స్థిరరాశి {
        తిరిగి is<T>() ? static_cast<స్థిరరాశి T*>(this) : nullptr;
    }

    మాయా string name() స్థిరరాశి VL_MT_DISABLED;
    మాయా string dotLabel() స్థిరరాశి { తిరిగి ""; }
    మాయా string dotColor() స్థిరరాశి { తిరిగి cutable() ? "yellowGreen" : "red"; }
    మాయా string dotStyle() స్థిరరాశి { తిరిగి cutable() ? "dashed" : ""; }
    మాయా పూర్ణాంకం క్రమీకరించు_Cmp(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* rhsp) స్థిరరాశి VL_MT_DISABLED;
    శూన్య unlinkDelete() VL_MT_DISABLED;
    శూన్య relinkFromp(V3_లేఖాచిత్రం_శీర్షం* newFromp) VL_MT_DISABLED;
    శూన్య relink_శిఖరం(V3_లేఖాచిత్రం_శీర్షం* new_శిఖరం) VL_MT_DISABLED;
    // ACCESSORS
    పూర్ణాంకం బరువు_2() స్థిరరాశి { తిరిగి m_బరువు; }
    శూన్య బరువు_2(పూర్ణాంకం బరువు_2) { m_బరువు = బరువు_2; }
    bool cutable() స్థిరరాశి { తిరిగి m_cutable; }
    శూన్య cutable(bool cutable) { m_cutable = cutable; }
    శూన్య userp(శూన్య* user) { m_userp = user; }
    శూన్య* userp() స్థిరరాశి { తిరిగి m_userp; }
    శూన్య user(uint64_t user) { m_user = user; }
    uint64_t user() స్థిరరాశి { తిరిగి m_user; }
    V3_లేఖాచిత్రం_శీర్షం* fromp() స్థిరరాశి { తిరిగి m_fromp; }
    V3_లేఖాచిత్రం_శీర్షం* శిఖరం() స్థిరరాశి { తిరిగి m_శిఖరం; }
    మూస <లేఖాచిత్రం_Way::en N_Way>
    V3_లేఖాచిత్రం_శీర్షం* furtherp() స్థిరరాశి {
        తిరిగి N_Way == లేఖాచిత్రం_Way::FORWARD ? శిఖరం() : fromp();
    }
    // STATIC ACCESSORS
    స్థిర bool followNotCutable(స్థిరరాశి V3_లేఖాచిత్రం_అంచు* edgep) { తిరిగి !edgep->m_cutable; }
    స్థిర bool followAlwaysTrue(స్థిరరాశి V3_లేఖాచిత్రం_అంచు*) { తిరిగి true; }
};

//============================================================================

class V3_లేఖాచిత్రం_శీర్షం VL_NOT_FINAL {
    VL_RTTI_IMPL_పునాది(V3_లేఖాచిత్రం_శీర్షం)
    మిత్ర class V3_లేఖాచిత్రం;
    మిత్ర class V3_లేఖాచిత్రం_అంచు;
    మిత్ర class లేఖాచిత్రం_Acyc;
    మిత్ర class లేఖాచిత్రం_Alg_Rank;
    మిత్ర class లేఖాచిత్రం_Alg_Rank_లోతు;
    V3ListLinks<V3_లేఖాచిత్రం_శీర్షం> m_links;  // List links to store instances of this class
    V3_లేఖాచిత్రం_అంచు::OList m_outs;  // List of outbound edges
    V3_లేఖాచిత్రం_అంచు::IList m_ins;  // List of inbound edges
    ద్విగుణ m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    సంఘం {
        శూన్య* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    శూన్య fanout(ద్విగుణ fanout) { m_fanout = fanout; }

రక్షిత:
    // CONSTRUCTORS
    V3_లేఖాచిత్రం_శీర్షం(V3_లేఖాచిత్రం* graphp, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& old) VL_MT_DISABLED;

అంతరంగ:
    V3ListLinks<V3_లేఖాచిత్రం_శీర్షం>& links() { తిరిగి m_links; }

బహిరంగ:
    // List types to store instances of this class
    using List = V3List<V3_లేఖాచిత్రం_శీర్షం, &V3_లేఖాచిత్రం_శీర్షం::links>;

    explicit V3_లేఖాచిత్రం_శీర్షం(V3_లేఖాచిత్రం* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    మాయా V3_లేఖాచిత్రం_శీర్షం* clone(V3_లేఖాచిత్రం* graphp) స్థిరరాశి VL_MT_DISABLED {
        తిరిగి new V3_లేఖాచిత్రం_శీర్షం{graphp, *this};
    }
    మాయా ~V3_లేఖాచిత్రం_శీర్షం() = అప్రమేయ;
    శూన్య unlink_అంచులు(V3_లేఖాచిత్రం* graphp) VL_MT_DISABLED;
    శూన్య unlinkDelete(V3_లేఖాచిత్రం* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    మూస <typename T>
    bool is() స్థిరరాశి {
        static_assert(std::is_పునాది_యొక్క<V3_లేఖాచిత్రం_శీర్షం, T>::విలువ,
                      "'T' must be a subtype of V3GraphVertex");
        తిరిగి V3Rtti::is_Instance_యొక్క<T>(this);
    }

    // Return cast to subtype T and assert of that type
    మూస <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        తిరిగి static_cast<T*>(this);
    }
    మూస <typename T>
    స్థిరరాశి T* as() స్థిరరాశి {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        తిరిగి static_cast<స్థిరరాశి T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    మూస <typename T>
    T* cast() {
        తిరిగి is<T>() ? static_cast<T*>(this) : nullptr;
    }
    మూస <typename T>
    స్థిరరాశి T* cast() స్థిరరాశి {
        తిరిగి is<T>() ? static_cast<స్థిరరాశి T*>(this) : nullptr;
    }

    // ACCESSORS
    మాయా string name() స్థిరరాశి { తిరిగి ""; }
    మాయా string dotColor() స్థిరరాశి { తిరిగి "black"; }
    మాయా string dotShape() స్థిరరాశి { తిరిగి ""; }
    మాయా string dotStyle() స్థిరరాశి { తిరిగి ""; }
    మాయా string dotName() స్థిరరాశి { తిరిగి ""; }
    మాయా string dotRank() స్థిరరాశి { తిరిగి ""; }
    మాయా uint32_t rankAdder() స్థిరరాశి { తిరిగి 1; }
    మాయా File_పంక్తి* fileline() స్థిరరాశి { తిరిగి nullptr; }  // nullptr for unknown
    మాయా పూర్ణాంకం క్రమీకరించు_Cmp(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* rhsp) స్థిరరాశి {
        // LHS goes first if of lower rank, or lower fanout
        ఒకవేళ (m_rank < rhsp->m_rank) తిరిగి -1;
        ఒకవేళ (m_rank > rhsp->m_rank) తిరిగి 1;
        ఒకవేళ (m_fanout < rhsp->m_fanout) తిరిగి -1;
        ఒకవేళ (m_fanout > rhsp->m_fanout) తిరిగి 1;
        తిరిగి 0;
    }
    uint32_t color() స్థిరరాశి { తిరిగి m_color; }
    శూన్య color(uint32_t color) { m_color = color; }
    uint32_t rank() స్థిరరాశి { తిరిగి m_rank; }
    శూన్య rank(uint32_t rank) { m_rank = rank; }
    ద్విగుణ fanout() స్థిరరాశి { తిరిగి m_fanout; }
    శూన్య user(uint32_t user) { m_user = user; }
    uint32_t user() స్థిరరాశి VL_MT_STABLE { తిరిగి m_user; }
    శూన్య userp(శూన్య* userp) { m_userp = userp; }
    శూన్య* userp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_userp; }
    V3_లేఖాచిత్రం_అంచు::IList& లోపల_అంచులు() { తిరిగి m_ins; }
    స్థిరరాశి V3_లేఖాచిత్రం_అంచు::IList& లోపల_అంచులు() స్థిరరాశి { తిరిగి m_ins; }
    మూస <లేఖాచిత్రం_Way::en N_Way>
    అంతఃవరుస స్వయంచాలక& అంచులు();
    మూస <లేఖాచిత్రం_Way::en N_Way>
    అంతఃవరుస స్థిరరాశి స్వయంచాలక& అంచులు() స్థిరరాశి;
    bool లోపల_ఖాళీలు() స్థిరరాశి { తిరిగి m_ins.ఖాళీలు(); }
    bool లోపల_Size1() స్థిరరాశి { తిరిగి m_ins.hasSingleElement(); }
    V3_లేఖాచిత్రం_అంచు::OList& బహిర్గతం_అంచులు() { తిరిగి m_outs; }
    స్థిరరాశి V3_లేఖాచిత్రం_అంచు::OList& బహిర్గతం_అంచులు() స్థిరరాశి { తిరిగి m_outs; }
    bool బహిర్గతం_ఖాళీలు() స్థిరరాశి { తిరిగి m_outs.ఖాళీలు(); }
    bool బహిర్గతం_Size1() స్థిరరాశి { తిరిగి m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    శూన్య v3error_చివర(స్థిరరాశి std::ostringstream& str) స్థిరరాశి
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    శూన్య v3error_చివర_Fatal(స్థిరరాశి std::ostringstream& str) స్థిరరాశి
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    శూన్య reroute_అంచులు(V3_లేఖాచిత్రం* graphp) VL_MT_DISABLED;
};

std::ostream& క్రియాకారకం<<(std::ostream& os, V3_లేఖాచిత్రం_శీర్షం* vertexp) VL_MT_DISABLED;

మూస <>
అంతఃవరుస స్వయంచాలక& V3_లేఖాచిత్రం_శీర్షం::అంచులు<లేఖాచిత్రం_Way::FORWARD>() {
    తిరిగి m_outs;
}

మూస <>
అంతఃవరుస స్వయంచాలక& V3_లేఖాచిత్రం_శీర్షం::అంచులు<లేఖాచిత్రం_Way::తిరగేయి_2>() {
    తిరిగి m_ins;
}

మూస <>
అంతఃవరుస స్థిరరాశి స్వయంచాలక& V3_లేఖాచిత్రం_శీర్షం::అంచులు<లేఖాచిత్రం_Way::FORWARD>() స్థిరరాశి {
    తిరిగి m_outs;
}

మూస <>
అంతఃవరుస స్థిరరాశి స్వయంచాలక& V3_లేఖాచిత్రం_శీర్షం::అంచులు<లేఖాచిత్రం_Way::తిరగేయి_2>() స్థిరరాశి {
    తిరిగి m_ins;
}

//============================================================================

class V3_లేఖాచిత్రం VL_NOT_FINAL {
    // MEMBERS
    V3_లేఖాచిత్రం_శీర్షం::List m_vertices;  // All vertices

రక్షిత:
    మిత్ర class V3_లేఖాచిత్రం_శీర్షం;
    మిత్ర class V3_లేఖాచిత్రం_అంచు;
    మిత్ర class లేఖాచిత్రం_Acyc;
    // METHODS
    ద్విగుణ క్రమం_DFSIterate(V3_లేఖాచిత్రం_శీర్షం* vertexp) VL_MT_DISABLED;
    శూన్య dump_అంచు(std::ostream& os, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& శీర్షం,
                  స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు) స్థిరరాశి VL_MT_DISABLED;
    // ACCESSORS

బహిరంగ:
    V3_లేఖాచిత్రం() VL_MT_DISABLED;
    మాయా ~V3_లేఖాచిత్రం() VL_MT_DISABLED;
    మాయా string dotRankDir() స్థిరరాశి { తిరిగి "TB"; }  // rankdir for dot plotting

    // METHODS
    శూన్య clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool ఖాళీలు() స్థిరరాశి { తిరిగి m_vertices.ఖాళీలు(); }
    V3_లేఖాచిత్రం_శీర్షం::List& vertices() { తిరిగి m_vertices; }
    స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం::List& vertices() స్థిరరాశి { తిరిగి m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3_అంచు_Func_P అంచు_Funcp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    శూన్య stronglyConnected(V3_అంచు_Func_P అంచు_Funcp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    శూన్య rank(V3_అంచు_Func_P అంచు_Funcp) VL_MT_DISABLED;
    శూన్య rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    శూన్య rankMin(V3_అంచు_Func_P అంచు_Funcp) VL_MT_DISABLED;
    శూన్య rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    శూన్య క్రమీకరించు_Vertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    శూన్య క్రమీకరించు_అంచులు() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    శూన్య క్రమం() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    శూన్య క్రమం_Pre_Ranked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    శూన్య acyclic(V3_అంచు_Func_P అంచు_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    శూన్య remove_Redundant_అంచులు_Max(V3_అంచు_Func_P అంచు_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    శూన్య remove_Redundant_అంచులు_కూడిక(V3_అంచు_Func_P అంచు_Funcp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    శూన్య remove_సంక్రమణ_అంచులు() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3_అంచు_Func_P అంచు_Funcp, V3_లేఖాచిత్రం_శీర్షం* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    శూన్య subtreeLoops(V3_అంచు_Func_P అంచు_Funcp, V3_లేఖాచిత్రం_శీర్షం* vertexp,
                      V3_లేఖాచిత్రం* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    శూన్య userClearVertices() VL_MT_DISABLED;
    శూన్య user_Clear_అంచులు() VL_MT_DISABLED;

    /// Debugging
    శూన్య dump(std::ostream& os = std::cout) స్థిరరాశి VL_MT_DISABLED;
    శూన్య dumpDotFile(స్థిరరాశి string& filename, bool colorAsSubgraph) స్థిరరాశి VL_MT_DISABLED;
    శూన్య dumpDotFilePrefixed(స్థిరరాశి string& nameComment,
                             bool colorAsSubgraph = false) స్థిరరాశి VL_MT_DISABLED;
    శూన్య dumpDotFilePrefixedAlways(స్థిరరాశి string& nameComment,
                                   bool colorAsSubgraph = false) స్థిరరాశి VL_MT_DISABLED;
    శూన్య dump_అంచులు(std::ostream& os, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& శీర్షం) స్థిరరాశి VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    శూన్య hash_లేఖాచిత్రం_Debug(స్థిరరాశి అక్షరం* debugName) స్థిరరాశి VL_MT_DISABLED;

    స్థిర శూన్య selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        మిత్ర class లేఖాచిత్రం_Alg_Parallelism_Report;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_మొత్తం_లేఖాచిత్రం_ఖర్చు = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_critical_మార్గం_ఖర్చు = 0;

        uint64_t m_శీర్షం_లెక్క = 0;  // Number of vertexes in the graph
        uint64_t m_అంచు_లెక్క = 0;  // Number of edges in the grap

        ParallelismReport() = అప్రమేయ;

    బహిరంగ:
        ~ParallelismReport() = అప్రమేయ;
        ParallelismReport(స్థిరరాశి ParallelismReport&) = అప్రమేయ;
        ParallelismReport& క్రియాకారకం=(స్థిరరాశి ParallelismReport&) = అప్రమేయ;

        uint64_t మొత్తం_లేఖాచిత్రం_ఖర్చు() స్థిరరాశి { తిరిగి m_మొత్తం_లేఖాచిత్రం_ఖర్చు; }
        uint64_t critical_మార్గం_ఖర్చు() స్థిరరాశి { తిరిగి m_critical_మార్గం_ఖర్చు; }
        uint64_t శీర్షం_లెక్క() స్థిరరాశి { తిరిగి m_శీర్షం_లెక్క; }
        uint64_t అంచు_లెక్క() స్థిరరాశి { తిరిగి m_అంచు_లెక్క; }
        ద్విగుణ parallelism_గుణకం() స్థిరరాశి {
            తిరిగి (static_cast<ద్విగుణ>(m_మొత్తం_లేఖాచిత్రం_ఖర్చు) / m_critical_మార్గం_ఖర్చు);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*)> శీర్షం_ఖర్చు) VL_MT_DISABLED;

    // CALLBACKS
    మాయా శూన్య loops_సందేశం_Cb(V3_లేఖాచిత్రం_శీర్షం* vertexp, V3_అంచు_Func_P అంచు_Funcp) VL_MT_DISABLED;
    మాయా string loops_శీర్షం_Cb(V3_లేఖాచిత్రం_శీర్షం* vertexp) VL_MT_DISABLED;
};

//============================================================================

#అయితే_ముగింపు  // Guard
