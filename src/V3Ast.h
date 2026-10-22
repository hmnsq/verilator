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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Broken.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3FunctionTraits.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Number.h"
#അന്തര്ഭാവ "V3StdFuture.h"

#അന്തര്ഭാവ "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#അന്തര്ഭാവ <array>
#അന്തര്ഭാവ <cmath>
#അന്തര്ഭാവ <cstdint>
#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <type_traits>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <utility>
#അന്തര്ഭാവ <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#എങ്കിൽ defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#undef VL_USER_TYPE_CHECKS
#endif
#ifdef VL_USER_TYPE_CHECKS
#അന്തര്ഭാവ <variant>
#endif

// clang-format off
#അന്തര്ഭാവ "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3Graph;
class ExecMTask;
class VIfaceCaptureTag;

//######################################################################

// For broken() function, return error string if have a match
#define BROKEN_RTN(test) \
    ചെയ്യുക { \
        എങ്കിൽ (VL_UNCOVERABLE(test)) തിരികെ "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } ആകുമ്പോൾ (false)
// For broken() function, return error string if a base of this class has a match
#define BROKEN_BASE_RTN(test) \
    ചെയ്യുക { \
        സ്ഥിരം അക്ഷരം* സ്ഥിരം reasonp = (test); \
        എങ്കിൽ (VL_UNCOVERABLE(reasonp)) തിരികെ reasonp; \
    } ആകുമ്പോൾ (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#define VN_IS(nodep, nodetypename) (Ast_കണ്ണി::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#define VN_CAST(nodep, nodetypename) (Ast_കണ്ണി::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#define VN_AS(nodep, nodetypename) (Ast_കണ്ണി::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#എങ്കിൽ defined(VL_DEBUG) || defined(VL_ASAN)
#define VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#അല്ലെങ്കിൽ
#define VN_DBG_AS(nodep, nodetypename) (Ast_കണ്ണി::unsafePrivateAs<Ast##nodetypename>(nodep))
#endif

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#define VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

ഘടന VNTypeInfo final {
    സ്ഥിരം അക്ഷരം* m_namep;
    എണ്ണൽ OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    സ്ഥിരം അക്ഷരം* m_opNamep[4];
    size_t m_sizeof;
};

class VNType final {
    സ്ഥിതം സ്ഥിരം VNTypeInfo s_typeInfoTable[];

പൊതു:
#അന്തര്ഭാവ "V3Ast__gen_type_enum.h"  // From ./astgen
    സ്ഥിരം en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr operator en() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e; }

    // Retrieve VNTypeInfo for the given type
    സ്ഥിതം സ്ഥിരം VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { തിരികെ s_typeInfoTable[t.m_e]; }
};
constexpr bool operator==(സ്ഥിരം VNType& lhs, സ്ഥിരം VNType& rhs) VL_PURE {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VNType& lhs, VNType::en rhs) VL_PURE { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VNType::en lhs, സ്ഥിരം VNType& rhs) VL_PURE { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VNType& rhs) { തിരികെ os << rhs.ascii(); }

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
    std::variant<std::monostate, uint64_t, ശൂന്യം*> m_u;
#അല്ലെങ്കിൽ
    സംഘം {
        ശൂന്യം* up;
        uint64_t uq;
    } m_u;
#endif

പൊതു:
#ifdef VL_USER_TYPE_CHECKS
    VNUser() = സ്ഥിരസ്ഥിതി;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        എങ്കിൽ (i) m_u = i;
    }
    explicit VNUser(ശൂന്യം* p) {
        // VNUser{nullptr} represents the monostate
        എങ്കിൽ (p) m_u = p;
    }
    ~VNUser() = സ്ഥിരസ്ഥിതി;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::മൂല്യം, T>::type to() സ്ഥിരം VL_MT_SAFE {
        എങ്കിൽ (std::holds_alternative<std::monostate>(m_u)) തിരികെ nullptr;
        ശൂന്യം* സ്ഥിരം* സ്ഥിരം upp = std::get_if<ശൂന്യം*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        തിരികെ reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() സ്ഥിരം {
        എങ്കിൽ (std::holds_alternative<std::monostate>(m_u)) തിരികെ 0;
        സ്ഥിരം uint64_t* സ്ഥിരം uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        തിരികെ *uip;
    }
#അല്ലെങ്കിൽ
    VNUser() = സ്ഥിരസ്ഥിതി;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.up = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(ശൂന്യം* p) { m_u.up = p; }
    ~VNUser() = സ്ഥിരസ്ഥിതി;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::മൂല്യം, T>::type to() സ്ഥിരം VL_MT_SAFE {
        തിരികെ reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() സ്ഥിരം { തിരികെ m_u.uq; }
#endif
    VSymEnt* toSymEnt() സ്ഥിരം { തിരികെ to<VSymEnt*>(); }
    Ast_കണ്ണി* toNodep() സ്ഥിരം VL_MT_SAFE { തിരികെ to<Ast_കണ്ണി*>(); }
    V3GraphVertex* toGraphVertex() സ്ഥിരം { തിരികെ to<V3GraphVertex*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(സ്ഥിരം ശൂന്യം*)) സ്ഥിരം;
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
VL_NOT_FINAL{സംരക്ഷിത : സ്ഥിതം ശൂന്യം allocate(പൂർണ്ണസംഖ്യ id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvtToStr(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
സ്ഥിതം ശൂന്യം സ്വതന്ത്രം(പൂർണ്ണസംഖ്യ id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvtToStr(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
സ്ഥിതം ശൂന്യം clearcnt(പൂർണ്ണസംഖ്യ id, uint32_t& cntGblRef, സ്ഥിരം bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
സ്ഥിതം ശൂന്യം checkcnt(പൂർണ്ണസംഖ്യ id, uint32_t&, സ്ഥിരം bool& userBusyRef) {
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
സംരക്ഷിത:
    friend class Ast_കണ്ണി;
    സ്ഥിതം uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    സ്ഥിതം bool s_userBusy;  // Count is in use
പൊതു:
    VNUser1InUse()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1InUse()     { സ്വതന്ത്രം    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    സ്ഥിതം ശൂന്യം clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    സ്ഥിതം ശൂന്യം check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2InUse final : VNUserInUseBase {
സംരക്ഷിത:
    friend class Ast_കണ്ണി;
    സ്ഥിതം uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    സ്ഥിതം bool s_userBusy;  // Count is in use
പൊതു:
    VNUser2InUse()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2InUse()     { സ്വതന്ത്രം    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    സ്ഥിതം ശൂന്യം clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    സ്ഥിതം ശൂന്യം check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3InUse final : VNUserInUseBase {
സംരക്ഷിത:
    friend class Ast_കണ്ണി;
    സ്ഥിതം uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    സ്ഥിതം bool s_userBusy;  // Count is in use
പൊതു:
    VNUser3InUse()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3InUse()     { സ്വതന്ത്രം    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    സ്ഥിതം ശൂന്യം clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    സ്ഥിതം ശൂന്യം check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4InUse final : VNUserInUseBase {
സംരക്ഷിത:
    friend class Ast_കണ്ണി;
    സ്ഥിതം uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    സ്ഥിതം bool s_userBusy;  // Count is in use
പൊതു:
    VNUser4InUse()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4InUse()     { സ്വതന്ത്രം    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    സ്ഥിതം ശൂന്യം clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    സ്ഥിതം ശൂന്യം check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::vector<Ast_കണ്ണി*> m_deleteps;  // Nodes to delete

പൊതു:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    ശൂന്യം തള്ളുക_Deletep(Ast_കണ്ണി* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.തള്ളുക_back(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    ശൂന്യം doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    friend class Ast_കണ്ണി;

പൊതു:
    /// Call visit()s on nodep
    വരിയിൽ ശൂന്യം iterateConst(Ast_കണ്ണി* nodep);
    /// Call visit()s on nodep
    വരിയിൽ ശൂന്യം iterateConstNull(Ast_കണ്ണി* nodep);
    /// Call visit()s on const nodep's children
    വരിയിൽ ശൂന്യം iterateChildrenConst(Ast_കണ്ണി* nodep);
    /// Call visit()s on nodep's children in backp() order
    വരിയിൽ ശൂന്യം iterateChildrenBackwardsConst(Ast_കണ്ണി* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    വരിയിൽ ശൂന്യം iterateAndNextConstNull(Ast_കണ്ണി* nodep);

    മായാ ശൂന്യം visit(Ast_കണ്ണി* nodep) = 0;
    മായാ ~VNVisitorConst() {}
#അന്തര്ഭാവ "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : പൊതു VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
പൊതു:
    /// Call visit()s on nodep
    വരിയിൽ ശൂന്യം iterate(Ast_കണ്ണി* nodep);
    /// Call visit()s on nodep
    വരിയിൽ ശൂന്യം iterateNull(Ast_കണ്ണി* nodep);
    /// Call visit()s on nodep's children
    വരിയിൽ ശൂന്യം iterateChildren(Ast_കണ്ണി* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    വരിയിൽ ശൂന്യം iterateAndNextNull(Ast_കണ്ണി* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    വരിയിൽ Ast_കണ്ണി* iterateSubtreeReturnEdits(Ast_കണ്ണി* nodep);

    VNDeleter& deleter() { തിരികെ m_deleter; }
    ശൂന്യം തള്ളുക_Deletep(Ast_കണ്ണി* nodep) { deleter().തള്ളുക_Deletep(nodep); }
    ശൂന്യം doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
സംരക്ഷിത:
    friend class Ast_കണ്ണി;
    എണ്ണൽ RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_NEXT,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_കണ്ണി* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_കണ്ണി* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_കണ്ണി** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

പൊതു:
    VNRelinker() = സ്ഥിരസ്ഥിതി;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    വരിയിൽ ശൂന്യം relink(Ast_കണ്ണി* newp);
    ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം;
};
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VNRelinker& rhs) {
    rhs.dump(os);
    തിരികെ os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{പൊതു : മായാ bool കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം {തിരികെ true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#define ASTNODE_PREFETCH_NON_NULL(nodep) \
    ചെയ്യുക { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } ആകുമ്പോൾ (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#define ASTNODE_PREFETCH(nodep) \
    ചെയ്യുക { \
        എങ്കിൽ (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } ആകുമ്പോൾ (false)

class Ast_കണ്ണി VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_കണ്ണി* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_കണ്ണി* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_കണ്ണി* m_op1p = nullptr;  // Generic pointer 1
    Ast_കണ്ണി* m_op2p = nullptr;  // Generic pointer 2
    Ast_കണ്ണി* m_op3p = nullptr;  // Generic pointer 3
    Ast_കണ്ണി* m_op4p = nullptr;  // Generic pointer 4
    Ast_കണ്ണി** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    സ്ഥിരം VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    ഘടന {
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

    പൂർണ്ണസംഖ്യ m_cloneCnt = 0;  // Sequence number for when last clone was made

#എങ്കിൽ defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(വലുപ്പം(m_type) + വലുപ്പം(m_flags) + വലുപ്പം(m_brokenState) + വലുപ്പം(m_cloneCnt)
                      <= വലുപ്പം(ശൂന്യം*),
                  "packing requires padding");
#endif

    Ast_കണ്ണി_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_കണ്ണി* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_വരി* m_fileline;  // Where it was declared
#ifdef VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_എണ്ണം;  // When it was last edited
#endif
    സ്ഥിതം uint64_t s_editCntGbl;  // Global edit counter
    സ്ഥിതം uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_കണ്ണി* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    സ്ഥിതം പൂർണ്ണസംഖ്യ s_cloneCntGbl;  // Count of which userp is set

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
    ശൂന്യം op1p(Ast_കണ്ണി* nodep) {
        m_op1p = nodep;
        എങ്കിൽ (nodep) nodep->m_backp = this;
    }
    ശൂന്യം op2p(Ast_കണ്ണി* nodep) {
        m_op2p = nodep;
        എങ്കിൽ (nodep) nodep->m_backp = this;
    }
    ശൂന്യം op3p(Ast_കണ്ണി* nodep) {
        m_op3p = nodep;
        എങ്കിൽ (nodep) nodep->m_backp = this;
    }
    ശൂന്യം op4p(Ast_കണ്ണി* nodep) {
        m_op4p = nodep;
        എങ്കിൽ (nodep) nodep->m_backp = this;
    }

സ്വകാര്യ:
    Ast_കണ്ണി* clone_വൃക്ഷം_Iter(bool needPure);
    Ast_കണ്ണി* clone_വൃക്ഷം_Iter_List(bool needPure);
    ശൂന്യം check_വൃക്ഷം_Iter(സ്ഥിരം Ast_കണ്ണി* prevBackp) സ്ഥിരം VL_MT_STABLE;
    bool gate_വൃക്ഷം_Iter() സ്ഥിരം;
    സ്ഥിതം bool same_വൃക്ഷം_Iter(സ്ഥിരം Ast_കണ്ണി* node1p, സ്ഥിരം Ast_കണ്ണി* node2p, bool ignNext,
                             bool gateOnly);
    ശൂന്യം delete_വൃക്ഷം_Iter();
    ശൂന്യം delete_കണ്ണി();
    string instanceStr() സ്ഥിരം;

പൊതു:
    ശൂന്യം purityCheck();
    സ്ഥിതം ശൂന്യം relinkOneLink(Ast_കണ്ണി*& pointpr, Ast_കണ്ണി* newp);
    // cppcheck-suppress functionConst
    സ്ഥിതം ശൂന്യം debug_വൃക്ഷം_Change(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം അക്ഷരം* prefix, പൂർണ്ണസംഖ്യ lineno, bool next);

സംരക്ഷിത:
    // CONSTRUCTORS
    Ast_കണ്ണി(VNType t, File_വരി* fl);
    മായാ ~Ast_കണ്ണി() = സ്ഥിരസ്ഥിതി;  // Use 'deleteTree' instead
    മായാ Ast_കണ്ണി* clone() = 0;  // Generally, cloneTree is what you want instead
    മായാ ശൂന്യം cloneRelink() { cloneRelinkGen(); }
    മായാ ശൂന്യം cloneRelinkGen() {};  // Overrides generated by 'astgen'
    ശൂന്യം clone_Relink_വൃക്ഷം();

    // METHODS
    ശൂന്യം setOp1p(Ast_കണ്ണി* newp);  // Set non-list-type op1 to non-list element
    ശൂന്യം setOp2p(Ast_കണ്ണി* newp);  // Set non-list-type op2 to non-list element
    ശൂന്യം setOp3p(Ast_കണ്ണി* newp);  // Set non-list-type op3 to non-list element
    ശൂന്യം setOp4p(Ast_കണ്ണി* newp);  // Set non-list-type op4 to non-list element

    ശൂന്യം addOp1p(Ast_കണ്ണി* newp);  // Append newp to end of op1
    ശൂന്യം addOp2p(Ast_കണ്ണി* newp);  // Append newp to end of op2
    ശൂന്യം addOp3p(Ast_കണ്ണി* newp);  // Append newp to end of op3
    ശൂന്യം addOp4p(Ast_കണ്ണി* newp);  // Append newp to end of op4

    ശൂന്യം prependOp1p(Ast_കണ്ണി* newp);  // Prepend newp to front of op1
    ശൂന്യം prependOp2p(Ast_കണ്ണി* newp);  // Prepend newp to front of op2
    ശൂന്യം prependOp3p(Ast_കണ്ണി* newp);  // Prepend newp to front of op3
    ശൂന്യം prependOp4p(Ast_കണ്ണി* newp);  // Prepend newp to front of op4

    // clang-format off
    ശൂന്യം setNOp1p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) setOp1p(newp); }
    ശൂന്യം setNOp2p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) setOp2p(newp); }
    ശൂന്യം setNOp3p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) setOp3p(newp); }
    ശൂന്യം setNOp4p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) setOp4p(newp); }

    ശൂന്യം addNOp1p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) addOp1p(newp); }
    ശൂന്യം addNOp2p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) addOp2p(newp); }
    ശൂന്യം addNOp3p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) addOp3p(newp); }
    ശൂന്യം addNOp4p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) addOp4p(newp); }

    ശൂന്യം prependNOp1p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) prependOp1p(newp); }
    ശൂന്യം prependNOp2p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) prependOp2p(newp); }
    ശൂന്യം prependNOp3p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) prependOp3p(newp); }
    ശൂന്യം prependNOp4p(Ast_കണ്ണി* newp) { എങ്കിൽ (newp) prependOp4p(newp); }
    // clang-format on

    ശൂന്യം clonep(Ast_കണ്ണി* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    സ്ഥിതം ശൂന്യം clone_Clear_വൃക്ഷം() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    മായാ bool same_കണ്ണി(സ്ഥിരം Ast_കണ്ണി*) സ്ഥിരം { തിരികെ true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    മായാ bool wouldBreakGen(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം oldp,
                               സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം newp) സ്ഥിരം
        = 0;  // Generated by 'astgen'

പൊതു:
    // ACCESSORS
    VNType type() സ്ഥിരം VL_MT_SAFE { തിരികെ m_type; }
    സ്ഥിരം അക്ഷരം* typeName() സ്ഥിരം VL_MT_SAFE { തിരികെ type().ascii(); }  // See also prettyTypeName
    Ast_കണ്ണി* nextp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_nextp; }
    Ast_കണ്ണി* backp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_backp; }
    Ast_കണ്ണി* abovep() സ്ഥിരം;  // Get parent node above, only for list head and tail
    Ast_കണ്ണി* aboveLoopp() സ്ഥിരം;  // Get parent node above, may have performance issues as loops
    Ast_കണ്ണി* lastp() സ്ഥിരം {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        തിരികെ m_headtailp;
    }
    Ast_കണ്ണി* prevp() സ്ഥിരം {  // Previous node in list, nullptr for head of list
        എങ്കിൽ (m_backp->m_nextp != this) തിരികെ nullptr;
        തിരികെ m_backp;
    }
    Ast_കണ്ണി* op1p() സ്ഥിരം VL_MT_STABLE { തിരികെ m_op1p; }
    Ast_കണ്ണി* op2p() സ്ഥിരം VL_MT_STABLE { തിരികെ m_op2p; }
    Ast_കണ്ണി* op3p() സ്ഥിരം VL_MT_STABLE { തിരികെ m_op3p; }
    Ast_കണ്ണി* op4p() സ്ഥിരം VL_MT_STABLE { തിരികെ m_op4p; }
    Ast_കണ്ണി_DType* dtypep() സ്ഥിരം VL_MT_STABLE { തിരികെ m_dtypep; }
    Ast_കണ്ണി* clonep() സ്ഥിരം { തിരികെ ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_കണ്ണി* firstAbovep() സ്ഥിരം {  // Returns nullptr when second or later in list
        തിരികെ ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() സ്ഥിരം VL_MT_SAFE { തിരികെ m_brokenState; }
    ശൂന്യം brokenState(uint8_t മൂല്യം) { m_brokenState = മൂല്യം; }

    // Used by AstNode::broken()
    bool brokeExists() സ്ഥിരം { തിരികെ V3Broken::isLinkable(this); }
    bool brokeExistsAbove() സ്ഥിരം { തിരികെ brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    മായാ ശൂന്യം foreachLink(std::function<ശൂന്യം(Ast_കണ്ണി** linkpp, സ്ഥിരം അക്ഷരം* namep)> f) = 0;

#എങ്കിൽ defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    സ്ഥിതം ശൂന്യം* operator new(size_t size);
    സ്ഥിതം ശൂന്യം operator delete(ശൂന്യം* obj, size_t size);
#endif

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_BRANCH = 4;  // Branch
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_CALL = INSTR_എണ്ണം_BRANCH + 10;  // Subroutine call
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_LD = 2;  // Load memory
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_INT_MUL = 3;  // Integer multiply
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_INT_DIV = 10;  // Integer divide
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_DBL = 8;  // Convert or do float ops
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_DBL_DIV = 40;  // Double divide
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_DBL_TRIG = 200;  // Double trigonometric ops
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_STR = 100;  // String ops
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_TIME = INSTR_എണ്ണം_CALL + 5;  // Determine simulation time
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_PLI = 20;  // PLI routines
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INSTR_എണ്ണം_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    മായാ string name() സ്ഥിരം VL_MT_STABLE { തിരികെ ""; }
    മായാ string origName() സ്ഥിരം { തിരികെ ""; }
    string prettyDehashOrigOrName() സ്ഥിരം {
        തിരികെ prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    മായാ ശൂന്യം name(സ്ഥിരം string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    മായാ ശൂന്യം tag(സ്ഥിരം string& text) {}
    മായാ string tag() സ്ഥിരം { തിരികെ ""; }
    മായാ uint32_t decl_Token_അക്കം() സ്ഥിരം { തിരികെ 0; }
    മായാ ശൂന്യം decl_Token_അക്കം_Set_Min(uint32_t token_അക്കം) {}
    മായാ string verilogKwd() സ്ഥിരം { തിരികെ ""; }
    string nameProtect() സ്ഥിരം VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() സ്ഥിരം;  // origName with --protect-id applied
    string shortName() സ്ഥിരം;  // Name with __PVT__ removed for concatenating scopes
    സ്ഥിതം string dedotName(സ്ഥിരം string& namein);  // Name with dots removed
    സ്ഥിതം string nameNoArray(സ്ഥിരം string& namein);  // Name with any array index removed
    സ്ഥിതം string prettyName(സ്ഥിരം string& namein) VL_PURE;  // Name for printing out to the user
    സ്ഥിതം string vpiName(സ്ഥിരം string& namein);  // Name for vpi access
    സ്ഥിതം string prettyNameQ(സ്ഥിരം string& namein) {  // Quoted pretty name (for errors)
        തിരികെ "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    സ്ഥിതം string encodeName(സ്ഥിരം string& namein);
    സ്ഥിതം string encode_സംഖ്യ(int64_t അക്കം);  // Encode number into internal C representation
    സ്ഥിതം string vcdName(സ്ഥിരം string& namein);  // Name for printing out to vcd files
    string prettyName() സ്ഥിരം { തിരികെ prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    മായാ string pretty_Name_സന്ദേശം() സ്ഥിരം { തിരികെ prettyName(); }
    string prettyNameQ() സ്ഥിരം { തിരികെ "'"s + pretty_Name_സന്ദേശം() + "'"; }  // Quoted, for messages
    string verilogName() സ്ഥിരം { തിരികെ vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() സ്ഥിരം;
    മായാ string prettyOperatorName() സ്ഥിരം { തിരികെ "operator " + prettyTypeName(); }
    File_വരി* fileline() സ്ഥിരം VL_MT_SAFE { തിരികെ m_fileline; }
    ശൂന്യം fileline(File_വരി* fl) { m_fileline = fl; }
    വരിയിൽ bool width1() സ്ഥിരം;
    വരിയിൽ പൂർണ്ണസംഖ്യ widthInstrs() സ്ഥിരം;
    bool didWidth() സ്ഥിരം { തിരികെ m_flags.didWidth; }
    ശൂന്യം didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        എങ്കിൽ (didWidth()) തിരികെ true;
        didWidth(true);
        തിരികെ false;
    }
    bool doingWidth() സ്ഥിരം { തിരികെ m_flags.doingWidth; }
    ശൂന്യം doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() സ്ഥിരം VL_MT_SAFE { തിരികെ m_flags.protect; }
    ശൂന്യം protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    വരിയിൽ പൂർണ്ണസംഖ്യ width() സ്ഥിരം VL_MT_STABLE;
    വരിയിൽ പൂർണ്ണസംഖ്യ widthMin() സ്ഥിരം VL_MT_STABLE;
    പൂർണ്ണസംഖ്യ widthMinV() സ്ഥിരം {
        തിരികെ v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    പൂർണ്ണസംഖ്യ width_വാക്കുകൾ() സ്ഥിരം { തിരികെ VL_വാക്കുകൾ_I(width()); }
    bool isQuad() സ്ഥിരം VL_MT_STABLE { തിരികെ (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() സ്ഥിരം VL_MT_STABLE { തിരികെ (width() > VL_QUADSIZE); }
    വരിയിൽ bool isCHandle() സ്ഥിരം VL_MT_STABLE;
    വരിയിൽ bool isDouble() സ്ഥിരം VL_MT_STABLE;
    വരിയിൽ bool isSigned() സ്ഥിരം VL_MT_STABLE;
    വരിയിൽ bool isString() സ്ഥിരം VL_MT_STABLE;
    വരിയിൽ bool isEvent() സ്ഥിരം VL_MT_STABLE;

    // clang-format off
    VNUser user1u() സ്ഥിരം VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        തിരികെ ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_കണ്ണി* user1p() സ്ഥിരം VL_MT_STABLE { തിരികെ user1u().toNodep(); }
    ശൂന്യം user1u(സ്ഥിരം VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    ശൂന്യം user1p(ശൂന്യം* userp) { user1u(VNUser{userp}); }
    ശൂന്യം user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() സ്ഥിരം { തിരികെ user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { സ്ഥിരം uint64_t v = user1(); user1(v + val); തിരികെ v; }
    uint64_t user1Or(uint64_t val) { സ്ഥിരം uint64_t v = user1(); user1(v | val); തിരികെ v; }
    uint64_t user1SetOnce() { സ്ഥിരം uint64_t v = user1(); എങ്കിൽ (!v) user1(1); തിരികെ v; }  // Better for cache than user1Inc()
    സ്ഥിതം ശൂന്യം user1_Clear_വൃക്ഷം() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() സ്ഥിരം VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        തിരികെ ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_കണ്ണി* user2p() സ്ഥിരം VL_MT_STABLE { തിരികെ user2u().toNodep(); }
    ശൂന്യം user2u(സ്ഥിരം VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    ശൂന്യം user2p(ശൂന്യം* userp) { user2u(VNUser{userp}); }
    ശൂന്യം user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() സ്ഥിരം { തിരികെ user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { സ്ഥിരം uint64_t v = user2(); user2(v + val); തിരികെ v; }
    uint64_t user2Or(uint64_t val) { സ്ഥിരം uint64_t v = user2(); user2(v | val); തിരികെ v; }
    uint64_t user2SetOnce() { സ്ഥിരം uint64_t v = user2(); എങ്കിൽ (!v) user2(1); തിരികെ v; }  // Better for cache than user2Inc()
    സ്ഥിതം ശൂന്യം user2_Clear_വൃക്ഷം() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() സ്ഥിരം VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        തിരികെ ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_കണ്ണി* user3p() സ്ഥിരം VL_MT_STABLE { തിരികെ user3u().toNodep(); }
    ശൂന്യം user3u(സ്ഥിരം VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    ശൂന്യം user3p(ശൂന്യം* userp) { user3u(VNUser{userp}); }
    ശൂന്യം user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() സ്ഥിരം { തിരികെ user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { സ്ഥിരം uint64_t v = user3(); user3(v + val); തിരികെ v; }
    uint64_t user3Or(uint64_t val) { സ്ഥിരം uint64_t v = user3(); user3(v | val); തിരികെ v; }
    uint64_t user3SetOnce() { സ്ഥിരം uint64_t v = user3(); എങ്കിൽ (!v) user3(1); തിരികെ v; }  // Better for cache than user3Inc()
    സ്ഥിതം ശൂന്യം user3_Clear_വൃക്ഷം() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() സ്ഥിരം VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        തിരികെ ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_കണ്ണി* user4p() സ്ഥിരം VL_MT_STABLE { തിരികെ user4u().toNodep(); }
    ശൂന്യം user4u(സ്ഥിരം VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    ശൂന്യം user4p(ശൂന്യം* userp) { user4u(VNUser{userp}); }
    ശൂന്യം user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() സ്ഥിരം { തിരികെ user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { സ്ഥിരം uint64_t v = user4(); user4(v | val); തിരികെ v; }
    uint64_t user4Inc(uint64_t val = 1) { സ്ഥിരം uint64_t v = user4(); user4(v + val); തിരികെ v; }
    uint64_t user4SetOnce() { സ്ഥിരം uint64_t v = user4(); എങ്കിൽ (!v) user4(1); തിരികെ v; }  // Better for cache than user4Inc()
    സ്ഥിതം ശൂന്യം user4_Clear_വൃക്ഷം() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ifdef VL_DEBUG
    uint64_t edit_എണ്ണം() സ്ഥിരം { തിരികെ m_edit_എണ്ണം; }
    ശൂന്യം edit_എണ്ണം_Inc() {
        m_edit_എണ്ണം = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_വൃക്ഷം();  // Any edit clears all caching
    }
#അല്ലെങ്കിൽ
    ശൂന്യം edit_എണ്ണം_Inc() { ++s_editCntGbl; }
#endif
    സ്ഥിതം uint64_t edit_എണ്ണം_Last() VL_MT_SAFE { തിരികെ s_editCntLast; }
    സ്ഥിതം uint64_t edit_എണ്ണം_Gbl() VL_MT_SAFE { തിരികെ s_editCntGbl; }
    സ്ഥിതം ശൂന്യം edit_എണ്ണം_Set_Last() { s_editCntLast = edit_എണ്ണം_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_തള്ളുക_Self_Stmt();
    വരിയിൽ bool is_Class_Handle_മൂല്യം() സ്ഥിരം;
    വരിയിൽ bool isNull() സ്ഥിരം;
    വരിയിൽ bool isZero() സ്ഥിരം;
    വരിയിൽ bool isOne() സ്ഥിരം;
    വരിയിൽ bool isNeqZero() സ്ഥിരം;
    വരിയിൽ bool isAllOnes() സ്ഥിരം;
    വരിയിൽ bool isAllOnesV() സ്ഥിരം;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    ശൂന്യം dtypep(Ast_കണ്ണി_DType* nodep) {
        എങ്കിൽ (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_എണ്ണം_Inc();
        }
    }
    ശൂന്യം dtypeFrom(സ്ഥിരം Ast_കണ്ണി* fromp) {
        എങ്കിൽ (fromp) dtypep(fromp->dtypep());
    }
    ശൂന്യം dtypeChgSigned(bool flag = true);
    ശൂന്യം dtypeChgWidth(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin);
    ശൂന്യം dtypeChgWidthSigned(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric);
    ശൂന്യം dtypeSetBitUnsized(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) {
        dtypep(findBitDType(width, widthMin, numeric));
    }
    ശൂന്യം dtypeSetBitSized(പൂർണ്ണസംഖ്യ width, VSigning numeric) {
        dtypep(findBitDType(width, width, numeric));  // Since sized, widthMin is width
    }
    ശൂന്യം dtypeSetLogicUnsized(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) {
        dtypep(findLogicDType(width, widthMin, numeric));
    }
    ശൂന്യം dtypeSetLogicSized(പൂർണ്ണസംഖ്യ width, VSigning numeric) {
        dtypep(findLogicDType(width, width, numeric));  // Since sized, widthMin is width
    }
    ശൂന്യം dtypeSetLogic2StateUnsized(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) {
        dtypep(findLogic2StateDType(width, widthMin, numeric));
    }
    ശൂന്യം dtypeSetLogic2StateSized(പൂർണ്ണസംഖ്യ width, VSigning numeric) {
        dtypep(findLogic2StateDType(width, width, numeric));  // Since sized, widthMin is width
    }
    ശൂന്യം dtypeSetBitOrLogicUnsized(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, widthMin, numeric, isFourstate));
    }
    ശൂന്യം dtypeSetBitOrLogicSized(പൂർണ്ണസംഖ്യ width, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, width, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    ശൂന്യം dtypeSetBit() { dtypep(findBitDType()); }
    ശൂന്യം dtypeSetLogic() { dtypep(findLogicDType()); }
    ശൂന്യം dtypeSetLogic2State() { dtypep(findLogic2StateDType()); }
    ശൂന്യം dtypeSetDouble() { dtypep(findDoubleDType()); }
    ശൂന്യം dtypeSetInt() { dtypep(findIntDType()); }
    ശൂന്യം dtypeSetInteger() { dtypep(findIntegerDType()); }
    ശൂന്യം dtypeSetInteger2State() { dtypep(findInteger2StateDType()); }
    ശൂന്യം dtypeSetString() { dtypep(findStringDType()); }
    ശൂന്യം dtypeSetUInt32() { dtypep(findUInt32DType()); }  // Twostate
    ശൂന്യം dtypeSetUInt64() { dtypep(findUInt64DType()); }  // Twostate
    ശൂന്യം dtypeSetTime() { dtypep(findTimeDType()); }
    ശൂന്യം dtypeSetTime2State() { dtypep(findTime2StateDType()); }
    ശൂന്യം dtypeSetEmptyQueue() { dtypep(findEmptyQueueDType()); }
    ശൂന്യം dtypeSetStream() { dtypep(findStreamDType()); }
    ശൂന്യം dtypeSetVoid() { dtypep(findVoidDType()); }
    ശൂന്യം dtypeSetBitOrLogic(bool isFourstate) {
        dtypep(isFourstate ? findLogicDType() : findBitDType());
    }

    // Data type locators
    Ast_കണ്ണി_DType* findBitDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_കണ്ണി_DType* findLogicDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_കണ്ണി_DType* findNullDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_കണ്ണി_DType* findLogic2StateDType() സ്ഥിരം {
        തിരികെ findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_കണ്ണി_DType* findDoubleDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_കണ്ണി_DType* findIntDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::INT); }
    Ast_കണ്ണി_DType* findIntegerDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_കണ്ണി_DType* findInteger2StateDType() സ്ഥിരം {
        തിരികെ findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_കണ്ണി_DType* findStringDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_കണ്ണി_DType* findSigned8DType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_കണ്ണി_DType* findUInt32DType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_കണ്ണി_DType* findUInt64DType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_കണ്ണി_DType* findTimeDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_കണ്ണി_DType* findTime2StateDType() സ്ഥിരം {
        തിരികെ findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_കണ്ണി_DType* findCHandleDType() സ്ഥിരം { തിരികെ findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_കണ്ണി_DType* findConstraintRefDType() സ്ഥിരം;
    Ast_കണ്ണി_DType* findEmptyQueueDType() സ്ഥിരം;
    Ast_കണ്ണി_DType* findQueueIndexDType() സ്ഥിരം;
    Ast_കണ്ണി_DType* findStreamDType() സ്ഥിരം;
    Ast_കണ്ണി_DType* findVoidDType() സ്ഥിരം;
    Ast_കണ്ണി_DType* findBitDType(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) സ്ഥിരം;
    Ast_കണ്ണി_DType* findLogicDType(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) സ്ഥിരം;
    Ast_കണ്ണി_DType* findLogic2StateDType(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) സ്ഥിരം;
    Ast_കണ്ണി_DType* findBitOrLogicDType(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric,
                                      bool isFourstate) സ്ഥിരം {
        തിരികെ isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_കണ്ണി_DType* findBitOrLogicRangeDType(സ്ഥിരം VNumRange& range, സ്ഥിരം പൂർണ്ണസംഖ്യ widthMin,
                                           സ്ഥിരം VSigning numeric,
                                           സ്ഥിരം bool isFourstate) സ്ഥിരം VL_MT_STABLE {
        തിരികെ isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_കണ്ണി_DType* findLogicRangeDType(സ്ഥിരം VNumRange& range, പൂർണ്ണസംഖ്യ widthMin,
                                      VSigning numeric) സ്ഥിരം VL_MT_STABLE;
    Ast_കണ്ണി_DType* findBitRangeDType(സ്ഥിരം VNumRange& range, പൂർണ്ണസംഖ്യ widthMin,
                                    VSigning numeric) സ്ഥിരം VL_MT_STABLE;
    Ast_കണ്ണി_DType* findBasicDType(VBasicDTypeKwd kwd) സ്ഥിരം;
    സ്ഥിതം AstBasicDType* find_തിരുകുക_Same_DType(AstBasicDType* nodep);

    സ്ഥിതം VCastable computeCastable(സ്ഥിരം Ast_കണ്ണി_DType* toDtp, സ്ഥിരം Ast_കണ്ണി_DType* fromDtp,
                                     സ്ഥിരം Ast_കണ്ണി* fromConstp, bool checkIfaceArgCompat = false);
    സ്ഥിതം Ast_കണ്ണി_DType* getCommonClassTypep(Ast_കണ്ണി* nodep1, Ast_കണ്ണി* nodep2);

    // METHODS - dump and error
    ശൂന്യം v3errorEnd(സ്ഥിരം std::ostringstream& str) സ്ഥിരം VL_RELEASE(V3Error::s().m_mutex);
    ശൂന്യം v3errorEndFatal(സ്ഥിരം std::ostringstream& str) സ്ഥിരം VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex) {
        തിരികെ fileline()->warnContextPrimary();
    }
    string warnContextSecondary() സ്ഥിരം { തിരികെ fileline()->warnContextSecondary(); }
    string warnMore() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex) { തിരികെ fileline()->warnMore(); }
    string warnOther() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex) { തിരികെ fileline()->warnOther(); }

    മായാ ശൂന്യം dump(std::ostream& str = std::cout) സ്ഥിരം;
    സ്ഥിതം അക്ഷരം* dump_വൃക്ഷം_Json_Gdb(സ്ഥിരം Ast_കണ്ണി* nodep);  // For GDB only, free()d by caller
    സ്ഥിതം അക്ഷരം* dump_വൃക്ഷം_Json_Gdb(സ്ഥിരം അക്ഷരം* str);  // For GDB only, free()d by caller
    സ്ഥിതം അക്ഷരം* dump_വൃക്ഷം_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    സ്ഥിതം ശൂന്യം dumpGdb(സ്ഥിരം Ast_കണ്ണി* nodep);  // For GDB only
    ശൂന്യം dumpGdbHeader() സ്ഥിരം;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    template <typename T_കണ്ണി_Result, typename T_കണ്ണി_Next>
    സ്ഥിതം T_കണ്ണി_Result* addNext(T_കണ്ണി_Result* nodep, T_കണ്ണി_Next* newp) {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി_Result>::മൂല്യം,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_കണ്ണി_Result, T_കണ്ണി_Next>::മൂല്യം,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        എങ്കിൽ (!newp) തിരികെ nodep;
        തിരികെ static_cast<T_കണ്ണി_Result*>(addNext<Ast_കണ്ണി, Ast_കണ്ണി>(nodep, newp));
    }
    template <typename T_കണ്ണി_Result, typename T_കണ്ണി_Next>
    സ്ഥിതം T_കണ്ണി_Result* addNextNull(T_കണ്ണി_Result* nodep, T_കണ്ണി_Next* newp) {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി_Result>::മൂല്യം,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_കണ്ണി_Result, T_കണ്ണി_Next>::മൂല്യം,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        എങ്കിൽ (!newp) തിരികെ nodep;
        എങ്കിൽ (!nodep) തിരികെ newp;
        തിരികെ static_cast<T_കണ്ണി_Result*>(addNext<Ast_കണ്ണി, Ast_കണ്ണി>(nodep, newp));
    }
    വരിയിൽ Ast_കണ്ണി* addNext(Ast_കണ്ണി* newp);
    ശൂന്യം addNextHere(Ast_കണ്ണി* newp);  // Insert newp at this->nextp
    ശൂന്യം addHereThisAsNext(Ast_കണ്ണി* newp);  // Adds at old place of this, this becomes next
    ശൂന്യം replaceWith(Ast_കണ്ണി* newp);  // Replace current node in tree with new node
    ശൂന്യം replaceWithKeepDType(Ast_കണ്ണി* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_കണ്ണി* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_കണ്ണി* unlinkFrBackWithNext(VNRelinker* linkerp = nullptr);
    ശൂന്യം relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    ശൂന്യം clone_Relink_കണ്ണി() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_കണ്ണി* clone_വൃക്ഷം(bool cloneNextLink,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_കണ്ണി* clone_വൃക്ഷം_Pure(bool cloneNextLink) { തിരികെ clone_വൃക്ഷം(cloneNextLink, true); }
    bool gate_വൃക്ഷം() { തിരികെ gate_വൃക്ഷം_Iter(); }  // Is tree isGateOptimizable?
    വരിയിൽ bool same_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* node2p) സ്ഥിരം;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    വരിയിൽ bool same_Gate_വൃക്ഷം(സ്ഥിരം Ast_കണ്ണി* node2p) സ്ഥിരം;
    ശൂന്യം delete_വൃക്ഷം();  // Always deletes the next link
    ശൂന്യം check_വൃക്ഷം() സ്ഥിരം VL_MT_STABLE {
        എങ്കിൽ (v3Global.opt.debugCheck()) check_വൃക്ഷം_Iter(backp());
    }
    ശൂന്യം checkIter() സ്ഥിരം;
    ശൂന്യം dumpPtrs(std::ostream& os = std::cout) സ്ഥിരം;
    ശൂന്യം dump_വൃക്ഷം(std::ostream& os = std::cout, സ്ഥിരം string& indent = "    ",
                  പൂർണ്ണസംഖ്യ maxDepth = 0) സ്ഥിരം;
    ശൂന്യം dump_വൃക്ഷം(സ്ഥിരം string& indent, പൂർണ്ണസംഖ്യ maxDepth = 0) സ്ഥിരം {
        dump_വൃക്ഷം(std::cout, indent, maxDepth);
    }
    സ്ഥിതം ശൂന്യം dump_വൃക്ഷം_Gdb(സ്ഥിരം Ast_കണ്ണി* nodep);  // For GDB only
    ശൂന്യം dump_വൃക്ഷം_And_Next(std::ostream& os = std::cout, സ്ഥിരം string& indent = "    ",
                         പൂർണ്ണസംഖ്യ maxDepth = 0) സ്ഥിരം;
    ശൂന്യം dump_വൃക്ഷം_File(സ്ഥിരം string& filename, bool doDump = true);
    സ്ഥിതം ശൂന്യം dump_വൃക്ഷം_File_Gdb(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം അക്ഷരം* filenamep = nullptr);
    ശൂന്യം dump_വൃക്ഷം_Dot(std::ostream& os = std::cout) സ്ഥിരം;
    ശൂന്യം dump_വൃക്ഷം_Dot_File(സ്ഥിരം string& filename, bool doDump = true);
    മായാ ശൂന്യം dumpJson(std::ostream& os) സ്ഥിരം { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    ശൂന്യം dumpJsonGen(std::ostream& os) സ്ഥിരം {};
    മായാ ശൂന്യം dump_വൃക്ഷം_Json_Op_Gen(std::ostream& os, സ്ഥിരം string& indent) സ്ഥിരം {};
    ശൂന്യം dump_വൃക്ഷം_Json(std::ostream& os, സ്ഥിരം string& indent = "") സ്ഥിരം;
    ശൂന്യം dump_വൃക്ഷം_Json_File(സ്ഥിരം string& filename, bool doDump = true);
    സ്ഥിതം ശൂന്യം dumpJsonMetaFileGdb(സ്ഥിരം അക്ഷരം* filename);
    സ്ഥിതം ശൂന്യം dumpJsonMetaFile(സ്ഥിരം string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    സ്ഥിതം std::string കണ്ണി_Addr(സ്ഥിരം Ast_കണ്ണി* nodep) {
        തിരികെ v3Global.opt.dump_വൃക്ഷം_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    സ്ഥിതം Ast_കണ്ണി* afterCommentp(Ast_കണ്ണി* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        ആകുമ്പോൾ (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        തിരികെ nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    മായാ bool isBrancher() സ്ഥിരം { തിരികെ false; }
    // Else a AstTime etc that can't be pushed out
    മായാ bool isGateOptimizable() സ്ഥിരം { തിരികെ !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    മായാ bool isGateDedupable() സ്ഥിരം { തിരികെ isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    മായാ bool is_വ്യഞ്ജകം_Coverage_Eligible() സ്ഥിരം { തിരികെ isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    മായാ bool isOutputter() { തിരികെ false; }
    // Else a AstTime etc which output can't be predicted from input
    മായാ bool isPredictOptimizable() സ്ഥിരം { തിരികെ !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    മായാ bool isPure() { തിരികെ true; }
    // Iff isPure on current node and any nextp()
    bool isPureAndNext() { തിരികെ isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    മായാ bool isTimingControl() സ്ഥിരം { തിരികെ false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    മായാ bool isUnlikely() സ്ഥിരം { തിരികെ false; }
    // Is an IEEE system function (versus internally-generated)
    മായാ bool isSystemFunc() സ്ഥിരം { തിരികെ false; }
    മായാ പൂർണ്ണസംഖ്യ instr_എണ്ണം() സ്ഥിരം { തിരികെ 0; }
    // Iff node is identical to another node
    മായാ bool isSame(സ്ഥിരം Ast_കണ്ണി* samep) സ്ഥിരം {
        തിരികെ type() == samep->type() && same_കണ്ണി(samep);
    }
    // Iff has a data type; dtype() must be non-null
    മായാ bool hasDType() സ്ഥിരം VL_MT_SAFE { തിരികെ false; }
    // Iff has a non-null childDTypep(), as generic node function
    മായാ Ast_കണ്ണി_DType* getChildDTypep() സ്ഥിരം { തിരികെ nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    മായാ Ast_കണ്ണി_DType* getChild2DTypep() സ്ഥിരം { തിരികെ nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    മായാ bool maybePointedTo() സ്ഥിരം VL_MT_SAFE { തിരികെ false; }
    // Don't reclaim this node in V3Dead
    മായാ bool undead() സ്ഥിരം { തിരികെ false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    മായാ സ്ഥിരം അക്ഷരം* broken() സ്ഥിരം { തിരികെ nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    മായാ സ്ഥിരം അക്ഷരം* brokenGen() സ്ഥിരം = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം newp) സ്ഥിരം { തിരികെ backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    മായാ ശൂന്യം accept(VNVisitorConst& v) = 0;

സംരക്ഷിത:
    // All VNVisitor related functions are called as methods off the visitor
    friend class VNVisitor;
    friend class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    ശൂന്യം iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    ശൂന്യം iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    ശൂന്യം iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    ശൂന്യം iterateAndNext(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    ശൂന്യം iterateAndNextConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_കണ്ണി* iterateSubtreeReturnEdits(VNVisitor& v);

    സ്ഥിതം ശൂന്യം dump_Json_അക്കം(std::ostream& os, സ്ഥിരം std::string& name, int64_t val);
    സ്ഥിതം ശൂന്യം dumpJsonBoolIf(std::ostream& os, സ്ഥിരം std::string& name, bool val);
    സ്ഥിതം ശൂന്യം dumpJsonStr(std::ostream& os, സ്ഥിരം std::string& name, സ്ഥിരം std::string& val);
    സ്ഥിതം ശൂന്യം dumpJsonPtr(std::ostream& os, സ്ഥിരം std::string& name, സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം valp);

സംരക്ഷിത:
    ശൂന്യം iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    template <typename T>
    വരിയിൽ സ്ഥിതം bool privateTypeTest(സ്ഥിരം Ast_കണ്ണി* nodep);

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr സ്ഥിതം bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        തിരികെ std::is_base_of<T_TargetType, NonCV>::മൂല്യം;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr സ്ഥിതം bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        തിരികെ !std::is_base_of<NonCV, T_TargetType>::മൂല്യം;
    }  // LCOV_EXCL_STOP

പൊതു:
    // For use via the VN_IS macro only, or in templated code
    template <typename T_TargetType, typename T_കണ്ണി>
    സ്ഥിതം bool is(സ്ഥിരം T_കണ്ണി* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        തിരികെ nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    template <typename T_TargetType, typename T_കണ്ണി>
    സ്ഥിതം T_TargetType* cast(T_കണ്ണി* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        തിരികെ nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    template <typename T_TargetType, typename T_കണ്ണി>
    സ്ഥിതം സ്ഥിരം T_TargetType* cast(സ്ഥിരം T_കണ്ണി* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        തിരികെ nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<സ്ഥിരം T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    template <typename T_TargetType, typename T_കണ്ണി>
    സ്ഥിതം T_TargetType* as(T_കണ്ണി* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        തിരികെ unsafePrivateAs<T_TargetType, T_കണ്ണി>(nodep);
    }
    template <typename T_TargetType, typename T_കണ്ണി>
    സ്ഥിതം സ്ഥിരം T_TargetType* as(സ്ഥിരം T_കണ്ണി* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        തിരികെ unsafePrivateAs<T_TargetType, T_കണ്ണി>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    template <typename T_TargetType, typename T_കണ്ണി>
    സ്ഥിതം T_TargetType* unsafePrivateAs(T_കണ്ണി* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        തിരികെ reinterpret_cast<T_TargetType*>(nodep);
    }
    template <typename T_TargetType, typename T_കണ്ണി>
    സ്ഥിതം സ്ഥിരം T_TargetType* unsafePrivateAs(സ്ഥിരം T_കണ്ണി* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_കണ്ണി>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        തിരികെ reinterpret_cast<സ്ഥിരം T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_കണ്ണി>
    സ്ഥിതം bool mayBeUnder(സ്ഥിരം Ast_കണ്ണി* nodep) {
        static_assert(!std::is_const<T_കണ്ണി>::മൂല്യം,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        തിരികെ true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_കണ്ണി>
    സ്ഥിതം constexpr bool isLeaf() {
        static_assert(!std::is_const<T_കണ്ണി>::മൂല്യം,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        തിരികെ false;
    }

സ്വകാര്യ:
    // Using std::conditional for const correctness in the public 'foreach' functions
    template <typename T_Arg>
    using Const_Correct_Ast_കണ്ണി =
        typename std::conditional<std::is_const<T_Arg>::മൂല്യം, സ്ഥിരം Ast_കണ്ണി, Ast_കണ്ണി>::type;

    template <typename T_Arg, typename T_Callable>
    വരിയിൽ സ്ഥിതം ശൂന്യം foreachImpl(Const_Correct_Ast_കണ്ണി<T_Arg>* nodep, സ്ഥിരം T_Callable& f,
                                   bool visitNext);

    template <typename T_Arg, bool N_Default, typename T_Callable>
    വരിയിൽ സ്ഥിതം bool predicateImpl(Const_Correct_Ast_കണ്ണി<T_Arg>* nodep, സ്ഥിരം T_Callable& p,
                                     bool visitNext);

പൊതു:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    template <typename T_Callable>
    ശൂന്യം foreach(T_Callable&& f) {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_കണ്ണി*>::മൂല്യം
                          && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_കണ്ണി>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    ശൂന്യം foreach(T_Callable&& f) സ്ഥിരം {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, സ്ഥിരം T_കണ്ണി*>::മൂല്യം
                && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<സ്ഥിരം T_കണ്ണി>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    ശൂന്യം foreachAndNext(T_Callable&& f) {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_കണ്ണി*>::മൂല്യം
                          && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_കണ്ണി>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    ശൂന്യം foreachAndNext(T_Callable&& f) സ്ഥിരം {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, സ്ഥിരം T_കണ്ണി*>::മൂല്യം
                && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<സ്ഥിരം T_കണ്ണി>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    template <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_കണ്ണി*>::മൂല്യം
                          && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        തിരികെ predicateImpl<T_കണ്ണി, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool exists(T_Callable&& p) സ്ഥിരം {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, സ്ഥിരം T_കണ്ണി*>::മൂല്യം
                          && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        തിരികെ predicateImpl<സ്ഥിരം T_കണ്ണി, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, സ്ഥിരം T_കണ്ണി*>::മൂല്യം
                          && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        തിരികെ predicateImpl<T_കണ്ണി, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) സ്ഥിരം {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, സ്ഥിരം T_കണ്ണി*>::മൂല്യം
                          && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        തിരികെ predicateImpl<സ്ഥിരം T_കണ്ണി, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    template <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_കണ്ണി*>::മൂല്യം
                          && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        തിരികെ predicateImpl<T_കണ്ണി, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool forall(T_Callable&& p) സ്ഥിരം {
        using T_കണ്ണി = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, സ്ഥിരം T_കണ്ണി*>::മൂല്യം
                          && std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        തിരികെ predicateImpl<സ്ഥിരം T_കണ്ണി, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    പൂർണ്ണസംഖ്യ കണ്ണി_എണ്ണം() സ്ഥിരം {
        // TODO: this should really return size_t, but need to fix use sites
        പൂർണ്ണസംഖ്യ എണ്ണം = 0;
        this->foreach([&എണ്ണം](സ്ഥിരം Ast_കണ്ണി*) { ++എണ്ണം; });
        തിരികെ എണ്ണം;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(പൂർണ്ണസംഖ്യ limit) സ്ഥിരം {
        പൂർണ്ണസംഖ്യ എണ്ണം = 0;
        തിരികെ this->exists([&എണ്ണം, limit](സ്ഥിരം Ast_കണ്ണി*) { തിരികെ ++എണ്ണം > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
template <>
Ast_കണ്ണി* Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(Ast_കണ്ണി* nodep, Ast_കണ്ണി* newp);

// Inline method implementations
Ast_കണ്ണി* Ast_കണ്ണി::addNext(Ast_കണ്ണി* newp) { തിരികെ addNext(this, newp); }

// Specializations of privateTypeTest
#അന്തര്ഭാവ "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
template <>
വരിയിൽ bool Ast_കണ്ണി::mayBeUnder<AstCell>(സ്ഥിരം Ast_കണ്ണി* nodep) {
    തിരികെ !VN_IS(nodep, കണ്ണി_Stmt) && !VN_IS(nodep, കണ്ണി_വ്യഞ്ജകം);
}
template <>
വരിയിൽ bool Ast_കണ്ണി::mayBeUnder<Ast_കണ്ണി_Assign>(സ്ഥിരം Ast_കണ്ണി* nodep) {
    തിരികെ !VN_IS(nodep, കണ്ണി_വ്യഞ്ജകം);
}
template <>
വരിയിൽ bool Ast_കണ്ണി::mayBeUnder<AstVarScope>(സ്ഥിരം Ast_കണ്ണി* nodep) {
    എങ്കിൽ (VN_IS(nodep, VarScope)) തിരികെ false;  // Should not nest
    എങ്കിൽ (VN_IS(nodep, Var)) തിരികെ false;
    എങ്കിൽ (VN_IS(nodep, Active)) തിരികെ false;
    എങ്കിൽ (VN_IS(nodep, കണ്ണി_Stmt)) തിരികെ false;
    എങ്കിൽ (VN_IS(nodep, കണ്ണി_വ്യഞ്ജകം)) തിരികെ false;
    തിരികെ true;
}
template <>
വരിയിൽ bool Ast_കണ്ണി::mayBeUnder<AstExecGraph>(സ്ഥിരം Ast_കണ്ണി* nodep) {
    എങ്കിൽ (VN_IS(nodep, ExecGraph)) തിരികെ false;  // Should not nest
    എങ്കിൽ (VN_IS(nodep, കണ്ണി_Stmt)) തിരികെ false;  // Should be directly under CFunc
    തിരികെ true;
}
template <>
വരിയിൽ bool Ast_കണ്ണി::mayBeUnder<AstActive>(സ്ഥിരം Ast_കണ്ണി* nodep) {
    തിരികെ !VN_IS(nodep, Active);  // AstActives do not nest
}
template <>
വരിയിൽ bool Ast_കണ്ണി::mayBeUnder<AstScope>(സ്ഥിരം Ast_കണ്ണി* nodep) {
    തിരികെ !VN_IS(nodep, Scope);  // AstScopes do not nest
}
template <>
വരിയിൽ bool Ast_കണ്ണി::mayBeUnder<Ast_Sen_വൃക്ഷം>(സ്ഥിരം Ast_കണ്ണി* nodep) {
    തിരികെ !VN_IS(nodep, Sen_വൃക്ഷം);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
template <>
constexpr bool Ast_കണ്ണി::isLeaf<Ast_കണ്ണി_Var_Ref>() {
    തിരികെ true;
}
template <>
constexpr bool Ast_കണ്ണി::isLeaf<AstVarRef>() {
    തിരികെ true;
}
template <>
constexpr bool Ast_കണ്ണി::isLeaf<AstVarXRef>() {
    തിരികെ true;
}

// foreach implementation
template <typename T_Arg, typename T_Callable>
ശൂന്യം Ast_കണ്ണി::foreachImpl(Const_Correct_Ast_കണ്ണി<T_Arg>* nodep, സ്ഥിരം T_Callable& f, bool visitNext) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using കണ്ണി_2 = Const_Correct_Ast_കണ്ണി<T_Arg>;

    // We prefetch this far into the stack
    constexpr പൂർണ്ണസംഖ്യ PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_അട്ടി_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t അട്ടി_Size = Initial_അട്ടി_Size;
    കണ്ണി_2* in_വരി_അട്ടി[Initial_അട്ടി_Size];  // In-line initial stack storage
    std::unique_ptr<കണ്ണി_2*[]> out_Of_വരി_Stackp{nullptr};  // In case we need more, we will allocate
    കണ്ണി_2** basep = in_വരി_അട്ടി + PrefetchDistance;  // Pointer to base of stack
    കണ്ണി_2** topp = basep;  // Pointer to top of stack
    കണ്ണി_2** limp = in_വരി_അട്ടി + അട്ടി_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    സ്ഥിരം auto visit = [&](കണ്ണി_2* currp) {
        // Type test this node
        എങ്കിൽ (Ast_കണ്ണി::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            എങ്കിൽ VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) തിരികെ;
        }

        // Enqueue children for traversal, unless futile
        എങ്കിൽ (mayBeUnder<T_Arg_NonConst>(currp)) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം op4p = currp->op4p()) *topp++ = op4p;
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം op3p = currp->op3p()) *topp++ = op3p;
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം op2p = currp->op2p()) *topp++ = op2p;
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    എങ്കിൽ (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    ആകുമ്പോൾ (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        കണ്ണി_2* സ്ഥിരം headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        എങ്കിൽ (VL_UNLIKELY(topp >= limp)) {
            സ്ഥിരം ptrdiff_t occupancy = topp - basep;
            {
                സ്ഥിരം size_t new_അട്ടി_Size = അട്ടി_Size * 2;
                കണ്ണി_2** സ്ഥിരം newStackp = new കണ്ണി_2*[new_അട്ടി_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            വലുപ്പം(കണ്ണി_2**) * (occupancy + PrefetchDistance));
                അട്ടി_Size = new_അട്ടി_Size;
                out_Of_വരി_Stackp.reset(newStackp);
            }
            basep = out_Of_വരി_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_വരി_Stackp.get() + അട്ടി_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        എങ്കിൽ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
template <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_കണ്ണി::predicateImpl(Const_Correct_Ast_കണ്ണി<T_Arg>* nodep, സ്ഥിരം T_Callable& p,
                            bool visitNext) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using കണ്ണി_2 = Const_Correct_Ast_കണ്ണി<T_Arg>;

    // We prefetch this far into the stack
    constexpr പൂർണ്ണസംഖ്യ PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_അട്ടി_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t അട്ടി_Size = Initial_അട്ടി_Size;
    കണ്ണി_2* in_വരി_അട്ടി[Initial_അട്ടി_Size];  // In-line initial stack storage
    std::unique_ptr<കണ്ണി_2*[]> out_Of_വരി_Stackp{nullptr};  // In case we need more, we will allocate
    കണ്ണി_2** basep = in_വരി_അട്ടി + PrefetchDistance;  // Pointer to base of stack
    കണ്ണി_2** topp = basep;  // Pointer to top of stack
    കണ്ണി_2** limp = in_വരി_അട്ടി + അട്ടി_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    സ്ഥിരം auto visit = [&](കണ്ണി_2* currp) {
        // Type test this node
        എങ്കിൽ (Ast_കണ്ണി::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            എങ്കിൽ (p(static_cast<T_Arg*>(currp)) != N_Default) തിരികെ true;
            // Short circuit if iterating leaf nodes
            എങ്കിൽ VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) തിരികെ false;
        }

        // Enqueue children for traversal, unless futile
        എങ്കിൽ (mayBeUnder<T_Arg_NonConst>(currp)) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം op4p = currp->op4p()) *topp++ = op4p;
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം op3p = currp->op3p()) *topp++ = op3p;
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം op2p = currp->op2p()) *topp++ = op2p;
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം op1p = currp->op1p()) *topp++ = op1p;
        }

        തിരികെ false;
    };

    // Enqueue the next of the root node, if required
    എങ്കിൽ (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    എങ്കിൽ (visit(nodep)) തിരികെ !N_Default;

    // Visit the rest of the tree
    ആകുമ്പോൾ (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        കണ്ണി_2* സ്ഥിരം headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        എങ്കിൽ (VL_UNLIKELY(topp >= limp)) {
            സ്ഥിരം ptrdiff_t occupancy = topp - basep;
            {
                സ്ഥിരം size_t new_അട്ടി_Size = അട്ടി_Size * 2;
                കണ്ണി_2** സ്ഥിരം newStackp = new കണ്ണി_2*[new_അട്ടി_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            വലുപ്പം(കണ്ണി_2**) * (occupancy + PrefetchDistance));
                അട്ടി_Size = new_അട്ടി_Size;
                out_Of_വരി_Stackp.reset(newStackp);
            }
            basep = out_Of_വരി_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_വരി_Stackp.get() + അട്ടി_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        എങ്കിൽ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        എങ്കിൽ (visit(headp)) തിരികെ !N_Default;
    }

    തിരികെ N_Default;
}

വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം Ast_കണ്ണി* rhs) {
    എങ്കിൽ (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } അല്ലെങ്കിൽ {
        rhs->dump(os);
    }
    തിരികെ os;
}
ശൂന്യം VNRelinker::relink(Ast_കണ്ണി* newp) { newp->Ast_കണ്ണി::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
template <typename T_കണ്ണി>
class VNRef final : പൊതു std::reference_wrapper<T_കണ്ണി> {
    static_assert(std::is_base_of<Ast_കണ്ണി, T_കണ്ണി>::മൂല്യം,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

പൊതു:
    template <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_കണ്ണി>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(സ്ഥിരം std::reference_wrapper<T_കണ്ണി>& other)
        : std::reference_wrapper<T_കണ്ണി>{other} {}
};

static_assert(വലുപ്പം(VNRef<Ast_കണ്ണി>) == വലുപ്പം(std::reference_wrapper<Ast_കണ്ണി>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
size_t V3HasherUncachedHash(സ്ഥിരം Ast_കണ്ണി&);

// Specialization of std::hash for VNRef
template <typename T_കണ്ണി>
ഘടന std::hash<VNRef<T_കണ്ണി>> final {
    size_t operator()(VNRef<T_കണ്ണി> r) സ്ഥിരം { തിരികെ V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
template <typename T_കണ്ണി>
ഘടന std::equal_to<VNRef<T_കണ്ണി>> final {
    size_t operator()(VNRef<T_കണ്ണി> ra, VNRef<T_കണ്ണി> rb) സ്ഥിരം {
        തിരികെ ra.get().same_വൃക്ഷം(&(rb.get()));
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
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി*, Ast_കണ്ണി*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി*, std::vector<Ast_കണ്ണി*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<സ്ഥിരം Ast_കണ്ണി*, std::string> m_prefixes;
    സ്ഥിരം VDTypeNameScopes* സ്ഥിരം m_prevp;  // Index in use before this one
    സ്ഥിതം സ്ഥിരം VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    ശൂന്യം index(Ast_കണ്ണി* nodep, Ast_കണ്ണി* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

പൊതു:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    സ്ഥിതം സ്ഥിരം VDTypeNameScopes* currentp() { തിരികെ s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(സ്ഥിരം Ast_കണ്ണി* nodep, Ast_കണ്ണി*& outerpr) സ്ഥിരം;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    സ്ഥിരം std::vector<Ast_കണ്ണി*>* paramsp(സ്ഥിരം Ast_കണ്ണി* modp) സ്ഥിരം;
    // The prefix kept for scope 'scopep', or nullptr if none
    സ്ഥിരം std::string* prefixp(സ്ഥിരം Ast_കണ്ണി* scopep) സ്ഥിരം;
    // Keep 'prefix', final, for scope 'scopep'
    ശൂന്യം prefix(സ്ഥിരം Ast_കണ്ണി* scopep, സ്ഥിരം std::string& prefix) സ്ഥിരം;
};

//######################################################################
// Inline VNVisitor METHODS

ശൂന്യം VNVisitorConst::iterateConst(Ast_കണ്ണി* nodep) { nodep->accept(*this); }
ശൂന്യം VNVisitorConst::iterateConstNull(Ast_കണ്ണി* nodep) {
    എങ്കിൽ (VL_LIKELY(nodep)) nodep->accept(*this);
}
ശൂന്യം VNVisitorConst::iterateChildrenConst(Ast_കണ്ണി* nodep) { nodep->iterateChildrenConst(*this); }
ശൂന്യം VNVisitorConst::iterateChildrenBackwardsConst(Ast_കണ്ണി* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
ശൂന്യം VNVisitorConst::iterateAndNextConstNull(Ast_കണ്ണി* nodep) {
    എങ്കിൽ (VL_LIKELY(nodep)) nodep->iterateAndNextConst(*this);
}

ശൂന്യം VNVisitor::iterate(Ast_കണ്ണി* nodep) { nodep->accept(*this); }
ശൂന്യം VNVisitor::iterateNull(Ast_കണ്ണി* nodep) {
    എങ്കിൽ (VL_LIKELY(nodep)) nodep->accept(*this);
}
ശൂന്യം VNVisitor::iterateChildren(Ast_കണ്ണി* nodep) { nodep->iterateChildren(*this); }
ശൂന്യം VNVisitor::iterateAndNextNull(Ast_കണ്ണി* nodep) {
    എങ്കിൽ (VL_LIKELY(nodep)) nodep->iterateAndNext(*this);
}
Ast_കണ്ണി* VNVisitor::iterateSubtreeReturnEdits(Ast_കണ്ണി* nodep) {
    തിരികെ nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#അന്തര്ഭാവ "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#അന്തര്ഭാവ "V3AstNodeDType.h"
#അന്തര്ഭാവ "V3AstNodeOther.h"
#അന്തര്ഭാവ "V3AstNodeExpr.h"
#അന്തര്ഭാവ "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#അന്തര്ഭാവ "V3AstInlines.h"
ശൂന്യം dump_കണ്ണി_List_Json(std::ostream& os, സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം std::string& listName,
                      സ്ഥിരം string& indent);

#endif  // Guard
