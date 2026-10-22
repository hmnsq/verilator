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

#که_نه_تعریف_شوی VERILATOR_V3AST_H_
#تعریف_کړه VERILATOR_V3AST_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Broken.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3FunctionTraits.h"
#ضمن "V3Global.h"
#ضمن "V3Number.h"
#ضمن "V3StdFuture.h"

#ضمن "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#ضمن <array>
#ضمن <cmath>
#ضمن <cstdint>
#ضمن <functional>
#ضمن <map>
#ضمن <set>
#ضمن <type_traits>
#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <utility>
#ضمن <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#إذا defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#تعریف_لغوه VL_USER_TYPE_CHECKS
#د_که_پای
#که_تعریف_شوی VL_USER_TYPE_CHECKS
#ضمن <variant>
#د_که_پای

// clang-format off
#ضمن "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3_ګراف;
class ExecMTask;
class VIface_Capture_نښان;

//######################################################################

// For broken() function, return error string if have a match
#تعریف_کړه BROKEN_RTN(test) \
    افعل { \
        إذا (VL_UNCOVERABLE(test)) أرجع "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } طالما (false)
// For broken() function, return error string if a base of this class has a match
#تعریف_کړه BROKEN_بنسټ_RTN(test) \
    افعل { \
        ثابت حرف* ثابت reasonp = (test); \
        إذا (VL_UNCOVERABLE(reasonp)) أرجع reasonp; \
    } طالما (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#تعریف_کړه VN_IS(nodep, nodetypename) (Ast_غوټه::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#تعریف_کړه VN_CAST(nodep, nodetypename) (Ast_غوټه::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#تعریف_کړه VN_AS(nodep, nodetypename) (Ast_غوټه::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#إذا defined(VL_DEBUG) || defined(VL_ASAN)
#تعریف_کړه VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#وإلا
#تعریف_کړه VN_DBG_AS(nodep, nodetypename) (Ast_غوټه::unsafe_شخصي_As<Ast##nodetypename>(nodep))
#د_که_پای

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#تعریف_کړه VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

بنية VNTypeInfo final {
    ثابت حرف* m_namep;
    تعداد OpEn : uint8_t {
        OP_ناکارول_شوی,
        OP_کارول_شوی,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ثابت حرف* m_opNamep[4];
    اندازه_t m_sizeof;
};

class VNType final {
    ساكن ثابت VNTypeInfo s_type_Info_جدول[];

عام:
#ضمن "V3Ast__gen_type_enum.h"  // From ./astgen
    ثابت en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_خوندي : m_e{_e} {}
    constexpr چلوونکی en() ثابت VL_MT_خوندي { أرجع m_e; }

    // Retrieve VNTypeInfo for the given type
    ساكن ثابت VNTypeInfo& typeInfo(VNType t) VL_MT_خوندي { أرجع s_type_Info_جدول[t.m_e]; }
};
constexpr bool چلوونکی==(ثابت VNType& lhs, ثابت VNType& rhs) VL_خالص {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VNType& lhs, VNType::en rhs) VL_خالص { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VNType::en lhs, ثابت VNType& rhs) VL_خالص { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VNType& rhs) { أرجع os << rhs.ascii(); }

//######################################################################
// AstNUser - Generic base class for AST User nodes.
//          - Also used to allow parameter passing up/down iterate calls

class پلنوالی_VP;
class V3_ګراف_راس;
class VSymEnt;

class VNUser final {
// Defining VL_USER_TYPE_CHECKS replaces the VNUser union with a std::variant
// which remembers which form of VNUser is being stored to catch mismatched reads.
#که_تعریف_شوی VL_USER_TYPE_CHECKS
    // monostate is an unwritten / cleared slot. It can be read as either form
    // and yields nullptr/0.
    std::variant<std::monostate, uint64_t, فراغ*> m_u;
#وإلا
    اتحاد {
        فراغ* پورته;
        uint64_t uq;
    } m_u;
#د_که_پای

عام:
#که_تعریف_شوی VL_USER_TYPE_CHECKS
    VNUser() = افتراضي;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        إذا (i) m_u = i;
    }
    explicit VNUser(فراغ* p) {
        // VNUser{nullptr} represents the monostate
        إذا (p) m_u = p;
    }
    ~VNUser() = افتراضي;
    // Casters
    کينډۍ <typename T>
    typename std::enable_if<std::is_اشاره_ګر<T>::ارزښت, T>::type منزل() ثابت VL_MT_خوندي {
        إذا (std::holds_alternative<std::monostate>(m_u)) أرجع nullptr;
        فراغ* ثابت* ثابت upp = std::get_if<فراغ*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        أرجع reinterpret_cast<T>(*upp);
    }
    uint64_t منزل_UQuad() ثابت {
        إذا (std::holds_alternative<std::monostate>(m_u)) أرجع 0;
        ثابت uint64_t* ثابت uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        أرجع *uip;
    }
#وإلا
    VNUser() = افتراضي;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.پورته = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(فراغ* p) { m_u.پورته = p; }
    ~VNUser() = افتراضي;
    // Casters
    کينډۍ <typename T>
    typename std::enable_if<std::is_اشاره_ګر<T>::ارزښت, T>::type منزل() ثابت VL_MT_خوندي {
        أرجع reinterpret_cast<T>(m_u.پورته);
    }
    uint64_t منزل_UQuad() ثابت { أرجع m_u.uq; }
#د_که_پای
    VSymEnt* منزل_Sym_Ent() ثابت { أرجع منزل<VSymEnt*>(); }
    Ast_غوټه* منزل_Nodep() ثابت VL_MT_خوندي { أرجع منزل<Ast_غوټه*>(); }
    V3_ګراف_راس* منزل_ګراف_راس() ثابت { أرجع منزل<V3_ګراف_راس*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(ثابت فراغ*)) ثابت;
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

class VNUser_دننه_Use_بنسټ
VL_NOT_FINAL{محمي : ساكن فراغ allocate(صحيح id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvt_منزل_Str(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
ساكن فراغ حرر(صحيح id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvt_منزل_Str(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
ساكن فراغ clearcnt(صحيح id, uint32_t& cntGblRef, ثابت bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvt_منزل_Str(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
ساكن فراغ checkcnt(صحيح id, uint32_t&, ثابت bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef,
                   "Check of User" + cvt_منزل_Str(id) + "() failed, not under AstUserInUse");
}
}
;

// For each user() declare the in use structure
// We let AstNode peek into here, because when under low optimization even
// an accessor would be way too slow.
// clang-format off
class VNUser1_دننه_Use final : VNUser_دننه_Use_بنسټ {
محمي:
    ملګری class Ast_غوټه;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser1_دننه_Use()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1_دننه_Use()     { حرر    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2_دننه_Use final : VNUser_دننه_Use_بنسټ {
محمي:
    ملګری class Ast_غوټه;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser2_دننه_Use()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2_دننه_Use()     { حرر    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3_دننه_Use final : VNUser_دننه_Use_بنسټ {
محمي:
    ملګری class Ast_غوټه;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser3_دننه_Use()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3_دننه_Use()     { حرر    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4_دننه_Use final : VNUser_دننه_Use_بنسټ {
محمي:
    ملګری class Ast_غوټه;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser4_دننه_Use()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4_دننه_Use()     { حرر    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::ویکټور<Ast_غوټه*> m_deleteps;  // Nodes to delete

عام:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    فراغ ټېل_Deletep(Ast_غوټه* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.ټېل_شا(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    فراغ doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    ملګری class Ast_غوټه;

عام:
    /// Call visit()s on nodep
    مضمن فراغ iterateConst(Ast_غوټه* nodep);
    /// Call visit()s on nodep
    مضمن فراغ iterateConstNull(Ast_غوټه* nodep);
    /// Call visit()s on const nodep's children
    مضمن فراغ iterateChildrenConst(Ast_غوټه* nodep);
    /// Call visit()s on nodep's children in backp() order
    مضمن فراغ iterateChildrenBackwardsConst(Ast_غوټه* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    مضمن فراغ iterate_And_بل_Const_Null(Ast_غوټه* nodep);

    مجازي فراغ لیدنه(Ast_غوټه* nodep) = 0;
    مجازي ~VNVisitorConst() {}
#ضمن "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : عام VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
عام:
    /// Call visit()s on nodep
    مضمن فراغ iterate(Ast_غوټه* nodep);
    /// Call visit()s on nodep
    مضمن فراغ iterateNull(Ast_غوټه* nodep);
    /// Call visit()s on nodep's children
    مضمن فراغ iterateChildren(Ast_غوټه* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    مضمن فراغ iterate_And_بل_Null(Ast_غوټه* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    مضمن Ast_غوټه* iterateSubtreeReturnEdits(Ast_غوټه* nodep);

    VNDeleter& deleter() { أرجع m_deleter; }
    فراغ ټېل_Deletep(Ast_غوټه* nodep) { deleter().ټېل_Deletep(nodep); }
    فراغ doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
محمي:
    ملګری class Ast_غوټه;
    تعداد RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_بل,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_غوټه* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_غوټه* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_غوټه** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

عام:
    VNRelinker() = افتراضي;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    مضمن فراغ relink(Ast_غوټه* newp);
    فراغ dump(std::ostream& str = std::cout) ثابت;
};
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VNRelinker& rhs) {
    rhs.dump(os);
    أرجع os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{عام : مجازي bool غوټه_سمون(ثابت Ast_غوټه* nodep) ثابت {أرجع true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#تعریف_کړه ASTNODE_PREFETCH_NON_NULL(nodep) \
    افعل { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } طالما (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#تعریف_کړه ASTNODE_PREFETCH(nodep) \
    افعل { \
        إذا (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } طالما (false)

class Ast_غوټه VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_غوټه* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_غوټه* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_غوټه* m_op1p = nullptr;  // Generic pointer 1
    Ast_غوټه* m_op2p = nullptr;  // Generic pointer 2
    Ast_غوټه* m_op3p = nullptr;  // Generic pointer 3
    Ast_غوټه* m_op4p = nullptr;  // Generic pointer 4
    Ast_غوټه** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    ثابت VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    بنية {
        bool did_پلنوالی : 1;  // Did V3Width computation
        bool doing_پلنوالی : 1;  // Inside V3Width
        bool protect : 1;  // Protect name if protection is on
        // Space for more flags here (there must be 8 bits in total)
        uint8_t ناکارول_شوی : 5;
    } m_flags;  // Attribute flags

    // State variable used by V3Broken for consistency checking. The top bit of this is byte is a
    // flag, representing V3Broken is currently proceeding under this node. The bottom 7 bits are
    // a generation number. This is hot when --debug-checks so we access as a whole to avoid bit
    // field masking resulting in unnecessary read-modify-write ops.
    uint8_t m_brokenState = 0;

    صحيح m_cloneCnt = 0;  // Sequence number for when last clone was made

#إذا defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(حجم(m_type) + حجم(m_flags) + حجم(m_brokenState) + حجم(m_cloneCnt)
                      <= حجم(فراغ*),
                  "packing requires padding");
#د_که_پای

    Ast_غوټه_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_غوټه* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_کرښه* m_fileline;  // Where it was declared
#که_تعریف_شوی VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_شمېر;  // When it was last edited
#د_که_پای
    ساكن uint64_t s_editCntGbl;  // Global edit counter
    ساكن uint64_t s_edit_Cnt_وروستی;  // Last committed value of global edit counter

    Ast_غوټه* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    ساكن صحيح s_cloneCntGbl;  // Count of which userp is set

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
    فراغ op1p(Ast_غوټه* nodep) {
        m_op1p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op2p(Ast_غوټه* nodep) {
        m_op2p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op3p(Ast_غوټه* nodep) {
        m_op3p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op4p(Ast_غوټه* nodep) {
        m_op4p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }

خاص:
    Ast_غوټه* clone_ونه_Iter(bool need_خالص);
    Ast_غوټه* clone_ونه_Iter_List(bool need_خالص);
    فراغ check_ونه_Iter(ثابت Ast_غوټه* prevBackp) ثابت VL_MT_STABLE;
    bool gate_ونه_Iter() ثابت;
    ساكن bool same_ونه_Iter(ثابت Ast_غوټه* node1p, ثابت Ast_غوټه* node2p, bool ign_بل,
                             bool gateOnly);
    فراغ delete_ونه_Iter();
    فراغ delete_غوټه();
    string instanceStr() ثابت;

عام:
    فراغ purityCheck();
    ساكن فراغ relinkOneLink(Ast_غوټه*& pointpr, Ast_غوټه* newp);
    // cppcheck-suppress functionConst
    ساكن فراغ debug_ونه_Change(ثابت Ast_غوټه* nodep, ثابت حرف* مختاړی, صحيح lineno, bool بل);

محمي:
    // CONSTRUCTORS
    Ast_غوټه(VNType t, File_کرښه* fl);
    مجازي ~Ast_غوټه() = افتراضي;  // Use 'deleteTree' instead
    مجازي Ast_غوټه* clone() = 0;  // Generally, cloneTree is what you want instead
    مجازي فراغ cloneRelink() { cloneRelinkGen(); }
    مجازي فراغ cloneRelinkGen() {};  // Overrides generated by 'astgen'
    فراغ clone_Relink_ونه();

    // METHODS
    فراغ سېټ_Op1p(Ast_غوټه* newp);  // Set non-list-type op1 to non-list element
    فراغ سېټ_Op2p(Ast_غوټه* newp);  // Set non-list-type op2 to non-list element
    فراغ سېټ_Op3p(Ast_غوټه* newp);  // Set non-list-type op3 to non-list element
    فراغ سېټ_Op4p(Ast_غوټه* newp);  // Set non-list-type op4 to non-list element

    فراغ جمع_کړه_Op1p(Ast_غوټه* newp);  // Append newp to end of op1
    فراغ جمع_کړه_Op2p(Ast_غوټه* newp);  // Append newp to end of op2
    فراغ جمع_کړه_Op3p(Ast_غوټه* newp);  // Append newp to end of op3
    فراغ جمع_کړه_Op4p(Ast_غوټه* newp);  // Append newp to end of op4

    فراغ prependOp1p(Ast_غوټه* newp);  // Prepend newp to front of op1
    فراغ prependOp2p(Ast_غوټه* newp);  // Prepend newp to front of op2
    فراغ prependOp3p(Ast_غوټه* newp);  // Prepend newp to front of op3
    فراغ prependOp4p(Ast_غوټه* newp);  // Prepend newp to front of op4

    // clang-format off
    فراغ سېټ_NOp1p(Ast_غوټه* newp) { إذا (newp) سېټ_Op1p(newp); }
    فراغ سېټ_NOp2p(Ast_غوټه* newp) { إذا (newp) سېټ_Op2p(newp); }
    فراغ سېټ_NOp3p(Ast_غوټه* newp) { إذا (newp) سېټ_Op3p(newp); }
    فراغ سېټ_NOp4p(Ast_غوټه* newp) { إذا (newp) سېټ_Op4p(newp); }

    فراغ جمع_کړه_NOp1p(Ast_غوټه* newp) { إذا (newp) جمع_کړه_Op1p(newp); }
    فراغ جمع_کړه_NOp2p(Ast_غوټه* newp) { إذا (newp) جمع_کړه_Op2p(newp); }
    فراغ جمع_کړه_NOp3p(Ast_غوټه* newp) { إذا (newp) جمع_کړه_Op3p(newp); }
    فراغ جمع_کړه_NOp4p(Ast_غوټه* newp) { إذا (newp) جمع_کړه_Op4p(newp); }

    فراغ prependNOp1p(Ast_غوټه* newp) { إذا (newp) prependOp1p(newp); }
    فراغ prependNOp2p(Ast_غوټه* newp) { إذا (newp) prependOp2p(newp); }
    فراغ prependNOp3p(Ast_غوټه* newp) { إذا (newp) prependOp3p(newp); }
    فراغ prependNOp4p(Ast_غوټه* newp) { إذا (newp) prependOp4p(newp); }
    // clang-format on

    فراغ clonep(Ast_غوټه* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    ساكن فراغ clone_Clear_ونه() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    مجازي bool same_غوټه(ثابت Ast_غوټه*) ثابت { أرجع true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    مجازي bool wouldBreakGen(ثابت Ast_غوټه* ثابت oldp,
                               ثابت Ast_غوټه* ثابت newp) ثابت
        = 0;  // Generated by 'astgen'

عام:
    // ACCESSORS
    VNType type() ثابت VL_MT_خوندي { أرجع m_type; }
    ثابت حرف* typeName() ثابت VL_MT_خوندي { أرجع type().ascii(); }  // See also prettyTypeName
    Ast_غوټه* nextp() ثابت VL_MT_STABLE { أرجع m_nextp; }
    Ast_غوټه* backp() ثابت VL_MT_STABLE { أرجع m_backp; }
    Ast_غوټه* abovep() ثابت;  // Get parent node above, only for list head and tail
    Ast_غوټه* aboveLoopp() ثابت;  // Get parent node above, may have performance issues as loops
    Ast_غوټه* lastp() ثابت {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        أرجع m_headtailp;
    }
    Ast_غوټه* prevp() ثابت {  // Previous node in list, nullptr for head of list
        إذا (m_backp->m_nextp != this) أرجع nullptr;
        أرجع m_backp;
    }
    Ast_غوټه* op1p() ثابت VL_MT_STABLE { أرجع m_op1p; }
    Ast_غوټه* op2p() ثابت VL_MT_STABLE { أرجع m_op2p; }
    Ast_غوټه* op3p() ثابت VL_MT_STABLE { أرجع m_op3p; }
    Ast_غوټه* op4p() ثابت VL_MT_STABLE { أرجع m_op4p; }
    Ast_غوټه_DType* dtypep() ثابت VL_MT_STABLE { أرجع m_dtypep; }
    Ast_غوټه* clonep() ثابت { أرجع ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_غوټه* لومړی_Abovep() ثابت {  // Returns nullptr when second or later in list
        أرجع ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ثابت VL_MT_خوندي { أرجع m_brokenState; }
    فراغ brokenState(uint8_t ارزښت) { m_brokenState = ارزښت; }

    // Used by AstNode::broken()
    bool brokeExists() ثابت { أرجع V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ثابت { أرجع brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    مجازي فراغ foreachLink(std::function<فراغ(Ast_غوټه** linkpp, ثابت حرف* namep)> f) = 0;

#إذا defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    ساكن فراغ* چلوونکی new(اندازه_t اندازه);
    ساكن فراغ چلوونکی delete(فراغ* obj, اندازه_t اندازه);
#د_که_پای

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    ساكن constexpr صحيح INSTR_شمېر_BRANCH = 4;  // Branch
    ساكن constexpr صحيح INSTR_شمېر_CALL = INSTR_شمېر_BRANCH + 10;  // Subroutine call
    ساكن constexpr صحيح INSTR_شمېر_LD = 2;  // Load memory
    ساكن constexpr صحيح INSTR_شمېر_INT_MUL = 3;  // Integer multiply
    ساكن constexpr صحيح INSTR_شمېر_INT_DIV = 10;  // Integer divide
    ساكن constexpr صحيح INSTR_شمېر_DBL = 8;  // Convert or do float ops
    ساكن constexpr صحيح INSTR_شمېر_DBL_DIV = 40;  // Double divide
    ساكن constexpr صحيح INSTR_شمېر_DBL_TRIG = 200;  // Double trigonometric ops
    ساكن constexpr صحيح INSTR_شمېر_STR = 100;  // String ops
    ساكن constexpr صحيح INSTR_شمېر_وخت = INSTR_شمېر_CALL + 5;  // Determine simulation time
    ساكن constexpr صحيح INSTR_شمېر_PLI = 20;  // PLI routines
    ساكن constexpr صحيح INSTR_شمېر_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    مجازي string name() ثابت VL_MT_STABLE { أرجع ""; }
    مجازي string origName() ثابت { أرجع ""; }
    string prettyDehashOrigOrName() ثابت {
        أرجع prettyName(VName::dehash(origName().تشه() ? name() : origName()));
    }
    مجازي فراغ name(ثابت string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    مجازي فراغ نښان(ثابت string& متن) {}
    مجازي string نښان() ثابت { أرجع ""; }
    مجازي uint32_t decl_Token_عدد() ثابت { أرجع 0; }
    مجازي فراغ decl_Token_عدد_سېټ_Min(uint32_t token_عدد) {}
    مجازي string verilogKwd() ثابت { أرجع ""; }
    string nameProtect() ثابت VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() ثابت;  // origName with --protect-id applied
    string shortName() ثابت;  // Name with __PVT__ removed for concatenating scopes
    ساكن string dedotName(ثابت string& namein);  // Name with dots removed
    ساكن string nameNoArray(ثابت string& namein);  // Name with any array index removed
    ساكن string prettyName(ثابت string& namein) VL_خالص;  // Name for printing out to the user
    ساكن string vpiName(ثابت string& namein);  // Name for vpi access
    ساكن string prettyNameQ(ثابت string& namein) {  // Quoted pretty name (for errors)
        أرجع "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    ساكن string encodeName(ثابت string& namein);
    ساكن string encode_شمېره(int64_t عدد);  // Encode number into internal C representation
    ساكن string vcdName(ثابت string& namein);  // Name for printing out to vcd files
    string prettyName() ثابت { أرجع prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    مجازي string pretty_Name_پیغام() ثابت { أرجع prettyName(); }
    string prettyNameQ() ثابت { أرجع "'"s + pretty_Name_پیغام() + "'"; }  // Quoted, for messages
    string verilogName() ثابت { أرجع vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ثابت;
    مجازي string prettyOperatorName() ثابت { أرجع "operator " + prettyTypeName(); }
    File_کرښه* fileline() ثابت VL_MT_خوندي { أرجع m_fileline; }
    فراغ fileline(File_کرښه* fl) { m_fileline = fl; }
    مضمن bool width1() ثابت;
    مضمن صحيح پلنوالی_Instrs() ثابت;
    bool did_پلنوالی() ثابت { أرجع m_flags.did_پلنوالی; }
    فراغ did_پلنوالی(bool flag) { m_flags.did_پلنوالی = flag; }
    bool did_پلنوالی_And_سېټ() {
        إذا (did_پلنوالی()) أرجع true;
        did_پلنوالی(true);
        أرجع false;
    }
    bool doing_پلنوالی() ثابت { أرجع m_flags.doing_پلنوالی; }
    فراغ doing_پلنوالی(bool flag) { m_flags.doing_پلنوالی = flag; }
    bool protect() ثابت VL_MT_خوندي { أرجع m_flags.protect; }
    فراغ protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    مضمن صحيح پلنوالی() ثابت VL_MT_STABLE;
    مضمن صحيح پلنوالی_Min() ثابت VL_MT_STABLE;
    صحيح پلنوالی_Min_V() ثابت {
        أرجع v3Global.پلنوالی_Min_Usage() == VWidthMinUsage::VERILOG_پلنوالی ? پلنوالی_Min() : پلنوالی();
    }
    صحيح پلنوالی_ټکي() ثابت { أرجع VL_ټکي_I(پلنوالی()); }
    bool isQuad() ثابت VL_MT_STABLE { أرجع (پلنوالی() > VL_IDATASIZE && پلنوالی() <= VL_QUADSIZE); }
    bool isWide() ثابت VL_MT_STABLE { أرجع (پلنوالی() > VL_QUADSIZE); }
    مضمن bool isCHandle() ثابت VL_MT_STABLE;
    مضمن bool isDouble() ثابت VL_MT_STABLE;
    مضمن bool isSigned() ثابت VL_MT_STABLE;
    مضمن bool isString() ثابت VL_MT_STABLE;
    مضمن bool isEvent() ثابت VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        أرجع ((m_user1Cnt == VNUser1_دننه_Use::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_غوټه* user1p() ثابت VL_MT_STABLE { أرجع user1u().منزل_Nodep(); }
    فراغ user1u(ثابت VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1_دننه_Use::s_userCntGbl; }
    فراغ user1p(فراغ* userp) { user1u(VNUser{userp}); }
    فراغ user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ثابت { أرجع user1u().منزل_UQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ثابت uint64_t v = user1(); user1(v + val); أرجع v; }
    uint64_t user1Or(uint64_t val) { ثابت uint64_t v = user1(); user1(v | val); أرجع v; }
    uint64_t user1_سېټ_Once() { ثابت uint64_t v = user1(); إذا (!v) user1(1); أرجع v; }  // Better for cache than user1Inc()
    ساكن فراغ user1_Clear_ونه() { VNUser1_دننه_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        أرجع ((m_user2Cnt == VNUser2_دننه_Use::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_غوټه* user2p() ثابت VL_MT_STABLE { أرجع user2u().منزل_Nodep(); }
    فراغ user2u(ثابت VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2_دننه_Use::s_userCntGbl; }
    فراغ user2p(فراغ* userp) { user2u(VNUser{userp}); }
    فراغ user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ثابت { أرجع user2u().منزل_UQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ثابت uint64_t v = user2(); user2(v + val); أرجع v; }
    uint64_t user2Or(uint64_t val) { ثابت uint64_t v = user2(); user2(v | val); أرجع v; }
    uint64_t user2_سېټ_Once() { ثابت uint64_t v = user2(); إذا (!v) user2(1); أرجع v; }  // Better for cache than user2Inc()
    ساكن فراغ user2_Clear_ونه() { VNUser2_دننه_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        أرجع ((m_user3Cnt == VNUser3_دننه_Use::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_غوټه* user3p() ثابت VL_MT_STABLE { أرجع user3u().منزل_Nodep(); }
    فراغ user3u(ثابت VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3_دننه_Use::s_userCntGbl; }
    فراغ user3p(فراغ* userp) { user3u(VNUser{userp}); }
    فراغ user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ثابت { أرجع user3u().منزل_UQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ثابت uint64_t v = user3(); user3(v + val); أرجع v; }
    uint64_t user3Or(uint64_t val) { ثابت uint64_t v = user3(); user3(v | val); أرجع v; }
    uint64_t user3_سېټ_Once() { ثابت uint64_t v = user3(); إذا (!v) user3(1); أرجع v; }  // Better for cache than user3Inc()
    ساكن فراغ user3_Clear_ونه() { VNUser3_دننه_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        أرجع ((m_user4Cnt == VNUser4_دننه_Use::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_غوټه* user4p() ثابت VL_MT_STABLE { أرجع user4u().منزل_Nodep(); }
    فراغ user4u(ثابت VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4_دننه_Use::s_userCntGbl; }
    فراغ user4p(فراغ* userp) { user4u(VNUser{userp}); }
    فراغ user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ثابت { أرجع user4u().منزل_UQuad(); }
    uint64_t user4Or(uint64_t val) { ثابت uint64_t v = user4(); user4(v | val); أرجع v; }
    uint64_t user4Inc(uint64_t val = 1) { ثابت uint64_t v = user4(); user4(v + val); أرجع v; }
    uint64_t user4_سېټ_Once() { ثابت uint64_t v = user4(); إذا (!v) user4(1); أرجع v; }  // Better for cache than user4Inc()
    ساكن فراغ user4_Clear_ونه() { VNUser4_دننه_Use::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#که_تعریف_شوی VL_DEBUG
    uint64_t edit_شمېر() ثابت { أرجع m_edit_شمېر; }
    فراغ edit_شمېر_Inc() {
        m_edit_شمېر = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_ونه();  // Any edit clears all caching
    }
#وإلا
    فراغ edit_شمېر_Inc() { ++s_editCntGbl; }
#د_که_پای
    ساكن uint64_t edit_شمېر_وروستی() VL_MT_خوندي { أرجع s_edit_Cnt_وروستی; }
    ساكن uint64_t edit_شمېر_Gbl() VL_MT_خوندي { أرجع s_editCntGbl; }
    ساكن فراغ edit_شمېر_سېټ_وروستی() { s_edit_Cnt_وروستی = edit_شمېر_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_کتار_ټېل_Self_Stmt();
    مضمن bool is_Class_Handle_ارزښت() ثابت;
    مضمن bool isNull() ثابت;
    مضمن bool isZero() ثابت;
    مضمن bool isOne() ثابت;
    مضمن bool isNeqZero() ثابت;
    مضمن bool isAllOnes() ثابت;
    مضمن bool isAllOnesV() ثابت;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    فراغ dtypep(Ast_غوټه_DType* nodep) {
        إذا (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_شمېر_Inc();
        }
    }
    فراغ dtypeFrom(ثابت Ast_غوټه* fromp) {
        إذا (fromp) dtypep(fromp->dtypep());
    }
    فراغ dtypeChgSigned(bool flag = true);
    فراغ dtype_Chg_پلنوالی(صحيح پلنوالی, صحيح پلنوالی_Min);
    فراغ dtype_Chg_پلنوالی_Signed(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric);
    فراغ dtype_سېټ_بټ_Unsized(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) {
        dtypep(پیدا_کړه_بټ_DType(پلنوالی, پلنوالی_Min, numeric));
    }
    فراغ dtype_سېټ_بټ_Sized(صحيح پلنوالی, VSigning numeric) {
        dtypep(پیدا_کړه_بټ_DType(پلنوالی, پلنوالی, numeric));  // Since sized, widthMin is width
    }
    فراغ dtype_سېټ_Logic_Unsized(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) {
        dtypep(پیدا_کړه_Logic_DType(پلنوالی, پلنوالی_Min, numeric));
    }
    فراغ dtype_سېټ_Logic_Sized(صحيح پلنوالی, VSigning numeric) {
        dtypep(پیدا_کړه_Logic_DType(پلنوالی, پلنوالی, numeric));  // Since sized, widthMin is width
    }
    فراغ dtype_سېټ_Logic2_State_Unsized(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) {
        dtypep(پیدا_کړه_Logic2_State_DType(پلنوالی, پلنوالی_Min, numeric));
    }
    فراغ dtype_سېټ_Logic2_State_Sized(صحيح پلنوالی, VSigning numeric) {
        dtypep(پیدا_کړه_Logic2_State_DType(پلنوالی, پلنوالی, numeric));  // Since sized, widthMin is width
    }
    فراغ dtype_سېټ_بټ_Or_Logic_Unsized(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric, bool isFourstate) {
        dtypep(پیدا_کړه_بټ_Or_Logic_DType(پلنوالی, پلنوالی_Min, numeric, isFourstate));
    }
    فراغ dtype_سېټ_بټ_Or_Logic_Sized(صحيح پلنوالی, VSigning numeric, bool isFourstate) {
        dtypep(پیدا_کړه_بټ_Or_Logic_DType(پلنوالی, پلنوالی, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    فراغ dtype_سېټ_بټ() { dtypep(پیدا_کړه_بټ_DType()); }
    فراغ dtype_سېټ_Logic() { dtypep(پیدا_کړه_Logic_DType()); }
    فراغ dtype_سېټ_Logic2_State() { dtypep(پیدا_کړه_Logic2_State_DType()); }
    فراغ dtype_سېټ_Double() { dtypep(پیدا_کړه_Double_DType()); }
    فراغ dtype_سېټ_Int() { dtypep(پیدا_کړه_Int_DType()); }
    فراغ dtype_سېټ_Integer() { dtypep(پیدا_کړه_Integer_DType()); }
    فراغ dtype_سېټ_Integer2_State() { dtypep(پیدا_کړه_Integer2_State_DType()); }
    فراغ dtype_سېټ_String() { dtypep(پیدا_کړه_String_DType()); }
    فراغ dtype_سېټ_UInt32() { dtypep(پیدا_کړه_UInt32_DType()); }  // Twostate
    فراغ dtype_سېټ_UInt64() { dtypep(پیدا_کړه_UInt64_DType()); }  // Twostate
    فراغ dtype_سېټ_وخت() { dtypep(پیدا_کړه_وخت_DType()); }
    فراغ dtype_سېټ_Time2_State() { dtypep(پیدا_کړه_Time2_State_DType()); }
    فراغ dtype_سېټ_تشه_کتار() { dtypep(پیدا_کړه_تشه_کتار_DType()); }
    فراغ dtype_سېټ_بهیر() { dtypep(پیدا_کړه_بهیر_DType()); }
    فراغ dtype_سېټ_Void() { dtypep(پیدا_کړه_Void_DType()); }
    فراغ dtype_سېټ_بټ_Or_Logic(bool isFourstate) {
        dtypep(isFourstate ? پیدا_کړه_Logic_DType() : پیدا_کړه_بټ_DType());
    }

    // Data type locators
    Ast_غوټه_DType* پیدا_کړه_بټ_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::بټ); }
    Ast_غوټه_DType* پیدا_کړه_Logic_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::LOGIC); }
    Ast_غوټه_DType* پیدا_کړه_Null_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::NULLTYPE); }
    Ast_غوټه_DType* پیدا_کړه_Logic2_State_DType() ثابت {
        أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_غوټه_DType* پیدا_کړه_Double_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::DOUBLE); }
    Ast_غوټه_DType* پیدا_کړه_Int_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::INT); }
    Ast_غوټه_DType* پیدا_کړه_Integer_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::INTEGER); }
    Ast_غوټه_DType* پیدا_کړه_Integer2_State_DType() ثابت {
        أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_غوټه_DType* پیدا_کړه_String_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::STRING); }
    Ast_غوټه_DType* پیدا_کړه_Signed8_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::BYTE); }
    Ast_غوټه_DType* پیدا_کړه_UInt32_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT32); }
    Ast_غوټه_DType* پیدا_کړه_UInt64_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::UINT64); }
    Ast_غوټه_DType* پیدا_کړه_وخت_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::وخت); }
    Ast_غوټه_DType* پیدا_کړه_Time2_State_DType() ثابت {
        أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_غوټه_DType* پیدا_کړه_CHandle_DType() ثابت { أرجع پیدا_کړه_Basic_DType(VBasicDTypeKwd::CHANDLE); }
    Ast_غوټه_DType* پیدا_کړه_Constraint_Ref_DType() ثابت;
    Ast_غوټه_DType* پیدا_کړه_تشه_کتار_DType() ثابت;
    Ast_غوټه_DType* پیدا_کړه_کتار_شاخص_DType() ثابت;
    Ast_غوټه_DType* پیدا_کړه_بهیر_DType() ثابت;
    Ast_غوټه_DType* پیدا_کړه_Void_DType() ثابت;
    Ast_غوټه_DType* پیدا_کړه_بټ_DType(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) ثابت;
    Ast_غوټه_DType* پیدا_کړه_Logic_DType(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) ثابت;
    Ast_غوټه_DType* پیدا_کړه_Logic2_State_DType(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) ثابت;
    Ast_غوټه_DType* پیدا_کړه_بټ_Or_Logic_DType(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric,
                                      bool isFourstate) ثابت {
        أرجع isFourstate ? پیدا_کړه_Logic_DType(پلنوالی, پلنوالی_Min, numeric)
                           : پیدا_کړه_بټ_DType(پلنوالی, پلنوالی_Min, numeric);
    }
    Ast_غوټه_DType* پیدا_کړه_بټ_Or_Logic_Range_DType(ثابت VNumRange& range, ثابت صحيح پلنوالی_Min,
                                           ثابت VSigning numeric,
                                           ثابت bool isFourstate) ثابت VL_MT_STABLE {
        أرجع isFourstate ? پیدا_کړه_Logic_Range_DType(range, پلنوالی_Min, numeric)
                           : پیدا_کړه_بټ_Range_DType(range, پلنوالی_Min, numeric);
    }
    Ast_غوټه_DType* پیدا_کړه_Logic_Range_DType(ثابت VNumRange& range, صحيح پلنوالی_Min,
                                      VSigning numeric) ثابت VL_MT_STABLE;
    Ast_غوټه_DType* پیدا_کړه_بټ_Range_DType(ثابت VNumRange& range, صحيح پلنوالی_Min,
                                    VSigning numeric) ثابت VL_MT_STABLE;
    Ast_غوټه_DType* پیدا_کړه_Basic_DType(VBasicDTypeKwd kwd) ثابت;
    ساكن AstBasicDType* پیدا_کړه_ورننباسه_Same_DType(AstBasicDType* nodep);

    ساكن VCastable computeCastable(ثابت Ast_غوټه_DType* منزل_Dtp, ثابت Ast_غوټه_DType* fromDtp,
                                     ثابت Ast_غوټه* fromConstp, bool checkIfaceArgCompat = false);
    ساكن Ast_غوټه_DType* getCommonClassTypep(Ast_غوټه* nodep1, Ast_غوټه* nodep2);

    // METHODS - dump and error
    فراغ v3error_پای_ټکی(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_پای_ټکی_Fatal(ثابت std::ostringstream& str) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
        أرجع fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ثابت { أرجع fileline()->warnContextSecondary(); }
    string warn_نور() ثابت VL_REQUIRES(V3Error::s().m_mutex) { أرجع fileline()->warn_نور(); }
    string warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) { أرجع fileline()->warnOther(); }

    مجازي فراغ dump(std::ostream& str = std::cout) ثابت;
    ساكن حرف* dump_ونه_Json_Gdb(ثابت Ast_غوټه* nodep);  // For GDB only, free()d by caller
    ساكن حرف* dump_ونه_Json_Gdb(ثابت حرف* str);  // For GDB only, free()d by caller
    ساكن حرف* dump_ونه_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    ساكن فراغ dumpGdb(ثابت Ast_غوټه* nodep);  // For GDB only
    فراغ dump_Gdb_سرلیک() ثابت;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    کينډۍ <typename T_غوټه_پایله, typename T_غوټه_بل>
    ساكن T_غوټه_پایله* جمع_کړه_بل(T_غوټه_پایله* nodep, T_غوټه_بل* newp) {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه_پایله>::ارزښت,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_بنسټ_د<T_غوټه_پایله, T_غوټه_بل>::ارزښت,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        إذا (!newp) أرجع nodep;
        أرجع static_cast<T_غوټه_پایله*>(جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(nodep, newp));
    }
    کينډۍ <typename T_غوټه_پایله, typename T_غوټه_بل>
    ساكن T_غوټه_پایله* جمع_کړه_بل_Null(T_غوټه_پایله* nodep, T_غوټه_بل* newp) {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه_پایله>::ارزښت,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_بنسټ_د<T_غوټه_پایله, T_غوټه_بل>::ارزښت,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        إذا (!newp) أرجع nodep;
        إذا (!nodep) أرجع newp;
        أرجع static_cast<T_غوټه_پایله*>(جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(nodep, newp));
    }
    مضمن Ast_غوټه* جمع_کړه_بل(Ast_غوټه* newp);
    فراغ جمع_کړه_بل_دلته(Ast_غوټه* newp);  // Insert newp at this->nextp
    فراغ جمع_کړه_دلته_This_As_بل(Ast_غوټه* newp);  // Adds at old place of this, this becomes next
    فراغ replaceWith(Ast_غوټه* newp);  // Replace current node in tree with new node
    فراغ replaceWithKeepDType(Ast_غوټه* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_غوټه* unlink_Fr_شا(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_غوټه* unlink_Fr_شا_With_بل(VNRelinker* linkerp = nullptr);
    فراغ relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    فراغ clone_Relink_غوټه() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_غوټه* clone_ونه(bool clone_بل_Link,
                       bool need_خالص = false);  // Not const, as sets clonep() on original nodep
    Ast_غوټه* clone_ونه_خالص(bool clone_بل_Link) { أرجع clone_ونه(clone_بل_Link, true); }
    bool gate_ونه() { أرجع gate_ونه_Iter(); }  // Is tree isGateOptimizable?
    مضمن bool same_ونه(ثابت Ast_غوټه* node2p) ثابت;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    مضمن bool same_Gate_ونه(ثابت Ast_غوټه* node2p) ثابت;
    فراغ delete_ونه();  // Always deletes the next link
    فراغ check_ونه() ثابت VL_MT_STABLE {
        إذا (v3Global.opt.debugCheck()) check_ونه_Iter(backp());
    }
    فراغ checkIter() ثابت;
    فراغ dumpPtrs(std::ostream& os = std::cout) ثابت;
    فراغ dump_ونه(std::ostream& os = std::cout, ثابت string& indent = "    ",
                  صحيح max_ژوروالی = 0) ثابت;
    فراغ dump_ونه(ثابت string& indent, صحيح max_ژوروالی = 0) ثابت {
        dump_ونه(std::cout, indent, max_ژوروالی);
    }
    ساكن فراغ dump_ونه_Gdb(ثابت Ast_غوټه* nodep);  // For GDB only
    فراغ dump_ونه_And_بل(std::ostream& os = std::cout, ثابت string& indent = "    ",
                         صحيح max_ژوروالی = 0) ثابت;
    فراغ dump_ونه_File(ثابت string& filename, bool doDump = true);
    ساكن فراغ dump_ونه_File_Gdb(ثابت Ast_غوټه* nodep, ثابت حرف* filenamep = nullptr);
    فراغ dump_ونه_Dot(std::ostream& os = std::cout) ثابت;
    فراغ dump_ونه_Dot_File(ثابت string& filename, bool doDump = true);
    مجازي فراغ dumpJson(std::ostream& os) ثابت { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    فراغ dumpJsonGen(std::ostream& os) ثابت {};
    مجازي فراغ dump_ونه_Json_Op_Gen(std::ostream& os, ثابت string& indent) ثابت {};
    فراغ dump_ونه_Json(std::ostream& os, ثابت string& indent = "") ثابت;
    فراغ dump_ونه_Json_File(ثابت string& filename, bool doDump = true);
    ساكن فراغ dumpJsonMetaFileGdb(ثابت حرف* filename);
    ساكن فراغ dumpJsonMetaFile(ثابت string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    ساكن std::string غوټه_Addr(ثابت Ast_غوټه* nodep) {
        أرجع v3Global.opt.dump_ونه_Addrids() ? v3Global.ptr_منزل_Id(nodep) : cvt_منزل_Hex(nodep);
    }

    // METHODS - static advancement
    ساكن Ast_غوټه* afterCommentp(Ast_غوټه* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        طالما (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    مجازي bool isBrancher() ثابت { أرجع false; }
    // Else a AstTime etc that can't be pushed out
    مجازي bool isGateOptimizable() ثابت { أرجع !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    مجازي bool isGateDedupable() ثابت { أرجع isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    مجازي bool is_بیان_Coverage_Eligible() ثابت { أرجع isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    مجازي bool isOutputter() { أرجع false; }
    // Else a AstTime etc which output can't be predicted from input
    مجازي bool isPredictOptimizable() ثابت { أرجع !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    مجازي bool is_خالص() { أرجع true; }
    // Iff isPure on current node and any nextp()
    bool is_خالص_And_بل() { أرجع is_خالص() && (!nextp() || nextp()->is_خالص()); }
    // An event control, delay, wait, etc.
    مجازي bool isTimingControl() ثابت { أرجع false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    مجازي bool isUnlikely() ثابت { أرجع false; }
    // Is an IEEE system function (versus internally-generated)
    مجازي bool isSystemFunc() ثابت { أرجع false; }
    مجازي صحيح instr_شمېر() ثابت { أرجع 0; }
    // Iff node is identical to another node
    مجازي bool isSame(ثابت Ast_غوټه* samep) ثابت {
        أرجع type() == samep->type() && same_غوټه(samep);
    }
    // Iff has a data type; dtype() must be non-null
    مجازي bool hasDType() ثابت VL_MT_خوندي { أرجع false; }
    // Iff has a non-null childDTypep(), as generic node function
    مجازي Ast_غوټه_DType* get_ماشوم_DTypep() ثابت { أرجع nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    مجازي Ast_غوټه_DType* getChild2DTypep() ثابت { أرجع nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    مجازي bool maybe_Pointed_منزل() ثابت VL_MT_خوندي { أرجع false; }
    // Don't reclaim this node in V3Dead
    مجازي bool undead() ثابت { أرجع false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    مجازي ثابت حرف* broken() ثابت { أرجع nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    مجازي ثابت حرف* brokenGen() ثابت = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ثابت Ast_غوټه* ثابت newp) ثابت { أرجع backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    مجازي فراغ accept(VNVisitorConst& v) = 0;

محمي:
    // All VNVisitor related functions are called as methods off the visitor
    ملګری class VNVisitor;
    ملګری class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    فراغ iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    فراغ iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    فراغ iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    فراغ iterate_And_بل(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    فراغ iterate_And_بل_Const(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_غوټه* iterateSubtreeReturnEdits(VNVisitor& v);

    ساكن فراغ dump_Json_عدد(std::ostream& os, ثابت std::string& name, int64_t val);
    ساكن فراغ dumpJsonBoolIf(std::ostream& os, ثابت std::string& name, bool val);
    ساكن فراغ dumpJsonStr(std::ostream& os, ثابت std::string& name, ثابت std::string& val);
    ساكن فراغ dumpJsonPtr(std::ostream& os, ثابت std::string& name, ثابت Ast_غوټه* ثابت valp);

محمي:
    فراغ iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    کينډۍ <typename T>
    مضمن ساكن bool شخصي_Type_Test(ثابت Ast_غوټه* nodep);

    // For internal use only.
    کينډۍ <typename T_موخه_Type, typename T_DeclType>
    constexpr ساكن bool uselessCast() VL_خالص {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_اشاره_ګر<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        أرجع std::is_بنسټ_د<T_موخه_Type, NonCV>::ارزښت;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    کينډۍ <typename T_موخه_Type, typename T_DeclType>
    constexpr ساكن bool impossibleCast() VL_خالص {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_اشاره_ګر<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        أرجع !std::is_بنسټ_د<NonCV, T_موخه_Type>::ارزښت;
    }  // LCOV_EXCL_STOP

عام:
    // For use via the VN_IS macro only, or in templated code
    کينډۍ <typename T_موخه_Type, typename T_غوټه>
    ساكن bool is(ثابت T_غوټه* nodep) VL_MT_خوندي {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        أرجع nodep && شخصي_Type_Test<T_موخه_Type>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    کينډۍ <typename T_موخه_Type, typename T_غوټه>
    ساكن T_موخه_Type* cast(T_غوټه* nodep) VL_MT_خوندي {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        أرجع nodep && شخصي_Type_Test<T_موخه_Type>(nodep)
                   ? reinterpret_cast<T_موخه_Type*>(nodep)
                   : nullptr;
    }
    کينډۍ <typename T_موخه_Type, typename T_غوټه>
    ساكن ثابت T_موخه_Type* cast(ثابت T_غوټه* nodep) VL_MT_خوندي {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        أرجع nodep && شخصي_Type_Test<T_موخه_Type>(nodep)
                   ? reinterpret_cast<ثابت T_موخه_Type*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    کينډۍ <typename T_موخه_Type, typename T_غوټه>
    ساكن T_موخه_Type* as(T_غوټه* nodep) VL_خالص {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || شخصي_Type_Test<T_موخه_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        أرجع unsafe_شخصي_As<T_موخه_Type, T_غوټه>(nodep);
    }
    کينډۍ <typename T_موخه_Type, typename T_غوټه>
    ساكن ثابت T_موخه_Type* as(ثابت T_غوټه* nodep) VL_خالص {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || شخصي_Type_Test<T_موخه_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        أرجع unsafe_شخصي_As<T_موخه_Type, T_غوټه>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    کينډۍ <typename T_موخه_Type, typename T_غوټه>
    ساكن T_موخه_Type* unsafe_شخصي_As(T_غوټه* nodep) VL_خالص {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        أرجع reinterpret_cast<T_موخه_Type*>(nodep);
    }
    کينډۍ <typename T_موخه_Type, typename T_غوټه>
    ساكن ثابت T_موخه_Type* unsafe_شخصي_As(ثابت T_غوټه* nodep) VL_خالص {
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_موخه_Type, T_غوټه>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        أرجع reinterpret_cast<ثابت T_موخه_Type*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    کينډۍ <typename T_غوټه>
    ساكن bool mayBeUnder(ثابت Ast_غوټه* nodep) {
        static_assert(!std::is_const<T_غوټه>::ارزښت,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        أرجع true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    کينډۍ <typename T_غوټه>
    ساكن constexpr bool is_ونې_پاڼه() {
        static_assert(!std::is_const<T_غوټه>::ارزښت,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        أرجع false;
    }

خاص:
    // Using std::conditional for const correctness in the public 'foreach' functions
    کينډۍ <typename T_Arg>
    using Const_Correct_Ast_غوټه =
        typename std::conditional<std::is_const<T_Arg>::ارزښت, ثابت Ast_غوټه, Ast_غوټه>::type;

    کينډۍ <typename T_Arg, typename T_Callable>
    مضمن ساكن فراغ foreachImpl(Const_Correct_Ast_غوټه<T_Arg>* nodep, ثابت T_Callable& f,
                                   bool لیدنه_بل);

    کينډۍ <typename T_Arg, bool N_Default, typename T_Callable>
    مضمن ساكن bool predicateImpl(Const_Correct_Ast_غوټه<T_Arg>* nodep, ثابت T_Callable& p,
                                     bool لیدنه_بل);

عام:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    کينډۍ <typename T_Callable>
    فراغ foreach(T_Callable&& f) {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_غوټه*>::ارزښت
                          && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_غوټه>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    کينډۍ <typename T_Callable>
    فراغ foreach(T_Callable&& f) ثابت {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ثابت T_غوټه*>::ارزښت
                && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ثابت T_غوټه>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    کينډۍ <typename T_Callable>
    فراغ foreach_And_بل(T_Callable&& f) {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_غوټه*>::ارزښت
                          && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_غوټه>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    کينډۍ <typename T_Callable>
    فراغ foreach_And_بل(T_Callable&& f) ثابت {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ثابت T_غوټه*>::ارزښت
                && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ثابت T_غوټه>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    کينډۍ <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_غوټه*>::ارزښت
                          && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_غوټه, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    کينډۍ <typename T_Callable>
    bool exists(T_Callable&& p) ثابت {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_غوټه*>::ارزښت
                          && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_غوټه, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    کينډۍ <typename T_Callable>
    bool exists_And_بل(T_Callable&& p) {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_غوټه*>::ارزښت
                          && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_غوټه, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    کينډۍ <typename T_Callable>
    bool exists_And_بل(T_Callable&& p) ثابت {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_غوټه*>::ارزښت
                          && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_غوټه, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    کينډۍ <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_غوټه*>::ارزښت
                          && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_غوټه, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    کينډۍ <typename T_Callable>
    bool forall(T_Callable&& p) ثابت {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_غوټه*>::ارزښت
                          && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_غوټه, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    صحيح غوټه_شمېر() ثابت {
        // TODO: this should really return size_t, but need to fix use sites
        صحيح شمېر = 0;
        this->foreach([&شمېر](ثابت Ast_غوټه*) { ++شمېر; });
        أرجع شمېر;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(صحيح حد) ثابت {
        صحيح شمېر = 0;
        أرجع this->exists([&شمېر, حد](ثابت Ast_غوټه*) { أرجع ++شمېر > حد; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
کينډۍ <>
Ast_غوټه* Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(Ast_غوټه* nodep, Ast_غوټه* newp);

// Inline method implementations
Ast_غوټه* Ast_غوټه::جمع_کړه_بل(Ast_غوټه* newp) { أرجع جمع_کړه_بل(this, newp); }

// Specializations of privateTypeTest
#ضمن "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
کينډۍ <>
مضمن bool Ast_غوټه::mayBeUnder<AstCell>(ثابت Ast_غوټه* nodep) {
    أرجع !VN_IS(nodep, غوټه_Stmt) && !VN_IS(nodep, غوټه_بیان);
}
کينډۍ <>
مضمن bool Ast_غوټه::mayBeUnder<Ast_غوټه_ټاکل>(ثابت Ast_غوټه* nodep) {
    أرجع !VN_IS(nodep, غوټه_بیان);
}
کينډۍ <>
مضمن bool Ast_غوټه::mayBeUnder<AstVarScope>(ثابت Ast_غوټه* nodep) {
    إذا (VN_IS(nodep, VarScope)) أرجع false;  // Should not nest
    إذا (VN_IS(nodep, Var)) أرجع false;
    إذا (VN_IS(nodep, فعال)) أرجع false;
    إذا (VN_IS(nodep, غوټه_Stmt)) أرجع false;
    إذا (VN_IS(nodep, غوټه_بیان)) أرجع false;
    أرجع true;
}
کينډۍ <>
مضمن bool Ast_غوټه::mayBeUnder<Ast_Exec_ګراف>(ثابت Ast_غوټه* nodep) {
    إذا (VN_IS(nodep, Exec_ګراف)) أرجع false;  // Should not nest
    إذا (VN_IS(nodep, غوټه_Stmt)) أرجع false;  // Should be directly under CFunc
    أرجع true;
}
کينډۍ <>
مضمن bool Ast_غوټه::mayBeUnder<Ast_فعال>(ثابت Ast_غوټه* nodep) {
    أرجع !VN_IS(nodep, فعال);  // AstActives do not nest
}
کينډۍ <>
مضمن bool Ast_غوټه::mayBeUnder<AstScope>(ثابت Ast_غوټه* nodep) {
    أرجع !VN_IS(nodep, Scope);  // AstScopes do not nest
}
کينډۍ <>
مضمن bool Ast_غوټه::mayBeUnder<Ast_Sen_ونه>(ثابت Ast_غوټه* nodep) {
    أرجع !VN_IS(nodep, Sen_ونه);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
کينډۍ <>
constexpr bool Ast_غوټه::is_ونې_پاڼه<Ast_غوټه_Var_Ref>() {
    أرجع true;
}
کينډۍ <>
constexpr bool Ast_غوټه::is_ونې_پاڼه<AstVarRef>() {
    أرجع true;
}
کينډۍ <>
constexpr bool Ast_غوټه::is_ونې_پاڼه<AstVarXRef>() {
    أرجع true;
}

// foreach implementation
کينډۍ <typename T_Arg, typename T_Callable>
فراغ Ast_غوټه::foreachImpl(Const_Correct_Ast_غوټه<T_Arg>* nodep, ثابت T_Callable& f, bool لیدنه_بل) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using غوټه_2 = Const_Correct_Ast_غوټه<T_Arg>;

    // We prefetch this far into the stack
    constexpr صحيح Prefetch_واټن = 2;
    // We push max 5 items per iteration
    constexpr اندازه_t Max_Pushes_فی_Iteration = 5;
    // Initial stack sie
    constexpr اندازه_t Initial_ډېری_اندازه = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    اندازه_t ډېری_اندازه = Initial_ډېری_اندازه;
    غوټه_2* دننه_کرښه_ډېری[Initial_ډېری_اندازه];  // In-line initial stack storage
    std::unique_ptr<غوټه_2*[]> بهر_د_کرښه_Stackp{nullptr};  // In case we need more, we will allocate
    غوټه_2** basep = دننه_کرښه_ډېری + Prefetch_واټن;  // Pointer to base of stack
    غوټه_2** topp = basep;  // Pointer to top of stack
    غوټه_2** limp = دننه_کرښه_ډېری + ډېری_اندازه - Max_Pushes_فی_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    لكل (صحيح i = -Prefetch_واټن; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ثابت خپلکاری لیدنه = [&](غوټه_2* currp) {
        // Type test this node
        إذا (Ast_غوټه::شخصي_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            إذا VL_CONSTEXPR_CXX17 (is_ونې_پاڼه<T_Arg_NonConst>()) أرجع;
        }

        // Enqueue children for traversal, unless futile
        إذا (mayBeUnder<T_Arg_NonConst>(currp)) {
            إذا (Ast_غوټه* ثابت op4p = currp->op4p()) *topp++ = op4p;
            إذا (Ast_غوټه* ثابت op3p = currp->op3p()) *topp++ = op3p;
            إذا (Ast_غوټه* ثابت op2p = currp->op2p()) *topp++ = op2p;
            إذا (Ast_غوټه* ثابت op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    إذا (لیدنه_بل && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    لیدنه(nodep);

    // Visit the rest of the tree
    طالما (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        غوټه_2* ثابت headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_واټن]);

        // Ensure we have stack space for nextp and the 4 children
        إذا (VL_UNLIKELY(topp >= limp)) {
            ثابت ptrdiff_t occupancy = topp - basep;
            {
                ثابت اندازه_t new_ډېری_اندازه = ډېری_اندازه * 2;
                غوټه_2** ثابت newStackp = new غوټه_2*[new_ډېری_اندازه];
                std::حافظه_نقل(newStackp, basep - Prefetch_واټن,
                            حجم(غوټه_2**) * (occupancy + Prefetch_واټن));
                ډېری_اندازه = new_ډېری_اندازه;
                بهر_د_کرښه_Stackp.reset(newStackp);
            }
            basep = بهر_د_کرښه_Stackp.get() + Prefetch_واټن;
            topp = basep + occupancy;
            limp = بهر_د_کرښه_Stackp.get() + ډېری_اندازه - Max_Pushes_فی_Iteration;
        }

        // Enqueue the next node
        إذا (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        لیدنه(headp);
    }
}

// predicate implementation
کينډۍ <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_غوټه::predicateImpl(Const_Correct_Ast_غوټه<T_Arg>* nodep, ثابت T_Callable& p,
                            bool لیدنه_بل) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using غوټه_2 = Const_Correct_Ast_غوټه<T_Arg>;

    // We prefetch this far into the stack
    constexpr صحيح Prefetch_واټن = 2;
    // We push max 5 items per iteration
    constexpr اندازه_t Max_Pushes_فی_Iteration = 5;
    // Initial stack sie
    constexpr اندازه_t Initial_ډېری_اندازه = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    اندازه_t ډېری_اندازه = Initial_ډېری_اندازه;
    غوټه_2* دننه_کرښه_ډېری[Initial_ډېری_اندازه];  // In-line initial stack storage
    std::unique_ptr<غوټه_2*[]> بهر_د_کرښه_Stackp{nullptr};  // In case we need more, we will allocate
    غوټه_2** basep = دننه_کرښه_ډېری + Prefetch_واټن;  // Pointer to base of stack
    غوټه_2** topp = basep;  // Pointer to top of stack
    غوټه_2** limp = دننه_کرښه_ډېری + ډېری_اندازه - Max_Pushes_فی_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    لكل (صحيح i = -Prefetch_واټن; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ثابت خپلکاری لیدنه = [&](غوټه_2* currp) {
        // Type test this node
        إذا (Ast_غوټه::شخصي_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            إذا (p(static_cast<T_Arg*>(currp)) != N_Default) أرجع true;
            // Short circuit if iterating leaf nodes
            إذا VL_CONSTEXPR_CXX17 (is_ونې_پاڼه<T_Arg_NonConst>()) أرجع false;
        }

        // Enqueue children for traversal, unless futile
        إذا (mayBeUnder<T_Arg_NonConst>(currp)) {
            إذا (Ast_غوټه* ثابت op4p = currp->op4p()) *topp++ = op4p;
            إذا (Ast_غوټه* ثابت op3p = currp->op3p()) *topp++ = op3p;
            إذا (Ast_غوټه* ثابت op2p = currp->op2p()) *topp++ = op2p;
            إذا (Ast_غوټه* ثابت op1p = currp->op1p()) *topp++ = op1p;
        }

        أرجع false;
    };

    // Enqueue the next of the root node, if required
    إذا (لیدنه_بل && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    إذا (لیدنه(nodep)) أرجع !N_Default;

    // Visit the rest of the tree
    طالما (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        غوټه_2* ثابت headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_واټن]);

        // Ensure we have stack space for nextp and the 4 children
        إذا (VL_UNLIKELY(topp >= limp)) {
            ثابت ptrdiff_t occupancy = topp - basep;
            {
                ثابت اندازه_t new_ډېری_اندازه = ډېری_اندازه * 2;
                غوټه_2** ثابت newStackp = new غوټه_2*[new_ډېری_اندازه];
                std::حافظه_نقل(newStackp, basep - Prefetch_واټن,
                            حجم(غوټه_2**) * (occupancy + Prefetch_واټن));
                ډېری_اندازه = new_ډېری_اندازه;
                بهر_د_کرښه_Stackp.reset(newStackp);
            }
            basep = بهر_د_کرښه_Stackp.get() + Prefetch_واټن;
            topp = basep + occupancy;
            limp = بهر_د_کرښه_Stackp.get() + ډېری_اندازه - Max_Pushes_فی_Iteration;
        }

        // Enqueue the next node
        إذا (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        إذا (لیدنه(headp)) أرجع !N_Default;
    }

    أرجع N_Default;
}

مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت Ast_غوټه* rhs) {
    إذا (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } وإلا إذا (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } وإلا {
        rhs->dump(os);
    }
    أرجع os;
}
فراغ VNRelinker::relink(Ast_غوټه* newp) { newp->Ast_غوټه::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
کينډۍ <typename T_غوټه>
class VNRef final : عام std::reference_wrapper<T_غوټه> {
    static_assert(std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

عام:
    کينډۍ <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_غوټه>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(ثابت std::reference_wrapper<T_غوټه>& other)
        : std::reference_wrapper<T_غوټه>{other} {}
};

static_assert(حجم(VNRef<Ast_غوټه>) == حجم(std::reference_wrapper<Ast_غوټه>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
اندازه_t V3HasherUncachedHash(ثابت Ast_غوټه&);

// Specialization of std::hash for VNRef
کينډۍ <typename T_غوټه>
بنية std::hash<VNRef<T_غوټه>> final {
    اندازه_t چلوونکی()(VNRef<T_غوټه> r) ثابت { أرجع V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
کينډۍ <typename T_غوټه>
بنية std::equal_منزل<VNRef<T_غوټه>> final {
    اندازه_t چلوونکی()(VNRef<T_غوټه> ra, VNRef<T_غوټه> rb) ثابت {
        أرجع ra.get().same_ونه(&(rb.get()));
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
    std::unordered_map<ثابت Ast_غوټه*, Ast_غوټه*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<ثابت Ast_غوټه*, std::ویکټور<Ast_غوټه*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<ثابت Ast_غوټه*, std::string> m_prefixes;
    ثابت VDTypeNameScopes* ثابت m_prevp;  // Index in use before this one
    ساكن ثابت VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    فراغ شاخص(Ast_غوټه* nodep, Ast_غوټه* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

عام:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    ساكن ثابت VDTypeNameScopes* currentp() { أرجع s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(ثابت Ast_غوټه* nodep, Ast_غوټه*& outerpr) ثابت;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ثابت std::ویکټور<Ast_غوټه*>* paramsp(ثابت Ast_غوټه* modp) ثابت;
    // The prefix kept for scope 'scopep', or nullptr if none
    ثابت std::string* prefixp(ثابت Ast_غوټه* scopep) ثابت;
    // Keep 'prefix', final, for scope 'scopep'
    فراغ مختاړی(ثابت Ast_غوټه* scopep, ثابت std::string& مختاړی) ثابت;
};

//######################################################################
// Inline VNVisitor METHODS

فراغ VNVisitorConst::iterateConst(Ast_غوټه* nodep) { nodep->accept(*this); }
فراغ VNVisitorConst::iterateConstNull(Ast_غوټه* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->accept(*this);
}
فراغ VNVisitorConst::iterateChildrenConst(Ast_غوټه* nodep) { nodep->iterateChildrenConst(*this); }
فراغ VNVisitorConst::iterateChildrenBackwardsConst(Ast_غوټه* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
فراغ VNVisitorConst::iterate_And_بل_Const_Null(Ast_غوټه* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->iterate_And_بل_Const(*this);
}

فراغ VNVisitor::iterate(Ast_غوټه* nodep) { nodep->accept(*this); }
فراغ VNVisitor::iterateNull(Ast_غوټه* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->accept(*this);
}
فراغ VNVisitor::iterateChildren(Ast_غوټه* nodep) { nodep->iterateChildren(*this); }
فراغ VNVisitor::iterate_And_بل_Null(Ast_غوټه* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->iterate_And_بل(*this);
}
Ast_غوټه* VNVisitor::iterateSubtreeReturnEdits(Ast_غوټه* nodep) {
    أرجع nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#ضمن "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#ضمن "V3AstNodeDType.h"
#ضمن "V3AstNodeOther.h"
#ضمن "V3AstNodeExpr.h"
#ضمن "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#ضمن "V3AstInlines.h"
فراغ dump_غوټه_List_Json(std::ostream& os, ثابت Ast_غوټه* nodep, ثابت std::string& listName,
                      ثابت string& indent);

#د_که_پای  // Guard
