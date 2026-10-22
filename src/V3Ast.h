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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Broken.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3FunctionTraits.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Number.h"
#அந்தர்பாவ "V3StdFuture.h"

#அந்தர்பாவ "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#அந்தர்பாவ <array>
#அந்தர்பாவ <cmath>
#அந்தர்பாவ <cstdint>
#அந்தர்பாவ <functional>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <type_traits>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#எனில் defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#undef VL_USER_TYPE_CHECKS
#endif
#ifdef VL_USER_TYPE_CHECKS
#அந்தர்பாவ <variant>
#endif

// clang-format off
#அந்தர்பாவ "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3Graph;
class ExecMTask;
class VIfaceCaptureTag;

//######################################################################

// For broken() function, return error string if have a match
#define BROKEN_RTN(test) \
    செய் { \
        எனில் (VL_UNCOVERABLE(test)) திரும்பு "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } போது (false)
// For broken() function, return error string if a base of this class has a match
#define BROKEN_BASE_RTN(test) \
    செய் { \
        மாறிலி எழுத்து* மாறிலி reasonp = (test); \
        எனில் (VL_UNCOVERABLE(reasonp)) திரும்பு reasonp; \
    } போது (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#define VN_IS(nodep, nodetypename) (Ast_முனை::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#define VN_CAST(nodep, nodetypename) (Ast_முனை::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#define VN_AS(nodep, nodetypename) (Ast_முனை::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#எனில் defined(VL_DEBUG) || defined(VL_ASAN)
#define VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#இல்லையெனில்
#define VN_DBG_AS(nodep, nodetypename) (Ast_முனை::unsafePrivateAs<Ast##nodetypename>(nodep))
#endif

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#define VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

கட்டமைப்பு VNTypeInfo final {
    மாறிலி எழுத்து* m_namep;
    எண்ணிக்கை OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    மாறிலி எழுத்து* m_opNamep[4];
    size_t m_sizeof;
};

class VNType final {
    நிலையான மாறிலி VNTypeInfo s_typeInfoTable[];

பொது:
#அந்தர்பாவ "V3Ast__gen_type_enum.h"  // From ./astgen
    மாறிலி en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr operator en() மாறிலி VL_MT_SAFE { திரும்பு m_e; }

    // Retrieve VNTypeInfo for the given type
    நிலையான மாறிலி VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { திரும்பு s_typeInfoTable[t.m_e]; }
};
constexpr bool operator==(மாறிலி VNType& lhs, மாறிலி VNType& rhs) VL_PURE {
    திரும்பு lhs.m_e == rhs.m_e;
}
constexpr bool operator==(மாறிலி VNType& lhs, VNType::en rhs) VL_PURE { திரும்பு lhs.m_e == rhs; }
constexpr bool operator==(VNType::en lhs, மாறிலி VNType& rhs) VL_PURE { திரும்பு lhs == rhs.m_e; }
உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி VNType& rhs) { திரும்பு os << rhs.ascii(); }

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
    std::variant<std::monostate, uint64_t, வெறுமை*> m_u;
#இல்லையெனில்
    ஒன்றிப்பு {
        வெறுமை* up;
        uint64_t uq;
    } m_u;
#endif

பொது:
#ifdef VL_USER_TYPE_CHECKS
    VNUser() = இயல்பு;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        எனில் (i) m_u = i;
    }
    explicit VNUser(வெறுமை* p) {
        // VNUser{nullptr} represents the monostate
        எனில் (p) m_u = p;
    }
    ~VNUser() = இயல்பு;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::மதிப்பு, T>::type to() மாறிலி VL_MT_SAFE {
        எனில் (std::holds_alternative<std::monostate>(m_u)) திரும்பு nullptr;
        வெறுமை* மாறிலி* மாறிலி upp = std::get_if<வெறுமை*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        திரும்பு reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() மாறிலி {
        எனில் (std::holds_alternative<std::monostate>(m_u)) திரும்பு 0;
        மாறிலி uint64_t* மாறிலி uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        திரும்பு *uip;
    }
#இல்லையெனில்
    VNUser() = இயல்பு;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.up = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(வெறுமை* p) { m_u.up = p; }
    ~VNUser() = இயல்பு;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::மதிப்பு, T>::type to() மாறிலி VL_MT_SAFE {
        திரும்பு reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() மாறிலி { திரும்பு m_u.uq; }
#endif
    VSymEnt* toSymEnt() மாறிலி { திரும்பு to<VSymEnt*>(); }
    Ast_முனை* toNodep() மாறிலி VL_MT_SAFE { திரும்பு to<Ast_முனை*>(); }
    V3GraphVertex* toGraphVertex() மாறிலி { திரும்பு to<V3GraphVertex*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(மாறிலி வெறுமை*)) மாறிலி;
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
VL_NOT_FINAL{பாதுகாக்கப்பட்ட : நிலையான வெறுமை allocate(முழு id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvtToStr(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
நிலையான வெறுமை விடுவி(முழு id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvtToStr(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
நிலையான வெறுமை clearcnt(முழு id, uint32_t& cntGblRef, மாறிலி bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
நிலையான வெறுமை checkcnt(முழு id, uint32_t&, மாறிலி bool& userBusyRef) {
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
பாதுகாக்கப்பட்ட:
    friend class Ast_முனை;
    நிலையான uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    நிலையான bool s_userBusy;  // Count is in use
பொது:
    VNUser1InUse()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1InUse()     { விடுவி    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    நிலையான வெறுமை clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    நிலையான வெறுமை check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2InUse final : VNUserInUseBase {
பாதுகாக்கப்பட்ட:
    friend class Ast_முனை;
    நிலையான uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    நிலையான bool s_userBusy;  // Count is in use
பொது:
    VNUser2InUse()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2InUse()     { விடுவி    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    நிலையான வெறுமை clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    நிலையான வெறுமை check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3InUse final : VNUserInUseBase {
பாதுகாக்கப்பட்ட:
    friend class Ast_முனை;
    நிலையான uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    நிலையான bool s_userBusy;  // Count is in use
பொது:
    VNUser3InUse()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3InUse()     { விடுவி    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    நிலையான வெறுமை clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    நிலையான வெறுமை check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4InUse final : VNUserInUseBase {
பாதுகாக்கப்பட்ட:
    friend class Ast_முனை;
    நிலையான uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    நிலையான bool s_userBusy;  // Count is in use
பொது:
    VNUser4InUse()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4InUse()     { விடுவி    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    நிலையான வெறுமை clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    நிலையான வெறுமை check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::vector<Ast_முனை*> m_deleteps;  // Nodes to delete

பொது:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    வெறுமை தள்ளு_Deletep(Ast_முனை* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.தள்ளு_back(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    வெறுமை doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    friend class Ast_முனை;

பொது:
    /// Call visit()s on nodep
    உள்வரி வெறுமை iterateConst(Ast_முனை* nodep);
    /// Call visit()s on nodep
    உள்வரி வெறுமை iterateConstNull(Ast_முனை* nodep);
    /// Call visit()s on const nodep's children
    உள்வரி வெறுமை iterateChildrenConst(Ast_முனை* nodep);
    /// Call visit()s on nodep's children in backp() order
    உள்வரி வெறுமை iterateChildrenBackwardsConst(Ast_முனை* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    உள்வரி வெறுமை iterateAndNextConstNull(Ast_முனை* nodep);

    மெய்நிகர் வெறுமை visit(Ast_முனை* nodep) = 0;
    மெய்நிகர் ~VNVisitorConst() {}
#அந்தர்பாவ "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : பொது VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
பொது:
    /// Call visit()s on nodep
    உள்வரி வெறுமை iterate(Ast_முனை* nodep);
    /// Call visit()s on nodep
    உள்வரி வெறுமை iterateNull(Ast_முனை* nodep);
    /// Call visit()s on nodep's children
    உள்வரி வெறுமை iterateChildren(Ast_முனை* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    உள்வரி வெறுமை iterateAndNextNull(Ast_முனை* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    உள்வரி Ast_முனை* iterateSubtreeReturnEdits(Ast_முனை* nodep);

    VNDeleter& deleter() { திரும்பு m_deleter; }
    வெறுமை தள்ளு_Deletep(Ast_முனை* nodep) { deleter().தள்ளு_Deletep(nodep); }
    வெறுமை doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
பாதுகாக்கப்பட்ட:
    friend class Ast_முனை;
    எண்ணிக்கை RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_NEXT,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_முனை* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_முனை* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_முனை** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

பொது:
    VNRelinker() = இயல்பு;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    உள்வரி வெறுமை relink(Ast_முனை* newp);
    வெறுமை dump(std::ostream& str = std::cout) மாறிலி;
};
உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி VNRelinker& rhs) {
    rhs.dump(os);
    திரும்பு os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{பொது : மெய்நிகர் bool முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி {திரும்பு true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#define ASTNODE_PREFETCH_NON_NULL(nodep) \
    செய் { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } போது (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#define ASTNODE_PREFETCH(nodep) \
    செய் { \
        எனில் (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } போது (false)

class Ast_முனை VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_முனை* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_முனை* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_முனை* m_op1p = nullptr;  // Generic pointer 1
    Ast_முனை* m_op2p = nullptr;  // Generic pointer 2
    Ast_முனை* m_op3p = nullptr;  // Generic pointer 3
    Ast_முனை* m_op4p = nullptr;  // Generic pointer 4
    Ast_முனை** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    மாறிலி VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    கட்டமைப்பு {
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

    முழு m_cloneCnt = 0;  // Sequence number for when last clone was made

#எனில் defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(அளவு(m_type) + அளவு(m_flags) + அளவு(m_brokenState) + அளவு(m_cloneCnt)
                      <= அளவு(வெறுமை*),
                  "packing requires padding");
#endif

    Ast_முனை_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_முனை* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_வரி* m_fileline;  // Where it was declared
#ifdef VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_மொத்தம்;  // When it was last edited
#endif
    நிலையான uint64_t s_editCntGbl;  // Global edit counter
    நிலையான uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_முனை* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    நிலையான முழு s_cloneCntGbl;  // Count of which userp is set

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
    வெறுமை op1p(Ast_முனை* nodep) {
        m_op1p = nodep;
        எனில் (nodep) nodep->m_backp = this;
    }
    வெறுமை op2p(Ast_முனை* nodep) {
        m_op2p = nodep;
        எனில் (nodep) nodep->m_backp = this;
    }
    வெறுமை op3p(Ast_முனை* nodep) {
        m_op3p = nodep;
        எனில் (nodep) nodep->m_backp = this;
    }
    வெறுமை op4p(Ast_முனை* nodep) {
        m_op4p = nodep;
        எனில் (nodep) nodep->m_backp = this;
    }

தனிப்பட்ட:
    Ast_முனை* clone_மரம்_Iter(bool needPure);
    Ast_முனை* clone_மரம்_Iter_List(bool needPure);
    வெறுமை check_மரம்_Iter(மாறிலி Ast_முனை* prevBackp) மாறிலி VL_MT_STABLE;
    bool gate_மரம்_Iter() மாறிலி;
    நிலையான bool same_மரம்_Iter(மாறிலி Ast_முனை* node1p, மாறிலி Ast_முனை* node2p, bool ignNext,
                             bool gateOnly);
    வெறுமை delete_மரம்_Iter();
    வெறுமை delete_முனை();
    string instanceStr() மாறிலி;

பொது:
    வெறுமை purityCheck();
    நிலையான வெறுமை relinkOneLink(Ast_முனை*& pointpr, Ast_முனை* newp);
    // cppcheck-suppress functionConst
    நிலையான வெறுமை debug_மரம்_Change(மாறிலி Ast_முனை* nodep, மாறிலி எழுத்து* prefix, முழு lineno, bool next);

பாதுகாக்கப்பட்ட:
    // CONSTRUCTORS
    Ast_முனை(VNType t, File_வரி* fl);
    மெய்நிகர் ~Ast_முனை() = இயல்பு;  // Use 'deleteTree' instead
    மெய்நிகர் Ast_முனை* clone() = 0;  // Generally, cloneTree is what you want instead
    மெய்நிகர் வெறுமை cloneRelink() { cloneRelinkGen(); }
    மெய்நிகர் வெறுமை cloneRelinkGen() {};  // Overrides generated by 'astgen'
    வெறுமை clone_Relink_மரம்();

    // METHODS
    வெறுமை setOp1p(Ast_முனை* newp);  // Set non-list-type op1 to non-list element
    வெறுமை setOp2p(Ast_முனை* newp);  // Set non-list-type op2 to non-list element
    வெறுமை setOp3p(Ast_முனை* newp);  // Set non-list-type op3 to non-list element
    வெறுமை setOp4p(Ast_முனை* newp);  // Set non-list-type op4 to non-list element

    வெறுமை addOp1p(Ast_முனை* newp);  // Append newp to end of op1
    வெறுமை addOp2p(Ast_முனை* newp);  // Append newp to end of op2
    வெறுமை addOp3p(Ast_முனை* newp);  // Append newp to end of op3
    வெறுமை addOp4p(Ast_முனை* newp);  // Append newp to end of op4

    வெறுமை prependOp1p(Ast_முனை* newp);  // Prepend newp to front of op1
    வெறுமை prependOp2p(Ast_முனை* newp);  // Prepend newp to front of op2
    வெறுமை prependOp3p(Ast_முனை* newp);  // Prepend newp to front of op3
    வெறுமை prependOp4p(Ast_முனை* newp);  // Prepend newp to front of op4

    // clang-format off
    வெறுமை setNOp1p(Ast_முனை* newp) { எனில் (newp) setOp1p(newp); }
    வெறுமை setNOp2p(Ast_முனை* newp) { எனில் (newp) setOp2p(newp); }
    வெறுமை setNOp3p(Ast_முனை* newp) { எனில் (newp) setOp3p(newp); }
    வெறுமை setNOp4p(Ast_முனை* newp) { எனில் (newp) setOp4p(newp); }

    வெறுமை addNOp1p(Ast_முனை* newp) { எனில் (newp) addOp1p(newp); }
    வெறுமை addNOp2p(Ast_முனை* newp) { எனில் (newp) addOp2p(newp); }
    வெறுமை addNOp3p(Ast_முனை* newp) { எனில் (newp) addOp3p(newp); }
    வெறுமை addNOp4p(Ast_முனை* newp) { எனில் (newp) addOp4p(newp); }

    வெறுமை prependNOp1p(Ast_முனை* newp) { எனில் (newp) prependOp1p(newp); }
    வெறுமை prependNOp2p(Ast_முனை* newp) { எனில் (newp) prependOp2p(newp); }
    வெறுமை prependNOp3p(Ast_முனை* newp) { எனில் (newp) prependOp3p(newp); }
    வெறுமை prependNOp4p(Ast_முனை* newp) { எனில் (newp) prependOp4p(newp); }
    // clang-format on

    வெறுமை clonep(Ast_முனை* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    நிலையான வெறுமை clone_Clear_மரம்() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    மெய்நிகர் bool same_முனை(மாறிலி Ast_முனை*) மாறிலி { திரும்பு true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    மெய்நிகர் bool wouldBreakGen(மாறிலி Ast_முனை* மாறிலி oldp,
                               மாறிலி Ast_முனை* மாறிலி newp) மாறிலி
        = 0;  // Generated by 'astgen'

பொது:
    // ACCESSORS
    VNType type() மாறிலி VL_MT_SAFE { திரும்பு m_type; }
    மாறிலி எழுத்து* typeName() மாறிலி VL_MT_SAFE { திரும்பு type().ascii(); }  // See also prettyTypeName
    Ast_முனை* nextp() மாறிலி VL_MT_STABLE { திரும்பு m_nextp; }
    Ast_முனை* backp() மாறிலி VL_MT_STABLE { திரும்பு m_backp; }
    Ast_முனை* abovep() மாறிலி;  // Get parent node above, only for list head and tail
    Ast_முனை* aboveLoopp() மாறிலி;  // Get parent node above, may have performance issues as loops
    Ast_முனை* lastp() மாறிலி {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        திரும்பு m_headtailp;
    }
    Ast_முனை* prevp() மாறிலி {  // Previous node in list, nullptr for head of list
        எனில் (m_backp->m_nextp != this) திரும்பு nullptr;
        திரும்பு m_backp;
    }
    Ast_முனை* op1p() மாறிலி VL_MT_STABLE { திரும்பு m_op1p; }
    Ast_முனை* op2p() மாறிலி VL_MT_STABLE { திரும்பு m_op2p; }
    Ast_முனை* op3p() மாறிலி VL_MT_STABLE { திரும்பு m_op3p; }
    Ast_முனை* op4p() மாறிலி VL_MT_STABLE { திரும்பு m_op4p; }
    Ast_முனை_DType* dtypep() மாறிலி VL_MT_STABLE { திரும்பு m_dtypep; }
    Ast_முனை* clonep() மாறிலி { திரும்பு ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_முனை* firstAbovep() மாறிலி {  // Returns nullptr when second or later in list
        திரும்பு ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() மாறிலி VL_MT_SAFE { திரும்பு m_brokenState; }
    வெறுமை brokenState(uint8_t மதிப்பு) { m_brokenState = மதிப்பு; }

    // Used by AstNode::broken()
    bool brokeExists() மாறிலி { திரும்பு V3Broken::isLinkable(this); }
    bool brokeExistsAbove() மாறிலி { திரும்பு brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    மெய்நிகர் வெறுமை foreachLink(std::function<வெறுமை(Ast_முனை** linkpp, மாறிலி எழுத்து* namep)> f) = 0;

#எனில் defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    நிலையான வெறுமை* operator new(size_t size);
    நிலையான வெறுமை operator delete(வெறுமை* obj, size_t size);
#endif

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    நிலையான constexpr முழு INSTR_மொத்தம்_BRANCH = 4;  // Branch
    நிலையான constexpr முழு INSTR_மொத்தம்_CALL = INSTR_மொத்தம்_BRANCH + 10;  // Subroutine call
    நிலையான constexpr முழு INSTR_மொத்தம்_LD = 2;  // Load memory
    நிலையான constexpr முழு INSTR_மொத்தம்_INT_MUL = 3;  // Integer multiply
    நிலையான constexpr முழு INSTR_மொத்தம்_INT_DIV = 10;  // Integer divide
    நிலையான constexpr முழு INSTR_மொத்தம்_DBL = 8;  // Convert or do float ops
    நிலையான constexpr முழு INSTR_மொத்தம்_DBL_DIV = 40;  // Double divide
    நிலையான constexpr முழு INSTR_மொத்தம்_DBL_TRIG = 200;  // Double trigonometric ops
    நிலையான constexpr முழு INSTR_மொத்தம்_STR = 100;  // String ops
    நிலையான constexpr முழு INSTR_மொத்தம்_TIME = INSTR_மொத்தம்_CALL + 5;  // Determine simulation time
    நிலையான constexpr முழு INSTR_மொத்தம்_PLI = 20;  // PLI routines
    நிலையான constexpr முழு INSTR_மொத்தம்_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    மெய்நிகர் string name() மாறிலி VL_MT_STABLE { திரும்பு ""; }
    மெய்நிகர் string origName() மாறிலி { திரும்பு ""; }
    string prettyDehashOrigOrName() மாறிலி {
        திரும்பு prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    மெய்நிகர் வெறுமை name(மாறிலி string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    மெய்நிகர் வெறுமை tag(மாறிலி string& text) {}
    மெய்நிகர் string tag() மாறிலி { திரும்பு ""; }
    மெய்நிகர் uint32_t decl_Token_இலக்கம்() மாறிலி { திரும்பு 0; }
    மெய்நிகர் வெறுமை decl_Token_இலக்கம்_Set_Min(uint32_t token_இலக்கம்) {}
    மெய்நிகர் string verilogKwd() மாறிலி { திரும்பு ""; }
    string nameProtect() மாறிலி VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() மாறிலி;  // origName with --protect-id applied
    string shortName() மாறிலி;  // Name with __PVT__ removed for concatenating scopes
    நிலையான string dedotName(மாறிலி string& namein);  // Name with dots removed
    நிலையான string nameNoArray(மாறிலி string& namein);  // Name with any array index removed
    நிலையான string prettyName(மாறிலி string& namein) VL_PURE;  // Name for printing out to the user
    நிலையான string vpiName(மாறிலி string& namein);  // Name for vpi access
    நிலையான string prettyNameQ(மாறிலி string& namein) {  // Quoted pretty name (for errors)
        திரும்பு "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    நிலையான string encodeName(மாறிலி string& namein);
    நிலையான string encode_எண்(int64_t இலக்கம்);  // Encode number into internal C representation
    நிலையான string vcdName(மாறிலி string& namein);  // Name for printing out to vcd files
    string prettyName() மாறிலி { திரும்பு prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    மெய்நிகர் string pretty_Name_செய்தி() மாறிலி { திரும்பு prettyName(); }
    string prettyNameQ() மாறிலி { திரும்பு "'"s + pretty_Name_செய்தி() + "'"; }  // Quoted, for messages
    string verilogName() மாறிலி { திரும்பு vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() மாறிலி;
    மெய்நிகர் string prettyOperatorName() மாறிலி { திரும்பு "operator " + prettyTypeName(); }
    File_வரி* fileline() மாறிலி VL_MT_SAFE { திரும்பு m_fileline; }
    வெறுமை fileline(File_வரி* fl) { m_fileline = fl; }
    உள்வரி bool width1() மாறிலி;
    உள்வரி முழு widthInstrs() மாறிலி;
    bool didWidth() மாறிலி { திரும்பு m_flags.didWidth; }
    வெறுமை didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        எனில் (didWidth()) திரும்பு true;
        didWidth(true);
        திரும்பு false;
    }
    bool doingWidth() மாறிலி { திரும்பு m_flags.doingWidth; }
    வெறுமை doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() மாறிலி VL_MT_SAFE { திரும்பு m_flags.protect; }
    வெறுமை protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    உள்வரி முழு width() மாறிலி VL_MT_STABLE;
    உள்வரி முழு widthMin() மாறிலி VL_MT_STABLE;
    முழு widthMinV() மாறிலி {
        திரும்பு v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    முழு width_சொற்கள்() மாறிலி { திரும்பு VL_சொற்கள்_I(width()); }
    bool isQuad() மாறிலி VL_MT_STABLE { திரும்பு (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() மாறிலி VL_MT_STABLE { திரும்பு (width() > VL_QUADSIZE); }
    உள்வரி bool isCHandle() மாறிலி VL_MT_STABLE;
    உள்வரி bool isDouble() மாறிலி VL_MT_STABLE;
    உள்வரி bool isSigned() மாறிலி VL_MT_STABLE;
    உள்வரி bool isString() மாறிலி VL_MT_STABLE;
    உள்வரி bool isEvent() மாறிலி VL_MT_STABLE;

    // clang-format off
    VNUser user1u() மாறிலி VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        திரும்பு ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_முனை* user1p() மாறிலி VL_MT_STABLE { திரும்பு user1u().toNodep(); }
    வெறுமை user1u(மாறிலி VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    வெறுமை user1p(வெறுமை* userp) { user1u(VNUser{userp}); }
    வெறுமை user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() மாறிலி { திரும்பு user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { மாறிலி uint64_t v = user1(); user1(v + val); திரும்பு v; }
    uint64_t user1Or(uint64_t val) { மாறிலி uint64_t v = user1(); user1(v | val); திரும்பு v; }
    uint64_t user1SetOnce() { மாறிலி uint64_t v = user1(); எனில் (!v) user1(1); திரும்பு v; }  // Better for cache than user1Inc()
    நிலையான வெறுமை user1_Clear_மரம்() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() மாறிலி VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        திரும்பு ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_முனை* user2p() மாறிலி VL_MT_STABLE { திரும்பு user2u().toNodep(); }
    வெறுமை user2u(மாறிலி VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    வெறுமை user2p(வெறுமை* userp) { user2u(VNUser{userp}); }
    வெறுமை user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() மாறிலி { திரும்பு user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { மாறிலி uint64_t v = user2(); user2(v + val); திரும்பு v; }
    uint64_t user2Or(uint64_t val) { மாறிலி uint64_t v = user2(); user2(v | val); திரும்பு v; }
    uint64_t user2SetOnce() { மாறிலி uint64_t v = user2(); எனில் (!v) user2(1); திரும்பு v; }  // Better for cache than user2Inc()
    நிலையான வெறுமை user2_Clear_மரம்() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() மாறிலி VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        திரும்பு ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_முனை* user3p() மாறிலி VL_MT_STABLE { திரும்பு user3u().toNodep(); }
    வெறுமை user3u(மாறிலி VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    வெறுமை user3p(வெறுமை* userp) { user3u(VNUser{userp}); }
    வெறுமை user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() மாறிலி { திரும்பு user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { மாறிலி uint64_t v = user3(); user3(v + val); திரும்பு v; }
    uint64_t user3Or(uint64_t val) { மாறிலி uint64_t v = user3(); user3(v | val); திரும்பு v; }
    uint64_t user3SetOnce() { மாறிலி uint64_t v = user3(); எனில் (!v) user3(1); திரும்பு v; }  // Better for cache than user3Inc()
    நிலையான வெறுமை user3_Clear_மரம்() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() மாறிலி VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        திரும்பு ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_முனை* user4p() மாறிலி VL_MT_STABLE { திரும்பு user4u().toNodep(); }
    வெறுமை user4u(மாறிலி VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    வெறுமை user4p(வெறுமை* userp) { user4u(VNUser{userp}); }
    வெறுமை user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() மாறிலி { திரும்பு user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { மாறிலி uint64_t v = user4(); user4(v | val); திரும்பு v; }
    uint64_t user4Inc(uint64_t val = 1) { மாறிலி uint64_t v = user4(); user4(v + val); திரும்பு v; }
    uint64_t user4SetOnce() { மாறிலி uint64_t v = user4(); எனில் (!v) user4(1); திரும்பு v; }  // Better for cache than user4Inc()
    நிலையான வெறுமை user4_Clear_மரம்() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ifdef VL_DEBUG
    uint64_t edit_மொத்தம்() மாறிலி { திரும்பு m_edit_மொத்தம்; }
    வெறுமை edit_மொத்தம்_Inc() {
        m_edit_மொத்தம் = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_மரம்();  // Any edit clears all caching
    }
#இல்லையெனில்
    வெறுமை edit_மொத்தம்_Inc() { ++s_editCntGbl; }
#endif
    நிலையான uint64_t edit_மொத்தம்_Last() VL_MT_SAFE { திரும்பு s_editCntLast; }
    நிலையான uint64_t edit_மொத்தம்_Gbl() VL_MT_SAFE { திரும்பு s_editCntGbl; }
    நிலையான வெறுமை edit_மொத்தம்_Set_Last() { s_editCntLast = edit_மொத்தம்_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_தள்ளு_Self_Stmt();
    உள்வரி bool is_Class_Handle_மதிப்பு() மாறிலி;
    உள்வரி bool isNull() மாறிலி;
    உள்வரி bool isZero() மாறிலி;
    உள்வரி bool isOne() மாறிலி;
    உள்வரி bool isNeqZero() மாறிலி;
    உள்வரி bool isAllOnes() மாறிலி;
    உள்வரி bool isAllOnesV() மாறிலி;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    வெறுமை dtypep(Ast_முனை_DType* nodep) {
        எனில் (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_மொத்தம்_Inc();
        }
    }
    வெறுமை dtypeFrom(மாறிலி Ast_முனை* fromp) {
        எனில் (fromp) dtypep(fromp->dtypep());
    }
    வெறுமை dtypeChgSigned(bool flag = true);
    வெறுமை dtypeChgWidth(முழு width, முழு widthMin);
    வெறுமை dtypeChgWidthSigned(முழு width, முழு widthMin, VSigning numeric);
    வெறுமை dtypeSetBitUnsized(முழு width, முழு widthMin, VSigning numeric) {
        dtypep(findBitDType(width, widthMin, numeric));
    }
    வெறுமை dtypeSetBitSized(முழு width, VSigning numeric) {
        dtypep(findBitDType(width, width, numeric));  // Since sized, widthMin is width
    }
    வெறுமை dtypeSetLogicUnsized(முழு width, முழு widthMin, VSigning numeric) {
        dtypep(findLogicDType(width, widthMin, numeric));
    }
    வெறுமை dtypeSetLogicSized(முழு width, VSigning numeric) {
        dtypep(findLogicDType(width, width, numeric));  // Since sized, widthMin is width
    }
    வெறுமை dtypeSetLogic2StateUnsized(முழு width, முழு widthMin, VSigning numeric) {
        dtypep(findLogic2StateDType(width, widthMin, numeric));
    }
    வெறுமை dtypeSetLogic2StateSized(முழு width, VSigning numeric) {
        dtypep(findLogic2StateDType(width, width, numeric));  // Since sized, widthMin is width
    }
    வெறுமை dtypeSetBitOrLogicUnsized(முழு width, முழு widthMin, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, widthMin, numeric, isFourstate));
    }
    வெறுமை dtypeSetBitOrLogicSized(முழு width, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, width, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    வெறுமை dtypeSetBit() { dtypep(findBitDType()); }
    வெறுமை dtypeSetLogic() { dtypep(findLogicDType()); }
    வெறுமை dtypeSetLogic2State() { dtypep(findLogic2StateDType()); }
    வெறுமை dtypeSetDouble() { dtypep(findDoubleDType()); }
    வெறுமை dtypeSetInt() { dtypep(findIntDType()); }
    வெறுமை dtypeSetInteger() { dtypep(findIntegerDType()); }
    வெறுமை dtypeSetInteger2State() { dtypep(findInteger2StateDType()); }
    வெறுமை dtypeSetString() { dtypep(findStringDType()); }
    வெறுமை dtypeSetUInt32() { dtypep(findUInt32DType()); }  // Twostate
    வெறுமை dtypeSetUInt64() { dtypep(findUInt64DType()); }  // Twostate
    வெறுமை dtypeSetTime() { dtypep(findTimeDType()); }
    வெறுமை dtypeSetTime2State() { dtypep(findTime2StateDType()); }
    வெறுமை dtypeSetEmptyQueue() { dtypep(findEmptyQueueDType()); }
    வெறுமை dtypeSetStream() { dtypep(findStreamDType()); }
    வெறுமை dtypeSetVoid() { dtypep(findVoidDType()); }
    வெறுமை dtypeSetBitOrLogic(bool isFourstate) {
        dtypep(isFourstate ? findLogicDType() : findBitDType());
    }

    // Data type locators
    Ast_முனை_DType* findBitDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_முனை_DType* findLogicDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_முனை_DType* findNullDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_முனை_DType* findLogic2StateDType() மாறிலி {
        திரும்பு findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_முனை_DType* findDoubleDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_முனை_DType* findIntDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::INT); }
    Ast_முனை_DType* findIntegerDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_முனை_DType* findInteger2StateDType() மாறிலி {
        திரும்பு findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_முனை_DType* findStringDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_முனை_DType* findSigned8DType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_முனை_DType* findUInt32DType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_முனை_DType* findUInt64DType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_முனை_DType* findTimeDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_முனை_DType* findTime2StateDType() மாறிலி {
        திரும்பு findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_முனை_DType* findCHandleDType() மாறிலி { திரும்பு findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_முனை_DType* findConstraintRefDType() மாறிலி;
    Ast_முனை_DType* findEmptyQueueDType() மாறிலி;
    Ast_முனை_DType* findQueueIndexDType() மாறிலி;
    Ast_முனை_DType* findStreamDType() மாறிலி;
    Ast_முனை_DType* findVoidDType() மாறிலி;
    Ast_முனை_DType* findBitDType(முழு width, முழு widthMin, VSigning numeric) மாறிலி;
    Ast_முனை_DType* findLogicDType(முழு width, முழு widthMin, VSigning numeric) மாறிலி;
    Ast_முனை_DType* findLogic2StateDType(முழு width, முழு widthMin, VSigning numeric) மாறிலி;
    Ast_முனை_DType* findBitOrLogicDType(முழு width, முழு widthMin, VSigning numeric,
                                      bool isFourstate) மாறிலி {
        திரும்பு isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_முனை_DType* findBitOrLogicRangeDType(மாறிலி VNumRange& range, மாறிலி முழு widthMin,
                                           மாறிலி VSigning numeric,
                                           மாறிலி bool isFourstate) மாறிலி VL_MT_STABLE {
        திரும்பு isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_முனை_DType* findLogicRangeDType(மாறிலி VNumRange& range, முழு widthMin,
                                      VSigning numeric) மாறிலி VL_MT_STABLE;
    Ast_முனை_DType* findBitRangeDType(மாறிலி VNumRange& range, முழு widthMin,
                                    VSigning numeric) மாறிலி VL_MT_STABLE;
    Ast_முனை_DType* findBasicDType(VBasicDTypeKwd kwd) மாறிலி;
    நிலையான AstBasicDType* find_செருகு_Same_DType(AstBasicDType* nodep);

    நிலையான VCastable computeCastable(மாறிலி Ast_முனை_DType* toDtp, மாறிலி Ast_முனை_DType* fromDtp,
                                     மாறிலி Ast_முனை* fromConstp, bool checkIfaceArgCompat = false);
    நிலையான Ast_முனை_DType* getCommonClassTypep(Ast_முனை* nodep1, Ast_முனை* nodep2);

    // METHODS - dump and error
    வெறுமை v3errorEnd(மாறிலி std::ostringstream& str) மாறிலி VL_RELEASE(V3Error::s().m_mutex);
    வெறுமை v3errorEndFatal(மாறிலி std::ostringstream& str) மாறிலி VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() மாறிலி VL_REQUIRES(V3Error::s().m_mutex) {
        திரும்பு fileline()->warnContextPrimary();
    }
    string warnContextSecondary() மாறிலி { திரும்பு fileline()->warnContextSecondary(); }
    string warnMore() மாறிலி VL_REQUIRES(V3Error::s().m_mutex) { திரும்பு fileline()->warnMore(); }
    string warnOther() மாறிலி VL_REQUIRES(V3Error::s().m_mutex) { திரும்பு fileline()->warnOther(); }

    மெய்நிகர் வெறுமை dump(std::ostream& str = std::cout) மாறிலி;
    நிலையான எழுத்து* dump_மரம்_Json_Gdb(மாறிலி Ast_முனை* nodep);  // For GDB only, free()d by caller
    நிலையான எழுத்து* dump_மரம்_Json_Gdb(மாறிலி எழுத்து* str);  // For GDB only, free()d by caller
    நிலையான எழுத்து* dump_மரம்_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    நிலையான வெறுமை dumpGdb(மாறிலி Ast_முனை* nodep);  // For GDB only
    வெறுமை dumpGdbHeader() மாறிலி;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    template <typename T_முனை_Result, typename T_முனை_Next>
    நிலையான T_முனை_Result* addNext(T_முனை_Result* nodep, T_முனை_Next* newp) {
        static_assert(std::is_base_of<Ast_முனை, T_முனை_Result>::மதிப்பு,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_முனை_Result, T_முனை_Next>::மதிப்பு,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        எனில் (!newp) திரும்பு nodep;
        திரும்பு static_cast<T_முனை_Result*>(addNext<Ast_முனை, Ast_முனை>(nodep, newp));
    }
    template <typename T_முனை_Result, typename T_முனை_Next>
    நிலையான T_முனை_Result* addNextNull(T_முனை_Result* nodep, T_முனை_Next* newp) {
        static_assert(std::is_base_of<Ast_முனை, T_முனை_Result>::மதிப்பு,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_முனை_Result, T_முனை_Next>::மதிப்பு,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        எனில் (!newp) திரும்பு nodep;
        எனில் (!nodep) திரும்பு newp;
        திரும்பு static_cast<T_முனை_Result*>(addNext<Ast_முனை, Ast_முனை>(nodep, newp));
    }
    உள்வரி Ast_முனை* addNext(Ast_முனை* newp);
    வெறுமை addNextHere(Ast_முனை* newp);  // Insert newp at this->nextp
    வெறுமை addHereThisAsNext(Ast_முனை* newp);  // Adds at old place of this, this becomes next
    வெறுமை replaceWith(Ast_முனை* newp);  // Replace current node in tree with new node
    வெறுமை replaceWithKeepDType(Ast_முனை* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_முனை* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_முனை* unlinkFrBackWithNext(VNRelinker* linkerp = nullptr);
    வெறுமை relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    வெறுமை clone_Relink_முனை() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_முனை* clone_மரம்(bool cloneNextLink,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_முனை* clone_மரம்_Pure(bool cloneNextLink) { திரும்பு clone_மரம்(cloneNextLink, true); }
    bool gate_மரம்() { திரும்பு gate_மரம்_Iter(); }  // Is tree isGateOptimizable?
    உள்வரி bool same_மரம்(மாறிலி Ast_முனை* node2p) மாறிலி;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    உள்வரி bool same_Gate_மரம்(மாறிலி Ast_முனை* node2p) மாறிலி;
    வெறுமை delete_மரம்();  // Always deletes the next link
    வெறுமை check_மரம்() மாறிலி VL_MT_STABLE {
        எனில் (v3Global.opt.debugCheck()) check_மரம்_Iter(backp());
    }
    வெறுமை checkIter() மாறிலி;
    வெறுமை dumpPtrs(std::ostream& os = std::cout) மாறிலி;
    வெறுமை dump_மரம்(std::ostream& os = std::cout, மாறிலி string& indent = "    ",
                  முழு maxDepth = 0) மாறிலி;
    வெறுமை dump_மரம்(மாறிலி string& indent, முழு maxDepth = 0) மாறிலி {
        dump_மரம்(std::cout, indent, maxDepth);
    }
    நிலையான வெறுமை dump_மரம்_Gdb(மாறிலி Ast_முனை* nodep);  // For GDB only
    வெறுமை dump_மரம்_And_Next(std::ostream& os = std::cout, மாறிலி string& indent = "    ",
                         முழு maxDepth = 0) மாறிலி;
    வெறுமை dump_மரம்_File(மாறிலி string& filename, bool doDump = true);
    நிலையான வெறுமை dump_மரம்_File_Gdb(மாறிலி Ast_முனை* nodep, மாறிலி எழுத்து* filenamep = nullptr);
    வெறுமை dump_மரம்_Dot(std::ostream& os = std::cout) மாறிலி;
    வெறுமை dump_மரம்_Dot_File(மாறிலி string& filename, bool doDump = true);
    மெய்நிகர் வெறுமை dumpJson(std::ostream& os) மாறிலி { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    வெறுமை dumpJsonGen(std::ostream& os) மாறிலி {};
    மெய்நிகர் வெறுமை dump_மரம்_Json_Op_Gen(std::ostream& os, மாறிலி string& indent) மாறிலி {};
    வெறுமை dump_மரம்_Json(std::ostream& os, மாறிலி string& indent = "") மாறிலி;
    வெறுமை dump_மரம்_Json_File(மாறிலி string& filename, bool doDump = true);
    நிலையான வெறுமை dumpJsonMetaFileGdb(மாறிலி எழுத்து* filename);
    நிலையான வெறுமை dumpJsonMetaFile(மாறிலி string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    நிலையான std::string முனை_Addr(மாறிலி Ast_முனை* nodep) {
        திரும்பு v3Global.opt.dump_மரம்_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    நிலையான Ast_முனை* afterCommentp(Ast_முனை* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        போது (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        திரும்பு nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    மெய்நிகர் bool isBrancher() மாறிலி { திரும்பு false; }
    // Else a AstTime etc that can't be pushed out
    மெய்நிகர் bool isGateOptimizable() மாறிலி { திரும்பு !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    மெய்நிகர் bool isGateDedupable() மாறிலி { திரும்பு isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    மெய்நிகர் bool is_கோவை_Coverage_Eligible() மாறிலி { திரும்பு isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    மெய்நிகர் bool isOutputter() { திரும்பு false; }
    // Else a AstTime etc which output can't be predicted from input
    மெய்நிகர் bool isPredictOptimizable() மாறிலி { திரும்பு !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    மெய்நிகர் bool isPure() { திரும்பு true; }
    // Iff isPure on current node and any nextp()
    bool isPureAndNext() { திரும்பு isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    மெய்நிகர் bool isTimingControl() மாறிலி { திரும்பு false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    மெய்நிகர் bool isUnlikely() மாறிலி { திரும்பு false; }
    // Is an IEEE system function (versus internally-generated)
    மெய்நிகர் bool isSystemFunc() மாறிலி { திரும்பு false; }
    மெய்நிகர் முழு instr_மொத்தம்() மாறிலி { திரும்பு 0; }
    // Iff node is identical to another node
    மெய்நிகர் bool isSame(மாறிலி Ast_முனை* samep) மாறிலி {
        திரும்பு type() == samep->type() && same_முனை(samep);
    }
    // Iff has a data type; dtype() must be non-null
    மெய்நிகர் bool hasDType() மாறிலி VL_MT_SAFE { திரும்பு false; }
    // Iff has a non-null childDTypep(), as generic node function
    மெய்நிகர் Ast_முனை_DType* getChildDTypep() மாறிலி { திரும்பு nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    மெய்நிகர் Ast_முனை_DType* getChild2DTypep() மாறிலி { திரும்பு nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    மெய்நிகர் bool maybePointedTo() மாறிலி VL_MT_SAFE { திரும்பு false; }
    // Don't reclaim this node in V3Dead
    மெய்நிகர் bool undead() மாறிலி { திரும்பு false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    மெய்நிகர் மாறிலி எழுத்து* broken() மாறிலி { திரும்பு nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    மெய்நிகர் மாறிலி எழுத்து* brokenGen() மாறிலி = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(மாறிலி Ast_முனை* மாறிலி newp) மாறிலி { திரும்பு backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    மெய்நிகர் வெறுமை accept(VNVisitorConst& v) = 0;

பாதுகாக்கப்பட்ட:
    // All VNVisitor related functions are called as methods off the visitor
    friend class VNVisitor;
    friend class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    வெறுமை iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    வெறுமை iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    வெறுமை iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    வெறுமை iterateAndNext(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    வெறுமை iterateAndNextConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_முனை* iterateSubtreeReturnEdits(VNVisitor& v);

    நிலையான வெறுமை dump_Json_இலக்கம்(std::ostream& os, மாறிலி std::string& name, int64_t val);
    நிலையான வெறுமை dumpJsonBoolIf(std::ostream& os, மாறிலி std::string& name, bool val);
    நிலையான வெறுமை dumpJsonStr(std::ostream& os, மாறிலி std::string& name, மாறிலி std::string& val);
    நிலையான வெறுமை dumpJsonPtr(std::ostream& os, மாறிலி std::string& name, மாறிலி Ast_முனை* மாறிலி valp);

பாதுகாக்கப்பட்ட:
    வெறுமை iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    template <typename T>
    உள்வரி நிலையான bool privateTypeTest(மாறிலி Ast_முனை* nodep);

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr நிலையான bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        திரும்பு std::is_base_of<T_TargetType, NonCV>::மதிப்பு;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr நிலையான bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        திரும்பு !std::is_base_of<NonCV, T_TargetType>::மதிப்பு;
    }  // LCOV_EXCL_STOP

பொது:
    // For use via the VN_IS macro only, or in templated code
    template <typename T_TargetType, typename T_முனை>
    நிலையான bool is(மாறிலி T_முனை* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        திரும்பு nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    template <typename T_TargetType, typename T_முனை>
    நிலையான T_TargetType* cast(T_முனை* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        திரும்பு nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    template <typename T_TargetType, typename T_முனை>
    நிலையான மாறிலி T_TargetType* cast(மாறிலி T_முனை* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        திரும்பு nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<மாறிலி T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    template <typename T_TargetType, typename T_முனை>
    நிலையான T_TargetType* as(T_முனை* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        திரும்பு unsafePrivateAs<T_TargetType, T_முனை>(nodep);
    }
    template <typename T_TargetType, typename T_முனை>
    நிலையான மாறிலி T_TargetType* as(மாறிலி T_முனை* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        திரும்பு unsafePrivateAs<T_TargetType, T_முனை>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    template <typename T_TargetType, typename T_முனை>
    நிலையான T_TargetType* unsafePrivateAs(T_முனை* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        திரும்பு reinterpret_cast<T_TargetType*>(nodep);
    }
    template <typename T_TargetType, typename T_முனை>
    நிலையான மாறிலி T_TargetType* unsafePrivateAs(மாறிலி T_முனை* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_முனை>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        திரும்பு reinterpret_cast<மாறிலி T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_முனை>
    நிலையான bool mayBeUnder(மாறிலி Ast_முனை* nodep) {
        static_assert(!std::is_const<T_முனை>::மதிப்பு,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        திரும்பு true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_முனை>
    நிலையான constexpr bool isLeaf() {
        static_assert(!std::is_const<T_முனை>::மதிப்பு,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        திரும்பு false;
    }

தனிப்பட்ட:
    // Using std::conditional for const correctness in the public 'foreach' functions
    template <typename T_Arg>
    using Const_Correct_Ast_முனை =
        typename std::conditional<std::is_const<T_Arg>::மதிப்பு, மாறிலி Ast_முனை, Ast_முனை>::type;

    template <typename T_Arg, typename T_Callable>
    உள்வரி நிலையான வெறுமை foreachImpl(Const_Correct_Ast_முனை<T_Arg>* nodep, மாறிலி T_Callable& f,
                                   bool visitNext);

    template <typename T_Arg, bool N_Default, typename T_Callable>
    உள்வரி நிலையான bool predicateImpl(Const_Correct_Ast_முனை<T_Arg>* nodep, மாறிலி T_Callable& p,
                                     bool visitNext);

பொது:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    template <typename T_Callable>
    வெறுமை foreach(T_Callable&& f) {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_முனை*>::மதிப்பு
                          && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_முனை>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    வெறுமை foreach(T_Callable&& f) மாறிலி {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, மாறிலி T_முனை*>::மதிப்பு
                && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<மாறிலி T_முனை>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    வெறுமை foreachAndNext(T_Callable&& f) {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_முனை*>::மதிப்பு
                          && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_முனை>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    வெறுமை foreachAndNext(T_Callable&& f) மாறிலி {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, மாறிலி T_முனை*>::மதிப்பு
                && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<மாறிலி T_முனை>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    template <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_முனை*>::மதிப்பு
                          && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        திரும்பு predicateImpl<T_முனை, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool exists(T_Callable&& p) மாறிலி {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, மாறிலி T_முனை*>::மதிப்பு
                          && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        திரும்பு predicateImpl<மாறிலி T_முனை, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, மாறிலி T_முனை*>::மதிப்பு
                          && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        திரும்பு predicateImpl<T_முனை, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) மாறிலி {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, மாறிலி T_முனை*>::மதிப்பு
                          && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        திரும்பு predicateImpl<மாறிலி T_முனை, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    template <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_முனை*>::மதிப்பு
                          && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        திரும்பு predicateImpl<T_முனை, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool forall(T_Callable&& p) மாறிலி {
        using T_முனை = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, மாறிலி T_முனை*>::மதிப்பு
                          && std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        திரும்பு predicateImpl<மாறிலி T_முனை, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    முழு முனை_மொத்தம்() மாறிலி {
        // TODO: this should really return size_t, but need to fix use sites
        முழு மொத்தம் = 0;
        this->foreach([&மொத்தம்](மாறிலி Ast_முனை*) { ++மொத்தம்; });
        திரும்பு மொத்தம்;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(முழு limit) மாறிலி {
        முழு மொத்தம் = 0;
        திரும்பு this->exists([&மொத்தம், limit](மாறிலி Ast_முனை*) { திரும்பு ++மொத்தம் > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
template <>
Ast_முனை* Ast_முனை::addNext<Ast_முனை, Ast_முனை>(Ast_முனை* nodep, Ast_முனை* newp);

// Inline method implementations
Ast_முனை* Ast_முனை::addNext(Ast_முனை* newp) { திரும்பு addNext(this, newp); }

// Specializations of privateTypeTest
#அந்தர்பாவ "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
template <>
உள்வரி bool Ast_முனை::mayBeUnder<AstCell>(மாறிலி Ast_முனை* nodep) {
    திரும்பு !VN_IS(nodep, முனை_Stmt) && !VN_IS(nodep, முனை_கோவை);
}
template <>
உள்வரி bool Ast_முனை::mayBeUnder<Ast_முனை_Assign>(மாறிலி Ast_முனை* nodep) {
    திரும்பு !VN_IS(nodep, முனை_கோவை);
}
template <>
உள்வரி bool Ast_முனை::mayBeUnder<AstVarScope>(மாறிலி Ast_முனை* nodep) {
    எனில் (VN_IS(nodep, VarScope)) திரும்பு false;  // Should not nest
    எனில் (VN_IS(nodep, Var)) திரும்பு false;
    எனில் (VN_IS(nodep, Active)) திரும்பு false;
    எனில் (VN_IS(nodep, முனை_Stmt)) திரும்பு false;
    எனில் (VN_IS(nodep, முனை_கோவை)) திரும்பு false;
    திரும்பு true;
}
template <>
உள்வரி bool Ast_முனை::mayBeUnder<AstExecGraph>(மாறிலி Ast_முனை* nodep) {
    எனில் (VN_IS(nodep, ExecGraph)) திரும்பு false;  // Should not nest
    எனில் (VN_IS(nodep, முனை_Stmt)) திரும்பு false;  // Should be directly under CFunc
    திரும்பு true;
}
template <>
உள்வரி bool Ast_முனை::mayBeUnder<AstActive>(மாறிலி Ast_முனை* nodep) {
    திரும்பு !VN_IS(nodep, Active);  // AstActives do not nest
}
template <>
உள்வரி bool Ast_முனை::mayBeUnder<AstScope>(மாறிலி Ast_முனை* nodep) {
    திரும்பு !VN_IS(nodep, Scope);  // AstScopes do not nest
}
template <>
உள்வரி bool Ast_முனை::mayBeUnder<Ast_Sen_மரம்>(மாறிலி Ast_முனை* nodep) {
    திரும்பு !VN_IS(nodep, Sen_மரம்);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
template <>
constexpr bool Ast_முனை::isLeaf<Ast_முனை_Var_Ref>() {
    திரும்பு true;
}
template <>
constexpr bool Ast_முனை::isLeaf<AstVarRef>() {
    திரும்பு true;
}
template <>
constexpr bool Ast_முனை::isLeaf<AstVarXRef>() {
    திரும்பு true;
}

// foreach implementation
template <typename T_Arg, typename T_Callable>
வெறுமை Ast_முனை::foreachImpl(Const_Correct_Ast_முனை<T_Arg>* nodep, மாறிலி T_Callable& f, bool visitNext) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using முனை_2 = Const_Correct_Ast_முனை<T_Arg>;

    // We prefetch this far into the stack
    constexpr முழு PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_அடுக்கு_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t அடுக்கு_Size = Initial_அடுக்கு_Size;
    முனை_2* in_வரி_அடுக்கு[Initial_அடுக்கு_Size];  // In-line initial stack storage
    std::unique_ptr<முனை_2*[]> out_Of_வரி_Stackp{nullptr};  // In case we need more, we will allocate
    முனை_2** basep = in_வரி_அடுக்கு + PrefetchDistance;  // Pointer to base of stack
    முனை_2** topp = basep;  // Pointer to top of stack
    முனை_2** limp = in_வரி_அடுக்கு + அடுக்கு_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    க்கு (முழு i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    மாறிலி auto visit = [&](முனை_2* currp) {
        // Type test this node
        எனில் (Ast_முனை::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            எனில் VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) திரும்பு;
        }

        // Enqueue children for traversal, unless futile
        எனில் (mayBeUnder<T_Arg_NonConst>(currp)) {
            எனில் (Ast_முனை* மாறிலி op4p = currp->op4p()) *topp++ = op4p;
            எனில் (Ast_முனை* மாறிலி op3p = currp->op3p()) *topp++ = op3p;
            எனில் (Ast_முனை* மாறிலி op2p = currp->op2p()) *topp++ = op2p;
            எனில் (Ast_முனை* மாறிலி op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    எனில் (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    போது (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        முனை_2* மாறிலி headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        எனில் (VL_UNLIKELY(topp >= limp)) {
            மாறிலி ptrdiff_t occupancy = topp - basep;
            {
                மாறிலி size_t new_அடுக்கு_Size = அடுக்கு_Size * 2;
                முனை_2** மாறிலி newStackp = new முனை_2*[new_அடுக்கு_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            அளவு(முனை_2**) * (occupancy + PrefetchDistance));
                அடுக்கு_Size = new_அடுக்கு_Size;
                out_Of_வரி_Stackp.reset(newStackp);
            }
            basep = out_Of_வரி_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_வரி_Stackp.get() + அடுக்கு_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        எனில் (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
template <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_முனை::predicateImpl(Const_Correct_Ast_முனை<T_Arg>* nodep, மாறிலி T_Callable& p,
                            bool visitNext) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using முனை_2 = Const_Correct_Ast_முனை<T_Arg>;

    // We prefetch this far into the stack
    constexpr முழு PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_அடுக்கு_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t அடுக்கு_Size = Initial_அடுக்கு_Size;
    முனை_2* in_வரி_அடுக்கு[Initial_அடுக்கு_Size];  // In-line initial stack storage
    std::unique_ptr<முனை_2*[]> out_Of_வரி_Stackp{nullptr};  // In case we need more, we will allocate
    முனை_2** basep = in_வரி_அடுக்கு + PrefetchDistance;  // Pointer to base of stack
    முனை_2** topp = basep;  // Pointer to top of stack
    முனை_2** limp = in_வரி_அடுக்கு + அடுக்கு_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    க்கு (முழு i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    மாறிலி auto visit = [&](முனை_2* currp) {
        // Type test this node
        எனில் (Ast_முனை::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            எனில் (p(static_cast<T_Arg*>(currp)) != N_Default) திரும்பு true;
            // Short circuit if iterating leaf nodes
            எனில் VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) திரும்பு false;
        }

        // Enqueue children for traversal, unless futile
        எனில் (mayBeUnder<T_Arg_NonConst>(currp)) {
            எனில் (Ast_முனை* மாறிலி op4p = currp->op4p()) *topp++ = op4p;
            எனில் (Ast_முனை* மாறிலி op3p = currp->op3p()) *topp++ = op3p;
            எனில் (Ast_முனை* மாறிலி op2p = currp->op2p()) *topp++ = op2p;
            எனில் (Ast_முனை* மாறிலி op1p = currp->op1p()) *topp++ = op1p;
        }

        திரும்பு false;
    };

    // Enqueue the next of the root node, if required
    எனில் (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    எனில் (visit(nodep)) திரும்பு !N_Default;

    // Visit the rest of the tree
    போது (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        முனை_2* மாறிலி headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        எனில் (VL_UNLIKELY(topp >= limp)) {
            மாறிலி ptrdiff_t occupancy = topp - basep;
            {
                மாறிலி size_t new_அடுக்கு_Size = அடுக்கு_Size * 2;
                முனை_2** மாறிலி newStackp = new முனை_2*[new_அடுக்கு_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            அளவு(முனை_2**) * (occupancy + PrefetchDistance));
                அடுக்கு_Size = new_அடுக்கு_Size;
                out_Of_வரி_Stackp.reset(newStackp);
            }
            basep = out_Of_வரி_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_வரி_Stackp.get() + அடுக்கு_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        எனில் (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        எனில் (visit(headp)) திரும்பு !N_Default;
    }

    திரும்பு N_Default;
}

உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி Ast_முனை* rhs) {
    எனில் (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } இல்லையெனில் எனில் (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } இல்லையெனில் {
        rhs->dump(os);
    }
    திரும்பு os;
}
வெறுமை VNRelinker::relink(Ast_முனை* newp) { newp->Ast_முனை::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
template <typename T_முனை>
class VNRef final : பொது std::reference_wrapper<T_முனை> {
    static_assert(std::is_base_of<Ast_முனை, T_முனை>::மதிப்பு,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

பொது:
    template <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_முனை>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(மாறிலி std::reference_wrapper<T_முனை>& other)
        : std::reference_wrapper<T_முனை>{other} {}
};

static_assert(அளவு(VNRef<Ast_முனை>) == அளவு(std::reference_wrapper<Ast_முனை>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
size_t V3HasherUncachedHash(மாறிலி Ast_முனை&);

// Specialization of std::hash for VNRef
template <typename T_முனை>
கட்டமைப்பு std::hash<VNRef<T_முனை>> final {
    size_t operator()(VNRef<T_முனை> r) மாறிலி { திரும்பு V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
template <typename T_முனை>
கட்டமைப்பு std::equal_to<VNRef<T_முனை>> final {
    size_t operator()(VNRef<T_முனை> ra, VNRef<T_முனை> rb) மாறிலி {
        திரும்பு ra.get().same_மரம்(&(rb.get()));
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
    std::unordered_map<மாறிலி Ast_முனை*, Ast_முனை*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<மாறிலி Ast_முனை*, std::vector<Ast_முனை*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<மாறிலி Ast_முனை*, std::string> m_prefixes;
    மாறிலி VDTypeNameScopes* மாறிலி m_prevp;  // Index in use before this one
    நிலையான மாறிலி VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    வெறுமை index(Ast_முனை* nodep, Ast_முனை* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

பொது:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    நிலையான மாறிலி VDTypeNameScopes* currentp() { திரும்பு s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(மாறிலி Ast_முனை* nodep, Ast_முனை*& outerpr) மாறிலி;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    மாறிலி std::vector<Ast_முனை*>* paramsp(மாறிலி Ast_முனை* modp) மாறிலி;
    // The prefix kept for scope 'scopep', or nullptr if none
    மாறிலி std::string* prefixp(மாறிலி Ast_முனை* scopep) மாறிலி;
    // Keep 'prefix', final, for scope 'scopep'
    வெறுமை prefix(மாறிலி Ast_முனை* scopep, மாறிலி std::string& prefix) மாறிலி;
};

//######################################################################
// Inline VNVisitor METHODS

வெறுமை VNVisitorConst::iterateConst(Ast_முனை* nodep) { nodep->accept(*this); }
வெறுமை VNVisitorConst::iterateConstNull(Ast_முனை* nodep) {
    எனில் (VL_LIKELY(nodep)) nodep->accept(*this);
}
வெறுமை VNVisitorConst::iterateChildrenConst(Ast_முனை* nodep) { nodep->iterateChildrenConst(*this); }
வெறுமை VNVisitorConst::iterateChildrenBackwardsConst(Ast_முனை* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
வெறுமை VNVisitorConst::iterateAndNextConstNull(Ast_முனை* nodep) {
    எனில் (VL_LIKELY(nodep)) nodep->iterateAndNextConst(*this);
}

வெறுமை VNVisitor::iterate(Ast_முனை* nodep) { nodep->accept(*this); }
வெறுமை VNVisitor::iterateNull(Ast_முனை* nodep) {
    எனில் (VL_LIKELY(nodep)) nodep->accept(*this);
}
வெறுமை VNVisitor::iterateChildren(Ast_முனை* nodep) { nodep->iterateChildren(*this); }
வெறுமை VNVisitor::iterateAndNextNull(Ast_முனை* nodep) {
    எனில் (VL_LIKELY(nodep)) nodep->iterateAndNext(*this);
}
Ast_முனை* VNVisitor::iterateSubtreeReturnEdits(Ast_முனை* nodep) {
    திரும்பு nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#அந்தர்பாவ "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#அந்தர்பாவ "V3AstNodeDType.h"
#அந்தர்பாவ "V3AstNodeOther.h"
#அந்தர்பாவ "V3AstNodeExpr.h"
#அந்தர்பாவ "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#அந்தர்பாவ "V3AstInlines.h"
வெறுமை dump_முனை_List_Json(std::ostream& os, மாறிலி Ast_முனை* nodep, மாறிலி std::string& listName,
                      மாறிலி string& indent);

#endif  // Guard
