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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Broken.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3FunctionTraits.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Number.h"
#অন্তর্ভাব "V3StdFuture.h"

#অন্তর্ভাব "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#অন্তর্ভাব <array>
#অন্তর্ভাব <cmath>
#অন্তর্ভাব <cstdint>
#অন্তর্ভাব <functional>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <type_traits>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <utility>
#অন্তর্ভাব <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#যদি defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#undef VL_USER_TYPE_CHECKS
#endif
#ifdef VL_USER_TYPE_CHECKS
#অন্তর্ভাব <variant>
#endif

// clang-format off
#অন্তর্ভাব "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3Graph;
class ExecMTask;
class VIfaceCaptureTag;

//######################################################################

// For broken() function, return error string if have a match
#define BROKEN_RTN(test) \
    করো { \
        যদি (VL_UNCOVERABLE(test)) ফেরত "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } যখন (false)
// For broken() function, return error string if a base of this class has a match
#define BROKEN_BASE_RTN(test) \
    করো { \
        ধ্রুবক অক্ষর* ধ্রুবক reasonp = (test); \
        যদি (VL_UNCOVERABLE(reasonp)) ফেরত reasonp; \
    } যখন (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#define VN_IS(nodep, nodetypename) (Ast_গ্রন্থি::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#define VN_CAST(nodep, nodetypename) (Ast_গ্রন্থি::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#define VN_AS(nodep, nodetypename) (Ast_গ্রন্থি::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#যদি defined(VL_DEBUG) || defined(VL_ASAN)
#define VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#অন্যথা
#define VN_DBG_AS(nodep, nodetypename) (Ast_গ্রন্থি::unsafePrivateAs<Ast##nodetypename>(nodep))
#endif

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#define VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

কাঠামো VNTypeInfo final {
    ধ্রুবক অক্ষর* m_namep;
    গণনা OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ধ্রুবক অক্ষর* m_opNamep[4];
    আকার_t m_sizeof;
};

class VNType final {
    স্থির ধ্রুবক VNTypeInfo s_typeInfoTable[];

সর্বজনীন:
#অন্তর্ভাব "V3Ast__gen_type_enum.h"  // From ./astgen
    ধ্রুবক en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr operator en() ধ্রুবক VL_MT_SAFE { ফেরত m_e; }

    // Retrieve VNTypeInfo for the given type
    স্থির ধ্রুবক VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { ফেরত s_typeInfoTable[t.m_e]; }
};
constexpr bool operator==(ধ্রুবক VNType& lhs, ধ্রুবক VNType& rhs) VL_PURE {
    ফেরত lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুবক VNType& lhs, VNType::en rhs) VL_PURE { ফেরত lhs.m_e == rhs; }
constexpr bool operator==(VNType::en lhs, ধ্রুবক VNType& rhs) VL_PURE { ফেরত lhs == rhs.m_e; }
অন্তঃরেখ std::ostream& operator<<(std::ostream& os, ধ্রুবক VNType& rhs) { ফেরত os << rhs.ascii(); }

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
    std::variant<std::monostate, uint64_t, শূন্য*> m_u;
#অন্যথা
    সংঘ {
        শূন্য* up;
        uint64_t uq;
    } m_u;
#endif

সর্বজনীন:
#ifdef VL_USER_TYPE_CHECKS
    VNUser() = পূর্বনির্ধারিত;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        যদি (i) m_u = i;
    }
    explicit VNUser(শূন্য* p) {
        // VNUser{nullptr} represents the monostate
        যদি (p) m_u = p;
    }
    ~VNUser() = পূর্বনির্ধারিত;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::মান, T>::type to() ধ্রুবক VL_MT_SAFE {
        যদি (std::holds_alternative<std::monostate>(m_u)) ফেরত nullptr;
        শূন্য* ধ্রুবক* ধ্রুবক upp = std::get_if<শূন্য*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        ফেরত reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() ধ্রুবক {
        যদি (std::holds_alternative<std::monostate>(m_u)) ফেরত 0;
        ধ্রুবক uint64_t* ধ্রুবক uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        ফেরত *uip;
    }
#অন্যথা
    VNUser() = পূর্বনির্ধারিত;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.up = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(শূন্য* p) { m_u.up = p; }
    ~VNUser() = পূর্বনির্ধারিত;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::মান, T>::type to() ধ্রুবক VL_MT_SAFE {
        ফেরত reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() ধ্রুবক { ফেরত m_u.uq; }
#endif
    VSymEnt* toSymEnt() ধ্রুবক { ফেরত to<VSymEnt*>(); }
    Ast_গ্রন্থি* toNodep() ধ্রুবক VL_MT_SAFE { ফেরত to<Ast_গ্রন্থি*>(); }
    V3GraphVertex* toGraphVertex() ধ্রুবক { ফেরত to<V3GraphVertex*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(ধ্রুবক শূন্য*)) ধ্রুবক;
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
VL_NOT_FINAL{সুরক্ষিত : স্থির শূন্য allocate(পূর্ণসংখ্যা id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvtToStr(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
স্থির শূন্য মুক্ত(পূর্ণসংখ্যা id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvtToStr(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
স্থির শূন্য clearcnt(পূর্ণসংখ্যা id, uint32_t& cntGblRef, ধ্রুবক bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
স্থির শূন্য checkcnt(পূর্ণসংখ্যা id, uint32_t&, ধ্রুবক bool& userBusyRef) {
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
সুরক্ষিত:
    friend class Ast_গ্রন্থি;
    স্থির uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    স্থির bool s_userBusy;  // Count is in use
সর্বজনীন:
    VNUser1InUse()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1InUse()     { মুক্ত    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    স্থির শূন্য clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    স্থির শূন্য check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2InUse final : VNUserInUseBase {
সুরক্ষিত:
    friend class Ast_গ্রন্থি;
    স্থির uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    স্থির bool s_userBusy;  // Count is in use
সর্বজনীন:
    VNUser2InUse()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2InUse()     { মুক্ত    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    স্থির শূন্য clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    স্থির শূন্য check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3InUse final : VNUserInUseBase {
সুরক্ষিত:
    friend class Ast_গ্রন্থি;
    স্থির uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    স্থির bool s_userBusy;  // Count is in use
সর্বজনীন:
    VNUser3InUse()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3InUse()     { মুক্ত    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    স্থির শূন্য clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    স্থির শূন্য check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4InUse final : VNUserInUseBase {
সুরক্ষিত:
    friend class Ast_গ্রন্থি;
    স্থির uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    স্থির bool s_userBusy;  // Count is in use
সর্বজনীন:
    VNUser4InUse()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4InUse()     { মুক্ত    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    স্থির শূন্য clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    স্থির শূন্য check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::vector<Ast_গ্রন্থি*> m_deleteps;  // Nodes to delete

সর্বজনীন:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    শূন্য ঠেলো_Deletep(Ast_গ্রন্থি* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.ঠেলো_back(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    শূন্য doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    friend class Ast_গ্রন্থি;

সর্বজনীন:
    /// Call visit()s on nodep
    অন্তঃরেখ শূন্য iterateConst(Ast_গ্রন্থি* nodep);
    /// Call visit()s on nodep
    অন্তঃরেখ শূন্য iterateConstNull(Ast_গ্রন্থি* nodep);
    /// Call visit()s on const nodep's children
    অন্তঃরেখ শূন্য iterateChildrenConst(Ast_গ্রন্থি* nodep);
    /// Call visit()s on nodep's children in backp() order
    অন্তঃরেখ শূন্য iterateChildrenBackwardsConst(Ast_গ্রন্থি* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    অন্তঃরেখ শূন্য iterate_And_পরবর্তী_Const_Null(Ast_গ্রন্থি* nodep);

    আভাসী শূন্য visit(Ast_গ্রন্থি* nodep) = 0;
    আভাসী ~VNVisitorConst() {}
#অন্তর্ভাব "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : সর্বজনীন VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
সর্বজনীন:
    /// Call visit()s on nodep
    অন্তঃরেখ শূন্য iterate(Ast_গ্রন্থি* nodep);
    /// Call visit()s on nodep
    অন্তঃরেখ শূন্য iterateNull(Ast_গ্রন্থি* nodep);
    /// Call visit()s on nodep's children
    অন্তঃরেখ শূন্য iterateChildren(Ast_গ্রন্থি* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    অন্তঃরেখ শূন্য iterate_And_পরবর্তী_Null(Ast_গ্রন্থি* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    অন্তঃরেখ Ast_গ্রন্থি* iterateSubtreeReturnEdits(Ast_গ্রন্থি* nodep);

    VNDeleter& deleter() { ফেরত m_deleter; }
    শূন্য ঠেলো_Deletep(Ast_গ্রন্থি* nodep) { deleter().ঠেলো_Deletep(nodep); }
    শূন্য doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
সুরক্ষিত:
    friend class Ast_গ্রন্থি;
    গণনা RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_পরবর্তী,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_গ্রন্থি* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_গ্রন্থি* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_গ্রন্থি** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

সর্বজনীন:
    VNRelinker() = পূর্বনির্ধারিত;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    অন্তঃরেখ শূন্য relink(Ast_গ্রন্থি* newp);
    শূন্য dump(std::ostream& str = std::cout) ধ্রুবক;
};
অন্তঃরেখ std::ostream& operator<<(std::ostream& os, ধ্রুবক VNRelinker& rhs) {
    rhs.dump(os);
    ফেরত os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{সর্বজনীন : আভাসী bool গ্রন্থি_Match(ধ্রুবক Ast_গ্রন্থি* nodep) ধ্রুবক {ফেরত true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#define ASTNODE_PREFETCH_NON_NULL(nodep) \
    করো { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } যখন (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#define ASTNODE_PREFETCH(nodep) \
    করো { \
        যদি (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } যখন (false)

class Ast_গ্রন্থি VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_গ্রন্থি* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_গ্রন্থি* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_গ্রন্থি* m_op1p = nullptr;  // Generic pointer 1
    Ast_গ্রন্থি* m_op2p = nullptr;  // Generic pointer 2
    Ast_গ্রন্থি* m_op3p = nullptr;  // Generic pointer 3
    Ast_গ্রন্থি* m_op4p = nullptr;  // Generic pointer 4
    Ast_গ্রন্থি** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    ধ্রুবক VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    কাঠামো {
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

    পূর্ণসংখ্যা m_cloneCnt = 0;  // Sequence number for when last clone was made

#যদি defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(আকার(m_type) + আকার(m_flags) + আকার(m_brokenState) + আকার(m_cloneCnt)
                      <= আকার(শূন্য*),
                  "packing requires padding");
#endif

    Ast_গ্রন্থি_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_গ্রন্থি* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_সারি* m_fileline;  // Where it was declared
#ifdef VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_গুনতি;  // When it was last edited
#endif
    স্থির uint64_t s_editCntGbl;  // Global edit counter
    স্থির uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_গ্রন্থি* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    স্থির পূর্ণসংখ্যা s_cloneCntGbl;  // Count of which userp is set

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
    শূন্য op1p(Ast_গ্রন্থি* nodep) {
        m_op1p = nodep;
        যদি (nodep) nodep->m_backp = this;
    }
    শূন্য op2p(Ast_গ্রন্থি* nodep) {
        m_op2p = nodep;
        যদি (nodep) nodep->m_backp = this;
    }
    শূন্য op3p(Ast_গ্রন্থি* nodep) {
        m_op3p = nodep;
        যদি (nodep) nodep->m_backp = this;
    }
    শূন্য op4p(Ast_গ্রন্থি* nodep) {
        m_op4p = nodep;
        যদি (nodep) nodep->m_backp = this;
    }

ব্যক্তিগত:
    Ast_গ্রন্থি* clone_বৃক্ষ_Iter(bool needPure);
    Ast_গ্রন্থি* clone_বৃক্ষ_Iter_List(bool needPure);
    শূন্য check_বৃক্ষ_Iter(ধ্রুবক Ast_গ্রন্থি* পূর্ব_Backp) ধ্রুবক VL_MT_STABLE;
    bool gate_বৃক্ষ_Iter() ধ্রুবক;
    স্থির bool same_বৃক্ষ_Iter(ধ্রুবক Ast_গ্রন্থি* node1p, ধ্রুবক Ast_গ্রন্থি* node2p, bool ign_পরবর্তী,
                             bool gateOnly);
    শূন্য delete_বৃক্ষ_Iter();
    শূন্য delete_গ্রন্থি();
    string instanceStr() ধ্রুবক;

সর্বজনীন:
    শূন্য purityCheck();
    স্থির শূন্য relinkOneLink(Ast_গ্রন্থি*& pointpr, Ast_গ্রন্থি* newp);
    // cppcheck-suppress functionConst
    স্থির শূন্য debug_বৃক্ষ_Change(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক অক্ষর* prefix, পূর্ণসংখ্যা lineno, bool পরবর্তী);

সুরক্ষিত:
    // CONSTRUCTORS
    Ast_গ্রন্থি(VNType t, File_সারি* fl);
    আভাসী ~Ast_গ্রন্থি() = পূর্বনির্ধারিত;  // Use 'deleteTree' instead
    আভাসী Ast_গ্রন্থি* clone() = 0;  // Generally, cloneTree is what you want instead
    আভাসী শূন্য cloneRelink() { cloneRelinkGen(); }
    আভাসী শূন্য cloneRelinkGen() {};  // Overrides generated by 'astgen'
    শূন্য clone_Relink_বৃক্ষ();

    // METHODS
    শূন্য setOp1p(Ast_গ্রন্থি* newp);  // Set non-list-type op1 to non-list element
    শূন্য setOp2p(Ast_গ্রন্থি* newp);  // Set non-list-type op2 to non-list element
    শূন্য setOp3p(Ast_গ্রন্থি* newp);  // Set non-list-type op3 to non-list element
    শূন্য setOp4p(Ast_গ্রন্থি* newp);  // Set non-list-type op4 to non-list element

    শূন্য addOp1p(Ast_গ্রন্থি* newp);  // Append newp to end of op1
    শূন্য addOp2p(Ast_গ্রন্থি* newp);  // Append newp to end of op2
    শূন্য addOp3p(Ast_গ্রন্থি* newp);  // Append newp to end of op3
    শূন্য addOp4p(Ast_গ্রন্থি* newp);  // Append newp to end of op4

    শূন্য prependOp1p(Ast_গ্রন্থি* newp);  // Prepend newp to front of op1
    শূন্য prependOp2p(Ast_গ্রন্থি* newp);  // Prepend newp to front of op2
    শূন্য prependOp3p(Ast_গ্রন্থি* newp);  // Prepend newp to front of op3
    শূন্য prependOp4p(Ast_গ্রন্থি* newp);  // Prepend newp to front of op4

    // clang-format off
    শূন্য setNOp1p(Ast_গ্রন্থি* newp) { যদি (newp) setOp1p(newp); }
    শূন্য setNOp2p(Ast_গ্রন্থি* newp) { যদি (newp) setOp2p(newp); }
    শূন্য setNOp3p(Ast_গ্রন্থি* newp) { যদি (newp) setOp3p(newp); }
    শূন্য setNOp4p(Ast_গ্রন্থি* newp) { যদি (newp) setOp4p(newp); }

    শূন্য addNOp1p(Ast_গ্রন্থি* newp) { যদি (newp) addOp1p(newp); }
    শূন্য addNOp2p(Ast_গ্রন্থি* newp) { যদি (newp) addOp2p(newp); }
    শূন্য addNOp3p(Ast_গ্রন্থি* newp) { যদি (newp) addOp3p(newp); }
    শূন্য addNOp4p(Ast_গ্রন্থি* newp) { যদি (newp) addOp4p(newp); }

    শূন্য prependNOp1p(Ast_গ্রন্থি* newp) { যদি (newp) prependOp1p(newp); }
    শূন্য prependNOp2p(Ast_গ্রন্থি* newp) { যদি (newp) prependOp2p(newp); }
    শূন্য prependNOp3p(Ast_গ্রন্থি* newp) { যদি (newp) prependOp3p(newp); }
    শূন্য prependNOp4p(Ast_গ্রন্থি* newp) { যদি (newp) prependOp4p(newp); }
    // clang-format on

    শূন্য clonep(Ast_গ্রন্থি* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    স্থির শূন্য clone_Clear_বৃক্ষ() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    আভাসী bool same_গ্রন্থি(ধ্রুবক Ast_গ্রন্থি*) ধ্রুবক { ফেরত true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    আভাসী bool wouldBreakGen(ধ্রুবক Ast_গ্রন্থি* ধ্রুবক oldp,
                               ধ্রুবক Ast_গ্রন্থি* ধ্রুবক newp) ধ্রুবক
        = 0;  // Generated by 'astgen'

সর্বজনীন:
    // ACCESSORS
    VNType type() ধ্রুবক VL_MT_SAFE { ফেরত m_type; }
    ধ্রুবক অক্ষর* typeName() ধ্রুবক VL_MT_SAFE { ফেরত type().ascii(); }  // See also prettyTypeName
    Ast_গ্রন্থি* nextp() ধ্রুবক VL_MT_STABLE { ফেরত m_nextp; }
    Ast_গ্রন্থি* backp() ধ্রুবক VL_MT_STABLE { ফেরত m_backp; }
    Ast_গ্রন্থি* abovep() ধ্রুবক;  // Get parent node above, only for list head and tail
    Ast_গ্রন্থি* aboveLoopp() ধ্রুবক;  // Get parent node above, may have performance issues as loops
    Ast_গ্রন্থি* lastp() ধ্রুবক {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        ফেরত m_headtailp;
    }
    Ast_গ্রন্থি* prevp() ধ্রুবক {  // Previous node in list, nullptr for head of list
        যদি (m_backp->m_nextp != this) ফেরত nullptr;
        ফেরত m_backp;
    }
    Ast_গ্রন্থি* op1p() ধ্রুবক VL_MT_STABLE { ফেরত m_op1p; }
    Ast_গ্রন্থি* op2p() ধ্রুবক VL_MT_STABLE { ফেরত m_op2p; }
    Ast_গ্রন্থি* op3p() ধ্রুবক VL_MT_STABLE { ফেরত m_op3p; }
    Ast_গ্রন্থি* op4p() ধ্রুবক VL_MT_STABLE { ফেরত m_op4p; }
    Ast_গ্রন্থি_DType* dtypep() ধ্রুবক VL_MT_STABLE { ফেরত m_dtypep; }
    Ast_গ্রন্থি* clonep() ধ্রুবক { ফেরত ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_গ্রন্থি* firstAbovep() ধ্রুবক {  // Returns nullptr when second or later in list
        ফেরত ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ধ্রুবক VL_MT_SAFE { ফেরত m_brokenState; }
    শূন্য brokenState(uint8_t মান) { m_brokenState = মান; }

    // Used by AstNode::broken()
    bool brokeExists() ধ্রুবক { ফেরত V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ধ্রুবক { ফেরত brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    আভাসী শূন্য foreachLink(std::function<শূন্য(Ast_গ্রন্থি** linkpp, ধ্রুবক অক্ষর* namep)> f) = 0;

#যদি defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    স্থির শূন্য* operator new(আকার_t আকার_2);
    স্থির শূন্য operator delete(শূন্য* obj, আকার_t আকার_2);
#endif

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_BRANCH = 4;  // Branch
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_CALL = INSTR_গুনতি_BRANCH + 10;  // Subroutine call
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_LD = 2;  // Load memory
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_INT_MUL = 3;  // Integer multiply
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_INT_DIV = 10;  // Integer divide
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_DBL = 8;  // Convert or do float ops
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_DBL_DIV = 40;  // Double divide
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_DBL_TRIG = 200;  // Double trigonometric ops
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_STR = 100;  // String ops
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_TIME = INSTR_গুনতি_CALL + 5;  // Determine simulation time
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_PLI = 20;  // PLI routines
    স্থির constexpr পূর্ণসংখ্যা INSTR_গুনতি_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    আভাসী string name() ধ্রুবক VL_MT_STABLE { ফেরত ""; }
    আভাসী string origName() ধ্রুবক { ফেরত ""; }
    string prettyDehashOrigOrName() ধ্রুবক {
        ফেরত prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    আভাসী শূন্য name(ধ্রুবক string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    আভাসী শূন্য tag(ধ্রুবক string& text) {}
    আভাসী string tag() ধ্রুবক { ফেরত ""; }
    আভাসী uint32_t decl_Token_অঙ্ক() ধ্রুবক { ফেরত 0; }
    আভাসী শূন্য decl_Token_অঙ্ক_Set_Min(uint32_t token_অঙ্ক) {}
    আভাসী string verilogKwd() ধ্রুবক { ফেরত ""; }
    string nameProtect() ধ্রুবক VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() ধ্রুবক;  // origName with --protect-id applied
    string shortName() ধ্রুবক;  // Name with __PVT__ removed for concatenating scopes
    স্থির string dedotName(ধ্রুবক string& namein);  // Name with dots removed
    স্থির string nameNoArray(ধ্রুবক string& namein);  // Name with any array index removed
    স্থির string prettyName(ধ্রুবক string& namein) VL_PURE;  // Name for printing out to the user
    স্থির string vpiName(ধ্রুবক string& namein);  // Name for vpi access
    স্থির string prettyNameQ(ধ্রুবক string& namein) {  // Quoted pretty name (for errors)
        ফেরত "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    স্থির string encodeName(ধ্রুবক string& namein);
    স্থির string encode_সংখ্যা(int64_t অঙ্ক);  // Encode number into internal C representation
    স্থির string vcdName(ধ্রুবক string& namein);  // Name for printing out to vcd files
    string prettyName() ধ্রুবক { ফেরত prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    আভাসী string pretty_Name_বার্তা() ধ্রুবক { ফেরত prettyName(); }
    string prettyNameQ() ধ্রুবক { ফেরত "'"s + pretty_Name_বার্তা() + "'"; }  // Quoted, for messages
    string verilogName() ধ্রুবক { ফেরত vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ধ্রুবক;
    আভাসী string prettyOperatorName() ধ্রুবক { ফেরত "operator " + prettyTypeName(); }
    File_সারি* fileline() ধ্রুবক VL_MT_SAFE { ফেরত m_fileline; }
    শূন্য fileline(File_সারি* fl) { m_fileline = fl; }
    অন্তঃরেখ bool width1() ধ্রুবক;
    অন্তঃরেখ পূর্ণসংখ্যা widthInstrs() ধ্রুবক;
    bool didWidth() ধ্রুবক { ফেরত m_flags.didWidth; }
    শূন্য didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        যদি (didWidth()) ফেরত true;
        didWidth(true);
        ফেরত false;
    }
    bool doingWidth() ধ্রুবক { ফেরত m_flags.doingWidth; }
    শূন্য doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() ধ্রুবক VL_MT_SAFE { ফেরত m_flags.protect; }
    শূন্য protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    অন্তঃরেখ পূর্ণসংখ্যা width() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ পূর্ণসংখ্যা widthMin() ধ্রুবক VL_MT_STABLE;
    পূর্ণসংখ্যা widthMinV() ধ্রুবক {
        ফেরত v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    পূর্ণসংখ্যা width_শব্দগুলি() ধ্রুবক { ফেরত VL_শব্দগুলি_I(width()); }
    bool isQuad() ধ্রুবক VL_MT_STABLE { ফেরত (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() ধ্রুবক VL_MT_STABLE { ফেরত (width() > VL_QUADSIZE); }
    অন্তঃরেখ bool isCHandle() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ bool isDouble() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ bool isSigned() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ bool isString() ধ্রুবক VL_MT_STABLE;
    অন্তঃরেখ bool isEvent() ধ্রুবক VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ধ্রুবক VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        ফেরত ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_গ্রন্থি* user1p() ধ্রুবক VL_MT_STABLE { ফেরত user1u().toNodep(); }
    শূন্য user1u(ধ্রুবক VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    শূন্য user1p(শূন্য* userp) { user1u(VNUser{userp}); }
    শূন্য user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ধ্রুবক { ফেরত user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ধ্রুবক uint64_t v = user1(); user1(v + val); ফেরত v; }
    uint64_t user1Or(uint64_t val) { ধ্রুবক uint64_t v = user1(); user1(v | val); ফেরত v; }
    uint64_t user1SetOnce() { ধ্রুবক uint64_t v = user1(); যদি (!v) user1(1); ফেরত v; }  // Better for cache than user1Inc()
    স্থির শূন্য user1_Clear_বৃক্ষ() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ধ্রুবক VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        ফেরত ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_গ্রন্থি* user2p() ধ্রুবক VL_MT_STABLE { ফেরত user2u().toNodep(); }
    শূন্য user2u(ধ্রুবক VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    শূন্য user2p(শূন্য* userp) { user2u(VNUser{userp}); }
    শূন্য user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ধ্রুবক { ফেরত user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ধ্রুবক uint64_t v = user2(); user2(v + val); ফেরত v; }
    uint64_t user2Or(uint64_t val) { ধ্রুবক uint64_t v = user2(); user2(v | val); ফেরত v; }
    uint64_t user2SetOnce() { ধ্রুবক uint64_t v = user2(); যদি (!v) user2(1); ফেরত v; }  // Better for cache than user2Inc()
    স্থির শূন্য user2_Clear_বৃক্ষ() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ধ্রুবক VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        ফেরত ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_গ্রন্থি* user3p() ধ্রুবক VL_MT_STABLE { ফেরত user3u().toNodep(); }
    শূন্য user3u(ধ্রুবক VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    শূন্য user3p(শূন্য* userp) { user3u(VNUser{userp}); }
    শূন্য user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ধ্রুবক { ফেরত user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ধ্রুবক uint64_t v = user3(); user3(v + val); ফেরত v; }
    uint64_t user3Or(uint64_t val) { ধ্রুবক uint64_t v = user3(); user3(v | val); ফেরত v; }
    uint64_t user3SetOnce() { ধ্রুবক uint64_t v = user3(); যদি (!v) user3(1); ফেরত v; }  // Better for cache than user3Inc()
    স্থির শূন্য user3_Clear_বৃক্ষ() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ধ্রুবক VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        ফেরত ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_গ্রন্থি* user4p() ধ্রুবক VL_MT_STABLE { ফেরত user4u().toNodep(); }
    শূন্য user4u(ধ্রুবক VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    শূন্য user4p(শূন্য* userp) { user4u(VNUser{userp}); }
    শূন্য user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ধ্রুবক { ফেরত user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { ধ্রুবক uint64_t v = user4(); user4(v | val); ফেরত v; }
    uint64_t user4Inc(uint64_t val = 1) { ধ্রুবক uint64_t v = user4(); user4(v + val); ফেরত v; }
    uint64_t user4SetOnce() { ধ্রুবক uint64_t v = user4(); যদি (!v) user4(1); ফেরত v; }  // Better for cache than user4Inc()
    স্থির শূন্য user4_Clear_বৃক্ষ() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ifdef VL_DEBUG
    uint64_t edit_গুনতি() ধ্রুবক { ফেরত m_edit_গুনতি; }
    শূন্য edit_গুনতি_Inc() {
        m_edit_গুনতি = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_বৃক্ষ();  // Any edit clears all caching
    }
#অন্যথা
    শূন্য edit_গুনতি_Inc() { ++s_editCntGbl; }
#endif
    স্থির uint64_t edit_গুনতি_Last() VL_MT_SAFE { ফেরত s_editCntLast; }
    স্থির uint64_t edit_গুনতি_Gbl() VL_MT_SAFE { ফেরত s_editCntGbl; }
    স্থির শূন্য edit_গুনতি_Set_Last() { s_editCntLast = edit_গুনতি_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_ঠেলো_Self_Stmt();
    অন্তঃরেখ bool is_Class_Handle_মান() ধ্রুবক;
    অন্তঃরেখ bool isNull() ধ্রুবক;
    অন্তঃরেখ bool isZero() ধ্রুবক;
    অন্তঃরেখ bool isOne() ধ্রুবক;
    অন্তঃরেখ bool isNeqZero() ধ্রুবক;
    অন্তঃরেখ bool isAllOnes() ধ্রুবক;
    অন্তঃরেখ bool isAllOnesV() ধ্রুবক;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    শূন্য dtypep(Ast_গ্রন্থি_DType* nodep) {
        যদি (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_গুনতি_Inc();
        }
    }
    শূন্য dtypeFrom(ধ্রুবক Ast_গ্রন্থি* fromp) {
        যদি (fromp) dtypep(fromp->dtypep());
    }
    শূন্য dtypeChgSigned(bool flag = true);
    শূন্য dtypeChgWidth(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin);
    শূন্য dtypeChgWidthSigned(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric);
    শূন্য dtypeSetBitUnsized(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) {
        dtypep(findBitDType(width, widthMin, numeric));
    }
    শূন্য dtypeSetBitSized(পূর্ণসংখ্যা width, VSigning numeric) {
        dtypep(findBitDType(width, width, numeric));  // Since sized, widthMin is width
    }
    শূন্য dtypeSetLogicUnsized(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) {
        dtypep(findLogicDType(width, widthMin, numeric));
    }
    শূন্য dtypeSetLogicSized(পূর্ণসংখ্যা width, VSigning numeric) {
        dtypep(findLogicDType(width, width, numeric));  // Since sized, widthMin is width
    }
    শূন্য dtypeSetLogic2StateUnsized(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) {
        dtypep(findLogic2StateDType(width, widthMin, numeric));
    }
    শূন্য dtypeSetLogic2StateSized(পূর্ণসংখ্যা width, VSigning numeric) {
        dtypep(findLogic2StateDType(width, width, numeric));  // Since sized, widthMin is width
    }
    শূন্য dtypeSetBitOrLogicUnsized(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, widthMin, numeric, isFourstate));
    }
    শূন্য dtypeSetBitOrLogicSized(পূর্ণসংখ্যা width, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, width, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    শূন্য dtypeSetBit() { dtypep(findBitDType()); }
    শূন্য dtypeSetLogic() { dtypep(findLogicDType()); }
    শূন্য dtypeSetLogic2State() { dtypep(findLogic2StateDType()); }
    শূন্য dtypeSetDouble() { dtypep(findDoubleDType()); }
    শূন্য dtypeSetInt() { dtypep(findIntDType()); }
    শূন্য dtypeSetInteger() { dtypep(findIntegerDType()); }
    শূন্য dtypeSetInteger2State() { dtypep(findInteger2StateDType()); }
    শূন্য dtypeSetString() { dtypep(findStringDType()); }
    শূন্য dtypeSetUInt32() { dtypep(findUInt32DType()); }  // Twostate
    শূন্য dtypeSetUInt64() { dtypep(findUInt64DType()); }  // Twostate
    শূন্য dtypeSetTime() { dtypep(findTimeDType()); }
    শূন্য dtypeSetTime2State() { dtypep(findTime2StateDType()); }
    শূন্য dtypeSetEmptyQueue() { dtypep(findEmptyQueueDType()); }
    শূন্য dtypeSetStream() { dtypep(findStreamDType()); }
    শূন্য dtypeSetVoid() { dtypep(findVoidDType()); }
    শূন্য dtypeSetBitOrLogic(bool isFourstate) {
        dtypep(isFourstate ? findLogicDType() : findBitDType());
    }

    // Data type locators
    Ast_গ্রন্থি_DType* findBitDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_গ্রন্থি_DType* findLogicDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_গ্রন্থি_DType* findNullDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_গ্রন্থি_DType* findLogic2StateDType() ধ্রুবক {
        ফেরত findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_গ্রন্থি_DType* findDoubleDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_গ্রন্থি_DType* findIntDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::INT); }
    Ast_গ্রন্থি_DType* findIntegerDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_গ্রন্থি_DType* findInteger2StateDType() ধ্রুবক {
        ফেরত findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_গ্রন্থি_DType* findStringDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_গ্রন্থি_DType* findSigned8DType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_গ্রন্থি_DType* findUInt32DType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_গ্রন্থি_DType* findUInt64DType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_গ্রন্থি_DType* findTimeDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_গ্রন্থি_DType* findTime2StateDType() ধ্রুবক {
        ফেরত findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_গ্রন্থি_DType* findCHandleDType() ধ্রুবক { ফেরত findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_গ্রন্থি_DType* findConstraintRefDType() ধ্রুবক;
    Ast_গ্রন্থি_DType* findEmptyQueueDType() ধ্রুবক;
    Ast_গ্রন্থি_DType* find_Queue_সূচক_DType() ধ্রুবক;
    Ast_গ্রন্থি_DType* findStreamDType() ধ্রুবক;
    Ast_গ্রন্থি_DType* findVoidDType() ধ্রুবক;
    Ast_গ্রন্থি_DType* findBitDType(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) ধ্রুবক;
    Ast_গ্রন্থি_DType* findLogicDType(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) ধ্রুবক;
    Ast_গ্রন্থি_DType* findLogic2StateDType(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) ধ্রুবক;
    Ast_গ্রন্থি_DType* findBitOrLogicDType(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric,
                                      bool isFourstate) ধ্রুবক {
        ফেরত isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_গ্রন্থি_DType* findBitOrLogicRangeDType(ধ্রুবক VNumRange& range, ধ্রুবক পূর্ণসংখ্যা widthMin,
                                           ধ্রুবক VSigning numeric,
                                           ধ্রুবক bool isFourstate) ধ্রুবক VL_MT_STABLE {
        ফেরত isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_গ্রন্থি_DType* findLogicRangeDType(ধ্রুবক VNumRange& range, পূর্ণসংখ্যা widthMin,
                                      VSigning numeric) ধ্রুবক VL_MT_STABLE;
    Ast_গ্রন্থি_DType* findBitRangeDType(ধ্রুবক VNumRange& range, পূর্ণসংখ্যা widthMin,
                                    VSigning numeric) ধ্রুবক VL_MT_STABLE;
    Ast_গ্রন্থি_DType* findBasicDType(VBasicDTypeKwd kwd) ধ্রুবক;
    স্থির AstBasicDType* find_ঢোকাও_Same_DType(AstBasicDType* nodep);

    স্থির VCastable computeCastable(ধ্রুবক Ast_গ্রন্থি_DType* toDtp, ধ্রুবক Ast_গ্রন্থি_DType* fromDtp,
                                     ধ্রুবক Ast_গ্রন্থি* fromConstp, bool checkIfaceArgCompat = false);
    স্থির Ast_গ্রন্থি_DType* getCommonClassTypep(Ast_গ্রন্থি* nodep1, Ast_গ্রন্থি* nodep2);

    // METHODS - dump and error
    শূন্য v3errorEnd(ধ্রুবক std::ostringstream& str) ধ্রুবক VL_RELEASE(V3Error::s().m_mutex);
    শূন্য v3errorEndFatal(ধ্রুবক std::ostringstream& str) ধ্রুবক VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ধ্রুবক VL_REQUIRES(V3Error::s().m_mutex) {
        ফেরত fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ধ্রুবক { ফেরত fileline()->warnContextSecondary(); }
    string warnMore() ধ্রুবক VL_REQUIRES(V3Error::s().m_mutex) { ফেরত fileline()->warnMore(); }
    string warnOther() ধ্রুবক VL_REQUIRES(V3Error::s().m_mutex) { ফেরত fileline()->warnOther(); }

    আভাসী শূন্য dump(std::ostream& str = std::cout) ধ্রুবক;
    স্থির অক্ষর* dump_বৃক্ষ_Json_Gdb(ধ্রুবক Ast_গ্রন্থি* nodep);  // For GDB only, free()d by caller
    স্থির অক্ষর* dump_বৃক্ষ_Json_Gdb(ধ্রুবক অক্ষর* str);  // For GDB only, free()d by caller
    স্থির অক্ষর* dump_বৃক্ষ_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    স্থির শূন্য dumpGdb(ধ্রুবক Ast_গ্রন্থি* nodep);  // For GDB only
    শূন্য dumpGdbHeader() ধ্রুবক;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    template <typename T_গ্রন্থি_ফলাফল, typename T_গ্রন্থি_পরবর্তী>
    স্থির T_গ্রন্থি_ফলাফল* add_পরবর্তী(T_গ্রন্থি_ফলাফল* nodep, T_গ্রন্থি_পরবর্তী* newp) {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি_ফলাফল>::মান,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_গ্রন্থি_ফলাফল, T_গ্রন্থি_পরবর্তী>::মান,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        যদি (!newp) ফেরত nodep;
        ফেরত static_cast<T_গ্রন্থি_ফলাফল*>(add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, newp));
    }
    template <typename T_গ্রন্থি_ফলাফল, typename T_গ্রন্থি_পরবর্তী>
    স্থির T_গ্রন্থি_ফলাফল* add_পরবর্তী_Null(T_গ্রন্থি_ফলাফল* nodep, T_গ্রন্থি_পরবর্তী* newp) {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি_ফলাফল>::মান,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_গ্রন্থি_ফলাফল, T_গ্রন্থি_পরবর্তী>::মান,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        যদি (!newp) ফেরত nodep;
        যদি (!nodep) ফেরত newp;
        ফেরত static_cast<T_গ্রন্থি_ফলাফল*>(add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, newp));
    }
    অন্তঃরেখ Ast_গ্রন্থি* add_পরবর্তী(Ast_গ্রন্থি* newp);
    শূন্য add_পরবর্তী_Here(Ast_গ্রন্থি* newp);  // Insert newp at this->nextp
    শূন্য add_Here_This_As_পরবর্তী(Ast_গ্রন্থি* newp);  // Adds at old place of this, this becomes next
    শূন্য replaceWith(Ast_গ্রন্থি* newp);  // Replace current node in tree with new node
    শূন্য replaceWithKeepDType(Ast_গ্রন্থি* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_গ্রন্থি* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_গ্রন্থি* unlink_Fr_Back_With_পরবর্তী(VNRelinker* linkerp = nullptr);
    শূন্য relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    শূন্য clone_Relink_গ্রন্থি() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_গ্রন্থি* clone_বৃক্ষ(bool clone_পরবর্তী_Link,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_গ্রন্থি* clone_বৃক্ষ_Pure(bool clone_পরবর্তী_Link) { ফেরত clone_বৃক্ষ(clone_পরবর্তী_Link, true); }
    bool gate_বৃক্ষ() { ফেরত gate_বৃক্ষ_Iter(); }  // Is tree isGateOptimizable?
    অন্তঃরেখ bool same_বৃক্ষ(ধ্রুবক Ast_গ্রন্থি* node2p) ধ্রুবক;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    অন্তঃরেখ bool same_Gate_বৃক্ষ(ধ্রুবক Ast_গ্রন্থি* node2p) ধ্রুবক;
    শূন্য delete_বৃক্ষ();  // Always deletes the next link
    শূন্য check_বৃক্ষ() ধ্রুবক VL_MT_STABLE {
        যদি (v3Global.opt.debugCheck()) check_বৃক্ষ_Iter(backp());
    }
    শূন্য checkIter() ধ্রুবক;
    শূন্য dumpPtrs(std::ostream& os = std::cout) ধ্রুবক;
    শূন্য dump_বৃক্ষ(std::ostream& os = std::cout, ধ্রুবক string& indent = "    ",
                  পূর্ণসংখ্যা maxDepth = 0) ধ্রুবক;
    শূন্য dump_বৃক্ষ(ধ্রুবক string& indent, পূর্ণসংখ্যা maxDepth = 0) ধ্রুবক {
        dump_বৃক্ষ(std::cout, indent, maxDepth);
    }
    স্থির শূন্য dump_বৃক্ষ_Gdb(ধ্রুবক Ast_গ্রন্থি* nodep);  // For GDB only
    শূন্য dump_বৃক্ষ_And_পরবর্তী(std::ostream& os = std::cout, ধ্রুবক string& indent = "    ",
                         পূর্ণসংখ্যা maxDepth = 0) ধ্রুবক;
    শূন্য dump_বৃক্ষ_File(ধ্রুবক string& filename, bool doDump = true);
    স্থির শূন্য dump_বৃক্ষ_File_Gdb(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক অক্ষর* filenamep = nullptr);
    শূন্য dump_বৃক্ষ_Dot(std::ostream& os = std::cout) ধ্রুবক;
    শূন্য dump_বৃক্ষ_Dot_File(ধ্রুবক string& filename, bool doDump = true);
    আভাসী শূন্য dumpJson(std::ostream& os) ধ্রুবক { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    শূন্য dumpJsonGen(std::ostream& os) ধ্রুবক {};
    আভাসী শূন্য dump_বৃক্ষ_Json_Op_Gen(std::ostream& os, ধ্রুবক string& indent) ধ্রুবক {};
    শূন্য dump_বৃক্ষ_Json(std::ostream& os, ধ্রুবক string& indent = "") ধ্রুবক;
    শূন্য dump_বৃক্ষ_Json_File(ধ্রুবক string& filename, bool doDump = true);
    স্থির শূন্য dumpJsonMetaFileGdb(ধ্রুবক অক্ষর* filename);
    স্থির শূন্য dumpJsonMetaFile(ধ্রুবক string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    স্থির std::string গ্রন্থি_Addr(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ফেরত v3Global.opt.dump_বৃক্ষ_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    স্থির Ast_গ্রন্থি* afterCommentp(Ast_গ্রন্থি* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        যখন (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        ফেরত nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    আভাসী bool isBrancher() ধ্রুবক { ফেরত false; }
    // Else a AstTime etc that can't be pushed out
    আভাসী bool isGateOptimizable() ধ্রুবক { ফেরত !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    আভাসী bool isGateDedupable() ধ্রুবক { ফেরত isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    আভাসী bool is_রাশিমালা_Coverage_Eligible() ধ্রুবক { ফেরত isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    আভাসী bool isOutputter() { ফেরত false; }
    // Else a AstTime etc which output can't be predicted from input
    আভাসী bool isPredictOptimizable() ধ্রুবক { ফেরত !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    আভাসী bool isPure() { ফেরত true; }
    // Iff isPure on current node and any nextp()
    bool is_Pure_And_পরবর্তী() { ফেরত isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    আভাসী bool isTimingControl() ধ্রুবক { ফেরত false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    আভাসী bool isUnlikely() ধ্রুবক { ফেরত false; }
    // Is an IEEE system function (versus internally-generated)
    আভাসী bool isSystemFunc() ধ্রুবক { ফেরত false; }
    আভাসী পূর্ণসংখ্যা instr_গুনতি() ধ্রুবক { ফেরত 0; }
    // Iff node is identical to another node
    আভাসী bool isSame(ধ্রুবক Ast_গ্রন্থি* samep) ধ্রুবক {
        ফেরত type() == samep->type() && same_গ্রন্থি(samep);
    }
    // Iff has a data type; dtype() must be non-null
    আভাসী bool hasDType() ধ্রুবক VL_MT_SAFE { ফেরত false; }
    // Iff has a non-null childDTypep(), as generic node function
    আভাসী Ast_গ্রন্থি_DType* getChildDTypep() ধ্রুবক { ফেরত nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    আভাসী Ast_গ্রন্থি_DType* getChild2DTypep() ধ্রুবক { ফেরত nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    আভাসী bool maybePointedTo() ধ্রুবক VL_MT_SAFE { ফেরত false; }
    // Don't reclaim this node in V3Dead
    আভাসী bool undead() ধ্রুবক { ফেরত false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    আভাসী ধ্রুবক অক্ষর* broken() ধ্রুবক { ফেরত nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    আভাসী ধ্রুবক অক্ষর* brokenGen() ধ্রুবক = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ধ্রুবক Ast_গ্রন্থি* ধ্রুবক newp) ধ্রুবক { ফেরত backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    আভাসী শূন্য accept(VNVisitorConst& v) = 0;

সুরক্ষিত:
    // All VNVisitor related functions are called as methods off the visitor
    friend class VNVisitor;
    friend class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    শূন্য iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    শূন্য iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    শূন্য iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    শূন্য iterate_And_পরবর্তী(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    শূন্য iterate_And_পরবর্তী_Const(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_গ্রন্থি* iterateSubtreeReturnEdits(VNVisitor& v);

    স্থির শূন্য dump_Json_অঙ্ক(std::ostream& os, ধ্রুবক std::string& name, int64_t val);
    স্থির শূন্য dumpJsonBoolIf(std::ostream& os, ধ্রুবক std::string& name, bool val);
    স্থির শূন্য dumpJsonStr(std::ostream& os, ধ্রুবক std::string& name, ধ্রুবক std::string& val);
    স্থির শূন্য dumpJsonPtr(std::ostream& os, ধ্রুবক std::string& name, ধ্রুবক Ast_গ্রন্থি* ধ্রুবক valp);

সুরক্ষিত:
    শূন্য iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    template <typename T>
    অন্তঃরেখ স্থির bool privateTypeTest(ধ্রুবক Ast_গ্রন্থি* nodep);

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr স্থির bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        ফেরত std::is_base_of<T_TargetType, NonCV>::মান;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr স্থির bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        ফেরত !std::is_base_of<NonCV, T_TargetType>::মান;
    }  // LCOV_EXCL_STOP

সর্বজনীন:
    // For use via the VN_IS macro only, or in templated code
    template <typename T_TargetType, typename T_গ্রন্থি>
    স্থির bool is(ধ্রুবক T_গ্রন্থি* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        ফেরত nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    template <typename T_TargetType, typename T_গ্রন্থি>
    স্থির T_TargetType* cast(T_গ্রন্থি* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        ফেরত nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    template <typename T_TargetType, typename T_গ্রন্থি>
    স্থির ধ্রুবক T_TargetType* cast(ধ্রুবক T_গ্রন্থি* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        ফেরত nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<ধ্রুবক T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    template <typename T_TargetType, typename T_গ্রন্থি>
    স্থির T_TargetType* as(T_গ্রন্থি* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        ফেরত unsafePrivateAs<T_TargetType, T_গ্রন্থি>(nodep);
    }
    template <typename T_TargetType, typename T_গ্রন্থি>
    স্থির ধ্রুবক T_TargetType* as(ধ্রুবক T_গ্রন্থি* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        ফেরত unsafePrivateAs<T_TargetType, T_গ্রন্থি>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    template <typename T_TargetType, typename T_গ্রন্থি>
    স্থির T_TargetType* unsafePrivateAs(T_গ্রন্থি* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        ফেরত reinterpret_cast<T_TargetType*>(nodep);
    }
    template <typename T_TargetType, typename T_গ্রন্থি>
    স্থির ধ্রুবক T_TargetType* unsafePrivateAs(ধ্রুবক T_গ্রন্থি* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_গ্রন্থি>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        ফেরত reinterpret_cast<ধ্রুবক T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_গ্রন্থি>
    স্থির bool mayBeUnder(ধ্রুবক Ast_গ্রন্থি* nodep) {
        static_assert(!std::is_const<T_গ্রন্থি>::মান,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        ফেরত true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_গ্রন্থি>
    স্থির constexpr bool isLeaf() {
        static_assert(!std::is_const<T_গ্রন্থি>::মান,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        ফেরত false;
    }

ব্যক্তিগত:
    // Using std::conditional for const correctness in the public 'foreach' functions
    template <typename T_Arg>
    using Const_Correct_Ast_গ্রন্থি =
        typename std::conditional<std::is_const<T_Arg>::মান, ধ্রুবক Ast_গ্রন্থি, Ast_গ্রন্থি>::type;

    template <typename T_Arg, typename T_Callable>
    অন্তঃরেখ স্থির শূন্য foreachImpl(Const_Correct_Ast_গ্রন্থি<T_Arg>* nodep, ধ্রুবক T_Callable& f,
                                   bool visit_পরবর্তী);

    template <typename T_Arg, bool N_Default, typename T_Callable>
    অন্তঃরেখ স্থির bool predicateImpl(Const_Correct_Ast_গ্রন্থি<T_Arg>* nodep, ধ্রুবক T_Callable& p,
                                     bool visit_পরবর্তী);

সর্বজনীন:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    template <typename T_Callable>
    শূন্য foreach(T_Callable&& f) {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_গ্রন্থি*>::মান
                          && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_গ্রন্থি>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    শূন্য foreach(T_Callable&& f) ধ্রুবক {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ধ্রুবক T_গ্রন্থি*>::মান
                && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ধ্রুবক T_গ্রন্থি>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    শূন্য foreach_And_পরবর্তী(T_Callable&& f) {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_গ্রন্থি*>::মান
                          && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_গ্রন্থি>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    শূন্য foreach_And_পরবর্তী(T_Callable&& f) ধ্রুবক {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ধ্রুবক T_গ্রন্থি*>::মান
                && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ধ্রুবক T_গ্রন্থি>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    template <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_গ্রন্থি*>::মান
                          && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ফেরত predicateImpl<T_গ্রন্থি, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool exists(T_Callable&& p) ধ্রুবক {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ধ্রুবক T_গ্রন্থি*>::মান
                          && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ফেরত predicateImpl<ধ্রুবক T_গ্রন্থি, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    bool exists_And_পরবর্তী(T_Callable&& p) {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ধ্রুবক T_গ্রন্থি*>::মান
                          && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ফেরত predicateImpl<T_গ্রন্থি, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool exists_And_পরবর্তী(T_Callable&& p) ধ্রুবক {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ধ্রুবক T_গ্রন্থি*>::মান
                          && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ফেরত predicateImpl<ধ্রুবক T_গ্রন্থি, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    template <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_গ্রন্থি*>::মান
                          && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ফেরত predicateImpl<T_গ্রন্থি, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool forall(T_Callable&& p) ধ্রুবক {
        using T_গ্রন্থি = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ধ্রুবক T_গ্রন্থি*>::মান
                          && std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ফেরত predicateImpl<ধ্রুবক T_গ্রন্থি, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    পূর্ণসংখ্যা গ্রন্থি_গুনতি() ধ্রুবক {
        // TODO: this should really return size_t, but need to fix use sites
        পূর্ণসংখ্যা গুনতি = 0;
        this->foreach([&গুনতি](ধ্রুবক Ast_গ্রন্থি*) { ++গুনতি; });
        ফেরত গুনতি;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(পূর্ণসংখ্যা limit) ধ্রুবক {
        পূর্ণসংখ্যা গুনতি = 0;
        ফেরত this->exists([&গুনতি, limit](ধ্রুবক Ast_গ্রন্থি*) { ফেরত ++গুনতি > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
template <>
Ast_গ্রন্থি* Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* newp);

// Inline method implementations
Ast_গ্রন্থি* Ast_গ্রন্থি::add_পরবর্তী(Ast_গ্রন্থি* newp) { ফেরত add_পরবর্তী(this, newp); }

// Specializations of privateTypeTest
#অন্তর্ভাব "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
template <>
অন্তঃরেখ bool Ast_গ্রন্থি::mayBeUnder<AstCell>(ধ্রুবক Ast_গ্রন্থি* nodep) {
    ফেরত !VN_IS(nodep, গ্রন্থি_Stmt) && !VN_IS(nodep, গ্রন্থি_রাশিমালা);
}
template <>
অন্তঃরেখ bool Ast_গ্রন্থি::mayBeUnder<Ast_গ্রন্থি_Assign>(ধ্রুবক Ast_গ্রন্থি* nodep) {
    ফেরত !VN_IS(nodep, গ্রন্থি_রাশিমালা);
}
template <>
অন্তঃরেখ bool Ast_গ্রন্থি::mayBeUnder<AstVarScope>(ধ্রুবক Ast_গ্রন্থি* nodep) {
    যদি (VN_IS(nodep, VarScope)) ফেরত false;  // Should not nest
    যদি (VN_IS(nodep, Var)) ফেরত false;
    যদি (VN_IS(nodep, Active)) ফেরত false;
    যদি (VN_IS(nodep, গ্রন্থি_Stmt)) ফেরত false;
    যদি (VN_IS(nodep, গ্রন্থি_রাশিমালা)) ফেরত false;
    ফেরত true;
}
template <>
অন্তঃরেখ bool Ast_গ্রন্থি::mayBeUnder<AstExecGraph>(ধ্রুবক Ast_গ্রন্থি* nodep) {
    যদি (VN_IS(nodep, ExecGraph)) ফেরত false;  // Should not nest
    যদি (VN_IS(nodep, গ্রন্থি_Stmt)) ফেরত false;  // Should be directly under CFunc
    ফেরত true;
}
template <>
অন্তঃরেখ bool Ast_গ্রন্থি::mayBeUnder<AstActive>(ধ্রুবক Ast_গ্রন্থি* nodep) {
    ফেরত !VN_IS(nodep, Active);  // AstActives do not nest
}
template <>
অন্তঃরেখ bool Ast_গ্রন্থি::mayBeUnder<AstScope>(ধ্রুবক Ast_গ্রন্থি* nodep) {
    ফেরত !VN_IS(nodep, Scope);  // AstScopes do not nest
}
template <>
অন্তঃরেখ bool Ast_গ্রন্থি::mayBeUnder<Ast_Sen_বৃক্ষ>(ধ্রুবক Ast_গ্রন্থি* nodep) {
    ফেরত !VN_IS(nodep, Sen_বৃক্ষ);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
template <>
constexpr bool Ast_গ্রন্থি::isLeaf<Ast_গ্রন্থি_Var_Ref>() {
    ফেরত true;
}
template <>
constexpr bool Ast_গ্রন্থি::isLeaf<AstVarRef>() {
    ফেরত true;
}
template <>
constexpr bool Ast_গ্রন্থি::isLeaf<AstVarXRef>() {
    ফেরত true;
}

// foreach implementation
template <typename T_Arg, typename T_Callable>
শূন্য Ast_গ্রন্থি::foreachImpl(Const_Correct_Ast_গ্রন্থি<T_Arg>* nodep, ধ্রুবক T_Callable& f, bool visit_পরবর্তী) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using গ্রন্থি_2 = Const_Correct_Ast_গ্রন্থি<T_Arg>;

    // We prefetch this far into the stack
    constexpr পূর্ণসংখ্যা PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr আকার_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr আকার_t Initial_স্তূপ_আকার = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    আকার_t স্তূপ_আকার = Initial_স্তূপ_আকার;
    গ্রন্থি_2* in_সারি_স্তূপ[Initial_স্তূপ_আকার];  // In-line initial stack storage
    std::unique_ptr<গ্রন্থি_2*[]> out_Of_সারি_Stackp{nullptr};  // In case we need more, we will allocate
    গ্রন্থি_2** basep = in_সারি_স্তূপ + PrefetchDistance;  // Pointer to base of stack
    গ্রন্থি_2** topp = basep;  // Pointer to top of stack
    গ্রন্থি_2** limp = in_সারি_স্তূপ + স্তূপ_আকার - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    জন্য (পূর্ণসংখ্যা i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ধ্রুবক auto visit = [&](গ্রন্থি_2* currp) {
        // Type test this node
        যদি (Ast_গ্রন্থি::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            যদি VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) ফেরত;
        }

        // Enqueue children for traversal, unless futile
        যদি (mayBeUnder<T_Arg_NonConst>(currp)) {
            যদি (Ast_গ্রন্থি* ধ্রুবক op4p = currp->op4p()) *topp++ = op4p;
            যদি (Ast_গ্রন্থি* ধ্রুবক op3p = currp->op3p()) *topp++ = op3p;
            যদি (Ast_গ্রন্থি* ধ্রুবক op2p = currp->op2p()) *topp++ = op2p;
            যদি (Ast_গ্রন্থি* ধ্রুবক op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    যদি (visit_পরবর্তী && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    যখন (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        গ্রন্থি_2* ধ্রুবক headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        যদি (VL_UNLIKELY(topp >= limp)) {
            ধ্রুবক ptrdiff_t occupancy = topp - basep;
            {
                ধ্রুবক আকার_t new_স্তূপ_আকার = স্তূপ_আকার * 2;
                গ্রন্থি_2** ধ্রুবক newStackp = new গ্রন্থি_2*[new_স্তূপ_আকার];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            আকার(গ্রন্থি_2**) * (occupancy + PrefetchDistance));
                স্তূপ_আকার = new_স্তূপ_আকার;
                out_Of_সারি_Stackp.reset(newStackp);
            }
            basep = out_Of_সারি_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_সারি_Stackp.get() + স্তূপ_আকার - MaxPushesPerIteration;
        }

        // Enqueue the next node
        যদি (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
template <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_গ্রন্থি::predicateImpl(Const_Correct_Ast_গ্রন্থি<T_Arg>* nodep, ধ্রুবক T_Callable& p,
                            bool visit_পরবর্তী) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using গ্রন্থি_2 = Const_Correct_Ast_গ্রন্থি<T_Arg>;

    // We prefetch this far into the stack
    constexpr পূর্ণসংখ্যা PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr আকার_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr আকার_t Initial_স্তূপ_আকার = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    আকার_t স্তূপ_আকার = Initial_স্তূপ_আকার;
    গ্রন্থি_2* in_সারি_স্তূপ[Initial_স্তূপ_আকার];  // In-line initial stack storage
    std::unique_ptr<গ্রন্থি_2*[]> out_Of_সারি_Stackp{nullptr};  // In case we need more, we will allocate
    গ্রন্থি_2** basep = in_সারি_স্তূপ + PrefetchDistance;  // Pointer to base of stack
    গ্রন্থি_2** topp = basep;  // Pointer to top of stack
    গ্রন্থি_2** limp = in_সারি_স্তূপ + স্তূপ_আকার - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    জন্য (পূর্ণসংখ্যা i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ধ্রুবক auto visit = [&](গ্রন্থি_2* currp) {
        // Type test this node
        যদি (Ast_গ্রন্থি::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            যদি (p(static_cast<T_Arg*>(currp)) != N_Default) ফেরত true;
            // Short circuit if iterating leaf nodes
            যদি VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) ফেরত false;
        }

        // Enqueue children for traversal, unless futile
        যদি (mayBeUnder<T_Arg_NonConst>(currp)) {
            যদি (Ast_গ্রন্থি* ধ্রুবক op4p = currp->op4p()) *topp++ = op4p;
            যদি (Ast_গ্রন্থি* ধ্রুবক op3p = currp->op3p()) *topp++ = op3p;
            যদি (Ast_গ্রন্থি* ধ্রুবক op2p = currp->op2p()) *topp++ = op2p;
            যদি (Ast_গ্রন্থি* ধ্রুবক op1p = currp->op1p()) *topp++ = op1p;
        }

        ফেরত false;
    };

    // Enqueue the next of the root node, if required
    যদি (visit_পরবর্তী && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    যদি (visit(nodep)) ফেরত !N_Default;

    // Visit the rest of the tree
    যখন (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        গ্রন্থি_2* ধ্রুবক headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        যদি (VL_UNLIKELY(topp >= limp)) {
            ধ্রুবক ptrdiff_t occupancy = topp - basep;
            {
                ধ্রুবক আকার_t new_স্তূপ_আকার = স্তূপ_আকার * 2;
                গ্রন্থি_2** ধ্রুবক newStackp = new গ্রন্থি_2*[new_স্তূপ_আকার];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            আকার(গ্রন্থি_2**) * (occupancy + PrefetchDistance));
                স্তূপ_আকার = new_স্তূপ_আকার;
                out_Of_সারি_Stackp.reset(newStackp);
            }
            basep = out_Of_সারি_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_সারি_Stackp.get() + স্তূপ_আকার - MaxPushesPerIteration;
        }

        // Enqueue the next node
        যদি (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        যদি (visit(headp)) ফেরত !N_Default;
    }

    ফেরত N_Default;
}

অন্তঃরেখ std::ostream& operator<<(std::ostream& os, ধ্রুবক Ast_গ্রন্থি* rhs) {
    যদি (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } অন্যথা যদি (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } অন্যথা {
        rhs->dump(os);
    }
    ফেরত os;
}
শূন্য VNRelinker::relink(Ast_গ্রন্থি* newp) { newp->Ast_গ্রন্থি::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
template <typename T_গ্রন্থি>
class VNRef final : সর্বজনীন std::reference_wrapper<T_গ্রন্থি> {
    static_assert(std::is_base_of<Ast_গ্রন্থি, T_গ্রন্থি>::মান,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

সর্বজনীন:
    template <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_গ্রন্থি>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(ধ্রুবক std::reference_wrapper<T_গ্রন্থি>& other)
        : std::reference_wrapper<T_গ্রন্থি>{other} {}
};

static_assert(আকার(VNRef<Ast_গ্রন্থি>) == আকার(std::reference_wrapper<Ast_গ্রন্থি>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
আকার_t V3HasherUncachedHash(ধ্রুবক Ast_গ্রন্থি&);

// Specialization of std::hash for VNRef
template <typename T_গ্রন্থি>
কাঠামো std::hash<VNRef<T_গ্রন্থি>> final {
    আকার_t operator()(VNRef<T_গ্রন্থি> r) ধ্রুবক { ফেরত V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
template <typename T_গ্রন্থি>
কাঠামো std::equal_to<VNRef<T_গ্রন্থি>> final {
    আকার_t operator()(VNRef<T_গ্রন্থি> ra, VNRef<T_গ্রন্থি> rb) ধ্রুবক {
        ফেরত ra.get().same_বৃক্ষ(&(rb.get()));
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
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি*, Ast_গ্রন্থি*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<ধ্রুবক Ast_গ্রন্থি*, std::vector<Ast_গ্রন্থি*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<ধ্রুবক Ast_গ্রন্থি*, std::string> m_prefixes;
    ধ্রুবক VDTypeNameScopes* ধ্রুবক m_prevp;  // Index in use before this one
    স্থির ধ্রুবক VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    শূন্য সূচক(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

সর্বজনীন:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    স্থির ধ্রুবক VDTypeNameScopes* currentp() { ফেরত s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(ধ্রুবক Ast_গ্রন্থি* nodep, Ast_গ্রন্থি*& outerpr) ধ্রুবক;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ধ্রুবক std::vector<Ast_গ্রন্থি*>* paramsp(ধ্রুবক Ast_গ্রন্থি* modp) ধ্রুবক;
    // The prefix kept for scope 'scopep', or nullptr if none
    ধ্রুবক std::string* prefixp(ধ্রুবক Ast_গ্রন্থি* scopep) ধ্রুবক;
    // Keep 'prefix', final, for scope 'scopep'
    শূন্য prefix(ধ্রুবক Ast_গ্রন্থি* scopep, ধ্রুবক std::string& prefix) ধ্রুবক;
};

//######################################################################
// Inline VNVisitor METHODS

শূন্য VNVisitorConst::iterateConst(Ast_গ্রন্থি* nodep) { nodep->accept(*this); }
শূন্য VNVisitorConst::iterateConstNull(Ast_গ্রন্থি* nodep) {
    যদি (VL_LIKELY(nodep)) nodep->accept(*this);
}
শূন্য VNVisitorConst::iterateChildrenConst(Ast_গ্রন্থি* nodep) { nodep->iterateChildrenConst(*this); }
শূন্য VNVisitorConst::iterateChildrenBackwardsConst(Ast_গ্রন্থি* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
শূন্য VNVisitorConst::iterate_And_পরবর্তী_Const_Null(Ast_গ্রন্থি* nodep) {
    যদি (VL_LIKELY(nodep)) nodep->iterate_And_পরবর্তী_Const(*this);
}

শূন্য VNVisitor::iterate(Ast_গ্রন্থি* nodep) { nodep->accept(*this); }
শূন্য VNVisitor::iterateNull(Ast_গ্রন্থি* nodep) {
    যদি (VL_LIKELY(nodep)) nodep->accept(*this);
}
শূন্য VNVisitor::iterateChildren(Ast_গ্রন্থি* nodep) { nodep->iterateChildren(*this); }
শূন্য VNVisitor::iterate_And_পরবর্তী_Null(Ast_গ্রন্থি* nodep) {
    যদি (VL_LIKELY(nodep)) nodep->iterate_And_পরবর্তী(*this);
}
Ast_গ্রন্থি* VNVisitor::iterateSubtreeReturnEdits(Ast_গ্রন্থি* nodep) {
    ফেরত nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#অন্তর্ভাব "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#অন্তর্ভাব "V3AstNodeDType.h"
#অন্তর্ভাব "V3AstNodeOther.h"
#অন্তর্ভাব "V3AstNodeExpr.h"
#অন্তর্ভাব "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#অন্তর্ভাব "V3AstInlines.h"
শূন্য dump_গ্রন্থি_List_Json(std::ostream& os, ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক std::string& listName,
                      ধ্রুবক string& indent);

#endif  // Guard
