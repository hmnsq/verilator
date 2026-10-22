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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3_ನಕ್ಷೆ_H
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3_ನಕ್ಷೆ_H

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3List.h"
#ಅನ್ತರ್ಭಾವ "V3Rtti.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <functional>

class File_ಸಾಲು;
class V3_ನಕ್ಷೆ;
class V3_ನಕ್ಷೆ_ಶೃಂಗ;
class V3_ನಕ್ಷೆ_ಅಂಚು;
class ನಕ್ಷೆ_Acyc_ಅಂಚು;
class ಕ್ರಮ_Either_ಶೃಂಗ;
class ಕ್ರಮ_Logic_ಶೃಂಗ;

//=============================================================================
// Most graph algorithms accept an arbitrary function that returns
// True for those edges we should honor.

using V3_ಅಂಚು_Func_P = bool (*)(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* edgep);

//=============================================================================
// When the Graph represents a directional acyclical graph (DAG), following
// the to() edges is forward, and back() is reverse.  However, sometimes
// it's useful to have algorithms that can walk in either direction, hence
// some methods take GraphWay to programmatically select the direction.

class ನಕ್ಷೆ_Way final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        FORWARD = 0,
        ಹಿಮ್ಮುಖ_2 = 1,
        ಅಂಕೆ_WAYS = 2  // NUM_WAYS is not an actual way, it's typically
        //          // an array dimension or loop bound.
    };
    ಎಣಿಕೆ en m_e;
    ನಕ್ಷೆ_Way()
        : m_e{FORWARD} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr ನಕ್ಷೆ_Way(en _e)
        : m_e{_e} {}
    explicit constexpr ನಕ್ಷೆ_Way(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"FORWARD", "REVERSE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    // METHODS unique to this class
    constexpr ನಕ್ಷೆ_Way invert() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ನಕ್ಷೆ_Way{m_e ^ 1}; }
    constexpr bool forward() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == FORWARD; }
    constexpr bool ಹಿಮ್ಮುಖ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e != FORWARD; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Way& lhs, ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Way& rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Way& lhs, ನಕ್ಷೆ_Way::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ನಕ್ಷೆ_Way::en lhs, ಸ್ಥಿರಾಂಕ ನಕ್ಷೆ_Way& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }

//============================================================================
// TODO should we have a smaller edge structure when we don't need weight etc?

class V3_ನಕ್ಷೆ_ಅಂಚು VL_NOT_FINAL {
    VL_RTTI_IMPL_ತಳ(V3_ನಕ್ಷೆ_ಅಂಚು)
    // Wires/variables aren't edges.  Edges have only a single to/from vertex
ಸಾರ್ವಜನಿಕ:
    // ENUMS
    ಎಣಿಕೆ Cutable : uint8_t { NOT_CUTABLE = false, CUTABLE = true };  // For passing to V3GraphEdge
ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ class V3_ನಕ್ಷೆ;
    ಮಿತ್ರ class V3_ನಕ್ಷೆ_ಶೃಂಗ;
    ಮಿತ್ರ class ನಕ್ಷೆ_Acyc;
    ಮಿತ್ರ class ನಕ್ಷೆ_Acyc_ಅಂಚು;

    V3ListLinks<V3_ನಕ್ಷೆ_ಅಂಚು> m_oLinks;  // List links to store instances of this class (out edges)
    V3ListLinks<V3_ನಕ್ಷೆ_ಅಂಚು> m_iLinks;  // List links to store instances of this class (in edges)
    //
    V3_ನಕ್ಷೆ_ಶೃಂಗ* m_fromp;  // Vertices pointing to this edge
    V3_ನಕ್ಷೆ_ಶೃಂಗ* m_ತುದಿ;  // Vertices this edge points to
    ಪೂರ್ಣಾಂಕ m_ತೂಕ;  // Weight of the connection
    bool m_cutable;  // Interconnect may be broken in order sorting
    ಒಕ್ಕೂಟ {
        ಶೂನ್ಯ* m_userp;  // Marker for some algorithms
        uint64_t m_user;  // Marker for some algorithms
    };
    // METHODS
    ಶೂನ್ಯ init(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2,
              bool cutable = false) VL_MT_DISABLED;
    ಶೂನ್ಯ cut() { m_ತೂಕ = 0; }  // 0 weight is same as disconnected
    // CONSTRUCTORS
    V3_ನಕ್ಷೆ_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ,
                ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& old) VL_MT_DISABLED {
        init(graphp, fromp, ತುದಿ, old.m_ತೂಕ, old.m_cutable);
    }

ಖಾಸಗಿ:
    V3ListLinks<V3_ನಕ್ಷೆ_ಅಂಚು>& oLinks() { ಹಿಂತಿರುಗಿ m_oLinks; }
    V3ListLinks<V3_ನಕ್ಷೆ_ಅಂಚು>& iLinks() { ಹಿಂತಿರುಗಿ m_iLinks; }

ಸಾರ್ವಜನಿಕ:
    // List types to store instances of this class
    using OList = V3List<V3_ನಕ್ಷೆ_ಅಂಚು, &V3_ನಕ್ಷೆ_ಅಂಚು::oLinks>;
    using IList = V3List<V3_ನಕ್ಷೆ_ಅಂಚು, &V3_ನಕ್ಷೆ_ಅಂಚು::iLinks>;

    //! Add DAG from one node to the specified node
    V3_ನಕ್ಷೆ_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2,
                bool cutable = false) VL_MT_DISABLED {
        init(graphp, fromp, ತುದಿ, ತೂಕ_2, cutable);
    }
    //! Clone copy constructor.  Doesn't copy existing vertices or user/userp.
    ವಾಸ್ತವಿಕ V3_ನಕ್ಷೆ_ಅಂಚು* clone(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp,
                               V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED {
        ಹಿಂತಿರುಗಿ new V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, *this};
    }
    ವಾಸ್ತವಿಕ ~V3_ನಕ್ಷೆ_ಅಂಚು() = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    // Return true iff of type T
    ಮಾದರಿ <typename T>
    bool is() ಸ್ಥಿರಾಂಕ {
        static_assert(std::is_ತಳ_ದ<V3_ನಕ್ಷೆ_ಅಂಚು, T>::ಮೌಲ್ಯ,
                      "'T' must be a subtype of V3GraphEdge");
        ಹಿಂತಿರುಗಿ V3Rtti::is_Instance_ದ<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ಮಾದರಿ <typename T>
    T* as() {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ಹಿಂತಿರುಗಿ static_cast<T*>(this);
    }
    ಮಾದರಿ <typename T>
    ಸ್ಥಿರಾಂಕ T* as() ಸ್ಥಿರಾಂಕ {
        UASSERT(is<T>(), "V3GraphEdge is not of expected type");
        ಹಿಂತಿರುಗಿ static_cast<ಸ್ಥಿರಾಂಕ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ಮಾದರಿ <typename T>
    T* cast() {
        ಹಿಂತಿರುಗಿ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ಮಾದರಿ <typename T>
    ಸ್ಥಿರಾಂಕ T* cast() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ is<T>() ? static_cast<ಸ್ಥಿರಾಂಕ T*>(this) : nullptr;
    }

    ವಾಸ್ತವಿಕ string name() ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ವಾಸ್ತವಿಕ string dotLabel() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    ವಾಸ್ತವಿಕ string dotColor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ cutable() ? "yellowGreen" : "red"; }
    ವಾಸ್ತವಿಕ string dotStyle() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ cutable() ? "dashed" : ""; }
    ವಾಸ್ತವಿಕ ಪೂರ್ಣಾಂಕ ವಿಂಗಡಿಸು_Cmp(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* rhsp) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಶೂನ್ಯ unlinkDelete() VL_MT_DISABLED;
    ಶೂನ್ಯ relinkFromp(V3_ನಕ್ಷೆ_ಶೃಂಗ* newFromp) VL_MT_DISABLED;
    ಶೂನ್ಯ relink_ತುದಿ(V3_ನಕ್ಷೆ_ಶೃಂಗ* new_ತುದಿ) VL_MT_DISABLED;
    // ACCESSORS
    ಪೂರ್ಣಾಂಕ ತೂಕ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತೂಕ; }
    ಶೂನ್ಯ ತೂಕ_2(ಪೂರ್ಣಾಂಕ ತೂಕ_2) { m_ತೂಕ = ತೂಕ_2; }
    bool cutable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cutable; }
    ಶೂನ್ಯ cutable(bool cutable) { m_cutable = cutable; }
    ಶೂನ್ಯ userp(ಶೂನ್ಯ* user) { m_userp = user; }
    ಶೂನ್ಯ* userp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_userp; }
    ಶೂನ್ಯ user(uint64_t user) { m_user = user; }
    uint64_t user() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_user; }
    V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fromp; }
    V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತುದಿ; }
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    V3_ನಕ್ಷೆ_ಶೃಂಗ* furtherp() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ N_Way == ನಕ್ಷೆ_Way::FORWARD ? ತುದಿ() : fromp();
    }
    // STATIC ACCESSORS
    ಸ್ಥಿರ bool followNotCutable(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು* edgep) { ಹಿಂತಿರುಗಿ !edgep->m_cutable; }
    ಸ್ಥಿರ bool followAlwaysTrue(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು*) { ಹಿಂತಿರುಗಿ true; }
};

//============================================================================

class V3_ನಕ್ಷೆ_ಶೃಂಗ VL_NOT_FINAL {
    VL_RTTI_IMPL_ತಳ(V3_ನಕ್ಷೆ_ಶೃಂಗ)
    ಮಿತ್ರ class V3_ನಕ್ಷೆ;
    ಮಿತ್ರ class V3_ನಕ್ಷೆ_ಅಂಚು;
    ಮಿತ್ರ class ನಕ್ಷೆ_Acyc;
    ಮಿತ್ರ class ನಕ್ಷೆ_Alg_Rank;
    ಮಿತ್ರ class ನಕ್ಷೆ_Alg_Rank_ಆಳ;
    V3ListLinks<V3_ನಕ್ಷೆ_ಶೃಂಗ> m_links;  // List links to store instances of this class
    V3_ನಕ್ಷೆ_ಅಂಚು::OList m_outs;  // List of outbound edges
    V3_ನಕ್ಷೆ_ಅಂಚು::IList m_ins;  // List of inbound edges
    ದ್ವಿಗುಣ m_fanout;  // Order fanout
    uint32_t m_color;  // Color of the node
    uint32_t m_rank;  // Rank of edge
    ಒಕ್ಕೂಟ {
        ಶೂನ್ಯ* m_userp;  // Marker for some algorithms
        uint32_t m_user;  // Marker for some algorithms
    };
    // ACCESSORS
    ಶೂನ್ಯ fanout(ದ್ವಿಗುಣ fanout) { m_fanout = fanout; }

ಸಂರಕ್ಷಿತ:
    // CONSTRUCTORS
    V3_ನಕ್ಷೆ_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& old) VL_MT_DISABLED;

ಖಾಸಗಿ:
    V3ListLinks<V3_ನಕ್ಷೆ_ಶೃಂಗ>& links() { ಹಿಂತಿರುಗಿ m_links; }

ಸಾರ್ವಜನಿಕ:
    // List types to store instances of this class
    using List = V3List<V3_ನಕ್ಷೆ_ಶೃಂಗ, &V3_ನಕ್ಷೆ_ಶೃಂಗ::links>;

    explicit V3_ನಕ್ಷೆ_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp) VL_MT_DISABLED;
    //! Clone copy constructor. Doesn't copy edges or user/userp.
    ವಾಸ್ತವಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* clone(V3_ನಕ್ಷೆ* graphp) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED {
        ಹಿಂತಿರುಗಿ new V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp, *this};
    }
    ವಾಸ್ತವಿಕ ~V3_ನಕ್ಷೆ_ಶೃಂಗ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ unlink_ಅಂಚುಗಳು(V3_ನಕ್ಷೆ* graphp) VL_MT_DISABLED;
    ಶೂನ್ಯ unlinkDelete(V3_ನಕ್ಷೆ* graphp) VL_MT_DISABLED;

    // METHODS
    // Return true iff of type T
    ಮಾದರಿ <typename T>
    bool is() ಸ್ಥಿರಾಂಕ {
        static_assert(std::is_ತಳ_ದ<V3_ನಕ್ಷೆ_ಶೃಂಗ, T>::ಮೌಲ್ಯ,
                      "'T' must be a subtype of V3GraphVertex");
        ಹಿಂತಿರುಗಿ V3Rtti::is_Instance_ದ<T>(this);
    }

    // Return cast to subtype T and assert of that type
    ಮಾದರಿ <typename T>
    T* as() {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ಹಿಂತಿರುಗಿ static_cast<T*>(this);
    }
    ಮಾದರಿ <typename T>
    ಸ್ಥಿರಾಂಕ T* as() ಸ್ಥಿರಾಂಕ {
        UASSERT_OBJ(is<T>(), this, "V3GraphVertex is not of expected type");
        ಹಿಂತಿರುಗಿ static_cast<ಸ್ಥಿರಾಂಕ T*>(this);
    }

    // Return cast to subtype T, else nullptr if different type
    ಮಾದರಿ <typename T>
    T* cast() {
        ಹಿಂತಿರುಗಿ is<T>() ? static_cast<T*>(this) : nullptr;
    }
    ಮಾದರಿ <typename T>
    ಸ್ಥಿರಾಂಕ T* cast() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ is<T>() ? static_cast<ಸ್ಥಿರಾಂಕ T*>(this) : nullptr;
    }

    // ACCESSORS
    ವಾಸ್ತವಿಕ string name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    ವಾಸ್ತವಿಕ string dotColor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "black"; }
    ವಾಸ್ತವಿಕ string dotShape() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    ವಾಸ್ತವಿಕ string dotStyle() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    ವಾಸ್ತವಿಕ string dotName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    ವಾಸ್ತವಿಕ string dotRank() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    ವಾಸ್ತವಿಕ uint32_t rankAdder() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ 1; }
    ವಾಸ್ತವಿಕ File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ nullptr; }  // nullptr for unknown
    ವಾಸ್ತವಿಕ ಪೂರ್ಣಾಂಕ ವಿಂಗಡಿಸು_Cmp(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* rhsp) ಸ್ಥಿರಾಂಕ {
        // LHS goes first if of lower rank, or lower fanout
        ಆದರೆ (m_rank < rhsp->m_rank) ಹಿಂತಿರುಗಿ -1;
        ಆದರೆ (m_rank > rhsp->m_rank) ಹಿಂತಿರುಗಿ 1;
        ಆದರೆ (m_fanout < rhsp->m_fanout) ಹಿಂತಿರುಗಿ -1;
        ಆದರೆ (m_fanout > rhsp->m_fanout) ಹಿಂತಿರುಗಿ 1;
        ಹಿಂತಿರುಗಿ 0;
    }
    uint32_t color() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_color; }
    ಶೂನ್ಯ color(uint32_t color) { m_color = color; }
    uint32_t rank() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rank; }
    ಶೂನ್ಯ rank(uint32_t rank) { m_rank = rank; }
    ದ್ವಿಗುಣ fanout() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fanout; }
    ಶೂನ್ಯ user(uint32_t user) { m_user = user; }
    uint32_t user() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_user; }
    ಶೂನ್ಯ userp(ಶೂನ್ಯ* userp) { m_userp = userp; }
    ಶೂನ್ಯ* userp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_userp; }
    V3_ನಕ್ಷೆ_ಅಂಚು::IList& ಒಳ_ಅಂಚುಗಳು() { ಹಿಂತಿರುಗಿ m_ins; }
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು::IList& ಒಳ_ಅಂಚುಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ins; }
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಸಾಲೊಳಗೆ ಸ್ವಯಂಚಾಲಿತ& ಅಂಚುಗಳು();
    ಮಾದರಿ <ನಕ್ಷೆ_Way::en N_Way>
    ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಅಂಚುಗಳು() ಸ್ಥಿರಾಂಕ;
    bool ಒಳ_ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ins.ಖಾಲಿಗಳು(); }
    bool ಒಳ_Size1() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ins.hasSingleElement(); }
    V3_ನಕ್ಷೆ_ಅಂಚು::OList& ಹೊರಹೋಗು_ಅಂಚುಗಳು() { ಹಿಂತಿರುಗಿ m_outs; }
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು::OList& ಹೊರಹೋಗು_ಅಂಚುಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_outs; }
    bool ಹೊರಹೋಗು_ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_outs.ಖಾಲಿಗಳು(); }
    bool ಹೊರಹೋಗು_Size1() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_outs.hasSingleElement(); }
    // METHODS
    /// Error reporting
    ಶೂನ್ಯ v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    ಶೂನ್ಯ v3error_ಕೊನೆ_Fatal(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ
        VL_RELEASE(V3Error::s().m_mutex) VL_MT_DISABLED;
    /// Edges are routed around this vertex to point from "from" directly to "to"
    ಶೂನ್ಯ reroute_ಅಂಚುಗಳು(V3_ನಕ್ಷೆ* graphp) VL_MT_DISABLED;
};

std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) VL_MT_DISABLED;

ಮಾದರಿ <>
ಸಾಲೊಳಗೆ ಸ್ವಯಂಚಾಲಿತ& V3_ನಕ್ಷೆ_ಶೃಂಗ::ಅಂಚುಗಳು<ನಕ್ಷೆ_Way::FORWARD>() {
    ಹಿಂತಿರುಗಿ m_outs;
}

ಮಾದರಿ <>
ಸಾಲೊಳಗೆ ಸ್ವಯಂಚಾಲಿತ& V3_ನಕ್ಷೆ_ಶೃಂಗ::ಅಂಚುಗಳು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>() {
    ಹಿಂತಿರುಗಿ m_ins;
}

ಮಾದರಿ <>
ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& V3_ನಕ್ಷೆ_ಶೃಂಗ::ಅಂಚುಗಳು<ನಕ್ಷೆ_Way::FORWARD>() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ m_outs;
}

ಮಾದರಿ <>
ಸಾಲೊಳಗೆ ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& V3_ನಕ್ಷೆ_ಶೃಂಗ::ಅಂಚುಗಳು<ನಕ್ಷೆ_Way::ಹಿಮ್ಮುಖ_2>() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ m_ins;
}

//============================================================================

class V3_ನಕ್ಷೆ VL_NOT_FINAL {
    // MEMBERS
    V3_ನಕ್ಷೆ_ಶೃಂಗ::List m_vertices;  // All vertices

ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ class V3_ನಕ್ಷೆ_ಶೃಂಗ;
    ಮಿತ್ರ class V3_ನಕ್ಷೆ_ಅಂಚು;
    ಮಿತ್ರ class ನಕ್ಷೆ_Acyc;
    // METHODS
    ದ್ವಿಗುಣ ಕ್ರಮ_DFSIterate(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) VL_MT_DISABLED;
    ಶೂನ್ಯ dump_ಅಂಚು(std::ostream& os, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ,
                  ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    // ACCESSORS

ಸಾರ್ವಜನಿಕ:
    V3_ನಕ್ಷೆ() VL_MT_DISABLED;
    ವಾಸ್ತವಿಕ ~V3_ನಕ್ಷೆ() VL_MT_DISABLED;
    ವಾಸ್ತವಿಕ string dotRankDir() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "TB"; }  // rankdir for dot plotting

    // METHODS
    ಶೂನ್ಯ clear() VL_MT_DISABLED;  // Empty it of all vertices/edges, as if making a new object
    bool ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vertices.ಖಾಲಿಗಳು(); }
    V3_ನಕ್ಷೆ_ಶೃಂಗ::List& vertices() { ಹಿಂತಿರುಗಿ m_vertices; }
    ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ::List& vertices() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vertices; }

    // METHODS - ALGORITHMS

    /// Assign same color to all vertices in the same weakly connected component
    /// Thus different color if there's no edges between the two subgraphs
    /// Colors are assigned densely, as 0 .. n-1, and 'n' is returned
    uint32_t weaklyConnected(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) VL_MT_DISABLED;

    /// Assign same color to all vertices that are strongly connected
    /// Thus different color if there's no directional circuit within the subgraphs.
    /// (I.E. all loops will occur within each color, not between them.)
    /// Side-effect: changes user()
    ಶೂನ್ಯ stronglyConnected(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the maximum of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ಶೂನ್ಯ rank(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) VL_MT_DISABLED;
    ಶೂನ್ಯ rank() VL_MT_DISABLED;

    /// Assign an ordering number to all vertexes in a tree.
    /// For multiple usages of a vertex, get the minimu of all inbound ranks + 1
    /// All nodes with no inputs will get rank 1
    /// Side-effect: changes user()
    ಶೂನ್ಯ rankMin(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) VL_MT_DISABLED;
    ಶೂನ್ಯ rankMin() VL_MT_DISABLED;

    /// Sort all vertices and edges using the V3GraphVertex::sortCmp() function
    ಶೂನ್ಯ ವಿಂಗಡಿಸು_Vertices() VL_MT_DISABLED;
    /// Sort all edges and edges using the V3GraphEdge::sortCmp() function
    ಶೂನ್ಯ ವಿಂಗಡಿಸು_ಅಂಚುಗಳು() VL_MT_DISABLED;

    /// Order all vertices by rank and fanout, lowest first
    /// Sort all vertices by rank and fanout, lowest first
    /// Sort all edges by weight, lowest first
    /// Side-effect: assigns ranks to every node, and changes user()
    ಶೂನ್ಯ ಕ್ರಮ() VL_MT_DISABLED;

    /// Similar to order() but does not assign ranks. Caller must
    /// ensure that the graph has been ranked ahead of the call.
    /// Side-effect: assigns ranks to every node, and changes user()
    ಶೂನ್ಯ ಕ್ರಮ_Pre_Ranked() VL_MT_DISABLED;

    /// Make acyclical (into a tree) by breaking a minimal subset of cutable edges.
    /// Side-effect: changes rank(), changes user()
    ಶೂನ್ಯ acyclic(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become MAX of any other weight
    /// Side-effect: changes user()
    ಶೂನ್ಯ remove_Redundant_ಅಂಚುಗಳು_Max(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) VL_MT_DISABLED;

    /// Remove any redundant edges, weights become SUM of any other weight
    /// Side-effect: changes user()
    ಶೂನ್ಯ remove_Redundant_ಅಂಚುಗಳು_ಮೊತ್ತ(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) VL_MT_DISABLED;

    /// Remove any transitive edges.  E.g. if have edges A->B, B->C, and A->C
    /// then A->C is a "transitive" edge; it's implied by the first two
    /// (assuming the DAG is a dependency graph.)
    /// This algorithm can be expensive.
    ಶೂನ್ಯ remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು() VL_MT_DISABLED;

    /// Call loopsVertexCb on any one loop starting where specified
    /// Side-effect: changes user()
    string reportLoops(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp, V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) VL_MT_DISABLED;

    /// Build a subgraph of all loops starting where specified
    /// Side-effect: changes user()
    ಶೂನ್ಯ subtreeLoops(V3_ಅಂಚು_Func_P ಅಂಚು_Funcp, V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp,
                      V3_ನಕ್ಷೆ* loopGraphp) VL_MT_DISABLED;

    /// Clear user()
    ಶೂನ್ಯ userClearVertices() VL_MT_DISABLED;
    ಶೂನ್ಯ user_Clear_ಅಂಚುಗಳು() VL_MT_DISABLED;

    /// Debugging
    ಶೂನ್ಯ dump(std::ostream& os = std::cout) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಶೂನ್ಯ dumpDotFile(ಸ್ಥಿರಾಂಕ string& filename, bool colorAsSubgraph) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಶೂನ್ಯ dumpDotFilePrefixed(ಸ್ಥಿರಾಂಕ string& nameComment,
                             bool colorAsSubgraph = false) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಶೂನ್ಯ dumpDotFilePrefixedAlways(ಸ್ಥಿರಾಂಕ string& nameComment,
                                   bool colorAsSubgraph = false) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಶೂನ್ಯ dump_ಅಂಚುಗಳು(std::ostream& os, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ& ಶೃಂಗ) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    // Print a hash of the shape of graphp. When debugging nondeterminism, this can help
    // pinpoint where it's coming from.
    ಶೂನ್ಯ hash_ನಕ್ಷೆ_Debug(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* debugName) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;

    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() VL_MT_DISABLED;

    class ParallelismReport final {
        ಮಿತ್ರ class ನಕ್ಷೆ_Alg_Parallelism_Report;
        // Total cost of evaluating the whole graph. The ratio of m_totalGraphCost to
        // m_criticalPathCost gives us an estimate of the parallelizability of this graph which is
        // only as good as the guess returned by vertexCost.
        uint64_t m_ಒಟ್ಟು_ನಕ್ಷೆ_ವೆಚ್ಚ = 0;

        // Cost of the critical path, in abstract units (the same units returned by the vertexCost)
        uint64_t m_critical_ಮಾರ್ಗ_ವೆಚ್ಚ = 0;

        uint64_t m_ಶೃಂಗ_ಎಣಿಕೆ = 0;  // Number of vertexes in the graph
        uint64_t m_ಅಂಚು_ಎಣಿಕೆ = 0;  // Number of edges in the grap

        ParallelismReport() = ಪೂರ್ವನಿಯೋಜಿತ;

    ಸಾರ್ವಜನಿಕ:
        ~ParallelismReport() = ಪೂರ್ವನಿಯೋಜಿತ;
        ParallelismReport(ಸ್ಥಿರಾಂಕ ParallelismReport&) = ಪೂರ್ವನಿಯೋಜಿತ;
        ParallelismReport& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ ParallelismReport&) = ಪೂರ್ವನಿಯೋಜಿತ;

        uint64_t ಒಟ್ಟು_ನಕ್ಷೆ_ವೆಚ್ಚ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಟ್ಟು_ನಕ್ಷೆ_ವೆಚ್ಚ; }
        uint64_t critical_ಮಾರ್ಗ_ವೆಚ್ಚ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_critical_ಮಾರ್ಗ_ವೆಚ್ಚ; }
        uint64_t ಶೃಂಗ_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಶೃಂಗ_ಎಣಿಕೆ; }
        uint64_t ಅಂಚು_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಚು_ಎಣಿಕೆ; }
        ದ್ವಿಗುಣ parallelism_ಗುಣಕ() ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ (static_cast<ದ್ವಿಗುಣ>(m_ಒಟ್ಟು_ನಕ್ಷೆ_ವೆಚ್ಚ) / m_critical_ಮಾರ್ಗ_ವೆಚ್ಚ);
        }
    };

    ParallelismReport
    parallelismReport(std::function<uint64_t(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ*)> ಶೃಂಗ_ವೆಚ್ಚ) VL_MT_DISABLED;

    // CALLBACKS
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ loops_ಸಂದೇಶ_Cb(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp, V3_ಅಂಚು_Func_P ಅಂಚು_Funcp) VL_MT_DISABLED;
    ವಾಸ್ತವಿಕ string loops_ಶೃಂಗ_Cb(V3_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) VL_MT_DISABLED;
};

//============================================================================

#ಆದರೆ_ಅಂತ್ಯ  // Guard
