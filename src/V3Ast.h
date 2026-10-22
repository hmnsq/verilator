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

#جے_بنا_تعریف VERILATOR_V3AST_H_
#تعریف_کرو VERILATOR_V3AST_H_

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
#تعریف_رد VL_USER_TYPE_CHECKS
#جے_انت
#جے_تعریف_شدہ VL_USER_TYPE_CHECKS
#ضمن <variant>
#جے_انت

// clang-format off
#ضمن "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3_گراف;
class ExecMTask;
class VIface_Capture_ٹیگ;

//######################################################################

// For broken() function, return error string if have a match
#تعریف_کرو BROKEN_RTN(test) \
    افعل { \
        إذا (VL_UNCOVERABLE(test)) أرجع "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } طالما (false)
// For broken() function, return error string if a base of this class has a match
#تعریف_کرو BROKEN_ادھار_RTN(test) \
    افعل { \
        ثابت حرف* ثابت reasonp = (test); \
        إذا (VL_UNCOVERABLE(reasonp)) أرجع reasonp; \
    } طالما (false)

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
#إذا defined(VL_DEBUG) || defined(VL_ASAN)
#تعریف_کرو VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#وإلا
#تعریف_کرو VN_DBG_AS(nodep, nodetypename) (Ast_گرہ::unsafe_نجی_As<Ast##nodetypename>(nodep))
#جے_انت

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#تعریف_کرو VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

بنية VNTypeInfo final {
    ثابت حرف* m_namep;
    تعداد OpEn : uint8_t {
        OP_بن_ورتیا,
        OP_ورتیا,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ثابت حرف* m_opNamep[4];
    ناپ_t m_sizeof;
};

class VNType final {
    ساكن ثابت VNTypeInfo s_type_Info_جدول[];

عام:
#ضمن "V3Ast__gen_type_enum.h"  // From ./astgen
    ثابت en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_محفوظ : m_e{_e} {}
    constexpr عامل en() ثابت VL_MT_محفوظ { أرجع m_e; }

    // Retrieve VNTypeInfo for the given type
    ساكن ثابت VNTypeInfo& typeInfo(VNType t) VL_MT_محفوظ { أرجع s_type_Info_جدول[t.m_e]; }
};
constexpr bool عامل==(ثابت VNType& lhs, ثابت VNType& rhs) VL_خالص {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VNType& lhs, VNType::en rhs) VL_خالص { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VNType::en lhs, ثابت VNType& rhs) VL_خالص { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VNType& rhs) { أرجع os << rhs.ascii(); }

//######################################################################
// AstNUser - Generic base class for AST User nodes.
//          - Also used to allow parameter passing up/down iterate calls

class چوڑائی_VP;
class V3_گراف_راس;
class VSymEnt;

class VNUser final {
// Defining VL_USER_TYPE_CHECKS replaces the VNUser union with a std::variant
// which remembers which form of VNUser is being stored to catch mismatched reads.
#جے_تعریف_شدہ VL_USER_TYPE_CHECKS
    // monostate is an unwritten / cleared slot. It can be read as either form
    // and yields nullptr/0.
    std::variant<std::monostate, uint64_t, فراغ*> m_u;
#وإلا
    اتحاد {
        فراغ* اتے;
        uint64_t uq;
    } m_u;
#جے_انت

عام:
#جے_تعریف_شدہ VL_USER_TYPE_CHECKS
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
    سانچا <typename T>
    typename std::enable_if<std::is_اشارہ_گر<T>::قدر, T>::type منزل() ثابت VL_MT_محفوظ {
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
        m_u.اتے = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(فراغ* p) { m_u.اتے = p; }
    ~VNUser() = افتراضي;
    // Casters
    سانچا <typename T>
    typename std::enable_if<std::is_اشارہ_گر<T>::قدر, T>::type منزل() ثابت VL_MT_محفوظ {
        أرجع reinterpret_cast<T>(m_u.اتے);
    }
    uint64_t منزل_UQuad() ثابت { أرجع m_u.uq; }
#جے_انت
    VSymEnt* منزل_Sym_Ent() ثابت { أرجع منزل<VSymEnt*>(); }
    Ast_گرہ* منزل_Nodep() ثابت VL_MT_محفوظ { أرجع منزل<Ast_گرہ*>(); }
    V3_گراف_راس* منزل_گراف_راس() ثابت { أرجع منزل<V3_گراف_راس*>(); }
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

class VNUser_وچ_Use_ادھار
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
class VNUser1_وچ_Use final : VNUser_وچ_Use_ادھار {
محمي:
    یار class Ast_گرہ;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser1_وچ_Use()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1_وچ_Use()     { حرر    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2_وچ_Use final : VNUser_وچ_Use_ادھار {
محمي:
    یار class Ast_گرہ;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser2_وچ_Use()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2_وچ_Use()     { حرر    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3_وچ_Use final : VNUser_وچ_Use_ادھار {
محمي:
    یار class Ast_گرہ;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser3_وچ_Use()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3_وچ_Use()     { حرر    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4_وچ_Use final : VNUser_وچ_Use_ادھار {
محمي:
    یار class Ast_گرہ;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser4_وچ_Use()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4_وچ_Use()     { حرر    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
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
    std::سمتیہ<Ast_گرہ*> m_deleteps;  // Nodes to delete

عام:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    فراغ ڈالو_Deletep(Ast_گرہ* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.ڈالو_پٹھ(nodep);
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
    یار class Ast_گرہ;

عام:
    /// Call visit()s on nodep
    مضمن فراغ iterateConst(Ast_گرہ* nodep);
    /// Call visit()s on nodep
    مضمن فراغ iterateConstNull(Ast_گرہ* nodep);
    /// Call visit()s on const nodep's children
    مضمن فراغ iterateChildrenConst(Ast_گرہ* nodep);
    /// Call visit()s on nodep's children in backp() order
    مضمن فراغ iterateChildrenBackwardsConst(Ast_گرہ* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    مضمن فراغ iterate_And_اگلا_Const_Null(Ast_گرہ* nodep);

    مجازی فراغ ویکھو(Ast_گرہ* nodep) = 0;
    مجازی ~VNVisitorConst() {}
#ضمن "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : عام VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
عام:
    /// Call visit()s on nodep
    مضمن فراغ iterate(Ast_گرہ* nodep);
    /// Call visit()s on nodep
    مضمن فراغ iterateNull(Ast_گرہ* nodep);
    /// Call visit()s on nodep's children
    مضمن فراغ iterateChildren(Ast_گرہ* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    مضمن فراغ iterate_And_اگلا_Null(Ast_گرہ* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    مضمن Ast_گرہ* iterateSubtreeReturnEdits(Ast_گرہ* nodep);

    VNDeleter& deleter() { أرجع m_deleter; }
    فراغ ڈالو_Deletep(Ast_گرہ* nodep) { deleter().ڈالو_Deletep(nodep); }
    فراغ doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
محمي:
    یار class Ast_گرہ;
    تعداد RelinkWhatEn : uint8_t {
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

عام:
    VNRelinker() = افتراضي;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    مضمن فراغ relink(Ast_گرہ* newp);
    فراغ dump(std::ostream& str = std::cout) ثابت;
};
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VNRelinker& rhs) {
    rhs.dump(os);
    أرجع os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{عام : مجازی bool گرہ_میل(ثابت Ast_گرہ* nodep) ثابت {أرجع true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#تعریف_کرو ASTNODE_PREFETCH_NON_NULL(nodep) \
    افعل { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } طالما (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#تعریف_کرو ASTNODE_PREFETCH(nodep) \
    افعل { \
        إذا (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } طالما (false)

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

    بنية {
        bool did_چوڑائی : 1;  // Did V3Width computation
        bool doing_چوڑائی : 1;  // Inside V3Width
        bool protect : 1;  // Protect name if protection is on
        // Space for more flags here (there must be 8 bits in total)
        uint8_t بن_ورتیا : 5;
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
#جے_انت

    Ast_گرہ_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_گرہ* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_سطر* m_fileline;  // Where it was declared
#جے_تعریف_شدہ VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_گنتی;  // When it was last edited
#جے_انت
    ساكن uint64_t s_editCntGbl;  // Global edit counter
    ساكن uint64_t s_edit_Cnt_آخری;  // Last committed value of global edit counter

    Ast_گرہ* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
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
    فراغ op1p(Ast_گرہ* nodep) {
        m_op1p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op2p(Ast_گرہ* nodep) {
        m_op2p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op3p(Ast_گرہ* nodep) {
        m_op3p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op4p(Ast_گرہ* nodep) {
        m_op4p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }

خاص:
    Ast_گرہ* clone_درخت_Iter(bool need_خالص);
    Ast_گرہ* clone_درخت_Iter_List(bool need_خالص);
    فراغ check_درخت_Iter(ثابت Ast_گرہ* prevBackp) ثابت VL_MT_STABLE;
    bool gate_درخت_Iter() ثابت;
    ساكن bool same_درخت_Iter(ثابت Ast_گرہ* node1p, ثابت Ast_گرہ* node2p, bool ign_اگلا,
                             bool gateOnly);
    فراغ delete_درخت_Iter();
    فراغ delete_گرہ();
    string instanceStr() ثابت;

عام:
    فراغ purityCheck();
    ساكن فراغ relinkOneLink(Ast_گرہ*& pointpr, Ast_گرہ* newp);
    // cppcheck-suppress functionConst
    ساكن فراغ debug_درخت_Change(ثابت Ast_گرہ* nodep, ثابت حرف* اگیتر, صحيح lineno, bool اگلا);

محمي:
    // CONSTRUCTORS
    Ast_گرہ(VNType t, File_سطر* fl);
    مجازی ~Ast_گرہ() = افتراضي;  // Use 'deleteTree' instead
    مجازی Ast_گرہ* clone() = 0;  // Generally, cloneTree is what you want instead
    مجازی فراغ cloneRelink() { cloneRelinkGen(); }
    مجازی فراغ cloneRelinkGen() {};  // Overrides generated by 'astgen'
    فراغ clone_Relink_درخت();

    // METHODS
    فراغ سیٹ_Op1p(Ast_گرہ* newp);  // Set non-list-type op1 to non-list element
    فراغ سیٹ_Op2p(Ast_گرہ* newp);  // Set non-list-type op2 to non-list element
    فراغ سیٹ_Op3p(Ast_گرہ* newp);  // Set non-list-type op3 to non-list element
    فراغ سیٹ_Op4p(Ast_گرہ* newp);  // Set non-list-type op4 to non-list element

    فراغ جوڑو_Op1p(Ast_گرہ* newp);  // Append newp to end of op1
    فراغ جوڑو_Op2p(Ast_گرہ* newp);  // Append newp to end of op2
    فراغ جوڑو_Op3p(Ast_گرہ* newp);  // Append newp to end of op3
    فراغ جوڑو_Op4p(Ast_گرہ* newp);  // Append newp to end of op4

    فراغ prependOp1p(Ast_گرہ* newp);  // Prepend newp to front of op1
    فراغ prependOp2p(Ast_گرہ* newp);  // Prepend newp to front of op2
    فراغ prependOp3p(Ast_گرہ* newp);  // Prepend newp to front of op3
    فراغ prependOp4p(Ast_گرہ* newp);  // Prepend newp to front of op4

    // clang-format off
    فراغ سیٹ_NOp1p(Ast_گرہ* newp) { إذا (newp) سیٹ_Op1p(newp); }
    فراغ سیٹ_NOp2p(Ast_گرہ* newp) { إذا (newp) سیٹ_Op2p(newp); }
    فراغ سیٹ_NOp3p(Ast_گرہ* newp) { إذا (newp) سیٹ_Op3p(newp); }
    فراغ سیٹ_NOp4p(Ast_گرہ* newp) { إذا (newp) سیٹ_Op4p(newp); }

    فراغ جوڑو_NOp1p(Ast_گرہ* newp) { إذا (newp) جوڑو_Op1p(newp); }
    فراغ جوڑو_NOp2p(Ast_گرہ* newp) { إذا (newp) جوڑو_Op2p(newp); }
    فراغ جوڑو_NOp3p(Ast_گرہ* newp) { إذا (newp) جوڑو_Op3p(newp); }
    فراغ جوڑو_NOp4p(Ast_گرہ* newp) { إذا (newp) جوڑو_Op4p(newp); }

    فراغ prependNOp1p(Ast_گرہ* newp) { إذا (newp) prependOp1p(newp); }
    فراغ prependNOp2p(Ast_گرہ* newp) { إذا (newp) prependOp2p(newp); }
    فراغ prependNOp3p(Ast_گرہ* newp) { إذا (newp) prependOp3p(newp); }
    فراغ prependNOp4p(Ast_گرہ* newp) { إذا (newp) prependOp4p(newp); }
    // clang-format on

    فراغ clonep(Ast_گرہ* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    ساكن فراغ clone_Clear_درخت() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    مجازی bool same_گرہ(ثابت Ast_گرہ*) ثابت { أرجع true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    مجازی bool wouldBreakGen(ثابت Ast_گرہ* ثابت oldp,
                               ثابت Ast_گرہ* ثابت newp) ثابت
        = 0;  // Generated by 'astgen'

عام:
    // ACCESSORS
    VNType type() ثابت VL_MT_محفوظ { أرجع m_type; }
    ثابت حرف* typeName() ثابت VL_MT_محفوظ { أرجع type().ascii(); }  // See also prettyTypeName
    Ast_گرہ* nextp() ثابت VL_MT_STABLE { أرجع m_nextp; }
    Ast_گرہ* backp() ثابت VL_MT_STABLE { أرجع m_backp; }
    Ast_گرہ* abovep() ثابت;  // Get parent node above, only for list head and tail
    Ast_گرہ* aboveLoopp() ثابت;  // Get parent node above, may have performance issues as loops
    Ast_گرہ* lastp() ثابت {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        أرجع m_headtailp;
    }
    Ast_گرہ* prevp() ثابت {  // Previous node in list, nullptr for head of list
        إذا (m_backp->m_nextp != this) أرجع nullptr;
        أرجع m_backp;
    }
    Ast_گرہ* op1p() ثابت VL_MT_STABLE { أرجع m_op1p; }
    Ast_گرہ* op2p() ثابت VL_MT_STABLE { أرجع m_op2p; }
    Ast_گرہ* op3p() ثابت VL_MT_STABLE { أرجع m_op3p; }
    Ast_گرہ* op4p() ثابت VL_MT_STABLE { أرجع m_op4p; }
    Ast_گرہ_DType* dtypep() ثابت VL_MT_STABLE { أرجع m_dtypep; }
    Ast_گرہ* clonep() ثابت { أرجع ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_گرہ* پہلا_Abovep() ثابت {  // Returns nullptr when second or later in list
        أرجع ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ثابت VL_MT_محفوظ { أرجع m_brokenState; }
    فراغ brokenState(uint8_t قدر) { m_brokenState = قدر; }

    // Used by AstNode::broken()
    bool brokeExists() ثابت { أرجع V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ثابت { أرجع brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    مجازی فراغ foreachLink(std::function<فراغ(Ast_گرہ** linkpp, ثابت حرف* namep)> f) = 0;

#إذا defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    ساكن فراغ* عامل new(ناپ_t ناپ);
    ساكن فراغ عامل delete(فراغ* obj, ناپ_t ناپ);
#جے_انت

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    ساكن constexpr صحيح INSTR_گنتی_BRANCH = 4;  // Branch
    ساكن constexpr صحيح INSTR_گنتی_CALL = INSTR_گنتی_BRANCH + 10;  // Subroutine call
    ساكن constexpr صحيح INSTR_گنتی_LD = 2;  // Load memory
    ساكن constexpr صحيح INSTR_گنتی_INT_MUL = 3;  // Integer multiply
    ساكن constexpr صحيح INSTR_گنتی_INT_DIV = 10;  // Integer divide
    ساكن constexpr صحيح INSTR_گنتی_DBL = 8;  // Convert or do float ops
    ساكن constexpr صحيح INSTR_گنتی_DBL_DIV = 40;  // Double divide
    ساكن constexpr صحيح INSTR_گنتی_DBL_TRIG = 200;  // Double trigonometric ops
    ساكن constexpr صحيح INSTR_گنتی_STR = 100;  // String ops
    ساكن constexpr صحيح INSTR_گنتی_ویلا = INSTR_گنتی_CALL + 5;  // Determine simulation time
    ساكن constexpr صحيح INSTR_گنتی_PLI = 20;  // PLI routines
    ساكن constexpr صحيح INSTR_گنتی_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    مجازی string name() ثابت VL_MT_STABLE { أرجع ""; }
    مجازی string origName() ثابت { أرجع ""; }
    string prettyDehashOrigOrName() ثابت {
        أرجع prettyName(VName::dehash(origName().خالی_تھاواں() ? name() : origName()));
    }
    مجازی فراغ name(ثابت string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    مجازی فراغ ٹیگ(ثابت string& متن) {}
    مجازی string ٹیگ() ثابت { أرجع ""; }
    مجازی uint32_t decl_Token_عدد() ثابت { أرجع 0; }
    مجازی فراغ decl_Token_عدد_سیٹ_Min(uint32_t token_عدد) {}
    مجازی string verilogKwd() ثابت { أرجع ""; }
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
    ساكن string encode_نمبر(int64_t عدد);  // Encode number into internal C representation
    ساكن string vcdName(ثابت string& namein);  // Name for printing out to vcd files
    string prettyName() ثابت { أرجع prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    مجازی string pretty_Name_پیغام() ثابت { أرجع prettyName(); }
    string prettyNameQ() ثابت { أرجع "'"s + pretty_Name_پیغام() + "'"; }  // Quoted, for messages
    string verilogName() ثابت { أرجع vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ثابت;
    مجازی string prettyOperatorName() ثابت { أرجع "operator " + prettyTypeName(); }
    File_سطر* fileline() ثابت VL_MT_محفوظ { أرجع m_fileline; }
    فراغ fileline(File_سطر* fl) { m_fileline = fl; }
    مضمن bool width1() ثابت;
    مضمن صحيح چوڑائی_Instrs() ثابت;
    bool did_چوڑائی() ثابت { أرجع m_flags.did_چوڑائی; }
    فراغ did_چوڑائی(bool flag) { m_flags.did_چوڑائی = flag; }
    bool did_چوڑائی_And_سیٹ() {
        إذا (did_چوڑائی()) أرجع true;
        did_چوڑائی(true);
        أرجع false;
    }
    bool doing_چوڑائی() ثابت { أرجع m_flags.doing_چوڑائی; }
    فراغ doing_چوڑائی(bool flag) { m_flags.doing_چوڑائی = flag; }
    bool protect() ثابت VL_MT_محفوظ { أرجع m_flags.protect; }
    فراغ protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    مضمن صحيح چوڑائی() ثابت VL_MT_STABLE;
    مضمن صحيح چوڑائی_Min() ثابت VL_MT_STABLE;
    صحيح چوڑائی_Min_V() ثابت {
        أرجع v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::VERILOG_چوڑائی ? چوڑائی_Min() : چوڑائی();
    }
    صحيح چوڑائی_الفاظ() ثابت { أرجع VL_الفاظ_I(چوڑائی()); }
    bool isQuad() ثابت VL_MT_STABLE { أرجع (چوڑائی() > VL_IDATASIZE && چوڑائی() <= VL_QUADSIZE); }
    bool isWide() ثابت VL_MT_STABLE { أرجع (چوڑائی() > VL_QUADSIZE); }
    مضمن bool isCHandle() ثابت VL_MT_STABLE;
    مضمن bool isDouble() ثابت VL_MT_STABLE;
    مضمن bool isSigned() ثابت VL_MT_STABLE;
    مضمن bool isString() ثابت VL_MT_STABLE;
    مضمن bool isEvent() ثابت VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        أرجع ((m_user1Cnt == VNUser1_وچ_Use::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_گرہ* user1p() ثابت VL_MT_STABLE { أرجع user1u().منزل_Nodep(); }
    فراغ user1u(ثابت VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1_وچ_Use::s_userCntGbl; }
    فراغ user1p(فراغ* userp) { user1u(VNUser{userp}); }
    فراغ user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ثابت { أرجع user1u().منزل_UQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ثابت uint64_t v = user1(); user1(v + val); أرجع v; }
    uint64_t user1Or(uint64_t val) { ثابت uint64_t v = user1(); user1(v | val); أرجع v; }
    uint64_t user1_سیٹ_Once() { ثابت uint64_t v = user1(); إذا (!v) user1(1); أرجع v; }  // Better for cache than user1Inc()
    ساكن فراغ user1_Clear_درخت() { VNUser1_وچ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        أرجع ((m_user2Cnt == VNUser2_وچ_Use::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_گرہ* user2p() ثابت VL_MT_STABLE { أرجع user2u().منزل_Nodep(); }
    فراغ user2u(ثابت VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2_وچ_Use::s_userCntGbl; }
    فراغ user2p(فراغ* userp) { user2u(VNUser{userp}); }
    فراغ user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ثابت { أرجع user2u().منزل_UQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ثابت uint64_t v = user2(); user2(v + val); أرجع v; }
    uint64_t user2Or(uint64_t val) { ثابت uint64_t v = user2(); user2(v | val); أرجع v; }
    uint64_t user2_سیٹ_Once() { ثابت uint64_t v = user2(); إذا (!v) user2(1); أرجع v; }  // Better for cache than user2Inc()
    ساكن فراغ user2_Clear_درخت() { VNUser2_وچ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        أرجع ((m_user3Cnt == VNUser3_وچ_Use::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_گرہ* user3p() ثابت VL_MT_STABLE { أرجع user3u().منزل_Nodep(); }
    فراغ user3u(ثابت VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3_وچ_Use::s_userCntGbl; }
    فراغ user3p(فراغ* userp) { user3u(VNUser{userp}); }
    فراغ user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ثابت { أرجع user3u().منزل_UQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ثابت uint64_t v = user3(); user3(v + val); أرجع v; }
    uint64_t user3Or(uint64_t val) { ثابت uint64_t v = user3(); user3(v | val); أرجع v; }
    uint64_t user3_سیٹ_Once() { ثابت uint64_t v = user3(); إذا (!v) user3(1); أرجع v; }  // Better for cache than user3Inc()
    ساكن فراغ user3_Clear_درخت() { VNUser3_وچ_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        أرجع ((m_user4Cnt == VNUser4_وچ_Use::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_گرہ* user4p() ثابت VL_MT_STABLE { أرجع user4u().منزل_Nodep(); }
    فراغ user4u(ثابت VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4_وچ_Use::s_userCntGbl; }
    فراغ user4p(فراغ* userp) { user4u(VNUser{userp}); }
    فراغ user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ثابت { أرجع user4u().منزل_UQuad(); }
    uint64_t user4Or(uint64_t val) { ثابت uint64_t v = user4(); user4(v | val); أرجع v; }
    uint64_t user4Inc(uint64_t val = 1) { ثابت uint64_t v = user4(); user4(v + val); أرجع v; }
    uint64_t user4_سیٹ_Once() { ثابت uint64_t v = user4(); إذا (!v) user4(1); أرجع v; }  // Better for cache than user4Inc()
    ساكن فراغ user4_Clear_درخت() { VNUser4_وچ_Use::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#جے_تعریف_شدہ VL_DEBUG
    uint64_t edit_گنتی() ثابت { أرجع m_edit_گنتی; }
    فراغ edit_گنتی_Inc() {
        m_edit_گنتی = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_درخت();  // Any edit clears all caching
    }
#وإلا
    فراغ edit_گنتی_Inc() { ++s_editCntGbl; }
#جے_انت
    ساكن uint64_t edit_گنتی_آخری() VL_MT_محفوظ { أرجع s_edit_Cnt_آخری; }
    ساكن uint64_t edit_گنتی_Gbl() VL_MT_محفوظ { أرجع s_editCntGbl; }
    ساكن فراغ edit_گنتی_سیٹ_آخری() { s_edit_Cnt_آخری = edit_گنتی_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_قطار_ڈالو_Self_Stmt();
    مضمن bool is_Class_Handle_قدر() ثابت;
    مضمن bool isNull() ثابت;
    مضمن bool isZero() ثابت;
    مضمن bool isOne() ثابت;
    مضمن bool isNeqZero() ثابت;
    مضمن bool isAllOnes() ثابت;
    مضمن bool isAllOnesV() ثابت;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    فراغ dtypep(Ast_گرہ_DType* nodep) {
        إذا (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_گنتی_Inc();
        }
    }
    فراغ dtypeFrom(ثابت Ast_گرہ* fromp) {
        إذا (fromp) dtypep(fromp->dtypep());
    }
    فراغ dtypeChgSigned(bool flag = true);
    فراغ dtype_Chg_چوڑائی(صحيح چوڑائی, صحيح چوڑائی_Min);
    فراغ dtype_Chg_چوڑائی_Signed(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric);
    فراغ dtype_سیٹ_ثنائی_ہندسہ_Unsized(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric) {
        dtypep(لبھو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی_Min, numeric));
    }
    فراغ dtype_سیٹ_ثنائی_ہندسہ_Sized(صحيح چوڑائی, VSigning numeric) {
        dtypep(لبھو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی, numeric));  // Since sized, widthMin is width
    }
    فراغ dtype_سیٹ_Logic_Unsized(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric) {
        dtypep(لبھو_Logic_DType(چوڑائی, چوڑائی_Min, numeric));
    }
    فراغ dtype_سیٹ_Logic_Sized(صحيح چوڑائی, VSigning numeric) {
        dtypep(لبھو_Logic_DType(چوڑائی, چوڑائی, numeric));  // Since sized, widthMin is width
    }
    فراغ dtype_سیٹ_Logic2_State_Unsized(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric) {
        dtypep(لبھو_Logic2_State_DType(چوڑائی, چوڑائی_Min, numeric));
    }
    فراغ dtype_سیٹ_Logic2_State_Sized(صحيح چوڑائی, VSigning numeric) {
        dtypep(لبھو_Logic2_State_DType(چوڑائی, چوڑائی, numeric));  // Since sized, widthMin is width
    }
    فراغ dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric, bool isFourstate) {
        dtypep(لبھو_ثنائی_ہندسہ_Or_Logic_DType(چوڑائی, چوڑائی_Min, numeric, isFourstate));
    }
    فراغ dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Sized(صحيح چوڑائی, VSigning numeric, bool isFourstate) {
        dtypep(لبھو_ثنائی_ہندسہ_Or_Logic_DType(چوڑائی, چوڑائی, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    فراغ dtype_سیٹ_ثنائی_ہندسہ() { dtypep(لبھو_ثنائی_ہندسہ_DType()); }
    فراغ dtype_سیٹ_Logic() { dtypep(لبھو_Logic_DType()); }
    فراغ dtype_سیٹ_Logic2_State() { dtypep(لبھو_Logic2_State_DType()); }
    فراغ dtype_سیٹ_Double() { dtypep(لبھو_Double_DType()); }
    فراغ dtype_سیٹ_Int() { dtypep(لبھو_Int_DType()); }
    فراغ dtype_سیٹ_Integer() { dtypep(لبھو_Integer_DType()); }
    فراغ dtype_سیٹ_Integer2_State() { dtypep(لبھو_Integer2_State_DType()); }
    فراغ dtype_سیٹ_String() { dtypep(لبھو_String_DType()); }
    فراغ dtype_سیٹ_UInt32() { dtypep(لبھو_UInt32_DType()); }  // Twostate
    فراغ dtype_سیٹ_UInt64() { dtypep(لبھو_UInt64_DType()); }  // Twostate
    فراغ dtype_سیٹ_ویلا() { dtypep(لبھو_ویلا_DType()); }
    فراغ dtype_سیٹ_Time2_State() { dtypep(لبھو_Time2_State_DType()); }
    فراغ dtype_سیٹ_خالی_تھاواں_قطار() { dtypep(لبھو_خالی_تھاواں_قطار_DType()); }
    فراغ dtype_سیٹ_دھارا() { dtypep(لبھو_دھارا_DType()); }
    فراغ dtype_سیٹ_Void() { dtypep(لبھو_Void_DType()); }
    فراغ dtype_سیٹ_ثنائی_ہندسہ_Or_Logic(bool isFourstate) {
        dtypep(isFourstate ? لبھو_Logic_DType() : لبھو_ثنائی_ہندسہ_DType());
    }

    // Data type locators
    Ast_گرہ_DType* لبھو_ثنائی_ہندسہ_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::ثنائی_ہندسہ); }
    Ast_گرہ_DType* لبھو_Logic_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::LOGIC); }
    Ast_گرہ_DType* لبھو_Null_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::NULLTYPE); }
    Ast_گرہ_DType* لبھو_Logic2_State_DType() ثابت {
        أرجع لبھو_Basic_DType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_گرہ_DType* لبھو_Double_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::DOUBLE); }
    Ast_گرہ_DType* لبھو_Int_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::INT); }
    Ast_گرہ_DType* لبھو_Integer_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::INTEGER); }
    Ast_گرہ_DType* لبھو_Integer2_State_DType() ثابت {
        أرجع لبھو_Basic_DType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_گرہ_DType* لبھو_String_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::STRING); }
    Ast_گرہ_DType* لبھو_Signed8_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::BYTE); }
    Ast_گرہ_DType* لبھو_UInt32_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::UINT32); }
    Ast_گرہ_DType* لبھو_UInt64_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::UINT64); }
    Ast_گرہ_DType* لبھو_ویلا_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::ویلا); }
    Ast_گرہ_DType* لبھو_Time2_State_DType() ثابت {
        أرجع لبھو_Basic_DType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_گرہ_DType* لبھو_CHandle_DType() ثابت { أرجع لبھو_Basic_DType(VBasicDTypeKwd::CHANDLE); }
    Ast_گرہ_DType* لبھو_Constraint_Ref_DType() ثابت;
    Ast_گرہ_DType* لبھو_خالی_تھاواں_قطار_DType() ثابت;
    Ast_گرہ_DType* لبھو_قطار_اشاریہ_DType() ثابت;
    Ast_گرہ_DType* لبھو_دھارا_DType() ثابت;
    Ast_گرہ_DType* لبھو_Void_DType() ثابت;
    Ast_گرہ_DType* لبھو_ثنائی_ہندسہ_DType(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric) ثابت;
    Ast_گرہ_DType* لبھو_Logic_DType(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric) ثابت;
    Ast_گرہ_DType* لبھو_Logic2_State_DType(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric) ثابت;
    Ast_گرہ_DType* لبھو_ثنائی_ہندسہ_Or_Logic_DType(صحيح چوڑائی, صحيح چوڑائی_Min, VSigning numeric,
                                      bool isFourstate) ثابت {
        أرجع isFourstate ? لبھو_Logic_DType(چوڑائی, چوڑائی_Min, numeric)
                           : لبھو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی_Min, numeric);
    }
    Ast_گرہ_DType* لبھو_ثنائی_ہندسہ_Or_Logic_Range_DType(ثابت VNumRange& range, ثابت صحيح چوڑائی_Min,
                                           ثابت VSigning numeric,
                                           ثابت bool isFourstate) ثابت VL_MT_STABLE {
        أرجع isFourstate ? لبھو_Logic_Range_DType(range, چوڑائی_Min, numeric)
                           : لبھو_ثنائی_ہندسہ_Range_DType(range, چوڑائی_Min, numeric);
    }
    Ast_گرہ_DType* لبھو_Logic_Range_DType(ثابت VNumRange& range, صحيح چوڑائی_Min,
                                      VSigning numeric) ثابت VL_MT_STABLE;
    Ast_گرہ_DType* لبھو_ثنائی_ہندسہ_Range_DType(ثابت VNumRange& range, صحيح چوڑائی_Min,
                                    VSigning numeric) ثابت VL_MT_STABLE;
    Ast_گرہ_DType* لبھو_Basic_DType(VBasicDTypeKwd kwd) ثابت;
    ساكن AstBasicDType* لبھو_داخل_کرو_Same_DType(AstBasicDType* nodep);

    ساكن VCastable computeCastable(ثابت Ast_گرہ_DType* منزل_Dtp, ثابت Ast_گرہ_DType* fromDtp,
                                     ثابت Ast_گرہ* fromConstp, bool checkIfaceArgCompat = false);
    ساكن Ast_گرہ_DType* getCommonClassTypep(Ast_گرہ* nodep1, Ast_گرہ* nodep2);

    // METHODS - dump and error
    فراغ v3error_سرا(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_سرا_Fatal(ثابت std::ostringstream& str) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
        أرجع fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ثابت { أرجع fileline()->warnContextSecondary(); }
    string warn_ہور() ثابت VL_REQUIRES(V3Error::s().m_mutex) { أرجع fileline()->warn_ہور(); }
    string warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) { أرجع fileline()->warnOther(); }

    مجازی فراغ dump(std::ostream& str = std::cout) ثابت;
    ساكن حرف* dump_درخت_Json_Gdb(ثابت Ast_گرہ* nodep);  // For GDB only, free()d by caller
    ساكن حرف* dump_درخت_Json_Gdb(ثابت حرف* str);  // For GDB only, free()d by caller
    ساكن حرف* dump_درخت_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    ساكن فراغ dumpGdb(ثابت Ast_گرہ* nodep);  // For GDB only
    فراغ dump_Gdb_سرخی() ثابت;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    سانچا <typename T_گرہ_نتیجہ, typename T_گرہ_اگلا>
    ساكن T_گرہ_نتیجہ* جوڑو_اگلا(T_گرہ_نتیجہ* nodep, T_گرہ_اگلا* newp) {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ_نتیجہ>::قدر,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_ادھار_دا<T_گرہ_نتیجہ, T_گرہ_اگلا>::قدر,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        إذا (!newp) أرجع nodep;
        أرجع static_cast<T_گرہ_نتیجہ*>(جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, newp));
    }
    سانچا <typename T_گرہ_نتیجہ, typename T_گرہ_اگلا>
    ساكن T_گرہ_نتیجہ* جوڑو_اگلا_Null(T_گرہ_نتیجہ* nodep, T_گرہ_اگلا* newp) {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ_نتیجہ>::قدر,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_ادھار_دا<T_گرہ_نتیجہ, T_گرہ_اگلا>::قدر,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        إذا (!newp) أرجع nodep;
        إذا (!nodep) أرجع newp;
        أرجع static_cast<T_گرہ_نتیجہ*>(جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, newp));
    }
    مضمن Ast_گرہ* جوڑو_اگلا(Ast_گرہ* newp);
    فراغ جوڑو_اگلا_ایتھے(Ast_گرہ* newp);  // Insert newp at this->nextp
    فراغ جوڑو_ایتھے_This_As_اگلا(Ast_گرہ* newp);  // Adds at old place of this, this becomes next
    فراغ replaceWith(Ast_گرہ* newp);  // Replace current node in tree with new node
    فراغ replaceWithKeepDType(Ast_گرہ* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_گرہ* unlink_Fr_پٹھ(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_گرہ* unlink_Fr_پٹھ_With_اگلا(VNRelinker* linkerp = nullptr);
    فراغ relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    فراغ clone_Relink_گرہ() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_گرہ* clone_درخت(bool clone_اگلا_Link,
                       bool need_خالص = false);  // Not const, as sets clonep() on original nodep
    Ast_گرہ* clone_درخت_خالص(bool clone_اگلا_Link) { أرجع clone_درخت(clone_اگلا_Link, true); }
    bool gate_درخت() { أرجع gate_درخت_Iter(); }  // Is tree isGateOptimizable?
    مضمن bool same_درخت(ثابت Ast_گرہ* node2p) ثابت;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    مضمن bool same_Gate_درخت(ثابت Ast_گرہ* node2p) ثابت;
    فراغ delete_درخت();  // Always deletes the next link
    فراغ check_درخت() ثابت VL_MT_STABLE {
        إذا (v3Global.opt.debugCheck()) check_درخت_Iter(backp());
    }
    فراغ checkIter() ثابت;
    فراغ dumpPtrs(std::ostream& os = std::cout) ثابت;
    فراغ dump_درخت(std::ostream& os = std::cout, ثابت string& indent = "    ",
                  صحيح max_ڈونگھائی = 0) ثابت;
    فراغ dump_درخت(ثابت string& indent, صحيح max_ڈونگھائی = 0) ثابت {
        dump_درخت(std::cout, indent, max_ڈونگھائی);
    }
    ساكن فراغ dump_درخت_Gdb(ثابت Ast_گرہ* nodep);  // For GDB only
    فراغ dump_درخت_And_اگلا(std::ostream& os = std::cout, ثابت string& indent = "    ",
                         صحيح max_ڈونگھائی = 0) ثابت;
    فراغ dump_درخت_File(ثابت string& filename, bool doDump = true);
    ساكن فراغ dump_درخت_File_Gdb(ثابت Ast_گرہ* nodep, ثابت حرف* filenamep = nullptr);
    فراغ dump_درخت_Dot(std::ostream& os = std::cout) ثابت;
    فراغ dump_درخت_Dot_File(ثابت string& filename, bool doDump = true);
    مجازی فراغ dumpJson(std::ostream& os) ثابت { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    فراغ dumpJsonGen(std::ostream& os) ثابت {};
    مجازی فراغ dump_درخت_Json_Op_Gen(std::ostream& os, ثابت string& indent) ثابت {};
    فراغ dump_درخت_Json(std::ostream& os, ثابت string& indent = "") ثابت;
    فراغ dump_درخت_Json_File(ثابت string& filename, bool doDump = true);
    ساكن فراغ dumpJsonMetaFileGdb(ثابت حرف* filename);
    ساكن فراغ dumpJsonMetaFile(ثابت string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    ساكن std::string گرہ_Addr(ثابت Ast_گرہ* nodep) {
        أرجع v3Global.opt.dump_درخت_Addrids() ? v3Global.ptr_منزل_Id(nodep) : cvt_منزل_Hex(nodep);
    }

    // METHODS - static advancement
    ساكن Ast_گرہ* afterCommentp(Ast_گرہ* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        طالما (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    مجازی bool isBrancher() ثابت { أرجع false; }
    // Else a AstTime etc that can't be pushed out
    مجازی bool isGateOptimizable() ثابت { أرجع !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    مجازی bool isGateDedupable() ثابت { أرجع isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    مجازی bool is_اظہاریہ_Coverage_Eligible() ثابت { أرجع isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    مجازی bool isOutputter() { أرجع false; }
    // Else a AstTime etc which output can't be predicted from input
    مجازی bool isPredictOptimizable() ثابت { أرجع !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    مجازی bool is_خالص() { أرجع true; }
    // Iff isPure on current node and any nextp()
    bool is_خالص_And_اگلا() { أرجع is_خالص() && (!nextp() || nextp()->is_خالص()); }
    // An event control, delay, wait, etc.
    مجازی bool isTimingControl() ثابت { أرجع false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    مجازی bool isUnlikely() ثابت { أرجع false; }
    // Is an IEEE system function (versus internally-generated)
    مجازی bool isSystemFunc() ثابت { أرجع false; }
    مجازی صحيح instr_گنتی() ثابت { أرجع 0; }
    // Iff node is identical to another node
    مجازی bool isSame(ثابت Ast_گرہ* samep) ثابت {
        أرجع type() == samep->type() && same_گرہ(samep);
    }
    // Iff has a data type; dtype() must be non-null
    مجازی bool hasDType() ثابت VL_MT_محفوظ { أرجع false; }
    // Iff has a non-null childDTypep(), as generic node function
    مجازی Ast_گرہ_DType* get_بچہ_DTypep() ثابت { أرجع nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    مجازی Ast_گرہ_DType* getChild2DTypep() ثابت { أرجع nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    مجازی bool maybe_Pointed_منزل() ثابت VL_MT_محفوظ { أرجع false; }
    // Don't reclaim this node in V3Dead
    مجازی bool undead() ثابت { أرجع false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    مجازی ثابت حرف* broken() ثابت { أرجع nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    مجازی ثابت حرف* brokenGen() ثابت = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ثابت Ast_گرہ* ثابت newp) ثابت { أرجع backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    مجازی فراغ accept(VNVisitorConst& v) = 0;

محمي:
    // All VNVisitor related functions are called as methods off the visitor
    یار class VNVisitor;
    یار class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    فراغ iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    فراغ iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    فراغ iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    فراغ iterate_And_اگلا(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    فراغ iterate_And_اگلا_Const(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_گرہ* iterateSubtreeReturnEdits(VNVisitor& v);

    ساكن فراغ dump_Json_عدد(std::ostream& os, ثابت std::string& name, int64_t val);
    ساكن فراغ dumpJsonBoolIf(std::ostream& os, ثابت std::string& name, bool val);
    ساكن فراغ dumpJsonStr(std::ostream& os, ثابت std::string& name, ثابت std::string& val);
    ساكن فراغ dumpJsonPtr(std::ostream& os, ثابت std::string& name, ثابت Ast_گرہ* ثابت valp);

محمي:
    فراغ iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    سانچا <typename T>
    مضمن ساكن bool نجی_Type_Test(ثابت Ast_گرہ* nodep);

    // For internal use only.
    سانچا <typename T_نشانہ_Type, typename T_DeclType>
    constexpr ساكن bool uselessCast() VL_خالص {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_اشارہ_گر<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        أرجع std::is_ادھار_دا<T_نشانہ_Type, NonCV>::قدر;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    سانچا <typename T_نشانہ_Type, typename T_DeclType>
    constexpr ساكن bool impossibleCast() VL_خالص {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_اشارہ_گر<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        أرجع !std::is_ادھار_دا<NonCV, T_نشانہ_Type>::قدر;
    }  // LCOV_EXCL_STOP

عام:
    // For use via the VN_IS macro only, or in templated code
    سانچا <typename T_نشانہ_Type, typename T_گرہ>
    ساكن bool is(ثابت T_گرہ* nodep) VL_MT_محفوظ {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        أرجع nodep && نجی_Type_Test<T_نشانہ_Type>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    سانچا <typename T_نشانہ_Type, typename T_گرہ>
    ساكن T_نشانہ_Type* cast(T_گرہ* nodep) VL_MT_محفوظ {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        أرجع nodep && نجی_Type_Test<T_نشانہ_Type>(nodep)
                   ? reinterpret_cast<T_نشانہ_Type*>(nodep)
                   : nullptr;
    }
    سانچا <typename T_نشانہ_Type, typename T_گرہ>
    ساكن ثابت T_نشانہ_Type* cast(ثابت T_گرہ* nodep) VL_MT_محفوظ {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        أرجع nodep && نجی_Type_Test<T_نشانہ_Type>(nodep)
                   ? reinterpret_cast<ثابت T_نشانہ_Type*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    سانچا <typename T_نشانہ_Type, typename T_گرہ>
    ساكن T_نشانہ_Type* as(T_گرہ* nodep) VL_خالص {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || نجی_Type_Test<T_نشانہ_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        أرجع unsafe_نجی_As<T_نشانہ_Type, T_گرہ>(nodep);
    }
    سانچا <typename T_نشانہ_Type, typename T_گرہ>
    ساكن ثابت T_نشانہ_Type* as(ثابت T_گرہ* nodep) VL_خالص {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || نجی_Type_Test<T_نشانہ_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        أرجع unsafe_نجی_As<T_نشانہ_Type, T_گرہ>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    سانچا <typename T_نشانہ_Type, typename T_گرہ>
    ساكن T_نشانہ_Type* unsafe_نجی_As(T_گرہ* nodep) VL_خالص {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        أرجع reinterpret_cast<T_نشانہ_Type*>(nodep);
    }
    سانچا <typename T_نشانہ_Type, typename T_گرہ>
    ساكن ثابت T_نشانہ_Type* unsafe_نجی_As(ثابت T_گرہ* nodep) VL_خالص {
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_نشانہ_Type, T_گرہ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        أرجع reinterpret_cast<ثابت T_نشانہ_Type*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    سانچا <typename T_گرہ>
    ساكن bool mayBeUnder(ثابت Ast_گرہ* nodep) {
        static_assert(!std::is_const<T_گرہ>::قدر,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        أرجع true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    سانچا <typename T_گرہ>
    ساكن constexpr bool is_پتی() {
        static_assert(!std::is_const<T_گرہ>::قدر,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        أرجع false;
    }

خاص:
    // Using std::conditional for const correctness in the public 'foreach' functions
    سانچا <typename T_Arg>
    using Const_Correct_Ast_گرہ =
        typename std::conditional<std::is_const<T_Arg>::قدر, ثابت Ast_گرہ, Ast_گرہ>::type;

    سانچا <typename T_Arg, typename T_Callable>
    مضمن ساكن فراغ foreachImpl(Const_Correct_Ast_گرہ<T_Arg>* nodep, ثابت T_Callable& f,
                                   bool ویکھو_اگلا);

    سانچا <typename T_Arg, bool N_Default, typename T_Callable>
    مضمن ساكن bool predicateImpl(Const_Correct_Ast_گرہ<T_Arg>* nodep, ثابت T_Callable& p,
                                     bool ویکھو_اگلا);

عام:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    سانچا <typename T_Callable>
    فراغ foreach(T_Callable&& f) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_گرہ*>::قدر
                          && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_گرہ>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    سانچا <typename T_Callable>
    فراغ foreach(T_Callable&& f) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ثابت T_گرہ*>::قدر
                && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ثابت T_گرہ>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    سانچا <typename T_Callable>
    فراغ foreach_And_اگلا(T_Callable&& f) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_گرہ*>::قدر
                          && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_گرہ>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    سانچا <typename T_Callable>
    فراغ foreach_And_اگلا(T_Callable&& f) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ثابت T_گرہ*>::قدر
                && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ثابت T_گرہ>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    سانچا <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_گرہ*>::قدر
                          && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_گرہ, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    سانچا <typename T_Callable>
    bool exists(T_Callable&& p) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_گرہ*>::قدر
                          && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_گرہ, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    سانچا <typename T_Callable>
    bool exists_And_اگلا(T_Callable&& p) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_گرہ*>::قدر
                          && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_گرہ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    سانچا <typename T_Callable>
    bool exists_And_اگلا(T_Callable&& p) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_گرہ*>::قدر
                          && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_گرہ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    سانچا <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_گرہ*>::قدر
                          && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_گرہ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    سانچا <typename T_Callable>
    bool forall(T_Callable&& p) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_گرہ*>::قدر
                          && std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_گرہ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    صحيح گرہ_گنتی() ثابت {
        // TODO: this should really return size_t, but need to fix use sites
        صحيح گنتی = 0;
        this->foreach([&گنتی](ثابت Ast_گرہ*) { ++گنتی; });
        أرجع گنتی;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(صحيح حد_بندی) ثابت {
        صحيح گنتی = 0;
        أرجع this->exists([&گنتی, حد_بندی](ثابت Ast_گرہ*) { أرجع ++گنتی > حد_بندی; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
سانچا <>
Ast_گرہ* Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(Ast_گرہ* nodep, Ast_گرہ* newp);

// Inline method implementations
Ast_گرہ* Ast_گرہ::جوڑو_اگلا(Ast_گرہ* newp) { أرجع جوڑو_اگلا(this, newp); }

// Specializations of privateTypeTest
#ضمن "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
سانچا <>
مضمن bool Ast_گرہ::mayBeUnder<AstCell>(ثابت Ast_گرہ* nodep) {
    أرجع !VN_IS(nodep, گرہ_Stmt) && !VN_IS(nodep, گرہ_اظہاریہ);
}
سانچا <>
مضمن bool Ast_گرہ::mayBeUnder<Ast_گرہ_سونپو>(ثابت Ast_گرہ* nodep) {
    أرجع !VN_IS(nodep, گرہ_اظہاریہ);
}
سانچا <>
مضمن bool Ast_گرہ::mayBeUnder<AstVarScope>(ثابت Ast_گرہ* nodep) {
    إذا (VN_IS(nodep, VarScope)) أرجع false;  // Should not nest
    إذا (VN_IS(nodep, Var)) أرجع false;
    إذا (VN_IS(nodep, سرگرم)) أرجع false;
    إذا (VN_IS(nodep, گرہ_Stmt)) أرجع false;
    إذا (VN_IS(nodep, گرہ_اظہاریہ)) أرجع false;
    أرجع true;
}
سانچا <>
مضمن bool Ast_گرہ::mayBeUnder<Ast_Exec_گراف>(ثابت Ast_گرہ* nodep) {
    إذا (VN_IS(nodep, Exec_گراف)) أرجع false;  // Should not nest
    إذا (VN_IS(nodep, گرہ_Stmt)) أرجع false;  // Should be directly under CFunc
    أرجع true;
}
سانچا <>
مضمن bool Ast_گرہ::mayBeUnder<Ast_سرگرم>(ثابت Ast_گرہ* nodep) {
    أرجع !VN_IS(nodep, سرگرم);  // AstActives do not nest
}
سانچا <>
مضمن bool Ast_گرہ::mayBeUnder<AstScope>(ثابت Ast_گرہ* nodep) {
    أرجع !VN_IS(nodep, Scope);  // AstScopes do not nest
}
سانچا <>
مضمن bool Ast_گرہ::mayBeUnder<Ast_Sen_درخت>(ثابت Ast_گرہ* nodep) {
    أرجع !VN_IS(nodep, Sen_درخت);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
سانچا <>
constexpr bool Ast_گرہ::is_پتی<Ast_گرہ_Var_Ref>() {
    أرجع true;
}
سانچا <>
constexpr bool Ast_گرہ::is_پتی<AstVarRef>() {
    أرجع true;
}
سانچا <>
constexpr bool Ast_گرہ::is_پتی<AstVarXRef>() {
    أرجع true;
}

// foreach implementation
سانچا <typename T_Arg, typename T_Callable>
فراغ Ast_گرہ::foreachImpl(Const_Correct_Ast_گرہ<T_Arg>* nodep, ثابت T_Callable& f, bool ویکھو_اگلا) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using گرہ_2 = Const_Correct_Ast_گرہ<T_Arg>;

    // We prefetch this far into the stack
    constexpr صحيح Prefetch_فاصلہ = 2;
    // We push max 5 items per iteration
    constexpr ناپ_t Max_Pushes_فی_Iteration = 5;
    // Initial stack sie
    constexpr ناپ_t Initial_ڈھیر_ناپ = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    ناپ_t ڈھیر_ناپ = Initial_ڈھیر_ناپ;
    گرہ_2* وچ_سطر_ڈھیر[Initial_ڈھیر_ناپ];  // In-line initial stack storage
    std::unique_ptr<گرہ_2*[]> باہرلا_دا_سطر_Stackp{nullptr};  // In case we need more, we will allocate
    گرہ_2** basep = وچ_سطر_ڈھیر + Prefetch_فاصلہ;  // Pointer to base of stack
    گرہ_2** topp = basep;  // Pointer to top of stack
    گرہ_2** limp = وچ_سطر_ڈھیر + ڈھیر_ناپ - Max_Pushes_فی_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    لكل (صحيح i = -Prefetch_فاصلہ; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ثابت خودکار ویکھو = [&](گرہ_2* currp) {
        // Type test this node
        إذا (Ast_گرہ::نجی_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            إذا VL_CONSTEXPR_CXX17 (is_پتی<T_Arg_NonConst>()) أرجع;
        }

        // Enqueue children for traversal, unless futile
        إذا (mayBeUnder<T_Arg_NonConst>(currp)) {
            إذا (Ast_گرہ* ثابت op4p = currp->op4p()) *topp++ = op4p;
            إذا (Ast_گرہ* ثابت op3p = currp->op3p()) *topp++ = op3p;
            إذا (Ast_گرہ* ثابت op2p = currp->op2p()) *topp++ = op2p;
            إذا (Ast_گرہ* ثابت op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    إذا (ویکھو_اگلا && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    ویکھو(nodep);

    // Visit the rest of the tree
    طالما (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        گرہ_2* ثابت headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_فاصلہ]);

        // Ensure we have stack space for nextp and the 4 children
        إذا (VL_UNLIKELY(topp >= limp)) {
            ثابت ptrdiff_t occupancy = topp - basep;
            {
                ثابت ناپ_t new_ڈھیر_ناپ = ڈھیر_ناپ * 2;
                گرہ_2** ثابت newStackp = new گرہ_2*[new_ڈھیر_ناپ];
                std::حافظہ_نقل(newStackp, basep - Prefetch_فاصلہ,
                            حجم(گرہ_2**) * (occupancy + Prefetch_فاصلہ));
                ڈھیر_ناپ = new_ڈھیر_ناپ;
                باہرلا_دا_سطر_Stackp.reset(newStackp);
            }
            basep = باہرلا_دا_سطر_Stackp.get() + Prefetch_فاصلہ;
            topp = basep + occupancy;
            limp = باہرلا_دا_سطر_Stackp.get() + ڈھیر_ناپ - Max_Pushes_فی_Iteration;
        }

        // Enqueue the next node
        إذا (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        ویکھو(headp);
    }
}

// predicate implementation
سانچا <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_گرہ::predicateImpl(Const_Correct_Ast_گرہ<T_Arg>* nodep, ثابت T_Callable& p,
                            bool ویکھو_اگلا) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using گرہ_2 = Const_Correct_Ast_گرہ<T_Arg>;

    // We prefetch this far into the stack
    constexpr صحيح Prefetch_فاصلہ = 2;
    // We push max 5 items per iteration
    constexpr ناپ_t Max_Pushes_فی_Iteration = 5;
    // Initial stack sie
    constexpr ناپ_t Initial_ڈھیر_ناپ = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    ناپ_t ڈھیر_ناپ = Initial_ڈھیر_ناپ;
    گرہ_2* وچ_سطر_ڈھیر[Initial_ڈھیر_ناپ];  // In-line initial stack storage
    std::unique_ptr<گرہ_2*[]> باہرلا_دا_سطر_Stackp{nullptr};  // In case we need more, we will allocate
    گرہ_2** basep = وچ_سطر_ڈھیر + Prefetch_فاصلہ;  // Pointer to base of stack
    گرہ_2** topp = basep;  // Pointer to top of stack
    گرہ_2** limp = وچ_سطر_ڈھیر + ڈھیر_ناپ - Max_Pushes_فی_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    لكل (صحيح i = -Prefetch_فاصلہ; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ثابت خودکار ویکھو = [&](گرہ_2* currp) {
        // Type test this node
        إذا (Ast_گرہ::نجی_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            إذا (p(static_cast<T_Arg*>(currp)) != N_Default) أرجع true;
            // Short circuit if iterating leaf nodes
            إذا VL_CONSTEXPR_CXX17 (is_پتی<T_Arg_NonConst>()) أرجع false;
        }

        // Enqueue children for traversal, unless futile
        إذا (mayBeUnder<T_Arg_NonConst>(currp)) {
            إذا (Ast_گرہ* ثابت op4p = currp->op4p()) *topp++ = op4p;
            إذا (Ast_گرہ* ثابت op3p = currp->op3p()) *topp++ = op3p;
            إذا (Ast_گرہ* ثابت op2p = currp->op2p()) *topp++ = op2p;
            إذا (Ast_گرہ* ثابت op1p = currp->op1p()) *topp++ = op1p;
        }

        أرجع false;
    };

    // Enqueue the next of the root node, if required
    إذا (ویکھو_اگلا && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    إذا (ویکھو(nodep)) أرجع !N_Default;

    // Visit the rest of the tree
    طالما (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        گرہ_2* ثابت headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_فاصلہ]);

        // Ensure we have stack space for nextp and the 4 children
        إذا (VL_UNLIKELY(topp >= limp)) {
            ثابت ptrdiff_t occupancy = topp - basep;
            {
                ثابت ناپ_t new_ڈھیر_ناپ = ڈھیر_ناپ * 2;
                گرہ_2** ثابت newStackp = new گرہ_2*[new_ڈھیر_ناپ];
                std::حافظہ_نقل(newStackp, basep - Prefetch_فاصلہ,
                            حجم(گرہ_2**) * (occupancy + Prefetch_فاصلہ));
                ڈھیر_ناپ = new_ڈھیر_ناپ;
                باہرلا_دا_سطر_Stackp.reset(newStackp);
            }
            basep = باہرلا_دا_سطر_Stackp.get() + Prefetch_فاصلہ;
            topp = basep + occupancy;
            limp = باہرلا_دا_سطر_Stackp.get() + ڈھیر_ناپ - Max_Pushes_فی_Iteration;
        }

        // Enqueue the next node
        إذا (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        إذا (ویکھو(headp)) أرجع !N_Default;
    }

    أرجع N_Default;
}

مضمن std::ostream& عامل<<(std::ostream& os, ثابت Ast_گرہ* rhs) {
    إذا (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } وإلا إذا (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } وإلا {
        rhs->dump(os);
    }
    أرجع os;
}
فراغ VNRelinker::relink(Ast_گرہ* newp) { newp->Ast_گرہ::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
سانچا <typename T_گرہ>
class VNRef final : عام std::reference_wrapper<T_گرہ> {
    static_assert(std::is_ادھار_دا<Ast_گرہ, T_گرہ>::قدر,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

عام:
    سانچا <typename U>
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
ناپ_t V3HasherUncachedHash(ثابت Ast_گرہ&);

// Specialization of std::hash for VNRef
سانچا <typename T_گرہ>
بنية std::hash<VNRef<T_گرہ>> final {
    ناپ_t عامل()(VNRef<T_گرہ> r) ثابت { أرجع V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
سانچا <typename T_گرہ>
بنية std::equal_منزل<VNRef<T_گرہ>> final {
    ناپ_t عامل()(VNRef<T_گرہ> ra, VNRef<T_گرہ> rb) ثابت {
        أرجع ra.get().same_درخت(&(rb.get()));
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
    ساكن ثابت VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    فراغ اشاریہ(Ast_گرہ* nodep, Ast_گرہ* scopep);
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
    bool outerp(ثابت Ast_گرہ* nodep, Ast_گرہ*& outerpr) ثابت;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ثابت std::سمتیہ<Ast_گرہ*>* paramsp(ثابت Ast_گرہ* modp) ثابت;
    // The prefix kept for scope 'scopep', or nullptr if none
    ثابت std::string* prefixp(ثابت Ast_گرہ* scopep) ثابت;
    // Keep 'prefix', final, for scope 'scopep'
    فراغ اگیتر(ثابت Ast_گرہ* scopep, ثابت std::string& اگیتر) ثابت;
};

//######################################################################
// Inline VNVisitor METHODS

فراغ VNVisitorConst::iterateConst(Ast_گرہ* nodep) { nodep->accept(*this); }
فراغ VNVisitorConst::iterateConstNull(Ast_گرہ* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->accept(*this);
}
فراغ VNVisitorConst::iterateChildrenConst(Ast_گرہ* nodep) { nodep->iterateChildrenConst(*this); }
فراغ VNVisitorConst::iterateChildrenBackwardsConst(Ast_گرہ* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
فراغ VNVisitorConst::iterate_And_اگلا_Const_Null(Ast_گرہ* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->iterate_And_اگلا_Const(*this);
}

فراغ VNVisitor::iterate(Ast_گرہ* nodep) { nodep->accept(*this); }
فراغ VNVisitor::iterateNull(Ast_گرہ* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->accept(*this);
}
فراغ VNVisitor::iterateChildren(Ast_گرہ* nodep) { nodep->iterateChildren(*this); }
فراغ VNVisitor::iterate_And_اگلا_Null(Ast_گرہ* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->iterate_And_اگلا(*this);
}
Ast_گرہ* VNVisitor::iterateSubtreeReturnEdits(Ast_گرہ* nodep) {
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
فراغ dump_گرہ_List_Json(std::ostream& os, ثابت Ast_گرہ* nodep, ثابت std::string& listName,
                      ثابت string& indent);

#جے_انت  // Guard
