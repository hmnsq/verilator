// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Ast node structure
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3AST_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3AST_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Broken.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3FunctionTraits.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Number.h"
#ಅನ್ತರ್ಭಾವ "V3StdFuture.h"

#ಅನ್ತರ್ಭಾವ "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <cmath>
#ಅನ್ತರ್ಭಾವ <cstdint>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#ಆದರೆ defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_USER_TYPE_CHECKS
#ಆದರೆ_ಅಂತ್ಯ
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_USER_TYPE_CHECKS
#ಅನ್ತರ್ಭಾವ <variant>
#ಆದರೆ_ಅಂತ್ಯ

// clang-format off
#ಅನ್ತರ್ಭಾವ "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3_ನಕ್ಷೆ;
class ExecMTask;
class VIface_Capture_ಟ್ಯಾಗ್;

//######################################################################

// For broken() function, return error string if have a match
#ವ್ಯಾಖ್ಯಾನಿಸು BROKEN_RTN(test) \
    ಮಾಡು { \
        ಆದರೆ (VL_UNCOVERABLE(test)) ಹಿಂತಿರುಗಿ "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } ಆಗ (false)
// For broken() function, return error string if a base of this class has a match
#ವ್ಯಾಖ್ಯಾನಿಸು BROKEN_ತಳ_RTN(test) \
    ಮಾಡು { \
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ reasonp = (test); \
        ಆದರೆ (VL_UNCOVERABLE(reasonp)) ಹಿಂತಿರುಗಿ reasonp; \
    } ಆಗ (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#ವ್ಯಾಖ್ಯಾನಿಸು VN_IS(nodep, nodetypename) (Ast_ಗಂಟು::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#ವ್ಯಾಖ್ಯಾನಿಸು VN_CAST(nodep, nodetypename) (Ast_ಗಂಟು::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#ವ್ಯಾಖ್ಯಾನಿಸು VN_AS(nodep, nodetypename) (Ast_ಗಂಟು::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#ಆದರೆ defined(VL_DEBUG) || defined(VL_ASAN)
#ವ್ಯಾಖ್ಯಾನಿಸು VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#ಇಲ್ಲದಿದ್ದರೆ
#ವ್ಯಾಖ್ಯಾನಿಸು VN_DBG_AS(nodep, nodetypename) (Ast_ಗಂಟು::unsafe_ಖಾಸಗಿ_As<Ast##nodetypename>(nodep))
#ಆದರೆ_ಅಂತ್ಯ

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#ವ್ಯಾಖ್ಯಾನಿಸು VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

ರಚನೆ VNTypeInfo final {
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_namep;
    ಎಣಿಕೆ OpEn : uint8_t {
        OP_ಬಳಸದ,
        OP_ಬಳಸಿದ,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_opNamep[4];
    ಗಾತ್ರ_t m_sizeof;
};

class VNType final {
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ VNTypeInfo s_type_Info_ಕೋಷ್ಟಕ[];

ಸಾರ್ವಜನಿಕ:
#ಅನ್ತರ್ಭಾವ "V3Ast__gen_type_enum.h"  // From ./astgen
    ಸ್ಥಿರಾಂಕ en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_ಸುರಕ್ಷಿತ : m_e{_e} {}
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e; }

    // Retrieve VNTypeInfo for the given type
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ VNTypeInfo& typeInfo(VNType t) VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ s_type_Info_ಕೋಷ್ಟಕ[t.m_e]; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VNType& lhs, ಸ್ಥಿರಾಂಕ VNType& rhs) VL_ಶುದ್ಧ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VNType& lhs, VNType::en rhs) VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VNType::en lhs, ಸ್ಥಿರಾಂಕ VNType& rhs) VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VNType& rhs) { ಹಿಂತಿರುಗಿ os << rhs.ascii(); }

//######################################################################
// AstNUser - Generic base class for AST User nodes.
//          - Also used to allow parameter passing up/down iterate calls

class ಅಗಲ_VP;
class V3_ನಕ್ಷೆ_ಶೃಂಗ;
class VSymEnt;

class VNUser final {
// Defining VL_USER_TYPE_CHECKS replaces the VNUser union with a std::variant
// which remembers which form of VNUser is being stored to catch mismatched reads.
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_USER_TYPE_CHECKS
    // monostate is an unwritten / cleared slot. It can be read as either form
    // and yields nullptr/0.
    std::variant<std::monostate, uint64_t, ಶೂನ್ಯ*> m_u;
#ಇಲ್ಲದಿದ್ದರೆ
    ಒಕ್ಕೂಟ {
        ಶೂನ್ಯ* ಮೇಲೆ;
        uint64_t uq;
    } m_u;
#ಆದರೆ_ಅಂತ್ಯ

ಸಾರ್ವಜನಿಕ:
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_USER_TYPE_CHECKS
    VNUser() = ಪೂರ್ವನಿಯೋಜಿತ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        ಆದರೆ (i) m_u = i;
    }
    explicit VNUser(ಶೂನ್ಯ* p) {
        // VNUser{nullptr} represents the monostate
        ಆದರೆ (p) m_u = p;
    }
    ~VNUser() = ಪೂರ್ವನಿಯೋಜಿತ;
    // Casters
    ಮಾದರಿ <typename T>
    typename std::enable_if<std::is_ಸೂಚಕ<T>::ಮೌಲ್ಯ, T>::type ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆದರೆ (std::holds_alternative<std::monostate>(m_u)) ಹಿಂತಿರುಗಿ nullptr;
        ಶೂನ್ಯ* ಸ್ಥಿರಾಂಕ* ಸ್ಥಿರಾಂಕ upp = std::get_if<ಶೂನ್ಯ*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        ಹಿಂತಿರುಗಿ reinterpret_cast<T>(*upp);
    }
    uint64_t ಗಮ್ಯ_UQuad() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (std::holds_alternative<std::monostate>(m_u)) ಹಿಂತಿರುಗಿ 0;
        ಸ್ಥಿರಾಂಕ uint64_t* ಸ್ಥಿರಾಂಕ uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        ಹಿಂತಿರುಗಿ *uip;
    }
#ಇಲ್ಲದಿದ್ದರೆ
    VNUser() = ಪೂರ್ವನಿಯೋಜಿತ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.ಮೇಲೆ = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(ಶೂನ್ಯ* p) { m_u.ಮೇಲೆ = p; }
    ~VNUser() = ಪೂರ್ವನಿಯೋಜಿತ;
    // Casters
    ಮಾದರಿ <typename T>
    typename std::enable_if<std::is_ಸೂಚಕ<T>::ಮೌಲ್ಯ, T>::type ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ reinterpret_cast<T>(m_u.ಮೇಲೆ);
    }
    uint64_t ಗಮ್ಯ_UQuad() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_u.uq; }
#ಆದರೆ_ಅಂತ್ಯ
    VSymEnt* ಗಮ್ಯ_Sym_Ent() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಗಮ್ಯ<VSymEnt*>(); }
    Ast_ಗಂಟು* ಗಮ್ಯ_Nodep() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ ಗಮ್ಯ<Ast_ಗಂಟು*>(); }
    V3_ನಕ್ಷೆ_ಶೃಂಗ* ಗಮ್ಯ_ನಕ್ಷೆ_ಶೃಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಗಮ್ಯ<V3_ನಕ್ಷೆ_ಶೃಂಗ*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*)) ಸ್ಥಿರಾಂಕ;
};

//######################################################################
// AstUserResource - Generic pointer base class for tracking usage of user()
//
//  Where AstNode->user2() is going to be used, for example, you write:
//
//      VNUser2InUse  m_userres;
//
//  This will clear the tree, and prevent another visitor from clobbering
//  user2.  When the member goes out of scope it will be automagically
//  freed up.

class VNUser_ಒಳ_Use_ತಳ
VL_NOT_FINAL{ಸಂರಕ್ಷಿತ : ಸ್ಥಿರ ಶೂನ್ಯ allocate(ಪೂರ್ಣಾಂಕ id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvt_ಗಮ್ಯ_Str(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
ಸ್ಥಿರ ಶೂನ್ಯ ಬಿಡುಗಡೆ(ಪೂರ್ಣಾಂಕ id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvt_ಗಮ್ಯ_Str(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
ಸ್ಥಿರ ಶೂನ್ಯ clearcnt(ಪೂರ್ಣಾಂಕ id, uint32_t& cntGblRef, ಸ್ಥಿರಾಂಕ bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvt_ಗಮ್ಯ_Str(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
ಸ್ಥಿರ ಶೂನ್ಯ checkcnt(ಪೂರ್ಣಾಂಕ id, uint32_t&, ಸ್ಥಿರಾಂಕ bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef,
                   "Check of User" + cvt_ಗಮ್ಯ_Str(id) + "() failed, not under AstUserInUse");
}
}
;

// For each user() declare the in use structure
// We let AstNode peek into here, because when under low optimization even
// an accessor would be way too slow.
// clang-format off
class VNUser1_ಒಳ_Use final : VNUser_ಒಳ_Use_ತಳ {
ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ class Ast_ಗಂಟು;
    ಸ್ಥಿರ uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ಸ್ಥಿರ bool s_userBusy;  // Count is in use
ಸಾರ್ವಜನಿಕ:
    VNUser1_ಒಳ_Use()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1_ಒಳ_Use()     { ಬಿಡುಗಡೆ    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ಸ್ಥಿರ ಶೂನ್ಯ clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ಸ್ಥಿರ ಶೂನ್ಯ check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2_ಒಳ_Use final : VNUser_ಒಳ_Use_ತಳ {
ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ class Ast_ಗಂಟು;
    ಸ್ಥಿರ uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ಸ್ಥಿರ bool s_userBusy;  // Count is in use
ಸಾರ್ವಜನಿಕ:
    VNUser2_ಒಳ_Use()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2_ಒಳ_Use()     { ಬಿಡುಗಡೆ    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ಸ್ಥಿರ ಶೂನ್ಯ clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ಸ್ಥಿರ ಶೂನ್ಯ check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3_ಒಳ_Use final : VNUser_ಒಳ_Use_ತಳ {
ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ class Ast_ಗಂಟು;
    ಸ್ಥಿರ uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ಸ್ಥಿರ bool s_userBusy;  // Count is in use
ಸಾರ್ವಜನಿಕ:
    VNUser3_ಒಳ_Use()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3_ಒಳ_Use()     { ಬಿಡುಗಡೆ    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ಸ್ಥಿರ ಶೂನ್ಯ clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ಸ್ಥಿರ ಶೂನ್ಯ check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4_ಒಳ_Use final : VNUser_ಒಳ_Use_ತಳ {
ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ class Ast_ಗಂಟು;
    ಸ್ಥಿರ uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ಸ್ಥಿರ bool s_userBusy;  // Count is in use
ಸಾರ್ವಜನಿಕ:
    VNUser4_ಒಳ_Use()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4_ಒಳ_Use()     { ಬಿಡುಗಡೆ    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ಸ್ಥಿರ ಶೂನ್ಯ clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ಸ್ಥಿರ ಶೂನ್ಯ check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::ಸದಿಶ<Ast_ಗಂಟು*> m_deleteps;  // Nodes to delete

ಸಾರ್ವಜನಿಕ:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    ಶೂನ್ಯ ತಳ್ಳು_Deletep(Ast_ಗಂಟು* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    ಶೂನ್ಯ doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    ಮಿತ್ರ class Ast_ಗಂಟು;

ಸಾರ್ವಜನಿಕ:
    /// Call visit()s on nodep
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterateConst(Ast_ಗಂಟು* nodep);
    /// Call visit()s on nodep
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterateConstNull(Ast_ಗಂಟು* nodep);
    /// Call visit()s on const nodep's children
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterateChildrenConst(Ast_ಗಂಟು* nodep);
    /// Call visit()s on nodep's children in backp() order
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterateChildrenBackwardsConst(Ast_ಗಂಟು* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterate_And_ಮುಂದಿನ_Const_Null(Ast_ಗಂಟು* nodep);

    ವಾಸ್ತವಿಕ ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) = 0;
    ವಾಸ್ತವಿಕ ~VNVisitorConst() {}
#ಅನ್ತರ್ಭಾವ "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
ಸಾರ್ವಜನಿಕ:
    /// Call visit()s on nodep
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterate(Ast_ಗಂಟು* nodep);
    /// Call visit()s on nodep
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterateNull(Ast_ಗಂಟು* nodep);
    /// Call visit()s on nodep's children
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterateChildren(Ast_ಗಂಟು* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ iterate_And_ಮುಂದಿನ_Null(Ast_ಗಂಟು* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    ಸಾಲೊಳಗೆ Ast_ಗಂಟು* iterateSubtreeReturnEdits(Ast_ಗಂಟು* nodep);

    VNDeleter& deleter() { ಹಿಂತಿರುಗಿ m_deleter; }
    ಶೂನ್ಯ ತಳ್ಳು_Deletep(Ast_ಗಂಟು* nodep) { deleter().ತಳ್ಳು_Deletep(nodep); }
    ಶೂನ್ಯ doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ class Ast_ಗಂಟು;
    ಎಣಿಕೆ RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_ಮುಂದಿನ,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_ಗಂಟು* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_ಗಂಟು* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_ಗಂಟು** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

ಸಾರ್ವಜನಿಕ:
    VNRelinker() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ relink(Ast_ಗಂಟು* newp);
    ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ;
};
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VNRelinker& rhs) {
    rhs.dump(os);
    ಹಿಂತಿರುಗಿ os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{ಸಾರ್ವಜನಿಕ : ವಾಸ್ತವಿಕ bool ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {ಹಿಂತಿರುಗಿ true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#ವ್ಯಾಖ್ಯಾನಿಸು ASTNODE_PREFETCH_NON_NULL(nodep) \
    ಮಾಡು { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } ಆಗ (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#ವ್ಯಾಖ್ಯಾನಿಸು ASTNODE_PREFETCH(nodep) \
    ಮಾಡು { \
        ಆದರೆ (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } ಆಗ (false)

class Ast_ಗಂಟು VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_ಗಂಟು* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_ಗಂಟು* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_ಗಂಟು* m_op1p = nullptr;  // Generic pointer 1
    Ast_ಗಂಟು* m_op2p = nullptr;  // Generic pointer 2
    Ast_ಗಂಟು* m_op3p = nullptr;  // Generic pointer 3
    Ast_ಗಂಟು* m_op4p = nullptr;  // Generic pointer 4
    Ast_ಗಂಟು** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    ಸ್ಥಿರಾಂಕ VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    ರಚನೆ {
        bool did_ಅಗಲ : 1;  // Did V3Width computation
        bool doing_ಅಗಲ : 1;  // Inside V3Width
        bool protect : 1;  // Protect name if protection is on
        // Space for more flags here (there must be 8 bits in total)
        uint8_t ಬಳಸದ : 5;
    } m_flags;  // Attribute flags

    // State variable used by V3Broken for consistency checking. The top bit of this is byte is a
    // flag, representing V3Broken is currently proceeding under this node. The bottom 7 bits are
    // a generation number. This is hot when --debug-checks so we access as a whole to avoid bit
    // field masking resulting in unnecessary read-modify-write ops.
    uint8_t m_brokenState = 0;

    ಪೂರ್ಣಾಂಕ m_cloneCnt = 0;  // Sequence number for when last clone was made

#ಆದರೆ defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(ಗಾತ್ರ(m_type) + ಗಾತ್ರ(m_flags) + ಗಾತ್ರ(m_brokenState) + ಗಾತ್ರ(m_cloneCnt)
                      <= ಗಾತ್ರ(ಶೂನ್ಯ*),
                  "packing requires padding");
#ಆದರೆ_ಅಂತ್ಯ

    Ast_ಗಂಟು_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_ಗಂಟು* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_ಸಾಲು* m_fileline;  // Where it was declared
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_ಎಣಿಕೆ;  // When it was last edited
#ಆದರೆ_ಅಂತ್ಯ
    ಸ್ಥಿರ uint64_t s_editCntGbl;  // Global edit counter
    ಸ್ಥಿರ uint64_t s_edit_Cnt_ಕೊನೆಯ;  // Last committed value of global edit counter

    Ast_ಗಂಟು* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_cloneCntGbl;  // Count of which userp is set

    // This member ordering both allows 64 bit alignment and puts associated data together
    // (under VL_USER_TYPE_CHECKS a VNUser is larger than 64 bits, so this packing no
    // longer holds; that build trades node size for catching int/pointer confusion)
    VNUser m_user1u{nullptr};  // Contains any information the user iteration routine wants
    uint32_t m_user1Cnt = 0;  // Mark of when userp was set
    uint32_t m_user2Cnt = 0;  // Mark of when userp was set
    VNUser m_user2u{nullptr};  // Contains any information the user iteration routine wants
    VNUser m_user3u{nullptr};  // Contains any information the user iteration routine wants
    uint32_t m_user3Cnt = 0;  // Mark of when userp was set
    uint32_t m_user4Cnt = 0;  // Mark of when userp was set
    VNUser m_user4u{nullptr};  // Contains any information the user iteration routine wants

    // METHODS
    ಶೂನ್ಯ op1p(Ast_ಗಂಟು* nodep) {
        m_op1p = nodep;
        ಆದರೆ (nodep) nodep->m_backp = this;
    }
    ಶೂನ್ಯ op2p(Ast_ಗಂಟು* nodep) {
        m_op2p = nodep;
        ಆದರೆ (nodep) nodep->m_backp = this;
    }
    ಶೂನ್ಯ op3p(Ast_ಗಂಟು* nodep) {
        m_op3p = nodep;
        ಆದರೆ (nodep) nodep->m_backp = this;
    }
    ಶೂನ್ಯ op4p(Ast_ಗಂಟು* nodep) {
        m_op4p = nodep;
        ಆದರೆ (nodep) nodep->m_backp = this;
    }

ಖಾಸಗಿ:
    Ast_ಗಂಟು* clone_ವೃಕ್ಷ_Iter(bool need_ಶುದ್ಧ);
    Ast_ಗಂಟು* clone_ವೃಕ್ಷ_Iter_List(bool need_ಶುದ್ಧ);
    ಶೂನ್ಯ check_ವೃಕ್ಷ_Iter(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* prevBackp) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    bool gate_ವೃಕ್ಷ_Iter() ಸ್ಥಿರಾಂಕ;
    ಸ್ಥಿರ bool same_ವೃಕ್ಷ_Iter(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node1p, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node2p, bool ign_ಮುಂದಿನ,
                             bool gateOnly);
    ಶೂನ್ಯ delete_ವೃಕ್ಷ_Iter();
    ಶೂನ್ಯ delete_ಗಂಟು();
    string instanceStr() ಸ್ಥಿರಾಂಕ;

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ purityCheck();
    ಸ್ಥಿರ ಶೂನ್ಯ relinkOneLink(Ast_ಗಂಟು*& pointpr, Ast_ಗಂಟು* newp);
    // cppcheck-suppress functionConst
    ಸ್ಥಿರ ಶೂನ್ಯ debug_ವೃಕ್ಷ_Change(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪೂರ್ವಪ್ರತ್ಯಯ, ಪೂರ್ಣಾಂಕ lineno, bool ಮುಂದಿನ);

ಸಂರಕ್ಷಿತ:
    // CONSTRUCTORS
    Ast_ಗಂಟು(VNType t, File_ಸಾಲು* fl);
    ವಾಸ್ತವಿಕ ~Ast_ಗಂಟು() = ಪೂರ್ವನಿಯೋಜಿತ;  // Use 'deleteTree' instead
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು* clone() = 0;  // Generally, cloneTree is what you want instead
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ cloneRelink() { cloneRelinkGen(); }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ cloneRelinkGen() {};  // Overrides generated by 'astgen'
    ಶೂನ್ಯ clone_Relink_ವೃಕ್ಷ();

    // METHODS
    ಶೂನ್ಯ ಗಣ_Op1p(Ast_ಗಂಟು* newp);  // Set non-list-type op1 to non-list element
    ಶೂನ್ಯ ಗಣ_Op2p(Ast_ಗಂಟು* newp);  // Set non-list-type op2 to non-list element
    ಶೂನ್ಯ ಗಣ_Op3p(Ast_ಗಂಟು* newp);  // Set non-list-type op3 to non-list element
    ಶೂನ್ಯ ಗಣ_Op4p(Ast_ಗಂಟು* newp);  // Set non-list-type op4 to non-list element

    ಶೂನ್ಯ ಕೂಡಿಸು_Op1p(Ast_ಗಂಟು* newp);  // Append newp to end of op1
    ಶೂನ್ಯ ಕೂಡಿಸು_Op2p(Ast_ಗಂಟು* newp);  // Append newp to end of op2
    ಶೂನ್ಯ ಕೂಡಿಸು_Op3p(Ast_ಗಂಟು* newp);  // Append newp to end of op3
    ಶೂನ್ಯ ಕೂಡಿಸು_Op4p(Ast_ಗಂಟು* newp);  // Append newp to end of op4

    ಶೂನ್ಯ prependOp1p(Ast_ಗಂಟು* newp);  // Prepend newp to front of op1
    ಶೂನ್ಯ prependOp2p(Ast_ಗಂಟು* newp);  // Prepend newp to front of op2
    ಶೂನ್ಯ prependOp3p(Ast_ಗಂಟು* newp);  // Prepend newp to front of op3
    ಶೂನ್ಯ prependOp4p(Ast_ಗಂಟು* newp);  // Prepend newp to front of op4

    // clang-format off
    ಶೂನ್ಯ ಗಣ_NOp1p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) ಗಣ_Op1p(newp); }
    ಶೂನ್ಯ ಗಣ_NOp2p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) ಗಣ_Op2p(newp); }
    ಶೂನ್ಯ ಗಣ_NOp3p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) ಗಣ_Op3p(newp); }
    ಶೂನ್ಯ ಗಣ_NOp4p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) ಗಣ_Op4p(newp); }

    ಶೂನ್ಯ ಕೂಡಿಸು_NOp1p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) ಕೂಡಿಸು_Op1p(newp); }
    ಶೂನ್ಯ ಕೂಡಿಸು_NOp2p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) ಕೂಡಿಸು_Op2p(newp); }
    ಶೂನ್ಯ ಕೂಡಿಸು_NOp3p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) ಕೂಡಿಸು_Op3p(newp); }
    ಶೂನ್ಯ ಕೂಡಿಸು_NOp4p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) ಕೂಡಿಸು_Op4p(newp); }

    ಶೂನ್ಯ prependNOp1p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) prependOp1p(newp); }
    ಶೂನ್ಯ prependNOp2p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) prependOp2p(newp); }
    ಶೂನ್ಯ prependNOp3p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) prependOp3p(newp); }
    ಶೂನ್ಯ prependNOp4p(Ast_ಗಂಟು* newp) { ಆದರೆ (newp) prependOp4p(newp); }
    // clang-format on

    ಶೂನ್ಯ clonep(Ast_ಗಂಟು* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ clone_Clear_ವೃಕ್ಷ() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    ವಾಸ್ತವಿಕ bool same_ಗಂಟು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    ವಾಸ್ತವಿಕ bool wouldBreakGen(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldp,
                               ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp) ಸ್ಥಿರಾಂಕ
        = 0;  // Generated by 'astgen'

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    VNType type() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_type; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* typeName() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ type().ascii(); }  // See also prettyTypeName
    Ast_ಗಂಟು* nextp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_nextp; }
    Ast_ಗಂಟು* backp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_backp; }
    Ast_ಗಂಟು* abovep() ಸ್ಥಿರಾಂಕ;  // Get parent node above, only for list head and tail
    Ast_ಗಂಟು* aboveLoopp() ಸ್ಥಿರಾಂಕ;  // Get parent node above, may have performance issues as loops
    Ast_ಗಂಟು* lastp() ಸ್ಥಿರಾಂಕ {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        ಹಿಂತಿರುಗಿ m_headtailp;
    }
    Ast_ಗಂಟು* prevp() ಸ್ಥಿರಾಂಕ {  // Previous node in list, nullptr for head of list
        ಆದರೆ (m_backp->m_nextp != this) ಹಿಂತಿರುಗಿ nullptr;
        ಹಿಂತಿರುಗಿ m_backp;
    }
    Ast_ಗಂಟು* op1p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_op1p; }
    Ast_ಗಂಟು* op2p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_op2p; }
    Ast_ಗಂಟು* op3p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_op3p; }
    Ast_ಗಂಟು* op4p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_op4p; }
    Ast_ಗಂಟು_DType* dtypep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_dtypep; }
    Ast_ಗಂಟು* clonep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_ಗಂಟು* ಮೊದಲ_Abovep() ಸ್ಥಿರಾಂಕ {  // Returns nullptr when second or later in list
        ಹಿಂತಿರುಗಿ ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_brokenState; }
    ಶೂನ್ಯ brokenState(uint8_t ಮೌಲ್ಯ) { m_brokenState = ಮೌಲ್ಯ; }

    // Used by AstNode::broken()
    bool brokeExists() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ foreachLink(std::function<ಶೂನ್ಯ(Ast_ಗಂಟು** linkpp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* namep)> f) = 0;

#ಆದರೆ defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    ಸ್ಥಿರ ಶೂನ್ಯ* ಕ್ರಿಯಾಕಾರಕ new(ಗಾತ್ರ_t ಗಾತ್ರ_2);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕ್ರಿಯಾಕಾರಕ delete(ಶೂನ್ಯ* obj, ಗಾತ್ರ_t ಗಾತ್ರ_2);
#ಆದರೆ_ಅಂತ್ಯ

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_BRANCH = 4;  // Branch
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_CALL = INSTR_ಎಣಿಕೆ_BRANCH + 10;  // Subroutine call
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_LD = 2;  // Load memory
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_INT_MUL = 3;  // Integer multiply
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_INT_DIV = 10;  // Integer divide
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_DBL = 8;  // Convert or do float ops
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_DBL_DIV = 40;  // Double divide
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_DBL_TRIG = 200;  // Double trigonometric ops
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_STR = 100;  // String ops
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_ಸಮಯ = INSTR_ಎಣಿಕೆ_CALL + 5;  // Determine simulation time
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_PLI = 20;  // PLI routines
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INSTR_ಎಣಿಕೆ_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    ವಾಸ್ತವಿಕ string name() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ ""; }
    ವಾಸ್ತವಿಕ string origName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    string prettyDehashOrigOrName() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ prettyName(VName::dehash(origName().ಖಾಲಿಗಳು() ? name() : origName()));
    }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ ಟ್ಯಾಗ್(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {}
    ವಾಸ್ತವಿಕ string ಟ್ಯಾಗ್() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    ವಾಸ್ತವಿಕ uint32_t decl_Token_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ 0; }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ decl_Token_ಅಂಕೆ_ಗಣ_Min(uint32_t token_ಅಂಕೆ) {}
    ವಾಸ್ತವಿಕ string verilogKwd() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ""; }
    string nameProtect() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() ಸ್ಥಿರಾಂಕ;  // origName with --protect-id applied
    string shortName() ಸ್ಥಿರಾಂಕ;  // Name with __PVT__ removed for concatenating scopes
    ಸ್ಥಿರ string dedotName(ಸ್ಥಿರಾಂಕ string& namein);  // Name with dots removed
    ಸ್ಥಿರ string nameNoArray(ಸ್ಥಿರಾಂಕ string& namein);  // Name with any array index removed
    ಸ್ಥಿರ string prettyName(ಸ್ಥಿರಾಂಕ string& namein) VL_ಶುದ್ಧ;  // Name for printing out to the user
    ಸ್ಥಿರ string vpiName(ಸ್ಥಿರಾಂಕ string& namein);  // Name for vpi access
    ಸ್ಥಿರ string prettyNameQ(ಸ್ಥಿರಾಂಕ string& namein) {  // Quoted pretty name (for errors)
        ಹಿಂತಿರುಗಿ "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    ಸ್ಥಿರ string encodeName(ಸ್ಥಿರಾಂಕ string& namein);
    ಸ್ಥಿರ string encode_ಸಂಖ್ಯೆ(int64_t ಅಂಕೆ);  // Encode number into internal C representation
    ಸ್ಥಿರ string vcdName(ಸ್ಥಿರಾಂಕ string& namein);  // Name for printing out to vcd files
    string prettyName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    ವಾಸ್ತವಿಕ string pretty_Name_ಸಂದೇಶ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ prettyName(); }
    string prettyNameQ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "'"s + pretty_Name_ಸಂದೇಶ() + "'"; }  // Quoted, for messages
    string verilogName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ಸ್ಥಿರಾಂಕ;
    ವಾಸ್ತವಿಕ string prettyOperatorName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "operator " + prettyTypeName(); }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_fileline; }
    ಶೂನ್ಯ fileline(File_ಸಾಲು* fl) { m_fileline = fl; }
    ಸಾಲೊಳಗೆ bool width1() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ ಅಗಲ_Instrs() ಸ್ಥಿರಾಂಕ;
    bool did_ಅಗಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_flags.did_ಅಗಲ; }
    ಶೂನ್ಯ did_ಅಗಲ(bool flag) { m_flags.did_ಅಗಲ = flag; }
    bool did_ಅಗಲ_And_ಗಣ() {
        ಆದರೆ (did_ಅಗಲ()) ಹಿಂತಿರುಗಿ true;
        did_ಅಗಲ(true);
        ಹಿಂತಿರುಗಿ false;
    }
    bool doing_ಅಗಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_flags.doing_ಅಗಲ; }
    ಶೂನ್ಯ doing_ಅಗಲ(bool flag) { m_flags.doing_ಅಗಲ = flag; }
    bool protect() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_flags.protect; }
    ಶೂನ್ಯ protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ ಅಗಲ() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಪೂರ್ಣಾಂಕ ಅಗಲ_Min_V() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::VERILOG_ಅಗಲ ? ಅಗಲ_Min() : ಅಗಲ();
    }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಪದಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VL_ಪದಗಳು_I(ಅಗಲ()); }
    bool isQuad() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ (ಅಗಲ() > VL_IDATASIZE && ಅಗಲ() <= VL_QUADSIZE); }
    bool isWide() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ (ಅಗಲ() > VL_QUADSIZE); }
    ಸಾಲೊಳಗೆ bool isCHandle() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ bool isDouble() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ bool isSigned() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ bool isString() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸಾಲೊಳಗೆ bool isEvent() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        ಹಿಂತಿರುಗಿ ((m_user1Cnt == VNUser1_ಒಳ_Use::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_ಗಂಟು* user1p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ user1u().ಗಮ್ಯ_Nodep(); }
    ಶೂನ್ಯ user1u(ಸ್ಥಿರಾಂಕ VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1_ಒಳ_Use::s_userCntGbl; }
    ಶೂನ್ಯ user1p(ಶೂನ್ಯ* userp) { user1u(VNUser{userp}); }
    ಶೂನ್ಯ user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ user1u().ಗಮ್ಯ_UQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ಸ್ಥಿರಾಂಕ uint64_t v = user1(); user1(v + val); ಹಿಂತಿರುಗಿ v; }
    uint64_t user1Or(uint64_t val) { ಸ್ಥಿರಾಂಕ uint64_t v = user1(); user1(v | val); ಹಿಂತಿರುಗಿ v; }
    uint64_t user1_ಗಣ_Once() { ಸ್ಥಿರಾಂಕ uint64_t v = user1(); ಆದರೆ (!v) user1(1); ಹಿಂತಿರುಗಿ v; }  // Better for cache than user1Inc()
    ಸ್ಥಿರ ಶೂನ್ಯ user1_Clear_ವೃಕ್ಷ() { VNUser1_ಒಳ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        ಹಿಂತಿರುಗಿ ((m_user2Cnt == VNUser2_ಒಳ_Use::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_ಗಂಟು* user2p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ user2u().ಗಮ್ಯ_Nodep(); }
    ಶೂನ್ಯ user2u(ಸ್ಥಿರಾಂಕ VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2_ಒಳ_Use::s_userCntGbl; }
    ಶೂನ್ಯ user2p(ಶೂನ್ಯ* userp) { user2u(VNUser{userp}); }
    ಶೂನ್ಯ user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ user2u().ಗಮ್ಯ_UQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ಸ್ಥಿರಾಂಕ uint64_t v = user2(); user2(v + val); ಹಿಂತಿರುಗಿ v; }
    uint64_t user2Or(uint64_t val) { ಸ್ಥಿರಾಂಕ uint64_t v = user2(); user2(v | val); ಹಿಂತಿರುಗಿ v; }
    uint64_t user2_ಗಣ_Once() { ಸ್ಥಿರಾಂಕ uint64_t v = user2(); ಆದರೆ (!v) user2(1); ಹಿಂತಿರುಗಿ v; }  // Better for cache than user2Inc()
    ಸ್ಥಿರ ಶೂನ್ಯ user2_Clear_ವೃಕ್ಷ() { VNUser2_ಒಳ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        ಹಿಂತಿರುಗಿ ((m_user3Cnt == VNUser3_ಒಳ_Use::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_ಗಂಟು* user3p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ user3u().ಗಮ್ಯ_Nodep(); }
    ಶೂನ್ಯ user3u(ಸ್ಥಿರಾಂಕ VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3_ಒಳ_Use::s_userCntGbl; }
    ಶೂನ್ಯ user3p(ಶೂನ್ಯ* userp) { user3u(VNUser{userp}); }
    ಶೂನ್ಯ user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ user3u().ಗಮ್ಯ_UQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ಸ್ಥಿರಾಂಕ uint64_t v = user3(); user3(v + val); ಹಿಂತಿರುಗಿ v; }
    uint64_t user3Or(uint64_t val) { ಸ್ಥಿರಾಂಕ uint64_t v = user3(); user3(v | val); ಹಿಂತಿರುಗಿ v; }
    uint64_t user3_ಗಣ_Once() { ಸ್ಥಿರಾಂಕ uint64_t v = user3(); ಆದರೆ (!v) user3(1); ಹಿಂತಿರುಗಿ v; }  // Better for cache than user3Inc()
    ಸ್ಥಿರ ಶೂನ್ಯ user3_Clear_ವೃಕ್ಷ() { VNUser3_ಒಳ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        ಹಿಂತಿರುಗಿ ((m_user4Cnt == VNUser4_ಒಳ_Use::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_ಗಂಟು* user4p() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ user4u().ಗಮ್ಯ_Nodep(); }
    ಶೂನ್ಯ user4u(ಸ್ಥಿರಾಂಕ VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4_ಒಳ_Use::s_userCntGbl; }
    ಶೂನ್ಯ user4p(ಶೂನ್ಯ* userp) { user4u(VNUser{userp}); }
    ಶೂನ್ಯ user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ user4u().ಗಮ್ಯ_UQuad(); }
    uint64_t user4Or(uint64_t val) { ಸ್ಥಿರಾಂಕ uint64_t v = user4(); user4(v | val); ಹಿಂತಿರುಗಿ v; }
    uint64_t user4Inc(uint64_t val = 1) { ಸ್ಥಿರಾಂಕ uint64_t v = user4(); user4(v + val); ಹಿಂತಿರುಗಿ v; }
    uint64_t user4_ಗಣ_Once() { ಸ್ಥಿರಾಂಕ uint64_t v = user4(); ಆದರೆ (!v) user4(1); ಹಿಂತಿರುಗಿ v; }  // Better for cache than user4Inc()
    ಸ್ಥಿರ ಶೂನ್ಯ user4_Clear_ವೃಕ್ಷ() { VNUser4_ಒಳ_Use::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
    uint64_t edit_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_edit_ಎಣಿಕೆ; }
    ಶೂನ್ಯ edit_ಎಣಿಕೆ_Inc() {
        m_edit_ಎಣಿಕೆ = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_ವೃಕ್ಷ();  // Any edit clears all caching
    }
#ಇಲ್ಲದಿದ್ದರೆ
    ಶೂನ್ಯ edit_ಎಣಿಕೆ_Inc() { ++s_editCntGbl; }
#ಆದರೆ_ಅಂತ್ಯ
    ಸ್ಥಿರ uint64_t edit_ಎಣಿಕೆ_ಕೊನೆಯ() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ s_edit_Cnt_ಕೊನೆಯ; }
    ಸ್ಥಿರ uint64_t edit_ಎಣಿಕೆ_Gbl() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ s_editCntGbl; }
    ಸ್ಥಿರ ಶೂನ್ಯ edit_ಎಣಿಕೆ_ಗಣ_ಕೊನೆಯ() { s_edit_Cnt_ಕೊನೆಯ = edit_ಎಣಿಕೆ_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_ಸರತಿ_ತಳ್ಳು_Self_Stmt();
    ಸಾಲೊಳಗೆ bool is_Class_Handle_ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool isNull() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool isZero() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool isOne() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool isNeqZero() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool isAllOnes() ಸ್ಥಿರಾಂಕ;
    ಸಾಲೊಳಗೆ bool isAllOnesV() ಸ್ಥಿರಾಂಕ;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    ಶೂನ್ಯ dtypep(Ast_ಗಂಟು_DType* nodep) {
        ಆದರೆ (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_ಎಣಿಕೆ_Inc();
        }
    }
    ಶೂನ್ಯ dtypeFrom(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromp) {
        ಆದರೆ (fromp) dtypep(fromp->dtypep());
    }
    ಶೂನ್ಯ dtypeChgSigned(bool flag = true);
    ಶೂನ್ಯ dtype_Chg_ಅಗಲ(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min);
    ಶೂನ್ಯ dtype_Chg_ಅಗಲ_Signed(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric);
    ಶೂನ್ಯ dtype_ಗಣ_ಬಿಟ್_Unsized(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) {
        dtypep(ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಅಗಲ, ಅಗಲ_Min, numeric));
    }
    ಶೂನ್ಯ dtype_ಗಣ_ಬಿಟ್_Sized(ಪೂರ್ಣಾಂಕ ಅಗಲ, VSigning numeric) {
        dtypep(ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಅಗಲ, ಅಗಲ, numeric));  // Since sized, widthMin is width
    }
    ಶೂನ್ಯ dtype_ಗಣ_Logic_Unsized(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) {
        dtypep(ಕಂಡುಹಿಡಿ_Logic_DType(ಅಗಲ, ಅಗಲ_Min, numeric));
    }
    ಶೂನ್ಯ dtype_ಗಣ_Logic_Sized(ಪೂರ್ಣಾಂಕ ಅಗಲ, VSigning numeric) {
        dtypep(ಕಂಡುಹಿಡಿ_Logic_DType(ಅಗಲ, ಅಗಲ, numeric));  // Since sized, widthMin is width
    }
    ಶೂನ್ಯ dtype_ಗಣ_Logic2_State_Unsized(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) {
        dtypep(ಕಂಡುಹಿಡಿ_Logic2_State_DType(ಅಗಲ, ಅಗಲ_Min, numeric));
    }
    ಶೂನ್ಯ dtype_ಗಣ_Logic2_State_Sized(ಪೂರ್ಣಾಂಕ ಅಗಲ, VSigning numeric) {
        dtypep(ಕಂಡುಹಿಡಿ_Logic2_State_DType(ಅಗಲ, ಅಗಲ, numeric));  // Since sized, widthMin is width
    }
    ಶೂನ್ಯ dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric, bool isFourstate) {
        dtypep(ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(ಅಗಲ, ಅಗಲ_Min, numeric, isFourstate));
    }
    ಶೂನ್ಯ dtype_ಗಣ_ಬಿಟ್_Or_Logic_Sized(ಪೂರ್ಣಾಂಕ ಅಗಲ, VSigning numeric, bool isFourstate) {
        dtypep(ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(ಅಗಲ, ಅಗಲ, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    ಶೂನ್ಯ dtype_ಗಣ_ಬಿಟ್() { dtypep(ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_Logic() { dtypep(ಕಂಡುಹಿಡಿ_Logic_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_Logic2_State() { dtypep(ಕಂಡುಹಿಡಿ_Logic2_State_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_Double() { dtypep(ಕಂಡುಹಿಡಿ_Double_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_Int() { dtypep(ಕಂಡುಹಿಡಿ_Int_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_Integer() { dtypep(ಕಂಡುಹಿಡಿ_Integer_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_Integer2_State() { dtypep(ಕಂಡುಹಿಡಿ_Integer2_State_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_String() { dtypep(ಕಂಡುಹಿಡಿ_String_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_UInt32() { dtypep(ಕಂಡುಹಿಡಿ_UInt32_DType()); }  // Twostate
    ಶೂನ್ಯ dtype_ಗಣ_UInt64() { dtypep(ಕಂಡುಹಿಡಿ_UInt64_DType()); }  // Twostate
    ಶೂನ್ಯ dtype_ಗಣ_ಸಮಯ() { dtypep(ಕಂಡುಹಿಡಿ_ಸಮಯ_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_Time2_State() { dtypep(ಕಂಡುಹಿಡಿ_Time2_State_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_ಖಾಲಿಗಳು_ಸರತಿ() { dtypep(ಕಂಡುಹಿಡಿ_ಖಾಲಿಗಳು_ಸರತಿ_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_ಹರಿವು() { dtypep(ಕಂಡುಹಿಡಿ_ಹರಿವು_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_Void() { dtypep(ಕಂಡುಹಿಡಿ_Void_DType()); }
    ಶೂನ್ಯ dtype_ಗಣ_ಬಿಟ್_Or_Logic(bool isFourstate) {
        dtypep(isFourstate ? ಕಂಡುಹಿಡಿ_Logic_DType() : ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType());
    }

    // Data type locators
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::ಬಿಟ್); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Logic_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::LOGIC); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Null_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::NULLTYPE); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Logic2_State_DType() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Double_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::DOUBLE); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Int_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::INT); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Integer_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::INTEGER); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Integer2_State_DType() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_String_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::STRING); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Signed8_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::BYTE); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_UInt32_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT32); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_UInt64_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::UINT64); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಸಮಯ_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::ಸಮಯ); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Time2_State_DType() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_CHandle_DType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd::CHANDLE); }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Constraint_Ref_DType() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಖಾಲಿಗಳು_ಸರತಿ_DType() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಸರತಿ_ಸೂಚ್ಯಂಕ_DType() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಹರಿವು_DType() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Void_DType() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Logic_DType(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Logic2_State_DType(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_DType(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric,
                                      bool isFourstate) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ isFourstate ? ಕಂಡುಹಿಡಿ_Logic_DType(ಅಗಲ, ಅಗಲ_Min, numeric)
                           : ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಅಗಲ, ಅಗಲ_Min, numeric);
    }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಬಿಟ್_Or_Logic_Range_DType(ಸ್ಥಿರಾಂಕ VNumRange& range, ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min,
                                           ಸ್ಥಿರಾಂಕ VSigning numeric,
                                           ಸ್ಥಿರಾಂಕ bool isFourstate) ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ isFourstate ? ಕಂಡುಹಿಡಿ_Logic_Range_DType(range, ಅಗಲ_Min, numeric)
                           : ಕಂಡುಹಿಡಿ_ಬಿಟ್_Range_DType(range, ಅಗಲ_Min, numeric);
    }
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Logic_Range_DType(ಸ್ಥಿರಾಂಕ VNumRange& range, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min,
                                      VSigning numeric) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_ಬಿಟ್_Range_DType(ಸ್ಥಿರಾಂಕ VNumRange& range, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min,
                                    VSigning numeric) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    Ast_ಗಂಟು_DType* ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd kwd) ಸ್ಥಿರಾಂಕ;
    ಸ್ಥಿರ AstBasicDType* ಕಂಡುಹಿಡಿ_ಸೇರಿಸು_Same_DType(AstBasicDType* nodep);

    ಸ್ಥಿರ VCastable computeCastable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಗಮ್ಯ_Dtp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* fromDtp,
                                     ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromConstp, bool checkIfaceArgCompat = false);
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* getCommonClassTypep(Ast_ಗಂಟು* nodep1, Ast_ಗಂಟು* nodep2);

    // METHODS - dump and error
    ಶೂನ್ಯ v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ VL_RELEASE(V3Error::s().m_mutex);
    ಶೂನ್ಯ v3error_ಕೊನೆ_Fatal(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex) {
        ಹಿಂತಿರುಗಿ fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ fileline()->warnContextSecondary(); }
    string warn_ಇನ್ನಷ್ಟು() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex) { ಹಿಂತಿರುಗಿ fileline()->warn_ಇನ್ನಷ್ಟು(); }
    string warnOther() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex) { ಹಿಂತಿರುಗಿ fileline()->warnOther(); }

    ವಾಸ್ತವಿಕ ಶೂನ್ಯ dump(std::ostream& str = std::cout) ಸ್ಥಿರಾಂಕ;
    ಸ್ಥಿರ ಅಕ್ಷರ* dump_ವೃಕ್ಷ_Json_Gdb(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);  // For GDB only, free()d by caller
    ಸ್ಥಿರ ಅಕ್ಷರ* dump_ವೃಕ್ಷ_Json_Gdb(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* str);  // For GDB only, free()d by caller
    ಸ್ಥಿರ ಅಕ್ಷರ* dump_ವೃಕ್ಷ_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    ಸ್ಥಿರ ಶೂನ್ಯ dumpGdb(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);  // For GDB only
    ಶೂನ್ಯ dump_Gdb_ಶೀರ್ಷಿಕೆ() ಸ್ಥಿರಾಂಕ;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    ಮಾದರಿ <typename T_ಗಂಟು_ಫಲಿತಾಂಶ, typename T_ಗಂಟು_ಮುಂದಿನ>
    ಸ್ಥಿರ T_ಗಂಟು_ಫಲಿತಾಂಶ* ಕೂಡಿಸು_ಮುಂದಿನ(T_ಗಂಟು_ಫಲಿತಾಂಶ* nodep, T_ಗಂಟು_ಮುಂದಿನ* newp) {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು_ಫಲಿತಾಂಶ>::ಮೌಲ್ಯ,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_ತಳ_ದ<T_ಗಂಟು_ಫಲಿತಾಂಶ, T_ಗಂಟು_ಮುಂದಿನ>::ಮೌಲ್ಯ,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        ಆದರೆ (!newp) ಹಿಂತಿರುಗಿ nodep;
        ಹಿಂತಿರುಗಿ static_cast<T_ಗಂಟು_ಫಲಿತಾಂಶ*>(ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, newp));
    }
    ಮಾದರಿ <typename T_ಗಂಟು_ಫಲಿತಾಂಶ, typename T_ಗಂಟು_ಮುಂದಿನ>
    ಸ್ಥಿರ T_ಗಂಟು_ಫಲಿತಾಂಶ* ಕೂಡಿಸು_ಮುಂದಿನ_Null(T_ಗಂಟು_ಫಲಿತಾಂಶ* nodep, T_ಗಂಟು_ಮುಂದಿನ* newp) {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು_ಫಲಿತಾಂಶ>::ಮೌಲ್ಯ,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_ತಳ_ದ<T_ಗಂಟು_ಫಲಿತಾಂಶ, T_ಗಂಟು_ಮುಂದಿನ>::ಮೌಲ್ಯ,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        ಆದರೆ (!newp) ಹಿಂತಿರುಗಿ nodep;
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ newp;
        ಹಿಂತಿರುಗಿ static_cast<T_ಗಂಟು_ಫಲಿತಾಂಶ*>(ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, newp));
    }
    ಸಾಲೊಳಗೆ Ast_ಗಂಟು* ಕೂಡಿಸು_ಮುಂದಿನ(Ast_ಗಂಟು* newp);
    ಶೂನ್ಯ ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(Ast_ಗಂಟು* newp);  // Insert newp at this->nextp
    ಶೂನ್ಯ ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(Ast_ಗಂಟು* newp);  // Adds at old place of this, this becomes next
    ಶೂನ್ಯ replaceWith(Ast_ಗಂಟು* newp);  // Replace current node in tree with new node
    ಶೂನ್ಯ replaceWithKeepDType(Ast_ಗಂಟು* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_ಗಂಟು* unlink_Fr_ಹಿಂಭಾಗ(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_ಗಂಟು* unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(VNRelinker* linkerp = nullptr);
    ಶೂನ್ಯ relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    ಶೂನ್ಯ clone_Relink_ಗಂಟು() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_ಗಂಟು* clone_ವೃಕ್ಷ(bool clone_ಮುಂದಿನ_Link,
                       bool need_ಶುದ್ಧ = false);  // Not const, as sets clonep() on original nodep
    Ast_ಗಂಟು* clone_ವೃಕ್ಷ_ಶುದ್ಧ(bool clone_ಮುಂದಿನ_Link) { ಹಿಂತಿರುಗಿ clone_ವೃಕ್ಷ(clone_ಮುಂದಿನ_Link, true); }
    bool gate_ವೃಕ್ಷ() { ಹಿಂತಿರುಗಿ gate_ವೃಕ್ಷ_Iter(); }  // Is tree isGateOptimizable?
    ಸಾಲೊಳಗೆ bool same_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node2p) ಸ್ಥಿರಾಂಕ;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    ಸಾಲೊಳಗೆ bool same_Gate_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node2p) ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ delete_ವೃಕ್ಷ();  // Always deletes the next link
    ಶೂನ್ಯ check_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಆದರೆ (v3Global.opt.debugCheck()) check_ವೃಕ್ಷ_Iter(backp());
    }
    ಶೂನ್ಯ checkIter() ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ dumpPtrs(std::ostream& os = std::cout) ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ dump_ವೃಕ್ಷ(std::ostream& os = std::cout, ಸ್ಥಿರಾಂಕ string& indent = "    ",
                  ಪೂರ್ಣಾಂಕ max_ಆಳ = 0) ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ dump_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ string& indent, ಪೂರ್ಣಾಂಕ max_ಆಳ = 0) ಸ್ಥಿರಾಂಕ {
        dump_ವೃಕ್ಷ(std::cout, indent, max_ಆಳ);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dump_ವೃಕ್ಷ_Gdb(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);  // For GDB only
    ಶೂನ್ಯ dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(std::ostream& os = std::cout, ಸ್ಥಿರಾಂಕ string& indent = "    ",
                         ಪೂರ್ಣಾಂಕ max_ಆಳ = 0) ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ dump_ವೃಕ್ಷ_File(ಸ್ಥಿರಾಂಕ string& filename, bool doDump = true);
    ಸ್ಥಿರ ಶೂನ್ಯ dump_ವೃಕ್ಷ_File_Gdb(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* filenamep = nullptr);
    ಶೂನ್ಯ dump_ವೃಕ್ಷ_Dot(std::ostream& os = std::cout) ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ dump_ವೃಕ್ಷ_Dot_File(ಸ್ಥಿರಾಂಕ string& filename, bool doDump = true);
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ dumpJson(std::ostream& os) ಸ್ಥಿರಾಂಕ { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    ಶೂನ್ಯ dumpJsonGen(std::ostream& os) ಸ್ಥಿರಾಂಕ {};
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ dump_ವೃಕ್ಷ_Json_Op_Gen(std::ostream& os, ಸ್ಥಿರಾಂಕ string& indent) ಸ್ಥಿರಾಂಕ {};
    ಶೂನ್ಯ dump_ವೃಕ್ಷ_Json(std::ostream& os, ಸ್ಥಿರಾಂಕ string& indent = "") ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ dump_ವೃಕ್ಷ_Json_File(ಸ್ಥಿರಾಂಕ string& filename, bool doDump = true);
    ಸ್ಥಿರ ಶೂನ್ಯ dumpJsonMetaFileGdb(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* filename);
    ಸ್ಥಿರ ಶೂನ್ಯ dumpJsonMetaFile(ಸ್ಥಿರಾಂಕ string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    ಸ್ಥಿರ std::string ಗಂಟು_Addr(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಹಿಂತಿರುಗಿ v3Global.opt.dump_ವೃಕ್ಷ_Addrids() ? v3Global.ptr_ಗಮ್ಯ_Id(nodep) : cvt_ಗಮ್ಯ_Hex(nodep);
    }

    // METHODS - static advancement
    ಸ್ಥಿರ Ast_ಗಂಟು* afterCommentp(Ast_ಗಂಟು* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        ಆಗ (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        ಹಿಂತಿರುಗಿ nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    ವಾಸ್ತವಿಕ bool isBrancher() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    // Else a AstTime etc that can't be pushed out
    ವಾಸ್ತವಿಕ bool isGateOptimizable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    ವಾಸ್ತವಿಕ bool isGateDedupable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    ವಾಸ್ತವಿಕ bool is_ಅಭಿವ್ಯಕ್ತಿ_Coverage_Eligible() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    ವಾಸ್ತವಿಕ bool isOutputter() { ಹಿಂತಿರುಗಿ false; }
    // Else a AstTime etc which output can't be predicted from input
    ವಾಸ್ತವಿಕ bool isPredictOptimizable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    ವಾಸ್ತವಿಕ bool is_ಶುದ್ಧ() { ಹಿಂತಿರುಗಿ true; }
    // Iff isPure on current node and any nextp()
    bool is_ಶುದ್ಧ_And_ಮುಂದಿನ() { ಹಿಂತಿರುಗಿ is_ಶುದ್ಧ() && (!nextp() || nextp()->is_ಶುದ್ಧ()); }
    // An event control, delay, wait, etc.
    ವಾಸ್ತವಿಕ bool isTimingControl() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    ವಾಸ್ತವಿಕ bool isUnlikely() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    // Is an IEEE system function (versus internally-generated)
    ವಾಸ್ತವಿಕ bool isSystemFunc() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    ವಾಸ್ತವಿಕ ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ 0; }
    // Iff node is identical to another node
    ವಾಸ್ತವಿಕ bool isSame(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* samep) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ type() == samep->type() && same_ಗಂಟು(samep);
    }
    // Iff has a data type; dtype() must be non-null
    ವಾಸ್ತವಿಕ bool hasDType() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ false; }
    // Iff has a non-null childDTypep(), as generic node function
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_DType* get_ಮಗು_DTypep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    ವಾಸ್ತವಿಕ Ast_ಗಂಟು_DType* getChild2DTypep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    ವಾಸ್ತವಿಕ bool maybe_Pointed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ false; }
    // Don't reclaim this node in V3Dead
    ವಾಸ್ತವಿಕ bool undead() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    ವಾಸ್ತವಿಕ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* broken() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    ವಾಸ್ತವಿಕ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* brokenGen() ಸ್ಥಿರಾಂಕ = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ accept(VNVisitorConst& v) = 0;

ಸಂರಕ್ಷಿತ:
    // All VNVisitor related functions are called as methods off the visitor
    ಮಿತ್ರ class VNVisitor;
    ಮಿತ್ರ class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    ಶೂನ್ಯ iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    ಶೂನ್ಯ iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    ಶೂನ್ಯ iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    ಶೂನ್ಯ iterate_And_ಮುಂದಿನ(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    ಶೂನ್ಯ iterate_And_ಮುಂದಿನ_Const(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_ಗಂಟು* iterateSubtreeReturnEdits(VNVisitor& v);

    ಸ್ಥಿರ ಶೂನ್ಯ dump_Json_ಅಂಕೆ(std::ostream& os, ಸ್ಥಿರಾಂಕ std::string& name, int64_t val);
    ಸ್ಥಿರ ಶೂನ್ಯ dumpJsonBoolIf(std::ostream& os, ಸ್ಥಿರಾಂಕ std::string& name, bool val);
    ಸ್ಥಿರ ಶೂನ್ಯ dumpJsonStr(std::ostream& os, ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ std::string& val);
    ಸ್ಥಿರ ಶೂನ್ಯ dumpJsonPtr(std::ostream& os, ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ valp);

ಸಂರಕ್ಷಿತ:
    ಶೂನ್ಯ iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    ಮಾದರಿ <typename T>
    ಸಾಲೊಳಗೆ ಸ್ಥಿರ bool ಖಾಸಗಿ_Type_Test(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep);

    // For internal use only.
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_DeclType>
    constexpr ಸ್ಥಿರ bool uselessCast() VL_ಶುದ್ಧ {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_ಸೂಚಕ<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        ಹಿಂತಿರುಗಿ std::is_ತಳ_ದ<T_ಗುರಿ_Type, NonCV>::ಮೌಲ್ಯ;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_DeclType>
    constexpr ಸ್ಥಿರ bool impossibleCast() VL_ಶುದ್ಧ {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_ಸೂಚಕ<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        ಹಿಂತಿರುಗಿ !std::is_ತಳ_ದ<NonCV, T_ಗುರಿ_Type>::ಮೌಲ್ಯ;
    }  // LCOV_EXCL_STOP

ಸಾರ್ವಜನಿಕ:
    // For use via the VN_IS macro only, or in templated code
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_ಗಂಟು>
    ಸ್ಥಿರ bool is(ಸ್ಥಿರಾಂಕ T_ಗಂಟು* nodep) VL_MT_ಸುರಕ್ಷಿತ {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        ಹಿಂತಿರುಗಿ nodep && ಖಾಸಗಿ_Type_Test<T_ಗುರಿ_Type>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_ಗಂಟು>
    ಸ್ಥಿರ T_ಗುರಿ_Type* cast(T_ಗಂಟು* nodep) VL_MT_ಸುರಕ್ಷಿತ {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        ಹಿಂತಿರುಗಿ nodep && ಖಾಸಗಿ_Type_Test<T_ಗುರಿ_Type>(nodep)
                   ? reinterpret_cast<T_ಗುರಿ_Type*>(nodep)
                   : nullptr;
    }
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_ಗಂಟು>
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ T_ಗುರಿ_Type* cast(ಸ್ಥಿರಾಂಕ T_ಗಂಟು* nodep) VL_MT_ಸುರಕ್ಷಿತ {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        ಹಿಂತಿರುಗಿ nodep && ಖಾಸಗಿ_Type_Test<T_ಗುರಿ_Type>(nodep)
                   ? reinterpret_cast<ಸ್ಥಿರಾಂಕ T_ಗುರಿ_Type*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_ಗಂಟು>
    ಸ್ಥಿರ T_ಗುರಿ_Type* as(T_ಗಂಟು* nodep) VL_ಶುದ್ಧ {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || ಖಾಸಗಿ_Type_Test<T_ಗುರಿ_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        ಹಿಂತಿರುಗಿ unsafe_ಖಾಸಗಿ_As<T_ಗುರಿ_Type, T_ಗಂಟು>(nodep);
    }
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_ಗಂಟು>
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ T_ಗುರಿ_Type* as(ಸ್ಥಿರಾಂಕ T_ಗಂಟು* nodep) VL_ಶುದ್ಧ {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || ಖಾಸಗಿ_Type_Test<T_ಗುರಿ_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        ಹಿಂತಿರುಗಿ unsafe_ಖಾಸಗಿ_As<T_ಗುರಿ_Type, T_ಗಂಟು>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_ಗಂಟು>
    ಸ್ಥಿರ T_ಗುರಿ_Type* unsafe_ಖಾಸಗಿ_As(T_ಗಂಟು* nodep) VL_ಶುದ್ಧ {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        ಹಿಂತಿರುಗಿ reinterpret_cast<T_ಗುರಿ_Type*>(nodep);
    }
    ಮಾದರಿ <typename T_ಗುರಿ_Type, typename T_ಗಂಟು>
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ T_ಗುರಿ_Type* unsafe_ಖಾಸಗಿ_As(ಸ್ಥಿರಾಂಕ T_ಗಂಟು* nodep) VL_ಶುದ್ಧ {
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ಗುರಿ_Type, T_ಗಂಟು>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        ಹಿಂತಿರುಗಿ reinterpret_cast<ಸ್ಥಿರಾಂಕ T_ಗುರಿ_Type*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ಮಾದರಿ <typename T_ಗಂಟು>
    ಸ್ಥಿರ bool mayBeUnder(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        static_assert(!std::is_const<T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        ಹಿಂತಿರುಗಿ true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ಮಾದರಿ <typename T_ಗಂಟು>
    ಸ್ಥಿರ constexpr bool is_ಎಲೆ() {
        static_assert(!std::is_const<T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        ಹಿಂತಿರುಗಿ false;
    }

ಖಾಸಗಿ:
    // Using std::conditional for const correctness in the public 'foreach' functions
    ಮಾದರಿ <typename T_Arg>
    using Const_Correct_Ast_ಗಂಟು =
        typename std::conditional<std::is_const<T_Arg>::ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು, Ast_ಗಂಟು>::type;

    ಮಾದರಿ <typename T_Arg, typename T_Callable>
    ಸಾಲೊಳಗೆ ಸ್ಥಿರ ಶೂನ್ಯ foreachImpl(Const_Correct_Ast_ಗಂಟು<T_Arg>* nodep, ಸ್ಥಿರಾಂಕ T_Callable& f,
                                   bool ಭೇಟಿ_ಮುಂದಿನ);

    ಮಾದರಿ <typename T_Arg, bool N_Default, typename T_Callable>
    ಸಾಲೊಳಗೆ ಸ್ಥಿರ bool predicateImpl(Const_Correct_Ast_ಗಂಟು<T_Arg>* nodep, ಸ್ಥಿರಾಂಕ T_Callable& p,
                                     bool ಭೇಟಿ_ಮುಂದಿನ);

ಸಾರ್ವಜನಿಕ:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    ಮಾದರಿ <typename T_Callable>
    ಶೂನ್ಯ foreach(T_Callable&& f) {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_ಗಂಟು*>::ಮೌಲ್ಯ
                          && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_ಗಂಟು>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ಮಾದರಿ <typename T_Callable>
    ಶೂನ್ಯ foreach(T_Callable&& f) ಸ್ಥಿರಾಂಕ {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ಸ್ಥಿರಾಂಕ T_ಗಂಟು*>::ಮೌಲ್ಯ
                && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ಸ್ಥಿರಾಂಕ T_ಗಂಟು>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    ಮಾದರಿ <typename T_Callable>
    ಶೂನ್ಯ foreach_And_ಮುಂದಿನ(T_Callable&& f) {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_ಗಂಟು*>::ಮೌಲ್ಯ
                          && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_ಗಂಟು>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ಮಾದರಿ <typename T_Callable>
    ಶೂನ್ಯ foreach_And_ಮುಂದಿನ(T_Callable&& f) ಸ್ಥಿರಾಂಕ {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ಸ್ಥಿರಾಂಕ T_ಗಂಟು*>::ಮೌಲ್ಯ
                && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ಸ್ಥಿರಾಂಕ T_ಗಂಟು>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    ಮಾದರಿ <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_ಗಂಟು*>::ಮೌಲ್ಯ
                          && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ಹಿಂತಿರುಗಿ predicateImpl<T_ಗಂಟು, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ಮಾದರಿ <typename T_Callable>
    bool exists(T_Callable&& p) ಸ್ಥಿರಾಂಕ {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ಸ್ಥಿರಾಂಕ T_ಗಂಟು*>::ಮೌಲ್ಯ
                          && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ಹಿಂತಿರುಗಿ predicateImpl<ಸ್ಥಿರಾಂಕ T_ಗಂಟು, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    ಮಾದರಿ <typename T_Callable>
    bool exists_And_ಮುಂದಿನ(T_Callable&& p) {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ಸ್ಥಿರಾಂಕ T_ಗಂಟು*>::ಮೌಲ್ಯ
                          && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ಹಿಂತಿರುಗಿ predicateImpl<T_ಗಂಟು, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ಮಾದರಿ <typename T_Callable>
    bool exists_And_ಮುಂದಿನ(T_Callable&& p) ಸ್ಥಿರಾಂಕ {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ಸ್ಥಿರಾಂಕ T_ಗಂಟು*>::ಮೌಲ್ಯ
                          && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ಹಿಂತಿರುಗಿ predicateImpl<ಸ್ಥಿರಾಂಕ T_ಗಂಟು, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    ಮಾದರಿ <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_ಗಂಟು*>::ಮೌಲ್ಯ
                          && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ಹಿಂತಿರುಗಿ predicateImpl<T_ಗಂಟು, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ಮಾದರಿ <typename T_Callable>
    bool forall(T_Callable&& p) ಸ್ಥಿರಾಂಕ {
        using T_ಗಂಟು = typename Function_Arg_No_ಸೂಚಕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ಸ್ಥಿರಾಂಕ T_ಗಂಟು*>::ಮೌಲ್ಯ
                          && std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ಹಿಂತಿರುಗಿ predicateImpl<ಸ್ಥಿರಾಂಕ T_ಗಂಟು, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    ಪೂರ್ಣಾಂಕ ಗಂಟು_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ {
        // TODO: this should really return size_t, but need to fix use sites
        ಪೂರ್ಣಾಂಕ ಎಣಿಕೆ_2 = 0;
        this->foreach([&ಎಣಿಕೆ_2](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) { ++ಎಣಿಕೆ_2; });
        ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(ಪೂರ್ಣಾಂಕ ಎಲ್ಲೆ) ಸ್ಥಿರಾಂಕ {
        ಪೂರ್ಣಾಂಕ ಎಣಿಕೆ_2 = 0;
        ಹಿಂತಿರುಗಿ this->exists([&ಎಣಿಕೆ_2, ಎಲ್ಲೆ](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*) { ಹಿಂತಿರುಗಿ ++ಎಣಿಕೆ_2 > ಎಲ್ಲೆ; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
ಮಾದರಿ <>
Ast_ಗಂಟು* Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp);

// Inline method implementations
Ast_ಗಂಟು* Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(Ast_ಗಂಟು* newp) { ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_ಮುಂದಿನ(this, newp); }

// Specializations of privateTypeTest
#ಅನ್ತರ್ಭಾವ "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
ಮಾದರಿ <>
ಸಾಲೊಳಗೆ bool Ast_ಗಂಟು::mayBeUnder<AstCell>(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಹಿಂತಿರುಗಿ !VN_IS(nodep, ಗಂಟು_Stmt) && !VN_IS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
}
ಮಾದರಿ <>
ಸಾಲೊಳಗೆ bool Ast_ಗಂಟು::mayBeUnder<Ast_ಗಂಟು_ನಿಯೋಜಿಸು>(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಹಿಂತಿರುಗಿ !VN_IS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
}
ಮಾದರಿ <>
ಸಾಲೊಳಗೆ bool Ast_ಗಂಟು::mayBeUnder<AstVarScope>(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಆದರೆ (VN_IS(nodep, VarScope)) ಹಿಂತಿರುಗಿ false;  // Should not nest
    ಆದರೆ (VN_IS(nodep, Var)) ಹಿಂತಿರುಗಿ false;
    ಆದರೆ (VN_IS(nodep, ಸಕ್ರಿಯ)) ಹಿಂತಿರುಗಿ false;
    ಆದರೆ (VN_IS(nodep, ಗಂಟು_Stmt)) ಹಿಂತಿರುಗಿ false;
    ಆದರೆ (VN_IS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) ಹಿಂತಿರುಗಿ false;
    ಹಿಂತಿರುಗಿ true;
}
ಮಾದರಿ <>
ಸಾಲೊಳಗೆ bool Ast_ಗಂಟು::mayBeUnder<Ast_Exec_ನಕ್ಷೆ>(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಆದರೆ (VN_IS(nodep, Exec_ನಕ್ಷೆ)) ಹಿಂತಿರುಗಿ false;  // Should not nest
    ಆದರೆ (VN_IS(nodep, ಗಂಟು_Stmt)) ಹಿಂತಿರುಗಿ false;  // Should be directly under CFunc
    ಹಿಂತಿರುಗಿ true;
}
ಮಾದರಿ <>
ಸಾಲೊಳಗೆ bool Ast_ಗಂಟು::mayBeUnder<Ast_ಸಕ್ರಿಯ>(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಹಿಂತಿರುಗಿ !VN_IS(nodep, ಸಕ್ರಿಯ);  // AstActives do not nest
}
ಮಾದರಿ <>
ಸಾಲೊಳಗೆ bool Ast_ಗಂಟು::mayBeUnder<AstScope>(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಹಿಂತಿರುಗಿ !VN_IS(nodep, Scope);  // AstScopes do not nest
}
ಮಾದರಿ <>
ಸಾಲೊಳಗೆ bool Ast_ಗಂಟು::mayBeUnder<Ast_Sen_ವೃಕ್ಷ>(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಹಿಂತಿರುಗಿ !VN_IS(nodep, Sen_ವೃಕ್ಷ);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
ಮಾದರಿ <>
constexpr bool Ast_ಗಂಟು::is_ಎಲೆ<Ast_ಗಂಟು_Var_Ref>() {
    ಹಿಂತಿರುಗಿ true;
}
ಮಾದರಿ <>
constexpr bool Ast_ಗಂಟು::is_ಎಲೆ<AstVarRef>() {
    ಹಿಂತಿರುಗಿ true;
}
ಮಾದರಿ <>
constexpr bool Ast_ಗಂಟು::is_ಎಲೆ<AstVarXRef>() {
    ಹಿಂತಿರುಗಿ true;
}

// foreach implementation
ಮಾದರಿ <typename T_Arg, typename T_Callable>
ಶೂನ್ಯ Ast_ಗಂಟು::foreachImpl(Const_Correct_Ast_ಗಂಟು<T_Arg>* nodep, ಸ್ಥಿರಾಂಕ T_Callable& f, bool ಭೇಟಿ_ಮುಂದಿನ) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using ಗಂಟು_2 = Const_Correct_Ast_ಗಂಟು<T_Arg>;

    // We prefetch this far into the stack
    constexpr ಪೂರ್ಣಾಂಕ Prefetch_ದೂರ = 2;
    // We push max 5 items per iteration
    constexpr ಗಾತ್ರ_t Max_Pushes_ಪ್ರತಿ_Iteration = 5;
    // Initial stack sie
    constexpr ಗಾತ್ರ_t Initial_ರಾಶಿ_ಗಾತ್ರ = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    ಗಾತ್ರ_t ರಾಶಿ_ಗಾತ್ರ = Initial_ರಾಶಿ_ಗಾತ್ರ;
    ಗಂಟು_2* ಒಳ_ಸಾಲು_ರಾಶಿ[Initial_ರಾಶಿ_ಗಾತ್ರ];  // In-line initial stack storage
    std::unique_ptr<ಗಂಟು_2*[]> ಹೊರಹೋಗು_ದ_ಸಾಲು_Stackp{nullptr};  // In case we need more, we will allocate
    ಗಂಟು_2** basep = ಒಳ_ಸಾಲು_ರಾಶಿ + Prefetch_ದೂರ;  // Pointer to base of stack
    ಗಂಟು_2** topp = basep;  // Pointer to top of stack
    ಗಂಟು_2** limp = ಒಳ_ಸಾಲು_ರಾಶಿ + ರಾಶಿ_ಗಾತ್ರ - Max_Pushes_ಪ್ರತಿ_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = -Prefetch_ದೂರ; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಭೇಟಿ = [&](ಗಂಟು_2* currp) {
        // Type test this node
        ಆದರೆ (Ast_ಗಂಟು::ಖಾಸಗಿ_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            ಆದರೆ VL_CONSTEXPR_CXX17 (is_ಎಲೆ<T_Arg_NonConst>()) ಹಿಂತಿರುಗಿ;
        }

        // Enqueue children for traversal, unless futile
        ಆದರೆ (mayBeUnder<T_Arg_NonConst>(currp)) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ op4p = currp->op4p()) *topp++ = op4p;
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ op3p = currp->op3p()) *topp++ = op3p;
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ op2p = currp->op2p()) *topp++ = op2p;
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    ಆದರೆ (ಭೇಟಿ_ಮುಂದಿನ && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    ಭೇಟಿ(nodep);

    // Visit the rest of the tree
    ಆಗ (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        ಗಂಟು_2* ಸ್ಥಿರಾಂಕ headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_ದೂರ]);

        // Ensure we have stack space for nextp and the 4 children
        ಆದರೆ (VL_UNLIKELY(topp >= limp)) {
            ಸ್ಥಿರಾಂಕ ptrdiff_t occupancy = topp - basep;
            {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t new_ರಾಶಿ_ಗಾತ್ರ = ರಾಶಿ_ಗಾತ್ರ * 2;
                ಗಂಟು_2** ಸ್ಥಿರಾಂಕ newStackp = new ಗಂಟು_2*[new_ರಾಶಿ_ಗಾತ್ರ];
                std::ಸ್ಮರಣೆ_ನಕಲು(newStackp, basep - Prefetch_ದೂರ,
                            ಗಾತ್ರ(ಗಂಟು_2**) * (occupancy + Prefetch_ದೂರ));
                ರಾಶಿ_ಗಾತ್ರ = new_ರಾಶಿ_ಗಾತ್ರ;
                ಹೊರಹೋಗು_ದ_ಸಾಲು_Stackp.reset(newStackp);
            }
            basep = ಹೊರಹೋಗು_ದ_ಸಾಲು_Stackp.get() + Prefetch_ದೂರ;
            topp = basep + occupancy;
            limp = ಹೊರಹೋಗು_ದ_ಸಾಲು_Stackp.get() + ರಾಶಿ_ಗಾತ್ರ - Max_Pushes_ಪ್ರತಿ_Iteration;
        }

        // Enqueue the next node
        ಆದರೆ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        ಭೇಟಿ(headp);
    }
}

// predicate implementation
ಮಾದರಿ <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_ಗಂಟು::predicateImpl(Const_Correct_Ast_ಗಂಟು<T_Arg>* nodep, ಸ್ಥಿರಾಂಕ T_Callable& p,
                            bool ಭೇಟಿ_ಮುಂದಿನ) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using ಗಂಟು_2 = Const_Correct_Ast_ಗಂಟು<T_Arg>;

    // We prefetch this far into the stack
    constexpr ಪೂರ್ಣಾಂಕ Prefetch_ದೂರ = 2;
    // We push max 5 items per iteration
    constexpr ಗಾತ್ರ_t Max_Pushes_ಪ್ರತಿ_Iteration = 5;
    // Initial stack sie
    constexpr ಗಾತ್ರ_t Initial_ರಾಶಿ_ಗಾತ್ರ = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    ಗಾತ್ರ_t ರಾಶಿ_ಗಾತ್ರ = Initial_ರಾಶಿ_ಗಾತ್ರ;
    ಗಂಟು_2* ಒಳ_ಸಾಲು_ರಾಶಿ[Initial_ರಾಶಿ_ಗಾತ್ರ];  // In-line initial stack storage
    std::unique_ptr<ಗಂಟು_2*[]> ಹೊರಹೋಗು_ದ_ಸಾಲು_Stackp{nullptr};  // In case we need more, we will allocate
    ಗಂಟು_2** basep = ಒಳ_ಸಾಲು_ರಾಶಿ + Prefetch_ದೂರ;  // Pointer to base of stack
    ಗಂಟು_2** topp = basep;  // Pointer to top of stack
    ಗಂಟು_2** limp = ಒಳ_ಸಾಲು_ರಾಶಿ + ರಾಶಿ_ಗಾತ್ರ - Max_Pushes_ಪ್ರತಿ_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = -Prefetch_ದೂರ; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಭೇಟಿ = [&](ಗಂಟು_2* currp) {
        // Type test this node
        ಆದರೆ (Ast_ಗಂಟು::ಖಾಸಗಿ_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            ಆದರೆ (p(static_cast<T_Arg*>(currp)) != N_Default) ಹಿಂತಿರುಗಿ true;
            // Short circuit if iterating leaf nodes
            ಆದರೆ VL_CONSTEXPR_CXX17 (is_ಎಲೆ<T_Arg_NonConst>()) ಹಿಂತಿರುಗಿ false;
        }

        // Enqueue children for traversal, unless futile
        ಆದರೆ (mayBeUnder<T_Arg_NonConst>(currp)) {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ op4p = currp->op4p()) *topp++ = op4p;
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ op3p = currp->op3p()) *topp++ = op3p;
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ op2p = currp->op2p()) *topp++ = op2p;
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ op1p = currp->op1p()) *topp++ = op1p;
        }

        ಹಿಂತಿರುಗಿ false;
    };

    // Enqueue the next of the root node, if required
    ಆದರೆ (ಭೇಟಿ_ಮುಂದಿನ && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    ಆದರೆ (ಭೇಟಿ(nodep)) ಹಿಂತಿರುಗಿ !N_Default;

    // Visit the rest of the tree
    ಆಗ (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        ಗಂಟು_2* ಸ್ಥಿರಾಂಕ headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_ದೂರ]);

        // Ensure we have stack space for nextp and the 4 children
        ಆದರೆ (VL_UNLIKELY(topp >= limp)) {
            ಸ್ಥಿರಾಂಕ ptrdiff_t occupancy = topp - basep;
            {
                ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t new_ರಾಶಿ_ಗಾತ್ರ = ರಾಶಿ_ಗಾತ್ರ * 2;
                ಗಂಟು_2** ಸ್ಥಿರಾಂಕ newStackp = new ಗಂಟು_2*[new_ರಾಶಿ_ಗಾತ್ರ];
                std::ಸ್ಮರಣೆ_ನಕಲು(newStackp, basep - Prefetch_ದೂರ,
                            ಗಾತ್ರ(ಗಂಟು_2**) * (occupancy + Prefetch_ದೂರ));
                ರಾಶಿ_ಗಾತ್ರ = new_ರಾಶಿ_ಗಾತ್ರ;
                ಹೊರಹೋಗು_ದ_ಸಾಲು_Stackp.reset(newStackp);
            }
            basep = ಹೊರಹೋಗು_ದ_ಸಾಲು_Stackp.get() + Prefetch_ದೂರ;
            topp = basep + occupancy;
            limp = ಹೊರಹೋಗು_ದ_ಸಾಲು_Stackp.get() + ರಾಶಿ_ಗಾತ್ರ - Max_Pushes_ಪ್ರತಿ_Iteration;
        }

        // Enqueue the next node
        ಆದರೆ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        ಆದರೆ (ಭೇಟಿ(headp)) ಹಿಂತಿರುಗಿ !N_Default;
    }

    ಹಿಂತಿರುಗಿ N_Default;
}

ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* rhs) {
    ಆದರೆ (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } ಇಲ್ಲದಿದ್ದರೆ {
        rhs->dump(os);
    }
    ಹಿಂತಿರುಗಿ os;
}
ಶೂನ್ಯ VNRelinker::relink(Ast_ಗಂಟು* newp) { newp->Ast_ಗಂಟು::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
ಮಾದರಿ <typename T_ಗಂಟು>
class VNRef final : ಸಾರ್ವಜನಿಕ std::reference_wrapper<T_ಗಂಟು> {
    static_assert(std::is_ತಳ_ದ<Ast_ಗಂಟು, T_ಗಂಟು>::ಮೌಲ್ಯ,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

ಸಾರ್ವಜನಿಕ:
    ಮಾದರಿ <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_ಗಂಟು>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(ಸ್ಥಿರಾಂಕ std::reference_wrapper<T_ಗಂಟು>& other)
        : std::reference_wrapper<T_ಗಂಟು>{other} {}
};

static_assert(ಗಾತ್ರ(VNRef<Ast_ಗಂಟು>) == ಗಾತ್ರ(std::reference_wrapper<Ast_ಗಂಟು>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
ಗಾತ್ರ_t V3HasherUncachedHash(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು&);

// Specialization of std::hash for VNRef
ಮಾದರಿ <typename T_ಗಂಟು>
ರಚನೆ std::hash<VNRef<T_ಗಂಟು>> final {
    ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(VNRef<T_ಗಂಟು> r) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
ಮಾದರಿ <typename T_ಗಂಟು>
ರಚನೆ std::equal_ಗಮ್ಯ<VNRef<T_ಗಂಟು>> final {
    ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(VNRef<T_ಗಂಟು> ra, VNRef<T_ಗಂಟು> rb) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ ra.get().same_ವೃಕ್ಷ(&(rb.get()));
    }
};

//######################################################################
// VDTypeNameScopes -- Index of the scopes declaring types, for naming them

// While alive, indexes the scopes declaring the types that dtypeName() names, as $typename names
// them: maps each class, typedef, generate block, and design unit to the generate block or design
// unit declaring it, and each design unit to the parameters its name holds; and keeps the
// prefixes of the names of the types each scope declares, once final.  So naming the types of a
// scope costs no search up the tree for it, as AstNode::aboveLoopp() is linear in the number of
// previous statements.  For passes that name many types but neither add, move, nor delete such
// declarations, V3Width and V3WidthCommit; others, as V3Param, search the tree.
class VDTypeNameScopes final {
    // STATE
    // Each class, typedef, generate block, and design unit to the generate block or design unit
    // declaring it, or nullptr if none, as at the top
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*, Ast_ಗಂಟು*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*, std::ಸದಿಶ<Ast_ಗಂಟು*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*, std::string> m_prefixes;
    ಸ್ಥಿರಾಂಕ VDTypeNameScopes* ಸ್ಥಿರಾಂಕ m_prevp;  // Index in use before this one
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    ಶೂನ್ಯ ಸೂಚ್ಯಂಕ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ VDTypeNameScopes* currentp() { ಹಿಂತಿರುಗಿ s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, Ast_ಗಂಟು*& outerpr) ಸ್ಥಿರಾಂಕ;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Ast_ಗಂಟು*>* paramsp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* modp) ಸ್ಥಿರಾಂಕ;
    // The prefix kept for scope 'scopep', or nullptr if none
    ಸ್ಥಿರಾಂಕ std::string* prefixp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* scopep) ಸ್ಥಿರಾಂಕ;
    // Keep 'prefix', final, for scope 'scopep'
    ಶೂನ್ಯ ಪೂರ್ವಪ್ರತ್ಯಯ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* scopep, ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ) ಸ್ಥಿರಾಂಕ;
};

//######################################################################
// Inline VNVisitor METHODS

ಶೂನ್ಯ VNVisitorConst::iterateConst(Ast_ಗಂಟು* nodep) { nodep->accept(*this); }
ಶೂನ್ಯ VNVisitorConst::iterateConstNull(Ast_ಗಂಟು* nodep) {
    ಆದರೆ (VL_LIKELY(nodep)) nodep->accept(*this);
}
ಶೂನ್ಯ VNVisitorConst::iterateChildrenConst(Ast_ಗಂಟು* nodep) { nodep->iterateChildrenConst(*this); }
ಶೂನ್ಯ VNVisitorConst::iterateChildrenBackwardsConst(Ast_ಗಂಟು* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
ಶೂನ್ಯ VNVisitorConst::iterate_And_ಮುಂದಿನ_Const_Null(Ast_ಗಂಟು* nodep) {
    ಆದರೆ (VL_LIKELY(nodep)) nodep->iterate_And_ಮುಂದಿನ_Const(*this);
}

ಶೂನ್ಯ VNVisitor::iterate(Ast_ಗಂಟು* nodep) { nodep->accept(*this); }
ಶೂನ್ಯ VNVisitor::iterateNull(Ast_ಗಂಟು* nodep) {
    ಆದರೆ (VL_LIKELY(nodep)) nodep->accept(*this);
}
ಶೂನ್ಯ VNVisitor::iterateChildren(Ast_ಗಂಟು* nodep) { nodep->iterateChildren(*this); }
ಶೂನ್ಯ VNVisitor::iterate_And_ಮುಂದಿನ_Null(Ast_ಗಂಟು* nodep) {
    ಆದರೆ (VL_LIKELY(nodep)) nodep->iterate_And_ಮುಂದಿನ(*this);
}
Ast_ಗಂಟು* VNVisitor::iterateSubtreeReturnEdits(Ast_ಗಂಟು* nodep) {
    ಹಿಂತಿರುಗಿ nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#ಅನ್ತರ್ಭಾವ "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#ಅನ್ತರ್ಭಾವ "V3AstNodeDType.h"
#ಅನ್ತರ್ಭಾವ "V3AstNodeOther.h"
#ಅನ್ತರ್ಭಾವ "V3AstNodeExpr.h"
#ಅನ್ತರ್ಭಾವ "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#ಅನ್ತರ್ಭಾವ "V3AstInlines.h"
ಶೂನ್ಯ dump_ಗಂಟು_List_Json(std::ostream& os, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ std::string& listName,
                      ಸ್ಥಿರಾಂಕ string& indent);

#ಆದರೆ_ಅಂತ್ಯ  // Guard
