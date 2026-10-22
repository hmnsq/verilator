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

#ifndef VERILATOR_V3AST_H_
#define VERILATOR_V3AST_H_

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
#undef VL_USER_TYPE_CHECKS
#endif
#ifdef VL_USER_TYPE_CHECKS
#ضمن <variant>
#endif

// clang-format off
#ضمن "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3Graph;
class ExecMTask;
class VIfaceCaptureTag;

//######################################################################

// For broken() function, return error string if have a match
#define BROKEN_RTN(test) \
    افعل { \
        إذا (VL_UNCOVERABLE(test)) أرجع "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } طالما (false)
// For broken() function, return error string if a base of this class has a match
#define BROKEN_BASE_RTN(test) \
    افعل { \
        ثابت حرف* ثابت reasonp = (test); \
        إذا (VL_UNCOVERABLE(reasonp)) أرجع reasonp; \
    } طالما (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#define VN_IS(nodep, nodetypename) (Ast_كلي::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#define VN_CAST(nodep, nodetypename) (Ast_كلي::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#define VN_AS(nodep, nodetypename) (Ast_كلي::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#إذا defined(VL_DEBUG) || defined(VL_ASAN)
#define VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#وإلا
#define VN_DBG_AS(nodep, nodetypename) (Ast_كلي::unsafePrivateAs<Ast##nodetypename>(nodep))
#endif

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#define VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

بنية VNTypeInfo final {
    ثابت حرف* m_namep;
    تعداد OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ثابت حرف* m_opNamep[4];
    size_t m_sizeof;
};

class VNType final {
    ساكن ثابت VNTypeInfo s_typeInfoTable[];

عام:
#ضمن "V3Ast__gen_type_enum.h"  // From ./astgen
    ثابت en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr operator en() ثابت VL_MT_SAFE { أرجع m_e; }

    // Retrieve VNTypeInfo for the given type
    ساكن ثابت VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { أرجع s_typeInfoTable[t.m_e]; }
};
constexpr bool operator==(ثابت VNType& lhs, ثابت VNType& rhs) VL_PURE {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VNType& lhs, VNType::en rhs) VL_PURE { أرجع lhs.m_e == rhs; }
constexpr bool operator==(VNType::en lhs, ثابت VNType& rhs) VL_PURE { أرجع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VNType& rhs) { أرجع os << rhs.ascii(); }

//######################################################################
// AstNUser - Generic base class for AST User nodes.
//          - Also used to allow parameter passing up/down iterate calls

class WidthVP;
class V3GraphVertex;
class VSymEnt;

class VNUser final {
// Defining VL_USER_TYPE_CHECKS replaces the VNUser union with a std::variant
// which remembers which form of VNUser is being stored to catch mismatched reads.
#ifdef VL_USER_TYPE_CHECKS
    // monostate is an unwritten / cleared slot. It can be read as either form
    // and yields nullptr/0.
    std::variant<std::monostate, uint64_t, فراغ*> m_u;
#وإلا
    اتحاد {
        فراغ* up;
        uint64_t uq;
    } m_u;
#endif

عام:
#ifdef VL_USER_TYPE_CHECKS
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
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::درج, T>::type to() ثابت VL_MT_SAFE {
        إذا (std::holds_alternative<std::monostate>(m_u)) أرجع nullptr;
        فراغ* ثابت* ثابت upp = std::get_if<فراغ*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        أرجع reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() ثابت {
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
        m_u.up = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(فراغ* p) { m_u.up = p; }
    ~VNUser() = افتراضي;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::درج, T>::type to() ثابت VL_MT_SAFE {
        أرجع reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() ثابت { أرجع m_u.uq; }
#endif
    VSymEnt* toSymEnt() ثابت { أرجع to<VSymEnt*>(); }
    Ast_كلي* toNodep() ثابت VL_MT_SAFE { أرجع to<Ast_كلي*>(); }
    V3GraphVertex* toGraphVertex() ثابت { أرجع to<V3GraphVertex*>(); }
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

class VNUserInUseBase
VL_NOT_FINAL{محمي : ساكن فراغ allocate(صحيح id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvtToStr(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
ساكن فراغ حرر(صحيح id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvtToStr(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
ساكن فراغ clearcnt(صحيح id, uint32_t& cntGblRef, ثابت bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
ساكن فراغ checkcnt(صحيح id, uint32_t&, ثابت bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef,
                   "Check of User" + cvtToStr(id) + "() failed, not under AstUserInUse");
}
}
;

// For each user() declare the in use structure
// We let AstNode peek into here, because when under low optimization even
// an accessor would be way too slow.
// clang-format off
class VNUser1InUse final : VNUserInUseBase {
محمي:
    friend class Ast_كلي;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser1InUse()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1InUse()     { حرر    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2InUse final : VNUserInUseBase {
محمي:
    friend class Ast_كلي;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser2InUse()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2InUse()     { حرر    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3InUse final : VNUserInUseBase {
محمي:
    friend class Ast_كلي;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser3InUse()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3InUse()     { حرر    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ساكن فراغ check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4InUse final : VNUserInUseBase {
محمي:
    friend class Ast_كلي;
    ساكن uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ساكن bool s_userBusy;  // Count is in use
عام:
    VNUser4InUse()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4InUse()     { حرر    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
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
    std::vector<Ast_كلي*> m_deleteps;  // Nodes to delete

عام:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    فراغ تور_Deletep(Ast_كلي* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.تور_back(nodep);
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
    friend class Ast_كلي;

عام:
    /// Call visit()s on nodep
    مضمن فراغ iterateConst(Ast_كلي* nodep);
    /// Call visit()s on nodep
    مضمن فراغ iterateConstNull(Ast_كلي* nodep);
    /// Call visit()s on const nodep's children
    مضمن فراغ iterateChildrenConst(Ast_كلي* nodep);
    /// Call visit()s on nodep's children in backp() order
    مضمن فراغ iterateChildrenBackwardsConst(Ast_كلي* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    مضمن فراغ iterateAndNextConstNull(Ast_كلي* nodep);

    virtual فراغ visit(Ast_كلي* nodep) = 0;
    virtual ~VNVisitorConst() {}
#ضمن "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : عام VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
عام:
    /// Call visit()s on nodep
    مضمن فراغ iterate(Ast_كلي* nodep);
    /// Call visit()s on nodep
    مضمن فراغ iterateNull(Ast_كلي* nodep);
    /// Call visit()s on nodep's children
    مضمن فراغ iterateChildren(Ast_كلي* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    مضمن فراغ iterateAndNextNull(Ast_كلي* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    مضمن Ast_كلي* iterateSubtreeReturnEdits(Ast_كلي* nodep);

    VNDeleter& deleter() { أرجع m_deleter; }
    فراغ تور_Deletep(Ast_كلي* nodep) { deleter().تور_Deletep(nodep); }
    فراغ doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
محمي:
    friend class Ast_كلي;
    تعداد RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_NEXT,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_كلي* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_كلي* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_كلي** m_iterpp = nullptr;
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
    مضمن فراغ relink(Ast_كلي* newp);
    فراغ dump(std::ostream& str = std::cout) ثابت;
};
مضمن std::ostream& operator<<(std::ostream& os, ثابت VNRelinker& rhs) {
    rhs.dump(os);
    أرجع os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{عام : virtual bool كلي_Match(ثابت Ast_كلي* nodep) ثابت {أرجع true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#define ASTNODE_PREFETCH_NON_NULL(nodep) \
    افعل { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } طالما (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#define ASTNODE_PREFETCH(nodep) \
    افعل { \
        إذا (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } طالما (false)

class Ast_كلي VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_كلي* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_كلي* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_كلي* m_op1p = nullptr;  // Generic pointer 1
    Ast_كلي* m_op2p = nullptr;  // Generic pointer 2
    Ast_كلي* m_op3p = nullptr;  // Generic pointer 3
    Ast_كلي* m_op4p = nullptr;  // Generic pointer 4
    Ast_كلي** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    ثابت VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    بنية {
        bool didWidth : 1;  // Did V3Width computation
        bool doingWidth : 1;  // Inside V3Width
        bool protect : 1;  // Protect name if protection is on
        // Space for more flags here (there must be 8 bits in total)
        uint8_t unused : 5;
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
#endif

    Ast_كلي_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_كلي* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_لي* m_fileline;  // Where it was declared
#ifdef VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_كدي;  // When it was last edited
#endif
    ساكن uint64_t s_editCntGbl;  // Global edit counter
    ساكن uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_كلي* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
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
    فراغ op1p(Ast_كلي* nodep) {
        m_op1p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op2p(Ast_كلي* nodep) {
        m_op2p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op3p(Ast_كلي* nodep) {
        m_op3p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }
    فراغ op4p(Ast_كلي* nodep) {
        m_op4p = nodep;
        إذا (nodep) nodep->m_backp = this;
    }

خاص:
    Ast_كلي* clone_بشيا_Iter(bool needPure);
    Ast_كلي* clone_بشيا_Iter_List(bool needPure);
    فراغ check_بشيا_Iter(ثابت Ast_كلي* prevBackp) ثابت VL_MT_STABLE;
    bool gate_بشيا_Iter() ثابت;
    ساكن bool same_بشيا_Iter(ثابت Ast_كلي* node1p, ثابت Ast_كلي* node2p, bool ignNext,
                             bool gateOnly);
    فراغ delete_بشيا_Iter();
    فراغ delete_كلي();
    string instanceStr() ثابت;

عام:
    فراغ purityCheck();
    ساكن فراغ relinkOneLink(Ast_كلي*& pointpr, Ast_كلي* newp);
    // cppcheck-suppress functionConst
    ساكن فراغ debug_بشيا_Change(ثابت Ast_كلي* nodep, ثابت حرف* prefix, صحيح lineno, bool next);

محمي:
    // CONSTRUCTORS
    Ast_كلي(VNType t, File_لي* fl);
    virtual ~Ast_كلي() = افتراضي;  // Use 'deleteTree' instead
    virtual Ast_كلي* clone() = 0;  // Generally, cloneTree is what you want instead
    virtual فراغ cloneRelink() { cloneRelinkGen(); }
    virtual فراغ cloneRelinkGen() {};  // Overrides generated by 'astgen'
    فراغ clone_Relink_بشيا();

    // METHODS
    فراغ setOp1p(Ast_كلي* newp);  // Set non-list-type op1 to non-list element
    فراغ setOp2p(Ast_كلي* newp);  // Set non-list-type op2 to non-list element
    فراغ setOp3p(Ast_كلي* newp);  // Set non-list-type op3 to non-list element
    فراغ setOp4p(Ast_كلي* newp);  // Set non-list-type op4 to non-list element

    فراغ addOp1p(Ast_كلي* newp);  // Append newp to end of op1
    فراغ addOp2p(Ast_كلي* newp);  // Append newp to end of op2
    فراغ addOp3p(Ast_كلي* newp);  // Append newp to end of op3
    فراغ addOp4p(Ast_كلي* newp);  // Append newp to end of op4

    فراغ prependOp1p(Ast_كلي* newp);  // Prepend newp to front of op1
    فراغ prependOp2p(Ast_كلي* newp);  // Prepend newp to front of op2
    فراغ prependOp3p(Ast_كلي* newp);  // Prepend newp to front of op3
    فراغ prependOp4p(Ast_كلي* newp);  // Prepend newp to front of op4

    // clang-format off
    فراغ setNOp1p(Ast_كلي* newp) { إذا (newp) setOp1p(newp); }
    فراغ setNOp2p(Ast_كلي* newp) { إذا (newp) setOp2p(newp); }
    فراغ setNOp3p(Ast_كلي* newp) { إذا (newp) setOp3p(newp); }
    فراغ setNOp4p(Ast_كلي* newp) { إذا (newp) setOp4p(newp); }

    فراغ addNOp1p(Ast_كلي* newp) { إذا (newp) addOp1p(newp); }
    فراغ addNOp2p(Ast_كلي* newp) { إذا (newp) addOp2p(newp); }
    فراغ addNOp3p(Ast_كلي* newp) { إذا (newp) addOp3p(newp); }
    فراغ addNOp4p(Ast_كلي* newp) { إذا (newp) addOp4p(newp); }

    فراغ prependNOp1p(Ast_كلي* newp) { إذا (newp) prependOp1p(newp); }
    فراغ prependNOp2p(Ast_كلي* newp) { إذا (newp) prependOp2p(newp); }
    فراغ prependNOp3p(Ast_كلي* newp) { إذا (newp) prependOp3p(newp); }
    فراغ prependNOp4p(Ast_كلي* newp) { إذا (newp) prependOp4p(newp); }
    // clang-format on

    فراغ clonep(Ast_كلي* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    ساكن فراغ clone_Clear_بشيا() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    virtual bool same_كلي(ثابت Ast_كلي*) ثابت { أرجع true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    virtual bool wouldBreakGen(ثابت Ast_كلي* ثابت oldp,
                               ثابت Ast_كلي* ثابت newp) ثابت
        = 0;  // Generated by 'astgen'

عام:
    // ACCESSORS
    VNType type() ثابت VL_MT_SAFE { أرجع m_type; }
    ثابت حرف* typeName() ثابت VL_MT_SAFE { أرجع type().ascii(); }  // See also prettyTypeName
    Ast_كلي* nextp() ثابت VL_MT_STABLE { أرجع m_nextp; }
    Ast_كلي* backp() ثابت VL_MT_STABLE { أرجع m_backp; }
    Ast_كلي* abovep() ثابت;  // Get parent node above, only for list head and tail
    Ast_كلي* aboveLoopp() ثابت;  // Get parent node above, may have performance issues as loops
    Ast_كلي* lastp() ثابت {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        أرجع m_headtailp;
    }
    Ast_كلي* prevp() ثابت {  // Previous node in list, nullptr for head of list
        إذا (m_backp->m_nextp != this) أرجع nullptr;
        أرجع m_backp;
    }
    Ast_كلي* op1p() ثابت VL_MT_STABLE { أرجع m_op1p; }
    Ast_كلي* op2p() ثابت VL_MT_STABLE { أرجع m_op2p; }
    Ast_كلي* op3p() ثابت VL_MT_STABLE { أرجع m_op3p; }
    Ast_كلي* op4p() ثابت VL_MT_STABLE { أرجع m_op4p; }
    Ast_كلي_DType* dtypep() ثابت VL_MT_STABLE { أرجع m_dtypep; }
    Ast_كلي* clonep() ثابت { أرجع ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_كلي* firstAbovep() ثابت {  // Returns nullptr when second or later in list
        أرجع ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ثابت VL_MT_SAFE { أرجع m_brokenState; }
    فراغ brokenState(uint8_t درج) { m_brokenState = درج; }

    // Used by AstNode::broken()
    bool brokeExists() ثابت { أرجع V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ثابت { أرجع brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    virtual فراغ foreachLink(std::function<فراغ(Ast_كلي** linkpp, ثابت حرف* namep)> f) = 0;

#إذا defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    ساكن فراغ* operator new(size_t size);
    ساكن فراغ operator delete(فراغ* obj, size_t size);
#endif

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    ساكن constexpr صحيح INSTR_كدي_BRANCH = 4;  // Branch
    ساكن constexpr صحيح INSTR_كدي_CALL = INSTR_كدي_BRANCH + 10;  // Subroutine call
    ساكن constexpr صحيح INSTR_كدي_LD = 2;  // Load memory
    ساكن constexpr صحيح INSTR_كدي_INT_MUL = 3;  // Integer multiply
    ساكن constexpr صحيح INSTR_كدي_INT_DIV = 10;  // Integer divide
    ساكن constexpr صحيح INSTR_كدي_DBL = 8;  // Convert or do float ops
    ساكن constexpr صحيح INSTR_كدي_DBL_DIV = 40;  // Double divide
    ساكن constexpr صحيح INSTR_كدي_DBL_TRIG = 200;  // Double trigonometric ops
    ساكن constexpr صحيح INSTR_كدي_STR = 100;  // String ops
    ساكن constexpr صحيح INSTR_كدي_TIME = INSTR_كدي_CALL + 5;  // Determine simulation time
    ساكن constexpr صحيح INSTR_كدي_PLI = 20;  // PLI routines
    ساكن constexpr صحيح INSTR_كدي_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    virtual string name() ثابت VL_MT_STABLE { أرجع ""; }
    virtual string origName() ثابت { أرجع ""; }
    string prettyDehashOrigOrName() ثابت {
        أرجع prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    virtual فراغ name(ثابت string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    virtual فراغ tag(ثابت string& text) {}
    virtual string tag() ثابت { أرجع ""; }
    virtual uint32_t decl_Token_لمب() ثابت { أرجع 0; }
    virtual فراغ decl_Token_لمب_Set_Min(uint32_t token_لمب) {}
    virtual string verilogKwd() ثابت { أرجع ""; }
    string nameProtect() ثابت VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() ثابت;  // origName with --protect-id applied
    string shortName() ثابت;  // Name with __PVT__ removed for concatenating scopes
    ساكن string dedotName(ثابت string& namein);  // Name with dots removed
    ساكن string nameNoArray(ثابت string& namein);  // Name with any array index removed
    ساكن string prettyName(ثابت string& namein) VL_PURE;  // Name for printing out to the user
    ساكن string vpiName(ثابت string& namein);  // Name for vpi access
    ساكن string prettyNameQ(ثابت string& namein) {  // Quoted pretty name (for errors)
        أرجع "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    ساكن string encodeName(ثابت string& namein);
    ساكن string encode_لمب(int64_t لمب);  // Encode number into internal C representation
    ساكن string vcdName(ثابت string& namein);  // Name for printing out to vcd files
    string prettyName() ثابت { أرجع prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    virtual string pretty_Name_سكو() ثابت { أرجع prettyName(); }
    string prettyNameQ() ثابت { أرجع "'"s + pretty_Name_سكو() + "'"; }  // Quoted, for messages
    string verilogName() ثابت { أرجع vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ثابت;
    virtual string prettyOperatorName() ثابت { أرجع "operator " + prettyTypeName(); }
    File_لي* fileline() ثابت VL_MT_SAFE { أرجع m_fileline; }
    فراغ fileline(File_لي* fl) { m_fileline = fl; }
    مضمن bool width1() ثابت;
    مضمن صحيح widthInstrs() ثابت;
    bool didWidth() ثابت { أرجع m_flags.didWidth; }
    فراغ didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        إذا (didWidth()) أرجع true;
        didWidth(true);
        أرجع false;
    }
    bool doingWidth() ثابت { أرجع m_flags.doingWidth; }
    فراغ doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() ثابت VL_MT_SAFE { أرجع m_flags.protect; }
    فراغ protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    مضمن صحيح width() ثابت VL_MT_STABLE;
    مضمن صحيح widthMin() ثابت VL_MT_STABLE;
    صحيح widthMinV() ثابت {
        أرجع v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    صحيح width_كلموم() ثابت { أرجع VL_كلموم_I(width()); }
    bool isQuad() ثابت VL_MT_STABLE { أرجع (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() ثابت VL_MT_STABLE { أرجع (width() > VL_QUADSIZE); }
    مضمن bool isCHandle() ثابت VL_MT_STABLE;
    مضمن bool isDouble() ثابت VL_MT_STABLE;
    مضمن bool isSigned() ثابت VL_MT_STABLE;
    مضمن bool isString() ثابت VL_MT_STABLE;
    مضمن bool isEvent() ثابت VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        أرجع ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_كلي* user1p() ثابت VL_MT_STABLE { أرجع user1u().toNodep(); }
    فراغ user1u(ثابت VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    فراغ user1p(فراغ* userp) { user1u(VNUser{userp}); }
    فراغ user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ثابت { أرجع user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ثابت uint64_t v = user1(); user1(v + val); أرجع v; }
    uint64_t user1Or(uint64_t val) { ثابت uint64_t v = user1(); user1(v | val); أرجع v; }
    uint64_t user1SetOnce() { ثابت uint64_t v = user1(); إذا (!v) user1(1); أرجع v; }  // Better for cache than user1Inc()
    ساكن فراغ user1_Clear_بشيا() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        أرجع ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_كلي* user2p() ثابت VL_MT_STABLE { أرجع user2u().toNodep(); }
    فراغ user2u(ثابت VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    فراغ user2p(فراغ* userp) { user2u(VNUser{userp}); }
    فراغ user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ثابت { أرجع user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ثابت uint64_t v = user2(); user2(v + val); أرجع v; }
    uint64_t user2Or(uint64_t val) { ثابت uint64_t v = user2(); user2(v | val); أرجع v; }
    uint64_t user2SetOnce() { ثابت uint64_t v = user2(); إذا (!v) user2(1); أرجع v; }  // Better for cache than user2Inc()
    ساكن فراغ user2_Clear_بشيا() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        أرجع ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_كلي* user3p() ثابت VL_MT_STABLE { أرجع user3u().toNodep(); }
    فراغ user3u(ثابت VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    فراغ user3p(فراغ* userp) { user3u(VNUser{userp}); }
    فراغ user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ثابت { أرجع user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ثابت uint64_t v = user3(); user3(v + val); أرجع v; }
    uint64_t user3Or(uint64_t val) { ثابت uint64_t v = user3(); user3(v | val); أرجع v; }
    uint64_t user3SetOnce() { ثابت uint64_t v = user3(); إذا (!v) user3(1); أرجع v; }  // Better for cache than user3Inc()
    ساكن فراغ user3_Clear_بشيا() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ثابت VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        أرجع ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_كلي* user4p() ثابت VL_MT_STABLE { أرجع user4u().toNodep(); }
    فراغ user4u(ثابت VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    فراغ user4p(فراغ* userp) { user4u(VNUser{userp}); }
    فراغ user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ثابت { أرجع user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { ثابت uint64_t v = user4(); user4(v | val); أرجع v; }
    uint64_t user4Inc(uint64_t val = 1) { ثابت uint64_t v = user4(); user4(v + val); أرجع v; }
    uint64_t user4SetOnce() { ثابت uint64_t v = user4(); إذا (!v) user4(1); أرجع v; }  // Better for cache than user4Inc()
    ساكن فراغ user4_Clear_بشيا() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ifdef VL_DEBUG
    uint64_t edit_كدي() ثابت { أرجع m_edit_كدي; }
    فراغ edit_كدي_Inc() {
        m_edit_كدي = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_بشيا();  // Any edit clears all caching
    }
#وإلا
    فراغ edit_كدي_Inc() { ++s_editCntGbl; }
#endif
    ساكن uint64_t edit_كدي_Last() VL_MT_SAFE { أرجع s_editCntLast; }
    ساكن uint64_t edit_كدي_Gbl() VL_MT_SAFE { أرجع s_editCntGbl; }
    ساكن فراغ edit_كدي_Set_Last() { s_editCntLast = edit_كدي_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_تور_Self_Stmt();
    مضمن bool is_Class_Handle_درج() ثابت;
    مضمن bool isNull() ثابت;
    مضمن bool isZero() ثابت;
    مضمن bool isOne() ثابت;
    مضمن bool isNeqZero() ثابت;
    مضمن bool isAllOnes() ثابت;
    مضمن bool isAllOnesV() ثابت;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    فراغ dtypep(Ast_كلي_DType* nodep) {
        إذا (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_كدي_Inc();
        }
    }
    فراغ dtypeFrom(ثابت Ast_كلي* fromp) {
        إذا (fromp) dtypep(fromp->dtypep());
    }
    فراغ dtypeChgSigned(bool flag = true);
    فراغ dtypeChgWidth(صحيح width, صحيح widthMin);
    فراغ dtypeChgWidthSigned(صحيح width, صحيح widthMin, VSigning numeric);
    فراغ dtypeSetBitUnsized(صحيح width, صحيح widthMin, VSigning numeric) {
        dtypep(findBitDType(width, widthMin, numeric));
    }
    فراغ dtypeSetBitSized(صحيح width, VSigning numeric) {
        dtypep(findBitDType(width, width, numeric));  // Since sized, widthMin is width
    }
    فراغ dtypeSetLogicUnsized(صحيح width, صحيح widthMin, VSigning numeric) {
        dtypep(findLogicDType(width, widthMin, numeric));
    }
    فراغ dtypeSetLogicSized(صحيح width, VSigning numeric) {
        dtypep(findLogicDType(width, width, numeric));  // Since sized, widthMin is width
    }
    فراغ dtypeSetLogic2StateUnsized(صحيح width, صحيح widthMin, VSigning numeric) {
        dtypep(findLogic2StateDType(width, widthMin, numeric));
    }
    فراغ dtypeSetLogic2StateSized(صحيح width, VSigning numeric) {
        dtypep(findLogic2StateDType(width, width, numeric));  // Since sized, widthMin is width
    }
    فراغ dtypeSetBitOrLogicUnsized(صحيح width, صحيح widthMin, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, widthMin, numeric, isFourstate));
    }
    فراغ dtypeSetBitOrLogicSized(صحيح width, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, width, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    فراغ dtypeSetBit() { dtypep(findBitDType()); }
    فراغ dtypeSetLogic() { dtypep(findLogicDType()); }
    فراغ dtypeSetLogic2State() { dtypep(findLogic2StateDType()); }
    فراغ dtypeSetDouble() { dtypep(findDoubleDType()); }
    فراغ dtypeSetInt() { dtypep(findIntDType()); }
    فراغ dtypeSetInteger() { dtypep(findIntegerDType()); }
    فراغ dtypeSetInteger2State() { dtypep(findInteger2StateDType()); }
    فراغ dtypeSetString() { dtypep(findStringDType()); }
    فراغ dtypeSetUInt32() { dtypep(findUInt32DType()); }  // Twostate
    فراغ dtypeSetUInt64() { dtypep(findUInt64DType()); }  // Twostate
    فراغ dtypeSetTime() { dtypep(findTimeDType()); }
    فراغ dtypeSetTime2State() { dtypep(findTime2StateDType()); }
    فراغ dtypeSetEmptyQueue() { dtypep(findEmptyQueueDType()); }
    فراغ dtypeSetStream() { dtypep(findStreamDType()); }
    فراغ dtypeSetVoid() { dtypep(findVoidDType()); }
    فراغ dtypeSetBitOrLogic(bool isFourstate) {
        dtypep(isFourstate ? findLogicDType() : findBitDType());
    }

    // Data type locators
    Ast_كلي_DType* findBitDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_كلي_DType* findLogicDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_كلي_DType* findNullDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_كلي_DType* findLogic2StateDType() ثابت {
        أرجع findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_كلي_DType* findDoubleDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_كلي_DType* findIntDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::INT); }
    Ast_كلي_DType* findIntegerDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_كلي_DType* findInteger2StateDType() ثابت {
        أرجع findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_كلي_DType* findStringDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_كلي_DType* findSigned8DType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_كلي_DType* findUInt32DType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_كلي_DType* findUInt64DType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_كلي_DType* findTimeDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_كلي_DType* findTime2StateDType() ثابت {
        أرجع findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_كلي_DType* findCHandleDType() ثابت { أرجع findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_كلي_DType* findConstraintRefDType() ثابت;
    Ast_كلي_DType* findEmptyQueueDType() ثابت;
    Ast_كلي_DType* findQueueIndexDType() ثابت;
    Ast_كلي_DType* findStreamDType() ثابت;
    Ast_كلي_DType* findVoidDType() ثابت;
    Ast_كلي_DType* findBitDType(صحيح width, صحيح widthMin, VSigning numeric) ثابت;
    Ast_كلي_DType* findLogicDType(صحيح width, صحيح widthMin, VSigning numeric) ثابت;
    Ast_كلي_DType* findLogic2StateDType(صحيح width, صحيح widthMin, VSigning numeric) ثابت;
    Ast_كلي_DType* findBitOrLogicDType(صحيح width, صحيح widthMin, VSigning numeric,
                                      bool isFourstate) ثابت {
        أرجع isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_كلي_DType* findBitOrLogicRangeDType(ثابت VNumRange& range, ثابت صحيح widthMin,
                                           ثابت VSigning numeric,
                                           ثابت bool isFourstate) ثابت VL_MT_STABLE {
        أرجع isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_كلي_DType* findLogicRangeDType(ثابت VNumRange& range, صحيح widthMin,
                                      VSigning numeric) ثابت VL_MT_STABLE;
    Ast_كلي_DType* findBitRangeDType(ثابت VNumRange& range, صحيح widthMin,
                                    VSigning numeric) ثابت VL_MT_STABLE;
    Ast_كلي_DType* findBasicDType(VBasicDTypeKwd kwd) ثابت;
    ساكن AstBasicDType* find_سك_Same_DType(AstBasicDType* nodep);

    ساكن VCastable computeCastable(ثابت Ast_كلي_DType* toDtp, ثابت Ast_كلي_DType* fromDtp,
                                     ثابت Ast_كلي* fromConstp, bool checkIfaceArgCompat = false);
    ساكن Ast_كلي_DType* getCommonClassTypep(Ast_كلي* nodep1, Ast_كلي* nodep2);

    // METHODS - dump and error
    فراغ v3errorEnd(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3errorEndFatal(ثابت std::ostringstream& str) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
        أرجع fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ثابت { أرجع fileline()->warnContextSecondary(); }
    string warnMore() ثابت VL_REQUIRES(V3Error::s().m_mutex) { أرجع fileline()->warnMore(); }
    string warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) { أرجع fileline()->warnOther(); }

    virtual فراغ dump(std::ostream& str = std::cout) ثابت;
    ساكن حرف* dump_بشيا_Json_Gdb(ثابت Ast_كلي* nodep);  // For GDB only, free()d by caller
    ساكن حرف* dump_بشيا_Json_Gdb(ثابت حرف* str);  // For GDB only, free()d by caller
    ساكن حرف* dump_بشيا_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    ساكن فراغ dumpGdb(ثابت Ast_كلي* nodep);  // For GDB only
    فراغ dumpGdbHeader() ثابت;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    template <typename T_كلي_Result, typename T_كلي_Next>
    ساكن T_كلي_Result* addNext(T_كلي_Result* nodep, T_كلي_Next* newp) {
        static_assert(std::is_base_of<Ast_كلي, T_كلي_Result>::درج,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_كلي_Result, T_كلي_Next>::درج,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        إذا (!newp) أرجع nodep;
        أرجع static_cast<T_كلي_Result*>(addNext<Ast_كلي, Ast_كلي>(nodep, newp));
    }
    template <typename T_كلي_Result, typename T_كلي_Next>
    ساكن T_كلي_Result* addNextNull(T_كلي_Result* nodep, T_كلي_Next* newp) {
        static_assert(std::is_base_of<Ast_كلي, T_كلي_Result>::درج,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_كلي_Result, T_كلي_Next>::درج,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        إذا (!newp) أرجع nodep;
        إذا (!nodep) أرجع newp;
        أرجع static_cast<T_كلي_Result*>(addNext<Ast_كلي, Ast_كلي>(nodep, newp));
    }
    مضمن Ast_كلي* addNext(Ast_كلي* newp);
    فراغ addNextHere(Ast_كلي* newp);  // Insert newp at this->nextp
    فراغ addHereThisAsNext(Ast_كلي* newp);  // Adds at old place of this, this becomes next
    فراغ replaceWith(Ast_كلي* newp);  // Replace current node in tree with new node
    فراغ replaceWithKeepDType(Ast_كلي* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_كلي* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_كلي* unlinkFrBackWithNext(VNRelinker* linkerp = nullptr);
    فراغ relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    فراغ clone_Relink_كلي() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_كلي* clone_بشيا(bool cloneNextLink,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_كلي* clone_بشيا_Pure(bool cloneNextLink) { أرجع clone_بشيا(cloneNextLink, true); }
    bool gate_بشيا() { أرجع gate_بشيا_Iter(); }  // Is tree isGateOptimizable?
    مضمن bool same_بشيا(ثابت Ast_كلي* node2p) ثابت;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    مضمن bool same_Gate_بشيا(ثابت Ast_كلي* node2p) ثابت;
    فراغ delete_بشيا();  // Always deletes the next link
    فراغ check_بشيا() ثابت VL_MT_STABLE {
        إذا (v3Global.opt.debugCheck()) check_بشيا_Iter(backp());
    }
    فراغ checkIter() ثابت;
    فراغ dumpPtrs(std::ostream& os = std::cout) ثابت;
    فراغ dump_بشيا(std::ostream& os = std::cout, ثابت string& indent = "    ",
                  صحيح maxDepth = 0) ثابت;
    فراغ dump_بشيا(ثابت string& indent, صحيح maxDepth = 0) ثابت {
        dump_بشيا(std::cout, indent, maxDepth);
    }
    ساكن فراغ dump_بشيا_Gdb(ثابت Ast_كلي* nodep);  // For GDB only
    فراغ dump_بشيا_And_Next(std::ostream& os = std::cout, ثابت string& indent = "    ",
                         صحيح maxDepth = 0) ثابت;
    فراغ dump_بشيا_File(ثابت string& filename, bool doDump = true);
    ساكن فراغ dump_بشيا_File_Gdb(ثابت Ast_كلي* nodep, ثابت حرف* filenamep = nullptr);
    فراغ dump_بشيا_Dot(std::ostream& os = std::cout) ثابت;
    فراغ dump_بشيا_Dot_File(ثابت string& filename, bool doDump = true);
    virtual فراغ dumpJson(std::ostream& os) ثابت { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    فراغ dumpJsonGen(std::ostream& os) ثابت {};
    virtual فراغ dump_بشيا_Json_Op_Gen(std::ostream& os, ثابت string& indent) ثابت {};
    فراغ dump_بشيا_Json(std::ostream& os, ثابت string& indent = "") ثابت;
    فراغ dump_بشيا_Json_File(ثابت string& filename, bool doDump = true);
    ساكن فراغ dumpJsonMetaFileGdb(ثابت حرف* filename);
    ساكن فراغ dumpJsonMetaFile(ثابت string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    ساكن std::string كلي_Addr(ثابت Ast_كلي* nodep) {
        أرجع v3Global.opt.dump_بشيا_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    ساكن Ast_كلي* afterCommentp(Ast_كلي* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        طالما (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    virtual bool isBrancher() ثابت { أرجع false; }
    // Else a AstTime etc that can't be pushed out
    virtual bool isGateOptimizable() ثابت { أرجع !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    virtual bool isGateDedupable() ثابت { أرجع isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    virtual bool is_فرچ_Coverage_Eligible() ثابت { أرجع isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    virtual bool isOutputter() { أرجع false; }
    // Else a AstTime etc which output can't be predicted from input
    virtual bool isPredictOptimizable() ثابت { أرجع !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    virtual bool isPure() { أرجع true; }
    // Iff isPure on current node and any nextp()
    bool isPureAndNext() { أرجع isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    virtual bool isTimingControl() ثابت { أرجع false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    virtual bool isUnlikely() ثابت { أرجع false; }
    // Is an IEEE system function (versus internally-generated)
    virtual bool isSystemFunc() ثابت { أرجع false; }
    virtual صحيح instr_كدي() ثابت { أرجع 0; }
    // Iff node is identical to another node
    virtual bool isSame(ثابت Ast_كلي* samep) ثابت {
        أرجع type() == samep->type() && same_كلي(samep);
    }
    // Iff has a data type; dtype() must be non-null
    virtual bool hasDType() ثابت VL_MT_SAFE { أرجع false; }
    // Iff has a non-null childDTypep(), as generic node function
    virtual Ast_كلي_DType* getChildDTypep() ثابت { أرجع nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    virtual Ast_كلي_DType* getChild2DTypep() ثابت { أرجع nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    virtual bool maybePointedTo() ثابت VL_MT_SAFE { أرجع false; }
    // Don't reclaim this node in V3Dead
    virtual bool undead() ثابت { أرجع false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    virtual ثابت حرف* broken() ثابت { أرجع nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    virtual ثابت حرف* brokenGen() ثابت = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ثابت Ast_كلي* ثابت newp) ثابت { أرجع backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    virtual فراغ accept(VNVisitorConst& v) = 0;

محمي:
    // All VNVisitor related functions are called as methods off the visitor
    friend class VNVisitor;
    friend class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    فراغ iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    فراغ iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    فراغ iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    فراغ iterateAndNext(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    فراغ iterateAndNextConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_كلي* iterateSubtreeReturnEdits(VNVisitor& v);

    ساكن فراغ dump_Json_لمب(std::ostream& os, ثابت std::string& name, int64_t val);
    ساكن فراغ dumpJsonBoolIf(std::ostream& os, ثابت std::string& name, bool val);
    ساكن فراغ dumpJsonStr(std::ostream& os, ثابت std::string& name, ثابت std::string& val);
    ساكن فراغ dumpJsonPtr(std::ostream& os, ثابت std::string& name, ثابت Ast_كلي* ثابت valp);

محمي:
    فراغ iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    template <typename T>
    مضمن ساكن bool privateTypeTest(ثابت Ast_كلي* nodep);

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr ساكن bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        أرجع std::is_base_of<T_TargetType, NonCV>::درج;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr ساكن bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        أرجع !std::is_base_of<NonCV, T_TargetType>::درج;
    }  // LCOV_EXCL_STOP

عام:
    // For use via the VN_IS macro only, or in templated code
    template <typename T_TargetType, typename T_كلي>
    ساكن bool is(ثابت T_كلي* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        أرجع nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    template <typename T_TargetType, typename T_كلي>
    ساكن T_TargetType* cast(T_كلي* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        أرجع nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    template <typename T_TargetType, typename T_كلي>
    ساكن ثابت T_TargetType* cast(ثابت T_كلي* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        أرجع nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<ثابت T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    template <typename T_TargetType, typename T_كلي>
    ساكن T_TargetType* as(T_كلي* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        أرجع unsafePrivateAs<T_TargetType, T_كلي>(nodep);
    }
    template <typename T_TargetType, typename T_كلي>
    ساكن ثابت T_TargetType* as(ثابت T_كلي* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        أرجع unsafePrivateAs<T_TargetType, T_كلي>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    template <typename T_TargetType, typename T_كلي>
    ساكن T_TargetType* unsafePrivateAs(T_كلي* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        أرجع reinterpret_cast<T_TargetType*>(nodep);
    }
    template <typename T_TargetType, typename T_كلي>
    ساكن ثابت T_TargetType* unsafePrivateAs(ثابت T_كلي* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_كلي>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        أرجع reinterpret_cast<ثابت T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_كلي>
    ساكن bool mayBeUnder(ثابت Ast_كلي* nodep) {
        static_assert(!std::is_const<T_كلي>::درج,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        أرجع true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_كلي>
    ساكن constexpr bool isLeaf() {
        static_assert(!std::is_const<T_كلي>::درج,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        أرجع false;
    }

خاص:
    // Using std::conditional for const correctness in the public 'foreach' functions
    template <typename T_Arg>
    using Const_Correct_Ast_كلي =
        typename std::conditional<std::is_const<T_Arg>::درج, ثابت Ast_كلي, Ast_كلي>::type;

    template <typename T_Arg, typename T_Callable>
    مضمن ساكن فراغ foreachImpl(Const_Correct_Ast_كلي<T_Arg>* nodep, ثابت T_Callable& f,
                                   bool visitNext);

    template <typename T_Arg, bool N_Default, typename T_Callable>
    مضمن ساكن bool predicateImpl(Const_Correct_Ast_كلي<T_Arg>* nodep, ثابت T_Callable& p,
                                     bool visitNext);

عام:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    template <typename T_Callable>
    فراغ foreach(T_Callable&& f) {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_كلي*>::درج
                          && std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_كلي>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    فراغ foreach(T_Callable&& f) ثابت {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ثابت T_كلي*>::درج
                && std::is_base_of<Ast_كلي, T_كلي>::درج,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ثابت T_كلي>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    فراغ foreachAndNext(T_Callable&& f) {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_كلي*>::درج
                          && std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_كلي>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    فراغ foreachAndNext(T_Callable&& f) ثابت {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ثابت T_كلي*>::درج
                && std::is_base_of<Ast_كلي, T_كلي>::درج,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ثابت T_كلي>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    template <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_كلي*>::درج
                          && std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_كلي, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool exists(T_Callable&& p) ثابت {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_كلي*>::درج
                          && std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_كلي, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_كلي*>::درج
                          && std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_كلي, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) ثابت {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_كلي*>::درج
                          && std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_كلي, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    template <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_كلي*>::درج
                          && std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<T_كلي, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool forall(T_Callable&& p) ثابت {
        using T_كلي = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ثابت T_كلي*>::درج
                          && std::is_base_of<Ast_كلي, T_كلي>::درج,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        أرجع predicateImpl<ثابت T_كلي, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    صحيح كلي_كدي() ثابت {
        // TODO: this should really return size_t, but need to fix use sites
        صحيح كدي = 0;
        this->foreach([&كدي](ثابت Ast_كلي*) { ++كدي; });
        أرجع كدي;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(صحيح limit) ثابت {
        صحيح كدي = 0;
        أرجع this->exists([&كدي, limit](ثابت Ast_كلي*) { أرجع ++كدي > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
template <>
Ast_كلي* Ast_كلي::addNext<Ast_كلي, Ast_كلي>(Ast_كلي* nodep, Ast_كلي* newp);

// Inline method implementations
Ast_كلي* Ast_كلي::addNext(Ast_كلي* newp) { أرجع addNext(this, newp); }

// Specializations of privateTypeTest
#ضمن "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
template <>
مضمن bool Ast_كلي::mayBeUnder<AstCell>(ثابت Ast_كلي* nodep) {
    أرجع !VN_IS(nodep, كلي_Stmt) && !VN_IS(nodep, كلي_فرچ);
}
template <>
مضمن bool Ast_كلي::mayBeUnder<Ast_كلي_Assign>(ثابت Ast_كلي* nodep) {
    أرجع !VN_IS(nodep, كلي_فرچ);
}
template <>
مضمن bool Ast_كلي::mayBeUnder<AstVarScope>(ثابت Ast_كلي* nodep) {
    إذا (VN_IS(nodep, VarScope)) أرجع false;  // Should not nest
    إذا (VN_IS(nodep, Var)) أرجع false;
    إذا (VN_IS(nodep, Active)) أرجع false;
    إذا (VN_IS(nodep, كلي_Stmt)) أرجع false;
    إذا (VN_IS(nodep, كلي_فرچ)) أرجع false;
    أرجع true;
}
template <>
مضمن bool Ast_كلي::mayBeUnder<AstExecGraph>(ثابت Ast_كلي* nodep) {
    إذا (VN_IS(nodep, ExecGraph)) أرجع false;  // Should not nest
    إذا (VN_IS(nodep, كلي_Stmt)) أرجع false;  // Should be directly under CFunc
    أرجع true;
}
template <>
مضمن bool Ast_كلي::mayBeUnder<AstActive>(ثابت Ast_كلي* nodep) {
    أرجع !VN_IS(nodep, Active);  // AstActives do not nest
}
template <>
مضمن bool Ast_كلي::mayBeUnder<AstScope>(ثابت Ast_كلي* nodep) {
    أرجع !VN_IS(nodep, Scope);  // AstScopes do not nest
}
template <>
مضمن bool Ast_كلي::mayBeUnder<Ast_Sen_بشيا>(ثابت Ast_كلي* nodep) {
    أرجع !VN_IS(nodep, Sen_بشيا);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
template <>
constexpr bool Ast_كلي::isLeaf<Ast_كلي_Var_Ref>() {
    أرجع true;
}
template <>
constexpr bool Ast_كلي::isLeaf<AstVarRef>() {
    أرجع true;
}
template <>
constexpr bool Ast_كلي::isLeaf<AstVarXRef>() {
    أرجع true;
}

// foreach implementation
template <typename T_Arg, typename T_Callable>
فراغ Ast_كلي::foreachImpl(Const_Correct_Ast_كلي<T_Arg>* nodep, ثابت T_Callable& f, bool visitNext) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using كلي_2 = Const_Correct_Ast_كلي<T_Arg>;

    // We prefetch this far into the stack
    constexpr صحيح PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_تر_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t تر_Size = Initial_تر_Size;
    كلي_2* in_لي_تر[Initial_تر_Size];  // In-line initial stack storage
    std::unique_ptr<كلي_2*[]> out_Of_لي_Stackp{nullptr};  // In case we need more, we will allocate
    كلي_2** basep = in_لي_تر + PrefetchDistance;  // Pointer to base of stack
    كلي_2** topp = basep;  // Pointer to top of stack
    كلي_2** limp = in_لي_تر + تر_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    لكل (صحيح i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ثابت auto visit = [&](كلي_2* currp) {
        // Type test this node
        إذا (Ast_كلي::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            إذا VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) أرجع;
        }

        // Enqueue children for traversal, unless futile
        إذا (mayBeUnder<T_Arg_NonConst>(currp)) {
            إذا (Ast_كلي* ثابت op4p = currp->op4p()) *topp++ = op4p;
            إذا (Ast_كلي* ثابت op3p = currp->op3p()) *topp++ = op3p;
            إذا (Ast_كلي* ثابت op2p = currp->op2p()) *topp++ = op2p;
            إذا (Ast_كلي* ثابت op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    إذا (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    طالما (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        كلي_2* ثابت headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        إذا (VL_UNLIKELY(topp >= limp)) {
            ثابت ptrdiff_t occupancy = topp - basep;
            {
                ثابت size_t new_تر_Size = تر_Size * 2;
                كلي_2** ثابت newStackp = new كلي_2*[new_تر_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            حجم(كلي_2**) * (occupancy + PrefetchDistance));
                تر_Size = new_تر_Size;
                out_Of_لي_Stackp.reset(newStackp);
            }
            basep = out_Of_لي_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_لي_Stackp.get() + تر_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        إذا (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
template <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_كلي::predicateImpl(Const_Correct_Ast_كلي<T_Arg>* nodep, ثابت T_Callable& p,
                            bool visitNext) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using كلي_2 = Const_Correct_Ast_كلي<T_Arg>;

    // We prefetch this far into the stack
    constexpr صحيح PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_تر_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t تر_Size = Initial_تر_Size;
    كلي_2* in_لي_تر[Initial_تر_Size];  // In-line initial stack storage
    std::unique_ptr<كلي_2*[]> out_Of_لي_Stackp{nullptr};  // In case we need more, we will allocate
    كلي_2** basep = in_لي_تر + PrefetchDistance;  // Pointer to base of stack
    كلي_2** topp = basep;  // Pointer to top of stack
    كلي_2** limp = in_لي_تر + تر_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    لكل (صحيح i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ثابت auto visit = [&](كلي_2* currp) {
        // Type test this node
        إذا (Ast_كلي::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            إذا (p(static_cast<T_Arg*>(currp)) != N_Default) أرجع true;
            // Short circuit if iterating leaf nodes
            إذا VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) أرجع false;
        }

        // Enqueue children for traversal, unless futile
        إذا (mayBeUnder<T_Arg_NonConst>(currp)) {
            إذا (Ast_كلي* ثابت op4p = currp->op4p()) *topp++ = op4p;
            إذا (Ast_كلي* ثابت op3p = currp->op3p()) *topp++ = op3p;
            إذا (Ast_كلي* ثابت op2p = currp->op2p()) *topp++ = op2p;
            إذا (Ast_كلي* ثابت op1p = currp->op1p()) *topp++ = op1p;
        }

        أرجع false;
    };

    // Enqueue the next of the root node, if required
    إذا (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    إذا (visit(nodep)) أرجع !N_Default;

    // Visit the rest of the tree
    طالما (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        كلي_2* ثابت headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        إذا (VL_UNLIKELY(topp >= limp)) {
            ثابت ptrdiff_t occupancy = topp - basep;
            {
                ثابت size_t new_تر_Size = تر_Size * 2;
                كلي_2** ثابت newStackp = new كلي_2*[new_تر_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            حجم(كلي_2**) * (occupancy + PrefetchDistance));
                تر_Size = new_تر_Size;
                out_Of_لي_Stackp.reset(newStackp);
            }
            basep = out_Of_لي_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_لي_Stackp.get() + تر_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        إذا (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        إذا (visit(headp)) أرجع !N_Default;
    }

    أرجع N_Default;
}

مضمن std::ostream& operator<<(std::ostream& os, ثابت Ast_كلي* rhs) {
    إذا (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } وإلا إذا (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } وإلا {
        rhs->dump(os);
    }
    أرجع os;
}
فراغ VNRelinker::relink(Ast_كلي* newp) { newp->Ast_كلي::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
template <typename T_كلي>
class VNRef final : عام std::reference_wrapper<T_كلي> {
    static_assert(std::is_base_of<Ast_كلي, T_كلي>::درج,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

عام:
    template <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_كلي>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(ثابت std::reference_wrapper<T_كلي>& other)
        : std::reference_wrapper<T_كلي>{other} {}
};

static_assert(حجم(VNRef<Ast_كلي>) == حجم(std::reference_wrapper<Ast_كلي>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
size_t V3HasherUncachedHash(ثابت Ast_كلي&);

// Specialization of std::hash for VNRef
template <typename T_كلي>
بنية std::hash<VNRef<T_كلي>> final {
    size_t operator()(VNRef<T_كلي> r) ثابت { أرجع V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
template <typename T_كلي>
بنية std::equal_to<VNRef<T_كلي>> final {
    size_t operator()(VNRef<T_كلي> ra, VNRef<T_كلي> rb) ثابت {
        أرجع ra.get().same_بشيا(&(rb.get()));
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
    std::unordered_map<ثابت Ast_كلي*, Ast_كلي*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<ثابت Ast_كلي*, std::vector<Ast_كلي*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<ثابت Ast_كلي*, std::string> m_prefixes;
    ثابت VDTypeNameScopes* ثابت m_prevp;  // Index in use before this one
    ساكن ثابت VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    فراغ index(Ast_كلي* nodep, Ast_كلي* scopep);
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
    bool outerp(ثابت Ast_كلي* nodep, Ast_كلي*& outerpr) ثابت;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ثابت std::vector<Ast_كلي*>* paramsp(ثابت Ast_كلي* modp) ثابت;
    // The prefix kept for scope 'scopep', or nullptr if none
    ثابت std::string* prefixp(ثابت Ast_كلي* scopep) ثابت;
    // Keep 'prefix', final, for scope 'scopep'
    فراغ prefix(ثابت Ast_كلي* scopep, ثابت std::string& prefix) ثابت;
};

//######################################################################
// Inline VNVisitor METHODS

فراغ VNVisitorConst::iterateConst(Ast_كلي* nodep) { nodep->accept(*this); }
فراغ VNVisitorConst::iterateConstNull(Ast_كلي* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->accept(*this);
}
فراغ VNVisitorConst::iterateChildrenConst(Ast_كلي* nodep) { nodep->iterateChildrenConst(*this); }
فراغ VNVisitorConst::iterateChildrenBackwardsConst(Ast_كلي* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
فراغ VNVisitorConst::iterateAndNextConstNull(Ast_كلي* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->iterateAndNextConst(*this);
}

فراغ VNVisitor::iterate(Ast_كلي* nodep) { nodep->accept(*this); }
فراغ VNVisitor::iterateNull(Ast_كلي* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->accept(*this);
}
فراغ VNVisitor::iterateChildren(Ast_كلي* nodep) { nodep->iterateChildren(*this); }
فراغ VNVisitor::iterateAndNextNull(Ast_كلي* nodep) {
    إذا (VL_LIKELY(nodep)) nodep->iterateAndNext(*this);
}
Ast_كلي* VNVisitor::iterateSubtreeReturnEdits(Ast_كلي* nodep) {
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
فراغ dump_كلي_List_Json(std::ostream& os, ثابت Ast_كلي* nodep, ثابت std::string& listName,
                      ثابت string& indent);

#endif  // Guard
