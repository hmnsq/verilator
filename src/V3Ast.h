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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3AST_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3AST_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Broken.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FunctionTraits.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Number.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3StdFuture.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#ਅਨ੍ਤਰ੍ਭਾਵ <array>
#ਅਨ੍ਤਰ੍ਭਾਵ <cmath>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdint>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#ਜੇ defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_USER_TYPE_CHECKS
#ਜੇ_ਅੰਤ
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_USER_TYPE_CHECKS
#ਅਨ੍ਤਰ੍ਭਾਵ <variant>
#ਜੇ_ਅੰਤ

// clang-format off
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3_ਗ੍ਰਾਫ;
class ExecMTask;
class VIface_Capture_ਟੈਗ;

//######################################################################

// For broken() function, return error string if have a match
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ BROKEN_RTN(test) \
    ਕਰੋ { \
        ਜੇ (VL_UNCOVERABLE(test)) ਵਾਪਸ "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } ਜਦੋਂ (false)
// For broken() function, return error string if a base of this class has a match
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ BROKEN_ਅਧਾਰ_RTN(test) \
    ਕਰੋ { \
        ਅਚਲ ਅੱਖਰ* ਅਚਲ reasonp = (test); \
        ਜੇ (VL_UNCOVERABLE(reasonp)) ਵਾਪਸ reasonp; \
    } ਜਦੋਂ (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VN_IS(nodep, nodetypename) (Ast_ਗੰਢ::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VN_CAST(nodep, nodetypename) (Ast_ਗੰਢ::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VN_AS(nodep, nodetypename) (Ast_ਗੰਢ::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#ਜੇ defined(VL_DEBUG) || defined(VL_ASAN)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#ਨਹੀਂਤਾਂ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VN_DBG_AS(nodep, nodetypename) (Ast_ਗੰਢ::unsafe_ਨਿੱਜੀ_As<Ast##nodetypename>(nodep))
#ਜੇ_ਅੰਤ

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

ਬਣਤਰ VNTypeInfo final {
    ਅਚਲ ਅੱਖਰ* m_namep;
    ਗਿਣਤੀ OpEn : uint8_t {
        OP_ਅਣਵਰਤਿਆ,
        OP_ਵਰਤਿਆ,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ਅਚਲ ਅੱਖਰ* m_opNamep[4];
    ਮਾਪ_t m_sizeof;
};

class VNType final {
    ਸਥਿਰ ਅਚਲ VNTypeInfo s_type_Info_ਸਾਰਣੀ[];

ਜਨਤਕ:
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast__gen_type_enum.h"  // From ./astgen
    ਅਚਲ en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_ਸੁਰੱਖਿਅਤ : m_e{_e} {}
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e; }

    // Retrieve VNTypeInfo for the given type
    ਸਥਿਰ ਅਚਲ VNTypeInfo& typeInfo(VNType t) VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ s_type_Info_ਸਾਰਣੀ[t.m_e]; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VNType& lhs, ਅਚਲ VNType& rhs) VL_ਸ਼ੁੱਧ {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VNType& lhs, VNType::en rhs) VL_ਸ਼ੁੱਧ { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VNType::en lhs, ਅਚਲ VNType& rhs) VL_ਸ਼ੁੱਧ { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VNType& rhs) { ਵਾਪਸ os << rhs.ascii(); }

//######################################################################
// AstNUser - Generic base class for AST User nodes.
//          - Also used to allow parameter passing up/down iterate calls

class ਚੌੜਾਈ_VP;
class V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼;
class VSymEnt;

class VNUser final {
// Defining VL_USER_TYPE_CHECKS replaces the VNUser union with a std::variant
// which remembers which form of VNUser is being stored to catch mismatched reads.
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_USER_TYPE_CHECKS
    // monostate is an unwritten / cleared slot. It can be read as either form
    // and yields nullptr/0.
    std::variant<std::monostate, uint64_t, ਖਾਲੀ*> m_u;
#ਨਹੀਂਤਾਂ
    ਸੰਘ {
        ਖਾਲੀ* ਉੱਪਰ;
        uint64_t uq;
    } m_u;
#ਜੇ_ਅੰਤ

ਜਨਤਕ:
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_USER_TYPE_CHECKS
    VNUser() = ਮੂਲ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        ਜੇ (i) m_u = i;
    }
    explicit VNUser(ਖਾਲੀ* p) {
        // VNUser{nullptr} represents the monostate
        ਜੇ (p) m_u = p;
    }
    ~VNUser() = ਮੂਲ;
    // Casters
    ਸਾਂਚਾ <typename T>
    typename std::enable_if<std::is_ਸੂਚਕ<T>::ਮੁੱਲ, T>::type ਮੰਜ਼ਿਲ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਜੇ (std::holds_alternative<std::monostate>(m_u)) ਵਾਪਸ nullptr;
        ਖਾਲੀ* ਅਚਲ* ਅਚਲ upp = std::get_if<ਖਾਲੀ*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        ਵਾਪਸ reinterpret_cast<T>(*upp);
    }
    uint64_t ਮੰਜ਼ਿਲ_UQuad() ਅਚਲ {
        ਜੇ (std::holds_alternative<std::monostate>(m_u)) ਵਾਪਸ 0;
        ਅਚਲ uint64_t* ਅਚਲ uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        ਵਾਪਸ *uip;
    }
#ਨਹੀਂਤਾਂ
    VNUser() = ਮੂਲ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.ਉੱਪਰ = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(ਖਾਲੀ* p) { m_u.ਉੱਪਰ = p; }
    ~VNUser() = ਮੂਲ;
    // Casters
    ਸਾਂਚਾ <typename T>
    typename std::enable_if<std::is_ਸੂਚਕ<T>::ਮੁੱਲ, T>::type ਮੰਜ਼ਿਲ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ reinterpret_cast<T>(m_u.ਉੱਪਰ);
    }
    uint64_t ਮੰਜ਼ਿਲ_UQuad() ਅਚਲ { ਵਾਪਸ m_u.uq; }
#ਜੇ_ਅੰਤ
    VSymEnt* ਮੰਜ਼ਿਲ_Sym_Ent() ਅਚਲ { ਵਾਪਸ ਮੰਜ਼ਿਲ<VSymEnt*>(); }
    Ast_ਗੰਢ* ਮੰਜ਼ਿਲ_Nodep() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ ਮੰਜ਼ਿਲ<Ast_ਗੰਢ*>(); }
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਮੰਜ਼ਿਲ_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼() ਅਚਲ { ਵਾਪਸ ਮੰਜ਼ਿਲ<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(ਅਚਲ ਖਾਲੀ*)) ਅਚਲ;
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

class VNUser_ਵਿੱਚ_Use_ਅਧਾਰ
VL_NOT_FINAL{ਸੁਰੱਖਿਅਤ : ਸਥਿਰ ਖਾਲੀ allocate(ਪੂਰਨ id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvt_ਮੰਜ਼ਿਲ_Str(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
ਸਥਿਰ ਖਾਲੀ ਮੁਕਤ(ਪੂਰਨ id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvt_ਮੰਜ਼ਿਲ_Str(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
ਸਥਿਰ ਖਾਲੀ clearcnt(ਪੂਰਨ id, uint32_t& cntGblRef, ਅਚਲ bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvt_ਮੰਜ਼ਿਲ_Str(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
ਸਥਿਰ ਖਾਲੀ checkcnt(ਪੂਰਨ id, uint32_t&, ਅਚਲ bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef,
                   "Check of User" + cvt_ਮੰਜ਼ਿਲ_Str(id) + "() failed, not under AstUserInUse");
}
}
;

// For each user() declare the in use structure
// We let AstNode peek into here, because when under low optimization even
// an accessor would be way too slow.
// clang-format off
class VNUser1_ਵਿੱਚ_Use final : VNUser_ਵਿੱਚ_Use_ਅਧਾਰ {
ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ class Ast_ਗੰਢ;
    ਸਥਿਰ uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ਸਥਿਰ bool s_userBusy;  // Count is in use
ਜਨਤਕ:
    VNUser1_ਵਿੱਚ_Use()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1_ਵਿੱਚ_Use()     { ਮੁਕਤ    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ਸਥਿਰ ਖਾਲੀ clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ਸਥਿਰ ਖਾਲੀ check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2_ਵਿੱਚ_Use final : VNUser_ਵਿੱਚ_Use_ਅਧਾਰ {
ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ class Ast_ਗੰਢ;
    ਸਥਿਰ uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ਸਥਿਰ bool s_userBusy;  // Count is in use
ਜਨਤਕ:
    VNUser2_ਵਿੱਚ_Use()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2_ਵਿੱਚ_Use()     { ਮੁਕਤ    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ਸਥਿਰ ਖਾਲੀ clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ਸਥਿਰ ਖਾਲੀ check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3_ਵਿੱਚ_Use final : VNUser_ਵਿੱਚ_Use_ਅਧਾਰ {
ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ class Ast_ਗੰਢ;
    ਸਥਿਰ uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ਸਥਿਰ bool s_userBusy;  // Count is in use
ਜਨਤਕ:
    VNUser3_ਵਿੱਚ_Use()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3_ਵਿੱਚ_Use()     { ਮੁਕਤ    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ਸਥਿਰ ਖਾਲੀ clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ਸਥਿਰ ਖਾਲੀ check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4_ਵਿੱਚ_Use final : VNUser_ਵਿੱਚ_Use_ਅਧਾਰ {
ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ class Ast_ਗੰਢ;
    ਸਥਿਰ uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ਸਥਿਰ bool s_userBusy;  // Count is in use
ਜਨਤਕ:
    VNUser4_ਵਿੱਚ_Use()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4_ਵਿੱਚ_Use()     { ਮੁਕਤ    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ਸਥਿਰ ਖਾਲੀ clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ਸਥਿਰ ਖਾਲੀ check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::ਸਦਿਸ਼<Ast_ਗੰਢ*> m_deleteps;  // Nodes to delete

ਜਨਤਕ:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    ਖਾਲੀ ਧੱਕੋ_Deletep(Ast_ਗੰਢ* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.ਧੱਕੋ_ਪਿੱਠ(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    ਖਾਲੀ doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    ਮਿੱਤਰ class Ast_ਗੰਢ;

ਜਨਤਕ:
    /// Call visit()s on nodep
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterateConst(Ast_ਗੰਢ* nodep);
    /// Call visit()s on nodep
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterateConstNull(Ast_ਗੰਢ* nodep);
    /// Call visit()s on const nodep's children
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterateChildrenConst(Ast_ਗੰਢ* nodep);
    /// Call visit()s on nodep's children in backp() order
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterateChildrenBackwardsConst(Ast_ਗੰਢ* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterate_And_ਅਗਲਾ_Const_Null(Ast_ਗੰਢ* nodep);

    ਆਭਾਸੀ ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) = 0;
    ਆਭਾਸੀ ~VNVisitorConst() {}
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : ਜਨਤਕ VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
ਜਨਤਕ:
    /// Call visit()s on nodep
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterate(Ast_ਗੰਢ* nodep);
    /// Call visit()s on nodep
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterateNull(Ast_ਗੰਢ* nodep);
    /// Call visit()s on nodep's children
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterateChildren(Ast_ਗੰਢ* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    ਸਤਰਅੰਦਰ ਖਾਲੀ iterate_And_ਅਗਲਾ_Null(Ast_ਗੰਢ* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    ਸਤਰਅੰਦਰ Ast_ਗੰਢ* iterateSubtreeReturnEdits(Ast_ਗੰਢ* nodep);

    VNDeleter& deleter() { ਵਾਪਸ m_deleter; }
    ਖਾਲੀ ਧੱਕੋ_Deletep(Ast_ਗੰਢ* nodep) { deleter().ਧੱਕੋ_Deletep(nodep); }
    ਖਾਲੀ doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ class Ast_ਗੰਢ;
    ਗਿਣਤੀ RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_ਅਗਲਾ,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_ਗੰਢ* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_ਗੰਢ* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_ਗੰਢ** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

ਜਨਤਕ:
    VNRelinker() = ਮੂਲ;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    ਸਤਰਅੰਦਰ ਖਾਲੀ relink(Ast_ਗੰਢ* newp);
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ;
};
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VNRelinker& rhs) {
    rhs.dump(os);
    ਵਾਪਸ os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{ਜਨਤਕ : ਆਭਾਸੀ bool ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ {ਵਾਪਸ true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ASTNODE_PREFETCH_NON_NULL(nodep) \
    ਕਰੋ { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } ਜਦੋਂ (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ASTNODE_PREFETCH(nodep) \
    ਕਰੋ { \
        ਜੇ (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } ਜਦੋਂ (false)

class Ast_ਗੰਢ VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_ਗੰਢ* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_ਗੰਢ* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_ਗੰਢ* m_op1p = nullptr;  // Generic pointer 1
    Ast_ਗੰਢ* m_op2p = nullptr;  // Generic pointer 2
    Ast_ਗੰਢ* m_op3p = nullptr;  // Generic pointer 3
    Ast_ਗੰਢ* m_op4p = nullptr;  // Generic pointer 4
    Ast_ਗੰਢ** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    ਅਚਲ VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    ਬਣਤਰ {
        bool did_ਚੌੜਾਈ : 1;  // Did V3Width computation
        bool doing_ਚੌੜਾਈ : 1;  // Inside V3Width
        bool protect : 1;  // Protect name if protection is on
        // Space for more flags here (there must be 8 bits in total)
        uint8_t ਅਣਵਰਤਿਆ : 5;
    } m_flags;  // Attribute flags

    // State variable used by V3Broken for consistency checking. The top bit of this is byte is a
    // flag, representing V3Broken is currently proceeding under this node. The bottom 7 bits are
    // a generation number. This is hot when --debug-checks so we access as a whole to avoid bit
    // field masking resulting in unnecessary read-modify-write ops.
    uint8_t m_brokenState = 0;

    ਪੂਰਨ m_cloneCnt = 0;  // Sequence number for when last clone was made

#ਜੇ defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(ਆਕਾਰ(m_type) + ਆਕਾਰ(m_flags) + ਆਕਾਰ(m_brokenState) + ਆਕਾਰ(m_cloneCnt)
                      <= ਆਕਾਰ(ਖਾਲੀ*),
                  "packing requires padding");
#ਜੇ_ਅੰਤ

    Ast_ਗੰਢ_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_ਗੰਢ* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_ਸਤਰ* m_fileline;  // Where it was declared
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_ਗਿਣਤੀ;  // When it was last edited
#ਜੇ_ਅੰਤ
    ਸਥਿਰ uint64_t s_editCntGbl;  // Global edit counter
    ਸਥਿਰ uint64_t s_edit_Cnt_ਆਖਰੀ;  // Last committed value of global edit counter

    Ast_ਗੰਢ* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    ਸਥਿਰ ਪੂਰਨ s_cloneCntGbl;  // Count of which userp is set

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
    ਖਾਲੀ op1p(Ast_ਗੰਢ* nodep) {
        m_op1p = nodep;
        ਜੇ (nodep) nodep->m_backp = this;
    }
    ਖਾਲੀ op2p(Ast_ਗੰਢ* nodep) {
        m_op2p = nodep;
        ਜੇ (nodep) nodep->m_backp = this;
    }
    ਖਾਲੀ op3p(Ast_ਗੰਢ* nodep) {
        m_op3p = nodep;
        ਜੇ (nodep) nodep->m_backp = this;
    }
    ਖਾਲੀ op4p(Ast_ਗੰਢ* nodep) {
        m_op4p = nodep;
        ਜੇ (nodep) nodep->m_backp = this;
    }

ਨਿਜੀ:
    Ast_ਗੰਢ* clone_ਰੁੱਖ_Iter(bool need_ਸ਼ੁੱਧ);
    Ast_ਗੰਢ* clone_ਰੁੱਖ_Iter_List(bool need_ਸ਼ੁੱਧ);
    ਖਾਲੀ check_ਰੁੱਖ_Iter(ਅਚਲ Ast_ਗੰਢ* prevBackp) ਅਚਲ VL_MT_STABLE;
    bool gate_ਰੁੱਖ_Iter() ਅਚਲ;
    ਸਥਿਰ bool same_ਰੁੱਖ_Iter(ਅਚਲ Ast_ਗੰਢ* node1p, ਅਚਲ Ast_ਗੰਢ* node2p, bool ign_ਅਗਲਾ,
                             bool gateOnly);
    ਖਾਲੀ delete_ਰੁੱਖ_Iter();
    ਖਾਲੀ delete_ਗੰਢ();
    string instanceStr() ਅਚਲ;

ਜਨਤਕ:
    ਖਾਲੀ purityCheck();
    ਸਥਿਰ ਖਾਲੀ relinkOneLink(Ast_ਗੰਢ*& pointpr, Ast_ਗੰਢ* newp);
    // cppcheck-suppress functionConst
    ਸਥਿਰ ਖਾਲੀ debug_ਰੁੱਖ_Change(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ ਅੱਖਰ* ਅਗੇਤਰ, ਪੂਰਨ lineno, bool ਅਗਲਾ);

ਸੁਰੱਖਿਅਤ:
    // CONSTRUCTORS
    Ast_ਗੰਢ(VNType t, File_ਸਤਰ* fl);
    ਆਭਾਸੀ ~Ast_ਗੰਢ() = ਮੂਲ;  // Use 'deleteTree' instead
    ਆਭਾਸੀ Ast_ਗੰਢ* clone() = 0;  // Generally, cloneTree is what you want instead
    ਆਭਾਸੀ ਖਾਲੀ cloneRelink() { cloneRelinkGen(); }
    ਆਭਾਸੀ ਖਾਲੀ cloneRelinkGen() {};  // Overrides generated by 'astgen'
    ਖਾਲੀ clone_Relink_ਰੁੱਖ();

    // METHODS
    ਖਾਲੀ ਸੈੱਟ_Op1p(Ast_ਗੰਢ* newp);  // Set non-list-type op1 to non-list element
    ਖਾਲੀ ਸੈੱਟ_Op2p(Ast_ਗੰਢ* newp);  // Set non-list-type op2 to non-list element
    ਖਾਲੀ ਸੈੱਟ_Op3p(Ast_ਗੰਢ* newp);  // Set non-list-type op3 to non-list element
    ਖਾਲੀ ਸੈੱਟ_Op4p(Ast_ਗੰਢ* newp);  // Set non-list-type op4 to non-list element

    ਖਾਲੀ ਜੋੜੋ_Op1p(Ast_ਗੰਢ* newp);  // Append newp to end of op1
    ਖਾਲੀ ਜੋੜੋ_Op2p(Ast_ਗੰਢ* newp);  // Append newp to end of op2
    ਖਾਲੀ ਜੋੜੋ_Op3p(Ast_ਗੰਢ* newp);  // Append newp to end of op3
    ਖਾਲੀ ਜੋੜੋ_Op4p(Ast_ਗੰਢ* newp);  // Append newp to end of op4

    ਖਾਲੀ prependOp1p(Ast_ਗੰਢ* newp);  // Prepend newp to front of op1
    ਖਾਲੀ prependOp2p(Ast_ਗੰਢ* newp);  // Prepend newp to front of op2
    ਖਾਲੀ prependOp3p(Ast_ਗੰਢ* newp);  // Prepend newp to front of op3
    ਖਾਲੀ prependOp4p(Ast_ਗੰਢ* newp);  // Prepend newp to front of op4

    // clang-format off
    ਖਾਲੀ ਸੈੱਟ_NOp1p(Ast_ਗੰਢ* newp) { ਜੇ (newp) ਸੈੱਟ_Op1p(newp); }
    ਖਾਲੀ ਸੈੱਟ_NOp2p(Ast_ਗੰਢ* newp) { ਜੇ (newp) ਸੈੱਟ_Op2p(newp); }
    ਖਾਲੀ ਸੈੱਟ_NOp3p(Ast_ਗੰਢ* newp) { ਜੇ (newp) ਸੈੱਟ_Op3p(newp); }
    ਖਾਲੀ ਸੈੱਟ_NOp4p(Ast_ਗੰਢ* newp) { ਜੇ (newp) ਸੈੱਟ_Op4p(newp); }

    ਖਾਲੀ ਜੋੜੋ_NOp1p(Ast_ਗੰਢ* newp) { ਜੇ (newp) ਜੋੜੋ_Op1p(newp); }
    ਖਾਲੀ ਜੋੜੋ_NOp2p(Ast_ਗੰਢ* newp) { ਜੇ (newp) ਜੋੜੋ_Op2p(newp); }
    ਖਾਲੀ ਜੋੜੋ_NOp3p(Ast_ਗੰਢ* newp) { ਜੇ (newp) ਜੋੜੋ_Op3p(newp); }
    ਖਾਲੀ ਜੋੜੋ_NOp4p(Ast_ਗੰਢ* newp) { ਜੇ (newp) ਜੋੜੋ_Op4p(newp); }

    ਖਾਲੀ prependNOp1p(Ast_ਗੰਢ* newp) { ਜੇ (newp) prependOp1p(newp); }
    ਖਾਲੀ prependNOp2p(Ast_ਗੰਢ* newp) { ਜੇ (newp) prependOp2p(newp); }
    ਖਾਲੀ prependNOp3p(Ast_ਗੰਢ* newp) { ਜੇ (newp) prependOp3p(newp); }
    ਖਾਲੀ prependNOp4p(Ast_ਗੰਢ* newp) { ਜੇ (newp) prependOp4p(newp); }
    // clang-format on

    ਖਾਲੀ clonep(Ast_ਗੰਢ* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    ਸਥਿਰ ਖਾਲੀ clone_Clear_ਰੁੱਖ() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    ਆਭਾਸੀ bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ { ਵਾਪਸ true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    ਆਭਾਸੀ bool wouldBreakGen(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ oldp,
                               ਅਚਲ Ast_ਗੰਢ* ਅਚਲ newp) ਅਚਲ
        = 0;  // Generated by 'astgen'

ਜਨਤਕ:
    // ACCESSORS
    VNType type() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_type; }
    ਅਚਲ ਅੱਖਰ* typeName() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ type().ascii(); }  // See also prettyTypeName
    Ast_ਗੰਢ* nextp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_nextp; }
    Ast_ਗੰਢ* backp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_backp; }
    Ast_ਗੰਢ* abovep() ਅਚਲ;  // Get parent node above, only for list head and tail
    Ast_ਗੰਢ* aboveLoopp() ਅਚਲ;  // Get parent node above, may have performance issues as loops
    Ast_ਗੰਢ* lastp() ਅਚਲ {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        ਵਾਪਸ m_headtailp;
    }
    Ast_ਗੰਢ* prevp() ਅਚਲ {  // Previous node in list, nullptr for head of list
        ਜੇ (m_backp->m_nextp != this) ਵਾਪਸ nullptr;
        ਵਾਪਸ m_backp;
    }
    Ast_ਗੰਢ* op1p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_op1p; }
    Ast_ਗੰਢ* op2p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_op2p; }
    Ast_ਗੰਢ* op3p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_op3p; }
    Ast_ਗੰਢ* op4p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_op4p; }
    Ast_ਗੰਢ_DType* dtypep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_dtypep; }
    Ast_ਗੰਢ* clonep() ਅਚਲ { ਵਾਪਸ ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_ਗੰਢ* ਪਹਿਲਾ_Abovep() ਅਚਲ {  // Returns nullptr when second or later in list
        ਵਾਪਸ ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_brokenState; }
    ਖਾਲੀ brokenState(uint8_t ਮੁੱਲ) { m_brokenState = ਮੁੱਲ; }

    // Used by AstNode::broken()
    bool brokeExists() ਅਚਲ { ਵਾਪਸ V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ਅਚਲ { ਵਾਪਸ brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    ਆਭਾਸੀ ਖਾਲੀ foreachLink(std::function<ਖਾਲੀ(Ast_ਗੰਢ** linkpp, ਅਚਲ ਅੱਖਰ* namep)> f) = 0;

#ਜੇ defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    ਸਥਿਰ ਖਾਲੀ* ਸੰਚਾਲਕ new(ਮਾਪ_t ਮਾਪ);
    ਸਥਿਰ ਖਾਲੀ ਸੰਚਾਲਕ delete(ਖਾਲੀ* obj, ਮਾਪ_t ਮਾਪ);
#ਜੇ_ਅੰਤ

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_BRANCH = 4;  // Branch
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_CALL = INSTR_ਗਿਣਤੀ_BRANCH + 10;  // Subroutine call
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_LD = 2;  // Load memory
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_INT_MUL = 3;  // Integer multiply
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_INT_DIV = 10;  // Integer divide
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_DBL = 8;  // Convert or do float ops
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_DBL_DIV = 40;  // Double divide
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_DBL_TRIG = 200;  // Double trigonometric ops
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_STR = 100;  // String ops
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_ਸਮਾਂ = INSTR_ਗਿਣਤੀ_CALL + 5;  // Determine simulation time
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_PLI = 20;  // PLI routines
    ਸਥਿਰ constexpr ਪੂਰਨ INSTR_ਗਿਣਤੀ_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    ਆਭਾਸੀ string name() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ ""; }
    ਆਭਾਸੀ string origName() ਅਚਲ { ਵਾਪਸ ""; }
    string prettyDehashOrigOrName() ਅਚਲ {
        ਵਾਪਸ prettyName(VName::dehash(origName().ਖਾਲੀ_ਥਾਵਾਂ() ? name() : origName()));
    }
    ਆਭਾਸੀ ਖਾਲੀ name(ਅਚਲ string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    ਆਭਾਸੀ ਖਾਲੀ ਟੈਗ(ਅਚਲ string& ਪਾਠ) {}
    ਆਭਾਸੀ string ਟੈਗ() ਅਚਲ { ਵਾਪਸ ""; }
    ਆਭਾਸੀ uint32_t decl_Token_ਅੰਕ() ਅਚਲ { ਵਾਪਸ 0; }
    ਆਭਾਸੀ ਖਾਲੀ decl_Token_ਅੰਕ_ਸੈੱਟ_Min(uint32_t token_ਅੰਕ) {}
    ਆਭਾਸੀ string verilogKwd() ਅਚਲ { ਵਾਪਸ ""; }
    string nameProtect() ਅਚਲ VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() ਅਚਲ;  // origName with --protect-id applied
    string shortName() ਅਚਲ;  // Name with __PVT__ removed for concatenating scopes
    ਸਥਿਰ string dedotName(ਅਚਲ string& namein);  // Name with dots removed
    ਸਥਿਰ string nameNoArray(ਅਚਲ string& namein);  // Name with any array index removed
    ਸਥਿਰ string prettyName(ਅਚਲ string& namein) VL_ਸ਼ੁੱਧ;  // Name for printing out to the user
    ਸਥਿਰ string vpiName(ਅਚਲ string& namein);  // Name for vpi access
    ਸਥਿਰ string prettyNameQ(ਅਚਲ string& namein) {  // Quoted pretty name (for errors)
        ਵਾਪਸ "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    ਸਥਿਰ string encodeName(ਅਚਲ string& namein);
    ਸਥਿਰ string encode_ਸੰਖਿਆ(int64_t ਅੰਕ);  // Encode number into internal C representation
    ਸਥਿਰ string vcdName(ਅਚਲ string& namein);  // Name for printing out to vcd files
    string prettyName() ਅਚਲ { ਵਾਪਸ prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    ਆਭਾਸੀ string pretty_Name_ਸੁਨੇਹਾ() ਅਚਲ { ਵਾਪਸ prettyName(); }
    string prettyNameQ() ਅਚਲ { ਵਾਪਸ "'"s + pretty_Name_ਸੁਨੇਹਾ() + "'"; }  // Quoted, for messages
    string verilogName() ਅਚਲ { ਵਾਪਸ vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ਅਚਲ;
    ਆਭਾਸੀ string prettyOperatorName() ਅਚਲ { ਵਾਪਸ "operator " + prettyTypeName(); }
    File_ਸਤਰ* fileline() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_fileline; }
    ਖਾਲੀ fileline(File_ਸਤਰ* fl) { m_fileline = fl; }
    ਸਤਰਅੰਦਰ bool width1() ਅਚਲ;
    ਸਤਰਅੰਦਰ ਪੂਰਨ ਚੌੜਾਈ_Instrs() ਅਚਲ;
    bool did_ਚੌੜਾਈ() ਅਚਲ { ਵਾਪਸ m_flags.did_ਚੌੜਾਈ; }
    ਖਾਲੀ did_ਚੌੜਾਈ(bool flag) { m_flags.did_ਚੌੜਾਈ = flag; }
    bool did_ਚੌੜਾਈ_And_ਸੈੱਟ() {
        ਜੇ (did_ਚੌੜਾਈ()) ਵਾਪਸ true;
        did_ਚੌੜਾਈ(true);
        ਵਾਪਸ false;
    }
    bool doing_ਚੌੜਾਈ() ਅਚਲ { ਵਾਪਸ m_flags.doing_ਚੌੜਾਈ; }
    ਖਾਲੀ doing_ਚੌੜਾਈ(bool flag) { m_flags.doing_ਚੌੜਾਈ = flag; }
    bool protect() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_flags.protect; }
    ਖਾਲੀ protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    ਸਤਰਅੰਦਰ ਪੂਰਨ ਚੌੜਾਈ() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ ਪੂਰਨ ਚੌੜਾਈ_Min() ਅਚਲ VL_MT_STABLE;
    ਪੂਰਨ ਚੌੜਾਈ_Min_V() ਅਚਲ {
        ਵਾਪਸ v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::VERILOG_ਚੌੜਾਈ ? ਚੌੜਾਈ_Min() : ਚੌੜਾਈ();
    }
    ਪੂਰਨ ਚੌੜਾਈ_ਸ਼ਬਦ() ਅਚਲ { ਵਾਪਸ VL_ਸ਼ਬਦ_I(ਚੌੜਾਈ()); }
    bool isQuad() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ (ਚੌੜਾਈ() > VL_IDATASIZE && ਚੌੜਾਈ() <= VL_QUADSIZE); }
    bool isWide() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ (ਚੌੜਾਈ() > VL_QUADSIZE); }
    ਸਤਰਅੰਦਰ bool isCHandle() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ bool isDouble() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ bool isSigned() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ bool isString() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ bool isEvent() ਅਚਲ VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ਅਚਲ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        ਵਾਪਸ ((m_user1Cnt == VNUser1_ਵਿੱਚ_Use::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_ਗੰਢ* user1p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ user1u().ਮੰਜ਼ਿਲ_Nodep(); }
    ਖਾਲੀ user1u(ਅਚਲ VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1_ਵਿੱਚ_Use::s_userCntGbl; }
    ਖਾਲੀ user1p(ਖਾਲੀ* userp) { user1u(VNUser{userp}); }
    ਖਾਲੀ user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ਅਚਲ { ਵਾਪਸ user1u().ਮੰਜ਼ਿਲ_UQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ਅਚਲ uint64_t v = user1(); user1(v + val); ਵਾਪਸ v; }
    uint64_t user1Or(uint64_t val) { ਅਚਲ uint64_t v = user1(); user1(v | val); ਵਾਪਸ v; }
    uint64_t user1_ਸੈੱਟ_Once() { ਅਚਲ uint64_t v = user1(); ਜੇ (!v) user1(1); ਵਾਪਸ v; }  // Better for cache than user1Inc()
    ਸਥਿਰ ਖਾਲੀ user1_Clear_ਰੁੱਖ() { VNUser1_ਵਿੱਚ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ਅਚਲ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        ਵਾਪਸ ((m_user2Cnt == VNUser2_ਵਿੱਚ_Use::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_ਗੰਢ* user2p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ user2u().ਮੰਜ਼ਿਲ_Nodep(); }
    ਖਾਲੀ user2u(ਅਚਲ VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2_ਵਿੱਚ_Use::s_userCntGbl; }
    ਖਾਲੀ user2p(ਖਾਲੀ* userp) { user2u(VNUser{userp}); }
    ਖਾਲੀ user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ਅਚਲ { ਵਾਪਸ user2u().ਮੰਜ਼ਿਲ_UQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ਅਚਲ uint64_t v = user2(); user2(v + val); ਵਾਪਸ v; }
    uint64_t user2Or(uint64_t val) { ਅਚਲ uint64_t v = user2(); user2(v | val); ਵਾਪਸ v; }
    uint64_t user2_ਸੈੱਟ_Once() { ਅਚਲ uint64_t v = user2(); ਜੇ (!v) user2(1); ਵਾਪਸ v; }  // Better for cache than user2Inc()
    ਸਥਿਰ ਖਾਲੀ user2_Clear_ਰੁੱਖ() { VNUser2_ਵਿੱਚ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ਅਚਲ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        ਵਾਪਸ ((m_user3Cnt == VNUser3_ਵਿੱਚ_Use::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_ਗੰਢ* user3p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ user3u().ਮੰਜ਼ਿਲ_Nodep(); }
    ਖਾਲੀ user3u(ਅਚਲ VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3_ਵਿੱਚ_Use::s_userCntGbl; }
    ਖਾਲੀ user3p(ਖਾਲੀ* userp) { user3u(VNUser{userp}); }
    ਖਾਲੀ user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ਅਚਲ { ਵਾਪਸ user3u().ਮੰਜ਼ਿਲ_UQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ਅਚਲ uint64_t v = user3(); user3(v + val); ਵਾਪਸ v; }
    uint64_t user3Or(uint64_t val) { ਅਚਲ uint64_t v = user3(); user3(v | val); ਵਾਪਸ v; }
    uint64_t user3_ਸੈੱਟ_Once() { ਅਚਲ uint64_t v = user3(); ਜੇ (!v) user3(1); ਵਾਪਸ v; }  // Better for cache than user3Inc()
    ਸਥਿਰ ਖਾਲੀ user3_Clear_ਰੁੱਖ() { VNUser3_ਵਿੱਚ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ਅਚਲ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        ਵਾਪਸ ((m_user4Cnt == VNUser4_ਵਿੱਚ_Use::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_ਗੰਢ* user4p() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ user4u().ਮੰਜ਼ਿਲ_Nodep(); }
    ਖਾਲੀ user4u(ਅਚਲ VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4_ਵਿੱਚ_Use::s_userCntGbl; }
    ਖਾਲੀ user4p(ਖਾਲੀ* userp) { user4u(VNUser{userp}); }
    ਖਾਲੀ user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ਅਚਲ { ਵਾਪਸ user4u().ਮੰਜ਼ਿਲ_UQuad(); }
    uint64_t user4Or(uint64_t val) { ਅਚਲ uint64_t v = user4(); user4(v | val); ਵਾਪਸ v; }
    uint64_t user4Inc(uint64_t val = 1) { ਅਚਲ uint64_t v = user4(); user4(v + val); ਵਾਪਸ v; }
    uint64_t user4_ਸੈੱਟ_Once() { ਅਚਲ uint64_t v = user4(); ਜੇ (!v) user4(1); ਵਾਪਸ v; }  // Better for cache than user4Inc()
    ਸਥਿਰ ਖਾਲੀ user4_Clear_ਰੁੱਖ() { VNUser4_ਵਿੱਚ_Use::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
    uint64_t edit_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_edit_ਗਿਣਤੀ; }
    ਖਾਲੀ edit_ਗਿਣਤੀ_Inc() {
        m_edit_ਗਿਣਤੀ = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_ਰੁੱਖ();  // Any edit clears all caching
    }
#ਨਹੀਂਤਾਂ
    ਖਾਲੀ edit_ਗਿਣਤੀ_Inc() { ++s_editCntGbl; }
#ਜੇ_ਅੰਤ
    ਸਥਿਰ uint64_t edit_ਗਿਣਤੀ_ਆਖਰੀ() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ s_edit_Cnt_ਆਖਰੀ; }
    ਸਥਿਰ uint64_t edit_ਗਿਣਤੀ_Gbl() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ s_editCntGbl; }
    ਸਥਿਰ ਖਾਲੀ edit_ਗਿਣਤੀ_ਸੈੱਟ_ਆਖਰੀ() { s_edit_Cnt_ਆਖਰੀ = edit_ਗਿਣਤੀ_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_ਕਤਾਰ_ਧੱਕੋ_Self_Stmt();
    ਸਤਰਅੰਦਰ bool is_Class_Handle_ਮੁੱਲ() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool isNull() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool isZero() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool isOne() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool isNeqZero() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool isAllOnes() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool isAllOnesV() ਅਚਲ;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    ਖਾਲੀ dtypep(Ast_ਗੰਢ_DType* nodep) {
        ਜੇ (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_ਗਿਣਤੀ_Inc();
        }
    }
    ਖਾਲੀ dtypeFrom(ਅਚਲ Ast_ਗੰਢ* fromp) {
        ਜੇ (fromp) dtypep(fromp->dtypep());
    }
    ਖਾਲੀ dtypeChgSigned(bool flag = true);
    ਖਾਲੀ dtype_Chg_ਚੌੜਾਈ(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min);
    ਖਾਲੀ dtype_Chg_ਚੌੜਾਈ_Signed(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric);
    ਖਾਲੀ dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) {
        dtypep(ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric));
    }
    ਖਾਲੀ dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(ਪੂਰਨ ਚੌੜਾਈ, VSigning numeric) {
        dtypep(ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, numeric));  // Since sized, widthMin is width
    }
    ਖਾਲੀ dtype_ਸੈੱਟ_Logic_Unsized(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) {
        dtypep(ਲੱਭੋ_Logic_DType(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric));
    }
    ਖਾਲੀ dtype_ਸੈੱਟ_Logic_Sized(ਪੂਰਨ ਚੌੜਾਈ, VSigning numeric) {
        dtypep(ਲੱਭੋ_Logic_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, numeric));  // Since sized, widthMin is width
    }
    ਖਾਲੀ dtype_ਸੈੱਟ_Logic2_State_Unsized(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) {
        dtypep(ਲੱਭੋ_Logic2_State_DType(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric));
    }
    ਖਾਲੀ dtype_ਸੈੱਟ_Logic2_State_Sized(ਪੂਰਨ ਚੌੜਾਈ, VSigning numeric) {
        dtypep(ਲੱਭੋ_Logic2_State_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, numeric));  // Since sized, widthMin is width
    }
    ਖਾਲੀ dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric, bool isFourstate) {
        dtypep(ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric, isFourstate));
    }
    ਖਾਲੀ dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Sized(ਪੂਰਨ ਚੌੜਾਈ, VSigning numeric, bool isFourstate) {
        dtypep(ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    ਖਾਲੀ dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ() { dtypep(ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_Logic() { dtypep(ਲੱਭੋ_Logic_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_Logic2_State() { dtypep(ਲੱਭੋ_Logic2_State_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_Double() { dtypep(ਲੱਭੋ_Double_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_Int() { dtypep(ਲੱਭੋ_Int_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_Integer() { dtypep(ਲੱਭੋ_Integer_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_Integer2_State() { dtypep(ਲੱਭੋ_Integer2_State_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_String() { dtypep(ਲੱਭੋ_String_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_UInt32() { dtypep(ਲੱਭੋ_UInt32_DType()); }  // Twostate
    ਖਾਲੀ dtype_ਸੈੱਟ_UInt64() { dtypep(ਲੱਭੋ_UInt64_DType()); }  // Twostate
    ਖਾਲੀ dtype_ਸੈੱਟ_ਸਮਾਂ() { dtypep(ਲੱਭੋ_ਸਮਾਂ_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_Time2_State() { dtypep(ਲੱਭੋ_Time2_State_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ() { dtypep(ਲੱਭੋ_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_ਧਾਰਾ() { dtypep(ਲੱਭੋ_ਧਾਰਾ_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_Void() { dtypep(ਲੱਭੋ_Void_DType()); }
    ਖਾਲੀ dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(bool isFourstate) {
        dtypep(isFourstate ? ਲੱਭੋ_Logic_DType() : ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType());
    }

    // Data type locators
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Logic_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::LOGIC); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Null_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::NULLTYPE); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Logic2_State_DType() ਅਚਲ {
        ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Double_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::DOUBLE); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Int_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::INT); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Integer_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::INTEGER); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Integer2_State_DType() ਅਚਲ {
        ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_String_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::STRING); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Signed8_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::BYTE); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_UInt32_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT32); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_UInt64_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::UINT64); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਸਮਾਂ_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::ਸਮਾਂ); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Time2_State_DType() ਅਚਲ {
        ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_CHandle_DType() ਅਚਲ { ਵਾਪਸ ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::CHANDLE); }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Constraint_Ref_DType() ਅਚਲ;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType() ਅਚਲ;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਕਤਾਰ_ਸੂਚਕਾਂਕ_DType() ਅਚਲ;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਧਾਰਾ_DType() ਅਚਲ;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Void_DType() ਅਚਲ;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) ਅਚਲ;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Logic_DType(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) ਅਚਲ;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Logic2_State_DType(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) ਅਚਲ;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric,
                                      bool isFourstate) ਅਚਲ {
        ਵਾਪਸ isFourstate ? ਲੱਭੋ_Logic_DType(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric)
                           : ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric);
    }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Range_DType(ਅਚਲ VNumRange& range, ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Min,
                                           ਅਚਲ VSigning numeric,
                                           ਅਚਲ bool isFourstate) ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ isFourstate ? ਲੱਭੋ_Logic_Range_DType(range, ਚੌੜਾਈ_Min, numeric)
                           : ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range_DType(range, ਚੌੜਾਈ_Min, numeric);
    }
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Logic_Range_DType(ਅਚਲ VNumRange& range, ਪੂਰਨ ਚੌੜਾਈ_Min,
                                      VSigning numeric) ਅਚਲ VL_MT_STABLE;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range_DType(ਅਚਲ VNumRange& range, ਪੂਰਨ ਚੌੜਾਈ_Min,
                                    VSigning numeric) ਅਚਲ VL_MT_STABLE;
    Ast_ਗੰਢ_DType* ਲੱਭੋ_Basic_DType(VBasicDTypeKwd kwd) ਅਚਲ;
    ਸਥਿਰ AstBasicDType* ਲੱਭੋ_ਪਾਓ_Same_DType(AstBasicDType* nodep);

    ਸਥਿਰ VCastable computeCastable(ਅਚਲ Ast_ਗੰਢ_DType* ਮੰਜ਼ਿਲ_Dtp, ਅਚਲ Ast_ਗੰਢ_DType* fromDtp,
                                     ਅਚਲ Ast_ਗੰਢ* fromConstp, bool checkIfaceArgCompat = false);
    ਸਥਿਰ Ast_ਗੰਢ_DType* getCommonClassTypep(Ast_ਗੰਢ* nodep1, Ast_ਗੰਢ* nodep2);

    // METHODS - dump and error
    ਖਾਲੀ v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& str) ਅਚਲ VL_RELEASE(V3Error::s().m_mutex);
    ਖਾਲੀ v3error_ਸਿਰਾ_Fatal(ਅਚਲ std::ostringstream& str) ਅਚਲ VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex) {
        ਵਾਪਸ fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ਅਚਲ { ਵਾਪਸ fileline()->warnContextSecondary(); }
    string warn_ਹੋਰ() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex) { ਵਾਪਸ fileline()->warn_ਹੋਰ(); }
    string warnOther() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex) { ਵਾਪਸ fileline()->warnOther(); }

    ਆਭਾਸੀ ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ;
    ਸਥਿਰ ਅੱਖਰ* dump_ਰੁੱਖ_Json_Gdb(ਅਚਲ Ast_ਗੰਢ* nodep);  // For GDB only, free()d by caller
    ਸਥਿਰ ਅੱਖਰ* dump_ਰੁੱਖ_Json_Gdb(ਅਚਲ ਅੱਖਰ* str);  // For GDB only, free()d by caller
    ਸਥਿਰ ਅੱਖਰ* dump_ਰੁੱਖ_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    ਸਥਿਰ ਖਾਲੀ dumpGdb(ਅਚਲ Ast_ਗੰਢ* nodep);  // For GDB only
    ਖਾਲੀ dump_Gdb_ਸਿਰਲੇਖ() ਅਚਲ;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    ਸਾਂਚਾ <typename T_ਗੰਢ_ਨਤੀਜਾ, typename T_ਗੰਢ_ਅਗਲਾ>
    ਸਥਿਰ T_ਗੰਢ_ਨਤੀਜਾ* ਜੋੜੋ_ਅਗਲਾ(T_ਗੰਢ_ਨਤੀਜਾ* nodep, T_ਗੰਢ_ਅਗਲਾ* newp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ_ਨਤੀਜਾ>::ਮੁੱਲ,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_ਅਧਾਰ_ਦਾ<T_ਗੰਢ_ਨਤੀਜਾ, T_ਗੰਢ_ਅਗਲਾ>::ਮੁੱਲ,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        ਜੇ (!newp) ਵਾਪਸ nodep;
        ਵਾਪਸ static_cast<T_ਗੰਢ_ਨਤੀਜਾ*>(ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(nodep, newp));
    }
    ਸਾਂਚਾ <typename T_ਗੰਢ_ਨਤੀਜਾ, typename T_ਗੰਢ_ਅਗਲਾ>
    ਸਥਿਰ T_ਗੰਢ_ਨਤੀਜਾ* ਜੋੜੋ_ਅਗਲਾ_Null(T_ਗੰਢ_ਨਤੀਜਾ* nodep, T_ਗੰਢ_ਅਗਲਾ* newp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ_ਨਤੀਜਾ>::ਮੁੱਲ,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_ਅਧਾਰ_ਦਾ<T_ਗੰਢ_ਨਤੀਜਾ, T_ਗੰਢ_ਅਗਲਾ>::ਮੁੱਲ,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        ਜੇ (!newp) ਵਾਪਸ nodep;
        ਜੇ (!nodep) ਵਾਪਸ newp;
        ਵਾਪਸ static_cast<T_ਗੰਢ_ਨਤੀਜਾ*>(ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(nodep, newp));
    }
    ਸਤਰਅੰਦਰ Ast_ਗੰਢ* ਜੋੜੋ_ਅਗਲਾ(Ast_ਗੰਢ* newp);
    ਖਾਲੀ ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(Ast_ਗੰਢ* newp);  // Insert newp at this->nextp
    ਖਾਲੀ ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(Ast_ਗੰਢ* newp);  // Adds at old place of this, this becomes next
    ਖਾਲੀ replaceWith(Ast_ਗੰਢ* newp);  // Replace current node in tree with new node
    ਖਾਲੀ replaceWithKeepDType(Ast_ਗੰਢ* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_ਗੰਢ* unlink_Fr_ਪਿੱਠ(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_ਗੰਢ* unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(VNRelinker* linkerp = nullptr);
    ਖਾਲੀ relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    ਖਾਲੀ clone_Relink_ਗੰਢ() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_ਗੰਢ* clone_ਰੁੱਖ(bool clone_ਅਗਲਾ_Link,
                       bool need_ਸ਼ੁੱਧ = false);  // Not const, as sets clonep() on original nodep
    Ast_ਗੰਢ* clone_ਰੁੱਖ_ਸ਼ੁੱਧ(bool clone_ਅਗਲਾ_Link) { ਵਾਪਸ clone_ਰੁੱਖ(clone_ਅਗਲਾ_Link, true); }
    bool gate_ਰੁੱਖ() { ਵਾਪਸ gate_ਰੁੱਖ_Iter(); }  // Is tree isGateOptimizable?
    ਸਤਰਅੰਦਰ bool same_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* node2p) ਅਚਲ;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    ਸਤਰਅੰਦਰ bool same_Gate_ਰੁੱਖ(ਅਚਲ Ast_ਗੰਢ* node2p) ਅਚਲ;
    ਖਾਲੀ delete_ਰੁੱਖ();  // Always deletes the next link
    ਖਾਲੀ check_ਰੁੱਖ() ਅਚਲ VL_MT_STABLE {
        ਜੇ (v3Global.opt.debugCheck()) check_ਰੁੱਖ_Iter(backp());
    }
    ਖਾਲੀ checkIter() ਅਚਲ;
    ਖਾਲੀ dumpPtrs(std::ostream& os = std::cout) ਅਚਲ;
    ਖਾਲੀ dump_ਰੁੱਖ(std::ostream& os = std::cout, ਅਚਲ string& indent = "    ",
                  ਪੂਰਨ max_ਡੂੰਘਾਈ = 0) ਅਚਲ;
    ਖਾਲੀ dump_ਰੁੱਖ(ਅਚਲ string& indent, ਪੂਰਨ max_ਡੂੰਘਾਈ = 0) ਅਚਲ {
        dump_ਰੁੱਖ(std::cout, indent, max_ਡੂੰਘਾਈ);
    }
    ਸਥਿਰ ਖਾਲੀ dump_ਰੁੱਖ_Gdb(ਅਚਲ Ast_ਗੰਢ* nodep);  // For GDB only
    ਖਾਲੀ dump_ਰੁੱਖ_And_ਅਗਲਾ(std::ostream& os = std::cout, ਅਚਲ string& indent = "    ",
                         ਪੂਰਨ max_ਡੂੰਘਾਈ = 0) ਅਚਲ;
    ਖਾਲੀ dump_ਰੁੱਖ_File(ਅਚਲ string& filename, bool doDump = true);
    ਸਥਿਰ ਖਾਲੀ dump_ਰੁੱਖ_File_Gdb(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ ਅੱਖਰ* filenamep = nullptr);
    ਖਾਲੀ dump_ਰੁੱਖ_Dot(std::ostream& os = std::cout) ਅਚਲ;
    ਖਾਲੀ dump_ਰੁੱਖ_Dot_File(ਅਚਲ string& filename, bool doDump = true);
    ਆਭਾਸੀ ਖਾਲੀ dumpJson(std::ostream& os) ਅਚਲ { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    ਖਾਲੀ dumpJsonGen(std::ostream& os) ਅਚਲ {};
    ਆਭਾਸੀ ਖਾਲੀ dump_ਰੁੱਖ_Json_Op_Gen(std::ostream& os, ਅਚਲ string& indent) ਅਚਲ {};
    ਖਾਲੀ dump_ਰੁੱਖ_Json(std::ostream& os, ਅਚਲ string& indent = "") ਅਚਲ;
    ਖਾਲੀ dump_ਰੁੱਖ_Json_File(ਅਚਲ string& filename, bool doDump = true);
    ਸਥਿਰ ਖਾਲੀ dumpJsonMetaFileGdb(ਅਚਲ ਅੱਖਰ* filename);
    ਸਥਿਰ ਖਾਲੀ dumpJsonMetaFile(ਅਚਲ string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    ਸਥਿਰ std::string ਗੰਢ_Addr(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਵਾਪਸ v3Global.opt.dump_ਰੁੱਖ_Addrids() ? v3Global.ptr_ਮੰਜ਼ਿਲ_Id(nodep) : cvt_ਮੰਜ਼ਿਲ_Hex(nodep);
    }

    // METHODS - static advancement
    ਸਥਿਰ Ast_ਗੰਢ* afterCommentp(Ast_ਗੰਢ* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        ਜਦੋਂ (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        ਵਾਪਸ nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    ਆਭਾਸੀ bool isBrancher() ਅਚਲ { ਵਾਪਸ false; }
    // Else a AstTime etc that can't be pushed out
    ਆਭਾਸੀ bool isGateOptimizable() ਅਚਲ { ਵਾਪਸ !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    ਆਭਾਸੀ bool isGateDedupable() ਅਚਲ { ਵਾਪਸ isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    ਆਭਾਸੀ bool is_ਵਿਅੰਜਕ_Coverage_Eligible() ਅਚਲ { ਵਾਪਸ isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    ਆਭਾਸੀ bool isOutputter() { ਵਾਪਸ false; }
    // Else a AstTime etc which output can't be predicted from input
    ਆਭਾਸੀ bool isPredictOptimizable() ਅਚਲ { ਵਾਪਸ !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    ਆਭਾਸੀ bool is_ਸ਼ੁੱਧ() { ਵਾਪਸ true; }
    // Iff isPure on current node and any nextp()
    bool is_ਸ਼ੁੱਧ_And_ਅਗਲਾ() { ਵਾਪਸ is_ਸ਼ੁੱਧ() && (!nextp() || nextp()->is_ਸ਼ੁੱਧ()); }
    // An event control, delay, wait, etc.
    ਆਭਾਸੀ bool isTimingControl() ਅਚਲ { ਵਾਪਸ false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    ਆਭਾਸੀ bool isUnlikely() ਅਚਲ { ਵਾਪਸ false; }
    // Is an IEEE system function (versus internally-generated)
    ਆਭਾਸੀ bool isSystemFunc() ਅਚਲ { ਵਾਪਸ false; }
    ਆਭਾਸੀ ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ 0; }
    // Iff node is identical to another node
    ਆਭਾਸੀ bool isSame(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ {
        ਵਾਪਸ type() == samep->type() && same_ਗੰਢ(samep);
    }
    // Iff has a data type; dtype() must be non-null
    ਆਭਾਸੀ bool hasDType() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ false; }
    // Iff has a non-null childDTypep(), as generic node function
    ਆਭਾਸੀ Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ { ਵਾਪਸ nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    ਆਭਾਸੀ Ast_ਗੰਢ_DType* getChild2DTypep() ਅਚਲ { ਵਾਪਸ nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    ਆਭਾਸੀ bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ false; }
    // Don't reclaim this node in V3Dead
    ਆਭਾਸੀ bool undead() ਅਚਲ { ਵਾਪਸ false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    ਆਭਾਸੀ ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ { ਵਾਪਸ nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    ਆਭਾਸੀ ਅਚਲ ਅੱਖਰ* brokenGen() ਅਚਲ = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ newp) ਅਚਲ { ਵਾਪਸ backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    ਆਭਾਸੀ ਖਾਲੀ accept(VNVisitorConst& v) = 0;

ਸੁਰੱਖਿਅਤ:
    // All VNVisitor related functions are called as methods off the visitor
    ਮਿੱਤਰ class VNVisitor;
    ਮਿੱਤਰ class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    ਖਾਲੀ iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    ਖਾਲੀ iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    ਖਾਲੀ iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    ਖਾਲੀ iterate_And_ਅਗਲਾ(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    ਖਾਲੀ iterate_And_ਅਗਲਾ_Const(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_ਗੰਢ* iterateSubtreeReturnEdits(VNVisitor& v);

    ਸਥਿਰ ਖਾਲੀ dump_Json_ਅੰਕ(std::ostream& os, ਅਚਲ std::string& name, int64_t val);
    ਸਥਿਰ ਖਾਲੀ dumpJsonBoolIf(std::ostream& os, ਅਚਲ std::string& name, bool val);
    ਸਥਿਰ ਖਾਲੀ dumpJsonStr(std::ostream& os, ਅਚਲ std::string& name, ਅਚਲ std::string& val);
    ਸਥਿਰ ਖਾਲੀ dumpJsonPtr(std::ostream& os, ਅਚਲ std::string& name, ਅਚਲ Ast_ਗੰਢ* ਅਚਲ valp);

ਸੁਰੱਖਿਅਤ:
    ਖਾਲੀ iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    ਸਾਂਚਾ <typename T>
    ਸਤਰਅੰਦਰ ਸਥਿਰ bool ਨਿੱਜੀ_Type_Test(ਅਚਲ Ast_ਗੰਢ* nodep);

    // For internal use only.
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_DeclType>
    constexpr ਸਥਿਰ bool uselessCast() VL_ਸ਼ੁੱਧ {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_ਸੂਚਕ<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        ਵਾਪਸ std::is_ਅਧਾਰ_ਦਾ<T_ਨਿਸ਼ਾਨਾ_Type, NonCV>::ਮੁੱਲ;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_DeclType>
    constexpr ਸਥਿਰ bool impossibleCast() VL_ਸ਼ੁੱਧ {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_ਸੂਚਕ<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        ਵਾਪਸ !std::is_ਅਧਾਰ_ਦਾ<NonCV, T_ਨਿਸ਼ਾਨਾ_Type>::ਮੁੱਲ;
    }  // LCOV_EXCL_STOP

ਜਨਤਕ:
    // For use via the VN_IS macro only, or in templated code
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_ਗੰਢ>
    ਸਥਿਰ bool is(ਅਚਲ T_ਗੰਢ* nodep) VL_MT_ਸੁਰੱਖਿਅਤ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        ਵਾਪਸ nodep && ਨਿੱਜੀ_Type_Test<T_ਨਿਸ਼ਾਨਾ_Type>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_ਗੰਢ>
    ਸਥਿਰ T_ਨਿਸ਼ਾਨਾ_Type* cast(T_ਗੰਢ* nodep) VL_MT_ਸੁਰੱਖਿਅਤ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        ਵਾਪਸ nodep && ਨਿੱਜੀ_Type_Test<T_ਨਿਸ਼ਾਨਾ_Type>(nodep)
                   ? reinterpret_cast<T_ਨਿਸ਼ਾਨਾ_Type*>(nodep)
                   : nullptr;
    }
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_ਗੰਢ>
    ਸਥਿਰ ਅਚਲ T_ਨਿਸ਼ਾਨਾ_Type* cast(ਅਚਲ T_ਗੰਢ* nodep) VL_MT_ਸੁਰੱਖਿਅਤ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        ਵਾਪਸ nodep && ਨਿੱਜੀ_Type_Test<T_ਨਿਸ਼ਾਨਾ_Type>(nodep)
                   ? reinterpret_cast<ਅਚਲ T_ਨਿਸ਼ਾਨਾ_Type*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_ਗੰਢ>
    ਸਥਿਰ T_ਨਿਸ਼ਾਨਾ_Type* as(T_ਗੰਢ* nodep) VL_ਸ਼ੁੱਧ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || ਨਿੱਜੀ_Type_Test<T_ਨਿਸ਼ਾਨਾ_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        ਵਾਪਸ unsafe_ਨਿੱਜੀ_As<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(nodep);
    }
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_ਗੰਢ>
    ਸਥਿਰ ਅਚਲ T_ਨਿਸ਼ਾਨਾ_Type* as(ਅਚਲ T_ਗੰਢ* nodep) VL_ਸ਼ੁੱਧ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || ਨਿੱਜੀ_Type_Test<T_ਨਿਸ਼ਾਨਾ_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        ਵਾਪਸ unsafe_ਨਿੱਜੀ_As<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_ਗੰਢ>
    ਸਥਿਰ T_ਨਿਸ਼ਾਨਾ_Type* unsafe_ਨਿੱਜੀ_As(T_ਗੰਢ* nodep) VL_ਸ਼ੁੱਧ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        ਵਾਪਸ reinterpret_cast<T_ਨਿਸ਼ਾਨਾ_Type*>(nodep);
    }
    ਸਾਂਚਾ <typename T_ਨਿਸ਼ਾਨਾ_Type, typename T_ਗੰਢ>
    ਸਥਿਰ ਅਚਲ T_ਨਿਸ਼ਾਨਾ_Type* unsafe_ਨਿੱਜੀ_As(ਅਚਲ T_ਗੰਢ* nodep) VL_ਸ਼ੁੱਧ {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ਨਿਸ਼ਾਨਾ_Type, T_ਗੰਢ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        ਵਾਪਸ reinterpret_cast<ਅਚਲ T_ਨਿਸ਼ਾਨਾ_Type*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ਸਾਂਚਾ <typename T_ਗੰਢ>
    ਸਥਿਰ bool mayBeUnder(ਅਚਲ Ast_ਗੰਢ* nodep) {
        static_assert(!std::is_const<T_ਗੰਢ>::ਮੁੱਲ,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        ਵਾਪਸ true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ਸਾਂਚਾ <typename T_ਗੰਢ>
    ਸਥਿਰ constexpr bool is_ਪੱਤੀ() {
        static_assert(!std::is_const<T_ਗੰਢ>::ਮੁੱਲ,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        ਵਾਪਸ false;
    }

ਨਿਜੀ:
    // Using std::conditional for const correctness in the public 'foreach' functions
    ਸਾਂਚਾ <typename T_Arg>
    using Const_Correct_Ast_ਗੰਢ =
        typename std::conditional<std::is_const<T_Arg>::ਮੁੱਲ, ਅਚਲ Ast_ਗੰਢ, Ast_ਗੰਢ>::type;

    ਸਾਂਚਾ <typename T_Arg, typename T_Callable>
    ਸਤਰਅੰਦਰ ਸਥਿਰ ਖਾਲੀ foreachImpl(Const_Correct_Ast_ਗੰਢ<T_Arg>* nodep, ਅਚਲ T_Callable& f,
                                   bool ਦੇਖੋ_ਅਗਲਾ);

    ਸਾਂਚਾ <typename T_Arg, bool N_Default, typename T_Callable>
    ਸਤਰਅੰਦਰ ਸਥਿਰ bool predicateImpl(Const_Correct_Ast_ਗੰਢ<T_Arg>* nodep, ਅਚਲ T_Callable& p,
                                     bool ਦੇਖੋ_ਅਗਲਾ);

ਜਨਤਕ:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    ਸਾਂਚਾ <typename T_Callable>
    ਖਾਲੀ foreach(T_Callable&& f) {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_ਗੰਢ*>::ਮੁੱਲ
                          && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_ਗੰਢ>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ਸਾਂਚਾ <typename T_Callable>
    ਖਾਲੀ foreach(T_Callable&& f) ਅਚਲ {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ਅਚਲ T_ਗੰਢ*>::ਮੁੱਲ
                && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ਅਚਲ T_ਗੰਢ>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    ਸਾਂਚਾ <typename T_Callable>
    ਖਾਲੀ foreach_And_ਅਗਲਾ(T_Callable&& f) {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_ਗੰਢ*>::ਮੁੱਲ
                          && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_ਗੰਢ>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ਸਾਂਚਾ <typename T_Callable>
    ਖਾਲੀ foreach_And_ਅਗਲਾ(T_Callable&& f) ਅਚਲ {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ਅਚਲ T_ਗੰਢ*>::ਮੁੱਲ
                && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ਅਚਲ T_ਗੰਢ>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    ਸਾਂਚਾ <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_ਗੰਢ*>::ਮੁੱਲ
                          && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ਵਾਪਸ predicateImpl<T_ਗੰਢ, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ਸਾਂਚਾ <typename T_Callable>
    bool exists(T_Callable&& p) ਅਚਲ {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ਅਚਲ T_ਗੰਢ*>::ਮੁੱਲ
                          && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ਵਾਪਸ predicateImpl<ਅਚਲ T_ਗੰਢ, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    ਸਾਂਚਾ <typename T_Callable>
    bool exists_And_ਅਗਲਾ(T_Callable&& p) {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ਅਚਲ T_ਗੰਢ*>::ਮੁੱਲ
                          && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ਵਾਪਸ predicateImpl<T_ਗੰਢ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ਸਾਂਚਾ <typename T_Callable>
    bool exists_And_ਅਗਲਾ(T_Callable&& p) ਅਚਲ {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ਅਚਲ T_ਗੰਢ*>::ਮੁੱਲ
                          && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ਵਾਪਸ predicateImpl<ਅਚਲ T_ਗੰਢ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    ਸਾਂਚਾ <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_ਗੰਢ*>::ਮੁੱਲ
                          && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ਵਾਪਸ predicateImpl<T_ਗੰਢ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ਸਾਂਚਾ <typename T_Callable>
    bool forall(T_Callable&& p) ਅਚਲ {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ਅਚਲ T_ਗੰਢ*>::ਮੁੱਲ
                          && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ਵਾਪਸ predicateImpl<ਅਚਲ T_ਗੰਢ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    ਪੂਰਨ ਗੰਢ_ਗਿਣਤੀ() ਅਚਲ {
        // TODO: this should really return size_t, but need to fix use sites
        ਪੂਰਨ ਗਿਣਤੀ_2 = 0;
        this->foreach([&ਗਿਣਤੀ_2](ਅਚਲ Ast_ਗੰਢ*) { ++ਗਿਣਤੀ_2; });
        ਵਾਪਸ ਗਿਣਤੀ_2;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(ਪੂਰਨ ਹੱਦਬੰਦੀ) ਅਚਲ {
        ਪੂਰਨ ਗਿਣਤੀ_2 = 0;
        ਵਾਪਸ this->exists([&ਗਿਣਤੀ_2, ਹੱਦਬੰਦੀ](ਅਚਲ Ast_ਗੰਢ*) { ਵਾਪਸ ++ਗਿਣਤੀ_2 > ਹੱਦਬੰਦੀ; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
ਸਾਂਚਾ <>
Ast_ਗੰਢ* Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* newp);

// Inline method implementations
Ast_ਗੰਢ* Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(Ast_ਗੰਢ* newp) { ਵਾਪਸ ਜੋੜੋ_ਅਗਲਾ(this, newp); }

// Specializations of privateTypeTest
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ bool Ast_ਗੰਢ::mayBeUnder<AstCell>(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਵਾਪਸ !VN_IS(nodep, ਗੰਢ_Stmt) && !VN_IS(nodep, ਗੰਢ_ਵਿਅੰਜਕ);
}
ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ bool Ast_ਗੰਢ::mayBeUnder<Ast_ਗੰਢ_ਸੌਂਪੋ>(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਵਾਪਸ !VN_IS(nodep, ਗੰਢ_ਵਿਅੰਜਕ);
}
ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ bool Ast_ਗੰਢ::mayBeUnder<AstVarScope>(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਜੇ (VN_IS(nodep, VarScope)) ਵਾਪਸ false;  // Should not nest
    ਜੇ (VN_IS(nodep, Var)) ਵਾਪਸ false;
    ਜੇ (VN_IS(nodep, ਸਰਗਰਮ)) ਵਾਪਸ false;
    ਜੇ (VN_IS(nodep, ਗੰਢ_Stmt)) ਵਾਪਸ false;
    ਜੇ (VN_IS(nodep, ਗੰਢ_ਵਿਅੰਜਕ)) ਵਾਪਸ false;
    ਵਾਪਸ true;
}
ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ bool Ast_ਗੰਢ::mayBeUnder<Ast_Exec_ਗ੍ਰਾਫ>(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਜੇ (VN_IS(nodep, Exec_ਗ੍ਰਾਫ)) ਵਾਪਸ false;  // Should not nest
    ਜੇ (VN_IS(nodep, ਗੰਢ_Stmt)) ਵਾਪਸ false;  // Should be directly under CFunc
    ਵਾਪਸ true;
}
ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ bool Ast_ਗੰਢ::mayBeUnder<Ast_ਸਰਗਰਮ>(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਵਾਪਸ !VN_IS(nodep, ਸਰਗਰਮ);  // AstActives do not nest
}
ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ bool Ast_ਗੰਢ::mayBeUnder<AstScope>(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਵਾਪਸ !VN_IS(nodep, Scope);  // AstScopes do not nest
}
ਸਾਂਚਾ <>
ਸਤਰਅੰਦਰ bool Ast_ਗੰਢ::mayBeUnder<Ast_Sen_ਰੁੱਖ>(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਵਾਪਸ !VN_IS(nodep, Sen_ਰੁੱਖ);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
ਸਾਂਚਾ <>
constexpr bool Ast_ਗੰਢ::is_ਪੱਤੀ<Ast_ਗੰਢ_Var_Ref>() {
    ਵਾਪਸ true;
}
ਸਾਂਚਾ <>
constexpr bool Ast_ਗੰਢ::is_ਪੱਤੀ<AstVarRef>() {
    ਵਾਪਸ true;
}
ਸਾਂਚਾ <>
constexpr bool Ast_ਗੰਢ::is_ਪੱਤੀ<AstVarXRef>() {
    ਵਾਪਸ true;
}

// foreach implementation
ਸਾਂਚਾ <typename T_Arg, typename T_Callable>
ਖਾਲੀ Ast_ਗੰਢ::foreachImpl(Const_Correct_Ast_ਗੰਢ<T_Arg>* nodep, ਅਚਲ T_Callable& f, bool ਦੇਖੋ_ਅਗਲਾ) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using ਗੰਢ_2 = Const_Correct_Ast_ਗੰਢ<T_Arg>;

    // We prefetch this far into the stack
    constexpr ਪੂਰਨ Prefetch_ਦੂਰੀ = 2;
    // We push max 5 items per iteration
    constexpr ਮਾਪ_t Max_Pushes_ਪ੍ਰਤੀ_Iteration = 5;
    // Initial stack sie
    constexpr ਮਾਪ_t Initial_ਢੇਰ_ਮਾਪ = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    ਮਾਪ_t ਢੇਰ_ਮਾਪ = Initial_ਢੇਰ_ਮਾਪ;
    ਗੰਢ_2* ਵਿੱਚ_ਸਤਰ_ਢੇਰ[Initial_ਢੇਰ_ਮਾਪ];  // In-line initial stack storage
    std::unique_ptr<ਗੰਢ_2*[]> ਬਾਹਰਲਾ_ਦਾ_ਸਤਰ_Stackp{nullptr};  // In case we need more, we will allocate
    ਗੰਢ_2** basep = ਵਿੱਚ_ਸਤਰ_ਢੇਰ + Prefetch_ਦੂਰੀ;  // Pointer to base of stack
    ਗੰਢ_2** topp = basep;  // Pointer to top of stack
    ਗੰਢ_2** limp = ਵਿੱਚ_ਸਤਰ_ਢੇਰ + ਢੇਰ_ਮਾਪ - Max_Pushes_ਪ੍ਰਤੀ_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    ਲਈ (ਪੂਰਨ i = -Prefetch_ਦੂਰੀ; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਦੇਖੋ = [&](ਗੰਢ_2* currp) {
        // Type test this node
        ਜੇ (Ast_ਗੰਢ::ਨਿੱਜੀ_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            ਜੇ VL_CONSTEXPR_CXX17 (is_ਪੱਤੀ<T_Arg_NonConst>()) ਵਾਪਸ;
        }

        // Enqueue children for traversal, unless futile
        ਜੇ (mayBeUnder<T_Arg_NonConst>(currp)) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ op4p = currp->op4p()) *topp++ = op4p;
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ op3p = currp->op3p()) *topp++ = op3p;
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ op2p = currp->op2p()) *topp++ = op2p;
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    ਜੇ (ਦੇਖੋ_ਅਗਲਾ && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    ਦੇਖੋ(nodep);

    // Visit the rest of the tree
    ਜਦੋਂ (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        ਗੰਢ_2* ਅਚਲ headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_ਦੂਰੀ]);

        // Ensure we have stack space for nextp and the 4 children
        ਜੇ (VL_UNLIKELY(topp >= limp)) {
            ਅਚਲ ptrdiff_t occupancy = topp - basep;
            {
                ਅਚਲ ਮਾਪ_t new_ਢੇਰ_ਮਾਪ = ਢੇਰ_ਮਾਪ * 2;
                ਗੰਢ_2** ਅਚਲ newStackp = new ਗੰਢ_2*[new_ਢੇਰ_ਮਾਪ];
                std::ਸਮ੍ਰਿਤੀ_ਨਕਲ(newStackp, basep - Prefetch_ਦੂਰੀ,
                            ਆਕਾਰ(ਗੰਢ_2**) * (occupancy + Prefetch_ਦੂਰੀ));
                ਢੇਰ_ਮਾਪ = new_ਢੇਰ_ਮਾਪ;
                ਬਾਹਰਲਾ_ਦਾ_ਸਤਰ_Stackp.reset(newStackp);
            }
            basep = ਬਾਹਰਲਾ_ਦਾ_ਸਤਰ_Stackp.get() + Prefetch_ਦੂਰੀ;
            topp = basep + occupancy;
            limp = ਬਾਹਰਲਾ_ਦਾ_ਸਤਰ_Stackp.get() + ਢੇਰ_ਮਾਪ - Max_Pushes_ਪ੍ਰਤੀ_Iteration;
        }

        // Enqueue the next node
        ਜੇ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        ਦੇਖੋ(headp);
    }
}

// predicate implementation
ਸਾਂਚਾ <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_ਗੰਢ::predicateImpl(Const_Correct_Ast_ਗੰਢ<T_Arg>* nodep, ਅਚਲ T_Callable& p,
                            bool ਦੇਖੋ_ਅਗਲਾ) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using ਗੰਢ_2 = Const_Correct_Ast_ਗੰਢ<T_Arg>;

    // We prefetch this far into the stack
    constexpr ਪੂਰਨ Prefetch_ਦੂਰੀ = 2;
    // We push max 5 items per iteration
    constexpr ਮਾਪ_t Max_Pushes_ਪ੍ਰਤੀ_Iteration = 5;
    // Initial stack sie
    constexpr ਮਾਪ_t Initial_ਢੇਰ_ਮਾਪ = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    ਮਾਪ_t ਢੇਰ_ਮਾਪ = Initial_ਢੇਰ_ਮਾਪ;
    ਗੰਢ_2* ਵਿੱਚ_ਸਤਰ_ਢੇਰ[Initial_ਢੇਰ_ਮਾਪ];  // In-line initial stack storage
    std::unique_ptr<ਗੰਢ_2*[]> ਬਾਹਰਲਾ_ਦਾ_ਸਤਰ_Stackp{nullptr};  // In case we need more, we will allocate
    ਗੰਢ_2** basep = ਵਿੱਚ_ਸਤਰ_ਢੇਰ + Prefetch_ਦੂਰੀ;  // Pointer to base of stack
    ਗੰਢ_2** topp = basep;  // Pointer to top of stack
    ਗੰਢ_2** limp = ਵਿੱਚ_ਸਤਰ_ਢੇਰ + ਢੇਰ_ਮਾਪ - Max_Pushes_ਪ੍ਰਤੀ_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    ਲਈ (ਪੂਰਨ i = -Prefetch_ਦੂਰੀ; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਦੇਖੋ = [&](ਗੰਢ_2* currp) {
        // Type test this node
        ਜੇ (Ast_ਗੰਢ::ਨਿੱਜੀ_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            ਜੇ (p(static_cast<T_Arg*>(currp)) != N_Default) ਵਾਪਸ true;
            // Short circuit if iterating leaf nodes
            ਜੇ VL_CONSTEXPR_CXX17 (is_ਪੱਤੀ<T_Arg_NonConst>()) ਵਾਪਸ false;
        }

        // Enqueue children for traversal, unless futile
        ਜੇ (mayBeUnder<T_Arg_NonConst>(currp)) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ op4p = currp->op4p()) *topp++ = op4p;
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ op3p = currp->op3p()) *topp++ = op3p;
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ op2p = currp->op2p()) *topp++ = op2p;
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ op1p = currp->op1p()) *topp++ = op1p;
        }

        ਵਾਪਸ false;
    };

    // Enqueue the next of the root node, if required
    ਜੇ (ਦੇਖੋ_ਅਗਲਾ && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    ਜੇ (ਦੇਖੋ(nodep)) ਵਾਪਸ !N_Default;

    // Visit the rest of the tree
    ਜਦੋਂ (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        ਗੰਢ_2* ਅਚਲ headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_ਦੂਰੀ]);

        // Ensure we have stack space for nextp and the 4 children
        ਜੇ (VL_UNLIKELY(topp >= limp)) {
            ਅਚਲ ptrdiff_t occupancy = topp - basep;
            {
                ਅਚਲ ਮਾਪ_t new_ਢੇਰ_ਮਾਪ = ਢੇਰ_ਮਾਪ * 2;
                ਗੰਢ_2** ਅਚਲ newStackp = new ਗੰਢ_2*[new_ਢੇਰ_ਮਾਪ];
                std::ਸਮ੍ਰਿਤੀ_ਨਕਲ(newStackp, basep - Prefetch_ਦੂਰੀ,
                            ਆਕਾਰ(ਗੰਢ_2**) * (occupancy + Prefetch_ਦੂਰੀ));
                ਢੇਰ_ਮਾਪ = new_ਢੇਰ_ਮਾਪ;
                ਬਾਹਰਲਾ_ਦਾ_ਸਤਰ_Stackp.reset(newStackp);
            }
            basep = ਬਾਹਰਲਾ_ਦਾ_ਸਤਰ_Stackp.get() + Prefetch_ਦੂਰੀ;
            topp = basep + occupancy;
            limp = ਬਾਹਰਲਾ_ਦਾ_ਸਤਰ_Stackp.get() + ਢੇਰ_ਮਾਪ - Max_Pushes_ਪ੍ਰਤੀ_Iteration;
        }

        // Enqueue the next node
        ਜੇ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        ਜੇ (ਦੇਖੋ(headp)) ਵਾਪਸ !N_Default;
    }

    ਵਾਪਸ N_Default;
}

ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ Ast_ਗੰਢ* rhs) {
    ਜੇ (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } ਨਹੀਂਤਾਂ ਜੇ (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } ਨਹੀਂਤਾਂ {
        rhs->dump(os);
    }
    ਵਾਪਸ os;
}
ਖਾਲੀ VNRelinker::relink(Ast_ਗੰਢ* newp) { newp->Ast_ਗੰਢ::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
ਸਾਂਚਾ <typename T_ਗੰਢ>
class VNRef final : ਜਨਤਕ std::reference_wrapper<T_ਗੰਢ> {
    static_assert(std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

ਜਨਤਕ:
    ਸਾਂਚਾ <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_ਗੰਢ>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(ਅਚਲ std::reference_wrapper<T_ਗੰਢ>& other)
        : std::reference_wrapper<T_ਗੰਢ>{other} {}
};

static_assert(ਆਕਾਰ(VNRef<Ast_ਗੰਢ>) == ਆਕਾਰ(std::reference_wrapper<Ast_ਗੰਢ>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
ਮਾਪ_t V3HasherUncachedHash(ਅਚਲ Ast_ਗੰਢ&);

// Specialization of std::hash for VNRef
ਸਾਂਚਾ <typename T_ਗੰਢ>
ਬਣਤਰ std::hash<VNRef<T_ਗੰਢ>> final {
    ਮਾਪ_t ਸੰਚਾਲਕ()(VNRef<T_ਗੰਢ> r) ਅਚਲ { ਵਾਪਸ V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
ਸਾਂਚਾ <typename T_ਗੰਢ>
ਬਣਤਰ std::equal_ਮੰਜ਼ਿਲ<VNRef<T_ਗੰਢ>> final {
    ਮਾਪ_t ਸੰਚਾਲਕ()(VNRef<T_ਗੰਢ> ra, VNRef<T_ਗੰਢ> rb) ਅਚਲ {
        ਵਾਪਸ ra.get().same_ਰੁੱਖ(&(rb.get()));
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
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ*, Ast_ਗੰਢ*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ*, std::ਸਦਿਸ਼<Ast_ਗੰਢ*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<ਅਚਲ Ast_ਗੰਢ*, std::string> m_prefixes;
    ਅਚਲ VDTypeNameScopes* ਅਚਲ m_prevp;  // Index in use before this one
    ਸਥਿਰ ਅਚਲ VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    ਖਾਲੀ ਸੂਚਕਾਂਕ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

ਜਨਤਕ:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    ਸਥਿਰ ਅਚਲ VDTypeNameScopes* currentp() { ਵਾਪਸ s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(ਅਚਲ Ast_ਗੰਢ* nodep, Ast_ਗੰਢ*& outerpr) ਅਚਲ;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ*>* paramsp(ਅਚਲ Ast_ਗੰਢ* modp) ਅਚਲ;
    // The prefix kept for scope 'scopep', or nullptr if none
    ਅਚਲ std::string* prefixp(ਅਚਲ Ast_ਗੰਢ* scopep) ਅਚਲ;
    // Keep 'prefix', final, for scope 'scopep'
    ਖਾਲੀ ਅਗੇਤਰ(ਅਚਲ Ast_ਗੰਢ* scopep, ਅਚਲ std::string& ਅਗੇਤਰ) ਅਚਲ;
};

//######################################################################
// Inline VNVisitor METHODS

ਖਾਲੀ VNVisitorConst::iterateConst(Ast_ਗੰਢ* nodep) { nodep->accept(*this); }
ਖਾਲੀ VNVisitorConst::iterateConstNull(Ast_ਗੰਢ* nodep) {
    ਜੇ (VL_LIKELY(nodep)) nodep->accept(*this);
}
ਖਾਲੀ VNVisitorConst::iterateChildrenConst(Ast_ਗੰਢ* nodep) { nodep->iterateChildrenConst(*this); }
ਖਾਲੀ VNVisitorConst::iterateChildrenBackwardsConst(Ast_ਗੰਢ* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
ਖਾਲੀ VNVisitorConst::iterate_And_ਅਗਲਾ_Const_Null(Ast_ਗੰਢ* nodep) {
    ਜੇ (VL_LIKELY(nodep)) nodep->iterate_And_ਅਗਲਾ_Const(*this);
}

ਖਾਲੀ VNVisitor::iterate(Ast_ਗੰਢ* nodep) { nodep->accept(*this); }
ਖਾਲੀ VNVisitor::iterateNull(Ast_ਗੰਢ* nodep) {
    ਜੇ (VL_LIKELY(nodep)) nodep->accept(*this);
}
ਖਾਲੀ VNVisitor::iterateChildren(Ast_ਗੰਢ* nodep) { nodep->iterateChildren(*this); }
ਖਾਲੀ VNVisitor::iterate_And_ਅਗਲਾ_Null(Ast_ਗੰਢ* nodep) {
    ਜੇ (VL_LIKELY(nodep)) nodep->iterate_And_ਅਗਲਾ(*this);
}
Ast_ਗੰਢ* VNVisitor::iterateSubtreeReturnEdits(Ast_ਗੰਢ* nodep) {
    ਵਾਪਸ nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstNodeDType.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstNodeOther.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstNodeExpr.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstInlines.h"
ਖਾਲੀ dump_ਗੰਢ_List_Json(std::ostream& os, ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ std::string& listName,
                      ਅਚਲ string& indent);

#ਜੇ_ਅੰਤ  // Guard
