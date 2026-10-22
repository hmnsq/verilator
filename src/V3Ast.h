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

#اگر_غیر_تعریف_شدہ VERILATOR_V3AST_H_
#تعریف_کرو VERILATOR_V3AST_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Broken.h"
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3FunctionTraits.h"
#شامل "V3Global.h"
#شامل "V3Number.h"
#شامل "V3StdFuture.h"

#شامل "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#شامل <array>
#شامل <cmath>
#شامل <cstdint>
#شامل <functional>
#شامل <map>
#شامل <set>
#شامل <type_traits>
#شامل <unordered_map>
#شامل <unordered_set>
#شامل <utility>
#شامل <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#اگر defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#تعریف_منسوخ VL_USER_TYPE_CHECKS
#اگر_ختم
#اگر_تعریف_شدہ VL_USER_TYPE_CHECKS
#شامل <variant>
#اگر_ختم

// clang-format off
#شامل "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3_گراف;
class ExecMTask;
class VIface_Capture_لیبل;

//######################################################################

// For broken() function, return error string if have a match
#تعریف_کرو BROKEN_RTN(test) \
    کرو { \
        اگر (VL_UNCOVERABLE(test)) واپس "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } جب (false)
// For broken() function, return error string if a base of this class has a match
#تعریف_کرو BROKEN_بنیاد_RTN(test) \
    کرو { \
        ثابت حرف* ثابت reasonp = (test); \
        اگر (VL_UNCOVERABLE(reasonp)) واپس reasonp; \
    } جب (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#تعریف_کرو VN_IS(nodep, nodetypename) (Ast_گرہ::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#تعریف_کرو VN_CAST(nodep, nodetypename) (Ast_گرہ::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#تعریف_کرو VN_AS(nodep, nodetypename) (Ast_گرہ::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#اگر defined(VL_DEBUG) || defined(VL_ASAN)
#تعریف_کرو VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#ورنہ
#تعریف_کرو VN_DBG_AS(nodep, nodetypename) (Ast_گرہ::unsafe_نجی_As<Ast##nodetypename>(nodep))
#اگر_ختم

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#تعریف_کرو VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

ساخت VNTypeInfo final {
    ثابت حرف* m_namep;
    شمار OpEn : uint8_t {
        OP_غیر_مستعمل,
        OP_مستعمل,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ثابت حرف* m_opNamep[4];
    سائز_t m_sizeof;
};

class VNType final {
    ساکن ثابت VNTypeInfo s_type_Info_جدول[];

عوامی:
#شامل "V3Ast__gen_type_enum.h"  // From ./astgen
    ثابت en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_محفوظ : m_e{_e} {}
    constexpr عامل en() ثابت VL_MT_محفوظ { واپس m_e; }

    // Retrieve VNTypeInfo for the given type
    ساکن ثابت VNTypeInfo& typeInfo(VNType t) VL_MT_محفوظ { واپس s_type_Info_جدول[t.m_e]; }
};
constexpr bool عامل==(ثابت VNType& lhs, ثابت VNType& rhs) VL_خالص {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VNType& lhs, VNType::en rhs) VL_خالص { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VNType::en lhs, ثابت VNType& rhs) VL_خالص { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VNType& rhs) { واپس os << rhs.ascii(); }

//######################################################################
// AstNUser - Generic base class for AST User nodes.
//          - Also used to allow parameter passing up/down iterate calls

class چوڑائی_VP;
class V3_گراف_راس;
class VSymEnt;

class VNUser final {
// Defining VL_USER_TYPE_CHECKS replaces the VNUser union with a std::variant
// which remembers which form of VNUser is being stored to catch mismatched reads.
#اگر_تعریف_شدہ VL_USER_TYPE_CHECKS
    // monostate is an unwritten / cleared slot. It can be read as either form
    // and yields nullptr/0.
    std::variant<std::monostate, uint64_t, خالی*> m_u;
#ورنہ
    اتحاد {
        خالی* اوپر;
        uint64_t uq;
    } m_u;
#اگر_ختم

عوامی:
#اگر_تعریف_شدہ VL_USER_TYPE_CHECKS
    VNUser() = طےشدہ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        اگر (i) m_u = i;
    }
    explicit VNUser(خالی* p) {
        // VNUser{nullptr} represents the monostate
        اگر (p) m_u = p;
    }
    ~VNUser() = طےشدہ;
    // Casters
    سانچہ <typename T>
    typename std::enable_if<std::is_اشارہ_گر<T>::قدر, T>::type مقصد() ثابت VL_MT_محفوظ {
        اگر (std::holds_alternative<std::monostate>(m_u)) واپس nullptr;
        خالی* ثابت* ثابت upp = std::get_if<خالی*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        واپس reinterpret_cast<T>(*upp);
    }
    uint64_t مقصد_UQuad() ثابت {
        اگر (std::holds_alternative<std::monostate>(m_u)) واپس 0;
        ثابت uint64_t* ثابت uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        واپس *uip;
    }
#ورنہ
    VNUser() = طےشدہ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.اوپر = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(خالی* p) { m_u.اوپر = p; }
    ~VNUser() = طےشدہ;
    // Casters
    سانچہ <typename T>
    typename std::enable_if<std::is_اشارہ_گر<T>::قدر, T>::type مقصد() ثابت VL_MT_محفوظ {
        واپس reinterpret_cast<T>(m_u.اوپر);
    }
    uint64_t مقصد_UQuad() ثابت { واپس m_u.uq; }
#اگر_ختم
    VSymEnt* مقصد_Sym_Ent() ثابت { واپس مقصد<VSymEnt*>(); }
    Ast_گرہ* مقصد_Nodep() ثابت VL_MT_محفوظ { واپس مقصد<Ast_گرہ*>(); }
    V3_گراف_راس* مقصد_گراف_راس() ثابت { واپس مقصد<V3_گراف_راس*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(ثابت خالی*)) ثابت;
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

class VNUser_اندر_Use_بنیاد
VL_NOT_FINAL{محفوظ : ساکن خالی allocate(صحیح id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvt_مقصد_Str(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
ساکن خالی آزاد(صحیح id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvt_مقصد_Str(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
ساکن خالی clearcnt(صحیح id, uint32_t& cntGblRef, ثابت bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvt_مقصد_Str(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
ساکن خالی checkcnt(صحیح id, uint32_t&, ثابت bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef,
                   "Check of User" + cvt_مقصد_Str(id) + "() failed, not under AstUserInUse");
}
}
;

// For each user() declare the in use structure
// We let AstNode peek into here, because when under low optimization even
// an accessor would be way too slow.
// clang-format off
class VNUser1_اندر_Use final : VNUser_اندر_Use_بنیاد {
محفوظ:
    دوست class Ast_گرہ;
    ساکن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساکن bool s_userBusy;  // Count is in use
عوامی:
    VNUser1_اندر_Use()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1_اندر_Use()     { آزاد    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساکن خالی clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساکن خالی check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2_اندر_Use final : VNUser_اندر_Use_بنیاد {
محفوظ:
    دوست class Ast_گرہ;
    ساکن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساکن bool s_userBusy;  // Count is in use
عوامی:
    VNUser2_اندر_Use()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2_اندر_Use()     { آزاد    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساکن خالی clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساکن خالی check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3_اندر_Use final : VNUser_اندر_Use_بنیاد {
محفوظ:
    دوست class Ast_گرہ;
    ساکن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساکن bool s_userBusy;  // Count is in use
عوامی:
    VNUser3_اندر_Use()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3_اندر_Use()     { آزاد    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساکن خالی clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساکن خالی check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4_اندر_Use final : VNUser_اندر_Use_بنیاد {
محفوظ:
    دوست class Ast_گرہ;
    ساکن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساکن bool s_userBusy;  // Count is in use
عوامی:
    VNUser4_اندر_Use()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4_اندر_Use()     { آزاد    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساکن خالی clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساکن خالی check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::سمتیہ<Ast_گرہ*> m_deleteps;  // Nodes to delete

عوامی:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    خالی ڈالو_Deletep(Ast_گرہ* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.ڈالو_عقب(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    خالی doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    دوست class Ast_گرہ;

عوامی:
    /// Call visit()s on nodep
    اندرونی خالی iterateConst(Ast_گرہ* nodep);
    /// Call visit()s on nodep
    اندرونی خالی iterateConstNull(Ast_گرہ* nodep);
    /// Call visit()s on const nodep's children
    اندرونی خالی iterateChildrenConst(Ast_گرہ* nodep);
    /// Call visit()s on nodep's children in backp() order
    اندرونی خالی iterateChildrenBackwardsConst(Ast_گرہ* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    اندرونی خالی iterate_And_اگلا_Const_Null(Ast_گرہ* nodep);

    مجازی خالی دورہ(Ast_گرہ* nodep) = 0;
    مجازی ~VNVisitorConst() {}
#شامل "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : عوامی VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
عوامی:
    /// Call visit()s on nodep
    اندرونی خالی iterate(Ast_گرہ* nodep);
    /// Call visit()s on nodep
    اندرونی خالی iterateNull(Ast_گرہ* nodep);
    /// Call visit()s on nodep's children
    اندرونی خالی iterateChildren(Ast_گرہ* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    اندرونی خالی iterate_And_اگلا_Null(Ast_گرہ* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    اندرونی Ast_گرہ* iterateSubtreeReturnEdits(Ast_گرہ* nodep);

    VNDeleter& deleter() { واپس m_deleter; }
    خالی ڈالو_Deletep(Ast_گرہ* nodep) { deleter().ڈالو_Deletep(nodep); }
    خالی doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
محفوظ:
    دوست class Ast_گرہ;
    شمار RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_اگلا,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_گرہ* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_گرہ* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_گرہ** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

عوامی:
    VNRelinker() = طےشدہ;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    اندرونی خالی relink(Ast_گرہ* newp);
    خالی dump(std::ostream& str = std::cout) ثابت;
};
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VNRelinker& rhs) {
    rhs.dump(os);
    واپس os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{عوامی : مجازی bool گرہ_مطابقت(ثابت Ast_گرہ* nodep) ثابت {واپس true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#تعریف_کرو ASTNODE_PREFETCH_NON_NULL(nodep) \
    کرو { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } جب (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#تعریف_کرو ASTNODE_PREFETCH(nodep) \
    کرو { \
        اگر (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } جب (false)

class Ast_گرہ VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_گرہ* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_گرہ* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_گرہ* m_op1p = nullptr;  // Generic pointer 1
    Ast_گرہ* m_op2p = nullptr;  // Generic pointer 2
    Ast_گرہ* m_op3p = nullptr;  // Generic pointer 3
    Ast_گرہ* m_op4p = nullptr;  // Generic pointer 4
    Ast_گرہ** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    ثابت VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    ساخت {
        bool did_چوڑائی : 1;  // Did V3Width computation
        bool doing_چوڑائی : 1;  // Inside V3Width
        bool protect : 1;  // Protect name if protection is on
        // Space for more flags here (there must be 8 bits in total)
        uint8_t غیر_مستعمل : 5;
    } m_flags;  // Attribute flags

    // State variable used by V3Broken for consistency checking. The top bit of this is byte is a
    // flag, representing V3Broken is currently proceeding under this node. The bottom 7 bits are
    // a generation number. This is hot when --debug-checks so we access as a whole to avoid bit
    // field masking resulting in unnecessary read-modify-write ops.
    uint8_t m_brokenState = 0;

    صحیح m_cloneCnt = 0;  // Sequence number for when last clone was made

#اگر defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(حجم(m_type) + حجم(m_flags) + حجم(m_brokenState) + حجم(m_cloneCnt)
                      <= حجم(خالی*),
                  "packing requires padding");
#اگر_ختم

    Ast_گرہ_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_گرہ* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_سطر* m_fileline;  // Where it was declared
#اگر_تعریف_شدہ VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_گنتی;  // When it was last edited
#اگر_ختم
    ساکن uint64_t s_editCntGbl;  // Global edit counter
    ساکن uint64_t s_edit_Cnt_آخری;  // Last committed value of global edit counter

    Ast_گرہ* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    ساکن صحیح s_cloneCntGbl;  // Count of which userp is set

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
    خالی op1p(Ast_گرہ* nodep) {
        m_op1p = nodep;
        اگر (nodep) nodep->m_backp = this;
    }
    خالی op2p(Ast_گرہ* nodep) {
        m_op2p = nodep;
        اگر (nodep) nodep->m_backp = this;
    }
    خالی op3p(Ast_گرہ* nodep) {
        m_op3p = nodep;
        اگر (nodep) nodep->m_backp = this;
    }
    خالی op4p(Ast_گرہ* nodep) {
        m_op4p = nodep;
        اگر (nodep) nodep->m_backp = this;
    }

نجی:
    Ast_گرہ* clone_درخت_Iter(bool need_خالص);
    Ast_گرہ* clone_درخت_Iter_List(bool need_خالص);
    خالی check_درخت_Iter(ثابت Ast_گرہ* پچھلا_Backp) ثابت VL_MT_STABLE;
    bool gate_درخت_Iter() ثابت;
    ساکن bool same_درخت_Iter(ثابت Ast_گرہ* node1p, ثابت Ast_گرہ* node2p, bool ign_اگلا,
                             bool gateOnly);
    خالی delete_درخت_Iter();
    خالی delete_گرہ();
    string instanceStr() ثابت;

عوامی:
    خالی purityCheck();
    ساکن خالی relinkOneLink(Ast_گرہ*& pointpr, Ast_گرہ* newp);
    // cppcheck-suppress functionConst
    ساکن خالی debug_درخت_Change(ثابت Ast_گرہ* nodep, ثابت حرف* سابقہ, صحیح lineno, bool اگلا);

محفوظ:
    // CONSTRUCTORS
    Ast_گرہ(VNType t, File_سطر* fl);
    مجازی ~Ast_گرہ() = طےشدہ;  // Use 'deleteTree' instead
    مجازی Ast_گرہ* clone() = 0;  // Generally, cloneTree is what you want instead
    مجازی خالی cloneRelink() { cloneRelinkGen(); }
    مجازی خالی cloneRelinkGen() {};  // Overrides generated by 'astgen'
    خالی clone_Relink_درخت();

    // METHODS
    خالی سیٹ_Op1p(Ast_گرہ* newp);  // Set non-list-type op1 to non-list element
    خالی سیٹ_Op2p(Ast_گرہ* newp);  // Set non-list-type op2 to non-list element
    خالی سیٹ_Op3p(Ast_گرہ* newp);  // Set non-list-type op3 to non-list element
    خالی سیٹ_Op4p(Ast_گرہ* newp);  // Set non-list-type op4 to non-list element

    خالی جوڑو_Op1p(Ast_گرہ* newp);  // Append newp to end of op1
    خالی جوڑو_Op2p(Ast_گرہ* newp);  // Append newp to end of op2
    خالی جوڑو_Op3p(Ast_گرہ* newp);  // Append newp to end of op3
    خالی جوڑو_Op4p(Ast_گرہ* newp);  // Append newp to end of op4

    خالی prependOp1p(Ast_گرہ* newp);  // Prepend newp to front of op1
    خالی prependOp2p(Ast_گرہ* newp);  // Prepend newp to front of op2
    خالی prependOp3p(Ast_گرہ* newp);  // Prepend newp to front of op3
    خالی prependOp4p(Ast_گرہ* newp);  // Prepend newp to front of op4

    // clang-format off
    خالی سیٹ_NOp1p(Ast_گرہ* newp) { اگر (newp) سیٹ_Op1p(newp); }
    خالی سیٹ_NOp2p(Ast_گرہ* newp) { اگر (newp) سیٹ_Op2p(newp); }
    خالی سیٹ_NOp3p(Ast_گرہ* newp) { اگر (newp) سیٹ_Op3p(newp); }
    خالی سیٹ_NOp4p(Ast_گرہ* newp) { اگر (newp) سیٹ_Op4p(newp); }

    خالی جوڑو_NOp1p(Ast_گرہ* newp) { اگر (newp) جوڑو_Op1p(newp); }
    خالی جوڑو_NOp2p(Ast_گرہ* newp) { اگر (newp) جوڑو_Op2p(newp); }
    خالی جوڑو_NOp3p(Ast_گرہ* newp) { اگر (newp) جوڑو_Op3p(newp); }
    خالی جوڑو_NOp4p(Ast_گرہ* newp) { اگر (newp) جوڑو_Op4p(newp); }

    خالی prependNOp1p(Ast_گرہ* newp) { اگر (newp) prependOp1p(newp); }
    خالی prependNOp2p(Ast_گرہ* newp) { اگر (newp) prependOp2p(newp); }
    خالی prependNOp3p(Ast_گرہ* newp) { اگر (newp) prependOp3p(newp); }
    خالی prependNOp4p(Ast_گرہ* newp) { اگر (newp) prependOp4p(newp); }
    // clang-format on

    خالی clonep(Ast_گرہ* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    ساکن خالی clone_Clear_درخت() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    مجازی bool same_گرہ(ثابت Ast_گرہ*) ثابت { واپس true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    مجازی bool wouldBreakGen(ثابت Ast_گرہ* ثابت oldp,
                               ثابت Ast_گرہ* ثابت newp) ثابت
        = 0;  // Generated by 'astgen'

عوامی:
    // ACCESSORS
    VNType type() ثابت VL_MT_محفوظ { واپس m_type; }
    ثابت حرف* typeName() ثابت VL_MT_محفوظ { واپس type().ascii(); }  // See also prettyTypeName
    Ast_گرہ* nextp() ثابت VL_MT_STABLE { واپس m_nextp; }
    Ast_گرہ* backp() ثابت VL_MT_STABLE { واپس m_backp; }
    Ast_گرہ* abovep() ثابت;  // Get parent node above, only for list head and tail
    Ast_گرہ* aboveLoopp() ثابت;  // Get parent node above, may have performance issues as loops
    Ast_گرہ* lastp() ثابت {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        واپس m_headtailp;
    }
    Ast_گرہ* prevp() ثابت {  // Previous node in list, nullptr for head of list
        اگر (m_backp->m_nextp != this) واپس nullptr;
        واپس m_backp;
    }
    Ast_گرہ* op1p() ثابت VL_MT_STABLE { واپس m_op1p; }
    Ast_گرہ* op2p() ثابت VL_MT_STABLE { واپس m_op2p; }
    Ast_گرہ* op3p() ثابت VL_MT_STABLE { واپس m_op3p; }
    Ast_گرہ* op4p() ثابت VL_MT_STABLE { واپس m_op4p; }
    Ast_گرہ_DType* dtypep() ثابت VL_MT_STABLE { واپس m_dtypep; }
    Ast_گرہ* clonep() ثابت { واپس ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_گرہ* پہلا_Abovep() ثابت {  // Returns nullptr when second or later in list
        واپس ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ثابت VL_MT_محفوظ { واپس m_brokenState; }
    خالی brokenState(uint8_t قدر) { m_brokenState = قدر; }

    // Used by AstNode::broken()
    bool brokeExists() ثابت { واپس V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ثابت { واپس brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    مجازی خالی foreachLink(std::function<خالی(Ast_گرہ** linkpp, ثابت حرف* namep)> f) = 0;

#اگر defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    ساکن خالی* عامل new(سائز_t سائز);
    ساکن خالی عامل delete(خالی* obj, سائز_t سائز);
#اگر_ختم

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    ساکن constexpr صحیح INSTR_گنتی_BRANCH = 4;  // Branch
    ساکن constexpr صحیح INSTR_گنتی_CALL = INSTR_گنتی_BRANCH + 10;  // Subroutine call
    ساکن constexpr صحیح INSTR_گنتی_LD = 2;  // Load memory
    ساکن constexpr صحیح INSTR_گنتی_INT_MUL = 3;  // Integer multiply
    ساکن constexpr صحیح INSTR_گنتی_INT_DIV = 10;  // Integer divide
    ساکن constexpr صحیح INSTR_گنتی_DBL = 8;  // Convert or do float ops
    ساکن constexpr صحیح INSTR_گنتی_DBL_DIV = 40;  // Double divide
    ساکن constexpr صحیح INSTR_گنتی_DBL_TRIG = 200;  // Double trigonometric ops
    ساکن constexpr صحیح INSTR_گنتی_STR = 100;  // String ops
    ساکن constexpr صحیح INSTR_گنتی_وقت = INSTR_گنتی_CALL + 5;  // Determine simulation time
    ساکن constexpr صحیح INSTR_گنتی_PLI = 20;  // PLI routines
    ساکن constexpr صحیح INSTR_گنتی_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    مجازی string name() ثابت VL_MT_STABLE { واپس ""; }
    مجازی string origName() ثابت { واپس ""; }
    string prettyDehashOrigOrName() ثابت {
        واپس prettyName(VName::dehash(origName().خالی_خانے() ? name() : origName()));
    }
    مجازی خالی name(ثابت string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    مجازی خالی لیبل(ثابت string& متن) {}
    مجازی string لیبل() ثابت { واپس ""; }
    مجازی uint32_t decl_Token_عدد() ثابت { واپس 0; }
    مجازی خالی decl_Token_عدد_سیٹ_Min(uint32_t token_عدد) {}
    مجازی string verilogKwd() ثابت { واپس ""; }
    string nameProtect() ثابت VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() ثابت;  // origName with --protect-id applied
    string shortName() ثابت;  // Name with __PVT__ removed for concatenating scopes
    ساکن string dedotName(ثابت string& namein);  // Name with dots removed
    ساکن string nameNoArray(ثابت string& namein);  // Name with any array index removed
    ساکن string prettyName(ثابت string& namein) VL_خالص;  // Name for printing out to the user
    ساکن string vpiName(ثابت string& namein);  // Name for vpi access
    ساکن string prettyNameQ(ثابت string& namein) {  // Quoted pretty name (for errors)
        واپس "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    ساکن string encodeName(ثابت string& namein);
    ساکن string encode_نمبر(int64_t عدد);  // Encode number into internal C representation
    ساکن string vcdName(ثابت string& namein);  // Name for printing out to vcd files
    string prettyName() ثابت { واپس prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    مجازی string pretty_Name_پیغام() ثابت { واپس prettyName(); }
    string prettyNameQ() ثابت { واپس "'"s + pretty_Name_پیغام() + "'"; }  // Quoted, for messages
    string verilogName() ثابت { واپس vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ثابت;
    مجازی string prettyOperatorName() ثابت { واپس "operator " + prettyTypeName(); }
    File_سطر* fileline() ثابت VL_MT_محفوظ { واپس m_fileline; }
    خالی fileline(File_سطر* fl) { m_fileline = fl; }
    اندرونی bool width1() ثابت;
    اندرونی صحیح چوڑائی_Instrs() ثابت;
    bool did_چوڑائی() ثابت { واپس m_flags.did_چوڑائی; }
    خالی did_چوڑائی(bool flag) { m_flags.did_چوڑائی = flag; }
    bool did_چوڑائی_And_سیٹ() {
        اگر (did_چوڑائی()) واپس true;
        did_چوڑائی(true);
        واپس false;
    }
    bool doing_چوڑائی() ثابت { واپس m_flags.doing_چوڑائی; }
    خالی doing_چوڑائی(bool flag) { m_flags.doing_چوڑائی = flag; }
    bool protect() ثابت VL_MT_محفوظ { واپس m_flags.protect; }
    خالی protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    اندرونی صحیح چوڑائی() ثابت VL_MT_STABLE;
    اندرونی صحیح چوڑائی_Min() ثابت VL_MT_STABLE;
    صحیح چوڑائی_Min_V() ثابت {
        واپس v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::VERILOG_چوڑائی ? چوڑائی_Min() : چوڑائی();
    }
    صحیح چوڑائی_الفاظ() ثابت { واپس VL_الفاظ_I(چوڑائی()); }
    bool isQuad() ثابت VL_MT_STABLE { واپس (چوڑائی() > VL_IDATASIZE && چوڑائی() <= VL_QUADSIZE); }
    bool isWide() ثابت VL_MT_STABLE { واپس (چوڑائی() > VL_QUADSIZE); }
    اندرونی bool isCHandle() ثابت VL_MT_STABLE;
    اندرونی bool isDouble() ثابت VL_MT_STABLE;
    اندرونی bool isSigned() ثابت VL_MT_STABLE;
    اندرونی bool isString() ثابت VL_MT_STABLE;
    اندرونی bool isEvent() ثابت VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        واپس ((m_user1Cnt == VNUser1_اندر_Use::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_گرہ* user1p() ثابت VL_MT_STABLE { واپس user1u().مقصد_Nodep(); }
    خالی user1u(ثابت VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1_اندر_Use::s_userCntGbl; }
    خالی user1p(خالی* userp) { user1u(VNUser{userp}); }
    خالی user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ثابت { واپس user1u().مقصد_UQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ثابت uint64_t v = user1(); user1(v + val); واپس v; }
    uint64_t user1Or(uint64_t val) { ثابت uint64_t v = user1(); user1(v | val); واپس v; }
    uint64_t user1_سیٹ_Once() { ثابت uint64_t v = user1(); اگر (!v) user1(1); واپس v; }  // Better for cache than user1Inc()
    ساکن خالی user1_Clear_درخت() { VNUser1_اندر_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        واپس ((m_user2Cnt == VNUser2_اندر_Use::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_گرہ* user2p() ثابت VL_MT_STABLE { واپس user2u().مقصد_Nodep(); }
    خالی user2u(ثابت VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2_اندر_Use::s_userCntGbl; }
    خالی user2p(خالی* userp) { user2u(VNUser{userp}); }
    خالی user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ثابت { واپس user2u().مقصد_UQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ثابت uint64_t v = user2(); user2(v + val); واپس v; }
    uint64_t user2Or(uint64_t val) { ثابت uint64_t v = user2(); user2(v | val); واپس v; }
    uint64_t user2_سیٹ_Once() { ثابت uint64_t v = user2(); اگر (!v) user2(1); واپس v; }  // Better for cache than user2Inc()
    ساکن خالی user2_Clear_درخت() { VNUser2_اندر_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        واپس ((m_user3Cnt == VNUser3_اندر_Use::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_گرہ* user3p() ثابت VL_MT_STABLE { واپس user3u().مقصد_Nodep(); }
    خالی user3u(ثابت VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3_اندر_Use::s_userCntGbl; }
    خالی user3p(خالی* userp) { user3u(VNUser{userp}); }
    خالی user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ثابت { واپس user3u().مقصد_UQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ثابت uint64_t v = user3(); user3(v + val); واپس v; }
    uint64_t user3Or(uint64_t val) { ثابت uint64_t v = user3(); user3(v | val); واپس v; }
    uint64_t user3_سیٹ_Once() { ثابت uint64_t v = user3(); اگر (!v) user3(1); واپس v; }  // Better for cache than user3Inc()
    ساکن خالی user3_Clear_درخت() { VNUser3_اندر_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        واپس ((m_user4Cnt == VNUser4_اندر_Use::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_گرہ* user4p() ثابت VL_MT_STABLE { واپس user4u().مقصد_Nodep(); }
    خالی user4u(ثابت VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4_اندر_Use::s_userCntGbl; }
    خالی user4p(خالی* userp) { user4u(VNUser{userp}); }
    خالی user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ثابت { واپس user4u().مقصد_UQuad(); }
    uint64_t user4Or(uint64_t val) { ثابت uint64_t v = user4(); user4(v | val); واپس v; }
    uint64_t user4Inc(uint64_t val = 1) { ثابت uint64_t v = user4(); user4(v + val); واپس v; }
    uint64_t user4_سیٹ_Once() { ثابت uint64_t v = user4(); اگر (!v) user4(1); واپس v; }  // Better for cache than user4Inc()
    ساکن خالی user4_Clear_درخت() { VNUser4_اندر_Use::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#اگر_تعریف_شدہ VL_DEBUG
    uint64_t edit_گنتی() ثابت { واپس m_edit_گنتی; }
    خالی edit_گنتی_Inc() {
        m_edit_گنتی = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_درخت();  // Any edit clears all caching
    }
#ورنہ
    خالی edit_گنتی_Inc() { ++s_editCntGbl; }
#اگر_ختم
    ساکن uint64_t edit_گنتی_آخری() VL_MT_محفوظ { واپس s_edit_Cnt_آخری; }
    ساکن uint64_t edit_گنتی_Gbl() VL_MT_محفوظ { واپس s_editCntGbl; }
    ساکن خالی edit_گنتی_سیٹ_آخری() { s_edit_Cnt_آخری = edit_گنتی_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_قطار_ڈالو_Self_Stmt();
    اندرونی bool is_Class_Handle_قدر() ثابت;
    اندرونی bool isNull() ثابت;
    اندرونی bool isZero() ثابت;
    اندرونی bool isOne() ثابت;
    اندرونی bool isNeqZero() ثابت;
    اندرونی bool isAllOnes() ثابت;
    اندرونی bool isAllOnesV() ثابت;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    خالی dtypep(Ast_گرہ_DType* nodep) {
        اگر (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_گنتی_Inc();
        }
    }
    خالی dtypeFrom(ثابت Ast_گرہ* fromp) {
        اگر (fromp) dtypep(fromp->dtypep());
    }
    خالی dtypeChgSigned(bool flag = true);
    خالی dtype_Chg_چوڑائی(صحیح چوڑائی, صحیح چوڑائی_Min);
    خالی dtype_Chg_چوڑائی_Signed(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric);
    خالی dtype_سیٹ_ثنائی_ہندسہ_Unsized(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) {
        dtypep(ڈھونڈو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی_Min, numeric));
    }
    خالی dtype_سیٹ_ثنائی_ہندسہ_Sized(صحیح چوڑائی, VSigning numeric) {
        dtypep(ڈھونڈو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی, numeric));  // Since sized, widthMin is width
    }
    خالی dtype_سیٹ_Logic_Unsized(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) {
        dtypep(ڈھونڈو_Logic_DType(چوڑائی, چوڑائی_Min, numeric));
    }
    خالی dtype_سیٹ_Logic_Sized(صحیح چوڑائی, VSigning numeric) {
        dtypep(ڈھونڈو_Logic_DType(چوڑائی, چوڑائی, numeric));  // Since sized, widthMin is width
    }
    خالی dtype_سیٹ_Logic2_State_Unsized(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) {
        dtypep(ڈھونڈو_Logic2_State_DType(چوڑائی, چوڑائی_Min, numeric));
    }
    خالی dtype_سیٹ_Logic2_State_Sized(صحیح چوڑائی, VSigning numeric) {
        dtypep(ڈھونڈو_Logic2_State_DType(چوڑائی, چوڑائی, numeric));  // Since sized, widthMin is width
    }
    خالی dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric, bool isFourstate) {
        dtypep(ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(چوڑائی, چوڑائی_Min, numeric, isFourstate));
    }
    خالی dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Sized(صحیح چوڑائی, VSigning numeric, bool isFourstate) {
        dtypep(ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(چوڑائی, چوڑائی, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    خالی dtype_سیٹ_ثنائی_ہندسہ() { dtypep(ڈھونڈو_ثنائی_ہندسہ_DType()); }
    خالی dtype_سیٹ_Logic() { dtypep(ڈھونڈو_Logic_DType()); }
    خالی dtype_سیٹ_Logic2_State() { dtypep(ڈھونڈو_Logic2_State_DType()); }
    خالی dtype_سیٹ_Double() { dtypep(ڈھونڈو_Double_DType()); }
    خالی dtype_سیٹ_Int() { dtypep(ڈھونڈو_Int_DType()); }
    خالی dtype_سیٹ_Integer() { dtypep(ڈھونڈو_Integer_DType()); }
    خالی dtype_سیٹ_Integer2_State() { dtypep(ڈھونڈو_Integer2_State_DType()); }
    خالی dtype_سیٹ_String() { dtypep(ڈھونڈو_String_DType()); }
    خالی dtype_سیٹ_UInt32() { dtypep(ڈھونڈو_UInt32_DType()); }  // Twostate
    خالی dtype_سیٹ_UInt64() { dtypep(ڈھونڈو_UInt64_DType()); }  // Twostate
    خالی dtype_سیٹ_وقت() { dtypep(ڈھونڈو_وقت_DType()); }
    خالی dtype_سیٹ_Time2_State() { dtypep(ڈھونڈو_Time2_State_DType()); }
    خالی dtype_سیٹ_خالی_خانے_قطار() { dtypep(ڈھونڈو_خالی_خانے_قطار_DType()); }
    خالی dtype_سیٹ_دھارا() { dtypep(ڈھونڈو_دھارا_DType()); }
    خالی dtype_سیٹ_Void() { dtypep(ڈھونڈو_Void_DType()); }
    خالی dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(bool isFourstate) {
        dtypep(isFourstate ? ڈھونڈو_Logic_DType() : ڈھونڈو_ثنائی_ہندسہ_DType());
    }

    // Data type locators
    Ast_گرہ_DType* ڈھونڈو_ثنائی_ہندسہ_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::ثنائی_ہندسہ); }
    Ast_گرہ_DType* ڈھونڈو_Logic_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::LOGIC); }
    Ast_گرہ_DType* ڈھونڈو_Null_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::NULLTYPE); }
    Ast_گرہ_DType* ڈھونڈو_Logic2_State_DType() ثابت {
        واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_گرہ_DType* ڈھونڈو_Double_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::DOUBLE); }
    Ast_گرہ_DType* ڈھونڈو_Int_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::INT); }
    Ast_گرہ_DType* ڈھونڈو_Integer_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::INTEGER); }
    Ast_گرہ_DType* ڈھونڈو_Integer2_State_DType() ثابت {
        واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_گرہ_DType* ڈھونڈو_String_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::STRING); }
    Ast_گرہ_DType* ڈھونڈو_Signed8_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::BYTE); }
    Ast_گرہ_DType* ڈھونڈو_UInt32_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT32); }
    Ast_گرہ_DType* ڈھونڈو_UInt64_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::UINT64); }
    Ast_گرہ_DType* ڈھونڈو_وقت_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::وقت); }
    Ast_گرہ_DType* ڈھونڈو_Time2_State_DType() ثابت {
        واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_گرہ_DType* ڈھونڈو_CHandle_DType() ثابت { واپس ڈھونڈو_Basic_DType(VBasicDTypeKwd::CHANDLE); }
    Ast_گرہ_DType* ڈھونڈو_Constraint_Ref_DType() ثابت;
    Ast_گرہ_DType* ڈھونڈو_خالی_خانے_قطار_DType() ثابت;
    Ast_گرہ_DType* ڈھونڈو_قطار_اشاریہ_DType() ثابت;
    Ast_گرہ_DType* ڈھونڈو_دھارا_DType() ثابت;
    Ast_گرہ_DType* ڈھونڈو_Void_DType() ثابت;
    Ast_گرہ_DType* ڈھونڈو_ثنائی_ہندسہ_DType(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) ثابت;
    Ast_گرہ_DType* ڈھونڈو_Logic_DType(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) ثابت;
    Ast_گرہ_DType* ڈھونڈو_Logic2_State_DType(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) ثابت;
    Ast_گرہ_DType* ڈھونڈو_ثنائی_ہندسہ_Or_Logic_DType(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric,
                                      bool isFourstate) ثابت {
        واپس isFourstate ? ڈھونڈو_Logic_DType(چوڑائی, چوڑائی_Min, numeric)
                           : ڈھونڈو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی_Min, numeric);
    }
    Ast_گرہ_DType* ڈھونڈو_ثنائی_ہندسہ_Or_Logic_Range_DType(ثابت VNumRange& range, ثابت صحیح چوڑائی_Min,
                                           ثابت VSigning numeric,
                                           ثابت bool isFourstate) ثابت VL_MT_STABLE {
        واپس isFourstate ? ڈھونڈو_Logic_Range_DType(range, چوڑائی_Min, numeric)
                           : ڈھونڈو_ثنائی_ہندسہ_Range_DType(range, چوڑائی_Min, numeric);
    }
    Ast_گرہ_DType* ڈھونڈو_Logic_Range_DType(ثابت VNumRange& range, صحیح چوڑائی_Min,
                                      VSigning numeric) ثابت VL_MT_STABLE;
    Ast_گرہ_DType* ڈھونڈو_ثنائی_ہندسہ_Range_DType(ثابت VNumRange& range, صحیح چوڑائی_Min,
                                    VSigning numeric) ثابت VL_MT_STABLE;
    Ast_گرہ_DType* ڈھونڈو_Basic_DType(VBasicDTypeKwd kwd) ثابت;
    ساکن AstBasicDType* ڈھونڈو_داخل_کرو_Same_DType(AstBasicDType* nodep);

    ساکن VCastable computeCastable(ثابت Ast_گرہ_DType* مقصد_Dtp, ثابت Ast_گرہ_DType* fromDtp,
                                     ثابت Ast_گرہ* fromConstp, bool checkIfaceArgCompat = false);
    ساکن Ast_گرہ_DType* getCommonClassTypep(Ast_گرہ* nodep1, Ast_گرہ* nodep2);

    // METHODS - dump and error
    خالی v3error_انتہا(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex);
    خالی v3error_انتہا_Fatal(ثابت std::ostringstream& str) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
        واپس fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ثابت { واپس fileline()->warnContextSecondary(); }
    string warn_مزید() ثابت VL_REQUIRES(V3Error::s().m_mutex) { واپس fileline()->warn_مزید(); }
    string warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) { واپس fileline()->warnOther(); }

    مجازی خالی dump(std::ostream& str = std::cout) ثابت;
    ساکن حرف* dump_درخت_Json_Gdb(ثابت Ast_گرہ* nodep);  // For GDB only, free()d by caller
    ساکن حرف* dump_درخت_Json_Gdb(ثابت حرف* str);  // For GDB only, free()d by caller
    ساکن حرف* dump_درخت_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    ساکن خالی dumpGdb(ثابت Ast_گرہ* nodep);  // For GDB only
    خالی dump_Gdb_سرخی() ثابت;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    سانچہ <typename T_گرہ_نتیجہ, typename T_گرہ_اگلا>
    ساکن T_گرہ_نتیجہ* جوڑو_اگلا(T_گرہ_نتیجہ* nodep, T_گرہ_اگلا* newp) {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ_نتیجہ>::قدر,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_بنیاد_کا<T_گرہ_نتیجہ, T_گرہ_اگلا>::قدر,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        اگر (!newp) واپس nodep;
        واپس static_cast<T_گرہ_نتیجہ*>(جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, newp));
    }
    سانچہ <typename T_گرہ_نتیجہ, typename T_گرہ_اگلا>
    ساکن T_گرہ_نتیجہ* جوڑو_اگلا_Null(T_گرہ_نتیجہ* nodep, T_گرہ_اگلا* newp) {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ_نتیجہ>::قدر,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_بنیاد_کا<T_گرہ_نتیجہ, T_گرہ_اگلا>::قدر,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        اگر (!newp) واپس nodep;
        اگر (!nodep) واپس newp;
        واپس static_cast<T_گرہ_نتیجہ*>(جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, newp));
    }
    اندرونی Ast_گرہ* جوڑو_اگلا(Ast_گرہ* newp);
    خالی جوڑو_اگلا_یہاں(Ast_گرہ* newp);  // Insert newp at this->nextp
    خالی جوڑو_یہاں_This_As_اگلا(Ast_گرہ* newp);  // Adds at old place of this, this becomes next
    خالی replaceWith(Ast_گرہ* newp);  // Replace current node in tree with new node
    خالی replaceWithKeepDType(Ast_گرہ* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_گرہ* unlink_Fr_عقب(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_گرہ* unlink_Fr_عقب_With_اگلا(VNRelinker* linkerp = nullptr);
    خالی relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    خالی clone_Relink_گرہ() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_گرہ* clone_درخت(bool clone_اگلا_Link,
                       bool need_خالص = false);  // Not const, as sets clonep() on original nodep
    Ast_گرہ* clone_درخت_خالص(bool clone_اگلا_Link) { واپس clone_درخت(clone_اگلا_Link, true); }
    bool gate_درخت() { واپس gate_درخت_Iter(); }  // Is tree isGateOptimizable?
    اندرونی bool same_درخت(ثابت Ast_گرہ* node2p) ثابت;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    اندرونی bool same_Gate_درخت(ثابت Ast_گرہ* node2p) ثابت;
    خالی delete_درخت();  // Always deletes the next link
    خالی check_درخت() ثابت VL_MT_STABLE {
        اگر (v3Global.opt.debugCheck()) check_درخت_Iter(backp());
    }
    خالی checkIter() ثابت;
    خالی dumpPtrs(std::ostream& os = std::cout) ثابت;
    خالی dump_درخت(std::ostream& os = std::cout, ثابت string& indent = "    ",
                  صحیح max_گہرائی = 0) ثابت;
    خالی dump_درخت(ثابت string& indent, صحیح max_گہرائی = 0) ثابت {
        dump_درخت(std::cout, indent, max_گہرائی);
    }
    ساکن خالی dump_درخت_Gdb(ثابت Ast_گرہ* nodep);  // For GDB only
    خالی dump_درخت_And_اگلا(std::ostream& os = std::cout, ثابت string& indent = "    ",
                         صحیح max_گہرائی = 0) ثابت;
    خالی dump_درخت_File(ثابت string& filename, bool doDump = true);
    ساکن خالی dump_درخت_File_Gdb(ثابت Ast_گرہ* nodep, ثابت حرف* filenamep = nullptr);
    خالی dump_درخت_Dot(std::ostream& os = std::cout) ثابت;
    خالی dump_درخت_Dot_File(ثابت string& filename, bool doDump = true);
    مجازی خالی dumpJson(std::ostream& os) ثابت { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    خالی dumpJsonGen(std::ostream& os) ثابت {};
    مجازی خالی dump_درخت_Json_Op_Gen(std::ostream& os, ثابت string& indent) ثابت {};
    خالی dump_درخت_Json(std::ostream& os, ثابت string& indent = "") ثابت;
    خالی dump_درخت_Json_File(ثابت string& filename, bool doDump = true);
    ساکن خالی dumpJsonMetaFileGdb(ثابت حرف* filename);
    ساکن خالی dumpJsonMetaFile(ثابت string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    ساکن std::string گرہ_Addr(ثابت Ast_گرہ* nodep) {
        واپس v3Global.opt.dump_درخت_Addrids() ? v3Global.ptr_مقصد_Id(nodep) : cvt_مقصد_Hex(nodep);
    }

    // METHODS - static advancement
    ساکن Ast_گرہ* afterCommentp(Ast_گرہ* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        جب (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        واپس nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    مجازی bool isBrancher() ثابت { واپس false; }
    // Else a AstTime etc that can't be pushed out
    مجازی bool isGateOptimizable() ثابت { واپس !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    مجازی bool isGateDedupable() ثابت { واپس isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    مجازی bool is_اظہاریہ_Coverage_Eligible() ثابت { واپس isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    مجازی bool isOutputter() { واپس false; }
    // Else a AstTime etc which output can't be predicted from input
    مجازی bool isPredictOptimizable() ثابت { واپس !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    مجازی bool is_خالص() { واپس true; }
    // Iff isPure on current node and any nextp()
    bool is_خالص_And_اگلا() { واپس is_خالص() && (!nextp() || nextp()->is_خالص()); }
    // An event control, delay, wait, etc.
    مجازی bool isTimingControl() ثابت { واپس false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    مجازی bool isUnlikely() ثابت { واپس false; }
    // Is an IEEE system function (versus internally-generated)
    مجازی bool isSystemFunc() ثابت { واپس false; }
    مجازی صحیح instr_گنتی() ثابت { واپس 0; }
    // Iff node is identical to another node
    مجازی bool isSame(ثابت Ast_گرہ* samep) ثابت {
        واپس type() == samep->type() && same_گرہ(samep);
    }
    // Iff has a data type; dtype() must be non-null
    مجازی bool hasDType() ثابت VL_MT_محفوظ { واپس false; }
    // Iff has a non-null childDTypep(), as generic node function
    مجازی Ast_گرہ_DType* get_بچہ_DTypep() ثابت { واپس nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    مجازی Ast_گرہ_DType* getChild2DTypep() ثابت { واپس nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    مجازی bool maybe_Pointed_مقصد() ثابت VL_MT_محفوظ { واپس false; }
    // Don't reclaim this node in V3Dead
    مجازی bool undead() ثابت { واپس false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    مجازی ثابت حرف* broken() ثابت { واپس nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    مجازی ثابت حرف* brokenGen() ثابت = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ثابت Ast_گرہ* ثابت newp) ثابت { واپس backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    مجازی خالی accept(VNVisitorConst& v) = 0;

محفوظ:
    // All VNVisitor related functions are called as methods off the visitor
    دوست class VNVisitor;
    دوست class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    خالی iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    خالی iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    خالی iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    خالی iterate_And_اگلا(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    خالی iterate_And_اگلا_Const(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_گرہ* iterateSubtreeReturnEdits(VNVisitor& v);

    ساکن خالی dump_Json_عدد(std::ostream& os, ثابت std::string& name, int64_t val);
    ساکن خالی dumpJsonBoolIf(std::ostream& os, ثابت std::string& name, bool val);
    ساکن خالی dumpJsonStr(std::ostream& os, ثابت std::string& name, ثابت std::string& val);
    ساکن خالی dumpJsonPtr(std::ostream& os, ثابت std::string& name, ثابت Ast_گرہ* ثابت valp);

محفوظ:
    خالی iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    سانچہ <typename T>
    اندرونی ساکن bool نجی_Type_Test(ثابت Ast_گرہ* nodep);

    // For internal use only.
    سانچہ <typename T_ہدف_Type, typename T_DeclType>
    constexpr ساکن bool uselessCast() VL_خالص {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_اشارہ_گر<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        واپس std::is_بنیاد_کا<T_ہدف_Type, NonCV>::قدر;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    سانچہ <typename T_ہدف_Type, typename T_DeclType>
    constexpr ساکن bool impossibleCast() VL_خالص {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_اشارہ_گر<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        واپس !std::is_بنیاد_کا<NonCV, T_ہدف_Type>::قدر;
    }  // LCOV_EXCL_STOP

عوامی:
    // For use via the VN_IS macro only, or in templated code
    سانچہ <typename T_ہدف_Type, typename T_گرہ>
    ساکن bool is(ثابت T_گرہ* nodep) VL_MT_محفوظ {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        واپس nodep && نجی_Type_Test<T_ہدف_Type>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    سانچہ <typename T_ہدف_Type, typename T_گرہ>
    ساکن T_ہدف_Type* cast(T_گرہ* nodep) VL_MT_محفوظ {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        واپس nodep && نجی_Type_Test<T_ہدف_Type>(nodep)
                   ? reinterpret_cast<T_ہدف_Type*>(nodep)
                   : nullptr;
    }
    سانچہ <typename T_ہدف_Type, typename T_گرہ>
    ساکن ثابت T_ہدف_Type* cast(ثابت T_گرہ* nodep) VL_MT_محفوظ {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        واپس nodep && نجی_Type_Test<T_ہدف_Type>(nodep)
                   ? reinterpret_cast<ثابت T_ہدف_Type*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    سانچہ <typename T_ہدف_Type, typename T_گرہ>
    ساکن T_ہدف_Type* as(T_گرہ* nodep) VL_خالص {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || نجی_Type_Test<T_ہدف_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        واپس unsafe_نجی_As<T_ہدف_Type, T_گرہ>(nodep);
    }
    سانچہ <typename T_ہدف_Type, typename T_گرہ>
    ساکن ثابت T_ہدف_Type* as(ثابت T_گرہ* nodep) VL_خالص {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || نجی_Type_Test<T_ہدف_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        واپس unsafe_نجی_As<T_ہدف_Type, T_گرہ>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    سانچہ <typename T_ہدف_Type, typename T_گرہ>
    ساکن T_ہدف_Type* unsafe_نجی_As(T_گرہ* nodep) VL_خالص {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        واپس reinterpret_cast<T_ہدف_Type*>(nodep);
    }
    سانچہ <typename T_ہدف_Type, typename T_گرہ>
    ساکن ثابت T_ہدف_Type* unsafe_نجی_As(ثابت T_گرہ* nodep) VL_خالص {
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_ہدف_Type, T_گرہ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        واپس reinterpret_cast<ثابت T_ہدف_Type*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    سانچہ <typename T_گرہ>
    ساکن bool mayBeUnder(ثابت Ast_گرہ* nodep) {
        static_assert(!std::is_const<T_گرہ>::قدر,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        واپس true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    سانچہ <typename T_گرہ>
    ساکن constexpr bool is_برگ() {
        static_assert(!std::is_const<T_گرہ>::قدر,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        واپس false;
    }

نجی:
    // Using std::conditional for const correctness in the public 'foreach' functions
    سانچہ <typename T_Arg>
    using Const_Correct_Ast_گرہ =
        typename std::conditional<std::is_const<T_Arg>::قدر, ثابت Ast_گرہ, Ast_گرہ>::type;

    سانچہ <typename T_Arg, typename T_Callable>
    اندرونی ساکن خالی foreachImpl(Const_Correct_Ast_گرہ<T_Arg>* nodep, ثابت T_Callable& f,
                                   bool دورہ_اگلا);

    سانچہ <typename T_Arg, bool N_Default, typename T_Callable>
    اندرونی ساکن bool predicateImpl(Const_Correct_Ast_گرہ<T_Arg>* nodep, ثابت T_Callable& p,
                                     bool دورہ_اگلا);

عوامی:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    سانچہ <typename T_Callable>
    خالی foreach(T_Callable&& f) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_گرہ*>::قدر
                          && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_گرہ>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    سانچہ <typename T_Callable>
    خالی foreach(T_Callable&& f) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ثابت T_گرہ*>::قدر
                && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ثابت T_گرہ>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    سانچہ <typename T_Callable>
    خالی foreach_And_اگلا(T_Callable&& f) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_گرہ*>::قدر
                          && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_گرہ>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    سانچہ <typename T_Callable>
    خالی foreach_And_اگلا(T_Callable&& f) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ثابت T_گرہ*>::قدر
                && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ثابت T_گرہ>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    سانچہ <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_گرہ*>::قدر
                          && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        واپس predicateImpl<T_گرہ, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    سانچہ <typename T_Callable>
    bool exists(T_Callable&& p) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_گرہ*>::قدر
                          && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        واپس predicateImpl<ثابت T_گرہ, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    سانچہ <typename T_Callable>
    bool exists_And_اگلا(T_Callable&& p) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_گرہ*>::قدر
                          && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        واپس predicateImpl<T_گرہ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    سانچہ <typename T_Callable>
    bool exists_And_اگلا(T_Callable&& p) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_گرہ*>::قدر
                          && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        واپس predicateImpl<ثابت T_گرہ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    سانچہ <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_گرہ*>::قدر
                          && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        واپس predicateImpl<T_گرہ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    سانچہ <typename T_Callable>
    bool forall(T_Callable&& p) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_گرہ*>::قدر
                          && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        واپس predicateImpl<ثابت T_گرہ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    صحیح گرہ_گنتی() ثابت {
        // TODO: this should really return size_t, but need to fix use sites
        صحیح گنتی = 0;
        this->foreach([&گنتی](ثابت Ast_گرہ*) { ++گنتی; });
        واپس گنتی;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(صحیح حد_بندی) ثابت {
        صحیح گنتی = 0;
        واپس this->exists([&گنتی, حد_بندی](ثابت Ast_گرہ*) { واپس ++گنتی > حد_بندی; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
سانچہ <>
Ast_گرہ* Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(Ast_گرہ* nodep, Ast_گرہ* newp);

// Inline method implementations
Ast_گرہ* Ast_گرہ::جوڑو_اگلا(Ast_گرہ* newp) { واپس جوڑو_اگلا(this, newp); }

// Specializations of privateTypeTest
#شامل "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
سانچہ <>
اندرونی bool Ast_گرہ::mayBeUnder<AstCell>(ثابت Ast_گرہ* nodep) {
    واپس !VN_IS(nodep, گرہ_Stmt) && !VN_IS(nodep, گرہ_اظہاریہ);
}
سانچہ <>
اندرونی bool Ast_گرہ::mayBeUnder<Ast_گرہ_تفویض>(ثابت Ast_گرہ* nodep) {
    واپس !VN_IS(nodep, گرہ_اظہاریہ);
}
سانچہ <>
اندرونی bool Ast_گرہ::mayBeUnder<AstVarScope>(ثابت Ast_گرہ* nodep) {
    اگر (VN_IS(nodep, VarScope)) واپس false;  // Should not nest
    اگر (VN_IS(nodep, Var)) واپس false;
    اگر (VN_IS(nodep, فعال)) واپس false;
    اگر (VN_IS(nodep, گرہ_Stmt)) واپس false;
    اگر (VN_IS(nodep, گرہ_اظہاریہ)) واپس false;
    واپس true;
}
سانچہ <>
اندرونی bool Ast_گرہ::mayBeUnder<Ast_Exec_گراف>(ثابت Ast_گرہ* nodep) {
    اگر (VN_IS(nodep, Exec_گراف)) واپس false;  // Should not nest
    اگر (VN_IS(nodep, گرہ_Stmt)) واپس false;  // Should be directly under CFunc
    واپس true;
}
سانچہ <>
اندرونی bool Ast_گرہ::mayBeUnder<Ast_فعال>(ثابت Ast_گرہ* nodep) {
    واپس !VN_IS(nodep, فعال);  // AstActives do not nest
}
سانچہ <>
اندرونی bool Ast_گرہ::mayBeUnder<AstScope>(ثابت Ast_گرہ* nodep) {
    واپس !VN_IS(nodep, Scope);  // AstScopes do not nest
}
سانچہ <>
اندرونی bool Ast_گرہ::mayBeUnder<Ast_Sen_درخت>(ثابت Ast_گرہ* nodep) {
    واپس !VN_IS(nodep, Sen_درخت);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
سانچہ <>
constexpr bool Ast_گرہ::is_برگ<Ast_گرہ_Var_Ref>() {
    واپس true;
}
سانچہ <>
constexpr bool Ast_گرہ::is_برگ<AstVarRef>() {
    واپس true;
}
سانچہ <>
constexpr bool Ast_گرہ::is_برگ<AstVarXRef>() {
    واپس true;
}

// foreach implementation
سانچہ <typename T_Arg, typename T_Callable>
خالی Ast_گرہ::foreachImpl(Const_Correct_Ast_گرہ<T_Arg>* nodep, ثابت T_Callable& f, bool دورہ_اگلا) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using گرہ_2 = Const_Correct_Ast_گرہ<T_Arg>;

    // We prefetch this far into the stack
    constexpr صحیح Prefetch_فاصلہ = 2;
    // We push max 5 items per iteration
    constexpr سائز_t Max_Pushes_فی_Iteration = 5;
    // Initial stack sie
    constexpr سائز_t Initial_ڈھیر_سائز = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    سائز_t ڈھیر_سائز = Initial_ڈھیر_سائز;
    گرہ_2* اندر_سطر_ڈھیر[Initial_ڈھیر_سائز];  // In-line initial stack storage
    std::unique_ptr<گرہ_2*[]> برآمد_کا_سطر_Stackp{nullptr};  // In case we need more, we will allocate
    گرہ_2** basep = اندر_سطر_ڈھیر + Prefetch_فاصلہ;  // Pointer to base of stack
    گرہ_2** topp = basep;  // Pointer to top of stack
    گرہ_2** limp = اندر_سطر_ڈھیر + ڈھیر_سائز - Max_Pushes_فی_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    برائے (صحیح i = -Prefetch_فاصلہ; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ثابت خودکار دورہ = [&](گرہ_2* currp) {
        // Type test this node
        اگر (Ast_گرہ::نجی_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            اگر VL_CONSTEXPR_CXX17 (is_برگ<T_Arg_NonConst>()) واپس;
        }

        // Enqueue children for traversal, unless futile
        اگر (mayBeUnder<T_Arg_NonConst>(currp)) {
            اگر (Ast_گرہ* ثابت op4p = currp->op4p()) *topp++ = op4p;
            اگر (Ast_گرہ* ثابت op3p = currp->op3p()) *topp++ = op3p;
            اگر (Ast_گرہ* ثابت op2p = currp->op2p()) *topp++ = op2p;
            اگر (Ast_گرہ* ثابت op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    اگر (دورہ_اگلا && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    دورہ(nodep);

    // Visit the rest of the tree
    جب (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        گرہ_2* ثابت headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_فاصلہ]);

        // Ensure we have stack space for nextp and the 4 children
        اگر (VL_UNLIKELY(topp >= limp)) {
            ثابت ptrdiff_t occupancy = topp - basep;
            {
                ثابت سائز_t new_ڈھیر_سائز = ڈھیر_سائز * 2;
                گرہ_2** ثابت newStackp = new گرہ_2*[new_ڈھیر_سائز];
                std::حافظہ_نقل(newStackp, basep - Prefetch_فاصلہ,
                            حجم(گرہ_2**) * (occupancy + Prefetch_فاصلہ));
                ڈھیر_سائز = new_ڈھیر_سائز;
                برآمد_کا_سطر_Stackp.reset(newStackp);
            }
            basep = برآمد_کا_سطر_Stackp.get() + Prefetch_فاصلہ;
            topp = basep + occupancy;
            limp = برآمد_کا_سطر_Stackp.get() + ڈھیر_سائز - Max_Pushes_فی_Iteration;
        }

        // Enqueue the next node
        اگر (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        دورہ(headp);
    }
}

// predicate implementation
سانچہ <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_گرہ::predicateImpl(Const_Correct_Ast_گرہ<T_Arg>* nodep, ثابت T_Callable& p,
                            bool دورہ_اگلا) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using گرہ_2 = Const_Correct_Ast_گرہ<T_Arg>;

    // We prefetch this far into the stack
    constexpr صحیح Prefetch_فاصلہ = 2;
    // We push max 5 items per iteration
    constexpr سائز_t Max_Pushes_فی_Iteration = 5;
    // Initial stack sie
    constexpr سائز_t Initial_ڈھیر_سائز = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    سائز_t ڈھیر_سائز = Initial_ڈھیر_سائز;
    گرہ_2* اندر_سطر_ڈھیر[Initial_ڈھیر_سائز];  // In-line initial stack storage
    std::unique_ptr<گرہ_2*[]> برآمد_کا_سطر_Stackp{nullptr};  // In case we need more, we will allocate
    گرہ_2** basep = اندر_سطر_ڈھیر + Prefetch_فاصلہ;  // Pointer to base of stack
    گرہ_2** topp = basep;  // Pointer to top of stack
    گرہ_2** limp = اندر_سطر_ڈھیر + ڈھیر_سائز - Max_Pushes_فی_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    برائے (صحیح i = -Prefetch_فاصلہ; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ثابت خودکار دورہ = [&](گرہ_2* currp) {
        // Type test this node
        اگر (Ast_گرہ::نجی_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            اگر (p(static_cast<T_Arg*>(currp)) != N_Default) واپس true;
            // Short circuit if iterating leaf nodes
            اگر VL_CONSTEXPR_CXX17 (is_برگ<T_Arg_NonConst>()) واپس false;
        }

        // Enqueue children for traversal, unless futile
        اگر (mayBeUnder<T_Arg_NonConst>(currp)) {
            اگر (Ast_گرہ* ثابت op4p = currp->op4p()) *topp++ = op4p;
            اگر (Ast_گرہ* ثابت op3p = currp->op3p()) *topp++ = op3p;
            اگر (Ast_گرہ* ثابت op2p = currp->op2p()) *topp++ = op2p;
            اگر (Ast_گرہ* ثابت op1p = currp->op1p()) *topp++ = op1p;
        }

        واپس false;
    };

    // Enqueue the next of the root node, if required
    اگر (دورہ_اگلا && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    اگر (دورہ(nodep)) واپس !N_Default;

    // Visit the rest of the tree
    جب (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        گرہ_2* ثابت headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_فاصلہ]);

        // Ensure we have stack space for nextp and the 4 children
        اگر (VL_UNLIKELY(topp >= limp)) {
            ثابت ptrdiff_t occupancy = topp - basep;
            {
                ثابت سائز_t new_ڈھیر_سائز = ڈھیر_سائز * 2;
                گرہ_2** ثابت newStackp = new گرہ_2*[new_ڈھیر_سائز];
                std::حافظہ_نقل(newStackp, basep - Prefetch_فاصلہ,
                            حجم(گرہ_2**) * (occupancy + Prefetch_فاصلہ));
                ڈھیر_سائز = new_ڈھیر_سائز;
                برآمد_کا_سطر_Stackp.reset(newStackp);
            }
            basep = برآمد_کا_سطر_Stackp.get() + Prefetch_فاصلہ;
            topp = basep + occupancy;
            limp = برآمد_کا_سطر_Stackp.get() + ڈھیر_سائز - Max_Pushes_فی_Iteration;
        }

        // Enqueue the next node
        اگر (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        اگر (دورہ(headp)) واپس !N_Default;
    }

    واپس N_Default;
}

اندرونی std::ostream& عامل<<(std::ostream& os, ثابت Ast_گرہ* rhs) {
    اگر (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } ورنہ اگر (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } ورنہ {
        rhs->dump(os);
    }
    واپس os;
}
خالی VNRelinker::relink(Ast_گرہ* newp) { newp->Ast_گرہ::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
سانچہ <typename T_گرہ>
class VNRef final : عوامی std::reference_wrapper<T_گرہ> {
    static_assert(std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

عوامی:
    سانچہ <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_گرہ>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(ثابت std::reference_wrapper<T_گرہ>& other)
        : std::reference_wrapper<T_گرہ>{other} {}
};

static_assert(حجم(VNRef<Ast_گرہ>) == حجم(std::reference_wrapper<Ast_گرہ>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
سائز_t V3HasherUncachedHash(ثابت Ast_گرہ&);

// Specialization of std::hash for VNRef
سانچہ <typename T_گرہ>
ساخت std::hash<VNRef<T_گرہ>> final {
    سائز_t عامل()(VNRef<T_گرہ> r) ثابت { واپس V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
سانچہ <typename T_گرہ>
ساخت std::equal_مقصد<VNRef<T_گرہ>> final {
    سائز_t عامل()(VNRef<T_گرہ> ra, VNRef<T_گرہ> rb) ثابت {
        واپس ra.get().same_درخت(&(rb.get()));
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
    std::unordered_map<ثابت Ast_گرہ*, Ast_گرہ*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<ثابت Ast_گرہ*, std::سمتیہ<Ast_گرہ*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<ثابت Ast_گرہ*, std::string> m_prefixes;
    ثابت VDTypeNameScopes* ثابت m_prevp;  // Index in use before this one
    ساکن ثابت VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    خالی اشاریہ(Ast_گرہ* nodep, Ast_گرہ* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

عوامی:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    ساکن ثابت VDTypeNameScopes* currentp() { واپس s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(ثابت Ast_گرہ* nodep, Ast_گرہ*& outerpr) ثابت;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ثابت std::سمتیہ<Ast_گرہ*>* paramsp(ثابت Ast_گرہ* modp) ثابت;
    // The prefix kept for scope 'scopep', or nullptr if none
    ثابت std::string* prefixp(ثابت Ast_گرہ* scopep) ثابت;
    // Keep 'prefix', final, for scope 'scopep'
    خالی سابقہ(ثابت Ast_گرہ* scopep, ثابت std::string& سابقہ) ثابت;
};

//######################################################################
// Inline VNVisitor METHODS

خالی VNVisitorConst::iterateConst(Ast_گرہ* nodep) { nodep->accept(*this); }
خالی VNVisitorConst::iterateConstNull(Ast_گرہ* nodep) {
    اگر (VL_LIKELY(nodep)) nodep->accept(*this);
}
خالی VNVisitorConst::iterateChildrenConst(Ast_گرہ* nodep) { nodep->iterateChildrenConst(*this); }
خالی VNVisitorConst::iterateChildrenBackwardsConst(Ast_گرہ* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
خالی VNVisitorConst::iterate_And_اگلا_Const_Null(Ast_گرہ* nodep) {
    اگر (VL_LIKELY(nodep)) nodep->iterate_And_اگلا_Const(*this);
}

خالی VNVisitor::iterate(Ast_گرہ* nodep) { nodep->accept(*this); }
خالی VNVisitor::iterateNull(Ast_گرہ* nodep) {
    اگر (VL_LIKELY(nodep)) nodep->accept(*this);
}
خالی VNVisitor::iterateChildren(Ast_گرہ* nodep) { nodep->iterateChildren(*this); }
خالی VNVisitor::iterate_And_اگلا_Null(Ast_گرہ* nodep) {
    اگر (VL_LIKELY(nodep)) nodep->iterate_And_اگلا(*this);
}
Ast_گرہ* VNVisitor::iterateSubtreeReturnEdits(Ast_گرہ* nodep) {
    واپس nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#شامل "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#شامل "V3AstNodeDType.h"
#شامل "V3AstNodeOther.h"
#شامل "V3AstNodeExpr.h"
#شامل "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#شامل "V3AstInlines.h"
خالی dump_گرہ_List_Json(std::ostream& os, ثابت Ast_گرہ* nodep, ثابت std::string& listName,
                      ثابت string& indent);

#اگر_ختم  // Guard
