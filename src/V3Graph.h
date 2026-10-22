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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3_ਗ੍ਰਾਫ_H
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3_ਗ੍ਰਾਫ_H

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3List.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Rtti.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>

class File_ਸਤਰ;
class V3_ਗ੍ਰਾਫ;
class V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼;
class V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ;
class ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ;
class ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼;
class ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3_ਕਿਨਾਰਾ_Func_P = bool (*)(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class ਗ੍ਰਾਫ_Way final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        FORWARD = 0,
        ਉਲਟ_2 = 1,
        ਅੰਕ_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    ਗਿਣਤੀ en m_e;
    ਗ੍ਰਾਫ_Way()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr ਗ੍ਰਾਫ_Way(en _e)
        : m_e{_e} {}
    explicit constexpr ਗ੍ਰਾਫ_Way(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"FORWARD", "REVERSE"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    // METHODS unique to this class
    constexpr ਗ੍ਰਾਫ_Way invert() ਅਚਲ { ਵਾਪਸ ਗ੍ਰਾਫ_Way{m_e ^ 1}; }
    constexpr bool forward() ਅਚਲ { ਵਾਪਸ m_e == FORWARD; }
    constexpr bool ਉਲਟ() ਅਚਲ { ਵਾਪਸ m_e != FORWARD; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ ਗ੍ਰਾਫ_Way& lhs, ਅਚਲ ਗ੍ਰਾਫ_Way& rhs) { ਵਾਪਸ lhs.m_e == rhs.m_e; }
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ ਗ੍ਰਾਫ_Way& lhs, ਗ੍ਰਾਫ_Way::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(ਗ੍ਰਾਫ_Way::en lhs, ਅਚਲ ਗ੍ਰਾਫ_Way& rhs) { ਵਾਪਸ lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ VL_NOT_FINAL {
    VL_RTTI_IMPL_ਅਧਾਰ(V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
ਜਨਤਕ:
    // ENUMS
    ਗਿਣਤੀ Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ class V3_ਗ੍ਰਾਫ;
    ਮਿੱਤਰ class V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼;
    ਮਿੱਤਰ class ਗ੍ਰਾਫ_Acyc;
    ਮਿੱਤਰ class ਗ੍ਰਾਫ_Acyc_ਕਿਨਾਰਾ;

    V3ListLinks<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* m_fromp;  // Vertices pointing to this edge
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* m_ਸਿਖਰ;  // Vertices this edge points to
    ਪੂਰਨ m_ਭਾਰ;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    ਸੰਘ {
        ਖਾਲੀ* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    ਖਾਲੀ init(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2,
              bool cutable = false) VL_MT_DISABLED;
    ਖਾਲੀ cut() { m_ਭਾਰ = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& old) VL_MT_DISABLED {
        init(graphp, fromp, ਸਿਖਰ, old.m_ਭਾਰ, old.m_cutable);
    }

ਨਿਜੀ:
    V3ListLinks<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ>& oLinks() { ਵਾਪਸ m_oLinks; }
    V3ListLinks<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ>& iLinks() { ਵਾਪਸ m_iLinks; }

ਜਨਤਕ:
    // List types to store instances of this class
    using OList = V3List<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ, &V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::oLinks>;
    using IList = V3List<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ, &V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::iLinks>;

    //! Add DAG from one node to the specified node
    V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, ਸਿਖਰ, ਭਾਰ_2, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    ਆਭਾਸੀ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* clone(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp,
                               V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ) ਅਚਲ VL_MT_DISABLED {
        ਵਾਪਸ new V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, *this};
    }
    ਆਭਾਸੀ ~V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ() = ਮੂਲ;
    // METHODS
    // Return true iff of type T
    ਸਾਂਚਾ <typename T>
    bool is() ਅਚਲ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ, T>::ਮੁੱਲ,
                      "'T' must be a subtype of V3GraphEdge");
        ਵਾਪਸ V3Rtti::is_Instance_ਦਾ<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ਸਾਂਚਾ <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ਵਾਪਸ static_cast<T*>(this);
    }
    ਸਾਂਚਾ <typename T>
    ਅਚਲ T* as() ਅਚਲ {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ਵਾਪਸ static_cast<ਅਚਲ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ਸਾਂਚਾ <typename T>
    T* cast() {
        ਵਾਪਸ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ਸਾਂਚਾ <typename T>
    ਅਚਲ T* cast() ਅਚਲ {
        ਵਾਪਸ is<T>() ? static_cast<ਅਚਲ T*>(this) : nullptr;
    }

    ਆਭਾਸੀ string name() ਅਚਲ VL_MT_DISABLED;
    ਆਭਾਸੀ string dotLabel() ਅਚਲ { ਵਾਪਸ ""; }
    ਆਭਾਸੀ string dotColor() ਅਚਲ { ਵਾਪਸ cutable() ? "yellowGreen" : "red"; }
    ਆਭਾਸੀ string dotStyle() ਅਚਲ { ਵਾਪਸ cutable() ? "dashed" : ""; }
    ਆਭਾਸੀ ਪੂਰਨ ਛਾਂਟੋ_Cmp(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* rhsp) ਅਚਲ VL_MT_DISABLED;
    ਖਾਲੀ unlinkDelete() VL_MT_DISABLED;
    ਖਾਲੀ relinkFromp(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* newFromp) VL_MT_DISABLED;
    ਖਾਲੀ relink_ਸਿਖਰ(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* new_ਸਿਖਰ) VL_MT_DISABLED;
    // ACCESSORS
    ਪੂਰਨ ਭਾਰ_2() ਅਚਲ { ਵਾਪਸ m_ਭਾਰ; }
    ਖਾਲੀ ਭਾਰ_2(ਪੂਰਨ ਭਾਰ_2) { m_ਭਾਰ = ਭਾਰ_2; }
    bool cutable() ਅਚਲ { ਵਾਪਸ m_cutable; }
    ਖਾਲੀ cutable(bool cutable) { m_cutable = cutable; }
    ਖਾਲੀ userp(ਖਾਲੀ* user) { m_userp = user; }
    ਖਾਲੀ* userp() ਅਚਲ { ਵਾਪਸ m_userp; }
    ਖਾਲੀ user(uint64_t user) { m_user = user; }
    uint64_t user() ਅਚਲ { ਵਾਪਸ m_user; }
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp() ਅਚਲ { ਵਾਪਸ m_fromp; }
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ() ਅਚਲ { ਵਾਪਸ m_ਸਿਖਰ; }
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* furtherp() ਅਚਲ {
        ਵਾਪਸ N_Way == ਗ੍ਰਾਫ_Way::FORWARD ? ਸਿਖਰ() : fromp();
    }
    // STATIC ACCESSORS
    ਸਥਿਰ bool followNotCutable(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* edgep) { ਵਾਪਸ !edgep->m_cutable; }
    ਸਥਿਰ bool followAlwaysTrue(ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ*) { ਵਾਪਸ true; }
};

//============================================================================

class V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ VL_NOT_FINAL {
    VL_RTTI_IMPL_ਅਧਾਰ(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    ਮਿੱਤਰ class V3_ਗ੍ਰਾਫ;
    ਮਿੱਤਰ class V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ;
    ਮਿੱਤਰ class ਗ੍ਰਾਫ_Acyc;
    ਮਿੱਤਰ class ਗ੍ਰਾਫ_Alg_Rank;
    ਮਿੱਤਰ class ਗ੍ਰਾਫ_Alg_Rank_ਡੂੰਘਾਈ;
    V3ListLinks<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼> m_links;  // List links to store instances of this class
    V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::OList m_outs;  // List of outbound edges
    V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::IList m_ins;  // List of inbound edges
    ਦੁੱਗਣਾ m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    ਸੰਘ {
        ਖਾਲੀ* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    ਖਾਲੀ fanout(ਦੁੱਗਣਾ fanout) { m_fanout = fanout; }

ਸੁਰੱਖਿਅਤ:
    // CONSTRUCTORS
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& old) VL_MT_DISABLED;

ਨਿਜੀ:
    V3ListLinks<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼>& links() { ਵਾਪਸ m_links; }

ਜਨਤਕ:
    // List types to store instances of this class
    using List = V3List<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼, &V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::links>;

    explicit V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    ਆਭਾਸੀ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* clone(V3_ਗ੍ਰਾਫ* graphp) ਅਚਲ VL_MT_DISABLED {
        ਵਾਪਸ new V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp, *this};
    }
    ਆਭਾਸੀ ~V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼() = ਮੂਲ;
    ਖਾਲੀ unlink_ਕਿਨਾਰੇ(V3_ਗ੍ਰਾਫ* graphp) VL_MT_DISABLED;
    ਖਾਲੀ unlinkDelete(V3_ਗ੍ਰਾਫ* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    ਸਾਂਚਾ <typename T>
    bool is() ਅਚਲ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼, T>::ਮੁੱਲ,
                      "'T' must be a subtype of V3GraphVertex");
        ਵਾਪਸ V3Rtti::is_Instance_ਦਾ<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ਸਾਂਚਾ <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ਵਾਪਸ static_cast<T*>(this);
    }
    ਸਾਂਚਾ <typename T>
    ਅਚਲ T* as() ਅਚਲ {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ਵਾਪਸ static_cast<ਅਚਲ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ਸਾਂਚਾ <typename T>
    T* cast() {
        ਵਾਪਸ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ਸਾਂਚਾ <typename T>
    ਅਚਲ T* cast() ਅਚਲ {
        ਵਾਪਸ is<T>() ? static_cast<ਅਚਲ T*>(this) : nullptr;
    }

    // ACCESSORS
    ਆਭਾਸੀ string name() ਅਚਲ { ਵਾਪਸ ""; }
    ਆਭਾਸੀ string dotColor() ਅਚਲ { ਵਾਪਸ "black"; }
    ਆਭਾਸੀ string dotShape() ਅਚਲ { ਵਾਪਸ ""; }
    ਆਭਾਸੀ string dotStyle() ਅਚਲ { ਵਾਪਸ ""; }
    ਆਭਾਸੀ string dotName() ਅਚਲ { ਵਾਪਸ ""; }
    ਆਭਾਸੀ string dotRank() ਅਚਲ { ਵਾਪਸ ""; }
    ਆਭਾਸੀ uint32_t rankAdder() ਅਚਲ { ਵਾਪਸ 1; }
    ਆਭਾਸੀ File_ਸਤਰ* fileline() ਅਚਲ { ਵਾਪਸ nullptr; }  // nullptr for unknown
    ਆਭਾਸੀ ਪੂਰਨ ਛਾਂਟੋ_Cmp(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* rhsp) ਅਚਲ {
        // LHS goes first if of lower rank, or lower fanout
        ਜੇ (m_rank < rhsp->m_rank) ਵਾਪਸ -1;
        ਜੇ (m_rank > rhsp->m_rank) ਵਾਪਸ 1;
        ਜੇ (m_fanout < rhsp->m_fanout) ਵਾਪਸ -1;
        ਜੇ (m_fanout > rhsp->m_fanout) ਵਾਪਸ 1;
        ਵਾਪਸ 0;
    }
    uint32_t color() ਅਚਲ { ਵਾਪਸ m_color; }
    ਖਾਲੀ color(uint32_t color) { m_color = color; }
    uint32_t rank() ਅਚਲ { ਵਾਪਸ m_rank; }
    ਖਾਲੀ rank(uint32_t rank) { m_rank = rank; }
    ਦੁੱਗਣਾ fanout() ਅਚਲ { ਵਾਪਸ m_fanout; }
    ਖਾਲੀ user(uint32_t user) { m_user = user; }
    uint32_t user() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_user; }
    ਖਾਲੀ userp(ਖਾਲੀ* userp) { m_userp = userp; }
    ਖਾਲੀ* userp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_userp; }
    V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::IList& ਵਿੱਚ_ਕਿਨਾਰੇ() { ਵਾਪਸ m_ins; }
    ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::IList& ਵਿੱਚ_ਕਿਨਾਰੇ() ਅਚਲ { ਵਾਪਸ m_ins; }
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਸਤਰਅੰਦਰ ਸਵੈਚਾਲਿਤ& ਕਿਨਾਰੇ();
    ਸਾਂਚਾ <ਗ੍ਰਾਫ_Way::en N_Way>
    ਸਤਰਅੰਦਰ ਅਚਲ ਸਵੈਚਾਲਿਤ& ਕਿਨਾਰੇ() ਅਚਲ;
    bool ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_ins.ਖਾਲੀ_ਥਾਵਾਂ(); }
    bool ਵਿੱਚ_Size1() ਅਚਲ { ਵਾਪਸ m_ins.hasSingleElement(); }
    V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::OList& ਬਾਹਰਲਾ_ਕਿਨਾਰੇ() { ਵਾਪਸ m_outs; }
    ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::OList& ਬਾਹਰਲਾ_ਕਿਨਾਰੇ() ਅਚਲ { ਵਾਪਸ m_outs; }
    bool ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_outs.ਖਾਲੀ_ਥਾਵਾਂ(); }
    bool ਬਾਹਰਲਾ_Size1() ਅਚਲ { ਵਾਪਸ m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    ਖਾਲੀ v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& str) ਅਚਲ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    ਖਾਲੀ v3error_ਸਿਰਾ_Fatal(ਅਚਲ std::ostringstream& str) ਅਚਲ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    ਖਾਲੀ reroute_ਕਿਨਾਰੇ(V3_ਗ੍ਰਾਫ* graphp) VL_MT_DISABLED;
};

std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) VL_MT_DISABLED;

ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ ਸਵੈਚਾਲਿਤ& V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::ਕਿਨਾਰੇ<ਗ੍ਰਾਫ_Way::FORWARD>() {
    ਵਾਪਸ m_outs;
}

ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ ਸਵੈਚਾਲਿਤ& V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::ਕਿਨਾਰੇ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>() {
    ਵਾਪਸ m_ins;
}

ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ ਅਚਲ ਸਵੈਚਾਲਿਤ& V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::ਕਿਨਾਰੇ<ਗ੍ਰਾਫ_Way::FORWARD>() ਅਚਲ {
    ਵਾਪਸ m_outs;
}

ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ ਅਚਲ ਸਵੈਚਾਲਿਤ& V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::ਕਿਨਾਰੇ<ਗ੍ਰਾਫ_Way::ਉਲਟ_2>() ਅਚਲ {
    ਵਾਪਸ m_ins;
}

//============================================================================

class V3_ਗ੍ਰਾਫ VL_NOT_FINAL {
    // MEMBERS
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::List m_vertices;  // All vertices

ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ class V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼;
    ਮਿੱਤਰ class V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ;
    ਮਿੱਤਰ class ਗ੍ਰਾਫ_Acyc;
    // METHODS
    ਦੁੱਗਣਾ ਕ੍ਰਮ_DFSIterate(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) VL_MT_DISABLED;
    ਖਾਲੀ dump_ਕਿਨਾਰਾ(std::ostream& os, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼,
                  ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ) ਅਚਲ VL_MT_DISABLED;
    // ACCESSORS

ਜਨਤਕ:
    V3_ਗ੍ਰਾਫ() VL_MT_DISABLED;
    ਆਭਾਸੀ ~V3_ਗ੍ਰਾਫ() VL_MT_DISABLED;
    ਆਭਾਸੀ string dotRankDir() ਅਚਲ { ਵਾਪਸ "TB"; }  // rankdir for dot plotting

    // METHODS
    ਖਾਲੀ clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_vertices.ਖਾਲੀ_ਥਾਵਾਂ(); }
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::List& vertices() { ਵਾਪਸ m_vertices; }
    ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::List& vertices() ਅਚਲ { ਵਾਪਸ m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    ਖਾਲੀ stronglyConnected(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ਖਾਲੀ rank(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) VL_MT_DISABLED;
    ਖਾਲੀ rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ਖਾਲੀ rankMin(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) VL_MT_DISABLED;
    ਖਾਲੀ rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    ਖਾਲੀ ਛਾਂਟੋ_Vertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    ਖਾਲੀ ਛਾਂਟੋ_ਕਿਨਾਰੇ() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    ਖਾਲੀ ਕ੍ਰਮ() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    ਖਾਲੀ ਕ੍ਰਮ_Pre_Ranked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    ਖਾਲੀ acyclic(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    ਖਾਲੀ remove_Redundant_ਕਿਨਾਰੇ_Max(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    ਖਾਲੀ remove_Redundant_ਕਿਨਾਰੇ_ਜੋੜ(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    ਖਾਲੀ remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    ਖਾਲੀ subtreeLoops(V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp,
                      V3_ਗ੍ਰਾਫ* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    ਖਾਲੀ userClearVertices() VL_MT_DISABLED;
    ਖਾਲੀ user_Clear_ਕਿਨਾਰੇ() VL_MT_DISABLED;

    /// Debugging
    ਖਾਲੀ dump(std::ostream& os = std::cout) ਅਚਲ VL_MT_DISABLED;
    ਖਾਲੀ dumpDotFile(ਅਚਲ string& filename, bool colorAsSubgraph) ਅਚਲ VL_MT_DISABLED;
    ਖਾਲੀ dumpDotFilePrefixed(ਅਚਲ string& nameComment,
                             bool colorAsSubgraph = false) ਅਚਲ VL_MT_DISABLED;
    ਖਾਲੀ dumpDotFilePrefixedAlways(ਅਚਲ string& nameComment,
                                   bool colorAsSubgraph = false) ਅਚਲ VL_MT_DISABLED;
    ਖਾਲੀ dump_ਕਿਨਾਰੇ(std::ostream& os, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& ਸ਼ੀਰਸ਼) ਅਚਲ VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    ਖਾਲੀ hash_ਗ੍ਰਾਫ_Debug(ਅਚਲ ਅੱਖਰ* debugName) ਅਚਲ VL_MT_DISABLED;

    ਸਥਿਰ ਖਾਲੀ selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        ਮਿੱਤਰ class ਗ੍ਰਾਫ_Alg_Parallelism_Report;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_ਕੁੱਲ_ਗ੍ਰਾਫ_ਲਾਗਤ = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_critical_ਰਸਤਾ_ਲਾਗਤ = 0;

        uint64_t m_ਸ਼ੀਰਸ਼_ਗਿਣਤੀ = 0;  // Number of vertexes in the graph
        uint64_t m_ਕਿਨਾਰਾ_ਗਿਣਤੀ = 0;  // Number of edges in the grap

        ParallelismReport() = ਮੂਲ;

    ਜਨਤਕ:
        ~ParallelismReport() = ਮੂਲ;
        ParallelismReport(ਅਚਲ ParallelismReport&) = ਮੂਲ;
        ParallelismReport& ਸੰਚਾਲਕ=(ਅਚਲ ParallelismReport&) = ਮੂਲ;

        uint64_t ਕੁੱਲ_ਗ੍ਰਾਫ_ਲਾਗਤ() ਅਚਲ { ਵਾਪਸ m_ਕੁੱਲ_ਗ੍ਰਾਫ_ਲਾਗਤ; }
        uint64_t critical_ਰਸਤਾ_ਲਾਗਤ() ਅਚਲ { ਵਾਪਸ m_critical_ਰਸਤਾ_ਲਾਗਤ; }
        uint64_t ਸ਼ੀਰਸ਼_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_ਸ਼ੀਰਸ਼_ਗਿਣਤੀ; }
        uint64_t ਕਿਨਾਰਾ_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_ਕਿਨਾਰਾ_ਗਿਣਤੀ; }
        ਦੁੱਗਣਾ parallelism_ਗੁਣਕ() ਅਚਲ {
            ਵਾਪਸ (static_cast<ਦੁੱਗਣਾ>(m_ਕੁੱਲ_ਗ੍ਰਾਫ_ਲਾਗਤ) / m_critical_ਰਸਤਾ_ਲਾਗਤ);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*)> ਸ਼ੀਰਸ਼_ਲਾਗਤ) VL_MT_DISABLED;

    // CALLBACKS
    ਆਭਾਸੀ ਖਾਲੀ loops_ਸੁਨੇਹਾ_Cb(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp, V3_ਕਿਨਾਰਾ_Func_P ਕਿਨਾਰਾ_Funcp) VL_MT_DISABLED;
    ਆਭਾਸੀ string loops_ਸ਼ੀਰਸ਼_Cb(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) VL_MT_DISABLED;
};

//============================================================================

#ਜੇ_ਅੰਤ  // Guard
